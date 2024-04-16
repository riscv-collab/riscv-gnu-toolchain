/* Main simulator entry points specific to the FRV.
   Copyright (C) 1998-2024 Free Software Foundation, Inc.
   Contributed by Red Hat.

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

#include <stdlib.h>

#include "sim/callback.h"

#define WANT_CPU
#define WANT_CPU_FRVBF
#include "sim-main.h"
#include "sim-options.h"
#include "libiberty.h"
#include "bfd.h"
#include "bfd/elf-bfd.h"

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

extern const SIM_MACH * const frv_sim_machs[];

/* Create an instance of the simulator.  */

SIM_DESC
sim_open (SIM_OPEN_KIND kind, host_callback *callback, bfd *abfd,
	  char * const *argv)
{
  char c;
  int i;
  unsigned long elf_flags = 0;
  SIM_DESC sd = sim_state_alloc (kind, callback);

  /* Set default options before parsing user options.  */
  STATE_MACHS (sd) = frv_sim_machs;
  STATE_MODEL_NAME (sd) = "fr500";
  current_alignment = STRICT_ALIGNMENT;
  current_target_byte_order = BFD_ENDIAN_BIG;

  /* The cpu data is kept in a separately allocated chunk of memory.  */
  if (sim_cpu_alloc_all_extra (sd, 0, sizeof (struct frv_sim_cpu)) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  if (sim_pre_argv_init (sd, argv[0]) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* These options override any module options.
     Obviously ambiguity should be avoided, however the caller may wish to
     augment the meaning of an option.  */
  sim_add_option_table (sd, NULL, frv_options);

  /* The parser will print an error message for us, so we silently return.  */
  if (sim_parse_args (sd, argv) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* Allocate core managed memory if none specified by user.
     Use address 4 here in case the user wanted address 0 unmapped.  */
  if (sim_core_read_buffer (sd, NULL, read_map, &c, 4, 1) == 0)
    sim_do_commandf (sd, "memory region 0,0x%x", FRV_DEFAULT_MEM_SIZE);

  /* check for/establish the reference program image */
  if (sim_analyze_program (sd, STATE_PROG_FILE (sd), abfd) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* set machine and architecture correctly instead of defaulting to frv */
  {
    bfd *prog_bfd = STATE_PROG_BFD (sd);
    if (prog_bfd != NULL)
      {
	const struct elf_backend_data *backend_data;

	if (bfd_get_arch (prog_bfd) != bfd_arch_frv)
	  {
	    sim_io_eprintf (sd, "%s: \"%s\" is not a FRV object file\n",
			    STATE_MY_NAME (sd),
			    bfd_get_filename (prog_bfd));
	    free_state (sd);
	    return 0;
	  }

	backend_data = get_elf_backend_data (prog_bfd);

	if (backend_data != NULL)
	  backend_data->elf_backend_object_p (prog_bfd);

	elf_flags = elf_elfheader (prog_bfd)->e_flags;
      }
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
    CGEN_CPU_DESC cd = frv_cgen_cpu_open_1 (STATE_ARCHITECTURE (sd)->printable_name,
					     CGEN_ENDIAN_BIG);
    for (i = 0; i < MAX_NR_PROCESSORS; ++i)
      {
	SIM_CPU *cpu = STATE_CPU (sd, i);
	CPU_CPU_DESC (cpu) = cd;
	CPU_DISASSEMBLER (cpu) = sim_cgen_disassemble_insn;
	CPU_ELF_FLAGS (cpu) = elf_flags;
      }
    frv_cgen_init_dis (cd);
  }

  /* CPU specific initialization.  */
  for (i = 0; i < MAX_NR_PROCESSORS; ++i)
    {
      SIM_CPU* cpu = STATE_CPU (sd, i);
      frv_initialize (cpu, sd);
    }

  return sd;
}

void
frv_sim_close (SIM_DESC sd, int quitting)
{
  int i;
  /* Terminate cache support.  */
  for (i = 0; i < MAX_NR_PROCESSORS; ++i)
    {
      SIM_CPU* cpu = STATE_CPU (sd, i);
      frv_cache_term (CPU_INSN_CACHE (cpu));
      frv_cache_term (CPU_DATA_CACHE (cpu));
    }
}

SIM_RC
sim_create_inferior (SIM_DESC sd, bfd *abfd, char * const *argv,
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
