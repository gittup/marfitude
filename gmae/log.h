/*
   Marfitude
   Copyright (C) 2005 Mike Shal

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
 * Provides some macros that can log messages. The logged messages
 * contain the file and line number of where the message was logged from.
 */

/** Defined as 1 if logging is enabled. Set by command line options */
extern int logging;

/** Write the logged message to a file, if logging is enabled */
#define Log(s) do {if(logging) {log_file(__FILE__, __LINE__); log_msg s;}} while(0)
/** Write the logged message stderr. Also write to the log if enabled */
#define ELog(s) do {elog_file(__FILE__, __LINE__); elog_msg s; if(logging) {log_msg s;}} while(0)

void log_file(const char *file, int line);
void log_msg(const char *s, ...);
void elog_file(const char *file, int line);
void elog_msg(const char *s, ...);
void quit_log(void);
int init_log(void);
