#include <stdio.h>
#include <stdlib.h>

#include "slist.h"

typedef struct {
	slist *list;
	int active;
	} Listmem;

static void AddSlists(Listmem *m, int x);
static void ClearMem(slist *l);
static slist *NextList(void);

static Listmem *mem = NULL;
static int memsize = 0;
static int memused = 0;
static int current = 0;

void AddSlists(Listmem *m, int x)
{
	int i;
	slist *s = (slist*)malloc(sizeof(slist) * x);

	for(i=0;i<x;i++)
	{
		m->list = &s[i];
		m->active = 0;
		m++;
	}
}

void ClearMem(slist *l)
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

slist *NextList(void)
{
	int x;
	if(memused >= memsize)
	{
		if(!memsize) memsize = 128;
		else memsize <<= 1;
		mem = (Listmem*)realloc(mem, sizeof(Listmem) * memsize);
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

int slist_length(slist *l)
{
	int len = 0;
	while(l != NULL)
	{
		l = l->next;
		len++;
	}
	return len;
}

slist *slist_append(slist *l, void *d)
{
	slist *last;
	slist *head = l;

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

slist *slist_remove(slist *l, void *d)
{
	slist *head = l;
	slist *prev;
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

slist *slist_nth(slist *l, int n)
{
	while(n)
	{
		if(l == NULL) return l;
		l = l->next;
		n--;
	}
	return l;
}

slist *slist_insert_sorted(slist *l, void *d, CompareFunc c)
{
	slist *ins = NextList();
	slist *head = l;
	slist *prev;

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

slist *slist_find_custom(slist *l, void *d, CompareFunc c)
{
	while(l != NULL)
	{
		if(!c(l->data, d)) return l;
		l = l->next;
	}
	return NULL;
}

slist *slist_next(slist *l)
{
	return l->next;
}

void slist_foreach(slist *l, ForeachFunc f, void *user)
{
	while(l != NULL)
	{
		f(l->data, user);
		l = l->next;
	}
}

void slist_free(slist *l)
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
