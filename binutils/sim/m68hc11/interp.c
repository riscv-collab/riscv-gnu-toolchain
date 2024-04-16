/* interp.c -- Simulator for Motorola 68HC11/68HC12
   Copyright (C) 1999-2024 Free Software Foundation, Inc.
   Written by Stephane Carrez (stcarrez@nerim.fr)

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

#include "bfd.h"

#include "sim-main.h"
#include "sim-assert.h"
#include "sim-hw.h"
#include "sim-options.h"
#include "hw-tree.h"
#include "hw-device.h"
#include "hw-ports.h"
#include "bfd/elf32-m68hc1x.h"

#include "m68hc11-sim.h"

#ifndef MONITOR_BASE
# define MONITOR_BASE (0x0C000)
# define MONITOR_SIZE (0x04000)
#endif

static void sim_get_info (SIM_DESC sd, char *cmd);

struct sim_info_list
{
  const char *name;
  const char *device;
};

struct sim_info_list dev_list_68hc11[] = {
  {"cpu", "/m68hc11"},
  {"timer", "/m68hc11/m68hc11tim"},
  {"sio", "/m68hc11/m68hc11sio"},
  {"spi", "/m68hc11/m68hc11spi"},
  {"eeprom", "/m68hc11/m68hc11eepr"},
  {0, 0}
};

struct sim_info_list dev_list_68hc12[] = {
  {"cpu", "/m68hc12"},
  {"timer", "/m68hc12/m68hc12tim"},
  {"sio", "/m68hc12/m68hc12sio"},
  {"spi", "/m68hc12/m68hc12spi"},
  {"eeprom", "/m68hc12/m68hc12eepr"},
  {0, 0}
};

/* Cover function of sim_state_free to free the cpu buffers as well.  */

static void
free_state (SIM_DESC sd)
{
  if (STATE_MODULES (sd) != NULL)
    sim_module_uninstall (sd);

  sim_state_free (sd);
}

/* Give some information about the simulator.  */
static void
sim_get_info (SIM_DESC sd, char *cmd)
{
  sim_cpu *cpu;

  cpu = STATE_CPU (sd, 0);
  if (cmd != 0 && (cmd[0] == ' ' || cmd[0] == '-'))
    {
      int i;
      struct hw *hw_dev;
      struct sim_info_list *dev_list;
      const struct bfd_arch_info *arch;

      arch = STATE_ARCHITECTURE (sd);
      cmd++;

      if (arch->arch == bfd_arch_m68hc11)
        dev_list = dev_list_68hc11;
      else
        dev_list = dev_list_68hc12;

      for (i = 0; dev_list[i].name; i++)
	if (strcmp (cmd, dev_list[i].name) == 0)
	  break;

      if (dev_list[i].name == 0)
	{
	  sim_io_eprintf (sd, "Device '%s' not found.\n", cmd);
	  sim_io_eprintf (sd, "Valid devices: cpu timer sio eeprom\n");
	  return;
	}
      hw_dev = sim_hw_parse (sd, "%s", dev_list[i].device);
      if (hw_dev == 0)
	{
	  sim_io_eprintf (sd, "Device '%s' not found\n", dev_list[i].device);
	  return;
	}
      hw_ioctl (hw_dev, 23, 0);
      return;
    }

  cpu_info (sd, cpu);
  interrupts_info (sd, &M68HC11_SIM_CPU (cpu)->cpu_interrupts);
}


void
sim_board_reset (SIM_DESC sd)
{
  struct hw *hw_cpu;
  sim_cpu *cpu;
  struct m68hc11_sim_cpu *m68hc11_cpu;
  const struct bfd_arch_info *arch;
  const char *cpu_type;

  cpu = STATE_CPU (sd, 0);
  m68hc11_cpu = M68HC11_SIM_CPU (cpu);
  arch = STATE_ARCHITECTURE (sd);

  /*  hw_cpu = sim_hw_parse (sd, "/"); */
  if (arch->arch == bfd_arch_m68hc11)
    {
      m68hc11_cpu->cpu_type = CPU_M6811;
      cpu_type = "/m68hc11";
    }
  else
    {
      m68hc11_cpu->cpu_type = CPU_M6812;
      cpu_type = "/m68hc12";
    }
  
  hw_cpu = sim_hw_parse (sd, "%s", cpu_type);
  if (hw_cpu == 0)
    {
      sim_io_eprintf (sd, "%s cpu not found in device tree.", cpu_type);
      return;
    }

  cpu_reset (cpu);
  hw_port_event (hw_cpu, 3, 0);
  cpu_restart (cpu);
}

static int
sim_hw_configure (SIM_DESC sd)
{
  const struct bfd_arch_info *arch;
  struct hw *device_tree;
  sim_cpu *cpu;
  struct m68hc11_sim_cpu *m68hc11_cpu;
  
  arch = STATE_ARCHITECTURE (sd);
  if (arch == 0)
    return 0;

  cpu = STATE_CPU (sd, 0);
  m68hc11_cpu = M68HC11_SIM_CPU (cpu);
  m68hc11_cpu->cpu_configured_arch = arch;
  device_tree = sim_hw_parse (sd, "/");
  if (arch->arch == bfd_arch_m68hc11)
    {
      m68hc11_cpu->cpu_interpretor = cpu_interp_m6811;
      if (hw_tree_find_property (device_tree, "/m68hc11/reg") == 0)
	{
	  /* Allocate core managed memory */

	  /* the monitor  */
	  sim_do_commandf (sd, "memory region 0x%x@%d,0x%x",
			   /* MONITOR_BASE, MONITOR_SIZE */
			   0x8000, M6811_RAM_LEVEL, 0x8000);
	  sim_do_commandf (sd, "memory region 0x000@%d,0x8000",
			   M6811_RAM_LEVEL);
	  sim_hw_parse (sd, "/m68hc11/reg 0x1000 0x03F");
          if (m68hc11_cpu->bank_start < m68hc11_cpu->bank_end)
            {
              sim_do_commandf (sd, "memory region 0x%x@%d,0x100000",
                               m68hc11_cpu->bank_virtual, M6811_RAM_LEVEL);
              sim_hw_parse (sd, "/m68hc11/use_bank 1");
            }
	}
      if (m68hc11_cpu->cpu_start_mode)
        {
          sim_hw_parse (sd, "/m68hc11/mode %s", m68hc11_cpu->cpu_start_mode);
        }
      if (hw_tree_find_property (device_tree, "/m68hc11/m68hc11sio/reg") == 0)
	{
	  sim_hw_parse (sd, "/m68hc11/m68hc11sio/reg 0x2b 0x5");
	  sim_hw_parse (sd, "/m68hc11/m68hc11sio/backend stdio");
	  sim_hw_parse (sd, "/m68hc11 > cpu-reset reset /m68hc11/m68hc11sio");
	}
      if (hw_tree_find_property (device_tree, "/m68hc11/m68hc11tim/reg") == 0)
	{
	  /* M68hc11 Timer configuration. */
	  sim_hw_parse (sd, "/m68hc11/m68hc11tim/reg 0x1b 0x5");
	  sim_hw_parse (sd, "/m68hc11 > cpu-reset reset /m68hc11/m68hc11tim");
          sim_hw_parse (sd, "/m68hc11 > capture capture /m68hc11/m68hc11tim");
	}

      /* Create the SPI device.  */
      if (hw_tree_find_property (device_tree, "/m68hc11/m68hc11spi/reg") == 0)
	{
	  sim_hw_parse (sd, "/m68hc11/m68hc11spi/reg 0x28 0x3");
	  sim_hw_parse (sd, "/m68hc11 > cpu-reset reset /m68hc11/m68hc11spi");
	}
      if (hw_tree_find_property (device_tree, "/m68hc11/nvram/reg") == 0)
	{
	  /* M68hc11 persistent ram configuration. */
	  sim_hw_parse (sd, "/m68hc11/nvram/reg 0x0 256");
	  sim_hw_parse (sd, "/m68hc11/nvram/file m68hc11.ram");
	  sim_hw_parse (sd, "/m68hc11/nvram/mode save-modified");
	  /*sim_hw_parse (sd, "/m68hc11 > cpu-reset reset /m68hc11/pram"); */
	}
      if (hw_tree_find_property (device_tree, "/m68hc11/m68hc11eepr/reg") == 0)
	{
	  sim_hw_parse (sd, "/m68hc11/m68hc11eepr/reg 0xb000 512");
	  sim_hw_parse (sd, "/m68hc11 > cpu-reset reset /m68hc11/m68hc11eepr");
	}
      sim_hw_parse (sd, "/m68hc11 > port-a cpu-write-port /m68hc11");
      sim_hw_parse (sd, "/m68hc11 > port-b cpu-write-port /m68hc11");
      sim_hw_parse (sd, "/m68hc11 > port-c cpu-write-port /m68hc11");
      sim_hw_parse (sd, "/m68hc11 > port-d cpu-write-port /m68hc11");
      m68hc11_cpu->hw_cpu = sim_hw_parse (sd, "/m68hc11");
    }
  else
    {
      m68hc11_cpu->cpu_interpretor = cpu_interp_m6812;
      if (hw_tree_find_property (device_tree, "/m68hc12/reg") == 0)
	{
	  /* Allocate core external memory.  */
	  sim_do_commandf (sd, "memory region 0x%x@%d,0x%x",
			   0x8000, M6811_RAM_LEVEL, 0x8000);
	  sim_do_commandf (sd, "memory region 0x000@%d,0x8000",
			   M6811_RAM_LEVEL);
          if (m68hc11_cpu->bank_start < m68hc11_cpu->bank_end)
            {
              sim_do_commandf (sd, "memory region 0x%x@%d,0x100000",
                               m68hc11_cpu->bank_virtual, M6811_RAM_LEVEL);
              sim_hw_parse (sd, "/m68hc12/use_bank 1");
            }
	  sim_hw_parse (sd, "/m68hc12/reg 0x0 0x3FF");
	}

      if (!hw_tree_find_property (device_tree, "/m68hc12/m68hc12sio@1/reg"))
	{
	  sim_hw_parse (sd, "/m68hc12/m68hc12sio@1/reg 0xC0 0x8");
	  sim_hw_parse (sd, "/m68hc12/m68hc12sio@1/backend stdio");
	  sim_hw_parse (sd, "/m68hc12 > cpu-reset reset /m68hc12/m68hc12sio@1");
	}
      if (hw_tree_find_property (device_tree, "/m68hc12/m68hc12tim/reg") == 0)
	{
	  /* M68hc11 Timer configuration. */
	  sim_hw_parse (sd, "/m68hc12/m68hc12tim/reg 0x1b 0x5");
	  sim_hw_parse (sd, "/m68hc12 > cpu-reset reset /m68hc12/m68hc12tim");
          sim_hw_parse (sd, "/m68hc12 > capture capture /m68hc12/m68hc12tim");
	}

      /* Create the SPI device.  */
      if (hw_tree_find_property (device_tree, "/m68hc12/m68hc12spi/reg") == 0)
	{
	  sim_hw_parse (sd, "/m68hc12/m68hc12spi/reg 0x28 0x3");
	  sim_hw_parse (sd, "/m68hc12 > cpu-reset reset /m68hc12/m68hc12spi");
	}
      if (hw_tree_find_property (device_tree, "/m68hc12/nvram/reg") == 0)
	{
	  /* M68hc11 persistent ram configuration. */
	  sim_hw_parse (sd, "/m68hc12/nvram/reg 0x2000 8192");
	  sim_hw_parse (sd, "/m68hc12/nvram/file m68hc12.ram");
	  sim_hw_parse (sd, "/m68hc12/nvram/mode save-modified");
	}
      if (hw_tree_find_property (device_tree, "/m68hc12/m68hc12eepr/reg") == 0)
	{
	  sim_hw_parse (sd, "/m68hc12/m68hc12eepr/reg 0x0800 2048");
	  sim_hw_parse (sd, "/m68hc12 > cpu-reset reset /m68hc12/m68hc12eepr");
	}

      sim_hw_parse (sd, "/m68hc12 > port-a cpu-write-port /m68hc12");
      sim_hw_parse (sd, "/m68hc12 > port-b cpu-write-port /m68hc12");
      sim_hw_parse (sd, "/m68hc12 > port-c cpu-write-port /m68hc12");
      sim_hw_parse (sd, "/m68hc12 > port-d cpu-write-port /m68hc12");
      m68hc11_cpu->hw_cpu = sim_hw_parse (sd, "/m68hc12");
    }
  return 1;
}

/* Get the memory bank parameters by looking at the global symbols
   defined by the linker.  */
static int
sim_get_bank_parameters (SIM_DESC sd)
{
  sim_cpu *cpu;
  struct m68hc11_sim_cpu *m68hc11_cpu;
  unsigned size;
  bfd_vma addr;

  cpu = STATE_CPU (sd, 0);
  m68hc11_cpu = M68HC11_SIM_CPU (cpu);

  addr = trace_sym_value (sd, BFD_M68HC11_BANK_START_NAME);
  if (addr != -1)
    m68hc11_cpu->bank_start = addr;

  size = trace_sym_value (sd, BFD_M68HC11_BANK_SIZE_NAME);
  if (size == -1)
    size = 0;

  addr = trace_sym_value (sd, BFD_M68HC11_BANK_VIRTUAL_NAME);
  if (addr != -1)
    m68hc11_cpu->bank_virtual = addr;

  m68hc11_cpu->bank_end = m68hc11_cpu->bank_start + size;
  m68hc11_cpu->bank_shift = 0;
  for (; size > 1; size >>= 1)
    m68hc11_cpu->bank_shift++;

  return 0;
}

static int
sim_prepare_for_program (SIM_DESC sd, bfd* abfd)
{
  sim_cpu *cpu;
  struct m68hc11_sim_cpu *m68hc11_cpu;
  int elf_flags = 0;

  cpu = STATE_CPU (sd, 0);
  m68hc11_cpu = M68HC11_SIM_CPU (cpu);

  if (abfd != NULL)
    {
      asection *s;

      if (bfd_get_flavour (abfd) == bfd_target_elf_flavour)
        elf_flags = elf_elfheader (abfd)->e_flags;

      m68hc11_cpu->cpu_elf_start = bfd_get_start_address (abfd);
      /* See if any section sets the reset address */
      m68hc11_cpu->cpu_use_elf_start = 1;
      for (s = abfd->sections; s && m68hc11_cpu->cpu_use_elf_start; s = s->next)
        {
          if (s->flags & SEC_LOAD)
            {
              bfd_size_type size;

	      size = bfd_section_size (s);
              if (size > 0)
                {
                  bfd_vma lma;

                  if (STATE_LOAD_AT_LMA_P (sd))
		    lma = bfd_section_lma (s);
                  else
		    lma = bfd_section_vma (s);

                  if (lma <= 0xFFFE && lma+size >= 0x10000)
                    m68hc11_cpu->cpu_use_elf_start = 0;
                }
            }
        }

      if (elf_flags & E_M68HC12_BANKS)
        {
          if (sim_get_bank_parameters (sd) != 0)
            sim_io_eprintf (sd, "Memory bank parameters are not initialized\n");
        }
    }

  if (!sim_hw_configure (sd))
    return SIM_RC_FAIL;

  /* reset all state information */
  sim_board_reset (sd);

  return SIM_RC_OK;
}

static sim_cia
m68hc11_pc_get (sim_cpu *cpu)
{
  return cpu_get_pc (cpu);
}

static void
m68hc11_pc_set (sim_cpu *cpu, sim_cia pc)
{
  cpu_set_pc (cpu, pc);
}

static int m68hc11_reg_fetch (SIM_CPU *, int, void *, int);
static int m68hc11_reg_store (SIM_CPU *, int, const void *, int);

SIM_DESC
sim_open (SIM_OPEN_KIND kind, host_callback *callback,
	  bfd *abfd, char * const *argv)
{
  int i;
  SIM_DESC sd;
  sim_cpu *cpu;

  sd = sim_state_alloc (kind, callback);

  SIM_ASSERT (STATE_MAGIC (sd) == SIM_MAGIC_NUMBER);

  /* Set default options before parsing user options.  */
  current_target_byte_order = BFD_ENDIAN_BIG;

  /* The cpu data is kept in a separately allocated chunk of memory.  */
  if (sim_cpu_alloc_all_extra (sd, 0, sizeof (struct m68hc11_sim_cpu))
      != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  cpu = STATE_CPU (sd, 0);

  cpu_initialize (sd, cpu);

  if (sim_pre_argv_init (sd, argv[0]) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* The parser will print an error message for us, so we silently return.  */
  if (sim_parse_args (sd, argv) != SIM_RC_OK)
    {
      /* Uninstall the modules to avoid memory leaks,
         file descriptor leaks, etc.  */
      free_state (sd);
      return 0;
    }

  /* Check for/establish the a reference program image.  */
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
      /* Uninstall the modules to avoid memory leaks,
         file descriptor leaks, etc.  */
      free_state (sd);
      return 0;
    }
  if (sim_prepare_for_program (sd, abfd) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }      

  /* CPU specific initialization.  */
  for (i = 0; i < MAX_NR_PROCESSORS; ++i)
    {
      cpu = STATE_CPU (sd, i);

      CPU_REG_FETCH (cpu) = m68hc11_reg_fetch;
      CPU_REG_STORE (cpu) = m68hc11_reg_store;
      CPU_PC_FETCH (cpu) = m68hc11_pc_get;
      CPU_PC_STORE (cpu) = m68hc11_pc_set;
    }

  return sd;
}

/* Generic implementation of sim_engine_run that works within the
   sim_engine setjmp/longjmp framework. */

void
sim_engine_run (SIM_DESC sd,
                int next_cpu_nr,	/* ignore */
		int nr_cpus,	/* ignore */
		int siggnal)	/* ignore */
{
  sim_cpu *cpu;

  SIM_ASSERT (STATE_MAGIC (sd) == SIM_MAGIC_NUMBER);
  cpu = STATE_CPU (sd, 0);
  while (1)
    {
      cpu_single_step (cpu);

      /* process any events */
      if (sim_events_tickn (sd, M68HC11_SIM_CPU (cpu)->cpu_current_cycle))
	{
	  sim_events_process (sd);
	}
    }
}

void
sim_info (SIM_DESC sd, bool verbose)
{
  const char *cpu_type;
  const struct bfd_arch_info *arch;

  /* Nothing to do if there is no verbose flag set.  */
  if (verbose == 0 && STATE_VERBOSE_P (sd) == 0)
    return;

  arch = STATE_ARCHITECTURE (sd);
  if (arch->arch == bfd_arch_m68hc11)
    cpu_type = "68HC11";
  else
    cpu_type = "68HC12";

  sim_io_eprintf (sd, "Simulator info:\n");
  sim_io_eprintf (sd, "  CPU Motorola %s\n", cpu_type);
  sim_get_info (sd, 0);
  sim_module_info (sd, verbose || STATE_VERBOSE_P (sd));
}

SIM_RC
sim_create_inferior (SIM_DESC sd, struct bfd *abfd,
                     char * const *argv, char * const *env)
{
  return sim_prepare_for_program (sd, abfd);
}

static int
m68hc11_reg_fetch (SIM_CPU *cpu, int rn, void *buf, int length)
{
  unsigned char *memory = buf;
  uint16_t val;
  int size = 2;

  switch (rn)
    {
    case A_REGNUM:
      val = cpu_get_a (cpu);
      size = 1;
      break;

    case B_REGNUM:
      val = cpu_get_b (cpu);
      size = 1;
      break;

    case D_REGNUM:
      val = cpu_get_d (cpu);
      break;

    case X_REGNUM:
      val = cpu_get_x (cpu);
      break;

    case Y_REGNUM:
      val = cpu_get_y (cpu);
      break;

    case SP_REGNUM:
      val = cpu_get_sp (cpu);
      break;

    case PC_REGNUM:
      val = cpu_get_pc (cpu);
      break;

    case PSW_REGNUM:
      val = cpu_get_ccr (cpu);
      size = 1;
      break;

    case PAGE_REGNUM:
      val = cpu_get_page (cpu);
      size = 1;
      break;

    default:
      val = 0;
      break;
    }
  if (size == 1)
    {
      memory[0] = val;
    }
  else
    {
      memory[0] = val >> 8;
      memory[1] = val & 0x0FF;
    }
  return size;
}

static int
m68hc11_reg_store (SIM_CPU *cpu, int rn, const void *buf, int length)
{
  const unsigned char *memory = buf;
  uint16_t val;

  val = *memory++;
  if (length == 2)
    val = (val << 8) | *memory;

  switch (rn)
    {
    case D_REGNUM:
      cpu_set_d (cpu, val);
      break;

    case A_REGNUM:
      cpu_set_a (cpu, val);
      return 1;

    case B_REGNUM:
      cpu_set_b (cpu, val);
      return 1;

    case X_REGNUM:
      cpu_set_x (cpu, val);
      break;

    case Y_REGNUM:
      cpu_set_y (cpu, val);
      break;

    case SP_REGNUM:
      cpu_set_sp (cpu, val);
      break;

    case PC_REGNUM:
      cpu_set_pc (cpu, val);
      break;

    case PSW_REGNUM:
      cpu_set_ccr (cpu, val);
      return 1;

    case PAGE_REGNUM:
      cpu_set_page (cpu, val);
      return 1;

    default:
      break;
    }

  return 2;
}
