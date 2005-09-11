#include "SDL_opengl.h"

#include "marfitude.h"
#include "rows.h"

#include "gmae/event.h"
#include "gmae/textures.h"
#include "gmae/wam.h"

#include "util/slist.h"

static void draw_rows(const void *data);

static GLuint row_texes[MAX_COLS];

void rows_init(void)
{
	row_texes[0] = texture_num("Slate.png");
	row_texes[1] = texture_num("Walnut.png");
	row_texes[2] = texture_num("ElectricBlue.png");
	row_texes[3] = texture_num("Clovers.png");
	row_texes[4] = texture_num("Lava.png");
	row_texes[5] = texture_num("Parque3.png");
	row_texes[6] = texture_num("Slate.png");
	row_texes[7] = texture_num("ElectricBlue.png");

	register_event("draw opaque", draw_rows);
}

void rows_exit(void)
{
	deregister_event("draw opaque", draw_rows);
}

void draw_rows(const void *data)
{
	int col;
	double start, stop;
	double startTic, stopTic;
	struct marfitude_pos pos;
	const struct wam *wam = marfitude_get_wam();
	const struct marfitude_attack_col *ac = marfitude_get_ac();
	const struct marfitude_player *ps;

	if(data) {}
	marfitude_get_pos(&pos);
	glColor4f(1.0, 1.0, 1.0, 1.0);

	/* usually we draw from -NEGATIVE_TICKS to +POSITIVE_TICKS, with the
	 * notes currently being played at position 0.
	 * At the beginning of the song, we start drawing instead from 
	 * 0 to +POSITIVE_TICKS, and at the end of the song we draw from
	 * -NEGATIVE_TICKS to wam->num_tics.  Of course, if the song is less than
	 * NEGATIVE_TICKS + POSITIVE_TICKS long, some other combinations will
	 * arise :)
	 */
	startTic = pos.tic - NEGATIVE_TICKS >= 0 ?
		((double)pos.tic - NEGATIVE_TICKS) : 0.0;
	stopTic = pos.tic < wam->num_tics - POSITIVE_TICKS ?
		(double)pos.tic + POSITIVE_TICKS :
		(double)wam->num_tics;

	if(startTic >= stopTic) return;
	glPushMatrix();
	for(col=0;col<wam->num_cols;col++) {
		struct row *r;

		start = startTic;
		stop = stopTic;
		glBindTexture(GL_TEXTURE_2D, row_texes[col]);
		r = wam_row(wam, ac[col].minRow);
		if(r->ticpos > start)
			start = r->ticpos;
		ps = ac[col].ps ? ac[col].ps->data : 0;
		if(ps) {
			if(col == ps->channel && ps->ap.startTic != -1) {
				if(ps->ap.startTic > start)
					start = ps->ap.startTic;
				if(ps->ap.stopTic < stop)
					stop = ps->ap.stopTic;
			}
		}
		if(start >= stop) {
			glTranslated(-BLOCK_WIDTH, 0, 0);
			continue;
		}
		start *= TIC_HEIGHT;
		stop *= TIC_HEIGHT;
		glBegin(GL_QUADS); {
			glTexCoord2f(0.0, start/ 4.0);
			glVertex3f(-1.0, -0.01, start);
			glTexCoord2f(1.0, start / 4.0);
			glVertex3f(1.0, -0.01, start);
			glTexCoord2f(1.0, stop / 4.0);
			glVertex3f(1.0, -0.01, stop);
			glTexCoord2f(0.0, stop / 4.0);
			glVertex3f(-1.0, -0.01, stop);
		} glEnd();

		glTranslated(-BLOCK_WIDTH, 0, 0);
	}
	glPopMatrix();
}
