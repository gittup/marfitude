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

/** @file
 * Handles event registration/firing.
 */

/** All functions that will be registered for events must be of this type.
 * The void * parameter is dependent on the specific event that is registered.
 */
typedef void (*event_handler)(const void *);

/** A list of event structures */
struct event_handler {
	event_handler handler;       /**< The function to call on an event */
	int ref_count;               /**< Number of references to the handler */
};

/** An event - relates a name to a set of event handlers */
struct event {
	char *name;             /**< The name of the event */
	int fired;              /**< Number of times its been fired */
	struct slist *handlers; /**< The list of event handlers */
};

void fire_event(const char *event, const void *data);
void handle_event(struct event *e, const void *data);
struct event *get_event(const char *event);
void register_event(const char *event, event_handler handler);
void deregister_event(const char *event, event_handler handler);
void quit_events(void);
