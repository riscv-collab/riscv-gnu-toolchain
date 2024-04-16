/* This testcase is part of GDB, the GNU debugger.

   Copyright 2009-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Please email any bugs, comments, and/or additions to this file to:
   bug-gdb@gnu.org  */

/* This source is mainly to test what happens when a watchpoint is
   removed while another watchpoint, inserted later is left active.  */

int count = -1;
int ival1 = -1;
int ival2 = -1;
int ival3 = -1;
int ival4 = -1;

int 
main ()
{
  for (count = 0; count < 4; count++) {
    ival1 = count; ival2 = count;
    ival3 = count; ival4 = count;
  }

  ival1 = count; ival2 = count;  /* Outside loop */
  ival3 = count; ival4 = count;

  return 0;
}
