/* This testcase is part of GDB, the GNU debugger.

   Copyright 2018-2024 Free Software Foundation, Inc.

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

#include "subtypes.h"

int
main (int argc, char *argv[])
{
  struct Foo
  {
    int doit (void) { return 1111; }
  } foo;

  struct Bar
  {
    int doit (void) { return 2222; }
  } bar;

  struct Baz
  {
    int doit (void) { return 3333; }
  } baz;
  Outer o;
  o.e = Outer::Oenum::OA;

  return foo.doit () + bar.doit () + baz.doit () + foobar<int> (6)
    + foobar<char> ('c');
}
