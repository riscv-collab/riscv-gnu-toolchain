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

/* Exercise various cases of reading/writing ZA contents for AArch64's
   Scalable Matrix Extension.  */

#include <stdio.h>
#include <sys/auxv.h>
#include <sys/prctl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef HWCAP_SVE
#define HWCAP_SVE (1 << 22)
#endif

#ifndef HWCAP2_SME
#define HWCAP2_SME (1 << 23)
#endif

#ifndef PR_SVE_SET_VL
#define PR_SVE_SET_VL 50
#define PR_SVE_GET_VL 51
#define PR_SVE_VL_LEN_MASK 0xffff
#endif

#ifndef PR_SME_SET_VL
#define PR_SME_SET_VL 63
#define PR_SME_GET_VL 64
#define PR_SME_VL_LEN_MASK 0xffff
#endif

static void
enable_za ()
{
  /* smstart za */
  __asm __volatile (".word 0xD503457F");
}

static void
disable_za ()
{
  /* smstop za */
  __asm __volatile (".word 0xD503447F");
}

static int get_vl_size ()
{
  int res = prctl (PR_SVE_GET_VL, 0, 0, 0, 0);
  if (res < 0)
    {
      printf ("FAILED to PR_SVE_GET_VL (%d)\n", res);
      return -1;
    }
  return (res & PR_SVE_VL_LEN_MASK);
}

static int get_svl_size ()
{
  int res = prctl (PR_SME_GET_VL, 0, 0, 0, 0);
  if (res < 0)
    {
      printf ("FAILED to PR_SME_GET_VL (%d)\n", res);
      return -1;
    }
  return (res & PR_SVE_VL_LEN_MASK);
}

static int set_vl_size (int new_vl)
{
  int res = prctl (PR_SVE_SET_VL, new_vl, 0, 0, 0, 0);
  if (res < 0)
    {
      printf ("FAILED to PR_SVE_SET_VL (%d)\n", res);
      return -1;
    }

  res = get_vl_size ();
  if (res != new_vl)
    {
      printf ("Unexpected VL value (%d)\n", res);
      return -1;
    }

  return res;
}

static int set_svl_size (int new_svl)
{
  int res = prctl (PR_SME_SET_VL, new_svl, 0, 0, 0, 0);
  if (res < 0)
    {
      printf ("FAILED to PR_SME_SET_VL (%d)\n", res);
      return -1;
    }

  res = get_svl_size ();
  if (res != new_svl)
    {
      printf ("Unexpected SVL value (%d)\n", res);
      return -1;
    }

  return res;
}

static int
test_id_to_vl (int id)
{
  return 16 << ((id / 5) % 5);
}

static int
test_id_to_svl (int id)
{
  return 16 << (id % 5);
}

static void
dummy ()
{
}

int
main (int argc, char **argv)
{
  if (getauxval (AT_HWCAP) & HWCAP_SVE && getauxval (AT_HWCAP2) & HWCAP2_SME)
    {
      int id_start = ID_START;
      int id_end = ID_END;

      for (int id = id_start; id <= id_end; id++)
	{
	  int vl = test_id_to_vl (id);
	  int svl = test_id_to_svl (id);

	  if (set_vl_size (vl) == -1 || set_svl_size (svl) == -1)
	    continue;

	  enable_za ();
	  dummy (); /* stop 1 */
	}

      for (int id = id_start; id <= id_end; id++)
	{
	  int vl = test_id_to_vl (id);
	  int svl = test_id_to_svl (id);

	  if (set_vl_size (vl) == -1 || set_svl_size (svl) == -1)
	    continue;

	  disable_za ();
	  dummy (); /* stop 2 */
	}
    }
  else
    {
      printf ("SKIP: no HWCAP_SVE or HWCAP2_SME on this system\n");
      return -1;
    }

  return 0;
}
