/* This testcase is part of GDB, the GNU debugger.

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

struct s
{
  int a;
  int b;
};

union u
{
  int a;
  float b;
};

enum color { red, green, blue };

static void
break_me (void)
{
}

static void
call_me (int i, float f, struct s s, struct s *ss, union u u, enum color e)
{
  break_me ();
}

int
main (void)
{
  struct s s;
  union u u;

  s.a = 3;
  s.b = 5;
  u.a = 7;

  call_me (3, 5.0, s, &s, u, green);

  return 0;
}
