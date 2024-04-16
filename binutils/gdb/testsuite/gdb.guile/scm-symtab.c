/* This testcase is part of GDB, the GNU debugger.

   Copyright 2010-2024 Free Software Foundation, Inc.

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

/* In scm-symtab-2.c.  */
extern void func1 (void);
extern int func2 (void);

struct simple_struct
{
  int a;
};

struct simple_struct qq;

int
func (int arg)
{
  int i = 2;
  i = i * arg; /* Block break here.  */
  return arg;
}

int
main (int argc, char *argv[])
{
  qq.a = func (42);

  func1 ();
  func2 ();      /* Break at func2 call site.  */
  return 0;      /* Break to end.  */
}
