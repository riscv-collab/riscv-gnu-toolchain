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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <stdio.h>
#include <stdlib.h>

/* Wrapper around getenv for GDB.  */

static const char *
my_getenv (const char *name)
{
  return getenv (name);
}

int
main (int argc, char *argv[])
{
  /* Call malloc to ensure it is linked in.  */
  char *tmp = (char *) malloc (1);
  /* Similarly call my_getenv instead of getenv directly to make sure
     the former isn't optimized out.  my_getenv is called by GDB.  */
  const char *myvar = my_getenv ("GDB_TEST_VAR");

  if (myvar != NULL)
    printf ("It worked!  myvar = '%s'\n", myvar);
  else
    printf ("It failed.");

  free (tmp);
  return 0;	/* break-here */
}
