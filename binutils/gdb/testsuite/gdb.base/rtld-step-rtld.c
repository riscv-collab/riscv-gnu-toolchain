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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <unistd.h>

/* The declarations below are needed if you try to make this test case
   work on FreeBSD.  They're disabled because there are other problems
   with this test on FreeBSD.  See the .exp file for more info.  If
   those other problems can be resolved, it may be worth reenabling
   these declarations.  */
#if 0
__attribute__((weak))
char *__progname = "fake-rtld";

__attribute__((weak))
char **environ = 0;
#endif

void
baz (int i)
{
}

void
foo (int a)
{
  baz (a);
}

void
bar ()
{
  foo (1);
  baz (99);
  foo (2);
}

int
main ()
{
  foo (0);
  bar ();
  return 0;
}

void
_start ()
{
  main ();
  _exit (0);
}
