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
 * Defines the memory test functions that can track memory leaks.
 * Note that the My* functions are used by macros and don't need to be
 * called directly.
 */

void *my_malloc(size_t x, int line, const char *file);
void my_free(void *p, int line, const char *file);
void *my_realloc(void *p, int x, int line, const char *file);
void *my_calloc(size_t nm, size_t x, int line, const char *file);
void check_mem_usage(void);
int query_mem_usage(void);
