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

#include "plugin.h"
#include "dl.h"
#include "memtest.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** @file
 * Implements loading and freeing plugins
 */

/** Returns a plugin handle for the plugin @a file */
void *load_plugin(const char *file)
{
	void *handle;
	const char *error;
	char *path;

	path = malloc(strlen(file) + strlen(MARFSHLIBEXT) + 3);
	strcpy(path, "./");
	strcat(path, file);
	strcat(path, MARFSHLIBEXT);

	error = dlerror();

	handle = dlopen(path, RTLD_NOW);
	if(handle == NULL) {
		error = dlerror();
		if(error != NULL)
			printf("%s\n", error);
		printf("Error: Couldn't load plugin: %s\n", path);
	}
	free(path);
	return handle;
}

/** Frees the plugin handle that was returned by load_plugin */
void free_plugin(void *handle)
{
	if(handle != NULL) {
		dlclose(handle);
	}
}
