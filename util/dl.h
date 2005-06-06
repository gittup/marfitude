#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)
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
