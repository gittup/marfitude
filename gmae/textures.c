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
#include <string.h>

#include "SDL.h"
#include "SDL_image.h"

#include "textures.h"
#include "glfunc.h"
#include "log.h"
#include "sdlfatalerror.h"

#include "util/memtest.h"
#include "util/textprogress.h"
#include "util/fatalerror.h"
#include "util/flist.h"
#include "util/strfunc.h"

/** The directory where .png files are located */
#define TEXDIR "images/"

/** @file
 * Loads and provides access to all png files in the images/ directory
 */

/** Maps a filename to an OpenGL texture */
struct tex_entry {
	GLuint texture; /**< The texture number assigned by glGenTextures */
	char *name;     /**< The name of the texture (set to the filename) */
};

static struct tex_entry *textures;
static int texInited = 0;
static int num_textures = 0;

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

/** Loads the texture @a filename and returns the OpenGL texture number
 * @param filename The name of the file
 * @return The OpenGL texture number from glGenTextures. You should call
 * glDeleteTextures on it when you're done.
 */
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

/** Get the OpenGL texture number from one of the automatically loaded
 * textures found in the images directory.
 * @param name The filename of the texture
 * @return The OpenGL texture number from glGenTextures
 */
GLuint TextureNum(const char *name)
{
	int x;

	if(name == NULL) return -1;

	for(x=0;x<num_textures;x++) {
		if(strcmp(name, textures[x].name) == 0)
			return textures[x].texture;
	}
	fprintf(stderr, "Warning: Texture '%s' not found.\n", name);
	return -1;
}

/** Loads all the textures in the images directory. */
int InitTextures(void)
{
	int x;
	int format;
	char *t;
	struct flist f;
	SDL_Surface *s;

	flist_foreach(&f, TEXDIR) {
		if(ValidPngFile(f.filename)) {
			textures = (struct tex_entry*)realloc(textures, sizeof(struct tex_entry) * (num_textures+1));
			textures[num_textures].name = malloc(strlen(f.filename) + 1);
			strcpy(textures[num_textures].name, f.filename);
			glGenTextures(1, &textures[num_textures].texture);
			num_textures++;
		}
	}

	/* This just means the memory is allocated */
	texInited = 1;

	progress_meter("Loading textures");
	for(x=0;x<num_textures;x++) {
		t = cat_str(TEXDIR, textures[x].name);
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
		switch(s->format->BytesPerPixel) {
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
		update_progress(x+1, num_textures);
	}
	end_progress_meter();
	return 0;
}

/** Frees all of the automatically loaded textures */
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
