#include "SDL_opengl.h"

#include "gmae/event.h"
#include "gmae/fft.h"
#include "gmae/glfunc.h"

void __attribute__ ((constructor)) fft_curtain_init(void);
void __attribute__ ((destructor)) fft_curtain_exit(void);
static void fft_curtain_draw(const void *data);

void fft_curtain_init(void)
{
	init_fft();
	register_event("draw ortho", fft_curtain_draw, EVENTTYPE_MULTI);
}

void fft_curtain_exit(void)
{
	deregister_event("draw ortho", fft_curtain_draw);
	free_fft();
}

void fft_curtain_draw(const void *data)
{
	const struct fft_data *fft = fft_get_data();
	unsigned int x;
	double width;
	double divisor = .0058 * fft->max;
	int tmp_width;

	if(data) {}

	glDisable(GL_TEXTURE_2D);
	tmp_width = display_width(); /* stupid cast warning! */
	width = (double)tmp_width / (double)fft->len;
	for(x=0;x<fft->len;x++) {
		int fft_col;
		double red, green;

		fft_col = (display_height() * fft->data[x])/divisor;
		glNormal3f(0.0, 0.0, 1.0);
		red = (double)fft_col / display_height();
		green = 1.0 - red;
		if(red>1.0)
			red = 1.0;
		if(green<0.0)
			green = 0.0;
		glBegin(GL_QUADS); {
			glColor4f(0.0, 1.0, 0.0, 0.5);
			glVertex3f(x*width, 0, -1.0);
			glVertex3f((x+1) * width, 0, -1.0);
			glColor4f(red, green, 0.0, 0.5);
			glVertex3f((x+1) * width, (double)fft_col, -1.0);
			glVertex3f(x * width, (double)fft_col, -1.0);
		} glEnd();
	}
	glEnable(GL_TEXTURE_2D);
}
