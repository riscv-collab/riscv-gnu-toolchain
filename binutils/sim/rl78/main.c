/* main.c --- main function for stand-alone RL78 simulator.

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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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

#include "libiberty.h"
#include "bfd.h"

#include "cpu.h"
#include "mem.h"
#include "load.h"
#include "trace.h"

static int disassemble = 0;
static const char * dump_counts_filename = NULL;

static void
done (int exit_code)
{
  if (verbose)
    {
      printf ("Exit code: %d\n", exit_code);
      printf ("total clocks: %lld\n", total_clocks);
    }
  if (dump_counts_filename)
    dump_counts_per_insn (dump_counts_filename);
  exit (exit_code);
}

int
main (int argc, char **argv)
{
  int o;
  int save_trace;
  bfd *prog;
  int rc;
  static const struct option longopts[] = { { 0 } };

  xmalloc_set_program_name (argv[0]);

  while ((o = getopt_long (argc, argv, "tvdr:D:M:", longopts, NULL))
	 != -1)
    {
      switch (o)
	{
	case 't':
	  trace ++;
	  break;
	case 'v':
	  verbose ++;
	  break;
	case 'd':
	  disassemble ++;
	  break;
	case 'r':
	  mem_ram_size (atoi (optarg));
	  break;
	case 'D':
	  dump_counts_filename = optarg;
	  break;
	case 'M':
	  if (strcmp (optarg, "g10") == 0)
	    {
	      rl78_g10_mode = 1;
	      g13_multiply = 0;
	      g14_multiply = 0;
	      mem_set_mirror (0, 0xf8000, 4096);
	    }
	  if (strcmp (optarg, "g13") == 0)
	    {
	      rl78_g10_mode = 0;
	      g13_multiply = 1;
	      g14_multiply = 0;
	    }
	  if (strcmp (optarg, "g14") == 0)
	    {
	      rl78_g10_mode = 0;
	      g13_multiply = 0;
	      g14_multiply = 1;
	    }
	  break;
	case '?':
	  {
	    fprintf (stderr,
		     "usage: run [options] program [arguments]\n");
	    fprintf (stderr,
		     "\t-v\t\t- increase verbosity.\n"
		     "\t-t\t\t- trace.\n"
		     "\t-d\t\t- disassemble.\n"
		     "\t-r <bytes>\t- ram size.\n"
		     "\t-M <mcu>\t- mcu type, default none, allowed: g10,g13,g14\n"
		     "\t-D <filename>\t- dump cycle count histogram\n");
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
      fprintf (stderr, "%s not a rl78 program\n", argv[optind]);
      exit (1);
    }

  init_cpu ();

  rl78_in_gdb = 0;
  save_trace = trace;
  trace = 0;
  rl78_load (prog, 0, argv[0]);
  trace = save_trace;

  sim_disasm_init (prog);

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
	      sim_disasm_one ();

	    rc = decode_opcode ();

	    if (trace)
	      trace_register_changes ();
	  }
    }

  if (RL78_HIT_BREAK (rc))
    done (1);
  else if (RL78_EXITED (rc))
    done (RL78_EXIT_STATUS (rc));
  else if (RL78_STOPPED (rc))
    {
      if (verbose)
	printf ("Stopped on signal %d\n", RL78_STOP_SIG (rc));
      exit (1);
    }
  done (0);
  exit (0);
}
