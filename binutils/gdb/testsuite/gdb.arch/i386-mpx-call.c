/* Test for inferior function calls MPX context.

   Copyright (C) 2017-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#include <stdlib.h>
#include <string.h>

/* Defined size for arrays.  */
#define ARRAY_LENGTH    5


int
upper (int *a, int *b, int *c, int *d, int len)
{
  int value;

  value = *(a + len);
  value = *(b + len);
  value = *(c + len);
  value = *(d + len);

  value = value - *a + 1;
  return value;
}


int
lower (int *a, int *b, int *c, int *d, int len)
{
  int value;

  value = *(a - len);
  value = *(b - len);
  value = *(c - len);
  value = *(d - len);

  value = value - *a + 1;
  return value;
}


char
char_upper (char *str, int length)
{
  char ch;
  ch = *(str + length);

  return ch;
}


char
char_lower (char *str, int length)
{
  char ch;
  ch = *(str - length);

  return ch;
}


int
main (void)
{
  int sa[ARRAY_LENGTH];
  int sb[ARRAY_LENGTH];
  int sc[ARRAY_LENGTH];
  int sd[ARRAY_LENGTH];
  int *x, *a, *b, *c, *d;
  char mchar;
  char hello[] = "Hello";

  x = malloc (sizeof (int) * ARRAY_LENGTH);
  a = malloc (sizeof (int) * ARRAY_LENGTH);
  b = malloc (sizeof (int) * ARRAY_LENGTH);
  c = malloc (sizeof (int) * ARRAY_LENGTH);
  d = malloc (sizeof (int) * ARRAY_LENGTH);

  *x = upper (sa, sb, sc, sd, 0);  /* bkpt 1.  */
  *x = lower (a, b, c, d, 0);

  mchar = char_upper (hello, 10);
  mchar = char_lower (hello, 10);

  free (x);
  free (a);
  free (b);
  free (c);
  free (d);

  return 0;
}
