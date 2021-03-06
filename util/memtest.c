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

#include "memtest_defs.h"

#include <stdio.h>
#include <stdlib.h>

/** @file
 * The implementation of memory allocation routines to track memory leaks.
 */

/** A structure to keep track of what was allocated when and by who */
struct mem_block {
	size_t size;      /**< The size of the allocated memory block */
	int line;         /**< The line number it was allocated on */
	const char *file; /**< The file that made the allocation */
	void *ptr;        /**< Where the block points to */
	int active;       /**< 1 if this mem_block structure is in use */
};

/* note that the memory used to keep track of memory is never freed
 * ... ahh sweet irony
 */
static struct mem_block *mb = NULL;
static int num_blocks = 0;
static int start_block = 0;

/** Overrides malloc if CONFIG_MEMTEST == 1 */
void *my_malloc(size_t x, int line, const char *file)
{
	void *p;
	p = malloc(x);

	mb = (struct mem_block*)realloc(mb, sizeof(struct mem_block) * (num_blocks+1));
	mb[num_blocks].size = x;
	mb[num_blocks].line = line;
	mb[num_blocks].file = file;
	mb[num_blocks].ptr = p;
	mb[num_blocks].active = 1;
	num_blocks++;
	return p;
}

/** Overrides free if CONFIG_MEMTEST == 1 */
void my_free(void *p, int line, const char *file)
{
	int i;

	/* Freeing null doesn't do anything */
	if(p == 0)
		return;
	for(i=start_block; i<num_blocks; i++) {
		if(mb[i].ptr == p && mb[i].active) {
			mb[i].active = 0;
			if(i == start_block) {
				while(!mb[start_block].active && start_block < num_blocks) start_block++;
			}
			free(p);
			return;
		}
	}
	fprintf(stderr, "Mem Error: Can't free ptr in %s line %i!\n", file, line);
}

/** Overrides realloc if CONFIG_MEMTEST == 1 */
void *my_realloc(void *p, int x, int line, const char *file)
{
	int i;
	if(p == NULL)
		return my_malloc(x, line, file);
	if(x == 0) {
		my_free(p, line, file);
		return NULL;
	}
	for(i=start_block; i<num_blocks; i++) {
		if(mb[i].ptr == p && mb[i].active) {
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

/** Overrides calloc if CONFIG_MEMTEST == 1 */
void *my_calloc(size_t nm, size_t x, int line, const char *file)
{
	void *p;
	p = calloc(nm, x);
	mb = (struct mem_block*)realloc(mb, sizeof(struct mem_block) * (num_blocks+1));
	mb[num_blocks].size = x*nm;
	mb[num_blocks].line = line;
	mb[num_blocks].file = file;
	mb[num_blocks].ptr = p;
	mb[num_blocks].active = 1;
	num_blocks++;
	return p;
}

/** Checks all blocks to see if they're active. If any memories have not been
 * deallocated, a message is displayed on stdout. This is usually called at
 * the end of a program, after all cleanup code.
 */
void check_mem_usage(void)
{
	int x;
	for(x=0;x<num_blocks;x++) {
		if(mb[x].active) {
			printf("Block from %s line %i occupying %i bytes is active.\n", mb[x].file, mb[x].line, (int)mb[x].size);
		}
	}
}

/** Returns the amount of bytes currently in use */
int query_mem_usage(void)
{
	int x;
	int total = 0;
	for(x=0;x<num_blocks;x++) {
		if(mb[x].active) total += mb[x].size;
	}
	return total;
}
