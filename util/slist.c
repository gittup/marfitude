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

struct listmem {
	struct slist *list;
	int active;
};

static void AddSlists(struct listmem *m, int x);
static void ClearMem(struct slist *l);
static struct slist *NextList(void);

static struct listmem *mem = NULL;
static int memsize = 0;
static int memused = 0;
static int current = 0;

void AddSlists(struct listmem *m, int x)
{
	int i;
	struct slist *s = (struct slist*)malloc(sizeof(struct slist) * x);

	for(i=0;i<x;i++)
	{
		m->list = &s[i];
		m->active = 0;
		m++;
	}
}

void ClearMem(struct slist *l)
{
	int x;
	for(x=0;x<memsize;x++)
	{
		if(mem[x].list == l)
		{
			mem[x].active = 0;
			memused--;
			return;
		}
	}
	fprintf(stderr, "SLIST ERROR: Couldn't find mem to clear!\n");
}

struct slist *NextList(void)
{
	int x;
	if(memused >= memsize)
	{
		if(!memsize) memsize = 128;
		else memsize <<= 1;
		mem = (struct listmem*)realloc(mem, sizeof(struct listmem) * memsize);
		AddSlists(mem+memused, memsize-memused);
	}

	x = current + 1;
	while(x != current)
	{
		if(x == memsize) x = 0;
		if(!mem[x].active)
		{
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

int slist_length(struct slist *l)
{
	int len = 0;
	while(l != NULL)
	{
		l = l->next;
		len++;
	}
	return len;
}

struct slist *slist_append(struct slist *l, void *d)
{
	struct slist *last;
	struct slist *head = l;

	last = NextList();
	last->next = NULL;
	last->data = d;
	if(head == NULL) return last;
	while(l->next != NULL)
	{
		l = l->next;
	}
	l->next = last;
	return head;
}

struct slist *slist_remove(struct slist *l, void *d)
{
	struct slist *head = l;
	struct slist *prev;
	if(head->data == d)
	{
		ClearMem(head);
		return head->next;
	}
	prev = l;
	l = l->next;
	while(l != NULL)
	{
		if(l->data == d)
		{
			prev->next = l->next;
			ClearMem(l);
		}
		prev = l;
		l = l->next;
	}
	return head;
}

struct slist *slist_nth(struct slist *l, int n)
{
	while(n)
	{
		if(l == NULL) return l;
		l = l->next;
		n--;
	}
	return l;
}

struct slist *slist_insert(struct slist *l, void *d)
{
	struct slist *head;

	head = NextList();
	head->next = l;
	head->data = d;
	return head;
}

struct slist *slist_insert_sorted(struct slist *l, void *d, CompareFunc c)
{
	struct slist *ins = NextList();
	struct slist *head = l;
	struct slist *prev;

	ins->next = NULL;
	ins->data = d;
	if(head == NULL) return ins;
	if(c(d, head->data) < 0)
	{
		ins->next = head;
		return ins;
	}
	prev = head;
	l = l->next;
	while(l != NULL)
	{
		if(c(d, l->data) < 0)
		{
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

struct slist *slist_find_custom(struct slist *l, void *d, CompareFunc c)
{
	while(l != NULL)
	{
		if(!c(l->data, d)) return l;
		l = l->next;
	}
	return NULL;
}

struct slist *slist_next(struct slist *l)
{
	return l->next;
}

void slist_foreach(struct slist *l, ForeachFunc f, void *user)
{
	while(l != NULL)
	{
		f(l->data, user);
		l = l->next;
	}
}

void slist_free(struct slist *l)
{
	while(l != NULL)
	{
		ClearMem(l);
		l = l->next;
	}
}

void slist_usage(void)
{
	int x;
	for(x=0;x<memsize;x++)
		if(mem[x].active)
			printf("%i active\n", x);
	printf("%i / %i mem used, at %i\n", memused, memsize, current);
}
