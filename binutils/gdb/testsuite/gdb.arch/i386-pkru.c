/* Test program for PKEYS registers.

   Copyright 2015-2024 Free Software Foundation, Inc.

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

#include <stddef.h>
#include "x86-cpuid.h"

#ifndef NOINLINE
#define NOINLINE __attribute__ ((noinline))
#endif

unsigned int have_pkru (void) NOINLINE;

static inline unsigned long
rdpkru (void)
{
  unsigned int eax, edx;
  unsigned int ecx = 0;
  unsigned int pkru;

  asm volatile (".byte 0x0f,0x01,0xee\n\t"
               : "=a" (eax), "=d" (edx)
               : "c" (ecx));
  pkru = eax;
  return pkru;
}

static inline void
wrpkru (unsigned int pkru)
{
  unsigned int eax = pkru;
  unsigned int ecx = 0;
  unsigned int edx = 0;

  asm volatile (".byte 0x0f,0x01,0xef\n\t"
               : : "a" (eax), "c" (ecx), "d" (edx));
}

unsigned int NOINLINE
have_pkru (void)
{
  unsigned int eax, ebx, ecx, edx;

  if (!__get_cpuid (1, &eax, &ebx, &ecx, &edx))
    return 0;

  if ((ecx & bit_OSXSAVE) == bit_OSXSAVE)
    {
      if (__get_cpuid_max (0, NULL) < 7)
	return 0;

      __cpuid_count (7, 0, eax, ebx, ecx, edx);

      if ((ecx & bit_PKU) == bit_PKU)
	return 1;
    }
  return 0;
}

int
main (int argc, char **argv)
{
  unsigned int wr_value = 0x12345678;
  unsigned int rd_value = 0x0;

  if (have_pkru ())
    {
      wrpkru (wr_value);
      asm ("nop\n\t");	/* break here 1.  */

      rd_value = rdpkru ();
      asm ("nop\n\t");	/* break here 2.  */
    }
  return 0;
}
