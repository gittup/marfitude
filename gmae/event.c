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

#include "SDL.h"

#include "event.h"
#include "main.h"
#include "menu.h"
#include "joy.h"
#include "cfg.h"
#include "log.h"

#include "util/memtest.h"
#include "util/strfunc.h"

/** The type field in struct joykey for keyboardses */
#define JK_KEYBOARD -1
/** The type field in struct joykey for mouses */
#define JK_MOUSE -2
/** The axis field in struct joykey for buttonses */
#define JK_BUTTON -1

/** @file
 * Handles SDL events and event registration/firing.
 */

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
static struct event *FindEvent(const char *);
static void button_event(int button);
static void ChkEvent(struct event *e);

void button_event(int button)
{
	struct button_e b;
	b.button = button;
	FireEvent("button", &b);
}

static int eventMode = MENU;
static struct event *events = NULL;
static struct joykey buttons[B_LAST] = {{0,0,0}};
static const char *cfgMsg[B_LAST] = {	"buttons.up",
					"buttons.down",
					"buttons.left",
					"buttons.right",
					"buttons.button1",
					"buttons.button2",
					"buttons.button3",
					"buttons.button4",
					"buttons.select",
					"buttons.menu"};

/** Set the EventMode to one of KEY, MENU, or GAME */
void EventMode(int mode)
{
	if(mode == MENU) SDL_EnableKeyRepeat(500, 20);
	else SDL_EnableKeyRepeat(0, 0);
	eventMode = mode;
}

/** Returns the struct event if one exists for @a s, or creates a new
 * struct event and returns that. Can be used with HandleEvent() as a
 * replacement for FireEvent if speed is an issue.
 */
struct event *GetEvent(const char *s)
{
	struct event *e;
	e = FindEvent(s);
	if(e == NULL) {
		e = (struct event *)malloc(sizeof(struct event));
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

/** Set the game button @a b to be set to the joykey structure @a jk */
int SetButton(int b, const struct joykey *jk)
{
	char *s;
	if(b < 0 || b >= B_LAST) return 0;
	s = (char*)malloc(IntLen(jk->type)+IntLen(jk->button)+IntLen(jk->axis)+3);
	sprintf(s, "%i.%i.%i", jk->type, jk->button, jk->axis);
	CfgSetS(cfgMsg[b], s);
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
	if(CfgS(cfgParam) == NULL) {
		return 1;
	}
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

/** Initializes joykeys from the configuration file */
int ConfigureJoyKey(void)
{
	int x;
	int err = 0;
	for(x=0;x<B_LAST;x++) {
		err += CfgButton(&(buttons[x]), cfgMsg[x]);
	}
	return err;
}

/** Returns a string that represents the requested button from the buttonType
 * enum.
 */
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
	struct eventHandler *h;

	e->fired++;
	h = e->handlers;
	while(h != NULL) {
		h->handler(data);
		if(h->stopHere) break;
		h = h->next;
	}
}

/** Register's the @a handler with event named @a event. Normally, all
 * registered events are called when the event is fired. However, if @a stopHere
 * is specified, then this becomes the last event to fire until it is
 * unregistered.
 */
void RegisterEvent(const char *event, EventHandler handler, int stopHere)
{
	struct event *e;
	struct eventHandler *h;

	e = GetEvent(event);
	h = (struct eventHandler *)malloc(sizeof(struct eventHandler));
	h->handler = handler;
	h->stopHere = stopHere;
	h->next = e->handlers;
	e->handlers = h;
}

/** Deregisters the @a handler from the event named @a event. */
void DeregisterEvent(const char *event, EventHandler handler)
{
	struct event *e;
	struct eventHandler *h;
	struct eventHandler *prev = NULL;

	e = FindEvent(event);
	if(e == NULL) {
		ELog(("Error: Event %s not available for deregister.\n", event));
		return;
	}
	h = e->handlers;
	while(h != NULL) {
		if(h->handler == handler) {
			if(prev)
				prev->next = h->next;
			else
				e->handlers = h->next;
			break;
		}
		prev = h;
		h = h->next;
	}
	if(h == NULL) ELog(("Error: Event %s not available for deregister.\n", event));
	else free(h);
}

void ChkEvent(struct event *e)
{
	if(e->handlers != NULL) {
		printf("Warning: Event \"%s\" is still registered.\n", e->name);
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

/** Clear out the SDL_Event queue */
void ClearEvents(void)
{
	SDL_Event event;
	while(SDL_PollEvent(&event)) {}
}

void KeyDownEvent(SDL_KeyboardEvent *e)
{
	int x;
	struct joykey jk;

	if(eventMode == KEY)
	{
		jk.type = JK_KEYBOARD;
		jk.button = e->keysym.sym;
		jk.axis = JK_KEYBOARD;
		FireEvent("key", &jk);
	}
	else if(eventMode == MENU)
	{
		if(	e->keysym.sym == SDLK_ESCAPE ||
				KeyEqual(&buttons[B_MENU], e))
			button_event(B_MENU);

		else if(	e->keysym.sym == SDLK_UP ||
				KeyEqual(&buttons[B_UP], e))
			button_event(B_UP);

		else if(	e->keysym.sym == SDLK_DOWN ||
				KeyEqual(&buttons[B_DOWN], e))
			button_event(B_DOWN);

		else if(	e->keysym.sym == SDLK_RIGHT ||
				KeyEqual(&buttons[B_RIGHT], e))
			button_event(B_RIGHT);

		else if(	e->keysym.sym == SDLK_LEFT ||
				KeyEqual(&buttons[B_LEFT], e))
			button_event(B_LEFT);

		else if(	e->keysym.sym == SDLK_RETURN ||
				KeyEqual(&buttons[B_BUTTON1], e) ||
				KeyEqual(&buttons[B_BUTTON2], e) ||
				KeyEqual(&buttons[B_BUTTON3], e) ||
				KeyEqual(&buttons[B_BUTTON4], e))
			FireEvent("enter", NULL);
		else if (	e->keysym.sym == SDLK_TAB ||
				KeyEqual(&buttons[B_SELECT], e)
			)
			button_event(B_SELECT);
		else if(e->keysym.sym == SDLK_PAGEUP)
			FireEvent("pageup", NULL);
		else if(e->keysym.sym == SDLK_PAGEDOWN)
			FireEvent("pagedown", NULL);
		else if(e->keysym.sym == SDLK_HOME)
			FireEvent("home", NULL);
		else if(e->keysym.sym == SDLK_END)
			FireEvent("end", NULL);
		/* no else cuz all other keys are ignored :) */
	}
	else if(eventMode == GAME)
	{
		for(x=0;x<B_LAST;x++)
			if(KeyEqual(&buttons[x], e))
				button_event(x);
	}
}

void MouseButtonEvent(SDL_MouseButtonEvent *e)
{
	int x;
	struct joykey jk;

	if(eventMode == KEY)
	{
		jk.type = JK_MOUSE;
		jk.button = e->button;
		jk.axis = JK_BUTTON;
		FireEvent("key", &jk);
	}
	else if(eventMode == MENU)
	{
		if(MouseButtonEqual(&buttons[B_MENU], e))
			button_event(B_MENU);
		else
			FireEvent("enter", NULL);
	}
	else if(eventMode == GAME)
	{
		for(x=0;x<B_LAST;x++)
			if(MouseButtonEqual(&buttons[x], e))
				button_event(x);
	}
}

void JoyButtonDownEvent(SDL_JoyButtonEvent *e)
{
	int x;
	struct joykey jk;

	if(eventMode == KEY)
	{
		jk.type = e->which;
		jk.button = e->button;
		jk.axis = JK_BUTTON;
		FireEvent("key", &jk);
	}
	else if(eventMode == MENU)
	{
		/* all buttons activate in menu mode except MENU/SELECT */
		if(JoyButtonEqual(&buttons[B_MENU], e))
			button_event(B_MENU);
		if(JoyButtonEqual(&buttons[B_SELECT], e))
			button_event(B_SELECT);
		else
			FireEvent("enter", NULL);
	}
	else if(eventMode == GAME)
	{
		for(x=B_UP;x<B_LAST;x++)
			if(JoyButtonEqual(&buttons[x], e))
				button_event(x);
	}
}

void JoyAxisEvent(SDL_JoyAxisEvent *e)
{
	int x;
	struct joykey jk;

	if(e->value == 0) return;
	if(eventMode == KEY)
	{
		jk.type = e->which;
		jk.button = (e->value>0)?1:-1;
		jk.axis = e->axis;
		FireEvent("key", &jk);
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
				button_event(B_DOWN);
			}
			if(e->value < -JOY_THRESHOLD)
			{
				button_event(B_UP);
			}
			/* wait for event.jaxis.value to be less than
			 * JOY_THRESHOLD before executing menu again?
			 */
		}
	}
	else if(eventMode == GAME)
	{
		for(x=B_UP;x<B_LAST;x++)
			if(JoyAxisEqual(&buttons[x], e))
				button_event(x);
	}
}

/** Handle all SDL input, and fire appropriate events. */
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
