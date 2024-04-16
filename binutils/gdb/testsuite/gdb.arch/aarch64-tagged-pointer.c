/* This file is part of GDB, the GNU debugger.

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

#include <stdint.h>

struct s
{
  int i;
};

static void
foo (void)
{
}

int
main (void)
{
  struct s s1;
  struct s *sp1, *sp2;
  int i = 1234;
  int *p1, *p2;

  s1.i = 1234;
  sp1 = &s1;
  p1 = &i;
  /* SP1 and SP2 have different tags, but point to the same address.  */
  sp2 = (struct s *) ((uintptr_t) sp1 | 0xf000000000000000ULL);
  p2 = (int *) ((uintptr_t) p1 | 0xf000000000000000ULL);

  void (*func_ptr) (void) = foo;
  func_ptr = (void (*) (void)) ((uintptr_t) func_ptr | 0xf000000000000000ULL);
  sp2->i = 4321; /* breakpoint here.  */

  for (int i = 0; i < 2; i++)
    {
      foo ();
      (*func_ptr) ();
    }

  sp1->i = 8765;
  sp2->i = 4321;
  sp1->i = 8765;
  sp2->i = 4321;
  *p1 = 1;
  *p2 = 2;
  *p1 = 1;
  *p2 = 2;
}
