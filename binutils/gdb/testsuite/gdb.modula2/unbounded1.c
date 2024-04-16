/* This test script is part of GDB, the GNU debugger.

   Copyright 2007-2024 Free Software Foundation, Inc.

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

typedef struct {
   char *_m2_contents;
   int _m2_high;
} unbounded;


static int
foo (unbounded a)
{
  if (a._m2_contents[3] == 'd')
    return 0;
  else
    return 1;
}

int
main ()
{
  unbounded t;
  char data[6];
  char *input = "abcde";
  int i;

  t._m2_contents = (char *)&data;
  t._m2_high = 4;
  /* include the <nul> in the string, even though high is set to 4.  */
  
  for (i=0; i<6; i++)
    data[i] = input[i];
  return foo (t);
}
