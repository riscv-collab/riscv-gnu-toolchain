/* This testcase is part of GDB, the GNU debugger.

   Copyright 2018-2024 Free Software Foundation, Inc.

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

*/

static int
inc (int x)
{                /* inc.1 */
  return x + 1;  /* inc.2 */
}                /* inc.3 */

static int
thrice (int (*op)(int), int x)
{                 /* thrice.1 */
  x = op (x);     /* thrice.2 */
  x = op (x);     /* thrice.3 */
  return op (x);  /* thrice.4 */
}                 /* thrice.5 */

int
main ()
{
  int x;

  x = thrice (inc, 40);

  return x;
}
