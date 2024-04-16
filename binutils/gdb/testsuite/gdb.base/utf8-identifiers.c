/* -*- coding: utf-8 -*- */

/* This testcase is part of GDB, the GNU debugger.

   Copyright 2017-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* UTF-8 "função1".  */
#define FUNCAO1 fun\u00e7\u00e3o1

/* UTF-8 "função2".  */
#define FUNCAO2 fun\u00e7\u00e3o2

/* UTF-8 "my_função".  */
#define MY_FUNCAO my_fun\u00e7\u00e3o

/* UTF-8 "num_€".  */
#define NUM_EUROS num_\u20ac

struct S
{
  int NUM_EUROS;
} g_s;

void
FUNCAO1 (void)
{
  g_s.NUM_EUROS = 1000;
}

void
FUNCAO2 (void)
{
  g_s.NUM_EUROS = 1000;
}

void
MY_FUNCAO (void)
{
}

int NUM_EUROS = 2000;

static void
done ()
{
}

int
main ()
{
  FUNCAO1 ();
  done ();
  FUNCAO2 ();
  MY_FUNCAO ();

  return 0;
}
