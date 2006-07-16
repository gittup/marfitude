#include <math.h>

#include "SDL_opengl.h"

#include "marfitude.h"
#include "greynotes.h"

#include "gmae/event.h"
#include "gmae/log.h"
#include "gmae/timer.h"
#include "gmae/wam.h"

#include "util/slist.h"

static void draw_notes(const void *);
static void gen_list(const void *);

static GLuint note;
static float theta;

void greynotes_init(void)
{
	register_event("draw opaque", draw_notes);
	register_event("sdl re-init", gen_list);

	gen_list(0);
	theta = 0.0;
}

void greynotes_exit(void)
{
	glDeleteLists(note, 1);
	deregister_event("sdl re-init", gen_list);
	deregister_event("draw opaque", draw_notes);
}

void gen_list(const void *data)
{
	if(data) {}

	note = glGenLists(1);

	glNewList(note, GL_COMPILE); {
		glBegin(GL_TRIANGLE_FAN); {
			glNormal3f( 0.0, 1.0, 0.0);
			glVertex3f( 0.0, 0.15, 0.0);

			glNormal3f( 0.2, 0.5, 0.0);
			glVertex3f( 0.2, 0.0, 0.2);
			glVertex3f( 0.2, 0.0, -0.2);

			glNormal3f( 0.0, 0.5, -0.2);
			glVertex3f( 0.2, 0.0, -0.2);
			glVertex3f(-0.2, 0.0, -0.2);
			
			glNormal3f(-0.2, 0.5, 0.0);
			glVertex3f(-0.2, 0.0, -0.2);
			glVertex3f(-0.2, 0.0, 0.2);

			glNormal3f(0.0, 0.5, 0.2);
			glVertex3f(-0.2, 0.0, 0.2);
			glVertex3f(0.2, 0.0, 0.2);

		} glEnd();
		glPopMatrix();
	} glEndList();
}

void draw_notes(const void *data)
{
	const struct slist *t;
	struct marfitude_pos pos;
	GLuint rotnoteList;

	if(data) {}

	glDisable(GL_TEXTURE_2D);
	marfitude_get_pos(&pos);

	rotnoteList = glGenLists(1);
	glNewList(rotnoteList, GL_COMPILE); {
		glRotatef(theta, 0.0, 1.0, 0.0);
		glCallList(note);
	} glEndList();

	slist_foreach(t, marfitude_get_notes()) {
		int active;
		struct slist *ps;
		struct marfitude_note *sn = t->data;
		const struct wam *wam = marfitude_get_wam();
		const struct marfitude_attack_col *col;
		int mat = fabs(sn->time - pos.modtime) <= MARFITUDE_TIME_ERROR;

		col = &marfitude_get_ac()[sn->col];

		/* Fade the note if a player is playing the column and the note
		 * is not in their AP.
		 */
		ps = col->ps;
		if(ps) {
			struct marfitude_player *player = ps->data;
			active = sn->tic < player->ap.startTic ||
				sn->tic >= player->ap.stopTic;
		} else {
			active = 0;
		}

		/* If the row is being cleared, fade the note then too */
		if(sn->tic < wam_row(wam, col->cleared)->ticpos)
			active = 1;

		glPushMatrix();
		marfitude_translatev(&sn->pos);
		if(mat)
			glColor4f(1.0, 0.4, 0.4, 1.0 - 0.4 * active);
		else
			glColor4f(0.4, 0.4, 0.4, 1.0 - 0.4 * active);
		Log(("Dn\n"));
		glCallList(rotnoteList);
	}
	glDeleteLists(rotnoteList, 1);
	glEnable(GL_TEXTURE_2D);
	Log(("dn\n"));
	theta += timeDiff * 120.0;
}
