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

#include "slist.h"

/** @file
 * Implements a singly-linked list
 */

/** Internal structure used to hold list memory */
struct listmem {
	struct slist *list; /**< A pointer to a list element */
	int active;         /**< 1 if this element is in use, 0 otherwise */
};

static void add_slists(struct listmem *m, int x);
static void clear_mem(struct slist *l);
static struct slist *next_list(void);

static struct listmem *mem = NULL;
static int memsize = 0;
static int memused = 0;
static int current = 0;

void add_slists(struct listmem *m, int x)
{
	int i;
	struct slist *s = malloc(sizeof(struct slist) * x);

	for(i=0;i<x;i++) {
		m->list = &s[i];
		m->active = 0;
		m++;
	}
}

void clear_mem(struct slist *l)
{
	int x;
	for(x=0;x<memsize;x++) {
		if(mem[x].list == l) {
			mem[x].active = 0;
			memused--;
			return;
		}
	}
	fprintf(stderr, "SLIST ERROR: Couldn't find mem to clear!\n");
}

struct slist *next_list(void)
{
	int x;
	if(memused >= memsize) {
		if(!memsize) memsize = 128;
		else memsize <<= 1;
		mem = (struct listmem*)realloc(mem, sizeof(struct listmem) * memsize);
		add_slists(mem+memused, memsize-memused);
	}

	x = current + 1;
	while(x != current) {
		if(x == memsize) x = 0;
		if(!mem[x].active) {
			mem[x].active = 1;
			memused++;
			current = x;
			return mem[x].list;
		}
		x++;
	}
	fprintf(stderr, "SLIST ERROR: Couldn't find inactive list!\n");
	return NULL;
}

/** Determine the length of the list. O(n) operation
 * @param l The list
 * @return The number of list elements in @a l
 */
int slist_length(struct slist *l)
{
	int len = 0;
	while(l != NULL) {
		l = l->next;
		len++;
	}
	return len;
}

/** Appends @a d to the end of list @a l. O(n) operation
 * @param l The list
 * @param d The new data member
 * @return The new list
 */
struct slist *slist_append(struct slist *l, void *d)
{
	struct slist *last;
	struct slist *head = l;

	last = next_list();
	last->next = NULL;
	last->data = d;
	if(head == NULL) return last;
	while(l->next != NULL) {
		l = l->next;
	}
	l->next = last;
	return head;
}

/** Removes @a d to from list @a l, if it exists. No action is taken
 * if @a d does not exist in @a l. O(n) operation.
 * @param l The list
 * @param d The data member to be removed.
 * @return The new list.
 */
struct slist *slist_remove(struct slist *l, void *d)
{
	struct slist *head = l;
	struct slist *prev;
	if(head->data == d) {
		clear_mem(head);
		return head->next;
	}
	prev = l;
	l = l->next;
	while(l != NULL) {
		if(l->data == d) {
			prev->next = l->next;
			clear_mem(l);
		}
		prev = l;
		l = l->next;
	}
	return head;
}

/** Returns the @a n th element in list @a l. O(n) operation.
 * @param l The list
 * @param n The element to return (0 == head)
 * @return The @a n th element, or NULL if outside the bounds of the list.
 */
struct slist *slist_nth(struct slist *l, int n)
{
	if(n < 0) return NULL;
	while(n) {
		if(l == NULL) return l;
		l = l->next;
		n--;
	}
	return l;
}

/** Prepends @a d to list @a l. O(1) operation.
 * @param l The list
 * @param d The new data member
 * @return The new list
 */
struct slist *slist_insert(struct slist *l, void *d)
{
	struct slist *head;

	head = next_list();
	head->next = l;
	head->data = d;
	return head;
}

/** Inserts @a d into list @a l. @a d is inserted before the first value in the
 * list where d < list->data. @a c is used to compare d to the data in the list.
 * O(n) operation.
 * @param l The list
 * @param d The new data member
 * @param c A comparison function.
 * @return The new list
 */
struct slist *slist_insert_sorted(struct slist *l, void *d, CompareFunc c)
{
	struct slist *ins = next_list();
	struct slist *head = l;
	struct slist *prev;

	ins->next = NULL;
	ins->data = d;
	if(head == NULL) return ins;
	if(c(d, head->data) < 0) {
		ins->next = head;
		return ins;
	}
	prev = head;
	l = l->next;
	while(l != NULL) {
		if(c(d, l->data) < 0) {
			ins->next = prev->next;
			prev->next = ins;
			return head;
		}
		prev = l;
		l = l->next;
	}
	prev->next = ins;
	return head;
}

/** Finds the first value in the list where @a d is equal to the list data. @a c
 * is used to compare the data. O(n) operation.
 * @param l The list
 * @param d The data member to compare
 * @param c A comparison function.
 * @return The list where d == list->data
 */
struct slist *slist_find_custom(struct slist *l, void *d, CompareFunc c)
{
	while(l != NULL) {
		if(!c(l->data, d)) return l;
		l = l->next;
	}
	return NULL;
}

/** Returns the next element in the list. O(1) operation.
 * @param l The list
 * @return @a l ->next
 */
struct slist *slist_next(struct slist *l)
{
	return l->next;
}

/** Frees all memory used to hold the list. O(n) operation.
 * @param l The list
 */
void slist_free(struct slist *l)
{
	while(l != NULL) {
		clear_mem(l);
		l = l->next;
	}
}

/** Displays memory usage for slists.
 */
void slist_usage(void)
{
	int x;
	for(x=0;x<memsize;x++)
		if(mem[x].active)
			printf("%i active\n", x);
	printf("%i / %i mem used, at %i\n", memused, memsize, current);
}
