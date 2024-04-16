/* interp.c -- AArch64 sim interface to GDB.

   Copyright (C) 2015-2024 Free Software Foundation, Inc.

   Contributed by Red Hat.

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

/* This must come before any other includes.  */
#include "defs.h"

#include <stdio.h>
#include <assert.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "ansidecl.h"
#include "bfd.h"
#include "sim/callback.h"
#include "sim/sim.h"
#include "gdb/signals.h"
#include "sim/sim-aarch64.h"

#include "sim-main.h"
#include "sim-options.h"
#include "memory.h"
#include "simulator.h"
#include "sim-assert.h"

#include "aarch64-sim.h"

/* Filter out (in place) symbols that are useless for disassembly.
   COUNT is the number of elements in SYMBOLS.
   Return the number of useful symbols. */

static long
remove_useless_symbols (asymbol **symbols, long count)
{
  asymbol **in_ptr  = symbols;
  asymbol **out_ptr = symbols;

  while (count-- > 0)
    {
      asymbol *sym = *in_ptr++;

      if (strstr (sym->name, "gcc2_compiled"))
	continue;
      if (sym->name == NULL || sym->name[0] == '\0')
	continue;
      if (sym->flags & (BSF_DEBUGGING))
	continue;
      if (   bfd_is_und_section (sym->section)
	  || bfd_is_com_section (sym->section))
	continue;
      if (sym->name[0] == '$')
	continue;

      *out_ptr++ = sym;
    }
  return out_ptr - symbols;
}

static signed int
compare_symbols (const void *ap, const void *bp)
{
  const asymbol *a = * (const asymbol **) ap;
  const asymbol *b = * (const asymbol **) bp;

  if (bfd_asymbol_value (a) > bfd_asymbol_value (b))
    return 1;
  if (bfd_asymbol_value (a) < bfd_asymbol_value (b))
    return -1;
  return 0;
}

/* Find the name of the function at ADDR.  */
const char *
aarch64_get_func (SIM_DESC sd, uint64_t addr)
{
  long symcount = STATE_PROG_SYMS_COUNT (sd);
  asymbol **symtab = STATE_PROG_SYMS (sd);
  int  min, max;

  min = -1;
  max = symcount;
  while (min < max - 1)
    {
      int sym;
      bfd_vma sa;

      sym = (min + max) / 2;
      sa = bfd_asymbol_value (symtab[sym]);

      if (sa > addr)
	max = sym;
      else if (sa < addr)
	min = sym;
      else
	{
	  min = sym;
	  break;
	}
    }

  if (min != -1)
    return bfd_asymbol_name (symtab [min]);

  return "";
}

SIM_RC
sim_create_inferior (SIM_DESC sd, struct bfd *abfd,
		     char * const *argv, char * const *env)
{
  sim_cpu *cpu = STATE_CPU (sd, 0);
  host_callback *cb = STATE_CALLBACK (sd);
  bfd_vma addr = 0;

  if (abfd != NULL)
    addr = bfd_get_start_address (abfd);

  aarch64_set_next_PC (cpu, addr);
  aarch64_update_PC (cpu);

  /* Standalone mode (i.e. `run`) will take care of the argv for us in
     sim_open() -> sim_parse_args().  But in debug mode (i.e. 'target sim'
     with `gdb`), we need to handle it because the user can change the
     argv on the fly via gdb's 'run'.  */
  if (STATE_PROG_ARGV (sd) != argv)
    {
      freeargv (STATE_PROG_ARGV (sd));
      STATE_PROG_ARGV (sd) = dupargv (argv);
    }

  if (STATE_PROG_ENVP (sd) != env)
    {
      freeargv (STATE_PROG_ENVP (sd));
      STATE_PROG_ENVP (sd) = dupargv (env);
    }

  cb->argv = STATE_PROG_ARGV (sd);
  cb->envp = STATE_PROG_ENVP (sd);

  if (trace_load_symbols (sd))
    {
      STATE_PROG_SYMS_COUNT (sd) =
	remove_useless_symbols (STATE_PROG_SYMS (sd),
				STATE_PROG_SYMS_COUNT (sd));
      qsort (STATE_PROG_SYMS (sd), STATE_PROG_SYMS_COUNT (sd),
	     sizeof (asymbol *), compare_symbols);
    }

  aarch64_init (cpu, addr);

  return SIM_RC_OK;
}

/* Read the LENGTH bytes at BUF as a little-endian value.  */

static bfd_vma
get_le (const unsigned char *buf, unsigned int length)
{
  bfd_vma acc = 0;

  while (length -- > 0)
    acc = (acc << 8) + buf[length];

  return acc;
}

/* Store VAL as a little-endian value in the LENGTH bytes at BUF.  */

static void
put_le (unsigned char *buf, unsigned int length, bfd_vma val)
{
  int i;

  for (i = 0; i < length; i++)
    {
      buf[i] = val & 0xff;
      val >>= 8;
    }
}

static int
check_regno (int regno)
{
  return 0 <= regno && regno < AARCH64_MAX_REGNO;
}

static size_t
reg_size (int regno)
{
  if (regno == AARCH64_CPSR_REGNO || regno == AARCH64_FPSR_REGNO)
    return 32;
  return 64;
}

static int
aarch64_reg_get (SIM_CPU *cpu, int regno, void *buf, int length)
{
  size_t size;
  bfd_vma val;

  if (!check_regno (regno))
    return 0;

  size = reg_size (regno);

  if (length != size)
    return 0;

  switch (regno)
    {
    case AARCH64_MIN_GR ... AARCH64_MAX_GR:
      val = aarch64_get_reg_u64 (cpu, regno, 0);
      break;

    case AARCH64_MIN_FR ... AARCH64_MAX_FR:
      val = aarch64_get_FP_double (cpu, regno - 32);
      break;

    case AARCH64_PC_REGNO:
      val = aarch64_get_PC (cpu);
      break;

    case AARCH64_CPSR_REGNO:
      val = aarch64_get_CPSR (cpu);
      break;

    case AARCH64_FPSR_REGNO:
      val = aarch64_get_FPSR (cpu);
      break;

    default:
      sim_io_eprintf (CPU_STATE (cpu),
		      "sim: unrecognized register number: %d\n", regno);
      return -1;
    }

  put_le (buf, length, val);

  return size;
}

static int
aarch64_reg_set (SIM_CPU *cpu, int regno, const void *buf, int length)
{
  size_t size;
  bfd_vma val;

  if (!check_regno (regno))
    return -1;

  size = reg_size (regno);

  if (length != size)
    return -1;

  val = get_le (buf, length);

  switch (regno)
    {
    case AARCH64_MIN_GR ... AARCH64_MAX_GR:
      aarch64_set_reg_u64 (cpu, regno, 1, val);
      break;

    case AARCH64_MIN_FR ... AARCH64_MAX_FR:
      aarch64_set_FP_double (cpu, regno - 32, (double) val);
      break;

    case AARCH64_PC_REGNO:
      aarch64_set_next_PC (cpu, val);
      aarch64_update_PC (cpu);
      break;

    case AARCH64_CPSR_REGNO:
      aarch64_set_CPSR (cpu, val);
      break;

    case AARCH64_FPSR_REGNO:
      aarch64_set_FPSR (cpu, val);
      break;

    default:
      sim_io_eprintf (CPU_STATE (cpu),
		      "sim: unrecognized register number: %d\n", regno);
      return 0;
    }

  return size;
}

static sim_cia
aarch64_pc_get (sim_cpu *cpu)
{
  return aarch64_get_PC (cpu);
}

static void
aarch64_pc_set (sim_cpu *cpu, sim_cia pc)
{
  aarch64_set_next_PC (cpu, pc);
  aarch64_update_PC (cpu);
}

static void
free_state (SIM_DESC sd)
{
  if (STATE_MODULES (sd) != NULL)
    sim_module_uninstall (sd);
  sim_cpu_free_all (sd);
  sim_state_free (sd);
}

SIM_DESC
sim_open (SIM_OPEN_KIND                  kind,
	  struct host_callback_struct *  callback,
	  struct bfd *                   abfd,
	  char * const *                 argv)
{
  sim_cpu *cpu;
  SIM_DESC sd = sim_state_alloc (kind, callback);

  if (sd == NULL)
    return sd;

  SIM_ASSERT (STATE_MAGIC (sd) == SIM_MAGIC_NUMBER);

  /* We use NONSTRICT_ALIGNMENT as the default because AArch64 only enforces
     4-byte alignment, even for 8-byte reads/writes.  The common core does not
     support this, so we opt for non-strict alignment instead.  */
  current_alignment = NONSTRICT_ALIGNMENT;

  /* Perform the initialization steps one by one.  */
  if (sim_cpu_alloc_all_extra (sd, 0, sizeof (struct aarch64_sim_cpu))
      != SIM_RC_OK
      || sim_pre_argv_init (sd, argv[0]) != SIM_RC_OK
      || sim_parse_args (sd, argv) != SIM_RC_OK
      || sim_analyze_program (sd, STATE_PROG_FILE (sd), abfd) != SIM_RC_OK
      || sim_config (sd) != SIM_RC_OK
      || sim_post_argv_init (sd) != SIM_RC_OK)
    {
      free_state (sd);
      return NULL;
    }

  aarch64_init_LIT_table ();

  assert (MAX_NR_PROCESSORS == 1);
  cpu = STATE_CPU (sd, 0);
  CPU_PC_FETCH (cpu) = aarch64_pc_get;
  CPU_PC_STORE (cpu) = aarch64_pc_set;
  CPU_REG_FETCH (cpu) = aarch64_reg_get;
  CPU_REG_STORE (cpu) = aarch64_reg_set;

  /* Set SP, FP and PC to 0 and set LR to -1
     so we can detect a top-level return.  */
  aarch64_set_reg_u64 (cpu, SP, 1, 0);
  aarch64_set_reg_u64 (cpu, FP, 1, 0);
  aarch64_set_reg_u64 (cpu, LR, 1, TOP_LEVEL_RETURN_PC);
  aarch64_set_next_PC (cpu, 0);
  aarch64_update_PC (cpu);

  /* Default to a 128 Mbyte (== 2^27) memory space.  */
  sim_do_commandf (sd, "memory-size 0x8000000");

  return sd;
}

void
sim_engine_run (SIM_DESC sd,
		int next_cpu_nr ATTRIBUTE_UNUSED,
		int nr_cpus ATTRIBUTE_UNUSED,
		int siggnal ATTRIBUTE_UNUSED)
{
  aarch64_run (sd);
}
