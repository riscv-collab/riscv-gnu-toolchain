/* This test program is part of GDB, the GNU debugger.

   Copyright 2022-2024 Free Software Foundation, Inc.

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

/* Exercise AArch64's Memory Tagging Extension corefile support.  We allocate
   multiple memory mappings with PROT_MTE and assign tag values for all the
   existing MTE granules.  */

/* This test was based on the documentation for the AArch64 Memory Tagging
   Extension from the Linux Kernel, found in the sources in
   Documentation/arm64/memory-tagging-extension.rst.  */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/auxv.h>
#include <sys/mman.h>
#include <sys/prctl.h>

/* From arch/arm64/include/uapi/asm/hwcap.h */
#ifndef HWCAP2_MTE
#define HWCAP2_MTE              (1 << 18)
#endif

/* From arch/arm64/include/uapi/asm/mman.h */
#ifndef PROT_MTE
#define PROT_MTE  0x20
#endif

#ifndef PR_SET_TAGGED_ADDR_CTRL
#define PR_SET_TAGGED_ADDR_CTRL 55
#define PR_TAGGED_ADDR_ENABLE	(1UL << 0)
#endif

/* From include/uapi/linux/prctl.h */
#ifndef PR_MTE_TCF_SHIFT
#define PR_MTE_TCF_SHIFT	1
#define PR_MTE_TCF_SYNC		(1UL << PR_MTE_TCF_SHIFT)
#define PR_MTE_TCF_ASYNC	(2UL << PR_MTE_TCF_SHIFT)
#define PR_MTE_TAG_SHIFT	3
#define PR_MTE_TAG_MASK		(0xffffUL << PR_MTE_TAG_SHIFT)
#endif

#ifdef ASYNC
#define TCF_MODE PR_MTE_TCF_ASYNC
#else
#define TCF_MODE PR_MTE_TCF_SYNC
#endif

#define NMAPS 5

/* We store the pointers and sizes of the memory maps we requested.  Each
   of them has a different size.  */
unsigned char *mmap_pointers[NMAPS];

/* Set the allocation tag on the destination address.  */
#define set_tag(tagged_addr) do {				  \
  asm volatile("stg %0, [%0]" : : "r" (tagged_addr) : "memory");  \
} while (0)


uintptr_t
set_logical_tag (uintptr_t ptr, unsigned char tag)
{
  ptr &= ~0xFF00000000000000ULL;
  ptr |= ((uintptr_t) tag << 56);
  return ptr;
}

void
fill_map_with_tags (unsigned char *ptr, size_t size, unsigned char *tag)
{
  for (size_t start = 0; start < size; start += 16)
    {
      set_tag (set_logical_tag (((uintptr_t)ptr + start) & ~(0xFULL), *tag));
      *tag = (*tag + 1) % 16;
    }
}

int
main (int argc, char **argv)
{
  unsigned char *tagged_ptr;
  unsigned long page_sz = sysconf (_SC_PAGESIZE);
  unsigned long hwcap2 = getauxval (AT_HWCAP2);

  /* Bail out if MTE is not supported.  */
  if (!(hwcap2 & HWCAP2_MTE))
    return 1;

  /* Enable the tagged address ABI, synchronous MTE tag check faults and
     allow all non-zero tags in the randomly generated set.  */
  if (prctl (PR_SET_TAGGED_ADDR_CTRL,
	     PR_TAGGED_ADDR_ENABLE | TCF_MODE
	     | (0xfffe << PR_MTE_TAG_SHIFT),
	     0, 0, 0))
    {
      perror ("prctl () failed");
      return 1;
    }

  /* Map a big area of NMAPS * 2 pages.  */
  unsigned char *big_map = mmap (0, NMAPS * 2 * page_sz, PROT_NONE,
				 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  if (big_map == MAP_FAILED)
    {
      perror ("mmap () failed");
      return 1;
    }

  /* Start with a tag of 0x1 so we can crash later.  */
  unsigned char tag = 1;

  /* From that big area of NMAPS * 2 pages, go through each page and protect
     alternating pages.  This should prevent the kernel from merging different
     mmap's and force the creation of multiple individual MTE-protected entries
     in /proc/<pid>/smaps.  */
  for (int i = 0; i < NMAPS; i++)
    {
      mmap_pointers[i] = big_map + (i * 2 * page_sz);

      /* Enable MTE on alternating pages.  */
      if (mprotect (mmap_pointers[i], page_sz,
		    PROT_READ | PROT_WRITE | PROT_MTE))
	{
	  perror ("mprotect () failed");
	  return 1;
	}

      fill_map_with_tags (mmap_pointers[i], page_sz, &tag);
    }

  /* The following line causes a crash on purpose.  */
  *mmap_pointers[0] = 0x4;

  return 0;
}
