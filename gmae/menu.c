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

#include "SDL_opengl.h"

#include "menu.h"
#include "main.h"
#include "event.h"
#include "glfunc.h"
#include "input.h"
#include "phys.h"
#include "textures.h"
#include "scene.h"
#include "sounds.h"
#include "cfg.h"
#include "log.h"

#include "util/memtest.h"
#include "util/fatalerror.h"
#include "util/flist.h"
#include "util/myrand.h"
#include "util/slist.h"
#include "util/strfunc.h"

/** A slider menu item. Can select between a range of values. */
#define MENU_SLIDER 0
/** A button menu item. Can be activated to call a function */
#define MENU_BUTTON 1
/** A button param menu item. Can be activated to call a function with a
 * parameter.
 */
#define MENU_BUTTONPARAM 2
/** A test menu item. Just writes text somewhere */
#define MENU_TEXT 3
/** Items from 0 to MENU_SELECTABLE are selectable */
#define MENU_SELECTABLE 3
/** Where music files are located */
#define MUSICDIR "music/"
/** Where scenes are located */
#define SCENEDIR "scenes/"

/** For the bounding box */
#define NOBOX -1
/** Bounding box offset */
#define BBO 5

/** Minimum spacing between the name and value (for slider, boolean, etc) */
#define MENU_SPACING 3

/** @file
 * Draws a bunch of different menus and allows switching between them.
 */

static struct menu *activeMenu = NULL;

/** A slider menu object */
struct slider {
	int min; /**< The minimum value */
	int max; /**< The maximum value */
	int del; /**< The delta */
	int val; /**< The actual value */
	int namelen; /**< The length of the name of the slider */
	char **strs; /**< Optional list of strings describing the values */
	float c[4]; /**< The color of the slider object */
	void (*handler)(void); /**< Handler to call on change */
};

/** A button menu object */
struct button {
	void (*activeFunc)(int); /**< The function to execute when activated */
	int sound_enabled;       /**< If a sound should play when activated */
};

/** A button menu object that takes a parameter */
struct buttonParam {
	int (*activeFunc)(int); /**< The function to execute when activated */
	int param;              /**< The parameter to send the function */
	int xoffset;            /**< Move the param left or right */
};

/** A text menu object */
struct text {
	float c[4]; /**< The color of the object */
	int x;      /**< The x position */
	int y;      /**< The y position */
	int active; /**< true if the text is to be displayed*/
};

/** A menu item */
struct menuItem {
	int type;   /**< One of the MENU_ defines above */
	char *name; /**< The name of this menu item */
	void *item; /**< Pointer to one of the item structs above */
};

/** Describes a menu box on the screen */
struct screenMenu {
	int activeMenuItem;       /**< The menu item that is highlighted */
	struct menuItem *items;   /**< Pointers to item elements */
	int numItems;             /**< The number of items in the menu */
	int itemStart;            /**< The item to start displaying */
	int menuSize;             /**< The number of items displayed at once */
	int (*BoundsCheck)(struct screenMenu *m); /**< A bounds checking
						   * function - returns 1 if a
						   * sound is to be played
						   */
	int menuX; /**< X position of the menu */
	int menuY; /**< Y position of the menu */
	int minX;  /**< Left side of the bounding box */
	int minY;  /**< Top side of the bounding box */
	int maxX;  /**< Right side of the bounding box */
	int maxY;  /**< Bottom side of the bounding box */
};

static void ShadedBox(int x1, int y1, int x2, int y2, int fade);
static void DrawPartialMenu(struct screenMenu *m, int start, int stop);
static void active_color(int item, struct screenMenu *m);
static void DrawMenu(struct screenMenu *m);
static void AddMenuItem(struct screenMenu *m, const char *name, void *item, int type);
static void ClearMenuItems(struct screenMenu *m);
static void UpdateBox(struct screenMenu *m, int x1, int y1, int x2, int y2);
static int clip_slider_val(struct slider *s);
static void name_slider_item(struct screenMenu *m, struct slider *s, int i, const char *name);
static struct slider *CreateSlider(struct screenMenu *m, const char *name, const float *c, int min, int max, int delta, int initVal);
static struct slider *CreateBoolean(struct screenMenu *m, const char *name, float *c, const char *trueString, const char *falseString, int initVal);
static struct button *CreateButton(struct screenMenu *m, const char *name, void (*activeFunc)(int));
static struct buttonParam *CreateButtonParam(struct screenMenu *m, const char *name, int (*activeFunc)(int), int param, int xoff);
static struct text *CreateText(struct screenMenu *m, const char *name, float *c, int x, int y);
static int MenuClamp(struct screenMenu *m);
static int MenuWrap(struct screenMenu *m);
static int DownOne(struct screenMenu *m);
static int UpOne(struct screenMenu *m);
static void MenuUp(int);
static void MenuDown(int);
static void MenuDec(int);
static void MenuInc(int);
static void MenuActivate(const void *);
static void MenuSelect(void);
static int valid_music_file(const char *s);
static char *scene_convert(const char *s);
static int alphabetical(const void *a, const void *b);
static int FightMenuInit(void);
static void FightMenuQuit(void);
static void EQTriangle(int fade);
static void FightMenu(void);
static int option_menu_init(void);
static void option_menu_quit(void);
static void option_menu(void);
static void MenuBack(int);
static void RegisterMenuEvents(void);
static void ShowMenu(const void *);
static void HideMenu(void);
static int NullMenuInit(void);
static void NullMenuQuit(void);
static void NullMenu(void);
static int NoMenuInit(void);
static void NoMenuQuit(void);
static void NoMenu(void);
static void Retry(int);
static int MainMenuInit(void);
static void MainMenuQuit(void);
static void MainMenu(void);
static void FightActivate(int);
static void FightPageUp(const void *);
static void FightPageDown(const void *);
static void FightHome(const void *);
static void FightEnd(const void *);
static int FindActiveItem(struct menuItem *activeItems, int numActiveItems);
/*static void DrawButton(GLuint button, int x, int y, int on);*/
static void ConfigChangeHandler(void);
static void ConfigCreateItems(void);
static void ConfigKeyHandler(const void *data);
static int ConfigButton(int b);
static int ConfigMenuInit(void);
static void ConfigMenuQuit(void);
static void ConfigMenu(void);
static void Quit(int);
static int QuitMenuInit(void);
static void QuitMenuQuit(void);
static void QuitMenu(void);
static void button_handler(const void *data);
static void null_handler(void);

/** Set to 1 if a menu is active, 0 otherwise */
static int menuActive = 0;

static int curMenu;
static int numScreenMenus = 1;
static struct screenMenu screenMenus[2];
static struct screenMenu *mainMenu = &screenMenus[0];
static int cur_player = 0;

static int snd_tick;
static int snd_push;
static int snd_back;

void button_handler(const void *data)
{
	const struct button_e *b = data;
	switch(b->button) {
		case B_UP:
			MenuUp(b->shift);
			break;
		case B_DOWN:
			MenuDown(b->shift);
			break;
		case B_LEFT:
			MenuDec(b->shift);
			break;
		case B_RIGHT:
			MenuInc(b->shift);
			break;
		case B_MENU:
			MenuBack(0);
			break;
		case B_SELECT:
			MenuSelect();
			break;
		default:
			break;
	}
}

void ShadedBox(int x1, int y1, int x2, int y2, int fade)
{
	set_ortho_projection();
	glColor4f(0.3, 0.3, 0.3, 0.5);

	glDisable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	{
		glVertex2i(x1, y1);
		glVertex2i(x1, y2);
		glVertex2i(x2, y2);
		glVertex2i(x2, y1);
	} glEnd();

	if(fade)
		glColor4f(0.58, 0.58, 0.58, 1.0);
	else
		glColor4f(1.0, 1.0, 1.0, 1.0);
	glBegin(GL_LINE_LOOP);
	{
		glVertex2i(x1, y1);
		glVertex2i(x1, y2);
		glVertex2i(x2, y2);
		glVertex2i(x2, y1);
	} glEnd();
	glEnable(GL_TEXTURE_2D);
	reset_projection();
}

void EQTriangle(int fade)
{
	glDisable(GL_TEXTURE_2D);

	glColor4f(0.3, 0.3, 0.3, 0.5);
	glTranslated(0.0, 0.0, -0.3);
	glBegin(GL_TRIANGLES); {
		glVertex3f(-1.0, 0.0, 0.0);
		glVertex3f(1.0, 0.0, 0.0);
		glVertex3f(0.0, 1.732050807, 0.0);
	} glEnd();

	if(fade)
		glColor4f(0.58, 0.58, 0.58, 1.0);
	else
		glColor4f(1.0, 1.0, 1.0, 1.0);
	glTranslatef(0.0, 0.0, 0.3);
	glBegin(GL_LINE_LOOP); {
		glVertex3f(-1.0, 0.0, 0.0);
		glVertex3f(1.0, 0.0, 0.0);
		glVertex3f(0.0, 1.732050807, 0.0);
	} glEnd();

	glEnable(GL_TEXTURE_2D);
}

void active_color(int item, struct screenMenu *m)
{
	if(item == m->activeMenuItem) {
		if(m == &screenMenus[curMenu])
			glColor3f(1.0, 0.0, 0.0);
		else
			glColor3f(0.58, 0.0, 0.0);
	}
	else {
		if(m == &screenMenus[curMenu])
			glColor3f(1.0, 1.0, 1.0);
		else
			glColor3f(0.58, 0.58, 0.58);
	}
}

void DrawPartialMenu(struct screenMenu *m, int start, int stop)
{
	int i;
	int x, y;
	int bottom;
	struct text *t;
	struct slider *s;
	struct buttonParam *bp;

	if(m->menuSize == -1) {
		bottom = m->maxY;
	} else {
		i = m->minY + FONT_HEIGHT * m->menuSize;
		bottom = (m->maxY < i) ? m->maxY : i;
	}
	ShadedBox(m->minX-BBO, m->minY-BBO, m->maxX+BBO, bottom + BBO, m != &screenMenus[curMenu]);
	if(start < 0 || start >= m->numItems) start = 0;
	if(stop < 0 || stop >= m->numItems) stop = m->numItems;
	for(i=start;i<stop;i++)
	{
		x = m->menuX;
		y = m->menuY + (i-start) * FONT_HEIGHT;
		switch(m->items[i].type)
		{
			case MENU_SLIDER:
				s = (struct slider*)m->items[i].item;
				glColor4fv(s->c);
				print_gl(x, y, m->items[i].name);
				active_color(i, m);
				if(s->val <= s->max && s->val >= s->min &&
						s->strs[s->val] != NULL)
					print_gl(m->maxX - strlen(s->strs[s->val]) * FONT_WIDTH, y, s->strs[s->val]);
				else
					print_gl(m->maxX - int_len(s->val) * FONT_WIDTH, y, "%i", s->val);
				break;
			case MENU_BUTTON:
				active_color(i, m);
				print_gl(x, y, m->items[i].name);
				break;
			case MENU_BUTTONPARAM:
				bp = (struct buttonParam*)m->items[i].item;
				active_color(i, m);
				print_gl(x + bp->xoffset, y, m->items[i].name);
				break;
			case MENU_TEXT:
				t = (struct text*)m->items[i].item;
				if(t->active)
				{
					glColor4fv(t->c);
					print_gl(t->x, t->y, m->items[i].name);
				}
		}
	}

	glColor3f(1.0, 1.0, 1.0);
	set_ortho_projection();
	if(m->itemStart)
	{
		glPushMatrix();
			glTranslatef(m->menuX-20, m->menuY, 0.0);
			glScalef(10.0, -10.0, 1.0);
			EQTriangle(m != &screenMenus[curMenu]);
		glPopMatrix();
	}
	if(m->menuSize != -1 && m->itemStart < m->numItems - m->menuSize)
	{
		int tmp = m->minY + FONT_HEIGHT * m->menuSize;
		glPushMatrix();
			glTranslatef(m->menuX-20, tmp, 0.0);
			glScalef(10.0, 10.0, 1.0);
			EQTriangle(m != &screenMenus[curMenu]);
		glPopMatrix();
	}
	reset_projection();
}

void DrawMenu(struct screenMenu *m)
{
	DrawPartialMenu(m, 0, m->numItems);
}

void AddMenuItem(struct screenMenu *m, const char *name, void *item, int type)
{
	m->items = (struct menuItem*)realloc(m->items, sizeof(struct menuItem) * (m->numItems+1));
	m->items[m->numItems].name = malloc(sizeof(char) * (strlen(name)+1));
	strcpy(m->items[m->numItems].name, name);
	m->items[m->numItems].item = item;
	m->items[m->numItems].type = type;
	m->numItems++;
}

void ClearMenuItems(struct screenMenu *m)
{
	int x;
	int y;
	struct slider *s;
	Log(("Clearing items...\n"));
	m->minX = NOBOX;
	m->minY = 0;
	m->maxX = 0;
	m->maxY = 0;
	for(x=0;x<m->numItems;x++)
	{
		free(m->items[x].name);
		switch(m->items[x].type)
		{
			case MENU_SLIDER:
				s = (struct slider*)m->items[x].item;
				for(y=0; y<=s->max; y++) {
					free(s->strs[y]);
				}
				free(s->strs);
				free(s);
				break;
			case MENU_BUTTON:
			case MENU_BUTTONPARAM:
			case MENU_TEXT:
				free(m->items[x].item);
				break;
			default:
				ELog(("Error: Invalid menu item type!\n"));
				break;
		}
	}
	m->numItems = 0;
	m->activeMenuItem = 0;
	free(m->items);
	m->items = NULL;
	Log(("All items freed.\n"));
}

void UpdateBox(struct screenMenu *m, int x1, int y1, int x2, int y2)
{
	if(m->minX == NOBOX)
	{
		m->minX = x1;
		m->minY = y1;
		m->maxX = x2;
		m->maxY = y2;
	}
	else
	{
		if(x1 < m->minX) m->minX = x1;
		if(y1 < m->minY) m->minY = y1;
		if(x2 > m->maxX) m->maxX = x2;
		if(y2 > m->maxY) m->maxY = y2;
	}
}

int clip_slider_val(struct slider *s)
{
	if(s->val < s->min) {
		s->val = s->min;
		return 0;
	}
	if(s->val > s->max) {
		s->val = s->max;
		return 0;
	}
	return 1;
}

void name_slider_item(struct screenMenu *m, struct slider *s, int i, const char *name)
{
	int len;

	if(i < s->min || i > s->max) {
		ELog(("Slider item '%s' for index %i not in range of %i-%i\n",
					name, i, s->min, s->max));
		return;
	}
	s->strs[i] = string_copy(name);
	len = s->namelen + strlen(name) + MENU_SPACING;
	UpdateBox(m, m->minX, m->minY, m->menuX + len * FONT_WIDTH, m->maxY);
}

struct slider *CreateSlider(struct screenMenu *m, const char *name, const float *c, int min, int max, int delta, int initVal)
{
	struct slider *s;
	int x;
	int len;

	s = malloc(sizeof(struct slider));
	s->min = min;
	s->max = max;
	s->del = delta;
	if(initVal < min)
		initVal = min;
	if(initVal > max)
		initVal = max;
	s->val = initVal;
	s->c[RED] = c[RED];
	s->c[GREEN] = c[GREEN];
	s->c[BLUE] = c[BLUE];
	s->c[ALPHA] = c[ALPHA];
	s->namelen = strlen(name);
	s->handler = null_handler;
	s->strs = (char**)malloc(sizeof(char*) * max + 1);
	for(x=0; x<=max; x++) {
		s->strs[x] = NULL;
	}

	len = s->namelen + int_len(max) + MENU_SPACING;
	UpdateBox(m, m->menuX, m->menuY + FONT_HEIGHT * m->numItems, m->menuX + len * FONT_WIDTH, m->menuY + FONT_HEIGHT * (m->numItems+1));
	AddMenuItem(m, name, (void*)s, MENU_SLIDER);
	return s;
}

struct slider *CreateBoolean(struct screenMenu *m, const char *name, float *c, const char *trueString, const char *falseString, int initVal)
{
	struct slider *s;

	s = CreateSlider(m, name, c, 0, 1, 1, initVal);
	name_slider_item(m, s, 0, falseString);
	name_slider_item(m, s, 1, trueString);

	return s;
}

struct button *CreateButton(struct screenMenu *m, const char *name, void (*activeFunc)(int))
{
	struct button *b;
	b = malloc(sizeof(struct button));
	b->activeFunc = activeFunc;
	b->sound_enabled = 1;
	UpdateBox(m, m->menuX, m->menuY + FONT_HEIGHT * m->numItems, m->menuX + strlen(name) * FONT_WIDTH, m->menuY + FONT_HEIGHT * (m->numItems+1));
	AddMenuItem(m, name, (void*)b, MENU_BUTTON);
	return b;
}

struct buttonParam *CreateButtonParam(struct screenMenu *m, const char *name, int (*activeFunc)(int), int param, int xoff)
{
	struct buttonParam *b;
	b = malloc(sizeof(struct buttonParam));
	b->activeFunc = activeFunc;
	b->param = param;
	b->xoffset = xoff;
	UpdateBox(m, m->menuX + xoff, m->menuY + FONT_HEIGHT * m->numItems, m->menuX + strlen(name) * FONT_WIDTH + xoff, m->menuY + FONT_HEIGHT * (m->numItems+1));
	AddMenuItem(m, name, (void*)b, MENU_BUTTONPARAM);
	return b;
}

/* assumes text is on a single line for bounding box purposes */
struct text *CreateText(struct screenMenu *m, const char *name, float *c, int x, int y)
{
	struct text *t;
	t = malloc(sizeof(struct text));
	t->c[RED] = c[RED];
	t->c[GREEN] = c[GREEN];
	t->c[BLUE] = c[BLUE];
	t->c[ALPHA] = c[ALPHA];
	t->x = x;
	t->y = y;
	t->active = 1;
	UpdateBox(m, x, y, x + strlen(name) * FONT_WIDTH, y + FONT_HEIGHT);
	AddMenuItem(m, name, (void*)t, MENU_TEXT);
	return t;
}

int MenuClamp(struct screenMenu *m)
{
	if(m->activeMenuItem < 0)
	{
		m->activeMenuItem = 0;
		return 0;
	}
	if(m->activeMenuItem >= m->numItems)
	{
		m->activeMenuItem = m->numItems >= 0 ? m->numItems - 1 : 0;
		return 0;
	}
	return 1;
}

int MenuWrap(struct screenMenu *m)
{
	if(m->activeMenuItem >= m->numItems) m->activeMenuItem = 0;
	else if(m->activeMenuItem < 0) m->activeMenuItem = m->numItems >= 0 ? m->numItems -1 : 0;
	return 1;
}

int DownOne(struct screenMenu *m)
{
	m->activeMenuItem++;
	if(m->activeMenuItem >= m->numItems)
	{
		if(m->BoundsCheck(m)) return 1;
		else return 0;
	}
	else return 1;
}

void MenuDown(int shift)
{
	struct screenMenu *m = &screenMenus[curMenu];
	int old = m->activeMenuItem;
	int play = 0;
	int num = (shift && m->BoundsCheck == MenuClamp) ? m->menuSize : 1;

	do {
		if(DownOne(m)) {
			if(m->items[m->activeMenuItem].type < MENU_SELECTABLE) {
				play++;
				if(play == num)
					break;
			}
		}
		else break;
	} while(m->activeMenuItem != old);
	if(play) MPlaySound(snd_tick);
}

int UpOne(struct screenMenu *m)
{
	m->activeMenuItem--;
	if(m->activeMenuItem < 0)
	{
		if(m->BoundsCheck(m)) return 1;
		else return 0;
	}
	else return 1;
}

void MenuUp(int shift)
{
	struct screenMenu *m = &screenMenus[curMenu];
	int old = m->activeMenuItem;
	int play = 0;
	int num = (shift && m->BoundsCheck == MenuClamp) ? m->menuSize : 1;

	do {
		if(UpOne(m)) {
			if(m->items[m->activeMenuItem].type < MENU_SELECTABLE) {
				play++;
				if(play == num)
					break;
			}
		}
		else break;
	} while(m->activeMenuItem != old);
	if(play) MPlaySound(snd_tick);
}

void MenuDec(int shift)
{
	struct screenMenu *m = &screenMenus[curMenu];
	struct menuItem *i = &m->items[m->activeMenuItem];
	struct slider *s;
	int play = 0;

	switch(i->type) {
		case MENU_SLIDER:
			s = (struct slider*)i->item;
			if(shift) {
				if(s->val != s->min) {
					s->val = s->min;
					s->handler();
					play = 1;
				}
			} else {
				s->val--;
				if(clip_slider_val(s)) {
					s->handler();
					play = 1;
				}
			}
			break;
	}
	if(play) MPlaySound(snd_tick);
}

void MenuInc(int shift)
{
	struct screenMenu *m = &screenMenus[curMenu];
	struct menuItem *i = &m->items[m->activeMenuItem];
	struct slider *s;
	int play = 0;

	switch(i->type) {
		case MENU_SLIDER:
			s = (struct slider*)i->item;
			if(shift) {
				if(s->val != s->max) {
					s->val = s->max;
					s->handler();
					play = 1;
				}
			} else {
				s->val++;
				if(clip_slider_val(s)) {
					s->handler();
					play = 1;
				}
			}
			break;
	}
	if(play)
		MPlaySound(snd_tick);
}

void MenuActivate(const void *data)
{
	struct buttonParam *bp;
	struct button *b;
	struct screenMenu *m = &screenMenus[curMenu];
	int shift = *(const int*)data;

	switch(m->items[m->activeMenuItem].type)
	{
		case MENU_BUTTON:
			b = m->items[m->activeMenuItem].item;
			if(b->sound_enabled) MPlaySound(snd_push);
			b->activeFunc(shift);
			break;
		case MENU_BUTTONPARAM:
			bp = m->items[m->activeMenuItem].item;
			MPlaySound(snd_push);
			bp->activeFunc(bp->param);
		default:
			break;
	}
}

void MenuSelect(void)
{
	int oldMenu = curMenu;

	curMenu = (curMenu + 1) % numScreenMenus;
	if(oldMenu != curMenu)
		MPlaySound(snd_tick);
}

void MenuBack(int shift)
{
	if(shift) {}
	MPlaySound(snd_back);
	switch_menu(activeMenu->back);
}

void RegisterMenuEvents(void)
{
	menuActive = 1;
	curMenu = 0;
	register_event("button", button_handler);
	register_event("enter", MenuActivate);
}

void ShowMenu(const void *data)
{
	struct menu_e m;
	const struct button_e *b = data;

	if(menuActive) return;
	if(b->button != B_MENU) return;

	RegisterMenuEvents();
	MPlaySound(snd_push);
	m.active = 1;
	fire_event("menu", &m);
	switch_menu(MAINMENU);
}

void HideMenu(void)
{
	struct menu_e m;
	if(!menuActive) return;
	menuActive = 0;
	deregister_event("enter", MenuActivate);
	deregister_event("button", button_handler);
	m.active = 0;
	fire_event("menu", &m);
}

int NullMenuInit(void)
{
	HideMenu(); /* deregister events for good measure */
	return 0;
}

void NullMenuQuit(void)
{
}

void NullMenu(void)
{
}

int NoMenuInit(void)
{
	input_mode(GAME);
	HideMenu();
	register_event("button", ShowMenu);
	return 0;
}

void NoMenuQuit(void)
{
	deregister_event("button", ShowMenu);
}

void NoMenu(void)
{
}

void Retry(int shift)
{
	if(shift) {}
	switch_menu(NOMENU);
	Log(("Retry switch_scene\n"));
	switch_scene(MAINSCENE);
	Log(("Retry Scene Switched\n"));
}

int MainMenuInit(void)
{
	if(!menuActive) RegisterMenuEvents();
	input_mode(MENU);
	CreateButtonParam(mainMenu, "Fight", switch_menu, FIGHTMENU, 0);
	if(is_scene_active(MAINSCENE)) CreateButton(mainMenu, "Retry", Retry);
	CreateButtonParam(mainMenu, "Configure", switch_menu, CONFIGMENU, 0);
	CreateButtonParam(mainMenu, "Options", switch_menu, OPTIONMENU, 0);
	CreateButtonParam(mainMenu, "Quit", switch_menu, QUITMENU, 0);
	return 0;
}

void MainMenuQuit(void)
{
	ClearMenuItems(mainMenu);
}

void MainMenu(void)
{
	set_ortho_projection();
	glLoadIdentity();

	glBindTexture(GL_TEXTURE_2D, texture_num("Title.png"));
	glDisable(GL_LIGHTING);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0); glVertex2i(0, 0);
	glTexCoord2f(1.0, 0.0); glVertex2i(512, 0);
	glTexCoord2f(1.0, 1.0); glVertex2i(512, 256);
	glTexCoord2f(0.0, 1.0); glVertex2i(0, 256);
	glEnd();
	glEnable(GL_LIGHTING);

	DrawMenu(mainMenu);

	reset_projection();
}

static struct slist *fileList;
static struct slist *sceneList;
static struct screenMenu *fightSceneSelect = &screenMenus[1];

void FightActivate(int shift)
{
	char *song;
	char *scene;

	if(shift) {
		struct screenMenu *m = &screenMenus[curMenu];
		m->activeMenuItem = rand_int(m->numItems);
		return;
	}

	song = cat_str(MUSICDIR, (char*)slist_nth(fileList, mainMenu->activeMenuItem)->data);
	scene = cat_str(SCENEDIR, (char*)slist_nth(sceneList, fightSceneSelect->activeMenuItem)->data);

	printf("Activating %i, %i\n", mainMenu->activeMenuItem, fightSceneSelect->activeMenuItem);
	cfg_set("main", "song", song);
	cfg_set("main", "scene", scene);
	free(song);
	free(scene);
	switch_menu(NOMENU);
	Log(("FightActivate switch_scene\n"));
	switch_scene(MAINSCENE);
	Log(("FightActivate SceneSwitched\n"));
}

void FightPageUp(const void *data)
{
	struct screenMenu *m = &screenMenus[curMenu];
	int oldItem = m->activeMenuItem;

	if(data) {}
	m->activeMenuItem -= m->menuSize;
	MenuClamp(m);
	if(m->activeMenuItem != oldItem)
		MPlaySound(snd_tick);
}

void FightPageDown(const void *data)
{
	struct screenMenu *m = &screenMenus[curMenu];
	int oldItem = m->activeMenuItem;

	if(data) {}
	m->activeMenuItem += m->menuSize;
	MenuClamp(m);
	if(m->activeMenuItem != oldItem)
		MPlaySound(snd_tick);
}

void FightHome(const void *data)
{
	struct screenMenu *m = &screenMenus[curMenu];

	if(data) {}
	if(m->activeMenuItem) {
		m->activeMenuItem = 0;
		MPlaySound(snd_tick);
	}
}

void FightEnd(const void *data)
{
	struct screenMenu *m = &screenMenus[curMenu];

	if(data) {}
	if(m->activeMenuItem < (signed)slist_length(fileList) - 1) {
		m->activeMenuItem = (signed)slist_length(fileList) - 1;
		MPlaySound(snd_tick);
	}
}

int valid_music_file(const char *s)
{
	if(s[0] == '.') return 0;
	while(*s)
	{
		if(*s == '.') return 1;
		s++;
	}
	return 0;
}

char *scene_convert(const char *s)
{
	char *t;
	char *p;
	t = string_copy(s);
	p = t;
	while(*p) {
		if(*p == '.') {
			*p = 0;
			break;
		}
		p++;
	}
	return t;
}

int alphabetical(const void *a, const void *b)
{
	const char *s1 = a;
	const char *s2 = b;
	while(*s1 && *s2 && *s1 == *s2) {
		s1++;
		s2++;
	}
	if(*s1 == *s2)
		return 0;
	if(*s1 == 0)
		return -1;
	if(*s2 == 0)
		return 1;
	return (*s1 < *s2) ? -1 : 1;
}

int FightMenuInit(void)
{
	struct flist f;
	const char *lastFile;
	int cnt;
	int len;
	struct slist *tmp;

	if(!menuActive) RegisterMenuEvents();
	mainMenu->menuX = 200;
	mainMenu->menuY = 150;
	mainMenu->BoundsCheck = MenuClamp;
	mainMenu->menuSize = 11;
	fileList = NULL;
	sceneList = NULL;

	numScreenMenus = 2;

	fightSceneSelect->menuX = 200;
	fightSceneSelect->menuY = 350;
	fightSceneSelect->BoundsCheck = MenuClamp;
	fightSceneSelect->activeMenuItem = 0;
	fightSceneSelect->items = NULL;
	fightSceneSelect->numItems = 0;
	fightSceneSelect->menuSize = 5;
	fightSceneSelect->minX = NOBOX;
	fightSceneSelect->minY = 0;
	fightSceneSelect->maxX = 0;
	fightSceneSelect->maxY = 0;

	flist_foreach(&f, SCENEDIR) {
		char *s;
		if(valid_music_file(f.filename)) {
			s = scene_convert(f.filename);
			sceneList = slist_insert_sorted(sceneList, s, alphabetical);
		}
	}
	sceneList = slist_insert(sceneList, string_copy("default"));

	flist_foreach(&f, MUSICDIR) {
		if(valid_music_file(f.filename)) {
			char *s;
			s = string_copy(f.filename);
			fileList = slist_insert_sorted(fileList, s, alphabetical);
		}
	}
	if(slist_length(fileList) == 0) {
		float c[4] = {0.0, 1.0, 1.0, 1.0};
		Error("Generating playlist");
		CreateText(mainMenu, "Download MODs from", c, 200, 200);
		CreateText(mainMenu, " http://www.modarchive.com !", c, 200, 200 + FONT_HEIGHT);
		CreateText(mainMenu, "For more info go to ", c, 200, 200 + FONT_HEIGHT * 3);
		CreateText(mainMenu, " http://www.erestar.net/games/marfitude", c, 200, 200 + FONT_HEIGHT * 4);
	}

	lastFile = cfg_get("main", "song");
	if(!lastFile)
		lastFile = "null";
	cnt = 0;
	len = slist_length(fileList);
	slist_foreach(tmp, fileList) {
		char *t = cat_str(MUSICDIR, (char*)tmp->data);
		if(strcmp(t, lastFile) == 0) {
			mainMenu->itemStart = cnt - mainMenu->menuSize/2;
			while(mainMenu->itemStart > len - mainMenu->menuSize)
				mainMenu->itemStart--;
			while(mainMenu->itemStart < 0)
				mainMenu->itemStart++;
			mainMenu->activeMenuItem = cnt;
		}
		free(t);
		CreateButton(mainMenu, tmp->data, FightActivate);
		cnt++;
	}

	lastFile = cfg_get("main", "scene");
	if(!lastFile)
		lastFile = "null";
	cnt = 0;
	len = slist_length(sceneList);
	slist_foreach(tmp, sceneList) {
		char *t = cat_str(SCENEDIR, (char*)tmp->data);
		if(strcmp(t, lastFile) == 0) {
			fightSceneSelect->itemStart = cnt - fightSceneSelect->menuSize/2;
			while(fightSceneSelect->itemStart > len - fightSceneSelect->menuSize)
				fightSceneSelect->itemStart--;
			while(fightSceneSelect->itemStart < 0)
				fightSceneSelect->itemStart++;
			fightSceneSelect->activeMenuItem = cnt;
		}
		free(t);
		CreateButton(fightSceneSelect, tmp->data, FightActivate);
		cnt++;
	}

	register_event("pageup", FightPageUp);
	register_event("pagedown", FightPageDown);
	register_event("home", FightHome);
	register_event("end", FightEnd);
	return 0;
}

void FightMenuQuit(void)
{
	struct slist *tmp;

	slist_foreach(tmp, fileList) {
		free(tmp->data);
	}
	slist_free(fileList);

	slist_foreach(tmp, sceneList) {
		free(tmp->data);
	}
	slist_free(sceneList);

	ClearMenuItems(mainMenu);
	ClearMenuItems(fightSceneSelect);
	deregister_event("pageup", FightPageUp);
	deregister_event("pagedown", FightPageDown);
	deregister_event("home", FightHome);
	deregister_event("end", FightEnd);
}

void FightMenu(void)
{
	while(mainMenu->activeMenuItem >= mainMenu->itemStart+mainMenu->menuSize)
		mainMenu->itemStart++;
	while(mainMenu->activeMenuItem < mainMenu->itemStart)
		mainMenu->itemStart--;

	DrawPartialMenu(mainMenu, mainMenu->itemStart, mainMenu->itemStart+mainMenu->menuSize);

	while(fightSceneSelect->activeMenuItem >= fightSceneSelect->itemStart+fightSceneSelect->menuSize)
		fightSceneSelect->itemStart++;
	while(fightSceneSelect->activeMenuItem < fightSceneSelect->itemStart)
		fightSceneSelect->itemStart--;
	DrawPartialMenu(fightSceneSelect, fightSceneSelect->itemStart, fightSceneSelect->itemStart+fightSceneSelect->menuSize);
}

int option_menu_init(void)
{
	float c[4] = {0.0, 1.0, 1.0, 1.0};
	struct slider *s;
	struct button *b;
	int fullscreen;
	int players = cfg_get_int("main", "players");
	int difficulty = cfg_get_int("main", "difficulty");

	if(!menuActive) RegisterMenuEvents();
	CreateSlider(mainMenu, "Players", c, 1, 4, 1, players);

	s = CreateSlider(mainMenu, "Difficulty", c, 0, 3, 1, difficulty);
	name_slider_item(mainMenu, s, 0, "Muffins & Lollipops");
	name_slider_item(mainMenu, s, 1, "DJ McFunAdoo");
	name_slider_item(mainMenu, s, 2, "Too-Dope Extreme!");
	name_slider_item(mainMenu, s, 3, "The Heart Stopper!!");

	fullscreen = cfg_eq("video", "fullscreen", "yes");
	CreateBoolean(mainMenu, "Full screen", c, "On", "Off", fullscreen);

	b = CreateButton(mainMenu, "Back", MenuBack);
	b->sound_enabled = 0;
	return 0;
}

void option_menu_quit(void)
{
	struct slider *s;

	s = (struct slider*)mainMenu->items[0].item;
	cfg_set_int("main", "players", s->val);

	s = (struct slider*)mainMenu->items[1].item;
	cfg_set_int("main", "difficulty", s->val);

	s = (struct slider*)mainMenu->items[2].item;
	cfg_set("video", "fullscreen", s->val ? "yes" : "no");

	ClearMenuItems(mainMenu);
}

void option_menu(void)
{
	DrawMenu(mainMenu);
}

static int configuring = -1;
static const char *cfglabels[] = {"Up", "Down", "Left", "Right", "Laser 1", "Laser 2", "Laser 3", "Repeat", "Shift", "Select", "Menu"};
static struct text *newKeyText;
static struct slider *config_player_slider;

void ConfigChangeHandler(void)
{
	cur_player = config_player_slider->val - 1;
	ClearMenuItems(mainMenu);
	ConfigCreateItems();
}

void ConfigCreateItems(void)
{
	int x;
	char *s;
	float c[4] = {0.0, 1.0, 1.0, 1.0};

	config_player_slider = CreateSlider(mainMenu, "Player", get_player_color(cur_player), 1, 4, 1, cur_player + 1);
	config_player_slider->handler = ConfigChangeHandler;

	for(x=0;x<B_LAST;x++) {
		s = joykey_name(x, cur_player);
		CreateButtonParam(mainMenu, s, ConfigButton, x, 90);
		free(s);
	}
	for(x=0;x<(signed)(sizeof(cfglabels)/sizeof(*cfglabels));x++) {
		CreateText(mainMenu, cfglabels[x], c, 200, 200 + (x+1) * FONT_HEIGHT);
	}
	newKeyText = CreateText(mainMenu, "Press a new key", c, mainMenu->menuX, mainMenu->menuY-FONT_HEIGHT);
	newKeyText->active = 0;
}

void ConfigKeyHandler(const void *data)
{
	int tmp = mainMenu->activeMenuItem;
	const struct joykey *jk = data;

	if(set_button(configuring, jk, cur_player)) {
		ELog(("Error setting configure button %i!\n", configuring));
	}
	MPlaySound(snd_push);
	newKeyText->active = 0;
	input_mode(MENU);
	ClearMenuItems(mainMenu);
	ConfigCreateItems();
	mainMenu->activeMenuItem = tmp;
}

int ConfigButton(int b)
{
	configuring = b;
	register_event("key", ConfigKeyHandler);
	newKeyText->active = 1;
	input_mode(KEY);
	return 1;
}

int ConfigMenuInit(void)
{
	if(!menuActive) RegisterMenuEvents();
	input_mode(MENU);
	ConfigCreateItems();
	return 0;
}

void ConfigMenuQuit(void)
{
	ClearMenuItems(mainMenu);
}

void ConfigMenu(void)
{
	if(configuring != -1 && newKeyText->active == 0) {
		configuring = -1;
		deregister_event("key", ConfigKeyHandler);
	}
	DrawMenu(mainMenu);
}

void Quit(int shift)
{
	if(shift) {}
	gmae_quit();
}

int QuitMenuInit(void)
{
	float c[4] = {0.0, 1.0, 1.0, 1.0};
	struct button *b;

	if(!menuActive) RegisterMenuEvents();
	input_mode(MENU);
	mainMenu->menuX = 350;
	mainMenu->menuY = 200;
	CreateButton(mainMenu, "Yes", Quit);
	b = CreateButton(mainMenu, "No", MenuBack);
	b->sound_enabled = 0;
	CreateText(mainMenu, "Really Quit?", c, 200, 200);
	mainMenu->activeMenuItem = 1;
	return 0;
}

void QuitMenuQuit(void)
{
	ClearMenuItems(mainMenu);
}

void QuitMenu(void)
{
	DrawMenu(mainMenu);
}

/** The number of menus that are defined */
#define NUMMENUS 7
static struct menu menus[NUMMENUS] = {
	{NullMenuInit, NullMenuQuit, NullMenu, NULLMENU},
	{NoMenuInit, NoMenuQuit, NoMenu, NULLMENU},
	{MainMenuInit, MainMenuQuit, MainMenu, NOMENU},
	{FightMenuInit, FightMenuQuit, FightMenu, MAINMENU},
	{ConfigMenuInit, ConfigMenuQuit, ConfigMenu, MAINMENU},
	{option_menu_init, option_menu_quit, option_menu, MAINMENU},
	{QuitMenuInit, QuitMenuQuit, QuitMenu, MAINMENU}
};

int FindActiveItem(struct menuItem *activeItems, int numActiveItems)
{
	int x;
	for(x=0;x<numActiveItems;x++)
	{
		if(activeItems[x].type < MENU_SELECTABLE) return x;
	}
	return 0;
}

/** Get the active menu
 * @return the struct menu
 */
const struct menu *active_menu(void)
{
	return activeMenu;
}

/** Determines whether or not a menu is currently active on the screen.
 * @return 1 if a menu is active, 0 if not
 */
int is_menu_active(void)
{
	return menuActive;
}

/** Switch to menu @a n
 * @param n The menu number to switch to
 * @return 0 if the menu is switched in, 1 if not
 */
int switch_menu(int n)
{
	if(n < 0 || n >= NUMMENUS) return 0;

	snd_tick = sound_num("wepnsel1.wav");
	snd_push = sound_num("spnray03.wav");
	snd_back = sound_num("spnray02.wav");

	Log(("Switching Menu: %i\n", n));
	if(activeMenu) activeMenu->QuitMenu();
	else NullMenuQuit(); /* called for the first menu switch */

	mainMenu->BoundsCheck = MenuWrap;
	mainMenu->menuX = 200;
	mainMenu->menuY = 200;
	mainMenu->minX = NOBOX;
	mainMenu->minY = 0;
	mainMenu->maxX = 0;
	mainMenu->maxY = 0;
	mainMenu->itemStart = 0;
	mainMenu->menuSize = -1;
	numScreenMenus = 1;
	curMenu = 0;
	if(menus[n].InitMenu())
	{
		activeMenu = &(menus[NULLMENU]);
		Log(("Menu switch failed\n"));
		return 1;
	}
	activeMenu = &(menus[n]);
	if(mainMenu->activeMenuItem < mainMenu->numItems && mainMenu->items[mainMenu->activeMenuItem].type >= MENU_SELECTABLE)
		mainMenu->activeMenuItem = FindActiveItem(mainMenu->items, mainMenu->numItems);
	Log(("Menu switched\n"));
	return 0;
}

void null_handler(void)
{
}

/*void DrawButton(GLuint button, int x, int y, int on)
{
	if(on) glColor3f(1.0, 1.0, 1.0);
	else glColor3f(0.0, .7, .7);
	glBindTexture(GL_TEXTURE_2D, button);

	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0); glVertex2i(0+x, 0+y);
	glTexCoord2f(1.0, 0.0); glVertex2i(256+x, 0+y);
	glTexCoord2f(1.0, 1.0); glVertex2i(256+x, 64+y);
	glTexCoord2f(0.0, 1.0); glVertex2i(0+x, 64+y);
	glEnd();
}*/
