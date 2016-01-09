#ifndef matrix_t_h
#define matrix_t_h

union matrix {
	double v[16];
	double p[4][4];
};

typedef double matrix_array[];

#endif
