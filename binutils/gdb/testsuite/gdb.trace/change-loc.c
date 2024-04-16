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

#include <stdio.h>
#include <dlfcn.h>
#include "change-loc.h"

extern void func (int x);

static void
marker () {}

int main()
{
  const char *libname = "change-loc-2.sl";
  void *h;
  int (*p_func) (int);

  func (3);

  func4 ();

  marker ();

  h = dlopen (libname, RTLD_LAZY);
  if (h == NULL) return 1;

  p_func = dlsym (h, "func2");
  if (p_func == NULL) return 2;

  (*p_func) (4);

  marker ();

  dlclose (h);

  marker ();
  return 0;
}
