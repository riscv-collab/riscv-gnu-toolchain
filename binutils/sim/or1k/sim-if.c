/* Main simulator entry points specific to the OR1K.
   Copyright (C) 2017-2024 Free Software Foundation, Inc.

   This file is part of GDB, the GNU debugger.

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

#include "sim-main.h"
#include "sim-options.h"
#include "libiberty.h"
#include "bfd.h"

#include <string.h>
#include <stdlib.h>

static void free_state (SIM_DESC);


/* Cover function of sim_state_free to free the cpu buffers as well.  */

static void
free_state (SIM_DESC sd)
{
  if (STATE_MODULES (sd) != NULL)
    sim_module_uninstall (sd);
  sim_cpu_free_all (sd);
  sim_state_free (sd);
}

/* Defaults for user passed arguments.  */
static const USI or1k_default_vr = 0x0;
static const USI or1k_default_upr = 0x0
  | SPR_FIELD_MASK_SYS_UPR_UP;
static const USI or1k_default_cpucfgr = 0x0
  | SPR_FIELD_MASK_SYS_CPUCFGR_OB32S
  | SPR_FIELD_MASK_SYS_CPUCFGR_OF32S;

static UWI or1k_upr;
static UWI or1k_vr;
static UWI or1k_cpucfgr;

enum
{
  OPTION_OR1K_VR,
  OPTION_OR1K_UPR,
  OPTION_OR1K_CPUCFGR = OPTION_START,
};

/* Setup help and handlers for the user defined arguments.  */
DECLARE_OPTION_HANDLER (or1k_option_handler);

static const OPTION or1k_options[] = {
  {{"or1k-cpucfgr", required_argument, NULL, OPTION_OR1K_CPUCFGR},
   '\0', "INTEGER|default", "Set simulator CPUCFGR value",
   or1k_option_handler},
  {{"or1k-vr", required_argument, NULL, OPTION_OR1K_VR},
   '\0', "INTEGER|default", "Set simulator VR value",
   or1k_option_handler},
  {{"or1k-upr", required_argument, NULL, OPTION_OR1K_UPR},
   '\0', "INTEGER|default", "Set simulator UPR value",
   or1k_option_handler},
  {{NULL, no_argument, NULL, 0}, '\0', NULL, NULL, NULL}
};

/* Handler for parsing user defined arguments.  Currently we support
   configuring some of the CPU implementation specific registers including
   the Version Register (VR), the Unit Present Register (UPR) and the CPU
   Configuration Register (CPUCFGR).  */
SIM_RC
or1k_option_handler (SIM_DESC sd, sim_cpu *cpu, int opt, char *arg,
		     int is_command)
{
  switch (opt)
    {
    case OPTION_OR1K_VR:
      if (strcmp ("default", arg) == 0)
	or1k_vr = or1k_default_vr;
      else
	{
	  unsigned long long n;
	  char *endptr;

	  n = strtoull (arg, &endptr, 0);
	  if (*arg != '\0' && *endptr == '\0')
	    or1k_vr = n;
	  else
	    return SIM_RC_FAIL;
	}
      return SIM_RC_OK;

    case OPTION_OR1K_UPR:
      if (strcmp ("default", arg) == 0)
	or1k_upr = or1k_default_upr;
      else
	{
	  unsigned long long n;
	  char *endptr;

	  n = strtoull (arg, &endptr, 0);
	  if (*arg != '\0' && *endptr == '\0')
	    or1k_upr = n;
	  else
	    {
	      sim_io_eprintf
		(sd, "invalid argument to option --or1k-upr: `%s'\n", arg);
	      return SIM_RC_FAIL;
	    }
	}
      return SIM_RC_OK;

    case OPTION_OR1K_CPUCFGR:
      if (strcmp ("default", arg) == 0)
	or1k_cpucfgr = or1k_default_cpucfgr;
      else
	{
	  unsigned long long n;
	  char *endptr;

	  n = strtoull (arg, &endptr, 0);
	  if (*arg != '\0' && *endptr == '\0')
	    or1k_cpucfgr = n;
	  else
	    {
	      sim_io_eprintf
		(sd, "invalid argument to option --or1k-cpucfgr: `%s'\n", arg);
	      return SIM_RC_FAIL;
	    }
	}
      return SIM_RC_OK;

    default:
      sim_io_eprintf (sd, "Unknown or1k option %d\n", opt);
      return SIM_RC_FAIL;
    }

  return SIM_RC_FAIL;
}

extern const SIM_MACH * const or1k_sim_machs[];

/* Create an instance of the simulator.  */

SIM_DESC
sim_open (SIM_OPEN_KIND kind, host_callback *callback, struct bfd *abfd,
	  char * const *argv)
{
  SIM_DESC sd = sim_state_alloc (kind, callback);
  char c;
  int i;

  /* Set default options before parsing user options.  */
  STATE_MACHS (sd) = or1k_sim_machs;
  STATE_MODEL_NAME (sd) = "or1200";
  current_target_byte_order = BFD_ENDIAN_BIG;

  /* The cpu data is kept in a separately allocated chunk of memory.  */
  if (sim_cpu_alloc_all_extra (sd, 0, sizeof (struct or1k_sim_cpu))
      != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* Perform initial sim setups.  */
  if (sim_pre_argv_init (sd, argv[0]) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  or1k_upr = or1k_default_upr;
  or1k_vr = or1k_default_vr;
  or1k_cpucfgr = or1k_default_cpucfgr;
  sim_add_option_table (sd, NULL, or1k_options);

  /* Parse the user passed arguments.  */
  if (sim_parse_args (sd, argv) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* Allocate core managed memory if none specified by user.
     Use address 4 here in case the user wanted address 0 unmapped.  */
  if (sim_core_read_buffer (sd, NULL, read_map, &c, 4, 1) == 0)
    {
      sim_do_commandf (sd, "memory region 0,0x%x", OR1K_DEFAULT_MEM_SIZE);
    }

  /* Check for/establish the reference program image.  */
  if (sim_analyze_program (sd, STATE_PROG_FILE (sd), abfd) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* Establish any remaining configuration options.  */
  if (sim_config (sd) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  if (sim_post_argv_init (sd) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* Make sure delay slot mode is consistent with the loaded binary.  */
  if (STATE_ARCHITECTURE (sd)->mach == bfd_mach_or1knd)
    or1k_cpucfgr |= SPR_FIELD_MASK_SYS_CPUCFGR_ND;
  else
    or1k_cpucfgr &= ~SPR_FIELD_MASK_SYS_CPUCFGR_ND;

  /* Open a copy of the cpu descriptor table and initialize the
     disassembler.  These initialization functions are generated by CGEN
     using the binutils scheme cpu description files.  */
  {
    CGEN_CPU_DESC cd =
      or1k_cgen_cpu_open_1 (STATE_ARCHITECTURE (sd)->printable_name,
			    CGEN_ENDIAN_BIG);
    for (i = 0; i < MAX_NR_PROCESSORS; ++i)
      {
	SIM_CPU *cpu = STATE_CPU (sd, i);
	CPU_CPU_DESC (cpu) = cd;
	CPU_DISASSEMBLER (cpu) = sim_cgen_disassemble_insn;
      }
    or1k_cgen_init_dis (cd);
  }

  /* Do some final OpenRISC sim specific initializations.  */
  for (i = 0; i < MAX_NR_PROCESSORS; ++i)
    {
      SIM_CPU *cpu = STATE_CPU (sd, i);
      /* Only needed for profiling, but the structure member is small.  */
      memset (CPU_OR1K_MISC_PROFILE (cpu), 0,
	      sizeof (*CPU_OR1K_MISC_PROFILE (cpu)));

      or1k_cpu_init (sd, cpu, or1k_vr, or1k_upr, or1k_cpucfgr);
    }

  return sd;
}


SIM_RC
sim_create_inferior (SIM_DESC sd, struct bfd *abfd,
		     char * const *argv, char * const *envp)
{
  SIM_CPU *current_cpu = STATE_CPU (sd, 0);
  bfd_vma addr;

  if (abfd != NULL)
    addr = bfd_get_start_address (abfd);
  else
    addr = 0;
  sim_pc_set (current_cpu, addr);

  return SIM_RC_OK;
}
