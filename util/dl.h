#if defined(__linux__) || defined(__FreeBSD__)
#include <dlfcn.h>
#else
#error "This platform does not have dynamic library support!"
#endif
/* WIN32 or _WIN32 ? MacOSX */
