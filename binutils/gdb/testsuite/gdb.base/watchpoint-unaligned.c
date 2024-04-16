/* This testcase is part of GDB, the GNU debugger.

   Copyright 2017-2024 Free Software Foundation, Inc.

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

#include <stdint.h>
#include <assert.h>

static int again;

static volatile struct
{
  uint64_t alignment;
  union
    {
      uint64_t size8[1];
      uint32_t size4[2];
      uint16_t size2[4];
      uint8_t size1[8];
      uint64_t size8twice[2];
    }
  u;
} data;

static int size = 0;
static int offset;

static void
write_size8twice (void)
{
  static const uint64_t first = 1;
  static const uint64_t second = 2;

#ifdef __aarch64__
  asm volatile ("stp %1, %2, [%0]"
		: /* output */
		: "r" (data.u.size8twice), "r" (first), "r" (second) /* input */
		: "memory" /* clobber */);
#else
  data.u.size8twice[0] = first;
  data.u.size8twice[1] = second;
#endif
}

int
main (void)
{
  volatile uint64_t local;

  assert (sizeof (data) == 8 + 2 * 8);

  write_size8twice ();

  while (size)
    {
      switch (size)
	{
/* __s390x__ also defines __s390__ */
#ifdef __s390__
# define ACCESS(var) var = ~var
#else
# define ACCESS(var) local = var
#endif
	case 8:
	  ACCESS (data.u.size8[offset]);
	  break;
	case 4:
	  ACCESS (data.u.size4[offset]);
	  break;
	case 2:
	  ACCESS (data.u.size2[offset]);
	  break;
	case 1:
	  ACCESS (data.u.size1[offset]);
	  break;
#undef ACCESS
	default:
	  assert (0);
	}
      size = 0;
      size = size; /* start_again */
    }
  return 0; /* final_return */
}
