/* Copyright 2012-2024 Free Software Foundation, Inc.

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
#include <stddef.h>

void
stop (void)
{
}

int
main (void)
{
  void *handle1, *handle2;
  void (*func)(int);

  handle1 = dlopen (SHLIB1_NAME, RTLD_LAZY);
  assert (handle1 != NULL);
  stop ();

  handle2 = dlopen (SHLIB2_NAME, RTLD_LAZY);
  assert (handle2 != NULL);
  stop ();

  func = (void (*)(int)) dlsym (handle1, "foo");
  func (1);

  func = (void (*)(int)) dlsym (handle2, "bar");
  func (2);

  dlclose (handle1);
  stop ();

  dlclose (handle2);
  stop ();

  return 0;
}
