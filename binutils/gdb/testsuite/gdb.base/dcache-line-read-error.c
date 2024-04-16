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

#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

size_t pg_size;
void *first_mapped_page;
void *last_mapped_page;

void
breakpt (void)
{
  /* Nothing. */
}

int
main (void)
{
  void *p;
  int pg_count;
  size_t i;

  /* Map 6 contiguous pages, and then unmap all second first, and
     second last.

     From GDB we will disassemble each of the _mapped_ pages, with a
     code-cache (dcache) line size bigger than the page size (twice
     bigger).  This makes GDB try to read one page before the mapped
     page once, and the page after another time.  GDB should give no
     error in either case.

     That is, depending on where the kernel aligns the pages, we get
     either:

      .---.---.---.---.---.---.
      | U | M | U | U | M | U |
      '---'---'---'---'---'---.
      |       |       |       |  <- line alignment
       ^^^^^^^         ^^^^^^^
          |               |
          + line1         + line2

     Or:

      .---.---.---.---.---.---.
      | U | M | U | U | M | U |
      '---'---'---'---'---'---.
          |       |       |      <- line alignment
           ^^^^^^^ ^^^^^^^
              |       |
        line1 +       + line2

    Note we really want to test that dcache behaves correctly when
    reading a cache line fails.  We're just using unmapped memory as
    proxy for any kind of error.  */

  pg_size = getpagesize ();
  pg_count = 6;

  p = mmap (0, pg_count * pg_size, PROT_READ|PROT_WRITE,
	    MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
  if (p == MAP_FAILED)
    {
      perror ("mmap");
      return EXIT_FAILURE;
    }

  /* Leave memory zero-initialized.  Disassembling 0s should behave on
     all targets.  */

  for (i = 0; i < pg_count; i++)
    {
      if (i == 1 || i == 4)
	continue;

      if (munmap (p + (i * pg_size), pg_size) == -1)
	{
	  perror ("munmap");
	  return EXIT_FAILURE;
	}
    }

  first_mapped_page = p + 1 * pg_size;;
  last_mapped_page = p + 4 * pg_size;

  breakpt ();

  return EXIT_SUCCESS;
}
