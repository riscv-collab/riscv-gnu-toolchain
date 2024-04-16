/* Motorola m68k target-dependent support for GNU/Linux.

   Copyright (C) 1996-2024 Free Software Foundation, Inc.

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
#include "gdbcore.h"
#include "frame.h"
#include "target.h"
#include "gdbtypes.h"
#include "osabi.h"
#include "regcache.h"
#include "objfiles.h"
#include "symtab.h"
#include "m68k-tdep.h"
#include "trad-frame.h"
#include "frame-unwind.h"
#include "glibc-tdep.h"
#include "solib-svr4.h"
#include "auxv.h"
#include "observable.h"
#include "elf/common.h"
#include "linux-tdep.h"
#include "regset.h"

/* Offsets (in target ints) into jmp_buf.  */

#define M68K_LINUX_JB_ELEMENT_SIZE 4
#define M68K_LINUX_JB_PC 7

/* Check whether insn1 and insn2 are parts of a signal trampoline.  */

#define IS_SIGTRAMP(insn1, insn2)					\
  (/* addaw #20,sp; moveq #119,d0; trap #0 */				\
   (insn1 == 0xdefc0014 && insn2 == 0x70774e40)				\
   /* moveq #119,d0; trap #0 */						\
   || insn1 == 0x70774e40)

#define IS_RT_SIGTRAMP(insn1, insn2)					\
  (/* movel #173,d0; trap #0 */						\
   (insn1 == 0x203c0000 && insn2 == 0x00ad4e40)				\
   /* moveq #82,d0; notb d0; trap #0 */					\
   || (insn1 == 0x70524600 && (insn2 >> 16) == 0x4e40))

/* Return non-zero if THIS_FRAME corresponds to a signal trampoline.  For
   the sake of m68k_linux_get_sigtramp_info we also distinguish between
   non-RT and RT signal trampolines.  */

static int
m68k_linux_pc_in_sigtramp (frame_info_ptr this_frame)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte buf[12];
  unsigned long insn0, insn1, insn2;
  CORE_ADDR pc = get_frame_pc (this_frame);

  if (!safe_frame_unwind_memory (this_frame, pc - 4, {buf, sizeof (buf)}))
    return 0;
  insn1 = extract_unsigned_integer (buf + 4, 4, byte_order);
  insn2 = extract_unsigned_integer (buf + 8, 4, byte_order);
  if (IS_SIGTRAMP (insn1, insn2))
    return 1;
  if (IS_RT_SIGTRAMP (insn1, insn2))
    return 2;

  insn0 = extract_unsigned_integer (buf, 4, byte_order);
  if (IS_SIGTRAMP (insn0, insn1))
    return 1;
  if (IS_RT_SIGTRAMP (insn0, insn1))
    return 2;

  insn0 = ((insn0 << 16) & 0xffffffff) | (insn1 >> 16);
  insn1 = ((insn1 << 16) & 0xffffffff) | (insn2 >> 16);
  if (IS_SIGTRAMP (insn0, insn1))
    return 1;
  if (IS_RT_SIGTRAMP (insn0, insn1))
    return 2;

  return 0;
}

/* From <asm/sigcontext.h>.  */
static int m68k_linux_sigcontext_reg_offset[M68K_NUM_REGS] =
{
  2 * 4,			/* %d0 */
  3 * 4,			/* %d1 */
  -1,				/* %d2 */
  -1,				/* %d3 */
  -1,				/* %d4 */
  -1,				/* %d5 */
  -1,				/* %d6 */
  -1,				/* %d7 */
  4 * 4,			/* %a0 */
  5 * 4,			/* %a1 */
  -1,				/* %a2 */
  -1,				/* %a3 */
  -1,				/* %a4 */
  -1,				/* %a5 */
  -1,				/* %fp */
  1 * 4,			/* %sp */
  6 * 4,			/* %sr */
  6 * 4 + 2,			/* %pc */
  8 * 4,			/* %fp0 */
  11 * 4,			/* %fp1 */
  -1,				/* %fp2 */
  -1,				/* %fp3 */
  -1,				/* %fp4 */
  -1,				/* %fp5 */
  -1,				/* %fp6 */
  -1,				/* %fp7 */
  14 * 4,			/* %fpcr */
  15 * 4,			/* %fpsr */
  16 * 4			/* %fpiaddr */
};

static int m68k_uclinux_sigcontext_reg_offset[M68K_NUM_REGS] =
{
  2 * 4,			/* %d0 */
  3 * 4,			/* %d1 */
  -1,				/* %d2 */
  -1,				/* %d3 */
  -1,				/* %d4 */
  -1,				/* %d5 */
  -1,				/* %d6 */
  -1,				/* %d7 */
  4 * 4,			/* %a0 */
  5 * 4,			/* %a1 */
  -1,				/* %a2 */
  -1,				/* %a3 */
  -1,				/* %a4 */
  6 * 4,			/* %a5 */
  -1,				/* %fp */
  1 * 4,			/* %sp */
  7 * 4,			/* %sr */
  7 * 4 + 2,			/* %pc */
  -1,				/* %fp0 */
  -1,				/* %fp1 */
  -1,				/* %fp2 */
  -1,				/* %fp3 */
  -1,				/* %fp4 */
  -1,				/* %fp5 */
  -1,				/* %fp6 */
  -1,				/* %fp7 */
  -1,				/* %fpcr */
  -1,				/* %fpsr */
  -1				/* %fpiaddr */
};

/* From <asm/ucontext.h>.  */
static int m68k_linux_ucontext_reg_offset[M68K_NUM_REGS] =
{
  6 * 4,			/* %d0 */
  7 * 4,			/* %d1 */
  8 * 4,			/* %d2 */
  9 * 4,			/* %d3 */
  10 * 4,			/* %d4 */
  11 * 4,			/* %d5 */
  12 * 4,			/* %d6 */
  13 * 4,			/* %d7 */
  14 * 4,			/* %a0 */
  15 * 4,			/* %a1 */
  16 * 4,			/* %a2 */
  17 * 4,			/* %a3 */
  18 * 4,			/* %a4 */
  19 * 4,			/* %a5 */
  20 * 4,			/* %fp */
  21 * 4,			/* %sp */
  23 * 4,			/* %sr */
  22 * 4,			/* %pc */
  27 * 4,			/* %fp0 */
  30 * 4,			/* %fp1 */
  33 * 4,			/* %fp2 */
  36 * 4,			/* %fp3 */
  39 * 4,			/* %fp4 */
  42 * 4,			/* %fp5 */
  45 * 4,			/* %fp6 */
  48 * 4,			/* %fp7 */
  24 * 4,			/* %fpcr */
  25 * 4,			/* %fpsr */
  26 * 4			/* %fpiaddr */
};


/* Get info about saved registers in sigtramp.  */

struct m68k_linux_sigtramp_info
{
  /* Address of sigcontext.  */
  CORE_ADDR sigcontext_addr;

  /* Offset of registers in `struct sigcontext'.  */
  int *sc_reg_offset;
};

/* Nonzero if running on uClinux.  */
static int target_is_uclinux;

static void
m68k_linux_inferior_created (inferior *inf)
{
  /* Record that we will need to re-evaluate whether we are running on a
     uClinux or normal GNU/Linux target (see m68k_linux_get_sigtramp_info).  */
  target_is_uclinux = -1;
}

static struct m68k_linux_sigtramp_info
m68k_linux_get_sigtramp_info (frame_info_ptr this_frame)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR sp;
  struct m68k_linux_sigtramp_info info;

  /* Determine whether we are running on a uClinux or normal GNU/Linux
     target so we can use the correct sigcontext layouts.  */
  if (target_is_uclinux == -1)
    target_is_uclinux = linux_is_uclinux ();

  sp = get_frame_register_unsigned (this_frame, M68K_SP_REGNUM);

  /* Get sigcontext address, it is the third parameter on the stack.  */
  info.sigcontext_addr = read_memory_unsigned_integer (sp + 8, 4, byte_order);

  if (m68k_linux_pc_in_sigtramp (this_frame) == 2)
    info.sc_reg_offset = m68k_linux_ucontext_reg_offset;
  else
    info.sc_reg_offset = (target_is_uclinux
			  ? m68k_uclinux_sigcontext_reg_offset
			  : m68k_linux_sigcontext_reg_offset);
  return info;
}

/* Signal trampolines.  */

static struct trad_frame_cache *
m68k_linux_sigtramp_frame_cache (frame_info_ptr this_frame,
				 void **this_cache)
{
  struct frame_id this_id;
  struct trad_frame_cache *cache;
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  struct m68k_linux_sigtramp_info info;
  gdb_byte buf[4];
  int i;

  if (*this_cache)
    return (struct trad_frame_cache *) *this_cache;

  cache = trad_frame_cache_zalloc (this_frame);

  /* FIXME: cagney/2004-05-01: This is is long standing broken code.
     The frame ID's code address should be the start-address of the
     signal trampoline and not the current PC within that
     trampoline.  */
  get_frame_register (this_frame, M68K_SP_REGNUM, buf);
  /* See the end of m68k_push_dummy_call.  */
  this_id = frame_id_build (extract_unsigned_integer (buf, 4, byte_order)
			    - 4 + 8, get_frame_pc (this_frame));
  trad_frame_set_id (cache, this_id);

  info = m68k_linux_get_sigtramp_info (this_frame);

  for (i = 0; i < M68K_NUM_REGS; i++)
    if (info.sc_reg_offset[i] != -1)
      trad_frame_set_reg_addr (cache, i,
			       info.sigcontext_addr + info.sc_reg_offset[i]);

  *this_cache = cache;
  return cache;
}

static void
m68k_linux_sigtramp_frame_this_id (frame_info_ptr this_frame,
				   void **this_cache,
				   struct frame_id *this_id)
{
  struct trad_frame_cache *cache =
    m68k_linux_sigtramp_frame_cache (this_frame, this_cache);
  trad_frame_get_id (cache, this_id);
}

static struct value *
m68k_linux_sigtramp_frame_prev_register (frame_info_ptr this_frame,
					 void **this_cache,
					 int regnum)
{
  /* Make sure we've initialized the cache.  */
  struct trad_frame_cache *cache =
    m68k_linux_sigtramp_frame_cache (this_frame, this_cache);
  return trad_frame_get_register (cache, this_frame, regnum);
}

static int
m68k_linux_sigtramp_frame_sniffer (const struct frame_unwind *self,
				   frame_info_ptr this_frame,
				   void **this_prologue_cache)
{
  return m68k_linux_pc_in_sigtramp (this_frame);
}

static const struct frame_unwind m68k_linux_sigtramp_frame_unwind =
{
  "m68k linux sigtramp",
  SIGTRAMP_FRAME,
  default_frame_unwind_stop_reason,
  m68k_linux_sigtramp_frame_this_id,
  m68k_linux_sigtramp_frame_prev_register,
  NULL,
  m68k_linux_sigtramp_frame_sniffer
};

/* Register maps for supply/collect regset functions.  */

static const struct regcache_map_entry m68k_linux_gregmap[] =
  {
    { 7, M68K_D1_REGNUM, 4 },	/* d1 ... d7 */
    { 7, M68K_A0_REGNUM, 4 },	/* a0 ... a6 */
    { 1, M68K_D0_REGNUM, 4 },
    { 1, M68K_SP_REGNUM, 4 },
    { 1, REGCACHE_MAP_SKIP, 4 }, /* orig_d0 (skip) */
    { 1, M68K_PS_REGNUM, 4 },
    { 1, M68K_PC_REGNUM, 4 },
    /* Ignore 16-bit fields 'fmtvec' and '__fill'.  */
    { 0 }
  };

#define M68K_LINUX_GREGS_SIZE (20 * 4)

static const struct regcache_map_entry m68k_linux_fpregmap[] =
  {
    { 8, M68K_FP0_REGNUM, 12 },	/* fp0 ... fp7 */
    { 1, M68K_FPC_REGNUM, 4 },
    { 1, M68K_FPS_REGNUM, 4 },
    { 1, M68K_FPI_REGNUM, 4 },
    { 0 }
  };

#define M68K_LINUX_FPREGS_SIZE (27 * 4)

/* Register sets. */

static const struct regset m68k_linux_gregset =
  {
    m68k_linux_gregmap,
    regcache_supply_regset, regcache_collect_regset
  };

static const struct regset m68k_linux_fpregset =
  {
    m68k_linux_fpregmap,
    regcache_supply_regset, regcache_collect_regset
  };

/* Iterate over core file register note sections.  */

static void
m68k_linux_iterate_over_regset_sections (struct gdbarch *gdbarch,
					 iterate_over_regset_sections_cb *cb,
					 void *cb_data,
					 const struct regcache *regcache)
{
  cb (".reg", M68K_LINUX_GREGS_SIZE, M68K_LINUX_GREGS_SIZE, &m68k_linux_gregset,
      NULL, cb_data);
  cb (".reg2", M68K_LINUX_FPREGS_SIZE, M68K_LINUX_FPREGS_SIZE,
      &m68k_linux_fpregset, NULL, cb_data);
}

static void
m68k_linux_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  m68k_gdbarch_tdep *tdep = gdbarch_tdep<m68k_gdbarch_tdep> (gdbarch);

  linux_init_abi (info, gdbarch, 0);

  tdep->jb_pc = M68K_LINUX_JB_PC;
  tdep->jb_elt_size = M68K_LINUX_JB_ELEMENT_SIZE;

  /* GNU/Linux uses a calling convention that's similar to SVR4.  It
     returns integer values in %d0/%d1, pointer values in %a0 and
     floating values in %fp0, just like SVR4, but uses %a1 to pass the
     address to store a structure value.  It also returns small
     structures in registers instead of memory.  */
  m68k_svr4_init_abi (info, gdbarch);
  tdep->struct_value_regnum = M68K_A1_REGNUM;
  tdep->struct_return = reg_struct_return;

  set_gdbarch_decr_pc_after_break (gdbarch, 2);

  frame_unwind_append_unwinder (gdbarch, &m68k_linux_sigtramp_frame_unwind);

  /* Shared library handling.  */

  /* GNU/Linux uses SVR4-style shared libraries.  */
  set_solib_svr4_fetch_link_map_offsets (gdbarch,
					 linux_ilp32_fetch_link_map_offsets);

  /* GNU/Linux uses the dynamic linker included in the GNU C Library.  */
  set_gdbarch_skip_solib_resolver (gdbarch, glibc_skip_solib_resolver);

  set_gdbarch_skip_trampoline_code (gdbarch, find_solib_trampoline_target);

  /* Core file support. */
  set_gdbarch_iterate_over_regset_sections
    (gdbarch, m68k_linux_iterate_over_regset_sections);

  /* Enable TLS support.  */
  set_gdbarch_fetch_tls_load_module_address (gdbarch,
					     svr4_fetch_objfile_link_map);
}

void _initialize_m68k_linux_tdep ();
void
_initialize_m68k_linux_tdep ()
{
  gdbarch_register_osabi (bfd_arch_m68k, 0, GDB_OSABI_LINUX,
			  m68k_linux_init_abi);
  gdb::observers::inferior_created.attach (m68k_linux_inferior_created,
					   "m68k-linux-tdep");
}
