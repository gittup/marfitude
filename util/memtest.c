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

#include "memtest_defs.h"

struct memBlock {
	size_t size;
	int line;
	const char *file;
	void *ptr;
	int active;
};

/* note that the memory used to keep track of memory is never freed
 * ... ahh sweet irony
 */
static struct memBlock *mb = NULL;
static int numBlocks = 0;
static int startBlock = 0;

void *MyMalloc(size_t x, int line, const char *file)
{
	void *p;
	p = malloc(x);

	mb = (struct memBlock*)realloc(mb, sizeof(struct memBlock) * (numBlocks+1));
	mb[numBlocks].size = x;
	mb[numBlocks].line = line;
	mb[numBlocks].file = file;
	mb[numBlocks].ptr = p;
	mb[numBlocks].active = 1;
	numBlocks++;
	return p;
}

void MyFree(void *p, int line, const char *file)
{
	int i;
	for(i=startBlock;i<numBlocks;i++)
	{
		if(mb[i].ptr == p && mb[i].active)
		{
			mb[i].active = 0;
			if(i == startBlock)
			{
				while(!mb[startBlock].active && startBlock < numBlocks) startBlock++;
			}
			free(p);
			return;
		}
	}
	fprintf(stderr, "Mem Error: Can't free ptr in %s line %i!\n", file, line);
}

void *MyRealloc(void *p, int x, int line, const char *file)
{
	int i;
	if(p == NULL) return MyMalloc(x, line, file);
	if(x == 0)
	{
		MyFree(p, line, file);
		return NULL;
	}
	for(i=startBlock;i<numBlocks;i++)
	{
		if(mb[i].ptr == p && mb[i].active)
		{
			mb[i].ptr = realloc(mb[i].ptr, x);
			mb[i].line = line;
			mb[i].size = x;
			mb[i].file = file;
			return mb[i].ptr;
		}
	}
	fprintf(stderr, "Mem error: Can't realloc at %s line %i - ptr doesn't exist!\n", file, line);
	return NULL;
}

void *MyCalloc(size_t nm, size_t x, int line, const char *file)
{
	void *p;
	p = calloc(nm, x);
	mb = (struct memBlock*)realloc(mb, sizeof(struct memBlock) * (numBlocks+1));
	mb[numBlocks].size = x*nm;
	mb[numBlocks].line = line;
	mb[numBlocks].file = file;
	mb[numBlocks].ptr = p;
	mb[numBlocks].active = 1;
	numBlocks++;
	return p;
}

/* checks to see if any memories have not been deallocated
 * usually called at the end of the program, after all cleanup code
 */
void CheckMemUsage()
{
	int x;
	for(x=0;x<numBlocks;x++)
	{
		if(mb[x].active)
		{
			printf("Block from %s line %i occupying %i bytes is active.\n", mb[x].file, mb[x].line, mb[x].size);
		}
	}
}

/* returns the amount of bytes currently in use */
int QueryMemUsage()
{
	int x;
	int total = 0;
	for(x=0;x<numBlocks;x++)
	{
		if(mb[x].active) total += mb[x].size;
	}
	return total;
}
