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

#include <stdlib.h>
#include <string.h>

/* Test various kinds of stepping.
*/
int myglob = 0;

int callee() {		/* ENTER CALLEE */
  return myglob++;	/* ARRIVED IN CALLEE */
}			/* RETURN FROM CALLEE */

/* We need to make this function take more than a single instruction
   to run, otherwise it could hide PR gdb/16678, as reverse execution can
   step over a single-instruction function.  */
int
recursive_callee (int val)
{
  if (val == 0)
    return 0;
  val /= 2;
  if (val > 1)
    val++;
  return recursive_callee (val);	/* RECURSIVE CALL */
} /* EXIT RECURSIVE FUNCTION */

/* A structure which, we hope, will need to be passed using memcpy.  */
struct rhomboidal {
  int rather_large[100];
};

void
large_struct_by_value (struct rhomboidal r)
{
  myglob += r.rather_large[42]; /* step-test.exp: arrive here 1 */
}

int main () {
   int w,x,y,z;
   int a[10], b[10];

   /* Test "next" and "step" */
   w = 0;	/* BREAK AT MAIN */
   x = 1;	/* NEXT TEST 1 */
   y = 2;	/* STEP TEST 1 */
   z = 3;	/* REVERSE NEXT TEST 1 */
   w = w + 2;	/* NEXT TEST 2 */
   x = x + 3;	/* REVERSE STEP TEST 1 */
   y = y + 4;
   z = z + 5;	/* STEP TEST 2 */

   /* Test that next goes over recursive calls too */
   recursive_callee (32); /* NEXT OVER THIS RECURSION */

   /* Test that "next" goes over a call */
   callee();	/* NEXT OVER THIS CALL */

   /* Test that "step" doesn't */
   callee();	/* STEP INTO THIS CALL */

   /* Test "stepi" */
   a[5] = a[3] - a[4]; /* FINISH TEST */
   callee();	/* STEPI TEST */

   /* Test "nexti" */
   callee();	/* NEXTI TEST */

   y = w + z;

   {
     struct rhomboidal r;
     memset (r.rather_large, 0, sizeof (r.rather_large));
     r.rather_large[42] = 10;
     large_struct_by_value (r);  /* step-test.exp: large struct by value */
   }

   exit (0); /* end of main */
}

