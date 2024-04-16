/* mem.c --- memory for RL78 simulator.

   Copyright (C) 2011-2024 Free Software Foundation, Inc.
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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* This must come before any other includes.  */
#include "defs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "opcode/rl78.h"
#include "mem.h"
#include "cpu.h"

#define ILLEGAL_OPCODE 0xff

int rom_limit = 0x100000;
int ram_base = 0xf8000;
unsigned char memory[MEM_SIZE];
#define MASK 0xfffff

unsigned char initted[MEM_SIZE];
int skip_init = 0;

#define tprintf if (trace) printf

void
init_mem (void)
{
  memset (memory, ILLEGAL_OPCODE, sizeof (memory));
  memset (memory + 0xf0000, 0x33, 0x10000);

  memset (initted, 0, sizeof (initted));
  memset (initted + 0xffee0, 1, 0x00120);
  memset (initted + 0xf0000, 1, 0x01000);
}

void
mem_ram_size (int ram_bytes)
{
  ram_base = 0x100000 - ram_bytes;
}

void
mem_rom_size (int rom_bytes)
{
  rom_limit = rom_bytes;
}

static int mirror_rom_base = 0x01000;
static int mirror_ram_base = 0xf1000;
static int mirror_length = 0x7000;

void
mem_set_mirror (int rom_base, int ram_base, int length)
{
  mirror_rom_base = rom_base;
  mirror_ram_base = ram_base;
  mirror_length = length;
}

/* ---------------------------------------------------------------------- */
/* Note: the RL78 memory map has a few surprises.  For starters, part
   of the first 64k is mapped to the last 64k, depending on an SFR bit
   and how much RAM the chip has.  This is simulated here, as are a
   few peripherals.  */

/* This is stdout.  We only care about the data byte, not the upper byte.  */
#define SDR00	0xfff10
#define SSR00	0xf0100
#define TS0	0xf01b2

/* RL78/G13 multiply/divide peripheral.  */
#define MDUC	0xf00e8
#define MDAL	0xffff0
#define MDAH	0xffff2
#define MDBL	0xffff6
#define MDBH	0xffff4
#define MDCL	0xf00e0
#define MDCH	0xf00e2
static long long mduc_clock = 0;
static int mda_set = 0;
#define MDA_SET  15

static int last_addr_was_mirror;

static int
address_mapping (int address)
{
  address &= MASK;
  if (address >= mirror_ram_base && address < mirror_ram_base + mirror_length)
    {
      address = address - mirror_ram_base + mirror_rom_base;
      if (memory[RL78_SFR_PMC] & 1)
	{
	  address |= 0x10000;
	}
      last_addr_was_mirror = 1;
    }
  else
      last_addr_was_mirror = 0;
    
  return address;
}

static void
mem_put_byte (int address, unsigned char value)
{
  address = address_mapping (address);
  memory [address] = value;
  initted [address] = 1;
  if (address == SDR00)
    {
      putchar (value);
      fflush (stdout);
    }
  if (address == TS0)
    {
      if (timer_enabled == 2)
	{
	  total_clocks = 0;
	  pending_clocks = 0;
	  memset (counts_per_insn, 0, sizeof (counts_per_insn));
	  memory[0xf0180] = 0xff;
	  memory[0xf0181] = 0xff;
	}
      if (value & 1)
	timer_enabled = 1;
      else
	timer_enabled = 0;
    }
  if (address == RL78_SFR_SP && value & 1)
    {
      printf ("Warning: SP value 0x%04x truncated at pc=0x%05x\n", value, pc);
      value &= ~1;
    }

  if (! g13_multiply)
    return;

  if (address == MDUC)
    {
      if ((value & 0x81) == 0x81)
	{
	  /* division */
	  mduc_clock = total_clocks;
	}
    }
  if ((address & ~3) == MDAL)
    {
      mda_set |= (1 << (address & 3));
      if (mda_set == MDA_SET)
	{
	  long als, ahs;
	  unsigned long alu, ahu;
	  long rvs;
	  long mdc;
	  unsigned long rvu;
	  mda_set = 0;
	  switch (memory [MDUC] & 0xc8)
	    {
	    case 0x00:
	      alu = mem_get_hi (MDAL);
	      ahu = mem_get_hi (MDAH);
	      rvu = alu * ahu;
	      tprintf  ("MDUC: %lu * %lu = %lu\n", alu, ahu, rvu);
	      mem_put_hi (MDBL, rvu & 0xffff);
	      mem_put_hi (MDBH, rvu >> 16);
	      break;
	    case 0x08:
	      als = sign_ext (mem_get_hi (MDAL), 16);
	      ahs = sign_ext (mem_get_hi (MDAH), 16);
	      rvs = als * ahs;
	      tprintf  ("MDUC: %ld * %ld = %ld\n", als, ahs, rvs);
	      mem_put_hi (MDBL, rvs & 0xffff);
	      mem_put_hi (MDBH, rvs >> 16);
	      break;
	    case 0x40:
	      alu = mem_get_hi (MDAL);
	      ahu = mem_get_hi (MDAH);
	      rvu = alu * ahu;
	      mem_put_hi (MDBL, rvu & 0xffff);
	      mem_put_hi (MDBH, rvu >> 16);
	      mdc = mem_get_si (MDCL);
	      tprintf  ("MDUC: %lu * %lu + %lu = ", alu, ahu, mdc);
	      mdc += (long) rvu;
	      tprintf ("%lu\n", mdc);
	      mem_put_si (MDCL, mdc);
	      break;
	    case 0x48:
	      als = sign_ext (mem_get_hi (MDAL), 16);
	      ahs = sign_ext (mem_get_hi (MDAH), 16);
	      rvs = als * ahs;
	      mem_put_hi (MDBL, rvs & 0xffff);
	      mem_put_hi (MDBH, rvs >> 16);
	      mdc = mem_get_si (MDCL);
	      tprintf  ("MDUC: %ld * %ld + %ld = ", als, ahs, mdc);
	      tprintf ("%ld\n", mdc);
	      mdc += rvs;
	      mem_put_si (MDCL, mdc);
	      break;
	    }
	}
    }
}

extern long long total_clocks;

static unsigned char
mem_get_byte (int address)
{
  address = address_mapping (address);
  switch (address)
    {
    case SSR00:
    case SSR00 + 1:
      return 0x00;
    case 0xf00f0:
      return 0;
    case 0xf0180:
    case 0xf0181:
      return memory[address];

    case MDUC:
      {
	unsigned char mduc = memory [MDUC];
	if ((mduc & 0x81) == 0x81
	    && total_clocks > mduc_clock + 16)
	  {
	    unsigned long a, b, q, r;
	    memory [MDUC] &= 0xfe;
	    a = mem_get_si (MDAL);
	    b = mem_get_hi (MDBL) | (mem_get_hi (MDBH) << 16);
	    if (b == 0)
	      {
		q = ~0;
		r = ~0;
	      }
	    else
	      {
		q = a / b;
		r = a % b;
	      }
	    tprintf  ("MDUC: %lu / %lu = q %lu, r %lu\n", a, b, q, r);
	    mem_put_si (MDAL, q);
	    mem_put_si (MDCL, r);
	  }
	return memory[address];
      }
    case MDCL:
    case MDCL + 1:
    case MDCH:
    case MDCH + 1:
      return memory[address];
    }
  if (address < 0xf1000 && address >= 0xf0000)
    {
#if 1
      /* Note: comment out this return to trap the invalid access
	 instead of returning an "undefined" value.  */
      return 0x11;
#else
      fprintf (stderr, "SFR access error: addr 0x%05x pc 0x%05x\n", address, pc);
      exit (1);
#endif
    }
#if 0
  /* Uncomment this block if you want to trap on reads from unwritten memory.  */
  if (!skip_init && !initted [address])
    {
      static int uninit_count = 0;
      fprintf (stderr, "\033[31mwarning :read from uninit addr %05x pc %05x\033[0m\n", address, pc);
      uninit_count ++;
      if (uninit_count > 5)
	exit (1);
    }
#endif
  return memory [address];
}

extern jmp_buf decode_jmp_buf;
#define DO_RETURN(x) longjmp (decode_jmp_buf, x)

#define CHECK_ALIGNMENT(a,v,m) \
  if (a & m) { printf ("Misalignment addr 0x%05x val 0x%04x pc %05x\n", (int)a, (int)v, (int)pc); \
    DO_RETURN (RL78_MAKE_HIT_BREAK ()); }

/* ---------------------------------------------------------------------- */
#define SPECIAL_ADDR(a) (0xffff0 <= a || (0xffee0 <= a && a < 0xfff00))

void
mem_put_qi (int address, unsigned char value)
{
  if (!SPECIAL_ADDR (address))
    tprintf ("\033[34m([%05X]<-%02X)\033[0m", address, value);
  mem_put_byte (address, value);
}

void
mem_put_hi (int address, unsigned short value)
{
  if (!SPECIAL_ADDR (address))
    tprintf ("\033[34m([%05X]<-%04X)\033[0m", address, value);
  CHECK_ALIGNMENT (address, value, 1);
  if (address > 0xffff8 && address != RL78_SFR_SP)
    {
      tprintf ("Word access to 0x%05x!!\n", address);
      DO_RETURN (RL78_MAKE_HIT_BREAK ());
    }
  mem_put_byte (address, value);
  mem_put_byte (address + 1, value >> 8);
}

void
mem_put_psi (int address, unsigned long value)
{
  tprintf ("\033[34m([%05X]<-%06lX)\033[0m", address, value);
  mem_put_byte (address, value);
  mem_put_byte (address + 1, value >> 8);
  mem_put_byte (address + 2, value >> 16);
}

void
mem_put_si (int address, unsigned long value)
{
  tprintf ("\033[34m([%05X]<-%08lX)\033[0m", address, value);
  CHECK_ALIGNMENT (address, value, 3);
  mem_put_byte (address, value);
  mem_put_byte (address + 1, value >> 8);
  mem_put_byte (address + 2, value >> 16);
  mem_put_byte (address + 3, value >> 24);
}

void
mem_put_blk (int address, const void *bufptr, int nbytes)
{
  const unsigned char *bp = (unsigned char *)bufptr;
  while (nbytes --)
    mem_put_byte (address ++, *bp ++);
}

unsigned char
mem_get_pc (int address)
{
  /* Catch obvious problems.  */
  if (address >= rom_limit && address < 0xf0000)
    return 0xff;
  /* This does NOT go through the flash mirror area; you cannot
     execute out of the mirror.  */
  return memory [address & MASK];
}

unsigned char
mem_get_qi (int address)
{
  int v;
  v = mem_get_byte (address);
  if (!SPECIAL_ADDR (address))
    tprintf ("\033[35m([%05X]->%04X)\033[0m", address, v);
  if (last_addr_was_mirror)
    {
      pending_clocks += 3;
      tprintf ("ROM read\n");
    }
  return v;
}

unsigned short
mem_get_hi (int address)
{
  int v;
  v = mem_get_byte (address)
    | mem_get_byte (address + 1) * 256;
  CHECK_ALIGNMENT (address, v, 1);
  if (!SPECIAL_ADDR (address))
    tprintf ("\033[35m([%05X]->%04X)\033[0m", address, v);
  if (last_addr_was_mirror)
    {
      pending_clocks += 3;
      tprintf ("ROM read\n");
    }
  return v;
}

unsigned long
mem_get_psi (int address)
{
  int v;
  v = mem_get_byte (address)
    | mem_get_byte (address + 1) * 256
    | mem_get_byte (address + 2) * 65536;
  tprintf ("\033[35m([%05X]->%04X)\033[0m", address, v);
  return v;
}

unsigned long
mem_get_si (int address)
{
  int v;
  v = mem_get_byte (address)
    | mem_get_byte (address + 1) * 256
    | mem_get_byte (address + 2) * 65536
    | mem_get_byte (address + 2) * 16777216;
  CHECK_ALIGNMENT (address, v, 3);
  tprintf ("(\033[35m[%05X]->%04X)\033[0m", address, v);
  return v;
}

void
mem_get_blk (int address, void *bufptr, int nbytes)
{
  unsigned char *bp = (unsigned char *)bufptr;
  while (nbytes --)
    *bp ++ = mem_get_byte (address ++);
}

int
sign_ext (int v, int bits)
{
  if (bits < 8 * sizeof (int))
    {
      v &= (1 << bits) - 1;
      if (v & (1 << (bits - 1)))
	v -= (1 << bits);
    }
  return v;
}
