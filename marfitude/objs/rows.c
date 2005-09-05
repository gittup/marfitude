#include "SDL_opengl.h"

#include "marfitude.h"
#include "rows.h"

#include "gmae/event.h"
#include "gmae/textures.h"
#include "gmae/wam.h"

#include "util/slist.h"

#define Row(row) (row < 0 ? 0 : (row >= wam->numRows ? wam->numRows - 1: row))

static void draw_rows(const void *data);
static void gen_list(const void *data);

static GLuint row_texes[MAX_COLS];
static GLuint row_list;

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
	register_event("sdl re-init", gen_list);
	gen_list(NULL);
}

void rows_exit(void)
{
	const struct wam *wam = marfitude_get_wam();

	deregister_event("sdl re-init", gen_list);
	deregister_event("draw opaque", draw_rows);
	glDeleteLists(row_list, wam->numCols);
}

void gen_list(const void *data)
{
	int x;
	const struct wam *wam = marfitude_get_wam();

	if(data) {}
	row_list = glGenLists(wam->numCols);
	for(x=0;x<wam->numCols;x++) {
		glNewList(row_list+x, GL_COMPILE); {
			glColor4f(1.0, 1.0, 1.0, 1.0);
			glNormal3f(0.0, 1.0, 0.0);
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
	 * -NEGATIVE_TICKS to wam->numTics.  Of course, if the song is less than
	 * NEGATIVE_TICKS + POSITIVE_TICKS long, some other combinations will
	 * arise :)
	 */
	startTic = pos.tic - NEGATIVE_TICKS >= 0 ?
		((double)pos.tic - NEGATIVE_TICKS) : 0.0;
	stopTic = pos.tic < wam->numTics - POSITIVE_TICKS ?
		(double)pos.tic + POSITIVE_TICKS :
		(double)wam->numTics;

	if(startTic >= stopTic) return;
	glPushMatrix();
	for(col=0;col<wam->numCols;col++) {
		start = startTic;
		stop = stopTic;
		glBindTexture(GL_TEXTURE_2D, row_texes[col]);
		if(wam->rowData[Row(ac[col].minRow)].ticpos > start)
			start = wam->rowData[Row(ac[col].minRow)].ticpos;
		ps = ac[col].ps ? ac[col].ps->data : NULL;
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
