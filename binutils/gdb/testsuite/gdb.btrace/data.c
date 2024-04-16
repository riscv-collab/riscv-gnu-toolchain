/* This testcase is part of GDB, the GNU debugger.

   Copyright 2013-2024 Free Software Foundation, Inc.

   Contributed by Intel Corp. <markus.t.metzger@intel.com>

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

volatile static int glob;

void
test (void)
{		/* test.1 */
  volatile static int loc;

  loc += 1;	/* test.2 */
  glob += loc;	/* test.3 */
}		/* test.4 */

int
main (void)
{		/* main.1 */
  test ();	/* main.2 */
  return 0;	/* main.3 */
}		/* main.4 */
