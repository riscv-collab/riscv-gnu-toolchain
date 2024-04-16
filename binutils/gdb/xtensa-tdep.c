/* Target-dependent code for the Xtensa port of GDB, the GNU debugger.

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

#include "defs.h"
#include "frame.h"
#include "solib-svr4.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "gdbcore.h"
#include "value.h"
#include "osabi.h"
#include "regcache.h"
#include "reggroups.h"
#include "regset.h"

#include "dwarf2/frame.h"
#include "frame-base.h"
#include "frame-unwind.h"

#include "arch-utils.h"
#include "gdbarch.h"

#include "command.h"
#include "gdbcmd.h"

#include "xtensa-isa.h"
#include "xtensa-tdep.h"
#include "xtensa-config.h"
#include <algorithm>


static unsigned int xtensa_debug_level = 0;

#define DEBUGWARN(args...) \
  if (xtensa_debug_level > 0) \
    gdb_printf (gdb_stdlog, "(warn ) " args)

#define DEBUGINFO(args...) \
  if (xtensa_debug_level > 1) \
    gdb_printf (gdb_stdlog, "(info ) " args)

#define DEBUGTRACE(args...) \
  if (xtensa_debug_level > 2) \
    gdb_printf (gdb_stdlog, "(trace) " args)

#define DEBUGVERB(args...) \
  if (xtensa_debug_level > 3) \
    gdb_printf (gdb_stdlog, "(verb ) " args)


/* According to the ABI, the SP must be aligned to 16-byte boundaries.  */
#define SP_ALIGNMENT 16


/* On Windowed ABI, we use a6 through a11 for passing arguments
   to a function called by GDB because CALL4 is used.  */
#define ARGS_NUM_REGS		6
#define REGISTER_SIZE		4


/* Extract the call size from the return address or PS register.  */
#define PS_CALLINC_SHIFT	16
#define PS_CALLINC_MASK		0x00030000
#define CALLINC(ps)		(((ps) & PS_CALLINC_MASK) >> PS_CALLINC_SHIFT)
#define WINSIZE(ra)		(4 * (( (ra) >> 30) & 0x3))

/* On TX,  hardware can be configured without Exception Option.
   There is no PS register in this case.  Inside XT-GDB,  let us treat
   it as a virtual read-only register always holding the same value.  */
#define TX_PS			0x20

/* ABI-independent macros.  */
#define ARG_NOF(tdep) \
  (tdep->call_abi \
   == CallAbiCall0Only ? C0_NARGS : (ARGS_NUM_REGS))
#define ARG_1ST(tdep) \
  (tdep->call_abi  == CallAbiCall0Only \
   ? (tdep->a0_base + C0_ARGS) \
   : (tdep->a0_base + 6))

/* XTENSA_IS_ENTRY tests whether the first byte of an instruction
   indicates that the instruction is an ENTRY instruction.  */

#define XTENSA_IS_ENTRY(gdbarch, op1) \
  ((gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG) \
   ? ((op1) == 0x6c) : ((op1) == 0x36))

#define XTENSA_ENTRY_LENGTH	3

/* windowing_enabled() returns true, if windowing is enabled.
   WOE must be set to 1; EXCM to 0.
   Note: We assume that EXCM is always 0 for XEA1.  */

#define PS_WOE			(1<<18)
#define PS_EXC			(1<<4)

/* Big enough to hold the size of the largest register in bytes.  */
#define XTENSA_MAX_REGISTER_SIZE	64

static int
windowing_enabled (struct gdbarch *gdbarch, unsigned int ps)
{
  xtensa_gdbarch_tdep *tdep = gdbarch_tdep<xtensa_gdbarch_tdep> (gdbarch);

  /* If we know CALL0 ABI is set explicitly,  say it is Call0.  */
  if (tdep->call_abi == CallAbiCall0Only)
    return 0;

  return ((ps & PS_EXC) == 0 && (ps & PS_WOE) != 0);
}

/* Convert a live A-register number to the corresponding AR-register
   number.  */
static int
arreg_number (struct gdbarch *gdbarch, int a_regnum, ULONGEST wb)
{
  xtensa_gdbarch_tdep *tdep = gdbarch_tdep<xtensa_gdbarch_tdep> (gdbarch);
  int arreg;

  arreg = a_regnum - tdep->a0_base;
  arreg += (wb & ((tdep->num_aregs - 1) >> 2)) << WB_SHIFT;
  arreg &= tdep->num_aregs - 1;

  return arreg + tdep->ar_base;
}

/* Convert a live AR-register number to the corresponding A-register order
   number in a range [0..15].  Return -1, if AR_REGNUM is out of WB window.  */
static int
areg_number (struct gdbarch *gdbarch, int ar_regnum, unsigned int wb)
{
  xtensa_gdbarch_tdep *tdep = gdbarch_tdep<xtensa_gdbarch_tdep> (gdbarch);
  int areg;

  areg = ar_regnum - tdep->ar_base;
  if (areg < 0 || areg >= tdep->num_aregs)
    return -1;
  areg = (areg - wb * 4) & (tdep->num_aregs - 1);
  return (areg > 15) ? -1 : areg;
}

/* Read Xtensa register directly from the hardware.  */ 
static unsigned long
xtensa_read_register (int regnum)
{
  ULONGEST value;

  regcache_raw_read_unsigned (get_thread_regcache (inferior_thread ()), regnum,
			      &value);
  return (unsigned long) value;
}

/* Write Xtensa register directly to the hardware.  */ 
static void
xtensa_write_register (int regnum, ULONGEST value)
{
  regcache_raw_write_unsigned (get_thread_regcache (inferior_thread ()), regnum,
			       value);
}

/* Return the window size of the previous call to the function from which we
   have just returned.

   This function is used to extract the return value after a called function
   has returned to the caller.  On Xtensa, the register that holds the return
   value (from the perspective of the caller) depends on what call
   instruction was used.  For now, we are assuming that the call instruction
   precedes the current address, so we simply analyze the call instruction.
   If we are in a dummy frame, we simply return 4 as we used a 'pseudo-call4'
   method to call the inferior function.  */

static int
extract_call_winsize (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int winsize = 4;
  int insn;
  gdb_byte buf[4];

  DEBUGTRACE ("extract_call_winsize (pc = 0x%08x)\n", (int) pc);

  /* Read the previous instruction (should be a call[x]{4|8|12}.  */
  read_memory (pc-3, buf, 3);
  insn = extract_unsigned_integer (buf, 3, byte_order);

  /* Decode call instruction:
     Little Endian
       call{0,4,8,12}   OFFSET || {00,01,10,11} || 0101
       callx{0,4,8,12}  OFFSET || 11 || {00,01,10,11} || 0000
     Big Endian
       call{0,4,8,12}   0101 || {00,01,10,11} || OFFSET
       callx{0,4,8,12}  0000 || {00,01,10,11} || 11 || OFFSET.  */

  if (byte_order == BFD_ENDIAN_LITTLE)
    {
      if (((insn & 0xf) == 0x5) || ((insn & 0xcf) == 0xc0))
	winsize = (insn & 0x30) >> 2;   /* 0, 4, 8, 12.  */
    }
  else
    {
      if (((insn >> 20) == 0x5) || (((insn >> 16) & 0xf3) == 0x03))
	winsize = (insn >> 16) & 0xc;   /* 0, 4, 8, 12.  */
    }
  return winsize;
}


/* REGISTER INFORMATION */

/* Find register by name.  */
static int
xtensa_find_register_by_name (struct gdbarch *gdbarch, const char *name)
{
  int i;
  xtensa_gdbarch_tdep *tdep = gdbarch_tdep<xtensa_gdbarch_tdep> (gdbarch);

  for (i = 0; i < gdbarch_num_cooked_regs (gdbarch); i++)
    if (strcasecmp (tdep->regmap[i].name, name) == 0)
      return i;

  return -1;
}

/* Returns the name of a register.  */
static const char *
xtensa_register_name (struct gdbarch *gdbarch, int regnum)
{
  xtensa_gdbarch_tdep *tdep = gdbarch_tdep<xtensa_gdbarch_tdep> (gdbarch);

  /* Return the name stored in the register map.  */
  return tdep->regmap[regnum].name;
}

/* Return the type of a register.  Create a new type, if necessary.  */

static struct type *
xtensa_register_type (struct gdbarch *gdbarch, int regnum)
{
  xtensa_gdbarch_tdep *tdep = gdbarch_tdep<xtensa_gdbarch_tdep> (gdbarch);

  /* Return signed integer for ARx and Ax registers.  */
  if ((regnum >= tdep->ar_base
       && regnum < tdep->ar_base + tdep->num_aregs)
      || (regnum >= tdep->a0_base
	  && regnum < tdep->a0_base + 16))
    return builtin_type (gdbarch)->builtin_int;

  if (regnum == gdbarch_pc_regnum (gdbarch)
      || regnum == tdep->a0_base + 1)
    return builtin_type (gdbarch)->builtin_data_ptr;

  /* Return the stored type for all other registers.  */
  else if (regnum >= 0 && regnum < gdbarch_num_cooked_regs (gdbarch))
    {
      xtensa_register_t* reg = &tdep->regmap[regnum];

      /* Set ctype for this register (only the first time).  */

      if (reg->ctype == 0)
	{
	  struct ctype_cache *tp;
	  int size = reg->byte_size;

	  /* We always use the memory representation,
	     even if the register width is smaller.  */
	  switch (size)
	    {
	    case 1:
	      reg->ctype = builtin_type (gdbarch)->builtin_uint8;
	      break;

	    case 2:
	      reg->ctype = builtin_type (gdbarch)->builtin_uint16;
	      break;

	    case 4:
	      reg->ctype = builtin_type (gdbarch)->builtin_uint32;
	      break;

	    case 8:
	      reg->ctype = builtin_type (gdbarch)->builtin_uint64;
	      break;

	    case 16:
	      reg->ctype = builtin_type (gdbarch)->builtin_uint128;
	      break;

	    default:
	      for (tp = tdep->type_entries; tp != NULL; tp = tp->next)
		if (tp->size == size)
		  break;

	      if (tp == NULL)
		{
		  std::string name = string_printf ("int%d", size * 8);

		  tp = XNEW (struct ctype_cache);
		  tp->next = tdep->type_entries;
		  tdep->type_entries = tp;
		  tp->size = size;
		  type_allocator alloc (gdbarch);
		  tp->virtual_type
		    = init_integer_type (alloc, size * 8, 1, name.c_str ());
		}

	      reg->ctype = tp->virtual_type;
	    }
	}
      return reg->ctype;
    }

  internal_error (_("invalid register number %d"), regnum);
  return 0;
}


/* Return the 'local' register number for stubs, dwarf2, etc.
   The debugging information enumerates registers starting from 0 for A0
   to n for An.  So, we only have to add the base number for A0.  */

static int
xtensa_reg_to_regnum (struct gdbarch *gdbarch, int regnum)
{
  int i;
  xtensa_gdbarch_tdep *tdep = gdbarch_tdep<xtensa_gdbarch_tdep> (gdbarch);

  if (regnum >= 0 && regnum < 16)
    return tdep->a0_base + regnum;

  for (i = 0; i < gdbarch_num_cooked_regs (gdbarch); i++)
    if (regnum == tdep->regmap[i].target_number)
      return i;

  return -1;
}


/* Write the bits of a masked register to the various registers.
   Only the masked areas of these registers are modified; the other
   fields are untouched.  The size of masked registers is always less
   than or equal to 32 bits.  */

static void
xtensa_register_write_masked (struct regcache *regcache,
			      xtensa_register_t *reg, const gdb_byte *buffer)
{
  unsigned int value[(XTENSA_MAX_REGISTER_SIZE + 3) / 4];
  const xtensa_mask_t *mask = reg->mask;

  int shift = 0;		/* Shift for next mask (mod 32).  */
  int start, size;		/* Start bit and size of current mask.  */

  unsigned int *ptr = value;
  unsigned int regval, m, mem = 0;

  int bytesize = reg->byte_size;
  int bitsize = bytesize * 8;
  int i, r;

  DEBUGTRACE ("xtensa_register_write_masked ()\n");

  /* Copy the masked register to host byte-order.  */
  if (gdbarch_byte_order (regcache->arch ()) == BFD_ENDIAN_BIG)
    for (i = 0; i < bytesize; i++)
      {
	mem >>= 8;
	mem |= (buffer[bytesize - i - 1] << 24);
	if ((i & 3) == 3)
	  *ptr++ = mem;
      }
  else
    for (i = 0; i < bytesize; i++)
      {
	mem >>= 8;
	mem |= (buffer[i] << 24);
	if ((i & 3) == 3)
	  *ptr++ = mem;
      }

  /* We might have to shift the final value:
     bytesize & 3 == 0 -> nothing to do, we use the full 32 bits,
     bytesize & 3 == x -> shift (4-x) * 8.  */

  *ptr = mem >> (((0 - bytesize) & 3) * 8);
  ptr = value;
  mem = *ptr;

  /* Write the bits to the masked areas of the other registers.  */
  for (i = 0; i < mask->count; i++)
    {
      start = mask->mask[i].bit_start;
      size = mask->mask[i].bit_size;
      regval = mem >> shift;

      if ((shift += size) > bitsize)
	error (_("size of all masks is larger than the register"));

      if (shift >= 32)
	{
	  mem = *(++ptr);
	  shift -= 32;
	  bitsize -= 32;

	  if (shift > 0)
	    regval |= mem << (size - shift);
	}

      /* Make sure we have a valid register.  */
      r = mask->mask[i].reg_num;
      if (r >= 0 && size > 0)
	{
	  /* Don't overwrite the unmasked areas.  */
	  ULONGEST old_val;
	  regcache_cooked_read_unsigned (regcache, r, &old_val);
	  m = 0xffffffff >> (32 - size) << start;
	  regval <<= start;
	  regval = (regval & m) | (old_val & ~m);
	  regcache_cooked_write_unsigned (regcache, r, regval);
	}
    }
}


/* Read a tie state or mapped registers.  Read the masked areas
   of the registers and assemble them into a single value.  */

static enum register_status
xtensa_register_read_masked (readable_regcache *regcache,
			     xtensa_register_t *reg, gdb_byte *buffer)
{
  unsigned int value[(XTENSA_MAX_REGISTER_SIZE + 3) / 4];
  const xtensa_mask_t *mask = reg->mask;

  int shift = 0;
  int start, size;

  unsigned int *ptr = value;
  unsigned int regval, mem = 0;

  int bytesize = reg->byte_size;
  int bitsize = bytesize * 8;
  int i;

  DEBUGTRACE ("xtensa_register_read_masked (reg \"%s\", ...)\n",
	      reg->name == 0 ? "" : reg->name);

  /* Assemble the register from the masked areas of other registers.  */
  for (i = 0; i < mask->count; i++)
    {
      int r = mask->mask[i].reg_num;
      if (r >= 0)
	{
	  enum register_status status;
	  ULONGEST val;

	  status = regcache->cooked_read (r, &val);
	  if (status != REG_VALID)
	    return status;
	  regval = (unsigned int) val;
	}
      else
	regval = 0;

      start = mask->mask[i].bit_start;
      size = mask->mask[i].bit_size;

      regval >>= start;

      if (size < 32)
	regval &= (0xffffffff >> (32 - size));

      mem |= regval << shift;

      if ((shift += size) > bitsize)
	error (_("size of all masks is larger than the register"));

      if (shift >= 32)
	{
	  *ptr++ = mem;
	  bitsize -= 32;
	  shift -= 32;

	  if (shift == 0)
	    mem = 0;
	  else
	    mem = regval >> (size - shift);
	}
    }

  if (shift > 0)
    *ptr = mem;

  /* Copy value to target byte order.  */
  ptr = value;
  mem = *ptr;

  if (gdbarch_byte_order (regcache->arch ()) == BFD_ENDIAN_BIG)
    for (i = 0; i < bytesize; i++)
      {
	if ((i & 3) == 0)
	  mem = *ptr++;
	buffer[bytesize - i - 1] = mem & 0xff;
	mem >>= 8;
      }
  else
    for (i = 0; i < bytesize; i++)
      {
	if ((i & 3) == 0)
	  mem = *ptr++;
	buffer[i] = mem & 0xff;
	mem >>= 8;
      }

  return REG_VALID;
}


/* Read pseudo registers.  */

static enum register_status
xtensa_pseudo_register_read (struct gdbarch *gdbarch,
			     readable_regcache *regcache,
			     int regnum,
			     gdb_byte *buffer)
{
  DEBUGTRACE ("xtensa_pseudo_register_read (... regnum = %d (%s) ...)\n",
	      regnum, xtensa_register_name (gdbarch, regnum));
  xtensa_gdbarch_tdep *tdep = gdbarch_tdep<xtensa_gdbarch_tdep> (gdbarch);

  /* Read aliases a0..a15, if this is a Windowed ABI.  */
  if (tdep->isa_use_windowed_registers
      && (regnum >= tdep->a0_base)
      && (regnum <= tdep->a0_base + 15))
    {
      ULONGEST value;
      enum register_status status;

      status = regcache->raw_read (tdep->wb_regnum,
				   &value);
      if (status != REG_VALID)
	return status;
      regnum = arreg_number (gdbarch, regnum, value);
    }

  /* We can always read non-pseudo registers.  */
  if (regnum >= 0 && regnum < gdbarch_num_regs (gdbarch))
    return regcache->raw_read (regnum, buffer);

  /* We have to find out how to deal with privileged registers.
     Let's treat them as pseudo-registers, but we cannot read/write them.  */
     
  else if (tdep->call_abi == CallAbiCall0Only
	   || regnum < tdep->a0_base)
    {
      buffer[0] = (gdb_byte)0;
      buffer[1] = (gdb_byte)0;
      buffer[2] = (gdb_byte)0;
      buffer[3] = (gdb_byte)0;
      return REG_VALID;
    }
  /* Pseudo registers.  */
  else if (regnum >= 0 && regnum < gdbarch_num_cooked_regs (gdbarch))
    {
      xtensa_register_t *reg = &tdep->regmap[regnum];
      xtensa_register_type_t type = reg->type;
      int flags = tdep->target_flags;

      /* We cannot read Unknown or Unmapped registers.  */
      if (type == xtRegisterTypeUnmapped || type == xtRegisterTypeUnknown)
	{
	  if ((flags & xtTargetFlagsNonVisibleRegs) == 0)
	    {
	      warning (_("cannot read register %s"),
		       xtensa_register_name (gdbarch, regnum));
	      return REG_VALID;
	    }
	}

      /* Some targets cannot read TIE register files.  */
      else if (type == xtRegisterTypeTieRegfile)
	{
	  /* Use 'fetch' to get register?  */
	  if (flags & xtTargetFlagsUseFetchStore)
	    {
	      warning (_("cannot read register"));
	      return REG_VALID;
	    }

	  /* On some targets (esp. simulators), we can always read the reg.  */
	  else if ((flags & xtTargetFlagsNonVisibleRegs) == 0)
	    {
	      warning (_("cannot read register"));
	      return REG_VALID;
	    }
	}

      /* We can always read mapped registers.  */
      else if (type == xtRegisterTypeMapped || type == xtRegisterTypeTieState)
	return xtensa_register_read_masked (regcache, reg, buffer);

      /* Assume that we can read the register.  */
      return regcache->raw_read (regnum, buffer);
    }
  else
    internal_error (_("invalid register number %d"), regnum);
}


/* Write pseudo registers.  */

static void
xtensa_pseudo_register_write (struct gdbarch *gdbarch,
			      struct regcache *regcache,
			      int regnum,
			      const gdb_byte *buffer)
{
  DEBUGTRACE ("xtensa_pseudo_register_write (... regnum = %d (%s) ...)\n",
	      regnum, xtensa_register_name (gdbarch, regnum));
  xtensa_gdbarch_tdep *tdep = gdbarch_tdep<xtensa_gdbarch_tdep> (gdbarch);

  /* Renumber register, if aliases a0..a15 on Windowed ABI.  */
  if (tdep->isa_use_windowed_registers
      && (regnum >= tdep->a0_base)
      && (regnum <= tdep->a0_base + 15))
    {
      ULONGEST value;
      regcache_raw_read_unsigned (regcache,
				  tdep->wb_regnum, &value);
      regnum = arreg_number (gdbarch, regnum, value);
    }

  /* We can always write 'core' registers.
     Note: We might have converted Ax->ARy.  */
  if (regnum >= 0 && regnum < gdbarch_num_regs (gdbarch))
    regcache->raw_write (regnum, buffer);

  /* We have to find out how to deal with privileged registers.
     Let's treat them as pseudo-registers, but we cannot read/write them.  */

  else if (regnum < tdep->a0_base)
    {
      return;
    }
  /* Pseudo registers.  */
  else if (regnum >= 0 && regnum < gdbarch_num_cooked_regs (gdbarch))
    {
      xtensa_register_t *reg = &tdep->regmap[regnum];
      xtensa_register_type_t type = reg->type;
      int flags = tdep->target_flags;

      /* On most targets, we cannot write registers
	 of type "Unknown" or "Unmapped".  */
      if (type == xtRegisterTypeUnmapped || type == xtRegisterTypeUnknown)
	{
	  if ((flags & xtTargetFlagsNonVisibleRegs) == 0)
	    {
	      warning (_("cannot write register %s"),
		       xtensa_register_name (gdbarch, regnum));
	      return;
	    }
	}

      /* Some targets cannot read TIE register files.  */
      else if (type == xtRegisterTypeTieRegfile)
	{
	  /* Use 'store' to get register?  */
	  if (flags & xtTargetFlagsUseFetchStore)
	    {
	      warning (_("cannot write register"));
	      return;
	    }

	  /* On some targets (esp. simulators), we can always write
	     the register.  */
	  else if ((flags & xtTargetFlagsNonVisibleRegs) == 0)
	    {
	      warning (_("cannot write register"));
	      return;
	    }
	}

      /* We can always write mapped registers.  */
      else if (type == xtRegisterTypeMapped || type == xtRegisterTypeTieState)
	{
	  xtensa_register_write_masked (regcache, reg, buffer);
	  return;
	}

      /* Assume that we can write the register.  */
      regcache->raw_write (regnum, buffer);
    }
  else
    internal_error (_("invalid register number %d"), regnum);
}

static const reggroup *xtensa_ar_reggroup;
static const reggroup *xtensa_user_reggroup;
static const reggroup *xtensa_vectra_reggroup;
static const reggroup *xtensa_cp[XTENSA_MAX_COPROCESSOR];

static void
xtensa_init_reggroups (void)
{
  int i;

  xtensa_ar_reggroup = reggroup_new ("ar", USER_REGGROUP);
  xtensa_user_reggroup = reggroup_new ("user", USER_REGGROUP);
  xtensa_vectra_reggroup = reggroup_new ("vectra", USER_REGGROUP);

  for (i = 0; i < XTENSA_MAX_COPROCESSOR; i++)
    xtensa_cp[i] = reggroup_new (xstrprintf ("cp%d", i).release (),
				 USER_REGGROUP);
}

static void
xtensa_add_reggroups (struct gdbarch *gdbarch)
{
  /* Xtensa-specific groups.  */
  reggroup_add (gdbarch, xtensa_ar_reggroup);
  reggroup_add (gdbarch, xtensa_user_reggroup);
  reggroup_add (gdbarch, xtensa_vectra_reggroup);

  for (int i = 0; i < XTENSA_MAX_COPROCESSOR; i++)
    reggroup_add (gdbarch, xtensa_cp[i]);
}

static int 
xtensa_coprocessor_register_group (const struct reggroup *group)
{
  int i;

  for (i = 0; i < XTENSA_MAX_COPROCESSOR; i++)
    if (group == xtensa_cp[i])
      return i;

  return -1;
}

#define SAVE_REST_FLAGS	(XTENSA_REGISTER_FLAGS_READABLE \
			| XTENSA_REGISTER_FLAGS_WRITABLE \
			| XTENSA_REGISTER_FLAGS_VOLATILE)

#define SAVE_REST_VALID	(XTENSA_REGISTER_FLAGS_READABLE \
			| XTENSA_REGISTER_FLAGS_WRITABLE)

static int
xtensa_register_reggroup_p (struct gdbarch *gdbarch,
			    int regnum,
			    const struct reggroup *group)
{
  xtensa_gdbarch_tdep *tdep = gdbarch_tdep<xtensa_gdbarch_tdep> (gdbarch);
  xtensa_register_t* reg = &tdep->regmap[regnum];
  xtensa_register_type_t type = reg->type;
  xtensa_register_group_t rg = reg->group;
  int cp_number;

  if (group == save_reggroup)
    /* Every single register should be included into the list of registers
       to be watched for changes while using -data-list-changed-registers.  */
    return 1;

  /* First, skip registers that are not visible to this target
     (unknown and unmapped registers when not using ISS).  */

  if (type == xtRegisterTypeUnmapped || type == xtRegisterTypeUnknown)
    return 0;
  if (group == all_reggroup)
    return 1;
  if (group == xtensa_ar_reggroup)
    return rg & xtRegisterGroupAddrReg;
  if (group == xtensa_user_reggroup)
    return rg & xtRegisterGroupUser;
  if (group == float_reggroup)
    return rg & xtRegisterGroupFloat;
  if (group == general_reggroup)
    return rg & xtRegisterGroupGeneral;
  if (group == system_reggroup)
    return rg & xtRegisterGroupState;
  if (group == vector_reggroup || group == xtensa_vectra_reggroup)
    return rg & xtRegisterGroupVectra;
  if (group == restore_reggroup)
    return (regnum < gdbarch_num_regs (gdbarch)
	    && (reg->flags & SAVE_REST_FLAGS) == SAVE_REST_VALID);
  cp_number = xtensa_coprocessor_register_group (group);
  if (cp_number >= 0)
    return rg & (xtRegisterGroupCP0 << cp_number);
  else
    return 1;
}


/* Supply register REGNUM from the buffer specified by GREGS and LEN
   in the general-purpose register set REGSET to register cache
   REGCACHE.  If REGNUM is -1 do this for all registers in REGSET.  */

static void
xtensa_supply_gregset (const struct regset *regset,
		       struct regcache *rc,
		       int regnum,
		       const void *gregs,
		       size_t len)
{
  const xtensa_elf_gregset_t *regs = (const xtensa_elf_gregset_t *) gregs;
  struct gdbarch *gdbarch = rc->arch ();
  xtensa_gdbarch_tdep *tdep = gdbarch_tdep<xtensa_gdbarch_tdep> (gdbarch);
  int i;

  DEBUGTRACE ("xtensa_supply_gregset (..., regnum==%d, ...)\n", regnum);

  if (regnum == gdbarch_pc_regnum (gdbarch) || regnum == -1)
    rc->raw_supply (gdbarch_pc_regnum (gdbarch), (char *) &regs->pc);
  if (regnum == gdbarch_ps_regnum (gdbarch) || regnum == -1)
    rc->raw_supply (gdbarch_ps_regnum (gdbarch), (char *) &regs->ps);
  if (regnum == tdep->wb_regnum || regnum == -1)
    rc->raw_supply (tdep->wb_regnum,
		    (char *) &regs->windowbase);
  if (regnum == tdep->ws_regnum || regnum == -1)
    rc->raw_supply (tdep->ws_regnum,
		    (char *) &regs->windowstart);
  if (regnum == tdep->lbeg_regnum || regnum == -1)
    rc->raw_supply (tdep->lbeg_regnum,
		    (char *) &regs->lbeg);
  if (regnum == tdep->lend_regnum || regnum == -1)
    rc->raw_supply (tdep->lend_regnum,
		    (char *) &regs->lend);
  if (regnum == tdep->lcount_regnum || regnum == -1)
    rc->raw_supply (tdep->lcount_regnum,
		    (char *) &regs->lcount);
  if (regnum == tdep->sar_regnum || regnum == -1)
    rc->raw_supply (tdep->sar_regnum,
		    (char *) &regs->sar);
  if (regnum >=tdep->ar_base
      && regnum < tdep->ar_base
		    + tdep->num_aregs)
    rc->raw_supply
      (regnum, (char *) &regs->ar[regnum - tdep->ar_base]);
  else if (regnum == -1)
    {
      for (i = 0; i < tdep->num_aregs; ++i)
	rc->raw_supply (tdep->ar_base + i,
			(char *) &regs->ar[i]);
    }
}


/* Xtensa register set.  */

static struct regset
xtensa_gregset =
{
  NULL,
  xtensa_supply_gregset
};


/* Iterate over supported core file register note sections. */

static void
xtensa_iterate_over_regset_sections (struct gdbarch *gdbarch,
				     iterate_over_regset_sections_cb *cb,
				     void *cb_data,
				     const struct regcache *regcache)
{
  DEBUGTRACE ("xtensa_iterate_over_regset_sections\n");

  cb (".reg", sizeof (xtensa_elf_gregset_t), sizeof (xtensa_elf_gregset_t),
      &xtensa_gregset, NULL, cb_data);
}


/* Handling frames.  */

/* Number of registers to save in case of Windowed ABI.  */
#define XTENSA_NUM_SAVED_AREGS		12

/* Frame cache part for Windowed ABI.  */
typedef struct xtensa_windowed_frame_cache
{
  int wb;		/* WINDOWBASE of the previous frame.  */
  int callsize;		/* Call size of this frame.  */
  int ws;		/* WINDOWSTART of the previous frame.  It keeps track of
			   life windows only.  If there is no bit set for the
			   window,  that means it had been already spilled
			   because of window overflow.  */

   /* Addresses of spilled A-registers.
      AREGS[i] == -1, if corresponding AR is alive.  */
  CORE_ADDR aregs[XTENSA_NUM_SAVED_AREGS];
} xtensa_windowed_frame_cache_t;

/* Call0 ABI Definitions.  */

#define C0_MAXOPDS  3	/* Maximum number of operands for prologue
			   analysis.  */
#define C0_CLESV   12	/* Callee-saved registers are here and up.  */
#define C0_SP	    1	/* Register used as SP.  */
#define C0_FP	   15	/* Register used as FP.  */
#define C0_RA	    0	/* Register used as return address.  */
#define C0_ARGS	    2	/* Register used as first arg/retval.  */
#define C0_NARGS    6	/* Number of A-regs for args/retvals.  */

/* Each element of xtensa_call0_frame_cache.c0_rt[] describes for each
   A-register where the current content of the reg came from (in terms
   of an original reg and a constant).  Negative values of c0_rt[n].fp_reg
   mean that the original content of the register was saved to the stack.
   c0_rt[n].fr.ofs is NOT the offset from the frame base because we don't 
   know where SP will end up until the entire prologue has been analyzed.  */

#define C0_CONST   -1	/* fr_reg value if register contains a constant.  */
#define C0_INEXP   -2	/* fr_reg value if inexpressible as reg + offset.  */
#define C0_NOSTK   -1	/* to_stk value if register has not been stored.  */

extern xtensa_isa xtensa_default_isa;

typedef struct xtensa_c0reg
{
  int fr_reg;  /* original register from which register content
		  is derived, or C0_CONST, or C0_INEXP.  */
  int fr_ofs;  /* constant offset from reg, or immediate value.  */
  int to_stk;  /* offset from original SP to register (4-byte aligned),
		  or C0_NOSTK if register has not been saved.  */
} xtensa_c0reg_t;

/* Frame cache part for Call0 ABI.  */
typedef struct xtensa_call0_frame_cache
{
  int c0_frmsz;			   /* Stack frame size.  */
  int c0_hasfp;			   /* Current frame uses frame pointer.  */
  int fp_regnum;		   /* A-register used as FP.  */
  int c0_fp;			   /* Actual value of frame pointer.  */
  int c0_fpalign;		   /* Dynamic adjustment for the stack
				      pointer. It's an AND mask. Zero,
				      if alignment was not adjusted.  */
  int c0_old_sp;		   /* In case of dynamic adjustment, it is
				      a register holding unaligned sp. 
				      C0_INEXP, when undefined.  */
  int c0_sp_ofs;		   /* If "c0_old_sp" was spilled it's a
				      stack offset. C0_NOSTK otherwise.  */
					   
  xtensa_c0reg_t c0_rt[C0_NREGS];  /* Register tracking information.  */
} xtensa_call0_frame_cache_t;

typedef struct xtensa_frame_cache
{
  CORE_ADDR base;	/* Stack pointer of this frame.  */
  CORE_ADDR pc;		/* PC of this frame at the function entry point.  */
  CORE_ADDR ra;		/* The raw return address of this frame.  */
  CORE_ADDR ps;		/* The PS register of the previous (older) frame.  */
  CORE_ADDR prev_sp;	/* Stack Pointer of the previous (older) frame.  */
  int call0;		/* It's a call0 framework (else windowed).  */
  union
    {
      xtensa_windowed_frame_cache_t	wd;	/* call0 == false.  */
      xtensa_call0_frame_cache_t       	c0;	/* call0 == true.  */
    };
} xtensa_frame_cache_t;


static struct xtensa_frame_cache *
xtensa_alloc_frame_cache (int windowed)
{
  xtensa_frame_cache_t *cache;
  int i;

  DEBUGTRACE ("xtensa_alloc_frame_cache ()\n");

  cache = FRAME_OBSTACK_ZALLOC (xtensa_frame_cache_t);

  cache->base = 0;
  cache->pc = 0;
  cache->ra = 0;
  cache->ps = 0;
  cache->prev_sp = 0;
  cache->call0 = !windowed;
  if (cache->call0)
    {
      cache->c0.c0_frmsz  = -1;
      cache->c0.c0_hasfp  =  0;
      cache->c0.fp_regnum = -1;
      cache->c0.c0_fp     = -1;
      cache->c0.c0_fpalign =  0;
      cache->c0.c0_old_sp  =  C0_INEXP;
      cache->c0.c0_sp_ofs  =  C0_NOSTK;

      for (i = 0; i < C0_NREGS; i++)
	{
	  cache->c0.c0_rt[i].fr_reg = i;
	  cache->c0.c0_rt[i].fr_ofs = 0;
	  cache->c0.c0_rt[i].to_stk = C0_NOSTK;
	}
    }
  else
    {
      cache->wd.wb = 0;
      cache->wd.ws = 0;
      cache->wd.callsize = -1;

      for (i = 0; i < XTENSA_NUM_SAVED_AREGS; i++)
	cache->wd.aregs[i] = -1;
    }
  return cache;
}


static CORE_ADDR
xtensa_frame_align (struct gdbarch *gdbarch, CORE_ADDR address)
{
  return address & ~15;
}


static CORE_ADDR
xtensa_unwind_pc (struct gdbarch *gdbarch, frame_info_ptr next_frame)
{
  gdb_byte buf[8];
  CORE_ADDR pc;

  DEBUGTRACE ("xtensa_unwind_pc (next_frame = %s)\n", 
		host_address_to_string (next_frame.get ()));

  frame_unwind_register (next_frame, gdbarch_pc_regnum (gdbarch), buf);
  pc = extract_typed_address (buf, builtin_type (gdbarch)->builtin_func_ptr);

  DEBUGINFO ("[xtensa_unwind_pc] pc = 0x%08x\n", (unsigned int) pc);

  return pc;
}


static struct frame_id
xtensa_dummy_id (struct gdbarch *gdbarch, frame_info_ptr this_frame)
{
  CORE_ADDR pc, fp;
  xtensa_gdbarch_tdep *tdep = gdbarch_tdep<xtensa_gdbarch_tdep> (gdbarch);

  /* THIS-FRAME is a dummy frame.  Return a frame ID of that frame.  */

  pc = get_frame_pc (this_frame);
  fp = get_frame_register_unsigned
	 (this_frame, tdep->a0_base + 1);

  /* Make dummy frame ID unique by adding a constant.  */
  return frame_id_build (fp + SP_ALIGNMENT, pc);
}

/* Returns true,  if instruction to execute next is unique to Xtensa Window
   Interrupt Handlers.  It can only be one of L32E,  S32E,  RFWO,  or RFWU.  */

static int
xtensa_window_interrupt_insn (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  unsigned int insn = read_memory_integer (pc, 4, byte_order);
  unsigned int code;

  if (byte_order == BFD_ENDIAN_BIG)
    {
      /* Check, if this is L32E or S32E.  */
      code = insn & 0xf000ff00;
      if ((code == 0x00009000) || (code == 0x00009400))
	return 1;
      /* Check, if this is RFWU or RFWO.  */
      code = insn & 0xffffff00;
      return ((code == 0x00430000) || (code == 0x00530000));
    }
  else
    {
      /* Check, if this is L32E or S32E.  */
      code = insn & 0x00ff000f;
      if ((code == 0x090000) || (code == 0x490000))
	return 1;
      /* Check, if this is RFWU or RFWO.  */
      code = insn & 0x00ffffff;
      return ((code == 0x00003400) || (code == 0x00003500));
    }
}

/* Returns the best guess about which register is a frame pointer
   for the function containing CURRENT_PC.  */

#define XTENSA_ISA_BSZ		32		/* Instruction buffer size.  */
#define XTENSA_ISA_BADPC	((CORE_ADDR)0)	/* Bad PC value.  */

static unsigned int
xtensa_scan_prologue (struct gdbarch *gdbarch, CORE_ADDR current_pc)
{
#define RETURN_FP goto done

  xtensa_gdbarch_tdep *tdep = gdbarch_tdep<xtensa_gdbarch_tdep> (gdbarch);
  unsigned int fp_regnum = tdep->a0_base + 1;
  CORE_ADDR start_addr;
  xtensa_isa isa;
  xtensa_insnbuf ins, slot;
  gdb_byte ibuf[XTENSA_ISA_BSZ];
  CORE_ADDR ia, bt, ba;
  xtensa_format ifmt;
  int ilen, islots, is;
  xtensa_opcode opc;
  const char *opcname;

  find_pc_partial_function (current_pc, NULL, &start_addr, NULL);
  if (start_addr == 0)
    return fp_regnum;

  isa = xtensa_default_isa;
  gdb_assert (XTENSA_ISA_BSZ >= xtensa_isa_maxlength (isa));
  ins = xtensa_insnbuf_alloc (isa);
  slot = xtensa_insnbuf_alloc (isa);
  ba = 0;

  for (ia = start_addr, bt = ia; ia < current_pc ; ia += ilen)
    {
      if (ia + xtensa_isa_maxlength (isa) > bt)
	{
	  ba = ia;
	  bt = (ba + XTENSA_ISA_BSZ) < current_pc
	    ? ba + XTENSA_ISA_BSZ : current_pc;
	  if (target_read_memory (ba, ibuf, bt - ba) != 0)
	    RETURN_FP;
	}

      xtensa_insnbuf_from_chars (isa, ins, &ibuf[ia-ba], 0);
      ifmt = xtensa_format_decode (isa, ins);
      if (ifmt == XTENSA_UNDEFINED)
	RETURN_FP;
      ilen = xtensa_format_length (isa, ifmt);
      if (ilen == XTENSA_UNDEFINED)
	RETURN_FP;
      islots = xtensa_format_num_slots (isa, ifmt);
      if (islots == XTENSA_UNDEFINED)
	RETURN_FP;
      
      for (is = 0; is < islots; ++is)
	{
	  if (xtensa_format_get_slot (isa, ifmt, is, ins, slot))
	    RETURN_FP;
	  
	  opc = xtensa_opcode_decode (isa, ifmt, is, slot);
	  if (opc == XTENSA_UNDEFINED) 
	    RETURN_FP;
	  
	  opcname = xtensa_opcode_name (isa, opc);

	  if (strcasecmp (opcname, "mov.n") == 0
	      || strcasecmp (opcname, "or") == 0)
	    {
	      unsigned int register_operand;

	      /* Possible candidate for setting frame pointer
		 from A1.  This is what we are looking for.  */

	      if (xtensa_operand_get_field (isa, opc, 1, ifmt, 
					    is, slot, &register_operand) != 0)
		RETURN_FP;
	      if (xtensa_operand_decode (isa, opc, 1, &register_operand) != 0)
		RETURN_FP;
	      if (register_operand == 1)  /* Mov{.n} FP A1.  */
		{
		  if (xtensa_operand_get_field (isa, opc, 0, ifmt, is, slot, 
						&register_operand) != 0)
		    RETURN_FP;
		  if (xtensa_operand_decode (isa, opc, 0,
					     &register_operand) != 0)
		    RETURN_FP;

		  fp_regnum
		    = tdep->a0_base + register_operand;
		  RETURN_FP;
		}
	    }

	  if (
	      /* We have problems decoding the memory.  */
	      opcname == NULL 
	      || strcasecmp (opcname, "ill") == 0
	      || strcasecmp (opcname, "ill.n") == 0
	      /* Hit planted breakpoint.  */
	      || strcasecmp (opcname, "break") == 0
	      || strcasecmp (opcname, "break.n") == 0
	      /* Flow control instructions finish prologue.  */
	      || xtensa_opcode_is_branch (isa, opc) > 0
	      || xtensa_opcode_is_jump   (isa, opc) > 0
	      || xtensa_opcode_is_loop   (isa, opc) > 0
	      || xtensa_opcode_is_call   (isa, opc) > 0
	      || strcasecmp (opcname, "simcall") == 0
	      || strcasecmp (opcname, "syscall") == 0)
	    /* Can not continue analysis.  */
	    RETURN_FP;
	}
    }
done:
  xtensa_insnbuf_free(isa, slot);
  xtensa_insnbuf_free(isa, ins);
  return fp_regnum;
}

/* The key values to identify the frame using "cache" are 

	cache->base    = SP (or best guess about FP) of this frame;
	cache->pc      = entry-PC (entry point of the frame function);
	cache->prev_sp = SP of the previous frame.  */

static void
call0_frame_cache (frame_info_ptr this_frame,
		   xtensa_frame_cache_t *cache, CORE_ADDR pc);

static void
xtensa_window_interrupt_frame_cache (frame_info_ptr this_frame,
				     xtensa_frame_cache_t *cache,
				     CORE_ADDR pc);

static struct xtensa_frame_cache *
xtensa_frame_cache (frame_info_ptr this_frame, void **this_cache)
{
  xtensa_frame_cache_t *cache;
  CORE_ADDR ra, wb, ws, pc, sp, ps;
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  unsigned int fp_regnum;
  int  windowed, ps_regnum;

  if (*this_cache)
    return (struct xtensa_frame_cache *) *this_cache;

  pc = get_frame_register_unsigned (this_frame, gdbarch_pc_regnum (gdbarch));
  ps_regnum = gdbarch_ps_regnum (gdbarch);
  ps = (ps_regnum >= 0
	? get_frame_register_unsigned (this_frame, ps_regnum) : TX_PS);

  windowed = windowing_enabled (gdbarch, ps);

  /* Get pristine xtensa-frame.  */
  cache = xtensa_alloc_frame_cache (windowed);
  *this_cache = cache;

  if (windowed)
    {
      LONGEST op1;
      xtensa_gdbarch_tdep *tdep = gdbarch_tdep<xtensa_gdbarch_tdep> (gdbarch);

      /* Get WINDOWBASE, WINDOWSTART, and PS registers.  */
      wb = get_frame_register_unsigned (this_frame, 
					tdep->wb_regnum);
      ws = get_frame_register_unsigned (this_frame,
					tdep->ws_regnum);

      if (safe_read_memory_integer (pc, 1, byte_order, &op1)
	  && XTENSA_IS_ENTRY (gdbarch, op1))
	{
	  int callinc = CALLINC (ps);
	  ra = get_frame_register_unsigned
	    (this_frame, tdep->a0_base + callinc * 4);
	  
	  /* ENTRY hasn't been executed yet, therefore callsize is still 0.  */
	  cache->wd.callsize = 0;
	  cache->wd.wb = wb;
	  cache->wd.ws = ws;
	  cache->prev_sp = get_frame_register_unsigned
			     (this_frame, tdep->a0_base + 1);

	  /* This only can be the outermost frame since we are
	     just about to execute ENTRY.  SP hasn't been set yet.
	     We can assume any frame size, because it does not
	     matter, and, let's fake frame base in cache.  */
	  cache->base = cache->prev_sp - 16;

	  cache->pc = pc;
	  cache->ra = (cache->pc & 0xc0000000) | (ra & 0x3fffffff);
	  cache->ps = (ps & ~PS_CALLINC_MASK)
	    | ((WINSIZE(ra)/4) << PS_CALLINC_SHIFT);

	  return cache;
	}
      else
	{
	  fp_regnum = xtensa_scan_prologue (gdbarch, pc);
	  ra = get_frame_register_unsigned (this_frame,
					    tdep->a0_base);
	  cache->wd.callsize = WINSIZE (ra);
	  cache->wd.wb = (wb - cache->wd.callsize / 4)
			  & (tdep->num_aregs / 4 - 1);
	  cache->wd.ws = ws & ~(1 << wb);

	  cache->pc = get_frame_func (this_frame);
	  cache->ra = (pc & 0xc0000000) | (ra & 0x3fffffff);
	  cache->ps = (ps & ~PS_CALLINC_MASK)
	    | ((WINSIZE(ra)/4) << PS_CALLINC_SHIFT);
	}

      if (cache->wd.ws == 0)
	{
	  int i;

	  /* Set A0...A3.  */
	  sp = get_frame_register_unsigned
	    (this_frame, tdep->a0_base + 1) - 16;
	  
	  for (i = 0; i < 4; i++, sp += 4)
	    {
	      cache->wd.aregs[i] = sp;
	    }

	  if (cache->wd.callsize > 4)
	    {
	      /* Set A4...A7/A11.  */
	      /* Get the SP of the frame previous to the previous one.
		 To achieve this, we have to dereference SP twice.  */
	      sp = (CORE_ADDR) read_memory_integer (sp - 12, 4, byte_order);
	      sp = (CORE_ADDR) read_memory_integer (sp - 12, 4, byte_order);
	      sp -= cache->wd.callsize * 4;

	      for ( i = 4; i < cache->wd.callsize; i++, sp += 4)
		{
		  cache->wd.aregs[i] = sp;
		}
	    }
	}

      if ((cache->prev_sp == 0) && ( ra != 0 ))
	/* If RA is equal to 0 this frame is an outermost frame.  Leave
	   cache->prev_sp unchanged marking the boundary of the frame stack.  */
	{
	  if ((cache->wd.ws & (1 << cache->wd.wb)) == 0)
	    {
	      /* Register window overflow already happened.
		 We can read caller's SP from the proper spill location.  */
	      sp = get_frame_register_unsigned
		(this_frame, tdep->a0_base + 1);
	      cache->prev_sp = read_memory_integer (sp - 12, 4, byte_order);
	    }
	  else
	    {
	      /* Read caller's frame SP directly from the previous window.  */
	      int regnum = arreg_number
			     (gdbarch, tdep->a0_base + 1,
			      cache->wd.wb);

	      cache->prev_sp = xtensa_read_register (regnum);
	    }
	}
    }
  else if (xtensa_window_interrupt_insn (gdbarch, pc))
    {
      /* Execution stopped inside Xtensa Window Interrupt Handler.  */

      xtensa_window_interrupt_frame_cache (this_frame, cache, pc);
      /* Everything was set already,  including cache->base.  */
      return cache;
    }
  else	/* Call0 framework.  */
    {
      call0_frame_cache (this_frame, cache, pc);  
      fp_regnum = cache->c0.fp_regnum;
    }

  cache->base = get_frame_register_unsigned (this_frame, fp_regnum);

  return cache;
}

static int xtensa_session_once_reported = 1;

/* Report a problem with prologue analysis while doing backtracing.
   But, do it only once to avoid annoying repeated messages.  */

static void
warning_once (void)
{
  if (xtensa_session_once_reported == 0)
    warning (_("\
\nUnrecognised function prologue. Stack trace cannot be resolved. \
This message will not be repeated in this session.\n"));

  xtensa_session_once_reported = 1;
}


static void
xtensa_frame_this_id (frame_info_ptr this_frame,
		      void **this_cache,
		      struct frame_id *this_id)
{
  struct xtensa_frame_cache *cache =
    xtensa_frame_cache (this_frame, this_cache);

  if (cache->prev_sp == 0)
    return;

  (*this_id) = frame_id_build (cache->prev_sp, cache->pc);
}

static struct value *
xtensa_frame_prev_register (frame_info_ptr this_frame,
			    void **this_cache,
			    int regnum)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct xtensa_frame_cache *cache;
  ULONGEST saved_reg = 0;
  int done = 1;
  xtensa_gdbarch_tdep *tdep = gdbarch_tdep<xtensa_gdbarch_tdep> (gdbarch);

  if (*this_cache == NULL)
    *this_cache = xtensa_frame_cache (this_frame, this_cache);
  cache = (struct xtensa_frame_cache *) *this_cache;

  if (regnum ==gdbarch_pc_regnum (gdbarch))
    saved_reg = cache->ra;
  else if (regnum == tdep->a0_base + 1)
    saved_reg = cache->prev_sp;
  else if (!cache->call0)
    {
      if (regnum == tdep->ws_regnum)
	saved_reg = cache->wd.ws;
      else if (regnum == tdep->wb_regnum)
	saved_reg = cache->wd.wb;
      else if (regnum == gdbarch_ps_regnum (gdbarch))
	saved_reg = cache->ps;
      else
	done = 0;
    }
  else
    done = 0;

  if (done)
    return frame_unwind_got_constant (this_frame, regnum, saved_reg);

  if (!cache->call0) /* Windowed ABI.  */
    {
      /* Convert A-register numbers to AR-register numbers,
	 if we deal with A-register.  */
      if (regnum >= tdep->a0_base
	  && regnum <= tdep->a0_base + 15)
	regnum = arreg_number (gdbarch, regnum, cache->wd.wb);

      /* Check, if we deal with AR-register saved on stack.  */
      if (regnum >= tdep->ar_base
	  && regnum <= (tdep->ar_base
			 + tdep->num_aregs))
	{
	  int areg = areg_number (gdbarch, regnum, cache->wd.wb);

	  if (areg >= 0
	      && areg < XTENSA_NUM_SAVED_AREGS
	      && cache->wd.aregs[areg] != -1)
	    return frame_unwind_got_memory (this_frame, regnum,
					    cache->wd.aregs[areg]);
	}
    }
  else /* Call0 ABI.  */
    {
      int reg = (regnum >= tdep->ar_base
		&& regnum <= (tdep->ar_base
			       + C0_NREGS))
		  ? regnum - tdep->ar_base : regnum;

      if (reg < C0_NREGS)
	{
	  CORE_ADDR spe;
	  int stkofs;

	  /* If register was saved in the prologue, retrieve it.  */
	  stkofs = cache->c0.c0_rt[reg].to_stk;
	  if (stkofs != C0_NOSTK)
	    {
	      /* Determine SP on entry based on FP.  */
	      spe = cache->c0.c0_fp
		- cache->c0.c0_rt[cache->c0.fp_regnum].fr_ofs;

	      return frame_unwind_got_memory (this_frame, regnum,
					      spe + stkofs);
	    }
	}
    }

  /* All other registers have been either saved to
     the stack or are still alive in the processor.  */

  return frame_unwind_got_register (this_frame, regnum, regnum);
}


static const struct frame_unwind
xtensa_unwind =
{
  "xtensa prologue",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  xtensa_frame_this_id,
  xtensa_frame_prev_register,
  NULL,
  default_frame_sniffer
};

static CORE_ADDR
xtensa_frame_base_address (frame_info_ptr this_frame, void **this_cache)
{
  struct xtensa_frame_cache *cache =
    xtensa_frame_cache (this_frame, this_cache);

  return cache->base;
}

static const struct frame_base
xtensa_frame_base =
{
  &xtensa_unwind,
  xtensa_frame_base_address,
  xtensa_frame_base_address,
  xtensa_frame_base_address
};


static void
xtensa_extract_return_value (struct type *type,
			     struct regcache *regcache,
			     void *dst)
{
  struct gdbarch *gdbarch = regcache->arch ();
  bfd_byte *valbuf = (bfd_byte *) dst;
  int len = type->length ();
  ULONGEST pc, wb;
  int callsize, areg;
  int offset = 0;

  DEBUGTRACE ("xtensa_extract_return_value (...)\n");

  gdb_assert(len > 0);

  xtensa_gdbarch_tdep *tdep = gdbarch_tdep<xtensa_gdbarch_tdep> (gdbarch);
  if (tdep->call_abi != CallAbiCall0Only)
    {
      /* First, we have to find the caller window in the register file.  */
      regcache_raw_read_unsigned (regcache, gdbarch_pc_regnum (gdbarch), &pc);
      callsize = extract_call_winsize (gdbarch, pc);

      /* On Xtensa, we can return up to 4 words (or 2 for call12).  */
      if (len > (callsize > 8 ? 8 : 16))
	internal_error (_("cannot extract return value of %d bytes long"),
			len);

      /* Get the register offset of the return
	 register (A2) in the caller window.  */
      regcache_raw_read_unsigned
	(regcache, tdep->wb_regnum, &wb);
      areg = arreg_number (gdbarch,
			  tdep->a0_base + 2 + callsize, wb);
    }
  else
    {
      /* No windowing hardware - Call0 ABI.  */
      areg = tdep->a0_base + C0_ARGS;
    }

  DEBUGINFO ("[xtensa_extract_return_value] areg %d len %d\n", areg, len);

  if (len < 4 && gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
    offset = 4 - len;

  for (; len > 0; len -= 4, areg++, valbuf += 4)
    {
      if (len < 4)
	regcache->raw_read_part (areg, offset, len, valbuf);
      else
	regcache->raw_read (areg, valbuf);
    }
}


static void
xtensa_store_return_value (struct type *type,
			   struct regcache *regcache,
			   const void *dst)
{
  struct gdbarch *gdbarch = regcache->arch ();
  const bfd_byte *valbuf = (const bfd_byte *) dst;
  unsigned int areg;
  ULONGEST pc, wb;
  int callsize;
  int len = type->length ();
  int offset = 0;

  DEBUGTRACE ("xtensa_store_return_value (...)\n");

  xtensa_gdbarch_tdep *tdep = gdbarch_tdep<xtensa_gdbarch_tdep> (gdbarch);
  if (tdep->call_abi != CallAbiCall0Only)
    {
      regcache_raw_read_unsigned 
	(regcache, tdep->wb_regnum, &wb);
      regcache_raw_read_unsigned (regcache, gdbarch_pc_regnum (gdbarch), &pc);
      callsize = extract_call_winsize (gdbarch, pc);

      if (len > (callsize > 8 ? 8 : 16))
	internal_error (_("unimplemented for this length: %s"),
			pulongest (type->length ()));
      areg = arreg_number (gdbarch,
			   tdep->a0_base + 2 + callsize, wb);

      DEBUGTRACE ("[xtensa_store_return_value] callsize %d wb %d\n",
	      callsize, (int) wb);
    }
  else
    {
      areg = tdep->a0_base + C0_ARGS;
    }

  if (len < 4 && gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
    offset = 4 - len;

  for (; len > 0; len -= 4, areg++, valbuf += 4)
    {
      if (len < 4)
	regcache->raw_write_part (areg, offset, len, valbuf);
      else
	regcache->raw_write (areg, valbuf);
    }
}


static enum return_value_convention
xtensa_return_value (struct gdbarch *gdbarch,
		     struct value *function,
		     struct type *valtype,
		     struct regcache *regcache,
		     gdb_byte *readbuf,
		     const gdb_byte *writebuf)
{
  /* Structures up to 16 bytes are returned in registers.  */

  int struct_return = ((valtype->code () == TYPE_CODE_STRUCT
			|| valtype->code () == TYPE_CODE_UNION
			|| valtype->code () == TYPE_CODE_ARRAY)
		       && valtype->length () > 16);

  if (struct_return)
    return RETURN_VALUE_STRUCT_CONVENTION;

  DEBUGTRACE ("xtensa_return_value(...)\n");

  if (writebuf != NULL)
    {
      xtensa_store_return_value (valtype, regcache, writebuf);
    }

  if (readbuf != NULL)
    {
      gdb_assert (!struct_return);
      xtensa_extract_return_value (valtype, regcache, readbuf);
    }
  return RETURN_VALUE_REGISTER_CONVENTION;
}


/* DUMMY FRAME */

static CORE_ADDR
xtensa_push_dummy_call (struct gdbarch *gdbarch,
			struct value *function,
			struct regcache *regcache,
			CORE_ADDR bp_addr,
			int nargs,
			struct value **args,
			CORE_ADDR sp,
			function_call_return_method return_method,
			CORE_ADDR struct_addr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  xtensa_gdbarch_tdep *tdep = gdbarch_tdep<xtensa_gdbarch_tdep> (gdbarch);
  int size, onstack_size;
  gdb_byte *buf = (gdb_byte *) alloca (16);
  CORE_ADDR ra, ps;
  struct argument_info
  {
    const bfd_byte *contents;
    int length;
    int onstack;		/* onstack == 0 => in reg */
    int align;			/* alignment */
    union
    {
      int offset;		/* stack offset if on stack.  */
      int regno;		/* regno if in register.  */
    } u;
  };

  struct argument_info *arg_info =
    (struct argument_info *) alloca (nargs * sizeof (struct argument_info));

  CORE_ADDR osp = sp;

  DEBUGTRACE ("xtensa_push_dummy_call (...)\n");

  if (xtensa_debug_level > 3)
    {
      DEBUGINFO ("[xtensa_push_dummy_call] nargs = %d\n", nargs);
      DEBUGINFO ("[xtensa_push_dummy_call] sp=0x%x, return_method=%d, "
		 "struct_addr=0x%x\n",
		 (int) sp, (int) return_method, (int) struct_addr);

      for (int i = 0; i < nargs; i++)
	{
	  struct value *arg = args[i];
	  struct type *arg_type = check_typedef (arg->type ());
	  gdb_printf (gdb_stdlog, "%2d: %s %3s ", i,
		      host_address_to_string (arg),
		      pulongest (arg_type->length ()));
	  switch (arg_type->code ())
	    {
	    case TYPE_CODE_INT:
	      gdb_printf (gdb_stdlog, "int");
	      break;
	    case TYPE_CODE_STRUCT:
	      gdb_printf (gdb_stdlog, "struct");
	      break;
	    default:
	      gdb_printf (gdb_stdlog, "%3d", arg_type->code ());
	      break;
	    }
	  gdb_printf (gdb_stdlog, " %s\n",
		      host_address_to_string (arg->contents ().data ()));
	}
    }

  /* First loop: collect information.
     Cast into type_long.  (This shouldn't happen often for C because
     GDB already does this earlier.)  It's possible that GDB could
     do it all the time but it's harmless to leave this code here.  */

  size = 0;
  onstack_size = 0;

  if (return_method == return_method_struct)
    size = REGISTER_SIZE;

  for (int i = 0; i < nargs; i++)
    {
      struct argument_info *info = &arg_info[i];
      struct value *arg = args[i];
      struct type *arg_type = check_typedef (arg->type ());

      switch (arg_type->code ())
	{
	case TYPE_CODE_INT:
	case TYPE_CODE_BOOL:
	case TYPE_CODE_CHAR:
	case TYPE_CODE_RANGE:
	case TYPE_CODE_ENUM:

	  /* Cast argument to long if necessary as the mask does it too.  */
	  if (arg_type->length ()
	      < builtin_type (gdbarch)->builtin_long->length ())
	    {
	      arg_type = builtin_type (gdbarch)->builtin_long;
	      arg = value_cast (arg_type, arg);
	    }
	  /* Aligment is equal to the type length for the basic types.  */
	  info->align = arg_type->length ();
	  break;

	case TYPE_CODE_FLT:

	  /* Align doubles correctly.  */
	  if (arg_type->length ()
	      == builtin_type (gdbarch)->builtin_double->length ())
	    info->align = builtin_type (gdbarch)->builtin_double->length ();
	  else
	    info->align = builtin_type (gdbarch)->builtin_long->length ();
	  break;

	case TYPE_CODE_STRUCT:
	default:
	  info->align = builtin_type (gdbarch)->builtin_long->length ();
	  break;
	}
      info->length = arg_type->length ();
      info->contents = arg->contents ().data ();

      /* Align size and onstack_size.  */
      size = (size + info->align - 1) & ~(info->align - 1);
      onstack_size = (onstack_size + info->align - 1) & ~(info->align - 1);

      if (size + info->length > REGISTER_SIZE * ARG_NOF (tdep))
	{
	  info->onstack = 1;
	  info->u.offset = onstack_size;
	  onstack_size += info->length;
	}
      else
	{
	  info->onstack = 0;
	  info->u.regno = ARG_1ST (tdep) + size / REGISTER_SIZE;
	}
      size += info->length;
    }

  /* Adjust the stack pointer and align it.  */
  sp = align_down (sp - onstack_size, SP_ALIGNMENT);

  /* Simulate MOVSP, if Windowed ABI.  */
  if ((tdep->call_abi != CallAbiCall0Only)
      && (sp != osp))
    {
      read_memory (osp - 16, buf, 16);
      write_memory (sp - 16, buf, 16);
    }

  /* Second Loop: Load arguments.  */

  if (return_method == return_method_struct)
    {
      store_unsigned_integer (buf, REGISTER_SIZE, byte_order, struct_addr);
      regcache->cooked_write (ARG_1ST (tdep), buf);
    }

  for (int i = 0; i < nargs; i++)
    {
      struct argument_info *info = &arg_info[i];

      if (info->onstack)
	{
	  int n = info->length;
	  CORE_ADDR offset = sp + info->u.offset;

	  /* Odd-sized structs are aligned to the lower side of a memory
	     word in big-endian mode and require a shift.  This only
	     applies for structures smaller than one word.  */

	  if (n < REGISTER_SIZE
	      && gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
	    offset += (REGISTER_SIZE - n);

	  write_memory (offset, info->contents, info->length);

	}
      else
	{
	  int n = info->length;
	  const bfd_byte *cp = info->contents;
	  int r = info->u.regno;

	  /* Odd-sized structs are aligned to the lower side of registers in
	     big-endian mode and require a shift.  The odd-sized leftover will
	     be at the end.  Note that this is only true for structures smaller
	     than REGISTER_SIZE; for larger odd-sized structures the excess
	     will be left-aligned in the register on both endiannesses.  */

	  if (n < REGISTER_SIZE && byte_order == BFD_ENDIAN_BIG)
	    {
	      ULONGEST v;
	      v = extract_unsigned_integer (cp, REGISTER_SIZE, byte_order);
	      v = v >> ((REGISTER_SIZE - n) * TARGET_CHAR_BIT);

	      store_unsigned_integer (buf, REGISTER_SIZE, byte_order, v);
	      regcache->cooked_write (r, buf);

	      cp += REGISTER_SIZE;
	      n -= REGISTER_SIZE;
	      r++;
	    }
	  else
	    while (n > 0)
	      {
		regcache->cooked_write (r, cp);

		cp += REGISTER_SIZE;
		n -= REGISTER_SIZE;
		r++;
	      }
	}
    }

  /* Set the return address of dummy frame to the dummy address.
     The return address for the current function (in A0) is
     saved in the dummy frame, so we can safely overwrite A0 here.  */

  if (tdep->call_abi != CallAbiCall0Only)
    {
      ULONGEST val;

      ra = (bp_addr & 0x3fffffff) | 0x40000000;
      regcache_raw_read_unsigned (regcache, gdbarch_ps_regnum (gdbarch), &val);
      ps = (unsigned long) val & ~0x00030000;
      regcache_cooked_write_unsigned
	(regcache, tdep->a0_base + 4, ra);
      regcache_cooked_write_unsigned (regcache,
				      gdbarch_ps_regnum (gdbarch),
				      ps | 0x00010000);

      /* All the registers have been saved.  After executing
	 dummy call, they all will be restored.  So it's safe
	 to modify WINDOWSTART register to make it look like there
	 is only one register window corresponding to WINDOWEBASE.  */

      regcache->raw_read (tdep->wb_regnum, buf);
      regcache_cooked_write_unsigned
	(regcache, tdep->ws_regnum,
	 1 << extract_unsigned_integer (buf, 4, byte_order));
    }
  else
    {
      /* Simulate CALL0: write RA into A0 register.  */
      regcache_cooked_write_unsigned
	(regcache, tdep->a0_base, bp_addr);
    }

  /* Set new stack pointer and return it.  */
  regcache_cooked_write_unsigned (regcache,
				  tdep->a0_base + 1, sp);
  /* Make dummy frame ID unique by adding a constant.  */
  return sp + SP_ALIGNMENT;
}

/* Implement the breakpoint_kind_from_pc gdbarch method.  */

static int
xtensa_breakpoint_kind_from_pc (struct gdbarch *gdbarch, CORE_ADDR *pcptr)
{
  xtensa_gdbarch_tdep *tdep = gdbarch_tdep<xtensa_gdbarch_tdep> (gdbarch);

  if (tdep->isa_use_density_instructions)
    return 2;
  else
    return 4;
}

/* Return a breakpoint for the current location of PC.  We always use
   the density version if we have density instructions (regardless of the
   current instruction at PC), and use regular instructions otherwise.  */

#define BIG_BREAKPOINT { 0x00, 0x04, 0x00 }
#define LITTLE_BREAKPOINT { 0x00, 0x40, 0x00 }
#define DENSITY_BIG_BREAKPOINT { 0xd2, 0x0f }
#define DENSITY_LITTLE_BREAKPOINT { 0x2d, 0xf0 }

/* Implement the sw_breakpoint_from_kind gdbarch method.  */

static const gdb_byte *
xtensa_sw_breakpoint_from_kind (struct gdbarch *gdbarch, int kind, int *size)
{
  *size = kind;

  if (kind == 4)
    {
      static unsigned char big_breakpoint[] = BIG_BREAKPOINT;
      static unsigned char little_breakpoint[] = LITTLE_BREAKPOINT;

      if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
	return big_breakpoint;
      else
	return little_breakpoint;
    }
  else
    {
      static unsigned char density_big_breakpoint[] = DENSITY_BIG_BREAKPOINT;
      static unsigned char density_little_breakpoint[]
	= DENSITY_LITTLE_BREAKPOINT;

      if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
	return density_big_breakpoint;
      else
	return density_little_breakpoint;
    }
}

/* Call0 ABI support routines.  */

/* Return true, if PC points to "ret" or "ret.n".  */ 

static int
call0_ret (CORE_ADDR start_pc, CORE_ADDR finish_pc)
{
#define RETURN_RET goto done
  xtensa_isa isa;
  xtensa_insnbuf ins, slot;
  gdb_byte ibuf[XTENSA_ISA_BSZ];
  CORE_ADDR ia, bt, ba;
  xtensa_format ifmt;
  int ilen, islots, is;
  xtensa_opcode opc;
  const char *opcname;
  int found_ret = 0;

  isa = xtensa_default_isa;
  gdb_assert (XTENSA_ISA_BSZ >= xtensa_isa_maxlength (isa));
  ins = xtensa_insnbuf_alloc (isa);
  slot = xtensa_insnbuf_alloc (isa);
  ba = 0;

  for (ia = start_pc, bt = ia; ia < finish_pc ; ia += ilen)
    {
      if (ia + xtensa_isa_maxlength (isa) > bt)
	{
	  ba = ia;
	  bt = (ba + XTENSA_ISA_BSZ) < finish_pc
	    ? ba + XTENSA_ISA_BSZ : finish_pc;
	  if (target_read_memory (ba, ibuf, bt - ba) != 0 )
	    RETURN_RET;
	}

      xtensa_insnbuf_from_chars (isa, ins, &ibuf[ia-ba], 0);
      ifmt = xtensa_format_decode (isa, ins);
      if (ifmt == XTENSA_UNDEFINED)
	RETURN_RET;
      ilen = xtensa_format_length (isa, ifmt);
      if (ilen == XTENSA_UNDEFINED)
	RETURN_RET;
      islots = xtensa_format_num_slots (isa, ifmt);
      if (islots == XTENSA_UNDEFINED)
	RETURN_RET;
      
      for (is = 0; is < islots; ++is)
	{
	  if (xtensa_format_get_slot (isa, ifmt, is, ins, slot))
	    RETURN_RET;
	  
	  opc = xtensa_opcode_decode (isa, ifmt, is, slot);
	  if (opc == XTENSA_UNDEFINED) 
	    RETURN_RET;
	  
	  opcname = xtensa_opcode_name (isa, opc);
	  
	  if ((strcasecmp (opcname, "ret.n") == 0)
	      || (strcasecmp (opcname, "ret") == 0))
	    {
	      found_ret = 1;
	      RETURN_RET;
	    }
	}
    }
 done:
  xtensa_insnbuf_free(isa, slot);
  xtensa_insnbuf_free(isa, ins);
  return found_ret;
}

/* Call0 opcode class.  Opcodes are preclassified according to what they
   mean for Call0 prologue analysis, and their number of significant operands.
   The purpose of this is to simplify prologue analysis by separating 
   instruction decoding (libisa) from the semantics of prologue analysis.  */

enum xtensa_insn_kind
{
  c0opc_illegal,       /* Unknown to libisa (invalid) or 'ill' opcode.  */
  c0opc_uninteresting, /* Not interesting for Call0 prologue analysis.  */
  c0opc_flow,	       /* Flow control insn.  */
  c0opc_entry,	       /* ENTRY indicates non-Call0 prologue.  */
  c0opc_break,	       /* Debugger software breakpoints.  */
  c0opc_add,	       /* Adding two registers.  */
  c0opc_addi,	       /* Adding a register and an immediate.  */
  c0opc_and,	       /* Bitwise "and"-ing two registers.  */
  c0opc_sub,	       /* Subtracting a register from a register.  */
  c0opc_mov,	       /* Moving a register to a register.  */
  c0opc_movi,	       /* Moving an immediate to a register.  */
  c0opc_l32r,	       /* Loading a literal.  */
  c0opc_s32i,	       /* Storing word at fixed offset from a base register.  */
  c0opc_rwxsr,	       /* RSR, WRS, or XSR instructions.  */
  c0opc_l32e,          /* L32E instruction.  */
  c0opc_s32e,          /* S32E instruction.  */
  c0opc_rfwo,          /* RFWO instruction.  */
  c0opc_rfwu,          /* RFWU instruction.  */
  c0opc_NrOf	       /* Number of opcode classifications.  */
};

/* Return true,  if OPCNAME is RSR,  WRS,  or XSR instruction.  */

static int
rwx_special_register (const char *opcname)
{
  char ch = *opcname++;
  
  if ((ch != 'r') && (ch != 'w') && (ch != 'x'))
    return 0;
  if (*opcname++ != 's')
    return 0;
  if (*opcname++ != 'r')
    return 0;
  if (*opcname++ != '.')
    return 0;

  return 1;
}

/* Classify an opcode based on what it means for Call0 prologue analysis.  */

static xtensa_insn_kind
call0_classify_opcode (xtensa_isa isa, xtensa_opcode opc)
{
  const char *opcname;
  xtensa_insn_kind opclass = c0opc_uninteresting;

  DEBUGTRACE ("call0_classify_opcode (..., opc = %d)\n", opc);

  /* Get opcode name and handle special classifications.  */

  opcname = xtensa_opcode_name (isa, opc);

  if (opcname == NULL 
      || strcasecmp (opcname, "ill") == 0
      || strcasecmp (opcname, "ill.n") == 0)
    opclass = c0opc_illegal;
  else if (strcasecmp (opcname, "break") == 0
	   || strcasecmp (opcname, "break.n") == 0)
     opclass = c0opc_break;
  else if (strcasecmp (opcname, "entry") == 0)
    opclass = c0opc_entry;
  else if (strcasecmp (opcname, "rfwo") == 0)
    opclass = c0opc_rfwo;
  else if (strcasecmp (opcname, "rfwu") == 0)
    opclass = c0opc_rfwu;
  else if (xtensa_opcode_is_branch (isa, opc) > 0
	   || xtensa_opcode_is_jump   (isa, opc) > 0
	   || xtensa_opcode_is_loop   (isa, opc) > 0
	   || xtensa_opcode_is_call   (isa, opc) > 0
	   || strcasecmp (opcname, "simcall") == 0
	   || strcasecmp (opcname, "syscall") == 0)
    opclass = c0opc_flow;

  /* Also, classify specific opcodes that need to be tracked.  */
  else if (strcasecmp (opcname, "add") == 0 
	   || strcasecmp (opcname, "add.n") == 0)
    opclass = c0opc_add;
  else if (strcasecmp (opcname, "and") == 0)
    opclass = c0opc_and;
  else if (strcasecmp (opcname, "addi") == 0 
	   || strcasecmp (opcname, "addi.n") == 0
	   || strcasecmp (opcname, "addmi") == 0)
    opclass = c0opc_addi;
  else if (strcasecmp (opcname, "sub") == 0)
    opclass = c0opc_sub;
  else if (strcasecmp (opcname, "mov.n") == 0
	   || strcasecmp (opcname, "or") == 0) /* Could be 'mov' asm macro.  */
    opclass = c0opc_mov;
  else if (strcasecmp (opcname, "movi") == 0 
	   || strcasecmp (opcname, "movi.n") == 0)
    opclass = c0opc_movi;
  else if (strcasecmp (opcname, "l32r") == 0)
    opclass = c0opc_l32r;
  else if (strcasecmp (opcname, "s32i") == 0 
	   || strcasecmp (opcname, "s32i.n") == 0)
    opclass = c0opc_s32i;
  else if (strcasecmp (opcname, "l32e") == 0)
    opclass = c0opc_l32e;
  else if (strcasecmp (opcname, "s32e") == 0)
    opclass = c0opc_s32e;
  else if (rwx_special_register (opcname))
    opclass = c0opc_rwxsr;

  return opclass;
}

/* Tracks register movement/mutation for a given operation, which may
   be within a bundle.  Updates the destination register tracking info
   accordingly.  The pc is needed only for pc-relative load instructions
   (eg. l32r).  The SP register number is needed to identify stores to
   the stack frame.  Returns 0, if analysis was successful, non-zero
   otherwise.  */

static int
call0_track_op (struct gdbarch *gdbarch, xtensa_c0reg_t dst[], xtensa_c0reg_t src[],
		xtensa_insn_kind opclass, int nods, unsigned odv[],
		CORE_ADDR pc, int spreg, xtensa_frame_cache_t *cache)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  unsigned litbase, litaddr, litval;
  xtensa_gdbarch_tdep *tdep = gdbarch_tdep<xtensa_gdbarch_tdep> (gdbarch);

  switch (opclass)
    {
    case c0opc_addi:
      /* 3 operands: dst, src, imm.  */
      gdb_assert (nods == 3);
      dst[odv[0]].fr_reg = src[odv[1]].fr_reg;
      dst[odv[0]].fr_ofs = src[odv[1]].fr_ofs + odv[2];
      break;
    case c0opc_add:
      /* 3 operands: dst, src1, src2.  */
      gdb_assert (nods == 3); 
      if      (src[odv[1]].fr_reg == C0_CONST)
	{
	  dst[odv[0]].fr_reg = src[odv[2]].fr_reg;
	  dst[odv[0]].fr_ofs = src[odv[2]].fr_ofs + src[odv[1]].fr_ofs;
	}
      else if (src[odv[2]].fr_reg == C0_CONST)
	{
	  dst[odv[0]].fr_reg = src[odv[1]].fr_reg;
	  dst[odv[0]].fr_ofs = src[odv[1]].fr_ofs + src[odv[2]].fr_ofs;
	}
      else dst[odv[0]].fr_reg = C0_INEXP;
      break;
    case c0opc_and:
      /* 3 operands:  dst, src1, src2.  */
      gdb_assert (nods == 3);
      if (cache->c0.c0_fpalign == 0)
	{
	  /* Handle dynamic stack alignment.  */
	  if ((src[odv[0]].fr_reg == spreg) && (src[odv[1]].fr_reg == spreg))
	    {
	      if (src[odv[2]].fr_reg == C0_CONST)
		cache->c0.c0_fpalign = src[odv[2]].fr_ofs;
	      break;
	    }
	  else if ((src[odv[0]].fr_reg == spreg)
		   && (src[odv[2]].fr_reg == spreg))
	    {
	      if (src[odv[1]].fr_reg == C0_CONST)
		cache->c0.c0_fpalign = src[odv[1]].fr_ofs;
	      break;
	    }
	  /* else fall through.  */
	}
      if      (src[odv[1]].fr_reg == C0_CONST)
	{
	  dst[odv[0]].fr_reg = src[odv[2]].fr_reg;
	  dst[odv[0]].fr_ofs = src[odv[2]].fr_ofs & src[odv[1]].fr_ofs;
	}
      else if (src[odv[2]].fr_reg == C0_CONST)
	{
	  dst[odv[0]].fr_reg = src[odv[1]].fr_reg;
	  dst[odv[0]].fr_ofs = src[odv[1]].fr_ofs & src[odv[2]].fr_ofs;
	}
      else dst[odv[0]].fr_reg = C0_INEXP;
      break;
    case c0opc_sub:
      /* 3 operands: dst, src1, src2.  */
      gdb_assert (nods == 3);
      if      (src[odv[2]].fr_reg == C0_CONST)
	{
	  dst[odv[0]].fr_reg = src[odv[1]].fr_reg;
	  dst[odv[0]].fr_ofs = src[odv[1]].fr_ofs - src[odv[2]].fr_ofs;
	}
      else dst[odv[0]].fr_reg = C0_INEXP;
      break;
    case c0opc_mov:
      /* 2 operands: dst, src [, src].  */
      gdb_assert (nods == 2);
      /* First, check if it's a special case of saving unaligned SP
	 to a spare register in case of dynamic stack adjustment.
	 But, only do it one time.  The second time could be initializing
	 frame pointer.  We don't want to overwrite the first one.  */
      if ((odv[1] == spreg) && (cache->c0.c0_old_sp == C0_INEXP))
	cache->c0.c0_old_sp = odv[0];

      dst[odv[0]].fr_reg = src[odv[1]].fr_reg;
      dst[odv[0]].fr_ofs = src[odv[1]].fr_ofs;
      break;
    case c0opc_movi:
      /* 2 operands: dst, imm.  */
      gdb_assert (nods == 2);
      dst[odv[0]].fr_reg = C0_CONST;
      dst[odv[0]].fr_ofs = odv[1];
      break;
    case c0opc_l32r:
      /* 2 operands: dst, literal offset.  */
      gdb_assert (nods == 2);
      /* litbase = xtensa_get_litbase (pc);  can be also used.  */
      litbase = (tdep->litbase_regnum == -1)
	? 0 : xtensa_read_register
		(tdep->litbase_regnum);
      litaddr = litbase & 1
		  ? (litbase & ~1) + (signed)odv[1]
		  : (pc + 3  + (signed)odv[1]) & ~3;
      litval = read_memory_integer (litaddr, 4, byte_order);
      dst[odv[0]].fr_reg = C0_CONST;
      dst[odv[0]].fr_ofs = litval;
      break;
    case c0opc_s32i:
      /* 3 operands: value, base, offset.  */
      gdb_assert (nods == 3 && spreg >= 0 && spreg < C0_NREGS);
      /* First, check if it's a spill for saved unaligned SP,
	 when dynamic stack adjustment was applied to this frame.  */
      if ((cache->c0.c0_fpalign != 0)		/* Dynamic stack adjustment.  */
	  && (odv[1] == spreg)			/* SP usage indicates spill.  */
	  && (odv[0] == cache->c0.c0_old_sp))	/* Old SP register spilled.  */
	cache->c0.c0_sp_ofs = odv[2];

      if (src[odv[1]].fr_reg == spreg	     /* Store to stack frame.  */
	  && (src[odv[1]].fr_ofs & 3) == 0   /* Alignment preserved.  */
	  &&  src[odv[0]].fr_reg >= 0	     /* Value is from a register.  */
	  &&  src[odv[0]].fr_ofs == 0	     /* Value hasn't been modified.  */
	  &&  src[src[odv[0]].fr_reg].to_stk == C0_NOSTK) /* First time.  */
	{
	  /* ISA encoding guarantees alignment.  But, check it anyway.  */
	  gdb_assert ((odv[2] & 3) == 0);
	  dst[src[odv[0]].fr_reg].to_stk = src[odv[1]].fr_ofs + odv[2];
	}
      break;
      /* If we end up inside Window Overflow / Underflow interrupt handler
	 report an error because these handlers should have been handled
	 already in a different way.  */
    case c0opc_l32e:
    case c0opc_s32e:
    case c0opc_rfwo:
    case c0opc_rfwu:
      return 1;
    default:
      return 1;
    }
  return 0;
}

/* Analyze prologue of the function at start address to determine if it uses
   the Call0 ABI, and if so track register moves and linear modifications
   in the prologue up to the PC or just beyond the prologue, whichever is
   first. An 'entry' instruction indicates non-Call0 ABI and the end of the
   prologue. The prologue may overlap non-prologue instructions but is
   guaranteed to end by the first flow-control instruction (jump, branch,
   call or return).  Since an optimized function may move information around
   and change the stack frame arbitrarily during the prologue, the information
   is guaranteed valid only at the point in the function indicated by the PC.
   May be used to skip the prologue or identify the ABI, w/o tracking.

   Returns:   Address of first instruction after prologue, or PC (whichever 
	      is first), or 0, if decoding failed (in libisa).
   Input args:
      start   Start address of function/prologue.
      pc      Program counter to stop at.  Use 0 to continue to end of prologue.
	      If 0, avoids infinite run-on in corrupt code memory by bounding
	      the scan to the end of the function if that can be determined.
      nregs   Number of general registers to track.
   InOut args:
      cache   Xtensa frame cache.

      Note that these may produce useful results even if decoding fails
      because they begin with default assumptions that analysis may change.  */

static CORE_ADDR
call0_analyze_prologue (struct gdbarch *gdbarch,
			CORE_ADDR start, CORE_ADDR pc,
			int nregs, xtensa_frame_cache_t *cache)
{
  CORE_ADDR ia;		    /* Current insn address in prologue.  */
  CORE_ADDR ba = 0;	    /* Current address at base of insn buffer.  */
  CORE_ADDR bt;		    /* Current address at top+1 of insn buffer.  */
  gdb_byte ibuf[XTENSA_ISA_BSZ];/* Instruction buffer for decoding prologue.  */
  xtensa_isa isa;	    /* libisa ISA handle.  */
  xtensa_insnbuf ins, slot; /* libisa handle to decoded insn, slot.  */
  xtensa_format ifmt;	    /* libisa instruction format.  */
  int ilen, islots, is;	    /* Instruction length, nbr slots, current slot.  */
  xtensa_opcode opc;	    /* Opcode in current slot.  */
  xtensa_insn_kind opclass; /* Opcode class for Call0 prologue analysis.  */
  int nods;		    /* Opcode number of operands.  */
  unsigned odv[C0_MAXOPDS]; /* Operand values in order provided by libisa.  */
  xtensa_c0reg_t *rtmp;	    /* Register tracking info snapshot.  */
  int j;		    /* General loop counter.  */
  int fail = 0;		    /* Set non-zero and exit, if decoding fails.  */
  CORE_ADDR body_pc;	    /* The PC for the first non-prologue insn.  */
  CORE_ADDR end_pc;	    /* The PC for the lust function insn.  */

  struct symtab_and_line prologue_sal;

  DEBUGTRACE ("call0_analyze_prologue (start = 0x%08x, pc = 0x%08x, ...)\n", 
	      (int)start, (int)pc);

  /* Try to limit the scan to the end of the function if a non-zero pc
     arg was not supplied to avoid probing beyond the end of valid memory.
     If memory is full of garbage that classifies as c0opc_uninteresting.
     If this fails (eg. if no symbols) pc ends up 0 as it was.
     Initialize the Call0 frame and register tracking info.
     Assume it's Call0 until an 'entry' instruction is encountered.
     Assume we may be in the prologue until we hit a flow control instr.  */

  rtmp = NULL;
  body_pc = UINT_MAX;
  end_pc = 0;

  /* Find out, if we have an information about the prologue from DWARF.  */
  prologue_sal = find_pc_line (start, 0);
  if (prologue_sal.line != 0) /* Found debug info.  */
    body_pc = prologue_sal.end;

  /* If we are going to analyze the prologue in general without knowing about
     the current PC, make the best assumption for the end of the prologue.  */
  if (pc == 0)
    {
      find_pc_partial_function (start, 0, NULL, &end_pc);
      body_pc = std::min (end_pc, body_pc);
    }
  else
    body_pc = std::min (pc, body_pc);

  cache->call0 = 1;
  rtmp = (xtensa_c0reg_t*) alloca(nregs * sizeof(xtensa_c0reg_t));

  isa = xtensa_default_isa;
  gdb_assert (XTENSA_ISA_BSZ >= xtensa_isa_maxlength (isa));
  ins = xtensa_insnbuf_alloc (isa);
  slot = xtensa_insnbuf_alloc (isa);

  for (ia = start, bt = ia; ia < body_pc ; ia += ilen)
    {
      /* (Re)fill instruction buffer from memory if necessary, but do not
	 read memory beyond PC to be sure we stay within text section
	 (this protection only works if a non-zero pc is supplied).  */

      if (ia + xtensa_isa_maxlength (isa) > bt)
	{
	  ba = ia;
	  bt = (ba + XTENSA_ISA_BSZ) < body_pc ? ba + XTENSA_ISA_BSZ : body_pc;
	  if (target_read_memory (ba, ibuf, bt - ba) != 0 )
	    error (_("Unable to read target memory ..."));
	}

      /* Decode format information.  */

      xtensa_insnbuf_from_chars (isa, ins, &ibuf[ia-ba], 0);
      ifmt = xtensa_format_decode (isa, ins);
      if (ifmt == XTENSA_UNDEFINED)
	{
	  fail = 1;
	  goto done;
	}
      ilen = xtensa_format_length (isa, ifmt);
      if (ilen == XTENSA_UNDEFINED)
	{
	  fail = 1;
	  goto done;
	}
      islots = xtensa_format_num_slots (isa, ifmt);
      if (islots == XTENSA_UNDEFINED)
	{
	  fail = 1;
	  goto done;
	}

      /* Analyze a bundle or a single instruction, using a snapshot of 
	 the register tracking info as input for the entire bundle so that
	 register changes do not take effect within this bundle.  */

      for (j = 0; j < nregs; ++j)
	rtmp[j] = cache->c0.c0_rt[j];

      for (is = 0; is < islots; ++is)
	{
	  /* Decode a slot and classify the opcode.  */

	  fail = xtensa_format_get_slot (isa, ifmt, is, ins, slot);
	  if (fail)
	    goto done;

	  opc = xtensa_opcode_decode (isa, ifmt, is, slot);
	  DEBUGVERB ("[call0_analyze_prologue] instr addr = 0x%08x, opc = %d\n", 
		     (unsigned)ia, opc);
	  if (opc == XTENSA_UNDEFINED) 
	    opclass = c0opc_illegal;
	  else
	    opclass = call0_classify_opcode (isa, opc);

	  /* Decide whether to track this opcode, ignore it, or bail out.  */

	  switch (opclass)
	    {
	    case c0opc_illegal:
	    case c0opc_break:
	      fail = 1;
	      goto done;

	    case c0opc_uninteresting:
	      continue;

	    case c0opc_flow:  /* Flow control instructions stop analysis.  */
	    case c0opc_rwxsr: /* RSR, WSR, XSR instructions stop analysis.  */
	      goto done;

	    case c0opc_entry:
	      cache->call0 = 0;
	      ia += ilen;	       	/* Skip over 'entry' insn.  */
	      goto done;

	    default:
	      cache->call0 = 1;
	    }

	  /* Only expected opcodes should get this far.  */

	  /* Extract and decode the operands.  */
	  nods = xtensa_opcode_num_operands (isa, opc);
	  if (nods == XTENSA_UNDEFINED)
	    {
	      fail = 1;
	      goto done;
	    }

	  for (j = 0; j < nods && j < C0_MAXOPDS; ++j)
	    {
	      fail = xtensa_operand_get_field (isa, opc, j, ifmt, 
					       is, slot, &odv[j]);
	      if (fail)
		goto done;

	      fail = xtensa_operand_decode (isa, opc, j, &odv[j]);
	      if (fail)
		goto done;
	    }

	  /* Check operands to verify use of 'mov' assembler macro.  */
	  if (opclass == c0opc_mov && nods == 3)
	    {
	      if (odv[2] == odv[1])
		{
		  nods = 2;
		  if ((odv[0] == 1) && (odv[1] != 1))
		    /* OR  A1, An, An  , where n != 1.
		       This means we are inside epilogue already.  */
		    goto done;
		}
	      else
		{
		  opclass = c0opc_uninteresting;
		  continue;
		}
	    }

	  /* Track register movement and modification for this operation.  */
	  fail = call0_track_op (gdbarch, cache->c0.c0_rt, rtmp,
				 opclass, nods, odv, ia, 1, cache);
	  if (fail)
	    goto done;
	}
    }
done:
  DEBUGVERB ("[call0_analyze_prologue] stopped at instr addr 0x%08x, %s\n",
	     (unsigned)ia, fail ? "failed" : "succeeded");
  xtensa_insnbuf_free(isa, slot);
  xtensa_insnbuf_free(isa, ins);
  return fail ? XTENSA_ISA_BADPC : ia;
}

/* Initialize frame cache for the current frame in CALL0 ABI.  */

static void
call0_frame_cache (frame_info_ptr this_frame,
		   xtensa_frame_cache_t *cache, CORE_ADDR pc)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR start_pc;		/* The beginning of the function.  */
  CORE_ADDR body_pc=UINT_MAX;	/* PC, where prologue analysis stopped.  */
  CORE_ADDR sp, fp, ra;
  int fp_regnum = C0_SP, c0_hasfp = 0, c0_frmsz = 0, prev_sp = 0, to_stk;
  xtensa_gdbarch_tdep *tdep = gdbarch_tdep<xtensa_gdbarch_tdep> (gdbarch);
 
  sp = get_frame_register_unsigned
    (this_frame, tdep->a0_base + 1);
  fp = sp; /* Assume FP == SP until proven otherwise.  */

  /* Find the beginning of the prologue of the function containing the PC
     and analyze it up to the PC or the end of the prologue.  */

  if (find_pc_partial_function (pc, NULL, &start_pc, NULL))
    {
      body_pc = call0_analyze_prologue (gdbarch, start_pc, pc, C0_NREGS, cache);

      if (body_pc == XTENSA_ISA_BADPC)
	{
	  warning_once ();
	  ra = 0;
	  goto finish_frame_analysis;
	}
    }
  
  /* Get the frame information and FP (if used) at the current PC.
     If PC is in the prologue, the prologue analysis is more reliable
     than DWARF info.  We don't not know for sure, if PC is in the prologue,
     but we do know no calls have yet taken place, so we can almost
     certainly rely on the prologue analysis.  */

  if (body_pc <= pc)
    {
      /* Prologue analysis was successful up to the PC.
	 It includes the cases when PC == START_PC.  */
      c0_hasfp = cache->c0.c0_rt[C0_FP].fr_reg == C0_SP;
      /* c0_hasfp == true means there is a frame pointer because
	 we analyzed the prologue and found that cache->c0.c0_rt[C0_FP]
	 was derived from SP.  Otherwise, it would be C0_FP.  */
      fp_regnum = c0_hasfp ? C0_FP : C0_SP;
      c0_frmsz = - cache->c0.c0_rt[fp_regnum].fr_ofs;
      fp_regnum += tdep->a0_base;
    }
  else  /* No data from the prologue analysis.  */
    {
      c0_hasfp = 0;
      fp_regnum = tdep->a0_base + C0_SP;
      c0_frmsz = 0;
      start_pc = pc;
   }

  if (cache->c0.c0_fpalign)
    {
      /* This frame has a special prologue with a dynamic stack adjustment
	 to force an alignment, which is bigger than standard 16 bytes.  */

      CORE_ADDR unaligned_sp;

      if (cache->c0.c0_old_sp == C0_INEXP)
	/* This can't be.  Prologue code should be consistent.
	   Unaligned stack pointer should be saved in a spare register.  */
	{
	  warning_once ();
	  ra = 0;
	  goto finish_frame_analysis;
	}

      if (cache->c0.c0_sp_ofs == C0_NOSTK)
	/* Saved unaligned value of SP is kept in a register.  */
	unaligned_sp = get_frame_register_unsigned
	  (this_frame, tdep->a0_base + cache->c0.c0_old_sp);
      else
	/* Get the value from stack.  */
	unaligned_sp = (CORE_ADDR)
	  read_memory_integer (fp + cache->c0.c0_sp_ofs, 4, byte_order);

      prev_sp = unaligned_sp + c0_frmsz;
    }
  else
    prev_sp = fp + c0_frmsz;

  /* Frame size from debug info or prologue tracking does not account for 
     alloca() and other dynamic allocations.  Adjust frame size by FP - SP.  */
  if (c0_hasfp)
    {
      fp = get_frame_register_unsigned (this_frame, fp_regnum);

      /* Update the stack frame size.  */
      c0_frmsz += fp - sp;
    }

  /* Get the return address (RA) from the stack if saved,
     or try to get it from a register.  */

  to_stk = cache->c0.c0_rt[C0_RA].to_stk;
  if (to_stk != C0_NOSTK)
    ra = (CORE_ADDR) 
      read_memory_integer (sp + c0_frmsz + cache->c0.c0_rt[C0_RA].to_stk,
			   4, byte_order);

  else if (cache->c0.c0_rt[C0_RA].fr_reg == C0_CONST
	   && cache->c0.c0_rt[C0_RA].fr_ofs == 0)
    {
      /* Special case for terminating backtrace at a function that wants to
	 be seen as the outermost one.  Such a function will clear it's RA (A0)
	 register to 0 in the prologue instead of saving its original value.  */
      ra = 0;
    }
  else
    {
      /* RA was copied to another register or (before any function call) may
	 still be in the original RA register.  This is not always reliable:
	 even in a leaf function, register tracking stops after prologue, and
	 even in prologue, non-prologue instructions (not tracked) may overwrite
	 RA or any register it was copied to.  If likely in prologue or before
	 any call, use retracking info and hope for the best (compiler should
	 have saved RA in stack if not in a leaf function).  If not in prologue,
	 too bad.  */

      int i;
      for (i = 0;
	   (i < C0_NREGS)
	   && (i == C0_RA || cache->c0.c0_rt[i].fr_reg != C0_RA);
	   ++i);
      if (i >= C0_NREGS && cache->c0.c0_rt[C0_RA].fr_reg == C0_RA)
	i = C0_RA;
      if (i < C0_NREGS)
	{
	  ra = get_frame_register_unsigned
	    (this_frame,
	     tdep->a0_base + cache->c0.c0_rt[i].fr_reg);
	}
      else ra = 0;
    }
  
 finish_frame_analysis:
  cache->pc = start_pc;
  cache->ra = ra;
  /* RA == 0 marks the outermost frame.  Do not go past it.  */
  cache->prev_sp = (ra != 0) ?  prev_sp : 0;
  cache->c0.fp_regnum = fp_regnum;
  cache->c0.c0_frmsz = c0_frmsz;
  cache->c0.c0_hasfp = c0_hasfp;
  cache->c0.c0_fp = fp;
}

static CORE_ADDR a0_saved;
static CORE_ADDR a7_saved;
static CORE_ADDR a11_saved;
static int a0_was_saved;
static int a7_was_saved;
static int a11_was_saved;

/* Simulate L32E instruction:  AT <-- ref (AS + offset).  */
static void
execute_l32e (struct gdbarch *gdbarch, int at, int as, int offset, CORE_ADDR wb)
{
  xtensa_gdbarch_tdep *tdep = gdbarch_tdep<xtensa_gdbarch_tdep> (gdbarch);
  int atreg = arreg_number (gdbarch, tdep->a0_base + at, wb);
  int asreg = arreg_number (gdbarch, tdep->a0_base + as, wb);
  CORE_ADDR addr = xtensa_read_register (asreg) + offset;
  unsigned int spilled_value
    = read_memory_unsigned_integer (addr, 4, gdbarch_byte_order (gdbarch));

  if ((at == 0) && !a0_was_saved)
    {
      a0_saved = xtensa_read_register (atreg);
      a0_was_saved = 1;
    }
  else if ((at == 7) && !a7_was_saved)
    {
      a7_saved = xtensa_read_register (atreg);
      a7_was_saved = 1;
    }
  else if ((at == 11) && !a11_was_saved)
    {
      a11_saved = xtensa_read_register (atreg);
      a11_was_saved = 1;
    }

  xtensa_write_register (atreg, spilled_value);
}

/* Simulate S32E instruction:  AT --> ref (AS + offset).  */
static void
execute_s32e (struct gdbarch *gdbarch, int at, int as, int offset, CORE_ADDR wb)
{
  xtensa_gdbarch_tdep *tdep = gdbarch_tdep<xtensa_gdbarch_tdep> (gdbarch);
  int atreg = arreg_number (gdbarch, tdep->a0_base + at, wb);
  int asreg = arreg_number (gdbarch, tdep->a0_base + as, wb);
  CORE_ADDR addr = xtensa_read_register (asreg) + offset;
  ULONGEST spilled_value = xtensa_read_register (atreg);

  write_memory_unsigned_integer (addr, 4,
				 gdbarch_byte_order (gdbarch),
				 spilled_value);
}

#define XTENSA_MAX_WINDOW_INTERRUPT_HANDLER_LEN  200

enum xtensa_exception_handler_t
{
  xtWindowOverflow,
  xtWindowUnderflow,
  xtNoExceptionHandler
};

/* Execute instruction stream from current PC until hitting RFWU or RFWO.
   Return type of Xtensa Window Interrupt Handler on success.  */
static xtensa_exception_handler_t
execute_code (struct gdbarch *gdbarch, CORE_ADDR current_pc, CORE_ADDR wb)
{
  xtensa_isa isa;
  xtensa_insnbuf ins, slot;
  gdb_byte ibuf[XTENSA_ISA_BSZ];
  CORE_ADDR ia, bt, ba;
  xtensa_format ifmt;
  int ilen, islots, is;
  xtensa_opcode opc;
  int insn_num = 0;
  void (*func) (struct gdbarch *, int, int, int, CORE_ADDR);
  xtensa_gdbarch_tdep *tdep = gdbarch_tdep<xtensa_gdbarch_tdep> (gdbarch);

  uint32_t at, as, offset;

  /* WindowUnderflow12 = true, when inside _WindowUnderflow12.  */ 
  int WindowUnderflow12 = (current_pc & 0x1ff) >= 0x140; 

  isa = xtensa_default_isa;
  gdb_assert (XTENSA_ISA_BSZ >= xtensa_isa_maxlength (isa));
  ins = xtensa_insnbuf_alloc (isa);
  slot = xtensa_insnbuf_alloc (isa);
  ba = 0;
  ia = current_pc;
  bt = ia;

  a0_was_saved = 0;
  a7_was_saved = 0;
  a11_was_saved = 0;

  while (insn_num++ < XTENSA_MAX_WINDOW_INTERRUPT_HANDLER_LEN)
    {
      if (ia + xtensa_isa_maxlength (isa) > bt)
	{
	  ba = ia;
	  bt = (ba + XTENSA_ISA_BSZ);
	  if (target_read_memory (ba, ibuf, bt - ba) != 0)
	    return xtNoExceptionHandler;
	}
      xtensa_insnbuf_from_chars (isa, ins, &ibuf[ia-ba], 0);
      ifmt = xtensa_format_decode (isa, ins);
      if (ifmt == XTENSA_UNDEFINED)
	return xtNoExceptionHandler;
      ilen = xtensa_format_length (isa, ifmt);
      if (ilen == XTENSA_UNDEFINED)
	return xtNoExceptionHandler;
      islots = xtensa_format_num_slots (isa, ifmt);
      if (islots == XTENSA_UNDEFINED)
	return xtNoExceptionHandler;
      for (is = 0; is < islots; ++is)
	{
	  if (xtensa_format_get_slot (isa, ifmt, is, ins, slot))
	    return xtNoExceptionHandler;
	  opc = xtensa_opcode_decode (isa, ifmt, is, slot);
	  if (opc == XTENSA_UNDEFINED) 
	    return xtNoExceptionHandler;
	  switch (call0_classify_opcode (isa, opc))
	    {
	    case c0opc_illegal:
	    case c0opc_flow:
	    case c0opc_entry:
	    case c0opc_break:
	      /* We expect none of them here.  */
	      return xtNoExceptionHandler;
	    case c0opc_l32e:
	      func = execute_l32e;
	      break;
	    case c0opc_s32e:
	      func = execute_s32e;
	      break;
	    case c0opc_rfwo: /* RFWO.  */
	      /* Here, we return from WindowOverflow handler and,
		 if we stopped at the very beginning, which means
		 A0 was saved, we have to restore it now.  */
	      if (a0_was_saved)
		{
		  int arreg = arreg_number (gdbarch,
					    tdep->a0_base,
					    wb);
		  xtensa_write_register (arreg, a0_saved);
		}
	      return xtWindowOverflow;
	    case c0opc_rfwu: /* RFWU.  */
	      /* Here, we return from WindowUnderflow handler.
		 Let's see if either A7 or A11 has to be restored.  */
	      if (WindowUnderflow12)
		{
		  if (a11_was_saved)
		    {
		      int arreg = arreg_number (gdbarch,
						tdep->a0_base + 11,
						wb);
		      xtensa_write_register (arreg, a11_saved);
		    }
		}
	      else if (a7_was_saved)
		{
		  int arreg = arreg_number (gdbarch,
					    tdep->a0_base + 7,
					    wb);
		  xtensa_write_register (arreg, a7_saved);
		}
	      return xtWindowUnderflow;
	    default: /* Simply skip this insns.  */
	      continue;
	    }

	  /* Decode arguments for L32E / S32E and simulate their execution.  */
	  if ( xtensa_opcode_num_operands (isa, opc) != 3 )
	    return xtNoExceptionHandler;
	  if (xtensa_operand_get_field (isa, opc, 0, ifmt, is, slot, &at))
	    return xtNoExceptionHandler;
	  if (xtensa_operand_decode (isa, opc, 0, &at))
	    return xtNoExceptionHandler;
	  if (xtensa_operand_get_field (isa, opc, 1, ifmt, is, slot, &as))
	    return xtNoExceptionHandler;
	  if (xtensa_operand_decode (isa, opc, 1, &as))
	    return xtNoExceptionHandler;
	  if (xtensa_operand_get_field (isa, opc, 2, ifmt, is, slot, &offset))
	    return xtNoExceptionHandler;
	  if (xtensa_operand_decode (isa, opc, 2, &offset))
	    return xtNoExceptionHandler;

	  (*func) (gdbarch, at, as, offset, wb);
	}

      ia += ilen;
    }
  return xtNoExceptionHandler;
}

/* Handle Window Overflow / Underflow exception frames.  */

static void
xtensa_window_interrupt_frame_cache (frame_info_ptr this_frame,
				     xtensa_frame_cache_t *cache,
				     CORE_ADDR pc)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  CORE_ADDR ps, wb, ws, ra;
  int epc1_regnum, i, regnum;
  xtensa_exception_handler_t eh_type;
  xtensa_gdbarch_tdep *tdep = gdbarch_tdep<xtensa_gdbarch_tdep> (gdbarch);

  /* Read PS, WB, and WS from the hardware. Note that PS register
     must be present, if Windowed ABI is supported.  */
  ps = xtensa_read_register (gdbarch_ps_regnum (gdbarch));
  wb = xtensa_read_register (tdep->wb_regnum);
  ws = xtensa_read_register (tdep->ws_regnum);

  /* Execute all the remaining instructions from Window Interrupt Handler
     by simulating them on the remote protocol level.  On return, set the
     type of Xtensa Window Interrupt Handler, or report an error.  */
  eh_type = execute_code (gdbarch, pc, wb);
  if (eh_type == xtNoExceptionHandler)
    error (_("\
Unable to decode Xtensa Window Interrupt Handler's code."));

  cache->ps = ps ^ PS_EXC;	/* Clear the exception bit in PS.  */
  cache->call0 = 0;		/* It's Windowed ABI.  */

  /* All registers for the cached frame will be alive.  */
  for (i = 0; i < XTENSA_NUM_SAVED_AREGS; i++)
    cache->wd.aregs[i] = -1;

  if (eh_type == xtWindowOverflow)
    cache->wd.ws = ws ^ (1 << wb);
  else /* eh_type == xtWindowUnderflow.  */
    cache->wd.ws = ws | (1 << wb);

  cache->wd.wb = (ps & 0xf00) >> 8; /* Set WB to OWB.  */
  regnum = arreg_number (gdbarch, tdep->a0_base,
			 cache->wd.wb);
  ra = xtensa_read_register (regnum);
  cache->wd.callsize = WINSIZE (ra);
  cache->prev_sp = xtensa_read_register (regnum + 1);
  /* Set regnum to a frame pointer of the frame being cached.  */
  regnum = xtensa_scan_prologue (gdbarch, pc);
  regnum = arreg_number (gdbarch,
			 tdep->a0_base + regnum,
			 cache->wd.wb);
  cache->base = get_frame_register_unsigned (this_frame, regnum);

  /* Read PC of interrupted function from EPC1 register.  */
  epc1_regnum = xtensa_find_register_by_name (gdbarch,"epc1");
  if (epc1_regnum < 0)
    error(_("Unable to read Xtensa register EPC1"));
  cache->ra = xtensa_read_register (epc1_regnum);
  cache->pc = get_frame_func (this_frame);
}


/* Skip function prologue.

   Return the pc of the first instruction after prologue.  GDB calls this to
   find the address of the first line of the function or (if there is no line
   number information) to skip the prologue for planting breakpoints on 
   function entries.  Use debug info (if present) or prologue analysis to skip 
   the prologue to achieve reliable debugging behavior.  For windowed ABI, 
   only the 'entry' instruction is skipped.  It is not strictly necessary to 
   skip the prologue (Call0) or 'entry' (Windowed) because xt-gdb knows how to
   backtrace at any point in the prologue, however certain potential hazards 
   are avoided and a more "normal" debugging experience is ensured by 
   skipping the prologue (can be disabled by defining DONT_SKIP_PROLOG).
   For example, if we don't skip the prologue:
   - Some args may not yet have been saved to the stack where the debug
     info expects to find them (true anyway when only 'entry' is skipped);
   - Software breakpoints ('break' instrs) may not have been unplanted 
     when the prologue analysis is done on initializing the frame cache, 
     and breaks in the prologue will throw off the analysis.

   If we have debug info ( line-number info, in particular ) we simply skip
   the code associated with the first function line effectively skipping
   the prologue code.  It works even in cases like

   int main()
   {	int local_var = 1;
	....
   }

   because, for this source code, both Xtensa compilers will generate two
   separate entries ( with the same line number ) in dwarf line-number
   section to make sure there is a boundary between the prologue code and
   the rest of the function.

   If there is no debug info, we need to analyze the code.  */

/* #define DONT_SKIP_PROLOGUE  */

static CORE_ADDR
xtensa_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR start_pc)
{
  struct symtab_and_line prologue_sal;
  CORE_ADDR body_pc;

  DEBUGTRACE ("xtensa_skip_prologue (start_pc = 0x%08x)\n", (int) start_pc);

#if DONT_SKIP_PROLOGUE
  return start_pc;
#endif

 /* Try to find first body line from debug info.  */

  prologue_sal = find_pc_line (start_pc, 0);
  if (prologue_sal.line != 0) /* Found debug info.  */
    {
      /* In Call0,  it is possible to have a function with only one instruction
	 ('ret') resulting from a one-line optimized function that does nothing.
	 In that case,  prologue_sal.end may actually point to the start of the
	 next function in the text section,  causing a breakpoint to be set at
	 the wrong place.  Check,  if the end address is within a different
	 function,  and if so return the start PC.  We know we have symbol
	 information.  */

      CORE_ADDR end_func;

      xtensa_gdbarch_tdep *tdep = gdbarch_tdep<xtensa_gdbarch_tdep> (gdbarch);
      if ((tdep->call_abi == CallAbiCall0Only)
	  && call0_ret (start_pc, prologue_sal.end))
	return start_pc;

      find_pc_partial_function (prologue_sal.end, NULL, &end_func, NULL);
      if (end_func != start_pc)
	return start_pc;

      return prologue_sal.end;
    }

  /* No debug line info.  Analyze prologue for Call0 or simply skip ENTRY.  */
  body_pc = call0_analyze_prologue (gdbarch, start_pc, 0, 0,
				    xtensa_alloc_frame_cache (0));
  return body_pc != 0 ? body_pc : start_pc;
}

/* Verify the current configuration.  */
static void
xtensa_verify_config (struct gdbarch *gdbarch)
{
  xtensa_gdbarch_tdep *tdep = gdbarch_tdep<xtensa_gdbarch_tdep> (gdbarch);
  string_file log;

  /* Verify that we got a reasonable number of AREGS.  */
  if ((tdep->num_aregs & -tdep->num_aregs) != tdep->num_aregs)
    log.printf (_("\
\n\tnum_aregs: Number of AR registers (%d) is not a power of two!"),
		tdep->num_aregs);

  /* Verify that certain registers exist.  */

  if (tdep->pc_regnum == -1)
    log.printf (_("\n\tpc_regnum: No PC register"));
  if (tdep->isa_use_exceptions && tdep->ps_regnum == -1)
    log.printf (_("\n\tps_regnum: No PS register"));

  if (tdep->isa_use_windowed_registers)
    {
      if (tdep->wb_regnum == -1)
	log.printf (_("\n\twb_regnum: No WB register"));
      if (tdep->ws_regnum == -1)
	log.printf (_("\n\tws_regnum: No WS register"));
      if (tdep->ar_base == -1)
	log.printf (_("\n\tar_base: No AR registers"));
    }

  if (tdep->a0_base == -1)
    log.printf (_("\n\ta0_base: No Ax registers"));

  if (!log.empty ())
    internal_error (_("the following are invalid: %s"), log.c_str ());
}


/* Derive specific register numbers from the array of registers.  */

static void
xtensa_derive_tdep (xtensa_gdbarch_tdep *tdep)
{
  xtensa_register_t* rmap;
  int n, max_size = 4;

  tdep->num_regs = 0;
  tdep->num_nopriv_regs = 0;

/* Special registers 0..255 (core).  */
#define XTENSA_DBREGN_SREG(n)  (0x0200+(n))
/* User registers 0..255.  */
#define XTENSA_DBREGN_UREG(n)  (0x0300+(n))

  for (rmap = tdep->regmap, n = 0; rmap->target_number != -1; n++, rmap++)
    {
      if (rmap->target_number == 0x0020)
	tdep->pc_regnum = n;
      else if (rmap->target_number == 0x0100)
	tdep->ar_base = n;
      else if (rmap->target_number == 0x0000)
	tdep->a0_base = n;
      else if (rmap->target_number == XTENSA_DBREGN_SREG(72))
	tdep->wb_regnum = n;
      else if (rmap->target_number == XTENSA_DBREGN_SREG(73))
	tdep->ws_regnum = n;
      else if (rmap->target_number == XTENSA_DBREGN_SREG(233))
	tdep->debugcause_regnum = n;
      else if (rmap->target_number == XTENSA_DBREGN_SREG(232))
	tdep->exccause_regnum = n;
      else if (rmap->target_number == XTENSA_DBREGN_SREG(238))
	tdep->excvaddr_regnum = n;
      else if (rmap->target_number == XTENSA_DBREGN_SREG(0))
	tdep->lbeg_regnum = n;
      else if (rmap->target_number == XTENSA_DBREGN_SREG(1))
	tdep->lend_regnum = n;
      else if (rmap->target_number == XTENSA_DBREGN_SREG(2))
	tdep->lcount_regnum = n;
      else if (rmap->target_number == XTENSA_DBREGN_SREG(3))
	tdep->sar_regnum = n;
      else if (rmap->target_number == XTENSA_DBREGN_SREG(5))
	tdep->litbase_regnum = n;
      else if (rmap->target_number == XTENSA_DBREGN_SREG(230))
	tdep->ps_regnum = n;
      else if (rmap->target_number == XTENSA_DBREGN_UREG(231))
	tdep->threadptr_regnum = n;
#if 0
      else if (rmap->target_number == XTENSA_DBREGN_SREG(226))
	tdep->interrupt_regnum = n;
      else if (rmap->target_number == XTENSA_DBREGN_SREG(227))
	tdep->interrupt2_regnum = n;
      else if (rmap->target_number == XTENSA_DBREGN_SREG(224))
	tdep->cpenable_regnum = n;
#endif

      if (rmap->byte_size > max_size)
	max_size = rmap->byte_size;
      if (rmap->mask != 0 && tdep->num_regs == 0)
	tdep->num_regs = n;
      if ((rmap->flags & XTENSA_REGISTER_FLAGS_PRIVILEGED) != 0
	  && tdep->num_nopriv_regs == 0)
	tdep->num_nopriv_regs = n;
    }
  if (tdep->num_regs == 0)
    tdep->num_regs = tdep->num_nopriv_regs;

  /* Number of pseudo registers.  */
  tdep->num_pseudo_regs = n - tdep->num_regs;

  /* Empirically determined maximum sizes.  */
  tdep->max_register_raw_size = max_size;
  tdep->max_register_virtual_size = max_size;
}

/* Module "constructor" function.  */

extern xtensa_register_t xtensa_rmap[];

static struct gdbarch *
xtensa_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  DEBUGTRACE ("gdbarch_init()\n");

  if (!xtensa_default_isa)
    xtensa_default_isa = xtensa_isa_init (0, 0);

  /* We have to set the byte order before we call gdbarch_alloc.  */
  info.byte_order = XCHAL_HAVE_BE ? BFD_ENDIAN_BIG : BFD_ENDIAN_LITTLE;

  gdbarch *gdbarch
    = gdbarch_alloc (&info,
		     gdbarch_tdep_up (new xtensa_gdbarch_tdep (xtensa_rmap)));
  xtensa_gdbarch_tdep *tdep = gdbarch_tdep<xtensa_gdbarch_tdep> (gdbarch);
  xtensa_derive_tdep (tdep);

  /* Verify our configuration.  */
  xtensa_verify_config (gdbarch);
  xtensa_session_once_reported = 0;

  set_gdbarch_wchar_bit (gdbarch, 2 * TARGET_CHAR_BIT);
  set_gdbarch_wchar_signed (gdbarch, 0);

  /* Pseudo-Register read/write.  */
  set_gdbarch_pseudo_register_read (gdbarch, xtensa_pseudo_register_read);
  set_gdbarch_deprecated_pseudo_register_write (gdbarch,
						xtensa_pseudo_register_write);

  /* Set target information.  */
  set_gdbarch_num_regs (gdbarch, tdep->num_regs);
  set_gdbarch_num_pseudo_regs (gdbarch, tdep->num_pseudo_regs);
  set_gdbarch_sp_regnum (gdbarch, tdep->a0_base + 1);
  set_gdbarch_pc_regnum (gdbarch, tdep->pc_regnum);
  set_gdbarch_ps_regnum (gdbarch, tdep->ps_regnum);

  /* Renumber registers for known formats (stabs and dwarf2).  */
  set_gdbarch_stab_reg_to_regnum (gdbarch, xtensa_reg_to_regnum);
  set_gdbarch_dwarf2_reg_to_regnum (gdbarch, xtensa_reg_to_regnum);

  /* We provide our own function to get register information.  */
  set_gdbarch_register_name (gdbarch, xtensa_register_name);
  set_gdbarch_register_type (gdbarch, xtensa_register_type);

  /* To call functions from GDB using dummy frame.  */
  set_gdbarch_push_dummy_call (gdbarch, xtensa_push_dummy_call);

  set_gdbarch_believe_pcc_promotion (gdbarch, 1);

  set_gdbarch_return_value (gdbarch, xtensa_return_value);

  /* Advance PC across any prologue instructions to reach "real" code.  */
  set_gdbarch_skip_prologue (gdbarch, xtensa_skip_prologue);

  /* Stack grows downward.  */
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);

  /* Set breakpoints.  */
  set_gdbarch_breakpoint_kind_from_pc (gdbarch,
				       xtensa_breakpoint_kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch,
				       xtensa_sw_breakpoint_from_kind);

  /* After breakpoint instruction or illegal instruction, pc still
     points at break instruction, so don't decrement.  */
  set_gdbarch_decr_pc_after_break (gdbarch, 0);

  /* We don't skip args.  */
  set_gdbarch_frame_args_skip (gdbarch, 0);

  set_gdbarch_unwind_pc (gdbarch, xtensa_unwind_pc);

  set_gdbarch_frame_align (gdbarch, xtensa_frame_align);

  set_gdbarch_dummy_id (gdbarch, xtensa_dummy_id);

  /* Frame handling.  */
  frame_base_set_default (gdbarch, &xtensa_frame_base);
  frame_unwind_append_unwinder (gdbarch, &xtensa_unwind);
  dwarf2_append_unwinders (gdbarch);

  set_gdbarch_have_nonsteppable_watchpoint (gdbarch, 1);

  xtensa_add_reggroups (gdbarch);
  set_gdbarch_register_reggroup_p (gdbarch, xtensa_register_reggroup_p);

  set_gdbarch_iterate_over_regset_sections
    (gdbarch, xtensa_iterate_over_regset_sections);

  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, svr4_ilp32_fetch_link_map_offsets);

  /* Hook in the ABI-specific overrides, if they have been registered.  */
  gdbarch_init_osabi (info, gdbarch);

  return gdbarch;
}

static void
xtensa_dump_tdep (struct gdbarch *gdbarch, struct ui_file *file)
{
  error (_("xtensa_dump_tdep(): not implemented"));
}

void _initialize_xtensa_tdep ();
void
_initialize_xtensa_tdep ()
{
  gdbarch_register (bfd_arch_xtensa, xtensa_gdbarch_init, xtensa_dump_tdep);
  xtensa_init_reggroups ();

  add_setshow_zuinteger_cmd ("xtensa",
			     class_maintenance,
			     &xtensa_debug_level,
			    _("Set Xtensa debugging."),
			    _("Show Xtensa debugging."), _("\
When non-zero, Xtensa-specific debugging is enabled. \
Can be 1, 2, 3, or 4 indicating the level of debugging."),
			     NULL,
			     NULL,
			     &setdebuglist, &showdebuglist);
}
