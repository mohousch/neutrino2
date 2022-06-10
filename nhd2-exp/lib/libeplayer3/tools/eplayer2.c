/*
 * eplayer3: command line playback using libeplayer3
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "common.h"
#include "subtitle.h"

extern OutputHandler_t        OutputHandler;
extern PlaybackHandler_t    PlaybackHandler;
extern ContainerHandler_t    ContainerHandler;
extern ManagerHandler_t        ManagerHandler;

Context_t * player = NULL;

/* ******************************************** */
/* Framebuffer for subtitle                     */
/* ******************************************** */
static int               fd  = -1;
static unsigned char*    lfb = NULL;
struct fb_fix_screeninfo fix;
struct fb_var_screeninfo screeninfo, oldscreen;

static int               stride = 0;
static int               xRes   = 0;
static int               yRes   = 0;
static int               bpp    = 0;

int kbhit(void) {
        struct timeval tv;
        fd_set read_fd;

        tv.tv_sec=1;
        tv.tv_usec=0;

        FD_ZERO(&read_fd);
        FD_SET(0,&read_fd);

        if(select(1, &read_fd, NULL, NULL, &tv) == -1)
                return 0;

        if(FD_ISSET(0,&read_fd))
                return 1;

        return 0;
}

void framebuffer_init()
{
	int available  = 0;

	fd = open("/dev/fb0", O_RDWR);

	if (fd < 0) 
    {
		perror("/dev/fb0");
		return;
	}

	if (ioctl(fd, FBIOGET_VSCREENINFO, &screeninfo) < 0) 
    {
		perror("FBIOGET_VSCREENINFO");
		return;
	}

	memcpy(&oldscreen, &screeninfo, sizeof(screeninfo));

	ioctl(fd, FBIOGET_VSCREENINFO, &screeninfo);
	
    printf("mode %d, %d, %d\n", screeninfo.xres, screeninfo.yres, screeninfo.bits_per_pixel);

	if (ioctl(fd, FBIOGET_FSCREENINFO, &fix)<0)
	{
		perror("FBIOGET_FSCREENINFO");
		printf("fb failed\n");
	}

    stride = fix.line_length;
	xRes   = screeninfo.xres;
	yRes   = screeninfo.yres;
	bpp    = screeninfo.bits_per_pixel;

    printf("stride = %d, width %d\n", stride, xRes);

	available = fix.smem_len;
    
	printf("%dk video mem\n", available/1024);
	
    lfb = (unsigned char*) mmap(0, available, PROT_WRITE|PROT_READ, MAP_SHARED, fd, 0);

	if (lfb == NULL) 
    {
		perror("mmap");
		return;
	}

    memset(lfb, 0, available);
}


int main(int argc,char* argv[]) {  
    SubtitleOutputDef_t out;    
    int showInfos = 0, noinput = 0;
    char file[255] = {""};
    int speed = 0, speedmap = 0;
    printf("%s >\n", __FILE__);

    if (argc < 2)
    {
        printf("give me a filename please\n");
        exit(1);
    }

    if (strstr(argv[1], "://") == NULL)
    {
        strcpy(file, "file://");
    }

    strcat(file, argv[1]);

    /* debug helper */
    if(argc == 3 && !strcmp(argv[2], "-d"))
    {
        showInfos = 1;
    }
    
    if(argc == 3 && !strcmp(argv[2], "-n"))
        noinput = 1;

    player = malloc(sizeof(Context_t));

    player->playback    = &PlaybackHandler;
    player->output        = &OutputHandler;
    player->container    = &ContainerHandler;
    player->manager        = &ManagerHandler;

    printf("%s\n", player->output->Name);

    //Registrating output devices
    player->output->Command(player,OUTPUT_ADD, "audio");
    player->output->Command(player,OUTPUT_ADD, "video");   
    player->output->Command(player,OUTPUT_ADD, "subtitle");   

    framebuffer_init();

    /* for testing ass subtitles */    
    out.screen_width = xRes;
    out.screen_height = yRes;
    out.framebufferFD = fd;
    out.destination   = lfb;
    out.destStride    = stride;
    out.shareFramebuffer = 1;
    
    player->output->subtitle->Command(player, (OutputCmd_t)OUTPUT_SET_SUBTITLE_OUTPUT, (void*) &out);    

    if(player->playback->Command(player, PLAYBACK_OPEN, file) < 0)
        return 10;

    {
        char ** TrackList = NULL;
        player->manager->audio->Command(player, MANAGER_LIST, &TrackList);
        if (TrackList != NULL) {
            printf("AudioTrack List\n");
            int i = 0;
            for (i = 0; TrackList[i] != NULL; i+=2) {
                printf("\t%s - %s\n", TrackList[i], TrackList[i+1]);
                free(TrackList[i]);
                free(TrackList[i+1]);
            }
            free(TrackList);
        }

        player->manager->video->Command(player, MANAGER_LIST, &TrackList);
        if (TrackList != NULL) {
            printf("VideoTrack List\n");
            int i = 0;
            for (i = 0; TrackList[i] != NULL; i+=2) {
                printf("\t%s - %s\n", TrackList[i], TrackList[i+1]);
                free(TrackList[i]);
                free(TrackList[i+1]);
            }
            free(TrackList);
        }
        player->manager->subtitle->Command(player, MANAGER_LIST, &TrackList);
        if (TrackList != NULL) {
            printf("SubtitleTrack List\n");
            int i = 0;
            for (i = 0; TrackList[i] != NULL; i+=2) {
                printf("\t%s - %s\n", TrackList[i], TrackList[i+1]);
                free(TrackList[i]);
                free(TrackList[i+1]);
            }
            free(TrackList);
        }       
    }
    {
        int AudioTrackId = -1;
        char * AudioTrackEncoding = NULL;
        char * AudioTrackName = NULL;
        player->manager->audio->Command(player, MANAGER_GET, &AudioTrackId);
        player->manager->audio->Command(player, MANAGER_GETENCODING, &AudioTrackEncoding);
        player->manager->audio->Command(player, MANAGER_GETNAME, &AudioTrackName);
        printf("Current Audio Track : %d %s %s\n", AudioTrackId, AudioTrackEncoding, AudioTrackName);
        free(AudioTrackEncoding);
        free(AudioTrackName);
        AudioTrackEncoding = NULL;
        AudioTrackName = NULL;

        player->manager->video->Command(player, MANAGER_GET, &AudioTrackId);
        player->manager->video->Command(player, MANAGER_GETENCODING, &AudioTrackEncoding);
        player->manager->video->Command(player, MANAGER_GETNAME, &AudioTrackName);
        printf("Current Video Track : %d %s %s\n", AudioTrackId, AudioTrackEncoding, AudioTrackName);
        free(AudioTrackEncoding);
        free(AudioTrackName);
        AudioTrackEncoding = NULL;
        AudioTrackName = NULL;
        player->manager->subtitle->Command(player, MANAGER_GET, &AudioTrackId);
        player->manager->subtitle->Command(player, MANAGER_GETENCODING, &AudioTrackEncoding);
        player->manager->subtitle->Command(player, MANAGER_GETNAME, &AudioTrackName);
        printf("Current Subtitle Track : %d %s %s\n", AudioTrackId, AudioTrackEncoding, AudioTrackName);
        free(AudioTrackEncoding);
        free(AudioTrackName);
        AudioTrackEncoding = NULL;
        AudioTrackName = NULL;
        /*      player->manager->audio->Command(player, MANAGER_SET, 2);
                player->manager->audio->Command(player, MANAGER_GET, &AudioTrackId);
                player->manager->audio->Command(player, MANAGER_GETNAME, &AudioTrackName);
                free(AudioTrackName);
                AudioTrackName = NULL;*/

    }
    {
        player->output->Command(player, OUTPUT_OPEN, NULL);

        if (showInfos == 1)
        {
            char *tags[] =
            {
                "Title",
                "Artist",
                "Album",
                "Year",
                "Genre",
                "Comment",
                "Track",
                "Copyright",
                "TestLibEplayer",
                NULL
            };
            int i = 0;
            while (tags[i] != NULL)
            {
                char* tag = tags[i];
                player->playback->Command(player, PLAYBACK_INFO, &tag);
#if !defined(VDR1722)
                if (tag != NULL)
                    printf("\t%s:\t%s\n",tags[i], tag);
                else
                    printf("\t%s:\tNULL\n",tags[i]);
#endif
                i++;
            }

            player->output->Command(player, OUTPUT_CLOSE, NULL);

            exit(1);
        } else
            player->playback->Command(player, PLAYBACK_PLAY, NULL);

        /*{
            int pid = 0;
            player->playback->Command(player, PLAYBACK_SWITCH_SUBTITLE, (void*)&pid);
        }*/

        while(player->playback->isPlaying) {
        		int Key = 0;

	    			if(kbhit())
	    				if(noinput == 0)
            		Key = getchar();
            
            if(!player->playback->isPlaying) {
                break;
            }
            
            if(Key == 0)
							continue;
            
            switch (Key) {
            case 'a': {
                int Key2 = getchar();
                switch (Key2) {
                case 'l': {
                    char ** TrackList = NULL;
                    player->manager->audio->Command(player, MANAGER_LIST, &TrackList);
                    if (TrackList != NULL) {
                        printf("AudioTrack List\n");
                        int i = 0;
                        for (i = 0; TrackList[i] != NULL; i+=2) {
                            printf("\t%s - %s\n", TrackList[i], TrackList[i+1]);
                            free(TrackList[i]);
                            free(TrackList[i+1]);
                        }
                        free(TrackList);
                    }
                    break;
                }
                case 'c': {
                    int AudioTrackId = -1;
                    char * AudioTrackEncoding = NULL;
                    char * AudioTrackName = NULL;
                    player->manager->audio->Command(player, MANAGER_GET, &AudioTrackId);
                    player->manager->audio->Command(player, MANAGER_GETENCODING, &AudioTrackEncoding);
                    player->manager->audio->Command(player, MANAGER_GETNAME, &AudioTrackName);
                    printf("Current Audio Track : %d %s %s\n", AudioTrackId, AudioTrackEncoding, AudioTrackName);
                    free(AudioTrackEncoding);
                    free(AudioTrackName);
                    AudioTrackEncoding = NULL;
                    AudioTrackName = NULL;

                    break;
                }
                default: {
                    Key2 -= 0x30;
                    if(Key2 >= 0 && Key2 <= 9) {
                        player->playback->Command(player, PLAYBACK_SWITCH_AUDIO, (void*)&Key2);
                    }

                }
                }
                break;
            }
            case 's': {
                int Key2 = getchar();
                switch (Key2) {
                case 'l': {
                    char ** TrackList = NULL;
                    player->manager->subtitle->Command(player, MANAGER_LIST, &TrackList);
                    if (TrackList != NULL) {
                        printf("SubtitleTrack List\n");
                        int i = 0;
                        for (i = 0; TrackList[i] != NULL; i+=2) {
                            printf("\t%s - %s\n", TrackList[i], TrackList[i+1]);
                            free(TrackList[i]);
                            free(TrackList[i+1]);
                        }
                        free(TrackList);
                    }
                    break;
                }
                case 'c': {
                    int SubtitleTrackId = -1;
                    char * SubtitleTrackEncoding = NULL;
                    char * SubtitleTrackName = NULL;
                    player->manager->subtitle->Command(player, MANAGER_GET, &SubtitleTrackId);
                    player->manager->subtitle->Command(player, MANAGER_GETENCODING, &SubtitleTrackEncoding);
                    player->manager->subtitle->Command(player, MANAGER_GETNAME, &SubtitleTrackName);
                    printf("Current Subtitle Track : %d %s %s\n", SubtitleTrackId, SubtitleTrackEncoding, SubtitleTrackName);
                    free(SubtitleTrackEncoding);
                    free(SubtitleTrackName);
                    SubtitleTrackEncoding = NULL;
                    SubtitleTrackName = NULL;

                    break;
                }
                default: {
                    Key2 -= 0x30;
                    if(Key2 >= 0 && Key2 <= 9) {
                        player->playback->Command(player, PLAYBACK_SWITCH_SUBTITLE, (void*)&Key2);
                    }

                }
                }
                break;		
            }

            case 'q':
                player->playback->Command(player, PLAYBACK_STOP, NULL);
                break;

            case 'c':
                player->playback->Command(player, PLAYBACK_CONTINUE, NULL);
                break;

            case 'p':
                player->playback->Command(player, PLAYBACK_PAUSE, NULL);
                break;

            case 'f': {
                 
                if (speed < 0)
                   speed = 0;
                    
                speed++;

                if (speed > 7)
                   speed = 1;

                switch(speed)
                {
                    case 1: speedmap = 1; break;
                    case 2: speedmap = 3; break;
                    case 3: speedmap = 7; break;
                    case 4: speedmap = 15; break;
                    case 5: speedmap = 31; break;
                    case 6: speedmap = 63; break;
                    case 7: speedmap = 127; break;
                }
                
                player->playback->Command(player, PLAYBACK_FASTFORWARD, &speedmap);
                break;
            }

            case 'b': {
                if (speed > 0)
                   speed = 0;
                   
                speed--;
                   
                if (speed < -7)
                    speed = -1;

                switch(speed)
                {
                    case -1: speedmap = -5; break;
                    case -2: speedmap = -10; break;
                    case -3: speedmap = -20; break;
                    case -4: speedmap = -40; break;
                    case -5: speedmap = -80; break;
                    case -6: speedmap = -160; break;
                    case -7: speedmap = -320; break;
                }

                player->playback->Command(player, PLAYBACK_FASTBACKWARD, &speedmap);
                break;
            }
#if defined(VDR1722)
            case 'g': {
				char gotoString [256];
				gets (gotoString);
                int gotoPos = atoi(gotoString);

				double length = 0;
				float sec;

				printf("gotoPos %i\n", gotoPos);
                if (player->container && player->container->selectedContainer)
                    player->container->selectedContainer->Command(player, CONTAINER_LENGTH, &length);

				if(gotoPos <= 0){
					printf("kleiner als erlaubt\n");
					sec = 0.0;
				}else if(gotoPos >= ((int)length - 10)){
					printf("laenger als erlaubt\n");
					sec = (int)length - 10;
				}else{
					printf("normal action\n");
					sec = gotoPos;
				}

				player->playback->Command(player, PLAYBACK_SEEK, (void*)&sec);	
                printf("goto postion (%i sec)\n", sec);
                break;
            }
#endif
            case 'k': {
#if !defined(VDR1722)
                int Key2 = getchar() - 48;
                float sec=0.0;
                printf("seconds %d \n", Key2);
                switch (Key2) {
                    case 1: sec=-15.0;break;
                    case 4: sec=-60.0;break;
                    case 7: sec=-300.0;break;
                    case 3: sec= 15.0;break;
                    case 6: sec= 60.0;break;
                    case 9: sec= 300.0;break;
                }
#else
		char seek [256];
		gets (seek);
                unsigned int seekTo = atoi(seek);
		double length = 0;
		float sec;
		
		unsigned long long int CurrentPTS = 0;
                player->playback->Command(player, PLAYBACK_PTS, &CurrentPTS);
                if (player->container && player->container->selectedContainer)
                    player->container->selectedContainer->Command(player, CONTAINER_LENGTH, &length);
				
		int CurrentSec = CurrentPTS / 90000;
		printf("CurrentSec = %i, seekTo = %i, abs(seekTo) = %i  seekTo + CurrentSec %i\n", CurrentSec, seekTo, abs(seekTo), (seekTo + CurrentSec));
		int ergSec = CurrentSec + seekTo;
		if(ergSec < 0){
			printf("kleiner als erlaubt\n");
			sec = 0.0;
		}else if((CurrentSec + seekTo) >= ((int)length - 10)){
			printf("laenger als erlaubt\n");
			sec = (int)length - 10;
		}else{
			printf("normal action\n");
			sec = seekTo + CurrentSec;
		}

		printf("springe %i \n", (int)sec);
#endif
                player->playback->Command(player, PLAYBACK_SEEK, (void*)&sec);
                break;
            }

            case 'l': {
                double length = 0;
                if (player->container && player->container->selectedContainer)
                    player->container->selectedContainer->Command(player, CONTAINER_LENGTH, &length);
                printf("Length = %02d:%02d:%02d (%.4f sec)\n", (int)((length/60)/60)%60, (int)(length/60)%60, (int)length%60, length);
                break;
            }
            case 'j': {
                unsigned long long int pts = 0;
                player->playback->Command(player, PLAYBACK_PTS, &pts);
                unsigned long long int sec = pts / 90000;
                printf("Pts = %02d:%02d:%02d (%llu.0000 sec)\n", (int)((sec/60)/60)%60, (int)(sec/60)%60, (int)sec%60, sec);
                break;
            }

            case 'i':
            {
                char *tags[] =
                {
                    "Title",
                    "Artist",
                    "Album",
                    "Year",
                    "Genre",
                    "Comment",
                    "Track",
                    "Copyright",
                    "TestLibEplayer",
                    NULL
                };
                int i = 0;
                while (tags[i] != NULL)
                {
                    char* tag = tags[i];
                    player->playback->Command(player, PLAYBACK_INFO, &tag);

                    if (tag != NULL)
                        printf("\t%s:\t%s\n",tags[i], tag);
                    else
                        printf("\t%s:\tNULL\n",tags[i]);
                    i++;
                }
                break;
            }
            default: {
                printf("Control:\n");
                printf("al:       List audio tracks\n");
                printf("ac:       List current audio track\n");
                printf("a[id]     Select audio track\n");
                printf("sl:       List subtitles\n");
                printf("sc:       List current subtitle\n");
                printf("s[id]     Select subtitles\n");
                printf("q:        Stop\n");
                printf("c:        Continue\n");
                printf("p:        Pause\n");
                printf("f:        Increase speed (Fast forward) (stepwise)\n");
                printf("b:        Decrease speed (Fast reverse) (stepwise)\n");
                printf("l:        Print duration\n");
                printf("j:        Print current PTS\n");
                printf("k[1,4,7]: Jump back [15,60,300] seconds\n");
                printf("k[3,6,9]: Jump forward [15,60,300] seconds\n");
                printf("i:        Print Info\n");
                break;
            }
            }
        }

        player->output->Command(player, OUTPUT_CLOSE, NULL);
    }

    //printOutputCapabilities();

    exit(0);
}
