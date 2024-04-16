/* This testcase is part of GDB, the GNU debugger.

   Copyright 2011-2024 Free Software Foundation, Inc.

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
#include <assert.h>

static const char *volatile filename;

static void
notify (void)
{
  filename = NULL; /* notify-here */
}

int
main (void)
{
  void *h;
  void (*fp) (void (*) (void));

  assert (filename != NULL);
  h = dlopen (filename, RTLD_LAZY);
  assert (h != NULL);

  fp = dlsym (h, "f");
  assert (fp != NULL);

  fp (notify);

  return 0;
}
