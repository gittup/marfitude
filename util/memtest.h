/*
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

#ifdef DEBUG_MEM
#define malloc(x) MyMalloc(x, __LINE__, __FILE__)
#define realloc(p, x) MyRealloc(p, x, __LINE__, __FILE__)
#define free(p) MyFree(p, __LINE__, __FILE__)
#define calloc(x, y) MyCalloc(x, y, __LINE__, __FILE__)
#endif

void *MyMalloc(size_t x, int line, const char *file);
void MyFree(void *p, int line, const char *file);
void *MyRealloc(void *p, int x, int line, const char *file);
void *MyCalloc(size_t nm, size_t x, int line, const char *file);
void CheckMemUsage(void); /* checks all blocks to see if they're active */
int QueryMemUsage(void);  /* returns current mem usage in bytes */
