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

#include <signal.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>

#ifndef SA_SIGINFO
# error "SA_SIGINFO is required for this test"
#endif

static int
callme (void)
{
  return 42;
}

static int
pass (void)
{
  return 1;
}

static int
fail (void)
{
  return 1;
}

static void
handler (int sig, siginfo_t *siginfo, void *context)
{
  assert (sig == SIGUSR1);
  assert (siginfo->si_signo == SIGUSR1);
  if (siginfo->si_pid == getpid ())
    pass ();
  else
    fail ();
}

int
main (void)
{
  struct sigaction sa;
  int i;

  callme ();

  memset (&sa, 0, sizeof (sa));
  sa.sa_sigaction = handler;
  sa.sa_flags = SA_SIGINFO;

  i = sigemptyset (&sa.sa_mask);
  assert (i == 0);

  i = sigaction (SIGUSR1, &sa, NULL);
  assert (i == 0);

  i = raise (SIGUSR1);
  assert (i == 0);

  sleep (600);
  return 0;
}
