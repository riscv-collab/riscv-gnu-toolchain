/* Intel 387 floating point stuff.

   Copyright (C) 1988-2024 Free Software Foundation, Inc.

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
#include "gdbcore.h"
#include "inferior.h"
#include "language.h"
#include "regcache.h"
#include "target-float.h"
#include "value.h"

#include "i386-tdep.h"
#include "i387-tdep.h"
#include "gdbsupport/x86-xstate.h"

/* Print the floating point number specified by RAW.  */

static void
print_i387_value (struct gdbarch *gdbarch,
		  const gdb_byte *raw, struct ui_file *file)
{
  /* We try to print 19 digits.  The last digit may or may not contain
     garbage, but we'd better print one too many.  We need enough room
     to print the value, 1 position for the sign, 1 for the decimal
     point, 19 for the digits and 6 for the exponent adds up to 27.  */
  const struct type *type = i387_ext_type (gdbarch);
  std::string str = target_float_to_string (raw, type, " %-+27.19g");
  gdb_printf (file, "%s", str.c_str ());
}

/* Print the classification for the register contents RAW.  */

static void
print_i387_ext (struct gdbarch *gdbarch,
		const gdb_byte *raw, struct ui_file *file)
{
  int sign;
  int integer;
  unsigned int exponent;
  unsigned long fraction[2];

  sign = raw[9] & 0x80;
  integer = raw[7] & 0x80;
  exponent = (((raw[9] & 0x7f) << 8) | raw[8]);
  fraction[0] = ((raw[3] << 24) | (raw[2] << 16) | (raw[1] << 8) | raw[0]);
  fraction[1] = (((raw[7] & 0x7f) << 24) | (raw[6] << 16)
		 | (raw[5] << 8) | raw[4]);

  if (exponent == 0x7fff && integer)
    {
      if (fraction[0] == 0x00000000 && fraction[1] == 0x00000000)
	/* Infinity.  */
	gdb_printf (file, " %cInf", (sign ? '-' : '+'));
      else if (sign && fraction[0] == 0x00000000 && fraction[1] == 0x40000000)
	/* Real Indefinite (QNaN).  */
	gdb_puts (" Real Indefinite (QNaN)", file);
      else if (fraction[1] & 0x40000000)
	/* QNaN.  */
	gdb_puts (" QNaN", file);
      else
	/* SNaN.  */
	gdb_puts (" SNaN", file);
    }
  else if (exponent < 0x7fff && exponent > 0x0000 && integer)
    /* Normal.  */
    print_i387_value (gdbarch, raw, file);
  else if (exponent == 0x0000)
    {
      /* Denormal or zero.  */
      print_i387_value (gdbarch, raw, file);
      
      if (integer)
	/* Pseudo-denormal.  */
	gdb_puts (" Pseudo-denormal", file);
      else if (fraction[0] || fraction[1])
	/* Denormal.  */
	gdb_puts (" Denormal", file);
    }
  else
    /* Unsupported.  */
    gdb_puts (" Unsupported", file);
}

/* Print the status word STATUS.  If STATUS_P is false, then STATUS
   was unavailable.  */

static void
print_i387_status_word (int status_p,
			unsigned int status, struct ui_file *file)
{
  gdb_printf (file, "Status Word:         ");
  if (!status_p)
    {
      gdb_printf (file, "%s\n", _("<unavailable>"));
      return;
    }

  gdb_printf (file, "%s", hex_string_custom (status, 4));
  gdb_puts ("  ", file);
  gdb_printf (file, " %s", (status & 0x0001) ? "IE" : "  ");
  gdb_printf (file, " %s", (status & 0x0002) ? "DE" : "  ");
  gdb_printf (file, " %s", (status & 0x0004) ? "ZE" : "  ");
  gdb_printf (file, " %s", (status & 0x0008) ? "OE" : "  ");
  gdb_printf (file, " %s", (status & 0x0010) ? "UE" : "  ");
  gdb_printf (file, " %s", (status & 0x0020) ? "PE" : "  ");
  gdb_puts ("  ", file);
  gdb_printf (file, " %s", (status & 0x0080) ? "ES" : "  ");
  gdb_puts ("  ", file);
  gdb_printf (file, " %s", (status & 0x0040) ? "SF" : "  ");
  gdb_puts ("  ", file);
  gdb_printf (file, " %s", (status & 0x0100) ? "C0" : "  ");
  gdb_printf (file, " %s", (status & 0x0200) ? "C1" : "  ");
  gdb_printf (file, " %s", (status & 0x0400) ? "C2" : "  ");
  gdb_printf (file, " %s", (status & 0x4000) ? "C3" : "  ");

  gdb_puts ("\n", file);

  gdb_printf (file,
	      "                       TOP: %d\n", ((status >> 11) & 7));
}

/* Print the control word CONTROL.  If CONTROL_P is false, then
   CONTROL was unavailable.  */

static void
print_i387_control_word (int control_p,
			 unsigned int control, struct ui_file *file)
{
  gdb_printf (file, "Control Word:        ");
  if (!control_p)
    {
      gdb_printf (file, "%s\n", _("<unavailable>"));
      return;
    }

  gdb_printf (file, "%s", hex_string_custom (control, 4));
  gdb_puts ("  ", file);
  gdb_printf (file, " %s", (control & 0x0001) ? "IM" : "  ");
  gdb_printf (file, " %s", (control & 0x0002) ? "DM" : "  ");
  gdb_printf (file, " %s", (control & 0x0004) ? "ZM" : "  ");
  gdb_printf (file, " %s", (control & 0x0008) ? "OM" : "  ");
  gdb_printf (file, " %s", (control & 0x0010) ? "UM" : "  ");
  gdb_printf (file, " %s", (control & 0x0020) ? "PM" : "  ");

  gdb_puts ("\n", file);

  gdb_puts ("                       PC: ", file);
  switch ((control >> 8) & 3)
    {
    case 0:
      gdb_puts ("Single Precision (24-bits)\n", file);
      break;
    case 1:
      gdb_puts ("Reserved\n", file);
      break;
    case 2:
      gdb_puts ("Double Precision (53-bits)\n", file);
      break;
    case 3:
      gdb_puts ("Extended Precision (64-bits)\n", file);
      break;
    }
      
  gdb_puts ("                       RC: ", file);
  switch ((control >> 10) & 3)
    {
    case 0:
      gdb_puts ("Round to nearest\n", file);
      break;
    case 1:
      gdb_puts ("Round down\n", file);
      break;
    case 2:
      gdb_puts ("Round up\n", file);
      break;
    case 3:
      gdb_puts ("Round toward zero\n", file);
      break;
    }
}

/* Print out the i387 floating point state.  Note that we ignore FRAME
   in the code below.  That's OK since floating-point registers are
   never saved on the stack.  */

void
i387_print_float_info (struct gdbarch *gdbarch, struct ui_file *file,
		       frame_info_ptr frame, const char *args)
{
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);
  ULONGEST fctrl;
  int fctrl_p;
  ULONGEST fstat;
  int fstat_p;
  ULONGEST ftag;
  int ftag_p;
  ULONGEST fiseg;
  int fiseg_p;
  ULONGEST fioff;
  int fioff_p;
  ULONGEST foseg;
  int foseg_p;
  ULONGEST fooff;
  int fooff_p;
  ULONGEST fop;
  int fop_p;
  int fpreg;
  int top;

  gdb_assert (gdbarch == get_frame_arch (frame));

  fctrl_p = read_frame_register_unsigned (frame,
					  I387_FCTRL_REGNUM (tdep), &fctrl);
  fstat_p = read_frame_register_unsigned (frame,
					  I387_FSTAT_REGNUM (tdep), &fstat);
  ftag_p = read_frame_register_unsigned (frame,
					 I387_FTAG_REGNUM (tdep), &ftag);
  fiseg_p = read_frame_register_unsigned (frame,
					  I387_FISEG_REGNUM (tdep), &fiseg);
  fioff_p = read_frame_register_unsigned (frame,
					  I387_FIOFF_REGNUM (tdep), &fioff);
  foseg_p = read_frame_register_unsigned (frame,
					  I387_FOSEG_REGNUM (tdep), &foseg);
  fooff_p = read_frame_register_unsigned (frame,
					  I387_FOOFF_REGNUM (tdep), &fooff);
  fop_p = read_frame_register_unsigned (frame,
					I387_FOP_REGNUM (tdep), &fop);

  if (fstat_p)
    {
      top = ((fstat >> 11) & 7);

      for (fpreg = 7; fpreg >= 0; fpreg--)
	{
	  struct value *regval;
	  int regnum;
	  int i;
	  int tag = -1;

	  gdb_printf (file, "%sR%d: ", fpreg == top ? "=>" : "  ", fpreg);

	  if (ftag_p)
	    {
	      tag = (ftag >> (fpreg * 2)) & 3;

	      switch (tag)
		{
		case 0:
		  gdb_puts ("Valid   ", file);
		  break;
		case 1:
		  gdb_puts ("Zero    ", file);
		  break;
		case 2:
		  gdb_puts ("Special ", file);
		  break;
		case 3:
		  gdb_puts ("Empty   ", file);
		  break;
		}
	    }
	  else
	    gdb_puts ("Unknown ", file);

	  regnum = (fpreg + 8 - top) % 8 + I387_ST0_REGNUM (tdep);
	  regval = get_frame_register_value (frame, regnum);

	  if (regval->entirely_available ())
	    {
	      const gdb_byte *raw = regval->contents ().data ();

	      gdb_puts ("0x", file);
	      for (i = 9; i >= 0; i--)
		gdb_printf (file, "%02x", raw[i]);

	      if (tag != -1 && tag != 3)
		print_i387_ext (gdbarch, raw, file);
	    }
	  else
	    gdb_printf (file, "%s", _("<unavailable>"));

	  gdb_puts ("\n", file);
	}
    }

  gdb_puts ("\n", file);
  print_i387_status_word (fstat_p, fstat, file);
  print_i387_control_word (fctrl_p, fctrl, file);
  gdb_printf (file, "Tag Word:            %s\n",
	      ftag_p ? hex_string_custom (ftag, 4) : _("<unavailable>"));
  gdb_printf (file, "Instruction Pointer: %s:",
	      fiseg_p ? hex_string_custom (fiseg, 2) : _("<unavailable>"));
  gdb_printf (file, "%s\n",
	      fioff_p ? hex_string_custom (fioff, 8) : _("<unavailable>"));
  gdb_printf (file, "Operand Pointer:     %s:",
	      foseg_p ? hex_string_custom (foseg, 2) : _("<unavailable>"));
  gdb_printf (file, "%s\n",
	      fooff_p ? hex_string_custom (fooff, 8) : _("<unavailable>"));
  gdb_printf (file, "Opcode:              %s\n",
	      fop_p
	      ? (hex_string_custom (fop ? (fop | 0xd800) : 0, 4))
	      : _("<unavailable>"));
}


/* Return nonzero if a value of type TYPE stored in register REGNUM
   needs any special handling.  */

int
i387_convert_register_p (struct gdbarch *gdbarch, int regnum,
			 struct type *type)
{
  if (i386_fp_regnum_p (gdbarch, regnum))
    {
      /* Floating point registers must be converted unless we are
	 accessing them in their hardware type or TYPE is not float.  */
      if (type == i387_ext_type (gdbarch)
	  || type->code () != TYPE_CODE_FLT)
	return 0;
      else
	return 1;
    }

  return 0;
}

/* Read a value of type TYPE from register REGNUM in frame FRAME, and
   return its contents in TO.  */

int
i387_register_to_value (frame_info_ptr frame, int regnum,
			struct type *type, gdb_byte *to,
			int *optimizedp, int *unavailablep)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  gdb_byte from[I386_MAX_REGISTER_SIZE];

  gdb_assert (i386_fp_regnum_p (gdbarch, regnum));

  /* We only support floating-point values.  */
  if (type->code () != TYPE_CODE_FLT)
    {
      warning (_("Cannot convert floating-point register value "
	       "to non-floating-point type."));
      *optimizedp = *unavailablep = 0;
      return 0;
    }

  /* Convert to TYPE.  */
  auto from_view
    = gdb::make_array_view (from, register_size (gdbarch, regnum));
  frame_info_ptr next_frame = get_next_frame_sentinel_okay (frame);
  if (!get_frame_register_bytes (next_frame, regnum, 0, from_view, optimizedp,
				 unavailablep))
    return 0;

  target_float_convert (from, i387_ext_type (gdbarch), to, type);
  *optimizedp = *unavailablep = 0;
  return 1;
}

/* Write the contents FROM of a value of type TYPE into register
   REGNUM in frame FRAME.  */

void
i387_value_to_register (frame_info_ptr frame, int regnum,
			struct type *type, const gdb_byte *from)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  gdb_byte to[I386_MAX_REGISTER_SIZE];

  gdb_assert (i386_fp_regnum_p (gdbarch, regnum));

  /* We only support floating-point values.  */
  if (type->code () != TYPE_CODE_FLT)
    {
      warning (_("Cannot convert non-floating-point type "
	       "to floating-point register value."));
      return;
    }

  /* Convert from TYPE.  */
  struct type *to_type = i387_ext_type (gdbarch);
  target_float_convert (from, type, to, to_type);
  auto to_view = gdb::make_array_view (to, to_type->length ());
  put_frame_register (get_next_frame_sentinel_okay (frame), regnum, to_view);
}


/* Handle FSAVE and FXSAVE formats.  */

/* At fsave_offset[REGNUM] you'll find the offset to the location in
   the data structure used by the "fsave" instruction where GDB
   register REGNUM is stored.  */

static int fsave_offset[] =
{
  28 + 0 * 10,			/* %st(0) ...  */
  28 + 1 * 10,
  28 + 2 * 10,
  28 + 3 * 10,
  28 + 4 * 10,
  28 + 5 * 10,
  28 + 6 * 10,
  28 + 7 * 10,			/* ... %st(7).  */
  0,				/* `fctrl' (16 bits).  */
  4,				/* `fstat' (16 bits).  */
  8,				/* `ftag' (16 bits).  */
  16,				/* `fiseg' (16 bits).  */
  12,				/* `fioff'.  */
  24,				/* `foseg' (16 bits).  */
  20,				/* `fooff'.  */
  18				/* `fop' (bottom 11 bits).  */
};

#define FSAVE_ADDR(tdep, fsave, regnum) \
  (fsave + fsave_offset[regnum - I387_ST0_REGNUM (tdep)])


/* Fill register REGNUM in REGCACHE with the appropriate value from
   *FSAVE.  This function masks off any of the reserved bits in
   *FSAVE.  */

void
i387_supply_fsave (struct regcache *regcache, int regnum, const void *fsave)
{
  struct gdbarch *gdbarch = regcache->arch ();
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  const gdb_byte *regs = (const gdb_byte *) fsave;
  int i;

  gdb_assert (tdep->st0_regnum >= I386_ST0_REGNUM);

  for (i = I387_ST0_REGNUM (tdep); i < I387_XMM0_REGNUM (tdep); i++)
    if (regnum == -1 || regnum == i)
      {
	if (fsave == NULL)
	  {
	    regcache->raw_supply (i, NULL);
	    continue;
	  }

	/* Most of the FPU control registers occupy only 16 bits in the
	   fsave area.  Give those a special treatment.  */
	if (i >= I387_FCTRL_REGNUM (tdep)
	    && i != I387_FIOFF_REGNUM (tdep) && i != I387_FOOFF_REGNUM (tdep))
	  {
	    gdb_byte val[4];

	    memcpy (val, FSAVE_ADDR (tdep, regs, i), 2);
	    val[2] = val[3] = 0;
	    if (i == I387_FOP_REGNUM (tdep))
	      val[1] &= ((1 << 3) - 1);
	    regcache->raw_supply (i, val);
	  }
	else
	  regcache->raw_supply (i, FSAVE_ADDR (tdep, regs, i));
      }

  /* Provide dummy values for the SSE registers.  */
  for (i = I387_XMM0_REGNUM (tdep); i < I387_MXCSR_REGNUM (tdep); i++)
    if (regnum == -1 || regnum == i)
      regcache->raw_supply (i, NULL);
  if (regnum == -1 || regnum == I387_MXCSR_REGNUM (tdep))
    {
      gdb_byte buf[4];

      store_unsigned_integer (buf, 4, byte_order, I387_MXCSR_INIT_VAL);
      regcache->raw_supply (I387_MXCSR_REGNUM (tdep), buf);
    }
}

/* Fill register REGNUM (if it is a floating-point register) in *FSAVE
   with the value from REGCACHE.  If REGNUM is -1, do this for all
   registers.  This function doesn't touch any of the reserved bits in
   *FSAVE.  */

void
i387_collect_fsave (const struct regcache *regcache, int regnum, void *fsave)
{
  gdbarch *arch = regcache->arch ();
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (arch);
  gdb_byte *regs = (gdb_byte *) fsave;
  int i;

  gdb_assert (tdep->st0_regnum >= I386_ST0_REGNUM);

  for (i = I387_ST0_REGNUM (tdep); i < I387_XMM0_REGNUM (tdep); i++)
    if (regnum == -1 || regnum == i)
      {
	/* Most of the FPU control registers occupy only 16 bits in
	   the fsave area.  Give those a special treatment.  */
	if (i >= I387_FCTRL_REGNUM (tdep)
	    && i != I387_FIOFF_REGNUM (tdep) && i != I387_FOOFF_REGNUM (tdep))
	  {
	    gdb_byte buf[4];

	    regcache->raw_collect (i, buf);

	    if (i == I387_FOP_REGNUM (tdep))
	      {
		/* The opcode occupies only 11 bits.  Make sure we
		   don't touch the other bits.  */
		buf[1] &= ((1 << 3) - 1);
		buf[1] |= ((FSAVE_ADDR (tdep, regs, i))[1] & ~((1 << 3) - 1));
	      }
	    memcpy (FSAVE_ADDR (tdep, regs, i), buf, 2);
	  }
	else
	  regcache->raw_collect (i, FSAVE_ADDR (tdep, regs, i));
      }
}


/* At fxsave_offset[REGNUM] you'll find the offset to the location in
   the data structure used by the "fxsave" instruction where GDB
   register REGNUM is stored.  */

static int fxsave_offset[] =
{
  32,				/* %st(0) through ...  */
  48,
  64,
  80,
  96,
  112,
  128,
  144,				/* ... %st(7) (80 bits each).  */
  0,				/* `fctrl' (16 bits).  */
  2,				/* `fstat' (16 bits).  */
  4,				/* `ftag' (16 bits).  */
  12,				/* `fiseg' (16 bits).  */
  8,				/* `fioff'.  */
  20,				/* `foseg' (16 bits).  */
  16,				/* `fooff'.  */
  6,				/* `fop' (bottom 11 bits).  */
  160 + 0 * 16,			/* %xmm0 through ...  */
  160 + 1 * 16,
  160 + 2 * 16,
  160 + 3 * 16,
  160 + 4 * 16,
  160 + 5 * 16,
  160 + 6 * 16,
  160 + 7 * 16,
  160 + 8 * 16,
  160 + 9 * 16,
  160 + 10 * 16,
  160 + 11 * 16,
  160 + 12 * 16,
  160 + 13 * 16,
  160 + 14 * 16,
  160 + 15 * 16,		/* ... %xmm15 (128 bits each).  */
};

#define FXSAVE_ADDR(tdep, fxsave, regnum) \
  (fxsave + fxsave_offset[regnum - I387_ST0_REGNUM (tdep)])

/* We made an unfortunate choice in putting %mxcsr after the SSE
   registers %xmm0-%xmm7 instead of before, since it makes supporting
   the registers %xmm8-%xmm15 on AMD64 a bit involved.  Therefore we
   don't include the offset for %mxcsr here above.  */

#define FXSAVE_MXCSR_ADDR(fxsave) (fxsave + 24)

static int i387_tag (const gdb_byte *raw);


/* Fill register REGNUM in REGCACHE with the appropriate
   floating-point or SSE register value from *FXSAVE.  This function
   masks off any of the reserved bits in *FXSAVE.  */

void
i387_supply_fxsave (struct regcache *regcache, int regnum, const void *fxsave)
{
  gdbarch *arch = regcache->arch ();
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (arch);
  const gdb_byte *regs = (const gdb_byte *) fxsave;
  int i;

  gdb_assert (tdep->st0_regnum >= I386_ST0_REGNUM);
  gdb_assert (tdep->num_xmm_regs > 0);

  for (i = I387_ST0_REGNUM (tdep); i < I387_MXCSR_REGNUM (tdep); i++)
    if (regnum == -1 || regnum == i)
      {
	if (regs == NULL)
	  {
	    regcache->raw_supply (i, NULL);
	    continue;
	  }

	/* Most of the FPU control registers occupy only 16 bits in
	   the fxsave area.  Give those a special treatment.  */
	if (i >= I387_FCTRL_REGNUM (tdep) && i < I387_XMM0_REGNUM (tdep)
	    && i != I387_FIOFF_REGNUM (tdep) && i != I387_FOOFF_REGNUM (tdep))
	  {
	    gdb_byte val[4];

	    memcpy (val, FXSAVE_ADDR (tdep, regs, i), 2);
	    val[2] = val[3] = 0;
	    if (i == I387_FOP_REGNUM (tdep))
	      val[1] &= ((1 << 3) - 1);
	    else if (i== I387_FTAG_REGNUM (tdep))
	      {
		/* The fxsave area contains a simplified version of
		   the tag word.  We have to look at the actual 80-bit
		   FP data to recreate the traditional i387 tag word.  */

		unsigned long ftag = 0;
		int fpreg;
		int top;

		top = ((FXSAVE_ADDR (tdep, regs,
				     I387_FSTAT_REGNUM (tdep)))[1] >> 3);
		top &= 0x7;

		for (fpreg = 7; fpreg >= 0; fpreg--)
		  {
		    int tag;

		    if (val[0] & (1 << fpreg))
		      {
			int thisreg = (fpreg + 8 - top) % 8 
				       + I387_ST0_REGNUM (tdep);
			tag = i387_tag (FXSAVE_ADDR (tdep, regs, thisreg));
		      }
		    else
		      tag = 3;		/* Empty */

		    ftag |= tag << (2 * fpreg);
		  }
		val[0] = ftag & 0xff;
		val[1] = (ftag >> 8) & 0xff;
	      }
	    regcache->raw_supply (i, val);
	  }
	else
	  regcache->raw_supply (i, FXSAVE_ADDR (tdep, regs, i));
      }

  if (regnum == I387_MXCSR_REGNUM (tdep) || regnum == -1)
    {
      if (regs == NULL)
	regcache->raw_supply (I387_MXCSR_REGNUM (tdep), NULL);
      else
	regcache->raw_supply (I387_MXCSR_REGNUM (tdep),
			     FXSAVE_MXCSR_ADDR (regs));
    }
}

/* Fill register REGNUM (if it is a floating-point or SSE register) in
   *FXSAVE with the value from REGCACHE.  If REGNUM is -1, do this for
   all registers.  This function doesn't touch any of the reserved
   bits in *FXSAVE.  */

void
i387_collect_fxsave (const struct regcache *regcache, int regnum, void *fxsave)
{
  gdbarch *arch = regcache->arch ();
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (arch);
  gdb_byte *regs = (gdb_byte *) fxsave;
  int i;

  gdb_assert (tdep->st0_regnum >= I386_ST0_REGNUM);
  gdb_assert (tdep->num_xmm_regs > 0);

  for (i = I387_ST0_REGNUM (tdep); i < I387_MXCSR_REGNUM (tdep); i++)
    if (regnum == -1 || regnum == i)
      {
	/* Most of the FPU control registers occupy only 16 bits in
	   the fxsave area.  Give those a special treatment.  */
	if (i >= I387_FCTRL_REGNUM (tdep) && i < I387_XMM0_REGNUM (tdep)
	    && i != I387_FIOFF_REGNUM (tdep) && i != I387_FOOFF_REGNUM (tdep))
	  {
	    gdb_byte buf[4];

	    regcache->raw_collect (i, buf);

	    if (i == I387_FOP_REGNUM (tdep))
	      {
		/* The opcode occupies only 11 bits.  Make sure we
		   don't touch the other bits.  */
		buf[1] &= ((1 << 3) - 1);
		buf[1] |= ((FXSAVE_ADDR (tdep, regs, i))[1] & ~((1 << 3) - 1));
	      }
	    else if (i == I387_FTAG_REGNUM (tdep))
	      {
		/* Converting back is much easier.  */

		unsigned short ftag;
		int fpreg;

		ftag = (buf[1] << 8) | buf[0];
		buf[0] = 0;
		buf[1] = 0;

		for (fpreg = 7; fpreg >= 0; fpreg--)
		  {
		    int tag = (ftag >> (fpreg * 2)) & 3;

		    if (tag != 3)
		      buf[0] |= (1 << fpreg);
		  }
	      }
	    memcpy (FXSAVE_ADDR (tdep, regs, i), buf, 2);
	  }
	else
	  regcache->raw_collect (i, FXSAVE_ADDR (tdep, regs, i));
      }

  if (regnum == I387_MXCSR_REGNUM (tdep) || regnum == -1)
    regcache->raw_collect (I387_MXCSR_REGNUM (tdep),
			  FXSAVE_MXCSR_ADDR (regs));
}

/* `xstate_bv' is at byte offset 512.  */
#define XSAVE_XSTATE_BV_ADDR(xsave) (xsave + 512)

/* At xsave_avxh_offset[REGNUM] you'll find the relative offset within
   the AVX region of the XSAVE extended state where the upper 128bits
   of GDB register YMM0 + REGNUM is stored.  */

static int xsave_avxh_offset[] =
{
  0 * 16,		/* Upper 128bit of %ymm0 through ...  */
  1 * 16,
  2 * 16,
  3 * 16,
  4 * 16,
  5 * 16,
  6 * 16,
  7 * 16,
  8 * 16,
  9 * 16,
  10 * 16,
  11 * 16,
  12 * 16,
  13 * 16,
  14 * 16,
  15 * 16		/* Upper 128bit of ... %ymm15 (128 bits each).  */
};

#define XSAVE_AVXH_ADDR(tdep, xsave, regnum)			\
  (xsave + (tdep)->xsave_layout.avx_offset			\
   + xsave_avxh_offset[regnum - I387_YMM0H_REGNUM (tdep)])

/* At xsave_ymm_avx512_offset[REGNUM] you'll find the relative offset
   within the ZMM region of the XSAVE extended state where the second
   128bits of GDB register YMM16 + REGNUM is stored.  */

static int xsave_ymm_avx512_offset[] =
{
  16 + 0 * 64,		/* %ymm16 through...  */
  16 + 1 * 64,
  16 + 2 * 64,
  16 + 3 * 64,
  16 + 4 * 64,
  16 + 5 * 64,
  16 + 6 * 64,
  16 + 7 * 64,
  16 + 8 * 64,
  16 + 9 * 64,
  16 + 10 * 64,
  16 + 11 * 64,
  16 + 12 * 64,
  16 + 13 * 64,
  16 + 14 * 64,
  16 + 15 * 64		/* ...  %ymm31 (128 bits each).  */
};

#define XSAVE_YMM_AVX512_ADDR(tdep, xsave, regnum)			\
  (xsave + (tdep)->xsave_layout.zmm_offset				\
   + xsave_ymm_avx512_offset[regnum - I387_YMM16H_REGNUM (tdep)])

/* At xsave_xmm_avx512_offset[REGNUM] you'll find the relative offset
   within the ZMM region of the XSAVE extended state where the first
   128bits of GDB register XMM16 + REGNUM is stored.  */

static int xsave_xmm_avx512_offset[] =
{
  0 * 64,			/* %xmm16 through...  */
  1 * 64,
  2 * 64,
  3 * 64,
  4 * 64,
  5 * 64,
  6 * 64,
  7 * 64,
  8 * 64,
  9 * 64,
  10 * 64,
  11 * 64,
  12 * 64,
  13 * 64,
  14 * 64,
  15 * 64			/* ...  %xmm31 (128 bits each).  */
};

#define XSAVE_XMM_AVX512_ADDR(tdep, xsave, regnum)			\
  (xsave + (tdep)->xsave_layout.zmm_offset				\
   + xsave_xmm_avx512_offset[regnum - I387_XMM16_REGNUM (tdep)])

/* At xsave_bndregs_offset[REGNUM] you'll find the relative offset
   within the BNDREGS region of the XSAVE extended state where the GDB
   register BND0R + REGNUM is stored.  */

static int xsave_bndregs_offset[] = {
  0 * 16,			/* bnd0r...bnd3r registers.  */
  1 * 16,
  2 * 16,
  3 * 16
};

#define XSAVE_BNDREGS_ADDR(tdep, xsave, regnum)				\
  (xsave + (tdep)->xsave_layout.bndregs_offset				\
   + xsave_bndregs_offset[regnum - I387_BND0R_REGNUM (tdep)])

static int xsave_bndcfg_offset[] = {
  0 * 8,			/* bndcfg ... bndstatus.  */
  1 * 8,
};

#define XSAVE_BNDCFG_ADDR(tdep, xsave, regnum)			\
  (xsave + (tdep)->xsave_layout.bndcfg_offset			\
   + xsave_bndcfg_offset[regnum - I387_BNDCFGU_REGNUM (tdep)])

/* At xsave_avx512_k_offset[REGNUM] you'll find the relative offset
   within the K region of the XSAVE extended state where the AVX512
   opmask register K0 + REGNUM is stored.  */

static int xsave_avx512_k_offset[] =
{
  0 * 8,			/* %k0 through...  */
  1 * 8,
  2 * 8,
  3 * 8,
  4 * 8,
  5 * 8,
  6 * 8,
  7 * 8				/* %k7 (64 bits each).	*/
};

#define XSAVE_AVX512_K_ADDR(tdep, xsave, regnum)		\
  (xsave + (tdep)->xsave_layout.k_offset			\
   + xsave_avx512_k_offset[regnum - I387_K0_REGNUM (tdep)])


/* At xsave_avx512_zmm0_h_offset[REGNUM] you find the relative offset
   within the ZMM_H region of the XSAVE extended state where the upper
   256bits of the GDB register ZMM0 + REGNUM is stored.  */

static int xsave_avx512_zmm0_h_offset[] =
{
  0 * 32,		/* Upper 256bit of %zmmh0 through...  */
  1 * 32,
  2 * 32,
  3 * 32,
  4 * 32,
  5 * 32,
  6 * 32,
  7 * 32,
  8 * 32,
  9 * 32,
  10 * 32,
  11 * 32,
  12 * 32,
  13 * 32,
  14 * 32,
  15 * 32		/* Upper 256bit of...  %zmmh15 (256 bits each).  */
};

#define XSAVE_AVX512_ZMM0_H_ADDR(tdep, xsave, regnum)			\
  (xsave + (tdep)->xsave_layout.zmm_h_offset				\
   + xsave_avx512_zmm0_h_offset[regnum - I387_ZMM0H_REGNUM (tdep)])

/* At xsave_avx512_zmm16_h_offset[REGNUM] you find the relative offset
   within the ZMM_H region of the XSAVE extended state where the upper
   256bits of the GDB register ZMM16 + REGNUM is stored.  */

static int xsave_avx512_zmm16_h_offset[] =
{
  32 + 0 * 64,		/* Upper 256bit of...  %zmmh16 (256 bits each).  */
  32 + 1 * 64,
  32 + 2 * 64,
  32 + 3 * 64,
  32 + 4 * 64,
  32 + 5 * 64,
  32 + 6 * 64,
  32 + 7 * 64,
  32 + 8 * 64,
  32 + 9 * 64,
  32 + 10 * 64,
  32 + 11 * 64,
  32 + 12 * 64,
  32 + 13 * 64,
  32 + 14 * 64,
  32 + 15 * 64		/* Upper 256bit of... %zmmh31 (256 bits each).  */
};

#define XSAVE_AVX512_ZMM16_H_ADDR(tdep, xsave, regnum)			\
  (xsave + (tdep)->xsave_layout.zmm_offset				\
   + xsave_avx512_zmm16_h_offset[regnum - I387_ZMM16H_REGNUM (tdep)])

/* At xsave_pkeys_offset[REGNUM] you'll find the relative offset
   within the PKEYS region of the XSAVE extended state where the PKRU
   register is stored.  */

static int xsave_pkeys_offset[] =
{
  0 * 8			/* %pkru (64 bits in XSTATE, 32-bit actually used by
			   instructions and applications).  */
};

#define XSAVE_PKEYS_ADDR(tdep, xsave, regnum)				\
  (xsave + (tdep)->xsave_layout.pkru_offset				\
   + xsave_pkeys_offset[regnum - I387_PKRU_REGNUM (tdep)])


/* See i387-tdep.h.  */

bool
i387_guess_xsave_layout (uint64_t xcr0, size_t xsave_size,
			 x86_xsave_layout &layout)
{
  if (HAS_PKRU (xcr0) && xsave_size == 2696)
    {
      /* Intel CPUs supporting PKRU.  */
      layout.avx_offset = 576;
      layout.bndregs_offset = 960;
      layout.bndcfg_offset = 1024;
      layout.k_offset = 1088;
      layout.zmm_h_offset = 1152;
      layout.zmm_offset = 1664;
      layout.pkru_offset = 2688;
    }
  else if (HAS_PKRU (xcr0) && xsave_size == 2440)
    {
      /* AMD CPUs supporting PKRU.  */
      layout.avx_offset = 576;
      layout.k_offset = 832;
      layout.zmm_h_offset = 896;
      layout.zmm_offset = 1408;
      layout.pkru_offset = 2432;
    }
  else if (HAS_AVX512 (xcr0) && xsave_size == 2688)
    {
      /* Intel CPUs supporting AVX512.  */
      layout.avx_offset = 576;
      layout.bndregs_offset = 960;
      layout.bndcfg_offset = 1024;
      layout.k_offset = 1088;
      layout.zmm_h_offset = 1152;
      layout.zmm_offset = 1664;
    }
  else if (HAS_MPX (xcr0) && xsave_size == 1088)
    {
      /* Intel CPUs supporting MPX.  */
      layout.avx_offset = 576;
      layout.bndregs_offset = 960;
      layout.bndcfg_offset = 1024;
    }
  else if (HAS_AVX (xcr0) && xsave_size == 832)
    {
      /* Intel and AMD CPUs supporting AVX.  */
      layout.avx_offset = 576;
    }
  else
    return false;

  layout.sizeof_xsave = xsave_size;
  return true;
}

/* See i387-tdep.h.  */

x86_xsave_layout
i387_fallback_xsave_layout (uint64_t xcr0)
{
  x86_xsave_layout layout;

  if (HAS_PKRU (xcr0))
    {
      /* Intel CPUs supporting PKRU.  */
      layout.avx_offset = 576;
      layout.bndregs_offset = 960;
      layout.bndcfg_offset = 1024;
      layout.k_offset = 1088;
      layout.zmm_h_offset = 1152;
      layout.zmm_offset = 1664;
      layout.pkru_offset = 2688;
      layout.sizeof_xsave = 2696;
    }
  else if (HAS_AVX512 (xcr0))
    {
      /* Intel CPUs supporting AVX512.  */
      layout.avx_offset = 576;
      layout.bndregs_offset = 960;
      layout.bndcfg_offset = 1024;
      layout.k_offset = 1088;
      layout.zmm_h_offset = 1152;
      layout.zmm_offset = 1664;
      layout.sizeof_xsave = 2688;
    }
  else if (HAS_MPX (xcr0))
    {
      /* Intel CPUs supporting MPX.  */
      layout.avx_offset = 576;
      layout.bndregs_offset = 960;
      layout.bndcfg_offset = 1024;
      layout.sizeof_xsave = 1088;
    }
  else if (HAS_AVX (xcr0))
    {
      /* Intel and AMD CPUs supporting AVX.  */
      layout.avx_offset = 576;
      layout.sizeof_xsave = 832;
    }

  return layout;
}

/* Extract from XSAVE a bitset of the features that are available on the
   target, but which have not yet been enabled.  */

ULONGEST
i387_xsave_get_clear_bv (struct gdbarch *gdbarch, const void *xsave)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  const gdb_byte *regs = (const gdb_byte *) xsave;
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);

  /* Get `xstat_bv'.  The supported bits in `xstat_bv' are 8 bytes.  */
  ULONGEST xstate_bv = extract_unsigned_integer (XSAVE_XSTATE_BV_ADDR (regs),
						 8, byte_order);

  /* Clear part in vector registers if its bit in xstat_bv is zero.  */
  ULONGEST clear_bv = (~(xstate_bv)) & tdep->xcr0;

  return clear_bv;
}

/* Similar to i387_supply_fxsave, but use XSAVE extended state.  */

void
i387_supply_xsave (struct regcache *regcache, int regnum,
		   const void *xsave)
{
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);
  const gdb_byte *regs = (const gdb_byte *) xsave;
  int i;
  /* In 64-bit mode the split between "low" and "high" ZMM registers is at
     ZMM16.  Outside of 64-bit mode there are no "high" ZMM registers at all.
     Precalculate the number to be used for the split point, with the all
     registers in the "low" portion outside of 64-bit mode.  */
  unsigned int zmm_endlo_regnum = I387_ZMM0H_REGNUM (tdep)
				  + std::min (tdep->num_zmm_regs, 16);
  ULONGEST clear_bv;
  static const gdb_byte zero[I386_MAX_REGISTER_SIZE] = { 0 };
  enum
    {
      none = 0x0,
      x87 = 0x1,
      sse = 0x2,
      avxh = 0x4,
      bndregs = 0x8,
      bndcfg = 0x10,
      avx512_k = 0x20,
      avx512_zmm0_h = 0x40,
      avx512_zmm16_h = 0x80,
      avx512_ymmh_avx512 = 0x100,
      avx512_xmm_avx512 = 0x200,
      pkeys = 0x400,
      all = x87 | sse | avxh | bndregs | bndcfg | avx512_k | avx512_zmm0_h
	    | avx512_zmm16_h | avx512_ymmh_avx512 | avx512_xmm_avx512 | pkeys
    } regclass;

  gdb_assert (regs != NULL);
  gdb_assert (tdep->st0_regnum >= I386_ST0_REGNUM);
  gdb_assert (tdep->num_xmm_regs > 0);

  if (regnum == -1)
    regclass = all;
  else if (regnum >= I387_PKRU_REGNUM (tdep)
	   && regnum < I387_PKEYSEND_REGNUM (tdep))
    regclass = pkeys;
  else if (regnum >= I387_ZMM0H_REGNUM (tdep)
	   && regnum < I387_ZMM16H_REGNUM (tdep))
    regclass = avx512_zmm0_h;
  else if (regnum >= I387_ZMM16H_REGNUM (tdep)
	   && regnum < I387_ZMMENDH_REGNUM (tdep))
    regclass = avx512_zmm16_h;
  else if (regnum >= I387_K0_REGNUM (tdep)
	   && regnum < I387_KEND_REGNUM (tdep))
    regclass = avx512_k;
  else if (regnum >= I387_YMM16H_REGNUM (tdep)
	   && regnum < I387_YMMH_AVX512_END_REGNUM (tdep))
    regclass = avx512_ymmh_avx512;
  else if (regnum >= I387_XMM16_REGNUM (tdep)
	   && regnum < I387_XMM_AVX512_END_REGNUM (tdep))
    regclass = avx512_xmm_avx512;
  else if (regnum >= I387_YMM0H_REGNUM (tdep)
	   && regnum < I387_YMMENDH_REGNUM (tdep))
    regclass = avxh;
  else if (regnum >= I387_BND0R_REGNUM (tdep)
	   && regnum < I387_BNDCFGU_REGNUM (tdep))
    regclass = bndregs;
  else if (regnum >= I387_BNDCFGU_REGNUM (tdep)
	   && regnum < I387_MPXEND_REGNUM (tdep))
    regclass = bndcfg;
  else if (regnum >= I387_XMM0_REGNUM (tdep)
	   && regnum < I387_MXCSR_REGNUM (tdep))
    regclass = sse;
  else if (regnum >= I387_ST0_REGNUM (tdep)
	   && regnum < I387_FCTRL_REGNUM (tdep))
    regclass = x87;
  else
    regclass = none;

  clear_bv = i387_xsave_get_clear_bv (gdbarch, xsave);

  /* With the delayed xsave mechanism, in between the program
     starting, and the program accessing the vector registers for the
     first time, the register's values are invalid.  The kernel
     initializes register states to zero when they are set the first
     time in a program.  This means that from the user-space programs'
     perspective, it's the same as if the registers have always been
     zero from the start of the program.  Therefore, the debugger
     should provide the same illusion to the user.  */

  switch (regclass)
    {
    case none:
      break;

    case pkeys:
      if ((clear_bv & X86_XSTATE_PKRU))
	regcache->raw_supply (regnum, zero);
      else
	regcache->raw_supply (regnum, XSAVE_PKEYS_ADDR (tdep, regs, regnum));
      return;

    case avx512_zmm0_h:
      if ((clear_bv & X86_XSTATE_ZMM_H))
	regcache->raw_supply (regnum, zero);
      else
	regcache->raw_supply (regnum,
			      XSAVE_AVX512_ZMM0_H_ADDR (tdep, regs, regnum));
      return;

    case avx512_zmm16_h:
      if ((clear_bv & X86_XSTATE_ZMM))
	regcache->raw_supply (regnum, zero);
      else
	regcache->raw_supply (regnum,
			      XSAVE_AVX512_ZMM16_H_ADDR (tdep, regs, regnum));
      return;

    case avx512_k:
      if ((clear_bv & X86_XSTATE_K))
	regcache->raw_supply (regnum, zero);
      else
	regcache->raw_supply (regnum, XSAVE_AVX512_K_ADDR (tdep, regs, regnum));
      return;

    case avx512_ymmh_avx512:
      if ((clear_bv & X86_XSTATE_ZMM))
	regcache->raw_supply (regnum, zero);
      else
	regcache->raw_supply (regnum,
			      XSAVE_YMM_AVX512_ADDR (tdep, regs, regnum));
      return;

    case avx512_xmm_avx512:
      if ((clear_bv & X86_XSTATE_ZMM))
	regcache->raw_supply (regnum, zero);
      else
	regcache->raw_supply (regnum,
			      XSAVE_XMM_AVX512_ADDR (tdep, regs, regnum));
      return;

    case avxh:
      if ((clear_bv & X86_XSTATE_AVX))
	regcache->raw_supply (regnum, zero);
      else
	regcache->raw_supply (regnum, XSAVE_AVXH_ADDR (tdep, regs, regnum));
      return;

    case bndcfg:
      if ((clear_bv & X86_XSTATE_BNDCFG))
	regcache->raw_supply (regnum, zero);
      else
	regcache->raw_supply (regnum, XSAVE_BNDCFG_ADDR (tdep, regs, regnum));
      return;

    case bndregs:
      if ((clear_bv & X86_XSTATE_BNDREGS))
	regcache->raw_supply (regnum, zero);
      else
	regcache->raw_supply (regnum, XSAVE_BNDREGS_ADDR (tdep, regs, regnum));
      return;

    case sse:
      if ((clear_bv & X86_XSTATE_SSE))
	regcache->raw_supply (regnum, zero);
      else
	regcache->raw_supply (regnum, FXSAVE_ADDR (tdep, regs, regnum));
      return;

    case x87:
      if ((clear_bv & X86_XSTATE_X87))
	regcache->raw_supply (regnum, zero);
      else
	regcache->raw_supply (regnum, FXSAVE_ADDR (tdep, regs, regnum));
      return;

    case all:
      /* Handle PKEYS registers.  */
      if ((tdep->xcr0 & X86_XSTATE_PKRU))
	{
	  if ((clear_bv & X86_XSTATE_PKRU))
	    {
	      for (i = I387_PKRU_REGNUM (tdep);
		   i < I387_PKEYSEND_REGNUM (tdep);
		   i++)
		regcache->raw_supply (i, zero);
	    }
	  else
	    {
	      for (i = I387_PKRU_REGNUM (tdep);
		   i < I387_PKEYSEND_REGNUM (tdep);
		   i++)
		regcache->raw_supply (i, XSAVE_PKEYS_ADDR (tdep, regs, i));
	    }
	}

      /* Handle the upper halves of the low 8/16 ZMM registers.  */
      if ((tdep->xcr0 & X86_XSTATE_ZMM_H))
	{
	  if ((clear_bv & X86_XSTATE_ZMM_H))
	    {
	      for (i = I387_ZMM0H_REGNUM (tdep); i < zmm_endlo_regnum; i++)
		regcache->raw_supply (i, zero);
	    }
	  else
	    {
	      for (i = I387_ZMM0H_REGNUM (tdep); i < zmm_endlo_regnum; i++)
		regcache->raw_supply (i,
				      XSAVE_AVX512_ZMM0_H_ADDR (tdep, regs, i));
	    }
	}

      /* Handle AVX512 OpMask registers.  */
      if ((tdep->xcr0 & X86_XSTATE_K))
	{
	  if ((clear_bv & X86_XSTATE_K))
	    {
	      for (i = I387_K0_REGNUM (tdep);
		   i < I387_KEND_REGNUM (tdep);
		   i++)
		regcache->raw_supply (i, zero);
	    }
	  else
	    {
	      for (i = I387_K0_REGNUM (tdep);
		   i < I387_KEND_REGNUM (tdep);
		   i++)
		regcache->raw_supply (i, XSAVE_AVX512_K_ADDR (tdep, regs, i));
	    }
	}

      /* Handle the upper 16 ZMM/YMM/XMM registers (if any).  */
      if ((tdep->xcr0 & X86_XSTATE_ZMM))
	{
	  if ((clear_bv & X86_XSTATE_ZMM))
	    {
	      for (i = I387_ZMM16H_REGNUM (tdep);
		   i < I387_ZMMENDH_REGNUM (tdep); i++)
		regcache->raw_supply (i, zero);
	      for (i = I387_YMM16H_REGNUM (tdep);
		   i < I387_YMMH_AVX512_END_REGNUM (tdep);
		   i++)
		regcache->raw_supply (i, zero);
	      for (i = I387_XMM16_REGNUM (tdep);
		   i < I387_XMM_AVX512_END_REGNUM (tdep);
		   i++)
		regcache->raw_supply (i, zero);
	    }
	  else
	    {
	      for (i = I387_ZMM16H_REGNUM (tdep);
		   i < I387_ZMMENDH_REGNUM (tdep); i++)
		regcache->raw_supply (i,
				      XSAVE_AVX512_ZMM16_H_ADDR (tdep, regs, i));
	      for (i = I387_YMM16H_REGNUM (tdep);
		   i < I387_YMMH_AVX512_END_REGNUM (tdep);
		   i++)
		regcache->raw_supply (i, XSAVE_YMM_AVX512_ADDR (tdep, regs, i));
	      for (i = I387_XMM16_REGNUM (tdep);
		   i < I387_XMM_AVX512_END_REGNUM (tdep);
		   i++)
		regcache->raw_supply (i, XSAVE_XMM_AVX512_ADDR (tdep, regs, i));
	    }
	}
      /* Handle the upper YMM registers.  */
      if ((tdep->xcr0 & X86_XSTATE_AVX))
	{
	  if ((clear_bv & X86_XSTATE_AVX))
	    {
	      for (i = I387_YMM0H_REGNUM (tdep);
		   i < I387_YMMENDH_REGNUM (tdep);
		   i++)
		regcache->raw_supply (i, zero);
	    }
	  else
	    {
	      for (i = I387_YMM0H_REGNUM (tdep);
		   i < I387_YMMENDH_REGNUM (tdep);
		   i++)
		regcache->raw_supply (i, XSAVE_AVXH_ADDR (tdep, regs, i));
	    }
	}

      /* Handle the MPX registers.  */
      if ((tdep->xcr0 & X86_XSTATE_BNDREGS))
	{
	  if (clear_bv & X86_XSTATE_BNDREGS)
	    {
	      for (i = I387_BND0R_REGNUM (tdep);
		   i < I387_BNDCFGU_REGNUM (tdep); i++)
		regcache->raw_supply (i, zero);
	    }
	  else
	    {
	      for (i = I387_BND0R_REGNUM (tdep);
		   i < I387_BNDCFGU_REGNUM (tdep); i++)
		regcache->raw_supply (i, XSAVE_BNDREGS_ADDR (tdep, regs, i));
	    }
	}

      /* Handle the MPX registers.  */
      if ((tdep->xcr0 & X86_XSTATE_BNDCFG))
	{
	  if (clear_bv & X86_XSTATE_BNDCFG)
	    {
	      for (i = I387_BNDCFGU_REGNUM (tdep);
		   i < I387_MPXEND_REGNUM (tdep); i++)
		regcache->raw_supply (i, zero);
	    }
	  else
	    {
	      for (i = I387_BNDCFGU_REGNUM (tdep);
		   i < I387_MPXEND_REGNUM (tdep); i++)
		regcache->raw_supply (i, XSAVE_BNDCFG_ADDR (tdep, regs, i));
	    }
	}

      /* Handle the XMM registers.  */
      if ((tdep->xcr0 & X86_XSTATE_SSE))
	{
	  if ((clear_bv & X86_XSTATE_SSE))
	    {
	      for (i = I387_XMM0_REGNUM (tdep);
		   i < I387_MXCSR_REGNUM (tdep);
		   i++)
		regcache->raw_supply (i, zero);
	    }
	  else
	    {
	      for (i = I387_XMM0_REGNUM (tdep);
		   i < I387_MXCSR_REGNUM (tdep); i++)
		regcache->raw_supply (i, FXSAVE_ADDR (tdep, regs, i));
	    }
	}

      /* Handle the x87 registers.  */
      if ((tdep->xcr0 & X86_XSTATE_X87))
	{
	  if ((clear_bv & X86_XSTATE_X87))
	    {
	      for (i = I387_ST0_REGNUM (tdep);
		   i < I387_FCTRL_REGNUM (tdep);
		   i++)
		regcache->raw_supply (i, zero);
	    }
	  else
	    {
	      for (i = I387_ST0_REGNUM (tdep);
		   i < I387_FCTRL_REGNUM (tdep);
		   i++)
		regcache->raw_supply (i, FXSAVE_ADDR (tdep, regs, i));
	    }
	}
      break;
    }

  /* Only handle x87 control registers.  */
  for (i = I387_FCTRL_REGNUM (tdep); i < I387_XMM0_REGNUM (tdep); i++)
    if (regnum == -1 || regnum == i)
      {
	if (clear_bv & X86_XSTATE_X87)
	  {
	    if (i == I387_FCTRL_REGNUM (tdep))
	      {
		gdb_byte buf[4];

		store_unsigned_integer (buf, 4, byte_order,
					I387_FCTRL_INIT_VAL);
		regcache->raw_supply (i, buf);
	      }
	    else if (i == I387_FTAG_REGNUM (tdep))
	      {
		gdb_byte buf[4];

		store_unsigned_integer (buf, 4, byte_order, 0xffff);
		regcache->raw_supply (i, buf);
	      }
	    else
	      regcache->raw_supply (i, zero);
	  }
	/* Most of the FPU control registers occupy only 16 bits in
	   the xsave extended state.  Give those a special treatment.  */
	else if (i != I387_FIOFF_REGNUM (tdep)
		 && i != I387_FOOFF_REGNUM (tdep))
	  {
	    gdb_byte val[4];

	    memcpy (val, FXSAVE_ADDR (tdep, regs, i), 2);
	    val[2] = val[3] = 0;
	    if (i == I387_FOP_REGNUM (tdep))
	      val[1] &= ((1 << 3) - 1);
	    else if (i == I387_FTAG_REGNUM (tdep))
	      {
		/* The fxsave area contains a simplified version of
		   the tag word.  We have to look at the actual 80-bit
		   FP data to recreate the traditional i387 tag word.  */

		unsigned long ftag = 0;
		int fpreg;
		int top;

		top = ((FXSAVE_ADDR (tdep, regs,
				     I387_FSTAT_REGNUM (tdep)))[1] >> 3);
		top &= 0x7;

		for (fpreg = 7; fpreg >= 0; fpreg--)
		  {
		    int tag;

		    if (val[0] & (1 << fpreg))
		      {
			int thisreg = (fpreg + 8 - top) % 8 
				       + I387_ST0_REGNUM (tdep);
			tag = i387_tag (FXSAVE_ADDR (tdep, regs, thisreg));
		      }
		    else
		      tag = 3;		/* Empty */

		    ftag |= tag << (2 * fpreg);
		  }
		val[0] = ftag & 0xff;
		val[1] = (ftag >> 8) & 0xff;
	      }
	    regcache->raw_supply (i, val);
	  }
	else
	  regcache->raw_supply (i, FXSAVE_ADDR (tdep, regs, i));
      }

  if (regnum == I387_MXCSR_REGNUM (tdep) || regnum == -1)
    {
      /* The MXCSR register is placed into the xsave buffer if either the
	 AVX or SSE features are enabled.  */
      if ((clear_bv & (X86_XSTATE_AVX | X86_XSTATE_SSE))
	  == (X86_XSTATE_AVX | X86_XSTATE_SSE))
	{
	  gdb_byte buf[4];

	  store_unsigned_integer (buf, 4, byte_order, I387_MXCSR_INIT_VAL);
	  regcache->raw_supply (I387_MXCSR_REGNUM (tdep), buf);
	}
      else
	regcache->raw_supply (I387_MXCSR_REGNUM (tdep),
			      FXSAVE_MXCSR_ADDR (regs));
    }
}

/* Similar to i387_collect_fxsave, but use XSAVE extended state.  */

void
i387_collect_xsave (const struct regcache *regcache, int regnum,
		    void *xsave, int gcore)
{
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);
  gdb_byte *p, *regs = (gdb_byte *) xsave;
  gdb_byte raw[I386_MAX_REGISTER_SIZE];
  ULONGEST initial_xstate_bv, clear_bv, xstate_bv = 0;
  unsigned int i;
  /* See the comment in i387_supply_xsave().  */
  unsigned int zmm_endlo_regnum = I387_ZMM0H_REGNUM (tdep)
				  + std::min (tdep->num_zmm_regs, 16);
  enum
    {
      x87_ctrl_or_mxcsr = 0x1,
      x87 = 0x2,
      sse = 0x4,
      avxh = 0x8,
      bndregs = 0x10,
      bndcfg = 0x20,
      avx512_k = 0x40,
      avx512_zmm0_h = 0x80,
      avx512_zmm16_h = 0x100,
      avx512_ymmh_avx512 = 0x200,
      avx512_xmm_avx512 = 0x400,
      pkeys = 0x800,
      all = x87 | sse | avxh | bndregs | bndcfg | avx512_k | avx512_zmm0_h
	    | avx512_zmm16_h | avx512_ymmh_avx512 | avx512_xmm_avx512 | pkeys
    } regclass;

  gdb_assert (tdep->st0_regnum >= I386_ST0_REGNUM);
  gdb_assert (tdep->num_xmm_regs > 0);

  if (regnum == -1)
    regclass = all;
  else if (regnum >= I387_PKRU_REGNUM (tdep)
	   && regnum < I387_PKEYSEND_REGNUM (tdep))
    regclass = pkeys;
  else if (regnum >= I387_ZMM0H_REGNUM (tdep)
	   && regnum < I387_ZMM16H_REGNUM (tdep))
    regclass = avx512_zmm0_h;
  else if (regnum >= I387_ZMM16H_REGNUM (tdep)
	   && regnum < I387_ZMMENDH_REGNUM (tdep))
    regclass = avx512_zmm16_h;
  else if (regnum >= I387_K0_REGNUM (tdep)
	   && regnum < I387_KEND_REGNUM (tdep))
    regclass = avx512_k;
  else if (regnum >= I387_YMM16H_REGNUM (tdep)
	   && regnum < I387_YMMH_AVX512_END_REGNUM (tdep))
    regclass = avx512_ymmh_avx512;
  else if (regnum >= I387_XMM16_REGNUM (tdep)
	   && regnum < I387_XMM_AVX512_END_REGNUM (tdep))
    regclass = avx512_xmm_avx512;
  else if (regnum >= I387_YMM0H_REGNUM (tdep)
	   && regnum < I387_YMMENDH_REGNUM (tdep))
    regclass = avxh;
  else if (regnum >= I387_BND0R_REGNUM (tdep)
	   && regnum < I387_BNDCFGU_REGNUM (tdep))
    regclass = bndregs;
  else if (regnum >= I387_BNDCFGU_REGNUM (tdep)
	   && regnum < I387_MPXEND_REGNUM (tdep))
    regclass = bndcfg;
  else if (regnum >= I387_XMM0_REGNUM (tdep)
	   && regnum < I387_MXCSR_REGNUM (tdep))
    regclass = sse;
  else if (regnum >= I387_ST0_REGNUM (tdep)
	   && regnum < I387_FCTRL_REGNUM (tdep))
    regclass = x87;
  else if ((regnum >= I387_FCTRL_REGNUM (tdep)
	    && regnum < I387_XMM0_REGNUM (tdep))
	   || regnum == I387_MXCSR_REGNUM (tdep))
    regclass = x87_ctrl_or_mxcsr;
  else
    internal_error (_("invalid i387 regnum %d"), regnum);

  if (gcore)
    {
      /* Clear XSAVE extended state.  */
      memset (regs, 0, tdep->xsave_layout.sizeof_xsave);

      /* Update XCR0 and `xstate_bv' with XCR0 for gcore.  */
      if (tdep->xsave_xcr0_offset != -1)
	memcpy (regs + tdep->xsave_xcr0_offset, &tdep->xcr0, 8);
      memcpy (XSAVE_XSTATE_BV_ADDR (regs), &tdep->xcr0, 8);
    }

  /* The supported bits in `xstat_bv' are 8 bytes.  */
  initial_xstate_bv = extract_unsigned_integer (XSAVE_XSTATE_BV_ADDR (regs),
						8, byte_order);
  clear_bv = (~(initial_xstate_bv)) & tdep->xcr0;

  /* The XSAVE buffer was filled lazily by the kernel.  Only those
     features that are enabled were written into the buffer, disabled
     features left the buffer uninitialised.  In order to identify if any
     registers have changed we will be comparing the register cache
     version to the version in the XSAVE buffer, it is important then that
     at this point we initialise to the default values any features in
     XSAVE that are not yet initialised.

     This could be made more efficient, we know which features (from
     REGNUM) we will be potentially updating, and could limit ourselves to
     only clearing that feature.  However, the extra complexity does not
     seem justified at this point.  */
  if (clear_bv)
    {
      if ((clear_bv & X86_XSTATE_PKRU))
	for (i = I387_PKRU_REGNUM (tdep);
	     i < I387_PKEYSEND_REGNUM (tdep); i++)
	  memset (XSAVE_PKEYS_ADDR (tdep, regs, i), 0, 4);

      if ((clear_bv & X86_XSTATE_BNDREGS))
	for (i = I387_BND0R_REGNUM (tdep);
	     i < I387_BNDCFGU_REGNUM (tdep); i++)
	  memset (XSAVE_BNDREGS_ADDR (tdep, regs, i), 0, 16);

      if ((clear_bv & X86_XSTATE_BNDCFG))
	for (i = I387_BNDCFGU_REGNUM (tdep);
	     i < I387_MPXEND_REGNUM (tdep); i++)
	  memset (XSAVE_BNDCFG_ADDR (tdep, regs, i), 0, 8);

      if ((clear_bv & X86_XSTATE_ZMM_H))
	for (i = I387_ZMM0H_REGNUM (tdep); i < zmm_endlo_regnum; i++)
	  memset (XSAVE_AVX512_ZMM0_H_ADDR (tdep, regs, i), 0, 32);

      if ((clear_bv & X86_XSTATE_K))
	for (i = I387_K0_REGNUM (tdep);
	     i < I387_KEND_REGNUM (tdep); i++)
	  memset (XSAVE_AVX512_K_ADDR (tdep, regs, i), 0, 8);

      if ((clear_bv & X86_XSTATE_ZMM))
	{
	  for (i = I387_ZMM16H_REGNUM (tdep); i < I387_ZMMENDH_REGNUM (tdep);
	       i++)
	    memset (XSAVE_AVX512_ZMM16_H_ADDR (tdep, regs, i), 0, 32);
	  for (i = I387_YMM16H_REGNUM (tdep);
	       i < I387_YMMH_AVX512_END_REGNUM (tdep); i++)
	    memset (XSAVE_YMM_AVX512_ADDR (tdep, regs, i), 0, 16);
	  for (i = I387_XMM16_REGNUM (tdep);
	       i < I387_XMM_AVX512_END_REGNUM (tdep); i++)
	    memset (XSAVE_XMM_AVX512_ADDR (tdep, regs, i), 0, 16);
	}

      if ((clear_bv & X86_XSTATE_AVX))
	for (i = I387_YMM0H_REGNUM (tdep);
	     i < I387_YMMENDH_REGNUM (tdep); i++)
	  memset (XSAVE_AVXH_ADDR (tdep, regs, i), 0, 16);

      if ((clear_bv & X86_XSTATE_SSE))
	for (i = I387_XMM0_REGNUM (tdep);
	     i < I387_MXCSR_REGNUM (tdep); i++)
	  memset (FXSAVE_ADDR (tdep, regs, i), 0, 16);

      /* The mxcsr register is written into the xsave buffer if either AVX
	 or SSE is enabled, so only clear it if both of those features
	 require clearing.  */
      if ((clear_bv & (X86_XSTATE_AVX | X86_XSTATE_SSE))
	  == (X86_XSTATE_AVX | X86_XSTATE_SSE))
	store_unsigned_integer (FXSAVE_MXCSR_ADDR (regs), 2, byte_order,
				I387_MXCSR_INIT_VAL);

      if ((clear_bv & X86_XSTATE_X87))
	{
	  for (i = I387_ST0_REGNUM (tdep);
	       i < I387_FCTRL_REGNUM (tdep); i++)
	    memset (FXSAVE_ADDR (tdep, regs, i), 0, 10);

	  for (i = I387_FCTRL_REGNUM (tdep);
	       i < I387_XMM0_REGNUM (tdep); i++)
	    {
	      if (i == I387_FCTRL_REGNUM (tdep))
		store_unsigned_integer (FXSAVE_ADDR (tdep, regs, i), 2,
					byte_order, I387_FCTRL_INIT_VAL);
	      else
		memset (FXSAVE_ADDR (tdep, regs, i), 0,
			regcache_register_size (regcache, i));
	    }
	}
    }

  if (regclass == all)
    {
      /* Check if any PKEYS registers are changed.  */
      if ((tdep->xcr0 & X86_XSTATE_PKRU))
	for (i = I387_PKRU_REGNUM (tdep);
	     i < I387_PKEYSEND_REGNUM (tdep); i++)
	  {
	    regcache->raw_collect (i, raw);
	    p = XSAVE_PKEYS_ADDR (tdep, regs, i);
	    if (memcmp (raw, p, 4) != 0)
	      {
		xstate_bv |= X86_XSTATE_PKRU;
		memcpy (p, raw, 4);
	      }
	  }

      /* Check if any ZMMH registers are changed.  */
      if ((tdep->xcr0 & X86_XSTATE_ZMM))
	for (i = I387_ZMM16H_REGNUM (tdep);
	     i < I387_ZMMENDH_REGNUM (tdep); i++)
	  {
	    regcache->raw_collect (i, raw);
	    p = XSAVE_AVX512_ZMM16_H_ADDR (tdep, regs, i);
	    if (memcmp (raw, p, 32) != 0)
	      {
		xstate_bv |= X86_XSTATE_ZMM;
		memcpy (p, raw, 32);
	      }
	  }

      if ((tdep->xcr0 & X86_XSTATE_ZMM_H))
	for (i = I387_ZMM0H_REGNUM (tdep); i < zmm_endlo_regnum; i++)
	  {
	    regcache->raw_collect (i, raw);
	    p = XSAVE_AVX512_ZMM0_H_ADDR (tdep, regs, i);
	    if (memcmp (raw, p, 32) != 0)
	      {
		xstate_bv |= X86_XSTATE_ZMM_H;
		memcpy (p, raw, 32);
	      }
	  }

      /* Check if any K registers are changed.  */
      if ((tdep->xcr0 & X86_XSTATE_K))
	for (i = I387_K0_REGNUM (tdep);
	     i < I387_KEND_REGNUM (tdep); i++)
	  {
	    regcache->raw_collect (i, raw);
	    p = XSAVE_AVX512_K_ADDR (tdep, regs, i);
	    if (memcmp (raw, p, 8) != 0)
	      {
		xstate_bv |= X86_XSTATE_K;
		memcpy (p, raw, 8);
	      }
	  }

      /* Check if any XMM or upper YMM registers are changed.  */
      if ((tdep->xcr0 & X86_XSTATE_ZMM))
	{
	  for (i = I387_YMM16H_REGNUM (tdep);
	       i < I387_YMMH_AVX512_END_REGNUM (tdep); i++)
	    {
	      regcache->raw_collect (i, raw);
	      p = XSAVE_YMM_AVX512_ADDR (tdep, regs, i);
	      if (memcmp (raw, p, 16) != 0)
		{
		  xstate_bv |= X86_XSTATE_ZMM;
		  memcpy (p, raw, 16);
		}
	    }
	  for (i = I387_XMM16_REGNUM (tdep);
	       i < I387_XMM_AVX512_END_REGNUM (tdep); i++)
	    {
	      regcache->raw_collect (i, raw);
	      p = XSAVE_XMM_AVX512_ADDR (tdep, regs, i);
	      if (memcmp (raw, p, 16) != 0)
		{
		  xstate_bv |= X86_XSTATE_ZMM;
		  memcpy (p, raw, 16);
		}
	    }
	}

      /* Check if any upper MPX registers are changed.  */
      if ((tdep->xcr0 & X86_XSTATE_BNDREGS))
	for (i = I387_BND0R_REGNUM (tdep);
	     i < I387_BNDCFGU_REGNUM (tdep); i++)
	  {
	    regcache->raw_collect (i, raw);
	    p = XSAVE_BNDREGS_ADDR (tdep, regs, i);
	    if (memcmp (raw, p, 16))
	      {
		xstate_bv |= X86_XSTATE_BNDREGS;
		memcpy (p, raw, 16);
	      }
	  }

      /* Check if any upper MPX registers are changed.  */
      if ((tdep->xcr0 & X86_XSTATE_BNDCFG))
	for (i = I387_BNDCFGU_REGNUM (tdep);
	     i < I387_MPXEND_REGNUM (tdep); i++)
	  {
	    regcache->raw_collect (i, raw);
	    p = XSAVE_BNDCFG_ADDR (tdep, regs, i);
	    if (memcmp (raw, p, 8))
	      {
		xstate_bv |= X86_XSTATE_BNDCFG;
		memcpy (p, raw, 8);
	      }
	  }

      /* Check if any upper YMM registers are changed.  */
      if ((tdep->xcr0 & X86_XSTATE_AVX))
	for (i = I387_YMM0H_REGNUM (tdep);
	     i < I387_YMMENDH_REGNUM (tdep); i++)
	  {
	    regcache->raw_collect (i, raw);
	    p = XSAVE_AVXH_ADDR (tdep, regs, i);
	    if (memcmp (raw, p, 16))
	      {
		xstate_bv |= X86_XSTATE_AVX;
		memcpy (p, raw, 16);
	      }
	  }

      /* Check if any SSE registers are changed.  */
      if ((tdep->xcr0 & X86_XSTATE_SSE))
	for (i = I387_XMM0_REGNUM (tdep);
	     i < I387_MXCSR_REGNUM (tdep); i++)
	  {
	    regcache->raw_collect (i, raw);
	    p = FXSAVE_ADDR (tdep, regs, i);
	    if (memcmp (raw, p, 16))
	      {
		xstate_bv |= X86_XSTATE_SSE;
		memcpy (p, raw, 16);
	      }
	  }

      if ((tdep->xcr0 & X86_XSTATE_AVX) || (tdep->xcr0 & X86_XSTATE_SSE))
	{
	  i = I387_MXCSR_REGNUM (tdep);
	  regcache->raw_collect (i, raw);
	  p = FXSAVE_MXCSR_ADDR (regs);
	  if (memcmp (raw, p, 4))
	    {
	      /* Now, we need to mark one of either SSE of AVX as enabled.
		 We could pick either.  What we do is check to see if one
		 of the features is already enabled, if it is then we leave
		 it at that, otherwise we pick SSE.  */
	      if ((xstate_bv & (X86_XSTATE_SSE | X86_XSTATE_AVX)) == 0)
		xstate_bv |= X86_XSTATE_SSE;
	      memcpy (p, raw, 4);
	    }
	}

      /* Check if any X87 registers are changed.  Only the non-control
	 registers are handled here, the control registers are all handled
	 later on in this function.  */
      if ((tdep->xcr0 & X86_XSTATE_X87))
	for (i = I387_ST0_REGNUM (tdep);
	     i < I387_FCTRL_REGNUM (tdep); i++)
	  {
	    regcache->raw_collect (i, raw);
	    p = FXSAVE_ADDR (tdep, regs, i);
	    if (memcmp (raw, p, 10))
	      {
		xstate_bv |= X86_XSTATE_X87;
		memcpy (p, raw, 10);
	      }
	  }
    }
  else
    {
      /* Check if REGNUM is changed.  */
      regcache->raw_collect (regnum, raw);

      switch (regclass)
	{
	default:
	  internal_error (_("invalid i387 regclass"));

	case pkeys:
	  /* This is a PKEYS register.  */
	  p = XSAVE_PKEYS_ADDR (tdep, regs, regnum);
	  if (memcmp (raw, p, 4) != 0)
	    {
	      xstate_bv |= X86_XSTATE_PKRU;
	      memcpy (p, raw, 4);
	    }
	  break;

	case avx512_zmm16_h:
	  /* This is a ZMM16-31 register.  */
	  p = XSAVE_AVX512_ZMM16_H_ADDR (tdep, regs, regnum);
	  if (memcmp (raw, p, 32) != 0)
	    {
	      xstate_bv |= X86_XSTATE_ZMM;
	      memcpy (p, raw, 32);
	    }
	  break;

	case avx512_zmm0_h:
	  /* This is a ZMM0-15 register.  */
	  p = XSAVE_AVX512_ZMM0_H_ADDR (tdep, regs, regnum);
	  if (memcmp (raw, p, 32) != 0)
	    {
	      xstate_bv |= X86_XSTATE_ZMM_H;
	      memcpy (p, raw, 32);
	    }
	  break;

	case avx512_k:
	  /* This is a AVX512 mask register.  */
	  p = XSAVE_AVX512_K_ADDR (tdep, regs, regnum);
	  if (memcmp (raw, p, 8) != 0)
	    {
	      xstate_bv |= X86_XSTATE_K;
	      memcpy (p, raw, 8);
	    }
	  break;

	case avx512_ymmh_avx512:
	  /* This is an upper YMM16-31 register.  */
	  p = XSAVE_YMM_AVX512_ADDR (tdep, regs, regnum);
	  if (memcmp (raw, p, 16) != 0)
	    {
	      xstate_bv |= X86_XSTATE_ZMM;
	      memcpy (p, raw, 16);
	    }
	  break;

	case avx512_xmm_avx512:
	  /* This is an upper XMM16-31 register.  */
	  p = XSAVE_XMM_AVX512_ADDR (tdep, regs, regnum);
	  if (memcmp (raw, p, 16) != 0)
	    {
	      xstate_bv |= X86_XSTATE_ZMM;
	      memcpy (p, raw, 16);
	    }
	  break;

	case avxh:
	  /* This is an upper YMM register.  */
	  p = XSAVE_AVXH_ADDR (tdep, regs, regnum);
	  if (memcmp (raw, p, 16))
	    {
	      xstate_bv |= X86_XSTATE_AVX;
	      memcpy (p, raw, 16);
	    }
	  break;

	case bndregs:
	  regcache->raw_collect (regnum, raw);
	  p = XSAVE_BNDREGS_ADDR (tdep, regs, regnum);
	  if (memcmp (raw, p, 16))
	    {
	      xstate_bv |= X86_XSTATE_BNDREGS;
	      memcpy (p, raw, 16);
	    }
	  break;

	case bndcfg:
	  p = XSAVE_BNDCFG_ADDR (tdep, regs, regnum);
	  xstate_bv |= X86_XSTATE_BNDCFG;
	  memcpy (p, raw, 8);
	  break;

	case sse:
	  /* This is an SSE register.  */
	  p = FXSAVE_ADDR (tdep, regs, regnum);
	  if (memcmp (raw, p, 16))
	    {
	      xstate_bv |= X86_XSTATE_SSE;
	      memcpy (p, raw, 16);
	    }
	  break;

	case x87:
	  /* This is an x87 register.  */
	  p = FXSAVE_ADDR (tdep, regs, regnum);
	  if (memcmp (raw, p, 10))
	    {
	      xstate_bv |= X86_XSTATE_X87;
	      memcpy (p, raw, 10);
	    }
	  break;

	case x87_ctrl_or_mxcsr:
	  /* We only handle MXCSR here.  All other x87 control registers
	     are handled separately below.  */
	  if (regnum == I387_MXCSR_REGNUM (tdep))
	    {
	      p = FXSAVE_MXCSR_ADDR (regs);
	      if (memcmp (raw, p, 2))
		{
		  /* We're only setting MXCSR, so check the initial state
		     to see if either of AVX or SSE are already enabled.
		     If they are then we'll attribute this changed MXCSR to
		     that feature.  If neither feature is enabled, then
		     we'll attribute this change to the SSE feature.  */
		  xstate_bv |= (initial_xstate_bv
				& (X86_XSTATE_AVX | X86_XSTATE_SSE));
		  if ((xstate_bv & (X86_XSTATE_AVX | X86_XSTATE_SSE)) == 0)
		    xstate_bv |= X86_XSTATE_SSE;
		  memcpy (p, raw, 2);
		}
	    }
	}
    }

  /* Only handle x87 control registers.  */
  for (i = I387_FCTRL_REGNUM (tdep); i < I387_XMM0_REGNUM (tdep); i++)
    if (regnum == -1 || regnum == i)
      {
	/* Most of the FPU control registers occupy only 16 bits in
	   the xsave extended state.  Give those a special treatment.  */
	if (i != I387_FIOFF_REGNUM (tdep)
	    && i != I387_FOOFF_REGNUM (tdep))
	  {
	    gdb_byte buf[4];

	    regcache->raw_collect (i, buf);

	    if (i == I387_FOP_REGNUM (tdep))
	      {
		/* The opcode occupies only 11 bits.  Make sure we
		   don't touch the other bits.  */
		buf[1] &= ((1 << 3) - 1);
		buf[1] |= ((FXSAVE_ADDR (tdep, regs, i))[1] & ~((1 << 3) - 1));
	      }
	    else if (i == I387_FTAG_REGNUM (tdep))
	      {
		/* Converting back is much easier.  */

		unsigned short ftag;
		int fpreg;

		ftag = (buf[1] << 8) | buf[0];
		buf[0] = 0;
		buf[1] = 0;

		for (fpreg = 7; fpreg >= 0; fpreg--)
		  {
		    int tag = (ftag >> (fpreg * 2)) & 3;

		    if (tag != 3)
		      buf[0] |= (1 << fpreg);
		  }
	      }
	    p = FXSAVE_ADDR (tdep, regs, i);
	    if (memcmp (p, buf, 2))
	      {
		xstate_bv |= X86_XSTATE_X87;
		memcpy (p, buf, 2);
	      }
	  }
	else
	  {
	    int regsize;

	    regcache->raw_collect (i, raw);
	    regsize = regcache_register_size (regcache, i);
	    p = FXSAVE_ADDR (tdep, regs, i);
	    if (memcmp (raw, p, regsize))
	      {
		xstate_bv |= X86_XSTATE_X87;
		memcpy (p, raw, regsize);
	      }
	  }
      }

  /* Update the corresponding bits in `xstate_bv' if any
     registers are changed.  */
  if (xstate_bv)
    {
      /* The supported bits in `xstat_bv' are 8 bytes.  */
      initial_xstate_bv |= xstate_bv;
      store_unsigned_integer (XSAVE_XSTATE_BV_ADDR (regs),
			      8, byte_order,
			      initial_xstate_bv);
    }
}

/* Recreate the FTW (tag word) valid bits from the 80-bit FP data in
   *RAW.  */

static int
i387_tag (const gdb_byte *raw)
{
  int integer;
  unsigned int exponent;
  unsigned long fraction[2];

  integer = raw[7] & 0x80;
  exponent = (((raw[9] & 0x7f) << 8) | raw[8]);
  fraction[0] = ((raw[3] << 24) | (raw[2] << 16) | (raw[1] << 8) | raw[0]);
  fraction[1] = (((raw[7] & 0x7f) << 24) | (raw[6] << 16)
		 | (raw[5] << 8) | raw[4]);

  if (exponent == 0x7fff)
    {
      /* Special.  */
      return (2);
    }
  else if (exponent == 0x0000)
    {
      if (fraction[0] == 0x0000 && fraction[1] == 0x0000 && !integer)
	{
	  /* Zero.  */
	  return (1);
	}
      else
	{
	  /* Special.  */
	  return (2);
	}
    }
  else
    {
      if (integer)
	{
	  /* Valid.  */
	  return (0);
	}
      else
	{
	  /* Special.  */
	  return (2);
	}
    }
}

/* Prepare the FPU stack in REGCACHE for a function return.  */

void
i387_return_value (struct gdbarch *gdbarch, struct regcache *regcache)
{
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);
  ULONGEST fstat;

  /* Set the top of the floating-point register stack to 7.  The
     actual value doesn't really matter, but 7 is what a normal
     function return would end up with if the program started out with
     a freshly initialized FPU.  */
  regcache_raw_read_unsigned (regcache, I387_FSTAT_REGNUM (tdep), &fstat);
  fstat |= (7 << 11);
  regcache_raw_write_unsigned (regcache, I387_FSTAT_REGNUM (tdep), fstat);

  /* Mark %st(1) through %st(7) as empty.  Since we set the top of the
     floating-point register stack to 7, the appropriate value for the
     tag word is 0x3fff.  */
  regcache_raw_write_unsigned (regcache, I387_FTAG_REGNUM (tdep), 0x3fff);

}

/* See i387-tdep.h.  */

void
i387_reset_bnd_regs (struct gdbarch *gdbarch, struct regcache *regcache)
{
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);

  if (I387_BND0R_REGNUM (tdep) > 0)
    {
      gdb_byte bnd_buf[16];

      memset (bnd_buf, 0, 16);
      for (int i = 0; i < I387_NUM_BND_REGS; i++)
	regcache->raw_write (I387_BND0R_REGNUM (tdep) + i, bnd_buf);
    }
}
