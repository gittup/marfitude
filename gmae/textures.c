/*
   Marfitude
   Copyright (C) 2006 Mike Shal

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
#include "event.h"
#include "glfunc.h"
#include "log.h"
#include "sdlfatalerror.h"

#include "util/memtest.h"
#include "util/textprogress.h"
#include "util/flist.h"
#include "util/slist.h"
#include "util/strfunc.h"

/** The directory where .png files are located */
#define TEXDIR "images/"

/** @file
 * Loads and provides access to all png files in the images/ directory
 */

/** Maps a filename to an OpenGL texture */
struct png_tex_entry {
	GLuint texture; /**< The texture number assigned by glGenTextures */
	char *name;     /**< The name of the texture (set to the filename) */
};

struct tex_entry {
	int *tex;   /**< A pointer to the texture handle */
	int width;  /**< The width of the texture (power of 2!) */
	int height; /**< The height of the texture (power of 2!) */
	SDL_Surface *surface; /**< The surface that was drawn. */
	void (*draw)(unsigned char *, int, int); /**< The draw function.
						  * Arguments are the pixel
						  * to write to and the x & y
						  * coordinates.
						  */
};

static void recreate_textures(const void *);
static int valid_png_file(const char *s);
static void create_texture_internal(struct tex_entry *entry);

static struct png_tex_entry *textures = NULL;
static struct slist *texlist = NULL;
static int tex_inited = 0;
static int num_textures = 0;

int valid_png_file(const char *s)
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

/** Create an OpenGL texture with dimensions @a width x @a height. The function
 * @a draw is called with the pixel to write to for the given x and y values.
 * The pixel is 32 bits, assumed 0 is red, 1 is green, 2 is blue, and 3 is
 * alpha. The integer @a tex is filled with the OpenGL texture number
 * (for use in glBindTexture).
 *
 * When SDL is reinitialized, create_texture will be called again automatically
 * with all textures that have been previously initialized. Thus it is possible
 * that the value of @a tex will change after SDL re-initializes. Also note
 * that this means that the @a draw function can be called more than once
 * during the program execution.
 *
 * @param tex A pointer to the texture number, to be filled by this function.
 * @param width The width of the texture (must be power of 2)
 * @param height The height of the texture (must be power of 2)
 * @param draw The function to draw the texture. Takes a pixel (four
 *             unsigned characters), and x & y coordinates.
 */
void create_texture(int *tex, int width, int height, void (*draw)(unsigned char *, int, int))
{
	int x;
	int y;
	unsigned char *p;
	unsigned char *q;
	struct tex_entry *entry;

	entry = malloc(sizeof(struct tex_entry));
	entry->tex = tex;
	entry->width = width;
	entry->height = height;
	entry->draw = draw;

	entry->surface = SDL_CreateRGBSurface(0, entry->width, entry->height, 32, MASKS);
	if(!entry->surface) {
		ELog(("ERROR: Couldn't create SDL Surface!\n"));
		*(entry->tex) = 0;
		return;
	}

	q = entry->surface->pixels;
	for(y=0; y<entry->height; y++) {
		p = q;
		for(x=0; x<entry->width; x++) {
			entry->draw(p, x, y);
			p += 4;
		}
		q += entry->surface->pitch;
	}

	texlist = slist_insert(texlist, entry);
	create_texture_internal(entry);
}

/** Delete a previously created texture.
 *
 * @param tex The tex parameter that was passed in to create_texture.
 */
void delete_texture(int *tex)
{
	struct slist *t;

	slist_foreach(t, texlist) {
		struct tex_entry *entry = t->data;
		if(entry->tex == tex) {
			texlist = slist_remove(texlist, entry);
			SDL_FreeSurface(entry->surface);
			free(entry);
			break;
		}
	}
}

void create_texture_internal(struct tex_entry *entry)
{
	GLuint gtex;

	glGenTextures(1, &gtex);
	glBindTexture(GL_TEXTURE_2D, gtex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, entry->width, entry->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, entry->surface->pixels);

	*(entry->tex) = (int)gtex;
}

/** Loads the texture @a filename and returns the OpenGL texture number
 * @param filename The name of the file
 * @return The OpenGL texture number from glGenTextures. You should call
 *         glDeleteTextures on it when you're done.
 */
GLuint load_texture(const char *filename)
{
	int format;
	GLuint tex;
	SDL_Surface *s;

	s = IMG_Load(filename);
	if(!s) {
		SDLError("opening texture");
		return 0;
	}

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	switch(s->format->BytesPerPixel) {
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
GLuint texture_num(const char *name)
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
int init_textures(void)
{
	int x;
	int format;
	char *t;
	struct flist f;
	SDL_Surface *s;

	register_event("sdl re-init", recreate_textures);

	flist_foreach(&f, TEXDIR) {
		if(valid_png_file(f.filename)) {
			textures = (struct png_tex_entry*)realloc(textures, sizeof(struct png_tex_entry) * (num_textures+1));
			textures[num_textures].name = malloc(strlen(f.filename) + 1);
			strcpy(textures[num_textures].name, f.filename);
			glGenTextures(1, &textures[num_textures].texture);
			num_textures++;
		}
	}

	/* This just means the memory is allocated */
	tex_inited = 1;

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
#ifdef __APPLE__
				/* WTF Apple? */
				format = GL_BGR;
#else
				format = GL_RGB;
#endif
				break;
			case 4:
#ifdef __APPLE__
				format = GL_BGRA;
#else
				format = GL_RGBA;
#endif
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
void quit_textures(void)
{
	int x;
	if(!tex_inited) return;

	deregister_event("sdl re-init", recreate_textures);

	for(x=0;x<num_textures;x++) {
		free(textures[x].name);
		glDeleteTextures(1, &textures[x].texture);
	}
	free(textures);
	textures = NULL;
	num_textures = 0;
	tex_inited = 0;
	/* Commented out temporarily, since this gets called when the video
	 * mode changes (and the textures wouldn't have been freed then)
	if(slist_length(texlist)) {
		ELog(("\nERROR: There are still %i textures that were not deleted!\n", slist_length(texlist)));
	}
	 */
	printf("Textures shutdown\n");
	return;
}

void recreate_textures(const void *data)
{
	struct slist *t;

	if(data) {}
	slist_foreach(t, texlist) {
		create_texture_internal(t->data);
	}
}
