/* This testcase is part of GDB, the GNU debugger.

   Copyright 2014-2024 Free Software Foundation, Inc.

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

int global = 3;

class C {
public:
  struct C1 {} C1;
  enum E1 {a1, b1, c1} E1;
  union U1 {int a1; char b1;} U1;

  C () : E1 (b1) {}
  void global (void) const {}
  int f (void) const { global (); return 0; }
} C;

struct S {} S;
enum E {a, b, c} E;
union U {int a; char b;} U;

class CC {} cc;
struct SS {} ss;
enum EE {ea, eb, ec} ee;
union UU {int aa; char bb;} uu;

int
main (void)
{
  return C.f ();
}
