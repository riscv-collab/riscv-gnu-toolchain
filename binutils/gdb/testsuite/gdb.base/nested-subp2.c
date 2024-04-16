/* This test program is part of GDB, the GNU debugger.

   Copyright 2015-2024 Free Software Foundation, Inc.

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
iter_str (const char *str, void (*callback) (char c))
{
  for (; *str != '\0'; ++str)
    callback (*str);
}

int
length_str (const char *str)
{
  int count = 0;

  void
  increment (char c)
  {
    /* Here with COUNT, we can test that GDB can read a non-local variable even
       though it's not directly in the upper stack frame.  */
    count += 1; /* STOP */
  }

  iter_str (str, &increment);
  return count;
}

int
main ()
{
  if (length_str ("foo") == 3)
    return 0;
  return 1;
}
