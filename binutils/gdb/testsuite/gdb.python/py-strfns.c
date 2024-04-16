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

const char str1[] = "Hello.";
const char str2[] = "Hello.";
const char str3[] = "Goodbye.";

const char buf1[] = { 0, 1, 2, 3 };
const char buf2[] = { 0, 1, 2, 3 };
const char buf3[] = { 0, 1, 2, 4 };

static void
func (const char *arg)
{
  return; /* Break func here.  */
}

static void
bfunc (const char *arg)
{
  return; /* Break bfunc here.  */
}

int
main ()
{
  func (str1);
  func (str2);
  func (str3);

  bfunc (buf1);
  bfunc (buf2);
  bfunc (buf3);

  return 0;
}
