/* Common target-dependent code for NetBSD systems.

   Copyright (C) 2002-2024 Free Software Foundation, Inc.

   Contributed by Wasabi Systems, Inc.
  
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

#include "defs.h"
#include "auxv.h"
#include "solib-svr4.h"
#include "netbsd-tdep.h"
#include "gdbarch.h"
#include "objfiles.h"
#include "xml-syscall.h"

/* Flags in the 'kve_protection' field in struct kinfo_vmentry.  These
   match the KVME_PROT_* constants in <sys/sysctl.h>.  */

#define	KINFO_VME_PROT_READ	0x00000001
#define	KINFO_VME_PROT_WRITE	0x00000002
#define	KINFO_VME_PROT_EXEC	0x00000004

/* Flags in the 'kve_flags' field in struct kinfo_vmentry.  These
   match the KVME_FLAG_* constants in <sys/sysctl.h>.  */

#define	KINFO_VME_FLAG_COW		0x00000001
#define	KINFO_VME_FLAG_NEEDS_COPY	0x00000002
#define	KINFO_VME_FLAG_NOCOREDUMP	0x00000004
#define	KINFO_VME_FLAG_PAGEABLE		0x00000008
#define	KINFO_VME_FLAG_GROWS_UP		0x00000010
#define	KINFO_VME_FLAG_GROWS_DOWN	0x00000020

int
nbsd_pc_in_sigtramp (CORE_ADDR pc, const char *func_name)
{
  /* Check for libc-provided signal trampoline.  All such trampolines
     have function names which begin with "__sigtramp".  */

  return (func_name != NULL
	  && startswith (func_name, "__sigtramp"));
}

/* This enum is derived from NETBSD's <sys/signal.h>.  */

enum
  {
   NBSD_SIGHUP = 1,
   NBSD_SIGINT = 2,
   NBSD_SIGQUIT = 3,
   NBSD_SIGILL = 4,
   NBSD_SIGTRAP = 5,
   NBSD_SIGABRT = 6,
   NBSD_SIGEMT = 7,
   NBSD_SIGFPE = 8,
   NBSD_SIGKILL = 9,
   NBSD_SIGBUS = 10,
   NBSD_SIGSEGV = 11,
   NBSD_SIGSYS = 12,
   NBSD_SIGPIPE = 13,
   NBSD_SIGALRM = 14,
   NBSD_SIGTERM = 15,
   NBSD_SIGURG = 16,
   NBSD_SIGSTOP = 17,
   NBSD_SIGTSTP = 18,
   NBSD_SIGCONT = 19,
   NBSD_SIGCHLD = 20,
   NBSD_SIGTTIN = 21,
   NBSD_SIGTTOU = 22,
   NBSD_SIGIO = 23,
   NBSD_SIGXCPU = 24,
   NBSD_SIGXFSZ = 25,
   NBSD_SIGVTALRM = 26,
   NBSD_SIGPROF = 27,
   NBSD_SIGWINCH = 28,
   NBSD_SIGINFO = 29,
   NBSD_SIGUSR1 = 30,
   NBSD_SIGUSR2 = 31,
   NBSD_SIGPWR = 32,
   NBSD_SIGRTMIN = 33,
   NBSD_SIGRTMAX = 63,
  };

/* Implement the "gdb_signal_from_target" gdbarch method.  */

static enum gdb_signal
nbsd_gdb_signal_from_target (struct gdbarch *gdbarch, int signal)
{
  switch (signal)
    {
    case 0:
      return GDB_SIGNAL_0;

    case NBSD_SIGHUP:
      return GDB_SIGNAL_HUP;

    case NBSD_SIGINT:
      return GDB_SIGNAL_INT;

    case NBSD_SIGQUIT:
      return GDB_SIGNAL_QUIT;

    case NBSD_SIGILL:
      return GDB_SIGNAL_ILL;

    case NBSD_SIGTRAP:
      return GDB_SIGNAL_TRAP;

    case NBSD_SIGABRT:
      return GDB_SIGNAL_ABRT;

    case NBSD_SIGEMT:
      return GDB_SIGNAL_EMT;

    case NBSD_SIGFPE:
      return GDB_SIGNAL_FPE;

    case NBSD_SIGKILL:
      return GDB_SIGNAL_KILL;

    case NBSD_SIGBUS:
      return GDB_SIGNAL_BUS;

    case NBSD_SIGSEGV:
      return GDB_SIGNAL_SEGV;

    case NBSD_SIGSYS:
      return GDB_SIGNAL_SYS;

    case NBSD_SIGPIPE:
      return GDB_SIGNAL_PIPE;

    case NBSD_SIGALRM:
      return GDB_SIGNAL_ALRM;

    case NBSD_SIGTERM:
      return GDB_SIGNAL_TERM;

    case NBSD_SIGURG:
      return GDB_SIGNAL_URG;

    case NBSD_SIGSTOP:
      return GDB_SIGNAL_STOP;

    case NBSD_SIGTSTP:
      return GDB_SIGNAL_TSTP;

    case NBSD_SIGCONT:
      return GDB_SIGNAL_CONT;

    case NBSD_SIGCHLD:
      return GDB_SIGNAL_CHLD;

    case NBSD_SIGTTIN:
      return GDB_SIGNAL_TTIN;

    case NBSD_SIGTTOU:
      return GDB_SIGNAL_TTOU;

    case NBSD_SIGIO:
      return GDB_SIGNAL_IO;

    case NBSD_SIGXCPU:
      return GDB_SIGNAL_XCPU;

    case NBSD_SIGXFSZ:
      return GDB_SIGNAL_XFSZ;

    case NBSD_SIGVTALRM:
      return GDB_SIGNAL_VTALRM;

    case NBSD_SIGPROF:
      return GDB_SIGNAL_PROF;

    case NBSD_SIGWINCH:
      return GDB_SIGNAL_WINCH;

    case NBSD_SIGINFO:
      return GDB_SIGNAL_INFO;

    case NBSD_SIGUSR1:
      return GDB_SIGNAL_USR1;

    case NBSD_SIGUSR2:
      return GDB_SIGNAL_USR2;

    case NBSD_SIGPWR:
      return GDB_SIGNAL_PWR;

    /* SIGRTMIN and SIGRTMAX are not continuous in <gdb/signals.def>,
       therefore we have to handle them here.  */
    case NBSD_SIGRTMIN:
      return GDB_SIGNAL_REALTIME_33;

    case NBSD_SIGRTMAX:
      return GDB_SIGNAL_REALTIME_63;
    }

  if (signal >= NBSD_SIGRTMIN + 1 && signal <= NBSD_SIGRTMAX - 1)
    {
      int offset = signal - NBSD_SIGRTMIN + 1;

      return (enum gdb_signal) ((int) GDB_SIGNAL_REALTIME_34 + offset);
    }

  return GDB_SIGNAL_UNKNOWN;
}

/* Implement the "gdb_signal_to_target" gdbarch method.  */

static int
nbsd_gdb_signal_to_target (struct gdbarch *gdbarch,
		enum gdb_signal signal)
{
  switch (signal)
    {
    case GDB_SIGNAL_0:
      return 0;

    case GDB_SIGNAL_HUP:
      return NBSD_SIGHUP;

    case GDB_SIGNAL_INT:
      return NBSD_SIGINT;

    case GDB_SIGNAL_QUIT:
      return NBSD_SIGQUIT;

    case GDB_SIGNAL_ILL:
      return NBSD_SIGILL;

    case GDB_SIGNAL_TRAP:
      return NBSD_SIGTRAP;

    case GDB_SIGNAL_ABRT:
      return NBSD_SIGABRT;

    case GDB_SIGNAL_EMT:
      return NBSD_SIGEMT;

    case GDB_SIGNAL_FPE:
      return NBSD_SIGFPE;

    case GDB_SIGNAL_KILL:
      return NBSD_SIGKILL;

    case GDB_SIGNAL_BUS:
      return NBSD_SIGBUS;

    case GDB_SIGNAL_SEGV:
      return NBSD_SIGSEGV;

    case GDB_SIGNAL_SYS:
      return NBSD_SIGSYS;

    case GDB_SIGNAL_PIPE:
      return NBSD_SIGPIPE;

    case GDB_SIGNAL_ALRM:
      return NBSD_SIGALRM;

    case GDB_SIGNAL_TERM:
      return NBSD_SIGTERM;

    case GDB_SIGNAL_URG:
      return NBSD_SIGSTOP;

    case GDB_SIGNAL_TSTP:
      return NBSD_SIGTSTP;

    case GDB_SIGNAL_CONT:
      return NBSD_SIGCONT;

    case GDB_SIGNAL_CHLD:
      return NBSD_SIGCHLD;

    case GDB_SIGNAL_TTIN:
      return NBSD_SIGTTIN;

    case GDB_SIGNAL_TTOU:
      return NBSD_SIGTTOU;

    case GDB_SIGNAL_IO:
      return NBSD_SIGIO;

    case GDB_SIGNAL_XCPU:
      return NBSD_SIGXCPU;

    case GDB_SIGNAL_XFSZ:
      return NBSD_SIGXFSZ;

    case GDB_SIGNAL_VTALRM:
      return NBSD_SIGVTALRM;

    case GDB_SIGNAL_PROF:
      return NBSD_SIGPROF;

    case GDB_SIGNAL_WINCH:
      return NBSD_SIGWINCH;

    case GDB_SIGNAL_INFO:
      return NBSD_SIGINFO;

    case GDB_SIGNAL_USR1:
      return NBSD_SIGUSR1;

    case GDB_SIGNAL_USR2:
      return NBSD_SIGUSR2;

    case GDB_SIGNAL_PWR:
      return NBSD_SIGPWR;

    /* GDB_SIGNAL_REALTIME_33 is not continuous in <gdb/signals.def>,
       therefore we have to handle it here.  */
    case GDB_SIGNAL_REALTIME_33:
      return NBSD_SIGRTMIN;

    /* Same comment applies to _64.  */
    case GDB_SIGNAL_REALTIME_63:
      return NBSD_SIGRTMAX;
    }

  if (signal >= GDB_SIGNAL_REALTIME_34
      && signal <= GDB_SIGNAL_REALTIME_62)
    {
      int offset = signal - GDB_SIGNAL_REALTIME_32;

      return NBSD_SIGRTMIN + 1 + offset;
    }

  return -1;
}

/* Shared library resolver handling.  */

static CORE_ADDR
nbsd_skip_solib_resolver (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  struct bound_minimal_symbol msym;

  msym = lookup_minimal_symbol ("_rtld_bind_start", NULL, NULL);
  if (msym.minsym && msym.value_address () == pc)
    return frame_unwind_caller_pc (get_current_frame ());
  else
    return find_solib_trampoline_target (get_current_frame (), pc);
}

struct nbsd_gdbarch_data
{
  struct type *siginfo_type = nullptr;
};

static const registry<gdbarch>::key<nbsd_gdbarch_data>
     nbsd_gdbarch_data_handle;

static struct nbsd_gdbarch_data *
get_nbsd_gdbarch_data (struct gdbarch *gdbarch)
{
  struct nbsd_gdbarch_data *result = nbsd_gdbarch_data_handle.get (gdbarch);
  if (result == nullptr)
    result = nbsd_gdbarch_data_handle.emplace (gdbarch);
  return result;
}

/* Implement the "get_siginfo_type" gdbarch method.  */

static struct type *
nbsd_get_siginfo_type (struct gdbarch *gdbarch)
{
  nbsd_gdbarch_data *nbsd_gdbarch_data = get_nbsd_gdbarch_data (gdbarch);
  if (nbsd_gdbarch_data->siginfo_type != NULL)
    return nbsd_gdbarch_data->siginfo_type;

  type *char_type = builtin_type (gdbarch)->builtin_char;
  type *int_type = builtin_type (gdbarch)->builtin_int;
  type *long_type = builtin_type (gdbarch)->builtin_long;

  type *void_ptr_type
    = lookup_pointer_type (builtin_type (gdbarch)->builtin_void);

  type *int32_type = builtin_type (gdbarch)->builtin_int32;
  type *uint32_type = builtin_type (gdbarch)->builtin_uint32;
  type *uint64_type = builtin_type (gdbarch)->builtin_uint64;

  bool lp64 = void_ptr_type->length () == 8;
  size_t char_bits = gdbarch_addressable_memory_unit_size (gdbarch) * 8;

  /* pid_t */
  type_allocator alloc (gdbarch);
  type *pid_type = alloc.new_type (TYPE_CODE_TYPEDEF,
				   int32_type->length () * char_bits,
				   "pid_t");
  pid_type->set_target_type (int32_type);

  /* uid_t */
  type *uid_type = alloc.new_type (TYPE_CODE_TYPEDEF,
				   uint32_type->length () * char_bits,
				   "uid_t");
  uid_type->set_target_type (uint32_type);

  /* clock_t */
  type *clock_type = alloc.new_type (TYPE_CODE_TYPEDEF,
				     int_type->length () * char_bits,
				     "clock_t");
  clock_type->set_target_type (int_type);

  /* lwpid_t */
  type *lwpid_type = alloc.new_type (TYPE_CODE_TYPEDEF,
				     int32_type->length () * char_bits,
				     "lwpid_t");
  lwpid_type->set_target_type (int32_type);

  /* union sigval */
  type *sigval_type = arch_composite_type (gdbarch, NULL, TYPE_CODE_UNION);
  sigval_type->set_name (gdbarch_obstack_strdup (gdbarch, "sigval"));
  append_composite_type_field (sigval_type, "sival_int", int_type);
  append_composite_type_field (sigval_type, "sival_ptr", void_ptr_type);

  /* union _option */
  type *option_type = arch_composite_type (gdbarch, NULL, TYPE_CODE_UNION);
  option_type->set_name (gdbarch_obstack_strdup (gdbarch, "_option"));
  append_composite_type_field (option_type, "_pe_other_pid", pid_type);
  append_composite_type_field (option_type, "_pe_lwp", lwpid_type);

  /* union _reason */
  type *reason_type = arch_composite_type (gdbarch, NULL, TYPE_CODE_UNION);

  /* _rt */
  type *t = arch_composite_type (gdbarch, NULL, TYPE_CODE_STRUCT);
  append_composite_type_field (t, "_pid", pid_type);
  append_composite_type_field (t, "_uid", uid_type);
  append_composite_type_field (t, "_value", sigval_type);
  append_composite_type_field (reason_type, "_rt", t);

  /* _child */
  t = arch_composite_type (gdbarch, NULL, TYPE_CODE_STRUCT);
  append_composite_type_field (t, "_pid", pid_type);
  append_composite_type_field (t, "_uid", uid_type);
  append_composite_type_field (t, "_status", int_type);
  append_composite_type_field (t, "_utime", clock_type);
  append_composite_type_field (t, "_stime", clock_type);
  append_composite_type_field (reason_type, "_child", t);

  /* _fault */
  t = arch_composite_type (gdbarch, NULL, TYPE_CODE_STRUCT);
  append_composite_type_field (t, "_addr", void_ptr_type);
  append_composite_type_field (t, "_trap", int_type);
  append_composite_type_field (t, "_trap2", int_type);
  append_composite_type_field (t, "_trap3", int_type);
  append_composite_type_field (reason_type, "_fault", t);

  /* _poll */
  t = arch_composite_type (gdbarch, NULL, TYPE_CODE_STRUCT);
  append_composite_type_field (t, "_band", long_type);
  append_composite_type_field (t, "_fd", int_type);
  append_composite_type_field (reason_type, "_poll", t);

  /* _syscall */
  t = arch_composite_type (gdbarch, NULL, TYPE_CODE_STRUCT);
  append_composite_type_field (t, "_sysnum", int_type);
  append_composite_type_field (t, "_retval",
			       init_vector_type (int_type, 2));
  append_composite_type_field (t, "_error", int_type);
  append_composite_type_field (t, "_args",
			       init_vector_type (uint64_type, 8));
  append_composite_type_field (reason_type, "_syscall", t);

  /* _ptrace_state */
  t = arch_composite_type (gdbarch, NULL, TYPE_CODE_STRUCT);
  append_composite_type_field (t, "_pe_report_event", int_type);
  append_composite_type_field (t, "_option", option_type);
  append_composite_type_field (reason_type, "_ptrace_state", t);

  /* struct _ksiginfo */
  type *ksiginfo_type = arch_composite_type (gdbarch, NULL, TYPE_CODE_STRUCT);
  ksiginfo_type->set_name (gdbarch_obstack_strdup (gdbarch, "_ksiginfo"));
  append_composite_type_field (ksiginfo_type, "_signo", int_type);
  append_composite_type_field (ksiginfo_type, "_code", int_type);
  append_composite_type_field (ksiginfo_type, "_errno", int_type);
  if (lp64)
    append_composite_type_field (ksiginfo_type, "_pad", int_type);
  append_composite_type_field (ksiginfo_type, "_reason", reason_type);

  /* union siginfo */
  type *siginfo_type = arch_composite_type (gdbarch, NULL, TYPE_CODE_UNION);
  siginfo_type->set_name (gdbarch_obstack_strdup (gdbarch, "siginfo"));
  append_composite_type_field (siginfo_type, "si_pad",
			       init_vector_type (char_type, 128));
  append_composite_type_field (siginfo_type, "_info", ksiginfo_type);

  nbsd_gdbarch_data->siginfo_type = siginfo_type;

  return siginfo_type;
}

/* See netbsd-tdep.h.  */

void
nbsd_info_proc_mappings_header (int addr_bit)
{
  gdb_printf (_("Mapped address spaces:\n\n"));
  if (addr_bit == 64)
    {
      gdb_printf ("  %18s %18s %10s %10s %9s %s\n",
		  "Start Addr",
		  "  End Addr",
		  "      Size", "    Offset", "Flags  ", "File");
    }
  else
    {
      gdb_printf ("\t%10s %10s %10s %10s %9s %s\n",
		  "Start Addr",
		  "  End Addr",
		  "      Size", "    Offset", "Flags  ", "File");
    }
}

/* Helper function to generate mappings flags for a single VM map
   entry in 'info proc mappings'.  */

static const char *
nbsd_vm_map_entry_flags (int kve_flags, int kve_protection)
{
  static char vm_flags[9];

  vm_flags[0] = (kve_protection & KINFO_VME_PROT_READ) ? 'r' : '-';
  vm_flags[1] = (kve_protection & KINFO_VME_PROT_WRITE) ? 'w' : '-';
  vm_flags[2] = (kve_protection & KINFO_VME_PROT_EXEC) ? 'x' : '-';
  vm_flags[3] = ' ';
  vm_flags[4] = (kve_flags & KINFO_VME_FLAG_COW) ? 'C' : '-';
  vm_flags[5] = (kve_flags & KINFO_VME_FLAG_NEEDS_COPY) ? 'N' : '-';
  vm_flags[6] = (kve_flags & KINFO_VME_FLAG_PAGEABLE) ? 'P' : '-';
  vm_flags[7] = (kve_flags & KINFO_VME_FLAG_GROWS_UP) ? 'U'
    : (kve_flags & KINFO_VME_FLAG_GROWS_DOWN) ? 'D' : '-';
  vm_flags[8] = '\0';

  return vm_flags;
}

void
nbsd_info_proc_mappings_entry (int addr_bit, ULONGEST kve_start,
			       ULONGEST kve_end, ULONGEST kve_offset,
			       int kve_flags, int kve_protection,
			       const char *kve_path)
{
  if (addr_bit == 64)
    {
      gdb_printf ("  %18s %18s %10s %10s %9s %s\n",
		  hex_string (kve_start),
		  hex_string (kve_end),
		  hex_string (kve_end - kve_start),
		  hex_string (kve_offset),
		  nbsd_vm_map_entry_flags (kve_flags, kve_protection),
		  kve_path);
    }
  else
    {
      gdb_printf ("\t%10s %10s %10s %10s %9s %s\n",
		  hex_string (kve_start),
		  hex_string (kve_end),
		  hex_string (kve_end - kve_start),
		  hex_string (kve_offset),
		  nbsd_vm_map_entry_flags (kve_flags, kve_protection),
		  kve_path);
    }
}

/* Implement the "get_syscall_number" gdbarch method.  */

static LONGEST
nbsd_get_syscall_number (struct gdbarch *gdbarch, thread_info *thread)
{

  /* NetBSD doesn't use gdbarch_get_syscall_number since NetBSD
     native targets fetch the system call number from the
     'si_sysnum' member of siginfo_t in nbsd_nat_target::wait.
     However, system call catching requires this function to be
     set.  */

  internal_error (_("nbsd_get_sycall_number called"));
}

/* See netbsd-tdep.h.  */

void
nbsd_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  set_gdbarch_gdb_signal_from_target (gdbarch, nbsd_gdb_signal_from_target);
  set_gdbarch_gdb_signal_to_target (gdbarch, nbsd_gdb_signal_to_target);
  set_gdbarch_skip_solib_resolver (gdbarch, nbsd_skip_solib_resolver);
  set_gdbarch_auxv_parse (gdbarch, svr4_auxv_parse);
  set_gdbarch_get_siginfo_type (gdbarch, nbsd_get_siginfo_type);

  /* `catch syscall' */
  set_xml_syscall_file_name (gdbarch, "syscalls/netbsd.xml");
  set_gdbarch_get_syscall_number (gdbarch, nbsd_get_syscall_number);
}
