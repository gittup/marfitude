#include "SDL_opengl.h"

#include "marfitude.h"

#include "gmae/event.h"
#include "gmae/glfunc.h"
#include "gmae/module.h"
#include "gmae/wam.h"

#include "util/plugin.h"

/* tmp */
extern int rowIndex;
extern struct wam *wam;
extern int highscore;
extern int score;
extern int newhighscore;
extern int multiplier;
extern double modTime;
/*extern struct attackPattern ap;*/
/* endtmp */

static int scoreboard_init(void);
static void scoreboard_exit(void);
static void draw_scoreboard(const void *);

int scoreboard_init(void)
{
	RegisterEvent("draw opaque", draw_scoreboard, EVENTTYPE_MULTI);
	return 0;
}

void scoreboard_exit(void)
{
	DeregisterEvent("draw opaque", draw_scoreboard);
}

void draw_scoreboard(const void *data)
{
	if(data) {}

	glColor4f(1.0, 1.0, 1.0, 1.0);

	PrintGL(50, 0, "Playing: %s", mod->songname);
	if(rowIndex == wam->numRows) {
		PrintGL(50, 15, "Song complete!");
		if(newhighscore) {
			PrintGL(DisplayWidth() / 2 - 85, 120, "New High Score!!!");
		}
	} else if(rowIndex < 0) {
		int timeLeft = (int)(- BpmToSec(wam->rowData[0].sngspd, wam->rowData[0].bpm) * (double)rowIndex);
		if(timeLeft > 0) PrintGL(50, 15, "%i...", timeLeft);
		else PrintGL(50, 15, "GO!!");
	}
	PrintGL(50, 30, "Speed: %2i/%i at %i\n", mod->vbtick, mod->sngspd, mod->bpm);
/*	PrintGL(0, 50, "%i - %i, note: %i, hit: %i/%i  %.2f/%.2f\n", ap.startTic, ap.stopTic, ap.lastTic, ap.notesHit, ap.notesTotal, modTime, wam->songLength);*/
	PrintGL(350, 20, "Score: %6i High: %i\nMultiplier: %i\n", score, highscore, multiplier);

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
		float mult = modTime / wam->songLength;
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

plugin_init(scoreboard_init);
plugin_exit(scoreboard_exit);
