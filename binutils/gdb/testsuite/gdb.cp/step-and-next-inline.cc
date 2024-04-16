/* This testcase is part of GDB, the GNU debugger.

   Copyright 2019-2024 Free Software Foundation, Inc.

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

#ifdef USE_NEXT_INLINE_H

#include "step-and-next-inline.h"

#else	/* USE_NEXT_INLINE_H */

/* The code below must remain identical to the code in
   step-and-next-inline.h.  */

#include <stdlib.h>

struct tree
{
  volatile int x;
  volatile int z;
};

#define TREE_TYPE(NODE) (*tree_check (NODE, 0))

inline tree *
tree_check (tree *t, int i)
{
  if (t->x != i)
    abort();
  tree *x = t;
  return x;
} // tree-check

#endif	/* USE_NEXT_INLINE_H */

int __attribute__((noinline, noclone))
get_alias_set (tree *t)
{
  if (t != NULL
      && TREE_TYPE (t).z != 1
      && TREE_TYPE (t).z != 2
      && TREE_TYPE (t).z != 3)
    return 0;
  return 1;
} // get_alias_set

tree xx;

int
main()
{
  get_alias_set (&xx);  /* Beginning of main */
  return 0;
} // main
