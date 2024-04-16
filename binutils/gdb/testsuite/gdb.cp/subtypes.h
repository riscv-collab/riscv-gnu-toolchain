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

/* A template that defines subtypes.  */

template <typename T>
T foobar (int arg)
{
  struct Foo
  {
    T doit (void) { return 1; }
  } foo;

  struct Bar
  {
    T doit (void) { return 2; }
  } bar;

  struct Baz
  {
    T doit (void) { return 3; }
  } baz;

  return arg - foo.doit () - bar.doit () - baz.doit ();
}

/* A structure that defines other types.  */

struct Outer
{
  enum class Oenum { OA, OB, OC, OD };
  struct Inner;
  Inner *p;
  Oenum e;
  Outer ();
};
