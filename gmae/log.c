#include <stdio.h>
#include <stdarg.h>

#include "../util/memtest.h"

FILE *logFile = NULL;

void LogWrite(char *file, int line, char *s, ...)
{
	va_list ap;
	if(logFile)
	{
		fprintf(logFile, "%s line %i: ", file, line);
		va_start(ap, s);
		vfprintf(logFile, s, ap);
		fflush(logFile);
		va_end(ap);
	}
}

void ELogWrite(char *file, int line, char *s, ...)
{
	va_list ap;
	if(logFile)
	{
		fprintf(logFile, "%s line %i: ", file, line);
		va_start(ap, s);
		vfprintf(logFile, s, ap);
		fflush(logFile);
		va_end(ap);
	}
	fprintf(stderr, "%s line %i: ", file, line);
	va_start(ap, s);
	vfprintf(stderr, s, ap);
	va_end(ap);
}

int InitLog()
{
#ifdef LOG
	logFile = fopen("log.txt", "w");
	if(!logFile) return 0;
	printf("Logging initialized\n");
#endif
	return 1;
}

void QuitLog()
{
#ifdef LOG
	if(logFile) fclose(logFile);
	printf("Log shutdown\n");
#endif
#ifdef DEBUG_MEM
	CheckMemUsage();
#endif
}
