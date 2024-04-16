/* Target-dependent code for Atmel AVR, for GDB.

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

/* Contributed by Theodore A. Roth, troth@openavr.org */

/* Portions of this file were taken from the original gdb-4.18 patch developed
   by Denis Chertykov, denisc@overta.ru */

#include "defs.h"
#include "frame.h"
#include "frame-unwind.h"
#include "frame-base.h"
#include "trad-frame.h"
#include "gdbcmd.h"
#include "gdbcore.h"
#include "gdbtypes.h"
#include "inferior.h"
#include "symfile.h"
#include "arch-utils.h"
#include "regcache.h"
#include "dis-asm.h"
#include "objfiles.h"
#include <algorithm>
#include "gdbarch.h"

/* AVR Background:

   (AVR micros are pure Harvard Architecture processors.)

   The AVR family of microcontrollers have three distinctly different memory
   spaces: flash, sram and eeprom.  The flash is 16 bits wide and is used for
   the most part to store program instructions.  The sram is 8 bits wide and is
   used for the stack and the heap.  Some devices lack sram and some can have
   an additional external sram added on as a peripheral.

   The eeprom is 8 bits wide and is used to store data when the device is
   powered down.  Eeprom is not directly accessible, it can only be accessed
   via io-registers using a special algorithm.  Accessing eeprom via gdb's
   remote serial protocol ('m' or 'M' packets) looks difficult to do and is
   not included at this time.

   [The eeprom could be read manually via ``x/b <eaddr + AVR_EMEM_START>'' or
   written using ``set {unsigned char}<eaddr + AVR_EMEM_START>''.  For this to
   work, the remote target must be able to handle eeprom accesses and perform
   the address translation.]

   All three memory spaces have physical addresses beginning at 0x0.  In
   addition, the flash is addressed by gcc/binutils/gdb with respect to 8 bit
   bytes instead of the 16 bit wide words used by the real device for the
   Program Counter.

   In order for remote targets to work correctly, extra bits must be added to
   addresses before they are send to the target or received from the target
   via the remote serial protocol.  The extra bits are the MSBs and are used to
   decode which memory space the address is referring to.  */

/* Constants: prefixed with AVR_ to avoid name space clashes */

/* Address space flags */

/* We are assigning the TYPE_INSTANCE_FLAG_ADDRESS_CLASS_1 to the flash address
   space.  */

#define AVR_TYPE_ADDRESS_CLASS_FLASH TYPE_ADDRESS_CLASS_1
#define AVR_TYPE_INSTANCE_FLAG_ADDRESS_CLASS_FLASH  \
  TYPE_INSTANCE_FLAG_ADDRESS_CLASS_1


enum
{
  AVR_REG_W = 24,
  AVR_REG_X = 26,
  AVR_REG_Y = 28,
  AVR_FP_REGNUM = 28,
  AVR_REG_Z = 30,

  AVR_SREG_REGNUM = 32,
  AVR_SP_REGNUM = 33,
  AVR_PC_REGNUM = 34,

  AVR_NUM_REGS = 32 + 1 /*SREG*/ + 1 /*SP*/ + 1 /*PC*/,
  AVR_NUM_REG_BYTES = 32 + 1 /*SREG*/ + 2 /*SP*/ + 4 /*PC*/,

  /* Pseudo registers.  */
  AVR_PSEUDO_PC_REGNUM = 35,
  AVR_NUM_PSEUDO_REGS = 1,

  AVR_PC_REG_INDEX = 35,	/* index into array of registers */

  AVR_MAX_PROLOGUE_SIZE = 64,	/* bytes */

  /* Count of pushed registers.  From r2 to r17 (inclusively), r28, r29 */
  AVR_MAX_PUSHES = 18,

  /* Number of the last pushed register.  r17 for current avr-gcc */
  AVR_LAST_PUSHED_REGNUM = 17,

  AVR_ARG1_REGNUM = 24,         /* Single byte argument */
  AVR_ARGN_REGNUM = 25,         /* Multi byte arguments */
  AVR_LAST_ARG_REGNUM = 8,      /* Last argument register */

  AVR_RET1_REGNUM = 24,         /* Single byte return value */
  AVR_RETN_REGNUM = 25,         /* Multi byte return value */

  /* FIXME: TRoth/2002-01-??: Can we shift all these memory masks left 8
     bits?  Do these have to match the bfd vma values?  It sure would make
     things easier in the future if they didn't need to match.

     Note: I chose these values so as to be consistent with bfd vma
     addresses.

     TRoth/2002-04-08: There is already a conflict with very large programs
     in the mega128.  The mega128 has 128K instruction bytes (64K words),
     thus the Most Significant Bit is 0x10000 which gets masked off my
     AVR_MEM_MASK.

     The problem manifests itself when trying to set a breakpoint in a
     function which resides in the upper half of the instruction space and
     thus requires a 17-bit address.

     For now, I've just removed the EEPROM mask and changed AVR_MEM_MASK
     from 0x00ff0000 to 0x00f00000.  Eeprom is not accessible from gdb yet,
     but could be for some remote targets by just adding the correct offset
     to the address and letting the remote target handle the low-level
     details of actually accessing the eeprom.  */

  AVR_IMEM_START = 0x00000000,	/* INSN memory */
  AVR_SMEM_START = 0x00800000,	/* SRAM memory */
#if 1
  /* No eeprom mask defined */
  AVR_MEM_MASK = 0x00f00000,	/* mask to determine memory space */
#else
  AVR_EMEM_START = 0x00810000,	/* EEPROM memory */
  AVR_MEM_MASK = 0x00ff0000,	/* mask to determine memory space */
#endif
};

/* Prologue types:

   NORMAL and CALL are the typical types (the -mcall-prologues gcc option
   causes the generation of the CALL type prologues).  */

enum {
    AVR_PROLOGUE_NONE,              /* No prologue */
    AVR_PROLOGUE_NORMAL,
    AVR_PROLOGUE_CALL,              /* -mcall-prologues */
    AVR_PROLOGUE_MAIN,
    AVR_PROLOGUE_INTR,              /* interrupt handler */
    AVR_PROLOGUE_SIG,               /* signal handler */
};

/* Any function with a frame looks like this
   .......    <-SP POINTS HERE
   LOCALS1    <-FP POINTS HERE
   LOCALS0
   SAVED FP
   SAVED R3
   SAVED R2
   RET PC
   FIRST ARG
   SECOND ARG */

struct avr_unwind_cache
{
  /* The previous frame's inner most stack address.  Used as this
     frame ID's stack_addr.  */
  CORE_ADDR prev_sp;
  /* The frame's base, optionally used by the high-level debug info.  */
  CORE_ADDR base;
  int size;
  int prologue_type;
  /* Table indicating the location of each and every register.  */
  trad_frame_saved_reg *saved_regs;
};

struct avr_gdbarch_tdep : gdbarch_tdep_base
{
  /* Number of bytes stored to the stack by call instructions.
     2 bytes for avr1-5 and avrxmega1-5, 3 bytes for avr6 and avrxmega6-7.  */
  int call_length = 0;

  /* Type for void.  */
  struct type *void_type = nullptr;
  /* Type for a function returning void.  */
  struct type *func_void_type = nullptr;
  /* Type for a pointer to a function.  Used for the type of PC.  */
  struct type *pc_type = nullptr;
};

/* Lookup the name of a register given it's number.  */

static const char *
avr_register_name (struct gdbarch *gdbarch, int regnum)
{
  static const char * const register_names[] = {
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
    "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
    "r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31",
    "SREG", "SP", "PC2",
    "pc"
  };
  static_assert (ARRAY_SIZE (register_names)
		     == (AVR_NUM_REGS + AVR_NUM_PSEUDO_REGS));
  return register_names[regnum];
}

/* Return the GDB type object for the "standard" data type
   of data in register N.  */

static struct type *
avr_register_type (struct gdbarch *gdbarch, int reg_nr)
{
  if (reg_nr == AVR_PC_REGNUM)
    return builtin_type (gdbarch)->builtin_uint32;

  avr_gdbarch_tdep *tdep = gdbarch_tdep<avr_gdbarch_tdep> (gdbarch);
  if (reg_nr == AVR_PSEUDO_PC_REGNUM)
    return tdep->pc_type;

  if (reg_nr == AVR_SP_REGNUM)
    return builtin_type (gdbarch)->builtin_data_ptr;

  return builtin_type (gdbarch)->builtin_uint8;
}

/* Instruction address checks and conversions.  */

static CORE_ADDR
avr_make_iaddr (CORE_ADDR x)
{
  return ((x) | AVR_IMEM_START);
}

/* FIXME: TRoth: Really need to use a larger mask for instructions.  Some
   devices are already up to 128KBytes of flash space.

   TRoth/2002-04-8: See comment above where AVR_IMEM_START is defined.  */

static CORE_ADDR
avr_convert_iaddr_to_raw (CORE_ADDR x)
{
  return ((x) & 0xffffffff);
}

/* SRAM address checks and conversions.  */

static CORE_ADDR
avr_make_saddr (CORE_ADDR x)
{
  /* Return 0 for NULL.  */
  if (x == 0)
    return 0;

  return ((x) | AVR_SMEM_START);
}

static CORE_ADDR
avr_convert_saddr_to_raw (CORE_ADDR x)
{
  return ((x) & 0xffffffff);
}

/* EEPROM address checks and conversions.  I don't know if these will ever
   actually be used, but I've added them just the same.  TRoth */

/* TRoth/2002-04-08: Commented out for now to allow fix for problem with large
   programs in the mega128.  */

/*  static CORE_ADDR */
/*  avr_make_eaddr (CORE_ADDR x) */
/*  { */
/*    return ((x) | AVR_EMEM_START); */
/*  } */

/*  static int */
/*  avr_eaddr_p (CORE_ADDR x) */
/*  { */
/*    return (((x) & AVR_MEM_MASK) == AVR_EMEM_START); */
/*  } */

/*  static CORE_ADDR */
/*  avr_convert_eaddr_to_raw (CORE_ADDR x) */
/*  { */
/*    return ((x) & 0xffffffff); */
/*  } */

/* Convert from address to pointer and vice-versa.  */

static void
avr_address_to_pointer (struct gdbarch *gdbarch,
			struct type *type, gdb_byte *buf, CORE_ADDR addr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  /* Is it a data address in flash?  */
  if (AVR_TYPE_ADDRESS_CLASS_FLASH (type))
    {
      /* A data pointer in flash is byte addressed.  */
      store_unsigned_integer (buf, type->length (), byte_order,
			      avr_convert_iaddr_to_raw (addr));
    }
  /* Is it a code address?  */
  else if (type->target_type ()->code () == TYPE_CODE_FUNC
	   || type->target_type ()->code () == TYPE_CODE_METHOD)
    {
      /* A code pointer is word (16 bits) addressed.  We shift the address down
	 by 1 bit to convert it to a pointer.  */
      store_unsigned_integer (buf, type->length (), byte_order,
			      avr_convert_iaddr_to_raw (addr >> 1));
    }
  else
    {
      /* Strip off any upper segment bits.  */
      store_unsigned_integer (buf, type->length (), byte_order,
			      avr_convert_saddr_to_raw (addr));
    }
}

static CORE_ADDR
avr_pointer_to_address (struct gdbarch *gdbarch,
			struct type *type, const gdb_byte *buf)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR addr
    = extract_unsigned_integer (buf, type->length (), byte_order);

  /* Is it a data address in flash?  */
  if (AVR_TYPE_ADDRESS_CLASS_FLASH (type))
    {
      /* A data pointer in flash is already byte addressed.  */
      return avr_make_iaddr (addr);
    }
  /* Is it a code address?  */
  else if (type->target_type ()->code () == TYPE_CODE_FUNC
	   || type->target_type ()->code () == TYPE_CODE_METHOD
	   || TYPE_CODE_SPACE (type->target_type ()))
    {
      /* A code pointer is word (16 bits) addressed so we shift it up
	 by 1 bit to convert it to an address.  */
      return avr_make_iaddr (addr << 1);
    }
  else
    return avr_make_saddr (addr);
}

static CORE_ADDR
avr_integer_to_address (struct gdbarch *gdbarch,
			struct type *type, const gdb_byte *buf)
{
  ULONGEST addr = unpack_long (type, buf);

  if (TYPE_DATA_SPACE (type))
    return avr_make_saddr (addr);
  else
    return avr_make_iaddr (addr);
}

static CORE_ADDR
avr_read_pc (readable_regcache *regcache)
{
  ULONGEST pc;

  regcache->cooked_read (AVR_PC_REGNUM, &pc);
  return avr_make_iaddr (pc);
}

static void
avr_write_pc (struct regcache *regcache, CORE_ADDR val)
{
  regcache_cooked_write_unsigned (regcache, AVR_PC_REGNUM,
				  avr_convert_iaddr_to_raw (val));
}

static enum register_status
avr_pseudo_register_read (struct gdbarch *gdbarch, readable_regcache *regcache,
			  int regnum, gdb_byte *buf)
{
  ULONGEST val;
  enum register_status status;

  switch (regnum)
    {
    case AVR_PSEUDO_PC_REGNUM:
      status = regcache->raw_read (AVR_PC_REGNUM, &val);
      if (status != REG_VALID)
	return status;
      val >>= 1;
      store_unsigned_integer (buf, 4, gdbarch_byte_order (gdbarch), val);
      return status;
    default:
      internal_error (_("invalid regnum"));
    }
}

static void
avr_pseudo_register_write (struct gdbarch *gdbarch, struct regcache *regcache,
			   int regnum, const gdb_byte *buf)
{
  ULONGEST val;

  switch (regnum)
    {
    case AVR_PSEUDO_PC_REGNUM:
      val = extract_unsigned_integer (buf, 4, gdbarch_byte_order (gdbarch));
      val <<= 1;
      regcache_raw_write_unsigned (regcache, AVR_PC_REGNUM, val);
      break;
    default:
      internal_error (_("invalid regnum"));
    }
}

/* Function: avr_scan_prologue

   This function decodes an AVR function prologue to determine:
     1) the size of the stack frame
     2) which registers are saved on it
     3) the offsets of saved regs
   This information is stored in the avr_unwind_cache structure.

   Some devices lack the sbiw instruction, so on those replace this:
	sbiw    r28, XX
   with this:
	subi    r28,lo8(XX)
	sbci    r29,hi8(XX)

   A typical AVR function prologue with a frame pointer might look like this:
	push    rXX        ; saved regs
	...
	push    r28
	push    r29
	in      r28,__SP_L__
	in      r29,__SP_H__
	sbiw    r28,<LOCALS_SIZE>
	in      __tmp_reg__,__SREG__
	cli
	out     __SP_H__,r29
	out     __SREG__,__tmp_reg__
	out     __SP_L__,r28

   A typical AVR function prologue without a frame pointer might look like
   this:
	push    rXX        ; saved regs
	...

   A main function prologue looks like this:
	ldi     r28,lo8(<RAM_ADDR> - <LOCALS_SIZE>)
	ldi     r29,hi8(<RAM_ADDR> - <LOCALS_SIZE>)
	out     __SP_H__,r29
	out     __SP_L__,r28

   A signal handler prologue looks like this:
	push    __zero_reg__
	push    __tmp_reg__
	in      __tmp_reg__, __SREG__
	push    __tmp_reg__
	clr     __zero_reg__
	push    rXX             ; save registers r18:r27, r30:r31
	...
	push    r28             ; save frame pointer
	push    r29
	in      r28, __SP_L__
	in      r29, __SP_H__
	sbiw    r28, <LOCALS_SIZE>
	out     __SP_H__, r29
	out     __SP_L__, r28
	
   A interrupt handler prologue looks like this:
	sei
	push    __zero_reg__
	push    __tmp_reg__
	in      __tmp_reg__, __SREG__
	push    __tmp_reg__
	clr     __zero_reg__
	push    rXX             ; save registers r18:r27, r30:r31
	...
	push    r28             ; save frame pointer
	push    r29
	in      r28, __SP_L__
	in      r29, __SP_H__
	sbiw    r28, <LOCALS_SIZE>
	cli
	out     __SP_H__, r29
	sei     
	out     __SP_L__, r28

   A `-mcall-prologues' prologue looks like this (Note that the megas use a
   jmp instead of a rjmp, thus the prologue is one word larger since jmp is a
   32 bit insn and rjmp is a 16 bit insn):
	ldi     r26,lo8(<LOCALS_SIZE>)
	ldi     r27,hi8(<LOCALS_SIZE>)
	ldi     r30,pm_lo8(.L_foo_body)
	ldi     r31,pm_hi8(.L_foo_body)
	rjmp    __prologue_saves__+RRR
	.L_foo_body:  */

/* Not really part of a prologue, but still need to scan for it, is when a
   function prologue moves values passed via registers as arguments to new
   registers.  In this case, all local variables live in registers, so there
   may be some register saves.  This is what it looks like:
	movw    rMM, rNN
	...

   There could be multiple movw's.  If the target doesn't have a movw insn, it
   will use two mov insns.  This could be done after any of the above prologue
   types.  */

static CORE_ADDR
avr_scan_prologue (struct gdbarch *gdbarch, CORE_ADDR pc_beg, CORE_ADDR pc_end,
		   struct avr_unwind_cache *info)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int i;
  unsigned short insn;
  int scan_stage = 0;
  struct bound_minimal_symbol msymbol;
  unsigned char prologue[AVR_MAX_PROLOGUE_SIZE];
  int vpc = 0;
  int len;

  len = pc_end - pc_beg;
  if (len > AVR_MAX_PROLOGUE_SIZE)
    len = AVR_MAX_PROLOGUE_SIZE;

  /* FIXME: TRoth/2003-06-11: This could be made more efficient by only
     reading in the bytes of the prologue.  The problem is that the figuring
     out where the end of the prologue is is a bit difficult.  The old code 
     tried to do that, but failed quite often.  */
  read_memory (pc_beg, prologue, len);

  /* Scanning main()'s prologue
     ldi r28,lo8(<RAM_ADDR> - <LOCALS_SIZE>)
     ldi r29,hi8(<RAM_ADDR> - <LOCALS_SIZE>)
     out __SP_H__,r29
     out __SP_L__,r28 */

  if (len >= 4)
    {
      CORE_ADDR locals;
      static const unsigned char img[] = {
	0xde, 0xbf,		/* out __SP_H__,r29 */
	0xcd, 0xbf		/* out __SP_L__,r28 */
      };

      insn = extract_unsigned_integer (&prologue[vpc], 2, byte_order);
      /* ldi r28,lo8(<RAM_ADDR> - <LOCALS_SIZE>) */
      if ((insn & 0xf0f0) == 0xe0c0)
	{
	  locals = (insn & 0xf) | ((insn & 0x0f00) >> 4);
	  insn = extract_unsigned_integer (&prologue[vpc + 2], 2, byte_order);
	  /* ldi r29,hi8(<RAM_ADDR> - <LOCALS_SIZE>) */
	  if ((insn & 0xf0f0) == 0xe0d0)
	    {
	      locals |= ((insn & 0xf) | ((insn & 0x0f00) >> 4)) << 8;
	      if (vpc + 4 + sizeof (img) < len
		  && memcmp (prologue + vpc + 4, img, sizeof (img)) == 0)
		{
		  info->prologue_type = AVR_PROLOGUE_MAIN;
		  info->base = locals;
		  return pc_beg + 4;
		}
	    }
	}
    }

  /* Scanning `-mcall-prologues' prologue
     Classic prologue is 10 bytes, mega prologue is a 12 bytes long */

  while (1)	/* Using a while to avoid many goto's */
    {
      int loc_size;
      int body_addr;
      unsigned num_pushes;
      int pc_offset = 0;

      /* At least the fifth instruction must have been executed to
	 modify frame shape.  */
      if (len < 10)
	break;

      insn = extract_unsigned_integer (&prologue[vpc], 2, byte_order);
      /* ldi r26,<LOCALS_SIZE> */
      if ((insn & 0xf0f0) != 0xe0a0)
	break;
      loc_size = (insn & 0xf) | ((insn & 0x0f00) >> 4);
      pc_offset += 2;

      insn = extract_unsigned_integer (&prologue[vpc + 2], 2, byte_order);
      /* ldi r27,<LOCALS_SIZE> / 256 */
      if ((insn & 0xf0f0) != 0xe0b0)
	break;
      loc_size |= ((insn & 0xf) | ((insn & 0x0f00) >> 4)) << 8;
      pc_offset += 2;

      insn = extract_unsigned_integer (&prologue[vpc + 4], 2, byte_order);
      /* ldi r30,pm_lo8(.L_foo_body) */
      if ((insn & 0xf0f0) != 0xe0e0)
	break;
      body_addr = (insn & 0xf) | ((insn & 0x0f00) >> 4);
      pc_offset += 2;

      insn = extract_unsigned_integer (&prologue[vpc + 6], 2, byte_order);
      /* ldi r31,pm_hi8(.L_foo_body) */
      if ((insn & 0xf0f0) != 0xe0f0)
	break;
      body_addr |= ((insn & 0xf) | ((insn & 0x0f00) >> 4)) << 8;
      pc_offset += 2;

      msymbol = lookup_minimal_symbol ("__prologue_saves__", NULL, NULL);
      if (!msymbol.minsym)
	break;

      insn = extract_unsigned_integer (&prologue[vpc + 8], 2, byte_order);
      /* rjmp __prologue_saves__+RRR */
      if ((insn & 0xf000) == 0xc000)
	{
	  /* Extract PC relative offset from RJMP */
	  i = (insn & 0xfff) | (insn & 0x800 ? (-1 ^ 0xfff) : 0);
	  /* Convert offset to byte addressable mode */
	  i *= 2;
	  /* Destination address */
	  i += pc_beg + 10;

	  if (body_addr != (pc_beg + 10)/2)
	    break;

	  pc_offset += 2;
	}
      else if ((insn & 0xfe0e) == 0x940c)
	{
	  /* Extract absolute PC address from JMP */
	  i = (((insn & 0x1) | ((insn & 0x1f0) >> 3) << 16)
	       | (extract_unsigned_integer (&prologue[vpc + 10], 2, byte_order)
		  & 0xffff));
	  /* Convert address to byte addressable mode */
	  i *= 2;

	  if (body_addr != (pc_beg + 12)/2)
	    break;

	  pc_offset += 4;
	}
      else
	break;

      /* Resolve offset (in words) from __prologue_saves__ symbol.
	 Which is a pushes count in `-mcall-prologues' mode */
      num_pushes = AVR_MAX_PUSHES - (i - msymbol.value_address ()) / 2;

      if (num_pushes > AVR_MAX_PUSHES)
	{
	  gdb_printf (gdb_stderr, _("Num pushes too large: %d\n"),
		      num_pushes);
	  num_pushes = 0;
	}

      if (num_pushes)
	{
	  int from;

	  info->saved_regs[AVR_FP_REGNUM + 1].set_addr (num_pushes);
	  if (num_pushes >= 2)
	    info->saved_regs[AVR_FP_REGNUM].set_addr (num_pushes - 1);

	  i = 0;
	  for (from = AVR_LAST_PUSHED_REGNUM + 1 - (num_pushes - 2);
	       from <= AVR_LAST_PUSHED_REGNUM; ++from)
	    info->saved_regs [from].set_addr (++i);
	}
      info->size = loc_size + num_pushes;
      info->prologue_type = AVR_PROLOGUE_CALL;

      return pc_beg + pc_offset;
    }

  /* Scan for the beginning of the prologue for an interrupt or signal
     function.  Note that we have to set the prologue type here since the
     third stage of the prologue may not be present (e.g. no saved registered
     or changing of the SP register).  */

  if (1)
    {
      static const unsigned char img[] = {
	0x78, 0x94,		/* sei */
	0x1f, 0x92,		/* push r1 */
	0x0f, 0x92,		/* push r0 */
	0x0f, 0xb6,		/* in r0,0x3f SREG */
	0x0f, 0x92,		/* push r0 */
	0x11, 0x24		/* clr r1 */
      };
      if (len >= sizeof (img)
	  && memcmp (prologue, img, sizeof (img)) == 0)
	{
	  info->prologue_type = AVR_PROLOGUE_INTR;
	  vpc += sizeof (img);
	  info->saved_regs[AVR_SREG_REGNUM].set_addr (3);
	  info->saved_regs[0].set_addr (2);
	  info->saved_regs[1].set_addr (1);
	  info->size += 3;
	}
      else if (len >= sizeof (img) - 2
	       && memcmp (img + 2, prologue, sizeof (img) - 2) == 0)
	{
	  info->prologue_type = AVR_PROLOGUE_SIG;
	  vpc += sizeof (img) - 2;
	  info->saved_regs[AVR_SREG_REGNUM].set_addr (3);
	  info->saved_regs[0].set_addr (2);
	  info->saved_regs[1].set_addr (1);
	  info->size += 2;
	}
    }

  /* First stage of the prologue scanning.
     Scan pushes (saved registers) */

  for (; vpc < len; vpc += 2)
    {
      insn = extract_unsigned_integer (&prologue[vpc], 2, byte_order);
      if ((insn & 0xfe0f) == 0x920f)	/* push rXX */
	{
	  /* Bits 4-9 contain a mask for registers R0-R32.  */
	  int regno = (insn & 0x1f0) >> 4;
	  info->size++;
	  info->saved_regs[regno].set_addr (info->size);
	  scan_stage = 1;
	}
      else
	break;
    }

  gdb_assert (vpc < AVR_MAX_PROLOGUE_SIZE);

  /* Handle static small stack allocation using rcall or push.  */
  avr_gdbarch_tdep *tdep = gdbarch_tdep<avr_gdbarch_tdep> (gdbarch);
  while (scan_stage == 1 && vpc < len)
    {
      insn = extract_unsigned_integer (&prologue[vpc], 2, byte_order);
      if (insn == 0xd000)	/* rcall .+0 */
	{
	  info->size += tdep->call_length;
	  vpc += 2;
	}
      else if (insn == 0x920f || insn == 0x921f)  /* push r0 or push r1 */
	{
	  info->size += 1;
	  vpc += 2;
	}
      else
	break;
    }

  /* Second stage of the prologue scanning.
     Scan:
     in r28,__SP_L__
     in r29,__SP_H__ */

  if (scan_stage == 1 && vpc < len)
    {
      static const unsigned char img[] = {
	0xcd, 0xb7,		/* in r28,__SP_L__ */
	0xde, 0xb7		/* in r29,__SP_H__ */
      };

      if (vpc + sizeof (img) < len
	  && memcmp (prologue + vpc, img, sizeof (img)) == 0)
	{
	  vpc += 4;
	  scan_stage = 2;
	}
    }

  /* Third stage of the prologue scanning.  (Really two stages).
     Scan for:
     sbiw r28,XX or subi r28,lo8(XX)
		    sbci r29,hi8(XX)
     in __tmp_reg__,__SREG__
     cli
     out __SP_H__,r29
     out __SREG__,__tmp_reg__
     out __SP_L__,r28 */

  if (scan_stage == 2 && vpc < len)
    {
      int locals_size = 0;
      static const unsigned char img[] = {
	0x0f, 0xb6,		/* in r0,0x3f */
	0xf8, 0x94,		/* cli */
	0xde, 0xbf,		/* out 0x3e,r29 ; SPH */
	0x0f, 0xbe,		/* out 0x3f,r0  ; SREG */
	0xcd, 0xbf		/* out 0x3d,r28 ; SPL */
      };
      static const unsigned char img_sig[] = {
	0xde, 0xbf,		/* out 0x3e,r29 ; SPH */
	0xcd, 0xbf		/* out 0x3d,r28 ; SPL */
      };
      static const unsigned char img_int[] = {
	0xf8, 0x94,		/* cli */
	0xde, 0xbf,		/* out 0x3e,r29 ; SPH */
	0x78, 0x94,		/* sei */
	0xcd, 0xbf		/* out 0x3d,r28 ; SPL */
      };

      insn = extract_unsigned_integer (&prologue[vpc], 2, byte_order);
      if ((insn & 0xff30) == 0x9720)	/* sbiw r28,XXX */
	{
	  locals_size = (insn & 0xf) | ((insn & 0xc0) >> 2);
	  vpc += 2;
	}
      else if ((insn & 0xf0f0) == 0x50c0)	/* subi r28,lo8(XX) */
	{
	  locals_size = (insn & 0xf) | ((insn & 0xf00) >> 4);
	  vpc += 2;
	  insn = extract_unsigned_integer (&prologue[vpc], 2, byte_order);
	  vpc += 2;
	  locals_size += ((insn & 0xf) | ((insn & 0xf00) >> 4)) << 8;
	}
      else
	return pc_beg + vpc;

      /* Scan the last part of the prologue.  May not be present for interrupt
	 or signal handler functions, which is why we set the prologue type
	 when we saw the beginning of the prologue previously.  */

      if (vpc + sizeof (img_sig) < len
	  && memcmp (prologue + vpc, img_sig, sizeof (img_sig)) == 0)
	{
	  vpc += sizeof (img_sig);
	}
      else if (vpc + sizeof (img_int) < len 
	       && memcmp (prologue + vpc, img_int, sizeof (img_int)) == 0)
	{
	  vpc += sizeof (img_int);
	}
      if (vpc + sizeof (img) < len
	  && memcmp (prologue + vpc, img, sizeof (img)) == 0)
	{
	  info->prologue_type = AVR_PROLOGUE_NORMAL;
	  vpc += sizeof (img);
	}

      info->size += locals_size;

      /* Fall through.  */
    }

  /* If we got this far, we could not scan the prologue, so just return the pc
     of the frame plus an adjustment for argument move insns.  */

  for (; vpc < len; vpc += 2)
    {
      insn = extract_unsigned_integer (&prologue[vpc], 2, byte_order);
      if ((insn & 0xff00) == 0x0100)	/* movw rXX, rYY */
	continue;
      else if ((insn & 0xfc00) == 0x2c00) /* mov rXX, rYY */
	continue;
      else
	  break;
    }
    
  return pc_beg + vpc;
}

static CORE_ADDR
avr_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  CORE_ADDR func_addr, func_end;
  CORE_ADDR post_prologue_pc;

  /* See what the symbol table says */

  if (!find_pc_partial_function (pc, NULL, &func_addr, &func_end))
    return pc;

  post_prologue_pc = skip_prologue_using_sal (gdbarch, func_addr);
  if (post_prologue_pc != 0)
    return std::max (pc, post_prologue_pc);

  {
    CORE_ADDR prologue_end = pc;
    struct avr_unwind_cache info = {0};
    trad_frame_saved_reg saved_regs[AVR_NUM_REGS];

    info.saved_regs = saved_regs;
    
    /* Need to run the prologue scanner to figure out if the function has a
       prologue and possibly skip over moving arguments passed via registers
       to other registers.  */
    
    prologue_end = avr_scan_prologue (gdbarch, func_addr, func_end, &info);
    
    if (info.prologue_type != AVR_PROLOGUE_NONE)
      return prologue_end;
  }

  /* Either we didn't find the start of this function (nothing we can do),
     or there's no line info, or the line after the prologue is after
     the end of the function (there probably isn't a prologue).  */

  return pc;
}

/* Not all avr devices support the BREAK insn.  Those that don't should treat
   it as a NOP.  Thus, it should be ok.  Since the avr is currently a remote
   only target, this shouldn't be a problem (I hope).  TRoth/2003-05-14  */

constexpr gdb_byte avr_break_insn [] = { 0x98, 0x95 };

typedef BP_MANIPULATION (avr_break_insn) avr_breakpoint;

/* Determine, for architecture GDBARCH, how a return value of TYPE
   should be returned.  If it is supposed to be returned in registers,
   and READBUF is non-zero, read the appropriate value from REGCACHE,
   and copy it into READBUF.  If WRITEBUF is non-zero, write the value
   from WRITEBUF into REGCACHE.  */

static enum return_value_convention
avr_return_value (struct gdbarch *gdbarch, struct value *function,
		  struct type *valtype, struct regcache *regcache,
		  gdb_byte *readbuf, const gdb_byte *writebuf)
{
  int i;
  /* Single byte are returned in r24.
     Otherwise, the MSB of the return value is always in r25, calculate which
     register holds the LSB.  */
  int lsb_reg;

  if ((valtype->code () == TYPE_CODE_STRUCT
       || valtype->code () == TYPE_CODE_UNION
       || valtype->code () == TYPE_CODE_ARRAY)
      && valtype->length () > 8)
    return RETURN_VALUE_STRUCT_CONVENTION;

  if (valtype->length () <= 2)
    lsb_reg = 24;
  else if (valtype->length () <= 4)
    lsb_reg = 22;
  else if (valtype->length () <= 8)
    lsb_reg = 18;
  else
    gdb_assert_not_reached ("unexpected type length");

  if (writebuf != NULL)
    {
      for (i = 0; i < valtype->length (); i++)
	regcache->cooked_write (lsb_reg + i, writebuf + i);
    }

  if (readbuf != NULL)
    {
      for (i = 0; i < valtype->length (); i++)
	regcache->cooked_read (lsb_reg + i, readbuf + i);
    }

  return RETURN_VALUE_REGISTER_CONVENTION;
}


/* Put here the code to store, into fi->saved_regs, the addresses of
   the saved registers of frame described by FRAME_INFO.  This
   includes special registers such as pc and fp saved in special ways
   in the stack frame.  sp is even more special: the address we return
   for it IS the sp for the next frame.  */

static struct avr_unwind_cache *
avr_frame_unwind_cache (frame_info_ptr this_frame,
			void **this_prologue_cache)
{
  CORE_ADDR start_pc, current_pc;
  ULONGEST prev_sp;
  ULONGEST this_base;
  struct avr_unwind_cache *info;
  struct gdbarch *gdbarch;
  int i;

  if (*this_prologue_cache)
    return (struct avr_unwind_cache *) *this_prologue_cache;

  info = FRAME_OBSTACK_ZALLOC (struct avr_unwind_cache);
  *this_prologue_cache = info;
  info->saved_regs = trad_frame_alloc_saved_regs (this_frame);

  info->size = 0;
  info->prologue_type = AVR_PROLOGUE_NONE;

  start_pc = get_frame_func (this_frame);
  current_pc = get_frame_pc (this_frame);
  if ((start_pc > 0) && (start_pc <= current_pc))
    avr_scan_prologue (get_frame_arch (this_frame),
		       start_pc, current_pc, info);

  if ((info->prologue_type != AVR_PROLOGUE_NONE)
      && (info->prologue_type != AVR_PROLOGUE_MAIN))
    {
      ULONGEST high_base;       /* High byte of FP */

      /* The SP was moved to the FP.  This indicates that a new frame
	 was created.  Get THIS frame's FP value by unwinding it from
	 the next frame.  */
      this_base = get_frame_register_unsigned (this_frame, AVR_FP_REGNUM);
      high_base = get_frame_register_unsigned (this_frame, AVR_FP_REGNUM + 1);
      this_base += (high_base << 8);
      
      /* The FP points at the last saved register.  Adjust the FP back
	 to before the first saved register giving the SP.  */
      prev_sp = this_base + info->size; 
   }
  else
    {
      /* Assume that the FP is this frame's SP but with that pushed
	 stack space added back.  */
      this_base = get_frame_register_unsigned (this_frame, AVR_SP_REGNUM);
      prev_sp = this_base + info->size;
    }

  /* Add 1 here to adjust for the post-decrement nature of the push
     instruction.*/
  info->prev_sp = avr_make_saddr (prev_sp + 1);
  info->base = avr_make_saddr (this_base);

  gdbarch = get_frame_arch (this_frame);

  /* Adjust all the saved registers so that they contain addresses and not
     offsets.  */
  for (i = 0; i < gdbarch_num_regs (gdbarch) - 1; i++)
    if (info->saved_regs[i].is_addr ())
      info->saved_regs[i].set_addr (info->prev_sp
				    - info->saved_regs[i].addr ());

  /* Except for the main and startup code, the return PC is always saved on
     the stack and is at the base of the frame.  */

  if (info->prologue_type != AVR_PROLOGUE_MAIN)
    info->saved_regs[AVR_PC_REGNUM].set_addr (info->prev_sp);

  /* The previous frame's SP needed to be computed.  Save the computed
     value.  */
  avr_gdbarch_tdep *tdep = gdbarch_tdep<avr_gdbarch_tdep> (gdbarch);
  info->saved_regs[AVR_SP_REGNUM].set_value (info->prev_sp
					     - 1 + tdep->call_length);

  return info;
}

static CORE_ADDR
avr_unwind_pc (struct gdbarch *gdbarch, frame_info_ptr next_frame)
{
  ULONGEST pc;

  pc = frame_unwind_register_unsigned (next_frame, AVR_PC_REGNUM);

  return avr_make_iaddr (pc);
}

static CORE_ADDR
avr_unwind_sp (struct gdbarch *gdbarch, frame_info_ptr next_frame)
{
  ULONGEST sp;

  sp = frame_unwind_register_unsigned (next_frame, AVR_SP_REGNUM);

  return avr_make_saddr (sp);
}

/* Given a GDB frame, determine the address of the calling function's
   frame.  This will be used to create a new GDB frame struct.  */

static void
avr_frame_this_id (frame_info_ptr this_frame,
		   void **this_prologue_cache,
		   struct frame_id *this_id)
{
  struct avr_unwind_cache *info
    = avr_frame_unwind_cache (this_frame, this_prologue_cache);
  CORE_ADDR base;
  CORE_ADDR func;
  struct frame_id id;

  /* The FUNC is easy.  */
  func = get_frame_func (this_frame);

  /* Hopefully the prologue analysis either correctly determined the
     frame's base (which is the SP from the previous frame), or set
     that base to "NULL".  */
  base = info->prev_sp;
  if (base == 0)
    return;

  id = frame_id_build (base, func);
  (*this_id) = id;
}

static struct value *
avr_frame_prev_register (frame_info_ptr this_frame,
			 void **this_prologue_cache, int regnum)
{
  struct avr_unwind_cache *info
    = avr_frame_unwind_cache (this_frame, this_prologue_cache);

  if (regnum == AVR_PC_REGNUM || regnum == AVR_PSEUDO_PC_REGNUM)
    {
      if (info->saved_regs[AVR_PC_REGNUM].is_addr ())
	{
	  /* Reading the return PC from the PC register is slightly
	     abnormal.  register_size(AVR_PC_REGNUM) says it is 4 bytes,
	     but in reality, only two bytes (3 in upcoming mega256) are
	     stored on the stack.

	     Also, note that the value on the stack is an addr to a word
	     not a byte, so we will need to multiply it by two at some
	     point. 

	     And to confuse matters even more, the return address stored
	     on the stack is in big endian byte order, even though most
	     everything else about the avr is little endian.  Ick!  */
	  ULONGEST pc;
	  int i;
	  gdb_byte buf[3];
	  struct gdbarch *gdbarch = get_frame_arch (this_frame);
	  avr_gdbarch_tdep *tdep = gdbarch_tdep<avr_gdbarch_tdep> (gdbarch);

	  read_memory (info->saved_regs[AVR_PC_REGNUM].addr (),
		       buf, tdep->call_length);

	  /* Extract the PC read from memory as a big-endian.  */
	  pc = 0;
	  for (i = 0; i < tdep->call_length; i++)
	    pc = (pc << 8) | buf[i];

	  if (regnum == AVR_PC_REGNUM)
	    pc <<= 1;

	  return frame_unwind_got_constant (this_frame, regnum, pc);
	}

      return frame_unwind_got_optimized (this_frame, regnum);
    }

  return trad_frame_get_prev_register (this_frame, info->saved_regs, regnum);
}

static const struct frame_unwind avr_frame_unwind = {
  "avr prologue",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  avr_frame_this_id,
  avr_frame_prev_register,
  NULL,
  default_frame_sniffer
};

static CORE_ADDR
avr_frame_base_address (frame_info_ptr this_frame, void **this_cache)
{
  struct avr_unwind_cache *info
    = avr_frame_unwind_cache (this_frame, this_cache);

  return info->base;
}

static const struct frame_base avr_frame_base = {
  &avr_frame_unwind,
  avr_frame_base_address,
  avr_frame_base_address,
  avr_frame_base_address
};

/* Assuming THIS_FRAME is a dummy, return the frame ID of that dummy
   frame.  The frame ID's base needs to match the TOS value saved by
   save_dummy_frame_tos(), and the PC match the dummy frame's breakpoint.  */

static struct frame_id
avr_dummy_id (struct gdbarch *gdbarch, frame_info_ptr this_frame)
{
  ULONGEST base;

  base = get_frame_register_unsigned (this_frame, AVR_SP_REGNUM);
  return frame_id_build (avr_make_saddr (base), get_frame_pc (this_frame));
}

/* When arguments must be pushed onto the stack, they go on in reverse
   order.  The below implements a FILO (stack) to do this.  */

struct avr_stack_item
{
  int len;
  struct avr_stack_item *prev;
  gdb_byte *data;
};

static struct avr_stack_item *
push_stack_item (struct avr_stack_item *prev, const bfd_byte *contents,
		 int len)
{
  struct avr_stack_item *si;
  si = XNEW (struct avr_stack_item);
  si->data = (gdb_byte *) xmalloc (len);
  si->len = len;
  si->prev = prev;
  memcpy (si->data, contents, len);
  return si;
}

static struct avr_stack_item *
pop_stack_item (struct avr_stack_item *si)
{
  struct avr_stack_item *dead = si;
  si = si->prev;
  xfree (dead->data);
  xfree (dead);
  return si;
}

/* Setup the function arguments for calling a function in the inferior.

   On the AVR architecture, there are 18 registers (R25 to R8) which are
   dedicated for passing function arguments.  Up to the first 18 arguments
   (depending on size) may go into these registers.  The rest go on the stack.

   All arguments are aligned to start in even-numbered registers (odd-sized
   arguments, including char, have one free register above them).  For example,
   an int in arg1 and a char in arg2 would be passed as such:

      arg1 -> r25:r24
      arg2 -> r22

   Arguments that are larger than 2 bytes will be split between two or more
   registers as available, but will NOT be split between a register and the
   stack.  Arguments that go onto the stack are pushed last arg first (this is
   similar to the d10v).  */

/* NOTE: TRoth/2003-06-17: The rest of this comment is old looks to be
   inaccurate.

   An exceptional case exists for struct arguments (and possibly other
   aggregates such as arrays) -- if the size is larger than WORDSIZE bytes but
   not a multiple of WORDSIZE bytes.  In this case the argument is never split
   between the registers and the stack, but instead is copied in its entirety
   onto the stack, AND also copied into as many registers as there is room
   for.  In other words, space in registers permitting, two copies of the same
   argument are passed in.  As far as I can tell, only the one on the stack is
   used, although that may be a function of the level of compiler
   optimization.  I suspect this is a compiler bug.  Arguments of these odd
   sizes are left-justified within the word (as opposed to arguments smaller
   than WORDSIZE bytes, which are right-justified).
 
   If the function is to return an aggregate type such as a struct, the caller
   must allocate space into which the callee will copy the return value.  In
   this case, a pointer to the return value location is passed into the callee
   in register R0, which displaces one of the other arguments passed in via
   registers R0 to R2.  */

static CORE_ADDR
avr_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
		     struct regcache *regcache, CORE_ADDR bp_addr,
		     int nargs, struct value **args, CORE_ADDR sp,
		     function_call_return_method return_method,
		     CORE_ADDR struct_addr)
{
  int i;
  gdb_byte buf[3];
  avr_gdbarch_tdep *tdep = gdbarch_tdep<avr_gdbarch_tdep> (gdbarch);
  int call_length = tdep->call_length;
  CORE_ADDR return_pc = avr_convert_iaddr_to_raw (bp_addr);
  int regnum = AVR_ARGN_REGNUM;
  struct avr_stack_item *si = NULL;

  if (return_method == return_method_struct)
    {
      regcache_cooked_write_unsigned
	(regcache, regnum--, (struct_addr >> 8) & 0xff);
      regcache_cooked_write_unsigned
	(regcache, regnum--, struct_addr & 0xff);
      /* SP being post decremented, we need to reserve one byte so that the
	 return address won't overwrite the result (or vice-versa).  */
      if (sp == struct_addr)
	sp--;
    }

  for (i = 0; i < nargs; i++)
    {
      int last_regnum;
      int j;
      struct value *arg = args[i];
      struct type *type = check_typedef (arg->type ());
      const bfd_byte *contents = arg->contents ().data ();
      int len = type->length ();

      /* Calculate the potential last register needed.
	 E.g. For length 2, registers regnum and regnum-1 (say 25 and 24)
	 shall be used. So, last needed register will be regnum-1(24).  */
      last_regnum = regnum - (len + (len & 1)) + 1;

      /* If there are registers available, use them.  Once we start putting
	 stuff on the stack, all subsequent args go on stack.  */
      if ((si == NULL) && (last_regnum >= AVR_LAST_ARG_REGNUM))
	{
	  /* Skip a register for odd length args.  */
	  if (len & 1)
	    regnum--;

	  /* Write MSB of argument into register and subsequent bytes in
	     decreasing register numbers.  */
	  for (j = 0; j < len; j++)
	    regcache_cooked_write_unsigned
	      (regcache, regnum--, contents[len - j - 1]);
	}
      /* No registers available, push the args onto the stack.  */
      else
	{
	  /* From here on, we don't care about regnum.  */
	  si = push_stack_item (si, contents, len);
	}
    }

  /* Push args onto the stack.  */
  while (si)
    {
      sp -= si->len;
      /* Add 1 to sp here to account for post decr nature of pushes.  */
      write_memory (sp + 1, si->data, si->len);
      si = pop_stack_item (si);
    }

  /* Set the return address.  For the avr, the return address is the BP_ADDR.
     Need to push the return address onto the stack noting that it needs to be
     in big-endian order on the stack.  */
  for (i = 1; i <= call_length; i++)
    {
      buf[call_length - i] = return_pc & 0xff;
      return_pc >>= 8;
    }

  sp -= call_length;
  /* Use 'sp + 1' since pushes are post decr ops.  */
  write_memory (sp + 1, buf, call_length);

  /* Finally, update the SP register.  */
  regcache_cooked_write_unsigned (regcache, AVR_SP_REGNUM,
				  avr_convert_saddr_to_raw (sp));

  /* Return SP value for the dummy frame, where the return address hasn't been
     pushed.  */
  return sp + call_length;
}

/* Unfortunately dwarf2 register for SP is 32.  */

static int
avr_dwarf_reg_to_regnum (struct gdbarch *gdbarch, int reg)
{
  if (reg >= 0 && reg < 32)
    return reg;
  if (reg == 32)
    return AVR_SP_REGNUM;
  return -1;
}

/* Implementation of `address_class_type_flags' gdbarch method.

   This method maps DW_AT_address_class attributes to a
   type_instance_flag_value.  */

static type_instance_flags
avr_address_class_type_flags (int byte_size, int dwarf2_addr_class)
{
  /* The value 1 of the DW_AT_address_class attribute corresponds to the
     __flash qualifier.  Note that this attribute is only valid with
     pointer types and therefore the flag is set to the pointer type and
     not its target type.  */
  if (dwarf2_addr_class == 1 && byte_size == 2)
    return AVR_TYPE_INSTANCE_FLAG_ADDRESS_CLASS_FLASH;
  return 0;
}

/* Implementation of `address_class_type_flags_to_name' gdbarch method.

   Convert a type_instance_flag_value to an address space qualifier.  */

static const char*
avr_address_class_type_flags_to_name (struct gdbarch *gdbarch,
				      type_instance_flags type_flags)
{
  if (type_flags & AVR_TYPE_INSTANCE_FLAG_ADDRESS_CLASS_FLASH)
    return "flash";
  else
    return NULL;
}

/* Implementation of `address_class_name_to_type_flags' gdbarch method.

   Convert an address space qualifier to a type_instance_flag_value.  */

static bool
avr_address_class_name_to_type_flags (struct gdbarch *gdbarch,
				      const char* name,
				      type_instance_flags *type_flags_ptr)
{
  if (strcmp (name, "flash") == 0)
    {
      *type_flags_ptr = AVR_TYPE_INSTANCE_FLAG_ADDRESS_CLASS_FLASH;
      return true;
    }
  else
    return false;
}

/* Initialize the gdbarch structure for the AVR's.  */

static struct gdbarch *
avr_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  struct gdbarch_list *best_arch;
  int call_length;

  /* Avr-6 call instructions save 3 bytes.  */
  switch (info.bfd_arch_info->mach)
    {
    case bfd_mach_avr1:
    case bfd_mach_avrxmega1:
    case bfd_mach_avr2:
    case bfd_mach_avrxmega2:
    case bfd_mach_avr3:
    case bfd_mach_avrxmega3:
    case bfd_mach_avr4:
    case bfd_mach_avrxmega4:
    case bfd_mach_avr5:
    case bfd_mach_avrxmega5:
    default:
      call_length = 2;
      break;
    case bfd_mach_avr6:
    case bfd_mach_avrxmega6:
    case bfd_mach_avrxmega7:
      call_length = 3;
      break;
    }

  /* If there is already a candidate, use it.  */
  for (best_arch = gdbarch_list_lookup_by_info (arches, &info);
       best_arch != NULL;
       best_arch = gdbarch_list_lookup_by_info (best_arch->next, &info))
    {
      avr_gdbarch_tdep *tdep
	= gdbarch_tdep<avr_gdbarch_tdep> (best_arch->gdbarch);

      if (tdep->call_length == call_length)
	return best_arch->gdbarch;
    }

  /* None found, create a new architecture from the information provided.  */
  gdbarch *gdbarch
    = gdbarch_alloc (&info, gdbarch_tdep_up (new avr_gdbarch_tdep));
  avr_gdbarch_tdep *tdep = gdbarch_tdep<avr_gdbarch_tdep> (gdbarch);
  
  tdep->call_length = call_length;

  /* Create a type for PC.  We can't use builtin types here, as they may not
     be defined.  */
  type_allocator alloc (gdbarch);
  tdep->void_type = alloc.new_type (TYPE_CODE_VOID, TARGET_CHAR_BIT, "void");
  tdep->func_void_type = make_function_type (tdep->void_type, NULL);
  tdep->pc_type = init_pointer_type (alloc, 4 * TARGET_CHAR_BIT, NULL,
				     tdep->func_void_type);

  set_gdbarch_short_bit (gdbarch, 2 * TARGET_CHAR_BIT);
  set_gdbarch_int_bit (gdbarch, 2 * TARGET_CHAR_BIT);
  set_gdbarch_long_bit (gdbarch, 4 * TARGET_CHAR_BIT);
  set_gdbarch_long_long_bit (gdbarch, 8 * TARGET_CHAR_BIT);
  set_gdbarch_ptr_bit (gdbarch, 2 * TARGET_CHAR_BIT);
  set_gdbarch_addr_bit (gdbarch, 32);

  set_gdbarch_wchar_bit (gdbarch, 2 * TARGET_CHAR_BIT);
  set_gdbarch_wchar_signed (gdbarch, 1);

  set_gdbarch_float_bit (gdbarch, 4 * TARGET_CHAR_BIT);
  set_gdbarch_double_bit (gdbarch, 4 * TARGET_CHAR_BIT);
  set_gdbarch_long_double_bit (gdbarch, 4 * TARGET_CHAR_BIT);

  set_gdbarch_float_format (gdbarch, floatformats_ieee_single);
  set_gdbarch_double_format (gdbarch, floatformats_ieee_single);
  set_gdbarch_long_double_format (gdbarch, floatformats_ieee_single);

  set_gdbarch_read_pc (gdbarch, avr_read_pc);
  set_gdbarch_write_pc (gdbarch, avr_write_pc);

  set_gdbarch_num_regs (gdbarch, AVR_NUM_REGS);

  set_gdbarch_sp_regnum (gdbarch, AVR_SP_REGNUM);
  set_gdbarch_pc_regnum (gdbarch, AVR_PC_REGNUM);

  set_gdbarch_register_name (gdbarch, avr_register_name);
  set_gdbarch_register_type (gdbarch, avr_register_type);

  set_gdbarch_num_pseudo_regs (gdbarch, AVR_NUM_PSEUDO_REGS);
  set_gdbarch_pseudo_register_read (gdbarch, avr_pseudo_register_read);
  set_gdbarch_deprecated_pseudo_register_write (gdbarch,
						avr_pseudo_register_write);

  set_gdbarch_return_value (gdbarch, avr_return_value);

  set_gdbarch_push_dummy_call (gdbarch, avr_push_dummy_call);

  set_gdbarch_dwarf2_reg_to_regnum (gdbarch, avr_dwarf_reg_to_regnum);

  set_gdbarch_address_to_pointer (gdbarch, avr_address_to_pointer);
  set_gdbarch_pointer_to_address (gdbarch, avr_pointer_to_address);
  set_gdbarch_integer_to_address (gdbarch, avr_integer_to_address);

  set_gdbarch_skip_prologue (gdbarch, avr_skip_prologue);
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);

  set_gdbarch_breakpoint_kind_from_pc (gdbarch, avr_breakpoint::kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch, avr_breakpoint::bp_from_kind);

  frame_unwind_append_unwinder (gdbarch, &avr_frame_unwind);
  frame_base_set_default (gdbarch, &avr_frame_base);

  set_gdbarch_dummy_id (gdbarch, avr_dummy_id);

  set_gdbarch_unwind_pc (gdbarch, avr_unwind_pc);
  set_gdbarch_unwind_sp (gdbarch, avr_unwind_sp);

  set_gdbarch_address_class_type_flags (gdbarch, avr_address_class_type_flags);
  set_gdbarch_address_class_name_to_type_flags
    (gdbarch, avr_address_class_name_to_type_flags);
  set_gdbarch_address_class_type_flags_to_name
    (gdbarch, avr_address_class_type_flags_to_name);

  return gdbarch;
}

/* Send a query request to the avr remote target asking for values of the io
   registers.  If args parameter is not NULL, then the user has requested info
   on a specific io register [This still needs implemented and is ignored for
   now].  The query string should be one of these forms:

   "Ravr.io_reg" -> reply is "NN" number of io registers

   "Ravr.io_reg:addr,len" where addr is first register and len is number of
   registers to be read.  The reply should be "<NAME>,VV;" for each io register
   where, <NAME> is a string, and VV is the hex value of the register.

   All io registers are 8-bit.  */

static void
avr_io_reg_read_command (const char *args, int from_tty)
{
  char query[400];
  unsigned int nreg = 0;
  unsigned int val;

  /* Find out how many io registers the target has.  */
  std::optional<gdb::byte_vector> buf
    = target_read_alloc (current_inferior ()->top_target (),
			 TARGET_OBJECT_AVR, "avr.io_reg");

  if (!buf)
    {
      gdb_printf (gdb_stderr,
		  _("ERR: info io_registers NOT supported "
		    "by current target\n"));
      return;
    }

  const char *bufstr = (const char *) buf->data ();

  if (sscanf (bufstr, "%x", &nreg) != 1)
    {
      gdb_printf (gdb_stderr,
		  _("Error fetching number of io registers\n"));
      return;
    }

  gdb_printf (_("Target has %u io registers:\n\n"), nreg);

  /* only fetch up to 8 registers at a time to keep the buffer small */
  int step = 8;

  for (int i = 0; i < nreg; i += step)
    {
      /* how many registers this round? */
      int j = step;
      if ((i+j) >= nreg)
	j = nreg - i;           /* last block is less than 8 registers */

      snprintf (query, sizeof (query) - 1, "avr.io_reg:%x,%x", i, j);
      buf = target_read_alloc (current_inferior ()->top_target (),
			       TARGET_OBJECT_AVR, query);

      if (!buf)
	{
	  gdb_printf (gdb_stderr,
		      _("ERR: error reading avr.io_reg:%x,%x\n"),
		      i, j);
	  return;
	}

      const char *p = (const char *) buf->data ();
      for (int k = i; k < (i + j); k++)
	{
	  if (sscanf (p, "%[^,],%x;", query, &val) == 2)
	    {
	      gdb_printf ("[%02x] %-15s : %02x\n", k, query, val);
	      while ((*p != ';') && (*p != '\0'))
		p++;
	      p++;		/* skip over ';' */
	      if (*p == '\0')
		break;
	    }
	}
    }
}

void _initialize_avr_tdep ();
void
_initialize_avr_tdep ()
{
  gdbarch_register (bfd_arch_avr, avr_gdbarch_init);

  /* Add a new command to allow the user to query the avr remote target for
     the values of the io space registers in a saner way than just using
     `x/NNNb ADDR`.  */

  /* FIXME: TRoth/2002-02-18: This should probably be changed to 'info avr
     io_registers' to signify it is not available on other platforms.  */

  add_info ("io_registers", avr_io_reg_read_command,
	    _("Query remote AVR target for I/O space register values."));
}
