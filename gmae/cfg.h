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
 * Has some functions to query and set configuration options
 */

void cfg_set(const char *header, const char *option, const char *value);
void cfg_set_int(const char *header, const char *option, int value);
char *cfg_copy(const char *header, const char *option, const char *unset);
const char *cfg_get(const char *header, const char *option, const char *unset);
int cfg_get_int(const char *header, const char *option, int unset);
int cfg_eq(const char *header, const char *option, const char *string);
int init_config(void);
void quit_config(void);
