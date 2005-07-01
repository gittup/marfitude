#include "SDL_opengl.h"

#include "marfitude.h"

#include "gmae/event.h"
#include "gmae/glfunc.h"
#include "gmae/module.h"
#include "gmae/wam.h"

void scoreboard_init(void) __attribute__ ((constructor));
void scoreboard_exit(void) __attribute__ ((destructor));
static void draw_scoreboard(const void *);

void scoreboard_init(void)
{
	register_event("draw opaque", draw_scoreboard, EVENTTYPE_MULTI);
}

void scoreboard_exit(void)
{
	deregister_event("draw opaque", draw_scoreboard);
}

void draw_scoreboard(const void *data)
{
	struct marfitude_pos p;
	const struct wam *wam = marfitude_get_wam();
	const struct marfitude_score *s = marfitude_get_score();
	if(data) {}

	marfitude_get_pos(&p);
	glColor4f(1.0, 1.0, 1.0, 1.0);

	print_gl(50, 0, "Playing: %s", mod->songname);
	if(p.row_index == wam->numRows) {
		print_gl(50, 15, "Song complete!");
		if(s->score > s->highscore) {
			print_gl(display_width() / 2 - 85, 120, "New High Score!!!");
		}
	} else if(p.row_index < 0) {
		int timeLeft = (int)(- BpmToSec(wam->rowData[0].sngspd, wam->rowData[0].bpm) * (double)p.row_index);
		if(timeLeft > 0) print_gl(50, 15, "%i...", timeLeft);
		else print_gl(50, 15, "GO!!");
	}
	print_gl(50, 30, "Speed: %2i/%i at %i\n", mod->vbtick, mod->sngspd, mod->bpm);
/*	print_gl(0, 50, "%i - %i, note: %i, hit: %i/%i  %.2f/%.2f\n", ap.startTic, ap.stopTic, ap.lastTic, ap.notesHit, ap.notesTotal, p.modtime, wam->songLength);*/
	if(s->score > s->highscore) {
		glColor4f(1.0, 1.0, 0.7, 1.0);
	} else {
		glColor4f(1.0, 1.0, 1.0, 1.0);
	}
	print_gl(350, 20, "Score: %6i\n", s->score);

	glColor4f(1.0, 1.0, 1.0, 1.0);
	if(s->highscore) {
		print_gl(490, 20, "High: %i", s->highscore);
	}
	print_gl(350, 34, "Multiplier: %i\n", s->multiplier);

	set_ortho_projection();
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
		float mult = p.modtime / wam->songLength;
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
	reset_projection();
	glColor4f(1.0, 1.0, 1.0, 1.0);
}
