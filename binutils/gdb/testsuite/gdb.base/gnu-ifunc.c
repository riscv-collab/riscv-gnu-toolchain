/* This testcase is part of GDB, the GNU debugger.

   Copyright 2009-2024 Free Software Foundation, Inc.

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

#include <assert.h>

int
init_stub (int arg)
{
  return 0;
}

/* Make differentiation of how the gnu_ifunc call resolves before and after
   calling gnu_ifunc_pre.  This ensures the resolved function address is not
   being cached anywhere for the debugging purposes.  */

volatile int gnu_ifunc_initialized;

/* This stores the argument received by the ifunc resolver.  */

volatile unsigned long resolver_hwcap = -1;

static void
gnu_ifunc_pre (void)
{
  assert (!gnu_ifunc_initialized);

  gnu_ifunc_initialized = 1;
}

extern int gnu_ifunc (int arg);

int
main (void)
{
  int i;

  gnu_ifunc_pre ();
  
  i = gnu_ifunc (1);	/* break-at-call */
  assert (i == 2);

  gnu_ifunc (2);	/* break-at-nextcall */

  return 0;	/* break-at-exit */
}
