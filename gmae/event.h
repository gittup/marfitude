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
 *
 * This file also provides a mechanism to register with named events and fire
 * them.
 */

/** All functions that will be registered for events must be of this type.
 * The void * parameter is dependent on the specific event that is registered.
 */
typedef void (*EventHandler)(const void *);

/** A list of event structures */
struct event_handler {
	EventHandler handler;        /**< The function to call on an event */
	int stopHere;                /**< Don't call all other handlers */
	int registered;              /**< 1 if enabled, 0 if not */
	struct event_handler *next;  /**< Next handler in the list */
};

/** An event - relates a name to a set of event handlers */
struct event {
	char *name;                     /**< The name of the event */
	int fired;                      /**< Number of times its been fired */
	struct event_handler *handlers; /**< The first handler in the list */
	struct event *next;             /**< Next event in the list */
};

void FireEvent(const char *event, const void *data);
void HandleEvent(struct event *e, const void *data);
struct event *GetEvent(const char *event);
void RegisterEvent(const char *event, EventHandler handler, int stopHere);
void DeregisterEvent(const char *event, EventHandler handler);
void QuitEvents(void);

/** allows already registered events to fire */
#define EVENTTYPE_MULTI 0
/** stops all future events from firing, until deregistered */
#define EVENTTYPE_STOP 1
