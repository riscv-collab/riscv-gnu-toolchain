/* This testcase is part of GDB, the GNU debugger.

   Copyright 2012-2024 Free Software Foundation, Inc.

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

volatile int v;

static __attribute__ ((noinline, noclone)) void
g (void)
{
  v = 2;
}

static __attribute__ ((noinline, noclone)) void
f (void)
{
  g ();
}

extern void nodebug (void);

int
main (void)
{
  v = 1;
  f ();
  nodebug ();
  v = 3;
  return 0;
}
