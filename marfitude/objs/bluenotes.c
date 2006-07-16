#include "SDL_opengl.h"

#include "marfitude.h"
#include "bluenotes.h"

#include "gmae/event.h"
#include "gmae/glfunc.h"
#include "gmae/textures.h"

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
	struct marfitude_pos pos;

	if(data) {}
	marfitude_get_pos(&pos);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	slist_foreach(t, marfitude_get_hitnotes()) {
		struct marfitude_note *sn = t->data;

		glPushMatrix();
		marfitude_translatev(&sn->pos);
		setup_billboard();

		if(sn->tic - pos.tic <= 0)
			glColor4f(1.0, 1.0, 1.0, 1.0);
		else
			glColor4f(0.5, 0.5, 0.5, 1.0);
		glBindTexture(GL_TEXTURE_2D, texture_num("BlueNova.png"));
		glBegin(GL_QUADS); {
			glTexCoord2f(0.0, 0.0); glVertex3f(-0.5, -0.5, 0.0);
			glTexCoord2f(1.0, 0.0); glVertex3f(0.5, -0.5, 0.0);
			glTexCoord2f(1.0, 1.0); glVertex3f(0.5, 0.5, 0.0);
			glTexCoord2f(0.0, 1.0); glVertex3f(-0.5, 0.5, 0.0);
		} glEnd();

		glPopMatrix();
	}
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
