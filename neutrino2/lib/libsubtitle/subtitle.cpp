#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <inttypes.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>

#include <syscall.h>

#include <cerrno>

#include <dmx_cs.h>	/* libdvbapi */

#include "semaphore.h"
#include "dvbsubtitle.h"

#include <zapit/frontend_c.h>


//// globals
static PacketQueue packet_queue;
static PacketQueue bitmap_queue;
//
static pthread_t threadReader;
static pthread_t threadDvbsub;
//
static pthread_cond_t readerCond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t readerMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t packetCond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t packetMutex = PTHREAD_MUTEX_INITIALIZER;
//
static int reader_running;
static int dvbsub_running;
static int dvbsub_paused = true;
static int dvbsub_pid;
static int dvbsub_stopped;
static int pid_change_req;
//
cDvbSubtitleConverter *dvbSubtitleConverter;
//
static void* reader_thread(void* arg);
static void* dvbsub_thread(void* arg);
static void clear_queue();
extern "C" void dvbsub_write(AVSubtitle *sub, int64_t pts);
static bool isEplayer = false;

//
int dvbsub_init() 
{
	printf("dvbsub_init: starting... tid %ld\n", syscall(__NR_gettid));
	
	int trc;

	reader_running = true;
	dvbsub_stopped = 1;
	pid_change_req = 1;
	
	// reader-Thread starten
	trc = pthread_create(&threadReader, 0, reader_thread, (void *) NULL);
	
	if (trc) 
	{
		fprintf(stderr, "[dvb-sub] failed to create reader-thread (rc=%d)\n", trc);
		reader_running = false;
		return -1;
	}

	dvbsub_running = true;
	
	// subtitle decoder-Thread starten
	trc = pthread_create(&threadDvbsub, 0, dvbsub_thread, NULL);

	if (trc) 
	{
		fprintf(stderr, "[dvb-sub] failed to create dvbsub-thread (rc=%d)\n", trc);
		dvbsub_running = false;
		return -1;
	}

	return(0);
}

int dvbsub_pause()
{
	if(reader_running) 
	{
		dvbsub_paused = true;
		
		if(dvbSubtitleConverter)
			dvbSubtitleConverter->Pause(true);

		printf("[dvb-sub] paused\n");
	}

	return 0;
}

int dvbsub_start(int pid, bool _isEplayer)
{
	isEplayer = _isEplayer;
	
	if (isEplayer && !dvbsub_paused)
		return 0;
		
	if (!isEplayer && !pid)
		pid = -1;
		
	if(!dvbsub_paused && (pid < 0)) 
	{
		return 0;
	}

	if (pid > -1 && pid != dvbsub_pid)
	{
		dvbsub_pause();
			
		if(dvbSubtitleConverter)
			dvbSubtitleConverter->Reset();
				
		dvbsub_pid = pid;
		pid_change_req = 1;
	}
	
	printf("[dvb-sub] start, stopped %d pid %x\n", dvbsub_stopped, dvbsub_pid);

	if(dvbsub_pid > -1) 
	{
		dvbsub_stopped = 0;
		dvbsub_paused = false;
		
		if(dvbSubtitleConverter)
			dvbSubtitleConverter->Pause(false);
			
		pthread_mutex_lock(&readerMutex);
		pthread_cond_broadcast(&readerCond);
		pthread_mutex_unlock(&readerMutex);
		
		printf("[dvb-sub] started with pid 0x%x\n", pid);
	}

	return 1;
}

int dvbsub_stop()
{
	dvbsub_pid = -1;
	
	if(reader_running) 
	{
		dvbsub_stopped = 1;
		dvbsub_pause();
		pid_change_req = 1;
	}

	return 0;
}

int dvbsub_getpid()
{
	return dvbsub_pid;
}

void dvbsub_setpid(int pid)
{
	if (!isEplayer && !pid)
		pid = -1;
		
	dvbsub_pid = pid;

	if(dvbsub_pid < 0)
		return;

	clear_queue();

	if(dvbSubtitleConverter)
		dvbSubtitleConverter->Reset();

	pid_change_req = 1;
	dvbsub_stopped = 0;

	pthread_mutex_lock(&readerMutex);
	pthread_cond_broadcast(&readerCond);
	pthread_mutex_unlock(&readerMutex);
}

int dvbsub_close()
{
	if(threadReader) 
	{
		dvbsub_pause();
		reader_running = false;
		dvbsub_stopped = 1;

		pthread_mutex_lock(&readerMutex);
		pthread_cond_broadcast(&readerCond);
		pthread_mutex_unlock(&readerMutex);

		pthread_join(threadReader, NULL);
		threadReader = 0;
	}
	
	if(threadDvbsub) 
	{
		dvbsub_running = false;

		pthread_mutex_lock(&packetMutex);
		pthread_cond_broadcast(&packetCond);
		pthread_mutex_unlock(&packetMutex);

		pthread_join(threadDvbsub, NULL);
		threadDvbsub = 0;
	}
	printf("[dvb-sub] stopped\n");

	return 0;
}

static cDemux * dmx;
extern void getPlayerPts(int64_t *);
void dvbsub_get_stc(int64_t * STC)
{
	if (isEplayer)
	{
		getPlayerPts(STC);
		return;
	}
	
	if(dmx)
		dmx->getSTC(STC);
}

static int64_t get_pts(unsigned char * packet)
{
	int64_t pts;
	int pts_dts_flag;

	pts_dts_flag = getbits(packet, 7*8, 2);
	
	if ((pts_dts_flag == 2) || (pts_dts_flag == 3)) 
	{
		pts = (uint64_t)getbits(packet, 9*8+4, 3) << 30;  /* PTS[32..30] */
		pts |= getbits(packet, 10*8, 15) << 15;           /* PTS[29..15] */
		pts |= getbits(packet, 12*8, 15);                 /* PTS[14..0] */
	} 
	else 
	{
		pts = 0;
	}
	
	return pts;
}

#define LimitTo32Bit(n) (n & 0x00000000FFFFFFFFL)

static int64_t get_pts_stc_delta(int64_t pts)
{
	int64_t stc, delta;

	dvbsub_get_stc(&stc);
	delta = LimitTo32Bit(pts) - LimitTo32Bit(stc);
	delta /= 90;
	
	return delta;
}

static void clear_queue()
{
	uint8_t* packet;
	cDvbSubtitleBitmaps *Bitmaps;

	pthread_mutex_lock(&packetMutex);
	
	while(packet_queue.size()) 
	{
		packet = packet_queue.pop();
		free(packet);
	}
	
	while (bitmap_queue.size())
	{
		Bitmaps = (cDvbSubtitleBitmaps *) bitmap_queue.pop();
		delete Bitmaps;
	}
	
	pthread_mutex_unlock(&packetMutex);
}

////
void dvbsub_write(AVSubtitle *sub, int64_t pts)
{
	pthread_mutex_lock(&packetMutex);
	cDvbSubtitleBitmaps *Bitmaps = new cDvbSubtitleBitmaps(pts);
	Bitmaps->SetSub(sub); // Note: this will copy sub, including all references. DON'T call avsubtitle_free() from the caller.
	memset(sub, 0, sizeof(AVSubtitle));
	bitmap_queue.push((unsigned char *) Bitmaps);
	pthread_cond_broadcast(&packetCond);
	pthread_mutex_unlock(&packetMutex);
}
////

static void* reader_thread(void * /*arg*/)
{
	printf("dvbsub_thread\n");
	
	uint8_t tmp[16];  /* actually 6 should be enough */
	int count;
	int len;
	uint16_t packlen;
	uint8_t* buf;

        dmx = new cDemux();
	
	dmx->Open(DMX_PES_CHANNEL, 64*1024, CZapit::getInstance()->getCurrentFrontend());	

	while (reader_running) 
	{
		if(dvbsub_stopped) 
		{
			dmx->Stop();

			pthread_mutex_lock(&packetMutex);
			pthread_cond_broadcast(&packetCond);
			pthread_mutex_unlock(&packetMutex);

			pthread_mutex_lock(&readerMutex );
			int ret = pthread_cond_wait(&readerCond, &readerMutex);
			pthread_mutex_unlock(&readerMutex);
			
			if(!reader_running)
				break;
				
			dvbsub_stopped = 0;
		}

		if(pid_change_req) 
		{
			pid_change_req = 0;
			clear_queue();
			dmx->Stop();
			//			
			dmx->Open(DMX_PES_CHANNEL, 64*1024, CZapit::getInstance()->getCurrentFrontend());				
			//
			dmx->pesFilter(dvbsub_pid);
			dmx->Start();
		}

		len = 0;
		count = 0;

		len = dmx->Read(tmp, 6, 1000);
		
		if(len <= 0)
			continue;

		if(memcmp(tmp, "\x00\x00\x01\xbd", 4)) 
		{
			continue;
		}
		count = 6;

		packlen =  getbits(tmp, 4*8, 16) + 6;

		buf = (uint8_t*) malloc(packlen);

		memcpy(buf, tmp, 6);
		
		// read rest of the packet
		while(count < packlen) 
		{
			len = dmx->Read(buf+count, packlen-count, 1000);
			
			if (len < 0) 
			{
				continue;
			} 
			else 
			{
				count += len;
			}
		}

		if(!dvbsub_stopped) 
		{
			// Packet now in memory
			packet_queue.push(buf);

			// wake up dvb thread
			pthread_mutex_lock(&packetMutex);
			pthread_cond_broadcast(&packetCond);
			pthread_mutex_unlock(&packetMutex);
		} 
		else 
		{
			free(buf);
			buf=NULL;
		}
	}

	dmx->Stop();
	delete dmx;
	dmx = NULL;

	pthread_exit(NULL);
}

static void* dvbsub_thread(void* /*arg*/)
{
	printf("dvbsub_thread\n");
	
	struct timespec restartWait;
	struct timeval now;
	
	if (!dvbSubtitleConverter)
		dvbSubtitleConverter = new cDvbSubtitleConverter;

	int timeout = 1000000;
	
	while(dvbsub_running) 
	{
		uint8_t* packet;
		int64_t pts;
		int dataoffset;
		int packlen;

		gettimeofday(&now, NULL);

		int ret = 0;
		now.tv_usec += (timeout == 0) ? 1000000 : timeout;   // add the timeout
		
		while (now.tv_usec >= 1000000) 
		{   
			// take care of an overflow
			now.tv_sec++;
			now.tv_usec -= 1000000;
		}
		
		restartWait.tv_sec = now.tv_sec;          // seconds
		restartWait.tv_nsec = now.tv_usec * 1000; // nano seconds

		pthread_mutex_lock( &packetMutex );
		ret = pthread_cond_timedwait( &packetCond, &packetMutex, &restartWait );
		pthread_mutex_unlock( &packetMutex );

		timeout = dvbSubtitleConverter->Action();

		if(packet_queue.size() == 0 && bitmap_queue.size() == 0) 
		{
			continue;
		}

		//
		if(dvbsub_stopped) 
		{
			clear_queue();
			continue;
		}

		if (packet_queue.size())
		{
		pthread_mutex_lock(&packetMutex);
		packet = packet_queue.pop();
		pthread_mutex_unlock(&packetMutex);

		if (!packet) 
		{
			continue;
		}
		
		packlen = (packet[4] << 8 | packet[5]) + 6;

		pts = get_pts(packet);

		dataoffset = packet[8] + 8 + 1;
		
		if (packet[dataoffset] != 0x20) 
		{
			goto next_round;
		}

		if (packlen <= dataoffset + 3) 
		{
			goto next_round;
		}

		if (packet[dataoffset + 2] == 0x0f) 
		{
			dvbSubtitleConverter->Convert(&packet[dataoffset + 2], packlen - (dataoffset + 2), pts);
		} 
		
//		timeout = dvbSubtitleConverter->Action();

next_round:
		if (packet)
			free(packet);
			
		}
		else
		{
			cDvbSubtitleBitmaps *Bitmaps = (cDvbSubtitleBitmaps *) bitmap_queue.pop();
			pthread_mutex_unlock(&packetMutex);
			dvbSubtitleConverter->Convert(Bitmaps->GetSub(), Bitmaps->Pts());
		}
		
		timeout = dvbSubtitleConverter->Action();
	}

	delete dvbSubtitleConverter;
	
	pthread_exit(NULL);
}

