/* This testcase is part of GDB, the GNU debugger.

   Copyright 2022-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see  <http://www.gnu.org/licenses/>.  */

#include <unistd.h>

#ifdef __WIN32__
#include <windows.h>
#define dlopen(name, mode) LoadLibrary (TEXT (name))
#define dlclose(handle) FreeLibrary (handle)
#else
#include <dlfcn.h>
#endif

/* This is updated by the .exp file.  */
char *libname = "py-events-shlib.so";

int
main ()
{
  void *h;

  h = dlopen (libname, RTLD_LAZY);

  dlclose (h);

  h = NULL;			/* final breakpoint here */
  return 0;
}
