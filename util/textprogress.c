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

/** The number of hash marks to display */
#define HASHES 40

/** @file
 * Displays a text progress bar
 */

static int marks;

/** Starts the progress meter with label @a text
 * @param text The label for the text bar
 */
void ProgressMeter(const char *text)
{
	int i;
        printf("%s: [", text);
        for(i=0;i<HASHES;i++) printf(" ");
        printf("]");
        for(i=0;i<HASHES+1;i++) printf("%c", 8);
	marks = 0;
}

/** Updates the progress bar. Sets it to be @a part / @a whole finished.
 * @param part How far along the bar is
 * @param whole How long the bar is
 */
void UpdateProgress(int part, int whole)
{
	while(part * HASHES >= (marks+1) * whole)
	{
		printf("#");
		marks++;
	}
	fflush(stdout);
}

/** Finishes the progress meter and prints a new line */
void EndProgressMeter(void)
{
	printf("]\n");
}
