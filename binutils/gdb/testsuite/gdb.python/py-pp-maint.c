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


enum flag_enum
  {
    /* Define the enumeration values in an unsorted manner to verify that we
       effectively sort them by value.  */
    FOO_MASK = 0x07,
    FOO_1    = 0x01,
    FOO_2    = 0x02,
    FOO_3    = 0x04,

    BAR_MASK = 0x70,
    BAR_1    = 0x10,
    BAR_2    = 0x20,
    BAR_3    = 0x40,
  };

enum flag_enum fval;

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

struct ss
{
  struct s a;
  struct s b;
};

void
init_s (struct s *s, int a)
{
  s->a = a;
  s->b = &s->a;
}

void
init_ss (struct ss *s, int a, int b)
{
  init_s (&s->a, a);
  init_s (&s->b, b);
}

int
main ()
{
  struct function_lookup_test flt;
  struct ss ss;

  init_flt (&flt, 42, 43);
  init_ss (&ss, 1, 2);
  
  return 0;      /* break to inspect */
}
