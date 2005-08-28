#include "SDL_opengl.h"

#include "marfitude.h"
#include "scoreboard.h"

#include "gmae/event.h"
#include "gmae/glfunc.h"
#include "gmae/input.h"
#include "gmae/module.h"
#include "gmae/wam.h"

static void draw_scoreboard(const void *);

void scoreboard_init(void)
{
	register_event("draw opaque", draw_scoreboard);
}

void scoreboard_exit(void)
{
	deregister_event("draw opaque", draw_scoreboard);
}

void draw_scoreboard(const void *data)
{
	struct marfitude_pos pos;
	const struct wam *wam = marfitude_get_wam();
	const struct marfitude_player *ps;
	int highscore = marfitude_get_highscore();
	int p;

	if(data) {}

	marfitude_get_pos(&pos);
	glColor4f(1.0, 1.0, 1.0, 1.0);

	print_gl(20, 0, "Playing: %s", mod->songname);
	if(pos.row_index == wam->numRows) {
		print_gl(50, 15, "Song complete!");
		for(p=0; p<marfitude_num_players(); p++) {
			ps = marfitude_get_player(p);
			if(ps->score.score > highscore) {
				print_gl(display_width() / 2 - 85, 120, "New High Score!!!");
			}
		}
	} else if(pos.row_index < 0) {
		int timeLeft = (int)(- BpmToSec(wam->rowData[0].sngspd, wam->rowData[0].bpm) * (double)pos.row_index);
		if(timeLeft > 0) print_gl(50, 15, "%i...", timeLeft);
		else print_gl(50, 15, "GO!!");
	}
	print_gl(50, 30, "Speed: %2i/%i at %i\n", mod->vbtick, mod->sngspd, mod->bpm);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	if(highscore) {
		print_gl(350, 0, "Highscore: %i", highscore);
	}

	print_gl(0, 65, "%.2f/%.2f\n", pos.modtime, wam->songLength);

	print_gl(300, 20, "Score:");
	print_gl(300, 35, "Multiplier:");
	print_gl(300, 50, "Hits:");

	for(p=0; p<marfitude_num_players(); p++) {
		int space = p * 6 * FONT_WIDTH;
		ps = marfitude_get_player(p);

		glColor4fv(get_player_color(p));
		print_gl(420 + space, 35, "%i\n", ps->score.multiplier);
		print_gl(420 + space, 50, "%i/%i\n", ps->ap.notesHit, ps->ap.notesTotal);

		if(ps->score.score && ps->score.score == marfitude_get_local_highscore()) {
			if(ps->score.score > highscore)
				glColor4f(1.0, 1.0, 0.7, 1.0);
			else if(marfitude_num_players() > 1)
				glColor4f(0.8, 0.8, 0.8, 1.0);
		}
		print_gl(420 + space - FONT_WIDTH * 5, 20, "%6i\n", ps->score.score);
	}

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
		float mult = pos.modtime / wam->songLength;
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
