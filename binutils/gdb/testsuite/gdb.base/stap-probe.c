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

#include "attributes.h"

#if USE_SEMAPHORES

#define _SDT_HAS_SEMAPHORES
__extension__ unsigned short test_user_semaphore __attribute__ ((unused)) __attribute__ ((section (".probes")));
#define TEST test_user_semaphore

__extension__ unsigned short test_two_semaphore __attribute__ ((unused)) __attribute__ ((section (".probes")));
#define TEST2 test_two_semaphore

__extension__ unsigned short test_m4_semaphore __attribute__ ((unused)) __attribute__ ((section (".probes")));

__extension__ unsigned short test_pstr_semaphore __attribute__ ((unused)) __attribute__ ((section (".probes")));

__extension__ unsigned short test_ps_semaphore __attribute__ ((unused)) __attribute__ ((section (".probes")));

__extension__ unsigned short test_xmmreg_semaphore __attribute__ ((unused)) __attribute__ ((section (".probes")));
#else

int relocation_marker __attribute__ ((unused));

#define TEST 1
#define TEST2 1

#endif

#include <sys/sdt.h>

/* We only support SystemTap and only the v3 form.  */
#if _SDT_NOTE_TYPE != 3
#error "not using SystemTap v3 probes"
#endif

struct funcs
{
  int val;

  const char *(*ps) (int);
};

static void
m1 (void)
{
  /* m1 and m2 are equivalent, but because of some compiler
     optimizations we have to make each of them unique.  This is why
     we have this dummy variable here.  */
  volatile int dummy = 0;

  if (TEST2)
    STAP_PROBE1 (test, two, dummy);
}

static void
m2 (void)
{
  if (TEST2)
    STAP_PROBE (test, two);
}

static int
f (int x)
{
  if (TEST)
    STAP_PROBE1 (test, user, x);
  return x+5;
}

static const char *
pstr (int val)
{
  const char *a = "This is a test message.";
  const char *b = "This is another test message.";

  STAP_PROBE3 (test, ps, a, b, val);

  return val == 0 ? a : b;
}

#ifdef __SSE2__
static const char * __attribute__((noinline))
use_xmm_reg (int val)
{
  volatile register int val_in_reg asm ("xmm0") = val;

  STAP_PROBE1 (test, xmmreg, val_in_reg);

  return val == 0 ? "xxx" : "yyy";
}
#else
static const char * __attribute__((noinline)) ATTRIBUTE_NOCLONE
use_xmm_reg (int val)
{
  /* Nothing.  */
}
#endif /* __SSE2__ */

static void
m4 (const struct funcs *fs, int v)
{
  STAP_PROBE3 (test, m4, fs->val, fs->ps (v), v);
}

int
main()
{
  struct funcs fs;

  fs.val = 42;
  fs.ps = pstr;

  f (f (23));
  m1 ();
  m2 ();

  m4 (&fs, 0);
  m4 (&fs, 1);

  use_xmm_reg (0x1234);

  return 0; /* last break here */
}
