/*
   Marfitude
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

struct joykey {
	int type; /* -1 for keybd, -2 for mouse, 0-n for joysticks */
	int button;	/* keysym.sym for keybd, mouse button for mouse,
			 * button # for joystick button
			 * 1 if axis>0, -1 if axis<0
			 */
	int axis; /* -1 if no axis, >=0 if this was an axis */
};

typedef void (*EventHandler)(void);
typedef void (*KeyHandler)(struct joykey *);

void ClearEvents(void);
void EventLoop(void);
void EventMode(int mode);
void FireEvent(int event);
int ConfigureJoyKey(void); /* initialies joykeys from config file */
char *JoyKeyName(int button); /* button defined B_ below (caller must free string) */
int SetButton(int b, struct joykey *jk);
int RegisterKeyEvent(KeyHandler handler);
void DeregisterKeyEvent(void);
int RegisterEvent(int event, EventHandler handler, int stopHere);
void DeregisterEvent(int event, EventHandler handler);

/* KEY mode allows raw key input - eg. for configuring */
/* MENU mode is the same as GAME but also allows some default keys */
/*  (eg. escape key is always the "menu" key, enter functions as a "button" key */
/* GAME mode is based solely on the config file */
#define KEY 0
#define MENU 1
#define GAME 2

#define EVENTTYPE_MULTI 0 /* allows already registered events to fire */
#define EVENTTYPE_STOP 1 /* stops all future events from firing, until deregistered */

/* first B_LAST events correspond to the buttons below */
enum eventType {
	EVENT_UP, EVENT_DOWN, EVENT_LEFT, EVENT_RIGHT,
	EVENT_BUTTON1, EVENT_BUTTON2, EVENT_BUTTON3, EVENT_BUTTON4,
	EVENT_MENU,
	EVENT_ENTER, EVENT_SHOWMENU, EVENT_HIDEMENU,
	EVENT_PAGEUP, EVENT_PAGEDOWN,
	EVENT_HOME, EVENT_END,
	EVENT_LAST
};

enum buttonType {
	B_UP = 0, B_DOWN, B_LEFT, B_RIGHT,
	B_BUTTON1, B_BUTTON2, B_BUTTON3, B_BUTTON4,
	B_MENU,
	B_LAST
};
