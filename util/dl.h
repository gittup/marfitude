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
 * A little wrapper for dynamic loading functions for differen OS's
 */

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
#   if defined(__APPLE__)
/* Umm...yeah */
#     include <CoreFoundation/CoreFoundation.h>
#     define dlstrtmp(s) CFStringCreateWithCString(kCFAllocatorDefault, s,\
				kCFStringEncodingASCII)
#     define dlopentmp(l, a) CFBundleCreate(kCFAllocatorDefault, \
		CFURLCreateWithFileSystemPath(kCFAllocatorDefault, \
			dlstrtmp(l), \
			kCFURLPOSIXPathStyle, true))
#     define dlopen(l, a) (CFBundleLoadExecutable(dlopentmp(l, a)), \
	dlopentmp(l, a))
#     define dlclose CFBundleUnloadExecutable
#     define dlsym(l, s) CFBundleGetFunctionPointerForName(l, dlstrtmp(s))
#     define dlerror() strerror(errno)
#   else
#     error "This platform does not have dynamic library support!"
#   endif
# endif
#endif
