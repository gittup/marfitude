#include <stdio.h>
#include <stdarg.h>

#include "log.h"

#include "memtest.h"

FILE *logFile = NULL;

void LogFile(const char *file, int line)
{
	if(logFile)
	{
		fprintf(logFile, "%s line %i ", file, line);
	}
}

void LogMsg(const char *s, ...)
{
	va_list ap;
	if(logFile)
	{
		va_start(ap, s);
		vfprintf(logFile, s, ap);
		fflush(logFile);
		va_end(ap);
	}
}

void ELogFile(const char *file, int line)
{
	LogFile(file, line);
	fprintf(stderr, "%s line %i: ", file, line);
}

void ELogMsg(const char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	vfprintf(stderr, s, ap);
	va_end(ap);
}

int InitLog(void)
{
#ifdef LOG
	logFile = fopen("log.txt", "w");
	if(!logFile) return 0;
	printf("Logging initialized\n");
#endif
	return 1;
}

void QuitLog(void)
{
#ifdef LOG
	if(logFile) fclose(logFile);
	printf("Log shutdown\n");
#endif
#ifdef DEBUG_MEM
	CheckMemUsage();
#endif
}
