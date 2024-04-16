/* Frame unwinder for frames with DWARF Call Frame Information.

   Copyright (C) 2003-2024 Free Software Foundation, Inc.

   Contributed by Mark Kettenis.

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
#include "dwarf2/expr.h"
#include "dwarf2.h"
#include "dwarf2/leb.h"
#include "frame.h"
#include "frame-base.h"
#include "frame-unwind.h"
#include "gdbcore.h"
#include "gdbtypes.h"
#include "symtab.h"
#include "objfiles.h"
#include "regcache.h"
#include "value.h"
#include "record.h"

#include "complaints.h"
#include "dwarf2/frame.h"
#include "dwarf2/read.h"
#include "dwarf2/public.h"
#include "ax.h"
#include "dwarf2/loc.h"
#include "dwarf2/frame-tailcall.h"
#include "gdbsupport/gdb_binary_search.h"
#if GDB_SELF_TEST
#include "gdbsupport/selftest.h"
#include "selftest-arch.h"
#endif
#include <unordered_map>

#include <algorithm>

struct comp_unit;

/* Call Frame Information (CFI).  */

/* Common Information Entry (CIE).  */

struct dwarf2_cie
{
  /* Computation Unit for this CIE.  */
  struct comp_unit *unit;

  /* Offset into the .debug_frame section where this CIE was found.
     Used to identify this CIE.  */
  ULONGEST cie_pointer;

  /* Constant that is factored out of all advance location
     instructions.  */
  ULONGEST code_alignment_factor;

  /* Constants that is factored out of all offset instructions.  */
  LONGEST data_alignment_factor;

  /* Return address column.  */
  ULONGEST return_address_register;

  /* Instruction sequence to initialize a register set.  */
  const gdb_byte *initial_instructions;
  const gdb_byte *end;

  /* Saved augmentation, in case it's needed later.  */
  const char *augmentation;

  /* Encoding of addresses.  */
  gdb_byte encoding;

  /* Target address size in bytes.  */
  int addr_size;

  /* Target pointer size in bytes.  */
  int ptr_size;

  /* True if a 'z' augmentation existed.  */
  unsigned char saw_z_augmentation;

  /* True if an 'S' augmentation existed.  */
  unsigned char signal_frame;

  /* The version recorded in the CIE.  */
  unsigned char version;

  /* The segment size.  */
  unsigned char segment_size;
};

/* The CIE table is used to find CIEs during parsing, but then
   discarded.  It maps from the CIE's offset to the CIE.  */
typedef std::unordered_map<ULONGEST, dwarf2_cie *> dwarf2_cie_table;

/* Frame Description Entry (FDE).  */

struct dwarf2_fde
{
  /* Return the final location in this FDE.  */
  unrelocated_addr end_addr () const
  {
    return (unrelocated_addr) ((ULONGEST) initial_location
			       + address_range);
  }

  /* CIE for this FDE.  */
  struct dwarf2_cie *cie;

  /* First location associated with this FDE.  */
  unrelocated_addr initial_location;

  /* Number of bytes of program instructions described by this FDE.  */
  ULONGEST address_range;

  /* Instruction sequence.  */
  const gdb_byte *instructions;
  const gdb_byte *end;

  /* True if this FDE is read from a .eh_frame instead of a .debug_frame
     section.  */
  unsigned char eh_frame_p;
};

typedef std::vector<dwarf2_fde *> dwarf2_fde_table;

/* A minimal decoding of DWARF2 compilation units.  We only decode
   what's needed to get to the call frame information.  */

struct comp_unit
{
  comp_unit (struct objfile *objf)
    : abfd (objf->obfd.get ())
  {
  }

  /* Keep the bfd convenient.  */
  bfd *abfd;

  /* Pointer to the .debug_frame section loaded into memory.  */
  const gdb_byte *dwarf_frame_buffer = nullptr;

  /* Length of the loaded .debug_frame section.  */
  bfd_size_type dwarf_frame_size = 0;

  /* Pointer to the .debug_frame section.  */
  asection *dwarf_frame_section = nullptr;

  /* Base for DW_EH_PE_datarel encodings.  */
  bfd_vma dbase = 0;

  /* Base for DW_EH_PE_textrel encodings.  */
  bfd_vma tbase = 0;

  /* The FDE table.  */
  dwarf2_fde_table fde_table;

  /* Hold data used by this module.  */
  auto_obstack obstack;
};

static struct dwarf2_fde *dwarf2_frame_find_fde
  (CORE_ADDR *pc, dwarf2_per_objfile **out_per_objfile);

static int dwarf2_frame_adjust_regnum (struct gdbarch *gdbarch, int regnum,
				       int eh_frame_p);

static ULONGEST read_encoded_value (struct comp_unit *unit, gdb_byte encoding,
				    int ptr_len, const gdb_byte *buf,
				    unsigned int *bytes_read_ptr,
				    unrelocated_addr func_base);


/* See dwarf2/frame.h.  */
bool dwarf2_frame_unwinders_enabled_p = true;

/* Store the length the expression for the CFA in the `cfa_reg' field,
   which is unused in that case.  */
#define cfa_exp_len cfa_reg

dwarf2_frame_state::dwarf2_frame_state (CORE_ADDR pc_, struct dwarf2_cie *cie)
  : pc (pc_), data_align (cie->data_alignment_factor),
    code_align (cie->code_alignment_factor),
    retaddr_column (cie->return_address_register)
{
}

/* Execute the required actions for both the DW_CFA_restore and
DW_CFA_restore_extended instructions.  */
static void
dwarf2_restore_rule (struct gdbarch *gdbarch, ULONGEST reg_num,
		     struct dwarf2_frame_state *fs, int eh_frame_p)
{
  ULONGEST reg;

  reg = dwarf2_frame_adjust_regnum (gdbarch, reg_num, eh_frame_p);
  fs->regs.alloc_regs (reg + 1);

  /* Check if this register was explicitly initialized in the
  CIE initial instructions.  If not, default the rule to
  UNSPECIFIED.  */
  if (reg < fs->initial.reg.size ())
    fs->regs.reg[reg] = fs->initial.reg[reg];
  else
    fs->regs.reg[reg].how = DWARF2_FRAME_REG_UNSPECIFIED;

  if (fs->regs.reg[reg].how == DWARF2_FRAME_REG_UNSPECIFIED)
    {
      int regnum = dwarf_reg_to_regnum (gdbarch, reg);

      complaint (_("\
incomplete CFI data; DW_CFA_restore unspecified\n\
register %s (#%d) at %s"),
		 gdbarch_register_name (gdbarch, regnum), regnum,
		 paddress (gdbarch, fs->pc));
    }
}

static CORE_ADDR
execute_stack_op (const gdb_byte *exp, ULONGEST len, int addr_size,
		  frame_info_ptr this_frame, CORE_ADDR initial,
		  int initial_in_stack_memory, dwarf2_per_objfile *per_objfile)
{
  dwarf_expr_context ctx (per_objfile, addr_size);
  scoped_value_mark free_values;

  ctx.push_address (initial, initial_in_stack_memory);
  value *result_val = ctx.evaluate (exp, len, true, nullptr, this_frame);

  if (result_val->lval () == lval_memory)
    return result_val->address ();
  else
    return value_as_address (result_val);
}


/* Execute FDE program from INSN_PTR possibly up to INSN_END or up to inferior
   PC.  Modify FS state accordingly.  Return current INSN_PTR where the
   execution has stopped, one can resume it on the next call.  */

static const gdb_byte *
execute_cfa_program (struct dwarf2_fde *fde, const gdb_byte *insn_ptr,
		     const gdb_byte *insn_end, struct gdbarch *gdbarch,
		     CORE_ADDR pc, struct dwarf2_frame_state *fs,
		     CORE_ADDR text_offset)
{
  int eh_frame_p = fde->eh_frame_p;
  unsigned int bytes_read;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  while (insn_ptr < insn_end && fs->pc <= pc)
    {
      gdb_byte insn = *insn_ptr++;
      uint64_t utmp, reg;
      int64_t offset;

      if ((insn & 0xc0) == DW_CFA_advance_loc)
	fs->pc += (insn & 0x3f) * fs->code_align;
      else if ((insn & 0xc0) == DW_CFA_offset)
	{
	  reg = insn & 0x3f;
	  reg = dwarf2_frame_adjust_regnum (gdbarch, reg, eh_frame_p);
	  insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &utmp);
	  offset = utmp * fs->data_align;
	  fs->regs.alloc_regs (reg + 1);
	  fs->regs.reg[reg].how = DWARF2_FRAME_REG_SAVED_OFFSET;
	  fs->regs.reg[reg].loc.offset = offset;
	}
      else if ((insn & 0xc0) == DW_CFA_restore)
	{
	  reg = insn & 0x3f;
	  dwarf2_restore_rule (gdbarch, reg, fs, eh_frame_p);
	}
      else
	{
	  switch (insn)
	    {
	    case DW_CFA_set_loc:
	      fs->pc = read_encoded_value (fde->cie->unit, fde->cie->encoding,
					   fde->cie->ptr_size, insn_ptr,
					   &bytes_read, fde->initial_location);
	      /* Apply the text offset for relocatable objects.  */
	      fs->pc += text_offset;
	      insn_ptr += bytes_read;
	      break;

	    case DW_CFA_advance_loc1:
	      utmp = extract_unsigned_integer (insn_ptr, 1, byte_order);
	      fs->pc += utmp * fs->code_align;
	      insn_ptr++;
	      break;
	    case DW_CFA_advance_loc2:
	      utmp = extract_unsigned_integer (insn_ptr, 2, byte_order);
	      fs->pc += utmp * fs->code_align;
	      insn_ptr += 2;
	      break;
	    case DW_CFA_advance_loc4:
	      utmp = extract_unsigned_integer (insn_ptr, 4, byte_order);
	      fs->pc += utmp * fs->code_align;
	      insn_ptr += 4;
	      break;

	    case DW_CFA_offset_extended:
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &reg);
	      reg = dwarf2_frame_adjust_regnum (gdbarch, reg, eh_frame_p);
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &utmp);
	      offset = utmp * fs->data_align;
	      fs->regs.alloc_regs (reg + 1);
	      fs->regs.reg[reg].how = DWARF2_FRAME_REG_SAVED_OFFSET;
	      fs->regs.reg[reg].loc.offset = offset;
	      break;

	    case DW_CFA_restore_extended:
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &reg);
	      dwarf2_restore_rule (gdbarch, reg, fs, eh_frame_p);
	      break;

	    case DW_CFA_undefined:
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &reg);
	      reg = dwarf2_frame_adjust_regnum (gdbarch, reg, eh_frame_p);
	      fs->regs.alloc_regs (reg + 1);
	      fs->regs.reg[reg].how = DWARF2_FRAME_REG_UNDEFINED;
	      break;

	    case DW_CFA_same_value:
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &reg);
	      reg = dwarf2_frame_adjust_regnum (gdbarch, reg, eh_frame_p);
	      fs->regs.alloc_regs (reg + 1);
	      fs->regs.reg[reg].how = DWARF2_FRAME_REG_SAME_VALUE;
	      break;

	    case DW_CFA_register:
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &reg);
	      reg = dwarf2_frame_adjust_regnum (gdbarch, reg, eh_frame_p);
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &utmp);
	      utmp = dwarf2_frame_adjust_regnum (gdbarch, utmp, eh_frame_p);
	      fs->regs.alloc_regs (reg + 1);
	      fs->regs.reg[reg].how = DWARF2_FRAME_REG_SAVED_REG;
	      fs->regs.reg[reg].loc.reg = utmp;
	      break;

	    case DW_CFA_remember_state:
	      {
		struct dwarf2_frame_state_reg_info *new_rs;

		new_rs = new dwarf2_frame_state_reg_info (fs->regs);
		fs->regs.prev = new_rs;
	      }
	      break;

	    case DW_CFA_restore_state:
	      {
		struct dwarf2_frame_state_reg_info *old_rs = fs->regs.prev;

		if (old_rs == NULL)
		  {
		    complaint (_("\
bad CFI data; mismatched DW_CFA_restore_state at %s"),
			       paddress (gdbarch, fs->pc));
		  }
		else
		  fs->regs = std::move (*old_rs);
	      }
	      break;

	    case DW_CFA_def_cfa:
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &reg);
	      fs->regs.cfa_reg = reg;
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &utmp);

	      if (fs->armcc_cfa_offsets_sf)
		utmp *= fs->data_align;

	      fs->regs.cfa_offset = utmp;
	      fs->regs.cfa_how = CFA_REG_OFFSET;
	      break;

	    case DW_CFA_def_cfa_register:
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &reg);
	      fs->regs.cfa_reg = dwarf2_frame_adjust_regnum (gdbarch, reg,
							     eh_frame_p);
	      fs->regs.cfa_how = CFA_REG_OFFSET;
	      break;

	    case DW_CFA_def_cfa_offset:
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &utmp);

	      if (fs->armcc_cfa_offsets_sf)
		utmp *= fs->data_align;

	      fs->regs.cfa_offset = utmp;
	      /* cfa_how deliberately not set.  */
	      break;

	    case DW_CFA_nop:
	      break;

	    case DW_CFA_def_cfa_expression:
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &utmp);
	      fs->regs.cfa_exp_len = utmp;
	      fs->regs.cfa_exp = insn_ptr;
	      fs->regs.cfa_how = CFA_EXP;
	      insn_ptr += fs->regs.cfa_exp_len;
	      break;

	    case DW_CFA_expression:
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &reg);
	      reg = dwarf2_frame_adjust_regnum (gdbarch, reg, eh_frame_p);
	      fs->regs.alloc_regs (reg + 1);
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &utmp);
	      fs->regs.reg[reg].loc.exp.start = insn_ptr;
	      fs->regs.reg[reg].loc.exp.len = utmp;
	      fs->regs.reg[reg].how = DWARF2_FRAME_REG_SAVED_EXP;
	      insn_ptr += utmp;
	      break;

	    case DW_CFA_offset_extended_sf:
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &reg);
	      reg = dwarf2_frame_adjust_regnum (gdbarch, reg, eh_frame_p);
	      insn_ptr = safe_read_sleb128 (insn_ptr, insn_end, &offset);
	      offset *= fs->data_align;
	      fs->regs.alloc_regs (reg + 1);
	      fs->regs.reg[reg].how = DWARF2_FRAME_REG_SAVED_OFFSET;
	      fs->regs.reg[reg].loc.offset = offset;
	      break;

	    case DW_CFA_val_offset:
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &reg);
	      fs->regs.alloc_regs (reg + 1);
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &utmp);
	      offset = utmp * fs->data_align;
	      fs->regs.reg[reg].how = DWARF2_FRAME_REG_SAVED_VAL_OFFSET;
	      fs->regs.reg[reg].loc.offset = offset;
	      break;

	    case DW_CFA_val_offset_sf:
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &reg);
	      fs->regs.alloc_regs (reg + 1);
	      insn_ptr = safe_read_sleb128 (insn_ptr, insn_end, &offset);
	      offset *= fs->data_align;
	      fs->regs.reg[reg].how = DWARF2_FRAME_REG_SAVED_VAL_OFFSET;
	      fs->regs.reg[reg].loc.offset = offset;
	      break;

	    case DW_CFA_val_expression:
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &reg);
	      fs->regs.alloc_regs (reg + 1);
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &utmp);
	      fs->regs.reg[reg].loc.exp.start = insn_ptr;
	      fs->regs.reg[reg].loc.exp.len = utmp;
	      fs->regs.reg[reg].how = DWARF2_FRAME_REG_SAVED_VAL_EXP;
	      insn_ptr += utmp;
	      break;

	    case DW_CFA_def_cfa_sf:
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &reg);
	      fs->regs.cfa_reg = dwarf2_frame_adjust_regnum (gdbarch, reg,
							     eh_frame_p);
	      insn_ptr = safe_read_sleb128 (insn_ptr, insn_end, &offset);
	      fs->regs.cfa_offset = offset * fs->data_align;
	      fs->regs.cfa_how = CFA_REG_OFFSET;
	      break;

	    case DW_CFA_def_cfa_offset_sf:
	      insn_ptr = safe_read_sleb128 (insn_ptr, insn_end, &offset);
	      fs->regs.cfa_offset = offset * fs->data_align;
	      /* cfa_how deliberately not set.  */
	      break;

	    case DW_CFA_GNU_args_size:
	      /* Ignored.  */
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &utmp);
	      break;

	    case DW_CFA_GNU_negative_offset_extended:
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &reg);
	      reg = dwarf2_frame_adjust_regnum (gdbarch, reg, eh_frame_p);
	      insn_ptr = safe_read_uleb128 (insn_ptr, insn_end, &utmp);
	      offset = utmp * fs->data_align;
	      fs->regs.alloc_regs (reg + 1);
	      fs->regs.reg[reg].how = DWARF2_FRAME_REG_SAVED_OFFSET;
	      fs->regs.reg[reg].loc.offset = -offset;
	      break;

	    default:
	      if (insn >= DW_CFA_lo_user && insn <= DW_CFA_hi_user)
		{
		  /* Handle vendor-specific CFI for different architectures.  */
		  if (!gdbarch_execute_dwarf_cfa_vendor_op (gdbarch, insn, fs))
		    error (_("Call Frame Instruction op %d in vendor extension "
			     "space is not handled on this architecture."),
			   insn);
		}
	      else
		internal_error (_("Unknown CFI encountered."));
	    }
	}
    }

  if (fs->initial.reg.empty ())
    {
      /* Don't allow remember/restore between CIE and FDE programs.  */
      delete fs->regs.prev;
      fs->regs.prev = NULL;
    }

  return insn_ptr;
}

#if GDB_SELF_TEST

namespace selftests {

/* Unit test to function execute_cfa_program.  */

static void
execute_cfa_program_test (struct gdbarch *gdbarch)
{
  struct dwarf2_fde fde;
  struct dwarf2_cie cie;

  memset (&fde, 0, sizeof fde);
  memset (&cie, 0, sizeof cie);

  cie.data_alignment_factor = -4;
  cie.code_alignment_factor = 2;
  fde.cie = &cie;

  dwarf2_frame_state fs (0, fde.cie);

  gdb_byte insns[] =
    {
      DW_CFA_def_cfa, 1, 4,  /* DW_CFA_def_cfa: r1 ofs 4 */
      DW_CFA_offset | 0x2, 1,  /* DW_CFA_offset: r2 at cfa-4 */
      DW_CFA_remember_state,
      DW_CFA_restore_state,
    };

  const gdb_byte *insn_end = insns + sizeof (insns);
  const gdb_byte *out = execute_cfa_program (&fde, insns, insn_end, gdbarch,
					     0, &fs, 0);

  SELF_CHECK (out == insn_end);
  SELF_CHECK (fs.pc == 0);

  /* The instructions above only use r1 and r2, but the register numbers
     used are adjusted by dwarf2_frame_adjust_regnum.  */
  auto r1 = dwarf2_frame_adjust_regnum (gdbarch, 1, fde.eh_frame_p);
  auto r2 = dwarf2_frame_adjust_regnum (gdbarch, 2, fde.eh_frame_p);

  SELF_CHECK (fs.regs.reg.size () == (std::max (r1, r2) + 1));

  SELF_CHECK (fs.regs.reg[r2].how == DWARF2_FRAME_REG_SAVED_OFFSET);
  SELF_CHECK (fs.regs.reg[r2].loc.offset == -4);

  for (auto i = 0; i < fs.regs.reg.size (); i++)
    if (i != r2)
      SELF_CHECK (fs.regs.reg[i].how == DWARF2_FRAME_REG_UNSPECIFIED);

  SELF_CHECK (fs.regs.cfa_reg == 1);
  SELF_CHECK (fs.regs.cfa_offset == 4);
  SELF_CHECK (fs.regs.cfa_how == CFA_REG_OFFSET);
  SELF_CHECK (fs.regs.cfa_exp == NULL);
  SELF_CHECK (fs.regs.prev == NULL);
}

} // namespace selftests
#endif /* GDB_SELF_TEST */



/* Architecture-specific operations.  */

static void dwarf2_frame_default_init_reg (struct gdbarch *gdbarch,
					   int regnum,
					   struct dwarf2_frame_state_reg *reg,
					   frame_info_ptr this_frame);

struct dwarf2_frame_ops
{
  /* Pre-initialize the register state REG for register REGNUM.  */
  void (*init_reg) (struct gdbarch *, int, struct dwarf2_frame_state_reg *,
		    frame_info_ptr)
    = dwarf2_frame_default_init_reg;

  /* Check whether the THIS_FRAME is a signal trampoline.  */
  int (*signal_frame_p) (struct gdbarch *, frame_info_ptr) = nullptr;

  /* Convert .eh_frame register number to DWARF register number, or
     adjust .debug_frame register number.  */
  int (*adjust_regnum) (struct gdbarch *, int, int) = nullptr;
};

/* Per-architecture data key.  */
static const registry<gdbarch>::key<dwarf2_frame_ops> dwarf2_frame_data;

/* Get or initialize the frame ops.  */
static dwarf2_frame_ops *
get_frame_ops (struct gdbarch *gdbarch)
{
  dwarf2_frame_ops *result = dwarf2_frame_data.get (gdbarch);
  if (result == nullptr)
    result = dwarf2_frame_data.emplace (gdbarch);
  return result;
}

/* Default architecture-specific register state initialization
   function.  */

static void
dwarf2_frame_default_init_reg (struct gdbarch *gdbarch, int regnum,
			       struct dwarf2_frame_state_reg *reg,
			       frame_info_ptr this_frame)
{
  /* If we have a register that acts as a program counter, mark it as
     a destination for the return address.  If we have a register that
     serves as the stack pointer, arrange for it to be filled with the
     call frame address (CFA).  The other registers are marked as
     unspecified.

     We copy the return address to the program counter, since many
     parts in GDB assume that it is possible to get the return address
     by unwinding the program counter register.  However, on ISA's
     with a dedicated return address register, the CFI usually only
     contains information to unwind that return address register.

     The reason we're treating the stack pointer special here is
     because in many cases GCC doesn't emit CFI for the stack pointer
     and implicitly assumes that it is equal to the CFA.  This makes
     some sense since the DWARF specification (version 3, draft 8,
     p. 102) says that:

     "Typically, the CFA is defined to be the value of the stack
     pointer at the call site in the previous frame (which may be
     different from its value on entry to the current frame)."

     However, this isn't true for all platforms supported by GCC
     (e.g. IBM S/390 and zSeries).  Those architectures should provide
     their own architecture-specific initialization function.  */

  if (regnum == gdbarch_pc_regnum (gdbarch))
    reg->how = DWARF2_FRAME_REG_RA;
  else if (regnum == gdbarch_sp_regnum (gdbarch))
    reg->how = DWARF2_FRAME_REG_CFA;
}

/* Set the architecture-specific register state initialization
   function for GDBARCH to INIT_REG.  */

void
dwarf2_frame_set_init_reg (struct gdbarch *gdbarch,
			   void (*init_reg) (struct gdbarch *, int,
					     struct dwarf2_frame_state_reg *,
					     frame_info_ptr))
{
  struct dwarf2_frame_ops *ops = get_frame_ops (gdbarch);

  ops->init_reg = init_reg;
}

/* Pre-initialize the register state REG for register REGNUM.  */

static void
dwarf2_frame_init_reg (struct gdbarch *gdbarch, int regnum,
		       struct dwarf2_frame_state_reg *reg,
		       frame_info_ptr this_frame)
{
  struct dwarf2_frame_ops *ops = get_frame_ops (gdbarch);

  ops->init_reg (gdbarch, regnum, reg, this_frame);
}

/* Set the architecture-specific signal trampoline recognition
   function for GDBARCH to SIGNAL_FRAME_P.  */

void
dwarf2_frame_set_signal_frame_p (struct gdbarch *gdbarch,
				 int (*signal_frame_p) (struct gdbarch *,
							frame_info_ptr))
{
  struct dwarf2_frame_ops *ops = get_frame_ops (gdbarch);

  ops->signal_frame_p = signal_frame_p;
}

/* Query the architecture-specific signal frame recognizer for
   THIS_FRAME.  */

static int
dwarf2_frame_signal_frame_p (struct gdbarch *gdbarch,
			     frame_info_ptr this_frame)
{
  struct dwarf2_frame_ops *ops = get_frame_ops (gdbarch);

  if (ops->signal_frame_p == NULL)
    return 0;
  return ops->signal_frame_p (gdbarch, this_frame);
}

/* Set the architecture-specific adjustment of .eh_frame and .debug_frame
   register numbers.  */

void
dwarf2_frame_set_adjust_regnum (struct gdbarch *gdbarch,
				int (*adjust_regnum) (struct gdbarch *,
						      int, int))
{
  struct dwarf2_frame_ops *ops = get_frame_ops (gdbarch);

  ops->adjust_regnum = adjust_regnum;
}

/* Translate a .eh_frame register to DWARF register, or adjust a .debug_frame
   register.  */

static int
dwarf2_frame_adjust_regnum (struct gdbarch *gdbarch,
			    int regnum, int eh_frame_p)
{
  struct dwarf2_frame_ops *ops = get_frame_ops (gdbarch);

  if (ops->adjust_regnum == NULL)
    return regnum;
  return ops->adjust_regnum (gdbarch, regnum, eh_frame_p);
}

static void
dwarf2_frame_find_quirks (struct dwarf2_frame_state *fs,
			  struct dwarf2_fde *fde)
{
  struct compunit_symtab *cust;

  cust = find_pc_compunit_symtab (fs->pc);
  if (cust == NULL)
    return;

  if (producer_is_realview (cust->producer ()))
    {
      if (fde->cie->version == 1)
	fs->armcc_cfa_offsets_sf = 1;

      if (fde->cie->version == 1)
	fs->armcc_cfa_offsets_reversed = 1;

      /* The reversed offset problem is present in some compilers
	 using DWARF3, but it was eventually fixed.  Check the ARM
	 defined augmentations, which are in the format "armcc" followed
	 by a list of one-character options.  The "+" option means
	 this problem is fixed (no quirk needed).  If the armcc
	 augmentation is missing, the quirk is needed.  */
      if (fde->cie->version == 3
	  && (!startswith (fde->cie->augmentation, "armcc")
	      || strchr (fde->cie->augmentation + 5, '+') == NULL))
	fs->armcc_cfa_offsets_reversed = 1;

      return;
    }
}


/* See dwarf2/frame.h.  */

int
dwarf2_fetch_cfa_info (struct gdbarch *gdbarch, CORE_ADDR pc,
		       struct dwarf2_per_cu_data *data,
		       int *regnum_out, LONGEST *offset_out,
		       CORE_ADDR *text_offset_out,
		       const gdb_byte **cfa_start_out,
		       const gdb_byte **cfa_end_out)
{
  struct dwarf2_fde *fde;
  dwarf2_per_objfile *per_objfile;
  CORE_ADDR pc1 = pc;

  /* Find the correct FDE.  */
  fde = dwarf2_frame_find_fde (&pc1, &per_objfile);
  if (fde == NULL)
    error (_("Could not compute CFA; needed to translate this expression"));

  gdb_assert (per_objfile != nullptr);

  dwarf2_frame_state fs (pc1, fde->cie);

  /* Check for "quirks" - known bugs in producers.  */
  dwarf2_frame_find_quirks (&fs, fde);

  /* First decode all the insns in the CIE.  */
  execute_cfa_program (fde, fde->cie->initial_instructions,
		       fde->cie->end, gdbarch, pc, &fs,
		       per_objfile->objfile->text_section_offset ());

  /* Save the initialized register set.  */
  fs.initial = fs.regs;

  /* Then decode the insns in the FDE up to our target PC.  */
  execute_cfa_program (fde, fde->instructions, fde->end, gdbarch, pc, &fs,
		       per_objfile->objfile->text_section_offset ());

  /* Calculate the CFA.  */
  switch (fs.regs.cfa_how)
    {
    case CFA_REG_OFFSET:
      {
	int regnum = dwarf_reg_to_regnum_or_error (gdbarch, fs.regs.cfa_reg);

	*regnum_out = regnum;
	if (fs.armcc_cfa_offsets_reversed)
	  *offset_out = -fs.regs.cfa_offset;
	else
	  *offset_out = fs.regs.cfa_offset;
	return 1;
      }

    case CFA_EXP:
      *text_offset_out = per_objfile->objfile->text_section_offset ();
      *cfa_start_out = fs.regs.cfa_exp;
      *cfa_end_out = fs.regs.cfa_exp + fs.regs.cfa_exp_len;
      return 0;

    default:
      internal_error (_("Unknown CFA rule."));
    }
}


/* Custom function data object for architecture specific prev_register
   implementation.  Main purpose of this object is to allow caching of
   expensive data lookups in the prev_register handling.  */

struct dwarf2_frame_fn_data
{
  /* The cookie to identify the custom function data by.  */
  fn_prev_register cookie;

  /* The custom function data.  */
  void *data;

  /* Pointer to the next custom function data object for this frame.  */
  struct dwarf2_frame_fn_data *next;
};

struct dwarf2_frame_cache
{
  /* DWARF Call Frame Address.  */
  CORE_ADDR cfa;

  /* Set if the return address column was marked as unavailable
     (required non-collected memory or registers to compute).  */
  int unavailable_retaddr;

  /* Set if the return address column was marked as undefined.  */
  int undefined_retaddr;

  /* Saved registers, indexed by GDB register number, not by DWARF
     register number.  */
  struct dwarf2_frame_state_reg *reg;

  /* Return address register.  */
  struct dwarf2_frame_state_reg retaddr_reg;

  /* Target address size in bytes.  */
  int addr_size;

  /* The dwarf2_per_objfile from which this frame description came.  */
  dwarf2_per_objfile *per_objfile;

  /* If not NULL then this frame is the bottom frame of a TAILCALL_FRAME
     sequence.  If NULL then it is a normal case with no TAILCALL_FRAME
     involved.  Non-bottom frames of a virtual tail call frames chain use
     dwarf2_tailcall_frame_unwind unwinder so this field does not apply for
     them.  */
  void *tailcall_cache;

  struct dwarf2_frame_fn_data *fn_data;
};

static struct dwarf2_frame_cache *
dwarf2_frame_cache (frame_info_ptr this_frame, void **this_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  const int num_regs = gdbarch_num_cooked_regs (gdbarch);
  struct dwarf2_frame_cache *cache;
  struct dwarf2_fde *fde;
  CORE_ADDR entry_pc;
  const gdb_byte *instr;

  if (*this_cache)
    return (struct dwarf2_frame_cache *) *this_cache;

  /* Allocate a new cache.  */
  cache = FRAME_OBSTACK_ZALLOC (struct dwarf2_frame_cache);
  cache->reg = FRAME_OBSTACK_CALLOC (num_regs, struct dwarf2_frame_state_reg);
  *this_cache = cache;

  /* Unwind the PC.

     Note that if the next frame is never supposed to return (i.e. a call
     to abort), the compiler might optimize away the instruction at
     its return address.  As a result the return address will
     point at some random instruction, and the CFI for that
     instruction is probably worthless to us.  GCC's unwinder solves
     this problem by substracting 1 from the return address to get an
     address in the middle of a presumed call instruction (or the
     instruction in the associated delay slot).  This should only be
     done for "normal" frames and not for resume-type frames (signal
     handlers, sentinel frames, dummy frames).  The function
     get_frame_address_in_block does just this.  It's not clear how
     reliable the method is though; there is the potential for the
     register state pre-call being different to that on return.  */
  CORE_ADDR pc1 = get_frame_address_in_block (this_frame);

  /* Find the correct FDE.  */
  fde = dwarf2_frame_find_fde (&pc1, &cache->per_objfile);
  gdb_assert (fde != NULL);
  gdb_assert (cache->per_objfile != nullptr);

  CORE_ADDR text_offset = cache->per_objfile->objfile->text_section_offset ();

  /* Allocate and initialize the frame state.  */
  struct dwarf2_frame_state fs (pc1, fde->cie);

  cache->addr_size = fde->cie->addr_size;

  /* Check for "quirks" - known bugs in producers.  */
  dwarf2_frame_find_quirks (&fs, fde);

  /* First decode all the insns in the CIE.  */
  execute_cfa_program (fde, fde->cie->initial_instructions,
		       fde->cie->end, gdbarch,
		       get_frame_address_in_block (this_frame), &fs,
		       text_offset);

  /* Save the initialized register set.  */
  fs.initial = fs.regs;

  /* Fetching the entry pc for THIS_FRAME won't necessarily result
     in an address that's within the range of FDE locations.  This
     is due to the possibility of the function occupying non-contiguous
     ranges.  */
  LONGEST entry_cfa_sp_offset;
  int entry_cfa_sp_offset_p = 0;
  if (get_frame_func_if_available (this_frame, &entry_pc)
      && fde->initial_location <= (unrelocated_addr) (entry_pc - text_offset)
      && (unrelocated_addr) (entry_pc - text_offset) < fde->end_addr ())
    {
      /* Decode the insns in the FDE up to the entry PC.  */
      instr = execute_cfa_program (fde, fde->instructions, fde->end, gdbarch,
				   entry_pc, &fs, text_offset);

      if (fs.regs.cfa_how == CFA_REG_OFFSET
	  && (dwarf_reg_to_regnum (gdbarch, fs.regs.cfa_reg)
	      == gdbarch_sp_regnum (gdbarch)))
	{
	  entry_cfa_sp_offset = fs.regs.cfa_offset;
	  entry_cfa_sp_offset_p = 1;
	}
    }
  else
    instr = fde->instructions;

  /* Then decode the insns in the FDE up to our target PC.  */
  execute_cfa_program (fde, instr, fde->end, gdbarch,
		       get_frame_address_in_block (this_frame), &fs,
		       text_offset);

  try
    {
      /* Calculate the CFA.  */
      switch (fs.regs.cfa_how)
	{
	case CFA_REG_OFFSET:
	  cache->cfa = read_addr_from_reg (this_frame, fs.regs.cfa_reg);
	  if (fs.armcc_cfa_offsets_reversed)
	    cache->cfa -= fs.regs.cfa_offset;
	  else
	    cache->cfa += fs.regs.cfa_offset;
	  break;

	case CFA_EXP:
	  cache->cfa =
	    execute_stack_op (fs.regs.cfa_exp, fs.regs.cfa_exp_len,
			      cache->addr_size, this_frame, 0, 0,
			      cache->per_objfile);
	  break;

	default:
	  internal_error (_("Unknown CFA rule."));
	}
    }
  catch (const gdb_exception_error &ex)
    {
      if (ex.error == NOT_AVAILABLE_ERROR)
	{
	  cache->unavailable_retaddr = 1;
	  return cache;
	}

      throw;
    }

  /* Initialize the register state.  */
  {
    int regnum;

    for (regnum = 0; regnum < num_regs; regnum++)
      dwarf2_frame_init_reg (gdbarch, regnum, &cache->reg[regnum], this_frame);
  }

  /* Go through the DWARF2 CFI generated table and save its register
     location information in the cache.  Note that we don't skip the
     return address column; it's perfectly all right for it to
     correspond to a real register.  */
  {
    int column;		/* CFI speak for "register number".  */

    for (column = 0; column < fs.regs.reg.size (); column++)
      {
	/* Use the GDB register number as the destination index.  */
	int regnum = dwarf_reg_to_regnum (gdbarch, column);

	/* Protect against a target returning a bad register.  */
	if (regnum < 0 || regnum >= num_regs)
	  continue;

	/* NOTE: cagney/2003-09-05: CFI should specify the disposition
	   of all debug info registers.  If it doesn't, complain (but
	   not too loudly).  It turns out that GCC assumes that an
	   unspecified register implies "same value" when CFI (draft
	   7) specifies nothing at all.  Such a register could equally
	   be interpreted as "undefined".  Also note that this check
	   isn't sufficient; it only checks that all registers in the
	   range [0 .. max column] are specified, and won't detect
	   problems when a debug info register falls outside of the
	   table.  We need a way of iterating through all the valid
	   DWARF2 register numbers.  */
	if (fs.regs.reg[column].how == DWARF2_FRAME_REG_UNSPECIFIED)
	  {
	    if (cache->reg[regnum].how == DWARF2_FRAME_REG_UNSPECIFIED)
	      complaint (_("\
incomplete CFI data; unspecified registers (e.g., %s) at %s"),
			 gdbarch_register_name (gdbarch, regnum),
			 paddress (gdbarch, fs.pc));
	  }
	else
	  cache->reg[regnum] = fs.regs.reg[column];
      }
  }

  /* Eliminate any DWARF2_FRAME_REG_RA rules, and save the information
     we need for evaluating DWARF2_FRAME_REG_RA_OFFSET rules.  */
  {
    int regnum;

    for (regnum = 0; regnum < num_regs; regnum++)
      {
	if (cache->reg[regnum].how == DWARF2_FRAME_REG_RA
	    || cache->reg[regnum].how == DWARF2_FRAME_REG_RA_OFFSET)
	  {
	    const std::vector<struct dwarf2_frame_state_reg> &regs
	      = fs.regs.reg;
	    ULONGEST retaddr_column = fs.retaddr_column;

	    /* It seems rather bizarre to specify an "empty" column as
	       the return adress column.  However, this is exactly
	       what GCC does on some targets.  It turns out that GCC
	       assumes that the return address can be found in the
	       register corresponding to the return address column.
	       Incidentally, that's how we should treat a return
	       address column specifying "same value" too.  */
	    if (fs.retaddr_column < fs.regs.reg.size ()
		&& regs[retaddr_column].how != DWARF2_FRAME_REG_UNSPECIFIED
		&& regs[retaddr_column].how != DWARF2_FRAME_REG_SAME_VALUE)
	      {
		if (cache->reg[regnum].how == DWARF2_FRAME_REG_RA)
		  cache->reg[regnum] = regs[retaddr_column];
		else
		  cache->retaddr_reg = regs[retaddr_column];
	      }
	    else
	      {
		if (cache->reg[regnum].how == DWARF2_FRAME_REG_RA)
		  {
		    cache->reg[regnum].loc.reg = fs.retaddr_column;
		    cache->reg[regnum].how = DWARF2_FRAME_REG_SAVED_REG;
		  }
		else
		  {
		    cache->retaddr_reg.loc.reg = fs.retaddr_column;
		    cache->retaddr_reg.how = DWARF2_FRAME_REG_SAVED_REG;
		  }
	      }
	  }
      }
  }

  if (fs.retaddr_column < fs.regs.reg.size ()
      && fs.regs.reg[fs.retaddr_column].how == DWARF2_FRAME_REG_UNDEFINED)
    cache->undefined_retaddr = 1;

  dwarf2_tailcall_sniffer_first (this_frame, &cache->tailcall_cache,
				 (entry_cfa_sp_offset_p
				  ? &entry_cfa_sp_offset : NULL));

  return cache;
}

static enum unwind_stop_reason
dwarf2_frame_unwind_stop_reason (frame_info_ptr this_frame,
				 void **this_cache)
{
  struct dwarf2_frame_cache *cache
    = dwarf2_frame_cache (this_frame, this_cache);

  if (cache->unavailable_retaddr)
    return UNWIND_UNAVAILABLE;

  if (cache->undefined_retaddr)
    return UNWIND_OUTERMOST;

  return UNWIND_NO_REASON;
}

static void
dwarf2_frame_this_id (frame_info_ptr this_frame, void **this_cache,
		      struct frame_id *this_id)
{
  struct dwarf2_frame_cache *cache =
    dwarf2_frame_cache (this_frame, this_cache);

  if (cache->unavailable_retaddr)
    (*this_id) = frame_id_build_unavailable_stack (get_frame_func (this_frame));
  else if (cache->undefined_retaddr)
    return;
  else
    (*this_id) = frame_id_build (cache->cfa, get_frame_func (this_frame));
}

static struct value *
dwarf2_frame_prev_register (frame_info_ptr this_frame, void **this_cache,
			    int regnum)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct dwarf2_frame_cache *cache =
    dwarf2_frame_cache (this_frame, this_cache);
  CORE_ADDR addr;
  int realnum;

  /* Non-bottom frames of a virtual tail call frames chain use
     dwarf2_tailcall_frame_unwind unwinder so this code does not apply for
     them.  If dwarf2_tailcall_prev_register_first does not have specific value
     unwind the register, tail call frames are assumed to have the register set
     of the top caller.  */
  if (cache->tailcall_cache)
    {
      struct value *val;
      
      val = dwarf2_tailcall_prev_register_first (this_frame,
						 &cache->tailcall_cache,
						 regnum);
      if (val)
	return val;
    }

  switch (cache->reg[regnum].how)
    {
    case DWARF2_FRAME_REG_UNDEFINED:
      /* If CFI explicitly specified that the value isn't defined,
	 mark it as optimized away; the value isn't available.  */
      return frame_unwind_got_optimized (this_frame, regnum);

    case DWARF2_FRAME_REG_SAVED_OFFSET:
      addr = cache->cfa + cache->reg[regnum].loc.offset;
      return frame_unwind_got_memory (this_frame, regnum, addr);

    case DWARF2_FRAME_REG_SAVED_REG:
      realnum = dwarf_reg_to_regnum_or_error
	(gdbarch, cache->reg[regnum].loc.reg);
      return frame_unwind_got_register (this_frame, regnum, realnum);

    case DWARF2_FRAME_REG_SAVED_EXP:
      addr = execute_stack_op (cache->reg[regnum].loc.exp.start,
			       cache->reg[regnum].loc.exp.len,
			       cache->addr_size,
			       this_frame, cache->cfa, 1,
			       cache->per_objfile);
      return frame_unwind_got_memory (this_frame, regnum, addr);

    case DWARF2_FRAME_REG_SAVED_VAL_OFFSET:
      addr = cache->cfa + cache->reg[regnum].loc.offset;
      return frame_unwind_got_constant (this_frame, regnum, addr);

    case DWARF2_FRAME_REG_SAVED_VAL_EXP:
      addr = execute_stack_op (cache->reg[regnum].loc.exp.start,
			       cache->reg[regnum].loc.exp.len,
			       cache->addr_size,
			       this_frame, cache->cfa, 1,
			       cache->per_objfile);
      return frame_unwind_got_constant (this_frame, regnum, addr);

    case DWARF2_FRAME_REG_UNSPECIFIED:
      /* GCC, in its infinite wisdom decided to not provide unwind
	 information for registers that are "same value".  Since
	 DWARF2 (3 draft 7) doesn't define such behavior, said
	 registers are actually undefined (which is different to CFI
	 "undefined").  Code above issues a complaint about this.
	 Here just fudge the books, assume GCC, and that the value is
	 more inner on the stack.  */
      if (regnum < gdbarch_num_regs (gdbarch))
	return frame_unwind_got_register (this_frame, regnum, regnum);
      else
	return nullptr;

    case DWARF2_FRAME_REG_SAME_VALUE:
      return frame_unwind_got_register (this_frame, regnum, regnum);

    case DWARF2_FRAME_REG_CFA:
      return frame_unwind_got_address (this_frame, regnum, cache->cfa);

    case DWARF2_FRAME_REG_CFA_OFFSET:
      addr = cache->cfa + cache->reg[regnum].loc.offset;
      return frame_unwind_got_address (this_frame, regnum, addr);

    case DWARF2_FRAME_REG_RA_OFFSET:
      addr = cache->reg[regnum].loc.offset;
      regnum = dwarf_reg_to_regnum_or_error
	(gdbarch, cache->retaddr_reg.loc.reg);
      addr += get_frame_register_unsigned (this_frame, regnum);
      return frame_unwind_got_address (this_frame, regnum, addr);

    case DWARF2_FRAME_REG_FN:
      return cache->reg[regnum].loc.fn (this_frame, this_cache, regnum);

    default:
      internal_error (_("Unknown register rule."));
    }
}

/* See frame.h.  */

void *
dwarf2_frame_get_fn_data (frame_info_ptr this_frame, void **this_cache,
			  fn_prev_register cookie)
{
  struct dwarf2_frame_fn_data *fn_data = nullptr;
  struct dwarf2_frame_cache *cache
    = dwarf2_frame_cache (this_frame, this_cache);

  /* Find the object for the function.  */
  for (fn_data = cache->fn_data; fn_data; fn_data = fn_data->next)
    if (fn_data->cookie == cookie)
      return fn_data->data;

  return nullptr;
}

/* See frame.h.  */

void *
dwarf2_frame_allocate_fn_data (frame_info_ptr this_frame, void **this_cache,
			       fn_prev_register cookie, unsigned long size)
{
  struct dwarf2_frame_fn_data *fn_data = nullptr;
  struct dwarf2_frame_cache *cache
    = dwarf2_frame_cache (this_frame, this_cache);

  /* First try to find an existing object.  */
  void *data = dwarf2_frame_get_fn_data (this_frame, this_cache, cookie);
  gdb_assert (data == nullptr);

  /* No object found, lets create a new instance.  */
  fn_data = FRAME_OBSTACK_ZALLOC (struct dwarf2_frame_fn_data);
  fn_data->cookie = cookie;
  fn_data->data = frame_obstack_zalloc (size);
  fn_data->next = cache->fn_data;
  cache->fn_data = fn_data;

  return fn_data->data;
}

/* Proxy for tailcall_frame_dealloc_cache for bottom frame of a virtual tail
   call frames chain.  */

static void
dwarf2_frame_dealloc_cache (frame_info *self, void *this_cache)
{
  struct dwarf2_frame_cache *cache
      = dwarf2_frame_cache (frame_info_ptr (self), &this_cache);

  if (cache->tailcall_cache)
    dwarf2_tailcall_frame_unwind.dealloc_cache (self, cache->tailcall_cache);
}

static int
dwarf2_frame_sniffer (const struct frame_unwind *self,
		      frame_info_ptr this_frame, void **this_cache)
{
  if (!dwarf2_frame_unwinders_enabled_p)
    return 0;

  /* Grab an address that is guaranteed to reside somewhere within the
     function.  get_frame_pc(), with a no-return next function, can
     end up returning something past the end of this function's body.
     If the frame we're sniffing for is a signal frame whose start
     address is placed on the stack by the OS, its FDE must
     extend one byte before its start address or we could potentially
     select the FDE of the previous function.  */
  CORE_ADDR block_addr = get_frame_address_in_block (this_frame);
  struct dwarf2_fde *fde = dwarf2_frame_find_fde (&block_addr, NULL);

  if (!fde)
    return 0;

  /* On some targets, signal trampolines may have unwind information.
     We need to recognize them so that we set the frame type
     correctly.  */

  if (fde->cie->signal_frame
      || dwarf2_frame_signal_frame_p (get_frame_arch (this_frame),
				      this_frame))
    return self->type == SIGTRAMP_FRAME;

  if (self->type != NORMAL_FRAME)
    return 0;

  return 1;
}

static const struct frame_unwind dwarf2_frame_unwind =
{
  "dwarf2",
  NORMAL_FRAME,
  dwarf2_frame_unwind_stop_reason,
  dwarf2_frame_this_id,
  dwarf2_frame_prev_register,
  NULL,
  dwarf2_frame_sniffer,
  dwarf2_frame_dealloc_cache
};

static const struct frame_unwind dwarf2_signal_frame_unwind =
{
  "dwarf2 signal",
  SIGTRAMP_FRAME,
  dwarf2_frame_unwind_stop_reason,
  dwarf2_frame_this_id,
  dwarf2_frame_prev_register,
  NULL,
  dwarf2_frame_sniffer,

  /* TAILCALL_CACHE can never be in such frame to need dealloc_cache.  */
  NULL
};

/* Append the DWARF-2 frame unwinders to GDBARCH's list.  */

void
dwarf2_append_unwinders (struct gdbarch *gdbarch)
{
  frame_unwind_append_unwinder (gdbarch, &dwarf2_frame_unwind);
  frame_unwind_append_unwinder (gdbarch, &dwarf2_signal_frame_unwind);
}


/* There is no explicitly defined relationship between the CFA and the
   location of frame's local variables and arguments/parameters.
   Therefore, frame base methods on this page should probably only be
   used as a last resort, just to avoid printing total garbage as a
   response to the "info frame" command.  */

static CORE_ADDR
dwarf2_frame_base_address (frame_info_ptr this_frame, void **this_cache)
{
  struct dwarf2_frame_cache *cache =
    dwarf2_frame_cache (this_frame, this_cache);

  return cache->cfa;
}

static const struct frame_base dwarf2_frame_base =
{
  &dwarf2_frame_unwind,
  dwarf2_frame_base_address,
  dwarf2_frame_base_address,
  dwarf2_frame_base_address
};

const struct frame_base *
dwarf2_frame_base_sniffer (frame_info_ptr this_frame)
{
  CORE_ADDR block_addr = get_frame_address_in_block (this_frame);

  if (dwarf2_frame_find_fde (&block_addr, NULL))
    return &dwarf2_frame_base;

  return NULL;
}

/* Compute the CFA for THIS_FRAME, but only if THIS_FRAME came from
   the DWARF unwinder.  This is used to implement
   DW_OP_call_frame_cfa.  */

CORE_ADDR
dwarf2_frame_cfa (frame_info_ptr this_frame)
{
  if (frame_unwinder_is (this_frame, &record_btrace_tailcall_frame_unwind)
      || frame_unwinder_is (this_frame, &record_btrace_frame_unwind))
    throw_error (NOT_AVAILABLE_ERROR,
		 _("cfa not available for record btrace target"));

  while (get_frame_type (this_frame) == INLINE_FRAME)
    this_frame = get_prev_frame (this_frame);
  if (get_frame_unwind_stop_reason (this_frame) == UNWIND_UNAVAILABLE)
    throw_error (NOT_AVAILABLE_ERROR,
		_("can't compute CFA for this frame: "
		  "required registers or memory are unavailable"));

  if (get_frame_id (this_frame).stack_status != FID_STACK_VALID)
    throw_error (NOT_AVAILABLE_ERROR,
		_("can't compute CFA for this frame: "
		  "frame base not available"));

  return get_frame_base (this_frame);
}

/* We store the frame data on the BFD.  This is only done if it is
   independent of the address space and so can be shared.  */
static const registry<bfd>::key<comp_unit> dwarf2_frame_bfd_data;

/* If any BFD sections require relocations (note; really should be if
   any debug info requires relocations), then we store the frame data
   on the objfile instead, and do not share it.  */
static const registry<objfile>::key<comp_unit> dwarf2_frame_objfile_data;


/* Pointer encoding helper functions.  */

/* GCC supports exception handling based on DWARF2 CFI.  However, for
   technical reasons, it encodes addresses in its FDE's in a different
   way.  Several "pointer encodings" are supported.  The encoding
   that's used for a particular FDE is determined by the 'R'
   augmentation in the associated CIE.  The argument of this
   augmentation is a single byte.  

   The address can be encoded as 2 bytes, 4 bytes, 8 bytes, or as a
   LEB128.  This is encoded in bits 0, 1 and 2.  Bit 3 encodes whether
   the address is signed or unsigned.  Bits 4, 5 and 6 encode how the
   address should be interpreted (absolute, relative to the current
   position in the FDE, ...).  Bit 7, indicates that the address
   should be dereferenced.  */

static gdb_byte
encoding_for_size (unsigned int size)
{
  switch (size)
    {
    case 2:
      return DW_EH_PE_udata2;
    case 4:
      return DW_EH_PE_udata4;
    case 8:
      return DW_EH_PE_udata8;
    default:
      internal_error (_("Unsupported address size"));
    }
}

static ULONGEST
read_encoded_value (struct comp_unit *unit, gdb_byte encoding,
		    int ptr_len, const gdb_byte *buf,
		    unsigned int *bytes_read_ptr,
		    unrelocated_addr func_base)
{
  ptrdiff_t offset;
  ULONGEST base;

  /* GCC currently doesn't generate DW_EH_PE_indirect encodings for
     FDE's.  */
  if (encoding & DW_EH_PE_indirect)
    internal_error (_("Unsupported encoding: DW_EH_PE_indirect"));

  *bytes_read_ptr = 0;

  switch (encoding & 0x70)
    {
    case DW_EH_PE_absptr:
      base = 0;
      break;
    case DW_EH_PE_pcrel:
      base = bfd_section_vma (unit->dwarf_frame_section);
      base += (buf - unit->dwarf_frame_buffer);
      break;
    case DW_EH_PE_datarel:
      base = unit->dbase;
      break;
    case DW_EH_PE_textrel:
      base = unit->tbase;
      break;
    case DW_EH_PE_funcrel:
      base = (ULONGEST) func_base;
      break;
    case DW_EH_PE_aligned:
      base = 0;
      offset = buf - unit->dwarf_frame_buffer;
      if ((offset % ptr_len) != 0)
	{
	  *bytes_read_ptr = ptr_len - (offset % ptr_len);
	  buf += *bytes_read_ptr;
	}
      break;
    default:
      internal_error (_("Invalid or unsupported encoding"));
    }

  if ((encoding & 0x07) == 0x00)
    {
      encoding |= encoding_for_size (ptr_len);
      if (bfd_get_sign_extend_vma (unit->abfd))
	encoding |= DW_EH_PE_signed;
    }

  switch (encoding & 0x0f)
    {
    case DW_EH_PE_uleb128:
      {
	uint64_t value;
	const gdb_byte *end_buf = buf + (sizeof (value) + 1) * 8 / 7;

	*bytes_read_ptr += safe_read_uleb128 (buf, end_buf, &value) - buf;
	return base + value;
      }
    case DW_EH_PE_udata2:
      *bytes_read_ptr += 2;
      return (base + bfd_get_16 (unit->abfd, (bfd_byte *) buf));
    case DW_EH_PE_udata4:
      *bytes_read_ptr += 4;
      return (base + bfd_get_32 (unit->abfd, (bfd_byte *) buf));
    case DW_EH_PE_udata8:
      *bytes_read_ptr += 8;
      return (base + bfd_get_64 (unit->abfd, (bfd_byte *) buf));
    case DW_EH_PE_sleb128:
      {
	int64_t value;
	const gdb_byte *end_buf = buf + (sizeof (value) + 1) * 8 / 7;

	*bytes_read_ptr += safe_read_sleb128 (buf, end_buf, &value) - buf;
	return base + value;
      }
    case DW_EH_PE_sdata2:
      *bytes_read_ptr += 2;
      return (base + bfd_get_signed_16 (unit->abfd, (bfd_byte *) buf));
    case DW_EH_PE_sdata4:
      *bytes_read_ptr += 4;
      return (base + bfd_get_signed_32 (unit->abfd, (bfd_byte *) buf));
    case DW_EH_PE_sdata8:
      *bytes_read_ptr += 8;
      return (base + bfd_get_signed_64 (unit->abfd, (bfd_byte *) buf));
    default:
      internal_error (_("Invalid or unsupported encoding"));
    }
}


/* Find CIE with the given CIE_POINTER in CIE_TABLE.  */
static struct dwarf2_cie *
find_cie (const dwarf2_cie_table &cie_table, ULONGEST cie_pointer)
{
  auto iter = cie_table.find (cie_pointer);
  if (iter != cie_table.end ())
    return iter->second;
  return NULL;
}

static inline int
bsearch_fde_cmp (const dwarf2_fde *fde, unrelocated_addr seek_pc)
{
  if (fde->end_addr () <= seek_pc)
    return -1;
  if (fde->initial_location <= seek_pc)
    return 0;
  return 1;
}

/* Find an existing comp_unit for an objfile, if any.  */

static comp_unit *
find_comp_unit (struct objfile *objfile)
{
  bfd *abfd = objfile->obfd.get ();
  if (gdb_bfd_requires_relocations (abfd))
    return dwarf2_frame_objfile_data.get (objfile);

  return dwarf2_frame_bfd_data.get (abfd);
}

/* Store the comp_unit on OBJFILE, or the corresponding BFD, as
   appropriate.  */

static void
set_comp_unit (struct objfile *objfile, struct comp_unit *unit)
{
  bfd *abfd = objfile->obfd.get ();
  if (gdb_bfd_requires_relocations (abfd))
    return dwarf2_frame_objfile_data.set (objfile, unit);

  return dwarf2_frame_bfd_data.set (abfd, unit);
}

/* Find the FDE for *PC.  Return a pointer to the FDE, and store the
   initial location associated with it into *PC.  */

static struct dwarf2_fde *
dwarf2_frame_find_fde (CORE_ADDR *pc, dwarf2_per_objfile **out_per_objfile)
{
  for (objfile *objfile : current_program_space->objfiles ())
    {
      CORE_ADDR offset;

      if (objfile->obfd == nullptr)
	continue;

      comp_unit *unit = find_comp_unit (objfile);
      if (unit == NULL)
	{
	  dwarf2_build_frame_info (objfile);
	  unit = find_comp_unit (objfile);
	}
      gdb_assert (unit != NULL);

      dwarf2_fde_table *fde_table = &unit->fde_table;
      if (fde_table->empty ())
	continue;

      gdb_assert (!objfile->section_offsets.empty ());
      offset = objfile->text_section_offset ();

      gdb_assert (!fde_table->empty ());
      unrelocated_addr seek_pc = (unrelocated_addr) (*pc - offset);
      if (seek_pc < (*fde_table)[0]->initial_location)
	continue;

      auto it = gdb::binary_search (fde_table->begin (), fde_table->end (),
				    seek_pc, bsearch_fde_cmp);
      if (it != fde_table->end ())
	{
	  *pc = (CORE_ADDR) (*it)->initial_location + offset;
	  if (out_per_objfile != nullptr)
	    *out_per_objfile = get_dwarf2_per_objfile (objfile);

	  return *it;
	}
    }
  return NULL;
}

/* Add FDE to FDE_TABLE.  */
static void
add_fde (dwarf2_fde_table *fde_table, struct dwarf2_fde *fde)
{
  if (fde->address_range == 0)
    /* Discard useless FDEs.  */
    return;

  fde_table->push_back (fde);
}

#define DW64_CIE_ID 0xffffffffffffffffULL

/* Defines the type of eh_frames that are expected to be decoded: CIE, FDE
   or any of them.  */

enum eh_frame_type
{
  EH_CIE_TYPE_ID = 1 << 0,
  EH_FDE_TYPE_ID = 1 << 1,
  EH_CIE_OR_FDE_TYPE_ID = EH_CIE_TYPE_ID | EH_FDE_TYPE_ID
};

static const gdb_byte *decode_frame_entry (struct gdbarch *gdbarch,
					   struct comp_unit *unit,
					   const gdb_byte *start,
					   int eh_frame_p,
					   dwarf2_cie_table &cie_table,
					   dwarf2_fde_table *fde_table,
					   enum eh_frame_type entry_type);

/* Decode the next CIE or FDE, entry_type specifies the expected type.
   Return NULL if invalid input, otherwise the next byte to be processed.  */

static const gdb_byte *
decode_frame_entry_1 (struct gdbarch *gdbarch,
		      struct comp_unit *unit, const gdb_byte *start,
		      int eh_frame_p,
		      dwarf2_cie_table &cie_table,
		      dwarf2_fde_table *fde_table,
		      enum eh_frame_type entry_type)
{
  const gdb_byte *buf, *end;
  ULONGEST length;
  unsigned int bytes_read;
  int dwarf64_p;
  ULONGEST cie_id;
  ULONGEST cie_pointer;
  int64_t sleb128;
  uint64_t uleb128;

  buf = start;
  length = read_initial_length (unit->abfd, buf, &bytes_read, false);
  buf += bytes_read;
  end = buf + (size_t) length;

  if (length == 0)
    return end;

  /* Are we still within the section?  */
  if (end <= buf || end > unit->dwarf_frame_buffer + unit->dwarf_frame_size)
    return NULL;

  /* Distinguish between 32 and 64-bit encoded frame info.  */
  dwarf64_p = (bytes_read == 12);

  /* In a .eh_frame section, zero is used to distinguish CIEs from FDEs.  */
  if (eh_frame_p)
    cie_id = 0;
  else if (dwarf64_p)
    cie_id = DW64_CIE_ID;
  else
    cie_id = DW_CIE_ID;

  if (dwarf64_p)
    {
      cie_pointer = read_8_bytes (unit->abfd, buf);
      buf += 8;
    }
  else
    {
      cie_pointer = read_4_bytes (unit->abfd, buf);
      buf += 4;
    }

  if (cie_pointer == cie_id)
    {
      /* This is a CIE.  */
      struct dwarf2_cie *cie;
      const char *augmentation;
      unsigned int cie_version;

      /* Check that a CIE was expected.  */
      if ((entry_type & EH_CIE_TYPE_ID) == 0)
	error (_("Found a CIE when not expecting it."));

      /* Record the offset into the .debug_frame section of this CIE.  */
      cie_pointer = start - unit->dwarf_frame_buffer;

      /* Check whether we've already read it.  */
      if (find_cie (cie_table, cie_pointer))
	return end;

      cie = XOBNEW (&unit->obstack, struct dwarf2_cie);
      cie->initial_instructions = NULL;
      cie->cie_pointer = cie_pointer;

      /* The encoding for FDE's in a normal .debug_frame section
	 depends on the target address size.  */
      cie->encoding = DW_EH_PE_absptr;

      /* We'll determine the final value later, but we need to
	 initialize it conservatively.  */
      cie->signal_frame = 0;

      /* Check version number.  */
      cie_version = read_1_byte (unit->abfd, buf);
      if (cie_version != 1 && cie_version != 3 && cie_version != 4)
	return NULL;
      cie->version = cie_version;
      buf += 1;

      /* Interpret the interesting bits of the augmentation.  */
      cie->augmentation = augmentation = (const char *) buf;
      buf += (strlen (augmentation) + 1);

      /* Ignore armcc augmentations.  We only use them for quirks,
	 and that doesn't happen until later.  */
      if (startswith (augmentation, "armcc"))
	augmentation += strlen (augmentation);

      /* The GCC 2.x "eh" augmentation has a pointer immediately
	 following the augmentation string, so it must be handled
	 first.  */
      if (augmentation[0] == 'e' && augmentation[1] == 'h')
	{
	  /* Skip.  */
	  buf += gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT;
	  augmentation += 2;
	}

      if (cie->version >= 4)
	{
	  /* FIXME: check that this is the same as from the CU header.  */
	  cie->addr_size = read_1_byte (unit->abfd, buf);
	  ++buf;
	  cie->segment_size = read_1_byte (unit->abfd, buf);
	  ++buf;
	}
      else
	{
	  cie->addr_size = gdbarch_dwarf2_addr_size (gdbarch);
	  cie->segment_size = 0;
	}
      /* Address values in .eh_frame sections are defined to have the
	 target's pointer size.  Watchout: This breaks frame info for
	 targets with pointer size < address size, unless a .debug_frame
	 section exists as well.  */
      if (eh_frame_p)
	cie->ptr_size = gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT;
      else
	cie->ptr_size = cie->addr_size;

      buf = gdb_read_uleb128 (buf, end, &uleb128);
      if (buf == NULL)
	return NULL;
      cie->code_alignment_factor = uleb128;

      buf = gdb_read_sleb128 (buf, end, &sleb128);
      if (buf == NULL)
	return NULL;
      cie->data_alignment_factor = sleb128;

      if (cie_version == 1)
	{
	  cie->return_address_register = read_1_byte (unit->abfd, buf);
	  ++buf;
	}
      else
	{
	  buf = gdb_read_uleb128 (buf, end, &uleb128);
	  if (buf == NULL)
	    return NULL;
	  cie->return_address_register = uleb128;
	}

      cie->return_address_register
	= dwarf2_frame_adjust_regnum (gdbarch,
				      cie->return_address_register,
				      eh_frame_p);

      cie->saw_z_augmentation = (*augmentation == 'z');
      if (cie->saw_z_augmentation)
	{
	  uint64_t uleb_length;

	  buf = gdb_read_uleb128 (buf, end, &uleb_length);
	  if (buf == NULL)
	    return NULL;
	  cie->initial_instructions = buf + uleb_length;
	  augmentation++;
	}

      while (*augmentation)
	{
	  /* "L" indicates a byte showing how the LSDA pointer is encoded.  */
	  if (*augmentation == 'L')
	    {
	      /* Skip.  */
	      buf++;
	      augmentation++;
	    }

	  /* "R" indicates a byte indicating how FDE addresses are encoded.  */
	  else if (*augmentation == 'R')
	    {
	      cie->encoding = *buf++;
	      augmentation++;
	    }

	  /* "P" indicates a personality routine in the CIE augmentation.  */
	  else if (*augmentation == 'P')
	    {
	      /* Skip.  Avoid indirection since we throw away the result.  */
	      gdb_byte encoding = (*buf++) & ~DW_EH_PE_indirect;
	      read_encoded_value (unit, encoding, cie->ptr_size,
				  buf, &bytes_read, (unrelocated_addr) 0);
	      buf += bytes_read;
	      augmentation++;
	    }

	  /* "S" indicates a signal frame, such that the return
	     address must not be decremented to locate the call frame
	     info for the previous frame; it might even be the first
	     instruction of a function, so decrementing it would take
	     us to a different function.  */
	  else if (*augmentation == 'S')
	    {
	      cie->signal_frame = 1;
	      augmentation++;
	    }

	  /* Otherwise we have an unknown augmentation.  Assume that either
	     there is no augmentation data, or we saw a 'z' prefix.  */
	  else
	    {
	      if (cie->initial_instructions)
		buf = cie->initial_instructions;
	      break;
	    }
	}

      cie->initial_instructions = buf;
      cie->end = end;
      cie->unit = unit;

      cie_table[cie->cie_pointer] = cie;
    }
  else
    {
      /* This is a FDE.  */
      struct dwarf2_fde *fde;

      /* Check that an FDE was expected.  */
      if ((entry_type & EH_FDE_TYPE_ID) == 0)
	error (_("Found an FDE when not expecting it."));

      /* In an .eh_frame section, the CIE pointer is the delta between the
	 address within the FDE where the CIE pointer is stored and the
	 address of the CIE.  Convert it to an offset into the .eh_frame
	 section.  */
      if (eh_frame_p)
	{
	  cie_pointer = buf - unit->dwarf_frame_buffer - cie_pointer;
	  cie_pointer -= (dwarf64_p ? 8 : 4);
	}

      /* In either case, validate the result is still within the section.  */
      if (cie_pointer >= unit->dwarf_frame_size)
	return NULL;

      fde = XOBNEW (&unit->obstack, struct dwarf2_fde);
      fde->cie = find_cie (cie_table, cie_pointer);
      if (fde->cie == NULL)
	{
	  decode_frame_entry (gdbarch, unit,
			      unit->dwarf_frame_buffer + cie_pointer,
			      eh_frame_p, cie_table, fde_table,
			      EH_CIE_TYPE_ID);
	  fde->cie = find_cie (cie_table, cie_pointer);
	}

      gdb_assert (fde->cie != NULL);

      ULONGEST init_addr
	= read_encoded_value (unit, fde->cie->encoding, fde->cie->ptr_size,
			      buf, &bytes_read, (unrelocated_addr) 0);
      fde->initial_location
	= (unrelocated_addr) gdbarch_adjust_dwarf2_addr (gdbarch, init_addr);
      buf += bytes_read;

      ULONGEST range
	= read_encoded_value (unit, fde->cie->encoding & 0x0f,
			      fde->cie->ptr_size, buf, &bytes_read,
			      (unrelocated_addr) 0);
      ULONGEST addr = gdbarch_adjust_dwarf2_addr (gdbarch, init_addr + range);
      fde->address_range = addr - (ULONGEST) fde->initial_location;
      buf += bytes_read;

      /* A 'z' augmentation in the CIE implies the presence of an
	 augmentation field in the FDE as well.  The only thing known
	 to be in here at present is the LSDA entry for EH.  So we
	 can skip the whole thing.  */
      if (fde->cie->saw_z_augmentation)
	{
	  uint64_t uleb_length;

	  buf = gdb_read_uleb128 (buf, end, &uleb_length);
	  if (buf == NULL)
	    return NULL;
	  buf += uleb_length;
	  if (buf > end)
	    return NULL;
	}

      fde->instructions = buf;
      fde->end = end;

      fde->eh_frame_p = eh_frame_p;

      add_fde (fde_table, fde);
    }

  return end;
}

/* Read a CIE or FDE in BUF and decode it. Entry_type specifies whether we
   expect an FDE or a CIE.  */

static const gdb_byte *
decode_frame_entry (struct gdbarch *gdbarch,
		    struct comp_unit *unit, const gdb_byte *start,
		    int eh_frame_p,
		    dwarf2_cie_table &cie_table,
		    dwarf2_fde_table *fde_table,
		    enum eh_frame_type entry_type)
{
  enum { NONE, ALIGN4, ALIGN8, FAIL } workaround = NONE;
  const gdb_byte *ret;
  ptrdiff_t start_offset;

  while (1)
    {
      ret = decode_frame_entry_1 (gdbarch, unit, start, eh_frame_p,
				  cie_table, fde_table, entry_type);
      if (ret != NULL)
	break;

      /* We have corrupt input data of some form.  */

      /* ??? Try, weakly, to work around compiler/assembler/linker bugs
	 and mismatches wrt padding and alignment of debug sections.  */
      /* Note that there is no requirement in the standard for any
	 alignment at all in the frame unwind sections.  Testing for
	 alignment before trying to interpret data would be incorrect.

	 However, GCC traditionally arranged for frame sections to be
	 sized such that the FDE length and CIE fields happen to be
	 aligned (in theory, for performance).  This, unfortunately,
	 was done with .align directives, which had the side effect of
	 forcing the section to be aligned by the linker.

	 This becomes a problem when you have some other producer that
	 creates frame sections that are not as strictly aligned.  That
	 produces a hole in the frame info that gets filled by the 
	 linker with zeros.

	 The GCC behaviour is arguably a bug, but it's effectively now
	 part of the ABI, so we're now stuck with it, at least at the
	 object file level.  A smart linker may decide, in the process
	 of compressing duplicate CIE information, that it can rewrite
	 the entire output section without this extra padding.  */

      start_offset = start - unit->dwarf_frame_buffer;
      if (workaround < ALIGN4 && (start_offset & 3) != 0)
	{
	  start += 4 - (start_offset & 3);
	  workaround = ALIGN4;
	  continue;
	}
      if (workaround < ALIGN8 && (start_offset & 7) != 0)
	{
	  start += 8 - (start_offset & 7);
	  workaround = ALIGN8;
	  continue;
	}

      /* Nothing left to try.  Arrange to return as if we've consumed
	 the entire input section.  Hopefully we'll get valid info from
	 the other of .debug_frame/.eh_frame.  */
      workaround = FAIL;
      ret = unit->dwarf_frame_buffer + unit->dwarf_frame_size;
      break;
    }

  switch (workaround)
    {
    case NONE:
      break;

    case ALIGN4:
      complaint (_("\
Corrupt data in %s:%s; align 4 workaround apparently succeeded"),
		 bfd_get_filename (unit->dwarf_frame_section->owner),
		 bfd_section_name (unit->dwarf_frame_section));
      break;

    case ALIGN8:
      complaint (_("\
Corrupt data in %s:%s; align 8 workaround apparently succeeded"),
		 bfd_get_filename (unit->dwarf_frame_section->owner),
		 bfd_section_name (unit->dwarf_frame_section));
      break;

    default:
      complaint (_("Corrupt data in %s:%s"),
		 bfd_get_filename (unit->dwarf_frame_section->owner),
		 bfd_section_name (unit->dwarf_frame_section));
      break;
    }

  return ret;
}

static bool
fde_is_less_than (const dwarf2_fde *aa, const dwarf2_fde *bb)
{
  if (aa->initial_location == bb->initial_location)
    {
      if (aa->address_range != bb->address_range
	  && aa->eh_frame_p == 0 && bb->eh_frame_p == 0)
	/* Linker bug, e.g. gold/10400.
	   Work around it by keeping stable sort order.  */
	return aa < bb;
      else
	/* Put eh_frame entries after debug_frame ones.  */
	return aa->eh_frame_p < bb->eh_frame_p;
    }

  return aa->initial_location < bb->initial_location;
}

void
dwarf2_build_frame_info (struct objfile *objfile)
{
  const gdb_byte *frame_ptr;
  dwarf2_cie_table cie_table;
  dwarf2_fde_table fde_table;

  struct gdbarch *gdbarch = objfile->arch ();

  /* Build a minimal decoding of the DWARF2 compilation unit.  */
  auto unit = std::make_unique<comp_unit> (objfile);

  if (objfile->separate_debug_objfile_backlink == NULL)
    {
      /* Do not read .eh_frame from separate file as they must be also
	 present in the main file.  */
      dwarf2_get_section_info (objfile, DWARF2_EH_FRAME,
			       &unit->dwarf_frame_section,
			       &unit->dwarf_frame_buffer,
			       &unit->dwarf_frame_size);
      if (unit->dwarf_frame_size)
	{
	  asection *got, *txt;

	  /* FIXME: kettenis/20030602: This is the DW_EH_PE_datarel base
	     that is used for the i386/amd64 target, which currently is
	     the only target in GCC that supports/uses the
	     DW_EH_PE_datarel encoding.  */
	  got = bfd_get_section_by_name (unit->abfd, ".got");
	  if (got)
	    unit->dbase = got->vma;

	  /* GCC emits the DW_EH_PE_textrel encoding type on sh and ia64
	     so far.  */
	  txt = bfd_get_section_by_name (unit->abfd, ".text");
	  if (txt)
	    unit->tbase = txt->vma;

	  try
	    {
	      frame_ptr = unit->dwarf_frame_buffer;
	      while (frame_ptr < unit->dwarf_frame_buffer + unit->dwarf_frame_size)
		frame_ptr = decode_frame_entry (gdbarch, unit.get (),
						frame_ptr, 1,
						cie_table, &fde_table,
						EH_CIE_OR_FDE_TYPE_ID);
	    }

	  catch (const gdb_exception_error &e)
	    {
	      warning (_("skipping .eh_frame info of %s: %s"),
		       objfile_name (objfile), e.what ());

	      fde_table.clear ();
	      /* The cie_table is discarded below.  */
	    }

	  cie_table.clear ();
	}
    }

  dwarf2_get_section_info (objfile, DWARF2_DEBUG_FRAME,
			   &unit->dwarf_frame_section,
			   &unit->dwarf_frame_buffer,
			   &unit->dwarf_frame_size);
  if (unit->dwarf_frame_size)
    {
      size_t num_old_fde_entries = fde_table.size ();

      try
	{
	  frame_ptr = unit->dwarf_frame_buffer;
	  while (frame_ptr < unit->dwarf_frame_buffer + unit->dwarf_frame_size)
	    frame_ptr = decode_frame_entry (gdbarch, unit.get (), frame_ptr, 0,
					    cie_table, &fde_table,
					    EH_CIE_OR_FDE_TYPE_ID);
	}
      catch (const gdb_exception_error &e)
	{
	  warning (_("skipping .debug_frame info of %s: %s"),
		   objfile_name (objfile), e.what ());

	  fde_table.resize (num_old_fde_entries);
	}
    }

  struct dwarf2_fde *fde_prev = NULL;
  struct dwarf2_fde *first_non_zero_fde = NULL;

  /* Prepare FDE table for lookups.  */
  std::sort (fde_table.begin (), fde_table.end (), fde_is_less_than);

  /* Check for leftovers from --gc-sections.  The GNU linker sets
     the relevant symbols to zero, but doesn't zero the FDE *end*
     ranges because there's no relocation there.  It's (offset,
     length), not (start, end).  On targets where address zero is
     just another valid address this can be a problem, since the
     FDEs appear to be non-empty in the output --- we could pick
     out the wrong FDE.  To work around this, when overlaps are
     detected, we prefer FDEs that do not start at zero.

     Start by finding the first FDE with non-zero start.  Below
     we'll discard all FDEs that start at zero and overlap this
     one.  */
  for (struct dwarf2_fde *fde : fde_table)
    {
      if (fde->initial_location != (unrelocated_addr) 0)
	{
	  first_non_zero_fde = fde;
	  break;
	}
    }

  /* Since we'll be doing bsearch, squeeze out identical (except
     for eh_frame_p) fde entries so bsearch result is predictable.
     Also discard leftovers from --gc-sections.  */
  for (struct dwarf2_fde *fde : fde_table)
    {
      if (fde->initial_location == (unrelocated_addr) 0
	  && first_non_zero_fde != NULL
	  && first_non_zero_fde->initial_location < fde->end_addr ())
	continue;

      if (fde_prev != NULL
	  && fde_prev->initial_location == fde->initial_location)
	continue;

      unit->fde_table.push_back (fde);
      fde_prev = fde;
    }
  unit->fde_table.shrink_to_fit ();

  set_comp_unit (objfile, unit.release ());
}

/* Handle 'maintenance show dwarf unwinders'.  */

static void
show_dwarf_unwinders_enabled_p (struct ui_file *file, int from_tty,
				struct cmd_list_element *c,
				const char *value)
{
  gdb_printf (file,
	      _("The DWARF stack unwinders are currently %s.\n"),
	      value);
}

void _initialize_dwarf2_frame ();
void
_initialize_dwarf2_frame ()
{
  add_setshow_boolean_cmd ("unwinders", class_obscure,
			   &dwarf2_frame_unwinders_enabled_p , _("\
Set whether the DWARF stack frame unwinders are used."), _("\
Show whether the DWARF stack frame unwinders are used."), _("\
When enabled the DWARF stack frame unwinders can be used for architectures\n\
that support the DWARF unwinders.  Enabling the DWARF unwinders for an\n\
architecture that doesn't support them will have no effect."),
			   NULL,
			   show_dwarf_unwinders_enabled_p,
			   &set_dwarf_cmdlist,
			   &show_dwarf_cmdlist);

#if GDB_SELF_TEST
  selftests::register_test_foreach_arch ("execute_cfa_program",
					 selftests::execute_cfa_program_test);
#endif
}
