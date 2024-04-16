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
 * Pretty-print trace of api calls to the /proc api
 */

#include "defs.h"
#include "gdbcmd.h"
#include "completer.h"

#include <sys/types.h>
#include <sys/procfs.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <fcntl.h>
#include "gdbsupport/gdb_wait.h"

#include "proc-utils.h"

/*  Much of the information used in the /proc interface, particularly for
    printing status information, is kept as tables of structures of the
    following form.  These tables can be used to map numeric values to
    their symbolic names and to a string that describes their specific use.  */

struct trans {
  long value;                   /* The numeric value */
  const char *name;             /* The equivalent symbolic value */
  const char *desc;             /* Short description of value */
};

static bool  procfs_trace   = false;
static FILE *procfs_file     = NULL;
static std::string procfs_filename = "procfs_trace";

static void
prepare_to_trace (void)
{
  if (procfs_trace)			/* if procfs tracing turned on */
    if (procfs_file == NULL)		/* if output file not yet open */
      procfs_file = fopen (procfs_filename.c_str (), "a");	/* open output file */
}

static void
set_procfs_trace_cmd (const char *args,
		      int from_tty, struct cmd_list_element *c)
{
#if 0	/* not sure what I might actually need to do here, if anything */
  if (procfs_file)
    fflush (procfs_file);
#endif
}

static void
set_procfs_file_cmd (const char *args,
		     int from_tty, struct cmd_list_element *c)
{
  /* Just changed the filename for procfs tracing.
     If a file was already open, close it.  */
  if (procfs_file)
    fclose (procfs_file);
  procfs_file = NULL;
}

static struct trans rw_table[] = {
  { PCAGENT,  "PCAGENT",  "create agent lwp with regs from argument" },
  { PCCFAULT, "PCCFAULT", "clear current fault" },
  { PCCSIG,   "PCCSIG",   "clear current signal" },
  { PCDSTOP,  "PCDSTOP",  "post stop request" },
  { PCKILL,   "PCKILL",   "post a signal" },
  { PCNICE,   "PCNICE",   "set nice priority" },
  { PCREAD,   "PCREAD",   "read from the address space" },
  { PCWRITE,  "PCWRITE",  "write to the address space" },
  { PCRUN,    "PCRUN",    "make process/lwp runnable" },
  { PCSASRS,  "PCSASRS",  "set ancillary state registers" },
  { PCSCRED,  "PCSCRED",  "set process credentials" },
  { PCSENTRY, "PCSENTRY", "set traced syscall entry set" },
  { PCSET,    "PCSET",    "set modes" },
  { PCSEXIT,  "PCSEXIT",  "set traced syscall exit  set" },
  { PCSFAULT, "PCSFAULT", "set traced fault set" },
  { PCSFPREG, "PCSFPREG", "set floating point registers" },
  { PCSHOLD,  "PCSHOLD",  "set signal mask" },
  { PCSREG,   "PCSREG",   "set general registers" },
  { PCSSIG,   "PCSSIG",   "set current signal" },
  { PCSTOP,   "PCSTOP",   "post stop request and wait" },
  { PCSTRACE, "PCSTRACE", "set traced signal set" },
  { PCSVADDR, "PCSVADDR", "set pc virtual address" },
  { PCSXREG,  "PCSXREG",  "set extra registers" },
  { PCTWSTOP, "PCTWSTOP", "wait for stop, with timeout arg" },
  { PCUNKILL, "PCUNKILL", "delete a pending signal" },
  { PCUNSET,  "PCUNSET",  "unset modes" },
  { PCWATCH,  "PCWATCH",  "set/unset watched memory area" },
  { PCWSTOP,  "PCWSTOP",  "wait for process/lwp to stop, no timeout" },
  { 0,        NULL,      NULL }
};

static off_t lseek_offset;

int
write_with_trace (int fd, void *varg, size_t len, char *file, int line)
{
  int i = ARRAY_SIZE (rw_table) - 1;
  int ret;
  procfs_ctl_t *arg = (procfs_ctl_t *) varg;

  prepare_to_trace ();
  if (procfs_trace)
    {
      procfs_ctl_t opcode = arg[0];
      for (i = 0; rw_table[i].name != NULL; i++)
	if (rw_table[i].value == opcode)
	  break;

      if (info_verbose)
	fprintf (procfs_file ? procfs_file : stdout, 
		 "%s:%d -- ", file, line);
      switch (opcode) {
      case PCSET:
	fprintf (procfs_file ? procfs_file : stdout, 
		 "write (PCSET,   %s) %s\n", 
		 arg[1] == PR_FORK  ? "PR_FORK"  :
		 arg[1] == PR_RLC   ? "PR_RLC"   :
		 arg[1] == PR_ASYNC ? "PR_ASYNC" :
		 "<unknown flag>",
		 info_verbose ? rw_table[i].desc : "");
	break;
      case PCUNSET:
	fprintf (procfs_file ? procfs_file : stdout, 
		 "write (PCRESET, %s) %s\n", 
		 arg[1] == PR_FORK  ? "PR_FORK"  :
		 arg[1] == PR_RLC   ? "PR_RLC"   :
		 arg[1] == PR_ASYNC ? "PR_ASYNC" :
		 "<unknown flag>",
		 info_verbose ? rw_table[i].desc : "");
	break;
      case PCSTRACE:
	fprintf (procfs_file ? procfs_file : stdout, 
		 "write (PCSTRACE) ");
	proc_prettyfprint_signalset (procfs_file ? procfs_file : stdout,
				     (sigset_t *) &arg[1], 0);
	break;
      case PCSFAULT:
	fprintf (procfs_file ? procfs_file : stdout, 
		 "write (PCSFAULT) ");
	proc_prettyfprint_faultset (procfs_file ? procfs_file : stdout,
				    (fltset_t *) &arg[1], 0);
	break;
      case PCSENTRY:
	fprintf (procfs_file ? procfs_file : stdout, 
		 "write (PCSENTRY) ");
	proc_prettyfprint_syscalls (procfs_file ? procfs_file : stdout,
				    (sysset_t *) &arg[1], 0);
	break;
      case PCSEXIT:
	fprintf (procfs_file ? procfs_file : stdout, 
		 "write (PCSEXIT) ");
	proc_prettyfprint_syscalls (procfs_file ? procfs_file : stdout,
				    (sysset_t *) &arg[1], 0);
	break;
      case PCSHOLD:
	fprintf (procfs_file ? procfs_file : stdout, 
		 "write (PCSHOLD) ");
	proc_prettyfprint_signalset (procfs_file ? procfs_file : stdout,
				     (sigset_t *) &arg[1], 0);
	break;
      case PCSSIG:
	fprintf (procfs_file ? procfs_file : stdout, 
		 "write (PCSSIG) ");
	proc_prettyfprint_signal (procfs_file ? procfs_file : stdout,
				  arg[1] ? ((siginfo_t *) &arg[1])->si_signo 
					 : 0, 
				  0);
	fprintf (procfs_file ? procfs_file : stdout, "\n");
	break;
      case PCRUN:
	fprintf (procfs_file ? procfs_file : stdout, 
		 "write (PCRUN) ");
	if (arg[1] & PRCSIG)
	  fprintf (procfs_file ? procfs_file : stdout, "clearSig ");
	if (arg[1] & PRCFAULT)
	  fprintf (procfs_file ? procfs_file : stdout, "clearFlt ");
	if (arg[1] & PRSTEP)
	  fprintf (procfs_file ? procfs_file : stdout, "step ");
	if (arg[1] & PRSABORT)
	  fprintf (procfs_file ? procfs_file : stdout, "syscallAbort ");
	if (arg[1] & PRSTOP)
	  fprintf (procfs_file ? procfs_file : stdout, "stopReq ");
	  
	fprintf (procfs_file ? procfs_file : stdout, "\n");
	break;
      case PCKILL:
	fprintf (procfs_file ? procfs_file : stdout, 
		 "write (PCKILL) ");
	proc_prettyfprint_signal (procfs_file ? procfs_file : stdout,
				  arg[1], 0);
	fprintf (procfs_file ? procfs_file : stdout, "\n");
	break;
      default:
	{
	  if (rw_table[i].name)
	    fprintf (procfs_file ? procfs_file : stdout, 
		     "write (%s) %s\n", 
		     rw_table[i].name, 
		     info_verbose ? rw_table[i].desc : "");
	  else
	    {
	      if (lseek_offset != -1)
		fprintf (procfs_file ? procfs_file : stdout, 
			 "write (<unknown>, %lud bytes at 0x%08lx) \n", 
			 (unsigned long) len, (unsigned long) lseek_offset);
	      else
		fprintf (procfs_file ? procfs_file : stdout, 
			 "write (<unknown>, %lud bytes) \n", 
			 (unsigned long) len);
	    }
	  break;
	}
      }
      if (procfs_file)
	fflush (procfs_file);
    }
  errno = 0;
  ret = write (fd, (void *) arg, len);
  if (procfs_trace && ret != len)
    {
      fprintf (procfs_file ? procfs_file : stdout, 
	       "[write (%s) FAILED! (%s)]\n",
	       rw_table[i].name != NULL ? 
	       rw_table[i].name : "<unknown>", 
	       safe_strerror (errno));
      if (procfs_file)
	fflush (procfs_file);
    }

  lseek_offset = -1;
  return ret;
}

off_t
lseek_with_trace (int fd, off_t offset, int whence, char *file, int line)
{
  off_t ret;

  prepare_to_trace ();
  errno = 0;
  ret = lseek (fd, offset, whence);
  lseek_offset = ret;
  if (procfs_trace && (ret == -1 || errno != 0))
    {
      fprintf (procfs_file ? procfs_file : stdout, 
	       "[lseek (0x%08lx) FAILED! (%s)]\n", 
	       (unsigned long) offset, safe_strerror (errno));
      if (procfs_file)
	fflush (procfs_file);
    }

  return ret;
}

int
open_with_trace (char *filename, int mode, char *file, int line)
{
  int ret;

  prepare_to_trace ();
  errno = 0;
  ret = open (filename, mode);
  if (procfs_trace)
    {
      if (info_verbose)
	fprintf (procfs_file ? procfs_file : stdout, 
		 "%s:%d -- ", file, line);

      if (errno)
	{
	  fprintf (procfs_file ? procfs_file : stdout, 
		   "[open FAILED! (%s) line %d]\\n", 
		   safe_strerror (errno), line);
	}
      else
	{
	  fprintf (procfs_file ? procfs_file : stdout, 
		   "%d = open (%s, ", ret, filename);
	  if (mode == O_RDONLY)
	    fprintf (procfs_file ? procfs_file : stdout, "O_RDONLY) %d\n",
		     line);
	  else if (mode == O_WRONLY)
	    fprintf (procfs_file ? procfs_file : stdout, "O_WRONLY) %d\n",
		     line);
	  else if (mode == O_RDWR)
	    fprintf (procfs_file ? procfs_file : stdout, "O_RDWR)   %d\n",
		     line);
	}
      if (procfs_file)
	fflush (procfs_file);
    }

  return ret;
}

int
close_with_trace (int fd, char *file, int line)
{
  int ret;

  prepare_to_trace ();
  errno = 0;
  ret = close (fd);
  if (procfs_trace)
    {
      if (info_verbose)
	fprintf (procfs_file ? procfs_file : stdout, 
		 "%s:%d -- ", file, line);
      if (errno)
	fprintf (procfs_file ? procfs_file : stdout, 
		 "[close FAILED! (%s)]\n", safe_strerror (errno));
      else
	fprintf (procfs_file ? procfs_file : stdout, 
		 "%d = close (%d)\n", ret, fd);
      if (procfs_file)
	fflush (procfs_file);
    }

  return ret;
}

pid_t
wait_with_trace (int *wstat, char *file, int line)
{
  int ret, lstat = 0;

  prepare_to_trace ();
  if (procfs_trace)
    {
      if (info_verbose)
	fprintf (procfs_file ? procfs_file : stdout, 
		 "%s:%d -- ", file, line);
      fprintf (procfs_file ? procfs_file : stdout, 
	       "wait (line %d) ", line);
      if (procfs_file)
	fflush (procfs_file);
    }
  errno = 0;
  ret = wait (&lstat);
  if (procfs_trace)
    {
      if (errno)
	fprintf (procfs_file ? procfs_file : stdout, 
		 "[wait FAILED! (%s)]\n", safe_strerror (errno));
      else
	fprintf (procfs_file ? procfs_file : stdout, 
		 "returned pid %d, status 0x%x\n", ret, lstat);
      if (procfs_file)
	fflush (procfs_file);
    }
  if (wstat)
    *wstat = lstat;

  return ret;
}

void
procfs_note (const char *msg, const char *file, int line)
{
  prepare_to_trace ();
  if (procfs_trace)
    {
      if (info_verbose)
	fprintf (procfs_file ? procfs_file : stdout, 
		 "%s:%d -- ", file, line);
      fprintf (procfs_file ? procfs_file : stdout, "%s", msg);
      if (procfs_file)
	fflush (procfs_file);
    }
}

void
proc_prettyfprint_status (long flags, int why, int what, int thread)
{
  prepare_to_trace ();
  if (procfs_trace)
    {
      if (thread)
	fprintf (procfs_file ? procfs_file : stdout,
		 "Thread %d: ", thread);

      proc_prettyfprint_flags (procfs_file ? procfs_file : stdout, 
			       flags, 0);

      if (flags & (PR_STOPPED | PR_ISTOP))
	proc_prettyfprint_why (procfs_file ? procfs_file : stdout, 
			       why, what, 0);
      if (procfs_file)
	fflush (procfs_file);
    }
}

void _initialize_proc_api ();
void
_initialize_proc_api ()
{
  add_setshow_boolean_cmd ("procfs-trace", no_class, &procfs_trace, _("\
Set tracing for /proc api calls."), _("\
Show tracing for /proc api calls."), NULL,
			   set_procfs_trace_cmd,
			   NULL, /* FIXME: i18n: */
			   &setlist, &showlist);

  add_setshow_filename_cmd ("procfs-file", no_class, &procfs_filename, _("\
Set filename for /proc tracefile."), _("\
Show filename for /proc tracefile."), NULL,
			    set_procfs_file_cmd,
			    NULL, /* FIXME: i18n: */
			    &setlist, &showlist);
}
