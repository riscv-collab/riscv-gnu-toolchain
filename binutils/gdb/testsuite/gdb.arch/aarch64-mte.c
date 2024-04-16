/* This test program is part of GDB, the GNU debugger.

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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Exercise AArch64's Memory Tagging Extension with tagged pointers.  */

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
#endif

void
access_memory (unsigned char *tagged_ptr, unsigned char *untagged_ptr)
{
  tagged_ptr[0] = 'a';
}

int
main (int argc, char **argv)
{
  unsigned char *tagged_ptr;
  unsigned char *untagged_ptr;
  unsigned long page_sz = sysconf (_SC_PAGESIZE);
  unsigned long hwcap2 = getauxval(AT_HWCAP2);

  /* Bail out if MTE is not supported.  */
  if (!(hwcap2 & HWCAP2_MTE))
    return 1;

  /* Enable the tagged address ABI, synchronous MTE tag check faults and
     allow all non-zero tags in the randomly generated set.  */
  if (prctl (PR_SET_TAGGED_ADDR_CTRL,
	     PR_TAGGED_ADDR_ENABLE | PR_MTE_TCF_SYNC
	     | (0xfffe << PR_MTE_TAG_SHIFT),
	     0, 0, 0))
    {
      perror ("prctl () failed");
      return 1;
    }

  /* Create a mapping that will have PROT_MTE set.  */
  tagged_ptr = mmap (0, page_sz, PROT_READ | PROT_WRITE,
		     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (tagged_ptr == MAP_FAILED)
    {
      perror ("mmap () failed");
      return 1;
    }

  /* Create another mapping that won't have PROT_MTE set.  */
  untagged_ptr = mmap (0, page_sz, PROT_READ | PROT_WRITE,
		       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (untagged_ptr == MAP_FAILED)
    {
      perror ("mmap () failed");
      return 1;
    }

  /* Enable MTE on the above anonymous mmap.  */
  if (mprotect (tagged_ptr, page_sz, PROT_READ | PROT_WRITE | PROT_MTE))
    {
      perror ("mprotect () failed");
      return 1;
    }

  access_memory (tagged_ptr, untagged_ptr);

  return 0;
}
