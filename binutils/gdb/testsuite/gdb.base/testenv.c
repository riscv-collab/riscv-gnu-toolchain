/* This testcase is part of GDB, the GNU debugger.

   Copyright 2011-2024 Free Software Foundation, Inc.

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

/*
    This source is used to check that GDB correctly
    passes on environment variables down to inferior.
    One of the tests checks that 'unset' variables also are removed from
    inferior environment list.  */

#include <stdio.h>
#include <string.h>

int main (int argc, char **argv, char **envp)

{
    int i, j;

    j = 0;
    for (i = 0; envp[i]; i++)
      {
	if (strncmp ("TEST_GDB", envp[i], 8) == 0)
	  {
	    printf ("%s\n", envp[i]);
	    j++;
	  }
      }
    printf ("Program found %d variables starting with TEST_GDB\n", j);
    return 0; /* set breakpoint here.  */
}

