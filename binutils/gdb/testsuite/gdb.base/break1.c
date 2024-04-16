/* This testcase is part of GDB, the GNU debugger.

   Copyright 1992-2024 Free Software Foundation, Inc.

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

/* The code for this file was extracted from the gdb testsuite
   testcase "break.c". */

/* A structure we use for field name completion tests.  */
struct some_struct
{
  int a_field;
  int b_field;
  union { int z_field; };
};

struct some_struct values[50];

/* Some definitions for tag completion.  */
enum some_enum { VALUE };

enum some_enum some_enum_global;

union some_union
{
  int f1;
  double f2;
};

union some_union some_union_global;

/* A variable with a name "similar" to the above struct, to test that
   tag completion works ok.  */
int some_variable;

/* The following functions do nothing useful.  They are included
   simply as places to try setting breakpoints at.  They are
   explicitly "one-line functions" to verify that this case works
   (some versions of gcc have or have had problems with this).
  
   These functions are in a separate source file to prevent an
   optimizing compiler from inlining them and optimizing them away. */

int marker1 (void) { return (0); }	/* set breakpoint 15 here */
int marker2 (int a) { return (1); }	/* set breakpoint 8 here */
void marker3 (char *a, char *b) {}	/* set breakpoint 17 here */
void marker4 (long d) { values[0].a_field = d; }	/* set breakpoint 14 here */
