/* Main simulator entry points specific to Lattice Mico32.
   Contributed by Jon Beniston <jon@beniston.com>
   
   Copyright (C) 2009-2024 Free Software Foundation, Inc.

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

#include <stdlib.h>

#include "sim/callback.h"
#include "sim-main.h"
#include "sim-options.h"
#include "libiberty.h"
#include "bfd.h"

/* Cover function of sim_state_free to free the cpu buffers as well.  */

static void
free_state (SIM_DESC sd)
{
  if (STATE_MODULES (sd) != NULL)
    sim_module_uninstall (sd);
  sim_cpu_free_all (sd);
  sim_state_free (sd);
}

/* Find memory range used by program.  */

static unsigned long
find_base (bfd *prog_bfd)
{
  int found;
  unsigned long base = ~(0UL);
  asection *s;

  found = 0;
  for (s = prog_bfd->sections; s; s = s->next)
    {
      if ((strcmp (bfd_section_name (s), ".boot") == 0)
	  || (strcmp (bfd_section_name (s), ".text") == 0)
	  || (strcmp (bfd_section_name (s), ".data") == 0)
	  || (strcmp (bfd_section_name (s), ".bss") == 0))
	{
	  if (!found)
	    {
	      base = bfd_section_vma (s);
	      found = 1;
	    }
	  else
	    base = bfd_section_vma (s) < base ? bfd_section_vma (s) : base;
	}
    }
  return base & ~(0xffffUL);
}

static unsigned long
find_limit (SIM_DESC sd)
{
  bfd_vma addr;

  addr = trace_sym_value (sd, "_fstack");
  if (addr == -1)
    return 0;

  return (addr + 65536) & ~(0xffffUL);
}

extern const SIM_MACH * const lm32_sim_machs[];

/* Create an instance of the simulator.  */

SIM_DESC
sim_open (SIM_OPEN_KIND kind, host_callback *callback, struct bfd *abfd,
	  char * const *argv)
{
  SIM_DESC sd = sim_state_alloc (kind, callback);
  char c;
  int i;
  unsigned long base, limit;

  /* Set default options before parsing user options.  */
  STATE_MACHS (sd) = lm32_sim_machs;
  STATE_MODEL_NAME (sd) = "lm32";
  current_alignment = STRICT_ALIGNMENT;
  current_target_byte_order = BFD_ENDIAN_BIG;

  /* The cpu data is kept in a separately allocated chunk of memory.  */
  if (sim_cpu_alloc_all_extra (sd, 0, sizeof (struct lm32_sim_cpu))
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

#if 0
  /* Allocate a handler for I/O devices
     if no memory for that range has been allocated by the user.
     All are allocated in one chunk to keep things from being
     unnecessarily complicated.  */
  if (sim_core_read_buffer (sd, NULL, read_map, &c, LM32_DEVICE_ADDR, 1) == 0)
    sim_core_attach (sd, NULL, 0 /*level */ ,
		     access_read_write, 0 /*space ??? */ ,
		     LM32_DEVICE_ADDR, LM32_DEVICE_LEN /*nr_bytes */ ,
		     0 /*modulo */ ,
		     &lm32_devices, NULL /*buffer */ );
#endif

  /* check for/establish the reference program image.  */
  if (sim_analyze_program (sd, STATE_PROG_FILE (sd), abfd) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* Check to see if memory exists at programs start address.  */
  if (sim_core_read_buffer (sd, NULL, read_map, &c, STATE_START_ADDR (sd), 1)
      == 0)
    {
      if (STATE_PROG_BFD (sd) != NULL)
	{
	  /* It doesn't, so we should try to allocate enough memory to hold program.  */
	  base = find_base (STATE_PROG_BFD (sd));
	  limit = find_limit (sd);
	  if (limit == 0)
	    {
	      sim_io_eprintf (sd,
			      "Failed to find symbol _fstack in program. You must specify memory regions with --memory-region.\n");
	      free_state (sd);
	      return 0;
	    }
	  /*sim_io_printf (sd, "Allocating memory at 0x%lx size 0x%lx\n", base, limit); */
	  sim_do_commandf (sd, "memory region 0x%lx,0x%lx", base, limit);
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
    CGEN_CPU_DESC cd =
      lm32_cgen_cpu_open_1 (STATE_ARCHITECTURE (sd)->printable_name,
			    CGEN_ENDIAN_BIG);
    for (i = 0; i < MAX_NR_PROCESSORS; ++i)
      {
	SIM_CPU *cpu = STATE_CPU (sd, i);
	CPU_CPU_DESC (cpu) = cd;
	CPU_DISASSEMBLER (cpu) = sim_cgen_disassemble_insn;
      }
    lm32_cgen_init_dis (cd);
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
