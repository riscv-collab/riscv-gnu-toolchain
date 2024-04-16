/* This testcase is part of GDB, the GNU debugger.

   Copyright 2018-2024 Free Software Foundation, Inc.

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
#include <malloc.h>
#include <sys/mman.h>

int
main (void)
{
  char *ptr;
  int ind, cnt;

  cnt = 100000;
  for (ind = 0; ind < cnt; ind++)
    {
      ptr = mmap (NULL, 100, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
      if (ptr == NULL)
	{
	  fprintf (stderr, "Error allocating memory using mmap\n");
	  return -1;
	}

      ptr = mmap (NULL, 100, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
      if (ptr == NULL)
	{
	  fprintf (stderr, "Error allocating memory using mmap\n");
	  return -1;
	}
    }

  ptr = NULL;
  *ptr = '\0';

  return 0;
}
