/* This testcase is part of GDB, the GNU debugger.

   Copyright 2010-2024 Free Software Foundation, Inc.

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
#include <stddef.h>

static int
openlib (const char *filename)
{
  void *h = dlopen (filename, RTLD_LAZY);

  if (filename == NULL)
    return 0;

  if (h == NULL)
    return 0;
  if (dlclose (h) != 0)
    return 0;
  return 1;
}

int
main (void)
{
  /* Dummy call to get the function always compiled in.  */
  openlib (NULL);

  return 0;
}
