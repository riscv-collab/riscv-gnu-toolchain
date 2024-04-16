/* Native debugging support for Intel x86 running DJGPP.
   Copyright (C) 1997-2024 Free Software Foundation, Inc.
   Written by Robert Hoehne.

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

/* To whomever it may concern, here's a general description of how
   debugging in DJGPP works, and the special quirks GDB does to
   support that.

   When the DJGPP port of GDB is debugging a DJGPP program natively,
   there aren't 2 separate processes, the debuggee and GDB itself, as
   on other systems.  (This is DOS, where there can only be one active
   process at any given time, remember?)  Instead, GDB and the
   debuggee live in the same process.  So when GDB calls
   go32_create_inferior below, and that function calls edi_init from
   the DJGPP debug support library libdbg.a, we load the debuggee's
   executable file into GDB's address space, set it up for execution
   as the stub loader (a short real-mode program prepended to each
   DJGPP executable) normally would, and do a lot of preparations for
   swapping between GDB's and debuggee's internal state, primarily wrt
   the exception handlers.  This swapping happens every time we resume
   the debuggee or switch back to GDB's code, and it includes:

    . swapping all the segment registers
    . swapping the PSP (the Program Segment Prefix)
    . swapping the signal handlers
    . swapping the exception handlers
    . swapping the FPU status
    . swapping the 3 standard file handles (more about this below)

   Then running the debuggee simply means longjmp into it where its PC
   is and let it run until it stops for some reason.  When it stops,
   GDB catches the exception that stopped it and longjmp's back into
   its own code.  All the possible exit points of the debuggee are
   watched; for example, the normal exit point is recognized because a
   DOS program issues a special system call to exit.  If one of those
   exit points is hit, we mourn the inferior and clean up after it.
   Cleaning up is very important, even if the process exits normally,
   because otherwise we might leave behind traces of previous
   execution, and in several cases GDB itself might be left hosed,
   because all the exception handlers were not restored.

   Swapping of the standard handles (in redir_to_child and
   redir_to_debugger) is needed because, since both GDB and the
   debuggee live in the same process, as far as the OS is concerned,
   the share the same file table.  This means that the standard
   handles 0, 1, and 2 point to the same file table entries, and thus
   are connected to the same devices.  Therefore, if the debugger
   redirects its standard output, the standard output of the debuggee
   is also automagically redirected to the same file/device!
   Similarly, if the debuggee redirects its stdout to a file, you
   won't be able to see debugger's output (it will go to the same file
   where the debuggee has its output); and if the debuggee closes its
   standard input, you will lose the ability to talk to debugger!

   For this reason, every time the debuggee is about to be resumed, we
   call redir_to_child, which redirects the standard handles to where
   the debuggee expects them to be.  When the debuggee stops and GDB
   regains control, we call redir_to_debugger, which redirects those 3
   handles back to where GDB expects.

   Note that only the first 3 handles are swapped, so if the debuggee
   redirects or closes any other handles, GDB will not notice.  In
   particular, the exit code of a DJGPP program forcibly closes all
   file handles beyond the first 3 ones, so when the debuggee exits,
   GDB currently loses its stdaux and stdprn streams.  Fortunately,
   GDB does not use those as of this writing, and will never need
   to.  */

#include "defs.h"

#include <fcntl.h>

#include "x86-nat.h"
#include "inferior.h"
#include "infrun.h"
#include "gdbthread.h"
#include "gdbsupport/gdb_wait.h"
#include "gdbcore.h"
#include "command.h"
#include "gdbcmd.h"
#include "floatformat.h"
#include "buildsym-legacy.h"
#include "i387-tdep.h"
#include "i386-tdep.h"
#include "nat/x86-cpuid.h"
#include "value.h"
#include "regcache.h"
#include "top.h"
#include "cli/cli-utils.h"
#include "inf-child.h"

#include <ctype.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <io.h>
#include <dos.h>
#include <dpmi.h>
#include <go32.h>
#include <sys/farptr.h>
#include <debug/v2load.h>
#include <debug/dbgcom.h>
#if __DJGPP_MINOR__ > 2
#include <debug/redir.h>
#endif

#include <langinfo.h>

#if __DJGPP_MINOR__ < 3
/* This code will be provided from DJGPP 2.03 on.  Until then I code it
   here.  */
typedef struct
  {
    unsigned short sig0;
    unsigned short sig1;
    unsigned short sig2;
    unsigned short sig3;
    unsigned short exponent:15;
    unsigned short sign:1;
  }
NPXREG;

typedef struct
  {
    unsigned int control;
    unsigned int status;
    unsigned int tag;
    unsigned int eip;
    unsigned int cs;
    unsigned int dataptr;
    unsigned int datasel;
    NPXREG reg[8];
  }
NPX;

static NPX npx;

static void save_npx (void);	/* Save the FPU of the debugged program.  */
static void load_npx (void);	/* Restore the FPU of the debugged program.  */

/* ------------------------------------------------------------------------- */
/* Store the contents of the NPX in the global variable `npx'.  */

static void
save_npx (void)
{
  asm ("inb    $0xa0, %%al  \n\
       testb $0x20, %%al    \n\
       jz 1f 	    	    \n\
       xorb %%al, %%al	    \n\
       outb %%al, $0xf0     \n\
       movb $0x20, %%al	    \n\
       outb %%al, $0xa0     \n\
       outb %%al, $0x20     \n\
1:     	       	   	    \n\
       fnsave %0	    \n\
       fwait "
:     "=m" (npx)
:				/* No input */
:     "%eax");
}



/* ------------------------------------------------------------------------- */
/* Reload the contents of the NPX from the global variable `npx'.  */

static void
load_npx (void)
{
  asm ("frstor %0":"=m" (npx));
}
/* ------------------------------------------------------------------------- */
/* Stubs for the missing redirection functions.  */
typedef struct {
  char *command;
  int redirected;
} cmdline_t;

void
redir_cmdline_delete (cmdline_t *ptr)
{
  ptr->redirected = 0;
}

int
redir_cmdline_parse (const char *args, cmdline_t *ptr)
{
  return -1;
}

int
redir_to_child (cmdline_t *ptr)
{
  return 1;
}

int
redir_to_debugger (cmdline_t *ptr)
{
  return 1;
}

int
redir_debug_init (cmdline_t *ptr)
{
  return 0;
}
#endif /* __DJGPP_MINOR < 3 */

typedef enum { wp_insert, wp_remove, wp_count } wp_op;

/* This holds the current reference counts for each debug register.  */
static int dr_ref_count[4];

#define SOME_PID 42

static int prog_has_started = 0;

#define r_ofs(x) (offsetof(TSS,x))

static struct
{
  size_t tss_ofs;
  size_t size;
}
regno_mapping[] =
{
  {r_ofs (tss_eax), 4},	/* normal registers, from a_tss */
  {r_ofs (tss_ecx), 4},
  {r_ofs (tss_edx), 4},
  {r_ofs (tss_ebx), 4},
  {r_ofs (tss_esp), 4},
  {r_ofs (tss_ebp), 4},
  {r_ofs (tss_esi), 4},
  {r_ofs (tss_edi), 4},
  {r_ofs (tss_eip), 4},
  {r_ofs (tss_eflags), 4},
  {r_ofs (tss_cs), 2},
  {r_ofs (tss_ss), 2},
  {r_ofs (tss_ds), 2},
  {r_ofs (tss_es), 2},
  {r_ofs (tss_fs), 2},
  {r_ofs (tss_gs), 2},
  {0, 10},		/* 8 FP registers, from npx.reg[] */
  {1, 10},
  {2, 10},
  {3, 10},
  {4, 10},
  {5, 10},
  {6, 10},
  {7, 10},
	/* The order of the next 7 registers must be consistent
	   with their numbering in config/i386/tm-i386.h, which see.  */
  {0, 2},		/* control word, from npx */
  {4, 2},		/* status word, from npx */
  {8, 2},		/* tag word, from npx */
  {16, 2},		/* last FP exception CS from npx */
  {12, 4},		/* last FP exception EIP from npx */
  {24, 2},		/* last FP exception operand selector from npx */
  {20, 4},		/* last FP exception operand offset from npx */
  {18, 2}		/* last FP opcode from npx */
};

static struct
  {
    int go32_sig;
    enum gdb_signal gdb_sig;
  }
sig_map[] =
{
  {0, GDB_SIGNAL_FPE},
  {1, GDB_SIGNAL_TRAP},
  /* Exception 2 is triggered by the NMI.  DJGPP handles it as SIGILL,
     but I think SIGBUS is better, since the NMI is usually activated
     as a result of a memory parity check failure.  */
  {2, GDB_SIGNAL_BUS},
  {3, GDB_SIGNAL_TRAP},
  {4, GDB_SIGNAL_FPE},
  {5, GDB_SIGNAL_SEGV},
  {6, GDB_SIGNAL_ILL},
  {7, GDB_SIGNAL_EMT},	/* no-coprocessor exception */
  {8, GDB_SIGNAL_SEGV},
  {9, GDB_SIGNAL_SEGV},
  {10, GDB_SIGNAL_BUS},
  {11, GDB_SIGNAL_SEGV},
  {12, GDB_SIGNAL_SEGV},
  {13, GDB_SIGNAL_SEGV},
  {14, GDB_SIGNAL_SEGV},
  {16, GDB_SIGNAL_FPE},
  {17, GDB_SIGNAL_BUS},
  {31, GDB_SIGNAL_ILL},
  {0x1b, GDB_SIGNAL_INT},
  {0x75, GDB_SIGNAL_FPE},
  {0x78, GDB_SIGNAL_ALRM},
  {0x79, GDB_SIGNAL_INT},
  {0x7a, GDB_SIGNAL_QUIT},
  {-1, GDB_SIGNAL_LAST}
};

static struct {
  enum gdb_signal gdb_sig;
  int djgpp_excepno;
} excepn_map[] = {
  {GDB_SIGNAL_0, -1},
  {GDB_SIGNAL_ILL, 6},	/* Invalid Opcode */
  {GDB_SIGNAL_EMT, 7},	/* triggers SIGNOFP */
  {GDB_SIGNAL_SEGV, 13},	/* GPF */
  {GDB_SIGNAL_BUS, 17},	/* Alignment Check */
  /* The rest are fake exceptions, see dpmiexcp.c in djlsr*.zip for
     details.  */
  {GDB_SIGNAL_TERM, 0x1b},	/* triggers Ctrl-Break type of SIGINT */
  {GDB_SIGNAL_FPE, 0x75},
  {GDB_SIGNAL_INT, 0x79},
  {GDB_SIGNAL_QUIT, 0x7a},
  {GDB_SIGNAL_ALRM, 0x78},	/* triggers SIGTIMR */
  {GDB_SIGNAL_PROF, 0x78},
  {GDB_SIGNAL_LAST, -1}
};

/* The go32 target.  */

struct go32_nat_target final : public x86_nat_target<inf_child_target>
{
  void attach (const char *, int) override;

  void resume (ptid_t, int, enum gdb_signal) override;

  ptid_t wait (ptid_t, struct target_waitstatus *, target_wait_flags) override;

  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;

  enum target_xfer_status xfer_partial (enum target_object object,
					const char *annex,
					gdb_byte *readbuf,
					const gdb_byte *writebuf,
					ULONGEST offset, ULONGEST len,
					ULONGEST *xfered_len) override;

  void files_info () override;

  void terminal_init () override;

  void terminal_inferior () override;

  void terminal_ours_for_output () override;

  void terminal_ours () override;

  void terminal_info (const char *, int) override;

  void pass_ctrlc () override;

  void kill () override;

  void create_inferior (const char *, const std::string &,
			char **, int) override;

  void mourn_inferior () override;

  bool thread_alive (ptid_t ptid) override;

  std::string pid_to_str (ptid_t) override;
};

static go32_nat_target the_go32_nat_target;

void
go32_nat_target::attach (const char *args, int from_tty)
{
  error (_("\
You cannot attach to a running program on this platform.\n\
Use the `run' command to run DJGPP programs."));
}

static int resume_is_step;
static int resume_signal = -1;

void
go32_nat_target::resume (ptid_t ptid, int step, enum gdb_signal siggnal)
{
  int i;

  resume_is_step = step;

  if (siggnal != GDB_SIGNAL_0 && siggnal != GDB_SIGNAL_TRAP)
    {
      for (i = 0, resume_signal = -1;
	   excepn_map[i].gdb_sig != GDB_SIGNAL_LAST; i++)
	if (excepn_map[i].gdb_sig == siggnal)
	  {
	    resume_signal = excepn_map[i].djgpp_excepno;
	    break;
	  }
      if (resume_signal == -1)
	printf_unfiltered ("Cannot deliver signal %s on this platform.\n",
			   gdb_signal_to_name (siggnal));
    }
}

static char child_cwd[FILENAME_MAX];

ptid_t
go32_nat_target::wait (ptid_t ptid, struct target_waitstatus *status,
		       target_wait_flags options)
{
  int i;
  unsigned char saved_opcode;
  unsigned long INT3_addr = 0;
  int stepping_over_INT = 0;

  a_tss.tss_eflags &= 0xfeff;	/* Reset the single-step flag (TF).  */
  if (resume_is_step)
    {
      /* If the next instruction is INT xx or INTO, we need to handle
	 them specially.  Intel manuals say that these instructions
	 reset the single-step flag (a.k.a. TF).  However, it seems
	 that, at least in the DPMI environment, and at least when
	 stepping over the DPMI interrupt 31h, the problem is having
	 TF set at all when INT 31h is executed: the debuggee either
	 crashes (and takes the system with it) or is killed by a
	 SIGTRAP.

	 So we need to emulate single-step mode: we put an INT3 opcode
	 right after the INT xx instruction, let the debuggee run
	 until it hits INT3 and stops, then restore the original
	 instruction which we overwrote with the INT3 opcode, and back
	 up the debuggee's EIP to that instruction.  */
      read_child (a_tss.tss_eip, &saved_opcode, 1);
      if (saved_opcode == 0xCD || saved_opcode == 0xCE)
	{
	  unsigned char INT3_opcode = 0xCC;

	  INT3_addr
	    = saved_opcode == 0xCD ? a_tss.tss_eip + 2 : a_tss.tss_eip + 1;
	  stepping_over_INT = 1;
	  read_child (INT3_addr, &saved_opcode, 1);
	  write_child (INT3_addr, &INT3_opcode, 1);
	}
      else
	a_tss.tss_eflags |= 0x0100; /* normal instruction: set TF */
    }

  /* The special value FFFFh in tss_trap indicates to run_child that
     tss_irqn holds a signal to be delivered to the debuggee.  */
  if (resume_signal <= -1)
    {
      a_tss.tss_trap = 0;
      a_tss.tss_irqn = 0xff;
    }
  else
    {
      a_tss.tss_trap = 0xffff;	/* run_child looks for this.  */
      a_tss.tss_irqn = resume_signal;
    }

  /* The child might change working directory behind our back.  The
     GDB users won't like the side effects of that when they work with
     relative file names, and GDB might be confused by its current
     directory not being in sync with the truth.  So we always make a
     point of changing back to where GDB thinks is its cwd, when we
     return control to the debugger, but restore child's cwd before we
     run it.  */
  /* Initialize child_cwd, before the first call to run_child and not
     in the initialization, so the child get also the changed directory
     set with the gdb-command "cd ..."  */
  if (!*child_cwd)
    /* Initialize child's cwd with the current one.  */
    getcwd (child_cwd, sizeof (child_cwd));

  chdir (child_cwd);

#if __DJGPP_MINOR__ < 3
  load_npx ();
#endif
  run_child ();
#if __DJGPP_MINOR__ < 3
  save_npx ();
#endif

  /* Did we step over an INT xx instruction?  */
  if (stepping_over_INT && a_tss.tss_eip == INT3_addr + 1)
    {
      /* Restore the original opcode.  */
      a_tss.tss_eip--;	/* EIP points *after* the INT3 instruction.  */
      write_child (a_tss.tss_eip, &saved_opcode, 1);
      /* Simulate a TRAP exception.  */
      a_tss.tss_irqn = 1;
      a_tss.tss_eflags |= 0x0100;
    }

  getcwd (child_cwd, sizeof (child_cwd)); /* in case it has changed */
  if (current_directory != NULL)
    chdir (current_directory);

  if (a_tss.tss_irqn == 0x21)
    status->set_exited (a_tss.tss_eax & 0xff);
  else
    {
      status->set_stopped (GDB_SIGNAL_UNKNOWN);
      for (i = 0; sig_map[i].go32_sig != -1; i++)
	{
	  if (a_tss.tss_irqn == sig_map[i].go32_sig)
	    {
#if __DJGPP_MINOR__ < 3
	      status->set_stopped (sig_map[i].gdb_sig);
	      if (status->sig () != GDB_SIGNAL_TRAP)
		status->set_signalled (status->sig ());
#else
	      status->set_stopped (sig_map[i].gdb_sig);
#endif
	      break;
	    }
	}
    }
  return ptid_t (SOME_PID);
}

static void
fetch_register (struct regcache *regcache, int regno)
{
  struct gdbarch *gdbarch = regcache->arch ();
  if (regno < gdbarch_fp0_regnum (gdbarch))
    regcache->raw_supply (regno,
			  (char *) &a_tss + regno_mapping[regno].tss_ofs);
  else if (i386_fp_regnum_p (gdbarch, regno) || i386_fpc_regnum_p (gdbarch,
								   regno))
    i387_supply_fsave (regcache, regno, &npx);
  else
    internal_error (_("Invalid register no. %d in fetch_register."), regno);
}

void
go32_nat_target::fetch_registers (struct regcache *regcache, int regno)
{
  if (regno >= 0)
    fetch_register (regcache, regno);
  else
    {
      for (regno = 0;
	   regno < gdbarch_fp0_regnum (regcache->arch ());
	   regno++)
	fetch_register (regcache, regno);
      i387_supply_fsave (regcache, -1, &npx);
    }
}

static void
store_register (const struct regcache *regcache, int regno)
{
  struct gdbarch *gdbarch = regcache->arch ();
  if (regno < gdbarch_fp0_regnum (gdbarch))
    regcache->raw_collect (regno,
			   (char *) &a_tss + regno_mapping[regno].tss_ofs);
  else if (i386_fp_regnum_p (gdbarch, regno) || i386_fpc_regnum_p (gdbarch,
								   regno))
    i387_collect_fsave (regcache, regno, &npx);
  else
    internal_error (_("Invalid register no. %d in store_register."), regno);
}

void
go32_nat_target::store_registers (struct regcache *regcache, int regno)
{
  unsigned r;

  if (regno >= 0)
    store_register (regcache, regno);
  else
    {
      for (r = 0; r < gdbarch_fp0_regnum (regcache->arch ()); r++)
	store_register (regcache, r);
      i387_collect_fsave (regcache, -1, &npx);
    }
}

/* Const-correct version of DJGPP's write_child, which unfortunately
   takes a non-const buffer pointer.  */

static int
my_write_child (unsigned child_addr, const void *buf, unsigned len)
{
  static void *buffer = NULL;
  static unsigned buffer_len = 0;
  int res;

  if (buffer_len < len)
    {
      buffer = xrealloc (buffer, len);
      buffer_len = len;
    }

  memcpy (buffer, buf, len);
  res = write_child (child_addr, buffer, len);
  return res;
}

/* Helper for go32_xfer_partial that handles memory transfers.
   Arguments are like target_xfer_partial.  */

static enum target_xfer_status
go32_xfer_memory (gdb_byte *readbuf, const gdb_byte *writebuf,
		  ULONGEST memaddr, ULONGEST len, ULONGEST *xfered_len)
{
  int res;

  if (writebuf != NULL)
    res = my_write_child (memaddr, writebuf, len);
  else
    res = read_child (memaddr, readbuf, len);

  /* read_child and write_child return zero on success, non-zero on
     failure.  */
  if (res != 0)
    return TARGET_XFER_E_IO;

  *xfered_len = len;
  return TARGET_XFER_OK;
}

/* Target to_xfer_partial implementation.  */

enum target_xfer_status
go32_nat_target::xfer_partial (enum target_object object,
			       const char *annex, gdb_byte *readbuf,
			       const gdb_byte *writebuf, ULONGEST offset,
			       ULONGEST len,
			       ULONGEST *xfered_len)
{
  switch (object)
    {
    case TARGET_OBJECT_MEMORY:
      return go32_xfer_memory (readbuf, writebuf, offset, len, xfered_len);

    default:
      return this->beneath ()->xfer_partial (object, annex,
					     readbuf, writebuf, offset, len,
					     xfered_len);
    }
}

static cmdline_t child_cmd;	/* Parsed child's command line kept here.  */

void
go32_nat_target::files_info ()
{
  gdb_printf ("You are running a DJGPP V2 program.\n");
}

void
go32_nat_target::kill_inferior ()
{
  mourn_inferior ();
}

void
go32_nat_target::create_inferior (const char *exec_file,
				  const std::string &allargs,
				  char **env, int from_tty)
{
  extern char **environ;
  jmp_buf start_state;
  char *cmdline;
  char **env_save = environ;
  size_t cmdlen;
  struct inferior *inf;
  int result;
  const char *args = allargs.c_str ();

  /* If no exec file handed to us, get it from the exec-file command -- with
     a good, common error message if none is specified.  */
  if (exec_file == 0)
    exec_file = get_exec_file (1);

  resume_signal = -1;
  resume_is_step = 0;

  /* Initialize child's cwd as empty to be initialized when starting
     the child.  */
  *child_cwd = 0;

  /* Init command line storage.  */
  if (redir_debug_init (&child_cmd) == -1)
    internal_error (_("Cannot allocate redirection storage: "
		      "not enough memory.\n"));

  /* Parse the command line and create redirections.  */
  if (strpbrk (args, "<>"))
    {
      if (redir_cmdline_parse (args, &child_cmd) == 0)
	args = child_cmd.command;
      else
	error (_("Syntax error in command line."));
    }
  else
    child_cmd.command = xstrdup (args);

  cmdlen = strlen (args);
  /* v2loadimage passes command lines via DOS memory, so it cannot
     possibly handle commands longer than 1MB.  */
  if (cmdlen > 1024*1024)
    error (_("Command line too long."));

  cmdline = (char *) xmalloc (cmdlen + 4);
  strcpy (cmdline + 1, args);
  /* If the command-line length fits into DOS 126-char limits, use the
     DOS command tail format; otherwise, tell v2loadimage to pass it
     through a buffer in conventional memory.  */
  if (cmdlen < 127)
    {
      cmdline[0] = strlen (args);
      cmdline[cmdlen + 1] = 13;
    }
  else
    cmdline[0] = 0xff;	/* Signal v2loadimage it's a long command.  */

  environ = env;

  result = v2loadimage (exec_file, cmdline, start_state);

  environ = env_save;
  xfree (cmdline);

  if (result != 0)
    error (_("Load failed for image %s"), exec_file);

  edi_init (start_state);
#if __DJGPP_MINOR__ < 3
  save_npx ();
#endif

  inf = current_inferior ();
  inferior_appeared (inf, SOME_PID);

  if (!inf->target_is_pushed (this))
    inf->push_target (this);

  thread_info *thr = add_thread_silent (ptid_t (SOME_PID));
  switch_to_thread (thr);

  clear_proceed_status (0);
  insert_breakpoints ();
  prog_has_started = 1;
}

void
go32_nat_target::mourn_inferior ()
{
  redir_cmdline_delete (&child_cmd);
  resume_signal = -1;
  resume_is_step = 0;

  cleanup_client ();

  /* We need to make sure all the breakpoint enable bits in the DR7
     register are reset when the inferior exits.  Otherwise, if they
     rerun the inferior, the uncleared bits may cause random SIGTRAPs,
     failure to set more watchpoints, and other calamities.  It would
     be nice if GDB itself would take care to remove all breakpoints
     at all times, but it doesn't, probably under an assumption that
     the OS cleans up when the debuggee exits.  */
  x86_cleanup_dregs ();

  prog_has_started = 0;

  generic_mourn_inferior ();
  maybe_unpush_target ();
}

/* Hardware watchpoint support.  */

#define D_REGS edi.dr
#define CONTROL D_REGS[7]
#define STATUS D_REGS[6]

/* Pass the address ADDR to the inferior in the I'th debug register.
   Here we just store the address in D_REGS, the watchpoint will be
   actually set up when go32_wait runs the debuggee.  */
static void
go32_set_dr (int i, CORE_ADDR addr)
{
  if (i < 0 || i > 3)
    internal_error (_("Invalid register %d in go32_set_dr.\n"), i);
  D_REGS[i] = addr;
}

/* Pass the value VAL to the inferior in the DR7 debug control
   register.  Here we just store the address in D_REGS, the watchpoint
   will be actually set up when go32_wait runs the debuggee.  */
static void
go32_set_dr7 (unsigned long val)
{
  CONTROL = val;
}

/* Get the value of the DR6 debug status register from the inferior.
   Here we just return the value stored in D_REGS, as we've got it
   from the last go32_wait call.  */
static unsigned long
go32_get_dr6 (void)
{
  return STATUS;
}

/* Get the value of the DR7 debug status register from the inferior.
   Here we just return the value stored in D_REGS, as we've got it
   from the last go32_wait call.  */

static unsigned long
go32_get_dr7 (void)
{
  return CONTROL;
}

/* Get the value of the DR debug register I from the inferior.  Here
   we just return the value stored in D_REGS, as we've got it from the
   last go32_wait call.  */

static CORE_ADDR
go32_get_dr (int i)
{
  if (i < 0 || i > 3)
    internal_error (_("Invalid register %d in go32_get_dr.\n"), i);
  return D_REGS[i];
}

/* Put the device open on handle FD into either raw or cooked
   mode, return 1 if it was in raw mode, zero otherwise.  */

static int
device_mode (int fd, int raw_p)
{
  int oldmode, newmode;
  __dpmi_regs regs;

  regs.x.ax = 0x4400;
  regs.x.bx = fd;
  __dpmi_int (0x21, &regs);
  if (regs.x.flags & 1)
    return -1;
  newmode = oldmode = regs.x.dx;

  if (raw_p)
    newmode |= 0x20;
  else
    newmode &= ~0x20;

  if (oldmode & 0x80)	/* Only for character dev.  */
    {
      regs.x.ax = 0x4401;
      regs.x.bx = fd;
      regs.x.dx = newmode & 0xff;   /* Force upper byte zero, else it fails.  */
      __dpmi_int (0x21, &regs);
      if (regs.x.flags & 1)
	return -1;
    }
  return (oldmode & 0x20) == 0x20;
}


static int inf_mode_valid = 0;
static int inf_terminal_mode;

/* This semaphore is needed because, amazingly enough, GDB calls
   target.to_terminal_ours more than once after the inferior stops.
   But we need the information from the first call only, since the
   second call will always see GDB's own cooked terminal.  */
static int terminal_is_ours = 1;

void
go32_nat_target::terminal_init ()
{
  inf_mode_valid = 0;	/* Reinitialize, in case they are restarting child.  */
  terminal_is_ours = 1;
}

void
go32_nat_target::terminal_info (const char *args, int from_tty)
{
  gdb_printf ("Inferior's terminal is in %s mode.\n",
	      !inf_mode_valid
	      ? "default" : inf_terminal_mode ? "raw" : "cooked");

#if __DJGPP_MINOR__ > 2
  if (child_cmd.redirection)
    {
      int i;

      for (i = 0; i < DBG_HANDLES; i++)
	{
	  if (child_cmd.redirection[i]->file_name)
	    gdb_printf ("\tFile handle %d is redirected to `%s'.\n",
			i, child_cmd.redirection[i]->file_name);
	  else if (_get_dev_info (child_cmd.redirection[i]->inf_handle) == -1)
	    gdb_printf
	      ("\tFile handle %d appears to be closed by inferior.\n", i);
	  /* Mask off the raw/cooked bit when comparing device info words.  */
	  else if ((_get_dev_info (child_cmd.redirection[i]->inf_handle) & 0xdf)
		   != (_get_dev_info (i) & 0xdf))
	    gdb_printf
	      ("\tFile handle %d appears to be redirected by inferior.\n", i);
	}
    }
#endif
}

void
go32_nat_target::terminal_inferior ()
{
  /* Redirect standard handles as child wants them.  */
  errno = 0;
  if (redir_to_child (&child_cmd) == -1)
    {
      redir_to_debugger (&child_cmd);
      error (_("Cannot redirect standard handles for program: %s."),
	     safe_strerror (errno));
    }
  /* Set the console device of the inferior to whatever mode
     (raw or cooked) we found it last time.  */
  if (terminal_is_ours)
    {
      if (inf_mode_valid)
	device_mode (0, inf_terminal_mode);
      terminal_is_ours = 0;
    }
}

void
go32_nat_target::terminal_ours ()
{
  /* Switch to cooked mode on the gdb terminal and save the inferior
     terminal mode to be restored when it is resumed.  */
  if (!terminal_is_ours)
    {
      inf_terminal_mode = device_mode (0, 0);
      if (inf_terminal_mode != -1)
	inf_mode_valid = 1;
      else
	/* If device_mode returned -1, we don't know what happens with
	   handle 0 anymore, so make the info invalid.  */
	inf_mode_valid = 0;
      terminal_is_ours = 1;

      /* Restore debugger's standard handles.  */
      errno = 0;
      if (redir_to_debugger (&child_cmd) == -1)
	{
	  redir_to_child (&child_cmd);
	  error (_("Cannot redirect standard handles for debugger: %s."),
		 safe_strerror (errno));
	}
    }
}

void
go32_nat_target::pass_ctrlc ()
{
}

bool
go32_nat_target::thread_alive (ptid_t ptid)
{
  return ptid != null_ptid;
}

std::string
go32_nat_target::pid_to_str (ptid_t ptid)
{
  return normal_pid_to_str (ptid);
}

/* Return the current DOS codepage number.  */
static int
dos_codepage (void)
{
  __dpmi_regs regs;

  regs.x.ax = 0x6601;
  __dpmi_int (0x21, &regs);
  if (!(regs.x.flags & 1))
    return regs.x.bx & 0xffff;
  else
    return 437;	/* default */
}

/* Limited emulation of `nl_langinfo', for charset.c.  */
char *
nl_langinfo (nl_item item)
{
  char *retval;

  switch (item)
    {
      case CODESET:
	{
	  /* 8 is enough for SHORT_MAX + "CP" + null.  */
	  char buf[8];
	  int blen = sizeof (buf);
	  int needed = snprintf (buf, blen, "CP%d", dos_codepage ());

	  if (needed > blen)	/* Should never happen.  */
	    buf[0] = 0;
	  retval = xstrdup (buf);
	}
	break;
      default:
	retval = xstrdup ("");
	break;
    }
  return retval;
}

unsigned short windows_major, windows_minor;

/* Compute the version Windows reports via Int 2Fh/AX=1600h.  */
static void
go32_get_windows_version(void)
{
  __dpmi_regs r;

  r.x.ax = 0x1600;
  __dpmi_int(0x2f, &r);
  if (r.h.al > 2 && r.h.al != 0x80 && r.h.al != 0xff
      && (r.h.al > 3 || r.h.ah > 0))
    {
      windows_major = r.h.al;
      windows_minor = r.h.ah;
    }
  else
    windows_major = 0xff;	/* meaning no Windows */
}

/* A subroutine of go32_sysinfo to display memory info.  */
static void
print_mem (unsigned long datum, const char *header, int in_pages_p)
{
  if (datum != 0xffffffffUL)
    {
      if (in_pages_p)
	datum <<= 12;
      gdb_puts (header);
      if (datum > 1024)
	{
	  gdb_printf ("%lu KB", datum >> 10);
	  if (datum > 1024 * 1024)
	    gdb_printf (" (%lu MB)", datum >> 20);
	}
      else
	gdb_printf ("%lu Bytes", datum);
      gdb_puts ("\n");
    }
}

/* Display assorted information about the underlying OS.  */
static void
go32_sysinfo (const char *arg, int from_tty)
{
  static const char test_pattern[] =
    "deadbeafdeadbeafdeadbeafdeadbeafdeadbeaf"
    "deadbeafdeadbeafdeadbeafdeadbeafdeadbeaf"
    "deadbeafdeadbeafdeadbeafdeadbeafdeadbeafdeadbeaf";
  struct utsname u;
  char cpuid_vendor[13];
  unsigned cpuid_max = 0, cpuid_eax, cpuid_ebx, cpuid_ecx, cpuid_edx;
  unsigned true_dos_version = _get_dos_version (1);
  unsigned advertized_dos_version = ((unsigned int)_osmajor << 8) | _osminor;
  int dpmi_flags;
  char dpmi_vendor_info[129];
  int dpmi_vendor_available;
  __dpmi_version_ret dpmi_version_data;
  long eflags;
  __dpmi_free_mem_info mem_info;
  __dpmi_regs regs;

  cpuid_vendor[0] = '\0';
  if (uname (&u))
    strcpy (u.machine, "Unknown x86");
  else if (u.machine[0] == 'i' && u.machine[1] > 4)
    {
      /* CPUID with EAX = 0 returns the Vendor ID.  */
#if 0
      /* Ideally we would use x86_cpuid(), but it needs someone to run
	 native tests first to make sure things actually work.  They should.
	 http://sourceware.org/ml/gdb-patches/2013-05/msg00164.html  */
      unsigned int eax, ebx, ecx, edx;

      if (x86_cpuid (0, &eax, &ebx, &ecx, &edx))
	{
	  cpuid_max = eax;
	  memcpy (&vendor[0], &ebx, 4);
	  memcpy (&vendor[4], &ecx, 4);
	  memcpy (&vendor[8], &edx, 4);
	  cpuid_vendor[12] = '\0';
	}
#else
      __asm__ __volatile__ ("xorl   %%ebx, %%ebx;"
			    "xorl   %%ecx, %%ecx;"
			    "xorl   %%edx, %%edx;"
			    "movl   $0,    %%eax;"
			    "cpuid;"
			    "movl   %%ebx,  %0;"
			    "movl   %%edx,  %1;"
			    "movl   %%ecx,  %2;"
			    "movl   %%eax,  %3;"
			    : "=m" (cpuid_vendor[0]),
			      "=m" (cpuid_vendor[4]),
			      "=m" (cpuid_vendor[8]),
			      "=m" (cpuid_max)
			    :
			    : "%eax", "%ebx", "%ecx", "%edx");
      cpuid_vendor[12] = '\0';
#endif
    }

  gdb_printf ("CPU Type.......................%s", u.machine);
  if (cpuid_vendor[0])
    gdb_printf (" (%s)", cpuid_vendor);
  gdb_puts ("\n");

  /* CPUID with EAX = 1 returns processor signature and features.  */
  if (cpuid_max >= 1)
    {
      static const char *brand_name[] = {
	"",
	" Celeron",
	" III",
	" III Xeon",
	"", "", "", "",
	" 4"
      };
      char cpu_string[80];
      char cpu_brand[20];
      unsigned brand_idx;
      int intel_p = strcmp (cpuid_vendor, "GenuineIntel") == 0;
      int amd_p = strcmp (cpuid_vendor, "AuthenticAMD") == 0;
      int hygon_p = strcmp (cpuid_vendor, "HygonGenuine") == 0;
      unsigned cpu_family, cpu_model;

#if 0
      /* See comment above about cpuid usage.  */
      x86_cpuid (1, &cpuid_eax, &cpuid_ebx, NULL, &cpuid_edx);
#else
      __asm__ __volatile__ ("movl   $1, %%eax;"
			    "cpuid;"
			    : "=a" (cpuid_eax),
			      "=b" (cpuid_ebx),
			      "=d" (cpuid_edx)
			    :
			    : "%ecx");
#endif
      brand_idx = cpuid_ebx & 0xff;
      cpu_family = (cpuid_eax >> 8) & 0xf;
      cpu_model  = (cpuid_eax >> 4) & 0xf;
      cpu_brand[0] = '\0';
      if (intel_p)
	{
	  if (brand_idx > 0
	      && brand_idx < sizeof(brand_name)/sizeof(brand_name[0])
	      && *brand_name[brand_idx])
	    strcpy (cpu_brand, brand_name[brand_idx]);
	  else if (cpu_family == 5)
	    {
	      if (((cpuid_eax >> 12) & 3) == 0 && cpu_model == 4)
		strcpy (cpu_brand, " MMX");
	      else if (cpu_model > 1 && ((cpuid_eax >> 12) & 3) == 1)
		strcpy (cpu_brand, " OverDrive");
	      else if (cpu_model > 1 && ((cpuid_eax >> 12) & 3) == 2)
		strcpy (cpu_brand, " Dual");
	    }
	  else if (cpu_family == 6 && cpu_model < 8)
	    {
	      switch (cpu_model)
		{
		  case 1:
		    strcpy (cpu_brand, " Pro");
		    break;
		  case 3:
		    strcpy (cpu_brand, " II");
		    break;
		  case 5:
		    strcpy (cpu_brand, " II Xeon");
		    break;
		  case 6:
		    strcpy (cpu_brand, " Celeron");
		    break;
		  case 7:
		    strcpy (cpu_brand, " III");
		    break;
		}
	    }
	}
      else if (amd_p)
	{
	  switch (cpu_family)
	    {
	      case 4:
		strcpy (cpu_brand, "486/5x86");
		break;
	      case 5:
		switch (cpu_model)
		  {
		    case 0:
		    case 1:
		    case 2:
		    case 3:
		      strcpy (cpu_brand, "-K5");
		      break;
		    case 6:
		    case 7:
		      strcpy (cpu_brand, "-K6");
		      break;
		    case 8:
		      strcpy (cpu_brand, "-K6-2");
		      break;
		    case 9:
		      strcpy (cpu_brand, "-K6-III");
		      break;
		  }
		break;
	      case 6:
		switch (cpu_model)
		  {
		    case 1:
		    case 2:
		    case 4:
		      strcpy (cpu_brand, " Athlon");
		      break;
		    case 3:
		      strcpy (cpu_brand, " Duron");
		      break;
		  }
		break;
	    }
	}
      xsnprintf (cpu_string, sizeof (cpu_string), "%s%s Model %d Stepping %d",
		 intel_p ? "Pentium" : (amd_p ? "AMD" : (hygon_p ? "Hygon" : "ix86")),
		 cpu_brand, cpu_model, cpuid_eax & 0xf);
      gdb_printf ("%*s%s\n", 31, "", cpu_string);
      if (((cpuid_edx & (6 | (0x0d << 23))) != 0)
	  || ((cpuid_edx & 1) == 0)
	  || ((amd_p || hygon_p) && (cpuid_edx & (3 << 30)) != 0))
	{
	  gdb_puts ("CPU Features...................");
	  /* We only list features which might be useful in the DPMI
	     environment.  */
	  if ((cpuid_edx & 1) == 0)
	    gdb_puts ("No FPU "); /* It's unusual to not have an FPU.  */
	  if ((cpuid_edx & (1 << 1)) != 0)
	    gdb_puts ("VME ");
	  if ((cpuid_edx & (1 << 2)) != 0)
	    gdb_puts ("DE ");
	  if ((cpuid_edx & (1 << 4)) != 0)
	    gdb_puts ("TSC ");
	  if ((cpuid_edx & (1 << 23)) != 0)
	    gdb_puts ("MMX ");
	  if ((cpuid_edx & (1 << 25)) != 0)
	    gdb_puts ("SSE ");
	  if ((cpuid_edx & (1 << 26)) != 0)
	    gdb_puts ("SSE2 ");
	  if (amd_p || hygon_p)
	    {
	      if ((cpuid_edx & (1 << 31)) != 0)
		gdb_puts ("3DNow! ");
	      if ((cpuid_edx & (1 << 30)) != 0)
		gdb_puts ("3DNow!Ext");
	    }
	  gdb_puts ("\n");
	}
    }
  gdb_puts ("\n");
  gdb_printf ("DOS Version....................%s %s.%s",
	      _os_flavor, u.release, u.version);
  if (true_dos_version != advertized_dos_version)
    gdb_printf (" (disguised as v%d.%d)", _osmajor, _osminor);
  gdb_puts ("\n");
  if (!windows_major)
    go32_get_windows_version ();
  if (windows_major != 0xff)
    {
      const char *windows_flavor;

      gdb_printf ("Windows Version................%d.%02d (Windows ",
		  windows_major, windows_minor);
      switch (windows_major)
	{
	  case 3:
	    windows_flavor = "3.X";
	    break;
	  case 4:
	    switch (windows_minor)
	      {
		case 0:
		  windows_flavor = "95, 95A, or 95B";
		  break;
		case 3:
		  windows_flavor = "95B OSR2.1 or 95C OSR2.5";
		  break;
		case 10:
		  windows_flavor = "98 or 98 SE";
		  break;
		case 90:
		  windows_flavor = "ME";
		  break;
		default:
		  windows_flavor = "9X";
		  break;
	      }
	    break;
	  default:
	    windows_flavor = "??";
	    break;
	}
      gdb_printf ("%s)\n", windows_flavor);
    }
  else if (true_dos_version == 0x532 && advertized_dos_version == 0x500)
    gdb_printf ("Windows Version................"
		"Windows NT family (W2K/XP/W2K3/Vista/W2K8)\n");
  gdb_puts ("\n");
  /* On some versions of Windows, __dpmi_get_capabilities returns
     zero, but the buffer is not filled with info, so we fill the
     buffer with a known pattern and test for it afterwards.  */
  memcpy (dpmi_vendor_info, test_pattern, sizeof(dpmi_vendor_info));
  dpmi_vendor_available =
    __dpmi_get_capabilities (&dpmi_flags, dpmi_vendor_info);
  if (dpmi_vendor_available == 0
      && memcmp (dpmi_vendor_info, test_pattern,
		 sizeof(dpmi_vendor_info)) != 0)
    {
      /* The DPMI spec says the vendor string should be ASCIIZ, but
	 I don't trust the vendors to follow that...  */
      if (!memchr (&dpmi_vendor_info[2], 0, 126))
	dpmi_vendor_info[128] = '\0';
      gdb_printf ("DPMI Host......................"
		  "%s v%d.%d (capabilities: %#x)\n",
		  &dpmi_vendor_info[2],
		  (unsigned)dpmi_vendor_info[0],
		  (unsigned)dpmi_vendor_info[1],
		  ((unsigned)dpmi_flags & 0x7f));
    }
  else
    gdb_printf ("DPMI Host......................(Info not available)\n");
  __dpmi_get_version (&dpmi_version_data);
  gdb_printf ("DPMI Version...................%d.%02d\n",
	      dpmi_version_data.major, dpmi_version_data.minor);
  gdb_printf ("DPMI Info......................"
	      "%s-bit DPMI, with%s Virtual Memory support\n",
	      (dpmi_version_data.flags & 1) ? "32" : "16",
	      (dpmi_version_data.flags & 4) ? "" : "out");
  gdb_printf ("%*sInterrupts reflected to %s mode\n", 31, "",
	      (dpmi_version_data.flags & 2) ? "V86" : "Real");
  gdb_printf ("%*sProcessor type: i%d86\n", 31, "",
	      dpmi_version_data.cpu);
  gdb_printf ("%*sPIC base interrupt: Master: %#x  Slave: %#x\n", 31, "",
	      dpmi_version_data.master_pic, dpmi_version_data.slave_pic);

  /* a_tss is only initialized when the debuggee is first run.  */
  if (prog_has_started)
    {
      __asm__ __volatile__ ("pushfl ; popl %0" : "=g" (eflags));
      gdb_printf ("Protection....................."
		  "Ring %d (in %s), with%s I/O protection\n",
		  a_tss.tss_cs & 3, (a_tss.tss_cs & 4) ? "LDT" : "GDT",
		  (a_tss.tss_cs & 3) > ((eflags >> 12) & 3) ? "" : "out");
    }
  gdb_puts ("\n");
  __dpmi_get_free_memory_information (&mem_info);
  print_mem (mem_info.total_number_of_physical_pages,
	     "DPMI Total Physical Memory.....", 1);
  print_mem (mem_info.total_number_of_free_pages,
	     "DPMI Free Physical Memory......", 1);
  print_mem (mem_info.size_of_paging_file_partition_in_pages,
	     "DPMI Swap Space................", 1);
  print_mem (mem_info.linear_address_space_size_in_pages,
	     "DPMI Total Linear Address Size.", 1);
  print_mem (mem_info.free_linear_address_space_in_pages,
	     "DPMI Free Linear Address Size..", 1);
  print_mem (mem_info.largest_available_free_block_in_bytes,
	     "DPMI Largest Free Memory Block.", 0);

  regs.h.ah = 0x48;
  regs.x.bx = 0xffff;
  __dpmi_int (0x21, &regs);
  print_mem (regs.x.bx << 4, "Free DOS Memory................", 0);
  regs.x.ax = 0x5800;
  __dpmi_int (0x21, &regs);
  if ((regs.x.flags & 1) == 0)
    {
      static const char *dos_hilo[] = {
	"Low", "", "", "", "High", "", "", "", "High, then Low"
      };
      static const char *dos_fit[] = {
	"First", "Best", "Last"
      };
      int hilo_idx = (regs.x.ax >> 4) & 0x0f;
      int fit_idx  = regs.x.ax & 0x0f;

      if (hilo_idx > 8)
	hilo_idx = 0;
      if (fit_idx > 2)
	fit_idx = 0;
      gdb_printf ("DOS Memory Allocation..........%s memory, %s fit\n",
		  dos_hilo[hilo_idx], dos_fit[fit_idx]);
      regs.x.ax = 0x5802;
      __dpmi_int (0x21, &regs);
      if ((regs.x.flags & 1) != 0)
	regs.h.al = 0;
      gdb_printf ("%*sUMBs %sin DOS memory chain\n", 31, "",
		  regs.h.al == 0 ? "not " : "");
    }
}

struct seg_descr {
  unsigned short limit0;
  unsigned short base0;
  unsigned char  base1;
  unsigned       stype:5;
  unsigned       dpl:2;
  unsigned       present:1;
  unsigned       limit1:4;
  unsigned       available:1;
  unsigned       dummy:1;
  unsigned       bit32:1;
  unsigned       page_granular:1;
  unsigned char  base2;
} __attribute__ ((packed));

struct gate_descr {
  unsigned short offset0;
  unsigned short selector;
  unsigned       param_count:5;
  unsigned       dummy:3;
  unsigned       stype:5;
  unsigned       dpl:2;
  unsigned       present:1;
  unsigned short offset1;
} __attribute__ ((packed));

/* Read LEN bytes starting at logical address ADDR, and put the result
   into DEST.  Return 1 if success, zero if not.  */
static int
read_memory_region (unsigned long addr, void *dest, size_t len)
{
  unsigned long dos_ds_limit = __dpmi_get_segment_limit (_dos_ds);
  int retval = 1;

  /* For the low memory, we can simply use _dos_ds.  */
  if (addr <= dos_ds_limit - len)
    dosmemget (addr, len, dest);
  else
    {
      /* For memory above 1MB we need to set up a special segment to
	 be able to access that memory.  */
      int sel = __dpmi_allocate_ldt_descriptors (1);

      if (sel <= 0)
	retval = 0;
      else
	{
	  int access_rights = __dpmi_get_descriptor_access_rights (sel);
	  size_t segment_limit = len - 1;

	  /* Make sure the crucial bits in the descriptor access
	     rights are set correctly.  Some DPMI providers might barf
	     if we set the segment limit to something that is not an
	     integral multiple of 4KB pages if the granularity bit is
	     not set to byte-granular, even though the DPMI spec says
	     it's the host's responsibility to set that bit correctly.  */
	  if (len > 1024 * 1024)
	    {
	      access_rights |= 0x8000;
	      /* Page-granular segments should have the low 12 bits of
		 the limit set.  */
	      segment_limit |= 0xfff;
	    }
	  else
	    access_rights &= ~0x8000;

	  if (__dpmi_set_segment_base_address (sel, addr) != -1
	      && __dpmi_set_descriptor_access_rights (sel, access_rights) != -1
	      && __dpmi_set_segment_limit (sel, segment_limit) != -1
	      /* W2K silently fails to set the segment limit, leaving
		 it at zero; this test avoids the resulting crash.  */
	      && __dpmi_get_segment_limit (sel) >= segment_limit)
	    movedata (sel, 0, _my_ds (), (unsigned)dest, len);
	  else
	    retval = 0;

	  __dpmi_free_ldt_descriptor (sel);
	}
    }
  return retval;
}

/* Get a segment descriptor stored at index IDX in the descriptor
   table whose base address is TABLE_BASE.  Return the descriptor
   type, or -1 if failure.  */
static int
get_descriptor (unsigned long table_base, int idx, void *descr)
{
  unsigned long addr = table_base + idx * 8; /* 8 bytes per entry */

  if (read_memory_region (addr, descr, 8))
    return (int)((struct seg_descr *)descr)->stype;
  return -1;
}

struct dtr_reg {
  unsigned short limit __attribute__((packed));
  unsigned long  base  __attribute__((packed));
};

/* Display a segment descriptor stored at index IDX in a descriptor
   table whose type is TYPE and whose base address is BASE_ADDR.  If
   FORCE is non-zero, display even invalid descriptors.  */
static void
display_descriptor (unsigned type, unsigned long base_addr, int idx, int force)
{
  struct seg_descr descr;
  struct gate_descr gate;

  /* Get the descriptor from the table.  */
  if (idx == 0 && type == 0)
    gdb_puts ("0x000: null descriptor\n");
  else if (get_descriptor (base_addr, idx, &descr) != -1)
    {
      /* For each type of descriptor table, this has a bit set if the
	 corresponding type of selectors is valid in that table.  */
      static unsigned allowed_descriptors[] = {
	  0xffffdafeL,   /* GDT */
	  0x0000c0e0L,   /* IDT */
	  0xffffdafaL    /* LDT */
      };

      /* If the program hasn't started yet, assume the debuggee will
	 have the same CPL as the debugger.  */
      int cpl = prog_has_started ? (a_tss.tss_cs & 3) : _my_cs () & 3;
      unsigned long limit = (descr.limit1 << 16) | descr.limit0;

      if (descr.present
	  && (allowed_descriptors[type] & (1 << descr.stype)) != 0)
	{
	  gdb_printf ("0x%03x: ",
		      type == 1
		      ? idx : (idx * 8) | (type ? (cpl | 4) : 0));
	  if (descr.page_granular)
	    limit = (limit << 12) | 0xfff; /* big segment: low 12 bit set */
	  if (descr.stype == 1 || descr.stype == 2 || descr.stype == 3
	      || descr.stype == 9 || descr.stype == 11
	      || (descr.stype >= 16 && descr.stype < 32))
	    gdb_printf ("base=0x%02x%02x%04x limit=0x%08lx",
			descr.base2, descr.base1, descr.base0, limit);

	  switch (descr.stype)
	    {
	      case 1:
	      case 3:
		gdb_printf (" 16-bit TSS  (task %sactive)",
			    descr.stype == 3 ? "" : "in");
		break;
	      case 2:
		gdb_puts (" LDT");
		break;
	      case 4:
		memcpy (&gate, &descr, sizeof gate);
		gdb_printf ("selector=0x%04x  offs=0x%04x%04x",
			    gate.selector, gate.offset1, gate.offset0);
		gdb_printf (" 16-bit Call Gate (params=%d)",
			    gate.param_count);
		break;
	      case 5:
		gdb_printf ("TSS selector=0x%04x", descr.base0);
		gdb_printf ("%*sTask Gate", 16, "");
		break;
	      case 6:
	      case 7:
		memcpy (&gate, &descr, sizeof gate);
		gdb_printf ("selector=0x%04x  offs=0x%04x%04x",
			    gate.selector, gate.offset1, gate.offset0);
		gdb_printf (" 16-bit %s Gate",
			    descr.stype == 6 ? "Interrupt" : "Trap");
		break;
	      case 9:
	      case 11:
		gdb_printf (" 32-bit TSS (task %sactive)",
			    descr.stype == 3 ? "" : "in");
		break;
	      case 12:
		memcpy (&gate, &descr, sizeof gate);
		gdb_printf ("selector=0x%04x  offs=0x%04x%04x",
			    gate.selector, gate.offset1, gate.offset0);
		gdb_printf (" 32-bit Call Gate (params=%d)",
			    gate.param_count);
		break;
	      case 14:
	      case 15:
		memcpy (&gate, &descr, sizeof gate);
		gdb_printf ("selector=0x%04x  offs=0x%04x%04x",
			    gate.selector, gate.offset1, gate.offset0);
		gdb_printf (" 32-bit %s Gate",
			    descr.stype == 14 ? "Interrupt" : "Trap");
		break;
	      case 16:		/* data segments */
	      case 17:
	      case 18:
	      case 19:
	      case 20:
	      case 21:
	      case 22:
	      case 23:
		gdb_printf (" %s-bit Data (%s Exp-%s%s)",
			    descr.bit32 ? "32" : "16",
			    descr.stype & 2
			    ? "Read/Write," : "Read-Only, ",
			    descr.stype & 4 ? "down" : "up",
			    descr.stype & 1 ? "" : ", N.Acc");
		break;
	      case 24:		/* code segments */
	      case 25:
	      case 26:
	      case 27:
	      case 28:
	      case 29:
	      case 30:
	      case 31:
		gdb_printf (" %s-bit Code (%s,  %sConf%s)",
			    descr.bit32 ? "32" : "16",
			    descr.stype & 2 ? "Exec/Read" : "Exec-Only",
			    descr.stype & 4 ? "" : "N.",
			    descr.stype & 1 ? "" : ", N.Acc");
		break;
	      default:
		gdb_printf ("Unknown type 0x%02x", descr.stype);
		break;
	    }
	  gdb_puts ("\n");
	}
      else if (force)
	{
	  gdb_printf ("0x%03x: ",
		      type == 1
		      ? idx : (idx * 8) | (type ? (cpl | 4) : 0));
	  if (!descr.present)
	    gdb_puts ("Segment not present\n");
	  else
	    gdb_printf ("Segment type 0x%02x is invalid in this table\n",
			descr.stype);
	}
    }
  else if (force)
    gdb_printf ("0x%03x: Cannot read this descriptor\n", idx);
}

static void
go32_sldt (const char *arg, int from_tty)
{
  struct dtr_reg gdtr;
  unsigned short ldtr = 0;
  int ldt_idx;
  struct seg_descr ldt_descr;
  long ldt_entry = -1L;
  int cpl = (prog_has_started ? a_tss.tss_cs : _my_cs ()) & 3;

  if (arg && *arg)
    {
      arg = skip_spaces (arg);

      if (*arg)
	{
	  ldt_entry = parse_and_eval_long (arg);
	  if (ldt_entry < 0
	      || (ldt_entry & 4) == 0
	      || (ldt_entry & 3) != (cpl & 3))
	    error (_("Invalid LDT entry 0x%03lx."), (unsigned long)ldt_entry);
	}
    }

  __asm__ __volatile__ ("sgdt   %0" : "=m" (gdtr) : /* no inputs */ );
  __asm__ __volatile__ ("sldt   %0" : "=m" (ldtr) : /* no inputs */ );
  ldt_idx = ldtr / 8;
  if (ldt_idx == 0)
    gdb_puts ("There is no LDT.\n");
  /* LDT's entry in the GDT must have the type LDT, which is 2.  */
  else if (get_descriptor (gdtr.base, ldt_idx, &ldt_descr) != 2)
    gdb_printf ("LDT is present (at %#x), but unreadable by GDB.\n",
		ldt_descr.base0
		| (ldt_descr.base1 << 16)
		| (ldt_descr.base2 << 24));
  else
    {
      unsigned base =
	ldt_descr.base0
	| (ldt_descr.base1 << 16)
	| (ldt_descr.base2 << 24);
      unsigned limit = ldt_descr.limit0 | (ldt_descr.limit1 << 16);
      int max_entry;

      if (ldt_descr.page_granular)
	/* Page-granular segments must have the low 12 bits of their
	   limit set.  */
	limit = (limit << 12) | 0xfff;
      /* LDT cannot have more than 8K 8-byte entries, i.e. more than
	 64KB.  */
      if (limit > 0xffff)
	limit = 0xffff;

      max_entry = (limit + 1) / 8;

      if (ldt_entry >= 0)
	{
	  if (ldt_entry > limit)
	    error (_("Invalid LDT entry %#lx: outside valid limits [0..%#x]"),
		   (unsigned long)ldt_entry, limit);

	  display_descriptor (ldt_descr.stype, base, ldt_entry / 8, 1);
	}
      else
	{
	  int i;

	  for (i = 0; i < max_entry; i++)
	    display_descriptor (ldt_descr.stype, base, i, 0);
	}
    }
}

static void
go32_sgdt (const char *arg, int from_tty)
{
  struct dtr_reg gdtr;
  long gdt_entry = -1L;
  int max_entry;

  if (arg && *arg)
    {
      arg = skip_spaces (arg);

      if (*arg)
	{
	  gdt_entry = parse_and_eval_long (arg);
	  if (gdt_entry < 0 || (gdt_entry & 7) != 0)
	    error (_("Invalid GDT entry 0x%03lx: "
		     "not an integral multiple of 8."),
		   (unsigned long)gdt_entry);
	}
    }

  __asm__ __volatile__ ("sgdt   %0" : "=m" (gdtr) : /* no inputs */ );
  max_entry = (gdtr.limit + 1) / 8;

  if (gdt_entry >= 0)
    {
      if (gdt_entry > gdtr.limit)
	error (_("Invalid GDT entry %#lx: outside valid limits [0..%#x]"),
	       (unsigned long)gdt_entry, gdtr.limit);

      display_descriptor (0, gdtr.base, gdt_entry / 8, 1);
    }
  else
    {
      int i;

      for (i = 0; i < max_entry; i++)
	display_descriptor (0, gdtr.base, i, 0);
    }
}

static void
go32_sidt (const char *arg, int from_tty)
{
  struct dtr_reg idtr;
  long idt_entry = -1L;
  int max_entry;

  if (arg && *arg)
    {
      arg = skip_spaces (arg);

      if (*arg)
	{
	  idt_entry = parse_and_eval_long (arg);
	  if (idt_entry < 0)
	    error (_("Invalid (negative) IDT entry %ld."), idt_entry);
	}
    }

  __asm__ __volatile__ ("sidt   %0" : "=m" (idtr) : /* no inputs */ );
  max_entry = (idtr.limit + 1) / 8;
  if (max_entry > 0x100)	/* No more than 256 entries.  */
    max_entry = 0x100;

  if (idt_entry >= 0)
    {
      if (idt_entry > idtr.limit)
	error (_("Invalid IDT entry %#lx: outside valid limits [0..%#x]"),
	       (unsigned long)idt_entry, idtr.limit);

      display_descriptor (1, idtr.base, idt_entry, 1);
    }
  else
    {
      int i;

      for (i = 0; i < max_entry; i++)
	display_descriptor (1, idtr.base, i, 0);
    }
}

/* Cached linear address of the base of the page directory.  For
   now, available only under CWSDPMI.  Code based on ideas and
   suggestions from Charles Sandmann <sandmann@clio.rice.edu>.  */
static unsigned long pdbr;

static unsigned long
get_cr3 (void)
{
  unsigned offset;
  unsigned taskreg;
  unsigned long taskbase, cr3;
  struct dtr_reg gdtr;

  if (pdbr > 0 && pdbr <= 0xfffff)
    return pdbr;

  /* Get the linear address of GDT and the Task Register.  */
  __asm__ __volatile__ ("sgdt   %0" : "=m" (gdtr) : /* no inputs */ );
  __asm__ __volatile__ ("str    %0" : "=m" (taskreg) : /* no inputs */ );

  /* Task Register is a segment selector for the TSS of the current
     task.  Therefore, it can be used as an index into the GDT to get
     at the segment descriptor for the TSS.  To get the index, reset
     the low 3 bits of the selector (which give the CPL).  Add 2 to the
     offset to point to the 3 low bytes of the base address.  */
  offset = gdtr.base + (taskreg & 0xfff8) + 2;


  /* CWSDPMI's task base is always under the 1MB mark.  */
  if (offset > 0xfffff)
    return 0;

  _farsetsel (_dos_ds);
  taskbase  = _farnspeekl (offset) & 0xffffffU;
  taskbase += _farnspeekl (offset + 2) & 0xff000000U;
  if (taskbase > 0xfffff)
    return 0;

  /* CR3 (a.k.a. PDBR, the Page Directory Base Register) is stored at
     offset 1Ch in the TSS.  */
  cr3 = _farnspeekl (taskbase + 0x1c) & ~0xfff;
  if (cr3 > 0xfffff)
    {
#if 0  /* Not fully supported yet.  */
      /* The Page Directory is in UMBs.  In that case, CWSDPMI puts
	 the first Page Table right below the Page Directory.  Thus,
	 the first Page Table's entry for its own address and the Page
	 Directory entry for that Page Table will hold the same
	 physical address.  The loop below searches the entire UMB
	 range of addresses for such an occurrence.  */
      unsigned long addr, pte_idx;

      for (addr = 0xb0000, pte_idx = 0xb0;
	   pte_idx < 0xff;
	   addr += 0x1000, pte_idx++)
	{
	  if (((_farnspeekl (addr + 4 * pte_idx) & 0xfffff027) ==
	       (_farnspeekl (addr + 0x1000) & 0xfffff027))
	      && ((_farnspeekl (addr + 4 * pte_idx + 4) & 0xfffff000) == cr3))
	    {
	      cr3 = addr + 0x1000;
	      break;
	    }
	}
#endif

      if (cr3 > 0xfffff)
	cr3 = 0;
    }

  return cr3;
}

/* Return the N'th Page Directory entry.  */
static unsigned long
get_pde (int n)
{
  unsigned long pde = 0;

  if (pdbr && n >= 0 && n < 1024)
    {
      pde = _farpeekl (_dos_ds, pdbr + 4*n);
    }
  return pde;
}

/* Return the N'th entry of the Page Table whose Page Directory entry
   is PDE.  */
static unsigned long
get_pte (unsigned long pde, int n)
{
  unsigned long pte = 0;

  /* pde & 0x80 tests the 4MB page bit.  We don't support 4MB
     page tables, for now.  */
  if ((pde & 1) && !(pde & 0x80) && n >= 0 && n < 1024)
    {
      pde &= ~0xfff;	/* Clear non-address bits.  */
      pte = _farpeekl (_dos_ds, pde + 4*n);
    }
  return pte;
}

/* Display a Page Directory or Page Table entry.  IS_DIR, if non-zero,
   says this is a Page Directory entry.  If FORCE is non-zero, display
   the entry even if its Present flag is off.  OFF is the offset of the
   address from the page's base address.  */
static void
display_ptable_entry (unsigned long entry, int is_dir, int force, unsigned off)
{
  if ((entry & 1) != 0)
    {
      gdb_printf ("Base=0x%05lx000", entry >> 12);
      if ((entry & 0x100) && !is_dir)
	gdb_puts (" Global");
      if ((entry & 0x40) && !is_dir)
	gdb_puts (" Dirty");
      gdb_printf (" %sAcc.", (entry & 0x20) ? "" : "Not-");
      gdb_printf (" %sCached", (entry & 0x10) ? "" : "Not-");
      gdb_printf (" Write-%s", (entry & 8) ? "Thru" : "Back");
      gdb_printf (" %s", (entry & 4) ? "Usr" : "Sup");
      gdb_printf (" Read-%s", (entry & 2) ? "Write" : "Only");
      if (off)
	gdb_printf (" +0x%x", off);
      gdb_puts ("\n");
    }
  else if (force)
    gdb_printf ("Page%s not present or not supported; value=0x%lx.\n",
		is_dir ? " Table" : "", entry >> 1);
}

static void
go32_pde (const char *arg, int from_tty)
{
  long pde_idx = -1, i;

  if (arg && *arg)
    {
      arg = skip_spaces (arg);

      if (*arg)
	{
	  pde_idx = parse_and_eval_long (arg);
	  if (pde_idx < 0 || pde_idx >= 1024)
	    error (_("Entry %ld is outside valid limits [0..1023]."), pde_idx);
	}
    }

  pdbr = get_cr3 ();
  if (!pdbr)
    gdb_puts ("Access to Page Directories is "
	      "not supported on this system.\n");
  else if (pde_idx >= 0)
    display_ptable_entry (get_pde (pde_idx), 1, 1, 0);
  else
    for (i = 0; i < 1024; i++)
      display_ptable_entry (get_pde (i), 1, 0, 0);
}

/* A helper function to display entries in a Page Table pointed to by
   the N'th entry in the Page Directory.  If FORCE is non-zero, say
   something even if the Page Table is not accessible.  */
static void
display_page_table (long n, int force)
{
  unsigned long pde = get_pde (n);

  if ((pde & 1) != 0)
    {
      int i;

      gdb_printf ("Page Table pointed to by "
		  "Page Directory entry 0x%lx:\n", n);
      for (i = 0; i < 1024; i++)
	display_ptable_entry (get_pte (pde, i), 0, 0, 0);
      gdb_puts ("\n");
    }
  else if (force)
    gdb_printf ("Page Table not present; value=0x%lx.\n", pde >> 1);
}

static void
go32_pte (const char *arg, int from_tty)
{
  long pde_idx = -1L, i;

  if (arg && *arg)
    {
      arg = skip_spaces (arg);

      if (*arg)
	{
	  pde_idx = parse_and_eval_long (arg);
	  if (pde_idx < 0 || pde_idx >= 1024)
	    error (_("Entry %ld is outside valid limits [0..1023]."), pde_idx);
	}
    }

  pdbr = get_cr3 ();
  if (!pdbr)
    gdb_puts ("Access to Page Tables is not supported on this system.\n");
  else if (pde_idx >= 0)
    display_page_table (pde_idx, 1);
  else
    for (i = 0; i < 1024; i++)
      display_page_table (i, 0);
}

static void
go32_pte_for_address (const char *arg, int from_tty)
{
  CORE_ADDR addr = 0, i;

  if (arg && *arg)
    {
      arg = skip_spaces (arg);

      if (*arg)
	addr = parse_and_eval_address (arg);
    }
  if (!addr)
    error_no_arg (_("linear address"));

  pdbr = get_cr3 ();
  if (!pdbr)
    gdb_puts ("Access to Page Tables is not supported on this system.\n");
  else
    {
      int pde_idx = (addr >> 22) & 0x3ff;
      int pte_idx = (addr >> 12) & 0x3ff;
      unsigned offs = addr & 0xfff;

      gdb_printf ("Page Table entry for address %s:\n",
		  hex_string(addr));
      display_ptable_entry (get_pte (get_pde (pde_idx), pte_idx), 0, 1, offs);
    }
}

static struct cmd_list_element *info_dos_cmdlist = NULL;

void _initialize_go32_nat ();
void
_initialize_go32_nat ()
{
  x86_dr_low.set_control = go32_set_dr7;
  x86_dr_low.set_addr = go32_set_dr;
  x86_dr_low.get_status = go32_get_dr6;
  x86_dr_low.get_control = go32_get_dr7;
  x86_dr_low.get_addr = go32_get_dr;
  x86_set_debug_register_length (4);

  add_inf_child_target (&the_go32_nat_target);

  /* Initialize child's cwd as empty to be initialized when starting
     the child.  */
  *child_cwd = 0;

  /* Initialize child's command line storage.  */
  if (redir_debug_init (&child_cmd) == -1)
    internal_error (_("Cannot allocate redirection storage: "
		      "not enough memory.\n"));

  /* We are always processing GCC-compiled programs.  */
  processing_gcc_compilation = 2;

  add_basic_prefix_cmd ("dos", class_info, _("\
Print information specific to DJGPP (aka MS-DOS) debugging."),
			&info_dos_cmdlist, 0, &infolist);

  add_cmd ("sysinfo", class_info, go32_sysinfo, _("\
Display information about the target system, including CPU, OS, DPMI, etc."),
	   &info_dos_cmdlist);
  add_cmd ("ldt", class_info, go32_sldt, _("\
Display entries in the LDT (Local Descriptor Table).\n\
Entry number (an expression) as an argument means display only that entry."),
	   &info_dos_cmdlist);
  add_cmd ("gdt", class_info, go32_sgdt, _("\
Display entries in the GDT (Global Descriptor Table).\n\
Entry number (an expression) as an argument means display only that entry."),
	   &info_dos_cmdlist);
  add_cmd ("idt", class_info, go32_sidt, _("\
Display entries in the IDT (Interrupt Descriptor Table).\n\
Entry number (an expression) as an argument means display only that entry."),
	   &info_dos_cmdlist);
  add_cmd ("pde", class_info, go32_pde, _("\
Display entries in the Page Directory.\n\
Entry number (an expression) as an argument means display only that entry."),
	   &info_dos_cmdlist);
  add_cmd ("pte", class_info, go32_pte, _("\
Display entries in Page Tables.\n\
Entry number (an expression) as an argument means display only entries\n\
from the Page Table pointed to by the specified Page Directory entry."),
	   &info_dos_cmdlist);
  add_cmd ("address-pte", class_info, go32_pte_for_address, _("\
Display a Page Table entry for a linear address.\n\
The address argument must be a linear address, after adding to\n\
it the base address of the appropriate segment.\n\
The base address of variables and functions in the debuggee's data\n\
or code segment is stored in the variable __djgpp_base_address,\n\
so use `__djgpp_base_address + (char *)&var' as the argument.\n\
For other segments, look up their base address in the output of\n\
the `info dos ldt' command."),
	   &info_dos_cmdlist);
}

pid_t
tcgetpgrp (int fd)
{
  if (isatty (fd))
    return SOME_PID;
  errno = ENOTTY;
  return -1;
}

int
tcsetpgrp (int fd, pid_t pgid)
{
  if (isatty (fd) && pgid == SOME_PID)
    return 0;
  errno = pgid == SOME_PID ? ENOTTY : ENOSYS;
  return -1;
}
