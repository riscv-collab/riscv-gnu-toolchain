/* Test program for MPX map allocated bounds.

   Copyright 2015-2024 Free Software Foundation, Inc.

   Contributed by Intel Corp. <walfred.tedeschi@intel.com>
			      <mircea.gherzan@intel.com>

   This file is part of GDB.

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

#include <stdlib.h>
#define SIZE  5

typedef int T;

void
foo (T *p)
{
  T *x;

#if defined  __GNUC__ && !defined __INTEL_COMPILER
  __bnd_store_ptr_bounds (p, &p);
#endif

  x = p + SIZE - 1;

#if defined  __GNUC__ && !defined __INTEL_COMPILER
  __bnd_store_ptr_bounds (x, &x);
#endif
  /* Dummy assign.  */
  x = x + 1;			/* after-assign */
  return;
}

int
main (void)
{
  T *a = NULL;

  a = calloc (SIZE, sizeof (T));	/* after-decl */
#if defined  __GNUC__ && !defined __INTEL_COMPILER
  __bnd_store_ptr_bounds (a, &a);
#endif

  foo (a);				/* after-alloc */
  free (a);

  return 0;
}
