/* This testcase is part of GDB, the GNU debugger.

   Copyright 2015-2024 Free Software Foundation, Inc.

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
#include <assert.h>
#include <stdlib.h>

static int
test (void)
{
  void *dso;
  int (*fun) (void);
  int answer;

  dso = dlopen (DSO_NAME, RTLD_NOW | RTLD_GLOBAL);
  assert (dso != NULL);

  fun = (int (*) (void)) dlsym (dso, "answer");
  assert (fun != NULL);

  answer = fun ();

  dlclose (dso);

  return answer;
}

int
main (void)
{
  int answer;

  answer = test ();

  return answer;
}
