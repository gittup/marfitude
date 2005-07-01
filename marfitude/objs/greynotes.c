#include <math.h>

#include "SDL_opengl.h"

#include "marfitude.h"

#include "gmae/event.h"
#include "gmae/log.h"
#include "gmae/timer.h"
#include "gmae/wam.h"

#include "util/slist.h"

void notes_init(void) __attribute__ ((constructor));
void notes_exit(void) __attribute__ ((destructor));
static void draw_notes(const void *);

static GLuint noteGl;
static float theta;

void notes_init(void)
{
	register_event("draw opaque", draw_notes, EVENTTYPE_MULTI);

	noteGl = glGenLists(1);
	theta = 0.0;

	glNewList(noteGl, GL_COMPILE); {
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

void notes_exit(void)
{
	glDeleteLists(noteGl, 1);
	deregister_event("draw opaque", draw_notes);
}

void draw_notes(const void *data)
{
	const struct slist *t;
	struct marfitude_pos p;
	GLuint rotnoteList;

	if(data) {}

	glDisable(GL_TEXTURE_2D);
	marfitude_get_pos(&p);

	rotnoteList = glGenLists(1);
	glNewList(rotnoteList, GL_COMPILE); {
		glRotatef(theta, 0.0, 1.0, 0.0);
		glCallList(noteGl);
	} glEndList();

	slist_foreach(t, marfitude_get_notes()) {
		struct marfitude_note *sn = t->data;
		int mat = fabs(sn->time - p.modtime) <= MARFITUDE_TIME_ERROR;

		glPushMatrix();
		glTranslated(   sn->pos.x,
				sn->pos.y,
				sn->pos.z+0.3);
		if(mat)
			glColor4f(1.0, 0.4, 0.4, 1.0);
		else
			glColor4f(0.4, 0.4, 0.4, 1.0);
		Log(("Dn\n"));
		glCallList(rotnoteList);
	}
	glDeleteLists(rotnoteList, 1);
	glEnable(GL_TEXTURE_2D);
	Log(("dn\n"));
	theta += timeDiff * 120.0;
}
