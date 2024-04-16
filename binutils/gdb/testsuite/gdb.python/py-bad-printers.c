/* This testcase is part of GDB, the GNU debugger.

   Copyright 2008-2024 Free Software Foundation, Inc.

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

/* This lets us avoid malloc.  */
int array[100];

struct container
{
  const char *name;
  int len;
  int *elements;
};

struct container
make_container (const char *name)
{
  struct container result;

  result.name = name;
  result.len = 0;
  result.elements = 0;

  return result;
}

void
add_item (struct container *c, int val)
{
  if (c->len == 0)
    c->elements = array;
  c->elements[c->len] = val;
  ++c->len;
}

int
main ()
{
  struct container c = make_container ("foo");

  add_item (&c, 23);

  return 0; /* break here */
}
