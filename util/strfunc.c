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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include "memtest.h"

#include "strfunc.h"

int IntLen(int x)
{
	if(!x) return 1;
	if(x < 0) return IntLen(-x)+1;
	return floor(log10((double)x))+1;
}

int StrEq(const char *a, const char *b)
{
	while(*a && *b)
	{
		if(*a != *b) return 0;
		a++;
		b++;
	}
	if(!*a && !*b) return 1;
	return 0;
}

char *CatStr(const char *a, const char *b)
{
	char *s;
	s = (char*)malloc(sizeof(char) * (strlen(a) + strlen(b) + 1));
	strcpy(s, a);
	strcat(s, b);
	return s;
}
