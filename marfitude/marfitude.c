#include <math.h>
#include <string.h>

#include "marfitude.h"
#include "objs/bluenotes.h"
#include "objs/explode.h"
#include "objs/fireball.h"
#include "objs/greynotes.h"
#include "objs/laser.h"
#include "objs/lines.h"
#include "objs/rows.h"
#include "objs/scoreboard.h"
#include "objs/targets.h"

#include "SDL_opengl.h"

#include "gmae/cfg.h"
#include "gmae/event.h"
#include "gmae/glfunc.h"
#include "gmae/input.h"
#include "gmae/log.h"
#include "gmae/mainscene.h"
#include "gmae/menu.h"
#include "gmae/module.h"
#include "gmae/particles.h"
#include "gmae/textures.h"
#include "gmae/timer.h"
#include "gmae/wam.h"

#include "util/memtest.h"
#include "util/myrand.h"
#include "util/plugin.h"
#include "util/slist.h"


static Uint32 ticTime;
static int channelFocus = 0;
static double viewFocus = 0.0;
static struct wam *wam; /* note file */
static int difficulty = 0;

/* returns 1 if the row is valid, 0 otherwise */
#define IsValidRow(row) ((row >= 0 && row < wam->numRows) ? 1 : 0)
/* return a clamped row number */
#define Row(row) (row < 0 ? 0 : (row >= wam->numRows ? wam->numRows - 1: row))
#define Max(a, b) ((a) > (b) ? (a) : (b))

static int curTic; /* tick counter from 0 - total ticks in the song */
static int firstVb, curVb, lastVb;     /* tick counters for the three rows */
static int firstRow, rowIndex, lastRow;  /* first row on screen, current row
                                   * playing and the last row on screen
                                   */
static struct row *curRow;
static double modTime;
static double partialTic;

static void ResetAp(void);
static void ChannelUp(int);
static void ChannelDown(int);
static struct column *ColumnFromNum(int col);
static void Setmute(struct column *c, int mute);
static void UpdateModule(void);
static int is_note(int row, int col);
static void AddNotes(int row);
static struct slist *RemoveList(struct slist *list, int tic);
static void RemoveNotes(int row);

static void Press(int button);
static void SetMainView(void);
static void MoveHitNotes(int tic, int col);
static void UpdateClearedCols(void);
static void UpdatePosition(void);
static int NearRow(void);
static struct marfitude_note *FindNote(struct slist *list, int tic, int col);
static int get_clear_column(int start, int skip_lines);
static void FixVb(int *vb, int *row);
static void TickHandler(void);
static void CheckMissedNotes(void);
static void CheckColumn(int row);
static int NoteListTic(const void *snp, const void *tp);
static int SortByTic(const void *a, const void *b);
static void menu_handler(const void *data);
static void button_handler(const void *data);

static struct marfitude_attack_pat ap;
static struct marfitude_attack_col ac[MAX_COLS];
static struct marfitude_note *notesOnScreen; /* little ring buffer of notes */
static struct slist *unusedList;	/* unused notes */
static struct slist *notesList;	/* notes on the screen, not hit */
static struct slist *hitList;	/* notes on the screen, hit */
static int numNotes;	/* max number of notes on screen (wam->numCols * NUM_TICKS) */
static char *cursong;

static struct marfitude_score score;

static void *plugin = NULL;
static int *noteOffset;
static MikMod_player_t oldHand;
static int tickCounter;
static int songStarted;	/* this is set once we get to the first row, and the
			 * song is unpaused
			 */

void menu_handler(const void *data)
{
	const struct menu_e *m = data;

	if(m->active ^ Player_Paused()) {
		Player_TogglePause();
	}
}

void button_handler(const void *data)
{
	static int lastkeypressed[2] = {1, 1};
	const struct button_e *b = data;

	switch(b->button) {
		case B_RIGHT:
			ChannelUp(b->shift);
			break;
		case B_LEFT:
			ChannelDown(b->shift);
			break;
		case B_BUTTON1:
			if(b->shift) {
				lastkeypressed[0] = 1;
				lastkeypressed[1] = 1;
				Press(1);
				Press(1);
			} else {
				lastkeypressed[0] = lastkeypressed[1];
				lastkeypressed[1] = 1;
				Press(1);
			}
			break;
		case B_BUTTON2:
			if(b->shift) {
				lastkeypressed[0] = 2;
				lastkeypressed[1] = 2;
				Press(2);
				Press(2);
			} else {
				lastkeypressed[0] = lastkeypressed[1];
				lastkeypressed[1] = 2;
				Press(2);
			}
			break;
		case B_BUTTON3:
			/* yes, this is really 4 (3rd bit) */
			if(b->shift) {
				lastkeypressed[0] = 4;
				lastkeypressed[1] = 4;
				Press(4);
				Press(4);
			} else {
				lastkeypressed[0] = lastkeypressed[1];
				lastkeypressed[1] = 4;
				Press(4);
			}
			break;
		case B_BUTTON4:
			if(b->shift) {
				Press(lastkeypressed[0]);
				Press(lastkeypressed[1]);
			} else {
				lastkeypressed[0] = lastkeypressed[1];
				Press(lastkeypressed[1]);
			}
			break;
		case B_UP:
		case B_DOWN:
		default:
			break;
	}
}

int main_init()
{
	int x;
	char *scene;

	Log(("Load Wam\n"));
	cursong = cfg_copy("main", "song");

	wam = load_wam(cursong);
	if(wam == NULL) {
		ELog(("Error: Couldn't load WAM file\n"));
		return 1;
	}
	Log(("Start module\n"));
	if(start_module(cursong)) {
		ELog(("Error: Couldn't start module\n"));
		return 2;
	}
	/* module and module data (where to place the notes) are now loaded,
	 * and the module is paused
	 */
	Log(("Module ready\n"));
	difficulty = cfg_get_int("main", "difficulty");
	score.highscore = cfg_get_int("highscore", cursong);
	score.score = 0;
	score.multiplier = 1;
	tickCounter = 0;
	songStarted = 0;
	modTime = -5.0;
	oldHand = MikMod_RegisterPlayer(TickHandler);
	noteOffset = malloc(sizeof(int) * (MAX_NOTE+1));

	noteOffset[1] = -1;
	noteOffset[2] = 0;
	noteOffset[4] = 1;

	curRow = &wam->rowData[Row(0)];

	scene = cfg_get("main", "scene");
	if(strcmp(scene, "scenes/default") == 0) {
		rows_init();
		laser_init();
		targets_init();
		lines_init();
		greynotes_init();
		bluenotes_init();
		fireball_init();
		explode_init();
		scoreboard_init();
		plugin = NULL;
	} else {
		plugin = load_plugin(scene);
	}

	ticTime = 0;
	channelFocus = 0;
	viewFocus = 0.0;

	for(x=0;x<wam->numCols;x++) {
		int y;
		ac[x].part = 0.0;
		ac[x].cleared = 0;
		for(y=0; y<LINES_PER_AP*x; y++) {
			ac[x].cleared++;
			while(ac[x].cleared < wam->numRows && wam->rowData[ac[x].cleared].line == 0)
				ac[x].cleared++;
		}
		ac[x].minRow = ac[x].cleared;
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
	FixVb(&curVb, &rowIndex);
	FixVb(&lastVb, &lastRow);

	partialTic = 0.0;

	numNotes = wam->numCols * NUM_TICKS;
	notesOnScreen = malloc(sizeof(struct marfitude_note) * numNotes);
	unusedList = NULL;
	notesList = NULL;
	hitList = NULL;
	for(x=0;x<numNotes;x++) {
		unusedList = slist_append(unusedList, (void *)&notesOnScreen[x]);
		notesOnScreen[x].ins = __LINE__;
	}
	for(x=0;x<=lastRow;x++) AddNotes(x);

	for(x=0;x<=lastRow;x++) fire_event("row", &wam->rowData[x]);

	ap.nextStartRow = -1;
	ResetAp();
	register_event("button", button_handler, EVENTTYPE_MULTI);

	init_timer();
	Log(("Lists created\n"));
	return 0;
}

void main_quit(void)
{
	Log(("Main Scene quit\n"));
	if(score.score > score.highscore) {
		cfg_set_int("highscore", cursong, score.score);
	}
	if(plugin == NULL) {
		scoreboard_exit();
		explode_exit();
		fireball_exit();
		bluenotes_exit();
		greynotes_exit();
		lines_exit();
		targets_exit();
		laser_exit();
		rows_exit();
	} else {
		free_plugin(plugin);
	}
	free(cursong);
	oldHand = MikMod_RegisterPlayer(oldHand);
	Log(("A\n"));
	if(songStarted) {
		deregister_event("menu", menu_handler);
	}
	Log(("A\n"));
	songStarted = 0;
	deregister_event("button", button_handler);
	Log(("A\n"));
	free(notesOnScreen);
	Log(("A\n"));
	free(noteOffset);
	Log(("A\n"));
	clear_particles();
	Log(("A\n"));
	check_objs();
	Log(("A\n"));
	free_wam(wam);
	Log(("A\n"));
	stop_module();
	slist_free(hitList);
	slist_free(notesList);
	slist_free(unusedList);
	Log(("Main scene quit finished\n"));
}

void main_scene(void)
{
	Log(("main_scene\n"));

	/* Set key repeating here in case the configuration changes mid-game */
	set_key_repeat(B_BUTTON1, 0);
	set_key_repeat(B_BUTTON2, 0);
	set_key_repeat(B_BUTTON3, 0);
	set_key_repeat(B_BUTTON4, 0);

	glLoadIdentity();

	if(rowIndex != wam->numRows) UpdatePosition();
	update_objs(timeDiff);
	SetMainView();

	set_ortho_projection();
	fire_event("draw ortho", NULL);
	reset_projection();

	fire_event("draw opaque", NULL);

	glDisable(GL_LIGHTING);
	glDepthMask(GL_FALSE);
	fire_event("draw transparent", NULL);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	draw_particles();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDepthMask(GL_TRUE);
	glEnable(GL_LIGHTING);

	Log(("endMainScene\n"));
}

void ChannelUp(int shift)
{
	if(channelFocus + 1 < wam->numCols) {
		if(shift)
			channelFocus = wam->numCols - 1;
		else
			channelFocus++;
		if(ap.notesHit > 0) score.multiplier = 1;
		ResetAp();
		ac[channelFocus].hit = ap.startTic - 1;
	}
}

void ChannelDown(int shift)
{
	if(channelFocus != 0) {
		if(shift)
			channelFocus = 0;
		else
			channelFocus--;
		if(ap.notesHit > 0) score.multiplier = 1;
		ResetAp();
		ac[channelFocus].hit = ap.startTic - 1;
	}
}

struct column *ColumnFromNum(int col)
{
	return &wam->patterns[curRow->patnum].columns[col];
}

void Setmute(struct column *c, int mute)
{
	int x;
	for(x=0;x<c->numchn;x++) {
		if(mute) Player_Mute(c->chan[x]);
		else Player_Unmute(c->chan[x]);
	}
}

/* update which channels are playing based on marfitude_attack_col contents */
void UpdateModule(void)
{
	int x;
	for(x=0;x<wam->numCols;x++) {
		if(ac[x].cleared || ac[x].hit > ac[x].miss)
			Setmute(ColumnFromNum(x), UNMUTE);
		else
			Setmute(ColumnFromNum(x), MUTE);
	}
	Setmute(&wam->patterns[curRow->patnum].unplayed, UNMUTE);
}

int is_note(int row, int col)
{
	if(wam->rowData[row].notes[col] &&
			wam->rowData[row].difficulty[col] <= difficulty)
		return 1;
	return 0;
}
void AddNotes(int row)
{
	int x;
	int tic;
	struct marfitude_note *sn;
	if(row < 0 || row >= wam->numRows) return;
	tic = wam->rowData[row].ticpos;
	for(x=0;x<wam->numCols;x++) {
		if(is_note(row, x)) {
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
			sn->time = wam->rowData[row].time;
			sn->col = x;
			sn->difficulty = wam->rowData[row].difficulty[x];
		}
	}
}

struct slist *RemoveList(struct slist *list, int tic)
{
	struct marfitude_note *sn;
	while(list) {
		sn = (struct marfitude_note*)list->data;
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

struct marfitude_note *FindNote(struct slist *list, int tic, int col)
{
	struct marfitude_note *sn;
	while(list) {
		sn = (struct marfitude_note*)list->data;
		if(sn->tic == tic && sn->col == col) return sn;
		list = slist_next(list);
	}
	return NULL;
}

/** Convert the column @a start plus @a lines line columns to the next line'd
 * column. Also make sure there is enough space at the end for another attack
 * pattern.
 */
int get_clear_column(int start, int skip_lines)
{
	int lines = 0;
	int tmp;

	while(start < wam->numRows && skip_lines > 0) {
		if(wam->rowData[start].line != 0)
			skip_lines--;
		start++;
	}
	/* clear to a line break */
	while(start < wam->numRows && wam->rowData[start].line == 0)
		start++;
	tmp = start;
	while(tmp < wam->numRows && lines < LINES_PER_AP) {
		if(wam->rowData[tmp].line != 0)
			lines++;
		tmp++;
	}

	/* Not enough space to start a new AP, set to the end of the song */
	if(lines < LINES_PER_AP)
		start = wam->numRows;
	return start;
}

void Press(int button)
{
	int i;
	int noteHit = 0;
	int rowStart;
	int rowStop;
	struct marfitude_note *sn;
	struct row *r;

	fire_event("shoot", &button);

	rowStart = rowIndex;
	while(rowStart > 0 && curRow->time - wam->rowData[Row(rowStart)].time < MARFITUDE_TIME_ERROR)
		rowStart--;

	rowStop = rowIndex;
	while(rowStop < wam->numRows && wam->rowData[Row(rowStop)].time - curRow->time < MARFITUDE_TIME_ERROR)
		rowStop++;

	for(i=rowStart;i<=rowStop;i++) {
		if(i >= 0 && i < wam->numRows) {
			r = &wam->rowData[Row(i)];
			/* if we're in the attackpattern limits, and
			 * if we didn't already play this row, and this row 
			 * is within our acceptable error, and we hit the
			 * right note, and it's within the difficulty range,
			 * then yay
			 */
			if(	
				r->ticpos >= ap.startTic &&
				r->ticpos < ap.stopTic &&
				r->ticpos > ap.lastTic &&
				fabs(r->time - modTime) <= MARFITUDE_TIME_ERROR &&
				button == r->notes[channelFocus] &&
				r->difficulty[channelFocus] <= difficulty) {

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
					ac[channelFocus].cleared = get_clear_column(ap.stopRow, LINES_PER_AP * wam->numCols);
					ac[channelFocus].minRow = rowIndex;
					ac[channelFocus].part = 0.0;
					score.score += ap.notesHit * score.multiplier;
					if(score.multiplier < 8)
						score.multiplier++;
					ap.notesHit = 0;
				}
				ac[channelFocus].hit = r->ticpos;
				noteHit = 1;
				r->notes[channelFocus] = 0;
				break;
			}
		}
	}

	if(!noteHit && curRow->ticpos >= ap.startTic && curRow->ticpos < ap.stopTic) {
		/* oops, we missed! */
		if(ap.notesHit > 0) score.multiplier = 1;
		if(IsValidRow(rowIndex) && curRow->ticpos >= ap.startTic && curRow->ticpos < ap.stopTic)
			ac[channelFocus].miss = curRow->ticpos;
		ResetAp();
	}
	UpdateModule();
}

void SetMainView(void)
{
	float mainPos[3] = {0.0, 3.0, -8.0};
	float mainView[3] = {0.0, 0.8, 0.0};
	double tmp;

	/* Make sure the view focus update doesn't go past channel focus.
	 * It should be a smooth transition and stop when it gets there.
	 */
	tmp = viewFocus + ((double)channelFocus - viewFocus) * timeDiff * 8.0;
	if( (channelFocus < viewFocus) != (channelFocus < tmp) ) {
		viewFocus = (double)channelFocus;
	} else {
		viewFocus = tmp;
	}
	mainView[2] = TIC_HEIGHT * ((double)curTic + partialTic);
	mainPos[2] = mainView[2] - 8.0;

	glLoadIdentity();
	gluLookAt(	mainPos[0] - viewFocus * BLOCK_WIDTH, mainPos[1], mainPos[2],
			mainView[0] - viewFocus * BLOCK_WIDTH, mainView[1], mainView[2],
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

/* keep track of the number of actual ticks played by the mod player,
 * so we know if we're off
 */
void TickHandler(void)
{
	if(songStarted) {
		if(!Player_Paused()) tickCounter++;
	}
	oldHand();
}

/* gets either rowIndex or rowIndex+1, depending on which one row is closest
 * based on the time (eg within acceptable error)
 */
int NearRow(void)
{
	if(modTime - curRow->time < MARFITUDE_TIME_ERROR)
		return rowIndex;
	return rowIndex+1;
}

void ResetAp(void)
{
	int start;
	int end;
	int apLines = 0;

	ap.notesHit = 0;
	ap.notesTotal = 0;
	start = Row(Max(Max(NearRow(), ap.nextStartRow), ac[channelFocus].cleared));
	while(start < wam->numRows && (wam->rowData[start].line == 0 || wam->rowData[start].ticpos <= ac[channelFocus].miss))
		start++;
	if(start == wam->numRows)
		start = wam->numRows - 1;
	end = start;

	/* Make sure there are at least LINES_PER_AP lines in the AP */
	while(apLines < LINES_PER_AP && end < wam->numRows) {
		/* don't count the note on the last row, since that will
		 * be the beginning of the next "AttackPattern"
		 */
		if(is_note(end, channelFocus)) ap.notesTotal++;
		end++;
		if(wam->rowData[end].line != 0) apLines++;
	}

	/* Make sure there is at least one note in the AP */
	while(ap.notesTotal == 0 && end < wam->numRows) {
		if(is_note(end, channelFocus)) ap.notesTotal++;
		end++;
	}

	/* Make sure the AP ends on a line */
	while(wam->rowData[end].line == 0 && end < wam->numRows) {
		if(is_note(end, channelFocus)) ap.notesTotal++;
		end++;
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
	struct marfitude_note *sn;
	struct slist *list;
	list = notesList;
	while(list) {
		sn = (struct marfitude_note*)list->data;
		if(modTime - sn->time > MARFITUDE_TIME_ERROR && sn->tic > ac[sn->col].miss) {
			ac[sn->col].miss = sn->tic;
			if(sn->col == channelFocus && sn->tic >= ap.startTic && sn->tic < ap.stopTic) {
				if(ap.notesHit > 0)
					score.multiplier = 1;
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
	const struct marfitude_note *sn = (const struct marfitude_note*)snp;
	return sn->tic - tic;
}

int SortByTic(const void *a, const void *b)
{
	const struct marfitude_note *an = (const struct marfitude_note*)a;
	const struct marfitude_note *bn = (const struct marfitude_note*)b;
	return an->tic - bn->tic;
}

void MoveHitNotes(int tic, int col)
{
	struct marfitude_note *sn;
	struct slist *tmp;
	struct slist *holder = NULL;
	Log(("moveHIt\n"));
	tmp = slist_find_custom(notesList, (void *)tic, NoteListTic);
	while(tmp) {
		sn = (struct marfitude_note*)tmp->data;

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
	struct row *r;

	for(x=0;x<wam->numCols;x++) {
		r = &wam->rowData[Row(ac[x].minRow)];
		/* Yes I realize this is a bunch of magic numbers. Sue me. */
		ac[x].part += (double)ticDiff * (double)r->bpm / (833.0 * (double)r->sngspd);
		while(ac[x].part >= 1.0 && ac[x].minRow < ac[x].cleared && ac[x].minRow < wam->numRows) {
			struct marfitude_pos p;

			tic = r->ticpos;
			MoveHitNotes(tic, x);

			p.modtime = modTime;
			p.tic = tic;
			p.row = r;
			p.row_index = Row(ac[x].minRow);
			p.channel =  x;
			fire_event("row explosion", &p);

			ac[x].part -= 1.0;
			ac[x].minRow++;
			r++;
		}
	}
}

void UpdatePosition(void)
{
	int tmpAdj;
	/* calculate the amount of ticTime elapsed
	 * every 2500 ticTime is one tick
	 */
	if(!is_menu_active()) ticTime += ticDiff * curRow->bpm;

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
		if(curVb >= curRow->sngspd) {
			curVb -= curRow->sngspd;
			rowIndex++;
			curRow = &wam->rowData[Row(rowIndex)];
			if(rowIndex > ap.stopRow) ResetAp();
			CheckColumn(Row(rowIndex));
			if(rowIndex == 0) { /* start the song! */
				Player_TogglePause();
				songStarted = 1;
				register_event("menu", menu_handler, EVENTTYPE_MULTI);
			}
		}
		modTime = curRow->time + (curTic - curRow->ticpos) * BpmToSec(curRow->sngspd, curRow->bpm) / curRow->sngspd;
		UpdateModule();

		if(firstVb >= wam->rowData[Row(firstRow)].sngspd) {
			firstVb -= wam->rowData[Row(firstRow)].sngspd;
			/* the remove functions check to make sure 
			 * the row is valid, so it's ok to pass firstRow
			 */
			RemoveNotes(firstRow);
			if(firstRow >= 0 && firstRow < wam->numRows)
				fire_event("de-row", &wam->rowData[firstRow]);
			firstRow++;
		}

		if(lastVb >= wam->rowData[Row(lastRow)].sngspd) {
			lastVb -= wam->rowData[Row(lastRow)].sngspd;
			lastRow++;
			/* the add functions check to make sure the
			 * row is valid, so it's ok to pass lastRow
			 */
			AddNotes(lastRow);
			if(lastRow >= 0 && lastRow < wam->numRows)
				fire_event("row", &wam->rowData[lastRow]);
		}
	}
	UpdateClearedCols();
	partialTic = (double)ticTime / 2500.0;
}

/** Gets the wam structure for the current song.
 * @return The wam struct.
 */
const struct wam *marfitude_get_wam(void)
{
	return wam;
}

/** Gets the note offsets, size MAX_NOTE+1 */
const int *marfitude_get_offsets(void)
{
	return noteOffset;
}

/** Gets the current score structure and stores it in @a s */
const struct marfitude_score *marfitude_get_score(void)
{
	return &score;
}

/** Returns a slist of the notes to play */
const struct slist *marfitude_get_notes(void)
{
	return notesList;
}

/** Returns a slist of the exploded notes */
const struct slist *marfitude_get_hitnotes(void)
{
	return hitList;
}

/** Returns an array of attack columns. Size is MAX_COLS, only wam->numCols
 * are populated with stuff.
 */
const struct marfitude_attack_col *marfitude_get_ac(void)
{
	return ac;
}

/** Gets the current attack pattern. */
const struct marfitude_attack_pat *marfitude_get_ap(void)
{
	return &ap;
}

/** Gets the current module time in seconds */
void marfitude_get_pos(struct marfitude_pos *p)
{
	p->modtime = modTime;
	p->tic = (double)curTic + partialTic;
	p->row = curRow;
	p->row_index = rowIndex;
	p->channel = channelFocus;
	p->view = viewFocus;
}
