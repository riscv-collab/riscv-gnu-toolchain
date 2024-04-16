/* This test is part of GDB, the GNU debugger.

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

volatile int v = 42;

__attribute__((__always_inline__)) static inline int
f (void)
{
  /* Provide first stub line so that GDB understand the PC is already inside
     the inlined function and does not expect a step into it.  */
  v++;
  v++;		/* break-here */

  return v;
}

__attribute__((__noinline__)) static int
g (void)
{
  volatile int l = v;

  return f ();
}

int
main (void)
{
  int x = g ();
  x += f ();
  return x;
}
