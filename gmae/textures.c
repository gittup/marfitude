#include <stdio.h>
#include <stdlib.h>

#include "SDL.h"
#include "SDL_image.h"

#include "textures.h"
#include "texlist.h"
#include "glfunc.h"
#include "log.h"

#include "memtest.h"
#include "textprogress.h"
#include "fatalerror.h"
#include "sdlfatalerror.h"

GLuint *GLTexture;
int texInited = 0;

GLuint LoadTexture(const char *filename)
{
	int format;
	GLuint tex;
	SDL_Surface *s;

	s = IMG_Load(filename);
	if(!s)
	{
		SDLError("opening texture");
	}
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	switch(s->format->BytesPerPixel)
	{
		case 3:
			format = GL_RGB;
			break;
		case 4:
			format = GL_RGBA;
			break;
		default:
			ELog(("\nERROR: Incorrect image format in %s - image must be RGB or RGBA\n", filename));
			return 0;
	}
	glTexImage2D(GL_TEXTURE_2D, 0, s->format->BytesPerPixel, s->w, s->h, 0, format, GL_UNSIGNED_BYTE, s->pixels);

	SDL_FreeSurface(s);
	return tex;
}

int InitTextures(void)
{
	int x;
	int format;
	SDL_Surface *s;

	GLTexture = (GLuint*)malloc(sizeof(GLuint)*NUM_TEXTURES);
	glGenTextures(NUM_TEXTURES, GLTexture);
	texInited = 1;

	ProgressMeter("Loading textures");
	for(x=0;x<NUM_TEXTURES;x++)
	{
		s = IMG_Load(TEX_LIST[x]);
		if(!s)
		{
			ELog(("\nERROR: Couldn't load texture '%s': %s\n", TEX_LIST[x], IMG_GetError()));
			return 0;
		}
		glBindTexture(GL_TEXTURE_2D, GLTexture[x]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		switch(s->format->BytesPerPixel)
		{
			case 3:
				format = GL_RGB;
				break;
			case 4:
				format = GL_RGBA;
				break;
			default:
				ELog(("\nERROR: Incorrect image format in tex.dat - image must be RGB or RGBA\n"));
				return 0;
		}
		glTexImage2D(GL_TEXTURE_2D, 0, s->format->BytesPerPixel, s->w, s->h, 0, format, GL_UNSIGNED_BYTE, s->pixels);

		SDL_FreeSurface(s);
		UpdateProgress(x+1, NUM_TEXTURES);
	}
	EndProgressMeter();
	return 1;
}

void QuitTextures(void)
{
	if(!texInited) return;
	glDeleteTextures(NUM_TEXTURES, GLTexture);
	free(GLTexture);
	GLTexture = NULL;
	printf("Textures shutdown\n");
	return;
}
