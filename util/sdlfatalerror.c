#include <stdio.h>
#include "SDL.h"

void SDLFatalError(char *file, int line, char *msg)
{
	fprintf(stderr, "SDL error (%s) in %s, line %i: %s\n", msg, file, line, SDL_GetError());
}
