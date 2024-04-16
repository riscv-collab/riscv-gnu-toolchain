/* This testcase is part of GDB, the GNU debugger.

   Copyright 2010-2024 Free Software Foundation, Inc.

   Contributed by Pierre Muller.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Qualifiers of forward types are not resolved correctly with stabs.  */

struct dummy;

enum dummy_enum;

/* This function prevents the compiler from dropping local variables
   we need for the test.  */
void *hack (const struct dummy *t, const enum dummy_enum *e);

const void *
test (const struct dummy *t)
{
  const struct dummy *tt;
  enum dummy_enum *e;
  tt = t;
  return hack (t, e);
}

void *
test2 (struct dummy *t)
{
  struct dummy *tt;
  const enum dummy_enum *e;
  tt = t;
  return hack (t, e);
}


struct dummy {
 int x;
 int y;
 double b;
} tag_dummy;

enum dummy_enum {
  enum1,
  enum2
} tag_dummy_enum;

void *
hack (const struct dummy *t, const enum dummy_enum *e)
{
  return (void *) t;
}

int
main ()
{
  struct dummy tt;
  tt.x = 5;
  tt.y = 25;
  tt.b = 2.5;
  test2 (&tt);
  test (&tt);
  return 0;
}
