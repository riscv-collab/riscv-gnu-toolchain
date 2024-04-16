/* This testcase is part of GDB, the GNU debugger.

   Copyright 2016-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define _GNU_SOURCE
#include <sched.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define STACK_SIZE 0x1000

int clone_pid;

static int
clone_fn (void *unused)
{
  /* Wait for alarm.  */
  while (1)
    sleep (1);
  return 0;
}

int
main (int argc, char **argv)
{
  unsigned char *stack;
  int res;

  alarm (300);

  stack = malloc (STACK_SIZE);
  assert (stack != NULL);

#define CLONE_FLAGS (CLONE_THREAD | CLONE_SIGHAND | CLONE_VM)

#ifdef __ia64__
  clone_pid = __clone2 (clone_fn, stack, STACK_SIZE, CLONE_FLAGS, NULL);
#else
  clone_pid = clone (clone_fn, stack + STACK_SIZE, CLONE_FLAGS, NULL);
#endif

  assert (clone_pid > 0);

  /* Wait for alarm.  */
  while (1)
    sleep (1);

  return 0;
}
