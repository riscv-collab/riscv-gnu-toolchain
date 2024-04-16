/* Copyright 2016-2024 Free Software Foundation, Inc.

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

static void
fun_three (int a, char b, void *c)
{
  /* Do nothing.  */
}

static void
fun_two (unsigned int p, const char *y)
{
  fun_three ((int) p, '1', (void *) y);
}

static void
fun_one (int *x)
{
  fun_two (10, (const char *) x);
}

int
main (void)
{
  int a = 10;

  fun_one (&a);
  return 0;
}
