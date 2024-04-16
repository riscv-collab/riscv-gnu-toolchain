/* Copyright (C) 2007-2024 Free Software Foundation, Inc.

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

/* This shared library will dlopen another shared object.
   This is based on gdb.base/solib-disc.c.  */

#include <stdio.h>
#include <stdlib.h>

#ifdef __WIN32__
#include <windows.h>
#define dlopen(name, mode) LoadLibrary (name)
#define dlsym(handle, func) GetProcAddress (handle, func)
#define dlclose(handle) FreeLibrary (handle)
#define dlerror() "an error occurred"
#else
#include <dlfcn.h>
#endif

const char *the_shlib = SHLIB_NAME;

int
shlib_function (void)
{
  void *handle;
  int (*func) (void);
  int result;

  handle = dlopen (the_shlib, RTLD_LAZY);
  if (!handle)
    {
      fprintf (stderr, "%s\n", dlerror ());
      exit (1);
    }

  func = (int (*)(void)) dlsym (handle, "crashfunc");
  if (func == NULL)
    {
      fprintf (stderr, "%s\n", dlerror ());
      exit (1);
    }

  result = func ();
  dlclose (handle);
  return result;
}
