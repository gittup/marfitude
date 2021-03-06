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
	register_event("draw ortho", draw_scoreboard);
}

void scoreboard_exit(void)
{
	deregister_event("draw ortho", draw_scoreboard);
}

void draw_scoreboard(const void *data)
{
	struct marfitude_pos pos;
	const struct wam *wam = marfitude_get_wam();
	const struct marfitude_player *ps;
	int highscore = marfitude_get_highscore();
	int x1, x2, y1, y2;
	int difficulty;

	if(data) {}

	marfitude_get_pos(&pos);
	glColor4f(1.0, 1.0, 1.0, 1.0);

	difficulty = marfitude_get_difficulty();

	print_gl(50, 0, "Playing: %s", mod->songname);
	if(pos.row_index == wam->num_rows) {
		print_gl(50, 45, "Song complete!");
		marfitude_foreach_player(ps) {
			if(ps->score.score > highscore) {
				print_gl(640 / 2 - 85, 120, "New High Score!!!");
				break;
			}
		}
	} else if(pos.row_index < 0) {
		int timeLeft = (int)(- BpmToSec(wam->row_data[0].sngspd, wam->row_data[0].bpm) * (double)pos.row_index);
		if(timeLeft > 0) print_gl(50, 45, "%i...", timeLeft);
		else print_gl(50, 45, "GO!!");
	}
	print_gl(50, 15, "Difficulty: %i", difficulty);
	print_gl(50, 30, "Speed: %2i/%-2i at %i", mod->vbtick, mod->sngspd, mod->bpm);
	if(highscore) {
		print_gl(300, 15, "Highscore: %i", highscore);
	}

	print_gl(0, 65, "%.2f/%.2f\n", pos.modtime, wam->song_length);

	print_gl(300, 30, "Score:");
	print_gl(300, 45, "Multiplier:");
	print_gl(300, 60, "Hits:");

	marfitude_foreach_player(ps) {
		int space = ps->num * 6 * FONT_WIDTH;

		glColor4fv(get_player_color(ps->num));
		print_gl(420 + space, 45, "%i\n", ps->score.multiplier);
		print_gl(420 + space, 60, "%i/%i\n", ps->ap.notesHit, ps->ap.notesTotal);

		if(ps->score.score && ps->score.score == marfitude_get_local_highscore()) {
			if(ps->score.score > highscore)
				glColor4f(1.0, 1.0, 0.7, 1.0);
			else if(marfitude_num_players() > 1)
				glColor4f(0.8, 0.8, 0.8, 1.0);
		}
		print_gl(420 + space - FONT_WIDTH * 5, 30, "%6i\n", ps->score.score);
	}

	glDisable(GL_TEXTURE_2D);

	x1 = orthox(5);
	x2 = orthox(15);
	y1 = orthoy(80);
	y2 = orthoy(450);
	glColor4f(0.0, 0.8, 0.5, 1.0);
	glBegin(GL_QUADS); {
		glVertex2i(x1, y1);
		glVertex2i(x1, y2);
		glVertex2i(x2, y2);
		glVertex2i(x2, y1);
	} glEnd();

	glColor4f(0.0, 0.0, 0.0, 1.0);
	glBegin(GL_QUADS); {
		glVertex2i(x1+1, y1+1);
		glVertex2i(x1+1, y2-1);
		glVertex2i(x2-1, y2-1);
		glVertex2i(x2-1, y1+1);
	} glEnd();

	glBegin(GL_QUADS); {
		float mult = pos.modtime / wam->song_length;
		glColor4f(0.7, 1.0, 0.7, 1.0);
		glVertex2i(x1+1, (y1+1)*mult+(y2-1)*(1.0-mult));
		glColor4f(0.0, 0.0, 0.0, 1.0);
		glVertex2i(x1+1, y2-1);
		glColor4f(0.0, 0.5, 0.0, 1.0);
		glVertex2i(x2-1, y2-1);
		glColor4f(0.7, 1.0, 0.7, 1.0);
		glVertex2i(x2-1, (y1+1)*mult+(y2-1)*(1.0-mult));
	} glEnd();
	glEnable(GL_TEXTURE_2D);
	glColor4f(1.0, 1.0, 1.0, 1.0);
}
