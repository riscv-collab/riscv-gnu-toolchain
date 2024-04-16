/* This testcase is part of GDB, the GNU debugger.

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

#define SIZE 5

void
foo (int n, int vla_ptr[n])
{
  return;         /* foo_bp */
}

void
bar (int *vla_ptr)
{
  return;         /* bar_bp */
}

void
vla_func (int n)
{
  int vla[n];
  typedef int typedef_vla[n];
  typedef_vla td_vla;
  int i;

  for (i = 0; i < n; i++)
    {
      vla[i] = 2+i;
      td_vla[i] = 4+i;
    }

  foo(n, vla);
  bar(vla);

  return;         /* vla_func_bp */
}

int
main (void)
{
  vla_func(SIZE);

  return 0;
}
