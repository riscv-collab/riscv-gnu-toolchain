/* Copyright (C) 2014-2024 Free Software Foundation, Inc.

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

#include <string.h>
#ifdef SIGNALS
#include <signal.h>
#include <unistd.h>
#endif

/* NOP instruction: must have the same size as the breakpoint
   instruction.  */

#if defined(__s390__) || defined(__s390x__)
#define NOP asm("nopr 0")
#elif defined(__or1k__)
#define NOP asm("l.nop")
#else
#define NOP asm("nop")
#endif

/* Buffer holding the breakpoint instruction.  */
unsigned char buffer[16];

volatile unsigned char *addr_bp;
volatile unsigned char *addr_after_bp;
int counter = 0;

void
test (void)
{
  NOP;
  NOP;
  NOP;
  NOP; /* write permanent bp here */
  NOP; /* after permanent bp */
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;

  counter++;
}

void
setup (void)
{
  memcpy (buffer, (void *) addr_bp, addr_after_bp - addr_bp);
}

void
test_basics (void)
{
  test (); /* for SIGTRAP */
  test (); /* for breakpoint once */
  test (); /* for breakpoint twice */
  test (); /* for disabled bp SIGTRAP */
  test (); /* for breakpoint thrice */
}

void
test_next (void)
{
  test (); /* for next */
  counter = 0; /* after next */
}

#ifdef SIGNALS

static void
test_signal_handler (int sig)
{
}

void
test_signal_with_handler (void)
{
  signal (SIGUSR1, test_signal_handler);
  test ();
}

void
test_signal_no_handler (void)
{
  signal (SIGUSR1, SIG_IGN);
  test ();
}

static void
test_signal_nested_handler ()
{
  test ();
}

void
test_signal_nested_done (void)
{
}

void
test_signal_nested (void)
{
  counter = 0;
  signal (SIGALRM, test_signal_nested_handler);
  alarm (1);
  test ();
  test_signal_nested_done ();
}

#endif

int
main (void)
{
  setup ();
  test_basics ();
  test_next ();
#ifdef SIGNALS
  test_signal_nested ();
  test_signal_with_handler ();
  test_signal_no_handler ();
#endif
  return 0;
}
