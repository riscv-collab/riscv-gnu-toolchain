/* Copyright (C) 2015-2024 Free Software Foundation, Inc.

   Contributed by Intel Corp. <walfred.tedeschi@intel.com>

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

#define OUR_SIZE    5

void
upper (int * p, int len)
{
  int value;
  len++;			/* b0-size-test.  */
  value = *(p + len);
}

int
main (void)
{
  int a = 0;			/* Dummy variable for debugging purposes.  */
  int sx[OUR_SIZE];
  a++;				/* register-eval.  */
  upper (sx, OUR_SIZE + 2);
  return sx[1];
}
