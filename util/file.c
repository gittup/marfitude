#include <stdio.h>
#include <dirent.h>
#include "file.h"

/** Execute the action for each file in path
  * @param path A directory path (full or relative)
  * @param action A function pointer that is called for each file in the path.
  *        The single argument will be path/file.
  * @return 0 For success, or 1 if the path cannot be opened
  */
int foreach_file(const char *path, void (*action)(const char *))
{
	DIR *dir;
	struct dirent *d;

	dir = opendir(path);
	if(dir == NULL) {
		fprintf(stderr, "Can't open %s\n", path);
		return 1;
	}
	while((d = readdir(dir)) != NULL) {
		action(d->d_name);
	}
	closedir(dir);
	return 0;
}
