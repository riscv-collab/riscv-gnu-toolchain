/* Platform independent shared object routines for GDB.

   Copyright (C) 2011-2024 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "common-defs.h"
#include "gdb-dlfcn.h"

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#elif __MINGW32__
#include <windows.h>
#else
/* Unsupported configuration. */
#define NO_SHARED_LIB
#endif

#ifdef NO_SHARED_LIB

gdb_dlhandle_up
gdb_dlopen (const char *filename)
{
  gdb_assert_not_reached ("gdb_dlopen should not be called on this platform.");
}

void *
gdb_dlsym (const gdb_dlhandle_up &handle, const char *symbol)
{
  gdb_assert_not_reached ("gdb_dlsym should not be called on this platform.");
}

void
dlclose_deleter::operator() (void *handle) const
{
  gdb_assert_not_reached ("gdb_dlclose should not be called on this platform.");
}

int
is_dl_available (void)
{
  return 0;
}

#else /* NO_SHARED_LIB */

gdb_dlhandle_up
gdb_dlopen (const char *filename)
{
  void *result;
#ifdef HAVE_DLFCN_H
  result = dlopen (filename, RTLD_NOW);
#elif __MINGW32__
  result = (void *) LoadLibrary (filename);
#endif
  if (result != NULL)
    return gdb_dlhandle_up (result);

#ifdef HAVE_DLFCN_H
  error (_("Could not load %s: %s"), filename, dlerror());
#else
  {
    LPVOID buffer;
    DWORD dw;

    dw = GetLastError();

    FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
		   FORMAT_MESSAGE_IGNORE_INSERTS,
		   NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		   (LPTSTR) &buffer,
		   0, NULL);

    error (_("Could not load %s: %s"), filename, (char *) buffer);
  }
#endif
}

void *
gdb_dlsym (const gdb_dlhandle_up &handle, const char *symbol)
{
#ifdef HAVE_DLFCN_H
  return dlsym (handle.get (), symbol);
#elif __MINGW32__
  return (void *) GetProcAddress ((HMODULE) handle.get (), symbol);
#endif
}

void
dlclose_deleter::operator() (void *handle) const
{
#ifdef HAVE_DLFCN_H
  dlclose (handle);
#elif __MINGW32__
  FreeLibrary ((HMODULE) handle);
#endif
}

int
is_dl_available (void)
{
  return 1;
}

#endif /* NO_SHARED_LIB */
