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


/* One could use unique_ptr instead, but that requires a GCC which can
   support "-std=c++11".  */

int
func (int i, int j)
{
  return i + j;
}

class A
{
public:
  A () { a = 12345; f = &func; }
  int geta ();
  int adda (int i);

  int a;
  int (*f) (int, int);
};

int
A::geta ()
{
  return a;
}

int
A::adda (int i)
{
  return a + i;
}

int
main ()
{
  A a;
  A *a_ptr = &a;
  int (A::*m1) ();
  int (A::*m2) (int);

  m1 = &A::geta;
  m2 = &A::adda;

  return (a.*m1) () + (a.*m2) (12) + (a.*(&A::f)) (1, 2);  /* Break here  */
}
