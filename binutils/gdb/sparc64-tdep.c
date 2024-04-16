/* Target-dependent code for UltraSPARC.

   Copyright (C) 2003-2024 Free Software Foundation, Inc.

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
#include "arch-utils.h"
#include "dwarf2/frame.h"
#include "frame.h"
#include "frame-base.h"
#include "frame-unwind.h"
#include "gdbcore.h"
#include "gdbtypes.h"
#include "inferior.h"
#include "symtab.h"
#include "objfiles.h"
#include "osabi.h"
#include "regcache.h"
#include "target-descriptions.h"
#include "target.h"
#include "value.h"
#include "sparc64-tdep.h"
#include <forward_list>

/* This file implements the SPARC 64-bit ABI as defined by the
   section "Low-Level System Information" of the SPARC Compliance
   Definition (SCD) 2.4.1, which is the 64-bit System V psABI for
   SPARC.  */

/* Please use the sparc32_-prefix for 32-bit specific code, the
   sparc64_-prefix for 64-bit specific code and the sparc_-prefix for
   code can handle both.  */

/* The M7 processor supports an Application Data Integrity (ADI) feature
   that detects invalid data accesses.  When software allocates memory and 
   enables ADI on the allocated memory, it chooses a 4-bit version number, 
   sets the version in the upper 4 bits of the 64-bit pointer to that data, 
   and stores the 4-bit version in every cacheline of the object.  Hardware 
   saves the latter in spare bits in the cache and memory hierarchy. On each 
   load and store, the processor compares the upper 4 VA (virtual address) bits 
   to the cacheline's version. If there is a mismatch, the processor generates
   a version mismatch trap which can be either precise or disrupting.
   The trap is an error condition which the kernel delivers to the process
   as a SIGSEGV signal.

   The upper 4 bits of the VA represent a version and are not part of the
   true address.  The processor clears these bits and sign extends bit 59
   to generate the true address.

   Note that 32-bit applications cannot use ADI. */


#include <algorithm>
#include "cli/cli-utils.h"
#include "gdbcmd.h"
#include "auxv.h"

#define MAX_PROC_NAME_SIZE sizeof("/proc/99999/lwp/9999/adi/lstatus")

/* ELF Auxiliary vectors */
#ifndef AT_ADI_BLKSZ
#define AT_ADI_BLKSZ    34
#endif
#ifndef AT_ADI_NBITS
#define AT_ADI_NBITS    35
#endif
#ifndef AT_ADI_UEONADI
#define AT_ADI_UEONADI  36
#endif

/* ADI command list.  */
static struct cmd_list_element *sparc64adilist = NULL;

/* ADI stat settings.  */
struct adi_stat_t
{
  /* The ADI block size.  */
  unsigned long blksize;

  /* Number of bits used for an ADI version tag which can be
     used together with the shift value for an ADI version tag
     to encode or extract the ADI version value in a pointer.  */
  unsigned long nbits;

  /* The maximum ADI version tag value supported.  */
  int max_version;

  /* ADI version tag file.  */
  int tag_fd = 0;

  /* ADI availability check has been done.  */
  bool checked_avail = false;

  /* ADI is available.  */
  bool is_avail = false;

};

/* Per-process ADI stat info.  */

struct sparc64_adi_info
{
  sparc64_adi_info (pid_t pid_)
    : pid (pid_)
  {}

  /* The process identifier.  */
  pid_t pid;

  /* The ADI stat.  */
  adi_stat_t stat = {};

};

static std::forward_list<sparc64_adi_info> adi_proc_list;


/* Get ADI info for process PID, creating one if it doesn't exist.  */

static sparc64_adi_info * 
get_adi_info_proc (pid_t pid)
{
  auto found = std::find_if (adi_proc_list.begin (), adi_proc_list.end (),
			     [&pid] (const sparc64_adi_info &info)
			     {
			       return info.pid == pid;
			     });

  if (found == adi_proc_list.end ())
    {
      adi_proc_list.emplace_front (pid);
      return &adi_proc_list.front ();
    }
  else
    {
      return &(*found);
    }
}

static adi_stat_t 
get_adi_info (pid_t pid)
{
  sparc64_adi_info *proc;

  proc = get_adi_info_proc (pid);
  return proc->stat;
}

/* Is called when GDB is no longer debugging process PID.  It
   deletes data structure that keeps track of the ADI stat.  */

void
sparc64_forget_process (pid_t pid)
{
  fileio_error target_errno;

  for (auto pit = adi_proc_list.before_begin (),
	 it = std::next (pit);
       it != adi_proc_list.end ();
       )
    {
      if ((*it).pid == pid)
	{
	  if ((*it).stat.tag_fd > 0) 
	    target_fileio_close ((*it).stat.tag_fd, &target_errno);
	  adi_proc_list.erase_after (pit);
	  break;
	}
      else
	pit = it++;
    }

}

/* Read attributes of a maps entry in /proc/[pid]/adi/maps.  */

static void
read_maps_entry (const char *line,
	      ULONGEST *addr, ULONGEST *endaddr)
{
  const char *p = line;

  *addr = strtoulst (p, &p, 16);
  if (*p == '-')
    p++;

  *endaddr = strtoulst (p, &p, 16);
}

/* Check if ADI is available.  */

static bool
adi_available (void)
{
  pid_t pid = inferior_ptid.pid ();
  sparc64_adi_info *proc = get_adi_info_proc (pid);
  CORE_ADDR value;

  if (proc->stat.checked_avail)
    return proc->stat.is_avail;

  proc->stat.checked_avail = true;
  if (target_auxv_search (AT_ADI_BLKSZ, &value) <= 0)
    return false;
  proc->stat.blksize = value;
  target_auxv_search (AT_ADI_NBITS, &value);
  proc->stat.nbits = value;
  proc->stat.max_version = (1 << proc->stat.nbits) - 2;
  proc->stat.is_avail = true;

  return proc->stat.is_avail;
}

/* Normalize a versioned address - a VA with ADI bits (63-60) set.  */

static CORE_ADDR
adi_normalize_address (CORE_ADDR addr)
{
  adi_stat_t ast = get_adi_info (inferior_ptid.pid ());

  if (ast.nbits)
    {
      /* Clear upper bits.  */
      addr &= ((uint64_t) -1) >> ast.nbits;

      /* Sign extend.  */
      CORE_ADDR signbit = (uint64_t) 1 << (64 - ast.nbits - 1);
      return (addr ^ signbit) - signbit;
    }
  return addr;
}

/* Align a normalized address - a VA with bit 59 sign extended into 
   ADI bits.  */

static CORE_ADDR
adi_align_address (CORE_ADDR naddr)
{
  adi_stat_t ast = get_adi_info (inferior_ptid.pid ());

  return (naddr - (naddr % ast.blksize)) / ast.blksize;
}

/* Convert a byte count to count at a ratio of 1:adi_blksz.  */

static int
adi_convert_byte_count (CORE_ADDR naddr, int nbytes, CORE_ADDR locl)
{
  adi_stat_t ast = get_adi_info (inferior_ptid.pid ());

  return ((naddr + nbytes + ast.blksize - 1) / ast.blksize) - locl;
}

/* The /proc/[pid]/adi/tags file, which allows gdb to get/set ADI
   version in a target process, maps linearly to the address space
   of the target process at a ratio of 1:adi_blksz.

   A read (or write) at offset K in the file returns (or modifies)
   the ADI version tag stored in the cacheline containing address
   K * adi_blksz, encoded as 1 version tag per byte.  The allowed
   version tag values are between 0 and adi_stat.max_version.  */

static int
adi_tag_fd (void)
{
  pid_t pid = inferior_ptid.pid ();
  sparc64_adi_info *proc = get_adi_info_proc (pid);

  if (proc->stat.tag_fd != 0)
    return proc->stat.tag_fd;

  char cl_name[MAX_PROC_NAME_SIZE];
  snprintf (cl_name, sizeof(cl_name), "/proc/%ld/adi/tags", (long) pid);
  fileio_error target_errno;
  proc->stat.tag_fd = target_fileio_open (NULL, cl_name, O_RDWR|O_EXCL, 
					  false, 0, &target_errno);
  return proc->stat.tag_fd;
}

/* Check if an address set is ADI enabled, using /proc/[pid]/adi/maps
   which was exported by the kernel and contains the currently ADI
   mapped memory regions and their access permissions.  */

static bool
adi_is_addr_mapped (CORE_ADDR vaddr, size_t cnt)
{
  char filename[MAX_PROC_NAME_SIZE];
  size_t i = 0;

  pid_t pid = inferior_ptid.pid ();
  snprintf (filename, sizeof filename, "/proc/%ld/adi/maps", (long) pid);
  gdb::unique_xmalloc_ptr<char> data
    = target_fileio_read_stralloc (NULL, filename);
  if (data)
    {
      adi_stat_t adi_stat = get_adi_info (pid);
      char *saveptr;
      for (char *line = strtok_r (data.get (), "\n", &saveptr);
	   line;
	   line = strtok_r (NULL, "\n", &saveptr))
	{
	  ULONGEST addr, endaddr;

	  read_maps_entry (line, &addr, &endaddr);

	  while (((vaddr + i) * adi_stat.blksize) >= addr
		 && ((vaddr + i) * adi_stat.blksize) < endaddr)
	    {
	      if (++i == cnt)
		return true;
	    }
	}
      }
  else
    warning (_("unable to open /proc file '%s'"), filename);

  return false;
}

/* Read ADI version tag value for memory locations starting at "VADDR"
   for "SIZE" number of bytes.  */

static int
adi_read_versions (CORE_ADDR vaddr, size_t size, gdb_byte *tags)
{
  int fd = adi_tag_fd ();
  if (fd == -1)
    return -1;

  if (!adi_is_addr_mapped (vaddr, size))
    {
      adi_stat_t ast = get_adi_info (inferior_ptid.pid ());
      error(_("Address at %s is not in ADI maps"),
	    paddress (current_inferior ()->arch (), vaddr * ast.blksize));
    }

  fileio_error target_errno;
  return target_fileio_pread (fd, tags, size, vaddr, &target_errno);
}

/* Write ADI version tag for memory locations starting at "VADDR" for
 "SIZE" number of bytes to "TAGS".  */

static int
adi_write_versions (CORE_ADDR vaddr, size_t size, unsigned char *tags)
{
  int fd = adi_tag_fd ();
  if (fd == -1)
    return -1;

  if (!adi_is_addr_mapped (vaddr, size))
    {
      adi_stat_t ast = get_adi_info (inferior_ptid.pid ());
      error(_("Address at %s is not in ADI maps"),
	    paddress (current_inferior ()->arch (), vaddr * ast.blksize));
    }

  fileio_error target_errno;
  return target_fileio_pwrite (fd, tags, size, vaddr, &target_errno);
}

/* Print ADI version tag value in "TAGS" for memory locations starting
   at "VADDR" with number of "CNT".  */

static void
adi_print_versions (CORE_ADDR vaddr, size_t cnt, gdb_byte *tags)
{
  int v_idx = 0;
  const int maxelts = 8;  /* # of elements per line */

  adi_stat_t adi_stat = get_adi_info (inferior_ptid.pid ());

  while (cnt > 0)
    {
      QUIT;
      gdb_printf ("%s:\t",
		  paddress (current_inferior ()->arch (),
			    vaddr * adi_stat.blksize));
      for (int i = maxelts; i > 0 && cnt > 0; i--, cnt--)
	{
	  if (tags[v_idx] == 0xff)    /* no version tag */
	    gdb_printf ("-");
	  else
	    gdb_printf ("%1X", tags[v_idx]);
	  if (cnt > 1)
	    gdb_printf (" ");
	  ++v_idx;
	}
      gdb_printf ("\n");
      vaddr += maxelts;
    }
}

static void
do_examine (CORE_ADDR start, int bcnt)
{
  CORE_ADDR vaddr = adi_normalize_address (start);

  CORE_ADDR vstart = adi_align_address (vaddr);
  int cnt = adi_convert_byte_count (vaddr, bcnt, vstart);
  gdb::byte_vector buf (cnt);
  int read_cnt = adi_read_versions (vstart, cnt, buf.data ());
  if (read_cnt == -1)
    error (_("No ADI information"));
  else if (read_cnt < cnt)
    error(_("No ADI information at %s"),
	  paddress (current_inferior ()->arch (), vaddr));

  adi_print_versions (vstart, cnt, buf.data ());
}

static void
do_assign (CORE_ADDR start, size_t bcnt, int version)
{
  CORE_ADDR vaddr = adi_normalize_address (start);

  CORE_ADDR vstart = adi_align_address (vaddr);
  int cnt = adi_convert_byte_count (vaddr, bcnt, vstart);
  std::vector<unsigned char> buf (cnt, version);
  int set_cnt = adi_write_versions (vstart, cnt, buf.data ());

  if (set_cnt == -1)
    error (_("No ADI information"));
  else if (set_cnt < cnt)
    error(_("No ADI information at %s"),
	  paddress (current_inferior ()->arch (), vaddr));
}

/* ADI examine version tag command.

   Command syntax:

     adi (examine|x)[/COUNT] [ADDR] */

static void
adi_examine_command (const char *args, int from_tty)
{
  /* make sure program is active and adi is available */
  if (!target_has_execution ())
    error (_("ADI command requires a live process/thread"));

  if (!adi_available ())
    error (_("No ADI information"));

  int cnt = 1;
  const char *p = args;
  if (p && *p == '/')
    {
      p++;
      cnt = get_number (&p);
    }

  CORE_ADDR next_address = 0;
  if (p != 0 && *p != 0)
    next_address = parse_and_eval_address (p);
  if (!cnt || !next_address)
    error (_("Usage: adi examine|x[/COUNT] [ADDR]"));

  do_examine (next_address, cnt);
}

/* ADI assign version tag command.

   Command syntax:

     adi (assign|a)[/COUNT] ADDR = VERSION  */

static void
adi_assign_command (const char *args, int from_tty)
{
  static const char *adi_usage
    = N_("Usage: adi assign|a[/COUNT] ADDR = VERSION");

  /* make sure program is active and adi is available */
  if (!target_has_execution ())
    error (_("ADI command requires a live process/thread"));

  if (!adi_available ())
    error (_("No ADI information"));

  const char *exp = args;
  if (exp == 0)
    error_no_arg (_(adi_usage));

  char *q = (char *) strchr (exp, '=');
  if (q)
    *q++ = 0;
  else
    error ("%s", _(adi_usage));

  size_t cnt = 1;
  const char *p = args;
  if (exp && *exp == '/')
    {
      p = exp + 1;
      cnt = get_number (&p);
    }

  CORE_ADDR next_address = 0;
  if (p != 0 && *p != 0)
    next_address = parse_and_eval_address (p);
  else
    error ("%s", _(adi_usage));

  int version = 0;
  if (q != NULL)           /* parse version tag */
    {
      adi_stat_t ast = get_adi_info (inferior_ptid.pid ());
      version = parse_and_eval_long (q);
      if (version < 0 || version > ast.max_version)
	error (_("Invalid ADI version tag %d"), version);
    }

  do_assign (next_address, cnt, version);
}

void _initialize_sparc64_adi_tdep ();
void
_initialize_sparc64_adi_tdep ()
{
  add_basic_prefix_cmd ("adi", class_support,
			_("ADI version related commands."),
			&sparc64adilist, 0, &cmdlist);
  cmd_list_element *adi_examine_cmd
    = add_cmd ("examine", class_support, adi_examine_command,
	       _("Examine ADI versions."), &sparc64adilist);
  add_alias_cmd ("x", adi_examine_cmd, no_class, 1, &sparc64adilist);
  add_cmd ("assign", class_support, adi_assign_command,
	   _("Assign ADI versions."), &sparc64adilist);

}


/* The functions on this page are intended to be used to classify
   function arguments.  */

/* Check whether TYPE is "Integral or Pointer".  */

static int
sparc64_integral_or_pointer_p (const struct type *type)
{
  switch (type->code ())
    {
    case TYPE_CODE_INT:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_CHAR:
    case TYPE_CODE_ENUM:
    case TYPE_CODE_RANGE:
      {
	int len = type->length ();
	gdb_assert (len == 1 || len == 2 || len == 4 || len == 8);
      }
      return 1;
    case TYPE_CODE_PTR:
    case TYPE_CODE_REF:
    case TYPE_CODE_RVALUE_REF:
      {
	int len = type->length ();
	gdb_assert (len == 8);
      }
      return 1;
    default:
      break;
    }

  return 0;
}

/* Check whether TYPE is "Floating".  */

static int
sparc64_floating_p (const struct type *type)
{
  switch (type->code ())
    {
    case TYPE_CODE_FLT:
      {
	int len = type->length ();
	gdb_assert (len == 4 || len == 8 || len == 16);
      }
      return 1;
    default:
      break;
    }

  return 0;
}

/* Check whether TYPE is "Complex Floating".  */

static int
sparc64_complex_floating_p (const struct type *type)
{
  switch (type->code ())
    {
    case TYPE_CODE_COMPLEX:
      {
	int len = type->length ();
	gdb_assert (len == 8 || len == 16 || len == 32);
      }
      return 1;
    default:
      break;
    }

  return 0;
}

/* Check whether TYPE is "Structure or Union".

   In terms of Ada subprogram calls, arrays are treated the same as
   struct and union types.  So this function also returns non-zero
   for array types.  */

static int
sparc64_structure_or_union_p (const struct type *type)
{
  switch (type->code ())
    {
    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
    case TYPE_CODE_ARRAY:
      return 1;
    default:
      break;
    }

  return 0;
}


/* Construct types for ISA-specific registers.  */

static struct type *
sparc64_pstate_type (struct gdbarch *gdbarch)
{
  sparc_gdbarch_tdep *tdep = gdbarch_tdep<sparc_gdbarch_tdep> (gdbarch);

  if (!tdep->sparc64_pstate_type)
    {
      struct type *type;

      type = arch_flags_type (gdbarch, "builtin_type_sparc64_pstate", 64);
      append_flags_type_flag (type, 0, "AG");
      append_flags_type_flag (type, 1, "IE");
      append_flags_type_flag (type, 2, "PRIV");
      append_flags_type_flag (type, 3, "AM");
      append_flags_type_flag (type, 4, "PEF");
      append_flags_type_flag (type, 5, "RED");
      append_flags_type_flag (type, 8, "TLE");
      append_flags_type_flag (type, 9, "CLE");
      append_flags_type_flag (type, 10, "PID0");
      append_flags_type_flag (type, 11, "PID1");

      tdep->sparc64_pstate_type = type;
    }

  return tdep->sparc64_pstate_type;
}

static struct type *
sparc64_ccr_type (struct gdbarch *gdbarch)
{
  sparc_gdbarch_tdep *tdep = gdbarch_tdep<sparc_gdbarch_tdep> (gdbarch);

  if (tdep->sparc64_ccr_type == NULL)
    {
      struct type *type;

      type = arch_flags_type (gdbarch, "builtin_type_sparc64_ccr", 64);
      append_flags_type_flag (type, 0, "icc.c");
      append_flags_type_flag (type, 1, "icc.v");
      append_flags_type_flag (type, 2, "icc.z");
      append_flags_type_flag (type, 3, "icc.n");
      append_flags_type_flag (type, 4, "xcc.c");
      append_flags_type_flag (type, 5, "xcc.v");
      append_flags_type_flag (type, 6, "xcc.z");
      append_flags_type_flag (type, 7, "xcc.n");

      tdep->sparc64_ccr_type = type;
    }

  return tdep->sparc64_ccr_type;
}

static struct type *
sparc64_fsr_type (struct gdbarch *gdbarch)
{
  sparc_gdbarch_tdep *tdep = gdbarch_tdep<sparc_gdbarch_tdep> (gdbarch);

  if (!tdep->sparc64_fsr_type)
    {
      struct type *type;

      type = arch_flags_type (gdbarch, "builtin_type_sparc64_fsr", 64);
      append_flags_type_flag (type, 0, "NXC");
      append_flags_type_flag (type, 1, "DZC");
      append_flags_type_flag (type, 2, "UFC");
      append_flags_type_flag (type, 3, "OFC");
      append_flags_type_flag (type, 4, "NVC");
      append_flags_type_flag (type, 5, "NXA");
      append_flags_type_flag (type, 6, "DZA");
      append_flags_type_flag (type, 7, "UFA");
      append_flags_type_flag (type, 8, "OFA");
      append_flags_type_flag (type, 9, "NVA");
      append_flags_type_flag (type, 22, "NS");
      append_flags_type_flag (type, 23, "NXM");
      append_flags_type_flag (type, 24, "DZM");
      append_flags_type_flag (type, 25, "UFM");
      append_flags_type_flag (type, 26, "OFM");
      append_flags_type_flag (type, 27, "NVM");

      tdep->sparc64_fsr_type = type;
    }

  return tdep->sparc64_fsr_type;
}

static struct type *
sparc64_fprs_type (struct gdbarch *gdbarch)
{
  sparc_gdbarch_tdep *tdep = gdbarch_tdep<sparc_gdbarch_tdep> (gdbarch);

  if (!tdep->sparc64_fprs_type)
    {
      struct type *type;

      type = arch_flags_type (gdbarch, "builtin_type_sparc64_fprs", 64);
      append_flags_type_flag (type, 0, "DL");
      append_flags_type_flag (type, 1, "DU");
      append_flags_type_flag (type, 2, "FEF");

      tdep->sparc64_fprs_type = type;
    }

  return tdep->sparc64_fprs_type;
}


/* Register information.  */
#define SPARC64_FPU_REGISTERS                             \
  "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7",         \
  "f8", "f9", "f10", "f11", "f12", "f13", "f14", "f15",   \
  "f16", "f17", "f18", "f19", "f20", "f21", "f22", "f23", \
  "f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31", \
  "f32", "f34", "f36", "f38", "f40", "f42", "f44", "f46", \
  "f48", "f50", "f52", "f54", "f56", "f58", "f60", "f62"
#define SPARC64_CP0_REGISTERS                                             \
  "pc", "npc",                                                            \
  /* FIXME: Give "state" a name until we start using register groups.  */ \
  "state",                                                                \
  "fsr",                                                                  \
  "fprs",                                                                 \
  "y"

static const char * const sparc64_fpu_register_names[] = {
  SPARC64_FPU_REGISTERS
};
static const char * const sparc64_cp0_register_names[] = {
  SPARC64_CP0_REGISTERS
};

static const char * const sparc64_register_names[] =
{
  SPARC_CORE_REGISTERS,
  SPARC64_FPU_REGISTERS,
  SPARC64_CP0_REGISTERS
};

/* Total number of registers.  */
#define SPARC64_NUM_REGS ARRAY_SIZE (sparc64_register_names)

/* We provide the aliases %d0..%d62 and %q0..%q60 for the floating
   registers as "psuedo" registers.  */

static const char * const sparc64_pseudo_register_names[] =
{
  "cwp", "pstate", "asi", "ccr",

  "d0", "d2", "d4", "d6", "d8", "d10", "d12", "d14",
  "d16", "d18", "d20", "d22", "d24", "d26", "d28", "d30",
  "d32", "d34", "d36", "d38", "d40", "d42", "d44", "d46",
  "d48", "d50", "d52", "d54", "d56", "d58", "d60", "d62",

  "q0", "q4", "q8", "q12", "q16", "q20", "q24", "q28",
  "q32", "q36", "q40", "q44", "q48", "q52", "q56", "q60",
};

/* Total number of pseudo registers.  */
#define SPARC64_NUM_PSEUDO_REGS ARRAY_SIZE (sparc64_pseudo_register_names)

/* Return the name of pseudo register REGNUM.  */

static const char *
sparc64_pseudo_register_name (struct gdbarch *gdbarch, int regnum)
{
  regnum -= gdbarch_num_regs (gdbarch);

  gdb_assert (regnum < SPARC64_NUM_PSEUDO_REGS);
  return sparc64_pseudo_register_names[regnum];
}

/* Return the name of register REGNUM.  */

static const char *
sparc64_register_name (struct gdbarch *gdbarch, int regnum)
{
  if (tdesc_has_registers (gdbarch_target_desc (gdbarch)))
    return tdesc_register_name (gdbarch, regnum);

  if (regnum >= 0 && regnum < gdbarch_num_regs (gdbarch))
    return sparc64_register_names[regnum];

  return sparc64_pseudo_register_name (gdbarch, regnum);
}

/* Return the GDB type object for the "standard" data type of data in
   pseudo register REGNUM.  */

static struct type *
sparc64_pseudo_register_type (struct gdbarch *gdbarch, int regnum)
{
  regnum -= gdbarch_num_regs (gdbarch);

  if (regnum == SPARC64_CWP_REGNUM)
    return builtin_type (gdbarch)->builtin_int64;
  if (regnum == SPARC64_PSTATE_REGNUM)
    return sparc64_pstate_type (gdbarch);
  if (regnum == SPARC64_ASI_REGNUM)
    return builtin_type (gdbarch)->builtin_int64;
  if (regnum == SPARC64_CCR_REGNUM)
    return sparc64_ccr_type (gdbarch);
  if (regnum >= SPARC64_D0_REGNUM && regnum <= SPARC64_D62_REGNUM)
    return builtin_type (gdbarch)->builtin_double;
  if (regnum >= SPARC64_Q0_REGNUM && regnum <= SPARC64_Q60_REGNUM)
    return builtin_type (gdbarch)->builtin_long_double;

  internal_error (_("sparc64_pseudo_register_type: bad register number %d"),
		  regnum);
}

/* Return the GDB type object for the "standard" data type of data in
   register REGNUM.  */

static struct type *
sparc64_register_type (struct gdbarch *gdbarch, int regnum)
{
  if (tdesc_has_registers (gdbarch_target_desc (gdbarch)))
    return tdesc_register_type (gdbarch, regnum);

  /* Raw registers.  */
  if (regnum == SPARC_SP_REGNUM || regnum == SPARC_FP_REGNUM)
    return builtin_type (gdbarch)->builtin_data_ptr;
  if (regnum >= SPARC_G0_REGNUM && regnum <= SPARC_I7_REGNUM)
    return builtin_type (gdbarch)->builtin_int64;
  if (regnum >= SPARC_F0_REGNUM && regnum <= SPARC_F31_REGNUM)
    return builtin_type (gdbarch)->builtin_float;
  if (regnum >= SPARC64_F32_REGNUM && regnum <= SPARC64_F62_REGNUM)
    return builtin_type (gdbarch)->builtin_double;
  if (regnum == SPARC64_PC_REGNUM || regnum == SPARC64_NPC_REGNUM)
    return builtin_type (gdbarch)->builtin_func_ptr;
  /* This raw register contains the contents of %cwp, %pstate, %asi
     and %ccr as laid out in a %tstate register.  */
  if (regnum == SPARC64_STATE_REGNUM)
    return builtin_type (gdbarch)->builtin_int64;
  if (regnum == SPARC64_FSR_REGNUM)
    return sparc64_fsr_type (gdbarch);
  if (regnum == SPARC64_FPRS_REGNUM)
    return sparc64_fprs_type (gdbarch);
  /* "Although Y is a 64-bit register, its high-order 32 bits are
     reserved and always read as 0."  */
  if (regnum == SPARC64_Y_REGNUM)
    return builtin_type (gdbarch)->builtin_int64;

  /* Pseudo registers.  */
  if (regnum >= gdbarch_num_regs (gdbarch))
    return sparc64_pseudo_register_type (gdbarch, regnum);

  internal_error (_("invalid regnum"));
}

static enum register_status
sparc64_pseudo_register_read (struct gdbarch *gdbarch,
			      readable_regcache *regcache,
			      int regnum, gdb_byte *buf)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  enum register_status status;

  regnum -= gdbarch_num_regs (gdbarch);

  if (regnum >= SPARC64_D0_REGNUM && regnum <= SPARC64_D30_REGNUM)
    {
      regnum = SPARC_F0_REGNUM + 2 * (regnum - SPARC64_D0_REGNUM);
      status = regcache->raw_read (regnum, buf);
      if (status == REG_VALID)
	status = regcache->raw_read (regnum + 1, buf + 4);
      return status;
    }
  else if (regnum >= SPARC64_D32_REGNUM && regnum <= SPARC64_D62_REGNUM)
    {
      regnum = SPARC64_F32_REGNUM + (regnum - SPARC64_D32_REGNUM);
      return regcache->raw_read (regnum, buf);
    }
  else if (regnum >= SPARC64_Q0_REGNUM && regnum <= SPARC64_Q28_REGNUM)
    {
      regnum = SPARC_F0_REGNUM + 4 * (regnum - SPARC64_Q0_REGNUM);

      status = regcache->raw_read (regnum, buf);
      if (status == REG_VALID)
	status = regcache->raw_read (regnum + 1, buf + 4);
      if (status == REG_VALID)
	status = regcache->raw_read (regnum + 2, buf + 8);
      if (status == REG_VALID)
	status = regcache->raw_read (regnum + 3, buf + 12);

      return status;
    }
  else if (regnum >= SPARC64_Q32_REGNUM && regnum <= SPARC64_Q60_REGNUM)
    {
      regnum = SPARC64_F32_REGNUM + 2 * (regnum - SPARC64_Q32_REGNUM);

      status = regcache->raw_read (regnum, buf);
      if (status == REG_VALID)
	status = regcache->raw_read (regnum + 1, buf + 8);

      return status;
    }
  else if (regnum == SPARC64_CWP_REGNUM
	   || regnum == SPARC64_PSTATE_REGNUM
	   || regnum == SPARC64_ASI_REGNUM
	   || regnum == SPARC64_CCR_REGNUM)
    {
      ULONGEST state;

      status = regcache->raw_read (SPARC64_STATE_REGNUM, &state);
      if (status != REG_VALID)
	return status;

      switch (regnum)
	{
	case SPARC64_CWP_REGNUM:
	  state = (state >> 0) & ((1 << 5) - 1);
	  break;
	case SPARC64_PSTATE_REGNUM:
	  state = (state >> 8) & ((1 << 12) - 1);
	  break;
	case SPARC64_ASI_REGNUM:
	  state = (state >> 24) & ((1 << 8) - 1);
	  break;
	case SPARC64_CCR_REGNUM:
	  state = (state >> 32) & ((1 << 8) - 1);
	  break;
	}
      store_unsigned_integer (buf, 8, byte_order, state);
    }

  return REG_VALID;
}

static void
sparc64_pseudo_register_write (struct gdbarch *gdbarch,
			       struct regcache *regcache,
			       int regnum, const gdb_byte *buf)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  regnum -= gdbarch_num_regs (gdbarch);

  if (regnum >= SPARC64_D0_REGNUM && regnum <= SPARC64_D30_REGNUM)
    {
      regnum = SPARC_F0_REGNUM + 2 * (regnum - SPARC64_D0_REGNUM);
      regcache->raw_write (regnum, buf);
      regcache->raw_write (regnum + 1, buf + 4);
    }
  else if (regnum >= SPARC64_D32_REGNUM && regnum <= SPARC64_D62_REGNUM)
    {
      regnum = SPARC64_F32_REGNUM + (regnum - SPARC64_D32_REGNUM);
      regcache->raw_write (regnum, buf);
    }
  else if (regnum >= SPARC64_Q0_REGNUM && regnum <= SPARC64_Q28_REGNUM)
    {
      regnum = SPARC_F0_REGNUM + 4 * (regnum - SPARC64_Q0_REGNUM);
      regcache->raw_write (regnum, buf);
      regcache->raw_write (regnum + 1, buf + 4);
      regcache->raw_write (regnum + 2, buf + 8);
      regcache->raw_write (regnum + 3, buf + 12);
    }
  else if (regnum >= SPARC64_Q32_REGNUM && regnum <= SPARC64_Q60_REGNUM)
    {
      regnum = SPARC64_F32_REGNUM + 2 * (regnum - SPARC64_Q32_REGNUM);
      regcache->raw_write (regnum, buf);
      regcache->raw_write (regnum + 1, buf + 8);
    }
  else if (regnum == SPARC64_CWP_REGNUM
	   || regnum == SPARC64_PSTATE_REGNUM
	   || regnum == SPARC64_ASI_REGNUM
	   || regnum == SPARC64_CCR_REGNUM)
    {
      ULONGEST state, bits;

      regcache_raw_read_unsigned (regcache, SPARC64_STATE_REGNUM, &state);
      bits = extract_unsigned_integer (buf, 8, byte_order);
      switch (regnum)
	{
	case SPARC64_CWP_REGNUM:
	  state |= ((bits & ((1 << 5) - 1)) << 0);
	  break;
	case SPARC64_PSTATE_REGNUM:
	  state |= ((bits & ((1 << 12) - 1)) << 8);
	  break;
	case SPARC64_ASI_REGNUM:
	  state |= ((bits & ((1 << 8) - 1)) << 24);
	  break;
	case SPARC64_CCR_REGNUM:
	  state |= ((bits & ((1 << 8) - 1)) << 32);
	  break;
	}
      regcache_raw_write_unsigned (regcache, SPARC64_STATE_REGNUM, state);
    }
}


/* Return PC of first real instruction of the function starting at
   START_PC.  */

static CORE_ADDR
sparc64_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR start_pc)
{
  struct symtab_and_line sal;
  CORE_ADDR func_start, func_end;
  struct sparc_frame_cache cache;

  /* This is the preferred method, find the end of the prologue by
     using the debugging information.  */
  if (find_pc_partial_function (start_pc, NULL, &func_start, &func_end))
    {
      sal = find_pc_line (func_start, 0);

      if (sal.end < func_end
	  && start_pc <= sal.end)
	return sal.end;
    }

  return sparc_analyze_prologue (gdbarch, start_pc, 0xffffffffffffffffULL,
				 &cache);
}

/* Normal frames.  */

static struct sparc_frame_cache *
sparc64_frame_cache (frame_info_ptr this_frame, void **this_cache)
{
  return sparc_frame_cache (this_frame, this_cache);
}

static void
sparc64_frame_this_id (frame_info_ptr this_frame, void **this_cache,
		       struct frame_id *this_id)
{
  struct sparc_frame_cache *cache =
    sparc64_frame_cache (this_frame, this_cache);

  /* This marks the outermost frame.  */
  if (cache->base == 0)
    return;

  (*this_id) = frame_id_build (cache->base, cache->pc);
}

static struct value *
sparc64_frame_prev_register (frame_info_ptr this_frame, void **this_cache,
			     int regnum)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct sparc_frame_cache *cache =
    sparc64_frame_cache (this_frame, this_cache);

  if (regnum == SPARC64_PC_REGNUM || regnum == SPARC64_NPC_REGNUM)
    {
      CORE_ADDR pc = (regnum == SPARC64_NPC_REGNUM) ? 4 : 0;

      regnum =
	(cache->copied_regs_mask & 0x80) ? SPARC_I7_REGNUM : SPARC_O7_REGNUM;
      pc += get_frame_register_unsigned (this_frame, regnum) + 8;
      return frame_unwind_got_constant (this_frame, regnum, pc);
    }

  /* Handle StackGhost.  */
  {
    ULONGEST wcookie = sparc_fetch_wcookie (gdbarch);

    if (wcookie != 0 && !cache->frameless_p && regnum == SPARC_I7_REGNUM)
      {
	CORE_ADDR addr = cache->base + (regnum - SPARC_L0_REGNUM) * 8;
	ULONGEST i7;

	/* Read the value in from memory.  */
	i7 = get_frame_memory_unsigned (this_frame, addr, 8);
	return frame_unwind_got_constant (this_frame, regnum, i7 ^ wcookie);
      }
  }

  /* The previous frame's `local' and `in' registers may have been saved
     in the register save area.  */
  if (regnum >= SPARC_L0_REGNUM && regnum <= SPARC_I7_REGNUM
      && (cache->saved_regs_mask & (1 << (regnum - SPARC_L0_REGNUM))))
    {
      CORE_ADDR addr = cache->base + (regnum - SPARC_L0_REGNUM) * 8;

      return frame_unwind_got_memory (this_frame, regnum, addr);
    }

  /* The previous frame's `out' registers may be accessible as the current
     frame's `in' registers.  */
  if (regnum >= SPARC_O0_REGNUM && regnum <= SPARC_O7_REGNUM
      && (cache->copied_regs_mask & (1 << (regnum - SPARC_O0_REGNUM))))
    regnum += (SPARC_I0_REGNUM - SPARC_O0_REGNUM);

  return frame_unwind_got_register (this_frame, regnum, regnum);
}

static const struct frame_unwind sparc64_frame_unwind =
{
  "sparc64 prologue",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  sparc64_frame_this_id,
  sparc64_frame_prev_register,
  NULL,
  default_frame_sniffer
};


static CORE_ADDR
sparc64_frame_base_address (frame_info_ptr this_frame, void **this_cache)
{
  struct sparc_frame_cache *cache =
    sparc64_frame_cache (this_frame, this_cache);

  return cache->base;
}

static const struct frame_base sparc64_frame_base =
{
  &sparc64_frame_unwind,
  sparc64_frame_base_address,
  sparc64_frame_base_address,
  sparc64_frame_base_address
};

/* Check whether TYPE must be 16-byte aligned.  */

static int
sparc64_16_byte_align_p (struct type *type)
{
  if (type->code () == TYPE_CODE_ARRAY)
    {
      struct type *t = check_typedef (type->target_type ());

      if (sparc64_floating_p (t))
	return 1;
    }
  if (sparc64_floating_p (type) && type->length () == 16)
    return 1;

  if (sparc64_structure_or_union_p (type))
    {
      int i;

      for (i = 0; i < type->num_fields (); i++)
	{
	  struct type *subtype = check_typedef (type->field (i).type ());

	  if (sparc64_16_byte_align_p (subtype))
	    return 1;
	}
    }

  return 0;
}

/* Store floating fields of element ELEMENT of an "parameter array"
   that has type TYPE and is stored at BITPOS in VALBUF in the
   appropriate registers of REGCACHE.  This function can be called
   recursively and therefore handles floating types in addition to
   structures.  */

static void
sparc64_store_floating_fields (struct regcache *regcache, struct type *type,
			       const gdb_byte *valbuf, int element, int bitpos)
{
  struct gdbarch *gdbarch = regcache->arch ();
  int len = type->length ();

  gdb_assert (element < 16);

  if (type->code () == TYPE_CODE_ARRAY)
    {
      gdb_byte buf[8];
      int regnum = SPARC_F0_REGNUM + element * 2 + bitpos / 32;

      valbuf += bitpos / 8;
      if (len < 8)
	{
	  memset (buf, 0, 8 - len);
	  memcpy (buf + 8 - len, valbuf, len);
	  valbuf = buf;
	  len = 8;
	}
      for (int n = 0; n < (len + 3) / 4; n++)
	regcache->cooked_write (regnum + n, valbuf + n * 4);
    }
  else if (sparc64_floating_p (type)
      || (sparc64_complex_floating_p (type) && len <= 16))
    {
      int regnum;

      if (len == 16)
	{
	  gdb_assert (bitpos == 0);
	  gdb_assert ((element % 2) == 0);

	  regnum = gdbarch_num_regs (gdbarch) + SPARC64_Q0_REGNUM + element / 2;
	  regcache->cooked_write (regnum, valbuf);
	}
      else if (len == 8)
	{
	  gdb_assert (bitpos == 0 || bitpos == 64);

	  regnum = gdbarch_num_regs (gdbarch) + SPARC64_D0_REGNUM
		   + element + bitpos / 64;
	  regcache->cooked_write (regnum, valbuf + (bitpos / 8));
	}
      else
	{
	  gdb_assert (len == 4);
	  gdb_assert (bitpos % 32 == 0 && bitpos >= 0 && bitpos < 128);

	  regnum = SPARC_F0_REGNUM + element * 2 + bitpos / 32;
	  regcache->cooked_write (regnum, valbuf + (bitpos / 8));
	}
    }
  else if (sparc64_structure_or_union_p (type))
    {
      int i;

      for (i = 0; i < type->num_fields (); i++)
	{
	  struct type *subtype = check_typedef (type->field (i).type ());
	  int subpos = bitpos + type->field (i).loc_bitpos ();

	  sparc64_store_floating_fields (regcache, subtype, valbuf,
					 element, subpos);
	}

      /* GCC has an interesting bug.  If TYPE is a structure that has
	 a single `float' member, GCC doesn't treat it as a structure
	 at all, but rather as an ordinary `float' argument.  This
	 argument will be stored in %f1, as required by the psABI.
	 However, as a member of a structure the psABI requires it to
	 be stored in %f0.  This bug is present in GCC 3.3.2, but
	 probably in older releases to.  To appease GCC, if a
	 structure has only a single `float' member, we store its
	 value in %f1 too (we already have stored in %f0).  */
      if (type->num_fields () == 1)
	{
	  struct type *subtype = check_typedef (type->field (0).type ());

	  if (sparc64_floating_p (subtype) && subtype->length () == 4)
	    regcache->cooked_write (SPARC_F1_REGNUM, valbuf);
	}
    }
}

/* Fetch floating fields from a variable of type TYPE from the
   appropriate registers for BITPOS in REGCACHE and store it at BITPOS
   in VALBUF.  This function can be called recursively and therefore
   handles floating types in addition to structures.  */

static void
sparc64_extract_floating_fields (struct regcache *regcache, struct type *type,
				 gdb_byte *valbuf, int bitpos)
{
  struct gdbarch *gdbarch = regcache->arch ();

  if (type->code () == TYPE_CODE_ARRAY)
    {
      int len = type->length ();
      int regnum =  SPARC_F0_REGNUM + bitpos / 32;

      valbuf += bitpos / 8;
      if (len < 4)
	{
	  gdb_byte buf[4];
	  regcache->cooked_read (regnum, buf);
	  memcpy (valbuf, buf + 4 - len, len);
	}
      else
	for (int i = 0; i < (len + 3) / 4; i++)
	  regcache->cooked_read (regnum + i, valbuf + i * 4);
    }
  else if (sparc64_floating_p (type))
    {
      int len = type->length ();
      int regnum;

      if (len == 16)
	{
	  gdb_assert (bitpos == 0 || bitpos == 128);

	  regnum = gdbarch_num_regs (gdbarch) + SPARC64_Q0_REGNUM
		   + bitpos / 128;
	  regcache->cooked_read (regnum, valbuf + (bitpos / 8));
	}
      else if (len == 8)
	{
	  gdb_assert (bitpos % 64 == 0 && bitpos >= 0 && bitpos < 256);

	  regnum = gdbarch_num_regs (gdbarch) + SPARC64_D0_REGNUM + bitpos / 64;
	  regcache->cooked_read (regnum, valbuf + (bitpos / 8));
	}
      else
	{
	  gdb_assert (len == 4);
	  gdb_assert (bitpos % 32 == 0 && bitpos >= 0 && bitpos < 256);

	  regnum = SPARC_F0_REGNUM + bitpos / 32;
	  regcache->cooked_read (regnum, valbuf + (bitpos / 8));
	}
    }
  else if (sparc64_structure_or_union_p (type))
    {
      int i;

      for (i = 0; i < type->num_fields (); i++)
	{
	  struct type *subtype = check_typedef (type->field (i).type ());
	  int subpos = bitpos + type->field (i).loc_bitpos ();

	  sparc64_extract_floating_fields (regcache, subtype, valbuf, subpos);
	}
    }
}

/* Store the NARGS arguments ARGS and STRUCT_ADDR (if STRUCT_RETURN is
   non-zero) in REGCACHE and on the stack (starting from address SP).  */

static CORE_ADDR
sparc64_store_arguments (struct regcache *regcache, int nargs,
			 struct value **args, CORE_ADDR sp,
			 function_call_return_method return_method,
			 CORE_ADDR struct_addr)
{
  struct gdbarch *gdbarch = regcache->arch ();
  /* Number of extended words in the "parameter array".  */
  int num_elements = 0;
  int element = 0;
  int i;

  /* Take BIAS into account.  */
  sp += BIAS;

  /* First we calculate the number of extended words in the "parameter
     array".  While doing so we also convert some of the arguments.  */

  if (return_method == return_method_struct)
    num_elements++;

  for (i = 0; i < nargs; i++)
    {
      struct type *type = args[i]->type ();
      int len = type->length ();

      if (sparc64_structure_or_union_p (type)
	  || (sparc64_complex_floating_p (type) && len == 32))
	{
	  /* Structure or Union arguments.  */
	  if (len <= 16)
	    {
	      if (num_elements % 2 && sparc64_16_byte_align_p (type))
		num_elements++;
	      num_elements += ((len + 7) / 8);
	    }
	  else
	    {
	      /* The psABI says that "Structures or unions larger than
		 sixteen bytes are copied by the caller and passed
		 indirectly; the caller will pass the address of a
		 correctly aligned structure value.  This sixty-four
		 bit address will occupy one word in the parameter
		 array, and may be promoted to an %o register like any
		 other pointer value."  Allocate memory for these
		 values on the stack.  */
	      sp -= len;

	      /* Use 16-byte alignment for these values.  That's
		 always correct, and wasting a few bytes shouldn't be
		 a problem.  */
	      sp &= ~0xf;

	      write_memory (sp, args[i]->contents ().data (), len);
	      args[i] = value_from_pointer (lookup_pointer_type (type), sp);
	      num_elements++;
	    }
	}
      else if (sparc64_floating_p (type) || sparc64_complex_floating_p (type))
	{
	  /* Floating arguments.  */
	  if (len == 16)
	    {
	      /* The psABI says that "Each quad-precision parameter
		 value will be assigned to two extended words in the
		 parameter array.  */
	      num_elements += 2;

	      /* The psABI says that "Long doubles must be
		 quad-aligned, and thus a hole might be introduced
		 into the parameter array to force alignment."  Skip
		 an element if necessary.  */
	      if ((num_elements % 2) && sparc64_16_byte_align_p (type))
		num_elements++;
	    }
	  else
	    num_elements++;
	}
      else
	{
	  /* Integral and pointer arguments.  */
	  gdb_assert (sparc64_integral_or_pointer_p (type));

	  /* The psABI says that "Each argument value of integral type
	     smaller than an extended word will be widened by the
	     caller to an extended word according to the signed-ness
	     of the argument type."  */
	  if (len < 8)
	    args[i] = value_cast (builtin_type (gdbarch)->builtin_int64,
				  args[i]);
	  num_elements++;
	}
    }

  /* Allocate the "parameter array".  */
  sp -= num_elements * 8;

  /* The psABI says that "Every stack frame must be 16-byte aligned."  */
  sp &= ~0xf;

  /* Now we store the arguments in to the "parameter array".  Some
     Integer or Pointer arguments and Structure or Union arguments
     will be passed in %o registers.  Some Floating arguments and
     floating members of structures are passed in floating-point
     registers.  However, for functions with variable arguments,
     floating arguments are stored in an %0 register, and for
     functions without a prototype floating arguments are stored in
     both a floating-point and an %o registers, or a floating-point
     register and memory.  To simplify the logic here we always pass
     arguments in memory, an %o register, and a floating-point
     register if appropriate.  This should be no problem since the
     contents of any unused memory or registers in the "parameter
     array" are undefined.  */

  if (return_method == return_method_struct)
    {
      regcache_cooked_write_unsigned (regcache, SPARC_O0_REGNUM, struct_addr);
      element++;
    }

  for (i = 0; i < nargs; i++)
    {
      const gdb_byte *valbuf = args[i]->contents ().data ();
      struct type *type = args[i]->type ();
      int len = type->length ();
      int regnum = -1;
      gdb_byte buf[16];

      if (sparc64_structure_or_union_p (type)
	  || (sparc64_complex_floating_p (type) && len == 32))
	{
	  /* Structure, Union or long double Complex arguments.  */
	  gdb_assert (len <= 16);
	  memset (buf, 0, sizeof (buf));
	  memcpy (buf, valbuf, len);
	  valbuf = buf;

	  if (element % 2 && sparc64_16_byte_align_p (type))
	    element++;

	  if (element < 6)
	    {
	      regnum = SPARC_O0_REGNUM + element;
	      if (len > 8 && element < 5)
		regcache->cooked_write (regnum + 1, valbuf + 8);
	    }

	  if (element < 16)
	    sparc64_store_floating_fields (regcache, type, valbuf, element, 0);
	}
      else if (sparc64_complex_floating_p (type))
	{
	  /* Float Complex or double Complex arguments.  */
	  if (element < 16)
	    {
	      regnum = gdbarch_num_regs (gdbarch) + SPARC64_D0_REGNUM + element;

	      if (len == 16)
		{
		  if (regnum < gdbarch_num_regs (gdbarch) + SPARC64_D30_REGNUM)
		    regcache->cooked_write (regnum + 1, valbuf + 8);
		  if (regnum < gdbarch_num_regs (gdbarch) + SPARC64_D10_REGNUM)
		    regcache->cooked_write (SPARC_O0_REGNUM + element + 1,
					    valbuf + 8);
		}
	    }
	}
      else if (sparc64_floating_p (type))
	{
	  /* Floating arguments.  */
	  if (len == 16)
	    {
	      if (element % 2)
		element++;
	      if (element < 16)
		regnum = gdbarch_num_regs (gdbarch) + SPARC64_Q0_REGNUM
			 + element / 2;
	    }
	  else if (len == 8)
	    {
	      if (element < 16)
		regnum = gdbarch_num_regs (gdbarch) + SPARC64_D0_REGNUM
			 + element;
	    }
	  else if (len == 4)
	    {
	      /* The psABI says "Each single-precision parameter value
		 will be assigned to one extended word in the
		 parameter array, and right-justified within that
		 word; the left half (even float register) is
		 undefined."  Even though the psABI says that "the
		 left half is undefined", set it to zero here.  */
	      memset (buf, 0, 4);
	      memcpy (buf + 4, valbuf, 4);
	      valbuf = buf;
	      len = 8;
	      if (element < 16)
		regnum = gdbarch_num_regs (gdbarch) + SPARC64_D0_REGNUM
			 + element;
	    }
	}
      else
	{
	  /* Integral and pointer arguments.  */
	  gdb_assert (len == 8);
	  if (element < 6)
	    regnum = SPARC_O0_REGNUM + element;
	}

      if (regnum != -1)
	{
	  regcache->cooked_write (regnum, valbuf);

	  /* If we're storing the value in a floating-point register,
	     also store it in the corresponding %0 register(s).  */
	  if (regnum >= gdbarch_num_regs (gdbarch))
	    {
	      regnum -= gdbarch_num_regs (gdbarch);

	      if (regnum >= SPARC64_D0_REGNUM && regnum <= SPARC64_D10_REGNUM)
		{
		  gdb_assert (element < 6);
		  regnum = SPARC_O0_REGNUM + element;
		  regcache->cooked_write (regnum, valbuf);
		}
	      else if (regnum >= SPARC64_Q0_REGNUM && regnum <= SPARC64_Q8_REGNUM)
		{
		  gdb_assert (element < 5);
		  regnum = SPARC_O0_REGNUM + element;
		  regcache->cooked_write (regnum, valbuf);
		  regcache->cooked_write (regnum + 1, valbuf + 8);
		}
	    }
	}

      /* Always store the argument in memory.  */
      write_memory (sp + element * 8, valbuf, len);
      element += ((len + 7) / 8);
    }

  gdb_assert (element == num_elements);

  /* Take BIAS into account.  */
  sp -= BIAS;
  return sp;
}

static CORE_ADDR
sparc64_frame_align (struct gdbarch *gdbarch, CORE_ADDR address)
{
  /* The ABI requires 16-byte alignment.  */
  return address & ~0xf;
}

static CORE_ADDR
sparc64_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
			 struct regcache *regcache, CORE_ADDR bp_addr,
			 int nargs, struct value **args, CORE_ADDR sp,
			 function_call_return_method return_method,
			 CORE_ADDR struct_addr)
{
  /* Set return address.  */
  regcache_cooked_write_unsigned (regcache, SPARC_O7_REGNUM, bp_addr - 8);

  /* Set up function arguments.  */
  sp = sparc64_store_arguments (regcache, nargs, args, sp, return_method,
				struct_addr);

  /* Allocate the register save area.  */
  sp -= 16 * 8;

  /* Stack should be 16-byte aligned at this point.  */
  gdb_assert ((sp + BIAS) % 16 == 0);

  /* Finally, update the stack pointer.  */
  regcache_cooked_write_unsigned (regcache, SPARC_SP_REGNUM, sp);

  return sp + BIAS;
}


/* Extract from an array REGBUF containing the (raw) register state, a
   function return value of TYPE, and copy that into VALBUF.  */

static void
sparc64_extract_return_value (struct type *type, struct regcache *regcache,
			      gdb_byte *valbuf)
{
  int len = type->length ();
  gdb_byte buf[32];
  int i;

  if (sparc64_structure_or_union_p (type))
    {
      /* Structure or Union return values.  */
      gdb_assert (len <= 32);

      for (i = 0; i < ((len + 7) / 8); i++)
	regcache->cooked_read (SPARC_O0_REGNUM + i, buf + i * 8);
      if (type->code () != TYPE_CODE_UNION)
	sparc64_extract_floating_fields (regcache, type, buf, 0);
      memcpy (valbuf, buf, len);
    }
  else if (sparc64_floating_p (type) || sparc64_complex_floating_p (type))
    {
      /* Floating return values.  */
      for (i = 0; i < len / 4; i++)
	regcache->cooked_read (SPARC_F0_REGNUM + i, buf + i * 4);
      memcpy (valbuf, buf, len);
    }
  else if (type->code () == TYPE_CODE_ARRAY)
    {
      /* Small arrays are returned the same way as small structures.  */
      gdb_assert (len <= 32);

      for (i = 0; i < ((len + 7) / 8); i++)
	regcache->cooked_read (SPARC_O0_REGNUM + i, buf + i * 8);
      memcpy (valbuf, buf, len);
    }
  else
    {
      /* Integral and pointer return values.  */
      gdb_assert (sparc64_integral_or_pointer_p (type));

      /* Just stripping off any unused bytes should preserve the
	 signed-ness just fine.  */
      regcache->cooked_read (SPARC_O0_REGNUM, buf);
      memcpy (valbuf, buf + 8 - len, len);
    }
}

/* Write into the appropriate registers a function return value stored
   in VALBUF of type TYPE.  */

static void
sparc64_store_return_value (struct type *type, struct regcache *regcache,
			    const gdb_byte *valbuf)
{
  int len = type->length ();
  gdb_byte buf[16];
  int i;

  if (sparc64_structure_or_union_p (type))
    {
      /* Structure or Union return values.  */
      gdb_assert (len <= 32);

      /* Simplify matters by storing the complete value (including
	 floating members) into %o0 and %o1.  Floating members are
	 also store in the appropriate floating-point registers.  */
      memset (buf, 0, sizeof (buf));
      memcpy (buf, valbuf, len);
      for (i = 0; i < ((len + 7) / 8); i++)
	regcache->cooked_write (SPARC_O0_REGNUM + i, buf + i * 8);
      if (type->code () != TYPE_CODE_UNION)
	sparc64_store_floating_fields (regcache, type, buf, 0, 0);
    }
  else if (sparc64_floating_p (type) || sparc64_complex_floating_p (type))
    {
      /* Floating return values.  */
      memcpy (buf, valbuf, len);
      for (i = 0; i < len / 4; i++)
	regcache->cooked_write (SPARC_F0_REGNUM + i, buf + i * 4);
    }
  else if (type->code () == TYPE_CODE_ARRAY)
    {
      /* Small arrays are returned the same way as small structures.  */
      gdb_assert (len <= 32);

      memset (buf, 0, sizeof (buf));
      memcpy (buf, valbuf, len);
      for (i = 0; i < ((len + 7) / 8); i++)
	regcache->cooked_write (SPARC_O0_REGNUM + i, buf + i * 8);
    }
  else
    {
      /* Integral and pointer return values.  */
      gdb_assert (sparc64_integral_or_pointer_p (type));

      /* ??? Do we need to do any sign-extension here?  */
      memset (buf, 0, 8);
      memcpy (buf + 8 - len, valbuf, len);
      regcache->cooked_write (SPARC_O0_REGNUM, buf);
    }
}

static enum return_value_convention
sparc64_return_value (struct gdbarch *gdbarch, struct value *function,
		      struct type *type, struct regcache *regcache,
		      gdb_byte *readbuf, const gdb_byte *writebuf)
{
  if (type->length () > 32)
    return RETURN_VALUE_STRUCT_CONVENTION;

  if (readbuf)
    sparc64_extract_return_value (type, regcache, readbuf);
  if (writebuf)
    sparc64_store_return_value (type, regcache, writebuf);

  return RETURN_VALUE_REGISTER_CONVENTION;
}


static void
sparc64_dwarf2_frame_init_reg (struct gdbarch *gdbarch, int regnum,
			       struct dwarf2_frame_state_reg *reg,
			       frame_info_ptr this_frame)
{
  switch (regnum)
    {
    case SPARC_G0_REGNUM:
      /* Since %g0 is always zero, there is no point in saving it, and
	 people will be inclined omit it from the CFI.  Make sure we
	 don't warn about that.  */
      reg->how = DWARF2_FRAME_REG_SAME_VALUE;
      break;
    case SPARC_SP_REGNUM:
      reg->how = DWARF2_FRAME_REG_CFA;
      break;
    case SPARC64_PC_REGNUM:
      reg->how = DWARF2_FRAME_REG_RA_OFFSET;
      reg->loc.offset = 8;
      break;
    case SPARC64_NPC_REGNUM:
      reg->how = DWARF2_FRAME_REG_RA_OFFSET;
      reg->loc.offset = 12;
      break;
    }
}

/* sparc64_addr_bits_remove - remove useless address bits  */

static CORE_ADDR
sparc64_addr_bits_remove (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  return adi_normalize_address (addr);
}

void
sparc64_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  sparc_gdbarch_tdep *tdep = gdbarch_tdep<sparc_gdbarch_tdep> (gdbarch);

  tdep->pc_regnum = SPARC64_PC_REGNUM;
  tdep->npc_regnum = SPARC64_NPC_REGNUM;
  tdep->fpu_register_names = sparc64_fpu_register_names;
  tdep->fpu_registers_num = ARRAY_SIZE (sparc64_fpu_register_names);
  tdep->cp0_register_names = sparc64_cp0_register_names;
  tdep->cp0_registers_num = ARRAY_SIZE (sparc64_cp0_register_names);

  /* This is what all the fuss is about.  */
  set_gdbarch_long_bit (gdbarch, 64);
  set_gdbarch_long_long_bit (gdbarch, 64);
  set_gdbarch_ptr_bit (gdbarch, 64);

  set_gdbarch_wchar_bit (gdbarch, 16);
  set_gdbarch_wchar_signed (gdbarch, 0);

  set_gdbarch_num_regs (gdbarch, SPARC64_NUM_REGS);
  set_gdbarch_register_name (gdbarch, sparc64_register_name);
  set_gdbarch_register_type (gdbarch, sparc64_register_type);
  set_gdbarch_num_pseudo_regs (gdbarch, SPARC64_NUM_PSEUDO_REGS);
  set_tdesc_pseudo_register_name (gdbarch, sparc64_pseudo_register_name);
  set_tdesc_pseudo_register_type (gdbarch, sparc64_pseudo_register_type);
  set_gdbarch_pseudo_register_read (gdbarch, sparc64_pseudo_register_read);
  set_gdbarch_deprecated_pseudo_register_write (gdbarch,
						sparc64_pseudo_register_write);

  /* Register numbers of various important registers.  */
  set_gdbarch_pc_regnum (gdbarch, SPARC64_PC_REGNUM); /* %pc */

  /* Call dummy code.  */
  set_gdbarch_frame_align (gdbarch, sparc64_frame_align);
  set_gdbarch_call_dummy_location (gdbarch, AT_ENTRY_POINT);
  set_gdbarch_push_dummy_code (gdbarch, NULL);
  set_gdbarch_push_dummy_call (gdbarch, sparc64_push_dummy_call);

  set_gdbarch_return_value (gdbarch, sparc64_return_value);
  set_gdbarch_return_value_as_value (gdbarch, default_gdbarch_return_value);
  set_gdbarch_stabs_argument_has_addr
    (gdbarch, default_stabs_argument_has_addr);

  set_gdbarch_skip_prologue (gdbarch, sparc64_skip_prologue);
  set_gdbarch_stack_frame_destroyed_p (gdbarch, sparc_stack_frame_destroyed_p);

  /* Hook in the DWARF CFI frame unwinder.  */
  dwarf2_frame_set_init_reg (gdbarch, sparc64_dwarf2_frame_init_reg);
  /* FIXME: kettenis/20050423: Don't enable the unwinder until the
     StackGhost issues have been resolved.  */

  frame_unwind_append_unwinder (gdbarch, &sparc64_frame_unwind);
  frame_base_set_default (gdbarch, &sparc64_frame_base);

  set_gdbarch_addr_bits_remove (gdbarch, sparc64_addr_bits_remove);
}


/* Helper functions for dealing with register sets.  */

#define TSTATE_CWP	0x000000000000001fULL
#define TSTATE_ICC	0x0000000f00000000ULL
#define TSTATE_XCC	0x000000f000000000ULL

#define PSR_S		0x00000080
#ifndef PSR_ICC
#define PSR_ICC		0x00f00000
#endif
#define PSR_VERS	0x0f000000
#ifndef PSR_IMPL
#define PSR_IMPL	0xf0000000
#endif
#define PSR_V8PLUS	0xff000000
#define PSR_XCC		0x000f0000

void
sparc64_supply_gregset (const struct sparc_gregmap *gregmap,
			struct regcache *regcache,
			int regnum, const void *gregs)
{
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int sparc32 = (gdbarch_ptr_bit (gdbarch) == 32);
  const gdb_byte *regs = (const gdb_byte *) gregs;
  gdb_byte zero[8] = { 0 };
  int i;

  if (sparc32)
    {
      if (regnum == SPARC32_PSR_REGNUM || regnum == -1)
	{
	  int offset = gregmap->r_tstate_offset;
	  ULONGEST tstate, psr;
	  gdb_byte buf[4];

	  tstate = extract_unsigned_integer (regs + offset, 8, byte_order);
	  psr = ((tstate & TSTATE_CWP) | PSR_S | ((tstate & TSTATE_ICC) >> 12)
		 | ((tstate & TSTATE_XCC) >> 20) | PSR_V8PLUS);
	  store_unsigned_integer (buf, 4, byte_order, psr);
	  regcache->raw_supply (SPARC32_PSR_REGNUM, buf);
	}

      if (regnum == SPARC32_PC_REGNUM || regnum == -1)
	regcache->raw_supply (SPARC32_PC_REGNUM,
			      regs + gregmap->r_pc_offset + 4);

      if (regnum == SPARC32_NPC_REGNUM || regnum == -1)
	regcache->raw_supply (SPARC32_NPC_REGNUM,
			      regs + gregmap->r_npc_offset + 4);

      if (regnum == SPARC32_Y_REGNUM || regnum == -1)
	{
	  int offset = gregmap->r_y_offset + 8 - gregmap->r_y_size;
	  regcache->raw_supply (SPARC32_Y_REGNUM, regs + offset);
	}
    }
  else
    {
      if (regnum == SPARC64_STATE_REGNUM || regnum == -1)
	regcache->raw_supply (SPARC64_STATE_REGNUM,
			      regs + gregmap->r_tstate_offset);

      if (regnum == SPARC64_PC_REGNUM || regnum == -1)
	regcache->raw_supply (SPARC64_PC_REGNUM,
			      regs + gregmap->r_pc_offset);

      if (regnum == SPARC64_NPC_REGNUM || regnum == -1)
	regcache->raw_supply (SPARC64_NPC_REGNUM,
			      regs + gregmap->r_npc_offset);

      if (regnum == SPARC64_Y_REGNUM || regnum == -1)
	{
	  gdb_byte buf[8];

	  memset (buf, 0, 8);
	  memcpy (buf + 8 - gregmap->r_y_size,
		  regs + gregmap->r_y_offset, gregmap->r_y_size);
	  regcache->raw_supply (SPARC64_Y_REGNUM, buf);
	}

      if ((regnum == SPARC64_FPRS_REGNUM || regnum == -1)
	  && gregmap->r_fprs_offset != -1)
	regcache->raw_supply (SPARC64_FPRS_REGNUM,
			      regs + gregmap->r_fprs_offset);
    }

  if (regnum == SPARC_G0_REGNUM || regnum == -1)
    regcache->raw_supply (SPARC_G0_REGNUM, &zero);

  if ((regnum >= SPARC_G1_REGNUM && regnum <= SPARC_O7_REGNUM) || regnum == -1)
    {
      int offset = gregmap->r_g1_offset;

      if (sparc32)
	offset += 4;

      for (i = SPARC_G1_REGNUM; i <= SPARC_O7_REGNUM; i++)
	{
	  if (regnum == i || regnum == -1)
	    regcache->raw_supply (i, regs + offset);
	  offset += 8;
	}
    }

  if ((regnum >= SPARC_L0_REGNUM && regnum <= SPARC_I7_REGNUM) || regnum == -1)
    {
      /* Not all of the register set variants include Locals and
	 Inputs.  For those that don't, we read them off the stack.  */
      if (gregmap->r_l0_offset == -1)
	{
	  ULONGEST sp;

	  regcache_cooked_read_unsigned (regcache, SPARC_SP_REGNUM, &sp);
	  sparc_supply_rwindow (regcache, sp, regnum);
	}
      else
	{
	  int offset = gregmap->r_l0_offset;

	  if (sparc32)
	    offset += 4;

	  for (i = SPARC_L0_REGNUM; i <= SPARC_I7_REGNUM; i++)
	    {
	      if (regnum == i || regnum == -1)
		regcache->raw_supply (i, regs + offset);
	      offset += 8;
	    }
	}
    }
}

void
sparc64_collect_gregset (const struct sparc_gregmap *gregmap,
			 const struct regcache *regcache,
			 int regnum, void *gregs)
{
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int sparc32 = (gdbarch_ptr_bit (gdbarch) == 32);
  gdb_byte *regs = (gdb_byte *) gregs;
  int i;

  if (sparc32)
    {
      if (regnum == SPARC32_PSR_REGNUM || regnum == -1)
	{
	  int offset = gregmap->r_tstate_offset;
	  ULONGEST tstate, psr;
	  gdb_byte buf[8];

	  tstate = extract_unsigned_integer (regs + offset, 8, byte_order);
	  regcache->raw_collect (SPARC32_PSR_REGNUM, buf);
	  psr = extract_unsigned_integer (buf, 4, byte_order);
	  tstate |= (psr & PSR_ICC) << 12;
	  if ((psr & (PSR_VERS | PSR_IMPL)) == PSR_V8PLUS)
	    tstate |= (psr & PSR_XCC) << 20;
	  store_unsigned_integer (buf, 8, byte_order, tstate);
	  memcpy (regs + offset, buf, 8);
	}

      if (regnum == SPARC32_PC_REGNUM || regnum == -1)
	regcache->raw_collect (SPARC32_PC_REGNUM,
			       regs + gregmap->r_pc_offset + 4);

      if (regnum == SPARC32_NPC_REGNUM || regnum == -1)
	regcache->raw_collect (SPARC32_NPC_REGNUM,
			       regs + gregmap->r_npc_offset + 4);

      if (regnum == SPARC32_Y_REGNUM || regnum == -1)
	{
	  int offset = gregmap->r_y_offset + 8 - gregmap->r_y_size;
	  regcache->raw_collect (SPARC32_Y_REGNUM, regs + offset);
	}
    }
  else
    {
      if (regnum == SPARC64_STATE_REGNUM || regnum == -1)
	regcache->raw_collect (SPARC64_STATE_REGNUM,
			       regs + gregmap->r_tstate_offset);

      if (regnum == SPARC64_PC_REGNUM || regnum == -1)
	regcache->raw_collect (SPARC64_PC_REGNUM,
			       regs + gregmap->r_pc_offset);

      if (regnum == SPARC64_NPC_REGNUM || regnum == -1)
	regcache->raw_collect (SPARC64_NPC_REGNUM,
			       regs + gregmap->r_npc_offset);

      if (regnum == SPARC64_Y_REGNUM || regnum == -1)
	{
	  gdb_byte buf[8];

	  regcache->raw_collect (SPARC64_Y_REGNUM, buf);
	  memcpy (regs + gregmap->r_y_offset,
		  buf + 8 - gregmap->r_y_size, gregmap->r_y_size);
	}

      if ((regnum == SPARC64_FPRS_REGNUM || regnum == -1)
	  && gregmap->r_fprs_offset != -1)
	regcache->raw_collect (SPARC64_FPRS_REGNUM,
			       regs + gregmap->r_fprs_offset);

    }

  if ((regnum >= SPARC_G1_REGNUM && regnum <= SPARC_O7_REGNUM) || regnum == -1)
    {
      int offset = gregmap->r_g1_offset;

      if (sparc32)
	offset += 4;

      /* %g0 is always zero.  */
      for (i = SPARC_G1_REGNUM; i <= SPARC_O7_REGNUM; i++)
	{
	  if (regnum == i || regnum == -1)
	    regcache->raw_collect (i, regs + offset);
	  offset += 8;
	}
    }

  if ((regnum >= SPARC_L0_REGNUM && regnum <= SPARC_I7_REGNUM) || regnum == -1)
    {
      /* Not all of the register set variants include Locals and
	 Inputs.  For those that don't, we read them off the stack.  */
      if (gregmap->r_l0_offset != -1)
	{
	  int offset = gregmap->r_l0_offset;

	  if (sparc32)
	    offset += 4;

	  for (i = SPARC_L0_REGNUM; i <= SPARC_I7_REGNUM; i++)
	    {
	      if (regnum == i || regnum == -1)
		regcache->raw_collect (i, regs + offset);
	      offset += 8;
	    }
	}
    }
}

void
sparc64_supply_fpregset (const struct sparc_fpregmap *fpregmap,
			 struct regcache *regcache,
			 int regnum, const void *fpregs)
{
  int sparc32 = (gdbarch_ptr_bit (regcache->arch ()) == 32);
  const gdb_byte *regs = (const gdb_byte *) fpregs;
  int i;

  for (i = 0; i < 32; i++)
    {
      if (regnum == (SPARC_F0_REGNUM + i) || regnum == -1)
	regcache->raw_supply (SPARC_F0_REGNUM + i,
			      regs + fpregmap->r_f0_offset + (i * 4));
    }

  if (sparc32)
    {
      if (regnum == SPARC32_FSR_REGNUM || regnum == -1)
	regcache->raw_supply (SPARC32_FSR_REGNUM,
			     regs + fpregmap->r_fsr_offset);
    }
  else
    {
      for (i = 0; i < 16; i++)
	{
	  if (regnum == (SPARC64_F32_REGNUM + i) || regnum == -1)
	    regcache->raw_supply
	      (SPARC64_F32_REGNUM + i,
	       regs + fpregmap->r_f0_offset + (32 * 4) + (i * 8));
	}

      if (regnum == SPARC64_FSR_REGNUM || regnum == -1)
	regcache->raw_supply (SPARC64_FSR_REGNUM,
			      regs + fpregmap->r_fsr_offset);
    }
}

void
sparc64_collect_fpregset (const struct sparc_fpregmap *fpregmap,
			  const struct regcache *regcache,
			  int regnum, void *fpregs)
{
  int sparc32 = (gdbarch_ptr_bit (regcache->arch ()) == 32);
  gdb_byte *regs = (gdb_byte *) fpregs;
  int i;

  for (i = 0; i < 32; i++)
    {
      if (regnum == (SPARC_F0_REGNUM + i) || regnum == -1)
	regcache->raw_collect (SPARC_F0_REGNUM + i,
			       regs + fpregmap->r_f0_offset + (i * 4));
    }

  if (sparc32)
    {
      if (regnum == SPARC32_FSR_REGNUM || regnum == -1)
	regcache->raw_collect (SPARC32_FSR_REGNUM,
			       regs + fpregmap->r_fsr_offset);
    }
  else
    {
      for (i = 0; i < 16; i++)
	{
	  if (regnum == (SPARC64_F32_REGNUM + i) || regnum == -1)
	    regcache->raw_collect (SPARC64_F32_REGNUM + i,
				   (regs + fpregmap->r_f0_offset
				    + (32 * 4) + (i * 8)));
	}

      if (regnum == SPARC64_FSR_REGNUM || regnum == -1)
	regcache->raw_collect (SPARC64_FSR_REGNUM,
			       regs + fpregmap->r_fsr_offset);
    }
}

const struct sparc_fpregmap sparc64_bsd_fpregmap =
{
  0 * 8,			/* %f0 */
  32 * 8,			/* %fsr */
};
