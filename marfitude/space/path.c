#include <stdlib.h>

#include "SDL_opengl.h"

#include "marfitude.h"

#include "gmae/event.h"
#include "gmae/input.h"

static void path_init(void) __attribute__((constructor));
static void path_exit(void) __attribute__((destructor));
static void create_path(const void *);
static void draw_paths(const void *data);

struct line {
	union vector *pts;
	int pts_used;
	int num_pts;
	const float *color;
};

static struct line lines[MAX_PLAYERS];

void path_init(void)
{
	int x;

	for(x=0; x<MAX_PLAYERS; x++)  {
		lines[x].num_pts = 0;
		lines[x].pts_used = 0;
		lines[x].pts = 0;
	}

	register_event("reset AP", create_path);
	register_event("draw transparent", draw_paths);
}

void path_exit(void)
{
	int x;

	deregister_event("draw transparent", draw_paths);
	deregister_event("reset AP", create_path);

	for(x=0; x<MAX_PLAYERS; x++) {
		free(lines[x].pts);
	}
}

void create_path(const void *data)
{
	const struct marfitude_player *p = data;
	const float *pc;
	struct line *l = &lines[p->num];
	int pcount;
	int row;
	int col;

	pc = get_player_color(p->num);
	l->color = pc;
	col = p->channel;

	/* The +1 is because we may add a point at the first row of the AP
	 * if there isn't one there already.
	 */
	if(p->ap.notesTotal + 1 > l->num_pts) {
		l->num_pts = p->ap.notesTotal + 1;
		l->pts = realloc(l->pts, sizeof(union vector) * l->num_pts);
	}

	l->pts_used = p->ap.notesTotal + 1;
	l->pts[0].v[0] = col;
	l->pts[0].v[1] = 0.0;
	l->pts[0].v[2] = p->ap.startTic;
	marfitude_evalvec(&l->pts[0]);

	pcount = 1;
	for(row = p->ap.startRow; row < p->ap.stopRow; row++) {
		int note = marfitude_get_note(row, col);
		if(note) {
			marfitude_get_notepos(&l->pts[pcount], row, col);
			marfitude_evalvec(&l->pts[pcount]);
			pcount++;
			if(row == p->ap.startRow) {
				/* Override the fake point */
				marfitude_get_notepos(&l->pts[0], row, col);
				marfitude_evalvec(&l->pts[0]);
			}
			if(pcount == l->pts_used)
				break;
		}
	}
}

void draw_paths(const void *data)
{
	int x;
	struct line *l;
	const struct marfitude_player *ps;

	if(data) {}

	marfitude_foreach_player(ps) {
		if(ps->ap.active) {
			l = &lines[ps->num];

			glDisable(GL_TEXTURE_2D);
			glBegin(GL_LINE_STRIP); {
				glColor4f(l->color[0], l->color[1], l->color[2], 0.30);
				glVertex3dv(l->pts[0].v);
				glColor4f(l->color[0], l->color[1], l->color[2], 0.75);
				for(x=1; x<l->pts_used; x++) {
					glVertex3dv(l->pts[x].v);
				}
			} glEnd();
			glEnable(GL_TEXTURE_2D);
		}
	}
}
