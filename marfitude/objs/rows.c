#include "SDL_opengl.h"

#include "marfitude.h"
#include "rows.h"

#include "gmae/event.h"
#include "gmae/textures.h"
#include "gmae/timer.h"
#include "gmae/wam.h"

#define Row(row) (row < 0 ? 0 : (row >= wam->numRows ? wam->numRows - 1: row))

static void draw_rows(const void *data);

static GLuint row_texes[MAX_COLS];
static GLuint row_list;

void rows_init(void)
{
	int x;
	const struct wam *wam = marfitude_get_wam();

	row_texes[0] = texture_num("Slate.png");
	row_texes[1] = texture_num("Walnut.png");
	row_texes[2] = texture_num("ElectricBlue.png");
	row_texes[3] = texture_num("Clovers.png");
	row_texes[4] = texture_num("Lava.png");
	row_texes[5] = texture_num("Parque3.png");
	row_texes[6] = texture_num("Slate.png");
	row_texes[7] = texture_num("ElectricBlue.png");

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
	register_event("draw opaque", draw_rows, EVENTTYPE_MULTI);
}

void rows_exit(void)
{
	const struct wam *wam = marfitude_get_wam();

	deregister_event("draw opaque", draw_rows);
	glDeleteLists(row_list, wam->numCols);
}

void draw_rows(const void *data)
{
	int col;
	double start, stop;
	double startTic, stopTic;
	struct marfitude_pos p;
	const struct wam *wam = marfitude_get_wam();
	const struct marfitude_attack_col *ac = marfitude_get_ac();
	const struct marfitude_attack_pat *ap = marfitude_get_ap();

	if(data) {}
	marfitude_get_pos(&p);
	glColor4f(1.0, 1.0, 1.0, 1.0);

	/* usually we draw from -NEGATIVE_TICKS to +POSITIVE_TICKS, with the
	 * notes currently being played at position 0.
	 * At the beginning of the song, we start drawing instead from 
	 * 0 to +POSITIVE_TICKS, and at the end of the song we draw from
	 * -NEGATIVE_TICKS to wam->numTics.  Of course, if the song is less than
	 * NEGATIVE_TICKS + POSITIVE_TICKS long, some other combinations will
	 * arise :)
	 */
	startTic = p.tic - NEGATIVE_TICKS >= 0 ?
		((double)p.tic - NEGATIVE_TICKS) : 0.0;
	stopTic = p.tic < wam->numTics - POSITIVE_TICKS ?
		(double)p.tic + POSITIVE_TICKS :
		(double)wam->numTics;

	if(startTic >= stopTic) return;
	glPushMatrix();
	glDisable(GL_LIGHTING);
	for(col=0;col<wam->numCols;col++) {
		start = startTic;
		stop = stopTic;
		glBindTexture(GL_TEXTURE_2D, row_texes[col]);
		if(wam->rowData[Row(ac[col].minRow)].ticpos > start)
			start = wam->rowData[Row(ac[col].minRow)].ticpos;
		if(col == p.channel && ap->startTic != -1) {
			if(ap->startTic > start) start = ap->startTic;
			if(ap->stopTic < stop) stop = ap->stopTic;
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