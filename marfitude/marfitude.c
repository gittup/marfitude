#include <math.h>

#include "marfitude.h"
#include "press.h"

#include "SDL_opengl.h"

#include "gmae/cfg.h"
#include "gmae/event.h"
#include "gmae/glfunc.h"
#include "gmae/log.h"
#include "gmae/mainscene.h"
#include "gmae/menu.h"
#include "gmae/module.h"
#include "gmae/particles.h"
#include "gmae/phys.h"
#include "gmae/textures.h"
#include "gmae/timer.h"
#include "gmae/wam.h"

#include "util/memtest.h"
#include "util/myrand.h"
#include "util/plugin.h"
#include "util/slist.h"

static float lightNone[4] = {0.0, 0.0, 0.0, 1.0};
static float lightNormal[4] = {0.8, 0.8, 0.8, 1.0};

static Uint32 ticTime;
static GLuint rowList;
static GLuint noteList;
static GLuint mainTexes[MAX_COLS];
int channelFocus = 0;
struct wam *wam;	/* note file */

static float theta = 0.0;

static void MoveFaster(void);
static void MoveSlower(void);

void MoveFaster(void)
{
}
void MoveSlower(void)
{
}


/* returns 1 if the row is valid, 0 otherwise */
#define IsValidRow(row) ((row >= 0 && row < wam->numRows) ? 1 : 0)
/* return a clamped row number */
#define Row(row) (row < 0 ? 0 : (row >= wam->numRows ? wam->numRows - 1: row))
#define Max(a, b) ((a) > (b) ? (a) : (b))

int curTic; /* tick counter from 0 - total ticks in the song */
static int firstVb, curVb, lastVb;     /* tick counters for the three rows */
static int firstRow, curRow, lastRow;  /* first row on screen, current row
                                        * playing and the last row on screen
                                        */
double partialTic;

/** A note on the screen */
struct screenNote {
	struct vector pos; /**< The coordinates of where to draw the note */
	int tic;           /**< The mod tic this note is on */
	int col;           /**< The column the note is in */
	int ins;           /**< DEBUG - the line in the src where this note was
			    * added
			    */
};

/** This structure keeps track of information needed for the column that is
 * being played
 */
struct attackPattern {
	int startTic;     /**< first tic that we need to play */
	int stopTic;      /**< last tic that we need to play */
	int stopRow;      /**< corresponding row to stopTic */
	int nextStartRow; /**< which row the game is cleared to. */
	int lastTic;      /**< last note played is in lastTic */
	int notesHit;     /**< number of notes we hit so far */
	int notesTotal;   /**< total number of notes we need to play */
};

/** This structure keeps track of clearing information for each column */
struct attackCol {
	double part;	/**< cumulative row adder, when >= 1.0 inc minRow */
	int minRow;	/**< equal to cleared, but doesn't get set to 0
			 * after the column is recreated
			 */
	int cleared;	/**< equals the last row this col is cleared to, 0 if
			 * not cleared
			 */
	int hit;	/**< equals the tic of the last hit note */
	int miss;	/**< equals the tic of the last missed note */
};

static void ResetAp(void);
static void ChannelUp(void);
static void ChannelDown(void);
static struct column *ColumnFromNum(int col);
static void Setmute(struct column *c, int mute);
static void UpdateModule(void);
static void AddNotes(int row);
static struct slist *RemoveList(struct slist *list, int tic);
static void RemoveNotes(int row);

static int BelowBoard(struct particle *p);
static int AboveBoard(struct particle *p);
static void Press(int button);
static void SetMainView(void);
/*static void DrawNote(void *snp, void *not_used);*/
/*static void DrawHitNote(void *snp, void *not_used);*/
static void DrawNotes(void);
static void DrawHitNotes(void);
/*static void DrawTargets(void);*/
static void DrawScoreboard(void);
static void MoveHitNotes(int tic, int col);
static void UpdateClearedCols(void);
static void UpdatePosition(void);
static void DrawRows(double startTic, double stopTic);
static void RandomColor(float col[4]);
static int RowByTic(int tic);
static struct screenNote *FindNote(struct slist *list, int tic, int col);
static void FixVb(int *vb, int *row);
static void TickHandler(void);
static void CheckMissedNotes(void);
static void CheckColumn(int row);
static int NoteListTic(const void *snp, const void *tp);
static int SortByTic(const void *a, const void *b);
static void menu_handler(const void *data);
static void button_handler(const void *data);

static struct attackPattern ap;
static struct attackCol ac[MAX_COLS];
static struct screenNote *notesOnScreen; /* little ring buffer of notes */
static struct slist *unusedList;	/* unused notes */
static struct slist *notesList;	/* notes on the screen, not hit */
static struct slist *hitList;	/* notes on the screen, hit */
static int numNotes;	/* max number of notes on screen (wam->numCols * NUM_TICKS) */
static char *cursong;
static int newhighscore;
static int highscore;
static int score;
static int multiplier;
/*static float light[4] = {0.0, 1.0, -8.0, 1.0}; */
static float light[4] = {0.0, 0.5, 0.0, 1.0};

static float bounceTime;
static int *noteOffset;
static MikMod_player_t oldHand;
static int tickCounter;
static int songStarted;	/* this is set once we get to the first row, and the */
			/* song is unpaused */
static int fireball_tex;

void menu_handler(const void *data)
{
	const struct menu_e *m = data;
	if(m->active ^ Player_Paused()) {
		Player_TogglePause();
	}
}

void button_handler(const void *data)
{
	static int lastkeypressed = 1;
	const struct button_e *b = data;

	switch(b->button) {
		case B_RIGHT:
			ChannelUp();
			break;
		case B_LEFT:
			ChannelDown();
			break;
		case B_UP:
			MoveFaster();
			break;
		case B_DOWN:
			MoveSlower();
			break;
		case B_BUTTON1:
			lastkeypressed = 1;
			Press(1);
			break;
		case B_BUTTON2:
			lastkeypressed = 2;
			Press(2);
			break;
		case B_BUTTON3:
			/* yes, this is really 4 (3rd bit) */
			lastkeypressed = 4;
			Press(4);
			break;
		case B_BUTTON4:
			Press(lastkeypressed);
			break;
		default:
			break;
	}
}

static void Load(void);
static void Unload(void);
static void AddPlugin(const char *s);

static void **plugins = NULL;
static int numPlugins = 0;

void AddPlugin(const char *s)
{
	plugins = (void**)realloc(plugins, sizeof(void*) * (numPlugins + 1));
	plugins[numPlugins] = load_plugin(s);
	numPlugins++;
}

void Load(void)
{
	AddPlugin("./liblaser.so");
	AddPlugin("./libtargets.so");
	AddPlugin("./liblines.so");
	AddPlugin("./libfft-curtain.so");
}

void Unload(void)
{
	int x;

	for(x=0; x<numPlugins; x++) {
		free_plugin(plugins[x]);
	}
	free(plugins);
	plugins = NULL;
	numPlugins = 0;
}

int MainInit()
{
	int x;
	Log(("Load Wam\n"));
	cursong = CfgSCpy("main", "song");

	wam = LoadWam(cursong);
	if(wam == NULL) {
		ELog(("Error: Couldn't load WAM file\n"));
		return 1;
	}
	Log(("Start module\n"));
	if(!StartModule(cursong)) {
		ELog(("Error: Couldn't start module\n"));
		return 2;
	}
	/* module and module data (where to place the notes) are now loaded, */
	/* and the module is paused */
	Log(("Module ready\n"));
	highscore = CfgIp("highscore", cursong);
	newhighscore = 0;
	score = 0;
	multiplier = 1;
	tickCounter = 0;
	songStarted = 0;
	oldHand = MikMod_RegisterPlayer(TickHandler);
	noteOffset = (int*)malloc(sizeof(int) * (MAX_NOTE+1));

	noteOffset[1] = -1;
	noteOffset[2] = 0;
	noteOffset[4] = 1;

	Load();
	fireball_tex = TextureNum("Fireball.png");

	ticTime = 0;
	channelFocus = 0;
	theta = 0.0;

	for(x=0;x<wam->numCols;x++) {
		ac[x].part = 0.0;
		ac[x].minRow = 0;
		ac[x].cleared = 0;
		ac[x].hit = -1;
		ac[x].miss = -2;
	}

	/* start back about 3.5 seconds worth of ticks
	 * doesn't need to be exact, we just need some time for
	 * the player to get ready
	 */
	curTic = (int)(-3500.0 * (double)wam->rowData[0].bpm / 2500.0);
	/* set the tic positions of where we can see and where we're looking */
	firstVb = curTic - NEGATIVE_TICKS;
	curVb = curTic;
	lastVb = curTic + POSITIVE_TICKS;
	/* convert tic values to rows, now the ticks are all within */
	/* [0 .. wam->rowData[xRow].sngspd - 1] */
	FixVb(&firstVb, &firstRow);
	FixVb(&curVb, &curRow);
	FixVb(&lastVb, &lastRow);

	partialTic = 0.0;

	numNotes = wam->numCols * NUM_TICKS;
	notesOnScreen = (struct screenNote*)malloc(sizeof(struct screenNote) * numNotes);
	unusedList = NULL;
	notesList = NULL;
	hitList = NULL;
	for(x=0;x<numNotes;x++) {
		unusedList = slist_append(unusedList, (void *)&notesOnScreen[x]);
		notesOnScreen[x].ins = __LINE__;
	}
	for(x=0;x<=lastRow;x++) AddNotes(x);

	for(x=0;x<=lastRow;x++) FireEvent("row", &wam->rowData[x]);

	ap.nextStartRow = -1;
	ResetAp();
	RegisterEvent("button", button_handler, EVENTTYPE_MULTI);
	Log(("Creating lists\n"));
	rowList = glGenLists(wam->numCols);
	noteList = glGenLists(1);

	mainTexes[0] = TextureNum("Slate.png");
	mainTexes[1] = TextureNum("Walnut.png");
	mainTexes[2] = TextureNum("ElectricBlue.png");
	mainTexes[3] = TextureNum("Clovers.png");
	mainTexes[4] = TextureNum("Lava.png");
	mainTexes[5] = TextureNum("Parque3.png");
	mainTexes[6] = TextureNum("Slate.png");
	mainTexes[7] = TextureNum("ElectricBlue.png");

	for(x=0;x<wam->numCols;x++) {
		glNewList(rowList+x, GL_COMPILE); {
			glColor4f(1.0, 1.0, 1.0, 1.0);
			glNormal3f(0.0, 1.0, 0.0);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, lightNormal);
			glBegin(GL_QUADS); {
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

	glNewList(noteList, GL_COMPILE); {
		glBegin(GL_TRIANGLE_FAN); {
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
		glPopMatrix();
	} glEndList();
	InitTimer();
	Log(("Lists created\n"));
	return 0;
}

void MainQuit(void)
{
	Log(("Main Scene quit\n"));
	if(newhighscore) {
		CfgSetIp("highscore", cursong, highscore);
	}
	free(cursong);
	oldHand = MikMod_RegisterPlayer(oldHand);
	Log(("A\n"));
	if(songStarted) {
		DeregisterEvent("menu", menu_handler);
	}
	Log(("A\n"));
	songStarted = 0;
	DeregisterEvent("button", button_handler);
	Log(("A\n"));
	Unload();
	glDeleteLists(rowList, wam->numCols);
	Log(("A\n"));
	glDeleteLists(noteList, 1);
	Log(("A\n"));
	free(notesOnScreen);
	Log(("A\n"));
	free(noteOffset);
	Log(("A\n"));
	ClearParticles();
	Log(("A\n"));
	CheckObjs();
	Log(("A\n"));
	FreeWam(wam);
	Log(("A\n"));
	StopModule();
	slist_free(hitList);
	slist_free(notesList);
	slist_free(unusedList);
	Log(("Main scene quit finished\n"));
}

void MainScene(void)
{
	float temp[4] = {.35, 0.0, 0.0, .5};
	float sintmp;
	struct row *row;

	Log(("MainScene\n"));

	glLoadIdentity();
	glPushMatrix();

	Log(("U"));
	if(curRow != wam->numRows) UpdatePosition();
	Log(("1"));
	UpdateObjs(timeDiff);
	Log(("u"));
	SetMainView();

	row = &wam->rowData[Row(curRow)];
	bounceTime = 2.0 * 3.1415 * ((double)row->ticprt + (double)curTic - (double)row->ticpos + partialTic) / (double)row->ticgrp;
	sintmp = sin(bounceTime);
	light[0] = -BLOCK_WIDTH * channelFocus + cos(bounceTime);
	light[1] = 1.0 + sintmp * sintmp;
	light[2] = TIC_HEIGHT * ((double)curTic + partialTic);
	glLightfv(GL_LIGHT1, GL_POSITION, light);

	glColor4f(1.0, 1.0, 1.0, 1.0);

	Log(("A"));
	/* usually we draw from -NEGATIVE_TICKS to +POSITIVE_TICKS, with the
	 * notes currently being played at position 0.
	 * At the beginning of the song, we start drawing instead from 
	 * 0 to +POSITIVE_TICKS, and at the end of the song we draw from
	 * -NEGATIVE_TICKS to wam->numTics.  Of course, if the song is less than
	 * NEGATIVE_TICKS + POSITIVE_TICKS long, some other combinations will
	 * arise :)
	 */
	DrawRows(
			/* start */
			curTic - NEGATIVE_TICKS >= 0 ?
			((double)curTic + partialTic - NEGATIVE_TICKS) :
			0.0,
			/* end */
			curTic < wam->numTics - POSITIVE_TICKS ?
			(double)curTic+partialTic+POSITIVE_TICKS :
			(double)wam->numTics);


	Log(("B"));
	DrawNotes();
	Log(("C"));

	SetOrthoProjection();
	FireEvent("draw ortho", NULL);
	ResetProjection();

	FireEvent("draw opaque", NULL);

	Log(("E"));
	DrawScoreboard();
	Log(("F"));

	StartParticles();
	Log(("F1\n"));
	DrawParticlesTest(BelowBoard);
	Log(("F2\n"));
	DrawHitNotes();
	Log(("F3\n"));
	FireEvent("draw transparent", NULL);
	Log(("G"));
	DrawParticlesTest(AboveBoard);
	Log(("g"));
	StopParticles();

	Log(("L"));


	glBindTexture(GL_TEXTURE_2D, fireball_tex);
	temp[0] *= 3.0;
	glMaterialfv(GL_FRONT, GL_EMISSION, temp);
	glNormal3f(0.0, 1.0, 0.0);
	glBegin(GL_QUADS); {
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
	Log(("H"));
	Log(("endMainScene\n"));
}

void ChannelUp(void)
{
	if(channelFocus + 1 < wam->numCols) {
		channelFocus++;
		if(ap.notesHit > 0) multiplier = 1;
		ResetAp();
		ac[channelFocus].hit = ap.startTic - 1;
	}
}

void ChannelDown(void)
{
	if(channelFocus != 0) {
		channelFocus--;
		if(ap.notesHit > 0) multiplier = 1;
		ResetAp();
		ac[channelFocus].hit = ap.startTic - 1;
	}
}

struct column *ColumnFromNum(int col)
{
	return &wam->patterns[wam->rowData[Row(curRow)].patnum].columns[col];
}

void Setmute(struct column *c, int mute)
{
	int x;
	for(x=0;x<c->numchn;x++) {
		if(mute) Player_Mute(c->chan[x]);
		else Player_Unmute(c->chan[x]);
	}
}

/* update which channels are playing based on attackCol contents */
void UpdateModule(void)
{
	int x;
	for(x=0;x<wam->numCols;x++) {
		if(ac[x].cleared || ac[x].hit > ac[x].miss)
			Setmute(ColumnFromNum(x), UNMUTE);
		else
			Setmute(ColumnFromNum(x), MUTE);
	}
	Setmute(&wam->patterns[wam->rowData[Row(curRow)].patnum].unplayed, UNMUTE);
}

void AddNotes(int row)
{
	int x;
	int tic;
	struct screenNote *sn;
	if(row < 0 || row >= wam->numRows) return;
	tic = wam->rowData[row].ticpos;
	for(x=0;x<wam->numCols;x++) {
		if(wam->rowData[row].notes[x]) {
			sn = unusedList->data;
			if(row < ac[x].minRow) {
				hitList = slist_append(hitList, sn);
				sn->ins = __LINE__;
			} else {
				notesList = slist_append(notesList, sn);
				sn->ins = __LINE__;
			}
			unusedList = slist_remove(unusedList, sn);
			sn->pos.x = -x * BLOCK_WIDTH - NOTE_WIDTH * (double)noteOffset[(int)wam->rowData[row].notes[x]];
			sn->pos.y = 0.0;
			sn->pos.z = TIC_HEIGHT * (double)wam->rowData[row].ticpos;
			sn->tic = tic;
			sn->col = x;
		}
	}
}

struct slist *RemoveList(struct slist *list, int tic)
{
	struct screenNote *sn;
	while(list) {
		sn = (struct screenNote*)list->data;
		if(sn->tic != tic) break;
		unusedList = slist_append(unusedList, list->data);
		list = slist_remove(list, list->data);
		sn->ins = __LINE__;
	}
	return list;
}

void RemoveNotes(int row)
{
	int tic;
	if(row < 0 || row >= wam->numRows) return;
	tic = wam->rowData[row].ticpos;
	Log(("RM\n"));
	notesList = RemoveList(notesList, tic);
	Log(("RMHit: %i\n", hitList));
	hitList = RemoveList(hitList, tic);
	Log(("done: %i\n", hitList));
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

struct screenNote *FindNote(struct slist *list, int tic, int col)
{
	struct screenNote *sn;
	while(list) {
		sn = (struct screenNote*)list->data;
		if(sn->tic == tic && sn->col == col) return sn;
		list = slist_next(list);
	}
	return NULL;
}


void Press(int button)
{
	int i;
	int noteHit = 0;
	struct screenNote *sn;
	struct row *r;
	struct vector laser1;
	struct vector laser2;
	struct press_e e;

	laser1.x = light[0];
	laser1.y = light[1];
	laser1.z = light[2];
	laser2.x = -channelFocus * BLOCK_WIDTH - NOTE_WIDTH * noteOffset[button];
	laser2.y = 0.0;
	laser2.z = TIC_HEIGHT * ((double)curTic + partialTic);
	e.v1 = &laser1;
	e.v2 = &laser2;
	FireEvent("press", &e);

	for(i=-1;i<=1;i++) {
		if(curRow + i >= 0 && curRow + i < wam->numRows) {
			r = &wam->rowData[curRow+i];
			/* if we're in the attackpattern limits, and */
			/* if we didn't already play this row, and this row */
			/* is within our acceptable error, and we hit the */
			/* right note, then yay */
			if(	
				r->ticpos >= ap.startTic &&
				r->ticpos < ap.stopTic &&
				r->ticpos > ap.lastTic &&
				abs(r->ticpos - curTic) <= TIC_ERROR &&
				button == r->notes[channelFocus]) {

				sn = FindNote(notesList, r->ticpos, channelFocus);
				if(!sn) {
					ELog(("Error: Struck note not found!\n"));
					sn = FindNote(hitList, r->ticpos, channelFocus);
					if(sn) {
						ELog(("note found in hitList: %i\n", sn->ins));
					}
					sn = FindNote(unusedList, r->ticpos, channelFocus);
					if(sn) {
						ELog(("note found in unusedList: %i\n", sn->ins));
					}
					break;
				}
				notesList = slist_remove(notesList, (void *)sn);
				unusedList = slist_append(unusedList, (void *)sn);
				sn->ins = __LINE__;
				ap.lastTic = r->ticpos;
				ap.notesHit++;
				if(ap.notesHit == ap.notesTotal) {
					ap.nextStartRow = ap.stopRow;
					ac[channelFocus].cleared = ap.stopRow + LINES_PER_AP * ROWS_PER_LINE * wam->numCols;
					/* clear to a line break */
					while(ac[channelFocus].cleared < wam->numRows && wam->rowData[ac[channelFocus].cleared].line == 0)
						ac[channelFocus].cleared++;
					ac[channelFocus].minRow = curRow;
					ac[channelFocus].part = 0.0;
					score += ap.notesHit * multiplier;
					if(score > highscore) {
						highscore = score;
						newhighscore = 1;
					}
					if(multiplier < 8) multiplier++;
					ap.notesHit = 0;
				}
				ac[channelFocus].hit = r->ticpos;
				noteHit = 1;
				r->notes[channelFocus] = 0;
				break;
			}
		}
	}

	r = &wam->rowData[Row(curRow)];
	if(!noteHit && r->ticpos >= ap.startTic && r->ticpos < ap.stopTic) {
		/* oops, we missed! */
		if(ap.notesHit > 0) multiplier = 1;
		if(IsValidRow(curRow) && r->ticpos >= ap.startTic && r->ticpos < ap.stopTic)
			ac[channelFocus].miss = r->ticpos;
		ResetAp();
	}
	UpdateModule();
}

void SetMainView(void)
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
	while(1) {
		if(*row < 0 && *vb >= wam->rowData[0].sngspd) {
			(*row)++;
			*vb -= wam->rowData[0].sngspd;
		} else if(*row >= 0 && *vb >= wam->rowData[*row].sngspd) {
			(*row)++;
			*vb -= wam->rowData[*row].sngspd;
		} else if(*vb < 0) {
			(*row)--;
			*vb += wam->rowData[0].sngspd;
		}
		else break;
	}
}

/* keep track of the number of actual ticks played by the mod player, */
/* so we know if we're off */
void TickHandler(void)
{
	if(songStarted) {
		if(!Player_Paused()) tickCounter++;
	}
	oldHand();
}

/* gets either curRow or curRow+1, depending on which one tic is closest to */
/* (eg within acceptable error) */
int RowByTic(int tic)
{
	if(tic - wam->rowData[Row(curRow)].ticpos < TIC_ERROR) return curRow;
	return curRow+1;
}

void ResetAp(void)
{
	int start;
	int end;
	int apLines = 0;

	ap.notesHit = 0;
	ap.notesTotal = 0;
	start = Row(Max(Max(RowByTic(curTic), ap.nextStartRow), ac[channelFocus].cleared));
	while(start < wam->numRows && (wam->rowData[start].line == 0 || wam->rowData[start].ticpos <= ac[channelFocus].miss)) start++;
	if(start == wam->numRows)
		start = wam->numRows - 1;
	end = start;
	while(apLines < LINES_PER_AP && end < wam->numRows) {
		/* don't count the note on the last row, since that will */
		/* be the beginning of the next "AttackPattern" */
		if(wam->rowData[end].notes[channelFocus]) ap.notesTotal++;
		end++;
		if(wam->rowData[end].line != 0) apLines++;
	}
	if(apLines < LINES_PER_AP) {
		/* at the end of the song we can't find a valid pattern to do */
		ap.startTic = -1;
		ap.stopTic = -1;
		ap.lastTic = -1;
		ap.stopRow = -1;
		return;
	}
	if(end == wam->numRows)
		end = wam->numRows - 1;
	Log(("StarT: %i, End: %i\n", start, end));
	ap.startTic = wam->rowData[start].ticpos;
	ap.stopTic = wam->rowData[end].ticpos;
	ap.lastTic = wam->rowData[start].ticpos - 1;
	ap.stopRow = end;
}

void CheckMissedNotes(void)
{
	struct screenNote *sn;
	struct slist *list;
	list = notesList;
	while(list) {
		sn = (struct screenNote*)list->data;
		if(curTic - sn->tic > TIC_ERROR && sn->tic > ac[sn->col].miss) {
			ac[sn->col].miss = sn->tic;
			if(sn->col == channelFocus && sn->tic >= ap.startTic && sn->tic < ap.stopTic) {
				if(ap.notesHit > 0) multiplier = 1;
				ResetAp();
			}
		}
		list = slist_next(list);
	}
}

void CheckColumn(int row)
{
	int x;
	for(x=0;x<wam->numCols;x++) {
		if(ac[x].cleared == row) ac[x].cleared = 0;
	}
}

int NoteListTic(const void *snp, const void *tp)
{
	int tic = (int)tp;
	const struct screenNote *sn = (const struct screenNote*)snp;
	return sn->tic - tic;
}

int SortByTic(const void *a, const void *b)
{
	const struct screenNote *an = (const struct screenNote*)a;
	const struct screenNote *bn = (const struct screenNote*)b;
	return an->tic - bn->tic;
}

void MoveHitNotes(int tic, int col)
{
	struct screenNote *sn;
	struct slist *tmp;
	struct slist *holder = NULL;
	Log(("moveHIt\n"));
	tmp = slist_find_custom(notesList, (void *)tic, NoteListTic);
	while(tmp) {
		sn = (struct screenNote*)tmp->data;

		if(sn->tic == tic && sn->col == col) {
			hitList = slist_insert_sorted(hitList, sn, SortByTic);
			holder = slist_append(holder, sn);
			sn->ins = __LINE__;
		}
		tmp = slist_next(tmp);
	}
	tmp = holder;
	while(tmp) {
		notesList = slist_remove(notesList, tmp->data);
		tmp = slist_next(tmp);
	}
	slist_free(holder);
	Log(("movehit\n"));
}

void UpdateClearedCols(void)
{
	int x;
	int tic;
	float col[4];
	struct row *r;
	struct obj *o;
	for(x=0;x<wam->numCols;x++) {
		r = &wam->rowData[Row(ac[x].minRow)];
		/* Yes I realize this is a bunch of magic numbers. Sue me. */
		ac[x].part += (double)ticDiff * (double)r->bpm / (833.0 * (double)r->sngspd);
		while(ac[x].part >= 1.0 && ac[x].minRow < ac[x].cleared && ac[x].minRow < wam->numRows) {
			tic = r->ticpos;
			MoveHitNotes(tic, x);
			ac[x].part -= 1.0;
			ac[x].minRow++;
			r++;
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
	}
}

void UpdatePosition(void)
{
	int tmpAdj;
	/* calculate the amount of ticTime elapsed
	 * every 2500 ticTime is one tick
	 */
	if(!menuActive) ticTime += ticDiff * wam->rowData[Row(curRow)].bpm;

	/* adjust the ticTime if our time is different from the
	 * song time.  This is needed in case a little blip in the process
	 * causes the song to be ahead or behind the game, so we can
	 * right ourselves.
	 */
	if(songStarted) {
		tmpAdj = (tickCounter - curTic) << 5;
		Log(("Adj: %i\n", (tickCounter-curTic) << 5));
		if((signed)ticTime + tmpAdj < 0) ticTime = 0;
		else ticTime += tmpAdj;
	}

	while(ticTime >= 2500) {
		ticTime -= 2500;
		curTic++;
		curVb++;
		firstVb++;
		lastVb++;
		CheckMissedNotes();
		if(curVb >= wam->rowData[Row(curRow)].sngspd) {
			curVb -= wam->rowData[Row(curRow)].sngspd;
			curRow++;
			if(curRow > ap.stopRow) ResetAp();
			CheckColumn(Row(curRow));
			if(curRow == 0) { /* start the song! */
				Player_TogglePause();
				songStarted = 1;
				RegisterEvent("menu", menu_handler, EVENTTYPE_MULTI);
			}
		}
		UpdateModule();

		if(firstVb >= wam->rowData[Row(firstRow)].sngspd) {
			firstVb -= wam->rowData[Row(firstRow)].sngspd;
			/* the remove functions check to make sure */
			/* the row is valid, so it's ok to pass firstRow */
			RemoveNotes(firstRow);
			if(firstRow >= 0 && firstRow < wam->numRows)
				FireEvent("de-row", &wam->rowData[firstRow]);
			firstRow++;
		}

		if(lastVb >= wam->rowData[Row(lastRow)].sngspd) {
			lastVb -= wam->rowData[Row(lastRow)].sngspd;
			lastRow++;
			/* the add functions check to make sure the */
			/* row is valid, so it's ok to pass lastRow */
			AddNotes(lastRow);
			if(lastRow >= 0 && lastRow < wam->numRows)
				FireEvent("row", &wam->rowData[lastRow]);
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
	for(col=0;col<wam->numCols;col++) {
		start = startTic;
		stop = stopTic;
		glBindTexture(GL_TEXTURE_2D, mainTexes[col]);
		if(wam->rowData[Row(ac[col].minRow)].ticpos > start)
			start = wam->rowData[Row(ac[col].minRow)].ticpos;
		if(col == channelFocus && ap.startTic != -1) {
			if(ap.startTic > start) start = ap.startTic;
			if(ap.stopTic < stop) stop = ap.stopTic;
		}
		if(start >= stop) {
			glTranslated(-BLOCK_WIDTH, 0, 0);
			continue;
		}
		start *= TIC_HEIGHT;
		stop *= TIC_HEIGHT;
		glBegin(GL_QUADS); {
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

void DrawNotes(void)
{
	struct slist *t;
	GLuint rotnoteList;

	Log(("DN: %i, %i, %i\n", slist_length(notesList), slist_length(hitList), slist_length(unusedList)));
	glDisable(GL_TEXTURE_2D);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, lightNormal);

	rotnoteList = glGenLists(1);
	glNewList(rotnoteList, GL_COMPILE); {
		glRotatef(theta, 0.0, 1.0, 0.0);
		glCallList(noteList);
	} glEndList();

	slist_foreach(t, notesList) {
		float temp[4] = {.7, 0.0, 0.0, 1.0};
		struct screenNote *sn = t->data;
		int mat = abs(sn->tic - curTic) <= TIC_ERROR;

		glPushMatrix();
		glTranslated(	sn->pos.x,
				sn->pos.y,
				sn->pos.z+0.3);
		if(mat) glMaterialfv(GL_FRONT, GL_EMISSION, temp);
		Log(("Dn\n"));
		glCallList(rotnoteList);
		if(mat) glMaterialfv(GL_FRONT, GL_EMISSION, lightNone);
	}
	glDeleteLists(rotnoteList, 1);
	glEnable(GL_TEXTURE_2D);
	Log(("dn\n"));
}

void DrawHitNotes(void)
{
	struct slist *t;

	slist_foreach(t, hitList) {
		int i;
		int j;
		float mat[16];
		struct screenNote *sn = t->data;

		glPushMatrix();
		glTranslated(	sn->pos.x,
				sn->pos.y,
				sn->pos.z+0.3);
		glGetFloatv(GL_MODELVIEW_MATRIX, mat);
		for(i=0;i<3;i++)
			for(j=0;j<3;j++)
			{
				if(i == j) mat[i+j*4] = 1.0;
				else mat[i+j*4] = 0.0;
			}
		glLoadMatrixf(mat);

		if(sn->tic - curTic <= 0)
			glColor4f(1.0, 1.0, 1.0, 1.0);
		else
			glColor4f(0.5, 0.5, 0.5, 1.0);
		glCallList(plist+P_BlueNova);
	}
}

void DrawScoreboard(void)
{
/*	int x; */
	glColor4f(1.0, 1.0, 1.0, 1.0);
/*	for(x=0;x<wam->numCols;x++) */
/*	{ */
/*		PrintGL(0, 75+x*14, "Col %i  Hit %i, %i Clear %4i", x, ac[x].hit, ac[x].miss, ac[x].cleared); */
/*	} */
	PrintGL(50, 0, "Playing: %s", mod->songname);
	if(curRow == wam->numRows) {
		PrintGL(50, 15, "Song complete!");
		if(newhighscore) {
			PrintGL(DisplayWidth() / 2 - 85, 120, "New High Score!!!");
		}
	} else if(curRow < 0) {
		int timeLeft = (int)(0.5 + -2500.0 * (double)wam->rowData[0].sngspd * ((double)curRow) / (1000.0 * (double)wam->rowData[0].bpm));
		if(timeLeft > 0) PrintGL(50, 15, "%i...", timeLeft);
		else PrintGL(50, 15, "GO!!");
	}
/*	if(curRow >= 0 && curRow < wam->numRows) PrintGL(0, 62, "Tick: %i, %i.%f / %i\n", wam->rowData[curRow].ticpos, curTic, partialTic, wam->numTics); */
	PrintGL(50, 30, "Speed: %2i/%i at %i\n", mod->vbtick, mod->sngspd, mod->bpm);
	PrintGL(0, 50, "%i - %i, note: %i, hit: %i/%i\n", ap.startTic, ap.stopTic, ap.lastTic, ap.notesHit, ap.notesTotal);
	PrintGL(350, 20, "Score: %6i High: %i\nMultiplier: %i\n", score, highscore, multiplier);
/*	PrintGL(400, 50, "DN: %i, %i, %i\n", slist_length(notesList), slist_length(hitList), slist_length(unusedList)); */


	SetOrthoProjection();
	glDisable(GL_TEXTURE_2D);

	glColor4f(0.0, 0.8, 0.5, 1.0);
	glBegin(GL_QUADS); {
		glVertex2i(5, 80);
		glVertex2i(15, 80);
		glVertex2i(15, 450);
		glVertex2i(5, 450);
	} glEnd();

	glColor4f(0.0, 0.0, 0.0, 1.0);
	glBegin(GL_QUADS); {
		glVertex2i(6, 81);
		glVertex2i(14, 81);
		glVertex2i(14, 449);
		glVertex2i(6, 449);
	} glEnd();

	glBegin(GL_QUADS); {
		float mult = (double)curRow / (double)wam->numRows;
		glColor4f(0.0, 0.8, 0.5, 1.0);
		glVertex2i(6, 81*mult+449*(1.0-mult));
		glColor4f(0.5, 0.8, 0.5, 1.0);
		glVertex2i(14, 81*mult+449*(1.0-mult));
		glColor4f(0.0, 0.5, 0.0, 1.0);
		glVertex2i(14, 449);
		glColor4f(0.0, 0.0, 0.0, 1.0);
		glVertex2i(6, 449);
	} glEnd();
	glEnable(GL_TEXTURE_2D);
	ResetProjection();
	glColor4f(1.0, 1.0, 1.0, 1.0);
}

int BelowBoard(struct particle *p)
{
	if(p->o->pos.y < 0.0) return 1;
	return 0;
}

int AboveBoard(struct particle *p)
{
	if(p->o->pos.y >= 0.0) return 1;
	return 0;
}