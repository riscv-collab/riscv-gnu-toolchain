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
   along with this program.  If not, see  <http://www.gnu.org/licenses/>.  */

#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Defined in py-events-shlib.h.  */
extern void do_nothing (void);

int increase_1 (int *a)
{
  *a += 1;
  return -5;
}

void increase (int *a)
{
  increase_1 (a);
}

int increase_2 (int *a)
{
  *a += 10;
  return -8;
}

inline void __attribute__((always_inline))
increase_inlined (int *a)
{
  increase_2 (a);
  *a += 5;
}

int
test_1 (int i, int j)
{
  return i == j;
}

int
test (int i, int j)
{
  return test_1 (i, j);
}

int
call_longjmp_1 (jmp_buf *buf)
{
  longjmp (*buf, 1);
}

int
call_longjmp (jmp_buf *buf)
{
  call_longjmp_1 (buf);
  return 0;
}

void
test_exec_exit (const char *self_exec)
{
  if (self_exec == NULL)
    exit (0);
  else
    execl (self_exec, self_exec, "exit", (char *)0);
}

int main (int argc, char *argv[])
{
  jmp_buf env;
  int foo = 5;
  int bar = 42;
  int i, j;

  if (argc == 2 && strcmp (argv[1], "exit") == 0)
    return 0;

  do_nothing ();

  i = 0;
  /* Break at increase.  */
  increase (&i);
  increase (&i);
  increase (&i);
  increase_inlined (&i);

  for (i = 0; i < 10; i++)
    {
      j += 1; /* Condition Break.  */
    }

  if (setjmp (env) == 0) /* longjmp caught */
    {
      call_longjmp (&env);
    }
  else
    j += 1; /* after longjmp.  */

  test_exec_exit (argv[0]);

  return j; /* Break at end.  */
}
