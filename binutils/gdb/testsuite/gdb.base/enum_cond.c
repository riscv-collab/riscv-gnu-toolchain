/* This testcase is part of GDB, the GNU debugger.
   Copyright 2012-2024 Free Software Foundation, Inc.

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

enum EE
{
  VALUE = 1
};

struct x
{
  unsigned char before;
  enum EE e;
  unsigned char after;
};


int
call_me (struct x param)
{
  return param.e;
}

int
main (void)
{
  struct x val;

  val.before = 0xff;
  val.e = VALUE;
  val.after = 0xff;

  call_me (val);
  return 0;
}

