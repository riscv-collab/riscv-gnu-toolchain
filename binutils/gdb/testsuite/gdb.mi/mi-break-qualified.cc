/* Copyright 2020-2024 Free Software Foundation, Inc.

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

namespace NS {

int
func (int i)
{
  return i; /* location NS::func here */
}

} /* namespace NS */

struct foo
{
  long func (long l);
};

long
foo::func (long l)
{
  return l; /* location foo::func here */
}

char
func (char c)
{
  return c; /* location func here */
}

int
main ()
{
  foo f;
  f.func (1);
  NS::func (2);
  func (3);
  return 0;
}
