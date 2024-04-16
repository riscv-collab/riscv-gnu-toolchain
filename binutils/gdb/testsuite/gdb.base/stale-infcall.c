/* This testcase is part of GDB, the GNU debugger.

   Copyright 2012-2024 Free Software Foundation, Inc.

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

#include <setjmp.h>
#include <string.h>
#include <stdlib.h>

#define BUFSIZE 0x1000

static jmp_buf jmp;

void
infcall (void)
{
  longjmp (jmp, 1); /* test-next */
}

static void
run1 (void)
{
  char buf[BUFSIZE / 2];
  int dummy = 0;

  dummy++; /* break-run1 */
}

static char buf_zero[BUFSIZE];

static void
run2 (void)
{
  char buf[BUFSIZE];

  memset (buf, 0, sizeof (buf));

  if (memcmp (buf, buf_zero, sizeof (buf)) != 0) /* break-run2 */
    abort (); /* break-fail */
}

int
main ()
{
  if (setjmp (jmp) == 0) /* test-pass */
    infcall ();

  if (setjmp (jmp) == 0) /* test-fail */
    run1 ();
  else
    run2 ();

  return 0; /* break-exit */
}
