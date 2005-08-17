/*
   Marfitude
   Copyright (C) 2005 Mike Shal

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

/** @file
 * Handles event registration/firing.
 */

static struct event *find_event(const char *);
static void chk_event(struct event *e);

static struct event *events = NULL;

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
		e->next = events;
		events = e;
		printf("Create: %s\n", e->name);
	}
	return e;
}

/* Returns the struct event if one exists for s, or NULL if none do. */
struct event *find_event(const char *s)
{
	struct event *e;
	e = events;
	while(e != NULL) {
		if(strcmp(s, e->name) == 0) {
			return e;
		}
		e = e->next;
	}
	return e;
}

/** Fires the event named @a event, passing the event-specific @a data */
void fire_event(const char *event, const void *data)
{
	struct event *e;

	e = find_event(event);

	if(e != NULL) {
		handle_event(e, data);
	}
}

/** Fires the event structure represented by @a e, passing the event-specific
 * @a data. Can be used with get_event() as a replacement for fire_event()
 * if speed is a concern.
 */
void handle_event(struct event *e, const void *data)
{
	struct event_handler *h;

	e->fired++;
	h = e->handlers;
	while(h != NULL) {
		struct event_handler *next;

		next = h->next;
		if(h->registered)
			h->handler(data);
		h = next;
	}
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
	h = e->handlers;
	while(h != NULL && h->registered)
		h = h->next;

	if(h == NULL) {
		h = malloc(sizeof(struct event_handler));
		h->next = e->handlers;
		e->handlers = h;
	}
	h->handler = handler;
	h->registered = 1;
}

/** Deregisters the @a handler from the event named @a event. */
void deregister_event(const char *event, event_handler handler)
{
	struct event *e;
	struct event_handler *h;

	e = find_event(event);
	if(e == NULL) {
		ELog(("Error: %s not available for deregister.\n", event));
		return;
	}
	h = e->handlers;
	while(h != NULL) {
		if(h->handler == handler) {
			h->registered = 0;
			break;
		}
		h = h->next;
	}
	if(h == NULL)
		ELog(("Error: %s not available for deregister.\n", event));
}

void chk_event(struct event *e)
{
	struct event_handler *h;
	int num_handlers = 0;

	h = e->handlers;
	while(h != NULL) {
		if(h->registered) {
			printf("Event \"%s\" is still registered.\n", e->name);
		}
		h = h->next;
		free(e->handlers);
		e->handlers = h;
		num_handlers++;
	}
	printf("Event: %18s %12i    %12i\n", e->name, e->fired, num_handlers);
}

/** All the struct events are kept around for the life of the program. This
 * ends that life.
 */
void quit_events(void)
{
	struct event *e = events;

	printf("       --- Free Event --- Times Fired --- Max Handlers ---\n");
	/* Free the first event in the list */
	while(e != NULL) {
		chk_event(e);
		free(e->name);
		free(e);
		e = events->next;
		events = e;
	}
}
