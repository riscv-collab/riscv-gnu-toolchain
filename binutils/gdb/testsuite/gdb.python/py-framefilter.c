/* This testcase is part of GDB, the GNU debugger.

   Copyright 2013-2024 Free Software Foundation, Inc.

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

#include <stdlib.h>

void funca(void);
int count = 0;

typedef struct
{
  char *nothing;
  int f;
  short s;
} foobar;

void end_func (int foo, char *bar, foobar *fb, foobar bf)
{
  const char *str = "The End";
  const char *st2 = "Is Near";
  int b = 12;
  short c = 5;

  {
    int d = 15;
    int e = 14;
    const char *foo = "Inside block";
    {
      int f = 42;
      int g = 19;
      const char *bar = "Inside block x2";
      {
	short h = 9;
	h = h +1;  /* Inner test breakpoint  */
      }
    }
  }

  return; /* Backtrace end breakpoint */
}

void funcb(int j)
{
  struct foo
  {
    int a;
    int b;
  };

  struct foo bar;

  bar.a = 42;
  bar.b = 84;

  funca();
  return;
}

void funca(void)
{
  foobar fb;
  foobar *bf = NULL;

  if (count < 10)
    {
      count++;
      funcb(count);
    }

  fb.nothing = "Foo Bar";
  fb.f = 42;
  fb.s = 19;

  bf = (foobar *) alloca (sizeof (foobar));
  bf->nothing = (char *) alloca (128);
  bf->nothing = "Bar Foo";
  bf->f = 24;
  bf->s = 91;

  end_func(21, "Param", bf, fb);
  return;
}


void func1(void)
{
  funca();
  return;
}

int func2(int f)
{
  int c;
  const char *elided = "Elided frame";
  foobar fb;
  foobar *bf = NULL;

  fb.nothing = "Elided Foo Bar";
  fb.f = 84;
  fb.s = 38;

  bf = (foobar *) alloca (sizeof (foobar));
  bf->nothing = (char *) alloca (128);
  bf->nothing = "Elided Bar Foo";
  bf->f = 48;
  bf->s = 182;

  func1();
  return 1;
}

void func3(int i)
{
  func2(i);

  return;
}

int func4(int j)
{
  func3(j);

  return 2;
}

int func5(int f, int d)
{
  int i = 0;
  char *random = "random";
  i=i+f;

  func4(i);
  return i;
}

int
main()
{
  int z = 32;
  int y = 44;
  const char *foo1 = "Test";
  func5(3,5);
  return 0;
}
