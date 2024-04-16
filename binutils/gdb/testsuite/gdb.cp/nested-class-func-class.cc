/* This testcase is part of GDB, the GNU debugger.

   Copyright 2021-2024 Free Software Foundation, Inc.

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

int
foo ()
{
  class F1
  {
  public:
    int a;
    int
    vvv ()
    {
      class F2
      {
	int f;
      };
      F2 abcd;
      return 1; /* BP 1 */
    }
  };

  F1 x;
  int a = x.vvv ();
  return a; /* BP 2 */
}

int
main ()
{
  return foo ();
}
