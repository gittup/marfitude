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

struct slist {
	struct slist *next;
	void *data;
};

typedef void (*ForeachFunc)(void *, void *);
typedef int (*CompareFunc)(const void *, const void *);

int slist_length(struct slist *l);
struct slist *slist_append(struct slist *l, void *d);
struct slist *slist_remove(struct slist *l, void *d);
struct slist *slist_nth(struct slist *l, int n);
struct slist *slist_insert(struct slist *l, void *d);
struct slist *slist_insert_sorted(struct slist *l, void *d, CompareFunc c);
struct slist *slist_find_custom(struct slist *l, void *d, CompareFunc c);
struct slist *slist_next(struct slist *l);
void slist_foreach(struct slist *l, ForeachFunc f, void *user);
void slist_free(struct slist *l);
void slist_usage(void);
