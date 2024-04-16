/* Target-dependent code for GNU/Linux, architecture independent.

   Copyright (C) 2009-2024 Free Software Foundation, Inc.

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
#include "gdbtypes.h"
#include "linux-tdep.h"
#include "auxv.h"
#include "target.h"
#include "gdbthread.h"
#include "gdbcore.h"
#include "regcache.h"
#include "regset.h"
#include "elf/common.h"
#include "elf-bfd.h"
#include "inferior.h"
#include "cli/cli-utils.h"
#include "arch-utils.h"
#include "gdbsupport/gdb_obstack.h"
#include "observable.h"
#include "objfiles.h"
#include "infcall.h"
#include "gdbcmd.h"
#include "gdbsupport/gdb_regex.h"
#include "gdbsupport/enum-flags.h"
#include <optional>
#include "gcore.h"
#include "gcore-elf.h"
#include "solib-svr4.h"
#include "memtag.h"

#include <ctype.h>
#include <unordered_map>

/* This enum represents the values that the user can choose when
   informing the Linux kernel about which memory mappings will be
   dumped in a corefile.  They are described in the file
   Documentation/filesystems/proc.txt, inside the Linux kernel
   tree.  */

enum filter_flag
  {
    COREFILTER_ANON_PRIVATE = 1 << 0,
    COREFILTER_ANON_SHARED = 1 << 1,
    COREFILTER_MAPPED_PRIVATE = 1 << 2,
    COREFILTER_MAPPED_SHARED = 1 << 3,
    COREFILTER_ELF_HEADERS = 1 << 4,
    COREFILTER_HUGETLB_PRIVATE = 1 << 5,
    COREFILTER_HUGETLB_SHARED = 1 << 6,
  };
DEF_ENUM_FLAGS_TYPE (enum filter_flag, filter_flags);

/* This struct is used to map flags found in the "VmFlags:" field (in
   the /proc/<PID>/smaps file).  */

struct smaps_vmflags
  {
    /* Zero if this structure has not been initialized yet.  It
       probably means that the Linux kernel being used does not emit
       the "VmFlags:" field on "/proc/PID/smaps".  */

    unsigned int initialized_p : 1;

    /* Memory mapped I/O area (VM_IO, "io").  */

    unsigned int io_page : 1;

    /* Area uses huge TLB pages (VM_HUGETLB, "ht").  */

    unsigned int uses_huge_tlb : 1;

    /* Do not include this memory region on the coredump (VM_DONTDUMP, "dd").  */

    unsigned int exclude_coredump : 1;

    /* Is this a MAP_SHARED mapping (VM_SHARED, "sh").  */

    unsigned int shared_mapping : 1;

    /* Memory map has memory tagging enabled.  */

    unsigned int memory_tagging : 1;
  };

/* Data structure that holds the information contained in the
   /proc/<pid>/smaps file.  */

struct smaps_data
{
  ULONGEST start_address;
  ULONGEST end_address;
  std::string filename;
  struct smaps_vmflags vmflags;
  bool read;
  bool write;
  bool exec;
  bool priv;
  bool has_anonymous;
  bool mapping_anon_p;
  bool mapping_file_p;

  ULONGEST inode;
  ULONGEST offset;
};

/* Whether to take the /proc/PID/coredump_filter into account when
   generating a corefile.  */

static bool use_coredump_filter = true;

/* Whether the value of smaps_vmflags->exclude_coredump should be
   ignored, including mappings marked with the VM_DONTDUMP flag in
   the dump.  */
static bool dump_excluded_mappings = false;

/* This enum represents the signals' numbers on a generic architecture
   running the Linux kernel.  The definition of "generic" comes from
   the file <include/uapi/asm-generic/signal.h>, from the Linux kernel
   tree, which is the "de facto" implementation of signal numbers to
   be used by new architecture ports.

   For those architectures which have differences between the generic
   standard (e.g., Alpha), we define the different signals (and *only*
   those) in the specific target-dependent file (e.g.,
   alpha-linux-tdep.c, for Alpha).  Please refer to the architecture's
   tdep file for more information.

   ARM deserves a special mention here.  On the file
   <arch/arm/include/uapi/asm/signal.h>, it defines only one different
   (and ARM-only) signal, which is SIGSWI, with the same number as
   SIGRTMIN.  This signal is used only for a very specific target,
   called ArthurOS (from RISCOS).  Therefore, we do not handle it on
   the ARM-tdep file, and we can safely use the generic signal handler
   here for ARM targets.

   As stated above, this enum is derived from
   <include/uapi/asm-generic/signal.h>, from the Linux kernel
   tree.  */

enum
  {
    LINUX_SIGHUP = 1,
    LINUX_SIGINT = 2,
    LINUX_SIGQUIT = 3,
    LINUX_SIGILL = 4,
    LINUX_SIGTRAP = 5,
    LINUX_SIGABRT = 6,
    LINUX_SIGIOT = 6,
    LINUX_SIGBUS = 7,
    LINUX_SIGFPE = 8,
    LINUX_SIGKILL = 9,
    LINUX_SIGUSR1 = 10,
    LINUX_SIGSEGV = 11,
    LINUX_SIGUSR2 = 12,
    LINUX_SIGPIPE = 13,
    LINUX_SIGALRM = 14,
    LINUX_SIGTERM = 15,
    LINUX_SIGSTKFLT = 16,
    LINUX_SIGCHLD = 17,
    LINUX_SIGCONT = 18,
    LINUX_SIGSTOP = 19,
    LINUX_SIGTSTP = 20,
    LINUX_SIGTTIN = 21,
    LINUX_SIGTTOU = 22,
    LINUX_SIGURG = 23,
    LINUX_SIGXCPU = 24,
    LINUX_SIGXFSZ = 25,
    LINUX_SIGVTALRM = 26,
    LINUX_SIGPROF = 27,
    LINUX_SIGWINCH = 28,
    LINUX_SIGIO = 29,
    LINUX_SIGPOLL = LINUX_SIGIO,
    LINUX_SIGPWR = 30,
    LINUX_SIGSYS = 31,
    LINUX_SIGUNUSED = 31,

    LINUX_SIGRTMIN = 32,
    LINUX_SIGRTMAX = 64,
  };

struct linux_gdbarch_data
{
  struct type *siginfo_type = nullptr;
  int num_disp_step_buffers = 0;
};

static const registry<gdbarch>::key<linux_gdbarch_data>
     linux_gdbarch_data_handle;

static struct linux_gdbarch_data *
get_linux_gdbarch_data (struct gdbarch *gdbarch)
{
  struct linux_gdbarch_data *result = linux_gdbarch_data_handle.get (gdbarch);
  if (result == nullptr)
    result = linux_gdbarch_data_handle.emplace (gdbarch);
  return result;
}

/* Linux-specific cached data.  This is used by GDB for caching
   purposes for each inferior.  This helps reduce the overhead of
   transfering data from a remote target to the local host.  */
struct linux_info
{
  /* Cache of the inferior's vsyscall/vDSO mapping range.  Only valid
     if VSYSCALL_RANGE_P is positive.  This is cached because getting
     at this info requires an auxv lookup (which is itself cached),
     and looking through the inferior's mappings (which change
     throughout execution and therefore cannot be cached).  */
  struct mem_range vsyscall_range {};

  /* Zero if we haven't tried looking up the vsyscall's range before
     yet.  Positive if we tried looking it up, and found it.  Negative
     if we tried looking it up but failed.  */
  int vsyscall_range_p = 0;

  /* Inferior's displaced step buffers.  */
  std::optional<displaced_step_buffers> disp_step_bufs;
};

/* Per-inferior data key.  */
static const registry<inferior>::key<linux_info> linux_inferior_data;

/* Frees whatever allocated space there is to be freed and sets INF's
   linux cache data pointer to NULL.  */

static void
invalidate_linux_cache_inf (struct inferior *inf)
{
  linux_inferior_data.clear (inf);
}

/* inferior_execd observer.  */

static void
linux_inferior_execd (inferior *exec_inf, inferior *follow_inf)
{
  invalidate_linux_cache_inf (follow_inf);
}

/* Fetch the linux cache info for INF.  This function always returns a
   valid INFO pointer.  */

static struct linux_info *
get_linux_inferior_data (inferior *inf)
{
  linux_info *info = linux_inferior_data.get (inf);

  if (info == nullptr)
    info = linux_inferior_data.emplace (inf);

  return info;
}

/* See linux-tdep.h.  */

struct type *
linux_get_siginfo_type_with_fields (struct gdbarch *gdbarch,
				    linux_siginfo_extra_fields extra_fields)
{
  struct linux_gdbarch_data *linux_gdbarch_data;
  struct type *int_type, *uint_type, *long_type, *void_ptr_type, *short_type;
  struct type *uid_type, *pid_type;
  struct type *sigval_type, *clock_type;
  struct type *siginfo_type, *sifields_type;
  struct type *type;

  linux_gdbarch_data = get_linux_gdbarch_data (gdbarch);
  if (linux_gdbarch_data->siginfo_type != NULL)
    return linux_gdbarch_data->siginfo_type;

  type_allocator alloc (gdbarch);

  int_type = init_integer_type (alloc, gdbarch_int_bit (gdbarch),
			 	0, "int");
  uint_type = init_integer_type (alloc, gdbarch_int_bit (gdbarch),
				 1, "unsigned int");
  long_type = init_integer_type (alloc, gdbarch_long_bit (gdbarch),
				 0, "long");
  short_type = init_integer_type (alloc, gdbarch_long_bit (gdbarch),
				 0, "short");
  void_ptr_type = lookup_pointer_type (builtin_type (gdbarch)->builtin_void);

  /* sival_t */
  sigval_type = arch_composite_type (gdbarch, NULL, TYPE_CODE_UNION);
  sigval_type->set_name (xstrdup ("sigval_t"));
  append_composite_type_field (sigval_type, "sival_int", int_type);
  append_composite_type_field (sigval_type, "sival_ptr", void_ptr_type);

  /* __pid_t */
  pid_type = alloc.new_type (TYPE_CODE_TYPEDEF,
			     int_type->length () * TARGET_CHAR_BIT,
			     "__pid_t");
  pid_type->set_target_type (int_type);
  pid_type->set_target_is_stub (true);

  /* __uid_t */
  uid_type = alloc.new_type (TYPE_CODE_TYPEDEF,
			     uint_type->length () * TARGET_CHAR_BIT,
			     "__uid_t");
  uid_type->set_target_type (uint_type);
  uid_type->set_target_is_stub (true);

  /* __clock_t */
  clock_type = alloc.new_type (TYPE_CODE_TYPEDEF,
			       long_type->length () * TARGET_CHAR_BIT,
			       "__clock_t");
  clock_type->set_target_type (long_type);
  clock_type->set_target_is_stub (true);

  /* _sifields */
  sifields_type = arch_composite_type (gdbarch, NULL, TYPE_CODE_UNION);

  {
    const int si_max_size = 128;
    int si_pad_size;
    int size_of_int = gdbarch_int_bit (gdbarch) / HOST_CHAR_BIT;

    /* _pad */
    if (gdbarch_ptr_bit (gdbarch) == 64)
      si_pad_size = (si_max_size / size_of_int) - 4;
    else
      si_pad_size = (si_max_size / size_of_int) - 3;
    append_composite_type_field (sifields_type, "_pad",
				 init_vector_type (int_type, si_pad_size));
  }

  /* _kill */
  type = arch_composite_type (gdbarch, NULL, TYPE_CODE_STRUCT);
  append_composite_type_field (type, "si_pid", pid_type);
  append_composite_type_field (type, "si_uid", uid_type);
  append_composite_type_field (sifields_type, "_kill", type);

  /* _timer */
  type = arch_composite_type (gdbarch, NULL, TYPE_CODE_STRUCT);
  append_composite_type_field (type, "si_tid", int_type);
  append_composite_type_field (type, "si_overrun", int_type);
  append_composite_type_field (type, "si_sigval", sigval_type);
  append_composite_type_field (sifields_type, "_timer", type);

  /* _rt */
  type = arch_composite_type (gdbarch, NULL, TYPE_CODE_STRUCT);
  append_composite_type_field (type, "si_pid", pid_type);
  append_composite_type_field (type, "si_uid", uid_type);
  append_composite_type_field (type, "si_sigval", sigval_type);
  append_composite_type_field (sifields_type, "_rt", type);

  /* _sigchld */
  type = arch_composite_type (gdbarch, NULL, TYPE_CODE_STRUCT);
  append_composite_type_field (type, "si_pid", pid_type);
  append_composite_type_field (type, "si_uid", uid_type);
  append_composite_type_field (type, "si_status", int_type);
  append_composite_type_field (type, "si_utime", clock_type);
  append_composite_type_field (type, "si_stime", clock_type);
  append_composite_type_field (sifields_type, "_sigchld", type);

  /* _sigfault */
  type = arch_composite_type (gdbarch, NULL, TYPE_CODE_STRUCT);
  append_composite_type_field (type, "si_addr", void_ptr_type);

  /* Additional bound fields for _sigfault in case they were requested.  */
  if ((extra_fields & LINUX_SIGINFO_FIELD_ADDR_BND) != 0)
    {
      struct type *sigfault_bnd_fields;

      append_composite_type_field (type, "_addr_lsb", short_type);
      sigfault_bnd_fields = arch_composite_type (gdbarch, NULL, TYPE_CODE_STRUCT);
      append_composite_type_field (sigfault_bnd_fields, "_lower", void_ptr_type);
      append_composite_type_field (sigfault_bnd_fields, "_upper", void_ptr_type);
      append_composite_type_field (type, "_addr_bnd", sigfault_bnd_fields);
    }
  append_composite_type_field (sifields_type, "_sigfault", type);

  /* _sigpoll */
  type = arch_composite_type (gdbarch, NULL, TYPE_CODE_STRUCT);
  append_composite_type_field (type, "si_band", long_type);
  append_composite_type_field (type, "si_fd", int_type);
  append_composite_type_field (sifields_type, "_sigpoll", type);

  /* _sigsys */
  type = arch_composite_type (gdbarch, NULL, TYPE_CODE_STRUCT);
  append_composite_type_field (type, "_call_addr", void_ptr_type);
  append_composite_type_field (type, "_syscall", int_type);
  append_composite_type_field (type, "_arch", uint_type);
  append_composite_type_field (sifields_type, "_sigsys", type);

  /* struct siginfo */
  siginfo_type = arch_composite_type (gdbarch, NULL, TYPE_CODE_STRUCT);
  siginfo_type->set_name (xstrdup ("siginfo"));
  append_composite_type_field (siginfo_type, "si_signo", int_type);
  append_composite_type_field (siginfo_type, "si_errno", int_type);
  append_composite_type_field (siginfo_type, "si_code", int_type);
  append_composite_type_field_aligned (siginfo_type,
				       "_sifields", sifields_type,
				       long_type->length ());

  linux_gdbarch_data->siginfo_type = siginfo_type;

  return siginfo_type;
}

/* This function is suitable for architectures that don't
   extend/override the standard siginfo structure.  */

static struct type *
linux_get_siginfo_type (struct gdbarch *gdbarch)
{
  return linux_get_siginfo_type_with_fields (gdbarch, 0);
}

/* Return true if the target is running on uClinux instead of normal
   Linux kernel.  */

int
linux_is_uclinux (void)
{
  CORE_ADDR dummy;

  return (target_auxv_search (AT_NULL, &dummy) > 0
	  && target_auxv_search (AT_PAGESZ, &dummy) == 0);
}

static int
linux_has_shared_address_space (struct gdbarch *gdbarch)
{
  return linux_is_uclinux ();
}

/* This is how we want PTIDs from core files to be printed.  */

static std::string
linux_core_pid_to_str (struct gdbarch *gdbarch, ptid_t ptid)
{
  if (ptid.lwp () != 0)
    return string_printf ("LWP %ld", ptid.lwp ());

  return normal_pid_to_str (ptid);
}

/* Data from one mapping from /proc/PID/maps.  */

struct mapping
{
  ULONGEST addr;
  ULONGEST endaddr;
  std::string_view permissions;
  ULONGEST offset;
  std::string_view device;
  ULONGEST inode;

  /* This field is guaranteed to be NULL-terminated, hence it is not a
     std::string_view.  */
  const char *filename;
};

/* Service function for corefiles and info proc.  */

static mapping
read_mapping (const char *line)
{
  struct mapping mapping;
  const char *p = line;

  mapping.addr = strtoulst (p, &p, 16);
  if (*p == '-')
    p++;
  mapping.endaddr = strtoulst (p, &p, 16);

  p = skip_spaces (p);
  const char *permissions_start = p;
  while (*p && !isspace (*p))
    p++;
  mapping.permissions = {permissions_start, (size_t) (p - permissions_start)};

  mapping.offset = strtoulst (p, &p, 16);

  p = skip_spaces (p);
  const char *device_start = p;
  while (*p && !isspace (*p))
    p++;
  mapping.device = {device_start, (size_t) (p - device_start)};

  mapping.inode = strtoulst (p, &p, 10);

  p = skip_spaces (p);
  mapping.filename = p;

  return mapping;
}

/* Helper function to decode the "VmFlags" field in /proc/PID/smaps.

   This function was based on the documentation found on
   <Documentation/filesystems/proc.txt>, on the Linux kernel.

   Linux kernels before commit
   834f82e2aa9a8ede94b17b656329f850c1471514 (3.10) do not have this
   field on smaps.  */

static void
decode_vmflags (char *p, struct smaps_vmflags *v)
{
  char *saveptr = NULL;
  const char *s;

  v->initialized_p = 1;
  p = skip_to_space (p);
  p = skip_spaces (p);

  for (s = strtok_r (p, " ", &saveptr);
       s != NULL;
       s = strtok_r (NULL, " ", &saveptr))
    {
      if (strcmp (s, "io") == 0)
	v->io_page = 1;
      else if (strcmp (s, "ht") == 0)
	v->uses_huge_tlb = 1;
      else if (strcmp (s, "dd") == 0)
	v->exclude_coredump = 1;
      else if (strcmp (s, "sh") == 0)
	v->shared_mapping = 1;
      else if (strcmp (s, "mt") == 0)
	v->memory_tagging = 1;
    }
}

/* Regexes used by mapping_is_anonymous_p.  Put in a structure because
   they're initialized lazily.  */

struct mapping_regexes
{
  /* Matches "/dev/zero" filenames (with or without the "(deleted)"
     string in the end).  We know for sure, based on the Linux kernel
     code, that memory mappings whose associated filename is
     "/dev/zero" are guaranteed to be MAP_ANONYMOUS.  */
  compiled_regex dev_zero
    {"^/dev/zero\\( (deleted)\\)\\?$", REG_NOSUB,
     _("Could not compile regex to match /dev/zero filename")};

  /* Matches "/SYSV%08x" filenames (with or without the "(deleted)"
     string in the end).  These filenames refer to shared memory
     (shmem), and memory mappings associated with them are
     MAP_ANONYMOUS as well.  */
  compiled_regex shmem_file
    {"^/\\?SYSV[0-9a-fA-F]\\{8\\}\\( (deleted)\\)\\?$", REG_NOSUB,
     _("Could not compile regex to match shmem filenames")};

  /* A heuristic we use to try to mimic the Linux kernel's 'n_link ==
     0' code, which is responsible to decide if it is dealing with a
     'MAP_SHARED | MAP_ANONYMOUS' mapping.  In other words, if
     FILE_DELETED matches, it does not necessarily mean that we are
     dealing with an anonymous shared mapping.  However, there is no
     easy way to detect this currently, so this is the best
     approximation we have.

     As a result, GDB will dump readonly pages of deleted executables
     when using the default value of coredump_filter (0x33), while the
     Linux kernel will not dump those pages.  But we can live with
     that.  */
  compiled_regex file_deleted
    {" (deleted)$", REG_NOSUB,
     _("Could not compile regex to match '<file> (deleted)'")};
};

/* Return 1 if the memory mapping is anonymous, 0 otherwise.

   FILENAME is the name of the file present in the first line of the
   memory mapping, in the "/proc/PID/smaps" output.  For example, if
   the first line is:

   7fd0ca877000-7fd0d0da0000 r--p 00000000 fd:02 2100770   /path/to/file

   Then FILENAME will be "/path/to/file".  */

static int
mapping_is_anonymous_p (const char *filename)
{
  static std::optional<mapping_regexes> regexes;
  static int init_regex_p = 0;

  if (!init_regex_p)
    {
      /* Let's be pessimistic and assume there will be an error while
	 compiling the regex'es.  */
      init_regex_p = -1;

      regexes.emplace ();

      /* If we reached this point, then everything succeeded.  */
      init_regex_p = 1;
    }

  if (init_regex_p == -1)
    {
      const char deleted[] = " (deleted)";
      size_t del_len = sizeof (deleted) - 1;
      size_t filename_len = strlen (filename);

      /* There was an error while compiling the regex'es above.  In
	 order to try to give some reliable information to the caller,
	 we just try to find the string " (deleted)" in the filename.
	 If we managed to find it, then we assume the mapping is
	 anonymous.  */
      return (filename_len >= del_len
	      && strcmp (filename + filename_len - del_len, deleted) == 0);
    }

  if (*filename == '\0'
      || regexes->dev_zero.exec (filename, 0, NULL, 0) == 0
      || regexes->shmem_file.exec (filename, 0, NULL, 0) == 0
      || regexes->file_deleted.exec (filename, 0, NULL, 0) == 0)
    return 1;

  return 0;
}

/* Return 0 if the memory mapping (which is related to FILTERFLAGS, V,
   MAYBE_PRIVATE_P, MAPPING_ANONYMOUS_P, ADDR and OFFSET) should not
   be dumped, or greater than 0 if it should.

   In a nutshell, this is the logic that we follow in order to decide
   if a mapping should be dumped or not.

   - If the mapping is associated to a file whose name ends with
     " (deleted)", or if the file is "/dev/zero", or if it is
     "/SYSV%08x" (shared memory), or if there is no file associated
     with it, or if the AnonHugePages: or the Anonymous: fields in the
     /proc/PID/smaps have contents, then GDB considers this mapping to
     be anonymous.  Otherwise, GDB considers this mapping to be a
     file-backed mapping (because there will be a file associated with
     it).
 
     It is worth mentioning that, from all those checks described
     above, the most fragile is the one to see if the file name ends
     with " (deleted)".  This does not necessarily mean that the
     mapping is anonymous, because the deleted file associated with
     the mapping may have been a hard link to another file, for
     example.  The Linux kernel checks to see if "i_nlink == 0", but
     GDB cannot easily (and normally) do this check (iff running as
     root, it could find the mapping in /proc/PID/map_files/ and
     determine whether there still are other hard links to the
     inode/file).  Therefore, we made a compromise here, and we assume
     that if the file name ends with " (deleted)", then the mapping is
     indeed anonymous.  FWIW, this is something the Linux kernel could
     do better: expose this information in a more direct way.
 
   - If we see the flag "sh" in the "VmFlags:" field (in
     /proc/PID/smaps), then certainly the memory mapping is shared
     (VM_SHARED).  If we have access to the VmFlags, and we don't see
     the "sh" there, then certainly the mapping is private.  However,
     Linux kernels before commit
     834f82e2aa9a8ede94b17b656329f850c1471514 (3.10) do not have the
     "VmFlags:" field; in that case, we use another heuristic: if we
     see 'p' in the permission flags, then we assume that the mapping
     is private, even though the presence of the 's' flag there would
     mean VM_MAYSHARE, which means the mapping could still be private.
     This should work OK enough, however.

   - Even if, at the end, we decided that we should not dump the
     mapping, we still have to check if it is something like an ELF
     header (of a DSO or an executable, for example).  If it is, and
     if the user is interested in dump it, then we should dump it.  */

static int
dump_mapping_p (filter_flags filterflags, const struct smaps_vmflags *v,
		int maybe_private_p, int mapping_anon_p, int mapping_file_p,
		const char *filename, ULONGEST addr, ULONGEST offset)
{
  /* Initially, we trust in what we received from our caller.  This
     value may not be very precise (i.e., it was probably gathered
     from the permission line in the /proc/PID/smaps list, which
     actually refers to VM_MAYSHARE, and not VM_SHARED), but it is
     what we have until we take a look at the "VmFlags:" field
     (assuming that the version of the Linux kernel being used
     supports it, of course).  */
  int private_p = maybe_private_p;
  int dump_p;

  /* We always dump vDSO and vsyscall mappings, because it's likely that
     there'll be no file to read the contents from at core load time.
     The kernel does the same.  */
  if (strcmp ("[vdso]", filename) == 0
      || strcmp ("[vsyscall]", filename) == 0)
    return 1;

  if (v->initialized_p)
    {
      /* We never dump I/O mappings.  */
      if (v->io_page)
	return 0;

      /* Check if we should exclude this mapping.  */
      if (!dump_excluded_mappings && v->exclude_coredump)
	return 0;

      /* Update our notion of whether this mapping is shared or
	 private based on a trustworthy value.  */
      private_p = !v->shared_mapping;

      /* HugeTLB checking.  */
      if (v->uses_huge_tlb)
	{
	  if ((private_p && (filterflags & COREFILTER_HUGETLB_PRIVATE))
	      || (!private_p && (filterflags & COREFILTER_HUGETLB_SHARED)))
	    return 1;

	  return 0;
	}
    }

  if (private_p)
    {
      if (mapping_anon_p && mapping_file_p)
	{
	  /* This is a special situation.  It can happen when we see a
	     mapping that is file-backed, but that contains anonymous
	     pages.  */
	  dump_p = ((filterflags & COREFILTER_ANON_PRIVATE) != 0
		    || (filterflags & COREFILTER_MAPPED_PRIVATE) != 0);
	}
      else if (mapping_anon_p)
	dump_p = (filterflags & COREFILTER_ANON_PRIVATE) != 0;
      else
	dump_p = (filterflags & COREFILTER_MAPPED_PRIVATE) != 0;
    }
  else
    {
      if (mapping_anon_p && mapping_file_p)
	{
	  /* This is a special situation.  It can happen when we see a
	     mapping that is file-backed, but that contains anonymous
	     pages.  */
	  dump_p = ((filterflags & COREFILTER_ANON_SHARED) != 0
		    || (filterflags & COREFILTER_MAPPED_SHARED) != 0);
	}
      else if (mapping_anon_p)
	dump_p = (filterflags & COREFILTER_ANON_SHARED) != 0;
      else
	dump_p = (filterflags & COREFILTER_MAPPED_SHARED) != 0;
    }

  /* Even if we decided that we shouldn't dump this mapping, we still
     have to check whether (a) the user wants us to dump mappings
     containing an ELF header, and (b) the mapping in question
     contains an ELF header.  If (a) and (b) are true, then we should
     dump this mapping.

     A mapping contains an ELF header if it is a private mapping, its
     offset is zero, and its first word is ELFMAG.  */
  if (!dump_p && private_p && offset == 0
      && (filterflags & COREFILTER_ELF_HEADERS) != 0)
    {
      /* Useful define specifying the size of the ELF magical
	 header.  */
#ifndef SELFMAG
#define SELFMAG 4
#endif

      /* Let's check if we have an ELF header.  */
      gdb_byte h[SELFMAG];
      if (target_read_memory (addr, h, SELFMAG) == 0)
	{
	  /* The EI_MAG* and ELFMAG* constants come from
	     <elf/common.h>.  */
	  if (h[EI_MAG0] == ELFMAG0 && h[EI_MAG1] == ELFMAG1
	      && h[EI_MAG2] == ELFMAG2 && h[EI_MAG3] == ELFMAG3)
	    {
	      /* This mapping contains an ELF header, so we
		 should dump it.  */
	      dump_p = 1;
	    }
	}
    }

  return dump_p;
}

/* As above, but return true only when we should dump the NT_FILE
   entry.  */

static int
dump_note_entry_p (filter_flags filterflags, const struct smaps_vmflags *v,
		int maybe_private_p, int mapping_anon_p, int mapping_file_p,
		const char *filename, ULONGEST addr, ULONGEST offset)
{
  /* vDSO and vsyscall mappings will end up in the core file.  Don't
     put them in the NT_FILE note.  */
  if (strcmp ("[vdso]", filename) == 0
      || strcmp ("[vsyscall]", filename) == 0)
    return 0;

  /* Otherwise, any other file-based mapping should be placed in the
     note.  */
  return 1;
}

/* Implement the "info proc" command.  */

static void
linux_info_proc (struct gdbarch *gdbarch, const char *args,
		 enum info_proc_what what)
{
  /* A long is used for pid instead of an int to avoid a loss of precision
     compiler warning from the output of strtoul.  */
  long pid;
  int cmdline_f = (what == IP_MINIMAL || what == IP_CMDLINE || what == IP_ALL);
  int cwd_f = (what == IP_MINIMAL || what == IP_CWD || what == IP_ALL);
  int exe_f = (what == IP_MINIMAL || what == IP_EXE || what == IP_ALL);
  int mappings_f = (what == IP_MAPPINGS || what == IP_ALL);
  int status_f = (what == IP_STATUS || what == IP_ALL);
  int stat_f = (what == IP_STAT || what == IP_ALL);
  char filename[100];
  fileio_error target_errno;

  if (args && isdigit (args[0]))
    {
      char *tem;

      pid = strtoul (args, &tem, 10);
      args = tem;
    }
  else
    {
      if (!target_has_execution ())
	error (_("No current process: you must name one."));
      if (current_inferior ()->fake_pid_p)
	error (_("Can't determine the current process's PID: you must name one."));

      pid = current_inferior ()->pid;
    }

  args = skip_spaces (args);
  if (args && args[0])
    error (_("Too many parameters: %s"), args);

  gdb_printf (_("process %ld\n"), pid);
  if (cmdline_f)
    {
      xsnprintf (filename, sizeof filename, "/proc/%ld/cmdline", pid);
      gdb_byte *buffer;
      ssize_t len = target_fileio_read_alloc (NULL, filename, &buffer);

      if (len > 0)
	{
	  gdb::unique_xmalloc_ptr<char> cmdline ((char *) buffer);
	  ssize_t pos;

	  for (pos = 0; pos < len - 1; pos++)
	    {
	      if (buffer[pos] == '\0')
		buffer[pos] = ' ';
	    }
	  buffer[len - 1] = '\0';
	  gdb_printf ("cmdline = '%s'\n", buffer);
	}
      else
	warning (_("unable to open /proc file '%s'"), filename);
    }
  if (cwd_f)
    {
      xsnprintf (filename, sizeof filename, "/proc/%ld/cwd", pid);
      std::optional<std::string> contents
	= target_fileio_readlink (NULL, filename, &target_errno);
      if (contents.has_value ())
	gdb_printf ("cwd = '%s'\n", contents->c_str ());
      else
	warning (_("unable to read link '%s'"), filename);
    }
  if (exe_f)
    {
      xsnprintf (filename, sizeof filename, "/proc/%ld/exe", pid);
      std::optional<std::string> contents
	= target_fileio_readlink (NULL, filename, &target_errno);
      if (contents.has_value ())
	gdb_printf ("exe = '%s'\n", contents->c_str ());
      else
	warning (_("unable to read link '%s'"), filename);
    }
  if (mappings_f)
    {
      xsnprintf (filename, sizeof filename, "/proc/%ld/maps", pid);
      gdb::unique_xmalloc_ptr<char> map
	= target_fileio_read_stralloc (NULL, filename);
      if (map != NULL)
	{
	  char *line;

	  gdb_printf (_("Mapped address spaces:\n\n"));
	  if (gdbarch_addr_bit (gdbarch) == 32)
	    {
	      gdb_printf ("\t%10s %10s %10s %10s  %s %s\n",
			  "Start Addr", "  End Addr", "      Size",
			  "    Offset", "Perms  ", "objfile");
	    }
	  else
	    {
	      gdb_printf ("  %18s %18s %10s %10s  %s %s\n",
			  "Start Addr", "  End Addr", "      Size",
			  "    Offset", "Perms ", "objfile");
	    }

	  char *saveptr;
	  for (line = strtok_r (map.get (), "\n", &saveptr);
	       line;
	       line = strtok_r (NULL, "\n", &saveptr))
	    {
	      struct mapping m = read_mapping (line);

	      if (gdbarch_addr_bit (gdbarch) == 32)
		{
		  gdb_printf ("\t%10s %10s %10s %10s  %-5.*s  %s\n",
			      paddress (gdbarch, m.addr),
			      paddress (gdbarch, m.endaddr),
			      hex_string (m.endaddr - m.addr),
			      hex_string (m.offset),
			      (int) m.permissions.size (),
			      m.permissions.data (),
			      m.filename);
		}
	      else
		{
		  gdb_printf ("  %18s %18s %10s %10s  %-5.*s  %s\n",
			      paddress (gdbarch, m.addr),
			      paddress (gdbarch, m.endaddr),
			      hex_string (m.endaddr - m.addr),
			      hex_string (m.offset),
			      (int) m.permissions.size (),
			      m.permissions.data (),
			      m.filename);
		}
	    }
	}
      else
	warning (_("unable to open /proc file '%s'"), filename);
    }
  if (status_f)
    {
      xsnprintf (filename, sizeof filename, "/proc/%ld/status", pid);
      gdb::unique_xmalloc_ptr<char> status
	= target_fileio_read_stralloc (NULL, filename);
      if (status)
	gdb_puts (status.get ());
      else
	warning (_("unable to open /proc file '%s'"), filename);
    }
  if (stat_f)
    {
      xsnprintf (filename, sizeof filename, "/proc/%ld/stat", pid);
      gdb::unique_xmalloc_ptr<char> statstr
	= target_fileio_read_stralloc (NULL, filename);
      if (statstr)
	{
	  const char *p = statstr.get ();

	  gdb_printf (_("Process: %s\n"),
		      pulongest (strtoulst (p, &p, 10)));

	  p = skip_spaces (p);
	  if (*p == '(')
	    {
	      /* ps command also relies on no trailing fields
		 ever contain ')'.  */
	      const char *ep = strrchr (p, ')');
	      if (ep != NULL)
		{
		  gdb_printf ("Exec file: %.*s\n",
			      (int) (ep - p - 1), p + 1);
		  p = ep + 1;
		}
	    }

	  p = skip_spaces (p);
	  if (*p)
	    gdb_printf (_("State: %c\n"), *p++);

	  if (*p)
	    gdb_printf (_("Parent process: %s\n"),
			pulongest (strtoulst (p, &p, 10)));
	  if (*p)
	    gdb_printf (_("Process group: %s\n"),
			pulongest (strtoulst (p, &p, 10)));
	  if (*p)
	    gdb_printf (_("Session id: %s\n"),
			pulongest (strtoulst (p, &p, 10)));
	  if (*p)
	    gdb_printf (_("TTY: %s\n"),
			pulongest (strtoulst (p, &p, 10)));
	  if (*p)
	    gdb_printf (_("TTY owner process group: %s\n"),
			pulongest (strtoulst (p, &p, 10)));

	  if (*p)
	    gdb_printf (_("Flags: %s\n"),
			hex_string (strtoulst (p, &p, 10)));
	  if (*p)
	    gdb_printf (_("Minor faults (no memory page): %s\n"),
			pulongest (strtoulst (p, &p, 10)));
	  if (*p)
	    gdb_printf (_("Minor faults, children: %s\n"),
			pulongest (strtoulst (p, &p, 10)));
	  if (*p)
	    gdb_printf (_("Major faults (memory page faults): %s\n"),
			pulongest (strtoulst (p, &p, 10)));
	  if (*p)
	    gdb_printf (_("Major faults, children: %s\n"),
			pulongest (strtoulst (p, &p, 10)));
	  if (*p)
	    gdb_printf (_("utime: %s\n"),
			pulongest (strtoulst (p, &p, 10)));
	  if (*p)
	    gdb_printf (_("stime: %s\n"),
			pulongest (strtoulst (p, &p, 10)));
	  if (*p)
	    gdb_printf (_("utime, children: %s\n"),
			pulongest (strtoulst (p, &p, 10)));
	  if (*p)
	    gdb_printf (_("stime, children: %s\n"),
			pulongest (strtoulst (p, &p, 10)));
	  if (*p)
	    gdb_printf (_("jiffies remaining in current "
			  "time slice: %s\n"),
			pulongest (strtoulst (p, &p, 10)));
	  if (*p)
	    gdb_printf (_("'nice' value: %s\n"),
			pulongest (strtoulst (p, &p, 10)));
	  if (*p)
	    gdb_printf (_("jiffies until next timeout: %s\n"),
			pulongest (strtoulst (p, &p, 10)));
	  if (*p)
	    gdb_printf (_("jiffies until next SIGALRM: %s\n"),
			pulongest (strtoulst (p, &p, 10)));
	  if (*p)
	    gdb_printf (_("start time (jiffies since "
			  "system boot): %s\n"),
			pulongest (strtoulst (p, &p, 10)));
	  if (*p)
	    gdb_printf (_("Virtual memory size: %s\n"),
			pulongest (strtoulst (p, &p, 10)));
	  if (*p)
	    gdb_printf (_("Resident set size: %s\n"),
			pulongest (strtoulst (p, &p, 10)));
	  if (*p)
	    gdb_printf (_("rlim: %s\n"),
			pulongest (strtoulst (p, &p, 10)));
	  if (*p)
	    gdb_printf (_("Start of text: %s\n"),
			hex_string (strtoulst (p, &p, 10)));
	  if (*p)
	    gdb_printf (_("End of text: %s\n"),
			hex_string (strtoulst (p, &p, 10)));
	  if (*p)
	    gdb_printf (_("Start of stack: %s\n"),
			hex_string (strtoulst (p, &p, 10)));
#if 0	/* Don't know how architecture-dependent the rest is...
	   Anyway the signal bitmap info is available from "status".  */
	  if (*p)
	    gdb_printf (_("Kernel stack pointer: %s\n"),
			hex_string (strtoulst (p, &p, 10)));
	  if (*p)
	    gdb_printf (_("Kernel instr pointer: %s\n"),
			hex_string (strtoulst (p, &p, 10)));
	  if (*p)
	    gdb_printf (_("Pending signals bitmap: %s\n"),
			hex_string (strtoulst (p, &p, 10)));
	  if (*p)
	    gdb_printf (_("Blocked signals bitmap: %s\n"),
			hex_string (strtoulst (p, &p, 10)));
	  if (*p)
	    gdb_printf (_("Ignored signals bitmap: %s\n"),
			hex_string (strtoulst (p, &p, 10)));
	  if (*p)
	    gdb_printf (_("Catched signals bitmap: %s\n"),
			hex_string (strtoulst (p, &p, 10)));
	  if (*p)
	    gdb_printf (_("wchan (system call): %s\n"),
			hex_string (strtoulst (p, &p, 10)));
#endif
	}
      else
	warning (_("unable to open /proc file '%s'"), filename);
    }
}

/* Implementation of `gdbarch_read_core_file_mappings', as defined in
   gdbarch.h.
   
   This function reads the NT_FILE note (which BFD turns into the
   section ".note.linuxcore.file").  The format of this note / section
   is described as follows in the Linux kernel sources in
   fs/binfmt_elf.c:
   
      long count     -- how many files are mapped
      long page_size -- units for file_ofs
      array of [COUNT] elements of
	long start
	long end
	long file_ofs
      followed by COUNT filenames in ASCII: "FILE1" NUL "FILE2" NUL...
      
   CBFD is the BFD of the core file.

   PRE_LOOP_CB is the callback function to invoke prior to starting
   the loop which processes individual entries.  This callback will
   only be executed after the note has been examined in enough
   detail to verify that it's not malformed in some way.
   
   LOOP_CB is the callback function that will be executed once
   for each mapping.  */

static void
linux_read_core_file_mappings
  (struct gdbarch *gdbarch,
   struct bfd *cbfd,
   read_core_file_mappings_pre_loop_ftype pre_loop_cb,
   read_core_file_mappings_loop_ftype  loop_cb)
{
  /* Ensure that ULONGEST is big enough for reading 64-bit core files.  */
  static_assert (sizeof (ULONGEST) >= 8);

  /* It's not required that the NT_FILE note exists, so return silently
     if it's not found.  Beyond this point though, we'll complain
     if problems are found.  */
  asection *section = bfd_get_section_by_name (cbfd, ".note.linuxcore.file");
  if (section == nullptr)
    return;

  unsigned int addr_size_bits = gdbarch_addr_bit (gdbarch);
  unsigned int addr_size = addr_size_bits / 8;
  size_t note_size = bfd_section_size (section);

  if (note_size < 2 * addr_size)
    {
      warning (_("malformed core note - too short for header"));
      return;
    }

  gdb::byte_vector contents (note_size);
  if (!bfd_get_section_contents (core_bfd, section, contents.data (),
				 0, note_size))
    {
      warning (_("could not get core note contents"));
      return;
    }

  gdb_byte *descdata = contents.data ();
  char *descend = (char *) descdata + note_size;

  if (descdata[note_size - 1] != '\0')
    {
      warning (_("malformed note - does not end with \\0"));
      return;
    }

  ULONGEST count = bfd_get (addr_size_bits, core_bfd, descdata);
  descdata += addr_size;

  ULONGEST page_size = bfd_get (addr_size_bits, core_bfd, descdata);
  descdata += addr_size;

  if (note_size < 2 * addr_size + count * 3 * addr_size)
    {
      warning (_("malformed note - too short for supplied file count"));
      return;
    }

  char *filenames = (char *) descdata + count * 3 * addr_size;

  /* Make sure that the correct number of filenames exist.  Complain
     if there aren't enough or are too many.  */
  char *f = filenames;
  for (int i = 0; i < count; i++)
    {
      if (f >= descend)
	{
	  warning (_("malformed note - filename area is too small"));
	  return;
	}
      f += strnlen (f, descend - f) + 1;
    }
  /* Complain, but don't return early if the filename area is too big.  */
  if (f != descend)
    warning (_("malformed note - filename area is too big"));

  const bfd_build_id *orig_build_id = cbfd->build_id;
  std::unordered_map<ULONGEST, const bfd_build_id *> vma_map;

  /* Search for solib build-ids in the core file.  Each time one is found,
     map the start vma of the corresponding elf header to the build-id.  */
  for (bfd_section *sec = cbfd->sections; sec != nullptr; sec = sec->next)
    {
      cbfd->build_id = nullptr;

      if (sec->flags & SEC_LOAD
	  && (get_elf_backend_data (cbfd)->elf_backend_core_find_build_id
	       (cbfd, (bfd_vma) sec->filepos)))
	vma_map[sec->vma] = cbfd->build_id;
    }

  cbfd->build_id = orig_build_id;
  pre_loop_cb (count);

  for (int i = 0; i < count; i++)
    {
      ULONGEST start = bfd_get (addr_size_bits, core_bfd, descdata);
      descdata += addr_size;
      ULONGEST end = bfd_get (addr_size_bits, core_bfd, descdata);
      descdata += addr_size;
      ULONGEST file_ofs
	= bfd_get (addr_size_bits, core_bfd, descdata) * page_size;
      descdata += addr_size;
      char * filename = filenames;
      filenames += strlen ((char *) filenames) + 1;
      const bfd_build_id *build_id = nullptr;
      auto vma_map_it = vma_map.find (start);

      if (vma_map_it != vma_map.end ())
	build_id = vma_map_it->second;

      loop_cb (i, start, end, file_ofs, filename, build_id);
    }
}

/* Implement "info proc mappings" for a corefile.  */

static void
linux_core_info_proc_mappings (struct gdbarch *gdbarch, const char *args)
{
  linux_read_core_file_mappings (gdbarch, core_bfd,
    [=] (ULONGEST count)
      {
	gdb_printf (_("Mapped address spaces:\n\n"));
	if (gdbarch_addr_bit (gdbarch) == 32)
	  {
	    gdb_printf ("\t%10s %10s %10s %10s %s\n",
			"Start Addr",
			"  End Addr",
			"      Size", "    Offset", "objfile");
	  }
	else
	  {
	    gdb_printf ("  %18s %18s %10s %10s %s\n",
			"Start Addr",
			"  End Addr",
			"      Size", "    Offset", "objfile");
	  }
      },
    [=] (int num, ULONGEST start, ULONGEST end, ULONGEST file_ofs,
	 const char *filename, const bfd_build_id *build_id)
      {
	if (gdbarch_addr_bit (gdbarch) == 32)
	  gdb_printf ("\t%10s %10s %10s %10s %s\n",
		      paddress (gdbarch, start),
		      paddress (gdbarch, end),
		      hex_string (end - start),
		      hex_string (file_ofs),
		      filename);
	else
	  gdb_printf ("  %18s %18s %10s %10s %s\n",
		      paddress (gdbarch, start),
		      paddress (gdbarch, end),
		      hex_string (end - start),
		      hex_string (file_ofs),
		      filename);
      });
}

/* Implement "info proc" for a corefile.  */

static void
linux_core_info_proc (struct gdbarch *gdbarch, const char *args,
		      enum info_proc_what what)
{
  int exe_f = (what == IP_MINIMAL || what == IP_EXE || what == IP_ALL);
  int mappings_f = (what == IP_MAPPINGS || what == IP_ALL);

  if (exe_f)
    {
      const char *exe;

      exe = bfd_core_file_failing_command (core_bfd);
      if (exe != NULL)
	gdb_printf ("exe = '%s'\n", exe);
      else
	warning (_("unable to find command name in core file"));
    }

  if (mappings_f)
    linux_core_info_proc_mappings (gdbarch, args);

  if (!exe_f && !mappings_f)
    error (_("unable to handle request"));
}

/* Read siginfo data from the core, if possible.  Returns -1 on
   failure.  Otherwise, returns the number of bytes read.  READBUF,
   OFFSET, and LEN are all as specified by the to_xfer_partial
   interface.  */

static LONGEST
linux_core_xfer_siginfo (struct gdbarch *gdbarch, gdb_byte *readbuf,
			 ULONGEST offset, ULONGEST len)
{
  thread_section_name section_name (".note.linuxcore.siginfo", inferior_ptid);
  asection *section = bfd_get_section_by_name (core_bfd, section_name.c_str ());
  if (section == NULL)
    return -1;

  if (!bfd_get_section_contents (core_bfd, section, readbuf, offset, len))
    return -1;

  return len;
}

typedef int linux_find_memory_region_ftype (ULONGEST vaddr, ULONGEST size,
					    ULONGEST offset, ULONGEST inode,
					    int read, int write,
					    int exec, int modified,
					    bool memory_tagged,
					    const char *filename,
					    void *data);

typedef int linux_dump_mapping_p_ftype (filter_flags filterflags,
					const struct smaps_vmflags *v,
					int maybe_private_p,
					int mapping_anon_p,
					int mapping_file_p,
					const char *filename,
					ULONGEST addr,
					ULONGEST offset);

/* Helper function to parse the contents of /proc/<pid>/smaps into a data
   structure, for easy access.

   DATA is the contents of the smaps file.  The parsed contents are stored
   into the SMAPS vector.  */

static std::vector<struct smaps_data>
parse_smaps_data (const char *data,
		  const std::string maps_filename)
{
  char *line, *t;

  gdb_assert (data != nullptr);

  line = strtok_r ((char *) data, "\n", &t);

  std::vector<struct smaps_data> smaps;

  while (line != NULL)
    {
      struct smaps_vmflags v;
      int read, write, exec, priv;
      int has_anonymous = 0;
      int mapping_anon_p;
      int mapping_file_p;

      memset (&v, 0, sizeof (v));
      struct mapping m = read_mapping (line);
      mapping_anon_p = mapping_is_anonymous_p (m.filename);
      /* If the mapping is not anonymous, then we can consider it
	 to be file-backed.  These two states (anonymous or
	 file-backed) seem to be exclusive, but they can actually
	 coexist.  For example, if a file-backed mapping has
	 "Anonymous:" pages (see more below), then the Linux
	 kernel will dump this mapping when the user specified
	 that she only wants anonymous mappings in the corefile
	 (*even* when she explicitly disabled the dumping of
	 file-backed mappings).  */
      mapping_file_p = !mapping_anon_p;

      /* Decode permissions.  */
      auto has_perm = [&m] (char c)
	{ return m.permissions.find (c) != std::string_view::npos; };
      read = has_perm ('r');
      write = has_perm ('w');
      exec = has_perm ('x');

      /* 'private' here actually means VM_MAYSHARE, and not
	 VM_SHARED.  In order to know if a mapping is really
	 private or not, we must check the flag "sh" in the
	 VmFlags field.  This is done by decode_vmflags.  However,
	 if we are using a Linux kernel released before the commit
	 834f82e2aa9a8ede94b17b656329f850c1471514 (3.10), we will
	 not have the VmFlags there.  In this case, there is
	 really no way to know if we are dealing with VM_SHARED,
	 so we just assume that VM_MAYSHARE is enough.  */
      priv = has_perm ('p');

      /* Try to detect if region should be dumped by parsing smaps
	 counters.  */
      for (line = strtok_r (NULL, "\n", &t);
	   line != NULL && line[0] >= 'A' && line[0] <= 'Z';
	   line = strtok_r (NULL, "\n", &t))
	{
	  char keyword[64 + 1];

	  if (sscanf (line, "%64s", keyword) != 1)
	    {
	      warning (_("Error parsing {s,}maps file '%s'"),
		       maps_filename.c_str ());
	      break;
	    }

	  if (strcmp (keyword, "Anonymous:") == 0)
	    {
	      /* Older Linux kernels did not support the
		 "Anonymous:" counter.  Check it here.  */
	      has_anonymous = 1;
	    }
	  else if (strcmp (keyword, "VmFlags:") == 0)
	    decode_vmflags (line, &v);

	  if (strcmp (keyword, "AnonHugePages:") == 0
	      || strcmp (keyword, "Anonymous:") == 0)
	    {
	      unsigned long number;

	      if (sscanf (line, "%*s%lu", &number) != 1)
		{
		  warning (_("Error parsing {s,}maps file '%s' number"),
			   maps_filename.c_str ());
		  break;
		}
	      if (number > 0)
		{
		  /* Even if we are dealing with a file-backed
		     mapping, if it contains anonymous pages we
		     consider it to be *also* an anonymous
		     mapping, because this is what the Linux
		     kernel does:

		     // Dump segments that have been written to.
		     if (vma->anon_vma && FILTER(ANON_PRIVATE))
		       goto whole;

		    Note that if the mapping is already marked as
		    file-backed (i.e., mapping_file_p is
		    non-zero), then this is a special case, and
		    this mapping will be dumped either when the
		    user wants to dump file-backed *or* anonymous
		    mappings.  */
		  mapping_anon_p = 1;
		}
	    }
	}
      /* Save the smaps entry to the vector.  */
	struct smaps_data map;

	map.start_address = m.addr;
	map.end_address = m.endaddr;
	map.filename = m.filename;
	map.vmflags = v;
	map.read = read? true : false;
	map.write = write? true : false;
	map.exec = exec? true : false;
	map.priv = priv? true : false;
	map.has_anonymous = has_anonymous;
	map.mapping_anon_p = mapping_anon_p? true : false;
	map.mapping_file_p = mapping_file_p? true : false;
	map.offset = m.offset;
	map.inode = m.inode;

	smaps.emplace_back (map);
    }

  return smaps;
}

/* Helper that checks if an address is in a memory tag page for a live
   process.  */

static bool
linux_process_address_in_memtag_page (CORE_ADDR address)
{
  if (current_inferior ()->fake_pid_p)
    return false;

  pid_t pid = current_inferior ()->pid;

  std::string smaps_file = string_printf ("/proc/%d/smaps", pid);

  gdb::unique_xmalloc_ptr<char> data
    = target_fileio_read_stralloc (NULL, smaps_file.c_str ());

  if (data == nullptr)
    return false;

  /* Parse the contents of smaps into a vector.  */
  std::vector<struct smaps_data> smaps
    = parse_smaps_data (data.get (), smaps_file);

  for (const smaps_data &map : smaps)
    {
      /* Is the address within [start_address, end_address) in a page
	 mapped with memory tagging?  */
      if (address >= map.start_address
	  && address < map.end_address
	  && map.vmflags.memory_tagging)
	return true;
    }

  return false;
}

/* Helper that checks if an address is in a memory tag page for a core file
   process.  */

static bool
linux_core_file_address_in_memtag_page (CORE_ADDR address)
{
  if (core_bfd == nullptr)
    return false;

  memtag_section_info info;
  return get_next_core_memtag_section (core_bfd, nullptr, address, info);
}

/* See linux-tdep.h.  */

bool
linux_address_in_memtag_page (CORE_ADDR address)
{
  if (!target_has_execution ())
    return linux_core_file_address_in_memtag_page (address);

  return linux_process_address_in_memtag_page (address);
}

/* List memory regions in the inferior for a corefile.  */

static int
linux_find_memory_regions_full (struct gdbarch *gdbarch,
				linux_dump_mapping_p_ftype *should_dump_mapping_p,
				linux_find_memory_region_ftype *func,
				void *obfd)
{
  pid_t pid;
  /* Default dump behavior of coredump_filter (0x33), according to
     Documentation/filesystems/proc.txt from the Linux kernel
     tree.  */
  filter_flags filterflags = (COREFILTER_ANON_PRIVATE
			      | COREFILTER_ANON_SHARED
			      | COREFILTER_ELF_HEADERS
			      | COREFILTER_HUGETLB_PRIVATE);

  /* We need to know the real target PID to access /proc.  */
  if (current_inferior ()->fake_pid_p)
    return 1;

  pid = current_inferior ()->pid;

  if (use_coredump_filter)
    {
      std::string core_dump_filter_name
	= string_printf ("/proc/%d/coredump_filter", pid);

      gdb::unique_xmalloc_ptr<char> coredumpfilterdata
	= target_fileio_read_stralloc (NULL, core_dump_filter_name.c_str ());

      if (coredumpfilterdata != NULL)
	{
	  unsigned int flags;

	  sscanf (coredumpfilterdata.get (), "%x", &flags);
	  filterflags = (enum filter_flag) flags;
	}
    }

  std::string maps_filename = string_printf ("/proc/%d/smaps", pid);

  gdb::unique_xmalloc_ptr<char> data
    = target_fileio_read_stralloc (NULL, maps_filename.c_str ());

  if (data == NULL)
    {
      /* Older Linux kernels did not support /proc/PID/smaps.  */
      maps_filename = string_printf ("/proc/%d/maps", pid);
      data = target_fileio_read_stralloc (NULL, maps_filename.c_str ());

      if (data == nullptr)
	return 1;
    }

  /* Parse the contents of smaps into a vector.  */
  std::vector<struct smaps_data> smaps
    = parse_smaps_data (data.get (), maps_filename.c_str ());

  for (const struct smaps_data &map : smaps)
    {
      int should_dump_p = 0;

      if (map.has_anonymous)
	{
	  should_dump_p
	    = should_dump_mapping_p (filterflags, &map.vmflags,
				     map.priv,
				     map.mapping_anon_p,
				     map.mapping_file_p,
				     map.filename.c_str (),
				     map.start_address,
				     map.offset);
	}
      else
	{
	  /* Older Linux kernels did not support the "Anonymous:" counter.
	     If it is missing, we can't be sure - dump all the pages.  */
	  should_dump_p = 1;
	}

      /* Invoke the callback function to create the corefile segment.  */
      if (should_dump_p)
	{
	  func (map.start_address, map.end_address - map.start_address,
		map.offset, map.inode, map.read, map.write, map.exec,
		1, /* MODIFIED is true because we want to dump
		      the mapping.  */
		map.vmflags.memory_tagging != 0,
		map.filename.c_str (), obfd);
	}
    }

  return 0;
}

/* A structure for passing information through
   linux_find_memory_regions_full.  */

struct linux_find_memory_regions_data
{
  /* The original callback.  */

  find_memory_region_ftype func;

  /* The original datum.  */

  void *obfd;
};

/* A callback for linux_find_memory_regions that converts between the
   "full"-style callback and find_memory_region_ftype.  */

static int
linux_find_memory_regions_thunk (ULONGEST vaddr, ULONGEST size,
				 ULONGEST offset, ULONGEST inode,
				 int read, int write, int exec, int modified,
				 bool memory_tagged,
				 const char *filename, void *arg)
{
  struct linux_find_memory_regions_data *data
    = (struct linux_find_memory_regions_data *) arg;

  return data->func (vaddr, size, read, write, exec, modified, memory_tagged,
		     data->obfd);
}

/* A variant of linux_find_memory_regions_full that is suitable as the
   gdbarch find_memory_regions method.  */

static int
linux_find_memory_regions (struct gdbarch *gdbarch,
			   find_memory_region_ftype func, void *obfd)
{
  struct linux_find_memory_regions_data data;

  data.func = func;
  data.obfd = obfd;

  return linux_find_memory_regions_full (gdbarch,
					 dump_mapping_p,
					 linux_find_memory_regions_thunk,
					 &data);
}

/* This is used to pass information from
   linux_make_mappings_corefile_notes through
   linux_find_memory_regions_full.  */

struct linux_make_mappings_data
{
  /* Number of files mapped.  */
  ULONGEST file_count;

  /* The obstack for the main part of the data.  */
  struct obstack *data_obstack;

  /* The filename obstack.  */
  struct obstack *filename_obstack;

  /* The architecture's "long" type.  */
  struct type *long_type;
};

static linux_find_memory_region_ftype linux_make_mappings_callback;

/* A callback for linux_find_memory_regions_full that updates the
   mappings data for linux_make_mappings_corefile_notes.

   MEMORY_TAGGED is true if the memory region contains memory tags, false
   otherwise.  */

static int
linux_make_mappings_callback (ULONGEST vaddr, ULONGEST size,
			      ULONGEST offset, ULONGEST inode,
			      int read, int write, int exec, int modified,
			      bool memory_tagged,
			      const char *filename, void *data)
{
  struct linux_make_mappings_data *map_data
    = (struct linux_make_mappings_data *) data;
  gdb_byte buf[sizeof (ULONGEST)];

  if (*filename == '\0' || inode == 0)
    return 0;

  ++map_data->file_count;

  pack_long (buf, map_data->long_type, vaddr);
  obstack_grow (map_data->data_obstack, buf, map_data->long_type->length ());
  pack_long (buf, map_data->long_type, vaddr + size);
  obstack_grow (map_data->data_obstack, buf, map_data->long_type->length ());
  pack_long (buf, map_data->long_type, offset);
  obstack_grow (map_data->data_obstack, buf, map_data->long_type->length ());

  obstack_grow_str0 (map_data->filename_obstack, filename);

  return 0;
}

/* Write the file mapping data to the core file, if possible.  OBFD is
   the output BFD.  NOTE_DATA is the current note data, and NOTE_SIZE
   is a pointer to the note size.  Updates NOTE_DATA and NOTE_SIZE.  */

static void
linux_make_mappings_corefile_notes (struct gdbarch *gdbarch, bfd *obfd,
				    gdb::unique_xmalloc_ptr<char> &note_data,
				    int *note_size)
{
  struct linux_make_mappings_data mapping_data;
  type_allocator alloc (gdbarch);
  struct type *long_type
    = init_integer_type (alloc, gdbarch_long_bit (gdbarch), 0, "long");
  gdb_byte buf[sizeof (ULONGEST)];

  auto_obstack data_obstack, filename_obstack;

  mapping_data.file_count = 0;
  mapping_data.data_obstack = &data_obstack;
  mapping_data.filename_obstack = &filename_obstack;
  mapping_data.long_type = long_type;

  /* Reserve space for the count.  */
  obstack_blank (&data_obstack, long_type->length ());
  /* We always write the page size as 1 since we have no good way to
     determine the correct value.  */
  pack_long (buf, long_type, 1);
  obstack_grow (&data_obstack, buf, long_type->length ());

  linux_find_memory_regions_full (gdbarch, 
				  dump_note_entry_p,
				  linux_make_mappings_callback,
				  &mapping_data);

  if (mapping_data.file_count != 0)
    {
      /* Write the count to the obstack.  */
      pack_long ((gdb_byte *) obstack_base (&data_obstack),
		 long_type, mapping_data.file_count);

      /* Copy the filenames to the data obstack.  */
      int size = obstack_object_size (&filename_obstack);
      obstack_grow (&data_obstack, obstack_base (&filename_obstack),
		    size);

      note_data.reset (elfcore_write_file_note (obfd, note_data.release (), note_size,
						obstack_base (&data_obstack),
						obstack_object_size (&data_obstack)));
    }
}

/* Fetch the siginfo data for the specified thread, if it exists.  If
   there is no data, or we could not read it, return an empty
   buffer.  */

static gdb::byte_vector
linux_get_siginfo_data (thread_info *thread, struct gdbarch *gdbarch)
{
  struct type *siginfo_type;
  LONGEST bytes_read;

  if (!gdbarch_get_siginfo_type_p (gdbarch))
    return gdb::byte_vector ();

  scoped_restore_current_thread save_current_thread;
  switch_to_thread (thread);

  siginfo_type = gdbarch_get_siginfo_type (gdbarch);

  gdb::byte_vector buf (siginfo_type->length ());

  bytes_read = target_read (current_inferior ()->top_target (),
			    TARGET_OBJECT_SIGNAL_INFO, NULL,
			    buf.data (), 0, siginfo_type->length ());
  if (bytes_read != siginfo_type->length ())
    buf.clear ();

  return buf;
}

/* Records the thread's register state for the corefile note
   section.  */

static void
linux_corefile_thread (struct thread_info *info,
		       struct gdbarch *gdbarch, bfd *obfd,
		       gdb::unique_xmalloc_ptr<char> &note_data,
		       int *note_size, gdb_signal stop_signal)
{
  gcore_elf_build_thread_register_notes (gdbarch, info, stop_signal, obfd,
					 &note_data, note_size);

  /* Don't return anything if we got no register information above,
     such a core file is useless.  */
  if (note_data != nullptr)
    {
      gdb::byte_vector siginfo_data
	= linux_get_siginfo_data (info, gdbarch);
      if (!siginfo_data.empty ())
	note_data.reset (elfcore_write_note (obfd, note_data.release (),
					     note_size, "CORE", NT_SIGINFO,
					     siginfo_data.data (),
					     siginfo_data.size ()));
    }
}

/* Fill the PRPSINFO structure with information about the process being
   debugged.  Returns 1 in case of success, 0 for failures.  Please note that
   even if the structure cannot be entirely filled (e.g., GDB was unable to
   gather information about the process UID/GID), this function will still
   return 1 since some information was already recorded.  It will only return
   0 iff nothing can be gathered.  */

static int
linux_fill_prpsinfo (struct elf_internal_linux_prpsinfo *p)
{
  /* The filename which we will use to obtain some info about the process.
     We will basically use this to store the `/proc/PID/FILENAME' file.  */
  char filename[100];
  /* The basename of the executable.  */
  const char *basename;
  /* Temporary buffer.  */
  char *tmpstr;
  /* The valid states of a process, according to the Linux kernel.  */
  const char valid_states[] = "RSDTZW";
  /* The program state.  */
  const char *prog_state;
  /* The state of the process.  */
  char pr_sname;
  /* The PID of the program which generated the corefile.  */
  pid_t pid;
  /* Process flags.  */
  unsigned int pr_flag;
  /* Process nice value.  */
  long pr_nice;
  /* The number of fields read by `sscanf'.  */
  int n_fields = 0;

  gdb_assert (p != NULL);

  /* Obtaining PID and filename.  */
  pid = inferior_ptid.pid ();
  xsnprintf (filename, sizeof (filename), "/proc/%d/cmdline", (int) pid);
  /* The full name of the program which generated the corefile.  */
  gdb_byte *buf = NULL;
  size_t buf_len = target_fileio_read_alloc (NULL, filename, &buf);
  gdb::unique_xmalloc_ptr<char> fname ((char *)buf);

  if (buf_len < 1 || fname.get ()[0] == '\0')
    {
      /* No program name was read, so we won't be able to retrieve more
	 information about the process.  */
      return 0;
    }
  if (fname.get ()[buf_len - 1] != '\0')
    {
      warning (_("target file %s "
		 "does not contain a trailing null character"),
	       filename);
      return 0;
    }

  memset (p, 0, sizeof (*p));

  /* Defining the PID.  */
  p->pr_pid = pid;

  /* Copying the program name.  Only the basename matters.  */
  basename = lbasename (fname.get ());
  strncpy (p->pr_fname, basename, sizeof (p->pr_fname) - 1);
  p->pr_fname[sizeof (p->pr_fname) - 1] = '\0';

  const std::string &infargs = current_inferior ()->args ();

  /* The arguments of the program.  */
  std::string psargs = fname.get ();
  if (!infargs.empty ())
    psargs += ' ' + infargs;

  strncpy (p->pr_psargs, psargs.c_str (), sizeof (p->pr_psargs) - 1);
  p->pr_psargs[sizeof (p->pr_psargs) - 1] = '\0';

  xsnprintf (filename, sizeof (filename), "/proc/%d/stat", (int) pid);
  /* The contents of `/proc/PID/stat'.  */
  gdb::unique_xmalloc_ptr<char> proc_stat_contents
    = target_fileio_read_stralloc (NULL, filename);
  char *proc_stat = proc_stat_contents.get ();

  if (proc_stat == NULL || *proc_stat == '\0')
    {
      /* Despite being unable to read more information about the
	 process, we return 1 here because at least we have its
	 command line, PID and arguments.  */
      return 1;
    }

  /* Ok, we have the stats.  It's time to do a little parsing of the
     contents of the buffer, so that we end up reading what we want.

     The following parsing mechanism is strongly based on the
     information generated by the `fs/proc/array.c' file, present in
     the Linux kernel tree.  More details about how the information is
     displayed can be obtained by seeing the manpage of proc(5),
     specifically under the entry of `/proc/[pid]/stat'.  */

  /* Getting rid of the PID, since we already have it.  */
  while (isdigit (*proc_stat))
    ++proc_stat;

  proc_stat = skip_spaces (proc_stat);

  /* ps command also relies on no trailing fields ever contain ')'.  */
  proc_stat = strrchr (proc_stat, ')');
  if (proc_stat == NULL)
    return 1;
  proc_stat++;

  proc_stat = skip_spaces (proc_stat);

  n_fields = sscanf (proc_stat,
		     "%c"		/* Process state.  */
		     "%d%d%d"		/* Parent PID, group ID, session ID.  */
		     "%*d%*d"		/* tty_nr, tpgid (not used).  */
		     "%u"		/* Flags.  */
		     "%*s%*s%*s%*s"	/* minflt, cminflt, majflt,
					   cmajflt (not used).  */
		     "%*s%*s%*s%*s"	/* utime, stime, cutime,
					   cstime (not used).  */
		     "%*s"		/* Priority (not used).  */
		     "%ld",		/* Nice.  */
		     &pr_sname,
		     &p->pr_ppid, &p->pr_pgrp, &p->pr_sid,
		     &pr_flag,
		     &pr_nice);

  if (n_fields != 6)
    {
      /* Again, we couldn't read the complementary information about
	 the process state.  However, we already have minimal
	 information, so we just return 1 here.  */
      return 1;
    }

  /* Filling the structure fields.  */
  prog_state = strchr (valid_states, pr_sname);
  if (prog_state != NULL)
    p->pr_state = prog_state - valid_states;
  else
    {
      /* Zero means "Running".  */
      p->pr_state = 0;
    }

  p->pr_sname = p->pr_state > 5 ? '.' : pr_sname;
  p->pr_zomb = p->pr_sname == 'Z';
  p->pr_nice = pr_nice;
  p->pr_flag = pr_flag;

  /* Finally, obtaining the UID and GID.  For that, we read and parse the
     contents of the `/proc/PID/status' file.  */
  xsnprintf (filename, sizeof (filename), "/proc/%d/status", (int) pid);
  /* The contents of `/proc/PID/status'.  */
  gdb::unique_xmalloc_ptr<char> proc_status_contents
    = target_fileio_read_stralloc (NULL, filename);
  char *proc_status = proc_status_contents.get ();

  if (proc_status == NULL || *proc_status == '\0')
    {
      /* Returning 1 since we already have a bunch of information.  */
      return 1;
    }

  /* Extracting the UID.  */
  tmpstr = strstr (proc_status, "Uid:");
  if (tmpstr != NULL)
    {
      /* Advancing the pointer to the beginning of the UID.  */
      tmpstr += sizeof ("Uid:");
      while (*tmpstr != '\0' && !isdigit (*tmpstr))
	++tmpstr;

      if (isdigit (*tmpstr))
	p->pr_uid = strtol (tmpstr, &tmpstr, 10);
    }

  /* Extracting the GID.  */
  tmpstr = strstr (proc_status, "Gid:");
  if (tmpstr != NULL)
    {
      /* Advancing the pointer to the beginning of the GID.  */
      tmpstr += sizeof ("Gid:");
      while (*tmpstr != '\0' && !isdigit (*tmpstr))
	++tmpstr;

      if (isdigit (*tmpstr))
	p->pr_gid = strtol (tmpstr, &tmpstr, 10);
    }

  return 1;
}

/* Build the note section for a corefile, and return it in a malloc
   buffer.  */

static gdb::unique_xmalloc_ptr<char>
linux_make_corefile_notes (struct gdbarch *gdbarch, bfd *obfd, int *note_size)
{
  struct elf_internal_linux_prpsinfo prpsinfo;
  gdb::unique_xmalloc_ptr<char> note_data;

  if (! gdbarch_iterate_over_regset_sections_p (gdbarch))
    return NULL;

  if (linux_fill_prpsinfo (&prpsinfo))
    {
      if (gdbarch_ptr_bit (gdbarch) == 64)
	note_data.reset (elfcore_write_linux_prpsinfo64 (obfd,
							 note_data.release (),
							 note_size, &prpsinfo));
      else
	note_data.reset (elfcore_write_linux_prpsinfo32 (obfd,
							 note_data.release (),
							 note_size, &prpsinfo));
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
     "First thread" is what tools use to infer the signalled
     thread.  */
  thread_info *signalled_thr = gcore_find_signalled_thread ();
  gdb_signal stop_signal;
  if (signalled_thr != nullptr)
    stop_signal = signalled_thr->stop_signal ();
  else
    stop_signal = GDB_SIGNAL_0;

  if (signalled_thr != nullptr)
    {
      /* On some architectures, like AArch64, each thread can have a distinct
	 gdbarch (due to scalable extensions), and using the inferior gdbarch
	 is incorrect.

	 Fetch each thread's gdbarch and pass it down to the lower layers so
	 we can dump the right set of registers.  */
      linux_corefile_thread (signalled_thr,
			     target_thread_architecture (signalled_thr->ptid),
			     obfd, note_data, note_size, stop_signal);
    }
  for (thread_info *thr : current_inferior ()->non_exited_threads ())
    {
      if (thr == signalled_thr)
	continue;

      /* On some architectures, like AArch64, each thread can have a distinct
	 gdbarch (due to scalable extensions), and using the inferior gdbarch
	 is incorrect.

	 Fetch each thread's gdbarch and pass it down to the lower layers so
	 we can dump the right set of registers.  */
      linux_corefile_thread (thr, target_thread_architecture (thr->ptid),
			     obfd, note_data, note_size, stop_signal);
    }

  if (!note_data)
    return NULL;

  /* Auxillary vector.  */
  std::optional<gdb::byte_vector> auxv =
    target_read_alloc (current_inferior ()->top_target (),
		       TARGET_OBJECT_AUXV, NULL);
  if (auxv && !auxv->empty ())
    {
      note_data.reset (elfcore_write_note (obfd, note_data.release (),
					   note_size, "CORE", NT_AUXV,
					   auxv->data (), auxv->size ()));

      if (!note_data)
	return NULL;
    }

  /* File mappings.  */
  linux_make_mappings_corefile_notes (gdbarch, obfd, note_data, note_size);

  /* Include the target description when possible.  Some architectures
     allow for per-thread gdbarch so we should really be emitting a tdesc
     per-thread, however, we don't currently support reading in a
     per-thread tdesc, so just emit the tdesc for the signalled thread.  */
  gdbarch = target_thread_architecture (signalled_thr->ptid);
  gcore_elf_make_tdesc_note (gdbarch, obfd, &note_data, note_size);

  return note_data;
}

/* Implementation of `gdbarch_gdb_signal_from_target', as defined in
   gdbarch.h.  This function is not static because it is exported to
   other -tdep files.  */

enum gdb_signal
linux_gdb_signal_from_target (struct gdbarch *gdbarch, int signal)
{
  switch (signal)
    {
    case 0:
      return GDB_SIGNAL_0;

    case LINUX_SIGHUP:
      return GDB_SIGNAL_HUP;

    case LINUX_SIGINT:
      return GDB_SIGNAL_INT;

    case LINUX_SIGQUIT:
      return GDB_SIGNAL_QUIT;

    case LINUX_SIGILL:
      return GDB_SIGNAL_ILL;

    case LINUX_SIGTRAP:
      return GDB_SIGNAL_TRAP;

    case LINUX_SIGABRT:
      return GDB_SIGNAL_ABRT;

    case LINUX_SIGBUS:
      return GDB_SIGNAL_BUS;

    case LINUX_SIGFPE:
      return GDB_SIGNAL_FPE;

    case LINUX_SIGKILL:
      return GDB_SIGNAL_KILL;

    case LINUX_SIGUSR1:
      return GDB_SIGNAL_USR1;

    case LINUX_SIGSEGV:
      return GDB_SIGNAL_SEGV;

    case LINUX_SIGUSR2:
      return GDB_SIGNAL_USR2;

    case LINUX_SIGPIPE:
      return GDB_SIGNAL_PIPE;

    case LINUX_SIGALRM:
      return GDB_SIGNAL_ALRM;

    case LINUX_SIGTERM:
      return GDB_SIGNAL_TERM;

    case LINUX_SIGCHLD:
      return GDB_SIGNAL_CHLD;

    case LINUX_SIGCONT:
      return GDB_SIGNAL_CONT;

    case LINUX_SIGSTOP:
      return GDB_SIGNAL_STOP;

    case LINUX_SIGTSTP:
      return GDB_SIGNAL_TSTP;

    case LINUX_SIGTTIN:
      return GDB_SIGNAL_TTIN;

    case LINUX_SIGTTOU:
      return GDB_SIGNAL_TTOU;

    case LINUX_SIGURG:
      return GDB_SIGNAL_URG;

    case LINUX_SIGXCPU:
      return GDB_SIGNAL_XCPU;

    case LINUX_SIGXFSZ:
      return GDB_SIGNAL_XFSZ;

    case LINUX_SIGVTALRM:
      return GDB_SIGNAL_VTALRM;

    case LINUX_SIGPROF:
      return GDB_SIGNAL_PROF;

    case LINUX_SIGWINCH:
      return GDB_SIGNAL_WINCH;

    /* No way to differentiate between SIGIO and SIGPOLL.
       Therefore, we just handle the first one.  */
    case LINUX_SIGIO:
      return GDB_SIGNAL_IO;

    case LINUX_SIGPWR:
      return GDB_SIGNAL_PWR;

    case LINUX_SIGSYS:
      return GDB_SIGNAL_SYS;

    /* SIGRTMIN and SIGRTMAX are not continuous in <gdb/signals.def>,
       therefore we have to handle them here.  */
    case LINUX_SIGRTMIN:
      return GDB_SIGNAL_REALTIME_32;

    case LINUX_SIGRTMAX:
      return GDB_SIGNAL_REALTIME_64;
    }

  if (signal >= LINUX_SIGRTMIN + 1 && signal <= LINUX_SIGRTMAX - 1)
    {
      int offset = signal - LINUX_SIGRTMIN + 1;

      return (enum gdb_signal) ((int) GDB_SIGNAL_REALTIME_33 + offset);
    }

  return GDB_SIGNAL_UNKNOWN;
}

/* Implementation of `gdbarch_gdb_signal_to_target', as defined in
   gdbarch.h.  This function is not static because it is exported to
   other -tdep files.  */

int
linux_gdb_signal_to_target (struct gdbarch *gdbarch,
			    enum gdb_signal signal)
{
  switch (signal)
    {
    case GDB_SIGNAL_0:
      return 0;

    case GDB_SIGNAL_HUP:
      return LINUX_SIGHUP;

    case GDB_SIGNAL_INT:
      return LINUX_SIGINT;

    case GDB_SIGNAL_QUIT:
      return LINUX_SIGQUIT;

    case GDB_SIGNAL_ILL:
      return LINUX_SIGILL;

    case GDB_SIGNAL_TRAP:
      return LINUX_SIGTRAP;

    case GDB_SIGNAL_ABRT:
      return LINUX_SIGABRT;

    case GDB_SIGNAL_FPE:
      return LINUX_SIGFPE;

    case GDB_SIGNAL_KILL:
      return LINUX_SIGKILL;

    case GDB_SIGNAL_BUS:
      return LINUX_SIGBUS;

    case GDB_SIGNAL_SEGV:
      return LINUX_SIGSEGV;

    case GDB_SIGNAL_SYS:
      return LINUX_SIGSYS;

    case GDB_SIGNAL_PIPE:
      return LINUX_SIGPIPE;

    case GDB_SIGNAL_ALRM:
      return LINUX_SIGALRM;

    case GDB_SIGNAL_TERM:
      return LINUX_SIGTERM;

    case GDB_SIGNAL_URG:
      return LINUX_SIGURG;

    case GDB_SIGNAL_STOP:
      return LINUX_SIGSTOP;

    case GDB_SIGNAL_TSTP:
      return LINUX_SIGTSTP;

    case GDB_SIGNAL_CONT:
      return LINUX_SIGCONT;

    case GDB_SIGNAL_CHLD:
      return LINUX_SIGCHLD;

    case GDB_SIGNAL_TTIN:
      return LINUX_SIGTTIN;

    case GDB_SIGNAL_TTOU:
      return LINUX_SIGTTOU;

    case GDB_SIGNAL_IO:
      return LINUX_SIGIO;

    case GDB_SIGNAL_XCPU:
      return LINUX_SIGXCPU;

    case GDB_SIGNAL_XFSZ:
      return LINUX_SIGXFSZ;

    case GDB_SIGNAL_VTALRM:
      return LINUX_SIGVTALRM;

    case GDB_SIGNAL_PROF:
      return LINUX_SIGPROF;

    case GDB_SIGNAL_WINCH:
      return LINUX_SIGWINCH;

    case GDB_SIGNAL_USR1:
      return LINUX_SIGUSR1;

    case GDB_SIGNAL_USR2:
      return LINUX_SIGUSR2;

    case GDB_SIGNAL_PWR:
      return LINUX_SIGPWR;

    case GDB_SIGNAL_POLL:
      return LINUX_SIGPOLL;

    /* GDB_SIGNAL_REALTIME_32 is not continuous in <gdb/signals.def>,
       therefore we have to handle it here.  */
    case GDB_SIGNAL_REALTIME_32:
      return LINUX_SIGRTMIN;

    /* Same comment applies to _64.  */
    case GDB_SIGNAL_REALTIME_64:
      return LINUX_SIGRTMAX;
    }

  /* GDB_SIGNAL_REALTIME_33 to _64 are continuous.  */
  if (signal >= GDB_SIGNAL_REALTIME_33
      && signal <= GDB_SIGNAL_REALTIME_63)
    {
      int offset = signal - GDB_SIGNAL_REALTIME_33;

      return LINUX_SIGRTMIN + 1 + offset;
    }

  return -1;
}

/* Helper for linux_vsyscall_range that does the real work of finding
   the vsyscall's address range.  */

static int
linux_vsyscall_range_raw (struct gdbarch *gdbarch, struct mem_range *range)
{
  char filename[100];
  long pid;

  if (target_auxv_search (AT_SYSINFO_EHDR, &range->start) <= 0)
    return 0;

  /* It doesn't make sense to access the host's /proc when debugging a
     core file.  Instead, look for the PT_LOAD segment that matches
     the vDSO.  */
  if (!target_has_execution ())
    {
      long phdrs_size;
      int num_phdrs, i;

      phdrs_size = bfd_get_elf_phdr_upper_bound (core_bfd);
      if (phdrs_size == -1)
	return 0;

      gdb::unique_xmalloc_ptr<Elf_Internal_Phdr>
	phdrs ((Elf_Internal_Phdr *) xmalloc (phdrs_size));
      num_phdrs = bfd_get_elf_phdrs (core_bfd, phdrs.get ());
      if (num_phdrs == -1)
	return 0;

      for (i = 0; i < num_phdrs; i++)
	if (phdrs.get ()[i].p_type == PT_LOAD
	    && phdrs.get ()[i].p_vaddr == range->start)
	  {
	    range->length = phdrs.get ()[i].p_memsz;
	    return 1;
	  }

      return 0;
    }

  /* We need to know the real target PID to access /proc.  */
  if (current_inferior ()->fake_pid_p)
    return 0;

  pid = current_inferior ()->pid;

  /* Note that reading /proc/PID/task/PID/maps (1) is much faster than
     reading /proc/PID/maps (2).  The later identifies thread stacks
     in the output, which requires scanning every thread in the thread
     group to check whether a VMA is actually a thread's stack.  With
     Linux 4.4 on an Intel i7-4810MQ @ 2.80GHz, with an inferior with
     a few thousand threads, (1) takes a few miliseconds, while (2)
     takes several seconds.  Also note that "smaps", what we read for
     determining core dump mappings, is even slower than "maps".  */
  xsnprintf (filename, sizeof filename, "/proc/%ld/task/%ld/maps", pid, pid);
  gdb::unique_xmalloc_ptr<char> data
    = target_fileio_read_stralloc (NULL, filename);
  if (data != NULL)
    {
      char *line;
      char *saveptr = NULL;

      for (line = strtok_r (data.get (), "\n", &saveptr);
	   line != NULL;
	   line = strtok_r (NULL, "\n", &saveptr))
	{
	  ULONGEST addr, endaddr;
	  const char *p = line;

	  addr = strtoulst (p, &p, 16);
	  if (addr == range->start)
	    {
	      if (*p == '-')
		p++;
	      endaddr = strtoulst (p, &p, 16);
	      range->length = endaddr - addr;
	      return 1;
	    }
	}
    }
  else
    warning (_("unable to open /proc file '%s'"), filename);

  return 0;
}

/* Implementation of the "vsyscall_range" gdbarch hook.  Handles
   caching, and defers the real work to linux_vsyscall_range_raw.  */

static int
linux_vsyscall_range (struct gdbarch *gdbarch, struct mem_range *range)
{
  struct linux_info *info = get_linux_inferior_data (current_inferior ());

  if (info->vsyscall_range_p == 0)
    {
      if (linux_vsyscall_range_raw (gdbarch, &info->vsyscall_range))
	info->vsyscall_range_p = 1;
      else
	info->vsyscall_range_p = -1;
    }

  if (info->vsyscall_range_p < 0)
    return 0;

  *range = info->vsyscall_range;
  return 1;
}

/* Symbols for linux_infcall_mmap's ARG_FLAGS; their Linux MAP_* system
   definitions would be dependent on compilation host.  */
#define GDB_MMAP_MAP_PRIVATE	0x02		/* Changes are private.  */
#define GDB_MMAP_MAP_ANONYMOUS	0x20		/* Don't use a file.  */

/* See gdbarch.sh 'infcall_mmap'.  */

static CORE_ADDR
linux_infcall_mmap (CORE_ADDR size, unsigned prot)
{
  struct objfile *objf;
  /* Do there still exist any Linux systems without "mmap64"?
     "mmap" uses 64-bit off_t on x86_64 and 32-bit off_t on i386 and x32.  */
  struct value *mmap_val = find_function_in_inferior ("mmap64", &objf);
  struct value *addr_val;
  struct gdbarch *gdbarch = objf->arch ();
  CORE_ADDR retval;
  enum
    {
      ARG_ADDR, ARG_LENGTH, ARG_PROT, ARG_FLAGS, ARG_FD, ARG_OFFSET, ARG_LAST
    };
  struct value *arg[ARG_LAST];

  arg[ARG_ADDR] = value_from_pointer (builtin_type (gdbarch)->builtin_data_ptr,
				      0);
  /* Assuming sizeof (unsigned long) == sizeof (size_t).  */
  arg[ARG_LENGTH] = value_from_ulongest
		    (builtin_type (gdbarch)->builtin_unsigned_long, size);
  gdb_assert ((prot & ~(GDB_MMAP_PROT_READ | GDB_MMAP_PROT_WRITE
			| GDB_MMAP_PROT_EXEC))
	      == 0);
  arg[ARG_PROT] = value_from_longest (builtin_type (gdbarch)->builtin_int, prot);
  arg[ARG_FLAGS] = value_from_longest (builtin_type (gdbarch)->builtin_int,
				       GDB_MMAP_MAP_PRIVATE
				       | GDB_MMAP_MAP_ANONYMOUS);
  arg[ARG_FD] = value_from_longest (builtin_type (gdbarch)->builtin_int, -1);
  arg[ARG_OFFSET] = value_from_longest (builtin_type (gdbarch)->builtin_int64,
					0);
  addr_val = call_function_by_hand (mmap_val, NULL, arg);
  retval = value_as_address (addr_val);
  if (retval == (CORE_ADDR) -1)
    error (_("Failed inferior mmap call for %s bytes, errno is changed."),
	   pulongest (size));
  return retval;
}

/* See gdbarch.sh 'infcall_munmap'.  */

static void
linux_infcall_munmap (CORE_ADDR addr, CORE_ADDR size)
{
  struct objfile *objf;
  struct value *munmap_val = find_function_in_inferior ("munmap", &objf);
  struct value *retval_val;
  struct gdbarch *gdbarch = objf->arch ();
  LONGEST retval;
  enum
    {
      ARG_ADDR, ARG_LENGTH, ARG_LAST
    };
  struct value *arg[ARG_LAST];

  arg[ARG_ADDR] = value_from_pointer (builtin_type (gdbarch)->builtin_data_ptr,
				      addr);
  /* Assuming sizeof (unsigned long) == sizeof (size_t).  */
  arg[ARG_LENGTH] = value_from_ulongest
		    (builtin_type (gdbarch)->builtin_unsigned_long, size);
  retval_val = call_function_by_hand (munmap_val, NULL, arg);
  retval = value_as_long (retval_val);
  if (retval != 0)
    warning (_("Failed inferior munmap call at %s for %s bytes, "
	       "errno is changed."),
	     hex_string (addr), pulongest (size));
}

/* See linux-tdep.h.  */

CORE_ADDR
linux_displaced_step_location (struct gdbarch *gdbarch)
{
  CORE_ADDR addr;
  int bp_len;

  /* Determine entry point from target auxiliary vector.  This avoids
     the need for symbols.  Also, when debugging a stand-alone SPU
     executable, entry_point_address () will point to an SPU
     local-store address and is thus not usable as displaced stepping
     location.  The auxiliary vector gets us the PowerPC-side entry
     point address instead.  */
  if (target_auxv_search (AT_ENTRY, &addr) <= 0)
    throw_error (NOT_SUPPORTED_ERROR,
		 _("Cannot find AT_ENTRY auxiliary vector entry."));

  /* Make certain that the address points at real code, and not a
     function descriptor.  */
  addr = gdbarch_convert_from_func_ptr_addr
    (gdbarch, addr, current_inferior ()->top_target ());

  /* Inferior calls also use the entry point as a breakpoint location.
     We don't want displaced stepping to interfere with those
     breakpoints, so leave space.  */
  gdbarch_breakpoint_from_pc (gdbarch, &addr, &bp_len);
  addr += bp_len * 2;

  return addr;
}

/* See linux-tdep.h.  */

displaced_step_prepare_status
linux_displaced_step_prepare (gdbarch *arch, thread_info *thread,
			      CORE_ADDR &displaced_pc)
{
  linux_info *per_inferior = get_linux_inferior_data (thread->inf);

  if (!per_inferior->disp_step_bufs.has_value ())
    {
      /* Figure out the location of the buffers.  They are contiguous, starting
	 at DISP_STEP_BUF_ADDR.  They are all of size BUF_LEN.  */
      CORE_ADDR disp_step_buf_addr
	= linux_displaced_step_location (thread->inf->arch ());
      int buf_len = gdbarch_displaced_step_buffer_length (arch);

      linux_gdbarch_data *gdbarch_data = get_linux_gdbarch_data (arch);
      gdb_assert (gdbarch_data->num_disp_step_buffers > 0);

      std::vector<CORE_ADDR> buffers;
      for (int i = 0; i < gdbarch_data->num_disp_step_buffers; i++)
	buffers.push_back (disp_step_buf_addr + i * buf_len);

      per_inferior->disp_step_bufs.emplace (buffers);
    }

  return per_inferior->disp_step_bufs->prepare (thread, displaced_pc);
}

/* See linux-tdep.h.  */

displaced_step_finish_status
linux_displaced_step_finish (gdbarch *arch, thread_info *thread,
			     const target_waitstatus &status)
{
  linux_info *per_inferior = get_linux_inferior_data (thread->inf);

  gdb_assert (per_inferior->disp_step_bufs.has_value ());

  return per_inferior->disp_step_bufs->finish (arch, thread, status);
}

/* See linux-tdep.h.  */

const displaced_step_copy_insn_closure *
linux_displaced_step_copy_insn_closure_by_addr (inferior *inf, CORE_ADDR addr)
{
  linux_info *per_inferior = linux_inferior_data.get (inf);

  if (per_inferior == nullptr
      || !per_inferior->disp_step_bufs.has_value ())
    return nullptr;

  return per_inferior->disp_step_bufs->copy_insn_closure_by_addr (addr);
}

/* See linux-tdep.h.  */

void
linux_displaced_step_restore_all_in_ptid (inferior *parent_inf, ptid_t ptid)
{
  linux_info *per_inferior = linux_inferior_data.get (parent_inf);

  if (per_inferior == nullptr
      || !per_inferior->disp_step_bufs.has_value ())
    return;

  per_inferior->disp_step_bufs->restore_in_ptid (ptid);
}

/* Helper for linux_get_hwcap and linux_get_hwcap2.  */

static CORE_ADDR
linux_get_hwcap_helper (const std::optional<gdb::byte_vector> &auxv,
			target_ops *target, gdbarch *gdbarch, CORE_ADDR match)
{
  CORE_ADDR field;
  if (!auxv.has_value ()
      || target_auxv_search (*auxv, target, gdbarch, match, &field) != 1)
    return 0;
  return field;
}

/* See linux-tdep.h.  */

CORE_ADDR
linux_get_hwcap (const std::optional<gdb::byte_vector> &auxv,
		 target_ops *target, gdbarch *gdbarch)
{
  return linux_get_hwcap_helper (auxv, target, gdbarch, AT_HWCAP);
}

/* See linux-tdep.h.  */

CORE_ADDR
linux_get_hwcap ()
{
  return linux_get_hwcap (target_read_auxv (),
			  current_inferior ()->top_target (),
			  current_inferior ()->arch ());
}

/* See linux-tdep.h.  */

CORE_ADDR
linux_get_hwcap2 (const std::optional<gdb::byte_vector> &auxv,
		  target_ops *target, gdbarch *gdbarch)
{
  return linux_get_hwcap_helper (auxv, target, gdbarch, AT_HWCAP2);
}

/* See linux-tdep.h.  */

CORE_ADDR
linux_get_hwcap2 ()
{
  return linux_get_hwcap2 (target_read_auxv (),
			   current_inferior ()->top_target (),
			   current_inferior ()->arch ());
}

/* Display whether the gcore command is using the
   /proc/PID/coredump_filter file.  */

static void
show_use_coredump_filter (struct ui_file *file, int from_tty,
			  struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Use of /proc/PID/coredump_filter file to generate"
		      " corefiles is %s.\n"), value);
}

/* Display whether the gcore command is dumping mappings marked with
   the VM_DONTDUMP flag.  */

static void
show_dump_excluded_mappings (struct ui_file *file, int from_tty,
			     struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Dumping of mappings marked with the VM_DONTDUMP"
		      " flag is %s.\n"), value);
}

/* To be called from the various GDB_OSABI_LINUX handlers for the
   various GNU/Linux architectures and machine types.

   NUM_DISP_STEP_BUFFERS is the number of displaced step buffers to use.  If 0,
   displaced stepping is not supported. */

void
linux_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch,
		int num_disp_step_buffers)
{
  if (num_disp_step_buffers > 0)
    {
      linux_gdbarch_data *gdbarch_data = get_linux_gdbarch_data (gdbarch);
      gdbarch_data->num_disp_step_buffers = num_disp_step_buffers;

      set_gdbarch_displaced_step_prepare (gdbarch,
					  linux_displaced_step_prepare);
      set_gdbarch_displaced_step_finish (gdbarch, linux_displaced_step_finish);
      set_gdbarch_displaced_step_copy_insn_closure_by_addr
	(gdbarch, linux_displaced_step_copy_insn_closure_by_addr);
      set_gdbarch_displaced_step_restore_all_in_ptid
	(gdbarch, linux_displaced_step_restore_all_in_ptid);
    }

  set_gdbarch_core_pid_to_str (gdbarch, linux_core_pid_to_str);
  set_gdbarch_info_proc (gdbarch, linux_info_proc);
  set_gdbarch_core_info_proc (gdbarch, linux_core_info_proc);
  set_gdbarch_core_xfer_siginfo (gdbarch, linux_core_xfer_siginfo);
  set_gdbarch_read_core_file_mappings (gdbarch, linux_read_core_file_mappings);
  set_gdbarch_find_memory_regions (gdbarch, linux_find_memory_regions);
  set_gdbarch_make_corefile_notes (gdbarch, linux_make_corefile_notes);
  set_gdbarch_has_shared_address_space (gdbarch,
					linux_has_shared_address_space);
  set_gdbarch_gdb_signal_from_target (gdbarch,
				      linux_gdb_signal_from_target);
  set_gdbarch_gdb_signal_to_target (gdbarch,
				    linux_gdb_signal_to_target);
  set_gdbarch_vsyscall_range (gdbarch, linux_vsyscall_range);
  set_gdbarch_infcall_mmap (gdbarch, linux_infcall_mmap);
  set_gdbarch_infcall_munmap (gdbarch, linux_infcall_munmap);
  set_gdbarch_get_siginfo_type (gdbarch, linux_get_siginfo_type);
}

void _initialize_linux_tdep ();
void
_initialize_linux_tdep ()
{
  /* Observers used to invalidate the cache when needed.  */
  gdb::observers::inferior_exit.attach (invalidate_linux_cache_inf,
					"linux-tdep");
  gdb::observers::inferior_appeared.attach (invalidate_linux_cache_inf,
					    "linux-tdep");
  gdb::observers::inferior_execd.attach (linux_inferior_execd,
					 "linux-tdep");

  add_setshow_boolean_cmd ("use-coredump-filter", class_files,
			   &use_coredump_filter, _("\
Set whether gcore should consider /proc/PID/coredump_filter."),
			   _("\
Show whether gcore should consider /proc/PID/coredump_filter."),
			   _("\
Use this command to set whether gcore should consider the contents\n\
of /proc/PID/coredump_filter when generating the corefile.  For more information\n\
about this file, refer to the manpage of core(5)."),
			   NULL, show_use_coredump_filter,
			   &setlist, &showlist);

  add_setshow_boolean_cmd ("dump-excluded-mappings", class_files,
			   &dump_excluded_mappings, _("\
Set whether gcore should dump mappings marked with the VM_DONTDUMP flag."),
			   _("\
Show whether gcore should dump mappings marked with the VM_DONTDUMP flag."),
			   _("\
Use this command to set whether gcore should dump mappings marked with the\n\
VM_DONTDUMP flag (\"dd\" in /proc/PID/smaps) when generating the corefile.  For\n\
more information about this file, refer to the manpage of proc(5) and core(5)."),
			   NULL, show_dump_excluded_mappings,
			   &setlist, &showlist);
}

/* Fetch (and possibly build) an appropriate `link_map_offsets' for
   ILP32/LP64 Linux systems which don't have the r_ldsomap field.  */

link_map_offsets *
linux_ilp32_fetch_link_map_offsets ()
{
  static link_map_offsets lmo;
  static link_map_offsets *lmp = nullptr;

  if (lmp == nullptr)
    {
      lmp = &lmo;

      lmo.r_version_offset = 0;
      lmo.r_version_size = 4;
      lmo.r_map_offset = 4;
      lmo.r_brk_offset = 8;
      lmo.r_ldsomap_offset = -1;
      lmo.r_next_offset = 20;

      /* Everything we need is in the first 20 bytes.  */
      lmo.link_map_size = 20;
      lmo.l_addr_offset = 0;
      lmo.l_name_offset = 4;
      lmo.l_ld_offset = 8;
      lmo.l_next_offset = 12;
      lmo.l_prev_offset = 16;
    }

  return lmp;
}

link_map_offsets *
linux_lp64_fetch_link_map_offsets ()
{
  static link_map_offsets lmo;
  static link_map_offsets *lmp = nullptr;

  if (lmp == nullptr)
    {
      lmp = &lmo;

      lmo.r_version_offset = 0;
      lmo.r_version_size = 4;
      lmo.r_map_offset = 8;
      lmo.r_brk_offset = 16;
      lmo.r_ldsomap_offset = -1;
      lmo.r_next_offset = 40;

      /* Everything we need is in the first 40 bytes.  */
      lmo.link_map_size = 40;
      lmo.l_addr_offset = 0;
      lmo.l_name_offset = 8;
      lmo.l_ld_offset = 16;
      lmo.l_next_offset = 24;
      lmo.l_prev_offset = 32;
    }

  return lmp;
}
