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

#include "vector.h"
#include <math.h>

/** @file
 * Provides some vector math functions
 */

static double trans(double x, double y, double t, double clip);

/** Set @a c = @a v1 X @a v2
 *
 * @param c The output matrix
 * @param v1 The first input matrix
 * @param v2 The second input matrix
 */
void vector_cross(union vector *c, const union vector *v1, const union vector *v2)
{
	c->v[0] = v1->v[1]*v2->v[2] - v1->v[2]*v2->v[1];
	c->v[1] = v1->v[2]*v2->v[0] - v1->v[0]*v2->v[2];
	c->v[2] = v1->v[0]*v2->v[1] - v1->v[1]*v2->v[0];
}

/** Return the magnitude of vector @a v
 *
 * @param v The input vector
 * @return The magnitude of the vector == sqrt(x^2 + y^2 + z^2)
 */
double vector_mag(const union vector *v)
{
	return sqrt(v->v[0]*v->v[0] + v->v[1]*v->v[1] + v->v[2]*v->v[2]);
}

/** Divide each element in vector @a v by the magnitude of @a v. If the vector
 * is close to 0, no normalization is done and the function returns 1.
 * Otherwise, the vector is normalized and the function returns 0.
 *
 * @param v The vector to normalize.
 * @retval 0 The normalization worked.
 * @retval 1 The normalization did not complete successfully (vector likely
 *           too small)
 * @retval 2 The normalization did not complete successfully - for some reason
 *           the magnitude was less than 0.9 after normalization.
 */
int vector_normalize(union vector *v)
{
	double m;
	m = vector_mag(v);
	if(m <= 0.000001)
		return 1;
	v->v[0] /= m;
	v->v[1] /= m;
	v->v[2] /= m;
	if(vector_mag(v) < 0.9)
		return 2;
	return 0;
}

/** Transition the vector @a src towards the vector @a dest by a scaling factor
 * of @a t. Each component of @a is set by the following equation:
 * @code
 *  src = src + (dest - src) * t
 * @endcode
 *
 * The only exceptions are that @a src never goes past @a dest, and @src will
 * be set to @a dest if the difference is less than @a clip. (The clipping is
 * done per vector component).
 *
 * @param src The source vector. It is written with the new vector.
 * @param dest The destination vector.
 * @param t The scaling factor.
 * @param clip The clipping factor.
 */
void vector_transition(union vector *src, const union vector *dest, double t, double clip)
{
	src->v[0] = trans(src->v[0], dest->v[0], t, clip);
	src->v[1] = trans(src->v[1], dest->v[1], t, clip);
	src->v[2] = trans(src->v[2], dest->v[2], t, clip);
	src->v[3] = trans(src->v[3], dest->v[3], t, clip);
}

double trans(double x, double y, double t, double clip)
{
	double tmp;

	/* Too close, return the destination */
	if(fabs(y - x) < clip)
		return y;

	tmp = x + (y - x) * t;

	/* If we went too far, just return the destination */
	if( (y < x) != (y < tmp) )
		return y;
	return tmp;
}
