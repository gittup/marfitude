/*
   Marfitude
   Copyright (C) 2004 Mike Shal

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
#include <string.h>
#include <dirent.h>

#include "SDL.h"
#include "SDL_image.h"

#include "textures.h"
#include "glfunc.h"
#include "log.h"

#include "util/memtest.h"
#include "util/textprogress.h"
#include "util/fatalerror.h"
#include "util/sdlfatalerror.h"
#include "util/strfunc.h"

#define TEXDIR "images/"

struct tex_entry *textures;
int texInited = 0;
int num_textures = 0;

static int ValidPngFile(const char *s);

int ValidPngFile(const char *s)
{
	int x = 0;
	while(s[x]) x++;
	if(x < 4) return 0;
	if(s[x-1] != 'g') return 0;
	if(s[x-2] != 'n') return 0;
	if(s[x-3] != 'p') return 0;
	if(s[x-4] != '.') return 0;
	return 1;
}

GLuint LoadTexture(const char *filename)
{
	int format;
	GLuint tex;
	SDL_Surface *s;

	s = IMG_Load(filename);
	if(!s)
	{
		SDLError("opening texture");
		return 0;
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

GLuint TextureNum(const char *name)
{
	int x;

	if(name == NULL) return -1;

	for(x=0;x<num_textures;x++) {
		if(StrEq(name, textures[x].name))
			return textures[x].texture;
	}
	fprintf(stderr, "Warning: Texture '%s' not found.\n", name);
	return -1;
}

int InitTextures(void)
{
	int x;
	int format;
	DIR *dir;
	struct dirent *d;
	char *t;
	SDL_Surface *s;

	dir = opendir(TEXDIR);
	if(dir == NULL) {
		Error("Loading textures");
		return 1;
	}

	while((d = readdir(dir)) != NULL) {
		if(ValidPngFile(d->d_name)) {
			textures = (struct tex_entry*)realloc(textures, sizeof(struct tex_entry) * (num_textures+1));
			textures[num_textures].name = malloc(strlen(d->d_name) + 1);
			strcpy(textures[num_textures].name, d->d_name);
			glGenTextures(1, &textures[num_textures].texture);
			num_textures++;
		}
	}

	/* This just means the memory is allocated */
	texInited = 1;

	ProgressMeter("Loading textures");
	for(x=0;x<num_textures;x++) {
		t = CatStr(TEXDIR, textures[x].name);
		s = IMG_Load(t);
		if(!s) {
			SDLError("opening texture");
			ELog(("\nERROR: Couldn't load texture '%s': %s\n", t, IMG_GetError()));
			free(t);
			return 2;
		}
		free(t);
		glBindTexture(GL_TEXTURE_2D, textures[x].texture);
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
				return 3;
		}
		glTexImage2D(GL_TEXTURE_2D, 0, s->format->BytesPerPixel, s->w, s->h, 0, format, GL_UNSIGNED_BYTE, s->pixels);

		SDL_FreeSurface(s);
		UpdateProgress(x+1, num_textures);
	}
	EndProgressMeter();
	return 0;
}

void QuitTextures(void)
{
	int x;
	if(!texInited) return;
	for(x=0;x<num_textures;x++) {
		free(textures[x].name);
		glDeleteTextures(1, &textures[x].texture);
	}
	free(textures);
	printf("Textures shutdown\n");
	return;
}
