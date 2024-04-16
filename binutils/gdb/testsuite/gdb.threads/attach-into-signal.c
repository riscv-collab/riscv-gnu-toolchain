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

#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef USE_THREADS
#include <pthread.h>
#endif

void action(int sig, siginfo_t * info, void *uc)
{
  raise (SIGALRM);
}

static void *func (void *arg)
{
  struct sigaction act;

  memset (&act, 0, sizeof(struct sigaction));
  act.sa_sigaction = action;
  act.sa_flags = SA_RESTART;
  sigaction (SIGALRM, &act, 0);

  raise (SIGALRM);

  /* We must not get past this point, either in a free standing or debugged
     state.  */

  abort ();
  /* NOTREACHED */
  return NULL;
}

int main ()
{

#ifndef USE_THREADS

  func (NULL);

#else

  pthread_t th;
  pthread_create (&th, NULL, func, NULL);
  pthread_join (th, NULL);

#endif

  return 0;
}
