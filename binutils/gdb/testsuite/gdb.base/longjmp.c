/* This testcase is part of GDB, the GNU debugger.

   Copyright 2008-2024 Free Software Foundation, Inc.

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

#include <setjmp.h>

jmp_buf env;

volatile int longjmps = 0;
volatile int resumes = 0;

int
call_longjmp (jmp_buf *buf)
{
  longjmps++;
  longjmp (*buf, 1);
}

void
hidden_longjmp (void)
{
  if (setjmp (env) == 0)
    {
      call_longjmp (&env);
    }
  else
    resumes++;
}

int
main ()
{
  volatile int i = 0;

  /* Pattern 1 - simple longjmp.  */
  if (setjmp (env) != 0) /* patt1 */
    {
      resumes++;
    }
  else
    {
      longjmps++;
      longjmp (env, 1);
    }

  i = 1; /* miss_step_1 */


  /* Pattern 2 - longjmp from an inner function.  */
  if (setjmp (env) == 0) /* patt2 */
    {
      call_longjmp (&env);
    }
  else
    {
      resumes++;
    }

  i = 2; /* miss_step_2 */

  /* Pattern 3 - setjmp/longjmp inside stepped-over function.  */
  hidden_longjmp (); /* patt3 */

  i = 77; /* longjmp caught */

  i = 3; /* patt_end3.  */

  return 0;
}
