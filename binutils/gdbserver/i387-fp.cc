/* i387-specific utility functions, for the remote server for GDB.
   Copyright (C) 2000-2024 Free Software Foundation, Inc.

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

#include "server.h"
#include "i387-fp.h"
#include "gdbsupport/x86-xstate.h"
#include "nat/x86-xstate.h"

/* Default to SSE.  */
static unsigned long long x86_xcr0 = X86_XSTATE_SSE_MASK;

static const int num_mpx_bnd_registers = 4;
static const int num_mpx_cfg_registers = 2;
static const int num_avx512_k_registers = 8;
static const int num_pkeys_registers = 1;

static x86_xsave_layout xsave_layout;

/* Note: These functions preserve the reserved bits in control registers.
   However, gdbserver promptly throws away that information.  */

/* These structs should have the proper sizes and alignment on both
   i386 and x86-64 machines.  */

struct i387_fsave
{
  /* All these are only sixteen bits, plus padding, except for fop (which
     is only eleven bits), and fooff / fioff (which are 32 bits each).  */
  unsigned short fctrl;
  unsigned short pad1;
  unsigned short fstat;
  unsigned short pad2;
  unsigned short ftag;
  unsigned short pad3;
  unsigned int fioff;
  unsigned short fiseg;
  unsigned short fop;
  unsigned int fooff;
  unsigned short foseg;
  unsigned short pad4;

  /* Space for eight 80-bit FP values.  */
  unsigned char st_space[80];
};

struct i387_fxsave
{
  /* All these are only sixteen bits, plus padding, except for fop (which
     is only eleven bits), and fooff / fioff (which are 32 bits each).  */
  unsigned short fctrl;
  unsigned short fstat;
  unsigned short ftag;
  unsigned short fop;
  unsigned int fioff;
  unsigned short fiseg;
  unsigned short pad1;
  unsigned int fooff;
  unsigned short foseg;
  unsigned short pad12;

  unsigned int mxcsr;
  unsigned int pad3;

  /* Space for eight 80-bit FP values in 128-bit spaces.  */
  unsigned char st_space[128];

  /* Space for eight 128-bit XMM values, or 16 on x86-64.  */
  unsigned char xmm_space[256];
};

static_assert (sizeof(i387_fxsave) == 416);

struct i387_xsave : public i387_fxsave
{
  unsigned char reserved1[48];

  /* The extended control register 0 (the XFEATURE_ENABLED_MASK
     register).  */
  unsigned long long xcr0;

  unsigned char reserved2[40];

  /* The XSTATE_BV bit vector.  */
  unsigned long long xstate_bv;

  /* The XCOMP_BV bit vector.  */
  unsigned long long xcomp_bv;

  unsigned char reserved3[48];

  /* Byte 576.  End of registers with fixed position in XSAVE.
     The position of other XSAVE registers will be calculated
     from the appropriate CPUID calls.  */

private:
  /* Base address of XSAVE data as an unsigned char *.  Used to derive
     pointers to XSAVE state components in the extended state
     area.  */
  unsigned char *xsave ()
  { return reinterpret_cast<unsigned char *> (this); }

public:
  /* Memory address of eight upper 128-bit YMM values, or 16 on x86-64.  */
  unsigned char *ymmh_space ()
  { return xsave () + xsave_layout.avx_offset; }

  /* Memory address of 4 bound registers values of 128 bits.  */
  unsigned char *bndregs_space ()
  { return xsave () + xsave_layout.bndregs_offset; }

  /* Memory address of 2 MPX configuration registers of 64 bits
     plus reserved space.  */
  unsigned char *bndcfg_space ()
  { return xsave () + xsave_layout.bndcfg_offset; }

  /* Memory address of 8 OpMask register values of 64 bits.  */
  unsigned char *k_space ()
  { return xsave () + xsave_layout.k_offset; }

  /* Memory address of 16 256-bit zmm0-15.  */
  unsigned char *zmmh_space ()
  { return xsave () + xsave_layout.zmm_h_offset; }

  /* Memory address of 16 512-bit zmm16-31 values.  */
  unsigned char *zmm16_space ()
  { return xsave () + xsave_layout.zmm_offset; }

  /* Memory address of 1 32-bit PKRU register.  The HW XSTATE size for this
     feature is actually 64 bits, but WRPKRU/RDPKRU instructions ignore upper
     32 bits.  */
  unsigned char *pkru_space ()
  { return xsave () + xsave_layout.pkru_offset; }
};

static_assert (sizeof(i387_xsave) == 576);

void
i387_cache_to_fsave (struct regcache *regcache, void *buf)
{
  struct i387_fsave *fp = (struct i387_fsave *) buf;
  int i;
  int st0_regnum = find_regno (regcache->tdesc, "st0");
  unsigned long val2;

  for (i = 0; i < 8; i++)
    collect_register (regcache, i + st0_regnum,
		      ((char *) &fp->st_space[0]) + i * 10);

  fp->fioff = regcache_raw_get_unsigned_by_name (regcache, "fioff");
  fp->fooff = regcache_raw_get_unsigned_by_name (regcache, "fooff");

  /* This one's 11 bits... */
  val2 = regcache_raw_get_unsigned_by_name (regcache, "fop");
  fp->fop = (val2 & 0x7FF) | (fp->fop & 0xF800);

  /* Some registers are 16-bit.  */
  fp->fctrl = regcache_raw_get_unsigned_by_name (regcache, "fctrl");
  fp->fstat = regcache_raw_get_unsigned_by_name (regcache, "fstat");
  fp->ftag = regcache_raw_get_unsigned_by_name (regcache, "ftag");
  fp->fiseg = regcache_raw_get_unsigned_by_name (regcache, "fiseg");
  fp->foseg = regcache_raw_get_unsigned_by_name (regcache, "foseg");
}

void
i387_fsave_to_cache (struct regcache *regcache, const void *buf)
{
  struct i387_fsave *fp = (struct i387_fsave *) buf;
  int i;
  int st0_regnum = find_regno (regcache->tdesc, "st0");
  unsigned long val;

  for (i = 0; i < 8; i++)
    supply_register (regcache, i + st0_regnum,
		     ((char *) &fp->st_space[0]) + i * 10);

  supply_register_by_name (regcache, "fioff", &fp->fioff);
  supply_register_by_name (regcache, "fooff", &fp->fooff);

  /* Some registers are 16-bit.  */
  val = fp->fctrl & 0xFFFF;
  supply_register_by_name (regcache, "fctrl", &val);

  val = fp->fstat & 0xFFFF;
  supply_register_by_name (regcache, "fstat", &val);

  val = fp->ftag & 0xFFFF;
  supply_register_by_name (regcache, "ftag", &val);

  val = fp->fiseg & 0xFFFF;
  supply_register_by_name (regcache, "fiseg", &val);

  val = fp->foseg & 0xFFFF;
  supply_register_by_name (regcache, "foseg", &val);

  /* fop has only 11 valid bits.  */
  val = (fp->fop) & 0x7FF;
  supply_register_by_name (regcache, "fop", &val);
}

void
i387_cache_to_fxsave (struct regcache *regcache, void *buf)
{
  struct i387_fxsave *fp = (struct i387_fxsave *) buf;
  int i;
  int st0_regnum = find_regno (regcache->tdesc, "st0");
  int xmm0_regnum = find_regno (regcache->tdesc, "xmm0");
  unsigned long val, val2;
  /* Amd64 has 16 xmm regs; I386 has 8 xmm regs.  */
  int num_xmm_registers = register_size (regcache->tdesc, 0) == 8 ? 16 : 8;

  for (i = 0; i < 8; i++)
    collect_register (regcache, i + st0_regnum,
		      ((char *) &fp->st_space[0]) + i * 16);
  for (i = 0; i < num_xmm_registers; i++)
    collect_register (regcache, i + xmm0_regnum,
		      ((char *) &fp->xmm_space[0]) + i * 16);

  fp->fioff = regcache_raw_get_unsigned_by_name (regcache, "fioff");
  fp->fooff = regcache_raw_get_unsigned_by_name (regcache, "fooff");
  fp->mxcsr = regcache_raw_get_unsigned_by_name (regcache, "mxcsr");

  /* This one's 11 bits... */
  val2 = regcache_raw_get_unsigned_by_name (regcache, "fop");
  fp->fop = (val2 & 0x7FF) | (fp->fop & 0xF800);

  /* Some registers are 16-bit.  */
  fp->fctrl = regcache_raw_get_unsigned_by_name (regcache, "fctrl");
  fp->fstat = regcache_raw_get_unsigned_by_name (regcache, "fstat");

  /* Convert to the simplifed tag form stored in fxsave data.  */
  val = regcache_raw_get_unsigned_by_name (regcache, "ftag");
  val2 = 0;
  for (i = 7; i >= 0; i--)
    {
      int tag = (val >> (i * 2)) & 3;

      if (tag != 3)
	val2 |= (1 << i);
    }
  fp->ftag = val2;

  fp->fiseg = regcache_raw_get_unsigned_by_name (regcache, "fiseg");
  fp->foseg = regcache_raw_get_unsigned_by_name (regcache, "foseg");
}

void
i387_cache_to_xsave (struct regcache *regcache, void *buf)
{
  struct i387_xsave *fp = (struct i387_xsave *) buf;
  bool amd64 = register_size (regcache->tdesc, 0) == 8;
  int i;
  unsigned long val, val2;
  unsigned long long xstate_bv = 0;
  unsigned long long clear_bv = 0;
  char raw[64];
  unsigned char *p;

  /* Amd64 has 16 xmm regs; I386 has 8 xmm regs.  */
  int num_xmm_registers = amd64 ? 16 : 8;
  /* AVX512 adds 16 extra ZMM regs in Amd64 mode, but none in I386 mode.*/
  int num_zmm_high_registers = amd64 ? 16 : 0;

  /* The supported bits in `xstat_bv' are 8 bytes.  Clear part in
     vector registers if its bit in xstat_bv is zero.  */
  clear_bv = (~fp->xstate_bv) & x86_xcr0;

  /* Clear part in x87 and vector registers if its bit in xstat_bv is
     zero.  */
  if (clear_bv)
    {
      if ((clear_bv & X86_XSTATE_X87))
	{
	  for (i = 0; i < 8; i++)
	    memset (((char *) &fp->st_space[0]) + i * 16, 0, 10);

	  fp->fioff = 0;
	  fp->fooff = 0;
	  fp->fctrl = I387_FCTRL_INIT_VAL;
	  fp->fstat = 0;
	  fp->ftag = 0;
	  fp->fiseg = 0;
	  fp->foseg = 0;
	  fp->fop = 0;
	}

      if ((clear_bv & X86_XSTATE_SSE))
	for (i = 0; i < num_xmm_registers; i++)
	  memset (((char *) &fp->xmm_space[0]) + i * 16, 0, 16);

      if ((clear_bv & X86_XSTATE_AVX))
	for (i = 0; i < num_xmm_registers; i++)
	  memset (fp->ymmh_space () + i * 16, 0, 16);

      if ((clear_bv & X86_XSTATE_SSE) && (clear_bv & X86_XSTATE_AVX))
	memset (((char *) &fp->mxcsr), 0, 4);

      if ((clear_bv & X86_XSTATE_BNDREGS))
	for (i = 0; i < num_mpx_bnd_registers; i++)
	  memset (fp->bndregs_space () + i * 16, 0, 16);

      if ((clear_bv & X86_XSTATE_BNDCFG))
	for (i = 0; i < num_mpx_cfg_registers; i++)
	  memset (fp->bndcfg_space () + i * 8, 0, 8);

      if ((clear_bv & X86_XSTATE_K))
	for (i = 0; i < num_avx512_k_registers; i++)
	  memset (fp->k_space () + i * 8, 0, 8);

      if ((clear_bv & X86_XSTATE_ZMM_H))
	for (i = 0; i < num_xmm_registers; i++)
	  memset (fp->zmmh_space () + i * 32, 0, 32);

      if ((clear_bv & X86_XSTATE_ZMM))
	for (i = 0; i < num_zmm_high_registers; i++)
	  memset (fp->zmm16_space () + i * 64, 0, 64);

      if ((clear_bv & X86_XSTATE_PKRU))
	for (i = 0; i < num_pkeys_registers; i++)
	  memset (fp->pkru_space () + i * 4, 0, 4);
    }

  /* Check if any x87 registers are changed.  */
  if ((x86_xcr0 & X86_XSTATE_X87))
    {
      int st0_regnum = find_regno (regcache->tdesc, "st0");

      for (i = 0; i < 8; i++)
	{
	  collect_register (regcache, i + st0_regnum, raw);
	  p = fp->st_space + i * 16;
	  if (memcmp (raw, p, 10))
	    {
	      xstate_bv |= X86_XSTATE_X87;
	      memcpy (p, raw, 10);
	    }
	}
    }

  /* Check if any SSE registers are changed.  */
  if ((x86_xcr0 & X86_XSTATE_SSE))
    {
      int xmm0_regnum = find_regno (regcache->tdesc, "xmm0");

      for (i = 0; i < num_xmm_registers; i++) 
	{
	  collect_register (regcache, i + xmm0_regnum, raw);
	  p = fp->xmm_space + i * 16;
	  if (memcmp (raw, p, 16))
	    {
	      xstate_bv |= X86_XSTATE_SSE;
	      memcpy (p, raw, 16);
	    }
	}
    }

  /* Check if any AVX registers are changed.  */
  if ((x86_xcr0 & X86_XSTATE_AVX))
    {
      int ymm0h_regnum = find_regno (regcache->tdesc, "ymm0h");

      for (i = 0; i < num_xmm_registers; i++) 
	{
	  collect_register (regcache, i + ymm0h_regnum, raw);
	  p = fp->ymmh_space () + i * 16;
	  if (memcmp (raw, p, 16))
	    {
	      xstate_bv |= X86_XSTATE_AVX;
	      memcpy (p, raw, 16);
	    }
	}
    }

  /* Check if any bound register has changed.  */
  if ((x86_xcr0 & X86_XSTATE_BNDREGS))
    {
     int bnd0r_regnum = find_regno (regcache->tdesc, "bnd0raw");

      for (i = 0; i < num_mpx_bnd_registers; i++)
	{
	  collect_register (regcache, i + bnd0r_regnum, raw);
	  p = fp->bndregs_space () + i * 16;
	  if (memcmp (raw, p, 16))
	    {
	      xstate_bv |= X86_XSTATE_BNDREGS;
	      memcpy (p, raw, 16);
	    }
	}
    }

  /* Check if any status register has changed.  */
  if ((x86_xcr0 & X86_XSTATE_BNDCFG))
    {
      int bndcfg_regnum = find_regno (regcache->tdesc, "bndcfgu");

      for (i = 0; i < num_mpx_cfg_registers; i++)
	{
	  collect_register (regcache, i + bndcfg_regnum, raw);
	  p = fp->bndcfg_space () + i * 8;
	  if (memcmp (raw, p, 8))
	    {
	      xstate_bv |= X86_XSTATE_BNDCFG;
	      memcpy (p, raw, 8);
	    }
	}
    }

  /* Check if any K registers are changed.  */
  if ((x86_xcr0 & X86_XSTATE_K))
    {
      int k0_regnum = find_regno (regcache->tdesc, "k0");

      for (i = 0; i < num_avx512_k_registers; i++)
	{
	  collect_register (regcache, i + k0_regnum, raw);
	  p = fp->k_space () + i * 8;
	  if (memcmp (raw, p, 8) != 0)
	    {
	      xstate_bv |= X86_XSTATE_K;
	      memcpy (p, raw, 8);
	    }
	}
    }

  /* Check if any of ZMM0H-ZMM15H registers are changed.  */
  if ((x86_xcr0 & X86_XSTATE_ZMM_H))
    {
      int zmm0h_regnum = find_regno (regcache->tdesc, "zmm0h");

      for (i = 0; i < num_xmm_registers; i++)
	{
	  collect_register (regcache, i + zmm0h_regnum, raw);
	  p = fp->zmmh_space () + i * 32;
	  if (memcmp (raw, p, 32) != 0)
	    {
	      xstate_bv |= X86_XSTATE_ZMM_H;
	      memcpy (p, raw, 32);
	    }
	}
    }

  /* Check if any of ZMM16-ZMM31 registers are changed.  */
  if ((x86_xcr0 & X86_XSTATE_ZMM) && num_zmm_high_registers != 0)
    {
      int zmm16h_regnum = find_regno (regcache->tdesc, "zmm16h");
      int ymm16h_regnum = find_regno (regcache->tdesc, "ymm16h");
      int xmm16_regnum = find_regno (regcache->tdesc, "xmm16");

      for (i = 0; i < num_zmm_high_registers; i++)
	{
	  p = fp->zmm16_space () + i * 64;

	  /* ZMMH sub-register.  */
	  collect_register (regcache, i + zmm16h_regnum, raw);
	  if (memcmp (raw, p + 32, 32) != 0)
	    {
	      xstate_bv |= X86_XSTATE_ZMM;
	      memcpy (p + 32, raw, 32);
	    }

	  /* YMMH sub-register.  */
	  collect_register (regcache, i + ymm16h_regnum, raw);
	  if (memcmp (raw, p + 16, 16) != 0)
	    {
	      xstate_bv |= X86_XSTATE_ZMM;
	      memcpy (p + 16, raw, 16);
	    }

	  /* XMM sub-register.  */
	  collect_register (regcache, i + xmm16_regnum, raw);
	  if (memcmp (raw, p, 16) != 0)
	    {
	      xstate_bv |= X86_XSTATE_ZMM;
	      memcpy (p, raw, 16);
	    }
	}
    }

  /* Check if any PKEYS registers are changed.  */
  if ((x86_xcr0 & X86_XSTATE_PKRU))
    {
      int pkru_regnum = find_regno (regcache->tdesc, "pkru");

      for (i = 0; i < num_pkeys_registers; i++)
	{
	  collect_register (regcache, i + pkru_regnum, raw);
	  p = fp->pkru_space () + i * 4;
	  if (memcmp (raw, p, 4) != 0)
	    {
	      xstate_bv |= X86_XSTATE_PKRU;
	      memcpy (p, raw, 4);
	    }
	}
    }

  if ((x86_xcr0 & X86_XSTATE_SSE) || (x86_xcr0 & X86_XSTATE_AVX))
    {
      collect_register_by_name (regcache, "mxcsr", raw);
      if (memcmp (raw, &fp->mxcsr, 4) != 0)
	{
	  if (((fp->xstate_bv | xstate_bv)
	       & (X86_XSTATE_SSE | X86_XSTATE_AVX)) == 0)
	    xstate_bv |= X86_XSTATE_SSE;
	  memcpy (&fp->mxcsr, raw, 4);
	}
    }

  if (x86_xcr0 & X86_XSTATE_X87)
    {
      collect_register_by_name (regcache, "fioff", raw);
      if (memcmp (raw, &fp->fioff, 4) != 0)
	{
	  xstate_bv |= X86_XSTATE_X87;
	  memcpy (&fp->fioff, raw, 4);
	}

      collect_register_by_name (regcache, "fooff", raw);
      if (memcmp (raw, &fp->fooff, 4) != 0)
	{
	  xstate_bv |= X86_XSTATE_X87;
	  memcpy (&fp->fooff, raw, 4);
	}

      /* This one's 11 bits... */
      val2 = regcache_raw_get_unsigned_by_name (regcache, "fop");
      val2 = (val2 & 0x7FF) | (fp->fop & 0xF800);
      if (fp->fop != val2)
	{
	  xstate_bv |= X86_XSTATE_X87;
	  fp->fop = val2;
	}

      /* Some registers are 16-bit.  */
      val = regcache_raw_get_unsigned_by_name (regcache, "fctrl");
      if (fp->fctrl != val)
	{
	  xstate_bv |= X86_XSTATE_X87;
	  fp->fctrl = val;
	}

      val = regcache_raw_get_unsigned_by_name (regcache, "fstat");
      if (fp->fstat != val)
	{
	  xstate_bv |= X86_XSTATE_X87;
	  fp->fstat = val;
	}

      /* Convert to the simplifed tag form stored in fxsave data.  */
      val = regcache_raw_get_unsigned_by_name (regcache, "ftag");
      val2 = 0;
      for (i = 7; i >= 0; i--)
	{
	  int tag = (val >> (i * 2)) & 3;

	  if (tag != 3)
	    val2 |= (1 << i);
	}
      if (fp->ftag != val2)
	{
	  xstate_bv |= X86_XSTATE_X87;
	  fp->ftag = val2;
	}

      val = regcache_raw_get_unsigned_by_name (regcache, "fiseg");
      if (fp->fiseg != val)
	{
	  xstate_bv |= X86_XSTATE_X87;
	  fp->fiseg = val;
	}

      val = regcache_raw_get_unsigned_by_name (regcache, "foseg");
      if (fp->foseg != val)
	{
	  xstate_bv |= X86_XSTATE_X87;
	  fp->foseg = val;
	}
    }

  /* Update the corresponding bits in xstate_bv if any SSE/AVX
     registers are changed.  */
  fp->xstate_bv |= xstate_bv;
}

static int
i387_ftag (struct i387_fxsave *fp, int regno)
{
  unsigned char *raw = &fp->st_space[regno * 16];
  unsigned int exponent;
  unsigned long fraction[2];
  int integer;

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

void
i387_fxsave_to_cache (struct regcache *regcache, const void *buf)
{
  struct i387_fxsave *fp = (struct i387_fxsave *) buf;
  int i, top;
  int st0_regnum = find_regno (regcache->tdesc, "st0");
  int xmm0_regnum = find_regno (regcache->tdesc, "xmm0");
  unsigned long val;
  /* Amd64 has 16 xmm regs; I386 has 8 xmm regs.  */
  int num_xmm_registers = register_size (regcache->tdesc, 0) == 8 ? 16 : 8;

  for (i = 0; i < 8; i++)
    supply_register (regcache, i + st0_regnum,
		     ((char *) &fp->st_space[0]) + i * 16);
  for (i = 0; i < num_xmm_registers; i++)
    supply_register (regcache, i + xmm0_regnum,
		     ((char *) &fp->xmm_space[0]) + i * 16);

  supply_register_by_name (regcache, "fioff", &fp->fioff);
  supply_register_by_name (regcache, "fooff", &fp->fooff);
  supply_register_by_name (regcache, "mxcsr", &fp->mxcsr);

  /* Some registers are 16-bit.  */
  val = fp->fctrl & 0xFFFF;
  supply_register_by_name (regcache, "fctrl", &val);

  val = fp->fstat & 0xFFFF;
  supply_register_by_name (regcache, "fstat", &val);

  /* Generate the form of ftag data that GDB expects.  */
  top = (fp->fstat >> 11) & 0x7;
  val = 0;
  for (i = 7; i >= 0; i--)
    {
      int tag;
      if (fp->ftag & (1 << i))
	tag = i387_ftag (fp, (i + 8 - top) % 8);
      else
	tag = 3;
      val |= tag << (2 * i);
    }
  supply_register_by_name (regcache, "ftag", &val);

  val = fp->fiseg & 0xFFFF;
  supply_register_by_name (regcache, "fiseg", &val);

  val = fp->foseg & 0xFFFF;
  supply_register_by_name (regcache, "foseg", &val);

  val = (fp->fop) & 0x7FF;
  supply_register_by_name (regcache, "fop", &val);
}

void
i387_xsave_to_cache (struct regcache *regcache, const void *buf)
{
  struct i387_xsave *fp = (struct i387_xsave *) buf;
  bool amd64 = register_size (regcache->tdesc, 0) == 8;
  int i, top;
  unsigned long val;
  unsigned long long clear_bv;
  unsigned char *p;

   /* Amd64 has 16 xmm regs; I386 has 8 xmm regs.  */
  int num_xmm_registers = amd64 ? 16 : 8;
  /* AVX512 adds 16 extra ZMM regs in Amd64 mode, but none in I386 mode.*/
  int num_zmm_high_registers = amd64 ? 16 : 0;

  /* The supported bits in `xstat_bv' are 8 bytes.  Clear part in
     vector registers if its bit in xstat_bv is zero.  */
  clear_bv = (~fp->xstate_bv) & x86_xcr0;

  /* Check if any x87 registers are changed.  */
  if ((x86_xcr0 & X86_XSTATE_X87) != 0)
    {
      int st0_regnum = find_regno (regcache->tdesc, "st0");

      if ((clear_bv & X86_XSTATE_X87) != 0)
	{
	  for (i = 0; i < 8; i++)
	    supply_register_zeroed (regcache, i + st0_regnum);
	}
      else
	{
	  p = (gdb_byte *) &fp->st_space[0];
	  for (i = 0; i < 8; i++)
	    supply_register (regcache, i + st0_regnum, p + i * 16);
	}
    }

  if ((x86_xcr0 & X86_XSTATE_SSE) != 0)
    {
      int xmm0_regnum = find_regno (regcache->tdesc, "xmm0");

      if ((clear_bv & X86_XSTATE_SSE))
	{
	  for (i = 0; i < num_xmm_registers; i++)
	    supply_register_zeroed (regcache, i + xmm0_regnum);
	}
      else
	{
	  p = (gdb_byte *) &fp->xmm_space[0];
	  for (i = 0; i < num_xmm_registers; i++)
	    supply_register (regcache, i + xmm0_regnum, p + i * 16);
	}
    }

  if ((x86_xcr0 & X86_XSTATE_AVX) != 0)
    {
      int ymm0h_regnum = find_regno (regcache->tdesc, "ymm0h");

      if ((clear_bv & X86_XSTATE_AVX) != 0)
	{
	  for (i = 0; i < num_xmm_registers; i++)
	    supply_register_zeroed (regcache, i + ymm0h_regnum);
	}
      else
	{
	  p = fp->ymmh_space ();
	  for (i = 0; i < num_xmm_registers; i++)
	    supply_register (regcache, i + ymm0h_regnum, p + i * 16);
	}
    }

  if ((x86_xcr0 & X86_XSTATE_BNDREGS))
    {
      int bnd0r_regnum = find_regno (regcache->tdesc, "bnd0raw");


      if ((clear_bv & X86_XSTATE_BNDREGS) != 0)
	{
	  for (i = 0; i < num_mpx_bnd_registers; i++)
	    supply_register_zeroed (regcache, i + bnd0r_regnum);
	}
      else
	{
	  p = fp->bndregs_space ();
	  for (i = 0; i < num_mpx_bnd_registers; i++)
	    supply_register (regcache, i + bnd0r_regnum, p + i * 16);
	}

    }

  if ((x86_xcr0 & X86_XSTATE_BNDCFG))
    {
      int bndcfg_regnum = find_regno (regcache->tdesc, "bndcfgu");

      if ((clear_bv & X86_XSTATE_BNDCFG) != 0)
	{
	  for (i = 0; i < num_mpx_cfg_registers; i++)
	    supply_register_zeroed (regcache, i + bndcfg_regnum);
	}
      else
	{
	  p = fp->bndcfg_space ();
	  for (i = 0; i < num_mpx_cfg_registers; i++)
	    supply_register (regcache, i + bndcfg_regnum, p + i * 8);
	}
    }

  if ((x86_xcr0 & X86_XSTATE_K) != 0)
    {
      int k0_regnum = find_regno (regcache->tdesc, "k0");

      if ((clear_bv & X86_XSTATE_K) != 0)
	{
	  for (i = 0; i < num_avx512_k_registers; i++)
	    supply_register_zeroed (regcache, i + k0_regnum);
	}
      else
	{
	  p = fp->k_space ();
	  for (i = 0; i < num_avx512_k_registers; i++)
	    supply_register (regcache, i + k0_regnum, p + i * 8);
	}
    }

  if ((x86_xcr0 & X86_XSTATE_ZMM_H) != 0)
    {
      int zmm0h_regnum = find_regno (regcache->tdesc, "zmm0h");

      if ((clear_bv & X86_XSTATE_ZMM_H) != 0)
	{
	  for (i = 0; i < num_xmm_registers; i++)
	    supply_register_zeroed (regcache, i + zmm0h_regnum);
	}
      else
	{
	  p = fp->zmmh_space ();
	  for (i = 0; i < num_xmm_registers; i++)
	    supply_register (regcache, i + zmm0h_regnum, p + i * 32);
	}
    }

  if ((x86_xcr0 & X86_XSTATE_ZMM) != 0 && num_zmm_high_registers != 0)
    {
      int zmm16h_regnum = find_regno (regcache->tdesc, "zmm16h");
      int ymm16h_regnum = find_regno (regcache->tdesc, "ymm16h");
      int xmm16_regnum = find_regno (regcache->tdesc, "xmm16");

      if ((clear_bv & X86_XSTATE_ZMM) != 0)
	{
	  for (i = 0; i < num_zmm_high_registers; i++)
	    {
	      supply_register_zeroed (regcache, i + zmm16h_regnum);
	      supply_register_zeroed (regcache, i + ymm16h_regnum);
	      supply_register_zeroed (regcache, i + xmm16_regnum);
	    }
	}
      else
	{
	  p = fp->zmm16_space ();
	  for (i = 0; i < num_zmm_high_registers; i++)
	    {
	      supply_register (regcache, i + zmm16h_regnum, p + 32 + i * 64);
	      supply_register (regcache, i + ymm16h_regnum, p + 16 + i * 64);
	      supply_register (regcache, i + xmm16_regnum, p + i * 64);
	    }
	}
    }

  if ((x86_xcr0 & X86_XSTATE_PKRU) != 0)
    {
      int pkru_regnum = find_regno (regcache->tdesc, "pkru");

      if ((clear_bv & X86_XSTATE_PKRU) != 0)
	{
	  for (i = 0; i < num_pkeys_registers; i++)
	    supply_register_zeroed (regcache, i + pkru_regnum);
	}
      else
	{
	  p = fp->pkru_space ();
	  for (i = 0; i < num_pkeys_registers; i++)
	    supply_register (regcache, i + pkru_regnum, p + i * 4);
	}
    }

  if ((clear_bv & (X86_XSTATE_SSE | X86_XSTATE_AVX))
      == (X86_XSTATE_SSE | X86_XSTATE_AVX))
    {
      unsigned int default_mxcsr = I387_MXCSR_INIT_VAL;
      supply_register_by_name (regcache, "mxcsr", &default_mxcsr);
    }
  else
    supply_register_by_name (regcache, "mxcsr", &fp->mxcsr);

  if ((clear_bv & X86_XSTATE_X87) != 0)
    {
      supply_register_by_name_zeroed (regcache, "fioff");
      supply_register_by_name_zeroed (regcache, "fooff");

      val = I387_FCTRL_INIT_VAL;
      supply_register_by_name (regcache, "fctrl", &val);

      supply_register_by_name_zeroed (regcache, "fstat");

      val = 0xFFFF;
      supply_register_by_name (regcache, "ftag", &val);

      supply_register_by_name_zeroed (regcache, "fiseg");
      supply_register_by_name_zeroed (regcache, "foseg");
      supply_register_by_name_zeroed (regcache, "fop");
    }
  else
    {
      supply_register_by_name (regcache, "fioff", &fp->fioff);
      supply_register_by_name (regcache, "fooff", &fp->fooff);

      /* Some registers are 16-bit.  */
      val = fp->fctrl & 0xFFFF;
      supply_register_by_name (regcache, "fctrl", &val);

      val = fp->fstat & 0xFFFF;
      supply_register_by_name (regcache, "fstat", &val);

      /* Generate the form of ftag data that GDB expects.  */
      top = (fp->fstat >> 11) & 0x7;
      val = 0;
      for (i = 7; i >= 0; i--)
	{
	  int tag;
	  if (fp->ftag & (1 << i))
	    tag = i387_ftag (fp, (i + 8 - top) % 8);
	  else
	    tag = 3;
	  val |= tag << (2 * i);
	}
      supply_register_by_name (regcache, "ftag", &val);

      val = fp->fiseg & 0xFFFF;
      supply_register_by_name (regcache, "fiseg", &val);

      val = fp->foseg & 0xFFFF;
      supply_register_by_name (regcache, "foseg", &val);

      val = (fp->fop) & 0x7FF;
      supply_register_by_name (regcache, "fop", &val);
    }
}

/* See i387-fp.h.  */

void
i387_set_xsave_mask (uint64_t xcr0, int len)
{
  x86_xcr0 = xcr0;
  xsave_layout = x86_fetch_xsave_layout (xcr0, len);
}
