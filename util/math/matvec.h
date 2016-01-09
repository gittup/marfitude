#ifndef matvec_h
#define matvec_h

#include "matrix_t.h"
#include "vector_t.h"

void matrix_colvec_mul(union vector *dest,
		       const union matrix *m,
		       const union vector *v);
void matrix_rowvec_mul(union vector *dest,
		       const union vector *v,
		       const matrix_array m,
		       int mstride);

#endif
