/* This testcase is part of GDB, the GNU debugger.

   Copyright 2019-2024 Free Software Foundation, Inc.

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

/* Using the DWARF assembler - see imported-unit.exp - we'll be constructing
   debug info corresponding to the following C++ code that also might have
   been compiled using -flto...

  int main()
  {
    class Foo {
    public:
      int doit ()
      {
	return 0;
      }
    };

    Foo foo;

    return foo.doit ();
  }

  An attempt was made to try to use the above code directly, but
  finding the start and end address of doit turned out to be
  difficult.
*/


int doit (void)
{
  asm ("doit_label: .globl doit_label");

  return 0;
}

int
main (int argc, char *argv[])
{
  asm ("main_label: .globl main_label");

  return doit ();
}
