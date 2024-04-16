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

#include <stddef.h>
#define SIZE 5

struct foo
{
  int a;
};

typedef struct bar
{
  int x;
  struct foo y;
} BAR;

void
vla_factory (int n)
{
  int             int_vla[n];
  unsigned int    unsigned_int_vla[n];
  double          double_vla[n];
  float           float_vla[n];
  long            long_vla[n];
  unsigned long   unsigned_long_vla[n];
  char            char_vla[n];
  short           short_vla[n];
  unsigned short  unsigned_short_vla[n];
  unsigned char   unsigned_char_vla[n];
  struct foo      foo_vla[n];
  BAR             bar_vla[n];
  int i;

  for (i = 0; i < n; i++)
    {
      int_vla[i] = i*2;
      unsigned_int_vla[i] = i*2;
      double_vla[i] = i/2.0;
      float_vla[i] = i/2.0f;
      long_vla[i] = i*2;
      unsigned_long_vla[i] = i*2;
      char_vla[i] = 'A';
      short_vla[i] = i*2;
      unsigned_short_vla[i] = i*2;
      unsigned_char_vla[i] = 'A';
      foo_vla[i].a = i*2;
      bar_vla[i].x = i*2;
      bar_vla[i].y.a = i*2;
    }

  size_t int_size        = sizeof(int_vla);     /* vlas_filled */
  size_t uint_size       = sizeof(unsigned_int_vla);
  size_t double_size     = sizeof(double_vla);
  size_t float_size      = sizeof(float_vla);
  size_t long_size       = sizeof(long_vla);
  size_t char_size       = sizeof(char_vla);
  size_t short_size      = sizeof(short_vla);
  size_t ushort_size     = sizeof(unsigned_short_vla);
  size_t uchar_size      = sizeof(unsigned_char_vla);
  size_t foo_size        = sizeof(foo_vla);
  size_t bar_size        = sizeof(bar_vla);

  return;                                 /* break_end_of_vla_factory */
}

int
main (void)
{
  vla_factory(SIZE);
  return 0;
}
