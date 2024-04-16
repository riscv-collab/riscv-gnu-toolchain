/* Machine independent support for Solaris /proc (process file system) for GDB.
   Copyright (C) 1999-2024 Free Software Foundation, Inc.
   Written by Michael Snyder at Cygnus Solutions.
   Based on work by Fred Fish, Stu Grossman, Geoff Noer, and others.

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

/*
 * Pretty-print the prstatus flags.
 * 
 * Arguments: unsigned long flags, int verbose
 *
 */

#include "defs.h"

#include <sys/types.h>
#include <sys/procfs.h>

#include "proc-utils.h"

/*  Much of the information used in the /proc interface, particularly for
    printing status information, is kept as tables of structures of the
    following form.  These tables can be used to map numeric values to
    their symbolic names and to a string that describes their specific use.  */

struct trans {
  int value;                    /* The numeric value */
  const char *name;             /* The equivalent symbolic value */
  const char *desc;             /* Short description of value */
};

/* Translate bits in the pr_flags member of the prstatus structure,
   into the names and desc information.  */

static struct trans pr_flag_table[] =
{
  /* lwp is stopped */
  { PR_STOPPED, "PR_STOPPED", "Process (LWP) is stopped" },
  /* lwp is stopped on an event of interest */
  { PR_ISTOP, "PR_ISTOP", "Stopped on an event of interest" },
  /* lwp has a stop directive in effect */
  { PR_DSTOP, "PR_DSTOP", "A stop directive is in effect" },
  /* lwp has a single-step directive in effect */
  { PR_STEP, "PR_STEP", "A single step directive is in effect" },
  /* lwp is sleeping in a system call */
  { PR_ASLEEP, "PR_ASLEEP", "Sleeping in an (interruptible) system call" },
  /* contents of pr_instr undefined */
  { PR_PCINVAL, "PR_PCINVAL", "PC (pr_instr) is invalid" },
  /* this lwp is the aslwp */
  { PR_ASLWP, "PR_ASLWP", "This is the asynchronous signal LWP" },
  /* this lwp is the /proc agent lwp */
  { PR_AGENT, "PR_AGENT", "This is the /proc agent LWP" },
  /* this is a system process */
  { PR_ISSYS, "PR_ISSYS", "Is a system process/thread" },
  /* process is the parent of a vfork()d child */
  { PR_VFORKP, "PR_VFORKP", "Process is the parent of a vforked child" },
  /* process's process group is orphaned */
  { PR_ORPHAN, "PR_ORPHAN", "Process's process group is orphaned" },
  /* inherit-on-fork is in effect */
  { PR_FORK, "PR_FORK", "Inherit-on-fork is in effect" },
  /* run-on-last-close is in effect */
  { PR_RLC, "PR_RLC", "Run-on-last-close is in effect" },
  /* kill-on-last-close is in effect */
  { PR_KLC, "PR_KLC", "Kill-on-last-close is in effect" },
  /* asynchronous-stop is in effect */
  { PR_ASYNC, "PR_ASYNC", "Asynchronous stop is in effect" },
  /* micro-state usage accounting is in effect */
  { PR_MSACCT, "PR_MSACCT", "Microstate accounting enabled" },
  /* breakpoint trap pc adjustment is in effect */
  { PR_BPTADJ, "PR_BPTADJ", "Breakpoint PC adjustment in effect" },
  /* ptrace-compatibility mode is in effect */
  { PR_PTRACE, "PR_PTRACE", "Process is being controlled by ptrace" },
  /* micro-state accounting inherited on fork */
  { PR_MSFORK, "PR_PCOMPAT", "Micro-state accounting inherited on fork" },
};

void
proc_prettyfprint_flags (FILE *file, unsigned long flags, int verbose)
{
  int i;

  for (i = 0; i < sizeof (pr_flag_table) / sizeof (pr_flag_table[0]); i++)
    if (flags & pr_flag_table[i].value)
      {
	fprintf (file, "%s ", pr_flag_table[i].name);
	if (verbose)
	  fprintf (file, "%s\n", pr_flag_table[i].desc);
      }
  if (!verbose)
    fprintf (file, "\n");
}

void
proc_prettyprint_flags (unsigned long flags, int verbose)
{
  proc_prettyfprint_flags (stdout, flags, verbose);
}
