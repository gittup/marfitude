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

#include "matrix.h"
#include <stdio.h>

#define div(m, f) \
	m->v[0] /= f;\
	m->v[1] /= f;\
	m->v[2] /= f;\
	m->v[4] /= f;\
	m->v[5] /= f;\
	m->v[6] /= f;\
	m->v[8] /= f;\
	m->v[9] /= f;\
	m->v[10] /= f;

#define det3x3(m) \
	m->v[0] * (m->v[5] * m->v[10] - m->v[6] * m->v[9]) - \
	m->v[1] * (m->v[4] * m->v[10] - m->v[6] * m->v[8]) + \
	m->v[2] * (m->v[4] * m->v[9] - m->v[5] * m->v[8]);

#define det2x2_00(m) (m->v[5] * m->v[10] - m->v[6] * m->v[9])
#define det2x2_01(m) (m->v[4] * m->v[10] - m->v[6] * m->v[8])
#define det2x2_02(m) (m->v[4] * m->v[9] - m->v[5] * m->v[8])
#define det2x2_10(m) (m->v[1] * m->v[10] - m->v[2] * m->v[9])
#define det2x2_11(m) (m->v[0] * m->v[10] - m->v[2] * m->v[8])
#define det2x2_12(m) (m->v[0] * m->v[9] - m->v[1] * m->v[8])
#define det2x2_20(m) (m->v[1] * m->v[6] - m->v[2] * m->v[5])
#define det2x2_21(m) (m->v[0] * m->v[6] - m->v[2] * m->v[4])
#define det2x2_22(m) (m->v[0] * m->v[5] - m->v[1] * m->v[4])

/*
 * 0  1  2  3
 * 4  5  6  7
 * 8  9  10 11
 * 12 13 14 15
 */

/** Invert the upper-left 3x3 matrix in the 4x4 matrix @a src. The result is
 * stored in @a dest. The rest of the 4x4 matrix in @a dest is the identity.
 */
void matrix_3x3inv(union matrix *dest, const union matrix *src)
{
	dest->v[3] = 0.0;
	dest->v[7] = 0.0;
	dest->v[11] = 0.0;
	dest->v[12] = 0.0;
	dest->v[13] = 0.0;
	dest->v[14] = 0.0;
	dest->v[15] = 1.0;

	dest->v[0]  =  det2x2_00(src);
	dest->v[1]  = -det2x2_10(src);
	dest->v[2]  =  det2x2_20(src);
	dest->v[4]  = -det2x2_01(src);
	dest->v[5]  =  det2x2_11(src);
	dest->v[6]  = -det2x2_21(src);
	dest->v[8]  =  det2x2_02(src);
	dest->v[9]  = -det2x2_12(src);
	dest->v[10] =  det2x2_22(src);

	div(dest, 1 / det3x3(src));
}

/** Display the matrix m in OpenGL format */
void matrix_print(union matrix *m)
{
	printf("%f\t%f\t%f\t%f\n", m->v[0], m->v[4], m->v[8], m->v[12]);
	printf("%f\t%f\t%f\t%f\n", m->v[1], m->v[5], m->v[9], m->v[13]);
	printf("%f\t%f\t%f\t%f\n", m->v[2], m->v[6], m->v[10], m->v[14]);
	printf("%f\t%f\t%f\t%f\n", m->v[3], m->v[7], m->v[11], m->v[15]);
}

/** Multiply 4x4 matricies @a dest x @a src, and store the result in @a dest. */
void matrix_mul(union matrix *dest, const union matrix *src)
{
	union matrix tmp;

	tmp.v[0] =
		dest->v[0] * src->v[0] + dest->v[1] * src->v[4] +
		dest->v[2] * src->v[8] + dest->v[3] * src->v[12];
	tmp.v[1] =
		dest->v[0] * src->v[1] + dest->v[1] * src->v[5] +
		dest->v[2] * src->v[9] + dest->v[3] * src->v[13];
	tmp.v[2] =
		dest->v[0] * src->v[2] + dest->v[1] * src->v[6] +
		dest->v[2] * src->v[10] + dest->v[3] * src->v[14];
	tmp.v[3] =
		dest->v[0] * src->v[3] + dest->v[1] * src->v[7] +
		dest->v[2] * src->v[11] + dest->v[3] * src->v[15];
	tmp.v[4] =
		dest->v[4] * src->v[0] + dest->v[5] * src->v[4] +
		dest->v[6] * src->v[8] + dest->v[7] * src->v[12];
	tmp.v[5] =
		dest->v[4] * src->v[1] + dest->v[5] * src->v[5] +
		dest->v[6] * src->v[9] + dest->v[7] * src->v[13];
	tmp.v[6] =
		dest->v[4] * src->v[2] + dest->v[5] * src->v[6] +
		dest->v[6] * src->v[10] + dest->v[7] * src->v[14];
	tmp.v[7] =
		dest->v[4] * src->v[3] + dest->v[5] * src->v[7] +
		dest->v[6] * src->v[11] + dest->v[7] * src->v[15];
	tmp.v[8] =
		dest->v[8] * src->v[0] + dest->v[9] * src->v[4] +
		dest->v[10] * src->v[8] + dest->v[11] * src->v[12];
	tmp.v[9] =
		dest->v[8] * src->v[1] + dest->v[9] * src->v[5] +
		dest->v[10] * src->v[9] + dest->v[11] * src->v[13];
	tmp.v[10] =
		dest->v[8] * src->v[2] + dest->v[9] * src->v[6] +
		dest->v[10] * src->v[10] + dest->v[11] * src->v[14];
	tmp.v[11] =
		dest->v[8] * src->v[3] + dest->v[9] * src->v[7] +
		dest->v[10] * src->v[11] + dest->v[11] * src->v[15];
	tmp.v[12] =
		dest->v[12] * src->v[0] + dest->v[13] * src->v[4] +
		dest->v[14] * src->v[8] + dest->v[15] * src->v[12];
	tmp.v[13] =
		dest->v[12] * src->v[1] + dest->v[13] * src->v[5] +
		dest->v[14] * src->v[9] + dest->v[15] * src->v[13];
	tmp.v[14] =
		dest->v[12] * src->v[2] + dest->v[13] * src->v[6] +
		dest->v[14] * src->v[10] + dest->v[15] * src->v[14];
	tmp.v[15] =
		dest->v[12] * src->v[3] + dest->v[13] * src->v[7] +
		dest->v[14] * src->v[11] + dest->v[15] * src->v[15];

	matrix_copy(dest, &tmp);
}

/** Copy the matrix @a src into @a dest */
void matrix_copy(union matrix *dest, const union matrix *src)
{
	int x;
	for(x=0; x<16; x++) {
		dest->v[x] = src->v[x];
	}
}

/** Transpose the @a src matrix into @a dest */
void matrix_transpose(union matrix *dest, const union matrix *src)
{
	dest->v[0] = src->v[0];
	dest->v[1] = src->v[4];
	dest->v[2] = src->v[8];
	dest->v[3] = src->v[12];
	dest->v[4] = src->v[1];
	dest->v[5] = src->v[5];
	dest->v[6] = src->v[9];
	dest->v[7] = src->v[13];
	dest->v[8] = src->v[2];
	dest->v[9] = src->v[6];
	dest->v[10] = src->v[10];
	dest->v[11] = src->v[14];
	dest->v[12] = src->v[3];
	dest->v[13] = src->v[7];
	dest->v[14] = src->v[11];
	dest->v[15] = src->v[15];
}
