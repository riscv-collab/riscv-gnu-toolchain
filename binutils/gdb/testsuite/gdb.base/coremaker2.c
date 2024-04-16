/* Copyright 1992-2024 Free Software Foundation, Inc.

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

/*  This test has two large memory areas buf_rw and buf_ro. 

    buf_rw is written to by the program while buf_ro is initialized at
    compile / load time.  Thus, when a core file is created, buf_rw's
    memory should reside in the core file, but buf_ro probably won't be.
    Instead, the contents of buf_ro are available from the executable.

    Now, for the wrinkle:  We create a one page read-only mapping over
    both of these areas.  This will create a one page "hole" of all
    zeros in each area.

    Will GDB be able to correctly read memory from each of the four
    (or six, if you count the regions on the other side of each hole)
    memory regions?  */

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>

/* These are globals so that we can find them easily when debugging
   the core file.  */
long pagesize;
uintptr_t addr;
char *mbuf_ro;
char *mbuf_rw;

/* 256 KiB buffer.  */
char buf_rw[256 * 1024];

#define C5_16 \
  0xc5, 0xc5, 0xc5, 0xc5, \
  0xc5, 0xc5, 0xc5, 0xc5, \
  0xc5, 0xc5, 0xc5, 0xc5, \
  0xc5, 0xc5, 0xc5, 0xc5

#define C5_256 \
  C5_16, C5_16, C5_16, C5_16, \
  C5_16, C5_16, C5_16, C5_16, \
  C5_16, C5_16, C5_16, C5_16, \
  C5_16, C5_16, C5_16, C5_16

#define C5_1k \
  C5_256, C5_256, C5_256, C5_256

#define C5_8k \
  C5_1k, C5_1k, C5_1k, C5_1k, \
  C5_1k, C5_1k, C5_1k, C5_1k

#define C5_64k \
  C5_8k, C5_8k, C5_8k, C5_8k, \
  C5_8k, C5_8k, C5_8k, C5_8k

#define C5_256k \
  C5_64k, C5_64k, C5_64k, C5_64k

/* 256 KiB worth of data.  For this test case, we can't allocate a
   buffer and then fill it; we want GDB to have to read this data
   from the executable; it should NOT find it in the core file.  */

const char buf_ro[] = { C5_256k };

int
main (int argc, char **argv)
{
  int i, bitcount;

#ifdef _SC_PAGESIZE
  pagesize = sysconf (_SC_PAGESIZE);
#else
  pagesize = 8192;
#endif

  /* Verify that pagesize is a power of 2.  */
  bitcount = 0;
  for (i = 0; i < 4 * sizeof (pagesize); i++)
    if (pagesize & (1 << i))
      bitcount++;

  if (bitcount != 1)
    {
      fprintf (stderr, "pagesize is not a power of 2.\n");
      exit (1);
    }

  /* Compute an address that should be within buf_ro.  Complain if not.  */
  addr = ((uintptr_t) buf_ro + pagesize) & ~(pagesize - 1);

  if (addr <= (uintptr_t) buf_ro
      || addr >= (uintptr_t) buf_ro + sizeof (buf_ro))
    {
      fprintf (stderr, "Unable to compute a suitable address within buf_ro.\n");
      exit (1);
    }

  mbuf_ro = mmap ((void *) addr, pagesize, PROT_READ,
               MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);

  if (mbuf_ro == MAP_FAILED)
    {
      fprintf (stderr, "mmap #1 failed: %s.\n", strerror (errno));
      exit (1);
    }

  /* Write (and fill) the R/W region.  */
  for (i = 0; i < sizeof (buf_rw); i++)
    buf_rw[i] = 0x6b;

  /* Compute an mmap address within buf_rw.  Complain if it's somewhere
     else.  */
  addr = ((uintptr_t) buf_rw + pagesize) & ~(pagesize - 1);

  if (addr <= (uintptr_t) buf_rw
      || addr >= (uintptr_t) buf_rw + sizeof (buf_rw))
    {
      fprintf (stderr, "Unable to compute a suitable address within buf_rw.\n");
      exit (1);
    }

  mbuf_rw = mmap ((void *) addr, pagesize, PROT_READ,
               MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);

  if (mbuf_rw == MAP_FAILED)
    {
      fprintf (stderr, "mmap #2 failed: %s.\n", strerror (errno));
      exit (1);
    }

  /* With correct ulimit, etc. this should cause a core dump.  */
  abort ();
}
