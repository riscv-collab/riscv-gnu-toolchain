/* Copyright (C) 2008-2024 Free Software Foundation, Inc.

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

/* This is only ever run if it is compiled with a new-enough GCC, but
   we don't want the compilation to fail if compiled by some other
   compiler.  */
#ifdef __GNUC__
#define ATTR __attribute__((always_inline))
#else
#define ATTR
#endif

int x, y;
volatile int z = 0;
volatile int result;
volatile int *array_p;

void bar(void);

void
init_array (int *array, int n)
{
  int i;
  for (i = 0; i < n; ++i)
    array[i] = 0;
}

inline ATTR int func1(int arg1)
{
  int array[64];
  init_array (array, 64);
  array_p = array;
  array[0] = result;
  array[1] = arg1;
  bar ();
  return x * y + array_p[0] * arg1;
}

inline ATTR int func2(int arg2)
{
  return x * func1 (arg2);
}

inline ATTR
void
scoped (int s)
{
  int loc1 = 10;
  if (s > 0)
    {
      int loc2 = 20;
      s++; /* bp for locals 1 */
      if (s > 1)
	{
	  int loc3 = 30;
	  s++; /* bp for locals 2 */
	}
    }
  s++; /* bp for locals 3 */
}

int main (void)
{
  int val;

  x = 7;
  y = 8;
  bar ();

  val = func1 (result);
  result = val;

  val = func2 (result);
  result = val;

  scoped (40);

  return 0;
}
