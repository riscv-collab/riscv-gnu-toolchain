/* Target-dependent code for FreeBSD, architecture-independent.

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
#include "auxv.h"
#include "gdbcore.h"
#include "inferior.h"
#include "objfiles.h"
#include "regcache.h"
#include "regset.h"
#include "gdbthread.h"
#include "objfiles.h"
#include "xml-syscall.h"
#include <sys/socket.h>
#include <arpa/inet.h>

#include "elf-bfd.h"
#include "fbsd-tdep.h"
#include "gcore-elf.h"

/* This enum is derived from FreeBSD's <sys/signal.h>.  */

enum
  {
    FREEBSD_SIGHUP = 1,
    FREEBSD_SIGINT = 2,
    FREEBSD_SIGQUIT = 3,
    FREEBSD_SIGILL = 4,
    FREEBSD_SIGTRAP = 5,
    FREEBSD_SIGABRT = 6,
    FREEBSD_SIGEMT = 7,
    FREEBSD_SIGFPE = 8,
    FREEBSD_SIGKILL = 9,
    FREEBSD_SIGBUS = 10,
    FREEBSD_SIGSEGV = 11,
    FREEBSD_SIGSYS = 12,
    FREEBSD_SIGPIPE = 13,
    FREEBSD_SIGALRM = 14,
    FREEBSD_SIGTERM = 15,
    FREEBSD_SIGURG = 16,
    FREEBSD_SIGSTOP = 17,
    FREEBSD_SIGTSTP = 18,
    FREEBSD_SIGCONT = 19,
    FREEBSD_SIGCHLD = 20,
    FREEBSD_SIGTTIN = 21,
    FREEBSD_SIGTTOU = 22,
    FREEBSD_SIGIO = 23,
    FREEBSD_SIGXCPU = 24,
    FREEBSD_SIGXFSZ = 25,
    FREEBSD_SIGVTALRM = 26,
    FREEBSD_SIGPROF = 27,
    FREEBSD_SIGWINCH = 28,
    FREEBSD_SIGINFO = 29,
    FREEBSD_SIGUSR1 = 30,
    FREEBSD_SIGUSR2 = 31,
    FREEBSD_SIGTHR = 32,
    FREEBSD_SIGLIBRT = 33,
    FREEBSD_SIGRTMIN = 65,
    FREEBSD_SIGRTMAX = 126,
  };

/* Constants for values of si_code as defined in FreeBSD's
   <sys/signal.h>.  */

#define	FBSD_SI_USER		0x10001
#define	FBSD_SI_QUEUE		0x10002
#define	FBSD_SI_TIMER		0x10003
#define	FBSD_SI_ASYNCIO		0x10004
#define	FBSD_SI_MESGQ		0x10005
#define	FBSD_SI_KERNEL		0x10006
#define	FBSD_SI_LWP		0x10007

#define	FBSD_ILL_ILLOPC		1
#define	FBSD_ILL_ILLOPN		2
#define	FBSD_ILL_ILLADR		3
#define	FBSD_ILL_ILLTRP		4
#define	FBSD_ILL_PRVOPC		5
#define	FBSD_ILL_PRVREG		6
#define	FBSD_ILL_COPROC		7
#define	FBSD_ILL_BADSTK		8

#define	FBSD_BUS_ADRALN		1
#define	FBSD_BUS_ADRERR		2
#define	FBSD_BUS_OBJERR		3
#define	FBSD_BUS_OOMERR		100

#define	FBSD_SEGV_MAPERR	1
#define	FBSD_SEGV_ACCERR	2
#define	FBSD_SEGV_PKUERR	100

#define	FBSD_FPE_INTOVF		1
#define	FBSD_FPE_INTDIV		2
#define	FBSD_FPE_FLTDIV		3
#define	FBSD_FPE_FLTOVF		4
#define	FBSD_FPE_FLTUND		5
#define	FBSD_FPE_FLTRES		6
#define	FBSD_FPE_FLTINV		7
#define	FBSD_FPE_FLTSUB		8

#define	FBSD_TRAP_BRKPT		1
#define	FBSD_TRAP_TRACE		2
#define	FBSD_TRAP_DTRACE	3
#define	FBSD_TRAP_CAP		4

#define	FBSD_CLD_EXITED		1
#define	FBSD_CLD_KILLED		2
#define	FBSD_CLD_DUMPED		3
#define	FBSD_CLD_TRAPPED	4
#define	FBSD_CLD_STOPPED	5
#define	FBSD_CLD_CONTINUED	6

#define	FBSD_POLL_IN		1
#define	FBSD_POLL_OUT		2
#define	FBSD_POLL_MSG		3
#define	FBSD_POLL_ERR		4
#define	FBSD_POLL_PRI		5
#define	FBSD_POLL_HUP		6

/* FreeBSD kernels 12.0 and later include a copy of the
   'ptrace_lwpinfo' structure returned by the PT_LWPINFO ptrace
   operation in an ELF core note (NT_FREEBSD_PTLWPINFO) for each LWP.
   The constants below define the offset of field members and flags in
   this structure used by methods in this file.  Note that the
   'ptrace_lwpinfo' struct in the note is preceded by a 4 byte integer
   containing the size of the structure.  */

#define	LWPINFO_OFFSET		0x4

/* Offsets in ptrace_lwpinfo.  */
#define	LWPINFO_PL_FLAGS	0x8
#define	LWPINFO64_PL_SIGINFO	0x30
#define	LWPINFO32_PL_SIGINFO	0x2c

/* Flags in pl_flags.  */
#define	PL_FLAG_SI	0x20	/* siginfo is valid */

/* Sizes of siginfo_t.	*/
#define	SIZE64_SIGINFO_T	80
#define	SIZE32_SIGINFO_T	64

/* Offsets in data structure used in NT_FREEBSD_PROCSTAT_VMMAP core
   dump notes.  See <sys/user.h> for the definition of struct
   kinfo_vmentry.  This data structure should have the same layout on
   all architectures.

   Note that FreeBSD 7.0 used an older version of this structure
   (struct kinfo_vmentry), but the NT_FREEBSD_PROCSTAT_VMMAP core
   dump note wasn't introduced until FreeBSD 9.2.  As a result, the
   core dump note has always used the 7.1 and later structure
   format.  */

#define	KVE_STRUCTSIZE		0x0
#define	KVE_START		0x8
#define	KVE_END			0x10
#define	KVE_OFFSET		0x18
#define	KVE_FLAGS		0x2c
#define	KVE_PROTECTION		0x38
#define	KVE_PATH		0x88

/* Flags in the 'kve_protection' field in struct kinfo_vmentry.  These
   match the KVME_PROT_* constants in <sys/user.h>.  */

#define	KINFO_VME_PROT_READ	0x00000001
#define	KINFO_VME_PROT_WRITE	0x00000002
#define	KINFO_VME_PROT_EXEC	0x00000004

/* Flags in the 'kve_flags' field in struct kinfo_vmentry.  These
   match the KVME_FLAG_* constants in <sys/user.h>.  */

#define	KINFO_VME_FLAG_COW		0x00000001
#define	KINFO_VME_FLAG_NEEDS_COPY	0x00000002
#define	KINFO_VME_FLAG_NOCOREDUMP	0x00000004
#define	KINFO_VME_FLAG_SUPER		0x00000008
#define	KINFO_VME_FLAG_GROWS_UP		0x00000010
#define	KINFO_VME_FLAG_GROWS_DOWN	0x00000020

/* Offsets in data structure used in NT_FREEBSD_PROCSTAT_FILES core
   dump notes.  See <sys/user.h> for the definition of struct
   kinfo_file.  This data structure should have the same layout on all
   architectures.

   Note that FreeBSD 7.0 used an older version of this structure
   (struct kinfo_ofile), but the NT_FREEBSD_PROCSTAT_FILES core dump
   note wasn't introduced until FreeBSD 9.2.  As a result, the core
   dump note has always used the 7.1 and later structure format.  */

#define	KF_STRUCTSIZE		0x0
#define	KF_TYPE			0x4
#define	KF_FD			0x8
#define	KF_FLAGS		0x10
#define	KF_OFFSET		0x18
#define	KF_VNODE_TYPE		0x20
#define	KF_SOCK_DOMAIN		0x24
#define	KF_SOCK_TYPE		0x28
#define	KF_SOCK_PROTOCOL	0x2c
#define	KF_SA_LOCAL		0x30
#define	KF_SA_PEER		0xb0
#define	KF_PATH			0x170

/* Constants for the 'kf_type' field in struct kinfo_file.  These
   match the KF_TYPE_* constants in <sys/user.h>.  */

#define	KINFO_FILE_TYPE_VNODE	1
#define	KINFO_FILE_TYPE_SOCKET	2
#define	KINFO_FILE_TYPE_PIPE	3
#define	KINFO_FILE_TYPE_FIFO	4
#define	KINFO_FILE_TYPE_KQUEUE	5
#define	KINFO_FILE_TYPE_CRYPTO	6
#define	KINFO_FILE_TYPE_MQUEUE	7
#define	KINFO_FILE_TYPE_SHM	8
#define	KINFO_FILE_TYPE_SEM	9
#define	KINFO_FILE_TYPE_PTS	10
#define	KINFO_FILE_TYPE_PROCDESC 11

/* Special values for the 'kf_fd' field in struct kinfo_file.  These
   match the KF_FD_TYPE_* constants in <sys/user.h>.  */

#define	KINFO_FILE_FD_TYPE_CWD	-1
#define	KINFO_FILE_FD_TYPE_ROOT	-2
#define	KINFO_FILE_FD_TYPE_JAIL	-3
#define	KINFO_FILE_FD_TYPE_TRACE -4
#define	KINFO_FILE_FD_TYPE_TEXT	-5
#define	KINFO_FILE_FD_TYPE_CTTY	-6

/* Flags in the 'kf_flags' field in struct kinfo_file.  These match
   the KF_FLAG_* constants in <sys/user.h>.  */

#define	KINFO_FILE_FLAG_READ		0x00000001
#define	KINFO_FILE_FLAG_WRITE		0x00000002
#define	KINFO_FILE_FLAG_APPEND		0x00000004
#define	KINFO_FILE_FLAG_ASYNC		0x00000008
#define	KINFO_FILE_FLAG_FSYNC		0x00000010
#define	KINFO_FILE_FLAG_NONBLOCK	0x00000020
#define	KINFO_FILE_FLAG_DIRECT		0x00000040
#define	KINFO_FILE_FLAG_HASLOCK		0x00000080
#define	KINFO_FILE_FLAG_EXEC		0x00004000

/* Constants for the 'kf_vnode_type' field in struct kinfo_file.
   These match the KF_VTYPE_* constants in <sys/user.h>.  */

#define	KINFO_FILE_VTYPE_VREG	1
#define	KINFO_FILE_VTYPE_VDIR	2
#define	KINFO_FILE_VTYPE_VCHR	4
#define	KINFO_FILE_VTYPE_VLNK	5
#define	KINFO_FILE_VTYPE_VSOCK	6
#define	KINFO_FILE_VTYPE_VFIFO	7

/* Constants for socket address families.  These match AF_* constants
   in <sys/socket.h>.  */

#define	FBSD_AF_UNIX		1
#define	FBSD_AF_INET		2
#define	FBSD_AF_INET6		28

/* Constants for socket types.  These match SOCK_* constants in
   <sys/socket.h>.  */

#define	FBSD_SOCK_STREAM	1
#define	FBSD_SOCK_DGRAM		2
#define	FBSD_SOCK_SEQPACKET	5

/* Constants for IP protocols.  These match IPPROTO_* constants in
   <netinet/in.h>.  */

#define	FBSD_IPPROTO_ICMP	1
#define	FBSD_IPPROTO_TCP	6
#define	FBSD_IPPROTO_UDP	17
#define	FBSD_IPPROTO_SCTP	132

/* Socket address structures.  These have the same layout on all
   FreeBSD architectures.  In addition, multibyte fields such as IP
   addresses are always stored in network byte order.  */

struct fbsd_sockaddr_in
{
  uint8_t sin_len;
  uint8_t sin_family;
  uint8_t sin_port[2];
  uint8_t sin_addr[4];
  char sin_zero[8];
};

struct fbsd_sockaddr_in6
{
  uint8_t sin6_len;
  uint8_t sin6_family;
  uint8_t sin6_port[2];
  uint32_t sin6_flowinfo;
  uint8_t sin6_addr[16];
  uint32_t sin6_scope_id;
};

struct fbsd_sockaddr_un
{
  uint8_t sun_len;
  uint8_t sun_family;
  char sun_path[104];
};

/* Number of 32-bit words in a signal set.  This matches _SIG_WORDS in
   <sys/_sigset.h> and is the same value on all architectures.  */

#define	SIG_WORDS		4

/* Offsets in data structure used in NT_FREEBSD_PROCSTAT_PROC core
   dump notes.  See <sys/user.h> for the definition of struct
   kinfo_proc.  This data structure has different layouts on different
   architectures mostly due to ILP32 vs LP64.  However, FreeBSD/i386
   uses a 32-bit time_t while all other architectures use a 64-bit
   time_t.

   The core dump note actually contains one kinfo_proc structure for
   each thread, but all of the process-wide data can be obtained from
   the first structure.  One result of this note's format is that some
   of the process-wide status available in the native target method
   from the kern.proc.pid.<pid> sysctl such as ki_stat and ki_siglist
   is not available from a core dump.  Instead, the per-thread data
   structures contain the value of these fields for individual
   threads.  */

struct kinfo_proc_layout
{
  /* Offsets of struct kinfo_proc members.  */
  int ki_layout;
  int ki_pid;
  int ki_ppid;
  int ki_pgid;
  int ki_tpgid;
  int ki_sid;
  int ki_tdev_freebsd11;
  int ki_sigignore;
  int ki_sigcatch;
  int ki_uid;
  int ki_ruid;
  int ki_svuid;
  int ki_rgid;
  int ki_svgid;
  int ki_ngroups;
  int ki_groups;
  int ki_size;
  int ki_rssize;
  int ki_tsize;
  int ki_dsize;
  int ki_ssize;
  int ki_start;
  int ki_nice;
  int ki_comm;
  int ki_tdev;
  int ki_rusage;
  int ki_rusage_ch;

  /* Offsets of struct rusage members.  */
  int ru_utime;
  int ru_stime;
  int ru_maxrss;
  int ru_minflt;
  int ru_majflt;
};

const struct kinfo_proc_layout kinfo_proc_layout_32 =
  {
    .ki_layout = 0x4,
    .ki_pid = 0x28,
    .ki_ppid = 0x2c,
    .ki_pgid = 0x30,
    .ki_tpgid = 0x34,
    .ki_sid = 0x38,
    .ki_tdev_freebsd11 = 0x44,
    .ki_sigignore = 0x68,
    .ki_sigcatch = 0x78,
    .ki_uid = 0x88,
    .ki_ruid = 0x8c,
    .ki_svuid = 0x90,
    .ki_rgid = 0x94,
    .ki_svgid = 0x98,
    .ki_ngroups = 0x9c,
    .ki_groups = 0xa0,
    .ki_size = 0xe0,
    .ki_rssize = 0xe4,
    .ki_tsize = 0xec,
    .ki_dsize = 0xf0,
    .ki_ssize = 0xf4,
    .ki_start = 0x118,
    .ki_nice = 0x145,
    .ki_comm = 0x17f,
    .ki_tdev = 0x1f0,
    .ki_rusage = 0x220,
    .ki_rusage_ch = 0x278,

    .ru_utime = 0x0,
    .ru_stime = 0x10,
    .ru_maxrss = 0x20,
    .ru_minflt = 0x30,
    .ru_majflt = 0x34,
  };

const struct kinfo_proc_layout kinfo_proc_layout_i386 =
  {
    .ki_layout = 0x4,
    .ki_pid = 0x28,
    .ki_ppid = 0x2c,
    .ki_pgid = 0x30,
    .ki_tpgid = 0x34,
    .ki_sid = 0x38,
    .ki_tdev_freebsd11 = 0x44,
    .ki_sigignore = 0x68,
    .ki_sigcatch = 0x78,
    .ki_uid = 0x88,
    .ki_ruid = 0x8c,
    .ki_svuid = 0x90,
    .ki_rgid = 0x94,
    .ki_svgid = 0x98,
    .ki_ngroups = 0x9c,
    .ki_groups = 0xa0,
    .ki_size = 0xe0,
    .ki_rssize = 0xe4,
    .ki_tsize = 0xec,
    .ki_dsize = 0xf0,
    .ki_ssize = 0xf4,
    .ki_start = 0x118,
    .ki_nice = 0x135,
    .ki_comm = 0x16f,
    .ki_tdev = 0x1e0,
    .ki_rusage = 0x210,
    .ki_rusage_ch = 0x258,

    .ru_utime = 0x0,
    .ru_stime = 0x8,
    .ru_maxrss = 0x10,
    .ru_minflt = 0x20,
    .ru_majflt = 0x24,
  };

const struct kinfo_proc_layout kinfo_proc_layout_64 =
  {
    .ki_layout = 0x4,
    .ki_pid = 0x48,
    .ki_ppid = 0x4c,
    .ki_pgid = 0x50,
    .ki_tpgid = 0x54,
    .ki_sid = 0x58,
    .ki_tdev_freebsd11 = 0x64,
    .ki_sigignore = 0x88,
    .ki_sigcatch = 0x98,
    .ki_uid = 0xa8,
    .ki_ruid = 0xac,
    .ki_svuid = 0xb0,
    .ki_rgid = 0xb4,
    .ki_svgid = 0xb8,
    .ki_ngroups = 0xbc,
    .ki_groups = 0xc0,
    .ki_size = 0x100,
    .ki_rssize = 0x108,
    .ki_tsize = 0x118,
    .ki_dsize = 0x120,
    .ki_ssize = 0x128,
    .ki_start = 0x150,
    .ki_nice = 0x185,
    .ki_comm = 0x1bf,
    .ki_tdev = 0x230,
    .ki_rusage = 0x260,
    .ki_rusage_ch = 0x2f0,

    .ru_utime = 0x0,
    .ru_stime = 0x10,
    .ru_maxrss = 0x20,
    .ru_minflt = 0x40,
    .ru_majflt = 0x48,
  };

struct fbsd_gdbarch_data
  {
    struct type *siginfo_type = nullptr;
  };

static const registry<gdbarch>::key<fbsd_gdbarch_data>
     fbsd_gdbarch_data_handle;

static struct fbsd_gdbarch_data *
get_fbsd_gdbarch_data (struct gdbarch *gdbarch)
{
  struct fbsd_gdbarch_data *result = fbsd_gdbarch_data_handle.get (gdbarch);
  if (result == nullptr)
    result = fbsd_gdbarch_data_handle.emplace (gdbarch);
  return result;
}

struct fbsd_pspace_data
{
  /* Offsets in the runtime linker's 'Obj_Entry' structure.  */
  LONGEST off_linkmap = 0;
  LONGEST off_tlsindex = 0;
  bool rtld_offsets_valid = false;

  /* vDSO mapping range.  */
  struct mem_range vdso_range {};

  /* Zero if the range hasn't been searched for, > 0 if a range was
     found, or < 0 if a range was not found.  */
  int vdso_range_p = 0;
};

/* Per-program-space data for FreeBSD architectures.  */
static const registry<program_space>::key<fbsd_pspace_data>
  fbsd_pspace_data_handle;

static struct fbsd_pspace_data *
get_fbsd_pspace_data (struct program_space *pspace)
{
  struct fbsd_pspace_data *data;

  data = fbsd_pspace_data_handle.get (pspace);
  if (data == NULL)
    data = fbsd_pspace_data_handle.emplace (pspace);

  return data;
}

/* This is how we want PTIDs from core files to be printed.  */

static std::string
fbsd_core_pid_to_str (struct gdbarch *gdbarch, ptid_t ptid)
{
  if (ptid.lwp () != 0)
    return string_printf ("LWP %ld", ptid.lwp ());

  return normal_pid_to_str (ptid);
}

/* Extract the name assigned to a thread from a core.  Returns the
   string in a static buffer.  */

static const char *
fbsd_core_thread_name (struct gdbarch *gdbarch, struct thread_info *thr)
{
  static char buf[80];
  struct bfd_section *section;
  bfd_size_type size;

  if (thr->ptid.lwp () != 0)
    {
      /* FreeBSD includes a NT_FREEBSD_THRMISC note for each thread
	 whose contents are defined by a "struct thrmisc" declared in
	 <sys/procfs.h> on FreeBSD.  The per-thread name is stored as
	 a null-terminated string as the first member of the
	 structure.  Rather than define the full structure here, just
	 extract the null-terminated name from the start of the
	 note.  */
      thread_section_name section_name (".thrmisc", thr->ptid);

      section = bfd_get_section_by_name (core_bfd, section_name.c_str ());
      if (section != NULL && bfd_section_size (section) > 0)
	{
	  /* Truncate the name if it is longer than "buf".  */
	  size = bfd_section_size (section);
	  if (size > sizeof buf - 1)
	    size = sizeof buf - 1;
	  if (bfd_get_section_contents (core_bfd, section, buf, (file_ptr) 0,
					size)
	      && buf[0] != '\0')
	    {
	      buf[size] = '\0';

	      /* Note that each thread will report the process command
		 as its thread name instead of an empty name if a name
		 has not been set explicitly.  Return a NULL name in
		 that case.  */
	      if (strcmp (buf, elf_tdata (core_bfd)->core->program) != 0)
		return buf;
	    }
	}
    }

  return NULL;
}

/* Implement the "core_xfer_siginfo" gdbarch method.  */

static LONGEST
fbsd_core_xfer_siginfo (struct gdbarch *gdbarch, gdb_byte *readbuf,
			ULONGEST offset, ULONGEST len)
{
  size_t siginfo_size;

  if (gdbarch_long_bit (gdbarch) == 32)
    siginfo_size = SIZE32_SIGINFO_T;
  else
    siginfo_size = SIZE64_SIGINFO_T;
  if (offset > siginfo_size)
    return -1;

  thread_section_name section_name (".note.freebsdcore.lwpinfo", inferior_ptid);
  asection *section = bfd_get_section_by_name (core_bfd, section_name.c_str ());
  if (section == NULL)
    return -1;

  gdb_byte buf[4];
  if (!bfd_get_section_contents (core_bfd, section, buf,
				 LWPINFO_OFFSET + LWPINFO_PL_FLAGS, 4))
    return -1;

  int pl_flags = extract_signed_integer (buf, gdbarch_byte_order (gdbarch));
  if (!(pl_flags & PL_FLAG_SI))
    return -1;

  if (offset + len > siginfo_size)
    len = siginfo_size - offset;

  ULONGEST siginfo_offset;
  if (gdbarch_long_bit (gdbarch) == 32)
    siginfo_offset = LWPINFO_OFFSET + LWPINFO32_PL_SIGINFO;
  else
    siginfo_offset = LWPINFO_OFFSET + LWPINFO64_PL_SIGINFO;

  if (!bfd_get_section_contents (core_bfd, section, readbuf,
				 siginfo_offset + offset, len))
    return -1;

  return len;
}

static int
find_signalled_thread (struct thread_info *info, void *data)
{
  if (info->stop_signal () != GDB_SIGNAL_0
      && info->ptid.pid () == inferior_ptid.pid ())
    return 1;

  return 0;
}

/* Return a byte_vector containing the contents of a core dump note
   for the target object of type OBJECT.  If STRUCTSIZE is non-zero,
   the data is prefixed with a 32-bit integer size to match the format
   used in FreeBSD NT_PROCSTAT_* notes.  */

static std::optional<gdb::byte_vector>
fbsd_make_note_desc (enum target_object object, uint32_t structsize)
{
  std::optional<gdb::byte_vector> buf =
    target_read_alloc (current_inferior ()->top_target (), object, NULL);
  if (!buf || buf->empty ())
    return {};

  if (structsize == 0)
    return buf;

  gdb::byte_vector desc (sizeof (structsize) + buf->size ());
  memcpy (desc.data (), &structsize, sizeof (structsize));
  std::copy (buf->begin (), buf->end (), desc.data () + sizeof (structsize));
  return desc;
}

/* Create appropriate note sections for a corefile, returning them in
   allocated memory.  */

static gdb::unique_xmalloc_ptr<char>
fbsd_make_corefile_notes (struct gdbarch *gdbarch, bfd *obfd, int *note_size)
{
  gdb::unique_xmalloc_ptr<char> note_data;
  Elf_Internal_Ehdr *i_ehdrp;
  struct thread_info *curr_thr, *signalled_thr;

  /* Put a "FreeBSD" label in the ELF header.  */
  i_ehdrp = elf_elfheader (obfd);
  i_ehdrp->e_ident[EI_OSABI] = ELFOSABI_FREEBSD;

  gdb_assert (gdbarch_iterate_over_regset_sections_p (gdbarch));

  if (get_exec_file (0))
    {
      const char *fname = lbasename (get_exec_file (0));
      std::string psargs = fname;

      const std::string &infargs = current_inferior ()->args ();
      if (!infargs.empty ())
	psargs += ' ' + infargs;

      note_data.reset (elfcore_write_prpsinfo (obfd, note_data.release (),
					       note_size, fname,
					       psargs.c_str ()));
    }

  /* Thread register information.  */
  try
    {
      update_thread_list ();
    }
  catch (const gdb_exception_error &e)
    {
      exception_print (gdb_stderr, e);
    }

  /* Like the kernel, prefer dumping the signalled thread first.
     "First thread" is what tools use to infer the signalled thread.
     In case there's more than one signalled thread, prefer the
     current thread, if it is signalled.  */
  curr_thr = inferior_thread ();
  if (curr_thr->stop_signal () != GDB_SIGNAL_0)
    signalled_thr = curr_thr;
  else
    {
      signalled_thr = iterate_over_threads (find_signalled_thread, NULL);
      if (signalled_thr == NULL)
	signalled_thr = curr_thr;
    }

  enum gdb_signal stop_signal = signalled_thr->stop_signal ();
  gcore_elf_build_thread_register_notes (gdbarch, signalled_thr, stop_signal,
					 obfd, &note_data, note_size);
  for (thread_info *thr : current_inferior ()->non_exited_threads ())
    {
      if (thr == signalled_thr)
	continue;

      gcore_elf_build_thread_register_notes (gdbarch, thr, stop_signal,
					     obfd, &note_data, note_size);
    }

  /* Auxiliary vector.  */
  uint32_t structsize = gdbarch_ptr_bit (gdbarch) / 4; /* Elf_Auxinfo  */
  std::optional<gdb::byte_vector> note_desc =
    fbsd_make_note_desc (TARGET_OBJECT_AUXV, structsize);
  if (note_desc && !note_desc->empty ())
    {
      note_data.reset (elfcore_write_note (obfd, note_data.release (),
					   note_size, "FreeBSD",
					   NT_FREEBSD_PROCSTAT_AUXV,
					   note_desc->data (),
					   note_desc->size ()));
      if (!note_data)
	return NULL;
    }

  /* Virtual memory mappings.  */
  note_desc = fbsd_make_note_desc (TARGET_OBJECT_FREEBSD_VMMAP, 0);
  if (note_desc && !note_desc->empty ())
    {
      note_data.reset (elfcore_write_note (obfd, note_data.release (),
					   note_size, "FreeBSD",
					   NT_FREEBSD_PROCSTAT_VMMAP,
					   note_desc->data (),
					   note_desc->size ()));
      if (!note_data)
	return NULL;
    }

  note_desc = fbsd_make_note_desc (TARGET_OBJECT_FREEBSD_PS_STRINGS, 0);
  if (note_desc && !note_desc->empty ())
    {
      note_data.reset (elfcore_write_note (obfd, note_data.release (),
					   note_size, "FreeBSD",
					   NT_FREEBSD_PROCSTAT_PSSTRINGS,
					   note_desc->data (),
					   note_desc->size ()));
      if (!note_data)
	return NULL;
    }

  /* Include the target description when possible.  Some architectures
     allow for per-thread gdbarch so we should really be emitting a tdesc
     per-thread, however, we don't currently support reading in a
     per-thread tdesc, so just emit the tdesc for the signalled thread.  */
  gdbarch = target_thread_architecture (signalled_thr->ptid);
  gcore_elf_make_tdesc_note (gdbarch, obfd, &note_data, note_size);

  return note_data;
}

/* Helper function to generate the file descriptor description for a
   single open file in 'info proc files'.  */

static const char *
fbsd_file_fd (int kf_fd)
{
  switch (kf_fd)
    {
    case KINFO_FILE_FD_TYPE_CWD:
      return "cwd";
    case KINFO_FILE_FD_TYPE_ROOT:
      return "root";
    case KINFO_FILE_FD_TYPE_JAIL:
      return "jail";
    case KINFO_FILE_FD_TYPE_TRACE:
      return "trace";
    case KINFO_FILE_FD_TYPE_TEXT:
      return "text";
    case KINFO_FILE_FD_TYPE_CTTY:
      return "ctty";
    default:
      return int_string (kf_fd, 10, 1, 0, 0);
    }
}

/* Helper function to generate the file type for a single open file in
   'info proc files'.  */

static const char *
fbsd_file_type (int kf_type, int kf_vnode_type)
{
  switch (kf_type)
    {
    case KINFO_FILE_TYPE_VNODE:
      switch (kf_vnode_type)
	{
	case KINFO_FILE_VTYPE_VREG:
	  return "file";
	case KINFO_FILE_VTYPE_VDIR:
	  return "dir";
	case KINFO_FILE_VTYPE_VCHR:
	  return "chr";
	case KINFO_FILE_VTYPE_VLNK:
	  return "link";
	case KINFO_FILE_VTYPE_VSOCK:
	  return "socket";
	case KINFO_FILE_VTYPE_VFIFO:
	  return "fifo";
	default:
	  {
	    char *str = get_print_cell ();

	    xsnprintf (str, PRINT_CELL_SIZE, "vn:%d", kf_vnode_type);
	    return str;
	  }
	}
    case KINFO_FILE_TYPE_SOCKET:
      return "socket";
    case KINFO_FILE_TYPE_PIPE:
      return "pipe";
    case KINFO_FILE_TYPE_FIFO:
      return "fifo";
    case KINFO_FILE_TYPE_KQUEUE:
      return "kqueue";
    case KINFO_FILE_TYPE_CRYPTO:
      return "crypto";
    case KINFO_FILE_TYPE_MQUEUE:
      return "mqueue";
    case KINFO_FILE_TYPE_SHM:
      return "shm";
    case KINFO_FILE_TYPE_SEM:
      return "sem";
    case KINFO_FILE_TYPE_PTS:
      return "pts";
    case KINFO_FILE_TYPE_PROCDESC:
      return "proc";
    default:
      return int_string (kf_type, 10, 1, 0, 0);
    }
}

/* Helper function to generate the file flags for a single open file in
   'info proc files'.  */

static const char *
fbsd_file_flags (int kf_flags)
{
  static char file_flags[10];

  file_flags[0] = (kf_flags & KINFO_FILE_FLAG_READ) ? 'r' : '-';
  file_flags[1] = (kf_flags & KINFO_FILE_FLAG_WRITE) ? 'w' : '-';
  file_flags[2] = (kf_flags & KINFO_FILE_FLAG_EXEC) ? 'x' : '-';
  file_flags[3] = (kf_flags & KINFO_FILE_FLAG_APPEND) ? 'a' : '-';
  file_flags[4] = (kf_flags & KINFO_FILE_FLAG_ASYNC) ? 's' : '-';
  file_flags[5] = (kf_flags & KINFO_FILE_FLAG_FSYNC) ? 'f' : '-';
  file_flags[6] = (kf_flags & KINFO_FILE_FLAG_NONBLOCK) ? 'n' : '-';
  file_flags[7] = (kf_flags & KINFO_FILE_FLAG_DIRECT) ? 'd' : '-';
  file_flags[8] = (kf_flags & KINFO_FILE_FLAG_HASLOCK) ? 'l' : '-';
  file_flags[9] = '\0';

  return file_flags;
}

/* Helper function to generate the name of an IP protocol.  */

static const char *
fbsd_ipproto (int protocol)
{
  switch (protocol)
    {
    case FBSD_IPPROTO_ICMP:
      return "icmp";
    case FBSD_IPPROTO_TCP:
      return "tcp";
    case FBSD_IPPROTO_UDP:
      return "udp";
    case FBSD_IPPROTO_SCTP:
      return "sctp";
    default:
      {
	char *str = get_print_cell ();

	xsnprintf (str, PRINT_CELL_SIZE, "ip<%d>", protocol);
	return str;
      }
    }
}

/* Helper function to print out an IPv4 socket address.  */

static void
fbsd_print_sockaddr_in (const void *sockaddr)
{
  const struct fbsd_sockaddr_in *sin =
    reinterpret_cast<const struct fbsd_sockaddr_in *> (sockaddr);
  char buf[INET_ADDRSTRLEN];

  if (inet_ntop (AF_INET, sin->sin_addr, buf, sizeof buf) == nullptr)
    error (_("Failed to format IPv4 address"));
  gdb_printf ("%s:%u", buf,
	      (sin->sin_port[0] << 8) | sin->sin_port[1]);
}

/* Helper function to print out an IPv6 socket address.  */

static void
fbsd_print_sockaddr_in6 (const void *sockaddr)
{
  const struct fbsd_sockaddr_in6 *sin6 =
    reinterpret_cast<const struct fbsd_sockaddr_in6 *> (sockaddr);
  char buf[INET6_ADDRSTRLEN];

  if (inet_ntop (AF_INET6, sin6->sin6_addr, buf, sizeof buf) == nullptr)
    error (_("Failed to format IPv6 address"));
  gdb_printf ("%s.%u", buf,
	      (sin6->sin6_port[0] << 8) | sin6->sin6_port[1]);
}

/* See fbsd-tdep.h.  */

void
fbsd_info_proc_files_header ()
{
  gdb_printf (_("Open files:\n\n"));
  gdb_printf ("  %6s %6s %10s %9s %s\n",
	      "FD", "Type", "Offset", "Flags  ", "Name");
}

/* See fbsd-tdep.h.  */

void
fbsd_info_proc_files_entry (int kf_type, int kf_fd, int kf_flags,
			    LONGEST kf_offset, int kf_vnode_type,
			    int kf_sock_domain, int kf_sock_type,
			    int kf_sock_protocol, const void *kf_sa_local,
			    const void *kf_sa_peer, const void *kf_path)
{
  gdb_printf ("  %6s %6s %10s %8s ",
	      fbsd_file_fd (kf_fd),
	      fbsd_file_type (kf_type, kf_vnode_type),
	      kf_offset > -1 ? hex_string (kf_offset) : "-",
	      fbsd_file_flags (kf_flags));
  if (kf_type == KINFO_FILE_TYPE_SOCKET)
    {
      switch (kf_sock_domain)
	{
	case FBSD_AF_UNIX:
	  {
	    switch (kf_sock_type)
	      {
	      case FBSD_SOCK_STREAM:
		gdb_printf ("unix stream:");
		break;
	      case FBSD_SOCK_DGRAM:
		gdb_printf ("unix dgram:");
		break;
	      case FBSD_SOCK_SEQPACKET:
		gdb_printf ("unix seqpacket:");
		break;
	      default:
		gdb_printf ("unix <%d>:", kf_sock_type);
		break;
	      }

	    /* For local sockets, print out the first non-nul path
	       rather than both paths.  */
	    const struct fbsd_sockaddr_un *saddr_un
	      = reinterpret_cast<const struct fbsd_sockaddr_un *> (kf_sa_local);
	    if (saddr_un->sun_path[0] == 0)
	      saddr_un = reinterpret_cast<const struct fbsd_sockaddr_un *>
		(kf_sa_peer);
	    gdb_printf ("%s", saddr_un->sun_path);
	    break;
	  }
	case FBSD_AF_INET:
	  gdb_printf ("%s4 ", fbsd_ipproto (kf_sock_protocol));
	  fbsd_print_sockaddr_in (kf_sa_local);
	  gdb_printf (" -> ");
	  fbsd_print_sockaddr_in (kf_sa_peer);
	  break;
	case FBSD_AF_INET6:
	  gdb_printf ("%s6 ", fbsd_ipproto (kf_sock_protocol));
	  fbsd_print_sockaddr_in6 (kf_sa_local);
	  gdb_printf (" -> ");
	  fbsd_print_sockaddr_in6 (kf_sa_peer);
	  break;
	}
    }
  else
    gdb_printf ("%s", reinterpret_cast<const char *> (kf_path));
  gdb_printf ("\n");
}

/* Implement "info proc files" for a corefile.  */

static void
fbsd_core_info_proc_files (struct gdbarch *gdbarch)
{
  asection *section
    = bfd_get_section_by_name (core_bfd, ".note.freebsdcore.files");
  if (section == NULL)
    {
      warning (_("unable to find open files in core file"));
      return;
    }

  size_t note_size = bfd_section_size (section);
  if (note_size < 4)
    error (_("malformed core note - too short for header"));

  gdb::def_vector<unsigned char> contents (note_size);
  if (!bfd_get_section_contents (core_bfd, section, contents.data (),
				 0, note_size))
    error (_("could not get core note contents"));

  unsigned char *descdata = contents.data ();
  unsigned char *descend = descdata + note_size;

  /* Skip over the structure size.  */
  descdata += 4;

  fbsd_info_proc_files_header ();

  while (descdata + KF_PATH < descend)
    {
      ULONGEST structsize = bfd_get_32 (core_bfd, descdata + KF_STRUCTSIZE);
      if (structsize < KF_PATH)
	error (_("malformed core note - file structure too small"));

      LONGEST type = bfd_get_signed_32 (core_bfd, descdata + KF_TYPE);
      LONGEST fd = bfd_get_signed_32 (core_bfd, descdata + KF_FD);
      LONGEST flags = bfd_get_signed_32 (core_bfd, descdata + KF_FLAGS);
      LONGEST offset = bfd_get_signed_64 (core_bfd, descdata + KF_OFFSET);
      LONGEST vnode_type = bfd_get_signed_32 (core_bfd,
					      descdata + KF_VNODE_TYPE);
      LONGEST sock_domain = bfd_get_signed_32 (core_bfd,
					       descdata + KF_SOCK_DOMAIN);
      LONGEST sock_type = bfd_get_signed_32 (core_bfd, descdata + KF_SOCK_TYPE);
      LONGEST sock_protocol = bfd_get_signed_32 (core_bfd,
						 descdata + KF_SOCK_PROTOCOL);
      fbsd_info_proc_files_entry (type, fd, flags, offset, vnode_type,
				  sock_domain, sock_type, sock_protocol,
				  descdata + KF_SA_LOCAL, descdata + KF_SA_PEER,
				  descdata + KF_PATH);

      descdata += structsize;
    }
}

/* Helper function to generate mappings flags for a single VM map
   entry in 'info proc mappings'.  */

static const char *
fbsd_vm_map_entry_flags (int kve_flags, int kve_protection)
{
  static char vm_flags[9];

  vm_flags[0] = (kve_protection & KINFO_VME_PROT_READ) ? 'r' : '-';
  vm_flags[1] = (kve_protection & KINFO_VME_PROT_WRITE) ? 'w' : '-';
  vm_flags[2] = (kve_protection & KINFO_VME_PROT_EXEC) ? 'x' : '-';
  vm_flags[3] = ' ';
  vm_flags[4] = (kve_flags & KINFO_VME_FLAG_COW) ? 'C' : '-';
  vm_flags[5] = (kve_flags & KINFO_VME_FLAG_NEEDS_COPY) ? 'N' : '-';
  vm_flags[6] = (kve_flags & KINFO_VME_FLAG_SUPER) ? 'S' : '-';
  vm_flags[7] = (kve_flags & KINFO_VME_FLAG_GROWS_UP) ? 'U'
    : (kve_flags & KINFO_VME_FLAG_GROWS_DOWN) ? 'D' : '-';
  vm_flags[8] = '\0';

  return vm_flags;
}

/* See fbsd-tdep.h.  */

void
fbsd_info_proc_mappings_header (int addr_bit)
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

/* See fbsd-tdep.h.  */

void
fbsd_info_proc_mappings_entry (int addr_bit, ULONGEST kve_start,
			       ULONGEST kve_end, ULONGEST kve_offset,
			       int kve_flags, int kve_protection,
			       const void *kve_path)
{
  if (addr_bit == 64)
    {
      gdb_printf ("  %18s %18s %10s %10s %9s %s\n",
		  hex_string (kve_start),
		  hex_string (kve_end),
		  hex_string (kve_end - kve_start),
		  hex_string (kve_offset),
		  fbsd_vm_map_entry_flags (kve_flags, kve_protection),
		  reinterpret_cast<const char *> (kve_path));
    }
  else
    {
      gdb_printf ("\t%10s %10s %10s %10s %9s %s\n",
		  hex_string (kve_start),
		  hex_string (kve_end),
		  hex_string (kve_end - kve_start),
		  hex_string (kve_offset),
		  fbsd_vm_map_entry_flags (kve_flags, kve_protection),
		  reinterpret_cast<const char *> (kve_path));
    }
}

/* Implement "info proc mappings" for a corefile.  */

static void
fbsd_core_info_proc_mappings (struct gdbarch *gdbarch)
{
  asection *section;
  unsigned char *descdata, *descend;
  size_t note_size;

  section = bfd_get_section_by_name (core_bfd, ".note.freebsdcore.vmmap");
  if (section == NULL)
    {
      warning (_("unable to find mappings in core file"));
      return;
    }

  note_size = bfd_section_size (section);
  if (note_size < 4)
    error (_("malformed core note - too short for header"));

  gdb::def_vector<unsigned char> contents (note_size);
  if (!bfd_get_section_contents (core_bfd, section, contents.data (),
				 0, note_size))
    error (_("could not get core note contents"));

  descdata = contents.data ();
  descend = descdata + note_size;

  /* Skip over the structure size.  */
  descdata += 4;

  fbsd_info_proc_mappings_header (gdbarch_addr_bit (gdbarch));
  while (descdata + KVE_PATH < descend)
    {
      ULONGEST structsize = bfd_get_32 (core_bfd, descdata + KVE_STRUCTSIZE);
      if (structsize < KVE_PATH)
	error (_("malformed core note - vmmap entry too small"));

      ULONGEST start = bfd_get_64 (core_bfd, descdata + KVE_START);
      ULONGEST end = bfd_get_64 (core_bfd, descdata + KVE_END);
      ULONGEST offset = bfd_get_64 (core_bfd, descdata + KVE_OFFSET);
      LONGEST flags = bfd_get_signed_32 (core_bfd, descdata + KVE_FLAGS);
      LONGEST prot = bfd_get_signed_32 (core_bfd, descdata + KVE_PROTECTION);
      fbsd_info_proc_mappings_entry (gdbarch_addr_bit (gdbarch), start, end,
				     offset, flags, prot, descdata + KVE_PATH);

      descdata += structsize;
    }
}

/* Fetch the pathname of a vnode for a single file descriptor from the
   file table core note.  */

static gdb::unique_xmalloc_ptr<char>
fbsd_core_vnode_path (struct gdbarch *gdbarch, int fd)
{
  asection *section;
  unsigned char *descdata, *descend;
  size_t note_size;

  section = bfd_get_section_by_name (core_bfd, ".note.freebsdcore.files");
  if (section == NULL)
    return nullptr;

  note_size = bfd_section_size (section);
  if (note_size < 4)
    error (_("malformed core note - too short for header"));

  gdb::def_vector<unsigned char> contents (note_size);
  if (!bfd_get_section_contents (core_bfd, section, contents.data (),
				 0, note_size))
    error (_("could not get core note contents"));

  descdata = contents.data ();
  descend = descdata + note_size;

  /* Skip over the structure size.  */
  descdata += 4;

  while (descdata + KF_PATH < descend)
    {
      ULONGEST structsize;

      structsize = bfd_get_32 (core_bfd, descdata + KF_STRUCTSIZE);
      if (structsize < KF_PATH)
	error (_("malformed core note - file structure too small"));

      if (bfd_get_32 (core_bfd, descdata + KF_TYPE) == KINFO_FILE_TYPE_VNODE
	  && bfd_get_signed_32 (core_bfd, descdata + KF_FD) == fd)
	{
	  char *path = (char *) descdata + KF_PATH;
	  return make_unique_xstrdup (path);
	}

      descdata += structsize;
    }
  return nullptr;
}

/* Helper function to read a struct timeval.  */

static void
fbsd_core_fetch_timeval (struct gdbarch *gdbarch, unsigned char *data,
			 LONGEST &sec, ULONGEST &usec)
{
  if (gdbarch_addr_bit (gdbarch) == 64)
    {
      sec = bfd_get_signed_64 (core_bfd, data);
      usec = bfd_get_64 (core_bfd, data + 8);
    }
  else if (bfd_get_arch (core_bfd) == bfd_arch_i386)
    {
      sec = bfd_get_signed_32 (core_bfd, data);
      usec = bfd_get_32 (core_bfd, data + 4);
    }
  else
    {
      sec = bfd_get_signed_64 (core_bfd, data);
      usec = bfd_get_32 (core_bfd, data + 8);
    }
}

/* Print out the contents of a signal set.  */

static void
fbsd_print_sigset (const char *descr, unsigned char *sigset)
{
  gdb_printf ("%s: ", descr);
  for (int i = 0; i < SIG_WORDS; i++)
    gdb_printf ("%08x ",
		(unsigned int) bfd_get_32 (core_bfd, sigset + i * 4));
  gdb_printf ("\n");
}

/* Implement "info proc status" for a corefile.  */

static void
fbsd_core_info_proc_status (struct gdbarch *gdbarch)
{
  const struct kinfo_proc_layout *kp;
  asection *section;
  unsigned char *descdata;
  int addr_bit, long_bit;
  size_t note_size;
  ULONGEST value;
  LONGEST sec;

  section = bfd_get_section_by_name (core_bfd, ".note.freebsdcore.proc");
  if (section == NULL)
    {
      warning (_("unable to find process info in core file"));
      return;
    }

  addr_bit = gdbarch_addr_bit (gdbarch);
  if (addr_bit == 64)
    kp = &kinfo_proc_layout_64;
  else if (bfd_get_arch (core_bfd) == bfd_arch_i386)
    kp = &kinfo_proc_layout_i386;
  else
    kp = &kinfo_proc_layout_32;
  long_bit = gdbarch_long_bit (gdbarch);

  /*
   * Ensure that the note is large enough for all of the fields fetched
   * by this function.  In particular, the note must contain the 32-bit
   * structure size, then it must be long enough to access the last
   * field used (ki_rusage_ch.ru_majflt) which is the size of a long.
   */
  note_size = bfd_section_size (section);
  if (note_size < (4 + kp->ki_rusage_ch + kp->ru_majflt
		   + long_bit / TARGET_CHAR_BIT))
    error (_("malformed core note - too short"));

  gdb::def_vector<unsigned char> contents (note_size);
  if (!bfd_get_section_contents (core_bfd, section, contents.data (),
				 0, note_size))
    error (_("could not get core note contents"));

  descdata = contents.data ();

  /* Skip over the structure size.  */
  descdata += 4;

  /* Verify 'ki_layout' is 0.  */
  if (bfd_get_32 (core_bfd, descdata + kp->ki_layout) != 0)
    {
      warning (_("unsupported process information in core file"));
      return;
    }

  gdb_printf ("Name: %.19s\n", descdata + kp->ki_comm);
  gdb_printf ("Process ID: %s\n",
	      pulongest (bfd_get_32 (core_bfd, descdata + kp->ki_pid)));
  gdb_printf ("Parent process: %s\n",
	      pulongest (bfd_get_32 (core_bfd, descdata + kp->ki_ppid)));
  gdb_printf ("Process group: %s\n",
	      pulongest (bfd_get_32 (core_bfd, descdata + kp->ki_pgid)));
  gdb_printf ("Session id: %s\n",
	      pulongest (bfd_get_32 (core_bfd, descdata + kp->ki_sid)));

  /* FreeBSD 12.0 and later store a 64-bit dev_t at 'ki_tdev'.  Older
     kernels store a 32-bit dev_t at 'ki_tdev_freebsd11'.  In older
     kernels the 64-bit 'ki_tdev' field is in a reserved section of
     the structure that is cleared to zero.  Assume that a zero value
     in ki_tdev indicates a core dump from an older kernel and use the
     value in 'ki_tdev_freebsd11' instead.  */
  value = bfd_get_64 (core_bfd, descdata + kp->ki_tdev);
  if (value == 0)
    value = bfd_get_32 (core_bfd, descdata + kp->ki_tdev_freebsd11);
  gdb_printf ("TTY: %s\n", pulongest (value));
  gdb_printf ("TTY owner process group: %s\n",
	      pulongest (bfd_get_32 (core_bfd, descdata + kp->ki_tpgid)));
  gdb_printf ("User IDs (real, effective, saved): %s %s %s\n",
	      pulongest (bfd_get_32 (core_bfd, descdata + kp->ki_ruid)),
	      pulongest (bfd_get_32 (core_bfd, descdata + kp->ki_uid)),
	      pulongest (bfd_get_32 (core_bfd, descdata + kp->ki_svuid)));
  gdb_printf ("Group IDs (real, effective, saved): %s %s %s\n",
	      pulongest (bfd_get_32 (core_bfd, descdata + kp->ki_rgid)),
	      pulongest (bfd_get_32 (core_bfd, descdata + kp->ki_groups)),
	      pulongest (bfd_get_32 (core_bfd, descdata + kp->ki_svgid)));
  gdb_printf ("Groups: ");
  uint16_t ngroups = bfd_get_16 (core_bfd, descdata + kp->ki_ngroups);
  for (int i = 0; i < ngroups; i++)
    gdb_printf ("%s ",
		pulongest (bfd_get_32 (core_bfd,
				       descdata + kp->ki_groups + i * 4)));
  gdb_printf ("\n");
  value = bfd_get (long_bit, core_bfd,
		   descdata + kp->ki_rusage + kp->ru_minflt);
  gdb_printf ("Minor faults (no memory page): %s\n", pulongest (value));
  value = bfd_get (long_bit, core_bfd,
		   descdata + kp->ki_rusage_ch + kp->ru_minflt);
  gdb_printf ("Minor faults, children: %s\n", pulongest (value));
  value = bfd_get (long_bit, core_bfd,
		   descdata + kp->ki_rusage + kp->ru_majflt);
  gdb_printf ("Major faults (memory page faults): %s\n",
	      pulongest (value));
  value = bfd_get (long_bit, core_bfd,
		   descdata + kp->ki_rusage_ch + kp->ru_majflt);
  gdb_printf ("Major faults, children: %s\n", pulongest (value));
  fbsd_core_fetch_timeval (gdbarch,
			   descdata + kp->ki_rusage + kp->ru_utime,
			   sec, value);
  gdb_printf ("utime: %s.%06d\n", plongest (sec), (int) value);
  fbsd_core_fetch_timeval (gdbarch,
			   descdata + kp->ki_rusage + kp->ru_stime,
			   sec, value);
  gdb_printf ("stime: %s.%06d\n", plongest (sec), (int) value);
  fbsd_core_fetch_timeval (gdbarch,
			   descdata + kp->ki_rusage_ch + kp->ru_utime,
			   sec, value);
  gdb_printf ("utime, children: %s.%06d\n", plongest (sec), (int) value);
  fbsd_core_fetch_timeval (gdbarch,
			   descdata + kp->ki_rusage_ch + kp->ru_stime,
			   sec, value);
  gdb_printf ("stime, children: %s.%06d\n", plongest (sec), (int) value);
  gdb_printf ("'nice' value: %d\n",
	      (int) bfd_get_signed_8 (core_bfd, descdata + kp->ki_nice));
  fbsd_core_fetch_timeval (gdbarch, descdata + kp->ki_start, sec, value);
  gdb_printf ("Start time: %s.%06d\n", plongest (sec), (int) value);
  gdb_printf ("Virtual memory size: %s kB\n",
	      pulongest (bfd_get (addr_bit, core_bfd,
				  descdata + kp->ki_size) / 1024));
  gdb_printf ("Data size: %s pages\n",
	      pulongest (bfd_get (addr_bit, core_bfd,
				  descdata + kp->ki_dsize)));
  gdb_printf ("Stack size: %s pages\n",
	      pulongest (bfd_get (addr_bit, core_bfd,
				  descdata + kp->ki_ssize)));
  gdb_printf ("Text size: %s pages\n",
	      pulongest (bfd_get (addr_bit, core_bfd,
				  descdata + kp->ki_tsize)));
  gdb_printf ("Resident set size: %s pages\n",
	      pulongest (bfd_get (addr_bit, core_bfd,
				  descdata + kp->ki_rssize)));
  gdb_printf ("Maximum RSS: %s pages\n",
	      pulongest (bfd_get (long_bit, core_bfd,
				  descdata + kp->ki_rusage
				  + kp->ru_maxrss)));
  fbsd_print_sigset ("Ignored Signals", descdata + kp->ki_sigignore);
  fbsd_print_sigset ("Caught Signals", descdata + kp->ki_sigcatch);
}

/* Implement the "core_info_proc" gdbarch method.  */

static void
fbsd_core_info_proc (struct gdbarch *gdbarch, const char *args,
		     enum info_proc_what what)
{
  bool do_cmdline = false;
  bool do_cwd = false;
  bool do_exe = false;
  bool do_files = false;
  bool do_mappings = false;
  bool do_status = false;
  int pid;

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
      return;
    }

  pid = bfd_core_file_pid (core_bfd);
  if (pid != 0)
    gdb_printf (_("process %d\n"), pid);

  if (do_cmdline)
    {
      const char *cmdline;

      cmdline = bfd_core_file_failing_command (core_bfd);
      if (cmdline)
	gdb_printf ("cmdline = '%s'\n", cmdline);
      else
	warning (_("Command line unavailable"));
    }
  if (do_cwd)
    {
      gdb::unique_xmalloc_ptr<char> cwd =
	fbsd_core_vnode_path (gdbarch, KINFO_FILE_FD_TYPE_CWD);
      if (cwd)
	gdb_printf ("cwd = '%s'\n", cwd.get ());
      else
	warning (_("unable to read current working directory"));
    }
  if (do_exe)
    {
      gdb::unique_xmalloc_ptr<char> exe =
	fbsd_core_vnode_path (gdbarch, KINFO_FILE_FD_TYPE_TEXT);
      if (exe)
	gdb_printf ("exe = '%s'\n", exe.get ());
      else
	warning (_("unable to read executable path name"));
    }
  if (do_files)
    fbsd_core_info_proc_files (gdbarch);
  if (do_mappings)
    fbsd_core_info_proc_mappings (gdbarch);
  if (do_status)
    fbsd_core_info_proc_status (gdbarch);
}

/* Print descriptions of FreeBSD-specific AUXV entries to FILE.  */

static void
fbsd_print_auxv_entry (struct gdbarch *gdbarch, struct ui_file *file,
		       CORE_ADDR type, CORE_ADDR val)
{
  const char *name = "???";
  const char *description = "";
  enum auxv_format format = AUXV_FORMAT_HEX;

  switch (type)
    {
    case AT_NULL:
    case AT_IGNORE:
    case AT_EXECFD:
    case AT_PHDR:
    case AT_PHENT:
    case AT_PHNUM:
    case AT_PAGESZ:
    case AT_BASE:
    case AT_FLAGS:
    case AT_ENTRY:
    case AT_NOTELF:
    case AT_UID:
    case AT_EUID:
    case AT_GID:
    case AT_EGID:
      default_print_auxv_entry (gdbarch, file, type, val);
      return;
#define _TAGNAME(tag) #tag
#define TAGNAME(tag) _TAGNAME(AT_##tag)
#define TAG(tag, text, kind) \
      case AT_FREEBSD_##tag: name = TAGNAME(tag); description = text; format = kind; break
      TAG (EXECPATH, _("Executable path"), AUXV_FORMAT_STR);
      TAG (CANARY, _("Canary for SSP"), AUXV_FORMAT_HEX);
      TAG (CANARYLEN, ("Length of the SSP canary"), AUXV_FORMAT_DEC);
      TAG (OSRELDATE, _("OSRELDATE"), AUXV_FORMAT_DEC);
      TAG (NCPUS, _("Number of CPUs"), AUXV_FORMAT_DEC);
      TAG (PAGESIZES, _("Pagesizes"), AUXV_FORMAT_HEX);
      TAG (PAGESIZESLEN, _("Number of pagesizes"), AUXV_FORMAT_DEC);
      TAG (TIMEKEEP, _("Pointer to timehands"), AUXV_FORMAT_HEX);
      TAG (STACKPROT, _("Initial stack protection"), AUXV_FORMAT_HEX);
      TAG (EHDRFLAGS, _("ELF header e_flags"), AUXV_FORMAT_HEX);
      TAG (HWCAP, _("Machine-dependent CPU capability hints"), AUXV_FORMAT_HEX);
      TAG (HWCAP2, _("Extension of AT_HWCAP"), AUXV_FORMAT_HEX);
      TAG (BSDFLAGS, _("ELF BSD flags"), AUXV_FORMAT_HEX);
      TAG (ARGC, _("Argument count"), AUXV_FORMAT_DEC);
      TAG (ARGV, _("Argument vector"), AUXV_FORMAT_HEX);
      TAG (ENVC, _("Environment count"), AUXV_FORMAT_DEC);
      TAG (ENVV, _("Environment vector"), AUXV_FORMAT_HEX);
      TAG (PS_STRINGS, _("Pointer to ps_strings"), AUXV_FORMAT_HEX);
      TAG (FXRNG, _("Pointer to root RNG seed version"), AUXV_FORMAT_HEX);
      TAG (KPRELOAD, _("Base address of vDSO"), AUXV_FORMAT_HEX);
      TAG (USRSTACKBASE, _("Top of user stack"), AUXV_FORMAT_HEX);
      TAG (USRSTACKLIM, _("Grow limit of user stack"), AUXV_FORMAT_HEX);
    }

  fprint_auxv_entry (file, name, description, format, type, val);
}

/* Implement the "get_siginfo_type" gdbarch method.  */

static struct type *
fbsd_get_siginfo_type (struct gdbarch *gdbarch)
{
  struct fbsd_gdbarch_data *fbsd_gdbarch_data;
  struct type *int_type, *int32_type, *uint32_type, *long_type, *void_ptr_type;
  struct type *uid_type, *pid_type;
  struct type *sigval_type, *reason_type;
  struct type *siginfo_type;
  struct type *type;

  fbsd_gdbarch_data = get_fbsd_gdbarch_data (gdbarch);
  if (fbsd_gdbarch_data->siginfo_type != NULL)
    return fbsd_gdbarch_data->siginfo_type;

  type_allocator alloc (gdbarch);
  int_type = init_integer_type (alloc, gdbarch_int_bit (gdbarch),
				0, "int");
  int32_type = init_integer_type (alloc, 32, 0, "int32_t");
  uint32_type = init_integer_type (alloc, 32, 1, "uint32_t");
  long_type = init_integer_type (alloc, gdbarch_long_bit (gdbarch),
				 0, "long");
  void_ptr_type = lookup_pointer_type (builtin_type (gdbarch)->builtin_void);

  /* union sigval */
  sigval_type = arch_composite_type (gdbarch, NULL, TYPE_CODE_UNION);
  sigval_type->set_name (xstrdup ("sigval"));
  append_composite_type_field (sigval_type, "sival_int", int_type);
  append_composite_type_field (sigval_type, "sival_ptr", void_ptr_type);

  /* __pid_t */
  pid_type = alloc.new_type (TYPE_CODE_TYPEDEF,
			     int32_type->length () * TARGET_CHAR_BIT,
			     "__pid_t");
  pid_type->set_target_type (int32_type);
  pid_type->set_target_is_stub (true);

  /* __uid_t */
  uid_type = alloc.new_type (TYPE_CODE_TYPEDEF,
			     uint32_type->length () * TARGET_CHAR_BIT,
			     "__uid_t");
  uid_type->set_target_type (uint32_type);
  pid_type->set_target_is_stub (true);

  /* _reason */
  reason_type = arch_composite_type (gdbarch, NULL, TYPE_CODE_UNION);

  /* _fault */
  type = arch_composite_type (gdbarch, NULL, TYPE_CODE_STRUCT);
  append_composite_type_field (type, "si_trapno", int_type);
  append_composite_type_field (reason_type, "_fault", type);

  /* _timer */
  type = arch_composite_type (gdbarch, NULL, TYPE_CODE_STRUCT);
  append_composite_type_field (type, "si_timerid", int_type);
  append_composite_type_field (type, "si_overrun", int_type);
  append_composite_type_field (reason_type, "_timer", type);

  /* _mesgq */
  type = arch_composite_type (gdbarch, NULL, TYPE_CODE_STRUCT);
  append_composite_type_field (type, "si_mqd", int_type);
  append_composite_type_field (reason_type, "_mesgq", type);

  /* _poll */
  type = arch_composite_type (gdbarch, NULL, TYPE_CODE_STRUCT);
  append_composite_type_field (type, "si_band", long_type);
  append_composite_type_field (reason_type, "_poll", type);

  /* __spare__ */
  type = arch_composite_type (gdbarch, NULL, TYPE_CODE_STRUCT);
  append_composite_type_field (type, "__spare1__", long_type);
  append_composite_type_field (type, "__spare2__",
			       init_vector_type (int_type, 7));
  append_composite_type_field (reason_type, "__spare__", type);

  /* struct siginfo */
  siginfo_type = arch_composite_type (gdbarch, NULL, TYPE_CODE_STRUCT);
  siginfo_type->set_name (xstrdup ("siginfo"));
  append_composite_type_field (siginfo_type, "si_signo", int_type);
  append_composite_type_field (siginfo_type, "si_errno", int_type);
  append_composite_type_field (siginfo_type, "si_code", int_type);
  append_composite_type_field (siginfo_type, "si_pid", pid_type);
  append_composite_type_field (siginfo_type, "si_uid", uid_type);
  append_composite_type_field (siginfo_type, "si_status", int_type);
  append_composite_type_field (siginfo_type, "si_addr", void_ptr_type);
  append_composite_type_field (siginfo_type, "si_value", sigval_type);
  append_composite_type_field (siginfo_type, "_reason", reason_type);

  fbsd_gdbarch_data->siginfo_type = siginfo_type;

  return siginfo_type;
}

/* Implement the "gdb_signal_from_target" gdbarch method.  */

static enum gdb_signal
fbsd_gdb_signal_from_target (struct gdbarch *gdbarch, int signal)
{
  switch (signal)
    {
    case 0:
      return GDB_SIGNAL_0;

    case FREEBSD_SIGHUP:
      return GDB_SIGNAL_HUP;

    case FREEBSD_SIGINT:
      return GDB_SIGNAL_INT;

    case FREEBSD_SIGQUIT:
      return GDB_SIGNAL_QUIT;

    case FREEBSD_SIGILL:
      return GDB_SIGNAL_ILL;

    case FREEBSD_SIGTRAP:
      return GDB_SIGNAL_TRAP;

    case FREEBSD_SIGABRT:
      return GDB_SIGNAL_ABRT;

    case FREEBSD_SIGEMT:
      return GDB_SIGNAL_EMT;

    case FREEBSD_SIGFPE:
      return GDB_SIGNAL_FPE;

    case FREEBSD_SIGKILL:
      return GDB_SIGNAL_KILL;

    case FREEBSD_SIGBUS:
      return GDB_SIGNAL_BUS;

    case FREEBSD_SIGSEGV:
      return GDB_SIGNAL_SEGV;

    case FREEBSD_SIGSYS:
      return GDB_SIGNAL_SYS;

    case FREEBSD_SIGPIPE:
      return GDB_SIGNAL_PIPE;

    case FREEBSD_SIGALRM:
      return GDB_SIGNAL_ALRM;

    case FREEBSD_SIGTERM:
      return GDB_SIGNAL_TERM;

    case FREEBSD_SIGURG:
      return GDB_SIGNAL_URG;

    case FREEBSD_SIGSTOP:
      return GDB_SIGNAL_STOP;

    case FREEBSD_SIGTSTP:
      return GDB_SIGNAL_TSTP;

    case FREEBSD_SIGCONT:
      return GDB_SIGNAL_CONT;

    case FREEBSD_SIGCHLD:
      return GDB_SIGNAL_CHLD;

    case FREEBSD_SIGTTIN:
      return GDB_SIGNAL_TTIN;

    case FREEBSD_SIGTTOU:
      return GDB_SIGNAL_TTOU;

    case FREEBSD_SIGIO:
      return GDB_SIGNAL_IO;

    case FREEBSD_SIGXCPU:
      return GDB_SIGNAL_XCPU;

    case FREEBSD_SIGXFSZ:
      return GDB_SIGNAL_XFSZ;

    case FREEBSD_SIGVTALRM:
      return GDB_SIGNAL_VTALRM;

    case FREEBSD_SIGPROF:
      return GDB_SIGNAL_PROF;

    case FREEBSD_SIGWINCH:
      return GDB_SIGNAL_WINCH;

    case FREEBSD_SIGINFO:
      return GDB_SIGNAL_INFO;

    case FREEBSD_SIGUSR1:
      return GDB_SIGNAL_USR1;

    case FREEBSD_SIGUSR2:
      return GDB_SIGNAL_USR2;

    /* SIGTHR is the same as SIGLWP on FreeBSD. */
    case FREEBSD_SIGTHR:
      return GDB_SIGNAL_LWP;

    case FREEBSD_SIGLIBRT:
      return GDB_SIGNAL_LIBRT;
    }

  if (signal >= FREEBSD_SIGRTMIN && signal <= FREEBSD_SIGRTMAX)
    {
      int offset = signal - FREEBSD_SIGRTMIN;

      return (enum gdb_signal) ((int) GDB_SIGNAL_REALTIME_65 + offset);
    }

  return GDB_SIGNAL_UNKNOWN;
}

/* Implement the "gdb_signal_to_target" gdbarch method.  */

static int
fbsd_gdb_signal_to_target (struct gdbarch *gdbarch,
		enum gdb_signal signal)
{
  switch (signal)
    {
    case GDB_SIGNAL_0:
      return 0;

    case GDB_SIGNAL_HUP:
      return FREEBSD_SIGHUP;

    case GDB_SIGNAL_INT:
      return FREEBSD_SIGINT;

    case GDB_SIGNAL_QUIT:
      return FREEBSD_SIGQUIT;

    case GDB_SIGNAL_ILL:
      return FREEBSD_SIGILL;

    case GDB_SIGNAL_TRAP:
      return FREEBSD_SIGTRAP;

    case GDB_SIGNAL_ABRT:
      return FREEBSD_SIGABRT;

    case GDB_SIGNAL_EMT:
      return FREEBSD_SIGEMT;

    case GDB_SIGNAL_FPE:
      return FREEBSD_SIGFPE;

    case GDB_SIGNAL_KILL:
      return FREEBSD_SIGKILL;

    case GDB_SIGNAL_BUS:
      return FREEBSD_SIGBUS;

    case GDB_SIGNAL_SEGV:
      return FREEBSD_SIGSEGV;

    case GDB_SIGNAL_SYS:
      return FREEBSD_SIGSYS;

    case GDB_SIGNAL_PIPE:
      return FREEBSD_SIGPIPE;

    case GDB_SIGNAL_ALRM:
      return FREEBSD_SIGALRM;

    case GDB_SIGNAL_TERM:
      return FREEBSD_SIGTERM;

    case GDB_SIGNAL_URG:
      return FREEBSD_SIGURG;

    case GDB_SIGNAL_STOP:
      return FREEBSD_SIGSTOP;

    case GDB_SIGNAL_TSTP:
      return FREEBSD_SIGTSTP;

    case GDB_SIGNAL_CONT:
      return FREEBSD_SIGCONT;

    case GDB_SIGNAL_CHLD:
      return FREEBSD_SIGCHLD;

    case GDB_SIGNAL_TTIN:
      return FREEBSD_SIGTTIN;

    case GDB_SIGNAL_TTOU:
      return FREEBSD_SIGTTOU;

    case GDB_SIGNAL_IO:
      return FREEBSD_SIGIO;

    case GDB_SIGNAL_XCPU:
      return FREEBSD_SIGXCPU;

    case GDB_SIGNAL_XFSZ:
      return FREEBSD_SIGXFSZ;

    case GDB_SIGNAL_VTALRM:
      return FREEBSD_SIGVTALRM;

    case GDB_SIGNAL_PROF:
      return FREEBSD_SIGPROF;

    case GDB_SIGNAL_WINCH:
      return FREEBSD_SIGWINCH;

    case GDB_SIGNAL_INFO:
      return FREEBSD_SIGINFO;

    case GDB_SIGNAL_USR1:
      return FREEBSD_SIGUSR1;

    case GDB_SIGNAL_USR2:
      return FREEBSD_SIGUSR2;

    case GDB_SIGNAL_LWP:
      return FREEBSD_SIGTHR;

    case GDB_SIGNAL_LIBRT:
      return FREEBSD_SIGLIBRT;
    }

  if (signal >= GDB_SIGNAL_REALTIME_65
      && signal <= GDB_SIGNAL_REALTIME_126)
    {
      int offset = signal - GDB_SIGNAL_REALTIME_65;

      return FREEBSD_SIGRTMIN + offset;
    }

  return -1;
}

/* Implement the "get_syscall_number" gdbarch method.  */

static LONGEST
fbsd_get_syscall_number (struct gdbarch *gdbarch, thread_info *thread)
{

  /* FreeBSD doesn't use gdbarch_get_syscall_number since FreeBSD
     native targets fetch the system call number from the
     'pl_syscall_code' member of struct ptrace_lwpinfo in fbsd_wait.
     However, system call catching requires this function to be
     set.  */

  internal_error (_("fbsd_get_sycall_number called"));
}

/* Read an integer symbol value from the current target.  */

static LONGEST
fbsd_read_integer_by_name (struct gdbarch *gdbarch, const char *name)
{
  bound_minimal_symbol ms = lookup_minimal_symbol (name, NULL, NULL);
  if (ms.minsym == NULL)
    error (_("Unable to resolve symbol '%s'"), name);

  gdb_byte buf[4];
  if (target_read_memory (ms.value_address (), buf, sizeof buf) != 0)
    error (_("Unable to read value of '%s'"), name);

  return extract_signed_integer (buf, gdbarch_byte_order (gdbarch));
}

/* Lookup offsets of fields in the runtime linker's 'Obj_Entry'
   structure needed to determine the TLS index of an object file.  */

static void
fbsd_fetch_rtld_offsets (struct gdbarch *gdbarch, struct fbsd_pspace_data *data)
{
  try
    {
      /* Fetch offsets from debug symbols in rtld.  */
      struct symbol *obj_entry_sym
	= lookup_symbol_in_language ("Struct_Obj_Entry", NULL, STRUCT_DOMAIN,
				     language_c, NULL).symbol;
      if (obj_entry_sym == NULL)
	error (_("Unable to find Struct_Obj_Entry symbol"));
      data->off_linkmap = lookup_struct_elt (obj_entry_sym->type (),
					     "linkmap", 0).offset / 8;
      data->off_tlsindex = lookup_struct_elt (obj_entry_sym->type (),
					      "tlsindex", 0).offset / 8;
      data->rtld_offsets_valid = true;
      return;
    }
  catch (const gdb_exception_error &e)
    {
      data->off_linkmap = -1;
    }

  try
    {
      /* Fetch offsets from global variables in libthr.  Note that
	 this does not work for single-threaded processes that are not
	 linked against libthr.  */
      data->off_linkmap = fbsd_read_integer_by_name (gdbarch,
						     "_thread_off_linkmap");
      data->off_tlsindex = fbsd_read_integer_by_name (gdbarch,
						      "_thread_off_tlsindex");
      data->rtld_offsets_valid = true;
      return;
    }
  catch (const gdb_exception_error &e)
    {
      data->off_linkmap = -1;
    }
}

/* Helper function to read the TLS index of an object file associated
   with a link map entry at LM_ADDR.  */

static LONGEST
fbsd_get_tls_index (struct gdbarch *gdbarch, CORE_ADDR lm_addr)
{
  struct fbsd_pspace_data *data = get_fbsd_pspace_data (current_program_space);

  if (!data->rtld_offsets_valid)
    fbsd_fetch_rtld_offsets (gdbarch, data);

  if (data->off_linkmap == -1)
    throw_error (TLS_GENERIC_ERROR,
		 _("Cannot fetch runtime linker structure offsets"));

  /* Simulate container_of to convert from LM_ADDR to the Obj_Entry
     pointer and then compute the offset of the tlsindex member.  */
  CORE_ADDR tlsindex_addr = lm_addr - data->off_linkmap + data->off_tlsindex;

  gdb_byte buf[4];
  if (target_read_memory (tlsindex_addr, buf, sizeof buf) != 0)
    throw_error (TLS_GENERIC_ERROR,
		 _("Cannot find thread-local variables on this target"));

  return extract_signed_integer (buf, gdbarch_byte_order (gdbarch));
}

/* See fbsd-tdep.h.  */

CORE_ADDR
fbsd_get_thread_local_address (struct gdbarch *gdbarch, CORE_ADDR dtv_addr,
			       CORE_ADDR lm_addr, CORE_ADDR offset)
{
  LONGEST tls_index = fbsd_get_tls_index (gdbarch, lm_addr);

  gdb_byte buf[gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT];
  if (target_read_memory (dtv_addr, buf, sizeof buf) != 0)
    throw_error (TLS_GENERIC_ERROR,
		 _("Cannot find thread-local variables on this target"));

  const struct builtin_type *builtin = builtin_type (gdbarch);
  CORE_ADDR addr = gdbarch_pointer_to_address (gdbarch,
					       builtin->builtin_data_ptr, buf);

  addr += (tls_index + 1) * builtin->builtin_data_ptr->length ();
  if (target_read_memory (addr, buf, sizeof buf) != 0)
    throw_error (TLS_GENERIC_ERROR,
		 _("Cannot find thread-local variables on this target"));

  addr = gdbarch_pointer_to_address (gdbarch, builtin->builtin_data_ptr, buf);
  return addr + offset;
}

/* See fbsd-tdep.h.  */

CORE_ADDR
fbsd_skip_solib_resolver (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  struct bound_minimal_symbol msym = lookup_bound_minimal_symbol ("_rtld_bind");
  if (msym.minsym != nullptr && msym.value_address () == pc)
    return frame_unwind_caller_pc (get_current_frame ());

  return 0;
}

/* Return description of signal code or nullptr.  */

static const char *
fbsd_signal_cause (enum gdb_signal siggnal, int code)
{
  /* Signal-independent causes.  */
  switch (code)
    {
    case FBSD_SI_USER:
      return _("Sent by kill()");
    case FBSD_SI_QUEUE:
      return _("Sent by sigqueue()");
    case FBSD_SI_TIMER:
      return _("Timer expired");
    case FBSD_SI_ASYNCIO:
      return _("Asynchronous I/O request completed");
    case FBSD_SI_MESGQ:
      return _("Message arrived on empty message queue");
    case FBSD_SI_KERNEL:
      return _("Sent by kernel");
    case FBSD_SI_LWP:
      return _("Sent by thr_kill()");
    }

  switch (siggnal)
    {
    case GDB_SIGNAL_ILL:
      switch (code)
	{
	case FBSD_ILL_ILLOPC:
	  return _("Illegal opcode");
	case FBSD_ILL_ILLOPN:
	  return _("Illegal operand");
	case FBSD_ILL_ILLADR:
	  return _("Illegal addressing mode");
	case FBSD_ILL_ILLTRP:
	  return _("Illegal trap");
	case FBSD_ILL_PRVOPC:
	  return _("Privileged opcode");
	case FBSD_ILL_PRVREG:
	  return _("Privileged register");
	case FBSD_ILL_COPROC:
	  return _("Coprocessor error");
	case FBSD_ILL_BADSTK:
	  return _("Internal stack error");
	}
      break;
    case GDB_SIGNAL_BUS:
      switch (code)
	{
	case FBSD_BUS_ADRALN:
	  return _("Invalid address alignment");
	case FBSD_BUS_ADRERR:
	  return _("Address not present");
	case FBSD_BUS_OBJERR:
	  return _("Object-specific hardware error");
	case FBSD_BUS_OOMERR:
	  return _("Out of memory");
	}
      break;
    case GDB_SIGNAL_SEGV:
      switch (code)
	{
	case FBSD_SEGV_MAPERR:
	  return _("Address not mapped to object");
	case FBSD_SEGV_ACCERR:
	  return _("Invalid permissions for mapped object");
	case FBSD_SEGV_PKUERR:
	  return _("PKU violation");
	}
      break;
    case GDB_SIGNAL_FPE:
      switch (code)
	{
	case FBSD_FPE_INTOVF:
	  return _("Integer overflow");
	case FBSD_FPE_INTDIV:
	  return _("Integer divide by zero");
	case FBSD_FPE_FLTDIV:
	  return _("Floating point divide by zero");
	case FBSD_FPE_FLTOVF:
	  return _("Floating point overflow");
	case FBSD_FPE_FLTUND:
	  return _("Floating point underflow");
	case FBSD_FPE_FLTRES:
	  return _("Floating point inexact result");
	case FBSD_FPE_FLTINV:
	  return _("Invalid floating point operation");
	case FBSD_FPE_FLTSUB:
	  return _("Subscript out of range");
	}
      break;
    case GDB_SIGNAL_TRAP:
      switch (code)
	{
	case FBSD_TRAP_BRKPT:
	  return _("Breakpoint");
	case FBSD_TRAP_TRACE:
	  return _("Trace trap");
	case FBSD_TRAP_DTRACE:
	  return _("DTrace-induced trap");
	case FBSD_TRAP_CAP:
	  return _("Capability violation");
	}
      break;
    case GDB_SIGNAL_CHLD:
      switch (code)
	{
	case FBSD_CLD_EXITED:
	  return _("Child has exited");
	case FBSD_CLD_KILLED:
	  return _("Child has terminated abnormally");
	case FBSD_CLD_DUMPED:
	  return _("Child has dumped core");
	case FBSD_CLD_TRAPPED:
	  return _("Traced child has trapped");
	case FBSD_CLD_STOPPED:
	  return _("Child has stopped");
	case FBSD_CLD_CONTINUED:
	  return _("Stopped child has continued");
	}
      break;
    case GDB_SIGNAL_POLL:
      switch (code)
	{
	case FBSD_POLL_IN:
	  return _("Data input available");
	case FBSD_POLL_OUT:
	  return _("Output buffers available");
	case FBSD_POLL_MSG:
	  return _("Input message available");
	case FBSD_POLL_ERR:
	  return _("I/O error");
	case FBSD_POLL_PRI:
	  return _("High priority input available");
	case FBSD_POLL_HUP:
	  return _("Device disconnected");
	}
      break;
    }

  return nullptr;
}

/* Report additional details for a signal stop.  */

static void
fbsd_report_signal_info (struct gdbarch *gdbarch, struct ui_out *uiout,
			 enum gdb_signal siggnal)
{
  LONGEST code, mqd, pid, status, timerid, uid;

  try
    {
      code = parse_and_eval_long ("$_siginfo.si_code");
      pid = parse_and_eval_long ("$_siginfo.si_pid");
      uid = parse_and_eval_long ("$_siginfo.si_uid");
      status = parse_and_eval_long ("$_siginfo.si_status");
      timerid = parse_and_eval_long ("$_siginfo._reason._timer.si_timerid");
      mqd = parse_and_eval_long ("$_siginfo._reason._mesgq.si_mqd");
    }
  catch (const gdb_exception_error &e)
    {
      return;
    }

  const char *meaning = fbsd_signal_cause (siggnal, code);
  if (meaning == nullptr)
    return;

  uiout->text (".\n");
  uiout->field_string ("sigcode-meaning", meaning);

  switch (code)
    {
    case FBSD_SI_USER:
    case FBSD_SI_QUEUE:
    case FBSD_SI_LWP:
      uiout->text (" from pid ");
      uiout->field_string ("sending-pid", plongest (pid));
      uiout->text (" and user ");
      uiout->field_string ("sending-uid", plongest (uid));
      return;
    case FBSD_SI_TIMER:
      uiout->text (": timerid ");
      uiout->field_string ("timerid", plongest (timerid));
      return;
    case FBSD_SI_MESGQ:
      uiout->text (": message queue ");
      uiout->field_string ("message-queue", plongest (mqd));
      return;
    case FBSD_SI_ASYNCIO:
      return;
    }

  if (siggnal == GDB_SIGNAL_CHLD)
    {
      uiout->text (": pid ");
      uiout->field_string ("child-pid", plongest (pid));
      uiout->text (", uid ");
      uiout->field_string ("child-uid", plongest (uid));
      if (code == FBSD_CLD_EXITED)
	{
	  uiout->text (", exit status ");
	  uiout->field_string ("exit-status", plongest (status));
	}
      else
	{
	  uiout->text (", signal ");
	  uiout->field_string ("signal", plongest (status));
	}
    }
}

/* Search a list of struct kinfo_vmmap entries in the ENTRIES buffer
   of LEN bytes to find the length of the entry starting at ADDR.
   Returns the length of the entry or zero if no entry was found.  */

static ULONGEST
fbsd_vmmap_length (struct gdbarch *gdbarch, unsigned char *entries, size_t len,
		   CORE_ADDR addr)
{
      enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
      unsigned char *descdata = entries;
      unsigned char *descend = descdata + len;

      /* Skip over the structure size.  */
      descdata += 4;

      while (descdata + KVE_PATH < descend)
	{
	  ULONGEST structsize = extract_unsigned_integer (descdata
							  + KVE_STRUCTSIZE, 4,
							  byte_order);
	  if (structsize < KVE_PATH)
	    return false;

	  ULONGEST start = extract_unsigned_integer (descdata + KVE_START, 8,
						     byte_order);
	  ULONGEST end = extract_unsigned_integer (descdata + KVE_END, 8,
						   byte_order);
	  if (start == addr)
	    return end - start;

	  descdata += structsize;
	}
      return 0;
}

/* Helper for fbsd_vsyscall_range that does the real work of finding
   the vDSO's address range.  */

static bool
fbsd_vdso_range (struct gdbarch *gdbarch, struct mem_range *range)
{
  if (target_auxv_search (AT_FREEBSD_KPRELOAD, &range->start) <= 0)
    return false;

  if (!target_has_execution ())
    {
      /* Search for the ending address in the NT_PROCSTAT_VMMAP note. */
      asection *section = bfd_get_section_by_name (core_bfd,
						   ".note.freebsdcore.vmmap");
      if (section == nullptr)
	return false;

      size_t note_size = bfd_section_size (section);
      if (note_size < 4)
	return false;

      gdb::def_vector<unsigned char> contents (note_size);
      if (!bfd_get_section_contents (core_bfd, section, contents.data (),
				     0, note_size))
	return false;

      range->length = fbsd_vmmap_length (gdbarch, contents.data (), note_size,
					 range->start);
    }
  else
    {
      /* Fetch the list of address space entries from the running target. */
      std::optional<gdb::byte_vector> buf =
	target_read_alloc (current_inferior ()->top_target (),
			   TARGET_OBJECT_FREEBSD_VMMAP, nullptr);
      if (!buf || buf->empty ())
	return false;

      range->length = fbsd_vmmap_length (gdbarch, buf->data (), buf->size (),
					 range->start);
    }
  return range->length != 0;
}

/* Return the address range of the vDSO for the current inferior.  */

static int
fbsd_vsyscall_range (struct gdbarch *gdbarch, struct mem_range *range)
{
  struct fbsd_pspace_data *data = get_fbsd_pspace_data (current_program_space);

  if (data->vdso_range_p == 0)
    {
      if (fbsd_vdso_range (gdbarch, &data->vdso_range))
	data->vdso_range_p = 1;
      else
	data->vdso_range_p = -1;
    }

  if (data->vdso_range_p < 0)
    return 0;

  *range = data->vdso_range;
  return 1;
}

/* To be called from GDB_OSABI_FREEBSD handlers. */

void
fbsd_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  set_gdbarch_core_pid_to_str (gdbarch, fbsd_core_pid_to_str);
  set_gdbarch_core_thread_name (gdbarch, fbsd_core_thread_name);
  set_gdbarch_core_xfer_siginfo (gdbarch, fbsd_core_xfer_siginfo);
  set_gdbarch_make_corefile_notes (gdbarch, fbsd_make_corefile_notes);
  set_gdbarch_core_info_proc (gdbarch, fbsd_core_info_proc);
  set_gdbarch_print_auxv_entry (gdbarch, fbsd_print_auxv_entry);
  set_gdbarch_get_siginfo_type (gdbarch, fbsd_get_siginfo_type);
  set_gdbarch_gdb_signal_from_target (gdbarch, fbsd_gdb_signal_from_target);
  set_gdbarch_gdb_signal_to_target (gdbarch, fbsd_gdb_signal_to_target);
  set_gdbarch_report_signal_info (gdbarch, fbsd_report_signal_info);
  set_gdbarch_skip_solib_resolver (gdbarch, fbsd_skip_solib_resolver);
  set_gdbarch_vsyscall_range (gdbarch, fbsd_vsyscall_range);

  /* `catch syscall' */
  set_xml_syscall_file_name (gdbarch, "syscalls/freebsd.xml");
  set_gdbarch_get_syscall_number (gdbarch, fbsd_get_syscall_number);
}
