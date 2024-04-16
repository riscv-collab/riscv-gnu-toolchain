/* Copyright 2019-2024 Free Software Foundation, Inc.

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

#include <string.h>

struct S1
{
  int a;
  int b;
};

struct S2
{
  char str[10];
  struct S1 s1;
};

struct S3
{
  struct S2 s2;
};

int
main ()
{
  struct S3 s3;

  memcpy (s3.s2.str, "abcde\0fghi", 10);
  s3.s2.s1.a = 3;
  s3.s2.s1.b = 4;

  return 0;	/* Break here.  */
}
