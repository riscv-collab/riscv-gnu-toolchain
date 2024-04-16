/* Copyright (C) 2010-2024 Free Software Foundation, Inc.

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

/* The original program corresponding to implptr.S.
   This came from Jakub's gcc-patches email implementing
   DW_OP_GNU_implicit_pointer.
   Note that it is not ever compiled, implptr.S is used instead.
   However, it is used to extract breakpoint line numbers.  */

struct S
{
  int *x, y;
};

int u[6];

static inline void
add (struct S *a, struct S *b, int c)
{
  *a->x += *b->x;		/* baz breakpoint */
  a->y += b->y;
  u[c + 0]++;
  a = (struct S *) 0;
  u[c + 1]++;
  a = b;
  u[c + 2]++;
}

int
foo (int i)
{
  int j = i;
  struct S p[2] = { {&i, i * 2}, {&j, j * 2} };
  add (&p[0], &p[1], 0);
  p[0].x = &j;
  p[1].x = &i;
  add (&p[0], &p[1], 3);
  return i + j;			/* foo breakpoint */
}

typedef int *intp;
typedef intp *intpp;
typedef intpp *intppp;

int __attribute__ ((noinline, used, noclone)) 
bar (int i) 
{
  intp j = &i;
  intpp k = &j;
  intppp l = &k;
  i++;				/* bar breakpoint */
  return i;
}

int main ()
{
  return bar(5) + foo (23);
}
