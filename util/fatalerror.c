#include <stdio.h>
#include <string.h>
#include <errno.h>

void FatalError(char *file, int line, char *msg)
{
	printf("Error %s in %s, line %i: %s\n", msg, file, line, strerror(errno));
}
