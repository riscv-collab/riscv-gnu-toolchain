/* This testcase is part of GDB, the GNU debugger.

   Copyright 2008-2024 Free Software Foundation, Inc.

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

enum e
  {
    ONE = 1,
    TWO = 2
  };

typedef struct s *PTR;

enum e evalue = TWO;

#ifdef __cplusplus

struct Base {
  virtual int x() { return 5; }
};

struct Derived : public Base {
};

Base *base = new Derived ();
Derived derived;
Base &base_ref = derived;

void ptr_ref(int*& rptr_int)
{
  return; /* break to inspect pointer by reference. */
}
#endif

void func1 ()
{
  printf ("void function called\n");
}

int func2 (int arg1, int arg2)
{
  return arg1 + arg2;
}

char **save_argv;

int
main (int argc, char *argv[])
{
  char *cp = argv[0]; /* Prevent gcc from optimizing argv[] out.  */
  struct s s;
  union u u;
  PTR x = &s;
  char st[17] = "divide et impera";
  char nullst[17] = "divide\0et\0impera";
  void (*fp1) (void)  = &func1;
  int  (*fp2) (int, int) = &func2;
  const char *embed = "embedded x\201\202\203\204";
  int a[3] = {1,2,3};
  int *p = a;
  int i = 2;
  int *ptr_i = &i;
  s.a = 3;
  s.b = 5;
  u.a = 7;
  (*fp1) ();
  (*fp2) (10,20);

#ifdef __cplusplus
  ptr_ref(ptr_i);
#endif

  save_argv = argv;      /* break to inspect struct and union */
  return 0;
}
