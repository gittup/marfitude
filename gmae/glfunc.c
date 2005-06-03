/*
   Marfitude
   Copyright (C) 2005 Mike Shal

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
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "SDL.h"
#include "SDL_opengl.h"
#include "SDL_image.h"

#include "glfunc.h"
#include "cfg.h"
#include "fps.h"
#include "log.h"
#include "particles.h"
#include "sdlfatalerror.h"
#include "textures.h"

#include "util/memtest.h"

/** @file
 * Initializes SDL/OpenGL and sets up default OpenGL parameters.
 */

#ifdef __STRICT_ANSI__
extern int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
#endif

static int LoadFont(void);
static void set_icon(void);

/** number of fonts stored in the Font.png file (assumed 16 characters wide) */
#define FONT_SIZE 96
static char newline;  /* offset of a newline character */
static char space;    /* offset of a space character */
static unsigned char numChars; /* total number of characters = FONT_SIZE+space-newline */
/* newline is the first printable character, the characters between newline */
/* and space are unusuable.  The space is stored in the font file, but is not */
/* used because a special case is made for it here (i just don't want to */
/* rearrange the font file for one character :) */

static int screenWidth = 0, screenHeight = 0;

static char *pbuf = NULL;    /* buffer used to hold messages for PrintGL - it is
                              * allocated once and then doubled as necessary
                              * so memory isn't constantly allocated/freed
                              */
static int pbufsize = 0;     /* size of said buffer */
static int sdlInited = 0;    /* do we need to call SDL_Quit? */
static int fontInited = 0;   /* do we need to deallocate font lists/memory? */
static GLuint fontTex;       /* texture with every character in it */
static GLuint fontList;      /* display lists to show individual characters in
                              * the texture
			      */
static float fontSize = 1.0; /* scaling factor for the font */

/** Returns the current screen width */
int DisplayWidth(void)
{
	return screenWidth;
}

/** Returns the current screen height */
int DisplayHeight(void)
{
	return screenHeight;
}

/** Prints the gluErrorString error along with the file and line number */
void GLError(char *file, int line, char *func)
{
	int i = glGetError();
	if(file || line || func) {} /* Get rid of warnings with no logging */
	if(i)
	{
		ELog(("OpenGL (gl%s) Error in file %s line %i: %s\n", func, file, line, gluErrorString(i)));
	}
}

int LoadFont(void)
{
	int x, y;
	fontTex = LoadTexture("Font.png");
	if(!fontTex)
	{
		ELog(("Error loading Font.png!\n"));
		return 0;
	}
	newline = '\n';
	space = ' ';
	numChars = space - newline + FONT_SIZE;
	fontList = glGenLists(numChars);
	glBindTexture(GL_TEXTURE_2D, fontTex);

	glNewList(fontList, GL_COMPILE); /* newline character */
	{
		glPopMatrix(); /* matrix is saved before glCallLists in PrintGL */
		glTranslated(0.0, FONT_HEIGHT, 0.0);
		glPushMatrix();
	} glEndList();
	glNewList(fontList+space-newline, GL_COMPILE); /* space character */
	{
		glTranslated(FONT_WIDTH, 0.0, 0.0);
	} glEndList();
	/* all lists between 10 and 32 are empty, they don't do anything */

	for(y=0;y<6;y++)
	for(x=0;x<16;x++)
	{
		if(!x && !y) continue; /* skip space (already handled above) */
		glNewList(fontList+space-newline+x+(y<<4), GL_COMPILE);
		{
			glBegin(GL_QUADS);
			{
				/* font character is 16x16 in file, */
				/* 10x14 when we pull it out */
				/* so offset by 3 in x and 1 in y since the */
				/* character is centered in the 16x16 block */
				glTexCoord2f(	(3+(double)x*16.0)/256.0,
						(1+(double)y*16.0)/256.0);
						glVertex2i(0, 0);
				glTexCoord2f(	(3+(double)x*16.0 + 16.0)/256.0,
						(1+(double)y*16.0)/256.0);
						glVertex2i(16, 0);
				glTexCoord2f(	(3+(double)x*16.0 + 16.0)/256.0,
						(1+(double)y*16.0 + 16.0)/256.0);
						glVertex2i(16, 16);
				glTexCoord2f(	(3+(double)x*16.0)/256.0,
						(1+(double)y*16.0 + 16.0)/256.0);
						glVertex2i(0, 16);
			} glEnd();
			glTranslated(FONT_WIDTH, 0, 0);
		} glEndList();
	}
	return 1;
}

void set_icon(void)
{
	SDL_Surface *icon;

	icon = IMG_Load("icon.png");
	if(icon) {
		SDL_WM_SetIcon(icon, NULL);
		SDL_FreeSurface(icon);
	}
}

/** Loads the font, and sets up the OpenGL parameters to some nice defaults */
int InitGL(void)
{
	Uint32 flags = SDL_OPENGL;
	SDL_Surface *screen;
	float ambientLight[4] = {0.7, 0.7, 0.7, 0.7};
	float diffuseLight[4] = {1.0, 1.0, 1.0, 1.0};
	float blackLight[4] = {0.0, 0.0, 0.0, 1.0};
	float light0[4] = {0.0, 1.0, 0.0, 0.0};
	float light0amb[4] = {0.3, 0.3, 0.3, 1.0};
	float light0dif[4] = {0.4, 0.4, 0.4, 1.0};

	printf("Starting video...\n");
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_TIMER) < 0)
	{
		SDLError("initializing");
		return 1;
	}
	sdlInited = 1;
	Log(("SDL Initialized\n"));

	screenWidth = CfgI("video.width");
	screenHeight = CfgI("video.height");

	switch(CfgI("video.bpp"))
	{
		case 32:
		case 24:
			SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
			SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
			break;
		case 16:
			SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
			SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
			SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
			break;
		case 0:
			break;
		default:
			Log(("Warning: Bpp not 0/16/24/32, this may not work\n"));
	}
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	if(CfgEq("video.fullscreen", "yes")) flags |= SDL_FULLSCREEN;
	screen = SDL_SetVideoMode(CfgI("video.width"), CfgI("video.height"), CfgI("video.bpp"), flags);
	if(screen == NULL)
	{
		SDLError("setting video mode");
		return 2;
	}
	Log(("Video mode set: (%i, %i)\n", CfgI("video.width"), CfgI("video.height")));
	SDL_WM_SetCaption("Gmae", NULL); /* second arg is icon */
	InitFPS();
	glViewport(0, 0, screenWidth, screenHeight);
	glEnable(GL_TEXTURE_2D);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1.0);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	glMaterialfv(GL_FRONT, GL_EMISSION, blackLight);

	glPolygonOffset(1.0, 1.0);
	glEnable(GL_POLYGON_OFFSET_FILL);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluPerspective(45.0f,(GLfloat)screenWidth/(GLfloat)screenHeight,0.1f,100.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_LIGHTING);
	glLightfv(GL_LIGHT0, GL_POSITION, light0);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light0amb);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light0dif);
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT1, GL_AMBIENT, ambientLight);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuseLight);
	glEnable(GL_LIGHT1);

	if(!LoadFont())
	{
		return 3;
	}
	set_icon();
	fontInited = 1;

	pbufsize = 128;
	pbuf = malloc(sizeof(char) * pbufsize);

	if(InitTextures())
	{
		ELog(("ERROR: Couldn't load textures!\n"));
		return 4;
	}

	if(!InitParticles())
	{
		ELog(("ERROR: Couldn't load particles!\n"));
		return 5;
	}

	return 0;
}

/** Shuts down OpenGL */
void QuitGL(void)
{
	if(!sdlInited || !fontInited) return;
	QuitParticles();
	QuitTextures();
	free(pbuf);
	if(fontInited)
	{
		glDeleteLists(fontList, numChars);
		glDeleteTextures(1, &fontTex);
	}
	if(sdlInited)
	{
		SDL_Quit();
		printf("OpenGL shutdown\n");
	}
}

/** Sets an orthographic projection matrix. Be sure to call ResetProjection()
 * afterward.
 */
void SetOrthoProjection(void)
{
	glMatrixMode(GL_PROJECTION);
	glPushMatrix(); /* popped in ResetProjection() */
	glLoadIdentity();
	glOrtho(0, screenWidth, screenHeight, 0, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix(); /* popped in ResetProjection() */
	glLoadIdentity();
	glDisable(GL_LIGHTING);
}

/** Resets the projection and modelview matrix. This balances out the
 * SetOrthoProjection() function.
 */
void ResetProjection(void)
{
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glEnable(GL_LIGHTING);
}

/** Swap buffers and update FPS */
void UpdateScreen(void)
{
	PrintFPS();
	SDL_GL_SwapBuffers();
	UpdateFPS();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

/** Sets the scaling factor for the font display */
void SetFontSize(float size)
{
	fontSize = size;
}

/** Prints a message to the screen and position x, y.
 * @param x The x location
 * @param y The y location
 * @param msg The message to print, as in printf. The remaining arguments are
 * the parameters for the message.
 */
void PrintGL(int x, int y, const char *msg, ...)
{
	va_list ap;
	int len;

	if(!pbufsize)
	{
		pbufsize = 128;
		pbuf = malloc(sizeof(char)*pbufsize);
	}
	va_start(ap, msg);
	while((len = vsnprintf(pbuf, pbufsize, msg, ap)) >= pbufsize)
	{
		va_end(ap); va_start(ap, msg);
		pbufsize *= 2;
		Log(("Warning: Doubling print buffer size: %i bytes\n", pbufsize));
		pbuf = (char*)realloc(pbuf, sizeof(char*)*pbufsize);
	}
	va_end(ap);

	/* fps stuff from
	 * http://www.geocities.com/SiliconValley/Hills/6287/fps/
	 * also from NeHe tutorial
	 */
	glBindTexture(GL_TEXTURE_2D, fontTex);
	SetOrthoProjection();
	glPushMatrix();
	glTranslated((double)x, (double)y, 0);
	glScalef(fontSize, fontSize, 1.0);
	glPushAttrib(GL_LIST_BIT);
	glListBase(fontList-newline);
	glPushMatrix(); /* save position for newline characters */
	glCallLists(len, GL_UNSIGNED_BYTE, pbuf);
	glPopMatrix();
	glPopAttrib();
	glPopMatrix();
	ResetProjection();
}
