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
 * Get access to a MikMod module
 */

#ifndef _MIKMOD_H_
#include "gmae/sdl_mixer/mikmod/mikmod.h"
#endif

int StartModule(const char *modFile);
void StopModule(void);

extern MODULE *mod;

/** Converts the sngspd @a s and bpm @a b parameters to a time in seconds */
#define BpmToSec(s, b) (2.5 * (double)s / (double)b)
