#include <stdio.h>
#include <math.h>

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
#include "phys.h"
#include "sounds.h"
#include "textures.h"
#include "timer.h"
#include "../util/memtest.h"
#include "../util/sdlfatalerror.h"

#include "mikmod_internals.h" // test

#define MAXNUM 2

int NullInit();
void NullScene();
int IntroInit();
void IntroScene();
void IntroQuit();
int MainInit();
void MainScene();
void MainQuit();

#define NUMSCENES 3
Scene scenes[NUMSCENES] = {	{NullInit, NullScene, NullScene},
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
		Log("Scene switch failed\n");
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
  GLBegin(GL_POLYGON);				// start drawing a polygon
  GLTexCoord2f(0.0, 0.0); GLVertex3f( 0.0f, 1.0f, 0.0f);		// Top
  GLTexCoord2f(1.0, 1.0); GLVertex3f( 1.0f,-1.0f, 0.0f);		// Bottom Right
  GLTexCoord2f(0.0, 1.0); GLVertex3f(-1.0f,-1.0f, 0.0f);		// Bottom Left	
  GLEnd();					// we're done with the polygon

  GLTranslatef(3.0f,0.0f,0.0f);		        // Move Right 3 Units
	
  // draw a square (quadrilateral)
  GLBegin(GL_QUADS);				// start drawing a polygon (4 sided)
  GLTexCoord2f(0.0, 0.0); GLVertex3f(-cos(theta), cos(theta), sin(theta));	// Top Left
  GLTexCoord2f(1.0, 0.0); GLVertex3f( cos(theta), cos(theta), sin(theta));	// Top Right
  GLTexCoord2f(1.0, 1.0); GLVertex3f( cos(theta),-cos(theta), sin(theta));	// Bottom Right
  GLTexCoord2f(0.0, 1.0); GLVertex3f(-cos(theta),-cos(theta), sin(theta));	// Bottom Left	
  GLEnd();					// done with the polygon
  theta += .01;
  // swap buffers to display, since we're double buffered.
}

Uint32 rowTime;
Uint32 ticTime;
GLuint mainList;
GLuint mainTexes[4];
int channelFocus = 0;

int oldpatpos = 0, oldvbtick = 0, oldsngspd = 1; // for UpdatePosition
float theta = 0.0;

void ChannelUp()
{
	if(channelFocus + 1 < numChannels)
	{
		channelFocus++;
	}
}

void ChannelDown()
{
	if(channelFocus != 0)
	{
		channelFocus--;
	}
}

#define numMainLists 5
#define BLOCK_HEIGHT .75
#define TIC_HEIGHT .25
#define BLOCK_WIDTH 2.0
#define NOTE_WIDTH .75

void MoveFaster(void)
{
}
void MoveSlower(void)
{
}

#define NEGATIVE_ROWS 7		// number of rows after the active row
#define POSITIVE_ROWS 57	// number of rows in the distance
				// (including active row)
#define NUM_ROWS 64

#define NEGATIVE_TICKS 7*6
#define POSITIVE_TICKS 57*6
#define NUM_TICKS 64*6

int curTic; // tick counter from 0 - total ticks in the song
int firstVb, curVb, lastVb;	// tick counters for the three rows
int firstRow, curRow, lastRow;	// first row on screen, current row playing,
				// and the last row on screen

double partialTic;
typedef struct {
	int active; // need this? Player_Muted
	int numCorrect;
	} Channel;
typedef struct {
	Point pos;
	int row;
	} ScreenNote;
typedef struct {
	Point p1;
	Point p2;
	int color;
	int row;
	} Line;

Channel *activeChannels;
ScreenNote *notesOnScreen; // little ring buffer of notes
int startNote; // first note on screen
int stopNote;  // last note +1 on screen
int numNotes;  // max number of notes on screen (numChannels * NUM_ROWS)

Line *linesOnScreen;
int startLine;
int stopLine;
int numLines;
Obj obj;
int objActive = 0;
int *noteOffset;
MikMod_player_t oldHand;
int tickCounter;
int songStarted;	// this is set once we get to the first row, and the
			// song is unpaused

void AddNotes(int row)
{
	int x;
	if(row < 0 || row >= numRows) return;
	for(x=0;x<numChannels;x++)
	{
		if(rowData[row].chans[x].note)
		{
//			notesOnScreen[stopNote].pos.x = -x * BLOCK_WIDTH - NOTE_WIDTH * (double)(rowData[row].notes[x] - 2);
			notesOnScreen[stopNote].pos.x = -x * BLOCK_WIDTH - NOTE_WIDTH * (double)noteOffset[rowData[row].chans[x].note];
			notesOnScreen[stopNote].pos.y = 0.0;
			notesOnScreen[stopNote].pos.z = TIC_HEIGHT * (double)rowData[row].ticpos;
			notesOnScreen[stopNote].row = row;
			stopNote++;
			if(stopNote == numNotes) stopNote = 0;
		}
	}
}

void RemoveNotes(int row)
{
	if(row < 0 || row >= numRows) return;
	while(notesOnScreen[startNote].row == row)
	{
		notesOnScreen[startNote].row = -1; // just in case ;)
		startNote++;
		if(startNote == numNotes) startNote = 0;
	}
}

void AddLine(int row)
{
	if(row < 0 || row >= numRows) return;
	if(rowData[row].line)
	{
		linesOnScreen[stopLine].p1.x = 1.0;
		linesOnScreen[stopLine].p1.y = 0.0;
		linesOnScreen[stopLine].p1.z = TIC_HEIGHT * (double)rowData[row].ticpos;
		linesOnScreen[stopLine].p2.x = 1.0 - 2.0 * numChannels;
		linesOnScreen[stopLine].p2.y = 0.0;
		linesOnScreen[stopLine].p2.z = TIC_HEIGHT * (double)rowData[row].ticpos;
		linesOnScreen[stopLine].row = row;
		linesOnScreen[stopLine].color = rowData[row].line;
		stopLine++;
		if(stopLine == numLines) stopLine = 0;
	}
}

void RemoveLine(int row)
{
	if(row < 0 || row >= numRows) return;
	while(linesOnScreen[startLine].row == row)
	{
		linesOnScreen[startLine].row = -1;
		startLine++;
		if(startLine == numLines) startLine = 0;
	}
}

void SetMute(ModChannel *c, int mute)
{
	int j;
	for(j=0;j<c->numModChannels;j++)
	{
		if(mute) Player_Mute(c->modChannels[j]);
		else Player_Unmute(c->modChannels[j]);
	}
}

void Press(int button)
{
	int i;
	ModChannel *c;
	for(i=-1;i<=1;i++)
	{
		if(curRow + i >= 0 && curRow + i < numRows)
		{
			c = &(rowData[curRow+i].chans[channelFocus]);
			if(!c->struck && button == c->note)
			{
				c->struck = 1;
				activeChannels[channelFocus].active = 1;
				activeChannels[channelFocus].numCorrect++;
				SetMute(c, 0);
//				objActive = 1;
				obj.pos.x = -channelFocus * 2.0;
				obj.pos.y = 0.0;
				obj.pos.z = TIC_HEIGHT * ((double)curTic + partialTic);
				obj.vel.x = 0.0;
				obj.vel.y = 1.0;
				obj.vel.z = 2.0;
				obj.acc.x = 0.0;
				obj.acc.y = -.2;
				obj.acc.z = 0.0;
				obj.axis.x = .3;
				obj.axis.y = .5;
				obj.axis.z = .8;
				obj.theta = 3.7;
				obj.rotvel = 38.0;
				obj.rotacc = 0.0;
				obj.mass = 1.0;
				return;
			}
		}
	}
	if(activeChannels[channelFocus].numCorrect >= MAXNUM) return;
	activeChannels[channelFocus].active = 0;
	PlaySound(SND_zoomout6);
	SetMute(&(rowData[curRow].chans[channelFocus]), 1);
}

// i really should fix the event system...i suck!
void Press1() {Press(1);}

void Press2() {Press(2);}

void Press3() {Press(4);}

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
	gluLookAt(	mainPos[0] - channelFocus * 2.0, mainPos[1], mainPos[2],
			mainView[0] - channelFocus * 2.0, mainView[1], mainView[2],
			0.0, 1.0, 0.0);
}

void FixVb(int *vb, int *row)
{
	*row = 0;
	while(1)
	{
		if(*row < 0 && *vb >= rowData[0].sngspd)
		{
			(*row)++;
			*vb -= rowData[0].sngspd;
		}
		else if(*row >= 0 && *vb >= rowData[*row].sngspd)
		{
			(*row)++;
			*vb -= rowData[*row].sngspd;
		}
		else if(*vb < 0)
		{
			(*row)--;
			*vb += rowData[0].sngspd;
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
	Log("Start module\n");
	if(!StartModule(CfgS("main.song")))
	{
		Log("Error loading module in scene!\n");
		return 0;
	}
	// module and module data (where to place the notes) are now loaded,
	// and the module is paused
	Log("Module ready\n");
	tickCounter = 0;
	songStarted = 0;
	oldHand = MikMod_RegisterPlayer(TickHandler);
	noteOffset = (int*)malloc(sizeof(int) * (MAX_NOTE+1));

	noteOffset[1] = -1;
	noteOffset[2] = 0;
	noteOffset[4] = 1;

	rowTime = 0;
	ticTime = 0;
	channelFocus = 0;
	oldpatpos = 0; oldvbtick = 0; oldsngspd = 1; // for UpdatePosition
	theta = 0.0;

	// start back about 3.5 seconds worth of ticks
	// doesn't need to be exact, we just need some time for
	// the player to get ready
	curTic = (int)(-3500.0 * (double)rowData[0].bpm / 2500.0);
	// set the tic positions of where we can see and where we're looking
	firstVb = curTic - NEGATIVE_TICKS;
	curVb = curTic;
	lastVb = curTic + POSITIVE_TICKS;
	// convert tic values to rows, now the ticks are all within
	// [0 .. rowData[xRow].sngspd - 1]
	FixVb(&firstVb, &firstRow);
	FixVb(&curVb, &curRow);
	FixVb(&lastVb, &lastRow);

	partialTic = 0.0;
	activeChannels = (Channel*)malloc(sizeof(Channel) * numChannels);

	// MARF CHECK make sure mem is at correct amount of size! -eg its not, could have all rows with sngspd 1 and run outta mem
	numNotes = numChannels * NUM_ROWS;
	notesOnScreen = (ScreenNote*)malloc(sizeof(ScreenNote) * numNotes);
	startNote = 0;
	stopNote = 0;
	for(x=0;x<=lastRow;x++) AddNotes(x);
//	for(x=0;x<=curRow + POSITIVE_ROWS;x++) AddNotes(x);

	numLines = NUM_ROWS;
	linesOnScreen = (Line*)malloc(sizeof(Line) * numLines);
	startLine = 0;
	stopLine = 0;
	for(x=0;x<=lastRow;x++) AddLine(x);
//	for(x=0;x<=curRow + POSITIVE_ROWS;x++) AddLine(x);

	for(x=0;x<numChannels;x++)
	{
		activeChannels[x].active = 1;
		activeChannels[x].numCorrect = 0;
	}
	RegisterEvent(EVENT_RIGHT, ChannelUp, EVENTTYPE_MULTI);
	RegisterEvent(EVENT_LEFT, ChannelDown, EVENTTYPE_MULTI);
	RegisterEvent(EVENT_UP, MoveFaster, EVENTTYPE_MULTI);
	RegisterEvent(EVENT_DOWN, MoveSlower, EVENTTYPE_MULTI);
	Log("Creating lists\n");
	mainList = GLGenLists(numMainLists);

	mainTexes[0] = TEX_Slate;
	mainTexes[1] = TEX_Walnut;
	mainTexes[2] = TEX_Lava;
	mainTexes[3] = TEX_Cloth;

	for(x=0;x<4;x++)
	{
		glNewList(mainList+x, GL_COMPILE);
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

	glNewList(mainList+4, GL_COMPILE);
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
	} glEndList();
	printf("Mem: %i\n", QueryMemUsage());
	InitTimer();
	Log("Lists created\n");
	return 1;
}

void MainQuit()
{
	Log("Main Scene quit\n");
	oldHand = MikMod_RegisterPlayer(oldHand);
	if(songStarted)
	{
		DeregisterEvent(EVENT_SHOWMENU, Player_TogglePause);
		DeregisterEvent(EVENT_HIDEMENU, Player_TogglePause);
		DeregisterEvent(EVENT_BUTTON1, Press1);
		DeregisterEvent(EVENT_BUTTON2, Press2);
		DeregisterEvent(EVENT_BUTTON3, Press3);
		DeregisterEvent(EVENT_BUTTON4, Press4);
	}
	songStarted = 0;
	DeregisterEvent(EVENT_RIGHT, ChannelUp);
	DeregisterEvent(EVENT_LEFT, ChannelDown);
	DeregisterEvent(EVENT_UP, MoveFaster);
	DeregisterEvent(EVENT_DOWN, MoveSlower);
	free(activeChannels);
	free(notesOnScreen);
	free(linesOnScreen);
	free(noteOffset);
	StopModule();
	GLDeleteLists(mainList, numMainLists);
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

// returns 1 if the row is valid, 0 otherwise
#define IsValidRow(row) ((row >= 0 && row < numRows) ? 1 : 0)
// return a clamped row number
#define Row(row) (row < 0 ? 0 : (row >= numRows ? numRows : row))

void UpdatePosition()
{
	int tmpAdj;
	// calculate the amount of ticTime elapsed
	// everyone 2500 ticTime is one tick
	if(!menuActive) ticTime += timeDiff * rowData[Row(curRow)].bpm;

	// adjust the ticTime if we're our time is different from the
	// song time.  This is needed in case a little blip in the process
	// causes the song to be ahead or behind the game, so we can
	// right ourselves.
	if(songStarted)
	{
		tmpAdj = (tickCounter - curTic) << 6;
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
		if(curVb >= rowData[Row(curRow)].sngspd)
		{
			curVb -= rowData[Row(curRow)].sngspd;
			curRow++;
			if(curRow == 0) // start the song!
			{
				Player_TogglePause();
				songStarted = 1;
				RegisterEvent(EVENT_SHOWMENU, Player_TogglePause, EVENTTYPE_MULTI);
				RegisterEvent(EVENT_HIDEMENU, Player_TogglePause, EVENTTYPE_MULTI);
				RegisterEvent(EVENT_BUTTON1, Press1, EVENTTYPE_MULTI);
				RegisterEvent(EVENT_BUTTON2, Press2, EVENTTYPE_MULTI);
				RegisterEvent(EVENT_BUTTON3, Press3, EVENTTYPE_MULTI);
				RegisterEvent(EVENT_BUTTON4, Press4, EVENTTYPE_MULTI);
			}
		}

		if(firstVb >= rowData[Row(firstRow)].sngspd)
		{
			firstVb -= rowData[Row(firstRow)].sngspd;
			// the remove functions check to make sure
			// the row is valid
			RemoveNotes(firstRow);
			RemoveLine(firstRow);
			firstRow++;
		}

		if(lastVb >= rowData[Row(lastRow)].sngspd)
		{
			lastVb -= rowData[Row(lastRow)].sngspd;
			lastRow++;
			// the add functions check to make sure the
			// row is valid
			AddNotes(lastRow);
			AddLine(lastRow);
		}
	}
	partialTic = (double)ticTime / 2500.0;
}

void DrawRows(double startTic, double stopTic)
{
	int chan;
	if(startTic >= stopTic) return;
	glPushMatrix();
	for(chan=0;chan<numChannels;chan++)
	{
		glBindTexture(GL_TEXTURE_2D, mainTexes[chan&3]);
		glDisable(GL_LIGHTING);
		glBegin(GL_QUADS);
		{
			glTexCoord2f(0.0, startTic / 4.0);
			glVertex3f(-1.0, 0.0, startTic);
			glTexCoord2f(1.0, startTic / 4.0);
			glVertex3f(1.0, 0.0, startTic);
			glTexCoord2f(1.0, stopTic / 4.0);
			glVertex3f(1.0, 0.0, stopTic);
			glTexCoord2f(0.0, stopTic / 4.0);
			glVertex3f(-1.0, 0.0, stopTic);
		} glEnd();
		glTranslated(-BLOCK_WIDTH, 0, 0);
		glEnable(GL_LIGHTING);
	}
	glPopMatrix();
}

void DrawNote(int note)
{
	float temp[4] = {.7, 0.0, 0.0, 1.0};
	glPushMatrix();
	if(abs(notesOnScreen[note].row - curRow) <= 1)
		glMaterialfv(GL_FRONT, GL_EMISSION, temp);
//	else glColor3f(0.7, 0.7, 0.7);
	glTranslated(	notesOnScreen[note].pos.x,
			notesOnScreen[note].pos.y,
			notesOnScreen[note].pos.z);
	glRotatef(theta, 0.0, 1.0, 0.0);
	glCallList(mainList+4);
	glMaterialfv(GL_FRONT, GL_EMISSION, lightNone);
	glPopMatrix();
}

void DrawNotes()
{
	int x;
	if(startNote <= stopNote)
	{
		for(x=startNote;x<stopNote;x++)
			DrawNote(x);
	}
	else
	{
		for(x=startNote;x<numNotes;x++)
			DrawNote(x);
		for(x=0;x<stopNote;x++)
			DrawNote(x);
	}
}

void CheckChannels()
{
	int x;
	ModChannel *c;
	for(x=0;x<numChannels;x++)
	{
		if(curRow - 2 >= 0 && curRow - 2 < numRows)
		{
			c = &(rowData[curRow-2].chans[x]);
			if(c->note && !c->struck && activeChannels[x].numCorrect < MAXNUM)
			{
				activeChannels[x].active = 0;
				SetMute(c, 1);
			}
		}
	}
}

void DrawTargets()
{
	int x;
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
}

void MainScene()
{
	float light[4] = {0.0, 1.0, -8.0, 1.0};
	float temp[4] = {.35, 0.0, 0.0, .5};
	float sintmp;
//	int x;

	Log("MainScene\n");

	glLoadIdentity();
	sintmp = sin(((double)(curTic) + partialTic) * 3.1415 / 24.0);
	light[0] += cos(((double)(curTic) + partialTic)  * 3.1415 / 24.0);
	light[1] += sintmp * sintmp;
	glPushMatrix();
	glLightfv(GL_LIGHT1, GL_POSITION, light);

	Log("U");
	if(curRow != numRows) UpdatePosition();
	Log("u");
	SetMainView();

	glColor4f(1.0, 1.0, 1.0, 1.0);
	CheckChannels();

	Log("A");
	// usually we draw from -NEGATIVE_TICKS to +POSITIVE_TICKS, with the
	// notes currently being played at position 0.
	// At the beginning of the song, we start drawing instead from 
	// 0 to +POSITIVE_TICKS, and at the end of the song we draw from
	// -NEGATIVE_TICKS to numTics.  Of course, if the song is less than
	// NEGATIVE_TICKS + POSITIVE_TICKS long, some other combinations will
	// arise :)
	DrawRows(
			// start
			curTic - NEGATIVE_TICKS >= 0 ?
			TIC_HEIGHT * ((double)curTic + partialTic - NEGATIVE_TICKS) :
			0.0,
			// end
			curTic < numTics - POSITIVE_TICKS ?
			TIC_HEIGHT * (double)curTic+partialTic+POSITIVE_TICKS :
			TIC_HEIGHT * (double)numTics);

	Log("B");
	DrawNotes();
	Log("C");
	DrawLines();
	Log("2");

	glPushMatrix();
	glTranslated((double)channelFocus * -2.0, 0.0, TIC_HEIGHT * ((double)curTic + partialTic));
	DrawTargets();
	glPopMatrix();
	Log("D");

	if(objActive)
	{
		glPushMatrix();
		glTranslated(obj.pos.x, obj.pos.y, obj.pos.z);
		glRotated(obj.theta, obj.axis.x, obj.axis.y, obj.axis.z);
		glDisable(GL_TEXTURE_2D);
		glNormal3f(0.0, 0.0, 1.0);
		glBegin(GL_QUADS);
		{
			glVertex3f(-0.5, -0.5, 0.0);
			glVertex3f(0.5, -0.5, 0.0);
			glVertex3f(0.5, 0.5, 0.0);
			glVertex3f(-0.5, 0.5, 0.0);
		} glEnd();
		glEnable(GL_TEXTURE_2D);
		glPopMatrix();
		UpdateObj(&obj, (double)timeDiff / 1000.0);
	}

	glColor4f(1.0, 1.0, 1.0, 1.0);
	if(activeChannels[channelFocus].numCorrect >= MAXNUM) PrintGL(50, 50, "Channel cleared - move on!\n");
	PrintGL(50, 0, "Playing: %s", mod->songname);
	if(curRow == numRows) PrintGL(50, 15, "Song complete!");
	else if(curRow >= 0) PrintGL(50, 15, "Song: %i (%i)/%i, Row: %i (%i)/%i Pattern: %i/%i", mod->sngpos, rowData[curRow].sngpos, mod->numpos, mod->patpos, rowData[curRow].patpos, NumPatternsAtSngPos(mod->sngpos),  mod->positions[mod->sngpos], mod->numpat);
	else
	{
		int timeLeft = (int)(0.5 + -2500.0 * (double)rowData[0].sngspd * ((double)curRow) / (1000.0 * (double)rowData[0].bpm));
		if(timeLeft > 0) PrintGL(50, 15, "%i...", timeLeft);
		else PrintGL(50, 15, "GO!!");
	}
	if(curRow >= 0 && curRow < numRows) PrintGL(0, 62, "Tick: %i, %i.%f / %i\n", rowData[curRow].ticpos, curTic, partialTic, numTics);
	PrintGL(50, 30, "Speed: %i/%i at %i\n", mod->vbtick, mod->sngspd, mod->bpm);

	Log("E");

	glPopMatrix();
	glPushMatrix();
	glBindTexture(GL_TEXTURE_2D, TEX_Fireball);
	temp[0] *= 3.0;
	glMaterialfv(GL_FRONT, GL_EMISSION, temp);
	glBegin(GL_QUADS);
	{
		glTexCoord2f(0.0, 0.0);
		glVertex3f(light[0]-.5, light[1], light[2]-.5);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(light[0]+.5, light[1], light[2]-.5);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(light[0]+.5, light[1], light[2]+.5);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(light[0]-.5, light[1], light[2]+.5);
	} glEnd();
	glMaterialfv(GL_FRONT, GL_EMISSION, lightNone);
	glPopMatrix();

	theta += (double)timeDiff * 120.0 / 1000.0;
	Log("F");
	Log("endMainScene\n");
}
