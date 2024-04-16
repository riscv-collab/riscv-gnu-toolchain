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

#include <stdio.h>
#include <stdlib.h>

extern int marker1 (void);
extern int marker2 (int a);
extern void marker3 (char *a, char *b);
extern void marker4 (long d);

/* We're used by a test that requires malloc, so make sure it is in
   the executable.  */
void *need_malloc ()
{
  return malloc (1);
}

/*
 *	This simple classical example of recursion is useful for
 *	testing stack backtraces and such.
 */

int factorial(int);

int
main (int argc, char **argv, char **envp)
{
    if (argc == 12345) {  /* an unlikely value < 2^16, in case uninited */ /* set breakpoint 6 here */
	fprintf (stderr, "usage:  factorial <number>\n");
	return 1;
    }
    printf ("%d\n", factorial (atoi ("6")));  /* set breakpoint 1 here */
    /* set breakpoint 12 here */
    marker1 ();  /* set breakpoint 11 here */
    marker2 (43); /* set breakpoint 20 here */
    marker3 ("stack", "trace"); /* set breakpoint 21 here */
    marker4 (177601976L);
    /* We're used by a test that requires malloc, so make sure it is
       in the executable.  */
    (void)malloc (1);

    argc = (argc == 12345); /* This is silly, but we can step off of it */ /* set breakpoint 2 here */
    return argc;  /* set breakpoint 10 here */
} /* set breakpoint 10a here */

int factorial (int value)
{
  if (value > 1) {  /* set breakpoint 7 here */
	value *= factorial (value - 1);
    }
    return (value); /* set breakpoint 19 here */
}

int multi_line_if_conditional (int a, int b, int c)
{
  if (a    /* set breakpoint 3 here */
      && b
      && c)
    return 0;
  else
    return 1;
}

int multi_line_while_conditional (int a, int b, int c)
{
  while (a /* set breakpoint 4 here */
      && b
      && c)
    {
      a--, b--, c--;
    }
  return 0;
}
