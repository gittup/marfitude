#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <glib.h>

#include "GL/gl.h"

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

#include "../util/memtest.h"
#include "../util/fatalerror.h"

#define SLIDER 0
#define BOOLEAN 1
#define BUTTON 2
#define BUTTONPARAM 3
#define TEXT 4
#define SELECTABLE 4 // all items from 0 to SELECTABLE are selectable
#define MUSICDIR "music/"

#define NOBOX -1 // for the bounding box
#define BBO 5	// bounding box offset

typedef struct {
	int min;
	int max;
	int del;
	int val;
	} Slider;

typedef struct {
	char *trueString;
	char *falseString;
	int val;
	} Boolean;

typedef struct {
	void (*activeFunc)();
	} Button;

typedef struct {
	int (*activeFunc)(int);
	int param;
	} ButtonParam;

typedef struct {
	float c[4];
	int x;
	int y;
	int active;
	} Text;

typedef struct {
	int type;
	char *name;
	void *item;
	} MenuItem;

// when the active menu item goes out of bounds
// returns 1 if a sound is to be played, 0 otherwise
int (*BoundsCheck)();
int activeMenuItem = 0;
int numItems = 0;
int menuX = 200, menuY = 200; // where to start placing menu items
int minX = NOBOX, minY = 0, maxX = 0, maxY = 0; // bounding box of menu items
int menuActive = 0;
MenuItem *items = NULL;

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
	Text *t;
	if(start < 0 || start >= numItems) start = 0;
	if(stop < 0 || stop >= numItems) stop = numItems;
	for(x=start;x<stop;x++)
	{
		switch(items[x].type)
		{
			case SLIDER:
				break;
			case BOOLEAN:
				break;
			case BUTTON:
			case BUTTONPARAM:
				if(x == activeMenuItem) glColor3f(1.0, 0.0, 0.0);
				else glColor3f(1.0, 1.0, 1.0);
				PrintGL(menuX, menuY+(x-start)*FONT_HEIGHT, items[x].name);
				break;
			case TEXT:
				t = (Text*)items[x].item;
				if(t->active)
				{
					glColor4f(t->c[RED], t->c[GREEN], t->c[BLUE], t->c[ALPHA]);
					PrintGL(t->x, t->y, items[x].name);
				}
		}
	}
}

void DrawMenu()
{
	DrawPartialMenu(0, numItems);
}

void AddMenuItem(char *name, void *item, int type)
{
	items = (MenuItem*)realloc(items, sizeof(MenuItem) * (numItems+1));
	items[numItems].name = (char*)malloc(sizeof(char) * (strlen(name)+1));
	strcpy(items[numItems].name, name);
	items[numItems].item = item;
	items[numItems].type = type;
	numItems++;
}

void ClearMenuItems()
{
	int x;
	Boolean *b;
	Log("Clearing items...\n");
	minX = NOBOX;
	minY = 0;
	maxX = 0;
	maxY = 0;
	for(x=0;x<numItems;x++)
	{
		free(items[x].name);
		switch(items[x].type)
		{
			case SLIDER:
				free(items[x].item);
				break;
			case BOOLEAN:
				b = (Boolean*)items[x].item;
				free(b->trueString);
				free(b->falseString);
				free(b);
				break;
			case BUTTON:
			case BUTTONPARAM:
			case TEXT:
				free(items[x].item);
				break;
			default:
				ELog("Error: Invalid menu item type!\n");
				break;
		}
	}
	numItems = 0;
	activeMenuItem = 0;
	free(items);
	items = NULL;
	Log("All items freed.\n");
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

Slider *CreateSlider(char *name, int min, int max, int delta, int initVal)
{
	Slider *s;
	s = (Slider*)malloc(sizeof(Slider));
	s->min = min;
	s->max = max;
	s->del = delta;
	s->val = initVal;
	AddMenuItem(name, (void*)s, SLIDER);
	return s;
}

Boolean *CreateBoolean(char *name, char *trueString, char *falseString, int initVal)
{
	Boolean *b;
	b = (Boolean*)malloc(sizeof(Boolean));
	b->trueString = (char*)malloc(sizeof(char) * (strlen(trueString)+1));
	b->falseString = (char*)malloc(sizeof(char) * (strlen(falseString)+1));
	strcpy(b->trueString, trueString);
	strcpy(b->falseString, falseString);
	b->val = initVal;
	AddMenuItem(name, (void*)b, BOOLEAN);
	return b;
}

Button *CreateButton(char *name, void (*activeFunc)())
{
	Button *b;
	b = (Button*)malloc(sizeof(Button));
	b->activeFunc = activeFunc;
	UpdateBox(menuX, menuY + FONT_HEIGHT * numItems, menuX + strlen(name) * FONT_WIDTH, menuY + FONT_HEIGHT * (numItems+1));
	AddMenuItem(name, (void*)b, BUTTON);
	return b;
}

ButtonParam *CreateButtonParam(char *name, int (*activeFunc)(int), int param)
{
	ButtonParam *b;
	b = (ButtonParam*)malloc(sizeof(ButtonParam));
	b->activeFunc = activeFunc;
	b->param = param;
	UpdateBox(menuX, menuY + FONT_HEIGHT * numItems, menuX + strlen(name) * FONT_WIDTH, menuY + FONT_HEIGHT * (numItems+1));
	AddMenuItem(name, (void*)b, BUTTONPARAM);
	return b;
}

// assumes text is on a single line for bounding box purposes
Text *CreateText(char *name, float *c, int x, int y)
{
	Text *t;
	t = (Text*)malloc(sizeof(Text));
	t->c[RED] = c[RED];
	t->c[GREEN] = c[GREEN];
	t->c[BLUE] = c[BLUE];
	t->c[ALPHA] = c[ALPHA];
	t->x = x;
	t->y = y;
	t->active = 1;
	UpdateBox(x, y, x + strlen(name) * FONT_WIDTH, y + FONT_HEIGHT);
	AddMenuItem(name, (void*)t, TEXT);
	return t;
}

int MenuClamp()
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

int MenuWrap()
{
	if(activeMenuItem >= numItems) activeMenuItem = 0;
	else if(activeMenuItem < 0) activeMenuItem = numItems >= 0 ? numItems -1 : 0;
	return 1;
}

int DownOne()
{
	activeMenuItem++;
	if(activeMenuItem >= numItems)
	{
		if(BoundsCheck()) return 1;
		else return 0;
	}
	else return 1;
}

void MenuDown()
{
	int old = activeMenuItem;
	int play = 0;
	do
	{
		if(DownOne())
		{
			if(items[activeMenuItem].type < SELECTABLE)
			{
				play = 1;
				break;
			}
		}
		else break;
	} while(activeMenuItem != old);
	if(play) SDLPlaySound(SND_wepnsel1);
}

int UpOne()
{
	activeMenuItem--;
	if(activeMenuItem < 0)
	{
		if(BoundsCheck()) return 1;
		else return 0;
	}
	else return 1;
}

void MenuUp()
{
	int old = activeMenuItem;
	int play = 0;
	do
	{
		if(UpOne())
		{
			if(items[activeMenuItem].type < SELECTABLE)
			{
				play = 1;
				break;
			}
		}
		else break;
	} while(activeMenuItem != old);
	if(play) SDLPlaySound(SND_wepnsel1);
}

void MenuDec()
{
	printf("Menu item decremented\n");
}

void MenuInc()
{
	printf("Menu item incremented\n");
}

void MenuActivate()
{
	SDLPlaySound(SND_spnray03);
	ButtonParam *bp;
	switch(items[activeMenuItem].type)
	{
		case BUTTON:
			((Button*)items[activeMenuItem].item)->activeFunc();
			break;
		case BUTTONPARAM:
			bp = (ButtonParam*)items[activeMenuItem].item;
			bp->activeFunc(bp->param);
		default:
			break;
	}
}

void MenuBack()
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

int NullMenuInit()
{
	HideMenu(); // deregister events for good measure
	return 1;
}

void NullMenuQuit()
{
}

void NullMenu()
{
}

int NoMenuInit()
{
	EventMode(GAME);
	HideMenu();
	RegisterEvent(EVENT_MENU, ShowMenu, EVENTTYPE_STOP);
	return 1;
}

void NoMenuQuit()
{
	DeregisterEvent(EVENT_MENU, ShowMenu);
}

void NoMenu()
{
}

void Retry()
{
	SwitchMenu(NOMENU);
	Log("Retry SwitchScene\n");
	SwitchScene(MAINSCENE);
	Log("Retry Scene Switched\n");
}

int MainMenuInit()
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

void MainMenuQuit()
{
	ClearMenuItems();
}

void MainMenu()
{
	SetOrthoProjection();
	GLLoadIdentity();

	ShadedBox(minX-BBO, minY-BBO, maxX+BBO, maxY+BBO);
	GLBindTexture(GL_TEXTURE_2D, TEX_Title);
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

#define FILE_LIST_SIZE 10
int fileStart;
GSList *fileList;

char *CatStr(char *a, char *b)
{
	char *s;
	s = (char*)malloc(sizeof(char) * (strlen(a) + strlen(b) + 1));
	strcpy(s, a);
	strcat(s, b);
	return s;
}

void FightActivate()
{
	char *s = CatStr(MUSICDIR, (char*)g_slist_nth(fileList, activeMenuItem)->data);
	CfgSetS("main.song", s);
	free(s);
	SwitchMenu(NOMENU);
	Log("FightActivate SwitchScene\n");
	SwitchScene(MAINSCENE);
	Log("FightActivate SceneSwitched\n");
}

void FightPageUp()
{
	if(fileStart)
	{
		activeMenuItem -= FILE_LIST_SIZE;
		MenuClamp();
		SDLPlaySound(SND_wepnsel1);
	}
}

void FightPageDown()
{
	if(fileStart < g_slist_length(fileList) - FILE_LIST_SIZE)
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

int FightMenuInit()
{
	char *s;
	DIR *dir;
	struct dirent *d;

	menuX = 200;
	menuY = 200;
	fileStart = 0;
	BoundsCheck = MenuClamp;
	fileList = NULL;

	dir = opendir(MUSICDIR);
	if(dir == NULL)
	{
		Error("Generating playlist");
		return 0;
	}
	while((d = readdir(dir)) != NULL)
	{
		if(ValidMusicFile(d->d_name))
		{
			s = StringCopy(d->d_name);
			fileList = g_slist_append(fileList, s);
			CreateButton(s, FightActivate);
		}
	}
	RegisterEvent(EVENT_PAGEUP, FightPageUp, EVENTTYPE_STOP);
	RegisterEvent(EVENT_PAGEDOWN, FightPageDown, EVENTTYPE_STOP);
	return 1;
}

void FightMenuQuit(void)
{
	while(fileList)
	{
		free(fileList->data);
		fileList = g_slist_next(fileList);
	}
	ClearMenuItems();
	DeregisterEvent(EVENT_PAGEUP, FightPageUp);
	DeregisterEvent(EVENT_PAGEDOWN, FightPageDown);
}

void EQTriangle()
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

void FightMenu()
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
	if(fileStart < g_slist_length(fileList) - FILE_LIST_SIZE)
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
int ConfigButton();
char *cfglabels[] = {"Up", "Down", "Left", "Right", "Button 1", "Button 2", "Button 3", "Button 4", "Menu"};
Text *newKeyText;

void ConfigCreateItems()
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
	for(x=0;x<sizeof(cfglabels)/sizeof(*cfglabels);x++)
	{
		CreateText(cfglabels[x], c, 200, 200 + x * FONT_HEIGHT);
	}
	newKeyText = CreateText("Press a new key", c, menuX-90, menuY-FONT_HEIGHT);
	newKeyText->active = 0;
}

void ConfigKeyHandler(JoyKey *jk)
{
	int tmp = activeMenuItem;
	if(!SetButton(configuring, jk))
	{
		ELog("Error setting configure button %i!\n", configuring);
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

int ConfigMenuInit()
{
	EventMode(MENU);
	ConfigCreateItems();
	return 1;
}

void ConfigMenuQuit()
{
	ClearMenuItems();
}

void ConfigMenu()
{
	ShadedBox(minX-BBO, minY-BBO, maxX+BBO, maxY+BBO);
	DrawMenu();
}

void Quit()
{
	quit = 1; // causes main loop to exit
}

int QuitMenuInit()
{
	float c[4] = {0.0, 1.0, 1.0, 1.0};
	EventMode(MENU);
	menuX = 350;
	menuY = 200;
	CreateButton("Yes", Quit);
	CreateButton("No", MenuBack);
	CreateText("Really Quit?", c, 200, 200);
	activeMenuItem = 1;
	return 1;
}

void QuitMenuQuit()
{
	ClearMenuItems();
}

void QuitMenu()
{
	ShadedBox(minX-BBO, minY-BBO, maxX+BBO, maxY+BBO);
	DrawMenu();
}

#define NUMMENUS 6
Menu menus[NUMMENUS] = {	{NullMenuInit, NullMenuQuit, NullMenu, NULLMENU},
				{NoMenuInit, NoMenuQuit, NoMenu, NULLMENU},
				{MainMenuInit, MainMenuQuit, MainMenu, NOMENU},
				{FightMenuInit, FightMenuQuit, FightMenu, MAINMENU},
				{ConfigMenuInit, ConfigMenuQuit, ConfigMenu, MAINMENU},
				{QuitMenuInit, QuitMenuQuit, QuitMenu, MAINMENU}};

int FindActiveItem(MenuItem *items, int numItems)
{
	int x;
	for(x=0;x<numItems;x++)
	{
		if(items[x].type < SELECTABLE) return x;
	}
	return 0;
}

int SwitchMenu(int m)
{
	if(m < 0 || m >= NUMMENUS) return 0;
	BoundsCheck = MenuWrap;
	Log("Switching Menu: %i\n", m);
	if(activeMenu) activeMenu->QuitMenu();
	else NullMenuQuit(); // called for the first menu switch
	if(!menus[m].InitMenu())
	{
		activeMenu = &(menus[NULLMENU]);
		Log("Menu switch failed\n");
		return 0;
	}
	activeMenu = &(menus[m]);
	activeMenuItem = FindActiveItem(items, numItems);
	Log("Menu switched\n");
	return 1;
}

void DrawButton(GLuint button, int x, int y, int on)
{
	if(on) glColor3f(1.0, 1.0, 1.0);
	else glColor3f(0.0, .7, .7);
	GLBindTexture(GL_TEXTURE_2D, button);

	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0); glVertex2i(0+x, 0+y);
	glTexCoord2f(1.0, 0.0); glVertex2i(256+x, 0+y);
	glTexCoord2f(1.0, 1.0); glVertex2i(256+x, 64+y);
	glTexCoord2f(0.0, 1.0); glVertex2i(0+x, 64+y);
	glEnd();
}
