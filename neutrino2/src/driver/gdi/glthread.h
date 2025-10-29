//
//	Neutrino-GUI  -   DBoxII-Project
//
//	$Id: glthread.h 30.10.2023 mohousch Exp $
//
//	Copyright 2010 Carsten Juttner <carjay@gmx.net>
//	Copyright 2012 Stefan Seyfried <seife@tuxboxcvs.slipkontur.de>
//
//	License: GPL
//
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program; if not, write to the Free Software
//	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#ifndef __glthread__
#define __glthread__
#include <OpenThreads/Thread>
#include <vector>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/gl.h>

extern "C" {
#include <libavutil/rational.h>
}

#include <driver/rcinput.h>


typedef enum
{
	DISPLAY_AR_MODE_PANSCAN = 0,
	DISPLAY_AR_MODE_LETTERBOX,
	DISPLAY_AR_MODE_NONE
} DISPLAY_AR_MODE;


class GLThreadObj : public OpenThreads::Thread
{
	public:
		GLThreadObj(int x, int y);
		~GLThreadObj(){};

		//
		void run();
		void Start() { OpenThreads::Thread::start(); }
		void waitInit();					/* wait until things are set up */
		void shutDown() { mShutDown = true; }			/* shut down event loop (causes thread to exit) */
		void join() { OpenThreads::Thread::join(); }
		unsigned char *getOSDBuffer() { return &mOSDBuffer[0]; } 	/* gets pointer to OSD bounce buffer */
		//
		int getOSDBufferSize() { return OSDBufferSize; };
		int getOSDWidth() { return mState.width; }
		int getOSDHeight() { return mState.height; }
		//
		void clear();
		void blit() { mState.blit = true; }
		
		//
		int64_t last_apts;
		AVRational mOA;         	/* output window aspect ratio */
		AVRational mVA;         	/* video aspect ratio */
		AVRational _mVA;        	/* for detecting changes in mVA */
		bool mVAchanged;
		float zoom;         		/* for cropping */
		float xscale;           	/* and aspect ratio */
		int mCrop;          		/* DISPLAY_AR_MODE */
		
		bool mFullscreen;       	/* fullscreen? */

	private:
		int *mX;
		int *mY;
		int _mX[2];         		/* output window size */
		int _mY[2];         		/* [0] = normal, [1] = fullscreen */
		//
		bool mReInit;			/* setup things for GL */
		bool mShutDown;			/* if set main loop is left */
		bool mInitDone;			/* condition predicate */

		std::vector<unsigned char> mOSDBuffer; /* silly bounce buffer */
		int OSDBufferSize;

		std::map<unsigned char, neutrino_msg_t> mKeyMap;
		std::map<int, neutrino_msg_t> mSpecialMap;

		static void resizecb(int w, int h);
		void checkReinit(int x, int y);	/* e.g. in case window was resized */
		static void rendercb();		/* callback for GLUT */
		void render();			/* actual render function */
		static void keyboardcb(unsigned char key, int x, int y);
		static void specialcb(int key, int x, int y);

		void initKeys();		/* setup key bindings for window */
		void setupCtx();		/* create the window and make the context current */
		void setupOSDBuffer();		/* create the OSD buffer */
		void setupGLObjects();		/* PBOs, textures and stuff */
		void releaseGLObjects();
		void drawSquare(float size, float x_factor);	/* do not be square */
		void initDone();		/* "things are now set up", called by this */

		struct {
			int width;		/* width and height, fixed for a framebuffer instance */
			int height;
			GLuint osdtex;		/* holds the OSD texture */
			GLuint osdpbo;		/* PBO we use for transfer to texture */
			GLuint displaytex;	/* holds the display texture */
			GLuint displaypbo;
			bool blit;
		} mState;

		void bltOSDBuffer();
		void bltDisplayBuffer();
		void bltPlayBuffer();
};

#endif

