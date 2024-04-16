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

#include <stddef.h>
#define SIZE 10

int
main (void)
{
  int n = SIZE;
  int i = 0;
  int j = 0;
  int vla2[SIZE][n];
  int vla1[n];

  for (i = 0; i < n; i++)
    vla1[i] = (i * 2) + n;

  for (i = 0; i < SIZE; i++)
    for (j = 0; j < n; j++)
      vla2[i][j] = (i + j) + n;


  i = 0;
  j = 0;

  return 0;           /* vla-filled */
}
