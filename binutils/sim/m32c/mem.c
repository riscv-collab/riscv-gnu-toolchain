/* mem.c --- memory for M32C simulator.

Copyright (C) 2005-2024 Free Software Foundation, Inc.
Contributed by Red Hat, Inc.

This file is part of the GNU simulators.

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

/* This must come before any other includes.  */
#include "defs.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif

#include "mem.h"
#include "cpu.h"
#include "syscalls.h"
#include "misc.h"
#ifdef TIMER_A
#include "int.h"
#include "timer_a.h"
#endif

#define L1_BITS  (10)
#define L2_BITS  (10)
#define OFF_BITS (12)

#define L1_LEN  (1 << L1_BITS)
#define L2_LEN  (1 << L2_BITS)
#define OFF_LEN (1 << OFF_BITS)

static unsigned char **pt[L1_LEN];

#ifdef HAVE_TERMIOS_H
int m32c_console_ifd = 0;
#endif
int m32c_console_ofd = 1;
#ifdef HAVE_TERMIOS_H
int m32c_use_raw_console = 0;
#endif

#ifdef TIMER_A
Timer_A timer_a;
#endif

/* [ get=0/put=1 ][ byte size ] */
static unsigned int mem_counters[2][5];

#define COUNT(isput,bytes)                                      \
  if (verbose && enable_counting) mem_counters[isput][bytes]++

void
init_mem (void)
{
  int i, j;

  for (i = 0; i < L1_LEN; i++)
    if (pt[i])
      {
	for (j = 0; j < L2_LEN; j++)
	  if (pt[i][j])
	    free (pt[i][j]);
	free (pt[i]);
      }
  memset (pt, 0, sizeof (pt));
  memset (mem_counters, 0, sizeof (mem_counters));
}

static unsigned char *
mem_ptr (int address)
{
  static int recursing = 0;
  int pt1 = (address >> (L2_BITS + OFF_BITS)) & ((1 << L1_BITS) - 1);
  int pt2 = (address >> OFF_BITS) & ((1 << L2_BITS) - 1);
  int pto = address & ((1 << OFF_BITS) - 1);

  if (address == 0 && !recursing)
    {
      recursing = 1;
      put_reg (pc, m32c_opcode_pc);
      printf ("NULL pointer dereference at pc=0x%x\n", get_reg (pc));
      step_result = M32C_MAKE_HIT_BREAK ();
#if 0
      /* This code can be re-enabled to help diagnose NULL pointer
         bugs that aren't debuggable in GDB.  */
      m32c_dump_all_registers ();
      exit (1);
#endif
    }

  if (pt[pt1] == 0)
    pt[pt1] = (unsigned char **) calloc (L2_LEN, sizeof (char **));
  if (pt[pt1][pt2] == 0)
    {
      pt[pt1][pt2] = (unsigned char *) malloc (OFF_LEN);
      memset (pt[pt1][pt2], 0, OFF_LEN);
    }

  return pt[pt1][pt2] + pto;
}

static void
used (int rstart, int i, int j)
{
  int rend = i << (L2_BITS + OFF_BITS);
  rend += j << OFF_BITS;
  if (rstart == 0xe0000 && rend == 0xe1000)
    return;
  printf ("mem:   %08x - %08x (%dk bytes)\n", rstart, rend - 1,
	  (rend - rstart) / 1024);
}

static char *
mcs (int isput, int bytes)
{
  return comma (mem_counters[isput][bytes]);
}

void
mem_usage_stats (void)
{
  int i, j;
  int rstart = 0;
  int pending = 0;

  for (i = 0; i < L1_LEN; i++)
    if (pt[i])
      {
	for (j = 0; j < L2_LEN; j++)
	  if (pt[i][j])
	    {
	      if (!pending)
		{
		  pending = 1;
		  rstart = (i << (L2_BITS + OFF_BITS)) + (j << OFF_BITS);
		}
	    }
	  else if (pending)
	    {
	      pending = 0;
	      used (rstart, i, j);
	    }
      }
    else
      {
	if (pending)
	  {
	    pending = 0;
	    used (rstart, i, 0);
	  }
      }
  /*       mem foo: 123456789012 123456789012 123456789012 123456789012
            123456789012 */
  printf ("                 byte        short      pointer         long"
	  "        fetch\n");
  printf ("mem get: %12s %12s %12s %12s %12s\n", mcs (0, 1), mcs (0, 2),
	  mcs (0, 3), mcs (0, 4), mcs (0, 0));
  printf ("mem put: %12s %12s %12s %12s\n", mcs (1, 1), mcs (1, 2),
	  mcs (1, 3), mcs (1, 4));
}

static int tpr = 0;
static void
s (int address, char *dir)
{
  if (tpr == 0)
    printf ("MEM[%0*x] %s", membus_mask == 0xfffff ? 5 : 6, address, dir);
  tpr++;
}

#define S(d) if (trace) s(address, d)
static void
e (void)
{
  if (!trace)
    return;
  tpr--;
  if (tpr == 0)
    printf ("\n");
}

#define E() if (trace) e()

extern int m32c_disassemble;

static void
mem_put_byte (int address, unsigned char value)
{
  unsigned char *m;
  address &= membus_mask;
  m = mem_ptr (address);
  if (trace)
    printf (" %02x", value);
  *m = value;
  switch (address)
    {
    case 0x00e1:
      {
	static int old_led = -1;
	static char *led_on[] =
	  { "\033[31m O ", "\033[32m O ", "\033[34m O " };
	static char *led_off[] = { "\033[0m · ", "\033[0m · ", "\033[0m · " };
	int i;
	if (old_led != value)
	  {
	    fputs ("  ", stdout);
	    for (i = 0; i < 3; i++)
	      if (value & (1 << i))
		fputs (led_off[i], stdout);
	      else
		fputs (led_on[i], stdout);
	    fputs ("\033[0m\r", stdout);
	    fflush (stdout);
	    old_led = value;
	  }
      }
      break;
#ifdef TIMER_A
      /* M32C Timer A */
    case 0x346:		/* TA0low */
      timer_a.count = (timer_a.count & 0xff00) | value;
      timer_a.reload = timer_a.count;
      break;
    case 0x347:		/* TA0high */
      timer_a.count = (timer_a.count & 0x00ff) | (value << 8);
      timer_a.reload = timer_a.count;
      break;
    case 0x340:		/* TABSR */
      timer_a.bsr = value;
      break;
    case 0x356:		/* TA0MR */
      timer_a.mode = value;
      break;
    case 0x35f:		/* TCSPR */
      timer_a.tcspr = value;
      break;
    case 0x006c:		/* TA0IC */
      timer_a.ic = value;
      break;

      /* R8C Timer RA */
    case 0x100:		/* TRACR */
      timer_a.bsr = value;
      break;
    case 0x102:		/* TRAMR */
      timer_a.mode = value;
      break;
    case 0x104:		/* TRA */
      timer_a.count = value;
      timer_a.reload = value;
      break;
    case 0x103:		/* TRAPRE */
      timer_a.tcspr = value;
      break;
    case 0x0056:		/* TA0IC */
      timer_a.ic = value;
      break;
#endif

    case 0x2ea:		/* m32c uart1tx */
    case 0x3aa:		/* m16c uart1tx */
      {
	static int pending_exit = 0;
	if (value == 0)
	  {
	    if (pending_exit)
	      {
		step_result = M32C_MAKE_EXITED (value);
		return;
	      }
	    pending_exit = 1;
	  }
	else
	  {
	    if (write (m32c_console_ofd, &value, 1) != 1)
	      printf ("write console failed: %s\n", strerror (errno));
	  }
      }
      break;

    case 0x400:
      m32c_syscall (value);
      break;

    case 0x401:
      putchar (value);
      break;

    case 0x402:
      printf ("SimTrace: %06lx %02x\n", regs.r_pc, value);
      break;

    case 0x403:
      printf ("SimTrap: %06lx %02x\n", regs.r_pc, value);
      abort ();
    }
}

void
mem_put_qi (int address, unsigned char value)
{
  S ("<=");
  mem_put_byte (address, value & 0xff);
  E ();
  COUNT (1, 1);
}

void
mem_put_hi (int address, unsigned short value)
{
  if (address == 0x402)
    {
      printf ("SimTrace: %06lx %04x\n", regs.r_pc, value);
      return;
    }
  S ("<=");
  mem_put_byte (address, value & 0xff);
  mem_put_byte (address + 1, value >> 8);
  E ();
  COUNT (1, 2);
}

void
mem_put_psi (int address, unsigned long value)
{
  S ("<=");
  mem_put_byte (address, value & 0xff);
  mem_put_byte (address + 1, (value >> 8) & 0xff);
  mem_put_byte (address + 2, value >> 16);
  E ();
  COUNT (1, 3);
}

void
mem_put_si (int address, unsigned long value)
{
  S ("<=");
  mem_put_byte (address, value & 0xff);
  mem_put_byte (address + 1, (value >> 8) & 0xff);
  mem_put_byte (address + 2, (value >> 16) & 0xff);
  mem_put_byte (address + 3, (value >> 24) & 0xff);
  E ();
  COUNT (1, 4);
}

void
mem_put_blk (int address, const void *bufptr, int nbytes)
{
  const unsigned char *buf = bufptr;

  S ("<=");
  if (enable_counting)
    mem_counters[1][1] += nbytes;
  while (nbytes--)
    mem_put_byte (address++, *buf++);
  E ();
}

unsigned char
mem_get_pc (void)
{
  unsigned char *m = mem_ptr (regs.r_pc & membus_mask);
  COUNT (0, 0);
  return *m;
}

#ifdef HAVE_TERMIOS_H
static int console_raw = 0;
static struct termios oattr;

static int
stdin_ready (void)
{
  fd_set ifd;
  int n;
  struct timeval t;

  t.tv_sec = 0;
  t.tv_usec = 0;
  FD_ZERO (&ifd);
  FD_SET (m32c_console_ifd, &ifd);
  n = select (1, &ifd, 0, 0, &t);
  return n > 0;
}

void
m32c_sim_restore_console (void)
{
  if (console_raw)
    tcsetattr (m32c_console_ifd, TCSANOW, &oattr);
  console_raw = 0;
}
#endif

static unsigned char
mem_get_byte (int address)
{
  unsigned char *m;
  address &= membus_mask;
  m = mem_ptr (address);
  switch (address)
    {
#ifdef HAVE_TERMIOS_H
    case 0x2ed:		/* m32c uart1c1 */
    case 0x3ad:		/* m16c uart1c1 */

      if (!console_raw && m32c_use_raw_console)
	{
	  struct termios attr;
	  tcgetattr (m32c_console_ifd, &attr);
	  tcgetattr (m32c_console_ifd, &oattr);
	  /* We want each key to be sent as the user presses them.  */
	  attr.c_lflag &= ~(ICANON | ECHO | ECHOE);
	  tcsetattr (m32c_console_ifd, TCSANOW, &attr);
	  console_raw = 1;
	  atexit (m32c_sim_restore_console);
	}

      if (stdin_ready ())
	return 0x02;		/* tx empty and rx full */
      else
	return 0x0a;		/* transmitter empty */

    case 0x2ee:		/* m32c uart1 rx */
      {
	char c;
	if (read (m32c_console_ifd, &c, 1) != 1)
	  return 0;
	if (m32c_console_ifd == 0 && c == 3)	/* Ctrl-C */
	  {
	    printf ("Ctrl-C!\n");
	    exit (0);
	  }

	if (m32c_console_ifd != 1)
	  {
	    if (isgraph (c))
	      printf ("\033[31m%c\033[0m", c);
	    else
	      printf ("\033[31m%02x\033[0m", c);
	  }
	return c;
      }
#endif

#ifdef TIMER_A
    case 0x346:		/* TA0low */
      return timer_a.count & 0xff;
    case 0x347:		/* TA0high */
      return (timer_a.count >> 8) & 0xff;
    case 0x104:		/* TRA */
      return timer_a.count;
#endif

    default:
      /* In case both cases above are not included.  */
      ;
    }

  S ("=>");
  if (trace)
    printf (" %02x", *m);
  E ();
  return *m;
}

unsigned char
mem_get_qi (int address)
{
  unsigned char rv;
  S ("=>");
  rv = mem_get_byte (address);
  COUNT (0, 1);
  E ();
  return rv;
}

unsigned short
mem_get_hi (int address)
{
  unsigned short rv;
  S ("=>");
  rv = mem_get_byte (address);
  rv |= mem_get_byte (address + 1) * 256;
  COUNT (0, 2);
  E ();
  return rv;
}

unsigned long
mem_get_psi (int address)
{
  unsigned long rv;
  S ("=>");
  rv = mem_get_byte (address);
  rv |= mem_get_byte (address + 1) * 256;
  rv |= mem_get_byte (address + 2) * 65536;
  COUNT (0, 3);
  E ();
  return rv;
}

unsigned long
mem_get_si (int address)
{
  unsigned long rv;
  S ("=>");
  rv = mem_get_byte (address);
  rv |= mem_get_byte (address + 1) << 8;
  rv |= mem_get_byte (address + 2) << 16;
  rv |= mem_get_byte (address + 3) << 24;
  COUNT (0, 4);
  E ();
  return rv;
}

void
mem_get_blk (int address, void *bufptr, int nbytes)
{
  char *buf = bufptr;

  S ("=>");
  if (enable_counting)
    mem_counters[0][1] += nbytes;
  while (nbytes--)
    *buf++ = mem_get_byte (address++);
  E ();
}

int
sign_ext (int v, int bits)
{
  if (bits < 32)
    {
      v &= (1 << bits) - 1;
      if (v & (1 << (bits - 1)))
	v -= (1 << bits);
    }
  return v;
}

#if TIMER_A
void
update_timer_a (void)
{
  if (timer_a.bsr & 1)
    {
      timer_a.prescale--;
      if (timer_a.prescale < 0)
	{
	  if (A24)
	    {
	      switch (timer_a.mode & 0xc0)
		{
		case 0x00:
		  timer_a.prescale = 0;
		  break;
		case 0x40:
		  timer_a.prescale = 8;
		  break;
		case 0x80:
		  timer_a.prescale = timer_a.tcspr & 0x0f;
		  break;
		case 0xc0:
		  timer_a.prescale = 32;
		  break;
		}
	    }
	  else
	    {
	      timer_a.prescale = timer_a.tcspr;
	    }
	  timer_a.count--;
	  if (timer_a.count < 0)
	    {
	      timer_a.count = timer_a.reload;
	      if (timer_a.ic & 7)
		{
		  if (A24)
		    mem_put_qi (0x6c, timer_a.ic | 0x08);
		  else
		    mem_put_qi (0x56, timer_a.ic | 0x08);
		}
	    }
	}
    }

  if (regs.r_flags & FLAGBIT_I	/* interrupts enabled */
      && timer_a.ic & 0x08	/* timer A interrupt triggered */
      && (timer_a.ic & 0x07) > ((regs.r_flags >> 12) & 0x07))
    {
      if (A24)
	trigger_peripheral_interrupt (12, 0x06c);
      else
	trigger_peripheral_interrupt (22, 0x056);
    }
}
#endif
