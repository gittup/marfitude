#include <stdio.h>
#include <stdlib.h>

#include "SDL.h"
#include "SDL_image.h"

#include "textures.h"
#include "texlist.h"
#include "glfunc.h"
#include "log.h"
#include "../util/memtest.h"
#include "../util/textprogress.h"
#include "../util/fatalerror.h"
#include "../util/sdlfatalerror.h"

GLuint *GLTexture;
int texInited = 0;

GLuint LoadTexture(char *filename)
{
	int format;
	GLuint tex;
	SDL_Surface *s;

	s = IMG_Load(filename);
	if(!s)
	{
		SDLError("opening texture");
	}
	GLGenTextures(1, &tex);
	GLBindTexture(GL_TEXTURE_2D, tex);
	GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	switch(s->format->BytesPerPixel)
	{
		case 3:
			format = GL_RGB;
			break;
		case 4:
			format = GL_RGBA;
			break;
		default:
			Log("\nERROR: Incorrect image format in %s - image must be RGB or RGBA\n", filename);
			return 0;
	}
	GLTexImage2D(GL_TEXTURE_2D, 0, s->format->BytesPerPixel, s->w, s->h, 0, format, GL_UNSIGNED_BYTE, s->pixels);

	SDL_FreeSurface(s);
	return tex;
}

int InitTextures(void)
{
	int x;
	int format;
	SDL_Surface *s;

	GLTexture = (GLuint*)malloc(sizeof(GLuint)*NUM_TEXTURES);
	GLGenTextures(NUM_TEXTURES, GLTexture);
	texInited = 1;

	ProgressMeter("Loading textures");
	for(x=0;x<NUM_TEXTURES;x++)
	{
		s = IMG_Load(TEX_LIST[x]);
		if(!s)
		{
			Log("\nERROR: Couldn't load texture '%s': %s\n", TEX_LIST[x], IMG_GetError());
			return 0;
		}
		GLBindTexture(GL_TEXTURE_2D, GLTexture[x]);
		GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		GLTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		switch(s->format->BytesPerPixel)
		{
			case 3:
				format = GL_RGB;
				break;
			case 4:
				format = GL_RGBA;
				break;
			default:
				Log("\nERROR: Incorrect image format in tex.dat - image must be RGB or RGBA\n");
				return 0;
		}
		GLTexImage2D(GL_TEXTURE_2D, 0, s->format->BytesPerPixel, s->w, s->h, 0, format, GL_UNSIGNED_BYTE, s->pixels);

		SDL_FreeSurface(s);
		UpdateProgress(x+1, NUM_TEXTURES);
	}
	EndProgressMeter();
	return 1;
}

void QuitTextures(void)
{
	if(!texInited) return;
	GLDeleteTextures(NUM_TEXTURES, GLTexture);
	free(GLTexture);
	GLTexture = NULL;
	printf("Textures shutdown\n");
	return;
}
