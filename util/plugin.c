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

#include <stdio.h>
#include <stdlib.h>

#include "plugin.h"
#include "dl.h"

/** @file
 * Implements loading and freeing plugins
 */

/** This structure contains the init/exit functions of a plugin. Also contains
 * the reference to the underlying dynamic library.
 */
struct plugin {
	int (*init)(void);  /**< Initializes the plugin. Returns 0 on success */
	void (*exit)(void); /**< The function called on unload */
	void *handle;       /**< Dynamic library handle */
};

/** Returns a plugin handle for the plugin @a file */
void *load_plugin(const char *file)
{
	void *handle;
	struct plugin *p = NULL;

	dlerror();

	handle = dlopen(file, RTLD_NOW);
	if(handle != NULL) {
		int (*initf)(void);
		void (*exitf)(void);

		initf = (int(*)(void))dlsym(handle, "init_plugin");
		exitf = (void(*)(void))dlsym(handle, "exit_plugin");
		if(initf != NULL && exitf != NULL) {
			int ret;
			p = malloc(sizeof(struct plugin));
			p->handle = handle;
			p->init = initf;
			p->exit = exitf;

			ret = p->init();
			if(ret != 0) {
				printf("Error: Plugin returned %i on initialize.\n", ret);
			}
		} else {
			printf("Error: Couldn't load init_plugin and exit_plugin symbols from %s\n", file);
		}
	} else {
		printf("%s\n", dlerror());
		printf("Error: Couldn't load plugin: %s\n", file);
	}
	return p;
}

/** Frees the plugin handle that was returned by load_plugin */
void free_plugin(void *plugin)
{
	struct plugin *p = plugin;
	if(p != NULL) {
		p->exit();
		dlclose(p->handle);
		free(p);
	}
}
