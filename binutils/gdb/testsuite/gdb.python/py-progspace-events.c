/* This testcase is part of GDB, the GNU debugger.

   Copyright 2023-2024 Free Software Foundation, Inc.

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

#include <unistd.h>
#include <sys/types.h>
#include <assert.h>
#include <sys/wait.h>

/* This gives the inferior something to do.  */
volatile int global_var = 0;

void
breakpt ()
{ /* Nothing.  */ }

void
do_child_stuff ()
{
  breakpt ();
  ++global_var;
}

void
do_parent_stuff ()
{
  breakpt ();
  ++global_var;
}

void
create_child ()
{
  int stat;
  pid_t wpid;
  breakpt ();
  pid_t pid = fork ();
  assert (pid != -1);

  if (pid == 0)
    {
      /* Child.  */
      do_child_stuff ();
      return;
    }

  /* Parent.  */
  do_parent_stuff ();
  wpid = waitpid (pid, &stat, 0);
  assert (wpid == pid);
}



int
main ()
{
  create_child ();
  return 0;
}
