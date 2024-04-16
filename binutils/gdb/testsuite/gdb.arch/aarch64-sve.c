/* This testcase is part of GDB, the GNU debugger.

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

/* Exercise AArch64's Scalable Vector Extension.

   This test was based on QEMU's sve-ioctls.c test file.  */

#include <stdio.h>
#include <sys/auxv.h>
#include <sys/prctl.h>

static int
do_sve_ioctl_test (void)
{
  int i, res, init_vl;

  res = prctl (PR_SVE_GET_VL, 0, 0, 0, 0);
  if (res < 0)
    {
      printf ("FAILED to PR_SVE_GET_VL (%d)", res);
      return -1;
    }
  init_vl = res & PR_SVE_VL_LEN_MASK;

  for (i = init_vl; i > 15; i /= 2)
    {
      printf ("Checking PR_SVE_SET_VL=%d\n", i);
      res = prctl (PR_SVE_SET_VL, i, 0, 0, 0, 0); /* break here */
      if (res < 0)
	{
	  printf ("FAILED to PR_SVE_SET_VL (%d)", res);
	  return -1;
	}
    }
  return 0;
}

int
main (int argc, char **argv)
{
  if (getauxval (AT_HWCAP) & HWCAP_SVE)
    {
      return do_sve_ioctl_test ();
    }
  else
    {
      printf ("SKIP: no HWCAP_SVE on this system\n");
      return 1;
    }
}
