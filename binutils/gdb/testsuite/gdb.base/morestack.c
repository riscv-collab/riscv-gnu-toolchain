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

/* Based on the gcc testcase `gcc/testsuite/gcc.dg/split-1.c'.  This test
   needs to use setrlimit to set the stack size, so it can only run on Unix.
   */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <stdio.h>
#include <sys/mman.h>

/* Use a noinline function to ensure that the buffer is not removed
   from the stack.  */
static void use_buffer (char *buf) __attribute__ ((noinline));
static void
use_buffer (char *buf)
{
  buf[0] = '\0';
}

static volatile int marker_var;

static void
marker_miss (void)
{
  marker_var = 0;
}

static void
marker_hit (void)
{
  marker_var = 0;
}

void *reserved;
#define RESERVED_SIZE 0x1000000

/* Each recursive call uses 10,000 bytes.  We call it 1000 times,
   using a total of 10,000,000 bytes.  If -fsplit-stack is not
   working, that will overflow our stack limit.  */

static void
down (int i)
{
  char buf[10000];
  static void *last;

  if (last && last < (void *) buf)
    marker_hit ();
  last = buf;

  if (i == 500)
    {
      if (munmap (reserved, RESERVED_SIZE) != 0)
        abort ();
      reserved = NULL;
    }

  if (i > 0)
    {
      use_buffer (buf);
      down (i - 1);
    }
  else
    marker_miss ();
}

int
main (void)
{
  struct rlimit r;

  reserved = mmap (NULL, RESERVED_SIZE, PROT_READ | PROT_WRITE,
		   MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if (reserved == MAP_FAILED)
    abort ();

  /* We set a stack limit because we are usually invoked via make, and
     make sets the stack limit to be as large as possible.  */
  r.rlim_cur = 8192 * 1024;
  r.rlim_max = 8192 * 1024;
  if (setrlimit (RLIMIT_STACK, &r) != 0)
    abort ();
  down (1000);
  return 0;
}
