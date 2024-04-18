/*
 *
 * $Id: playback_cs.cpp,v 1.0 2013/08/18 11:23:30 mohousch Exp $
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <fcntl.h>
#include <errno.h>

#include <sys/stat.h>

#include <pthread.h>
#include <syscall.h>

#include <unistd.h>

#include "playback_cs.h"

#include <driver/gfx/framebuffer.h>

#include <system/helpers.h>

#ifdef USE_OPENGL
#include "dmx_cs.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}
////
extern int buf_in;
extern int buf_out;
extern int buf_num;
#endif


//// global
#if defined ENABLE_GSTREAMER
#include <gst/gst.h>
#include <gst/tag/tag.h>
#include <gst/pbutils/missing-plugins.h>

#if defined (USE_OPENGL)
#if !GST_CHECK_VERSION(1,0,0)
#include <gst/interfaces/xoverlay.h>
#else
#include <gst/video/videooverlay.h>
#endif

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/gl.h>
#include <GL/glx.h>

extern int GLWinID;
extern int GLxStart;
extern int GLyStart;
extern int GLWidth;
extern int GLHeight;
uint32_t framerate;
#endif

typedef enum
{
	GST_PLAY_FLAG_VIDEO         = (1 << 0),
	GST_PLAY_FLAG_AUDIO         = (1 << 1),
	GST_PLAY_FLAG_TEXT          = (1 << 2),
	GST_PLAY_FLAG_VIS           = (1 << 3),
	GST_PLAY_FLAG_SOFT_VOLUME   = (1 << 4),
	GST_PLAY_FLAG_NATIVE_AUDIO  = (1 << 5),
	GST_PLAY_FLAG_NATIVE_VIDEO  = (1 << 6),
	GST_PLAY_FLAG_DOWNLOAD      = (1 << 7),
	GST_PLAY_FLAG_BUFFERING     = (1 << 8),
	GST_PLAY_FLAG_DEINTERLACE   = (1 << 9),
	GST_PLAY_FLAG_SOFT_COLORBALANCE = (1 << 10),
	GST_PLAY_FLAG_FORCE_FILTERS = (1 << 11),
} GstPlayFlags;

GstElement * m_gst_playbin = NULL;
GstElement * audioSink = NULL;
GstElement * videoSink = NULL;
GstElement *subsink = NULL;
////
gulong m_subs_to_pull_handler_id = 0;
GstMessage* messagePointer = NULL;
GstPad* messagePad = NULL;
GstBuffer* messageBuffer = NULL;
int messageType = -1;
//
gchar * uri = NULL;
GstBus * bus = NULL;
bool end_eof = false;
#define HTTP_TIMEOUT 30
#else
#include <common.h>
#include <linux/fb.h>

extern OutputHandler_t		OutputHandler;
extern PlaybackHandler_t	PlaybackHandler;
extern ContainerHandler_t	ContainerHandler;
extern ManagerHandler_t		ManagerHandler;

static Context_t * player = NULL;
#endif

#if defined ENABLE_GSTREAMER
#if GST_VERSION_MAJOR < 1
gint match_sinktype(GstElement *element, gpointer type)
{
	return strcmp(g_type_name(G_OBJECT_TYPE(element)), (const char*)type);
}
#else
gint match_sinktype(const GValue *velement, const gchar *type)
{
	GstElement *element = GST_ELEMENT_CAST(g_value_get_object(velement));
	return strcmp(g_type_name(G_OBJECT_TYPE(element)), type);
}
#endif

//
void playbinNotifySource(GObject *object, GParamSpec *unused, gpointer /*user_data*/)
{
	GstElement *source = NULL;
	g_object_get(object, "source", &source, NULL);

	if (source)
	{
		if (g_object_class_find_property(G_OBJECT_GET_CLASS(source), "timeout") != 0)
		{
			GstElementFactory *factory = gst_element_get_factory(source);
			if (factory)
			{
				const gchar *sourcename = gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory));
				if (!strcmp(sourcename, "souphttpsrc"))
				{
					g_object_set(G_OBJECT(source), "timeout", HTTP_TIMEOUT, NULL);
				}
			}
		}

		if (g_object_class_find_property(G_OBJECT_GET_CLASS(source), "ssl-strict") != 0)
		{
			g_object_set(G_OBJECT(source), "ssl-strict", FALSE, NULL);
		}
		
		gst_object_unref(source);
	}
}

GstBusSyncReply Gst_bus_call(GstBus *bus, GstMessage * msg, gpointer)
{
	//source name
	gchar * sourceName;
	
	// source
	GstObject * source;
	source = GST_MESSAGE_SRC(msg);
	
	if (!GST_IS_OBJECT(source))
		return GST_BUS_DROP;
	
	sourceName = gst_object_get_name(source);

	switch (GST_MESSAGE_TYPE(msg)) 
	{
		case GST_MESSAGE_EOS: 
		{
			g_message("End-of-stream");
			
			//
			printf("cPlayback::%s (EOS) !!!!EOF!!!! << -1\n", __func__);
			end_eof = true;
			//
			break;
		}
		
		case GST_MESSAGE_ERROR: 
		{
			gchar * debug1;
			GError *err;
			gst_message_parse_error(msg, &err, &debug1);
			g_free(debug1);
			
			printf("cPlayback:: Gstreamer error: %s (%i)\n", err->message, err->code );
			
			if ( err->domain == GST_STREAM_ERROR )
			{
				if ( err->code == GST_STREAM_ERROR_CODEC_NOT_FOUND )
				{
					if ( g_strrstr(sourceName, "videosink") )
						printf("Gst_bus_call: - videoSink\n");
					else if ( g_strrstr(sourceName, "audiosink") )
						printf("Gst_bus_call: - audioSink\n");
				}
			}
			else if ( err->domain == GST_RESOURCE_ERROR )
			{
				if ( err->code == GST_RESOURCE_ERROR_OPEN_READ || err->code == GST_RESOURCE_ERROR_READ )
				{
					end_eof = true;
				}
			}
			g_error_free(err);
			
			end_eof = true;
			
			break;
		}

		case GST_MESSAGE_INFO:
		{
			gchar * _debug;
			GError * inf;
	
			gst_message_parse_info (msg, &inf, &_debug);
			g_free(_debug);
			
			if ( inf->domain == GST_STREAM_ERROR && inf->code == GST_STREAM_ERROR_DECODE )
			{
				if ( g_strrstr(sourceName, "videosink") )
					printf("Gst_bus_call: videoSink\n");
			}
			g_error_free(inf);
			break;
		}

		//
		case GST_MESSAGE_TAG:
		{
			GstTagList *tags, *result;
			gst_message_parse_tag(msg, &tags);
			GstTagList *m_stream_tags = 0;

			result = gst_tag_list_merge(m_stream_tags, tags, GST_TAG_MERGE_REPLACE);
			
			// tags
			if (result)
			{
				if (m_stream_tags && gst_tag_list_is_equal(m_stream_tags, result))
				{
					gst_tag_list_free(tags);
					gst_tag_list_free(result);
					break;
				}
				
				if (m_stream_tags)
					gst_tag_list_free(m_stream_tags);
				m_stream_tags = result;
			}
			
#ifdef USE_OPENGL
			//
			guint value;
			
			if (gst_tag_list_get_uint(m_stream_tags, GST_TAG_BITRATE, &value))
			{
				framerate = (uint32_t) value;
			}
#endif
			
			gst_tag_list_free(tags);
			break;
		}
		//
#if GST_VERSION_MAJOR >= 1
		case GST_MESSAGE_WARNING:
		{
			gdouble m_currentTrickRatio = 1.0;
			gint64 m_last_seek_pos = 0; 
			gchar *debug_warn = NULL;
			GError *warn = NULL;
			gst_message_parse_warning (msg, &warn, &debug_warn);

			if(!strncmp(warn->message , "Internal data flow problem", 26) && !strncmp(sourceName, "subtitle_sink", 13))
			{
				printf("Gstreamer warning : %s (%i) from %s\n" , warn->message, warn->code, sourceName);

				if(subsink)
				{
					if (!gst_element_seek (subsink, m_currentTrickRatio, GST_FORMAT_TIME, (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT),
						GST_SEEK_TYPE_SET, m_last_seek_pos,
						GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE))
					{
						printf("seekToImpl subsink failed\n");
					}
					gst_object_unref(subsink);
				}
			}
			g_free(debug_warn);
			g_error_free(warn);
			break;
		}
#endif	
		case GST_MESSAGE_STATE_CHANGED:
		{
			if(GST_MESSAGE_SRC(msg) != GST_OBJECT(m_gst_playbin))
				break;

			GstState old_state, new_state;
			gst_message_parse_state_changed(msg, &old_state, &new_state, NULL);
			
			if(old_state == new_state)
				break;
		
			printf("cPlayback::state transition %s -> %s\n", gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));
		
			GstStateChange transition = (GstStateChange)GST_STATE_TRANSITION(old_state, new_state);
		
			switch(transition)
			{
				case GST_STATE_CHANGE_NULL_TO_READY:
				{
				}
				break;
				
				case GST_STATE_CHANGE_READY_TO_PAUSED:
				{
#if GST_VERSION_MAJOR >= 1
					GValue result = G_VALUE_INIT;
#endif
					GstIterator * children;

					//
					if (subsink)
					{
#ifdef GSTREAMER_SUBTITLE_SYNC_MODE_BUG
						g_object_set(G_OBJECT (subsink), "sync", FALSE, NULL);
#endif
						gst_object_unref(subsink);
					}
					
					if (audioSink)
					{
						gst_object_unref(GST_OBJECT(audioSink));
						audioSink = NULL;
						printf("Gst_bus_call: audio sink closed\n");
					}
					
					if (videoSink)
					{
						gst_object_unref(GST_OBJECT(videoSink));
						videoSink = NULL;
						printf("Gst_bus_call: video sink closed\n");
					}
					
					// set audio video sink
					children = gst_bin_iterate_recurse(GST_BIN(m_gst_playbin));
					
#if GST_VERSION_MAJOR < 1					
					audioSink = GST_ELEMENT_CAST(gst_iterator_find_custom(children, (GCompareFunc)match_sinktype, (gpointer)"GstDVBAudioSink"));
#else
					if (gst_iterator_find_custom(children, (GCompareFunc)match_sinktype, &result, (gpointer)"GstDVBAudioSink"))
					{
						audioSink = GST_ELEMENT_CAST(g_value_dup_object(&result));
						g_value_unset(&result);
					}
#endif	
					
					if(audioSink)
						printf("Gst_bus_call: audio sink created\n");
					
#if GST_VERSION_MAJOR < 1
					videoSink = GST_ELEMENT_CAST(gst_iterator_find_custom(children, (GCompareFunc)match_sinktype, (gpointer)"GstDVBVideoSink"));
#else
					if (gst_iterator_find_custom(children, (GCompareFunc)match_sinktype, &result, (gpointer)"GstDVBVideoSink"))
					{
						videoSink = GST_ELEMENT_CAST(g_value_dup_object(&result));
						g_value_unset(&result);
					}
#endif

					if(videoSink)
						printf("Gst_bus_call: video sink created\n");
						
					// subsin
					if(subsink)
					{
						gst_object_ref_sink(subsink);
						printf("subsink created ***\n");
					}
					
					gst_iterator_free(children);
				}
				break;
				
				case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
				{
				}
				break;
				
				case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
				{
				}	
				break;
				
				case GST_STATE_CHANGE_PAUSED_TO_READY:
				{
					if (audioSink)
					{
						gst_object_unref(GST_OBJECT(audioSink));
						audioSink = NULL;
						printf("Gst_bus_call: audio sink closed\n");
					}
					if (videoSink)
					{
						gst_object_unref(GST_OBJECT(videoSink));
						videoSink = NULL;
						printf("Gst_bus_call: video sink closed\n");
					}
				}	
				break;
				
				case GST_STATE_CHANGE_READY_TO_NULL:
				{
				}	
				break;
			}
			break;
		}
#if defined (USE_OPENGL) //FIXME: ???	
		case GST_MESSAGE_ELEMENT:
		{
#ifdef USE_OVERLAY
#if GST_VERSION_MAJOR < 1
			if( gst_structure_has_name(gst_message_get_structure(msg), "prepare-xwindow-id") || gst_structure_has_name(gst_message_get_structure(msg), "have-xwindow-id") )
#else
			if (gst_is_video_overlay_prepare_window_handle_message(msg))
#endif 
			{
#if GST_VERSION_MAJOR < 1
				gst_x_overlay_set_xwindow_id(GST_X_OVERLAY(GST_MESSAGE_SRC(msg)), GLWinID);
#else
				//
				gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(GST_MESSAGE_SRC(msg)), (guintptr)GLWinID);
				
				gst_video_overlay_expose(GST_VIDEO_OVERLAY(GST_MESSAGE_SRC(msg)));
				
				// reshape window
				gst_video_overlay_set_render_rectangle(GST_VIDEO_OVERLAY(GST_MESSAGE_SRC(msg)), 0, 0, GLWidth, GLHeight);
#endif
			}
#endif
		}
		break;
#endif	

		case GST_MESSAGE_BUFFERING:
		case GST_MESSAGE_TOC:
		case GST_MESSAGE_ASYNC_DONE:	
		default:
			break;
	}
	
	g_free(sourceName);
	

	return GST_BUS_DROP;
}

////
void gstCBsubtitleAvail(GstElement *subsink, GstBuffer *buffer, gpointer)
{
	Hexdump((uint8_t*)buffer, sizeof(buffer));
	
//	if (_this->mSubStream < 0)
	{
		if (buffer) 
			gst_buffer_unref(buffer);
		return;
	}
}
#endif

////
cPlayback::cPlayback(int)
{
}

bool cPlayback::Open()
{
	printf("cPlayback::Open\n");
	
	mAudioStream = 0;
	mSubStream = -1;
	mExtSubStream = -1;
	mSpeed = 0;
	playing = false;
	
#if defined (ENABLE_GSTREAMER)
	// create gst pipeline
#if GST_VERSION_MAJOR < 1
	m_gst_playbin = gst_element_factory_make("playbin2", "playbin");
#else
	m_gst_playbin = gst_element_factory_make("playbin", "playbin");
#endif
#else
	player = (Context_t*)malloc(sizeof(Context_t));

	//init player
	if(player) 
	{
		player->playback	= &PlaybackHandler;
		player->output		= &OutputHandler;
		player->container	= &ContainerHandler;
		player->manager		= &ManagerHandler;
	}
	
	SubtitleOutputDef_t out;

	out.screen_x = CFrameBuffer::getInstance()->getScreenX(true);
	out.screen_y = CFrameBuffer::getInstance()->getScreenY(true);
	out.screen_width = CFrameBuffer::getInstance()->getScreenWidth(true);
	out.screen_height = CFrameBuffer::getInstance()->getScreenHeight(true);
	
	out.framebufferFD = CFrameBuffer::getInstance()->getFileHandle();
	out.destination   = CFrameBuffer::getInstance()->getFrameBufferPointer();
	out.destStride    = CFrameBuffer::getInstance()->getStride();

	if(player && player->playback)
		player->playback->Command(player, PLAYBACK_INIT, (void*)&out);
#endif

	return true;
}

void cPlayback::Close(void)
{  
	printf("cPlayback::Close\n");
	
	Stop();
	
#if ENABLE_GSTREAMER
	end_eof = false;

	// disconnect subtitle callback
	if (subsink)
	{
		g_signal_handler_disconnect (subsink, m_subs_to_pull_handler_id);
		gst_object_unref(GST_OBJECT(subsink));
	}
	
	// disconnect bus handler
	if (m_gst_playbin)
	{
		// disconnect sync handler callback
#if GST_VERSION_MAJOR < 1
		gst_bus_set_sync_handler(bus, gst_bus_sync_signal_handler, NULL);
#else
		gst_bus_set_sync_handler(bus, gst_bus_sync_signal_handler, NULL, NULL);
#endif
		gst_object_unref(bus);
		
		printf("GST bus handler closed\n");
	}
	
	// sometimes video/audio event poll close only needed device, so be sure and decrease them 
	if (audioSink)
	{
		gst_object_unref(GST_OBJECT(audioSink));
		audioSink = NULL;
		printf("cPlayback::Close: audio sink closed\n");
	}
	
	if (videoSink)
	{
		gst_object_unref(GST_OBJECT(videoSink));
		videoSink = NULL;
		printf("cPlayback::Close: audio sink closed\n");
	}

	// close gst
	if (m_gst_playbin)
	{
		// unref m_gst_playbin
		gst_object_unref (GST_OBJECT (m_gst_playbin));
		m_gst_playbin = NULL;
		
		printf("GST playbin closed\n");
	}
#else
	//
	if(player && player->playback)
		player->playback->Command(player, PLAYBACK_CLOSE, NULL);

	if(player)
		free(player);

	if(player != NULL)
		player = NULL;
#endif	
}

// start
bool cPlayback::Start(char *filename)
{
	printf("cPlayback::Start: filename=%s\n", filename);
	
	// check for suburi
	if (filename == NULL)
	{
		playing = false;
		return false;
	}
	
	//
	std::string file("");
	bool isHTTP = false;

	if(!strncmp("http://", filename, 7))
	{
		isHTTP = true;
	}
	else if(!strncmp("https://", filename, 8))
	{
		isHTTP = true;
	}
	else if(!strncmp("upnp://", filename, 7))
	{
		isHTTP = true;
	}
	else if(!strncmp("rtmp://", filename, 7))
	{
		isHTTP = true;
	}
	else if(!strncmp("rtsp://", filename, 7))
	{
		isHTTP = true;
	}
	else if(!strncmp("mms://", filename, 6))
	{
		isHTTP = true;
	}
	else if(!strncmp("file://", filename, 7))
	{
		isHTTP = false;
	}
	else
		file = "file://";
	
	file.append(filename);

#if defined (ENABLE_GSTREAMER)
	end_eof = false;
	
	int m_buffer_size = 5*1024*1024;
	int flags = GST_PLAY_FLAG_AUDIO | GST_PLAY_FLAG_VIDEO | GST_PLAY_FLAG_TEXT | GST_PLAY_FLAG_NATIVE_VIDEO;
	
	if (isHTTP)
		uri = g_strdup_printf("%s", filename);
	else
		uri = g_filename_to_uri(filename, NULL, NULL);

	if(m_gst_playbin)
	{
		// increase the default 2 second / 2 MB buffer limitations to 5s / 5MB
		if(isHTTP)
		{
			g_signal_connect(G_OBJECT (m_gst_playbin), "notify::source", G_CALLBACK (playbinNotifySource), this);

			// set buffer size
			g_object_set(G_OBJECT(m_gst_playbin), "buffer-size", m_buffer_size, NULL);
			g_object_set(G_OBJECT(m_gst_playbin), "buffer-duration", 5LL * GST_SECOND, NULL);
			flags |= GST_PLAY_FLAG_BUFFERING;
		}
		
		// set flags
		g_object_set(G_OBJECT(m_gst_playbin), "flags", flags, NULL);

		// set uri
		g_object_set(G_OBJECT(m_gst_playbin), "uri", uri, NULL);

		// subsink
		subsink = gst_element_factory_make("subsink", "subtitle_sink");
		if (subsink)
		{
			m_subs_to_pull_handler_id = g_signal_connect (subsink, "new-buffer", G_CALLBACK (gstCBsubtitleAvail), NULL);
#if GST_VERSION_MAJOR < 1
			g_object_set (G_OBJECT (subsink), "caps", gst_caps_from_string("text/plain; text/x-plain; text/x-raw; text/x-pango-markup; video/x-dvd-subpicture; subpicture/x-pgs"), NULL);
#else
			g_object_set (G_OBJECT (subsink), "caps", gst_caps_from_string("text/plain; text/x-plain; text/x-raw; text/x-pango-markup; subpicture/x-dvd; subpicture/x-pgs"), NULL);
#endif
			g_object_set (G_OBJECT (m_gst_playbin), "text-sink", subsink, NULL);
			g_object_set (G_OBJECT (m_gst_playbin), "current-text", mSubStream, NULL);
		}	
		
		//gst bus handler
		bus = gst_pipeline_get_bus(GST_PIPELINE (m_gst_playbin));

#if GST_VERSION_MAJOR < 1
		gst_bus_set_sync_handler(bus, Gst_bus_call, NULL);
#else
		gst_bus_set_sync_handler(bus, (GstBusSyncHandler)Gst_bus_call, NULL, NULL);
#endif
		gst_object_unref(bus);
		
		// start playing
		gst_element_set_state(GST_ELEMENT(m_gst_playbin), GST_STATE_PLAYING);
		playing = true;
		mSpeed = 1;
	}
	else
	{
		gst_object_unref(GST_OBJECT(m_gst_playbin));

		m_gst_playbin = 0;
		
		printf("cPlayback::Start: failed to create GStreamer pipeline!, sorry we can not play\n");
		playing = false;
	}
	
	if(uri != NULL)
		g_free(uri);
#else	
	//open file
	if(player && player->playback && player->playback->Command(player, PLAYBACK_OPEN, (char *)file.c_str()) >= 0) 
	{
		// open suburi
		std::string suburi = filename;
		changeFileNameExt(suburi, ".srt");
		
		if ( !file_exists(suburi.c_str()) )
			changeFileNameExt(suburi, ".ass");
		else if ( !file_exists(suburi.c_str()) )
			changeFileNameExt(suburi, ".ssa");
			
		if ( file_exists(suburi.c_str()) )
		{
			printf("cPlayback::Open suburi:%s\n", suburi.c_str());
			player->playback->Command(player, PLAYBACK_OPEN_SUB, (char*)suburi.c_str());
		}
		
		// start playing
		if(player && player->output && player->playback) 
		{	
			if (player->playback->Command(player, PLAYBACK_PLAY, NULL) == 0 ) 
			{
				//
				if ( file_exists(suburi.c_str()) )
				{
					player->playback->Command(player, PLAYBACK_PLAY_SUB, NULL);
				}
				
				playing = true;
				mSpeed = 1;
			}
			else
			{
				printf("cPlayback::Start: failed to start playing file, sorry we can not play\n");
				playing = false;
			}
		}		
	}
	else
	{
		printf("cPlayback::Start: failed to start playing file, sorry we can not play\n");
		playing = false;
	}
#endif

	printf("cPlayback::Start: (playing %d)\n", playing);	

	return playing;
}

bool cPlayback::Play(void)
{
	printf("cPlayback::Play: (playing %d)\n", playing);	

	if(playing == true) 
		return true;
	
#if defined (ENABLE_GSTREAMER)
	if(m_gst_playbin)
	{
		gst_element_set_state(GST_ELEMENT(m_gst_playbin), GST_STATE_PLAYING);
		
		playing = true;
	}
#else
	if(player && player->output && player->playback) 
	{		
		if (player->playback->Command(player, PLAYBACK_PLAY, NULL) == 0 ) // playback.c uses "int = 0" for "true"
		{
			playing = true;
		}
	}
#endif

	printf("cPlayback::Play: (playing %d)\n", playing);

	return playing;
}

bool cPlayback::Stop(void)
{ 
	if(playing == false) 
		return false;
	
	printf("cPlayback::Stop: (playing %d)\n", playing);

#if defined (ENABLE_GSTREAMER)
	// stop
	if(m_gst_playbin)
	{
		gst_element_set_state(m_gst_playbin, GST_STATE_NULL);
	}
#else
	if(player && player->playback && player->output) 
		player->playback->Command(player, PLAYBACK_STOP, NULL);
#endif

	playing = false;
	
	printf("cPlayback::Stop: (playing %d)\n", playing);

	return true;
}

bool cPlayback::SetAPid(unsigned short pid)
{
	printf("cPlayback::SetAPid: curpid:%d nextpid:%d\n", mAudioStream, pid);
	
#if ENABLE_GSTREAMER
	if(pid != mAudioStream)
	{
		g_object_set (G_OBJECT (m_gst_playbin), "current-audio", pid, NULL);
		printf("cPlayback::SetAPid: switched to audio stream %i\n", pid);
		mAudioStream = pid;
	}
#else
	int track = pid;

	if(pid != mAudioStream)
	{
		if(player && player->playback)
			player->playback->Command(player, PLAYBACK_SWITCH_AUDIO, (void*)&track);

		mAudioStream = pid;
	}
#endif	

	return true;
}

//
bool cPlayback::SetSubPid(short pid)
{
	printf("cPlayback::SetSubPid: curpid:%d nextpid:%d\n", mSubStream, pid);
	
#if ENABLE_GSTREAMER
	if(pid != mSubStream)
	{
		g_object_set (G_OBJECT (m_gst_playbin), "current-text", pid, NULL);
		printf("cPlayback::SetSubPid: switched to subtitle stream %i\n", pid);
		mSubStream = pid;
	}
#else
	int track = pid;

	if(pid != mSubStream)
	{
		if(player && player->playback)
			player->playback->Command(player, PLAYBACK_SWITCH_SUBTITLE, (void*)&track);

		mSubStream = pid;
		mExtSubStream = -1;
	}
#endif	

	return true;
}

bool cPlayback::SetExtSubPid(short pid)
{
	printf("cPlayback::SetExtSubPid: curpid:%d nextpid:%d\n", mSubStream, pid);
	
#ifndef ENABLE_GSTREAMER
	int track = pid;

	if(pid != mExtSubStream)
	{
		if(player && player->playback)
			player->playback->Command(player, PLAYBACK_SWITCH_EXTSUBTITLE, (void*)&track);

		mExtSubStream = pid;
		mSubStream = -1;
	}
#endif	

	return true;
}

#if ENABLE_GSTREAMER
void cPlayback::trickSeek(double ratio)
{
/*	
	bool validposition = false;
	gint64 pos = 0;
	int position;
	int duration;
	
	// pause
	if (ratio > -0.01 && ratio < 0.01)
	{
		gst_element_set_state(m_gst_playbin, GST_STATE_PAUSED);
		return;
	}
	
	if( GetPosition(position, duration) )
	{
		validposition = true;
		pos = position*1000000; //ns
	}

	gst_element_set_state(m_gst_playbin, GST_STATE_PLAYING);
			
	if (validposition)
	{
		if(ratio >= 0.0)
			gst_element_seek(m_gst_playbin, ratio, GST_FORMAT_TIME, (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SKIP), GST_SEEK_TYPE_SET, pos, GST_SEEK_TYPE_SET, -1);
		else
			gst_element_seek(m_gst_playbin, ratio, GST_FORMAT_TIME, (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SKIP), GST_SEEK_TYPE_SET, 0, GST_SEEK_TYPE_SET, pos);
	}
*/
	GstFormat fmt = GST_FORMAT_TIME;
	gint64 pos = 0;

	gst_element_set_state(m_gst_playbin, GST_STATE_PLAYING);

	if (gst_element_query_position(m_gst_playbin, fmt, &pos))
	{
		if (ratio >= 0.0)
			gst_element_seek(m_gst_playbin, ratio, fmt, (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SKIP), GST_SEEK_TYPE_SET, pos, GST_SEEK_TYPE_SET, -1);
		else
			gst_element_seek(m_gst_playbin, ratio, fmt, (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SKIP), GST_SEEK_TYPE_SET, 0, GST_SEEK_TYPE_SET, pos);
	}
}
#endif

bool cPlayback::SetSpeed(int speed)
{  
	printf("cPlayback::SetSpeed: speed %d\n", speed);	

	if(playing == false) 
		return false;

#if defined (ENABLE_GSTREAMER)
	if(m_gst_playbin)
	{
		// pause
		if (speed == 0)
		{
			gst_element_set_state(m_gst_playbin, GST_STATE_PAUSED);
		}
		// play/continue
		else if (speed == 1)
		{
			trickSeek(1);
		}
		//ff
		else if (speed > 1)
		{
			trickSeek(speed);
		}
		//rf
		else if (speed < 0)
		{
			trickSeek(speed);
		}
	}
#else
	int speedmap = 0;
	
	if(player && player->playback) 
	{
		if(speed > 1) 		//forwarding
		{
			//
			if (speed > 7) 
				speed = 7;
			
			switch(speed)
			{
				case 2: speedmap = 3; break;
				case 3: speedmap = 7; break;
				case 4: speedmap = 15; break;
				case 5: speedmap = 31; break;
				case 6: speedmap = 63; break;
				case 7: speedmap = 127; break;
			}

			player->playback->Command(player, PLAYBACK_FASTFORWARD, (void*)&speedmap);
		}
		else if(speed == 0)	//pausing
		{
			player->playback->Command(player, PLAYBACK_PAUSE, NULL);
		}
		else if (speed < 0)	//backwarding
		{
			//
			if (speed > -1) 
				speed = -1;
			
			if (speed < -7) 
				speed = -7;
			
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
			
			player->playback->Command(player, PLAYBACK_FASTBACKWARD, (void*)&speedmap);

			// trickseek
			//player->playback->Command(player, PLAYBACK_SEEK, (void*)&speedmap);
		}
		else if(speed == 1) 	//continue
		{
			player->playback->Command(player, PLAYBACK_CONTINUE, NULL);
		}
	}
#endif

	mSpeed = speed;

	return true;
}

bool cPlayback::SetSlow(int slow)
{  
	printf("cPlayback::SetSlow: (playing %d)\n", playing);	

	if(playing == false) 
		return false;

#if ENABLE_GSTREAMER
	if(m_gst_playbin)
	{
		trickSeek(0.5);
	}
#else
	if(player && player->playback) 
	{
		player->playback->Command(player, PLAYBACK_SLOWMOTION, (void*)&slow);
	}
#endif

	mSpeed = slow;

	return true;
}

void cPlayback::GetPts(uint64_t &pts)
{
#ifndef ENABLE_GSTREAMER
	if (player && player->playback)
		player->playback->Command(player, PLAYBACK_PTS, (void *)&pts);
#endif
}

bool cPlayback::GetSpeed(int &speed) const
{
	speed = mSpeed;

	return true;
}

// in milliseconds
bool cPlayback::GetPosition(int &position, int &duration)
{
	if(playing == false) 
		return false;

#if ENABLE_GSTREAMER
	if(end_eof)
	{
		printf("cPlayback::%s !!!!EOF!!!! < -1\n", __FUNCTION__);
		return false;
	}
	
	if(m_gst_playbin)
	{
		GstFormat fmt = GST_FORMAT_TIME; //Returns time in nanosecs
		
		// position
		gint64 pts;
		position = 0;
		
#if defined (USE_OPENGL)
#if GST_VERSION_MAJOR < 1
		gst_element_query_position(m_gst_playbin, &fmt, &pts);
#else
		gst_element_query_position(m_gst_playbin, fmt, &pts);
#endif
#else
		
		if(audioSink || videoSink)
		{
			g_signal_emit_by_name(audioSink ? audioSink : videoSink, "get-decoder-time", &pts);
			GST_CLOCK_TIME_IS_VALID(pts);
		}
		else
		{
#if GST_VERSION_MAJOR < 1
			gst_element_query_position(m_gst_playbin, &fmt, &pts);
#else
			gst_element_query_position(m_gst_playbin, fmt, &pts);
#endif
		}
#endif		
			
		position = pts / 1000000.0;	// in ms
		
//		printf("%s: position: %d msec ", __FUNCTION__, position);
		
		//duration
		gint64 len;

#if GST_VERSION_MAJOR < 1
		gst_element_query_duration(m_gst_playbin, &fmt, &len);
#else
		gst_element_query_duration(m_gst_playbin, fmt, &len);
#endif
		
		duration = len / 1000000.0;	// in ms

//		printf("(duration: %d msec)\n", duration);
	}
#else
	if (player && player->playback && !player->playback->isPlaying) 
	{	  
		printf("cPlayback::%s !!!!EOF!!!! < -1\n", __func__);
		
		playing = false;
	
		return false;
	} 

	// position
	unsigned long long int vpts = 0;

	if(player && player->playback)
		player->playback->Command(player, PLAYBACK_PTS, &vpts);

	position = vpts/90;
	
//	printf("%s: position: %d ms ", __FUNCTION__, position);
	
	// duration
	double length = 0;

	if(player && player->playback)
		player->playback->Command(player, PLAYBACK_LENGTH, &length);
	
	if(length < 0) 
		length = 0;

	duration = (int)(length*1000);
	
//	printf("(duration: %d ms)\n", duration);
#endif
	
	return true;
}

bool cPlayback::SetPosition(int position)
{
	if(playing == false) 
		return false;
	
	printf("cPlayback::SetPosition: position: %d msec\n", position);
	
#if ENABLE_GSTREAMER
	gint64 time_nanoseconds = (gint64)position * 1000000.0;
	
	//
	gint64 pos;
	GstFormat fmt = GST_FORMAT_TIME;
	
	gst_element_query_position(m_gst_playbin, fmt, &pos);
	time_nanoseconds = pos + ((gint64)position * 1000000.0);

	if(time_nanoseconds < 0) 
		time_nanoseconds = 0;
		
	if(m_gst_playbin)
	{
		gst_element_seek(m_gst_playbin, 1.0, GST_FORMAT_TIME, (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE), GST_SEEK_TYPE_SET, time_nanoseconds, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
	}
#else
	float pos = (position/1000); // in sec

	if(player && player->playback)
		player->playback->Command(player, PLAYBACK_SEEK, (void*)&pos);
#endif

	return true;
}

void cPlayback::FindAllPids(uint16_t *apids, bool *ac3flags, uint16_t *numpida, std::string *language)
{ 
	printf("cPlayback::FindAllPids\n");
	
	*ac3flags = false;

#if defined (ENABLE_GSTREAMER)
	if(m_gst_playbin)
	{
		gint i, n_audio = 0;
		
		// get audio
		g_object_get (m_gst_playbin, "n-audio", &n_audio, NULL);
		
		if(n_audio == 0)
			return;

		language->clear();
		
		for (i = 0; i < n_audio; i++)
		{
			// apids
			apids[i] = i;
			
			GstPad * pad = 0;
			g_signal_emit_by_name (m_gst_playbin, "get-audio-pad", i, &pad);

#if GST_VERSION_MAJOR < 1
			GstCaps* caps = gst_pad_get_negotiated_caps(pad);
#else
			GstCaps* caps = gst_pad_get_current_caps(pad);
#endif
			gst_object_unref(pad);

			if (!caps)
				continue;

			// codec|language
			gchar * g_lang = NULL;
			gchar* g_codec = NULL;
			GstTagList *tags = NULL;


			g_signal_emit_by_name (m_gst_playbin, "get-audio-tags", i, &tags);
#if GST_VERSION_MAJOR < 1
			if (tags && gst_is_tag_list(tags))
#else
			if (tags && GST_IS_TAG_LIST(tags))
#endif
			{
				// language
				language[i] = "Stream";
				if (gst_tag_list_get_string(tags, GST_TAG_LANGUAGE_CODE, &g_lang))
				{
					language[i] = std::string(g_lang).c_str();
					g_free(g_lang);
				}

				// codec
				if (gst_tag_list_get_string(tags, GST_TAG_AUDIO_CODEC, &g_codec))
				{
					//
					if(std::string(g_codec) == "Dolby Digital (AC-3)" || std::string(g_codec) == "AC-3 (ATSC A/52)")
						ac3flags[i] = true;
					
					//
					language[i] += " (";
					language[i] += std::string(g_codec).c_str();
					language[i] += ")";
					
					g_free(g_codec);
				}

				gst_tag_list_free(tags);
			}
			
			gst_caps_unref(caps);
			
			printf("\t%s\n", std::string(language[i]).c_str());
		}
		
		// numpids
		*numpida = i;
	}
#else
	char ** TrackList = NULL;
	
	// audio pids
	if(player && player->manager && player->manager->audio) 
	{
		player->manager->audio->Command(player, MANAGER_LIST, (void*)&TrackList);

		if (TrackList != NULL) 
		{
			int i = 0,j = 0;

			for (i = 0, j = 0; TrackList[i] != NULL; i += 2, j++) 
			{
				printf("\t%s - %s\n", TrackList[i], TrackList[i + 1]);
				
				apids[j] = j;
				
				//
				if(!strncmp("A_AC3", TrackList[i + 1], 5))
					ac3flags[j] = true;
				
				//
				language[j] = "Stream";

				language[j] = TrackList[i];
				
				language[j] += " (";
				language[j] += TrackList[i + 1];
				language[j] += ")";
				
				free(TrackList[i]);
				free(TrackList[i+1]);
			}
			free(TrackList);
			*numpida = j;
		}
	}
#endif	
}

// subs pids
void cPlayback::FindAllSubPids(uint16_t *apids, uint16_t *numpida, std::string *language)
{
	printf("cPlayback::FindAllSubPids:\n");

#if defined (ENABLE_GSTREAMER)
	if(m_gst_playbin)
	{
		gint i, n_text = 0;
		
		// get audio
		g_object_get (m_gst_playbin, "n-text", &n_text, NULL);
		
		if(n_text == 0)
			return;

		language->clear();
		
		for (i = 0; i < n_text; i++)
		{
			// apids
			apids[i] = i;
			
			//
			gchar *g_codec = NULL, *g_lang = NULL, *g_lang_title = NULL;
			GstTagList *tags = NULL;
			g_signal_emit_by_name (m_gst_playbin, "get-text-tags", i, &tags);

#if GST_VERSION_MAJOR < 1
			if (tags && gst_is_tag_list(tags))
#else
			if (tags && GST_IS_TAG_LIST(tags))
#endif
			{
				//
				language[i] = "Sub";
				if (gst_tag_list_get_string(tags, GST_TAG_LANGUAGE_CODE, &g_lang))
				{
					language[i] = std::string(g_lang).c_str();
					g_free(g_lang);
				}
				
				//
				if (gst_tag_list_get_string(tags, GST_TAG_TITLE, &g_lang_title))
				{
					language[i] += std::string(g_lang_title).c_str();
					g_free(g_lang_title);
				}
				
				//
				if (gst_tag_list_get_string(tags, GST_TAG_SUBTITLE_CODEC, &g_codec))
				{
					language[i] += " (";
					language[i] += std::string(g_codec).c_str();
					language[i] += ")";
					
					g_free(g_codec);
				}
				
				gst_tag_list_free(tags);
			}
			
			printf("\t%s\n", std::string(language[i]).c_str());
		}
		
		// numpids
		*numpida = i;
	}
#else
	char ** TrackList = NULL;
	
	if(player && player->manager && player->manager->subtitle) 
	{
		player->manager->subtitle->Command(player, MANAGER_LIST, (void*)&TrackList);

		if (TrackList != NULL) 
		{
			int i = 0, j = 0;

			for (i = 0, j = 0; TrackList[i] != NULL; i += 2, j++) 
			{
				printf("\t%s - %s\n", TrackList[i], TrackList[i + 1]);
				
				apids[j] = j;

				language[j] = "Sub";

				language[j] = TrackList[i];
				
				language[j] += " (";
				language[j] += TrackList[i + 1];
				language[j] += ")";
				
				free(TrackList[i]);
				free(TrackList[i + 1]);
			}

			free(TrackList);
			*numpida = j;
		}
	}
#endif
}

////
// subs pids
void cPlayback::FindAllExtSubPids(uint16_t *apids, uint16_t *numpida, std::string *language)
{
	printf("cPlayback::FindAllExtSubPids:\n");

#ifndef ENABLE_GSTREAMER
	char ** TrackList = NULL;
	
	if(player && player->manager && player->manager->extsubtitle) 
	{
		player->manager->extsubtitle->Command(player, MANAGER_LIST, (void*)&TrackList);

		if (TrackList != NULL) 
		{
			int i = 0, j = 0;

			for (i = 0, j = 0; TrackList[i] != NULL; i += 2, j++) 
			{
				printf("\t%s - %s\n", TrackList[i], TrackList[i + 1]);
				
				apids[j] = j;

				language[j] = "Sub";

				language[j] = TrackList[i];
				
				language[j] += " (";
				language[j] += TrackList[i + 1];
				language[j] += ")";
				
				free(TrackList[i]);
				free(TrackList[i + 1]);
			}

			free(TrackList);
			*numpida = j;
		}
	}
#endif
}

//
void cPlayback::AddSubtitleFile(const char* const file)
{
	printf("cPlayback::AddSubtitleFile: %s\n", file? file : "null");
	
#ifndef ENABLE_GSTREAMER
	if (file != NULL)
	{
		player->playback->Command(player, PLAYBACK_OPEN_SUB, (char *)file);
		player->playback->Command(player, PLAYBACK_PLAY_SUB, NULL);
		
//		mExtSubStream = 0;
//		player->playback->Command(player, PLAYBACK_SWITCH_EXTSUBTITLE, (void*)&mExtSubStream);
	}
#endif
}

////
#ifdef USE_OPENGL
#ifndef ENABLE_GSTREAMER
extern Data_t data[64];

cPlayback::SWFramebuffer* cPlayback::getDecBuf(void)
{							
	SWFramebuffer *p = &buffers[0];

	p->resize(data[buf_out].size[0]);
	p->width(data[buf_out].width);
	p->height(data[buf_out].height);
	p->rate(data[buf_out].rate);
	p->vpts(data[buf_out].vpts);
	p->apts(data[buf_out].apts);
	
	av_image_fill_arrays(data[buf_out].buffer, data[buf_out].size, &(*p)[0], AV_PIX_FMT_RGB32, data[buf_out].width, data[buf_out].height, 1);
	
	buf_out++;
	buf_num--;
	buf_out %= 64;

	return p;
}
#endif
#endif

