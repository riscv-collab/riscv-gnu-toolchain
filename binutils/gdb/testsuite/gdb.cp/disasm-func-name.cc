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

class A
{
private:
  int m_i;

public:
  A(int i);

  int get_i () const
  { return m_i; }

  void set_i (int i)
  { m_i = i; }
};

A::A(int i)
  : m_i (i)
{ /* Nothing.  */ }

void process (A *obj, int num)
{
  obj->set_i (obj->get_i () + num);
}

int
main (void)
{
  A a(42);
  process (&a, 2);
  return a.get_i ();
}
