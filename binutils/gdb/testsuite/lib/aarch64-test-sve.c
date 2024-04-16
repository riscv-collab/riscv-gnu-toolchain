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

/* AArch64 SVE feature check.  This test serves as a way for the GDB testsuite
   to verify that a target supports SVE at runtime, and also reports data
   about the various supported SVE vector lengths.  */

#include <stdio.h>
#include <sys/auxv.h>
#include <sys/prctl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef HWCAP_SVE
#define HWCAP_SVE (1 << 22)
#endif

#ifndef PR_SVE_SET_VL
#define PR_SVE_SET_VL 50
#define PR_SVE_GET_VL 51
#define PR_SVE_VL_LEN_MASK 0xffff
#endif

static int get_vl_size ()
{
  int res = prctl (PR_SVE_GET_VL, 0, 0, 0, 0);

  if (res < 0)
    return -1;

  return (res & PR_SVE_VL_LEN_MASK);
}

static int set_vl_size (int new_vl)
{
  if (prctl (PR_SVE_SET_VL, new_vl, 0, 0, 0, 0) < 0)
    return -1;

  if (get_vl_size () != new_vl)
    return -1;

  return 0;
}

static void
dummy ()
{
}

#define VL_MIN 16
#define VL_MAX 256
#define VL_INCREMENT 16

int
main (int argc, char **argv)
{
  /* Number of supported SVE vector lengths.  */
  size_t supported_vl_count = 0;
  /* Vector containing the various supported SVE vector lengths.  */
  size_t supported_vl[16];

  if (getauxval (AT_HWCAP) & HWCAP_SVE)
    {
      for (int vl = VL_MIN; vl <= VL_MAX; vl += VL_INCREMENT)
	{
	  if (set_vl_size (vl) == 0)
	    {
	      supported_vl[supported_vl_count] = vl;
	      supported_vl_count++;
	    }
	}
    }

  return 0; /* stop here */
}
