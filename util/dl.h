#if defined(__linux__) || defined(__FreeBSD__)
# include <dlfcn.h>
#else
# if defined(WIN32)
#  include <windows.h>
#  define dlopen(l, a) LoadLibrary(l)
#  define dlclose FreeLibrary
#  define dlsym GetProcAddress
#  define dlerror() NULL
# else
#  error "This platform does not have dynamic library support!"
# endif
#endif
/* WIN32 or _WIN32 ? MacOSX */
