#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "SDL.h"
#include "GL/gl.h"
#include "GL/glu.h"

#include "glfunc.h"
#include "fps.h"
#include "cfg.h"
#include "textures.h"
#include "log.h"
#include "../util/memtest.h"
#include "../util/sdlfatalerror.h"

#define FONT_SIZE 96    // number of fonts stored in the Font.png file
			// (assumed 16 characters wide)
unsigned char newline;  // offset of a newline character
unsigned char space;    // offset of a space character
unsigned char numChars; // total number of characters = FONT_SIZE+space-newline
// newline is the first printable character, the characters between newline
// and space are unusuable.  The space is stored in the font file, but is not
// used because a special case is made for it here (i just don't want to
// rearrange the font file for one character :)

int screenWidth = 0, screenHeight = 0;

char *pbuf = NULL;	// buffer used to hold messages for PrintGL - it is
			// allocated once and then doubled as necessary
			// so memory isn't constantly allocated/freed
int pbufsize = 0;	// size of said buffer
int sdlInited = 0;	// do we need to call SDL_Quit?
int fontInited = 0;	// do we need to deallocate font lists/memory?
GLuint fontTex;		// texture with every character in it
GLuint fontList;	// display lists to show individual characters in the
			// texture
float fontSize = 1.0;	// scaling factor for the font

int DisplayWidth()
{
	return screenWidth;
}

int DisplayHeight()
{
	return screenHeight;
}

void GLError(char *file, int line, char *func)
{
	int i = glGetError();
	if(i) Log("OpenGL (gl%s) Error in file %s line %i: %s\n", func, file, line, gluErrorString(i));
}

int LoadFont(void)
{
	int x, y;
	fontTex = LoadTexture("Font.png");
	if(!fontTex)
	{
		Log("Error loading Font.png!\n");
		return 0;
	}
	// i think this makes the newline portable :)
	sprintf(&newline, "\n");
	sprintf(&space, " ");
	numChars = space - newline + FONT_SIZE;
	fontList = GLGenLists(numChars);
	GLBindTexture(GL_TEXTURE_2D, fontTex);

	glNewList(fontList, GL_COMPILE); // newline character
	{
		glPopMatrix(); // matrix is saved before glCallLists in PrintGL
		glTranslated(0.0, FONT_HEIGHT, 0.0);
		glPushMatrix();
	} glEndList();
	glNewList(fontList+space-newline, GL_COMPILE); // space character
	{
		glTranslated(FONT_WIDTH, 0.0, 0.0);
	} glEndList();
	// all lists between 10 and 32 are empty, they don't do anything

	for(y=0;y<6;y++)
	for(x=0;x<16;x++)
	{
		if(!x && !y) continue; // skip space (already handled above)
		glNewList(fontList+space-newline+x+(y<<4), GL_COMPILE);
		{
			glBegin(GL_QUADS);
			{
				glTexCoord2f(	((double)x*16.0)/256.0,
						((double)y*16.0)/256.0);
						glVertex2i(0, 0);
				glTexCoord2f(	((double)x*16.0 + 16.0)/256.0,
						((double)y*16.0)/256.0);
						glVertex2i(16, 0);
				glTexCoord2f(	((double)x*16.0 + 16.0)/256.0,
						((double)y*16.0 + 16.0)/256.0);
						glVertex2i(16, 16);
				glTexCoord2f(	((double)x*16.0)/256.0,
						((double)y*16.0 + 16.0)/256.0);
						glVertex2i(0, 16);
			} glEnd();
			glTranslated(FONT_WIDTH, 0, 0);
		} glEndList();
	}
	return 1;
}

SDL_Surface *InitGL()
{
	Uint32 flags = SDL_OPENGL;
	SDL_Surface *screen;
	float ambientLight[4] = {0.7, 0.7, 0.7, 0.7};
	float diffuseLight[4] = {1.0, 1.0, 1.0, 1.0};
	float light0[4] = {0.0, 1.0, 0.0, 0.0};
	float light0amb[4] = {0.3, 0.3, 0.3, 1.0};
	float light0dif[4] = {0.4, 0.4, 0.4, 1.0};

	printf("Starting video...\n");
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_TIMER) < 0)
	{
		SDLError("initializing");
		return NULL;
	}
	sdlInited = 1;
	Log("SDL Initialized\n");
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
			Log("Warning: Bpp not 0/16/24/32, this may not work\n");
	}
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	if(CfgEq("video.fullscreen", "yes")) flags |= SDL_FULLSCREEN;
	screen = SDL_SetVideoMode(CfgI("video.width"), CfgI("video.height"), CfgI("video.bpp"), flags);
	if(screen == NULL)
	{
		SDLError("setting video mode");
		return NULL;
	}
	Log("Video mode set: (%i, %i) ", CfgI("video.width"), CfgI("video.height"));
	if(CfgI("video.bpp")) printf("at %ibpp\n", CfgI("video.bpp"));
	else printf("at default bpp\n");
	SDL_WM_SetCaption("Gmae", NULL); // second arg is icon
	InitFPS();
	GLViewport(0, 0, screenWidth, screenHeight);
	GLEnable(GL_TEXTURE_2D);
	GLClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	GLClearDepth(1.0);
	GLDepthFunc(GL_LEQUAL);
	GLEnable(GL_DEPTH_TEST);
	GLShadeModel(GL_SMOOTH);

	GLEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GLMatrixMode(GL_PROJECTION);
	GLLoadIdentity();

	gluPerspective(45.0f,(GLfloat)screenWidth/(GLfloat)screenHeight,0.1f,100.0f);
	GLMatrixMode(GL_MODELVIEW);
	GLLoadIdentity();

	GLEnable(GL_LIGHTING);
	GLLightfv(GL_LIGHT0, GL_POSITION, light0);
	GLLightfv(GL_LIGHT0, GL_AMBIENT, light0amb);
	GLLightfv(GL_LIGHT0, GL_DIFFUSE, light0dif);
	GLEnable(GL_LIGHT0);
	GLLightfv(GL_LIGHT1, GL_AMBIENT, ambientLight);
	GLLightfv(GL_LIGHT1, GL_DIFFUSE, diffuseLight);
	GLEnable(GL_LIGHT1);

	pbufsize = 128;
	pbuf = (char*)malloc(sizeof(char)*pbufsize);
	if(!LoadFont())
	{
		free(pbuf);
		return NULL;
	}
	fontInited = 1;

	return screen;
}

void QuitGL()
{
	if(!sdlInited) return;
	SDL_Quit();
	if(!fontInited) return;
	free(pbuf);
	GLDeleteLists(fontList, numChars);
	GLDeleteTextures(1, &fontTex);
	printf("OpenGL shutdown\n");
}

void SetOrthoProjection()
{
	GLMatrixMode(GL_PROJECTION);
	GLPushMatrix(); // popped in ResetProjection()
	GLLoadIdentity();
	gluOrtho2D(0, screenWidth, 0, screenHeight);
	GLScalef(1.0, -1.0, 1.0);
	GLTranslatef(0.0, (float)-screenHeight, 0.0);
	GLMatrixMode(GL_MODELVIEW);
	GLPushMatrix(); // popped in ResetProjection()
	GLLoadIdentity();
	GLDisable(GL_LIGHTING);
}

void ResetProjection()
{
	GLMatrixMode(GL_PROJECTION);
	GLPopMatrix();
	GLMatrixMode(GL_MODELVIEW);
	GLPopMatrix();
	GLEnable(GL_LIGHTING);
}

void UpdateScreen()
{
	PrintFPS();
	SDL_GL_SwapBuffers();
	UpdateFPS();
	GLClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void SetFontSize(float size)
{
	fontSize = size;
}

void PrintGL(int x, int y, char *msg, ...)
{
	va_list ap;
	int len;

	if(!pbufsize)
	{
		pbufsize = 128;
		pbuf = (char*)malloc(sizeof(char)*pbufsize);
	}
	va_start(ap, msg);
	while((len = vsnprintf(pbuf, pbufsize, msg, ap)) >= pbufsize)
	{
		va_end(ap);
		va_start(ap, msg);
		pbufsize *= 2;
		Log("Warning: Doubling print buffer size: %i bytes\n", pbufsize);
		pbuf = (char*)realloc(pbuf, sizeof(char*)*pbufsize);
	}
	va_end(ap);

	//fps stuff from
	//http://www.geocities.com/SiliconValley/Hills/6287/fps/
	// also from NeHe tutorial
	GLBindTexture(GL_TEXTURE_2D, fontTex);
	SetOrthoProjection();
	GLPushMatrix();
	GLTranslated((double)x, (double)y, 0);
	GLScalef(fontSize, fontSize, 1.0);
	GLPushAttrib(GL_LIST_BIT);
	GLListBase(fontList-newline);
	GLPushMatrix(); // save position for newline characters
	GLCallLists(len, GL_UNSIGNED_BYTE, pbuf);
	GLPopMatrix();
	GLPopAttrib();
	GLPopMatrix();
	ResetProjection();
}
