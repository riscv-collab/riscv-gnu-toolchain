/* Target-dependent code for s390.

   Copyright (C) 2001-2024 Free Software Foundation, Inc.

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
#include "ax-gdb.h"
#include "dwarf2/frame.h"
#include "elf/s390.h"
#include "elf-bfd.h"
#include "frame-base.h"
#include "frame-unwind.h"
#include "gdbarch.h"
#include "gdbcore.h"
#include "infrun.h"
#include "linux-tdep.h"
#include "objfiles.h"
#include "osabi.h"
#include "record-full.h"
#include "regcache.h"
#include "reggroups.h"
#include "s390-tdep.h"
#include "target-descriptions.h"
#include "trad-frame.h"
#include "value.h"
#include "inferior.h"

#include "features/s390-linux32.c"
#include "features/s390x-linux64.c"

/* Holds the current set of options to be passed to the disassembler.  */
static char *s390_disassembler_options;

/* Breakpoints.  */

constexpr gdb_byte s390_break_insn[] = { 0x0, 0x1 };

typedef BP_MANIPULATION (s390_break_insn) s390_breakpoint;

/* Types.  */

/* Implement the gdbarch type alignment method.  */

static ULONGEST
s390_type_align (gdbarch *gdbarch, struct type *t)
{
  t = check_typedef (t);

  if (t->length () > 8)
    {
      switch (t->code ())
	{
	case TYPE_CODE_INT:
	case TYPE_CODE_RANGE:
	case TYPE_CODE_FLT:
	case TYPE_CODE_ENUM:
	case TYPE_CODE_CHAR:
	case TYPE_CODE_BOOL:
	case TYPE_CODE_DECFLOAT:
	  return 8;

	case TYPE_CODE_ARRAY:
	  if (t->is_vector ())
	    return 8;
	  break;
	}
    }
  return 0;
}

/* Decoding S/390 instructions.  */

/* Read a single instruction from address AT.  */

static int
s390_readinstruction (bfd_byte instr[], CORE_ADDR at)
{
  static int s390_instrlen[] = { 2, 4, 4, 6 };
  int instrlen;

  if (target_read_memory (at, &instr[0], 2))
    return -1;
  instrlen = s390_instrlen[instr[0] >> 6];
  if (instrlen > 2)
    {
      if (target_read_memory (at + 2, &instr[2], instrlen - 2))
	return -1;
    }
  return instrlen;
}

/* The functions below are for recognizing and decoding S/390
   instructions of various formats.  Each of them checks whether INSN
   is an instruction of the given format, with the specified opcodes.
   If it is, it sets the remaining arguments to the values of the
   instruction's fields, and returns a non-zero value; otherwise, it
   returns zero.

   These functions' arguments appear in the order they appear in the
   instruction, not in the machine-language form.  So, opcodes always
   come first, even though they're sometimes scattered around the
   instructions.  And displacements appear before base and extension
   registers, as they do in the assembly syntax, not at the end, as
   they do in the machine language.

   Test for RI instruction format.  */

static int
is_ri (bfd_byte *insn, int op1, int op2, unsigned int *r1, int *i2)
{
  if (insn[0] == op1 && (insn[1] & 0xf) == op2)
    {
      *r1 = (insn[1] >> 4) & 0xf;
      /* i2 is a 16-bit signed quantity.  */
      *i2 = (((insn[2] << 8) | insn[3]) ^ 0x8000) - 0x8000;
      return 1;
    }
  else
    return 0;
}

/* Test for RIL instruction format.  See comment on is_ri for details.  */

static int
is_ril (bfd_byte *insn, int op1, int op2,
	unsigned int *r1, int *i2)
{
  if (insn[0] == op1 && (insn[1] & 0xf) == op2)
    {
      *r1 = (insn[1] >> 4) & 0xf;
      /* i2 is a signed quantity.  If the host 'int' is 32 bits long,
	 no sign extension is necessary, but we don't want to assume
	 that.  */
      *i2 = (((insn[2] << 24)
	      | (insn[3] << 16)
	      | (insn[4] << 8)
	      | (insn[5])) ^ 0x80000000) - 0x80000000;
      return 1;
    }
  else
    return 0;
}

/* Test for RR instruction format.  See comment on is_ri for details.  */

static int
is_rr (bfd_byte *insn, int op, unsigned int *r1, unsigned int *r2)
{
  if (insn[0] == op)
    {
      *r1 = (insn[1] >> 4) & 0xf;
      *r2 = insn[1] & 0xf;
      return 1;
    }
  else
    return 0;
}

/* Test for RRE instruction format.  See comment on is_ri for details.  */

static int
is_rre (bfd_byte *insn, int op, unsigned int *r1, unsigned int *r2)
{
  if (((insn[0] << 8) | insn[1]) == op)
    {
      /* Yes, insn[3].  insn[2] is unused in RRE format.  */
      *r1 = (insn[3] >> 4) & 0xf;
      *r2 = insn[3] & 0xf;
      return 1;
    }
  else
    return 0;
}

/* Test for RS instruction format.  See comment on is_ri for details.  */

static int
is_rs (bfd_byte *insn, int op,
       unsigned int *r1, unsigned int *r3, int *d2, unsigned int *b2)
{
  if (insn[0] == op)
    {
      *r1 = (insn[1] >> 4) & 0xf;
      *r3 = insn[1] & 0xf;
      *b2 = (insn[2] >> 4) & 0xf;
      *d2 = ((insn[2] & 0xf) << 8) | insn[3];
      return 1;
    }
  else
    return 0;
}

/* Test for RSY instruction format.  See comment on is_ri for details.  */

static int
is_rsy (bfd_byte *insn, int op1, int op2,
	unsigned int *r1, unsigned int *r3, int *d2, unsigned int *b2)
{
  if (insn[0] == op1
      && insn[5] == op2)
    {
      *r1 = (insn[1] >> 4) & 0xf;
      *r3 = insn[1] & 0xf;
      *b2 = (insn[2] >> 4) & 0xf;
      /* The 'long displacement' is a 20-bit signed integer.  */
      *d2 = ((((insn[2] & 0xf) << 8) | insn[3] | (insn[4] << 12))
		^ 0x80000) - 0x80000;
      return 1;
    }
  else
    return 0;
}

/* Test for RX instruction format.  See comment on is_ri for details.  */

static int
is_rx (bfd_byte *insn, int op,
       unsigned int *r1, int *d2, unsigned int *x2, unsigned int *b2)
{
  if (insn[0] == op)
    {
      *r1 = (insn[1] >> 4) & 0xf;
      *x2 = insn[1] & 0xf;
      *b2 = (insn[2] >> 4) & 0xf;
      *d2 = ((insn[2] & 0xf) << 8) | insn[3];
      return 1;
    }
  else
    return 0;
}

/* Test for RXY instruction format.  See comment on is_ri for details.  */

static int
is_rxy (bfd_byte *insn, int op1, int op2,
	unsigned int *r1, int *d2, unsigned int *x2, unsigned int *b2)
{
  if (insn[0] == op1
      && insn[5] == op2)
    {
      *r1 = (insn[1] >> 4) & 0xf;
      *x2 = insn[1] & 0xf;
      *b2 = (insn[2] >> 4) & 0xf;
      /* The 'long displacement' is a 20-bit signed integer.  */
      *d2 = ((((insn[2] & 0xf) << 8) | insn[3] | (insn[4] << 12))
		^ 0x80000) - 0x80000;
      return 1;
    }
  else
    return 0;
}

/* A helper for s390_software_single_step, decides if an instruction
   is a partial-execution instruction that needs to be executed until
   completion when in record mode.  If it is, returns 1 and writes
   instruction length to a pointer.  */

static int
s390_is_partial_instruction (struct gdbarch *gdbarch, CORE_ADDR loc, int *len)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  uint16_t insn;

  insn = read_memory_integer (loc, 2, byte_order);

  switch (insn >> 8)
    {
    case 0xa8: /* MVCLE */
      *len = 4;
      return 1;

    case 0xeb:
      {
	insn = read_memory_integer (loc + 4, 2, byte_order);
	if ((insn & 0xff) == 0x8e)
	  {
	    /* MVCLU */
	    *len = 6;
	    return 1;
	  }
      }
      break;
    }

  switch (insn)
    {
    case 0xb255: /* MVST */
    case 0xb263: /* CMPSC */
    case 0xb2a5: /* TRE */
    case 0xb2a6: /* CU21 */
    case 0xb2a7: /* CU12 */
    case 0xb9b0: /* CU14 */
    case 0xb9b1: /* CU24 */
    case 0xb9b2: /* CU41 */
    case 0xb9b3: /* CU42 */
    case 0xb92a: /* KMF */
    case 0xb92b: /* KMO */
    case 0xb92f: /* KMC */
    case 0xb92d: /* KMCTR */
    case 0xb92e: /* KM */
    case 0xb93c: /* PPNO */
    case 0xb990: /* TRTT */
    case 0xb991: /* TRTO */
    case 0xb992: /* TROT */
    case 0xb993: /* TROO */
      *len = 4;
      return 1;
    }

  return 0;
}

/* Implement the "software_single_step" gdbarch method, needed to single step
   through instructions like MVCLE in record mode, to make sure they are
   executed to completion.  Without that, record will save the full length
   of destination buffer on every iteration, even though the CPU will only
   process about 4kiB of it each time, leading to O(n**2) memory and time
   complexity.  */

static std::vector<CORE_ADDR>
s390_software_single_step (struct regcache *regcache)
{
  struct gdbarch *gdbarch = regcache->arch ();
  CORE_ADDR loc = regcache_read_pc (regcache);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int len;
  uint16_t insn;

  /* Special handling only if recording.  */
  if (!record_full_is_used ())
    return {};

  /* First, match a partial instruction.  */
  if (!s390_is_partial_instruction (gdbarch, loc, &len))
    return {};

  loc += len;

  /* Second, look for a branch back to it.  */
  insn = read_memory_integer (loc, 2, byte_order);
  if (insn != 0xa714) /* BRC with mask 1 */
    return {};

  insn = read_memory_integer (loc + 2, 2, byte_order);
  if (insn != (uint16_t) -(len / 2))
    return {};

  loc += 4;

  /* Found it, step past the whole thing.  */
  return {loc};
}

/* Displaced stepping.  */

/* Return true if INSN is a non-branch RIL-b or RIL-c format
   instruction.  */

static int
is_non_branch_ril (gdb_byte *insn)
{
  gdb_byte op1 = insn[0];

  if (op1 == 0xc4)
    {
      gdb_byte op2 = insn[1] & 0x0f;

      switch (op2)
	{
	case 0x02: /* llhrl */
	case 0x04: /* lghrl */
	case 0x05: /* lhrl */
	case 0x06: /* llghrl */
	case 0x07: /* sthrl */
	case 0x08: /* lgrl */
	case 0x0b: /* stgrl */
	case 0x0c: /* lgfrl */
	case 0x0d: /* lrl */
	case 0x0e: /* llgfrl */
	case 0x0f: /* strl */
	  return 1;
	}
    }
  else if (op1 == 0xc6)
    {
      gdb_byte op2 = insn[1] & 0x0f;

      switch (op2)
	{
	case 0x00: /* exrl */
	case 0x02: /* pfdrl */
	case 0x04: /* cghrl */
	case 0x05: /* chrl */
	case 0x06: /* clghrl */
	case 0x07: /* clhrl */
	case 0x08: /* cgrl */
	case 0x0a: /* clgrl */
	case 0x0c: /* cgfrl */
	case 0x0d: /* crl */
	case 0x0e: /* clgfrl */
	case 0x0f: /* clrl */
	  return 1;
	}
    }

  return 0;
}

typedef buf_displaced_step_copy_insn_closure
  s390_displaced_step_copy_insn_closure;

/* Implementation of gdbarch_displaced_step_copy_insn.  */

static displaced_step_copy_insn_closure_up
s390_displaced_step_copy_insn (struct gdbarch *gdbarch,
			       CORE_ADDR from, CORE_ADDR to,
			       struct regcache *regs)
{
  size_t len = gdbarch_max_insn_length (gdbarch);
  std::unique_ptr<s390_displaced_step_copy_insn_closure> closure
    (new s390_displaced_step_copy_insn_closure (len));
  gdb_byte *buf = closure->buf.data ();

  read_memory (from, buf, len);

  /* Adjust the displacement field of PC-relative RIL instructions,
     except branches.  The latter are handled in the fixup hook.  */
  if (is_non_branch_ril (buf))
    {
      LONGEST offset;

      offset = extract_signed_integer (buf + 2, 4, BFD_ENDIAN_BIG);
      offset = (from - to + offset * 2) / 2;

      /* If the instruction is too far from the jump pad, punt.  This
	 will usually happen with instructions in shared libraries.
	 We could probably support these by rewriting them to be
	 absolute or fully emulating them.  */
      if (offset < INT32_MIN || offset > INT32_MAX)
	{
	  /* Let the core fall back to stepping over the breakpoint
	     in-line.  */
	  displaced_debug_printf ("can't displaced step RIL instruction: offset "
				  "%s out of range", plongest (offset));

	  return NULL;
	}

      store_signed_integer (buf + 2, 4, BFD_ENDIAN_BIG, offset);
    }

  write_memory (to, buf, len);

  displaced_debug_printf ("copy %s->%s: %s",
			  paddress (gdbarch, from), paddress (gdbarch, to),
			  bytes_to_string (buf, len).c_str ());

  /* This is a work around for a problem with g++ 4.8.  */
  return displaced_step_copy_insn_closure_up (closure.release ());
}

/* Fix up the state of registers and memory after having single-stepped
   a displaced instruction.  */

static void
s390_displaced_step_fixup (struct gdbarch *gdbarch,
			   displaced_step_copy_insn_closure *closure_,
			   CORE_ADDR from, CORE_ADDR to,
			   struct regcache *regs, bool completed_p)
{
  CORE_ADDR pc = regcache_read_pc (regs);

  /* If the displaced instruction didn't complete successfully then all we
     need to do is restore the program counter.  */
  if (!completed_p)
    {
      pc = from + (pc - to);
      regcache_write_pc (regs, pc);
      return;
    }

  /* Our closure is a copy of the instruction.  */
  s390_displaced_step_copy_insn_closure *closure
    = (s390_displaced_step_copy_insn_closure *) closure_;
  gdb_byte *insn = closure->buf.data ();
  static int s390_instrlen[] = { 2, 4, 4, 6 };
  int insnlen = s390_instrlen[insn[0] >> 6];

  /* Fields for various kinds of instructions.  */
  unsigned int b2, r1, r2, x2, r3;
  int i2, d2;

  /* Get addressing mode bit.  */
  ULONGEST amode = 0;
  if (register_size (gdbarch, S390_PSWA_REGNUM) == 4)
    {
      regcache_cooked_read_unsigned (regs, S390_PSWA_REGNUM, &amode);
      amode &= 0x80000000;
    }

  displaced_debug_printf ("(s390) fixup (%s, %s) pc %s len %d amode 0x%x",
			  paddress (gdbarch, from), paddress (gdbarch, to),
			  paddress (gdbarch, pc), insnlen, (int) amode);

  /* Handle absolute branch and save instructions.  */
  int op_basr_p = is_rr (insn, op_basr, &r1, &r2);
  if (op_basr_p
      || is_rx (insn, op_bas, &r1, &d2, &x2, &b2))
    {
      /* Recompute saved return address in R1.  */
      regcache_cooked_write_unsigned (regs, S390_R0_REGNUM + r1,
				      amode | (from + insnlen));
      /* Update PC iff the instruction doesn't actually branch.  */
      if (op_basr_p && r2 == 0)
	regcache_write_pc (regs, from + insnlen);
    }

  /* Handle absolute branch instructions.  */
  else if (is_rr (insn, op_bcr, &r1, &r2)
	   || is_rx (insn, op_bc, &r1, &d2, &x2, &b2)
	   || is_rr (insn, op_bctr, &r1, &r2)
	   || is_rre (insn, op_bctgr, &r1, &r2)
	   || is_rx (insn, op_bct, &r1, &d2, &x2, &b2)
	   || is_rxy (insn, op1_bctg, op2_brctg, &r1, &d2, &x2, &b2)
	   || is_rs (insn, op_bxh, &r1, &r3, &d2, &b2)
	   || is_rsy (insn, op1_bxhg, op2_bxhg, &r1, &r3, &d2, &b2)
	   || is_rs (insn, op_bxle, &r1, &r3, &d2, &b2)
	   || is_rsy (insn, op1_bxleg, op2_bxleg, &r1, &r3, &d2, &b2))
    {
      /* Update PC iff branch was *not* taken.  */
      if (pc == to + insnlen)
	regcache_write_pc (regs, from + insnlen);
    }

  /* Handle PC-relative branch and save instructions.  */
  else if (is_ri (insn, op1_bras, op2_bras, &r1, &i2)
	   || is_ril (insn, op1_brasl, op2_brasl, &r1, &i2))
    {
      /* Update PC.  */
      regcache_write_pc (regs, pc - to + from);
      /* Recompute saved return address in R1.  */
      regcache_cooked_write_unsigned (regs, S390_R0_REGNUM + r1,
				      amode | (from + insnlen));
    }

  /* Handle LOAD ADDRESS RELATIVE LONG.  */
  else if (is_ril (insn, op1_larl, op2_larl, &r1, &i2))
    {
      /* Update PC.  */
      regcache_write_pc (regs, from + insnlen);
      /* Recompute output address in R1.  */
      regcache_cooked_write_unsigned (regs, S390_R0_REGNUM + r1,
				      from + i2 * 2);
    }

  /* If we executed a breakpoint instruction, point PC right back at it.  */
  else if (insn[0] == 0x0 && insn[1] == 0x1)
    regcache_write_pc (regs, from);

  /* For any other insn, adjust PC by negated displacement.  PC then
     points right after the original instruction, except for PC-relative
     branches, where it points to the adjusted branch target.  */
  else
    regcache_write_pc (regs, pc - to + from);

  displaced_debug_printf ("(s390) pc is now %s",
			  paddress (gdbarch, regcache_read_pc (regs)));
}

/* Implement displaced_step_hw_singlestep gdbarch method.  */

static bool
s390_displaced_step_hw_singlestep (struct gdbarch *gdbarch)
{
  return true;
}

/* Prologue analysis.  */

struct s390_prologue_data {

  /* The stack.  */
  struct pv_area *stack;

  /* The size and byte-order of a GPR or FPR.  */
  int gpr_size;
  int fpr_size;
  enum bfd_endian byte_order;

  /* The general-purpose registers.  */
  pv_t gpr[S390_NUM_GPRS];

  /* The floating-point registers.  */
  pv_t fpr[S390_NUM_FPRS];

  /* The offset relative to the CFA where the incoming GPR N was saved
     by the function prologue.  0 if not saved or unknown.  */
  int gpr_slot[S390_NUM_GPRS];

  /* Likewise for FPRs.  */
  int fpr_slot[S390_NUM_FPRS];

  /* Nonzero if the backchain was saved.  This is assumed to be the
     case when the incoming SP is saved at the current SP location.  */
  int back_chain_saved_p;
};

/* Return the effective address for an X-style instruction, like:

	L R1, D2(X2, B2)

   Here, X2 and B2 are registers, and D2 is a signed 20-bit
   constant; the effective address is the sum of all three.  If either
   X2 or B2 are zero, then it doesn't contribute to the sum --- this
   means that r0 can't be used as either X2 or B2.  */

static pv_t
s390_addr (struct s390_prologue_data *data,
	   int d2, unsigned int x2, unsigned int b2)
{
  pv_t result;

  result = pv_constant (d2);
  if (x2)
    result = pv_add (result, data->gpr[x2]);
  if (b2)
    result = pv_add (result, data->gpr[b2]);

  return result;
}

/* Do a SIZE-byte store of VALUE to D2(X2,B2).  */

static void
s390_store (struct s390_prologue_data *data,
	    int d2, unsigned int x2, unsigned int b2, CORE_ADDR size,
	    pv_t value)
{
  pv_t addr = s390_addr (data, d2, x2, b2);
  pv_t offset;

  /* Check whether we are storing the backchain.  */
  offset = pv_subtract (data->gpr[S390_SP_REGNUM - S390_R0_REGNUM], addr);

  if (pv_is_constant (offset) && offset.k == 0)
    if (size == data->gpr_size
	&& pv_is_register_k (value, S390_SP_REGNUM, 0))
      {
	data->back_chain_saved_p = 1;
	return;
      }

  /* Check whether we are storing a register into the stack.  */
  if (!data->stack->store_would_trash (addr))
    data->stack->store (addr, size, value);

  /* Note: If this is some store we cannot identify, you might think we
     should forget our cached values, as any of those might have been hit.

     However, we make the assumption that the register save areas are only
     ever stored to once in any given function, and we do recognize these
     stores.  Thus every store we cannot recognize does not hit our data.  */
}

/* Do a SIZE-byte load from D2(X2,B2).  */

static pv_t
s390_load (struct s390_prologue_data *data,
	   int d2, unsigned int x2, unsigned int b2, CORE_ADDR size)

{
  pv_t addr = s390_addr (data, d2, x2, b2);

  /* If it's a load from an in-line constant pool, then we can
     simulate that, under the assumption that the code isn't
     going to change between the time the processor actually
     executed it creating the current frame, and the time when
     we're analyzing the code to unwind past that frame.  */
  if (pv_is_constant (addr))
    {
      const struct target_section *secp
	= target_section_by_addr (current_inferior ()->top_target (), addr.k);
      if (secp != NULL
	  && (bfd_section_flags (secp->the_bfd_section) & SEC_READONLY))
	return pv_constant (read_memory_integer (addr.k, size,
						 data->byte_order));
    }

  /* Check whether we are accessing one of our save slots.  */
  return data->stack->fetch (addr, size);
}

/* Function for finding saved registers in a 'struct pv_area'; we pass
   this to pv_area::scan.

   If VALUE is a saved register, ADDR says it was saved at a constant
   offset from the frame base, and SIZE indicates that the whole
   register was saved, record its offset in the reg_offset table in
   PROLOGUE_UNTYPED.  */

static void
s390_check_for_saved (void *data_untyped, pv_t addr,
		      CORE_ADDR size, pv_t value)
{
  struct s390_prologue_data *data = (struct s390_prologue_data *) data_untyped;
  int i, offset;

  if (!pv_is_register (addr, S390_SP_REGNUM))
    return;

  offset = 16 * data->gpr_size + 32 - addr.k;

  /* If we are storing the original value of a register, we want to
     record the CFA offset.  If the same register is stored multiple
     times, the stack slot with the highest address counts.  */

  for (i = 0; i < S390_NUM_GPRS; i++)
    if (size == data->gpr_size
	&& pv_is_register_k (value, S390_R0_REGNUM + i, 0))
      if (data->gpr_slot[i] == 0
	  || data->gpr_slot[i] > offset)
	{
	  data->gpr_slot[i] = offset;
	  return;
	}

  for (i = 0; i < S390_NUM_FPRS; i++)
    if (size == data->fpr_size
	&& pv_is_register_k (value, S390_F0_REGNUM + i, 0))
      if (data->fpr_slot[i] == 0
	  || data->fpr_slot[i] > offset)
	{
	  data->fpr_slot[i] = offset;
	  return;
	}
}

/* Analyze the prologue of the function starting at START_PC, continuing at
   most until CURRENT_PC.  Initialize DATA to hold all information we find
   out about the state of the registers and stack slots.  Return the address
   of the instruction after the last one that changed the SP, FP, or back
   chain; or zero on error.  */

static CORE_ADDR
s390_analyze_prologue (struct gdbarch *gdbarch,
		       CORE_ADDR start_pc,
		       CORE_ADDR current_pc,
		       struct s390_prologue_data *data)
{
  int word_size = gdbarch_ptr_bit (gdbarch) / 8;

  /* Our return value:
     The address of the instruction after the last one that changed
     the SP, FP, or back chain;  zero if we got an error trying to
     read memory.  */
  CORE_ADDR result = start_pc;

  /* The current PC for our abstract interpretation.  */
  CORE_ADDR pc;

  /* The address of the next instruction after that.  */
  CORE_ADDR next_pc;

  pv_area stack (S390_SP_REGNUM, gdbarch_addr_bit (gdbarch));
  scoped_restore restore_stack = make_scoped_restore (&data->stack, &stack);

  /* Set up everything's initial value.  */
  {
    int i;

    /* For the purpose of prologue tracking, we consider the GPR size to
       be equal to the ABI word size, even if it is actually larger
       (i.e. when running a 32-bit binary under a 64-bit kernel).  */
    data->gpr_size = word_size;
    data->fpr_size = 8;
    data->byte_order = gdbarch_byte_order (gdbarch);

    for (i = 0; i < S390_NUM_GPRS; i++)
      data->gpr[i] = pv_register (S390_R0_REGNUM + i, 0);

    for (i = 0; i < S390_NUM_FPRS; i++)
      data->fpr[i] = pv_register (S390_F0_REGNUM + i, 0);

    for (i = 0; i < S390_NUM_GPRS; i++)
      data->gpr_slot[i]  = 0;

    for (i = 0; i < S390_NUM_FPRS; i++)
      data->fpr_slot[i]  = 0;

    data->back_chain_saved_p = 0;
  }

  /* Start interpreting instructions, until we hit the frame's
     current PC or the first branch instruction.  */
  for (pc = start_pc; pc > 0 && pc < current_pc; pc = next_pc)
    {
      bfd_byte insn[S390_MAX_INSTR_SIZE];
      int insn_len = s390_readinstruction (insn, pc);

      bfd_byte dummy[S390_MAX_INSTR_SIZE] = { 0 };
      bfd_byte *insn32 = word_size == 4 ? insn : dummy;
      bfd_byte *insn64 = word_size == 8 ? insn : dummy;

      /* Fields for various kinds of instructions.  */
      unsigned int b2, r1, r2, x2, r3;
      int i2, d2;

      /* The values of SP and FP before this instruction,
	 for detecting instructions that change them.  */
      pv_t pre_insn_sp, pre_insn_fp;
      /* Likewise for the flag whether the back chain was saved.  */
      int pre_insn_back_chain_saved_p;

      /* If we got an error trying to read the instruction, report it.  */
      if (insn_len < 0)
	{
	  result = 0;
	  break;
	}

      next_pc = pc + insn_len;

      pre_insn_sp = data->gpr[S390_SP_REGNUM - S390_R0_REGNUM];
      pre_insn_fp = data->gpr[S390_FRAME_REGNUM - S390_R0_REGNUM];
      pre_insn_back_chain_saved_p = data->back_chain_saved_p;

      /* LHI r1, i2 --- load halfword immediate.  */
      /* LGHI r1, i2 --- load halfword immediate (64-bit version).  */
      /* LGFI r1, i2 --- load fullword immediate.  */
      if (is_ri (insn32, op1_lhi, op2_lhi, &r1, &i2)
	  || is_ri (insn64, op1_lghi, op2_lghi, &r1, &i2)
	  || is_ril (insn, op1_lgfi, op2_lgfi, &r1, &i2))
	data->gpr[r1] = pv_constant (i2);

      /* LR r1, r2 --- load from register.  */
      /* LGR r1, r2 --- load from register (64-bit version).  */
      else if (is_rr (insn32, op_lr, &r1, &r2)
	       || is_rre (insn64, op_lgr, &r1, &r2))
	data->gpr[r1] = data->gpr[r2];

      /* L r1, d2(x2, b2) --- load.  */
      /* LY r1, d2(x2, b2) --- load (long-displacement version).  */
      /* LG r1, d2(x2, b2) --- load (64-bit version).  */
      else if (is_rx (insn32, op_l, &r1, &d2, &x2, &b2)
	       || is_rxy (insn32, op1_ly, op2_ly, &r1, &d2, &x2, &b2)
	       || is_rxy (insn64, op1_lg, op2_lg, &r1, &d2, &x2, &b2))
	data->gpr[r1] = s390_load (data, d2, x2, b2, data->gpr_size);

      /* ST r1, d2(x2, b2) --- store.  */
      /* STY r1, d2(x2, b2) --- store (long-displacement version).  */
      /* STG r1, d2(x2, b2) --- store (64-bit version).  */
      else if (is_rx (insn32, op_st, &r1, &d2, &x2, &b2)
	       || is_rxy (insn32, op1_sty, op2_sty, &r1, &d2, &x2, &b2)
	       || is_rxy (insn64, op1_stg, op2_stg, &r1, &d2, &x2, &b2))
	s390_store (data, d2, x2, b2, data->gpr_size, data->gpr[r1]);

      /* STD r1, d2(x2,b2) --- store floating-point register.  */
      else if (is_rx (insn, op_std, &r1, &d2, &x2, &b2))
	s390_store (data, d2, x2, b2, data->fpr_size, data->fpr[r1]);

      /* STM r1, r3, d2(b2) --- store multiple.  */
      /* STMY r1, r3, d2(b2) --- store multiple (long-displacement
	 version).  */
      /* STMG r1, r3, d2(b2) --- store multiple (64-bit version).  */
      else if (is_rs (insn32, op_stm, &r1, &r3, &d2, &b2)
	       || is_rsy (insn32, op1_stmy, op2_stmy, &r1, &r3, &d2, &b2)
	       || is_rsy (insn64, op1_stmg, op2_stmg, &r1, &r3, &d2, &b2))
	{
	  for (; r1 <= r3; r1++, d2 += data->gpr_size)
	    s390_store (data, d2, 0, b2, data->gpr_size, data->gpr[r1]);
	}

      /* AHI r1, i2 --- add halfword immediate.  */
      /* AGHI r1, i2 --- add halfword immediate (64-bit version).  */
      /* AFI r1, i2 --- add fullword immediate.  */
      /* AGFI r1, i2 --- add fullword immediate (64-bit version).  */
      else if (is_ri (insn32, op1_ahi, op2_ahi, &r1, &i2)
	       || is_ri (insn64, op1_aghi, op2_aghi, &r1, &i2)
	       || is_ril (insn32, op1_afi, op2_afi, &r1, &i2)
	       || is_ril (insn64, op1_agfi, op2_agfi, &r1, &i2))
	data->gpr[r1] = pv_add_constant (data->gpr[r1], i2);

      /* ALFI r1, i2 --- add logical immediate.  */
      /* ALGFI r1, i2 --- add logical immediate (64-bit version).  */
      else if (is_ril (insn32, op1_alfi, op2_alfi, &r1, &i2)
	       || is_ril (insn64, op1_algfi, op2_algfi, &r1, &i2))
	data->gpr[r1] = pv_add_constant (data->gpr[r1],
					 (CORE_ADDR)i2 & 0xffffffff);

      /* AR r1, r2 -- add register.  */
      /* AGR r1, r2 -- add register (64-bit version).  */
      else if (is_rr (insn32, op_ar, &r1, &r2)
	       || is_rre (insn64, op_agr, &r1, &r2))
	data->gpr[r1] = pv_add (data->gpr[r1], data->gpr[r2]);

      /* A r1, d2(x2, b2) -- add.  */
      /* AY r1, d2(x2, b2) -- add (long-displacement version).  */
      /* AG r1, d2(x2, b2) -- add (64-bit version).  */
      else if (is_rx (insn32, op_a, &r1, &d2, &x2, &b2)
	       || is_rxy (insn32, op1_ay, op2_ay, &r1, &d2, &x2, &b2)
	       || is_rxy (insn64, op1_ag, op2_ag, &r1, &d2, &x2, &b2))
	data->gpr[r1] = pv_add (data->gpr[r1],
				s390_load (data, d2, x2, b2, data->gpr_size));

      /* SLFI r1, i2 --- subtract logical immediate.  */
      /* SLGFI r1, i2 --- subtract logical immediate (64-bit version).  */
      else if (is_ril (insn32, op1_slfi, op2_slfi, &r1, &i2)
	       || is_ril (insn64, op1_slgfi, op2_slgfi, &r1, &i2))
	data->gpr[r1] = pv_add_constant (data->gpr[r1],
					 -((CORE_ADDR)i2 & 0xffffffff));

      /* SR r1, r2 -- subtract register.  */
      /* SGR r1, r2 -- subtract register (64-bit version).  */
      else if (is_rr (insn32, op_sr, &r1, &r2)
	       || is_rre (insn64, op_sgr, &r1, &r2))
	data->gpr[r1] = pv_subtract (data->gpr[r1], data->gpr[r2]);

      /* S r1, d2(x2, b2) -- subtract.  */
      /* SY r1, d2(x2, b2) -- subtract (long-displacement version).  */
      /* SG r1, d2(x2, b2) -- subtract (64-bit version).  */
      else if (is_rx (insn32, op_s, &r1, &d2, &x2, &b2)
	       || is_rxy (insn32, op1_sy, op2_sy, &r1, &d2, &x2, &b2)
	       || is_rxy (insn64, op1_sg, op2_sg, &r1, &d2, &x2, &b2))
	data->gpr[r1] = pv_subtract (data->gpr[r1],
				s390_load (data, d2, x2, b2, data->gpr_size));

      /* LA r1, d2(x2, b2) --- load address.  */
      /* LAY r1, d2(x2, b2) --- load address (long-displacement version).  */
      else if (is_rx (insn, op_la, &r1, &d2, &x2, &b2)
	       || is_rxy (insn, op1_lay, op2_lay, &r1, &d2, &x2, &b2))
	data->gpr[r1] = s390_addr (data, d2, x2, b2);

      /* LARL r1, i2 --- load address relative long.  */
      else if (is_ril (insn, op1_larl, op2_larl, &r1, &i2))
	data->gpr[r1] = pv_constant (pc + i2 * 2);

      /* BASR r1, 0 --- branch and save.
	 Since r2 is zero, this saves the PC in r1, but doesn't branch.  */
      else if (is_rr (insn, op_basr, &r1, &r2)
	       && r2 == 0)
	data->gpr[r1] = pv_constant (next_pc);

      /* BRAS r1, i2 --- branch relative and save.  */
      else if (is_ri (insn, op1_bras, op2_bras, &r1, &i2))
	{
	  data->gpr[r1] = pv_constant (next_pc);
	  next_pc = pc + i2 * 2;

	  /* We'd better not interpret any backward branches.  We'll
	     never terminate.  */
	  if (next_pc <= pc)
	    break;
	}

      /* BRC/BRCL -- branch relative on condition.  Ignore "branch
	 never", branch to following instruction, and "conditional
	 trap" (BRC +2).  Otherwise terminate search.  */
      else if (is_ri (insn, op1_brc, op2_brc, &r1, &i2))
	{
	  if (r1 != 0 && i2 != 1 && i2 != 2)
	    break;
	}
      else if (is_ril (insn, op1_brcl, op2_brcl, &r1, &i2))
	{
	  if (r1 != 0 && i2 != 3)
	    break;
	}

      /* Terminate search when hitting any other branch instruction.  */
      else if (is_rr (insn, op_basr, &r1, &r2)
	       || is_rx (insn, op_bas, &r1, &d2, &x2, &b2)
	       || is_rr (insn, op_bcr, &r1, &r2)
	       || is_rx (insn, op_bc, &r1, &d2, &x2, &b2)
	       || is_ril (insn, op1_brasl, op2_brasl, &r2, &i2))
	break;

      else
	{
	  /* An instruction we don't know how to simulate.  The only
	     safe thing to do would be to set every value we're tracking
	     to 'unknown'.  Instead, we'll be optimistic: we assume that
	     we *can* interpret every instruction that the compiler uses
	     to manipulate any of the data we're interested in here --
	     then we can just ignore anything else.  */
	}

      /* Record the address after the last instruction that changed
	 the FP, SP, or backlink.  Ignore instructions that changed
	 them back to their original values --- those are probably
	 restore instructions.  (The back chain is never restored,
	 just popped.)  */
      {
	pv_t sp = data->gpr[S390_SP_REGNUM - S390_R0_REGNUM];
	pv_t fp = data->gpr[S390_FRAME_REGNUM - S390_R0_REGNUM];

	if ((! pv_is_identical (pre_insn_sp, sp)
	     && ! pv_is_register_k (sp, S390_SP_REGNUM, 0)
	     && sp.kind != pvk_unknown)
	    || (! pv_is_identical (pre_insn_fp, fp)
		&& ! pv_is_register_k (fp, S390_FRAME_REGNUM, 0)
		&& fp.kind != pvk_unknown)
	    || pre_insn_back_chain_saved_p != data->back_chain_saved_p)
	  result = next_pc;
      }
    }

  /* Record where all the registers were saved.  */
  data->stack->scan (s390_check_for_saved, data);

  return result;
}

/* Advance PC across any function entry prologue instructions to reach
   some "real" code.  */

static CORE_ADDR
s390_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  struct s390_prologue_data data;
  CORE_ADDR skip_pc, func_addr;

  if (find_pc_partial_function (pc, NULL, &func_addr, NULL))
    {
      CORE_ADDR post_prologue_pc
	= skip_prologue_using_sal (gdbarch, func_addr);
      if (post_prologue_pc != 0)
	return std::max (pc, post_prologue_pc);
    }

  skip_pc = s390_analyze_prologue (gdbarch, pc, (CORE_ADDR)-1, &data);
  return skip_pc ? skip_pc : pc;
}

/* Register handling.  */

/* ABI call-saved register information.  */

static int
s390_register_call_saved (struct gdbarch *gdbarch, int regnum)
{
  s390_gdbarch_tdep *tdep = gdbarch_tdep<s390_gdbarch_tdep> (gdbarch);

  switch (tdep->abi)
    {
    case ABI_LINUX_S390:
      if ((regnum >= S390_R6_REGNUM && regnum <= S390_R15_REGNUM)
	  || regnum == S390_F4_REGNUM || regnum == S390_F6_REGNUM
	  || regnum == S390_A0_REGNUM)
	return 1;

      break;

    case ABI_LINUX_ZSERIES:
      if ((regnum >= S390_R6_REGNUM && regnum <= S390_R15_REGNUM)
	  || (regnum >= S390_F8_REGNUM && regnum <= S390_F15_REGNUM)
	  || (regnum >= S390_A0_REGNUM && regnum <= S390_A1_REGNUM))
	return 1;

      break;
    }

  return 0;
}

/* The "guess_tracepoint_registers" gdbarch method.  */

static void
s390_guess_tracepoint_registers (struct gdbarch *gdbarch,
				 struct regcache *regcache,
				 CORE_ADDR addr)
{
  s390_gdbarch_tdep *tdep = gdbarch_tdep<s390_gdbarch_tdep> (gdbarch);
  int sz = register_size (gdbarch, S390_PSWA_REGNUM);
  gdb_byte *reg = (gdb_byte *) alloca (sz);
  ULONGEST pswm, pswa;

  /* Set PSWA from the location and a default PSWM (the only part we're
     unlikely to get right is the CC).  */
  if (tdep->abi == ABI_LINUX_S390)
    {
      /* 31-bit PSWA needs high bit set (it's very unlikely the target
	 was in 24-bit mode).  */
      pswa = addr | 0x80000000UL;
      pswm = 0x070d0000UL;
    }
  else
    {
      pswa = addr;
      pswm = 0x0705000180000000ULL;
    }

  store_unsigned_integer (reg, sz, gdbarch_byte_order (gdbarch), pswa);
  regcache->raw_supply (S390_PSWA_REGNUM, reg);

  store_unsigned_integer (reg, sz, gdbarch_byte_order (gdbarch), pswm);
  regcache->raw_supply (S390_PSWM_REGNUM, reg);
}

/* Return the name of register REGNO.  Return the empty string for
   registers that shouldn't be visible.  */

static const char *
s390_register_name (struct gdbarch *gdbarch, int regnum)
{
  if (regnum >= S390_V0_LOWER_REGNUM
      && regnum <= S390_V15_LOWER_REGNUM)
    return "";
  return tdesc_register_name (gdbarch, regnum);
}

/* DWARF Register Mapping.  */

static const short s390_dwarf_regmap[] =
{
  /* 0-15: General Purpose Registers.  */
  S390_R0_REGNUM, S390_R1_REGNUM, S390_R2_REGNUM, S390_R3_REGNUM,
  S390_R4_REGNUM, S390_R5_REGNUM, S390_R6_REGNUM, S390_R7_REGNUM,
  S390_R8_REGNUM, S390_R9_REGNUM, S390_R10_REGNUM, S390_R11_REGNUM,
  S390_R12_REGNUM, S390_R13_REGNUM, S390_R14_REGNUM, S390_R15_REGNUM,

  /* 16-31: Floating Point Registers / Vector Registers 0-15. */
  S390_F0_REGNUM, S390_F2_REGNUM, S390_F4_REGNUM, S390_F6_REGNUM,
  S390_F1_REGNUM, S390_F3_REGNUM, S390_F5_REGNUM, S390_F7_REGNUM,
  S390_F8_REGNUM, S390_F10_REGNUM, S390_F12_REGNUM, S390_F14_REGNUM,
  S390_F9_REGNUM, S390_F11_REGNUM, S390_F13_REGNUM, S390_F15_REGNUM,

  /* 32-47: Control Registers (not mapped).  */
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,

  /* 48-63: Access Registers.  */
  S390_A0_REGNUM, S390_A1_REGNUM, S390_A2_REGNUM, S390_A3_REGNUM,
  S390_A4_REGNUM, S390_A5_REGNUM, S390_A6_REGNUM, S390_A7_REGNUM,
  S390_A8_REGNUM, S390_A9_REGNUM, S390_A10_REGNUM, S390_A11_REGNUM,
  S390_A12_REGNUM, S390_A13_REGNUM, S390_A14_REGNUM, S390_A15_REGNUM,

  /* 64-65: Program Status Word.  */
  S390_PSWM_REGNUM,
  S390_PSWA_REGNUM,

  /* 66-67: Reserved.  */
  -1, -1,

  /* 68-83: Vector Registers 16-31.  */
  S390_V16_REGNUM, S390_V18_REGNUM, S390_V20_REGNUM, S390_V22_REGNUM,
  S390_V17_REGNUM, S390_V19_REGNUM, S390_V21_REGNUM, S390_V23_REGNUM,
  S390_V24_REGNUM, S390_V26_REGNUM, S390_V28_REGNUM, S390_V30_REGNUM,
  S390_V25_REGNUM, S390_V27_REGNUM, S390_V29_REGNUM, S390_V31_REGNUM,

  /* End of "official" DWARF registers.  The remainder of the map is
     for GDB internal use only.  */

  /* GPR Lower Half Access.  */
  S390_R0_REGNUM, S390_R1_REGNUM, S390_R2_REGNUM, S390_R3_REGNUM,
  S390_R4_REGNUM, S390_R5_REGNUM, S390_R6_REGNUM, S390_R7_REGNUM,
  S390_R8_REGNUM, S390_R9_REGNUM, S390_R10_REGNUM, S390_R11_REGNUM,
  S390_R12_REGNUM, S390_R13_REGNUM, S390_R14_REGNUM, S390_R15_REGNUM,
};

enum { s390_dwarf_reg_r0l = ARRAY_SIZE (s390_dwarf_regmap) - 16 };

/* Convert DWARF register number REG to the appropriate register
   number used by GDB.  */

static int
s390_dwarf_reg_to_regnum (struct gdbarch *gdbarch, int reg)
{
  s390_gdbarch_tdep *tdep = gdbarch_tdep<s390_gdbarch_tdep> (gdbarch);
  int gdb_reg = -1;

  /* In a 32-on-64 debug scenario, debug info refers to the full
     64-bit GPRs.  Note that call frame information still refers to
     the 32-bit lower halves, because s390_adjust_frame_regnum uses
     special register numbers to access GPRs.  */
  if (tdep->gpr_full_regnum != -1 && reg >= 0 && reg < 16)
    return tdep->gpr_full_regnum + reg;

  if (reg >= 0 && reg < ARRAY_SIZE (s390_dwarf_regmap))
    gdb_reg = s390_dwarf_regmap[reg];

  if (tdep->v0_full_regnum == -1)
    {
      if (gdb_reg >= S390_V16_REGNUM && gdb_reg <= S390_V31_REGNUM)
	gdb_reg = -1;
    }
  else
    {
      if (gdb_reg >= S390_F0_REGNUM && gdb_reg <= S390_F15_REGNUM)
	gdb_reg = gdb_reg - S390_F0_REGNUM + tdep->v0_full_regnum;
    }

  return gdb_reg;
}

/* Pseudo registers.  */

/* Check whether REGNUM indicates a coupled general purpose register.
   These pseudo-registers are composed of two adjacent gprs.  */

static int
regnum_is_gpr_full (s390_gdbarch_tdep *tdep, int regnum)
{
  return (tdep->gpr_full_regnum != -1
	  && regnum >= tdep->gpr_full_regnum
	  && regnum <= tdep->gpr_full_regnum + 15);
}

/* Check whether REGNUM indicates a full vector register (v0-v15).
   These pseudo-registers are composed of f0-f15 and v0l-v15l.  */

static int
regnum_is_vxr_full (s390_gdbarch_tdep *tdep, int regnum)
{
  return (tdep->v0_full_regnum != -1
	  && regnum >= tdep->v0_full_regnum
	  && regnum <= tdep->v0_full_regnum + 15);
}

/* 'float' values are stored in the upper half of floating-point
   registers, even though we are otherwise a big-endian platform.  The
   same applies to a 'float' value within a vector.  */

static value *
s390_value_from_register (gdbarch *gdbarch, type *type, int regnum,
			  const frame_info_ptr &this_frame)
{
  s390_gdbarch_tdep *tdep = gdbarch_tdep<s390_gdbarch_tdep> (gdbarch);
  value *value
    = default_value_from_register (gdbarch, type, regnum, this_frame);
  check_typedef (type);

  if ((regnum >= S390_F0_REGNUM && regnum <= S390_F15_REGNUM
       && type->length () < 8)
      || regnum_is_vxr_full (tdep, regnum)
      || (regnum >= S390_V16_REGNUM && regnum <= S390_V31_REGNUM))
    value->set_offset (0);

  return value;
}

/* Implement pseudo_register_name tdesc method.  */

static const char *
s390_pseudo_register_name (struct gdbarch *gdbarch, int regnum)
{
  s390_gdbarch_tdep *tdep = gdbarch_tdep<s390_gdbarch_tdep> (gdbarch);

  if (regnum == tdep->pc_regnum)
    return "pc";

  if (regnum == tdep->cc_regnum)
    return "cc";

  if (regnum_is_gpr_full (tdep, regnum))
    {
      static const char *full_name[] = {
	"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
	"r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
      };
      return full_name[regnum - tdep->gpr_full_regnum];
    }

  if (regnum_is_vxr_full (tdep, regnum))
    {
      static const char *full_name[] = {
	"v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7",
	"v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15"
      };
      return full_name[regnum - tdep->v0_full_regnum];
    }

  internal_error (_("invalid regnum"));
}

/* Implement pseudo_register_type tdesc method.  */

static struct type *
s390_pseudo_register_type (struct gdbarch *gdbarch, int regnum)
{
  s390_gdbarch_tdep *tdep = gdbarch_tdep<s390_gdbarch_tdep> (gdbarch);

  if (regnum == tdep->pc_regnum)
    return builtin_type (gdbarch)->builtin_func_ptr;

  if (regnum == tdep->cc_regnum)
    return builtin_type (gdbarch)->builtin_int;

  if (regnum_is_gpr_full (tdep, regnum))
    return builtin_type (gdbarch)->builtin_uint64;

  /* For the "concatenated" vector registers use the same type as v16.  */
  if (regnum_is_vxr_full (tdep, regnum))
    return tdesc_register_type (gdbarch, S390_V16_REGNUM);

  internal_error (_("invalid regnum"));
}

/* Implement pseudo_register_read gdbarch method.  */

static enum register_status
s390_pseudo_register_read (struct gdbarch *gdbarch, readable_regcache *regcache,
			   int regnum, gdb_byte *buf)
{
  s390_gdbarch_tdep *tdep = gdbarch_tdep<s390_gdbarch_tdep> (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int regsize = register_size (gdbarch, regnum);
  ULONGEST val;

  if (regnum == tdep->pc_regnum)
    {
      enum register_status status;

      status = regcache->raw_read (S390_PSWA_REGNUM, &val);
      if (status == REG_VALID)
	{
	  if (register_size (gdbarch, S390_PSWA_REGNUM) == 4)
	    val &= 0x7fffffff;
	  store_unsigned_integer (buf, regsize, byte_order, val);
	}
      return status;
    }

  if (regnum == tdep->cc_regnum)
    {
      enum register_status status;

      status = regcache->raw_read (S390_PSWM_REGNUM, &val);
      if (status == REG_VALID)
	{
	  if (register_size (gdbarch, S390_PSWA_REGNUM) == 4)
	    val = (val >> 12) & 3;
	  else
	    val = (val >> 44) & 3;
	  store_unsigned_integer (buf, regsize, byte_order, val);
	}
      return status;
    }

  if (regnum_is_gpr_full (tdep, regnum))
    {
      enum register_status status;
      ULONGEST val_upper;

      regnum -= tdep->gpr_full_regnum;

      status = regcache->raw_read (S390_R0_REGNUM + regnum, &val);
      if (status == REG_VALID)
	status = regcache->raw_read (S390_R0_UPPER_REGNUM + regnum,
				     &val_upper);
      if (status == REG_VALID)
	{
	  val |= val_upper << 32;
	  store_unsigned_integer (buf, regsize, byte_order, val);
	}
      return status;
    }

  if (regnum_is_vxr_full (tdep, regnum))
    {
      enum register_status status;

      regnum -= tdep->v0_full_regnum;

      status = regcache->raw_read (S390_F0_REGNUM + regnum, buf);
      if (status == REG_VALID)
	status = regcache->raw_read (S390_V0_LOWER_REGNUM + regnum, buf + 8);
      return status;
    }

  internal_error (_("invalid regnum"));
}

/* Implement pseudo_register_write gdbarch method.  */

static void
s390_pseudo_register_write (struct gdbarch *gdbarch, struct regcache *regcache,
			    int regnum, const gdb_byte *buf)
{
  s390_gdbarch_tdep *tdep = gdbarch_tdep<s390_gdbarch_tdep> (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int regsize = register_size (gdbarch, regnum);
  ULONGEST val, psw;

  if (regnum == tdep->pc_regnum)
    {
      val = extract_unsigned_integer (buf, regsize, byte_order);
      if (register_size (gdbarch, S390_PSWA_REGNUM) == 4)
	{
	  regcache_raw_read_unsigned (regcache, S390_PSWA_REGNUM, &psw);
	  val = (psw & 0x80000000) | (val & 0x7fffffff);
	}
      regcache_raw_write_unsigned (regcache, S390_PSWA_REGNUM, val);
      return;
    }

  if (regnum == tdep->cc_regnum)
    {
      val = extract_unsigned_integer (buf, regsize, byte_order);
      regcache_raw_read_unsigned (regcache, S390_PSWM_REGNUM, &psw);
      if (register_size (gdbarch, S390_PSWA_REGNUM) == 4)
	val = (psw & ~((ULONGEST)3 << 12)) | ((val & 3) << 12);
      else
	val = (psw & ~((ULONGEST)3 << 44)) | ((val & 3) << 44);
      regcache_raw_write_unsigned (regcache, S390_PSWM_REGNUM, val);
      return;
    }

  if (regnum_is_gpr_full (tdep, regnum))
    {
      regnum -= tdep->gpr_full_regnum;
      val = extract_unsigned_integer (buf, regsize, byte_order);
      regcache_raw_write_unsigned (regcache, S390_R0_REGNUM + regnum,
				   val & 0xffffffff);
      regcache_raw_write_unsigned (regcache, S390_R0_UPPER_REGNUM + regnum,
				   val >> 32);
      return;
    }

  if (regnum_is_vxr_full (tdep, regnum))
    {
      regnum -= tdep->v0_full_regnum;
      regcache->raw_write (S390_F0_REGNUM + regnum, buf);
      regcache->raw_write (S390_V0_LOWER_REGNUM + regnum, buf + 8);
      return;
    }

  internal_error (_("invalid regnum"));
}

/* Register groups.  */

/* Implement pseudo_register_reggroup_p tdesc method.  */

static int
s390_pseudo_register_reggroup_p (struct gdbarch *gdbarch, int regnum,
				 const struct reggroup *group)
{
  s390_gdbarch_tdep *tdep = gdbarch_tdep<s390_gdbarch_tdep> (gdbarch);

  /* We usually save/restore the whole PSW, which includes PC and CC.
     However, some older gdbservers may not support saving/restoring
     the whole PSW yet, and will return an XML register description
     excluding those from the save/restore register groups.  In those
     cases, we still need to explicitly save/restore PC and CC in order
     to push or pop frames.  Since this doesn't hurt anything if we
     already save/restore the whole PSW (it's just redundant), we add
     PC and CC at this point unconditionally.  */
  if (group == save_reggroup || group == restore_reggroup)
    return regnum == tdep->pc_regnum || regnum == tdep->cc_regnum;

  if (group == vector_reggroup)
    return regnum_is_vxr_full (tdep, regnum);

  if (group == general_reggroup && regnum_is_vxr_full (tdep, regnum))
    return 0;

  return default_register_reggroup_p (gdbarch, regnum, group);
}

/* The "ax_pseudo_register_collect" gdbarch method.  */

static int
s390_ax_pseudo_register_collect (struct gdbarch *gdbarch,
				 struct agent_expr *ax, int regnum)
{
  s390_gdbarch_tdep *tdep = gdbarch_tdep<s390_gdbarch_tdep> (gdbarch);
  if (regnum == tdep->pc_regnum)
    {
      ax_reg_mask (ax, S390_PSWA_REGNUM);
    }
  else if (regnum == tdep->cc_regnum)
    {
      ax_reg_mask (ax, S390_PSWM_REGNUM);
    }
  else if (regnum_is_gpr_full (tdep, regnum))
    {
      regnum -= tdep->gpr_full_regnum;
      ax_reg_mask (ax, S390_R0_REGNUM + regnum);
      ax_reg_mask (ax, S390_R0_UPPER_REGNUM + regnum);
    }
  else if (regnum_is_vxr_full (tdep, regnum))
    {
      regnum -= tdep->v0_full_regnum;
      ax_reg_mask (ax, S390_F0_REGNUM + regnum);
      ax_reg_mask (ax, S390_V0_LOWER_REGNUM + regnum);
    }
  else
    {
      internal_error (_("invalid regnum"));
    }
  return 0;
}

/* The "ax_pseudo_register_push_stack" gdbarch method.  */

static int
s390_ax_pseudo_register_push_stack (struct gdbarch *gdbarch,
				    struct agent_expr *ax, int regnum)
{
  s390_gdbarch_tdep *tdep = gdbarch_tdep<s390_gdbarch_tdep> (gdbarch);
  if (regnum == tdep->pc_regnum)
    {
      ax_reg (ax, S390_PSWA_REGNUM);
      if (register_size (gdbarch, S390_PSWA_REGNUM) == 4)
	{
	  ax_zero_ext (ax, 31);
	}
    }
  else if (regnum == tdep->cc_regnum)
    {
      ax_reg (ax, S390_PSWM_REGNUM);
      if (register_size (gdbarch, S390_PSWA_REGNUM) == 4)
	ax_const_l (ax, 12);
      else
	ax_const_l (ax, 44);
      ax_simple (ax, aop_rsh_unsigned);
      ax_zero_ext (ax, 2);
    }
  else if (regnum_is_gpr_full (tdep, regnum))
    {
      regnum -= tdep->gpr_full_regnum;
      ax_reg (ax, S390_R0_REGNUM + regnum);
      ax_reg (ax, S390_R0_UPPER_REGNUM + regnum);
      ax_const_l (ax, 32);
      ax_simple (ax, aop_lsh);
      ax_simple (ax, aop_bit_or);
    }
  else if (regnum_is_vxr_full (tdep, regnum))
    {
      /* Too large to stuff on the stack.  */
      return 1;
    }
  else
    {
      internal_error (_("invalid regnum"));
    }
  return 0;
}

/* The "gen_return_address" gdbarch method.  Since this is supposed to be
   just a best-effort method, and we don't really have the means to run
   the full unwinder here, just collect the link register.  */

static void
s390_gen_return_address (struct gdbarch *gdbarch,
			 struct agent_expr *ax, struct axs_value *value,
			 CORE_ADDR scope)
{
  value->type = register_type (gdbarch, S390_R14_REGNUM);
  value->kind = axs_lvalue_register;
  value->u.reg = S390_R14_REGNUM;
}

/* Address handling.  */

/* Implement addr_bits_remove gdbarch method.
   Only used for ABI_LINUX_S390.  */

static CORE_ADDR
s390_addr_bits_remove (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  return addr & 0x7fffffff;
}

/* Implement addr_class_type_flags gdbarch method.
   Only used for ABI_LINUX_ZSERIES.  */

static type_instance_flags
s390_address_class_type_flags (int byte_size, int dwarf2_addr_class)
{
  if (byte_size == 4)
    return TYPE_INSTANCE_FLAG_ADDRESS_CLASS_1;
  else
    return 0;
}

/* Implement addr_class_type_flags_to_name gdbarch method.
   Only used for ABI_LINUX_ZSERIES.  */

static const char *
s390_address_class_type_flags_to_name (struct gdbarch *gdbarch,
				       type_instance_flags type_flags)
{
  if (type_flags & TYPE_INSTANCE_FLAG_ADDRESS_CLASS_1)
    return "mode32";
  else
    return NULL;
}

/* Implement addr_class_name_to_type_flags gdbarch method.
   Only used for ABI_LINUX_ZSERIES.  */

static bool
s390_address_class_name_to_type_flags (struct gdbarch *gdbarch,
				       const char *name,
				       type_instance_flags *type_flags_ptr)
{
  if (strcmp (name, "mode32") == 0)
    {
      *type_flags_ptr = TYPE_INSTANCE_FLAG_ADDRESS_CLASS_1;
      return true;
    }
  else
    return false;
}

/* Inferior function calls.  */

/* Dummy function calls.  */

/* Unwrap any single-field structs in TYPE and return the effective
   "inner" type.  E.g., yield "float" for all these cases:

     float x;
     struct { float x };
     struct { struct { float x; } x; };
     struct { struct { struct { float x; } x; } x; };

   However, if an inner type is smaller than MIN_SIZE, abort the
   unwrapping.  */

static struct type *
s390_effective_inner_type (struct type *type, unsigned int min_size)
{
  while (type->code () == TYPE_CODE_STRUCT)
    {
      struct type *inner = NULL;

      /* Find a non-static field, if any.  Unless there's exactly one,
	 abort the unwrapping.  */
      for (int i = 0; i < type->num_fields (); i++)
	{
	  struct field f = type->field (i);

	  if (f.is_static ())
	    continue;
	  if (inner != NULL)
	    return type;
	  inner = f.type ();
	}

      if (inner == NULL)
	break;
      inner = check_typedef (inner);
      if (inner->length () < min_size)
	break;
      type = inner;
    }

  return type;
}

/* Return non-zero if TYPE should be passed like "float" or
   "double".  */

static int
s390_function_arg_float (struct type *type)
{
  /* Note that long double as well as complex types are intentionally
     excluded. */
  if (type->length () > 8)
    return 0;

  /* A struct containing just a float or double is passed like a float
     or double.  */
  type = s390_effective_inner_type (type, 0);

  return (type->code () == TYPE_CODE_FLT
	  || type->code () == TYPE_CODE_DECFLOAT);
}

/* Return non-zero if TYPE should be passed like a vector.  */

static int
s390_function_arg_vector (struct type *type)
{
  if (type->length () > 16)
    return 0;

  /* Structs containing just a vector are passed like a vector.  */
  type = s390_effective_inner_type (type, type->length ());

  return type->code () == TYPE_CODE_ARRAY && type->is_vector ();
}

/* Determine whether N is a power of two.  */

static int
is_power_of_two (unsigned int n)
{
  return n && ((n & (n - 1)) == 0);
}

/* For an argument whose type is TYPE and which is not passed like a
   float or vector, return non-zero if it should be passed like "int"
   or "long long".  */

static int
s390_function_arg_integer (struct type *type)
{
  enum type_code code = type->code ();

  if (type->length () > 8)
    return 0;

  if (code == TYPE_CODE_INT
      || code == TYPE_CODE_ENUM
      || code == TYPE_CODE_RANGE
      || code == TYPE_CODE_CHAR
      || code == TYPE_CODE_BOOL
      || code == TYPE_CODE_PTR
      || TYPE_IS_REFERENCE (type))
    return 1;

  return ((code == TYPE_CODE_UNION || code == TYPE_CODE_STRUCT)
	  && is_power_of_two (type->length ()));
}

/* Argument passing state: Internal data structure passed to helper
   routines of s390_push_dummy_call.  */

struct s390_arg_state
  {
    /* Register cache, or NULL, if we are in "preparation mode".  */
    struct regcache *regcache;
    /* Next available general/floating-point/vector register for
       argument passing.  */
    int gr, fr, vr;
    /* Current pointer to copy area (grows downwards).  */
    CORE_ADDR copy;
    /* Current pointer to parameter area (grows upwards).  */
    CORE_ADDR argp;
  };

/* Prepare one argument ARG for a dummy call and update the argument
   passing state AS accordingly.  If the regcache field in AS is set,
   operate in "write mode" and write ARG into the inferior.  Otherwise
   run "preparation mode" and skip all updates to the inferior.  */

static void
s390_handle_arg (struct s390_arg_state *as, struct value *arg,
		 s390_gdbarch_tdep *tdep, int word_size,
		 enum bfd_endian byte_order, int is_unnamed)
{
  struct type *type = check_typedef (arg->type ());
  unsigned int length = type->length ();
  int write_mode = as->regcache != NULL;

  if (s390_function_arg_float (type))
    {
      /* The GNU/Linux for S/390 ABI uses FPRs 0 and 2 to pass
	 arguments.  The GNU/Linux for zSeries ABI uses 0, 2, 4, and
	 6.  */
      if (as->fr <= (tdep->abi == ABI_LINUX_S390 ? 2 : 6))
	{
	  /* When we store a single-precision value in an FP register,
	     it occupies the leftmost bits.  */
	  if (write_mode)
	    as->regcache->cooked_write_part (S390_F0_REGNUM + as->fr, 0, length,
					     arg->contents ().data ());
	  as->fr += 2;
	}
      else
	{
	  /* When we store a single-precision value in a stack slot,
	     it occupies the rightmost bits.  */
	  as->argp = align_up (as->argp + length, word_size);
	  if (write_mode)
	    write_memory (as->argp - length, arg->contents ().data (),
			  length);
	}
    }
  else if (tdep->vector_abi == S390_VECTOR_ABI_128
	   && s390_function_arg_vector (type))
    {
      static const char use_vr[] = {24, 26, 28, 30, 25, 27, 29, 31};

      if (!is_unnamed && as->vr < ARRAY_SIZE (use_vr))
	{
	  int regnum = S390_V24_REGNUM + use_vr[as->vr] - 24;

	  if (write_mode)
	    as->regcache->cooked_write_part (regnum, 0, length,
					     arg->contents ().data ());
	  as->vr++;
	}
      else
	{
	  if (write_mode)
	    write_memory (as->argp, arg->contents ().data (), length);
	  as->argp = align_up (as->argp + length, word_size);
	}
    }
  else if (s390_function_arg_integer (type) && length <= word_size)
    {
      /* Initialize it just to avoid a GCC false warning.  */
      ULONGEST val = 0;

      if (write_mode)
	{
	  /* Place value in least significant bits of the register or
	     memory word and sign- or zero-extend to full word size.
	     This also applies to a struct or union.  */
	  val = type->is_unsigned ()
	    ? extract_unsigned_integer (arg->contents ().data (),
					length, byte_order)
	    : extract_signed_integer (arg->contents ().data (),
				      length, byte_order);
	}

      if (as->gr <= 6)
	{
	  if (write_mode)
	    regcache_cooked_write_unsigned (as->regcache,
					    S390_R0_REGNUM + as->gr,
					    val);
	  as->gr++;
	}
      else
	{
	  if (write_mode)
	    write_memory_unsigned_integer (as->argp, word_size,
					   byte_order, val);
	  as->argp += word_size;
	}
    }
  else if (s390_function_arg_integer (type) && length == 8)
    {
      if (as->gr <= 5)
	{
	  if (write_mode)
	    {
	      as->regcache->cooked_write (S390_R0_REGNUM + as->gr,
					  arg->contents ().data ());
	      as->regcache->cooked_write
		(S390_R0_REGNUM + as->gr + 1,
		 arg->contents ().data () + word_size);
	    }
	  as->gr += 2;
	}
      else
	{
	  /* If we skipped r6 because we couldn't fit a DOUBLE_ARG
	     in it, then don't go back and use it again later.  */
	  as->gr = 7;

	  if (write_mode)
	    write_memory (as->argp, arg->contents ().data (), length);
	  as->argp += length;
	}
    }
  else
    {
      /* This argument type is never passed in registers.  Place the
	 value in the copy area and pass a pointer to it.  Use 8-byte
	 alignment as a conservative assumption.  */
      as->copy = align_down (as->copy - length, 8);
      if (write_mode)
	write_memory (as->copy, arg->contents ().data (), length);

      if (as->gr <= 6)
	{
	  if (write_mode)
	    regcache_cooked_write_unsigned (as->regcache,
					    S390_R0_REGNUM + as->gr,
					    as->copy);
	  as->gr++;
	}
      else
	{
	  if (write_mode)
	    write_memory_unsigned_integer (as->argp, word_size,
					   byte_order, as->copy);
	  as->argp += word_size;
	}
    }
}

/* Put the actual parameter values pointed to by ARGS[0..NARGS-1] in
   place to be passed to a function, as specified by the "GNU/Linux
   for S/390 ELF Application Binary Interface Supplement".

   SP is the current stack pointer.  We must put arguments, links,
   padding, etc. whereever they belong, and return the new stack
   pointer value.

   If STRUCT_RETURN is non-zero, then the function we're calling is
   going to return a structure by value; STRUCT_ADDR is the address of
   a block we've allocated for it on the stack.

   Our caller has taken care of any type promotions needed to satisfy
   prototypes or the old K&R argument-passing rules.  */

static CORE_ADDR
s390_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
		      struct regcache *regcache, CORE_ADDR bp_addr,
		      int nargs, struct value **args, CORE_ADDR sp,
		      function_call_return_method return_method,
		      CORE_ADDR struct_addr)
{
  s390_gdbarch_tdep *tdep = gdbarch_tdep<s390_gdbarch_tdep> (gdbarch);
  int word_size = gdbarch_ptr_bit (gdbarch) / 8;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int i;
  struct s390_arg_state arg_state, arg_prep;
  CORE_ADDR param_area_start, new_sp;
  struct type *ftype = check_typedef (function->type ());

  if (ftype->code () == TYPE_CODE_PTR)
    ftype = check_typedef (ftype->target_type ());

  arg_prep.copy = sp;
  arg_prep.gr = (return_method == return_method_struct) ? 3 : 2;
  arg_prep.fr = 0;
  arg_prep.vr = 0;
  arg_prep.argp = 0;
  arg_prep.regcache = NULL;

  /* Initialize arg_state for "preparation mode".  */
  arg_state = arg_prep;

  /* Update arg_state.copy with the start of the reference-to-copy area
     and arg_state.argp with the size of the parameter area.  */
  for (i = 0; i < nargs; i++)
    s390_handle_arg (&arg_state, args[i], tdep, word_size, byte_order,
		     ftype->has_varargs () && i >= ftype->num_fields ());

  param_area_start = align_down (arg_state.copy - arg_state.argp, 8);

  /* Allocate the standard frame areas: the register save area, the
     word reserved for the compiler, and the back chain pointer.  */
  new_sp = param_area_start - (16 * word_size + 32);

  /* Now we have the final stack pointer.  Make sure we didn't
     underflow; on 31-bit, this would result in addresses with the
     high bit set, which causes confusion elsewhere.  Note that if we
     error out here, stack and registers remain untouched.  */
  if (gdbarch_addr_bits_remove (gdbarch, new_sp) != new_sp)
    error (_("Stack overflow"));

  /* Pass the structure return address in general register 2.  */
  if (return_method == return_method_struct)
    regcache_cooked_write_unsigned (regcache, S390_R2_REGNUM, struct_addr);

  /* Initialize arg_state for "write mode".  */
  arg_state = arg_prep;
  arg_state.argp = param_area_start;
  arg_state.regcache = regcache;

  /* Write all parameters.  */
  for (i = 0; i < nargs; i++)
    s390_handle_arg (&arg_state, args[i], tdep, word_size, byte_order,
		     ftype->has_varargs () && i >= ftype->num_fields ());

  /* Store return PSWA.  In 31-bit mode, keep addressing mode bit.  */
  if (word_size == 4)
    {
      ULONGEST pswa;
      regcache_cooked_read_unsigned (regcache, S390_PSWA_REGNUM, &pswa);
      bp_addr = (bp_addr & 0x7fffffff) | (pswa & 0x80000000);
    }
  regcache_cooked_write_unsigned (regcache, S390_RETADDR_REGNUM, bp_addr);

  /* Store updated stack pointer.  */
  regcache_cooked_write_unsigned (regcache, S390_SP_REGNUM, new_sp);

  /* We need to return the 'stack part' of the frame ID,
     which is actually the top of the register save area.  */
  return param_area_start;
}

/* Assuming THIS_FRAME is a dummy, return the frame ID of that
   dummy frame.  The frame ID's base needs to match the TOS value
   returned by push_dummy_call, and the PC match the dummy frame's
   breakpoint.  */

static struct frame_id
s390_dummy_id (struct gdbarch *gdbarch, frame_info_ptr this_frame)
{
  int word_size = gdbarch_ptr_bit (gdbarch) / 8;
  CORE_ADDR sp = get_frame_register_unsigned (this_frame, S390_SP_REGNUM);
  sp = gdbarch_addr_bits_remove (gdbarch, sp);

  return frame_id_build (sp + 16*word_size + 32,
			 get_frame_pc (this_frame));
}

/* Implement frame_align gdbarch method.  */

static CORE_ADDR
s390_frame_align (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  /* Both the 32- and 64-bit ABI's say that the stack pointer should
     always be aligned on an eight-byte boundary.  */
  return (addr & -8);
}

/* Helper for s390_return_value: Set or retrieve a function return
   value if it resides in a register.  */

static void
s390_register_return_value (struct gdbarch *gdbarch, struct type *type,
			    struct regcache *regcache,
			    gdb_byte *out, const gdb_byte *in)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int word_size = gdbarch_ptr_bit (gdbarch) / 8;
  int length = type->length ();
  int code = type->code ();

  if (code == TYPE_CODE_FLT || code == TYPE_CODE_DECFLOAT)
    {
      /* Float-like value: left-aligned in f0.  */
      if (in != NULL)
	regcache->cooked_write_part (S390_F0_REGNUM, 0, length, in);
      else
	regcache->cooked_read_part (S390_F0_REGNUM, 0, length, out);
    }
  else if (code == TYPE_CODE_ARRAY)
    {
      /* Vector: left-aligned in v24.  */
      if (in != NULL)
	regcache->cooked_write_part (S390_V24_REGNUM, 0, length, in);
      else
	regcache->cooked_read_part (S390_V24_REGNUM, 0, length, out);
    }
  else if (length <= word_size)
    {
      /* Integer: zero- or sign-extended in r2.  */
      if (out != NULL)
	regcache->cooked_read_part (S390_R2_REGNUM, word_size - length, length,
				    out);
      else if (type->is_unsigned ())
	regcache_cooked_write_unsigned
	  (regcache, S390_R2_REGNUM,
	   extract_unsigned_integer (in, length, byte_order));
      else
	regcache_cooked_write_signed
	  (regcache, S390_R2_REGNUM,
	   extract_signed_integer (in, length, byte_order));
    }
  else if (length == 2 * word_size)
    {
      /* Double word: in r2 and r3.  */
      if (in != NULL)
	{
	  regcache->cooked_write (S390_R2_REGNUM, in);
	  regcache->cooked_write (S390_R3_REGNUM, in + word_size);
	}
      else
	{
	  regcache->cooked_read (S390_R2_REGNUM, out);
	  regcache->cooked_read (S390_R3_REGNUM, out + word_size);
	}
    }
  else
    internal_error (_("invalid return type"));
}

/* Implement the 'return_value' gdbarch method.  */

static enum return_value_convention
s390_return_value (struct gdbarch *gdbarch, struct value *function,
		   struct type *type, struct regcache *regcache,
		   gdb_byte *out, const gdb_byte *in)
{
  enum return_value_convention rvc;

  type = check_typedef (type);

  switch (type->code ())
    {
    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
    case TYPE_CODE_COMPLEX:
      rvc = RETURN_VALUE_STRUCT_CONVENTION;
      break;
    case TYPE_CODE_ARRAY:
      {
	s390_gdbarch_tdep *tdep = gdbarch_tdep<s390_gdbarch_tdep> (gdbarch);
	rvc = (tdep->vector_abi == S390_VECTOR_ABI_128
	       && type->length () <= 16 && type->is_vector ())
	  ? RETURN_VALUE_REGISTER_CONVENTION
	  : RETURN_VALUE_STRUCT_CONVENTION;
	break;
      }
    default:
      rvc = type->length () <= 8
	? RETURN_VALUE_REGISTER_CONVENTION
	: RETURN_VALUE_STRUCT_CONVENTION;
    }

  if (in != NULL || out != NULL)
    {
      if (rvc == RETURN_VALUE_REGISTER_CONVENTION)
	s390_register_return_value (gdbarch, type, regcache, out, in);
      else if (in != NULL)
	error (_("Cannot set function return value."));
      else
	error (_("Function return value unknown."));
    }

  return rvc;
}

/* Frame unwinding.  */

/* Implement the stack_frame_destroyed_p gdbarch method.  */

static int
s390_stack_frame_destroyed_p (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  int word_size = gdbarch_ptr_bit (gdbarch) / 8;

  /* In frameless functions, there's no frame to destroy and thus
     we don't care about the epilogue.

     In functions with frame, the epilogue sequence is a pair of
     a LM-type instruction that restores (amongst others) the
     return register %r14 and the stack pointer %r15, followed
     by a branch 'br %r14' --or equivalent-- that effects the
     actual return.

     In that situation, this function needs to return 'true' in
     exactly one case: when pc points to that branch instruction.

     Thus we try to disassemble the one instructions immediately
     preceding pc and check whether it is an LM-type instruction
     modifying the stack pointer.

     Note that disassembling backwards is not reliable, so there
     is a slight chance of false positives here ...  */

  bfd_byte insn[6];
  unsigned int r1, r3, b2;
  int d2;

  if (word_size == 4
      && !target_read_memory (pc - 4, insn, 4)
      && is_rs (insn, op_lm, &r1, &r3, &d2, &b2)
      && r3 == S390_SP_REGNUM - S390_R0_REGNUM)
    return 1;

  if (word_size == 4
      && !target_read_memory (pc - 6, insn, 6)
      && is_rsy (insn, op1_lmy, op2_lmy, &r1, &r3, &d2, &b2)
      && r3 == S390_SP_REGNUM - S390_R0_REGNUM)
    return 1;

  if (word_size == 8
      && !target_read_memory (pc - 6, insn, 6)
      && is_rsy (insn, op1_lmg, op2_lmg, &r1, &r3, &d2, &b2)
      && r3 == S390_SP_REGNUM - S390_R0_REGNUM)
    return 1;

  return 0;
}

/* Implement unwind_pc gdbarch method.  */

static CORE_ADDR
s390_unwind_pc (struct gdbarch *gdbarch, frame_info_ptr next_frame)
{
  s390_gdbarch_tdep *tdep = gdbarch_tdep<s390_gdbarch_tdep> (gdbarch);
  ULONGEST pc;
  pc = frame_unwind_register_unsigned (next_frame, tdep->pc_regnum);
  return gdbarch_addr_bits_remove (gdbarch, pc);
}

/* Implement unwind_sp gdbarch method.  */

static CORE_ADDR
s390_unwind_sp (struct gdbarch *gdbarch, frame_info_ptr next_frame)
{
  ULONGEST sp;
  sp = frame_unwind_register_unsigned (next_frame, S390_SP_REGNUM);
  return gdbarch_addr_bits_remove (gdbarch, sp);
}

/* Helper routine to unwind pseudo registers.  */

static struct value *
s390_unwind_pseudo_register (frame_info_ptr this_frame, int regnum)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  s390_gdbarch_tdep *tdep = gdbarch_tdep<s390_gdbarch_tdep> (gdbarch);
  struct type *type = register_type (gdbarch, regnum);

  /* Unwind PC via PSW address.  */
  if (regnum == tdep->pc_regnum)
    {
      struct value *val;

      val = frame_unwind_register_value (this_frame, S390_PSWA_REGNUM);
      if (!val->optimized_out ())
	{
	  LONGEST pswa = value_as_long (val);

	  if (type->length () == 4)
	    return value_from_pointer (type, pswa & 0x7fffffff);
	  else
	    return value_from_pointer (type, pswa);
	}
    }

  /* Unwind CC via PSW mask.  */
  if (regnum == tdep->cc_regnum)
    {
      struct value *val;

      val = frame_unwind_register_value (this_frame, S390_PSWM_REGNUM);
      if (!val->optimized_out ())
	{
	  LONGEST pswm = value_as_long (val);

	  if (type->length () == 4)
	    return value_from_longest (type, (pswm >> 12) & 3);
	  else
	    return value_from_longest (type, (pswm >> 44) & 3);
	}
    }

  /* Unwind full GPRs to show at least the lower halves (as the
     upper halves are undefined).  */
  if (regnum_is_gpr_full (tdep, regnum))
    {
      int reg = regnum - tdep->gpr_full_regnum;
      struct value *val;

      val = frame_unwind_register_value (this_frame, S390_R0_REGNUM + reg);
      if (!val->optimized_out ())
	return value_cast (type, val);
    }

  return value::allocate_optimized_out (type);
}

/* Translate a .eh_frame register to DWARF register, or adjust a
   .debug_frame register.  */

static int
s390_adjust_frame_regnum (struct gdbarch *gdbarch, int num, int eh_frame_p)
{
  /* See s390_dwarf_reg_to_regnum for comments.  */
  return (num >= 0 && num < 16) ? num + s390_dwarf_reg_r0l : num;
}

/* DWARF-2 frame unwinding.  */

/* Function to unwind a pseudo-register in dwarf2_frame unwinder.  Used by
   s390_dwarf2_frame_init_reg.  */

static struct value *
s390_dwarf2_prev_register (frame_info_ptr this_frame, void **this_cache,
			   int regnum)
{
  return s390_unwind_pseudo_register (this_frame, regnum);
}

/* Implement init_reg dwarf2_frame method.  */

static void
s390_dwarf2_frame_init_reg (struct gdbarch *gdbarch, int regnum,
			    struct dwarf2_frame_state_reg *reg,
			    frame_info_ptr this_frame)
{
  /* The condition code (and thus PSW mask) is call-clobbered.  */
  if (regnum == S390_PSWM_REGNUM)
    reg->how = DWARF2_FRAME_REG_UNDEFINED;

  /* The PSW address unwinds to the return address.  */
  else if (regnum == S390_PSWA_REGNUM)
    reg->how = DWARF2_FRAME_REG_RA;

  /* Fixed registers are call-saved or call-clobbered
     depending on the ABI in use.  */
  else if (regnum < S390_NUM_REGS)
    {
      if (s390_register_call_saved (gdbarch, regnum))
	reg->how = DWARF2_FRAME_REG_SAME_VALUE;
      else
	reg->how = DWARF2_FRAME_REG_UNDEFINED;
    }

  /* We install a special function to unwind pseudos.  */
  else
    {
      reg->how = DWARF2_FRAME_REG_FN;
      reg->loc.fn = s390_dwarf2_prev_register;
    }
}

/* Frame unwinding. */

/* Wrapper for trad_frame_get_prev_register to allow for s390 pseudo
   register translation.  */

struct value *
s390_trad_frame_prev_register (frame_info_ptr this_frame,
			       trad_frame_saved_reg saved_regs[],
			       int regnum)
{
  if (regnum < S390_NUM_REGS)
    return trad_frame_get_prev_register (this_frame, saved_regs, regnum);
  else
    return s390_unwind_pseudo_register (this_frame, regnum);
}

/* Normal stack frames.  */

struct s390_unwind_cache {

  CORE_ADDR func;
  CORE_ADDR frame_base;
  CORE_ADDR local_base;

  trad_frame_saved_reg *saved_regs;
};

/* Unwind THIS_FRAME and write the information into unwind cache INFO using
   prologue analysis.  Helper for s390_frame_unwind_cache.  */

static int
s390_prologue_frame_unwind_cache (frame_info_ptr this_frame,
				  struct s390_unwind_cache *info)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  int word_size = gdbarch_ptr_bit (gdbarch) / 8;
  struct s390_prologue_data data;
  pv_t *fp = &data.gpr[S390_FRAME_REGNUM - S390_R0_REGNUM];
  pv_t *sp = &data.gpr[S390_SP_REGNUM - S390_R0_REGNUM];
  int i;
  CORE_ADDR cfa;
  CORE_ADDR func;
  CORE_ADDR result;
  ULONGEST reg;
  CORE_ADDR prev_sp;
  int frame_pointer;
  int size;
  frame_info_ptr next_frame;

  /* Try to find the function start address.  If we can't find it, we don't
     bother searching for it -- with modern compilers this would be mostly
     pointless anyway.  Trust that we'll either have valid DWARF-2 CFI data
     or else a valid backchain ...  */
  if (!get_frame_func_if_available (this_frame, &info->func))
    {
      info->func = -1;
      return 0;
    }
  func = info->func;

  /* Try to analyze the prologue.  */
  result = s390_analyze_prologue (gdbarch, func,
				  get_frame_pc (this_frame), &data);
  if (!result)
    return 0;

  /* If this was successful, we should have found the instruction that
     sets the stack pointer register to the previous value of the stack
     pointer minus the frame size.  */
  if (!pv_is_register (*sp, S390_SP_REGNUM))
    return 0;

  /* A frame size of zero at this point can mean either a real
     frameless function, or else a failure to find the prologue.
     Perform some sanity checks to verify we really have a
     frameless function.  */
  if (sp->k == 0)
    {
      /* If the next frame is a NORMAL_FRAME, this frame *cannot* have frame
	 size zero.  This is only possible if the next frame is a sentinel
	 frame, a dummy frame, or a signal trampoline frame.  */
      /* FIXME: cagney/2004-05-01: This sanity check shouldn't be
	 needed, instead the code should simpliy rely on its
	 analysis.  */
      next_frame = get_next_frame (this_frame);
      while (next_frame && get_frame_type (next_frame) == INLINE_FRAME)
	next_frame = get_next_frame (next_frame);
      if (next_frame
	  && get_frame_type (get_next_frame (this_frame)) == NORMAL_FRAME)
	return 0;

      /* If we really have a frameless function, %r14 must be valid
	 -- in particular, it must point to a different function.  */
      reg = get_frame_register_unsigned (this_frame, S390_RETADDR_REGNUM);
      reg = gdbarch_addr_bits_remove (gdbarch, reg) - 1;
      if (get_pc_function_start (reg) == func)
	{
	  /* However, there is one case where it *is* valid for %r14
	     to point to the same function -- if this is a recursive
	     call, and we have stopped in the prologue *before* the
	     stack frame was allocated.

	     Recognize this case by looking ahead a bit ...  */

	  struct s390_prologue_data data2;
	  pv_t *sp2 = &data2.gpr[S390_SP_REGNUM - S390_R0_REGNUM];

	  if (!(s390_analyze_prologue (gdbarch, func, (CORE_ADDR)-1, &data2)
		&& pv_is_register (*sp2, S390_SP_REGNUM)
		&& sp2->k != 0))
	    return 0;
	}
    }

  /* OK, we've found valid prologue data.  */
  size = -sp->k;

  /* If the frame pointer originally also holds the same value
     as the stack pointer, we're probably using it.  If it holds
     some other value -- even a constant offset -- it is most
     likely used as temp register.  */
  if (pv_is_identical (*sp, *fp))
    frame_pointer = S390_FRAME_REGNUM;
  else
    frame_pointer = S390_SP_REGNUM;

  /* If we've detected a function with stack frame, we'll still have to
     treat it as frameless if we're currently within the function epilog
     code at a point where the frame pointer has already been restored.
     This can only happen in an innermost frame.  */
  /* FIXME: cagney/2004-05-01: This sanity check shouldn't be needed,
     instead the code should simpliy rely on its analysis.  */
  next_frame = get_next_frame (this_frame);
  while (next_frame && get_frame_type (next_frame) == INLINE_FRAME)
    next_frame = get_next_frame (next_frame);
  if (size > 0
      && (next_frame == NULL
	  || get_frame_type (get_next_frame (this_frame)) != NORMAL_FRAME))
    {
      /* See the comment in s390_stack_frame_destroyed_p on why this is
	 not completely reliable ...  */
      if (s390_stack_frame_destroyed_p (gdbarch, get_frame_pc (this_frame)))
	{
	  memset (&data, 0, sizeof (data));
	  size = 0;
	  frame_pointer = S390_SP_REGNUM;
	}
    }

  /* Once we know the frame register and the frame size, we can unwind
     the current value of the frame register from the next frame, and
     add back the frame size to arrive that the previous frame's
     stack pointer value.  */
  prev_sp = get_frame_register_unsigned (this_frame, frame_pointer) + size;
  cfa = prev_sp + 16*word_size + 32;

  /* Set up ABI call-saved/call-clobbered registers.  */
  for (i = 0; i < S390_NUM_REGS; i++)
    if (!s390_register_call_saved (gdbarch, i))
      info->saved_regs[i].set_unknown ();

  /* CC is always call-clobbered.  */
  info->saved_regs[S390_PSWM_REGNUM].set_unknown ();

  /* Record the addresses of all register spill slots the prologue parser
     has recognized.  Consider only registers defined as call-saved by the
     ABI; for call-clobbered registers the parser may have recognized
     spurious stores.  */

  for (i = 0; i < 16; i++)
    if (s390_register_call_saved (gdbarch, S390_R0_REGNUM + i)
	&& data.gpr_slot[i] != 0)
      info->saved_regs[S390_R0_REGNUM + i].set_addr (cfa - data.gpr_slot[i]);

  for (i = 0; i < 16; i++)
    if (s390_register_call_saved (gdbarch, S390_F0_REGNUM + i)
	&& data.fpr_slot[i] != 0)
      info->saved_regs[S390_F0_REGNUM + i].set_addr (cfa - data.fpr_slot[i]);

  /* Function return will set PC to %r14.  */
  info->saved_regs[S390_PSWA_REGNUM] = info->saved_regs[S390_RETADDR_REGNUM];

  /* In frameless functions, we unwind simply by moving the return
     address to the PC.  However, if we actually stored to the
     save area, use that -- we might only think the function frameless
     because we're in the middle of the prologue ...  */
  if (size == 0
      && !info->saved_regs[S390_PSWA_REGNUM].is_addr ())
    {
      info->saved_regs[S390_PSWA_REGNUM].set_realreg (S390_RETADDR_REGNUM);
    }

  /* Another sanity check: unless this is a frameless function,
     we should have found spill slots for SP and PC.
     If not, we cannot unwind further -- this happens e.g. in
     libc's thread_start routine.  */
  if (size > 0)
    {
      if (!info->saved_regs[S390_SP_REGNUM].is_addr ()
	  || !info->saved_regs[S390_PSWA_REGNUM].is_addr ())
	prev_sp = -1;
    }

  /* We use the current value of the frame register as local_base,
     and the top of the register save area as frame_base.  */
  if (prev_sp != -1)
    {
      info->frame_base = prev_sp + 16*word_size + 32;
      info->local_base = prev_sp - size;
    }

  return 1;
}

/* Unwind THIS_FRAME and write the information into unwind cache INFO using
   back chain unwinding.  Helper for s390_frame_unwind_cache.  */

static void
s390_backchain_frame_unwind_cache (frame_info_ptr this_frame,
				   struct s390_unwind_cache *info)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  int word_size = gdbarch_ptr_bit (gdbarch) / 8;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR backchain;
  ULONGEST reg;
  LONGEST sp, tmp;
  int i;

  /* Set up ABI call-saved/call-clobbered registers.  */
  for (i = 0; i < S390_NUM_REGS; i++)
    if (!s390_register_call_saved (gdbarch, i))
      info->saved_regs[i].set_unknown ();

  /* CC is always call-clobbered.  */
  info->saved_regs[S390_PSWM_REGNUM].set_unknown ();

  /* Get the backchain.  */
  reg = get_frame_register_unsigned (this_frame, S390_SP_REGNUM);
  if (!safe_read_memory_integer (reg, word_size, byte_order, &tmp))
    tmp = 0;
  backchain = (CORE_ADDR) tmp;

  /* A zero backchain terminates the frame chain.  As additional
     sanity check, let's verify that the spill slot for SP in the
     save area pointed to by the backchain in fact links back to
     the save area.  */
  if (backchain != 0
      && safe_read_memory_integer (backchain + 15*word_size,
				   word_size, byte_order, &sp)
      && (CORE_ADDR)sp == backchain)
    {
      /* We don't know which registers were saved, but it will have
	 to be at least %r14 and %r15.  This will allow us to continue
	 unwinding, but other prev-frame registers may be incorrect ...  */
      info->saved_regs[S390_SP_REGNUM].set_addr (backchain + 15*word_size);
      info->saved_regs[S390_RETADDR_REGNUM].set_addr (backchain + 14*word_size);

      /* Function return will set PC to %r14.  */
      info->saved_regs[S390_PSWA_REGNUM]
	= info->saved_regs[S390_RETADDR_REGNUM];

      /* We use the current value of the frame register as local_base,
	 and the top of the register save area as frame_base.  */
      info->frame_base = backchain + 16*word_size + 32;
      info->local_base = reg;
    }

  info->func = get_frame_pc (this_frame);
}

/* Unwind THIS_FRAME and return the corresponding unwind cache for
   s390_frame_unwind and s390_frame_base.  */

static struct s390_unwind_cache *
s390_frame_unwind_cache (frame_info_ptr this_frame,
			 void **this_prologue_cache)
{
  struct s390_unwind_cache *info;

  if (*this_prologue_cache)
    return (struct s390_unwind_cache *) *this_prologue_cache;

  info = FRAME_OBSTACK_ZALLOC (struct s390_unwind_cache);
  *this_prologue_cache = info;
  info->saved_regs = trad_frame_alloc_saved_regs (this_frame);
  info->func = -1;
  info->frame_base = -1;
  info->local_base = -1;

  try
    {
      /* Try to use prologue analysis to fill the unwind cache.
	 If this fails, fall back to reading the stack backchain.  */
      if (!s390_prologue_frame_unwind_cache (this_frame, info))
	s390_backchain_frame_unwind_cache (this_frame, info);
    }
  catch (const gdb_exception_error &ex)
    {
      if (ex.error != NOT_AVAILABLE_ERROR)
	throw;
    }

  return info;
}

/* Implement this_id frame_unwind method for s390_frame_unwind.  */

static void
s390_frame_this_id (frame_info_ptr this_frame,
		    void **this_prologue_cache,
		    struct frame_id *this_id)
{
  struct s390_unwind_cache *info
    = s390_frame_unwind_cache (this_frame, this_prologue_cache);

  if (info->frame_base == -1)
    {
      if (info->func != -1)
	*this_id = frame_id_build_unavailable_stack (info->func);
      return;
    }

  *this_id = frame_id_build (info->frame_base, info->func);
}

/* Implement prev_register frame_unwind method for s390_frame_unwind.  */

static struct value *
s390_frame_prev_register (frame_info_ptr this_frame,
			  void **this_prologue_cache, int regnum)
{
  struct s390_unwind_cache *info
    = s390_frame_unwind_cache (this_frame, this_prologue_cache);

  return s390_trad_frame_prev_register (this_frame, info->saved_regs, regnum);
}

/* Default S390 frame unwinder.  */

static const struct frame_unwind s390_frame_unwind = {
  "s390 prologue",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  s390_frame_this_id,
  s390_frame_prev_register,
  NULL,
  default_frame_sniffer
};

/* Code stubs and their stack frames.  For things like PLTs and NULL
   function calls (where there is no true frame and the return address
   is in the RETADDR register).  */

struct s390_stub_unwind_cache
{
  CORE_ADDR frame_base;
  trad_frame_saved_reg *saved_regs;
};

/* Unwind THIS_FRAME and return the corresponding unwind cache for
   s390_stub_frame_unwind.  */

static struct s390_stub_unwind_cache *
s390_stub_frame_unwind_cache (frame_info_ptr this_frame,
			      void **this_prologue_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  int word_size = gdbarch_ptr_bit (gdbarch) / 8;
  struct s390_stub_unwind_cache *info;
  ULONGEST reg;

  if (*this_prologue_cache)
    return (struct s390_stub_unwind_cache *) *this_prologue_cache;

  info = FRAME_OBSTACK_ZALLOC (struct s390_stub_unwind_cache);
  *this_prologue_cache = info;
  info->saved_regs = trad_frame_alloc_saved_regs (this_frame);

  /* The return address is in register %r14.  */
  info->saved_regs[S390_PSWA_REGNUM].set_realreg (S390_RETADDR_REGNUM);

  /* Retrieve stack pointer and determine our frame base.  */
  reg = get_frame_register_unsigned (this_frame, S390_SP_REGNUM);
  info->frame_base = reg + 16*word_size + 32;

  return info;
}

/* Implement this_id frame_unwind method for s390_stub_frame_unwind.  */

static void
s390_stub_frame_this_id (frame_info_ptr this_frame,
			 void **this_prologue_cache,
			 struct frame_id *this_id)
{
  struct s390_stub_unwind_cache *info
    = s390_stub_frame_unwind_cache (this_frame, this_prologue_cache);
  *this_id = frame_id_build (info->frame_base, get_frame_pc (this_frame));
}

/* Implement prev_register frame_unwind method for s390_stub_frame_unwind.  */

static struct value *
s390_stub_frame_prev_register (frame_info_ptr this_frame,
			       void **this_prologue_cache, int regnum)
{
  struct s390_stub_unwind_cache *info
    = s390_stub_frame_unwind_cache (this_frame, this_prologue_cache);
  return s390_trad_frame_prev_register (this_frame, info->saved_regs, regnum);
}

/* Implement sniffer frame_unwind method for s390_stub_frame_unwind.  */

static int
s390_stub_frame_sniffer (const struct frame_unwind *self,
			 frame_info_ptr this_frame,
			 void **this_prologue_cache)
{
  CORE_ADDR addr_in_block;
  bfd_byte insn[S390_MAX_INSTR_SIZE];

  /* If the current PC points to non-readable memory, we assume we
     have trapped due to an invalid function pointer call.  We handle
     the non-existing current function like a PLT stub.  */
  addr_in_block = get_frame_address_in_block (this_frame);
  if (in_plt_section (addr_in_block)
      || s390_readinstruction (insn, get_frame_pc (this_frame)) < 0)
    return 1;
  return 0;
}

/* S390 stub frame unwinder.  */

static const struct frame_unwind s390_stub_frame_unwind = {
  "s390 stub",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  s390_stub_frame_this_id,
  s390_stub_frame_prev_register,
  NULL,
  s390_stub_frame_sniffer
};

/* Frame base handling.  */

static CORE_ADDR
s390_frame_base_address (frame_info_ptr this_frame, void **this_cache)
{
  struct s390_unwind_cache *info
    = s390_frame_unwind_cache (this_frame, this_cache);
  return info->frame_base;
}

static CORE_ADDR
s390_local_base_address (frame_info_ptr this_frame, void **this_cache)
{
  struct s390_unwind_cache *info
    = s390_frame_unwind_cache (this_frame, this_cache);
  return info->local_base;
}

static const struct frame_base s390_frame_base = {
  &s390_frame_unwind,
  s390_frame_base_address,
  s390_local_base_address,
  s390_local_base_address
};

/* Process record-replay */

/* Takes the intermediate sum of address calculations and masks off upper
   bits according to current addressing mode.  */

static CORE_ADDR
s390_record_address_mask (struct gdbarch *gdbarch, struct regcache *regcache,
			  CORE_ADDR val)
{
  s390_gdbarch_tdep *tdep = gdbarch_tdep<s390_gdbarch_tdep> (gdbarch);
  ULONGEST pswm, pswa;
  int am;
  if (tdep->abi == ABI_LINUX_S390)
    {
      regcache_raw_read_unsigned (regcache, S390_PSWA_REGNUM, &pswa);
      am = pswa >> 31 & 1;
    }
  else
    {
      regcache_raw_read_unsigned (regcache, S390_PSWM_REGNUM, &pswm);
      am = pswm >> 31 & 3;
    }
  switch (am)
    {
    case 0:
      return val & 0xffffff;
    case 1:
      return val & 0x7fffffff;
    case 3:
      return val;
    default:
      gdb_printf (gdb_stdlog, "Warning: Addressing mode %d used.", am);
      return 0;
    }
}

/* Calculates memory address using pre-calculated index, raw instruction word
   with b and d/dl fields, and raw instruction byte with dh field.  Index and
   dh should be set to 0 if unused.  */

static CORE_ADDR
s390_record_calc_disp_common (struct gdbarch *gdbarch, struct regcache *regcache,
			      ULONGEST x, uint16_t bd, int8_t dh)
{
  uint8_t rb = bd >> 12 & 0xf;
  int32_t d = (bd & 0xfff) | ((int32_t)dh << 12);
  ULONGEST b;
  CORE_ADDR res = d + x;
  if (rb)
    {
      regcache_raw_read_unsigned (regcache, S390_R0_REGNUM + rb, &b);
      res += b;
    }
  return s390_record_address_mask (gdbarch, regcache, res);
}

/* Calculates memory address using raw x, b + d/dl, dh fields from
   instruction.  rx and dh should be set to 0 if unused.  */

static CORE_ADDR
s390_record_calc_disp (struct gdbarch *gdbarch, struct regcache *regcache,
		       uint8_t rx, uint16_t bd, int8_t dh)
{
  ULONGEST x = 0;
  if (rx)
    regcache_raw_read_unsigned (regcache, S390_R0_REGNUM + rx, &x);
  return s390_record_calc_disp_common (gdbarch, regcache, x, bd, dh);
}

/* Calculates memory address for VSCE[GF] instructions.  */

static int
s390_record_calc_disp_vsce (struct gdbarch *gdbarch, struct regcache *regcache,
			    uint8_t vx, uint8_t el, uint8_t es, uint16_t bd,
			    int8_t dh, CORE_ADDR *res)
{
  s390_gdbarch_tdep *tdep = gdbarch_tdep<s390_gdbarch_tdep> (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  ULONGEST x;
  gdb_byte buf[16];
  if (tdep->v0_full_regnum == -1 || el * es >= 16)
    return -1;
  if (vx < 16)
    regcache->cooked_read (tdep->v0_full_regnum + vx, buf);
  else
    regcache->raw_read (S390_V16_REGNUM + vx - 16, buf);
  x = extract_unsigned_integer (buf + el * es, es, byte_order);
  *res = s390_record_calc_disp_common (gdbarch, regcache, x, bd, dh);
  return 0;
}

/* Calculates memory address for instructions with relative long addressing.  */

static CORE_ADDR
s390_record_calc_rl (struct gdbarch *gdbarch, struct regcache *regcache,
		     CORE_ADDR addr, uint16_t i1, uint16_t i2)
{
  int32_t ri = i1 << 16 | i2;
  return s390_record_address_mask (gdbarch, regcache, addr + (LONGEST)ri * 2);
}

/* Population count helper.  */

static int s390_popcnt (unsigned int x) {
  int res = 0;
  while (x)
    {
      if (x & 1)
	res++;
      x >>= 1;
    }
  return res;
}

/* Record 64-bit register.  */

static int
s390_record_gpr_g (struct gdbarch *gdbarch, struct regcache *regcache, int i)
{
  s390_gdbarch_tdep *tdep = gdbarch_tdep<s390_gdbarch_tdep> (gdbarch);
  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + i))
    return -1;
  if (tdep->abi == ABI_LINUX_S390)
    if (record_full_arch_list_add_reg (regcache, S390_R0_UPPER_REGNUM + i))
      return -1;
  return 0;
}

/* Record high 32 bits of a register.  */

static int
s390_record_gpr_h (struct gdbarch *gdbarch, struct regcache *regcache, int i)
{
  s390_gdbarch_tdep *tdep = gdbarch_tdep<s390_gdbarch_tdep> (gdbarch);
  if (tdep->abi == ABI_LINUX_S390)
    {
      if (record_full_arch_list_add_reg (regcache, S390_R0_UPPER_REGNUM + i))
	return -1;
    }
  else
    {
      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + i))
	return -1;
    }
  return 0;
}

/* Record vector register.  */

static int
s390_record_vr (struct gdbarch *gdbarch, struct regcache *regcache, int i)
{
  if (i < 16)
    {
      if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + i))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_V0_LOWER_REGNUM + i))
	return -1;
    }
  else
    {
      if (record_full_arch_list_add_reg (regcache, S390_V16_REGNUM + i - 16))
	return -1;
    }
  return 0;
}

/* Implement process_record gdbarch method.  */

static int
s390_process_record (struct gdbarch *gdbarch, struct regcache *regcache,
		     CORE_ADDR addr)
{
  s390_gdbarch_tdep *tdep = gdbarch_tdep<s390_gdbarch_tdep> (gdbarch);
  uint16_t insn[3] = {0};
  /* Instruction as bytes.  */
  uint8_t ibyte[6];
  /* Instruction as nibbles.  */
  uint8_t inib[12];
  /* Instruction vector registers.  */
  uint8_t ivec[4];
  CORE_ADDR oaddr, oaddr2, oaddr3;
  ULONGEST tmp;
  int i, n;
  /* if EX/EXRL instruction used, here's the reg parameter */
  int ex = -1;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  /* Attempting to use EX or EXRL jumps back here */
ex:

  /* Read instruction.  */
  insn[0] = read_memory_unsigned_integer (addr, 2, byte_order);
  /* If execute was involved, do the adjustment.  */
  if (ex != -1)
    insn[0] |= ex & 0xff;
  /* Two highest bits determine instruction size.  */
  if (insn[0] >= 0x4000)
    insn[1] = read_memory_unsigned_integer (addr+2, 2, byte_order);
  else
    /* Not necessary, but avoids uninitialized variable warnings.  */
    insn[1] = 0;
  if (insn[0] >= 0xc000)
    insn[2] = read_memory_unsigned_integer (addr+4, 2, byte_order);
  else
    insn[2] = 0;
  /* Split instruction into bytes and nibbles.  */
  for (i = 0; i < 3; i++)
    {
      ibyte[i*2] = insn[i] >> 8 & 0xff;
      ibyte[i*2+1] = insn[i] & 0xff;
    }
  for (i = 0; i < 6; i++)
    {
      inib[i*2] = ibyte[i] >> 4 & 0xf;
      inib[i*2+1] = ibyte[i] & 0xf;
    }
  /* Compute vector registers, if applicable.  */
  ivec[0] = (inib[9] >> 3 & 1) << 4 | inib[2];
  ivec[1] = (inib[9] >> 2 & 1) << 4 | inib[3];
  ivec[2] = (inib[9] >> 1 & 1) << 4 | inib[4];
  ivec[3] = (inib[9] >> 0 & 1) << 4 | inib[8];

  switch (ibyte[0])
    {
    /* 0x00 undefined */

    case 0x01:
      /* E-format instruction */
      switch (ibyte[1])
	{
	/* 0x00 undefined */
	/* 0x01 unsupported: PR - program return */
	/* 0x02 unsupported: UPT */
	/* 0x03 undefined */
	/* 0x04 privileged: PTFF - perform timing facility function */
	/* 0x05-0x06 undefined */
	/* 0x07 privileged: SCKPF - set clock programmable field */
	/* 0x08-0x09 undefined */

	case 0x0a: /* PFPO - perform floating point operation */
	  regcache_raw_read_unsigned (regcache, S390_R0_REGNUM, &tmp);
	  if (!(tmp & 0x80000000u))
	    {
	      uint8_t ofc = tmp >> 16 & 0xff;
	      switch (ofc)
		{
		case 0x00: /* HFP32 */
		case 0x01: /* HFP64 */
		case 0x05: /* BFP32 */
		case 0x06: /* BFP64 */
		case 0x08: /* DFP32 */
		case 0x09: /* DFP64 */
		  if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM))
		    return -1;
		  break;
		case 0x02: /* HFP128 */
		case 0x07: /* BFP128 */
		case 0x0a: /* DFP128 */
		  if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM))
		    return -1;
		  if (record_full_arch_list_add_reg (regcache, S390_F2_REGNUM))
		    return -1;
		  break;
		default:
		  gdb_printf (gdb_stdlog, "Warning: Unknown PFPO OFC %02x at %s.\n",
			      ofc, paddress (gdbarch, addr));
		  return -1;
		}

	      if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
		return -1;
	    }
	  if (record_full_arch_list_add_reg (regcache, S390_R1_REGNUM))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	case 0x0b: /* TAM - test address mode */
	case 0x0c: /* SAM24 - set address mode 24 */
	case 0x0d: /* SAM31 - set address mode 31 */
	case 0x0e: /* SAM64 - set address mode 64 */
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	/* 0x0f-0xfe undefined */

	/* 0xff unsupported: TRAP */

	default:
	  goto UNKNOWN_OP;
	}
      break;

    /* 0x02 undefined */
    /* 0x03 undefined */

    case 0x04: /* SPM - set program mask */
      if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	return -1;
      break;

    case 0x05: /* BALR - branch and link */
    case 0x45: /* BAL - branch and link */
    case 0x06: /* BCTR - branch on count */
    case 0x46: /* BCT - branch on count */
    case 0x0d: /* BASR - branch and save */
    case 0x4d: /* BAS - branch and save */
    case 0x84: /* BRXH - branch relative on index high */
    case 0x85: /* BRXLE - branch relative on index low or equal */
    case 0x86: /* BXH - branch on index high */
    case 0x87: /* BXLE - branch on index low or equal */
      /* BA[SL]* use native-size destination for linkage info, BCT*, BRX*, BX*
	 use 32-bit destination as counter.  */
      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[2]))
	return -1;
      break;

    case 0x07: /* BCR - branch on condition */
    case 0x47: /* BC - branch on condition */
      /* No effect other than PC transfer.  */
      break;

    /* 0x08 undefined */
    /* 0x09 undefined */

    case 0x0a:
      /* SVC - supervisor call */
      if (tdep->s390_syscall_record != NULL)
	{
	  if (tdep->s390_syscall_record (regcache, ibyte[1]))
	    return -1;
	}
      else
	{
	  gdb_printf (gdb_stderr, _("no syscall record support\n"));
	  return -1;
	}
      break;

    case 0x0b: /* BSM - branch and set mode */
      if (inib[2])
	if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[2]))
	  return -1;
      if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	return -1;
      break;

    case 0x0c: /* BASSM - branch and save and set mode */
      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[2]))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	return -1;
      break;

    case 0x0e: /* MVCL - move long [interruptible] */
      regcache_raw_read_unsigned (regcache, S390_R0_REGNUM + inib[2], &tmp);
      oaddr = s390_record_address_mask (gdbarch, regcache, tmp);
      regcache_raw_read_unsigned (regcache, S390_R0_REGNUM + (inib[2] | 1), &tmp);
      tmp &= 0xffffff;
      if (record_full_arch_list_add_mem (oaddr, tmp))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[2]))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + (inib[2] | 1)))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[3]))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + (inib[3] | 1)))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	return -1;
      break;

    case 0x0f: /* CLCL - compare logical long [interruptible] */
    case 0xa9: /* CLCLE - compare logical long extended [partial] */
      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[2]))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + (inib[2] | 1)))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[3]))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + (inib[3] | 1)))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	return -1;
      break;

    case 0x10: /* LPR - load positive */
    case 0x11: /* LNR - load negative */
    case 0x12: /* LTR - load and test */
    case 0x13: /* LCR - load complement */
    case 0x14: /* NR - and */
    case 0x16: /* OR - or */
    case 0x17: /* XR - xor */
    case 0x1a: /* AR - add */
    case 0x1b: /* SR - subtract */
    case 0x1e: /* ALR - add logical */
    case 0x1f: /* SLR - subtract logical */
    case 0x54: /* N - and */
    case 0x56: /* O - or */
    case 0x57: /* X - xor */
    case 0x5a: /* A - add */
    case 0x5b: /* S - subtract */
    case 0x5e: /* AL - add logical */
    case 0x5f: /* SL - subtract logical */
    case 0x4a: /* AH - add halfword */
    case 0x4b: /* SH - subtract halfword */
    case 0x8a: /* SRA - shift right single */
    case 0x8b: /* SLA - shift left single */
    case 0xbf: /* ICM - insert characters under mask */
      /* 32-bit destination + flags */
      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[2]))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	return -1;
      break;

    case 0x15: /* CLR - compare logical */
    case 0x55: /* CL - compare logical */
    case 0x19: /* CR - compare */
    case 0x29: /* CDR - compare */
    case 0x39: /* CER - compare */
    case 0x49: /* CH - compare halfword */
    case 0x59: /* C - compare */
    case 0x69: /* CD - compare */
    case 0x79: /* CE - compare */
    case 0x91: /* TM - test under mask */
    case 0x95: /* CLI - compare logical */
    case 0xbd: /* CLM - compare logical under mask */
    case 0xd5: /* CLC - compare logical */
      if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	return -1;
      break;

    case 0x18: /* LR - load */
    case 0x48: /* LH - load halfword */
    case 0x58: /* L - load */
    case 0x41: /* LA - load address */
    case 0x43: /* IC - insert character */
    case 0x4c: /* MH - multiply halfword */
    case 0x71: /* MS - multiply single */
    case 0x88: /* SRL - shift right single logical */
    case 0x89: /* SLL - shift left single logical */
      /* 32-bit, 8-bit (IC), or native width (LA) destination, no flags */
      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[2]))
	return -1;
      break;

    case 0x1c: /* MR - multiply */
    case 0x5c: /* M - multiply */
    case 0x1d: /* DR - divide */
    case 0x5d: /* D - divide */
    case 0x8c: /* SRDL - shift right double logical */
    case 0x8d: /* SLDL - shift left double logical */
      /* 32-bit pair destination, no flags */
      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[2]))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + (inib[2] | 1)))
	return -1;
      break;

    case 0x20: /* LPDR - load positive */
    case 0x30: /* LPER - load positive */
    case 0x21: /* LNDR - load negative */
    case 0x31: /* LNER - load negative */
    case 0x22: /* LTDR - load and test */
    case 0x32: /* LTER - load and test */
    case 0x23: /* LCDR - load complement */
    case 0x33: /* LCER - load complement */
    case 0x2a: /* ADR - add */
    case 0x3a: /* AER - add */
    case 0x6a: /* AD - add */
    case 0x7a: /* AE - add */
    case 0x2b: /* SDR - subtract */
    case 0x3b: /* SER - subtract */
    case 0x6b: /* SD - subtract */
    case 0x7b: /* SE - subtract */
    case 0x2e: /* AWR - add unnormalized */
    case 0x3e: /* AUR - add unnormalized */
    case 0x6e: /* AW - add unnormalized */
    case 0x7e: /* AU - add unnormalized */
    case 0x2f: /* SWR - subtract unnormalized */
    case 0x3f: /* SUR - subtract unnormalized */
    case 0x6f: /* SW - subtract unnormalized */
    case 0x7f: /* SU - subtract unnormalized */
      /* float destination + flags */
      if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + inib[2]))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	return -1;
      break;

    case 0x24: /* HDR - halve */
    case 0x34: /* HER - halve */
    case 0x25: /* LDXR - load rounded */
    case 0x35: /* LEDR - load rounded */
    case 0x28: /* LDR - load */
    case 0x38: /* LER - load */
    case 0x68: /* LD - load */
    case 0x78: /* LE - load */
    case 0x2c: /* MDR - multiply */
    case 0x3c: /* MDER - multiply */
    case 0x6c: /* MD - multiply */
    case 0x7c: /* MDE - multiply */
    case 0x2d: /* DDR - divide */
    case 0x3d: /* DER - divide */
    case 0x6d: /* DD - divide */
    case 0x7d: /* DE - divide */
      /* float destination, no flags */
      if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + inib[2]))
	return -1;
      break;

    case 0x26: /* MXR - multiply */
    case 0x27: /* MXDR - multiply */
    case 0x67: /* MXD - multiply */
      /* float pair destination, no flags */
      if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + inib[2]))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + (inib[2] | 2)))
	return -1;
      break;

    case 0x36: /* AXR - add */
    case 0x37: /* SXR - subtract */
      /* float pair destination + flags */
      if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + inib[2]))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + (inib[2] | 2)))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	return -1;
      break;

    case 0x40: /* STH - store halfword */
      oaddr = s390_record_calc_disp (gdbarch, regcache, inib[3], insn[1], 0);
      if (record_full_arch_list_add_mem (oaddr, 2))
	return -1;
      break;

    case 0x42: /* STC - store character */
      oaddr = s390_record_calc_disp (gdbarch, regcache, inib[3], insn[1], 0);
      if (record_full_arch_list_add_mem (oaddr, 1))
	return -1;
      break;

    case 0x44: /* EX - execute */
      if (ex != -1)
	{
	  gdb_printf (gdb_stdlog, "Warning: Double execute at %s.\n",
		      paddress (gdbarch, addr));
	  return -1;
	}
      addr = s390_record_calc_disp (gdbarch, regcache, inib[3], insn[1], 0);
      if (inib[2])
	{
	  regcache_raw_read_unsigned (regcache, S390_R0_REGNUM + inib[2], &tmp);
	  ex = tmp & 0xff;
	}
      else
	{
	  ex = 0;
	}
      goto ex;

    case 0x4e: /* CVD - convert to decimal */
    case 0x60: /* STD - store */
      oaddr = s390_record_calc_disp (gdbarch, regcache, inib[3], insn[1], 0);
      if (record_full_arch_list_add_mem (oaddr, 8))
	return -1;
      break;

    case 0x4f: /* CVB - convert to binary */
      /* 32-bit gpr destination + FPC (DXC write) */
      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[2]))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	return -1;
      break;

    case 0x50: /* ST - store */
    case 0x70: /* STE - store */
      oaddr = s390_record_calc_disp (gdbarch, regcache, inib[3], insn[1], 0);
      if (record_full_arch_list_add_mem (oaddr, 4))
	return -1;
      break;

    case 0x51: /* LAE - load address extended */
      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[2]))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_A0_REGNUM + inib[2]))
	return -1;
      break;

    /* 0x52 undefined */
    /* 0x53 undefined */

    /* 0x61-0x66 undefined */

    /* 0x72-0x77 undefined */

    /* 0x80 privileged: SSM - set system mask */
    /* 0x81 undefined */
    /* 0x82 privileged: LPSW - load PSW */
    /* 0x83 privileged: diagnose */

    case 0x8e: /* SRDA - shift right double */
    case 0x8f: /* SLDA - shift left double */
      /* 32-bit pair destination + flags */
      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[2]))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + (inib[2] | 1)))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	return -1;
      break;

    case 0x90: /* STM - store multiple */
    case 0x9b: /* STAM - store access multiple */
      oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], 0);
      if (inib[2] <= inib[3])
	n = inib[3] - inib[2] + 1;
      else
	n = inib[3] + 0x10 - inib[2] + 1;
      if (record_full_arch_list_add_mem (oaddr, n * 4))
	return -1;
      break;

    case 0x92: /* MVI - move */
      oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], 0);
      if (record_full_arch_list_add_mem (oaddr, 1))
	return -1;
      break;

    case 0x93: /* TS - test and set */
    case 0x94: /* NI - and */
    case 0x96: /* OI - or */
    case 0x97: /* XI - xor */
      oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], 0);
      if (record_full_arch_list_add_mem (oaddr, 1))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	return -1;
      break;

    case 0x98: /* LM - load multiple */
      for (i = inib[2]; i != inib[3]; i++, i &= 0xf)
	if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + i))
	  return -1;
      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[3]))
	return -1;
      break;

    /* 0x99 privileged: TRACE */

    case 0x9a: /* LAM - load access multiple */
      for (i = inib[2]; i != inib[3]; i++, i &= 0xf)
	if (record_full_arch_list_add_reg (regcache, S390_A0_REGNUM + i))
	  return -1;
      if (record_full_arch_list_add_reg (regcache, S390_A0_REGNUM + inib[3]))
	return -1;
      break;

    /* 0x9c-0x9f privileged and obsolete (old I/O) */
    /* 0xa0-0xa4 undefined */

    case 0xa5:
    case 0xa7:
      /* RI-format instruction */
      switch (ibyte[0] << 4 | inib[3])
	{
	case 0xa50: /* IIHH - insert immediate */
	case 0xa51: /* IIHL - insert immediate */
	  /* high 32-bit destination */
	  if (s390_record_gpr_h (gdbarch, regcache, inib[2]))
	    return -1;
	  break;

	case 0xa52: /* IILH - insert immediate */
	case 0xa53: /* IILL - insert immediate */
	case 0xa75: /* BRAS - branch relative and save */
	case 0xa76: /* BRCT - branch relative on count */
	case 0xa78: /* LHI - load halfword immediate */
	case 0xa7c: /* MHI - multiply halfword immediate */
	  /* 32-bit or native destination */
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[2]))
	    return -1;
	  break;

	case 0xa54: /* NIHH - and immediate */
	case 0xa55: /* NIHL - and immediate */
	case 0xa58: /* OIHH - or immediate */
	case 0xa59: /* OIHL - or immediate */
	  /* high 32-bit destination + flags */
	  if (s390_record_gpr_h (gdbarch, regcache, inib[2]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	case 0xa56: /* NILH - and immediate */
	case 0xa57: /* NILL - and immediate */
	case 0xa5a: /* OILH - or immediate */
	case 0xa5b: /* OILL - or immediate */
	case 0xa7a: /* AHI - add halfword immediate */
	  /* 32-bit destination + flags */
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[2]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	case 0xa5c: /* LLIHH - load logical immediate */
	case 0xa5d: /* LLIHL - load logical immediate */
	case 0xa5e: /* LLILH - load logical immediate */
	case 0xa5f: /* LLILL - load logical immediate */
	case 0xa77: /* BRCTG - branch relative on count */
	case 0xa79: /* LGHI - load halfword immediate */
	case 0xa7d: /* MGHI - multiply halfword immediate */
	  /* 64-bit destination */
	  if (s390_record_gpr_g (gdbarch, regcache, inib[2]))
	    return -1;
	  break;

	case 0xa70: /* TMLH - test under mask */
	case 0xa71: /* TMLL - test under mask */
	case 0xa72: /* TMHH - test under mask */
	case 0xa73: /* TMHL - test under mask */
	case 0xa7e: /* CHI - compare halfword immediate */
	case 0xa7f: /* CGHI - compare halfword immediate */
	  /* flags only */
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	case 0xa74: /* BRC - branch relative on condition */
	  /* no register change */
	  break;

	case 0xa7b: /* AGHI - add halfword immediate */
	  /* 64-bit destination + flags */
	  if (s390_record_gpr_g (gdbarch, regcache, inib[2]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	default:
	  goto UNKNOWN_OP;
	}
      break;

    /* 0xa6 undefined */

    case 0xa8: /* MVCLE - move long extended [partial] */
      regcache_raw_read_unsigned (regcache, S390_R0_REGNUM + inib[2], &tmp);
      oaddr = s390_record_address_mask (gdbarch, regcache, tmp);
      regcache_raw_read_unsigned (regcache, S390_R0_REGNUM + (inib[2] | 1), &tmp);
      if (record_full_arch_list_add_mem (oaddr, tmp))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[2]))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + (inib[2] | 1)))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[3]))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + (inib[3] | 1)))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	return -1;
      break;

    /* 0xaa-0xab undefined */
    /* 0xac privileged: STNSM - store then and system mask */
    /* 0xad privileged: STOSM - store then or system mask */
    /* 0xae privileged: SIGP - signal processor */
    /* 0xaf unsupported: MC - monitor call */
    /* 0xb0 undefined */
    /* 0xb1 privileged: LRA - load real address */

    case 0xb2:
    case 0xb3:
    case 0xb9:
      /* S/RRD/RRE/RRF/IE-format instruction */
      switch (insn[0])
	{
	/* 0xb200-0xb204 undefined or privileged */

	case 0xb205: /* STCK - store clock */
	case 0xb27c: /* STCKF - store clock fast */
	  oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], 0);
	  if (record_full_arch_list_add_mem (oaddr, 8))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	/* 0xb206-0xb219 undefined, privileged, or unsupported */
	/* 0xb21a unsupported: CFC */
	/* 0xb21b-0xb221 undefined or privileged */

	case 0xb222: /* IPM - insert program mask */
	case 0xb24f: /* EAR - extract access */
	case 0xb252: /* MSR - multiply single */
	case 0xb2ec: /* ETND - extract transaction nesting depth */
	case 0xb38c: /* EFPC - extract fpc */
	case 0xb91f: /* LRVR - load reversed */
	case 0xb926: /* LBR - load byte */
	case 0xb927: /* LHR - load halfword */
	case 0xb994: /* LLCR - load logical character */
	case 0xb995: /* LLHR - load logical halfword */
	case 0xb9f2: /* LOCR - load on condition */
	  /* 32-bit gpr destination */
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[6]))
	    return -1;
	  break;

	/* 0xb223-0xb22c privileged or unsupported */

	case 0xb22d: /* DXR - divide */
	case 0xb325: /* LXDR - load lengthened */
	case 0xb326: /* LXER - load lengthened */
	case 0xb336: /* SQXR - square root */
	case 0xb365: /* LXR - load */
	case 0xb367: /* FIXR - load fp integer */
	case 0xb376: /* LZXR - load zero */
	case 0xb3b6: /* CXFR - convert from fixed */
	case 0xb3c6: /* CXGR - convert from fixed */
	case 0xb3fe: /* IEXTR - insert biased exponent */
	  /* float pair destination */
	  if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + inib[6]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + (inib[6] | 2)))
	    return -1;
	  break;

	/* 0xb22e-0xb240 undefined, privileged, or unsupported */

	case 0xb241: /* CKSM - checksum [partial] */
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[6]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[7]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + (inib[7] | 1)))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	/* 0xb242-0xb243 undefined */

	case 0xb244: /* SQDR - square root */
	case 0xb245: /* SQER - square root */
	case 0xb324: /* LDER - load lengthened */
	case 0xb337: /* MEER - multiply */
	case 0xb366: /* LEXR - load rounded */
	case 0xb370: /* LPDFR - load positive */
	case 0xb371: /* LNDFR - load negative */
	case 0xb372: /* CSDFR - copy sign */
	case 0xb373: /* LCDFR - load complement */
	case 0xb374: /* LZER - load zero */
	case 0xb375: /* LZDR - load zero */
	case 0xb377: /* FIER - load fp integer */
	case 0xb37f: /* FIDR - load fp integer */
	case 0xb3b4: /* CEFR - convert from fixed */
	case 0xb3b5: /* CDFR - convert from fixed */
	case 0xb3c1: /* LDGR - load fpr from gr */
	case 0xb3c4: /* CEGR - convert from fixed */
	case 0xb3c5: /* CDGR - convert from fixed */
	case 0xb3f6: /* IEDTR - insert biased exponent */
	  /* float destination */
	  if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + inib[6]))
	    return -1;
	  break;

	/* 0xb246-0xb24c: privileged or unsupported */

	case 0xb24d: /* CPYA - copy access */
	case 0xb24e: /* SAR - set access */
	  if (record_full_arch_list_add_reg (regcache, S390_A0_REGNUM + inib[6]))
	    return -1;
	  break;

	/* 0xb250-0xb251 undefined or privileged */
	/* 0xb253-0xb254 undefined or privileged */

	case 0xb255: /* MVST - move string [partial] */
	  {
	    uint8_t end;
	    gdb_byte cur;
	    ULONGEST num = 0;
	    /* Read ending byte.  */
	    regcache_raw_read_unsigned (regcache, S390_R0_REGNUM, &tmp);
	    end = tmp & 0xff;
	    /* Get address of second operand.  */
	    regcache_raw_read_unsigned (regcache, S390_R0_REGNUM + inib[7], &tmp);
	    oaddr = s390_record_address_mask (gdbarch, regcache, tmp);
	    /* Search for ending byte and compute length.  */
	    do {
	      num++;
	      if (target_read_memory (oaddr, &cur, 1))
		return -1;
	      oaddr++;
	    } while (cur != end);
	    /* Get address of first operand and record it.  */
	    regcache_raw_read_unsigned (regcache, S390_R0_REGNUM + inib[6], &tmp);
	    oaddr = s390_record_address_mask (gdbarch, regcache, tmp);
	    if (record_full_arch_list_add_mem (oaddr, num))
	      return -1;
	    /* Record the registers.  */
	    if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[6]))
	      return -1;
	    if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[7]))
	      return -1;
	    if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	      return -1;
	  }
	  break;

	/* 0xb256 undefined */

	case 0xb257: /* CUSE - compare until substring equal [interruptible] */
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[6]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + (inib[6] | 1)))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[7]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + (inib[7] | 1)))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	/* 0xb258-0xb25c undefined, privileged, or unsupported */

	case 0xb25d: /* CLST - compare logical string [partial] */
	case 0xb25e: /* SRST - search string [partial] */
	case 0xb9be: /* SRSTU - search string unicode [partial] */
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[6]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[7]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	/* 0xb25f-0xb262 undefined */

	case 0xb263: /* CMPSC - compression call [interruptible] */
	  regcache_raw_read_unsigned (regcache, S390_R0_REGNUM + inib[6], &tmp);
	  oaddr = s390_record_address_mask (gdbarch, regcache, tmp);
	  regcache_raw_read_unsigned (regcache, S390_R0_REGNUM + (inib[6] | 1), &tmp);
	  if (record_full_arch_list_add_mem (oaddr, tmp))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[6]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + (inib[6] | 1)))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[7]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + (inib[7] | 1)))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_R1_REGNUM))
	    return -1;
	  /* DXC may be written */
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	/* 0xb264-0xb277 undefined, privileged, or unsupported */

	case 0xb278: /* STCKE - store clock extended */
	  oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], 0);
	  if (record_full_arch_list_add_mem (oaddr, 16))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	/* 0xb279-0xb27b undefined or unsupported */
	/* 0xb27d-0xb298 undefined or privileged */

	case 0xb299: /* SRNM - set rounding mode */
	case 0xb2b8: /* SRNMB - set bfp rounding mode */
	case 0xb2b9: /* SRNMT - set dfp rounding mode */
	case 0xb29d: /* LFPC - load fpc */
	case 0xb2bd: /* LFAS - load fpc and signal */
	case 0xb384: /* SFPC - set fpc */
	case 0xb385: /* SFASR - set fpc and signal */
	case 0xb960: /* CGRT - compare and trap */
	case 0xb961: /* CLGRT - compare logical and trap */
	case 0xb972: /* CRT - compare and trap */
	case 0xb973: /* CLRT - compare logical and trap */
	  /* fpc only - including possible DXC write for trapping insns */
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  break;

	/* 0xb29a-0xb29b undefined */

	case 0xb29c: /* STFPC - store fpc */
	  oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], 0);
	  if (record_full_arch_list_add_mem (oaddr, 4))
	    return -1;
	  break;

	/* 0xb29e-0xb2a4 undefined */

	case 0xb2a5: /* TRE - translate extended [partial] */
	  regcache_raw_read_unsigned (regcache, S390_R0_REGNUM + inib[6], &tmp);
	  oaddr = s390_record_address_mask (gdbarch, regcache, tmp);
	  regcache_raw_read_unsigned (regcache, S390_R0_REGNUM + (inib[6] | 1), &tmp);
	  if (record_full_arch_list_add_mem (oaddr, tmp))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[6]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + (inib[6] | 1)))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	case 0xb2a6: /* CU21 - convert UTF-16 to UTF-8 [partial] */
	case 0xb2a7: /* CU12 - convert UTF-8 to UTF-16 [partial] */
	case 0xb9b0: /* CU14 - convert UTF-8 to UTF-32 [partial] */
	case 0xb9b1: /* CU24 - convert UTF-16 to UTF-32 [partial] */
	case 0xb9b2: /* CU41 - convert UTF-32 to UTF-8 [partial] */
	case 0xb9b3: /* CU42 - convert UTF-32 to UTF-16 [partial] */
	  regcache_raw_read_unsigned (regcache, S390_R0_REGNUM + inib[6], &tmp);
	  oaddr = s390_record_address_mask (gdbarch, regcache, tmp);
	  regcache_raw_read_unsigned (regcache, S390_R0_REGNUM + (inib[6] | 1), &tmp);
	  if (record_full_arch_list_add_mem (oaddr, tmp))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[6]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + (inib[6] | 1)))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[7]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + (inib[7] | 1)))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	/* 0xb2a8-0xb2af undefined */

	case 0xb2b0: /* STFLE - store facility list extended */
	  oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], 0);
	  regcache_raw_read_unsigned (regcache, S390_R0_REGNUM, &tmp);
	  tmp &= 0xff;
	  if (record_full_arch_list_add_mem (oaddr, 8 * (tmp + 1)))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	/* 0xb2b1-0xb2b7 undefined or privileged */
	/* 0xb2ba-0xb2bc undefined */
	/* 0xb2be-0xb2e7 undefined */
	/* 0xb2e9-0xb2eb undefined */
	/* 0xb2ed-0xb2f7 undefined */
	/* 0xb2f8 unsupported: TEND */
	/* 0xb2f9 undefined */

	case 0xb2e8: /* PPA - perform processor assist */
	case 0xb2fa: /* NIAI - next instruction access intent */
	  /* no visible effects */
	  break;

	/* 0xb2fb undefined */
	/* 0xb2fc unsupported: TABORT */
	/* 0xb2fd-0xb2fe undefined */
	/* 0xb2ff unsupported: TRAP */

	case 0xb300: /* LPEBR - load positive */
	case 0xb301: /* LNEBR - load negative */
	case 0xb303: /* LCEBR - load complement */
	case 0xb310: /* LPDBR - load positive */
	case 0xb311: /* LNDBR - load negative */
	case 0xb313: /* LCDBR - load complement */
	case 0xb350: /* TBEDR - convert hfp to bfp */
	case 0xb351: /* TBDR - convert hfp to bfp */
	case 0xb358: /* THDER - convert bfp to hfp */
	case 0xb359: /* THDR - convert bfp to hfp */
	  /* float destination + flags */
	  if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + inib[6]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	case 0xb304: /* LDEBR - load lengthened */
	case 0xb30c: /* MDEBR - multiply */
	case 0xb30d: /* DEBR - divide */
	case 0xb314: /* SQEBR - square root */
	case 0xb315: /* SQDBR - square root */
	case 0xb317: /* MEEBR - multiply */
	case 0xb31c: /* MDBR - multiply */
	case 0xb31d: /* DDBR - divide */
	case 0xb344: /* LEDBRA - load rounded */
	case 0xb345: /* LDXBRA - load rounded */
	case 0xb346: /* LEXBRA - load rounded */
	case 0xb357: /* FIEBRA - load fp integer */
	case 0xb35f: /* FIDBRA - load fp integer */
	case 0xb390: /* CELFBR - convert from logical */
	case 0xb391: /* CDLFBR - convert from logical */
	case 0xb394: /* CEFBR - convert from fixed */
	case 0xb395: /* CDFBR - convert from fixed */
	case 0xb3a0: /* CELGBR - convert from logical */
	case 0xb3a1: /* CDLGBR - convert from logical */
	case 0xb3a4: /* CEGBR - convert from fixed */
	case 0xb3a5: /* CDGBR - convert from fixed */
	case 0xb3d0: /* MDTR - multiply */
	case 0xb3d1: /* DDTR - divide */
	case 0xb3d4: /* LDETR - load lengthened */
	case 0xb3d5: /* LEDTR - load lengthened */
	case 0xb3d7: /* FIDTR - load fp integer */
	case 0xb3dd: /* LDXTR - load lengthened */
	case 0xb3f1: /* CDGTR - convert from fixed */
	case 0xb3f2: /* CDUTR - convert from unsigned packed */
	case 0xb3f3: /* CDSTR - convert from signed packed */
	case 0xb3f5: /* QADTR - quantize */
	case 0xb3f7: /* RRDTR - reround */
	case 0xb951: /* CDFTR - convert from fixed */
	case 0xb952: /* CDLGTR - convert from logical */
	case 0xb953: /* CDLFTR - convert from logical */
	  /* float destination + fpc */
	  if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + inib[6]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  break;

	case 0xb305: /* LXDBR - load lengthened */
	case 0xb306: /* LXEBR - load lengthened */
	case 0xb307: /* MXDBR - multiply */
	case 0xb316: /* SQXBR - square root */
	case 0xb34c: /* MXBR - multiply */
	case 0xb34d: /* DXBR - divide */
	case 0xb347: /* FIXBRA - load fp integer */
	case 0xb392: /* CXLFBR - convert from logical */
	case 0xb396: /* CXFBR - convert from fixed */
	case 0xb3a2: /* CXLGBR - convert from logical */
	case 0xb3a6: /* CXGBR - convert from fixed */
	case 0xb3d8: /* MXTR - multiply */
	case 0xb3d9: /* DXTR - divide */
	case 0xb3dc: /* LXDTR - load lengthened */
	case 0xb3df: /* FIXTR - load fp integer */
	case 0xb3f9: /* CXGTR - convert from fixed */
	case 0xb3fa: /* CXUTR - convert from unsigned packed */
	case 0xb3fb: /* CXSTR - convert from signed packed */
	case 0xb3fd: /* QAXTR - quantize */
	case 0xb3ff: /* RRXTR - reround */
	case 0xb959: /* CXFTR - convert from fixed */
	case 0xb95a: /* CXLGTR - convert from logical */
	case 0xb95b: /* CXLFTR - convert from logical */
	  /* float pair destination + fpc */
	  if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + inib[6]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + (inib[6] | 2)))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  break;

	case 0xb308: /* KEBR - compare and signal */
	case 0xb309: /* CEBR - compare */
	case 0xb318: /* KDBR - compare and signal */
	case 0xb319: /* CDBR - compare */
	case 0xb348: /* KXBR - compare and signal */
	case 0xb349: /* CXBR - compare */
	case 0xb3e0: /* KDTR - compare and signal */
	case 0xb3e4: /* CDTR - compare */
	case 0xb3e8: /* KXTR - compare and signal */
	case 0xb3ec: /* CXTR - compare */
	  /* flags + fpc only */
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  break;

	case 0xb302: /* LTEBR - load and test */
	case 0xb312: /* LTDBR - load and test */
	case 0xb30a: /* AEBR - add */
	case 0xb30b: /* SEBR - subtract */
	case 0xb31a: /* ADBR - add */
	case 0xb31b: /* SDBR - subtract */
	case 0xb3d2: /* ADTR - add */
	case 0xb3d3: /* SDTR - subtract */
	case 0xb3d6: /* LTDTR - load and test */
	  /* float destination + flags + fpc */
	  if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + inib[6]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  break;

	case 0xb30e: /* MAEBR - multiply and add */
	case 0xb30f: /* MSEBR - multiply and subtract */
	case 0xb31e: /* MADBR - multiply and add */
	case 0xb31f: /* MSDBR - multiply and subtract */
	  /* float destination [RRD] + fpc */
	  if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + inib[4]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  break;

	/* 0xb320-0xb323 undefined */
	/* 0xb327-0xb32d undefined */

	case 0xb32e: /* MAER - multiply and add */
	case 0xb32f: /* MSER - multiply and subtract */
	case 0xb338: /* MAYLR - multiply and add unnormalized */
	case 0xb339: /* MYLR - multiply unnormalized */
	case 0xb33c: /* MAYHR - multiply and add unnormalized */
	case 0xb33d: /* MYHR - multiply unnormalized */
	case 0xb33e: /* MADR - multiply and add */
	case 0xb33f: /* MSDR - multiply and subtract */
	  /* float destination [RRD] */
	  if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + inib[4]))
	    return -1;
	  break;

	/* 0xb330-0xb335 undefined */

	case 0xb33a: /* MAYR - multiply and add unnormalized */
	case 0xb33b: /* MYR - multiply unnormalized */
	  /* float pair destination [RRD] */
	  if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + inib[4]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + (inib[4] | 2)))
	    return -1;
	  break;

	case 0xb340: /* LPXBR - load positive */
	case 0xb341: /* LNXBR - load negative */
	case 0xb343: /* LCXBR - load complement */
	case 0xb360: /* LPXR - load positive */
	case 0xb361: /* LNXR - load negative */
	case 0xb362: /* LTXR - load and test */
	case 0xb363: /* LCXR - load complement */
	  /* float pair destination + flags */
	  if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + inib[6]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + (inib[6] | 2)))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	case 0xb342: /* LTXBR - load and test */
	case 0xb34a: /* AXBR - add */
	case 0xb34b: /* SXBR - subtract */
	case 0xb3da: /* AXTR - add */
	case 0xb3db: /* SXTR - subtract */
	case 0xb3de: /* LTXTR - load and test */
	  /* float pair destination + flags + fpc */
	  if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + inib[6]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + (inib[6] | 2)))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  break;

	/* 0xb34e-0xb34f undefined */
	/* 0xb352 undefined */

	case 0xb353: /* DIEBR - divide to integer */
	case 0xb35b: /* DIDBR - divide to integer */
	  /* two float destinations + flags + fpc */
	  if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + inib[4]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + inib[6]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  break;

	/* 0xb354-0xb356 undefined */
	/* 0xb35a undefined */

	/* 0xb35c-0xb35e undefined */
	/* 0xb364 undefined */
	/* 0xb368 undefined */

	case 0xb369: /* CXR - compare */
	case 0xb3f4: /* CEDTR - compare biased exponent */
	case 0xb3fc: /* CEXTR - compare biased exponent */
	case 0xb920: /* CGR - compare */
	case 0xb921: /* CLGR - compare logical */
	case 0xb930: /* CGFR - compare */
	case 0xb931: /* CLGFR - compare logical */
	case 0xb9cd: /* CHHR - compare high */
	case 0xb9cf: /* CLHHR - compare logical high */
	case 0xb9dd: /* CHLR - compare high */
	case 0xb9df: /* CLHLR - compare logical high */
	  /* flags only */
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	/* 0xb36a-0xb36f undefined */
	/* 0xb377-0xb37e undefined */
	/* 0xb380-0xb383 undefined */
	/* 0xb386-0xb38b undefined */
	/* 0xb38d-0xb38f undefined */
	/* 0xb393 undefined */
	/* 0xb397 undefined */

	case 0xb398: /* CFEBR - convert to fixed */
	case 0xb399: /* CFDBR - convert to fixed */
	case 0xb39a: /* CFXBR - convert to fixed */
	case 0xb39c: /* CLFEBR - convert to logical */
	case 0xb39d: /* CLFDBR - convert to logical */
	case 0xb39e: /* CLFXBR - convert to logical */
	case 0xb941: /* CFDTR - convert to fixed */
	case 0xb949: /* CFXTR - convert to fixed */
	case 0xb943: /* CLFDTR - convert to logical */
	case 0xb94b: /* CLFXTR - convert to logical */
	  /* 32-bit gpr destination + flags + fpc */
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[6]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  break;

	/* 0xb39b undefined */
	/* 0xb39f undefined */

	/* 0xb3a3 undefined */
	/* 0xb3a7 undefined */

	case 0xb3a8: /* CGEBR - convert to fixed */
	case 0xb3a9: /* CGDBR - convert to fixed */
	case 0xb3aa: /* CGXBR - convert to fixed */
	case 0xb3ac: /* CLGEBR - convert to logical */
	case 0xb3ad: /* CLGDBR - convert to logical */
	case 0xb3ae: /* CLGXBR - convert to logical */
	case 0xb3e1: /* CGDTR - convert to fixed */
	case 0xb3e9: /* CGXTR - convert to fixed */
	case 0xb942: /* CLGDTR - convert to logical */
	case 0xb94a: /* CLGXTR - convert to logical */
	  /* 64-bit gpr destination + flags + fpc */
	  if (s390_record_gpr_g (gdbarch, regcache, inib[6]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  break;

	/* 0xb3ab undefined */
	/* 0xb3af-0xb3b3 undefined */
	/* 0xb3b7 undefined */

	case 0xb3b8: /* CFER - convert to fixed */
	case 0xb3b9: /* CFDR - convert to fixed */
	case 0xb3ba: /* CFXR - convert to fixed */
	case 0xb998: /* ALCR - add logical with carry */
	case 0xb999: /* SLBR - subtract logical with borrow */
	case 0xb9f4: /* NRK - and */
	case 0xb9f5: /* NCRK - and with complement */
	case 0xb9f6: /* ORK - or */
	case 0xb9f7: /* XRK - xor */
	case 0xb9f8: /* ARK - add */
	case 0xb9f9: /* SRK - subtract */
	case 0xb9fa: /* ALRK - add logical */
	case 0xb9fb: /* SLRK - subtract logical */
	  /* 32-bit gpr destination + flags */
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[6]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	case 0xb3c8: /* CGER - convert to fixed */
	case 0xb3c9: /* CGDR - convert to fixed */
	case 0xb3ca: /* CGXR - convert to fixed */
	case 0xb900: /* LPGR - load positive */
	case 0xb901: /* LNGR - load negative */
	case 0xb902: /* LTGR - load and test */
	case 0xb903: /* LCGR - load complement */
	case 0xb908: /* AGR - add */
	case 0xb909: /* SGR - subtract */
	case 0xb90a: /* ALGR - add logical */
	case 0xb90b: /* SLGR - subtract logical */
	case 0xb910: /* LPGFR - load positive */
	case 0xb911: /* LNGFR - load negative */
	case 0xb912: /* LTGFR - load and test */
	case 0xb913: /* LCGFR - load complement */
	case 0xb918: /* AGFR - add */
	case 0xb919: /* SGFR - subtract */
	case 0xb91a: /* ALGFR - add logical */
	case 0xb91b: /* SLGFR - subtract logical */
	case 0xb964: /* NNGRK - and 64 bit */
	case 0xb965: /* OCGRK - or with complement 64 bit */
	case 0xb966: /* NOGRK - or 64 bit */
	case 0xb967: /* NXGRK - not exclusive or 64 bit */
	case 0xb974: /* NNRK - and 32 bit */
	case 0xb975: /* OCRK - or with complement 32 bit */
	case 0xb976: /* NORK - or 32 bit */
	case 0xb977: /* NXRK - not exclusive or 32 bit */
	case 0xb980: /* NGR - and */
	case 0xb981: /* OGR - or */
	case 0xb982: /* XGR - xor */
	case 0xb988: /* ALCGR - add logical with carry */
	case 0xb989: /* SLBGR - subtract logical with borrow */
	case 0xb9c0: /* SELFHR - select high */
	case 0xb9e1: /* POPCNT - population count */
	case 0xb9e4: /* NGRK - and */
	case 0xb9e5: /* NCGRK - and with complement */
	case 0xb9e6: /* OGRK - or */
	case 0xb9e7: /* XGRK - xor */
	case 0xb9e8: /* AGRK - add */
	case 0xb9e9: /* SGRK - subtract */
	case 0xb9ea: /* ALGRK - add logical */
	case 0xb9e3: /* SELGR - select 64 bit */
	case 0xb9eb: /* SLGRK - subtract logical */
	case 0xb9ed: /* MSGRKC - multiply single 64x64 -> 64 */
	case 0xb9f0: /* SELR - select 32 bit */
	case 0xb9fd: /* MSRKC - multiply single 32x32 -> 32 */
	  /* 64-bit gpr destination + flags */
	  if (s390_record_gpr_g (gdbarch, regcache, inib[6]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	/* 0xb3bb-0xb3c0 undefined */
	/* 0xb3c2-0xb3c3 undefined */
	/* 0xb3c7 undefined */
	/* 0xb3cb-0xb3cc undefined */

	case 0xb3cd: /* LGDR - load gr from fpr */
	case 0xb3e2: /* CUDTR - convert to unsigned packed */
	case 0xb3e3: /* CSDTR - convert to signed packed */
	case 0xb3e5: /* EEDTR - extract biased exponent */
	case 0xb3e7: /* ESDTR - extract significance */
	case 0xb3ed: /* EEXTR - extract biased exponent */
	case 0xb3ef: /* ESXTR - extract significance */
	case 0xb904: /* LGR - load */
	case 0xb906: /* LGBR - load byte */
	case 0xb907: /* LGHR - load halfword */
	case 0xb90c: /* MSGR - multiply single */
	case 0xb90f: /* LRVGR - load reversed */
	case 0xb914: /* LGFR - load */
	case 0xb916: /* LLGFR - load logical */
	case 0xb917: /* LLGTR - load logical thirty one bits */
	case 0xb91c: /* MSGFR - multiply single 64<32 */
	case 0xb946: /* BCTGR - branch on count */
	case 0xb984: /* LLGCR - load logical character */
	case 0xb985: /* LLGHR - load logical halfword */
	case 0xb9e2: /* LOCGR - load on condition */
	  /* 64-bit gpr destination  */
	  if (s390_record_gpr_g (gdbarch, regcache, inib[6]))
	    return -1;
	  break;

	/* 0xb3ce-0xb3cf undefined */
	/* 0xb3e6 undefined */

	case 0xb3ea: /* CUXTR - convert to unsigned packed */
	case 0xb3eb: /* CSXTR - convert to signed packed */
	case 0xb90d: /* DSGR - divide single */
	case 0xb91d: /* DSGFR - divide single */
	case 0xb986: /* MLGR - multiply logical */
	case 0xb987: /* DLGR - divide logical */
	case 0xb9ec: /* MGRK - multiply 64x64 -> 128 */
	  /* 64-bit gpr pair destination  */
	  if (s390_record_gpr_g (gdbarch, regcache, inib[6]))
	    return -1;
	  if (s390_record_gpr_g (gdbarch, regcache, inib[6] | 1))
	    return -1;
	  break;

	/* 0xb3ee undefined */
	/* 0xb3f0 undefined */
	/* 0xb3f8 undefined */

	/* 0xb905 privileged */

	/* 0xb90e unsupported: EREGG */

	/* 0xb915 undefined */

	case 0xb91e: /* KMAC - compute message authentication code [partial] */
	  regcache_raw_read_unsigned (regcache, S390_R1_REGNUM, &tmp);
	  oaddr = s390_record_address_mask (gdbarch, regcache, tmp);
	  regcache_raw_read_unsigned (regcache, S390_R0_REGNUM, &tmp);
	  tmp &= 0xff;
	  switch (tmp)
	    {
	      case 0x00: /* KMAC-Query */
		if (record_full_arch_list_add_mem (oaddr, 16))
		  return -1;
		break;

	      case 0x01: /* KMAC-DEA */
	      case 0x02: /* KMAC-TDEA-128 */
	      case 0x03: /* KMAC-TDEA-192 */
	      case 0x09: /* KMAC-Encrypted-DEA */
	      case 0x0a: /* KMAC-Encrypted-TDEA-128 */
	      case 0x0b: /* KMAC-Encrypted-TDEA-192 */
		if (record_full_arch_list_add_mem (oaddr, 8))
		  return -1;
		break;

	      case 0x12: /* KMAC-AES-128 */
	      case 0x13: /* KMAC-AES-192 */
	      case 0x14: /* KMAC-AES-256 */
	      case 0x1a: /* KMAC-Encrypted-AES-128 */
	      case 0x1b: /* KMAC-Encrypted-AES-192 */
	      case 0x1c: /* KMAC-Encrypted-AES-256 */
		if (record_full_arch_list_add_mem (oaddr, 16))
		  return -1;
		break;

	      default:
		gdb_printf (gdb_stdlog, "Warning: Unknown KMAC function %02x at %s.\n",
			    (int)tmp, paddress (gdbarch, addr));
		return -1;
	    }
	  if (tmp != 0)
	    {
	      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[7]))
		return -1;
	      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + (inib[7] | 1)))
		return -1;
	    }
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	/* 0xb922-0xb924 undefined */
	/* 0xb925 privileged */
	/* 0xb928 privileged */

	case 0xb929: /* KMA - cipher message with authentication */
	case 0xb92a: /* KMF - cipher message with cipher feedback [partial] */
	case 0xb92b: /* KMO - cipher message with output feedback [partial] */
	case 0xb92f: /* KMC - cipher message with chaining [partial] */
	  regcache_raw_read_unsigned (regcache, S390_R1_REGNUM, &tmp);
	  oaddr = s390_record_address_mask (gdbarch, regcache, tmp);
	  regcache_raw_read_unsigned (regcache, S390_R0_REGNUM, &tmp);
	  tmp &= 0x7f;
	  switch (tmp)
	    {
	      case 0x00: /* KM*-Query */
		if (record_full_arch_list_add_mem (oaddr, 16))
		  return -1;
		break;

	      case 0x01: /* KM*-DEA */
	      case 0x02: /* KM*-TDEA-128 */
	      case 0x03: /* KM*-TDEA-192 */
	      case 0x09: /* KM*-Encrypted-DEA */
	      case 0x0a: /* KM*-Encrypted-TDEA-128 */
	      case 0x0b: /* KM*-Encrypted-TDEA-192 */
		if (record_full_arch_list_add_mem (oaddr, 8))
		  return -1;
		break;

	      case 0x12: /* KM*-AES-128 */
	      case 0x13: /* KM*-AES-192 */
	      case 0x14: /* KM*-AES-256 */
	      case 0x1a: /* KM*-Encrypted-AES-128 */
	      case 0x1b: /* KM*-Encrypted-AES-192 */
	      case 0x1c: /* KM*-Encrypted-AES-256 */
		if (record_full_arch_list_add_mem (oaddr, 16))
		  return -1;
		break;

	      case 0x43: /* KMC-PRNG */
		/* Only valid for KMC.  */
		if (insn[0] == 0xb92f)
		  {
		    if (record_full_arch_list_add_mem (oaddr, 8))
		      return -1;
		    break;
		  }
		/* For other instructions... */
		[[fallthrough]];
	      default:
		gdb_printf (gdb_stdlog, "Warning: Unknown KM* function %02x at %s.\n",
			    (int)tmp, paddress (gdbarch, addr));
		return -1;
	    }
	  if (tmp != 0)
	    {
	      regcache_raw_read_unsigned (regcache, S390_R0_REGNUM + inib[6], &tmp);
	      oaddr2 = s390_record_address_mask (gdbarch, regcache, tmp);
	      regcache_raw_read_unsigned (regcache, S390_R0_REGNUM + (inib[7] | 1), &tmp);
	      if (record_full_arch_list_add_mem (oaddr2, tmp))
		return -1;
	      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[6]))
		return -1;
	      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[7]))
		return -1;
	      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + (inib[7] | 1)))
		return -1;
	    }
	  if (tmp != 0 && insn[0] == 0xb929)
	    {
	      if (record_full_arch_list_add_reg (regcache,
						 S390_R0_REGNUM + inib[4]))
		return -1;
	      if (record_full_arch_list_add_reg (regcache,
						 S390_R0_REGNUM + (inib[4] | 1)))
		return -1;
	    }
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	case 0xb92c: /* PCC - perform cryptographic computation [partial] */
	  regcache_raw_read_unsigned (regcache, S390_R1_REGNUM, &tmp);
	  oaddr = s390_record_address_mask (gdbarch, regcache, tmp);
	  regcache_raw_read_unsigned (regcache, S390_R0_REGNUM, &tmp);
	  tmp &= 0x7f;
	  switch (tmp)
	    {
	      case 0x00: /* PCC-Query */
		if (record_full_arch_list_add_mem (oaddr, 16))
		  return -1;
		break;

	      case 0x01: /* PCC-Compute-Last-Block-CMAC-Using-DEA */
	      case 0x02: /* PCC-Compute-Last-Block-CMAC-Using-TDEA-128 */
	      case 0x03: /* PCC-Compute-Last-Block-CMAC-Using-TDEA-192 */
	      case 0x09: /* PCC-Compute-Last-Block-CMAC-Using-Encrypted-DEA */
	      case 0x0a: /* PCC-Compute-Last-Block-CMAC-Using-Encrypted-TDEA-128 */
	      case 0x0b: /* PCC-Compute-Last-Block-CMAC-Using-Encrypted-TDEA-192 */
		if (record_full_arch_list_add_mem (oaddr + 0x10, 8))
		  return -1;
		break;

	      case 0x12: /* PCC-Compute-Last-Block-CMAC-Using-AES-128 */
	      case 0x13: /* PCC-Compute-Last-Block-CMAC-Using-AES-192 */
	      case 0x14: /* PCC-Compute-Last-Block-CMAC-Using-AES-256 */
	      case 0x1a: /* PCC-Compute-Last-Block-CMAC-Using-Encrypted-AES-128 */
	      case 0x1b: /* PCC-Compute-Last-Block-CMAC-Using-Encrypted-AES-192 */
	      case 0x1c: /* PCC-Compute-Last-Block-CMAC-Using-Encrypted-AES-256 */
		if (record_full_arch_list_add_mem (oaddr + 0x18, 16))
		  return -1;
		break;

	      case 0x32: /* PCC-Compute-XTS-Parameter-Using-AES-128 */
		if (record_full_arch_list_add_mem (oaddr + 0x30, 32))
		  return -1;
		break;

	      case 0x34: /* PCC-Compute-XTS-Parameter-Using-AES-256 */
		if (record_full_arch_list_add_mem (oaddr + 0x40, 32))
		  return -1;
		break;

	      case 0x3a: /* PCC-Compute-XTS-Parameter-Using-Encrypted-AES-128 */
		if (record_full_arch_list_add_mem (oaddr + 0x50, 32))
		  return -1;
		break;

	      case 0x3c: /* PCC-Compute-XTS-Parameter-Using-Encrypted-AES-256 */
		if (record_full_arch_list_add_mem (oaddr + 0x60, 32))
		  return -1;
		break;

	      default:
		gdb_printf (gdb_stdlog, "Warning: Unknown PCC function %02x at %s.\n",
			    (int)tmp, paddress (gdbarch, addr));
		return -1;
	    }
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	case 0xb92d: /* KMCTR - cipher message with counter [partial] */
	  regcache_raw_read_unsigned (regcache, S390_R1_REGNUM, &tmp);
	  oaddr = s390_record_address_mask (gdbarch, regcache, tmp);
	  regcache_raw_read_unsigned (regcache, S390_R0_REGNUM, &tmp);
	  tmp &= 0x7f;
	  switch (tmp)
	    {
	      case 0x00: /* KMCTR-Query */
		if (record_full_arch_list_add_mem (oaddr, 16))
		  return -1;
		break;

	      case 0x01: /* KMCTR-DEA */
	      case 0x02: /* KMCTR-TDEA-128 */
	      case 0x03: /* KMCTR-TDEA-192 */
	      case 0x09: /* KMCTR-Encrypted-DEA */
	      case 0x0a: /* KMCTR-Encrypted-TDEA-128 */
	      case 0x0b: /* KMCTR-Encrypted-TDEA-192 */
	      case 0x12: /* KMCTR-AES-128 */
	      case 0x13: /* KMCTR-AES-192 */
	      case 0x14: /* KMCTR-AES-256 */
	      case 0x1a: /* KMCTR-Encrypted-AES-128 */
	      case 0x1b: /* KMCTR-Encrypted-AES-192 */
	      case 0x1c: /* KMCTR-Encrypted-AES-256 */
		break;

	      default:
		gdb_printf (gdb_stdlog, "Warning: Unknown KMCTR function %02x at %s.\n",
			    (int)tmp, paddress (gdbarch, addr));
		return -1;
	    }
	  if (tmp != 0)
	    {
	      regcache_raw_read_unsigned (regcache, S390_R0_REGNUM + inib[6], &tmp);
	      oaddr2 = s390_record_address_mask (gdbarch, regcache, tmp);
	      regcache_raw_read_unsigned (regcache, S390_R0_REGNUM + (inib[7] | 1), &tmp);
	      if (record_full_arch_list_add_mem (oaddr2, tmp))
		return -1;
	      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[6]))
		return -1;
	      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[7]))
		return -1;
	      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + (inib[7] | 1)))
		return -1;
	      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[4]))
		return -1;
	    }
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	case 0xb92e: /* KM - cipher message [partial] */
	  regcache_raw_read_unsigned (regcache, S390_R1_REGNUM, &tmp);
	  oaddr = s390_record_address_mask (gdbarch, regcache, tmp);
	  regcache_raw_read_unsigned (regcache, S390_R0_REGNUM, &tmp);
	  tmp &= 0x7f;
	  switch (tmp)
	    {
	      case 0x00: /* KM-Query */
		if (record_full_arch_list_add_mem (oaddr, 16))
		  return -1;
		break;

	      case 0x01: /* KM-DEA */
	      case 0x02: /* KM-TDEA-128 */
	      case 0x03: /* KM-TDEA-192 */
	      case 0x09: /* KM-Encrypted-DEA */
	      case 0x0a: /* KM-Encrypted-TDEA-128 */
	      case 0x0b: /* KM-Encrypted-TDEA-192 */
	      case 0x12: /* KM-AES-128 */
	      case 0x13: /* KM-AES-192 */
	      case 0x14: /* KM-AES-256 */
	      case 0x1a: /* KM-Encrypted-AES-128 */
	      case 0x1b: /* KM-Encrypted-AES-192 */
	      case 0x1c: /* KM-Encrypted-AES-256 */
		break;

	      case 0x32: /* KM-XTS-AES-128 */
		if (record_full_arch_list_add_mem (oaddr + 0x10, 16))
		  return -1;
		break;

	      case 0x34: /* KM-XTS-AES-256 */
		if (record_full_arch_list_add_mem (oaddr + 0x20, 16))
		  return -1;
		break;

	      case 0x3a: /* KM-XTS-Encrypted-AES-128 */
		if (record_full_arch_list_add_mem (oaddr + 0x30, 16))
		  return -1;
		break;

	      case 0x3c: /* KM-XTS-Encrypted-AES-256 */
		if (record_full_arch_list_add_mem (oaddr + 0x40, 16))
		  return -1;
		break;

	      default:
		gdb_printf (gdb_stdlog, "Warning: Unknown KM function %02x at %s.\n",
			    (int)tmp, paddress (gdbarch, addr));
		return -1;
	    }
	  if (tmp != 0)
	    {
	      regcache_raw_read_unsigned (regcache, S390_R0_REGNUM + inib[6], &tmp);
	      oaddr2 = s390_record_address_mask (gdbarch, regcache, tmp);
	      regcache_raw_read_unsigned (regcache, S390_R0_REGNUM + (inib[7] | 1), &tmp);
	      if (record_full_arch_list_add_mem (oaddr2, tmp))
		return -1;
	      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[6]))
		return -1;
	      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[7]))
		return -1;
	      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + (inib[7] | 1)))
		return -1;
	    }
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	/* 0xb932-0xb937 undefined */

	/* 0xb938 unsupported: SORTL - sort lists */
	/* 0xb939 unsupported: DFLTCC - deflate conversion call */
	/* 0xb93a unsupported: KDSA - compute dig. signature auth. */

	/* 0xb93b undefined */

	case 0xb93c: /* PPNO - perform pseudorandom number operation [partial] */
	  regcache_raw_read_unsigned (regcache, S390_R1_REGNUM, &tmp);
	  oaddr = s390_record_address_mask (gdbarch, regcache, tmp);
	  regcache_raw_read_unsigned (regcache, S390_R0_REGNUM, &tmp);
	  tmp &= 0xff;
	  switch (tmp)
	    {
	      case 0x00: /* PPNO-Query */
	      case 0x80: /* PPNO-Query */
		if (record_full_arch_list_add_mem (oaddr, 16))
		  return -1;
		break;

	      case 0x03: /* PPNO-SHA-512-DRNG - generate */
		if (record_full_arch_list_add_mem (oaddr, 240))
		  return -1;
		regcache_raw_read_unsigned (regcache, S390_R0_REGNUM + inib[6], &tmp);
		oaddr2 = s390_record_address_mask (gdbarch, regcache, tmp);
		regcache_raw_read_unsigned (regcache, S390_R0_REGNUM + (inib[6] | 1), &tmp);
		if (record_full_arch_list_add_mem (oaddr2, tmp))
		  return -1;
		if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[6]))
		  return -1;
		if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + (inib[6] | 1)))
		  return -1;
		break;

	      case 0x83: /* PPNO-SHA-512-DRNG - seed */
		if (record_full_arch_list_add_mem (oaddr, 240))
		  return -1;
		if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[7]))
		  return -1;
		if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + (inib[7] | 1)))
		  return -1;
		break;

	      default:
		gdb_printf (gdb_stdlog, "Warning: Unknown PPNO function %02x at %s.\n",
			    (int)tmp, paddress (gdbarch, addr));
		return -1;
	    }
	  /* DXC may be written */
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	/* 0xb93d undefined */

	case 0xb93e: /* KIMD - compute intermediate message digest [partial] */
	case 0xb93f: /* KLMD - compute last message digest [partial] */
	  regcache_raw_read_unsigned (regcache, S390_R1_REGNUM, &tmp);
	  oaddr = s390_record_address_mask (gdbarch, regcache, tmp);
	  regcache_raw_read_unsigned (regcache, S390_R0_REGNUM, &tmp);
	  tmp &= 0xff;
	  switch (tmp)
	    {
	      case 0x00: /* K*MD-Query */
		if (record_full_arch_list_add_mem (oaddr, 16))
		  return -1;
		break;

	      case 0x01: /* K*MD-SHA-1 */
		if (record_full_arch_list_add_mem (oaddr, 20))
		  return -1;
		break;

	      case 0x02: /* K*MD-SHA-256 */
		if (record_full_arch_list_add_mem (oaddr, 32))
		  return -1;
		break;

	      case 0x03: /* K*MD-SHA-512 */
		if (record_full_arch_list_add_mem (oaddr, 64))
		  return -1;
		break;

	      case 0x41: /* KIMD-GHASH */
		/* Only valid for KIMD.  */
		if (insn[0] == 0xb93e)
		  {
		    if (record_full_arch_list_add_mem (oaddr, 16))
		      return -1;
		    break;
		  }
		/* For KLMD...  */
		[[fallthrough]];
	      default:
		gdb_printf (gdb_stdlog, "Warning: Unknown KMAC function %02x at %s.\n",
			    (int)tmp, paddress (gdbarch, addr));
		return -1;
	    }
	  if (tmp != 0)
	    {
	      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[7]))
		return -1;
	      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + (inib[7] | 1)))
		return -1;
	    }
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	/* 0xb940 undefined */
	/* 0xb944-0xb945 undefined */
	/* 0xb947-0xb948 undefined */
	/* 0xb94c-0xb950 undefined */
	/* 0xb954-0xb958 undefined */
	/* 0xb95c-0xb95f undefined */
	/* 0xb962-0xb971 undefined */
	/* 0xb974-0xb97f undefined */

	case 0xb983: /* FLOGR - find leftmost one */
	  /* 64-bit gpr pair destination + flags */
	  if (s390_record_gpr_g (gdbarch, regcache, inib[6]))
	    return -1;
	  if (s390_record_gpr_g (gdbarch, regcache, inib[6] | 1))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	/* 0xb98a privileged */
	/* 0xb98b-0xb98c undefined */

	case 0xb98d: /* EPSW - extract psw */
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[6]))
	    return -1;
	  if (inib[7])
	    if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[7]))
	      return -1;
	  break;

	/* 0xb98e-0xb98f privileged */

	case 0xb990: /* TRTT - translate two to two [partial] */
	case 0xb991: /* TRTO - translate two to one [partial] */
	case 0xb992: /* TROT - translate one to two [partial] */
	case 0xb993: /* TROO - translate one to one [partial] */
	  regcache_raw_read_unsigned (regcache, S390_R0_REGNUM + inib[6], &tmp);
	  oaddr = s390_record_address_mask (gdbarch, regcache, tmp);
	  regcache_raw_read_unsigned (regcache, S390_R0_REGNUM + (inib[6] | 1), &tmp);
	  /* tmp is source length, we want destination length.  Adjust.  */
	  if (insn[0] == 0xb991)
	    tmp >>= 1;
	  if (insn[0] == 0xb992)
	    tmp <<= 1;
	  if (record_full_arch_list_add_mem (oaddr, tmp))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[6]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + (inib[6] | 1)))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[7]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	case 0xb996: /* MLR - multiply logical */
	case 0xb997: /* DLR - divide logical */
	  /* 32-bit gpr pair destination  */
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[6]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + (inib[6] | 1)))
	    return -1;
	  break;

	/* 0xb99a-0xb9af unsupported, privileged, or undefined */
	/* 0xb9b4-0xb9bc undefined */

	case 0xb9bd: /* TRTRE - translate and test reverse extended [partial] */
	case 0xb9bf: /* TRTE - translate and test extended [partial] */
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[6]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + (inib[6] | 1)))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[7]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	/* 0xb9c0-0xb9c7 undefined */

	case 0xb9c8: /* AHHHR - add high */
	case 0xb9c9: /* SHHHR - subtract high */
	case 0xb9ca: /* ALHHHR - add logical high */
	case 0xb9cb: /* SLHHHR - subtract logical high */
	case 0xb9d8: /* AHHLR - add high */
	case 0xb9d9: /* SHHLR - subtract high */
	case 0xb9da: /* ALHHLR - add logical high */
	case 0xb9db: /* SLHHLR - subtract logical high */
	  /* 32-bit high gpr destination + flags */
	  if (s390_record_gpr_h (gdbarch, regcache, inib[6]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	/* 0xb9cc undefined */
	/* 0xb9ce undefined */
	/* 0xb9d0-0xb9d7 undefined */
	/* 0xb9dc undefined */
	/* 0xb9de undefined */

	case 0xb9e0: /* LOCFHR - load high on condition */
	  /* 32-bit high gpr destination */
	  if (s390_record_gpr_h (gdbarch, regcache, inib[6]))
	    return -1;
	  break;

	/* 0xb9e3 undefined */
	/* 0xb9e5 undefined */
	/* 0xb9ee-0xb9f1 undefined */
	/* 0xb9f3 undefined */
	/* 0xb9f5 undefined */
	/* 0xb9fc undefined */
	/* 0xb9fe -0xb9ff undefined */

	default:
	  goto UNKNOWN_OP;
	}
      break;

    /* 0xb4-0xb5 undefined */
    /* 0xb6 privileged: STCTL - store control */
    /* 0xb7 privileged: LCTL - load control */
    /* 0xb8 undefined */

    case 0xba: /* CS - compare and swap */
      oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], 0);
      if (record_full_arch_list_add_mem (oaddr, 4))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[2]))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	return -1;
      break;

    case 0xbb: /* CDS - compare double and swap */
      oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], 0);
      if (record_full_arch_list_add_mem (oaddr, 8))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[2]))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + (inib[2] | 1)))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	return -1;
      break;

    /* 0xbc undefined */

    case 0xbe: /* STCM - store characters under mask */
      oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], 0);
      if (record_full_arch_list_add_mem (oaddr, s390_popcnt (inib[3])))
	return -1;
      break;

    case 0xc0:
    case 0xc2:
    case 0xc4:
    case 0xc6:
    case 0xcc:
      /* RIL-format instruction */
      switch (ibyte[0] << 4 | inib[3])
	{
	case 0xc00: /* LARL - load address relative long */
	case 0xc05: /* BRASL - branch relative and save long */
	case 0xc09: /* IILF - insert immediate */
	case 0xc21: /* MSFI - multiply single immediate */
	case 0xc42: /* LLHRL - load logical halfword relative long */
	case 0xc45: /* LHRL - load halfword relative long */
	case 0xc4d: /* LRL - load relative long */
	  /* 32-bit or native gpr destination */
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[2]))
	    return -1;
	  break;

	case 0xc01: /* LGFI - load immediate */
	case 0xc0e: /* LLIHF - load logical immediate */
	case 0xc0f: /* LLILF - load logical immediate */
	case 0xc20: /* MSGFI - multiply single immediate */
	case 0xc44: /* LGHRL - load halfword relative long */
	case 0xc46: /* LLGHRL - load logical halfword relative long */
	case 0xc48: /* LGRL - load relative long */
	case 0xc4c: /* LGFRL - load relative long */
	case 0xc4e: /* LLGFRL - load logical relative long */
	  /* 64-bit gpr destination */
	  if (s390_record_gpr_g (gdbarch, regcache, inib[2]))
	    return -1;
	  break;

	/* 0xc02-0xc03 undefined */

	case 0xc04: /* BRCL - branch relative on condition long */
	case 0xc62: /* PFDRL - prefetch data relative long */
	  break;

	case 0xc06: /* XIHF - xor immediate */
	case 0xc0a: /* NIHF - and immediate */
	case 0xc0c: /* OIHF - or immediate */
	case 0xcc8: /* AIH - add immediate high */
	case 0xcca: /* ALSIH - add logical with signed immediate high */
	  /* 32-bit high gpr destination + flags */
	  if (s390_record_gpr_h (gdbarch, regcache, inib[2]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	case 0xc07: /* XILF - xor immediate */
	case 0xc0b: /* NILF - and immediate */
	case 0xc0d: /* OILF - or immediate */
	case 0xc25: /* SLFI - subtract logical immediate */
	case 0xc29: /* AFI - add immediate */
	case 0xc2b: /* ALFI - add logical immediate */
	  /* 32-bit gpr destination + flags */
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[2]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	case 0xc08: /* IIHF - insert immediate */
	case 0xcc6: /* BRCTH - branch relative on count high */
	case 0xccb: /* ALSIHN - add logical with signed immediate high */
	  /* 32-bit high gpr destination */
	  if (s390_record_gpr_h (gdbarch, regcache, inib[2]))
	    return -1;
	  break;

	/* 0xc22-0xc23 undefined */

	case 0xc24: /* SLGFI - subtract logical immediate */
	case 0xc28: /* AGFI - add immediate */
	case 0xc2a: /* ALGFI - add logical immediate */
	  /* 64-bit gpr destination + flags */
	  if (s390_record_gpr_g (gdbarch, regcache, inib[2]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	/* 0xc26-0xc27 undefined */

	case 0xc2c: /* CGFI - compare immediate */
	case 0xc2d: /* CFI - compare immediate */
	case 0xc2e: /* CLGFI - compare logical immediate */
	case 0xc2f: /* CLFI - compare logical immediate */
	case 0xc64: /* CGHRL - compare halfword relative long */
	case 0xc65: /* CHRL - compare halfword relative long */
	case 0xc66: /* CLGHRL - compare logical halfword relative long */
	case 0xc67: /* CLHRL - compare logical halfword relative long */
	case 0xc68: /* CGRL - compare relative long */
	case 0xc6a: /* CLGRL - compare logical relative long */
	case 0xc6c: /* CGFRL - compare relative long */
	case 0xc6d: /* CRL - compare relative long */
	case 0xc6e: /* CLGFRL - compare logical relative long */
	case 0xc6f: /* CLRL - compare logical relative long */
	case 0xccd: /* CIH - compare immediate high */
	case 0xccf: /* CLIH - compare logical immediate high */
	  /* flags only */
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	/* 0xc40-0xc41 undefined */
	/* 0xc43 undefined */

	case 0xc47: /* STHRL - store halfword relative long */
	  oaddr = s390_record_calc_rl (gdbarch, regcache, addr, insn[1], insn[2]);
	  if (record_full_arch_list_add_mem (oaddr, 2))
	    return -1;
	  break;

	/* 0xc49-0xc4a undefined */

	case 0xc4b: /* STGRL - store relative long */
	  oaddr = s390_record_calc_rl (gdbarch, regcache, addr, insn[1], insn[2]);
	  if (record_full_arch_list_add_mem (oaddr, 8))
	    return -1;
	  break;

	case 0xc4f: /* STRL - store relative long */
	  oaddr = s390_record_calc_rl (gdbarch, regcache, addr, insn[1], insn[2]);
	  if (record_full_arch_list_add_mem (oaddr, 4))
	    return -1;
	  break;

	case 0xc60: /* EXRL - execute relative long */
	  if (ex != -1)
	    {
	      gdb_printf (gdb_stdlog, "Warning: Double execute at %s.\n",
			  paddress (gdbarch, addr));
	      return -1;
	    }
	  addr = s390_record_calc_rl (gdbarch, regcache, addr, insn[1], insn[2]);
	  if (inib[2])
	    {
	      regcache_raw_read_unsigned (regcache, S390_R0_REGNUM + inib[2], &tmp);
	      ex = tmp & 0xff;
	    }
	  else
	    {
	      ex = 0;
	    }
	  goto ex;

	/* 0xc61 undefined */
	/* 0xc63 undefined */
	/* 0xc69 undefined */
	/* 0xc6b undefined */
	/* 0xcc0-0xcc5 undefined */
	/* 0xcc7 undefined */
	/* 0xcc9 undefined */
	/* 0xccc undefined */
	/* 0xcce undefined */

	default:
	  goto UNKNOWN_OP;
	}
      break;

    /* 0xc1 undefined */
    /* 0xc3 undefined */

    case 0xc5: /* BPRP - branch prediction relative preload */
    case 0xc7: /* BPP - branch prediction preload */
      /* no visible effect */
      break;

    case 0xc8:
      /* SSF-format instruction */
      switch (ibyte[0] << 4 | inib[3])
	{
	/* 0xc80 unsupported */

	case 0xc81: /* ECTG - extract cpu time */
	  if (s390_record_gpr_g (gdbarch, regcache, inib[2]))
	    return -1;
	  if (s390_record_gpr_g (gdbarch, regcache, 0))
	    return -1;
	  if (s390_record_gpr_g (gdbarch, regcache, 1))
	    return -1;
	  break;

	case 0xc82: /* CSST - compare and swap and store */
	  {
	    uint8_t fc, sc;
	    regcache_raw_read_unsigned (regcache, S390_R0_REGNUM, &tmp);
	    fc = tmp & 0xff;
	    sc = tmp >> 8 & 0xff;

	    /* First and third operands.  */
	    oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], 0);
	    switch (fc)
	      {
		case 0x00: /* 32-bit */
		  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[2]))
		    return -1;
		  if (record_full_arch_list_add_mem (oaddr, 4))
		    return -1;
		  break;

		case 0x01: /* 64-bit */
		  if (s390_record_gpr_g (gdbarch, regcache, inib[2]))
		    return -1;
		  if (record_full_arch_list_add_mem (oaddr, 8))
		    return -1;
		  break;

		case 0x02: /* 128-bit */
		  if (s390_record_gpr_g (gdbarch, regcache, inib[2]))
		    return -1;
		  if (s390_record_gpr_g (gdbarch, regcache, inib[2] | 1))
		    return -1;
		  if (record_full_arch_list_add_mem (oaddr, 16))
		    return -1;
		  break;

		default:
		  gdb_printf (gdb_stdlog, "Warning: Unknown CSST FC %02x at %s.\n",
			      fc, paddress (gdbarch, addr));
		  return -1;
	      }

	    /* Second operand.  */
	    oaddr2 = s390_record_calc_disp (gdbarch, regcache, 0, insn[2], 0);
	    if (sc > 4)
	      {
		gdb_printf (gdb_stdlog, "Warning: Unknown CSST FC %02x at %s.\n",
			    sc, paddress (gdbarch, addr));
		return -1;
	      }

	    if (record_full_arch_list_add_mem (oaddr2, 1 << sc))
	      return -1;

	    /* Flags.  */
	    if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	      return -1;
	  }
	  break;

	/* 0xc83 undefined */

	case 0xc84: /* LPD - load pair disjoint */
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[2]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + (inib[2] | 1)))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	case 0xc85: /* LPDG - load pair disjoint */
	  if (s390_record_gpr_g (gdbarch, regcache, inib[2]))
	    return -1;
	  if (s390_record_gpr_g (gdbarch, regcache, inib[2] | 1))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	/* 0xc86-0xc8f undefined */

	default:
	  goto UNKNOWN_OP;
	}
      break;

    /* 0xc9-0xcb undefined */
    /* 0xcd-0xcf undefined */

    case 0xd0: /* TRTR - translate and test reversed */
    case 0xdd: /* TRT - translate and test */
      if (record_full_arch_list_add_reg (regcache, S390_R1_REGNUM))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_R2_REGNUM))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	return -1;
      break;

    case 0xd1: /* MVN - move numbers */
    case 0xd2: /* MVC - move */
    case 0xd3: /* MVZ - move zones */
    case 0xdc: /* TR - translate */
    case 0xe8: /* MVCIN - move inverse */
      oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], 0);
      if (record_full_arch_list_add_mem (oaddr, ibyte[1] + 1))
	return -1;
      break;

    case 0xd4: /* NC - and */
    case 0xd6: /* OC - or*/
    case 0xd7: /* XC - xor */
    case 0xe2: /* UNPKU - unpack unicode */
    case 0xea: /* UNPKA - unpack ASCII */
      oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], 0);
      if (record_full_arch_list_add_mem (oaddr, ibyte[1] + 1))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	return -1;
      break;

    case 0xde: /* ED - edit */
      oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], 0);
      if (record_full_arch_list_add_mem (oaddr, ibyte[1] + 1))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	return -1;
      /* DXC may be written */
      if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	return -1;
      break;

    case 0xdf: /* EDMK - edit and mark */
      oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], 0);
      if (record_full_arch_list_add_mem (oaddr, ibyte[1] + 1))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_R1_REGNUM))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	return -1;
      /* DXC may be written */
      if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	return -1;
      break;

    /* 0xd8 undefined */
    /* 0xd9 unsupported: MVCK - move with key */
    /* 0xda unsupported: MVCP - move to primary */
    /* 0xdb unsupported: MVCS - move to secondary */
    /* 0xe0 undefined */

    case 0xe1: /* PKU - pack unicode */
    case 0xe9: /* PKA - pack ASCII */
      oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], 0);
      if (record_full_arch_list_add_mem (oaddr, 16))
	return -1;
      break;

    case 0xe3:
    case 0xe6:
    case 0xe7:
    case 0xeb:
    case 0xed:
      /* RXY/RXE/RXF/RSL/RSY/SIY/V*-format instruction */
      switch (ibyte[0] << 8 | ibyte[5])
	{
	/* 0xe300-0xe301 undefined */

	case 0xe302: /* LTG - load and test */
	case 0xe308: /* AG - add */
	case 0xe309: /* SG - subtract */
	case 0xe30a: /* ALG - add logical */
	case 0xe30b: /* SLG - subtract logical */
	case 0xe318: /* AGF - add */
	case 0xe319: /* SGF - subtract */
	case 0xe31a: /* ALGF - add logical */
	case 0xe31b: /* SLGF - subtract logical */
	case 0xe332: /* LTGF - load and test */
	case 0xe380: /* NG - and */
	case 0xe381: /* OG - or */
	case 0xe382: /* XG - xor */
	case 0xe388: /* ALCG - add logical with carry */
	case 0xe389: /* SLBG - subtract logical with borrow */
	case 0xeb0a: /* SRAG - shift right single */
	case 0xeb0b: /* SLAG - shift left single */
	  /* 64-bit gpr destination + flags */
	  if (s390_record_gpr_g (gdbarch, regcache, inib[2]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	/* 0xe303 privileged */

	case 0xe304: /* LG - load */
	case 0xe30c: /* MSG - multiply single */
	case 0xe30f: /* LRVG - load reversed */
	case 0xe314: /* LGF - load */
	case 0xe315: /* LGH - load halfword */
	case 0xe316: /* LLGF - load logical */
	case 0xe317: /* LLGT - load logical thirty one bits */
	case 0xe31c: /* MSGF - multiply single */
	case 0xe32a: /* LZRG - load and zero rightmost byte */
	case 0xe33a: /* LLZRGF - load logical and zero rightmost byte */
	case 0xe33c: /* MGH - multiply halfword 64x16mem -> 64 */
	case 0xe346: /* BCTG - branch on count */
	case 0xe377: /* LGB - load byte */
	case 0xe390: /* LLGC - load logical character */
	case 0xe391: /* LLGH - load logical halfword */
	case 0xeb0c: /* SRLG - shift right single logical */
	case 0xeb0d: /* SLLG - shift left single logical */
	case 0xeb1c: /* RLLG - rotate left single logical */
	case 0xeb44: /* BXHG - branch on index high */
	case 0xeb45: /* BXLEG - branch on index low or equal */
	case 0xeb4c: /* ECAG - extract cpu attribute */
	case 0xebe2: /* LOCG - load on condition */
	  /* 64-bit gpr destination */
	  if (s390_record_gpr_g (gdbarch, regcache, inib[2]))
	    return -1;
	  break;

	/* 0xe305 undefined */

	case 0xe306: /* CVBY - convert to binary */
	  /* 32-bit or native gpr destination + FPC (DXC write) */
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[2]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  break;

	/* 0xe307 undefined */

	case 0xe30d: /* DSG - divide single */
	case 0xe31d: /* DSGF - divide single */
	case 0xe384: /* MG - multiply 64x64mem -> 128 */
	case 0xe386: /* MLG - multiply logical */
	case 0xe387: /* DLG - divide logical */
	case 0xe38f: /* LPQ - load pair from quadword */
	  /* 64-bit gpr pair destination  */
	  if (s390_record_gpr_g (gdbarch, regcache, inib[2]))
	    return -1;
	  if (s390_record_gpr_g (gdbarch, regcache, inib[2] | 1))
	    return -1;
	  break;

	case 0xe30e: /* CVBG - convert to binary */
	  /* 64-bit gpr destination + FPC (DXC write) */
	  if (s390_record_gpr_g (gdbarch, regcache, inib[2]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  break;

	/* 0xe310-0xe311 undefined */

	case 0xe312: /* LT - load and test */
	case 0xe338: /* AGH - add halfword to 64 bit value */
	case 0xe339: /* SGH - subtract halfword from 64 bit value */
	case 0xe353: /* MSC - multiply single 32x32mem -> 32 */
	case 0xe354: /* NY - and */
	case 0xe356: /* OY - or */
	case 0xe357: /* XY - xor */
	case 0xe35a: /* AY - add */
	case 0xe35b: /* SY - subtract */
	case 0xe35e: /* ALY - add logical */
	case 0xe35f: /* SLY - subtract logical */
	case 0xe37a: /* AHY - add halfword */
	case 0xe37b: /* SHY - subtract halfword */
	case 0xe383: /* MSGC - multiply single 64x64mem -> 64 */
	case 0xe398: /* ALC - add logical with carry */
	case 0xe399: /* SLB - subtract logical with borrow */
	case 0xe727: /* LCBB - load count to block boundary */
	case 0xeb81: /* ICMY - insert characters under mask */
	case 0xebdc: /* SRAK - shift left single */
	case 0xebdd: /* SLAK - shift left single */
	  /* 32/64-bit gpr destination + flags */
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[2]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	/* 0xe313 privileged */

	case 0xe31e: /* LRV - load reversed */
	case 0xe31f: /* LRVH - load reversed */
	case 0xe33b: /* LZRF - load and zero rightmost byte */
	case 0xe351: /* MSY - multiply single */
	case 0xe358: /* LY - load */
	case 0xe371: /* LAY - load address */
	case 0xe373: /* ICY - insert character */
	case 0xe376: /* LB - load byte */
	case 0xe378: /* LHY - load */
	case 0xe37c: /* MHY - multiply halfword */
	case 0xe394: /* LLC - load logical character */
	case 0xe395: /* LLH - load logical halfword */
	case 0xeb1d: /* RLL - rotate left single logical */
	case 0xebde: /* SRLK - shift left single logical */
	case 0xebdf: /* SLLK - shift left single logical */
	case 0xebf2: /* LOC - load on condition */
	  /* 32-bit or native gpr destination */
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[2]))
	    return -1;
	  break;

	case 0xe320: /* CG - compare */
	case 0xe321: /* CLG - compare logical */
	case 0xe330: /* CGF - compare */
	case 0xe331: /* CLGF - compare logical */
	case 0xe334: /* CGH - compare halfword */
	case 0xe355: /* CLY - compare logical */
	case 0xe359: /* CY - compare */
	case 0xe379: /* CHY - compare halfword */
	case 0xe3cd: /* CHF - compare high */
	case 0xe3cf: /* CLHF - compare logical high */
	case 0xeb20: /* CLMH - compare logical under mask high */
	case 0xeb21: /* CLMY - compare logical under mask */
	case 0xeb51: /* TMY - test under mask */
	case 0xeb55: /* CLIY - compare logical */
	case 0xebc0: /* TP - test decimal */
	case 0xed10: /* TCEB - test data class */
	case 0xed11: /* TCDB - test data class */
	case 0xed12: /* TCXB - test data class */
	case 0xed50: /* TDCET - test data class */
	case 0xed51: /* TDGET - test data group */
	case 0xed54: /* TDCDT - test data class */
	case 0xed55: /* TDGDT - test data group */
	case 0xed58: /* TDCXT - test data class */
	case 0xed59: /* TDGXT - test data group */
	  /* flags only */
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	/* 0xe322-0xe323 undefined */

	case 0xe324: /* STG - store */
	case 0xe325: /* NTSTG - nontransactional store */
	case 0xe326: /* CVDY - convert to decimal */
	case 0xe32f: /* STRVG - store reversed */
	case 0xed67: /* STDY - store */
	  oaddr = s390_record_calc_disp (gdbarch, regcache, inib[3], insn[1], ibyte[4]);
	  if (record_full_arch_list_add_mem (oaddr, 8))
	    return -1;
	  break;

	/* 0xe327-0xe329 undefined */
	/* 0xe32b-0xe32d undefined */

	case 0xe32e: /* CVDG - convert to decimal */
	case 0xe38e: /* STPQ - store pair to quadword */
	  oaddr = s390_record_calc_disp (gdbarch, regcache, inib[3], insn[1], ibyte[4]);
	  if (record_full_arch_list_add_mem (oaddr, 16))
	    return -1;
	  break;

	/* 0xe333 undefined */
	/* 0xe335 undefined */

	case 0xe336: /* PFD - prefetch data */
	  break;

	/* 0xe337 undefined */
	/* 0xe33c-0xe33d undefined */

	case 0xe33e: /* STRV - store reversed */
	case 0xe350: /* STY - store */
	case 0xe3cb: /* STFH - store high */
	case 0xed66: /* STEY - store */
	  oaddr = s390_record_calc_disp (gdbarch, regcache, inib[3], insn[1], ibyte[4]);
	  if (record_full_arch_list_add_mem (oaddr, 4))
	    return -1;
	  break;

	case 0xe33f: /* STRVH - store reversed */
	case 0xe370: /* STHY - store halfword */
	case 0xe3c7: /* STHH - store halfword high */
	  oaddr = s390_record_calc_disp (gdbarch, regcache, inib[3], insn[1], ibyte[4]);
	  if (record_full_arch_list_add_mem (oaddr, 2))
	    return -1;
	  break;

	/* 0xe340-0xe345 undefined */

	case 0xe347: /* BIC - branch indirect on condition */
	  break;

	/* 0xe348-0xe34f undefined */
	/* 0xe352 undefined */

	case 0xe35c: /* MFY - multiply */
	case 0xe396: /* ML - multiply logical */
	case 0xe397: /* DL - divide logical */
	  /* 32-bit gpr pair destination */
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[2]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + (inib[2] | 1)))
	    return -1;
	  break;

	/* 0xe35d undefined */
	/* 0xe360-0xe36f undefined */

	case 0xe372: /* STCY - store character */
	case 0xe3c3: /* STCH - store character high */
	  oaddr = s390_record_calc_disp (gdbarch, regcache, inib[3], insn[1], ibyte[4]);
	  if (record_full_arch_list_add_mem (oaddr, 1))
	    return -1;
	  break;

	/* 0xe374 undefined */

	case 0xe375: /* LAEY - load address extended */
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[2]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_A0_REGNUM + inib[2]))
	    return -1;
	  break;

	/* 0xe37d-0xe37f undefined */

	case 0xe385: /* LGAT - load and trap */
	case 0xe39c: /* LLGTAT - load logical thirty one bits and trap */
	case 0xe39d: /* LLGFAT - load logical and trap */
	case 0xe650: /* VCVB - vector convert to binary 32 bit*/
	case 0xe652: /* VCVBG - vector convert to binary 64 bit*/
	case 0xe721: /* VLGV - vector load gr from vr element */
	  /* 64-bit gpr destination + fpc for possible DXC write */
	  if (s390_record_gpr_g (gdbarch, regcache, inib[2]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  break;

	/* 0xe38a-0xe38d undefined */
	/* 0xe392-0xe393 undefined */
	/* 0xe39a-0xe39b undefined */
	/* 0xe39e undefined */

	case 0xe39f: /* LAT - load and trap */
	  /* 32-bit gpr destination + fpc for possible DXC write */
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[2]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  break;

	/* 0xe3a0-0xe3bf undefined */

	case 0xe3c0: /* LBH - load byte high */
	case 0xe3c2: /* LLCH - load logical character high */
	case 0xe3c4: /* LHH - load halfword high */
	case 0xe3c6: /* LLHH - load logical halfword high */
	case 0xe3ca: /* LFH - load high */
	case 0xebe0: /* LOCFH - load high on condition */
	  /* 32-bit high gpr destination */
	  if (s390_record_gpr_h (gdbarch, regcache, inib[2]))
	    return -1;
	  break;

	/* 0xe3c1 undefined */
	/* 0xe3c5 undefined */

	case 0xe3c8: /* LFHAT - load high and trap */
	  /* 32-bit high gpr destination + fpc for possible DXC write */
	  if (s390_record_gpr_h (gdbarch, regcache, inib[2]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  break;

	/* 0xe3c9 undefined */
	/* 0xe3cc undefined */
	/* 0xe3ce undefined */
	/* 0xe3d0-0xe3ff undefined */

	case 0xe601: /* VLEBRH - vector load byte reversed element */
	case 0xe602: /* VLEBRG - vector load byte reversed element */
	case 0xe603: /* VLEBRF - vector load byte reversed element */
	case 0xe604: /* VLLEBRZ - vector load byte rev. el. and zero */
	case 0xe605: /* VLBRREP - vector load byte rev. el. and replicate */
	case 0xe606: /* VLBR - vector load byte reversed elements */
	case 0xe607: /* VLER - vector load elements reversed */
	case 0xe634: /* VPKZ - vector pack zoned */
	case 0xe635: /* VLRL - vector load rightmost with immed. length */
	case 0xe637: /* VLRLR - vector load rightmost with length */
	case 0xe649: /* VLIP - vector load immediate decimal */
	case 0xe700: /* VLEB - vector load element */
	case 0xe701: /* VLEH - vector load element */
	case 0xe702: /* VLEG - vector load element */
	case 0xe703: /* VLEF - vector load element */
	case 0xe704: /* VLLEZ - vector load logical element and zero */
	case 0xe705: /* VLREP - vector load and replicate */
	case 0xe706: /* VL - vector load */
	case 0xe707: /* VLBB - vector load to block boundary */
	case 0xe712: /* VGEG - vector gather element */
	case 0xe713: /* VGEF - vector gather element */
	case 0xe722: /* VLVG - vector load vr element from gr */
	case 0xe730: /* VESL - vector element shift left */
	case 0xe733: /* VERLL - vector element rotate left logical */
	case 0xe737: /* VLL - vector load with length */
	case 0xe738: /* VESRL - vector element shift right logical */
	case 0xe73a: /* VESRA - vector element shift right arithmetic */
	case 0xe740: /* VLEIB - vector load element immediate */
	case 0xe741: /* VLEIH - vector load element immediate */
	case 0xe742: /* VLEIG - vector load element immediate */
	case 0xe743: /* VLEIF - vector load element immediate */
	case 0xe744: /* VGBM - vector generate byte mask */
	case 0xe745: /* VREPI - vector replicate immediate */
	case 0xe746: /* VGM - vector generate mask */
	case 0xe74d: /* VREP - vector replicate */
	case 0xe750: /* VPOPCT - vector population count */
	case 0xe752: /* VCTZ - vector count trailing zeros */
	case 0xe753: /* VCLZ - vector count leading zeros */
	case 0xe756: /* VLR - vector load */
	case 0xe75f: /* VSEG -vector sign extend to doubleword */
	case 0xe760: /* VMRL - vector merge low */
	case 0xe761: /* VMRH - vector merge high */
	case 0xe762: /* VLVGP - vector load vr from grs disjoint */
	case 0xe764: /* VSUM - vector sum across word */
	case 0xe765: /* VSUMG - vector sum across doubleword */
	case 0xe766: /* VCKSM - vector checksum */
	case 0xe767: /* VSUMQ - vector sum across quadword */
	case 0xe768: /* VN - vector and */
	case 0xe769: /* VNC - vector and with complement */
	case 0xe76a: /* VO - vector or */
	case 0xe76b: /* VNO - vector nor */
	case 0xe76c: /* VNX - vector not exclusive or */
	case 0xe76d: /* VX - vector xor */
	case 0xe76e: /* VNN - vector nand */
	case 0xe76f: /* VOC - vector or with complement */
	case 0xe770: /* VESLV - vector element shift left */
	case 0xe772: /* VERIM - vector element rotate and insert under mask */
	case 0xe773: /* VERLLV - vector element rotate left logical */
	case 0xe774: /* VSL - vector shift left */
	case 0xe775: /* VSLB - vector shift left by byte */
	case 0xe777: /* VSLDB - vector shift left double by byte */
	case 0xe778: /* VESRLV - vector element shift right logical */
	case 0xe77a: /* VESRAV - vector element shift right arithmetic */
	case 0xe77c: /* VSRL - vector shift right logical */
	case 0xe77d: /* VSRLB - vector shift right logical by byte */
	case 0xe77e: /* VSRA - vector shift right arithmetic */
	case 0xe77f: /* VSRAB - vector shift right arithmetic by byte */
	case 0xe784: /* VPDI - vector permute doubleword immediate */
	case 0xe785: /* VBPERM - vector bit permute */
	case 0xe786: /* VSLD - vector shift left double by bit */
	case 0xe787: /* VSRD - vector shift right double by bit */
	case 0xe78b: /* VSTRS - vector string search */
	case 0xe78c: /* VPERM - vector permute */
	case 0xe78d: /* VSEL - vector select */
	case 0xe78e: /* VFMS - vector fp multiply and subtract */
	case 0xe78f: /* VFMA - vector fp multiply and add */
	case 0xe794: /* VPK - vector pack */
	case 0xe79e: /* VFNMS - vector fp negative multiply and subtract */
	case 0xe79f: /* VFNMA - vector fp negative multiply and add */
	case 0xe7a1: /* VMLH - vector multiply logical high */
	case 0xe7a2: /* VML - vector multiply low */
	case 0xe7a3: /* VMH - vector multiply high */
	case 0xe7a4: /* VMLE - vector multiply logical even */
	case 0xe7a5: /* VMLO - vector multiply logical odd */
	case 0xe7a6: /* VME - vector multiply even */
	case 0xe7a7: /* VMO - vector multiply odd */
	case 0xe7a9: /* VMALH - vector multiply and add logical high */
	case 0xe7aa: /* VMAL - vector multiply and add low */
	case 0xe7ab: /* VMAH - vector multiply and add high */
	case 0xe7ac: /* VMALE - vector multiply and add logical even */
	case 0xe7ad: /* VMALO - vector multiply and add logical odd */
	case 0xe7ae: /* VMAE - vector multiply and add even */
	case 0xe7af: /* VMAO - vector multiply and add odd */
	case 0xe7b4: /* VGFM - vector Galois field multiply sum */
	case 0xe7b8: /* VMSL - vector multiply sum logical */
	case 0xe7b9: /* VACCC - vector add with carry compute carry */
	case 0xe7bb: /* VAC - vector add with carry */
	case 0xe7bc: /* VGFMA - vector Galois field multiply sum and accumulate */
	case 0xe7bd: /* VSBCBI - vector subtract with borrow compute borrow indication */
	case 0xe7bf: /* VSBI - vector subtract with borrow indication */
	case 0xe7c0: /* VCLFP - vector fp convert to logical */
	case 0xe7c1: /* VCFPL - vector fp convert from logical */
	case 0xe7c2: /* VCSFP - vector fp convert to fixed */
	case 0xe7c3: /* VCFPS - vector fp convert from fixed */
	case 0xe7c4: /* VLDE/VFLL - vector fp load lengthened */
	case 0xe7c5: /* VLED/VFLR - vector fp load rounded */
	case 0xe7c7: /* VFI - vector load fp integer */
	case 0xe7cc: /* VFPSO - vector fp perform sign operation */
	case 0xe7ce: /* VFSQ - vector fp square root */
	case 0xe7d4: /* VUPLL - vector unpack logical low */
	case 0xe7d6: /* VUPL - vector unpack low */
	case 0xe7d5: /* VUPLH - vector unpack logical high */
	case 0xe7d7: /* VUPH - vector unpack high */
	case 0xe7de: /* VLC - vector load complement */
	case 0xe7df: /* VLP - vector load positive */
	case 0xe7e2: /* VFA - vector fp subtract */
	case 0xe7e3: /* VFA - vector fp add */
	case 0xe7e5: /* VFD - vector fp divide */
	case 0xe7e7: /* VFM - vector fp multiply */
	case 0xe7ee: /* VFMIN - vector fp minimum */
	case 0xe7ef: /* VFMAX - vector fp maximum */
	case 0xe7f0: /* VAVGL - vector average logical */
	case 0xe7f1: /* VACC - vector add and compute carry */
	case 0xe7f2: /* VAVG - vector average */
	case 0xe7f3: /* VA - vector add */
	case 0xe7f5: /* VSCBI - vector subtract compute borrow indication */
	case 0xe7f7: /* VS - vector subtract */
	case 0xe7fc: /* VMNL - vector minimum logical */
	case 0xe7fd: /* VMXL - vector maximum logical */
	case 0xe7fe: /* VMN - vector minimum */
	case 0xe7ff: /* VMX - vector maximum */
	  /* vector destination + FPC */
	  if (s390_record_vr (gdbarch, regcache, ivec[0]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  break;

	case 0xe63d: /* VSTRL - vector store rightmost with immed. length */
	  oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], 0);
	  if (record_full_arch_list_add_mem (oaddr, inib[3] + 1))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  break;

	case 0xe708: /* VSTEB - vector store element */
	  oaddr = s390_record_calc_disp (gdbarch, regcache, inib[3], insn[1], 0);
	  if (record_full_arch_list_add_mem (oaddr, 1))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  break;

	case 0xe609: /* VSTEBRH - vector store byte reversed element */
	case 0xe709: /* VSTEH - vector store element */
	  oaddr = s390_record_calc_disp (gdbarch, regcache, inib[3], insn[1], 0);
	  if (record_full_arch_list_add_mem (oaddr, 2))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  break;

	case 0xe60a: /* VSTEBRG - vector store byte reversed element */
	case 0xe70a: /* VSTEG - vector store element */
	  oaddr = s390_record_calc_disp (gdbarch, regcache, inib[3], insn[1], 0);
	  if (record_full_arch_list_add_mem (oaddr, 8))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  break;

	case 0xe60b: /* VSTEBRF - vector store byte reversed element */
	case 0xe70b: /* VSTEF - vector store element */
	  oaddr = s390_record_calc_disp (gdbarch, regcache, inib[3], insn[1], 0);
	  if (record_full_arch_list_add_mem (oaddr, 4))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  break;

	/* 0xe70c-0xe70d undefined */

	case 0xe60e: /* VSTBR - vector store byte reversed elements */
	case 0xe60f: /* VSTER - vector store elements reversed */
	case 0xe70e: /* VST - vector store */
	  oaddr = s390_record_calc_disp (gdbarch, regcache, inib[3], insn[1], 0);
	  if (record_full_arch_list_add_mem (oaddr, 16))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  break;

	/* 0xe70f-0xe711 undefined */
	/* 0xe714-0xe719 undefined */

	case 0xe71a: /* VSCEG - vector scatter element */
	  if (s390_record_calc_disp_vsce (gdbarch, regcache, ivec[1], inib[8], 8, insn[1], 0, &oaddr))
	    return -1;
	  if (record_full_arch_list_add_mem (oaddr, 8))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  break;

	case 0xe71b: /* VSCEF - vector scatter element */
	  if (s390_record_calc_disp_vsce (gdbarch, regcache, ivec[1], inib[8], 4, insn[1], 0, &oaddr))
	    return -1;
	  if (record_full_arch_list_add_mem (oaddr, 4))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  break;

	/* 0xe71c-0xe720 undefined */
	/* 0xe723-0xe726 undefined */
	/* 0xe728-0xe72f undefined */
	/* 0xe731-0xe732 undefined */
	/* 0xe734-0xe735 undefined */

	case 0xe736: /* VLM - vector load multiple */
	  for (i = ivec[0]; i != ivec[1]; i++, i &= 0x1f)
	    if (s390_record_vr (gdbarch, regcache, i))
	      return -1;
	  if (s390_record_vr (gdbarch, regcache, ivec[1]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  break;

	/* 0xe739 undefined */
	/* 0xe73b-0xe73d undefined */

	case 0xe73e: /* VSTM - vector store multiple */
	  oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], 0);
	  if (ivec[0] <= ivec[1])
	    n = ivec[1] - ivec[0] + 1;
	  else
	    n = ivec[1] + 0x20 - ivec[0] + 1;
	  if (record_full_arch_list_add_mem (oaddr, n * 16))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  break;

	case 0xe63c: /* VUPKZ - vector unpack zoned */
	  oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], 0);
	  if (record_full_arch_list_add_mem (oaddr, (ibyte[1] + 1) & 31))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	case 0xe63f: /* VSTRLR - vector store rightmost with length */
	case 0xe73f: /* VSTL - vector store with length */
	  oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], 0);
	  regcache_raw_read_unsigned (regcache, S390_R0_REGNUM + inib[3], &tmp);
	  tmp &= 0xffffffffu;
	  if (tmp > 15)
	    tmp = 15;
	  if (record_full_arch_list_add_mem (oaddr, tmp + 1))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  break;

	/* 0xe747-0xe749 undefined */

	case 0xe658: /* VCVD - vector convert to decimal 32 bit */
	case 0xe659: /* VSRP - vector shift and round decimal */
	case 0xe65a: /* VCVDG - vector convert to decimal 64 bit*/
	case 0xe65b: /* VPSOP - vector perform sign operation decimal */
	case 0xe671: /* VAP - vector add decimal */
	case 0xe673: /* VSP - vector subtract decimal */
	case 0xe678: /* VMP - vector multiply decimal */
	case 0xe679: /* VMSP - vector multiply decimal */
	case 0xe67a: /* VDP - vector divide decimal */
	case 0xe67b: /* VRP - vector remainder decimal */
	case 0xe67e: /* VSDP - vector shift and divide decimal */
	case 0xe74a: /* VFTCI - vector fp test data class immediate */
	case 0xe75c: /* VISTR - vector isolate string */
	case 0xe780: /* VFEE - vector find element equal */
	case 0xe781: /* VFENE - vector find element not equal */
	case 0xe782: /* VFA - vector find any element equal */
	case 0xe78a: /* VSTRC - vector string range compare */
	case 0xe795: /* VPKLS - vector pack logical saturate */
	case 0xe797: /* VPKS - vector pack saturate */
	case 0xe7e8: /* VFCE - vector fp compare equal */
	case 0xe7ea: /* VFCHE - vector fp compare high or equal */
	case 0xe7eb: /* VFCH - vector fp compare high */
	case 0xe7f8: /* VCEQ - vector compare equal */
	case 0xe7f9: /* VCHL - vector compare high logical */
	case 0xe7fb: /* VCH - vector compare high */
	  /* vector destination + flags + FPC */
	  if (s390_record_vr (gdbarch, regcache, ivec[0]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  break;

	case 0xe65f: /* VTP - vector test decimal */
	  /* flags + FPC */
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  break;

	/* 0xe74b-0xe74c undefined */
	/* 0xe74e-0xe74f undefined */
	/* 0xe751 undefined */
	/* 0xe754-0xe755 undefined */
	/* 0xe757-0xe75b undefined */
	/* 0xe75d-0xe75e undefined */
	/* 0xe763 undefined */
	/* 0xe771 undefined */
	/* 0xe776 undefined */
	/* 0xe779 undefined */
	/* 0xe77b undefined */
	/* 0xe783 undefined */
	/* 0xe786-0xe789 undefined */
	/* 0xe78b undefined */
	/* 0xe790-0xe793 undefined */
	/* 0xe796 undefined */
	/* 0xe798-0xe79d undefined */
	/* 0xe7a0 undefined */
	/* 0xe7a8 undefined */
	/* 0xe7b0-0xe7b3 undefined */
	/* 0xe7b5-0xe7b7 undefined */
	/* 0xe7ba undefined */
	/* 0xe7be undefined */
	/* 0xe7c6 undefined */
	/* 0xe7c8-0xe7c9 undefined */

	case 0xe677: /* VCP - vector compare decimal */
	case 0xe7ca: /* WFK - vector fp compare and signal scalar */
	case 0xe7cb: /* WFC - vector fp compare scalar */
	case 0xe7d8: /* VTM - vector test under mask */
	case 0xe7d9: /* VECL - vector element compare logical */
	case 0xe7db: /* VEC - vector element compare */
	case 0xed08: /* KEB - compare and signal */
	case 0xed09: /* CEB - compare */
	case 0xed18: /* KDB - compare and signal */
	case 0xed19: /* CDB - compare */
	  /* flags + fpc only */
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  break;

	/* 0xe7cd undefined */
	/* 0xe7cf-0xe7d3 undefined */
	/* 0xe7da undefined */
	/* 0xe7dc-0xe7dd undefined */
	/* 0xe7e0-0xe7e1 undefined */
	/* 0xe7e4 undefined */
	/* 0xe7e6 undefined */
	/* 0xe7e9 undefined */
	/* 0xe7ec-0xe7ed undefined */
	/* 0xe7f4 undefined */
	/* 0xe7f6 undefined */
	/* 0xe7fa undefined */

	/* 0xeb00-0xeb03 undefined */

	case 0xeb04: /* LMG - load multiple */
	  for (i = inib[2]; i != inib[3]; i++, i &= 0xf)
	    if (s390_record_gpr_g (gdbarch, regcache, i))
	      return -1;
	  if (s390_record_gpr_g (gdbarch, regcache, inib[3]))
	    return -1;
	  break;

	/* 0xeb05-0xeb09 undefined */
	/* 0xeb0e undefined */
	/* 0xeb0f privileged: TRACG */
	/* 0xeb10-0xeb13 undefined */

	case 0xeb14: /* CSY - compare and swap */
	case 0xebf4: /* LAN - load and and */
	case 0xebf6: /* LAO - load and or */
	case 0xebf7: /* LAX - load and xor */
	case 0xebf8: /* LAA - load and add */
	case 0xebfa: /* LAAL - load and add logical */
	  oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], ibyte[4]);
	  if (record_full_arch_list_add_mem (oaddr, 4))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[2]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	/* 0xeb15-0xeb1b undefined */
	/* 0xeb1e-0xeb1f undefined */
	/* 0xeb22 undefined */

	case 0xeb23: /* CLT - compare logical and trap */
	case 0xeb2b: /* CLGT - compare logical and trap */
	  /* fpc only - including possible DXC write for trapping insns */
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  break;

	case 0xeb24: /* STMG - store multiple */
	  oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], ibyte[4]);
	  if (inib[2] <= inib[3])
	    n = inib[3] - inib[2] + 1;
	  else
	    n = inib[3] + 0x10 - inib[2] + 1;
	  if (record_full_arch_list_add_mem (oaddr, n * 8))
	    return -1;
	  break;

	/* 0xeb25 privileged */

	case 0xeb26: /* STMH - store multiple high */
	case 0xeb90: /* STMY - store multiple */
	case 0xeb9b: /* STAMY - store access multiple */
	  oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], ibyte[4]);
	  if (inib[2] <= inib[3])
	    n = inib[3] - inib[2] + 1;
	  else
	    n = inib[3] + 0x10 - inib[2] + 1;
	  if (record_full_arch_list_add_mem (oaddr, n * 4))
	    return -1;
	  break;

	/* 0xeb27-0xeb2a undefined */

	case 0xeb2c: /* STCMH - store characters under mask */
	case 0xeb2d: /* STCMY - store characters under mask */
	  oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], ibyte[4]);
	  if (record_full_arch_list_add_mem (oaddr, s390_popcnt (inib[3])))
	    return -1;
	  break;

	/* 0xeb2e undefined */
	/* 0xeb2f privileged */

	case 0xeb30: /* CSG - compare and swap */
	case 0xebe4: /* LANG - load and and */
	case 0xebe6: /* LAOG - load and or */
	case 0xebe7: /* LAXG - load and xor */
	case 0xebe8: /* LAAG - load and add */
	case 0xebea: /* LAALG - load and add logical */
	  oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], ibyte[4]);
	  if (record_full_arch_list_add_mem (oaddr, 8))
	    return -1;
	  if (s390_record_gpr_g (gdbarch, regcache, inib[2]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	case 0xeb31: /* CDSY - compare double and swap */
	  oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], ibyte[4]);
	  if (record_full_arch_list_add_mem (oaddr, 8))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[2]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + (inib[2] | 1)))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	/* 0xeb32-0xeb3d undefined */

	case 0xeb3e: /* CDSG - compare double and swap */
	  oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], ibyte[4]);
	  if (record_full_arch_list_add_mem (oaddr, 16))
	    return -1;
	  if (s390_record_gpr_g (gdbarch, regcache, inib[2]))
	    return -1;
	  if (s390_record_gpr_g (gdbarch, regcache, inib[2] | 1))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	/* 0xeb3f-0xeb43 undefined */
	/* 0xeb46-0xeb4b undefined */
	/* 0xeb4d-0xeb50 undefined */

	case 0xeb52: /* MVIY - move */
	  oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], ibyte[4]);
	  if (record_full_arch_list_add_mem (oaddr, 1))
	    return -1;
	  break;

	case 0xeb54: /* NIY - and */
	case 0xeb56: /* OIY - or */
	case 0xeb57: /* XIY - xor */
	  oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], ibyte[4]);
	  if (record_full_arch_list_add_mem (oaddr, 1))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	/* 0xeb53 undefined */
	/* 0xeb58-0xeb69 undefined */

	case 0xeb6a: /* ASI - add immediate */
	case 0xeb6e: /* ALSI - add immediate */
	  oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], ibyte[4]);
	  if (record_full_arch_list_add_mem (oaddr, 4))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	/* 0xeb6b-0xeb6d undefined */
	/* 0xeb6f-0xeb79 undefined */

	case 0xeb7a: /* AGSI - add immediate */
	case 0xeb7e: /* ALGSI - add immediate */
	  oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], ibyte[4]);
	  if (record_full_arch_list_add_mem (oaddr, 8))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	/* 0xeb7b-0xeb7d undefined */
	/* 0xeb7f undefined */

	case 0xeb80: /* ICMH - insert characters under mask */
	  /* 32-bit high gpr destination + flags */
	  if (s390_record_gpr_h (gdbarch, regcache, inib[2]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	/* 0xeb82-0xeb8d undefined */

	case 0xeb8e: /* MVCLU - move long unicode [partial] */
	  regcache_raw_read_unsigned (regcache, S390_R0_REGNUM + inib[2], &tmp);
	  oaddr = s390_record_address_mask (gdbarch, regcache, tmp);
	  regcache_raw_read_unsigned (regcache, S390_R0_REGNUM + (inib[2] | 1), &tmp);
	  if (record_full_arch_list_add_mem (oaddr, tmp))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[2]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + (inib[2] | 1)))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[3]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + (inib[3] | 1)))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	case 0xeb8f: /* CLCLU - compare logical long unicode [partial] */
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[2]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + (inib[2] | 1)))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[3]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + (inib[3] | 1)))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	/* 0xeb91-0xeb95 undefined */

	case 0xeb96: /* LMH - load multiple high */
	  for (i = inib[2]; i != inib[3]; i++, i &= 0xf)
	    if (s390_record_gpr_h (gdbarch, regcache, i))
	      return -1;
	  if (s390_record_gpr_h (gdbarch, regcache, inib[3]))
	    return -1;
	  break;

	/* 0xeb97 undefined */

	case 0xeb98: /* LMY - load multiple */
	  for (i = inib[2]; i != inib[3]; i++, i &= 0xf)
	    if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + i))
	      return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[3]))
	    return -1;
	  break;

	/* 0xeb99 undefined */

	case 0xeb9a: /* LAMY - load access multiple */
	  for (i = inib[2]; i != inib[3]; i++, i &= 0xf)
	    if (record_full_arch_list_add_reg (regcache, S390_A0_REGNUM + i))
	      return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_A0_REGNUM + inib[3]))
	    return -1;
	  break;

	/* 0xeb9c-0xebbf undefined */
	/* 0xebc1-0xebdb undefined */

	case 0xebe1: /* STOCFH - store high on condition */
	case 0xebf3: /* STOC - store on condition */
	  oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], ibyte[4]);
	  if (record_full_arch_list_add_mem (oaddr, 4))
	    return -1;
	  break;

	case 0xebe3: /* STOCG - store on condition */
	  oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], ibyte[4]);
	  if (record_full_arch_list_add_mem (oaddr, 8))
	    return -1;
	  break;

	/* 0xebe5 undefined */
	/* 0xebe9 undefined */
	/* 0xebeb-0xebf1 undefined */
	/* 0xebf5 undefined */
	/* 0xebf9 undefined */
	/* 0xebfb-0xebff undefined */

	/* 0xed00-0xed03 undefined */

	case 0xed04: /* LDEB - load lengthened */
	case 0xed0c: /* MDEB - multiply */
	case 0xed0d: /* DEB - divide */
	case 0xed14: /* SQEB - square root */
	case 0xed15: /* SQDB - square root */
	case 0xed17: /* MEEB - multiply */
	case 0xed1c: /* MDB - multiply */
	case 0xed1d: /* DDB - divide */
	  /* float destination + fpc */
	  if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + inib[2]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  break;

	case 0xed05: /* LXDB - load lengthened */
	case 0xed06: /* LXEB - load lengthened */
	case 0xed07: /* MXDB - multiply */
	  /* float pair destination + fpc */
	  if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + inib[2]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + (inib[2] | 2)))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  break;

	case 0xed0a: /* AEB - add */
	case 0xed0b: /* SEB - subtract */
	case 0xed1a: /* ADB - add */
	case 0xed1b: /* SDB - subtract */
	  /* float destination + flags + fpc */
	  if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + inib[2]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  break;

	case 0xed0e: /* MAEB - multiply and add */
	case 0xed0f: /* MSEB - multiply and subtract */
	case 0xed1e: /* MADB - multiply and add */
	case 0xed1f: /* MSDB - multiply and subtract */
	case 0xed40: /* SLDT - shift significand left */
	case 0xed41: /* SRDT - shift significand right */
	case 0xedaa: /* CDZT - convert from zoned */
	case 0xedae: /* CDPT - convert from packed */
	  /* float destination [RXF] + fpc */
	  if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + inib[8]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  break;

	/* 0xed13 undefined */
	/* 0xed16 undefined */
	/* 0xed20-0xed23 undefined */

	case 0xed24: /* LDE - load lengthened */
	case 0xed34: /* SQE - square root */
	case 0xed35: /* SQD - square root */
	case 0xed37: /* MEE - multiply */
	case 0xed64: /* LEY - load */
	case 0xed65: /* LDY - load */
	  /* float destination */
	  if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + inib[2]))
	    return -1;
	  break;

	case 0xed25: /* LXD - load lengthened */
	case 0xed26: /* LXE - load lengthened */
	  /* float pair destination */
	  if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + inib[2]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + (inib[2] | 2)))
	    return -1;
	  break;

	/* 0xed27-0xed2d undefined */

	case 0xed2e: /* MAE - multiply and add */
	case 0xed2f: /* MSE - multiply and subtract */
	case 0xed38: /* MAYL - multiply and add unnormalized */
	case 0xed39: /* MYL - multiply unnormalized */
	case 0xed3c: /* MAYH - multiply and add unnormalized */
	case 0xed3d: /* MYH - multiply unnormalized */
	case 0xed3e: /* MAD - multiply and add */
	case 0xed3f: /* MSD - multiply and subtract */
	  /* float destination [RXF] */
	  if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + inib[8]))
	    return -1;
	  break;

	/* 0xed30-0xed33 undefined */
	/* 0xed36 undefined */

	case 0xed3a: /* MAY - multiply and add unnormalized */
	case 0xed3b: /* MY - multiply unnormalized */
	  /* float pair destination [RXF] */
	  if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + inib[8]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + (inib[8] | 2)))
	    return -1;
	  break;

	/* 0xed42-0xed47 undefined */

	case 0xed48: /* SLXT - shift significand left */
	case 0xed49: /* SRXT - shift significand right */
	case 0xedab: /* CXZT - convert from zoned */
	case 0xedaf: /* CXPT - convert from packed */
	  /* float pair destination [RXF] + fpc */
	  if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + inib[8]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_F0_REGNUM + (inib[8] | 2)))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  break;

	/* 0xed4a-0xed4f undefined */
	/* 0xed52-0xed53 undefined */
	/* 0xed56-0xed57 undefined */
	/* 0xed5a-0xed63 undefined */
	/* 0xed68-0xeda7 undefined */

	case 0xeda8: /* CZDT - convert to zoned */
	case 0xeda9: /* CZXT - convert to zoned */
	case 0xedac: /* CPDT - convert to packed */
	case 0xedad: /* CPXT - convert to packed */
	  oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], 0);
	  if (record_full_arch_list_add_mem (oaddr, ibyte[1] + 1))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	/* 0xedb0-0xedff undefined */

	default:
	  goto UNKNOWN_OP;
	}
      break;

    /* 0xe4 undefined */

    case 0xe5:
      /* SSE/SIL-format instruction */
      switch (insn[0])
	{
	/* 0xe500-0xe509 undefined, privileged, or unsupported */

	case 0xe50a: /* MVCRL - move right to left */
	  regcache_raw_read_unsigned (regcache, S390_R0_REGNUM, &tmp);
	  oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], 0);
	  if (record_full_arch_list_add_mem (oaddr, (tmp & 0xff) + 1))
	    return -1;
	  break;

	/* 0xe50b-0xe543 undefined, privileged, or unsupported */

	case 0xe544: /* MVHHI - move */
	  oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], 0);
	  if (record_full_arch_list_add_mem (oaddr, 2))
	    return -1;
	  break;

	/* 0xe545-0xe547 undefined */

	case 0xe548: /* MVGHI - move */
	  oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], 0);
	  if (record_full_arch_list_add_mem (oaddr, 8))
	    return -1;
	  break;

	/* 0xe549-0xe54b undefined */

	case 0xe54c: /* MVHI - move */
	  oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], 0);
	  if (record_full_arch_list_add_mem (oaddr, 4))
	    return -1;
	  break;

	/* 0xe54d-0xe553 undefined */

	case 0xe554: /* CHHSI - compare halfword immediate */
	case 0xe555: /* CLHHSI - compare logical immediate */
	case 0xe558: /* CGHSI - compare halfword immediate */
	case 0xe559: /* CLGHSI - compare logical immediate */
	case 0xe55c: /* CHSI - compare halfword immediate */
	case 0xe55d: /* CLFHSI - compare logical immediate */
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	/* 0xe556-0xe557 undefined */
	/* 0xe55a-0xe55b undefined */
	/* 0xe55e-0xe55f undefined */

	case 0xe560: /* TBEGIN - transaction begin */
	  /* The transaction will be immediately aborted after this
	     instruction, due to single-stepping.  This instruction is
	     only supported so that the program can fail a few times
	     and go to the non-transactional fallback.  */
	  if (inib[4])
	    {
	      /* Transaction diagnostic block - user.  */
	      oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], 0);
	      if (record_full_arch_list_add_mem (oaddr, 256))
		return -1;
	    }
	  /* Transaction diagnostic block - supervisor.  */
	  if (record_full_arch_list_add_reg (regcache, S390_TDB_DWORD0_REGNUM))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_TDB_ABORT_CODE_REGNUM))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_TDB_CONFLICT_TOKEN_REGNUM))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_TDB_ATIA_REGNUM))
	    return -1;
	  for (i = 0; i < 16; i++)
	    if (record_full_arch_list_add_reg (regcache, S390_TDB_R0_REGNUM + i))
	      return -1;
	  /* And flags.  */
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	/* 0xe561 unsupported: TBEGINC */
	/* 0xe562-0xe5ff undefined */

	default:
	  goto UNKNOWN_OP;
	}
      break;

    case 0xec:
      /* RIE/RIS/RRS-format instruction */
      switch (ibyte[0] << 8 | ibyte[5])
	{
	/* 0xec00-0xec41 undefined */

	case 0xec42: /* LOCHI - load halfword immediate on condition */
	case 0xec51: /* RISBLG - rotate then insert selected bits low */
	  /* 32-bit or native gpr destination */
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[2]))
	    return -1;
	  break;

	/* 0xec43 undefined */

	case 0xec44: /* BRXHG - branch relative on index high */
	case 0xec45: /* BRXLG - branch relative on index low or equal */
	case 0xec46: /* LOCGHI - load halfword immediate on condition */
	case 0xec59: /* RISBGN - rotate then insert selected bits */
	  /* 64-bit gpr destination */
	  if (s390_record_gpr_g (gdbarch, regcache, inib[2]))
	    return -1;
	  break;

	/* 0xec47-0xec4d undefined */

	case 0xec4e: /* LOCHHI - load halfword immediate on condition */
	case 0xec5d: /* RISBHG - rotate then insert selected bits high */
	  /* 32-bit high gpr destination */
	  if (s390_record_gpr_h (gdbarch, regcache, inib[2]))
	    return -1;
	  break;

	/* 0xec4f-0xec50 undefined */
	/* 0xec52-0xec53 undefined */

	case 0xec54: /* RNSBG - rotate then and selected bits */
	case 0xec55: /* RISBG - rotate then insert selected bits */
	case 0xec56: /* ROSBG - rotate then or selected bits */
	case 0xec57: /* RXSBG - rotate then xor selected bits */
	case 0xecd9: /* AGHIK - add immediate */
	case 0xecdb: /* ALGHSIK - add logical immediate */
	  /* 64-bit gpr destination + flags */
	  if (s390_record_gpr_g (gdbarch, regcache, inib[2]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	/* 0xec58 undefined */
	/* 0xec5a-0xec5c undefined */
	/* 0xec5e-0xec63 undefined */

	case 0xec64: /* CGRJ - compare and branch relative */
	case 0xec65: /* CLGRJ - compare logical and branch relative */
	case 0xec76: /* CRJ - compare and branch relative */
	case 0xec77: /* CLRJ - compare logical and branch relative */
	case 0xec7c: /* CGIJ - compare immediate and branch relative */
	case 0xec7d: /* CLGIJ - compare logical immediate and branch relative */
	case 0xec7e: /* CIJ - compare immediate and branch relative */
	case 0xec7f: /* CLIJ - compare logical immediate and branch relative */
	case 0xece4: /* CGRB - compare and branch */
	case 0xece5: /* CLGRB - compare logical and branch */
	case 0xecf6: /* CRB - compare and branch */
	case 0xecf7: /* CLRB - compare logical and branch */
	case 0xecfc: /* CGIB - compare immediate and branch */
	case 0xecfd: /* CLGIB - compare logical immediate and branch */
	case 0xecfe: /* CIB - compare immediate and branch */
	case 0xecff: /* CLIB - compare logical immediate and branch */
	  break;

	/* 0xec66-0xec6f undefined */

	case 0xec70: /* CGIT - compare immediate and trap */
	case 0xec71: /* CLGIT - compare logical immediate and trap */
	case 0xec72: /* CIT - compare immediate and trap */
	case 0xec73: /* CLFIT - compare logical immediate and trap */
	  /* fpc only - including possible DXC write for trapping insns */
	  if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	    return -1;
	  break;

	/* 0xec74-0xec75 undefined */
	/* 0xec78-0xec7b undefined */

	/* 0xec80-0xecd7 undefined */

	case 0xecd8: /* AHIK - add immediate */
	case 0xecda: /* ALHSIK - add logical immediate */
	  /* 32-bit gpr destination + flags */
	  if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[2]))
	    return -1;
	  if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	    return -1;
	  break;

	/* 0xecdc-0xece3 undefined */
	/* 0xece6-0xecf5 undefined */
	/* 0xecf8-0xecfb undefined */

	default:
	  goto UNKNOWN_OP;
	}
      break;

    case 0xee: /* PLO - perform locked operation */
      regcache_raw_read_unsigned (regcache, S390_R0_REGNUM, &tmp);
      oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], 0);
      oaddr2 = s390_record_calc_disp (gdbarch, regcache, 0, insn[2], 0);
      if (!(tmp & 0x100))
	{
	  uint8_t fc = tmp & 0xff;
	  gdb_byte buf[8];
	  switch (fc)
	    {
	    case 0x00: /* CL */
	      /* op1c */
	      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[2]))
		return -1;
	      /* op3 */
	      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[3]))
		return -1;
	      break;

	    case 0x01: /* CLG */
	      /* op1c */
	      if (record_full_arch_list_add_mem (oaddr2 + 0x08, 8))
		return -1;
	      /* op3 */
	      if (record_full_arch_list_add_mem (oaddr2 + 0x28, 8))
		return -1;
	      break;

	    case 0x02: /* CLGR */
	      /* op1c */
	      if (s390_record_gpr_g (gdbarch, regcache, inib[2]))
		return -1;
	      /* op3 */
	      if (s390_record_gpr_g (gdbarch, regcache, inib[3]))
		return -1;
	      break;

	    case 0x03: /* CLX */
	      /* op1c */
	      if (record_full_arch_list_add_mem (oaddr2 + 0x00, 16))
		return -1;
	      /* op3 */
	      if (record_full_arch_list_add_mem (oaddr2 + 0x20, 16))
		return -1;
	      break;

	    case 0x08: /* DCS */
	      /* op3c */
	      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[3]))
		return -1;
	      [[fallthrough]];
	    case 0x0c: /* CSST */
	      /* op4 */
	      if (record_full_arch_list_add_mem (oaddr2, 4))
		return -1;
	      goto CS;

	    case 0x14: /* CSTST */
	      /* op8 */
	      if (target_read_memory (oaddr2 + 0x88, buf, 8))
		return -1;
	      oaddr3 = extract_unsigned_integer (buf, 8, byte_order);
	      oaddr3 = s390_record_address_mask (gdbarch, regcache, oaddr3);
	      if (record_full_arch_list_add_mem (oaddr3, 4))
		return -1;
	      [[fallthrough]];
	    case 0x10: /* CSDST */
	      /* op6 */
	      if (target_read_memory (oaddr2 + 0x68, buf, 8))
		return -1;
	      oaddr3 = extract_unsigned_integer (buf, 8, byte_order);
	      oaddr3 = s390_record_address_mask (gdbarch, regcache, oaddr3);
	      if (record_full_arch_list_add_mem (oaddr3, 4))
		return -1;
	      /* op4 */
	      if (target_read_memory (oaddr2 + 0x48, buf, 8))
		return -1;
	      oaddr3 = extract_unsigned_integer (buf, 8, byte_order);
	      oaddr3 = s390_record_address_mask (gdbarch, regcache, oaddr3);
	      if (record_full_arch_list_add_mem (oaddr3, 4))
		return -1;
	      [[fallthrough]];
	    case 0x04: /* CS */
CS:
	      /* op1c */
	      if (record_full_arch_list_add_reg (regcache, S390_R0_REGNUM + inib[2]))
		return -1;
	      /* op2 */
	      if (record_full_arch_list_add_mem (oaddr, 4))
		return -1;
	      break;

	    case 0x09: /* DCSG */
	      /* op3c */
	      if (record_full_arch_list_add_mem (oaddr2 + 0x28, 8))
		return -1;
	      goto CSSTG;

	    case 0x15: /* CSTSTG */
	      /* op8 */
	      if (target_read_memory (oaddr2 + 0x88, buf, 8))
		return -1;
	      oaddr3 = extract_unsigned_integer (buf, 8, byte_order);
	      oaddr3 = s390_record_address_mask (gdbarch, regcache, oaddr3);
	      if (record_full_arch_list_add_mem (oaddr3, 8))
		return -1;
	      [[fallthrough]];
	    case 0x11: /* CSDSTG */
	      /* op6 */
	      if (target_read_memory (oaddr2 + 0x68, buf, 8))
		return -1;
	      oaddr3 = extract_unsigned_integer (buf, 8, byte_order);
	      oaddr3 = s390_record_address_mask (gdbarch, regcache, oaddr3);
	      if (record_full_arch_list_add_mem (oaddr3, 8))
		return -1;
	      [[fallthrough]];
	    case 0x0d: /* CSSTG */
CSSTG:
	      /* op4 */
	      if (target_read_memory (oaddr2 + 0x48, buf, 8))
		return -1;
	      oaddr3 = extract_unsigned_integer (buf, 8, byte_order);
	      oaddr3 = s390_record_address_mask (gdbarch, regcache, oaddr3);
	      if (record_full_arch_list_add_mem (oaddr3, 8))
		return -1;
	      [[fallthrough]];
	    case 0x05: /* CSG */
	      /* op1c */
	      if (record_full_arch_list_add_mem (oaddr2 + 0x08, 8))
		return -1;
	      /* op2 */
	      if (record_full_arch_list_add_mem (oaddr, 8))
		return -1;
	      break;

	    case 0x0a: /* DCSGR */
	      /* op3c */
	      if (s390_record_gpr_g (gdbarch, regcache, inib[3]))
		return -1;
	      [[fallthrough]];
	    case 0x0e: /* CSSTGR */
	      /* op4 */
	      if (record_full_arch_list_add_mem (oaddr2, 8))
		return -1;
	      goto CSGR;

	    case 0x16: /* CSTSTGR */
	      /* op8 */
	      if (target_read_memory (oaddr2 + 0x88, buf, 8))
		return -1;
	      oaddr3 = extract_unsigned_integer (buf, 8, byte_order);
	      oaddr3 = s390_record_address_mask (gdbarch, regcache, oaddr3);
	      if (record_full_arch_list_add_mem (oaddr3, 8))
		return -1;
	      [[fallthrough]];
	    case 0x12: /* CSDSTGR */
	      /* op6 */
	      if (target_read_memory (oaddr2 + 0x68, buf, 8))
		return -1;
	      oaddr3 = extract_unsigned_integer (buf, 8, byte_order);
	      oaddr3 = s390_record_address_mask (gdbarch, regcache, oaddr3);
	      if (record_full_arch_list_add_mem (oaddr3, 8))
		return -1;
	      /* op4 */
	      if (target_read_memory (oaddr2 + 0x48, buf, 8))
		return -1;
	      oaddr3 = extract_unsigned_integer (buf, 8, byte_order);
	      oaddr3 = s390_record_address_mask (gdbarch, regcache, oaddr3);
	      if (record_full_arch_list_add_mem (oaddr3, 8))
		return -1;
	      [[fallthrough]];
	    case 0x06: /* CSGR */
CSGR:
	      /* op1c */
	      if (s390_record_gpr_g (gdbarch, regcache, inib[2]))
		return -1;
	      /* op2 */
	      if (record_full_arch_list_add_mem (oaddr, 8))
		return -1;
	      break;

	    case 0x0b: /* DCSX */
	      /* op3c */
	      if (record_full_arch_list_add_mem (oaddr2 + 0x20, 16))
		return -1;
	      goto CSSTX;

	    case 0x17: /* CSTSTX */
	      /* op8 */
	      if (target_read_memory (oaddr2 + 0x88, buf, 8))
		return -1;
	      oaddr3 = extract_unsigned_integer (buf, 8, byte_order);
	      oaddr3 = s390_record_address_mask (gdbarch, regcache, oaddr3);
	      if (record_full_arch_list_add_mem (oaddr3, 16))
		return -1;
	      [[fallthrough]];
	    case 0x13: /* CSDSTX */
	      /* op6 */
	      if (target_read_memory (oaddr2 + 0x68, buf, 8))
		return -1;
	      oaddr3 = extract_unsigned_integer (buf, 8, byte_order);
	      oaddr3 = s390_record_address_mask (gdbarch, regcache, oaddr3);
	      if (record_full_arch_list_add_mem (oaddr3, 16))
		return -1;
	      [[fallthrough]];
	    case 0x0f: /* CSSTX */
CSSTX:
	      /* op4 */
	      if (target_read_memory (oaddr2 + 0x48, buf, 8))
		return -1;
	      oaddr3 = extract_unsigned_integer (buf, 8, byte_order);
	      oaddr3 = s390_record_address_mask (gdbarch, regcache, oaddr3);
	      if (record_full_arch_list_add_mem (oaddr3, 16))
		return -1;
	      [[fallthrough]];
	    case 0x07: /* CSX */
	      /* op1c */
	      if (record_full_arch_list_add_mem (oaddr2 + 0x00, 16))
		return -1;
	      /* op2 */
	      if (record_full_arch_list_add_mem (oaddr, 16))
		return -1;
	      break;

	    default:
	      gdb_printf (gdb_stdlog, "Warning: Unknown PLO FC %02x at %s.\n",
			  fc, paddress (gdbarch, addr));
	      return -1;
	    }
	}
      if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	return -1;
      break;

    case 0xef: /* LMD - load multiple disjoint */
      for (i = inib[2]; i != inib[3]; i++, i &= 0xf)
	if (s390_record_gpr_g (gdbarch, regcache, i))
	  return -1;
      if (s390_record_gpr_g (gdbarch, regcache, inib[3]))
	return -1;
      break;

    case 0xf0: /* SRP - shift and round decimal */
    case 0xf8: /* ZAP - zero and add */
    case 0xfa: /* AP - add decimal */
    case 0xfb: /* SP - subtract decimal */
      oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], 0);
      if (record_full_arch_list_add_mem (oaddr, inib[2] + 1))
	return -1;
      if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	return -1;
      /* DXC may be written */
      if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	return -1;
      break;

    case 0xf1: /* MVO - move with offset */
    case 0xf2: /* PACK - pack */
    case 0xf3: /* UNPK - unpack */
      oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], 0);
      if (record_full_arch_list_add_mem (oaddr, inib[2] + 1))
	return -1;
      break;

    /* 0xf4-0xf7 undefined */

    case 0xf9: /* CP - compare decimal */
      if (record_full_arch_list_add_reg (regcache, S390_PSWM_REGNUM))
	return -1;
      /* DXC may be written */
      if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	return -1;
      break;

    case 0xfc: /* MP - multiply decimal */
    case 0xfd: /* DP - divide decimal */
      oaddr = s390_record_calc_disp (gdbarch, regcache, 0, insn[1], 0);
      if (record_full_arch_list_add_mem (oaddr, inib[2] + 1))
	return -1;
      /* DXC may be written */
      if (record_full_arch_list_add_reg (regcache, S390_FPC_REGNUM))
	return -1;
      break;

    /* 0xfe-0xff undefined */

    default:
UNKNOWN_OP:
      gdb_printf (gdb_stdlog, "Warning: Don't know how to record %04x "
		  "at %s.\n", insn[0], paddress (gdbarch, addr));
      return -1;
  }

  if (record_full_arch_list_add_reg (regcache, S390_PSWA_REGNUM))
    return -1;
  if (record_full_arch_list_add_end ())
    return -1;
  return 0;
}

/* Miscellaneous.  */

/* Implement gdbarch_gcc_target_options.  GCC does not know "-m32" or
   "-mcmodel=large".  */

static std::string
s390_gcc_target_options (struct gdbarch *gdbarch)
{
  return gdbarch_ptr_bit (gdbarch) == 64 ? "-m64" : "-m31";
}

/* Implement gdbarch_gnu_triplet_regexp.  Target triplets are "s390-*"
   for 31-bit and "s390x-*" for 64-bit, while the BFD arch name is
   always "s390".  Note that an s390x compiler supports "-m31" as
   well.  */

static const char *
s390_gnu_triplet_regexp (struct gdbarch *gdbarch)
{
  return "s390x?";
}

/* Implementation of `gdbarch_stap_is_single_operand', as defined in
   gdbarch.h.  */

static int
s390_stap_is_single_operand (struct gdbarch *gdbarch, const char *s)
{
  return ((isdigit (*s) && s[1] == '(' && s[2] == '%') /* Displacement
							  or indirection.  */
	  || *s == '%' /* Register access.  */
	  || isdigit (*s)); /* Literal number.  */
}

/* gdbarch init.  */

/* Validate the range of registers.  NAMES must be known at compile time.  */

#define s390_validate_reg_range(feature, tdesc_data, start, names)	\
do									\
{									\
  for (int i = 0; i < ARRAY_SIZE (names); i++)				\
    if (!tdesc_numbered_register (feature, tdesc_data, start + i, names[i])) \
      return false;							\
}									\
while (0)

/* Validate the target description.  Also numbers registers contained in
   tdesc.  */

static bool
s390_tdesc_valid (s390_gdbarch_tdep *tdep,
		  struct tdesc_arch_data *tdesc_data)
{
  static const char *const psw[] = {
    "pswm", "pswa"
  };
  static const char *const gprs[] = {
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
  };
  static const char *const fprs[] = {
    "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7",
    "f8", "f9", "f10", "f11", "f12", "f13", "f14", "f15"
  };
  static const char *const acrs[] = {
    "acr0", "acr1", "acr2", "acr3", "acr4", "acr5", "acr6", "acr7",
    "acr8", "acr9", "acr10", "acr11", "acr12", "acr13", "acr14", "acr15"
  };
  static const char *const gprs_lower[] = {
    "r0l", "r1l", "r2l", "r3l", "r4l", "r5l", "r6l", "r7l",
    "r8l", "r9l", "r10l", "r11l", "r12l", "r13l", "r14l", "r15l"
  };
  static const char *const gprs_upper[] = {
    "r0h", "r1h", "r2h", "r3h", "r4h", "r5h", "r6h", "r7h",
    "r8h", "r9h", "r10h", "r11h", "r12h", "r13h", "r14h", "r15h"
  };
  static const char *const tdb_regs[] = {
    "tdb0", "tac", "tct", "atia",
    "tr0", "tr1", "tr2", "tr3", "tr4", "tr5", "tr6", "tr7",
    "tr8", "tr9", "tr10", "tr11", "tr12", "tr13", "tr14", "tr15"
  };
  static const char *const vxrs_low[] = {
    "v0l", "v1l", "v2l", "v3l", "v4l", "v5l", "v6l", "v7l", "v8l",
    "v9l", "v10l", "v11l", "v12l", "v13l", "v14l", "v15l",
  };
  static const char *const vxrs_high[] = {
    "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24",
    "v25", "v26", "v27", "v28", "v29", "v30", "v31",
  };
  static const char *const gs_cb[] = {
    "gsd", "gssm", "gsepla",
  };
  static const char *const gs_bc[] = {
    "bc_gsd", "bc_gssm", "bc_gsepla",
  };

  const struct target_desc *tdesc = tdep->tdesc;
  const struct tdesc_feature *feature;

  if (!tdesc_has_registers (tdesc))
    return false;

  /* Core registers, i.e. general purpose and PSW.  */
  feature = tdesc_find_feature (tdesc, "org.gnu.gdb.s390.core");
  if (feature == NULL)
    return false;

  s390_validate_reg_range (feature, tdesc_data, S390_PSWM_REGNUM, psw);

  if (tdesc_unnumbered_register (feature, "r0"))
    {
      s390_validate_reg_range (feature, tdesc_data, S390_R0_REGNUM, gprs);
    }
  else
    {
      tdep->have_upper = true;
      s390_validate_reg_range (feature, tdesc_data, S390_R0_REGNUM,
			       gprs_lower);
      s390_validate_reg_range (feature, tdesc_data, S390_R0_UPPER_REGNUM,
			       gprs_upper);
    }

  /* Floating point registers.  */
  feature = tdesc_find_feature (tdesc, "org.gnu.gdb.s390.fpr");
  if (feature == NULL)
    return false;

  if (!tdesc_numbered_register (feature, tdesc_data, S390_FPC_REGNUM, "fpc"))
    return false;

  s390_validate_reg_range (feature, tdesc_data, S390_F0_REGNUM, fprs);

  /* Access control registers.  */
  feature = tdesc_find_feature (tdesc, "org.gnu.gdb.s390.acr");
  if (feature == NULL)
    return false;

  s390_validate_reg_range (feature, tdesc_data, S390_A0_REGNUM, acrs);

  /* Optional GNU/Linux-specific "registers".  */
  feature = tdesc_find_feature (tdesc, "org.gnu.gdb.s390.linux");
  if (feature)
    {
      tdesc_numbered_register (feature, tdesc_data,
			       S390_ORIG_R2_REGNUM, "orig_r2");

      if (tdesc_numbered_register (feature, tdesc_data,
				   S390_LAST_BREAK_REGNUM, "last_break"))
	tdep->have_linux_v1 = true;

      if (tdesc_numbered_register (feature, tdesc_data,
				   S390_SYSTEM_CALL_REGNUM, "system_call"))
	tdep->have_linux_v2 = true;

      if (tdep->have_linux_v2 && !tdep->have_linux_v1)
	return false;
    }

  /* Transaction diagnostic block.  */
  feature = tdesc_find_feature (tdesc, "org.gnu.gdb.s390.tdb");
  if (feature)
    {
      s390_validate_reg_range (feature, tdesc_data, S390_TDB_DWORD0_REGNUM,
			       tdb_regs);
      tdep->have_tdb = true;
    }

  /* Vector registers.  */
  feature = tdesc_find_feature (tdesc, "org.gnu.gdb.s390.vx");
  if (feature)
    {
      s390_validate_reg_range (feature, tdesc_data, S390_V0_LOWER_REGNUM,
			       vxrs_low);
      s390_validate_reg_range (feature, tdesc_data, S390_V16_REGNUM,
			       vxrs_high);
      tdep->have_vx = true;
    }

  /* Guarded-storage registers.  */
  feature = tdesc_find_feature (tdesc, "org.gnu.gdb.s390.gs");
  if (feature)
    {
      s390_validate_reg_range (feature, tdesc_data, S390_GSD_REGNUM, gs_cb);
      tdep->have_gs = true;
    }

  /* Guarded-storage broadcast control.  */
  feature = tdesc_find_feature (tdesc, "org.gnu.gdb.s390.gsbc");
  if (feature)
    {
      if (!tdep->have_gs)
	return false;
      s390_validate_reg_range (feature, tdesc_data, S390_BC_GSD_REGNUM,
			       gs_bc);
    }

  return true;
}

/* Allocate and initialize new gdbarch_tdep.  */

static s390_gdbarch_tdep_up
s390_gdbarch_tdep_alloc ()
{
  s390_gdbarch_tdep_up tdep (new s390_gdbarch_tdep);

  tdep->tdesc = NULL;

  tdep->abi = ABI_NONE;
  tdep->vector_abi = S390_VECTOR_ABI_NONE;

  tdep->gpr_full_regnum = -1;
  tdep->v0_full_regnum = -1;
  tdep->pc_regnum = -1;
  tdep->cc_regnum = -1;

  tdep->have_upper = false;
  tdep->have_linux_v1 = false;
  tdep->have_linux_v2 = false;
  tdep->have_tdb = false;
  tdep->have_vx = false;
  tdep->have_gs = false;

  tdep->s390_syscall_record = NULL;

  return tdep;
}

/* Set up gdbarch struct.  */

static struct gdbarch *
s390_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  const struct target_desc *tdesc = info.target_desc;
  int first_pseudo_reg, last_pseudo_reg;
  static const char *const stap_register_prefixes[] = { "%", NULL };
  static const char *const stap_register_indirection_prefixes[] = { "(",
								    NULL };
  static const char *const stap_register_indirection_suffixes[] = { ")",
								    NULL };

  gdbarch *gdbarch = gdbarch_alloc (&info, s390_gdbarch_tdep_alloc ());
  s390_gdbarch_tdep *tdep = gdbarch_tdep<s390_gdbarch_tdep> (gdbarch);
  tdesc_arch_data_up tdesc_data = tdesc_data_alloc ();
  info.tdesc_data = tdesc_data.get ();

  set_gdbarch_believe_pcc_promotion (gdbarch, 0);
  set_gdbarch_char_signed (gdbarch, 0);

  /* S/390 GNU/Linux uses either 64-bit or 128-bit long doubles.
     We can safely let them default to 128-bit, since the debug info
     will give the size of type actually used in each case.  */
  set_gdbarch_long_double_bit (gdbarch, 128);
  set_gdbarch_long_double_format (gdbarch, floatformats_ieee_quad);

  set_gdbarch_type_align (gdbarch, s390_type_align);

  /* Breakpoints.  */
  /* Amount PC must be decremented by after a breakpoint.  This is
     often the number of bytes returned by gdbarch_breakpoint_from_pc but not
     always.  */
  set_gdbarch_decr_pc_after_break (gdbarch, 2);
  set_gdbarch_breakpoint_kind_from_pc (gdbarch, s390_breakpoint::kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch, s390_breakpoint::bp_from_kind);

  /* Displaced stepping.  */
  set_gdbarch_displaced_step_copy_insn (gdbarch,
					s390_displaced_step_copy_insn);
  set_gdbarch_displaced_step_fixup (gdbarch, s390_displaced_step_fixup);
  set_gdbarch_displaced_step_hw_singlestep (gdbarch, s390_displaced_step_hw_singlestep);
  set_gdbarch_software_single_step (gdbarch, s390_software_single_step);
  set_gdbarch_max_insn_length (gdbarch, S390_MAX_INSTR_SIZE);

  /* Prologue analysis.  */
  set_gdbarch_skip_prologue (gdbarch, s390_skip_prologue);

  /* Register handling.  */
  set_gdbarch_num_regs (gdbarch, S390_NUM_REGS);
  set_gdbarch_sp_regnum (gdbarch, S390_SP_REGNUM);
  set_gdbarch_fp0_regnum (gdbarch, S390_F0_REGNUM);
  set_gdbarch_guess_tracepoint_registers (gdbarch,
					  s390_guess_tracepoint_registers);
  set_gdbarch_stab_reg_to_regnum (gdbarch, s390_dwarf_reg_to_regnum);
  set_gdbarch_dwarf2_reg_to_regnum (gdbarch, s390_dwarf_reg_to_regnum);
  set_gdbarch_value_from_register (gdbarch, s390_value_from_register);

  /* Pseudo registers.  */
  set_gdbarch_pseudo_register_read (gdbarch, s390_pseudo_register_read);
  set_gdbarch_deprecated_pseudo_register_write (gdbarch,
						s390_pseudo_register_write);
  set_tdesc_pseudo_register_name (gdbarch, s390_pseudo_register_name);
  set_tdesc_pseudo_register_type (gdbarch, s390_pseudo_register_type);
  set_tdesc_pseudo_register_reggroup_p (gdbarch,
					s390_pseudo_register_reggroup_p);
  set_gdbarch_ax_pseudo_register_collect (gdbarch,
					  s390_ax_pseudo_register_collect);
  set_gdbarch_ax_pseudo_register_push_stack
      (gdbarch, s390_ax_pseudo_register_push_stack);
  set_gdbarch_gen_return_address (gdbarch, s390_gen_return_address);

  /* Inferior function calls.  */
  set_gdbarch_push_dummy_call (gdbarch, s390_push_dummy_call);
  set_gdbarch_dummy_id (gdbarch, s390_dummy_id);
  set_gdbarch_frame_align (gdbarch, s390_frame_align);
  set_gdbarch_return_value (gdbarch, s390_return_value);

  /* Frame handling.  */
  /* Stack grows downward.  */
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);
  set_gdbarch_stack_frame_destroyed_p (gdbarch, s390_stack_frame_destroyed_p);
  dwarf2_frame_set_init_reg (gdbarch, s390_dwarf2_frame_init_reg);
  dwarf2_frame_set_adjust_regnum (gdbarch, s390_adjust_frame_regnum);
  dwarf2_append_unwinders (gdbarch);
  set_gdbarch_unwind_pc (gdbarch, s390_unwind_pc);
  set_gdbarch_unwind_sp (gdbarch, s390_unwind_sp);

  switch (info.bfd_arch_info->mach)
    {
    case bfd_mach_s390_31:
      set_gdbarch_addr_bits_remove (gdbarch, s390_addr_bits_remove);
      break;

    case bfd_mach_s390_64:
      set_gdbarch_long_bit (gdbarch, 64);
      set_gdbarch_long_long_bit (gdbarch, 64);
      set_gdbarch_ptr_bit (gdbarch, 64);
      set_gdbarch_address_class_type_flags (gdbarch,
					    s390_address_class_type_flags);
      set_gdbarch_address_class_type_flags_to_name (gdbarch,
						    s390_address_class_type_flags_to_name);
      set_gdbarch_address_class_name_to_type_flags (gdbarch,
						    s390_address_class_name_to_type_flags);
      break;
    }

  /* SystemTap functions.  */
  set_gdbarch_stap_register_prefixes (gdbarch, stap_register_prefixes);
  set_gdbarch_stap_register_indirection_prefixes (gdbarch,
					  stap_register_indirection_prefixes);
  set_gdbarch_stap_register_indirection_suffixes (gdbarch,
					  stap_register_indirection_suffixes);

  set_gdbarch_disassembler_options (gdbarch, &s390_disassembler_options);
  set_gdbarch_valid_disassembler_options (gdbarch,
					  disassembler_options_s390 ());

  /* Process record-replay */
  set_gdbarch_process_record (gdbarch, s390_process_record);

  /* Miscellaneous.  */
  set_gdbarch_stap_is_single_operand (gdbarch, s390_stap_is_single_operand);
  set_gdbarch_gcc_target_options (gdbarch, s390_gcc_target_options);
  set_gdbarch_gnu_triplet_regexp (gdbarch, s390_gnu_triplet_regexp);

  /* Initialize the OSABI.  */
  gdbarch_init_osabi (info, gdbarch);

  /* Always create a default tdesc.  Otherwise commands like 'set osabi'
     cause GDB to crash with an internal error when the user tries to set
     an unsupported OSABI.  */
  if (!tdesc_has_registers (tdesc))
    {
      if (info.bfd_arch_info->mach == bfd_mach_s390_31)
	tdesc = tdesc_s390_linux32;
      else
	tdesc = tdesc_s390x_linux64;
    }
  tdep->tdesc = tdesc;

  /* Check any target description for validity.  */
  if (!s390_tdesc_valid (tdep, tdesc_data.get ()))
    {
      gdbarch_free (gdbarch);
      return NULL;
    }

  /* Determine vector ABI.  */
#ifdef HAVE_ELF
  if (tdep->have_vx
      && info.abfd != NULL
      && info.abfd->format == bfd_object
      && bfd_get_flavour (info.abfd) == bfd_target_elf_flavour
      && bfd_elf_get_obj_attr_int (info.abfd, OBJ_ATTR_GNU,
				   Tag_GNU_S390_ABI_Vector) == 2)
    tdep->vector_abi = S390_VECTOR_ABI_128;
#endif

  /* Find a candidate among extant architectures.  */
  for (arches = gdbarch_list_lookup_by_info (arches, &info);
       arches != NULL;
       arches = gdbarch_list_lookup_by_info (arches->next, &info))
    {
      s390_gdbarch_tdep *tmp
	= gdbarch_tdep<s390_gdbarch_tdep> (arches->gdbarch);

      if (!tmp)
	continue;

      /* A program can 'choose' not to use the vector registers when they
	 are present.  Leading to the same tdesc but different tdep and
	 thereby a different gdbarch.  */
      if (tmp->vector_abi != tdep->vector_abi)
	continue;

      gdbarch_free (gdbarch);
      return arches->gdbarch;
    }

  tdesc_use_registers (gdbarch, tdep->tdesc, std::move (tdesc_data));
  set_gdbarch_register_name (gdbarch, s390_register_name);

  /* Assign pseudo register numbers.  */
  first_pseudo_reg = gdbarch_num_regs (gdbarch);
  last_pseudo_reg = first_pseudo_reg;
  if (tdep->have_upper)
    {
      tdep->gpr_full_regnum = last_pseudo_reg;
      last_pseudo_reg += 16;
    }
  if (tdep->have_vx)
    {
      tdep->v0_full_regnum = last_pseudo_reg;
      last_pseudo_reg += 16;
    }
  tdep->pc_regnum = last_pseudo_reg++;
  tdep->cc_regnum = last_pseudo_reg++;
  set_gdbarch_pc_regnum (gdbarch, tdep->pc_regnum);
  set_gdbarch_num_pseudo_regs (gdbarch, last_pseudo_reg - first_pseudo_reg);

  /* Frame handling.  */
  frame_base_append_sniffer (gdbarch, dwarf2_frame_base_sniffer);
  frame_unwind_append_unwinder (gdbarch, &s390_stub_frame_unwind);
  frame_unwind_append_unwinder (gdbarch, &s390_frame_unwind);
  frame_base_set_default (gdbarch, &s390_frame_base);

  return gdbarch;
}

void _initialize_s390_tdep ();
void
_initialize_s390_tdep ()
{
  /* Hook us into the gdbarch mechanism.  */
  gdbarch_register (bfd_arch_s390, s390_gdbarch_init);

  initialize_tdesc_s390_linux32 ();
  initialize_tdesc_s390x_linux64 ();
}
