/* gdb-if.c -- sim interface to GDB.

Copyright (C) 2008-2024 Free Software Foundation, Inc.
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
#include "bfd.h"
#include "libiberty.h"
#include "sim/callback.h"
#include "sim/sim.h"
#include "gdb/signals.h"
#include "sim/sim-rx.h"

#include "cpu.h"
#include "mem.h"
#include "load.h"
#include "syscalls.h"
#include "err.h"
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
  "This is the sole rx minisim instance.  See libsim.a's global variables."
};

static int rx_sim_is_open;

SIM_DESC
sim_open (SIM_OPEN_KIND kind,
	  struct host_callback_struct *callback,
	  struct bfd *abfd, char * const *argv)
{
  if (rx_sim_is_open)
    fprintf (stderr, "rx minisim: re-opened sim\n");

  /* The 'run' interface doesn't use this function, so we don't care
     about KIND; it's always SIM_OPEN_DEBUG.  */
  if (kind != SIM_OPEN_DEBUG)
    fprintf (stderr, "rx minisim: sim_open KIND != SIM_OPEN_DEBUG: %d\n",
	     kind);

  set_callbacks (callback);

  /* We don't expect any command-line arguments.  */

  init_mem ();
  init_regs ();
  execution_error_init_debugger ();

  sim_disasm_init (abfd);
  rx_sim_is_open = 1;
  return &the_minisim;
}

static void
check_desc (SIM_DESC sd)
{
  if (sd != &the_minisim)
    fprintf (stderr, "rx minisim: desc != &the_minisim\n");
}

void
sim_close (SIM_DESC sd, int quitting)
{
  check_desc (sd);

  /* Not much to do.  At least free up our memory.  */
  init_mem ();

  rx_sim_is_open = 0;
}

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
      fprintf (stderr, "%s not a rx program\n", filename);
      return 0;
    }

  return prog;
}

static struct swap_list
{
  bfd_vma start, end;
  struct swap_list *next;
} *swap_list = NULL;

static void
free_swap_list (void)
{
  while (swap_list)
    {
      struct swap_list *next = swap_list->next;
      free (swap_list);
      swap_list = next;
    }
}

/* When running in big endian mode, we must do an additional
   byte swap of memory areas used to hold instructions.  See
   the comment preceding rx_load in load.c to see why this is
   so.

   Construct a list of memory areas that must be byte swapped.
   This list will be consulted when either reading or writing
   memory.  */

static void
build_swap_list (struct bfd *abfd)
{
  asection *s;
  free_swap_list ();
  
  /* Nothing to do when in little endian mode.  */
  if (!rx_big_endian)
    return;

  for (s = abfd->sections; s; s = s->next)
    {
      if ((s->flags & SEC_LOAD) && (s->flags & SEC_CODE))
	{
	  struct swap_list *sl;
	  bfd_size_type size;

	  size = bfd_section_size (s);
	  if (size <= 0)
	    continue;
	  
	  sl = malloc (sizeof (struct swap_list));
	  assert (sl != NULL);
	  sl->next = swap_list;
	  sl->start = bfd_section_lma (s);
	  sl->end = sl->start + size;
	  swap_list = sl;
	}
    }
}

static int
addr_in_swap_list (bfd_vma addr)
{
  struct swap_list *s;

  for (s = swap_list; s; s = s->next)
    {
      if (s->start <= addr && addr < s->end)
	return 1;
    }
  return 0;
}

SIM_RC
sim_load (SIM_DESC sd, const char *prog, struct bfd *abfd, int from_tty)
{
  check_desc (sd);

  if (!abfd)
    abfd = open_objfile (prog);
  if (!abfd)
    return SIM_RC_FAIL;

  rx_load (abfd, get_callbacks ());
  build_swap_list (abfd);

  return SIM_RC_OK;
}

SIM_RC
sim_create_inferior (SIM_DESC sd, struct bfd *abfd,
		     char * const *argv, char * const *env)
{
  check_desc (sd);

  if (abfd)
    {
      rx_load (abfd, NULL);
      build_swap_list (abfd);
    }

  return SIM_RC_OK;
}

uint64_t
sim_read (SIM_DESC sd, uint64_t mem, void *buffer, uint64_t length)
{
  int i;
  unsigned char *data = buffer;

  check_desc (sd);

  if (mem == 0)
    return 0;

  execution_error_clear_last_error ();

  for (i = 0; i < length; i++)
    {
      bfd_vma addr = mem + i;
      int do_swap = addr_in_swap_list (addr);
      data[i] = mem_get_qi (addr ^ (do_swap ? 3 : 0));

      if (execution_error_get_last_error () != SIM_ERR_NONE)
	return i;
    }

  return length;
}

uint64_t
sim_write (SIM_DESC sd, uint64_t mem, const void *buffer, uint64_t length)
{
  int i;
  const unsigned char *data = buffer;

  check_desc (sd);

  execution_error_clear_last_error ();

  for (i = 0; i < length; i++)
    {
      bfd_vma addr = mem + i;
      int do_swap = addr_in_swap_list (addr);
      mem_put_qi (addr ^ (do_swap ? 3 : 0), data[i]);

      if (execution_error_get_last_error () != SIM_ERR_NONE)
	return i;
    }

  return length;
}

/* Read the LENGTH bytes at BUF as an little-endian value.  */
static DI
get_le (const unsigned char *buf, int length)
{
  DI acc = 0;
  while (--length >= 0)
    acc = (acc << 8) + buf[length];

  return acc;
}

/* Read the LENGTH bytes at BUF as a big-endian value.  */
static DI
get_be (const unsigned char *buf, int length)
{
  DI acc = 0;
  while (length-- > 0)
    acc = (acc << 8) + *buf++;

  return acc;
}

/* Store VAL as a little-endian value in the LENGTH bytes at BUF.  */
static void
put_le (unsigned char *buf, int length, DI val)
{
  int i;

  for (i = 0; i < length; i++)
    {
      buf[i] = val & 0xff;
      val >>= 8;
    }
}

/* Store VAL as a big-endian value in the LENGTH bytes at BUF.  */
static void
put_be (unsigned char *buf, int length, DI val)
{
  int i;

  for (i = length-1; i >= 0; i--)
    {
      buf[i] = val & 0xff;
      val >>= 8;
    }
}


static int
check_regno (enum sim_rx_regnum regno)
{
  return 0 <= regno && regno < sim_rx_num_regs;
}

static size_t
reg_size (enum sim_rx_regnum regno)
{
  size_t size;

  switch (regno)
    {
    case sim_rx_r0_regnum:
      size = sizeof (regs.r[0]);
      break;
    case sim_rx_r1_regnum:
      size = sizeof (regs.r[1]);
      break;
    case sim_rx_r2_regnum:
      size = sizeof (regs.r[2]);
      break;
    case sim_rx_r3_regnum:
      size = sizeof (regs.r[3]);
      break;
    case sim_rx_r4_regnum:
      size = sizeof (regs.r[4]);
      break;
    case sim_rx_r5_regnum:
      size = sizeof (regs.r[5]);
      break;
    case sim_rx_r6_regnum:
      size = sizeof (regs.r[6]);
      break;
    case sim_rx_r7_regnum:
      size = sizeof (regs.r[7]);
      break;
    case sim_rx_r8_regnum:
      size = sizeof (regs.r[8]);
      break;
    case sim_rx_r9_regnum:
      size = sizeof (regs.r[9]);
      break;
    case sim_rx_r10_regnum:
      size = sizeof (regs.r[10]);
      break;
    case sim_rx_r11_regnum:
      size = sizeof (regs.r[11]);
      break;
    case sim_rx_r12_regnum:
      size = sizeof (regs.r[12]);
      break;
    case sim_rx_r13_regnum:
      size = sizeof (regs.r[13]);
      break;
    case sim_rx_r14_regnum:
      size = sizeof (regs.r[14]);
      break;
    case sim_rx_r15_regnum:
      size = sizeof (regs.r[15]);
      break;
    case sim_rx_isp_regnum:
      size = sizeof (regs.r_isp);
      break;
    case sim_rx_usp_regnum:
      size = sizeof (regs.r_usp);
      break;
    case sim_rx_intb_regnum:
      size = sizeof (regs.r_intb);
      break;
    case sim_rx_pc_regnum:
      size = sizeof (regs.r_pc);
      break;
    case sim_rx_ps_regnum:
      size = sizeof (regs.r_psw);
      break;
    case sim_rx_bpc_regnum:
      size = sizeof (regs.r_bpc);
      break;
    case sim_rx_bpsw_regnum:
      size = sizeof (regs.r_bpsw);
      break;
    case sim_rx_fintv_regnum:
      size = sizeof (regs.r_fintv);
      break;
    case sim_rx_fpsw_regnum:
      size = sizeof (regs.r_fpsw);
      break;
    case sim_rx_acc_regnum:
      size = sizeof (regs.r_acc);
      break;
    default:
      size = 0;
      break;
    }
  return size;
}

int
sim_fetch_register (SIM_DESC sd, int regno, void *buf, int length)
{
  size_t size;
  DI val;

  check_desc (sd);

  if (!check_regno (regno))
    return 0;

  size = reg_size (regno);

  if (length != size)
    return 0;

  switch (regno)
    {
    case sim_rx_r0_regnum:
      val = get_reg (0);
      break;
    case sim_rx_r1_regnum:
      val = get_reg (1);
      break;
    case sim_rx_r2_regnum:
      val = get_reg (2);
      break;
    case sim_rx_r3_regnum:
      val = get_reg (3);
      break;
    case sim_rx_r4_regnum:
      val = get_reg (4);
      break;
    case sim_rx_r5_regnum:
      val = get_reg (5);
      break;
    case sim_rx_r6_regnum:
      val = get_reg (6);
      break;
    case sim_rx_r7_regnum:
      val = get_reg (7);
      break;
    case sim_rx_r8_regnum:
      val = get_reg (8);
      break;
    case sim_rx_r9_regnum:
      val = get_reg (9);
      break;
    case sim_rx_r10_regnum:
      val = get_reg (10);
      break;
    case sim_rx_r11_regnum:
      val = get_reg (11);
      break;
    case sim_rx_r12_regnum:
      val = get_reg (12);
      break;
    case sim_rx_r13_regnum:
      val = get_reg (13);
      break;
    case sim_rx_r14_regnum:
      val = get_reg (14);
      break;
    case sim_rx_r15_regnum:
      val = get_reg (15);
      break;
    case sim_rx_isp_regnum:
      val = get_reg (isp);
      break;
    case sim_rx_usp_regnum:
      val = get_reg (usp);
      break;
    case sim_rx_intb_regnum:
      val = get_reg (intb);
      break;
    case sim_rx_pc_regnum:
      val = get_reg (pc);
      break;
    case sim_rx_ps_regnum:
      val = get_reg (psw);
      break;
    case sim_rx_bpc_regnum:
      val = get_reg (bpc);
      break;
    case sim_rx_bpsw_regnum:
      val = get_reg (bpsw);
      break;
    case sim_rx_fintv_regnum:
      val = get_reg (fintv);
      break;
    case sim_rx_fpsw_regnum:
      val = get_reg (fpsw);
      break;
    case sim_rx_acc_regnum:
      val = ((DI) get_reg (acchi) << 32) | get_reg (acclo);
      break;
    default:
      fprintf (stderr, "rx minisim: unrecognized register number: %d\n",
	       regno);
      return -1;
    }

  if (rx_big_endian)
    put_be (buf, length, val);
  else
    put_le (buf, length, val);

  return size;
}

int
sim_store_register (SIM_DESC sd, int regno, const void *buf, int length)
{
  size_t size;
  DI val;

  check_desc (sd);

  if (!check_regno (regno))
    return -1;

  size = reg_size (regno);

  if (length != size)
    return -1;

  if (rx_big_endian)
    val = get_be (buf, length);
  else
    val = get_le (buf, length);

  switch (regno)
    {
    case sim_rx_r0_regnum:
      put_reg (0, val);
      break;
    case sim_rx_r1_regnum:
      put_reg (1, val);
      break;
    case sim_rx_r2_regnum:
      put_reg (2, val);
      break;
    case sim_rx_r3_regnum:
      put_reg (3, val);
      break;
    case sim_rx_r4_regnum:
      put_reg (4, val);
      break;
    case sim_rx_r5_regnum:
      put_reg (5, val);
      break;
    case sim_rx_r6_regnum:
      put_reg (6, val);
      break;
    case sim_rx_r7_regnum:
      put_reg (7, val);
      break;
    case sim_rx_r8_regnum:
      put_reg (8, val);
      break;
    case sim_rx_r9_regnum:
      put_reg (9, val);
      break;
    case sim_rx_r10_regnum:
      put_reg (10, val);
      break;
    case sim_rx_r11_regnum:
      put_reg (11, val);
      break;
    case sim_rx_r12_regnum:
      put_reg (12, val);
      break;
    case sim_rx_r13_regnum:
      put_reg (13, val);
      break;
    case sim_rx_r14_regnum:
      put_reg (14, val);
      break;
    case sim_rx_r15_regnum:
      put_reg (15, val);
      break;
    case sim_rx_isp_regnum:
      put_reg (isp, val);
      break;
    case sim_rx_usp_regnum:
      put_reg (usp, val);
      break;
    case sim_rx_intb_regnum:
      put_reg (intb, val);
      break;
    case sim_rx_pc_regnum:
      put_reg (pc, val);
      break;
    case sim_rx_ps_regnum:
      put_reg (psw, val);
      break;
    case sim_rx_bpc_regnum:
      put_reg (bpc, val);
      break;
    case sim_rx_bpsw_regnum:
      put_reg (bpsw, val);
      break;
    case sim_rx_fintv_regnum:
      put_reg (fintv, val);
      break;
    case sim_rx_fpsw_regnum:
      put_reg (fpsw, val);
      break;
    case sim_rx_acc_regnum:
      put_reg (acclo, val & 0xffffffff);
      put_reg (acchi, (val >> 32) & 0xffffffff);
      break;
    default:
      fprintf (stderr, "rx minisim: unrecognized register number: %d\n",
	       regno);
      return 0;
    }

  return size;
}

void
sim_info (SIM_DESC sd, bool verbose)
{
  check_desc (sd);

  printf ("The rx minisim doesn't collect any statistics.\n");
}

static volatile int stop;
static enum sim_stop reason;
int siggnal;


/* Given a signal number used by the RX bsp (that is, newlib),
   return a target signal number used by GDB.  */
static int
rx_signal_to_gdb_signal (int rx)
{
  switch (rx)
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

    case 2:
      return GDB_SIGNAL_INT;

    case 8:
      return GDB_SIGNAL_FPE;

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
  if (execution_error_get_last_error () != SIM_ERR_NONE)
    {
      reason = sim_stopped;
      siggnal = GDB_SIGNAL_SEGV;
    }
  if (RX_STEPPED (rc) || RX_HIT_BREAK (rc))
    {
      reason = sim_stopped;
      siggnal = GDB_SIGNAL_TRAP;
    }
  else if (RX_STOPPED (rc))
    {
      reason = sim_stopped;
      siggnal = rx_signal_to_gdb_signal (RX_STOP_SIG (rc));
    }
  else
    {
      assert (RX_EXITED (rc));
      reason = sim_exited;
      siggnal = RX_EXIT_STATUS (rc);
    }
}


void
sim_resume (SIM_DESC sd, int step, int sig_to_deliver)
{
  int rc;

  check_desc (sd);

  if (sig_to_deliver != 0)
    {
      fprintf (stderr,
	       "Warning: the rx minisim does not implement "
	       "signal delivery yet.\n" "Resuming with no signal.\n");
    }

  execution_error_clear_last_error ();

  if (step)
    {
      rc = setjmp (decode_jmp_buf);
      if (rc == 0)
	rc = decode_opcode ();
      handle_step (rc);
    }
  else
    {
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

	  if (execution_error_get_last_error () != SIM_ERR_NONE)
	    {
	      reason = sim_stopped;
	      siggnal = GDB_SIGNAL_SEGV;
	      break;
	    }

	  if (!RX_STEPPED (rc))
	    {
	      handle_step (rc);
	      break;
	    }
	}
    }
}

int
sim_stop (SIM_DESC sd)
{
  stop = 1;

  return 1;
}

void
sim_stop_reason (SIM_DESC sd, enum sim_stop *reason_p, int *sigrc_p)
{
  check_desc (sd);

  *reason_p = reason;
  *sigrc_p = siggnal;
}

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

char **
sim_complete_command (SIM_DESC sd, const char *text, const char *word)
{
  return NULL;
}

/* Stub this out for now.  */

char *
sim_memory_map (SIM_DESC sd)
{
  return NULL;
}
