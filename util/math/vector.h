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

/** @file
 * Some simple and crappy vector functions.
 */

#ifndef VECTOR_H
#define VECTOR_H

union vector {
	double v[4];
	struct {
		double x;
		double y;
		double z;
		double w;
	} p;
};

void vector_cross(union vector *c, const union vector *v1, const union vector *v2);
double vector_mag(const union vector *v);
int vector_normalize(union vector *v);

#endif
