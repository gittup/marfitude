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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL.h"

#include "event.h"
#include "main.h"
#include "menu.h"
#include "joy.h"
#include "cfg.h"
#include "log.h"

#include "memtest.h"
#include "strfunc.h"

#define JK_KEYBOARD -1
#define JK_MOUSE -2
#define JK_BUTTON -1

struct event {
	EventHandler handler;
	int stopHere;
	struct event *next;
};

static char *NextDot(char *s);
static int CfgButton(struct joykey *key, const char *cfgParam);
static int KeyEqual(struct joykey *key, SDL_KeyboardEvent *e);
static int MouseButtonEqual(struct joykey *key, SDL_MouseButtonEvent *e);
static int JoyButtonEqual(struct joykey *key, SDL_JoyButtonEvent *e);
static int JoyAxisEqual(struct joykey *key, SDL_JoyAxisEvent *e);
static void KeyDownEvent(SDL_KeyboardEvent *e);
static void MouseButtonEvent(SDL_MouseButtonEvent *e);
static void JoyButtonDownEvent(SDL_JoyButtonEvent *e);
static void JoyAxisEvent(SDL_JoyAxisEvent *e);

int eventMode = MENU;
struct event *events[EVENT_LAST] = {0};
KeyHandler keyHandler;
struct joykey buttons[B_LAST] = {{0,0,0}};
const char *cfgStrings[B_LAST] = {	"buttons.up",
					"buttons.down",
					"buttons.left",
					"buttons.right",
					"buttons.button1",
					"buttons.button2",
					"buttons.button3",
					"buttons.button4",
					"buttons.menu"};

void EventMode(int mode)
{
	if(mode == MENU) SDL_EnableKeyRepeat(500, 20);
	else SDL_EnableKeyRepeat(0, 0);
	eventMode = mode;
}

/* finds the beginning of the next . number
 * in config file, key is a.b.c
 * so after atoi() gets a, NextDot returns "b.c" then we get b, etc. */
char *NextDot(char *s)
{
	if(s == NULL) return NULL;
	while(*s && *s != '.') s++;
	if(*s) return s+1;
	return NULL;
}

int SetButton(int b, struct joykey *jk)
{
	char *s;
	if(b < 0 || b >= B_LAST) return 0;
	s = (char*)malloc(IntLen(jk->type)+IntLen(jk->button)+IntLen(jk->axis)+3);
	sprintf(s, "%i.%i.%i", jk->type, jk->button, jk->axis);
	CfgSetS(cfgStrings[b], s);
	free(s);
	buttons[b].type = jk->type;
	buttons[b].button = jk->button;
	buttons[b].axis = jk->axis;
	return 1;
}

int CfgButton(struct joykey *key, const char *cfgParam)
{
	char *s;
	char *t;
	s = (char*)malloc(sizeof(char) * (strlen(CfgS(cfgParam))+1));
	strcpy(s, CfgS(cfgParam));
	t = s;
	key->type = atoi(s);
	s = NextDot(s);
	if(s == NULL)
	{
		ELog(("Invalid configuration string for '%s'.\n", cfgParam));
		return 1;
	}
	key->button = atoi(s);
	s = NextDot(s);
	if(s == NULL)
	{
		ELog(("Invalid configuration string for '%s'.\n", cfgParam));
		return 1;
	}
	key->axis = atoi(s);
	free(t);
	return 0;
}

int ConfigureJoyKey(void)
{
	int x;
	int err = 0;
	for(x=0;x<B_LAST;x++)
	{
		err += CfgButton(&(buttons[x]), cfgStrings[x]);
	}
	return err;
}

char *JoyKeyName(int button)
{
	int len;
	char *s = NULL;
	struct joykey *jk = buttons+button;

	if(jk->type == JK_KEYBOARD)
	{
		char *t;
		t = SDL_GetKeyName(jk->button);
		s = (char*)malloc(sizeof(char) * (strlen(t)+1));
		strcpy(s, t);
	}
	else if(jk->type == JK_MOUSE)
	{
		len = IntLen(jk->button) + 7;
		s = (char*)malloc(sizeof(char) * len);
		sprintf(s, "Mouse %i", jk->button);
	}
	else /* joystick */
	{
		if(jk->axis == JK_BUTTON) /* button event */
		{
			len = IntLen(jk->type) + IntLen(jk->button) + 13;
			s = (char*)malloc(sizeof(char) * len);
			sprintf(s, "Joy %i Button %i", jk->type, jk->button);
		}
		else /* axis event */
		{
			len = IntLen(jk->type) + IntLen(jk->button) + 15;
			s = (char*)malloc(sizeof(char) * len);
			sprintf(s, "Joy %i Axis %i (%c)", jk->type, jk->axis, (jk->button > 0) ? '+' : '-');
		}
	}
	return s;
}

int KeyEqual(struct joykey *key, SDL_KeyboardEvent *e)
{
	if(	key->type == JK_KEYBOARD &&
		key->button == (signed)e->keysym.sym &&
		key->axis == JK_BUTTON)
		return 1;
	return 0;
}

int MouseButtonEqual(struct joykey *key, SDL_MouseButtonEvent *e)
{
	if(	key->type == JK_MOUSE &&
		key->button == (signed)e->button &&
		key->axis == JK_BUTTON)
		return 1;
	return 0;
}

int JoyButtonEqual(struct joykey *key, SDL_JoyButtonEvent *e)
{
	if(	key->type == e->which &&
		key->button == e->button &&
		key->axis == JK_BUTTON)
		return 1;
	return 0;
}

int JoyAxisEqual(struct joykey *key, SDL_JoyAxisEvent *e)
{
	if(	key->type == e->which &&
		key->button == (e->value > JOY_THRESHOLD ? 1 : e->value < -JOY_THRESHOLD ? -1 : 0) &&
		key->axis == e->axis)
		return 1;
	return 0;
}

void FireEvent(int event)
{
	struct event *e;
	e = events[event];
	while(e != NULL)
	{
		e->handler();
		if(e->stopHere) break;
		e = e->next;
	}
}

int RegisterKeyEvent(KeyHandler handler)
{
	if(!keyHandler)
	{
		keyHandler = handler;
		return 1;
	}
	else
	{
		ELog(("Error: KeyHandler already set!\n"));
		return 0;
	}
}

void DeregisterKeyEvent(void)
{
	if(keyHandler)
	{
		keyHandler = NULL;
	}
	else
	{
		ELog(("Error: KeyHandler not set!\n"));
	}
}

int RegisterEvent(int event, EventHandler handler, int stopHere)
{
	struct event *e;
	e = (struct event *)malloc(sizeof(struct event));
	e->handler = handler;
	e->stopHere = stopHere;
	e->next = events[event];
	events[event] = e;
	return 1;
}

void DeregisterEvent(int event, EventHandler handler)
{
	struct event *e;
	struct event *prev = NULL;
	e = events[event];
	while(e != NULL)
	{
		if(e->handler == handler)
		{
			if(prev)
			{
				prev->next = e->next;
			}
			else
			{
				events[event] = e->next;
			}
			break;
		}
		prev = e;
		e = e->next;
	}
	if(e == NULL) ELog(("Error: Event %i not available for deregister.\n", event));
	else free(e);
}

void ClearEvents(void)
{
	SDL_Event event;
	while(SDL_PollEvent(&event)) {}
}

void KeyDownEvent(SDL_KeyboardEvent *e)
{
	int x;
	struct joykey jk;

	if(eventMode == KEY && keyHandler)
	{
		jk.type = JK_KEYBOARD;
		jk.button = e->keysym.sym;
		jk.axis = JK_KEYBOARD;
		keyHandler(&jk);
	}
	else if(eventMode == MENU)
	{
		if(	e->keysym.sym == SDLK_ESCAPE ||
				KeyEqual(&buttons[B_MENU], e))
			FireEvent(EVENT_MENU);

		else if(	e->keysym.sym == SDLK_UP ||
				KeyEqual(&buttons[B_UP], e))
			FireEvent(EVENT_UP);

		else if(	e->keysym.sym == SDLK_DOWN ||
				KeyEqual(&buttons[B_DOWN], e))
			FireEvent(EVENT_DOWN);

		else if(	e->keysym.sym == SDLK_RIGHT ||
				KeyEqual(&buttons[B_RIGHT], e))
			FireEvent(EVENT_RIGHT);

		else if(	e->keysym.sym == SDLK_LEFT ||
				KeyEqual(&buttons[B_LEFT], e))
			FireEvent(EVENT_LEFT);

		else if(	e->keysym.sym == SDLK_RETURN ||
				KeyEqual(&buttons[B_BUTTON1], e) ||
				KeyEqual(&buttons[B_BUTTON2], e) ||
				KeyEqual(&buttons[B_BUTTON3], e) ||
				KeyEqual(&buttons[B_BUTTON4], e))
			FireEvent(EVENT_ENTER);
		else if(e->keysym.sym == SDLK_PAGEUP)
			FireEvent(EVENT_PAGEUP);
		else if(e->keysym.sym == SDLK_PAGEDOWN)
			FireEvent(EVENT_PAGEDOWN);
		/* no else cuz all other keys are ignored :) */
	}
	else if(eventMode == GAME)
	{
		for(x=0;x<B_LAST;x++)
			if(KeyEqual(&buttons[x], e)) FireEvent(x);
	}
	/* no else (ignore eventMode == KEY and no keyHandler) */
}

void MouseButtonEvent(SDL_MouseButtonEvent *e)
{
	int x;
	struct joykey jk;

	if(eventMode == KEY && keyHandler)
	{
		jk.type = JK_MOUSE;
		jk.button = e->button;
		jk.axis = JK_BUTTON;
		keyHandler(&jk);
	}
	else if(eventMode == MENU)
	{
		if(MouseButtonEqual(&buttons[B_MENU], e))
			FireEvent(EVENT_MENU);
		else
			FireEvent(EVENT_ENTER);
	}
	else if(eventMode == GAME)
	{
		for(x=0;x<B_LAST;x++)
			if(MouseButtonEqual(&buttons[x], e)) FireEvent(x);
	}
	/* no else (ignore eventMode == KEY and no keyHandler) */
}

void JoyButtonDownEvent(SDL_JoyButtonEvent *e)
{
	int x;
	struct joykey jk;

	if(eventMode == KEY && keyHandler)
	{
		jk.type = e->which;
		jk.button = e->button;
		jk.axis = JK_BUTTON;
		keyHandler(&jk);
	}
	else if(eventMode == MENU)
	{
		/* all buttons activate in menu mode except the MENU button */
		if(JoyButtonEqual(&buttons[B_MENU], e))
			FireEvent(EVENT_MENU);
		else
			FireEvent(EVENT_ENTER);
	}
	else if(eventMode == GAME)
	{
		for(x=B_UP;x<B_LAST;x++)
			if(JoyButtonEqual(&buttons[x], e)) FireEvent(x);
	}
	/* no else (ignore eventMode == KEY and no keyHandler) */
}

void JoyAxisEvent(SDL_JoyAxisEvent *e)
{
	int x;
	struct joykey jk;

	if(e->value == 0) return;
	if(eventMode == KEY && keyHandler)
	{
		jk.type = e->which;
		jk.button = (e->value>0)?1:-1;
		jk.axis = e->axis;
		keyHandler(&jk);
	}
	else if(eventMode == MENU)
	{
		/* Only handle up and down in menu mode
		 * (I think all verticle axi / axes / axisi are odd, maybe)
		 */
		if(e->axis & 1)
		{
			if(e->value > JOY_THRESHOLD)
			{
				FireEvent(EVENT_DOWN);
			}
			if(e->value < -JOY_THRESHOLD)
			{
				FireEvent(EVENT_UP);
			}
			/* wait for event.jaxis.value to be less than
			 * JOY_THRESHOLD before executing menu again?
			 */
		}
	}
	else if(eventMode == GAME)
	{
		for(x=B_UP;x<B_LAST;x++)
			if(JoyAxisEqual(&buttons[x], e)) FireEvent(x);
	}
	/* no else (ignore eventMode == KEY and no keyHandler) */
}

void EventLoop(void)
{
	int moreEvents = 1;
	SDL_Event event;

	while(moreEvents && SDL_PollEvent(&event))
	{
		/* this checks if there's another event waiting
		 * so if the event takes too long to process we won't
		 * stay in the event loop forever :) */
		moreEvents = SDL_PollEvent(NULL);
		switch(event.type)
		{
			case SDL_KEYDOWN:
				KeyDownEvent(&event.key);
				break;
			case SDL_KEYUP:
/*				printf("Key up\n"); */
				break;
			case SDL_JOYAXISMOTION:
				JoyAxisEvent(&event.jaxis);
#if 0
				if(event.jaxis.axis == 1)
				{
					if(event.jaxis.value > JOY_THRESHOLD)
					{
						FireEvent(EVENT_DOWN);
					}
					if(event.jaxis.value < -JOY_THRESHOLD)
					{
						FireEvent(EVENT_UP);
					}
					/* wait for event.jaxis.value to be less than JOY_THRESHOLD before executing menu again */
				}
#endif
				break;
			case SDL_JOYBUTTONDOWN:
				if(JoyIgnoreButton(event.jbutton.which, event.jbutton.button)) break;
				JoyButtonDownEvent(&event.jbutton);
				break;
			case SDL_MOUSEBUTTONDOWN:
				MouseButtonEvent(&event.button);
				break;
			case SDL_QUIT:
				quit = 1;
			default:
				break;
		}
	}
	return;
}
