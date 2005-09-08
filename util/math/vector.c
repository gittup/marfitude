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

#include <math.h>

#include "vector.h"

/** @file
 * Provides some vector math functions
 */

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
	return sqrt(v->p.x*v->p.x + v->p.y*v->p.y + v->p.z*v->p.z);
}

/** Divide each element in vector @a v by the magnitude of @a v. If the vector
 * is close to 0, no normalization is done and the function returns 0.
 * Otherwise, the function returns 1.
 *
 * @param v The vector to normalize.
 * @retval 0 The normalization did not complete successfully (vector likely
 *           too small)
 * @retval 1 The normalization worked.
 */
int vector_normalize(union vector *v)
{
	double m;
	m = vector_mag(v);
	if(m <= 0.000001)
		return 0;
	v->v[0] /= m;
	v->v[1] /= m;
	v->v[2] /= m;
	if(vector_mag(v) < 0.9)
		return 0;
	return 1;
}
