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

#ifdef LOG
#define Log(s) do {LogFile(__FILE__, __LINE__); LogMsg s;} while(0)
#define ELog(s) do {ELogFile(__FILE__, __LINE__); ELogMsg s; LogMsg s;} while(0)
#else
#define Log(s)
#define ELog(s)
#endif

void LogFile(const char *file, int line);
void LogMsg(const char *s, ...);
void ELogFile(const char *file, int line);
void ELogMsg(const char *s, ...);
void QuitLog(void);
int InitLog(void);
