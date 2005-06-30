#include "SDL_opengl.h"

#include "marfitude.h"

#include "gmae/event.h"
#include "gmae/textures.h"

void __attribute__ ((constructor)) targets_init(void);
void __attribute__ ((destructor)) targets_exit(void);
static void draw_targets(const void *);

static int target_tex;

void targets_init(void)
{
	target_tex = texture_num("Target.png");
	register_event("draw transparent", draw_targets, EVENTTYPE_MULTI);
}

void targets_exit(void)
{
	deregister_event("draw transparent", draw_targets);
}

void draw_targets(const void *data)
{
	int x;
	struct marfitude_pos p;

	if(data) {}
	marfitude_get_pos(&p);

	glPolygonOffset(0.0, 0.0);
	glPushMatrix();
	glTranslated((double)p.channel * -BLOCK_WIDTH, 0.0, TIC_HEIGHT * p.tic);
	glBindTexture(GL_TEXTURE_2D, target_tex);
	glTranslated(-NOTE_WIDTH, 0.0, 0.0);
	glColor4f(1.0, 1.0, 1.0, 0.7);
	glNormal3f(0.0, 1.0, 0.0);
	for(x=-1;x<=1;x++) {
		glBegin(GL_QUADS); {
			glTexCoord2f(0.0, 0.0);
			glVertex3f(-0.25, 0.01, -0.25);
			glTexCoord2f(1.0, 0.0);
			glVertex3f(0.25, 0.01, -0.25);
			glTexCoord2f(1.0, 1.0);
			glVertex3f(0.25, 0.01, 0.25);
			glTexCoord2f(0.0, 1.0);
			glVertex3f(-0.25, 0.01, 0.25);
		} glEnd();
		glTranslated(NOTE_WIDTH, 0.0, 0.0);
	}
	glPopMatrix();
	glPolygonOffset(1.0, 1.0);
}
