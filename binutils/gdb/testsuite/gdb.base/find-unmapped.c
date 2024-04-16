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

#define CHUNK_SIZE 16000 /* same as findcmd.c's */

void *global_var_0;
void *global_var_1;
void *global_var_2;

void
breakpt ()
{
  /* Nothing. */
}

int
main (void)
{
  void *p;
  size_t pg_size;
  int pg_count;
  void *unmapped_page, *last_mapped_page, *first_mapped_page;

  /*
    Map enough pages to cover at least CHUNK_SIZE, and one extra page.  We
    then unmap the last page.

    From gdb we can then perform find commands into unmapped region, gdb
    should give an error.

    .-- global_var_0  .-- global_var_1
    |                 |   .-- global_var_2
    |                 |   |
    .----.----.----.----.----.
    |    |    |    |    |    |
    '----'----'----'----'----'
    |<- CHUNK_SIZE ->|

    If CHUNK_SIZE equals page size then we'll get 3 pages, and if
    CHUNK_SIZE is less than page size we'll get 2 pages.  The test will
    still work in these cases.

    (1) We do a find from global_var_0 to global_var_2, this will fail when
    loading the second chunk, as we know at least CHUNK_SIZE bytes are in
    mapped space.

    (2) We do a find from global_var_1 to global_var_2, this will fail when
    loading the first chunk, assuming the CHUNK_SIZE is at least 16 bytes.

    (3) We do a find from global_var_2 to (global_var_2 + 16), this too
    will fail when loading the first chunk regardless of the chunk size.
  */

  pg_size = getpagesize ();
  /* The +2 ensures the extra page.  */
  pg_count = CHUNK_SIZE / pg_size + 2;

  p = mmap (0, pg_count * pg_size, PROT_READ|PROT_WRITE,
	    MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
  if (p == MAP_FAILED)
    {
      perror ("mmap");
      return EXIT_FAILURE;
    }

  memset (p, 0, pg_count * pg_size);

  if (munmap (p + (pg_count - 1) * pg_size, pg_size) == -1)
    {
      perror ("munmap");
      return EXIT_FAILURE;
    }

  first_mapped_page = p;
  last_mapped_page = p + (pg_count - 2) * pg_size;
  unmapped_page = last_mapped_page + pg_size;

  /* Setup global variables we reference from gdb.  */
  global_var_0 = first_mapped_page;
  global_var_1 = unmapped_page - 16;
  global_var_2 = unmapped_page + 16;

  breakpt ();

  return EXIT_SUCCESS;
}
