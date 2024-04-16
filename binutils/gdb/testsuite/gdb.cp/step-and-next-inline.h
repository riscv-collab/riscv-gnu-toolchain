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

/* The code below must remain identical to the block of code in
   step-and-next-inline.cc.  */

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
} // tree_check
