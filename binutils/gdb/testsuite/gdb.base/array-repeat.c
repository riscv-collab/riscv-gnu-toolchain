/* Copyright 2022-2024 Free Software Foundation, Inc.

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

#include <stdio.h>

int
main (void)
{
  int array_1d[5];
  int array_1d9[6];
  int array_2d[5][5];
  int array_2d9[6][6];
  int array_3d[5][5][5];
  int array_3d9[6][6][6];
  int i;

  for (i = 0; i < sizeof (array_1d) / sizeof (int); i++)
    *(array_1d + i) = 1;
  for (i = 0; i < sizeof (array_1d9) / sizeof (int); i++)
    *(array_1d9 + i) = i % 6 == 5 ? 9 : 1;
  for (i = 0; i < sizeof (array_2d) / sizeof (int); i++)
    *(*array_2d + i) = 2;
  for (i = 0; i < sizeof (array_2d9) / sizeof (int); i++)
    *(*array_2d9 + i) = i / 6 == 5 || i % 6 == 5 ? 9 : 2;
  for (i = 0; i < sizeof (array_3d) / sizeof (int); i++)
    *(**array_3d + i) = 3;
  for (i = 0; i < sizeof (array_3d9) / sizeof (int); i++)
    *(**array_3d9 + i) = i / 6 / 6 == 5 || i / 6 % 6 == 5 || i % 6 == 5 ? 9 : 3;

  printf("\n");						/* Break here */
  for (i = 0; i < sizeof (array_1d) / sizeof (int); i++)
    printf(" %d", *(array_1d + i));
  printf("\n");
  for (i = 0; i < sizeof (array_1d9) / sizeof (int); i++)
    printf(" %d", *(array_1d9 + i));
  printf("\n");
  for (i = 0; i < sizeof (array_2d) / sizeof (int); i++)
    printf(" %d", *(*array_2d + i));
  printf("\n");
  for (i = 0; i < sizeof (array_2d9) / sizeof (int); i++)
    printf(" %d", *(*array_2d9 + i));
  printf("\n");
  for (i = 0; i < sizeof (array_3d) / sizeof (int); i++)
    printf(" %d", *(**array_3d + i));
  printf("\n");
  for (i = 0; i < sizeof (array_3d9) / sizeof (int); i++)
    printf(" %d", *(**array_3d9 + i));
  printf("\n");

  return 0;
}
