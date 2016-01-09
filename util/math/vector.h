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

/** @file
 * Some simple and crappy vector functions.
 */

#include "vector_t.h"

void vector_cross(union vector *c, const union vector *v1, const union vector *v2);
double vector_dot(const union vector *v1, const union vector *v2);
double vector_mag(const union vector *v);
int vector_normalize(union vector *v);
void vector_transition(union vector *src, const union vector *dest, double t, double clip);
void vector_print(const union vector *v);

/** Macro to print the name of the vector, and then display the vector. */
#define print_vector(s) do {\
	printf("Vector: "#s"\n");\
	vector_print(s);\
	printf("\n");\
} while(0)
