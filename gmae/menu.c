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

#include "SDL_opengl.h"

#include "menu.h"
#include "main.h"
#include "event.h"
#include "glfunc.h"
#include "phys.h"
#include "textures.h"
#include "scene.h"
#include "sounds.h"
#include "cfg.h"
#include "log.h"

#include "util/memtest.h"
#include "util/fatalerror.h"
#include "util/flist.h"
#include "util/slist.h"
#include "util/strfunc.h"

#define MENU_SLIDER 0
#define MENU_BOOLEAN 1
#define MENU_BUTTON 2
#define MENU_BUTTONPARAM 3
#define MENU_TEXT 4
#define MENU_SELECTABLE 4 /* items from 0 to MENU_SELECTABLE are selectable */
#define MUSICDIR "music/"
#define SCENEDIR "scenes/"

#define NOBOX -1 /* for the bounding box */
#define BBO 5	/* bounding box offset */

/** A slider menu object
 */
struct slider {
	int min; /**< The minimum value */
	int max; /**< The maximum value */
	int del; /**< The delta */
	int val; /**< The actual value */
};

/** A boolean menu object
 */
struct boolean {
	char *trueString;  /**< String if true */
	char *falseString; /**< String if false */
	int val;           /**< Value (1 or 0) */
};

/** A button menu object
 */
struct button {
	void (*activeFunc)(void); /**< The function to execute when activated */
};

/** A button menu object that takes a parameter
 */
struct buttonParam {
	int (*activeFunc)(int); /**< The function to execute when activated */
	int param;              /**< The parameter to send the function */
};

/** A text menu object
 */
struct text {
	float c[4]; /**< The color of the object */
	int x;      /**< The x position */
	int y;      /**< The y position */
	int active; /**< true if the text is to be displayed*/
};

/** A menu item
 */
struct menuItem {
	int type;   /**< One of the MENU_ defines above */
	char *name; /**< The name of this menu item */
	void *item; /**< Pointer to one of the item structs above */
};

/** Describes a menu box on the screen
 */
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

static void DrawPartialMenu(struct screenMenu *m, int start, int stop);
static void DrawMenu(struct screenMenu *m);
static void AddMenuItem(struct screenMenu *m, const char *name, void *item, int type);
static void ClearMenuItems(struct screenMenu *m);
static void UpdateBox(struct screenMenu *m, int x1, int y1, int x2, int y2);
/*static struct slider *CreateSlider(const char *name, int min, int max, int delta, int initVal);
static struct boolean *CreateBoolean(const char *name, const char *trueString, const char *falseString, int initVal);*/
static struct button *CreateButton(struct screenMenu *m, const char *name, void (*activeFunc)(void));
static struct buttonParam *CreateButtonParam(struct screenMenu *m, const char *name, int (*activeFunc)(int), int param);
static struct text *CreateText(struct screenMenu *m, const char *name, float *c, int x, int y);
static int MenuClamp(struct screenMenu *m);
static int MenuWrap(struct screenMenu *m);
static int DownOne(struct screenMenu *m);
static void MenuDown(void);
static int UpOne(struct screenMenu *m);
static void MenuUp(void);
static void MenuDec(void);
static void MenuInc(void);
static void MenuActivate(const void *);
static void MenuSelect(void);
static int valid_music_file(const char *s);
static int alphabetical(const void *a, const void *b);
static int FightMenuInit(void);
static void FightMenuQuit(void);
static void EQTriangle(void);
static void FightMenu(void);
static void MenuBack(void);
static void RegisterMenuEvents(void);
static void ShowMenu(const void *);
static void HideMenu(void);
static int NullMenuInit(void);
static void NullMenuQuit(void);
static void NullMenu(void);
static int NoMenuInit(void);
static void NoMenuQuit(void);
static void NoMenu(void);
static void Retry(void);
static int MainMenuInit(void);
static void MainMenuQuit(void);
static void MainMenu(void);
static void FightActivate(void);
static void FightPageUp(const void *);
static void FightPageDown(const void *);
static void FightHome(const void *);
static void FightEnd(const void *);
static int FindActiveItem(struct menuItem *activeItems, int numActiveItems);
/*static void DrawButton(GLuint button, int x, int y, int on);*/
static void ConfigCreateItems(void);
static void ConfigKeyHandler(const void *data);
static int ConfigButton(int b);
static int ConfigMenuInit(void);
static void ConfigMenuQuit(void);
static void ConfigMenu(void);
static void Quit(void);
static int QuitMenuInit(void);
static void QuitMenuQuit(void);
static void QuitMenu(void);
static void button_handler(const void *data);

int menuActive = 0;

int curMenu;
int numScreenMenus = 1;
struct screenMenu screenMenus[2];
struct screenMenu *mainMenu = &screenMenus[0];

static int snd_tick;
static int snd_push;
static int snd_back;

void button_handler(const void *data)
{
	const struct button_e *b = data;
	switch(b->button) {
		case B_UP:
			MenuUp();
			break;
		case B_DOWN:
			MenuDown();
			break;
		case B_LEFT:
			MenuDec();
			break;
		case B_RIGHT:
			MenuInc();
			break;
		case B_MENU:
			MenuBack();
			break;
		case B_SELECT:
			MenuSelect();
			break;
		default:
			break;
	}
}

void ShadedBox(int x1, int y1, int x2, int y2)
{
	SetOrthoProjection();
	glColor4f(.3, .3, .3, .5);
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	{
		glVertex2i(x1, y1);
		glVertex2i(x1, y2);
		glVertex2i(x2, y2);
		glVertex2i(x2, y1);
	} glEnd();

	glColor4f(1.0, 1.0, 1.0, 1.0);
	glBegin(GL_LINE_LOOP);
	{
		glVertex2i(x1, y1);
		glVertex2i(x1, y2);
		glVertex2i(x2, y2);
		glVertex2i(x2, y1);
	} glEnd();
	glEnable(GL_TEXTURE_2D);
	ResetProjection();
}

void EQTriangle(void)
{
	glDisable(GL_TEXTURE_2D);

	glColor4f(0.3, 0.3, 0.3, 0.5);
	glTranslated(0.0, 0.0, -0.3);
	glBegin(GL_TRIANGLES); {
		glVertex3f(-1.0, 0.0, 0.0);
		glVertex3f(1.0, 0.0, 0.0);
		glVertex3f(0.0, 1.732050807, 0.0);
	} glEnd();

	glColor4f(1.0, 1.0, 1.0, 1.0);
	glTranslatef(0.0, 0.0, 0.3);
	glBegin(GL_LINE_LOOP); {
		glVertex3f(-1.0, 0.0, 0.0);
		glVertex3f(1.0, 0.0, 0.0);
		glVertex3f(0.0, 1.732050807, 0.0);
	} glEnd();

	glEnable(GL_TEXTURE_2D);
}

void DrawPartialMenu(struct screenMenu *m, int start, int stop)
{
	int x;
	int bottom;
	struct text *t;

	if(m->menuSize == -1) {
		bottom = m->maxY;
	} else {
		x = m->minY + FONT_HEIGHT * m->menuSize;
		bottom = (m->maxY < x) ? m->maxY : x;
	}
	ShadedBox(m->minX-BBO, m->minY-BBO, m->maxX+BBO, bottom + BBO);
	if(start < 0 || start >= m->numItems) start = 0;
	if(stop < 0 || stop >= m->numItems) stop = m->numItems;
	for(x=start;x<stop;x++)
	{
		switch(m->items[x].type)
		{
			case MENU_SLIDER:
				break;
			case MENU_BOOLEAN:
				break;
			case MENU_BUTTON:
			case MENU_BUTTONPARAM:
				if(x == m->activeMenuItem) {
					if(m == &screenMenus[curMenu])
						glColor3f(1.0, 0.0, 0.0);
					else
						glColor3f(0.0, 0.0, 1.0);
				}
				else glColor3f(1.0, 1.0, 1.0);
				PrintGL(m->menuX, m->menuY+(x-start)*FONT_HEIGHT, m->items[x].name);
				break;
			case MENU_TEXT:
				t = (struct text*)m->items[x].item;
				if(t->active)
				{
					glColor4f(t->c[RED], t->c[GREEN], t->c[BLUE], t->c[ALPHA]);
					PrintGL(t->x, t->y, m->items[x].name);
				}
		}
	}

	glColor3f(1.0, 1.0, 1.0);
	SetOrthoProjection();
	if(m->itemStart)
	{
		glPushMatrix();
			glTranslatef(m->menuX-20, m->menuY, 0.0);
			glScalef(10.0, -10.0, 1.0);
			EQTriangle();
		glPopMatrix();
	}
	if(m->menuSize != -1 && m->itemStart < m->numItems - m->menuSize)
	{
		int tmp = m->minY + FONT_HEIGHT * m->menuSize;
		glPushMatrix();
			glTranslatef(m->menuX-20, tmp, 0.0);
			glScalef(10.0, 10.0, 1.0);
			EQTriangle();
		glPopMatrix();
	}
	ResetProjection();
}

void DrawMenu(struct screenMenu *m)
{
	DrawPartialMenu(m, 0, m->numItems);
}

void AddMenuItem(struct screenMenu *m, const char *name, void *item, int type)
{
	m->items = (struct menuItem*)realloc(m->items, sizeof(struct menuItem) * (m->numItems+1));
	m->items[m->numItems].name = (char*)malloc(sizeof(char) * (strlen(name)+1));
	strcpy(m->items[m->numItems].name, name);
	m->items[m->numItems].item = item;
	m->items[m->numItems].type = type;
	m->numItems++;
}

void ClearMenuItems(struct screenMenu *m)
{
	int x;
	struct boolean *b;
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
				free(m->items[x].item);
				break;
			case MENU_BOOLEAN:
				b = (struct boolean*)m->items[x].item;
				free(b->trueString);
				free(b->falseString);
				free(b);
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

/*struct slider *CreateSlider(const char *name, int min, int max, int delta, int initVal)
{
	struct slider *s;
	s = (struct slider*)malloc(sizeof(struct slider));
	s->min = min;
	s->max = max;
	s->del = delta;
	s->val = initVal;
	AddMenuItem(name, (void*)s, SLIDER);
	return s;
}

struct boolean *CreateBoolean(const char *name, const char *trueString, const char *falseString, int initVal)
{
	struct boolean *b;
	b = (struct boolean*)malloc(sizeof(struct boolean));
	b->trueString = (char*)malloc(sizeof(char) * (strlen(trueString)+1));
	b->falseString = (char*)malloc(sizeof(char) * (strlen(falseString)+1));
	strcpy(b->trueString, trueString);
	strcpy(b->falseString, falseString);
	b->val = initVal;
	AddMenuItem(name, (void*)b, BOOLEAN);
	return b;
}*/

struct button *CreateButton(struct screenMenu *m, const char *name, void (*activeFunc)(void))
{
	struct button *b;
	b = (struct button*)malloc(sizeof(struct button));
	b->activeFunc = activeFunc;
	UpdateBox(m, m->menuX, m->menuY + FONT_HEIGHT * m->numItems, m->menuX + strlen(name) * FONT_WIDTH, m->menuY + FONT_HEIGHT * (m->numItems+1));
	AddMenuItem(m, name, (void*)b, MENU_BUTTON);
	return b;
}

struct buttonParam *CreateButtonParam(struct screenMenu *m, const char *name, int (*activeFunc)(int), int param)
{
	struct buttonParam *b;
	b = (struct buttonParam*)malloc(sizeof(struct buttonParam));
	b->activeFunc = activeFunc;
	b->param = param;
	UpdateBox(m, m->menuX, m->menuY + FONT_HEIGHT * m->numItems, m->menuX + strlen(name) * FONT_WIDTH, m->menuY + FONT_HEIGHT * (m->numItems+1));
	AddMenuItem(m, name, (void*)b, MENU_BUTTONPARAM);
	return b;
}

/* assumes text is on a single line for bounding box purposes */
struct text *CreateText(struct screenMenu *m, const char *name, float *c, int x, int y)
{
	struct text *t;
	t = (struct text*)malloc(sizeof(struct text));
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

void MenuDown(void)
{
	struct screenMenu *m = &screenMenus[curMenu];
	int old = m->activeMenuItem;
	int play = 0;

	do {
		if(DownOne(m)) {
			if(m->items[m->activeMenuItem].type < MENU_SELECTABLE) {
				play = 1;
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

void MenuUp(void)
{
	struct screenMenu *m = &screenMenus[curMenu];
	int old = m->activeMenuItem;
	int play = 0;

	do {
		if(UpOne(m)) {
			if(m->items[m->activeMenuItem].type < MENU_SELECTABLE) {
				play = 1;
				break;
			}
		}
		else break;
	} while(m->activeMenuItem != old);
	if(play) MPlaySound(snd_tick);
}

void MenuDec(void)
{
	printf("Menu item decremented\n");
}

void MenuInc(void)
{
	printf("Menu item incremented\n");
}

void MenuActivate(const void *data)
{
	struct buttonParam *bp;
	struct screenMenu *m = &screenMenus[curMenu];

	if(data) {}

	MPlaySound(snd_push);
	switch(m->items[m->activeMenuItem].type)
	{
		case MENU_BUTTON:
			((struct button*)m->items[m->activeMenuItem].item)->activeFunc();
			break;
		case MENU_BUTTONPARAM:
			bp = (struct buttonParam*)m->items[m->activeMenuItem].item;
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

void MenuBack(void)
{
	MPlaySound(snd_back);
	SwitchMenu(activeMenu->back);
}

void RegisterMenuEvents(void)
{
	menuActive = 1;
	curMenu = 0;
	RegisterEvent("button", button_handler, EVENTTYPE_STOP);
	RegisterEvent("enter", MenuActivate, EVENTTYPE_STOP);
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
	FireEvent("menu", &m);
	SwitchMenu(MAINMENU);
}

void HideMenu(void)
{
	struct menu_e m;
	if(!menuActive) return;
	menuActive = 0;
	DeregisterEvent("enter", MenuActivate);
	DeregisterEvent("button", button_handler);
	m.active = 0;
	FireEvent("menu", &m);
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
	EventMode(GAME);
	HideMenu();
	RegisterEvent("button", ShowMenu, EVENTTYPE_MULTI);
	return 0;
}

void NoMenuQuit(void)
{
	DeregisterEvent("button", ShowMenu);
}

void NoMenu(void)
{
}

void Retry(void)
{
	SwitchMenu(NOMENU);
	Log(("Retry SwitchScene\n"));
	SwitchScene(MAINSCENE);
	Log(("Retry Scene Switched\n"));
}

int MainMenuInit(void)
{
	if(!menuActive) RegisterMenuEvents();
	EventMode(MENU);
	mainMenu->menuX = 200;
	mainMenu->menuY = 200;
	CreateButtonParam(mainMenu, "Fight", SwitchMenu, FIGHTMENU);
	if(SceneActive(MAINSCENE)) CreateButton(mainMenu, "Retry", Retry);
	CreateButtonParam(mainMenu, "Configure", SwitchMenu, CONFIGMENU);
	CreateButtonParam(mainMenu, "Quit", SwitchMenu, QUITMENU);
	return 0;
}

void MainMenuQuit(void)
{
	ClearMenuItems(mainMenu);
}

void MainMenu(void)
{
	SetOrthoProjection();
	glLoadIdentity();

	glBindTexture(GL_TEXTURE_2D, TextureNum("Title.png"));
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

	ResetProjection();
}

struct slist *fileList;
struct slist *sceneList;
struct screenMenu *fightSceneSelect = &screenMenus[1];

void FightActivate(void)
{
	char *m = CatStr(MUSICDIR, (char*)slist_nth(fileList, mainMenu->activeMenuItem)->data);
	char *s = CatStr(SCENEDIR, (char*)slist_nth(sceneList, fightSceneSelect->activeMenuItem)->data);

	printf("Activating %i, %i\n", mainMenu->activeMenuItem, fightSceneSelect->activeMenuItem);
	CfgSetS("main.song", m);
	CfgSetS("main.scene", s);
	free(m);
	free(s);
	SwitchMenu(NOMENU);
	Log(("FightActivate SwitchScene\n"));
	SwitchScene(MAINSCENE);
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
	char *lastFile;
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
		s = StringCopy(f.filename);
		sceneList = slist_insert_sorted(sceneList, s, alphabetical);
	}
	sceneList = slist_insert(sceneList, StringCopy("default"));

	flist_foreach(&f, MUSICDIR) {
		if(valid_music_file(f.filename)) {
			char *s;
			s = StringCopy(f.filename);
			fileList = slist_insert_sorted(fileList, s, alphabetical);
		}
	}
	if(slist_length(fileList) == 0) {
		Error("Generating playlist");
		return 0;
	}

	lastFile = CfgS("main.song");
	cnt = 0;
	len = slist_length(fileList);
	slist_foreach(tmp, fileList) {
		char *t = CatStr(MUSICDIR, (char*)tmp->data);
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

	cnt = 0;
	len = slist_length(sceneList);
	slist_foreach(tmp, sceneList) {
		char *t = CatStr(MUSICDIR, (char*)tmp->data);
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

	RegisterEvent("pageup", FightPageUp, EVENTTYPE_STOP);
	RegisterEvent("pagedown", FightPageDown, EVENTTYPE_STOP);
	RegisterEvent("home", FightHome, EVENTTYPE_STOP);
	RegisterEvent("end", FightEnd, EVENTTYPE_STOP);
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
	DeregisterEvent("pageup", FightPageUp);
	DeregisterEvent("pagedown", FightPageDown);
	DeregisterEvent("home", FightHome);
	DeregisterEvent("end", FightEnd);
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

int configuring = -1;
const char *cfglabels[] = {"Up", "Down", "Left", "Right", "Laser 1", "Laser 2", "Laser 3", "Repeat", "Select", "Menu"};
struct text *newKeyText;

void ConfigCreateItems(void)
{
	int x;
	char *s;
	float c[4] = {0.0, 1.0, 1.0, 1.0};

	mainMenu->menuX = 290;
	mainMenu->menuY = 200;
	for(x=0;x<B_LAST;x++) {
		s = JoyKeyName(x);
		CreateButtonParam(mainMenu, s, ConfigButton, x);
		free(s);
	}
	for(x=0;x<(signed)(sizeof(cfglabels)/sizeof(*cfglabels));x++) {
		CreateText(mainMenu, cfglabels[x], c, 200, 200 + x * FONT_HEIGHT);
	}
	newKeyText = CreateText(mainMenu, "Press a new key", c, mainMenu->menuX-90, mainMenu->menuY-FONT_HEIGHT);
	newKeyText->active = 0;
}

void ConfigKeyHandler(const void *data)
{
	int tmp = mainMenu->activeMenuItem;
	const struct joykey *jk = data;

	if(!SetButton(configuring, jk)) {
		ELog(("Error setting configure button %i!\n", configuring));
	}
	MPlaySound(snd_push);
	newKeyText->active = 0;
	EventMode(MENU);
	ClearMenuItems(mainMenu);
	ConfigCreateItems();
	mainMenu->activeMenuItem = tmp;
}

int ConfigButton(int b)
{
	configuring = b;
	RegisterEvent("key", ConfigKeyHandler, EVENTTYPE_STOP);
	newKeyText->active = 1;
	EventMode(KEY);
	return 1;
}

int ConfigMenuInit(void)
{
	if(!menuActive) RegisterMenuEvents();
	EventMode(MENU);
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
		DeregisterEvent("key", ConfigKeyHandler);
	}
	DrawMenu(mainMenu);
}

void Quit(void)
{
	quit = 1; /* causes main loop to exit */
}

int QuitMenuInit(void)
{
	float c[4] = {0.0, 1.0, 1.0, 1.0};
	if(!menuActive) RegisterMenuEvents();
	EventMode(MENU);
	mainMenu->menuX = 350;
	mainMenu->menuY = 200;
	CreateButton(mainMenu, "Yes", Quit);
	CreateButton(mainMenu, "No", MenuBack);
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

#define NUMMENUS 6
struct menu menus[NUMMENUS] = {	{NullMenuInit, NullMenuQuit, NullMenu, NULLMENU},
				{NoMenuInit, NoMenuQuit, NoMenu, NULLMENU},
				{MainMenuInit, MainMenuQuit, MainMenu, NOMENU},
				{FightMenuInit, FightMenuQuit, FightMenu, MAINMENU},
				{ConfigMenuInit, ConfigMenuQuit, ConfigMenu, MAINMENU},
				{QuitMenuInit, QuitMenuQuit, QuitMenu, MAINMENU}};

int FindActiveItem(struct menuItem *activeItems, int numActiveItems)
{
	int x;
	for(x=0;x<numActiveItems;x++)
	{
		if(activeItems[x].type < MENU_SELECTABLE) return x;
	}
	return 0;
}

int SwitchMenu(int n)
{
	if(n < 0 || n >= NUMMENUS) return 0;

	snd_tick = SoundNum("wepnsel1.wav");
	snd_push = SoundNum("spnray03.wav");
	snd_back = SoundNum("spnray02.wav");

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
