#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glob.h>

#include "GL/gl.h"

#include "menu.h"
#include "main.h"
#include "event.h"
#include "glfunc.h"
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
	//int tex;
	} Button;

typedef struct {
	int (*activeFunc)(int);
	int param;
	} ButtonParam;

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
int menuActive = 0;
MenuItem *items = NULL;

void DrawPartialMenu(int start, int stop)
{
	int x;
	if(start < 0 || start >= numItems) start = 0;
	if(stop < 0 || stop >= numItems) stop = numItems;
	for(x=start;x<stop;x++)
	{
		if(x == activeMenuItem) glColor3f(1.0, 0.0, 0.0);
		else glColor3f(1.0, 1.0, 1.0);
		switch(items[x].type)
		{
			case SLIDER:
				break;
			case BOOLEAN:
				break;
			case BUTTON:
			case BUTTONPARAM:
				PrintGL(menuX, menuY+(x-start)*FONT_HEIGHT, items[x].name);
				break;
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
				free(items[x].item);
				break;
			default:
				Log("Error: Invalid menu item type!\n");
				break;
		}
	}
	numItems = 0;
	activeMenuItem = 0;
	free(items);
	items = NULL;
	Log("All items freed.\n");
}

void CreateSlider(char *name, int min, int max, int delta, int initVal)
{
	Slider *s;
	s = (Slider*)malloc(sizeof(Slider));
	s->min = min;
	s->max = max;
	s->del = delta;
	s->val = initVal;
	AddMenuItem(name, (void*)s, SLIDER);
}

void CreateBoolean(char *name, char *trueString, char *falseString, int initVal)
{
	Boolean *b;
	b = (Boolean*)malloc(sizeof(Boolean));
	b->trueString = (char*)malloc(sizeof(char) * (strlen(trueString)+1));
	b->falseString = (char*)malloc(sizeof(char) * (strlen(falseString)+1));
	strcpy(b->trueString, trueString);
	strcpy(b->falseString, falseString);
	b->val = initVal;
	AddMenuItem(name, (void*)b, BOOLEAN);
}

void CreateButton(char *name, void (*activeFunc)())
{
	Button *b;
	b = (Button*)malloc(sizeof(Button));
	b->activeFunc = activeFunc;
	AddMenuItem(name, (void*)b, BUTTON);
}

void CreateButtonParam(char *name, int (*activeFunc)(int), int param)
{
	ButtonParam *b;
	b = (ButtonParam*)malloc(sizeof(ButtonParam));
	b->activeFunc = activeFunc;
	b->param = param;
	AddMenuItem(name, (void*)b, BUTTONPARAM);
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

void MenuDown()
{
	activeMenuItem++;
	if(activeMenuItem >= numItems)
	{
		if(BoundsCheck()) PlaySound(SND_wepnsel1);
	}
	else PlaySound(SND_wepnsel1);
}

void MenuUp()
{
	activeMenuItem--;
	if(activeMenuItem < 0)
	{
		if(BoundsCheck()) PlaySound(SND_wepnsel1);
	}
	else PlaySound(SND_wepnsel1);
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
	PlaySound(SND_spnray03);
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
	PlaySound(SND_spnray02);
	SwitchMenu(activeMenu->back);
}

void ShowMenu(void)
{
	if(menuActive) return;
	PlaySound(SND_spnray03);
	menuActive = 1;
	RegisterEvent(EVENT_UP, MenuUp, EVENTTYPE_STOP);
	RegisterEvent(EVENT_DOWN, MenuDown, EVENTTYPE_STOP);
	RegisterEvent(EVENT_LEFT, MenuDec, EVENTTYPE_STOP);
	RegisterEvent(EVENT_RIGHT, MenuInc, EVENTTYPE_STOP);
	RegisterEvent(EVENT_MENU, MenuBack, EVENTTYPE_STOP);
	RegisterEvent(EVENT_ENTER, MenuActivate, EVENTTYPE_STOP);
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

	GLBindTexture(GL_TEXTURE_2D, TEX_Title);
	glBegin(GL_QUADS);
	glColor3f(1.0, 1.0, 1.0);
	glTexCoord2f(0.0, 0.0); glVertex2i(0, 0);
	glTexCoord2f(1.0, 0.0); glVertex2i(512, 0);
	glTexCoord2f(1.0, 1.0); glVertex2i(512, 256);
	glTexCoord2f(0.0, 1.0); glVertex2i(0, 256);
	glEnd();

	DrawMenu();

	ResetProjection();
}

#define FILE_LIST_SIZE 10
glob_t globbuf;
int fileStart;

void FightActivate()
{
	CfgSetS("main.song", globbuf.gl_pathv[activeMenuItem]);
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
		PlaySound(SND_wepnsel1);
	}
}

void FightPageDown()
{
	if(fileStart < globbuf.gl_pathc - FILE_LIST_SIZE)
	{
		activeMenuItem += FILE_LIST_SIZE;
		MenuClamp();
		PlaySound(SND_wepnsel1);
	}
}

int FightMenuInit()
{
	int x;
	menuX = 200;
	menuY = 200;
	fileStart = 0;
	BoundsCheck = MenuClamp;
	if(glob("music/*", 0, NULL, &globbuf))
	{
		Error("Generating playlist");
		return 0;
	}
	for(x=0;x<globbuf.gl_pathc;x++)
	{
		if(fileStart+x == globbuf.gl_pathc) break;
		CreateButton(globbuf.gl_pathv[fileStart+x], FightActivate);
	}
	RegisterEvent(EVENT_PAGEUP, FightPageUp, EVENTTYPE_STOP);
	RegisterEvent(EVENT_PAGEDOWN, FightPageDown, EVENTTYPE_STOP);
	return 1;
}

void FightMenuQuit(void)
{
	globfree(&globbuf);
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
	if(fileStart < globbuf.gl_pathc - FILE_LIST_SIZE)
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
void ConfigCreateItems()
{
	int x;
	char *s;
	menuX = 150;
	menuY = 150;
	for(x=0;x<B_LAST;x++)
	{
		s = JoyKeyName(x);
		CreateButtonParam(s, ConfigButton, x);
		free(s);
	}
}

void ConfigKeyHandler(JoyKey *jk)
{
	int tmp = activeMenuItem;
	if(!SetButton(configuring, jk))
	{
		Log("Error setting configure button %i!\n", configuring);
	}
	DeregisterKeyEvent();
	EventMode(MENU);
	ClearMenuItems();
	ConfigCreateItems();
	activeMenuItem = tmp;
}

int ConfigButton(int b)
{
	configuring = b;
	RegisterKeyEvent(ConfigKeyHandler);
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
	glPushAttrib(GL_CURRENT_BIT);
	glColor3f(0.0, 1.0, 1.0);
	PrintGL(60, 150, "Up\nDown\nLeft\nRight\nButton 1\nButton 2\nButton 3\nButton 4\nMenu\n");
	glPopAttrib();
	DrawMenu();
}

void Quit()
{
	quit = 1; // causes main loop to exit
}

int QuitMenuInit()
{
	EventMode(MENU);
	menuX = 200;
	menuY = 200;
	CreateButton("Yes", Quit);
	CreateButton("No", MenuBack);
	activeMenuItem = 1;
	return 1;
}

void QuitMenuQuit()
{
	ClearMenuItems();
}

void QuitMenu()
{
	PrintGL(50, 200, "Really Quit? ");
	DrawMenu();
}

#define NUMMENUS 6
Menu menus[NUMMENUS] = {	{NullMenuInit, NullMenuQuit, NullMenu, NULLMENU},
				{NoMenuInit, NoMenuQuit, NoMenu, NULLMENU},
				{MainMenuInit, MainMenuQuit, MainMenu, NOMENU},
				{FightMenuInit, FightMenuQuit, FightMenu, MAINMENU},
				{ConfigMenuInit, ConfigMenuQuit, ConfigMenu, MAINMENU},
				{QuitMenuInit, QuitMenuQuit, QuitMenu, MAINMENU}};

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
