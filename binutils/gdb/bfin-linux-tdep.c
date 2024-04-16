/* Target-dependent code for Analog Devices Blackfin processor, for GDB.

   Copyright (C) 2005-2024 Free Software Foundation, Inc.

   Contributed by Analog Devices, Inc.

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
#include "regcache.h"
#include "tramp-frame.h"
#include "trad-frame.h"
#include "osabi.h"
#include "xml-syscall.h"
#include "linux-tdep.h"
#include "bfin-tdep.h"

/* From <asm/sigcontext.h>.  */

#define SIGCONTEXT_OFFSET	168

static const int bfin_linux_sigcontext_reg_offset[BFIN_NUM_REGS] =
{
  0 * 4,	/* %r0 */
  1 * 4,	/* %r1 */
  2 * 4,	/* %r2 */
  3 * 4,	/* %r3 */
  4 * 4,	/* %r4 */
  5 * 4,	/* %r5 */
  6 * 4,	/* %r6 */
  7 * 4,	/* %r7 */
  8 * 4,	/* %p0 */
  9 * 4,	/* %p1 */
  10 * 4,	/* %p2 */
  11 * 4,	/* %p3 */
  12 * 4,	/* %p4 */
  13 * 4,	/* %p5 */
  14 * 4,	/* %sp */
  23 * 4,	/* %fp */
  24 * 4,	/* %i0 */
  25 * 4,	/* %i1 */
  26 * 4,	/* %i2 */
  27 * 4,	/* %i3 */
  28 * 4,	/* %m0 */
  29 * 4,	/* %m1 */
  30 * 4,	/* %m2 */
  31 * 4,	/* %m3 */
  36 * 4,	/* %b0 */
  37 * 4,	/* %b1 */
  38 * 4,	/* %b2 */
  39 * 4,	/* %b3 */
  32 * 4,	/* %l0 */
  33 * 4,	/* %l1 */
  34 * 4,	/* %l2 */
  35 * 4,	/* %l3 */
  17 * 4,	/* %a0x */
  15 * 4,	/* %a0w */
  18 * 4,	/* %a1x */
  16 * 4,	/* %a1w */
  19 * 4,	/* %astat */
  20 * 4,	/* %rets */
  40 * 4,	/* %lc0 */
  42 * 4,	/* %lt0 */
  44 * 4,	/* %lb0 */
  41 * 4,	/* %lc1 */
  43 * 4,	/* %lt1 */
  45 * 4,	/* %lb1 */
  -1,		/* %cycles */
  -1,		/* %cycles2 */
  -1,		/* %usp */
  46 * 4,	/* %seqstat */
  -1,		/* syscfg */
  21 * 4,	/* %reti */
  22 * 4,	/* %retx */
  -1,		/* %retn */
  -1,		/* %rete */
  21 * 4,	/* %pc */
};

/* Signal trampolines.  */

static void
bfin_linux_sigframe_init (const struct tramp_frame *self,
			  frame_info_ptr this_frame,
			  struct trad_frame_cache *this_cache,
			  CORE_ADDR func)
{
  CORE_ADDR sp = get_frame_sp (this_frame);
  CORE_ADDR pc = get_frame_pc (this_frame);
  CORE_ADDR sigcontext = sp + SIGCONTEXT_OFFSET;
  const int *reg_offset = bfin_linux_sigcontext_reg_offset;
  int i;

  for (i = 0; i < BFIN_NUM_REGS; i++)
    if (reg_offset[i] != -1)
      trad_frame_set_reg_addr (this_cache, i, sigcontext + reg_offset[i]);

  /* This would come after the LINK instruction in the ret_from_signal
     function, hence the frame id would be SP + 8.  */
  trad_frame_set_id (this_cache, frame_id_build (sp + 8, pc));
}

static const struct tramp_frame bfin_linux_sigframe =
{
  SIGTRAMP_FRAME,
  4,
  {
    { 0x00ADE128, 0xffffffff },	/* P0 = __NR_rt_sigreturn; */
    { 0x00A0, 0xffff },		/* EXCPT 0; */
    { TRAMP_SENTINEL_INSN, ULONGEST_MAX },
  },
  bfin_linux_sigframe_init,
};

static LONGEST
bfin_linux_get_syscall_number (struct gdbarch *gdbarch,
			       thread_info *thread)
{
  struct regcache *regcache = get_thread_regcache (thread);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  /* The content of a register.  */
  gdb_byte buf[4];
  /* The result.  */
  LONGEST ret;

  /* Getting the system call number from the register.
     When dealing with Blackfin architecture, this information
     is stored at %p0 register.  */
  regcache->cooked_read (BFIN_P0_REGNUM, buf);

  ret = extract_signed_integer (buf, 4, byte_order);

  return ret;
}

static void
bfin_linux_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  linux_init_abi (info, gdbarch, 0);

  /* Set the sigtramp frame sniffer.  */
  tramp_frame_prepend_unwinder (gdbarch, &bfin_linux_sigframe);

  /* Functions for 'catch syscall'.  */
  set_xml_syscall_file_name (gdbarch, "syscalls/bfin-linux.xml");
  set_gdbarch_get_syscall_number (gdbarch,
				  bfin_linux_get_syscall_number);
}

void _initialize_bfin_linux_tdep ();
void
_initialize_bfin_linux_tdep ()
{
  gdbarch_register_osabi (bfd_arch_bfin, 0, GDB_OSABI_LINUX,
			  bfin_linux_init_abi);
}
