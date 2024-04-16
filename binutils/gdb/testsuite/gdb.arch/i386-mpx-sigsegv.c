/* Copyright (C) 2015-2024 Free Software Foundation, Inc.

   Contributed by Intel Corp. <walfred.tedeschi@intel.com>

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

#define OUR_SIZE    5

int gx[OUR_SIZE];
int ga[OUR_SIZE];
int gb[OUR_SIZE];
int gc[OUR_SIZE];
int gd[OUR_SIZE];

int
bp1 (int value)
{
  return 1;
}

int
bp2 (int value)
{
  return 1;
}

void
upper (int * p, int * a, int * b, int * c, int * d, int len)
{
  int value;
  value = *(p + len);
  value = *(a + len);
  value = *(b + len);
  value = *(c + len);
  value = *(d + len);
}

void
lower (int * p, int * a, int * b, int * c, int * d, int len)
{
  int value;
  value = *(p - len);
  value = *(a - len);
  value = *(b - len);
  value = *(c - len);
  bp2 (value);
  value = *(d - len);
}

int
main (void)
{
  int sx[OUR_SIZE];
  int sa[OUR_SIZE];
  int sb[OUR_SIZE];
  int sc[OUR_SIZE];
  int sd[OUR_SIZE];
  int *x, *a, *b, *c, *d;

  x = calloc (OUR_SIZE, sizeof (int));
  a = calloc (OUR_SIZE, sizeof (int));
  b = calloc (OUR_SIZE, sizeof (int));
  c = calloc (OUR_SIZE, sizeof (int));
  d = calloc (OUR_SIZE, sizeof (int));

  upper (x, a, b, c, d, OUR_SIZE + 2);
  upper (sx, sa, sb, sc, sd, OUR_SIZE + 2);
  upper (gx, ga, gb, gc, gd, OUR_SIZE + 2);
  lower (x, a, b, c, d, 1);
  lower (sx, sa, sb, sc, sd, 1);
  bp1 (*x);
  lower (gx, ga, gb, gc, gd, 1);

  free (x);
  free (a);
  free (b);
  free (c);
  free (d);

  return 0;
}
