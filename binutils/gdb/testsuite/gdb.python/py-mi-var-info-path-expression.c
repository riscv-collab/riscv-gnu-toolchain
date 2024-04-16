/* This testcase is part of GDB, the GNU debugger.

   Copyright (C) 2018-2024 Free Software Foundation, Inc.

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

enum cons_type
{
  type_atom = 0,
  type_cons = 1
};

struct atom
{
  int ival;
};

struct cons
{
  enum cons_type type;
  union
  {
    struct atom atom;
    struct cons *slots[2];
  };
};

#define nil ((struct cons*)0);

int
main ()
{
  struct cons c1, c2, c3, c4;

  c1.type = type_cons;
  c1.slots[0] = &c4;
  c1.slots[1] = &c2;

  c2.type = type_cons;
  c2.slots[0] = nil;
  c2.slots[1] = &c3;

  c3.type = type_cons;
  c3.slots[0] = nil;
  c3.slots[1] = nil;

  c4.type = type_atom;
  c4.atom.ival = 13;

  return 0;			/* next line */
}
