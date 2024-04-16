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

static void
func1 (void)
{}

int buf[1024];

static void
func2 (void)
{}

static void
end (void)
{}

int
main (void)
{
  int i;

  func1 ();

  /* We call func2 as many times as possible to make sure that trace is
     stopped due to trace buffer is full.  */
  for (i = 0; i < 10000; i++)
    {
      func2 ();
    }

  end ();
  return 0;
}
