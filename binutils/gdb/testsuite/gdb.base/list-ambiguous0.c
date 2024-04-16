/* This testcase is part of GDB, the GNU debugger.

   Copyright 2017-2024 Free Software Foundation, Inc.

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





/* These symbols are defined in both
   list-ambiguous0.c/list-ambiguous1.c files, in order to test
   "list"'s behavior with ambiguous linespecs.  */

static void __attribute__ ((used)) ambiguous_fun (void) {}

static int __attribute__ ((used)) ambiguous_var;









int
main (void)
{
  return 0;
}
