/* This testcase is part of GDB, the GNU debugger.

   Copyright 2010-2024 Free Software Foundation, Inc.

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

#include <string.h>

struct function_lookup_test
{
  int x,y;
};

void
init_flt (struct function_lookup_test *p, int x, int y)
{
  p->x = x;
  p->y = y;
}

struct s
{
  int a;
  int *b;
};

void
init_s (struct s *s, int a)
{
  s->a = a;
  s->b = &s->a;
}

int
main ()
{
  struct function_lookup_test flt;
  struct s s;

  init_flt (&flt, 42, 43);
  init_s (&s, 1);
  
  return 0;      /* break to inspect */
}
