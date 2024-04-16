/* This testcase is part of GDB, the GNU debugger.

   Copyright 2012-2024 Free Software Foundation, Inc.

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

#if USE_PROBES

#define _SDT_HAS_SEMAPHORES
__extension__ unsigned short test_user_semaphore __attribute__ ((unused)) __attribute__ ((section (".probes")));
#define TEST test_user_semaphore

__extension__ unsigned short test_two_semaphore __attribute__ ((unused)) __attribute__ ((section (".probes")));
#define TEST2 test_two_semaphore

#else

#define TEST 1
#define TEST2 1

#endif /* USE_PROBES */

#include <sys/sdt.h>

/* We only support SystemTap and only the v3 form.  */
#if _SDT_NOTE_TYPE != 3
#error "not using SystemTap v3 probes"
#endif

void
m1 (int x)
{
  if (TEST2)
    STAP_PROBE1 (test, two, x);
}

int
f (int x)
{
  if (TEST)
    STAP_PROBE1(test, user, x);
  return x+5;
}

void
nothing (void)
{
  int a = 1 + 1;
  return;
}

int
main()
{
  f (f (23));
  m1 (46);
  nothing (); /* end-here */

  return 0;
}
