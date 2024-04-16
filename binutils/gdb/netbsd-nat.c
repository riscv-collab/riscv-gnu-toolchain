/* Native-dependent code for NetBSD.

   Copyright (C) 2006-2024 Free Software Foundation, Inc.

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

#include "netbsd-nat.h"
#include "nat/netbsd-nat.h"
#include "gdbthread.h"
#include "netbsd-tdep.h"
#include "inferior.h"
#include "gdbarch.h"
#include "gdbsupport/buildargv.h"

#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/sysctl.h>
#include <sys/wait.h>

/* Return the name of a file that can be opened to get the symbols for
   the child process identified by PID.  */

const char *
nbsd_nat_target::pid_to_exec_file (int pid)
{
  return netbsd_nat::pid_to_exec_file (pid);
}

/* Return the current directory for the process identified by PID.  */

static std::string
nbsd_pid_to_cwd (int pid)
{
  char buf[PATH_MAX];
  size_t buflen;
  int mib[4] = {CTL_KERN, KERN_PROC_ARGS, pid, KERN_PROC_CWD};
  buflen = sizeof (buf);
  if (sysctl (mib, ARRAY_SIZE (mib), buf, &buflen, NULL, 0))
    return "";
  return buf;
}

/* Return the kinfo_proc2 structure for the process identified by PID.  */

static bool
nbsd_pid_to_kinfo_proc2 (pid_t pid, struct kinfo_proc2 *kp)
{
  gdb_assert (kp != nullptr);

  size_t size = sizeof (*kp);
  int mib[6] = {CTL_KERN, KERN_PROC2, KERN_PROC_PID, pid,
		static_cast<int> (size), 1};
  return !sysctl (mib, ARRAY_SIZE (mib), kp, &size, NULL, 0);
}

/* Return the command line for the process identified by PID.  */

static gdb::unique_xmalloc_ptr<char[]>
nbsd_pid_to_cmdline (int pid)
{
  int mib[4] = {CTL_KERN, KERN_PROC_ARGS, pid, KERN_PROC_ARGV};

  size_t size = 0;
  if (::sysctl (mib, ARRAY_SIZE (mib), NULL, &size, NULL, 0) == -1 || size == 0)
    return nullptr;

  gdb::unique_xmalloc_ptr<char[]> args (XNEWVAR (char, size));

  if (::sysctl (mib, ARRAY_SIZE (mib), args.get (), &size, NULL, 0) == -1
      || size == 0)
    return nullptr;

  /* Arguments are returned as a flattened string with NUL separators.
     Join the arguments with spaces to form a single string.  */
  for (size_t i = 0; i < size - 1; i++)
    if (args[i] == '\0')
      args[i] = ' ';
  args[size - 1] = '\0';

  return args;
}

/* Return true if PTID is still active in the inferior.  */

bool
nbsd_nat_target::thread_alive (ptid_t ptid)
{
  return netbsd_nat::thread_alive (ptid);
}

/* Return the name assigned to a thread by an application.  Returns
   the string in a static buffer.  */

const char *
nbsd_nat_target::thread_name (struct thread_info *thr)
{
  ptid_t ptid = thr->ptid;
  return netbsd_nat::thread_name (ptid);
}

/* Implement the "post_attach" target_ops method.  */

static void
nbsd_add_threads (nbsd_nat_target *target, pid_t pid)
{
  auto fn
    = [&target] (ptid_t ptid)
      {
	if (!in_thread_list (target, ptid))
	  {
	    if (inferior_ptid.lwp () == 0)
	      thread_change_ptid (target, inferior_ptid, ptid);
	    else
	      add_thread (target, ptid);
	  }
      };

  netbsd_nat::for_each_thread (pid, fn);
}

/* Implement the virtual inf_ptrace_target::post_startup_inferior method.  */

void
nbsd_nat_target::post_startup_inferior (ptid_t ptid)
{
  netbsd_nat::enable_proc_events (ptid.pid ());
}

/* Implement the "post_attach" target_ops method.  */

void
nbsd_nat_target::post_attach (int pid)
{
  netbsd_nat::enable_proc_events (pid);
  nbsd_add_threads (this, pid);
}

/* Implement the "update_thread_list" target_ops method.  */

void
nbsd_nat_target::update_thread_list ()
{
  delete_exited_threads ();
}

/* Convert PTID to a string.  */

std::string
nbsd_nat_target::pid_to_str (ptid_t ptid)
{
  int lwp = ptid.lwp ();

  if (lwp != 0)
    {
      pid_t pid = ptid.pid ();

      return string_printf ("LWP %d of process %d", lwp, pid);
    }

  return normal_pid_to_str (ptid);
}

/* Retrieve all the memory regions in the specified process.  */

static gdb::unique_xmalloc_ptr<struct kinfo_vmentry[]>
nbsd_kinfo_get_vmmap (pid_t pid, size_t *size)
{
  int mib[5] = {CTL_VM, VM_PROC, VM_PROC_MAP, pid,
		sizeof (struct kinfo_vmentry)};

  size_t length = 0;
  if (sysctl (mib, ARRAY_SIZE (mib), NULL, &length, NULL, 0))
    {
      *size = 0;
      return NULL;
    }

  /* Prereserve more space.  The length argument is volatile and can change
     between the sysctl(3) calls as this function can be called against a
     running process.  */
  length = length * 5 / 3;

  gdb::unique_xmalloc_ptr<struct kinfo_vmentry[]> kiv
    (XNEWVAR (kinfo_vmentry, length));

  if (sysctl (mib, ARRAY_SIZE (mib), kiv.get (), &length, NULL, 0))
    {
      *size = 0;
      return NULL;
    }

  *size = length / sizeof (struct kinfo_vmentry);
  return kiv;
}

/* Iterate over all the memory regions in the current inferior,
   calling FUNC for each memory region.  OBFD is passed as the last
   argument to FUNC.  */

int
nbsd_nat_target::find_memory_regions (find_memory_region_ftype func,
				      void *data)
{
  pid_t pid = inferior_ptid.pid ();

  size_t nitems;
  gdb::unique_xmalloc_ptr<struct kinfo_vmentry[]> vmentl
    = nbsd_kinfo_get_vmmap (pid, &nitems);
  if (vmentl == NULL)
    perror_with_name (_("Couldn't fetch VM map entries"));

  for (size_t i = 0; i < nitems; i++)
    {
      struct kinfo_vmentry *kve = &vmentl[i];

      /* Skip unreadable segments and those where MAP_NOCORE has been set.  */
      if (!(kve->kve_protection & KVME_PROT_READ)
	  || kve->kve_flags & KVME_FLAG_NOCOREDUMP)
	continue;

      /* Skip segments with an invalid type.  */
      switch (kve->kve_type)
	{
	case KVME_TYPE_VNODE:
	case KVME_TYPE_ANON:
	case KVME_TYPE_SUBMAP:
	case KVME_TYPE_OBJECT:
	  break;
	default:
	  continue;
	}

      size_t size = kve->kve_end - kve->kve_start;
      if (info_verbose)
	{
	  gdb_printf ("Save segment, %ld bytes at %s (%c%c%c)\n",
		      (long) size,
		      paddress (current_inferior ()->arch  (), kve->kve_start),
		      kve->kve_protection & KVME_PROT_READ ? 'r' : '-',
		      kve->kve_protection & KVME_PROT_WRITE ? 'w' : '-',
		      kve->kve_protection & KVME_PROT_EXEC ? 'x' : '-');
	}

      /* Invoke the callback function to create the corefile segment.
	 Pass MODIFIED as true, we do not know the real modification state.  */
      func (kve->kve_start, size, kve->kve_protection & KVME_PROT_READ,
	    kve->kve_protection & KVME_PROT_WRITE,
	    kve->kve_protection & KVME_PROT_EXEC, 1, false, data);
    }
  return 0;
}

/* Implement the "info_proc" target_ops method.  */

bool
nbsd_nat_target::info_proc (const char *args, enum info_proc_what what)
{
  pid_t pid;
  bool do_cmdline = false;
  bool do_cwd = false;
  bool do_exe = false;
  bool do_mappings = false;
  bool do_status = false;

  switch (what)
    {
    case IP_MINIMAL:
      do_cmdline = true;
      do_cwd = true;
      do_exe = true;
      break;
    case IP_STAT:
    case IP_STATUS:
      do_status = true;
      break;
    case IP_MAPPINGS:
      do_mappings = true;
      break;
    case IP_CMDLINE:
      do_cmdline = true;
      break;
    case IP_EXE:
      do_exe = true;
      break;
    case IP_CWD:
      do_cwd = true;
      break;
    case IP_ALL:
      do_cmdline = true;
      do_cwd = true;
      do_exe = true;
      do_mappings = true;
      do_status = true;
      break;
    default:
      error (_("Not supported on this target."));
    }

  gdb_argv built_argv (args);
  if (built_argv.count () == 0)
    {
      pid = inferior_ptid.pid ();
      if (pid == 0)
	error (_("No current process: you must name one."));
    }
  else if (built_argv.count () == 1 && isdigit (built_argv[0][0]))
    pid = strtol (built_argv[0], NULL, 10);
  else
    error (_("Invalid arguments."));

  gdb_printf (_("process %d\n"), pid);

  if (do_cmdline)
    {
      gdb::unique_xmalloc_ptr<char[]> cmdline = nbsd_pid_to_cmdline (pid);
      if (cmdline != nullptr)
	gdb_printf ("cmdline = '%s'\n", cmdline.get ());
      else
	warning (_("unable to fetch command line"));
    }
  if (do_cwd)
    {
      std::string cwd = nbsd_pid_to_cwd (pid);
      if (cwd != "")
	gdb_printf ("cwd = '%s'\n", cwd.c_str ());
      else
	warning (_("unable to fetch current working directory"));
    }
  if (do_exe)
    {
      const char *exe = pid_to_exec_file (pid);
      if (exe != nullptr)
	gdb_printf ("exe = '%s'\n", exe);
      else
	warning (_("unable to fetch executable path name"));
    }
  if (do_mappings)
    {
      size_t nvment;
      gdb::unique_xmalloc_ptr<struct kinfo_vmentry[]> vmentl
	= nbsd_kinfo_get_vmmap (pid, &nvment);

      if (vmentl != nullptr)
	{
	  int addr_bit = TARGET_CHAR_BIT * sizeof (void *);
	  nbsd_info_proc_mappings_header (addr_bit);

	  struct kinfo_vmentry *kve = vmentl.get ();
	  for (int i = 0; i < nvment; i++, kve++)
	    nbsd_info_proc_mappings_entry (addr_bit, kve->kve_start,
					   kve->kve_end, kve->kve_offset,
					   kve->kve_flags, kve->kve_protection,
					   kve->kve_path);
	}
      else
	warning (_("unable to fetch virtual memory map"));
    }
  if (do_status)
    {
      struct kinfo_proc2 kp;
      if (!nbsd_pid_to_kinfo_proc2 (pid, &kp))
	warning (_("Failed to fetch process information"));
      else
	{
	  auto process_status
	    = [] (int8_t stat)
	      {
		switch (stat)
		  {
		  case SIDL:
		    return "IDL";
		  case SACTIVE:
		    return "ACTIVE";
		  case SDYING:
		    return "DYING";
		  case SSTOP:
		    return "STOP";
		  case SZOMB:
		    return "ZOMB";
		  case SDEAD:
		    return "DEAD";
		  default:
		    return "? (unknown)";
		  }
	      };

	  gdb_printf ("Name: %s\n", kp.p_comm);
	  gdb_printf ("State: %s\n", process_status(kp.p_realstat));
	  gdb_printf ("Parent process: %" PRId32 "\n", kp.p_ppid);
	  gdb_printf ("Process group: %" PRId32 "\n", kp.p__pgid);
	  gdb_printf ("Session id: %" PRId32 "\n", kp.p_sid);
	  gdb_printf ("TTY: %" PRId32 "\n", kp.p_tdev);
	  gdb_printf ("TTY owner process group: %" PRId32 "\n", kp.p_tpgid);
	  gdb_printf ("User IDs (real, effective, saved): "
		      "%" PRIu32 " %" PRIu32 " %" PRIu32 "\n",
		      kp.p_ruid, kp.p_uid, kp.p_svuid);
	  gdb_printf ("Group IDs (real, effective, saved): "
		      "%" PRIu32 " %" PRIu32 " %" PRIu32 "\n",
		      kp.p_rgid, kp.p_gid, kp.p_svgid);

	  gdb_printf ("Groups:");
	  for (int i = 0; i < kp.p_ngroups; i++)
	    gdb_printf (" %" PRIu32, kp.p_groups[i]);
	  gdb_printf ("\n");
	  gdb_printf ("Minor faults (no memory page): %" PRIu64 "\n",
		      kp.p_uru_minflt);
	  gdb_printf ("Major faults (memory page faults): %" PRIu64 "\n",
		      kp.p_uru_majflt);
	  gdb_printf ("utime: %" PRIu32 ".%06" PRIu32 "\n",
		      kp.p_uutime_sec, kp.p_uutime_usec);
	  gdb_printf ("stime: %" PRIu32 ".%06" PRIu32 "\n",
		      kp.p_ustime_sec, kp.p_ustime_usec);
	  gdb_printf ("utime+stime, children: %" PRIu32 ".%06" PRIu32 "\n",
		      kp.p_uctime_sec, kp.p_uctime_usec);
	  gdb_printf ("'nice' value: %" PRIu8 "\n", kp.p_nice);
	  gdb_printf ("Start time: %" PRIu32 ".%06" PRIu32 "\n",
		      kp.p_ustart_sec, kp.p_ustart_usec);
	  int pgtok = getpagesize () / 1024;
	  gdb_printf ("Data size: %" PRIuMAX " kB\n",
		      (uintmax_t) kp.p_vm_dsize * pgtok);
	  gdb_printf ("Stack size: %" PRIuMAX " kB\n",
		      (uintmax_t) kp.p_vm_ssize * pgtok);
	  gdb_printf ("Text size: %" PRIuMAX " kB\n",
		      (uintmax_t) kp.p_vm_tsize * pgtok);
	  gdb_printf ("Resident set size: %" PRIuMAX " kB\n",
		      (uintmax_t) kp.p_vm_rssize * pgtok);
	  gdb_printf ("Maximum RSS: %" PRIu64 " kB\n", kp.p_uru_maxrss);
	  gdb_printf ("Pending Signals:");
	  for (size_t i = 0; i < ARRAY_SIZE (kp.p_siglist.__bits); i++)
	    gdb_printf (" %08" PRIx32, kp.p_siglist.__bits[i]);
	  gdb_printf ("\n");
	  gdb_printf ("Ignored Signals:");
	  for (size_t i = 0; i < ARRAY_SIZE (kp.p_sigignore.__bits); i++)
	    gdb_printf (" %08" PRIx32, kp.p_sigignore.__bits[i]);
	  gdb_printf ("\n");
	  gdb_printf ("Caught Signals:");
	  for (size_t i = 0; i < ARRAY_SIZE (kp.p_sigcatch.__bits); i++)
	    gdb_printf (" %08" PRIx32, kp.p_sigcatch.__bits[i]);
	  gdb_printf ("\n");
	}
    }

  return true;
}

/* Resume execution of a specified PTID, that points to a process or a thread
   within a process.  If one thread is specified, all other threads are
   suspended.  If STEP is nonzero, single-step it.  If SIGNAL is nonzero,
   give it that signal.  */

static void
nbsd_resume(nbsd_nat_target *target, ptid_t ptid, int step,
	    enum gdb_signal signal)
{
  int request;

  gdb_assert (minus_one_ptid != ptid);

  if (ptid.lwp_p ())
    {
      /* If ptid is a specific LWP, suspend all other LWPs in the process.  */
      inferior *inf = find_inferior_ptid (target, ptid);

      for (thread_info *tp : inf->non_exited_threads ())
	{
	  if (tp->ptid.lwp () == ptid.lwp ())
	    request = PT_RESUME;
	  else
	    request = PT_SUSPEND;

	  if (ptrace (request, tp->ptid.pid (), NULL, tp->ptid.lwp ()) == -1)
	    perror_with_name (("ptrace"));
	}
    }
  else
    {
      /* If ptid is a wildcard, resume all matching threads (they won't run
	 until the process is continued however).  */
      for (thread_info *tp : all_non_exited_threads (target, ptid))
	if (ptrace (PT_RESUME, tp->ptid.pid (), NULL, tp->ptid.lwp ()) == -1)
	  perror_with_name (("ptrace"));
    }

  if (step)
    {
      for (thread_info *tp : all_non_exited_threads (target, ptid))
	if (ptrace (PT_SETSTEP, tp->ptid.pid (), NULL, tp->ptid.lwp ()) == -1)
	  perror_with_name (("ptrace"));
    }
  else
    {
      for (thread_info *tp : all_non_exited_threads (target, ptid))
	if (ptrace (PT_CLEARSTEP, tp->ptid.pid (), NULL, tp->ptid.lwp ()) == -1)
	  perror_with_name (("ptrace"));
    }

  if (catch_syscall_enabled ())
    request = PT_SYSCALL;
  else
    request = PT_CONTINUE;

  /* An address of (void *)1 tells ptrace to continue from
     where it was.  If GDB wanted it to start some other way, we have
     already written a new program counter value to the child.  */
  if (ptrace (request, ptid.pid (), (void *)1, gdb_signal_to_host (signal)) == -1)
    perror_with_name (("ptrace"));
}

/* Resume execution of thread PTID, or all threads of all inferiors
   if PTID is -1.  If STEP is nonzero, single-step it.  If SIGNAL is nonzero,
   give it that signal.  */

void
nbsd_nat_target::resume (ptid_t ptid, int step, enum gdb_signal signal)
{
  if (minus_one_ptid != ptid)
    nbsd_resume (this, ptid, step, signal);
  else
    {
      for (inferior *inf : all_non_exited_inferiors (this))
	nbsd_resume (this, ptid_t (inf->pid, 0, 0), step, signal);
    }
}

/* Implement a safe wrapper around waitpid().  */

static pid_t
nbsd_wait (ptid_t ptid, struct target_waitstatus *ourstatus,
	   target_wait_flags options)
{
  pid_t pid;
  int status;

  set_sigint_trap ();

  do
    {
      /* The common code passes WNOHANG that leads to crashes, overwrite it.  */
      pid = waitpid (ptid.pid (), &status, 0);
    }
  while (pid == -1 && errno == EINTR);

  clear_sigint_trap ();

  if (pid == -1)
    perror_with_name (_("Child process unexpectedly missing"));

  *ourstatus = host_status_to_waitstatus (status);
  return pid;
}

/* Wait for the child specified by PTID to do something.  Return the
   process ID of the child, or MINUS_ONE_PTID in case of error; store
   the status in *OURSTATUS.  */

ptid_t
nbsd_nat_target::wait (ptid_t ptid, struct target_waitstatus *ourstatus,
		       target_wait_flags target_options)
{
  pid_t pid = nbsd_wait (ptid, ourstatus, target_options);
  ptid_t wptid = ptid_t (pid);

  /* If the child stopped, keep investigating its status.  */
  if (ourstatus->kind () != TARGET_WAITKIND_STOPPED)
    return wptid;

  /* Extract the event and thread that received a signal.  */
  ptrace_siginfo_t psi;
  if (ptrace (PT_GET_SIGINFO, pid, &psi, sizeof (psi)) == -1)
    perror_with_name (("ptrace"));

  /* Pick child's siginfo_t.  */
  siginfo_t *si = &psi.psi_siginfo;

  int lwp = psi.psi_lwpid;

  int signo = si->si_signo;
  const int code = si->si_code;

  /* Construct PTID with a specified thread that received the event.
     If a signal was targeted to the whole process, lwp is 0.  */
  wptid = ptid_t (pid, lwp, 0);

  /* Bail out on non-debugger oriented signals..  */
  if (signo != SIGTRAP)
    return wptid;

  /* Stop examining non-debugger oriented SIGTRAP codes.  */
  if (code <= SI_USER || code == SI_NOINFO)
    return wptid;

  /* Process state for threading events */
  ptrace_state_t pst = {};
  if (code == TRAP_LWP)
    {
      if (ptrace (PT_GET_PROCESS_STATE, pid, &pst, sizeof (pst)) == -1)
	perror_with_name (("ptrace"));
    }

  if (code == TRAP_LWP && pst.pe_report_event == PTRACE_LWP_EXIT)
    {
      /* If GDB attaches to a multi-threaded process, exiting
	 threads might be skipped during post_attach that
	 have not yet reported their PTRACE_LWP_EXIT event.
	 Ignore exited events for an unknown LWP.  */
      thread_info *thr = this->find_thread (wptid);
      if (thr == nullptr)
	  ourstatus->set_spurious ();
      else
	{
	  /* NetBSD does not store an LWP exit status.  */
	  ourstatus->set_thread_exited (0);

	  delete_thread (thr);
	}

      /* The GDB core expects that the rest of the threads are running.  */
      if (ptrace (PT_CONTINUE, pid, (void *) 1, 0) == -1)
	perror_with_name (("ptrace"));

      return wptid;
    }

  if (in_thread_list (this, ptid_t (pid)))
      thread_change_ptid (this, ptid_t (pid), wptid);

  if (code == TRAP_LWP && pst.pe_report_event == PTRACE_LWP_CREATE)
    {
      /* If GDB attaches to a multi-threaded process, newborn
	 threads might be added by nbsd_add_threads that have
	 not yet reported their PTRACE_LWP_CREATE event.  Ignore
	 born events for an already-known LWP.  */
      if (in_thread_list (this, wptid))
	  ourstatus->set_spurious ();
      else
	{
	  add_thread (this, wptid);
	  ourstatus->set_thread_created ();
	}
      return wptid;
    }

  if (code == TRAP_EXEC)
    {
      ourstatus->set_execd (make_unique_xstrdup (pid_to_exec_file (pid)));
      return wptid;
    }

  if (code == TRAP_TRACE)
    {
      /* Unhandled at this level.  */
      return wptid;
    }

  if (code == TRAP_SCE || code == TRAP_SCX)
    {
      int sysnum = si->si_sysnum;

      if (!catch_syscall_enabled () || !catching_syscall_number (sysnum))
	{
	  /* If the core isn't interested in this event, ignore it.  */
	  ourstatus->set_spurious ();
	  return wptid;
	}

      if (code == TRAP_SCE)
	ourstatus->set_syscall_entry (sysnum);
      else
	ourstatus->set_syscall_return (sysnum);
      return wptid;
    }

  if (code == TRAP_BRKPT)
    {
      /* Unhandled at this level.  */
      return wptid;
    }

  /* Unclassified SIGTRAP event.  */
  ourstatus->set_spurious ();
  return wptid;
}

/* Implement the "insert_exec_catchpoint" target_ops method.  */

int
nbsd_nat_target::insert_exec_catchpoint (int pid)
{
  /* Nothing to do.  */
  return 0;
}

/* Implement the "remove_exec_catchpoint" target_ops method.  */

int
nbsd_nat_target::remove_exec_catchpoint (int pid)
{
  /* Nothing to do.  */
  return 0;
}

/* Implement the "set_syscall_catchpoint" target_ops method.  */

int
nbsd_nat_target::set_syscall_catchpoint (int pid, bool needed,
					 int any_count,
					 gdb::array_view<const int> syscall_counts)
{
  /* Ignore the arguments.  inf-ptrace.c will use PT_SYSCALL which
     will catch all system call entries and exits.  The system calls
     are filtered by GDB rather than the kernel.  */
  return 0;
}

/* Implement the "supports_multi_process" target_ops method. */

bool
nbsd_nat_target::supports_multi_process ()
{
  return true;
}

/* Implement the "xfer_partial" target_ops method.  */

enum target_xfer_status
nbsd_nat_target::xfer_partial (enum target_object object,
			       const char *annex, gdb_byte *readbuf,
			       const gdb_byte *writebuf,
			       ULONGEST offset, ULONGEST len,
			       ULONGEST *xfered_len)
{
  pid_t pid = inferior_ptid.pid ();

  switch (object)
    {
    case TARGET_OBJECT_SIGNAL_INFO:
      {
	len = netbsd_nat::qxfer_siginfo(pid, annex, readbuf, writebuf, offset,
					len);

	if (len == -1)
	  return TARGET_XFER_E_IO;

	*xfered_len = len;
	return TARGET_XFER_OK;
      }
    case TARGET_OBJECT_MEMORY:
      {
	size_t xfered;
	int res;
	if (writebuf != nullptr)
	  res = netbsd_nat::write_memory (pid, writebuf, offset, len, &xfered);
	else
	  res = netbsd_nat::read_memory (pid, readbuf, offset, len, &xfered);
	if (res != 0)
	  {
	    if (res == EACCES)
	      gdb_printf (gdb_stderr, "Cannot %s process at %s (%s). "
			  "Is PaX MPROTECT active? See security(7), "
			  "sysctl(7), paxctl(8)\n",
			  (writebuf ? "write to" : "read from"),
			  pulongest (offset), safe_strerror (errno));
	    return TARGET_XFER_E_IO;
	  }
	if (xfered == 0)
	  return TARGET_XFER_EOF;
	*xfered_len = (ULONGEST) xfered;
	return TARGET_XFER_OK;
      }
    default:
      return inf_ptrace_target::xfer_partial (object, annex,
					      readbuf, writebuf, offset,
					      len, xfered_len);
    }
}

/* Implement the "supports_dumpcore" target_ops method.  */

bool
nbsd_nat_target::supports_dumpcore ()
{
  return true;
}

/* Implement the "dumpcore" target_ops method.  */

void
nbsd_nat_target::dumpcore (const char *filename)
{
  pid_t pid = inferior_ptid.pid ();

  if (ptrace (PT_DUMPCORE, pid, const_cast<char *>(filename),
	      strlen (filename)) == -1)
    perror_with_name (("ptrace"));
}
