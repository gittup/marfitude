/** @file
  * @brief Support for plugins.
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
