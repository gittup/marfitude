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
#include "timer.h"

#include "util/memtest.h"
#include "util/strfunc.h"
#include "util/slist.h"

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

/** Keeps track of when a key was hit so it can be repeated if held down.
 * It is separate from the joykey/button dealy so in the menu un-set buttons
 * can be held and still repeat.
 */
struct keypush {
	struct joykey key; /**< The corresponding joykey */
	Uint32 time_hit;   /**< Last time this button was hit */
	int active;        /**< Currently being held down */
	int repeating;     /**< Currently repeating */
	int repeatable;    /**< 1 if this button can repeat, 0 if not */
	void (*handler)(struct joykey*); /**< Special handler for the menu */
};

static struct slist *keys = NULL;

static void handle_action(struct keypush *kp, int action);
static struct keypush *get_keypush(const struct joykey *jk, void (*)(struct joykey*));
static char *malloc_bstr(int player);
static int set_shift(const struct joykey *jk);
static int fire_joykey(const struct joykey *jk);
static void reset_key_repeats(void);
static char *next_dot(char *s);
static int cfg_button(struct joykey *key, const char *cfgParam, int player);
static int joykey_equal(const struct joykey *a, const struct joykey *b);
static void key_handler(struct joykey *);
static void mouse_button_handler(struct joykey *);
static void joy_button_handler(struct joykey *);
static void joy_axis_handler(struct joykey *);
static void null_handler(struct joykey *);
static void button_event(int button, int player);
static void menu_button_event(int button, int player);

static int cur_mode = MENU;
static struct joykey buttons[MAX_PLAYERS][B_LAST];
static int shift[MAX_PLAYERS] = {0};
static const char *cfgMsg[B_LAST] = {	"up",
					"down",
					"left",
					"right",
					"button1",
					"button2",
					"button3",
					"button4",
					"select",
					"shift",
					"menu"};

static float pc[MAX_PLAYERS][4] = {
	{0.6, 0.6, 1.0, 1.0},
	{1.0, 0.3, 0.3, 1.0},
	{1.0, 1.0, 0.0, 1.0},
	{0.0, 1.0, 0.0, 1.0}
};

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
	struct joykey jk;
	int action;
	void (*special_menu_handler)(struct joykey *) = null_handler;
	struct slist *t;
	SDL_Event event;

	while(moreEvents && SDL_PollEvent(&event))
	{
		struct keypush *kp;

		action = -1;
		special_menu_handler = null_handler;

		/* this checks if there's another event waiting
		 * so if the event takes too long to process we won't
		 * stay in the event loop forever :) */
		moreEvents = SDL_PollEvent(NULL);
		switch(event.type)
		{
			case SDL_KEYDOWN:
				jk.type = JK_KEYBOARD;
				jk.button = event.key.keysym.sym;
				jk.axis = JK_BUTTON;
				special_menu_handler = key_handler;
				action = 1;
				break;
			case SDL_KEYUP:
				jk.type = JK_KEYBOARD;
				jk.button = event.key.keysym.sym;
				jk.axis = JK_BUTTON;
				special_menu_handler = key_handler;
				action = 0;
				break;
			case SDL_JOYAXISMOTION:
				if(abs(event.jaxis.value) < JOY_MIN_THRESHOLD) {
					/* Cancel both the left&right or
					 * up&down keypushes.
					 */
					jk.type = event.jaxis.which;
					jk.button = 1;
					jk.axis = event.jaxis.axis;
					kp = get_keypush(&jk, joy_axis_handler);
					handle_action(kp, 0);
				}
				jk.type = event.jaxis.which;
				jk.button = (event.jaxis.value>=JOY_MIN_THRESHOLD)?1:-1;
				jk.axis = event.jaxis.axis;
				special_menu_handler = joy_axis_handler;
				if(abs(event.jaxis.value) > JOY_MAX_THRESHOLD)
					action = 1;
				if(abs(event.jaxis.value) < JOY_MIN_THRESHOLD)
					action = 0;
				break;
			case SDL_JOYBUTTONDOWN:
				if(joy_ignore_button(event.jbutton.which, event.jbutton.button)) break;
				jk.type = event.jbutton.which;
				jk.button = event.jbutton.button;
				jk.axis = JK_BUTTON;
				special_menu_handler = joy_button_handler;
				action = 1;
				break;
			case SDL_JOYBUTTONUP:
				if(joy_ignore_button(event.jbutton.which, event.jbutton.button)) break;
				jk.type = event.jbutton.which;
				jk.button = event.jbutton.button;
				jk.axis = JK_BUTTON;
				special_menu_handler = joy_button_handler;
				action = 0;
				break;
			case SDL_JOYHATMOTION:
				printf("%i, %i, %i, %i\n", event.jhat.type, event.jhat.which, event.jhat.hat, event.jhat.value);
				break;
			case SDL_MOUSEBUTTONDOWN:
				jk.type = JK_MOUSE;
				jk.button = event.button.button;
				jk.axis = JK_BUTTON;
				special_menu_handler = mouse_button_handler;
				action = 1;
				break;
			case SDL_MOUSEBUTTONUP:
				jk.type = JK_MOUSE;
				jk.button = event.button.button;
				jk.axis = JK_BUTTON;
				special_menu_handler = mouse_button_handler;
				action = 0;
				break;
			case SDL_QUIT:
				gmae_quit();
			default:
				break;
		}

		kp = get_keypush(&jk, special_menu_handler);
		handle_action(kp, action);
	}

	slist_foreach(t, keys) {
		struct keypush *kp = t->data;
		handle_action(kp, -1);
	}
}

/** Set the @a button to be @a repeatable.
 *
 * @param button The button to set
 * @param repeatable 1 if this key should repeat when held down, 0 if not
 */
void set_key_repeat(int button, int repeatable)
{
	struct keypush *kp;
	int p;

	for(p=0; p<MAX_PLAYERS; p++) {
		kp = get_keypush(&buttons[p][button], null_handler);
		kp->repeatable = repeatable;
	}
}

/** Set the input mode to one of KEY, MENU, or GAME */
void input_mode(enum event_mode mode)
{
	cur_mode = mode;
}

/** Initializes joykeys from the configuration file */
int configure_joykeys(void)
{
	int x, p;
	int err = 0;
	for(p=0; p<MAX_PLAYERS; p++) {
		for(x=0; x<B_LAST; x++) {
			int tmp;
			tmp = cfg_button(&(buttons[p][x]), cfgMsg[x], p);

			/* Only worry if the first player's keys aren't set */
			if(p == 0)
				err += tmp;
		}
	}
	return err;
}

/** Returns a string that represents the requested button from the buttonType
 * enum.
 */
char *joykey_name(int button, int player)
{
	int len;
	char *s = NULL;
	struct joykey *jk = &buttons[player][button];

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

/** Set the game button @a b to be set to the joykey structure @a jk
 *
 * @retval 0 Success
 * @retval 1 @a b is an invalid button
 */
int set_button(int b, const struct joykey *jk, int player)
{
	char *s;
	char *bstr;
	if(b < 0 || b >= B_LAST) return 1;

	bstr = malloc_bstr(player);
	reset_key_repeats();
	s = malloc(int_len(jk->type)+int_len(jk->button)+int_len(jk->axis)+3);
	sprintf(s, "%i.%i.%i", jk->type, jk->button, jk->axis);
	cfg_set(bstr, cfgMsg[b], s);
	free(s);
	buttons[player][b].type = jk->type;
	buttons[player][b].button = jk->button;
	buttons[player][b].axis = jk->axis;
	free(bstr);
	return 0;
}

/** Gets the @a player's colors in a float array.
 *
 * @param player The player (0 .. MAX_PLAYERS-1)
 * @return The RGBA colors
 */
const float *get_player_color(int player)
{
	return pc[player];
}

void handle_action(struct keypush *kp, int action)
{
	int fire = 0;

	if(action == 1 && kp->active == 0) {
		kp->active = 1;
		kp->time_hit = curTime;
		fire = 1;
	} else if(action == 0) {
		int p;

		for(p=0; p<MAX_PLAYERS; p++)
			if(joykey_equal(&buttons[p][B_SHIFT], &kp->key))
				shift[p] = 0;
		kp->active = 0;
		kp->repeating = 0;
	} else {
		if(kp->active && (kp->repeatable || cur_mode == MENU)) {
			if(kp->repeating && curTime - kp->time_hit > 30) {
				kp->time_hit = curTime;
				fire = 1;
			} else if(!kp->repeating && curTime - kp->time_hit > 500) {
				kp->time_hit = curTime;
				kp->repeating = 1;
				fire = 1;
			}
		}
	}

	if(fire && fire_joykey(&kp->key))
		kp->handler(&kp->key);
}

struct keypush *get_keypush(const struct joykey *jk, void (*handler)(struct joykey*))
{
	const struct slist *t;
	struct keypush *kp;

	slist_foreach(t, keys) {
		kp = t->data;
		if(joykey_equal(jk, &kp->key)) {
			/* Overwrite the handler in case the first time
			 * get_keypush was called was with the null_handler
			 */
			if(handler != null_handler)
				kp->handler = handler;
			return kp;
		}
	}

	/* Create a new keypush structure, append to the list */
	kp = malloc(sizeof(struct keypush));
	kp->key.type = jk->type;
	kp->key.button = jk->button;
	kp->key.axis = jk->axis;
	kp->time_hit = 0;
	kp->active = 0;
	kp->repeating = 0;
	kp->repeatable = 1;
	kp->handler = handler;
	keys = slist_append(keys, kp);
	return kp;
}

char *malloc_bstr(int player)
{
	char *bstr = string_copy("buttonsx");
	bstr[7] = player+'0';
	return bstr;
}

int set_shift(const struct joykey *jk)
{
	int ret = 0;
	int p;

	for(p=0; p<MAX_PLAYERS; p++) {
		if(joykey_equal(&buttons[p][B_SHIFT], jk)) {
			shift[p] = 1;
			ret = 1;
		}
	}
	return ret;
}

int fire_joykey(const struct joykey *jk)
{
	int x;
	int p;

	/* This if statement is weird: the shift key is only set if it's
	 * in MENU or GAME mode. So that's why the second if part doesn't
	 * match up with the rest.
	 */
	if(cur_mode == KEY) {
		fire_event("key", jk);
	} else if(set_shift(jk)) {
		/* shift is already set by set_shift */
	} else if(cur_mode == MENU) {
		int fired = 0;

		for(p=0; p<MAX_PLAYERS; p++)
			for(x=0; x<B_LAST; x++)
				if(joykey_equal(&buttons[p][x], jk)) {
					menu_button_event(x, p);
					fired = 1;
				}
		if(!fired)
			return 1;
	} else if(cur_mode == GAME) {
		for(p=0; p<MAX_PLAYERS; p++)
			for(x=0; x<B_LAST; x++)
				if(joykey_equal(&buttons[p][x], jk))
					button_event(x, p);
	}
	return 0;
}

void reset_key_repeats(void)
{
	struct slist *t;

	slist_foreach(t, keys) {
		struct keypush *kp = t->data;
		kp->repeatable = 1;
	}
}

void button_event(int button, int player)
{
	struct button_e b;
	b.button = button;
	b.shift = shift[player];
	b.player = player;
	fire_event("button", &b);
}

void menu_button_event(int button, int player)
{
	if(button >= B_BUTTON1 && button <= B_BUTTON4) {
		fire_event("enter", &shift[player]);
	} else {
		struct button_e b;
		b.button = button;
		b.shift = shift[player];
		b.player = player;
		fire_event("button", &b);
	}
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

int cfg_button(struct joykey *key, const char *cfgParam, int player)
{
	char *s;
	char *t;
	char *bstr = malloc_bstr(player);
	const char *cfg;

	cfg = cfg_get(bstr, cfgParam, NULL);
	if(cfg == NULL) {
		key->type = JK_UNSET;
		key->button = 0;
		key->axis = 0;
		free(bstr);
		return 1;
	}
	s = malloc(sizeof(char) * (strlen(cfg)+1));
	strcpy(s, cfg);
	t = s;
	key->type = atoi(s);
	s = next_dot(s);
	if(s == NULL)
	{
		ELog(("Invalid configuration string for '%s.%s'.\n", bstr, cfgParam));
		free(t);
		free(bstr);
		return 1;
	}
	key->button = atoi(s);
	s = next_dot(s);
	if(s == NULL)
	{
		ELog(("Invalid configuration string for '%s.%s'.\n", bstr, cfgParam));
		free(t);
		free(bstr);
		return 1;
	}
	key->axis = atoi(s);
	free(t);
	free(bstr);
	return 0;
}

int joykey_equal(const struct joykey *a, const struct joykey *b)
{
	if(a->type == b->type && a->button == b->button && a->axis == b->axis)
		return 1;
	return 0;
}

void key_handler(struct joykey *jk)
{
	if(jk->button == SDLK_ESCAPE)
		button_event(B_MENU, 0);

	else if(jk->button == SDLK_UP)
		button_event(B_UP, 0);

	else if(jk->button == SDLK_DOWN)
		button_event(B_DOWN, 0);

	else if(jk->button == SDLK_RIGHT)
		button_event(B_RIGHT, 0);

	else if(jk->button == SDLK_LEFT)
		button_event(B_LEFT, 0);

	else if(jk->button == SDLK_RETURN)
		fire_event("enter", &shift[0]);

	else if (jk->button == SDLK_TAB)
		button_event(B_SELECT, 0);

	else if(jk->button == SDLK_PAGEUP)
		fire_event("pageup", NULL);

	else if(jk->button == SDLK_PAGEDOWN)
		fire_event("pagedown", NULL);

	else if(jk->button == SDLK_HOME)
		fire_event("home", NULL);

	else if(jk->button == SDLK_END)
		fire_event("end", NULL);
	/* no else cuz all other keys are ignored :) */
}

void mouse_button_handler(struct joykey *jk)
{
	if(jk) {}
	/* All mouse buttons activate the menu */
	fire_event("enter", &shift[0]);
}

void joy_button_handler(struct joykey *jk)
{
	if(jk) {}
	/* All joystick buttons activate the menu */
	fire_event("enter", &shift[0]);
}

void joy_axis_handler(struct joykey *jk)
{
	/* Handle up and down in menu mode
	 * (I think all vertical axi / axes / axisi are odd, maybe)
	 */
	if(jk->axis & 1) {
		if(jk->button == 1)
			button_event(B_DOWN, 0);
		if(jk->button == -1)
			button_event(B_UP, 0);
	} else {
		if(jk->button == 1)
			button_event(B_RIGHT, 0);
		if(jk->button == -1)
			button_event(B_LEFT, 0);
	}
}

void null_handler(struct joykey *jk)
{
	if(jk) {}
}

/** Frees up the memory used by the input module */
void quit_input(void)
{
	struct slist *t;

	slist_foreach(t, keys) {
		free(t->data);
	}
	slist_free(keys);
}
