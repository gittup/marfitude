/*
   Marfitude
   Copyright (C) 2006 Mike Shal

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

/** @file
 * Log messages to the screen and/or a file.
 */

#include <stdio.h>
#include <stdarg.h>

#include "log.h"

#include "util/memtest.h"

static int logging = 0;
static FILE *logFile = NULL;

/** The internal log function, called by the Log macro
 * @param file The file that generated the log message
 * @param line The line number in the file
 */
void log_file(const char *file, int line)
{
	if(logFile)
		fprintf(logFile, "%s line %i ", file, line);
}

/** The internal log function, called by the Log macro. This piece actually
 * writes the log message.
 * @param s The log message
 */
void log_msg(const char *s, ...)
{
	va_list ap;
	if(logFile) {
		va_start(ap, s);
		vfprintf(logFile, s, ap);
		fflush(logFile);
		va_end(ap);
	}
}

/** The internal error log function, called by the ELog macro.
 * @param file The file that generated the log message
 * @param line The line number in the file
 */
void elog_file(const char *file, int line)
{
	log_file(file, line);
	fprintf(stderr, "%s line %i: ", file, line);
}

/** The internal error log function, called by the ELog macro.
 * @param s The log message
 */
void elog_msg(const char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	vfprintf(stderr, s, ap);
	va_end(ap);
}

/** Opens the log file if logging has been requested on the command line.
 * @return 0 on success, 1 if the log could not be opened
 */
int init_log(void)
{
	if(logging) {
		logFile = fopen("log.txt", "w");
		if(!logFile) return 1;
		printf("Logging initialized\n");
	}
	return 0;
}

/** Closes the log file. Also checks for memory usage if CONFIG_MEMTEST
 * has been enabled. */
void quit_log(void)
{
	if(logging) {
		if(logFile) fclose(logFile);
		printf("Log shutdown\n");
	}
#if CONFIG_MEMTEST == 1
	check_mem_usage();
#endif
}

/** Determines whether or not logging is enabled.
 * @return 1 if logging is enabled, 0 if not
 */
int log_enabled(void)
{
	return logging;
}

/** Enables logging */
void enable_logging(void)
{
	logging = 1;
}

/** Disables logging */
void disable_logging(void)
{
	logging = 0;
}
