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

#include "spline.h"
#include "matvec.h"
#include "vector.h"
#include "matrix.h"

#include <stdio.h>

#define tvec(vec, t) do { \
        (vec).v[1] = (t) * (t); \
        (vec).v[0] = (t) * (vec).v[1]; \
        (vec).v[2] = (t); \
        (vec).v[3] = 1; \
} while(0)

#define dvec(vec, t) do { \
        (vec).v[0] = 3 * (t) * (t); \
        (vec).v[1] = 2 * (t); \
        (vec).v[2] = 1; \
        (vec).v[3] = 0; \
} while(0)

static matrix_array bspline_basis = {
	-1 / 6.0,  3 / 6.0, -3 / 6.0, 1 / 6.0,
	3 / 6.0, -6 / 6.0,  3 / 6.0, 0 / 6.0,
	-3 / 6.0,  0 / 6.0,  3 / 6.0, 0 / 6.0,
	1 / 6.0,  4 / 6.0,  1 / 6.0, 0 / 6.0
};
static matrix_array bspline_transpose = {
	-1 / 6.0,  3 / 6.0, -3 / 6.0, 1 / 6.0,
	3 / 6.0, -6 / 6.0,  0 / 6.0, 4 / 6.0,
	-3 / 6.0,  3 / 6.0,  3 / 6.0, 1 / 6.0,
	1 / 6.0,  0 / 6.0,  0 / 6.0, 0 / 6.0
};

static int num_splines = 0;

/** Just print the number of spline patches used. This really exists to link
 * in the spline library without using whole-archive.
 */
void quit_spline(void)
{
	printf("Calculated %i spline patches\n", num_splines);
}

/** Create the point, tangent, binormal, and normal vectors from the input
 * control matrices and the (t, u) position. The point is the standard bspline
 * patch point, the tangent is the derivative in the t direction, and the
 * binormal is the derivative in the u direction. The normal is calculated as
 * the cross product of the tangent & binormal.
 */
void bspline_patch(const matrix_array x_points,
		   const matrix_array y_points,
		   const matrix_array z_points,
		   int mstride,
		   union vector *point,
		   union vector *tangent,
		   union vector *binormal,
		   union vector *normal,
		   double t,
		   double u)
{
	union vector xvec, yvec, zvec;
	union vector tv, uv, dtv, duv;
	union vector tmp;

	num_splines++;

	tvec(tv, t);
	tvec(uv, u);
	dvec(dtv, t);
	dvec(duv, u);

	matrix_rowvec_mul(&tmp, &tv, bspline_basis, 4);
	matrix_rowvec_mul(&xvec, &tmp, x_points, mstride);
	matrix_rowvec_mul(&yvec, &tmp, y_points, mstride);
	matrix_rowvec_mul(&zvec, &tmp, z_points, mstride);
	matrix_rowvec_mul(&tmp, &xvec, bspline_transpose, 4);
	point->v[0] = vector_dot(&tmp, &uv);
	binormal->v[0] = vector_dot(&tmp, &duv);
	matrix_rowvec_mul(&tmp, &yvec, bspline_transpose, 4);
	point->v[1] = vector_dot(&tmp, &uv);
	binormal->v[1] = vector_dot(&tmp, &duv);
	matrix_rowvec_mul(&tmp, &zvec, bspline_transpose, 4);
	point->v[2] = vector_dot(&tmp, &uv);
	binormal->v[2] = vector_dot(&tmp, &duv);
	point->v[3] = 0.0;
	binormal->v[3] = 0.0;

	matrix_rowvec_mul(&tmp, &dtv, bspline_basis, 4);
	matrix_rowvec_mul(&xvec, &tmp, x_points, mstride);
	matrix_rowvec_mul(&yvec, &tmp, y_points, mstride);
	matrix_rowvec_mul(&zvec, &tmp, z_points, mstride);
	matrix_rowvec_mul(&tmp, &xvec, bspline_transpose, 4);
	tangent->v[0] = vector_dot(&tmp, &uv);
	matrix_rowvec_mul(&tmp, &yvec, bspline_transpose, 4);
	tangent->v[1] = vector_dot(&tmp, &uv);
	matrix_rowvec_mul(&tmp, &zvec, bspline_transpose, 4);
	tangent->v[2] = vector_dot(&tmp, &uv);
	tangent->v[3] = 0.0;

	vector_normalize(binormal);
	vector_normalize(tangent);
	vector_cross(normal, tangent, binormal);
	normal->v[3] = 0.0;
}
