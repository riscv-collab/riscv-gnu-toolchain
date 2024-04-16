/* Machine-independent support for Solaris /proc (process file system)

   Copyright (C) 1999-2024 Free Software Foundation, Inc.

   Written by Michael Snyder at Cygnus Solutions.
   Based on work by Fred Fish, Stu Grossman, Geoff Noer, and others.

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

#include "defs.h"

#include <sys/types.h>
#include <sys/procfs.h>

#include "proc-utils.h"

/* Much of the information used in the /proc interface, particularly
   for printing status information, is kept as tables of structures of
   the following form.  These tables can be used to map numeric values
   to their symbolic names and to a string that describes their
   specific use.  */

struct trans
{
  int value;                    /* The numeric value.  */
  const char *name;                   /* The equivalent symbolic value.  */
  const char *desc;                   /* Short description of value.  */
};

/* Translate values in the pr_why field of a `struct prstatus' or
   `struct lwpstatus'.  */

static struct trans pr_why_table[] =
{
  { PR_REQUESTED, "PR_REQUESTED", 
    "Directed to stop by debugger via P(IO)CSTOP or P(IO)CWSTOP" },
  { PR_SIGNALLED, "PR_SIGNALLED", "Receipt of a traced signal" },
  { PR_SYSENTRY, "PR_SYSENTRY", "Entry to a traced system call" },
  { PR_SYSEXIT, "PR_SYSEXIT", "Exit from a traced system call" },
  { PR_JOBCONTROL, "PR_JOBCONTROL", "Default job control stop signal action" },
  { PR_FAULTED, "PR_FAULTED", "Incurred a traced hardware fault" },
  { PR_SUSPENDED, "PR_SUSPENDED", "Process suspended" },
  { PR_CHECKPOINT, "PR_CHECKPOINT", "Process stopped at checkpoint" },
};

/* Pretty-print the pr_why field of a `struct prstatus' or `struct
   lwpstatus'.  */

void
proc_prettyfprint_why (FILE *file, unsigned long why, unsigned long what,
		       int verbose)
{
  int i;

  if (why == 0)
    return;

  for (i = 0; i < ARRAY_SIZE (pr_why_table); i++)
    if (why == pr_why_table[i].value)
      {
	fprintf (file, "%s ", pr_why_table[i].name);
	if (verbose)
	  fprintf (file, ": %s ", pr_why_table[i].desc);

	switch (why) {
	case PR_REQUESTED:
	  break;		/* Nothing more to print.  */
	case PR_SIGNALLED:
	  proc_prettyfprint_signal (file, what, verbose);
	  break;
	case PR_FAULTED:
	  proc_prettyfprint_fault (file, what, verbose);
	  break;
	case PR_SYSENTRY:
	  fprintf (file, "Entry to ");
	  proc_prettyfprint_syscall (file, what, verbose);
	  break;
	case PR_SYSEXIT:
	  fprintf (file, "Exit from ");
	  proc_prettyfprint_syscall (file, what, verbose);
	  break;
	case PR_JOBCONTROL:
	  proc_prettyfprint_signal (file, what, verbose);
	  break;
	default:
	  fprintf (file, "Unknown why %ld, what %ld\n", why, what);
	  break;
	}
	fprintf (file, "\n");

	return;
      }

  fprintf (file, "Unknown pr_why.\n");
}

void
proc_prettyprint_why (unsigned long why, unsigned long what, int verbose)
{
  proc_prettyfprint_why (stdout, why, what, verbose);
}
