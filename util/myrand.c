#include <stdlib.h>

#include "myrand.h"

/* a random integer */
int IntRand(int x)
{
	return (int)((double)x * rand()/(RAND_MAX+1.0));
}

/* return a float from 0.0 to 1.0 */
float FloatRand(void)
{
	float x = rand();
	return x / (float)(RAND_MAX+1.0);
}
