/* Target-dependent code for GNU/Linux on s390.

   Copyright (C) 2001-2024 Free Software Foundation, Inc.

   Contributed by D.J. Barrow (djbarrow@de.ibm.com,barrow_dj@yahoo.com)
   for IBM Deutschland Entwicklung GmbH, IBM Corporation.

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
#include "elf/common.h"
#include "frame-base.h"
#include "frame-unwind.h"
#include "gdbarch.h"
#include "gdbcore.h"
#include "linux-record.h"
#include "linux-tdep.h"
#include "objfiles.h"
#include "osabi.h"
#include "regcache.h"
#include "record-full.h"
#include "regset.h"
#include "s390-tdep.h"
#include "s390-linux-tdep.h"
#include "solib-svr4.h"
#include "target.h"
#include "trad-frame.h"
#include "xml-syscall.h"

#include "features/s390-linux32v1.c"
#include "features/s390-linux32v2.c"
#include "features/s390-linux64.c"
#include "features/s390-linux64v1.c"
#include "features/s390-linux64v2.c"
#include "features/s390-te-linux64.c"
#include "features/s390-vx-linux64.c"
#include "features/s390-tevx-linux64.c"
#include "features/s390-gs-linux64.c"
#include "features/s390x-linux64v1.c"
#include "features/s390x-linux64v2.c"
#include "features/s390x-te-linux64.c"
#include "features/s390x-vx-linux64.c"
#include "features/s390x-tevx-linux64.c"
#include "features/s390x-gs-linux64.c"

#define XML_SYSCALL_FILENAME_S390 "syscalls/s390-linux.xml"
#define XML_SYSCALL_FILENAME_S390X "syscalls/s390x-linux.xml"


/* Register handling.  */

/* Implement cannot_store_register gdbarch method.  */

static int
s390_cannot_store_register (struct gdbarch *gdbarch, int regnum)
{
  /* The last-break address is read-only.  */
  return regnum == S390_LAST_BREAK_REGNUM;
}

/* Implement write_pc gdbarch method.  */

static void
s390_write_pc (struct regcache *regcache, CORE_ADDR pc)
{
  struct gdbarch *gdbarch = regcache->arch ();
  s390_gdbarch_tdep *tdep = gdbarch_tdep<s390_gdbarch_tdep> (gdbarch);

  regcache_cooked_write_unsigned (regcache, tdep->pc_regnum, pc);

  /* Set special SYSTEM_CALL register to 0 to prevent the kernel from
     messing with the PC we just installed, if we happen to be within
     an interrupted system call that the kernel wants to restart.

     Note that after we return from the dummy call, the SYSTEM_CALL and
     ORIG_R2 registers will be automatically restored, and the kernel
     continues to restart the system call at this point.  */
  if (register_size (gdbarch, S390_SYSTEM_CALL_REGNUM) > 0)
    regcache_cooked_write_unsigned (regcache, S390_SYSTEM_CALL_REGNUM, 0);
}

/* Maps for register sets.  */

static const struct regcache_map_entry s390_gregmap[] =
  {
    { 1, S390_PSWM_REGNUM },
    { 1, S390_PSWA_REGNUM },
    { 16, S390_R0_REGNUM },
    { 16, S390_A0_REGNUM },
    { 1, S390_ORIG_R2_REGNUM },
    { 0 }
  };

static const struct regcache_map_entry s390_fpregmap[] =
  {
    { 1, S390_FPC_REGNUM, 8 },
    { 16, S390_F0_REGNUM, 8 },
    { 0 }
  };

static const struct regcache_map_entry s390_regmap_upper[] =
  {
    { 16, S390_R0_UPPER_REGNUM, 4 },
    { 0 }
  };

static const struct regcache_map_entry s390_regmap_last_break[] =
  {
    { 1, REGCACHE_MAP_SKIP, 4 },
    { 1, S390_LAST_BREAK_REGNUM, 4 },
    { 0 }
  };

static const struct regcache_map_entry s390x_regmap_last_break[] =
  {
    { 1, S390_LAST_BREAK_REGNUM, 8 },
    { 0 }
  };

static const struct regcache_map_entry s390_regmap_system_call[] =
  {
    { 1, S390_SYSTEM_CALL_REGNUM, 4 },
    { 0 }
  };

static const struct regcache_map_entry s390_regmap_tdb[] =
  {
    { 1, S390_TDB_DWORD0_REGNUM, 8 },
    { 1, S390_TDB_ABORT_CODE_REGNUM, 8 },
    { 1, S390_TDB_CONFLICT_TOKEN_REGNUM, 8 },
    { 1, S390_TDB_ATIA_REGNUM, 8 },
    { 12, REGCACHE_MAP_SKIP, 8 },
    { 16, S390_TDB_R0_REGNUM, 8 },
    { 0 }
  };

static const struct regcache_map_entry s390_regmap_vxrs_low[] =
  {
    { 16, S390_V0_LOWER_REGNUM, 8 },
    { 0 }
  };

static const struct regcache_map_entry s390_regmap_vxrs_high[] =
  {
    { 16, S390_V16_REGNUM, 16 },
    { 0 }
  };

static const struct regcache_map_entry s390_regmap_gs[] =
  {
    { 1, REGCACHE_MAP_SKIP, 8 },
    { 1, S390_GSD_REGNUM, 8 },
    { 1, S390_GSSM_REGNUM, 8 },
    { 1, S390_GSEPLA_REGNUM, 8 },
    { 0 }
  };

static const struct regcache_map_entry s390_regmap_gsbc[] =
  {
    { 1, REGCACHE_MAP_SKIP, 8 },
    { 1, S390_BC_GSD_REGNUM, 8 },
    { 1, S390_BC_GSSM_REGNUM, 8 },
    { 1, S390_BC_GSEPLA_REGNUM, 8 },
    { 0 }
  };

/* Supply the TDB regset.  Like regcache_supply_regset, but invalidate
   the TDB registers unless the TDB format field is valid.  */

static void
s390_supply_tdb_regset (const struct regset *regset, struct regcache *regcache,
		    int regnum, const void *regs, size_t len)
{
  ULONGEST tdw;
  enum register_status ret;

  regcache_supply_regset (regset, regcache, regnum, regs, len);
  ret = regcache_cooked_read_unsigned (regcache, S390_TDB_DWORD0_REGNUM, &tdw);
  if (ret != REG_VALID || (tdw >> 56) != 1)
    regcache_supply_regset (regset, regcache, regnum, NULL, len);
}

const struct regset s390_gregset = {
  s390_gregmap,
  regcache_supply_regset,
  regcache_collect_regset
};

const struct regset s390_fpregset = {
  s390_fpregmap,
  regcache_supply_regset,
  regcache_collect_regset
};

static const struct regset s390_upper_regset = {
  s390_regmap_upper,
  regcache_supply_regset,
  regcache_collect_regset
};

const struct regset s390_last_break_regset = {
  s390_regmap_last_break,
  regcache_supply_regset,
  regcache_collect_regset
};

const struct regset s390x_last_break_regset = {
  s390x_regmap_last_break,
  regcache_supply_regset,
  regcache_collect_regset
};

const struct regset s390_system_call_regset = {
  s390_regmap_system_call,
  regcache_supply_regset,
  regcache_collect_regset
};

const struct regset s390_tdb_regset = {
  s390_regmap_tdb,
  s390_supply_tdb_regset,
  regcache_collect_regset
};

const struct regset s390_vxrs_low_regset = {
  s390_regmap_vxrs_low,
  regcache_supply_regset,
  regcache_collect_regset
};

const struct regset s390_vxrs_high_regset = {
  s390_regmap_vxrs_high,
  regcache_supply_regset,
  regcache_collect_regset
};

const struct regset s390_gs_regset = {
  s390_regmap_gs,
  regcache_supply_regset,
  regcache_collect_regset
};

const struct regset s390_gsbc_regset = {
  s390_regmap_gsbc,
  regcache_supply_regset,
  regcache_collect_regset
};

/* Iterate over supported core file register note sections. */

static void
s390_iterate_over_regset_sections (struct gdbarch *gdbarch,
				   iterate_over_regset_sections_cb *cb,
				   void *cb_data,
				   const struct regcache *regcache)
{
  s390_gdbarch_tdep *tdep = gdbarch_tdep<s390_gdbarch_tdep> (gdbarch);
  const int gregset_size = (tdep->abi == ABI_LINUX_S390 ?
			    s390_sizeof_gregset : s390x_sizeof_gregset);

  cb (".reg", gregset_size, gregset_size, &s390_gregset, NULL, cb_data);
  cb (".reg2", s390_sizeof_fpregset, s390_sizeof_fpregset, &s390_fpregset, NULL,
      cb_data);

  if (tdep->abi == ABI_LINUX_S390 && tdep->gpr_full_regnum != -1)
    cb (".reg-s390-high-gprs", 16 * 4, 16 * 4, &s390_upper_regset,
	"s390 GPR upper halves", cb_data);

  if (tdep->have_linux_v1)
    cb (".reg-s390-last-break", 8, 8,
	(gdbarch_ptr_bit (gdbarch) == 32
	 ? &s390_last_break_regset : &s390x_last_break_regset),
	"s390 last-break address", cb_data);

  if (tdep->have_linux_v2)
    cb (".reg-s390-system-call", 4, 4, &s390_system_call_regset,
	"s390 system-call", cb_data);

  /* If regcache is set, we are in "write" (gcore) mode.  In this
     case, don't iterate over the TDB unless its registers are
     available.  */
  if (tdep->have_tdb
      && (regcache == NULL
	  || (REG_VALID
	      == regcache->get_register_status (S390_TDB_DWORD0_REGNUM))))
    cb (".reg-s390-tdb", s390_sizeof_tdbregset, s390_sizeof_tdbregset,
	&s390_tdb_regset, "s390 TDB", cb_data);

  if (tdep->v0_full_regnum != -1)
    {
      cb (".reg-s390-vxrs-low", 16 * 8, 16 * 8, &s390_vxrs_low_regset,
	  "s390 vector registers 0-15 lower half", cb_data);
      cb (".reg-s390-vxrs-high", 16 * 16, 16 * 16, &s390_vxrs_high_regset,
	  "s390 vector registers 16-31", cb_data);
    }

  /* Iterate over the guarded-storage regsets if in "read" mode, or if
     their registers are available.  */
  if (tdep->have_gs)
    {
      if (regcache == NULL
	  || REG_VALID == regcache->get_register_status (S390_GSD_REGNUM))
	cb (".reg-s390-gs-cb", 4 * 8, 4 * 8, &s390_gs_regset,
	    "s390 guarded-storage registers", cb_data);

      if (regcache == NULL
	  || REG_VALID == regcache->get_register_status (S390_BC_GSD_REGNUM))
	cb (".reg-s390-gs-bc", 4 * 8, 4 * 8, &s390_gsbc_regset,
	    "s390 guarded-storage broadcast control", cb_data);
    }
}

/* Implement core_read_description gdbarch method.  */

static const struct target_desc *
s390_core_read_description (struct gdbarch *gdbarch,
			    struct target_ops *target, bfd *abfd)
{
  asection *section = bfd_get_section_by_name (abfd, ".reg");
  std::optional<gdb::byte_vector> auxv = target_read_auxv_raw (target);
  CORE_ADDR hwcap = linux_get_hwcap (auxv, target, gdbarch);
  bool high_gprs, v1, v2, te, vx, gs;

  if (!section)
    return NULL;

  high_gprs = (bfd_get_section_by_name (abfd, ".reg-s390-high-gprs")
	       != NULL);
  v1 = (bfd_get_section_by_name (abfd, ".reg-s390-last-break") != NULL);
  v2 = (bfd_get_section_by_name (abfd, ".reg-s390-system-call") != NULL);
  vx = (hwcap & HWCAP_S390_VX);
  te = (hwcap & HWCAP_S390_TE);
  gs = (hwcap & HWCAP_S390_GS);

  switch (bfd_section_size (section))
    {
    case s390_sizeof_gregset:
      if (high_gprs)
	return (gs ? tdesc_s390_gs_linux64 :
		te && vx ? tdesc_s390_tevx_linux64 :
		vx ? tdesc_s390_vx_linux64 :
		te ? tdesc_s390_te_linux64 :
		v2 ? tdesc_s390_linux64v2 :
		v1 ? tdesc_s390_linux64v1 : tdesc_s390_linux64);
      else
	return (v2 ? tdesc_s390_linux32v2 :
		v1 ? tdesc_s390_linux32v1 : tdesc_s390_linux32);

    case s390x_sizeof_gregset:
      return (gs ? tdesc_s390x_gs_linux64 :
	      te && vx ? tdesc_s390x_tevx_linux64 :
	      vx ? tdesc_s390x_vx_linux64 :
	      te ? tdesc_s390x_te_linux64 :
	      v2 ? tdesc_s390x_linux64v2 :
	      v1 ? tdesc_s390x_linux64v1 : tdesc_s390x_linux64);

    default:
      return NULL;
    }
}

/* Frame unwinding. */

/* Signal trampoline stack frames.  */

struct s390_sigtramp_unwind_cache {
  CORE_ADDR frame_base;
  trad_frame_saved_reg *saved_regs;
};

/* Unwind THIS_FRAME and return the corresponding unwind cache for
   s390_sigtramp_frame_unwind.  */

static struct s390_sigtramp_unwind_cache *
s390_sigtramp_frame_unwind_cache (frame_info_ptr this_frame,
				  void **this_prologue_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  s390_gdbarch_tdep *tdep = gdbarch_tdep<s390_gdbarch_tdep> (gdbarch);
  int word_size = gdbarch_ptr_bit (gdbarch) / 8;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  struct s390_sigtramp_unwind_cache *info;
  ULONGEST this_sp, prev_sp;
  CORE_ADDR next_ra, next_cfa, sigreg_ptr, sigreg_high_off;
  int i;

  if (*this_prologue_cache)
    return (struct s390_sigtramp_unwind_cache *) *this_prologue_cache;

  info = FRAME_OBSTACK_ZALLOC (struct s390_sigtramp_unwind_cache);
  *this_prologue_cache = info;
  info->saved_regs = trad_frame_alloc_saved_regs (this_frame);

  this_sp = get_frame_register_unsigned (this_frame, S390_SP_REGNUM);
  next_ra = get_frame_pc (this_frame);
  next_cfa = this_sp + 16*word_size + 32;

  /* New-style RT frame:
	retcode + alignment (8 bytes)
	siginfo (128 bytes)
	ucontext (contains sigregs at offset 5 words).  */
  if (next_ra == next_cfa)
    {
      sigreg_ptr = next_cfa + 8 + 128 + align_up (5*word_size, 8);
      /* sigregs are followed by uc_sigmask (8 bytes), then by the
	 upper GPR halves if present.  */
      sigreg_high_off = 8;
    }

  /* Old-style RT frame and all non-RT frames:
	old signal mask (8 bytes)
	pointer to sigregs.  */
  else
    {
      sigreg_ptr = read_memory_unsigned_integer (next_cfa + 8,
						 word_size, byte_order);
      /* sigregs are followed by signo (4 bytes), then by the
	 upper GPR halves if present.  */
      sigreg_high_off = 4;
    }

  /* The sigregs structure looks like this:
	    long   psw_mask;
	    long   psw_addr;
	    long   gprs[16];
	    int    acrs[16];
	    int    fpc;
	    int    __pad;
	    double fprs[16];  */

  /* PSW mask and address.  */
  info->saved_regs[S390_PSWM_REGNUM].set_addr (sigreg_ptr);
  sigreg_ptr += word_size;
  info->saved_regs[S390_PSWA_REGNUM].set_addr (sigreg_ptr);
  sigreg_ptr += word_size;

  /* Then the GPRs.  */
  for (i = 0; i < 16; i++)
    {
      info->saved_regs[S390_R0_REGNUM + i].set_addr (sigreg_ptr);
      sigreg_ptr += word_size;
    }

  /* Then the ACRs.  */
  for (i = 0; i < 16; i++)
    {
      info->saved_regs[S390_A0_REGNUM + i].set_addr (sigreg_ptr);
      sigreg_ptr += 4;
    }

  /* The floating-point control word.  */
  info->saved_regs[S390_FPC_REGNUM].set_addr (sigreg_ptr);
  sigreg_ptr += 8;

  /* And finally the FPRs.  */
  for (i = 0; i < 16; i++)
    {
      info->saved_regs[S390_F0_REGNUM + i].set_addr (sigreg_ptr);
      sigreg_ptr += 8;
    }

  /* If we have them, the GPR upper halves are appended at the end.  */
  sigreg_ptr += sigreg_high_off;
  if (tdep->gpr_full_regnum != -1)
    for (i = 0; i < 16; i++)
      {
	info->saved_regs[S390_R0_UPPER_REGNUM + i].set_addr (sigreg_ptr);
	sigreg_ptr += 4;
      }

  /* Restore the previous frame's SP.  */
  prev_sp = read_memory_unsigned_integer (
			info->saved_regs[S390_SP_REGNUM].addr (),
			word_size, byte_order);

  /* Determine our frame base.  */
  info->frame_base = prev_sp + 16*word_size + 32;

  return info;
}

/* Implement this_id frame_unwind method for s390_sigtramp_frame_unwind.  */

static void
s390_sigtramp_frame_this_id (frame_info_ptr this_frame,
			     void **this_prologue_cache,
			     struct frame_id *this_id)
{
  struct s390_sigtramp_unwind_cache *info
    = s390_sigtramp_frame_unwind_cache (this_frame, this_prologue_cache);
  *this_id = frame_id_build (info->frame_base, get_frame_pc (this_frame));
}

/* Implement prev_register frame_unwind method for sigtramp frames.  */

static struct value *
s390_sigtramp_frame_prev_register (frame_info_ptr this_frame,
				   void **this_prologue_cache, int regnum)
{
  struct s390_sigtramp_unwind_cache *info
    = s390_sigtramp_frame_unwind_cache (this_frame, this_prologue_cache);
  return s390_trad_frame_prev_register (this_frame, info->saved_regs, regnum);
}

/* Implement sniffer frame_unwind method for sigtramp frames.  */

static int
s390_sigtramp_frame_sniffer (const struct frame_unwind *self,
			     frame_info_ptr this_frame,
			     void **this_prologue_cache)
{
  CORE_ADDR pc = get_frame_pc (this_frame);
  bfd_byte sigreturn[2];

  if (target_read_memory (pc, sigreturn, 2))
    return 0;

  if (sigreturn[0] != op_svc)
    return 0;

  if (sigreturn[1] != 119 /* sigreturn */
      && sigreturn[1] != 173 /* rt_sigreturn */)
    return 0;

  return 1;
}

/* S390 sigtramp frame unwinder.  */

static const struct frame_unwind s390_sigtramp_frame_unwind = {
  "s390 linux sigtramp",
  SIGTRAMP_FRAME,
  default_frame_unwind_stop_reason,
  s390_sigtramp_frame_this_id,
  s390_sigtramp_frame_prev_register,
  NULL,
  s390_sigtramp_frame_sniffer
};

/* Syscall handling.  */

/* Retrieve the syscall number at a ptrace syscall-stop.  Return -1
   upon error. */

static LONGEST
s390_linux_get_syscall_number (struct gdbarch *gdbarch,
			       thread_info *thread)
{
  struct regcache *regs = get_thread_regcache (thread);
  s390_gdbarch_tdep *tdep = gdbarch_tdep<s390_gdbarch_tdep> (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  ULONGEST pc;
  ULONGEST svc_number = -1;
  unsigned opcode;

  /* Assume that the PC points after the 2-byte SVC instruction.  We
     don't currently support SVC via EXECUTE. */
  regcache_cooked_read_unsigned (regs, tdep->pc_regnum, &pc);
  pc -= 2;

  ULONGEST val;
  if (!safe_read_memory_unsigned_integer ((CORE_ADDR) pc, 1, byte_order,
					  &val))
    return -1;
  opcode = val;

  if (opcode != op_svc)
    return -1;

  if (!safe_read_memory_unsigned_integer ((CORE_ADDR) pc + 1, 1, byte_order,
					  &val))
    return -1;
  svc_number = val;

  if (svc_number == 0)
    regcache_cooked_read_unsigned (regs, S390_R1_REGNUM, &svc_number);

  return svc_number;
}

/* Process record-replay */

static struct linux_record_tdep s390_linux_record_tdep;
static struct linux_record_tdep s390x_linux_record_tdep;

/* Record all registers but PC register for process-record.  */

static int
s390_all_but_pc_registers_record (struct regcache *regcache)
{
  struct gdbarch *gdbarch = regcache->arch ();
  s390_gdbarch_tdep *tdep = gdbarch_tdep<s390_gdbarch_tdep> (gdbarch);
  int i;

  for (i = 0; i < 16; i++)
    {
      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + i))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_A0_REGNUM + i))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + i))
	return -1;
      if (tdep->gpr_full_regnum != -1)
	if (record_full_arch_list_add_reg (regcache, S390_R0_UPPER_REGNUM + i))
	  return -1;
      if (tdep->v0_full_regnum != -1)
	{
	  if (record_full_arch_list_add_reg (regcache, S390_V0_LOWER_REGNUM + i))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_V16_REGNUM + i))
	    return -1;
	}
    }
  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
    return -1;
  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
    return -1;

  return 0;
}

/* Canonicalize system call SYSCALL belonging to ABI.  Helper for
   s390_linux_syscall_record.  */

static enum gdb_syscall
s390_canonicalize_syscall (int syscall, enum s390_abi_kind abi)
{
  switch (syscall)
    {
    /* s390 syscall numbers < 222 are mostly the same as x86, so just list
       the exceptions.  */
    case 0:
      return gdb_sys_no_syscall;
    case 7:
      return gdb_sys_restart_syscall;
    /* These syscalls work only on 31-bit.  */
    case 13: /* time */
    case 16: /* lchown[16] */
    case 23: /* setuid[16] */
    case 24: /* getuid[16] */
    case 25: /* stime */
    case 46: /* setgid[16] */
    case 47: /* getgid[16] */
    case 49: /* seteuid[16] */
    case 50: /* getegid[16] */
    case 70: /* setreuid[16] */
    case 71: /* setregid[16] */
    case 76: /* [old_]getrlimit */
    case 80: /* getgroups[16] */
    case 81: /* setgroups[16] */
    case 95: /* fchown[16] */
    case 101: /* ioperm */
    case 138: /* setfsuid[16] */
    case 139: /* setfsgid[16] */
    case 140: /* _llseek */
    case 164: /* setresuid[16] */
    case 165: /* getresuid[16] */
    case 170: /* setresgid[16] */
    case 171: /* getresgid[16] */
    case 182: /* chown[16] */
    case 192: /* mmap2 */
    case 193: /* truncate64 */
    case 194: /* ftruncate64 */
    case 195: /* stat64 */
    case 196: /* lstat64 */
    case 197: /* fstat64 */
    case 221: /* fcntl64 */
      if (abi == ABI_LINUX_S390)
	return (enum gdb_syscall) syscall;
      return gdb_sys_no_syscall;
    /* These syscalls don't exist on s390.  */
    case 17: /* break */
    case 18: /* oldstat */
    case 28: /* oldfstat */
    case 31: /* stty */
    case 32: /* gtty */
    case 35: /* ftime */
    case 44: /* prof */
    case 53: /* lock */
    case 56: /* mpx */
    case 58: /* ulimit */
    case 59: /* oldolduname */
    case 68: /* sgetmask */
    case 69: /* ssetmask */
    case 82: /* [old_]select */
    case 84: /* oldlstat */
    case 98: /* profil */
    case 109: /* olduname */
    case 113: /* vm86old */
    case 123: /* modify_ldt */
    case 166: /* vm86 */
      return gdb_sys_no_syscall;
    case 110:
      return gdb_sys_lookup_dcookie;
    /* Here come the differences.  */
    case 222:
      return gdb_sys_readahead;
    case 223:
      if (abi == ABI_LINUX_S390)
	return gdb_sys_sendfile64;
      return gdb_sys_no_syscall;
    /* 224-235 handled below */
    case 236:
      return gdb_sys_gettid;
    case 237:
      return gdb_sys_tkill;
    case 238:
      return gdb_sys_futex;
    case 239:
      return gdb_sys_sched_setaffinity;
    case 240:
      return gdb_sys_sched_getaffinity;
    case 241:
      return gdb_sys_tgkill;
    /* 242 reserved */
    case 243:
      return gdb_sys_io_setup;
    case 244:
      return gdb_sys_io_destroy;
    case 245:
      return gdb_sys_io_getevents;
    case 246:
      return gdb_sys_io_submit;
    case 247:
      return gdb_sys_io_cancel;
    case 248:
      return gdb_sys_exit_group;
    case 249:
      return gdb_sys_epoll_create;
    case 250:
      return gdb_sys_epoll_ctl;
    case 251:
      return gdb_sys_epoll_wait;
    case 252:
      return gdb_sys_set_tid_address;
    case 253:
      return gdb_sys_fadvise64;
    /* 254-262 handled below */
    /* 263 reserved */
    case 264:
      if (abi == ABI_LINUX_S390)
	return gdb_sys_fadvise64_64;
      return gdb_sys_no_syscall;
    case 265:
      return gdb_sys_statfs64;
    case 266:
      return gdb_sys_fstatfs64;
    case 267:
      return gdb_sys_remap_file_pages;
    /* 268-270 reserved */
    /* 271-277 handled below */
    case 278:
      return gdb_sys_add_key;
    case 279:
      return gdb_sys_request_key;
    case 280:
      return gdb_sys_keyctl;
    case 281:
      return gdb_sys_waitid;
    /* 282-312 handled below */
    case 293:
      if (abi == ABI_LINUX_S390)
	return gdb_sys_fstatat64;
      return gdb_sys_newfstatat;
    /* 313+ not yet supported */
    default:
      {
	int ret;

	/* Most "old" syscalls copied from i386.  */
	if (syscall <= 221)
	  ret = syscall;
	/* xattr syscalls.  */
	else if (syscall >= 224 && syscall <= 235)
	  ret = syscall + 2;
	/* timer syscalls.  */
	else if (syscall >= 254 && syscall <= 262)
	  ret = syscall + 5;
	/* mq_* and kexec_load */
	else if (syscall >= 271 && syscall <= 277)
	  ret = syscall + 6;
	/* ioprio_set .. epoll_pwait */
	else if (syscall >= 282 && syscall <= 312)
	  ret = syscall + 7;
	else if (syscall == 349)
	  ret = gdb_sys_getrandom;
	else
	  ret = gdb_sys_no_syscall;

	return (enum gdb_syscall) ret;
      }
    }
}

/* Record a system call.  Returns 0 on success, -1 otherwise.
   Helper function for s390_process_record.  */

static int
s390_linux_syscall_record (struct regcache *regcache, LONGEST syscall_native)
{
  struct gdbarch *gdbarch = regcache->arch ();
  s390_gdbarch_tdep *tdep = gdbarch_tdep<s390_gdbarch_tdep> (gdbarch);
  int ret;
  enum gdb_syscall syscall_gdb;

  /* On s390, syscall number can be passed either as immediate field of svc
     instruction, or in %r1 (with svc 0).  */
  if (syscall_native == 0)
    regcache_raw_read_signed (regcache, S390_R1_REGNUM, &syscall_native);

  syscall_gdb = s390_canonicalize_syscall (syscall_native, tdep->abi);

  if (syscall_gdb < 0)
    {
      gdb_printf (gdb_stderr,
		  _("Process record and replay target doesn't "
		    "support syscall number %s\n"),
		  plongest (syscall_native));
      return -1;
    }

  if (syscall_gdb == gdb_sys_sigreturn
      || syscall_gdb == gdb_sys_rt_sigreturn)
    {
      if (s390_all_but_pc_registers_record (regcache))
	return -1;
      return 0;
    }

  if (tdep->abi == ABI_LINUX_ZSERIES)
    ret = record_linux_system_call (syscall_gdb, regcache,
				    &s390x_linux_record_tdep);
  else
    ret = record_linux_system_call (syscall_gdb, regcache,
				    &s390_linux_record_tdep);

  if (ret)
    return ret;

  /* Record the return value of the system call.  */
  if (record_full_arch_list_add_reg (regcache, S390_R2_REGNUM))
    return -1;

  return 0;
}

/* Implement process_record_signal gdbarch method.  */

static int
s390_linux_record_signal (struct gdbarch *gdbarch, struct regcache *regcache,
			  enum gdb_signal signal)
{
  s390_gdbarch_tdep *tdep = gdbarch_tdep<s390_gdbarch_tdep> (gdbarch);
  /* There are two kinds of signal frames on s390. rt_sigframe is always
     the larger one, so don't even bother with sigframe.  */
  const int sizeof_rt_sigframe = (tdep->abi == ABI_LINUX_ZSERIES ?
				  160 + 8 + 128 + 1024 : 96 + 8 + 128 + 1000);
  ULONGEST sp;
  int i;

  for (i = 0; i < 16; i++)
    {
      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + i))
	return -1;
      if (tdep->gpr_full_regnum != -1)
	if (record_full_arch_list_add_reg (regcache, S390_R0_UPPER_REGNUM + i))
	  return -1;
    }
  if (record_full_arch_list_add_reg (regcache, S390_PSWA_REGNUM))
    return -1;
  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
    return -1;

  /* Record the change in the stack.
     frame-size = sizeof (struct rt_sigframe) + SIGNAL_FRAMESIZE  */
  regcache_raw_read_unsigned (regcache, S390_SP_REGNUM, &sp);
  sp -= sizeof_rt_sigframe;

  if (record_full_arch_list_add_mem (sp, sizeof_rt_sigframe))
    return -1;

  if (record_full_arch_list_add_end ())
    return -1;

  return 0;
}

/* Initialize linux_record_tdep if not initialized yet.  */

static void
s390_init_linux_record_tdep (struct linux_record_tdep *record_tdep,
			     enum s390_abi_kind abi)
{
  /* These values are the size of the type that will be used in a system
     call.  They are obtained from Linux Kernel source.  */

  if (abi == ABI_LINUX_ZSERIES)
    {
      record_tdep->size_pointer = 8;
      /* no _old_kernel_stat */
      record_tdep->size_tms = 32;
      record_tdep->size_loff_t = 8;
      record_tdep->size_flock = 32;
      record_tdep->size_ustat = 32;
      record_tdep->size_old_sigaction = 32;
      record_tdep->size_old_sigset_t = 8;
      record_tdep->size_rlimit = 16;
      record_tdep->size_rusage = 144;
      record_tdep->size_timeval = 16;
      record_tdep->size_timezone = 8;
      /* old_[ug]id_t never used */
      record_tdep->size_fd_set = 128;
      record_tdep->size_old_dirent = 280;
      record_tdep->size_statfs = 88;
      record_tdep->size_statfs64 = 88;
      record_tdep->size_sockaddr = 16;
      record_tdep->size_int = 4;
      record_tdep->size_long = 8;
      record_tdep->size_ulong = 8;
      record_tdep->size_msghdr = 56;
      record_tdep->size_itimerval = 32;
      record_tdep->size_stat = 144;
      /* old_utsname unused */
      record_tdep->size_sysinfo = 112;
      record_tdep->size_msqid_ds = 120;
      record_tdep->size_shmid_ds = 112;
      record_tdep->size_new_utsname = 390;
      record_tdep->size_timex = 208;
      record_tdep->size_mem_dqinfo = 24;
      record_tdep->size_if_dqblk = 72;
      record_tdep->size_fs_quota_stat = 80;
      record_tdep->size_timespec = 16;
      record_tdep->size_pollfd = 8;
      record_tdep->size_NFS_FHSIZE = 32;
      record_tdep->size_knfsd_fh = 132;
      record_tdep->size_TASK_COMM_LEN = 16;
      record_tdep->size_sigaction = 32;
      record_tdep->size_sigset_t = 8;
      record_tdep->size_siginfo_t = 128;
      record_tdep->size_cap_user_data_t = 12;
      record_tdep->size_stack_t = 24;
      record_tdep->size_off_t = 8;
      /* stat64 unused */
      record_tdep->size_gid_t = 4;
      record_tdep->size_uid_t = 4;
      record_tdep->size_PAGE_SIZE = 0x1000;        /* 4KB */
      record_tdep->size_flock64 = 32;
      record_tdep->size_io_event = 32;
      record_tdep->size_iocb = 64;
      record_tdep->size_epoll_event = 16;
      record_tdep->size_itimerspec = 32;
      record_tdep->size_mq_attr = 64;
      record_tdep->size_termios = 36;
      record_tdep->size_termios2 = 44;
      record_tdep->size_pid_t = 4;
      record_tdep->size_winsize = 8;
      record_tdep->size_serial_struct = 72;
      record_tdep->size_serial_icounter_struct = 80;
      record_tdep->size_size_t = 8;
      record_tdep->size_iovec = 16;
      record_tdep->size_time_t = 8;
    }
  else if (abi == ABI_LINUX_S390)
    {
      record_tdep->size_pointer = 4;
      record_tdep->size__old_kernel_stat = 32;
      record_tdep->size_tms = 16;
      record_tdep->size_loff_t = 8;
      record_tdep->size_flock = 16;
      record_tdep->size_ustat = 20;
      record_tdep->size_old_sigaction = 16;
      record_tdep->size_old_sigset_t = 4;
      record_tdep->size_rlimit = 8;
      record_tdep->size_rusage = 72;
      record_tdep->size_timeval = 8;
      record_tdep->size_timezone = 8;
      record_tdep->size_old_gid_t = 2;
      record_tdep->size_old_uid_t = 2;
      record_tdep->size_fd_set = 128;
      record_tdep->size_old_dirent = 268;
      record_tdep->size_statfs = 64;
      record_tdep->size_statfs64 = 88;
      record_tdep->size_sockaddr = 16;
      record_tdep->size_int = 4;
      record_tdep->size_long = 4;
      record_tdep->size_ulong = 4;
      record_tdep->size_msghdr = 28;
      record_tdep->size_itimerval = 16;
      record_tdep->size_stat = 64;
      /* old_utsname unused */
      record_tdep->size_sysinfo = 64;
      record_tdep->size_msqid_ds = 88;
      record_tdep->size_shmid_ds = 84;
      record_tdep->size_new_utsname = 390;
      record_tdep->size_timex = 128;
      record_tdep->size_mem_dqinfo = 24;
      record_tdep->size_if_dqblk = 72;
      record_tdep->size_fs_quota_stat = 80;
      record_tdep->size_timespec = 8;
      record_tdep->size_pollfd = 8;
      record_tdep->size_NFS_FHSIZE = 32;
      record_tdep->size_knfsd_fh = 132;
      record_tdep->size_TASK_COMM_LEN = 16;
      record_tdep->size_sigaction = 20;
      record_tdep->size_sigset_t = 8;
      record_tdep->size_siginfo_t = 128;
      record_tdep->size_cap_user_data_t = 12;
      record_tdep->size_stack_t = 12;
      record_tdep->size_off_t = 4;
      record_tdep->size_stat64 = 104;
      record_tdep->size_gid_t = 4;
      record_tdep->size_uid_t = 4;
      record_tdep->size_PAGE_SIZE = 0x1000;        /* 4KB */
      record_tdep->size_flock64 = 32;
      record_tdep->size_io_event = 32;
      record_tdep->size_iocb = 64;
      record_tdep->size_epoll_event = 16;
      record_tdep->size_itimerspec = 16;
      record_tdep->size_mq_attr = 32;
      record_tdep->size_termios = 36;
      record_tdep->size_termios2 = 44;
      record_tdep->size_pid_t = 4;
      record_tdep->size_winsize = 8;
      record_tdep->size_serial_struct = 60;
      record_tdep->size_serial_icounter_struct = 80;
      record_tdep->size_size_t = 4;
      record_tdep->size_iovec = 8;
      record_tdep->size_time_t = 4;
    }

  /* These values are the second argument of system call "sys_fcntl"
     and "sys_fcntl64".  They are obtained from Linux Kernel source.  */
  record_tdep->fcntl_F_GETLK = 5;
  record_tdep->fcntl_F_GETLK64 = 12;
  record_tdep->fcntl_F_SETLK64 = 13;
  record_tdep->fcntl_F_SETLKW64 = 14;

  record_tdep->arg1 = S390_R2_REGNUM;
  record_tdep->arg2 = S390_R3_REGNUM;
  record_tdep->arg3 = S390_R4_REGNUM;
  record_tdep->arg4 = S390_R5_REGNUM;
  record_tdep->arg5 = S390_R6_REGNUM;

  /* These values are the second argument of system call "sys_ioctl".
     They are obtained from Linux Kernel source.
     See arch/s390/include/uapi/asm/ioctls.h.  */

  record_tdep->ioctl_TCGETS = 0x5401;
  record_tdep->ioctl_TCSETS = 0x5402;
  record_tdep->ioctl_TCSETSW = 0x5403;
  record_tdep->ioctl_TCSETSF = 0x5404;
  record_tdep->ioctl_TCGETA = 0x5405;
  record_tdep->ioctl_TCSETA = 0x5406;
  record_tdep->ioctl_TCSETAW = 0x5407;
  record_tdep->ioctl_TCSETAF = 0x5408;
  record_tdep->ioctl_TCSBRK = 0x5409;
  record_tdep->ioctl_TCXONC = 0x540a;
  record_tdep->ioctl_TCFLSH = 0x540b;
  record_tdep->ioctl_TIOCEXCL = 0x540c;
  record_tdep->ioctl_TIOCNXCL = 0x540d;
  record_tdep->ioctl_TIOCSCTTY = 0x540e;
  record_tdep->ioctl_TIOCGPGRP = 0x540f;
  record_tdep->ioctl_TIOCSPGRP = 0x5410;
  record_tdep->ioctl_TIOCOUTQ = 0x5411;
  record_tdep->ioctl_TIOCSTI = 0x5412;
  record_tdep->ioctl_TIOCGWINSZ = 0x5413;
  record_tdep->ioctl_TIOCSWINSZ = 0x5414;
  record_tdep->ioctl_TIOCMGET = 0x5415;
  record_tdep->ioctl_TIOCMBIS = 0x5416;
  record_tdep->ioctl_TIOCMBIC = 0x5417;
  record_tdep->ioctl_TIOCMSET = 0x5418;
  record_tdep->ioctl_TIOCGSOFTCAR = 0x5419;
  record_tdep->ioctl_TIOCSSOFTCAR = 0x541a;
  record_tdep->ioctl_FIONREAD = 0x541b;
  record_tdep->ioctl_TIOCINQ = 0x541b; /* alias */
  record_tdep->ioctl_TIOCLINUX = 0x541c;
  record_tdep->ioctl_TIOCCONS = 0x541d;
  record_tdep->ioctl_TIOCGSERIAL = 0x541e;
  record_tdep->ioctl_TIOCSSERIAL = 0x541f;
  record_tdep->ioctl_TIOCPKT = 0x5420;
  record_tdep->ioctl_FIONBIO = 0x5421;
  record_tdep->ioctl_TIOCNOTTY = 0x5422;
  record_tdep->ioctl_TIOCSETD = 0x5423;
  record_tdep->ioctl_TIOCGETD = 0x5424;
  record_tdep->ioctl_TCSBRKP = 0x5425;
  record_tdep->ioctl_TIOCSBRK = 0x5427;
  record_tdep->ioctl_TIOCCBRK = 0x5428;
  record_tdep->ioctl_TIOCGSID = 0x5429;
  record_tdep->ioctl_TCGETS2 = 0x802c542a;
  record_tdep->ioctl_TCSETS2 = 0x402c542b;
  record_tdep->ioctl_TCSETSW2 = 0x402c542c;
  record_tdep->ioctl_TCSETSF2 = 0x402c542d;
  record_tdep->ioctl_TIOCGPTN = 0x80045430;
  record_tdep->ioctl_TIOCSPTLCK = 0x40045431;
  record_tdep->ioctl_FIONCLEX = 0x5450;
  record_tdep->ioctl_FIOCLEX = 0x5451;
  record_tdep->ioctl_FIOASYNC = 0x5452;
  record_tdep->ioctl_TIOCSERCONFIG = 0x5453;
  record_tdep->ioctl_TIOCSERGWILD = 0x5454;
  record_tdep->ioctl_TIOCSERSWILD = 0x5455;
  record_tdep->ioctl_TIOCGLCKTRMIOS = 0x5456;
  record_tdep->ioctl_TIOCSLCKTRMIOS = 0x5457;
  record_tdep->ioctl_TIOCSERGSTRUCT = 0x5458;
  record_tdep->ioctl_TIOCSERGETLSR = 0x5459;
  record_tdep->ioctl_TIOCSERGETMULTI = 0x545a;
  record_tdep->ioctl_TIOCSERSETMULTI = 0x545b;
  record_tdep->ioctl_TIOCMIWAIT = 0x545c;
  record_tdep->ioctl_TIOCGICOUNT = 0x545d;
  record_tdep->ioctl_FIOQSIZE = 0x545e;
}

/* Initialize OSABI common for GNU/Linux on 31- and 64-bit systems.  */

static void
s390_linux_init_abi_any (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  s390_gdbarch_tdep *tdep = gdbarch_tdep<s390_gdbarch_tdep> (gdbarch);

  tdep->s390_syscall_record = s390_linux_syscall_record;

  linux_init_abi (info, gdbarch, 1);

  /* Register handling.  */
  set_gdbarch_core_read_description (gdbarch, s390_core_read_description);
  set_gdbarch_iterate_over_regset_sections (gdbarch,
					    s390_iterate_over_regset_sections);
  set_gdbarch_write_pc (gdbarch, s390_write_pc);
  set_gdbarch_cannot_store_register (gdbarch, s390_cannot_store_register);

  /* Syscall handling.  */
  set_gdbarch_get_syscall_number (gdbarch, s390_linux_get_syscall_number);

  /* Frame handling.  */
  frame_unwind_append_unwinder (gdbarch, &s390_sigtramp_frame_unwind);
  set_gdbarch_skip_trampoline_code (gdbarch, find_solib_trampoline_target);

  /* Enable TLS support.  */
  set_gdbarch_fetch_tls_load_module_address (gdbarch,
					     svr4_fetch_objfile_link_map);

  /* Support reverse debugging.  */
  set_gdbarch_process_record_signal (gdbarch, s390_linux_record_signal);
  s390_init_linux_record_tdep (&s390_linux_record_tdep, ABI_LINUX_S390);
  s390_init_linux_record_tdep (&s390x_linux_record_tdep, ABI_LINUX_ZSERIES);
}

/* Initialize OSABI for GNU/Linux on 31-bit systems.  */

static void
s390_linux_init_abi_31 (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  s390_gdbarch_tdep *tdep = gdbarch_tdep<s390_gdbarch_tdep> (gdbarch);

  tdep->abi = ABI_LINUX_S390;

  s390_linux_init_abi_any (info, gdbarch);

  set_solib_svr4_fetch_link_map_offsets (gdbarch,
					 linux_ilp32_fetch_link_map_offsets);
  set_xml_syscall_file_name (gdbarch, XML_SYSCALL_FILENAME_S390);
}

/* Initialize OSABI for GNU/Linux on 64-bit systems.  */

static void
s390_linux_init_abi_64 (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  s390_gdbarch_tdep *tdep = gdbarch_tdep<s390_gdbarch_tdep> (gdbarch);

  tdep->abi = ABI_LINUX_ZSERIES;

  s390_linux_init_abi_any (info, gdbarch);

  set_solib_svr4_fetch_link_map_offsets (gdbarch,
					 linux_lp64_fetch_link_map_offsets);
  set_xml_syscall_file_name (gdbarch, XML_SYSCALL_FILENAME_S390X);
}

void _initialize_s390_linux_tdep ();
void
_initialize_s390_linux_tdep ()
{
  /* Hook us into the OSABI mechanism.  */
  gdbarch_register_osabi (bfd_arch_s390, bfd_mach_s390_31, GDB_OSABI_LINUX,
			  s390_linux_init_abi_31);
  gdbarch_register_osabi (bfd_arch_s390, bfd_mach_s390_64, GDB_OSABI_LINUX,
			  s390_linux_init_abi_64);

  /* Initialize the GNU/Linux target descriptions.  */
  initialize_tdesc_s390_linux32v1 ();
  initialize_tdesc_s390_linux32v2 ();
  initialize_tdesc_s390_linux64 ();
  initialize_tdesc_s390_linux64v1 ();
  initialize_tdesc_s390_linux64v2 ();
  initialize_tdesc_s390_te_linux64 ();
  initialize_tdesc_s390_vx_linux64 ();
  initialize_tdesc_s390_tevx_linux64 ();
  initialize_tdesc_s390_gs_linux64 ();
  initialize_tdesc_s390x_linux64v1 ();
  initialize_tdesc_s390x_linux64v2 ();
  initialize_tdesc_s390x_te_linux64 ();
  initialize_tdesc_s390x_vx_linux64 ();
  initialize_tdesc_s390x_tevx_linux64 ();
  initialize_tdesc_s390x_gs_linux64 ();
}
