/*
   Copyright (C) 2005 Mike Shal

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

#include <stdlib.h>

#include "myrand.h"

/** @file
 * The implementation of random number functions
 */

/** Gets a random integer from [0, x)
 * @return The random integer
 */
int rand_int(int x)
{
	return (int)((double)x * rand()/(RAND_MAX+1.0));
}

/** Gets a random float from [0.0, 1.0)
 * @return The random float
 */
float rand_float(void)
{
	float x = rand();
	return x / (float)(RAND_MAX+1.0);
}
