/* This testcase is part of GDB, the GNU debugger.

   Copyright 2008-2024 Free Software Foundation, Inc.

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

/* Test gdb's "return" command in reverse.  The code for this test is based
   on the code in test finish-reverse.c.  The code was modified to call the
   function via a function pointer so the test will behave the same on all
   platforms.  See comments in finish-reverse-bkpt.exp.  */

int void_test = 0;

void
void_func ()
{
  void_test = 1;		/* VOID FUNC */
}

int
main (int argc, char **argv)
{
  int i;
  void (*funp) (void) = void_func;

  funp ();
  return 0;
}
