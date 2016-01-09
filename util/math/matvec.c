#include "matvec.h"
#include "matvec.h"

void matrix_colvec_mul(union vector *dest,
		       const union matrix *m,
		       const union vector *v)
{
	dest->v[0] =
		m->v[0] * v->v[0] + m->v[1] * v->v[1] +
		m->v[2] * v->v[2] + m->v[3] * v->v[3];
	dest->v[1] =
		m->v[4] * v->v[0] + m->v[5] * v->v[1] +
		m->v[6] * v->v[2] + m->v[7] * v->v[3];
	dest->v[2] =
		m->v[8] *  v->v[0] + m->v[9] * v->v[1] +
		m->v[10] * v->v[2] + m->v[11] * v->v[3];
	dest->v[3] =
		m->v[12] * v->v[0] + m->v[13] * v->v[1] +
		m->v[14] * v->v[2] + m->v[15] * v->v[3];
}

void matrix_rowvec_mul(union vector *dest,
		       const union vector *v,
		       const matrix_array m,
		       int mstride)
{
	dest->v[0] =
		m[0] * v->v[0] + m[mstride] * v->v[1] +
		m[mstride*2] * v->v[2] + m[mstride * 3] * v->v[3];
	dest->v[1] =
		m[1] * v->v[0] + m[mstride+1] * v->v[1] +
		m[mstride*2 + 1] * v->v[2] + m[mstride * 3 + 1] * v->v[3];
	dest->v[2] =
		m[2] * v->v[0] + m[mstride+2] * v->v[1] +
		m[mstride*2 + 2] * v->v[2] + m[mstride * 3 + 2] * v->v[3];
	dest->v[3] =
		m[3] * v->v[0] + m[mstride+3] * v->v[1] +
		m[mstride*2 + 3] * v->v[2] + m[mstride * 3 + 3] * v->v[3];
}
