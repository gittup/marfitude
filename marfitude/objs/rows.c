#include <math.h>

#include "SDL_opengl.h"

#include "marfitude.h"
#include "rows.h"

#include "gmae/event.h"
#include "gmae/textures.h"
#include "gmae/wam.h"

#include "util/slist.h"
#include "util/math/pi.h"

static void draw_rows(const void *data);
static void create_row0(unsigned char *p, int x, int y);
static void create_row1(unsigned char *p, int x, int y);
static void create_row2(unsigned char *p, int x, int y);
static void create_row3(unsigned char *p, int x, int y);

static int row_texes[MAX_COLS];
static const int width = 128;
static const int height = 128;

void rows_init(void)
{
	create_texture("row 0", &row_texes[0], width, height, create_row0);
	create_texture("row 1", &row_texes[1], width, height, create_row1);
	create_texture("row 2", &row_texes[2], width, height, create_row2);
	create_texture("row 3", &row_texes[3], width, height, create_row3);
	row_texes[4] = texture_num("Walnut.png");
	row_texes[5] = texture_num("Parque3.png");
	row_texes[6] = texture_num("Slate.png");

	register_event("draw opaque", draw_rows);
}

void rows_exit(void)
{
	deregister_event("draw opaque", draw_rows);
	delete_texture(&row_texes[3]);
	delete_texture(&row_texes[2]);
	delete_texture(&row_texes[1]);
	delete_texture(&row_texes[0]);
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
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(5.0, 0.0);

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
	glPopMatrix();
	glDisable(GL_POLYGON_OFFSET_FILL);
}

static double eq1(double x);
static double eq2(double x);
static double eq3(double x, double y);
static double eq4(double x);

double eq1(double x)
{
	return sin(2.0 * x * pi / (double)width);
}

double eq2(double x)
{
	return sin(2.0 * x * pi / (double)width) + sin(4.0 * x * pi / (double)width);
}

double eq3(double x, double y)
{
	return sin(2.0 * x * pi / (double)width) + cos(2.0 * y * pi / (double)width);
}

double eq4(double x)
{
	return (x > width / 2) ?
		(x - width / 2) * sin(6.0 * x * pi / (double)width) / width :
		(width / 2 - x) * sin(6.0 * x * pi / (double)width) / width;
}

void create_row0(unsigned char *p, int x, int y)
{
	double t;
	double u;
	double v;

	t = (eq1(y) + 1) * height/2 - x;
	u = (eq1(y) + 1) * height/2 + height - x;
	t = sqrt(t*t);
	u = sqrt(u*u);
	if(t < u)
		v = t;
	else
		v = u;
	u = (eq1(y) + 1) * height/2 - height - x;
	u = sqrt(u*u);
	if(u < v)
		v = u;

	p[0] = 0;
	p[1] = 0;
	p[2] = 168 - (v * 128 / height);
	p[3] = 255;
}

void create_row1(unsigned char *p, int x, int y)
{
	double t;
	double u;
	double v;

	t = (eq2(x) + 1) * height/2 - y;
	u = (eq2(x) + 1) * height/2 + height - y;
	t = sqrt(t*t);
	u = sqrt(u*u);
	if(t < u)
		v = t;
	else
		v = u;
	u = (eq2(x) + 1) * height/2 - height - y;
	u = sqrt(u*u);
	if(u < v)
		v = u;

	p[0] = 168 - (v * 128 / height);
	p[1] = 0;
	p[2] = 0;
	p[3] = 255;
}

void create_row2(unsigned char *p, int x, int y)
{
	double t;
	double u;
	double v;

	t = (eq3(x, y) + 1) * height/2 - y;
	u = (eq3(x, y) + 1) * height/2 + height - y;
	t = sqrt(t*t);
	u = sqrt(u*u);
	if(t < u)
		v = t;
	else
		v = u;
	u = (eq3(x, y) + 1) * height/2 - height - y;
	u = sqrt(u*u);
	if(u < v)
		v = u;

	p[0] = 128 - (v * 255 / height) / 3;
	p[1] = p[0];
	p[2] = 0;
	p[3] = 255;
	p += 4;
}

void create_row3(unsigned char *p, int x, int y)
{
	double t;
	double u;
	double v;

	t = (eq4(y) + 1) * height/2 - x;
	u = (eq4(y) + 1) * height/2 + height - x;
	t = sqrt(t*t);
	u = sqrt(u*u);
	if(t < u)
		v = t;
	else
		v = u;
	u = (eq4(y) + 1) * height/2 - height - x;
	u = sqrt(u*u);
	if(u < v)
		v = u;

	p[0] = 0;
	p[1] = 128 - (v * 255 / height) / 2;
	p[2] = 0;
	p[3] = 255;
	p += 4;
}
