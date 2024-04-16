/* This testcase is part of GDB, the GNU debugger.

   Copyright 2020-2024 Free Software Foundation, Inc.

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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "my-syscalls.h"

#if (!defined(LEADER_DOES_EXEC) && !defined(OTHER_DOES_EXEC) \
     || defined(LEADER_DOES_EXEC) && defined(OTHER_DOES_EXEC))
# error "Exactly one of LEADER_DOES_EXEC and OTHER_DOES_EXEC must be defined."
#endif


static char *argv0;
static pthread_barrier_t barrier;

static void
do_the_exec (void)
{
  char *execd_path = (char *) malloc (strlen (argv0) + sizeof ("-execd"));
  sprintf (execd_path, "%s-execd", argv0);
  char *argv[] = { execd_path, NULL };

  printf ("Exec-ing %s\n", execd_path);

  extern char **environ;
  my_execve (execd_path, argv, environ);

  printf ("Exec failed :(\n");
  abort ();
}

static void *
thread_func (void *arg)
{
  pthread_barrier_wait (&barrier);
#ifdef OTHER_DOES_EXEC
  printf ("Other going in exec.\n");
  do_the_exec ();
#endif

  /* Just make sure the thread does not exit when the leader does the exec.  */
  pthread_barrier_wait (&barrier);

  return NULL;
}

int
main (int argc, char *argv[])
{
  argv0 = argv[0];

  int ret = pthread_barrier_init (&barrier, NULL, 2);
  if (ret != 0)
    abort ();

  pthread_t thread;
  ret = pthread_create (&thread, NULL, thread_func, argv[0]);
  if (ret != 0)
    abort ();

  pthread_barrier_wait (&barrier);

#ifdef LEADER_DOES_EXEC
  printf ("Leader going in exec.\n");
  do_the_exec ();
#endif

  pthread_join (thread, NULL);

  return 0;
}
