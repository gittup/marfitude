#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "fatalerror.h"

void FatalError(const char *file, int line, const char *msg)
{
	fprintf(stderr, "Error %s in %s, line %i: %s\n", msg, file, line, strerror(errno));
}
