#include <math.h>

#include "SDL_opengl.h"

#include "marfitude.h"
#include "planks.h"

#include "objs/bluenotes.h"
#include "objs/greynotes.h"
#include "objs/targets.h"
#include "objs/view.h"

#include "util/math/matrix.h"
#include "util/math/vector.h"

static void coaster_init(void) __attribute__ ((constructor));
static void coaster_exit(void) __attribute__ ((destructor));
static void coaster_eval3d(double *x, double *y, double *z);
static void coaster_dv(union vector *dv, const union vector *v, double dt);
static void coaster_translate3d(double x, double y, double z);

static void rprime(union vector *v, const union vector *p);
static void rdoubleprime(union vector *v, const union vector *p);

#define HILL_HEIGHT 10.0

void coaster_init(void)
{
	marfitude_eval3d = coaster_eval3d;
	marfitude_dv = coaster_dv;
	marfitude_translate3d = coaster_translate3d;

	greynotes_init();
	bluenotes_init();
	targets_init();
	view_init();
	planks_init();
}

void coaster_exit(void)
{
	planks_exit();
	view_exit();
	targets_exit();
	bluenotes_exit();
	greynotes_exit();
}

void coaster_eval3d(double *x, double *y, double *z)
{
	if(*z < 0.0) {
		*x *= -BLOCK_WIDTH;
		*y += HILL_HEIGHT;
		*z *= TIC_HEIGHT;
	} else {
		*x *= -BLOCK_WIDTH;
		*y += HILL_HEIGHT * cos(*z / 58.0);
		*z *= TIC_HEIGHT;
	}
}

void coaster_dv(union vector *dv, const union vector *v, double dt)
{
	if(dt) {}
	if(v->v[2] < 0.0) {
		dv->v[0] = 0.0;
		dv->v[1] = 0.0;
		dv->v[2] = 1.0;
	} else {
		dv->v[0] = 0.0;
		dv->v[1] = -HILL_HEIGHT / 58.0 * sin(v->v[2] / 58.0);
		dv->v[2] = 1.0;
		vector_normalize(dv);
	}
}

void coaster_translate3d(double x, double y, double z)
{
	union matrix mat;
	union vector B;
	union vector N;
	union vector T;
	union vector P;

	P.v[0] = x;
	P.v[1] = y;
	P.v[2] = z;
	rprime(&T, &P);

	N.v[0] = 0.0;
	N.v[1] = 1.0;
	N.v[2] = 0.0;

	vector_normalize(&T);
	vector_cross(&B, &T, &N);

	coaster_eval3d(&x, &y, &z);

	mat.v[0]  = -B.p.x;
	mat.v[1]  = -B.p.y;
	mat.v[2]  = -B.p.z;
	mat.v[3]  = 0;
	mat.v[4]  = N.p.x;
	mat.v[5]  = N.p.y;
	mat.v[6]  = N.p.z;
	mat.v[7]  = 0;
	mat.v[8]  = T.p.x;
	mat.v[9]  = T.p.y;
	mat.v[10] = T.p.z;
	mat.v[11] = 0;
	mat.v[12] = x;
	mat.v[13] = y;
	mat.v[14] = z;
	mat.v[15] = 1;

	glMultMatrixd(mat.v);
}

void rprime(union vector *v, const union vector *p)
{
	double x = 0.05;
	union vector u1;
	union vector u2;

	u1.v[0] = p->v[0];
	u1.v[1] = p->v[1];
	u1.v[2] = p->v[2] - x;
	u2.v[0] = p->v[0];
	u2.v[1] = p->v[1];
	u2.v[2] = p->v[2] + x;
	marfitude_evalv(&u1);
	marfitude_evalv(&u2);
	x *= 2.0;
	v->v[0] = (u2.v[0] - u1.v[0]) / x;
	v->v[1] = (u2.v[1] - u1.v[1]) / x;
	v->v[2] = (u2.v[2] - u1.v[2]) / x;
}

void rdoubleprime(union vector *v, const union vector *p)
{
	double x = 0.05;
	union vector u1;
	union vector u2;
	union vector ptmp;

	ptmp.v[0] = p->v[0];
	ptmp.v[1] = p->v[1];
	ptmp.v[2] = p->v[2] + x;
	rprime(&u1, &ptmp);
	ptmp.v[2] = p->v[2] - x;
	rprime(&u2, &ptmp);
	x *= 2.0;
	v->v[0] = (u2.v[0] - u1.v[0]) / x;
	v->v[1] = (u2.v[1] - u1.v[1]) / x;
	v->v[2] = (u2.v[2] - u1.v[2]) / x;
}
