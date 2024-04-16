/* This testcase is part of GDB, the GNU debugger.

   Copyright (C) 2013-2024 Free Software Foundation, Inc.

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

struct s
{
  int a[256];
  char c[256];
};

static void
fun2 (void)
{

}

static void
fun1 (int i, int j, long k, struct s ss)
{
  /* Allocate local variables on stack.  */
  struct s s1;

  if (i < BACKTRACE_DEPTH)
    fun1 (i + 1, j + 2, k - 1, ss);
  else
    {
      int ii;

      for (ii = 0; ii < 10; ii++)
	fun2 ();
    }
}

int
main (void)
{
  struct s ss;

  fun1 (0, 0, 200, ss);
  return 0;
}
