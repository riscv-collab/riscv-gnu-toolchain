/* This testcase is part of GDB, the GNU debugger.

   Copyright 2010-2024 Free Software Foundation, Inc.

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
#include <unistd.h>

const char stub[] = {
#ifdef GEN
# include GEN
#endif
};

int
main (int argc, char **argv)
{
  /* Generator of GEN written in Python takes about 15s for x86_64's 4MB.  */
  if (argc == 2)
    {
      long count = strtol (argv[1], NULL, 0);

      while (count-- > 0)
	puts ("0x55,");

      return 0;
    }
  if (argc != 1)
    return 1;

  puts ("sleeping");
  fflush (stdout);

  return sleep (60);
}
