/*
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
  * Support for plugins.
  *
  * Plugins here are defined as shared libraries that export two symbols:
  * @a init_plugin - defined as int init_plugin(void)
  * @a exit_plugin - defined as void exit_plugin(void)
  *
  * When a plugin is loaded, the init_plugin function is called. When the
  * plugin is freed, exit_plugin is called.
  */

/** Macro to define the @a init_plugin function */
#define plugin_init(initfn) \
	int init_plugin(void) __attribute__((alias(#initfn)))

/** Macro to define the @a exit_plugin function */
#define plugin_exit(exitfn) \
	void exit_plugin(void) __attribute__((alias(#exitfn)))

void *load_plugin(const char *file);
void free_plugin(void *p);
