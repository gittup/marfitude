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

static struct event *FindEvent(const char *);
static void ChkEvent(struct event *e);

static struct event *events = NULL;

/** Returns the struct event if one exists for @a s, or creates a new
 * struct event and returns that. Can be used with HandleEvent() as a
 * replacement for FireEvent if speed is an issue.
 */
struct event *GetEvent(const char *s)
{
	struct event *e;
	e = FindEvent(s);
	if(e == NULL) {
		e = malloc(sizeof(struct event));
		e->name = StringCopy(s);
		e->handlers = NULL;
		e->fired = 0;
		e->next = events;
		events = e;
		printf("Create: %s\n", e->name);
	}
	return e;
}

/* Returns the struct event if one exists for s, or NULL if none do. */
struct event *FindEvent(const char *s)
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
void FireEvent(const char *event, const void *data)
{
	struct event *e;

	e = FindEvent(event);

	if(e != NULL) {
		HandleEvent(e, data);
	}
}

/** Fires the event structure represented by @a e, passing the event-specific
 * @a data. Can be used with GetEvent() as a replacement for FireEvent()
 * if speed is a concern.
 */
void HandleEvent(struct event *e, const void *data)
{
	static int handling = 0;
	struct event_handler *h;
	int x = 0;
	int y = 0;

	handling++;

	e->fired++;
	h = e->handlers;
	while(h != NULL) {
		if(h->registered) {
			h->handler(data);
			if(h->stopHere) break;
			y++;
		}
		h = h->next;
		x++;
	}
	handling--;
}

/** Register's the @a handler with event named @a event. Normally, all
 * registered events are called when the event is fired. However, if @a stopHere
 * is specified, then this becomes the last event to fire until it is
 * unregistered.
 */
void RegisterEvent(const char *event, EventHandler handler, int stopHere)
{
	struct event *e;
	struct event_handler *h;

	e = GetEvent(event);
	h = e->handlers;
	if(stopHere) {
		if(h && h->registered)
			h = NULL;
	} else {
		while(h != NULL && h->registered)
			h = h->next;
	}
	if(h == NULL) {
		h = malloc(sizeof(struct event_handler));
		h->next = e->handlers;
		e->handlers = h;
	}
	h->handler = handler;
	h->stopHere = stopHere;
	h->registered = 1;
}

/** Deregisters the @a handler from the event named @a event. */
void DeregisterEvent(const char *event, EventHandler handler)
{
	struct event *e;
	struct event_handler *h;

	e = FindEvent(event);
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

void ChkEvent(struct event *e)
{
	struct event_handler *h;

	h = e->handlers;
	while(h != NULL) {
		if(h->registered) {
			printf("Event \"%s\" is still registered.\n", e->name);
		}
		h = h->next;
		free(e->handlers);
		e->handlers = h;
	}
	printf("Free Event: \"%s\" fired %i times.\n", e->name, e->fired);
}

/** All the struct events are kept around for the life of the program. This
 * ends that life.
 */
void QuitEvents(void)
{
	struct event *e = events;

	/* Free the first event in the list */
	while(e != NULL) {
		ChkEvent(e);
		free(e->name);
		free(e);
		e = events->next;
		events = e;
	}
}
