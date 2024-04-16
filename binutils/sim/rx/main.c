/* main.c --- main function for stand-alone RX simulator.

Copyright (C) 2005-2024 Free Software Foundation, Inc.
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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <setjmp.h>
#include <signal.h>
#include <getopt.h>

#include "bfd.h"

#include "cpu.h"
#include "mem.h"
#include "misc.h"
#include "load.h"
#include "trace.h"
#include "err.h"

static int disassemble = 0;

/* This must be higher than any other option index.  */
#define OPT_ACT 400

#define ACT(E,A) (OPT_ACT + SIM_ERR_##E * SIM_ERRACTION_NUM_ACTIONS + SIM_ERRACTION_##A)

static struct option sim_options[] =
{
  { "end-sim-args", 0, NULL, 'E' },
  { "exit-null-deref", 0, NULL, ACT(NULL_POINTER_DEREFERENCE,EXIT) },
  { "warn-null-deref", 0, NULL, ACT(NULL_POINTER_DEREFERENCE,WARN) },
  { "ignore-null-deref", 0, NULL, ACT(NULL_POINTER_DEREFERENCE,IGNORE) },
  { "exit-unwritten-pages", 0, NULL, ACT(READ_UNWRITTEN_PAGES,EXIT) },
  { "warn-unwritten-pages", 0, NULL, ACT(READ_UNWRITTEN_PAGES,WARN) },
  { "ignore-unwritten-pages", 0, NULL, ACT(READ_UNWRITTEN_PAGES,IGNORE) },
  { "exit-unwritten-bytes", 0, NULL, ACT(READ_UNWRITTEN_BYTES,EXIT) },
  { "warn-unwritten-bytes", 0, NULL, ACT(READ_UNWRITTEN_BYTES,WARN) },
  { "ignore-unwritten-bytes", 0, NULL, ACT(READ_UNWRITTEN_BYTES,IGNORE) },
  { "exit-corrupt-stack", 0, NULL, ACT(CORRUPT_STACK,EXIT) },
  { "warn-corrupt-stack", 0, NULL, ACT(CORRUPT_STACK,WARN) },
  { "ignore-corrupt-stack", 0, NULL, ACT(CORRUPT_STACK,IGNORE) },
  { 0, 0, 0, 0 }
};

static void
done (int exit_code)
{
  if (verbose)
    {
      stack_heap_stats ();
      mem_usage_stats ();
      /* Only use comma separated numbers when being very verbose.
	 Comma separated numbers are hard to parse in awk scripts.  */
      if (verbose > 1)
	printf ("insns: %14s\n", comma (rx_cycles));
      else
	printf ("insns: %u\n", rx_cycles);

      pipeline_stats ();
    }
  exit (exit_code);
}

int
main (int argc, char **argv)
{
  int o;
  int save_trace;
  bfd *prog;
  int rc;

  /* By default, we exit when an execution error occurs.  */
  execution_error_init_standalone ();

  while ((o = getopt_long (argc, argv, "tvdeEwi", sim_options, NULL)) != -1)
    {
      if (o == 'E')
	/* Stop processing the command line. This is so that any remaining
	   words on the command line that look like arguments will be passed
	   on to the program being simulated.  */
	break;

      if (o >= OPT_ACT)
	{
	  int e, a;

	  o -= OPT_ACT;
	  e = o / SIM_ERRACTION_NUM_ACTIONS;
	  a = o % SIM_ERRACTION_NUM_ACTIONS;
	  execution_error_set_action (e, a);
	}
      else switch (o)
	{
	case 't':
	  trace++;
	  break;
	case 'v':
	  verbose++;
	  break;
	case 'd':
	  disassemble++;
	  break;
	case 'e':
	  execution_error_init_standalone ();
	  break;
	case 'w':
	  execution_error_warn_all ();
	  break;
	case 'i':
	  execution_error_ignore_all ();
	  break;
	case '?':
	  {
	    int i;
	    fprintf (stderr,
		     "usage: run [options] program [arguments]\n");
	    fprintf (stderr,
		     "\t-v\t- increase verbosity.\n"
		     "\t-t\t- trace.\n"
		     "\t-d\t- disassemble.\n"
		     "\t-E\t- stop processing sim args\n"
		     "\t-e\t- exit on all execution errors.\n"
		     "\t-w\t- warn (do not exit) on all execution errors.\n"
		     "\t-i\t- ignore all execution errors.\n");
	    for (i=0; sim_options[i].name; i++)
	      fprintf (stderr, "\t--%s\n", sim_options[i].name);
	    exit (1);
	  }
	}
    }

  prog = bfd_openr (argv[optind], 0);
  if (!prog)
    {
      fprintf (stderr, "Can't read %s\n", argv[optind]);
      exit (1);
    }

  if (!bfd_check_format (prog, bfd_object))
    {
      fprintf (stderr, "%s not a rx program\n", argv[optind]);
      exit (1);
    }

  init_regs ();

  rx_in_gdb = 0;
  save_trace = trace;
  trace = 0;
  rx_load (prog, NULL);
  trace = save_trace;

  sim_disasm_init (prog);

  enable_counting = verbose;

  rc = setjmp (decode_jmp_buf);

  if (rc == 0)
    {
      if (!trace && !disassemble)
	{
	  /* This will longjmp to the above if an exception
	     happens.  */
	  for (;;)
	    decode_opcode ();
	}
      else
	while (1)
	  {

	    if (trace)
	      printf ("\n");

	    if (disassemble)
	      {
		enable_counting = 0;
		sim_disasm_one ();
		enable_counting = verbose;
	      }

	    rc = decode_opcode ();

	    if (trace)
	      trace_register_changes ();
	  }
    }

  if (RX_HIT_BREAK (rc))
    done (1);
  else if (RX_EXITED (rc))
    done (RX_EXIT_STATUS (rc));
  else if (RX_STOPPED (rc))
    {
      if (verbose)
	printf("Stopped on signal %d\n", RX_STOP_SIG (rc));
      exit(1);
    }
  done (0);
  exit (0);
}
