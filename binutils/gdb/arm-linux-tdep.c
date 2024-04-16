/* GNU/Linux on ARM target support.

   Copyright (C) 1999-2024 Free Software Foundation, Inc.

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
#include "target.h"
#include "value.h"
#include "gdbtypes.h"
#include "gdbcore.h"
#include "frame.h"
#include "regcache.h"
#include "solib-svr4.h"
#include "osabi.h"
#include "regset.h"
#include "trad-frame.h"
#include "tramp-frame.h"
#include "breakpoint.h"
#include "auxv.h"
#include "xml-syscall.h"
#include "expop.h"

#include "aarch32-tdep.h"
#include "arch/arm.h"
#include "arch/arm-get-next-pcs.h"
#include "arch/arm-linux.h"
#include "arm-tdep.h"
#include "arm-linux-tdep.h"
#include "linux-tdep.h"
#include "glibc-tdep.h"
#include "arch-utils.h"
#include "inferior.h"
#include "infrun.h"
#include "gdbthread.h"
#include "symfile.h"

#include "record-full.h"
#include "linux-record.h"

#include "cli/cli-utils.h"
#include "stap-probe.h"
#include "parser-defs.h"
#include "user-regs.h"
#include <ctype.h>
#include "elf/common.h"

/* Under ARM GNU/Linux the traditional way of performing a breakpoint
   is to execute a particular software interrupt, rather than use a
   particular undefined instruction to provoke a trap.  Upon execution
   of the software interrupt the kernel stops the inferior with a
   SIGTRAP, and wakes the debugger.  */

static const gdb_byte arm_linux_arm_le_breakpoint[] = { 0x01, 0x00, 0x9f, 0xef };

static const gdb_byte arm_linux_arm_be_breakpoint[] = { 0xef, 0x9f, 0x00, 0x01 };

/* However, the EABI syscall interface (new in Nov. 2005) does not look at
   the operand of the swi if old-ABI compatibility is disabled.  Therefore,
   use an undefined instruction instead.  This is supported as of kernel
   version 2.5.70 (May 2003), so should be a safe assumption for EABI
   binaries.  */

static const gdb_byte eabi_linux_arm_le_breakpoint[] = { 0xf0, 0x01, 0xf0, 0xe7 };

static const gdb_byte eabi_linux_arm_be_breakpoint[] = { 0xe7, 0xf0, 0x01, 0xf0 };

/* All the kernels which support Thumb support using a specific undefined
   instruction for the Thumb breakpoint.  */

static const gdb_byte arm_linux_thumb_be_breakpoint[] = {0xde, 0x01};

static const gdb_byte arm_linux_thumb_le_breakpoint[] = {0x01, 0xde};

/* Because the 16-bit Thumb breakpoint is affected by Thumb-2 IT blocks,
   we must use a length-appropriate breakpoint for 32-bit Thumb
   instructions.  See also thumb_get_next_pc.  */

static const gdb_byte arm_linux_thumb2_be_breakpoint[] = { 0xf7, 0xf0, 0xa0, 0x00 };

static const gdb_byte arm_linux_thumb2_le_breakpoint[] = { 0xf0, 0xf7, 0x00, 0xa0 };

/* Description of the longjmp buffer.  The buffer is treated as an array of 
   elements of size ARM_LINUX_JB_ELEMENT_SIZE.

   The location of saved registers in this buffer (in particular the PC
   to use after longjmp is called) varies depending on the ABI (in 
   particular the FP model) and also (possibly) the C Library.

   For glibc, eglibc, and uclibc the following holds:  If the FP model is 
   SoftVFP or VFP (which implies EABI) then the PC is at offset 9 in the 
   buffer.  This is also true for the SoftFPA model.  However, for the FPA 
   model the PC is at offset 21 in the buffer.  */
#define ARM_LINUX_JB_ELEMENT_SIZE	ARM_INT_REGISTER_SIZE
#define ARM_LINUX_JB_PC_FPA		21
#define ARM_LINUX_JB_PC_EABI		9

/*
   Dynamic Linking on ARM GNU/Linux
   --------------------------------

   Note: PLT = procedure linkage table
   GOT = global offset table

   As much as possible, ELF dynamic linking defers the resolution of
   jump/call addresses until the last minute.  The technique used is
   inspired by the i386 ELF design, and is based on the following
   constraints.

   1) The calling technique should not force a change in the assembly
   code produced for apps; it MAY cause changes in the way assembly
   code is produced for position independent code (i.e. shared
   libraries).

   2) The technique must be such that all executable areas must not be
   modified; and any modified areas must not be executed.

   To do this, there are three steps involved in a typical jump:

   1) in the code
   2) through the PLT
   3) using a pointer from the GOT

   When the executable or library is first loaded, each GOT entry is
   initialized to point to the code which implements dynamic name
   resolution and code finding.  This is normally a function in the
   program interpreter (on ARM GNU/Linux this is usually
   ld-linux.so.2, but it does not have to be).  On the first
   invocation, the function is located and the GOT entry is replaced
   with the real function address.  Subsequent calls go through steps
   1, 2 and 3 and end up calling the real code.

   1) In the code: 

   b    function_call
   bl   function_call

   This is typical ARM code using the 26 bit relative branch or branch
   and link instructions.  The target of the instruction
   (function_call is usually the address of the function to be called.
   In position independent code, the target of the instruction is
   actually an entry in the PLT when calling functions in a shared
   library.  Note that this call is identical to a normal function
   call, only the target differs.

   2) In the PLT:

   The PLT is a synthetic area, created by the linker.  It exists in
   both executables and libraries.  It is an array of stubs, one per
   imported function call.  It looks like this:

   PLT[0]:
   str     lr, [sp, #-4]!       @push the return address (lr)
   ldr     lr, [pc, #16]   @load from 6 words ahead
   add     lr, pc, lr      @form an address for GOT[0]
   ldr     pc, [lr, #8]!   @jump to the contents of that addr

   The return address (lr) is pushed on the stack and used for
   calculations.  The load on the second line loads the lr with
   &GOT[3] - . - 20.  The addition on the third leaves:

   lr = (&GOT[3] - . - 20) + (. + 8)
   lr = (&GOT[3] - 12)
   lr = &GOT[0]

   On the fourth line, the pc and lr are both updated, so that:

   pc = GOT[2]
   lr = &GOT[0] + 8
   = &GOT[2]

   NOTE: PLT[0] borrows an offset .word from PLT[1].  This is a little
   "tight", but allows us to keep all the PLT entries the same size.

   PLT[n+1]:
   ldr     ip, [pc, #4]    @load offset from gotoff
   add     ip, pc, ip      @add the offset to the pc
   ldr     pc, [ip]        @jump to that address
   gotoff: .word   GOT[n+3] - .

   The load on the first line, gets an offset from the fourth word of
   the PLT entry.  The add on the second line makes ip = &GOT[n+3],
   which contains either a pointer to PLT[0] (the fixup trampoline) or
   a pointer to the actual code.

   3) In the GOT:

   The GOT contains helper pointers for both code (PLT) fixups and
   data fixups.  The first 3 entries of the GOT are special.  The next
   M entries (where M is the number of entries in the PLT) belong to
   the PLT fixups.  The next D (all remaining) entries belong to
   various data fixups.  The actual size of the GOT is 3 + M + D.

   The GOT is also a synthetic area, created by the linker.  It exists
   in both executables and libraries.  When the GOT is first
   initialized , all the GOT entries relating to PLT fixups are
   pointing to code back at PLT[0].

   The special entries in the GOT are:

   GOT[0] = linked list pointer used by the dynamic loader
   GOT[1] = pointer to the reloc table for this module
   GOT[2] = pointer to the fixup/resolver code

   The first invocation of function call comes through and uses the
   fixup/resolver code.  On the entry to the fixup/resolver code:

   ip = &GOT[n+3]
   lr = &GOT[2]
   stack[0] = return address (lr) of the function call
   [r0, r1, r2, r3] are still the arguments to the function call

   This is enough information for the fixup/resolver code to work
   with.  Before the fixup/resolver code returns, it actually calls
   the requested function and repairs &GOT[n+3].  */

/* The constants below were determined by examining the following files
   in the linux kernel sources:

      arch/arm/kernel/signal.c
	  - see SWI_SYS_SIGRETURN and SWI_SYS_RT_SIGRETURN
      include/asm-arm/unistd.h
	  - see __NR_sigreturn, __NR_rt_sigreturn, and __NR_SYSCALL_BASE */

#define ARM_LINUX_SIGRETURN_INSTR	0xef900077
#define ARM_LINUX_RT_SIGRETURN_INSTR	0xef9000ad

/* For ARM EABI, the syscall number is not in the SWI instruction
   (instead it is loaded into r7).  We recognize the pattern that
   glibc uses...  alternatively, we could arrange to do this by
   function name, but they are not always exported.  */
#define ARM_SET_R7_SIGRETURN		0xe3a07077
#define ARM_SET_R7_RT_SIGRETURN		0xe3a070ad
#define ARM_EABI_SYSCALL		0xef000000

/* Equivalent patterns for Thumb2.  */
#define THUMB2_SET_R7_SIGRETURN1	0xf04f
#define THUMB2_SET_R7_SIGRETURN2	0x0777
#define THUMB2_SET_R7_RT_SIGRETURN1	0xf04f
#define THUMB2_SET_R7_RT_SIGRETURN2	0x07ad
#define THUMB2_EABI_SYSCALL		0xdf00

/* OABI syscall restart trampoline, used for EABI executables too
   whenever OABI support has been enabled in the kernel.  */
#define ARM_OABI_SYSCALL_RESTART_SYSCALL 0xef900000
#define ARM_LDR_PC_SP_12		0xe49df00c
#define ARM_LDR_PC_SP_4			0xe49df004

/* Syscall number for sigreturn.  */
#define ARM_SIGRETURN 119
/* Syscall number for rt_sigreturn.  */
#define ARM_RT_SIGRETURN 173

static CORE_ADDR
  arm_linux_get_next_pcs_syscall_next_pc (struct arm_get_next_pcs *self);

/* Operation function pointers for get_next_pcs.  */
static struct arm_get_next_pcs_ops arm_linux_get_next_pcs_ops = {
  arm_get_next_pcs_read_memory_unsigned_integer,
  arm_linux_get_next_pcs_syscall_next_pc,
  arm_get_next_pcs_addr_bits_remove,
  arm_get_next_pcs_is_thumb,
  arm_linux_get_next_pcs_fixup,
};

static void
arm_linux_sigtramp_cache (frame_info_ptr this_frame,
			  struct trad_frame_cache *this_cache,
			  CORE_ADDR func, int regs_offset)
{
  CORE_ADDR sp = get_frame_register_unsigned (this_frame, ARM_SP_REGNUM);
  CORE_ADDR base = sp + regs_offset;
  int i;

  for (i = 0; i < 16; i++)
    trad_frame_set_reg_addr (this_cache, i, base + i * 4);

  trad_frame_set_reg_addr (this_cache, ARM_PS_REGNUM, base + 16 * 4);

  /* The VFP or iWMMXt registers may be saved on the stack, but there's
     no reliable way to restore them (yet).  */

  /* Save a frame ID.  */
  trad_frame_set_id (this_cache, frame_id_build (sp, func));
}

/* See arm-linux.h for stack layout details.  */
static void
arm_linux_sigreturn_init (const struct tramp_frame *self,
			  frame_info_ptr this_frame,
			  struct trad_frame_cache *this_cache,
			  CORE_ADDR func)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR sp = get_frame_register_unsigned (this_frame, ARM_SP_REGNUM);
  ULONGEST uc_flags = read_memory_unsigned_integer (sp, 4, byte_order);

  if (uc_flags == ARM_NEW_SIGFRAME_MAGIC)
    arm_linux_sigtramp_cache (this_frame, this_cache, func,
			      ARM_UCONTEXT_SIGCONTEXT
			      + ARM_SIGCONTEXT_R0);
  else
    arm_linux_sigtramp_cache (this_frame, this_cache, func,
			      ARM_SIGCONTEXT_R0);
}

static void
arm_linux_rt_sigreturn_init (const struct tramp_frame *self,
			  frame_info_ptr this_frame,
			  struct trad_frame_cache *this_cache,
			  CORE_ADDR func)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR sp = get_frame_register_unsigned (this_frame, ARM_SP_REGNUM);
  ULONGEST pinfo = read_memory_unsigned_integer (sp, 4, byte_order);

  if (pinfo == sp + ARM_OLD_RT_SIGFRAME_SIGINFO)
    arm_linux_sigtramp_cache (this_frame, this_cache, func,
			      ARM_OLD_RT_SIGFRAME_UCONTEXT
			      + ARM_UCONTEXT_SIGCONTEXT
			      + ARM_SIGCONTEXT_R0);
  else
    arm_linux_sigtramp_cache (this_frame, this_cache, func,
			      ARM_NEW_RT_SIGFRAME_UCONTEXT
			      + ARM_UCONTEXT_SIGCONTEXT
			      + ARM_SIGCONTEXT_R0);
}

static void
arm_linux_restart_syscall_init (const struct tramp_frame *self,
				frame_info_ptr this_frame,
				struct trad_frame_cache *this_cache,
				CORE_ADDR func)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  CORE_ADDR sp = get_frame_register_unsigned (this_frame, ARM_SP_REGNUM);
  CORE_ADDR pc = get_frame_memory_unsigned (this_frame, sp, 4);
  CORE_ADDR cpsr = get_frame_register_unsigned (this_frame, ARM_PS_REGNUM);
  ULONGEST t_bit = arm_psr_thumb_bit (gdbarch);
  int sp_offset;

  /* There are two variants of this trampoline; with older kernels, the
     stub is placed on the stack, while newer kernels use the stub from
     the vector page.  They are identical except that the older version
     increments SP by 12 (to skip stored PC and the stub itself), while
     the newer version increments SP only by 4 (just the stored PC).  */
  if (self->insn[1].bytes == ARM_LDR_PC_SP_4)
    sp_offset = 4;
  else
    sp_offset = 12;

  /* Update Thumb bit in CPSR.  */
  if (pc & 1)
    cpsr |= t_bit;
  else
    cpsr &= ~t_bit;

  /* Remove Thumb bit from PC.  */
  pc = gdbarch_addr_bits_remove (gdbarch, pc);

  /* Save previous register values.  */
  trad_frame_set_reg_value (this_cache, ARM_SP_REGNUM, sp + sp_offset);
  trad_frame_set_reg_value (this_cache, ARM_PC_REGNUM, pc);
  trad_frame_set_reg_value (this_cache, ARM_PS_REGNUM, cpsr);

  /* Save a frame ID.  */
  trad_frame_set_id (this_cache, frame_id_build (sp, func));
}

static struct tramp_frame arm_linux_sigreturn_tramp_frame = {
  SIGTRAMP_FRAME,
  4,
  {
    { ARM_LINUX_SIGRETURN_INSTR, ULONGEST_MAX },
    { TRAMP_SENTINEL_INSN }
  },
  arm_linux_sigreturn_init
};

static struct tramp_frame arm_linux_rt_sigreturn_tramp_frame = {
  SIGTRAMP_FRAME,
  4,
  {
    { ARM_LINUX_RT_SIGRETURN_INSTR, ULONGEST_MAX },
    { TRAMP_SENTINEL_INSN }
  },
  arm_linux_rt_sigreturn_init
};

static struct tramp_frame arm_eabi_linux_sigreturn_tramp_frame = {
  SIGTRAMP_FRAME,
  4,
  {
    { ARM_SET_R7_SIGRETURN, ULONGEST_MAX },
    { ARM_EABI_SYSCALL, ULONGEST_MAX },
    { TRAMP_SENTINEL_INSN }
  },
  arm_linux_sigreturn_init
};

static struct tramp_frame arm_eabi_linux_rt_sigreturn_tramp_frame = {
  SIGTRAMP_FRAME,
  4,
  {
    { ARM_SET_R7_RT_SIGRETURN, ULONGEST_MAX },
    { ARM_EABI_SYSCALL, ULONGEST_MAX },
    { TRAMP_SENTINEL_INSN }
  },
  arm_linux_rt_sigreturn_init
};

static struct tramp_frame thumb2_eabi_linux_sigreturn_tramp_frame = {
  SIGTRAMP_FRAME,
  2,
  {
    { THUMB2_SET_R7_SIGRETURN1, ULONGEST_MAX },
    { THUMB2_SET_R7_SIGRETURN2, ULONGEST_MAX },
    { THUMB2_EABI_SYSCALL, ULONGEST_MAX },
    { TRAMP_SENTINEL_INSN }
  },
  arm_linux_sigreturn_init
};

static struct tramp_frame thumb2_eabi_linux_rt_sigreturn_tramp_frame = {
  SIGTRAMP_FRAME,
  2,
  {
    { THUMB2_SET_R7_RT_SIGRETURN1, ULONGEST_MAX },
    { THUMB2_SET_R7_RT_SIGRETURN2, ULONGEST_MAX },
    { THUMB2_EABI_SYSCALL, ULONGEST_MAX },
    { TRAMP_SENTINEL_INSN }
  },
  arm_linux_rt_sigreturn_init
};

static struct tramp_frame arm_linux_restart_syscall_tramp_frame = {
  NORMAL_FRAME,
  4,
  {
    { ARM_OABI_SYSCALL_RESTART_SYSCALL, ULONGEST_MAX },
    { ARM_LDR_PC_SP_12, ULONGEST_MAX },
    { TRAMP_SENTINEL_INSN }
  },
  arm_linux_restart_syscall_init
};

static struct tramp_frame arm_kernel_linux_restart_syscall_tramp_frame = {
  NORMAL_FRAME,
  4,
  {
    { ARM_OABI_SYSCALL_RESTART_SYSCALL, ULONGEST_MAX },
    { ARM_LDR_PC_SP_4, ULONGEST_MAX },
    { TRAMP_SENTINEL_INSN }
  },
  arm_linux_restart_syscall_init
};

/* Core file and register set support.  */

#define ARM_LINUX_SIZEOF_GREGSET (18 * ARM_INT_REGISTER_SIZE)

void
arm_linux_supply_gregset (const struct regset *regset,
			  struct regcache *regcache,
			  int regnum, const void *gregs_buf, size_t len)
{
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  const gdb_byte *gregs = (const gdb_byte *) gregs_buf;
  int regno;
  CORE_ADDR reg_pc;
  gdb_byte pc_buf[ARM_INT_REGISTER_SIZE];

  for (regno = ARM_A1_REGNUM; regno < ARM_PC_REGNUM; regno++)
    if (regnum == -1 || regnum == regno)
      regcache->raw_supply (regno, gregs + ARM_INT_REGISTER_SIZE * regno);

  if (regnum == ARM_PS_REGNUM || regnum == -1)
    {
      if (arm_apcs_32)
	regcache->raw_supply (ARM_PS_REGNUM,
			      gregs + ARM_INT_REGISTER_SIZE * ARM_CPSR_GREGNUM);
      else
	regcache->raw_supply (ARM_PS_REGNUM,
			     gregs + ARM_INT_REGISTER_SIZE * ARM_PC_REGNUM);
    }

  if (regnum == ARM_PC_REGNUM || regnum == -1)
    {
      reg_pc = extract_unsigned_integer (
		 gregs + ARM_INT_REGISTER_SIZE * ARM_PC_REGNUM,
		 ARM_INT_REGISTER_SIZE, byte_order);
      reg_pc = gdbarch_addr_bits_remove (gdbarch, reg_pc);
      store_unsigned_integer (pc_buf, ARM_INT_REGISTER_SIZE, byte_order,
			      reg_pc);
      regcache->raw_supply (ARM_PC_REGNUM, pc_buf);
    }
}

void
arm_linux_collect_gregset (const struct regset *regset,
			   const struct regcache *regcache,
			   int regnum, void *gregs_buf, size_t len)
{
  gdb_byte *gregs = (gdb_byte *) gregs_buf;
  int regno;

  for (regno = ARM_A1_REGNUM; regno < ARM_PC_REGNUM; regno++)
    if (regnum == -1 || regnum == regno)
      regcache->raw_collect (regno,
			    gregs + ARM_INT_REGISTER_SIZE * regno);

  if (regnum == ARM_PS_REGNUM || regnum == -1)
    {
      if (arm_apcs_32)
	regcache->raw_collect (ARM_PS_REGNUM,
			      gregs + ARM_INT_REGISTER_SIZE * ARM_CPSR_GREGNUM);
      else
	regcache->raw_collect (ARM_PS_REGNUM,
			      gregs + ARM_INT_REGISTER_SIZE * ARM_PC_REGNUM);
    }

  if (regnum == ARM_PC_REGNUM || regnum == -1)
    regcache->raw_collect (ARM_PC_REGNUM,
			   gregs + ARM_INT_REGISTER_SIZE * ARM_PC_REGNUM);
}

/* Support for register format used by the NWFPE FPA emulator.  */

#define typeNone		0x00
#define typeSingle		0x01
#define typeDouble		0x02
#define typeExtended		0x03

void
supply_nwfpe_register (struct regcache *regcache, int regno,
		       const gdb_byte *regs)
{
  const gdb_byte *reg_data;
  gdb_byte reg_tag;
  gdb_byte buf[ARM_FP_REGISTER_SIZE];

  reg_data = regs + (regno - ARM_F0_REGNUM) * ARM_FP_REGISTER_SIZE;
  reg_tag = regs[(regno - ARM_F0_REGNUM) + NWFPE_TAGS_OFFSET];
  memset (buf, 0, ARM_FP_REGISTER_SIZE);

  switch (reg_tag)
    {
    case typeSingle:
      memcpy (buf, reg_data, 4);
      break;
    case typeDouble:
      memcpy (buf, reg_data + 4, 4);
      memcpy (buf + 4, reg_data, 4);
      break;
    case typeExtended:
      /* We want sign and exponent, then least significant bits,
	 then most significant.  NWFPE does sign, most, least.  */
      memcpy (buf, reg_data, 4);
      memcpy (buf + 4, reg_data + 8, 4);
      memcpy (buf + 8, reg_data + 4, 4);
      break;
    default:
      break;
    }

  regcache->raw_supply (regno, buf);
}

void
collect_nwfpe_register (const struct regcache *regcache, int regno,
			gdb_byte *regs)
{
  gdb_byte *reg_data;
  gdb_byte reg_tag;
  gdb_byte buf[ARM_FP_REGISTER_SIZE];

  regcache->raw_collect (regno, buf);

  /* NOTE drow/2006-06-07: This code uses the tag already in the
     register buffer.  I've preserved that when moving the code
     from the native file to the target file.  But this doesn't
     always make sense.  */

  reg_data = regs + (regno - ARM_F0_REGNUM) * ARM_FP_REGISTER_SIZE;
  reg_tag = regs[(regno - ARM_F0_REGNUM) + NWFPE_TAGS_OFFSET];

  switch (reg_tag)
    {
    case typeSingle:
      memcpy (reg_data, buf, 4);
      break;
    case typeDouble:
      memcpy (reg_data, buf + 4, 4);
      memcpy (reg_data + 4, buf, 4);
      break;
    case typeExtended:
      memcpy (reg_data, buf, 4);
      memcpy (reg_data + 4, buf + 8, 4);
      memcpy (reg_data + 8, buf + 4, 4);
      break;
    default:
      break;
    }
}

void
arm_linux_supply_nwfpe (const struct regset *regset,
			struct regcache *regcache,
			int regnum, const void *regs_buf, size_t len)
{
  const gdb_byte *regs = (const gdb_byte *) regs_buf;
  int regno;

  if (regnum == ARM_FPS_REGNUM || regnum == -1)
    regcache->raw_supply (ARM_FPS_REGNUM,
			 regs + NWFPE_FPSR_OFFSET);

  for (regno = ARM_F0_REGNUM; regno <= ARM_F7_REGNUM; regno++)
    if (regnum == -1 || regnum == regno)
      supply_nwfpe_register (regcache, regno, regs);
}

void
arm_linux_collect_nwfpe (const struct regset *regset,
			 const struct regcache *regcache,
			 int regnum, void *regs_buf, size_t len)
{
  gdb_byte *regs = (gdb_byte *) regs_buf;
  int regno;

  for (regno = ARM_F0_REGNUM; regno <= ARM_F7_REGNUM; regno++)
    if (regnum == -1 || regnum == regno)
      collect_nwfpe_register (regcache, regno, regs);

  if (regnum == ARM_FPS_REGNUM || regnum == -1)
    regcache->raw_collect (ARM_FPS_REGNUM,
			   regs + ARM_INT_REGISTER_SIZE * ARM_FPS_REGNUM);
}

/* Support VFP register format.  */

#define ARM_LINUX_SIZEOF_VFP (32 * 8 + 4)

static void
arm_linux_supply_vfp (const struct regset *regset,
		      struct regcache *regcache,
		      int regnum, const void *regs_buf, size_t len)
{
  const gdb_byte *regs = (const gdb_byte *) regs_buf;
  int regno;

  if (regnum == ARM_FPSCR_REGNUM || regnum == -1)
    regcache->raw_supply (ARM_FPSCR_REGNUM, regs + 32 * 8);

  for (regno = ARM_D0_REGNUM; regno <= ARM_D31_REGNUM; regno++)
    if (regnum == -1 || regnum == regno)
      regcache->raw_supply (regno, regs + (regno - ARM_D0_REGNUM) * 8);
}

static void
arm_linux_collect_vfp (const struct regset *regset,
			 const struct regcache *regcache,
			 int regnum, void *regs_buf, size_t len)
{
  gdb_byte *regs = (gdb_byte *) regs_buf;
  int regno;

  if (regnum == ARM_FPSCR_REGNUM || regnum == -1)
    regcache->raw_collect (ARM_FPSCR_REGNUM, regs + 32 * 8);

  for (regno = ARM_D0_REGNUM; regno <= ARM_D31_REGNUM; regno++)
    if (regnum == -1 || regnum == regno)
      regcache->raw_collect (regno, regs + (regno - ARM_D0_REGNUM) * 8);
}

static const struct regset arm_linux_gregset =
  {
    NULL, arm_linux_supply_gregset, arm_linux_collect_gregset
  };

static const struct regset arm_linux_fpregset =
  {
    NULL, arm_linux_supply_nwfpe, arm_linux_collect_nwfpe
  };

static const struct regset arm_linux_vfpregset =
  {
    NULL, arm_linux_supply_vfp, arm_linux_collect_vfp
  };

/* Iterate over core file register note sections.  */

static void
arm_linux_iterate_over_regset_sections (struct gdbarch *gdbarch,
					iterate_over_regset_sections_cb *cb,
					void *cb_data,
					const struct regcache *regcache)
{
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  cb (".reg", ARM_LINUX_SIZEOF_GREGSET, ARM_LINUX_SIZEOF_GREGSET,
      &arm_linux_gregset, NULL, cb_data);

  if (tdep->vfp_register_count > 0)
    cb (".reg-arm-vfp", ARM_LINUX_SIZEOF_VFP, ARM_LINUX_SIZEOF_VFP,
	&arm_linux_vfpregset, "VFP floating-point", cb_data);
  else if (tdep->have_fpa_registers)
    cb (".reg2", ARM_LINUX_SIZEOF_NWFPE, ARM_LINUX_SIZEOF_NWFPE,
	&arm_linux_fpregset, "FPA floating-point", cb_data);
}

/* Determine target description from core file.  */

static const struct target_desc *
arm_linux_core_read_description (struct gdbarch *gdbarch,
				 struct target_ops *target,
				 bfd *abfd)
{
  std::optional<gdb::byte_vector> auxv = target_read_auxv_raw (target);
  CORE_ADDR arm_hwcap = linux_get_hwcap (auxv, target, gdbarch);

  if (arm_hwcap & HWCAP_VFP)
    {
      /* NEON implies VFPv3-D32 or no-VFP unit.  Say that we only support
	 Neon with VFPv3-D32.  */
      if (arm_hwcap & HWCAP_NEON)
	return aarch32_read_description ();
      else if ((arm_hwcap & (HWCAP_VFPv3 | HWCAP_VFPv3D16)) == HWCAP_VFPv3)
	return arm_read_description (ARM_FP_TYPE_VFPV3, false);

      return arm_read_description (ARM_FP_TYPE_VFPV2, false);
    }

  return nullptr;
}


/* Copy the value of next pc of sigreturn and rt_sigrturn into PC,
   return 1.  In addition, set IS_THUMB depending on whether we
   will return to ARM or Thumb code.  Return 0 if it is not a
   rt_sigreturn/sigreturn syscall.  */
static int
arm_linux_sigreturn_return_addr (frame_info_ptr frame,
				 unsigned long svc_number,
				 CORE_ADDR *pc, int *is_thumb)
{
  /* Is this a sigreturn or rt_sigreturn syscall?  */
  if (svc_number == 119 || svc_number == 173)
    {
      if (get_frame_type (frame) == SIGTRAMP_FRAME)
	{
	  ULONGEST t_bit = arm_psr_thumb_bit (frame_unwind_arch (frame));
	  CORE_ADDR cpsr
	    = frame_unwind_register_unsigned (frame, ARM_PS_REGNUM);

	  *is_thumb = (cpsr & t_bit) != 0;
	  *pc = frame_unwind_caller_pc (frame);
	  return 1;
	}
    }
  return 0;
}

/* Find the value of the next PC after a sigreturn or rt_sigreturn syscall
   based on current processor state.  In addition, set IS_THUMB depending
   on whether we will return to ARM or Thumb code.  */

static CORE_ADDR
arm_linux_sigreturn_next_pc (struct regcache *regcache,
			     unsigned long svc_number, int *is_thumb)
{
  ULONGEST sp;
  unsigned long sp_data;
  CORE_ADDR next_pc = 0;
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int pc_offset = 0;
  int is_sigreturn = 0;
  CORE_ADDR cpsr;

  gdb_assert (svc_number == ARM_SIGRETURN
	      || svc_number == ARM_RT_SIGRETURN);

  is_sigreturn = (svc_number == ARM_SIGRETURN);
  regcache_cooked_read_unsigned (regcache, ARM_SP_REGNUM, &sp);
  sp_data = read_memory_unsigned_integer (sp, 4, byte_order);

  pc_offset = arm_linux_sigreturn_next_pc_offset (sp, sp_data, svc_number,
						  is_sigreturn);

  next_pc = read_memory_unsigned_integer (sp + pc_offset, 4, byte_order);

  /* Set IS_THUMB according the CPSR saved on the stack.  */
  cpsr = read_memory_unsigned_integer (sp + pc_offset + 4, 4, byte_order);
  *is_thumb = ((cpsr & arm_psr_thumb_bit (gdbarch)) != 0);

  return next_pc;
}

/* Return true if we're at execve syscall-exit-stop.  */

static bool
is_execve_syscall_exit (struct regcache *regs)
{
  ULONGEST reg = -1;

  /* Check that lr is 0.  */
  regcache_cooked_read_unsigned (regs, ARM_LR_REGNUM, &reg);
  if (reg != 0)
    return false;

  /* Check that r0-r8 is 0.  */
  for (int i = 0; i <= 8; ++i)
    {
      reg = -1;
      regcache_cooked_read_unsigned (regs, ARM_A1_REGNUM + i, &reg);
      if (reg != 0)
	return false;
    }

  return true;
}

#define arm_sys_execve 11

/* At a ptrace syscall-stop, return the syscall number.  This either
   comes from the SWI instruction (OABI) or from r7 (EABI).

   When the function fails, it should return -1.  */

static LONGEST
arm_linux_get_syscall_number (struct gdbarch *gdbarch,
			      thread_info *thread)
{
  struct regcache *regs = get_thread_regcache (thread);

  ULONGEST pc;
  ULONGEST cpsr;
  ULONGEST t_bit = arm_psr_thumb_bit (gdbarch);
  int is_thumb;
  ULONGEST svc_number = -1;

  if (is_execve_syscall_exit (regs))
    return arm_sys_execve;

  regcache_cooked_read_unsigned (regs, ARM_PC_REGNUM, &pc);
  regcache_cooked_read_unsigned (regs, ARM_PS_REGNUM, &cpsr);
  is_thumb = (cpsr & t_bit) != 0;

  if (is_thumb)
    {
      regcache_cooked_read_unsigned (regs, 7, &svc_number);
    }
  else
    {
      enum bfd_endian byte_order_for_code = 
	gdbarch_byte_order_for_code (gdbarch);

      /* PC gets incremented before the syscall-stop, so read the
	 previous instruction.  */
      unsigned long this_instr;
      {
	ULONGEST val;
	if (!safe_read_memory_unsigned_integer (pc - 4, 4, byte_order_for_code,
						&val))
	  return -1;
	this_instr = val;
      }
      unsigned long svc_operand = (0x00ffffff & this_instr);

      if (svc_operand)
	{
	  /* OABI */
	  svc_number = svc_operand - 0x900000;
	}
      else
	{
	  /* EABI */
	  regcache_cooked_read_unsigned (regs, 7, &svc_number);
	}
    }

  return svc_number;
}

static CORE_ADDR
arm_linux_get_next_pcs_syscall_next_pc (struct arm_get_next_pcs *self)
{
  CORE_ADDR next_pc = 0;
  regcache *regcache
    = gdb::checked_static_cast<struct regcache *> (self->regcache);
  CORE_ADDR pc = regcache_read_pc (regcache);
  int is_thumb = arm_is_thumb (regcache);
  ULONGEST svc_number = 0;

  if (is_thumb)
    {
      svc_number = regcache_raw_get_unsigned (self->regcache, 7);
      next_pc = pc + 2;
    }
  else
    {
      struct gdbarch *gdbarch = regcache->arch ();
      enum bfd_endian byte_order_for_code = 
	gdbarch_byte_order_for_code (gdbarch);
      unsigned long this_instr = 
	read_memory_unsigned_integer (pc, 4, byte_order_for_code);

      unsigned long svc_operand = (0x00ffffff & this_instr);
      if (svc_operand)  /* OABI.  */
	{
	  svc_number = svc_operand - 0x900000;
	}
      else /* EABI.  */
	{
	  svc_number = regcache_raw_get_unsigned (self->regcache, 7);
	}

      next_pc = pc + 4;
    }

  if (svc_number == ARM_SIGRETURN || svc_number == ARM_RT_SIGRETURN)
    {
      /* SIGRETURN or RT_SIGRETURN may affect the arm thumb mode, so
	 update IS_THUMB.   */
      next_pc = arm_linux_sigreturn_next_pc (regcache, svc_number, &is_thumb);
    }

  /* Addresses for calling Thumb functions have the bit 0 set.  */
  if (is_thumb)
    next_pc = MAKE_THUMB_ADDR (next_pc);

  return next_pc;
}


/* Insert a single step breakpoint at the next executed instruction.  */

static std::vector<CORE_ADDR>
arm_linux_software_single_step (struct regcache *regcache)
{
  struct gdbarch *gdbarch = regcache->arch ();
  struct arm_get_next_pcs next_pcs_ctx;

  /* If the target does have hardware single step, GDB doesn't have
     to bother software single step.  */
  if (target_can_do_single_step () == 1)
    return {};

  arm_get_next_pcs_ctor (&next_pcs_ctx,
			 &arm_linux_get_next_pcs_ops,
			 gdbarch_byte_order (gdbarch),
			 gdbarch_byte_order_for_code (gdbarch),
			 1,
			 regcache);

  std::vector<CORE_ADDR> next_pcs = arm_get_next_pcs (&next_pcs_ctx);

  for (CORE_ADDR &pc_ref : next_pcs)
    pc_ref = gdbarch_addr_bits_remove (gdbarch, pc_ref);

  return next_pcs;
}

/* Support for displaced stepping of Linux SVC instructions.  */

static void
arm_linux_cleanup_svc (struct gdbarch *gdbarch,
		       struct regcache *regs,
		       arm_displaced_step_copy_insn_closure *dsc)
{
  ULONGEST apparent_pc;
  int within_scratch;

  regcache_cooked_read_unsigned (regs, ARM_PC_REGNUM, &apparent_pc);

  within_scratch = (apparent_pc >= dsc->scratch_base
		    && apparent_pc < (dsc->scratch_base
				      + ARM_DISPLACED_MODIFIED_INSNS * 4 + 4));

  displaced_debug_printf ("PC is apparently %.8lx after SVC step %s",
			  (unsigned long) apparent_pc,
			  (within_scratch
			   ? "(within scratch space)"
			   : "(outside scratch space)"));

  if (within_scratch)
    displaced_write_reg (regs, dsc, ARM_PC_REGNUM,
			 dsc->insn_addr + dsc->insn_size, BRANCH_WRITE_PC);
}

static int
arm_linux_copy_svc (struct gdbarch *gdbarch, struct regcache *regs,
		    arm_displaced_step_copy_insn_closure *dsc)
{
  CORE_ADDR return_to = 0;

  frame_info_ptr frame;
  unsigned int svc_number = displaced_read_reg (regs, dsc, 7);
  int is_sigreturn = 0;
  int is_thumb;

  frame = get_current_frame ();

  is_sigreturn = arm_linux_sigreturn_return_addr(frame, svc_number,
						 &return_to, &is_thumb);
  if (is_sigreturn)
    {
      struct symtab_and_line sal;

      displaced_debug_printf ("found sigreturn/rt_sigreturn SVC call.  "
			      "PC in frame = %lx",
			      (unsigned long) get_frame_pc (frame));

      displaced_debug_printf ("unwind pc = %lx.  Setting momentary breakpoint.",
			      (unsigned long) return_to);

      gdb_assert (inferior_thread ()->control.step_resume_breakpoint
		  == NULL);

      sal = find_pc_line (return_to, 0);
      sal.pc = return_to;
      sal.section = find_pc_overlay (return_to);
      sal.explicit_pc = 1;

      frame = get_prev_frame (frame);

      if (frame)
	{
	  inferior_thread ()->control.step_resume_breakpoint
	    = set_momentary_breakpoint (gdbarch, sal, get_frame_id (frame),
					bp_step_resume).release ();

	  /* set_momentary_breakpoint invalidates FRAME.  */
	  frame = NULL;

	  /* We need to make sure we actually insert the momentary
	     breakpoint set above.  */
	  insert_breakpoints ();
	}
      else
	displaced_debug_printf ("couldn't find previous frame to set momentary "
				"breakpoint for sigreturn/rt_sigreturn");
    }
  else
    displaced_debug_printf ("found SVC call");

  /* Preparation: If we detect sigreturn, set momentary breakpoint at resume
		  location, else nothing.
     Insn: unmodified svc.
     Cleanup: if pc lands in scratch space, pc <- insn_addr + insn_size
	      else leave pc alone.  */


  dsc->cleanup = &arm_linux_cleanup_svc;
  /* Pretend we wrote to the PC, so cleanup doesn't set PC to the next
     instruction.  */
  dsc->wrote_to_pc = 1;

  return 0;
}


/* The following two functions implement single-stepping over calls to Linux
   kernel helper routines, which perform e.g. atomic operations on architecture
   variants which don't support them natively.

   When this function is called, the PC will be pointing at the kernel helper
   (at an address inaccessible to GDB), and r14 will point to the return
   address.  Displaced stepping always executes code in the copy area:
   so, make the copy-area instruction branch back to the kernel helper (the
   "from" address), and make r14 point to the breakpoint in the copy area.  In
   that way, we regain control once the kernel helper returns, and can clean
   up appropriately (as if we had just returned from the kernel helper as it
   would have been called from the non-displaced location).  */

static void
cleanup_kernel_helper_return (struct gdbarch *gdbarch,
			      struct regcache *regs,
			      arm_displaced_step_copy_insn_closure *dsc)
{
  displaced_write_reg (regs, dsc, ARM_LR_REGNUM, dsc->tmp[0], CANNOT_WRITE_PC);
  displaced_write_reg (regs, dsc, ARM_PC_REGNUM, dsc->tmp[0], BRANCH_WRITE_PC);
}

static void
arm_catch_kernel_helper_return (struct gdbarch *gdbarch, CORE_ADDR from,
				CORE_ADDR to, struct regcache *regs,
				arm_displaced_step_copy_insn_closure *dsc)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  dsc->numinsns = 1;
  dsc->insn_addr = from;
  dsc->cleanup = &cleanup_kernel_helper_return;
  /* Say we wrote to the PC, else cleanup will set PC to the next
     instruction in the helper, which isn't helpful.  */
  dsc->wrote_to_pc = 1;

  /* Preparation: tmp[0] <- r14
		  r14 <- <scratch space>+4
		  *(<scratch space>+8) <- from
     Insn: ldr pc, [r14, #4]
     Cleanup: r14 <- tmp[0], pc <- tmp[0].  */

  dsc->tmp[0] = displaced_read_reg (regs, dsc, ARM_LR_REGNUM);
  displaced_write_reg (regs, dsc, ARM_LR_REGNUM, (ULONGEST) to + 4,
		       CANNOT_WRITE_PC);
  write_memory_unsigned_integer (to + 8, 4, byte_order, from);

  dsc->modinsn[0] = 0xe59ef004;  /* ldr pc, [lr, #4].  */
}

/* Linux-specific displaced step instruction copying function.  Detects when
   the program has stepped into a Linux kernel helper routine (which must be
   handled as a special case).  */

static displaced_step_copy_insn_closure_up
arm_linux_displaced_step_copy_insn (struct gdbarch *gdbarch,
				    CORE_ADDR from, CORE_ADDR to,
				    struct regcache *regs)
{
  std::unique_ptr<arm_displaced_step_copy_insn_closure> dsc
    (new arm_displaced_step_copy_insn_closure);

  /* Detect when we enter an (inaccessible by GDB) Linux kernel helper, and
     stop at the return location.  */
  if (from > 0xffff0000)
    {
      displaced_debug_printf ("detected kernel helper at %.8lx",
			      (unsigned long) from);

      arm_catch_kernel_helper_return (gdbarch, from, to, regs, dsc.get ());
    }
  else
    {
      /* Override the default handling of SVC instructions.  */
      dsc->u.svc.copy_svc_os = arm_linux_copy_svc;

      arm_process_displaced_insn (gdbarch, from, to, regs, dsc.get ());
    }

  arm_displaced_init_closure (gdbarch, from, to, dsc.get ());

  /* This is a work around for a problem with g++ 4.8.  */
  return displaced_step_copy_insn_closure_up (dsc.release ());
}

/* Implementation of `gdbarch_stap_is_single_operand', as defined in
   gdbarch.h.  */

static int
arm_stap_is_single_operand (struct gdbarch *gdbarch, const char *s)
{
  return (*s == '#' || *s == '$' || isdigit (*s) /* Literal number.  */
	  || *s == '[' /* Register indirection or
			  displacement.  */
	  || isalpha (*s)); /* Register value.  */
}

/* This routine is used to parse a special token in ARM's assembly.

   The special tokens parsed by it are:

      - Register displacement (e.g, [fp, #-8])

   It returns one if the special token has been parsed successfully,
   or zero if the current token is not considered special.  */

static expr::operation_up
arm_stap_parse_special_token (struct gdbarch *gdbarch,
			      struct stap_parse_info *p)
{
  if (*p->arg == '[')
    {
      /* Temporary holder for lookahead.  */
      const char *tmp = p->arg;
      char *endp;
      /* Used to save the register name.  */
      const char *start;
      char *regname;
      int len, offset;
      int got_minus = 0;
      long displacement;

      ++tmp;
      start = tmp;

      /* Register name.  */
      while (isalnum (*tmp))
	++tmp;

      if (*tmp != ',')
	return {};

      len = tmp - start;
      regname = (char *) alloca (len + 2);

      offset = 0;
      if (isdigit (*start))
	{
	  /* If we are dealing with a register whose name begins with a
	     digit, it means we should prefix the name with the letter
	     `r', because GDB expects this name pattern.  Otherwise (e.g.,
	     we are dealing with the register `fp'), we don't need to
	     add such a prefix.  */
	  regname[0] = 'r';
	  offset = 1;
	}

      strncpy (regname + offset, start, len);
      len += offset;
      regname[len] = '\0';

      if (user_reg_map_name_to_regnum (gdbarch, regname, len) == -1)
	error (_("Invalid register name `%s' on expression `%s'."),
	       regname, p->saved_arg);

      ++tmp;
      tmp = skip_spaces (tmp);
      if (*tmp == '#' || *tmp == '$')
	++tmp;

      if (*tmp == '-')
	{
	  ++tmp;
	  got_minus = 1;
	}

      displacement = strtol (tmp, &endp, 10);
      tmp = endp;

      /* Skipping last `]'.  */
      if (*tmp++ != ']')
	return {};
      p->arg = tmp;

      using namespace expr;

      /* The displacement.  */
      struct type *long_type = builtin_type (gdbarch)->builtin_long;
      if (got_minus)
	displacement = -displacement;
      operation_up disp = make_operation<long_const_operation> (long_type,
								displacement);

      /* The register name.  */
      operation_up reg
	= make_operation<register_operation> (regname);

      operation_up sum
	= make_operation<add_operation> (std::move (reg), std::move (disp));

      /* Casting to the expected type.  */
      struct type *arg_ptr_type = lookup_pointer_type (p->arg_type);
      sum = make_operation<unop_cast_operation> (std::move (sum),
						 arg_ptr_type);
      return make_operation<unop_ind_operation> (std::move (sum));
    }

  return {};
}

/* ARM process record-replay constructs: syscall, signal etc.  */

static linux_record_tdep arm_linux_record_tdep;

/* arm_canonicalize_syscall maps from the native arm Linux set
   of syscall ids into a canonical set of syscall ids used by
   process record.  */

static enum gdb_syscall
arm_canonicalize_syscall (int syscall)
{
  switch (syscall)
    {
    case 0: return gdb_sys_restart_syscall;
    case 1: return gdb_sys_exit;
    case 2: return gdb_sys_fork;
    case 3: return gdb_sys_read;
    case 4: return gdb_sys_write;
    case 5: return gdb_sys_open;
    case 6: return gdb_sys_close;
    case 8: return gdb_sys_creat;
    case 9: return gdb_sys_link;
    case 10: return gdb_sys_unlink;
    case arm_sys_execve: return gdb_sys_execve;
    case 12: return gdb_sys_chdir;
    case 13: return gdb_sys_time;
    case 14: return gdb_sys_mknod;
    case 15: return gdb_sys_chmod;
    case 16: return gdb_sys_lchown16;
    case 19: return gdb_sys_lseek;
    case 20: return gdb_sys_getpid;
    case 21: return gdb_sys_mount;
    case 22: return gdb_sys_oldumount;
    case 23: return gdb_sys_setuid16;
    case 24: return gdb_sys_getuid16;
    case 25: return gdb_sys_stime;
    case 26: return gdb_sys_ptrace;
    case 27: return gdb_sys_alarm;
    case 29: return gdb_sys_pause;
    case 30: return gdb_sys_utime;
    case 33: return gdb_sys_access;
    case 34: return gdb_sys_nice;
    case 36: return gdb_sys_sync;
    case 37: return gdb_sys_kill;
    case 38: return gdb_sys_rename;
    case 39: return gdb_sys_mkdir;
    case 40: return gdb_sys_rmdir;
    case 41: return gdb_sys_dup;
    case 42: return gdb_sys_pipe;
    case 43: return gdb_sys_times;
    case 45: return gdb_sys_brk;
    case 46: return gdb_sys_setgid16;
    case 47: return gdb_sys_getgid16;
    case 49: return gdb_sys_geteuid16;
    case 50: return gdb_sys_getegid16;
    case 51: return gdb_sys_acct;
    case 52: return gdb_sys_umount;
    case 54: return gdb_sys_ioctl;
    case 55: return gdb_sys_fcntl;
    case 57: return gdb_sys_setpgid;
    case 60: return gdb_sys_umask;
    case 61: return gdb_sys_chroot;
    case 62: return gdb_sys_ustat;
    case 63: return gdb_sys_dup2;
    case 64: return gdb_sys_getppid;
    case 65: return gdb_sys_getpgrp;
    case 66: return gdb_sys_setsid;
    case 67: return gdb_sys_sigaction;
    case 70: return gdb_sys_setreuid16;
    case 71: return gdb_sys_setregid16;
    case 72: return gdb_sys_sigsuspend;
    case 73: return gdb_sys_sigpending;
    case 74: return gdb_sys_sethostname;
    case 75: return gdb_sys_setrlimit;
    case 76: return gdb_sys_getrlimit;
    case 77: return gdb_sys_getrusage;
    case 78: return gdb_sys_gettimeofday;
    case 79: return gdb_sys_settimeofday;
    case 80: return gdb_sys_getgroups16;
    case 81: return gdb_sys_setgroups16;
    case 82: return gdb_sys_select;
    case 83: return gdb_sys_symlink;
    case 85: return gdb_sys_readlink;
    case 86: return gdb_sys_uselib;
    case 87: return gdb_sys_swapon;
    case 88: return gdb_sys_reboot;
    case 89: return gdb_old_readdir;
    case 90: return gdb_old_mmap;
    case 91: return gdb_sys_munmap;
    case 92: return gdb_sys_truncate;
    case 93: return gdb_sys_ftruncate;
    case 94: return gdb_sys_fchmod;
    case 95: return gdb_sys_fchown16;
    case 96: return gdb_sys_getpriority;
    case 97: return gdb_sys_setpriority;
    case 99: return gdb_sys_statfs;
    case 100: return gdb_sys_fstatfs;
    case 102: return gdb_sys_socketcall;
    case 103: return gdb_sys_syslog;
    case 104: return gdb_sys_setitimer;
    case 105: return gdb_sys_getitimer;
    case 106: return gdb_sys_stat;
    case 107: return gdb_sys_lstat;
    case 108: return gdb_sys_fstat;
    case 111: return gdb_sys_vhangup;
    case 113: /* sys_syscall */
      return gdb_sys_no_syscall;
    case 114: return gdb_sys_wait4;
    case 115: return gdb_sys_swapoff;
    case 116: return gdb_sys_sysinfo;
    case 117: return gdb_sys_ipc;
    case 118: return gdb_sys_fsync;
    case 119: return gdb_sys_sigreturn;
    case 120: return gdb_sys_clone;
    case 121: return gdb_sys_setdomainname;
    case 122: return gdb_sys_uname;
    case 124: return gdb_sys_adjtimex;
    case 125: return gdb_sys_mprotect;
    case 126: return gdb_sys_sigprocmask;
    case 128: return gdb_sys_init_module;
    case 129: return gdb_sys_delete_module;
    case 131: return gdb_sys_quotactl;
    case 132: return gdb_sys_getpgid;
    case 133: return gdb_sys_fchdir;
    case 134: return gdb_sys_bdflush;
    case 135: return gdb_sys_sysfs;
    case 136: return gdb_sys_personality;
    case 138: return gdb_sys_setfsuid16;
    case 139: return gdb_sys_setfsgid16;
    case 140: return gdb_sys_llseek;
    case 141: return gdb_sys_getdents;
    case 142: return gdb_sys_select;
    case 143: return gdb_sys_flock;
    case 144: return gdb_sys_msync;
    case 145: return gdb_sys_readv;
    case 146: return gdb_sys_writev;
    case 147: return gdb_sys_getsid;
    case 148: return gdb_sys_fdatasync;
    case 149: return gdb_sys_sysctl;
    case 150: return gdb_sys_mlock;
    case 151: return gdb_sys_munlock;
    case 152: return gdb_sys_mlockall;
    case 153: return gdb_sys_munlockall;
    case 154: return gdb_sys_sched_setparam;
    case 155: return gdb_sys_sched_getparam;
    case 156: return gdb_sys_sched_setscheduler;
    case 157: return gdb_sys_sched_getscheduler;
    case 158: return gdb_sys_sched_yield;
    case 159: return gdb_sys_sched_get_priority_max;
    case 160: return gdb_sys_sched_get_priority_min;
    case 161: return gdb_sys_sched_rr_get_interval;
    case 162: return gdb_sys_nanosleep;
    case 163: return gdb_sys_mremap;
    case 164: return gdb_sys_setresuid16;
    case 165: return gdb_sys_getresuid16;
    case 168: return gdb_sys_poll;
    case 169: return gdb_sys_nfsservctl;
    case 170: return gdb_sys_setresgid;
    case 171: return gdb_sys_getresgid;
    case 172: return gdb_sys_prctl;
    case 173: return gdb_sys_rt_sigreturn;
    case 174: return gdb_sys_rt_sigaction;
    case 175: return gdb_sys_rt_sigprocmask;
    case 176: return gdb_sys_rt_sigpending;
    case 177: return gdb_sys_rt_sigtimedwait;
    case 178: return gdb_sys_rt_sigqueueinfo;
    case 179: return gdb_sys_rt_sigsuspend;
    case 180: return gdb_sys_pread64;
    case 181: return gdb_sys_pwrite64;
    case 182: return gdb_sys_chown;
    case 183: return gdb_sys_getcwd;
    case 184: return gdb_sys_capget;
    case 185: return gdb_sys_capset;
    case 186: return gdb_sys_sigaltstack;
    case 187: return gdb_sys_sendfile;
    case 190: return gdb_sys_vfork;
    case 191: return gdb_sys_getrlimit;
    case 192: return gdb_sys_mmap2;
    case 193: return gdb_sys_truncate64;
    case 194: return gdb_sys_ftruncate64;
    case 195: return gdb_sys_stat64;
    case 196: return gdb_sys_lstat64;
    case 197: return gdb_sys_fstat64;
    case 198: return gdb_sys_lchown;
    case 199: return gdb_sys_getuid;
    case 200: return gdb_sys_getgid;
    case 201: return gdb_sys_geteuid;
    case 202: return gdb_sys_getegid;
    case 203: return gdb_sys_setreuid;
    case 204: return gdb_sys_setregid;
    case 205: return gdb_sys_getgroups;
    case 206: return gdb_sys_setgroups;
    case 207: return gdb_sys_fchown;
    case 208: return gdb_sys_setresuid;
    case 209: return gdb_sys_getresuid;
    case 210: return gdb_sys_setresgid;
    case 211: return gdb_sys_getresgid;
    case 212: return gdb_sys_chown;
    case 213: return gdb_sys_setuid;
    case 214: return gdb_sys_setgid;
    case 215: return gdb_sys_setfsuid;
    case 216: return gdb_sys_setfsgid;
    case 217: return gdb_sys_getdents64;
    case 218: return gdb_sys_pivot_root;
    case 219: return gdb_sys_mincore;
    case 220: return gdb_sys_madvise;
    case 221: return gdb_sys_fcntl64;
    case 224: return gdb_sys_gettid;
    case 225: return gdb_sys_readahead;
    case 226: return gdb_sys_setxattr;
    case 227: return gdb_sys_lsetxattr;
    case 228: return gdb_sys_fsetxattr;
    case 229: return gdb_sys_getxattr;
    case 230: return gdb_sys_lgetxattr;
    case 231: return gdb_sys_fgetxattr;
    case 232: return gdb_sys_listxattr;
    case 233: return gdb_sys_llistxattr;
    case 234: return gdb_sys_flistxattr;
    case 235: return gdb_sys_removexattr;
    case 236: return gdb_sys_lremovexattr;
    case 237: return gdb_sys_fremovexattr;
    case 238: return gdb_sys_tkill;
    case 239: return gdb_sys_sendfile64;
    case 240: return gdb_sys_futex;
    case 241: return gdb_sys_sched_setaffinity;
    case 242: return gdb_sys_sched_getaffinity;
    case 243: return gdb_sys_io_setup;
    case 244: return gdb_sys_io_destroy;
    case 245: return gdb_sys_io_getevents;
    case 246: return gdb_sys_io_submit;
    case 247: return gdb_sys_io_cancel;
    case 248: return gdb_sys_exit_group;
    case 249: return gdb_sys_lookup_dcookie;
    case 250: return gdb_sys_epoll_create;
    case 251: return gdb_sys_epoll_ctl;
    case 252: return gdb_sys_epoll_wait;
    case 253: return gdb_sys_remap_file_pages;
    case 256: return gdb_sys_set_tid_address;
    case 257: return gdb_sys_timer_create;
    case 258: return gdb_sys_timer_settime;
    case 259: return gdb_sys_timer_gettime;
    case 260: return gdb_sys_timer_getoverrun;
    case 261: return gdb_sys_timer_delete;
    case 262: return gdb_sys_clock_settime;
    case 263: return gdb_sys_clock_gettime;
    case 264: return gdb_sys_clock_getres;
    case 265: return gdb_sys_clock_nanosleep;
    case 266: return gdb_sys_statfs64;
    case 267: return gdb_sys_fstatfs64;
    case 268: return gdb_sys_tgkill;
    case 269: return gdb_sys_utimes;
      /*
    case 270: return gdb_sys_arm_fadvise64_64;
    case 271: return gdb_sys_pciconfig_iobase;
    case 272: return gdb_sys_pciconfig_read;
    case 273: return gdb_sys_pciconfig_write;
      */
    case 274: return gdb_sys_mq_open;
    case 275: return gdb_sys_mq_unlink;
    case 276: return gdb_sys_mq_timedsend;
    case 277: return gdb_sys_mq_timedreceive;
    case 278: return gdb_sys_mq_notify;
    case 279: return gdb_sys_mq_getsetattr;
    case 280: return gdb_sys_waitid;
    case 281: return gdb_sys_socket;
    case 282: return gdb_sys_bind;
    case 283: return gdb_sys_connect;
    case 284: return gdb_sys_listen;
    case 285: return gdb_sys_accept;
    case 286: return gdb_sys_getsockname;
    case 287: return gdb_sys_getpeername;
    case 288: return gdb_sys_socketpair;
    case 289: /* send */ return gdb_sys_no_syscall;
    case 290: return gdb_sys_sendto;
    case 291: return gdb_sys_recv;
    case 292: return gdb_sys_recvfrom;
    case 293: return gdb_sys_shutdown;
    case 294: return gdb_sys_setsockopt;
    case 295: return gdb_sys_getsockopt;
    case 296: return gdb_sys_sendmsg;
    case 297: return gdb_sys_recvmsg;
    case 298: return gdb_sys_semop;
    case 299: return gdb_sys_semget;
    case 300: return gdb_sys_semctl;
    case 301: return gdb_sys_msgsnd;
    case 302: return gdb_sys_msgrcv;
    case 303: return gdb_sys_msgget;
    case 304: return gdb_sys_msgctl;
    case 305: return gdb_sys_shmat;
    case 306: return gdb_sys_shmdt;
    case 307: return gdb_sys_shmget;
    case 308: return gdb_sys_shmctl;
    case 309: return gdb_sys_add_key;
    case 310: return gdb_sys_request_key;
    case 311: return gdb_sys_keyctl;
    case 312: return gdb_sys_semtimedop;
    case 313: /* vserver */ return gdb_sys_no_syscall;
    case 314: return gdb_sys_ioprio_set;
    case 315: return gdb_sys_ioprio_get;
    case 316: return gdb_sys_inotify_init;
    case 317: return gdb_sys_inotify_add_watch;
    case 318: return gdb_sys_inotify_rm_watch;
    case 319: return gdb_sys_mbind;
    case 320: return gdb_sys_get_mempolicy;
    case 321: return gdb_sys_set_mempolicy;
    case 322: return gdb_sys_openat;
    case 323: return gdb_sys_mkdirat;
    case 324: return gdb_sys_mknodat;
    case 325: return gdb_sys_fchownat;
    case 326: return gdb_sys_futimesat;
    case 327: return gdb_sys_fstatat64;
    case 328: return gdb_sys_unlinkat;
    case 329: return gdb_sys_renameat;
    case 330: return gdb_sys_linkat;
    case 331: return gdb_sys_symlinkat;
    case 332: return gdb_sys_readlinkat;
    case 333: return gdb_sys_fchmodat;
    case 334: return gdb_sys_faccessat;
    case 335: return gdb_sys_pselect6;
    case 336: return gdb_sys_ppoll;
    case 337: return gdb_sys_unshare;
    case 338: return gdb_sys_set_robust_list;
    case 339: return gdb_sys_get_robust_list;
    case 340: return gdb_sys_splice;
    /*case 341: return gdb_sys_arm_sync_file_range;*/
    case 342: return gdb_sys_tee;
    case 343: return gdb_sys_vmsplice;
    case 344: return gdb_sys_move_pages;
    case 345: return gdb_sys_getcpu;
    case 346: return gdb_sys_epoll_pwait;
    case 347: return gdb_sys_kexec_load;
      /*
    case 348: return gdb_sys_utimensat;
    case 349: return gdb_sys_signalfd;
    case 350: return gdb_sys_timerfd_create;
    case 351: return gdb_sys_eventfd;
      */
    case 352: return gdb_sys_fallocate;
      /*
    case 353: return gdb_sys_timerfd_settime;
    case 354: return gdb_sys_timerfd_gettime;
    case 355: return gdb_sys_signalfd4;
      */
    case 356: return gdb_sys_eventfd2;
    case 357: return gdb_sys_epoll_create1;
    case 358: return gdb_sys_dup3;
    case 359: return gdb_sys_pipe2;
    case 360: return gdb_sys_inotify_init1;
      /*
    case 361: return gdb_sys_preadv;
    case 362: return gdb_sys_pwritev;
    case 363: return gdb_sys_rt_tgsigqueueinfo;
    case 364: return gdb_sys_perf_event_open;
    case 365: return gdb_sys_recvmmsg;
    case 366: return gdb_sys_accept4;
    case 367: return gdb_sys_fanotify_init;
    case 368: return gdb_sys_fanotify_mark;
    case 369: return gdb_sys_prlimit64;
    case 370: return gdb_sys_name_to_handle_at;
    case 371: return gdb_sys_open_by_handle_at;
    case 372: return gdb_sys_clock_adjtime;
    case 373: return gdb_sys_syncfs;
    case 374: return gdb_sys_sendmmsg;
    case 375: return gdb_sys_setns;
    case 376: return gdb_sys_process_vm_readv;
    case 377: return gdb_sys_process_vm_writev;
    case 378: return gdb_sys_kcmp;
    case 379: return gdb_sys_finit_module;
      */
    case 384: return gdb_sys_getrandom;
    case 983041: /* ARM_breakpoint */ return gdb_sys_no_syscall;
    case 983042: /* ARM_cacheflush */ return gdb_sys_no_syscall;
    case 983043: /* ARM_usr26 */ return gdb_sys_no_syscall;
    case 983044: /* ARM_usr32 */ return gdb_sys_no_syscall;
    case 983045: /* ARM_set_tls */ return gdb_sys_no_syscall;
    default: return gdb_sys_no_syscall;
    }
}

/* Record all registers but PC register for process-record.  */

static int
arm_all_but_pc_registers_record (struct regcache *regcache)
{
  int i;

  for (i = 0; i < ARM_PC_REGNUM; i++)
    {
      if (record_full_arch_list_add_reg (regcache, ARM_A1_REGNUM + i))
	return -1;
    }

  if (record_full_arch_list_add_reg (regcache, ARM_PS_REGNUM))
    return -1;

  return 0;
}

/* Handler for arm system call instruction recording.  */

static int
arm_linux_syscall_record (struct regcache *regcache, unsigned long svc_number)
{
  int ret = 0;
  enum gdb_syscall syscall_gdb;

  syscall_gdb = arm_canonicalize_syscall (svc_number);

  if (syscall_gdb == gdb_sys_no_syscall)
    {
      gdb_printf (gdb_stderr,
		  _("Process record and replay target doesn't "
		    "support syscall number %s\n"),
		  plongest (svc_number));
      return -1;
    }

  if (syscall_gdb == gdb_sys_sigreturn
      || syscall_gdb == gdb_sys_rt_sigreturn)
   {
     if (arm_all_but_pc_registers_record (regcache))
       return -1;
     return 0;
   }

  ret = record_linux_system_call (syscall_gdb, regcache,
				  &arm_linux_record_tdep);
  if (ret != 0)
    return ret;

  /* Record the return value of the system call.  */
  if (record_full_arch_list_add_reg (regcache, ARM_A1_REGNUM))
    return -1;
  /* Record LR.  */
  if (record_full_arch_list_add_reg (regcache, ARM_LR_REGNUM))
    return -1;
  /* Record CPSR.  */
  if (record_full_arch_list_add_reg (regcache, ARM_PS_REGNUM))
    return -1;

  return 0;
}

/* Implement the skip_trampoline_code gdbarch method.  */

static CORE_ADDR
arm_linux_skip_trampoline_code (frame_info_ptr frame, CORE_ADDR pc)
{
  CORE_ADDR target_pc = arm_skip_stub (frame, pc);

  if (target_pc != 0)
    return target_pc;

  return find_solib_trampoline_target (frame, pc);
}

/* Implement the gcc_target_options gdbarch method.  */

static std::string
arm_linux_gcc_target_options (struct gdbarch *gdbarch)
{
  /* GCC doesn't know "-m32".  */
  return {};
}

static void
arm_linux_init_abi (struct gdbarch_info info,
		    struct gdbarch *gdbarch)
{
  static const char *const stap_integer_prefixes[] = { "#", "$", "", NULL };
  static const char *const stap_register_prefixes[] = { "r", NULL };
  static const char *const stap_register_indirection_prefixes[] = { "[",
								    NULL };
  static const char *const stap_register_indirection_suffixes[] = { "]",
								    NULL };
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  linux_init_abi (info, gdbarch, 1);

  tdep->lowest_pc = 0x8000;
  if (info.byte_order_for_code == BFD_ENDIAN_BIG)
    {
      if (tdep->arm_abi == ARM_ABI_AAPCS)
	tdep->arm_breakpoint = eabi_linux_arm_be_breakpoint;
      else
	tdep->arm_breakpoint = arm_linux_arm_be_breakpoint;
      tdep->thumb_breakpoint = arm_linux_thumb_be_breakpoint;
      tdep->thumb2_breakpoint = arm_linux_thumb2_be_breakpoint;
    }
  else
    {
      if (tdep->arm_abi == ARM_ABI_AAPCS)
	tdep->arm_breakpoint = eabi_linux_arm_le_breakpoint;
      else
	tdep->arm_breakpoint = arm_linux_arm_le_breakpoint;
      tdep->thumb_breakpoint = arm_linux_thumb_le_breakpoint;
      tdep->thumb2_breakpoint = arm_linux_thumb2_le_breakpoint;
    }
  tdep->arm_breakpoint_size = sizeof (arm_linux_arm_le_breakpoint);
  tdep->thumb_breakpoint_size = sizeof (arm_linux_thumb_le_breakpoint);
  tdep->thumb2_breakpoint_size = sizeof (arm_linux_thumb2_le_breakpoint);

  if (tdep->fp_model == ARM_FLOAT_AUTO)
    tdep->fp_model = ARM_FLOAT_FPA;

  switch (tdep->fp_model)
    {
    case ARM_FLOAT_FPA:
      tdep->jb_pc = ARM_LINUX_JB_PC_FPA;
      break;
    case ARM_FLOAT_SOFT_FPA:
    case ARM_FLOAT_SOFT_VFP:
    case ARM_FLOAT_VFP:
      tdep->jb_pc = ARM_LINUX_JB_PC_EABI;
      break;
    default:
      internal_error
	(_("arm_linux_init_abi: Floating point model not supported"));
      break;
    }
  tdep->jb_elt_size = ARM_LINUX_JB_ELEMENT_SIZE;

  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, linux_ilp32_fetch_link_map_offsets);

  /* Single stepping.  */
  set_gdbarch_software_single_step (gdbarch, arm_linux_software_single_step);

  /* Shared library handling.  */
  set_gdbarch_skip_trampoline_code (gdbarch, arm_linux_skip_trampoline_code);
  set_gdbarch_skip_solib_resolver (gdbarch, glibc_skip_solib_resolver);

  /* Enable TLS support.  */
  set_gdbarch_fetch_tls_load_module_address (gdbarch,
					     svr4_fetch_objfile_link_map);

  tramp_frame_prepend_unwinder (gdbarch,
				&arm_linux_sigreturn_tramp_frame);
  tramp_frame_prepend_unwinder (gdbarch,
				&arm_linux_rt_sigreturn_tramp_frame);
  tramp_frame_prepend_unwinder (gdbarch,
				&arm_eabi_linux_sigreturn_tramp_frame);
  tramp_frame_prepend_unwinder (gdbarch,
				&arm_eabi_linux_rt_sigreturn_tramp_frame);
  tramp_frame_prepend_unwinder (gdbarch,
				&thumb2_eabi_linux_sigreturn_tramp_frame);
  tramp_frame_prepend_unwinder (gdbarch,
				&thumb2_eabi_linux_rt_sigreturn_tramp_frame);
  tramp_frame_prepend_unwinder (gdbarch,
				&arm_linux_restart_syscall_tramp_frame);
  tramp_frame_prepend_unwinder (gdbarch,
				&arm_kernel_linux_restart_syscall_tramp_frame);

  /* Core file support.  */
  set_gdbarch_iterate_over_regset_sections
    (gdbarch, arm_linux_iterate_over_regset_sections);
  set_gdbarch_core_read_description (gdbarch, arm_linux_core_read_description);

  /* Displaced stepping.  */
  set_gdbarch_displaced_step_copy_insn (gdbarch,
					arm_linux_displaced_step_copy_insn);
  set_gdbarch_displaced_step_fixup (gdbarch, arm_displaced_step_fixup);

  /* Reversible debugging, process record.  */
  set_gdbarch_process_record (gdbarch, arm_process_record);

  /* SystemTap functions.  */
  set_gdbarch_stap_integer_prefixes (gdbarch, stap_integer_prefixes);
  set_gdbarch_stap_register_prefixes (gdbarch, stap_register_prefixes);
  set_gdbarch_stap_register_indirection_prefixes (gdbarch,
					  stap_register_indirection_prefixes);
  set_gdbarch_stap_register_indirection_suffixes (gdbarch,
					  stap_register_indirection_suffixes);
  set_gdbarch_stap_gdb_register_prefix (gdbarch, "r");
  set_gdbarch_stap_is_single_operand (gdbarch, arm_stap_is_single_operand);
  set_gdbarch_stap_parse_special_token (gdbarch,
					arm_stap_parse_special_token);

  /* `catch syscall' */
  set_xml_syscall_file_name (gdbarch, "syscalls/arm-linux.xml");
  set_gdbarch_get_syscall_number (gdbarch, arm_linux_get_syscall_number);

  /* Syscall record.  */
  tdep->arm_syscall_record = arm_linux_syscall_record;

  /* Initialize the arm_linux_record_tdep.  */
  /* These values are the size of the type that will be used in a system
     call.  They are obtained from Linux Kernel source.  */
  arm_linux_record_tdep.size_pointer
    = gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT;
  arm_linux_record_tdep.size__old_kernel_stat = 32;
  arm_linux_record_tdep.size_tms = 16;
  arm_linux_record_tdep.size_loff_t = 8;
  arm_linux_record_tdep.size_flock = 16;
  arm_linux_record_tdep.size_oldold_utsname = 45;
  arm_linux_record_tdep.size_ustat = 20;
  arm_linux_record_tdep.size_old_sigaction = 16;
  arm_linux_record_tdep.size_old_sigset_t = 4;
  arm_linux_record_tdep.size_rlimit = 8;
  arm_linux_record_tdep.size_rusage = 72;
  arm_linux_record_tdep.size_timeval = 8;
  arm_linux_record_tdep.size_timezone = 8;
  arm_linux_record_tdep.size_old_gid_t = 2;
  arm_linux_record_tdep.size_old_uid_t = 2;
  arm_linux_record_tdep.size_fd_set = 128;
  arm_linux_record_tdep.size_old_dirent = 268;
  arm_linux_record_tdep.size_statfs = 64;
  arm_linux_record_tdep.size_statfs64 = 84;
  arm_linux_record_tdep.size_sockaddr = 16;
  arm_linux_record_tdep.size_int
    = gdbarch_int_bit (gdbarch) / TARGET_CHAR_BIT;
  arm_linux_record_tdep.size_long
    = gdbarch_long_bit (gdbarch) / TARGET_CHAR_BIT;
  arm_linux_record_tdep.size_ulong
    = gdbarch_long_bit (gdbarch) / TARGET_CHAR_BIT;
  arm_linux_record_tdep.size_msghdr = 28;
  arm_linux_record_tdep.size_itimerval = 16;
  arm_linux_record_tdep.size_stat = 88;
  arm_linux_record_tdep.size_old_utsname = 325;
  arm_linux_record_tdep.size_sysinfo = 64;
  arm_linux_record_tdep.size_msqid_ds = 88;
  arm_linux_record_tdep.size_shmid_ds = 84;
  arm_linux_record_tdep.size_new_utsname = 390;
  arm_linux_record_tdep.size_timex = 128;
  arm_linux_record_tdep.size_mem_dqinfo = 24;
  arm_linux_record_tdep.size_if_dqblk = 68;
  arm_linux_record_tdep.size_fs_quota_stat = 68;
  arm_linux_record_tdep.size_timespec = 8;
  arm_linux_record_tdep.size_pollfd = 8;
  arm_linux_record_tdep.size_NFS_FHSIZE = 32;
  arm_linux_record_tdep.size_knfsd_fh = 132;
  arm_linux_record_tdep.size_TASK_COMM_LEN = 16;
  arm_linux_record_tdep.size_sigaction = 20;
  arm_linux_record_tdep.size_sigset_t = 8;
  arm_linux_record_tdep.size_siginfo_t = 128;
  arm_linux_record_tdep.size_cap_user_data_t = 12;
  arm_linux_record_tdep.size_stack_t = 12;
  arm_linux_record_tdep.size_off_t = arm_linux_record_tdep.size_long;
  arm_linux_record_tdep.size_stat64 = 96;
  arm_linux_record_tdep.size_gid_t = 4;
  arm_linux_record_tdep.size_uid_t = 4;
  arm_linux_record_tdep.size_PAGE_SIZE = 4096;
  arm_linux_record_tdep.size_flock64 = 24;
  arm_linux_record_tdep.size_user_desc = 16;
  arm_linux_record_tdep.size_io_event = 32;
  arm_linux_record_tdep.size_iocb = 64;
  arm_linux_record_tdep.size_epoll_event = 12;
  arm_linux_record_tdep.size_itimerspec
    = arm_linux_record_tdep.size_timespec * 2;
  arm_linux_record_tdep.size_mq_attr = 32;
  arm_linux_record_tdep.size_termios = 36;
  arm_linux_record_tdep.size_termios2 = 44;
  arm_linux_record_tdep.size_pid_t = 4;
  arm_linux_record_tdep.size_winsize = 8;
  arm_linux_record_tdep.size_serial_struct = 60;
  arm_linux_record_tdep.size_serial_icounter_struct = 80;
  arm_linux_record_tdep.size_hayes_esp_config = 12;
  arm_linux_record_tdep.size_size_t = 4;
  arm_linux_record_tdep.size_iovec = 8;
  arm_linux_record_tdep.size_time_t = 4;

  /* These values are the second argument of system call "sys_ioctl".
     They are obtained from Linux Kernel source.  */
  arm_linux_record_tdep.ioctl_TCGETS = 0x5401;
  arm_linux_record_tdep.ioctl_TCSETS = 0x5402;
  arm_linux_record_tdep.ioctl_TCSETSW = 0x5403;
  arm_linux_record_tdep.ioctl_TCSETSF = 0x5404;
  arm_linux_record_tdep.ioctl_TCGETA = 0x5405;
  arm_linux_record_tdep.ioctl_TCSETA = 0x5406;
  arm_linux_record_tdep.ioctl_TCSETAW = 0x5407;
  arm_linux_record_tdep.ioctl_TCSETAF = 0x5408;
  arm_linux_record_tdep.ioctl_TCSBRK = 0x5409;
  arm_linux_record_tdep.ioctl_TCXONC = 0x540a;
  arm_linux_record_tdep.ioctl_TCFLSH = 0x540b;
  arm_linux_record_tdep.ioctl_TIOCEXCL = 0x540c;
  arm_linux_record_tdep.ioctl_TIOCNXCL = 0x540d;
  arm_linux_record_tdep.ioctl_TIOCSCTTY = 0x540e;
  arm_linux_record_tdep.ioctl_TIOCGPGRP = 0x540f;
  arm_linux_record_tdep.ioctl_TIOCSPGRP = 0x5410;
  arm_linux_record_tdep.ioctl_TIOCOUTQ = 0x5411;
  arm_linux_record_tdep.ioctl_TIOCSTI = 0x5412;
  arm_linux_record_tdep.ioctl_TIOCGWINSZ = 0x5413;
  arm_linux_record_tdep.ioctl_TIOCSWINSZ = 0x5414;
  arm_linux_record_tdep.ioctl_TIOCMGET = 0x5415;
  arm_linux_record_tdep.ioctl_TIOCMBIS = 0x5416;
  arm_linux_record_tdep.ioctl_TIOCMBIC = 0x5417;
  arm_linux_record_tdep.ioctl_TIOCMSET = 0x5418;
  arm_linux_record_tdep.ioctl_TIOCGSOFTCAR = 0x5419;
  arm_linux_record_tdep.ioctl_TIOCSSOFTCAR = 0x541a;
  arm_linux_record_tdep.ioctl_FIONREAD = 0x541b;
  arm_linux_record_tdep.ioctl_TIOCINQ = arm_linux_record_tdep.ioctl_FIONREAD;
  arm_linux_record_tdep.ioctl_TIOCLINUX = 0x541c;
  arm_linux_record_tdep.ioctl_TIOCCONS = 0x541d;
  arm_linux_record_tdep.ioctl_TIOCGSERIAL = 0x541e;
  arm_linux_record_tdep.ioctl_TIOCSSERIAL = 0x541f;
  arm_linux_record_tdep.ioctl_TIOCPKT = 0x5420;
  arm_linux_record_tdep.ioctl_FIONBIO = 0x5421;
  arm_linux_record_tdep.ioctl_TIOCNOTTY = 0x5422;
  arm_linux_record_tdep.ioctl_TIOCSETD = 0x5423;
  arm_linux_record_tdep.ioctl_TIOCGETD = 0x5424;
  arm_linux_record_tdep.ioctl_TCSBRKP = 0x5425;
  arm_linux_record_tdep.ioctl_TIOCTTYGSTRUCT = 0x5426;
  arm_linux_record_tdep.ioctl_TIOCSBRK = 0x5427;
  arm_linux_record_tdep.ioctl_TIOCCBRK = 0x5428;
  arm_linux_record_tdep.ioctl_TIOCGSID = 0x5429;
  arm_linux_record_tdep.ioctl_TCGETS2 = 0x802c542a;
  arm_linux_record_tdep.ioctl_TCSETS2 = 0x402c542b;
  arm_linux_record_tdep.ioctl_TCSETSW2 = 0x402c542c;
  arm_linux_record_tdep.ioctl_TCSETSF2 = 0x402c542d;
  arm_linux_record_tdep.ioctl_TIOCGPTN = 0x80045430;
  arm_linux_record_tdep.ioctl_TIOCSPTLCK = 0x40045431;
  arm_linux_record_tdep.ioctl_FIONCLEX = 0x5450;
  arm_linux_record_tdep.ioctl_FIOCLEX = 0x5451;
  arm_linux_record_tdep.ioctl_FIOASYNC = 0x5452;
  arm_linux_record_tdep.ioctl_TIOCSERCONFIG = 0x5453;
  arm_linux_record_tdep.ioctl_TIOCSERGWILD = 0x5454;
  arm_linux_record_tdep.ioctl_TIOCSERSWILD = 0x5455;
  arm_linux_record_tdep.ioctl_TIOCGLCKTRMIOS = 0x5456;
  arm_linux_record_tdep.ioctl_TIOCSLCKTRMIOS = 0x5457;
  arm_linux_record_tdep.ioctl_TIOCSERGSTRUCT = 0x5458;
  arm_linux_record_tdep.ioctl_TIOCSERGETLSR = 0x5459;
  arm_linux_record_tdep.ioctl_TIOCSERGETMULTI = 0x545a;
  arm_linux_record_tdep.ioctl_TIOCSERSETMULTI = 0x545b;
  arm_linux_record_tdep.ioctl_TIOCMIWAIT = 0x545c;
  arm_linux_record_tdep.ioctl_TIOCGICOUNT = 0x545d;
  arm_linux_record_tdep.ioctl_TIOCGHAYESESP = 0x545e;
  arm_linux_record_tdep.ioctl_TIOCSHAYESESP = 0x545f;
  arm_linux_record_tdep.ioctl_FIOQSIZE = 0x5460;

  /* These values are the second argument of system call "sys_fcntl"
     and "sys_fcntl64".  They are obtained from Linux Kernel source.  */
  arm_linux_record_tdep.fcntl_F_GETLK = 5;
  arm_linux_record_tdep.fcntl_F_GETLK64 = 12;
  arm_linux_record_tdep.fcntl_F_SETLK64 = 13;
  arm_linux_record_tdep.fcntl_F_SETLKW64 = 14;

  arm_linux_record_tdep.arg1 = ARM_A1_REGNUM;
  arm_linux_record_tdep.arg2 = ARM_A1_REGNUM + 1;
  arm_linux_record_tdep.arg3 = ARM_A1_REGNUM + 2;
  arm_linux_record_tdep.arg4 = ARM_A1_REGNUM + 3;
  arm_linux_record_tdep.arg5 = ARM_A1_REGNUM + 4;
  arm_linux_record_tdep.arg6 = ARM_A1_REGNUM + 5;
  arm_linux_record_tdep.arg7 = ARM_A1_REGNUM + 6;

  set_gdbarch_gcc_target_options (gdbarch, arm_linux_gcc_target_options);
}

void _initialize_arm_linux_tdep ();
void
_initialize_arm_linux_tdep ()
{
  gdbarch_register_osabi (bfd_arch_arm, 0, GDB_OSABI_LINUX,
			  arm_linux_init_abi);
}
