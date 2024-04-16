/* This testcase is part of GDB, the GNU debugger.

   Copyright 2004-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#include <stdio.h>
#include <stdlib.h>

#ifdef __WIN32__
#include <windows.h>
#define dlopen(name, mode) LoadLibrary (TEXT (name))
#ifdef _WIN32_WCE
# define dlsym(handle, func) GetProcAddress (handle, TEXT (func))
#else
# define dlsym(handle, func) GetProcAddress (handle, func)
#endif
#define dlclose(handle) FreeLibrary (handle)
#else
#include <dlfcn.h>
#endif


void open_shlib ()
{
  void *handle;
  void (*foo) (int);

  handle = dlopen (SHLIB_NAME, RTLD_LAZY);
  
  if (!handle)
    {
#ifdef __WIN32__
      fprintf (stderr, "error %d occurred\n", GetLastError ());
#else
      fprintf (stderr, "%s\n", dlerror ());
#endif
      exit (1);
    }

  foo = (void (*)(int))dlsym (handle, "foo");

  if (!foo)
    {
      fprintf (stderr, "%s\n", dlerror ());
      exit (1);
    }

  foo (1); // call to foo
  foo (2);

  dlclose (handle);
}


int main()
{
  open_shlib ();
  return 0;
}
