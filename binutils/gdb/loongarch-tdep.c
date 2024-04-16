/* Target-dependent code for the LoongArch architecture, for GDB.

   Copyright (C) 2022-2024 Free Software Foundation, Inc.

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
#include "elf-bfd.h"
#include "frame-unwind.h"
#include "gdbcore.h"
#include "loongarch-tdep.h"
#include "reggroups.h"
#include "target.h"
#include "target-descriptions.h"
#include "trad-frame.h"
#include "user-regs.h"

/* Fetch the instruction at PC.  */

static insn_t
loongarch_fetch_instruction (CORE_ADDR pc)
{
  size_t insn_len = loongarch_insn_length (0);
  gdb_byte buf[insn_len];
  int err;

  err = target_read_memory (pc, buf, insn_len);
  if (err)
    memory_error (TARGET_XFER_E_IO, pc);

  return extract_unsigned_integer (buf, insn_len, BFD_ENDIAN_LITTLE);
}

/* Return TRUE if INSN is a unconditional branch instruction, otherwise return FALSE.  */

static bool
loongarch_insn_is_uncond_branch (insn_t insn)
{
  if ((insn & 0xfc000000) == 0x4c000000		/* jirl  */
      || (insn & 0xfc000000) == 0x50000000	/* b     */
      || (insn & 0xfc000000) == 0x54000000)	/* bl    */
    return true;
  return false;
}

/* Return TRUE if INSN is a conditional branch instruction, otherwise return FALSE.  */

static bool
loongarch_insn_is_cond_branch (insn_t insn)
{
  if ((insn & 0xfc000000) == 0x58000000		/* beq   */
      || (insn & 0xfc000000) == 0x5c000000	/* bne   */
      || (insn & 0xfc000000) == 0x60000000	/* blt   */
      || (insn & 0xfc000000) == 0x64000000	/* bge	 */
      || (insn & 0xfc000000) == 0x68000000	/* bltu  */
      || (insn & 0xfc000000) == 0x6c000000	/* bgeu  */
      || (insn & 0xfc000000) == 0x40000000	/* beqz  */
      || (insn & 0xfc000000) == 0x44000000)	/* bnez  */
    return true;
  return false;
}

/* Return TRUE if INSN is a branch instruction, otherwise return FALSE.  */

static bool
loongarch_insn_is_branch (insn_t insn)
{
  bool is_uncond = loongarch_insn_is_uncond_branch (insn);
  bool is_cond = loongarch_insn_is_cond_branch (insn);

  return (is_uncond || is_cond);
}

/* Return TRUE if INSN is a Load Linked instruction, otherwise return FALSE.  */

static bool
loongarch_insn_is_ll (insn_t insn)
{
  if ((insn & 0xff000000) == 0x20000000		/* ll.w  */
      || (insn & 0xff000000) == 0x22000000)	/* ll.d  */
    return true;
  return false;
}

/* Return TRUE if INSN is a Store Conditional instruction, otherwise return FALSE.  */

static bool
loongarch_insn_is_sc (insn_t insn)
{
  if ((insn & 0xff000000) == 0x21000000		/* sc.w  */
      || (insn & 0xff000000) == 0x23000000)	/* sc.d  */
    return true;
  return false;
}

/* Analyze the function prologue from START_PC to LIMIT_PC.
   Return the address of the first instruction past the prologue.  */

static CORE_ADDR
loongarch_scan_prologue (struct gdbarch *gdbarch, CORE_ADDR start_pc,
			 CORE_ADDR limit_pc, frame_info_ptr this_frame,
			 struct trad_frame_cache *this_cache)
{
  CORE_ADDR cur_pc = start_pc, prologue_end = 0;
  int32_t sp = LOONGARCH_SP_REGNUM;
  int32_t fp = LOONGARCH_FP_REGNUM;
  int32_t reg_value[32] = {0};
  int32_t reg_used[32] = {1, 0};

  while (cur_pc < limit_pc)
    {
      insn_t insn = loongarch_fetch_instruction (cur_pc);
      size_t insn_len = loongarch_insn_length (insn);
      int32_t rd = loongarch_decode_imm ("0:5", insn, 0);
      int32_t rj = loongarch_decode_imm ("5:5", insn, 0);
      int32_t rk = loongarch_decode_imm ("10:5", insn, 0);
      int32_t si12 = loongarch_decode_imm ("10:12", insn, 1);
      int32_t si20 = loongarch_decode_imm ("5:20", insn, 1);

      if ((insn & 0xffc00000) == 0x02c00000	/* addi.d sp,sp,si12  */
	  && rd == sp && rj == sp && si12 < 0)
	{
	  prologue_end = cur_pc + insn_len;
	}
      else if ((insn & 0xffc00000) == 0x02c00000 /* addi.d fp,sp,si12  */
	       && rd == fp && rj == sp && si12 > 0)
	{
	  prologue_end = cur_pc + insn_len;
	}
      else if ((insn & 0xffc00000) == 0x29c00000 /* st.d rd,sp,si12  */
	       && rj == sp)
	{
	  prologue_end = cur_pc + insn_len;
	}
      else if ((insn & 0xff000000) == 0x27000000 /* stptr.d rd,sp,si14  */
	       && rj == sp)
	{
	  prologue_end = cur_pc + insn_len;
	}
      else if ((insn & 0xfe000000) == 0x14000000) /* lu12i.w rd,si20  */
	{
	  reg_value[rd] = si20 << 12;
	  reg_used[rd] = 1;
	}
      else if ((insn & 0xffc00000) == 0x03800000) /* ori rd,rj,si12  */
	{
	  if (reg_used[rj])
	    {
	      reg_value[rd] = reg_value[rj] | (si12 & 0xfff);
	      reg_used[rd] = 1;
	    }
	}
      else if ((insn & 0xffff8000) == 0x00108000 /* add.d sp,sp,rk  */
	       && rd == sp && rj == sp)
	{
	  if (reg_used[rk] == 1 && reg_value[rk] < 0)
	    {
	      prologue_end = cur_pc + insn_len;
	      break;
	    }
	}
      else if (loongarch_insn_is_branch (insn))
	{
	  break;
	}

      cur_pc += insn_len;
    }

  if (prologue_end == 0)
    prologue_end = cur_pc;

  return prologue_end;
}

/* Implement the loongarch_skip_prologue gdbarch method.  */

static CORE_ADDR
loongarch_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  CORE_ADDR func_addr;

  /* See if we can determine the end of the prologue via the symbol table.
     If so, then return either PC, or the PC after the prologue, whichever
     is greater.  */
  if (find_pc_partial_function (pc, nullptr, &func_addr, nullptr))
    {
      CORE_ADDR post_prologue_pc
	= skip_prologue_using_sal (gdbarch, func_addr);
      if (post_prologue_pc != 0)
	return std::max (pc, post_prologue_pc);
    }

  /* Can't determine prologue from the symbol table, need to examine
     instructions.  */

  /* Find an upper limit on the function prologue using the debug
     information.  If the debug information could not be used to provide
     that bound, then use an arbitrary large number as the upper bound.  */
  CORE_ADDR limit_pc = skip_prologue_using_sal (gdbarch, pc);
  if (limit_pc == 0)
    limit_pc = pc + 100;	/* Arbitrary large number.  */

  return loongarch_scan_prologue (gdbarch, pc, limit_pc, nullptr, nullptr);
}

/* Decode the current instruction and determine the address of the
   next instruction.  */

static CORE_ADDR
loongarch_next_pc (struct regcache *regcache, CORE_ADDR cur_pc)
{
  struct gdbarch *gdbarch = regcache->arch ();
  loongarch_gdbarch_tdep *tdep = gdbarch_tdep<loongarch_gdbarch_tdep> (gdbarch);
  insn_t insn = loongarch_fetch_instruction (cur_pc);
  size_t insn_len = loongarch_insn_length (insn);
  CORE_ADDR next_pc = cur_pc + insn_len;

  if ((insn & 0xfc000000) == 0x4c000000)		/* jirl rd, rj, offs16  */
    {
      LONGEST rj = regcache_raw_get_signed (regcache,
		     loongarch_decode_imm ("5:5", insn, 0));
      next_pc = rj + loongarch_decode_imm ("10:16<<2", insn, 1);
    }
  else if ((insn & 0xfc000000) == 0x50000000		/* b    offs26  */
	   || (insn & 0xfc000000) == 0x54000000)	/* bl	offs26  */
    {
      next_pc = cur_pc + loongarch_decode_imm ("0:10|10:16<<2", insn, 1);
    }
  else if ((insn & 0xfc000000) == 0x58000000)		/* beq	rj, rd, offs16  */
    {
      LONGEST rj = regcache_raw_get_signed (regcache,
		     loongarch_decode_imm ("5:5", insn, 0));
      LONGEST rd = regcache_raw_get_signed (regcache,
		     loongarch_decode_imm ("0:5", insn, 0));
      if (rj == rd)
	next_pc = cur_pc + loongarch_decode_imm ("10:16<<2", insn, 1);
    }
  else if ((insn & 0xfc000000) == 0x5c000000)		/* bne	rj, rd, offs16  */
    {
      LONGEST rj = regcache_raw_get_signed (regcache,
		     loongarch_decode_imm ("5:5", insn, 0));
      LONGEST rd = regcache_raw_get_signed (regcache,
		     loongarch_decode_imm ("0:5", insn, 0));
      if (rj != rd)
	next_pc = cur_pc + loongarch_decode_imm ("10:16<<2", insn, 1);
    }
  else if ((insn & 0xfc000000) == 0x60000000)		/* blt	rj, rd, offs16  */
    {
      LONGEST rj = regcache_raw_get_signed (regcache,
		     loongarch_decode_imm ("5:5", insn, 0));
      LONGEST rd = regcache_raw_get_signed (regcache,
		     loongarch_decode_imm ("0:5", insn, 0));
      if (rj < rd)
	next_pc = cur_pc + loongarch_decode_imm ("10:16<<2", insn, 1);
    }
  else if ((insn & 0xfc000000) == 0x64000000)		/* bge	rj, rd, offs16  */
    {
      LONGEST rj = regcache_raw_get_signed (regcache,
		     loongarch_decode_imm ("5:5", insn, 0));
      LONGEST rd = regcache_raw_get_signed (regcache,
		     loongarch_decode_imm ("0:5", insn, 0));
      if (rj >= rd)
	next_pc = cur_pc + loongarch_decode_imm ("10:16<<2", insn, 1);
    }
  else if ((insn & 0xfc000000) == 0x68000000)		/* bltu	rj, rd, offs16  */
    {
      ULONGEST rj = regcache_raw_get_unsigned (regcache,
		      loongarch_decode_imm ("5:5", insn, 0));
      ULONGEST rd = regcache_raw_get_unsigned (regcache,
		      loongarch_decode_imm ("0:5", insn, 0));
      if (rj < rd)
	next_pc = cur_pc + loongarch_decode_imm ("10:16<<2", insn, 1);
    }
  else if ((insn & 0xfc000000) == 0x6c000000)		/* bgeu	rj, rd, offs16  */
    {
      ULONGEST rj = regcache_raw_get_unsigned (regcache,
		      loongarch_decode_imm ("5:5", insn, 0));
      ULONGEST rd = regcache_raw_get_unsigned (regcache,
		      loongarch_decode_imm ("0:5", insn, 0));
      if (rj >= rd)
	next_pc = cur_pc + loongarch_decode_imm ("10:16<<2", insn, 1);
    }
  else if ((insn & 0xfc000000) == 0x40000000)		/* beqz	rj, offs21  */
    {
      LONGEST rj = regcache_raw_get_signed (regcache,
		     loongarch_decode_imm ("5:5", insn, 0));
      if (rj == 0)
	next_pc = cur_pc + loongarch_decode_imm ("0:5|10:16<<2", insn, 1);
    }
  else if ((insn & 0xfc000000) == 0x44000000)		/* bnez	rj, offs21  */
    {
      LONGEST rj = regcache_raw_get_signed (regcache,
		     loongarch_decode_imm ("5:5", insn, 0));
      if (rj != 0)
	next_pc = cur_pc + loongarch_decode_imm ("0:5|10:16<<2", insn, 1);
    }
  else if ((insn & 0xffff8000) == 0x002b0000)		/* syscall  */
    {
      if (tdep->syscall_next_pc != nullptr)
	next_pc = tdep->syscall_next_pc (get_current_frame ());
    }

  return next_pc;
}

/* We can't put a breakpoint in the middle of a ll/sc atomic sequence,
   so look for the end of the sequence and put the breakpoint there.  */

static std::vector<CORE_ADDR>
loongarch_deal_with_atomic_sequence (struct regcache *regcache, CORE_ADDR cur_pc)
{
  CORE_ADDR next_pc;
  std::vector<CORE_ADDR> next_pcs;
  insn_t insn = loongarch_fetch_instruction (cur_pc);
  size_t insn_len = loongarch_insn_length (insn);
  const int atomic_sequence_length = 16;
  bool found_atomic_sequence_endpoint = false;

  /* Look for a Load Linked instruction which begins the atomic sequence.  */
  if (!loongarch_insn_is_ll (insn))
    return {};

  /* Assume that no atomic sequence is longer than "atomic_sequence_length" instructions.  */
  for (int insn_count = 0; insn_count < atomic_sequence_length; ++insn_count)
    {
      cur_pc += insn_len;
      insn = loongarch_fetch_instruction (cur_pc);

      /* Look for a unconditional branch instruction, fallback to the standard code.  */
      if (loongarch_insn_is_uncond_branch (insn))
	{
	  return {};
	}
      /* Look for a conditional branch instruction, put a breakpoint in its destination address.  */
      else if (loongarch_insn_is_cond_branch (insn))
	{
	  next_pc = loongarch_next_pc (regcache, cur_pc);
	  next_pcs.push_back (next_pc);
	}
      /* Look for a Store Conditional instruction which closes the atomic sequence.  */
      else if (loongarch_insn_is_sc (insn))
	{
	  found_atomic_sequence_endpoint = true;
	  next_pc = cur_pc + insn_len;
	  next_pcs.push_back (next_pc);
	  break;
	}
    }

  /* We didn't find a closing Store Conditional instruction, fallback to the standard code.  */
  if (!found_atomic_sequence_endpoint)
    return {};

  return next_pcs;
}

/* Implement the software_single_step gdbarch method  */

static std::vector<CORE_ADDR>
loongarch_software_single_step (struct regcache *regcache)
{
  CORE_ADDR cur_pc = regcache_read_pc (regcache);
  std::vector<CORE_ADDR> next_pcs
    = loongarch_deal_with_atomic_sequence (regcache, cur_pc);

  if (!next_pcs.empty ())
    return next_pcs;

  CORE_ADDR next_pc = loongarch_next_pc (regcache, cur_pc);

  return {next_pc};
}

/* Callback function for user_reg_add.  */

static struct value *
value_of_loongarch_user_reg (frame_info_ptr frame, const void *baton)
{
  return value_of_register ((long long) baton,
			    get_next_frame_sentinel_okay (frame));
}

/* Implement the frame_align gdbarch method.  */

static CORE_ADDR
loongarch_frame_align (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  return align_down (addr, 16);
}

/* Generate, or return the cached frame cache for frame unwinder.  */

static struct trad_frame_cache *
loongarch_frame_cache (frame_info_ptr this_frame, void **this_cache)
{
  struct trad_frame_cache *cache;
  CORE_ADDR pc;

  if (*this_cache != nullptr)
    return (struct trad_frame_cache *) *this_cache;

  cache = trad_frame_cache_zalloc (this_frame);
  *this_cache = cache;

  trad_frame_set_reg_realreg (cache, LOONGARCH_PC_REGNUM, LOONGARCH_RA_REGNUM);

  pc = get_frame_address_in_block (this_frame);
  trad_frame_set_id (cache, frame_id_build_unavailable_stack (pc));

  return cache;
}

/* Implement the this_id callback for frame unwinder.  */

static void
loongarch_frame_this_id (frame_info_ptr this_frame, void **prologue_cache,
			 struct frame_id *this_id)
{
  struct trad_frame_cache *info;

  info = loongarch_frame_cache (this_frame, prologue_cache);
  trad_frame_get_id (info, this_id);
}

/* Implement the prev_register callback for frame unwinder.  */

static struct value *
loongarch_frame_prev_register (frame_info_ptr this_frame,
			       void **prologue_cache, int regnum)
{
  struct trad_frame_cache *info;

  info = loongarch_frame_cache (this_frame, prologue_cache);
  return trad_frame_get_register (info, this_frame, regnum);
}

static const struct frame_unwind loongarch_frame_unwind = {
  "loongarch prologue",
  /*.type	   =*/NORMAL_FRAME,
  /*.stop_reason   =*/default_frame_unwind_stop_reason,
  /*.this_id	   =*/loongarch_frame_this_id,
  /*.prev_register =*/loongarch_frame_prev_register,
  /*.unwind_data   =*/nullptr,
  /*.sniffer	   =*/default_frame_sniffer,
  /*.dealloc_cache =*/nullptr,
  /*.prev_arch	   =*/nullptr,
};

/* Write the contents of buffer VAL into the general-purpose argument
   register defined by GAR in REGCACHE.  GAR indicates the available
   general-purpose argument registers which should be a value in the
   range 1 to 8 (LOONGARCH_ARG_REGNUM), which correspond to registers
   a7 and a0 respectively, that is to say, regnum is a7 if GAR is 1,
   regnum is a6 if GAR is 2, regnum is a5 if GAR is 3, regnum is a4
   if GAR is 4, regnum is a3 if GAR is 5, regnum is a2 if GAR is 6,
   regnum is a1 if GAR is 7, regnum is a0 if GAR is 8.  */

static void
pass_in_gar (struct regcache *regcache, unsigned int gar, const gdb_byte *val)
{
  unsigned int regnum = LOONGARCH_ARG_REGNUM - gar + LOONGARCH_A0_REGNUM;
  regcache->cooked_write (regnum, val);
}

/* Write the contents of buffer VAL into the floating-point argument
   register defined by FAR in REGCACHE.  FAR indicates the available
   floating-point argument registers which should be a value in the
   range 1 to 8 (LOONGARCH_ARG_REGNUM), which correspond to registers
   f7 and f0 respectively, that is to say, regnum is f7 if FAR is 1,
   regnum is f6 if FAR is 2, regnum is f5 if FAR is 3, regnum is f4
   if FAR is 4, regnum is f3 if FAR is 5, regnum is f2 if FAR is 6,
   regnum is f1 if FAR is 7, regnum is f0 if FAR is 8.  */

static void
pass_in_far (struct regcache *regcache, unsigned int far, const gdb_byte *val)
{
  unsigned int regnum = LOONGARCH_ARG_REGNUM - far + LOONGARCH_FIRST_FP_REGNUM;
  regcache->cooked_write (regnum, val);
}

/* Pass a value on the stack.  */

static void
pass_on_stack (struct regcache *regcache, const gdb_byte *val,
	       size_t len, int align, gdb_byte **addr)
{
  align = align_up (align, 8);
  if (align > 16)
    align = 16;

  CORE_ADDR align_addr = (CORE_ADDR) (*addr);
  align_addr = align_up (align_addr, align);
  *addr = (gdb_byte *) align_addr;
  memcpy (*addr, val, len);
  *addr += len;
}

/* Compute the numbers of struct member.  */

static void
compute_struct_member (struct type *type,
		       unsigned int *fixed_point_members,
		       unsigned int *floating_point_members,
		       bool *first_member_is_fixed_point,
		       bool *has_long_double)
{
  for (int i = 0; i < type->num_fields (); i++)
    {
      /* Ignore any static fields.  */
      if (type->field (i).is_static ())
	continue;

      struct type *field_type = check_typedef (type->field (i).type ());

      if ((field_type->code () == TYPE_CODE_FLT
	   && field_type->length () == 16)
	  || (field_type->code () == TYPE_CODE_COMPLEX
	      && field_type->length () == 32))
	*has_long_double = true;

      if (field_type->code () == TYPE_CODE_INT
	  || field_type->code () == TYPE_CODE_BOOL
	  || field_type->code () == TYPE_CODE_CHAR
	  || field_type->code () == TYPE_CODE_RANGE
	  || field_type->code () == TYPE_CODE_ENUM
	  || field_type->code () == TYPE_CODE_PTR)
      {
	(*fixed_point_members)++;

	if (*floating_point_members == 0)
	  *first_member_is_fixed_point = true;
      }
      else if (field_type->code () == TYPE_CODE_FLT)
	(*floating_point_members)++;
      else if (field_type->code () == TYPE_CODE_STRUCT)
	compute_struct_member (field_type,
			       fixed_point_members,
			       floating_point_members,
			       first_member_is_fixed_point,
			       has_long_double);
      else if (field_type->code () == TYPE_CODE_COMPLEX)
	(*floating_point_members) += 2;
    }
}

/* Compute the lengths and offsets of struct member.  */

static void
struct_member_info (struct type *type,
		    unsigned int *member_offsets,
		    unsigned int *member_lens,
		    unsigned int offset,
		    unsigned int *fields)
{
  unsigned int count = type->num_fields ();
  unsigned int i;

  for (i = 0; i < count; ++i)
    {
      if (type->field (i).loc_kind () != FIELD_LOC_KIND_BITPOS)
	continue;

      struct type *field_type = check_typedef (type->field (i).type ());
      int field_offset
	= offset + type->field (i).loc_bitpos () / TARGET_CHAR_BIT;

      switch (field_type->code ())
	{
	case TYPE_CODE_STRUCT:
	  struct_member_info (field_type, member_offsets, member_lens,
			      field_offset, fields);
	  break;

	case TYPE_CODE_COMPLEX:
	  if (*fields == 0)
	    {
	      /* _Complex float */
	      if (field_type->length () == 8)
		{
		  member_offsets[0] = field_offset;
		  member_offsets[1] = field_offset + 4;
		  member_lens[0] = member_lens[1] = 4;
		  *fields = 2;
		}
	      /* _Complex double */
	      else if (field_type->length () == 16)
		{
		  member_offsets[0] = field_offset;
		  member_offsets[1] = field_offset + 8;
		  member_lens[0] = member_lens[1] = 8;
		  *fields = 2;
		}
	    }
	  break;

	default:
	  if (*fields < 2)
	    {
	      member_offsets[*fields] = field_offset;
	      member_lens[*fields] = field_type->length ();
	    }
	  (*fields)++;
	  break;
	}

      /* only has special handling for structures with 1 or 2 fields. */
      if (*fields > 2)
	return;
    }
}

/* Implement the push_dummy_call gdbarch method.  */

static CORE_ADDR
loongarch_push_dummy_call (struct gdbarch *gdbarch,
			   struct value *function,
			   struct regcache *regcache,
			   CORE_ADDR bp_addr,
			   int nargs,
			   struct value **args,
			   CORE_ADDR sp,
			   function_call_return_method return_method,
			   CORE_ADDR struct_addr)
{
  int regsize = register_size (gdbarch, 0);
  unsigned int gar = LOONGARCH_ARG_REGNUM;
  unsigned int far = LOONGARCH_ARG_REGNUM;
  unsigned int fixed_point_members;
  unsigned int floating_point_members;
  bool first_member_is_fixed_point;
  bool has_long_double;
  unsigned int member_offsets[2];
  unsigned int member_lens[2];
  unsigned int fields;
  gdb_byte buf[1024] = { 0 };
  gdb_byte *addr = buf;

  if (return_method != return_method_normal)
    pass_in_gar (regcache, gar--, (gdb_byte *) &struct_addr);

  for (int i = 0; i < nargs; i++)
    {
      struct value *arg = args[i];
      const gdb_byte *val = arg->contents ().data ();
      struct type *type = check_typedef (arg->type ());
      size_t len = type->length ();
      int align = type_align (type);
      enum type_code code = type->code ();
      struct type *func_type = check_typedef (function->type ());
      bool varargs = (func_type->has_varargs () && i >= func_type->num_fields ());

      switch (code)
	{
	case TYPE_CODE_INT:
	case TYPE_CODE_BOOL:
	case TYPE_CODE_CHAR:
	case TYPE_CODE_RANGE:
	case TYPE_CODE_ENUM:
	case TYPE_CODE_PTR:
	  {
	    /* integer or pointer type is passed in GAR.
	       If no GAR is available, it's passed on the stack.
	       When passed in registers or on the stack,
	       the unsigned integer scalars are zero-extended to GRLEN bits,
	       and the signed integer scalars are sign-extended.  */
	  if (type->is_unsigned ())
	    {
	      ULONGEST data = extract_unsigned_integer (val, len, BFD_ENDIAN_LITTLE);
	      if (gar > 0)
		pass_in_gar (regcache, gar--, (gdb_byte *) &data);
	      else
		pass_on_stack (regcache, (gdb_byte *) &data, len, align, &addr);
	    }
	  else
	    {
	      LONGEST data = extract_signed_integer (val, len, BFD_ENDIAN_LITTLE);
	      if (gar > 0)
		pass_in_gar (regcache, gar--, (gdb_byte *) &data);
	      else
		pass_on_stack (regcache, (gdb_byte *) &data, len, align, &addr);
	    }
	  }
	  break;
	case TYPE_CODE_FLT:
	  if (len == 2 * regsize)
	    {
	      if (!varargs)
		{
		  /* long double type is passed in a pair of GAR,
		     with the low-order GRLEN bits in the lower-numbered register
		     and the high-order GRLEN bits in the higher-numbered register.
		     If exactly one register is available,
		     the low-order GRLEN bits are passed in the register
		     and the high-order GRLEN bits are passed on the stack.
		     If no GAR is available, it's passed on the stack.  */
		  if (gar >= 2)
		    {
		      pass_in_gar (regcache, gar--, val);
		      pass_in_gar (regcache, gar--, val + regsize);
		    }
		  else if (gar == 1)
		    {
		      pass_in_gar (regcache, gar--, val);
		      pass_on_stack (regcache, val + regsize, len - regsize, align, &addr);
		    }
		  else
		    {
		      pass_on_stack (regcache, val, len, align, &addr);
		    }
		}
	      else
		{
		  /* Variadic arguments are passed in GARs
		     in the same manner as named arguments.
		     And after a variadic argument has been passed on the stack,
		     all future arguments will also be passed on the stack,
		     i.e., the last argument register may be left unused
		     due to the aligned register pair rule.
		     long double data type is passed in an aligned GAR pair,
		     the first register in the pair is even-numbered.  */
		  if (gar >= 2)
		    {
		      if (gar % 2 == 0)
			{
			  pass_in_gar (regcache, gar--, val);
			  pass_in_gar (regcache, gar--, val + regsize);
			}
		      else
			{
			  gar--;
			  pass_in_gar (regcache, gar--, val);
			  pass_in_gar (regcache, gar--, val + regsize);
			}
		    }
		  else if (gar == 1)
		    {
		      gar--;
		      pass_on_stack (regcache, val, len, align, &addr);
		    }
		  else
		    {
		      pass_on_stack (regcache, val, len, align, &addr);
		    }
		}
	    }
	  else
	    {
	      /* The other floating-point type is passed in FAR.
		 If no FAR is available, it's passed in GAR.
		 If no GAR is available, it's passed on the stack.  */
	      if (!varargs && far > 0)
		  pass_in_far (regcache, far--, val);
	      else if (gar > 0)
		  pass_in_gar (regcache, gar--, val);
	      else
		  pass_on_stack (regcache, val, len, align, &addr);
	    }
	  break;
	case TYPE_CODE_STRUCT:
	  {
	    fixed_point_members = 0;
	    floating_point_members = 0;
	    first_member_is_fixed_point = false;
	    has_long_double = false;
	    member_offsets[0] = member_offsets[1] = 0;
	    member_lens[0] = member_offsets[1] = 0;
	    fields = 0;
	    compute_struct_member (type,
				   &fixed_point_members,
				   &floating_point_members,
				   &first_member_is_fixed_point,
				   &has_long_double);
	    struct_member_info (type, member_offsets, member_lens, 0, &fields);
	    /* If the structure consists of one floating-point member within
	       FRLEN bits wide, it is passed in an FAR if available. If the
	       structure consists of two floating-point members both within
	       FRLEN bits wide, it is passed in two FARs if available. If the
	       structure consists of one integer member within GRLEN bits wide
	       and one floating-point member within FRLEN bits wide, it is
	       passed in a GAR and an FAR if available. */
	    if (has_long_double == false
		&& ((fixed_point_members == 0 && floating_point_members == 1
		     && far >= 1)
		    || (fixed_point_members == 0 && floating_point_members == 2
			&& far >= 2)
		    || (fixed_point_members == 1 && floating_point_members == 1
			&& far >= 1 && gar >= 1)))
	      {
		if (fixed_point_members == 0 && floating_point_members == 1)
		  {
		    pass_in_far (regcache, far--, val + member_offsets[0]);
		  }
		else if (fixed_point_members == 0 && floating_point_members == 2)
		  {
		    pass_in_far (regcache, far--, val + member_offsets[0]);
		    pass_in_far (regcache, far--, val + member_offsets[1]);
		  }
		else if (fixed_point_members == 1 && floating_point_members == 1)
		  {
		    if (first_member_is_fixed_point == false)
		      {
			pass_in_far (regcache, far--, val + member_offsets[0]);
			pass_in_gar (regcache, gar--, val + member_offsets[1]);
		      }
		    else
		      {
			pass_in_gar (regcache, gar--, val + member_offsets[0]);
			pass_in_far (regcache, far--, val + member_offsets[1]);
		      }
		  }
	      }
	    else if (len > 0 && len <= regsize)
	      {
		/* The structure has only fixed-point members.  */
		if (fixed_point_members > 0 && floating_point_members == 0)
		  {
		    /* If there is an available GAR,
		       the structure is passed through the GAR by value passing;
		       If no GAR is available, it's passed on the stack.  */
		    if (gar > 0)
		      pass_in_gar (regcache, gar--, val);
		    else
		      pass_on_stack (regcache, val, len, align, &addr);
		  }
		/* The structure has only floating-point members.  */
		else if (fixed_point_members == 0 && floating_point_members > 0)
		  {
		    /* The structure has one floating-point member.
		       The argument is passed in a FAR.
		       If no FAR is available, the value is passed in a GAR.
		       if no GAR is available, the value is passed on the stack.  */
		    if (floating_point_members == 1)
		      {
			if (!varargs && far > 0)
			  pass_in_far (regcache, far--, val);
			else if (gar > 0)
			  pass_in_gar (regcache, gar--, val);
			else
			  pass_on_stack (regcache, val, len, align, &addr);
		      }
		    /* The structure has two floating-point members.
		       The argument is passed in a pair of available FAR,
		       with the low-order float member bits in the lower-numbered FAR
		       and the high-order float member bits in the higher-numbered FAR.
		       If the number of available FAR is less than 2, it's passed in a GAR,
		       and passed on the stack if no GAR is available.  */
		    else if (floating_point_members == 2)
		      {
			if (!varargs && far >= 2)
			  {
			    pass_in_far (regcache, far--, val);
			    pass_in_far (regcache, far--, val + align);
			  }
			else if (gar > 0)
			  {
			    pass_in_gar (regcache, gar--, val);
			  }
			else
			  {
			    pass_on_stack (regcache, val, len, align, &addr);
			  }
		      }
		  }
		/* The structure has both fixed-point and floating-point members.  */
		else if (fixed_point_members > 0 && floating_point_members > 0)
		  {
		    /* The structure has one float member and multiple fixed-point members.
		       If there are available GAR, the structure is passed in a GAR,
		       and passed on the stack if no GAR is available.  */
		    if (floating_point_members == 1 && fixed_point_members > 1)
		      {
			if (gar > 0)
			  pass_in_gar (regcache, gar--, val);
			else
			  pass_on_stack (regcache, val, len, align, &addr);
		      }
		    /* The structure has one float member and one fixed-point member.
		       If one FAR and one GAR are available,
		       the floating-point member of the structure is passed in the FAR,
		       and the fixed-point member of the structure is passed in the GAR.
		       If no floating-point register but one GAR is available, it's passed in GAR;
		       If no GAR is available, it's passed on the stack.  */
		    else if (floating_point_members == 1 && fixed_point_members == 1)
		      {
			if (!varargs && far > 0 && gar > 0)
			  {
			    if (first_member_is_fixed_point == false)
			      {
				pass_in_far (regcache, far--, val);
				pass_in_gar (regcache, gar--, val + align);
			      }
			    else
			      {
				pass_in_gar (regcache, gar--, val);
				pass_in_far (regcache, far--, val + align);
			      }
			  }
			else
			  {
			    if (gar > 0)
			      pass_in_gar (regcache, gar--, val);
			    else
			      pass_on_stack (regcache, val, len, align, &addr);
			  }
		      }
		  }
	      }
	    else if (len > regsize && len <= 2 * regsize)
	      {
		/* The structure has only fixed-point members.  */
		if (fixed_point_members > 0 && floating_point_members == 0)
		  {
		    /* The argument is passed in a pair of available GAR,
		       with the low-order bits in the lower-numbered GAR
		       and the high-order bits in the higher-numbered GAR.
		       If only one GAR is available,
		       the low-order bits are in the GAR
		       and the high-order bits are on the stack,
		       and passed on the stack if no GAR is available.  */
		    if (gar >= 2)
		      {
			pass_in_gar (regcache, gar--, val);
			pass_in_gar (regcache, gar--, val + regsize);
		      }
		    else if (gar == 1)
		      {
			pass_in_gar (regcache, gar--, val);
			pass_on_stack (regcache, val + regsize, len - regsize, align, &addr);
		      }
		    else
		      {
			pass_on_stack (regcache, val, len, align, &addr);
		      }
		  }
		/* The structure has only floating-point members.  */
		else if (fixed_point_members == 0 && floating_point_members > 0)
		  {
		    /* The structure has one long double member
		       or one double member and two adjacent float members
		       or 3-4 float members.
		       The argument is passed in a pair of available GAR,
		       with the low-order bits in the lower-numbered GAR
		       and the high-order bits in the higher-numbered GAR.
		       If only one GAR is available,
		       the low-order bits are in the GAR
		       and the high-order bits are on the stack,
		       and passed on the stack if no GAR is available.  */
		    if ((len == 16 && floating_point_members == 1)
			 || (len == 16 && floating_point_members == 3)
			 || (len == 12 && floating_point_members == 3)
			 || (len == 16 && floating_point_members == 4))
		      {
			if (gar >= 2)
			  {
			    pass_in_gar (regcache, gar--, val);
			    pass_in_gar (regcache, gar--, val + regsize);
			  }
			else if (gar == 1)
			  {
			    if (!varargs)
			      {
				pass_in_gar (regcache, gar--, val);
				pass_on_stack (regcache, val + regsize, len - regsize, align, &addr);
			      }
			    else
			      {
				gar--;
				pass_on_stack (regcache, val, len, align, &addr);
			      }
			  }
			else
			  {
			    pass_on_stack (regcache, val, len, align, &addr);
			  }
		      }
		    /* The structure has two double members
		       or one double member and one float member.
		       The argument is passed in a pair of available FAR,
		       with the low-order bits in the lower-numbered FAR
		       and the high-order bits in the higher-numbered FAR.
		       If no a pair of available FAR,
		       it's passed in a pair of available GAR,
		       with the low-order bits in the lower-numbered GAR
		       and the high-order bits in the higher-numbered GAR.
		       If only one GAR is available,
		       the low-order bits are in the GAR
		       and the high-order bits are on stack,
		       and passed on the stack if no GAR is available.  */
		    else if ((len == 16 && floating_point_members == 2)
			     || (len == 12 && floating_point_members == 2))
		      {
			if (!varargs && far >= 2)
			  {
			    pass_in_far (regcache, far--, val);
			    pass_in_far (regcache, far--, val + regsize);
			  }
			else if (gar >= 2)
			  {
			    pass_in_gar (regcache, gar--, val);
			    pass_in_gar (regcache, gar--, val + regsize);
			  }
			else if (gar == 1)
			  {
			    pass_in_gar (regcache, gar--, val);
			    pass_on_stack (regcache, val + regsize, len - regsize, align, &addr);
			  }
			else
			  {
			    pass_on_stack (regcache, val, len, align, &addr);
			  }
		      }
		  }
		/* The structure has both fixed-point and floating-point members.  */
		else if (fixed_point_members > 0 && floating_point_members > 0)
		  {
		    /* The structure has one floating-point member and one fixed-point member.  */
		    if (floating_point_members == 1 && fixed_point_members == 1)
		      {
			/* If one FAR and one GAR are available,
			   the floating-point member of the structure is passed in the FAR,
			   and the fixed-point member of the structure is passed in the GAR;
			   If no floating-point registers but two GARs are available,
			   it's passed in the two GARs;
			   If only one GAR is available,
			   the low-order bits are in the GAR
			   and the high-order bits are on the stack;
			   And it's passed on the stack if no GAR is available.  */
			if (!varargs && far > 0 && gar > 0)
			  {
			    if (first_member_is_fixed_point == false)
			      {
				pass_in_far (regcache, far--, val);
				pass_in_gar (regcache, gar--, val + regsize);
			      }
			    else
			      {
				pass_in_gar (regcache, gar--, val);
				pass_in_far (regcache, far--, val + regsize);
			      }
			  }
			else if ((!varargs && far == 0 && gar >= 2) || (varargs && gar >= 2))
			  {
			    pass_in_gar (regcache, gar--, val);
			    pass_in_gar (regcache, gar--, val + regsize);
			  }
			else if ((!varargs && far == 0 && gar == 1) || (varargs && gar == 1))
			  {
			    pass_in_gar (regcache, gar--, val);
			    pass_on_stack (regcache, val + regsize, len - regsize, align, &addr);
			  }
			else if ((!varargs && far == 0 && gar == 0) || (varargs && gar == 0))
			  {
			    pass_on_stack (regcache, val, len, align, &addr);
			  }
		      }
		    else
		      {
			/* The argument is passed in a pair of available GAR,
			   with the low-order bits in the lower-numbered GAR
			   and the high-order bits in the higher-numbered GAR.
			   If only one GAR is available,
			   the low-order bits are in the GAR
			   and the high-order bits are on the stack,
			   and passed on the stack if no GAR is available.  */
			if (gar >= 2)
			  {
			    pass_in_gar (regcache, gar--, val);
			    pass_in_gar (regcache, gar--, val + regsize);
			  }
			else if (gar == 1)
			  {
			    pass_in_gar (regcache, gar--, val);
			    pass_on_stack (regcache, val + regsize, len - regsize, align, &addr);
			  }
			else
			  {
			    pass_on_stack (regcache, val, len, align, &addr);
			  }
		      }
		  }
	      }
	    else if (len > 2 * regsize)
	      {
		/* It's passed by reference and are replaced in the argument list with the address.
		   If there is an available GAR, the reference is passed in the GAR,
		   and passed on the stack if no GAR is available.  */
		sp = align_down (sp - len, 16);
		write_memory (sp, val, len);

		if (gar > 0)
		  pass_in_gar (regcache, gar--, (const gdb_byte *) &sp);
		else
		  pass_on_stack (regcache, (const gdb_byte*) &sp, len, regsize, &addr);
	      }
	  }
	  break;
	case TYPE_CODE_UNION:
	  /* Union is passed in GAR or stack.  */
	  if (len > 0 && len <= regsize)
	    {
	      /* The argument is passed in a GAR,
		 or on the stack by value if no GAR is available.  */
	      if (gar > 0)
		pass_in_gar (regcache, gar--, val);
	      else
		pass_on_stack (regcache, val, len, align, &addr);
	    }
	  else if (len > regsize && len <= 2 * regsize)
	    {
	      /* The argument is passed in a pair of available GAR,
		 with the low-order bits in the lower-numbered GAR
		 and the high-order bits in the higher-numbered GAR.
		 If only one GAR is available,
		 the low-order bits are in the GAR
		 and the high-order bits are on the stack.
		 The arguments are passed on the stack when no GAR is available.  */
	      if (gar >= 2)
		{
		  pass_in_gar (regcache, gar--, val);
		  pass_in_gar (regcache, gar--, val + regsize);
		}
	      else if (gar == 1)
		{
		  pass_in_gar (regcache, gar--, val);
		  pass_on_stack (regcache, val + regsize, len - regsize, align, &addr);
		}
	      else
		{
		  pass_on_stack (regcache, val, len, align, &addr);
		}
	    }
	  else if (len > 2 * regsize)
	    {
	      /* It's passed by reference and are replaced in the argument list with the address.
		 If there is an available GAR, the reference is passed in the GAR,
		 and passed on the stack if no GAR is available.  */
	      sp = align_down (sp - len, 16);
	      write_memory (sp, val, len);

	      if (gar > 0)
		pass_in_gar (regcache, gar--, (const gdb_byte *) &sp);
	      else
		pass_on_stack (regcache, (const gdb_byte*) &sp, len, regsize, &addr);
	    }
	  break;
	case TYPE_CODE_COMPLEX:
	  {
	    struct type *target_type = check_typedef (type->target_type ());
	    size_t target_len = target_type->length ();

	    if (target_len < regsize)
	      {
		/* The complex with two float members
		   is passed in a pair of available FAR,
		   with the low-order float member bits in the lower-numbered FAR
		   and the high-order float member bits in the higher-numbered FAR.
		   If the number of available FAR is less than 2, it's passed in a GAR,
		   and passed on the stack if no GAR is available.  */
		if (!varargs && far >= 2)
		  {
		    pass_in_far (regcache, far--, val);
		    pass_in_far (regcache, far--, val + align);
		  }
		else if (gar > 0)
		  {
		    pass_in_gar (regcache, gar--, val);
		  }
		else
		  {
		    pass_on_stack (regcache, val, len, align, &addr);
		  }
	      }
	    else if (target_len == regsize)
	      {
		/* The complex with two double members
		   is passed in a pair of available FAR,
		   with the low-order bits in the lower-numbered FAR
		   and the high-order bits in the higher-numbered FAR.
		   If no a pair of available FAR,
		   it's passed in a pair of available GAR,
		   with the low-order bits in the lower-numbered GAR
		   and the high-order bits in the higher-numbered GAR.
		   If only one GAR is available,
		   the low-order bits are in the GAR
		   and the high-order bits are on stack,
		   and passed on the stack if no GAR is available.  */
		{
		  if (!varargs && far >= 2)
		    {
		      pass_in_far (regcache, far--, val);
		      pass_in_far (regcache, far--, val + align);
		    }
		  else if (gar >= 2)
		    {
		      pass_in_gar (regcache, gar--, val);
		      pass_in_gar (regcache, gar--, val + align);
		    }
		  else if (gar == 1)
		    {
		      pass_in_gar (regcache, gar--, val);
		      pass_on_stack (regcache, val + align, len - align, align, &addr);
		    }
		  else
		    {
		      pass_on_stack (regcache, val, len, align, &addr);
		    }
		}
	      }
	    else if (target_len == 2 * regsize)
	      {
		/* The complex with two long double members
		   is passed by reference and are replaced in the argument list with the address.
		   If there is an available GAR, the reference is passed in the GAR,
		   and passed on the stack if no GAR is available.  */
		sp = align_down (sp - len, 16);
		write_memory (sp, val, len);

		if (gar > 0)
		  pass_in_gar (regcache, gar--, (const gdb_byte *) &sp);
		else
		  pass_on_stack (regcache, (const gdb_byte*) &sp, regsize, regsize, &addr);
	      }
	  }
	  break;
	default:
	  break;
	}
    }

  if (addr > buf)
    {
      sp -= addr - buf;
      sp = align_down (sp, 16);
      write_memory (sp, buf, addr - buf);
    }

  regcache_cooked_write_unsigned (regcache, LOONGARCH_RA_REGNUM, bp_addr);
  regcache_cooked_write_unsigned (regcache, LOONGARCH_SP_REGNUM, sp);

  return sp;
}

/* Partial transfer of a cooked register.  */

static void
loongarch_xfer_reg (struct regcache *regcache,
		    int regnum, int len, gdb_byte *readbuf,
		    const gdb_byte *writebuf, size_t offset)
{
  if (readbuf)
    regcache->cooked_read_part (regnum, 0, len, readbuf + offset);
  if (writebuf)
    regcache->cooked_write_part (regnum, 0, len, writebuf + offset);
}

/* Implement the return_value gdbarch method.  */

static enum return_value_convention
loongarch_return_value (struct gdbarch *gdbarch, struct value *function,
			struct type *type, struct regcache *regcache,
			gdb_byte *readbuf, const gdb_byte *writebuf)
{
  int regsize = register_size (gdbarch, 0);
  enum type_code code = type->code ();
  size_t len = type->length ();
  unsigned int fixed_point_members;
  unsigned int floating_point_members;
  bool first_member_is_fixed_point;
  bool has_long_double;
  unsigned int member_offsets[2];
  unsigned int member_lens[2];
  unsigned int fields;
  int a0 = LOONGARCH_A0_REGNUM;
  int a1 = LOONGARCH_A0_REGNUM + 1;
  int f0 = LOONGARCH_FIRST_FP_REGNUM;
  int f1 = LOONGARCH_FIRST_FP_REGNUM + 1;

  switch (code)
    {
    case TYPE_CODE_INT:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_CHAR:
    case TYPE_CODE_RANGE:
    case TYPE_CODE_ENUM:
    case TYPE_CODE_PTR:
      {
	/* integer or pointer type.
	   The return value is passed in a0,
	   the unsigned integer scalars are zero-extended to GRLEN bits,
	   and the signed integer scalars are sign-extended.  */
	if (writebuf)
	  {
	    gdb_byte buf[regsize];
	    if (type->is_unsigned ())
	      {
		ULONGEST data = extract_unsigned_integer (writebuf, len, BFD_ENDIAN_LITTLE);
		store_unsigned_integer (buf, regsize, BFD_ENDIAN_LITTLE, data);
	      }
	    else
	      {
		LONGEST data = extract_signed_integer (writebuf, len, BFD_ENDIAN_LITTLE);
		store_signed_integer (buf, regsize, BFD_ENDIAN_LITTLE, data);
	      }
	    loongarch_xfer_reg (regcache, a0, regsize, nullptr, buf, 0);
	  }
	else
	  loongarch_xfer_reg (regcache, a0, len, readbuf, nullptr, 0);
      }
      break;
    case TYPE_CODE_FLT:
      /* long double type.
	 The return value is passed in a0 and a1.  */
      if (len == 2 * regsize)
	{
	  loongarch_xfer_reg (regcache, a0, regsize, readbuf, writebuf, 0);
	  loongarch_xfer_reg (regcache, a1, len - regsize, readbuf, writebuf, regsize);
	}
      /* float or double type.
	 The return value is passed in f0.  */
      else
	{
	  loongarch_xfer_reg (regcache, f0, len, readbuf, writebuf, 0);
	}
      break;
    case TYPE_CODE_STRUCT:
      {
	fixed_point_members = 0;
	floating_point_members = 0;
	first_member_is_fixed_point = false;
	has_long_double = false;
	member_offsets[0] = member_offsets[1] = 0;
	member_lens[0] = member_offsets[1] = 0;
	fields = 0;
	compute_struct_member (type,
			       &fixed_point_members,
			       &floating_point_members,
			       &first_member_is_fixed_point,
			       &has_long_double);
	struct_member_info (type, member_offsets, member_lens, 0, &fields);
	/* struct consists of one floating-point member;
	   struct consists of two floating-point members;
	   struct consists of one floating-point member
	   and one integer member. */
	if (has_long_double == false
	    && ((fixed_point_members == 0 && floating_point_members == 1)
		|| (fixed_point_members == 0 && floating_point_members == 2)
		|| (fixed_point_members == 1 && floating_point_members == 1)))
	  {
	    if (fixed_point_members == 0 && floating_point_members == 1)
	      {
		loongarch_xfer_reg (regcache, f0, member_lens[0], readbuf,
				    writebuf, member_offsets[0]);
	      }
	    else if (fixed_point_members == 0 && floating_point_members == 2)
	      {
		loongarch_xfer_reg (regcache, f0, member_lens[0], readbuf,
				    writebuf, member_offsets[0]);
		loongarch_xfer_reg (regcache, f1, member_lens[1], readbuf,
				    writebuf, member_offsets[1]);
	      }
	    else if (fixed_point_members == 1 && floating_point_members == 1)
	      {
		if (first_member_is_fixed_point == false)
		  {
		    loongarch_xfer_reg (regcache, f0, member_lens[0], readbuf,
					writebuf, member_offsets[0]);
		    loongarch_xfer_reg (regcache, a0, member_lens[1], readbuf,
					writebuf, member_offsets[1]);
		  }
		else
		  {
		    loongarch_xfer_reg (regcache, a0, member_lens[0], readbuf,
					writebuf, member_offsets[0]);
		    loongarch_xfer_reg (regcache, f0, member_lens[1], readbuf,
					writebuf, member_offsets[1]);
		  }
	      }
	  }
	else if (len > 0 && len <= regsize)
	  {
	    /* The structure has only fixed-point members.  */
	    if (fixed_point_members > 0 && floating_point_members == 0)
	      {
		/* The return value is passed in a0.  */
		loongarch_xfer_reg (regcache, a0, len, readbuf, writebuf, 0);
	      }
	    /* The structure has only floating-point members.  */
	    else if (fixed_point_members == 0 && floating_point_members > 0)
	      {
		/* The structure has one floating-point member.
		   The return value is passed in f0.  */
		if (floating_point_members == 1)
		  {
		    loongarch_xfer_reg (regcache, f0, len, readbuf, writebuf, 0);
		  }
		/* The structure has two floating-point members.
		   The return value is passed in f0 and f1.  */
		else if (floating_point_members == 2)
		  {
		    loongarch_xfer_reg (regcache, f0, len / 2, readbuf, writebuf, 0);
		    loongarch_xfer_reg (regcache, f1, len / 2, readbuf, writebuf, len / 2);
		  }
	      }
	    /* The structure has both fixed-point and floating-point members.  */
	    else if (fixed_point_members > 0 && floating_point_members > 0)
	      {
		/* The structure has one float member and multiple fixed-point members.
		   The return value is passed in a0.  */
		if (floating_point_members == 1 && fixed_point_members > 1)
		  {
		    loongarch_xfer_reg (regcache, a0, len, readbuf, writebuf, 0);
		  }
		/* The structure has one float member and one fixed-point member.  */
		else if (floating_point_members == 1 && fixed_point_members == 1)
		  {
		    /* The return value is passed in f0 and a0 if the first member is floating-point.  */
		    if (first_member_is_fixed_point == false)
		      {
			loongarch_xfer_reg (regcache, f0, regsize / 2, readbuf, writebuf, 0);
			loongarch_xfer_reg (regcache, a0, regsize / 2, readbuf, writebuf, regsize / 2);
		      }
		    /* The return value is passed in a0 and f0 if the first member is fixed-point.  */
		    else
		      {
			loongarch_xfer_reg (regcache, a0, regsize / 2, readbuf, writebuf, 0);
			loongarch_xfer_reg (regcache, f0, regsize / 2, readbuf, writebuf, regsize / 2);
		      }
		  }
	      }
	  }
	else if (len > regsize && len <= 2 * regsize)
	  {
	    /* The structure has only fixed-point members.  */
	    if (fixed_point_members > 0 && floating_point_members == 0)
	      {
		/* The return value is passed in a0 and a1.  */
		loongarch_xfer_reg (regcache, a0, regsize, readbuf, writebuf, 0);
		loongarch_xfer_reg (regcache, a1, len - regsize, readbuf, writebuf, regsize);
	      }
	    /* The structure has only floating-point members.  */
	    else if (fixed_point_members == 0 && floating_point_members > 0)
	      {
		/* The structure has one long double member
		   or one double member and two adjacent float members
		   or 3-4 float members.
		   The return value is passed in a0 and a1.  */
		if ((len == 16 && floating_point_members == 1)
		    || (len == 16 && floating_point_members == 3)
		    || (len == 12 && floating_point_members == 3)
		    || (len == 16 && floating_point_members == 4))
		  {
		    loongarch_xfer_reg (regcache, a0, regsize, readbuf, writebuf, 0);
		    loongarch_xfer_reg (regcache, a1, len - regsize, readbuf, writebuf, regsize);
		  }
		/* The structure has two double members
		   or one double member and one float member.
		   The return value is passed in f0 and f1.  */
		else if ((len == 16 && floating_point_members == 2)
			 || (len == 12 && floating_point_members == 2))
		  {
		    loongarch_xfer_reg (regcache, f0, regsize, readbuf, writebuf, 0);
		    loongarch_xfer_reg (regcache, f1, len - regsize, readbuf, writebuf, regsize);
		  }
	      }
	    /* The structure has both fixed-point and floating-point members.  */
	    else if (fixed_point_members > 0 && floating_point_members > 0)
	      {
		/* The structure has one floating-point member and one fixed-point member.  */
		if (floating_point_members == 1 && fixed_point_members == 1)
		  {
		    /* The return value is passed in f0 and a0 if the first member is floating-point.  */
		    if (first_member_is_fixed_point == false)
		      {
			loongarch_xfer_reg (regcache, f0, regsize, readbuf, writebuf, 0);
			loongarch_xfer_reg (regcache, a0, len - regsize, readbuf, writebuf, regsize);
		      }
		    /* The return value is passed in a0 and f0 if the first member is fixed-point.  */
		    else
		      {
			loongarch_xfer_reg (regcache, a0, regsize, readbuf, writebuf, 0);
			loongarch_xfer_reg (regcache, f0, len - regsize, readbuf, writebuf, regsize);
		      }
		  }
		else
		  {
		    /* The return value is passed in a0 and a1.  */
		    loongarch_xfer_reg (regcache, a0, regsize, readbuf, writebuf, 0);
		    loongarch_xfer_reg (regcache, a1, len - regsize, readbuf, writebuf, regsize);
		  }
	      }
	  }
	else if (len > 2 * regsize)
	  return RETURN_VALUE_STRUCT_CONVENTION;
      }
      break;
    case TYPE_CODE_UNION:
      if (len > 0 && len <= regsize)
	{
	  /* The return value is passed in a0.  */
	  loongarch_xfer_reg (regcache, a0, len, readbuf, writebuf, 0);
	}
      else if (len > regsize && len <= 2 * regsize)
	{
	  /* The return value is passed in a0 and a1.  */
	  loongarch_xfer_reg (regcache, a0, regsize, readbuf, writebuf, 0);
	  loongarch_xfer_reg (regcache, a1, len - regsize, readbuf, writebuf, regsize);
	}
      else if (len > 2 * regsize)
	return RETURN_VALUE_STRUCT_CONVENTION;
      break;
    case TYPE_CODE_COMPLEX:
      if (len > 0 && len <= 2 * regsize)
	{
	  /* The return value is passed in f0 and f1.  */
	  loongarch_xfer_reg (regcache, f0, len / 2, readbuf, writebuf, 0);
	  loongarch_xfer_reg (regcache, f1, len / 2, readbuf, writebuf, len / 2);
	}
      else if (len > 2 * regsize)
	return RETURN_VALUE_STRUCT_CONVENTION;
      break;
    default:
      break;
    }

  return RETURN_VALUE_REGISTER_CONVENTION;
}

/* Implement the dwarf2_reg_to_regnum gdbarch method.  */

static int
loongarch_dwarf2_reg_to_regnum (struct gdbarch *gdbarch, int regnum)
{
  if (regnum >= 0 && regnum < 32)
    return regnum;
  else if (regnum >= 32 && regnum < 66)
    return LOONGARCH_FIRST_FP_REGNUM + regnum - 32;
  else
    return -1;
}

static constexpr gdb_byte loongarch_default_breakpoint[] = {0x05, 0x00, 0x2a, 0x00};
typedef BP_MANIPULATION (loongarch_default_breakpoint) loongarch_breakpoint;

/* Extract a set of required target features out of ABFD.  If ABFD is nullptr
   then a LOONGARCH_GDBARCH_FEATURES is returned in its default state.  */

static struct loongarch_gdbarch_features
loongarch_features_from_bfd (const bfd *abfd)
{
  struct loongarch_gdbarch_features features;

  /* Now try to improve on the defaults by looking at the binary we are
     going to execute.  We assume the user knows what they are doing and
     that the target will match the binary.  Remember, this code path is
     only used at all if the target hasn't given us a description, so this
     is really a last ditched effort to do something sane before giving
     up.  */
  if (abfd != nullptr && bfd_get_flavour (abfd) == bfd_target_elf_flavour)
    {
      unsigned char eclass = elf_elfheader (abfd)->e_ident[EI_CLASS];
      int e_flags = elf_elfheader (abfd)->e_flags;

      if (eclass == ELFCLASS32)
	features.xlen = 4;
      else if (eclass == ELFCLASS64)
	features.xlen = 8;
      else
	internal_error (_("unknown ELF header class %d"), eclass);

      if (EF_LOONGARCH_IS_SINGLE_FLOAT (e_flags))
	features.fputype = SINGLE_FLOAT;
      else if (EF_LOONGARCH_IS_DOUBLE_FLOAT (e_flags))
	features.fputype = DOUBLE_FLOAT;
    }

  return features;
}

/* Find a suitable default target description.  Use the contents of INFO,
   specifically the bfd object being executed, to guide the selection of a
   suitable default target description.  */

static const struct target_desc *
loongarch_find_default_target_description (const struct gdbarch_info info)
{
  /* Extract desired feature set from INFO.  */
  struct loongarch_gdbarch_features features
    = loongarch_features_from_bfd (info.abfd);

  /* If the XLEN field is still 0 then we got nothing useful from INFO.BFD,
     maybe there was no bfd object.  In this case we fall back to a minimal
     useful target, the x-register size is selected based on the architecture
     from INFO.  */
  if (features.xlen == 0)
    features.xlen = info.bfd_arch_info->bits_per_address == 32 ? 4 : 8;

  /* If the FPUTYPE field is still 0 then we got nothing useful from INFO.BFD,
     maybe there was no bfd object.  In this case we fall back to a usual useful
     target with double float.  */
  if (features.fputype == 0)
    features.fputype = DOUBLE_FLOAT;

  /* Now build a target description based on the feature set.  */
  return loongarch_lookup_target_description (features);
}

static int
loongarch_register_reggroup_p (struct gdbarch *gdbarch, int regnum,
			       const struct reggroup *group)
{
  if (gdbarch_register_name (gdbarch, regnum) == NULL
      || *gdbarch_register_name (gdbarch, regnum) == '\0')
    return 0;

  int raw_p = regnum < gdbarch_num_regs (gdbarch);

  if (group == save_reggroup || group == restore_reggroup)
    return raw_p;

  if (group == all_reggroup)
    return 1;

  if (0 <= regnum && regnum <= LOONGARCH_BADV_REGNUM)
    return group == general_reggroup;

  /* Only ORIG_A0, PC, BADV in general_reggroup */
  if (group == general_reggroup)
    return 0;

  if (LOONGARCH_FIRST_FP_REGNUM <= regnum && regnum <= LOONGARCH_FCSR_REGNUM)
    return group == float_reggroup;

  /* Only $fx / $fccx / $fcsr in float_reggroup */
  if (group == float_reggroup)
    return 0;

  int ret = tdesc_register_in_reggroup_p (gdbarch, regnum, group);
  if (ret != -1)
    return ret;

  return default_register_reggroup_p (gdbarch, regnum, group);
}

/* Initialize the current architecture based on INFO  */

static struct gdbarch *
loongarch_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  size_t regnum = 0;
  struct loongarch_gdbarch_features features;
  tdesc_arch_data_up tdesc_data = tdesc_data_alloc ();
  const struct target_desc *tdesc = info.target_desc;

  /* Ensure we always have a target description.  */
  if (!tdesc_has_registers (tdesc))
    tdesc = loongarch_find_default_target_description (info);

  const struct tdesc_feature *feature_cpu
    = tdesc_find_feature (tdesc, "org.gnu.gdb.loongarch.base");
  if (feature_cpu == nullptr)
    return nullptr;


  /* Validate the description provides the mandatory base registers
     and allocate their numbers.  */
  bool valid_p = true;
  for (int i = 0; i < 32; i++)
    valid_p &= tdesc_numbered_register (feature_cpu, tdesc_data.get (), regnum++,
					loongarch_r_normal_name[i] + 1);
  valid_p &= tdesc_numbered_register (feature_cpu, tdesc_data.get (), regnum++, "orig_a0");
  valid_p &= tdesc_numbered_register (feature_cpu, tdesc_data.get (), regnum++, "pc");
  valid_p &= tdesc_numbered_register (feature_cpu, tdesc_data.get (), regnum++, "badv");
  if (!valid_p)
    return nullptr;

  const struct tdesc_feature *feature_fpu
    = tdesc_find_feature (tdesc, "org.gnu.gdb.loongarch.fpu");
  if (feature_fpu == nullptr)
    return nullptr;

  /* Validate the description provides the fpu registers and
     allocate their numbers.  */
  regnum = LOONGARCH_FIRST_FP_REGNUM;
  for (int i = 0; i < LOONGARCH_LINUX_NUM_FPREGSET; i++)
    valid_p &= tdesc_numbered_register (feature_fpu, tdesc_data.get (), regnum++,
					loongarch_f_normal_name[i] + 1);
  for (int i = 0; i < LOONGARCH_LINUX_NUM_FCC; i++)
    valid_p &= tdesc_numbered_register (feature_fpu, tdesc_data.get (), regnum++,
					loongarch_c_normal_name[i] + 1);
  valid_p &= tdesc_numbered_register (feature_fpu, tdesc_data.get (), regnum++, "fcsr");
  if (!valid_p)
    return nullptr;

  /* LoongArch code is always little-endian.  */
  info.byte_order_for_code = BFD_ENDIAN_LITTLE;

  /* Have a look at what the supplied (if any) bfd object requires of the
     target, then check that this matches with what the target is
     providing.  */
  struct loongarch_gdbarch_features abi_features
    = loongarch_features_from_bfd (info.abfd);

  /* If the ABI_FEATURES xlen or fputype is 0 then this indicates we got
     no useful abi features from the INFO object.  In this case we just
     treat the hardware features as defining the abi.  */
  if (abi_features.xlen == 0)
    {
      int xlen_bitsize = tdesc_register_bitsize (feature_cpu, "pc");
      features.xlen = (xlen_bitsize / 8);
      features.fputype = abi_features.fputype;
      abi_features = features;
    }
  if (abi_features.fputype == 0)
    {
      features.xlen = abi_features.xlen;
      features.fputype = DOUBLE_FLOAT;
      abi_features = features;
    }

  /* Find a candidate among the list of pre-declared architectures.  */
  for (arches = gdbarch_list_lookup_by_info (arches, &info);
       arches != nullptr;
       arches = gdbarch_list_lookup_by_info (arches->next, &info))
    {
      /* Check that the feature set of the ARCHES matches the feature set
	 we are looking for.  If it doesn't then we can't reuse this
	 gdbarch.  */
      loongarch_gdbarch_tdep *candidate_tdep
	= gdbarch_tdep<loongarch_gdbarch_tdep> (arches->gdbarch);

      if (candidate_tdep->abi_features != abi_features)
	continue;

      break;
    }

  if (arches != nullptr)
    return arches->gdbarch;

  /* None found, so create a new architecture from the information provided.  */
  gdbarch *gdbarch
    = gdbarch_alloc (&info, gdbarch_tdep_up (new loongarch_gdbarch_tdep));
  loongarch_gdbarch_tdep *tdep = gdbarch_tdep<loongarch_gdbarch_tdep> (gdbarch);

  tdep->abi_features = abi_features;

  /* Target data types.  */
  set_gdbarch_short_bit (gdbarch, 16);
  set_gdbarch_int_bit (gdbarch, 32);
  set_gdbarch_long_bit (gdbarch, info.bfd_arch_info->bits_per_address);
  set_gdbarch_long_long_bit (gdbarch, 64);
  set_gdbarch_float_bit (gdbarch, 32);
  set_gdbarch_double_bit (gdbarch, 64);
  set_gdbarch_long_double_bit (gdbarch, 128);
  set_gdbarch_long_double_format (gdbarch, floatformats_ieee_quad);
  set_gdbarch_ptr_bit (gdbarch, info.bfd_arch_info->bits_per_address);
  set_gdbarch_char_signed (gdbarch, 0);

  info.target_desc = tdesc;
  info.tdesc_data = tdesc_data.get ();

  for (int i = 0; i < ARRAY_SIZE (loongarch_r_alias); ++i)
    if (loongarch_r_alias[i][0] != '\0')
      user_reg_add (gdbarch, loongarch_r_alias[i] + 1,
	value_of_loongarch_user_reg, (void *) (size_t) i);

  for (int i = 0; i < ARRAY_SIZE (loongarch_f_alias); ++i)
    {
      if (loongarch_f_alias[i][0] != '\0')
	user_reg_add (gdbarch, loongarch_f_alias[i] + 1,
		      value_of_loongarch_user_reg,
		      (void *) (size_t) (LOONGARCH_FIRST_FP_REGNUM + i));
    }

  /* Information about registers.  */
  set_gdbarch_num_regs (gdbarch, regnum);
  set_gdbarch_sp_regnum (gdbarch, LOONGARCH_SP_REGNUM);
  set_gdbarch_pc_regnum (gdbarch, LOONGARCH_PC_REGNUM);

  /* Finalise the target description registers.  */
  tdesc_use_registers (gdbarch, tdesc, std::move (tdesc_data));

  /* Functions handling dummy frames.  */
  set_gdbarch_push_dummy_call (gdbarch, loongarch_push_dummy_call);

  /* Return value info  */
  set_gdbarch_return_value (gdbarch, loongarch_return_value);

  /* Advance PC across function entry code.  */
  set_gdbarch_skip_prologue (gdbarch, loongarch_skip_prologue);

  /* Stack grows downward.  */
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);

  /* Frame info.  */
  set_gdbarch_frame_align (gdbarch, loongarch_frame_align);

  /* Breakpoint manipulation.  */
  set_gdbarch_software_single_step (gdbarch, loongarch_software_single_step);
  set_gdbarch_breakpoint_kind_from_pc (gdbarch, loongarch_breakpoint::kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch, loongarch_breakpoint::bp_from_kind);

  /* Frame unwinders. Use DWARF debug info if available, otherwise use our own unwinder.  */
  set_gdbarch_dwarf2_reg_to_regnum (gdbarch, loongarch_dwarf2_reg_to_regnum);
  dwarf2_append_unwinders (gdbarch);
  frame_unwind_append_unwinder (gdbarch, &loongarch_frame_unwind);

  /* Hook in OS ABI-specific overrides, if they have been registered.  */
  gdbarch_init_osabi (info, gdbarch);
  set_gdbarch_register_reggroup_p (gdbarch, loongarch_register_reggroup_p);

  return gdbarch;
}

void _initialize_loongarch_tdep ();
void
_initialize_loongarch_tdep ()
{
  gdbarch_register (bfd_arch_loongarch, loongarch_gdbarch_init, nullptr);
}
