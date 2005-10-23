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
static void create_row4(unsigned char *p, int x, int y);
static void create_row5(unsigned char *p, int x, int y);
static void create_row6(unsigned char *p, int x, int y);

static int row_texes[MAX_COLS];
static const int width = 128;
static const int height = 128;

void rows_init(void)
{
	create_texture(&row_texes[0], width, height, create_row0);
	create_texture(&row_texes[1], width, height, create_row1);
	create_texture(&row_texes[2], width, height, create_row2);
	create_texture(&row_texes[3], width, height, create_row3);
	create_texture(&row_texes[4], width, height, create_row4);
	create_texture(&row_texes[5], width, height, create_row5);
	create_texture(&row_texes[6], width, height, create_row6);

	register_event("draw opaque", draw_rows);
}

void rows_exit(void)
{
	deregister_event("draw opaque", draw_rows);
	delete_texture(&row_texes[6]);
	delete_texture(&row_texes[5]);
	delete_texture(&row_texes[4]);
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

static double eq0(double x);
static double eq1(double x);
static double eq2(double x);
static double eq3(double x);
static double eq4(double x);
static double eq5(double x);
static double eq6(double x);
static double f(double x, double y, double (eq)(double));
static double c(double x, double t);
static double s(double x, double t);

double eq0(double x)
{
	return (x > width / 2) ?
		(width - x) * c(x, 5.0) / (double)width :
		(x) * c(x, 5.0) / (double)width;
}

double eq1(double x)
{
	return (s(x, 6.0) + 0.5 * s(x, 12.0) + s(x, 4.5)) / 2.5;
}

double eq2(double x)
{
	return (s(x, 4.0) + s(x, 6.0)) / 2.0;
}

double eq3(double x)
{
	return (x > width / 2) ?
		(x - width / 2) * s(x, 6.0) / (double)width :
		(width / 2 - x) * s(x, 6.0) / (double)width;
}

double eq4(double x)
{
	return (x > width / 2) ?
		(width - x) * s(x, 3.0) / (double)width :
		(x) * s(x, 7.0) / (double)width;
}

double eq5(double x)
{
	return (s(x, 1.0) + s(x, 2.0) + s(x, 4.0)) / 3.0;
}

double eq6(double x)
{
	return (0.5 * s(x, 2.0) + s(x, 3.0) - s(x, 4.0)) / 2.5;
}

double f(double x, double y, double (eq)(double))
{
	double t;
	double u;
	double v;

	t = (eq(y) + 1) * height/2 - x;
	u = (eq(y) + 1) * height/2 + height - x;
	t = sqrt(t*t);
	u = sqrt(u*u);
	if(t < u)
		v = t;
	else
		v = u;
	u = (eq(y) + 1) * height/2 - height - x;
	u = sqrt(u*u);
	if(u < v)
		v = u;

	return v;
}

double c(double x, double t)
{
	return cos(t * x * pi / (double)width);
}

double s(double x, double t)
{
	return sin(t * x * pi / (double)width);
}

void create_row0(unsigned char *p, int x, int y)
{
	p[0] = 0;
	p[1] = 0;
	p[2] = 168 - (f(x, y, eq0) * 128 / height);
	p[3] = 255;
}

void create_row1(unsigned char *p, int x, int y)
{
	p[0] = 168 - (f(x, y, eq1) * 128 / height);
	p[1] = 0;
	p[2] = 0;
	p[3] = 255;
}

void create_row2(unsigned char *p, int x, int y)
{
	p[0] = 128 - (f(x, y, eq2) * 58 / height);
	p[1] = p[0];
	p[2] = 0;
	p[3] = 255;
}

void create_row3(unsigned char *p, int x, int y)
{
	p[0] = 0;
	p[1] = 128 - (f(x, y, eq3) * 128 / height);
	p[2] = 0;
	p[3] = 255;
}

void create_row4(unsigned char *p, int x, int y)
{
	p[0] = 128 - (f(x, y, eq4) * 128 / height);
	p[1] = 0;
	p[2] = p[0];
	p[3] = 255;
}

void create_row5(unsigned char *p, int x, int y)
{
	p[0] = 0;
	p[1] = 128 - (f(x, y, eq5) * 78 / height);
	p[2] = p[1];
	p[3] = 255;
}

void create_row6(unsigned char *p, int x, int y)
{
	p[0] = 98 - (f(x, y, eq6) * 58 / height);
	p[1] = p[0];
	p[2] = p[0];
	p[3] = 255;
}
