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

struct A
{
  virtual int doit () { return 1; }
  virtual int doit3 () { return -3; }
  virtual int doit2 () = 0;
};

struct B : virtual A
{
  int doit () { return 2; }
  int doit2 () { return 22; }
};

struct C : virtual A
{
  int doit () { return 3; }
  int doit2 () { return 33; }
};

struct D : B, C
{
  int doit () { return 4; }
  int doit2 () { return 44; }
};

int
main ()
{
  int var = 1234;
  B b;
  C c;
  D d;
  A *ap = &d;

  var = (b.doit () + c.doit () + d.doit () + d.doit3 ()
	 + ap->doit () + ap->doit2 ());	// break here

  return 0;
}
