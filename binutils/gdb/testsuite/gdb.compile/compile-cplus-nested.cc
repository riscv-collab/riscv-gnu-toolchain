/* Copyright 2015-2024 Free Software Foundation, Inc.

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

class A
{
public:
  A () : a_ (1) {}
  int get ();

protected:
  int a_;

private:
  /* It is important to not /not/ use the nested class definition in A.
     This exercises a different path through the code.  */
  struct Inner1
  {
    int a_;
    Inner1 () : a_ (2) {}

    struct Inner2
    {
      int a_;
      Inner2 () : a_ (3) {}
    };
  };
};

int
A::get ()
{
  A::Inner1 i1;
  A::Inner1::Inner2 i2;

  return i1.a_ + i2.a_; // break here
}

int var = 1234;

int
main ()
{
  A a;

  return a.get ();
}
