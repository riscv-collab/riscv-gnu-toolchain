/* Main simulator entry points specific to the IQ2000.
   Copyright (C) 2000-2024 Free Software Foundation, Inc.
   Contributed by Cygnus Solutions.

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

#include "sim-main.h"

#include <stdlib.h>

#include "sim/callback.h"
#include "sim-options.h"
#include "libiberty.h"
#include "bfd.h"

static void free_state (SIM_DESC);

/* Cover function for sim_cgen_disassemble_insn.  */

static void
iq2000bf_disassemble_insn (SIM_CPU *cpu, const CGEN_INSN *insn,
			  const ARGBUF *abuf, IADDR pc, char *buf)
{
  sim_cgen_disassemble_insn(cpu, insn, abuf, pc, buf);
}

/* Cover function of sim_state_free to free the cpu buffers as well.  */

static void
free_state (SIM_DESC sd)
{
  if (STATE_MODULES (sd) != NULL)
    sim_module_uninstall (sd);
  sim_cpu_free_all (sd);
  sim_state_free (sd);
}

extern const SIM_MACH * const iq2000_sim_machs[];

/* Create an instance of the simulator.  */

SIM_DESC
sim_open (SIM_OPEN_KIND kind, host_callback *callback, struct bfd *abfd,
	  char * const *argv)
{
  int i;
  SIM_DESC sd = sim_state_alloc (kind, callback);

  /* Set default options before parsing user options.  */
  STATE_MACHS (sd) = iq2000_sim_machs;
  STATE_MODEL_NAME (sd) = "iq2000";
  current_alignment = STRICT_ALIGNMENT;
  current_target_byte_order = BFD_ENDIAN_BIG;

  /* The cpu data is kept in a separately allocated chunk of memory.  */
  if (sim_cpu_alloc_all_extra (sd, 0, sizeof (struct iq2000_sim_cpu))
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

  /* Allocate core managed memory.  */
  sim_do_commandf (sd, "memory region 0x%x,0x%x", IQ2000_INSN_VALUE, IQ2000_INSN_MEM_SIZE); 
  sim_do_commandf (sd, "memory region 0x%x,0x%x", IQ2000_DATA_VALUE, IQ2000_DATA_MEM_SIZE); 

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
    CGEN_CPU_DESC cd = iq2000_cgen_cpu_open_1 (STATE_ARCHITECTURE (sd)->printable_name,
					      CGEN_ENDIAN_BIG);

    for (i = 0; i < MAX_NR_PROCESSORS; ++i)
      {
	SIM_CPU *cpu = STATE_CPU (sd, i);
	CPU_CPU_DESC (cpu) = cd;
	CPU_DISASSEMBLER (cpu) = iq2000bf_disassemble_insn;
      }
    iq2000_cgen_init_dis (cd);
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
    addr = CPU2INSN(0);
  sim_pc_set (current_cpu, addr);

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
