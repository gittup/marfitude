/*
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

#include "strfunc.h"
#include "memtest.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/** @file
 * Implements some random string utility functions.
 */

/** Determine the length required to hold a string representing the
 * integer @a x.
 * @param x An integer
 * @return The length of a buffer required to hold @a x.
 */
int int_len(int x)
{
	if(!x) return 1;
	if(x < 0) return int_len(-x)+1;
	return floor(log10((double)x))+1;
}

/** Concatenate @a a and @a b in a malloc()ed buffer.
  * @param a A nul-terminated character string
  * @param b A nul-terminated character string
  * @return The new string, which must be freed.
  */
char *cat_str(const char *a, const char *b)
{
	char *s;
	s = malloc(sizeof(char) * (strlen(a) + strlen(b) + 1));
	strcpy(s, a);
	strcat(s, b);
	return s;
}

/** Allocate a strlen(@a s)+1 buffer and copy @a s to it.
  * @param s A nul-terminated character string
  * @return A copy of the string, which must be freed.
  */
char *string_copy(const char *s)
{
	char *d;
	d = malloc(sizeof(char) * (strlen(s) + 1));
	strcpy(d, s);
	return d;
}
