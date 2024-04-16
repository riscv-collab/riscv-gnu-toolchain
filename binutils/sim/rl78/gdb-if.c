/* gdb-if.c -- sim interface to GDB.

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
#include "libiberty.h"
#include "sim/callback.h"
#include "sim/sim.h"
#include "gdb/signals.h"
#include "sim/sim-rl78.h"

#include "cpu.h"
#include "mem.h"
#include "load.h"
#include "trace.h"

/* Ideally, we'd wrap up all the minisim's data structures in an
   object and pass that around.  However, neither GDB nor run needs
   that ability.

   So we just have one instance, that lives in global variables, and
   each time we open it, we re-initialize it.  */

struct sim_state
{
  const char *message;
};

static struct sim_state the_minisim = {
  "This is the sole rl78 minisim instance."
};

static int is_open;

static struct host_callback_struct *host_callbacks;

/* Open an instance of the sim.  For this sim, only one instance
   is permitted.  If sim_open() is called multiple times, the sim
   will be reset.  */

SIM_DESC
sim_open (SIM_OPEN_KIND kind,
	  struct host_callback_struct *callback,
	  struct bfd *abfd, char * const *argv)
{
  if (is_open)
    fprintf (stderr, "rl78 minisim: re-opened sim\n");

  /* The 'run' interface doesn't use this function, so we don't care
     about KIND; it's always SIM_OPEN_DEBUG.  */
  if (kind != SIM_OPEN_DEBUG)
    fprintf (stderr, "rl78 minisim: sim_open KIND != SIM_OPEN_DEBUG: %d\n",
	     kind);

  /* We use this for the load command.  Perhaps someday, it'll be used
     for syscalls too.  */
  host_callbacks = callback;

  /* We don't expect any command-line arguments.  */

  init_cpu ();
  trace = 0;

  sim_disasm_init (abfd);
  is_open = 1;

  while (argv != NULL && *argv != NULL)
    {
      if (strcmp (*argv, "g10") == 0 || strcmp (*argv, "-Mg10") == 0)
	{
	  fprintf (stderr, "rl78 g10 support enabled.\n");
	  rl78_g10_mode = 1;
	  g13_multiply = 0;
	  g14_multiply = 0;
	  mem_set_mirror (0, 0xf8000, 4096);
	  break;
	}
      if (strcmp (*argv, "g13") == 0 || strcmp (*argv, "-Mg13") == 0)
	{
	  fprintf (stderr, "rl78 g13 support enabled.\n");
	  rl78_g10_mode = 0;
	  g13_multiply = 1;
	  g14_multiply = 0;
	  break;
	}
      if (strcmp (*argv, "g14") == 0 || strcmp (*argv, "-Mg14") == 0)
	{
	  fprintf (stderr, "rl78 g14 support enabled.\n");
	  rl78_g10_mode = 0;
	  g13_multiply = 0;
	  g14_multiply = 1;
	  break;
	}
      argv++;
    }

  return &the_minisim;
}

/* Verify the sim descriptor.  Just print a message if the descriptor
   doesn't match.  Nothing bad will happen if the descriptor doesn't
   match because all of the state is global.  But if it doesn't
   match, that means there's a problem with the caller.  */

static void
check_desc (SIM_DESC sd)
{
  if (sd != &the_minisim)
    fprintf (stderr, "rl78 minisim: desc != &the_minisim\n");
}

/* Close the sim.  */

void
sim_close (SIM_DESC sd, int quitting)
{
  check_desc (sd);

  /* Not much to do.  At least free up our memory.  */
  init_mem ();

  is_open = 0;
}

/* Open the program to run; print a message if the program cannot
   be opened.  */

static bfd *
open_objfile (const char *filename)
{
  bfd *prog = bfd_openr (filename, 0);

  if (!prog)
    {
      fprintf (stderr, "Can't read %s\n", filename);
      return 0;
    }

  if (!bfd_check_format (prog, bfd_object))
    {
      fprintf (stderr, "%s not a rl78 program\n", filename);
      return 0;
    }

  return prog;
}

/* Load a program.  */

SIM_RC
sim_load (SIM_DESC sd, const char *prog, struct bfd *abfd, int from_tty)
{
  check_desc (sd);

  if (!abfd)
    abfd = open_objfile (prog);
  if (!abfd)
    return SIM_RC_FAIL;

  rl78_load (abfd, host_callbacks, "sim");

  return SIM_RC_OK;
}

/* Create inferior.  */

SIM_RC
sim_create_inferior (SIM_DESC sd, struct bfd *abfd,
		     char * const *argv, char * const *env)
{
  check_desc (sd);

  if (abfd)
    rl78_load (abfd, 0, "sim");

  return SIM_RC_OK;
}

/* Read memory.  */

uint64_t
sim_read (SIM_DESC sd, uint64_t mem, void *buf, uint64_t length)
{
  check_desc (sd);

  if (mem >= MEM_SIZE)
    return 0;
  else if (mem + length > MEM_SIZE)
    length = MEM_SIZE - mem;

  mem_get_blk (mem, buf, length);
  return length;
}

/* Write memory.  */

uint64_t
sim_write (SIM_DESC sd, uint64_t mem, const void *buf, uint64_t length)
{
  check_desc (sd);

  if (mem >= MEM_SIZE)
    return 0;
  else if (mem + length > MEM_SIZE)
    length = MEM_SIZE - mem;

  mem_put_blk (mem, buf, length);
  return length;
}

/* Read the LENGTH bytes at BUF as an little-endian value.  */

static SI
get_le (const unsigned char *buf, int length)
{
  SI acc = 0;

  while (--length >= 0)
    acc = (acc << 8) + buf[length];

  return acc;
}

/* Store VAL as a little-endian value in the LENGTH bytes at BUF.  */

static void
put_le (unsigned char *buf, int length, SI val)
{
  int i;

  for (i = 0; i < length; i++)
    {
      buf[i] = val & 0xff;
      val >>= 8;
    }
}

/* Verify that REGNO is in the proper range.  Return 0 if not and
   something non-zero if so.  */

static int
check_regno (enum sim_rl78_regnum regno)
{
  return 0 <= regno && regno < sim_rl78_num_regs;
}

/* Return the size of the register REGNO.  */

static size_t
reg_size (enum sim_rl78_regnum regno)
{
  size_t size;

  if (regno == sim_rl78_pc_regnum)
    size = 4;
  else
    size = 1;

  return size;
}

/* Return the register address associated with the register specified by
   REGNO.  */

static unsigned long
reg_addr (enum sim_rl78_regnum regno)
{
  if (sim_rl78_bank0_r0_regnum <= regno
      && regno <= sim_rl78_bank0_r7_regnum)
    return 0xffef8 + (regno - sim_rl78_bank0_r0_regnum);
  else if (sim_rl78_bank1_r0_regnum <= regno
           && regno <= sim_rl78_bank1_r7_regnum)
    return 0xffef0 + (regno - sim_rl78_bank1_r0_regnum);
  else if (sim_rl78_bank2_r0_regnum <= regno
           && regno <= sim_rl78_bank2_r7_regnum)
    return 0xffee8 + (regno - sim_rl78_bank2_r0_regnum);
  else if (sim_rl78_bank3_r0_regnum <= regno
           && regno <= sim_rl78_bank3_r7_regnum)
    return 0xffee0 + (regno - sim_rl78_bank3_r0_regnum);
  else if (regno == sim_rl78_psw_regnum)
    return 0xffffa;
  else if (regno == sim_rl78_es_regnum)
    return 0xffffd;
  else if (regno == sim_rl78_cs_regnum)
    return 0xffffc;
  /* Note: We can't handle PC here because it's not memory mapped.  */
  else if (regno == sim_rl78_spl_regnum)
    return 0xffff8;
  else if (regno == sim_rl78_sph_regnum)
    return 0xffff9;
  else if (regno == sim_rl78_pmc_regnum)
    return 0xffffe;
  else if (regno == sim_rl78_mem_regnum)
    return 0xfffff;

  return 0;
}

/* Fetch the contents of the register specified by REGNO, placing the
   contents in BUF.  The length LENGTH must match the sim's internal
   notion of the register's size.  */

int
sim_fetch_register (SIM_DESC sd, int regno, void *buf, int length)
{
  size_t size;
  SI val;

  check_desc (sd);

  if (!check_regno (regno))
    return 0;

  size = reg_size (regno);

  if (length != size)
    return 0;

  if (regno == sim_rl78_pc_regnum)
    val = pc;
  else
    val = memory[reg_addr (regno)];

  put_le (buf, length, val);

  return size;
}

/* Store the value stored in BUF to the register REGNO.  The length
   LENGTH must match the sim's internal notion of the register size.  */

int
sim_store_register (SIM_DESC sd, int regno, const void *buf, int length)
{
  size_t size;
  SI val;

  check_desc (sd);

  if (!check_regno (regno))
    return -1;

  size = reg_size (regno);

  if (length != size)
    return -1;

  val = get_le (buf, length);

  if (regno == sim_rl78_pc_regnum)
    {
      pc = val;

      /* The rl78 program counter is 20 bits wide.  Ensure that GDB
         hasn't picked up any stray bits.  This has occurred when performing
	 a GDB "return" command in which the return address is obtained
	 from a 32-bit container on the stack.  */
      assert ((pc & ~0x0fffff) == 0);
    }
  else
    memory[reg_addr (regno)] = val;
  return size;
}

/* Print out message associated with "info target".  */

void
sim_info (SIM_DESC sd, bool verbose)
{
  check_desc (sd);

  printf ("The rl78 minisim doesn't collect any statistics.\n");
}

static volatile int stop;
static enum sim_stop reason;
int siggnal;


/* Given a signal number used by the rl78 bsp (that is, newlib),
   return the corresponding signal numbers.  */

static int
rl78_signal_to_target (int sig)
{
  switch (sig)
    {
    case 4:
      return GDB_SIGNAL_ILL;

    case 5:
      return GDB_SIGNAL_TRAP;

    case 10:
      return GDB_SIGNAL_BUS;

    case 11:
      return GDB_SIGNAL_SEGV;

    case 24:
      return GDB_SIGNAL_XCPU;
      break;

    case 2:
      return GDB_SIGNAL_INT;

    case 8:
      return GDB_SIGNAL_FPE;
      break;

    case 6:
      return GDB_SIGNAL_ABRT;
    }

  return 0;
}


/* Take a step return code RC and set up the variables consulted by
   sim_stop_reason appropriately.  */

static void
handle_step (int rc)
{
  if (RL78_STEPPED (rc) || RL78_HIT_BREAK (rc))
    {
      reason = sim_stopped;
      siggnal = GDB_SIGNAL_TRAP;
    }
  else if (RL78_STOPPED (rc))
    {
      reason = sim_stopped;
      siggnal = rl78_signal_to_target (RL78_STOP_SIG (rc));
    }
  else
    {
      assert (RL78_EXITED (rc));
      reason = sim_exited;
      siggnal = RL78_EXIT_STATUS (rc);
    }
}


/* Resume execution after a stop.  */

void
sim_resume (SIM_DESC sd, int step, int sig_to_deliver)
{
  int rc;

  check_desc (sd);

  if (sig_to_deliver != 0)
    {
      fprintf (stderr,
	       "Warning: the rl78 minisim does not implement "
	       "signal delivery yet.\n" "Resuming with no signal.\n");
    }

      /* We don't clear 'stop' here, because then we would miss
         interrupts that arrived on the way here.  Instead, we clear
         the flag in sim_stop_reason, after GDB has disabled the
         interrupt signal handler.  */
  for (;;)
    {
      if (stop)
	{
	  stop = 0;
	  reason = sim_stopped;
	  siggnal = GDB_SIGNAL_INT;
	  break;
	}

      rc = setjmp (decode_jmp_buf);
      if (rc == 0)
	rc = decode_opcode ();

      if (!RL78_STEPPED (rc) || step)
	{
	  handle_step (rc);
	  break;
	}
    }
}

/* Stop the sim.  */

int
sim_stop (SIM_DESC sd)
{
  stop = 1;

  return 1;
}

/* Fetch the stop reason and signal.  */

void
sim_stop_reason (SIM_DESC sd, enum sim_stop *reason_p, int *sigrc_p)
{
  check_desc (sd);

  *reason_p = reason;
  *sigrc_p = siggnal;
}

/* Execute the sim-specific command associated with GDB's "sim ..."
   command.  */

void
sim_do_command (SIM_DESC sd, const char *cmd)
{
  const char *arg;
  char **argv = buildargv (cmd);

  check_desc (sd);

  cmd = arg = "";
  if (argv != NULL)
    {
      if (argv[0] != NULL)
	cmd = argv[0];
      if (argv[1] != NULL)
	arg = argv[1];
    }

  if (strcmp (cmd, "trace") == 0)
    {
      if (strcmp (arg, "on") == 0)
	trace = 1;
      else if (strcmp (arg, "off") == 0)
	trace = 0;
      else
	printf ("The 'sim trace' command expects 'on' or 'off' "
		"as an argument.\n");
    }
  else if (strcmp (cmd, "verbose") == 0)
    {
      if (strcmp (arg, "on") == 0)
	verbose = 1;
      else if (strcmp (arg, "noisy") == 0)
	verbose = 2;
      else if (strcmp (arg, "off") == 0)
	verbose = 0;
      else
	printf ("The 'sim verbose' command expects 'on', 'noisy', or 'off'"
		" as an argument.\n");
    }
  else
    printf ("The 'sim' command expects either 'trace' or 'verbose'"
	    " as a subcommand.\n");

  freeargv (argv);
}

/* Stub for command completion.  */

char **
sim_complete_command (SIM_DESC sd, const char *text, const char *word)
{
    return NULL;
}

char *
sim_memory_map (SIM_DESC sd)
{
  return NULL;
}
