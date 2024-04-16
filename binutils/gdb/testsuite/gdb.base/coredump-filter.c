/* Copyright 2015-2024 Free Software Foundation, Inc.

   This file is part of GDB.

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

#define _GNU_SOURCE
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>

static void *
do_mmap (void *addr, size_t size, int prot, int flags, int fd, off_t offset)
{
  void *ret = mmap (addr, size, prot, flags, fd, offset);

  assert (ret != MAP_FAILED);
  return ret;
}

int
main (int argc, char *argv[])
{
  const size_t size = 10;
  const int default_prot = PROT_READ | PROT_WRITE;
  char *private_anon, *shared_anon;
  char *dont_dump;
  int i;

  private_anon = do_mmap (NULL, size, default_prot,
			  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  memset (private_anon, 0x11, size);

  shared_anon = do_mmap (NULL, size, default_prot,
			 MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  memset (shared_anon, 0x22, size);

  dont_dump = do_mmap (NULL, size, default_prot,
		       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  memset (dont_dump, 0x55, size);
  i = madvise (dont_dump, size, MADV_DONTDUMP);
  assert_perror (errno);
  assert (i == 0);

  return 0; /* break-here */
}

/* Write V to /proc/self/coredump_filter.  Return 0 on success.  */

int
set_coredump_filter (int v)
{
  FILE *f = fopen("/proc/self/coredump_filter", "r+");

  if (f == NULL)
    return 1;

  fprintf(f, "%#x", v);

  fclose (f);
  return 0;
}
