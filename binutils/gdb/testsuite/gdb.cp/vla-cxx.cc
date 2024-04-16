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

struct container;

struct element
{
  container &c;

  element(container &cc) : c (cc) { }
};

struct container
{
  element e;

  container() : e(*this) { }
};

int main(int argc, char **argv)
{
  int z = 3;
  // Note that this is a GNU extension.
  int vla[z];
  typeof (vla) &vlaref (vla);
  typedef typeof (vla) &vlareftypedef;
  vlareftypedef vlaref2 (vla);
  container c;

  for (int i = 0; i < z; ++i)
    vla[i] = 5 + 2 * i;

  // vlas_filled
  vla[0] = 2 * vla[0];
  return vla[2];
}
