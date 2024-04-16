/* This testcase is part of GDB, the GNU debugger.

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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#define _GNU_SOURCE
#include <dlfcn.h>
#include <stddef.h>
#include <assert.h>
#include <unistd.h>

volatile int wait_for_gdb = 1;

int
main (void)
{
  void *handle[4];
  int (*fun) (int);
  Lmid_t lmid;
  int dl;

  handle[0] = dlmopen (LM_ID_NEWLM, DSO1_NAME, RTLD_LAZY | RTLD_LOCAL);
  assert (handle[0] != NULL);

  dlinfo (handle[0], RTLD_DI_LMID, &lmid);

  handle[1] = dlopen (DSO1_NAME, RTLD_LAZY | RTLD_LOCAL);
  assert (handle[1] != NULL);

  handle[2] = dlmopen (LM_ID_NEWLM, DSO1_NAME, RTLD_LAZY | RTLD_LOCAL);
  assert (handle[2] != NULL);

  handle[3] = dlmopen (lmid, DSO2_NAME, RTLD_LAZY | RTLD_LOCAL);
  assert (handle[3] != NULL);

  alarm (20);
  while (wait_for_gdb != 0)
    usleep (1);

  for (dl = 0; dl < 4; ++dl)
    {
      fun = dlsym (handle[dl], "inc");
      assert (fun != NULL);

      fun (42);

      dlclose (handle[dl]);
    }

  return 0;  /* bp.main  */
}
