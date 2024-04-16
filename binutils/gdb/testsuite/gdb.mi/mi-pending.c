/* This testcase is part of GDB, the GNU debugger.

   Copyright 2007-2024 Free Software Foundation, Inc.

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

#include <dlfcn.h>
#include <pthread.h>

#define NUM 2

extern void pendfunc (int x);

void*
thread_func (void* arg)
{
  const char *libname = "mi-pendshr2.sl";
  void *h;
  int (*p_func) ();

  h = dlopen (libname, RTLD_LAZY);  /* set breakpoint here */
  if (h == NULL)
    return NULL;

  p_func = dlsym (h, "pendfunc3");
  if (p_func == NULL)
    return NULL;

  (*p_func) ();
}

int main()
{
  int res;
  pthread_t threads[NUM];
  int i;

  pendfunc (3);
  pendfunc (4);

  for (i = 0; i < NUM; i++)
    {
      res = pthread_create (&threads[i],
			     NULL,
			     &thread_func,
			     NULL);
    }

  for (i = 0; i < NUM; i++) {
    res = pthread_join (threads[i], NULL);
  }

  return 0;
}
