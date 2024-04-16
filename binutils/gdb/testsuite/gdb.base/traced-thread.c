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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#define _GNU_SOURCE

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>

int fds[2];

_Atomic pid_t bg_tid = 0;

pthread_barrier_t barrier;

#define FIVE_MINUTES (5 * 60)

/* One thread of the child process.  This is traced by the parent
   process.  */
void *
block (void *ignore)
{
  bg_tid = gettid ();
  pthread_barrier_wait (&barrier);
  sleep (FIVE_MINUTES);
  return 0;
}

/* The parent process blocks in this function.  */
void
parent_stop (pid_t child_thread_pid)
{
  sleep (FIVE_MINUTES);
}

int
main ()
{
  int result;

  pthread_barrier_init (&barrier, NULL, 2);

  result = pipe (fds);
  assert (result != -1);

  pid_t child = fork ();
  if (child != 0)
    {
      /* Parent.  */
      close (fds[1]);

      pid_t the_tid;
      result = read (fds[0], &the_tid, sizeof (the_tid));
      assert (result == sizeof (the_tid));

      /* Trace a single, non-main thread of the child.  This should
	 prevent gdb from attaching to the child at all.  The bug here
	 was that gdb would get into an infinite loop repeatedly
	 trying to attach to this thread.  */
      result = ptrace (PTRACE_SEIZE, the_tid, (void *) 0, (void *) 0);
      if (result == -1)
	perror ("ptrace");

      parent_stop (child);
    }
  else
    {
      /* Child.  */

      close (fds[0]);

      pthread_t thr;
      result = pthread_create (&thr, 0, block, 0);
      assert (result == 0);

      /* Wait until the TID has been assigned.  */
      pthread_barrier_wait (&barrier);
      assert (bg_tid != 0);

      result = write (fds[1], &bg_tid, sizeof (bg_tid));
      assert (result == sizeof (bg_tid));

      sleep (FIVE_MINUTES);
    }

  exit (0);
}
