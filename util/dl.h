/*
   Copyright (C) 2006 Mike Shal

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
 * A little wrapper for dynamic loading functions for different OS's
 */

#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)
# include <dlfcn.h>
#elif defined(WIN32)
#  include <windows.h>
#  define dlopen(l, a) LoadLibrary(l)
#  define dlclose FreeLibrary
#  define dlsym GetProcAddress
#  define dlerror() 0
#else
#  warning "This platform does not have dynamic library support!"
#  define dlopen(l, a) 0
#  define dlclose(l)
#  define dlsym(l, s) 0
#  define dlerror() 0
#endif
