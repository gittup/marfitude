#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "memtest.h"

char *MallocString(char *format, ...)
{
	char *s = NULL;
	int len;
	va_list ap;
	va_start(ap, format);
	len = vsnprintf(s, 0, format, ap);
	va_end(ap);
	s = (char*)malloc(sizeof(char) * (len+1));
	vsprintf(s, format, ap);
	return s;
}
