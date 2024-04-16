/* This testcase is part of GDB, the GNU debugger.

   Copyright 2009-2024 Free Software Foundation, Inc.

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


   The original issue we're trying to test is described in this
   thread:

     https://sourceware.org/legacy-ml/gdb-patches/2009-06/msg00802.html

   The NEW_THREAD_EVENT code the comments below refer to no longer
   exists in GDB, so the following comments are kept for historical
   reasons, and to guide future updates to the testcase.

   ---

   Do not use threads as we need to exploit a bug in LWP code masked by the
   threads code otherwise.

   INFERIOR_PTID must point to exited LWP.  Here we use the initial LWP as it
   is automatically INFERIOR_PTID for GDB.

   Finally we need to call target_resume (RESUME_ALL, ...) which we invoke by
   NEW_THREAD_EVENT (called from the new LWP as initial LWP is exited now).  */

#define _GNU_SOURCE
#include <sched.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

#define STACK_SIZE 0x1000

/* True if the 'fn_return' thread has been reached at the point after
   its parent is already gone.  */
volatile int fn_return_reached = 0;

/* True if the 'fn' thread has exited.  */
volatile int fn_exited = 0;

/* Wrapper around clone.  */

static int
do_clone (int (*fn)(void *))
{
  unsigned char *stack;
  int new_pid;

  stack = malloc (STACK_SIZE);
  assert (stack != NULL);

  new_pid = clone (fn, stack + STACK_SIZE, CLONE_FILES | CLONE_VM,
		   NULL, NULL, NULL, NULL);
  assert (new_pid > 0);

  return new_pid;
}

static int
fn_return (void *unused)
{
  /* Wait until our direct parent exits.  We want the breakpoint set a
     couple lines below to hit with the previously-selected thread
     gone.  */
  while (!fn_exited)
    usleep (1);

  fn_return_reached = 1; /* at-fn_return */
  return 0;
}

static int
fn (void *unused)
{
  do_clone (fn_return);
  return 0;
}

int
main (int argc, char **argv)
{
  int new_pid, status, ret;

  new_pid = do_clone (fn);

  /* Note the clone call above didn't use CLONE_THREAD, so it actually
     put the new child in a new thread group.  However, the new clone
     is still reported with PTRACE_EVENT_CLONE to GDB, since we didn't
     use CLONE_VFORK (results in PTRACE_EVENT_VFORK) nor set the
     termination signal to SIGCHLD (results in PTRACE_EVENT_FORK), so
     GDB thinks of it as a new thread of the same inferior.  It's a
     bit of an odd setup, but it's not important for what we're
     testing, and, it let's us conveniently use waitpid to wait for
     the child, which you can't with CLONE_THREAD.  */
  ret = waitpid (new_pid, &status, __WALL);
  assert (ret == new_pid);
  assert (WIFEXITED (status) && WEXITSTATUS (status) == 0);

  fn_exited = 1;

  /* Don't exit before the breakpoint at fn_return triggers.  */
  while (!fn_return_reached)
    usleep (1);

  return 0;
}
