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

/** @file
 * A pretty crappy singly-linked list implementation, based off of glib
 */

/** A singly-linked list element
 */
struct slist {
	struct slist *next; /**< Pointer to the next list element */
	void *data;         /**< Pointer to whatever data the list holds */
};

/** A comparison function. compare_func(a, b) will return a number less than
 * zero if a<b, 0 if a==b, and a number greater than zero if a>b
 */
typedef int (*compare_func)(const void *, const void *);

/** Iterate through elements of the list.
 * @param t struct slist * - The iterator
 * @param l struct slist * - The list head
 */
#define slist_foreach(t, l) \
	for((t)=(l); (t)!=0; (t)=(t)->next)

struct slist *slist_append(struct slist *l, void *d) ATTRIBUTE_UNUSED_RESULT;
struct slist *slist_remove(struct slist *l, void *d) ATTRIBUTE_UNUSED_RESULT;
struct slist *slist_nth(struct slist *l, int n) ATTRIBUTE_UNUSED_RESULT;
struct slist *slist_insert(struct slist *l, void *d) ATTRIBUTE_UNUSED_RESULT;
struct slist *slist_insert_sorted(struct slist *l, void *d, compare_func c) ATTRIBUTE_UNUSED_RESULT;
struct slist *slist_find(struct slist *l, void *d) ATTRIBUTE_UNUSED_RESULT;
struct slist *slist_find_custom(struct slist *l, void *d, compare_func c) ATTRIBUTE_UNUSED_RESULT;
struct slist *slist_next(struct slist *l) ATTRIBUTE_UNUSED_RESULT;

int slist_length(const struct slist *l);
void slist_free(struct slist *l);
void slist_usage(void);
void quit_slist(void);
