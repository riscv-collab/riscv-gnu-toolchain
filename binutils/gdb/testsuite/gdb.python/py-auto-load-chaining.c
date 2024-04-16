/* This testcase is part of GDB, the GNU debugger.

   Copyright 2021-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see  <http://www.gnu.org/licenses/>.  */

#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>

/* These will hold the addresses for two memory regions.  */

void *region_1;
void *region_2;

/* Allocate a page of memory using mmap, and return a pointer.  */

void *
allocate_page (void)
{
  void *addr;
  int pgsize = sysconf(_SC_PAGE_SIZE);
  addr = mmap (NULL, pgsize, PROT_EXEC | PROT_READ | PROT_WRITE,
               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (addr == MAP_FAILED)
    perror ("mmap");
}

/* Only called so we can create a breakpoint.  */

void
breakpt (void)
{
  /* Nothing.  */
}

/* The test.  */

int
main (void)
{
  region_1 = allocate_page ();
  region_2 = allocate_page ();

  breakpt ();	/* Break Here.  */
}
