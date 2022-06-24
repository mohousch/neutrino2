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

#define __STDC_CONSTANT_MACROS
#include <iostream>
#include <vector>
#include <deque>
#include "global.h"
#include "neutrinoMessages.h"

#include <sys/types.h>
#include <signal.h>

#include <sys/types.h>
#include <unistd.h>
#include "glthread.h"
#include <GL/glx.h>

#include <system/debug.h>


/*static*/ GLThreadObj *gThiz = 0; /* GLUT does not allow for an arbitrary argument to the render func */
int GLWinID;
int GLxStart;
int GLyStart;
int GLWidth;
int GLHeight;

GLThreadObj::GLThreadObj(int x, int y) : mX(x), mY(y), mReInit(true), mShutDown(false), mInitDone(false)
{
	mState.width  = mX;
	mState.height = mY;
	mState.blit = true;

	initKeys();
}


GLThreadObj::~GLThreadObj()
{
}

void GLThreadObj::initKeys()
{
	mSpecialMap[GLUT_KEY_UP]    = RC_up;
	mSpecialMap[GLUT_KEY_DOWN]  = RC_down;
	mSpecialMap[GLUT_KEY_LEFT]  = RC_left;
	mSpecialMap[GLUT_KEY_RIGHT] = RC_right;

	mSpecialMap[GLUT_KEY_F1] = RC_red;
	mSpecialMap[GLUT_KEY_F2] = RC_green;
	mSpecialMap[GLUT_KEY_F3] = RC_yellow;
	mSpecialMap[GLUT_KEY_F4] = RC_blue;
	
	mSpecialMap[GLUT_KEY_F5] = RC_play;
	mSpecialMap[GLUT_KEY_F6] = RC_stop;
	mSpecialMap[GLUT_KEY_F7] = RC_pause;
	mSpecialMap[GLUT_KEY_F8] = RC_rewind;
	mSpecialMap[GLUT_KEY_F9] = RC_forward;
	mSpecialMap[GLUT_KEY_F10] = RC_loop;
	mSpecialMap[GLUT_KEY_F11] = RC_record;
	mSpecialMap[GLUT_KEY_F12] = RC_mode;

	mSpecialMap[GLUT_KEY_PAGE_UP]   = RC_page_up;
	mSpecialMap[GLUT_KEY_PAGE_DOWN] = RC_page_down;

	mKeyMap[0x0d] = RC_ok;
	mKeyMap[0x1b] = RC_home;
	mKeyMap['i']  = RC_info;
	mKeyMap['m']  = RC_setup;

	mKeyMap['s']  = RC_spkr;
	mKeyMap['+']  = RC_plus;
	mKeyMap['-']  = RC_minus;
	
	mKeyMap['h']  = RC_info;

	mKeyMap['0']  = RC_0;
	mKeyMap['1']  = RC_1;
	mKeyMap['2']  = RC_2;
	mKeyMap['3']  = RC_3;
	mKeyMap['4']  = RC_4;
	mKeyMap['5']  = RC_5;
	mKeyMap['6']  = RC_6;
	mKeyMap['7']  = RC_7;
	mKeyMap['8']  = RC_8;
	mKeyMap['9']  = RC_9;
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
			/* start decode thread */
			gThiz = this;
			//glutSetCursor(GLUT_CURSOR_NONE);
			glutDisplayFunc(GLThreadObj::rendercb);
			glutKeyboardFunc(GLThreadObj::keyboardcb);
			glutSpecialFunc(GLThreadObj::specialcb);
			glutReshapeFunc(GLThreadObj::resizecb);
			setupGLObjects(); /* needs GLEW prototypes */
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
	{ /* yeah, whatever... */
		::kill(getpid(), SIGKILL);
	}

	dprintf(DEBUG_NORMAL, "GLThreadObj::run: GL thread stopping\n");
}

void GLThreadObj::setupCtx()
{
	int argc = 1;
	/* some dummy commandline for GLUT to be happy */
	char const *argv[2] = { "neutrino", 0 };
	dprintf(DEBUG_NORMAL, "GLThreadObj::setupCtx: GL thread starting\n");
	glutInit(&argc, const_cast<char **>(argv));
	glutInitWindowSize(mX, mY);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutCreateWindow("neutrino2");
	
	//
	GLWinID = glXGetCurrentDrawable();
	GLxStart = mX;
	GLyStart = mY;
	GLWidth = getOSDWidth();
	GLHeight = getOSDHeight();
	//
}

void GLThreadObj::setupOSDBuffer()
{	
	/* 
	   the OSD buffer size can be decoupled from the actual
	   window size since the GL can blit-stretch with no
	   trouble at all, ah, the luxury of ignorance... 
	*/
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
	glBindTexture(GL_TEXTURE_2D, mState.osdtex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mState.width, mState.height, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glBindTexture(GL_TEXTURE_2D, mState.displaytex); /* we do not yet know the size so will set that inline */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glGenBuffers(1, &mState.osdpbo);
	glGenBuffers(1, &mState.displaypbo);
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
	std::map<unsigned char, neutrino_msg_t>::const_iterator i = gThiz->mKeyMap.find(key);
	if(i != gThiz->mKeyMap.end())
	{ 
		/* let's assume globals are thread-safe */
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
		gThiz->mReInit = true;
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
		mReInit = false;
		glViewport(0, 0, mX, mY);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		float aspect = static_cast<float>(mX)/mY;
		float osdaspect = 1.0/(static_cast<float>(mState.width)/mState.height);
		
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
	
	if(mX != glutGet(GLUT_WINDOW_WIDTH) && mY != glutGet(GLUT_WINDOW_HEIGHT))
		glutReshapeWindow(mX, mY);
	
	// video display
	bltDisplayBuffer();

	// OSD
	if (mState.blit) 
	{
		mState.blit = false;
		bltOSDBuffer();
	}

	// clear 
	glBindTexture(GL_TEXTURE_2D, mState.osdtex);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// Display
	glBindTexture(GL_TEXTURE_2D, mState.displaytex);
	drawSquare(1.0);
	
	// OSD
	glBindTexture(GL_TEXTURE_2D, mState.osdtex);
	drawSquare(1.0);

	glFlush();
	glutSwapBuffers();

	GLuint err = glGetError();
	if(err != 0)
	{
		dprintf(DEBUG_NORMAL, "GLThreadObj::render: GLError:%d 0x%04x\n", err, err);
	}

	// simply limit to 30 Hz, if anyone wants to do this properly, feel free
	usleep(sleep_us);
	
	glutPostRedisplay();
}

void GLThreadObj::resizecb(int w, int h)
{
	gThiz->checkReinit(w, h);
}

void GLThreadObj::checkReinit(int x, int y)
{
	x = glutGet(GLUT_WINDOW_WIDTH);
	y = glutGet(GLUT_WINDOW_HEIGHT);
	
	if( x != mX || y != mY )
	{
		mX = x;
		mY = y;
		mReInit = true;
	}
}

void GLThreadObj::drawSquare(float size)
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
		usleep(1);
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

void GLThreadObj::bltDisplayBuffer()
{
	// set displayer buffer
	mDisplayBuffer.resize(5*1024*1024);

	//dprintf(DEBUG_NORMAL, "GLThreadObj::bltDisplayBuffer: DisplayBuffer set to %d bytes\n", 5*1024*1024);
	
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, mState.displaypbo);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, mDisplayBuffer.size(), &mDisplayBuffer[0], GL_STREAM_DRAW_ARB);

	glBindTexture(GL_TEXTURE_2D, mState.displaytex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mState.width, mState.height, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	
	sleep_us = 1;
}

void GLThreadObj::clear()
{
	memset(&mOSDBuffer[0], 0, mOSDBuffer.size());
}

