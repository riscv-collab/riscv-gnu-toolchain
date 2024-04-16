/* This testcase is part of GDB, the GNU debugger.

   Copyright 2018-2024 Free Software Foundation, Inc.

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

static void
setup_done (void)
{
}

static int
f1 (int f1arg)
{
  int f1loc;

  f1loc = f1arg - 1;

  setup_done ();
  return f1loc;
}

static int
f2 (int f2arg)
{
  int f2loc;

  f2loc = f1 (f2arg - 1);

  return f2loc;
}

static int
f3 (int f3arg)
{
  int f3loc;

  f3loc = f2 (f3arg - 1);

  return f3loc;
}

static int
f4 (int f4arg)
{
  int f4loc;

  f4loc = f3 (f4arg - 1);

  return f4loc;
}

int
main (void)
{
  int result;

  result = f4 (4);
  return 0;
}
