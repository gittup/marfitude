#include <stdio.h>
#include <stdlib.h>

#include "plugin.h"
#include "dl.h"

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
			p = (struct plugin *)malloc(sizeof(struct plugin));
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
	struct plugin *p = (struct plugin *)plugin;
	if(p != NULL) {
		p->exit();
		dlclose(p->handle);
		free(p);
	}
}
