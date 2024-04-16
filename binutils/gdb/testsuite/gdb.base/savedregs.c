/* This testcase is part of GDB, the GNU debugger.

   Copyright 2004-2024 Free Software Foundation, Inc.

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

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>

static volatile int done;

extern int
callee (int param)
{
  return param * done + 1;
}

extern int
caller (int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
  return callee ((a1 << a2 * a3 / a4) + a6 & a6 % a7 - a8) + done;
}

static void
catcher (int sig)
{
  done = 1;
} /* handler */

static void
thrower (void)
{
  /* Trigger a SIGSEGV.  */
  *(volatile char *)0 = 0;

  /* On MMU-less system, previous memory access to address zero doesn't
     trigger a SIGSEGV.  Trigger a SIGILL.  Each arch should define its
     own illegal instruction here.  */

#if defined(__arm__)
  asm(".word 0xf8f00000");
#elif defined(__TMS320C6X__)
  asm(".word 0x56454313");
#else
#endif

}

int
main ()
{
  signal (SIGILL, catcher);
  signal (SIGSEGV, catcher);
  thrower ();
  return 0;
}
