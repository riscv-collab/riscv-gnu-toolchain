/* This testcase is part of GDB, the GNU debugger.

   Copyright 2023-2024 Free Software Foundation, Inc.

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

/* AArch64 SME feature check.  This test serves as a way for the GDB testsuite
   to verify that a target supports SVE at runtime, and also reports data
   about the various supported SME streaming vector lengths.  */

#include <stdio.h>
#include <sys/auxv.h>
#include <sys/prctl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef HWCAP2_SME
#define HWCAP2_SME (1 << 23)
#endif

#ifndef PR_SME_SET_VL
#define PR_SME_SET_VL 63
#define PR_SME_GET_VL 64
#define PR_SME_VL_LEN_MASK 0xffff
#endif

static int get_svl_size ()
{
  int res = prctl (PR_SME_GET_VL, 0, 0, 0, 0);

  if (res < 0)
    return -1;

  return (res & PR_SME_VL_LEN_MASK);
}

static int set_svl_size (int new_svl)
{
  if (prctl (PR_SME_SET_VL, new_svl, 0, 0, 0, 0) < 0)
    return -1;

  if (get_svl_size () != new_svl)
    return -1;

  return 0;
}

static void
dummy ()
{
}

#define SVL_MIN 16
#define SVL_MAX 256
#define SVL_INCREMENT_POWER 1

int
main (int argc, char **argv)
{
  /* Number of supported SME streaming vector lengths.  */
  size_t supported_svl_count = 0;
  /* Vector containing the various supported SME streaming vector lengths.  */
  size_t supported_svl[5];

  if (getauxval (AT_HWCAP) & HWCAP2_SME)
    {
      for (int svl = SVL_MIN; svl <= SVL_MAX; svl <<= SVL_INCREMENT_POWER)
	{
	  if (set_svl_size (svl) == 0)
	    {
	      supported_svl[supported_svl_count] = svl;
	      supported_svl_count++;
	    }
	}
    }

  return 0; /* stop here */
}
