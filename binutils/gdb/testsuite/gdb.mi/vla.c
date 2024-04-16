/* This testcase is part of GDB, the GNU debugger.

   Contributed by Intel Corp. <keven.boell@intel.com>

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

int
func (int n)
{
  int vla[n], i;

  for (i = 0; i < n; i++)
    vla[i] = i;

  return n;                 /* vla-filled */
}

int
main (void)
{
  func (5);

  return 0;
}
