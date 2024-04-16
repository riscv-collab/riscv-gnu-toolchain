/* This testcase is part of GDB, the GNU debugger.

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

struct inner
{
  int a;
};

struct outer
{
  struct inner *inner;
};

int main (void)
{
  struct inner inner;
  struct outer outer;
  struct outer *p_outer;

  inner.a = 42;
  outer.inner = &inner;

  /* We force p_outer to an invalid value, but this also happens naturally
   * when a variable has not been initialized. */

  p_outer = 0;
  /* p_outer set to invalid value */
  p_outer = &outer;
  /* p_outer set to valid value */

  return 0;
}
