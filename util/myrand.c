#include <stdlib.h>

// a random integer
int IntRand(int x)
{
	return (int)((double)x * rand()/(RAND_MAX+1.0));
}

// return a float from 0.0 to 1.0
float FloatRand()
{
	return (float)rand() / (float)(RAND_MAX+1.0);
}
