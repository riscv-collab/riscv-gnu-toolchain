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

#include <pthread.h>
#include <assert.h>
#include <unistd.h>

static void *
start (void *arg)
{
  assert (0);
  return arg;
}

int main(void)
{
  pthread_t thread;
  int i;

  switch (fork ())
    {
    case -1:
      assert (0);
    default:
      break;
    case 0:
      i = pthread_create (&thread, NULL, start, NULL);
      assert (i == 0);
      i = pthread_join (thread, NULL);
      assert (i == 0);

      assert (0);
    }

  return 0;
}
