#include <stdio.h>
#include "SDL.h"

#include "sdlfatalerror.h"

void SDLFatalError(const char *file, int line, const char *msg)
{
	fprintf(stderr, "SDL error (%s) in %s, line %i: %s\n", msg, file, line, SDL_GetError());
}
