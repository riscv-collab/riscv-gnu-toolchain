/* This testcase is part of GDB, the GNU debugger.

   Copyright 2010-2024 Free Software Foundation, Inc.

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
#include <assert.h>
#include <signal.h>

#include <asm/unistd.h>
#include <unistd.h>
#define tgkill(tgid, tid, sig) syscall (__NR_tgkill, (tgid), (tid), (sig))
#define gettid() syscall (__NR_gettid)

static volatile int var;

static void
handler (int signo)	/* step-0 */
{			/* step-0 */
  var++;		/* step-1 */
  tgkill (getpid (), gettid (), SIGUSR1);	/* step-2 */
}

static void *
start (void *arg)
{
  tgkill (getpid (), gettid (), SIGUSR1);
  assert (0);
  return NULL;
}

int
main (void)
{
  pthread_t thread;

  signal (SIGUSR1, handler);

  pthread_create (&thread, NULL, start, NULL);
  start (NULL);	/* main-start */
  return 0;
}
