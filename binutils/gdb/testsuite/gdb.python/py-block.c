/* This testcase is part of GDB, the GNU debugger.

   Copyright 2010-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see  <http://www.gnu.org/licenses/>.  */

int block_func (void)
{
  int i = 0;
  {
    double i = 1.0;
    double f = 2.0;
    {
      const char *i = "stuff";
      const char *f = "foo";
      const char *b = "bar";
      return 0; /* Block break here.  */
    }
  }
}

/* A function with no locals.  Used for testing gdb.Block.__repr__().  */
int no_locals_func (void)
{
  return block_func ();
}

/* A function with 5 locals.  Used for testing gdb.Block.__repr__().  */
int few_locals_func (void)
{
  int i = 0;
  int j = 0;
  int k = 0;
  int x = 0;
  int y = 0;
  return block_func ();
}

/* A function with 6 locals.  Used for testing gdb.Block.__repr__().  */
int many_locals_func (void)
{
  int i = 0;
  int j = 0;
  int k = 0;
  int x = 0;
  int y = 0;
  int z = 0;
  return block_func ();
}

int main (int argc, char *argv[])
{
  block_func ();
  no_locals_func ();
  few_locals_func ();
  many_locals_func ();
  return 0; /* Break at end. */
}
