#include <stdlib.h>

#include "SDL_opengl.h"

#include "marfitude.h"

#include "gmae/event.h"
#include "gmae/phys.h"
#include "gmae/wam.h"

#include "util/plugin.h"

/* temporry */
extern int channelFocus;
extern int curTic;
extern double partialTic;
extern struct wam *wam;
/* end temporry */

static int lines_init(void);
static void lines_exit(void);
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
static int startLine;
static int stopLine;
static int numLines;

int lines_init(void)
{
	numLines = NUM_TICKS;
	lines = (struct line*)malloc(sizeof(struct line) * numLines);
	startLine = 0;
	stopLine = 0;

	RegisterEvent("draw opaque", draw_lines, EVENTTYPE_MULTI);
	RegisterEvent("row", create_line, EVENTTYPE_MULTI);
	RegisterEvent("de-row", remove_line, EVENTTYPE_MULTI);
	return 0;
}

void lines_exit(void)
{
	DeregisterEvent("de-row", remove_line);
	DeregisterEvent("row", create_line);
	DeregisterEvent("draw opaque", draw_lines);
	free(lines);
}

void create_line(const void *data)
{
	const struct row *r= data;

	if(r->line) {
		lines[stopLine].p1.x = 1.0;
		lines[stopLine].p1.y = 0.005;
		lines[stopLine].p1.z = TIC_HEIGHT * (double)r->ticpos;
		lines[stopLine].p2.x = 1.0 - 2.0 * wam->numCols;
		lines[stopLine].p2.y = 0.005;
		lines[stopLine].p2.z = TIC_HEIGHT * (double)r->ticpos;
		lines[stopLine].row = r;
		lines[stopLine].color = r->line;
		stopLine++;
		if(stopLine == numLines) stopLine = 0;
	}
}

void remove_line(const void *data)
{
	const struct row *r = data;

	while(lines[startLine].row == r) {
		lines[startLine].row = NULL;
		startLine++;
		if(startLine == numLines) startLine = 0;
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
	glDisable(GL_LIGHTING);
	glNormal3f(0.0, 1.0, 0.0);
	glBegin(GL_LINES);
	if(startLine <= stopLine) {
		for(x=startLine;x<stopLine;x++)
			draw_line(x);
	}
	else {
		for(x=startLine;x<numLines;x++)
			draw_line(x);
		for(x=0;x<stopLine;x++)
			draw_line(x);
	}
	glEnd();
	glEnable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
}

plugin_init(lines_init);
plugin_exit(lines_exit);
