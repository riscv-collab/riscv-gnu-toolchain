/* This testcase is part of GDB, the GNU debugger.

   Copyright 2012-2024 Free Software Foundation, Inc.

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
#include "trace-common.h"

static void *
thread_function(void *arg)
{
  FAST_TRACEPOINT_LABEL(set_point1);
}

static void
end (void)
{}

int
main (int argc, char *argv[], char *envp[])
{
  pthread_t threads[2];
  int i;

  for (i = 0; i < 2; i++)
    pthread_create (&threads[i], NULL, thread_function, NULL);

  for (i = 0; i < 2; i++)
    pthread_join (threads[i], NULL);

  end ();

  return 0;
}
