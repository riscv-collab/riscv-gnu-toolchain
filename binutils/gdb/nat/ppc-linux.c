/*Copyright (C) 2015-2024 Free Software Foundation, Inc.

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

#include "gdbsupport/common-defs.h"
#include "ppc-linux.h"
#include "nat/gdb_ptrace.h"
#include <elf.h>

#ifdef HAVE_GETAUXVAL
#include <sys/auxv.h>
#endif

#ifdef __powerpc64__

/* Get the HWCAP from the process of GDB or GDBserver.  If success,
   save it in *VALP.   */

static void
ppc64_host_hwcap (unsigned long *valp)
{
#ifdef HAVE_GETAUXVAL
  *valp = getauxval (AT_HWCAP);
#else
  unsigned long data[2];
  FILE *f = fopen ("/proc/self/auxv", "r");

  if (f == NULL)
    return;

  while (fread (data, sizeof (data), 1, f) > 0)
    {
      if (data[0] == AT_HWCAP)
	{
	  *valp = data[1];
	  break;
	}
    }

  fclose (f);
#endif /* HAVE_GETAUXVAL */
}

static inline int
ppc64_64bit_inferior_p (long msr)
{
  unsigned long ppc_host_hwcap = 0;

  /* Get host's HWCAP to check whether the machine is Book E.  */
  ppc64_host_hwcap (&ppc_host_hwcap);

  /* We actually have a 64-bit inferior only if the certain bit of the
     MSR is set.  The PowerISA Book III-S MSR is different from the
     PowerISA Book III-E MSR.  The Book III-S MSR is 64 bits wide, and
     its MSR[SF] is the bit 0 of a 64-bit value.  Book III-E MSR is 32
     bits wide, and its MSR[CM] is the bit 0 of a 32-bit value.   */
  if (ppc_host_hwcap & PPC_FEATURE_BOOKE)
    return msr & 0x80000000;
  else
    return msr < 0;
}

#endif

int
ppc_linux_target_wordsize (int tid)
{
  gdb_assert (tid != 0);

  int wordsize = 4;

  /* Check for 64-bit inferior process.  This is the case when the host is
     64-bit, and in addition the top bit of the MSR register is set.  */
#ifdef __powerpc64__
  long msr;

  errno = 0;
  msr = (long) ptrace (PTRACE_PEEKUSER, tid, PT_MSR * 8, 0);
  if (errno == 0 && ppc64_64bit_inferior_p (msr))
    wordsize = 8;
#endif

  return wordsize;
}
