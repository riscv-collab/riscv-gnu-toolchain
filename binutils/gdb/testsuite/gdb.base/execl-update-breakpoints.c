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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
some_function (void)
{
}

int
main (int argc, char **argv)
{
  size_t len;
  char *bin;

  len = strlen (argv[0]);
  bin = malloc (len + 1);
  memcpy (bin, argv[0], len + 1);
  if (bin[len - 1] == '1')
    bin[len - 1] = '2';

  execl (bin, bin, (char *) NULL);
  perror ("execl failed");
  some_function ();
  exit (1);
}
