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

#include "input.h"
#include "cfg.h"
#include "event.h"
#include "joy.h"
#include "log.h"
#include "main.h"

#include "util/memtest.h"
#include "util/strfunc.h"

/** The type field in struct joykey for keyboardses */
#define JK_KEYBOARD -1
/** The type field in struct joykey for mouses */
#define JK_MOUSE -2
/** The axis field in struct joykey for buttonses */
#define JK_BUTTON -1
/** If a key hasn't been set */
#define JK_UNSET -3

/** @file
 * Handles SDL events.
 */

static char *next_dot(char *s);
static int cfg_button(struct joykey *key, const char *cfgParam);
static int key_equal(struct joykey *key, SDL_KeyboardEvent *e);
static int mouse_button_equal(struct joykey *key, SDL_MouseButtonEvent *e);
static int joy_button_equal(struct joykey *key, SDL_JoyButtonEvent *e);
static int joy_axis_equal(struct joykey *key, SDL_JoyAxisEvent *e);
static void key_down_event(SDL_KeyboardEvent *e);
static void mouse_button_event(SDL_MouseButtonEvent *e);
static void joy_button_down_event(SDL_JoyButtonEvent *e);
static void joy_axis_event(SDL_JoyAxisEvent *e);
static void button_event(int button);

static int cur_mode = MENU;
static struct joykey buttons[B_LAST];
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

/** Clear out the SDL_Event queue */
void clear_input(void)
{
	SDL_Event event;
	while(SDL_PollEvent(&event)) {}
}

/** Handle all SDL input, and fire appropriate events. */
void input_loop(void)
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
				key_down_event(&event.key);
				break;
			case SDL_KEYUP:
/*				printf("Key up\n"); */
				break;
			case SDL_JOYAXISMOTION:
				joy_axis_event(&event.jaxis);
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
				joy_button_down_event(&event.jbutton);
				break;
			case SDL_MOUSEBUTTONDOWN:
				mouse_button_event(&event.button);
				break;
			case SDL_QUIT:
				quit = 1;
			default:
				break;
		}
	}
	return;
}

/** Set the input mode to one of KEY, MENU, or GAME */
void input_mode(enum event_mode mode)
{
	if(mode == MENU) SDL_EnableKeyRepeat(500, 20);
	else SDL_EnableKeyRepeat(0, 0);
	cur_mode = mode;
}

/** Initializes joykeys from the configuration file */
int configure_joykeys(void)
{
	int x;
	int err = 0;
	for(x=0;x<B_LAST;x++) {
		err += cfg_button(&(buttons[x]), cfgMsg[x]);
	}
	return err;
}

/** Returns a string that represents the requested button from the buttonType
 * enum.
 */
char *joykey_name(int button)
{
	int len;
	char *s = NULL;
	struct joykey *jk = buttons+button;

	if(jk->type == JK_KEYBOARD)
	{
		char *t;
		t = SDL_GetKeyName(jk->button);
		s = malloc(sizeof(char) * (strlen(t)+1));
		strcpy(s, t);
	}
	else if(jk->type == JK_MOUSE)
	{
		len = int_len(jk->button) + 7;
		s = malloc(sizeof(char) * len);
		sprintf(s, "Mouse %i", jk->button);
	}
	else if(jk->type == JK_UNSET)
	{
		const char *a = "*** UNSET ***";
		s = malloc(strlen(a) + 1);
		strcpy(s, a);
	}
	else /* joystick */
	{
		if(jk->axis == JK_BUTTON) /* button event */
		{
			len = int_len(jk->type) + int_len(jk->button) + 13;
			s = malloc(sizeof(char) * len);
			sprintf(s, "Joy %i Button %i", jk->type, jk->button);
		}
		else /* axis event */
		{
			len = int_len(jk->type) + int_len(jk->button) + 15;
			s = malloc(sizeof(char) * len);
			sprintf(s, "Joy %i Axis %i (%c)", jk->type, jk->axis, (jk->button > 0) ? '+' : '-');
		}
	}
	return s;
}

/** Set the game button @a b to be set to the joykey structure @a jk */
int set_button(int b, const struct joykey *jk)
{
	char *s;
	if(b < 0 || b >= B_LAST) return 0;
	s = malloc(int_len(jk->type)+int_len(jk->button)+int_len(jk->axis)+3);
	sprintf(s, "%i.%i.%i", jk->type, jk->button, jk->axis);
	CfgSetS(cfgMsg[b], s);
	free(s);
	buttons[b].type = jk->type;
	buttons[b].button = jk->button;
	buttons[b].axis = jk->axis;
	return 1;
}

void button_event(int button)
{
	struct button_e b;
	b.button = button;
	FireEvent("button", &b);
}

/* finds the beginning of the next . number
 * in config file, key is a.b.c
 * so after atoi() gets a, next_dot returns "b.c" then we get b, etc. */
char *next_dot(char *s)
{
	if(s == NULL) return NULL;
	while(*s && *s != '.') s++;
	if(*s) return s+1;
	return NULL;
}

int cfg_button(struct joykey *key, const char *cfgParam)
{
	char *s;
	char *t;
	if(CfgS(cfgParam) == NULL) {
		key->type = JK_UNSET;
		key->button = 0;
		key->axis = 0;
		return 1;
	}
	s = malloc(sizeof(char) * (strlen(CfgS(cfgParam))+1));
	strcpy(s, CfgS(cfgParam));
	t = s;
	key->type = atoi(s);
	s = next_dot(s);
	if(s == NULL)
	{
		ELog(("Invalid configuration string for '%s'.\n", cfgParam));
		return 1;
	}
	key->button = atoi(s);
	s = next_dot(s);
	if(s == NULL)
	{
		ELog(("Invalid configuration string for '%s'.\n", cfgParam));
		return 1;
	}
	key->axis = atoi(s);
	free(t);
	return 0;
}

int key_equal(struct joykey *key, SDL_KeyboardEvent *e)
{
	if(	key->type == JK_KEYBOARD &&
		key->button == (signed)e->keysym.sym &&
		key->axis == JK_BUTTON)
		return 1;
	return 0;
}

int mouse_button_equal(struct joykey *key, SDL_MouseButtonEvent *e)
{
	if(	key->type == JK_MOUSE &&
		key->button == (signed)e->button &&
		key->axis == JK_BUTTON)
		return 1;
	return 0;
}

int joy_button_equal(struct joykey *key, SDL_JoyButtonEvent *e)
{
	if(	key->type == e->which &&
		key->button == e->button &&
		key->axis == JK_BUTTON)
		return 1;
	return 0;
}

int joy_axis_equal(struct joykey *key, SDL_JoyAxisEvent *e)
{
	if(	key->type == e->which &&
		key->button == (e->value > JOY_THRESHOLD ? 1 : e->value < -JOY_THRESHOLD ? -1 : 0) &&
		key->axis == e->axis)
		return 1;
	return 0;
}

void key_down_event(SDL_KeyboardEvent *e)
{
	int x;
	struct joykey jk;

	if(cur_mode == KEY)
	{
		jk.type = JK_KEYBOARD;
		jk.button = e->keysym.sym;
		jk.axis = JK_KEYBOARD;
		FireEvent("key", &jk);
	}
	else if(cur_mode == MENU)
	{
		if(	e->keysym.sym == SDLK_ESCAPE ||
				key_equal(&buttons[B_MENU], e))
			button_event(B_MENU);

		else if(	e->keysym.sym == SDLK_UP ||
				key_equal(&buttons[B_UP], e))
			button_event(B_UP);

		else if(	e->keysym.sym == SDLK_DOWN ||
				key_equal(&buttons[B_DOWN], e))
			button_event(B_DOWN);

		else if(	e->keysym.sym == SDLK_RIGHT ||
				key_equal(&buttons[B_RIGHT], e))
			button_event(B_RIGHT);

		else if(	e->keysym.sym == SDLK_LEFT ||
				key_equal(&buttons[B_LEFT], e))
			button_event(B_LEFT);

		else if(	e->keysym.sym == SDLK_RETURN ||
				key_equal(&buttons[B_BUTTON1], e) ||
				key_equal(&buttons[B_BUTTON2], e) ||
				key_equal(&buttons[B_BUTTON3], e) ||
				key_equal(&buttons[B_BUTTON4], e))
			FireEvent("enter", NULL);
		else if (	e->keysym.sym == SDLK_TAB ||
				key_equal(&buttons[B_SELECT], e)
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
	else if(cur_mode == GAME)
	{
		for(x=0;x<B_LAST;x++)
			if(key_equal(&buttons[x], e))
				button_event(x);
	}
}

void mouse_button_event(SDL_MouseButtonEvent *e)
{
	int x;
	struct joykey jk;

	if(cur_mode == KEY)
	{
		jk.type = JK_MOUSE;
		jk.button = e->button;
		jk.axis = JK_BUTTON;
		FireEvent("key", &jk);
	}
	else if(cur_mode == MENU)
	{
		if(mouse_button_equal(&buttons[B_MENU], e))
			button_event(B_MENU);
		else
			FireEvent("enter", NULL);
	}
	else if(cur_mode == GAME)
	{
		for(x=0;x<B_LAST;x++)
			if(mouse_button_equal(&buttons[x], e))
				button_event(x);
	}
}

void joy_button_down_event(SDL_JoyButtonEvent *e)
{
	int x;
	struct joykey jk;

	if(cur_mode == KEY)
	{
		jk.type = e->which;
		jk.button = e->button;
		jk.axis = JK_BUTTON;
		FireEvent("key", &jk);
	}
	else if(cur_mode == MENU)
	{
		/* all buttons activate in menu mode except MENU/SELECT */
		if(joy_button_equal(&buttons[B_MENU], e))
			button_event(B_MENU);
		if(joy_button_equal(&buttons[B_SELECT], e))
			button_event(B_SELECT);
		else
			FireEvent("enter", NULL);
	}
	else if(cur_mode == GAME)
	{
		for(x=B_UP;x<B_LAST;x++)
			if(joy_button_equal(&buttons[x], e))
				button_event(x);
	}
}

void joy_axis_event(SDL_JoyAxisEvent *e)
{
	int x;
	struct joykey jk;

	if(e->value == 0) return;
	if(cur_mode == KEY)
	{
		jk.type = e->which;
		jk.button = (e->value>0)?1:-1;
		jk.axis = e->axis;
		FireEvent("key", &jk);
	}
	else if(cur_mode == MENU)
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
	else if(cur_mode == GAME)
	{
		for(x=B_UP;x<B_LAST;x++)
			if(joy_axis_equal(&buttons[x], e))
				button_event(x);
	}
}
