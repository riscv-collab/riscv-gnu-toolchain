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

struct Class
{
  int a;
  char b;
  long c;

  Class () : a (1), b ('2'), c (3) { }
};

union Union
{
  Class *kp;
  char a;
  int b;
  long c;
};

enum Enum { A, B, C, D };

typedef unsigned char a4[4];
typedef unsigned char a8[8];
typedef unsigned char a12[12];
typedef Class c4[4];
typedef Union u8[8];
typedef Enum e12[12];

#define T(N)					\
  N N ## obj;					\
  N& N ## _ref = N ## obj;			\
  N* N ## p = &(N ## obj);			\
  N*& N ## p_ref = N ## p;			\
  int size_ ## N = sizeof (N ## _ref);		\
  int size_ ## N ## p = sizeof (N ## p_ref);	\

int
main (void)
{
  T (char);
  T (int);
  T (long);
  T (float);
  T (double);
  T (a4);
  T (a8);
  T (a12);
  T (Class);
  T (Union);
  T (Enum);
  T (c4);
  T (u8);
  T (e12);

  return 0; /* break here */
}
