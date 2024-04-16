/* Main simulator entry points specific to the M32R.
   Copyright (C) 1996-2024 Free Software Foundation, Inc.
   Contributed by Cygnus Support.

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

#include <string.h>
#include <stdlib.h>

#include "sim/callback.h"
#include "sim-main.h"
#include "sim-hw.h"
#include "sim-options.h"
#include "libiberty.h"
#include "bfd.h"

#include "m32r-sim.h"
#include "dv-m32r_uart.h"

#define M32R_DEFAULT_MEM_SIZE 0x2000000 /* 32M */

static void free_state (SIM_DESC);
static void print_m32r_misc_cpu (SIM_CPU *cpu, bool verbose);

/* Cover function of sim_state_free to free the cpu buffers as well.  */

static void
free_state (SIM_DESC sd)
{
  if (STATE_MODULES (sd) != NULL)
    sim_module_uninstall (sd);
  sim_cpu_free_all (sd);
  sim_state_free (sd);
}

extern const SIM_MACH * const m32r_sim_machs[];

/* Create an instance of the simulator.  */

SIM_DESC
sim_open (SIM_OPEN_KIND kind, host_callback *callback, struct bfd *abfd,
	  char * const *argv)
{
  SIM_DESC sd = sim_state_alloc (kind, callback);
  char c;
  int i;

  /* Set default options before parsing user options.  */
  STATE_MACHS (sd) = m32r_sim_machs;
  STATE_MODEL_NAME (sd) = "m32r/d";
  current_alignment = STRICT_ALIGNMENT;
  current_target_byte_order = BFD_ENDIAN_BIG;

  /* The cpu data is kept in a separately allocated chunk of memory.  */
  if (sim_cpu_alloc_all_extra (sd, 0, sizeof (struct m32r_sim_cpu))
      != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  if (sim_pre_argv_init (sd, argv[0]) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* The parser will print an error message for us, so we silently return.  */
  if (sim_parse_args (sd, argv) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* Allocate a handler for the control registers and other devices
     if no memory for that range has been allocated by the user.
     All are allocated in one chunk to keep things from being
     unnecessarily complicated.
     TODO: Move these to the sim-model framework.  */
  sim_hw_parse (sd, "/core/%s/reg %#x %i", "m32r_uart", UART_BASE_ADDR, 0x100);
  sim_hw_parse (sd, "/core/%s/reg %#x %i", "m32r_cache", 0xfffffff0, 0x10);

  /* Allocate core managed memory if none specified by user.
     Use address 4 here in case the user wanted address 0 unmapped.  */
  if (sim_core_read_buffer (sd, NULL, read_map, &c, 4, 1) == 0)
    sim_do_commandf (sd, "memory region 0,0x%x", M32R_DEFAULT_MEM_SIZE);

  /* check for/establish the reference program image */
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

  /* Open a copy of the cpu descriptor table.  */
  {
    CGEN_CPU_DESC cd = m32r_cgen_cpu_open_1 (STATE_ARCHITECTURE (sd)->printable_name,
					     CGEN_ENDIAN_BIG);
    for (i = 0; i < MAX_NR_PROCESSORS; ++i)
      {
	SIM_CPU *cpu = STATE_CPU (sd, i);
	CPU_CPU_DESC (cpu) = cd;
	CPU_DISASSEMBLER (cpu) = sim_cgen_disassemble_insn;
      }
    m32r_cgen_init_dis (cd);
  }

  for (i = 0; i < MAX_NR_PROCESSORS; ++i)
    {
      /* Only needed for profiling, but the structure member is small.  */
      memset (CPU_M32R_MISC_PROFILE (STATE_CPU (sd, i)), 0,
	      sizeof (* CPU_M32R_MISC_PROFILE (STATE_CPU (sd, i))));
      /* Hook in callback for reporting these stats */
      PROFILE_INFO_CPU_CALLBACK (CPU_PROFILE_DATA (STATE_CPU (sd, i)))
	= print_m32r_misc_cpu;
    }

  return sd;
}

SIM_RC
sim_create_inferior (SIM_DESC sd, struct bfd *abfd, char * const *argv,
		     char * const *env)
{
  SIM_CPU *current_cpu = STATE_CPU (sd, 0);
  host_callback *cb = STATE_CALLBACK (sd);
  bfd_vma addr;

  if (abfd != NULL)
    addr = bfd_get_start_address (abfd);
  else
    addr = 0;
  sim_pc_set (current_cpu, addr);

  if (STATE_ENVIRONMENT (sd) == USER_ENVIRONMENT)
    {
      m32rbf_h_cr_set (current_cpu,
		       m32r_decode_gdb_ctrl_regnum(SPI_REGNUM), 0x1f00000);
      m32rbf_h_cr_set (current_cpu,
		       m32r_decode_gdb_ctrl_regnum(SPU_REGNUM), 0x1f00000);
    }

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

  return SIM_RC_OK;
}

/* PROFILE_CPU_CALLBACK */

static void
print_m32r_misc_cpu (SIM_CPU *cpu, bool verbose)
{
  SIM_DESC sd = CPU_STATE (cpu);
  char buf[20];

  if (CPU_PROFILE_FLAGS (cpu) [PROFILE_INSN_IDX])
    {
      sim_io_printf (sd, "Miscellaneous Statistics\n\n");
      sim_io_printf (sd, "  %-*s %s\n\n",
		     PROFILE_LABEL_WIDTH, "Fill nops:",
		     sim_add_commas (buf, sizeof (buf),
				     CPU_M32R_MISC_PROFILE (cpu)->fillnop_count));
      if (STATE_ARCHITECTURE (sd)->mach == bfd_mach_m32rx)
	sim_io_printf (sd, "  %-*s %s\n\n",
		       PROFILE_LABEL_WIDTH, "Parallel insns:",
		       sim_add_commas (buf, sizeof (buf),
				       CPU_M32R_MISC_PROFILE (cpu)->parallel_count));
      if (STATE_ARCHITECTURE (sd)->mach == bfd_mach_m32r2)
	sim_io_printf (sd, "  %-*s %s\n\n",
		       PROFILE_LABEL_WIDTH, "Parallel insns:",
		       sim_add_commas (buf, sizeof (buf),
				       CPU_M32R_MISC_PROFILE (cpu)->parallel_count));
    }
}
