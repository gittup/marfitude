#ifndef vector_t_h
#define vector_t_h

union vector {
	double v[4];
	struct {
		double x;
		double y;
		double z;
		double w;
	} p;
};

#endif
