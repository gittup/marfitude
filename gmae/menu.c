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
#include <dirent.h>

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

#include "memtest.h"
#include "fatalerror.h"
#include "slist.h"

#define MENU_SLIDER 0
#define MENU_BOOLEAN 1
#define MENU_BUTTON 2
#define MENU_BUTTONPARAM 3
#define MENU_TEXT 4
#define MENU_SELECTABLE 4 /* items from 0 to MENU_SELECTABLE are selectable */
#define MUSICDIR "music/"

#define NOBOX -1 /* for the bounding box */
#define BBO 5	/* bounding box offset */

struct slider {
	int min;
	int max;
	int del;
	int val;
};

struct boolean {
	char *trueString;
	char *falseString;
	int val;
};

struct button {
	void (*activeFunc)(void);
};

struct buttonParam {
	int (*activeFunc)(int);
	int param;
};

struct text {
	float c[4];
	int x;
	int y;
	int active;
};

struct menuItem {
	int type;
	char *name;
	void *item;
};

static void DrawPartialMenu(int start, int stop);
static void DrawMenu(void);
static void AddMenuItem(const char *name, void *item, int type);
static void ClearMenuItems(void);
static void UpdateBox(int x1, int y1, int x2, int y2);
/*static struct slider *CreateSlider(const char *name, int min, int max, int delta, int initVal);
static struct boolean *CreateBoolean(const char *name, const char *trueString, const char *falseString, int initVal);*/
static struct button *CreateButton(const char *name, void (*activeFunc)(void));
static struct buttonParam *CreateButtonParam(const char *name, int (*activeFunc)(int), int param);
static struct text *CreateText(const char *name, float *c, int x, int y);
static int MenuClamp(void);
static int MenuWrap(void);
static int DownOne(void);
static void MenuDown(void);
static int UpOne(void);
static void MenuUp(void);
static void MenuDec(void);
static void MenuInc(void);
static void MenuActivate(void);
static int ValidMusicFile(char *s);
static char *StringCopy(char *s);
static int alphabetical(const void *a, const void *b);
static int FightMenuInit(void);
static void FightMenuQuit(void);
static void EQTriangle(void);
static void FightMenu(void);
static void MenuBack(void);
static void RegisterMenuEvents(void);
static void ShowMenu(void);
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
static char *CatStr(const char *a, const char *b);
static void FightActivate(void);
static void FightPageUp(void);
static void FightPageDown(void);
static int FindActiveItem(struct menuItem *activeItems, int numActiveItems);
/*static void DrawButton(GLuint button, int x, int y, int on);*/
static void ConfigCreateItems(void);
static void ConfigKeyHandler(struct joykey *jk);
static int ConfigButton(int b);
static int ConfigMenuInit(void);
static void ConfigMenuQuit(void);
static void ConfigMenu(void);
static void Quit(void);
static int QuitMenuInit(void);
static void QuitMenuQuit(void);
static void QuitMenu(void);

/* when the active menu item goes out of bounds */
/* returns 1 if a sound is to be played, 0 otherwise */
int (*BoundsCheck)(void);
int activeMenuItem = 0;
int numItems = 0;
int menuX = 200, menuY = 200; /* where to start placing menu items */
int minX = NOBOX, minY = 0, maxX = 0, maxY = 0; /* bounding box of menu items */
int menuActive = 0;
struct menuItem *items = NULL;

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

void DrawPartialMenu(int start, int stop)
{
	int x;
	struct text *t;
	if(start < 0 || start >= numItems) start = 0;
	if(stop < 0 || stop >= numItems) stop = numItems;
	for(x=start;x<stop;x++)
	{
		switch(items[x].type)
		{
			case MENU_SLIDER:
				break;
			case MENU_BOOLEAN:
				break;
			case MENU_BUTTON:
			case MENU_BUTTONPARAM:
				if(x == activeMenuItem) glColor3f(1.0, 0.0, 0.0);
				else glColor3f(1.0, 1.0, 1.0);
				PrintGL(menuX, menuY+(x-start)*FONT_HEIGHT, items[x].name);
				break;
			case MENU_TEXT:
				t = (struct text*)items[x].item;
				if(t->active)
				{
					glColor4f(t->c[RED], t->c[GREEN], t->c[BLUE], t->c[ALPHA]);
					PrintGL(t->x, t->y, items[x].name);
				}
		}
	}
}

void DrawMenu(void)
{
	DrawPartialMenu(0, numItems);
}

void AddMenuItem(const char *name, void *item, int type)
{
	items = (struct menuItem*)realloc(items, sizeof(struct menuItem) * (numItems+1));
	items[numItems].name = (char*)malloc(sizeof(char) * (strlen(name)+1));
	strcpy(items[numItems].name, name);
	items[numItems].item = item;
	items[numItems].type = type;
	numItems++;
}

void ClearMenuItems(void)
{
	int x;
	struct boolean *b;
	Log(("Clearing items...\n"));
	minX = NOBOX;
	minY = 0;
	maxX = 0;
	maxY = 0;
	for(x=0;x<numItems;x++)
	{
		free(items[x].name);
		switch(items[x].type)
		{
			case MENU_SLIDER:
				free(items[x].item);
				break;
			case MENU_BOOLEAN:
				b = (struct boolean*)items[x].item;
				free(b->trueString);
				free(b->falseString);
				free(b);
				break;
			case MENU_BUTTON:
			case MENU_BUTTONPARAM:
			case MENU_TEXT:
				free(items[x].item);
				break;
			default:
				ELog(("Error: Invalid menu item type!\n"));
				break;
		}
	}
	numItems = 0;
	activeMenuItem = 0;
	free(items);
	items = NULL;
	Log(("All items freed.\n"));
}

void UpdateBox(int x1, int y1, int x2, int y2)
{
	if(minX == NOBOX)
	{
		minX = x1;
		minY = y1;
		maxX = x2;
		maxY = y2;
	}
	else
	{
		if(x1 < minX) minX = x1;
		if(y1 < minY) minY = y1;
		if(x2 > maxX) maxX = x2;
		if(y2 > maxY) maxY = y2;
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

struct button *CreateButton(const char *name, void (*activeFunc)(void))
{
	struct button *b;
	b = (struct button*)malloc(sizeof(struct button));
	b->activeFunc = activeFunc;
	UpdateBox(menuX, menuY + FONT_HEIGHT * numItems, menuX + strlen(name) * FONT_WIDTH, menuY + FONT_HEIGHT * (numItems+1));
	AddMenuItem(name, (void*)b, MENU_BUTTON);
	return b;
}

struct buttonParam *CreateButtonParam(const char *name, int (*activeFunc)(int), int param)
{
	struct buttonParam *b;
	b = (struct buttonParam*)malloc(sizeof(struct buttonParam));
	b->activeFunc = activeFunc;
	b->param = param;
	UpdateBox(menuX, menuY + FONT_HEIGHT * numItems, menuX + strlen(name) * FONT_WIDTH, menuY + FONT_HEIGHT * (numItems+1));
	AddMenuItem(name, (void*)b, MENU_BUTTONPARAM);
	return b;
}

/* assumes text is on a single line for bounding box purposes */
struct text *CreateText(const char *name, float *c, int x, int y)
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
	UpdateBox(x, y, x + strlen(name) * FONT_WIDTH, y + FONT_HEIGHT);
	AddMenuItem(name, (void*)t, MENU_TEXT);
	return t;
}

int MenuClamp(void)
{
	if(activeMenuItem < 0)
	{
		activeMenuItem = 0;
		return 0;
	}
	if(activeMenuItem >= numItems)
	{
		activeMenuItem = numItems >= 0 ? numItems - 1 : 0;
		return 0;
	}
	return 1;
}

int MenuWrap(void)
{
	if(activeMenuItem >= numItems) activeMenuItem = 0;
	else if(activeMenuItem < 0) activeMenuItem = numItems >= 0 ? numItems -1 : 0;
	return 1;
}

int DownOne(void)
{
	activeMenuItem++;
	if(activeMenuItem >= numItems)
	{
		if(BoundsCheck()) return 1;
		else return 0;
	}
	else return 1;
}

void MenuDown(void)
{
	int old = activeMenuItem;
	int play = 0;
	do
	{
		if(DownOne())
		{
			if(items[activeMenuItem].type < MENU_SELECTABLE)
			{
				play = 1;
				break;
			}
		}
		else break;
	} while(activeMenuItem != old);
	if(play) SDLPlaySound(SND_wepnsel1);
}

int UpOne(void)
{
	activeMenuItem--;
	if(activeMenuItem < 0)
	{
		if(BoundsCheck()) return 1;
		else return 0;
	}
	else return 1;
}

void MenuUp(void)
{
	int old = activeMenuItem;
	int play = 0;
	do
	{
		if(UpOne())
		{
			if(items[activeMenuItem].type < MENU_SELECTABLE)
			{
				play = 1;
				break;
			}
		}
		else break;
	} while(activeMenuItem != old);
	if(play) SDLPlaySound(SND_wepnsel1);
}

void MenuDec(void)
{
	printf("Menu item decremented\n");
}

void MenuInc(void)
{
	printf("Menu item incremented\n");
}

void MenuActivate(void)
{
	struct buttonParam *bp;

	SDLPlaySound(SND_spnray03);
	switch(items[activeMenuItem].type)
	{
		case MENU_BUTTON:
			((struct button*)items[activeMenuItem].item)->activeFunc();
			break;
		case MENU_BUTTONPARAM:
			bp = (struct buttonParam*)items[activeMenuItem].item;
			bp->activeFunc(bp->param);
		default:
			break;
	}
}

void MenuBack(void)
{
	SDLPlaySound(SND_spnray02);
	SwitchMenu(activeMenu->back);
}

void RegisterMenuEvents(void)
{
	menuActive = 1;
	RegisterEvent(EVENT_UP, MenuUp, EVENTTYPE_STOP);
	RegisterEvent(EVENT_DOWN, MenuDown, EVENTTYPE_STOP);
	RegisterEvent(EVENT_LEFT, MenuDec, EVENTTYPE_STOP);
	RegisterEvent(EVENT_RIGHT, MenuInc, EVENTTYPE_STOP);
	RegisterEvent(EVENT_MENU, MenuBack, EVENTTYPE_STOP);
	RegisterEvent(EVENT_ENTER, MenuActivate, EVENTTYPE_STOP);
}

void ShowMenu(void)
{
	if(menuActive) return;
	RegisterMenuEvents();
	SDLPlaySound(SND_spnray03);
	FireEvent(EVENT_SHOWMENU);
	SwitchMenu(MAINMENU);
}

void HideMenu(void)
{
	if(!menuActive) return;
	menuActive = 0;
	DeregisterEvent(EVENT_UP, MenuUp);
	DeregisterEvent(EVENT_DOWN, MenuDown);
	DeregisterEvent(EVENT_LEFT, MenuDec);
	DeregisterEvent(EVENT_RIGHT, MenuInc);
	DeregisterEvent(EVENT_MENU, MenuBack);
	DeregisterEvent(EVENT_ENTER, MenuActivate);
	FireEvent(EVENT_HIDEMENU);
}

int NullMenuInit(void)
{
	HideMenu(); /* deregister events for good measure */
	return 1;
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
	RegisterEvent(EVENT_MENU, ShowMenu, EVENTTYPE_STOP);
	return 1;
}

void NoMenuQuit(void)
{
	DeregisterEvent(EVENT_MENU, ShowMenu);
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
	menuX = 200;
	menuY = 200;
	CreateButtonParam("Fight", SwitchMenu, FIGHTMENU);
	if(SceneActive(MAINSCENE)) CreateButton("Retry", Retry);
	CreateButtonParam("Configure", SwitchMenu, CONFIGMENU);
	CreateButtonParam("Quit", SwitchMenu, QUITMENU);
	return 1;
}

void MainMenuQuit(void)
{
	ClearMenuItems();
}

void MainMenu(void)
{
	SetOrthoProjection();
	glLoadIdentity();

	ShadedBox(minX-BBO, minY-BBO, maxX+BBO, maxY+BBO);
	glBindTexture(GL_TEXTURE_2D, TEX_Title);
	glDisable(GL_LIGHTING);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0); glVertex2i(0, 0);
	glTexCoord2f(1.0, 0.0); glVertex2i(512, 0);
	glTexCoord2f(1.0, 1.0); glVertex2i(512, 256);
	glTexCoord2f(0.0, 1.0); glVertex2i(0, 256);
	glEnd();
	glEnable(GL_LIGHTING);

	DrawMenu();

	ResetProjection();
}

#define FILE_LIST_SIZE 11
int fileStart;
struct slist *fileList;

char *CatStr(const char *a, const char *b)
{
	char *s;
	s = (char*)malloc(sizeof(char) * (strlen(a) + strlen(b) + 1));
	strcpy(s, a);
	strcat(s, b);
	return s;
}

void FightActivate(void)
{
	char *s = CatStr(MUSICDIR, (char*)slist_nth(fileList, activeMenuItem)->data);
	CfgSetS("main.song", s);
	free(s);
	SwitchMenu(NOMENU);
	Log(("FightActivate SwitchScene\n"));
	SwitchScene(MAINSCENE);
	Log(("FightActivate SceneSwitched\n"));
}

void FightPageUp(void)
{
	if(fileStart)
	{
		activeMenuItem -= FILE_LIST_SIZE;
		MenuClamp();
		SDLPlaySound(SND_wepnsel1);
	}
}

void FightPageDown(void)
{
	if(fileStart < (signed)slist_length(fileList) - FILE_LIST_SIZE)
	{
		activeMenuItem += FILE_LIST_SIZE;
		MenuClamp();
		SDLPlaySound(SND_wepnsel1);
	}
}

int ValidMusicFile(char *s)
{
	if(s[0] == '.') return 0;
	while(*s)
	{
		if(*s == '.') return 1;
		s++;
	}
	return 0;
}

char *StringCopy(char *s)
{
	char *d;
	d = (char*)malloc(sizeof(char) * (strlen(s) + 1));
	strcpy(d, s);
	return d;
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
	char *s;
	char *lastFile;
	DIR *dir;
	struct dirent *d;
	int cnt;
	int len;
	struct slist *tmp;

	if(!menuActive) RegisterMenuEvents();
	menuX = 200;
	menuY = 200;
	fileStart = 0;
	BoundsCheck = MenuClamp;
	fileList = NULL;

	dir = opendir(MUSICDIR);
	if(dir == NULL) {
		Error("Generating playlist");
		return 0;
	}

	while((d = readdir(dir)) != NULL) {
		if(ValidMusicFile(d->d_name)) {
			s = StringCopy(d->d_name);
			fileList = slist_insert_sorted(fileList, s, alphabetical);
		}
	}

	lastFile = CfgS("main.song");
	cnt = 0;
	tmp = fileList;
	len = slist_length(fileList);
	while(tmp != NULL) {
		char *t = CatStr(MUSICDIR, (char*)tmp->data);
		if(strcmp(t, lastFile) == 0) {
			fileStart = cnt - FILE_LIST_SIZE/2;
			while(fileStart > len - FILE_LIST_SIZE)
				fileStart--;
			while(fileStart < 0)
				fileStart++;
			activeMenuItem = cnt;
		}
		free(t);
		CreateButton(tmp->data, FightActivate);
		tmp = slist_next(tmp);
		cnt++;
	}

	RegisterEvent(EVENT_PAGEUP, FightPageUp, EVENTTYPE_STOP);
	RegisterEvent(EVENT_PAGEDOWN, FightPageDown, EVENTTYPE_STOP);
	return 1;
}

void FightMenuQuit(void)
{
	struct slist *tmp;

	tmp = fileList;
	while(tmp)
	{
		free(tmp->data);
		tmp = slist_next(tmp);
	}
	slist_free(fileList);
	ClearMenuItems();
	DeregisterEvent(EVENT_PAGEUP, FightPageUp);
	DeregisterEvent(EVENT_PAGEDOWN, FightPageDown);
}

void EQTriangle(void)
{
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_TRIANGLES);
	{
		glVertex2f(-1.0, 0.0);
		glVertex2f(1.0, 0.0);
		glVertex2f(0.0, 1.732050807);
	} glEnd();
	glEnable(GL_TEXTURE_2D);
}

void FightMenu(void)
{
	while(activeMenuItem >= fileStart+FILE_LIST_SIZE)
		fileStart++;
	while(activeMenuItem < fileStart)
		fileStart--;
	ShadedBox(minX-BBO, minY-BBO, maxX+BBO, minY + FONT_HEIGHT * FILE_LIST_SIZE + BBO);
	DrawPartialMenu(fileStart, fileStart+FILE_LIST_SIZE);
	glColor3f(1.0, 1.0, 1.0);
	SetOrthoProjection();
	if(fileStart)
	{
		glPushMatrix();
			glTranslatef(180, 200, 0.0);
			glScalef(10.0, -10.0, 1.0);
			EQTriangle();
		glPopMatrix();
	}
	if(fileStart < (signed)slist_length(fileList) - FILE_LIST_SIZE)
	{
		glPushMatrix();
			glTranslatef(180, 350, 0.0);
			glScalef(10.0, 10.0, 1.0);
			EQTriangle();
		glPopMatrix();
	}
	ResetProjection();
}

int configuring = 0;
const char *cfglabels[] = {"Up", "Down", "Left", "Right", "Laser 1", "Laser 2", "Laser 3", "Repeat", "Menu"};
struct text *newKeyText;

void ConfigCreateItems(void)
{
	int x;
	char *s;
	float c[4] = {0.0, 1.0, 1.0, 1.0};

	menuX = 290;
	menuY = 200;
	for(x=0;x<B_LAST;x++)
	{
		s = JoyKeyName(x);
		CreateButtonParam(s, ConfigButton, x);
		free(s);
	}
	for(x=0;x<(signed)(sizeof(cfglabels)/sizeof(*cfglabels));x++)
	{
		CreateText(cfglabels[x], c, 200, 200 + x * FONT_HEIGHT);
	}
	newKeyText = CreateText("Press a new key", c, menuX-90, menuY-FONT_HEIGHT);
	newKeyText->active = 0;
}

void ConfigKeyHandler(struct joykey *jk)
{
	int tmp = activeMenuItem;
	if(!SetButton(configuring, jk))
	{
		ELog(("Error setting configure button %i!\n", configuring));
	}
	SDLPlaySound(SND_spnray03);
	DeregisterKeyEvent();
	newKeyText->active = 0;
	EventMode(MENU);
	ClearMenuItems();
	ConfigCreateItems();
	activeMenuItem = tmp;
}

int ConfigButton(int b)
{
	configuring = b;
	RegisterKeyEvent(ConfigKeyHandler);
	newKeyText->active = 1;
	EventMode(KEY);
	return 1;
}

int ConfigMenuInit(void)
{
	if(!menuActive) RegisterMenuEvents();
	EventMode(MENU);
	ConfigCreateItems();
	return 1;
}

void ConfigMenuQuit(void)
{
	ClearMenuItems();
}

void ConfigMenu(void)
{
	ShadedBox(minX-BBO, minY-BBO, maxX+BBO, maxY+BBO);
	DrawMenu();
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
	menuX = 350;
	menuY = 200;
	CreateButton("Yes", Quit);
	CreateButton("No", MenuBack);
	CreateText("Really Quit?", c, 200, 200);
	activeMenuItem = 1;
	return 1;
}

void QuitMenuQuit(void)
{
	ClearMenuItems();
}

void QuitMenu(void)
{
	ShadedBox(minX-BBO, minY-BBO, maxX+BBO, maxY+BBO);
	DrawMenu();
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

int SwitchMenu(int m)
{
	if(m < 0 || m >= NUMMENUS) return 0;
	BoundsCheck = MenuWrap;
	Log(("Switching Menu: %i\n", m));
	if(activeMenu) activeMenu->QuitMenu();
	else NullMenuQuit(); /* called for the first menu switch */
	if(!menus[m].InitMenu())
	{
		activeMenu = &(menus[NULLMENU]);
		Log(("Menu switch failed\n"));
		return 0;
	}
	activeMenu = &(menus[m]);
	if(activeMenuItem < numItems && items[activeMenuItem].type >= MENU_SELECTABLE)
		activeMenuItem = FindActiveItem(items, numItems);
	Log(("Menu switched\n"));
	return 1;
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
