/* Code to go along with tests in breakpoint.exp.
   
   Copyright 2004-2024 Free Software Foundation, Inc.

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

int g = 0;

class C1 {
public:
  C1(int i) : i_(i) {}

  int foo ()
  {
    return 1; // conditional breakpoint in method
  }

  void bar ()
  {
    for (int i = 0; i < 1; ++i)
      {
	int t = i * 2;
	g += t; // conditional breakpoint in method 2
      }
  }

  class Nested {
  public:
    int
    foo ()
    {
      return 1;
    }
  };

private:
  int i_;
};

int main ()
{
  C1::Nested c1;

  c1.foo ();
  
  C1 c2 (2), c3 (3);
  c2.foo ();
  c2.bar ();
  c3.foo ();
  c3.bar ();

  return 0;
}
