/* Copyright 2022-2024 Free Software Foundation, Inc.

   This file is part of GDB.

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

static inline int __attribute__((__always_inline__))
foo ()
{
  return 0;	/* Multi-location breakpoint here.  */
}

static int __attribute__((noinline))
bar ()
{
  int res = foo ();	/* Single-location breakpoint here.  */

  return res;
}

int
main ()
{
  int res = bar ();

  res = foo ();

  return res;
}
