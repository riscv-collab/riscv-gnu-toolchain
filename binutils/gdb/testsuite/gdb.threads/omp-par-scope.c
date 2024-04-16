/* This testcase is part of GDB, the GNU debugger.

   Copyright 2017-2024 Free Software Foundation, Inc.

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

#include <stdio.h>
#include <omp.h>

omp_lock_t lock;
omp_lock_t lock2;

/* Enforce execution order between two threads using a lock.  */

static void
omp_set_lock_in_order (int num, omp_lock_t *lock)
{
  /* Ensure that thread num 0 first sets the lock.  */
  if (num == 0)
    omp_set_lock (lock);
  #pragma omp barrier

  /* Block thread num 1 until it can set the lock.  */
  if (num == 1)
    omp_set_lock (lock);

  /* This bit here is guaranteed to be executed first by thread num 0, and
     once thread num 0 unsets the lock, to be executed by thread num 1.  */
  ;
}

/* Testcase for checking access to variables in a single / outer scope.
   Make sure that variables not referred to in the parallel section are
   accessible from the debugger.  */

void
single_scope (void)
{
  static int s1 = -41, s2 = -42, s3 = -43;
  int i1 = 11, i2 = 12, i3 = 13;

#pragma omp parallel num_threads (2) shared (s1, i1) private (s2, i2)
  {
    int thread_num = omp_get_thread_num ();
    omp_set_lock_in_order (thread_num, &lock);

    s2 = 100 * (thread_num + 1) + 2;
    i2 = s2 + 10;

    #pragma omp critical
    printf ("single_scope: thread_num=%d, s1=%d, i1=%d, s2=%d, i2=%d\n",
	    thread_num, s1, i1, s2, i2);

    omp_unset_lock (&lock);
  }

  printf ("single_scope: s1=%d, s2=%d, s3=%d, i1=%d, i2=%d, i3=%d\n",
	  s1, s2, s3, i1, i2, i3);
}

static int file_scope_var = 9876;

/* Testcase for checking access to variables from parallel region
   nested within more than one lexical scope.  Of particular interest
   are variables which are not referenced in the parallel section.  */

void
multi_scope (void)
{
  int i01 = 1, i02 = 2;

  {
    int i11 = 11, i12 = 12;

    {
      int i21 = -21, i22 = 22;

#pragma omp parallel num_threads (2) \
		     firstprivate (i01) \
		     shared (i11) \
		     private (i21)
	{
	  int thread_num = omp_get_thread_num ();
	  omp_set_lock_in_order (thread_num, &lock);

	  i21 = 100 * (thread_num + 1) + 21;

	  #pragma omp critical
	  printf ("multi_scope: thread_num=%d, i01=%d, i11=%d, i21=%d\n",
		  thread_num, i01, i11, i21);

	  omp_unset_lock (&lock);
	}

	printf ("multi_scope: i01=%d, i02=%d, i11=%d, "
		"i12=%d, i21=%d, i22=%d\n",
		i01, i02, i11, i12, i21, i22);
    }
  }
}

/* Nested functions in C is a GNU extension.  Some non-GNU compilers
   define __GNUC__, but they don't support nested functions.  So,
   unfortunately, we can't use that for our test.  */
#if HAVE_NESTED_FUNCTION_SUPPORT

/* Testcase for checking access of variables from within parallel
   region in a lexically nested function.  */

void
nested_func (void)
{
  static int s1 = -42;
  int i = 1, j = 2, k = 3;

  void
  foo (int p, int q, int r)
  {
    int x = 4;

    {
      int y = 5, z = 6;
#pragma omp parallel num_threads (2) shared (i, p, x) private (j, q, y)
      {
	int tn = omp_get_thread_num ();
	omp_set_lock_in_order (tn, &lock);

	j = 1000 * (tn + 1);
	q = j + 1;
	y = q + 1;
	#pragma omp critical
	printf ("nested_func: tn=%d: i=%d, p=%d, x=%d, j=%d, q=%d, y=%d\n",
		 tn, i, p, x, j, q, y);

	omp_unset_lock (&lock);
      }
    }
  }

  foo (10, 11, 12);

  i = 101; j = 102; k = 103;
  foo (20, 21, 22);
}
#endif

/* Testcase for checking access to variables from within a nested parallel
   region. */

void
nested_parallel (void)
{
  int i = 1, j = 2;
  int l = -1;

  omp_set_nested (1);
  omp_set_dynamic (0);
#pragma omp parallel num_threads (2) private (l)
  {
    int num = omp_get_thread_num ();
    omp_set_lock_in_order (num, &lock);

    int nthr = omp_get_num_threads ();
    int off = num * nthr;
    int k = off + 101;
    l = off + 102;
#pragma omp parallel num_threads (2) shared (num)
    {
      int inner_num = omp_get_thread_num ();
      omp_set_lock_in_order (inner_num, &lock2);

      #pragma omp critical
      printf ("nested_parallel (inner threads): outer thread num = %d, thread num = %d\n", num, inner_num);

      omp_unset_lock (&lock2);
    }
    #pragma omp critical
    printf ("nested_parallel (outer threads) %d: k = %d, l = %d\n", num, k, l);

    omp_unset_lock (&lock);
  }
}

int
main (int argc, char **argv)
{
  omp_init_lock (&lock);
  omp_init_lock (&lock2);

  single_scope ();
  multi_scope ();
#if HAVE_NESTED_FUNCTION_SUPPORT
  nested_func ();
#endif
  nested_parallel ();

  omp_destroy_lock (&lock);
  omp_destroy_lock (&lock2);

  return 0;
}

