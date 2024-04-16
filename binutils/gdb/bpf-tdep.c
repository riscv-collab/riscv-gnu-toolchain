/* Target-dependent code for BPF.

   Copyright (C) 2020-2024 Free Software Foundation, Inc.

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
#include "dis-asm.h"
#include "frame.h"
#include "frame-unwind.h"
#include "trad-frame.h"
#include "symtab.h"
#include "value.h"
#include "gdbcmd.h"
#include "breakpoint.h"
#include "inferior.h"
#include "regcache.h"
#include "target.h"
#include "dwarf2/frame.h"
#include "osabi.h"
#include "target-descriptions.h"
#include "remote.h"
#include "gdbarch.h"


/* eBPF registers.  */

enum bpf_regnum
{
  BPF_R0_REGNUM,		/* return value */
  BPF_R1_REGNUM,
  BPF_R2_REGNUM,
  BPF_R3_REGNUM,
  BPF_R4_REGNUM,
  BPF_R5_REGNUM,
  BPF_R6_REGNUM,
  BPF_R7_REGNUM,
  BPF_R8_REGNUM,
  BPF_R9_REGNUM,
  BPF_R10_REGNUM,		/* sp */
  BPF_PC_REGNUM,
};

#define BPF_NUM_REGS	(BPF_PC_REGNUM + 1)

/* Target-dependent structure in gdbarch.  */
struct bpf_gdbarch_tdep : gdbarch_tdep_base
{
};


/* Internal debugging facilities.  */

/* When this is set to non-zero debugging information will be
   printed.  */

static unsigned int bpf_debug_flag = 0;

/* The show callback for 'show debug bpf'.  */

static void
show_bpf_debug (struct ui_file *file, int from_tty,
		struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Debugging of BPF is %s.\n"), value);
}


/* BPF registers.  */

static const char *bpf_register_names[] =
{
  "r0",   "r1",  "r2",    "r3",   "r4",   "r5",   "r6",   "r7",
  "r8",   "r9",  "r10",   "pc"
};

/* Return the name of register REGNUM.  */

static const char *
bpf_register_name (struct gdbarch *gdbarch, int reg)
{
  static_assert (ARRAY_SIZE (bpf_register_names) == BPF_NUM_REGS);
  return bpf_register_names[reg];
}

/* Return the GDB type of register REGNUM.  */

static struct type *
bpf_register_type (struct gdbarch *gdbarch, int reg)
{
  if (reg == BPF_R10_REGNUM)
    return builtin_type (gdbarch)->builtin_data_ptr;
  else if (reg == BPF_PC_REGNUM)
    return builtin_type (gdbarch)->builtin_func_ptr;
  return builtin_type (gdbarch)->builtin_int64;
}

/* Return the GDB register number corresponding to DWARF's REG.  */

static int
bpf_dwarf2_reg_to_regnum (struct gdbarch *gdbarch, int reg)
{
  if (reg >= 0 && reg < BPF_NUM_REGS)
    return reg;
  return -1;
}

/* Implement the "print_insn" gdbarch method.  */

static int
bpf_gdb_print_insn (bfd_vma memaddr, disassemble_info *info)
{
  info->symbols = NULL;
  return default_print_insn (memaddr, info);
}


/* Return PC of first real instruction of the function starting at
   START_PC.  */

static CORE_ADDR
bpf_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR start_pc)
{
  gdb_printf (gdb_stdlog,
	      "Skipping prologue: start_pc=%s\n",
	      paddress (gdbarch, start_pc));
  /* XXX: to be completed.  */
  return start_pc + 0;
}


/* Frame unwinder.

   XXX it is not clear how to unwind in eBPF, since the stack is not
   guaranteed to be contiguous, and therefore no relative stack
   addressing can be done in the callee in order to access the
   caller's stack frame.  To explore with xBPF, which will relax this
   restriction.  */

/* Given THIS_FRAME, return its ID.  */

static void
bpf_frame_this_id (frame_info_ptr this_frame,
		   void **this_prologue_cache,
		   struct frame_id *this_id)
{
  /* Note that THIS_ID defaults to the outermost frame if we don't set
     anything here.  See frame.c:compute_frame_id.  */
}

/* Return the reason why we can't unwind past THIS_FRAME.  */

static enum unwind_stop_reason
bpf_frame_unwind_stop_reason (frame_info_ptr this_frame,
			      void **this_cache)
{
  return UNWIND_OUTERMOST;
}

/* Ask THIS_FRAME to unwind its register.  */

static struct value *
bpf_frame_prev_register (frame_info_ptr this_frame,
			 void **this_prologue_cache, int regnum)
{
  return frame_unwind_got_register (this_frame, regnum, regnum);
}

/* Frame unwinder machinery for BPF.  */

static const struct frame_unwind bpf_frame_unwind =
{
  "bpf prologue",
  NORMAL_FRAME,
  bpf_frame_unwind_stop_reason,
  bpf_frame_this_id,
  bpf_frame_prev_register,
  NULL,
  default_frame_sniffer
};


/* Breakpoints.  */

/* Enum describing the different kinds of breakpoints.  We currently
   just support one, implemented by the brkpt xbpf instruction.   */

enum bpf_breakpoint_kinds
{
  BPF_BP_KIND_BRKPT = 0,
};

/* Implement the breakpoint_kind_from_pc gdbarch method.  */

static int
bpf_breakpoint_kind_from_pc (struct gdbarch *gdbarch, CORE_ADDR *start_pc)
{
  /* We support just one kind of breakpoint.  */
  return BPF_BP_KIND_BRKPT;
}

/* Implement the sw_breakpoint_from_kind gdbarch method.  */

static const gdb_byte *
bpf_sw_breakpoint_from_kind (struct gdbarch *gdbarch, int kind, int *size)
{
  static unsigned char brkpt_insn[]
    = {0x8c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  switch (kind)
    {
    case BPF_BP_KIND_BRKPT:
      *size = 8;
      return brkpt_insn;
    default:
      gdb_assert_not_reached ("unexpected BPF breakpoint kind");
    }
}


/* Assuming THIS_FRAME is a dummy frame, return its frame ID.  */

static struct frame_id
bpf_dummy_id (struct gdbarch *gdbarch, frame_info_ptr this_frame)
{
  CORE_ADDR sp = get_frame_register_unsigned (this_frame,
					      gdbarch_sp_regnum (gdbarch));
  return frame_id_build (sp, get_frame_pc (this_frame));
}

/* Implement the push dummy call gdbarch callback.  */

static CORE_ADDR
bpf_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
		     struct regcache *regcache, CORE_ADDR bp_addr,
		     int nargs, struct value **args, CORE_ADDR sp,
		     function_call_return_method return_method,
		     CORE_ADDR struct_addr)
{
  gdb_printf (gdb_stdlog, "Pushing dummy call: sp=%s\n",
	      paddress (gdbarch, sp));
  /* XXX writeme  */
  return sp;
}

/* Extract a function return value of TYPE from REGCACHE,
   and copy it into VALBUF.  */

static void
bpf_extract_return_value (struct type *type, struct regcache *regcache,
			  gdb_byte *valbuf)
{
  int len = type->length ();
  gdb_byte vbuf[8];

  gdb_assert (len <= 8);
  regcache->cooked_read (BPF_R0_REGNUM, vbuf);
  memcpy (valbuf, vbuf + 8 - len, len);
}

/* Store the function return value of type TYPE from VALBUF into REGNAME.  */

static void
bpf_store_return_value (struct type *type, struct regcache *regcache,
			const gdb_byte *valbuf)
{
  int len = type->length ();
  gdb_byte vbuf[8];

  gdb_assert (len <= 8);
  memset (vbuf, 0, sizeof (vbuf));
  memcpy (vbuf + 8 - len, valbuf, len);
  regcache->cooked_write (BPF_R0_REGNUM, vbuf);
}

/* Handle function's return value.  */

static enum return_value_convention
bpf_return_value (struct gdbarch *gdbarch, struct value *function,
		  struct type *type, struct regcache *regcache,
		  gdb_byte *readbuf, const gdb_byte *writebuf)
{
  int len = type->length ();

  if (len > 8)
    return RETURN_VALUE_STRUCT_CONVENTION;

  if (readbuf != NULL)
    bpf_extract_return_value (type, regcache, readbuf);
  if (writebuf != NULL)
    bpf_store_return_value (type, regcache, writebuf);

  return RETURN_VALUE_REGISTER_CONVENTION;
}


/* Initialize the current architecture based on INFO.  If possible, re-use an
   architecture from ARCHES, which is a list of architectures already created
   during this debugging session.  */

static struct gdbarch *
bpf_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  /* If there is already a candidate, use it.  */
  arches = gdbarch_list_lookup_by_info (arches, &info);
  if (arches != NULL)
    return arches->gdbarch;

  /* Allocate space for the new architecture.  */
  gdbarch *gdbarch
    = gdbarch_alloc (&info, gdbarch_tdep_up (new bpf_gdbarch_tdep));

  /* Information about registers, etc.  */
  set_gdbarch_num_regs (gdbarch, BPF_NUM_REGS);
  set_gdbarch_register_name (gdbarch, bpf_register_name);
  set_gdbarch_register_type (gdbarch, bpf_register_type);

  /* Register numbers of various important registers.  */
  set_gdbarch_sp_regnum (gdbarch, BPF_R10_REGNUM);
  set_gdbarch_pc_regnum (gdbarch, BPF_PC_REGNUM);

  /* Map DWARF2 registers to GDB registers.  */
  set_gdbarch_dwarf2_reg_to_regnum (gdbarch, bpf_dwarf2_reg_to_regnum);

  /* Call dummy code.  */
  set_gdbarch_call_dummy_location (gdbarch, ON_STACK);
  set_gdbarch_dummy_id (gdbarch, bpf_dummy_id);
  set_gdbarch_push_dummy_call (gdbarch, bpf_push_dummy_call);

  /* Returning results.  */
  set_gdbarch_return_value (gdbarch, bpf_return_value);

  /* Advance PC across function entry code.  */
  set_gdbarch_skip_prologue (gdbarch, bpf_skip_prologue);

  /* Stack grows downward.  */
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);

  /* Breakpoint manipulation.  */
  set_gdbarch_breakpoint_kind_from_pc (gdbarch, bpf_breakpoint_kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch, bpf_sw_breakpoint_from_kind);

  /* Frame handling.  */
  set_gdbarch_frame_args_skip (gdbarch, 8);

  /* Disassembly.  */
  set_gdbarch_print_insn (gdbarch, bpf_gdb_print_insn);

  /* Hook in ABI-specific overrides, if they have been registered.  */
  gdbarch_init_osabi (info, gdbarch);

  /* Install unwinders.  */
  frame_unwind_append_unwinder (gdbarch, &bpf_frame_unwind);

  return gdbarch;
}

void _initialize_bpf_tdep ();
void
_initialize_bpf_tdep ()
{
  gdbarch_register (bfd_arch_bpf, bpf_gdbarch_init);

  /* Add commands 'set/show debug bpf'.  */
  add_setshow_zuinteger_cmd ("bpf", class_maintenance,
			     &bpf_debug_flag,
			     _("Set BPF debugging."),
			     _("Show BPF debugging."),
			     _("Enables BPF specific debugging output."),
			     NULL,
			     &show_bpf_debug,
			     &setdebuglist, &showdebuglist);
}
