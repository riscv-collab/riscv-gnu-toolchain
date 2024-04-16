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

#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

static char *program_name;

void *
thread_execler (void *arg)
{
  /* Exec ourselves again, but with an extra argument, to avoid
     infinite recursion.  */
  if (execl (program_name, program_name, "1", NULL) == -1)
    {
      perror ("execl");
      abort ();
    }

  return NULL;
}

int
main (int argc, char **argv)
{
  pthread_t thread;

  if (argc > 1)
    {
      /* Getting here via execl.  */
      return 0;
    }

  program_name = argv[0];

  pthread_create (&thread, NULL, thread_execler, NULL);
  pthread_join (thread, NULL);

  return 0;
}
