/* Copyright 2023-2024 Free Software Foundation, Inc.

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

   This test is used to test the reverse-step and reverse-next instruction
   execution for a source line that contains multiple function calls.  */

void
func1 (void)
{
} /* END FUNC1 */

void
func2 (void)
{
} /* END FUNC2 */

int
main (void)
{
  int a, b;
  a = 1;
  b = 2;
  func1 (); func2 ();
  a = a + b;     /* START REVERSE TEST */
}
