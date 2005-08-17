#include "SDL_opengl.h"

#include "marfitude.h"
#include "bluenotes.h"

#include "gmae/event.h"
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
		int i;
		int j;
		float mat[16];
		struct marfitude_note *sn = t->data;

		glPushMatrix();
		glTranslated(   sn->pos.x,
				sn->pos.y,
				sn->pos.z+0.3);
		glGetFloatv(GL_MODELVIEW_MATRIX, mat);
		for(i=0;i<3;i++)
			for(j=0;j<3;j++)
			{
				if(i == j) mat[i+j*4] = 1.0;
				else mat[i+j*4] = 0.0;
			}
		glLoadMatrixf(mat);

		if(sn->tic - p.tic <= 0)
			glColor4f(1.0, 1.0, 1.0, 1.0);
		else
			glColor4f(0.5, 0.5, 0.5, 1.0);
		glCallList(particle(P_BlueNova));
	}
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
