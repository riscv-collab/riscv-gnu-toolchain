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

/* This is the original source for namelessclass.S.  This file is never
   compiled by the test suite, since the assembler output of clang++,
   namelessclass.S, is used instead.  */

class A
{
public:
  A () : a_ (0xbeef) {}
  int doit (void) {
    int ret = fudge ([this] () {
	return a_; // set breakpoint here
      });

    return ret;
  }

private:
  template <typename Func>
  int fudge (Func func) { return func (); }
  int a_;
};

int
main (void)
{
  A a;

  return a.doit ();
}
