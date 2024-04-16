/* Target-dependent code for the IA-64 for GDB, the GNU debugger.

   Copyright (C) 1999-2024 Free Software Foundation, Inc.

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
#include "inferior.h"
#include "gdbcore.h"
#include "arch-utils.h"
#include "floatformat.h"
#include "gdbtypes.h"
#include "regcache.h"
#include "reggroups.h"
#include "frame.h"
#include "frame-base.h"
#include "frame-unwind.h"
#include "target-float.h"
#include "value.h"
#include "objfiles.h"
#include "elf/common.h"
#include "elf-bfd.h"
#include "dis-asm.h"
#include "infcall.h"
#include "osabi.h"
#include "ia64-tdep.h"
#include "cp-abi.h"

#ifdef HAVE_LIBUNWIND_IA64_H
#include "elf/ia64.h"
#include "ia64-libunwind-tdep.h"

/* Note: KERNEL_START is supposed to be an address which is not going
	 to ever contain any valid unwind info.  For ia64 linux, the choice
	 of 0xc000000000000000 is fairly safe since that's uncached space.
 
	 We use KERNEL_START as follows: after obtaining the kernel's
	 unwind table via getunwind(), we project its unwind data into
	 address-range KERNEL_START-(KERNEL_START+ktab_size) and then
	 when ia64_access_mem() sees a memory access to this
	 address-range, we redirect it to ktab instead.

	 None of this hackery is needed with a modern kernel/libcs
	 which uses the kernel virtual DSO to provide access to the
	 kernel's unwind info.  In that case, ktab_size remains 0 and
	 hence the value of KERNEL_START doesn't matter.  */

#define KERNEL_START 0xc000000000000000ULL

static size_t ktab_size = 0;
struct ia64_table_entry
  {
    uint64_t start_offset;
    uint64_t end_offset;
    uint64_t info_offset;
  };

static struct ia64_table_entry *ktab = NULL;
static std::optional<gdb::byte_vector> ktab_buf;

#endif

/* An enumeration of the different IA-64 instruction types.  */

enum ia64_instruction_type
{
  A,			/* Integer ALU ;    I-unit or M-unit */
  I,			/* Non-ALU integer; I-unit */
  M,			/* Memory ;         M-unit */
  F,			/* Floating-point ; F-unit */
  B,			/* Branch ;         B-unit */
  L,			/* Extended (L+X) ; I-unit */
  X,			/* Extended (L+X) ; I-unit */
  undefined		/* undefined or reserved */
};

/* We represent IA-64 PC addresses as the value of the instruction
   pointer or'd with some bit combination in the low nibble which
   represents the slot number in the bundle addressed by the
   instruction pointer.  The problem is that the Linux kernel
   multiplies its slot numbers (for exceptions) by one while the
   disassembler multiplies its slot numbers by 6.  In addition, I've
   heard it said that the simulator uses 1 as the multiplier.
   
   I've fixed the disassembler so that the bytes_per_line field will
   be the slot multiplier.  If bytes_per_line comes in as zero, it
   is set to six (which is how it was set up initially). -- objdump
   displays pretty disassembly dumps with this value.  For our purposes,
   we'll set bytes_per_line to SLOT_MULTIPLIER. This is okay since we
   never want to also display the raw bytes the way objdump does.  */

#define SLOT_MULTIPLIER 1

/* Length in bytes of an instruction bundle.  */

#define BUNDLE_LEN 16

/* See the saved memory layout comment for ia64_memory_insert_breakpoint.  */

#if BREAKPOINT_MAX < BUNDLE_LEN - 2
# error "BREAKPOINT_MAX < BUNDLE_LEN - 2"
#endif

static gdbarch_init_ftype ia64_gdbarch_init;

static gdbarch_register_name_ftype ia64_register_name;
static gdbarch_register_type_ftype ia64_register_type;
static gdbarch_breakpoint_from_pc_ftype ia64_breakpoint_from_pc;
static gdbarch_skip_prologue_ftype ia64_skip_prologue;
static struct type *is_float_or_hfa_type (struct type *t);
static CORE_ADDR ia64_find_global_pointer (struct gdbarch *gdbarch,
					   CORE_ADDR faddr);

#define NUM_IA64_RAW_REGS 462

/* Big enough to hold a FP register in bytes.  */
#define IA64_FP_REGISTER_SIZE 16

static int sp_regnum = IA64_GR12_REGNUM;

/* NOTE: we treat the register stack registers r32-r127 as
   pseudo-registers because they may not be accessible via the ptrace
   register get/set interfaces.  */

enum pseudo_regs { FIRST_PSEUDO_REGNUM = NUM_IA64_RAW_REGS,
		   VBOF_REGNUM = IA64_NAT127_REGNUM + 1, V32_REGNUM, 
		   V127_REGNUM = V32_REGNUM + 95, 
		   VP0_REGNUM, VP16_REGNUM = VP0_REGNUM + 16,
		   VP63_REGNUM = VP0_REGNUM + 63, LAST_PSEUDO_REGNUM };

/* Array of register names; There should be ia64_num_regs strings in
   the initializer.  */

static const char * const ia64_register_names[] =
{ "r0",   "r1",   "r2",   "r3",   "r4",   "r5",   "r6",   "r7",
  "r8",   "r9",   "r10",  "r11",  "r12",  "r13",  "r14",  "r15",
  "r16",  "r17",  "r18",  "r19",  "r20",  "r21",  "r22",  "r23",
  "r24",  "r25",  "r26",  "r27",  "r28",  "r29",  "r30",  "r31",
  "",     "",     "",     "",     "",     "",     "",     "",
  "",     "",     "",     "",     "",     "",     "",     "",
  "",     "",     "",     "",     "",     "",     "",     "",
  "",     "",     "",     "",     "",     "",     "",     "",
  "",     "",     "",     "",     "",     "",     "",     "",
  "",     "",     "",     "",     "",     "",     "",     "",
  "",     "",     "",     "",     "",     "",     "",     "",
  "",     "",     "",     "",     "",     "",     "",     "",
  "",     "",     "",     "",     "",     "",     "",     "",
  "",     "",     "",     "",     "",     "",     "",     "",
  "",     "",     "",     "",     "",     "",     "",     "",
  "",     "",     "",     "",     "",     "",     "",     "",

  "f0",   "f1",   "f2",   "f3",   "f4",   "f5",   "f6",   "f7",
  "f8",   "f9",   "f10",  "f11",  "f12",  "f13",  "f14",  "f15",
  "f16",  "f17",  "f18",  "f19",  "f20",  "f21",  "f22",  "f23",
  "f24",  "f25",  "f26",  "f27",  "f28",  "f29",  "f30",  "f31",
  "f32",  "f33",  "f34",  "f35",  "f36",  "f37",  "f38",  "f39",
  "f40",  "f41",  "f42",  "f43",  "f44",  "f45",  "f46",  "f47",
  "f48",  "f49",  "f50",  "f51",  "f52",  "f53",  "f54",  "f55",
  "f56",  "f57",  "f58",  "f59",  "f60",  "f61",  "f62",  "f63",
  "f64",  "f65",  "f66",  "f67",  "f68",  "f69",  "f70",  "f71",
  "f72",  "f73",  "f74",  "f75",  "f76",  "f77",  "f78",  "f79",
  "f80",  "f81",  "f82",  "f83",  "f84",  "f85",  "f86",  "f87",
  "f88",  "f89",  "f90",  "f91",  "f92",  "f93",  "f94",  "f95",
  "f96",  "f97",  "f98",  "f99",  "f100", "f101", "f102", "f103",
  "f104", "f105", "f106", "f107", "f108", "f109", "f110", "f111",
  "f112", "f113", "f114", "f115", "f116", "f117", "f118", "f119",
  "f120", "f121", "f122", "f123", "f124", "f125", "f126", "f127",

  "",     "",     "",     "",     "",     "",     "",     "",
  "",     "",     "",     "",     "",     "",     "",     "",
  "",     "",     "",     "",     "",     "",     "",     "",
  "",     "",     "",     "",     "",     "",     "",     "",
  "",     "",     "",     "",     "",     "",     "",     "",
  "",     "",     "",     "",     "",     "",     "",     "",
  "",     "",     "",     "",     "",     "",     "",     "",
  "",     "",     "",     "",     "",     "",     "",     "",

  "b0",   "b1",   "b2",   "b3",   "b4",   "b5",   "b6",   "b7",

  "vfp", "vrap",

  "pr", "ip", "psr", "cfm",

  "kr0",   "kr1",   "kr2",   "kr3",   "kr4",   "kr5",   "kr6",   "kr7",
  "", "", "", "", "", "", "", "",
  "rsc", "bsp", "bspstore", "rnat",
  "", "fcr", "", "",
  "eflag", "csd", "ssd", "cflg", "fsr", "fir", "fdr",  "",
  "ccv", "", "", "", "unat", "", "", "",
  "fpsr", "", "", "", "itc",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "",
  "pfs", "lc", "ec",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "", "", "",
  "",
  "nat0",  "nat1",  "nat2",  "nat3",  "nat4",  "nat5",  "nat6",  "nat7",
  "nat8",  "nat9",  "nat10", "nat11", "nat12", "nat13", "nat14", "nat15",
  "nat16", "nat17", "nat18", "nat19", "nat20", "nat21", "nat22", "nat23",
  "nat24", "nat25", "nat26", "nat27", "nat28", "nat29", "nat30", "nat31",
  "nat32", "nat33", "nat34", "nat35", "nat36", "nat37", "nat38", "nat39",
  "nat40", "nat41", "nat42", "nat43", "nat44", "nat45", "nat46", "nat47",
  "nat48", "nat49", "nat50", "nat51", "nat52", "nat53", "nat54", "nat55",
  "nat56", "nat57", "nat58", "nat59", "nat60", "nat61", "nat62", "nat63",
  "nat64", "nat65", "nat66", "nat67", "nat68", "nat69", "nat70", "nat71",
  "nat72", "nat73", "nat74", "nat75", "nat76", "nat77", "nat78", "nat79",
  "nat80", "nat81", "nat82", "nat83", "nat84", "nat85", "nat86", "nat87",
  "nat88", "nat89", "nat90", "nat91", "nat92", "nat93", "nat94", "nat95",
  "nat96", "nat97", "nat98", "nat99", "nat100","nat101","nat102","nat103",
  "nat104","nat105","nat106","nat107","nat108","nat109","nat110","nat111",
  "nat112","nat113","nat114","nat115","nat116","nat117","nat118","nat119",
  "nat120","nat121","nat122","nat123","nat124","nat125","nat126","nat127",

  "bof",
  
  "r32",  "r33",  "r34",  "r35",  "r36",  "r37",  "r38",  "r39",   
  "r40",  "r41",  "r42",  "r43",  "r44",  "r45",  "r46",  "r47",
  "r48",  "r49",  "r50",  "r51",  "r52",  "r53",  "r54",  "r55",
  "r56",  "r57",  "r58",  "r59",  "r60",  "r61",  "r62",  "r63",
  "r64",  "r65",  "r66",  "r67",  "r68",  "r69",  "r70",  "r71",
  "r72",  "r73",  "r74",  "r75",  "r76",  "r77",  "r78",  "r79",
  "r80",  "r81",  "r82",  "r83",  "r84",  "r85",  "r86",  "r87",
  "r88",  "r89",  "r90",  "r91",  "r92",  "r93",  "r94",  "r95",
  "r96",  "r97",  "r98",  "r99",  "r100", "r101", "r102", "r103",
  "r104", "r105", "r106", "r107", "r108", "r109", "r110", "r111",
  "r112", "r113", "r114", "r115", "r116", "r117", "r118", "r119",
  "r120", "r121", "r122", "r123", "r124", "r125", "r126", "r127",

  "p0",   "p1",   "p2",   "p3",   "p4",   "p5",   "p6",   "p7",
  "p8",   "p9",   "p10",  "p11",  "p12",  "p13",  "p14",  "p15",
  "p16",  "p17",  "p18",  "p19",  "p20",  "p21",  "p22",  "p23",
  "p24",  "p25",  "p26",  "p27",  "p28",  "p29",  "p30",  "p31",
  "p32",  "p33",  "p34",  "p35",  "p36",  "p37",  "p38",  "p39",
  "p40",  "p41",  "p42",  "p43",  "p44",  "p45",  "p46",  "p47",
  "p48",  "p49",  "p50",  "p51",  "p52",  "p53",  "p54",  "p55",
  "p56",  "p57",  "p58",  "p59",  "p60",  "p61",  "p62",  "p63",
};

struct ia64_frame_cache
{
  CORE_ADDR base;       /* frame pointer base for frame */
  CORE_ADDR pc;		/* function start pc for frame */
  CORE_ADDR saved_sp;	/* stack pointer for frame */
  CORE_ADDR bsp;	/* points at r32 for the current frame */
  CORE_ADDR cfm;	/* cfm value for current frame */
  CORE_ADDR prev_cfm;   /* cfm value for previous frame */
  int   frameless;
  int   sof;		/* Size of frame  (decoded from cfm value).  */
  int	sol;		/* Size of locals (decoded from cfm value).  */
  int	sor;		/* Number of rotating registers (decoded from
			   cfm value).  */
  CORE_ADDR after_prologue;
  /* Address of first instruction after the last
     prologue instruction;  Note that there may
     be instructions from the function's body
     intermingled with the prologue.  */
  int mem_stack_frame_size;
  /* Size of the memory stack frame (may be zero),
     or -1 if it has not been determined yet.  */
  int	fp_reg;		/* Register number (if any) used a frame pointer
			   for this frame.  0 if no register is being used
			   as the frame pointer.  */
  
  /* Saved registers.  */
  CORE_ADDR saved_regs[NUM_IA64_RAW_REGS];

};

static int
floatformat_valid (const struct floatformat *fmt, const void *from)
{
  return 1;
}

static const struct floatformat floatformat_ia64_ext_little =
{
  floatformat_little, 82, 0, 1, 17, 65535, 0x1ffff, 18, 64,
  floatformat_intbit_yes, "floatformat_ia64_ext_little", floatformat_valid, NULL
};

static const struct floatformat floatformat_ia64_ext_big =
{
  floatformat_big, 82, 46, 47, 17, 65535, 0x1ffff, 64, 64,
  floatformat_intbit_yes, "floatformat_ia64_ext_big", floatformat_valid
};

static const struct floatformat *floatformats_ia64_ext[2] =
{
  &floatformat_ia64_ext_big,
  &floatformat_ia64_ext_little
};

static struct type *
ia64_ext_type (struct gdbarch *gdbarch)
{
  ia64_gdbarch_tdep *tdep = gdbarch_tdep<ia64_gdbarch_tdep> (gdbarch);

  if (!tdep->ia64_ext_type)
    {
      type_allocator alloc (gdbarch);
      tdep->ia64_ext_type
	= init_float_type (alloc, 128, "builtin_type_ia64_ext",
			   floatformats_ia64_ext);
    }

  return tdep->ia64_ext_type;
}

static int
ia64_register_reggroup_p (struct gdbarch *gdbarch, int regnum,
			  const struct reggroup *group)
{
  int vector_p;
  int float_p;
  int raw_p;
  if (group == all_reggroup)
    return 1;
  vector_p = register_type (gdbarch, regnum)->is_vector ();
  float_p = register_type (gdbarch, regnum)->code () == TYPE_CODE_FLT;
  raw_p = regnum < NUM_IA64_RAW_REGS;
  if (group == float_reggroup)
    return float_p;
  if (group == vector_reggroup)
    return vector_p;
  if (group == general_reggroup)
    return (!vector_p && !float_p);
  if (group == save_reggroup || group == restore_reggroup)
    return raw_p; 
  return 0;
}

static const char *
ia64_register_name (struct gdbarch *gdbarch, int reg)
{
  return ia64_register_names[reg];
}

struct type *
ia64_register_type (struct gdbarch *arch, int reg)
{
  if (reg >= IA64_FR0_REGNUM && reg <= IA64_FR127_REGNUM)
    return ia64_ext_type (arch);
  else
    return builtin_type (arch)->builtin_long;
}

static int
ia64_dwarf_reg_to_regnum (struct gdbarch *gdbarch, int reg)
{
  if (reg >= IA64_GR32_REGNUM && reg <= IA64_GR127_REGNUM)
    return V32_REGNUM + (reg - IA64_GR32_REGNUM);
  return reg;
}


/* Extract ``len'' bits from an instruction bundle starting at
   bit ``from''.  */

static long long
extract_bit_field (const gdb_byte *bundle, int from, int len)
{
  long long result = 0LL;
  int to = from + len;
  int from_byte = from / 8;
  int to_byte = to / 8;
  unsigned char *b = (unsigned char *) bundle;
  unsigned char c;
  int lshift;
  int i;

  c = b[from_byte];
  if (from_byte == to_byte)
    c = ((unsigned char) (c << (8 - to % 8))) >> (8 - to % 8);
  result = c >> (from % 8);
  lshift = 8 - (from % 8);

  for (i = from_byte+1; i < to_byte; i++)
    {
      result |= ((long long) b[i]) << lshift;
      lshift += 8;
    }

  if (from_byte < to_byte && (to % 8 != 0))
    {
      c = b[to_byte];
      c = ((unsigned char) (c << (8 - to % 8))) >> (8 - to % 8);
      result |= ((long long) c) << lshift;
    }

  return result;
}

/* Replace the specified bits in an instruction bundle.  */

static void
replace_bit_field (gdb_byte *bundle, long long val, int from, int len)
{
  int to = from + len;
  int from_byte = from / 8;
  int to_byte = to / 8;
  unsigned char *b = (unsigned char *) bundle;
  unsigned char c;

  if (from_byte == to_byte)
    {
      unsigned char left, right;
      c = b[from_byte];
      left = (c >> (to % 8)) << (to % 8);
      right = ((unsigned char) (c << (8 - from % 8))) >> (8 - from % 8);
      c = (unsigned char) (val & 0xff);
      c = (unsigned char) (c << (from % 8 + 8 - to % 8)) >> (8 - to % 8);
      c |= right | left;
      b[from_byte] = c;
    }
  else
    {
      int i;
      c = b[from_byte];
      c = ((unsigned char) (c << (8 - from % 8))) >> (8 - from % 8);
      c = c | (val << (from % 8));
      b[from_byte] = c;
      val >>= 8 - from % 8;

      for (i = from_byte+1; i < to_byte; i++)
	{
	  c = val & 0xff;
	  val >>= 8;
	  b[i] = c;
	}

      if (to % 8 != 0)
	{
	  unsigned char cv = (unsigned char) val;
	  c = b[to_byte];
	  c = c >> (to % 8) << (to % 8);
	  c |= ((unsigned char) (cv << (8 - to % 8))) >> (8 - to % 8);
	  b[to_byte] = c;
	}
    }
}

/* Return the contents of slot N (for N = 0, 1, or 2) in
   and instruction bundle.  */

static long long
slotN_contents (gdb_byte *bundle, int slotnum)
{
  return extract_bit_field (bundle, 5+41*slotnum, 41);
}

/* Store an instruction in an instruction bundle.  */

static void
replace_slotN_contents (gdb_byte *bundle, long long instr, int slotnum)
{
  replace_bit_field (bundle, instr, 5+41*slotnum, 41);
}

static const enum ia64_instruction_type template_encoding_table[32][3] =
{
  { M, I, I },				/* 00 */
  { M, I, I },				/* 01 */
  { M, I, I },				/* 02 */
  { M, I, I },				/* 03 */
  { M, L, X },				/* 04 */
  { M, L, X },				/* 05 */
  { undefined, undefined, undefined },  /* 06 */
  { undefined, undefined, undefined },  /* 07 */
  { M, M, I },				/* 08 */
  { M, M, I },				/* 09 */
  { M, M, I },				/* 0A */
  { M, M, I },				/* 0B */
  { M, F, I },				/* 0C */
  { M, F, I },				/* 0D */
  { M, M, F },				/* 0E */
  { M, M, F },				/* 0F */
  { M, I, B },				/* 10 */
  { M, I, B },				/* 11 */
  { M, B, B },				/* 12 */
  { M, B, B },				/* 13 */
  { undefined, undefined, undefined },  /* 14 */
  { undefined, undefined, undefined },  /* 15 */
  { B, B, B },				/* 16 */
  { B, B, B },				/* 17 */
  { M, M, B },				/* 18 */
  { M, M, B },				/* 19 */
  { undefined, undefined, undefined },  /* 1A */
  { undefined, undefined, undefined },  /* 1B */
  { M, F, B },				/* 1C */
  { M, F, B },				/* 1D */
  { undefined, undefined, undefined },  /* 1E */
  { undefined, undefined, undefined },  /* 1F */
};

/* Fetch and (partially) decode an instruction at ADDR and return the
   address of the next instruction to fetch.  */

static CORE_ADDR
fetch_instruction (CORE_ADDR addr, ia64_instruction_type *it, long long *instr)
{
  gdb_byte bundle[BUNDLE_LEN];
  int slotnum = (int) (addr & 0x0f) / SLOT_MULTIPLIER;
  long long templ;
  int val;

  /* Warn about slot numbers greater than 2.  We used to generate
     an error here on the assumption that the user entered an invalid
     address.  But, sometimes GDB itself requests an invalid address.
     This can (easily) happen when execution stops in a function for
     which there are no symbols.  The prologue scanner will attempt to
     find the beginning of the function - if the nearest symbol
     happens to not be aligned on a bundle boundary (16 bytes), the
     resulting starting address will cause GDB to think that the slot
     number is too large.

     So we warn about it and set the slot number to zero.  It is
     not necessarily a fatal condition, particularly if debugging
     at the assembly language level.  */
  if (slotnum > 2)
    {
      warning (_("Can't fetch instructions for slot numbers greater than 2.\n"
	       "Using slot 0 instead"));
      slotnum = 0;
    }

  addr &= ~0x0f;

  val = target_read_memory (addr, bundle, BUNDLE_LEN);

  if (val != 0)
    return 0;

  *instr = slotN_contents (bundle, slotnum);
  templ = extract_bit_field (bundle, 0, 5);
  *it = template_encoding_table[(int)templ][slotnum];

  if (slotnum == 2 || (slotnum == 1 && *it == L))
    addr += 16;
  else
    addr += (slotnum + 1) * SLOT_MULTIPLIER;

  return addr;
}

/* There are 5 different break instructions (break.i, break.b,
   break.m, break.f, and break.x), but they all have the same
   encoding.  (The five bit template in the low five bits of the
   instruction bundle distinguishes one from another.)
   
   The runtime architecture manual specifies that break instructions
   used for debugging purposes must have the upper two bits of the 21
   bit immediate set to a 0 and a 1 respectively.  A breakpoint
   instruction encodes the most significant bit of its 21 bit
   immediate at bit 36 of the 41 bit instruction.  The penultimate msb
   is at bit 25 which leads to the pattern below.  
   
   Originally, I had this set up to do, e.g, a "break.i 0x80000"  But
   it turns out that 0x80000 was used as the syscall break in the early
   simulators.  So I changed the pattern slightly to do "break.i 0x080001"
   instead.  But that didn't work either (I later found out that this
   pattern was used by the simulator that I was using.)  So I ended up
   using the pattern seen below.

   SHADOW_CONTENTS has byte-based addressing (PLACED_ADDRESS and SHADOW_LEN)
   while we need bit-based addressing as the instructions length is 41 bits and
   we must not modify/corrupt the adjacent slots in the same bundle.
   Fortunately we may store larger memory incl. the adjacent bits with the
   original memory content (not the possibly already stored breakpoints there).
   We need to be careful in ia64_memory_remove_breakpoint to always restore
   only the specific bits of this instruction ignoring any adjacent stored
   bits.

   We use the original addressing with the low nibble in the range <0..2> which
   gets incorrectly interpreted by generic non-ia64 breakpoint_restore_shadows
   as the direct byte offset of SHADOW_CONTENTS.  We store whole BUNDLE_LEN
   bytes just without these two possibly skipped bytes to not to exceed to the
   next bundle.

   If we would like to store the whole bundle to SHADOW_CONTENTS we would have
   to store already the base address (`address & ~0x0f') into PLACED_ADDRESS.
   In such case there is no other place where to store
   SLOTNUM (`adress & 0x0f', value in the range <0..2>).  We need to know
   SLOTNUM in ia64_memory_remove_breakpoint.

   There is one special case where we need to be extra careful:
   L-X instructions, which are instructions that occupy 2 slots
   (The L part is always in slot 1, and the X part is always in
   slot 2).  We must refuse to insert breakpoints for an address
   that points at slot 2 of a bundle where an L-X instruction is
   present, since there is logically no instruction at that address.
   However, to make things more interesting, the opcode of L-X
   instructions is located in slot 2.  This means that, to insert
   a breakpoint at an address that points to slot 1, we actually
   need to write the breakpoint in slot 2!  Slot 1 is actually
   the extended operand, so writing the breakpoint there would not
   have the desired effect.  Another side-effect of this issue
   is that we need to make sure that the shadow contents buffer
   does save byte 15 of our instruction bundle (this is the tail
   end of slot 2, which wouldn't be saved if we were to insert
   the breakpoint in slot 1).
   
   ia64 16-byte bundle layout:
   | 5 bits | slot 0 with 41 bits | slot 1 with 41 bits | slot 2 with 41 bits |
   
   The current addressing used by the code below:
   original PC   placed_address   placed_size             required    covered
				  == bp_tgt->shadow_len   reqd \subset covered
   0xABCDE0      0xABCDE0         0x10                    <0x0...0x5> <0x0..0xF>
   0xABCDE1      0xABCDE1         0xF                     <0x5...0xA> <0x1..0xF>
   0xABCDE2      0xABCDE2         0xE                     <0xA...0xF> <0x2..0xF>

   L-X instructions are treated a little specially, as explained above:
   0xABCDE1      0xABCDE1         0xF                     <0xA...0xF> <0x1..0xF>

   `objdump -d' and some other tools show a bit unjustified offsets:
   original PC   byte where starts the instruction   objdump offset
   0xABCDE0      0xABCDE0                            0xABCDE0
   0xABCDE1      0xABCDE5                            0xABCDE6
   0xABCDE2      0xABCDEA                            0xABCDEC
   */

#define IA64_BREAKPOINT 0x00003333300LL

static int
ia64_memory_insert_breakpoint (struct gdbarch *gdbarch,
			       struct bp_target_info *bp_tgt)
{
  CORE_ADDR addr = bp_tgt->placed_address = bp_tgt->reqstd_address;
  gdb_byte bundle[BUNDLE_LEN];
  int slotnum = (int) (addr & 0x0f) / SLOT_MULTIPLIER, shadow_slotnum;
  long long instr_breakpoint;
  int val;
  int templ;

  if (slotnum > 2)
    error (_("Can't insert breakpoint for slot numbers greater than 2."));

  addr &= ~0x0f;

  /* Enable the automatic memory restoration from breakpoints while
     we read our instruction bundle for the purpose of SHADOW_CONTENTS.
     Otherwise, we could possibly store into the shadow parts of the adjacent
     placed breakpoints.  It is due to our SHADOW_CONTENTS overlapping the real
     breakpoint instruction bits region.  */
  scoped_restore restore_memory_0
    = make_scoped_restore_show_memory_breakpoints (0);
  val = target_read_memory (addr, bundle, BUNDLE_LEN);
  if (val != 0)
    return val;

  /* SHADOW_SLOTNUM saves the original slot number as expected by the caller
     for addressing the SHADOW_CONTENTS placement.  */
  shadow_slotnum = slotnum;

  /* Always cover the last byte of the bundle in case we are inserting
     a breakpoint on an L-X instruction.  */
  bp_tgt->shadow_len = BUNDLE_LEN - shadow_slotnum;

  templ = extract_bit_field (bundle, 0, 5);
  if (template_encoding_table[templ][slotnum] == X)
    {
      /* X unit types can only be used in slot 2, and are actually
	 part of a 2-slot L-X instruction.  We cannot break at this
	 address, as this is the second half of an instruction that
	 lives in slot 1 of that bundle.  */
      gdb_assert (slotnum == 2);
      error (_("Can't insert breakpoint for non-existing slot X"));
    }
  if (template_encoding_table[templ][slotnum] == L)
    {
      /* L unit types can only be used in slot 1.  But the associated
	 opcode for that instruction is in slot 2, so bump the slot number
	 accordingly.  */
      gdb_assert (slotnum == 1);
      slotnum = 2;
    }

  /* Store the whole bundle, except for the initial skipped bytes by the slot
     number interpreted as bytes offset in PLACED_ADDRESS.  */
  memcpy (bp_tgt->shadow_contents, bundle + shadow_slotnum,
	  bp_tgt->shadow_len);

  /* Re-read the same bundle as above except that, this time, read it in order
     to compute the new bundle inside which we will be inserting the
     breakpoint.  Therefore, disable the automatic memory restoration from
     breakpoints while we read our instruction bundle.  Otherwise, the general
     restoration mechanism kicks in and we would possibly remove parts of the
     adjacent placed breakpoints.  It is due to our SHADOW_CONTENTS overlapping
     the real breakpoint instruction bits region.  */
  scoped_restore restore_memory_1
    = make_scoped_restore_show_memory_breakpoints (1);
  val = target_read_memory (addr, bundle, BUNDLE_LEN);
  if (val != 0)
    return val;

  /* Breakpoints already present in the code will get detected and not get
     reinserted by bp_loc_is_permanent.  Multiple breakpoints at the same
     location cannot induce the internal error as they are optimized into
     a single instance by update_global_location_list.  */
  instr_breakpoint = slotN_contents (bundle, slotnum);
  if (instr_breakpoint == IA64_BREAKPOINT)
    internal_error (_("Address %s already contains a breakpoint."),
		    paddress (gdbarch, bp_tgt->placed_address));
  replace_slotN_contents (bundle, IA64_BREAKPOINT, slotnum);

  val = target_write_memory (addr + shadow_slotnum, bundle + shadow_slotnum,
			     bp_tgt->shadow_len);

  return val;
}

static int
ia64_memory_remove_breakpoint (struct gdbarch *gdbarch,
			       struct bp_target_info *bp_tgt)
{
  CORE_ADDR addr = bp_tgt->placed_address;
  gdb_byte bundle_mem[BUNDLE_LEN], bundle_saved[BUNDLE_LEN];
  int slotnum = (addr & 0x0f) / SLOT_MULTIPLIER, shadow_slotnum;
  long long instr_breakpoint, instr_saved;
  int val;
  int templ;

  addr &= ~0x0f;

  /* Disable the automatic memory restoration from breakpoints while
     we read our instruction bundle.  Otherwise, the general restoration
     mechanism kicks in and we would possibly remove parts of the adjacent
     placed breakpoints.  It is due to our SHADOW_CONTENTS overlapping the real
     breakpoint instruction bits region.  */
  scoped_restore restore_memory_1
    = make_scoped_restore_show_memory_breakpoints (1);
  val = target_read_memory (addr, bundle_mem, BUNDLE_LEN);
  if (val != 0)
    return val;

  /* SHADOW_SLOTNUM saves the original slot number as expected by the caller
     for addressing the SHADOW_CONTENTS placement.  */
  shadow_slotnum = slotnum;

  templ = extract_bit_field (bundle_mem, 0, 5);
  if (template_encoding_table[templ][slotnum] == X)
    {
      /* X unit types can only be used in slot 2, and are actually
	 part of a 2-slot L-X instruction.  We refuse to insert
	 breakpoints at this address, so there should be no reason
	 for us attempting to remove one there, except if the program's
	 code somehow got modified in memory.  */
      gdb_assert (slotnum == 2);
      warning (_("Cannot remove breakpoint at address %s from non-existing "
		 "X-type slot, memory has changed underneath"),
	       paddress (gdbarch, bp_tgt->placed_address));
      return -1;
    }
  if (template_encoding_table[templ][slotnum] == L)
    {
      /* L unit types can only be used in slot 1.  But the breakpoint
	 was actually saved using slot 2, so update the slot number
	 accordingly.  */
      gdb_assert (slotnum == 1);
      slotnum = 2;
    }

  gdb_assert (bp_tgt->shadow_len == BUNDLE_LEN - shadow_slotnum);

  instr_breakpoint = slotN_contents (bundle_mem, slotnum);
  if (instr_breakpoint != IA64_BREAKPOINT)
    {
      warning (_("Cannot remove breakpoint at address %s, "
		 "no break instruction at such address."),
	       paddress (gdbarch, bp_tgt->placed_address));
      return -1;
    }

  /* Extract the original saved instruction from SLOTNUM normalizing its
     bit-shift for INSTR_SAVED.  */
  memcpy (bundle_saved, bundle_mem, BUNDLE_LEN);
  memcpy (bundle_saved + shadow_slotnum, bp_tgt->shadow_contents,
	  bp_tgt->shadow_len);
  instr_saved = slotN_contents (bundle_saved, slotnum);

  /* In BUNDLE_MEM, be careful to modify only the bits belonging to SLOTNUM
     and not any of the other ones that are stored in SHADOW_CONTENTS.  */
  replace_slotN_contents (bundle_mem, instr_saved, slotnum);
  val = target_write_raw_memory (addr, bundle_mem, BUNDLE_LEN);

  return val;
}

/* Implement the breakpoint_kind_from_pc gdbarch method.  */

static int
ia64_breakpoint_kind_from_pc (struct gdbarch *gdbarch, CORE_ADDR *pcptr)
{
  /* A place holder of gdbarch method breakpoint_kind_from_pc.   */
  return 0;
}

/* As gdbarch_breakpoint_from_pc ranges have byte granularity and ia64
   instruction slots ranges are bit-granular (41 bits) we have to provide an
   extended range as described for ia64_memory_insert_breakpoint.  We also take
   care of preserving the `break' instruction 21-bit (or 62-bit) parameter to
   make a match for permanent breakpoints.  */

static const gdb_byte *
ia64_breakpoint_from_pc (struct gdbarch *gdbarch,
			 CORE_ADDR *pcptr, int *lenptr)
{
  CORE_ADDR addr = *pcptr;
  static gdb_byte bundle[BUNDLE_LEN];
  int slotnum = (int) (*pcptr & 0x0f) / SLOT_MULTIPLIER, shadow_slotnum;
  long long instr_fetched;
  int val;
  int templ;

  if (slotnum > 2)
    error (_("Can't insert breakpoint for slot numbers greater than 2."));

  addr &= ~0x0f;

  /* Enable the automatic memory restoration from breakpoints while
     we read our instruction bundle to match bp_loc_is_permanent.  */
  {
    scoped_restore restore_memory_0
      = make_scoped_restore_show_memory_breakpoints (0);
    val = target_read_memory (addr, bundle, BUNDLE_LEN);
  }

  /* The memory might be unreachable.  This can happen, for instance,
     when the user inserts a breakpoint at an invalid address.  */
  if (val != 0)
    return NULL;

  /* SHADOW_SLOTNUM saves the original slot number as expected by the caller
     for addressing the SHADOW_CONTENTS placement.  */
  shadow_slotnum = slotnum;

  /* Cover always the last byte of the bundle for the L-X slot case.  */
  *lenptr = BUNDLE_LEN - shadow_slotnum;

  /* Check for L type instruction in slot 1, if present then bump up the slot
     number to the slot 2.  */
  templ = extract_bit_field (bundle, 0, 5);
  if (template_encoding_table[templ][slotnum] == X)
    {
      gdb_assert (slotnum == 2);
      error (_("Can't insert breakpoint for non-existing slot X"));
    }
  if (template_encoding_table[templ][slotnum] == L)
    {
      gdb_assert (slotnum == 1);
      slotnum = 2;
    }

  /* A break instruction has its all its opcode bits cleared except for
     the parameter value.  For L+X slot pair we are at the X slot (slot 2) so
     we should not touch the L slot - the upper 41 bits of the parameter.  */
  instr_fetched = slotN_contents (bundle, slotnum);
  instr_fetched &= 0x1003ffffc0LL;
  replace_slotN_contents (bundle, instr_fetched, slotnum);

  return bundle + shadow_slotnum;
}

static CORE_ADDR
ia64_read_pc (readable_regcache *regcache)
{
  ULONGEST psr_value, pc_value;
  int slot_num;

  regcache->cooked_read (IA64_PSR_REGNUM, &psr_value);
  regcache->cooked_read (IA64_IP_REGNUM, &pc_value);
  slot_num = (psr_value >> 41) & 3;

  return pc_value | (slot_num * SLOT_MULTIPLIER);
}

void
ia64_write_pc (struct regcache *regcache, CORE_ADDR new_pc)
{
  int slot_num = (int) (new_pc & 0xf) / SLOT_MULTIPLIER;
  ULONGEST psr_value;

  regcache_cooked_read_unsigned (regcache, IA64_PSR_REGNUM, &psr_value);
  psr_value &= ~(3LL << 41);
  psr_value |= (ULONGEST)(slot_num & 0x3) << 41;

  new_pc &= ~0xfLL;

  regcache_cooked_write_unsigned (regcache, IA64_PSR_REGNUM, psr_value);
  regcache_cooked_write_unsigned (regcache, IA64_IP_REGNUM, new_pc);
}

#define IS_NaT_COLLECTION_ADDR(addr) ((((addr) >> 3) & 0x3f) == 0x3f)

/* Returns the address of the slot that's NSLOTS slots away from
   the address ADDR.  NSLOTS may be positive or negative.  */
static CORE_ADDR
rse_address_add(CORE_ADDR addr, int nslots)
{
  CORE_ADDR new_addr;
  int mandatory_nat_slots = nslots / 63;
  int direction = nslots < 0 ? -1 : 1;

  new_addr = addr + 8 * (nslots + mandatory_nat_slots);

  if ((new_addr >> 9)  != ((addr + 8 * 64 * mandatory_nat_slots) >> 9))
    new_addr += 8 * direction;

  if (IS_NaT_COLLECTION_ADDR(new_addr))
    new_addr += 8 * direction;

  return new_addr;
}

static enum register_status
ia64_pseudo_register_read (struct gdbarch *gdbarch, readable_regcache *regcache,
			   int regnum, gdb_byte *buf)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  enum register_status status;

  if (regnum >= V32_REGNUM && regnum <= V127_REGNUM)
    {
#ifdef HAVE_LIBUNWIND_IA64_H
      /* First try and use the libunwind special reg accessor,
	 otherwise fallback to standard logic.  */
      if (!libunwind_is_initialized ()
	  || libunwind_get_reg_special (gdbarch, regcache, regnum, buf) != 0)
#endif
	{
	  /* The fallback position is to assume that r32-r127 are
	     found sequentially in memory starting at $bof.  This
	     isn't always true, but without libunwind, this is the
	     best we can do.  */
	  ULONGEST cfm;
	  ULONGEST bsp;
	  CORE_ADDR reg;

	  status = regcache->cooked_read (IA64_BSP_REGNUM, &bsp);
	  if (status != REG_VALID)
	    return status;

	  status = regcache->cooked_read (IA64_CFM_REGNUM, &cfm);
	  if (status != REG_VALID)
	    return status;

	  /* The bsp points at the end of the register frame so we
	     subtract the size of frame from it to get start of
	     register frame.  */
	  bsp = rse_address_add (bsp, -(cfm & 0x7f));
	  
	  if ((cfm & 0x7f) > regnum - V32_REGNUM) 
	    {
	      ULONGEST reg_addr = rse_address_add (bsp, (regnum - V32_REGNUM));
	      reg = read_memory_integer ((CORE_ADDR)reg_addr, 8, byte_order);
	      store_unsigned_integer (buf, register_size (gdbarch, regnum),
				      byte_order, reg);
	    }
	  else
	    store_unsigned_integer (buf, register_size (gdbarch, regnum),
				    byte_order, 0);
	}
    }
  else if (IA64_NAT0_REGNUM <= regnum && regnum <= IA64_NAT31_REGNUM)
    {
      ULONGEST unatN_val;
      ULONGEST unat;

      status = regcache->cooked_read (IA64_UNAT_REGNUM, &unat);
      if (status != REG_VALID)
	return status;
      unatN_val = (unat & (1LL << (regnum - IA64_NAT0_REGNUM))) != 0;
      store_unsigned_integer (buf, register_size (gdbarch, regnum),
			      byte_order, unatN_val);
    }
  else if (IA64_NAT32_REGNUM <= regnum && regnum <= IA64_NAT127_REGNUM)
    {
      ULONGEST natN_val = 0;
      ULONGEST bsp;
      ULONGEST cfm;
      CORE_ADDR gr_addr = 0;

      status = regcache->cooked_read (IA64_BSP_REGNUM, &bsp);
      if (status != REG_VALID)
	return status;

      status = regcache->cooked_read (IA64_CFM_REGNUM, &cfm);
      if (status != REG_VALID)
	return status;

      /* The bsp points at the end of the register frame so we
	 subtract the size of frame from it to get start of register frame.  */
      bsp = rse_address_add (bsp, -(cfm & 0x7f));
 
      if ((cfm & 0x7f) > regnum - V32_REGNUM) 
	gr_addr = rse_address_add (bsp, (regnum - V32_REGNUM));
      
      if (gr_addr != 0)
	{
	  /* Compute address of nat collection bits.  */
	  CORE_ADDR nat_addr = gr_addr | 0x1f8;
	  ULONGEST nat_collection;
	  int nat_bit;
	  /* If our nat collection address is bigger than bsp, we have to get
	     the nat collection from rnat.  Otherwise, we fetch the nat
	     collection from the computed address.  */
	  if (nat_addr >= bsp)
	    regcache->cooked_read (IA64_RNAT_REGNUM, &nat_collection);
	  else
	    nat_collection = read_memory_integer (nat_addr, 8, byte_order);
	  nat_bit = (gr_addr >> 3) & 0x3f;
	  natN_val = (nat_collection >> nat_bit) & 1;
	}
      
      store_unsigned_integer (buf, register_size (gdbarch, regnum),
			      byte_order, natN_val);
    }
  else if (regnum == VBOF_REGNUM)
    {
      /* A virtual register frame start is provided for user convenience.
	 It can be calculated as the bsp - sof (sizeof frame).  */
      ULONGEST bsp, vbsp;
      ULONGEST cfm;

      status = regcache->cooked_read (IA64_BSP_REGNUM, &bsp);
      if (status != REG_VALID)
	return status;
      status = regcache->cooked_read (IA64_CFM_REGNUM, &cfm);
      if (status != REG_VALID)
	return status;

      /* The bsp points at the end of the register frame so we
	 subtract the size of frame from it to get beginning of frame.  */
      vbsp = rse_address_add (bsp, -(cfm & 0x7f));
      store_unsigned_integer (buf, register_size (gdbarch, regnum),
			      byte_order, vbsp);
    }
  else if (VP0_REGNUM <= regnum && regnum <= VP63_REGNUM)
    {
      ULONGEST pr;
      ULONGEST cfm;
      ULONGEST prN_val;

      status = regcache->cooked_read (IA64_PR_REGNUM, &pr);
      if (status != REG_VALID)
	return status;
      status = regcache->cooked_read (IA64_CFM_REGNUM, &cfm);
      if (status != REG_VALID)
	return status;

      if (VP16_REGNUM <= regnum && regnum <= VP63_REGNUM)
	{
	  /* Fetch predicate register rename base from current frame
	     marker for this frame.  */
	  int rrb_pr = (cfm >> 32) & 0x3f;

	  /* Adjust the register number to account for register rotation.  */
	  regnum = VP16_REGNUM 
		 + ((regnum - VP16_REGNUM) + rrb_pr) % 48;
	}
      prN_val = (pr & (1LL << (regnum - VP0_REGNUM))) != 0;
      store_unsigned_integer (buf, register_size (gdbarch, regnum),
			      byte_order, prN_val);
    }
  else
    memset (buf, 0, register_size (gdbarch, regnum));

  return REG_VALID;
}

static void
ia64_pseudo_register_write (struct gdbarch *gdbarch, struct regcache *regcache,
			    int regnum, const gdb_byte *buf)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  if (regnum >= V32_REGNUM && regnum <= V127_REGNUM)
    {
      ULONGEST bsp;
      ULONGEST cfm;
      regcache_cooked_read_unsigned (regcache, IA64_BSP_REGNUM, &bsp);
      regcache_cooked_read_unsigned (regcache, IA64_CFM_REGNUM, &cfm);

      bsp = rse_address_add (bsp, -(cfm & 0x7f));
 
      if ((cfm & 0x7f) > regnum - V32_REGNUM) 
	{
	  ULONGEST reg_addr = rse_address_add (bsp, (regnum - V32_REGNUM));
	  write_memory (reg_addr, buf, 8);
	}
    }
  else if (IA64_NAT0_REGNUM <= regnum && regnum <= IA64_NAT31_REGNUM)
    {
      ULONGEST unatN_val, unat, unatN_mask;
      regcache_cooked_read_unsigned (regcache, IA64_UNAT_REGNUM, &unat);
      unatN_val = extract_unsigned_integer (buf, register_size (gdbarch,
								regnum),
					    byte_order);
      unatN_mask = (1LL << (regnum - IA64_NAT0_REGNUM));
      if (unatN_val == 0)
	unat &= ~unatN_mask;
      else if (unatN_val == 1)
	unat |= unatN_mask;
      regcache_cooked_write_unsigned (regcache, IA64_UNAT_REGNUM, unat);
    }
  else if (IA64_NAT32_REGNUM <= regnum && regnum <= IA64_NAT127_REGNUM)
    {
      ULONGEST natN_val;
      ULONGEST bsp;
      ULONGEST cfm;
      CORE_ADDR gr_addr = 0;
      regcache_cooked_read_unsigned (regcache, IA64_BSP_REGNUM, &bsp);
      regcache_cooked_read_unsigned (regcache, IA64_CFM_REGNUM, &cfm);

      /* The bsp points at the end of the register frame so we
	 subtract the size of frame from it to get start of register frame.  */
      bsp = rse_address_add (bsp, -(cfm & 0x7f));
 
      if ((cfm & 0x7f) > regnum - V32_REGNUM) 
	gr_addr = rse_address_add (bsp, (regnum - V32_REGNUM));
      
      natN_val = extract_unsigned_integer (buf, register_size (gdbarch,
							       regnum),
					   byte_order);

      if (gr_addr != 0 && (natN_val == 0 || natN_val == 1))
	{
	  /* Compute address of nat collection bits.  */
	  CORE_ADDR nat_addr = gr_addr | 0x1f8;
	  CORE_ADDR nat_collection;
	  int natN_bit = (gr_addr >> 3) & 0x3f;
	  ULONGEST natN_mask = (1LL << natN_bit);
	  /* If our nat collection address is bigger than bsp, we have to get
	     the nat collection from rnat.  Otherwise, we fetch the nat
	     collection from the computed address.  */
	  if (nat_addr >= bsp)
	    {
	      regcache_cooked_read_unsigned (regcache,
					     IA64_RNAT_REGNUM,
					     &nat_collection);
	      if (natN_val)
		nat_collection |= natN_mask;
	      else
		nat_collection &= ~natN_mask;
	      regcache_cooked_write_unsigned (regcache, IA64_RNAT_REGNUM,
					      nat_collection);
	    }
	  else
	    {
	      gdb_byte nat_buf[8];
	      nat_collection = read_memory_integer (nat_addr, 8, byte_order);
	      if (natN_val)
		nat_collection |= natN_mask;
	      else
		nat_collection &= ~natN_mask;
	      store_unsigned_integer (nat_buf, register_size (gdbarch, regnum),
				      byte_order, nat_collection);
	      write_memory (nat_addr, nat_buf, 8);
	    }
	}
    }
  else if (VP0_REGNUM <= regnum && regnum <= VP63_REGNUM)
    {
      ULONGEST pr;
      ULONGEST cfm;
      ULONGEST prN_val;
      ULONGEST prN_mask;

      regcache_cooked_read_unsigned (regcache, IA64_PR_REGNUM, &pr);
      regcache_cooked_read_unsigned (regcache, IA64_CFM_REGNUM, &cfm);

      if (VP16_REGNUM <= regnum && regnum <= VP63_REGNUM)
	{
	  /* Fetch predicate register rename base from current frame
	     marker for this frame.  */
	  int rrb_pr = (cfm >> 32) & 0x3f;

	  /* Adjust the register number to account for register rotation.  */
	  regnum = VP16_REGNUM 
		 + ((regnum - VP16_REGNUM) + rrb_pr) % 48;
	}
      prN_val = extract_unsigned_integer (buf, register_size (gdbarch, regnum),
					  byte_order);
      prN_mask = (1LL << (regnum - VP0_REGNUM));
      if (prN_val == 0)
	pr &= ~prN_mask;
      else if (prN_val == 1)
	pr |= prN_mask;
      regcache_cooked_write_unsigned (regcache, IA64_PR_REGNUM, pr);
    }
}

/* The ia64 needs to convert between various ieee floating-point formats
   and the special ia64 floating point register format.  */

static int
ia64_convert_register_p (struct gdbarch *gdbarch, int regno, struct type *type)
{
  return (regno >= IA64_FR0_REGNUM && regno <= IA64_FR127_REGNUM
	  && type->code () == TYPE_CODE_FLT
	  && type != ia64_ext_type (gdbarch));
}

static int
ia64_register_to_value (frame_info_ptr frame, int regnum,
			struct type *valtype, gdb_byte *out,
			int *optimizedp, int *unavailablep)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  gdb_byte in[IA64_FP_REGISTER_SIZE];

  /* Convert to TYPE.  */
  auto in_view = gdb::make_array_view (in, register_size (gdbarch, regnum));
  frame_info_ptr next_frame = get_next_frame_sentinel_okay (frame);
  if (!get_frame_register_bytes (next_frame, regnum, 0, in_view, optimizedp,
				 unavailablep))
    return 0;

  target_float_convert (in, ia64_ext_type (gdbarch), out, valtype);
  *optimizedp = *unavailablep = 0;
  return 1;
}

static void
ia64_value_to_register (frame_info_ptr frame, int regnum,
			 struct type *valtype, const gdb_byte *in)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  gdb_byte out[IA64_FP_REGISTER_SIZE];
  type *to_type = ia64_ext_type (gdbarch);
  target_float_convert (in, valtype, out, to_type);
  auto out_view = gdb::make_array_view (out, to_type->length ());
  put_frame_register (get_next_frame_sentinel_okay (frame), regnum, out_view);
}


/* Limit the number of skipped non-prologue instructions since examining
   of the prologue is expensive.  */
static int max_skip_non_prologue_insns = 40;

/* Given PC representing the starting address of a function, and
   LIM_PC which is the (sloppy) limit to which to scan when looking
   for a prologue, attempt to further refine this limit by using
   the line data in the symbol table.  If successful, a better guess
   on where the prologue ends is returned, otherwise the previous
   value of lim_pc is returned.  TRUST_LIMIT is a pointer to a flag
   which will be set to indicate whether the returned limit may be
   used with no further scanning in the event that the function is
   frameless.  */

/* FIXME: cagney/2004-02-14: This function and logic have largely been
   superseded by skip_prologue_using_sal.  */

static CORE_ADDR
refine_prologue_limit (CORE_ADDR pc, CORE_ADDR lim_pc, int *trust_limit)
{
  struct symtab_and_line prologue_sal;
  CORE_ADDR start_pc = pc;
  CORE_ADDR end_pc;

  /* The prologue can not possibly go past the function end itself,
     so we can already adjust LIM_PC accordingly.  */
  if (find_pc_partial_function (pc, NULL, NULL, &end_pc) && end_pc < lim_pc)
    lim_pc = end_pc;

  /* Start off not trusting the limit.  */
  *trust_limit = 0;

  prologue_sal = find_pc_line (pc, 0);
  if (prologue_sal.line != 0)
    {
      int i;
      CORE_ADDR addr = prologue_sal.end;

      /* Handle the case in which compiler's optimizer/scheduler
	 has moved instructions into the prologue.  We scan ahead
	 in the function looking for address ranges whose corresponding
	 line number is less than or equal to the first one that we
	 found for the function.  (It can be less than when the
	 scheduler puts a body instruction before the first prologue
	 instruction.)  */
      for (i = 2 * max_skip_non_prologue_insns; 
	   i > 0 && (lim_pc == 0 || addr < lim_pc);
	   i--)
	{
	  struct symtab_and_line sal;

	  sal = find_pc_line (addr, 0);
	  if (sal.line == 0)
	    break;
	  if (sal.line <= prologue_sal.line 
	      && sal.symtab == prologue_sal.symtab)
	    {
	      prologue_sal = sal;
	    }
	  addr = sal.end;
	}

      if (lim_pc == 0 || prologue_sal.end < lim_pc)
	{
	  lim_pc = prologue_sal.end;
	  if (start_pc == get_pc_function_start (lim_pc))
	    *trust_limit = 1;
	}
    }
  return lim_pc;
}

#define isScratch(_regnum_) ((_regnum_) == 2 || (_regnum_) == 3 \
  || (8 <= (_regnum_) && (_regnum_) <= 11) \
  || (14 <= (_regnum_) && (_regnum_) <= 31))
#define imm9(_instr_) \
  ( ((((_instr_) & 0x01000000000LL) ? -1 : 0) << 8) \
   | (((_instr_) & 0x00008000000LL) >> 20) \
   | (((_instr_) & 0x00000001fc0LL) >> 6))

/* Allocate and initialize a frame cache.  */

static struct ia64_frame_cache *
ia64_alloc_frame_cache (void)
{
  struct ia64_frame_cache *cache;
  int i;

  cache = FRAME_OBSTACK_ZALLOC (struct ia64_frame_cache);

  /* Base address.  */
  cache->base = 0;
  cache->pc = 0;
  cache->cfm = 0;
  cache->prev_cfm = 0;
  cache->sof = 0;
  cache->sol = 0;
  cache->sor = 0;
  cache->bsp = 0;
  cache->fp_reg = 0;
  cache->frameless = 1;

  for (i = 0; i < NUM_IA64_RAW_REGS; i++)
    cache->saved_regs[i] = 0;

  return cache;
}

static CORE_ADDR
examine_prologue (CORE_ADDR pc, CORE_ADDR lim_pc,
		  frame_info_ptr this_frame,
		  struct ia64_frame_cache *cache)
{
  CORE_ADDR next_pc;
  CORE_ADDR last_prologue_pc = pc;
  ia64_instruction_type it;
  long long instr;
  int cfm_reg  = 0;
  int ret_reg  = 0;
  int fp_reg   = 0;
  int unat_save_reg = 0;
  int pr_save_reg = 0;
  int mem_stack_frame_size = 0;
  int spill_reg   = 0;
  CORE_ADDR spill_addr = 0;
  char instores[8];
  char infpstores[8];
  char reg_contents[256];
  int trust_limit;
  int frameless = 1;
  int i;
  CORE_ADDR addr;
  gdb_byte buf[8];
  CORE_ADDR bof, sor, sol, sof, cfm, rrb_gr;

  memset (instores, 0, sizeof instores);
  memset (infpstores, 0, sizeof infpstores);
  memset (reg_contents, 0, sizeof reg_contents);

  if (cache->after_prologue != 0
      && cache->after_prologue <= lim_pc)
    return cache->after_prologue;

  lim_pc = refine_prologue_limit (pc, lim_pc, &trust_limit);
  next_pc = fetch_instruction (pc, &it, &instr);

  /* We want to check if we have a recognizable function start before we
     look ahead for a prologue.  */
  if (pc < lim_pc && next_pc 
      && it == M && ((instr & 0x1ee0000003fLL) == 0x02c00000000LL))
    {
      /* alloc - start of a regular function.  */
      int sol_bits = (int) ((instr & 0x00007f00000LL) >> 20);
      int sof_bits = (int) ((instr & 0x000000fe000LL) >> 13);
      int rN = (int) ((instr & 0x00000001fc0LL) >> 6);

      /* Verify that the current cfm matches what we think is the
	 function start.  If we have somehow jumped within a function,
	 we do not want to interpret the prologue and calculate the
	 addresses of various registers such as the return address.
	 We will instead treat the frame as frameless.  */
      if (!this_frame ||
	  (sof_bits == (cache->cfm & 0x7f) &&
	   sol_bits == ((cache->cfm >> 7) & 0x7f)))
	frameless = 0;

      cfm_reg = rN;
      last_prologue_pc = next_pc;
      pc = next_pc;
    }
  else
    {
      /* Look for a leaf routine.  */
      if (pc < lim_pc && next_pc
	  && (it == I || it == M) 
	  && ((instr & 0x1ee00000000LL) == 0x10800000000LL))
	{
	  /* adds rN = imm14, rM   (or mov rN, rM  when imm14 is 0) */
	  int imm = (int) ((((instr & 0x01000000000LL) ? -1 : 0) << 13) 
			   | ((instr & 0x001f8000000LL) >> 20)
			   | ((instr & 0x000000fe000LL) >> 13));
	  int rM = (int) ((instr & 0x00007f00000LL) >> 20);
	  int rN = (int) ((instr & 0x00000001fc0LL) >> 6);
	  int qp = (int) (instr & 0x0000000003fLL);
	  if (qp == 0 && rN == 2 && imm == 0 && rM == 12 && fp_reg == 0)
	    {
	      /* mov r2, r12 - beginning of leaf routine.  */
	      fp_reg = rN;
	      last_prologue_pc = next_pc;
	    }
	} 

      /* If we don't recognize a regular function or leaf routine, we are
	 done.  */
      if (!fp_reg)
	{
	  pc = lim_pc;	
	  if (trust_limit)
	    last_prologue_pc = lim_pc;
	}
    }

  /* Loop, looking for prologue instructions, keeping track of
     where preserved registers were spilled.  */
  while (pc < lim_pc)
    {
      next_pc = fetch_instruction (pc, &it, &instr);
      if (next_pc == 0)
	break;

      if (it == B && ((instr & 0x1e1f800003fLL) != 0x04000000000LL))
	{
	  /* Exit loop upon hitting a non-nop branch instruction.  */ 
	  if (trust_limit)
	    lim_pc = pc;
	  break;
	}
      else if (((instr & 0x3fLL) != 0LL) && 
	       (frameless || ret_reg != 0))
	{
	  /* Exit loop upon hitting a predicated instruction if
	     we already have the return register or if we are frameless.  */ 
	  if (trust_limit)
	    lim_pc = pc;
	  break;
	}
      else if (it == I && ((instr & 0x1eff8000000LL) == 0x00188000000LL))
	{
	  /* Move from BR */
	  int b2 = (int) ((instr & 0x0000000e000LL) >> 13);
	  int rN = (int) ((instr & 0x00000001fc0LL) >> 6);
	  int qp = (int) (instr & 0x0000000003f);

	  if (qp == 0 && b2 == 0 && rN >= 32 && ret_reg == 0)
	    {
	      ret_reg = rN;
	      last_prologue_pc = next_pc;
	    }
	}
      else if ((it == I || it == M) 
	  && ((instr & 0x1ee00000000LL) == 0x10800000000LL))
	{
	  /* adds rN = imm14, rM   (or mov rN, rM  when imm14 is 0) */
	  int imm = (int) ((((instr & 0x01000000000LL) ? -1 : 0) << 13) 
			   | ((instr & 0x001f8000000LL) >> 20)
			   | ((instr & 0x000000fe000LL) >> 13));
	  int rM = (int) ((instr & 0x00007f00000LL) >> 20);
	  int rN = (int) ((instr & 0x00000001fc0LL) >> 6);
	  int qp = (int) (instr & 0x0000000003fLL);

	  if (qp == 0 && rN >= 32 && imm == 0 && rM == 12 && fp_reg == 0)
	    {
	      /* mov rN, r12 */
	      fp_reg = rN;
	      last_prologue_pc = next_pc;
	    }
	  else if (qp == 0 && rN == 12 && rM == 12)
	    {
	      /* adds r12, -mem_stack_frame_size, r12 */
	      mem_stack_frame_size -= imm;
	      last_prologue_pc = next_pc;
	    }
	  else if (qp == 0 && rN == 2 
		&& ((rM == fp_reg && fp_reg != 0) || rM == 12))
	    {
	      CORE_ADDR saved_sp = 0;
	      /* adds r2, spilloffset, rFramePointer 
		   or
		 adds r2, spilloffset, r12

		 Get ready for stf.spill or st8.spill instructions.
		 The address to start spilling at is loaded into r2.
		 FIXME:  Why r2?  That's what gcc currently uses; it
		 could well be different for other compilers.  */

	      /* Hmm...  whether or not this will work will depend on
		 where the pc is.  If it's still early in the prologue
		 this'll be wrong.  FIXME */
	      if (this_frame)
		saved_sp = get_frame_register_unsigned (this_frame,
							sp_regnum);
	      spill_addr  = saved_sp
			  + (rM == 12 ? 0 : mem_stack_frame_size) 
			  + imm;
	      spill_reg   = rN;
	      last_prologue_pc = next_pc;
	    }
	  else if (qp == 0 && rM >= 32 && rM < 40 && !instores[rM-32] && 
		   rN < 256 && imm == 0)
	    {
	      /* mov rN, rM where rM is an input register.  */
	      reg_contents[rN] = rM;
	      last_prologue_pc = next_pc;
	    }
	  else if (frameless && qp == 0 && rN == fp_reg && imm == 0 && 
		   rM == 2)
	    {
	      /* mov r12, r2 */
	      last_prologue_pc = next_pc;
	      break;
	    }
	}
      else if (it == M 
	    && (   ((instr & 0x1efc0000000LL) == 0x0eec0000000LL)
		|| ((instr & 0x1ffc8000000LL) == 0x0cec0000000LL) ))
	{
	  /* stf.spill [rN] = fM, imm9
	     or
	     stf.spill [rN] = fM  */

	  int imm = imm9(instr);
	  int rN = (int) ((instr & 0x00007f00000LL) >> 20);
	  int fM = (int) ((instr & 0x000000fe000LL) >> 13);
	  int qp = (int) (instr & 0x0000000003fLL);
	  if (qp == 0 && rN == spill_reg && spill_addr != 0
	      && ((2 <= fM && fM <= 5) || (16 <= fM && fM <= 31)))
	    {
	      cache->saved_regs[IA64_FR0_REGNUM + fM] = spill_addr;

	      if ((instr & 0x1efc0000000LL) == 0x0eec0000000LL)
		spill_addr += imm;
	      else
		spill_addr = 0;		/* last one; must be done.  */
	      last_prologue_pc = next_pc;
	    }
	}
      else if ((it == M && ((instr & 0x1eff8000000LL) == 0x02110000000LL))
	    || (it == I && ((instr & 0x1eff8000000LL) == 0x00050000000LL)) )
	{
	  /* mov.m rN = arM   
	       or 
	     mov.i rN = arM */

	  int arM = (int) ((instr & 0x00007f00000LL) >> 20);
	  int rN  = (int) ((instr & 0x00000001fc0LL) >> 6);
	  int qp  = (int) (instr & 0x0000000003fLL);
	  if (qp == 0 && isScratch (rN) && arM == 36 /* ar.unat */)
	    {
	      /* We have something like "mov.m r3 = ar.unat".  Remember the
		 r3 (or whatever) and watch for a store of this register...  */
	      unat_save_reg = rN;
	      last_prologue_pc = next_pc;
	    }
	}
      else if (it == I && ((instr & 0x1eff8000000LL) == 0x00198000000LL))
	{
	  /* mov rN = pr */
	  int rN  = (int) ((instr & 0x00000001fc0LL) >> 6);
	  int qp  = (int) (instr & 0x0000000003fLL);
	  if (qp == 0 && isScratch (rN))
	    {
	      pr_save_reg = rN;
	      last_prologue_pc = next_pc;
	    }
	}
      else if (it == M 
	    && (   ((instr & 0x1ffc8000000LL) == 0x08cc0000000LL)
		|| ((instr & 0x1efc0000000LL) == 0x0acc0000000LL)))
	{
	  /* st8 [rN] = rM 
	      or
	     st8 [rN] = rM, imm9 */
	  int rN = (int) ((instr & 0x00007f00000LL) >> 20);
	  int rM = (int) ((instr & 0x000000fe000LL) >> 13);
	  int qp = (int) (instr & 0x0000000003fLL);
	  int indirect = rM < 256 ? reg_contents[rM] : 0;
	  if (qp == 0 && rN == spill_reg && spill_addr != 0
	      && (rM == unat_save_reg || rM == pr_save_reg))
	    {
	      /* We've found a spill of either the UNAT register or the PR
		 register.  (Well, not exactly; what we've actually found is
		 a spill of the register that UNAT or PR was moved to).
		 Record that fact and move on...  */
	      if (rM == unat_save_reg)
		{
		  /* Track UNAT register.  */
		  cache->saved_regs[IA64_UNAT_REGNUM] = spill_addr;
		  unat_save_reg = 0;
		}
	      else
		{
		  /* Track PR register.  */
		  cache->saved_regs[IA64_PR_REGNUM] = spill_addr;
		  pr_save_reg = 0;
		}
	      if ((instr & 0x1efc0000000LL) == 0x0acc0000000LL)
		/* st8 [rN] = rM, imm9 */
		spill_addr += imm9(instr);
	      else
		spill_addr = 0;		/* Must be done spilling.  */
	      last_prologue_pc = next_pc;
	    }
	  else if (qp == 0 && 32 <= rM && rM < 40 && !instores[rM-32])
	    {
	      /* Allow up to one store of each input register.  */
	      instores[rM-32] = 1;
	      last_prologue_pc = next_pc;
	    }
	  else if (qp == 0 && 32 <= indirect && indirect < 40 && 
		   !instores[indirect-32])
	    {
	      /* Allow an indirect store of an input register.  */
	      instores[indirect-32] = 1;
	      last_prologue_pc = next_pc;
	    }
	}
      else if (it == M && ((instr & 0x1ff08000000LL) == 0x08c00000000LL))
	{
	  /* One of
	       st1 [rN] = rM
	       st2 [rN] = rM
	       st4 [rN] = rM
	       st8 [rN] = rM
	     Note that the st8 case is handled in the clause above.
	     
	     Advance over stores of input registers.  One store per input
	     register is permitted.  */
	  int rM = (int) ((instr & 0x000000fe000LL) >> 13);
	  int qp = (int) (instr & 0x0000000003fLL);
	  int indirect = rM < 256 ? reg_contents[rM] : 0;
	  if (qp == 0 && 32 <= rM && rM < 40 && !instores[rM-32])
	    {
	      instores[rM-32] = 1;
	      last_prologue_pc = next_pc;
	    }
	  else if (qp == 0 && 32 <= indirect && indirect < 40 && 
		   !instores[indirect-32])
	    {
	      /* Allow an indirect store of an input register.  */
	      instores[indirect-32] = 1;
	      last_prologue_pc = next_pc;
	    }
	}
      else if (it == M && ((instr & 0x1ff88000000LL) == 0x0cc80000000LL))
	{
	  /* Either
	       stfs [rN] = fM
	     or
	       stfd [rN] = fM

	     Advance over stores of floating point input registers.  Again
	     one store per register is permitted.  */
	  int fM = (int) ((instr & 0x000000fe000LL) >> 13);
	  int qp = (int) (instr & 0x0000000003fLL);
	  if (qp == 0 && 8 <= fM && fM < 16 && !infpstores[fM - 8])
	    {
	      infpstores[fM-8] = 1;
	      last_prologue_pc = next_pc;
	    }
	}
      else if (it == M
	    && (   ((instr & 0x1ffc8000000LL) == 0x08ec0000000LL)
		|| ((instr & 0x1efc0000000LL) == 0x0aec0000000LL)))
	{
	  /* st8.spill [rN] = rM
	       or
	     st8.spill [rN] = rM, imm9 */
	  int rN = (int) ((instr & 0x00007f00000LL) >> 20);
	  int rM = (int) ((instr & 0x000000fe000LL) >> 13);
	  int qp = (int) (instr & 0x0000000003fLL);
	  if (qp == 0 && rN == spill_reg && 4 <= rM && rM <= 7)
	    {
	      /* We've found a spill of one of the preserved general purpose
		 regs.  Record the spill address and advance the spill
		 register if appropriate.  */
	      cache->saved_regs[IA64_GR0_REGNUM + rM] = spill_addr;
	      if ((instr & 0x1efc0000000LL) == 0x0aec0000000LL)
		/* st8.spill [rN] = rM, imm9 */
		spill_addr += imm9(instr);
	      else
		spill_addr = 0;		/* Done spilling.  */
	      last_prologue_pc = next_pc;
	    }
	}

      pc = next_pc;
    }

  /* If not frameless and we aren't called by skip_prologue, then we need
     to calculate registers for the previous frame which will be needed
     later.  */

  if (!frameless && this_frame)
    {
      struct gdbarch *gdbarch = get_frame_arch (this_frame);
      enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

      /* Extract the size of the rotating portion of the stack
	 frame and the register rename base from the current
	 frame marker.  */
      cfm = cache->cfm;
      sor = cache->sor;
      sof = cache->sof;
      sol = cache->sol;
      rrb_gr = (cfm >> 18) & 0x7f;

      /* Find the bof (beginning of frame).  */
      bof = rse_address_add (cache->bsp, -sof);
      
      for (i = 0, addr = bof;
	   i < sof;
	   i++, addr += 8)
	{
	  if (IS_NaT_COLLECTION_ADDR (addr))
	    {
	      addr += 8;
	    }
	  if (i+32 == cfm_reg)
	    cache->saved_regs[IA64_CFM_REGNUM] = addr;
	  if (i+32 == ret_reg)
	    cache->saved_regs[IA64_VRAP_REGNUM] = addr;
	  if (i+32 == fp_reg)
	    cache->saved_regs[IA64_VFP_REGNUM] = addr;
	}

      /* For the previous argument registers we require the previous bof.
	 If we can't find the previous cfm, then we can do nothing.  */
      cfm = 0;
      if (cache->saved_regs[IA64_CFM_REGNUM] != 0)
	{
	  cfm = read_memory_integer (cache->saved_regs[IA64_CFM_REGNUM],
				     8, byte_order);
	}
      else if (cfm_reg != 0)
	{
	  get_frame_register (this_frame, cfm_reg, buf);
	  cfm = extract_unsigned_integer (buf, 8, byte_order);
	}
      cache->prev_cfm = cfm;
      
      if (cfm != 0)
	{
	  sor = ((cfm >> 14) & 0xf) * 8;
	  sof = (cfm & 0x7f);
	  sol = (cfm >> 7) & 0x7f;
	  rrb_gr = (cfm >> 18) & 0x7f;

	  /* The previous bof only requires subtraction of the sol (size of
	     locals) due to the overlap between output and input of
	     subsequent frames.  */
	  bof = rse_address_add (bof, -sol);
	  
	  for (i = 0, addr = bof;
	       i < sof;
	       i++, addr += 8)
	    {
	      if (IS_NaT_COLLECTION_ADDR (addr))
		{
		  addr += 8;
		}
	      if (i < sor)
		cache->saved_regs[IA64_GR32_REGNUM
				  + ((i + (sor - rrb_gr)) % sor)] 
		  = addr;
	      else
		cache->saved_regs[IA64_GR32_REGNUM + i] = addr;
	    }
	  
	}
    }
      
  /* Try and trust the lim_pc value whenever possible.  */
  if (trust_limit && lim_pc >= last_prologue_pc)
    last_prologue_pc = lim_pc;

  cache->frameless = frameless;
  cache->after_prologue = last_prologue_pc;
  cache->mem_stack_frame_size = mem_stack_frame_size;
  cache->fp_reg = fp_reg;

  return last_prologue_pc;
}

CORE_ADDR
ia64_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  struct ia64_frame_cache cache;
  cache.base = 0;
  cache.after_prologue = 0;
  cache.cfm = 0;
  cache.bsp = 0;

  /* Call examine_prologue with - as third argument since we don't
     have a next frame pointer to send.  */
  return examine_prologue (pc, pc+1024, 0, &cache);
}


/* Normal frames.  */

static struct ia64_frame_cache *
ia64_frame_cache (frame_info_ptr this_frame, void **this_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  struct ia64_frame_cache *cache;
  gdb_byte buf[8];
  CORE_ADDR cfm;

  if (*this_cache)
    return (struct ia64_frame_cache *) *this_cache;

  cache = ia64_alloc_frame_cache ();
  *this_cache = cache;

  get_frame_register (this_frame, sp_regnum, buf);
  cache->saved_sp = extract_unsigned_integer (buf, 8, byte_order);

  /* We always want the bsp to point to the end of frame.
     This way, we can always get the beginning of frame (bof)
     by subtracting frame size.  */
  get_frame_register (this_frame, IA64_BSP_REGNUM, buf);
  cache->bsp = extract_unsigned_integer (buf, 8, byte_order);
  
  get_frame_register (this_frame, IA64_PSR_REGNUM, buf);

  get_frame_register (this_frame, IA64_CFM_REGNUM, buf);
  cfm = extract_unsigned_integer (buf, 8, byte_order);

  cache->sof = (cfm & 0x7f);
  cache->sol = (cfm >> 7) & 0x7f;
  cache->sor = ((cfm >> 14) & 0xf) * 8;

  cache->cfm = cfm;

  cache->pc = get_frame_func (this_frame);

  if (cache->pc != 0)
    examine_prologue (cache->pc, get_frame_pc (this_frame), this_frame, cache);
  
  cache->base = cache->saved_sp + cache->mem_stack_frame_size;

  return cache;
}

static void
ia64_frame_this_id (frame_info_ptr this_frame, void **this_cache,
		    struct frame_id *this_id)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct ia64_frame_cache *cache =
    ia64_frame_cache (this_frame, this_cache);

  /* If outermost frame, mark with null frame id.  */
  if (cache->base != 0)
    (*this_id) = frame_id_build_special (cache->base, cache->pc, cache->bsp);
  if (gdbarch_debug >= 1)
    gdb_printf (gdb_stdlog,
		"regular frame id: code %s, stack %s, "
		"special %s, this_frame %s\n",
		paddress (gdbarch, this_id->code_addr),
		paddress (gdbarch, this_id->stack_addr),
		paddress (gdbarch, cache->bsp),
		host_address_to_string (this_frame.get ()));
}

static struct value *
ia64_frame_prev_register (frame_info_ptr this_frame, void **this_cache,
			  int regnum)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  struct ia64_frame_cache *cache = ia64_frame_cache (this_frame, this_cache);
  gdb_byte buf[8];

  gdb_assert (regnum >= 0);

  if (!target_has_registers ())
    error (_("No registers."));

  if (regnum == gdbarch_sp_regnum (gdbarch))
    return frame_unwind_got_constant (this_frame, regnum, cache->base);

  else if (regnum == IA64_BSP_REGNUM)
    {
      struct value *val;
      CORE_ADDR prev_cfm, bsp, prev_bsp;

      /* We want to calculate the previous bsp as the end of the previous
	 register stack frame.  This corresponds to what the hardware bsp
	 register will be if we pop the frame back which is why we might
	 have been called.  We know the beginning of the current frame is
	 cache->bsp - cache->sof.  This value in the previous frame points
	 to the start of the output registers.  We can calculate the end of
	 that frame by adding the size of output:
	    (sof (size of frame) - sol (size of locals)).  */
      val = ia64_frame_prev_register (this_frame, this_cache, IA64_CFM_REGNUM);
      prev_cfm = extract_unsigned_integer (val->contents_all ().data (),
					   8, byte_order);
      bsp = rse_address_add (cache->bsp, -(cache->sof));
      prev_bsp =
	rse_address_add (bsp, (prev_cfm & 0x7f) - ((prev_cfm >> 7) & 0x7f));

      return frame_unwind_got_constant (this_frame, regnum, prev_bsp);
    }

  else if (regnum == IA64_CFM_REGNUM)
    {
      CORE_ADDR addr = cache->saved_regs[IA64_CFM_REGNUM];
      
      if (addr != 0)
	return frame_unwind_got_memory (this_frame, regnum, addr);

      if (cache->prev_cfm)
	return frame_unwind_got_constant (this_frame, regnum, cache->prev_cfm);

      if (cache->frameless)
	return frame_unwind_got_register (this_frame, IA64_PFS_REGNUM,
					  IA64_PFS_REGNUM);
      return frame_unwind_got_register (this_frame, regnum, 0);
    }

  else if (regnum == IA64_VFP_REGNUM)
    {
      /* If the function in question uses an automatic register (r32-r127)
	 for the frame pointer, it'll be found by ia64_find_saved_register()
	 above.  If the function lacks one of these frame pointers, we can
	 still provide a value since we know the size of the frame.  */
      return frame_unwind_got_constant (this_frame, regnum, cache->base);
    }

  else if (VP0_REGNUM <= regnum && regnum <= VP63_REGNUM)
    {
      struct value *pr_val;
      ULONGEST prN;
      
      pr_val = ia64_frame_prev_register (this_frame, this_cache,
					 IA64_PR_REGNUM);
      if (VP16_REGNUM <= regnum && regnum <= VP63_REGNUM)
	{
	  /* Fetch predicate register rename base from current frame
	     marker for this frame.  */
	  int rrb_pr = (cache->cfm >> 32) & 0x3f;

	  /* Adjust the register number to account for register rotation.  */
	  regnum = VP16_REGNUM + ((regnum - VP16_REGNUM) + rrb_pr) % 48;
	}
      prN = extract_bit_field (pr_val->contents_all ().data (),
			       regnum - VP0_REGNUM, 1);
      return frame_unwind_got_constant (this_frame, regnum, prN);
    }

  else if (IA64_NAT0_REGNUM <= regnum && regnum <= IA64_NAT31_REGNUM)
    {
      struct value *unat_val;
      ULONGEST unatN;
      unat_val = ia64_frame_prev_register (this_frame, this_cache,
					   IA64_UNAT_REGNUM);
      unatN = extract_bit_field (unat_val->contents_all ().data (),
				 regnum - IA64_NAT0_REGNUM, 1);
      return frame_unwind_got_constant (this_frame, regnum, unatN);
    }

  else if (IA64_NAT32_REGNUM <= regnum && regnum <= IA64_NAT127_REGNUM)
    {
      int natval = 0;
      /* Find address of general register corresponding to nat bit we're
	 interested in.  */
      CORE_ADDR gr_addr;

      gr_addr = cache->saved_regs[regnum - IA64_NAT0_REGNUM + IA64_GR0_REGNUM];

      if (gr_addr != 0)
	{
	  /* Compute address of nat collection bits.  */
	  CORE_ADDR nat_addr = gr_addr | 0x1f8;
	  CORE_ADDR bsp;
	  CORE_ADDR nat_collection;
	  int nat_bit;

	  /* If our nat collection address is bigger than bsp, we have to get
	     the nat collection from rnat.  Otherwise, we fetch the nat
	     collection from the computed address.  */
	  get_frame_register (this_frame, IA64_BSP_REGNUM, buf);
	  bsp = extract_unsigned_integer (buf, 8, byte_order);
	  if (nat_addr >= bsp)
	    {
	      get_frame_register (this_frame, IA64_RNAT_REGNUM, buf);
	      nat_collection = extract_unsigned_integer (buf, 8, byte_order);
	    }
	  else
	    nat_collection = read_memory_integer (nat_addr, 8, byte_order);
	  nat_bit = (gr_addr >> 3) & 0x3f;
	  natval = (nat_collection >> nat_bit) & 1;
	}

      return frame_unwind_got_constant (this_frame, regnum, natval);
    }

  else if (regnum == IA64_IP_REGNUM)
    {
      CORE_ADDR pc = 0;
      CORE_ADDR addr = cache->saved_regs[IA64_VRAP_REGNUM];

      if (addr != 0)
	{
	  read_memory (addr, buf, register_size (gdbarch, IA64_IP_REGNUM));
	  pc = extract_unsigned_integer (buf, 8, byte_order);
	}
      else if (cache->frameless)
	{
	  get_frame_register (this_frame, IA64_BR0_REGNUM, buf);
	  pc = extract_unsigned_integer (buf, 8, byte_order);
	}
      pc &= ~0xf;
      return frame_unwind_got_constant (this_frame, regnum, pc);
    }

  else if (regnum == IA64_PSR_REGNUM)
    {
      /* We don't know how to get the complete previous PSR, but we need it
	 for the slot information when we unwind the pc (pc is formed of IP
	 register plus slot information from PSR).  To get the previous
	 slot information, we mask it off the return address.  */
      ULONGEST slot_num = 0;
      CORE_ADDR pc = 0;
      CORE_ADDR psr = 0;
      CORE_ADDR addr = cache->saved_regs[IA64_VRAP_REGNUM];

      get_frame_register (this_frame, IA64_PSR_REGNUM, buf);
      psr = extract_unsigned_integer (buf, 8, byte_order);

      if (addr != 0)
	{
	  read_memory (addr, buf, register_size (gdbarch, IA64_IP_REGNUM));
	  pc = extract_unsigned_integer (buf, 8, byte_order);
	}
      else if (cache->frameless)
	{
	  get_frame_register (this_frame, IA64_BR0_REGNUM, buf);
	  pc = extract_unsigned_integer (buf, 8, byte_order);
	}
      psr &= ~(3LL << 41);
      slot_num = pc & 0x3LL;
      psr |= (CORE_ADDR)slot_num << 41;
      return frame_unwind_got_constant (this_frame, regnum, psr);
    }

  else if (regnum == IA64_BR0_REGNUM)
    {
      CORE_ADDR addr = cache->saved_regs[IA64_BR0_REGNUM];

      if (addr != 0)
	return frame_unwind_got_memory (this_frame, regnum, addr);

      return frame_unwind_got_constant (this_frame, regnum, 0);
    }

  else if ((regnum >= IA64_GR32_REGNUM && regnum <= IA64_GR127_REGNUM)
	   || (regnum >= V32_REGNUM && regnum <= V127_REGNUM))
    {
      CORE_ADDR addr = 0;

      if (regnum >= V32_REGNUM)
	regnum = IA64_GR32_REGNUM + (regnum - V32_REGNUM);
      addr = cache->saved_regs[regnum];
      if (addr != 0)
	return frame_unwind_got_memory (this_frame, regnum, addr);

      if (cache->frameless)
	{
	  struct value *reg_val;
	  CORE_ADDR prev_cfm, prev_bsp, prev_bof;

	  /* FIXME: brobecker/2008-05-01: Doesn't this seem redundant
	     with the same code above?  */
	  if (regnum >= V32_REGNUM)
	    regnum = IA64_GR32_REGNUM + (regnum - V32_REGNUM);
	  reg_val = ia64_frame_prev_register (this_frame, this_cache,
					      IA64_CFM_REGNUM);
	  prev_cfm = extract_unsigned_integer
	    (reg_val->contents_all ().data (), 8, byte_order);
	  reg_val = ia64_frame_prev_register (this_frame, this_cache,
					      IA64_BSP_REGNUM);
	  prev_bsp = extract_unsigned_integer
	    (reg_val->contents_all ().data (), 8, byte_order);
	  prev_bof = rse_address_add (prev_bsp, -(prev_cfm & 0x7f));

	  addr = rse_address_add (prev_bof, (regnum - IA64_GR32_REGNUM));
	  return frame_unwind_got_memory (this_frame, regnum, addr);
	}
      
      return frame_unwind_got_constant (this_frame, regnum, 0);
    }

  else /* All other registers.  */
    {
      CORE_ADDR addr = 0;

      if (IA64_FR32_REGNUM <= regnum && regnum <= IA64_FR127_REGNUM)
	{
	  /* Fetch floating point register rename base from current
	     frame marker for this frame.  */
	  int rrb_fr = (cache->cfm >> 25) & 0x7f;

	  /* Adjust the floating point register number to account for
	     register rotation.  */
	  regnum = IA64_FR32_REGNUM
		 + ((regnum - IA64_FR32_REGNUM) + rrb_fr) % 96;
	}

      /* If we have stored a memory address, access the register.  */
      addr = cache->saved_regs[regnum];
      if (addr != 0)
	return frame_unwind_got_memory (this_frame, regnum, addr);
      /* Otherwise, punt and get the current value of the register.  */
      else 
	return frame_unwind_got_register (this_frame, regnum, regnum);
    }
}
 
static const struct frame_unwind ia64_frame_unwind =
{
  "ia64 prologue",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  &ia64_frame_this_id,
  &ia64_frame_prev_register,
  NULL,
  default_frame_sniffer
};

/* Signal trampolines.  */

static void
ia64_sigtramp_frame_init_saved_regs (frame_info_ptr this_frame,
				     struct ia64_frame_cache *cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  ia64_gdbarch_tdep *tdep = gdbarch_tdep<ia64_gdbarch_tdep> (gdbarch);

  if (tdep->sigcontext_register_address)
    {
      int regno;

      cache->saved_regs[IA64_VRAP_REGNUM]
	= tdep->sigcontext_register_address (gdbarch, cache->base,
					     IA64_IP_REGNUM);
      cache->saved_regs[IA64_CFM_REGNUM]
	= tdep->sigcontext_register_address (gdbarch, cache->base,
					     IA64_CFM_REGNUM);
      cache->saved_regs[IA64_PSR_REGNUM]
	= tdep->sigcontext_register_address (gdbarch, cache->base,
					     IA64_PSR_REGNUM);
      cache->saved_regs[IA64_BSP_REGNUM]
	= tdep->sigcontext_register_address (gdbarch, cache->base,
					     IA64_BSP_REGNUM);
      cache->saved_regs[IA64_RNAT_REGNUM]
	= tdep->sigcontext_register_address (gdbarch, cache->base,
					     IA64_RNAT_REGNUM);
      cache->saved_regs[IA64_CCV_REGNUM]
	= tdep->sigcontext_register_address (gdbarch, cache->base,
					     IA64_CCV_REGNUM);
      cache->saved_regs[IA64_UNAT_REGNUM]
	= tdep->sigcontext_register_address (gdbarch, cache->base,
					     IA64_UNAT_REGNUM);
      cache->saved_regs[IA64_FPSR_REGNUM]
	= tdep->sigcontext_register_address (gdbarch, cache->base,
					     IA64_FPSR_REGNUM);
      cache->saved_regs[IA64_PFS_REGNUM]
	= tdep->sigcontext_register_address (gdbarch, cache->base,
					     IA64_PFS_REGNUM);
      cache->saved_regs[IA64_LC_REGNUM]
	= tdep->sigcontext_register_address (gdbarch, cache->base,
					     IA64_LC_REGNUM);

      for (regno = IA64_GR1_REGNUM; regno <= IA64_GR31_REGNUM; regno++)
	cache->saved_regs[regno] =
	  tdep->sigcontext_register_address (gdbarch, cache->base, regno);
      for (regno = IA64_BR0_REGNUM; regno <= IA64_BR7_REGNUM; regno++)
	cache->saved_regs[regno] =
	  tdep->sigcontext_register_address (gdbarch, cache->base, regno);
      for (regno = IA64_FR2_REGNUM; regno <= IA64_FR31_REGNUM; regno++)
	cache->saved_regs[regno] =
	  tdep->sigcontext_register_address (gdbarch, cache->base, regno);
    }
}

static struct ia64_frame_cache *
ia64_sigtramp_frame_cache (frame_info_ptr this_frame, void **this_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  struct ia64_frame_cache *cache;
  gdb_byte buf[8];

  if (*this_cache)
    return (struct ia64_frame_cache *) *this_cache;

  cache = ia64_alloc_frame_cache ();

  get_frame_register (this_frame, sp_regnum, buf);
  /* Note that frame size is hard-coded below.  We cannot calculate it
     via prologue examination.  */
  cache->base = extract_unsigned_integer (buf, 8, byte_order) + 16;

  get_frame_register (this_frame, IA64_BSP_REGNUM, buf);
  cache->bsp = extract_unsigned_integer (buf, 8, byte_order);

  get_frame_register (this_frame, IA64_CFM_REGNUM, buf);
  cache->cfm = extract_unsigned_integer (buf, 8, byte_order);
  cache->sof = cache->cfm & 0x7f;

  ia64_sigtramp_frame_init_saved_regs (this_frame, cache);

  *this_cache = cache;
  return cache;
}

static void
ia64_sigtramp_frame_this_id (frame_info_ptr this_frame,
			     void **this_cache, struct frame_id *this_id)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct ia64_frame_cache *cache =
    ia64_sigtramp_frame_cache (this_frame, this_cache);

  (*this_id) = frame_id_build_special (cache->base,
				       get_frame_pc (this_frame),
				       cache->bsp);
  if (gdbarch_debug >= 1)
    gdb_printf (gdb_stdlog,
		"sigtramp frame id: code %s, stack %s, "
		"special %s, this_frame %s\n",
		paddress (gdbarch, this_id->code_addr),
		paddress (gdbarch, this_id->stack_addr),
		paddress (gdbarch, cache->bsp),
		host_address_to_string (this_frame.get ()));
}

static struct value *
ia64_sigtramp_frame_prev_register (frame_info_ptr this_frame,
				   void **this_cache, int regnum)
{
  struct ia64_frame_cache *cache =
    ia64_sigtramp_frame_cache (this_frame, this_cache);

  gdb_assert (regnum >= 0);

  if (!target_has_registers ())
    error (_("No registers."));

  if (regnum == IA64_IP_REGNUM)
    {
      CORE_ADDR pc = 0;
      CORE_ADDR addr = cache->saved_regs[IA64_VRAP_REGNUM];

      if (addr != 0)
	{
	  struct gdbarch *gdbarch = get_frame_arch (this_frame);
	  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
	  pc = read_memory_unsigned_integer (addr, 8, byte_order);
	}
      pc &= ~0xf;
      return frame_unwind_got_constant (this_frame, regnum, pc);
    }

  else if ((regnum >= IA64_GR32_REGNUM && regnum <= IA64_GR127_REGNUM)
	   || (regnum >= V32_REGNUM && regnum <= V127_REGNUM))
    {
      CORE_ADDR addr = 0;

      if (regnum >= V32_REGNUM)
	regnum = IA64_GR32_REGNUM + (regnum - V32_REGNUM);
      addr = cache->saved_regs[regnum];
      if (addr != 0)
	return frame_unwind_got_memory (this_frame, regnum, addr);

      return frame_unwind_got_constant (this_frame, regnum, 0);
    }

  else  /* All other registers not listed above.  */
    {
      CORE_ADDR addr = cache->saved_regs[regnum];

      if (addr != 0)
	return frame_unwind_got_memory (this_frame, regnum, addr);

      return frame_unwind_got_constant (this_frame, regnum, 0);
    }
}

static int
ia64_sigtramp_frame_sniffer (const struct frame_unwind *self,
			     frame_info_ptr this_frame,
			     void **this_cache)
{
  gdbarch *arch = get_frame_arch (this_frame);
  ia64_gdbarch_tdep *tdep = gdbarch_tdep<ia64_gdbarch_tdep> (arch);
  if (tdep->pc_in_sigtramp)
    {
      CORE_ADDR pc = get_frame_pc (this_frame);

      if (tdep->pc_in_sigtramp (pc))
	return 1;
    }

  return 0;
}

static const struct frame_unwind ia64_sigtramp_frame_unwind =
{
  "ia64 sigtramp",
  SIGTRAMP_FRAME,
  default_frame_unwind_stop_reason,
  ia64_sigtramp_frame_this_id,
  ia64_sigtramp_frame_prev_register,
  NULL,
  ia64_sigtramp_frame_sniffer
};



static CORE_ADDR
ia64_frame_base_address (frame_info_ptr this_frame, void **this_cache)
{
  struct ia64_frame_cache *cache = ia64_frame_cache (this_frame, this_cache);

  return cache->base;
}

static const struct frame_base ia64_frame_base =
{
  &ia64_frame_unwind,
  ia64_frame_base_address,
  ia64_frame_base_address,
  ia64_frame_base_address
};

#ifdef HAVE_LIBUNWIND_IA64_H

struct ia64_unwind_table_entry
  {
    unw_word_t start_offset;
    unw_word_t end_offset;
    unw_word_t info_offset;
  };

static __inline__ uint64_t
ia64_rse_slot_num (uint64_t addr)
{
  return (addr >> 3) & 0x3f;
}

/* Skip over a designated number of registers in the backing
   store, remembering every 64th position is for NAT.  */
static __inline__ uint64_t
ia64_rse_skip_regs (uint64_t addr, long num_regs)
{
  long delta = ia64_rse_slot_num(addr) + num_regs;

  if (num_regs < 0)
    delta -= 0x3e;
  return addr + ((num_regs + delta/0x3f) << 3);
}
  
/* Gdb ia64-libunwind-tdep callback function to convert from an ia64 gdb
   register number to a libunwind register number.  */
static int
ia64_gdb2uw_regnum (int regnum)
{
  if (regnum == sp_regnum)
    return UNW_IA64_SP;
  else if (regnum == IA64_BSP_REGNUM)
    return UNW_IA64_BSP;
  else if ((unsigned) (regnum - IA64_GR0_REGNUM) < 128)
    return UNW_IA64_GR + (regnum - IA64_GR0_REGNUM);
  else if ((unsigned) (regnum - V32_REGNUM) < 95)
    return UNW_IA64_GR + 32 + (regnum - V32_REGNUM);
  else if ((unsigned) (regnum - IA64_FR0_REGNUM) < 128)
    return UNW_IA64_FR + (regnum - IA64_FR0_REGNUM);
  else if ((unsigned) (regnum - IA64_PR0_REGNUM) < 64)
    return -1;
  else if ((unsigned) (regnum - IA64_BR0_REGNUM) < 8)
    return UNW_IA64_BR + (regnum - IA64_BR0_REGNUM);
  else if (regnum == IA64_PR_REGNUM)
    return UNW_IA64_PR;
  else if (regnum == IA64_IP_REGNUM)
    return UNW_REG_IP;
  else if (regnum == IA64_CFM_REGNUM)
    return UNW_IA64_CFM;
  else if ((unsigned) (regnum - IA64_AR0_REGNUM) < 128)
    return UNW_IA64_AR + (regnum - IA64_AR0_REGNUM);
  else if ((unsigned) (regnum - IA64_NAT0_REGNUM) < 128)
    return UNW_IA64_NAT + (regnum - IA64_NAT0_REGNUM);
  else
    return -1;
}
  
/* Gdb ia64-libunwind-tdep callback function to convert from a libunwind
   register number to a ia64 gdb register number.  */
static int
ia64_uw2gdb_regnum (int uw_regnum)
{
  if (uw_regnum == UNW_IA64_SP)
    return sp_regnum;
  else if (uw_regnum == UNW_IA64_BSP)
    return IA64_BSP_REGNUM;
  else if ((unsigned) (uw_regnum - UNW_IA64_GR) < 32)
    return IA64_GR0_REGNUM + (uw_regnum - UNW_IA64_GR);
  else if ((unsigned) (uw_regnum - UNW_IA64_GR) < 128)
    return V32_REGNUM + (uw_regnum - (IA64_GR0_REGNUM + 32));
  else if ((unsigned) (uw_regnum - UNW_IA64_FR) < 128)
    return IA64_FR0_REGNUM + (uw_regnum - UNW_IA64_FR);
  else if ((unsigned) (uw_regnum - UNW_IA64_BR) < 8)
    return IA64_BR0_REGNUM + (uw_regnum - UNW_IA64_BR);
  else if (uw_regnum == UNW_IA64_PR)
    return IA64_PR_REGNUM;
  else if (uw_regnum == UNW_REG_IP)
    return IA64_IP_REGNUM;
  else if (uw_regnum == UNW_IA64_CFM)
    return IA64_CFM_REGNUM;
  else if ((unsigned) (uw_regnum - UNW_IA64_AR) < 128)
    return IA64_AR0_REGNUM + (uw_regnum - UNW_IA64_AR);
  else if ((unsigned) (uw_regnum - UNW_IA64_NAT) < 128)
    return IA64_NAT0_REGNUM + (uw_regnum - UNW_IA64_NAT);
  else
    return -1;
}

/* Gdb ia64-libunwind-tdep callback function to reveal if register is
   a float register or not.  */
static int
ia64_is_fpreg (int uw_regnum)
{
  return unw_is_fpreg (uw_regnum);
}

/* Libunwind callback accessor function for general registers.  */
static int
ia64_access_reg (unw_addr_space_t as, unw_regnum_t uw_regnum, unw_word_t *val, 
		 int write, void *arg)
{
  int regnum = ia64_uw2gdb_regnum (uw_regnum);
  unw_word_t bsp, sof, cfm, psr, ip;
  struct frame_info *this_frame = (frame_info *) arg;
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  ia64_gdbarch_tdep *tdep = gdbarch_tdep<ia64_gdbarch_tdep> (gdbarch);
  
  /* We never call any libunwind routines that need to write registers.  */
  gdb_assert (!write);

  switch (uw_regnum)
    {
      case UNW_REG_IP:
	/* Libunwind expects to see the pc value which means the slot number
	   from the psr must be merged with the ip word address.  */
	ip = get_frame_register_unsigned (this_frame, IA64_IP_REGNUM);
	psr = get_frame_register_unsigned (this_frame, IA64_PSR_REGNUM);
	*val = ip | ((psr >> 41) & 0x3);
	break;
 
      case UNW_IA64_AR_BSP:
	/* Libunwind expects to see the beginning of the current
	   register frame so we must account for the fact that
	   ptrace() will return a value for bsp that points *after*
	   the current register frame.  */
	bsp = get_frame_register_unsigned (this_frame, IA64_BSP_REGNUM);
	cfm = get_frame_register_unsigned (this_frame, IA64_CFM_REGNUM);
	sof = tdep->size_of_register_frame (this_frame, cfm);
	*val = ia64_rse_skip_regs (bsp, -sof);
	break;

      case UNW_IA64_AR_BSPSTORE:
	/* Libunwind wants bspstore to be after the current register frame.
	   This is what ptrace() and gdb treats as the regular bsp value.  */
	*val = get_frame_register_unsigned (this_frame, IA64_BSP_REGNUM);
	break;

      default:
	/* For all other registers, just unwind the value directly.  */
	*val = get_frame_register_unsigned (this_frame, regnum);
	break;
    }
      
  if (gdbarch_debug >= 1)
    gdb_printf (gdb_stdlog, 
		"  access_reg: from cache: %4s=%s\n",
		(((unsigned) regnum <= IA64_NAT127_REGNUM)
		 ? ia64_register_names[regnum] : "r??"), 
		paddress (gdbarch, *val));
  return 0;
}

/* Libunwind callback accessor function for floating-point registers.  */
static int
ia64_access_fpreg (unw_addr_space_t as, unw_regnum_t uw_regnum,
		   unw_fpreg_t *val, int write, void *arg)
{
  int regnum = ia64_uw2gdb_regnum (uw_regnum);
  frame_info_ptr this_frame = (frame_info_ptr) arg;
  
  /* We never call any libunwind routines that need to write registers.  */
  gdb_assert (!write);

  get_frame_register (this_frame, regnum, (gdb_byte *) val);

  return 0;
}

/* Libunwind callback accessor function for top-level rse registers.  */
static int
ia64_access_rse_reg (unw_addr_space_t as, unw_regnum_t uw_regnum,
		     unw_word_t *val, int write, void *arg)
{
  int regnum = ia64_uw2gdb_regnum (uw_regnum);
  unw_word_t bsp, sof, cfm, psr, ip;
  struct regcache *regcache = (struct regcache *) arg;
  struct gdbarch *gdbarch = regcache->arch ();
  
  /* We never call any libunwind routines that need to write registers.  */
  gdb_assert (!write);

  switch (uw_regnum)
    {
      case UNW_REG_IP:
	/* Libunwind expects to see the pc value which means the slot number
	   from the psr must be merged with the ip word address.  */
	regcache_cooked_read_unsigned (regcache, IA64_IP_REGNUM, &ip);
	regcache_cooked_read_unsigned (regcache, IA64_PSR_REGNUM, &psr);
	*val = ip | ((psr >> 41) & 0x3);
	break;
	  
      case UNW_IA64_AR_BSP:
	/* Libunwind expects to see the beginning of the current
	   register frame so we must account for the fact that
	   ptrace() will return a value for bsp that points *after*
	   the current register frame.  */
	regcache_cooked_read_unsigned (regcache, IA64_BSP_REGNUM, &bsp);
	regcache_cooked_read_unsigned (regcache, IA64_CFM_REGNUM, &cfm);
	sof = (cfm & 0x7f);
	*val = ia64_rse_skip_regs (bsp, -sof);
	break;
	  
      case UNW_IA64_AR_BSPSTORE:
	/* Libunwind wants bspstore to be after the current register frame.
	   This is what ptrace() and gdb treats as the regular bsp value.  */
	regcache_cooked_read_unsigned (regcache, IA64_BSP_REGNUM, val);
	break;

      default:
	/* For all other registers, just unwind the value directly.  */
	regcache_cooked_read_unsigned (regcache, regnum, val);
	break;
    }
      
  if (gdbarch_debug >= 1)
    gdb_printf (gdb_stdlog, 
		"  access_rse_reg: from cache: %4s=%s\n",
		(((unsigned) regnum <= IA64_NAT127_REGNUM)
		 ? ia64_register_names[regnum] : "r??"), 
		paddress (gdbarch, *val));

  return 0;
}

/* Libunwind callback accessor function for top-level fp registers.  */
static int
ia64_access_rse_fpreg (unw_addr_space_t as, unw_regnum_t uw_regnum,
		       unw_fpreg_t *val, int write, void *arg)
{
  int regnum = ia64_uw2gdb_regnum (uw_regnum);
  struct regcache *regcache = (struct regcache *) arg;
  
  /* We never call any libunwind routines that need to write registers.  */
  gdb_assert (!write);

  regcache->cooked_read (regnum, (gdb_byte *) val);

  return 0;
}

/* Libunwind callback accessor function for accessing memory.  */
static int
ia64_access_mem (unw_addr_space_t as,
		 unw_word_t addr, unw_word_t *val,
		 int write, void *arg)
{
  if (addr - KERNEL_START < ktab_size)
    {
      unw_word_t *laddr = (unw_word_t*) ((char *) ktab
			  + (addr - KERNEL_START));
		
      if (write)
	*laddr = *val; 
      else 
	*val = *laddr;
      return 0;
    }

  /* XXX do we need to normalize byte-order here?  */
  if (write)
    return target_write_memory (addr, (gdb_byte *) val, sizeof (unw_word_t));
  else
    return target_read_memory (addr, (gdb_byte *) val, sizeof (unw_word_t));
}

/* Call low-level function to access the kernel unwind table.  */
static std::optional<gdb::byte_vector>
getunwind_table ()
{
  /* FIXME drow/2005-09-10: This code used to call
     ia64_linux_xfer_unwind_table directly to fetch the unwind table
     for the currently running ia64-linux kernel.  That data should
     come from the core file and be accessed via the auxv vector; if
     we want to preserve fall back to the running kernel's table, then
     we should find a way to override the corefile layer's
     xfer_partial method.  */

  return target_read_alloc (current_inferior ()->top_target (),
			    TARGET_OBJECT_UNWIND_TABLE, NULL);
}

/* Get the kernel unwind table.  */				 
static int
get_kernel_table (unw_word_t ip, unw_dyn_info_t *di)
{
  static struct ia64_table_entry *etab;

  if (!ktab) 
    {
      ktab_buf = getunwind_table ();
      if (!ktab_buf)
	return -UNW_ENOINFO;

      ktab = (struct ia64_table_entry *) ktab_buf->data ();
      ktab_size = ktab_buf->size ();

      for (etab = ktab; etab->start_offset; ++etab)
	etab->info_offset += KERNEL_START;
    }
  
  if (ip < ktab[0].start_offset || ip >= etab[-1].end_offset)
    return -UNW_ENOINFO;
  
  di->format = UNW_INFO_FORMAT_TABLE;
  di->gp = 0;
  di->start_ip = ktab[0].start_offset;
  di->end_ip = etab[-1].end_offset;
  di->u.ti.name_ptr = (unw_word_t) "<kernel>";
  di->u.ti.segbase = 0;
  di->u.ti.table_len = ((char *) etab - (char *) ktab) / sizeof (unw_word_t);
  di->u.ti.table_data = (unw_word_t *) ktab;
  
  if (gdbarch_debug >= 1)
    gdb_printf (gdb_stdlog, "get_kernel_table: found table `%s': "
		"segbase=%s, length=%s, gp=%s\n",
		(char *) di->u.ti.name_ptr, 
		hex_string (di->u.ti.segbase),
		pulongest (di->u.ti.table_len), 
		hex_string (di->gp));
  return 0;
}

/* Find the unwind table entry for a specified address.  */
static int
ia64_find_unwind_table (struct objfile *objfile, unw_word_t ip,
			unw_dyn_info_t *dip, void **buf)
{
  Elf_Internal_Phdr *phdr, *p_text = NULL, *p_unwind = NULL;
  Elf_Internal_Ehdr *ehdr;
  unw_word_t segbase = 0;
  CORE_ADDR load_base;
  bfd *bfd;
  int i;

  bfd = objfile->obfd;
  
  ehdr = elf_tdata (bfd)->elf_header;
  phdr = elf_tdata (bfd)->phdr;

  load_base = objfile->text_section_offset ();

  for (i = 0; i < ehdr->e_phnum; ++i)
    {
      switch (phdr[i].p_type)
	{
	case PT_LOAD:
	  if ((unw_word_t) (ip - load_base - phdr[i].p_vaddr)
	      < phdr[i].p_memsz)
	    p_text = phdr + i;
	  break;

	case PT_IA_64_UNWIND:
	  p_unwind = phdr + i;
	  break;

	default:
	  break;
	}
    }

  if (!p_text || !p_unwind)
    return -UNW_ENOINFO;

  /* Verify that the segment that contains the IP also contains
     the static unwind table.  If not, we may be in the Linux kernel's
     DSO gate page in which case the unwind table is another segment.
     Otherwise, we are dealing with runtime-generated code, for which we 
     have no info here.  */
  segbase = p_text->p_vaddr + load_base;

  if ((p_unwind->p_vaddr - p_text->p_vaddr) >= p_text->p_memsz)
    {
      int ok = 0;
      for (i = 0; i < ehdr->e_phnum; ++i)
	{
	  if (phdr[i].p_type == PT_LOAD
	      && (p_unwind->p_vaddr - phdr[i].p_vaddr) < phdr[i].p_memsz)
	    {
	      ok = 1;
	      /* Get the segbase from the section containing the
		 libunwind table.  */
	      segbase = phdr[i].p_vaddr + load_base;
	    }
	}
      if (!ok)
	return -UNW_ENOINFO;
    }

  dip->start_ip = p_text->p_vaddr + load_base;
  dip->end_ip = dip->start_ip + p_text->p_memsz;
  dip->gp = ia64_find_global_pointer (objfile->arch (), ip);
  dip->format = UNW_INFO_FORMAT_REMOTE_TABLE;
  dip->u.rti.name_ptr = (unw_word_t) bfd_get_filename (bfd);
  dip->u.rti.segbase = segbase;
  dip->u.rti.table_len = p_unwind->p_memsz / sizeof (unw_word_t);
  dip->u.rti.table_data = p_unwind->p_vaddr + load_base;

  return 0;
}

/* Libunwind callback accessor function to acquire procedure unwind-info.  */
static int
ia64_find_proc_info_x (unw_addr_space_t as, unw_word_t ip, unw_proc_info_t *pi,
		       int need_unwind_info, void *arg)
{
  struct obj_section *sec = find_pc_section (ip);
  unw_dyn_info_t di;
  int ret;
  void *buf = NULL;

  if (!sec)
    {
      /* XXX This only works if the host and the target architecture are
	 both ia64 and if the have (more or less) the same kernel
	 version.  */
      if (get_kernel_table (ip, &di) < 0)
	return -UNW_ENOINFO;

      if (gdbarch_debug >= 1)
	gdb_printf (gdb_stdlog, "ia64_find_proc_info_x: %s -> "
		    "(name=`%s',segbase=%s,start=%s,end=%s,gp=%s,"
		    "length=%s,data=%s)\n",
		    hex_string (ip), (char *)di.u.ti.name_ptr,
		    hex_string (di.u.ti.segbase),
		    hex_string (di.start_ip), hex_string (di.end_ip),
		    hex_string (di.gp),
		    pulongest (di.u.ti.table_len), 
		    hex_string ((CORE_ADDR)di.u.ti.table_data));
    }
  else
    {
      ret = ia64_find_unwind_table (sec->objfile, ip, &di, &buf);
      if (ret < 0)
	return ret;

      if (gdbarch_debug >= 1)
	gdb_printf (gdb_stdlog, "ia64_find_proc_info_x: %s -> "
		    "(name=`%s',segbase=%s,start=%s,end=%s,gp=%s,"
		    "length=%s,data=%s)\n",
		    hex_string (ip), (char *)di.u.rti.name_ptr,
		    hex_string (di.u.rti.segbase),
		    hex_string (di.start_ip), hex_string (di.end_ip),
		    hex_string (di.gp),
		    pulongest (di.u.rti.table_len), 
		    hex_string (di.u.rti.table_data));
    }

  ret = libunwind_search_unwind_table (&as, ip, &di, pi, need_unwind_info,
				       arg);

  /* We no longer need the dyn info storage so free it.  */
  xfree (buf);

  return ret;
}

/* Libunwind callback accessor function for cleanup.  */
static void
ia64_put_unwind_info (unw_addr_space_t as,
		      unw_proc_info_t *pip, void *arg)
{
  /* Nothing required for now.  */
}

/* Libunwind callback accessor function to get head of the dynamic 
   unwind-info registration list.  */ 
static int
ia64_get_dyn_info_list (unw_addr_space_t as,
			unw_word_t *dilap, void *arg)
{
  struct obj_section *text_sec;
  unw_word_t ip, addr;
  unw_dyn_info_t di;
  int ret;

  if (!libunwind_is_initialized ())
    return -UNW_ENOINFO;

  for (objfile *objfile : current_program_space->objfiles ())
    {
      void *buf = NULL;

      text_sec = objfile->sections + SECT_OFF_TEXT (objfile);
      ip = text_sec->addr ();
      ret = ia64_find_unwind_table (objfile, ip, &di, &buf);
      if (ret >= 0)
	{
	  addr = libunwind_find_dyn_list (as, &di, arg);
	  /* We no longer need the dyn info storage so free it.  */
	  xfree (buf);

	  if (addr)
	    {
	      if (gdbarch_debug >= 1)
		gdb_printf (gdb_stdlog,
			    "dynamic unwind table in objfile %s "
			    "at %s (gp=%s)\n",
			    bfd_get_filename (objfile->obfd),
			    hex_string (addr), hex_string (di.gp));
	      *dilap = addr;
	      return 0;
	    }
	}
    }
  return -UNW_ENOINFO;
}


/* Frame interface functions for libunwind.  */

static void
ia64_libunwind_frame_this_id (frame_info_ptr this_frame, void **this_cache,
			      struct frame_id *this_id)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  struct frame_id id = outer_frame_id;
  gdb_byte buf[8];
  CORE_ADDR bsp;

  libunwind_frame_this_id (this_frame, this_cache, &id);
  if (id == outer_frame_id)
    {
      (*this_id) = outer_frame_id;
      return;
    }

  /* We must add the bsp as the special address for frame comparison 
     purposes.  */
  get_frame_register (this_frame, IA64_BSP_REGNUM, buf);
  bsp = extract_unsigned_integer (buf, 8, byte_order);

  (*this_id) = frame_id_build_special (id.stack_addr, id.code_addr, bsp);

  if (gdbarch_debug >= 1)
    gdb_printf (gdb_stdlog,
		"libunwind frame id: code %s, stack %s, "
		"special %s, this_frame %s\n",
		paddress (gdbarch, id.code_addr),
		paddress (gdbarch, id.stack_addr),
		paddress (gdbarch, bsp),
		host_address_to_string (this_frame));
}

static struct value *
ia64_libunwind_frame_prev_register (frame_info_ptr this_frame,
				    void **this_cache, int regnum)
{
  int reg = regnum;
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  struct value *val;

  if (VP0_REGNUM <= regnum && regnum <= VP63_REGNUM)
    reg = IA64_PR_REGNUM;
  else if (IA64_NAT0_REGNUM <= regnum && regnum <= IA64_NAT127_REGNUM)
    reg = IA64_UNAT_REGNUM;

  /* Let libunwind do most of the work.  */
  val = libunwind_frame_prev_register (this_frame, this_cache, reg);

  if (VP0_REGNUM <= regnum && regnum <= VP63_REGNUM)
    {
      ULONGEST prN_val;

      if (VP16_REGNUM <= regnum && regnum <= VP63_REGNUM)
	{
	  int rrb_pr = 0;
	  ULONGEST cfm;

	  /* Fetch predicate register rename base from current frame
	     marker for this frame.  */
	  cfm = get_frame_register_unsigned (this_frame, IA64_CFM_REGNUM);
	  rrb_pr = (cfm >> 32) & 0x3f;
	  
	  /* Adjust the register number to account for register rotation.  */
	  regnum = VP16_REGNUM + ((regnum - VP16_REGNUM) + rrb_pr) % 48;
	}
      prN_val = extract_bit_field (val->contents_all ().data (),
				   regnum - VP0_REGNUM, 1);
      return frame_unwind_got_constant (this_frame, regnum, prN_val);
    }

  else if (IA64_NAT0_REGNUM <= regnum && regnum <= IA64_NAT127_REGNUM)
    {
      ULONGEST unatN_val;

      unatN_val = extract_bit_field (val->contents_all ().data (),
				     regnum - IA64_NAT0_REGNUM, 1);
      return frame_unwind_got_constant (this_frame, regnum, unatN_val);
    }

  else if (regnum == IA64_BSP_REGNUM)
    {
      struct value *cfm_val;
      CORE_ADDR prev_bsp, prev_cfm;

      /* We want to calculate the previous bsp as the end of the previous
	 register stack frame.  This corresponds to what the hardware bsp
	 register will be if we pop the frame back which is why we might
	 have been called.  We know that libunwind will pass us back the
	 beginning of the current frame so we should just add sof to it.  */
      prev_bsp = extract_unsigned_integer (val->contents_all ().data (),
					   8, byte_order);
      cfm_val = libunwind_frame_prev_register (this_frame, this_cache,
					       IA64_CFM_REGNUM);
      prev_cfm = extract_unsigned_integer (cfm_val->contents_all ().data (),
					   8, byte_order);
      prev_bsp = rse_address_add (prev_bsp, (prev_cfm & 0x7f));

      return frame_unwind_got_constant (this_frame, regnum, prev_bsp);
    }
  else
    return val;
}

static int
ia64_libunwind_frame_sniffer (const struct frame_unwind *self,
			      frame_info_ptr this_frame,
			      void **this_cache)
{
  if (libunwind_is_initialized ()
      && libunwind_frame_sniffer (self, this_frame, this_cache))
    return 1;

  return 0;
}

static const struct frame_unwind ia64_libunwind_frame_unwind =
{
  "ia64 libunwind",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  ia64_libunwind_frame_this_id,
  ia64_libunwind_frame_prev_register,
  NULL,
  ia64_libunwind_frame_sniffer,
  libunwind_frame_dealloc_cache
};

static void
ia64_libunwind_sigtramp_frame_this_id (frame_info_ptr this_frame,
				       void **this_cache,
				       struct frame_id *this_id)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte buf[8];
  CORE_ADDR bsp;
  struct frame_id id = outer_frame_id;

  libunwind_frame_this_id (this_frame, this_cache, &id);
  if (id == outer_frame_id)
    {
      (*this_id) = outer_frame_id;
      return;
    }

  /* We must add the bsp as the special address for frame comparison 
     purposes.  */
  get_frame_register (this_frame, IA64_BSP_REGNUM, buf);
  bsp = extract_unsigned_integer (buf, 8, byte_order);

  /* For a sigtramp frame, we don't make the check for previous ip being 0.  */
  (*this_id) = frame_id_build_special (id.stack_addr, id.code_addr, bsp);

  if (gdbarch_debug >= 1)
    gdb_printf (gdb_stdlog,
		"libunwind sigtramp frame id: code %s, "
		"stack %s, special %s, this_frame %s\n",
		paddress (gdbarch, id.code_addr),
		paddress (gdbarch, id.stack_addr),
		paddress (gdbarch, bsp),
		host_address_to_string (this_frame));
}

static struct value *
ia64_libunwind_sigtramp_frame_prev_register (frame_info_ptr this_frame,
					     void **this_cache, int regnum)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  struct value *prev_ip_val;
  CORE_ADDR prev_ip;

  /* If the previous frame pc value is 0, then we want to use the SIGCONTEXT
     method of getting previous registers.  */
  prev_ip_val = libunwind_frame_prev_register (this_frame, this_cache,
					       IA64_IP_REGNUM);
  prev_ip = extract_unsigned_integer (prev_ip_val->contents_all ().data (),
				      8, byte_order);

  if (prev_ip == 0)
    {
      void *tmp_cache = NULL;
      return ia64_sigtramp_frame_prev_register (this_frame, &tmp_cache,
						regnum);
    }
  else
    return ia64_libunwind_frame_prev_register (this_frame, this_cache, regnum);
}

static int
ia64_libunwind_sigtramp_frame_sniffer (const struct frame_unwind *self,
				       frame_info_ptr this_frame,
				       void **this_cache)
{
  if (libunwind_is_initialized ())
    {
      if (libunwind_sigtramp_frame_sniffer (self, this_frame, this_cache))
	return 1;
      return 0;
    }
  else
    return ia64_sigtramp_frame_sniffer (self, this_frame, this_cache);
}

static const struct frame_unwind ia64_libunwind_sigtramp_frame_unwind =
{
  "ia64 libunwind sigtramp",
  SIGTRAMP_FRAME,
  default_frame_unwind_stop_reason,
  ia64_libunwind_sigtramp_frame_this_id,
  ia64_libunwind_sigtramp_frame_prev_register,
  NULL,
  ia64_libunwind_sigtramp_frame_sniffer
};

/* Set of libunwind callback acccessor functions.  */
unw_accessors_t ia64_unw_accessors =
{
  ia64_find_proc_info_x,
  ia64_put_unwind_info,
  ia64_get_dyn_info_list,
  ia64_access_mem,
  ia64_access_reg,
  ia64_access_fpreg,
  /* resume */
  /* get_proc_name */
};

/* Set of special libunwind callback acccessor functions specific for accessing
   the rse registers.  At the top of the stack, we want libunwind to figure out
   how to read r32 - r127.  Though usually they are found sequentially in
   memory starting from $bof, this is not always true.  */
unw_accessors_t ia64_unw_rse_accessors =
{
  ia64_find_proc_info_x,
  ia64_put_unwind_info,
  ia64_get_dyn_info_list,
  ia64_access_mem,
  ia64_access_rse_reg,
  ia64_access_rse_fpreg,
  /* resume */
  /* get_proc_name */
};

/* Set of ia64-libunwind-tdep gdb callbacks and data for generic
   ia64-libunwind-tdep code to use.  */
struct libunwind_descr ia64_libunwind_descr =
{
  ia64_gdb2uw_regnum, 
  ia64_uw2gdb_regnum, 
  ia64_is_fpreg, 
  &ia64_unw_accessors,
  &ia64_unw_rse_accessors,
};

#endif /* HAVE_LIBUNWIND_IA64_H  */

static int
ia64_use_struct_convention (struct type *type)
{
  struct type *float_elt_type;

  /* Don't use the struct convention for anything but structure,
     union, or array types.  */
  if (!(type->code () == TYPE_CODE_STRUCT
	|| type->code () == TYPE_CODE_UNION
	|| type->code () == TYPE_CODE_ARRAY))
    return 0;

  /* HFAs are structures (or arrays) consisting entirely of floating
     point values of the same length.  Up to 8 of these are returned
     in registers.  Don't use the struct convention when this is the
     case.  */
  float_elt_type = is_float_or_hfa_type (type);
  if (float_elt_type != NULL
      && type->length () / float_elt_type->length () <= 8)
    return 0;

  /* Other structs of length 32 or less are returned in r8-r11.
     Don't use the struct convention for those either.  */
  return type->length () > 32;
}

/* Return non-zero if TYPE is a structure or union type.  */

static int
ia64_struct_type_p (const struct type *type)
{
  return (type->code () == TYPE_CODE_STRUCT
	  || type->code () == TYPE_CODE_UNION);
}

static void
ia64_extract_return_value (struct type *type, struct regcache *regcache,
			   gdb_byte *valbuf)
{
  struct gdbarch *gdbarch = regcache->arch ();
  struct type *float_elt_type;

  float_elt_type = is_float_or_hfa_type (type);
  if (float_elt_type != NULL)
    {
      gdb_byte from[IA64_FP_REGISTER_SIZE];
      int offset = 0;
      int regnum = IA64_FR8_REGNUM;
      int n = type->length () / float_elt_type->length ();

      while (n-- > 0)
	{
	  regcache->cooked_read (regnum, from);
	  target_float_convert (from, ia64_ext_type (gdbarch),
				valbuf + offset, float_elt_type);
	  offset += float_elt_type->length ();
	  regnum++;
	}
    }
  else if (!ia64_struct_type_p (type) && type->length () < 8)
    {
      /* This is an integral value, and its size is less than 8 bytes.
	 These values are LSB-aligned, so extract the relevant bytes,
	 and copy them into VALBUF.  */
      /* brobecker/2005-12-30: Actually, all integral values are LSB aligned,
	 so I suppose we should also add handling here for integral values
	 whose size is greater than 8.  But I wasn't able to create such
	 a type, neither in C nor in Ada, so not worrying about these yet.  */
      enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
      ULONGEST val;

      regcache_cooked_read_unsigned (regcache, IA64_GR8_REGNUM, &val);
      store_unsigned_integer (valbuf, type->length (), byte_order, val);
    }
  else
    {
      ULONGEST val;
      int offset = 0;
      int regnum = IA64_GR8_REGNUM;
      int reglen = register_type (gdbarch, IA64_GR8_REGNUM)->length ();
      int n = type->length () / reglen;
      int m = type->length () % reglen;

      while (n-- > 0)
	{
	  ULONGEST regval;
	  regcache_cooked_read_unsigned (regcache, regnum, &regval);
	  memcpy ((char *)valbuf + offset, &regval, reglen);
	  offset += reglen;
	  regnum++;
	}

      if (m)
	{
	  regcache_cooked_read_unsigned (regcache, regnum, &val);
	  memcpy ((char *)valbuf + offset, &val, m);
	}
    }
}

static void
ia64_store_return_value (struct type *type, struct regcache *regcache, 
			 const gdb_byte *valbuf)
{
  struct gdbarch *gdbarch = regcache->arch ();
  struct type *float_elt_type;

  float_elt_type = is_float_or_hfa_type (type);
  if (float_elt_type != NULL)
    {
      gdb_byte to[IA64_FP_REGISTER_SIZE];
      int offset = 0;
      int regnum = IA64_FR8_REGNUM;
      int n = type->length () / float_elt_type->length ();

      while (n-- > 0)
	{
	  target_float_convert (valbuf + offset, float_elt_type,
				to, ia64_ext_type (gdbarch));
	  regcache->cooked_write (regnum, to);
	  offset += float_elt_type->length ();
	  regnum++;
	}
    }
  else
    {
      int offset = 0;
      int regnum = IA64_GR8_REGNUM;
      int reglen = register_type (gdbarch, IA64_GR8_REGNUM)->length ();
      int n = type->length () / reglen;
      int m = type->length () % reglen;

      while (n-- > 0)
	{
	  ULONGEST val;
	  memcpy (&val, (char *)valbuf + offset, reglen);
	  regcache_cooked_write_unsigned (regcache, regnum, val);
	  offset += reglen;
	  regnum++;
	}

      if (m)
	{
	  ULONGEST val;
	  memcpy (&val, (char *)valbuf + offset, m);
	  regcache_cooked_write_unsigned (regcache, regnum, val);
	}
    }
}
  
static enum return_value_convention
ia64_return_value (struct gdbarch *gdbarch, struct value *function,
		   struct type *valtype, struct regcache *regcache,
		   gdb_byte *readbuf, const gdb_byte *writebuf)
{
  int struct_return = ia64_use_struct_convention (valtype);

  if (writebuf != NULL)
    {
      gdb_assert (!struct_return);
      ia64_store_return_value (valtype, regcache, writebuf);
    }

  if (readbuf != NULL)
    {
      gdb_assert (!struct_return);
      ia64_extract_return_value (valtype, regcache, readbuf);
    }

  if (struct_return)
    return RETURN_VALUE_STRUCT_CONVENTION;
  else
    return RETURN_VALUE_REGISTER_CONVENTION;
}

static int
is_float_or_hfa_type_recurse (struct type *t, struct type **etp)
{
  switch (t->code ())
    {
    case TYPE_CODE_FLT:
      if (*etp)
	return (*etp)->length () == t->length ();
      else
	{
	  *etp = t;
	  return 1;
	}
      break;
    case TYPE_CODE_ARRAY:
      return
	is_float_or_hfa_type_recurse (check_typedef (t->target_type ()),
				      etp);
      break;
    case TYPE_CODE_STRUCT:
      {
	int i;

	for (i = 0; i < t->num_fields (); i++)
	  if (!is_float_or_hfa_type_recurse
	      (check_typedef (t->field (i).type ()), etp))
	    return 0;
	return 1;
      }
      break;
    default:
      break;
    }

  return 0;
}

/* Determine if the given type is one of the floating point types or
   and HFA (which is a struct, array, or combination thereof whose
   bottom-most elements are all of the same floating point type).  */

static struct type *
is_float_or_hfa_type (struct type *t)
{
  struct type *et = 0;

  return is_float_or_hfa_type_recurse (t, &et) ? et : 0;
}


/* Return 1 if the alignment of T is such that the next even slot
   should be used.  Return 0, if the next available slot should
   be used.  (See section 8.5.1 of the IA-64 Software Conventions
   and Runtime manual).  */

static int
slot_alignment_is_next_even (struct type *t)
{
  switch (t->code ())
    {
    case TYPE_CODE_INT:
    case TYPE_CODE_FLT:
      if (t->length () > 8)
	return 1;
      else
	return 0;
    case TYPE_CODE_ARRAY:
      return
	slot_alignment_is_next_even (check_typedef (t->target_type ()));
    case TYPE_CODE_STRUCT:
      {
	int i;

	for (i = 0; i < t->num_fields (); i++)
	  if (slot_alignment_is_next_even
	      (check_typedef (t->field (i).type ())))
	    return 1;
	return 0;
      }
    default:
      return 0;
    }
}

/* Attempt to find (and return) the global pointer for the given
   function.

   This is a rather nasty bit of code searchs for the .dynamic section
   in the objfile corresponding to the pc of the function we're trying
   to call.  Once it finds the addresses at which the .dynamic section
   lives in the child process, it scans the Elf64_Dyn entries for a
   DT_PLTGOT tag.  If it finds one of these, the corresponding
   d_un.d_ptr value is the global pointer.  */

static CORE_ADDR
ia64_find_global_pointer_from_dynamic_section (struct gdbarch *gdbarch,
					       CORE_ADDR faddr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  struct obj_section *faddr_sect;
     
  faddr_sect = find_pc_section (faddr);
  if (faddr_sect != NULL)
    {
      for (obj_section *osect : faddr_sect->objfile->sections ())
	{
	  if (strcmp (osect->the_bfd_section->name, ".dynamic") == 0)
	    {
	      CORE_ADDR addr = osect->addr ();
	      CORE_ADDR endaddr = osect->endaddr ();

	      while (addr < endaddr)
		{
		  int status;
		  LONGEST tag;
		  gdb_byte buf[8];

		  status = target_read_memory (addr, buf, sizeof (buf));
		  if (status != 0)
		    break;
		  tag = extract_signed_integer (buf, byte_order);

		  if (tag == DT_PLTGOT)
		    {
		      CORE_ADDR global_pointer;

		      status = target_read_memory (addr + 8, buf,
						   sizeof (buf));
		      if (status != 0)
			break;
		      global_pointer
			= extract_unsigned_integer (buf, sizeof (buf),
						    byte_order);

		      /* The payoff...  */
		      return global_pointer;
		    }

		  if (tag == DT_NULL)
		    break;

		  addr += 16;
		}

	      break;
	    }
	}
    }
  return 0;
}

/* Attempt to find (and return) the global pointer for the given
   function.  We first try the find_global_pointer_from_solib routine
   from the gdbarch tdep vector, if provided.  And if that does not
   work, then we try ia64_find_global_pointer_from_dynamic_section.  */

static CORE_ADDR
ia64_find_global_pointer (struct gdbarch *gdbarch, CORE_ADDR faddr)
{
  ia64_gdbarch_tdep *tdep = gdbarch_tdep<ia64_gdbarch_tdep> (gdbarch);
  CORE_ADDR addr = 0;

  if (tdep->find_global_pointer_from_solib)
    addr = tdep->find_global_pointer_from_solib (gdbarch, faddr);
  if (addr == 0)
    addr = ia64_find_global_pointer_from_dynamic_section (gdbarch, faddr);
  return addr;
}

/* Given a function's address, attempt to find (and return) the
   corresponding (canonical) function descriptor.  Return 0 if
   not found.  */
static CORE_ADDR
find_extant_func_descr (struct gdbarch *gdbarch, CORE_ADDR faddr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  struct obj_section *faddr_sect;

  /* Return early if faddr is already a function descriptor.  */
  faddr_sect = find_pc_section (faddr);
  if (faddr_sect && strcmp (faddr_sect->the_bfd_section->name, ".opd") == 0)
    return faddr;

  if (faddr_sect != NULL)
    {
      for (obj_section *osect : faddr_sect->objfile->sections ())
	{
	  if (strcmp (osect->the_bfd_section->name, ".opd") == 0)
	    {
	      CORE_ADDR addr = osect->addr ();
	      CORE_ADDR endaddr = osect->endaddr ();

	      while (addr < endaddr)
		{
		  int status;
		  LONGEST faddr2;
		  gdb_byte buf[8];

		  status = target_read_memory (addr, buf, sizeof (buf));
		  if (status != 0)
		    break;
		  faddr2 = extract_signed_integer (buf, byte_order);

		  if (faddr == faddr2)
		    return addr;

		  addr += 16;
		}

	      break;
	    }
	}
    }
  return 0;
}

/* Attempt to find a function descriptor corresponding to the
   given address.  If none is found, construct one on the
   stack using the address at fdaptr.  */

static CORE_ADDR
find_func_descr (struct regcache *regcache, CORE_ADDR faddr, CORE_ADDR *fdaptr)
{
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR fdesc;

  fdesc = find_extant_func_descr (gdbarch, faddr);

  if (fdesc == 0)
    {
      ULONGEST global_pointer;
      gdb_byte buf[16];

      fdesc = *fdaptr;
      *fdaptr += 16;

      global_pointer = ia64_find_global_pointer (gdbarch, faddr);

      if (global_pointer == 0)
	regcache_cooked_read_unsigned (regcache,
				       IA64_GR1_REGNUM, &global_pointer);

      store_unsigned_integer (buf, 8, byte_order, faddr);
      store_unsigned_integer (buf + 8, 8, byte_order, global_pointer);

      write_memory (fdesc, buf, 16);
    }

  return fdesc; 
}

/* Use the following routine when printing out function pointers
   so the user can see the function address rather than just the
   function descriptor.  */
static CORE_ADDR
ia64_convert_from_func_ptr_addr (struct gdbarch *gdbarch, CORE_ADDR addr,
				 struct target_ops *targ)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  struct obj_section *s;
  gdb_byte buf[8];

  s = find_pc_section (addr);

  /* check if ADDR points to a function descriptor.  */
  if (s && strcmp (s->the_bfd_section->name, ".opd") == 0)
    return read_memory_unsigned_integer (addr, 8, byte_order);

  /* Normally, functions live inside a section that is executable.
     So, if ADDR points to a non-executable section, then treat it
     as a function descriptor and return the target address iff
     the target address itself points to a section that is executable.
     Check first the memory of the whole length of 8 bytes is readable.  */
  if (s && (s->the_bfd_section->flags & SEC_CODE) == 0
      && target_read_memory (addr, buf, 8) == 0)
    {
      CORE_ADDR pc = extract_unsigned_integer (buf, 8, byte_order);
      struct obj_section *pc_section = find_pc_section (pc);

      if (pc_section && (pc_section->the_bfd_section->flags & SEC_CODE))
	return pc;
    }

  /* There are also descriptors embedded in vtables.  */
  if (s)
    {
      struct bound_minimal_symbol minsym;

      minsym = lookup_minimal_symbol_by_pc (addr);

      if (minsym.minsym
	  && is_vtable_name (minsym.minsym->linkage_name ()))
	return read_memory_unsigned_integer (addr, 8, byte_order);
    }

  return addr;
}

static CORE_ADDR
ia64_frame_align (struct gdbarch *gdbarch, CORE_ADDR sp)
{
  return sp & ~0xfLL;
}

/* The default "allocate_new_rse_frame" ia64_infcall_ops routine for ia64.  */

static void
ia64_allocate_new_rse_frame (struct regcache *regcache, ULONGEST bsp, int sof)
{
  ULONGEST cfm, pfs, new_bsp;

  regcache_cooked_read_unsigned (regcache, IA64_CFM_REGNUM, &cfm);

  new_bsp = rse_address_add (bsp, sof);
  regcache_cooked_write_unsigned (regcache, IA64_BSP_REGNUM, new_bsp);

  regcache_cooked_read_unsigned (regcache, IA64_PFS_REGNUM, &pfs);
  pfs &= 0xc000000000000000LL;
  pfs |= (cfm & 0xffffffffffffLL);
  regcache_cooked_write_unsigned (regcache, IA64_PFS_REGNUM, pfs);

  cfm &= 0xc000000000000000LL;
  cfm |= sof;
  regcache_cooked_write_unsigned (regcache, IA64_CFM_REGNUM, cfm);
}

/* The default "store_argument_in_slot" ia64_infcall_ops routine for
   ia64.  */

static void
ia64_store_argument_in_slot (struct regcache *regcache, CORE_ADDR bsp,
			     int slotnum, gdb_byte *buf)
{
  write_memory (rse_address_add (bsp, slotnum), buf, 8);
}

/* The default "set_function_addr" ia64_infcall_ops routine for ia64.  */

static void
ia64_set_function_addr (struct regcache *regcache, CORE_ADDR func_addr)
{
  /* Nothing needed.  */
}

static CORE_ADDR
ia64_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
		      struct regcache *regcache, CORE_ADDR bp_addr,
		      int nargs, struct value **args, CORE_ADDR sp,
		      function_call_return_method return_method,
		      CORE_ADDR struct_addr)
{
  ia64_gdbarch_tdep *tdep = gdbarch_tdep<ia64_gdbarch_tdep> (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int argno;
  struct value *arg;
  struct type *type;
  int len, argoffset;
  int nslots, rseslots, memslots, slotnum, nfuncargs;
  int floatreg;
  ULONGEST bsp;
  CORE_ADDR funcdescaddr, global_pointer;
  CORE_ADDR func_addr = find_function_addr (function, NULL);

  nslots = 0;
  nfuncargs = 0;
  /* Count the number of slots needed for the arguments.  */
  for (argno = 0; argno < nargs; argno++)
    {
      arg = args[argno];
      type = check_typedef (arg->type ());
      len = type->length ();

      if ((nslots & 1) && slot_alignment_is_next_even (type))
	nslots++;

      if (type->code () == TYPE_CODE_FUNC)
	nfuncargs++;

      nslots += (len + 7) / 8;
    }

  /* Divvy up the slots between the RSE and the memory stack.  */
  rseslots = (nslots > 8) ? 8 : nslots;
  memslots = nslots - rseslots;

  /* Allocate a new RSE frame.  */
  regcache_cooked_read_unsigned (regcache, IA64_BSP_REGNUM, &bsp);
  tdep->infcall_ops.allocate_new_rse_frame (regcache, bsp, rseslots);
  
  /* We will attempt to find function descriptors in the .opd segment,
     but if we can't we'll construct them ourselves.  That being the
     case, we'll need to reserve space on the stack for them.  */
  funcdescaddr = sp - nfuncargs * 16;
  funcdescaddr &= ~0xfLL;

  /* Adjust the stack pointer to it's new value.  The calling conventions
     require us to have 16 bytes of scratch, plus whatever space is
     necessary for the memory slots and our function descriptors.  */
  sp = sp - 16 - (memslots + nfuncargs) * 8;
  sp &= ~0xfLL;				/* Maintain 16 byte alignment.  */

  /* Place the arguments where they belong.  The arguments will be
     either placed in the RSE backing store or on the memory stack.
     In addition, floating point arguments or HFAs are placed in
     floating point registers.  */
  slotnum = 0;
  floatreg = IA64_FR8_REGNUM;
  for (argno = 0; argno < nargs; argno++)
    {
      struct type *float_elt_type;

      arg = args[argno];
      type = check_typedef (arg->type ());
      len = type->length ();

      /* Special handling for function parameters.  */
      if (len == 8
	  && type->code () == TYPE_CODE_PTR
	  && type->target_type ()->code () == TYPE_CODE_FUNC)
	{
	  gdb_byte val_buf[8];
	  ULONGEST faddr = extract_unsigned_integer
	    (arg->contents ().data (), 8, byte_order);
	  store_unsigned_integer (val_buf, 8, byte_order,
				  find_func_descr (regcache, faddr,
						   &funcdescaddr));
	  if (slotnum < rseslots)
	    tdep->infcall_ops.store_argument_in_slot (regcache, bsp,
						      slotnum, val_buf);
	  else
	    write_memory (sp + 16 + 8 * (slotnum - rseslots), val_buf, 8);
	  slotnum++;
	  continue;
	}

      /* Normal slots.  */

      /* Skip odd slot if necessary...  */
      if ((slotnum & 1) && slot_alignment_is_next_even (type))
	slotnum++;

      argoffset = 0;
      while (len > 0)
	{
	  gdb_byte val_buf[8];

	  memset (val_buf, 0, 8);
	  if (!ia64_struct_type_p (type) && len < 8)
	    {
	      /* Integral types are LSB-aligned, so we have to be careful
		 to insert the argument on the correct side of the buffer.
		 This is why we use store_unsigned_integer.  */
	      store_unsigned_integer
		(val_buf, 8, byte_order,
		 extract_unsigned_integer (arg->contents ().data (), len,
					   byte_order));
	    }
	  else
	    {
	      /* This is either an 8bit integral type, or an aggregate.
		 For 8bit integral type, there is no problem, we just
		 copy the value over.

		 For aggregates, the only potentially tricky portion
		 is to write the last one if it is less than 8 bytes.
		 In this case, the data is Byte0-aligned.  Happy news,
		 this means that we don't need to differentiate the
		 handling of 8byte blocks and less-than-8bytes blocks.  */
	      memcpy (val_buf, arg->contents ().data () + argoffset,
		      (len > 8) ? 8 : len);
	    }

	  if (slotnum < rseslots)
	    tdep->infcall_ops.store_argument_in_slot (regcache, bsp,
						      slotnum, val_buf);
	  else
	    write_memory (sp + 16 + 8 * (slotnum - rseslots), val_buf, 8);

	  argoffset += 8;
	  len -= 8;
	  slotnum++;
	}

      /* Handle floating point types (including HFAs).  */
      float_elt_type = is_float_or_hfa_type (type);
      if (float_elt_type != NULL)
	{
	  argoffset = 0;
	  len = type->length ();
	  while (len > 0 && floatreg < IA64_FR16_REGNUM)
	    {
	      gdb_byte to[IA64_FP_REGISTER_SIZE];
	      target_float_convert (arg->contents ().data () + argoffset,
				    float_elt_type, to,
				    ia64_ext_type (gdbarch));
	      regcache->cooked_write (floatreg, to);
	      floatreg++;
	      argoffset += float_elt_type->length ();
	      len -= float_elt_type->length ();
	    }
	}
    }

  /* Store the struct return value in r8 if necessary.  */
  if (return_method == return_method_struct)
    regcache_cooked_write_unsigned (regcache, IA64_GR8_REGNUM,
				    (ULONGEST) struct_addr);

  global_pointer = ia64_find_global_pointer (gdbarch, func_addr);

  if (global_pointer != 0)
    regcache_cooked_write_unsigned (regcache, IA64_GR1_REGNUM, global_pointer);

  /* The following is not necessary on HP-UX, because we're using
     a dummy code sequence pushed on the stack to make the call, and
     this sequence doesn't need b0 to be set in order for our dummy
     breakpoint to be hit.  Nonetheless, this doesn't interfere, and
     it's needed for other OSes, so we do this unconditionaly.  */
  regcache_cooked_write_unsigned (regcache, IA64_BR0_REGNUM, bp_addr);

  regcache_cooked_write_unsigned (regcache, sp_regnum, sp);

  tdep->infcall_ops.set_function_addr (regcache, func_addr);

  return sp;
}

static const struct ia64_infcall_ops ia64_infcall_ops =
{
  ia64_allocate_new_rse_frame,
  ia64_store_argument_in_slot,
  ia64_set_function_addr
};

static struct frame_id
ia64_dummy_id (struct gdbarch *gdbarch, frame_info_ptr this_frame)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte buf[8];
  CORE_ADDR sp, bsp;

  get_frame_register (this_frame, sp_regnum, buf);
  sp = extract_unsigned_integer (buf, 8, byte_order);

  get_frame_register (this_frame, IA64_BSP_REGNUM, buf);
  bsp = extract_unsigned_integer (buf, 8, byte_order);

  if (gdbarch_debug >= 1)
    gdb_printf (gdb_stdlog,
		"dummy frame id: code %s, stack %s, special %s\n",
		paddress (gdbarch, get_frame_pc (this_frame)),
		paddress (gdbarch, sp), paddress (gdbarch, bsp));

  return frame_id_build_special (sp, get_frame_pc (this_frame), bsp);
}

static CORE_ADDR 
ia64_unwind_pc (struct gdbarch *gdbarch, frame_info_ptr next_frame)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte buf[8];
  CORE_ADDR ip, psr, pc;

  frame_unwind_register (next_frame, IA64_IP_REGNUM, buf);
  ip = extract_unsigned_integer (buf, 8, byte_order);
  frame_unwind_register (next_frame, IA64_PSR_REGNUM, buf);
  psr = extract_unsigned_integer (buf, 8, byte_order);
 
  pc = (ip & ~0xf) | ((psr >> 41) & 3);
  return pc;
}

static int
ia64_print_insn (bfd_vma memaddr, struct disassemble_info *info)
{
  info->bytes_per_line = SLOT_MULTIPLIER;
  return default_print_insn (memaddr, info);
}

/* The default "size_of_register_frame" gdbarch_tdep routine for ia64.  */

static int
ia64_size_of_register_frame (frame_info_ptr this_frame, ULONGEST cfm)
{
  return (cfm & 0x7f);
}

static struct gdbarch *
ia64_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  /* If there is already a candidate, use it.  */
  arches = gdbarch_list_lookup_by_info (arches, &info);
  if (arches != NULL)
    return arches->gdbarch;

  gdbarch *gdbarch
    = gdbarch_alloc (&info, gdbarch_tdep_up (new ia64_gdbarch_tdep));
  ia64_gdbarch_tdep *tdep = gdbarch_tdep<ia64_gdbarch_tdep> (gdbarch);

  tdep->size_of_register_frame = ia64_size_of_register_frame;

  /* According to the ia64 specs, instructions that store long double
     floats in memory use a long-double format different than that
     used in the floating registers.  The memory format matches the
     x86 extended float format which is 80 bits.  An OS may choose to
     use this format (e.g. GNU/Linux) or choose to use a different
     format for storing long doubles (e.g. HPUX).  In the latter case,
     the setting of the format may be moved/overridden in an
     OS-specific tdep file.  */
  set_gdbarch_long_double_format (gdbarch, floatformats_i387_ext);

  set_gdbarch_short_bit (gdbarch, 16);
  set_gdbarch_int_bit (gdbarch, 32);
  set_gdbarch_long_bit (gdbarch, 64);
  set_gdbarch_long_long_bit (gdbarch, 64);
  set_gdbarch_float_bit (gdbarch, 32);
  set_gdbarch_double_bit (gdbarch, 64);
  set_gdbarch_long_double_bit (gdbarch, 128);
  set_gdbarch_ptr_bit (gdbarch, 64);

  set_gdbarch_num_regs (gdbarch, NUM_IA64_RAW_REGS);
  set_gdbarch_num_pseudo_regs (gdbarch,
			       LAST_PSEUDO_REGNUM - FIRST_PSEUDO_REGNUM);
  set_gdbarch_sp_regnum (gdbarch, sp_regnum);
  set_gdbarch_fp0_regnum (gdbarch, IA64_FR0_REGNUM);

  set_gdbarch_register_name (gdbarch, ia64_register_name);
  set_gdbarch_register_type (gdbarch, ia64_register_type);

  set_gdbarch_pseudo_register_read (gdbarch, ia64_pseudo_register_read);
  set_gdbarch_deprecated_pseudo_register_write (gdbarch,
						ia64_pseudo_register_write);
  set_gdbarch_dwarf2_reg_to_regnum (gdbarch, ia64_dwarf_reg_to_regnum);
  set_gdbarch_register_reggroup_p (gdbarch, ia64_register_reggroup_p);
  set_gdbarch_convert_register_p (gdbarch, ia64_convert_register_p);
  set_gdbarch_register_to_value (gdbarch, ia64_register_to_value);
  set_gdbarch_value_to_register (gdbarch, ia64_value_to_register);

  set_gdbarch_skip_prologue (gdbarch, ia64_skip_prologue);

  set_gdbarch_return_value (gdbarch, ia64_return_value);

  set_gdbarch_memory_insert_breakpoint (gdbarch,
					ia64_memory_insert_breakpoint);
  set_gdbarch_memory_remove_breakpoint (gdbarch,
					ia64_memory_remove_breakpoint);
  set_gdbarch_breakpoint_from_pc (gdbarch, ia64_breakpoint_from_pc);
  set_gdbarch_breakpoint_kind_from_pc (gdbarch, ia64_breakpoint_kind_from_pc);
  set_gdbarch_read_pc (gdbarch, ia64_read_pc);
  set_gdbarch_write_pc (gdbarch, ia64_write_pc);

  /* Settings for calling functions in the inferior.  */
  set_gdbarch_push_dummy_call (gdbarch, ia64_push_dummy_call);
  tdep->infcall_ops = ia64_infcall_ops;
  set_gdbarch_frame_align (gdbarch, ia64_frame_align);
  set_gdbarch_dummy_id (gdbarch, ia64_dummy_id);

  set_gdbarch_unwind_pc (gdbarch, ia64_unwind_pc);
#ifdef HAVE_LIBUNWIND_IA64_H
  frame_unwind_append_unwinder (gdbarch,
				&ia64_libunwind_sigtramp_frame_unwind);
  frame_unwind_append_unwinder (gdbarch, &ia64_libunwind_frame_unwind);
  frame_unwind_append_unwinder (gdbarch, &ia64_sigtramp_frame_unwind);
  libunwind_frame_set_descr (gdbarch, &ia64_libunwind_descr);
#else
  frame_unwind_append_unwinder (gdbarch, &ia64_sigtramp_frame_unwind);
#endif
  frame_unwind_append_unwinder (gdbarch, &ia64_frame_unwind);
  frame_base_set_default (gdbarch, &ia64_frame_base);

  /* Settings that should be unnecessary.  */
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);

  set_gdbarch_print_insn (gdbarch, ia64_print_insn);
  set_gdbarch_convert_from_func_ptr_addr (gdbarch,
					  ia64_convert_from_func_ptr_addr);

  /* The virtual table contains 16-byte descriptors, not pointers to
     descriptors.  */
  set_gdbarch_vtable_function_descriptors (gdbarch, 1);

  /* Hook in ABI-specific overrides, if they have been registered.  */
  gdbarch_init_osabi (info, gdbarch);

  return gdbarch;
}

void _initialize_ia64_tdep ();
void
_initialize_ia64_tdep ()
{
  gdbarch_register (bfd_arch_ia64, ia64_gdbarch_init, NULL);
}
