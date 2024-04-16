/* This testcase is part of GDB, the GNU debugger.
   Copyright 2012-2024 Free Software Foundation, Inc.

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

extern int get_version_1 (void);
extern int get_version_2 (void);

int
main (void)
{
  int v1 = get_version_1 ();
  int v2 = get_version_2 ();

  if (v1 != 104)
    return 1;

  /* The value returned by get_version_2 depends on the target.
     On GNU/Linux, for instance, it should return 104.  But on
     x86-windows, for instance, it will return 203.  */
  if (v2 != 104 && v2 != 203)
    return 2;

  return 0;
}

