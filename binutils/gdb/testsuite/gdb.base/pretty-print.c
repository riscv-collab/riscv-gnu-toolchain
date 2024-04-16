/* This testcase is part of GDB, the GNU debugger.

   Copyright 2019-2024 Free Software Foundation, Inc.

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

struct s1_t
{
  int one;
  int two;

  struct
  {
    union {
      unsigned three : 3;
      int four : 4;
    };

    union {
      int five : 3;
      unsigned six : 4;
    };
  } data;
} s1 = { .one = 1, .two = 2,
	 /* Use all-ones bit patterns for endianness independence.  */
	 .data = { .four = -1, .six = 15 } };

struct s2_t
{
  int one;
  int two;

  struct
  {
    int three;
    int four;
  };
} s2 = { .one = 1, .two = 2, .three = 3, .four = 4 };

int
main ()
{
  return 0;
}
