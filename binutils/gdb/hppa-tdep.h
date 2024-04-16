/* Target-dependent code for the HP PA-RISC architecture.

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

#ifndef HPPA_TDEP_H
#define HPPA_TDEP_H

#include "gdbarch.h"

struct trad_frame_saved_reg;
struct objfile;
struct shobj;

/* Register numbers of various important registers.  */

enum hppa_regnum
{
  HPPA_R0_REGNUM = 0,		/* Doesn't actually exist, used as base for
				   other r registers.  */
  HPPA_R1_REGNUM = 1,
  HPPA_FLAGS_REGNUM = 0,	/* Various status flags */
  HPPA_RP_REGNUM = 2,		/* return pointer */
  HPPA_FP_REGNUM = 3,		/* The ABI's frame pointer, when used */
  HPPA_DP_REGNUM = 27,
  HPPA_RET0_REGNUM = 28,
  HPPA_RET1_REGNUM = 29,
  HPPA_SP_REGNUM = 30,		/* Stack pointer.  */
  HPPA_R31_REGNUM = 31,
  HPPA_SAR_REGNUM = 32,		/* Shift Amount Register */
  HPPA_IPSW_REGNUM = 41,	/* Interrupt Processor Status Word */
  HPPA_PCOQ_HEAD_REGNUM = 33,	/* instruction offset queue head */
  HPPA_PCSQ_HEAD_REGNUM = 34,	/* instruction space queue head */
  HPPA_PCOQ_TAIL_REGNUM = 35,	/* instruction offset queue tail */
  HPPA_PCSQ_TAIL_REGNUM = 36,	/* instruction space queue tail */
  HPPA_EIEM_REGNUM = 37,	/* External Interrupt Enable Mask */
  HPPA_IIR_REGNUM = 38,		/* Interrupt Instruction Register */
  HPPA_ISR_REGNUM = 39,		/* Interrupt Space Register */
  HPPA_IOR_REGNUM = 40,		/* Interrupt Offset Register */
  HPPA_SR4_REGNUM = 43,		/* space register 4 */
  HPPA_SR0_REGNUM = 44,		/* space register 0 */
  HPPA_SR1_REGNUM = 45,		/* space register 1 */
  HPPA_SR2_REGNUM = 46,		/* space register 2 */
  HPPA_SR3_REGNUM = 47,		/* space register 3 */
  HPPA_SR5_REGNUM = 48,		/* space register 5 */
  HPPA_SR6_REGNUM = 49,		/* space register 6 */
  HPPA_SR7_REGNUM = 50,		/* space register 7 */
  HPPA_RCR_REGNUM = 51,		/* Recover Counter (also known as cr0) */
  HPPA_PID0_REGNUM = 52,	/* Protection ID */
  HPPA_PID1_REGNUM = 53,	/* Protection ID */
  HPPA_PID2_REGNUM = 55,	/* Protection ID */
  HPPA_PID3_REGNUM = 56,	/* Protection ID */
  HPPA_CCR_REGNUM = 54,		/* Coprocessor Configuration Register */
  HPPA_TR0_REGNUM = 57,		/* Temporary Registers (cr24 -> cr31) */
  HPPA_CR26_REGNUM = 59,
  HPPA_CR27_REGNUM = 60,	/* Base register for thread-local
				   storage, cr27 */
  HPPA_FP0_REGNUM = 64,		/* First floating-point.  */
  HPPA_FP4_REGNUM = 72,
  HPPA64_FP4_REGNUM = 68,
  HPPA_FP31R_REGNUM = 127,	/* Last floating-point.  */

  HPPA_ARG0_REGNUM = 26,	/* The first argument of a callee.  */
  HPPA_ARG1_REGNUM = 25,	/* The second argument of a callee.  */
  HPPA_ARG2_REGNUM = 24,	/* The third argument of a callee.  */
  HPPA_ARG3_REGNUM = 23		/* The fourth argument of a callee.  */
};

/* Instruction size.  */
#define HPPA_INSN_SIZE 4

/* Target-dependent structure in gdbarch.  */
struct hppa_gdbarch_tdep : gdbarch_tdep_base
{
  /* The number of bytes in an address.  For now, this field is designed
     to allow us to differentiate hppa32 from hppa64 targets.  */
  int bytes_per_address = 0;

  /* Is this an ELF target? This can be 64-bit HP-UX, or a 32/64-bit GNU/Linux
     system.  */
  int is_elf = 0;

  /* Given a function address, try to find the global pointer for the 
     corresponding shared object.  */
  CORE_ADDR (*find_global_pointer) (struct gdbarch *, struct value *) = nullptr;

  /* For shared libraries, each call goes through a small piece of
     trampoline code in the ".plt" section.  IN_SOLIB_CALL_TRAMPOLINE
     evaluates to nonzero if we are currently stopped in one of these.  */
  int (*in_solib_call_trampoline) (struct gdbarch *gdbarch,
				   CORE_ADDR pc) = nullptr;

  /* For targets that support multiple spaces, we may have additional stubs
     in the return path.  These stubs are internal to the ABI, and users are
     not interested in them.  If we detect that we are returning to a stub,
     adjust the pc to the real caller.  This improves the behavior of commands
     that traverse frames such as "up" and "finish".  */
  void (*unwind_adjust_stub) (frame_info_ptr this_frame, CORE_ADDR base,
			      trad_frame_saved_reg *saved_regs) = nullptr;

  /* These are solib-dependent methods.  They are really HPUX only, but
     we don't have a HPUX-specific tdep vector at the moment.  */
  CORE_ADDR (*solib_thread_start_addr) (shobj *so) = nullptr;
  CORE_ADDR (*solib_get_got_by_pc) (CORE_ADDR addr) = nullptr;
  CORE_ADDR (*solib_get_solib_by_pc) (CORE_ADDR addr) = nullptr;
  CORE_ADDR (*solib_get_text_base) (struct objfile *objfile) = nullptr;
};

/*
 * Unwind table and descriptor.
 */

struct unwind_table_entry
  {
    CORE_ADDR region_start;
    CORE_ADDR region_end;

    unsigned int Cannot_unwind:1;	/* 0 */
    unsigned int Millicode:1;	/* 1 */
    unsigned int Millicode_save_sr0:1;	/* 2 */
    unsigned int Region_description:2;	/* 3..4 */
    unsigned int reserved:1;	/* 5 */
    unsigned int Entry_SR:1;	/* 6 */
    unsigned int Entry_FR:4;	/* number saved *//* 7..10 */
    unsigned int Entry_GR:5;	/* number saved *//* 11..15 */
    unsigned int Args_stored:1;	/* 16 */
    unsigned int Variable_Frame:1;	/* 17 */
    unsigned int Separate_Package_Body:1;	/* 18 */
    unsigned int Frame_Extension_Millicode:1;	/* 19 */
    unsigned int Stack_Overflow_Check:1;	/* 20 */
    unsigned int Two_Instruction_SP_Increment:1;	/* 21 */
    unsigned int sr4export:1;	/* 22 */
    unsigned int cxx_info:1;	/* 23 */
    unsigned int cxx_try_catch:1;	/* 24 */
    unsigned int sched_entry_seq:1;	/* 25 */
    unsigned int reserved1:1;	/* 26 */
    unsigned int Save_SP:1;	/* 27 */
    unsigned int Save_RP:1;	/* 28 */
    unsigned int Save_MRP_in_frame:1;	/* 29 */
    unsigned int save_r19:1;	/* 30 */
    unsigned int Cleanup_defined:1;	/* 31 */

    unsigned int MPE_XL_interrupt_marker:1;	/* 0 */
    unsigned int HP_UX_interrupt_marker:1;	/* 1 */
    unsigned int Large_frame:1;	/* 2 */
    unsigned int alloca_frame:1;	/* 3 */
    unsigned int reserved2:1;	/* 4 */
    unsigned int Total_frame_size:27;	/* 5..31 */

    /* This is *NOT* part of an actual unwind_descriptor in an object
       file.  It is *ONLY* part of the "internalized" descriptors that
       we create from those in a file.  */

    struct
      {
	unsigned int stub_type:4;	/* 0..3 */
	unsigned int padding:28;	/* 4..31 */
      }
    stub_unwind;
  };

/* HP linkers also generate unwinds for various linker-generated stubs.
   GDB reads in the stubs from the $UNWIND_END$ subspace, then 
   "converts" them into normal unwind entries using some of the reserved
   fields to store the stub type.  */

/* The gaps represent linker stubs used in MPE and space for future
   expansion.  */
enum unwind_stub_types
  {
    LONG_BRANCH = 1,
    PARAMETER_RELOCATION = 2,
    EXPORT = 10,
    IMPORT = 11,
    IMPORT_SHLIB = 12,
  };

struct unwind_table_entry *find_unwind_entry (CORE_ADDR);

int hppa_get_field (unsigned word, int from, int to);
int hppa_extract_5_load (unsigned int);
unsigned hppa_extract_5R_store (unsigned int);
unsigned hppa_extract_5r_store (unsigned int);
int hppa_extract_17 (unsigned int);
int hppa_extract_21 (unsigned);
int hppa_extract_14 (unsigned);
CORE_ADDR hppa_symbol_address(const char *sym);

extern struct value *
  hppa_frame_prev_register_helper (frame_info_ptr this_frame,
				   trad_frame_saved_reg *saved_regs,
				   int regnum);

extern CORE_ADDR hppa_read_pc (struct regcache *regcache);
extern void hppa_write_pc (struct regcache *regcache, CORE_ADDR pc);
extern CORE_ADDR hppa_unwind_pc (struct gdbarch *gdbarch,
				 frame_info_ptr next_frame);

extern int hppa_in_solib_call_trampoline (struct gdbarch *gdbarch,
					  CORE_ADDR pc);
extern CORE_ADDR hppa_skip_trampoline_code (frame_info_ptr, CORE_ADDR pc);

#endif  /* hppa-tdep.h */
