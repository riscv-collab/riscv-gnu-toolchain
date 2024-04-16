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

static int bar (void);
static int baz (void);
static int foo (void);

static void
stop_breakpt (void)
{
  /* Nothing.  */
}

int
main (void)
{
  int ret = baz ();
  stop_breakpt ();
  return ret;
}

static int
bar (void)
{
  return baz ();
}

static int
foo (void)
{
  return 0;
}

static int
baz (void)
{
  return foo ();
}
