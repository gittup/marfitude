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

/* Because all awesome programs need an fft.c file */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "fft.h"
#include "cmp.h"
#include "sdl_mixer/mikmod/sample_callback.h"
#include "sdl_mixer/mikmod/mikmod.h"

static struct fft_data my_fft;
static int num_registers = 0;
static int *data = NULL;
static unsigned long samples = 0;

/** The fft_data that a client can access. */
const struct fft_data *fft = &my_fft;

static struct cmp *weights = NULL;
static double *window = NULL;
static unsigned int *fft_bit_map = NULL;
static unsigned int bit_reverse(unsigned int a);
static void fft_handler(signed char *stream, int len, int flags);
static void fft_process(struct cmp *f);
static void internal_init(int len, int flags);
static struct cmp *fft_buf = NULL;
static int my_log(int x);

/** Initialize the FFT engine. This function registers with MikMod to receive
 * samples and process them.
 */
void init_fft(void)
{
	if(!num_registers)
		register_sample_callback(fft_handler);
	num_registers++;
}

/** Stops the FFT engine. Deregisters with MikMod and frees all memory used */
void free_fft(void)
{
	num_registers--;
	if(!num_registers) {
		deregister_sample_callback();

		my_fft.len = 0;
		my_fft.log_len = 0;
		free(data);
		data = NULL;
		my_fft.data = NULL;

		free(weights);
		weights = NULL;

		free(window);
		window = NULL;

		free(fft_bit_map);
		fft_bit_map = NULL;

		free(fft_buf);
		fft_buf = NULL;
	}
}

/** Displays number of FFT samples processed. The only real reason this exists
 * is to make sure the FFT engine gets pulled into the executable so plugins
 * can use it. This was done instead of doing a whole-archive.
 */
void QuitFFT(void)
{
	if(samples) {
		/* 32 bits = 4294967295 samples before overflow. Sampled at
		 * 44100Hz, this is 2^32 / 44100 ~= 97392 seconds, or ~27
		 * hours of samples that can be counted. Of course, given
		 * the total awesomeness of MODs, this is obviously not
		 * enough. But I'll just let it wrap around.
		 */
		printf("Processed %lu FFT samples\n", samples);
	}
}

int my_log(int x)
{
	int y;
	int z;

	y = 0;
	z = 1;
	while(!(x&z))
	{
		y++;
		z <<= 1;
		if(y > 30) return 0;
	}
	return y;
}

void internal_init(int len, int flags)
{
	int i;
	int x;
	int j;
	int log_len;
	double pi;

	if(flags & DMODE_16BITS)
		len /= 2;
	if(flags & DMODE_STEREO)
		len /= 2;

	log_len = my_log(len);

	data = (int*)malloc(sizeof(int) * len);
	my_fft.data = data;
	fft_buf = (struct cmp*)malloc(sizeof(struct cmp) * len);

	for(x=0;x<len;x++) {
		data[x] = 0;
	}
	my_fft.cur_max = 0;
	my_fft.hist_max = 0;
	my_fft.max = pow(2.0, (flags & DMODE_16BITS) ? 16 : 8) * len;
	/* Does mono/stereo affect my_fft.max? Probably, but...by how MUCH!? */

	pi = 4.0 * atan(1.0);
	weights = (struct cmp*)malloc(sizeof(struct cmp) * len * log_len);
	x = 2;
	for(j=0;j<log_len;j++)
	{
		for(i=0;i<len;i++)
		{
			weights[i+len*j].real =  cos(2.0 * pi * i / x);
			weights[i+len*j].imag = -sin(2.0 * pi * i / x);
		}
		x *= 2;
	}

	window = (double*)malloc(sizeof(double) * len);
	for(i=0;i<len;i++)
	{
		window[i] = 0.5 - 0.5*cos(2*pi*i / (len-1));
	}

	fft_bit_map = (unsigned int*)malloc(sizeof(unsigned int) * len);
	for(i=0;i<len;i++)
	{
		fft_bit_map[i] = bit_reverse(i) >> (32-log_len);
	}
	my_fft.log_len = log_len;
	my_fft.len = len;
}

void fft_handler(signed char *stream, int len, int flags)
{
	unsigned int i;
	unsigned int j;

	if(len && my_fft.len == 0)
		internal_init(len, flags);

	samples += len;

	for(i=0;i<my_fft.len;i++)
	{
		j = fft_bit_map[i];
		if(flags & DMODE_16BITS)
		{
			if(flags & DMODE_STEREO)
			{
				fft_buf[i].real = window[j] * (double)(stream[4*j] + stream[4*j+2] + 256*(stream[4*j+1] + stream[4*j+3]));
			}
			else
			{
				fft_buf[i].real = window[j] * (double)(stream[2*j] + 256*stream[2*j+1]);
			}
		}
		else
		{
			unsigned char *ustream = (unsigned char *)stream;
			if(flags & DMODE_STEREO)
			{
				fft_buf[i].real = window[j] * (double)(((signed)(ustream[2*j] + ustream[2*j+1])) - 256);
			}
			else
			{
				fft_buf[i].real = window[j] * (double)((signed)ustream[j] - 128);
			}
		}
		fft_buf[i].imag = 0;
	}

	fft_process(fft_buf);

	my_fft.cur_max = 0;
	for(i=0;i<my_fft.len;i++)
	{
		data[i] = mag(&fft_buf[i]);
		if(data[i] > my_fft.cur_max)
			my_fft.cur_max = data[i];
	}
	if(my_fft.cur_max > my_fft.hist_max)
		my_fft.hist_max = my_fft.cur_max;
}

void fft_process(struct cmp *x)
{
	unsigned int level;
	unsigned int step;

	step = 1;
	for(level=0; level<my_fft.log_len; level++)
	{
		unsigned int increm = step * 2;
		unsigned int j;

		for(j=0; j<step; j++)
		{
			unsigned int i;
			struct cmp u;

			u.real = weights[level*my_fft.len + j].real;
			u.imag = weights[level*my_fft.len + j].imag;

			for(i=j; i<my_fft.len; i+=increm)
			{
				struct cmp t;
				struct cmp t2;

				t.real = u.real;
				t.imag = u.imag;

				mul(&t2, &t, x+i+step);

				x[i+step].real = x[i].real;
				x[i+step].imag = x[i].imag;
				x[i+step].real -= t2.real;
				x[i+step].imag -= t2.imag;
				x[i].real += t2.real;
				x[i].imag += t2.imag;
			}
		}
		step *= 2;
	}
}

/* bit reversing is a blatant rip from somewhere, i think it was:
 * http://graphics.stanford.edu/~seander/bithacks.html
 */
static const unsigned char brt[] = {
	0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0,
	0x30, 0xB0, 0x70, 0xF0, 0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8,
	0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8, 0x04, 0x84, 0x44, 0xC4,
	0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
	0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC,
	0x3C, 0xBC, 0x7C, 0xFC, 0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2,
	0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2, 0x0A, 0x8A, 0x4A, 0xCA,
	0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
	0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6,
	0x36, 0xB6, 0x76, 0xF6, 0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE,
	0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE, 0x01, 0x81, 0x41, 0xC1,
	0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
	0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9,
	0x39, 0xB9, 0x79, 0xF9, 0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5,
	0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5, 0x0D, 0x8D, 0x4D, 0xCD,
	0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
	0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3,
	0x33, 0xB3, 0x73, 0xF3, 0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB,
	0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB, 0x07, 0x87, 0x47, 0xC7,
	0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
	0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF,
	0x3F, 0xBF, 0x7F, 0xFF
};

unsigned int bit_reverse(unsigned int a)
{
	unsigned int r;
	unsigned char * p = (unsigned char *) &a;
	unsigned char * q = (unsigned char *) &r;
	q[3] = brt[p[0]]; 
	q[2] = brt[p[1]]; 
	q[1] = brt[p[2]]; 
	q[0] = brt[p[3]];
	return r;
}
