/* This testcase is part of GDB, the GNU debugger.

   Copyright (C) 2013-2024 Free Software Foundation, Inc.

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

#include <stdio.h>
#include <stdlib.h>

#ifdef __WIN32__
#include <windows.h>
#define dlopen(name, mode) LoadLibrary (TEXT (name))
# define dlsym(handle, func) GetProcAddress (handle, func)
#define dlclose(handle) FreeLibrary (handle)
#else
#include <dlfcn.h>
#endif

static void **handles;

void
do_test_load (int number)
{
  char libname[40];
  int i;

  handles = malloc (sizeof (void *) * number);
  if (handles == NULL)
    {
      printf ("ERROR on malloc\n");
      exit (-1);
    }

  for (i = 0; i < number; i++)
    {
      sprintf (libname, "solib-lib%d", i);
      handles[i] = dlopen (libname, RTLD_LAZY);
      if (handles[i] == NULL)
	{
	  printf ("ERROR on dlopen %s\n", libname);
	  exit (-1);
	}
    }
}

void
do_test_unload (int number)
{
  int i;

  /* Unload shared libraries in different orders.  */
#ifndef SOLIB_DLCLOSE_REVERSED_ORDER
  for (i = 0; i < number; i++)
#else
  for (i = number - 1; i >= 0; i--)
#endif
    dlclose (handles[i]);

  free (handles);
}

static void
end (void)
{}

int
main (void)
{
  end ();

  return 0;
}
