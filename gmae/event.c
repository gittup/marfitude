/*
   Marfitude
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "event.h"
#include "log.h"

#include "util/memtest.h"
#include "util/strfunc.h"
#include "util/slist.h"

/** @file
 * Handles event registration/firing.
 */

static struct event *find_event(const char *);
static void chk_event(struct event *e);

static struct slist *events = NULL;

/** Returns the struct event if one exists for @a s, or creates a new
 * struct event and returns that. Can be used with handle_event() as a
 * replacement for fire_event if speed is an issue.
 */
struct event *get_event(const char *s)
{
	struct event *e;
	e = find_event(s);
	if(e == NULL) {
		e = malloc(sizeof(struct event));
		e->name = string_copy(s);
		e->handlers = NULL;
		e->fired = 0;
		events = slist_insert(events, e);
	}
	return e;
}

/* Returns the struct event if one exists for s, or NULL if none do. */
struct event *find_event(const char *s)
{
	struct event *e;
	struct slist *t;
	slist_foreach(t, events) {
		e = t->data;
		if(strcmp(s, e->name) == 0) {
			return e;
		}
	}
	return NULL;
}

/** Fires the event named @a event, passing the event-specific @a data */
void fire_event(const char *event, const void *data)
{
	struct event *e;

	e = get_event(event);
	handle_event(e, data);
}

/** Fires the event structure represented by @a e, passing the event-specific
 * @a data. Can be used with get_event() as a replacement for fire_event()
 * if speed is a concern.
 */
void handle_event(struct event *e, const void *data)
{
	struct event_handler *h;
	struct slist *tmp = NULL;
	struct slist *t;

	e->fired++;
	slist_foreach(t, e->handlers) {
		h = t->data;
		tmp = slist_insert(tmp, h);
		h->ref_count++;
	}

	slist_foreach(t, tmp) {
		h = t->data;
		/* Fire if there's more than just the ref_count we added above.
		 * This means the event is still registered, since the count
		 * is only decremented on deregister.
		 */
		if(h->ref_count > 1) {
			h->handler(data);
		}
		h->ref_count--;
		if(h->ref_count == 0) {
			e->handlers = slist_remove(e->handlers, h);
			free(h);
		}
	}
	slist_free(tmp);
}

/** Register's the @a handler with event named @a event. No guarantee is given
 * for the order the handlers are called when the event is fired. For example,
 * if handles A and B are registered for the same event, A could be called
 * before B, or B could be called for A.
 *
 * @param event The event name to register with.
 * @param handler The function to call when the event is fired.
 */
void register_event(const char *event, event_handler handler)
{
	struct event *e;
	struct event_handler *h;

	e = get_event(event);
	h = malloc(sizeof(struct event_handler));
	e->handlers = slist_insert(e->handlers, h);
	h->handler = handler;
	h->ref_count = 1;
}

/** Deregisters the @a handler from the event named @a event. */
void deregister_event(const char *event, event_handler handler)
{
	struct slist *t;
	struct event *e;
	struct event_handler *h;

	e = find_event(event);
	if(e == NULL) {
		ELog(("Error: %s not available for deregister.\n", event));
		return;
	}
	slist_foreach(t, e->handlers) {
		h = t->data;
		if(h->handler == handler) {
			h->ref_count--;
			if(h->ref_count == 0) {
				e->handlers = slist_remove(e->handlers, h);
				free(h);
			}
			return;
		}
	}
	ELog(("Error: %s not available for deregister.\n", event));
}

void chk_event(struct event *e)
{
	struct slist *t;
	struct event_handler *h;

	slist_foreach(t, e->handlers) {
		h = t->data;
		printf("Event \"%s\" is still registered.\n", e->name);
		free(h);
	}
	slist_free(e->handlers);
	printf("Event: %18s %11i\n", e->name, e->fired);
}

/** All the struct events are kept around for the life of the program. This
 * ends that life.
 */
void quit_events(void)
{
	struct slist *t;
	struct event *e;

	printf("       --- Free Event --- Times Fired ---\n");
	slist_foreach(t, events) {
		e = t->data;
		chk_event(e);
		free(e->name);
		free(e);
	}
	slist_free(events);
	events = NULL;
}
