/* Copyright (C) 2020-2024 Free Software Foundation, Inc.

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

#include "server.h"
#include "target.h"
#include "netbsd-low.h"
#include "nat/netbsd-nat.h"

#include <sys/param.h>
#include <sys/types.h>

#include <sys/ptrace.h>
#include <sys/sysctl.h>

#include <limits.h>
#include <unistd.h>
#include <signal.h>

#include <elf.h>

#include <type_traits>

#include "gdbsupport/eintr.h"
#include "gdbsupport/gdb_wait.h"
#include "gdbsupport/filestuff.h"
#include "gdbsupport/common-inferior.h"
#include "nat/fork-inferior.h"
#include "hostio.h"

int using_threads = 1;

/* Callback used by fork_inferior to start tracing the inferior.  */

static void
netbsd_ptrace_fun ()
{
  /* Switch child to its own process group so that signals won't
     directly affect GDBserver. */
  if (setpgid (0, 0) < 0)
    trace_start_error_with_name (("setpgid"));

  if (ptrace (PT_TRACE_ME, 0, nullptr, 0) < 0)
    trace_start_error_with_name (("ptrace"));

  /* If GDBserver is connected to gdb via stdio, redirect the inferior's
     stdout to stderr so that inferior i/o doesn't corrupt the connection.
     Also, redirect stdin to /dev/null.  */
  if (remote_connection_is_stdio ())
    {
      if (close (0) < 0)
	trace_start_error_with_name (("close"));
      if (open ("/dev/null", O_RDONLY) < 0)
	trace_start_error_with_name (("open"));
      if (dup2 (2, 1) < 0)
	trace_start_error_with_name (("dup2"));
      if (write (2, "stdin/stdout redirected\n",
		 sizeof ("stdin/stdout redirected\n") - 1) < 0)
	{
	  /* Errors ignored.  */
	}
    }
}

/* Implement the create_inferior method of the target_ops vector.  */

int
netbsd_process_target::create_inferior (const char *program,
					const std::vector<char *> &program_args)
{
  std::string str_program_args = construct_inferior_arguments (program_args);

  pid_t pid = fork_inferior (program, str_program_args.c_str (),
			     get_environ ()->envp (), netbsd_ptrace_fun,
			     nullptr, nullptr, nullptr, nullptr);

  add_process (pid, 0);

  post_fork_inferior (pid, program);

  return pid;
}

/* Implement the post_create_inferior target_ops method.  */

void
netbsd_process_target::post_create_inferior ()
{
  pid_t pid = current_process ()->pid;
  netbsd_nat::enable_proc_events (pid);

  low_arch_setup ();
}

/* Implement the attach target_ops method.  */

int
netbsd_process_target::attach (unsigned long pid)
{
  /* Unimplemented.  */
  return -1;
}

/* Returns true if GDB is interested in any child syscalls.  */

static bool
gdb_catching_syscalls_p (pid_t pid)
{
  struct process_info *proc = find_process_pid (pid);
  return !proc->syscalls_to_catch.empty ();
}

/* Implement the resume target_ops method.  */

void
netbsd_process_target::resume (struct thread_resume *resume_info, size_t n)
{
  ptid_t resume_ptid = resume_info[0].thread;
  const int signal = resume_info[0].sig;
  const bool step = resume_info[0].kind == resume_step;

  if (resume_ptid == minus_one_ptid)
    resume_ptid = ptid_of (current_thread);

  const pid_t pid = resume_ptid.pid ();
  const lwpid_t lwp = resume_ptid.lwp ();
  regcache_invalidate_pid (pid);

  auto fn
    = [&] (ptid_t ptid)
      {
	if (step)
	  {
	    if (ptid.lwp () == lwp || n != 1)
	      {
		if (ptrace (PT_SETSTEP, pid, NULL, ptid.lwp ()) == -1)
		  perror_with_name (("ptrace"));
		if (ptrace (PT_RESUME, pid, NULL, ptid.lwp ()) == -1)
		  perror_with_name (("ptrace"));
	      }
	    else
	      {
		if (ptrace (PT_CLEARSTEP, pid, NULL, ptid.lwp ()) == -1)
		  perror_with_name (("ptrace"));
		if (ptrace (PT_SUSPEND, pid, NULL, ptid.lwp ()) == -1)
		  perror_with_name (("ptrace"));
	      }
	  }
	else
	  {
	    if (ptrace (PT_CLEARSTEP, pid, NULL, ptid.lwp ()) == -1)
	      perror_with_name (("ptrace"));
	    if (ptrace (PT_RESUME, pid, NULL, ptid.lwp ()) == -1)
	      perror_with_name (("ptrace"));
	  }
      };

  netbsd_nat::for_each_thread (pid, fn);

  int request = gdb_catching_syscalls_p (pid) ? PT_CONTINUE : PT_SYSCALL;

  errno = 0;
  ptrace (request, pid, (void *)1, signal);
  if (errno)
    perror_with_name (("ptrace"));
}

/* Returns true if GDB is interested in the reported SYSNO syscall.  */

static bool
netbsd_catch_this_syscall (int sysno)
{
  struct process_info *proc = current_process ();

  if (proc->syscalls_to_catch.empty ())
    return false;

  if (proc->syscalls_to_catch[0] == ANY_SYSCALL)
    return true;

  for (int iter : proc->syscalls_to_catch)
    if (iter == sysno)
      return true;

  return false;
}

/* Helper function for child_wait and the derivatives of child_wait.
   HOSTSTATUS is the waitstatus from wait() or the equivalent; store our
   translation of that in OURSTATUS.  */

static void
netbsd_store_waitstatus (struct target_waitstatus *ourstatus, int hoststatus)
{
  if (WIFEXITED (hoststatus))
    ourstatus->set_exited (WEXITSTATUS (hoststatus));
  else if (!WIFSTOPPED (hoststatus))
    ourstatus->set_signalled (gdb_signal_from_host (WTERMSIG (hoststatus)));
  else
    ourstatus->set_stopped (gdb_signal_from_host (WSTOPSIG (hoststatus)));
}

/* Implement a safe wrapper around waitpid().  */

static pid_t
netbsd_waitpid (ptid_t ptid, struct target_waitstatus *ourstatus,
		target_wait_flags target_options)
{
  int status;
  int options = (target_options & TARGET_WNOHANG) ? WNOHANG : 0;

  pid_t pid
    = gdb::handle_eintr (-1, ::waitpid, ptid.pid (), &status, options);

  if (pid == -1)
    perror_with_name (_("Child process unexpectedly missing"));

  netbsd_store_waitstatus (ourstatus, status);
  return pid;
}


/* Implement the wait target_ops method.

   Wait for the child specified by PTID to do something.  Return the
   process ID of the child, or MINUS_ONE_PTID in case of error; store
   the status in *OURSTATUS.  */

static ptid_t
netbsd_wait (ptid_t ptid, struct target_waitstatus *ourstatus,
	     target_wait_flags target_options)
{
  pid_t pid = netbsd_waitpid (ptid, ourstatus, target_options);
  ptid_t wptid = ptid_t (pid);

  if (pid == 0)
    {
      gdb_assert (target_options & TARGET_WNOHANG);
      ourstatus->set_ignore ();
      return null_ptid;
    }

  gdb_assert (pid != -1);

  /* If the child stopped, keep investigating its status.  */
  if (ourstatus->kind () != TARGET_WAITKIND_STOPPED)
    return wptid;

  /* Extract the event and thread that received a signal.  */
  ptrace_siginfo_t psi;
  if (ptrace (PT_GET_SIGINFO, pid, &psi, sizeof (psi)) == -1)
    perror_with_name (("ptrace"));

  /* Pick child's siginfo_t.  */
  siginfo_t *si = &psi.psi_siginfo;

  lwpid_t lwp = psi.psi_lwpid;

  int signo = si->si_signo;
  const int code = si->si_code;

  /* Construct PTID with a specified thread that received the event.
     If a signal was targeted to the whole process, lwp is 0.  */
  wptid = ptid_t (pid, lwp, 0);

  /* Bail out on non-debugger oriented signals.  */
  if (signo != SIGTRAP)
    return wptid;

  /* Stop examining non-debugger oriented SIGTRAP codes.  */
  if (code <= SI_USER || code == SI_NOINFO)
    return wptid;

  /* Process state for threading events.  */
  ptrace_state_t pst = {};
  if (code == TRAP_LWP)
    if (ptrace (PT_GET_PROCESS_STATE, pid, &pst, sizeof (pst)) == -1)
      perror_with_name (("ptrace"));

  if (code == TRAP_LWP && pst.pe_report_event == PTRACE_LWP_EXIT)
    {
      /* If GDB attaches to a multi-threaded process, exiting
	 threads might be skipped during post_attach that
	 have not yet reported their PTRACE_LWP_EXIT event.
	 Ignore exited events for an unknown LWP.  */
      thread_info *thr = find_thread_ptid (wptid);
      if (thr == nullptr)
	  ourstatus->set_spurious ();
      else
	{
	  /* NetBSD does not store an LWP exit status.  */
	  ourstatus->set_thread_exited (0);

	  remove_thread (thr);
	}
      return wptid;
    }

  if (find_thread_ptid (ptid_t (pid)))
    switch_to_thread (find_thread_ptid (wptid));

  if (code == TRAP_LWP && pst.pe_report_event == PTRACE_LWP_CREATE)
    {
      /* If GDB attaches to a multi-threaded process, newborn
	 threads might be added by nbsd_add_threads that have
	 not yet reported their PTRACE_LWP_CREATE event.  Ignore
	 born events for an already-known LWP.  */
      if (find_thread_ptid (wptid))
	ourstatus->set_spurious ();
      else
	{
	  add_thread (wptid, NULL);
	  ourstatus->set_thread_created ();
	}
      return wptid;
    }

  if (code == TRAP_EXEC)
    {
      ourstatus->set_execd
	(make_unique_xstrdup (netbsd_nat::pid_to_exec_file (pid)));
      return wptid;
    }

  if (code == TRAP_TRACE)
      return wptid;

  if (code == TRAP_SCE || code == TRAP_SCX)
    {
      int sysnum = si->si_sysnum;

      if (!netbsd_catch_this_syscall(sysnum))
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
#ifdef PTRACE_BREAKPOINT_ADJ
      CORE_ADDR pc;
      struct reg r;
      ptrace (PT_GETREGS, pid, &r, psi.psi_lwpid);
      pc = PTRACE_REG_PC (&r);
      PTRACE_REG_SET_PC (&r, pc - PTRACE_BREAKPOINT_ADJ);
      ptrace (PT_SETREGS, pid, &r, psi.psi_lwpid);
#endif
      return wptid;
    }

  /* Unclassified SIGTRAP event.  */
  ourstatus->set_spurious ();
  return wptid;
}

/* Implement the wait target_ops method.  */

ptid_t
netbsd_process_target::wait (ptid_t ptid, struct target_waitstatus *ourstatus,
			     target_wait_flags target_options)
{
  while (true)
    {
      ptid_t wptid = netbsd_wait (ptid, ourstatus, target_options);

      /* Register thread in the gdbcore if a thread was not reported earlier.
	 This is required after ::create_inferior, when the gdbcore does not
	 know about the first internal thread.
	 This may also happen on attach, when an event is registered on a thread
	 that was not fully initialized during the attach stage.  */
      if (wptid.lwp () != 0 && !find_thread_ptid (wptid)
	  && ourstatus->kind () != TARGET_WAITKIND_THREAD_EXITED)
	add_thread (wptid, nullptr);

      switch (ourstatus->kind ())
	{
	case TARGET_WAITKIND_EXITED:
	case TARGET_WAITKIND_STOPPED:
	case TARGET_WAITKIND_SIGNALLED:
	case TARGET_WAITKIND_FORKED:
	case TARGET_WAITKIND_VFORKED:
	case TARGET_WAITKIND_EXECD:
	case TARGET_WAITKIND_VFORK_DONE:
	case TARGET_WAITKIND_SYSCALL_ENTRY:
	case TARGET_WAITKIND_SYSCALL_RETURN:
	  /* Pass the result to the generic code.  */
	  return wptid;
	case TARGET_WAITKIND_THREAD_CREATED:
	case TARGET_WAITKIND_THREAD_EXITED:
	  /* The core needlessly stops on these events.  */
	  [[fallthrough]];
	case TARGET_WAITKIND_SPURIOUS:
	  /* Spurious events are unhandled by the gdbserver core.  */
	  if (ptrace (PT_CONTINUE, current_process ()->pid, (void *) 1, 0)
	      == -1)
	    perror_with_name (("ptrace"));
	  break;
	default:
	  error (("Unknown stopped status"));
	}
    }
}

/* Implement the kill target_ops method.  */

int
netbsd_process_target::kill (process_info *process)
{
  pid_t pid = process->pid;
  if (ptrace (PT_KILL, pid, nullptr, 0) == -1)
    return -1;

  int status;
  if (gdb::handle_eintr (-1, ::waitpid, pid, &status, 0) == -1)
    return -1;
  mourn (process);
  return 0;
}

/* Implement the detach target_ops method.  */

int
netbsd_process_target::detach (process_info *process)
{
  pid_t pid = process->pid;

  ptrace (PT_DETACH, pid, (void *) 1, 0);
  mourn (process);
  return 0;
}

/* Implement the mourn target_ops method.  */

void
netbsd_process_target::mourn (struct process_info *proc)
{
  for_each_thread (proc->pid, remove_thread);

  remove_process (proc);
}

/* Implement the join target_ops method.  */

void
netbsd_process_target::join (int pid)
{
  /* The PT_DETACH is sufficient to detach from the process.
     So no need to do anything extra.  */
}

/* Implement the thread_alive target_ops method.  */

bool
netbsd_process_target::thread_alive (ptid_t ptid)
{
  return netbsd_nat::thread_alive (ptid);
}

/* Implement the fetch_registers target_ops method.  */

void
netbsd_process_target::fetch_registers (struct regcache *regcache, int regno)
{
  const netbsd_regset_info *regset = get_regs_info ();
  ptid_t inferior_ptid = ptid_of (current_thread);

  while (regset->size >= 0)
    {
      std::vector<char> buf;
      buf.resize (regset->size);
      int res = ptrace (regset->get_request, inferior_ptid.pid (), buf.data (),
			inferior_ptid.lwp ());
      if (res == -1)
	perror_with_name (("ptrace"));
      regset->store_function (regcache, buf.data ());
      regset++;
    }
}

/* Implement the store_registers target_ops method.  */

void
netbsd_process_target::store_registers (struct regcache *regcache, int regno)
{
  const netbsd_regset_info *regset = get_regs_info ();
  ptid_t inferior_ptid = ptid_of (current_thread);

  while (regset->size >= 0)
    {
      std::vector<char> buf;
      buf.resize (regset->size);
      int res = ptrace (regset->get_request, inferior_ptid.pid (), buf.data (),
			inferior_ptid.lwp ());
      if (res == -1)
	perror_with_name (("ptrace"));

      /* Then overlay our cached registers on that.  */
      regset->fill_function (regcache, buf.data ());
      /* Only now do we write the register set.  */
      res = ptrace (regset->set_request, inferior_ptid.pid (), buf. data (),
		    inferior_ptid.lwp ());
      if (res == -1)
	perror_with_name (("ptrace"));
      regset++;
    }
}

/* Implement the read_memory target_ops method.  */

int
netbsd_process_target::read_memory (CORE_ADDR memaddr, unsigned char *myaddr,
				    int size)
{
  pid_t pid = current_process ()->pid;
  return netbsd_nat::read_memory (pid, myaddr, memaddr, size, nullptr);
}

/* Implement the write_memory target_ops method.  */

int
netbsd_process_target::write_memory (CORE_ADDR memaddr,
				     const unsigned char *myaddr, int size)
{
  pid_t pid = current_process ()->pid;
  return netbsd_nat::write_memory (pid, myaddr, memaddr, size, nullptr);
}

/* Implement the request_interrupt target_ops method.  */

void
netbsd_process_target::request_interrupt ()
{
  ptid_t inferior_ptid = ptid_of (get_first_thread ());

  ::kill (inferior_ptid.pid (), SIGINT);
}

/* Read the AUX Vector for the specified PID, wrapping the ptrace(2) call
   with the PIOD_READ_AUXV operation and using the PT_IO standard input
   and output arguments.  */

static size_t
netbsd_read_auxv(pid_t pid, void *offs, void *addr, size_t len)
{
  struct ptrace_io_desc pio;

  pio.piod_op = PIOD_READ_AUXV;
  pio.piod_offs = offs;
  pio.piod_addr = addr;
  pio.piod_len = len;

  if (ptrace (PT_IO, pid, &pio, 0) == -1)
    perror_with_name (("ptrace"));

  return pio.piod_len;
}

/* Copy LEN bytes from inferior's auxiliary vector starting at OFFSET
   to debugger memory starting at MYADDR.  */

int
netbsd_process_target::read_auxv (int pid, CORE_ADDR offset,
				  unsigned char *myaddr, unsigned int len)
{
  return netbsd_read_auxv (pid, (void *) (intptr_t) offset, myaddr, len);
}

bool
netbsd_process_target::supports_z_point_type (char z_type)
{
  switch (z_type)
    {
    case Z_PACKET_SW_BP:
      return true;
    case Z_PACKET_HW_BP:
    case Z_PACKET_WRITE_WP:
    case Z_PACKET_READ_WP:
    case Z_PACKET_ACCESS_WP:
    default:
      return false; /* Not supported.  */
    }
}

/* Insert {break/watch}point at address ADDR.  SIZE is not used.  */

int
netbsd_process_target::insert_point (enum raw_bkpt_type type, CORE_ADDR addr,
		     int size, struct raw_breakpoint *bp)
{
  switch (type)
    {
    case raw_bkpt_type_sw:
      return insert_memory_breakpoint (bp);
    case raw_bkpt_type_hw:
    case raw_bkpt_type_write_wp:
    case raw_bkpt_type_read_wp:
    case raw_bkpt_type_access_wp:
    default:
      return 1; /* Not supported.  */
    }
}

/* Remove {break/watch}point at address ADDR.  SIZE is not used.  */

int
netbsd_process_target::remove_point (enum raw_bkpt_type type, CORE_ADDR addr,
				     int size, struct raw_breakpoint *bp)
{
  switch (type)
    {
    case raw_bkpt_type_sw:
      return remove_memory_breakpoint (bp);
    case raw_bkpt_type_hw:
    case raw_bkpt_type_write_wp:
    case raw_bkpt_type_read_wp:
    case raw_bkpt_type_access_wp:
    default:
      return 1; /* Not supported.  */
    }
}

/* Implement the stopped_by_sw_breakpoint target_ops method.  */

bool
netbsd_process_target::stopped_by_sw_breakpoint ()
{
  ptrace_siginfo_t psi;
  pid_t pid = current_process ()->pid;

  if (ptrace (PT_GET_SIGINFO, pid, &psi, sizeof (psi)) == -1)
    perror_with_name (("ptrace"));

  return psi.psi_siginfo.si_signo == SIGTRAP &&
	 psi.psi_siginfo.si_code == TRAP_BRKPT;
}

/* Implement the supports_stopped_by_sw_breakpoint target_ops method.  */

bool
netbsd_process_target::supports_stopped_by_sw_breakpoint ()
{
  return true;
}

/* Implement the supports_qxfer_siginfo target_ops method.  */

bool
netbsd_process_target::supports_qxfer_siginfo ()
{
  return true;
}

/* Implement the qxfer_siginfo target_ops method.  */

int
netbsd_process_target::qxfer_siginfo (const char *annex, unsigned char *readbuf,
				      unsigned const char *writebuf,
				      CORE_ADDR offset, int len)
{
  if (current_thread == nullptr)
    return -1;

  pid_t pid = current_process ()->pid;

  return netbsd_nat::qxfer_siginfo(pid, annex, readbuf, writebuf, offset, len);
}

/* Implement the supports_non_stop target_ops method.  */

bool
netbsd_process_target::supports_non_stop ()
{
  return false;
}

/* Implement the supports_multi_process target_ops method.  */

bool
netbsd_process_target::supports_multi_process ()
{
  return true;
}

/* Check if fork events are supported.  */

bool
netbsd_process_target::supports_fork_events ()
{
  return false;
}

/* Check if vfork events are supported.  */

bool
netbsd_process_target::supports_vfork_events ()
{
  return false;
}

/* Check if exec events are supported.  */

bool
netbsd_process_target::supports_exec_events ()
{
  return true;
}

/* Implement the supports_disable_randomization target_ops method.  */

bool
netbsd_process_target::supports_disable_randomization ()
{
  return false;
}

/* Extract &phdr and num_phdr in the inferior.  Return 0 on success.  */

template <typename T>
int get_phdr_phnum_from_proc_auxv (const pid_t pid,
				   CORE_ADDR *phdr_memaddr, int *num_phdr)
{
  typedef typename std::conditional<sizeof(T) == sizeof(int64_t),
				    Aux64Info, Aux32Info>::type auxv_type;
  const size_t auxv_size = sizeof (auxv_type);
  const size_t auxv_buf_size = 128 * sizeof (auxv_type);

  std::vector<char> auxv_buf;
  auxv_buf.resize (auxv_buf_size);

  netbsd_read_auxv (pid, nullptr, auxv_buf.data (), auxv_buf_size);

  *phdr_memaddr = 0;
  *num_phdr = 0;

  for (char *buf = auxv_buf.data ();
       buf < (auxv_buf.data () + auxv_buf_size);
       buf += auxv_size)
    {
      auxv_type *const aux = (auxv_type *) buf;

      switch (aux->a_type)
	{
	case AT_PHDR:
	  *phdr_memaddr = aux->a_v;
	  break;
	case AT_PHNUM:
	  *num_phdr = aux->a_v;
	  break;
	}

      if (*phdr_memaddr != 0 && *num_phdr != 0)
	break;
    }

  if (*phdr_memaddr == 0 || *num_phdr == 0)
    {
      warning ("Unexpected missing AT_PHDR and/or AT_PHNUM: "
	       "phdr_memaddr = %s, phdr_num = %d",
	       core_addr_to_string (*phdr_memaddr), *num_phdr);
      return 2;
    }

  return 0;
}

/* Return &_DYNAMIC (via PT_DYNAMIC) in the inferior, or 0 if not present.  */

template <typename T>
static CORE_ADDR
get_dynamic (const pid_t pid)
{
  typedef typename std::conditional<sizeof(T) == sizeof(int64_t),
				    Elf64_Phdr, Elf32_Phdr>::type phdr_type;
  const int phdr_size = sizeof (phdr_type);

  CORE_ADDR phdr_memaddr;
  int num_phdr;
  if (get_phdr_phnum_from_proc_auxv<T> (pid, &phdr_memaddr, &num_phdr))
    return 0;

  std::vector<unsigned char> phdr_buf;
  phdr_buf.resize (num_phdr * phdr_size);

  if (netbsd_nat::read_memory (pid, phdr_buf.data (), phdr_memaddr,
			       phdr_buf.size (), nullptr))
    return 0;

  /* Compute relocation: it is expected to be 0 for "regular" executables,
     non-zero for PIE ones.  */
  CORE_ADDR relocation = -1;
  for (int i = 0; relocation == -1 && i < num_phdr; i++)
    {
      phdr_type *const p = (phdr_type *) (phdr_buf.data () + i * phdr_size);

      if (p->p_type == PT_PHDR)
	relocation = phdr_memaddr - p->p_vaddr;
    }

  if (relocation == -1)
    {
      /* PT_PHDR is optional, but necessary for PIE in general.  Fortunately
	 any real world executables, including PIE executables, have always
	 PT_PHDR present.  PT_PHDR is not present in some shared libraries or
	 in fpc (Free Pascal 2.4) binaries but neither of those have a need for
	 or present DT_DEBUG anyway (fpc binaries are statically linked).

	 Therefore if there exists DT_DEBUG there is always also PT_PHDR.

	 GDB could find RELOCATION also from AT_ENTRY - e_entry.  */

      return 0;
    }

  for (int i = 0; i < num_phdr; i++)
    {
      phdr_type *const p = (phdr_type *) (phdr_buf.data () + i * phdr_size);

      if (p->p_type == PT_DYNAMIC)
	return p->p_vaddr + relocation;
    }

  return 0;
}

/* Return &_r_debug in the inferior, or -1 if not present.  Return value
   can be 0 if the inferior does not yet have the library list initialized.
   We look for DT_MIPS_RLD_MAP first.  MIPS executables use this instead of
   DT_DEBUG, although they sometimes contain an unused DT_DEBUG entry too.  */

template <typename T>
static CORE_ADDR
get_r_debug (const pid_t pid)
{
  typedef typename std::conditional<sizeof(T) == sizeof(int64_t),
				    Elf64_Dyn, Elf32_Dyn>::type dyn_type;
  const int dyn_size = sizeof (dyn_type);
  unsigned char buf[sizeof (dyn_type)];  /* The larger of the two.  */
  CORE_ADDR map = -1;

  CORE_ADDR dynamic_memaddr = get_dynamic<T> (pid);
  if (dynamic_memaddr == 0)
    return map;

  while (netbsd_nat::read_memory (pid, buf, dynamic_memaddr, dyn_size, nullptr)
	 == 0)
    {
      dyn_type *const dyn = (dyn_type *) buf;
#if defined DT_MIPS_RLD_MAP
      union
      {
	T map;
	unsigned char buf[sizeof (T)];
      }
      rld_map;

      if (dyn->d_tag == DT_MIPS_RLD_MAP)
	{
	  if (netbsd_nat::read_memory (pid, rld_map.buf, dyn->d_un.d_val,
				       sizeof (rld_map.buf), nullptr) == 0)
	    return rld_map.map;
	  else
	    break;
	}
#endif  /* DT_MIPS_RLD_MAP */

      if (dyn->d_tag == DT_DEBUG && map == -1)
	map = dyn->d_un.d_val;

      if (dyn->d_tag == DT_NULL)
	break;

      dynamic_memaddr += dyn_size;
    }

  return map;
}

/* Read one pointer from MEMADDR in the inferior.  */

static int
read_one_ptr (const pid_t pid, CORE_ADDR memaddr, CORE_ADDR *ptr, int ptr_size)
{
  /* Go through a union so this works on either big or little endian
     hosts, when the inferior's pointer size is smaller than the size
     of CORE_ADDR.  It is assumed the inferior's endianness is the
     same of the superior's.  */

  union
  {
    CORE_ADDR core_addr;
    unsigned int ui;
    unsigned char uc;
  } addr;

  int ret = netbsd_nat::read_memory (pid, &addr.uc, memaddr, ptr_size, nullptr);
  if (ret == 0)
    {
      if (ptr_size == sizeof (CORE_ADDR))
	*ptr = addr.core_addr;
      else if (ptr_size == sizeof (unsigned int))
	*ptr = addr.ui;
      else
	gdb_assert_not_reached ("unhandled pointer size");
    }
  return ret;
}

/* Construct qXfer:libraries-svr4:read reply.  */

template <typename T>
int
netbsd_qxfer_libraries_svr4 (const pid_t pid, const char *annex,
			     unsigned char *readbuf,
			     unsigned const char *writebuf,
			     CORE_ADDR offset, int len)
{
  struct link_map_offsets
  {
    /* Offset and size of r_debug.r_version.  */
    int r_version_offset;

    /* Offset and size of r_debug.r_map.  */
    int r_map_offset;

    /* Offset to l_addr field in struct link_map.  */
    int l_addr_offset;

    /* Offset to l_name field in struct link_map.  */
    int l_name_offset;

    /* Offset to l_ld field in struct link_map.  */
    int l_ld_offset;

    /* Offset to l_next field in struct link_map.  */
    int l_next_offset;

    /* Offset to l_prev field in struct link_map.  */
    int l_prev_offset;
  };

  static const struct link_map_offsets lmo_32bit_offsets =
    {
      0,     /* r_version offset. */
      4,     /* r_debug.r_map offset.  */
      0,     /* l_addr offset in link_map.  */
      4,     /* l_name offset in link_map.  */
      8,     /* l_ld offset in link_map.  */
      12,    /* l_next offset in link_map.  */
      16     /* l_prev offset in link_map.  */
    };

  static const struct link_map_offsets lmo_64bit_offsets =
    {
      0,     /* r_version offset. */
      8,     /* r_debug.r_map offset.  */
      0,     /* l_addr offset in link_map.  */
      8,     /* l_name offset in link_map.  */
      16,    /* l_ld offset in link_map.  */
      24,    /* l_next offset in link_map.  */
      32     /* l_prev offset in link_map.  */
    };

  CORE_ADDR lm_addr = 0, lm_prev = 0;
  CORE_ADDR l_name, l_addr, l_ld, l_next, l_prev;
  int header_done = 0;

  const struct link_map_offsets *lmo
    = ((sizeof (T) == sizeof (int64_t))
       ? &lmo_64bit_offsets : &lmo_32bit_offsets);
  int ptr_size = sizeof (T);

  while (annex[0] != '\0')
    {
      const char *sep = strchr (annex, '=');
      if (sep == nullptr)
	break;

      int name_len = sep - annex;
      CORE_ADDR *addrp;
      if (name_len == 5 && startswith (annex, "start"))
	addrp = &lm_addr;
      else if (name_len == 4 && startswith (annex, "prev"))
	addrp = &lm_prev;
      else
	{
	  annex = strchr (sep, ';');
	  if (annex == nullptr)
	    break;
	  annex++;
	  continue;
	}

      annex = decode_address_to_semicolon (addrp, sep + 1);
    }

  if (lm_addr == 0)
    {
      CORE_ADDR r_debug = get_r_debug<T> (pid);

      /* We failed to find DT_DEBUG.  Such situation will not change
	 for this inferior - do not retry it.  Report it to GDB as
	 E01, see for the reasons at the GDB solib-svr4.c side.  */
      if (r_debug == (CORE_ADDR) -1)
	return -1;

      if (r_debug != 0)
	{
	  CORE_ADDR map_offset = r_debug + lmo->r_map_offset;
	  if (read_one_ptr (pid, map_offset, &lm_addr, ptr_size) != 0)
	    warning ("unable to read r_map from %s",
		     core_addr_to_string (map_offset));
	}
    }

  std::string document = "<library-list-svr4 version=\"1.0\"";

  while (lm_addr
	 && read_one_ptr (pid, lm_addr + lmo->l_name_offset,
			  &l_name, ptr_size) == 0
	 && read_one_ptr (pid, lm_addr + lmo->l_addr_offset,
			  &l_addr, ptr_size) == 0
	 && read_one_ptr (pid, lm_addr + lmo->l_ld_offset,
			  &l_ld, ptr_size) == 0
	 && read_one_ptr (pid, lm_addr + lmo->l_prev_offset,
			  &l_prev, ptr_size) == 0
	 && read_one_ptr (pid, lm_addr + lmo->l_next_offset,
			  &l_next, ptr_size) == 0)
    {
      if (lm_prev != l_prev)
	{
	  warning ("Corrupted shared library list: 0x%lx != 0x%lx",
		   (long) lm_prev, (long) l_prev);
	  break;
	}

      /* Ignore the first entry even if it has valid name as the first entry
	 corresponds to the main executable.  The first entry should not be
	 skipped if the dynamic loader was loaded late by a static executable
	 (see solib-svr4.c parameter ignore_first).  But in such case the main
	 executable does not have PT_DYNAMIC present and this function already
	 exited above due to failed get_r_debug.  */
      if (lm_prev == 0)
	string_appendf (document, " main-lm=\"0x%lx\"",
			(unsigned long) lm_addr);
      else
	{
	  unsigned char libname[PATH_MAX];

	  /* Not checking for error because reading may stop before
	     we've got PATH_MAX worth of characters.  */
	  libname[0] = '\0';
	  netbsd_nat::read_memory (pid, libname, l_name, sizeof (libname) - 1,
				   nullptr);
	  libname[sizeof (libname) - 1] = '\0';
	  if (libname[0] != '\0')
	    {
	      if (!header_done)
		{
		  /* Terminate `<library-list-svr4'.  */
		  document += '>';
		  header_done = 1;
		}

	      string_appendf (document, "<library name=\"");
	      xml_escape_text_append (document, (char *) libname);
	      string_appendf (document, "\" lm=\"0x%lx\" "
			      "l_addr=\"0x%lx\" l_ld=\"0x%lx\"/>",
			      (unsigned long) lm_addr, (unsigned long) l_addr,
			      (unsigned long) l_ld);
	    }
	}

      lm_prev = lm_addr;
      lm_addr = l_next;
    }

  if (!header_done)
    {
      /* Empty list; terminate `<library-list-svr4'.  */
      document += "/>";
    }
  else
    document += "</library-list-svr4>";

  int document_len = document.length ();
  if (offset < document_len)
    document_len -= offset;
  else
    document_len = 0;
  if (len > document_len)
    len = document_len;

  memcpy (readbuf, document.data () + offset, len);

  return len;
}

/* Return true if FILE is a 64-bit ELF file,
   false if the file is not a 64-bit ELF file,
   and error if the file is not accessible or doesn't exist.  */

static bool
elf_64_file_p (const char *file)
{
  int fd = gdb::handle_eintr (-1, ::open, file, O_RDONLY);
  if (fd < 0)
    perror_with_name (("open"));

  Elf64_Ehdr header;
  ssize_t ret = gdb::handle_eintr (-1, ::read, fd, &header, sizeof (header));
  if (ret == -1)
    perror_with_name (("read"));
  gdb::handle_eintr (-1, ::close, fd);
  if (ret != sizeof (header))
    error ("Cannot read ELF file header: %s", file);

  if (header.e_ident[EI_MAG0] != ELFMAG0
      || header.e_ident[EI_MAG1] != ELFMAG1
      || header.e_ident[EI_MAG2] != ELFMAG2
      || header.e_ident[EI_MAG3] != ELFMAG3)
    error ("Unrecognized ELF file header: %s", file);

  return header.e_ident[EI_CLASS] == ELFCLASS64;
}

/* Construct qXfer:libraries-svr4:read reply.  */

int
netbsd_process_target::qxfer_libraries_svr4 (const char *annex,
					     unsigned char *readbuf,
					     unsigned const char *writebuf,
					     CORE_ADDR offset, int len)
{
  if (writebuf != nullptr)
    return -2;
  if (readbuf == nullptr)
    return -1;

  struct process_info *proc = current_process ();
  pid_t pid = proc->pid;
  bool is_elf64 = elf_64_file_p (netbsd_nat::pid_to_exec_file (pid));

  if (is_elf64)
    return netbsd_qxfer_libraries_svr4<int64_t> (pid, annex, readbuf,
						 writebuf, offset, len);
  else
    return netbsd_qxfer_libraries_svr4<int32_t> (pid, annex, readbuf,
						 writebuf, offset, len);
}

/* Implement the supports_qxfer_libraries_svr4 target_ops method.  */

bool
netbsd_process_target::supports_qxfer_libraries_svr4 ()
{
  return true;
}

/* Return the name of a file that can be opened to get the symbols for
   the child process identified by PID.  */

const char *
netbsd_process_target::pid_to_exec_file (pid_t pid)
{
  return netbsd_nat::pid_to_exec_file (pid);
}

/* Implementation of the target_ops method "supports_pid_to_exec_file".  */

bool
netbsd_process_target::supports_pid_to_exec_file ()
{
  return true;
}

/* Implementation of the target_ops method "supports_hardware_single_step".  */
bool
netbsd_process_target::supports_hardware_single_step ()
{
  return true;
}

/* Implementation of the target_ops method "sw_breakpoint_from_kind".  */

const gdb_byte *
netbsd_process_target::sw_breakpoint_from_kind (int kind, int *size)
{
  static gdb_byte brkpt[PTRACE_BREAKPOINT_SIZE] = {*PTRACE_BREAKPOINT};

  *size = PTRACE_BREAKPOINT_SIZE;

  return brkpt;
}

/* Implement the thread_name target_ops method.  */

const char *
netbsd_process_target::thread_name (ptid_t ptid)
{
  return netbsd_nat::thread_name (ptid);
}

/* Implement the supports_catch_syscall target_ops method.  */

bool
netbsd_process_target::supports_catch_syscall ()
{
  return true;
}

/* Implement the supports_read_auxv target_ops method.  */

bool
netbsd_process_target::supports_read_auxv ()
{
  return true;
}

void
initialize_low ()
{
  set_target_ops (the_netbsd_target);
}
