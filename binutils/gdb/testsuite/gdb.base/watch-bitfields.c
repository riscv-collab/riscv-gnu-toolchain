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

struct foo
{
  unsigned long a:1;
  unsigned char b:2;
  unsigned long c:3;
  char d:4;
  int e:5;
  char f:6;
  int g:7;
  long h:8;
} q = { 0 };

int
main (void)
{
  q.a = 1;
  q.b = 2;
  q.c = 3;
  q.d = 4;
  q.e = 5;
  q.f = 6;
  q.g = -7;
  q.h = -8;
  q.a--;
  q.h--;
  q.c--;
  q.b--;
  q.e--;
  q.d--;
  q.c--;
  q.f--;
  q.g--;
  q.h--;


  return 0;
}
