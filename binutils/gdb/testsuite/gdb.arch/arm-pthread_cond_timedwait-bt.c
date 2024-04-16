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
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

static void *fun(void *arg)
{
  struct timeval now;
  struct timespec until;
  int err;

  err = gettimeofday(&now, NULL);
  assert(!err);

  until.tv_sec = now.tv_sec + 60;
  until.tv_nsec = now.tv_usec * 1000UL;

  pthread_cond_timedwait(&cond, &mutex, &until);
  assert(0);
  err = pthread_mutex_unlock(&mutex);
  assert(!err);

  return arg;
}

void breakhere()
{
}

int main()
{
  pthread_t thread;
  void *ret;
  int err;

  err = pthread_mutex_lock(&mutex);
  assert(!err);
  err = pthread_create(&thread, NULL, fun, NULL);
  assert(!err);
  err = pthread_mutex_lock(&mutex);
  assert(!err);
  breakhere();
  err = pthread_join(thread, &ret);
  assert(0);

  return 0;
}
