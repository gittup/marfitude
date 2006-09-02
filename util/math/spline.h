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

#ifndef spline_h
#define spline_h

#include "matrix_t.h"
#include "vector_t.h"

void quit_spline(void);
void bspline_patch(const matrix_array x_points,
		   const matrix_array y_points,
		   const matrix_array z_points,
		   int mstride,
		   union vector *point,
		   union vector *tangent,
		   union vector *binormal,
		   union vector *normal,
		   double t,
		   double u);

#endif
