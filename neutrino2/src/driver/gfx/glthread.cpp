/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright 2010 Carsten Juttner <carjay@gmx.net>
	Copyright 2012 Stefan Seyfried <seife@tuxboxcvs.slipkontur.de>

	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <vector>
#include <deque>
#include "global.h"
#include "neutrinoMessages.h"

#include <sys/types.h>
#include <signal.h>
#include <inttypes.h>

#include <unistd.h>
#include "glthread.h"
#include <GL/glx.h>

#include <system/debug.h>

#include <audio_cs.h>
#include <video_cs.h>
#include <playback_cs.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
}


//// globals
GLThreadObj *gThiz = 0; /* GLUT does not allow for an arbitrary argument to the render func */
int GLWinID;
int GLxStart;
int GLyStart;
int GLWidth;
int GLHeight;
////
extern cVideo *videoDecoder;
extern cAudio *audioDecoder;
extern cPlayback *playback;
#ifdef ENABLE_GSTREAMER
extern uint32_t framerate;
#endif

GLThreadObj::GLThreadObj(int x, int y) : /*mX(x), mY(y),*/ mReInit(true), mShutDown(false), mInitDone(false)
{
	mState.width  = x;
	mState.height = y;
	mState.blit = true;
	////
	mX = &_mX[0];
	mY = &_mY[0];
	*mX = x;
	*mY = y;
	av_reduce(&mOA.num, &mOA.den, x, y, INT_MAX);
	mVA = mOA; 
	_mVA = mVA;
	mVAchanged = true;
	last_apts = 0;
	mCrop = DISPLAY_AR_MODE_PANSCAN;
	zoom = 1.0;
	xscale = 1.0;
	mFullscreen = false;

	initKeys();
}

void GLThreadObj::initKeys()
{
	mSpecialMap[GLUT_KEY_UP]    = CRCInput::RC_up;
	mSpecialMap[GLUT_KEY_DOWN]  = CRCInput::RC_down;
	mSpecialMap[GLUT_KEY_LEFT]  = CRCInput::RC_left;
	mSpecialMap[GLUT_KEY_RIGHT] = CRCInput::RC_right;

	mSpecialMap[GLUT_KEY_F1] = CRCInput::RC_red;
	mSpecialMap[GLUT_KEY_F2] = CRCInput::RC_green;
	mSpecialMap[GLUT_KEY_F3] = CRCInput::RC_yellow;
	mSpecialMap[GLUT_KEY_F4] = CRCInput::RC_blue;
	
	mSpecialMap[GLUT_KEY_F5] = CRCInput::RC_play;
	mSpecialMap[GLUT_KEY_F6] = CRCInput::RC_stop;
	mSpecialMap[GLUT_KEY_F7] = CRCInput::RC_pause;
	mSpecialMap[GLUT_KEY_F8] = CRCInput::RC_rewind;
	mSpecialMap[GLUT_KEY_F9] = CRCInput::RC_forward;
	mSpecialMap[GLUT_KEY_F10] = CRCInput::RC_loop;
	mSpecialMap[GLUT_KEY_F11] = CRCInput::RC_record;

	mSpecialMap[GLUT_KEY_PAGE_UP]   = CRCInput::RC_page_up;
	mSpecialMap[GLUT_KEY_PAGE_DOWN] = CRCInput::RC_page_down;

	mKeyMap[0x0d] = CRCInput::RC_ok;
	mKeyMap[0x1b] = CRCInput::RC_home;
	mKeyMap['i']  = CRCInput::RC_info;
	mKeyMap['m']  = CRCInput::RC_setup;
	mKeyMap['e']  = CRCInput::RC_epg;
	mKeyMap['t']  = CRCInput::RC_text;
	mKeyMap['d']  = CRCInput::RC_dvbsub;

	mKeyMap['s']  = CRCInput::RC_spkr;
	mKeyMap['+']  = CRCInput::RC_plus;
	mKeyMap['-']  = CRCInput::RC_minus;
	
	mKeyMap['h']  = CRCInput::RC_info;

	mKeyMap['0']  = CRCInput::RC_0;
	mKeyMap['1']  = CRCInput::RC_1;
	mKeyMap['2']  = CRCInput::RC_2;
	mKeyMap['3']  = CRCInput::RC_3;
	mKeyMap['4']  = CRCInput::RC_4;
	mKeyMap['5']  = CRCInput::RC_5;
	mKeyMap['6']  = CRCInput::RC_6;
	mKeyMap['7']  = CRCInput::RC_7;
	mKeyMap['8']  = CRCInput::RC_8;
	mKeyMap['9']  = CRCInput::RC_9;
}

void GLThreadObj::run()
{
	setupCtx();
	setupOSDBuffer();
	initDone(); // signal that setup is finished

	// init the good stuff
	GLenum err = glewInit();

	if(err == GLEW_OK)
	{
		if((!GLEW_VERSION_1_5)||(!GLEW_EXT_pixel_buffer_object)||(!GLEW_ARB_texture_non_power_of_two))
		{
			dprintf(DEBUG_NORMAL, "GLThreadObj::run: Sorry, your graphics card is not supported. Needs at least OpenGL 1.5, pixel buffer objects and NPOT textures.\n");
			perror("incompatible graphics card");
			_exit(1);
		}
		else
		{
			// start decode thread
			gThiz = this;
			//glutSetCursor(GLUT_CURSOR_NONE);
			glutDisplayFunc(GLThreadObj::rendercb);
			glutKeyboardFunc(GLThreadObj::keyboardcb);
			glutSpecialFunc(GLThreadObj::specialcb);
			glutReshapeFunc(GLThreadObj::resizecb);
			setupGLObjects();
			glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);
			glutMainLoop();
			releaseGLObjects();
		}
	}
	else
	{
		dprintf(DEBUG_NORMAL, "GLThreadObj::run: GLThread: error initializing glew: %d\n", err);
	}
	
	if(g_RCInput)
	{
		g_RCInput->postMsg(NeutrinoMessages::SHUTDOWN, 0);
	}
	else
	{
		::kill(getpid(), SIGKILL);
	}

	dprintf(DEBUG_NORMAL, "GLThreadObj::run: GL thread stopping\n");
}

void GLThreadObj::setupCtx()
{
	int argc = 1;
	char const *argv[2] = { "neutrino2", 0 };
	dprintf(DEBUG_NORMAL, "GLThreadObj::setupCtx: GL thread starting\n");
	glutInit(&argc, const_cast<char **>(argv));
	glutInitWindowSize(*mX, *mY);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutCreateWindow("neutrino2");
	
	//
	GLWinID = glXGetCurrentDrawable();
	GLxStart = *mX;
	GLyStart = *mY;
	GLWidth = getOSDWidth();
	GLHeight = getOSDHeight();
}

void GLThreadObj::setupOSDBuffer()
{	
	if(mState.width && mState.height)
	{
		int fbmem = mState.width * mState.height * 4 * 2;
		mOSDBuffer.resize(fbmem);
		dprintf(DEBUG_NORMAL, "GLThreadObj::setupOSDBuffer: OSD buffer set to %d bytes\n", fbmem);
	}
}

void GLThreadObj::setupGLObjects()
{
	glGenTextures(1, &mState.osdtex);
	glGenTextures(1, &mState.displaytex);
	
	//
	glBindTexture(GL_TEXTURE_2D, mState.osdtex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mState.width, mState.height, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	
	// 
	glBindTexture(GL_TEXTURE_2D, mState.displaytex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glGenBuffers(1, &mState.osdpbo);
	glGenBuffers(1, &mState.displaypbo);
	
	//
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, mState.displaypbo);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, mOSDBuffer.size(), &mOSDBuffer[0], GL_STREAM_DRAW_ARB);
	glBindTexture(GL_TEXTURE_2D, mState.displaytex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

void GLThreadObj::releaseGLObjects()
{
	glDeleteTextures(1, &mState.osdtex);
	glDeleteTextures(1, &mState.displaytex);
	glDeleteBuffers(1, &mState.osdpbo);
	glDeleteBuffers(1, &mState.displaypbo);
}

void GLThreadObj::rendercb()
{
	gThiz->render();
}

void GLThreadObj::keyboardcb(unsigned char key, int /*x*/, int /*y*/)
{
	if (key == 'f')
	{
		gThiz->mFullscreen = !gThiz->mFullscreen;
		gThiz->mReInit = true;
		
		return;
	}
	
	std::map<unsigned char, neutrino_msg_t>::const_iterator i = gThiz->mKeyMap.find(key);
	
	if(i != gThiz->mKeyMap.end())
	{ 
		if(g_RCInput)
		{
			g_RCInput->postMsg(i->second, 0);
		}
	}
}

void GLThreadObj::specialcb(int key, int /*x*/, int /*y*/)
{
	std::map<int, neutrino_msg_t>::const_iterator i = gThiz->mSpecialMap.find(key);
	
	if(key == GLUT_KEY_F12)
	{
		gThiz->mFullscreen = !gThiz->mFullscreen;
		gThiz->mReInit = true;
		
		return;
	}
	else if(i != gThiz->mSpecialMap.end())
	{
		if(g_RCInput)
		{
			g_RCInput->postMsg(i->second, 0);
		}
	}
}
  
int sleep_us = 30000;
void GLThreadObj::render() 
{
	if(mShutDown)
	{
		glutLeaveMainLoop();
	}

	if(mReInit)
	{
		int xoff = 0;
		int yoff = 0;
		mVAchanged = true;
		mReInit = false;
		mX = &_mX[mFullscreen];
		mY = &_mY[mFullscreen];
		
		//
		if (mFullscreen)
		{
			int x = glutGet(GLUT_SCREEN_WIDTH);
			int y = glutGet(GLUT_SCREEN_HEIGHT);
			*mX = x;
			*mY = y;
			
			AVRational a = { x, y };
			
			if (av_cmp_q(a, mOA) < 0)
				*mY = x * mOA.den / mOA.num;
			else if (av_cmp_q(a, mOA) > 0)
				*mX = y * mOA.num / mOA.den;
				
			xoff = (x - *mX) / 2;
			yoff = (y - *mY) / 2;
			
			glutFullScreen();
		}
		else
			*mX = *mY * mOA.num / mOA.den;
			
		//
		glViewport(xoff, yoff, *mX, *mY);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		float aspect = static_cast<float>(*mX) / *mY;
		float osdaspect = static_cast<float>(mOA.den) / mOA.num;
		
		glOrtho(aspect*-osdaspect, aspect*osdaspect, -1.0, 1.0, -1.0, 1.0 );
		glClearColor(0.0, 0.0, 0.0, 1.0);
		
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glEnable(GL_BLEND);
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_DEPTH_TEST);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	
	glutPostOverlayRedisplay();
	
	if(!mFullscreen &&  (*mX != glutGet(GLUT_WINDOW_WIDTH) && *mY != glutGet(GLUT_WINDOW_HEIGHT)) )
		glutReshapeWindow(*mX, *mY);
		
	//
	bltDisplayBuffer();
	bltPlayBuffer();

	// OSD
	if (mState.blit) 
	{
		mState.blit = false;
		bltOSDBuffer();
	}
	
	////
//	glBindTexture(GL_TEXTURE_2D, mState.osdtex);
//	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	//
	if (mVAchanged)
	{
		mVAchanged = false;
		zoom = 1.0;
		xscale = 1.0;
		int cmp = (mCrop == DISPLAY_AR_MODE_NONE) ? 0 : av_cmp_q(mVA, mOA);
		const AVRational a149 = { 14, 9 };
		
		switch (cmp)
		{
			default:
			case INT_MIN:
			case 0: 
				break;
			case 1:
				xscale = av_q2d(mVA) / av_q2d(mOA);
				switch (mCrop)
				{
					case DISPLAY_AR_MODE_PANSCAN:
						break;
						
					case DISPLAY_AR_MODE_LETTERBOX:
						zoom = av_q2d(mOA) / av_q2d(mVA);
						break;
						
					default:
						break;
				}
				break;
			case -1:
				xscale = av_q2d(mVA) / av_q2d(mOA);
				switch (mCrop)
				{
					case DISPLAY_AR_MODE_LETTERBOX:
						break;
						
					case DISPLAY_AR_MODE_PANSCAN:
						zoom = av_q2d(mOA) / av_q2d(mVA);
						break;
						
					default:
						break;
				}
				break;
		}
	}
	
	//
//	glBindTexture(GL_TEXTURE_2D, mState.osdtex);
//	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindTexture(GL_TEXTURE_2D, mState.displaytex);
	drawSquare(zoom, xscale);
	
	// OSD
	glBindTexture(GL_TEXTURE_2D, mState.osdtex);
	drawSquare(1.0, -100);

	glFlush();
	glutSwapBuffers();

	// simply limit to 30 Hz, if anyone wants to do this properly, feel free	
	usleep(sleep_us);
	
//	printf("sleep_us: %d\n", sleep_us);
	
	glutPostRedisplay();
}

void GLThreadObj::resizecb(int w, int h)
{
	gThiz->checkReinit(w, h);
}

void GLThreadObj::checkReinit(int x, int y)
{
	static int last_x = 0, last_y = 0;

	if (!mFullscreen && !mReInit && (x != *mX || y != *mY))
	{
		if (x != *mX && abs(x - last_x) > 2)
		{
			*mX = x;
			*mY = *mX * mOA.den / mOA.num;
		}
		else if (y != *mY && abs(y - last_y) > 2)
		{
			*mY = y;
			*mX = *mY * mOA.num / mOA.den;
		}
		
		mReInit = true;
	}

	last_x = x;
	last_y = y;
}

void GLThreadObj::drawSquare(float size, float x_factor)
{
	GLfloat vertices[] = {
		 1.0f,  1.0f,
		-1.0f,  1.0f,
		-1.0f, -1.0f,
		 1.0f, -1.0f,
	};

	GLubyte indices[] = { 0, 1, 2, 3 };

	GLfloat texcoords[] = {
		 1.0, 0.0,
		 0.0, 0.0,
		 0.0, 1.0,
		 1.0, 1.0,
	};
	
	//
	if (x_factor > -99.0)   /* x_factor == -100 => OSD */
	{
		if (videoDecoder &&
		    videoDecoder->pig_x > 0 && videoDecoder->pig_y > 0 &&
		    videoDecoder->pig_w > 0 && videoDecoder->pig_h > 0)
		{
			/* these calculations even consider cropping and panscan mode
			 * maybe this could be done with some clever opengl tricks? */
			double w2 = (double)mState.width * 0.5l;
			double h2 = (double)mState.height * 0.5l;
			double x = (double)(videoDecoder->pig_x - w2) / w2 / x_factor / size;
			double y = (double)(h2 - videoDecoder->pig_y) / h2 / size;
			double w = (double)videoDecoder->pig_w / w2;
			double h = (double)videoDecoder->pig_h / h2;
			x += ((1.0l - x_factor * size) / 2.0l) * w / x_factor / size;
			y += ((size - 1.0l) / 2.0l) * h / size;
			vertices[0] = x + w;        	/* top right x */
			vertices[1] = y;        	/* top right y */
			vertices[2] = x;        	/* top left x */
			vertices[3] = y;        	/* top left y */
			vertices[4] = x;        	/* bottom left x */
			vertices[5] = y - h;        	/* bottom left y */
			vertices[6] = vertices[0];  	/* bottom right x */
			vertices[7] = vertices[5];  	/* bottom right y */
		}
	}
	else
		x_factor = 1.0; /* OSD */

	glPushMatrix();
	glScalef(size, size, size);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, vertices);
	glTexCoordPointer(2, GL_FLOAT, 0, texcoords);
	glDrawElements(GL_QUADS, 4, GL_UNSIGNED_BYTE, indices);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glPopMatrix();
}


void GLThreadObj::initDone()
{
	mInitDone = true;
}

void GLThreadObj::waitInit()
{
	while(!mInitDone)
	{
		//usleep(1);
	}
}


void GLThreadObj::bltOSDBuffer()
{
	// FIXME: copy each time
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, mState.osdpbo);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, mOSDBuffer.size(), &mOSDBuffer[0], GL_STREAM_DRAW_ARB);

	glBindTexture(GL_TEXTURE_2D, mState.osdtex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mState.width, mState.height, GL_BGRA, GL_UNSIGNED_BYTE, 0);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

void GLThreadObj::clear()
{
	memset(&mOSDBuffer[0], 0, mOSDBuffer.size());
}

void GLThreadObj::bltDisplayBuffer()
{
	if (!videoDecoder)
		return;
	
	cVideo::SWFramebuffer* buf = videoDecoder->getDecBuf();
	
	if (!buf)
	{		
		//
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, mState.displaypbo);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, mOSDBuffer.size(), &mOSDBuffer[0], GL_STREAM_DRAW_ARB);
		glBindTexture(GL_TEXTURE_2D, mState.displaytex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
			
		return;
	}
	
	//
	int w = buf->width(), h = buf->height();
	
	if (w == 0 || h == 0)
		return;

	AVRational a = buf->AR();
	
	if (a.den != 0 && a.num != 0 && av_cmp_q(a, _mVA))
	{
		_mVA = a;
		// _mVA is the raw buffer's aspect, mVA is the real scaled output aspect
		av_reduce(&mVA.num, &mVA.den, w * a.num, h * a.den, INT_MAX);
		mVAchanged = true;
	}

	// render frame
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, mState.displaypbo);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, buf->size(), &(*buf)[0], GL_STREAM_DRAW_ARB);

	glBindTexture(GL_TEXTURE_2D, mState.displaytex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	
	//
	int64_t apts = 0;
	int64_t vpts = buf->pts() + 18000;
	
	if (audioDecoder)
		apts = audioDecoder->getPts();
			
	if (apts != last_apts)
	{
		if (apts < vpts)
			sleep_us = (sleep_us * 2 + (vpts - apts) * 10 / 9) / 3;
		else if (sleep_us > 1000)
			sleep_us -= 1000;
		
		last_apts = apts;
		
		//	
		int rate, dummy1, dummy2;
		
		videoDecoder->getPictureInfo(dummy1, dummy2, rate);
		
		if (rate > 0)
			rate = 2000000 / rate;
		else
			rate = 50000;
			
		if (sleep_us > rate)
			sleep_us = rate;
		else if (sleep_us < 1)
			sleep_us = 1;
	}
}

//
void GLThreadObj::bltPlayBuffer()
{
	if (!playback)
		return;
		 
	if (playback && !playback->playing)
		return;
	
#ifdef ENABLE_GSTREAMER
	sleep_us = framerate / 100;
			
	if (sleep_us > framerate)
		sleep_us = framerate;
	else if (sleep_us < 1)
		sleep_us = 1;
#else
	cPlayback::SWFramebuffer *buf = playback->getDecBuf();
	
	//
	if (buf == NULL)
	{		
		//
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, mState.displaypbo);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, mOSDBuffer.size(), &mOSDBuffer[0], GL_STREAM_DRAW_ARB);
		glBindTexture(GL_TEXTURE_2D, mState.displaytex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		
		return;
	}
	
	int w = buf->width(), h = buf->height();
	
	if (w == 0 || h == 0)
		return;
		
	AVRational a = buf->AR();
	
	//
	if (a.den != 0 && a.num != 0 && av_cmp_q(a, _mVA))
	{
		_mVA = a;
		// _mVA is the raw buffer's aspect, mVA is the real scaled output aspect
		av_reduce(&mVA.num, &mVA.den, w * a.num, h * a.den, INT_MAX);
		mVAchanged = true;
	}

	// render frame
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, mState.displaypbo);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, buf->size(), &(*buf)[0], GL_STREAM_DRAW_ARB);

	glBindTexture(GL_TEXTURE_2D, mState.displaytex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	
	// FIXME:
	int64_t apts = buf->apts();
	int64_t vpts = buf->vpts() + 18000;
	int rate = buf->rate();
			
	if (apts != last_apts)
	{
		if (apts < vpts)
			sleep_us = (sleep_us * 2 + (vpts - apts) * 10 / 9) / 3;
		else if (sleep_us > 1000)
			sleep_us -= 1000;
		
		last_apts = apts;
		
		//
		if (rate > 0)
			rate = 2000000 / rate;
		else
			rate = 50000;
			
		if (sleep_us > rate)
			sleep_us = rate;
		else if (sleep_us < 1)
			sleep_us = 1;
	}
#endif
}

