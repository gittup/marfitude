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
#include "cmp.h"

/** Finds the linear magnitude of a complex number
 * @param a Pointer to a complex number
 * @return The magnitude of the complex number
 */
double mag(const struct cmp *a)
{
	return sqrt(a->real*a->real + a->imag*a->imag);
}

/** Multiplies two complex numbers
 * @param dest Where to write the result of the multiplication
 * @param a The first complex number
 * @param b The second complex number
 */
void mul(struct cmp *dest, const struct cmp *a, const struct cmp *b)
{
	dest->real = a->real*b->real - a->imag*b->imag;
	dest->imag = a->real*b->imag + a->imag*b->real;
}
