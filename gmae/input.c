/*
   Marfitude
   Copyright (C) 2006 Mike Shal

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

/** Buffer size required to hold a string representing a 64 bit integer ==
 * 20 characters, +1 for a minus sign.
 */
#define INT_LEN_MAX 21

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
static int set_shift(const struct joykey *jk);
static int fire_joykey(const struct joykey *jk);
static int forbidden_button(const struct joykey *jk);
static void reset_key_repeats(void);
static const char *next_dot(const char *s);
static int cfg_button(struct joykey *key, const char *cfgParam, int player);
static int joykey_equal(const struct joykey *a, const struct joykey *b);
static void key_handler(struct joykey *);
static void mouse_button_handler(struct joykey *);
static void joy_button_handler(struct joykey *);
static void joy_axis_handler(struct joykey *);
static void null_handler(struct joykey *);
static void button_event(int button, int player);
static void deactive(const void *);

static int cur_mode = MENU;
static struct joykey buttons[MAX_PLAYERS][B_LAST];
static int shift[MAX_PLAYERS] = {0};
static const char *cfgMsg[B_LAST] = {
	"up",
	"down",
	"left",
	"right",
	"button1",
	"button2",
	"button3",
	"button4",
	"select",
	"shift",
	"menu"
};

static const char *bstr[MAX_PLAYERS] = {
	"buttons0",
	"buttons1",
	"buttons2",
	"buttons3"
};

/* The +1 is to allow a non-player color (the last element) */
static float pc[MAX_PLAYERS+1][4] = {
	{0.6, 0.6, 1.0, 1.0},
	{1.0, 0.3, 0.3, 1.0},
	{1.0, 1.0, 0.0, 1.0},
	{0.0, 1.0, 0.0, 1.0},
	{1.0, 0.0, 1.0, 1.0}
};

/** Clear out the SDL_Event queue */
void init_input(void)
{
	SDL_Event event;
	while(SDL_PollEvent(&event)) {}
	register_event("sdl re-init", deactive);
}

/** Frees up the memory used by the input module */
void quit_input(void)
{
	struct slist *t;

	slist_foreach(t, keys) {
		free(t->data);
	}
	slist_free(keys);
	deregister_event("sdl re-init", deactive);
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
				jk.type = event.jbutton.which;
				jk.button = event.jbutton.button;
				jk.axis = JK_BUTTON;
				special_menu_handler = joy_button_handler;
				action = 1;
				break;
			case SDL_JOYBUTTONUP:
				jk.type = event.jbutton.which;
				jk.button = event.jbutton.button;
				jk.axis = JK_BUTTON;
				special_menu_handler = joy_button_handler;
				action = 0;
				break;
			case SDL_JOYHATMOTION:
				/* Currently unsupported (obviously :), since
				 * none of my joysticks do hat events
				 */
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

int unset_button(int b, int player)
{
	struct joykey jk = {JK_UNSET, JK_UNSET, JK_UNSET};
	return set_button(b, &jk, player);
}

/** Set the game button @a b to be set to the joykey structure @a jk
 *
 * @retval 0 Success
 * @retval 1 @a b is an invalid button
 */
int set_button(int b, const struct joykey *jk, int player)
{
	char s[INT_LEN_MAX * 3 + 3]; /* +3 for 2 .'s and 1 \0 */
	if(b < 0 || b >= B_LAST) return 1;

	reset_key_repeats();
	sprintf(s, "%i.%i.%i", jk->type, jk->button, jk->axis);
	cfg_set(bstr[player], cfgMsg[b], s);
	buttons[player][b].type = jk->type;
	buttons[player][b].button = jk->button;
	buttons[player][b].axis = jk->axis;
	return 0;
}

/** Checks if @a jk is a keyboard joykey, and if its button is equal to the
 * input @a key.
 *
 * @retval 1 if they are equal
 * @retval 0 if they are not equal
 */
int joykey_keybd_equal(const struct joykey *jk, int key)
{
	if(jk->type == JK_KEYBOARD &&
			jk->button == key &&
			jk->axis == JK_BUTTON)
		return 1;
	return 0;
}

/** Gets the @a player's colors in a float array.
 *
 * @param player The player (0 .. MAX_PLAYERS). A value of MAX_PLAYERS is
 *               a fake player.
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
		kp->time_hit = timer_cur();
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
			if(kp->repeating && timer_cur() - kp->time_hit > 30) {
				kp->time_hit = timer_cur();
				fire = 1;
			} else if(!kp->repeating && timer_cur() - kp->time_hit > 500) {
				kp->time_hit = timer_cur();
				kp->repeating = 1;
				fire = 1;
			}
		}
	}

	/* If we're supposed to fire, and the joykey doesn't fire anything
	 * on it's own, and we're in the menu, then use the special menu
	 * handler to give default button settings.
	 */
	if(fire && fire_joykey(&kp->key) && cur_mode == MENU)
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
	int rc = 1;

	/* This if statement is weird: the shift key is only set if it's
	 * in MENU or GAME mode. So that's why the second if part doesn't
	 * match up with the rest.
	 */
	if(cur_mode == KEY) {
		fire_event("key", jk);
		rc = 0;
	} else if(set_shift(jk)) {
		rc = 0;
		/* shift is already set by set_shift */
	} else if((cur_mode == MENU && !forbidden_button(jk))
		  || cur_mode == GAME) {

		if(jk->button == SDLK_ESCAPE) {
			button_event(B_MENU, MAX_PLAYERS);
			return 0;
		}

		for(p=0; p<MAX_PLAYERS; p++)
			for(x=0; x<B_LAST; x++)
				if(joykey_equal(&buttons[p][x], jk)) {
					button_event(x, p);
					rc = 0;
				}
	}
	return rc;
}

/* Returns 1 if this joykey is forbidden to be overridden in the menu (as in,
 * it can be set by the user, but the user setting doesn't take effect in the
 * menu). Return 0 othersize.
 */
int forbidden_button(const struct joykey *jk)
{
	if((jk->type == JK_KEYBOARD && jk->axis == JK_BUTTON) &&
	   (jk->button == SDLK_UP ||
	    jk->button == SDLK_DOWN ||
	    jk->button == SDLK_LEFT ||
	    jk->button == SDLK_RIGHT ||
	    jk->button == SDLK_RETURN ||
	    jk->button == SDLK_TAB))
		return 1;
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
	if(player >= MAX_PLAYERS) {
		b.shift = 0;
	} else {
		b.shift = shift[player];
	}
	b.player = player;
	fire_event("button", &b);
}

/* finds the beginning of the next . number
 * in config file, key is a.b.c
 * so after atoi() gets a, next_dot returns "b.c" then we get b, etc. */
const char *next_dot(const char *s)
{
	if(s == NULL) return NULL;
	while(*s && *s != '.') s++;
	if(*s) return s+1;
	return NULL;
}

int cfg_button(struct joykey *key, const char *cfgParam, int player)
{
	const char *cfg;

	cfg = cfg_get(bstr[player], cfgParam, NULL);
	if(cfg == NULL) {
		key->type = JK_UNSET;
		key->button = 0;
		key->axis = 0;
		return 1;
	}
	key->type = atoi(cfg);
	cfg = next_dot(cfg);
	if(cfg == NULL)
	{
		ELog(("Invalid configuration string for '%s.%s'.\n",
		      bstr[player], cfgParam));
		return 1;
	}
	key->button = atoi(cfg);
	cfg = next_dot(cfg);
	if(cfg == NULL)
	{
		ELog(("Invalid configuration string for '%s.%s'.\n",
		      bstr[player], cfgParam));
		return 1;
	}
	key->axis = atoi(cfg);
	return 0;
}

int joykey_equal(const struct joykey *a, const struct joykey *b)
{
	if(a->type == b->type &&
	   a->button == b->button &&
	   a->axis == b->axis)
		return 1;
	return 0;
}

void key_handler(struct joykey *jk)
{
	if(jk->button == SDLK_UP)
		button_event(B_UP, MAX_PLAYERS);

	else if(jk->button == SDLK_DOWN)
		button_event(B_DOWN, MAX_PLAYERS);

	else if(jk->button == SDLK_RIGHT)
		button_event(B_RIGHT, MAX_PLAYERS);

	else if(jk->button == SDLK_LEFT)
		button_event(B_LEFT, MAX_PLAYERS);

	else if(jk->button == SDLK_RETURN)
		fire_event("enter", &shift[0]);

	else if (jk->button == SDLK_TAB)
		button_event(B_SELECT, MAX_PLAYERS);

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
	/* All joystick buttons do nothing. It would be cool if they could
	 * activate the menu, but some controllers (specifically, mine :) fire
	 * a joy button event *and* a joy axis event when using the directional
	 * pad, which causes all sorts of havoc in the menu. So, the joy buttons
	 * now always do nothing. This is favored over the previous solution
	 * of manually specifying which joy-buttons to ignore in the cfg file.
	 */
}

void joy_axis_handler(struct joykey *jk)
{
	if(jk) {}
	/* All joy axes (axises? axi?) do nothing. It would be cool if they
	 * could automatically move up/down or left/right in the menu, but
	 * there doesn't seem to be a standard as far as which is vertical and
	 * which is horizontal. Plus my dance axis buttons send two axis events,
	 * which is even more annoying :).
	 */
}

void null_handler(struct joykey *jk)
{
	if(jk) {}
}

/* Set all keys to be inactive when sdl reinitializes. */
void deactive(const void *data)
{
	const struct slist *t;

	if(data) {}
	slist_foreach(t, keys) {
		struct keypush *kp = t->data;
		kp->active = 0;
	}
}
