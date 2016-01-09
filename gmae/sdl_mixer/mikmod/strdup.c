#include <stdlib.h>
#include <string.h>

#include "mikmod_internals.h"

char *Mstrdup(const char *str)
{
	char *newstr;
	
	newstr = (char *)malloc(strlen(str)+1);
	if ( newstr != NULL ) {
		strcpy(newstr, str);
	}
	return(newstr);
}
