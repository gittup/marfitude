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
 * Handles all input from SDL and some event registration/firing.
 *
 * This file handles input from SDL (eg: keyboard, mouse, joystick) and
 * attempts to make it all somewhat generic. This file also provides a
 * mechanism to register with named events and fire them.
 */

/** Lists the types of buttons that can be configured. This probably shouldn't
 * be here, but in the actual game code instead.
 */
enum buttonType {
	B_UP = 0, B_DOWN, B_LEFT, B_RIGHT,
	B_BUTTON1, B_BUTTON2, B_BUTTON3, B_BUTTON4,
	B_SELECT, B_MENU,
	B_LAST
};

/** All functions that will be registered for events must be of this type.
 * The void * parameter is dependent on the specific event that is registered.
 */
typedef void (*EventHandler)(const void *);

/** A joystick / keyboard wrapper struct.
 * Also functions as the data for the "key" event.
 */
struct joykey {
	int type;    /**< -1 for keybd, -2 for mouse, 0-n for joysticks */
	int button;  /**< keysym.sym for keybd, mouse button for mouse,
		      * button # for joystick button
		      * 1 if axis>0, -1 if axis<0
		      */
	int axis;    /**< -1 if no axis, >=0 if this was an axis */
};

/** Data for the "button" event.
 */
struct button_e {
	enum buttonType button; /**< The button that caused the event */
};

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

void ClearEvents(void);
void EventLoop(void);
void EventMode(int mode);
void FireEvent(const char *event, const void *data);
void HandleEvent(struct event *e, const void *data);
struct event *GetEvent(const char *event);
int ConfigureJoyKey(void);
char *JoyKeyName(int button);
int SetButton(int b, const struct joykey *jk);
void RegisterEvent(const char *event, EventHandler handler, int stopHere);
void DeregisterEvent(const char *event, EventHandler handler);
void QuitEvents(void);

/** KEY mode allows raw key input - eg. for configuring  */
#define KEY 0
/** MENU mode is the same as GAME but also allows some default keys
 * (eg. escape key is always the "menu" key, enter functions as a "button" key
 */
#define MENU 1
/** GAME mode is based solely on the config file */
#define GAME 2

/** allows already registered events to fire */
#define EVENTTYPE_MULTI 0
/** stops all future events from firing, until deregistered */
#define EVENTTYPE_STOP 1
