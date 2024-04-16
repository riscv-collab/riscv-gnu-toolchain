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

   Test that GDB doesn't lose an event for a thread it didn't know
   about, until an event is reported for it.  */

#define _GNU_SOURCE
#include <sched.h>
#include <assert.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/wait.h>

#include <features.h>
#ifdef __UCLIBC__
#if !(defined(__UCLIBC_HAS_MMU__) || defined(__ARCH_HAS_MMU__))
#define HAS_NOMMU
#endif
#endif

#define STACK_SIZE 0x1000

static int
tkill (int lwpid, int signo)
{
  return syscall (__NR_tkill, lwpid, signo);
}

static pid_t
local_gettid (void)
{
  return syscall (__NR_gettid);
}

static int
fn (void *unused)
{
  tkill (local_gettid (), SIGUSR1);
  return 0;
}

int
main (int argc, char **argv)
{
  unsigned char *stack;
  int new_pid, status, ret;

  stack = malloc (STACK_SIZE);
  assert (stack != NULL);

  new_pid = clone (fn, stack + STACK_SIZE, CLONE_FILES
#if defined(__UCLIBC__) && defined(HAS_NOMMU)
		   | CLONE_VM
#endif /* defined(__UCLIBC__) && defined(HAS_NOMMU) */
		   , NULL, NULL, NULL, NULL);
  assert (new_pid > 0);

  /* Note the clone call above didn't use CLONE_THREAD, so it actually
     put the new thread in a new thread group.  However, the new clone
     is still reported with PTRACE_EVENT_CLONE to GDB, since we didn't
     use CLONE_VFORK (results in PTRACE_EVENT_VFORK) nor set the
     termination signal to SIGCHLD (results in PTRACE_EVENT_FORK), so
     GDB thinks of it as a new thread of the same inferior.  It's a
     bit of an odd setup, but it's not important for what we're
     testing, and, it let's us conveniently use waitpid to wait for
     the clone, which you can't with CLONE_THREAD.  */
  ret = waitpid (new_pid, &status, __WALL);
  assert (ret == new_pid);
  assert (WIFSIGNALED (status) && WTERMSIG (status) == SIGUSR1);

  return 0;
}
