/* This testcase is part of GDB, the GNU debugger.

   Copyright 2017-2024 Free Software Foundation, Inc.

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

#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main ()
{
  void *handle;
  int (*foo) (int);
  char *error;
  char *dest = VANISH_LIB ".renamed";

  /* Open library.  */
  handle = dlopen (VANISH_LIB, RTLD_NOW);
  if (!handle)
    {
      fprintf (stderr, "%s\n", dlerror ());
      exit (EXIT_FAILURE);
    }

  /* Clear any existing error */
  dlerror ();

  /* Simulate deleting file by renaming it.  */
  if (rename (VANISH_LIB, dest) == -1)
    {
      error = strerror (errno);
      fprintf (stderr, "rename %s -> %s: %s\n", VANISH_LIB, dest, error);
      exit (EXIT_FAILURE);
    }

  /* Get function pointer.  */
  foo = dlsym (handle, "foo");
  error = dlerror ();
  if (error != NULL)
    {
      fprintf (stderr, "%s\n", error);
      exit (EXIT_FAILURE);
    }

  /* Call function.  */
  (*foo) (1);

  /* Close and exit.  */
  dlclose (handle);

  /* Put VANISH_LIB back where we found it.  */
  if (rename (dest, VANISH_LIB) == -1)
    {
      error = strerror (errno);
      fprintf (stderr, "rename %s -> %s: %s\n", dest, VANISH_LIB, error);
      exit (EXIT_FAILURE);
    }

  exit (EXIT_SUCCESS);
}
