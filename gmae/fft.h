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

struct fft_data {
	int *data;            /* len worth of data points */
	unsigned int len;     /* length of above data, 0 if fft is not inited */
	unsigned int log_len; /* log2(length) */
	int cur_max;          /* The max data[i] for the current sample set */
	int hist_max;         /* The max data[i] since last init_fft() */
	int max;              /* The theoretical max, based on sample types */
};
extern struct fft_data fft;
void init_fft(void);
void free_fft(void);
