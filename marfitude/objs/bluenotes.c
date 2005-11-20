#include "SDL_opengl.h"

#include "marfitude.h"
#include "bluenotes.h"

#include "gmae/event.h"
#include "gmae/glfunc.h"
#include "gmae/particles.h"

#include "util/slist.h"

static void draw_notes(const void *);

void bluenotes_init(void)
{
	register_event("draw transparent", draw_notes);
}

void bluenotes_exit(void)
{
	deregister_event("draw transparent", draw_notes);
}

void draw_notes(const void *data)
{
	const struct slist *t;
	struct marfitude_pos p;

	if(data) {}
	marfitude_get_pos(&p);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	slist_foreach(t, marfitude_get_hitnotes()) {
		struct marfitude_note *sn = t->data;

		glPushMatrix();
		glTranslated(   sn->pos.v[0],
				sn->pos.v[1],
				sn->pos.v[2]+0.3);
		setup_billboard();

		if(sn->tic - p.tic <= 0)
			glColor4f(1.0, 1.0, 1.0, 1.0);
		else
			glColor4f(0.5, 0.5, 0.5, 1.0);
		glCallList(particle(P_BlueNova));
	}
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
