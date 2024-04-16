/* Copyright (C) 2010-2024 Free Software Foundation, Inc.

   This file is part of GDB.

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

/* The original program corresponding to pieces.S.
   This came from https://bugzilla.redhat.com/show_bug.cgi?id=589467
   Note that it is not ever compiled, pieces.S is used instead.
   However, it is used to extract breakpoint line numbers.  */

struct A { int i; int j; };
struct B { int i : 12; int j : 12; int : 4; };
struct C { int i; int j; int q; };

__attribute__((noinline)) void
bar (int x)
{
  asm volatile ("" : : "r" (x) : "memory");
}

__attribute__((noinline)) int
f1 (int k)
{
  struct A a = { 4, k + 6 };
  asm ("" : "+r" (a.i));
  a.j++;
  bar (a.i);		/* { dg-final { gdb-test 20 "a.i" "4" } } */
  bar (a.j);		/* { dg-final { gdb-test 20 "a.j" "14" } } */
  return a.i + a.j;	/* f1 breakpoint */
}

__attribute__((noinline)) int
f2 (int k)
{
  int a[2] = { 4, k + 6 };
  asm ("" : "+r" (a[0]));
  a[1]++;
  bar (a[0]);		/* { dg-final { gdb-test 31 "a\[0\]" "4" } } */
  bar (a[1]);		/* { dg-final { gdb-test 31 "a\[1\]" "14" } } */
  return a[0] + a[1];	/* f2 breakpoint */
}

__attribute__((noinline)) int
f3 (int k)
{
  struct B a = { 4, k + 6 };
  asm ("" : "+r" (a.i));
  a.j++;
  bar (a.i);		/* { dg-final { gdb-test 42 "a.i" "4" } } */
  bar (a.j);		/* { dg-final { gdb-test 42 "a.j" "14" } } */
  return a.i + a.j;	/* f3 breakpoint */
}

__attribute__((noinline)) int
f4 (int k)
{
  int a[2] = { k, k };
  asm ("" : "+r" (a[0]));
  a[1]++;
  bar (a[0]);
  bar (a[1]);
  return a[0] + a[1];		/* f4 breakpoint */
}

__attribute__((noinline)) int
f5 (int k)
{
  struct A a = { k, k };
  asm ("" : "+r" (a.i));
  a.j++;
  bar (a.i);
  bar (a.j);
  return a.i + a.j;		/* f5 breakpoint */
}

__attribute__((noinline)) int
f6 (int k)
{
  int z = 23;
  struct C a = { k, k, z };
  asm ("" : "+r" (a.i));
  a.j++;
  bar (a.i);
  bar (a.j);
  return a.i + a.j;		/* f6 breakpoint */
}

int
main (void)
{
  int k;
  asm ("" : "=r" (k) : "0" (7));
  f1 (k);
  f2 (k);
  f3 (k);
  f4 (k);
  f5 (k);
  f6 (k);
  return 0;
}
