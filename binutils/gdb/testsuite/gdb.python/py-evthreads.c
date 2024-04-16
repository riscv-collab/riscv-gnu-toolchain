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
   along with this program.  If not, see  <http://www.gnu.org/licenses/>.  */

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

pthread_t thread2_id;
pthread_t thread3_id;

void* thread3 (void* d)
{
  raise (SIGUSR1);

  return NULL;
}

void* thread2 (void* d)
{
  /* Do not quit thread3 asynchronously wrt thread2 stop - wait first on
     thread3_id to stop.  It would complicate testcase reception of the
     events.  */

  pthread_create (&thread3_id, NULL, thread3, NULL); pthread_join (thread3_id, NULL);

  return NULL;
}

int main (void)
{
  /* Use single line to not to race whether `thread2' breakpoint or `next' over
     pthread_create will stop first.  */

  pthread_create (&thread2_id, NULL, thread2, NULL); pthread_join (thread2_id, NULL);

  return 12;
}
