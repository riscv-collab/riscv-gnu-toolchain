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

#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>

static volatile int release_vfork = 0;
static volatile int release_main = 0;

static void *
vforker (void *arg)
{
  while (!release_vfork)
    usleep (1);

  pid_t pid = vfork ();
  if (pid == 0)
    {
      /* A vfork child is not supposed to mess with the state of the program,
	 but it is helpful for the purpose of this test.  */
      release_main = 1;
      _exit(7);
    }

  int stat;
  int ret = waitpid (pid, &stat, 0);
  assert (ret == pid);
  assert (WIFEXITED (stat));
  assert (WEXITSTATUS (stat) == 7);

  return NULL;
}

static void
should_break_here (void)
{}

int
main (void)
{

  pthread_t thread;
  int ret = pthread_create (&thread, NULL, vforker, NULL);
  assert (ret == 0);

  /* We break here first, while the thread is stuck on `!release_fork`.  */
  release_vfork = 1;

  /* We set a breakpoint on should_break_here.

     We then set "release_fork" from the debugger and continue.  The main
     thread hangs on `!release_main` while the non-main thread vforks.  During
     the window of time where the two processes have a shared address space
     (after vfork, before _exit), GDB removes the breakpoints from the address
     space.  During that window, only the vfork-ing thread (the non-main
     thread) is frozen by the kernel.  The main thread is free to execute.  The
     child process sets `release_main`, releasing the main thread. A buggy GDB
     would let the main thread execute during that window, leading to the
     breakpoint on should_break_here being missed.  A fixed GDB does not resume
     the threads of the vforking process other than the vforking thread.  When
     the vfork child exits, the fixed GDB resumes the main thread, after
     breakpoints are reinserted, so the breakpoint is not missed.  */

  while (!release_main)
    usleep (1);

  should_break_here ();

  pthread_join (thread, NULL);

  return 6;
}
