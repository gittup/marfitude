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

typedef struct Event {
	EventHandler handler;
	int stopHere;
	struct Event *next;
	} Event;

static char *NextDot(char *s);
static int CfgButton(JoyKey *key, const char *cfgParam);
static int KeyEqual(JoyKey *key, SDL_KeyboardEvent *e);
static int JoyButtonEqual(JoyKey *key, SDL_JoyButtonEvent *e);
static int JoyAxisEqual(JoyKey *key, SDL_JoyAxisEvent *e);
static void KeyDownEvent(SDL_KeyboardEvent *e);
static void JoyButtonDownEvent(SDL_JoyButtonEvent *e);
static void JoyAxisEvent(SDL_JoyAxisEvent *e);

int eventMode = MENU;
Event *events[EVENT_LAST] = {0};
KeyHandler keyHandler;
JoyKey buttons[B_LAST] = {{0,0,0}};
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

int SetButton(int b, JoyKey *jk)
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

int CfgButton(JoyKey *key, const char *cfgParam)
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
	char *s = NULL;
	JoyKey *jk = buttons+button;
	if(jk->type == JK_KEYBOARD)
	{
		char *t;
		t = SDL_GetKeyName(jk->button);
		s = (char*)malloc(sizeof(char) * (strlen(t)+1));
		strcpy(s, t);
	}
	else /* joystick */
	{
		int len;
		if(jk->axis == JK_JOYBUTTON) /* button event */
		{
			len = IntLen(jk->type) + IntLen(jk->button) + 13;
			s = (char*)malloc(sizeof(char) * (len+1));
			sprintf(s, "Joy %i Button %i", jk->type, jk->button);
		}
		else /* axis event */
		{
			len = IntLen(jk->type) + IntLen(jk->button) + 15;
			s = (char*)malloc(sizeof(char) * (len+1));
			sprintf(s, "Joy %i Axis %i (%c)", jk->type, jk->axis, (jk->button > 0) ? '+' : '-');
		}
	}
	return s;
}

int KeyEqual(JoyKey *key, SDL_KeyboardEvent *e)
{
	if(	key->type == -1 &&
		key->button == (signed)e->keysym.sym &&
		key->axis == -1)
		return 1;
	return 0;
}

int JoyButtonEqual(JoyKey *key, SDL_JoyButtonEvent *e)
{
	if(	key->type == e->which &&
		key->button == e->button &&
		key->axis == -1)
		return 1;
	return 0;
}

int JoyAxisEqual(JoyKey *key, SDL_JoyAxisEvent *e)
{
	if(	key->type == e->which &&
		key->button == (e->value > JOY_THRESHOLD ? 1 : e->value < -JOY_THRESHOLD ? -1 : 0) &&
		key->axis == e->axis)
		return 1;
	return 0;
}

void FireEvent(int event)
{
	Event *e;
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
	Event *e;
	e = (Event *)malloc(sizeof(Event));
	e->handler = handler;
	e->stopHere = stopHere;
	e->next = events[event];
	events[event] = e;
	return 1;
}

void DeregisterEvent(int event, EventHandler handler)
{
	Event *e;
	Event *prev = NULL;
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
	JoyKey jk;
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

void JoyButtonDownEvent(SDL_JoyButtonEvent *e)
{
	int x;
	JoyKey jk;
	if(eventMode == KEY && keyHandler)
	{
		jk.type = e->which;
		jk.button = e->button;
		jk.axis = JK_JOYBUTTON;
		keyHandler(&jk);
	}
	else if(eventMode == MENU)
	{
		/* all buttons activate in menu mode */
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
	JoyKey jk;
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
		/* Only handle up and down in menu mode */
		if(e->axis == 1)
		{
			if(e->value > JOY_THRESHOLD)
			{
				FireEvent(EVENT_DOWN);
			}
			if(e->value < -JOY_THRESHOLD)
			{
				FireEvent(EVENT_UP);
			}
			/* wait for event.jaxis.value to be less than JOY_THRESHOLD before executing menu again? */
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
			case SDL_QUIT:
				quit = 1;
			default:
				break;
		}
	}
	return;
}
