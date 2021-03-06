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

#include "fatalerror.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

/** @file
 * Wraps strerror with the file and line number.
 */

/** The underlying error function. Actually called using the Error macro. */
void fatal_error(const char *file, int line, const char *msg)
{
	fprintf(stderr, "Error %s in %s, line %i: %s\n", msg, file, line, strerror(errno));
}
