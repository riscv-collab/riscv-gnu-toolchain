/* This testcase is part of GDB, the GNU debugger.

   Copyright 2021-2024 Free Software Foundation, Inc.

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

#include <assert.h>
#include <pthread.h>

static void *
thread_func (void *p)
{
  return NULL;
}

int
main (void)
{
  const int nthreads = 10;
  pthread_t threads[nthreads];

  for (int i = 0; i < nthreads; ++i)
    {
      int ret = pthread_create (&threads[i], NULL, thread_func, NULL);
      assert (ret == 0);
    }

  for (int i = 0; i < nthreads; ++i)
    {
      int ret = pthread_join (threads[i], NULL);
      assert (ret == 0);
    }

  return 0;
}
