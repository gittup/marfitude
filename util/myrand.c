#include <stdlib.h>

// a random integer
int IntRand(int x)
{
	return (int)((double)x * rand()/(RAND_MAX+1.0));
}
