/* Native-dependent code for FreeBSD.

   Copyright (C) 2002-2024 Free Software Foundation, Inc.

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
#include "gdbsupport/block-signals.h"
#include "gdbsupport/byte-vector.h"
#include "gdbsupport/event-loop.h"
#include "gdbcore.h"
#include "inferior.h"
#include "regcache.h"
#include "regset.h"
#include "gdbarch.h"
#include "gdbcmd.h"
#include "gdbthread.h"
#include "gdbsupport/buildargv.h"
#include "gdbsupport/gdb_wait.h"
#include "inf-loop.h"
#include "inf-ptrace.h"
#include <sys/types.h>
#ifdef HAVE_SYS_PROCCTL_H
#include <sys/procctl.h>
#endif
#include <sys/procfs.h>
#include <sys/ptrace.h>
#include <sys/signal.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <libutil.h>

#include "elf-bfd.h"
#include "fbsd-nat.h"
#include "fbsd-tdep.h"

#ifndef PT_GETREGSET
#define	PT_GETREGSET	42	/* Get a target register set */
#define	PT_SETREGSET	43	/* Set a target register set */
#endif

/* Information stored about each inferior.  */
struct fbsd_inferior : public private_inferior
{
  /* Filter for resumed LWPs which can report events from wait.  */
  ptid_t resumed_lwps = null_ptid;

  /* Number of LWPs this process contains.  */
  unsigned int num_lwps = 0;

  /* Number of LWPs currently running.  */
  unsigned int running_lwps = 0;

  /* Have a pending SIGSTOP event that needs to be discarded.  */
  bool pending_sigstop = false;
};

/* Return the fbsd_inferior attached to INF.  */

static inline fbsd_inferior *
get_fbsd_inferior (inferior *inf)
{
  return gdb::checked_static_cast<fbsd_inferior *> (inf->priv.get ());
}

/* See fbsd-nat.h.  */

void
fbsd_nat_target::add_pending_event (const ptid_t &ptid,
				    const target_waitstatus &status)
{
  gdb_assert (find_inferior_ptid (this, ptid) != nullptr);
  m_pending_events.emplace_back (ptid, status);
}

/* See fbsd-nat.h.  */

bool
fbsd_nat_target::have_pending_event (ptid_t filter)
{
  for (const pending_event &event : m_pending_events)
    if (event.ptid.matches (filter))
      return true;
  return false;
}

/* See fbsd-nat.h.  */

std::optional<fbsd_nat_target::pending_event>
fbsd_nat_target::take_pending_event (ptid_t filter)
{
  for (auto it = m_pending_events.begin (); it != m_pending_events.end (); it++)
    if (it->ptid.matches (filter))
      {
	inferior *inf = find_inferior_ptid (this, it->ptid);
	fbsd_inferior *fbsd_inf = get_fbsd_inferior (inf);
	if (it->ptid.matches (fbsd_inf->resumed_lwps))
	  {
	    pending_event event = *it;
	    m_pending_events.erase (it);
	    return event;
	  }
      }
  return {};
}

/* Return the name of a file that can be opened to get the symbols for
   the child process identified by PID.  */

const char *
fbsd_nat_target::pid_to_exec_file (int pid)
{
  static char buf[PATH_MAX];
  size_t buflen;
  int mib[4];

  mib[0] = CTL_KERN;
  mib[1] = KERN_PROC;
  mib[2] = KERN_PROC_PATHNAME;
  mib[3] = pid;
  buflen = sizeof buf;
  if (sysctl (mib, 4, buf, &buflen, NULL, 0) == 0)
    /* The kern.proc.pathname.<pid> sysctl returns a length of zero
       for processes without an associated executable such as kernel
       processes.  */
    return buflen == 0 ? NULL : buf;

  return NULL;
}

/* Iterate over all the memory regions in the current inferior,
   calling FUNC for each memory region.  DATA is passed as the last
   argument to FUNC.  */

int
fbsd_nat_target::find_memory_regions (find_memory_region_ftype func,
				      void *data)
{
  pid_t pid = inferior_ptid.pid ();
  struct kinfo_vmentry *kve;
  uint64_t size;
  int i, nitems;

  gdb::unique_xmalloc_ptr<struct kinfo_vmentry>
    vmentl (kinfo_getvmmap (pid, &nitems));
  if (vmentl == NULL)
    perror_with_name (_("Couldn't fetch VM map entries"));

  for (i = 0, kve = vmentl.get (); i < nitems; i++, kve++)
    {
      /* Skip unreadable segments and those where MAP_NOCORE has been set.  */
      if (!(kve->kve_protection & KVME_PROT_READ)
	  || kve->kve_flags & KVME_FLAG_NOCOREDUMP)
	continue;

      /* Skip segments with an invalid type.  */
      if (kve->kve_type != KVME_TYPE_DEFAULT
	  && kve->kve_type != KVME_TYPE_VNODE
	  && kve->kve_type != KVME_TYPE_SWAP
	  && kve->kve_type != KVME_TYPE_PHYS)
	continue;

      size = kve->kve_end - kve->kve_start;
      if (info_verbose)
	{
	  gdb_printf ("Save segment, %ld bytes at %s (%c%c%c)\n",
		      (long) size,
		      paddress (current_inferior ()->arch (), kve->kve_start),
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

/* Fetch the command line for a running process.  */

static gdb::unique_xmalloc_ptr<char>
fbsd_fetch_cmdline (pid_t pid)
{
  size_t len;
  int mib[4];

  len = 0;
  mib[0] = CTL_KERN;
  mib[1] = KERN_PROC;
  mib[2] = KERN_PROC_ARGS;
  mib[3] = pid;
  if (sysctl (mib, 4, NULL, &len, NULL, 0) == -1)
    return nullptr;

  if (len == 0)
    return nullptr;

  gdb::unique_xmalloc_ptr<char> cmdline ((char *) xmalloc (len));
  if (sysctl (mib, 4, cmdline.get (), &len, NULL, 0) == -1)
    return nullptr;

  /* Join the arguments with spaces to form a single string.  */
  char *cp = cmdline.get ();
  for (size_t i = 0; i < len - 1; i++)
    if (cp[i] == '\0')
      cp[i] = ' ';
  cp[len - 1] = '\0';

  return cmdline;
}

/* Fetch the external variant of the kernel's internal process
   structure for the process PID into KP.  */

static bool
fbsd_fetch_kinfo_proc (pid_t pid, struct kinfo_proc *kp)
{
  size_t len;
  int mib[4];

  len = sizeof *kp;
  mib[0] = CTL_KERN;
  mib[1] = KERN_PROC;
  mib[2] = KERN_PROC_PID;
  mib[3] = pid;
  return (sysctl (mib, 4, kp, &len, NULL, 0) == 0);
}

/* Implement the "info_proc" target_ops method.  */

bool
fbsd_nat_target::info_proc (const char *args, enum info_proc_what what)
{
  gdb::unique_xmalloc_ptr<struct kinfo_file> fdtbl;
  int nfd = 0;
  struct kinfo_proc kp;
  pid_t pid;
  bool do_cmdline = false;
  bool do_cwd = false;
  bool do_exe = false;
  bool do_files = false;
  bool do_mappings = false;
  bool do_status = false;

  switch (what)
    {
    case IP_MINIMAL:
      do_cmdline = true;
      do_cwd = true;
      do_exe = true;
      break;
    case IP_MAPPINGS:
      do_mappings = true;
      break;
    case IP_STATUS:
    case IP_STAT:
      do_status = true;
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
    case IP_FILES:
      do_files = true;
      break;
    case IP_ALL:
      do_cmdline = true;
      do_cwd = true;
      do_exe = true;
      do_files = true;
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
  if (do_cwd || do_exe || do_files)
    fdtbl.reset (kinfo_getfile (pid, &nfd));

  if (do_cmdline)
    {
      gdb::unique_xmalloc_ptr<char> cmdline = fbsd_fetch_cmdline (pid);
      if (cmdline != nullptr)
	gdb_printf ("cmdline = '%s'\n", cmdline.get ());
      else
	warning (_("unable to fetch command line"));
    }
  if (do_cwd)
    {
      const char *cwd = NULL;
      struct kinfo_file *kf = fdtbl.get ();
      for (int i = 0; i < nfd; i++, kf++)
	{
	  if (kf->kf_type == KF_TYPE_VNODE && kf->kf_fd == KF_FD_TYPE_CWD)
	    {
	      cwd = kf->kf_path;
	      break;
	    }
	}
      if (cwd != NULL)
	gdb_printf ("cwd = '%s'\n", cwd);
      else
	warning (_("unable to fetch current working directory"));
    }
  if (do_exe)
    {
      const char *exe = NULL;
      struct kinfo_file *kf = fdtbl.get ();
      for (int i = 0; i < nfd; i++, kf++)
	{
	  if (kf->kf_type == KF_TYPE_VNODE && kf->kf_fd == KF_FD_TYPE_TEXT)
	    {
	      exe = kf->kf_path;
	      break;
	    }
	}
      if (exe == NULL)
	exe = pid_to_exec_file (pid);
      if (exe != NULL)
	gdb_printf ("exe = '%s'\n", exe);
      else
	warning (_("unable to fetch executable path name"));
    }
  if (do_files)
    {
      struct kinfo_file *kf = fdtbl.get ();

      if (nfd > 0)
	{
	  fbsd_info_proc_files_header ();
	  for (int i = 0; i < nfd; i++, kf++)
	    fbsd_info_proc_files_entry (kf->kf_type, kf->kf_fd, kf->kf_flags,
					kf->kf_offset, kf->kf_vnode_type,
					kf->kf_sock_domain, kf->kf_sock_type,
					kf->kf_sock_protocol, &kf->kf_sa_local,
					&kf->kf_sa_peer, kf->kf_path);
	}
      else
	warning (_("unable to fetch list of open files"));
    }
  if (do_mappings)
    {
      int nvment;
      gdb::unique_xmalloc_ptr<struct kinfo_vmentry>
	vmentl (kinfo_getvmmap (pid, &nvment));

      if (vmentl != nullptr)
	{
	  int addr_bit = TARGET_CHAR_BIT * sizeof (void *);
	  fbsd_info_proc_mappings_header (addr_bit);

	  struct kinfo_vmentry *kve = vmentl.get ();
	  for (int i = 0; i < nvment; i++, kve++)
	    fbsd_info_proc_mappings_entry (addr_bit, kve->kve_start,
					   kve->kve_end, kve->kve_offset,
					   kve->kve_flags, kve->kve_protection,
					   kve->kve_path);
	}
      else
	warning (_("unable to fetch virtual memory map"));
    }
  if (do_status)
    {
      if (!fbsd_fetch_kinfo_proc (pid, &kp))
	warning (_("Failed to fetch process information"));
      else
	{
	  const char *state;
	  int pgtok;

	  gdb_printf ("Name: %s\n", kp.ki_comm);
	  switch (kp.ki_stat)
	    {
	    case SIDL:
	      state = "I (idle)";
	      break;
	    case SRUN:
	      state = "R (running)";
	      break;
	    case SSTOP:
	      state = "T (stopped)";
	      break;
	    case SZOMB:
	      state = "Z (zombie)";
	      break;
	    case SSLEEP:
	      state = "S (sleeping)";
	      break;
	    case SWAIT:
	      state = "W (interrupt wait)";
	      break;
	    case SLOCK:
	      state = "L (blocked on lock)";
	      break;
	    default:
	      state = "? (unknown)";
	      break;
	    }
	  gdb_printf ("State: %s\n", state);
	  gdb_printf ("Parent process: %d\n", kp.ki_ppid);
	  gdb_printf ("Process group: %d\n", kp.ki_pgid);
	  gdb_printf ("Session id: %d\n", kp.ki_sid);
	  gdb_printf ("TTY: %s\n", pulongest (kp.ki_tdev));
	  gdb_printf ("TTY owner process group: %d\n", kp.ki_tpgid);
	  gdb_printf ("User IDs (real, effective, saved): %d %d %d\n",
		      kp.ki_ruid, kp.ki_uid, kp.ki_svuid);
	  gdb_printf ("Group IDs (real, effective, saved): %d %d %d\n",
		      kp.ki_rgid, kp.ki_groups[0], kp.ki_svgid);
	  gdb_printf ("Groups: ");
	  for (int i = 0; i < kp.ki_ngroups; i++)
	    gdb_printf ("%d ", kp.ki_groups[i]);
	  gdb_printf ("\n");
	  gdb_printf ("Minor faults (no memory page): %ld\n",
		      kp.ki_rusage.ru_minflt);
	  gdb_printf ("Minor faults, children: %ld\n",
		      kp.ki_rusage_ch.ru_minflt);
	  gdb_printf ("Major faults (memory page faults): %ld\n",
		      kp.ki_rusage.ru_majflt);
	  gdb_printf ("Major faults, children: %ld\n",
		      kp.ki_rusage_ch.ru_majflt);
	  gdb_printf ("utime: %s.%06ld\n",
		      plongest (kp.ki_rusage.ru_utime.tv_sec),
		      kp.ki_rusage.ru_utime.tv_usec);
	  gdb_printf ("stime: %s.%06ld\n",
		      plongest (kp.ki_rusage.ru_stime.tv_sec),
		      kp.ki_rusage.ru_stime.tv_usec);
	  gdb_printf ("utime, children: %s.%06ld\n",
		      plongest (kp.ki_rusage_ch.ru_utime.tv_sec),
		      kp.ki_rusage_ch.ru_utime.tv_usec);
	  gdb_printf ("stime, children: %s.%06ld\n",
		      plongest (kp.ki_rusage_ch.ru_stime.tv_sec),
		      kp.ki_rusage_ch.ru_stime.tv_usec);
	  gdb_printf ("'nice' value: %d\n", kp.ki_nice);
	  gdb_printf ("Start time: %s.%06ld\n",
		      plongest (kp.ki_start.tv_sec),
		      kp.ki_start.tv_usec);
	  pgtok = getpagesize () / 1024;
	  gdb_printf ("Virtual memory size: %s kB\n",
		      pulongest (kp.ki_size / 1024));
	  gdb_printf ("Data size: %s kB\n",
		      pulongest (kp.ki_dsize * pgtok));
	  gdb_printf ("Stack size: %s kB\n",
		      pulongest (kp.ki_ssize * pgtok));
	  gdb_printf ("Text size: %s kB\n",
		      pulongest (kp.ki_tsize * pgtok));
	  gdb_printf ("Resident set size: %s kB\n",
		      pulongest (kp.ki_rssize * pgtok));
	  gdb_printf ("Maximum RSS: %s kB\n",
		      pulongest (kp.ki_rusage.ru_maxrss));
	  gdb_printf ("Pending Signals: ");
	  for (int i = 0; i < _SIG_WORDS; i++)
	    gdb_printf ("%08x ", kp.ki_siglist.__bits[i]);
	  gdb_printf ("\n");
	  gdb_printf ("Ignored Signals: ");
	  for (int i = 0; i < _SIG_WORDS; i++)
	    gdb_printf ("%08x ", kp.ki_sigignore.__bits[i]);
	  gdb_printf ("\n");
	  gdb_printf ("Caught Signals: ");
	  for (int i = 0; i < _SIG_WORDS; i++)
	    gdb_printf ("%08x ", kp.ki_sigcatch.__bits[i]);
	  gdb_printf ("\n");
	}
    }

  return true;
}

/* Return the size of siginfo for the current inferior.  */

#ifdef __LP64__
union sigval32 {
  int sival_int;
  uint32_t sival_ptr;
};

/* This structure matches the naming and layout of `siginfo_t' in
   <sys/signal.h>.  In particular, the `si_foo' macros defined in that
   header can be used with both types to copy fields in the `_reason'
   union.  */

struct siginfo32
{
  int si_signo;
  int si_errno;
  int si_code;
  __pid_t si_pid;
  __uid_t si_uid;
  int si_status;
  uint32_t si_addr;
  union sigval32 si_value;
  union
  {
    struct
    {
      int _trapno;
    } _fault;
    struct
    {
      int _timerid;
      int _overrun;
    } _timer;
    struct
    {
      int _mqd;
    } _mesgq;
    struct
    {
      int32_t _band;
    } _poll;
    struct
    {
      int32_t __spare1__;
      int __spare2__[7];
    } __spare__;
  } _reason;
};
#endif

static size_t
fbsd_siginfo_size ()
{
#ifdef __LP64__
  struct gdbarch *gdbarch = get_frame_arch (get_current_frame ());

  /* Is the inferior 32-bit?  If so, use the 32-bit siginfo size.  */
  if (gdbarch_long_bit (gdbarch) == 32)
    return sizeof (struct siginfo32);
#endif
  return sizeof (siginfo_t);
}

/* Convert a native 64-bit siginfo object to a 32-bit object.  Note
   that FreeBSD doesn't support writing to $_siginfo, so this only
   needs to convert one way.  */

static void
fbsd_convert_siginfo (siginfo_t *si)
{
#ifdef __LP64__
  struct gdbarch *gdbarch = get_frame_arch (get_current_frame ());

  /* Is the inferior 32-bit?  If not, nothing to do.  */
  if (gdbarch_long_bit (gdbarch) != 32)
    return;

  struct siginfo32 si32;

  si32.si_signo = si->si_signo;
  si32.si_errno = si->si_errno;
  si32.si_code = si->si_code;
  si32.si_pid = si->si_pid;
  si32.si_uid = si->si_uid;
  si32.si_status = si->si_status;
  si32.si_addr = (uintptr_t) si->si_addr;

  /* If sival_ptr is being used instead of sival_int on a big-endian
     platform, then sival_int will be zero since it holds the upper
     32-bits of the pointer value.  */
#if _BYTE_ORDER == _BIG_ENDIAN
  if (si->si_value.sival_int == 0)
    si32.si_value.sival_ptr = (uintptr_t) si->si_value.sival_ptr;
  else
    si32.si_value.sival_int = si->si_value.sival_int;
#else
  si32.si_value.sival_int = si->si_value.sival_int;
#endif

  /* Always copy the spare fields and then possibly overwrite them for
     signal-specific or code-specific fields.  */
  si32._reason.__spare__.__spare1__ = si->_reason.__spare__.__spare1__;
  for (int i = 0; i < 7; i++)
    si32._reason.__spare__.__spare2__[i] = si->_reason.__spare__.__spare2__[i];
  switch (si->si_signo) {
  case SIGILL:
  case SIGFPE:
  case SIGSEGV:
  case SIGBUS:
    si32.si_trapno = si->si_trapno;
    break;
  }
  switch (si->si_code) {
  case SI_TIMER:
    si32.si_timerid = si->si_timerid;
    si32.si_overrun = si->si_overrun;
    break;
  case SI_MESGQ:
    si32.si_mqd = si->si_mqd;
    break;
  }

  memcpy(si, &si32, sizeof (si32));
#endif
}

/* Implement the "xfer_partial" target_ops method.  */

enum target_xfer_status
fbsd_nat_target::xfer_partial (enum target_object object,
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
	struct ptrace_lwpinfo pl;
	size_t siginfo_size;

	/* FreeBSD doesn't support writing to $_siginfo.  */
	if (writebuf != NULL)
	  return TARGET_XFER_E_IO;

	if (inferior_ptid.lwp_p ())
	  pid = inferior_ptid.lwp ();

	siginfo_size = fbsd_siginfo_size ();
	if (offset > siginfo_size)
	  return TARGET_XFER_E_IO;

	if (ptrace (PT_LWPINFO, pid, (PTRACE_TYPE_ARG3) &pl, sizeof (pl)) == -1)
	  return TARGET_XFER_E_IO;

	if (!(pl.pl_flags & PL_FLAG_SI))
	  return TARGET_XFER_E_IO;

	fbsd_convert_siginfo (&pl.pl_siginfo);
	if (offset + len > siginfo_size)
	  len = siginfo_size - offset;

	memcpy (readbuf, ((gdb_byte *) &pl.pl_siginfo) + offset, len);
	*xfered_len = len;
	return TARGET_XFER_OK;
      }
#ifdef KERN_PROC_AUXV
    case TARGET_OBJECT_AUXV:
      {
	gdb::byte_vector buf_storage;
	gdb_byte *buf;
	size_t buflen;
	int mib[4];

	if (writebuf != NULL)
	  return TARGET_XFER_E_IO;
	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_AUXV;
	mib[3] = pid;
	if (offset == 0)
	  {
	    buf = readbuf;
	    buflen = len;
	  }
	else
	  {
	    buflen = offset + len;
	    buf_storage.resize (buflen);
	    buf = buf_storage.data ();
	  }
	if (sysctl (mib, 4, buf, &buflen, NULL, 0) == 0)
	  {
	    if (offset != 0)
	      {
		if (buflen > offset)
		  {
		    buflen -= offset;
		    memcpy (readbuf, buf + offset, buflen);
		  }
		else
		  buflen = 0;
	      }
	    *xfered_len = buflen;
	    return (buflen == 0) ? TARGET_XFER_EOF : TARGET_XFER_OK;
	  }
	return TARGET_XFER_E_IO;
      }
#endif
#if defined(KERN_PROC_VMMAP) && defined(KERN_PROC_PS_STRINGS)
    case TARGET_OBJECT_FREEBSD_VMMAP:
    case TARGET_OBJECT_FREEBSD_PS_STRINGS:
      {
	gdb::byte_vector buf_storage;
	gdb_byte *buf;
	size_t buflen;
	int mib[4];

	int proc_target;
	uint32_t struct_size;
	switch (object)
	  {
	  case TARGET_OBJECT_FREEBSD_VMMAP:
	    proc_target = KERN_PROC_VMMAP;
	    struct_size = sizeof (struct kinfo_vmentry);
	    break;
	  case TARGET_OBJECT_FREEBSD_PS_STRINGS:
	    proc_target = KERN_PROC_PS_STRINGS;
	    struct_size = sizeof (void *);
	    break;
	  }

	if (writebuf != NULL)
	  return TARGET_XFER_E_IO;

	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = proc_target;
	mib[3] = pid;

	if (sysctl (mib, 4, NULL, &buflen, NULL, 0) != 0)
	  return TARGET_XFER_E_IO;
	buflen += sizeof (struct_size);

	if (offset >= buflen)
	  {
	    *xfered_len = 0;
	    return TARGET_XFER_EOF;
	  }

	buf_storage.resize (buflen);
	buf = buf_storage.data ();

	memcpy (buf, &struct_size, sizeof (struct_size));
	buflen -= sizeof (struct_size);
	if (sysctl (mib, 4, buf + sizeof (struct_size), &buflen, NULL, 0) != 0)
	  return TARGET_XFER_E_IO;
	buflen += sizeof (struct_size);

	if (buflen - offset < len)
	  len = buflen - offset;
	memcpy (readbuf, buf + offset, len);
	*xfered_len = len;
	return TARGET_XFER_OK;
      }
#endif
    default:
      return inf_ptrace_target::xfer_partial (object, annex,
					      readbuf, writebuf, offset,
					      len, xfered_len);
    }
}

static bool debug_fbsd_lwp;
static bool debug_fbsd_nat;

static void
show_fbsd_lwp_debug (struct ui_file *file, int from_tty,
		     struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Debugging of FreeBSD lwp module is %s.\n"), value);
}

static void
show_fbsd_nat_debug (struct ui_file *file, int from_tty,
		     struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Debugging of FreeBSD native target is %s.\n"),
	      value);
}

#define fbsd_lwp_debug_printf(fmt, ...) \
  debug_prefixed_printf_cond (debug_fbsd_lwp, "fbsd-lwp", fmt, ##__VA_ARGS__)

#define fbsd_nat_debug_printf(fmt, ...) \
  debug_prefixed_printf_cond (debug_fbsd_nat, "fbsd-nat", fmt, ##__VA_ARGS__)

#define fbsd_nat_debug_start_end(fmt, ...) \
  scoped_debug_start_end (debug_fbsd_nat, "fbsd-nat", fmt, ##__VA_ARGS__)

/*
  FreeBSD's first thread support was via a "reentrant" version of libc
  (libc_r) that first shipped in 2.2.7.  This library multiplexed all
  of the threads in a process onto a single kernel thread.  This
  library was supported via the bsd-uthread target.

  FreeBSD 5.1 introduced two new threading libraries that made use of
  multiple kernel threads.  The first (libkse) scheduled M user
  threads onto N (<= M) kernel threads (LWPs).  The second (libthr)
  bound each user thread to a dedicated kernel thread.  libkse shipped
  as the default threading library (libpthread).

  FreeBSD 5.3 added a libthread_db to abstract the interface across
  the various thread libraries (libc_r, libkse, and libthr).

  FreeBSD 7.0 switched the default threading library from from libkse
  to libpthread and removed libc_r.

  FreeBSD 8.0 removed libkse and the in-kernel support for it.  The
  only threading library supported by 8.0 and later is libthr which
  ties each user thread directly to an LWP.  To simplify the
  implementation, this target only supports LWP-backed threads using
  ptrace directly rather than libthread_db.

  FreeBSD 11.0 introduced LWP event reporting via PT_LWP_EVENTS.
*/

/* Return true if PTID is still active in the inferior.  */

bool
fbsd_nat_target::thread_alive (ptid_t ptid)
{
  if (ptid.lwp_p ())
    {
      struct ptrace_lwpinfo pl;

      if (ptrace (PT_LWPINFO, ptid.lwp (), (caddr_t) &pl, sizeof pl)
	  == -1)
	{
	  /* EBUSY means the associated process is running which means
	     the LWP does exist and belongs to a running process.  */
	  if (errno == EBUSY)
	    return true;
	  return false;
	}
#ifdef PL_FLAG_EXITED
      if (pl.pl_flags & PL_FLAG_EXITED)
	return false;
#endif
    }

  return true;
}

/* Convert PTID to a string.  */

std::string
fbsd_nat_target::pid_to_str (ptid_t ptid)
{
  lwpid_t lwp;

  lwp = ptid.lwp ();
  if (lwp != 0)
    {
      int pid = ptid.pid ();

      return string_printf ("LWP %d of process %d", lwp, pid);
    }

  return normal_pid_to_str (ptid);
}

#ifdef HAVE_STRUCT_PTRACE_LWPINFO_PL_TDNAME
/* Return the name assigned to a thread by an application.  Returns
   the string in a static buffer.  */

const char *
fbsd_nat_target::thread_name (struct thread_info *thr)
{
  struct ptrace_lwpinfo pl;
  struct kinfo_proc kp;
  int pid = thr->ptid.pid ();
  long lwp = thr->ptid.lwp ();
  static char buf[sizeof pl.pl_tdname + 1];

  /* Note that ptrace_lwpinfo returns the process command in pl_tdname
     if a name has not been set explicitly.  Return a NULL name in
     that case.  */
  if (!fbsd_fetch_kinfo_proc (pid, &kp))
    return nullptr;
  if (ptrace (PT_LWPINFO, lwp, (caddr_t) &pl, sizeof pl) == -1)
    return nullptr;
  if (strcmp (kp.ki_comm, pl.pl_tdname) == 0)
    return NULL;
  xsnprintf (buf, sizeof buf, "%s", pl.pl_tdname);
  return buf;
}
#endif

/* Enable additional event reporting on new processes.

   To catch fork events, PTRACE_FORK is set on every traced process
   to enable stops on returns from fork or vfork.  Note that both the
   parent and child will always stop, even if system call stops are
   not enabled.

   To catch LWP events, PTRACE_EVENTS is set on every traced process.
   This enables stops on the birth for new LWPs (excluding the "main" LWP)
   and the death of LWPs (excluding the last LWP in a process).  Note
   that unlike fork events, the LWP that creates a new LWP does not
   report an event.  */

static void
fbsd_enable_proc_events (pid_t pid)
{
#ifdef PT_GET_EVENT_MASK
  int events;

  if (ptrace (PT_GET_EVENT_MASK, pid, (PTRACE_TYPE_ARG3) &events,
	      sizeof (events)) == -1)
    perror_with_name (("ptrace (PT_GET_EVENT_MASK)"));
  events |= PTRACE_FORK | PTRACE_LWP;
#ifdef PTRACE_VFORK
  events |= PTRACE_VFORK;
#endif
  if (ptrace (PT_SET_EVENT_MASK, pid, (PTRACE_TYPE_ARG3) &events,
	      sizeof (events)) == -1)
    perror_with_name (("ptrace (PT_SET_EVENT_MASK)"));
#else
#ifdef TDP_RFPPWAIT
  if (ptrace (PT_FOLLOW_FORK, pid, (PTRACE_TYPE_ARG3) 0, 1) == -1)
    perror_with_name (("ptrace (PT_FOLLOW_FORK)"));
#endif
#ifdef PT_LWP_EVENTS
  if (ptrace (PT_LWP_EVENTS, pid, (PTRACE_TYPE_ARG3) 0, 1) == -1)
    perror_with_name (("ptrace (PT_LWP_EVENTS)"));
#endif
#endif
}

/* Add threads for any new LWPs in a process.

   When LWP events are used, this function is only used to detect existing
   threads when attaching to a process.  On older systems, this function is
   called to discover new threads each time the thread list is updated.  */

static void
fbsd_add_threads (fbsd_nat_target *target, pid_t pid)
{
  int i, nlwps;

  gdb_assert (!in_thread_list (target, ptid_t (pid)));
  nlwps = ptrace (PT_GETNUMLWPS, pid, NULL, 0);
  if (nlwps == -1)
    perror_with_name (("ptrace (PT_GETNUMLWPS)"));

  gdb::unique_xmalloc_ptr<lwpid_t[]> lwps (XCNEWVEC (lwpid_t, nlwps));

  nlwps = ptrace (PT_GETLWPLIST, pid, (caddr_t) lwps.get (), nlwps);
  if (nlwps == -1)
    perror_with_name (("ptrace (PT_GETLWPLIST)"));

  inferior *inf = find_inferior_ptid (target, ptid_t (pid));
  fbsd_inferior *fbsd_inf = get_fbsd_inferior (inf);
  gdb_assert (fbsd_inf != nullptr);
  for (i = 0; i < nlwps; i++)
    {
      ptid_t ptid = ptid_t (pid, lwps[i]);

      if (!in_thread_list (target, ptid))
	{
#ifdef PT_LWP_EVENTS
	  struct ptrace_lwpinfo pl;

	  /* Don't add exited threads.  Note that this is only called
	     when attaching to a multi-threaded process.  */
	  if (ptrace (PT_LWPINFO, lwps[i], (caddr_t) &pl, sizeof pl) == -1)
	    perror_with_name (("ptrace (PT_LWPINFO)"));
	  if (pl.pl_flags & PL_FLAG_EXITED)
	    continue;
#endif
	  fbsd_lwp_debug_printf ("adding thread for LWP %u", lwps[i]);
	  add_thread (target, ptid);
#ifdef PT_LWP_EVENTS
	  fbsd_inf->num_lwps++;
#endif
	}
    }
#ifndef PT_LWP_EVENTS
  fbsd_inf->num_lwps = nlwps;
#endif
}

/* Implement the "update_thread_list" target_ops method.  */

void
fbsd_nat_target::update_thread_list ()
{
#ifdef PT_LWP_EVENTS
  /* With support for thread events, threads are added/deleted from the
     list as events are reported, so just try deleting exited threads.  */
  delete_exited_threads ();
#else
  prune_threads ();

  fbsd_add_threads (this, inferior_ptid.pid ());
#endif
}

/* Async mode support.  */

/* Implement the "can_async_p" target method.  */

bool
fbsd_nat_target::can_async_p ()
{
  /* This flag should be checked in the common target.c code.  */
  gdb_assert (target_async_permitted);

  /* Otherwise, this targets is always able to support async mode.  */
  return true;
}

/* SIGCHLD handler notifies the event-loop in async mode.  */

static void
sigchld_handler (int signo)
{
  int old_errno = errno;

  fbsd_nat_target::async_file_mark_if_open ();

  errno = old_errno;
}

/* Callback registered with the target events file descriptor.  */

static void
handle_target_event (int error, gdb_client_data client_data)
{
  inferior_event_handler (INF_REG_EVENT);
}

/* Implement the "async" target method.  */

void
fbsd_nat_target::async (bool enable)
{
  if (enable == is_async_p ())
    return;

  /* Block SIGCHILD while we create/destroy the pipe, as the handler
     writes to it.  */
  gdb::block_signals blocker;

  if (enable)
    {
      if (!async_file_open ())
	internal_error ("failed to create event pipe.");

      add_file_handler (async_wait_fd (), handle_target_event, NULL, "fbsd-nat");

      /* Trigger a poll in case there are pending events to
	 handle.  */
      async_file_mark ();
    }
  else
    {
      delete_file_handler (async_wait_fd ());
      async_file_close ();
    }
}

#ifdef TDP_RFPPWAIT
/*
  To catch fork events, PT_FOLLOW_FORK is set on every traced process
  to enable stops on returns from fork or vfork.  Note that both the
  parent and child will always stop, even if system call stops are not
  enabled.

  After a fork, both the child and parent process will stop and report
  an event.  However, there is no guarantee of order.  If the parent
  reports its stop first, then fbsd_wait explicitly waits for the new
  child before returning.  If the child reports its stop first, then
  the event is saved on a list and ignored until the parent's stop is
  reported.  fbsd_wait could have been changed to fetch the parent PID
  of the new child and used that to wait for the parent explicitly.
  However, if two threads in the parent fork at the same time, then
  the wait on the parent might return the "wrong" fork event.

  The initial version of PT_FOLLOW_FORK did not set PL_FLAG_CHILD for
  the new child process.  This flag could be inferred by treating any
  events for an unknown pid as a new child.

  In addition, the initial version of PT_FOLLOW_FORK did not report a
  stop event for the parent process of a vfork until after the child
  process executed a new program or exited.  The kernel was changed to
  defer the wait for exit or exec of the child until after posting the
  stop event shortly after the change to introduce PL_FLAG_CHILD.
  This could be worked around by reporting a vfork event when the
  child event posted and ignoring the subsequent event from the
  parent.

  This implementation requires both of these fixes for simplicity's
  sake.  FreeBSD versions newer than 9.1 contain both fixes.
*/

static std::list<ptid_t> fbsd_pending_children;

/* Record a new child process event that is reported before the
   corresponding fork event in the parent.  */

static void
fbsd_remember_child (ptid_t pid)
{
  fbsd_pending_children.push_front (pid);
}

/* Check for a previously-recorded new child process event for PID.
   If one is found, remove it from the list and return the PTID.  */

static ptid_t
fbsd_is_child_pending (pid_t pid)
{
  for (auto it = fbsd_pending_children.begin ();
       it != fbsd_pending_children.end (); it++)
    if (it->pid () == pid)
      {
	ptid_t ptid = *it;
	fbsd_pending_children.erase (it);
	return ptid;
      }
  return null_ptid;
}

/* Wait for a child of a fork to report its stop.  Returns the PTID of
   the new child process.  */

static ptid_t
fbsd_wait_for_fork_child (pid_t pid)
{
  ptid_t ptid = fbsd_is_child_pending (pid);
  if (ptid != null_ptid)
    return ptid;

  int status;
  pid_t wpid = waitpid (pid, &status, 0);
  if (wpid == -1)
    perror_with_name (("waitpid"));

  gdb_assert (wpid == pid);

  struct ptrace_lwpinfo pl;
  if (ptrace (PT_LWPINFO, wpid, (caddr_t) &pl, sizeof pl) == -1)
    perror_with_name (("ptrace (PT_LWPINFO)"));

  gdb_assert (pl.pl_flags & PL_FLAG_CHILD);
  return ptid_t (wpid, pl.pl_lwpid);
}

#ifndef PTRACE_VFORK
/* Record a pending vfork done event.  */

static void
fbsd_add_vfork_done (ptid_t pid)
{
  add_pending_event (ptid, target_waitstatus ().set_vfork_done ());

  /* If we're in async mode, need to tell the event loop there's
     something here to process.  */
  if (target_is_async_p ())
    async_file_mark ();
}
#endif
#endif

/* Resume a single process.  */

void
fbsd_nat_target::resume_one_process (ptid_t ptid, int step,
				     enum gdb_signal signo)
{
  fbsd_nat_debug_printf ("[%s], step %d, signo %d (%s)",
			 target_pid_to_str (ptid).c_str (), step, signo,
			 gdb_signal_to_name (signo));

  inferior *inf = find_inferior_ptid (this, ptid);
  fbsd_inferior *fbsd_inf = get_fbsd_inferior (inf);
  fbsd_inf->resumed_lwps = ptid;
  gdb_assert (fbsd_inf->running_lwps == 0);

  /* Don't PT_CONTINUE a thread or process which has a pending event.  */
  if (have_pending_event (ptid))
    {
      fbsd_nat_debug_printf ("found pending event");
      return;
    }

  for (thread_info *tp : inf->non_exited_threads ())
    {
      /* If ptid is a specific LWP, suspend all other LWPs in the
	 process, otherwise resume all LWPs in the process..  */
      if (!ptid.lwp_p() || tp->ptid.lwp () == ptid.lwp ())
	{
	  if (ptrace (PT_RESUME, tp->ptid.lwp (), NULL, 0) == -1)
	    perror_with_name (("ptrace (PT_RESUME)"));
	  low_prepare_to_resume (tp);
	  fbsd_inf->running_lwps++;
	}
      else
	{
	  if (ptrace (PT_SUSPEND, tp->ptid.lwp (), NULL, 0) == -1)
	    perror_with_name (("ptrace (PT_SUSPEND)"));
	}
    }

  if (ptid.pid () != inferior_ptid.pid ())
    {
      step = 0;
      signo = GDB_SIGNAL_0;
      gdb_assert (!ptid.lwp_p ());
    }
  else
    {
      ptid = inferior_ptid;
#if __FreeBSD_version < 1200052
      /* When multiple threads within a process wish to report STOPPED
	 events from wait(), the kernel picks one thread event as the
	 thread event to report.  The chosen thread event is retrieved
	 via PT_LWPINFO by passing the process ID as the request pid.
	 If multiple events are pending, then the subsequent wait()
	 after resuming a process will report another STOPPED event
	 after resuming the process to handle the next thread event
	 and so on.

	 A single thread event is cleared as a side effect of resuming
	 the process with PT_CONTINUE, PT_STEP, etc.  In older
	 kernels, however, the request pid was used to select which
	 thread's event was cleared rather than always clearing the
	 event that was just reported.  To avoid clearing the event of
	 the wrong LWP, always pass the process ID instead of an LWP
	 ID to PT_CONTINUE or PT_SYSCALL.

	 In the case of stepping, the process ID cannot be used with
	 PT_STEP since it would step the thread that reported an event
	 which may not be the thread indicated by PTID.  For stepping,
	 use PT_SETSTEP to enable stepping on the desired thread
	 before resuming the process via PT_CONTINUE instead of using
	 PT_STEP.  */
      if (step)
	{
	  if (ptrace (PT_SETSTEP, get_ptrace_pid (ptid), NULL, 0) == -1)
	    perror_with_name (("ptrace (PT_SETSTEP)"));
	  step = 0;
	}
      ptid = ptid_t (ptid.pid ());
#endif
    }

  inf_ptrace_target::resume (ptid, step, signo);
}

/* Implement the "resume" target_ops method.  */

void
fbsd_nat_target::resume (ptid_t scope_ptid, int step, enum gdb_signal signo)
{
  fbsd_nat_debug_start_end ("[%s], step %d, signo %d (%s)",
			    target_pid_to_str (scope_ptid).c_str (), step, signo,
			    gdb_signal_to_name (signo));

  gdb_assert (inferior_ptid.matches (scope_ptid));
  gdb_assert (!scope_ptid.tid_p ());

  if (scope_ptid == minus_one_ptid)
    {
      for (inferior *inf : all_non_exited_inferiors (this))
	resume_one_process (ptid_t (inf->pid), step, signo);
    }
  else
    {
      resume_one_process (scope_ptid, step, signo);
    }
}

#ifdef USE_SIGTRAP_SIGINFO
/* Handle breakpoint and trace traps reported via SIGTRAP.  If the
   trap was a breakpoint or trace trap that should be reported to the
   core, return true.  */

static bool
fbsd_handle_debug_trap (fbsd_nat_target *target, ptid_t ptid,
			const struct ptrace_lwpinfo &pl)
{

  /* Ignore traps without valid siginfo or for signals other than
     SIGTRAP.

     FreeBSD kernels prior to r341800 can return stale siginfo for at
     least some events, but those events can be identified by
     additional flags set in pl_flags.  True breakpoint and
     single-step traps should not have other flags set in
     pl_flags.  */
  if (pl.pl_flags != PL_FLAG_SI || pl.pl_siginfo.si_signo != SIGTRAP)
    return false;

  /* Trace traps are either a single step or a hardware watchpoint or
     breakpoint.  */
  if (pl.pl_siginfo.si_code == TRAP_TRACE)
    {
      fbsd_nat_debug_printf ("trace trap for LWP %ld", ptid.lwp ());
      return true;
    }

  if (pl.pl_siginfo.si_code == TRAP_BRKPT)
    {
      /* Fixup PC for the software breakpoint.  */
      struct regcache *regcache = get_thread_regcache (target, ptid);
      struct gdbarch *gdbarch = regcache->arch ();
      int decr_pc = gdbarch_decr_pc_after_break (gdbarch);

      fbsd_nat_debug_printf ("sw breakpoint trap for LWP %ld", ptid.lwp ());
      if (decr_pc != 0)
	{
	  CORE_ADDR pc;

	  pc = regcache_read_pc (regcache);
	  regcache_write_pc (regcache, pc - decr_pc);
	}
      return true;
    }

  return false;
}
#endif

/* Wait for the child specified by PTID to do something.  Return the
   process ID of the child, or MINUS_ONE_PTID in case of error; store
   the status in *OURSTATUS.  */

ptid_t
fbsd_nat_target::wait_1 (ptid_t ptid, struct target_waitstatus *ourstatus,
			 target_wait_flags target_options)
{
  ptid_t wptid;

  while (1)
    {
      wptid = inf_ptrace_target::wait (ptid, ourstatus, target_options);
      if (ourstatus->kind () == TARGET_WAITKIND_STOPPED)
	{
	  struct ptrace_lwpinfo pl;
	  pid_t pid = wptid.pid ();
	  if (ptrace (PT_LWPINFO, pid, (caddr_t) &pl, sizeof pl) == -1)
	    perror_with_name (("ptrace (PT_LWPINFO)"));

	  wptid = ptid_t (pid, pl.pl_lwpid);

	  if (debug_fbsd_nat)
	    {
	      fbsd_nat_debug_printf ("stop for LWP %u event %d flags %#x",
				     pl.pl_lwpid, pl.pl_event, pl.pl_flags);
	      if (pl.pl_flags & PL_FLAG_SI)
		fbsd_nat_debug_printf ("si_signo %u si_code %u",
				       pl.pl_siginfo.si_signo,
				       pl.pl_siginfo.si_code);
	    }

	  /* There may not be an inferior for this pid if this is a
	     PL_FLAG_CHILD event.  */
	  inferior *inf = find_inferior_ptid (this, wptid);
	  fbsd_inferior *fbsd_inf = inf == nullptr ? nullptr
	    : get_fbsd_inferior (inf);
	  gdb_assert (fbsd_inf != nullptr || pl.pl_flags & PL_FLAG_CHILD);

#ifdef PT_LWP_EVENTS
	  if (pl.pl_flags & PL_FLAG_EXITED)
	    {
	      /* If GDB attaches to a multi-threaded process, exiting
		 threads might be skipped during post_attach that
		 have not yet reported their PL_FLAG_EXITED event.
		 Ignore EXITED events for an unknown LWP.  */
	      thread_info *thr = this->find_thread (wptid);
	      if (thr != nullptr)
		{
		  fbsd_lwp_debug_printf ("deleting thread for LWP %u",
					 pl.pl_lwpid);
		  low_delete_thread (thr);
		  delete_thread (thr);
		  fbsd_inf->num_lwps--;

		  /* If this LWP was the only resumed LWP from the
		     process, report an event to the core.  */
		  if (wptid == fbsd_inf->resumed_lwps)
		    {
		      ourstatus->set_spurious ();
		      return wptid;
		    }

		  /* During process exit LWPs that were not resumed
		     will report exit events.  */
		  if (wptid.matches (fbsd_inf->resumed_lwps))
		    fbsd_inf->running_lwps--;
		}
	      if (ptrace (PT_CONTINUE, pid, (caddr_t) 1, 0) == -1)
		perror_with_name (("ptrace (PT_CONTINUE)"));
	      continue;
	    }
#endif

	  /* Switch to an LWP PTID on the first stop in a new process.
	     This is done after handling PL_FLAG_EXITED to avoid
	     switching to an exited LWP.  It is done before checking
	     PL_FLAG_BORN in case the first stop reported after
	     attaching to an existing process is a PL_FLAG_BORN
	     event.  */
	  if (in_thread_list (this, ptid_t (pid)))
	    {
	      fbsd_lwp_debug_printf ("using LWP %u for first thread",
				     pl.pl_lwpid);
	      thread_change_ptid (this, ptid_t (pid), wptid);
	    }

#ifdef PT_LWP_EVENTS
	  if (pl.pl_flags & PL_FLAG_BORN)
	    {
	      /* If GDB attaches to a multi-threaded process, newborn
		 threads might be added by fbsd_add_threads that have
		 not yet reported their PL_FLAG_BORN event.  Ignore
		 BORN events for an already-known LWP.  */
	      if (!in_thread_list (this, wptid))
		{
		  fbsd_lwp_debug_printf ("adding thread for LWP %u",
					 pl.pl_lwpid);
		  add_thread (this, wptid);
		  fbsd_inf->num_lwps++;

		  if (wptid.matches(fbsd_inf->resumed_lwps))
		    fbsd_inf->running_lwps++;
		}
	      ourstatus->set_spurious ();
	      return wptid;
	    }
#endif

#ifdef TDP_RFPPWAIT
	  if (pl.pl_flags & PL_FLAG_FORKED)
	    {
#ifndef PTRACE_VFORK
	      struct kinfo_proc kp;
#endif
	      bool is_vfork = false;
	      ptid_t child_ptid;
	      pid_t child;

	      child = pl.pl_child_pid;
#ifdef PTRACE_VFORK
	      if (pl.pl_flags & PL_FLAG_VFORKED)
		is_vfork = true;
#endif

	      /* Make sure the other end of the fork is stopped too.  */
	      child_ptid = fbsd_wait_for_fork_child (child);

	      /* Enable additional events on the child process.  */
	      fbsd_enable_proc_events (child_ptid.pid ());

#ifndef PTRACE_VFORK
	      /* For vfork, the child process will have the P_PPWAIT
		 flag set.  */
	      if (fbsd_fetch_kinfo_proc (child, &kp))
		{
		  if (kp.ki_flag & P_PPWAIT)
		    is_vfork = true;
		}
	      else
		warning (_("Failed to fetch process information"));
#endif

	      low_new_fork (wptid, child);

	      if (is_vfork)
		ourstatus->set_vforked (child_ptid);
	      else
		ourstatus->set_forked (child_ptid);

	      return wptid;
	    }

	  if (pl.pl_flags & PL_FLAG_CHILD)
	    {
	      /* Remember that this child forked, but do not report it
		 until the parent reports its corresponding fork
		 event.  */
	      fbsd_remember_child (wptid);
	      continue;
	    }

#ifdef PTRACE_VFORK
	  if (pl.pl_flags & PL_FLAG_VFORK_DONE)
	    {
	      ourstatus->set_vfork_done ();
	      return wptid;
	    }
#endif
#endif

	  if (pl.pl_flags & PL_FLAG_EXEC)
	    {
	      ourstatus->set_execd
		(make_unique_xstrdup (pid_to_exec_file (pid)));
	      return wptid;
	    }

#ifdef USE_SIGTRAP_SIGINFO
	  if (fbsd_handle_debug_trap (this, wptid, pl))
	    return wptid;
#endif

	  /* Note that PL_FLAG_SCE is set for any event reported while
	     a thread is executing a system call in the kernel.  In
	     particular, signals that interrupt a sleep in a system
	     call will report this flag as part of their event.  Stops
	     explicitly for system call entry and exit always use
	     SIGTRAP, so only treat SIGTRAP events as system call
	     entry/exit events.  */
	  if (pl.pl_flags & (PL_FLAG_SCE | PL_FLAG_SCX)
	      && ourstatus->sig () == GDB_SIGNAL_TRAP)
	    {
#ifdef HAVE_STRUCT_PTRACE_LWPINFO_PL_SYSCALL_CODE
	      if (catch_syscall_enabled ())
		{
		  if (catching_syscall_number (pl.pl_syscall_code))
		    {
		      if (pl.pl_flags & PL_FLAG_SCE)
			ourstatus->set_syscall_entry (pl.pl_syscall_code);
		      else
			ourstatus->set_syscall_return (pl.pl_syscall_code);

		      return wptid;
		    }
		}
#endif
	      /* If the core isn't interested in this event, just
		 continue the process explicitly and wait for another
		 event.  Note that PT_SYSCALL is "sticky" on FreeBSD
		 and once system call stops are enabled on a process
		 it stops for all system call entries and exits.  */
	      if (ptrace (PT_CONTINUE, pid, (caddr_t) 1, 0) == -1)
		perror_with_name (("ptrace (PT_CONTINUE)"));
	      continue;
	    }

	  /* If this is a pending SIGSTOP event from an earlier call
	     to stop_process, discard the event and wait for another
	     event.  */
	  if (ourstatus->sig () == GDB_SIGNAL_STOP && fbsd_inf->pending_sigstop)
	    {
	      fbsd_nat_debug_printf ("ignoring SIGSTOP for pid %u", pid);
	      fbsd_inf->pending_sigstop = false;
	      if (ptrace (PT_CONTINUE, pid, (caddr_t) 1, 0) == -1)
		perror_with_name (("ptrace (PT_CONTINUE)"));
	      continue;
	    }
	}
      else
	fbsd_nat_debug_printf ("event [%s], [%s]",
			       target_pid_to_str (wptid).c_str (),
			       ourstatus->to_string ().c_str ());
      return wptid;
    }
}

/* Stop a given process.  If the process is already stopped, record
   its pending event instead.  */

void
fbsd_nat_target::stop_process (inferior *inf)
{
  fbsd_inferior *fbsd_inf = get_fbsd_inferior (inf);
  gdb_assert (fbsd_inf != nullptr);

  fbsd_inf->resumed_lwps = null_ptid;
  if (fbsd_inf->running_lwps == 0)
    return;

  ptid_t ptid (inf->pid);
  target_waitstatus status;
  ptid_t wptid = wait_1 (ptid, &status, TARGET_WNOHANG);

  if (wptid != minus_one_ptid)
    {
      /* Save the current event as a pending event.  */
      add_pending_event (wptid, status);
      fbsd_inf->running_lwps = 0;
      return;
    }

  /* If a SIGSTOP is already pending, don't send a new one, but tell
     wait_1 to report a SIGSTOP.  */
  if (fbsd_inf->pending_sigstop)
    {
      fbsd_nat_debug_printf ("waiting for existing pending SIGSTOP for %u",
			     inf->pid);
      fbsd_inf->pending_sigstop = false;
    }
  else
    {
      /* Ignore errors from kill as process exit might race with kill.  */
      fbsd_nat_debug_printf ("killing %u with SIGSTOP", inf->pid);
      ::kill (inf->pid, SIGSTOP);
    }

  /* Wait for SIGSTOP (or some other event) to be reported.  */
  wptid = wait_1 (ptid, &status, 0);

  switch (status.kind ())
    {
    case TARGET_WAITKIND_EXITED:
    case TARGET_WAITKIND_SIGNALLED:
      /* If the process has exited, we aren't going to get an
	 event for the SIGSTOP.  Save the current event and
	 return.  */
      add_pending_event (wptid, status);
      break;
    case TARGET_WAITKIND_IGNORE:
      /* wait() failed with ECHILD meaning the process no longer
	 exists.  This means a bug happened elsewhere, but at least
	 the process is no longer running.  */
      break;
    case TARGET_WAITKIND_STOPPED:
      /* If this is the SIGSTOP event, discard it and return
	 leaving the process stopped.  */
      if (status.sig () == GDB_SIGNAL_STOP)
	break;

      [[fallthrough]];
    default:
      /* Some other event has occurred.  Save the current
	 event.  */
      add_pending_event (wptid, status);

      /* Ignore the next SIGSTOP for this process.  */
      fbsd_nat_debug_printf ("ignoring next SIGSTOP for %u", inf->pid);
      fbsd_inf->pending_sigstop = true;
      break;
    }
  fbsd_inf->running_lwps = 0;
}

ptid_t
fbsd_nat_target::wait (ptid_t ptid, struct target_waitstatus *ourstatus,
		       target_wait_flags target_options)
{
  fbsd_nat_debug_printf ("[%s], [%s]", target_pid_to_str (ptid).c_str (),
			 target_options_to_string (target_options).c_str ());

  /* If there is a valid pending event, return it.  */
  std::optional<pending_event> event = take_pending_event (ptid);
  if (event.has_value ())
    {
      /* Stop any other inferiors currently running.  */
      for (inferior *inf : all_non_exited_inferiors (this))
	stop_process (inf);

      fbsd_nat_debug_printf ("returning pending event [%s], [%s]",
			     target_pid_to_str (event->ptid).c_str (),
			     event->status.to_string ().c_str ());
      gdb_assert (event->ptid.matches (ptid));
      *ourstatus = event->status;
      return event->ptid;
    }

  /* Ensure any subsequent events trigger a new event in the loop.  */
  if (is_async_p ())
    async_file_flush ();

  ptid_t wptid;
  while (1)
    {
      wptid = wait_1 (ptid, ourstatus, target_options);

      /* If no event was found, just return.  */
      if (ourstatus->kind () == TARGET_WAITKIND_IGNORE
	  || ourstatus->kind () == TARGET_WAITKIND_NO_RESUMED)
	break;

      inferior *winf = find_inferior_ptid (this, wptid);
      gdb_assert (winf != nullptr);
      fbsd_inferior *fbsd_inf = get_fbsd_inferior (winf);
      gdb_assert (fbsd_inf != nullptr);
      gdb_assert (fbsd_inf->resumed_lwps != null_ptid);
      gdb_assert (fbsd_inf->running_lwps > 0);

      /* If an event is reported for a thread or process while
	 stepping some other thread, suspend the thread reporting the
	 event and defer the event until it can be reported to the
	 core.  */
      if (!wptid.matches (fbsd_inf->resumed_lwps))
	{
	  add_pending_event (wptid, *ourstatus);
	  fbsd_nat_debug_printf ("deferring event [%s], [%s]",
				 target_pid_to_str (wptid).c_str (),
				 ourstatus->to_string ().c_str ());
	  if (ptrace (PT_SUSPEND, wptid.lwp (), NULL, 0) == -1)
	    perror_with_name (("ptrace (PT_SUSPEND)"));
	  if (ptrace (PT_CONTINUE, wptid.pid (), (caddr_t) 1, 0) == -1)
	    perror_with_name (("ptrace (PT_CONTINUE)"));
	  continue;
	}

      /* This process is no longer running.  */
      fbsd_inf->resumed_lwps = null_ptid;
      fbsd_inf->running_lwps = 0;

      /* Stop any other inferiors currently running.  */
      for (inferior *inf : all_non_exited_inferiors (this))
	stop_process (inf);

      break;
    }

  /* If we are in async mode and found an event, there may still be
     another event pending.  Trigger the event pipe so that that the
     event loop keeps polling until no event is returned.  */
  if (is_async_p ()
      && ((ourstatus->kind () != TARGET_WAITKIND_IGNORE
	  && ourstatus->kind () != TARGET_WAITKIND_NO_RESUMED)
	  || ptid != minus_one_ptid))
    async_file_mark ();

  fbsd_nat_debug_printf ("returning [%s], [%s]",
			 target_pid_to_str (wptid).c_str (),
			 ourstatus->to_string ().c_str ());
  return wptid;
}

#ifdef USE_SIGTRAP_SIGINFO
/* Implement the "stopped_by_sw_breakpoint" target_ops method.  */

bool
fbsd_nat_target::stopped_by_sw_breakpoint ()
{
  struct ptrace_lwpinfo pl;

  if (ptrace (PT_LWPINFO, get_ptrace_pid (inferior_ptid), (caddr_t) &pl,
	      sizeof pl) == -1)
    return false;

  return (pl.pl_flags == PL_FLAG_SI
	  && pl.pl_siginfo.si_signo == SIGTRAP
	  && pl.pl_siginfo.si_code == TRAP_BRKPT);
}

/* Implement the "supports_stopped_by_sw_breakpoint" target_ops
   method.  */

bool
fbsd_nat_target::supports_stopped_by_sw_breakpoint ()
{
  return true;
}
#endif

#ifdef PROC_ASLR_CTL
class maybe_disable_address_space_randomization
{
public:
  explicit maybe_disable_address_space_randomization (bool disable_randomization)
  {
    if (disable_randomization)
      {
	if (procctl (P_PID, getpid (), PROC_ASLR_STATUS, &m_aslr_ctl) == -1)
	  {
	    warning (_("Failed to fetch current address space randomization "
		       "status: %s"), safe_strerror (errno));
	    return;
	  }

	m_aslr_ctl &= ~PROC_ASLR_ACTIVE;
	if (m_aslr_ctl == PROC_ASLR_FORCE_DISABLE)
	  return;

	int ctl = PROC_ASLR_FORCE_DISABLE;
	if (procctl (P_PID, getpid (), PROC_ASLR_CTL, &ctl) == -1)
	  {
	    warning (_("Error disabling address space randomization: %s"),
		     safe_strerror (errno));
	    return;
	  }

	m_aslr_ctl_set = true;
      }
  }

  ~maybe_disable_address_space_randomization ()
  {
    if (m_aslr_ctl_set)
      {
	if (procctl (P_PID, getpid (), PROC_ASLR_CTL, &m_aslr_ctl) == -1)
	  warning (_("Error restoring address space randomization: %s"),
		   safe_strerror (errno));
      }
  }

  DISABLE_COPY_AND_ASSIGN (maybe_disable_address_space_randomization);

private:
  bool m_aslr_ctl_set = false;
  int m_aslr_ctl = 0;
};
#endif

void
fbsd_nat_target::create_inferior (const char *exec_file,
				  const std::string &allargs,
				  char **env, int from_tty)
{
#ifdef PROC_ASLR_CTL
  maybe_disable_address_space_randomization restore_aslr_ctl
    (disable_randomization);
#endif

  fbsd_inferior *fbsd_inf = new fbsd_inferior;
  current_inferior ()->priv.reset (fbsd_inf);
  fbsd_inf->resumed_lwps = minus_one_ptid;
  fbsd_inf->num_lwps = 1;
  fbsd_inf->running_lwps = 1;
  inf_ptrace_target::create_inferior (exec_file, allargs, env, from_tty);
}

void
fbsd_nat_target::attach (const char *args, int from_tty)
{
  fbsd_inferior *fbsd_inf = new fbsd_inferior;
  current_inferior ()->priv.reset (fbsd_inf);
  fbsd_inf->resumed_lwps = minus_one_ptid;
  fbsd_inf->num_lwps = 1;
  fbsd_inf->running_lwps = 1;
  inf_ptrace_target::attach (args, from_tty);
}

/* If this thread has a pending fork event, there is a child process
   GDB is attached to that the core of GDB doesn't know about.
   Detach from it.  */

void
fbsd_nat_target::detach_fork_children (thread_info *tp)
{
  /* Check in thread_info::pending_waitstatus.  */
  if (tp->has_pending_waitstatus ())
    {
      const target_waitstatus &ws = tp->pending_waitstatus ();

      if (ws.kind () == TARGET_WAITKIND_VFORKED
	  || ws.kind () == TARGET_WAITKIND_FORKED)
	{
	  pid_t pid = ws.child_ptid ().pid ();
	  fbsd_nat_debug_printf ("detaching from child %d", pid);
	  (void) ptrace (PT_DETACH, pid, (caddr_t) 1, 0);
	}
    }

  /* Check in thread_info::pending_follow.  */
  if (tp->pending_follow.kind () == TARGET_WAITKIND_VFORKED
      || tp->pending_follow.kind () == TARGET_WAITKIND_FORKED)
    {
      pid_t pid = tp->pending_follow.child_ptid ().pid ();
      fbsd_nat_debug_printf ("detaching from child %d", pid);
      (void) ptrace (PT_DETACH, pid, (caddr_t) 1, 0);
    }
}

/* Detach from any child processes associated with pending fork events
   for a stopped process.  Returns true if the process has terminated
   and false if it is still alive.  */

bool
fbsd_nat_target::detach_fork_children (inferior *inf)
{
  /* Detach any child processes associated with pending fork events in
     threads belonging to this process.  */
  for (thread_info *tp : inf->non_exited_threads ())
    detach_fork_children (tp);

  /* Unwind state associated with any pending events.  Reset
     fbsd_inf->resumed_lwps so that take_pending_event will harvest
     events.  */
  fbsd_inferior *fbsd_inf = get_fbsd_inferior (inf);
  ptid_t ptid = ptid_t (inf->pid);
  fbsd_inf->resumed_lwps = ptid;

  while (1)
    {
      std::optional<pending_event> event = take_pending_event (ptid);
      if (!event.has_value ())
	break;

      switch (event->status.kind ())
	{
	case TARGET_WAITKIND_EXITED:
	case TARGET_WAITKIND_SIGNALLED:
	  return true;
	case TARGET_WAITKIND_FORKED:
	case TARGET_WAITKIND_VFORKED:
	  {
	    pid_t pid = event->status.child_ptid ().pid ();
	    fbsd_nat_debug_printf ("detaching from child %d", pid);
	    (void) ptrace (PT_DETACH, pid, (caddr_t) 1, 0);
	  }
	  break;
	}
    }
  return false;
}

/* Scan all of the threads for a stopped process invoking the supplied
   callback on the ptrace_lwpinfo object for threads other than the
   thread which reported the current stop.  The callback can return
   true to terminate the iteration early.  This function returns true
   if the callback returned true, otherwise it returns false.  */

typedef bool (ptrace_event_ftype) (const struct ptrace_lwpinfo &pl);

static bool
iterate_other_ptrace_events (pid_t pid,
			     gdb::function_view<ptrace_event_ftype> callback)
{
  /* Fetch the LWP ID of the thread that just reported the last stop
     and ignore that LWP in the following loop.  */
  ptrace_lwpinfo pl;
  if (ptrace (PT_LWPINFO, pid, (caddr_t) &pl, sizeof (pl)) != 0)
    perror_with_name (("ptrace (PT_LWPINFO)"));
  lwpid_t lwpid = pl.pl_lwpid;

  int nlwps = ptrace (PT_GETNUMLWPS, pid, NULL, 0);
  if (nlwps == -1)
    perror_with_name (("ptrace (PT_GETLWPLIST)"));
  if (nlwps == 1)
    return false;

  gdb::unique_xmalloc_ptr<lwpid_t[]> lwps (XCNEWVEC (lwpid_t, nlwps));

  nlwps = ptrace (PT_GETLWPLIST, pid, (caddr_t) lwps.get (), nlwps);
  if (nlwps == -1)
    perror_with_name (("ptrace (PT_GETLWPLIST)"));

  for (int i = 0; i < nlwps; i++)
    {
      if (lwps[i] == lwpid)
	continue;

      if (ptrace (PT_LWPINFO, lwps[i], (caddr_t) &pl, sizeof (pl)) != 0)
	perror_with_name (("ptrace (PT_LWPINFO)"));

      if (callback (pl))
	return true;
    }
  return false;
}

/* True if there are any stopped threads with an interesting event.  */

static bool
pending_ptrace_events (inferior *inf)
{
  auto lambda = [] (const struct ptrace_lwpinfo &pl)
  {
#if defined(PT_LWP_EVENTS) && __FreeBSD_kernel_version < 1400090
    if (pl.pl_flags == PL_FLAG_BORN)
      return true;
#endif
#ifdef TDP_RFPPWAIT
    if (pl.pl_flags & PL_FLAG_FORKED)
      return true;
#endif
    if (pl.pl_event == PL_EVENT_SIGNAL)
      {
	if ((pl.pl_flags & PL_FLAG_SI) == 0)
	  {
	    /* Not sure which signal, assume it matters.  */
	    return true;
	  }
	if (pl.pl_siginfo.si_signo == SIGTRAP)
	  return true;
      }
    return false;
  };
  return iterate_other_ptrace_events (inf->pid,
				      gdb::make_function_view (lambda));
}

void
fbsd_nat_target::detach (inferior *inf, int from_tty)
{
  fbsd_nat_debug_start_end ("pid %d", inf->pid);

  stop_process (inf);

  remove_breakpoints_inf (inf);

  if (detach_fork_children (inf)) {
    /* No need to detach now.  */
    target_announce_detach (from_tty);

    detach_success (inf);
    return;
  }

  /* If there are any pending events (SIGSTOP from stop_process or a
     breakpoint hit that needs a PC fixup), drain events until the
     process can be safely detached.  */
  fbsd_inferior *fbsd_inf = get_fbsd_inferior (inf);
  ptid_t ptid = ptid_t (inf->pid);
  if (fbsd_inf->pending_sigstop || pending_ptrace_events (inf))
    {
      bool pending_sigstop = fbsd_inf->pending_sigstop;
      int sig = 0;

      if (pending_sigstop)
	fbsd_nat_debug_printf ("waiting for SIGSTOP");

      /* Force wait_1 to report the SIGSTOP instead of swallowing it.  */
      fbsd_inf->pending_sigstop = false;

      /* Report event for all threads from wait_1.  */
      fbsd_inf->resumed_lwps = ptid;

      do
	{
	  if (ptrace (PT_CONTINUE, inf->pid, (caddr_t) 1, sig) != 0)
	    perror_with_name (("ptrace(PT_CONTINUE)"));

	  target_waitstatus ws;
	  ptid_t wptid = wait_1 (ptid, &ws, 0);

	  switch (ws.kind ())
	    {
	    case TARGET_WAITKIND_EXITED:
	    case TARGET_WAITKIND_SIGNALLED:
	      /* No need to detach now.  */
	      target_announce_detach (from_tty);

	      detach_success (inf);
	      return;
	    case TARGET_WAITKIND_FORKED:
	    case TARGET_WAITKIND_VFORKED:
	      {
		pid_t pid = ws.child_ptid ().pid ();
		fbsd_nat_debug_printf ("detaching from child %d", pid);
		(void) ptrace (PT_DETACH, pid, (caddr_t) 1, 0);
		sig = 0;
	      }
	      break;
	    case TARGET_WAITKIND_STOPPED:
	      sig = gdb_signal_to_host (ws.sig ());
	      switch (sig)
		{
		case SIGSTOP:
		  if (pending_sigstop)
		    {
		      sig = 0;
		      pending_sigstop = false;
		    }
		  break;
		case SIGTRAP:
#ifndef USE_SIGTRAP_SIGINFO
		  {
		    /* Update PC from software breakpoint hit.  */
		    struct regcache *regcache = get_thread_regcache (this, wptid);
		    struct gdbarch *gdbarch = regcache->arch ();
		    int decr_pc = gdbarch_decr_pc_after_break (gdbarch);

		    if (decr_pc != 0)
		      {
			CORE_ADDR pc;

			pc = regcache_read_pc (regcache);
			if (breakpoint_inserted_here_p (regcache->aspace (),
							pc - decr_pc))
			  {
			    fbsd_nat_debug_printf ("adjusted PC for LWP %ld",
						   wptid.lwp ());
			    regcache_write_pc (regcache, pc - decr_pc);
			  }
		      }
		  }
#else
		  /* pacify gcc  */
		  (void) wptid;
#endif
		  sig = 0;
		  break;
		}
	    }
	}
      while (pending_sigstop || pending_ptrace_events (inf));
    }

  target_announce_detach (from_tty);

  if (ptrace (PT_DETACH, inf->pid, (caddr_t) 1, 0) == -1)
	perror_with_name (("ptrace (PT_DETACH)"));

  detach_success (inf);
}

/* Implement the "kill" target method.  */

void
fbsd_nat_target::kill ()
{
  pid_t pid = inferior_ptid.pid ();
  if (pid == 0)
    return;

  inferior *inf = current_inferior ();
  stop_process (inf);

  if (detach_fork_children (inf)) {
    /* No need to kill now.  */
    target_mourn_inferior (inferior_ptid);

    return;
  }

#ifdef TDP_RFPPWAIT
  /* If there are any threads that have forked a new child but not yet
     reported it because other threads reported events first, detach
     from the children before killing the parent.  */
  auto lambda = [] (const struct ptrace_lwpinfo &pl)
  {
    if (pl.pl_flags & PL_FLAG_FORKED)
      {
	pid_t child = pl.pl_child_pid;

	/* If the child hasn't reported its stop yet, wait for it to
	   stop.  */
	fbsd_wait_for_fork_child (child);

	/* Detach from the child.  */
	(void) ptrace (PT_DETACH, child, (caddr_t) 1, 0);
      }
    return false;
  };
  iterate_other_ptrace_events (pid, gdb::make_function_view (lambda));
#endif

  if (ptrace (PT_KILL, pid, NULL, 0) == -1)
	perror_with_name (("ptrace (PT_KILL)"));

  int status;
  waitpid (pid, &status, 0);

  target_mourn_inferior (inferior_ptid);
}

void
fbsd_nat_target::mourn_inferior ()
{
  gdb_assert (!have_pending_event (ptid_t (current_inferior ()->pid)));
  inf_ptrace_target::mourn_inferior ();
}

void
fbsd_nat_target::follow_exec (inferior *follow_inf, ptid_t ptid,
			      const char *execd_pathname)
{
  inferior *orig_inf = current_inferior ();

  inf_ptrace_target::follow_exec (follow_inf, ptid, execd_pathname);

  if (orig_inf != follow_inf)
    {
      /* Migrate the fbsd_inferior to the new inferior. */
      follow_inf->priv.reset (orig_inf->priv.release ());
    }
}

#ifdef TDP_RFPPWAIT
/* Target hook for follow_fork.  On entry and at return inferior_ptid is
   the ptid of the followed inferior.  */

void
fbsd_nat_target::follow_fork (inferior *child_inf, ptid_t child_ptid,
			      target_waitkind fork_kind, bool follow_child,
			      bool detach_fork)
{
  inf_ptrace_target::follow_fork (child_inf, child_ptid, fork_kind,
				  follow_child, detach_fork);

  if (child_inf != nullptr)
    {
      fbsd_inferior *fbsd_inf = new fbsd_inferior;
      child_inf->priv.reset (fbsd_inf);
      fbsd_inf->num_lwps = 1;
    }

  if (!follow_child && detach_fork)
    {
      pid_t child_pid = child_ptid.pid ();

      /* Breakpoints have already been detached from the child by
	 infrun.c.  */

      if (ptrace (PT_DETACH, child_pid, (PTRACE_TYPE_ARG3) 1, 0) == -1)
	perror_with_name (("ptrace (PT_DETACH)"));

#ifndef PTRACE_VFORK
      if (fork_kind () == TARGET_WAITKIND_VFORKED)
	{
	  /* We can't insert breakpoints until the child process has
	     finished with the shared memory region.  The parent
	     process doesn't wait for the child process to exit or
	     exec until after it has been resumed from the ptrace stop
	     to report the fork.  Once it has been resumed it doesn't
	     stop again before returning to userland, so there is no
	     reliable way to wait on the parent.

	     We can't stay attached to the child to wait for an exec
	     or exit because it may invoke ptrace(PT_TRACE_ME)
	     (e.g. if the parent process is a debugger forking a new
	     child process).

	     In the end, the best we can do is to make sure it runs
	     for a little while.  Hopefully it will be out of range of
	     any breakpoints we reinsert.  Usually this is only the
	     single-step breakpoint at vfork's return point.  */

	  usleep (10000);

	  /* Schedule a fake VFORK_DONE event to report on the next
	     wait.  */
	  fbsd_add_vfork_done (inferior_ptid);
	}
#endif
    }
}

int
fbsd_nat_target::insert_fork_catchpoint (int pid)
{
  return 0;
}

int
fbsd_nat_target::remove_fork_catchpoint (int pid)
{
  return 0;
}

int
fbsd_nat_target::insert_vfork_catchpoint (int pid)
{
  return 0;
}

int
fbsd_nat_target::remove_vfork_catchpoint (int pid)
{
  return 0;
}
#endif

/* Implement the virtual inf_ptrace_target::post_startup_inferior method.  */

void
fbsd_nat_target::post_startup_inferior (ptid_t pid)
{
  fbsd_enable_proc_events (pid.pid ());
}

/* Implement the "post_attach" target_ops method.  */

void
fbsd_nat_target::post_attach (int pid)
{
  fbsd_enable_proc_events (pid);
  fbsd_add_threads (this, pid);
}

/* Traced processes always stop after exec.  */

int
fbsd_nat_target::insert_exec_catchpoint (int pid)
{
  return 0;
}

int
fbsd_nat_target::remove_exec_catchpoint (int pid)
{
  return 0;
}

#ifdef HAVE_STRUCT_PTRACE_LWPINFO_PL_SYSCALL_CODE
int
fbsd_nat_target::set_syscall_catchpoint (int pid, bool needed,
					 int any_count,
					 gdb::array_view<const int> syscall_counts)
{

  /* Ignore the arguments.  inf-ptrace.c will use PT_SYSCALL which
     will catch all system call entries and exits.  The system calls
     are filtered by GDB rather than the kernel.  */
  return 0;
}
#endif

bool
fbsd_nat_target::supports_multi_process ()
{
  return true;
}

bool
fbsd_nat_target::supports_disable_randomization ()
{
#ifdef PROC_ASLR_CTL
  return true;
#else
  return false;
#endif
}

/* See fbsd-nat.h.  */

bool
fbsd_nat_target::fetch_register_set (struct regcache *regcache, int regnum,
				     int fetch_op, const struct regset *regset,
				     int regbase, void *regs, size_t size)
{
  const struct regcache_map_entry *map
    = (const struct regcache_map_entry *) regset->regmap;
  pid_t pid = get_ptrace_pid (regcache->ptid ());

  if (regnum == -1
      || (regnum >= regbase && regcache_map_supplies (map, regnum - regbase,
						      regcache->arch (), size)))
    {
      if (ptrace (fetch_op, pid, (PTRACE_TYPE_ARG3) regs, 0) == -1)
	perror_with_name (_("Couldn't get registers"));

      regset->supply_regset (regset, regcache, regnum, regs, size);
      return true;
    }
  return false;
}

/* See fbsd-nat.h.  */

bool
fbsd_nat_target::store_register_set (struct regcache *regcache, int regnum,
				     int fetch_op, int store_op,
				     const struct regset *regset, int regbase,
				     void *regs, size_t size)
{
  const struct regcache_map_entry *map
    = (const struct regcache_map_entry *) regset->regmap;
  pid_t pid = get_ptrace_pid (regcache->ptid ());

  if (regnum == -1
      || (regnum >= regbase && regcache_map_supplies (map, regnum - regbase,
						      regcache->arch (), size)))
    {
      if (ptrace (fetch_op, pid, (PTRACE_TYPE_ARG3) regs, 0) == -1)
	perror_with_name (_("Couldn't get registers"));

      regset->collect_regset (regset, regcache, regnum, regs, size);

      if (ptrace (store_op, pid, (PTRACE_TYPE_ARG3) regs, 0) == -1)
	perror_with_name (_("Couldn't write registers"));
      return true;
    }
  return false;
}

/* See fbsd-nat.h.  */

size_t
fbsd_nat_target::have_regset (ptid_t ptid, int note)
{
  pid_t pid = get_ptrace_pid (ptid);
  struct iovec iov;

  iov.iov_base = nullptr;
  iov.iov_len = 0;
  if (ptrace (PT_GETREGSET, pid, (PTRACE_TYPE_ARG3) &iov, note) == -1)
    return 0;
  return iov.iov_len;
}

/* See fbsd-nat.h.  */

bool
fbsd_nat_target::fetch_regset (struct regcache *regcache, int regnum, int note,
			       const struct regset *regset, int regbase,
			       void *regs, size_t size)
{
  const struct regcache_map_entry *map
    = (const struct regcache_map_entry *) regset->regmap;
  pid_t pid = get_ptrace_pid (regcache->ptid ());

  if (regnum == -1
      || (regnum >= regbase && regcache_map_supplies (map, regnum - regbase,
						      regcache->arch (), size)))
    {
      struct iovec iov;

      iov.iov_base = regs;
      iov.iov_len = size;
      if (ptrace (PT_GETREGSET, pid, (PTRACE_TYPE_ARG3) &iov, note) == -1)
	perror_with_name (_("Couldn't get registers"));

      regset->supply_regset (regset, regcache, regnum, regs, size);
      return true;
    }
  return false;
}

bool
fbsd_nat_target::store_regset (struct regcache *regcache, int regnum, int note,
			       const struct regset *regset, int regbase,
			       void *regs, size_t size)
{
  const struct regcache_map_entry *map
    = (const struct regcache_map_entry *) regset->regmap;
  pid_t pid = get_ptrace_pid (regcache->ptid ());

  if (regnum == -1
      || (regnum >= regbase && regcache_map_supplies (map, regnum - regbase,
						      regcache->arch (), size)))
    {
      struct iovec iov;

      iov.iov_base = regs;
      iov.iov_len = size;
      if (ptrace (PT_GETREGSET, pid, (PTRACE_TYPE_ARG3) &iov, note) == -1)
	perror_with_name (_("Couldn't get registers"));

      regset->collect_regset (regset, regcache, regnum, regs, size);

      if (ptrace (PT_SETREGSET, pid, (PTRACE_TYPE_ARG3) &iov, note) == -1)
	perror_with_name (_("Couldn't write registers"));
      return true;
    }
  return false;
}

/* See fbsd-nat.h.  */

bool
fbsd_nat_get_siginfo (ptid_t ptid, siginfo_t *siginfo)
{
  struct ptrace_lwpinfo pl;
  pid_t pid = get_ptrace_pid (ptid);

  if (ptrace (PT_LWPINFO, pid, (caddr_t) &pl, sizeof pl) == -1)
    return false;
  if (!(pl.pl_flags & PL_FLAG_SI))
    return false;;
  *siginfo = pl.pl_siginfo;
  return (true);
}

void _initialize_fbsd_nat ();
void
_initialize_fbsd_nat ()
{
  add_setshow_boolean_cmd ("fbsd-lwp", class_maintenance,
			   &debug_fbsd_lwp, _("\
Set debugging of FreeBSD lwp module."), _("\
Show debugging of FreeBSD lwp module."), _("\
Enables printf debugging output."),
			   NULL,
			   &show_fbsd_lwp_debug,
			   &setdebuglist, &showdebuglist);
  add_setshow_boolean_cmd ("fbsd-nat", class_maintenance,
			   &debug_fbsd_nat, _("\
Set debugging of FreeBSD native target."), _("\
Show debugging of FreeBSD native target."), _("\
Enables printf debugging output."),
			   NULL,
			   &show_fbsd_nat_debug,
			   &setdebuglist, &showdebuglist);

  /* Install a SIGCHLD handler.  */
  signal (SIGCHLD, sigchld_handler);
}
