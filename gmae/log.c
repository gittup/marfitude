/*
   Marfitude
   Copyright (C) 2004 Mike Shal

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <stdarg.h>

#include "log.h"

#include "util/memtest.h"

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
#if CONFIG_LOG == 1
	logFile = fopen("log.txt", "w");
	if(!logFile) return 1;
	printf("Logging initialized\n");
#endif
	return 0;
}

void QuitLog(void)
{
#if CONFIG_LOG == 1
	if(logFile) fclose(logFile);
	printf("Log shutdown\n");
#endif
#if CONFIG_MEMTEST == 1
	CheckMemUsage();
#endif
}
