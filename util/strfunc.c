#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include "memtest.h"

#include "strfunc.h"

int IntLen(int x)
{
	if(!x) return 1;
	if(x < 0) return IntLen(-x)+1;
	return floor(log10((double)x))+1;
}

int StrEq(const char *a, const char *b)
{
	while(*a && *b)
	{
		if(*a != *b) return 0;
		a++;
		b++;
	}
	if(!*a && !*b) return 1;
	return 0;
}
