/* Copyright (C) 2007-2024 Free Software Foundation, Inc.

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

   This file was written by Chris Demetriou (cgd@google.com).  */

/* Simple test to trigger thread events (thread start, thread exit).  */

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

static void *
threadfunc (void *arg)
{
  printf ("in threadfunc\n");
  return NULL;
}

static void
after_join_func (void)
{
  printf ("finished\n");
}

int main (int argc, char *argv[])
{
  pthread_t thread;

  if (pthread_create (&thread, NULL, threadfunc, NULL) != 0)
    {
      printf ("pthread_create failed\n");
      exit (1);
    }

  if (pthread_join (thread, NULL) != 0)
    {
      printf ("pthread_join failed\n");
      exit (1);
    }

  after_join_func ();
  return 0;
}
