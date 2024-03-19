/*
 * dvbsubtitle.c: DVB subtitles
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * Original author: Marco Schl��ler <marco@lordzodiac.de>
 * With some input from the "subtitle plugin" by Pekka Virtanen <pekka.virtanen@sci.fi>
 *
 * $Id: dvbsubtitle.cpp,v 1.1 2009/02/23 19:46:44 rhabarber1848 Exp $
 * dvbsubtitle for HD1 ported by Coolstream LTD
 */

#include "dvbsubtitle.h"

extern "C" {
#include <unistd.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}
#include "driver/gfx/framebuffer.h"


#if LIBAVCODEC_VERSION_INT <= AV_VERSION_INT(57, 1, 99)
#define CODEC_DVB_SUB CODEC_ID_DVB_SUBTITLE
#else
#define CODEC_DVB_SUB AV_CODEC_ID_DVB_SUBTITLE
#endif

//// cDvbSubtitleBitmaps
class cDvbSubtitleBitmaps : public cListObject 
{
	private:
		int64_t pts;
		int timeout;
		AVSubtitle sub;
	public:
		cDvbSubtitleBitmaps(int64_t Pts);
		~cDvbSubtitleBitmaps();
		int64_t Pts(void) { return pts; }
		int Timeout(void) { return sub.end_display_time; }
		void Draw(int &min_x, int &min_y, int &max_x, int &max_y);
		int Count(void) { return sub.num_rects; };
		AVSubtitle * GetSub(void) { return &sub; };
};

cDvbSubtitleBitmaps::cDvbSubtitleBitmaps(int64_t pPts)
{
//	printf("cDvbSubtitleBitmaps::new: PTS: %lld\n", pts);
	pts = pPts;
}

cDvbSubtitleBitmaps::~cDvbSubtitleBitmaps()
{
//	printf("cDvbSubtitleBitmaps::delete: PTS: %lld rects %d\n", pts, Count());
	
	avsubtitle_free(&sub);
}

void cDvbSubtitleBitmaps::Draw(int &min_x, int &min_y, int &max_x, int &max_y)
{
	int i;
	
	int stride = CFrameBuffer::getInstance()->getScreenWidth(true);
	int wd = CFrameBuffer::getInstance()->getScreenWidth();
	int xstart = CFrameBuffer::getInstance()->getScreenX();
	int yend = CFrameBuffer::getInstance()->getScreenY() + CFrameBuffer::getInstance()->getScreenHeight();
	int ystart = CFrameBuffer::getInstance()->getScreenY();

	//printf("cDvbSubtitleBitmaps::Draw: %d bitmaps, x= %d, width= %d yend=%d stride %d\n", Count(), xstart, wd, yend, stride);

	int sw = CFrameBuffer::getInstance()->getScreenWidth(true);
	int sh = CFrameBuffer::getInstance()->getScreenHeight(true);

	for (i = 0; i < Count(); i++) 
	{
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(59,0,100)
		uint32_t *colors = (uint32_t *) sub.rects[i]->pict.data[1];
#else
		uint32_t *colors = (uint32_t *) sub.rects[i]->data[1];
#endif
		int width = sub.rects[i]->w;
		int height = sub.rects[i]->h;

		int h2 = (width == 1280) ? 720 : 576;
		
		int xoff = sub.rects[i]->x * sw / width;
		int yoff = sub.rects[i]->y * sh / h2;
		int nw = width * sw / width;
		int nh = height * sh / h2;

		// resize color to 32 bit
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57, 5, 0)
		fb_pixel_t *newdata = simple_resize32(sub.rects[i]->pict.data[0], colors, sub.rects[i]->nb_colors, width, height, nw, nh);
#else
		fb_pixel_t *newdata = simple_resize32(sub.rects[i]->data[0], colors, sub.rects[i]->nb_colors, width, height, nw, nh);
#endif
		
		// blit2fb
		CFrameBuffer::getInstance()->blit2FB(newdata, nw, nh, xoff, yoff, 0, 0, false);
		CFrameBuffer::getInstance()->blit();

		free(newdata);

		// recalculate min_x min_y max_x max_y
		if(min_x > xoff)
			min_x = xoff;

		if(min_y > yoff)
			min_y = yoff;

		if(max_x < (xoff + nw))
			max_x = xoff + nw;

		if(max_y < (yoff + nh))
			max_y = yoff + nh;
	}
}

// --- cDvbSubtitleConverter -------------------------------------------------

cDvbSubtitleConverter::cDvbSubtitleConverter(void)
{
	printf("cDvbSubtitleConverter: new converter\n");

	bitmaps = new cList<cDvbSubtitleBitmaps>;
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK_NP);
	pthread_mutex_init(&mutex, &attr);
	running = false;

	avctx = NULL;
	avcodec = NULL;

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 9, 100)
	avcodec_register_all();
#endif
	avcodec = avcodec_find_decoder(CODEC_DVB_SUB);//CODEC_ID_DVB_SUBTITLE or AV_CODEC_ID_DVB_SUBTITLE from 57.1.100

	if (!avcodec) 
	{
		printf("cDvbSubtitleConverter: unable to get dvb subtitle codec!\n");
		return;
	}

#if LIBAVCODEC_VERSION_MAJOR < 54
	avctx = avcodec_alloc_context();
#else
	avctx = avcodec_alloc_context3(avcodec);
#endif	

	if (!avctx) 
	{
		printf("cDvbSubtitleConverter: unable to get dvb subtitle map!\n");
		return;
	}

#if LIBAVCODEC_VERSION_MAJOR < 54
	if (avcodec_open(avctx, avcodec) < 0)
#else	  
	if (avcodec_open2(avctx, avcodec, NULL) < 0) 
#endif	  
		printf("cDvbSubtitleConverter: unable to open codec !\n");

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52, 64, 0)
	av_log_set_level(AV_LOG_PANIC);
#else
	av_log_set_level(0);
#endif	
	
	min_x = CFrameBuffer::getInstance()->getScreenWidth(); 		/* screen width */
	min_y = CFrameBuffer::getInstance()->getScreenHeight();		/* screenheight */
	max_x = CFrameBuffer::getInstance()->getScreenX();		/* startX */
	max_y = CFrameBuffer::getInstance()->getScreenY();		/* startY */

	Timeout.Set(0xFFFF*1000);
}

cDvbSubtitleConverter::~cDvbSubtitleConverter()
{
	if (avctx) 
	{
		avcodec_close(avctx);
		av_free(avctx);
		avctx = NULL;
	}

	delete bitmaps;
}

void cDvbSubtitleConverter::Lock(void)
{
	pthread_mutex_lock(&mutex);
}

void cDvbSubtitleConverter::Unlock(void)
{
	pthread_mutex_unlock(&mutex);
}

void cDvbSubtitleConverter::Pause(bool pause)
{
	//printf("cDvbSubtitleConverter::Pause: %s\n", pause ? "pause" : "resume");

	if(pause) 
	{
		if(!running)
			return;
		Lock();
		Clear();
		running = false;
		Unlock();
		//Reset();
	} 
	else 
	{
		//Reset();
		running = true;
	}
}

void cDvbSubtitleConverter::Clear(void)
{
	//printf("cDvbSubtitleConverter::Clear: x=% d y= %d, w= %d, h= %d\n", min_x, min_y, max_x - min_x, max_y - min_y);

	if(running && (max_x - min_x > 0) && (max_y - min_y > 0)) 
	{
		CFrameBuffer::getInstance()->paintBackgroundBoxRel (min_x, min_y, max_x - min_x, max_y-min_y);
	
		CFrameBuffer::getInstance()->blit();		
	}
}

void cDvbSubtitleConverter::Reset(void)
{
	//printf("Converter reset\n");
	
	Lock();
	bitmaps->Clear();
	Unlock();
	Timeout.Set(0xFFFF*1000);
}

int cDvbSubtitleConverter::Convert(const unsigned char *Data, int Length, int64_t pts)
{
	AVPacket avpkt;
	int got_subtitle = 0;
	static cDvbSubtitleBitmaps *Bitmaps = NULL;

	if(!avctx) 
	{
		printf("cDvbSubtitleConverter::Convert: no context\n");
		
		return -1;
	}

	if(Bitmaps == NULL)
		Bitmaps = new cDvbSubtitleBitmaps(pts);

 	AVSubtitle * sub = Bitmaps->GetSub();

	av_init_packet(&avpkt);
	avpkt.data = (uint8_t*) Data;
	avpkt.size = Length;

	//printf("cDvbSubtitleConverter::Convert: sub %x pkt %x pts %lld\n", sub, &avpkt, pts);

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52, 64, 0)
	avcodec_decode_subtitle2(avctx, sub, &got_subtitle, &avpkt);
#else	
	avcodec_decode_subtitle(avctx, sub, &got_subtitle, avpkt.data, avpkt.size);
#endif

	//printf("cDvbSubtitleConverter::Convert: pts %lld subs ? %s, %d bitmaps\n", pts, got_subtitle? "yes" : "no", sub->num_rects);

	if(got_subtitle) 
	{
		bitmaps->Add(Bitmaps);
		Bitmaps = NULL;
	}

	return 0;
}

#define LimitTo32Bit(n) (n & 0x00000000FFFFFFFFL)
#define MAXDELTA 40000 // max. reasonable PTS/STC delta in ms
#define MIN_DISPLAY_TIME 1500
#define SHOW_DELTA 20
#ifdef __sh__
#define WAITMS 1500
#else
#define WAITMS 500
#endif

void dvbsub_get_stc(int64_t * STC);

int cDvbSubtitleConverter::Action(void)
{
	int WaitMs = WAITMS;

	if (!running)
		return 0;

	if(!avctx) 
	{
		printf("cDvbSubtitleConverter::Action: no context\n");

		return -1;
	}

	Lock();

	if (cDvbSubtitleBitmaps *sb = bitmaps->First()) 
	{
		int64_t STC;
		dvbsub_get_stc(&STC);
		int64_t Delta = 0;

		Delta = LimitTo32Bit(sb->Pts()) - LimitTo32Bit(STC);
		Delta /= 90; // STC and PTS are in 1/90000s
		
		//printf("cDvbSubtitleConverter::Action: PTS: %lld  STC: %lld (%lld) timeout: %d\n", sb->Pts(), STC, Delta, sb->Timeout());
		
		if (Delta <= MAXDELTA) 
		{
			if (Delta <= SHOW_DELTA) 
			{
				//printf("cDvbSubtitleConverter::Action: Got %d bitmaps, showing #%d\n", bitmaps->Count(), sb->Index() + 1);

				if (running) 
				{
					Clear();

					sb->Draw(min_x, min_y, max_x, max_y);

					Timeout.Set(sb->Timeout());
				}
				
				if(sb->Count())
					WaitMs = MIN_DISPLAY_TIME;
				bitmaps->Del(sb, true);				
			}
			else if (Delta < WaitMs)
				WaitMs = (Delta > SHOW_DELTA) ? Delta - SHOW_DELTA : Delta;
		}
		else
			bitmaps->Del(sb, true);		
	} 
	else 
	{
		if (Timeout.TimedOut()) 
		{
			printf("cDvbSubtitleConverter::Action: timeout, elapsed %lld\n", Timeout.Elapsed());
			
			Clear();
			Timeout.Set(0xFFFF*1000);
		}
	}

	Unlock();

	//if(WaitMs != WAITMS)
	//	printf("cDvbSubtitleConverter::Action: finish, WaitMs %d\n", WaitMs);

	return WaitMs*1000;
}

