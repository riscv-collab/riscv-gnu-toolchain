/* Target-dependent code for the S12Z, for the GDB.
   Copyright (C) 2018-2024 Free Software Foundation, Inc.

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

/* Much of this file is shamelessly copied from or1k-tdep.c and others.  */

#include "defs.h"

#include "arch-utils.h"
#include "dwarf2/frame.h"
#include "gdbsupport/errors.h"
#include "frame-unwind.h"
#include "gdbcore.h"
#include "gdbcmd.h"
#include "inferior.h"
#include "opcode/s12z.h"
#include "trad-frame.h"
#include "remote.h"
#include "opcodes/s12z-opc.h"
#include "gdbarch.h"
#include "disasm.h"

/* Two of the registers included in S12Z_N_REGISTERS are
   the CCH and CCL "registers" which are just views into
   the CCW register.  */
#define N_PHYSICAL_REGISTERS (S12Z_N_REGISTERS - 2)


/*  A permutation of all the physical registers.   Indexing this array
    with an integer from gdb's internal representation will return the
    register enum.  */
static const int reg_perm[N_PHYSICAL_REGISTERS] =
  {
   REG_D0,
   REG_D1,
   REG_D2,
   REG_D3,
   REG_D4,
   REG_D5,
   REG_D6,
   REG_D7,
   REG_X,
   REG_Y,
   REG_S,
   REG_P,
   REG_CCW
  };

/*  The inverse of the above permutation.  Indexing this
    array with a register enum (e.g. REG_D2) will return the register
    number in gdb's internal representation.  */
static const int inv_reg_perm[N_PHYSICAL_REGISTERS] =
  {
   2, 3, 4, 5,      /* d2, d3, d4, d5 */
   0, 1,            /* d0, d1 */
   6, 7,            /* d6, d7 */
   8, 9, 10, 11, 12 /* x, y, s, p, ccw */
  };

/*  Return the name of the register REGNUM.  */
static const char *
s12z_register_name (struct gdbarch *gdbarch, int regnum)
{
  /*  Registers is declared in opcodes/s12z.h.   */
  return registers[reg_perm[regnum]].name;
}

static CORE_ADDR
s12z_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  CORE_ADDR start_pc = 0;

  if (find_pc_partial_function (pc, NULL, &start_pc, NULL))
    {
      CORE_ADDR prologue_end = skip_prologue_using_sal (gdbarch, pc);

      if (prologue_end != 0)
	return prologue_end;
    }

  warning (_("%s Failed to find end of prologue PC = %08x"),
	   __FUNCTION__, (unsigned int) pc);

  return pc;
}

static struct type *
s12z_register_type (struct gdbarch *gdbarch, int reg_nr)
{
  switch (registers[reg_perm[reg_nr]].bytes)
    {
    case 1:
      return builtin_type (gdbarch)->builtin_uint8;
    case 2:
      return builtin_type (gdbarch)->builtin_uint16;
    case 3:
      return builtin_type (gdbarch)->builtin_uint24;
    case 4:
      return builtin_type (gdbarch)->builtin_uint32;
    default:
      return builtin_type (gdbarch)->builtin_uint32;
    }
  return builtin_type (gdbarch)->builtin_int0;
}


static int
s12z_dwarf_reg_to_regnum (struct gdbarch *gdbarch, int num)
{
  switch (num)
    {
    case 15:      return REG_S;
    case 7:       return REG_X;
    case 8:       return REG_Y;
    case 42:      return REG_D0;
    case 43:      return REG_D1;
    case 44:      return REG_D2;
    case 45:      return REG_D3;
    case 46:      return REG_D4;
    case 47:      return REG_D5;
    case 48:      return REG_D6;
    case 49:      return REG_D7;
    }
  return -1;
}


/* Support functions for frame handling.  */

/* A struct (based on mem_read_abstraction_base) to read memory
   through the disassemble_info API.  */
struct mem_read_abstraction
{
  struct mem_read_abstraction_base base; /* The parent struct.  */
  bfd_vma memaddr;                  /* Where to read from.  */
  struct disassemble_info* info;  /* The disassembler  to use for reading.  */
};

/* Advance the reader by one byte.  */
static void
advance (struct mem_read_abstraction_base *b)
{
  struct mem_read_abstraction *mra = (struct mem_read_abstraction *) b;
  mra->memaddr++;
}

/* Return the current position of the reader.  */
static bfd_vma
posn (struct mem_read_abstraction_base *b)
{
  struct mem_read_abstraction *mra = (struct mem_read_abstraction *) b;
  return mra->memaddr;
}

/* Read the N bytes at OFFSET  using B.  The bytes read are stored in BYTES.
   It is the caller's responsibility to ensure that this is of at least N
   in size.  */
static int
abstract_read_memory (struct mem_read_abstraction_base *b,
		      int offset,
		      size_t n, bfd_byte *bytes)
{
  struct mem_read_abstraction *mra = (struct mem_read_abstraction *) b;

  int status =
    (*mra->info->read_memory_func) (mra->memaddr + offset,
				    bytes, n, mra->info);

  if (status != 0)
    {
      (*mra->info->memory_error_func) (status, mra->memaddr, mra->info);
      return -1;
    }

  return 0;
}


/* Return the stack adjustment caused by a push or pull instruction.  */
static int
push_pull_get_stack_adjustment (int n_operands,
				struct operand *const *operands)
{
  int stack_adjustment = 0;
  gdb_assert (n_operands > 0);
  if (operands[0]->cl == OPND_CL_REGISTER_ALL)
    stack_adjustment = 26;  /* All the regs are involved.  */
  else if (operands[0]->cl == OPND_CL_REGISTER_ALL16)
    stack_adjustment = 4 * 2; /* All four 16 bit regs are involved.  */
  else
    for (int i = 0; i < n_operands; ++i)
      {
	if (operands[i]->cl != OPND_CL_REGISTER)
	  continue; /* I don't think this can ever happen.  */
	const struct register_operand *op
	  = (const struct register_operand *) operands[i];
	switch (op->reg)
	  {
	  case REG_X:
	  case REG_Y:
	    stack_adjustment += 3;
	    break;
	  case REG_D7:
	  case REG_D6:
	    stack_adjustment += 4;
	    break;
	  case REG_D2:
	  case REG_D3:
	  case REG_D4:
	  case REG_D5:
	    stack_adjustment += 2;
	    break;
	  case REG_D0:
	  case REG_D1:
	  case REG_CCL:
	  case REG_CCH:
	    stack_adjustment += 1;
	    break;
	  default:
	    gdb_assert_not_reached ("Invalid register in push/pull operation.");
	    break;
	  }
      }
  return stack_adjustment;
}

/* Initialize a prologue cache.  */

static struct trad_frame_cache *
s12z_frame_cache (frame_info_ptr this_frame, void **prologue_cache)
{
  struct trad_frame_cache *info;

  CORE_ADDR this_sp;
  CORE_ADDR this_sp_for_id;

  CORE_ADDR start_addr;
  CORE_ADDR end_addr;

  /* Nothing to do if we already have this info.  */
  if (NULL != *prologue_cache)
    return (struct trad_frame_cache *) *prologue_cache;

  /* Get a new prologue cache and populate it with default values.  */
  info = trad_frame_cache_zalloc (this_frame);
  *prologue_cache = info;

  /* Find the start address of this function (which is a normal frame, even
     if the next frame is the sentinel frame) and the end of its prologue.  */
  CORE_ADDR this_pc = get_frame_pc (this_frame);
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  find_pc_partial_function (this_pc, NULL, &start_addr, NULL);

  /* Get the stack pointer if we have one (if there's no process executing
     yet we won't have a frame.  */
  this_sp = (NULL == this_frame) ? 0 :
    get_frame_register_unsigned (this_frame, REG_S);

  /* Return early if GDB couldn't find the function.  */
  if (start_addr == 0)
    {
      warning (_("Couldn't find function including address %s SP is %s"),
	       paddress (gdbarch, this_pc),
	       paddress (gdbarch, this_sp));

      /* JPB: 28-Apr-11.  This is a temporary patch, to get round GDB
	 crashing right at the beginning.  Build the frame ID as best we
	 can.  */
      trad_frame_set_id (info, frame_id_build (this_sp, this_pc));

      return info;
    }

  /* The default frame base of this frame (for ID purposes only - frame
     base is an overloaded term) is its stack pointer.  For now we use the
     value of the SP register in this frame.  However if the PC is in the
     prologue of this frame, before the SP has been set up, then the value
     will actually be that of the prev frame, and we'll need to adjust it
     later.  */
  trad_frame_set_this_base (info, this_sp);
  this_sp_for_id = this_sp;

  /* We should only examine code that is in the prologue.  This is all code
     up to (but not including) end_addr.  We should only populate the cache
     while the address is up to (but not including) the PC or end_addr,
     whichever is first.  */
  end_addr = s12z_skip_prologue (gdbarch, start_addr);

  /* All the following analysis only occurs if we are in the prologue and
     have executed the code.  Check we have a sane prologue size, and if
     zero we are frameless and can give up here.  */
  if (end_addr < start_addr)
    error (_("end addr %s is less than start addr %s"),
	   paddress (gdbarch, end_addr), paddress (gdbarch, start_addr));

  CORE_ADDR addr = start_addr; /* Where we have got to?  */
  int frame_size = 0;
  int saved_frame_size = 0;

  struct gdb_non_printing_memory_disassembler dis (gdbarch);

  struct mem_read_abstraction mra;
  mra.base.read = (int (*)(mem_read_abstraction_base*,
			   int, size_t, bfd_byte*)) abstract_read_memory;
  mra.base.advance = advance ;
  mra.base.posn = posn;
  mra.info = dis.disasm_info ();

  while (this_pc > addr)
    {
      enum optr optr = OP_INVALID;
      short osize;
      int n_operands = 0;
      struct operand *operands[6];
      mra.memaddr = addr;
      int n_bytes =
	decode_s12z (&optr, &osize, &n_operands, operands,
		     (mem_read_abstraction_base *) &mra);

      switch (optr)
	{
	case OP_tbNE:
	case OP_tbPL:
	case OP_tbMI:
	case OP_tbGT:
	case OP_tbLE:
	case OP_dbNE:
	case OP_dbEQ:
	case OP_dbPL:
	case OP_dbMI:
	case OP_dbGT:
	case OP_dbLE:
	  /* Conditional Branches.   If any of these are encountered, then
	     it is likely that a RTS will terminate it.  So we need to save
	     the frame size so it can be restored.  */
	  saved_frame_size = frame_size;
	  break;
	case OP_rts:
	  /* Restore the frame size from a previously saved value.  */
	  frame_size = saved_frame_size;
	  break;
	case OP_push:
	  frame_size += push_pull_get_stack_adjustment (n_operands, operands);
	  break;
	case OP_pull:
	  frame_size -= push_pull_get_stack_adjustment (n_operands, operands);
	  break;
	case OP_lea:
	  if (operands[0]->cl == OPND_CL_REGISTER)
	    {
	      int reg = ((struct register_operand *) (operands[0]))->reg;
	      if ((reg == REG_S) && (operands[1]->cl == OPND_CL_MEMORY))
		{
		  const struct memory_operand *mo
		    = (const struct memory_operand * ) operands[1];
		  if (mo->n_regs == 1 && !mo->indirect
		      && mo->regs[0] == REG_S
		      && mo->mutation == OPND_RM_NONE)
		    {
		      /* LEA S, (xxx, S) -- Decrement the stack.   This is
			 almost certainly the start of a frame.  */
		      int simm = (signed char)  mo->base_offset;
		      frame_size -= simm;
		    }
		}
	    }
	  break;
	default:
	  break;
	}
      addr += n_bytes;
      for (int o = 0; o < n_operands; ++o)
	free (operands[o]);
    }

  /* If the PC has not actually got to this point, then the frame
     base will be wrong, and we adjust it. */
  if (this_pc < addr)
    {
      /* Only do if executing.  */
      if (0 != this_sp)
	{
	  this_sp_for_id = this_sp - frame_size;
	  trad_frame_set_this_base (info, this_sp_for_id);
	}
      trad_frame_set_reg_value (info, REG_S, this_sp + 3);
      trad_frame_set_reg_addr (info, REG_P, this_sp);
    }
  else
    {
      gdb_assert (this_sp == this_sp_for_id);
      /* The stack pointer of the prev frame is frame_size greater
	 than the stack pointer of this frame plus one address
	 size (caused by the JSR or BSR).  */
      trad_frame_set_reg_value (info, REG_S,
				this_sp + frame_size + 3);
      trad_frame_set_reg_addr (info, REG_P, this_sp + frame_size);
    }


  /* Build the frame ID.  */
  trad_frame_set_id (info, frame_id_build (this_sp_for_id, start_addr));

  return info;
}

/* Implement the this_id function for the stub unwinder.  */
static void
s12z_frame_this_id (frame_info_ptr this_frame,
		    void **prologue_cache, struct frame_id *this_id)
{
  struct trad_frame_cache *info = s12z_frame_cache (this_frame,
						    prologue_cache);

  trad_frame_get_id (info, this_id);
}


/* Implement the prev_register function for the stub unwinder.  */
static struct value *
s12z_frame_prev_register (frame_info_ptr this_frame,
			  void **prologue_cache, int regnum)
{
  struct trad_frame_cache *info = s12z_frame_cache (this_frame,
						    prologue_cache);

  return trad_frame_get_register (info, this_frame, regnum);
}

/* Data structures for the normal prologue-analysis-based unwinder.  */
static const struct frame_unwind s12z_frame_unwind = {
  "s12z prologue",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  s12z_frame_this_id,
  s12z_frame_prev_register,
  NULL,
  default_frame_sniffer,
  NULL,
};


constexpr gdb_byte s12z_break_insn[] = {0x00};

typedef BP_MANIPULATION (s12z_break_insn) s12z_breakpoint;

struct s12z_gdbarch_tdep : gdbarch_tdep_base
{
};

/*  A vector of human readable characters representing the
    bits in the CCW register.  Unused bits are represented as '-'.
    Lowest significant bit comes first.  */
static const char ccw_bits[] =
  {
   'C',  /* Carry  */
   'V',  /* Two's Complement Overflow  */
   'Z',  /* Zero  */
   'N',  /* Negative  */
   'I',  /* Interrupt  */
   '-',
   'X',  /* Non-Maskable Interrupt  */
   'S',  /* STOP Disable  */
   '0',  /*  Interrupt priority level */
   '0',  /*  ditto  */
   '0',  /*  ditto  */
   '-',
   '-',
   '-',
   '-',
   'U'   /* User/Supervisor State.  */
  };

/* Print a human readable representation of the CCW register.
   For example: "u----000SX-Inzvc" corresponds to the value
   0xD0.  */
static void
s12z_print_ccw_info (struct gdbarch *gdbarch,
		     struct ui_file *file,
		     frame_info_ptr frame,
		     int reg)
{
  value *v = value_of_register (reg, get_next_frame_sentinel_okay (frame));
  const char *name = gdbarch_register_name (gdbarch, reg);
  uint32_t ccw = value_as_long (v);
  gdb_puts (name, file);
  size_t len = strlen (name);
  const int stop_1 = 15;
  const int stop_2 = 17;
  for (int i = 0; i < stop_1 - len; ++i)
    gdb_putc (' ', file);
  gdb_printf (file, "0x%04x", ccw);
  for (int i = 0; i < stop_2 - len; ++i)
    gdb_putc (' ', file);
  for (int b = 15; b >= 0; --b)
    {
      if (ccw & (0x1u << b))
	{
	  if (ccw_bits[b] == 0)
	    gdb_putc ('1', file);
	  else
	    gdb_putc (ccw_bits[b], file);
	}
      else
	gdb_putc (tolower (ccw_bits[b]), file);
    }
  gdb_putc ('\n', file);
}

static void
s12z_print_registers_info (struct gdbarch *gdbarch,
			    struct ui_file *file,
			    frame_info_ptr frame,
			    int regnum, int print_all)
{
  const int numregs = (gdbarch_num_regs (gdbarch)
		       + gdbarch_num_pseudo_regs (gdbarch));

  if (regnum == -1)
    {
      for (int reg = 0; reg < numregs; reg++)
	{
	  if (REG_CCW == reg_perm[reg])
	    {
	      s12z_print_ccw_info (gdbarch, file, frame, reg);
	      continue;
	    }
	  default_print_registers_info (gdbarch, file, frame, reg, print_all);
	}
    }
  else if (REG_CCW == reg_perm[regnum])
    s12z_print_ccw_info (gdbarch, file, frame, regnum);
  else
    default_print_registers_info (gdbarch, file, frame, regnum, print_all);
}




static void
s12z_extract_return_value (struct type *type, struct regcache *regcache,
			      void *valbuf)
{
  int reg = -1;

  switch (type->length ())
    {
    case 0:   /* Nothing to do */
      return;

    case 1:
      reg = REG_D0;
      break;

    case 2:
      reg = REG_D2;
      break;

    case 3:
      reg = REG_X;
      break;

    case 4:
      reg = REG_D6;
      break;

    default:
      error (_("bad size for return value"));
      return;
    }

  regcache->cooked_read (inv_reg_perm[reg], (gdb_byte *) valbuf);
}

static enum return_value_convention
s12z_return_value (struct gdbarch *gdbarch, struct value *function,
		   struct type *type, struct regcache *regcache,
		   gdb_byte *readbuf, const gdb_byte *writebuf)
{
  if (type->code () == TYPE_CODE_STRUCT
      || type->code () == TYPE_CODE_UNION
      || type->code () == TYPE_CODE_ARRAY
      || type->length () > 4)
    return RETURN_VALUE_STRUCT_CONVENTION;

  if (readbuf)
    s12z_extract_return_value (type, regcache, readbuf);

  return RETURN_VALUE_REGISTER_CONVENTION;
}


static void
show_bdccsr_command (const char *args, int from_tty)
{
  struct string_file output;
  target_rcmd ("bdccsr", &output);

  gdb_printf ("The current BDCCSR value is %s\n", output.string().c_str());
}

static struct gdbarch *
s12z_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  gdbarch *gdbarch
    = gdbarch_alloc (&info, gdbarch_tdep_up (new s12z_gdbarch_tdep));

  add_cmd ("bdccsr", class_support, show_bdccsr_command,
	   _("Show the current value of the microcontroller's BDCCSR."),
	   &maintenanceinfolist);

  /* Target data types.  */
  set_gdbarch_short_bit (gdbarch, 16);
  set_gdbarch_int_bit (gdbarch, 16);
  set_gdbarch_long_bit (gdbarch, 32);
  set_gdbarch_long_long_bit (gdbarch, 32);
  set_gdbarch_ptr_bit (gdbarch, 24);
  set_gdbarch_addr_bit (gdbarch, 24);
  set_gdbarch_char_signed (gdbarch, 0);

  set_gdbarch_ps_regnum (gdbarch, REG_CCW);
  set_gdbarch_pc_regnum (gdbarch, REG_P);
  set_gdbarch_sp_regnum (gdbarch, REG_S);


  set_gdbarch_print_registers_info (gdbarch, s12z_print_registers_info);

  set_gdbarch_breakpoint_kind_from_pc (gdbarch,
				       s12z_breakpoint::kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch,
				       s12z_breakpoint::bp_from_kind);

  set_gdbarch_num_regs (gdbarch, N_PHYSICAL_REGISTERS);
  set_gdbarch_register_name (gdbarch, s12z_register_name);
  set_gdbarch_skip_prologue (gdbarch, s12z_skip_prologue);
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);
  set_gdbarch_dwarf2_reg_to_regnum (gdbarch, s12z_dwarf_reg_to_regnum);

  set_gdbarch_register_type (gdbarch, s12z_register_type);

  frame_unwind_append_unwinder (gdbarch, &s12z_frame_unwind);
  /* Currently, the only known producer for this architecture, produces buggy
     dwarf CFI.   So don't append a dwarf unwinder until the situation is
     better understood.  */

  set_gdbarch_return_value (gdbarch, s12z_return_value);

  return gdbarch;
}

void _initialize_s12z_tdep ();
void
_initialize_s12z_tdep ()
{
  gdbarch_register (bfd_arch_s12z, s12z_gdbarch_init, NULL);
}
