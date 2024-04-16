/* main.c --- main function for stand-alone M32C simulator.

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
#include <sys/types.h>
#include <getopt.h>

#ifdef HAVE_SYS_SOCKET_H
#ifdef HAVE_NETINET_IN_H
#ifdef HAVE_NETINET_TCP_H
#define HAVE_networking
#endif
#endif
#endif

#ifdef HAVE_networking
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#endif


#include "bfd.h"

#include "cpu.h"
#include "mem.h"
#include "misc.h"
#include "load.h"
#include "trace.h"
#ifdef TIMER_A
#include "int.h"
#include "timer_a.h"
#endif

#ifdef HAVE_networking
extern int m32c_console_ofd;
extern int m32c_console_ifd;
#endif

int m32c_disassemble = 0;
static unsigned int cycles = 0;

static void
done (int exit_code)
{
  if (verbose)
    {
      stack_heap_stats ();
      mem_usage_stats ();
      printf ("insns: %14s\n", comma (cycles));
    }
  exit (exit_code);
}

#ifdef HAVE_networking
static void
setup_tcp_console (char *portname)
{
  int port = atoi (portname);
  struct sockaddr_in address;
  int isocket;
  socklen_t as;
  unsigned char *a;

  if (port < 1024)
    {
      printf ("invalid port number %d\n", port);
      exit (1);
    }
  printf ("waiting for tcp console on port %d\n", port);

  memset (&address, 0, sizeof (address));
  address.sin_family = AF_INET;
  address.sin_port = htons (port);

  isocket = socket (AF_INET, SOCK_STREAM, 0);
  if (isocket == -1)
    {
      perror ("socket");
      exit (1);
    }

  if (bind (isocket, (struct sockaddr *) &address, sizeof (address)))
    {
      perror ("bind");
      exit (1);
    }
  listen (isocket, 2);

  printf ("waiting for connection...\n");
  as = sizeof (address);
  m32c_console_ifd = accept (isocket, (struct sockaddr *) &address, &as);
  if (m32c_console_ifd == -1)
    {
      perror ("accept");
      exit (1);
    }
  a = (unsigned char *) (&address.sin_addr.s_addr);
  printf ("connection from %d.%d.%d.%d\n", a[0], a[1], a[2], a[3]);
  m32c_console_ofd = m32c_console_ifd;
}
#endif

int
main (int argc, char **argv)
{
  int o;
  int save_trace;
  bfd *prog;
#ifdef HAVE_networking
  char *console_port_s = 0;
#endif
  static const struct option longopts[] = { { 0 } };

  setbuf (stdout, 0);

  in_gdb = 0;

  while ((o = getopt_long (argc, argv, "tc:vdm:C", longopts, NULL))
	 != -1)
    switch (o)
      {
      case 't':
	trace++;
	break;
      case 'c':
#ifdef HAVE_networking
	console_port_s = optarg;
#else
	fprintf (stderr, "Nework console not available in this build.\n");
#endif
	break;
      case 'C':
#ifdef HAVE_TERMIOS_H
	m32c_use_raw_console = 1;
#else
	fprintf (stderr, "Raw console not available in this build.\n");
#endif
	break;
      case 'v':
	verbose++;
	break;
      case 'd':
	m32c_disassemble++;
	break;
      case 'm':
	if (strcmp (optarg, "r8c") == 0 || strcmp (optarg, "m16c") == 0)
	  default_machine = bfd_mach_m16c;
	else if (strcmp (optarg, "m32cm") == 0
		 || strcmp (optarg, "m32c") == 0)
	  default_machine = bfd_mach_m32c;
	else
	  {
	    fprintf (stderr, "Invalid machine: %s\n", optarg);
	    exit (1);
	  }
	break;
      case '?':
	fprintf (stderr,
		 "usage: run [-v] [-C] [-c port] [-t] [-d] [-m r8c|m16c|m32cm|m32c]"
		 " program\n");
	exit (1);
      }

  prog = bfd_openr (argv[optind], 0);
  if (!prog)
    {
      fprintf (stderr, "Can't read %s\n", argv[optind]);
      exit (1);
    }

  if (!bfd_check_format (prog, bfd_object))
    {
      fprintf (stderr, "%s not a m32c program\n", argv[optind]);
      exit (1);
    }

  save_trace = trace;
  trace = 0;
  m32c_load (prog);
  trace = save_trace;

#ifdef HAVE_networking
  if (console_port_s)
    setup_tcp_console (console_port_s);
#endif

  sim_disasm_init (prog);

  while (1)
    {
      int rc;

      if (trace)
	printf ("\n");

      if (m32c_disassemble)
	sim_disasm_one ();

      enable_counting = verbose;
      cycles++;
      rc = decode_opcode ();
      enable_counting = 0;

      if (M32C_HIT_BREAK (rc))
	done (1);
      else if (M32C_EXITED (rc))
	done (M32C_EXIT_STATUS (rc));
      else
	assert (M32C_STEPPED (rc));

      trace_register_changes ();

#ifdef TIMER_A
      update_timer_a ();
#endif
    }
}
