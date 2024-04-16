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

void
fun1 (void)
{		/* fun1.1 */
}		/* fun1.2 */

void
fun2 (void)
{		/* fun2.1 */
  fun1 ();	/* fun2.2 */
}		/* fun2.3 */

void
fun3 (void)
{		/* fun3.1 */
  fun1 ();	/* fun3.2 */
  fun2 ();	/* fun3.3 */
}		/* fun3.4 */

void
fun4 (void)
{		/* fun4.1 */
  fun1 ();	/* fun4.2 */
  fun2 ();	/* fun4.3 */
  fun3 ();	/* fun4.4 */
}		/* fun4.5 */

int
main (void)
{		/* main.1 */
  fun4 ();	/* main.2 */
  return 0;	/* main.3 */
}		/* main.4 */
