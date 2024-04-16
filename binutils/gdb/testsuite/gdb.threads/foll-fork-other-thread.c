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

#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <assert.h>
#include <limits.h>

/* Set by GDB.  */
volatile int stop_looping = 0;

static void *
gdb_forker_thread (void *arg)
{
  int ret;
  int stat;
  pid_t pid = FORK_FUNC ();

  if (pid == 0)
    _exit (0);

  assert (pid > 0);

  /* Wait for child to exit.  */
  do
    {
      ret = waitpid (pid, &stat, 0);
    }
  while (ret == -1 && errno == EINTR);

  assert (ret == pid);
  assert (WIFEXITED (stat));
  assert (WEXITSTATUS (stat) == 0);

  stop_looping = 1;

  return NULL;
}

static void
sleep_a_bit (void)
{
  usleep (1000 * 50);
}

int
main (void)
{
  int i;
  int ret;
  pthread_t thread;

  alarm (60);

  ret = pthread_create (&thread, NULL, gdb_forker_thread, NULL);
  assert (ret == 0);

  while (!stop_looping)  /* while loop */
    {
      sleep_a_bit ();    /* break here */
      sleep_a_bit ();    /* other line */
    }

  pthread_join (thread, NULL);

  return 0; /* exiting here */
}
