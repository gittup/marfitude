#include <stdlib.h>

#include "SDL_opengl.h"

#include "marfitude.h"
#include "lines.h"

#include "gmae/event.h"
#include "gmae/wam.h"

static void create_line(const void *);
static void remove_line(const void *);
static void draw_lines(const void *);
static void draw_line(int l);

/** Some data for a line that goes across the row */
struct line {
	struct vector p1;       /**< One end of the line */
	struct vector p2;       /**< The other end of the line */
	int color;              /**< 2 for blue, one for gray, of course! */
	const struct row *row;  /**< What row this line is on */
};

static struct line *lines;
static int start_line;
static int stop_line;
static int num_lines;

void lines_init(void)
{
	num_lines = NUM_TICKS;
	lines = malloc(sizeof(struct line) * num_lines);
	start_line = 0;
	stop_line = 0;

	register_event("draw transparent", draw_lines);
	register_event("row", create_line);
	register_event("de-row", remove_line);
}

void lines_exit(void)
{
	deregister_event("de-row", remove_line);
	deregister_event("row", create_line);
	deregister_event("draw transparent", draw_lines);
	free(lines);
}

void create_line(const void *data)
{
	const struct row *r= data;
	const struct wam *wam = marfitude_get_wam();

	if(r->line) {
		lines[stop_line].p1.x = 1.0;
		lines[stop_line].p1.y = 0.005;
		lines[stop_line].p1.z = TIC_HEIGHT * (double)r->ticpos;
		lines[stop_line].p2.x = 1.0 - 2.0 * wam->numCols;
		lines[stop_line].p2.y = 0.005;
		lines[stop_line].p2.z = TIC_HEIGHT * (double)r->ticpos;
		lines[stop_line].row = r;
		lines[stop_line].color = r->line;
		stop_line++;
		if(stop_line == num_lines) stop_line = 0;
	}
}

void remove_line(const void *data)
{
	const struct row *r = data;

	while(lines[start_line].row == r) {
		lines[start_line].row = NULL;
		start_line++;
		if(start_line == num_lines) start_line = 0;
	}
}

void draw_line(int l)
{
	if(lines[l].color == 2) glColor4f(0.0, 0.0, 1.0, .7);
	else glColor4f(0.8, 0.8, 0.8, .3);
	glVertex3f( lines[l].p1.x, lines[l].p1.y, lines[l].p1.z);
	glVertex3f( lines[l].p2.x, lines[l].p2.y, lines[l].p2.z);
}

void draw_lines(const void *data)
{
	int x;
	if(data) {}

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glNormal3f(0.0, 1.0, 0.0);
	glBegin(GL_LINES);
	if(start_line <= stop_line) {
		for(x=start_line;x<stop_line;x++)
			draw_line(x);
	}
	else {
		for(x=start_line;x<num_lines;x++)
			draw_line(x);
		for(x=0;x<stop_line;x++)
			draw_line(x);
	}
	glEnd();
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
}
