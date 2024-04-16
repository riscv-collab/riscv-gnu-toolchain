/* Target-dependent code for AMD64.

   Copyright (C) 2001-2024 Free Software Foundation, Inc.

   Contributed by Jiri Smid, SuSE Labs.

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
#include "language.h"
#include "opcode/i386.h"
#include "dis-asm.h"
#include "arch-utils.h"
#include "dummy-frame.h"
#include "frame.h"
#include "frame-base.h"
#include "frame-unwind.h"
#include "inferior.h"
#include "infrun.h"
#include "gdbcmd.h"
#include "gdbcore.h"
#include "objfiles.h"
#include "regcache.h"
#include "regset.h"
#include "symfile.h"
#include "disasm.h"
#include "amd64-tdep.h"
#include "i387-tdep.h"
#include "gdbsupport/x86-xstate.h"
#include <algorithm>
#include "target-descriptions.h"
#include "arch/amd64.h"
#include "producer.h"
#include "ax.h"
#include "ax-gdb.h"
#include "gdbsupport/byte-vector.h"
#include "osabi.h"
#include "x86-tdep.h"
#include "amd64-ravenscar-thread.h"

/* Note that the AMD64 architecture was previously known as x86-64.
   The latter is (forever) engraved into the canonical system name as
   returned by config.guess, and used as the name for the AMD64 port
   of GNU/Linux.  The BSD's have renamed their ports to amd64; they
   don't like to shout.  For GDB we prefer the amd64_-prefix over the
   x86_64_-prefix since it's so much easier to type.  */

/* Register information.  */

static const char * const amd64_register_names[] = 
{
  "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp",

  /* %r8 is indeed register number 8.  */
  "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
  "rip", "eflags", "cs", "ss", "ds", "es", "fs", "gs",

  /* %st0 is register number 24.  */
  "st0", "st1", "st2", "st3", "st4", "st5", "st6", "st7",
  "fctrl", "fstat", "ftag", "fiseg", "fioff", "foseg", "fooff", "fop",

  /* %xmm0 is register number 40.  */
  "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7",
  "xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15",
  "mxcsr",
};

static const char * const amd64_ymm_names[] = 
{
  "ymm0", "ymm1", "ymm2", "ymm3",
  "ymm4", "ymm5", "ymm6", "ymm7",
  "ymm8", "ymm9", "ymm10", "ymm11",
  "ymm12", "ymm13", "ymm14", "ymm15"
};

static const char * const amd64_ymm_avx512_names[] =
{
  "ymm16", "ymm17", "ymm18", "ymm19",
  "ymm20", "ymm21", "ymm22", "ymm23",
  "ymm24", "ymm25", "ymm26", "ymm27",
  "ymm28", "ymm29", "ymm30", "ymm31"
};

static const char * const amd64_ymmh_names[] = 
{
  "ymm0h", "ymm1h", "ymm2h", "ymm3h",
  "ymm4h", "ymm5h", "ymm6h", "ymm7h",
  "ymm8h", "ymm9h", "ymm10h", "ymm11h",
  "ymm12h", "ymm13h", "ymm14h", "ymm15h"
};

static const char * const amd64_ymmh_avx512_names[] =
{
  "ymm16h", "ymm17h", "ymm18h", "ymm19h",
  "ymm20h", "ymm21h", "ymm22h", "ymm23h",
  "ymm24h", "ymm25h", "ymm26h", "ymm27h",
  "ymm28h", "ymm29h", "ymm30h", "ymm31h"
};

static const char * const amd64_mpx_names[] =
{
  "bnd0raw", "bnd1raw", "bnd2raw", "bnd3raw", "bndcfgu", "bndstatus"
};

static const char * const amd64_k_names[] =
{
  "k0", "k1", "k2", "k3",
  "k4", "k5", "k6", "k7"
};

static const char * const amd64_zmmh_names[] =
{
  "zmm0h", "zmm1h", "zmm2h", "zmm3h",
  "zmm4h", "zmm5h", "zmm6h", "zmm7h",
  "zmm8h", "zmm9h", "zmm10h", "zmm11h",
  "zmm12h", "zmm13h", "zmm14h", "zmm15h",
  "zmm16h", "zmm17h", "zmm18h", "zmm19h",
  "zmm20h", "zmm21h", "zmm22h", "zmm23h",
  "zmm24h", "zmm25h", "zmm26h", "zmm27h",
  "zmm28h", "zmm29h", "zmm30h", "zmm31h"
};

static const char * const amd64_zmm_names[] =
{
  "zmm0", "zmm1", "zmm2", "zmm3",
  "zmm4", "zmm5", "zmm6", "zmm7",
  "zmm8", "zmm9", "zmm10", "zmm11",
  "zmm12", "zmm13", "zmm14", "zmm15",
  "zmm16", "zmm17", "zmm18", "zmm19",
  "zmm20", "zmm21", "zmm22", "zmm23",
  "zmm24", "zmm25", "zmm26", "zmm27",
  "zmm28", "zmm29", "zmm30", "zmm31"
};

static const char * const amd64_xmm_avx512_names[] = {
    "xmm16",  "xmm17",  "xmm18",  "xmm19",
    "xmm20",  "xmm21",  "xmm22",  "xmm23",
    "xmm24",  "xmm25",  "xmm26",  "xmm27",
    "xmm28",  "xmm29",  "xmm30",  "xmm31"
};

static const char * const amd64_pkeys_names[] = {
    "pkru"
};

/* DWARF Register Number Mapping as defined in the System V psABI,
   section 3.6.  */

static int amd64_dwarf_regmap[] =
{
  /* General Purpose Registers RAX, RDX, RCX, RBX, RSI, RDI.  */
  AMD64_RAX_REGNUM, AMD64_RDX_REGNUM,
  AMD64_RCX_REGNUM, AMD64_RBX_REGNUM,
  AMD64_RSI_REGNUM, AMD64_RDI_REGNUM,

  /* Frame Pointer Register RBP.  */
  AMD64_RBP_REGNUM,

  /* Stack Pointer Register RSP.  */
  AMD64_RSP_REGNUM,

  /* Extended Integer Registers 8 - 15.  */
  AMD64_R8_REGNUM,		/* %r8 */
  AMD64_R9_REGNUM,		/* %r9 */
  AMD64_R10_REGNUM,		/* %r10 */
  AMD64_R11_REGNUM,		/* %r11 */
  AMD64_R12_REGNUM,		/* %r12 */
  AMD64_R13_REGNUM,		/* %r13 */
  AMD64_R14_REGNUM,		/* %r14 */
  AMD64_R15_REGNUM,		/* %r15 */

  /* Return Address RA.  Mapped to RIP.  */
  AMD64_RIP_REGNUM,

  /* SSE Registers 0 - 7.  */
  AMD64_XMM0_REGNUM + 0, AMD64_XMM1_REGNUM,
  AMD64_XMM0_REGNUM + 2, AMD64_XMM0_REGNUM + 3,
  AMD64_XMM0_REGNUM + 4, AMD64_XMM0_REGNUM + 5,
  AMD64_XMM0_REGNUM + 6, AMD64_XMM0_REGNUM + 7,

  /* Extended SSE Registers 8 - 15.  */
  AMD64_XMM0_REGNUM + 8, AMD64_XMM0_REGNUM + 9,
  AMD64_XMM0_REGNUM + 10, AMD64_XMM0_REGNUM + 11,
  AMD64_XMM0_REGNUM + 12, AMD64_XMM0_REGNUM + 13,
  AMD64_XMM0_REGNUM + 14, AMD64_XMM0_REGNUM + 15,

  /* Floating Point Registers 0-7.  */
  AMD64_ST0_REGNUM + 0, AMD64_ST0_REGNUM + 1,
  AMD64_ST0_REGNUM + 2, AMD64_ST0_REGNUM + 3,
  AMD64_ST0_REGNUM + 4, AMD64_ST0_REGNUM + 5,
  AMD64_ST0_REGNUM + 6, AMD64_ST0_REGNUM + 7,

  /* MMX Registers 0 - 7.
     We have to handle those registers specifically, as their register
     number within GDB depends on the target (or they may even not be
     available at all).  */
  -1, -1, -1, -1, -1, -1, -1, -1,

  /* Control and Status Flags Register.  */
  AMD64_EFLAGS_REGNUM,

  /* Selector Registers.  */
  AMD64_ES_REGNUM,
  AMD64_CS_REGNUM,
  AMD64_SS_REGNUM,
  AMD64_DS_REGNUM,
  AMD64_FS_REGNUM,
  AMD64_GS_REGNUM,
  -1,
  -1,

  /* Segment Base Address Registers.  */
  -1,
  -1,
  -1,
  -1,

  /* Special Selector Registers.  */
  -1,
  -1,

  /* Floating Point Control Registers.  */
  AMD64_MXCSR_REGNUM,
  AMD64_FCTRL_REGNUM,
  AMD64_FSTAT_REGNUM
};

static const int amd64_dwarf_regmap_len =
  (sizeof (amd64_dwarf_regmap) / sizeof (amd64_dwarf_regmap[0]));

/* Convert DWARF register number REG to the appropriate register
   number used by GDB.  */

static int
amd64_dwarf_reg_to_regnum (struct gdbarch *gdbarch, int reg)
{
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);
  int ymm0_regnum = tdep->ymm0_regnum;
  int regnum = -1;

  if (reg >= 0 && reg < amd64_dwarf_regmap_len)
    regnum = amd64_dwarf_regmap[reg];

  if (ymm0_regnum >= 0
	   && i386_xmm_regnum_p (gdbarch, regnum))
    regnum += ymm0_regnum - I387_XMM0_REGNUM (tdep);

  return regnum;
}

/* Map architectural register numbers to gdb register numbers.  */

static const int amd64_arch_regmap[16] =
{
  AMD64_RAX_REGNUM,	/* %rax */
  AMD64_RCX_REGNUM,	/* %rcx */
  AMD64_RDX_REGNUM,	/* %rdx */
  AMD64_RBX_REGNUM,	/* %rbx */
  AMD64_RSP_REGNUM,	/* %rsp */
  AMD64_RBP_REGNUM,	/* %rbp */
  AMD64_RSI_REGNUM,	/* %rsi */
  AMD64_RDI_REGNUM,	/* %rdi */
  AMD64_R8_REGNUM,	/* %r8 */
  AMD64_R9_REGNUM,	/* %r9 */
  AMD64_R10_REGNUM,	/* %r10 */
  AMD64_R11_REGNUM,	/* %r11 */
  AMD64_R12_REGNUM,	/* %r12 */
  AMD64_R13_REGNUM,	/* %r13 */
  AMD64_R14_REGNUM,	/* %r14 */
  AMD64_R15_REGNUM	/* %r15 */
};

static const int amd64_arch_regmap_len =
  (sizeof (amd64_arch_regmap) / sizeof (amd64_arch_regmap[0]));

/* Convert architectural register number REG to the appropriate register
   number used by GDB.  */

static int
amd64_arch_reg_to_regnum (int reg)
{
  gdb_assert (reg >= 0 && reg < amd64_arch_regmap_len);

  return amd64_arch_regmap[reg];
}

/* Register names for byte pseudo-registers.  */

static const char * const amd64_byte_names[] =
{
  "al", "bl", "cl", "dl", "sil", "dil", "bpl", "spl",
  "r8l", "r9l", "r10l", "r11l", "r12l", "r13l", "r14l", "r15l",
  "ah", "bh", "ch", "dh"
};

/* Number of lower byte registers.  */
#define AMD64_NUM_LOWER_BYTE_REGS 16

/* Register names for word pseudo-registers.  */

static const char * const amd64_word_names[] =
{
  "ax", "bx", "cx", "dx", "si", "di", "bp", "", 
  "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w"
};

/* Register names for dword pseudo-registers.  */

static const char * const amd64_dword_names[] =
{
  "eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp", 
  "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d",
  "eip"
};

/* Return the name of register REGNUM.  */

static const char *
amd64_pseudo_register_name (struct gdbarch *gdbarch, int regnum)
{
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);
  if (i386_byte_regnum_p (gdbarch, regnum))
    return amd64_byte_names[regnum - tdep->al_regnum];
  else if (i386_zmm_regnum_p (gdbarch, regnum))
    return amd64_zmm_names[regnum - tdep->zmm0_regnum];
  else if (i386_ymm_regnum_p (gdbarch, regnum))
    return amd64_ymm_names[regnum - tdep->ymm0_regnum];
  else if (i386_ymm_avx512_regnum_p (gdbarch, regnum))
    return amd64_ymm_avx512_names[regnum - tdep->ymm16_regnum];
  else if (i386_word_regnum_p (gdbarch, regnum))
    return amd64_word_names[regnum - tdep->ax_regnum];
  else if (i386_dword_regnum_p (gdbarch, regnum))
    return amd64_dword_names[regnum - tdep->eax_regnum];
  else
    return i386_pseudo_register_name (gdbarch, regnum);
}

static value *
amd64_pseudo_register_read_value (gdbarch *gdbarch, frame_info_ptr next_frame,
				  int regnum)
{
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);

  if (i386_byte_regnum_p (gdbarch, regnum))
    {
      int gpnum = regnum - tdep->al_regnum;

      /* Extract (always little endian).  */
      if (gpnum >= AMD64_NUM_LOWER_BYTE_REGS)
	{
	  gpnum -= AMD64_NUM_LOWER_BYTE_REGS;

	  /* Special handling for AH, BH, CH, DH.  */
	  return pseudo_from_raw_part (next_frame, regnum, gpnum, 1);
	}
      else
	return pseudo_from_raw_part (next_frame, regnum, gpnum, 0);
    }
  else if (i386_dword_regnum_p (gdbarch, regnum))
    {
      int gpnum = regnum - tdep->eax_regnum;

      return pseudo_from_raw_part (next_frame, regnum, gpnum, 0);
    }
  else
    return i386_pseudo_register_read_value (gdbarch, next_frame, regnum);
}

static void
amd64_pseudo_register_write (gdbarch *gdbarch, frame_info_ptr next_frame,
			     int regnum, gdb::array_view<const gdb_byte> buf)
{
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);

  if (i386_byte_regnum_p (gdbarch, regnum))
    {
      int gpnum = regnum - tdep->al_regnum;

      if (gpnum >= AMD64_NUM_LOWER_BYTE_REGS)
	{
	  gpnum -= AMD64_NUM_LOWER_BYTE_REGS;
	  pseudo_to_raw_part (next_frame, buf, gpnum, 1);
	}
      else
	pseudo_to_raw_part (next_frame, buf, gpnum, 0);
    }
  else if (i386_dword_regnum_p (gdbarch, regnum))
    {
      int gpnum = regnum - tdep->eax_regnum;
      pseudo_to_raw_part (next_frame, buf, gpnum, 0);
    }
  else
    i386_pseudo_register_write (gdbarch, next_frame, regnum, buf);
}

/* Implement the 'ax_pseudo_register_collect' gdbarch method.  */

static int
amd64_ax_pseudo_register_collect (struct gdbarch *gdbarch,
				  struct agent_expr *ax, int regnum)
{
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);

  if (i386_byte_regnum_p (gdbarch, regnum))
    {
      int gpnum = regnum - tdep->al_regnum;

      if (gpnum >= AMD64_NUM_LOWER_BYTE_REGS)
	ax_reg_mask (ax, gpnum - AMD64_NUM_LOWER_BYTE_REGS);
      else
	ax_reg_mask (ax, gpnum);
      return 0;
    }
  else if (i386_dword_regnum_p (gdbarch, regnum))
    {
      int gpnum = regnum - tdep->eax_regnum;

      ax_reg_mask (ax, gpnum);
      return 0;
    }
  else
    return i386_ax_pseudo_register_collect (gdbarch, ax, regnum);
}



/* Register classes as defined in the psABI.  */

enum amd64_reg_class
{
  AMD64_INTEGER,
  AMD64_SSE,
  AMD64_SSEUP,
  AMD64_X87,
  AMD64_X87UP,
  AMD64_COMPLEX_X87,
  AMD64_NO_CLASS,
  AMD64_MEMORY
};

/* Return the union class of CLASS1 and CLASS2.  See the psABI for
   details.  */

static enum amd64_reg_class
amd64_merge_classes (enum amd64_reg_class class1, enum amd64_reg_class class2)
{
  /* Rule (a): If both classes are equal, this is the resulting class.  */
  if (class1 == class2)
    return class1;

  /* Rule (b): If one of the classes is NO_CLASS, the resulting class
     is the other class.  */
  if (class1 == AMD64_NO_CLASS)
    return class2;
  if (class2 == AMD64_NO_CLASS)
    return class1;

  /* Rule (c): If one of the classes is MEMORY, the result is MEMORY.  */
  if (class1 == AMD64_MEMORY || class2 == AMD64_MEMORY)
    return AMD64_MEMORY;

  /* Rule (d): If one of the classes is INTEGER, the result is INTEGER.  */
  if (class1 == AMD64_INTEGER || class2 == AMD64_INTEGER)
    return AMD64_INTEGER;

  /* Rule (e): If one of the classes is X87, X87UP, COMPLEX_X87 class,
     MEMORY is used as class.  */
  if (class1 == AMD64_X87 || class1 == AMD64_X87UP
      || class1 == AMD64_COMPLEX_X87 || class2 == AMD64_X87
      || class2 == AMD64_X87UP || class2 == AMD64_COMPLEX_X87)
    return AMD64_MEMORY;

  /* Rule (f): Otherwise class SSE is used.  */
  return AMD64_SSE;
}

static void amd64_classify (struct type *type, enum amd64_reg_class theclass[2]);

/* Return true if TYPE is a structure or union with unaligned fields.  */

static bool
amd64_has_unaligned_fields (struct type *type)
{
  if (type->code () == TYPE_CODE_STRUCT
      || type->code () == TYPE_CODE_UNION)
    {
      for (int i = 0; i < type->num_fields (); i++)
	{
	  struct type *subtype = check_typedef (type->field (i).type ());

	  /* Ignore static fields, empty fields (for example nested
	     empty structures), and bitfields (these are handled by
	     the caller).  */
	  if (type->field (i).is_static ()
	      || (type->field (i).bitsize () == 0
		  && subtype->length () == 0)
	      || type->field (i).is_packed ())
	    continue;

	  int bitpos = type->field (i).loc_bitpos ();

	  if (bitpos % 8 != 0)
	    return true;

	  int align = type_align (subtype);
	  if (align == 0)
	    error (_("could not determine alignment of type"));

	  int bytepos = bitpos / 8;
	  if (bytepos % align != 0)
	    return true;

	  if (amd64_has_unaligned_fields (subtype))
	    return true;
	}
    }

  return false;
}

/* Classify field I of TYPE starting at BITOFFSET according to the rules for
   structures and union types, and store the result in THECLASS.  */

static void
amd64_classify_aggregate_field (struct type *type, int i,
				enum amd64_reg_class theclass[2],
				unsigned int bitoffset)
{
  struct type *subtype = check_typedef (type->field (i).type ());
  enum amd64_reg_class subclass[2];
  int bitsize = type->field (i).bitsize ();

  if (bitsize == 0)
    bitsize = subtype->length () * 8;

  /* Ignore static fields, or empty fields, for example nested
     empty structures.*/
  if (type->field (i).is_static () || bitsize == 0)
    return;

  int bitpos = bitoffset + type->field (i).loc_bitpos ();
  int pos = bitpos / 64;
  int endpos = (bitpos + bitsize - 1) / 64;

  if (subtype->code () == TYPE_CODE_STRUCT
      || subtype->code () == TYPE_CODE_UNION)
    {
      /* Each field of an object is classified recursively.  */
      int j;
      for (j = 0; j < subtype->num_fields (); j++)
	amd64_classify_aggregate_field (subtype, j, theclass, bitpos);
      return;
    }

  gdb_assert (pos == 0 || pos == 1);

  amd64_classify (subtype, subclass);
  theclass[pos] = amd64_merge_classes (theclass[pos], subclass[0]);
  if (bitsize <= 64 && pos == 0 && endpos == 1)
    /* This is a bit of an odd case:  We have a field that would
       normally fit in one of the two eightbytes, except that
       it is placed in a way that this field straddles them.
       This has been seen with a structure containing an array.

       The ABI is a bit unclear in this case, but we assume that
       this field's class (stored in subclass[0]) must also be merged
       into class[1].  In other words, our field has a piece stored
       in the second eight-byte, and thus its class applies to
       the second eight-byte as well.

       In the case where the field length exceeds 8 bytes,
       it should not be necessary to merge the field class
       into class[1].  As LEN > 8, subclass[1] is necessarily
       different from AMD64_NO_CLASS.  If subclass[1] is equal
       to subclass[0], then the normal class[1]/subclass[1]
       merging will take care of everything.  For subclass[1]
       to be different from subclass[0], I can only see the case
       where we have a SSE/SSEUP or X87/X87UP pair, which both
       use up all 16 bytes of the aggregate, and are already
       handled just fine (because each portion sits on its own
       8-byte).  */
    theclass[1] = amd64_merge_classes (theclass[1], subclass[0]);
  if (pos == 0)
    theclass[1] = amd64_merge_classes (theclass[1], subclass[1]);
}

/* Classify TYPE according to the rules for aggregate (structures and
   arrays) and union types, and store the result in CLASS.  */

static void
amd64_classify_aggregate (struct type *type, enum amd64_reg_class theclass[2])
{
  /* 1. If the size of an object is larger than two times eight bytes, or
	it is a non-trivial C++ object, or it has unaligned fields, then it
	has class memory.

	It is important that the trivially_copyable check is before the
	unaligned fields check, as C++ classes with virtual base classes
	will have fields (for the virtual base classes) with non-constant
	loc_bitpos attributes, which will cause an assert to trigger within
	the unaligned field check.  As classes with virtual bases are not
	trivially copyable, checking that first avoids this problem.  */
  if (TYPE_HAS_DYNAMIC_LENGTH (type)
      || type->length () > 16
      || !language_pass_by_reference (type).trivially_copyable
      || amd64_has_unaligned_fields (type))
    {
      theclass[0] = theclass[1] = AMD64_MEMORY;
      return;
    }

  /* 2. Both eightbytes get initialized to class NO_CLASS.  */
  theclass[0] = theclass[1] = AMD64_NO_CLASS;

  /* 3. Each field of an object is classified recursively so that
	always two fields are considered. The resulting class is
	calculated according to the classes of the fields in the
	eightbyte: */

  if (type->code () == TYPE_CODE_ARRAY)
    {
      struct type *subtype = check_typedef (type->target_type ());

      /* All fields in an array have the same type.  */
      amd64_classify (subtype, theclass);
      if (type->length () > 8 && theclass[1] == AMD64_NO_CLASS)
	theclass[1] = theclass[0];
    }
  else
    {
      int i;

      /* Structure or union.  */
      gdb_assert (type->code () == TYPE_CODE_STRUCT
		  || type->code () == TYPE_CODE_UNION);

      for (i = 0; i < type->num_fields (); i++)
	amd64_classify_aggregate_field (type, i, theclass, 0);
    }

  /* 4. Then a post merger cleanup is done:  */

  /* Rule (a): If one of the classes is MEMORY, the whole argument is
     passed in memory.  */
  if (theclass[0] == AMD64_MEMORY || theclass[1] == AMD64_MEMORY)
    theclass[0] = theclass[1] = AMD64_MEMORY;

  /* Rule (b): If SSEUP is not preceded by SSE, it is converted to
     SSE.  */
  if (theclass[0] == AMD64_SSEUP)
    theclass[0] = AMD64_SSE;
  if (theclass[1] == AMD64_SSEUP && theclass[0] != AMD64_SSE)
    theclass[1] = AMD64_SSE;
}

/* Classify TYPE, and store the result in CLASS.  */

static void
amd64_classify (struct type *type, enum amd64_reg_class theclass[2])
{
  enum type_code code = type->code ();
  int len = type->length ();

  theclass[0] = theclass[1] = AMD64_NO_CLASS;

  /* Arguments of types (signed and unsigned) _Bool, char, short, int,
     long, long long, and pointers are in the INTEGER class.  Similarly,
     range types, used by languages such as Ada, are also in the INTEGER
     class.  */
  if ((code == TYPE_CODE_INT || code == TYPE_CODE_ENUM
       || code == TYPE_CODE_BOOL || code == TYPE_CODE_RANGE
       || code == TYPE_CODE_CHAR
       || code == TYPE_CODE_PTR || TYPE_IS_REFERENCE (type))
      && (len == 1 || len == 2 || len == 4 || len == 8))
    theclass[0] = AMD64_INTEGER;

  /* Arguments of types _Float16, float, double, _Decimal32, _Decimal64 and
     __m64 are in class SSE.  */
  else if ((code == TYPE_CODE_FLT || code == TYPE_CODE_DECFLOAT)
	   && (len == 2 || len == 4 || len == 8))
    /* FIXME: __m64 .  */
    theclass[0] = AMD64_SSE;

  /* Arguments of types __float128, _Decimal128 and __m128 are split into
     two halves.  The least significant ones belong to class SSE, the most
     significant one to class SSEUP.  */
  else if (code == TYPE_CODE_DECFLOAT && len == 16)
    /* FIXME: __float128, __m128.  */
    theclass[0] = AMD64_SSE, theclass[1] = AMD64_SSEUP;

  /* The 64-bit mantissa of arguments of type long double belongs to
     class X87, the 16-bit exponent plus 6 bytes of padding belongs to
     class X87UP.  */
  else if (code == TYPE_CODE_FLT && len == 16)
    /* Class X87 and X87UP.  */
    theclass[0] = AMD64_X87, theclass[1] = AMD64_X87UP;

  /* Arguments of complex T - where T is one of the types _Float16, float or
     double - get treated as if they are implemented as:

     struct complexT {
       T real;
       T imag;
     };

  */
  else if (code == TYPE_CODE_COMPLEX && (len == 8 || len == 4))
    theclass[0] = AMD64_SSE;
  else if (code == TYPE_CODE_COMPLEX && len == 16)
    theclass[0] = theclass[1] = AMD64_SSE;

  /* A variable of type complex long double is classified as type
     COMPLEX_X87.  */
  else if (code == TYPE_CODE_COMPLEX && len == 32)
    theclass[0] = AMD64_COMPLEX_X87;

  /* Aggregates.  */
  else if (code == TYPE_CODE_ARRAY || code == TYPE_CODE_STRUCT
	   || code == TYPE_CODE_UNION)
    amd64_classify_aggregate (type, theclass);
}

static enum return_value_convention
amd64_return_value (struct gdbarch *gdbarch, struct value *function,
		    struct type *type, struct regcache *regcache,
		    struct value **read_value, const gdb_byte *writebuf)
{
  enum amd64_reg_class theclass[2];
  int len = type->length ();
  static int integer_regnum[] = { AMD64_RAX_REGNUM, AMD64_RDX_REGNUM };
  static int sse_regnum[] = { AMD64_XMM0_REGNUM, AMD64_XMM1_REGNUM };
  int integer_reg = 0;
  int sse_reg = 0;
  int i;

  gdb_assert (!(read_value && writebuf));

  /* 1. Classify the return type with the classification algorithm.  */
  amd64_classify (type, theclass);

  /* 2. If the type has class MEMORY, then the caller provides space
     for the return value and passes the address of this storage in
     %rdi as if it were the first argument to the function.  In effect,
     this address becomes a hidden first argument.

     On return %rax will contain the address that has been passed in
     by the caller in %rdi.  */
  if (theclass[0] == AMD64_MEMORY)
    {
      /* As indicated by the comment above, the ABI guarantees that we
	 can always find the return value just after the function has
	 returned.  */

      if (read_value != nullptr)
	{
	  ULONGEST addr;

	  regcache_raw_read_unsigned (regcache, AMD64_RAX_REGNUM, &addr);
	  *read_value = value_at_non_lval (type, addr);
	}

      return RETURN_VALUE_ABI_RETURNS_ADDRESS;
    }

  gdb_byte *readbuf = nullptr;
  if (read_value != nullptr)
    {
      *read_value = value::allocate (type);
      readbuf = (*read_value)->contents_raw ().data ();
    }

  /* 8. If the class is COMPLEX_X87, the real part of the value is
	returned in %st0 and the imaginary part in %st1.  */
  if (theclass[0] == AMD64_COMPLEX_X87)
    {
      if (readbuf)
	{
	  regcache->raw_read (AMD64_ST0_REGNUM, readbuf);
	  regcache->raw_read (AMD64_ST1_REGNUM, readbuf + 16);
	}

      if (writebuf)
	{
	  i387_return_value (gdbarch, regcache);
	  regcache->raw_write (AMD64_ST0_REGNUM, writebuf);
	  regcache->raw_write (AMD64_ST1_REGNUM, writebuf + 16);

	  /* Fix up the tag word such that both %st(0) and %st(1) are
	     marked as valid.  */
	  regcache_raw_write_unsigned (regcache, AMD64_FTAG_REGNUM, 0xfff);
	}

      return RETURN_VALUE_REGISTER_CONVENTION;
    }

  gdb_assert (theclass[1] != AMD64_MEMORY);
  gdb_assert (len <= 16);

  for (i = 0; len > 0; i++, len -= 8)
    {
      int regnum = -1;
      int offset = 0;

      switch (theclass[i])
	{
	case AMD64_INTEGER:
	  /* 3. If the class is INTEGER, the next available register
	     of the sequence %rax, %rdx is used.  */
	  regnum = integer_regnum[integer_reg++];
	  break;

	case AMD64_SSE:
	  /* 4. If the class is SSE, the next available SSE register
	     of the sequence %xmm0, %xmm1 is used.  */
	  regnum = sse_regnum[sse_reg++];
	  break;

	case AMD64_SSEUP:
	  /* 5. If the class is SSEUP, the eightbyte is passed in the
	     upper half of the last used SSE register.  */
	  gdb_assert (sse_reg > 0);
	  regnum = sse_regnum[sse_reg - 1];
	  offset = 8;
	  break;

	case AMD64_X87:
	  /* 6. If the class is X87, the value is returned on the X87
	     stack in %st0 as 80-bit x87 number.  */
	  regnum = AMD64_ST0_REGNUM;
	  if (writebuf)
	    i387_return_value (gdbarch, regcache);
	  break;

	case AMD64_X87UP:
	  /* 7. If the class is X87UP, the value is returned together
	     with the previous X87 value in %st0.  */
	  gdb_assert (i > 0 && theclass[0] == AMD64_X87);
	  regnum = AMD64_ST0_REGNUM;
	  offset = 8;
	  len = 2;
	  break;

	case AMD64_NO_CLASS:
	  continue;

	default:
	  gdb_assert (!"Unexpected register class.");
	}

      gdb_assert (regnum != -1);

      if (readbuf)
	regcache->raw_read_part (regnum, offset, std::min (len, 8),
				 readbuf + i * 8);
      if (writebuf)
	regcache->raw_write_part (regnum, offset, std::min (len, 8),
				  writebuf + i * 8);
    }

  return RETURN_VALUE_REGISTER_CONVENTION;
}


static CORE_ADDR
amd64_push_arguments (struct regcache *regcache, int nargs, struct value **args,
		      CORE_ADDR sp, function_call_return_method return_method)
{
  static int integer_regnum[] =
  {
    AMD64_RDI_REGNUM,		/* %rdi */
    AMD64_RSI_REGNUM,		/* %rsi */
    AMD64_RDX_REGNUM,		/* %rdx */
    AMD64_RCX_REGNUM,		/* %rcx */
    AMD64_R8_REGNUM,		/* %r8 */
    AMD64_R9_REGNUM		/* %r9 */
  };
  static int sse_regnum[] =
  {
    /* %xmm0 ... %xmm7 */
    AMD64_XMM0_REGNUM + 0, AMD64_XMM1_REGNUM,
    AMD64_XMM0_REGNUM + 2, AMD64_XMM0_REGNUM + 3,
    AMD64_XMM0_REGNUM + 4, AMD64_XMM0_REGNUM + 5,
    AMD64_XMM0_REGNUM + 6, AMD64_XMM0_REGNUM + 7,
  };
  struct value **stack_args = XALLOCAVEC (struct value *, nargs);
  int num_stack_args = 0;
  int num_elements = 0;
  int element = 0;
  int integer_reg = 0;
  int sse_reg = 0;
  int i;

  /* Reserve a register for the "hidden" argument.  */
if (return_method == return_method_struct)
    integer_reg++;

  for (i = 0; i < nargs; i++)
    {
      struct type *type = args[i]->type ();
      int len = type->length ();
      enum amd64_reg_class theclass[2];
      int needed_integer_regs = 0;
      int needed_sse_regs = 0;
      int j;

      /* Classify argument.  */
      amd64_classify (type, theclass);

      /* Calculate the number of integer and SSE registers needed for
	 this argument.  */
      for (j = 0; j < 2; j++)
	{
	  if (theclass[j] == AMD64_INTEGER)
	    needed_integer_regs++;
	  else if (theclass[j] == AMD64_SSE)
	    needed_sse_regs++;
	}

      /* Check whether enough registers are available, and if the
	 argument should be passed in registers at all.  */
      if (integer_reg + needed_integer_regs > ARRAY_SIZE (integer_regnum)
	  || sse_reg + needed_sse_regs > ARRAY_SIZE (sse_regnum)
	  || (needed_integer_regs == 0 && needed_sse_regs == 0))
	{
	  /* The argument will be passed on the stack.  */
	  num_elements += ((len + 7) / 8);
	  stack_args[num_stack_args++] = args[i];
	}
      else
	{
	  /* The argument will be passed in registers.  */
	  const gdb_byte *valbuf = args[i]->contents ().data ();
	  gdb_byte buf[8];

	  gdb_assert (len <= 16);

	  for (j = 0; len > 0; j++, len -= 8)
	    {
	      int regnum = -1;
	      int offset = 0;

	      switch (theclass[j])
		{
		case AMD64_INTEGER:
		  regnum = integer_regnum[integer_reg++];
		  break;

		case AMD64_SSE:
		  regnum = sse_regnum[sse_reg++];
		  break;

		case AMD64_SSEUP:
		  gdb_assert (sse_reg > 0);
		  regnum = sse_regnum[sse_reg - 1];
		  offset = 8;
		  break;

		case AMD64_NO_CLASS:
		  continue;

		default:
		  gdb_assert (!"Unexpected register class.");
		}

	      gdb_assert (regnum != -1);
	      memset (buf, 0, sizeof buf);
	      memcpy (buf, valbuf + j * 8, std::min (len, 8));
	      regcache->raw_write_part (regnum, offset, 8, buf);
	    }
	}
    }

  /* Allocate space for the arguments on the stack.  */
  sp -= num_elements * 8;

  /* The psABI says that "The end of the input argument area shall be
     aligned on a 16 byte boundary."  */
  sp &= ~0xf;

  /* Write out the arguments to the stack.  */
  for (i = 0; i < num_stack_args; i++)
    {
      struct type *type = stack_args[i]->type ();
      const gdb_byte *valbuf = stack_args[i]->contents ().data ();
      int len = type->length ();

      write_memory (sp + element * 8, valbuf, len);
      element += ((len + 7) / 8);
    }

  /* The psABI says that "For calls that may call functions that use
     varargs or stdargs (prototype-less calls or calls to functions
     containing ellipsis (...) in the declaration) %al is used as
     hidden argument to specify the number of SSE registers used.  */
  regcache_raw_write_unsigned (regcache, AMD64_RAX_REGNUM, sse_reg);
  return sp; 
}

static CORE_ADDR
amd64_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
		       struct regcache *regcache, CORE_ADDR bp_addr,
		       int nargs, struct value **args,	CORE_ADDR sp,
		       function_call_return_method return_method,
		       CORE_ADDR struct_addr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte buf[8];

  /* BND registers can be in arbitrary values at the moment of the
     inferior call.  This can cause boundary violations that are not
     due to a real bug or even desired by the user.  The best to be done
     is set the BND registers to allow access to the whole memory, INIT
     state, before pushing the inferior call.   */
  i387_reset_bnd_regs (gdbarch, regcache);

  /* Pass arguments.  */
  sp = amd64_push_arguments (regcache, nargs, args, sp, return_method);

  /* Pass "hidden" argument".  */
  if (return_method == return_method_struct)
    {
      store_unsigned_integer (buf, 8, byte_order, struct_addr);
      regcache->cooked_write (AMD64_RDI_REGNUM, buf);
    }

  /* Store return address.  */
  sp -= 8;
  store_unsigned_integer (buf, 8, byte_order, bp_addr);
  write_memory (sp, buf, 8);

  /* Finally, update the stack pointer...  */
  store_unsigned_integer (buf, 8, byte_order, sp);
  regcache->cooked_write (AMD64_RSP_REGNUM, buf);

  /* ...and fake a frame pointer.  */
  regcache->cooked_write (AMD64_RBP_REGNUM, buf);

  return sp + 16;
}

/* Displaced instruction handling.  */

/* A partially decoded instruction.
   This contains enough details for displaced stepping purposes.  */

struct amd64_insn
{
  /* The number of opcode bytes.  */
  int opcode_len;
  /* The offset of the REX/VEX instruction encoding prefix or -1 if
     not present.  */
  int enc_prefix_offset;
  /* The offset to the first opcode byte.  */
  int opcode_offset;
  /* The offset to the modrm byte or -1 if not present.  */
  int modrm_offset;

  /* The raw instruction.  */
  gdb_byte *raw_insn;
};

struct amd64_displaced_step_copy_insn_closure
  : public displaced_step_copy_insn_closure
{
  amd64_displaced_step_copy_insn_closure (int insn_buf_len)
  : insn_buf (insn_buf_len, 0)
  {}

  /* For rip-relative insns, saved copy of the reg we use instead of %rip.  */
  int tmp_used = 0;
  int tmp_regno;
  ULONGEST tmp_save;

  /* Details of the instruction.  */
  struct amd64_insn insn_details;

  /* The possibly modified insn.  */
  gdb::byte_vector insn_buf;
};

/* WARNING: Keep onebyte_has_modrm, twobyte_has_modrm in sync with
   ../opcodes/i386-dis.c (until libopcodes exports them, or an alternative,
   at which point delete these in favor of libopcodes' versions).  */

static const unsigned char onebyte_has_modrm[256] = {
  /*	   0 1 2 3 4 5 6 7 8 9 a b c d e f	  */
  /*	   -------------------------------	  */
  /* 00 */ 1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,0, /* 00 */
  /* 10 */ 1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,0, /* 10 */
  /* 20 */ 1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,0, /* 20 */
  /* 30 */ 1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,0, /* 30 */
  /* 40 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 40 */
  /* 50 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 50 */
  /* 60 */ 0,0,1,1,0,0,0,0,0,1,0,1,0,0,0,0, /* 60 */
  /* 70 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 70 */
  /* 80 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 80 */
  /* 90 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 90 */
  /* a0 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* a0 */
  /* b0 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* b0 */
  /* c0 */ 1,1,0,0,1,1,1,1,0,0,0,0,0,0,0,0, /* c0 */
  /* d0 */ 1,1,1,1,0,0,0,0,1,1,1,1,1,1,1,1, /* d0 */
  /* e0 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* e0 */
  /* f0 */ 0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1  /* f0 */
  /*	   -------------------------------	  */
  /*	   0 1 2 3 4 5 6 7 8 9 a b c d e f	  */
};

static const unsigned char twobyte_has_modrm[256] = {
  /*	   0 1 2 3 4 5 6 7 8 9 a b c d e f	  */
  /*	   -------------------------------	  */
  /* 00 */ 1,1,1,1,0,0,0,0,0,0,0,0,0,1,0,1, /* 0f */
  /* 10 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 1f */
  /* 20 */ 1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1, /* 2f */
  /* 30 */ 0,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0, /* 3f */
  /* 40 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 4f */
  /* 50 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 5f */
  /* 60 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 6f */
  /* 70 */ 1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1, /* 7f */
  /* 80 */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 8f */
  /* 90 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 9f */
  /* a0 */ 0,0,0,1,1,1,1,1,0,0,0,1,1,1,1,1, /* af */
  /* b0 */ 1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1, /* bf */
  /* c0 */ 1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0, /* cf */
  /* d0 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* df */
  /* e0 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* ef */
  /* f0 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0  /* ff */
  /*	   -------------------------------	  */
  /*	   0 1 2 3 4 5 6 7 8 9 a b c d e f	  */
};

static int amd64_syscall_p (const struct amd64_insn *insn, int *lengthp);

static int
rex_prefix_p (gdb_byte pfx)
{
  return REX_PREFIX_P (pfx);
}

/* True if PFX is the start of the 2-byte VEX prefix.  */

static bool
vex2_prefix_p (gdb_byte pfx)
{
  return pfx == 0xc5;
}

/* True if PFX is the start of the 3-byte VEX prefix.  */

static bool
vex3_prefix_p (gdb_byte pfx)
{
  return pfx == 0xc4;
}

/* Skip the legacy instruction prefixes in INSN.
   We assume INSN is properly sentineled so we don't have to worry
   about falling off the end of the buffer.  */

static gdb_byte *
amd64_skip_prefixes (gdb_byte *insn)
{
  while (1)
    {
      switch (*insn)
	{
	case DATA_PREFIX_OPCODE:
	case ADDR_PREFIX_OPCODE:
	case CS_PREFIX_OPCODE:
	case DS_PREFIX_OPCODE:
	case ES_PREFIX_OPCODE:
	case FS_PREFIX_OPCODE:
	case GS_PREFIX_OPCODE:
	case SS_PREFIX_OPCODE:
	case LOCK_PREFIX_OPCODE:
	case REPE_PREFIX_OPCODE:
	case REPNE_PREFIX_OPCODE:
	  ++insn;
	  continue;
	default:
	  break;
	}
      break;
    }

  return insn;
}

/* Return an integer register (other than RSP) that is unused as an input
   operand in INSN.
   In order to not require adding a rex prefix if the insn doesn't already
   have one, the result is restricted to RAX ... RDI, sans RSP.
   The register numbering of the result follows architecture ordering,
   e.g. RDI = 7.  */

static int
amd64_get_unused_input_int_reg (const struct amd64_insn *details)
{
  /* 1 bit for each reg */
  int used_regs_mask = 0;

  /* There can be at most 3 int regs used as inputs in an insn, and we have
     7 to choose from (RAX ... RDI, sans RSP).
     This allows us to take a conservative approach and keep things simple.
     E.g. By avoiding RAX, we don't have to specifically watch for opcodes
     that implicitly specify RAX.  */

  /* Avoid RAX.  */
  used_regs_mask |= 1 << EAX_REG_NUM;
  /* Similarily avoid RDX, implicit operand in divides.  */
  used_regs_mask |= 1 << EDX_REG_NUM;
  /* Avoid RSP.  */
  used_regs_mask |= 1 << ESP_REG_NUM;

  /* If the opcode is one byte long and there's no ModRM byte,
     assume the opcode specifies a register.  */
  if (details->opcode_len == 1 && details->modrm_offset == -1)
    used_regs_mask |= 1 << (details->raw_insn[details->opcode_offset] & 7);

  /* Mark used regs in the modrm/sib bytes.  */
  if (details->modrm_offset != -1)
    {
      int modrm = details->raw_insn[details->modrm_offset];
      int mod = MODRM_MOD_FIELD (modrm);
      int reg = MODRM_REG_FIELD (modrm);
      int rm = MODRM_RM_FIELD (modrm);
      int have_sib = mod != 3 && rm == 4;

      /* Assume the reg field of the modrm byte specifies a register.  */
      used_regs_mask |= 1 << reg;

      if (have_sib)
	{
	  int base = SIB_BASE_FIELD (details->raw_insn[details->modrm_offset + 1]);
	  int idx = SIB_INDEX_FIELD (details->raw_insn[details->modrm_offset + 1]);
	  used_regs_mask |= 1 << base;
	  used_regs_mask |= 1 << idx;
	}
      else
	{
	  used_regs_mask |= 1 << rm;
	}
    }

  gdb_assert (used_regs_mask < 256);
  gdb_assert (used_regs_mask != 255);

  /* Finally, find a free reg.  */
  {
    int i;

    for (i = 0; i < 8; ++i)
      {
	if (! (used_regs_mask & (1 << i)))
	  return i;
      }

    /* We shouldn't get here.  */
    internal_error (_("unable to find free reg"));
  }
}

/* Extract the details of INSN that we need.  */

static void
amd64_get_insn_details (gdb_byte *insn, struct amd64_insn *details)
{
  gdb_byte *start = insn;
  int need_modrm;

  details->raw_insn = insn;

  details->opcode_len = -1;
  details->enc_prefix_offset = -1;
  details->opcode_offset = -1;
  details->modrm_offset = -1;

  /* Skip legacy instruction prefixes.  */
  insn = amd64_skip_prefixes (insn);

  /* Skip REX/VEX instruction encoding prefixes.  */
  if (rex_prefix_p (*insn))
    {
      details->enc_prefix_offset = insn - start;
      ++insn;
    }
  else if (vex2_prefix_p (*insn))
    {
      /* Don't record the offset in this case because this prefix has
	 no REX.B equivalent.  */
      insn += 2;
    }
  else if (vex3_prefix_p (*insn))
    {
      details->enc_prefix_offset = insn - start;
      insn += 3;
    }

  details->opcode_offset = insn - start;

  if (*insn == TWO_BYTE_OPCODE_ESCAPE)
    {
      /* Two or three-byte opcode.  */
      ++insn;
      need_modrm = twobyte_has_modrm[*insn];

      /* Check for three-byte opcode.  */
      switch (*insn)
	{
	case 0x24:
	case 0x25:
	case 0x38:
	case 0x3a:
	case 0x7a:
	case 0x7b:
	  ++insn;
	  details->opcode_len = 3;
	  break;
	default:
	  details->opcode_len = 2;
	  break;
	}
    }
  else
    {
      /* One-byte opcode.  */
      need_modrm = onebyte_has_modrm[*insn];
      details->opcode_len = 1;
    }

  if (need_modrm)
    {
      ++insn;
      details->modrm_offset = insn - start;
    }
}

/* Update %rip-relative addressing in INSN.

   %rip-relative addressing only uses a 32-bit displacement.
   32 bits is not enough to be guaranteed to cover the distance between where
   the real instruction is and where its copy is.
   Convert the insn to use base+disp addressing.
   We set base = pc + insn_length so we can leave disp unchanged.  */

static void
fixup_riprel (struct gdbarch *gdbarch,
	      amd64_displaced_step_copy_insn_closure *dsc,
	      CORE_ADDR from, CORE_ADDR to, struct regcache *regs)
{
  const struct amd64_insn *insn_details = &dsc->insn_details;
  int modrm_offset = insn_details->modrm_offset;
  CORE_ADDR rip_base;
  int insn_length;
  int arch_tmp_regno, tmp_regno;
  ULONGEST orig_value;

  /* Compute the rip-relative address.	*/
  insn_length = gdb_buffered_insn_length (gdbarch, dsc->insn_buf.data (),
					  dsc->insn_buf.size (), from);
  rip_base = from + insn_length;

  /* We need a register to hold the address.
     Pick one not used in the insn.
     NOTE: arch_tmp_regno uses architecture ordering, e.g. RDI = 7.  */
  arch_tmp_regno = amd64_get_unused_input_int_reg (insn_details);
  tmp_regno = amd64_arch_reg_to_regnum (arch_tmp_regno);

  /* Position of the not-B bit in the 3-byte VEX prefix (in byte 1).  */
  static constexpr gdb_byte VEX3_NOT_B = 0x20;

  /* REX.B should be unset (VEX.!B set) as we were using rip-relative
     addressing, but ensure it's unset (set for VEX) anyway, tmp_regno
     is not r8-r15.  */
  if (insn_details->enc_prefix_offset != -1)
    {
      gdb_byte *pfx = &dsc->insn_buf[insn_details->enc_prefix_offset];
      if (rex_prefix_p (pfx[0]))
	pfx[0] &= ~REX_B;
      else if (vex3_prefix_p (pfx[0]))
	pfx[1] |= VEX3_NOT_B;
      else
	gdb_assert_not_reached ("unhandled prefix");
    }

  regcache_cooked_read_unsigned (regs, tmp_regno, &orig_value);
  dsc->tmp_regno = tmp_regno;
  dsc->tmp_save = orig_value;
  dsc->tmp_used = 1;

  /* Convert the ModRM field to be base+disp.  */
  dsc->insn_buf[modrm_offset] &= ~0xc7;
  dsc->insn_buf[modrm_offset] |= 0x80 + arch_tmp_regno;

  regcache_cooked_write_unsigned (regs, tmp_regno, rip_base);

  displaced_debug_printf ("%%rip-relative addressing used.");
  displaced_debug_printf ("using temp reg %d, old value %s, new value %s",
			  dsc->tmp_regno, paddress (gdbarch, dsc->tmp_save),
			  paddress (gdbarch, rip_base));
}

static void
fixup_displaced_copy (struct gdbarch *gdbarch,
		      amd64_displaced_step_copy_insn_closure *dsc,
		      CORE_ADDR from, CORE_ADDR to, struct regcache *regs)
{
  const struct amd64_insn *details = &dsc->insn_details;

  if (details->modrm_offset != -1)
    {
      gdb_byte modrm = details->raw_insn[details->modrm_offset];

      if ((modrm & 0xc7) == 0x05)
	{
	  /* The insn uses rip-relative addressing.
	     Deal with it.  */
	  fixup_riprel (gdbarch, dsc, from, to, regs);
	}
    }
}

displaced_step_copy_insn_closure_up
amd64_displaced_step_copy_insn (struct gdbarch *gdbarch,
				CORE_ADDR from, CORE_ADDR to,
				struct regcache *regs)
{
  int len = gdbarch_max_insn_length (gdbarch);
  /* Extra space for sentinels so fixup_{riprel,displaced_copy} don't have to
     continually watch for running off the end of the buffer.  */
  int fixup_sentinel_space = len;
  std::unique_ptr<amd64_displaced_step_copy_insn_closure> dsc
    (new amd64_displaced_step_copy_insn_closure (len + fixup_sentinel_space));
  gdb_byte *buf = &dsc->insn_buf[0];
  struct amd64_insn *details = &dsc->insn_details;

  read_memory (from, buf, len);

  /* Set up the sentinel space so we don't have to worry about running
     off the end of the buffer.  An excessive number of leading prefixes
     could otherwise cause this.  */
  memset (buf + len, 0, fixup_sentinel_space);

  amd64_get_insn_details (buf, details);

  /* GDB may get control back after the insn after the syscall.
     Presumably this is a kernel bug.
     If this is a syscall, make sure there's a nop afterwards.  */
  {
    int syscall_length;

    if (amd64_syscall_p (details, &syscall_length))
      buf[details->opcode_offset + syscall_length] = NOP_OPCODE;
  }

  /* Modify the insn to cope with the address where it will be executed from.
     In particular, handle any rip-relative addressing.	 */
  fixup_displaced_copy (gdbarch, dsc.get (), from, to, regs);

  write_memory (to, buf, len);

  displaced_debug_printf ("copy %s->%s: %s",
			  paddress (gdbarch, from), paddress (gdbarch, to),
			  bytes_to_string (buf, len).c_str ());

  /* This is a work around for a problem with g++ 4.8.  */
  return displaced_step_copy_insn_closure_up (dsc.release ());
}

static int
amd64_absolute_jmp_p (const struct amd64_insn *details)
{
  const gdb_byte *insn = &details->raw_insn[details->opcode_offset];

  if (insn[0] == 0xff)
    {
      /* jump near, absolute indirect (/4) */
      if ((insn[1] & 0x38) == 0x20)
	return 1;

      /* jump far, absolute indirect (/5) */
      if ((insn[1] & 0x38) == 0x28)
	return 1;
    }

  return 0;
}

/* Return non-zero if the instruction DETAILS is a jump, zero otherwise.  */

static int
amd64_jmp_p (const struct amd64_insn *details)
{
  const gdb_byte *insn = &details->raw_insn[details->opcode_offset];

  /* jump short, relative.  */
  if (insn[0] == 0xeb)
    return 1;

  /* jump near, relative.  */
  if (insn[0] == 0xe9)
    return 1;

  return amd64_absolute_jmp_p (details);
}

static int
amd64_absolute_call_p (const struct amd64_insn *details)
{
  const gdb_byte *insn = &details->raw_insn[details->opcode_offset];

  if (insn[0] == 0xff)
    {
      /* Call near, absolute indirect (/2) */
      if ((insn[1] & 0x38) == 0x10)
	return 1;

      /* Call far, absolute indirect (/3) */
      if ((insn[1] & 0x38) == 0x18)
	return 1;
    }

  return 0;
}

static int
amd64_ret_p (const struct amd64_insn *details)
{
  /* NOTE: gcc can emit "repz ; ret".  */
  const gdb_byte *insn = &details->raw_insn[details->opcode_offset];

  switch (insn[0])
    {
    case 0xc2: /* ret near, pop N bytes */
    case 0xc3: /* ret near */
    case 0xca: /* ret far, pop N bytes */
    case 0xcb: /* ret far */
    case 0xcf: /* iret */
      return 1;

    default:
      return 0;
    }
}

static int
amd64_call_p (const struct amd64_insn *details)
{
  const gdb_byte *insn = &details->raw_insn[details->opcode_offset];

  if (amd64_absolute_call_p (details))
    return 1;

  /* call near, relative */
  if (insn[0] == 0xe8)
    return 1;

  return 0;
}

/* Return non-zero if INSN is a system call, and set *LENGTHP to its
   length in bytes.  Otherwise, return zero.  */

static int
amd64_syscall_p (const struct amd64_insn *details, int *lengthp)
{
  const gdb_byte *insn = &details->raw_insn[details->opcode_offset];

  if (insn[0] == 0x0f && insn[1] == 0x05)
    {
      *lengthp = 2;
      return 1;
    }

  return 0;
}

/* Classify the instruction at ADDR using PRED.
   Throw an error if the memory can't be read.  */

static int
amd64_classify_insn_at (struct gdbarch *gdbarch, CORE_ADDR addr,
			int (*pred) (const struct amd64_insn *))
{
  struct amd64_insn details;

  gdb::byte_vector buf (gdbarch_max_insn_length (gdbarch));

  read_code (addr, buf.data (), buf.size ());
  amd64_get_insn_details (buf.data (), &details);

  int classification = pred (&details);

  return classification;
}

/* The gdbarch insn_is_call method.  */

static int
amd64_insn_is_call (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  return amd64_classify_insn_at (gdbarch, addr, amd64_call_p);
}

/* The gdbarch insn_is_ret method.  */

static int
amd64_insn_is_ret (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  return amd64_classify_insn_at (gdbarch, addr, amd64_ret_p);
}

/* The gdbarch insn_is_jump method.  */

static int
amd64_insn_is_jump (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  return amd64_classify_insn_at (gdbarch, addr, amd64_jmp_p);
}

/* Fix up the state of registers and memory after having single-stepped
   a displaced instruction.  */

void
amd64_displaced_step_fixup (struct gdbarch *gdbarch,
			    struct displaced_step_copy_insn_closure *dsc_,
			    CORE_ADDR from, CORE_ADDR to,
			    struct regcache *regs, bool completed_p)
{
  amd64_displaced_step_copy_insn_closure *dsc
    = (amd64_displaced_step_copy_insn_closure *) dsc_;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  /* The offset we applied to the instruction's address.  */
  ULONGEST insn_offset = to - from;
  gdb_byte *insn = dsc->insn_buf.data ();
  const struct amd64_insn *insn_details = &dsc->insn_details;

  displaced_debug_printf ("fixup (%s, %s), insn = 0x%02x 0x%02x ...",
			  paddress (gdbarch, from), paddress (gdbarch, to),
			  insn[0], insn[1]);

  /* If we used a tmp reg, restore it.	*/

  if (dsc->tmp_used)
    {
      displaced_debug_printf ("restoring reg %d to %s",
			      dsc->tmp_regno, paddress (gdbarch, dsc->tmp_save));
      regcache_cooked_write_unsigned (regs, dsc->tmp_regno, dsc->tmp_save);
    }

  /* The list of issues to contend with here is taken from
     resume_execution in arch/x86/kernel/kprobes.c, Linux 2.6.28.
     Yay for Free Software!  */

  /* Relocate the %rip back to the program's instruction stream,
     if necessary.  */

  /* Except in the case of absolute or indirect jump or call
     instructions, or a return instruction, the new rip is relative to
     the displaced instruction; make it relative to the original insn.
     Well, signal handler returns don't need relocation either, but we use the
     value of %rip to recognize those; see below.  */
  if (!completed_p
      || (!amd64_absolute_jmp_p (insn_details)
	  && !amd64_absolute_call_p (insn_details)
	  && !amd64_ret_p (insn_details)))
    {
      int insn_len;

      CORE_ADDR pc = regcache_read_pc (regs);

      /* A signal trampoline system call changes the %rip, resuming
	 execution of the main program after the signal handler has
	 returned.  That makes them like 'return' instructions; we
	 shouldn't relocate %rip.

	 But most system calls don't, and we do need to relocate %rip.

	 Our heuristic for distinguishing these cases: if stepping
	 over the system call instruction left control directly after
	 the instruction, the we relocate --- control almost certainly
	 doesn't belong in the displaced copy.	Otherwise, we assume
	 the instruction has put control where it belongs, and leave
	 it unrelocated.  Goodness help us if there are PC-relative
	 system calls.	*/
      if (amd64_syscall_p (insn_details, &insn_len)
	  /* GDB can get control back after the insn after the syscall.
	     Presumably this is a kernel bug.  Fixup ensures it's a nop, we
	     add one to the length for it.  */
	  && (pc < to || pc > (to + insn_len + 1)))
	displaced_debug_printf ("syscall changed %%rip; not relocating");
      else
	{
	  CORE_ADDR rip = pc - insn_offset;

	  /* If we just stepped over a breakpoint insn, we don't backup
	     the pc on purpose; this is to match behaviour without
	     stepping.  */

	  regcache_write_pc (regs, rip);

	  displaced_debug_printf ("relocated %%rip from %s to %s",
				  paddress (gdbarch, pc),
				  paddress (gdbarch, rip));
	}
    }

  /* If the instruction was PUSHFL, then the TF bit will be set in the
     pushed value, and should be cleared.  We'll leave this for later,
     since GDB already messes up the TF flag when stepping over a
     pushfl.  */

  /* If the instruction was a call, the return address now atop the
     stack is the address following the copied instruction.  We need
     to make it the address following the original instruction.	 */
  if (completed_p && amd64_call_p (insn_details))
    {
      ULONGEST rsp;
      ULONGEST retaddr;
      const ULONGEST retaddr_len = 8;

      regcache_cooked_read_unsigned (regs, AMD64_RSP_REGNUM, &rsp);
      retaddr = read_memory_unsigned_integer (rsp, retaddr_len, byte_order);
      retaddr = (retaddr - insn_offset) & 0xffffffffffffffffULL;
      write_memory_unsigned_integer (rsp, retaddr_len, byte_order, retaddr);

      displaced_debug_printf ("relocated return addr at %s to %s",
			      paddress (gdbarch, rsp),
			      paddress (gdbarch, retaddr));
    }
}

/* If the instruction INSN uses RIP-relative addressing, return the
   offset into the raw INSN where the displacement to be adjusted is
   found.  Returns 0 if the instruction doesn't use RIP-relative
   addressing.  */

static int
rip_relative_offset (struct amd64_insn *insn)
{
  if (insn->modrm_offset != -1)
    {
      gdb_byte modrm = insn->raw_insn[insn->modrm_offset];

      if ((modrm & 0xc7) == 0x05)
	{
	  /* The displacement is found right after the ModRM byte.  */
	  return insn->modrm_offset + 1;
	}
    }

  return 0;
}

static void
append_insns (CORE_ADDR *to, ULONGEST len, const gdb_byte *buf)
{
  target_write_memory (*to, buf, len);
  *to += len;
}

static void
amd64_relocate_instruction (struct gdbarch *gdbarch,
			    CORE_ADDR *to, CORE_ADDR oldloc)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int len = gdbarch_max_insn_length (gdbarch);
  /* Extra space for sentinels.  */
  int fixup_sentinel_space = len;
  gdb::byte_vector buf (len + fixup_sentinel_space);
  struct amd64_insn insn_details;
  int offset = 0;
  LONGEST rel32, newrel;
  gdb_byte *insn;
  int insn_length;

  read_memory (oldloc, buf.data (), len);

  /* Set up the sentinel space so we don't have to worry about running
     off the end of the buffer.  An excessive number of leading prefixes
     could otherwise cause this.  */
  memset (buf.data () + len, 0, fixup_sentinel_space);

  insn = buf.data ();
  amd64_get_insn_details (insn, &insn_details);

  insn_length = gdb_buffered_insn_length (gdbarch, insn, len, oldloc);

  /* Skip legacy instruction prefixes.  */
  insn = amd64_skip_prefixes (insn);

  /* Adjust calls with 32-bit relative addresses as push/jump, with
     the address pushed being the location where the original call in
     the user program would return to.  */
  if (insn[0] == 0xe8)
    {
      gdb_byte push_buf[32];
      CORE_ADDR ret_addr;
      int i = 0;

      /* Where "ret" in the original code will return to.  */
      ret_addr = oldloc + insn_length;

      /* If pushing an address higher than or equal to 0x80000000,
	 avoid 'pushq', as that sign extends its 32-bit operand, which
	 would be incorrect.  */
      if (ret_addr <= 0x7fffffff)
	{
	  push_buf[0] = 0x68; /* pushq $...  */
	  store_unsigned_integer (&push_buf[1], 4, byte_order, ret_addr);
	  i = 5;
	}
      else
	{
	  push_buf[i++] = 0x48; /* sub    $0x8,%rsp */
	  push_buf[i++] = 0x83;
	  push_buf[i++] = 0xec;
	  push_buf[i++] = 0x08;

	  push_buf[i++] = 0xc7; /* movl    $imm,(%rsp) */
	  push_buf[i++] = 0x04;
	  push_buf[i++] = 0x24;
	  store_unsigned_integer (&push_buf[i], 4, byte_order,
				  ret_addr & 0xffffffff);
	  i += 4;

	  push_buf[i++] = 0xc7; /* movl    $imm,4(%rsp) */
	  push_buf[i++] = 0x44;
	  push_buf[i++] = 0x24;
	  push_buf[i++] = 0x04;
	  store_unsigned_integer (&push_buf[i], 4, byte_order,
				  ret_addr >> 32);
	  i += 4;
	}
      gdb_assert (i <= sizeof (push_buf));
      /* Push the push.  */
      append_insns (to, i, push_buf);

      /* Convert the relative call to a relative jump.  */
      insn[0] = 0xe9;

      /* Adjust the destination offset.  */
      rel32 = extract_signed_integer (insn + 1, 4, byte_order);
      newrel = (oldloc - *to) + rel32;
      store_signed_integer (insn + 1, 4, byte_order, newrel);

      displaced_debug_printf ("adjusted insn rel32=%s at %s to rel32=%s at %s",
			      hex_string (rel32), paddress (gdbarch, oldloc),
			      hex_string (newrel), paddress (gdbarch, *to));

      /* Write the adjusted jump into its displaced location.  */
      append_insns (to, 5, insn);
      return;
    }

  offset = rip_relative_offset (&insn_details);
  if (!offset)
    {
      /* Adjust jumps with 32-bit relative addresses.  Calls are
	 already handled above.  */
      if (insn[0] == 0xe9)
	offset = 1;
      /* Adjust conditional jumps.  */
      else if (insn[0] == 0x0f && (insn[1] & 0xf0) == 0x80)
	offset = 2;
    }

  if (offset)
    {
      rel32 = extract_signed_integer (insn + offset, 4, byte_order);
      newrel = (oldloc - *to) + rel32;
      store_signed_integer (insn + offset, 4, byte_order, newrel);
      displaced_debug_printf ("adjusted insn rel32=%s at %s to rel32=%s at %s",
			      hex_string (rel32), paddress (gdbarch, oldloc),
			      hex_string (newrel), paddress (gdbarch, *to));
    }

  /* Write the adjusted instruction into its displaced location.  */
  append_insns (to, insn_length, buf.data ());
}


/* The maximum number of saved registers.  This should include %rip.  */
#define AMD64_NUM_SAVED_REGS	AMD64_NUM_GREGS

struct amd64_frame_cache
{
  /* Base address.  */
  CORE_ADDR base;
  int base_p;
  CORE_ADDR sp_offset;
  CORE_ADDR pc;

  /* Saved registers.  */
  CORE_ADDR saved_regs[AMD64_NUM_SAVED_REGS];
  CORE_ADDR saved_sp;
  int saved_sp_reg;

  /* Do we have a frame?  */
  int frameless_p;
};

/* Initialize a frame cache.  */

static void
amd64_init_frame_cache (struct amd64_frame_cache *cache)
{
  int i;

  /* Base address.  */
  cache->base = 0;
  cache->base_p = 0;
  cache->sp_offset = -8;
  cache->pc = 0;

  /* Saved registers.  We initialize these to -1 since zero is a valid
     offset (that's where %rbp is supposed to be stored).
     The values start out as being offsets, and are later converted to
     addresses (at which point -1 is interpreted as an address, still meaning
     "invalid").  */
  for (i = 0; i < AMD64_NUM_SAVED_REGS; i++)
    cache->saved_regs[i] = -1;
  cache->saved_sp = 0;
  cache->saved_sp_reg = -1;

  /* Frameless until proven otherwise.  */
  cache->frameless_p = 1;
}

/* Allocate and initialize a frame cache.  */

static struct amd64_frame_cache *
amd64_alloc_frame_cache (void)
{
  struct amd64_frame_cache *cache;

  cache = FRAME_OBSTACK_ZALLOC (struct amd64_frame_cache);
  amd64_init_frame_cache (cache);
  return cache;
}

/* GCC 4.4 and later, can put code in the prologue to realign the
   stack pointer.  Check whether PC points to such code, and update
   CACHE accordingly.  Return the first instruction after the code
   sequence or CURRENT_PC, whichever is smaller.  If we don't
   recognize the code, return PC.  */

static CORE_ADDR
amd64_analyze_stack_align (CORE_ADDR pc, CORE_ADDR current_pc,
			   struct amd64_frame_cache *cache)
{
  /* There are 2 code sequences to re-align stack before the frame
     gets set up:

	1. Use a caller-saved saved register:

		leaq  8(%rsp), %reg
		andq  $-XXX, %rsp
		pushq -8(%reg)

	2. Use a callee-saved saved register:

		pushq %reg
		leaq  16(%rsp), %reg
		andq  $-XXX, %rsp
		pushq -8(%reg)

     "andq $-XXX, %rsp" can be either 4 bytes or 7 bytes:
     
	0x48 0x83 0xe4 0xf0			andq $-16, %rsp
	0x48 0x81 0xe4 0x00 0xff 0xff 0xff	andq $-256, %rsp
   */

  gdb_byte buf[18];
  int reg, r;
  int offset, offset_and;

  if (target_read_code (pc, buf, sizeof buf))
    return pc;

  /* Check caller-saved saved register.  The first instruction has
     to be "leaq 8(%rsp), %reg".  */
  if ((buf[0] & 0xfb) == 0x48
      && buf[1] == 0x8d
      && buf[3] == 0x24
      && buf[4] == 0x8)
    {
      /* MOD must be binary 10 and R/M must be binary 100.  */
      if ((buf[2] & 0xc7) != 0x44)
	return pc;

      /* REG has register number.  */
      reg = (buf[2] >> 3) & 7;

      /* Check the REX.R bit.  */
      if (buf[0] == 0x4c)
	reg += 8;

      offset = 5;
    }
  else
    {
      /* Check callee-saved saved register.  The first instruction
	 has to be "pushq %reg".  */
      reg = 0;
      if ((buf[0] & 0xf8) == 0x50)
	offset = 0;
      else if ((buf[0] & 0xf6) == 0x40
	       && (buf[1] & 0xf8) == 0x50)
	{
	  /* Check the REX.B bit.  */
	  if ((buf[0] & 1) != 0)
	    reg = 8;

	  offset = 1;
	}
      else
	return pc;

      /* Get register.  */
      reg += buf[offset] & 0x7;

      offset++;

      /* The next instruction has to be "leaq 16(%rsp), %reg".  */
      if ((buf[offset] & 0xfb) != 0x48
	  || buf[offset + 1] != 0x8d
	  || buf[offset + 3] != 0x24
	  || buf[offset + 4] != 0x10)
	return pc;

      /* MOD must be binary 10 and R/M must be binary 100.  */
      if ((buf[offset + 2] & 0xc7) != 0x44)
	return pc;
      
      /* REG has register number.  */
      r = (buf[offset + 2] >> 3) & 7;

      /* Check the REX.R bit.  */
      if (buf[offset] == 0x4c)
	r += 8;

      /* Registers in pushq and leaq have to be the same.  */
      if (reg != r)
	return pc;

      offset += 5;
    }

  /* Rigister can't be %rsp nor %rbp.  */
  if (reg == 4 || reg == 5)
    return pc;

  /* The next instruction has to be "andq $-XXX, %rsp".  */
  if (buf[offset] != 0x48
      || buf[offset + 2] != 0xe4
      || (buf[offset + 1] != 0x81 && buf[offset + 1] != 0x83))
    return pc;

  offset_and = offset;
  offset += buf[offset + 1] == 0x81 ? 7 : 4;

  /* The next instruction has to be "pushq -8(%reg)".  */
  r = 0;
  if (buf[offset] == 0xff)
    offset++;
  else if ((buf[offset] & 0xf6) == 0x40
	   && buf[offset + 1] == 0xff)
    {
      /* Check the REX.B bit.  */
      if ((buf[offset] & 0x1) != 0)
	r = 8;
      offset += 2;
    }
  else
    return pc;

  /* 8bit -8 is 0xf8.  REG must be binary 110 and MOD must be binary
     01.  */
  if (buf[offset + 1] != 0xf8
      || (buf[offset] & 0xf8) != 0x70)
    return pc;

  /* R/M has register.  */
  r += buf[offset] & 7;

  /* Registers in leaq and pushq have to be the same.  */
  if (reg != r)
    return pc;

  if (current_pc > pc + offset_and)
    cache->saved_sp_reg = amd64_arch_reg_to_regnum (reg);

  return std::min (pc + offset + 2, current_pc);
}

/* Similar to amd64_analyze_stack_align for x32.  */

static CORE_ADDR
amd64_x32_analyze_stack_align (CORE_ADDR pc, CORE_ADDR current_pc,
			       struct amd64_frame_cache *cache) 
{
  /* There are 2 code sequences to re-align stack before the frame
     gets set up:

	1. Use a caller-saved saved register:

		leaq  8(%rsp), %reg
		andq  $-XXX, %rsp
		pushq -8(%reg)

	   or

		[addr32] leal  8(%rsp), %reg
		andl  $-XXX, %esp
		[addr32] pushq -8(%reg)

	2. Use a callee-saved saved register:

		pushq %reg
		leaq  16(%rsp), %reg
		andq  $-XXX, %rsp
		pushq -8(%reg)

	   or

		pushq %reg
		[addr32] leal  16(%rsp), %reg
		andl  $-XXX, %esp
		[addr32] pushq -8(%reg)

     "andq $-XXX, %rsp" can be either 4 bytes or 7 bytes:
     
	0x48 0x83 0xe4 0xf0			andq $-16, %rsp
	0x48 0x81 0xe4 0x00 0xff 0xff 0xff	andq $-256, %rsp

     "andl $-XXX, %esp" can be either 3 bytes or 6 bytes:
     
	0x83 0xe4 0xf0			andl $-16, %esp
	0x81 0xe4 0x00 0xff 0xff 0xff	andl $-256, %esp
   */

  gdb_byte buf[19];
  int reg, r;
  int offset, offset_and;

  if (target_read_memory (pc, buf, sizeof buf))
    return pc;

  /* Skip optional addr32 prefix.  */
  offset = buf[0] == 0x67 ? 1 : 0;

  /* Check caller-saved saved register.  The first instruction has
     to be "leaq 8(%rsp), %reg" or "leal 8(%rsp), %reg".  */
  if (((buf[offset] & 0xfb) == 0x48 || (buf[offset] & 0xfb) == 0x40)
      && buf[offset + 1] == 0x8d
      && buf[offset + 3] == 0x24
      && buf[offset + 4] == 0x8)
    {
      /* MOD must be binary 10 and R/M must be binary 100.  */
      if ((buf[offset + 2] & 0xc7) != 0x44)
	return pc;

      /* REG has register number.  */
      reg = (buf[offset + 2] >> 3) & 7;

      /* Check the REX.R bit.  */
      if ((buf[offset] & 0x4) != 0)
	reg += 8;

      offset += 5;
    }
  else
    {
      /* Check callee-saved saved register.  The first instruction
	 has to be "pushq %reg".  */
      reg = 0;
      if ((buf[offset] & 0xf6) == 0x40
	  && (buf[offset + 1] & 0xf8) == 0x50)
	{
	  /* Check the REX.B bit.  */
	  if ((buf[offset] & 1) != 0)
	    reg = 8;

	  offset += 1;
	}
      else if ((buf[offset] & 0xf8) != 0x50)
	return pc;

      /* Get register.  */
      reg += buf[offset] & 0x7;

      offset++;

      /* Skip optional addr32 prefix.  */
      if (buf[offset] == 0x67)
	offset++;

      /* The next instruction has to be "leaq 16(%rsp), %reg" or
	 "leal 16(%rsp), %reg".  */
      if (((buf[offset] & 0xfb) != 0x48 && (buf[offset] & 0xfb) != 0x40)
	  || buf[offset + 1] != 0x8d
	  || buf[offset + 3] != 0x24
	  || buf[offset + 4] != 0x10)
	return pc;

      /* MOD must be binary 10 and R/M must be binary 100.  */
      if ((buf[offset + 2] & 0xc7) != 0x44)
	return pc;
      
      /* REG has register number.  */
      r = (buf[offset + 2] >> 3) & 7;

      /* Check the REX.R bit.  */
      if ((buf[offset] & 0x4) != 0)
	r += 8;

      /* Registers in pushq and leaq have to be the same.  */
      if (reg != r)
	return pc;

      offset += 5;
    }

  /* Rigister can't be %rsp nor %rbp.  */
  if (reg == 4 || reg == 5)
    return pc;

  /* The next instruction may be "andq $-XXX, %rsp" or
     "andl $-XXX, %esp".  */
  if (buf[offset] != 0x48)
    offset--;

  if (buf[offset + 2] != 0xe4
      || (buf[offset + 1] != 0x81 && buf[offset + 1] != 0x83))
    return pc;

  offset_and = offset;
  offset += buf[offset + 1] == 0x81 ? 7 : 4;

  /* Skip optional addr32 prefix.  */
  if (buf[offset] == 0x67)
    offset++;

  /* The next instruction has to be "pushq -8(%reg)".  */
  r = 0;
  if (buf[offset] == 0xff)
    offset++;
  else if ((buf[offset] & 0xf6) == 0x40
	   && buf[offset + 1] == 0xff)
    {
      /* Check the REX.B bit.  */
      if ((buf[offset] & 0x1) != 0)
	r = 8;
      offset += 2;
    }
  else
    return pc;

  /* 8bit -8 is 0xf8.  REG must be binary 110 and MOD must be binary
     01.  */
  if (buf[offset + 1] != 0xf8
      || (buf[offset] & 0xf8) != 0x70)
    return pc;

  /* R/M has register.  */
  r += buf[offset] & 7;

  /* Registers in leaq and pushq have to be the same.  */
  if (reg != r)
    return pc;

  if (current_pc > pc + offset_and)
    cache->saved_sp_reg = amd64_arch_reg_to_regnum (reg);

  return std::min (pc + offset + 2, current_pc);
}

/* Do a limited analysis of the prologue at PC and update CACHE
   accordingly.  Bail out early if CURRENT_PC is reached.  Return the
   address where the analysis stopped.

   We will handle only functions beginning with:

      pushq %rbp        0x55
      movq %rsp, %rbp   0x48 0x89 0xe5 (or 0x48 0x8b 0xec)

   or (for the X32 ABI):

      pushq %rbp        0x55
      movl %esp, %ebp   0x89 0xe5 (or 0x8b 0xec)

   The `endbr64` instruction can be found before these sequences, and will be
   skipped if found.

   Any function that doesn't start with one of these sequences will be
   assumed to have no prologue and thus no valid frame pointer in
   %rbp.  */

static CORE_ADDR
amd64_analyze_prologue (struct gdbarch *gdbarch,
			CORE_ADDR pc, CORE_ADDR current_pc,
			struct amd64_frame_cache *cache)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  /* The `endbr64` instruction.  */
  static const gdb_byte endbr64[4] = { 0xf3, 0x0f, 0x1e, 0xfa };
  /* There are two variations of movq %rsp, %rbp.  */
  static const gdb_byte mov_rsp_rbp_1[3] = { 0x48, 0x89, 0xe5 };
  static const gdb_byte mov_rsp_rbp_2[3] = { 0x48, 0x8b, 0xec };
  /* Ditto for movl %esp, %ebp.  */
  static const gdb_byte mov_esp_ebp_1[2] = { 0x89, 0xe5 };
  static const gdb_byte mov_esp_ebp_2[2] = { 0x8b, 0xec };

  gdb_byte buf[3];
  gdb_byte op;

  if (current_pc <= pc)
    return current_pc;

  if (gdbarch_ptr_bit (gdbarch) == 32)
    pc = amd64_x32_analyze_stack_align (pc, current_pc, cache);
  else
    pc = amd64_analyze_stack_align (pc, current_pc, cache);

  op = read_code_unsigned_integer (pc, 1, byte_order);

  /* Check for the `endbr64` instruction, skip it if found.  */
  if (op == endbr64[0])
    {
      read_code (pc + 1, buf, 3);

      if (memcmp (buf, &endbr64[1], 3) == 0)
	pc += 4;

      op = read_code_unsigned_integer (pc, 1, byte_order);
    }

  if (current_pc <= pc)
    return current_pc;

  if (op == 0x55)		/* pushq %rbp */
    {
      /* Take into account that we've executed the `pushq %rbp' that
	 starts this instruction sequence.  */
      cache->saved_regs[AMD64_RBP_REGNUM] = 0;
      cache->sp_offset += 8;

      /* If that's all, return now.  */
      if (current_pc <= pc + 1)
	return current_pc;

      read_code (pc + 1, buf, 3);

      /* Check for `movq %rsp, %rbp'.  */
      if (memcmp (buf, mov_rsp_rbp_1, 3) == 0
	  || memcmp (buf, mov_rsp_rbp_2, 3) == 0)
	{
	  /* OK, we actually have a frame.  */
	  cache->frameless_p = 0;
	  return pc + 4;
	}

      /* For X32, also check for `movl %esp, %ebp'.  */
      if (gdbarch_ptr_bit (gdbarch) == 32)
	{
	  if (memcmp (buf, mov_esp_ebp_1, 2) == 0
	      || memcmp (buf, mov_esp_ebp_2, 2) == 0)
	    {
	      /* OK, we actually have a frame.  */
	      cache->frameless_p = 0;
	      return pc + 3;
	    }
	}

      return pc + 1;
    }

  return pc;
}

/* Work around false termination of prologue - GCC PR debug/48827.

   START_PC is the first instruction of a function, PC is its minimal already
   determined advanced address.  Function returns PC if it has nothing to do.

   84 c0                test   %al,%al
   74 23                je     after
   <-- here is 0 lines advance - the false prologue end marker.
   0f 29 85 70 ff ff ff movaps %xmm0,-0x90(%rbp)
   0f 29 4d 80          movaps %xmm1,-0x80(%rbp)
   0f 29 55 90          movaps %xmm2,-0x70(%rbp)
   0f 29 5d a0          movaps %xmm3,-0x60(%rbp)
   0f 29 65 b0          movaps %xmm4,-0x50(%rbp)
   0f 29 6d c0          movaps %xmm5,-0x40(%rbp)
   0f 29 75 d0          movaps %xmm6,-0x30(%rbp)
   0f 29 7d e0          movaps %xmm7,-0x20(%rbp)
   after:  */

static CORE_ADDR
amd64_skip_xmm_prologue (CORE_ADDR pc, CORE_ADDR start_pc)
{
  struct symtab_and_line start_pc_sal, next_sal;
  gdb_byte buf[4 + 8 * 7];
  int offset, xmmreg;

  if (pc == start_pc)
    return pc;

  start_pc_sal = find_pc_sect_line (start_pc, NULL, 0);
  if (start_pc_sal.symtab == NULL
      || producer_is_gcc_ge_4 (start_pc_sal.symtab->compunit ()
			       ->producer ()) < 6
      || start_pc_sal.pc != start_pc || pc >= start_pc_sal.end)
    return pc;

  next_sal = find_pc_sect_line (start_pc_sal.end, NULL, 0);
  if (next_sal.line != start_pc_sal.line)
    return pc;

  /* START_PC can be from overlayed memory, ignored here.  */
  if (target_read_code (next_sal.pc - 4, buf, sizeof (buf)) != 0)
    return pc;

  /* test %al,%al */
  if (buf[0] != 0x84 || buf[1] != 0xc0)
    return pc;
  /* je AFTER */
  if (buf[2] != 0x74)
    return pc;

  offset = 4;
  for (xmmreg = 0; xmmreg < 8; xmmreg++)
    {
      /* 0x0f 0x29 0b??000101 movaps %xmmreg?,-0x??(%rbp) */
      if (buf[offset] != 0x0f || buf[offset + 1] != 0x29
	  || (buf[offset + 2] & 0x3f) != (xmmreg << 3 | 0x5))
	return pc;

      /* 0b01?????? */
      if ((buf[offset + 2] & 0xc0) == 0x40)
	{
	  /* 8-bit displacement.  */
	  offset += 4;
	}
      /* 0b10?????? */
      else if ((buf[offset + 2] & 0xc0) == 0x80)
	{
	  /* 32-bit displacement.  */
	  offset += 7;
	}
      else
	return pc;
    }

  /* je AFTER */
  if (offset - 4 != buf[3])
    return pc;

  return next_sal.end;
}

/* Return PC of first real instruction.  */

static CORE_ADDR
amd64_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR start_pc)
{
  struct amd64_frame_cache cache;
  CORE_ADDR pc;
  CORE_ADDR func_addr;

  if (find_pc_partial_function (start_pc, NULL, &func_addr, NULL))
    {
      CORE_ADDR post_prologue_pc
	= skip_prologue_using_sal (gdbarch, func_addr);
      struct compunit_symtab *cust = find_pc_compunit_symtab (func_addr);

      /* LLVM backend (Clang/Flang) always emits a line note before the
	 prologue and another one after.  We trust clang and newer Intel
	 compilers to emit usable line notes.  */
      if (post_prologue_pc
	  && (cust != NULL
	      && cust->producer () != nullptr
	      && (producer_is_llvm (cust->producer ())
	      || producer_is_icc_ge_19 (cust->producer ()))))
	return std::max (start_pc, post_prologue_pc);
    }

  amd64_init_frame_cache (&cache);
  pc = amd64_analyze_prologue (gdbarch, start_pc, 0xffffffffffffffffLL,
			       &cache);
  if (cache.frameless_p)
    return start_pc;

  return amd64_skip_xmm_prologue (pc, start_pc);
}


/* Normal frames.  */

static void
amd64_frame_cache_1 (frame_info_ptr this_frame,
		     struct amd64_frame_cache *cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte buf[8];
  int i;

  cache->pc = get_frame_func (this_frame);
  if (cache->pc != 0)
    amd64_analyze_prologue (gdbarch, cache->pc, get_frame_pc (this_frame),
			    cache);

  if (cache->frameless_p)
    {
      /* We didn't find a valid frame.  If we're at the start of a
	 function, or somewhere half-way its prologue, the function's
	 frame probably hasn't been fully setup yet.  Try to
	 reconstruct the base address for the stack frame by looking
	 at the stack pointer.  For truly "frameless" functions this
	 might work too.  */

      if (cache->saved_sp_reg != -1)
	{
	  /* Stack pointer has been saved.  */
	  get_frame_register (this_frame, cache->saved_sp_reg, buf);
	  cache->saved_sp = extract_unsigned_integer (buf, 8, byte_order);

	  /* We're halfway aligning the stack.  */
	  cache->base = ((cache->saved_sp - 8) & 0xfffffffffffffff0LL) - 8;
	  cache->saved_regs[AMD64_RIP_REGNUM] = cache->saved_sp - 8;

	  /* This will be added back below.  */
	  cache->saved_regs[AMD64_RIP_REGNUM] -= cache->base;
	}
      else
	{
	  get_frame_register (this_frame, AMD64_RSP_REGNUM, buf);
	  cache->base = extract_unsigned_integer (buf, 8, byte_order)
			+ cache->sp_offset;
	}
    }
  else
    {
      get_frame_register (this_frame, AMD64_RBP_REGNUM, buf);
      cache->base = extract_unsigned_integer (buf, 8, byte_order);
    }

  /* Now that we have the base address for the stack frame we can
     calculate the value of %rsp in the calling frame.  */
  cache->saved_sp = cache->base + 16;

  /* For normal frames, %rip is stored at 8(%rbp).  If we don't have a
     frame we find it at the same offset from the reconstructed base
     address.  If we're halfway aligning the stack, %rip is handled
     differently (see above).  */
  if (!cache->frameless_p || cache->saved_sp_reg == -1)
    cache->saved_regs[AMD64_RIP_REGNUM] = 8;

  /* Adjust all the saved registers such that they contain addresses
     instead of offsets.  */
  for (i = 0; i < AMD64_NUM_SAVED_REGS; i++)
    if (cache->saved_regs[i] != -1)
      cache->saved_regs[i] += cache->base;

  cache->base_p = 1;
}

static struct amd64_frame_cache *
amd64_frame_cache (frame_info_ptr this_frame, void **this_cache)
{
  struct amd64_frame_cache *cache;

  if (*this_cache)
    return (struct amd64_frame_cache *) *this_cache;

  cache = amd64_alloc_frame_cache ();
  *this_cache = cache;

  try
    {
      amd64_frame_cache_1 (this_frame, cache);
    }
  catch (const gdb_exception_error &ex)
    {
      if (ex.error != NOT_AVAILABLE_ERROR)
	throw;
    }

  return cache;
}

static enum unwind_stop_reason
amd64_frame_unwind_stop_reason (frame_info_ptr this_frame,
				void **this_cache)
{
  struct amd64_frame_cache *cache =
    amd64_frame_cache (this_frame, this_cache);

  if (!cache->base_p)
    return UNWIND_UNAVAILABLE;

  /* This marks the outermost frame.  */
  if (cache->base == 0)
    return UNWIND_OUTERMOST;

  return UNWIND_NO_REASON;
}

static void
amd64_frame_this_id (frame_info_ptr this_frame, void **this_cache,
		     struct frame_id *this_id)
{
  struct amd64_frame_cache *cache =
    amd64_frame_cache (this_frame, this_cache);

  if (!cache->base_p)
    (*this_id) = frame_id_build_unavailable_stack (cache->pc);
  else if (cache->base == 0)
    {
      /* This marks the outermost frame.  */
      return;
    }
  else
    (*this_id) = frame_id_build (cache->base + 16, cache->pc);
}

static struct value *
amd64_frame_prev_register (frame_info_ptr this_frame, void **this_cache,
			   int regnum)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct amd64_frame_cache *cache =
    amd64_frame_cache (this_frame, this_cache);

  gdb_assert (regnum >= 0);

  if (regnum == gdbarch_sp_regnum (gdbarch) && cache->saved_sp)
    return frame_unwind_got_constant (this_frame, regnum, cache->saved_sp);

  if (regnum < AMD64_NUM_SAVED_REGS && cache->saved_regs[regnum] != -1)
    return frame_unwind_got_memory (this_frame, regnum,
				    cache->saved_regs[regnum]);

  return frame_unwind_got_register (this_frame, regnum, regnum);
}

static const struct frame_unwind amd64_frame_unwind =
{
  "amd64 prologue",
  NORMAL_FRAME,
  amd64_frame_unwind_stop_reason,
  amd64_frame_this_id,
  amd64_frame_prev_register,
  NULL,
  default_frame_sniffer
};

/* Generate a bytecode expression to get the value of the saved PC.  */

static void
amd64_gen_return_address (struct gdbarch *gdbarch,
			  struct agent_expr *ax, struct axs_value *value,
			  CORE_ADDR scope)
{
  /* The following sequence assumes the traditional use of the base
     register.  */
  ax_reg (ax, AMD64_RBP_REGNUM);
  ax_const_l (ax, 8);
  ax_simple (ax, aop_add);
  value->type = register_type (gdbarch, AMD64_RIP_REGNUM);
  value->kind = axs_lvalue_memory;
}


/* Signal trampolines.  */

/* FIXME: kettenis/20030419: Perhaps, we can unify the 32-bit and
   64-bit variants.  This would require using identical frame caches
   on both platforms.  */

static struct amd64_frame_cache *
amd64_sigtramp_frame_cache (frame_info_ptr this_frame, void **this_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  struct amd64_frame_cache *cache;
  CORE_ADDR addr;
  gdb_byte buf[8];
  int i;

  if (*this_cache)
    return (struct amd64_frame_cache *) *this_cache;

  cache = amd64_alloc_frame_cache ();

  try
    {
      get_frame_register (this_frame, AMD64_RSP_REGNUM, buf);
      cache->base = extract_unsigned_integer (buf, 8, byte_order) - 8;

      addr = tdep->sigcontext_addr (this_frame);
      gdb_assert (tdep->sc_reg_offset);
      gdb_assert (tdep->sc_num_regs <= AMD64_NUM_SAVED_REGS);
      for (i = 0; i < tdep->sc_num_regs; i++)
	if (tdep->sc_reg_offset[i] != -1)
	  cache->saved_regs[i] = addr + tdep->sc_reg_offset[i];

      cache->base_p = 1;
    }
  catch (const gdb_exception_error &ex)
    {
      if (ex.error != NOT_AVAILABLE_ERROR)
	throw;
    }

  *this_cache = cache;
  return cache;
}

static enum unwind_stop_reason
amd64_sigtramp_frame_unwind_stop_reason (frame_info_ptr this_frame,
					 void **this_cache)
{
  struct amd64_frame_cache *cache =
    amd64_sigtramp_frame_cache (this_frame, this_cache);

  if (!cache->base_p)
    return UNWIND_UNAVAILABLE;

  return UNWIND_NO_REASON;
}

static void
amd64_sigtramp_frame_this_id (frame_info_ptr this_frame,
			      void **this_cache, struct frame_id *this_id)
{
  struct amd64_frame_cache *cache =
    amd64_sigtramp_frame_cache (this_frame, this_cache);

  if (!cache->base_p)
    (*this_id) = frame_id_build_unavailable_stack (get_frame_pc (this_frame));
  else if (cache->base == 0)
    {
      /* This marks the outermost frame.  */
      return;
    }
  else
    (*this_id) = frame_id_build (cache->base + 16, get_frame_pc (this_frame));
}

static struct value *
amd64_sigtramp_frame_prev_register (frame_info_ptr this_frame,
				    void **this_cache, int regnum)
{
  /* Make sure we've initialized the cache.  */
  amd64_sigtramp_frame_cache (this_frame, this_cache);

  return amd64_frame_prev_register (this_frame, this_cache, regnum);
}

static int
amd64_sigtramp_frame_sniffer (const struct frame_unwind *self,
			      frame_info_ptr this_frame,
			      void **this_cache)
{
  gdbarch *arch = get_frame_arch (this_frame);
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (arch);

  /* We shouldn't even bother if we don't have a sigcontext_addr
     handler.  */
  if (tdep->sigcontext_addr == NULL)
    return 0;

  if (tdep->sigtramp_p != NULL)
    {
      if (tdep->sigtramp_p (this_frame))
	return 1;
    }

  if (tdep->sigtramp_start != 0)
    {
      CORE_ADDR pc = get_frame_pc (this_frame);

      gdb_assert (tdep->sigtramp_end != 0);
      if (pc >= tdep->sigtramp_start && pc < tdep->sigtramp_end)
	return 1;
    }

  return 0;
}

static const struct frame_unwind amd64_sigtramp_frame_unwind =
{
  "amd64 sigtramp",
  SIGTRAMP_FRAME,
  amd64_sigtramp_frame_unwind_stop_reason,
  amd64_sigtramp_frame_this_id,
  amd64_sigtramp_frame_prev_register,
  NULL,
  amd64_sigtramp_frame_sniffer
};


static CORE_ADDR
amd64_frame_base_address (frame_info_ptr this_frame, void **this_cache)
{
  struct amd64_frame_cache *cache =
    amd64_frame_cache (this_frame, this_cache);

  return cache->base;
}

static const struct frame_base amd64_frame_base =
{
  &amd64_frame_unwind,
  amd64_frame_base_address,
  amd64_frame_base_address,
  amd64_frame_base_address
};

/* Implement core of the stack_frame_destroyed_p gdbarch method.  */

static int
amd64_stack_frame_destroyed_p_1 (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  gdb_byte insn;

  std::optional<CORE_ADDR> epilogue = find_epilogue_using_linetable (pc);

  /* PC is pointing at the next instruction to be executed. If it is
     equal to the epilogue start, it means we're right before it starts,
     so the stack is still valid.  */
  if (epilogue)
    return pc > epilogue;

  if (target_read_memory (pc, &insn, 1))
    return 0;   /* Can't read memory at pc.  */

  if (insn != 0xc3)     /* 'ret' instruction.  */
    return 0;

  return 1;
}

/* Normal frames, but in a function epilogue.  */

/* Implement the stack_frame_destroyed_p gdbarch method.

   The epilogue is defined here as the 'ret' instruction, which will
   follow any instruction such as 'leave' or 'pop %ebp' that destroys
   the function's stack frame.  */

static int
amd64_stack_frame_destroyed_p (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  struct compunit_symtab *cust = find_pc_compunit_symtab (pc);

  if (cust != nullptr && cust->producer () != nullptr
      && producer_is_llvm (cust->producer ()))
    return amd64_stack_frame_destroyed_p_1 (gdbarch, pc);

  return 0;
}

static int
amd64_epilogue_frame_sniffer_1 (const struct frame_unwind *self,
				frame_info_ptr this_frame,
				void **this_prologue_cache, bool override_p)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  CORE_ADDR pc = get_frame_pc (this_frame);

  if (frame_relative_level (this_frame) != 0)
    /* We're not in the inner frame, so assume we're not in an epilogue.  */
    return 0;

  bool unwind_valid_p
    = compunit_epilogue_unwind_valid (find_pc_compunit_symtab (pc));
  if (override_p)
    {
      if (unwind_valid_p)
	/* Don't override the symtab unwinders, skip
	   "amd64 epilogue override".  */
	return 0;
    }
  else
    {
      if (!unwind_valid_p)
	/* "amd64 epilogue override" unwinder already ran, skip
	   "amd64 epilogue".  */
	return 0;
    }

  /* Check whether we're in an epilogue.  */
  return amd64_stack_frame_destroyed_p_1 (gdbarch, pc);
}

static int
amd64_epilogue_override_frame_sniffer (const struct frame_unwind *self,
				       frame_info_ptr this_frame,
				       void **this_prologue_cache)
{
  return amd64_epilogue_frame_sniffer_1 (self, this_frame, this_prologue_cache,
					 true);
}

static int
amd64_epilogue_frame_sniffer (const struct frame_unwind *self,
			      frame_info_ptr this_frame,
			      void **this_prologue_cache)
{
  return amd64_epilogue_frame_sniffer_1 (self, this_frame, this_prologue_cache,
					 false);
}

static struct amd64_frame_cache *
amd64_epilogue_frame_cache (frame_info_ptr this_frame, void **this_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  struct amd64_frame_cache *cache;
  gdb_byte buf[8];

  if (*this_cache)
    return (struct amd64_frame_cache *) *this_cache;

  cache = amd64_alloc_frame_cache ();
  *this_cache = cache;

  try
    {
      /* Cache base will be %rsp plus cache->sp_offset (-8).  */
      get_frame_register (this_frame, AMD64_RSP_REGNUM, buf);
      cache->base = extract_unsigned_integer (buf, 8,
					      byte_order) + cache->sp_offset;

      /* Cache pc will be the frame func.  */
      cache->pc = get_frame_func (this_frame);

      /* The previous value of %rsp is cache->base plus 16.  */
      cache->saved_sp = cache->base + 16;

      /* The saved %rip will be at cache->base plus 8.  */
      cache->saved_regs[AMD64_RIP_REGNUM] = cache->base + 8;

      cache->base_p = 1;
    }
  catch (const gdb_exception_error &ex)
    {
      if (ex.error != NOT_AVAILABLE_ERROR)
	throw;
    }

  return cache;
}

static enum unwind_stop_reason
amd64_epilogue_frame_unwind_stop_reason (frame_info_ptr this_frame,
					 void **this_cache)
{
  struct amd64_frame_cache *cache
    = amd64_epilogue_frame_cache (this_frame, this_cache);

  if (!cache->base_p)
    return UNWIND_UNAVAILABLE;

  return UNWIND_NO_REASON;
}

static void
amd64_epilogue_frame_this_id (frame_info_ptr this_frame,
			      void **this_cache,
			      struct frame_id *this_id)
{
  struct amd64_frame_cache *cache = amd64_epilogue_frame_cache (this_frame,
							       this_cache);

  if (!cache->base_p)
    (*this_id) = frame_id_build_unavailable_stack (cache->pc);
  else
    (*this_id) = frame_id_build (cache->base + 16, cache->pc);
}

static const struct frame_unwind amd64_epilogue_override_frame_unwind =
{
  "amd64 epilogue override",
  NORMAL_FRAME,
  amd64_epilogue_frame_unwind_stop_reason,
  amd64_epilogue_frame_this_id,
  amd64_frame_prev_register,
  NULL,
  amd64_epilogue_override_frame_sniffer
};

static const struct frame_unwind amd64_epilogue_frame_unwind =
{
  "amd64 epilogue",
  NORMAL_FRAME,
  amd64_epilogue_frame_unwind_stop_reason,
  amd64_epilogue_frame_this_id,
  amd64_frame_prev_register,
  NULL, 
  amd64_epilogue_frame_sniffer
};

static struct frame_id
amd64_dummy_id (struct gdbarch *gdbarch, frame_info_ptr this_frame)
{
  CORE_ADDR fp;

  fp = get_frame_register_unsigned (this_frame, AMD64_RBP_REGNUM);

  return frame_id_build (fp + 16, get_frame_pc (this_frame));
}

/* 16 byte align the SP per frame requirements.  */

static CORE_ADDR
amd64_frame_align (struct gdbarch *gdbarch, CORE_ADDR sp)
{
  return sp & -(CORE_ADDR)16;
}


/* Supply register REGNUM from the buffer specified by FPREGS and LEN
   in the floating-point register set REGSET to register cache
   REGCACHE.  If REGNUM is -1, do this for all registers in REGSET.  */

static void
amd64_supply_fpregset (const struct regset *regset, struct regcache *regcache,
		       int regnum, const void *fpregs, size_t len)
{
  struct gdbarch *gdbarch = regcache->arch ();
  const i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);

  gdb_assert (len >= tdep->sizeof_fpregset);
  amd64_supply_fxsave (regcache, regnum, fpregs);
}

/* Collect register REGNUM from the register cache REGCACHE and store
   it in the buffer specified by FPREGS and LEN as described by the
   floating-point register set REGSET.  If REGNUM is -1, do this for
   all registers in REGSET.  */

static void
amd64_collect_fpregset (const struct regset *regset,
			const struct regcache *regcache,
			int regnum, void *fpregs, size_t len)
{
  struct gdbarch *gdbarch = regcache->arch ();
  const i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);

  gdb_assert (len >= tdep->sizeof_fpregset);
  amd64_collect_fxsave (regcache, regnum, fpregs);
}

const struct regset amd64_fpregset =
  {
    NULL, amd64_supply_fpregset, amd64_collect_fpregset
  };


/* Figure out where the longjmp will land.  Slurp the jmp_buf out of
   %rdi.  We expect its value to be a pointer to the jmp_buf structure
   from which we extract the address that we will land at.  This
   address is copied into PC.  This routine returns non-zero on
   success.  */

static int
amd64_get_longjmp_target (frame_info_ptr frame, CORE_ADDR *pc)
{
  gdb_byte buf[8];
  CORE_ADDR jb_addr;
  struct gdbarch *gdbarch = get_frame_arch (frame);
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);
  int jb_pc_offset = tdep->jb_pc_offset;
  int len = builtin_type (gdbarch)->builtin_func_ptr->length ();

  /* If JB_PC_OFFSET is -1, we have no way to find out where the
     longjmp will land.	 */
  if (jb_pc_offset == -1)
    return 0;

  get_frame_register (frame, AMD64_RDI_REGNUM, buf);
  jb_addr= extract_typed_address
	    (buf, builtin_type (gdbarch)->builtin_data_ptr);
  if (target_read_memory (jb_addr + jb_pc_offset, buf, len))
    return 0;

  *pc = extract_typed_address (buf, builtin_type (gdbarch)->builtin_func_ptr);

  return 1;
}

static const int amd64_record_regmap[] =
{
  AMD64_RAX_REGNUM, AMD64_RCX_REGNUM, AMD64_RDX_REGNUM, AMD64_RBX_REGNUM,
  AMD64_RSP_REGNUM, AMD64_RBP_REGNUM, AMD64_RSI_REGNUM, AMD64_RDI_REGNUM,
  AMD64_R8_REGNUM, AMD64_R9_REGNUM, AMD64_R10_REGNUM, AMD64_R11_REGNUM,
  AMD64_R12_REGNUM, AMD64_R13_REGNUM, AMD64_R14_REGNUM, AMD64_R15_REGNUM,
  AMD64_RIP_REGNUM, AMD64_EFLAGS_REGNUM, AMD64_CS_REGNUM, AMD64_SS_REGNUM,
  AMD64_DS_REGNUM, AMD64_ES_REGNUM, AMD64_FS_REGNUM, AMD64_GS_REGNUM
};

/* Implement the "in_indirect_branch_thunk" gdbarch function.  */

static bool
amd64_in_indirect_branch_thunk (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  return x86_in_indirect_branch_thunk (pc, amd64_register_names,
				       AMD64_RAX_REGNUM,
				       AMD64_RIP_REGNUM);
}

void
amd64_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch,
		const target_desc *default_tdesc)
{
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);
  const struct target_desc *tdesc = info.target_desc;
  static const char *const stap_integer_prefixes[] = { "$", NULL };
  static const char *const stap_register_prefixes[] = { "%", NULL };
  static const char *const stap_register_indirection_prefixes[] = { "(",
								    NULL };
  static const char *const stap_register_indirection_suffixes[] = { ")",
								    NULL };

  /* AMD64 generally uses `fxsave' instead of `fsave' for saving its
     floating-point registers.  */
  tdep->sizeof_fpregset = I387_SIZEOF_FXSAVE;
  tdep->fpregset = &amd64_fpregset;

  if (! tdesc_has_registers (tdesc))
    tdesc = default_tdesc;
  tdep->tdesc = tdesc;

  tdep->num_core_regs = AMD64_NUM_GREGS + I387_NUM_REGS;
  tdep->register_names = amd64_register_names;

  if (tdesc_find_feature (tdesc, "org.gnu.gdb.i386.avx512") != NULL)
    {
      tdep->zmmh_register_names = amd64_zmmh_names;
      tdep->k_register_names = amd64_k_names;
      tdep->xmm_avx512_register_names = amd64_xmm_avx512_names;
      tdep->ymm16h_register_names = amd64_ymmh_avx512_names;

      tdep->num_zmm_regs = 32;
      tdep->num_xmm_avx512_regs = 16;
      tdep->num_ymm_avx512_regs = 16;

      tdep->zmm0h_regnum = AMD64_ZMM0H_REGNUM;
      tdep->k0_regnum = AMD64_K0_REGNUM;
      tdep->xmm16_regnum = AMD64_XMM16_REGNUM;
      tdep->ymm16h_regnum = AMD64_YMM16H_REGNUM;
    }

  if (tdesc_find_feature (tdesc, "org.gnu.gdb.i386.avx") != NULL)
    {
      tdep->ymmh_register_names = amd64_ymmh_names;
      tdep->num_ymm_regs = 16;
      tdep->ymm0h_regnum = AMD64_YMM0H_REGNUM;
    }

  if (tdesc_find_feature (tdesc, "org.gnu.gdb.i386.mpx") != NULL)
    {
      tdep->mpx_register_names = amd64_mpx_names;
      tdep->bndcfgu_regnum = AMD64_BNDCFGU_REGNUM;
      tdep->bnd0r_regnum = AMD64_BND0R_REGNUM;
    }

  if (tdesc_find_feature (tdesc, "org.gnu.gdb.i386.segments") != NULL)
    {
      tdep->fsbase_regnum = AMD64_FSBASE_REGNUM;
    }

  if (tdesc_find_feature (tdesc, "org.gnu.gdb.i386.pkeys") != NULL)
    {
      tdep->pkeys_register_names = amd64_pkeys_names;
      tdep->pkru_regnum = AMD64_PKRU_REGNUM;
      tdep->num_pkeys_regs = 1;
    }

  tdep->num_byte_regs = 20;
  tdep->num_word_regs = 16;
  tdep->num_dword_regs = 16;
  /* Avoid wiring in the MMX registers for now.  */
  tdep->num_mmx_regs = 0;

  set_gdbarch_pseudo_register_read_value (gdbarch,
					  amd64_pseudo_register_read_value);
  set_gdbarch_pseudo_register_write (gdbarch, amd64_pseudo_register_write);
  set_gdbarch_ax_pseudo_register_collect (gdbarch,
					  amd64_ax_pseudo_register_collect);

  set_tdesc_pseudo_register_name (gdbarch, amd64_pseudo_register_name);

  /* AMD64 has an FPU and 16 SSE registers.  */
  tdep->st0_regnum = AMD64_ST0_REGNUM;
  tdep->num_xmm_regs = 16;

  /* This is what all the fuss is about.  */
  set_gdbarch_long_bit (gdbarch, 64);
  set_gdbarch_long_long_bit (gdbarch, 64);
  set_gdbarch_ptr_bit (gdbarch, 64);

  /* In contrast to the i386, on AMD64 a `long double' actually takes
     up 128 bits, even though it's still based on the i387 extended
     floating-point format which has only 80 significant bits.  */
  set_gdbarch_long_double_bit (gdbarch, 128);

  set_gdbarch_num_regs (gdbarch, AMD64_NUM_REGS);

  /* Register numbers of various important registers.  */
  set_gdbarch_sp_regnum (gdbarch, AMD64_RSP_REGNUM); /* %rsp */
  set_gdbarch_pc_regnum (gdbarch, AMD64_RIP_REGNUM); /* %rip */
  set_gdbarch_ps_regnum (gdbarch, AMD64_EFLAGS_REGNUM); /* %eflags */
  set_gdbarch_fp0_regnum (gdbarch, AMD64_ST0_REGNUM); /* %st(0) */

  /* The "default" register numbering scheme for AMD64 is referred to
     as the "DWARF Register Number Mapping" in the System V psABI.
     The preferred debugging format for all known AMD64 targets is
     actually DWARF2, and GCC doesn't seem to support DWARF (that is
     DWARF-1), but we provide the same mapping just in case.  This
     mapping is also used for stabs, which GCC does support.  */
  set_gdbarch_stab_reg_to_regnum (gdbarch, amd64_dwarf_reg_to_regnum);
  set_gdbarch_dwarf2_reg_to_regnum (gdbarch, amd64_dwarf_reg_to_regnum);

  /* We don't override SDB_REG_RO_REGNUM, since COFF doesn't seem to
     be in use on any of the supported AMD64 targets.  */

  /* Call dummy code.  */
  set_gdbarch_push_dummy_call (gdbarch, amd64_push_dummy_call);
  set_gdbarch_frame_align (gdbarch, amd64_frame_align);
  set_gdbarch_frame_red_zone_size (gdbarch, 128);

  set_gdbarch_convert_register_p (gdbarch, i387_convert_register_p);
  set_gdbarch_register_to_value (gdbarch, i387_register_to_value);
  set_gdbarch_value_to_register (gdbarch, i387_value_to_register);

  set_gdbarch_return_value_as_value (gdbarch, amd64_return_value);

  set_gdbarch_skip_prologue (gdbarch, amd64_skip_prologue);

  tdep->record_regmap = amd64_record_regmap;

  set_gdbarch_dummy_id (gdbarch, amd64_dummy_id);

  /* Hook the function epilogue frame unwinder.  This unwinder is
     appended to the list first, so that it supersedes the other
     unwinders in function epilogues.  */
  frame_unwind_prepend_unwinder (gdbarch, &amd64_epilogue_override_frame_unwind);

  frame_unwind_append_unwinder (gdbarch, &amd64_epilogue_frame_unwind);

  /* Hook the prologue-based frame unwinders.  */
  frame_unwind_append_unwinder (gdbarch, &amd64_sigtramp_frame_unwind);
  frame_unwind_append_unwinder (gdbarch, &amd64_frame_unwind);
  frame_base_set_default (gdbarch, &amd64_frame_base);

  set_gdbarch_get_longjmp_target (gdbarch, amd64_get_longjmp_target);

  set_gdbarch_relocate_instruction (gdbarch, amd64_relocate_instruction);

  set_gdbarch_gen_return_address (gdbarch, amd64_gen_return_address);

  set_gdbarch_stack_frame_destroyed_p (gdbarch, amd64_stack_frame_destroyed_p);

  /* SystemTap variables and functions.  */
  set_gdbarch_stap_integer_prefixes (gdbarch, stap_integer_prefixes);
  set_gdbarch_stap_register_prefixes (gdbarch, stap_register_prefixes);
  set_gdbarch_stap_register_indirection_prefixes (gdbarch,
					  stap_register_indirection_prefixes);
  set_gdbarch_stap_register_indirection_suffixes (gdbarch,
					  stap_register_indirection_suffixes);
  set_gdbarch_stap_is_single_operand (gdbarch,
				      i386_stap_is_single_operand);
  set_gdbarch_stap_parse_special_token (gdbarch,
					i386_stap_parse_special_token);
  set_gdbarch_insn_is_call (gdbarch, amd64_insn_is_call);
  set_gdbarch_insn_is_ret (gdbarch, amd64_insn_is_ret);
  set_gdbarch_insn_is_jump (gdbarch, amd64_insn_is_jump);

  set_gdbarch_in_indirect_branch_thunk (gdbarch,
					amd64_in_indirect_branch_thunk);

  register_amd64_ravenscar_ops (gdbarch);
}

/* Initialize ARCH for x86-64, no osabi.  */

static void
amd64_none_init_abi (gdbarch_info info, gdbarch *arch)
{
  amd64_init_abi (info, arch, amd64_target_description (X86_XSTATE_SSE_MASK,
							true));
}

static struct type *
amd64_x32_pseudo_register_type (struct gdbarch *gdbarch, int regnum)
{
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);

  switch (regnum - tdep->eax_regnum)
    {
    case AMD64_RBP_REGNUM:	/* %ebp */
    case AMD64_RSP_REGNUM:	/* %esp */
      return builtin_type (gdbarch)->builtin_data_ptr;
    case AMD64_RIP_REGNUM:	/* %eip */
      return builtin_type (gdbarch)->builtin_func_ptr;
    }

  return i386_pseudo_register_type (gdbarch, regnum);
}

void
amd64_x32_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch,
		    const target_desc *default_tdesc)
{
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);

  amd64_init_abi (info, gdbarch, default_tdesc);

  tdep->num_dword_regs = 17;
  set_tdesc_pseudo_register_type (gdbarch, amd64_x32_pseudo_register_type);

  set_gdbarch_long_bit (gdbarch, 32);
  set_gdbarch_ptr_bit (gdbarch, 32);
}

/* Initialize ARCH for x64-32, no osabi.  */

static void
amd64_x32_none_init_abi (gdbarch_info info, gdbarch *arch)
{
  amd64_x32_init_abi (info, arch,
		      amd64_target_description (X86_XSTATE_SSE_MASK, true));
}

/* Return the target description for a specified XSAVE feature mask.  */

const struct target_desc *
amd64_target_description (uint64_t xcr0, bool segments)
{
  static target_desc *amd64_tdescs \
    [2/*AVX*/][2/*MPX*/][2/*AVX512*/][2/*PKRU*/][2/*segments*/] = {};
  target_desc **tdesc;

  tdesc = &amd64_tdescs[(xcr0 & X86_XSTATE_AVX) ? 1 : 0]
    [(xcr0 & X86_XSTATE_MPX) ? 1 : 0]
    [(xcr0 & X86_XSTATE_AVX512) ? 1 : 0]
    [(xcr0 & X86_XSTATE_PKRU) ? 1 : 0]
    [segments ? 1 : 0];

  if (*tdesc == NULL)
    *tdesc = amd64_create_target_description (xcr0, false, false,
					      segments);

  return *tdesc;
}

void _initialize_amd64_tdep ();
void
_initialize_amd64_tdep ()
{
  gdbarch_register_osabi (bfd_arch_i386, bfd_mach_x86_64, GDB_OSABI_NONE,
			  amd64_none_init_abi);
  gdbarch_register_osabi (bfd_arch_i386, bfd_mach_x64_32, GDB_OSABI_NONE,
			  amd64_x32_none_init_abi);
}


/* The 64-bit FXSAVE format differs from the 32-bit format in the
   sense that the instruction pointer and data pointer are simply
   64-bit offsets into the code segment and the data segment instead
   of a selector offset pair.  The functions below store the upper 32
   bits of these pointers (instead of just the 16-bits of the segment
   selector).  */

/* Fill register REGNUM in REGCACHE with the appropriate
   floating-point or SSE register value from *FXSAVE.  If REGNUM is
   -1, do this for all registers.  This function masks off any of the
   reserved bits in *FXSAVE.  */

void
amd64_supply_fxsave (struct regcache *regcache, int regnum,
		     const void *fxsave)
{
  struct gdbarch *gdbarch = regcache->arch ();
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);

  i387_supply_fxsave (regcache, regnum, fxsave);

  if (fxsave
      && gdbarch_bfd_arch_info (gdbarch)->bits_per_word == 64)
    {
      const gdb_byte *regs = (const gdb_byte *) fxsave;

      if (regnum == -1 || regnum == I387_FISEG_REGNUM (tdep))
	regcache->raw_supply (I387_FISEG_REGNUM (tdep), regs + 12);
      if (regnum == -1 || regnum == I387_FOSEG_REGNUM (tdep))
	regcache->raw_supply (I387_FOSEG_REGNUM (tdep), regs + 20);
    }
}

/* Similar to amd64_supply_fxsave, but use XSAVE extended state.  */

void
amd64_supply_xsave (struct regcache *regcache, int regnum,
		    const void *xsave)
{
  struct gdbarch *gdbarch = regcache->arch ();
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);

  i387_supply_xsave (regcache, regnum, xsave);

  if (xsave
      && gdbarch_bfd_arch_info (gdbarch)->bits_per_word == 64)
    {
      const gdb_byte *regs = (const gdb_byte *) xsave;
      ULONGEST clear_bv;

      clear_bv = i387_xsave_get_clear_bv (gdbarch, xsave);

      /* If the FISEG and FOSEG registers have not been initialised yet
	 (their CLEAR_BV bit is set) then their default values of zero will
	 have already been setup by I387_SUPPLY_XSAVE.  */
      if (!(clear_bv & X86_XSTATE_X87))
	{
	  if (regnum == -1 || regnum == I387_FISEG_REGNUM (tdep))
	    regcache->raw_supply (I387_FISEG_REGNUM (tdep), regs + 12);
	  if (regnum == -1 || regnum == I387_FOSEG_REGNUM (tdep))
	    regcache->raw_supply (I387_FOSEG_REGNUM (tdep), regs + 20);
	}
    }
}

/* Fill register REGNUM (if it is a floating-point or SSE register) in
   *FXSAVE with the value from REGCACHE.  If REGNUM is -1, do this for
   all registers.  This function doesn't touch any of the reserved
   bits in *FXSAVE.  */

void
amd64_collect_fxsave (const struct regcache *regcache, int regnum,
		      void *fxsave)
{
  struct gdbarch *gdbarch = regcache->arch ();
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);
  gdb_byte *regs = (gdb_byte *) fxsave;

  i387_collect_fxsave (regcache, regnum, fxsave);

  if (gdbarch_bfd_arch_info (gdbarch)->bits_per_word == 64)
    {
      if (regnum == -1 || regnum == I387_FISEG_REGNUM (tdep))
	regcache->raw_collect (I387_FISEG_REGNUM (tdep), regs + 12);
      if (regnum == -1 || regnum == I387_FOSEG_REGNUM (tdep))
	regcache->raw_collect (I387_FOSEG_REGNUM (tdep), regs + 20);
    }
}

/* Similar to amd64_collect_fxsave, but use XSAVE extended state.  */

void
amd64_collect_xsave (const struct regcache *regcache, int regnum,
		     void *xsave, int gcore)
{
  struct gdbarch *gdbarch = regcache->arch ();
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);
  gdb_byte *regs = (gdb_byte *) xsave;

  i387_collect_xsave (regcache, regnum, xsave, gcore);

  if (gdbarch_bfd_arch_info (gdbarch)->bits_per_word == 64)
    {
      if (regnum == -1 || regnum == I387_FISEG_REGNUM (tdep))
	regcache->raw_collect (I387_FISEG_REGNUM (tdep),
			      regs + 12);
      if (regnum == -1 || regnum == I387_FOSEG_REGNUM (tdep))
	regcache->raw_collect (I387_FOSEG_REGNUM (tdep),
			      regs + 20);
    }
}
