/* This testcase is part of GDB, the GNU debugger.

   Copyright 2020-2024 Free Software Foundation, Inc.

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

struct no_size
{
  int n;
  int items[];
};

struct zero_size
{
  int n;
  int items[0];
};

struct zero_size_only
{
  int items[0];
};

struct no_size *ns;
struct zero_size *zs;
struct zero_size_only *zso;

static void
break_here (void)
{
}

int
main (void)
{
  ns = (struct no_size *) malloc (sizeof (*ns) + 3 * sizeof (int));
  zs = (struct zero_size *) malloc (sizeof (*zs) + 3 * sizeof (int));
  zso = (struct zero_size_only *) malloc (sizeof (*zso) + 3 * sizeof (int));

  ns->n = 3;
  ns->items[0] = 101;
  ns->items[1] = 102;
  ns->items[2] = 103;

  zs->n = 3;
  zs->items[0] = 201;
  zs->items[1] = 202;
  zs->items[2] = 203;

  zso->items[0] = 301;
  zso->items[1] = 302;
  zso->items[2] = 303;

  break_here ();

  return 0;
}
