/*
 * TGA surface saving code for the SDL library
 *
 * Mattias Engdeg�rd
 *
 * Use, modification and distribution of this source is allowed without
 * limitation, warranty or liability of any kind.
 */


#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#include "savetga.h"

struct TGAheader {
    Uint8 infolen;		/* length of info field */
    Uint8 has_cmap;		/* 1 if image has colormap, 0 otherwise */
    Uint8 type;

    Uint8 cmap_start[2];	/* index of first colormap entry */
    Uint8 cmap_len[2];		/* number of entries in colormap */
    Uint8 cmap_bits;		/* bits per colormap entry */

    Uint8 yorigin[2];		/* image origin (ignored here) */
    Uint8 xorigin[2];
    Uint8 width[2];		/* image size */
    Uint8 height[2];
    Uint8 pixel_bits;		/* bits/pixel */
    Uint8 flags;
};

enum tga_type {
    TGA_TYPE_INDEXED = 1,
    TGA_TYPE_RGB = 2,
    TGA_TYPE_BW = 3,
    TGA_TYPE_RLE = 8		/* additive */
};

#define TGA_INTERLEAVE_MASK	0xc0
#define TGA_INTERLEAVE_NONE	0x00
#define TGA_INTERLEAVE_2WAY	0x40
#define TGA_INTERLEAVE_4WAY	0x80

#define TGA_ORIGIN_MASK		0x30
#define TGA_ORIGIN_LEFT		0x00
#define TGA_ORIGIN_RIGHT	0x10
#define TGA_ORIGIN_LOWER	0x00
#define TGA_ORIGIN_UPPER	0x20

/* read/write unaligned little-endian 16-bit ints */
#define LE16(p) ((p)[0] + ((p)[1] << 8))
#define SETLE16(p, v) ((p)[0] = (v), (p)[1] = (v) >> 8)

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#define TGA_RLE_MAX 128		/* max length of a TGA RLE chunk */

/* return the number of bytes in the resulting buffer after RLE-encoding
   a line of TGA data */
static int rle_line(Uint8 *src, Uint8 *dst, int w, int bpp)
{
    int x = 0;
    int out = 0;
    int raw = 0;
    while(x < w) {
	Uint32 pix;
	int x0 = x;
	memcpy(&pix, src + x * bpp, bpp);
	x++;
	while(x < w && memcmp(&pix, src + x * bpp, bpp) == 0
	      && x - x0 < TGA_RLE_MAX)
	    x++;
	/* use a repetition chunk iff the repeated pixels would consume
	   two bytes or more */
	if((x - x0 - 1) * bpp >= 2 || x == w) {
	    /* output previous raw chunks */
	    while(raw < x0) {
		int n = MIN(TGA_RLE_MAX, x0 - raw);
		dst[out++] = n - 1;
		memcpy(dst + out, src + raw * bpp, n * bpp);
		out += n * bpp;
		raw += n;
	    }

	    if(x - x0 > 0) {
		/* output new repetition chunk */
		dst[out++] = 0x7f + x - x0;
		memcpy(dst + out, &pix, bpp);
		out += bpp;
	    }
	    raw = x;
	}
    }
    return out;
}

/*
 * Save a surface to an output stream in TGA format.
 * 8bpp surfaces are saved as indexed images with 24bpp palette, or with
 *     32bpp palette if colourkeying is used.
 * 15, 16, 24 and 32bpp surfaces are saved as 24bpp RGB images,
 * or as 32bpp RGBA images if alpha channel is used.
 *
 * RLE compression is not used in the output file.
 *
 * Returns -1 upon error, 0 if success
 */
int SaveTGA_RW(SDL_Surface *surface, SDL_RWops *out, int rle)
{
    SDL_Surface *linebuf = NULL;
    int alpha = 0;
    int ckey = -1;
    struct TGAheader h;
    int srcbpp;
    unsigned surf_flags;
    unsigned surf_alpha;
    Uint32 rmask, gmask, bmask, amask;
    SDL_Rect r;
    int bpp;
    Uint8 *rlebuf = NULL;

    h.infolen = 0;
    SETLE16(h.cmap_start, 0);

    srcbpp = surface->format->BitsPerPixel;
    if(srcbpp < 8) {
	SDL_SetError("cannot save <8bpp images as TGA");
	return -1;
    }

    if(srcbpp == 8) {
	h.has_cmap = 1;
	h.type = TGA_TYPE_INDEXED;
	if(surface->flags & SDL_SRCCOLORKEY) {
	    ckey = surface->format->colorkey;
	    h.cmap_bits = 32;
	} else
	    h.cmap_bits = 24;
	SETLE16(h.cmap_len, surface->format->palette->ncolors);
	h.pixel_bits = 8;
	rmask = gmask = bmask = amask = 0;
    } else {
	h.has_cmap = 0;
	h.type = TGA_TYPE_RGB;
	h.cmap_bits = 0;
	SETLE16(h.cmap_len, 0);
	if(surface->format->Amask) {
	    alpha = 1;
	    h.pixel_bits = 32;
	} else
	    h.pixel_bits = 24;
	if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
	    int s = alpha ? 0 : 8;
	    amask = 0x000000ff >> s;
	    rmask = 0x0000ff00 >> s;
	    gmask = 0x00ff0000 >> s;
	    bmask = 0xff000000 >> s;
	} else {
	    amask = alpha ? 0xff000000 : 0;
	    rmask = 0x00ff0000;
	    gmask = 0x0000ff00;
	    bmask = 0x000000ff;
	}
    }
    bpp = h.pixel_bits >> 3;
    if(rle)
	    h.type += TGA_TYPE_RLE;

    SETLE16(h.yorigin, 0);
    SETLE16(h.xorigin, 0);
    SETLE16(h.width, surface->w);
    SETLE16(h.height, surface->h);
    h.flags = TGA_ORIGIN_UPPER | (alpha ? 8 : 0);

    if(!SDL_RWwrite(out, &h, sizeof(h), 1))
	return -1;

    if(h.has_cmap) {
	int i;
	SDL_Palette *pal = surface->format->palette;
	Uint8 entry[4];
	for(i = 0; i < pal->ncolors; i++) {
	    entry[0] = pal->colors[i].b;
	    entry[1] = pal->colors[i].g;
	    entry[2] = pal->colors[i].r;
	    entry[3] = (i == ckey) ? 0 : 0xff;
	    if(!SDL_RWwrite(out, entry, h.cmap_bits >> 3, 1))
		return -1;
	}
    }

    linebuf = SDL_CreateRGBSurface(SDL_SWSURFACE, surface->w, 1, h.pixel_bits,
				   rmask, gmask, bmask, amask);
    if(!linebuf)
	return -1;
    if(h.has_cmap)
	SDL_SetColors(linebuf, surface->format->palette->colors, 0,
		      surface->format->palette->ncolors);
    if(rle) {
	rlebuf = malloc(bpp * surface->w + 1 + surface->w / TGA_RLE_MAX);
	if(!rlebuf) {
	    SDL_SetError("out of memory");
	    goto error;
	}
    }

    /* Temporarily remove colourkey and alpha from surface so copies are
       opaque */
    surf_flags = surface->flags & (SDL_SRCALPHA | SDL_SRCCOLORKEY);
    surf_alpha = surface->format->alpha;
    if(surf_flags & SDL_SRCALPHA)
	SDL_SetAlpha(surface, 0, 255);
    if(surf_flags & SDL_SRCCOLORKEY)
	SDL_SetColorKey(surface, 0, surface->format->colorkey);

    r.x = 0;
    r.w = surface->w;
    r.h = 1;
    for(r.y = 0; r.y < surface->h; r.y++) {
	int n;
	void *buf;
	if(SDL_BlitSurface(surface, &r, linebuf, NULL) < 0)
	    break;
	if(rle) {
	    buf = rlebuf;
	    n = rle_line(linebuf->pixels, rlebuf, surface->w, bpp);
	} else {
	    buf = linebuf->pixels;
	    n = surface->w * bpp;
	}
	if(!SDL_RWwrite(out, buf, n, 1))
	    break;
    }

    /* restore flags */
    if(surf_flags & SDL_SRCALPHA)
	SDL_SetAlpha(surface, SDL_SRCALPHA, surf_alpha);
    if(surf_flags & SDL_SRCCOLORKEY)
	SDL_SetColorKey(surface, SDL_SRCCOLORKEY, surface->format->colorkey);

error:
    free(rlebuf);
    SDL_FreeSurface(linebuf);
    return 0;
}

int SaveTGA(SDL_Surface *surface, char *file, int rle)
{
    SDL_RWops *out = SDL_RWFromFile(file, "wb");
    int ret;
    if(!out)
	return -1;
    ret = SaveTGA_RW(surface, out, rle);
    SDL_RWclose(out);
    return ret;
}
