/* This testcase is part of GDB, the GNU debugger.

   Copyright 2021-2024 Free Software Foundation, Inc.

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

#include <stdlib.h>
#include <setjmp.h>

jmp_buf env;

void
worker (void)
{
  longjmp (env, 1);
}

void
test_inner (void)
{
  if (setjmp (env) == 0)
    {
      /* Direct call.  */
      worker ();

      /* Will never get here.  */
      abort ();
    }
  else
    {
      /* Called from longjmp.  */
    }
}

void
break_bt_here (void)
{
  test_inner ();
}

int
some_func (void)
{
  break_bt_here ();
  return 0;
}

int
main (void)
{
  some_func ();

  return 0;
}
