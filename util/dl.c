#include "dl.h"

void *opendl(const char *file)
{
	return dlopen(file, RTLD_NOW);
}

void closedl(void *handle)
{
	dlclose(handle);
}

void *getsym(void *handle, const char *sym)
{
	return dlsym(handle, sym);
}
