#include <stdio.h>
#include <math.h>
#include <glib.h>

#include "SDL_mixer.h"
#include "GL/gl.h"
#include "GL/glu.h"

#include "scene.h"
#include "cfg.h"
#include "event.h"
#include "glfunc.h"
#include "log.h"
#include "main.h"
#include "menu.h"
#include "module.h"
#include "particles.h"
#include "sounds.h"
#include "textures.h"
#include "timer.h"
#include "wam.h"
#include "../util/fatalerror.h"
#include "../util/memtest.h"
#include "../util/sdlfatalerror.h"
#include "../util/myrand.h"

#include "mikmod_internals.h" // test

#define MAXNUM 2

int NullInit();
void NullScene();
void NullQuit();
int IntroInit();
void IntroScene();
void IntroQuit();
int MainInit();
void MainScene();
void MainQuit();

#define NUMSCENES 3
Scene scenes[NUMSCENES] = {	{NullInit, NullQuit, NullScene},
				{IntroInit, IntroQuit, IntroScene},
				{MainInit, MainQuit, MainScene}};

float lightFull[4] = {1.0, 1.0, 1.0, 1.0};
float lightHalf[4] = {0.5, 0.5, 0.5, 0.5};
float lightNone[4] = {0.0, 0.0, 0.0, 1.0};
float lightNormal[4] = {0.8, 0.8, 0.8, 1.0};

int SwitchScene(int scene)
{
	if(scene < 0 || scene >= NUMSCENES) return 0;
	Log("Switching scene: %i\n", scene);
	if(activeScene) activeScene->QuitScene();
	if(!scenes[scene].InitScene())
	{
		activeScene = &(scenes[NULLSCENE]);
		ELog("Scene switch failed\n");
		return 0;
	}
	activeScene = &(scenes[scene]);
	Log("Scene switched\n");
	return 1;
}

int SceneActive(int scene)
{
	if(activeScene == &(scenes[scene])) return 1;
	return 0;
}

void NullScene() {}
void NullQuit() {}
int NullInit() {return 1;}

int IntroInit()
{
	return 1;
}

void IntroQuit()
{
}

void IntroScene()
{
	static float theta = 0.0;
  GLLoadIdentity();				// Reset The View

  GLTranslatef(-1.5f,0.0f,-6.0f);		// Move Left 1.5 Units And Into The Screen 6.0
	
  GLBindTexture(GL_TEXTURE_2D, TEX_FlatlandFiery);
  // draw a triangle
  glBegin(GL_POLYGON);				// start drawing a polygon
  glTexCoord2f(0.0, 0.0); glVertex3f( 0.0f, 1.0f, 0.0f);		// Top
  glTexCoord2f(1.0, 1.0); glVertex3f( 1.0f,-1.0f, 0.0f);		// Bottom Right
  glTexCoord2f(0.0, 1.0); glVertex3f(-1.0f,-1.0f, 0.0f);		// Bottom Left	
  glEnd();					// we're done with the polygon

  glTranslatef(3.0f,0.0f,0.0f);		        // Move Right 3 Units
	
  // draw a square (quadrilateral)
  glBegin(GL_QUADS);				// start drawing a polygon (4 sided)
  glTexCoord2f(0.0, 0.0); glVertex3f(-cos(theta), cos(theta), sin(theta));	// Top Left
  glTexCoord2f(1.0, 0.0); glVertex3f( cos(theta), cos(theta), sin(theta));	// Top Right
  glTexCoord2f(1.0, 1.0); glVertex3f( cos(theta),-cos(theta), sin(theta));	// Bottom Right
  glTexCoord2f(0.0, 1.0); glVertex3f(-cos(theta),-cos(theta), sin(theta));	// Bottom Left	
  glEnd();					// done with the polygon
  theta += .01;
  // swap buffers to display, since we're double buffered.
}

Uint32 rowTime;
Uint32 ticTime;
GLuint rowList;
GLuint noteList;
GLuint mainTexes[MAX_COLS];
int channelFocus = 0;
Wam *wam;	// note file

float theta = 0.0;

void ResetAp();

#define BLOCK_HEIGHT .75
#define TIC_HEIGHT .25
#define BLOCK_WIDTH 2.0
#define NOTE_WIDTH .75

int TIMEADJ = 1;
void MoveFaster(void)
{
	ticTime += 25000;
}
void MoveSlower(void)
{
	TIMEADJ = !TIMEADJ;
}

#define NEGATIVE_TICKS 7*6
#define POSITIVE_TICKS 57*6
#define NUM_TICKS 64*6
#define TIC_ERROR 5 // number of tics we can be off when hitting a button
#define LINES_PER_AP 8
#define ROWS_PER_LINE 4

#define NUM_LASERS 10
#define LASER_DECAY 3.0

#define UNMUTE 0
#define MUTE 1

// returns 1 if the row is valid, 0 otherwise
#define IsValidRow(row) ((row >= 0 && row < wam->numRows) ? 1 : 0)
// return a clamped row number
#define Row(row) (row < 0 ? 0 : (row >= wam->numRows ? wam->numRows - 1: row))
#define Max(a, b) ((a) > (b) ? (a) : (b))

int curTic; // tick counter from 0 - total ticks in the song
int firstVb, curVb, lastVb;	// tick counters for the three rows
int firstRow, curRow, lastRow;	// first row on screen, current row playing,
				// and the last row on screen

double partialTic;
typedef struct {
	Point pos;
	int tic;
	int col;
	} ScreenNote;
typedef struct {
	Point p1;
	Point p2;
	int color;
	int row;
	} Line;
typedef struct {
	Point p1;
	Point p2;
	float time;
	} Laser;
typedef struct {
	int startTic;	// first tic that we need to play
	int stopTic;	// last tic that we need to play
	int stopRow;	// corresponding row to stopTic
	int nextStartRow;//which row the game is cleared to.
	int lastTic;	// last note played is in lastTic
	int notesHit;	// number of notes we hit so far
	int notesTotal;	// total number of notes we need to play
	} AttackPattern;
typedef struct {
	double part;	// cumulative row adder, when >= 1.0 inc minRow
	int minRow;	// equal to cleared, but doesn't get set to 0
			// after the column is recreated
	int cleared;	// equals the last row this col is cleared to, 0 if
			// not cleared
	int hit;	// equals the tic of the last hit note
	int miss;	// equals the tic of the last missed note
	} AttackCol;

AttackPattern ap;
AttackCol ac[MAX_COLS];
ScreenNote *notesOnScreen; // little ring buffer of notes
GSList *unusedList;	// unused notes
GSList *notesList;	// notes on the screen, not hit
GSList *hitList;	// notes on the screen, hit
int numNotes;	// max number of notes on screen (wam->numCols * NUM_TICKS)
int score;
//float light[4] = {0.0, 1.0, -8.0, 1.0};
float light[4] = {0.0, 0.5, 0.0, 1.0};

Laser laser[NUM_LASERS];
int numLasers;
float bounceTime;
Line *linesOnScreen;
int startLine;
int stopLine;
int numLines;
int *noteOffset;
MikMod_player_t oldHand;
int tickCounter;
int songStarted;	// this is set once we get to the first row, and the
			// song is unpaused

void ChannelUp()
{
	if(channelFocus + 1 < wam->numCols)
	{
		channelFocus++;
		ResetAp();
		ac[channelFocus].hit = ap.startTic - 1;
	}
}

void ChannelDown()
{
	if(channelFocus != 0)
	{
		channelFocus--;
		ResetAp();
		ac[channelFocus].hit = ap.startTic - 1;
	}
}

Column *ColumnFromNum(int col)
{
	return &wam->patterns[wam->rowData[Row(curRow)].patnum].columns[col];
}

void Setmute(Column *c, int mute)
{
	int x;
	for(x=0;x<c->numchn;x++)
	{
		if(mute) Player_Mute(c->chan[x]);
		else Player_Unmute(c->chan[x]);
	}
}

// update which channels are playing based on AttackCol coontents
void UpdateModule()
{
	int x;
	for(x=0;x<wam->numCols;x++)
	{
		if(ac[x].cleared || ac[x].hit > ac[x].miss) Setmute(ColumnFromNum(x), UNMUTE);
		else Setmute(ColumnFromNum(x), MUTE);
	}
	Setmute(&wam->patterns[wam->rowData[Row(curRow)].patnum].unplayed, UNMUTE);
}

void AddNotes(int row)
{
	int x;
	int tic;
	ScreenNote *sn;
	if(row < 0 || row >= wam->numRows) return;
	tic = wam->rowData[row].ticpos;
	for(x=0;x<wam->numCols;x++)
	{
		if(wam->rowData[row].notes[x])
		{
			sn = unusedList->data;
			if(row < ac[x].minRow)
			{
				hitList = g_slist_append(hitList, sn);
			}
			else
			{
				notesList = g_slist_append(notesList, sn);
			}
			unusedList = g_slist_remove(unusedList, sn);
			sn->pos.x = -x * BLOCK_WIDTH - NOTE_WIDTH * (double)noteOffset[(int)wam->rowData[row].notes[x]];
			sn->pos.y = 0.0;
			sn->pos.z = TIC_HEIGHT * (double)wam->rowData[row].ticpos;
			sn->tic = tic;
			sn->col = x;
		}
	}
}

GSList *RemoveList(GSList *list, int tic)
{
	ScreenNote *sn;
	while(list)
	{
		sn = (ScreenNote*)list->data;
		if(sn->tic != tic) break;
		unusedList = g_slist_append(unusedList, list->data);
		list = g_slist_next(list);
	}
	return list;
}

void RemoveNotes(int row)
{
	int tic;
	if(row < 0 || row >= wam->numRows) return;
	tic = wam->rowData[row].ticpos;
	Log("RM\n");
	notesList = RemoveList(notesList, tic);
	Log("RMHit: %i\n", hitList);
	hitList = RemoveList(hitList, tic);
	Log("done: %i\n", hitList);
}

void AddLine(int row)
{
	if(row < 0 || row >= wam->numRows) return;
	if(wam->rowData[row].line)
	{
		linesOnScreen[stopLine].p1.x = 1.0;
		linesOnScreen[stopLine].p1.y = 0.0;
		linesOnScreen[stopLine].p1.z = TIC_HEIGHT * (double)wam->rowData[row].ticpos;
		linesOnScreen[stopLine].p2.x = 1.0 - 2.0 * wam->numCols;
		linesOnScreen[stopLine].p2.y = 0.0;
		linesOnScreen[stopLine].p2.z = TIC_HEIGHT * (double)wam->rowData[row].ticpos;
		linesOnScreen[stopLine].row = row;
		linesOnScreen[stopLine].color = wam->rowData[row].line;
		stopLine++;
		if(stopLine == numLines) stopLine = 0;
	}
}

void RemoveLine(int row)
{
	if(row < 0 || row >= wam->numRows) return;
	while(linesOnScreen[startLine].row == row)
	{
		linesOnScreen[startLine].row = -1;
		startLine++;
		if(startLine == numLines) startLine = 0;
	}
}

void SetMute(/*ModChannel *c, */int mute)
{
/*	int j;
	for(j=0;j<c->numModChannels;j++)
	{
		if(mute) Player_Mute(c->modChannels[j]);
		else Player_Unmute(c->modChannels[j]);
	}*/
}

void RandomColor(float col[4])
{
	int x = (int)(7.0 * rand() / (RAND_MAX+1.0)) + 1;
	col[ALPHA] = 1.0;
	col[RED] = 0.0;
	col[GREEN] = 0.0;
	col[BLUE] = 0.0;
	if(x&1) col[RED] = 1.0;
	if(x&2) col[GREEN] = 1.0;
	if(x&4) col[BLUE] = 1.0;
}

ScreenNote *FindNote(GSList *list, int tic, int col)
{
	ScreenNote *sn;
	while(list)
	{
		sn = (ScreenNote*)list->data;
		if(sn->tic == tic && sn->col == col) return sn;
		list = g_slist_next(list);
	}
	return NULL;
}

void Press(int button)
{
	int i;
	int noteHit = 0;
	ScreenNote *sn;
	Row *r;

	// p1 is set to the light position
	laser[numLasers].p1.x = light[0];
	laser[numLasers].p1.y = light[1];
	laser[numLasers].p1.z = light[2];

	// p2 is set to where the note is
	laser[numLasers].p2.x = -channelFocus * BLOCK_WIDTH - NOTE_WIDTH * noteOffset[button];
	laser[numLasers].p2.y = 0.0;
	laser[numLasers].p2.z = TIC_HEIGHT * ((double)curTic + partialTic);
	laser[numLasers].time = 1.0;
	numLasers++;
	if(numLasers >= NUM_LASERS) numLasers = 0;

	for(i=-1;i<=1;i++)
	{
		if(curRow + i >= 0 && curRow + i < wam->numRows)
		{
			r = &wam->rowData[curRow+i];
			// if we're in the attackpattern limits, and
			// if we didn't already play this row, and this row
			// is within our acceptable error, and we hit the
			// right note, then yay
			if(	
				r->ticpos >= ap.startTic &&
				r->ticpos < ap.stopTic &&
				r->ticpos > ap.lastTic &&
				abs(r->ticpos - curTic) <= TIC_ERROR &&
				button == r->notes[channelFocus])
			{
				sn = FindNote(notesList, r->ticpos, channelFocus);
				if(!sn)
				{
					ELog("Error: Struck note not found!\n");
					break;
				}
				notesList = g_slist_remove(notesList, (gpointer)sn);
				unusedList = g_slist_append(unusedList, (gpointer)sn);
				ap.lastTic = r->ticpos;
				ap.notesHit++;
				if(ap.notesHit == ap.notesTotal)
				{
					ap.nextStartRow = ap.stopRow;
					ac[channelFocus].cleared = ap.stopRow + LINES_PER_AP * ROWS_PER_LINE * wam->numCols;
					ac[channelFocus].minRow = curRow;
					ac[channelFocus].part = 0.0;
				}
				ac[channelFocus].hit = r->ticpos;
				noteHit = 1;
				break;
			}
		}
	}

	r = &wam->rowData[Row(curRow)];
	if(!noteHit && r->ticpos >= ap.startTic && r->ticpos < ap.stopTic) // oops, we missed!
	{
		ResetAp();
		if(IsValidRow(curRow) && r->ticpos >= ap.startTic && r->ticpos < ap.stopTic)
			ac[channelFocus].miss = r->ticpos;
	}
	UpdateModule();
}

// i really should fix the event system...i suck!
void Press1() {Press(1);}

void Press2() {Press(2);}

void Press3() {Press(4);} // yes, this is really 4 (3rd bit)

void Press4()
{
	printf("Autocatcher! Hehe\n");
}

void SetMainView()
{
	float mainPos[3] = {0.0, 3.0, -8.0};
	float mainView[3] = {0.0, 0.8, 0.0};

	mainView[2] = TIC_HEIGHT * ((double)curTic + partialTic);
	mainPos[2] = mainView[2] - 8.0;

	glLoadIdentity();
	gluLookAt(	mainPos[0] - channelFocus * BLOCK_WIDTH, mainPos[1], mainPos[2],
			mainView[0] - channelFocus * BLOCK_WIDTH, mainView[1], mainView[2],
			0.0, 1.0, 0.0);
}

void FixVb(int *vb, int *row)
{
	*row = 0;
	while(1)
	{
		if(*row < 0 && *vb >= wam->rowData[0].sngspd)
		{
			(*row)++;
			*vb -= wam->rowData[0].sngspd;
		}
		else if(*row >= 0 && *vb >= wam->rowData[*row].sngspd)
		{
			(*row)++;
			*vb -= wam->rowData[*row].sngspd;
		}
		else if(*vb < 0)
		{
			(*row)--;
			*vb += wam->rowData[0].sngspd;
		}
		else break;
	}
}

// keep track of the number of actual ticks played by the mod player,
// so we know if we're off
void TickHandler(void)
{
	if(songStarted)
	{
		if(!Player_Paused()) tickCounter++;
	}
	oldHand();
}

int MainInit()
{
	int x;
	Log("Load Wam\n");
	wam = LoadWam(CfgS("main.song"));
	if(wam == NULL)
	{
		ELog("Error: Couldn't load WAM file\n");
		return 0;
	}
	Log("Start module\n");
	if(!StartModule(CfgS("main.song")))
	{
		ELog("Error: Couldn't start module\n");
		return 0;
	}
	// module and module data (where to place the notes) are now loaded,
	// and the module is paused
	Log("Module ready\n");
	tickCounter = 0;
	score = 0;
	songStarted = 0;
	numLasers = 0;
	oldHand = MikMod_RegisterPlayer(TickHandler);
	noteOffset = (int*)malloc(sizeof(int) * (MAX_NOTE+1));

	noteOffset[1] = -1;
	noteOffset[2] = 0;
	noteOffset[4] = 1;

	rowTime = 0;
	ticTime = 0;
	channelFocus = 0;
	theta = 0.0;

	for(x=0;x<wam->numCols;x++)
	{
		ac[x].part = 0.0;
		ac[x].minRow = 0;
		ac[x].cleared = 0;
		ac[x].hit = -1;
		ac[x].miss = -2;
	}

	// start back about 3.5 seconds worth of ticks
	// doesn't need to be exact, we just need some time for
	// the player to get ready
	curTic = (int)(-3500.0 * (double)wam->rowData[0].bpm / 2500.0);
	// set the tic positions of where we can see and where we're looking
	firstVb = curTic - NEGATIVE_TICKS;
	curVb = curTic;
	lastVb = curTic + POSITIVE_TICKS;
	// convert tic values to rows, now the ticks are all within
	// [0 .. wam->rowData[xRow].sngspd - 1]
	FixVb(&firstVb, &firstRow);
	FixVb(&curVb, &curRow);
	FixVb(&lastVb, &lastRow);

	partialTic = 0.0;

	numNotes = wam->numCols * NUM_TICKS;
	notesOnScreen = (ScreenNote*)malloc(sizeof(ScreenNote) * numNotes);
	unusedList = NULL;
	notesList = NULL;
	hitList = NULL;
	for(x=0;x<numNotes;x++)
	{
		unusedList = g_slist_append(unusedList, (gpointer)&notesOnScreen[x]);
	}
	for(x=0;x<=lastRow;x++) AddNotes(x);

	numLines = NUM_TICKS;
	linesOnScreen = (Line*)malloc(sizeof(Line) * numLines);
	startLine = 0;
	stopLine = 0;
	for(x=0;x<=lastRow;x++) AddLine(x);

	ap.nextStartRow = -1;
	ResetAp();
	RegisterEvent(EVENT_RIGHT, ChannelUp, EVENTTYPE_MULTI);
	RegisterEvent(EVENT_LEFT, ChannelDown, EVENTTYPE_MULTI);
	RegisterEvent(EVENT_UP, MoveFaster, EVENTTYPE_MULTI);
	RegisterEvent(EVENT_DOWN, MoveSlower, EVENTTYPE_MULTI);
	RegisterEvent(EVENT_BUTTON1, Press1, EVENTTYPE_MULTI);
	RegisterEvent(EVENT_BUTTON2, Press2, EVENTTYPE_MULTI);
	RegisterEvent(EVENT_BUTTON3, Press3, EVENTTYPE_MULTI);
	RegisterEvent(EVENT_BUTTON4, Press4, EVENTTYPE_MULTI);
	Log("Creating lists\n");
	rowList = GLGenLists(wam->numCols);
	noteList = GLGenLists(1);

	mainTexes[0] = TEX_Slate;
	mainTexes[1] = TEX_Walnut;
	mainTexes[2] = TEX_Parque;
	mainTexes[3] = TEX_Clovers;
	mainTexes[4] = TEX_Lava;
	mainTexes[5] = TEX_Slate;
	mainTexes[6] = TEX_Walnut;
	mainTexes[7] = TEX_Parque;

	for(x=0;x<wam->numCols;x++)
	{
		glNewList(rowList+x, GL_COMPILE);
		{
			glColor4f(1.0, 1.0, 1.0, 1.0);
			glNormal3f(0.0, 1.0, 0.0);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, lightNormal);
			glBegin(GL_QUADS);
			{
				glTexCoord2f(0.0, (double)x/4.0);
				glVertex3f(-1.0, 0.0, 0.0);
				glTexCoord2f(1.0, (double)x/4.0);
				glVertex3f(1.0, 0.0, 0.0);
				glTexCoord2f(1.0, (double)(x+1)/4.0);
				glVertex3f(1.0, 0.0, BLOCK_HEIGHT);
				glTexCoord2f(0.0, (double)(x+1)/4.0);
				glVertex3f(-1.0, 0.0, BLOCK_HEIGHT);
			} glEnd();
		} glEndList();
	}

	glNewList(noteList, GL_COMPILE);
	{
		glDisable(GL_TEXTURE_2D);
		glColor4f(0.3, 0.7, 0.6, 1.0);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, lightNormal);
		glBegin(GL_TRIANGLE_FAN);
		{
			glNormal3f( 0.0, 1.0, 0.0);
			glVertex3f( 0.0, 0.15, 0.0);

			glNormal3f( 0.2, 0.5, 0.0);
			glVertex3f( 0.2, 0.0, 0.2);
			glVertex3f( 0.2, 0.0, -0.2);

			glNormal3f( 0.0, 0.5, -0.2);
			glVertex3f( 0.2, 0.0, -0.2);
			glVertex3f(-0.2, 0.0, -0.2);
			
			glNormal3f(-0.2, 0.5, 0.0);
			glVertex3f(-0.2, 0.0, -0.2);
			glVertex3f(-0.2, 0.0, 0.2);

			glNormal3f(0.0, 0.5, 0.2);
			glVertex3f(-0.2, 0.0, 0.2);
			glVertex3f(0.2, 0.0, 0.2);

		} glEnd();
		glEnable(GL_TEXTURE_2D);
		glPopMatrix();
	} glEndList();
	InitTimer();
	Log("Lists created\n");
	return 1;
}

void MainQuit()
{
	Log("Main Scene quit\n");
	oldHand = MikMod_RegisterPlayer(oldHand);
	Log("A\n");
	if(songStarted)
	{
		DeregisterEvent(EVENT_SHOWMENU, Player_TogglePause);
		DeregisterEvent(EVENT_HIDEMENU, Player_TogglePause);
	}
	Log("A\n");
	songStarted = 0;
	DeregisterEvent(EVENT_BUTTON1, Press1);
	DeregisterEvent(EVENT_BUTTON2, Press2);
	DeregisterEvent(EVENT_BUTTON3, Press3);
	DeregisterEvent(EVENT_BUTTON4, Press4);
	Log("A\n");
	DeregisterEvent(EVENT_RIGHT, ChannelUp);
	Log("A\n");
	DeregisterEvent(EVENT_LEFT, ChannelDown);
	Log("A\n");
	DeregisterEvent(EVENT_UP, MoveFaster);
	Log("A\n");
	DeregisterEvent(EVENT_DOWN, MoveSlower);
	Log("A\n");
	GLDeleteLists(rowList, wam->numCols);
	Log("A\n");
	GLDeleteLists(noteList, 1);
	Log("A\n");
	free(notesOnScreen);
	Log("A\n");
	free(linesOnScreen);
	Log("A\n");
	free(noteOffset);
	Log("A\n");
	ClearParticles();
	Log("A\n");
	CheckObjs();
	Log("A\n");
	FreeWam(wam);
	Log("A\n");
	StopModule();
	Log("Main scene quit finished\n");
}

void DrawLine(int line)
{
	if(linesOnScreen[line].color == 2) glColor4f(0.0, 0.0, 1.0, .7);
	else glColor4f(0.8, 0.8, 0.8, .3);
	glVertex3f( linesOnScreen[line].p1.x, linesOnScreen[line].p1.y, linesOnScreen[line].p1.z);
	glVertex3f( linesOnScreen[line].p2.x, linesOnScreen[line].p2.y, linesOnScreen[line].p2.z);
}

void DrawLines()
{
	int x;
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glNormal3f(0.0, 1.0, 0.0);
	glBegin(GL_LINES);
	if(startLine <= stopLine)
	{
		for(x=startLine;x<stopLine;x++)
			DrawLine(x);
	}
	else
	{
		for(x=startLine;x<numLines;x++)
			DrawLine(x);
		for(x=0;x<stopLine;x++)
			DrawLine(x);
	}
	glEnd();
	glEnable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
}

// gets either curRow or curRow+1, depending on which one tic is closest to
// (eg within acceptable error)
int RowByTic(int tic)
{
	if(tic - wam->rowData[Row(curRow)].ticpos < TIC_ERROR) return curRow;
	return curRow+1;
}

void ResetAp()
{
	int start;
	int end;
	int numLines = 0;
	ap.notesHit = 0;
	ap.notesTotal = 0;
	start = Row(Max(Max(RowByTic(curTic), ap.nextStartRow), ac[channelFocus].cleared));
	while(start < wam->numRows && wam->rowData[start].line == 0) start++;
	end = start;
	while(numLines < LINES_PER_AP && end < wam->numRows)
	{
		// don't count the note on the last row, since that will
		// be the beginning of the next "AttackPattern"
		if(wam->rowData[end].notes[channelFocus]) ap.notesTotal++;
		end++;
		if(wam->rowData[end].line != 0) numLines++;
	}
	if(numLines < LINES_PER_AP)
	{
		// at the end of the song we can't find a valid pattern to do
		ap.startTic = -1;
		ap.stopTic = -1;
		ap.lastTic = -1;
		ap.stopRow = -1;
		return;
	}
	Log("StarT: %i, End: %i\n", start, end);
	ap.startTic = wam->rowData[start].ticpos;
	ap.stopTic = wam->rowData[end].ticpos;
	ap.lastTic = wam->rowData[start].ticpos - 1;
	ap.stopRow = end;
}

void CheckMissedNotes()
{
	ScreenNote *sn;
	GSList *list;
	list = notesList;
	while(list)
	{
		sn = (ScreenNote*)list->data;
		if(	curTic - sn->tic > TIC_ERROR &&
			sn->tic > ac[sn->col].miss)
		{
			ac[sn->col].miss = sn->tic;
			if(sn->col == channelFocus && sn->tic >= ap.startTic && sn->tic < ap.stopTic)
				ResetAp();
		}
		list = g_slist_next(list);
	}
}

void CheckColumn(int row)
{
	int x;
	for(x=0;x<wam->numCols;x++)
	{
		if(ac[x].cleared == row) ac[x].cleared = 0;
	}
}

gint NoteListTic(gconstpointer snp, gconstpointer tp)
{
	int tic = (int)tp;
	ScreenNote *sn = (ScreenNote*)snp;
	return sn->tic - tic;
}

gint SortByTic(gconstpointer a, gconstpointer b)
{
	ScreenNote *an = (ScreenNote*)a;
	ScreenNote *bn = (ScreenNote*)b;
	return an->tic - bn->tic;
}

void MoveHitNotes(int tic, int col)
{
	ScreenNote *sn;
	GSList *tmp;
	GSList *holder = NULL;
	Log("moveHIt\n");
	tmp = g_slist_find_custom(notesList, (gpointer)tic, NoteListTic);
	while(tmp)
	{
		sn = (ScreenNote*)tmp->data;

		if(sn->tic == tic && sn->col == col)
		{
			hitList = g_slist_insert_sorted(hitList, sn, SortByTic);
			holder = g_slist_append(holder, sn);
		}
		tmp = g_slist_next(tmp);
	}
	tmp = holder;
	while(tmp)
	{
		notesList = g_slist_remove(notesList, tmp->data);
		tmp = g_slist_next(tmp);
	}
	g_slist_free(holder);
	Log("movehit\n");
}

void UpdateClearedCols()
{
	int x;
	int tic;
	float col[4];
	Obj *o;
	for(x=0;x<wam->numCols;x++)
	{
		ac[x].part += (double)ticDiff / 40.0;
		while(ac[x].part >= 1.0 && ac[x].minRow < ac[x].cleared)
		{
			tic = wam->rowData[Row(ac[x].minRow)].ticpos;
			MoveHitNotes(tic, x);
			ac[x].part -= 1.0;
			ac[x].minRow++;
			o = NewObj();
			o->pos.x = -x * 2.0;
			o->pos.z = TIC_HEIGHT * (double)tic;
			o->vel.x = FloatRand() - 0.5;
			o->vel.y = 2.0 + FloatRand();
			o->vel.z = 13.0 + FloatRand();
			o->rotvel = FloatRand() * 720.0 - 360.0;
			o->acc.y = -3.98;
			RandomColor(col);
			CreateParticle(o, col, P_StarBurst, 1.0);
		}

		// no point in going beyond where we can see
//		if(ac[x].minRow >= lastRow) ac[x].minRow = ac[x].cleared;
	}
}

void UpdatePosition()
{
	int tmpAdj;
	// calculate the amount of ticTime elapsed
	// every 2500 ticTime is one tick
	if(!menuActive) ticTime += ticDiff * wam->rowData[Row(curRow)].bpm;

	// adjust the ticTime if our time is different from the
	// song time.  This is needed in case a little blip in the process
	// causes the song to be ahead or behind the game, so we can
	// right ourselves.
	if(songStarted)
	{
		tmpAdj = (tickCounter - curTic) << 5;
		if(!TIMEADJ) tmpAdj = 0;
		Log("Adj: %i\n", (tickCounter-curTic) << 5);
		if((signed)ticTime + tmpAdj < 0) ticTime = 0;
		else ticTime += tmpAdj;
	}

	while(ticTime >= 2500)
	{
		ticTime -= 2500;
		curTic++;
		curVb++;
		firstVb++;
		lastVb++;
		CheckMissedNotes();
		if(curVb >= wam->rowData[Row(curRow)].sngspd)
		{
			curVb -= wam->rowData[Row(curRow)].sngspd;
			curRow++;
			if(curRow > ap.stopRow) ResetAp();
			CheckColumn(Row(curRow));
			if(curRow == 0) // start the song!
			{
				Player_TogglePause();
				songStarted = 1;
				RegisterEvent(EVENT_SHOWMENU, Player_TogglePause, EVENTTYPE_MULTI);
				RegisterEvent(EVENT_HIDEMENU, Player_TogglePause, EVENTTYPE_MULTI);
			}
		}
		UpdateModule();

		if(firstVb >= wam->rowData[Row(firstRow)].sngspd)
		{
			firstVb -= wam->rowData[Row(firstRow)].sngspd;
			// the remove functions check to make sure
			// the row is valid, so it's ok to pass firstRow
			RemoveNotes(firstRow);
			RemoveLine(firstRow);
			firstRow++;
		}

		if(lastVb >= wam->rowData[Row(lastRow)].sngspd)
		{
			lastVb -= wam->rowData[Row(lastRow)].sngspd;
			lastRow++;
			// the add functions check to make sure the
			// row is valid, so it's ok to pass lastRow
			AddNotes(lastRow);
			AddLine(lastRow);
		}
	}
	UpdateClearedCols();
	partialTic = (double)ticTime / 2500.0;
}

void DrawRows(double startTic, double stopTic)
{
	int col;
	double start, stop;
	if(startTic >= stopTic) return;
	glPushMatrix();
	glDisable(GL_LIGHTING);
	for(col=0;col<wam->numCols;col++)
	{
		start = startTic;
		stop = stopTic;
		glBindTexture(GL_TEXTURE_2D, mainTexes[col]);
		if(wam->rowData[Row(ac[col].minRow)].ticpos > start)
			start = wam->rowData[Row(ac[col].minRow)].ticpos;
		if(col == channelFocus && ap.startTic != -1)
		{
			if(ap.startTic > start) start = ap.startTic;
			if(ap.stopTic < stop) stop = ap.stopTic;
		}
		if(start >= stop)
		{
			glTranslated(-BLOCK_WIDTH, 0, 0);
			continue;
		}
		start *= TIC_HEIGHT;
		stop *= TIC_HEIGHT;
		glBegin(GL_QUADS);
		{
			glTexCoord2f(0.0, start/ 4.0);
			glVertex3f(-1.0, 0.0, start);
			glTexCoord2f(1.0, start / 4.0);
			glVertex3f(1.0, 0.0, start);
			glTexCoord2f(1.0, stop / 4.0);
			glVertex3f(1.0, 0.0, stop);
			glTexCoord2f(0.0, stop / 4.0);
			glVertex3f(-1.0, 0.0, stop);
		} glEnd();

		glTranslated(-BLOCK_WIDTH, 0, 0);
	}
	glEnable(GL_LIGHTING);
	glPopMatrix();
}

void DrawNote(gpointer snp, gpointer not_used)
{
	float temp[4] = {.7, 0.0, 0.0, 1.0};
	ScreenNote *sn = (ScreenNote*)snp;

	glPushMatrix();
	glTranslated(	sn->pos.x,
			sn->pos.y,
			sn->pos.z+0.3);
	if(abs(sn->tic - curTic) <= TIC_ERROR)
		glMaterialfv(GL_FRONT, GL_EMISSION, temp);
	glRotatef(theta, 0.0, 1.0, 0.0);
	Log("Dn\n");
	glCallList(noteList);
	glMaterialfv(GL_FRONT, GL_EMISSION, lightNone);
}

void DrawHitNote(gpointer snp, gpointer not_used)
{
	ScreenNote *sn = (ScreenNote*)snp;

	glPushMatrix();
	glTranslated(	sn->pos.x,
			sn->pos.y,
			sn->pos.z+0.3);
	if(sn->tic - curTic <= 0)
		glColor4f(1.0, 1.0, 1.0, 1.0);
	else
		glColor4f(0.5, 0.5, 0.5, 1.0);
	glCallList(plist+P_BlueNova);
}

void DrawNotes()
{
	Log("DN: %i, %i, %i\n", g_slist_length(notesList), g_slist_length(hitList), g_slist_length(unusedList));
	g_slist_foreach(notesList, DrawNote, NULL);
	Log("dn\n");
}

void DrawHitNotes()
{
	g_slist_foreach(hitList, DrawHitNote, NULL);
}

void DrawTargets()
{
	int x;
	glPushMatrix();
	glTranslated((double)channelFocus * -BLOCK_WIDTH, 0.0, TIC_HEIGHT * ((double)curTic + partialTic));
	glBindTexture(GL_TEXTURE_2D, TEX_Target);
	glTranslated(-NOTE_WIDTH, 0.0, 0.0);
	glNormal3f(0.0, 1.0, 0.0);
	for(x=-1;x<=1;x++)
	{
		glBegin(GL_QUADS);
		{
			glTexCoord2f(0.0, 0.0);
			glVertex3f(-0.25, 0.01, -0.25);
			glTexCoord2f(1.0, 0.0);
			glVertex3f(0.25, 0.01, -0.25);
			glTexCoord2f(1.0, 1.0);
			glVertex3f(0.25, 0.01, 0.25);
			glTexCoord2f(0.0, 1.0);
			glVertex3f(-0.25, 0.01, 0.25);
		} glEnd();
		glTranslated(NOTE_WIDTH, 0.0, 0.0);
	}
	glPopMatrix();
}

void DrawScoreboard()
{
//	int x;
	glColor4f(1.0, 1.0, 1.0, 1.0);
//	for(x=0;x<wam->numCols;x++)
//	{
//		PrintGL(0, 75+x*14, "Col %i  Hit %i, %i Clear %4i", x, ac[x].hit, ac[x].miss, ac[x].cleared);
//	}
	PrintGL(50, 0, "Playing: %s", mod->songname);
	if(curRow == wam->numRows) PrintGL(50, 15, "Song complete!");
	else if(curRow >= 0)
	{
		if(!TIMEADJ) glColor4f(1.0, 0.0, 0.0, 1.0);
		PrintGL(50, 15, "Song: %i (%i)/%i, Row: %i (%i)/%i Pattern: %i/%i", mod->sngpos, wam->rowData[curRow].sngpos, mod->numpos, mod->patpos, wam->rowData[curRow].patpos, NumPatternsAtSngPos(mod->sngpos),  mod->positions[mod->sngpos], mod->numpat);
		if(!TIMEADJ) glColor4f(1.0, 1.0, 1.0, 1.0);
	}
	else
	{
		int timeLeft = (int)(0.5 + -2500.0 * (double)wam->rowData[0].sngspd * ((double)curRow) / (1000.0 * (double)wam->rowData[0].bpm));
		if(timeLeft > 0) PrintGL(50, 15, "%i...", timeLeft);
		else PrintGL(50, 15, "GO!!");
	}
	if(curRow >= 0 && curRow < wam->numRows) PrintGL(0, 62, "Tick: %i, %i.%f / %i\n", wam->rowData[curRow].ticpos, curTic, partialTic, wam->numTics);
	PrintGL(50, 30, "Speed: %i/%i at %i\n", mod->vbtick, mod->sngspd, mod->bpm);
	PrintGL(0, 50, "%i - %i, note: %i, hit: %i/%i\n", ap.startTic, ap.stopTic, ap.lastTic, ap.notesHit, ap.notesTotal);
	PrintGL(400, 50, "DN: %i, %i, %i\n", g_slist_length(notesList), g_slist_length(hitList), g_slist_length(unusedList));
}

double LaserAdj(double a, double b, double dt)
{
	return a * (1.0 - dt) + b * dt;
}

void DrawLaser(Laser *l)
{
	glColor4f(1.0, 0.0, 1.0, l->time);
	glBegin(GL_QUADS);
	{
		glTexCoord2f(0.0, 0.0);
		glVertex3f(l->p1.x-0.1, l->p1.y, l->p1.z);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(l->p1.x+0.1, l->p1.y, l->p1.z);

		glTexCoord2f(1.0, 1.0);
		glVertex3f(l->p2.x+0.1, l->p2.y, l->p2.z);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(l->p2.x-0.1, l->p2.y, l->p2.z);
	} glEnd();
	l->p1.x = LaserAdj(l->p1.x, l->p2.x, timeDiff * 4.0);
	l->p1.y = LaserAdj(l->p1.y, l->p2.y, timeDiff * 4.0);
	l->p1.z = LaserAdj(l->p1.z, l->p2.z, timeDiff * 4.0);
	l->time -= timeDiff * LASER_DECAY;
}

void DrawLasers()
{
	int x;
	glBindTexture(GL_TEXTURE_2D, TEX_Laser);
	for(x=0;x<NUM_LASERS;x++)
	{
		DrawLaser(&laser[x]);
	}
}

void MainScene()
{
	float temp[4] = {.35, 0.0, 0.0, .5};
	float sintmp;
	Row *row;
//	int x;

	Log("MainScene\n");

	glLoadIdentity();
	glPushMatrix();

	Log("U");
	if(curRow != wam->numRows) UpdatePosition();
	Log("1");
	UpdateObjs(timeDiff);
	Log("u");
	SetMainView();

	row = &wam->rowData[Row(curRow)];
	bounceTime = 2.0 * 3.1415 * ((double)row->ticprt + (double)curTic - (double)row->ticpos + partialTic) / (double)row->ticgrp;
	sintmp = sin(bounceTime);
	light[0] = -BLOCK_WIDTH * channelFocus + cos(bounceTime);
	light[1] = 1.0 + sintmp * sintmp;
//	light[2] = TIC_HEIGHT * ((double)curTic + partialTic) - sintmp * sintmp - 3.0;
	light[2] = TIC_HEIGHT * ((double)curTic + partialTic);
	glLightfv(GL_LIGHT1, GL_POSITION, light);

	glColor4f(1.0, 1.0, 1.0, 1.0);

	Log("A");
	// usually we draw from -NEGATIVE_TICKS to +POSITIVE_TICKS, with the
	// notes currently being played at position 0.
	// At the beginning of the song, we start drawing instead from 
	// 0 to +POSITIVE_TICKS, and at the end of the song we draw from
	// -NEGATIVE_TICKS to wam->numTics.  Of course, if the song is less than
	// NEGATIVE_TICKS + POSITIVE_TICKS long, some other combinations will
	// arise :)
	DrawRows(
			// start
			curTic - NEGATIVE_TICKS >= 0 ?
			((double)curTic + partialTic - NEGATIVE_TICKS) :
			0.0,
			// end
			curTic < wam->numTics - POSITIVE_TICKS ?
			(double)curTic+partialTic+POSITIVE_TICKS :
			(double)wam->numTics);

	Log("B");
	DrawNotes();
	Log("C");
	DrawLines();
	Log("D");

	DrawTargets();
	Log("E");
	DrawScoreboard();
	Log("F");

	StartParticles();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	DrawHitNotes();
	DrawLasers();
	Log("G");
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	DrawParticles();
	Log("g");
	StopParticles();

	Log("L");

	glBindTexture(GL_TEXTURE_2D, TEX_Fireball);
	temp[0] *= 3.0;
	glMaterialfv(GL_FRONT, GL_EMISSION, temp);
	glNormal3f(0.0, 1.0, 0.0);
	glBegin(GL_QUADS);
	{
		glTexCoord2f(0.0, 0.0);
		glVertex3f(light[0]-.5, light[1]-.5, light[2]);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(light[0]+.5, light[1]-.5, light[2]);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(light[0]+.5, light[1]+.5, light[2]);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(light[0]-.5, light[1]+.5, light[2]);
	} glEnd();
	glMaterialfv(GL_FRONT, GL_EMISSION, lightNone);
	glPopMatrix();
	glDepthMask(GL_TRUE);

	theta += timeDiff * 120.0;
	Log("H");
	Log("endMainScene\n");
}
