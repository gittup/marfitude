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

#include "textprogress.h"

#define HASHES 40

static int marks;

void ProgressMeter(const char *text)
{
	int i;
        printf("%s: [", text);
        for(i=0;i<HASHES;i++) printf(" ");
        printf("]");
        for(i=0;i<HASHES+1;i++) printf("%c", 8);
	marks = 0;
}

void UpdateProgress(int part, int whole)
{
	while(part * HASHES >= (marks+1) * whole)
	{
		printf("#");
		marks++;
	}
	fflush(stdout);
}

void EndProgressMeter(void)
{
	printf("]\n");
}
