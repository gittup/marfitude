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

/** @file
 * Allows the ability to override memory functions to track leaks
 */

#if CONFIG_MEMTEST == 1
#define malloc(x) MyMalloc(x, __LINE__, __FILE__)
#define realloc(p, x) MyRealloc(p, x, __LINE__, __FILE__)
#define free(p) MyFree(p, __LINE__, __FILE__)
#define calloc(x, y) MyCalloc(x, y, __LINE__, __FILE__)
#endif

#include "memtest_defs.h"
