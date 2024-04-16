/* Common target dependent code for GDB on ARM systems.

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

#include <ctype.h>

#include "frame.h"
#include "language.h"
#include "inferior.h"
#include "infrun.h"
#include "gdbcmd.h"
#include "gdbcore.h"
#include "dis-asm.h"
#include "disasm.h"
#include "regcache.h"
#include "reggroups.h"
#include "target-float.h"
#include "value.h"
#include "arch-utils.h"
#include "osabi.h"
#include "frame-unwind.h"
#include "frame-base.h"
#include "trad-frame.h"
#include "objfiles.h"
#include "dwarf2.h"
#include "dwarf2/frame.h"
#include "gdbtypes.h"
#include "prologue-value.h"
#include "remote.h"
#include "target-descriptions.h"
#include "user-regs.h"
#include "observable.h"
#include "count-one-bits.h"

#include "arch/arm.h"
#include "arch/arm-get-next-pcs.h"
#include "arm-tdep.h"
#include "sim/sim-arm.h"

#include "elf-bfd.h"
#include "coff/internal.h"
#include "elf/arm.h"

#include "record.h"
#include "record-full.h"
#include <algorithm>

#include "producer.h"

#if GDB_SELF_TEST
#include "gdbsupport/selftest.h"
#endif

static bool arm_debug;

/* Print an "arm" debug statement.  */

#define arm_debug_printf(fmt, ...) \
  debug_prefixed_printf_cond (arm_debug, "arm", fmt, ##__VA_ARGS__)

/* Macros for setting and testing a bit in a minimal symbol that marks
   it as Thumb function.  The MSB of the minimal symbol's "info" field
   is used for this purpose.

   MSYMBOL_SET_SPECIAL	Actually sets the "special" bit.
   MSYMBOL_IS_SPECIAL   Tests the "special" bit in a minimal symbol.  */

#define MSYMBOL_SET_SPECIAL(msym)				\
	(msym)->set_target_flag_1 (true)

#define MSYMBOL_IS_SPECIAL(msym)				\
	(msym)->target_flag_1 ()

struct arm_mapping_symbol
{
  CORE_ADDR value;
  char type;

  bool operator< (const arm_mapping_symbol &other) const
  { return this->value < other.value; }
};

typedef std::vector<arm_mapping_symbol> arm_mapping_symbol_vec;

struct arm_per_bfd
{
  explicit arm_per_bfd (size_t num_sections)
  : section_maps (new arm_mapping_symbol_vec[num_sections]),
    section_maps_sorted (new bool[num_sections] ())
  {}

  DISABLE_COPY_AND_ASSIGN (arm_per_bfd);

  /* Information about mapping symbols ($a, $d, $t) in the objfile.

     The format is an array of vectors of arm_mapping_symbols, there is one
     vector for each section of the objfile (the array is index by BFD section
     index).

     For each section, the vector of arm_mapping_symbol is sorted by
     symbol value (address).  */
  std::unique_ptr<arm_mapping_symbol_vec[]> section_maps;

  /* For each corresponding element of section_maps above, is this vector
     sorted.  */
  std::unique_ptr<bool[]> section_maps_sorted;
};

/* Per-bfd data used for mapping symbols.  */
static const registry<bfd>::key<arm_per_bfd> arm_bfd_data_key;

/* The list of available "set arm ..." and "show arm ..." commands.  */
static struct cmd_list_element *setarmcmdlist = NULL;
static struct cmd_list_element *showarmcmdlist = NULL;

/* The type of floating-point to use.  Keep this in sync with enum
   arm_float_model, and the help string in _initialize_arm_tdep.  */
static const char *const fp_model_strings[] =
{
  "auto",
  "softfpa",
  "fpa",
  "softvfp",
  "vfp",
  NULL
};

/* A variable that can be configured by the user.  */
static enum arm_float_model arm_fp_model = ARM_FLOAT_AUTO;
static const char *current_fp_model = "auto";

/* The ABI to use.  Keep this in sync with arm_abi_kind.  */
static const char *const arm_abi_strings[] =
{
  "auto",
  "APCS",
  "AAPCS",
  NULL
};

/* A variable that can be configured by the user.  */
static enum arm_abi_kind arm_abi_global = ARM_ABI_AUTO;
static const char *arm_abi_string = "auto";

/* The execution mode to assume.  */
static const char *const arm_mode_strings[] =
  {
    "auto",
    "arm",
    "thumb",
    NULL
  };

static const char *arm_fallback_mode_string = "auto";
static const char *arm_force_mode_string = "auto";

/* The standard register names, and all the valid aliases for them.  Note
   that `fp', `sp' and `pc' are not added in this alias list, because they
   have been added as builtin user registers in
   std-regs.c:_initialize_frame_reg.  */
static const struct
{
  const char *name;
  int regnum;
} arm_register_aliases[] = {
  /* Basic register numbers.  */
  { "r0", 0 },
  { "r1", 1 },
  { "r2", 2 },
  { "r3", 3 },
  { "r4", 4 },
  { "r5", 5 },
  { "r6", 6 },
  { "r7", 7 },
  { "r8", 8 },
  { "r9", 9 },
  { "r10", 10 },
  { "r11", 11 },
  { "r12", 12 },
  { "r13", 13 },
  { "r14", 14 },
  { "r15", 15 },
  /* Synonyms (argument and variable registers).  */
  { "a1", 0 },
  { "a2", 1 },
  { "a3", 2 },
  { "a4", 3 },
  { "v1", 4 },
  { "v2", 5 },
  { "v3", 6 },
  { "v4", 7 },
  { "v5", 8 },
  { "v6", 9 },
  { "v7", 10 },
  { "v8", 11 },
  /* Other platform-specific names for r9.  */
  { "sb", 9 },
  { "tr", 9 },
  /* Special names.  */
  { "ip", 12 },
  { "lr", 14 },
  /* Names used by GCC (not listed in the ARM EABI).  */
  { "sl", 10 },
  /* A special name from the older ATPCS.  */
  { "wr", 7 },
};

static const char *const arm_register_names[] =
{"r0",  "r1",  "r2",  "r3",	/*  0  1  2  3 */
 "r4",  "r5",  "r6",  "r7",	/*  4  5  6  7 */
 "r8",  "r9",  "r10", "r11",	/*  8  9 10 11 */
 "r12", "sp",  "lr",  "pc",	/* 12 13 14 15 */
 "f0",  "f1",  "f2",  "f3",	/* 16 17 18 19 */
 "f4",  "f5",  "f6",  "f7",	/* 20 21 22 23 */
 "fps", "cpsr" };		/* 24 25       */

/* Holds the current set of options to be passed to the disassembler.  */
static char *arm_disassembler_options;

/* Valid register name styles.  */
static const char **valid_disassembly_styles;

/* Disassembly style to use. Default to "std" register names.  */
static const char *disassembly_style;

/* All possible arm target descriptors.  */
static struct target_desc *tdesc_arm_list[ARM_FP_TYPE_INVALID][2];
static struct target_desc *tdesc_arm_mprofile_list[ARM_M_TYPE_INVALID];

/* This is used to keep the bfd arch_info in sync with the disassembly
   style.  */
static void set_disassembly_style_sfunc (const char *, int,
					 struct cmd_list_element *);
static void show_disassembly_style_sfunc (struct ui_file *, int,
					  struct cmd_list_element *,
					  const char *);

static enum register_status arm_neon_quad_read (struct gdbarch *gdbarch,
						readable_regcache *regcache,
						int regnum, gdb_byte *buf);
static void arm_neon_quad_write (struct gdbarch *gdbarch,
				 struct regcache *regcache,
				 int regnum, const gdb_byte *buf);

static CORE_ADDR
  arm_get_next_pcs_syscall_next_pc (struct arm_get_next_pcs *self);


/* get_next_pcs operations.  */
static struct arm_get_next_pcs_ops arm_get_next_pcs_ops = {
  arm_get_next_pcs_read_memory_unsigned_integer,
  arm_get_next_pcs_syscall_next_pc,
  arm_get_next_pcs_addr_bits_remove,
  arm_get_next_pcs_is_thumb,
  NULL,
};

struct arm_prologue_cache
{
  /* The stack pointer at the time this frame was created; i.e. the
     caller's stack pointer when this function was called.  It is used
     to identify this frame.  */
  CORE_ADDR sp;

  /* Additional stack pointers used by M-profile with Security extension.  */
  /* Use msp_s / psp_s to hold the values of msp / psp when there is
     no Security extension.  */
  CORE_ADDR msp_s;
  CORE_ADDR msp_ns;
  CORE_ADDR psp_s;
  CORE_ADDR psp_ns;

  /* Active stack pointer.  */
  int active_sp_regnum;
  int active_msp_regnum;
  int active_psp_regnum;

  /* The frame base for this frame is just prev_sp - frame size.
     FRAMESIZE is the distance from the frame pointer to the
     initial stack pointer.  */

  int framesize;

  /* The register used to hold the frame pointer for this frame.  */
  int framereg;

  /* True if the return address is signed, false otherwise.  */
  std::optional<bool> ra_signed_state;

  /* Saved register offsets.  */
  trad_frame_saved_reg *saved_regs;

  arm_prologue_cache() = default;
};


/* Reconstruct T bit in program status register from LR value.  */

static inline ULONGEST
reconstruct_t_bit(struct gdbarch *gdbarch, CORE_ADDR lr, ULONGEST psr)
{
  ULONGEST t_bit = arm_psr_thumb_bit (gdbarch);
  if (IS_THUMB_ADDR (lr))
    psr |= t_bit;
  else
    psr &= ~t_bit;

  return psr;
}

/* Initialize CACHE fields for which zero is not adequate (CACHE is
   expected to have been ZALLOC'ed before calling this function).  */

static void
arm_cache_init (struct arm_prologue_cache *cache, struct gdbarch *gdbarch)
{
  cache->active_sp_regnum = ARM_SP_REGNUM;

  cache->saved_regs = trad_frame_alloc_saved_regs (gdbarch);
}

/* Similar to the previous function, but extracts GDBARCH from FRAME.  */

static void
arm_cache_init (struct arm_prologue_cache *cache, frame_info_ptr frame)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  arm_cache_init (cache, gdbarch);
  cache->sp = get_frame_register_unsigned (frame, ARM_SP_REGNUM);

  if (tdep->have_sec_ext)
    {
      const CORE_ADDR msp_val
	= get_frame_register_unsigned (frame, tdep->m_profile_msp_regnum);
      const CORE_ADDR psp_val
	= get_frame_register_unsigned (frame, tdep->m_profile_psp_regnum);

      cache->msp_s
	= get_frame_register_unsigned (frame, tdep->m_profile_msp_s_regnum);
      cache->msp_ns
	= get_frame_register_unsigned (frame, tdep->m_profile_msp_ns_regnum);
      cache->psp_s
	= get_frame_register_unsigned (frame, tdep->m_profile_psp_s_regnum);
      cache->psp_ns
	= get_frame_register_unsigned (frame, tdep->m_profile_psp_ns_regnum);

      /* Identify what msp is alias for (msp_s or msp_ns).  */
      if (msp_val == cache->msp_s)
	cache->active_msp_regnum = tdep->m_profile_msp_s_regnum;
      else if (msp_val == cache->msp_ns)
	cache->active_msp_regnum = tdep->m_profile_msp_ns_regnum;
      else
	{
	  warning (_("Invalid state, unable to determine msp alias, assuming "
		     "msp_s."));
	  cache->active_msp_regnum = tdep->m_profile_msp_s_regnum;
	}

      /* Identify what psp is alias for (psp_s or psp_ns).  */
      if (psp_val == cache->psp_s)
	cache->active_psp_regnum = tdep->m_profile_psp_s_regnum;
      else if (psp_val == cache->psp_ns)
	cache->active_psp_regnum = tdep->m_profile_psp_ns_regnum;
      else
	{
	  warning (_("Invalid state, unable to determine psp alias, assuming "
		     "psp_s."));
	  cache->active_psp_regnum = tdep->m_profile_psp_s_regnum;
	}

      /* Identify what sp is alias for (msp_s, msp_ns, psp_s or psp_ns).  */
      if (msp_val == cache->sp)
	cache->active_sp_regnum = cache->active_msp_regnum;
      else if (psp_val == cache->sp)
	cache->active_sp_regnum = cache->active_psp_regnum;
      else
	{
	  warning (_("Invalid state, unable to determine sp alias, assuming "
		     "msp."));
	  cache->active_sp_regnum = cache->active_msp_regnum;
	}
    }
  else if (tdep->is_m)
    {
      cache->msp_s
	= get_frame_register_unsigned (frame, tdep->m_profile_msp_regnum);
      cache->psp_s
	= get_frame_register_unsigned (frame, tdep->m_profile_psp_regnum);

      /* Identify what sp is alias for (msp or psp).  */
      if (cache->msp_s == cache->sp)
	cache->active_sp_regnum = tdep->m_profile_msp_regnum;
      else if (cache->psp_s == cache->sp)
	cache->active_sp_regnum = tdep->m_profile_psp_regnum;
      else
	{
	  warning (_("Invalid state, unable to determine sp alias, assuming "
		     "msp."));
	  cache->active_sp_regnum = tdep->m_profile_msp_regnum;
	}
    }
  else
    {
      cache->msp_s
	= get_frame_register_unsigned (frame, ARM_SP_REGNUM);

      cache->active_sp_regnum = ARM_SP_REGNUM;
    }
}

/* Return the requested stack pointer value (in REGNUM), taking into
   account whether we have a Security extension or an M-profile
   CPU.  */

static CORE_ADDR
arm_cache_get_sp_register (struct arm_prologue_cache *cache,
			   arm_gdbarch_tdep *tdep, int regnum)
{
  if (tdep->have_sec_ext)
    {
      if (regnum == tdep->m_profile_msp_s_regnum)
	return cache->msp_s;
      if (regnum == tdep->m_profile_msp_ns_regnum)
	return cache->msp_ns;
      if (regnum == tdep->m_profile_psp_s_regnum)
	return cache->psp_s;
      if (regnum == tdep->m_profile_psp_ns_regnum)
	return cache->psp_ns;
      if (regnum == tdep->m_profile_msp_regnum)
	return arm_cache_get_sp_register (cache, tdep, cache->active_msp_regnum);
      if (regnum == tdep->m_profile_psp_regnum)
	return arm_cache_get_sp_register (cache, tdep, cache->active_psp_regnum);
      if (regnum == ARM_SP_REGNUM)
	return arm_cache_get_sp_register (cache, tdep, cache->active_sp_regnum);
    }
  else if (tdep->is_m)
    {
      if (regnum == tdep->m_profile_msp_regnum)
	return cache->msp_s;
      if (regnum == tdep->m_profile_psp_regnum)
	return cache->psp_s;
      if (regnum == ARM_SP_REGNUM)
	return arm_cache_get_sp_register (cache, tdep, cache->active_sp_regnum);
    }
  else if (regnum == ARM_SP_REGNUM)
    return cache->sp;

  gdb_assert_not_reached ("Invalid SP selection");
}

/* Return the previous stack address, depending on which SP register
   is active.  */

static CORE_ADDR
arm_cache_get_prev_sp_value (struct arm_prologue_cache *cache, arm_gdbarch_tdep *tdep)
{
  CORE_ADDR val = arm_cache_get_sp_register (cache, tdep, cache->active_sp_regnum);
  return val;
}

/* Set the active stack pointer to VAL.  */

static void
arm_cache_set_active_sp_value (struct arm_prologue_cache *cache,
			       arm_gdbarch_tdep *tdep, CORE_ADDR val)
{
  if (tdep->have_sec_ext)
    {
      if (cache->active_sp_regnum == tdep->m_profile_msp_s_regnum)
	cache->msp_s = val;
      else if (cache->active_sp_regnum == tdep->m_profile_msp_ns_regnum)
	cache->msp_ns = val;
      else if (cache->active_sp_regnum == tdep->m_profile_psp_s_regnum)
	cache->psp_s = val;
      else if (cache->active_sp_regnum == tdep->m_profile_psp_ns_regnum)
	cache->psp_ns = val;

      return;
    }
  else if (tdep->is_m)
    {
      if (cache->active_sp_regnum == tdep->m_profile_msp_regnum)
	cache->msp_s = val;
      else if (cache->active_sp_regnum == tdep->m_profile_psp_regnum)
	cache->psp_s = val;

      return;
    }
  else if (cache->active_sp_regnum == ARM_SP_REGNUM)
    {
      cache->sp = val;
      return;
    }

  gdb_assert_not_reached ("Invalid SP selection");
}

/* Return true if REGNUM is one of the alternative stack pointers.  */

static bool
arm_is_alternative_sp_register (arm_gdbarch_tdep *tdep, int regnum)
{
  if ((regnum == tdep->m_profile_msp_regnum)
      || (regnum == tdep->m_profile_msp_s_regnum)
      || (regnum == tdep->m_profile_msp_ns_regnum)
      || (regnum == tdep->m_profile_psp_regnum)
      || (regnum == tdep->m_profile_psp_s_regnum)
      || (regnum == tdep->m_profile_psp_ns_regnum))
    return true;
  else
    return false;
}

/* Set the active stack pointer to SP_REGNUM.  */

static void
arm_cache_switch_prev_sp (struct arm_prologue_cache *cache,
			  arm_gdbarch_tdep *tdep, int sp_regnum)
{
  gdb_assert (arm_is_alternative_sp_register (tdep, sp_regnum));

  if (tdep->have_sec_ext)
    {
      gdb_assert (sp_regnum != tdep->m_profile_msp_regnum
		  && sp_regnum != tdep->m_profile_psp_regnum);

      if (sp_regnum == tdep->m_profile_msp_s_regnum
	  || sp_regnum == tdep->m_profile_psp_s_regnum)
	{
	  cache->active_msp_regnum = tdep->m_profile_msp_s_regnum;
	  cache->active_psp_regnum = tdep->m_profile_psp_s_regnum;
	}
      else if (sp_regnum == tdep->m_profile_msp_ns_regnum
	       || sp_regnum == tdep->m_profile_psp_ns_regnum)
	{
	  cache->active_msp_regnum = tdep->m_profile_msp_ns_regnum;
	  cache->active_psp_regnum = tdep->m_profile_psp_ns_regnum;
	}
    }

  cache->active_sp_regnum = sp_regnum;
}

namespace {

/* Abstract class to read ARM instructions from memory.  */

class arm_instruction_reader
{
public:
  /* Read a 4 bytes instruction from memory using the BYTE_ORDER endianness.  */
  virtual uint32_t read (CORE_ADDR memaddr, bfd_endian byte_order) const = 0;
};

/* Read instructions from target memory.  */

class target_arm_instruction_reader : public arm_instruction_reader
{
public:
  uint32_t read (CORE_ADDR memaddr, bfd_endian byte_order) const override
  {
    return read_code_unsigned_integer (memaddr, 4, byte_order);
  }
};

} /* namespace */

static CORE_ADDR arm_analyze_prologue
  (struct gdbarch *gdbarch, CORE_ADDR prologue_start, CORE_ADDR prologue_end,
   struct arm_prologue_cache *cache, const arm_instruction_reader &insn_reader);

/* Architecture version for displaced stepping.  This effects the behaviour of
   certain instructions, and really should not be hard-wired.  */

#define DISPLACED_STEPPING_ARCH_VERSION		5

/* See arm-tdep.h.  */

bool arm_apcs_32 = true;
bool arm_unwind_secure_frames = true;

/* Return the bit mask in ARM_PS_REGNUM that indicates Thumb mode.  */

int
arm_psr_thumb_bit (struct gdbarch *gdbarch)
{
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  if (tdep->is_m)
    return XPSR_T;
  else
    return CPSR_T;
}

/* Determine if the processor is currently executing in Thumb mode.  */

int
arm_is_thumb (struct regcache *regcache)
{
  ULONGEST cpsr;
  ULONGEST t_bit = arm_psr_thumb_bit (regcache->arch ());

  cpsr = regcache_raw_get_unsigned (regcache, ARM_PS_REGNUM);

  return (cpsr & t_bit) != 0;
}

/* Determine if FRAME is executing in Thumb mode.  FRAME must be an ARM
   frame.  */

int
arm_frame_is_thumb (frame_info_ptr frame)
{
  /* Check the architecture of FRAME.  */
  struct gdbarch *gdbarch = get_frame_arch (frame);
  gdb_assert (gdbarch_bfd_arch_info (gdbarch)->arch == bfd_arch_arm);

  /* Every ARM frame unwinder can unwind the T bit of the CPSR, either
     directly (from a signal frame or dummy frame) or by interpreting
     the saved LR (from a prologue or DWARF frame).  So consult it and
     trust the unwinders.  */
  CORE_ADDR cpsr = get_frame_register_unsigned (frame, ARM_PS_REGNUM);

  /* Find and extract the thumb bit.  */
  ULONGEST t_bit = arm_psr_thumb_bit (gdbarch);
  return (cpsr & t_bit) != 0;
}

/* Search for the mapping symbol covering MEMADDR.  If one is found,
   return its type.  Otherwise, return 0.  If START is non-NULL,
   set *START to the location of the mapping symbol.  */

static char
arm_find_mapping_symbol (CORE_ADDR memaddr, CORE_ADDR *start)
{
  struct obj_section *sec;

  /* If there are mapping symbols, consult them.  */
  sec = find_pc_section (memaddr);
  if (sec != NULL)
    {
      arm_per_bfd *data = arm_bfd_data_key.get (sec->objfile->obfd.get ());
      if (data != NULL)
	{
	  unsigned int section_idx = sec->the_bfd_section->index;
	  arm_mapping_symbol_vec &map
	    = data->section_maps[section_idx];

	  /* Sort the vector on first use.  */
	  if (!data->section_maps_sorted[section_idx])
	    {
	      std::sort (map.begin (), map.end ());
	      data->section_maps_sorted[section_idx] = true;
	    }

	  arm_mapping_symbol map_key = { memaddr - sec->addr (), 0 };
	  arm_mapping_symbol_vec::const_iterator it
	    = std::lower_bound (map.begin (), map.end (), map_key);

	  /* std::lower_bound finds the earliest ordered insertion
	     point.  If the symbol at this position starts at this exact
	     address, we use that; otherwise, the preceding
	     mapping symbol covers this address.  */
	  if (it < map.end ())
	    {
	      if (it->value == map_key.value)
		{
		  if (start)
		    *start = it->value + sec->addr ();
		  return it->type;
		}
	    }

	  if (it > map.begin ())
	    {
	      arm_mapping_symbol_vec::const_iterator prev_it
		= it - 1;

	      if (start)
		*start = prev_it->value + sec->addr ();
	      return prev_it->type;
	    }
	}
    }

  return 0;
}

/* Determine if the program counter specified in MEMADDR is in a Thumb
   function.  This function should be called for addresses unrelated to
   any executing frame; otherwise, prefer arm_frame_is_thumb.  */

int
arm_pc_is_thumb (struct gdbarch *gdbarch, CORE_ADDR memaddr)
{
  struct bound_minimal_symbol sym;
  char type;
  arm_displaced_step_copy_insn_closure *dsc = nullptr;
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  if (gdbarch_displaced_step_copy_insn_closure_by_addr_p (gdbarch))
    dsc = ((arm_displaced_step_copy_insn_closure * )
	   gdbarch_displaced_step_copy_insn_closure_by_addr
	     (gdbarch, current_inferior (), memaddr));

  /* If checking the mode of displaced instruction in copy area, the mode
     should be determined by instruction on the original address.  */
  if (dsc)
    {
      displaced_debug_printf ("check mode of %.8lx instead of %.8lx",
			      (unsigned long) dsc->insn_addr,
			      (unsigned long) memaddr);
      memaddr = dsc->insn_addr;
    }

  /* If bit 0 of the address is set, assume this is a Thumb address.  */
  if (IS_THUMB_ADDR (memaddr))
    return 1;

  /* If the user wants to override the symbol table, let him.  */
  if (strcmp (arm_force_mode_string, "arm") == 0)
    return 0;
  if (strcmp (arm_force_mode_string, "thumb") == 0)
    return 1;

  /* ARM v6-M and v7-M are always in Thumb mode.  */
  if (tdep->is_m)
    return 1;

  /* If there are mapping symbols, consult them.  */
  type = arm_find_mapping_symbol (memaddr, NULL);
  if (type)
    return type == 't';

  /* Thumb functions have a "special" bit set in minimal symbols.  */
  sym = lookup_minimal_symbol_by_pc (memaddr);
  if (sym.minsym)
    return (MSYMBOL_IS_SPECIAL (sym.minsym));

  /* If the user wants to override the fallback mode, let them.  */
  if (strcmp (arm_fallback_mode_string, "arm") == 0)
    return 0;
  if (strcmp (arm_fallback_mode_string, "thumb") == 0)
    return 1;

  /* If we couldn't find any symbol, but we're talking to a running
     target, then trust the current value of $cpsr.  This lets
     "display/i $pc" always show the correct mode (though if there is
     a symbol table we will not reach here, so it still may not be
     displayed in the mode it will be executed).  */
  if (target_has_registers ())
    return arm_frame_is_thumb (get_current_frame ());

  /* Otherwise we're out of luck; we assume ARM.  */
  return 0;
}

static inline bool
arm_m_addr_is_lockup (CORE_ADDR addr)
{
  switch (addr)
    {
      /* Values for lockup state.
	 For more details see "B1.5.15 Unrecoverable exception cases" in
	 both ARMv6-M and ARMv7-M Architecture Reference Manuals, or
	 see "B4.32 Lockup" in ARMv8-M Architecture Reference Manual.  */
      case 0xeffffffe:
      case 0xfffffffe:
      case 0xffffffff:
	return true;

      default:
	/* Address is not lockup.  */
	return false;
    }
}

/* Determine if the address specified equals any of these magic return
   values, called EXC_RETURN, defined by the ARM v6-M, v7-M and v8-M
   architectures.  Also include lockup magic PC value.
   Check also for FNC_RETURN if we have the v8-M security extension.

   From ARMv6-M Reference Manual B1.5.8
   Table B1-5 Exception return behavior

   EXC_RETURN    Return To        Return Stack
   0xFFFFFFF1    Handler mode     Main
   0xFFFFFFF9    Thread mode      Main
   0xFFFFFFFD    Thread mode      Process

   From ARMv7-M Reference Manual B1.5.8
   Table B1-8 EXC_RETURN definition of exception return behavior, no FP

   EXC_RETURN    Return To        Return Stack
   0xFFFFFFF1    Handler mode     Main
   0xFFFFFFF9    Thread mode      Main
   0xFFFFFFFD    Thread mode      Process

   Table B1-9 EXC_RETURN definition of exception return behavior, with
   FP

   EXC_RETURN    Return To        Return Stack    Frame Type
   0xFFFFFFE1    Handler mode     Main            Extended
   0xFFFFFFE9    Thread mode      Main            Extended
   0xFFFFFFED    Thread mode      Process         Extended
   0xFFFFFFF1    Handler mode     Main            Basic
   0xFFFFFFF9    Thread mode      Main            Basic
   0xFFFFFFFD    Thread mode      Process         Basic

   For more details see "B1.5.8 Exception return behavior"
   in both ARMv6-M and ARMv7-M Architecture Reference Manuals.

   From ARMv8-M Architecture Technical Reference, D1.2.95
   FType, Mode and SPSEL bits are to be considered when the Security
   Extension is not implemented.

   EXC_RETURN    Return To        Return Stack    Frame Type
   0xFFFFFFA0    Handler mode     Main            Extended
   0xFFFFFFA8    Thread mode      Main            Extended
   0xFFFFFFAC    Thread mode      Process         Extended
   0xFFFFFFB0    Handler mode     Main            Standard
   0xFFFFFFB8    Thread mode      Main            Standard
   0xFFFFFFBC    Thread mode      Process         Standard  */

static int
arm_m_addr_is_magic (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  if (arm_m_addr_is_lockup (addr))
    return 1;

  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);
  if (tdep->have_sec_ext)
    {
      switch ((addr & 0xff000000))
	{
	case 0xff000000: /* EXC_RETURN pattern.  */
	case 0xfe000000: /* FNC_RETURN pattern.  */
	  return 1;
	default:
	  return 0;
	}
    }
  else
    {
      switch (addr)
	{
	  /* Values from ARMv8-M Architecture Technical Reference.  */
	case 0xffffffa0:
	case 0xffffffa8:
	case 0xffffffac:
	case 0xffffffb0:
	case 0xffffffb8:
	case 0xffffffbc:
	  /* Values from Tables in B1.5.8 the EXC_RETURN definitions of
	     the exception return behavior.  */
	case 0xffffffe1:
	case 0xffffffe9:
	case 0xffffffed:
	case 0xfffffff1:
	case 0xfffffff9:
	case 0xfffffffd:
	  /* Address is magic.  */
	  return 1;

	default:
	  /* Address is not magic.  */
	  return 0;
	}
    }
}

/* Remove useless bits from addresses in a running program.  */
static CORE_ADDR
arm_addr_bits_remove (struct gdbarch *gdbarch, CORE_ADDR val)
{
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  /* On M-profile devices, do not strip the low bit from EXC_RETURN
     (the magic exception return address).  */
  if (tdep->is_m && arm_m_addr_is_magic (gdbarch, val))
    return val;

  if (arm_apcs_32)
    return UNMAKE_THUMB_ADDR (val);
  else
    return (val & 0x03fffffc);
}

/* Return 1 if PC is the start of a compiler helper function which
   can be safely ignored during prologue skipping.  IS_THUMB is true
   if the function is known to be a Thumb function due to the way it
   is being called.  */
static int
skip_prologue_function (struct gdbarch *gdbarch, CORE_ADDR pc, int is_thumb)
{
  enum bfd_endian byte_order_for_code = gdbarch_byte_order_for_code (gdbarch);
  struct bound_minimal_symbol msym;

  msym = lookup_minimal_symbol_by_pc (pc);
  if (msym.minsym != NULL
      && msym.value_address () == pc
      && msym.minsym->linkage_name () != NULL)
    {
      const char *name = msym.minsym->linkage_name ();

      /* The GNU linker's Thumb call stub to foo is named
	 __foo_from_thumb.  */
      if (strstr (name, "_from_thumb") != NULL)
	name += 2;

      /* On soft-float targets, __truncdfsf2 is called to convert promoted
	 arguments to their argument types in non-prototyped
	 functions.  */
      if (startswith (name, "__truncdfsf2"))
	return 1;
      if (startswith (name, "__aeabi_d2f"))
	return 1;

      /* Internal functions related to thread-local storage.  */
      if (startswith (name, "__tls_get_addr"))
	return 1;
      if (startswith (name, "__aeabi_read_tp"))
	return 1;
    }
  else
    {
      /* If we run against a stripped glibc, we may be unable to identify
	 special functions by name.  Check for one important case,
	 __aeabi_read_tp, by comparing the *code* against the default
	 implementation (this is hand-written ARM assembler in glibc).  */

      if (!is_thumb
	  && read_code_unsigned_integer (pc, 4, byte_order_for_code)
	     == 0xe3e00a0f /* mov r0, #0xffff0fff */
	  && read_code_unsigned_integer (pc + 4, 4, byte_order_for_code)
	     == 0xe240f01f) /* sub pc, r0, #31 */
	return 1;
    }

  return 0;
}

/* Extract the immediate from instruction movw/movt of encoding T.  INSN1 is
   the first 16-bit of instruction, and INSN2 is the second 16-bit of
   instruction.  */
#define EXTRACT_MOVW_MOVT_IMM_T(insn1, insn2) \
  ((bits ((insn1), 0, 3) << 12)               \
   | (bits ((insn1), 10, 10) << 11)           \
   | (bits ((insn2), 12, 14) << 8)            \
   | bits ((insn2), 0, 7))

/* Extract the immediate from instruction movw/movt of encoding A.  INSN is
   the 32-bit instruction.  */
#define EXTRACT_MOVW_MOVT_IMM_A(insn) \
  ((bits ((insn), 16, 19) << 12) \
   | bits ((insn), 0, 11))

/* Decode immediate value; implements ThumbExpandImmediate pseudo-op.  */

static unsigned int
thumb_expand_immediate (unsigned int imm)
{
  unsigned int count = imm >> 7;

  if (count < 8)
    switch (count / 2)
      {
      case 0:
	return imm & 0xff;
      case 1:
	return (imm & 0xff) | ((imm & 0xff) << 16);
      case 2:
	return ((imm & 0xff) << 8) | ((imm & 0xff) << 24);
      case 3:
	return (imm & 0xff) | ((imm & 0xff) << 8)
		| ((imm & 0xff) << 16) | ((imm & 0xff) << 24);
      }

  return (0x80 | (imm & 0x7f)) << (32 - count);
}

/* Return 1 if the 16-bit Thumb instruction INSN restores SP in
   epilogue, 0 otherwise.  */

static int
thumb_instruction_restores_sp (unsigned short insn)
{
  return (insn == 0x46bd  /* mov sp, r7 */
	  || (insn & 0xff80) == 0xb000  /* add sp, imm */
	  || (insn & 0xfe00) == 0xbc00);  /* pop <registers> */
}

/* Analyze a Thumb prologue, looking for a recognizable stack frame
   and frame pointer.  Scan until we encounter a store that could
   clobber the stack frame unexpectedly, or an unknown instruction.
   Return the last address which is definitely safe to skip for an
   initial breakpoint.  */

static CORE_ADDR
thumb_analyze_prologue (struct gdbarch *gdbarch,
			CORE_ADDR start, CORE_ADDR limit,
			struct arm_prologue_cache *cache)
{
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  enum bfd_endian byte_order_for_code = gdbarch_byte_order_for_code (gdbarch);
  int i;
  pv_t regs[16];
  CORE_ADDR offset;
  CORE_ADDR unrecognized_pc = 0;

  for (i = 0; i < 16; i++)
    regs[i] = pv_register (i, 0);
  pv_area stack (ARM_SP_REGNUM, gdbarch_addr_bit (gdbarch));

  while (start < limit)
    {
      unsigned short insn;
      std::optional<bool> ra_signed_state;

      insn = read_code_unsigned_integer (start, 2, byte_order_for_code);

      if ((insn & 0xfe00) == 0xb400)		/* push { rlist } */
	{
	  int regno;
	  int mask;

	  if (stack.store_would_trash (regs[ARM_SP_REGNUM]))
	    break;

	  /* Bits 0-7 contain a mask for registers R0-R7.  Bit 8 says
	     whether to save LR (R14).  */
	  mask = (insn & 0xff) | ((insn & 0x100) << 6);

	  /* Calculate offsets of saved R0-R7 and LR.  */
	  for (regno = ARM_LR_REGNUM; regno >= 0; regno--)
	    if (mask & (1 << regno))
	      {
		regs[ARM_SP_REGNUM] = pv_add_constant (regs[ARM_SP_REGNUM],
						       -4);
		stack.store (regs[ARM_SP_REGNUM], 4, regs[regno]);
	      }
	}
      else if ((insn & 0xff80) == 0xb080)	/* sub sp, #imm */
	{
	  offset = (insn & 0x7f) << 2;		/* get scaled offset */
	  regs[ARM_SP_REGNUM] = pv_add_constant (regs[ARM_SP_REGNUM],
						 -offset);
	}
      else if (thumb_instruction_restores_sp (insn))
	{
	  /* Don't scan past the epilogue.  */
	  break;
	}
      else if ((insn & 0xf800) == 0xa800)	/* add Rd, sp, #imm */
	regs[bits (insn, 8, 10)] = pv_add_constant (regs[ARM_SP_REGNUM],
						    (insn & 0xff) << 2);
      else if ((insn & 0xfe00) == 0x1c00	/* add Rd, Rn, #imm */
	       && pv_is_register (regs[bits (insn, 3, 5)], ARM_SP_REGNUM))
	regs[bits (insn, 0, 2)] = pv_add_constant (regs[bits (insn, 3, 5)],
						   bits (insn, 6, 8));
      else if ((insn & 0xf800) == 0x3000	/* add Rd, #imm */
	       && pv_is_register (regs[bits (insn, 8, 10)], ARM_SP_REGNUM))
	regs[bits (insn, 8, 10)] = pv_add_constant (regs[bits (insn, 8, 10)],
						    bits (insn, 0, 7));
      else if ((insn & 0xfe00) == 0x1800	/* add Rd, Rn, Rm */
	       && pv_is_register (regs[bits (insn, 6, 8)], ARM_SP_REGNUM)
	       && pv_is_constant (regs[bits (insn, 3, 5)]))
	regs[bits (insn, 0, 2)] = pv_add (regs[bits (insn, 3, 5)],
					  regs[bits (insn, 6, 8)]);
      else if ((insn & 0xff00) == 0x4400	/* add Rd, Rm */
	       && pv_is_constant (regs[bits (insn, 3, 6)]))
	{
	  int rd = (bit (insn, 7) << 3) + bits (insn, 0, 2);
	  int rm = bits (insn, 3, 6);
	  regs[rd] = pv_add (regs[rd], regs[rm]);
	}
      else if ((insn & 0xff00) == 0x4600)	/* mov hi, lo or mov lo, hi */
	{
	  int dst_reg = (insn & 0x7) + ((insn & 0x80) >> 4);
	  int src_reg = (insn & 0x78) >> 3;
	  regs[dst_reg] = regs[src_reg];
	}
      else if ((insn & 0xf800) == 0x9000)	/* str rd, [sp, #off] */
	{
	  /* Handle stores to the stack.  Normally pushes are used,
	     but with GCC -mtpcs-frame, there may be other stores
	     in the prologue to create the frame.  */
	  int regno = (insn >> 8) & 0x7;
	  pv_t addr;

	  offset = (insn & 0xff) << 2;
	  addr = pv_add_constant (regs[ARM_SP_REGNUM], offset);

	  if (stack.store_would_trash (addr))
	    break;

	  stack.store (addr, 4, regs[regno]);
	}
      else if ((insn & 0xf800) == 0x6000)	/* str rd, [rn, #off] */
	{
	  int rd = bits (insn, 0, 2);
	  int rn = bits (insn, 3, 5);
	  pv_t addr;

	  offset = bits (insn, 6, 10) << 2;
	  addr = pv_add_constant (regs[rn], offset);

	  if (stack.store_would_trash (addr))
	    break;

	  stack.store (addr, 4, regs[rd]);
	}
      else if (((insn & 0xf800) == 0x7000	/* strb Rd, [Rn, #off] */
		|| (insn & 0xf800) == 0x8000)	/* strh Rd, [Rn, #off] */
	       && pv_is_register (regs[bits (insn, 3, 5)], ARM_SP_REGNUM))
	/* Ignore stores of argument registers to the stack.  */
	;
      else if ((insn & 0xf800) == 0xc800	/* ldmia Rn!, { registers } */
	       && pv_is_register (regs[bits (insn, 8, 10)], ARM_SP_REGNUM))
	/* Ignore block loads from the stack, potentially copying
	   parameters from memory.  */
	;
      else if ((insn & 0xf800) == 0x9800	/* ldr Rd, [Rn, #immed] */
	       || ((insn & 0xf800) == 0x6800	/* ldr Rd, [sp, #immed] */
		   && pv_is_register (regs[bits (insn, 3, 5)], ARM_SP_REGNUM)))
	/* Similarly ignore single loads from the stack.  */
	;
      else if ((insn & 0xffc0) == 0x0000	/* lsls Rd, Rm, #0 */
	       || (insn & 0xffc0) == 0x1c00)	/* add Rd, Rn, #0 */
	/* Skip register copies, i.e. saves to another register
	   instead of the stack.  */
	;
      else if ((insn & 0xf800) == 0x2000)	/* movs Rd, #imm */
	/* Recognize constant loads; even with small stacks these are necessary
	   on Thumb.  */
	regs[bits (insn, 8, 10)] = pv_constant (bits (insn, 0, 7));
      else if ((insn & 0xf800) == 0x4800)	/* ldr Rd, [pc, #imm] */
	{
	  /* Constant pool loads, for the same reason.  */
	  unsigned int constant;
	  CORE_ADDR loc;

	  loc = start + 4 + bits (insn, 0, 7) * 4;
	  constant = read_memory_unsigned_integer (loc, 4, byte_order);
	  regs[bits (insn, 8, 10)] = pv_constant (constant);
	}
      else if (thumb_insn_size (insn) == 4) /* 32-bit Thumb-2 instructions.  */
	{
	  unsigned short inst2;

	  inst2 = read_code_unsigned_integer (start + 2, 2,
					      byte_order_for_code);
	  uint32_t whole_insn = (insn << 16) | inst2;

	  if ((insn & 0xf800) == 0xf000 && (inst2 & 0xe800) == 0xe800)
	    {
	      /* BL, BLX.  Allow some special function calls when
		 skipping the prologue; GCC generates these before
		 storing arguments to the stack.  */
	      CORE_ADDR nextpc;
	      int j1, j2, imm1, imm2;

	      imm1 = sbits (insn, 0, 10);
	      imm2 = bits (inst2, 0, 10);
	      j1 = bit (inst2, 13);
	      j2 = bit (inst2, 11);

	      offset = ((imm1 << 12) + (imm2 << 1));
	      offset ^= ((!j2) << 22) | ((!j1) << 23);

	      nextpc = start + 4 + offset;
	      /* For BLX make sure to clear the low bits.  */
	      if (bit (inst2, 12) == 0)
		nextpc = nextpc & 0xfffffffc;

	      if (!skip_prologue_function (gdbarch, nextpc,
					   bit (inst2, 12) != 0))
		break;
	    }

	  else if ((insn & 0xffd0) == 0xe900    /* stmdb Rn{!},
						   { registers } */
		   && pv_is_register (regs[bits (insn, 0, 3)], ARM_SP_REGNUM))
	    {
	      pv_t addr = regs[bits (insn, 0, 3)];
	      int regno;

	      if (stack.store_would_trash (addr))
		break;

	      /* Calculate offsets of saved registers.  */
	      for (regno = ARM_LR_REGNUM; regno >= 0; regno--)
		if (inst2 & (1 << regno))
		  {
		    addr = pv_add_constant (addr, -4);
		    stack.store (addr, 4, regs[regno]);
		  }

	      if (insn & 0x0020)
		regs[bits (insn, 0, 3)] = addr;
	    }

	  /* vstmdb Rn{!}, { D-registers } (aka vpush).  */
	  else if ((insn & 0xff20) == 0xed20
		   && (inst2 & 0x0f00) == 0x0b00
		   && pv_is_register (regs[bits (insn, 0, 3)], ARM_SP_REGNUM))
	    {
	      /* Address SP points to.  */
	      pv_t addr = regs[bits (insn, 0, 3)];

	      /* Number of registers saved.  */
	      unsigned int number = bits (inst2, 0, 7) >> 1;

	      /* First register to save.  */
	      int vd = bits (inst2, 12, 15) | (bits (insn, 6, 6) << 4);

	      if (stack.store_would_trash (addr))
		break;

	      /* Calculate offsets of saved registers.  */
	      for (; number > 0; number--)
		{
		  addr = pv_add_constant (addr, -8);
		  stack.store (addr, 8, pv_register (ARM_D0_REGNUM
						     + vd + number, 0));
		}

	      /* Writeback SP to account for the saved registers.  */
	      regs[bits (insn, 0, 3)] = addr;
	    }

	  else if ((insn & 0xff50) == 0xe940	/* strd Rt, Rt2,
						   [Rn, #+/-imm]{!} */
		   && pv_is_register (regs[bits (insn, 0, 3)], ARM_SP_REGNUM))
	    {
	      int regno1 = bits (inst2, 12, 15);
	      int regno2 = bits (inst2, 8, 11);
	      pv_t addr = regs[bits (insn, 0, 3)];

	      offset = inst2 & 0xff;
	      if (insn & 0x0080)
		addr = pv_add_constant (addr, offset);
	      else
		addr = pv_add_constant (addr, -offset);

	      if (stack.store_would_trash (addr))
		break;

	      stack.store (addr, 4, regs[regno1]);
	      stack.store (pv_add_constant (addr, 4),
			   4, regs[regno2]);

	      if (insn & 0x0020)
		regs[bits (insn, 0, 3)] = addr;
	    }

	  else if ((insn & 0xfff0) == 0xf8c0	/* str Rt,[Rn,+/-#imm]{!} */
		   && (inst2 & 0x0c00) == 0x0c00
		   && pv_is_register (regs[bits (insn, 0, 3)], ARM_SP_REGNUM))
	    {
	      int regno = bits (inst2, 12, 15);
	      pv_t addr = regs[bits (insn, 0, 3)];

	      offset = inst2 & 0xff;
	      if (inst2 & 0x0200)
		addr = pv_add_constant (addr, offset);
	      else
		addr = pv_add_constant (addr, -offset);

	      if (stack.store_would_trash (addr))
		break;

	      stack.store (addr, 4, regs[regno]);

	      if (inst2 & 0x0100)
		regs[bits (insn, 0, 3)] = addr;
	    }

	  else if ((insn & 0xfff0) == 0xf8c0	/* str.w Rt,[Rn,#imm] */
		   && pv_is_register (regs[bits (insn, 0, 3)], ARM_SP_REGNUM))
	    {
	      int regno = bits (inst2, 12, 15);
	      pv_t addr;

	      offset = inst2 & 0xfff;
	      addr = pv_add_constant (regs[bits (insn, 0, 3)], offset);

	      if (stack.store_would_trash (addr))
		break;

	      stack.store (addr, 4, regs[regno]);
	    }

	  else if ((insn & 0xffd0) == 0xf880	/* str{bh}.w Rt,[Rn,#imm] */
		   && pv_is_register (regs[bits (insn, 0, 3)], ARM_SP_REGNUM))
	    /* Ignore stores of argument registers to the stack.  */
	    ;

	  else if ((insn & 0xffd0) == 0xf800	/* str{bh} Rt,[Rn,#+/-imm] */
		   && (inst2 & 0x0d00) == 0x0c00
		   && pv_is_register (regs[bits (insn, 0, 3)], ARM_SP_REGNUM))
	    /* Ignore stores of argument registers to the stack.  */
	    ;

	  else if ((insn & 0xffd0) == 0xe890	/* ldmia Rn[!],
						   { registers } */
		   && (inst2 & 0x8000) == 0x0000
		   && pv_is_register (regs[bits (insn, 0, 3)], ARM_SP_REGNUM))
	    /* Ignore block loads from the stack, potentially copying
	       parameters from memory.  */
	    ;

	  else if ((insn & 0xff70) == 0xe950    /* ldrd Rt, Rt2,
						   [Rn, #+/-imm] */
		   && pv_is_register (regs[bits (insn, 0, 3)], ARM_SP_REGNUM))
	    /* Similarly ignore dual loads from the stack.  */
	    ;

	  else if ((insn & 0xfff0) == 0xf850	/* ldr Rt,[Rn,#+/-imm] */
		   && (inst2 & 0x0d00) == 0x0c00
		   && pv_is_register (regs[bits (insn, 0, 3)], ARM_SP_REGNUM))
	    /* Similarly ignore single loads from the stack.  */
	    ;

	  else if ((insn & 0xfff0) == 0xf8d0	/* ldr.w Rt,[Rn,#imm] */
		   && pv_is_register (regs[bits (insn, 0, 3)], ARM_SP_REGNUM))
	    /* Similarly ignore single loads from the stack.  */
	    ;

	  else if ((insn & 0xfbf0) == 0xf100	/* add.w Rd, Rn, #imm */
		   && (inst2 & 0x8000) == 0x0000)
	    {
	      unsigned int imm = ((bits (insn, 10, 10) << 11)
				  | (bits (inst2, 12, 14) << 8)
				  | bits (inst2, 0, 7));

	      regs[bits (inst2, 8, 11)]
		= pv_add_constant (regs[bits (insn, 0, 3)],
				   thumb_expand_immediate (imm));
	    }

	  else if ((insn & 0xfbf0) == 0xf200	/* addw Rd, Rn, #imm */
		   && (inst2 & 0x8000) == 0x0000)
	    {
	      unsigned int imm = ((bits (insn, 10, 10) << 11)
				  | (bits (inst2, 12, 14) << 8)
				  | bits (inst2, 0, 7));

	      regs[bits (inst2, 8, 11)]
		= pv_add_constant (regs[bits (insn, 0, 3)], imm);
	    }

	  else if ((insn & 0xfbf0) == 0xf1a0	/* sub.w Rd, Rn, #imm */
		   && (inst2 & 0x8000) == 0x0000)
	    {
	      unsigned int imm = ((bits (insn, 10, 10) << 11)
				  | (bits (inst2, 12, 14) << 8)
				  | bits (inst2, 0, 7));

	      regs[bits (inst2, 8, 11)]
		= pv_add_constant (regs[bits (insn, 0, 3)],
				   - (CORE_ADDR) thumb_expand_immediate (imm));
	    }

	  else if ((insn & 0xfbf0) == 0xf2a0	/* subw Rd, Rn, #imm */
		   && (inst2 & 0x8000) == 0x0000)
	    {
	      unsigned int imm = ((bits (insn, 10, 10) << 11)
				  | (bits (inst2, 12, 14) << 8)
				  | bits (inst2, 0, 7));

	      regs[bits (inst2, 8, 11)]
		= pv_add_constant (regs[bits (insn, 0, 3)], - (CORE_ADDR) imm);
	    }

	  else if ((insn & 0xfbff) == 0xf04f)	/* mov.w Rd, #const */
	    {
	      unsigned int imm = ((bits (insn, 10, 10) << 11)
				  | (bits (inst2, 12, 14) << 8)
				  | bits (inst2, 0, 7));

	      regs[bits (inst2, 8, 11)]
		= pv_constant (thumb_expand_immediate (imm));
	    }

	  else if ((insn & 0xfbf0) == 0xf240)	/* movw Rd, #const */
	    {
	      unsigned int imm
		= EXTRACT_MOVW_MOVT_IMM_T (insn, inst2);

	      regs[bits (inst2, 8, 11)] = pv_constant (imm);
	    }

	  else if (insn == 0xea5f		/* mov.w Rd,Rm */
		   && (inst2 & 0xf0f0) == 0)
	    {
	      int dst_reg = (inst2 & 0x0f00) >> 8;
	      int src_reg = inst2 & 0xf;
	      regs[dst_reg] = regs[src_reg];
	    }

	  else if ((insn & 0xff7f) == 0xf85f)	/* ldr.w Rt,<label> */
	    {
	      /* Constant pool loads.  */
	      unsigned int constant;
	      CORE_ADDR loc;

	      offset = bits (inst2, 0, 11);
	      if (insn & 0x0080)
		loc = start + 4 + offset;
	      else
		loc = start + 4 - offset;

	      constant = read_memory_unsigned_integer (loc, 4, byte_order);
	      regs[bits (inst2, 12, 15)] = pv_constant (constant);
	    }

	  else if ((insn & 0xff7f) == 0xe95f)	/* ldrd Rt,Rt2,<label> */
	    {
	      /* Constant pool loads.  */
	      unsigned int constant;
	      CORE_ADDR loc;

	      offset = bits (inst2, 0, 7) << 2;
	      if (insn & 0x0080)
		loc = start + 4 + offset;
	      else
		loc = start + 4 - offset;

	      constant = read_memory_unsigned_integer (loc, 4, byte_order);
	      regs[bits (inst2, 12, 15)] = pv_constant (constant);

	      constant = read_memory_unsigned_integer (loc + 4, 4, byte_order);
	      regs[bits (inst2, 8, 11)] = pv_constant (constant);
	    }
	  /* Start of ARMv8.1-m PACBTI extension instructions.  */
	  else if (IS_PAC (whole_insn))
	    {
	      /* LR and SP are input registers.  PAC is in R12.  LR is
		 signed from this point onwards.  NOP space.  */
	      ra_signed_state = true;
	    }
	  else if (IS_PACBTI (whole_insn))
	    {
	      /* LR and SP are input registers.  PAC is in R12 and PC is a
		 valid BTI landing pad.  LR is signed from this point onwards.
		 NOP space.  */
	      ra_signed_state = true;
	    }
	  else if (IS_BTI (whole_insn))
	    {
	      /* Valid BTI landing pad.  NOP space.  */
	    }
	  else if (IS_PACG (whole_insn))
	    {
	      /* Sign Rn using Rm and store the PAC in Rd.  Rd is signed from
		 this point onwards.  */
	      ra_signed_state = true;
	    }
	  else if (IS_AUT (whole_insn) || IS_AUTG (whole_insn))
	    {
	      /* These instructions appear close to the epilogue, when signed
		 pointers are getting authenticated.  */
	      ra_signed_state = false;
	    }
	  /* End of ARMv8.1-m PACBTI extension instructions */
	  else if (thumb2_instruction_changes_pc (insn, inst2))
	    {
	      /* Don't scan past anything that might change control flow.  */
	      break;
	    }
	  else
	    {
	      /* The optimizer might shove anything into the prologue,
		 so we just skip what we don't recognize.  */
	      unrecognized_pc = start;
	    }

	  /* Make sure we are dealing with a target that supports ARMv8.1-m
	     PACBTI.  */
	  if (cache != nullptr && tdep->have_pacbti
	      && ra_signed_state.has_value ())
	    {
	      arm_debug_printf ("Found pacbti instruction at %s",
				paddress (gdbarch, start));
	      arm_debug_printf ("RA is %s",
				*ra_signed_state ? "signed" : "not signed");
	      cache->ra_signed_state = ra_signed_state;
	    }

	  start += 2;
	}
      else if (thumb_instruction_changes_pc (insn))
	{
	  /* Don't scan past anything that might change control flow.  */
	  break;
	}
      else
	{
	  /* The optimizer might shove anything into the prologue,
	     so we just skip what we don't recognize.  */
	  unrecognized_pc = start;
	}

      start += 2;
    }

  arm_debug_printf ("Prologue scan stopped at %s",
		    paddress (gdbarch, start));

  if (unrecognized_pc == 0)
    unrecognized_pc = start;

  if (cache == NULL)
    return unrecognized_pc;

  if (pv_is_register (regs[ARM_FP_REGNUM], ARM_SP_REGNUM))
    {
      /* Frame pointer is fp.  Frame size is constant.  */
      cache->framereg = ARM_FP_REGNUM;
      cache->framesize = -regs[ARM_FP_REGNUM].k;
    }
  else if (pv_is_register (regs[THUMB_FP_REGNUM], ARM_SP_REGNUM))
    {
      /* Frame pointer is r7.  Frame size is constant.  */
      cache->framereg = THUMB_FP_REGNUM;
      cache->framesize = -regs[THUMB_FP_REGNUM].k;
    }
  else
    {
      /* Try the stack pointer... this is a bit desperate.  */
      cache->framereg = ARM_SP_REGNUM;
      cache->framesize = -regs[ARM_SP_REGNUM].k;
    }

  for (i = 0; i < gdbarch_num_regs (gdbarch); i++)
    if (stack.find_reg (gdbarch, i, &offset))
      {
	cache->saved_regs[i].set_addr (offset);
	if (i == ARM_SP_REGNUM)
	  arm_cache_set_active_sp_value(cache, tdep, offset);
      }

  return unrecognized_pc;
}


/* Try to analyze the instructions starting from PC, which load symbol
   __stack_chk_guard.  Return the address of instruction after loading this
   symbol, set the dest register number to *BASEREG, and set the size of
   instructions for loading symbol in OFFSET.  Return 0 if instructions are
   not recognized.  */

static CORE_ADDR
arm_analyze_load_stack_chk_guard(CORE_ADDR pc, struct gdbarch *gdbarch,
				 unsigned int *destreg, int *offset)
{
  enum bfd_endian byte_order_for_code = gdbarch_byte_order_for_code (gdbarch);
  int is_thumb = arm_pc_is_thumb (gdbarch, pc);
  unsigned int low, high, address;

  address = 0;
  if (is_thumb)
    {
      unsigned short insn1
	= read_code_unsigned_integer (pc, 2, byte_order_for_code);

      if ((insn1 & 0xf800) == 0x4800) /* ldr Rd, #immed */
	{
	  *destreg = bits (insn1, 8, 10);
	  *offset = 2;
	  address = (pc & 0xfffffffc) + 4 + (bits (insn1, 0, 7) << 2);
	  address = read_memory_unsigned_integer (address, 4,
						  byte_order_for_code);
	}
      else if ((insn1 & 0xfbf0) == 0xf240) /* movw Rd, #const */
	{
	  unsigned short insn2
	    = read_code_unsigned_integer (pc + 2, 2, byte_order_for_code);

	  low = EXTRACT_MOVW_MOVT_IMM_T (insn1, insn2);

	  insn1
	    = read_code_unsigned_integer (pc + 4, 2, byte_order_for_code);
	  insn2
	    = read_code_unsigned_integer (pc + 6, 2, byte_order_for_code);

	  /* movt Rd, #const */
	  if ((insn1 & 0xfbc0) == 0xf2c0)
	    {
	      high = EXTRACT_MOVW_MOVT_IMM_T (insn1, insn2);
	      *destreg = bits (insn2, 8, 11);
	      *offset = 8;
	      address = (high << 16 | low);
	    }
	}
    }
  else
    {
      unsigned int insn
	= read_code_unsigned_integer (pc, 4, byte_order_for_code);

      if ((insn & 0x0e5f0000) == 0x041f0000) /* ldr Rd, [PC, #immed] */
	{
	  address = bits (insn, 0, 11) + pc + 8;
	  address = read_memory_unsigned_integer (address, 4,
						  byte_order_for_code);

	  *destreg = bits (insn, 12, 15);
	  *offset = 4;
	}
      else if ((insn & 0x0ff00000) == 0x03000000) /* movw Rd, #const */
	{
	  low = EXTRACT_MOVW_MOVT_IMM_A (insn);

	  insn
	    = read_code_unsigned_integer (pc + 4, 4, byte_order_for_code);

	  if ((insn & 0x0ff00000) == 0x03400000) /* movt Rd, #const */
	    {
	      high = EXTRACT_MOVW_MOVT_IMM_A (insn);
	      *destreg = bits (insn, 12, 15);
	      *offset = 8;
	      address = (high << 16 | low);
	    }
	}
    }

  return address;
}

/* Try to skip a sequence of instructions used for stack protector.  If PC
   points to the first instruction of this sequence, return the address of
   first instruction after this sequence, otherwise, return original PC.

   On arm, this sequence of instructions is composed of mainly three steps,
     Step 1: load symbol __stack_chk_guard,
     Step 2: load from address of __stack_chk_guard,
     Step 3: store it to somewhere else.

   Usually, instructions on step 2 and step 3 are the same on various ARM
   architectures.  On step 2, it is one instruction 'ldr Rx, [Rn, #0]', and
   on step 3, it is also one instruction 'str Rx, [r7, #immd]'.  However,
   instructions in step 1 vary from different ARM architectures.  On ARMv7,
   they are,

	movw	Rn, #:lower16:__stack_chk_guard
	movt	Rn, #:upper16:__stack_chk_guard

   On ARMv5t, it is,

	ldr	Rn, .Label
	....
	.Lable:
	.word	__stack_chk_guard

   Since ldr/str is a very popular instruction, we can't use them as
   'fingerprint' or 'signature' of stack protector sequence.  Here we choose
   sequence {movw/movt, ldr}/ldr/str plus symbol __stack_chk_guard, if not
   stripped, as the 'fingerprint' of a stack protector cdoe sequence.  */

static CORE_ADDR
arm_skip_stack_protector(CORE_ADDR pc, struct gdbarch *gdbarch)
{
  enum bfd_endian byte_order_for_code = gdbarch_byte_order_for_code (gdbarch);
  unsigned int basereg;
  struct bound_minimal_symbol stack_chk_guard;
  int offset;
  int is_thumb = arm_pc_is_thumb (gdbarch, pc);
  CORE_ADDR addr;

  /* Try to parse the instructions in Step 1.  */
  addr = arm_analyze_load_stack_chk_guard (pc, gdbarch,
					   &basereg, &offset);
  if (!addr)
    return pc;

  stack_chk_guard = lookup_minimal_symbol_by_pc (addr);
  /* ADDR must correspond to a symbol whose name is __stack_chk_guard.
     Otherwise, this sequence cannot be for stack protector.  */
  if (stack_chk_guard.minsym == NULL
      || !startswith (stack_chk_guard.minsym->linkage_name (), "__stack_chk_guard"))
   return pc;

  if (is_thumb)
    {
      unsigned int destreg;
      unsigned short insn
	= read_code_unsigned_integer (pc + offset, 2, byte_order_for_code);

      /* Step 2: ldr Rd, [Rn, #immed], encoding T1.  */
      if ((insn & 0xf800) != 0x6800)
	return pc;
      if (bits (insn, 3, 5) != basereg)
	return pc;
      destreg = bits (insn, 0, 2);

      insn = read_code_unsigned_integer (pc + offset + 2, 2,
					 byte_order_for_code);
      /* Step 3: str Rd, [Rn, #immed], encoding T1.  */
      if ((insn & 0xf800) != 0x6000)
	return pc;
      if (destreg != bits (insn, 0, 2))
	return pc;
    }
  else
    {
      unsigned int destreg;
      unsigned int insn
	= read_code_unsigned_integer (pc + offset, 4, byte_order_for_code);

      /* Step 2: ldr Rd, [Rn, #immed], encoding A1.  */
      if ((insn & 0x0e500000) != 0x04100000)
	return pc;
      if (bits (insn, 16, 19) != basereg)
	return pc;
      destreg = bits (insn, 12, 15);
      /* Step 3: str Rd, [Rn, #immed], encoding A1.  */
      insn = read_code_unsigned_integer (pc + offset + 4,
					   4, byte_order_for_code);
      if ((insn & 0x0e500000) != 0x04000000)
	return pc;
      if (bits (insn, 12, 15) != destreg)
	return pc;
    }
  /* The size of total two instructions ldr/str is 4 on Thumb-2, while 8
     on arm.  */
  if (is_thumb)
    return pc + offset + 4;
  else
    return pc + offset + 8;
}

/* Advance the PC across any function entry prologue instructions to
   reach some "real" code.

   The APCS (ARM Procedure Call Standard) defines the following
   prologue:

   mov          ip, sp
   [stmfd       sp!, {a1,a2,a3,a4}]
   stmfd        sp!, {...,fp,ip,lr,pc}
   [stfe        f7, [sp, #-12]!]
   [stfe        f6, [sp, #-12]!]
   [stfe        f5, [sp, #-12]!]
   [stfe        f4, [sp, #-12]!]
   sub fp, ip, #nn @@ nn == 20 or 4 depending on second insn.  */

static CORE_ADDR
arm_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  CORE_ADDR func_addr, func_end_addr, limit_pc;

  /* See if we can determine the end of the prologue via the symbol table.
     If so, then return either PC, or the PC after the prologue, whichever
     is greater.  */
  bool func_addr_found
    = find_pc_partial_function (pc, NULL, &func_addr, &func_end_addr);

  /* Whether the function is thumb mode or not.  */
  bool func_is_thumb = false;

  if (func_addr_found)
    {
      CORE_ADDR post_prologue_pc
	= skip_prologue_using_sal (gdbarch, func_addr);
      struct compunit_symtab *cust = find_pc_compunit_symtab (func_addr);

      if (post_prologue_pc)
	post_prologue_pc
	  = arm_skip_stack_protector (post_prologue_pc, gdbarch);


      /* GCC always emits a line note before the prologue and another
	 one after, even if the two are at the same address or on the
	 same line.  Take advantage of this so that we do not need to
	 know every instruction that might appear in the prologue.  We
	 will have producer information for most binaries; if it is
	 missing (e.g. for -gstabs), assuming the GNU tools.  */
      if (post_prologue_pc
	  && (cust == NULL
	      || cust->producer () == NULL
	      || startswith (cust->producer (), "GNU ")
	      || producer_is_llvm (cust->producer ())))
	return post_prologue_pc;

      if (post_prologue_pc != 0)
	{
	  CORE_ADDR analyzed_limit;

	  /* For non-GCC compilers, make sure the entire line is an
	     acceptable prologue; GDB will round this function's
	     return value up to the end of the following line so we
	     can not skip just part of a line (and we do not want to).

	     RealView does not treat the prologue specially, but does
	     associate prologue code with the opening brace; so this
	     lets us skip the first line if we think it is the opening
	     brace.  */
	  func_is_thumb = arm_pc_is_thumb (gdbarch, func_addr);
	  if (func_is_thumb)
	    analyzed_limit = thumb_analyze_prologue (gdbarch, func_addr,
						     post_prologue_pc, NULL);
	  else
	    analyzed_limit
	      = arm_analyze_prologue (gdbarch, func_addr, post_prologue_pc,
				      NULL, target_arm_instruction_reader ());

	  if (analyzed_limit != post_prologue_pc)
	    return func_addr;

	  return post_prologue_pc;
	}
    }

  /* Can't determine prologue from the symbol table, need to examine
     instructions.  */

  /* Find an upper limit on the function prologue using the debug
     information.  If the debug information could not be used to provide
     that bound, then use an arbitrary large number as the upper bound.  */
  /* Like arm_scan_prologue, stop no later than pc + 64.  */
  limit_pc = skip_prologue_using_sal (gdbarch, pc);
  if (limit_pc == 0)
    limit_pc = pc + 64;          /* Magic.  */

  /* Set the correct adjustment based on whether the function is thumb mode or
     not.  We use it to get the address of the last instruction in the
     function (as opposed to the first address of the next function).  */
  CORE_ADDR adjustment = func_is_thumb ? 2 : 4;

  limit_pc
    = func_end_addr == 0 ? limit_pc : std::min (limit_pc,
						func_end_addr - adjustment);

  /* Check if this is Thumb code.  */
  if (arm_pc_is_thumb (gdbarch, pc))
    return thumb_analyze_prologue (gdbarch, pc, limit_pc, NULL);
  else
    return arm_analyze_prologue (gdbarch, pc, limit_pc, NULL,
				 target_arm_instruction_reader ());
}

/* Function: thumb_scan_prologue (helper function for arm_scan_prologue)
   This function decodes a Thumb function prologue to determine:
     1) the size of the stack frame
     2) which registers are saved on it
     3) the offsets of saved regs
     4) the offset from the stack pointer to the frame pointer

   A typical Thumb function prologue would create this stack frame
   (offsets relative to FP)
     old SP ->	24  stack parameters
		20  LR
		16  R7
     R7 ->       0  local variables (16 bytes)
     SP ->     -12  additional stack space (12 bytes)
   The frame size would thus be 36 bytes, and the frame offset would be
   12 bytes.  The frame register is R7.
   
   The comments for thumb_skip_prolog() describe the algorithm we use
   to detect the end of the prolog.  */

static void
thumb_scan_prologue (struct gdbarch *gdbarch, CORE_ADDR prev_pc,
		     CORE_ADDR block_addr, struct arm_prologue_cache *cache)
{
  CORE_ADDR prologue_start;
  CORE_ADDR prologue_end;

  if (find_pc_partial_function (block_addr, NULL, &prologue_start,
				&prologue_end))
    {
      /* See comment in arm_scan_prologue for an explanation of
	 this heuristics.  */
      if (prologue_end > prologue_start + 64)
	{
	  prologue_end = prologue_start + 64;
	}
    }
  else
    /* We're in the boondocks: we have no idea where the start of the
       function is.  */
    return;

  prologue_end = std::min (prologue_end, prev_pc);

  thumb_analyze_prologue (gdbarch, prologue_start, prologue_end, cache);
}

/* Return 1 if the ARM instruction INSN restores SP in epilogue, 0
   otherwise.  */

static int
arm_instruction_restores_sp (unsigned int insn)
{
  if (bits (insn, 28, 31) != INST_NV)
    {
      if ((insn & 0x0df0f000) == 0x0080d000
	  /* ADD SP (register or immediate).  */
	  || (insn & 0x0df0f000) == 0x0040d000
	  /* SUB SP (register or immediate).  */
	  || (insn & 0x0ffffff0) == 0x01a0d000
	  /* MOV SP.  */
	  || (insn & 0x0fff0000) == 0x08bd0000
	  /* POP (LDMIA).  */
	  || (insn & 0x0fff0000) == 0x049d0000)
	  /* POP of a single register.  */
	return 1;
    }

  return 0;
}

/* Implement immediate value decoding, as described in section A5.2.4
   (Modified immediate constants in ARM instructions) of the ARM Architecture
   Reference Manual (ARMv7-A and ARMv7-R edition).  */

static uint32_t
arm_expand_immediate (uint32_t imm)
{
  /* Immediate values are 12 bits long.  */
  gdb_assert ((imm & 0xfffff000) == 0);

  uint32_t unrotated_value = imm & 0xff;
  uint32_t rotate_amount = (imm & 0xf00) >> 7;

  if (rotate_amount == 0)
    return unrotated_value;

  return ((unrotated_value >> rotate_amount)
	  | (unrotated_value << (32 - rotate_amount)));
}

/* Analyze an ARM mode prologue starting at PROLOGUE_START and
   continuing no further than PROLOGUE_END.  If CACHE is non-NULL,
   fill it in.  Return the first address not recognized as a prologue
   instruction.

   We recognize all the instructions typically found in ARM prologues,
   plus harmless instructions which can be skipped (either for analysis
   purposes, or a more restrictive set that can be skipped when finding
   the end of the prologue).  */

static CORE_ADDR
arm_analyze_prologue (struct gdbarch *gdbarch,
		      CORE_ADDR prologue_start, CORE_ADDR prologue_end,
		      struct arm_prologue_cache *cache,
		      const arm_instruction_reader &insn_reader)
{
  enum bfd_endian byte_order_for_code = gdbarch_byte_order_for_code (gdbarch);
  int regno;
  CORE_ADDR offset, current_pc;
  pv_t regs[ARM_FPS_REGNUM];
  CORE_ADDR unrecognized_pc = 0;
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  /* Search the prologue looking for instructions that set up the
     frame pointer, adjust the stack pointer, and save registers.

     Be careful, however, and if it doesn't look like a prologue,
     don't try to scan it.  If, for instance, a frameless function
     begins with stmfd sp!, then we will tell ourselves there is
     a frame, which will confuse stack traceback, as well as "finish" 
     and other operations that rely on a knowledge of the stack
     traceback.  */

  for (regno = 0; regno < ARM_FPS_REGNUM; regno++)
    regs[regno] = pv_register (regno, 0);
  pv_area stack (ARM_SP_REGNUM, gdbarch_addr_bit (gdbarch));

  for (current_pc = prologue_start;
       current_pc < prologue_end;
       current_pc += 4)
    {
      uint32_t insn = insn_reader.read (current_pc, byte_order_for_code);

      if (insn == 0xe1a0c00d)		/* mov ip, sp */
	{
	  regs[ARM_IP_REGNUM] = regs[ARM_SP_REGNUM];
	  continue;
	}
      else if ((insn & 0xfff00000) == 0xe2800000	/* add Rd, Rn, #n */
	       && pv_is_register (regs[bits (insn, 16, 19)], ARM_SP_REGNUM))
	{
	  uint32_t imm = arm_expand_immediate (insn & 0xfff);
	  int rd = bits (insn, 12, 15);
	  regs[rd] = pv_add_constant (regs[bits (insn, 16, 19)], imm);
	  continue;
	}
      else if ((insn & 0xfff00000) == 0xe2400000	/* sub Rd, Rn, #n */
	       && pv_is_register (regs[bits (insn, 16, 19)], ARM_SP_REGNUM))
	{
	  uint32_t imm = arm_expand_immediate (insn & 0xfff);
	  int rd = bits (insn, 12, 15);
	  regs[rd] = pv_add_constant (regs[bits (insn, 16, 19)], -imm);
	  continue;
	}
      else if ((insn & 0xffff0fff) == 0xe52d0004)	/* str Rd,
							   [sp, #-4]! */
	{
	  if (stack.store_would_trash (regs[ARM_SP_REGNUM]))
	    break;
	  regs[ARM_SP_REGNUM] = pv_add_constant (regs[ARM_SP_REGNUM], -4);
	  stack.store (regs[ARM_SP_REGNUM], 4,
		       regs[bits (insn, 12, 15)]);
	  continue;
	}
      else if ((insn & 0xffff0000) == 0xe92d0000)
	/* stmfd sp!, {..., fp, ip, lr, pc}
	   or
	   stmfd sp!, {a1, a2, a3, a4}  */
	{
	  int mask = insn & 0xffff;

	  if (stack.store_would_trash (regs[ARM_SP_REGNUM]))
	    break;

	  /* Calculate offsets of saved registers.  */
	  for (regno = ARM_PC_REGNUM; regno >= 0; regno--)
	    if (mask & (1 << regno))
	      {
		regs[ARM_SP_REGNUM]
		  = pv_add_constant (regs[ARM_SP_REGNUM], -4);
		stack.store (regs[ARM_SP_REGNUM], 4, regs[regno]);
	      }
	}
      else if ((insn & 0xffff0000) == 0xe54b0000	/* strb rx,[r11,#-n] */
	       || (insn & 0xffff00f0) == 0xe14b00b0	/* strh rx,[r11,#-n] */
	       || (insn & 0xffffc000) == 0xe50b0000)	/* str  rx,[r11,#-n] */
	{
	  /* No need to add this to saved_regs -- it's just an arg reg.  */
	  continue;
	}
      else if ((insn & 0xffff0000) == 0xe5cd0000	/* strb rx,[sp,#n] */
	       || (insn & 0xffff00f0) == 0xe1cd00b0	/* strh rx,[sp,#n] */
	       || (insn & 0xffffc000) == 0xe58d0000)	/* str  rx,[sp,#n] */
	{
	  /* No need to add this to saved_regs -- it's just an arg reg.  */
	  continue;
	}
      else if ((insn & 0xfff00000) == 0xe8800000	/* stm Rn,
							   { registers } */
	       && pv_is_register (regs[bits (insn, 16, 19)], ARM_SP_REGNUM))
	{
	  /* No need to add this to saved_regs -- it's just arg regs.  */
	  continue;
	}
      else if ((insn & 0xfffff000) == 0xe24cb000)	/* sub fp, ip #n */
	{
	  uint32_t imm = arm_expand_immediate (insn & 0xfff);
	  regs[ARM_FP_REGNUM] = pv_add_constant (regs[ARM_IP_REGNUM], -imm);
	}
      else if ((insn & 0xfffff000) == 0xe24dd000)	/* sub sp, sp #n */
	{
	  uint32_t imm = arm_expand_immediate(insn & 0xfff);
	  regs[ARM_SP_REGNUM] = pv_add_constant (regs[ARM_SP_REGNUM], -imm);
	}
      else if ((insn & 0xffff7fff) == 0xed6d0103	/* stfe f?,
							   [sp, -#c]! */
	       && tdep->have_fpa_registers)
	{
	  if (stack.store_would_trash (regs[ARM_SP_REGNUM]))
	    break;

	  regs[ARM_SP_REGNUM] = pv_add_constant (regs[ARM_SP_REGNUM], -12);
	  regno = ARM_F0_REGNUM + ((insn >> 12) & 0x07);
	  stack.store (regs[ARM_SP_REGNUM], 12, regs[regno]);
	}
      else if ((insn & 0xffbf0fff) == 0xec2d0200	/* sfmfd f0, 4,
							   [sp!] */
	       && tdep->have_fpa_registers)
	{
	  int n_saved_fp_regs;
	  unsigned int fp_start_reg, fp_bound_reg;

	  if (stack.store_would_trash (regs[ARM_SP_REGNUM]))
	    break;

	  if ((insn & 0x800) == 0x800)		/* N0 is set */
	    {
	      if ((insn & 0x40000) == 0x40000)	/* N1 is set */
		n_saved_fp_regs = 3;
	      else
		n_saved_fp_regs = 1;
	    }
	  else
	    {
	      if ((insn & 0x40000) == 0x40000)	/* N1 is set */
		n_saved_fp_regs = 2;
	      else
		n_saved_fp_regs = 4;
	    }

	  fp_start_reg = ARM_F0_REGNUM + ((insn >> 12) & 0x7);
	  fp_bound_reg = fp_start_reg + n_saved_fp_regs;
	  for (; fp_start_reg < fp_bound_reg; fp_start_reg++)
	    {
	      regs[ARM_SP_REGNUM] = pv_add_constant (regs[ARM_SP_REGNUM], -12);
	      stack.store (regs[ARM_SP_REGNUM], 12,
			   regs[fp_start_reg++]);
	    }
	}
      else if ((insn & 0xff000000) == 0xeb000000 && cache == NULL) /* bl */
	{
	  /* Allow some special function calls when skipping the
	     prologue; GCC generates these before storing arguments to
	     the stack.  */
	  CORE_ADDR dest = BranchDest (current_pc, insn);

	  if (skip_prologue_function (gdbarch, dest, 0))
	    continue;
	  else
	    break;
	}
      else if ((insn & 0xf0000000) != 0xe0000000)
	break;			/* Condition not true, exit early.  */
      else if (arm_instruction_changes_pc (insn))
	/* Don't scan past anything that might change control flow.  */
	break;
      else if (arm_instruction_restores_sp (insn))
	{
	  /* Don't scan past the epilogue.  */
	  break;
	}
      else if ((insn & 0xfe500000) == 0xe8100000	/* ldm */
	       && pv_is_register (regs[bits (insn, 16, 19)], ARM_SP_REGNUM))
	/* Ignore block loads from the stack, potentially copying
	   parameters from memory.  */
	continue;
      else if ((insn & 0xfc500000) == 0xe4100000
	       && pv_is_register (regs[bits (insn, 16, 19)], ARM_SP_REGNUM))
	/* Similarly ignore single loads from the stack.  */
	continue;
      else if ((insn & 0xffff0ff0) == 0xe1a00000)
	/* MOV Rd, Rm.  Skip register copies, i.e. saves to another
	   register instead of the stack.  */
	continue;
      else
	{
	  /* The optimizer might shove anything into the prologue, if
	     we build up cache (cache != NULL) from scanning prologue,
	     we just skip what we don't recognize and scan further to
	     make cache as complete as possible.  However, if we skip
	     prologue, we'll stop immediately on unrecognized
	     instruction.  */
	  unrecognized_pc = current_pc;
	  if (cache != NULL)
	    continue;
	  else
	    break;
	}
    }

  if (unrecognized_pc == 0)
    unrecognized_pc = current_pc;

  if (cache)
    {
      int framereg, framesize;

      /* The frame size is just the distance from the frame register
	 to the original stack pointer.  */
      if (pv_is_register (regs[ARM_FP_REGNUM], ARM_SP_REGNUM))
	{
	  /* Frame pointer is fp.  */
	  framereg = ARM_FP_REGNUM;
	  framesize = -regs[ARM_FP_REGNUM].k;
	}
      else
	{
	  /* Try the stack pointer... this is a bit desperate.  */
	  framereg = ARM_SP_REGNUM;
	  framesize = -regs[ARM_SP_REGNUM].k;
	}

      cache->framereg = framereg;
      cache->framesize = framesize;

      for (regno = 0; regno < ARM_FPS_REGNUM; regno++)
	if (stack.find_reg (gdbarch, regno, &offset))
	  {
	    cache->saved_regs[regno].set_addr (offset);
	    if (regno == ARM_SP_REGNUM)
	      arm_cache_set_active_sp_value(cache, tdep, offset);
	  }
    }

  arm_debug_printf ("Prologue scan stopped at %s",
		    paddress (gdbarch, unrecognized_pc));

  return unrecognized_pc;
}

static void
arm_scan_prologue (frame_info_ptr this_frame,
		   struct arm_prologue_cache *cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR prologue_start, prologue_end;
  CORE_ADDR prev_pc = get_frame_pc (this_frame);
  CORE_ADDR block_addr = get_frame_address_in_block (this_frame);
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  /* Assume there is no frame until proven otherwise.  */
  cache->framereg = ARM_SP_REGNUM;
  cache->framesize = 0;

  /* Check for Thumb prologue.  */
  if (arm_frame_is_thumb (this_frame))
    {
      thumb_scan_prologue (gdbarch, prev_pc, block_addr, cache);
      return;
    }

  /* Find the function prologue.  If we can't find the function in
     the symbol table, peek in the stack frame to find the PC.  */
  if (find_pc_partial_function (block_addr, NULL, &prologue_start,
				&prologue_end))
    {
      /* One way to find the end of the prologue (which works well
	 for unoptimized code) is to do the following:

	    struct symtab_and_line sal = find_pc_line (prologue_start, 0);

	    if (sal.line == 0)
	      prologue_end = prev_pc;
	    else if (sal.end < prologue_end)
	      prologue_end = sal.end;

	 This mechanism is very accurate so long as the optimizer
	 doesn't move any instructions from the function body into the
	 prologue.  If this happens, sal.end will be the last
	 instruction in the first hunk of prologue code just before
	 the first instruction that the scheduler has moved from
	 the body to the prologue.

	 In order to make sure that we scan all of the prologue
	 instructions, we use a slightly less accurate mechanism which
	 may scan more than necessary.  To help compensate for this
	 lack of accuracy, the prologue scanning loop below contains
	 several clauses which'll cause the loop to terminate early if
	 an implausible prologue instruction is encountered.

	 The expression

	      prologue_start + 64

	 is a suitable endpoint since it accounts for the largest
	 possible prologue plus up to five instructions inserted by
	 the scheduler.  */

      if (prologue_end > prologue_start + 64)
	{
	  prologue_end = prologue_start + 64;	/* See above.  */
	}
    }
  else
    {
      /* We have no symbol information.  Our only option is to assume this
	 function has a standard stack frame and the normal frame register.
	 Then, we can find the value of our frame pointer on entrance to
	 the callee (or at the present moment if this is the innermost frame).
	 The value stored there should be the address of the stmfd + 8.  */
      CORE_ADDR frame_loc;
      ULONGEST return_value;

      /* AAPCS does not use a frame register, so we can abort here.  */
      if (tdep->arm_abi == ARM_ABI_AAPCS)
	return;

      frame_loc = get_frame_register_unsigned (this_frame, ARM_FP_REGNUM);
      if (!safe_read_memory_unsigned_integer (frame_loc, 4, byte_order,
					      &return_value))
	return;
      else
	{
	  prologue_start = gdbarch_addr_bits_remove
			     (gdbarch, return_value) - 8;
	  prologue_end = prologue_start + 64;	/* See above.  */
	}
    }

  if (prev_pc < prologue_end)
    prologue_end = prev_pc;

  arm_analyze_prologue (gdbarch, prologue_start, prologue_end, cache,
			target_arm_instruction_reader ());
}

static struct arm_prologue_cache *
arm_make_prologue_cache (frame_info_ptr this_frame)
{
  int reg;
  struct arm_prologue_cache *cache;
  CORE_ADDR unwound_fp, prev_sp;

  cache = FRAME_OBSTACK_ZALLOC (struct arm_prologue_cache);
  arm_cache_init (cache, this_frame);

  arm_scan_prologue (this_frame, cache);

  unwound_fp = get_frame_register_unsigned (this_frame, cache->framereg);
  if (unwound_fp == 0)
    return cache;

  arm_gdbarch_tdep *tdep =
    gdbarch_tdep<arm_gdbarch_tdep> (get_frame_arch (this_frame));

  prev_sp = unwound_fp + cache->framesize;
  arm_cache_set_active_sp_value (cache, tdep, prev_sp);

  /* Calculate actual addresses of saved registers using offsets
     determined by arm_scan_prologue.  */
  for (reg = 0; reg < gdbarch_num_regs (get_frame_arch (this_frame)); reg++)
    if (cache->saved_regs[reg].is_addr ())
      cache->saved_regs[reg].set_addr (cache->saved_regs[reg].addr () +
				       prev_sp);

  return cache;
}

/* Implementation of the stop_reason hook for arm_prologue frames.  */

static enum unwind_stop_reason
arm_prologue_unwind_stop_reason (frame_info_ptr this_frame,
				 void **this_cache)
{
  struct arm_prologue_cache *cache;
  CORE_ADDR pc;

  if (*this_cache == NULL)
    *this_cache = arm_make_prologue_cache (this_frame);
  cache = (struct arm_prologue_cache *) *this_cache;

  /* This is meant to halt the backtrace at "_start".  */
  pc = get_frame_pc (this_frame);
  gdbarch *arch = get_frame_arch (this_frame);
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (arch);
  if (pc <= tdep->lowest_pc)
    return UNWIND_OUTERMOST;

  /* If we've hit a wall, stop.  */
  if (arm_cache_get_prev_sp_value (cache, tdep) == 0)
    return UNWIND_OUTERMOST;

  return UNWIND_NO_REASON;
}

/* Our frame ID for a normal frame is the current function's starting PC
   and the caller's SP when we were called.  */

static void
arm_prologue_this_id (frame_info_ptr this_frame,
		      void **this_cache,
		      struct frame_id *this_id)
{
  struct arm_prologue_cache *cache;
  struct frame_id id;
  CORE_ADDR pc, func;

  if (*this_cache == NULL)
    *this_cache = arm_make_prologue_cache (this_frame);
  cache = (struct arm_prologue_cache *) *this_cache;

  arm_gdbarch_tdep *tdep
    = gdbarch_tdep<arm_gdbarch_tdep> (get_frame_arch (this_frame));

  /* Use function start address as part of the frame ID.  If we cannot
     identify the start address (due to missing symbol information),
     fall back to just using the current PC.  */
  pc = get_frame_pc (this_frame);
  func = get_frame_func (this_frame);
  if (!func)
    func = pc;

  id = frame_id_build (arm_cache_get_prev_sp_value (cache, tdep), func);
  *this_id = id;
}

static struct value *
arm_prologue_prev_register (frame_info_ptr this_frame,
			    void **this_cache,
			    int prev_regnum)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct arm_prologue_cache *cache;
  CORE_ADDR sp_value;

  if (*this_cache == NULL)
    *this_cache = arm_make_prologue_cache (this_frame);
  cache = (struct arm_prologue_cache *) *this_cache;

  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  /* If this frame has signed the return address, mark it as so.  */
  if (tdep->have_pacbti && cache->ra_signed_state.has_value ()
      && *cache->ra_signed_state)
    set_frame_previous_pc_masked (this_frame);

  /* If we are asked to unwind the PC, then we need to return the LR
     instead.  The prologue may save PC, but it will point into this
     frame's prologue, not the next frame's resume location.  Also
     strip the saved T bit.  A valid LR may have the low bit set, but
     a valid PC never does.  */
  if (prev_regnum == ARM_PC_REGNUM)
    {
      CORE_ADDR lr;

      lr = frame_unwind_register_unsigned (this_frame, ARM_LR_REGNUM);
      return frame_unwind_got_constant (this_frame, prev_regnum,
					arm_addr_bits_remove (gdbarch, lr));
    }

  /* SP is generally not saved to the stack, but this frame is
     identified by the next frame's stack pointer at the time of the call.
     The value was already reconstructed into PREV_SP.  */
  if (prev_regnum == ARM_SP_REGNUM)
    return frame_unwind_got_constant (this_frame, prev_regnum,
				      arm_cache_get_prev_sp_value (cache, tdep));

  /* The value might be one of the alternative SP, if so, use the
     value already constructed.  */
  if (arm_is_alternative_sp_register (tdep, prev_regnum))
    {
      sp_value = arm_cache_get_sp_register (cache, tdep, prev_regnum);
      return frame_unwind_got_constant (this_frame, prev_regnum, sp_value);
    }

  /* The CPSR may have been changed by the call instruction and by the
     called function.  The only bit we can reconstruct is the T bit,
     by checking the low bit of LR as of the call.  This is a reliable
     indicator of Thumb-ness except for some ARM v4T pre-interworking
     Thumb code, which could get away with a clear low bit as long as
     the called function did not use bx.  Guess that all other
     bits are unchanged; the condition flags are presumably lost,
     but the processor status is likely valid.  */
  if (prev_regnum == ARM_PS_REGNUM)
    {
      ULONGEST cpsr = get_frame_register_unsigned (this_frame, prev_regnum);
      CORE_ADDR lr = frame_unwind_register_unsigned (this_frame, ARM_LR_REGNUM);

      cpsr = reconstruct_t_bit (gdbarch, lr, cpsr);
      return frame_unwind_got_constant (this_frame, prev_regnum, cpsr);
    }

  return trad_frame_get_prev_register (this_frame, cache->saved_regs,
				       prev_regnum);
}

static frame_unwind arm_prologue_unwind = {
  "arm prologue",
  NORMAL_FRAME,
  arm_prologue_unwind_stop_reason,
  arm_prologue_this_id,
  arm_prologue_prev_register,
  NULL,
  default_frame_sniffer
};

/* Maintain a list of ARM exception table entries per objfile, similar to the
   list of mapping symbols.  We only cache entries for standard ARM-defined
   personality routines; the cache will contain only the frame unwinding
   instructions associated with the entry (not the descriptors).  */

struct arm_exidx_entry
{
  CORE_ADDR addr;
  gdb_byte *entry;

  bool operator< (const arm_exidx_entry &other) const
  {
    return addr < other.addr;
  }
};

struct arm_exidx_data
{
  std::vector<std::vector<arm_exidx_entry>> section_maps;
};

/* Per-BFD key to store exception handling information.  */
static const registry<bfd>::key<arm_exidx_data> arm_exidx_data_key;

static struct obj_section *
arm_obj_section_from_vma (struct objfile *objfile, bfd_vma vma)
{
  for (obj_section *osect : objfile->sections ())
    if (bfd_section_flags (osect->the_bfd_section) & SEC_ALLOC)
      {
	bfd_vma start, size;
	start = bfd_section_vma (osect->the_bfd_section);
	size = bfd_section_size (osect->the_bfd_section);

	if (start <= vma && vma < start + size)
	  return osect;
      }

  return NULL;
}

/* Parse contents of exception table and exception index sections
   of OBJFILE, and fill in the exception table entry cache.

   For each entry that refers to a standard ARM-defined personality
   routine, extract the frame unwinding instructions (from either
   the index or the table section).  The unwinding instructions
   are normalized by:
    - extracting them from the rest of the table data
    - converting to host endianness
    - appending the implicit 0xb0 ("Finish") code

   The extracted and normalized instructions are stored for later
   retrieval by the arm_find_exidx_entry routine.  */
 
static void
arm_exidx_new_objfile (struct objfile *objfile)
{
  struct arm_exidx_data *data;
  asection *exidx, *extab;
  bfd_vma exidx_vma = 0, extab_vma = 0;
  LONGEST i;

  /* If we've already touched this file, do nothing.  */
  if (arm_exidx_data_key.get (objfile->obfd.get ()) != nullptr)
    return;

  /* Read contents of exception table and index.  */
  exidx = bfd_get_section_by_name (objfile->obfd.get (),
				   ELF_STRING_ARM_unwind);
  gdb::byte_vector exidx_data;
  if (exidx)
    {
      exidx_vma = bfd_section_vma (exidx);
      exidx_data.resize (bfd_section_size (exidx));

      if (!bfd_get_section_contents (objfile->obfd.get (), exidx,
				     exidx_data.data (), 0,
				     exidx_data.size ()))
	return;
    }

  extab = bfd_get_section_by_name (objfile->obfd.get (), ".ARM.extab");
  gdb::byte_vector extab_data;
  if (extab)
    {
      extab_vma = bfd_section_vma (extab);
      extab_data.resize (bfd_section_size (extab));

      if (!bfd_get_section_contents (objfile->obfd.get (), extab,
				     extab_data.data (), 0,
				     extab_data.size ()))
	return;
    }

  /* Allocate exception table data structure.  */
  data = arm_exidx_data_key.emplace (objfile->obfd.get ());
  data->section_maps.resize (objfile->obfd->section_count);

  /* Fill in exception table.  */
  for (i = 0; i < exidx_data.size () / 8; i++)
    {
      struct arm_exidx_entry new_exidx_entry;
      bfd_vma idx = bfd_h_get_32 (objfile->obfd, exidx_data.data () + i * 8);
      bfd_vma val = bfd_h_get_32 (objfile->obfd,
				  exidx_data.data () + i * 8 + 4);
      bfd_vma addr = 0, word = 0;
      int n_bytes = 0, n_words = 0;
      struct obj_section *sec;
      gdb_byte *entry = NULL;

      /* Extract address of start of function.  */
      idx = ((idx & 0x7fffffff) ^ 0x40000000) - 0x40000000;
      idx += exidx_vma + i * 8;

      /* Find section containing function and compute section offset.  */
      sec = arm_obj_section_from_vma (objfile, idx);
      if (sec == NULL)
	continue;
      idx -= bfd_section_vma (sec->the_bfd_section);

      /* Determine address of exception table entry.  */
      if (val == 1)
	{
	  /* EXIDX_CANTUNWIND -- no exception table entry present.  */
	}
      else if ((val & 0xff000000) == 0x80000000)
	{
	  /* Exception table entry embedded in .ARM.exidx
	     -- must be short form.  */
	  word = val;
	  n_bytes = 3;
	}
      else if (!(val & 0x80000000))
	{
	  /* Exception table entry in .ARM.extab.  */
	  addr = ((val & 0x7fffffff) ^ 0x40000000) - 0x40000000;
	  addr += exidx_vma + i * 8 + 4;

	  if (addr >= extab_vma && addr + 4 <= extab_vma + extab_data.size ())
	    {
	      word = bfd_h_get_32 (objfile->obfd,
				   extab_data.data () + addr - extab_vma);
	      addr += 4;

	      if ((word & 0xff000000) == 0x80000000)
		{
		  /* Short form.  */
		  n_bytes = 3;
		}
	      else if ((word & 0xff000000) == 0x81000000
		       || (word & 0xff000000) == 0x82000000)
		{
		  /* Long form.  */
		  n_bytes = 2;
		  n_words = ((word >> 16) & 0xff);
		}
	      else if (!(word & 0x80000000))
		{
		  bfd_vma pers;
		  struct obj_section *pers_sec;
		  int gnu_personality = 0;

		  /* Custom personality routine.  */
		  pers = ((word & 0x7fffffff) ^ 0x40000000) - 0x40000000;
		  pers = UNMAKE_THUMB_ADDR (pers + addr - 4);

		  /* Check whether we've got one of the variants of the
		     GNU personality routines.  */
		  pers_sec = arm_obj_section_from_vma (objfile, pers);
		  if (pers_sec)
		    {
		      static const char *personality[] = 
			{
			  "__gcc_personality_v0",
			  "__gxx_personality_v0",
			  "__gcj_personality_v0",
			  "__gnu_objc_personality_v0",
			  NULL
			};

		      CORE_ADDR pc = pers + pers_sec->offset ();
		      int k;

		      for (k = 0; personality[k]; k++)
			if (lookup_minimal_symbol_by_pc_name
			      (pc, personality[k], objfile))
			  {
			    gnu_personality = 1;
			    break;
			  }
		    }

		  /* If so, the next word contains a word count in the high
		     byte, followed by the same unwind instructions as the
		     pre-defined forms.  */
		  if (gnu_personality
		      && addr + 4 <= extab_vma + extab_data.size ())
		    {
		      word = bfd_h_get_32 (objfile->obfd,
					   (extab_data.data ()
					    + addr - extab_vma));
		      addr += 4;
		      n_bytes = 3;
		      n_words = ((word >> 24) & 0xff);
		    }
		}
	    }
	}

      /* Sanity check address.  */
      if (n_words)
	if (addr < extab_vma
	    || addr + 4 * n_words > extab_vma + extab_data.size ())
	  n_words = n_bytes = 0;

      /* The unwind instructions reside in WORD (only the N_BYTES least
	 significant bytes are valid), followed by N_WORDS words in the
	 extab section starting at ADDR.  */
      if (n_bytes || n_words)
	{
	  gdb_byte *p = entry
	    = (gdb_byte *) obstack_alloc (&objfile->objfile_obstack,
					  n_bytes + n_words * 4 + 1);

	  while (n_bytes--)
	    *p++ = (gdb_byte) ((word >> (8 * n_bytes)) & 0xff);

	  while (n_words--)
	    {
	      word = bfd_h_get_32 (objfile->obfd,
				   extab_data.data () + addr - extab_vma);
	      addr += 4;

	      *p++ = (gdb_byte) ((word >> 24) & 0xff);
	      *p++ = (gdb_byte) ((word >> 16) & 0xff);
	      *p++ = (gdb_byte) ((word >> 8) & 0xff);
	      *p++ = (gdb_byte) (word & 0xff);
	    }

	  /* Implied "Finish" to terminate the list.  */
	  *p++ = 0xb0;
	}

      /* Push entry onto vector.  They are guaranteed to always
	 appear in order of increasing addresses.  */
      new_exidx_entry.addr = idx;
      new_exidx_entry.entry = entry;
      data->section_maps[sec->the_bfd_section->index].push_back
	(new_exidx_entry);
    }
}

/* Search for the exception table entry covering MEMADDR.  If one is found,
   return a pointer to its data.  Otherwise, return 0.  If START is non-NULL,
   set *START to the start of the region covered by this entry.  */

static gdb_byte *
arm_find_exidx_entry (CORE_ADDR memaddr, CORE_ADDR *start)
{
  struct obj_section *sec;

  sec = find_pc_section (memaddr);
  if (sec != NULL)
    {
      struct arm_exidx_data *data;
      struct arm_exidx_entry map_key = { memaddr - sec->addr (), 0 };

      data = arm_exidx_data_key.get (sec->objfile->obfd.get ());
      if (data != NULL)
	{
	  std::vector<arm_exidx_entry> &map
	    = data->section_maps[sec->the_bfd_section->index];
	  if (!map.empty ())
	    {
	      auto idx = std::lower_bound (map.begin (), map.end (), map_key);

	      /* std::lower_bound finds the earliest ordered insertion
		 point.  If the following symbol starts at this exact
		 address, we use that; otherwise, the preceding
		 exception table entry covers this address.  */
	      if (idx < map.end ())
		{
		  if (idx->addr == map_key.addr)
		    {
		      if (start)
			*start = idx->addr + sec->addr ();
		      return idx->entry;
		    }
		}

	      if (idx > map.begin ())
		{
		  idx = idx - 1;
		  if (start)
		    *start = idx->addr + sec->addr ();
		  return idx->entry;
		}
	    }
	}
    }

  return NULL;
}

/* Given the current frame THIS_FRAME, and its associated frame unwinding
   instruction list from the ARM exception table entry ENTRY, allocate and
   return a prologue cache structure describing how to unwind this frame.

   Return NULL if the unwinding instruction list contains a "spare",
   "reserved" or "refuse to unwind" instruction as defined in section
   "9.3 Frame unwinding instructions" of the "Exception Handling ABI
   for the ARM Architecture" document.  */

static struct arm_prologue_cache *
arm_exidx_fill_cache (frame_info_ptr this_frame, gdb_byte *entry)
{
  CORE_ADDR vsp = 0;
  int vsp_valid = 0;

  struct arm_prologue_cache *cache;
  cache = FRAME_OBSTACK_ZALLOC (struct arm_prologue_cache);
  arm_cache_init (cache, this_frame);

  for (;;)
    {
      gdb_byte insn;

      /* Whenever we reload SP, we actually have to retrieve its
	 actual value in the current frame.  */
      if (!vsp_valid)
	{
	  if (cache->saved_regs[ARM_SP_REGNUM].is_realreg ())
	    {
	      int reg = cache->saved_regs[ARM_SP_REGNUM].realreg ();
	      vsp = get_frame_register_unsigned (this_frame, reg);
	    }
	  else
	    {
	      CORE_ADDR addr = cache->saved_regs[ARM_SP_REGNUM].addr ();
	      vsp = get_frame_memory_unsigned (this_frame, addr, 4);
	    }

	  vsp_valid = 1;
	}

      /* Decode next unwind instruction.  */
      insn = *entry++;

      if ((insn & 0xc0) == 0)
	{
	  int offset = insn & 0x3f;
	  vsp += (offset << 2) + 4;
	}
      else if ((insn & 0xc0) == 0x40)
	{
	  int offset = insn & 0x3f;
	  vsp -= (offset << 2) + 4;
	}
      else if ((insn & 0xf0) == 0x80)
	{
	  int mask = ((insn & 0xf) << 8) | *entry++;
	  int i;

	  /* The special case of an all-zero mask identifies
	     "Refuse to unwind".  We return NULL to fall back
	     to the prologue analyzer.  */
	  if (mask == 0)
	    return NULL;

	  /* Pop registers r4..r15 under mask.  */
	  for (i = 0; i < 12; i++)
	    if (mask & (1 << i))
	      {
		cache->saved_regs[4 + i].set_addr (vsp);
		vsp += 4;
	      }

	  /* Special-case popping SP -- we need to reload vsp.  */
	  if (mask & (1 << (ARM_SP_REGNUM - 4)))
	    vsp_valid = 0;
	}
      else if ((insn & 0xf0) == 0x90)
	{
	  int reg = insn & 0xf;

	  /* Reserved cases.  */
	  if (reg == ARM_SP_REGNUM || reg == ARM_PC_REGNUM)
	    return NULL;

	  /* Set SP from another register and mark VSP for reload.  */
	  cache->saved_regs[ARM_SP_REGNUM] = cache->saved_regs[reg];
	  vsp_valid = 0;
	}
      else if ((insn & 0xf0) == 0xa0)
	{
	  int count = insn & 0x7;
	  int pop_lr = (insn & 0x8) != 0;
	  int i;

	  /* Pop r4..r[4+count].  */
	  for (i = 0; i <= count; i++)
	    {
	      cache->saved_regs[4 + i].set_addr (vsp);
	      vsp += 4;
	    }

	  /* If indicated by flag, pop LR as well.  */
	  if (pop_lr)
	    {
	      cache->saved_regs[ARM_LR_REGNUM].set_addr (vsp);
	      vsp += 4;
	    }
	}
      else if (insn == 0xb0)
	{
	  /* We could only have updated PC by popping into it; if so, it
	     will show up as address.  Otherwise, copy LR into PC.  */
	  if (!cache->saved_regs[ARM_PC_REGNUM].is_addr ())
	    cache->saved_regs[ARM_PC_REGNUM]
	      = cache->saved_regs[ARM_LR_REGNUM];

	  /* We're done.  */
	  break;
	}
      else if (insn == 0xb1)
	{
	  int mask = *entry++;
	  int i;

	  /* All-zero mask and mask >= 16 is "spare".  */
	  if (mask == 0 || mask >= 16)
	    return NULL;

	  /* Pop r0..r3 under mask.  */
	  for (i = 0; i < 4; i++)
	    if (mask & (1 << i))
	      {
		cache->saved_regs[i].set_addr (vsp);
		vsp += 4;
	      }
	}
      else if (insn == 0xb2)
	{
	  ULONGEST offset = 0;
	  unsigned shift = 0;

	  do
	    {
	      offset |= (*entry & 0x7f) << shift;
	      shift += 7;
	    }
	  while (*entry++ & 0x80);

	  vsp += 0x204 + (offset << 2);
	}
      else if (insn == 0xb3)
	{
	  int start = *entry >> 4;
	  int count = (*entry++) & 0xf;
	  int i;

	  /* Only registers D0..D15 are valid here.  */
	  if (start + count >= 16)
	    return NULL;

	  /* Pop VFP double-precision registers D[start]..D[start+count].  */
	  for (i = 0; i <= count; i++)
	    {
	      cache->saved_regs[ARM_D0_REGNUM + start + i].set_addr (vsp);
	      vsp += 8;
	    }

	  /* Add an extra 4 bytes for FSTMFDX-style stack.  */
	  vsp += 4;
	}
      else if ((insn & 0xf8) == 0xb8)
	{
	  int count = insn & 0x7;
	  int i;

	  /* Pop VFP double-precision registers D[8]..D[8+count].  */
	  for (i = 0; i <= count; i++)
	    {
	      cache->saved_regs[ARM_D0_REGNUM + 8 + i].set_addr (vsp);
	      vsp += 8;
	    }

	  /* Add an extra 4 bytes for FSTMFDX-style stack.  */
	  vsp += 4;
	}
      else if (insn == 0xc6)
	{
	  int start = *entry >> 4;
	  int count = (*entry++) & 0xf;
	  int i;

	  /* Only registers WR0..WR15 are valid.  */
	  if (start + count >= 16)
	    return NULL;

	  /* Pop iwmmx registers WR[start]..WR[start+count].  */
	  for (i = 0; i <= count; i++)
	    {
	      cache->saved_regs[ARM_WR0_REGNUM + start + i].set_addr (vsp);
	      vsp += 8;
	    }
	}
      else if (insn == 0xc7)
	{
	  int mask = *entry++;
	  int i;

	  /* All-zero mask and mask >= 16 is "spare".  */
	  if (mask == 0 || mask >= 16)
	    return NULL;

	  /* Pop iwmmx general-purpose registers WCGR0..WCGR3 under mask.  */
	  for (i = 0; i < 4; i++)
	    if (mask & (1 << i))
	      {
		cache->saved_regs[ARM_WCGR0_REGNUM + i].set_addr (vsp);
		vsp += 4;
	      }
	}
      else if ((insn & 0xf8) == 0xc0)
	{
	  int count = insn & 0x7;
	  int i;

	  /* Pop iwmmx registers WR[10]..WR[10+count].  */
	  for (i = 0; i <= count; i++)
	    {
	      cache->saved_regs[ARM_WR0_REGNUM + 10 + i].set_addr (vsp);
	      vsp += 8;
	    }
	}
      else if (insn == 0xc8)
	{
	  int start = *entry >> 4;
	  int count = (*entry++) & 0xf;
	  int i;

	  /* Only registers D0..D31 are valid.  */
	  if (start + count >= 16)
	    return NULL;

	  /* Pop VFP double-precision registers
	     D[16+start]..D[16+start+count].  */
	  for (i = 0; i <= count; i++)
	    {
	      cache->saved_regs[ARM_D0_REGNUM + 16 + start + i].set_addr (vsp);
	      vsp += 8;
	    }
	}
      else if (insn == 0xc9)
	{
	  int start = *entry >> 4;
	  int count = (*entry++) & 0xf;
	  int i;

	  /* Pop VFP double-precision registers D[start]..D[start+count].  */
	  for (i = 0; i <= count; i++)
	    {
	      cache->saved_regs[ARM_D0_REGNUM + start + i].set_addr (vsp);
	      vsp += 8;
	    }
	}
      else if ((insn & 0xf8) == 0xd0)
	{
	  int count = insn & 0x7;
	  int i;

	  /* Pop VFP double-precision registers D[8]..D[8+count].  */
	  for (i = 0; i <= count; i++)
	    {
	      cache->saved_regs[ARM_D0_REGNUM + 8 + i].set_addr (vsp);
	      vsp += 8;
	    }
	}
      else
	{
	  /* Everything else is "spare".  */
	  return NULL;
	}
    }

  /* If we restore SP from a register, assume this was the frame register.
     Otherwise just fall back to SP as frame register.  */
  if (cache->saved_regs[ARM_SP_REGNUM].is_realreg ())
    cache->framereg = cache->saved_regs[ARM_SP_REGNUM].realreg ();
  else
    cache->framereg = ARM_SP_REGNUM;

  /* Determine offset to previous frame.  */
  cache->framesize
    = vsp - get_frame_register_unsigned (this_frame, cache->framereg);

  /* We already got the previous SP.  */
  arm_gdbarch_tdep *tdep
    = gdbarch_tdep<arm_gdbarch_tdep> (get_frame_arch (this_frame));
  arm_cache_set_active_sp_value (cache, tdep, vsp);

  return cache;
}

/* Unwinding via ARM exception table entries.  Note that the sniffer
   already computes a filled-in prologue cache, which is then used
   with the same arm_prologue_this_id and arm_prologue_prev_register
   routines also used for prologue-parsing based unwinding.  */

static int
arm_exidx_unwind_sniffer (const struct frame_unwind *self,
			  frame_info_ptr this_frame,
			  void **this_prologue_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order_for_code = gdbarch_byte_order_for_code (gdbarch);
  CORE_ADDR addr_in_block, exidx_region, func_start;
  struct arm_prologue_cache *cache;
  gdb_byte *entry;

  /* See if we have an ARM exception table entry covering this address.  */
  addr_in_block = get_frame_address_in_block (this_frame);
  entry = arm_find_exidx_entry (addr_in_block, &exidx_region);
  if (!entry)
    return 0;

  /* The ARM exception table does not describe unwind information
     for arbitrary PC values, but is guaranteed to be correct only
     at call sites.  We have to decide here whether we want to use
     ARM exception table information for this frame, or fall back
     to using prologue parsing.  (Note that if we have DWARF CFI,
     this sniffer isn't even called -- CFI is always preferred.)

     Before we make this decision, however, we check whether we
     actually have *symbol* information for the current frame.
     If not, prologue parsing would not work anyway, so we might
     as well use the exception table and hope for the best.  */
  if (find_pc_partial_function (addr_in_block, NULL, &func_start, NULL))
    {
      int exc_valid = 0;

      /* If the next frame is "normal", we are at a call site in this
	 frame, so exception information is guaranteed to be valid.  */
      if (get_next_frame (this_frame)
	  && get_frame_type (get_next_frame (this_frame)) == NORMAL_FRAME)
	exc_valid = 1;

      /* Some syscalls keep PC pointing to the SVC instruction itself.  */
      for (int shift = 0; shift <= 1 && !exc_valid; ++shift)
	{
	  /* We also assume exception information is valid if we're currently
	     blocked in a system call.  The system library is supposed to
	     ensure this, so that e.g. pthread cancellation works.  */
	  if (arm_frame_is_thumb (this_frame))
	    {
	      ULONGEST insn;

	      if (safe_read_memory_unsigned_integer ((get_frame_pc (this_frame)
						      - (shift ? 2 : 0)),
						     2, byte_order_for_code,
						     &insn)
		  && (insn & 0xff00) == 0xdf00 /* svc */)
		exc_valid = 1;
	    }
	  else
	    {
	      ULONGEST insn;

	      if (safe_read_memory_unsigned_integer ((get_frame_pc (this_frame)
						      - (shift ? 4 : 0)),
						     4, byte_order_for_code,
						     &insn)
		  && (insn & 0x0f000000) == 0x0f000000 /* svc */)
		exc_valid = 1;
	    }
	}
	
      /* Bail out if we don't know that exception information is valid.  */
      if (!exc_valid)
	return 0;

     /* The ARM exception index does not mark the *end* of the region
	covered by the entry, and some functions will not have any entry.
	To correctly recognize the end of the covered region, the linker
	should have inserted dummy records with a CANTUNWIND marker.

	Unfortunately, current versions of GNU ld do not reliably do
	this, and thus we may have found an incorrect entry above.
	As a (temporary) sanity check, we only use the entry if it
	lies *within* the bounds of the function.  Note that this check
	might reject perfectly valid entries that just happen to cover
	multiple functions; therefore this check ought to be removed
	once the linker is fixed.  */
      if (func_start > exidx_region)
	return 0;
    }

  /* Decode the list of unwinding instructions into a prologue cache.
     Note that this may fail due to e.g. a "refuse to unwind" code.  */
  cache = arm_exidx_fill_cache (this_frame, entry);
  if (!cache)
    return 0;

  *this_prologue_cache = cache;
  return 1;
}

struct frame_unwind arm_exidx_unwind = {
  "arm exidx",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  arm_prologue_this_id,
  arm_prologue_prev_register,
  NULL,
  arm_exidx_unwind_sniffer
};

static struct arm_prologue_cache *
arm_make_epilogue_frame_cache (frame_info_ptr this_frame)
{
  struct arm_prologue_cache *cache;
  int reg;

  cache = FRAME_OBSTACK_ZALLOC (struct arm_prologue_cache);
  arm_cache_init (cache, this_frame);

  /* Still rely on the offset calculated from prologue.  */
  arm_scan_prologue (this_frame, cache);

  /* Since we are in epilogue, the SP has been restored.  */
  arm_gdbarch_tdep *tdep
    = gdbarch_tdep<arm_gdbarch_tdep> (get_frame_arch (this_frame));
  arm_cache_set_active_sp_value (cache, tdep,
				 get_frame_register_unsigned (this_frame,
							      ARM_SP_REGNUM));

  /* Calculate actual addresses of saved registers using offsets
     determined by arm_scan_prologue.  */
  for (reg = 0; reg < gdbarch_num_regs (get_frame_arch (this_frame)); reg++)
    if (cache->saved_regs[reg].is_addr ())
      cache->saved_regs[reg].set_addr (cache->saved_regs[reg].addr ()
				       + arm_cache_get_prev_sp_value (cache, tdep));

  return cache;
}

/* Implementation of function hook 'this_id' in
   'struct frame_uwnind' for epilogue unwinder.  */

static void
arm_epilogue_frame_this_id (frame_info_ptr this_frame,
			    void **this_cache,
			    struct frame_id *this_id)
{
  struct arm_prologue_cache *cache;
  CORE_ADDR pc, func;

  if (*this_cache == NULL)
    *this_cache = arm_make_epilogue_frame_cache (this_frame);
  cache = (struct arm_prologue_cache *) *this_cache;

  /* Use function start address as part of the frame ID.  If we cannot
     identify the start address (due to missing symbol information),
     fall back to just using the current PC.  */
  pc = get_frame_pc (this_frame);
  func = get_frame_func (this_frame);
  if (func == 0)
    func = pc;

  arm_gdbarch_tdep *tdep
    = gdbarch_tdep<arm_gdbarch_tdep> (get_frame_arch (this_frame));
  *this_id = frame_id_build (arm_cache_get_prev_sp_value (cache, tdep), pc);
}

/* Implementation of function hook 'prev_register' in
   'struct frame_uwnind' for epilogue unwinder.  */

static struct value *
arm_epilogue_frame_prev_register (frame_info_ptr this_frame,
				  void **this_cache, int regnum)
{
  if (*this_cache == NULL)
    *this_cache = arm_make_epilogue_frame_cache (this_frame);

  return arm_prologue_prev_register (this_frame, this_cache, regnum);
}

static int arm_stack_frame_destroyed_p_1 (struct gdbarch *gdbarch,
					  CORE_ADDR pc);
static int thumb_stack_frame_destroyed_p (struct gdbarch *gdbarch,
					  CORE_ADDR pc);

/* Implementation of function hook 'sniffer' in
   'struct frame_uwnind' for epilogue unwinder.  */

static int
arm_epilogue_frame_sniffer (const struct frame_unwind *self,
			    frame_info_ptr this_frame,
			    void **this_prologue_cache)
{
  if (frame_relative_level (this_frame) == 0)
    {
      struct gdbarch *gdbarch = get_frame_arch (this_frame);
      CORE_ADDR pc = get_frame_pc (this_frame);

      if (arm_frame_is_thumb (this_frame))
	return thumb_stack_frame_destroyed_p (gdbarch, pc);
      else
	return arm_stack_frame_destroyed_p_1 (gdbarch, pc);
    }
  else
    return 0;
}

/* Frame unwinder from epilogue.  */

static const struct frame_unwind arm_epilogue_frame_unwind =
{
  "arm epilogue",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  arm_epilogue_frame_this_id,
  arm_epilogue_frame_prev_register,
  NULL,
  arm_epilogue_frame_sniffer,
};

/* Recognize GCC's trampoline for thumb call-indirect.  If we are in a
   trampoline, return the target PC.  Otherwise return 0.

   void call0a (char c, short s, int i, long l) {}

   int main (void)
   {
     (*pointer_to_call0a) (c, s, i, l);
   }

   Instead of calling a stub library function  _call_via_xx (xx is
   the register name), GCC may inline the trampoline in the object
   file as below (register r2 has the address of call0a).

   .global main
   .type main, %function
   ...
   bl .L1
   ...
   .size main, .-main

   .L1:
   bx r2

   The trampoline 'bx r2' doesn't belong to main.  */

static CORE_ADDR
arm_skip_bx_reg (frame_info_ptr frame, CORE_ADDR pc)
{
  /* The heuristics of recognizing such trampoline is that FRAME is
     executing in Thumb mode and the instruction on PC is 'bx Rm'.  */
  if (arm_frame_is_thumb (frame))
    {
      gdb_byte buf[2];

      if (target_read_memory (pc, buf, 2) == 0)
	{
	  struct gdbarch *gdbarch = get_frame_arch (frame);
	  enum bfd_endian byte_order_for_code
	    = gdbarch_byte_order_for_code (gdbarch);
	  uint16_t insn
	    = extract_unsigned_integer (buf, 2, byte_order_for_code);

	  if ((insn & 0xff80) == 0x4700)  /* bx <Rm> */
	    {
	      CORE_ADDR dest
		= get_frame_register_unsigned (frame, bits (insn, 3, 6));

	      /* Clear the LSB so that gdb core sets step-resume
		 breakpoint at the right address.  */
	      return UNMAKE_THUMB_ADDR (dest);
	    }
	}
    }

  return 0;
}

static struct arm_prologue_cache *
arm_make_stub_cache (frame_info_ptr this_frame)
{
  struct arm_prologue_cache *cache;

  cache = FRAME_OBSTACK_ZALLOC (struct arm_prologue_cache);
  arm_cache_init (cache, this_frame);

  arm_gdbarch_tdep *tdep
    = gdbarch_tdep<arm_gdbarch_tdep> (get_frame_arch (this_frame));
  arm_cache_set_active_sp_value (cache, tdep,
				 get_frame_register_unsigned (this_frame,
							      ARM_SP_REGNUM));

  return cache;
}

/* Our frame ID for a stub frame is the current SP and LR.  */

static void
arm_stub_this_id (frame_info_ptr this_frame,
		  void **this_cache,
		  struct frame_id *this_id)
{
  struct arm_prologue_cache *cache;

  if (*this_cache == NULL)
    *this_cache = arm_make_stub_cache (this_frame);
  cache = (struct arm_prologue_cache *) *this_cache;

  arm_gdbarch_tdep *tdep
    = gdbarch_tdep<arm_gdbarch_tdep> (get_frame_arch (this_frame));
  *this_id = frame_id_build (arm_cache_get_prev_sp_value (cache, tdep),
			     get_frame_pc (this_frame));
}

static int
arm_stub_unwind_sniffer (const struct frame_unwind *self,
			 frame_info_ptr this_frame,
			 void **this_prologue_cache)
{
  CORE_ADDR addr_in_block;
  gdb_byte dummy[4];
  CORE_ADDR pc, start_addr;
  const char *name;

  addr_in_block = get_frame_address_in_block (this_frame);
  pc = get_frame_pc (this_frame);
  if (in_plt_section (addr_in_block)
      /* We also use the stub winder if the target memory is unreadable
	 to avoid having the prologue unwinder trying to read it.  */
      || target_read_memory (pc, dummy, 4) != 0)
    return 1;

  if (find_pc_partial_function (pc, &name, &start_addr, NULL) == 0
      && arm_skip_bx_reg (this_frame, pc) != 0)
    return 1;

  return 0;
}

struct frame_unwind arm_stub_unwind = {
  "arm stub",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  arm_stub_this_id,
  arm_prologue_prev_register,
  NULL,
  arm_stub_unwind_sniffer
};

/* Put here the code to store, into CACHE->saved_regs, the addresses
   of the saved registers of frame described by THIS_FRAME.  CACHE is
   returned.  */

static struct arm_prologue_cache *
arm_m_exception_cache (frame_info_ptr this_frame)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);
  struct arm_prologue_cache *cache;

  cache = FRAME_OBSTACK_ZALLOC (struct arm_prologue_cache);
  arm_cache_init (cache, this_frame);

  /* ARMv7-M Architecture Reference "B1.5.6 Exception entry behavior"
     describes which bits in LR that define which stack was used prior
     to the exception and if FPU is used (causing extended stack frame).  */

  /* In the lockup state PC contains a lockup magic value.
     The PC value of the the next outer frame is irreversibly
     lost.  The other registers are intact so LR likely contains
     PC of some frame next to the outer one, but we cannot analyze
     the next outer frame without knowing its PC
     therefore we do not know SP fixup for this frame.
     Some heuristics to resynchronize SP might be possible.
     For simplicity, just terminate the unwinding to prevent it going
     astray and attempting to read data/addresses it shouldn't,
     which may cause further issues due to side-effects.  */
  CORE_ADDR pc = get_frame_pc (this_frame);
  if (arm_m_addr_is_lockup (pc))
    {
      /* The lockup can be real just in the innermost frame
	 as the CPU is stopped and cannot create more frames.
	 If we hit lockup magic PC in the other frame, it is
	 just a sentinel at the top of stack: do not warn then.  */
      if (frame_relative_level (this_frame) == 0)
	warning (_("ARM M in lockup state, stack unwinding terminated."));

      /* Terminate any further stack unwinding.  */
      arm_cache_set_active_sp_value (cache, tdep, 0);
      return cache;
    }

  CORE_ADDR lr = get_frame_register_unsigned (this_frame, ARM_LR_REGNUM);

  /* ARMv7-M Architecture Reference "A2.3.1 Arm core registers"
     states that LR is set to 0xffffffff on reset.  ARMv8-M Architecture
     Reference "B3.3 Registers" states that LR is set to 0xffffffff on warm
     reset if Main Extension is implemented, otherwise the value is unknown.  */
  if (lr == 0xffffffff)
    {
      /* Terminate any further stack unwinding.  */
      arm_cache_set_active_sp_value (cache, tdep, 0);
      return cache;
    }

  /* Check FNC_RETURN indicator bits (24-31).  */
  bool fnc_return = (((lr >> 24) & 0xff) == 0xfe);
  if (fnc_return)
    {
      /* FNC_RETURN is only valid for targets with Security Extension.  */
      if (!tdep->have_sec_ext)
	{
	  error (_("While unwinding an exception frame, found unexpected Link "
		   "Register value %s that requires the security extension, "
		   "but the extension was not found or is disabled.  This "
		   "should not happen and may be caused by corrupt data or a "
		   "bug in GDB."), phex (lr, ARM_INT_REGISTER_SIZE));
	}

      if (!arm_unwind_secure_frames)
	{
	  warning (_("Non-secure to secure stack unwinding disabled."));

	  /* Terminate any further stack unwinding.  */
	  arm_cache_set_active_sp_value (cache, tdep, 0);
	  return cache;
	}

      ULONGEST xpsr = get_frame_register_unsigned (this_frame, ARM_PS_REGNUM);
      if ((xpsr & 0x1ff) != 0)
	/* Handler mode: This is the mode that exceptions are handled in.  */
	arm_cache_switch_prev_sp (cache, tdep, tdep->m_profile_msp_s_regnum);
      else
	/* Thread mode: This is the normal mode that programs run in.  */
	arm_cache_switch_prev_sp (cache, tdep, tdep->m_profile_psp_s_regnum);

      CORE_ADDR unwound_sp = arm_cache_get_prev_sp_value (cache, tdep);

      /* Stack layout for a function call from Secure to Non-Secure state
	 (ARMv8-M section B3.16):

	    SP Offset

		    +-------------------+
	     0x08   |                   |
		    +-------------------+   <-- Original SP
	     0x04   |    Partial xPSR   |
		    +-------------------+
	     0x00   |   Return Address  |
		    +===================+   <-- New SP  */

      cache->saved_regs[ARM_PC_REGNUM].set_addr (unwound_sp + 0x00);
      cache->saved_regs[ARM_LR_REGNUM].set_addr (unwound_sp + 0x00);
      cache->saved_regs[ARM_PS_REGNUM].set_addr (unwound_sp + 0x04);

      arm_cache_set_active_sp_value (cache, tdep, unwound_sp + 0x08);

      return cache;
    }

  /* Check EXC_RETURN indicator bits (24-31).  */
  bool exc_return = (((lr >> 24) & 0xff) == 0xff);
  if (exc_return)
    {
      int sp_regnum;
      bool secure_stack_used = false;
      bool default_callee_register_stacking = false;
      bool exception_domain_is_secure = false;
      enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

      /* Check EXC_RETURN bit SPSEL if Main or Thread (process) stack used.  */
      bool process_stack_used = (bit (lr, 2) != 0);

      if (tdep->have_sec_ext)
	{
	  secure_stack_used = (bit (lr, 6) != 0);
	  default_callee_register_stacking = (bit (lr, 5) != 0);
	  exception_domain_is_secure = (bit (lr, 0) != 0);

	  /* Unwinding from non-secure to secure can trip security
	     measures.  In order to avoid the debugger being
	     intrusive, rely on the user to configure the requested
	     mode.  */
	  if (secure_stack_used && !exception_domain_is_secure
	      && !arm_unwind_secure_frames)
	    {
	      warning (_("Non-secure to secure stack unwinding disabled."));

	      /* Terminate any further stack unwinding.  */
	      arm_cache_set_active_sp_value (cache, tdep, 0);
	      return cache;
	    }

	  if (process_stack_used)
	    {
	      if (secure_stack_used)
		/* Secure thread (process) stack used, use PSP_S as SP.  */
		sp_regnum = tdep->m_profile_psp_s_regnum;
	      else
		/* Non-secure thread (process) stack used, use PSP_NS as SP.  */
		sp_regnum = tdep->m_profile_psp_ns_regnum;
	    }
	  else
	    {
	      if (secure_stack_used)
		/* Secure main stack used, use MSP_S as SP.  */
		sp_regnum = tdep->m_profile_msp_s_regnum;
	      else
		/* Non-secure main stack used, use MSP_NS as SP.  */
		sp_regnum = tdep->m_profile_msp_ns_regnum;
	    }
	}
      else
	{
	  if (process_stack_used)
	    /* Thread (process) stack used, use PSP as SP.  */
	    sp_regnum = tdep->m_profile_psp_regnum;
	  else
	    /* Main stack used, use MSP as SP.  */
	    sp_regnum = tdep->m_profile_msp_regnum;
	}

      /* Set the active SP regnum.  */
      arm_cache_switch_prev_sp (cache, tdep, sp_regnum);

      /* Fetch the SP to use for this frame.  */
      CORE_ADDR unwound_sp = arm_cache_get_prev_sp_value (cache, tdep);

      /* Exception entry context stacking are described in ARMv8-M (section
	 B3.19) and ARMv7-M (sections B1.5.6 and B1.5.7) Architecture Reference
	 Manuals.

	 The following figure shows the structure of the stack frame when
	 Security and Floating-point extensions are present.

			      SP Offsets
		Without                         With
	      Callee Regs                    Callee Regs
					(Secure -> Non-Secure)
			+-------------------+
		 0xA8   |                   |   0xD0
			+===================+         --+  <-- Original SP
		 0xA4   |        S31        |   0xCC    |
			+-------------------+           |
				 ...                    |  Additional FP context
			+-------------------+           |
		 0x68   |        S16        |   0x90    |
			+===================+         --+
		 0x64   |      Reserved     |   0x8C    |
			+-------------------+           |
		 0x60   |       FPSCR       |   0x88    |
			+-------------------+           |
		 0x5C   |        S15        |   0x84    |  FP context
			+-------------------+           |
				 ...                    |
			+-------------------+           |
		 0x20   |         S0        |   0x48    |
			+===================+         --+
		 0x1C   |       xPSR        |   0x44    |
			+-------------------+           |
		 0x18   |  Return address   |   0x40    |
			+-------------------+           |
		 0x14   |      LR(R14)      |   0x3C    |
			+-------------------+           |
		 0x10   |        R12        |   0x38    |  State context
			+-------------------+           |
		 0x0C   |         R3        |   0x34    |
			+-------------------+           |
				 ...                    |
			+-------------------+           |
		 0x00   |         R0        |   0x28    |
			+===================+         --+
			|        R11        |   0x24    |
			+-------------------+           |
				 ...                    |
			+-------------------+           |  Additional state
			|         R4        |   0x08    |  context when
			+-------------------+           |  transitioning from
			|      Reserved     |   0x04    |  Secure to Non-Secure
			+-------------------+           |
			|  Magic signature  |   0x00    |
			+===================+         --+  <-- New SP  */

      uint32_t sp_r0_offset = 0;

      /* With the Security extension, the hardware saves R4..R11 too.  */
      if (tdep->have_sec_ext && secure_stack_used
	  && (!default_callee_register_stacking || !exception_domain_is_secure))
	{
	  /* Read R4..R11 from the integer callee registers.  */
	  cache->saved_regs[4].set_addr (unwound_sp + 0x08);
	  cache->saved_regs[5].set_addr (unwound_sp + 0x0C);
	  cache->saved_regs[6].set_addr (unwound_sp + 0x10);
	  cache->saved_regs[7].set_addr (unwound_sp + 0x14);
	  cache->saved_regs[8].set_addr (unwound_sp + 0x18);
	  cache->saved_regs[9].set_addr (unwound_sp + 0x1C);
	  cache->saved_regs[10].set_addr (unwound_sp + 0x20);
	  cache->saved_regs[11].set_addr (unwound_sp + 0x24);
	  sp_r0_offset = 0x28;
	}

      /* The hardware saves eight 32-bit words, comprising xPSR,
	 ReturnAddress, LR (R14), R12, R3, R2, R1, R0.  See details in
	 "B1.5.6 Exception entry behavior" in
	 "ARMv7-M Architecture Reference Manual".  */
      cache->saved_regs[0].set_addr (unwound_sp + sp_r0_offset);
      cache->saved_regs[1].set_addr (unwound_sp + sp_r0_offset + 0x04);
      cache->saved_regs[2].set_addr (unwound_sp + sp_r0_offset + 0x08);
      cache->saved_regs[3].set_addr (unwound_sp + sp_r0_offset + 0x0C);
      cache->saved_regs[ARM_IP_REGNUM].set_addr (unwound_sp + sp_r0_offset
						 + 0x10);
      cache->saved_regs[ARM_LR_REGNUM].set_addr (unwound_sp + sp_r0_offset
						 + 0x14);
      cache->saved_regs[ARM_PC_REGNUM].set_addr (unwound_sp + sp_r0_offset
						 + 0x18);
      cache->saved_regs[ARM_PS_REGNUM].set_addr (unwound_sp + sp_r0_offset
						 + 0x1C);

      /* Check EXC_RETURN bit FTYPE if extended stack frame (FPU regs stored)
	 type used.  */
      bool extended_frame_used = (bit (lr, 4) == 0);
      if (extended_frame_used)
	{
	  ULONGEST fpccr;
	  ULONGEST fpcar;

	  /* Read FPCCR register.  */
	  if (!safe_read_memory_unsigned_integer (FPCCR, ARM_INT_REGISTER_SIZE,
						 byte_order, &fpccr))
	    {
	      warning (_("Could not fetch required FPCCR content.  Further "
			 "unwinding is impossible."));
	      arm_cache_set_active_sp_value (cache, tdep, 0);
	      return cache;
	    }

	  /* Read FPCAR register.  */
	  if (!safe_read_memory_unsigned_integer (FPCAR, ARM_INT_REGISTER_SIZE,
						  byte_order, &fpcar))
	    {
	      warning (_("Could not fetch FPCAR content. Further unwinding of "
			 "FP register values will be unreliable."));
	      fpcar = 0;
	    }

	  bool fpccr_aspen = bit (fpccr, 31);
	  bool fpccr_lspen = bit (fpccr, 30);
	  bool fpccr_ts = bit (fpccr, 26);
	  bool fpccr_lspact = bit (fpccr, 0);

	  /* The LSPEN and ASPEN bits indicate if the lazy state preservation
	     for FP registers is enabled or disabled.  The LSPACT bit indicate,
	     together with FPCAR, if the lazy state preservation feature is
	     active for the current frame or for another frame.
	     See "Lazy context save of FP state", in B1.5.7, also ARM AN298,
	     supported by Cortex-M4F architecture for details.  */
	  bool fpcar_points_to_this_frame = ((unwound_sp + sp_r0_offset + 0x20)
					     == (fpcar & ~0x7));
	  bool read_fp_regs_from_stack = (!(fpccr_aspen && fpccr_lspen
					    && fpccr_lspact
					    && fpcar_points_to_this_frame));

	  /* Extended stack frame type used.  */
	  if (read_fp_regs_from_stack)
	    {
	      CORE_ADDR addr = unwound_sp + sp_r0_offset + 0x20;
	      for (int i = 0; i < 8; i++)
		{
		  cache->saved_regs[ARM_D0_REGNUM + i].set_addr (addr);
		  addr += 8;
		}
	    }
	  cache->saved_regs[ARM_FPSCR_REGNUM].set_addr (unwound_sp
							+ sp_r0_offset + 0x60);

	  if (tdep->have_sec_ext && !default_callee_register_stacking
	      && fpccr_ts)
	    {
	      /* Handle floating-point callee saved registers.  */
	      if (read_fp_regs_from_stack)
		{
		  CORE_ADDR addr = unwound_sp + sp_r0_offset + 0x68;
		  for (int i = 8; i < 16; i++)
		    {
		      cache->saved_regs[ARM_D0_REGNUM + i].set_addr (addr);
		      addr += 8;
		    }
		}

	      arm_cache_set_active_sp_value (cache, tdep,
					     unwound_sp + sp_r0_offset + 0xA8);
	    }
	  else
	    {
	      /* Offset 0x64 is reserved.  */
	      arm_cache_set_active_sp_value (cache, tdep,
					     unwound_sp + sp_r0_offset + 0x68);
	    }
	}
      else
	{
	  /* Standard stack frame type used.  */
	  arm_cache_set_active_sp_value (cache, tdep,
					 unwound_sp + sp_r0_offset + 0x20);
	}

      /* If bit 9 of the saved xPSR is set, then there is a four-byte
	 aligner between the top of the 32-byte stack frame and the
	 previous context's stack pointer.  */
      ULONGEST xpsr;
      if (!safe_read_memory_unsigned_integer (cache->saved_regs[ARM_PS_REGNUM]
					      .addr (), ARM_INT_REGISTER_SIZE,
					      byte_order, &xpsr))
	{
	  warning (_("Could not fetch required XPSR content.  Further "
		     "unwinding is impossible."));
	  arm_cache_set_active_sp_value (cache, tdep, 0);
	  return cache;
	}

      if (bit (xpsr, 9) != 0)
	{
	  CORE_ADDR new_sp = arm_cache_get_prev_sp_value (cache, tdep) + 4;
	  arm_cache_set_active_sp_value (cache, tdep, new_sp);
	}

      return cache;
    }

  internal_error (_("While unwinding an exception frame, "
		    "found unexpected Link Register value "
		    "%s.  This should not happen and may "
		    "be caused by corrupt data or a bug in"
		    " GDB."),
		  phex (lr, ARM_INT_REGISTER_SIZE));
}

/* Implementation of the stop_reason hook for arm_m_exception frames.  */

static enum unwind_stop_reason
arm_m_exception_frame_unwind_stop_reason (frame_info_ptr this_frame,
					  void **this_cache)
{
  struct arm_prologue_cache *cache;
  arm_gdbarch_tdep *tdep
    = gdbarch_tdep<arm_gdbarch_tdep> (get_frame_arch (this_frame));

  if (*this_cache == NULL)
    *this_cache = arm_m_exception_cache (this_frame);
  cache = (struct arm_prologue_cache *) *this_cache;

  /* If we've hit a wall, stop.  */
  if (arm_cache_get_prev_sp_value (cache, tdep) == 0)
    return UNWIND_OUTERMOST;

  return UNWIND_NO_REASON;
}

/* Implementation of function hook 'this_id' in
   'struct frame_uwnind'.  */

static void
arm_m_exception_this_id (frame_info_ptr this_frame,
			 void **this_cache,
			 struct frame_id *this_id)
{
  struct arm_prologue_cache *cache;

  if (*this_cache == NULL)
    *this_cache = arm_m_exception_cache (this_frame);
  cache = (struct arm_prologue_cache *) *this_cache;

  /* Our frame ID for a stub frame is the current SP and LR.  */
  arm_gdbarch_tdep *tdep
    = gdbarch_tdep<arm_gdbarch_tdep> (get_frame_arch (this_frame));
  *this_id = frame_id_build (arm_cache_get_prev_sp_value (cache, tdep),
			     get_frame_pc (this_frame));
}

/* Implementation of function hook 'prev_register' in
   'struct frame_uwnind'.  */

static struct value *
arm_m_exception_prev_register (frame_info_ptr this_frame,
			       void **this_cache,
			       int prev_regnum)
{
  struct arm_prologue_cache *cache;
  CORE_ADDR sp_value;

  if (*this_cache == NULL)
    *this_cache = arm_m_exception_cache (this_frame);
  cache = (struct arm_prologue_cache *) *this_cache;

  /* The value was already reconstructed into PREV_SP.  */
  arm_gdbarch_tdep *tdep
    = gdbarch_tdep<arm_gdbarch_tdep> (get_frame_arch (this_frame));
  if (prev_regnum == ARM_SP_REGNUM)
    return frame_unwind_got_constant (this_frame, prev_regnum,
				      arm_cache_get_prev_sp_value (cache, tdep));

  /* If we are asked to unwind the PC, strip the saved T bit.  */
  if (prev_regnum == ARM_PC_REGNUM)
    {
      struct value *value = trad_frame_get_prev_register (this_frame,
							  cache->saved_regs,
							  prev_regnum);
      CORE_ADDR pc = value_as_address (value);
      return frame_unwind_got_constant (this_frame, prev_regnum,
					UNMAKE_THUMB_ADDR (pc));
    }

  /* The value might be one of the alternative SP, if so, use the
     value already constructed.  */
  if (arm_is_alternative_sp_register (tdep, prev_regnum))
    {
      sp_value = arm_cache_get_sp_register (cache, tdep, prev_regnum);
      return frame_unwind_got_constant (this_frame, prev_regnum, sp_value);
    }

  /* If we are asked to unwind the xPSR, set T bit if PC is in thumb mode.
     LR register is unreliable as it contains FNC_RETURN or EXC_RETURN
     pattern.  */
  if (prev_regnum == ARM_PS_REGNUM)
    {
      struct gdbarch *gdbarch = get_frame_arch (this_frame);
      struct value *value = trad_frame_get_prev_register (this_frame,
							  cache->saved_regs,
							  ARM_PC_REGNUM);
      CORE_ADDR pc = value_as_address (value);
      value = trad_frame_get_prev_register (this_frame, cache->saved_regs,
					    ARM_PS_REGNUM);
      ULONGEST xpsr = value_as_long (value);

      /* Reconstruct the T bit; see arm_prologue_prev_register for details.  */
      xpsr = reconstruct_t_bit (gdbarch, pc, xpsr);
      return frame_unwind_got_constant (this_frame, ARM_PS_REGNUM, xpsr);
    }

  return trad_frame_get_prev_register (this_frame, cache->saved_regs,
				       prev_regnum);
}

/* Implementation of function hook 'sniffer' in
   'struct frame_uwnind'.  */

static int
arm_m_exception_unwind_sniffer (const struct frame_unwind *self,
				frame_info_ptr this_frame,
				void **this_prologue_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  CORE_ADDR this_pc = get_frame_pc (this_frame);

  /* No need to check is_m; this sniffer is only registered for
     M-profile architectures.  */

  /* Check if exception frame returns to a magic PC value.  */
  return arm_m_addr_is_magic (gdbarch, this_pc);
}

/* Frame unwinder for M-profile exceptions (EXC_RETURN on stack),
   lockup and secure/nonsecure interstate function calls (FNC_RETURN).  */

struct frame_unwind arm_m_exception_unwind =
{
  "arm m exception lockup sec_fnc",
  SIGTRAMP_FRAME,
  arm_m_exception_frame_unwind_stop_reason,
  arm_m_exception_this_id,
  arm_m_exception_prev_register,
  NULL,
  arm_m_exception_unwind_sniffer
};

static CORE_ADDR
arm_normal_frame_base (frame_info_ptr this_frame, void **this_cache)
{
  struct arm_prologue_cache *cache;

  if (*this_cache == NULL)
    *this_cache = arm_make_prologue_cache (this_frame);
  cache = (struct arm_prologue_cache *) *this_cache;

  arm_gdbarch_tdep *tdep
    = gdbarch_tdep<arm_gdbarch_tdep> (get_frame_arch (this_frame));
  return arm_cache_get_prev_sp_value (cache, tdep) - cache->framesize;
}

struct frame_base arm_normal_base = {
  &arm_prologue_unwind,
  arm_normal_frame_base,
  arm_normal_frame_base,
  arm_normal_frame_base
};

struct arm_dwarf2_prev_register_cache
{
  /* Cached value of the corresponding stack pointer for the inner frame.  */
  CORE_ADDR sp;
  CORE_ADDR msp;
  CORE_ADDR msp_s;
  CORE_ADDR msp_ns;
  CORE_ADDR psp;
  CORE_ADDR psp_s;
  CORE_ADDR psp_ns;
};

static struct value *
arm_dwarf2_prev_register (frame_info_ptr this_frame, void **this_cache,
			  int regnum)
{
  struct gdbarch * gdbarch = get_frame_arch (this_frame);
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);
  CORE_ADDR lr;
  ULONGEST cpsr;
  arm_dwarf2_prev_register_cache *cache
    = ((arm_dwarf2_prev_register_cache *)
       dwarf2_frame_get_fn_data (this_frame, this_cache,
				 arm_dwarf2_prev_register));

  if (!cache)
    {
      const unsigned int size = sizeof (struct arm_dwarf2_prev_register_cache);
      cache = ((arm_dwarf2_prev_register_cache *)
	       dwarf2_frame_allocate_fn_data (this_frame, this_cache,
					      arm_dwarf2_prev_register, size));

      if (tdep->have_sec_ext)
	{
	  cache->sp
	    = get_frame_register_unsigned (this_frame, ARM_SP_REGNUM);

	  cache->msp_s
	    = get_frame_register_unsigned (this_frame,
					   tdep->m_profile_msp_s_regnum);
	  cache->msp_ns
	    = get_frame_register_unsigned (this_frame,
					   tdep->m_profile_msp_ns_regnum);
	  cache->psp_s
	    = get_frame_register_unsigned (this_frame,
					   tdep->m_profile_psp_s_regnum);
	  cache->psp_ns
	    = get_frame_register_unsigned (this_frame,
					   tdep->m_profile_psp_ns_regnum);
	}
      else if (tdep->is_m)
	{
	  cache->sp
	    = get_frame_register_unsigned (this_frame, ARM_SP_REGNUM);

	  cache->msp
	    = get_frame_register_unsigned (this_frame,
					   tdep->m_profile_msp_regnum);
	  cache->psp
	    = get_frame_register_unsigned (this_frame,
					   tdep->m_profile_psp_regnum);
	}
    }

  if (regnum == ARM_PC_REGNUM)
    {
      /* The PC is normally copied from the return column, which
	 describes saves of LR.  However, that version may have an
	 extra bit set to indicate Thumb state.  The bit is not
	 part of the PC.  */

      /* Record in the frame whether the return address was signed.  */
      if (tdep->have_pacbti)
	{
	  CORE_ADDR ra_auth_code
	    = frame_unwind_register_unsigned (this_frame,
					      tdep->pacbti_pseudo_base);

	  if (ra_auth_code != 0)
	    set_frame_previous_pc_masked (this_frame);
	}

      lr = frame_unwind_register_unsigned (this_frame, ARM_LR_REGNUM);
      return frame_unwind_got_constant (this_frame, regnum,
					arm_addr_bits_remove (gdbarch, lr));
    }
  else if (regnum == ARM_PS_REGNUM)
    {
      /* Reconstruct the T bit; see arm_prologue_prev_register for details.  */
      cpsr = get_frame_register_unsigned (this_frame, regnum);
      lr = frame_unwind_register_unsigned (this_frame, ARM_LR_REGNUM);
      cpsr = reconstruct_t_bit (gdbarch, lr, cpsr);
      return frame_unwind_got_constant (this_frame, regnum, cpsr);
    }
  else if (arm_is_alternative_sp_register (tdep, regnum))
    {
      /* Handle the alternative SP registers on Cortex-M.  */
      bool override_with_sp_value = false;
      CORE_ADDR val;

      if (tdep->have_sec_ext)
	{
	  bool is_msp = (regnum == tdep->m_profile_msp_regnum)
	    && (cache->msp_s == cache->sp || cache->msp_ns == cache->sp);
	  bool is_msp_s = (regnum == tdep->m_profile_msp_s_regnum)
	    && (cache->msp_s == cache->sp);
	  bool is_msp_ns = (regnum == tdep->m_profile_msp_ns_regnum)
	    && (cache->msp_ns == cache->sp);
	  bool is_psp = (regnum == tdep->m_profile_psp_regnum)
	    && (cache->psp_s == cache->sp || cache->psp_ns == cache->sp);
	  bool is_psp_s = (regnum == tdep->m_profile_psp_s_regnum)
	    && (cache->psp_s == cache->sp);
	  bool is_psp_ns = (regnum == tdep->m_profile_psp_ns_regnum)
	    && (cache->psp_ns == cache->sp);

	  override_with_sp_value = is_msp || is_msp_s || is_msp_ns
	    || is_psp || is_psp_s || is_psp_ns;

	}
      else if (tdep->is_m)
	{
	  bool is_msp = (regnum == tdep->m_profile_msp_regnum)
	    && (cache->sp == cache->msp);
	  bool is_psp = (regnum == tdep->m_profile_psp_regnum)
	    && (cache->sp == cache->psp);

	  override_with_sp_value = is_msp || is_psp;
	}

      if (override_with_sp_value)
	{
	  /* Use value of SP from previous frame.  */
	  frame_info_ptr prev_frame = get_prev_frame (this_frame);
	  if (prev_frame)
	    val = get_frame_register_unsigned (prev_frame, ARM_SP_REGNUM);
	  else
	    val = get_frame_base (this_frame);
	}
      else
	/* Use value for the register from previous frame.  */
	val = get_frame_register_unsigned (this_frame, regnum);

      return frame_unwind_got_constant (this_frame, regnum, val);
    }

  internal_error (_("Unexpected register %d"), regnum);
}

/* Implement the stack_frame_destroyed_p gdbarch method.  */

static int
thumb_stack_frame_destroyed_p (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  enum bfd_endian byte_order_for_code = gdbarch_byte_order_for_code (gdbarch);
  unsigned int insn, insn2;
  int found_return = 0, found_stack_adjust = 0;
  CORE_ADDR func_start, func_end;
  CORE_ADDR scan_pc;
  gdb_byte buf[4];

  if (!find_pc_partial_function (pc, NULL, &func_start, &func_end))
    return 0;

  /* The epilogue is a sequence of instructions along the following lines:

    - add stack frame size to SP or FP
    - [if frame pointer used] restore SP from FP
    - restore registers from SP [may include PC]
    - a return-type instruction [if PC wasn't already restored]

    In a first pass, we scan forward from the current PC and verify the
    instructions we find as compatible with this sequence, ending in a
    return instruction.

    However, this is not sufficient to distinguish indirect function calls
    within a function from indirect tail calls in the epilogue in some cases.
    Therefore, if we didn't already find any SP-changing instruction during
    forward scan, we add a backward scanning heuristic to ensure we actually
    are in the epilogue.  */

  scan_pc = pc;
  while (scan_pc < func_end && !found_return)
    {
      if (target_read_memory (scan_pc, buf, 2))
	break;

      scan_pc += 2;
      insn = extract_unsigned_integer (buf, 2, byte_order_for_code);

      if ((insn & 0xff80) == 0x4700)  /* bx <Rm> */
	found_return = 1;
      else if (insn == 0x46f7)  /* mov pc, lr */
	found_return = 1;
      else if (thumb_instruction_restores_sp (insn))
	{
	  if ((insn & 0xff00) == 0xbd00)  /* pop <registers, PC> */
	    found_return = 1;
	}
      else if (thumb_insn_size (insn) == 4)  /* 32-bit Thumb-2 instruction */
	{
	  if (target_read_memory (scan_pc, buf, 2))
	    break;

	  scan_pc += 2;
	  insn2 = extract_unsigned_integer (buf, 2, byte_order_for_code);

	  if (insn == 0xe8bd)  /* ldm.w sp!, <registers> */
	    {
	      if (insn2 & 0x8000)  /* <registers> include PC.  */
		found_return = 1;
	    }
	  else if (insn == 0xf85d  /* ldr.w <Rt>, [sp], #4 */
		   && (insn2 & 0x0fff) == 0x0b04)
	    {
	      if ((insn2 & 0xf000) == 0xf000) /* <Rt> is PC.  */
		found_return = 1;
	    }
	  else if ((insn & 0xffbf) == 0xecbd  /* vldm sp!, <list> */
		   && (insn2 & 0x0e00) == 0x0a00)
	    ;
	  else
	    break;
	}
      else
	break;
    }

  if (!found_return)
    return 0;

  /* Since any instruction in the epilogue sequence, with the possible
     exception of return itself, updates the stack pointer, we need to
     scan backwards for at most one instruction.  Try either a 16-bit or
     a 32-bit instruction.  This is just a heuristic, so we do not worry
     too much about false positives.  */

  if (pc - 4 < func_start)
    return 0;
  if (target_read_memory (pc - 4, buf, 4))
    return 0;

  insn = extract_unsigned_integer (buf, 2, byte_order_for_code);
  insn2 = extract_unsigned_integer (buf + 2, 2, byte_order_for_code);

  if (thumb_instruction_restores_sp (insn2))
    found_stack_adjust = 1;
  else if (insn == 0xe8bd)  /* ldm.w sp!, <registers> */
    found_stack_adjust = 1;
  else if (insn == 0xf85d  /* ldr.w <Rt>, [sp], #4 */
	   && (insn2 & 0x0fff) == 0x0b04)
    found_stack_adjust = 1;
  else if ((insn & 0xffbf) == 0xecbd  /* vldm sp!, <list> */
	   && (insn2 & 0x0e00) == 0x0a00)
    found_stack_adjust = 1;

  return found_stack_adjust;
}

static int
arm_stack_frame_destroyed_p_1 (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  enum bfd_endian byte_order_for_code = gdbarch_byte_order_for_code (gdbarch);
  unsigned int insn;
  int found_return;
  CORE_ADDR func_start, func_end;

  if (!find_pc_partial_function (pc, NULL, &func_start, &func_end))
    return 0;

  /* We are in the epilogue if the previous instruction was a stack
     adjustment and the next instruction is a possible return (bx, mov
     pc, or pop).  We could have to scan backwards to find the stack
     adjustment, or forwards to find the return, but this is a decent
     approximation.  First scan forwards.  */

  found_return = 0;
  insn = read_memory_unsigned_integer (pc, 4, byte_order_for_code);
  if (bits (insn, 28, 31) != INST_NV)
    {
      if ((insn & 0x0ffffff0) == 0x012fff10)
	/* BX.  */
	found_return = 1;
      else if ((insn & 0x0ffffff0) == 0x01a0f000)
	/* MOV PC.  */
	found_return = 1;
      else if ((insn & 0x0fff0000) == 0x08bd0000
	  && (insn & 0x0000c000) != 0)
	/* POP (LDMIA), including PC or LR.  */
	found_return = 1;
    }

  if (!found_return)
    return 0;

  /* Scan backwards.  This is just a heuristic, so do not worry about
     false positives from mode changes.  */

  if (pc < func_start + 4)
    return 0;

  insn = read_memory_unsigned_integer (pc - 4, 4, byte_order_for_code);
  if (arm_instruction_restores_sp (insn))
    return 1;

  return 0;
}

/* Implement the stack_frame_destroyed_p gdbarch method.  */

static int
arm_stack_frame_destroyed_p (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  if (arm_pc_is_thumb (gdbarch, pc))
    return thumb_stack_frame_destroyed_p (gdbarch, pc);
  else
    return arm_stack_frame_destroyed_p_1 (gdbarch, pc);
}

/* When arguments must be pushed onto the stack, they go on in reverse
   order.  The code below implements a FILO (stack) to do this.  */

struct arm_stack_item
{
  int len;
  struct arm_stack_item *prev;
  gdb_byte *data;
};

static struct arm_stack_item *
push_stack_item (struct arm_stack_item *prev, const gdb_byte *contents,
		 int len)
{
  struct arm_stack_item *si;
  si = XNEW (struct arm_stack_item);
  si->data = (gdb_byte *) xmalloc (len);
  si->len = len;
  si->prev = prev;
  memcpy (si->data, contents, len);
  return si;
}

static struct arm_stack_item *
pop_stack_item (struct arm_stack_item *si)
{
  struct arm_stack_item *dead = si;
  si = si->prev;
  xfree (dead->data);
  xfree (dead);
  return si;
}

/* Implement the gdbarch type alignment method, overrides the generic
   alignment algorithm for anything that is arm specific.  */

static ULONGEST
arm_type_align (gdbarch *gdbarch, struct type *t)
{
  t = check_typedef (t);
  if (t->code () == TYPE_CODE_ARRAY && t->is_vector ())
    {
      /* Use the natural alignment for vector types (the same for
	 scalar type), but the maximum alignment is 64-bit.  */
      if (t->length () > 8)
	return 8;
      else
	return t->length ();
    }

  /* Allow the common code to calculate the alignment.  */
  return 0;
}

/* Possible base types for a candidate for passing and returning in
   VFP registers.  */

enum arm_vfp_cprc_base_type
{
  VFP_CPRC_UNKNOWN,
  VFP_CPRC_SINGLE,
  VFP_CPRC_DOUBLE,
  VFP_CPRC_VEC64,
  VFP_CPRC_VEC128
};

/* The length of one element of base type B.  */

static unsigned
arm_vfp_cprc_unit_length (enum arm_vfp_cprc_base_type b)
{
  switch (b)
    {
    case VFP_CPRC_SINGLE:
      return 4;
    case VFP_CPRC_DOUBLE:
      return 8;
    case VFP_CPRC_VEC64:
      return 8;
    case VFP_CPRC_VEC128:
      return 16;
    default:
      internal_error (_("Invalid VFP CPRC type: %d."),
		      (int) b);
    }
}

/* The character ('s', 'd' or 'q') for the type of VFP register used
   for passing base type B.  */

static int
arm_vfp_cprc_reg_char (enum arm_vfp_cprc_base_type b)
{
  switch (b)
    {
    case VFP_CPRC_SINGLE:
      return 's';
    case VFP_CPRC_DOUBLE:
      return 'd';
    case VFP_CPRC_VEC64:
      return 'd';
    case VFP_CPRC_VEC128:
      return 'q';
    default:
      internal_error (_("Invalid VFP CPRC type: %d."),
		      (int) b);
    }
}

/* Determine whether T may be part of a candidate for passing and
   returning in VFP registers, ignoring the limit on the total number
   of components.  If *BASE_TYPE is VFP_CPRC_UNKNOWN, set it to the
   classification of the first valid component found; if it is not
   VFP_CPRC_UNKNOWN, all components must have the same classification
   as *BASE_TYPE.  If it is found that T contains a type not permitted
   for passing and returning in VFP registers, a type differently
   classified from *BASE_TYPE, or two types differently classified
   from each other, return -1, otherwise return the total number of
   base-type elements found (possibly 0 in an empty structure or
   array).  Vector types are not currently supported, matching the
   generic AAPCS support.  */

static int
arm_vfp_cprc_sub_candidate (struct type *t,
			    enum arm_vfp_cprc_base_type *base_type)
{
  t = check_typedef (t);
  switch (t->code ())
    {
    case TYPE_CODE_FLT:
      switch (t->length ())
	{
	case 4:
	  if (*base_type == VFP_CPRC_UNKNOWN)
	    *base_type = VFP_CPRC_SINGLE;
	  else if (*base_type != VFP_CPRC_SINGLE)
	    return -1;
	  return 1;

	case 8:
	  if (*base_type == VFP_CPRC_UNKNOWN)
	    *base_type = VFP_CPRC_DOUBLE;
	  else if (*base_type != VFP_CPRC_DOUBLE)
	    return -1;
	  return 1;

	default:
	  return -1;
	}
      break;

    case TYPE_CODE_COMPLEX:
      /* Arguments of complex T where T is one of the types float or
	 double get treated as if they are implemented as:

	 struct complexT
	 {
	   T real;
	   T imag;
	 };

      */
      switch (t->length ())
	{
	case 8:
	  if (*base_type == VFP_CPRC_UNKNOWN)
	    *base_type = VFP_CPRC_SINGLE;
	  else if (*base_type != VFP_CPRC_SINGLE)
	    return -1;
	  return 2;

	case 16:
	  if (*base_type == VFP_CPRC_UNKNOWN)
	    *base_type = VFP_CPRC_DOUBLE;
	  else if (*base_type != VFP_CPRC_DOUBLE)
	    return -1;
	  return 2;

	default:
	  return -1;
	}
      break;

    case TYPE_CODE_ARRAY:
      {
	if (t->is_vector ())
	  {
	    /* A 64-bit or 128-bit containerized vector type are VFP
	       CPRCs.  */
	    switch (t->length ())
	      {
	      case 8:
		if (*base_type == VFP_CPRC_UNKNOWN)
		  *base_type = VFP_CPRC_VEC64;
		return 1;
	      case 16:
		if (*base_type == VFP_CPRC_UNKNOWN)
		  *base_type = VFP_CPRC_VEC128;
		return 1;
	      default:
		return -1;
	      }
	  }
	else
	  {
	    int count;
	    unsigned unitlen;

	    count = arm_vfp_cprc_sub_candidate (t->target_type (),
						base_type);
	    if (count == -1)
	      return -1;
	    if (t->length () == 0)
	      {
		gdb_assert (count == 0);
		return 0;
	      }
	    else if (count == 0)
	      return -1;
	    unitlen = arm_vfp_cprc_unit_length (*base_type);
	    gdb_assert ((t->length () % unitlen) == 0);
	    return t->length () / unitlen;
	  }
      }
      break;

    case TYPE_CODE_STRUCT:
      {
	int count = 0;
	unsigned unitlen;
	int i;
	for (i = 0; i < t->num_fields (); i++)
	  {
	    int sub_count = 0;

	    if (!t->field (i).is_static ())
	      sub_count = arm_vfp_cprc_sub_candidate (t->field (i).type (),
						      base_type);
	    if (sub_count == -1)
	      return -1;
	    count += sub_count;
	  }
	if (t->length () == 0)
	  {
	    gdb_assert (count == 0);
	    return 0;
	  }
	else if (count == 0)
	  return -1;
	unitlen = arm_vfp_cprc_unit_length (*base_type);
	if (t->length () != unitlen * count)
	  return -1;
	return count;
      }

    case TYPE_CODE_UNION:
      {
	int count = 0;
	unsigned unitlen;
	int i;
	for (i = 0; i < t->num_fields (); i++)
	  {
	    int sub_count = arm_vfp_cprc_sub_candidate (t->field (i).type (),
							base_type);
	    if (sub_count == -1)
	      return -1;
	    count = (count > sub_count ? count : sub_count);
	  }
	if (t->length () == 0)
	  {
	    gdb_assert (count == 0);
	    return 0;
	  }
	else if (count == 0)
	  return -1;
	unitlen = arm_vfp_cprc_unit_length (*base_type);
	if (t->length () != unitlen * count)
	  return -1;
	return count;
      }

    default:
      break;
    }

  return -1;
}

/* Determine whether T is a VFP co-processor register candidate (CPRC)
   if passed to or returned from a non-variadic function with the VFP
   ABI in effect.  Return 1 if it is, 0 otherwise.  If it is, set
   *BASE_TYPE to the base type for T and *COUNT to the number of
   elements of that base type before returning.  */

static int
arm_vfp_call_candidate (struct type *t, enum arm_vfp_cprc_base_type *base_type,
			int *count)
{
  enum arm_vfp_cprc_base_type b = VFP_CPRC_UNKNOWN;
  int c = arm_vfp_cprc_sub_candidate (t, &b);
  if (c <= 0 || c > 4)
    return 0;
  *base_type = b;
  *count = c;
  return 1;
}

/* Return 1 if the VFP ABI should be used for passing arguments to and
   returning values from a function of type FUNC_TYPE, 0
   otherwise.  */

static int
arm_vfp_abi_for_function (struct gdbarch *gdbarch, struct type *func_type)
{
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  /* Variadic functions always use the base ABI.  Assume that functions
     without debug info are not variadic.  */
  if (func_type && check_typedef (func_type)->has_varargs ())
    return 0;

  /* The VFP ABI is only supported as a variant of AAPCS.  */
  if (tdep->arm_abi != ARM_ABI_AAPCS)
    return 0;

  return tdep->fp_model == ARM_FLOAT_VFP;
}

/* We currently only support passing parameters in integer registers, which
   conforms with GCC's default model, and VFP argument passing following
   the VFP variant of AAPCS.  Several other variants exist and
   we should probably support some of them based on the selected ABI.  */

static CORE_ADDR
arm_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
		     struct regcache *regcache, CORE_ADDR bp_addr, int nargs,
		     struct value **args, CORE_ADDR sp,
		     function_call_return_method return_method,
		     CORE_ADDR struct_addr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int argnum;
  int argreg;
  int nstack;
  struct arm_stack_item *si = NULL;
  int use_vfp_abi;
  struct type *ftype;
  unsigned vfp_regs_free = (1 << 16) - 1;
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  /* Determine the type of this function and whether the VFP ABI
     applies.  */
  ftype = check_typedef (function->type ());
  if (ftype->code () == TYPE_CODE_PTR)
    ftype = check_typedef (ftype->target_type ());
  use_vfp_abi = arm_vfp_abi_for_function (gdbarch, ftype);

  /* Set the return address.  For the ARM, the return breakpoint is
     always at BP_ADDR.  */
  if (arm_pc_is_thumb (gdbarch, bp_addr))
    bp_addr |= 1;
  regcache_cooked_write_unsigned (regcache, ARM_LR_REGNUM, bp_addr);

  /* Walk through the list of args and determine how large a temporary
     stack is required.  Need to take care here as structs may be
     passed on the stack, and we have to push them.  */
  nstack = 0;

  argreg = ARM_A1_REGNUM;
  nstack = 0;

  /* The struct_return pointer occupies the first parameter
     passing register.  */
  if (return_method == return_method_struct)
    {
      arm_debug_printf ("struct return in %s = %s",
			gdbarch_register_name (gdbarch, argreg),
			paddress (gdbarch, struct_addr));

      regcache_cooked_write_unsigned (regcache, argreg, struct_addr);
      argreg++;
    }

  for (argnum = 0; argnum < nargs; argnum++)
    {
      int len;
      struct type *arg_type;
      struct type *target_type;
      enum type_code typecode;
      const bfd_byte *val;
      int align;
      enum arm_vfp_cprc_base_type vfp_base_type;
      int vfp_base_count;
      int may_use_core_reg = 1;

      arg_type = check_typedef (args[argnum]->type ());
      len = arg_type->length ();
      target_type = arg_type->target_type ();
      typecode = arg_type->code ();
      val = args[argnum]->contents ().data ();

      align = type_align (arg_type);
      /* Round alignment up to a whole number of words.  */
      align = (align + ARM_INT_REGISTER_SIZE - 1)
		& ~(ARM_INT_REGISTER_SIZE - 1);
      /* Different ABIs have different maximum alignments.  */
      if (tdep->arm_abi == ARM_ABI_APCS)
	{
	  /* The APCS ABI only requires word alignment.  */
	  align = ARM_INT_REGISTER_SIZE;
	}
      else
	{
	  /* The AAPCS requires at most doubleword alignment.  */
	  if (align > ARM_INT_REGISTER_SIZE * 2)
	    align = ARM_INT_REGISTER_SIZE * 2;
	}

      if (use_vfp_abi
	  && arm_vfp_call_candidate (arg_type, &vfp_base_type,
				     &vfp_base_count))
	{
	  int regno;
	  int unit_length;
	  int shift;
	  unsigned mask;

	  /* Because this is a CPRC it cannot go in a core register or
	     cause a core register to be skipped for alignment.
	     Either it goes in VFP registers and the rest of this loop
	     iteration is skipped for this argument, or it goes on the
	     stack (and the stack alignment code is correct for this
	     case).  */
	  may_use_core_reg = 0;

	  unit_length = arm_vfp_cprc_unit_length (vfp_base_type);
	  shift = unit_length / 4;
	  mask = (1 << (shift * vfp_base_count)) - 1;
	  for (regno = 0; regno < 16; regno += shift)
	    if (((vfp_regs_free >> regno) & mask) == mask)
	      break;

	  if (regno < 16)
	    {
	      int reg_char;
	      int reg_scaled;
	      int i;

	      vfp_regs_free &= ~(mask << regno);
	      reg_scaled = regno / shift;
	      reg_char = arm_vfp_cprc_reg_char (vfp_base_type);
	      for (i = 0; i < vfp_base_count; i++)
		{
		  char name_buf[4];
		  int regnum;
		  if (reg_char == 'q')
		    arm_neon_quad_write (gdbarch, regcache, reg_scaled + i,
					 val + i * unit_length);
		  else
		    {
		      xsnprintf (name_buf, sizeof (name_buf), "%c%d",
				 reg_char, reg_scaled + i);
		      regnum = user_reg_map_name_to_regnum (gdbarch, name_buf,
							    strlen (name_buf));
		      regcache->cooked_write (regnum, val + i * unit_length);
		    }
		}
	      continue;
	    }
	  else
	    {
	      /* This CPRC could not go in VFP registers, so all VFP
		 registers are now marked as used.  */
	      vfp_regs_free = 0;
	    }
	}

      /* Push stack padding for doubleword alignment.  */
      if (nstack & (align - 1))
	{
	  si = push_stack_item (si, val, ARM_INT_REGISTER_SIZE);
	  nstack += ARM_INT_REGISTER_SIZE;
	}
      
      /* Doubleword aligned quantities must go in even register pairs.  */
      if (may_use_core_reg
	  && argreg <= ARM_LAST_ARG_REGNUM
	  && align > ARM_INT_REGISTER_SIZE
	  && argreg & 1)
	argreg++;

      /* If the argument is a pointer to a function, and it is a
	 Thumb function, create a LOCAL copy of the value and set
	 the THUMB bit in it.  */
      if (TYPE_CODE_PTR == typecode
	  && target_type != NULL
	  && TYPE_CODE_FUNC == check_typedef (target_type)->code ())
	{
	  CORE_ADDR regval = extract_unsigned_integer (val, len, byte_order);
	  if (arm_pc_is_thumb (gdbarch, regval))
	    {
	      bfd_byte *copy = (bfd_byte *) alloca (len);
	      store_unsigned_integer (copy, len, byte_order,
				      MAKE_THUMB_ADDR (regval));
	      val = copy;
	    }
	}

      /* Copy the argument to general registers or the stack in
	 register-sized pieces.  Large arguments are split between
	 registers and stack.  */
      while (len > 0)
	{
	  int partial_len = len < ARM_INT_REGISTER_SIZE
			    ? len : ARM_INT_REGISTER_SIZE;
	  CORE_ADDR regval
	    = extract_unsigned_integer (val, partial_len, byte_order);

	  if (may_use_core_reg && argreg <= ARM_LAST_ARG_REGNUM)
	    {
	      /* The argument is being passed in a general purpose
		 register.  */
	      arm_debug_printf ("arg %d in %s = 0x%s", argnum,
				gdbarch_register_name (gdbarch, argreg),
				phex (regval, ARM_INT_REGISTER_SIZE));

	      regcache_cooked_write_unsigned (regcache, argreg, regval);
	      argreg++;
	    }
	  else
	    {
	      gdb_byte buf[ARM_INT_REGISTER_SIZE];

	      memset (buf, 0, sizeof (buf));
	      store_unsigned_integer (buf, partial_len, byte_order, regval);

	      /* Push the arguments onto the stack.  */
	      arm_debug_printf ("arg %d @ sp + %d", argnum, nstack);
	      si = push_stack_item (si, buf, ARM_INT_REGISTER_SIZE);
	      nstack += ARM_INT_REGISTER_SIZE;
	    }
	      
	  len -= partial_len;
	  val += partial_len;
	}
    }
  /* If we have an odd number of words to push, then decrement the stack
     by one word now, so first stack argument will be dword aligned.  */
  if (nstack & 4)
    sp -= 4;

  while (si)
    {
      sp -= si->len;
      write_memory (sp, si->data, si->len);
      si = pop_stack_item (si);
    }

  /* Finally, update teh SP register.  */
  regcache_cooked_write_unsigned (regcache, ARM_SP_REGNUM, sp);

  return sp;
}


/* Always align the frame to an 8-byte boundary.  This is required on
   some platforms and harmless on the rest.  */

static CORE_ADDR
arm_frame_align (struct gdbarch *gdbarch, CORE_ADDR sp)
{
  /* Align the stack to eight bytes.  */
  return sp & ~ (CORE_ADDR) 7;
}

static void
print_fpu_flags (struct ui_file *file, int flags)
{
  if (flags & (1 << 0))
    gdb_puts ("IVO ", file);
  if (flags & (1 << 1))
    gdb_puts ("DVZ ", file);
  if (flags & (1 << 2))
    gdb_puts ("OFL ", file);
  if (flags & (1 << 3))
    gdb_puts ("UFL ", file);
  if (flags & (1 << 4))
    gdb_puts ("INX ", file);
  gdb_putc ('\n', file);
}

/* Print interesting information about the floating point processor
   (if present) or emulator.  */
static void
arm_print_float_info (struct gdbarch *gdbarch, struct ui_file *file,
		      frame_info_ptr frame, const char *args)
{
  unsigned long status = get_frame_register_unsigned (frame, ARM_FPS_REGNUM);
  int type;

  type = (status >> 24) & 127;
  if (status & (1 << 31))
    gdb_printf (file, _("Hardware FPU type %d\n"), type);
  else
    gdb_printf (file, _("Software FPU type %d\n"), type);
  /* i18n: [floating point unit] mask */
  gdb_puts (_("mask: "), file);
  print_fpu_flags (file, status >> 16);
  /* i18n: [floating point unit] flags */
  gdb_puts (_("flags: "), file);
  print_fpu_flags (file, status);
}

/* Construct the ARM extended floating point type.  */
static struct type *
arm_ext_type (struct gdbarch *gdbarch)
{
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  if (!tdep->arm_ext_type)
    {
      type_allocator alloc (gdbarch);
      tdep->arm_ext_type
	= init_float_type (alloc, -1, "builtin_type_arm_ext",
			   floatformats_arm_ext);
    }

  return tdep->arm_ext_type;
}

static struct type *
arm_neon_double_type (struct gdbarch *gdbarch)
{
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  if (tdep->neon_double_type == NULL)
    {
      struct type *t, *elem;

      t = arch_composite_type (gdbarch, "__gdb_builtin_type_neon_d",
			       TYPE_CODE_UNION);
      elem = builtin_type (gdbarch)->builtin_uint8;
      append_composite_type_field (t, "u8", init_vector_type (elem, 8));
      elem = builtin_type (gdbarch)->builtin_uint16;
      append_composite_type_field (t, "u16", init_vector_type (elem, 4));
      elem = builtin_type (gdbarch)->builtin_uint32;
      append_composite_type_field (t, "u32", init_vector_type (elem, 2));
      elem = builtin_type (gdbarch)->builtin_uint64;
      append_composite_type_field (t, "u64", elem);
      elem = builtin_type (gdbarch)->builtin_float;
      append_composite_type_field (t, "f32", init_vector_type (elem, 2));
      elem = builtin_type (gdbarch)->builtin_double;
      append_composite_type_field (t, "f64", elem);

      t->set_is_vector (true);
      t->set_name ("neon_d");
      tdep->neon_double_type = t;
    }

  return tdep->neon_double_type;
}

/* FIXME: The vector types are not correctly ordered on big-endian
   targets.  Just as s0 is the low bits of d0, d0[0] is also the low
   bits of d0 - regardless of what unit size is being held in d0.  So
   the offset of the first uint8 in d0 is 7, but the offset of the
   first float is 4.  This code works as-is for little-endian
   targets.  */

static struct type *
arm_neon_quad_type (struct gdbarch *gdbarch)
{
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  if (tdep->neon_quad_type == NULL)
    {
      struct type *t, *elem;

      t = arch_composite_type (gdbarch, "__gdb_builtin_type_neon_q",
			       TYPE_CODE_UNION);
      elem = builtin_type (gdbarch)->builtin_uint8;
      append_composite_type_field (t, "u8", init_vector_type (elem, 16));
      elem = builtin_type (gdbarch)->builtin_uint16;
      append_composite_type_field (t, "u16", init_vector_type (elem, 8));
      elem = builtin_type (gdbarch)->builtin_uint32;
      append_composite_type_field (t, "u32", init_vector_type (elem, 4));
      elem = builtin_type (gdbarch)->builtin_uint64;
      append_composite_type_field (t, "u64", init_vector_type (elem, 2));
      elem = builtin_type (gdbarch)->builtin_float;
      append_composite_type_field (t, "f32", init_vector_type (elem, 4));
      elem = builtin_type (gdbarch)->builtin_double;
      append_composite_type_field (t, "f64", init_vector_type (elem, 2));

      t->set_is_vector (true);
      t->set_name ("neon_q");
      tdep->neon_quad_type = t;
    }

  return tdep->neon_quad_type;
}

/* Return true if REGNUM is a Q pseudo register.  Return false
   otherwise.

   REGNUM is the raw register number and not a pseudo-relative register
   number.  */

static bool
is_q_pseudo (struct gdbarch *gdbarch, int regnum)
{
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  /* Q pseudo registers are available for both NEON (Q0~Q15) and
     MVE (Q0~Q7) features.  */
  if (tdep->have_q_pseudos
      && regnum >= tdep->q_pseudo_base
      && regnum < (tdep->q_pseudo_base + tdep->q_pseudo_count))
    return true;

  return false;
}

/* Return true if REGNUM is a VFP S pseudo register.  Return false
   otherwise.

   REGNUM is the raw register number and not a pseudo-relative register
   number.  */

static bool
is_s_pseudo (struct gdbarch *gdbarch, int regnum)
{
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  if (tdep->have_s_pseudos
      && regnum >= tdep->s_pseudo_base
      && regnum < (tdep->s_pseudo_base + tdep->s_pseudo_count))
    return true;

  return false;
}

/* Return true if REGNUM is a MVE pseudo register (P0).  Return false
   otherwise.

   REGNUM is the raw register number and not a pseudo-relative register
   number.  */

static bool
is_mve_pseudo (struct gdbarch *gdbarch, int regnum)
{
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  if (tdep->have_mve
      && regnum >= tdep->mve_pseudo_base
      && regnum < tdep->mve_pseudo_base + tdep->mve_pseudo_count)
    return true;

  return false;
}

/* Return true if REGNUM is a PACBTI pseudo register (ra_auth_code).  Return
   false otherwise.

   REGNUM is the raw register number and not a pseudo-relative register
   number.  */

static bool
is_pacbti_pseudo (struct gdbarch *gdbarch, int regnum)
{
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  if (tdep->have_pacbti
      && regnum >= tdep->pacbti_pseudo_base
      && regnum < tdep->pacbti_pseudo_base + tdep->pacbti_pseudo_count)
    return true;

  return false;
}

/* Return the GDB type object for the "standard" data type of data in
   register N.  */

static struct type *
arm_register_type (struct gdbarch *gdbarch, int regnum)
{
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  if (is_s_pseudo (gdbarch, regnum))
    return builtin_type (gdbarch)->builtin_float;

  if (is_q_pseudo (gdbarch, regnum))
    return arm_neon_quad_type (gdbarch);

  if (is_mve_pseudo (gdbarch, regnum))
    return builtin_type (gdbarch)->builtin_int16;

  if (is_pacbti_pseudo (gdbarch, regnum))
    return builtin_type (gdbarch)->builtin_uint32;

  /* If the target description has register information, we are only
     in this function so that we can override the types of
     double-precision registers for NEON.  */
  if (tdesc_has_registers (gdbarch_target_desc (gdbarch)))
    {
      struct type *t = tdesc_register_type (gdbarch, regnum);

      if (regnum >= ARM_D0_REGNUM && regnum < ARM_D0_REGNUM + 32
	  && t->code () == TYPE_CODE_FLT
	  && tdep->have_neon)
	return arm_neon_double_type (gdbarch);
      else
	return t;
    }

  if (regnum >= ARM_F0_REGNUM && regnum < ARM_F0_REGNUM + NUM_FREGS)
    {
      if (!tdep->have_fpa_registers)
	return builtin_type (gdbarch)->builtin_void;

      return arm_ext_type (gdbarch);
    }
  else if (regnum == ARM_SP_REGNUM)
    return builtin_type (gdbarch)->builtin_data_ptr;
  else if (regnum == ARM_PC_REGNUM)
    return builtin_type (gdbarch)->builtin_func_ptr;
  else if (regnum >= ARRAY_SIZE (arm_register_names))
    /* These registers are only supported on targets which supply
       an XML description.  */
    return builtin_type (gdbarch)->builtin_int0;
  else
    return builtin_type (gdbarch)->builtin_uint32;
}

/* Map a DWARF register REGNUM onto the appropriate GDB register
   number.  */

static int
arm_dwarf_reg_to_regnum (struct gdbarch *gdbarch, int reg)
{
  /* Core integer regs.  */
  if (reg >= 0 && reg <= 15)
    return reg;

  /* Legacy FPA encoding.  These were once used in a way which
     overlapped with VFP register numbering, so their use is
     discouraged, but GDB doesn't support the ARM toolchain
     which used them for VFP.  */
  if (reg >= 16 && reg <= 23)
    return ARM_F0_REGNUM + reg - 16;

  /* New assignments for the FPA registers.  */
  if (reg >= 96 && reg <= 103)
    return ARM_F0_REGNUM + reg - 96;

  /* WMMX register assignments.  */
  if (reg >= 104 && reg <= 111)
    return ARM_WCGR0_REGNUM + reg - 104;

  if (reg >= 112 && reg <= 127)
    return ARM_WR0_REGNUM + reg - 112;

  /* PACBTI register containing the Pointer Authentication Code.  */
  if (reg == ARM_DWARF_RA_AUTH_CODE)
    {
      arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

      if (tdep->have_pacbti)
	return tdep->pacbti_pseudo_base;

      return -1;
    }

  if (reg >= 192 && reg <= 199)
    return ARM_WC0_REGNUM + reg - 192;

  /* VFP v2 registers.  A double precision value is actually
     in d1 rather than s2, but the ABI only defines numbering
     for the single precision registers.  This will "just work"
     in GDB for little endian targets (we'll read eight bytes,
     starting in s0 and then progressing to s1), but will be
     reversed on big endian targets with VFP.  This won't
     be a problem for the new Neon quad registers; you're supposed
     to use DW_OP_piece for those.  */
  if (reg >= 64 && reg <= 95)
    {
      char name_buf[4];

      xsnprintf (name_buf, sizeof (name_buf), "s%d", reg - 64);
      return user_reg_map_name_to_regnum (gdbarch, name_buf,
					  strlen (name_buf));
    }

  /* VFP v3 / Neon registers.  This range is also used for VFP v2
     registers, except that it now describes d0 instead of s0.  */
  if (reg >= 256 && reg <= 287)
    {
      char name_buf[4];

      xsnprintf (name_buf, sizeof (name_buf), "d%d", reg - 256);
      return user_reg_map_name_to_regnum (gdbarch, name_buf,
					  strlen (name_buf));
    }

  return -1;
}

/* Map GDB internal REGNUM onto the Arm simulator register numbers.  */
static int
arm_register_sim_regno (struct gdbarch *gdbarch, int regnum)
{
  int reg = regnum;
  gdb_assert (reg >= 0 && reg < gdbarch_num_regs (gdbarch));

  if (regnum >= ARM_WR0_REGNUM && regnum <= ARM_WR15_REGNUM)
    return regnum - ARM_WR0_REGNUM + SIM_ARM_IWMMXT_COP0R0_REGNUM;

  if (regnum >= ARM_WC0_REGNUM && regnum <= ARM_WC7_REGNUM)
    return regnum - ARM_WC0_REGNUM + SIM_ARM_IWMMXT_COP1R0_REGNUM;

  if (regnum >= ARM_WCGR0_REGNUM && regnum <= ARM_WCGR7_REGNUM)
    return regnum - ARM_WCGR0_REGNUM + SIM_ARM_IWMMXT_COP1R8_REGNUM;

  if (reg < NUM_GREGS)
    return SIM_ARM_R0_REGNUM + reg;
  reg -= NUM_GREGS;

  if (reg < NUM_FREGS)
    return SIM_ARM_FP0_REGNUM + reg;
  reg -= NUM_FREGS;

  if (reg < NUM_SREGS)
    return SIM_ARM_FPS_REGNUM + reg;
  reg -= NUM_SREGS;

  internal_error (_("Bad REGNUM %d"), regnum);
}

static const unsigned char op_lit0 = DW_OP_lit0;

static void
arm_dwarf2_frame_init_reg (struct gdbarch *gdbarch, int regnum,
			   struct dwarf2_frame_state_reg *reg,
			   frame_info_ptr this_frame)
{
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  if (is_pacbti_pseudo (gdbarch, regnum))
    {
      /* Initialize RA_AUTH_CODE to zero.  */
      reg->how = DWARF2_FRAME_REG_SAVED_VAL_EXP;
      reg->loc.exp.start = &op_lit0;
      reg->loc.exp.len = 1;
      return;
    }

  if (regnum == ARM_PC_REGNUM || regnum == ARM_PS_REGNUM)
    {
      reg->how = DWARF2_FRAME_REG_FN;
      reg->loc.fn = arm_dwarf2_prev_register;
    }
  else if (regnum == ARM_SP_REGNUM)
    reg->how = DWARF2_FRAME_REG_CFA;
  else if (arm_is_alternative_sp_register (tdep, regnum))
    {
      /* Handle the alternative SP registers on Cortex-M.  */
      reg->how = DWARF2_FRAME_REG_FN;
      reg->loc.fn = arm_dwarf2_prev_register;
    }
}

/* Given BUF, which is OLD_LEN bytes ending at ENDADDR, expand
   the buffer to be NEW_LEN bytes ending at ENDADDR.  Return
   NULL if an error occurs.  BUF is freed.  */

static gdb_byte *
extend_buffer_earlier (gdb_byte *buf, CORE_ADDR endaddr,
		       int old_len, int new_len)
{
  gdb_byte *new_buf;
  int bytes_to_read = new_len - old_len;

  new_buf = (gdb_byte *) xmalloc (new_len);
  memcpy (new_buf + bytes_to_read, buf, old_len);
  xfree (buf);
  if (target_read_code (endaddr - new_len, new_buf, bytes_to_read) != 0)
    {
      xfree (new_buf);
      return NULL;
    }
  return new_buf;
}

/* An IT block is at most the 2-byte IT instruction followed by
   four 4-byte instructions.  The furthest back we must search to
   find an IT block that affects the current instruction is thus
   2 + 3 * 4 == 14 bytes.  */
#define MAX_IT_BLOCK_PREFIX 14

/* Use a quick scan if there are more than this many bytes of
   code.  */
#define IT_SCAN_THRESHOLD 32

/* Adjust a breakpoint's address to move breakpoints out of IT blocks.
   A breakpoint in an IT block may not be hit, depending on the
   condition flags.  */
static CORE_ADDR
arm_adjust_breakpoint_address (struct gdbarch *gdbarch, CORE_ADDR bpaddr)
{
  gdb_byte *buf;
  char map_type;
  CORE_ADDR boundary, func_start;
  int buf_len;
  enum bfd_endian order = gdbarch_byte_order_for_code (gdbarch);
  int i, any, last_it, last_it_count;
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  /* If we are using BKPT breakpoints, none of this is necessary.  */
  if (tdep->thumb2_breakpoint == NULL)
    return bpaddr;

  /* ARM mode does not have this problem.  */
  if (!arm_pc_is_thumb (gdbarch, bpaddr))
    return bpaddr;

  /* We are setting a breakpoint in Thumb code that could potentially
     contain an IT block.  The first step is to find how much Thumb
     code there is; we do not need to read outside of known Thumb
     sequences.  */
  map_type = arm_find_mapping_symbol (bpaddr, &boundary);
  if (map_type == 0)
    /* Thumb-2 code must have mapping symbols to have a chance.  */
    return bpaddr;

  bpaddr = gdbarch_addr_bits_remove (gdbarch, bpaddr);

  if (find_pc_partial_function (bpaddr, NULL, &func_start, NULL))
    {
      func_start = gdbarch_addr_bits_remove (gdbarch, func_start);
      if (func_start > boundary)
	boundary = func_start;
    }

  /* Search for a candidate IT instruction.  We have to do some fancy
     footwork to distinguish a real IT instruction from the second
     half of a 32-bit instruction, but there is no need for that if
     there's no candidate.  */
  buf_len = std::min (bpaddr - boundary, (CORE_ADDR) MAX_IT_BLOCK_PREFIX);
  if (buf_len == 0)
    /* No room for an IT instruction.  */
    return bpaddr;

  buf = (gdb_byte *) xmalloc (buf_len);
  if (target_read_code (bpaddr - buf_len, buf, buf_len) != 0)
    return bpaddr;
  any = 0;
  for (i = 0; i < buf_len; i += 2)
    {
      unsigned short inst1 = extract_unsigned_integer (&buf[i], 2, order);
      if ((inst1 & 0xff00) == 0xbf00 && (inst1 & 0x000f) != 0)
	{
	  any = 1;
	  break;
	}
    }

  if (any == 0)
    {
      xfree (buf);
      return bpaddr;
    }

  /* OK, the code bytes before this instruction contain at least one
     halfword which resembles an IT instruction.  We know that it's
     Thumb code, but there are still two possibilities.  Either the
     halfword really is an IT instruction, or it is the second half of
     a 32-bit Thumb instruction.  The only way we can tell is to
     scan forwards from a known instruction boundary.  */
  if (bpaddr - boundary > IT_SCAN_THRESHOLD)
    {
      int definite;

      /* There's a lot of code before this instruction.  Start with an
	 optimistic search; it's easy to recognize halfwords that can
	 not be the start of a 32-bit instruction, and use that to
	 lock on to the instruction boundaries.  */
      buf = extend_buffer_earlier (buf, bpaddr, buf_len, IT_SCAN_THRESHOLD);
      if (buf == NULL)
	return bpaddr;
      buf_len = IT_SCAN_THRESHOLD;

      definite = 0;
      for (i = 0; i < buf_len - sizeof (buf) && ! definite; i += 2)
	{
	  unsigned short inst1 = extract_unsigned_integer (&buf[i], 2, order);
	  if (thumb_insn_size (inst1) == 2)
	    {
	      definite = 1;
	      break;
	    }
	}

      /* At this point, if DEFINITE, BUF[I] is the first place we
	 are sure that we know the instruction boundaries, and it is far
	 enough from BPADDR that we could not miss an IT instruction
	 affecting BPADDR.  If ! DEFINITE, give up - start from a
	 known boundary.  */
      if (! definite)
	{
	  buf = extend_buffer_earlier (buf, bpaddr, buf_len,
				       bpaddr - boundary);
	  if (buf == NULL)
	    return bpaddr;
	  buf_len = bpaddr - boundary;
	  i = 0;
	}
    }
  else
    {
      buf = extend_buffer_earlier (buf, bpaddr, buf_len, bpaddr - boundary);
      if (buf == NULL)
	return bpaddr;
      buf_len = bpaddr - boundary;
      i = 0;
    }

  /* Scan forwards.  Find the last IT instruction before BPADDR.  */
  last_it = -1;
  last_it_count = 0;
  while (i < buf_len)
    {
      unsigned short inst1 = extract_unsigned_integer (&buf[i], 2, order);
      last_it_count--;
      if ((inst1 & 0xff00) == 0xbf00 && (inst1 & 0x000f) != 0)
	{
	  last_it = i;
	  if (inst1 & 0x0001)
	    last_it_count = 4;
	  else if (inst1 & 0x0002)
	    last_it_count = 3;
	  else if (inst1 & 0x0004)
	    last_it_count = 2;
	  else
	    last_it_count = 1;
	}
      i += thumb_insn_size (inst1);
    }

  xfree (buf);

  if (last_it == -1)
    /* There wasn't really an IT instruction after all.  */
    return bpaddr;

  if (last_it_count < 1)
    /* It was too far away.  */
    return bpaddr;

  /* This really is a trouble spot.  Move the breakpoint to the IT
     instruction.  */
  return bpaddr - buf_len + last_it;
}

/* ARM displaced stepping support.

   Generally ARM displaced stepping works as follows:

   1. When an instruction is to be single-stepped, it is first decoded by
      arm_process_displaced_insn.  Depending on the type of instruction, it is
      then copied to a scratch location, possibly in a modified form.  The
      copy_* set of functions performs such modification, as necessary.  A
      breakpoint is placed after the modified instruction in the scratch space
      to return control to GDB.  Note in particular that instructions which
      modify the PC will no longer do so after modification.

   2. The instruction is single-stepped, by setting the PC to the scratch
      location address, and resuming.  Control returns to GDB when the
      breakpoint is hit.

   3. A cleanup function (cleanup_*) is called corresponding to the copy_*
      function used for the current instruction.  This function's job is to
      put the CPU/memory state back to what it would have been if the
      instruction had been executed unmodified in its original location.  */

/* NOP instruction (mov r0, r0).  */
#define ARM_NOP				0xe1a00000
#define THUMB_NOP 0x4600

/* Helper for register reads for displaced stepping.  In particular, this
   returns the PC as it would be seen by the instruction at its original
   location.  */

ULONGEST
displaced_read_reg (regcache *regs, arm_displaced_step_copy_insn_closure *dsc,
		    int regno)
{
  ULONGEST ret;
  CORE_ADDR from = dsc->insn_addr;

  if (regno == ARM_PC_REGNUM)
    {
      /* Compute pipeline offset:
	 - When executing an ARM instruction, PC reads as the address of the
	 current instruction plus 8.
	 - When executing a Thumb instruction, PC reads as the address of the
	 current instruction plus 4.  */

      if (!dsc->is_thumb)
	from += 8;
      else
	from += 4;

      displaced_debug_printf ("read pc value %.8lx",
			      (unsigned long) from);
      return (ULONGEST) from;
    }
  else
    {
      regcache_cooked_read_unsigned (regs, regno, &ret);

      displaced_debug_printf ("read r%d value %.8lx",
			      regno, (unsigned long) ret);

      return ret;
    }
}

static int
displaced_in_arm_mode (struct regcache *regs)
{
  ULONGEST ps;
  ULONGEST t_bit = arm_psr_thumb_bit (regs->arch ());

  regcache_cooked_read_unsigned (regs, ARM_PS_REGNUM, &ps);

  return (ps & t_bit) == 0;
}

/* Write to the PC as from a branch instruction.  */

static void
branch_write_pc (regcache *regs, arm_displaced_step_copy_insn_closure *dsc,
		 ULONGEST val)
{
  if (!dsc->is_thumb)
    /* Note: If bits 0/1 are set, this branch would be unpredictable for
       architecture versions < 6.  */
    regcache_cooked_write_unsigned (regs, ARM_PC_REGNUM,
				    val & ~(ULONGEST) 0x3);
  else
    regcache_cooked_write_unsigned (regs, ARM_PC_REGNUM,
				    val & ~(ULONGEST) 0x1);
}

/* Write to the PC as from a branch-exchange instruction.  */

static void
bx_write_pc (struct regcache *regs, ULONGEST val)
{
  ULONGEST ps;
  ULONGEST t_bit = arm_psr_thumb_bit (regs->arch ());

  regcache_cooked_read_unsigned (regs, ARM_PS_REGNUM, &ps);

  if ((val & 1) == 1)
    {
      regcache_cooked_write_unsigned (regs, ARM_PS_REGNUM, ps | t_bit);
      regcache_cooked_write_unsigned (regs, ARM_PC_REGNUM, val & 0xfffffffe);
    }
  else if ((val & 2) == 0)
    {
      regcache_cooked_write_unsigned (regs, ARM_PS_REGNUM, ps & ~t_bit);
      regcache_cooked_write_unsigned (regs, ARM_PC_REGNUM, val);
    }
  else
    {
      /* Unpredictable behaviour.  Try to do something sensible (switch to ARM
	  mode, align dest to 4 bytes).  */
      warning (_("Single-stepping BX to non-word-aligned ARM instruction."));
      regcache_cooked_write_unsigned (regs, ARM_PS_REGNUM, ps & ~t_bit);
      regcache_cooked_write_unsigned (regs, ARM_PC_REGNUM, val & 0xfffffffc);
    }
}

/* Write to the PC as if from a load instruction.  */

static void
load_write_pc (regcache *regs, arm_displaced_step_copy_insn_closure *dsc,
	       ULONGEST val)
{
  if (DISPLACED_STEPPING_ARCH_VERSION >= 5)
    bx_write_pc (regs, val);
  else
    branch_write_pc (regs, dsc, val);
}

/* Write to the PC as if from an ALU instruction.  */

static void
alu_write_pc (regcache *regs, arm_displaced_step_copy_insn_closure *dsc,
	      ULONGEST val)
{
  if (DISPLACED_STEPPING_ARCH_VERSION >= 7 && !dsc->is_thumb)
    bx_write_pc (regs, val);
  else
    branch_write_pc (regs, dsc, val);
}

/* Helper for writing to registers for displaced stepping.  Writing to the PC
   has a varying effects depending on the instruction which does the write:
   this is controlled by the WRITE_PC argument.  */

void
displaced_write_reg (regcache *regs, arm_displaced_step_copy_insn_closure *dsc,
		     int regno, ULONGEST val, enum pc_write_style write_pc)
{
  if (regno == ARM_PC_REGNUM)
    {
      displaced_debug_printf ("writing pc %.8lx", (unsigned long) val);

      switch (write_pc)
	{
	case BRANCH_WRITE_PC:
	  branch_write_pc (regs, dsc, val);
	  break;

	case BX_WRITE_PC:
	  bx_write_pc (regs, val);
	  break;

	case LOAD_WRITE_PC:
	  load_write_pc (regs, dsc, val);
	  break;

	case ALU_WRITE_PC:
	  alu_write_pc (regs, dsc, val);
	  break;

	case CANNOT_WRITE_PC:
	  warning (_("Instruction wrote to PC in an unexpected way when "
		     "single-stepping"));
	  break;

	default:
	  internal_error (_("Invalid argument to displaced_write_reg"));
	}

      dsc->wrote_to_pc = 1;
    }
  else
    {
      displaced_debug_printf ("writing r%d value %.8lx",
			      regno, (unsigned long) val);
      regcache_cooked_write_unsigned (regs, regno, val);
    }
}

/* This function is used to concisely determine if an instruction INSN
   references PC.  Register fields of interest in INSN should have the
   corresponding fields of BITMASK set to 0b1111.  The function
   returns return 1 if any of these fields in INSN reference the PC
   (also 0b1111, r15), else it returns 0.  */

static int
insn_references_pc (uint32_t insn, uint32_t bitmask)
{
  uint32_t lowbit = 1;

  while (bitmask != 0)
    {
      uint32_t mask;

      for (; lowbit && (bitmask & lowbit) == 0; lowbit <<= 1)
	;

      if (!lowbit)
	break;

      mask = lowbit * 0xf;

      if ((insn & mask) == mask)
	return 1;

      bitmask &= ~mask;
    }

  return 0;
}

/* The simplest copy function.  Many instructions have the same effect no
   matter what address they are executed at: in those cases, use this.  */

static int
arm_copy_unmodified (struct gdbarch *gdbarch, uint32_t insn, const char *iname,
		     arm_displaced_step_copy_insn_closure *dsc)
{
  displaced_debug_printf ("copying insn %.8lx, opcode/class '%s' unmodified",
			  (unsigned long) insn, iname);

  dsc->modinsn[0] = insn;

  return 0;
}

static int
thumb_copy_unmodified_32bit (struct gdbarch *gdbarch, uint16_t insn1,
			     uint16_t insn2, const char *iname,
			     arm_displaced_step_copy_insn_closure *dsc)
{
  displaced_debug_printf ("copying insn %.4x %.4x, opcode/class '%s' "
			  "unmodified", insn1, insn2, iname);

  dsc->modinsn[0] = insn1;
  dsc->modinsn[1] = insn2;
  dsc->numinsns = 2;

  return 0;
}

/* Copy 16-bit Thumb(Thumb and 16-bit Thumb-2) instruction without any
   modification.  */
static int
thumb_copy_unmodified_16bit (struct gdbarch *gdbarch, uint16_t insn,
			     const char *iname,
			     arm_displaced_step_copy_insn_closure *dsc)
{
  displaced_debug_printf ("copying insn %.4x, opcode/class '%s' unmodified",
			  insn, iname);

  dsc->modinsn[0] = insn;

  return 0;
}

/* Preload instructions with immediate offset.  */

static void
cleanup_preload (struct gdbarch *gdbarch, regcache *regs,
		 arm_displaced_step_copy_insn_closure *dsc)
{
  displaced_write_reg (regs, dsc, 0, dsc->tmp[0], CANNOT_WRITE_PC);
  if (!dsc->u.preload.immed)
    displaced_write_reg (regs, dsc, 1, dsc->tmp[1], CANNOT_WRITE_PC);
}

static void
install_preload (struct gdbarch *gdbarch, struct regcache *regs,
		 arm_displaced_step_copy_insn_closure *dsc, unsigned int rn)
{
  ULONGEST rn_val;
  /* Preload instructions:

     {pli/pld} [rn, #+/-imm]
     ->
     {pli/pld} [r0, #+/-imm].  */

  dsc->tmp[0] = displaced_read_reg (regs, dsc, 0);
  rn_val = displaced_read_reg (regs, dsc, rn);
  displaced_write_reg (regs, dsc, 0, rn_val, CANNOT_WRITE_PC);
  dsc->u.preload.immed = 1;

  dsc->cleanup = &cleanup_preload;
}

static int
arm_copy_preload (struct gdbarch *gdbarch, uint32_t insn, struct regcache *regs,
		  arm_displaced_step_copy_insn_closure *dsc)
{
  unsigned int rn = bits (insn, 16, 19);

  if (!insn_references_pc (insn, 0x000f0000ul))
    return arm_copy_unmodified (gdbarch, insn, "preload", dsc);

  displaced_debug_printf ("copying preload insn %.8lx", (unsigned long) insn);

  dsc->modinsn[0] = insn & 0xfff0ffff;

  install_preload (gdbarch, regs, dsc, rn);

  return 0;
}

static int
thumb2_copy_preload (struct gdbarch *gdbarch, uint16_t insn1, uint16_t insn2,
		     regcache *regs, arm_displaced_step_copy_insn_closure *dsc)
{
  unsigned int rn = bits (insn1, 0, 3);
  unsigned int u_bit = bit (insn1, 7);
  int imm12 = bits (insn2, 0, 11);
  ULONGEST pc_val;

  if (rn != ARM_PC_REGNUM)
    return thumb_copy_unmodified_32bit (gdbarch, insn1, insn2, "preload", dsc);

  /* PC is only allowed to use in PLI (immediate,literal) Encoding T3, and
     PLD (literal) Encoding T1.  */
  displaced_debug_printf ("copying pld/pli pc (0x%x) %c imm12 %.4x",
			  (unsigned int) dsc->insn_addr, u_bit ? '+' : '-',
			  imm12);

  if (!u_bit)
    imm12 = -1 * imm12;

  /* Rewrite instruction {pli/pld} PC imm12 into:
     Prepare: tmp[0] <- r0, tmp[1] <- r1, r0 <- pc, r1 <- imm12

     {pli/pld} [r0, r1]

     Cleanup: r0 <- tmp[0], r1 <- tmp[1].  */

  dsc->tmp[0] = displaced_read_reg (regs, dsc, 0);
  dsc->tmp[1] = displaced_read_reg (regs, dsc, 1);

  pc_val = displaced_read_reg (regs, dsc, ARM_PC_REGNUM);

  displaced_write_reg (regs, dsc, 0, pc_val, CANNOT_WRITE_PC);
  displaced_write_reg (regs, dsc, 1, imm12, CANNOT_WRITE_PC);
  dsc->u.preload.immed = 0;

  /* {pli/pld} [r0, r1] */
  dsc->modinsn[0] = insn1 & 0xfff0;
  dsc->modinsn[1] = 0xf001;
  dsc->numinsns = 2;

  dsc->cleanup = &cleanup_preload;
  return 0;
}

/* Preload instructions with register offset.  */

static void
install_preload_reg(struct gdbarch *gdbarch, struct regcache *regs,
		    arm_displaced_step_copy_insn_closure *dsc, unsigned int rn,
		    unsigned int rm)
{
  ULONGEST rn_val, rm_val;

  /* Preload register-offset instructions:

     {pli/pld} [rn, rm {, shift}]
     ->
     {pli/pld} [r0, r1 {, shift}].  */

  dsc->tmp[0] = displaced_read_reg (regs, dsc, 0);
  dsc->tmp[1] = displaced_read_reg (regs, dsc, 1);
  rn_val = displaced_read_reg (regs, dsc, rn);
  rm_val = displaced_read_reg (regs, dsc, rm);
  displaced_write_reg (regs, dsc, 0, rn_val, CANNOT_WRITE_PC);
  displaced_write_reg (regs, dsc, 1, rm_val, CANNOT_WRITE_PC);
  dsc->u.preload.immed = 0;

  dsc->cleanup = &cleanup_preload;
}

static int
arm_copy_preload_reg (struct gdbarch *gdbarch, uint32_t insn,
		      struct regcache *regs,
		      arm_displaced_step_copy_insn_closure *dsc)
{
  unsigned int rn = bits (insn, 16, 19);
  unsigned int rm = bits (insn, 0, 3);


  if (!insn_references_pc (insn, 0x000f000ful))
    return arm_copy_unmodified (gdbarch, insn, "preload reg", dsc);

  displaced_debug_printf ("copying preload insn %.8lx",
			  (unsigned long) insn);

  dsc->modinsn[0] = (insn & 0xfff0fff0) | 0x1;

  install_preload_reg (gdbarch, regs, dsc, rn, rm);
  return 0;
}

/* Copy/cleanup coprocessor load and store instructions.  */

static void
cleanup_copro_load_store (struct gdbarch *gdbarch,
			  struct regcache *regs,
			  arm_displaced_step_copy_insn_closure *dsc)
{
  ULONGEST rn_val = displaced_read_reg (regs, dsc, 0);

  displaced_write_reg (regs, dsc, 0, dsc->tmp[0], CANNOT_WRITE_PC);

  if (dsc->u.ldst.writeback)
    displaced_write_reg (regs, dsc, dsc->u.ldst.rn, rn_val, LOAD_WRITE_PC);
}

static void
install_copro_load_store (struct gdbarch *gdbarch, struct regcache *regs,
			  arm_displaced_step_copy_insn_closure *dsc,
			  int writeback, unsigned int rn)
{
  ULONGEST rn_val;

  /* Coprocessor load/store instructions:

     {stc/stc2} [<Rn>, #+/-imm]  (and other immediate addressing modes)
     ->
     {stc/stc2} [r0, #+/-imm].

     ldc/ldc2 are handled identically.  */

  dsc->tmp[0] = displaced_read_reg (regs, dsc, 0);
  rn_val = displaced_read_reg (regs, dsc, rn);
  /* PC should be 4-byte aligned.  */
  rn_val = rn_val & 0xfffffffc;
  displaced_write_reg (regs, dsc, 0, rn_val, CANNOT_WRITE_PC);

  dsc->u.ldst.writeback = writeback;
  dsc->u.ldst.rn = rn;

  dsc->cleanup = &cleanup_copro_load_store;
}

static int
arm_copy_copro_load_store (struct gdbarch *gdbarch, uint32_t insn,
			   struct regcache *regs,
			   arm_displaced_step_copy_insn_closure *dsc)
{
  unsigned int rn = bits (insn, 16, 19);

  if (!insn_references_pc (insn, 0x000f0000ul))
    return arm_copy_unmodified (gdbarch, insn, "copro load/store", dsc);

  displaced_debug_printf ("copying coprocessor load/store insn %.8lx",
			  (unsigned long) insn);

  dsc->modinsn[0] = insn & 0xfff0ffff;

  install_copro_load_store (gdbarch, regs, dsc, bit (insn, 25), rn);

  return 0;
}

static int
thumb2_copy_copro_load_store (struct gdbarch *gdbarch, uint16_t insn1,
			      uint16_t insn2, struct regcache *regs,
			      arm_displaced_step_copy_insn_closure *dsc)
{
  unsigned int rn = bits (insn1, 0, 3);

  if (rn != ARM_PC_REGNUM)
    return thumb_copy_unmodified_32bit (gdbarch, insn1, insn2,
					"copro load/store", dsc);

  displaced_debug_printf ("copying coprocessor load/store insn %.4x%.4x",
			  insn1, insn2);

  dsc->modinsn[0] = insn1 & 0xfff0;
  dsc->modinsn[1] = insn2;
  dsc->numinsns = 2;

  /* This function is called for copying instruction LDC/LDC2/VLDR, which
     doesn't support writeback, so pass 0.  */
  install_copro_load_store (gdbarch, regs, dsc, 0, rn);

  return 0;
}

/* Clean up branch instructions (actually perform the branch, by setting
   PC).  */

static void
cleanup_branch (struct gdbarch *gdbarch, struct regcache *regs,
		arm_displaced_step_copy_insn_closure *dsc)
{
  uint32_t status = displaced_read_reg (regs, dsc, ARM_PS_REGNUM);
  int branch_taken = condition_true (dsc->u.branch.cond, status);
  enum pc_write_style write_pc = dsc->u.branch.exchange
				 ? BX_WRITE_PC : BRANCH_WRITE_PC;

  if (!branch_taken)
    return;

  if (dsc->u.branch.link)
    {
      /* The value of LR should be the next insn of current one.  In order
       not to confuse logic handling later insn `bx lr', if current insn mode
       is Thumb, the bit 0 of LR value should be set to 1.  */
      ULONGEST next_insn_addr = dsc->insn_addr + dsc->insn_size;

      if (dsc->is_thumb)
	next_insn_addr |= 0x1;

      displaced_write_reg (regs, dsc, ARM_LR_REGNUM, next_insn_addr,
			   CANNOT_WRITE_PC);
    }

  displaced_write_reg (regs, dsc, ARM_PC_REGNUM, dsc->u.branch.dest, write_pc);
}

/* Copy B/BL/BLX instructions with immediate destinations.  */

static void
install_b_bl_blx (struct gdbarch *gdbarch, struct regcache *regs,
		  arm_displaced_step_copy_insn_closure *dsc,
		  unsigned int cond, int exchange, int link, long offset)
{
  /* Implement "BL<cond> <label>" as:

     Preparation: cond <- instruction condition
     Insn: mov r0, r0  (nop)
     Cleanup: if (condition true) { r14 <- pc; pc <- label }.

     B<cond> similar, but don't set r14 in cleanup.  */

  dsc->u.branch.cond = cond;
  dsc->u.branch.link = link;
  dsc->u.branch.exchange = exchange;

  dsc->u.branch.dest = dsc->insn_addr;
  if (link && exchange)
    /* For BLX, offset is computed from the Align (PC, 4).  */
    dsc->u.branch.dest = dsc->u.branch.dest & 0xfffffffc;

  if (dsc->is_thumb)
    dsc->u.branch.dest += 4 + offset;
  else
    dsc->u.branch.dest += 8 + offset;

  dsc->cleanup = &cleanup_branch;
}
static int
arm_copy_b_bl_blx (struct gdbarch *gdbarch, uint32_t insn,
		   regcache *regs, arm_displaced_step_copy_insn_closure *dsc)
{
  unsigned int cond = bits (insn, 28, 31);
  int exchange = (cond == 0xf);
  int link = exchange || bit (insn, 24);
  long offset;

  displaced_debug_printf ("copying %s immediate insn %.8lx",
			  (exchange) ? "blx" : (link) ? "bl" : "b",
			  (unsigned long) insn);
  if (exchange)
    /* For BLX, set bit 0 of the destination.  The cleanup_branch function will
       then arrange the switch into Thumb mode.  */
    offset = (bits (insn, 0, 23) << 2) | (bit (insn, 24) << 1) | 1;
  else
    offset = bits (insn, 0, 23) << 2;

  if (bit (offset, 25))
    offset = offset | ~0x3ffffff;

  dsc->modinsn[0] = ARM_NOP;

  install_b_bl_blx (gdbarch, regs, dsc, cond, exchange, link, offset);
  return 0;
}

static int
thumb2_copy_b_bl_blx (struct gdbarch *gdbarch, uint16_t insn1,
		      uint16_t insn2, struct regcache *regs,
		      arm_displaced_step_copy_insn_closure *dsc)
{
  int link = bit (insn2, 14);
  int exchange = link && !bit (insn2, 12);
  int cond = INST_AL;
  long offset = 0;
  int j1 = bit (insn2, 13);
  int j2 = bit (insn2, 11);
  int s = sbits (insn1, 10, 10);
  int i1 = !(j1 ^ bit (insn1, 10));
  int i2 = !(j2 ^ bit (insn1, 10));

  if (!link && !exchange) /* B */
    {
      offset = (bits (insn2, 0, 10) << 1);
      if (bit (insn2, 12)) /* Encoding T4 */
	{
	  offset |= (bits (insn1, 0, 9) << 12)
	    | (i2 << 22)
	    | (i1 << 23)
	    | (s << 24);
	  cond = INST_AL;
	}
      else /* Encoding T3 */
	{
	  offset |= (bits (insn1, 0, 5) << 12)
	    | (j1 << 18)
	    | (j2 << 19)
	    | (s << 20);
	  cond = bits (insn1, 6, 9);
	}
    }
  else
    {
      offset = (bits (insn1, 0, 9) << 12);
      offset |= ((i2 << 22) | (i1 << 23) | (s << 24));
      offset |= exchange ?
	(bits (insn2, 1, 10) << 2) : (bits (insn2, 0, 10) << 1);
    }

  displaced_debug_printf ("copying %s insn %.4x %.4x with offset %.8lx",
			  link ? (exchange) ? "blx" : "bl" : "b",
			  insn1, insn2, offset);

  dsc->modinsn[0] = THUMB_NOP;

  install_b_bl_blx (gdbarch, regs, dsc, cond, exchange, link, offset);
  return 0;
}

/* Copy B Thumb instructions.  */
static int
thumb_copy_b (struct gdbarch *gdbarch, uint16_t insn,
	      arm_displaced_step_copy_insn_closure *dsc)
{
  unsigned int cond = 0;
  int offset = 0;
  unsigned short bit_12_15 = bits (insn, 12, 15);
  CORE_ADDR from = dsc->insn_addr;

  if (bit_12_15 == 0xd)
    {
      /* offset = SignExtend (imm8:0, 32) */
      offset = sbits ((insn << 1), 0, 8);
      cond = bits (insn, 8, 11);
    }
  else if (bit_12_15 == 0xe) /* Encoding T2 */
    {
      offset = sbits ((insn << 1), 0, 11);
      cond = INST_AL;
    }

  displaced_debug_printf ("copying b immediate insn %.4x with offset %d",
			  insn, offset);

  dsc->u.branch.cond = cond;
  dsc->u.branch.link = 0;
  dsc->u.branch.exchange = 0;
  dsc->u.branch.dest = from + 4 + offset;

  dsc->modinsn[0] = THUMB_NOP;

  dsc->cleanup = &cleanup_branch;

  return 0;
}

/* Copy BX/BLX with register-specified destinations.  */

static void
install_bx_blx_reg (struct gdbarch *gdbarch, struct regcache *regs,
		    arm_displaced_step_copy_insn_closure *dsc, int link,
		    unsigned int cond, unsigned int rm)
{
  /* Implement {BX,BLX}<cond> <reg>" as:

     Preparation: cond <- instruction condition
     Insn: mov r0, r0 (nop)
     Cleanup: if (condition true) { r14 <- pc; pc <- dest; }.

     Don't set r14 in cleanup for BX.  */

  dsc->u.branch.dest = displaced_read_reg (regs, dsc, rm);

  dsc->u.branch.cond = cond;
  dsc->u.branch.link = link;

  dsc->u.branch.exchange = 1;

  dsc->cleanup = &cleanup_branch;
}

static int
arm_copy_bx_blx_reg (struct gdbarch *gdbarch, uint32_t insn,
		     regcache *regs, arm_displaced_step_copy_insn_closure *dsc)
{
  unsigned int cond = bits (insn, 28, 31);
  /* BX:  x12xxx1x
     BLX: x12xxx3x.  */
  int link = bit (insn, 5);
  unsigned int rm = bits (insn, 0, 3);

  displaced_debug_printf ("copying insn %.8lx", (unsigned long) insn);

  dsc->modinsn[0] = ARM_NOP;

  install_bx_blx_reg (gdbarch, regs, dsc, link, cond, rm);
  return 0;
}

static int
thumb_copy_bx_blx_reg (struct gdbarch *gdbarch, uint16_t insn,
		       struct regcache *regs,
		       arm_displaced_step_copy_insn_closure *dsc)
{
  int link = bit (insn, 7);
  unsigned int rm = bits (insn, 3, 6);

  displaced_debug_printf ("copying insn %.4x", (unsigned short) insn);

  dsc->modinsn[0] = THUMB_NOP;

  install_bx_blx_reg (gdbarch, regs, dsc, link, INST_AL, rm);

  return 0;
}


/* Copy/cleanup arithmetic/logic instruction with immediate RHS.  */

static void
cleanup_alu_imm (struct gdbarch *gdbarch,
		 regcache *regs, arm_displaced_step_copy_insn_closure *dsc)
{
  ULONGEST rd_val = displaced_read_reg (regs, dsc, 0);
  displaced_write_reg (regs, dsc, 0, dsc->tmp[0], CANNOT_WRITE_PC);
  displaced_write_reg (regs, dsc, 1, dsc->tmp[1], CANNOT_WRITE_PC);
  displaced_write_reg (regs, dsc, dsc->rd, rd_val, ALU_WRITE_PC);
}

static int
arm_copy_alu_imm (struct gdbarch *gdbarch, uint32_t insn, struct regcache *regs,
		  arm_displaced_step_copy_insn_closure *dsc)
{
  unsigned int rn = bits (insn, 16, 19);
  unsigned int rd = bits (insn, 12, 15);
  unsigned int op = bits (insn, 21, 24);
  int is_mov = (op == 0xd);
  ULONGEST rd_val, rn_val;

  if (!insn_references_pc (insn, 0x000ff000ul))
    return arm_copy_unmodified (gdbarch, insn, "ALU immediate", dsc);

  displaced_debug_printf ("copying immediate %s insn %.8lx",
			  is_mov ? "move" : "ALU",
			  (unsigned long) insn);

  /* Instruction is of form:

     <op><cond> rd, [rn,] #imm

     Rewrite as:

     Preparation: tmp1, tmp2 <- r0, r1;
		  r0, r1 <- rd, rn
     Insn: <op><cond> r0, r1, #imm
     Cleanup: rd <- r0; r0 <- tmp1; r1 <- tmp2
  */

  dsc->tmp[0] = displaced_read_reg (regs, dsc, 0);
  dsc->tmp[1] = displaced_read_reg (regs, dsc, 1);
  rn_val = displaced_read_reg (regs, dsc, rn);
  rd_val = displaced_read_reg (regs, dsc, rd);
  displaced_write_reg (regs, dsc, 0, rd_val, CANNOT_WRITE_PC);
  displaced_write_reg (regs, dsc, 1, rn_val, CANNOT_WRITE_PC);
  dsc->rd = rd;

  if (is_mov)
    dsc->modinsn[0] = insn & 0xfff00fff;
  else
    dsc->modinsn[0] = (insn & 0xfff00fff) | 0x10000;

  dsc->cleanup = &cleanup_alu_imm;

  return 0;
}

static int
thumb2_copy_alu_imm (struct gdbarch *gdbarch, uint16_t insn1,
		     uint16_t insn2, struct regcache *regs,
		     arm_displaced_step_copy_insn_closure *dsc)
{
  unsigned int op = bits (insn1, 5, 8);
  unsigned int rn, rm, rd;
  ULONGEST rd_val, rn_val;

  rn = bits (insn1, 0, 3); /* Rn */
  rm = bits (insn2, 0, 3); /* Rm */
  rd = bits (insn2, 8, 11); /* Rd */

  /* This routine is only called for instruction MOV.  */
  gdb_assert (op == 0x2 && rn == 0xf);

  if (rm != ARM_PC_REGNUM && rd != ARM_PC_REGNUM)
    return thumb_copy_unmodified_32bit (gdbarch, insn1, insn2, "ALU imm", dsc);

  displaced_debug_printf ("copying reg %s insn %.4x%.4x", "ALU", insn1, insn2);

  /* Instruction is of form:

     <op><cond> rd, [rn,] #imm

     Rewrite as:

     Preparation: tmp1, tmp2 <- r0, r1;
		  r0, r1 <- rd, rn
     Insn: <op><cond> r0, r1, #imm
     Cleanup: rd <- r0; r0 <- tmp1; r1 <- tmp2
  */

  dsc->tmp[0] = displaced_read_reg (regs, dsc, 0);
  dsc->tmp[1] = displaced_read_reg (regs, dsc, 1);
  rn_val = displaced_read_reg (regs, dsc, rn);
  rd_val = displaced_read_reg (regs, dsc, rd);
  displaced_write_reg (regs, dsc, 0, rd_val, CANNOT_WRITE_PC);
  displaced_write_reg (regs, dsc, 1, rn_val, CANNOT_WRITE_PC);
  dsc->rd = rd;

  dsc->modinsn[0] = insn1;
  dsc->modinsn[1] = ((insn2 & 0xf0f0) | 0x1);
  dsc->numinsns = 2;

  dsc->cleanup = &cleanup_alu_imm;

  return 0;
}

/* Copy/cleanup arithmetic/logic insns with register RHS.  */

static void
cleanup_alu_reg (struct gdbarch *gdbarch,
		 regcache *regs, arm_displaced_step_copy_insn_closure *dsc)
{
  ULONGEST rd_val;
  int i;

  rd_val = displaced_read_reg (regs, dsc, 0);

  for (i = 0; i < 3; i++)
    displaced_write_reg (regs, dsc, i, dsc->tmp[i], CANNOT_WRITE_PC);

  displaced_write_reg (regs, dsc, dsc->rd, rd_val, ALU_WRITE_PC);
}

static void
install_alu_reg (struct gdbarch *gdbarch, struct regcache *regs,
		 arm_displaced_step_copy_insn_closure *dsc,
		 unsigned int rd, unsigned int rn, unsigned int rm)
{
  ULONGEST rd_val, rn_val, rm_val;

  /* Instruction is of form:

     <op><cond> rd, [rn,] rm [, <shift>]

     Rewrite as:

     Preparation: tmp1, tmp2, tmp3 <- r0, r1, r2;
		  r0, r1, r2 <- rd, rn, rm
     Insn: <op><cond> r0, [r1,] r2 [, <shift>]
     Cleanup: rd <- r0; r0, r1, r2 <- tmp1, tmp2, tmp3
  */

  dsc->tmp[0] = displaced_read_reg (regs, dsc, 0);
  dsc->tmp[1] = displaced_read_reg (regs, dsc, 1);
  dsc->tmp[2] = displaced_read_reg (regs, dsc, 2);
  rd_val = displaced_read_reg (regs, dsc, rd);
  rn_val = displaced_read_reg (regs, dsc, rn);
  rm_val = displaced_read_reg (regs, dsc, rm);
  displaced_write_reg (regs, dsc, 0, rd_val, CANNOT_WRITE_PC);
  displaced_write_reg (regs, dsc, 1, rn_val, CANNOT_WRITE_PC);
  displaced_write_reg (regs, dsc, 2, rm_val, CANNOT_WRITE_PC);
  dsc->rd = rd;

  dsc->cleanup = &cleanup_alu_reg;
}

static int
arm_copy_alu_reg (struct gdbarch *gdbarch, uint32_t insn, struct regcache *regs,
		  arm_displaced_step_copy_insn_closure *dsc)
{
  unsigned int op = bits (insn, 21, 24);
  int is_mov = (op == 0xd);

  if (!insn_references_pc (insn, 0x000ff00ful))
    return arm_copy_unmodified (gdbarch, insn, "ALU reg", dsc);

  displaced_debug_printf ("copying reg %s insn %.8lx",
			  is_mov ? "move" : "ALU", (unsigned long) insn);

  if (is_mov)
    dsc->modinsn[0] = (insn & 0xfff00ff0) | 0x2;
  else
    dsc->modinsn[0] = (insn & 0xfff00ff0) | 0x10002;

  install_alu_reg (gdbarch, regs, dsc, bits (insn, 12, 15), bits (insn, 16, 19),
		   bits (insn, 0, 3));
  return 0;
}

static int
thumb_copy_alu_reg (struct gdbarch *gdbarch, uint16_t insn,
		    struct regcache *regs,
		    arm_displaced_step_copy_insn_closure *dsc)
{
  unsigned rm, rd;

  rm = bits (insn, 3, 6);
  rd = (bit (insn, 7) << 3) | bits (insn, 0, 2);

  if (rd != ARM_PC_REGNUM && rm != ARM_PC_REGNUM)
    return thumb_copy_unmodified_16bit (gdbarch, insn, "ALU reg", dsc);

  displaced_debug_printf ("copying ALU reg insn %.4x", (unsigned short) insn);

  dsc->modinsn[0] = ((insn & 0xff00) | 0x10);

  install_alu_reg (gdbarch, regs, dsc, rd, rd, rm);

  return 0;
}

/* Cleanup/copy arithmetic/logic insns with shifted register RHS.  */

static void
cleanup_alu_shifted_reg (struct gdbarch *gdbarch,
			 struct regcache *regs,
			 arm_displaced_step_copy_insn_closure *dsc)
{
  ULONGEST rd_val = displaced_read_reg (regs, dsc, 0);
  int i;

  for (i = 0; i < 4; i++)
    displaced_write_reg (regs, dsc, i, dsc->tmp[i], CANNOT_WRITE_PC);

  displaced_write_reg (regs, dsc, dsc->rd, rd_val, ALU_WRITE_PC);
}

static void
install_alu_shifted_reg (struct gdbarch *gdbarch, struct regcache *regs,
			 arm_displaced_step_copy_insn_closure *dsc,
			 unsigned int rd, unsigned int rn, unsigned int rm,
			 unsigned rs)
{
  int i;
  ULONGEST rd_val, rn_val, rm_val, rs_val;

  /* Instruction is of form:

     <op><cond> rd, [rn,] rm, <shift> rs

     Rewrite as:

     Preparation: tmp1, tmp2, tmp3, tmp4 <- r0, r1, r2, r3
		  r0, r1, r2, r3 <- rd, rn, rm, rs
     Insn: <op><cond> r0, r1, r2, <shift> r3
     Cleanup: tmp5 <- r0
	      r0, r1, r2, r3 <- tmp1, tmp2, tmp3, tmp4
	      rd <- tmp5
  */

  for (i = 0; i < 4; i++)
    dsc->tmp[i] = displaced_read_reg (regs, dsc, i);

  rd_val = displaced_read_reg (regs, dsc, rd);
  rn_val = displaced_read_reg (regs, dsc, rn);
  rm_val = displaced_read_reg (regs, dsc, rm);
  rs_val = displaced_read_reg (regs, dsc, rs);
  displaced_write_reg (regs, dsc, 0, rd_val, CANNOT_WRITE_PC);
  displaced_write_reg (regs, dsc, 1, rn_val, CANNOT_WRITE_PC);
  displaced_write_reg (regs, dsc, 2, rm_val, CANNOT_WRITE_PC);
  displaced_write_reg (regs, dsc, 3, rs_val, CANNOT_WRITE_PC);
  dsc->rd = rd;
  dsc->cleanup = &cleanup_alu_shifted_reg;
}

static int
arm_copy_alu_shifted_reg (struct gdbarch *gdbarch, uint32_t insn,
			  struct regcache *regs,
			  arm_displaced_step_copy_insn_closure *dsc)
{
  unsigned int op = bits (insn, 21, 24);
  int is_mov = (op == 0xd);
  unsigned int rd, rn, rm, rs;

  if (!insn_references_pc (insn, 0x000fff0ful))
    return arm_copy_unmodified (gdbarch, insn, "ALU shifted reg", dsc);

  displaced_debug_printf ("copying shifted reg %s insn %.8lx",
			  is_mov ? "move" : "ALU",
			  (unsigned long) insn);

  rn = bits (insn, 16, 19);
  rm = bits (insn, 0, 3);
  rs = bits (insn, 8, 11);
  rd = bits (insn, 12, 15);

  if (is_mov)
    dsc->modinsn[0] = (insn & 0xfff000f0) | 0x302;
  else
    dsc->modinsn[0] = (insn & 0xfff000f0) | 0x10302;

  install_alu_shifted_reg (gdbarch, regs, dsc, rd, rn, rm, rs);

  return 0;
}

/* Clean up load instructions.  */

static void
cleanup_load (struct gdbarch *gdbarch, struct regcache *regs,
	      arm_displaced_step_copy_insn_closure *dsc)
{
  ULONGEST rt_val, rt_val2 = 0, rn_val;

  rt_val = displaced_read_reg (regs, dsc, 0);
  if (dsc->u.ldst.xfersize == 8)
    rt_val2 = displaced_read_reg (regs, dsc, 1);
  rn_val = displaced_read_reg (regs, dsc, 2);

  displaced_write_reg (regs, dsc, 0, dsc->tmp[0], CANNOT_WRITE_PC);
  if (dsc->u.ldst.xfersize > 4)
    displaced_write_reg (regs, dsc, 1, dsc->tmp[1], CANNOT_WRITE_PC);
  displaced_write_reg (regs, dsc, 2, dsc->tmp[2], CANNOT_WRITE_PC);
  if (!dsc->u.ldst.immed)
    displaced_write_reg (regs, dsc, 3, dsc->tmp[3], CANNOT_WRITE_PC);

  /* Handle register writeback.  */
  if (dsc->u.ldst.writeback)
    displaced_write_reg (regs, dsc, dsc->u.ldst.rn, rn_val, CANNOT_WRITE_PC);
  /* Put result in right place.  */
  displaced_write_reg (regs, dsc, dsc->rd, rt_val, LOAD_WRITE_PC);
  if (dsc->u.ldst.xfersize == 8)
    displaced_write_reg (regs, dsc, dsc->rd + 1, rt_val2, LOAD_WRITE_PC);
}

/* Clean up store instructions.  */

static void
cleanup_store (struct gdbarch *gdbarch, struct regcache *regs,
	       arm_displaced_step_copy_insn_closure *dsc)
{
  ULONGEST rn_val = displaced_read_reg (regs, dsc, 2);

  displaced_write_reg (regs, dsc, 0, dsc->tmp[0], CANNOT_WRITE_PC);
  if (dsc->u.ldst.xfersize > 4)
    displaced_write_reg (regs, dsc, 1, dsc->tmp[1], CANNOT_WRITE_PC);
  displaced_write_reg (regs, dsc, 2, dsc->tmp[2], CANNOT_WRITE_PC);
  if (!dsc->u.ldst.immed)
    displaced_write_reg (regs, dsc, 3, dsc->tmp[3], CANNOT_WRITE_PC);
  if (!dsc->u.ldst.restore_r4)
    displaced_write_reg (regs, dsc, 4, dsc->tmp[4], CANNOT_WRITE_PC);

  /* Writeback.  */
  if (dsc->u.ldst.writeback)
    displaced_write_reg (regs, dsc, dsc->u.ldst.rn, rn_val, CANNOT_WRITE_PC);
}

/* Copy "extra" load/store instructions.  These are halfword/doubleword
   transfers, which have a different encoding to byte/word transfers.  */

static int
arm_copy_extra_ld_st (struct gdbarch *gdbarch, uint32_t insn, int unprivileged,
		      regcache *regs, arm_displaced_step_copy_insn_closure *dsc)
{
  unsigned int op1 = bits (insn, 20, 24);
  unsigned int op2 = bits (insn, 5, 6);
  unsigned int rt = bits (insn, 12, 15);
  unsigned int rn = bits (insn, 16, 19);
  unsigned int rm = bits (insn, 0, 3);
  char load[12]     = {0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1};
  char bytesize[12] = {2, 2, 2, 2, 8, 1, 8, 1, 8, 2, 8, 2};
  int immed = (op1 & 0x4) != 0;
  int opcode;
  ULONGEST rt_val, rt_val2 = 0, rn_val, rm_val = 0;

  if (!insn_references_pc (insn, 0x000ff00ful))
    return arm_copy_unmodified (gdbarch, insn, "extra load/store", dsc);

  displaced_debug_printf ("copying %sextra load/store insn %.8lx",
			  unprivileged ? "unprivileged " : "",
			  (unsigned long) insn);

  opcode = ((op2 << 2) | (op1 & 0x1) | ((op1 & 0x4) >> 1)) - 4;

  if (opcode < 0)
    internal_error (_("copy_extra_ld_st: instruction decode error"));

  dsc->tmp[0] = displaced_read_reg (regs, dsc, 0);
  dsc->tmp[1] = displaced_read_reg (regs, dsc, 1);
  dsc->tmp[2] = displaced_read_reg (regs, dsc, 2);
  if (!immed)
    dsc->tmp[3] = displaced_read_reg (regs, dsc, 3);

  rt_val = displaced_read_reg (regs, dsc, rt);
  if (bytesize[opcode] == 8)
    rt_val2 = displaced_read_reg (regs, dsc, rt + 1);
  rn_val = displaced_read_reg (regs, dsc, rn);
  if (!immed)
    rm_val = displaced_read_reg (regs, dsc, rm);

  displaced_write_reg (regs, dsc, 0, rt_val, CANNOT_WRITE_PC);
  if (bytesize[opcode] == 8)
    displaced_write_reg (regs, dsc, 1, rt_val2, CANNOT_WRITE_PC);
  displaced_write_reg (regs, dsc, 2, rn_val, CANNOT_WRITE_PC);
  if (!immed)
    displaced_write_reg (regs, dsc, 3, rm_val, CANNOT_WRITE_PC);

  dsc->rd = rt;
  dsc->u.ldst.xfersize = bytesize[opcode];
  dsc->u.ldst.rn = rn;
  dsc->u.ldst.immed = immed;
  dsc->u.ldst.writeback = bit (insn, 24) == 0 || bit (insn, 21) != 0;
  dsc->u.ldst.restore_r4 = 0;

  if (immed)
    /* {ldr,str}<width><cond> rt, [rt2,] [rn, #imm]
	->
       {ldr,str}<width><cond> r0, [r1,] [r2, #imm].  */
    dsc->modinsn[0] = (insn & 0xfff00fff) | 0x20000;
  else
    /* {ldr,str}<width><cond> rt, [rt2,] [rn, +/-rm]
	->
       {ldr,str}<width><cond> r0, [r1,] [r2, +/-r3].  */
    dsc->modinsn[0] = (insn & 0xfff00ff0) | 0x20003;

  dsc->cleanup = load[opcode] ? &cleanup_load : &cleanup_store;

  return 0;
}

/* Copy byte/half word/word loads and stores.  */

static void
install_load_store (struct gdbarch *gdbarch, struct regcache *regs,
		    arm_displaced_step_copy_insn_closure *dsc, int load,
		    int immed, int writeback, int size, int usermode,
		    int rt, int rm, int rn)
{
  ULONGEST rt_val, rn_val, rm_val = 0;

  dsc->tmp[0] = displaced_read_reg (regs, dsc, 0);
  dsc->tmp[2] = displaced_read_reg (regs, dsc, 2);
  if (!immed)
    dsc->tmp[3] = displaced_read_reg (regs, dsc, 3);
  if (!load)
    dsc->tmp[4] = displaced_read_reg (regs, dsc, 4);

  rt_val = displaced_read_reg (regs, dsc, rt);
  rn_val = displaced_read_reg (regs, dsc, rn);
  if (!immed)
    rm_val = displaced_read_reg (regs, dsc, rm);

  displaced_write_reg (regs, dsc, 0, rt_val, CANNOT_WRITE_PC);
  displaced_write_reg (regs, dsc, 2, rn_val, CANNOT_WRITE_PC);
  if (!immed)
    displaced_write_reg (regs, dsc, 3, rm_val, CANNOT_WRITE_PC);
  dsc->rd = rt;
  dsc->u.ldst.xfersize = size;
  dsc->u.ldst.rn = rn;
  dsc->u.ldst.immed = immed;
  dsc->u.ldst.writeback = writeback;

  /* To write PC we can do:

     Before this sequence of instructions:
     r0 is the PC value got from displaced_read_reg, so r0 = from + 8;
     r2 is the Rn value got from displaced_read_reg.

     Insn1: push {pc} Write address of STR instruction + offset on stack
     Insn2: pop  {r4} Read it back from stack, r4 = addr(Insn1) + offset
     Insn3: sub r4, r4, pc   r4 = addr(Insn1) + offset - pc
				= addr(Insn1) + offset - addr(Insn3) - 8
				= offset - 16
     Insn4: add r4, r4, #8   r4 = offset - 8
     Insn5: add r0, r0, r4   r0 = from + 8 + offset - 8
				= from + offset
     Insn6: str r0, [r2, #imm] (or str r0, [r2, r3])

     Otherwise we don't know what value to write for PC, since the offset is
     architecture-dependent (sometimes PC+8, sometimes PC+12).  More details
     of this can be found in Section "Saving from r15" in
     http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0204g/Cihbjifh.html */

  dsc->cleanup = load ? &cleanup_load : &cleanup_store;
}


static int
thumb2_copy_load_literal (struct gdbarch *gdbarch, uint16_t insn1,
			  uint16_t insn2, struct regcache *regs,
			  arm_displaced_step_copy_insn_closure *dsc, int size)
{
  unsigned int u_bit = bit (insn1, 7);
  unsigned int rt = bits (insn2, 12, 15);
  int imm12 = bits (insn2, 0, 11);
  ULONGEST pc_val;

  displaced_debug_printf ("copying ldr pc (0x%x) R%d %c imm12 %.4x",
			  (unsigned int) dsc->insn_addr, rt, u_bit ? '+' : '-',
			  imm12);

  if (!u_bit)
    imm12 = -1 * imm12;

  /* Rewrite instruction LDR Rt imm12 into:

     Prepare: tmp[0] <- r0, tmp[1] <- r2, tmp[2] <- r3, r2 <- pc, r3 <- imm12

     LDR R0, R2, R3,

     Cleanup: rt <- r0, r0 <- tmp[0], r2 <- tmp[1], r3 <- tmp[2].  */


  dsc->tmp[0] = displaced_read_reg (regs, dsc, 0);
  dsc->tmp[2] = displaced_read_reg (regs, dsc, 2);
  dsc->tmp[3] = displaced_read_reg (regs, dsc, 3);

  pc_val = displaced_read_reg (regs, dsc, ARM_PC_REGNUM);

  pc_val = pc_val & 0xfffffffc;

  displaced_write_reg (regs, dsc, 2, pc_val, CANNOT_WRITE_PC);
  displaced_write_reg (regs, dsc, 3, imm12, CANNOT_WRITE_PC);

  dsc->rd = rt;

  dsc->u.ldst.xfersize = size;
  dsc->u.ldst.immed = 0;
  dsc->u.ldst.writeback = 0;
  dsc->u.ldst.restore_r4 = 0;

  /* LDR R0, R2, R3 */
  dsc->modinsn[0] = 0xf852;
  dsc->modinsn[1] = 0x3;
  dsc->numinsns = 2;

  dsc->cleanup = &cleanup_load;

  return 0;
}

static int
thumb2_copy_load_reg_imm (struct gdbarch *gdbarch, uint16_t insn1,
			  uint16_t insn2, struct regcache *regs,
			  arm_displaced_step_copy_insn_closure *dsc,
			  int writeback, int immed)
{
  unsigned int rt = bits (insn2, 12, 15);
  unsigned int rn = bits (insn1, 0, 3);
  unsigned int rm = bits (insn2, 0, 3);  /* Only valid if !immed.  */
  /* In LDR (register), there is also a register Rm, which is not allowed to
     be PC, so we don't have to check it.  */

  if (rt != ARM_PC_REGNUM && rn != ARM_PC_REGNUM)
    return thumb_copy_unmodified_32bit (gdbarch, insn1, insn2, "load",
					dsc);

  displaced_debug_printf ("copying ldr r%d [r%d] insn %.4x%.4x",
			  rt, rn, insn1, insn2);

  install_load_store (gdbarch, regs, dsc, 1, immed, writeback, 4,
		      0, rt, rm, rn);

  dsc->u.ldst.restore_r4 = 0;

  if (immed)
    /* ldr[b]<cond> rt, [rn, #imm], etc.
       ->
       ldr[b]<cond> r0, [r2, #imm].  */
    {
      dsc->modinsn[0] = (insn1 & 0xfff0) | 0x2;
      dsc->modinsn[1] = insn2 & 0x0fff;
    }
  else
    /* ldr[b]<cond> rt, [rn, rm], etc.
       ->
       ldr[b]<cond> r0, [r2, r3].  */
    {
      dsc->modinsn[0] = (insn1 & 0xfff0) | 0x2;
      dsc->modinsn[1] = (insn2 & 0x0ff0) | 0x3;
    }

  dsc->numinsns = 2;

  return 0;
}


static int
arm_copy_ldr_str_ldrb_strb (struct gdbarch *gdbarch, uint32_t insn,
			    struct regcache *regs,
			    arm_displaced_step_copy_insn_closure *dsc,
			    int load, int size, int usermode)
{
  int immed = !bit (insn, 25);
  int writeback = (bit (insn, 24) == 0 || bit (insn, 21) != 0);
  unsigned int rt = bits (insn, 12, 15);
  unsigned int rn = bits (insn, 16, 19);
  unsigned int rm = bits (insn, 0, 3);  /* Only valid if !immed.  */

  if (!insn_references_pc (insn, 0x000ff00ful))
    return arm_copy_unmodified (gdbarch, insn, "load/store", dsc);

  displaced_debug_printf ("copying %s%s r%d [r%d] insn %.8lx",
			  load ? (size == 1 ? "ldrb" : "ldr")
			       : (size == 1 ? "strb" : "str"),
			  usermode ? "t" : "",
			  rt, rn,
			  (unsigned long) insn);

  install_load_store (gdbarch, regs, dsc, load, immed, writeback, size,
		      usermode, rt, rm, rn);

  if (load || rt != ARM_PC_REGNUM)
    {
      dsc->u.ldst.restore_r4 = 0;

      if (immed)
	/* {ldr,str}[b]<cond> rt, [rn, #imm], etc.
	   ->
	   {ldr,str}[b]<cond> r0, [r2, #imm].  */
	dsc->modinsn[0] = (insn & 0xfff00fff) | 0x20000;
      else
	/* {ldr,str}[b]<cond> rt, [rn, rm], etc.
	   ->
	   {ldr,str}[b]<cond> r0, [r2, r3].  */
	dsc->modinsn[0] = (insn & 0xfff00ff0) | 0x20003;
    }
  else
    {
      /* We need to use r4 as scratch.  Make sure it's restored afterwards.  */
      dsc->u.ldst.restore_r4 = 1;
      dsc->modinsn[0] = 0xe92d8000;  /* push {pc} */
      dsc->modinsn[1] = 0xe8bd0010;  /* pop  {r4} */
      dsc->modinsn[2] = 0xe044400f;  /* sub r4, r4, pc.  */
      dsc->modinsn[3] = 0xe2844008;  /* add r4, r4, #8.  */
      dsc->modinsn[4] = 0xe0800004;  /* add r0, r0, r4.  */

      /* As above.  */
      if (immed)
	dsc->modinsn[5] = (insn & 0xfff00fff) | 0x20000;
      else
	dsc->modinsn[5] = (insn & 0xfff00ff0) | 0x20003;

      dsc->numinsns = 6;
    }

  dsc->cleanup = load ? &cleanup_load : &cleanup_store;

  return 0;
}

/* Cleanup LDM instructions with fully-populated register list.  This is an
   unfortunate corner case: it's impossible to implement correctly by modifying
   the instruction.  The issue is as follows: we have an instruction,

   ldm rN, {r0-r15}

   which we must rewrite to avoid loading PC.  A possible solution would be to
   do the load in two halves, something like (with suitable cleanup
   afterwards):

   mov r8, rN
   ldm[id][ab] r8!, {r0-r7}
   str r7, <temp>
   ldm[id][ab] r8, {r7-r14}
   <bkpt>

   but at present there's no suitable place for <temp>, since the scratch space
   is overwritten before the cleanup routine is called.  For now, we simply
   emulate the instruction.  */

static void
cleanup_block_load_all (struct gdbarch *gdbarch, struct regcache *regs,
			arm_displaced_step_copy_insn_closure *dsc)
{
  int inc = dsc->u.block.increment;
  int bump_before = dsc->u.block.before ? (inc ? 4 : -4) : 0;
  int bump_after = dsc->u.block.before ? 0 : (inc ? 4 : -4);
  uint32_t regmask = dsc->u.block.regmask;
  int regno = inc ? 0 : 15;
  CORE_ADDR xfer_addr = dsc->u.block.xfer_addr;
  int exception_return = dsc->u.block.load && dsc->u.block.user
			 && (regmask & 0x8000) != 0;
  uint32_t status = displaced_read_reg (regs, dsc, ARM_PS_REGNUM);
  int do_transfer = condition_true (dsc->u.block.cond, status);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  if (!do_transfer)
    return;

  /* If the instruction is ldm rN, {...pc}^, I don't think there's anything
     sensible we can do here.  Complain loudly.  */
  if (exception_return)
    error (_("Cannot single-step exception return"));

  /* We don't handle any stores here for now.  */
  gdb_assert (dsc->u.block.load != 0);

  displaced_debug_printf ("emulating block transfer: %s %s %s",
			  dsc->u.block.load ? "ldm" : "stm",
			  dsc->u.block.increment ? "inc" : "dec",
			  dsc->u.block.before ? "before" : "after");

  while (regmask)
    {
      uint32_t memword;

      if (inc)
	while (regno <= ARM_PC_REGNUM && (regmask & (1 << regno)) == 0)
	  regno++;
      else
	while (regno >= 0 && (regmask & (1 << regno)) == 0)
	  regno--;

      xfer_addr += bump_before;

      memword = read_memory_unsigned_integer (xfer_addr, 4, byte_order);
      displaced_write_reg (regs, dsc, regno, memword, LOAD_WRITE_PC);

      xfer_addr += bump_after;

      regmask &= ~(1 << regno);
    }

  if (dsc->u.block.writeback)
    displaced_write_reg (regs, dsc, dsc->u.block.rn, xfer_addr,
			 CANNOT_WRITE_PC);
}

/* Clean up an STM which included the PC in the register list.  */

static void
cleanup_block_store_pc (struct gdbarch *gdbarch, struct regcache *regs,
			arm_displaced_step_copy_insn_closure *dsc)
{
  uint32_t status = displaced_read_reg (regs, dsc, ARM_PS_REGNUM);
  int store_executed = condition_true (dsc->u.block.cond, status);
  CORE_ADDR pc_stored_at, transferred_regs
    = count_one_bits (dsc->u.block.regmask);
  CORE_ADDR stm_insn_addr;
  uint32_t pc_val;
  long offset;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  /* If condition code fails, there's nothing else to do.  */
  if (!store_executed)
    return;

  if (dsc->u.block.increment)
    {
      pc_stored_at = dsc->u.block.xfer_addr + 4 * transferred_regs;

      if (dsc->u.block.before)
	 pc_stored_at += 4;
    }
  else
    {
      pc_stored_at = dsc->u.block.xfer_addr;

      if (dsc->u.block.before)
	 pc_stored_at -= 4;
    }

  pc_val = read_memory_unsigned_integer (pc_stored_at, 4, byte_order);
  stm_insn_addr = dsc->scratch_base;
  offset = pc_val - stm_insn_addr;

  displaced_debug_printf ("detected PC offset %.8lx for STM instruction",
			  offset);

  /* Rewrite the stored PC to the proper value for the non-displaced original
     instruction.  */
  write_memory_unsigned_integer (pc_stored_at, 4, byte_order,
				 dsc->insn_addr + offset);
}

/* Clean up an LDM which includes the PC in the register list.  We clumped all
   the registers in the transferred list into a contiguous range r0...rX (to
   avoid loading PC directly and losing control of the debugged program), so we
   must undo that here.  */

static void
cleanup_block_load_pc (struct gdbarch *gdbarch,
		       struct regcache *regs,
		       arm_displaced_step_copy_insn_closure *dsc)
{
  uint32_t status = displaced_read_reg (regs, dsc, ARM_PS_REGNUM);
  int load_executed = condition_true (dsc->u.block.cond, status);
  unsigned int mask = dsc->u.block.regmask, write_reg = ARM_PC_REGNUM;
  unsigned int regs_loaded = count_one_bits (mask);
  unsigned int num_to_shuffle = regs_loaded, clobbered;

  /* The method employed here will fail if the register list is fully populated
     (we need to avoid loading PC directly).  */
  gdb_assert (num_to_shuffle < 16);

  if (!load_executed)
    return;

  clobbered = (1 << num_to_shuffle) - 1;

  while (num_to_shuffle > 0)
    {
      if ((mask & (1 << write_reg)) != 0)
	{
	  unsigned int read_reg = num_to_shuffle - 1;

	  if (read_reg != write_reg)
	    {
	      ULONGEST rval = displaced_read_reg (regs, dsc, read_reg);
	      displaced_write_reg (regs, dsc, write_reg, rval, LOAD_WRITE_PC);
	      displaced_debug_printf ("LDM: move loaded register r%d to r%d",
				      read_reg, write_reg);
	    }
	  else
	    displaced_debug_printf ("LDM: register r%d already in the right "
				    "place", write_reg);

	  clobbered &= ~(1 << write_reg);

	  num_to_shuffle--;
	}

      write_reg--;
    }

  /* Restore any registers we scribbled over.  */
  for (write_reg = 0; clobbered != 0; write_reg++)
    {
      if ((clobbered & (1 << write_reg)) != 0)
	{
	  displaced_write_reg (regs, dsc, write_reg, dsc->tmp[write_reg],
			       CANNOT_WRITE_PC);
	  displaced_debug_printf ("LDM: restored clobbered register r%d",
				  write_reg);
	  clobbered &= ~(1 << write_reg);
	}
    }

  /* Perform register writeback manually.  */
  if (dsc->u.block.writeback)
    {
      ULONGEST new_rn_val = dsc->u.block.xfer_addr;

      if (dsc->u.block.increment)
	new_rn_val += regs_loaded * 4;
      else
	new_rn_val -= regs_loaded * 4;

      displaced_write_reg (regs, dsc, dsc->u.block.rn, new_rn_val,
			   CANNOT_WRITE_PC);
    }
}

/* Handle ldm/stm, apart from some tricky cases which are unlikely to occur
   in user-level code (in particular exception return, ldm rn, {...pc}^).  */

static int
arm_copy_block_xfer (struct gdbarch *gdbarch, uint32_t insn,
		     struct regcache *regs,
		     arm_displaced_step_copy_insn_closure *dsc)
{
  int load = bit (insn, 20);
  int user = bit (insn, 22);
  int increment = bit (insn, 23);
  int before = bit (insn, 24);
  int writeback = bit (insn, 21);
  int rn = bits (insn, 16, 19);

  /* Block transfers which don't mention PC can be run directly
     out-of-line.  */
  if (rn != ARM_PC_REGNUM && (insn & 0x8000) == 0)
    return arm_copy_unmodified (gdbarch, insn, "ldm/stm", dsc);

  if (rn == ARM_PC_REGNUM)
    {
      warning (_("displaced: Unpredictable LDM or STM with "
		 "base register r15"));
      return arm_copy_unmodified (gdbarch, insn, "unpredictable ldm/stm", dsc);
    }

  displaced_debug_printf ("copying block transfer insn %.8lx",
			  (unsigned long) insn);

  dsc->u.block.xfer_addr = displaced_read_reg (regs, dsc, rn);
  dsc->u.block.rn = rn;

  dsc->u.block.load = load;
  dsc->u.block.user = user;
  dsc->u.block.increment = increment;
  dsc->u.block.before = before;
  dsc->u.block.writeback = writeback;
  dsc->u.block.cond = bits (insn, 28, 31);

  dsc->u.block.regmask = insn & 0xffff;

  if (load)
    {
      if ((insn & 0xffff) == 0xffff)
	{
	  /* LDM with a fully-populated register list.  This case is
	     particularly tricky.  Implement for now by fully emulating the
	     instruction (which might not behave perfectly in all cases, but
	     these instructions should be rare enough for that not to matter
	     too much).  */
	  dsc->modinsn[0] = ARM_NOP;

	  dsc->cleanup = &cleanup_block_load_all;
	}
      else
	{
	  /* LDM of a list of registers which includes PC.  Implement by
	     rewriting the list of registers to be transferred into a
	     contiguous chunk r0...rX before doing the transfer, then shuffling
	     registers into the correct places in the cleanup routine.  */
	  unsigned int regmask = insn & 0xffff;
	  unsigned int num_in_list = count_one_bits (regmask), new_regmask;
	  unsigned int i;

	  for (i = 0; i < num_in_list; i++)
	    dsc->tmp[i] = displaced_read_reg (regs, dsc, i);

	  /* Writeback makes things complicated.  We need to avoid clobbering
	     the base register with one of the registers in our modified
	     register list, but just using a different register can't work in
	     all cases, e.g.:

	       ldm r14!, {r0-r13,pc}

	     which would need to be rewritten as:

	       ldm rN!, {r0-r14}

	     but that can't work, because there's no free register for N.

	     Solve this by turning off the writeback bit, and emulating
	     writeback manually in the cleanup routine.  */

	  if (writeback)
	    insn &= ~(1 << 21);

	  new_regmask = (1 << num_in_list) - 1;

	  displaced_debug_printf ("LDM r%d%s, {..., pc}: original reg list "
				  "%.4x, modified list %.4x",
				  rn, writeback ? "!" : "",
				  (int) insn & 0xffff, new_regmask);

	  dsc->modinsn[0] = (insn & ~0xffff) | (new_regmask & 0xffff);

	  dsc->cleanup = &cleanup_block_load_pc;
	}
    }
  else
    {
      /* STM of a list of registers which includes PC.  Run the instruction
	 as-is, but out of line: this will store the wrong value for the PC,
	 so we must manually fix up the memory in the cleanup routine.
	 Doing things this way has the advantage that we can auto-detect
	 the offset of the PC write (which is architecture-dependent) in
	 the cleanup routine.  */
      dsc->modinsn[0] = insn;

      dsc->cleanup = &cleanup_block_store_pc;
    }

  return 0;
}

static int
thumb2_copy_block_xfer (struct gdbarch *gdbarch, uint16_t insn1, uint16_t insn2,
			struct regcache *regs,
			arm_displaced_step_copy_insn_closure *dsc)
{
  int rn = bits (insn1, 0, 3);
  int load = bit (insn1, 4);
  int writeback = bit (insn1, 5);

  /* Block transfers which don't mention PC can be run directly
     out-of-line.  */
  if (rn != ARM_PC_REGNUM && (insn2 & 0x8000) == 0)
    return thumb_copy_unmodified_32bit (gdbarch, insn1, insn2, "ldm/stm", dsc);

  if (rn == ARM_PC_REGNUM)
    {
      warning (_("displaced: Unpredictable LDM or STM with "
		 "base register r15"));
      return thumb_copy_unmodified_32bit (gdbarch, insn1, insn2,
					  "unpredictable ldm/stm", dsc);
    }

  displaced_debug_printf ("copying block transfer insn %.4x%.4x",
			  insn1, insn2);

  /* Clear bit 13, since it should be always zero.  */
  dsc->u.block.regmask = (insn2 & 0xdfff);
  dsc->u.block.rn = rn;

  dsc->u.block.load = load;
  dsc->u.block.user = 0;
  dsc->u.block.increment = bit (insn1, 7);
  dsc->u.block.before = bit (insn1, 8);
  dsc->u.block.writeback = writeback;
  dsc->u.block.cond = INST_AL;
  dsc->u.block.xfer_addr = displaced_read_reg (regs, dsc, rn);

  if (load)
    {
      if (dsc->u.block.regmask == 0xffff)
	{
	  /* This branch is impossible to happen.  */
	  gdb_assert (0);
	}
      else
	{
	  unsigned int regmask = dsc->u.block.regmask;
	  unsigned int num_in_list = count_one_bits (regmask), new_regmask;
	  unsigned int i;

	  for (i = 0; i < num_in_list; i++)
	    dsc->tmp[i] = displaced_read_reg (regs, dsc, i);

	  if (writeback)
	    insn1 &= ~(1 << 5);

	  new_regmask = (1 << num_in_list) - 1;

	  displaced_debug_printf ("LDM r%d%s, {..., pc}: original reg list "
				  "%.4x, modified list %.4x",
				  rn, writeback ? "!" : "",
				  (int) dsc->u.block.regmask, new_regmask);

	  dsc->modinsn[0] = insn1;
	  dsc->modinsn[1] = (new_regmask & 0xffff);
	  dsc->numinsns = 2;

	  dsc->cleanup = &cleanup_block_load_pc;
	}
    }
  else
    {
      dsc->modinsn[0] = insn1;
      dsc->modinsn[1] = insn2;
      dsc->numinsns = 2;
      dsc->cleanup = &cleanup_block_store_pc;
    }
  return 0;
}

/* Wrapper over read_memory_unsigned_integer for use in arm_get_next_pcs.
 This is used to avoid a dependency on BFD's bfd_endian enum.  */

ULONGEST
arm_get_next_pcs_read_memory_unsigned_integer (CORE_ADDR memaddr, int len,
					       int byte_order)
{
  return read_memory_unsigned_integer (memaddr, len,
				       (enum bfd_endian) byte_order);
}

/* Wrapper over gdbarch_addr_bits_remove for use in arm_get_next_pcs.  */

CORE_ADDR
arm_get_next_pcs_addr_bits_remove (struct arm_get_next_pcs *self,
				   CORE_ADDR val)
{
  return gdbarch_addr_bits_remove
    (gdb::checked_static_cast<regcache *> (self->regcache)->arch (), val);
}

/* Wrapper over syscall_next_pc for use in get_next_pcs.  */

static CORE_ADDR
arm_get_next_pcs_syscall_next_pc (struct arm_get_next_pcs *self)
{
  return 0;
}

/* Wrapper over arm_is_thumb for use in arm_get_next_pcs.  */

int
arm_get_next_pcs_is_thumb (struct arm_get_next_pcs *self)
{
  return arm_is_thumb (gdb::checked_static_cast<regcache *> (self->regcache));
}

/* single_step() is called just before we want to resume the inferior,
   if we want to single-step it but there is no hardware or kernel
   single-step support.  We find the target of the coming instructions
   and breakpoint them.  */

std::vector<CORE_ADDR>
arm_software_single_step (struct regcache *regcache)
{
  struct gdbarch *gdbarch = regcache->arch ();
  struct arm_get_next_pcs next_pcs_ctx;

  arm_get_next_pcs_ctor (&next_pcs_ctx,
			 &arm_get_next_pcs_ops,
			 gdbarch_byte_order (gdbarch),
			 gdbarch_byte_order_for_code (gdbarch),
			 0,
			 regcache);

  std::vector<CORE_ADDR> next_pcs = arm_get_next_pcs (&next_pcs_ctx);

  for (CORE_ADDR &pc_ref : next_pcs)
    pc_ref = gdbarch_addr_bits_remove (gdbarch, pc_ref);

  return next_pcs;
}

/* Cleanup/copy SVC (SWI) instructions.  These two functions are overridden
   for Linux, where some SVC instructions must be treated specially.  */

static void
cleanup_svc (struct gdbarch *gdbarch, struct regcache *regs,
	     arm_displaced_step_copy_insn_closure *dsc)
{
  CORE_ADDR resume_addr = dsc->insn_addr + dsc->insn_size;

  displaced_debug_printf ("cleanup for svc, resume at %.8lx",
			  (unsigned long) resume_addr);

  displaced_write_reg (regs, dsc, ARM_PC_REGNUM, resume_addr, BRANCH_WRITE_PC);
}


/* Common copy routine for svc instruction.  */

static int
install_svc (struct gdbarch *gdbarch, struct regcache *regs,
	     arm_displaced_step_copy_insn_closure *dsc)
{
  /* Preparation: none.
     Insn: unmodified svc.
     Cleanup: pc <- insn_addr + insn_size.  */

  /* Pretend we wrote to the PC, so cleanup doesn't set PC to the next
     instruction.  */
  dsc->wrote_to_pc = 1;

  /* Allow OS-specific code to override SVC handling.  */
  if (dsc->u.svc.copy_svc_os)
    return dsc->u.svc.copy_svc_os (gdbarch, regs, dsc);
  else
    {
      dsc->cleanup = &cleanup_svc;
      return 0;
    }
}

static int
arm_copy_svc (struct gdbarch *gdbarch, uint32_t insn,
	      regcache *regs, arm_displaced_step_copy_insn_closure *dsc)
{

  displaced_debug_printf ("copying svc insn %.8lx",
			  (unsigned long) insn);

  dsc->modinsn[0] = insn;

  return install_svc (gdbarch, regs, dsc);
}

static int
thumb_copy_svc (struct gdbarch *gdbarch, uint16_t insn,
		regcache *regs, arm_displaced_step_copy_insn_closure *dsc)
{

  displaced_debug_printf ("copying svc insn %.4x", insn);

  dsc->modinsn[0] = insn;

  return install_svc (gdbarch, regs, dsc);
}

/* Copy undefined instructions.  */

static int
arm_copy_undef (struct gdbarch *gdbarch, uint32_t insn,
		arm_displaced_step_copy_insn_closure *dsc)
{
  displaced_debug_printf ("copying undefined insn %.8lx",
			  (unsigned long) insn);

  dsc->modinsn[0] = insn;

  return 0;
}

static int
thumb_32bit_copy_undef (struct gdbarch *gdbarch, uint16_t insn1, uint16_t insn2,
			arm_displaced_step_copy_insn_closure *dsc)
{

  displaced_debug_printf ("copying undefined insn %.4x %.4x",
			  (unsigned short) insn1, (unsigned short) insn2);

  dsc->modinsn[0] = insn1;
  dsc->modinsn[1] = insn2;
  dsc->numinsns = 2;

  return 0;
}

/* Copy unpredictable instructions.  */

static int
arm_copy_unpred (struct gdbarch *gdbarch, uint32_t insn,
		 arm_displaced_step_copy_insn_closure *dsc)
{
  displaced_debug_printf ("copying unpredictable insn %.8lx",
			  (unsigned long) insn);

  dsc->modinsn[0] = insn;

  return 0;
}

/* The decode_* functions are instruction decoding helpers.  They mostly follow
   the presentation in the ARM ARM.  */

static int
arm_decode_misc_memhint_neon (struct gdbarch *gdbarch, uint32_t insn,
			      struct regcache *regs,
			      arm_displaced_step_copy_insn_closure *dsc)
{
  unsigned int op1 = bits (insn, 20, 26), op2 = bits (insn, 4, 7);
  unsigned int rn = bits (insn, 16, 19);

  if (op1 == 0x10 && (op2 & 0x2) == 0x0 && (rn & 0x1) == 0x0)
    return arm_copy_unmodified (gdbarch, insn, "cps", dsc);
  else if (op1 == 0x10 && op2 == 0x0 && (rn & 0x1) == 0x1)
    return arm_copy_unmodified (gdbarch, insn, "setend", dsc);
  else if ((op1 & 0x60) == 0x20)
    return arm_copy_unmodified (gdbarch, insn, "neon dataproc", dsc);
  else if ((op1 & 0x71) == 0x40)
    return arm_copy_unmodified (gdbarch, insn, "neon elt/struct load/store",
				dsc);
  else if ((op1 & 0x77) == 0x41)
    return arm_copy_unmodified (gdbarch, insn, "unallocated mem hint", dsc);
  else if ((op1 & 0x77) == 0x45)
    return arm_copy_preload (gdbarch, insn, regs, dsc);  /* pli.  */
  else if ((op1 & 0x77) == 0x51)
    {
      if (rn != 0xf)
	return arm_copy_preload (gdbarch, insn, regs, dsc);  /* pld/pldw.  */
      else
	return arm_copy_unpred (gdbarch, insn, dsc);
    }
  else if ((op1 & 0x77) == 0x55)
    return arm_copy_preload (gdbarch, insn, regs, dsc);  /* pld/pldw.  */
  else if (op1 == 0x57)
    switch (op2)
      {
      case 0x1: return arm_copy_unmodified (gdbarch, insn, "clrex", dsc);
      case 0x4: return arm_copy_unmodified (gdbarch, insn, "dsb", dsc);
      case 0x5: return arm_copy_unmodified (gdbarch, insn, "dmb", dsc);
      case 0x6: return arm_copy_unmodified (gdbarch, insn, "isb", dsc);
      default: return arm_copy_unpred (gdbarch, insn, dsc);
      }
  else if ((op1 & 0x63) == 0x43)
    return arm_copy_unpred (gdbarch, insn, dsc);
  else if ((op2 & 0x1) == 0x0)
    switch (op1 & ~0x80)
      {
      case 0x61:
	return arm_copy_unmodified (gdbarch, insn, "unallocated mem hint", dsc);
      case 0x65:
	return arm_copy_preload_reg (gdbarch, insn, regs, dsc);  /* pli reg.  */
      case 0x71: case 0x75:
	/* pld/pldw reg.  */
	return arm_copy_preload_reg (gdbarch, insn, regs, dsc);
      case 0x63: case 0x67: case 0x73: case 0x77:
	return arm_copy_unpred (gdbarch, insn, dsc);
      default:
	return arm_copy_undef (gdbarch, insn, dsc);
      }
  else
    return arm_copy_undef (gdbarch, insn, dsc);  /* Probably unreachable.  */
}

static int
arm_decode_unconditional (struct gdbarch *gdbarch, uint32_t insn,
			  struct regcache *regs,
			  arm_displaced_step_copy_insn_closure *dsc)
{
  if (bit (insn, 27) == 0)
    return arm_decode_misc_memhint_neon (gdbarch, insn, regs, dsc);
  /* Switch on bits: 0bxxxxx321xxx0xxxxxxxxxxxxxxxxxxxx.  */
  else switch (((insn & 0x7000000) >> 23) | ((insn & 0x100000) >> 20))
    {
    case 0x0: case 0x2:
      return arm_copy_unmodified (gdbarch, insn, "srs", dsc);

    case 0x1: case 0x3:
      return arm_copy_unmodified (gdbarch, insn, "rfe", dsc);

    case 0x4: case 0x5: case 0x6: case 0x7:
      return arm_copy_b_bl_blx (gdbarch, insn, regs, dsc);

    case 0x8:
      switch ((insn & 0xe00000) >> 21)
	{
	case 0x1: case 0x3: case 0x4: case 0x5: case 0x6: case 0x7:
	  /* stc/stc2.  */
	  return arm_copy_copro_load_store (gdbarch, insn, regs, dsc);

	case 0x2:
	  return arm_copy_unmodified (gdbarch, insn, "mcrr/mcrr2", dsc);

	default:
	  return arm_copy_undef (gdbarch, insn, dsc);
	}

    case 0x9:
      {
	 int rn_f = (bits (insn, 16, 19) == 0xf);
	switch ((insn & 0xe00000) >> 21)
	  {
	  case 0x1: case 0x3:
	    /* ldc/ldc2 imm (undefined for rn == pc).  */
	    return rn_f ? arm_copy_undef (gdbarch, insn, dsc)
			: arm_copy_copro_load_store (gdbarch, insn, regs, dsc);

	  case 0x2:
	    return arm_copy_unmodified (gdbarch, insn, "mrrc/mrrc2", dsc);

	  case 0x4: case 0x5: case 0x6: case 0x7:
	    /* ldc/ldc2 lit (undefined for rn != pc).  */
	    return rn_f ? arm_copy_copro_load_store (gdbarch, insn, regs, dsc)
			: arm_copy_undef (gdbarch, insn, dsc);

	  default:
	    return arm_copy_undef (gdbarch, insn, dsc);
	  }
      }

    case 0xa:
      return arm_copy_unmodified (gdbarch, insn, "stc/stc2", dsc);

    case 0xb:
      if (bits (insn, 16, 19) == 0xf)
	/* ldc/ldc2 lit.  */
	return arm_copy_copro_load_store (gdbarch, insn, regs, dsc);
      else
	return arm_copy_undef (gdbarch, insn, dsc);

    case 0xc:
      if (bit (insn, 4))
	return arm_copy_unmodified (gdbarch, insn, "mcr/mcr2", dsc);
      else
	return arm_copy_unmodified (gdbarch, insn, "cdp/cdp2", dsc);

    case 0xd:
      if (bit (insn, 4))
	return arm_copy_unmodified (gdbarch, insn, "mrc/mrc2", dsc);
      else
	return arm_copy_unmodified (gdbarch, insn, "cdp/cdp2", dsc);

    default:
      return arm_copy_undef (gdbarch, insn, dsc);
    }
}

/* Decode miscellaneous instructions in dp/misc encoding space.  */

static int
arm_decode_miscellaneous (struct gdbarch *gdbarch, uint32_t insn,
			  struct regcache *regs,
			  arm_displaced_step_copy_insn_closure *dsc)
{
  unsigned int op2 = bits (insn, 4, 6);
  unsigned int op = bits (insn, 21, 22);

  switch (op2)
    {
    case 0x0:
      return arm_copy_unmodified (gdbarch, insn, "mrs/msr", dsc);

    case 0x1:
      if (op == 0x1)  /* bx.  */
	return arm_copy_bx_blx_reg (gdbarch, insn, regs, dsc);
      else if (op == 0x3)
	return arm_copy_unmodified (gdbarch, insn, "clz", dsc);
      else
	return arm_copy_undef (gdbarch, insn, dsc);

    case 0x2:
      if (op == 0x1)
	/* Not really supported.  */
	return arm_copy_unmodified (gdbarch, insn, "bxj", dsc);
      else
	return arm_copy_undef (gdbarch, insn, dsc);

    case 0x3:
      if (op == 0x1)
	return arm_copy_bx_blx_reg (gdbarch, insn,
				regs, dsc);  /* blx register.  */
      else
	return arm_copy_undef (gdbarch, insn, dsc);

    case 0x5:
      return arm_copy_unmodified (gdbarch, insn, "saturating add/sub", dsc);

    case 0x7:
      if (op == 0x1)
	return arm_copy_unmodified (gdbarch, insn, "bkpt", dsc);
      else if (op == 0x3)
	/* Not really supported.  */
	return arm_copy_unmodified (gdbarch, insn, "smc", dsc);
      [[fallthrough]];

    default:
      return arm_copy_undef (gdbarch, insn, dsc);
    }
}

static int
arm_decode_dp_misc (struct gdbarch *gdbarch, uint32_t insn,
		    struct regcache *regs,
		    arm_displaced_step_copy_insn_closure *dsc)
{
  if (bit (insn, 25))
    switch (bits (insn, 20, 24))
      {
      case 0x10:
	return arm_copy_unmodified (gdbarch, insn, "movw", dsc);

      case 0x14:
	return arm_copy_unmodified (gdbarch, insn, "movt", dsc);

      case 0x12: case 0x16:
	return arm_copy_unmodified (gdbarch, insn, "msr imm", dsc);

      default:
	return arm_copy_alu_imm (gdbarch, insn, regs, dsc);
      }
  else
    {
      uint32_t op1 = bits (insn, 20, 24), op2 = bits (insn, 4, 7);

      if ((op1 & 0x19) != 0x10 && (op2 & 0x1) == 0x0)
	return arm_copy_alu_reg (gdbarch, insn, regs, dsc);
      else if ((op1 & 0x19) != 0x10 && (op2 & 0x9) == 0x1)
	return arm_copy_alu_shifted_reg (gdbarch, insn, regs, dsc);
      else if ((op1 & 0x19) == 0x10 && (op2 & 0x8) == 0x0)
	return arm_decode_miscellaneous (gdbarch, insn, regs, dsc);
      else if ((op1 & 0x19) == 0x10 && (op2 & 0x9) == 0x8)
	return arm_copy_unmodified (gdbarch, insn, "halfword mul/mla", dsc);
      else if ((op1 & 0x10) == 0x00 && op2 == 0x9)
	return arm_copy_unmodified (gdbarch, insn, "mul/mla", dsc);
      else if ((op1 & 0x10) == 0x10 && op2 == 0x9)
	return arm_copy_unmodified (gdbarch, insn, "synch", dsc);
      else if (op2 == 0xb || (op2 & 0xd) == 0xd)
	/* 2nd arg means "unprivileged".  */
	return arm_copy_extra_ld_st (gdbarch, insn, (op1 & 0x12) == 0x02, regs,
				     dsc);
    }

  /* Should be unreachable.  */
  return 1;
}

static int
arm_decode_ld_st_word_ubyte (struct gdbarch *gdbarch, uint32_t insn,
			     struct regcache *regs,
			     arm_displaced_step_copy_insn_closure *dsc)
{
  int a = bit (insn, 25), b = bit (insn, 4);
  uint32_t op1 = bits (insn, 20, 24);

  if ((!a && (op1 & 0x05) == 0x00 && (op1 & 0x17) != 0x02)
      || (a && (op1 & 0x05) == 0x00 && (op1 & 0x17) != 0x02 && !b))
    return arm_copy_ldr_str_ldrb_strb (gdbarch, insn, regs, dsc, 0, 4, 0);
  else if ((!a && (op1 & 0x17) == 0x02)
	    || (a && (op1 & 0x17) == 0x02 && !b))
    return arm_copy_ldr_str_ldrb_strb (gdbarch, insn, regs, dsc, 0, 4, 1);
  else if ((!a && (op1 & 0x05) == 0x01 && (op1 & 0x17) != 0x03)
	    || (a && (op1 & 0x05) == 0x01 && (op1 & 0x17) != 0x03 && !b))
    return arm_copy_ldr_str_ldrb_strb (gdbarch, insn, regs, dsc, 1, 4, 0);
  else if ((!a && (op1 & 0x17) == 0x03)
	   || (a && (op1 & 0x17) == 0x03 && !b))
    return arm_copy_ldr_str_ldrb_strb (gdbarch, insn, regs, dsc, 1, 4, 1);
  else if ((!a && (op1 & 0x05) == 0x04 && (op1 & 0x17) != 0x06)
	    || (a && (op1 & 0x05) == 0x04 && (op1 & 0x17) != 0x06 && !b))
    return arm_copy_ldr_str_ldrb_strb (gdbarch, insn, regs, dsc, 0, 1, 0);
  else if ((!a && (op1 & 0x17) == 0x06)
	   || (a && (op1 & 0x17) == 0x06 && !b))
    return arm_copy_ldr_str_ldrb_strb (gdbarch, insn, regs, dsc, 0, 1, 1);
  else if ((!a && (op1 & 0x05) == 0x05 && (op1 & 0x17) != 0x07)
	   || (a && (op1 & 0x05) == 0x05 && (op1 & 0x17) != 0x07 && !b))
    return arm_copy_ldr_str_ldrb_strb (gdbarch, insn, regs, dsc, 1, 1, 0);
  else if ((!a && (op1 & 0x17) == 0x07)
	   || (a && (op1 & 0x17) == 0x07 && !b))
    return arm_copy_ldr_str_ldrb_strb (gdbarch, insn, regs, dsc, 1, 1, 1);

  /* Should be unreachable.  */
  return 1;
}

static int
arm_decode_media (struct gdbarch *gdbarch, uint32_t insn,
		  arm_displaced_step_copy_insn_closure *dsc)
{
  switch (bits (insn, 20, 24))
    {
    case 0x00: case 0x01: case 0x02: case 0x03:
      return arm_copy_unmodified (gdbarch, insn, "parallel add/sub signed", dsc);

    case 0x04: case 0x05: case 0x06: case 0x07:
      return arm_copy_unmodified (gdbarch, insn, "parallel add/sub unsigned", dsc);

    case 0x08: case 0x09: case 0x0a: case 0x0b:
    case 0x0c: case 0x0d: case 0x0e: case 0x0f:
      return arm_copy_unmodified (gdbarch, insn,
			      "decode/pack/unpack/saturate/reverse", dsc);

    case 0x18:
      if (bits (insn, 5, 7) == 0)  /* op2.  */
	 {
	  if (bits (insn, 12, 15) == 0xf)
	    return arm_copy_unmodified (gdbarch, insn, "usad8", dsc);
	  else
	    return arm_copy_unmodified (gdbarch, insn, "usada8", dsc);
	}
      else
	 return arm_copy_undef (gdbarch, insn, dsc);

    case 0x1a: case 0x1b:
      if (bits (insn, 5, 6) == 0x2)  /* op2[1:0].  */
	return arm_copy_unmodified (gdbarch, insn, "sbfx", dsc);
      else
	return arm_copy_undef (gdbarch, insn, dsc);

    case 0x1c: case 0x1d:
      if (bits (insn, 5, 6) == 0x0)  /* op2[1:0].  */
	 {
	  if (bits (insn, 0, 3) == 0xf)
	    return arm_copy_unmodified (gdbarch, insn, "bfc", dsc);
	  else
	    return arm_copy_unmodified (gdbarch, insn, "bfi", dsc);
	}
      else
	return arm_copy_undef (gdbarch, insn, dsc);

    case 0x1e: case 0x1f:
      if (bits (insn, 5, 6) == 0x2)  /* op2[1:0].  */
	return arm_copy_unmodified (gdbarch, insn, "ubfx", dsc);
      else
	return arm_copy_undef (gdbarch, insn, dsc);
    }

  /* Should be unreachable.  */
  return 1;
}

static int
arm_decode_b_bl_ldmstm (struct gdbarch *gdbarch, uint32_t insn,
			struct regcache *regs,
			arm_displaced_step_copy_insn_closure *dsc)
{
  if (bit (insn, 25))
    return arm_copy_b_bl_blx (gdbarch, insn, regs, dsc);
  else
    return arm_copy_block_xfer (gdbarch, insn, regs, dsc);
}

static int
arm_decode_ext_reg_ld_st (struct gdbarch *gdbarch, uint32_t insn,
			  struct regcache *regs,
			  arm_displaced_step_copy_insn_closure *dsc)
{
  unsigned int opcode = bits (insn, 20, 24);

  switch (opcode)
    {
    case 0x04: case 0x05:  /* VFP/Neon mrrc/mcrr.  */
      return arm_copy_unmodified (gdbarch, insn, "vfp/neon mrrc/mcrr", dsc);

    case 0x08: case 0x0a: case 0x0c: case 0x0e:
    case 0x12: case 0x16:
      return arm_copy_unmodified (gdbarch, insn, "vfp/neon vstm/vpush", dsc);

    case 0x09: case 0x0b: case 0x0d: case 0x0f:
    case 0x13: case 0x17:
      return arm_copy_unmodified (gdbarch, insn, "vfp/neon vldm/vpop", dsc);

    case 0x10: case 0x14: case 0x18: case 0x1c:  /* vstr.  */
    case 0x11: case 0x15: case 0x19: case 0x1d:  /* vldr.  */
      /* Note: no writeback for these instructions.  Bit 25 will always be
	 zero though (via caller), so the following works OK.  */
      return arm_copy_copro_load_store (gdbarch, insn, regs, dsc);
    }

  /* Should be unreachable.  */
  return 1;
}

/* Decode shifted register instructions.  */

static int
thumb2_decode_dp_shift_reg (struct gdbarch *gdbarch, uint16_t insn1,
			    uint16_t insn2,  struct regcache *regs,
			    arm_displaced_step_copy_insn_closure *dsc)
{
  /* PC is only allowed to be used in instruction MOV.  */

  unsigned int op = bits (insn1, 5, 8);
  unsigned int rn = bits (insn1, 0, 3);

  if (op == 0x2 && rn == 0xf) /* MOV */
    return thumb2_copy_alu_imm (gdbarch, insn1, insn2, regs, dsc);
  else
    return thumb_copy_unmodified_32bit (gdbarch, insn1, insn2,
					"dp (shift reg)", dsc);
}


/* Decode extension register load/store.  Exactly the same as
   arm_decode_ext_reg_ld_st.  */

static int
thumb2_decode_ext_reg_ld_st (struct gdbarch *gdbarch, uint16_t insn1,
			     uint16_t insn2,  struct regcache *regs,
			     arm_displaced_step_copy_insn_closure *dsc)
{
  unsigned int opcode = bits (insn1, 4, 8);

  switch (opcode)
    {
    case 0x04: case 0x05:
      return thumb_copy_unmodified_32bit (gdbarch, insn1, insn2,
					  "vfp/neon vmov", dsc);

    case 0x08: case 0x0c: /* 01x00 */
    case 0x0a: case 0x0e: /* 01x10 */
    case 0x12: case 0x16: /* 10x10 */
      return thumb_copy_unmodified_32bit (gdbarch, insn1, insn2,
					  "vfp/neon vstm/vpush", dsc);

    case 0x09: case 0x0d: /* 01x01 */
    case 0x0b: case 0x0f: /* 01x11 */
    case 0x13: case 0x17: /* 10x11 */
      return thumb_copy_unmodified_32bit (gdbarch, insn1, insn2,
					  "vfp/neon vldm/vpop", dsc);

    case 0x10: case 0x14: case 0x18: case 0x1c:  /* vstr.  */
      return thumb_copy_unmodified_32bit (gdbarch, insn1, insn2,
					  "vstr", dsc);
    case 0x11: case 0x15: case 0x19: case 0x1d:  /* vldr.  */
      return thumb2_copy_copro_load_store (gdbarch, insn1, insn2, regs, dsc);
    }

  /* Should be unreachable.  */
  return 1;
}

static int
arm_decode_svc_copro (struct gdbarch *gdbarch, uint32_t insn,
		      regcache *regs, arm_displaced_step_copy_insn_closure *dsc)
{
  unsigned int op1 = bits (insn, 20, 25);
  int op = bit (insn, 4);
  unsigned int coproc = bits (insn, 8, 11);

  if ((op1 & 0x20) == 0x00 && (op1 & 0x3a) != 0x00 && (coproc & 0xe) == 0xa)
    return arm_decode_ext_reg_ld_st (gdbarch, insn, regs, dsc);
  else if ((op1 & 0x21) == 0x00 && (op1 & 0x3a) != 0x00
	   && (coproc & 0xe) != 0xa)
    /* stc/stc2.  */
    return arm_copy_copro_load_store (gdbarch, insn, regs, dsc);
  else if ((op1 & 0x21) == 0x01 && (op1 & 0x3a) != 0x00
	   && (coproc & 0xe) != 0xa)
    /* ldc/ldc2 imm/lit.  */
    return arm_copy_copro_load_store (gdbarch, insn, regs, dsc);
  else if ((op1 & 0x3e) == 0x00)
    return arm_copy_undef (gdbarch, insn, dsc);
  else if ((op1 & 0x3e) == 0x04 && (coproc & 0xe) == 0xa)
    return arm_copy_unmodified (gdbarch, insn, "neon 64bit xfer", dsc);
  else if (op1 == 0x04 && (coproc & 0xe) != 0xa)
    return arm_copy_unmodified (gdbarch, insn, "mcrr/mcrr2", dsc);
  else if (op1 == 0x05 && (coproc & 0xe) != 0xa)
    return arm_copy_unmodified (gdbarch, insn, "mrrc/mrrc2", dsc);
  else if ((op1 & 0x30) == 0x20 && !op)
    {
      if ((coproc & 0xe) == 0xa)
	return arm_copy_unmodified (gdbarch, insn, "vfp dataproc", dsc);
      else
	return arm_copy_unmodified (gdbarch, insn, "cdp/cdp2", dsc);
    }
  else if ((op1 & 0x30) == 0x20 && op)
    return arm_copy_unmodified (gdbarch, insn, "neon 8/16/32 bit xfer", dsc);
  else if ((op1 & 0x31) == 0x20 && op && (coproc & 0xe) != 0xa)
    return arm_copy_unmodified (gdbarch, insn, "mcr/mcr2", dsc);
  else if ((op1 & 0x31) == 0x21 && op && (coproc & 0xe) != 0xa)
    return arm_copy_unmodified (gdbarch, insn, "mrc/mrc2", dsc);
  else if ((op1 & 0x30) == 0x30)
    return arm_copy_svc (gdbarch, insn, regs, dsc);
  else
    return arm_copy_undef (gdbarch, insn, dsc);  /* Possibly unreachable.  */
}

static int
thumb2_decode_svc_copro (struct gdbarch *gdbarch, uint16_t insn1,
			 uint16_t insn2, struct regcache *regs,
			 arm_displaced_step_copy_insn_closure *dsc)
{
  unsigned int coproc = bits (insn2, 8, 11);
  unsigned int bit_5_8 = bits (insn1, 5, 8);
  unsigned int bit_9 = bit (insn1, 9);
  unsigned int bit_4 = bit (insn1, 4);

  if (bit_9 == 0)
    {
      if (bit_5_8 == 2)
	return thumb_copy_unmodified_32bit (gdbarch, insn1, insn2,
					    "neon 64bit xfer/mrrc/mrrc2/mcrr/mcrr2",
					    dsc);
      else if (bit_5_8 == 0) /* UNDEFINED.  */
	return thumb_32bit_copy_undef (gdbarch, insn1, insn2, dsc);
      else
	{
	   /*coproc is 101x.  SIMD/VFP, ext registers load/store.  */
	  if ((coproc & 0xe) == 0xa)
	    return thumb2_decode_ext_reg_ld_st (gdbarch, insn1, insn2, regs,
						dsc);
	  else /* coproc is not 101x.  */
	    {
	      if (bit_4 == 0) /* STC/STC2.  */
		return thumb_copy_unmodified_32bit (gdbarch, insn1, insn2,
						    "stc/stc2", dsc);
	      else /* LDC/LDC2 {literal, immediate}.  */
		return thumb2_copy_copro_load_store (gdbarch, insn1, insn2,
						     regs, dsc);
	    }
	}
    }
  else
    return thumb_copy_unmodified_32bit (gdbarch, insn1, insn2, "coproc", dsc);

  return 0;
}

static void
install_pc_relative (struct gdbarch *gdbarch, struct regcache *regs,
		     arm_displaced_step_copy_insn_closure *dsc, int rd)
{
  /* ADR Rd, #imm

     Rewrite as:

     Preparation: Rd <- PC
     Insn: ADD Rd, #imm
     Cleanup: Null.
  */

  /* Rd <- PC */
  int val = displaced_read_reg (regs, dsc, ARM_PC_REGNUM);
  displaced_write_reg (regs, dsc, rd, val, CANNOT_WRITE_PC);
}

static int
thumb_copy_pc_relative_16bit (struct gdbarch *gdbarch, struct regcache *regs,
			      arm_displaced_step_copy_insn_closure *dsc,
			      int rd, unsigned int imm)
{

  /* Encoding T2: ADDS Rd, #imm */
  dsc->modinsn[0] = (0x3000 | (rd << 8) | imm);

  install_pc_relative (gdbarch, regs, dsc, rd);

  return 0;
}

static int
thumb_decode_pc_relative_16bit (struct gdbarch *gdbarch, uint16_t insn,
				struct regcache *regs,
				arm_displaced_step_copy_insn_closure *dsc)
{
  unsigned int rd = bits (insn, 8, 10);
  unsigned int imm8 = bits (insn, 0, 7);

  displaced_debug_printf ("copying thumb adr r%d, #%d insn %.4x",
			  rd, imm8, insn);

  return thumb_copy_pc_relative_16bit (gdbarch, regs, dsc, rd, imm8);
}

static int
thumb_copy_pc_relative_32bit (struct gdbarch *gdbarch, uint16_t insn1,
			      uint16_t insn2, struct regcache *regs,
			      arm_displaced_step_copy_insn_closure *dsc)
{
  unsigned int rd = bits (insn2, 8, 11);
  /* Since immediate has the same encoding in ADR ADD and SUB, so we simply
     extract raw immediate encoding rather than computing immediate.  When
     generating ADD or SUB instruction, we can simply perform OR operation to
     set immediate into ADD.  */
  unsigned int imm_3_8 = insn2 & 0x70ff;
  unsigned int imm_i = insn1 & 0x0400; /* Clear all bits except bit 10.  */

  displaced_debug_printf ("copying thumb adr r%d, #%d:%d insn %.4x%.4x",
			  rd, imm_i, imm_3_8, insn1, insn2);

  if (bit (insn1, 7)) /* Encoding T2 */
    {
      /* Encoding T3: SUB Rd, Rd, #imm */
      dsc->modinsn[0] = (0xf1a0 | rd | imm_i);
      dsc->modinsn[1] = ((rd << 8) | imm_3_8);
    }
  else /* Encoding T3 */
    {
      /* Encoding T3: ADD Rd, Rd, #imm */
      dsc->modinsn[0] = (0xf100 | rd | imm_i);
      dsc->modinsn[1] = ((rd << 8) | imm_3_8);
    }
  dsc->numinsns = 2;

  install_pc_relative (gdbarch, regs, dsc, rd);

  return 0;
}

static int
thumb_copy_16bit_ldr_literal (struct gdbarch *gdbarch, uint16_t insn1,
			      struct regcache *regs,
			      arm_displaced_step_copy_insn_closure *dsc)
{
  unsigned int rt = bits (insn1, 8, 10);
  unsigned int pc;
  int imm8 = (bits (insn1, 0, 7) << 2);

  /* LDR Rd, #imm8

     Rwrite as:

     Preparation: tmp0 <- R0, tmp2 <- R2, tmp3 <- R3, R2 <- PC, R3 <- #imm8;

     Insn: LDR R0, [R2, R3];
     Cleanup: R2 <- tmp2, R3 <- tmp3, Rd <- R0, R0 <- tmp0 */

  displaced_debug_printf ("copying thumb ldr r%d [pc #%d]", rt, imm8);

  dsc->tmp[0] = displaced_read_reg (regs, dsc, 0);
  dsc->tmp[2] = displaced_read_reg (regs, dsc, 2);
  dsc->tmp[3] = displaced_read_reg (regs, dsc, 3);
  pc = displaced_read_reg (regs, dsc, ARM_PC_REGNUM);
  /* The assembler calculates the required value of the offset from the
     Align(PC,4) value of this instruction to the label.  */
  pc = pc & 0xfffffffc;

  displaced_write_reg (regs, dsc, 2, pc, CANNOT_WRITE_PC);
  displaced_write_reg (regs, dsc, 3, imm8, CANNOT_WRITE_PC);

  dsc->rd = rt;
  dsc->u.ldst.xfersize = 4;
  dsc->u.ldst.rn = 0;
  dsc->u.ldst.immed = 0;
  dsc->u.ldst.writeback = 0;
  dsc->u.ldst.restore_r4 = 0;

  dsc->modinsn[0] = 0x58d0; /* ldr r0, [r2, r3]*/

  dsc->cleanup = &cleanup_load;

  return 0;
}

/* Copy Thumb cbnz/cbz instruction.  */

static int
thumb_copy_cbnz_cbz (struct gdbarch *gdbarch, uint16_t insn1,
		     struct regcache *regs,
		     arm_displaced_step_copy_insn_closure *dsc)
{
  int non_zero = bit (insn1, 11);
  unsigned int imm5 = (bit (insn1, 9) << 6) | (bits (insn1, 3, 7) << 1);
  CORE_ADDR from = dsc->insn_addr;
  int rn = bits (insn1, 0, 2);
  int rn_val = displaced_read_reg (regs, dsc, rn);

  dsc->u.branch.cond = (rn_val && non_zero) || (!rn_val && !non_zero);
  /* CBNZ and CBZ do not affect the condition flags.  If condition is true,
     set it INST_AL, so cleanup_branch will know branch is taken, otherwise,
     condition is false, let it be, cleanup_branch will do nothing.  */
  if (dsc->u.branch.cond)
    {
      dsc->u.branch.cond = INST_AL;
      dsc->u.branch.dest = from + 4 + imm5;
    }
  else
      dsc->u.branch.dest = from + 2;

  dsc->u.branch.link = 0;
  dsc->u.branch.exchange = 0;

  displaced_debug_printf ("copying %s [r%d = 0x%x] insn %.4x to %.8lx",
			  non_zero ? "cbnz" : "cbz",
			  rn, rn_val, insn1, dsc->u.branch.dest);

  dsc->modinsn[0] = THUMB_NOP;

  dsc->cleanup = &cleanup_branch;
  return 0;
}

/* Copy Table Branch Byte/Halfword */
static int
thumb2_copy_table_branch (struct gdbarch *gdbarch, uint16_t insn1,
			  uint16_t insn2, struct regcache *regs,
			  arm_displaced_step_copy_insn_closure *dsc)
{
  ULONGEST rn_val, rm_val;
  int is_tbh = bit (insn2, 4);
  CORE_ADDR halfwords = 0;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  rn_val = displaced_read_reg (regs, dsc, bits (insn1, 0, 3));
  rm_val = displaced_read_reg (regs, dsc, bits (insn2, 0, 3));

  if (is_tbh)
    {
      gdb_byte buf[2];

      target_read_memory (rn_val + 2 * rm_val, buf, 2);
      halfwords = extract_unsigned_integer (buf, 2, byte_order);
    }
  else
    {
      gdb_byte buf[1];

      target_read_memory (rn_val + rm_val, buf, 1);
      halfwords = extract_unsigned_integer (buf, 1, byte_order);
    }

  displaced_debug_printf ("%s base 0x%x offset 0x%x offset 0x%x",
			  is_tbh ? "tbh" : "tbb",
			  (unsigned int) rn_val, (unsigned int) rm_val,
			  (unsigned int) halfwords);

  dsc->u.branch.cond = INST_AL;
  dsc->u.branch.link = 0;
  dsc->u.branch.exchange = 0;
  dsc->u.branch.dest = dsc->insn_addr + 4 + 2 * halfwords;

  dsc->cleanup = &cleanup_branch;

  return 0;
}

static void
cleanup_pop_pc_16bit_all (struct gdbarch *gdbarch, struct regcache *regs,
			  arm_displaced_step_copy_insn_closure *dsc)
{
  /* PC <- r7 */
  int val = displaced_read_reg (regs, dsc, 7);
  displaced_write_reg (regs, dsc, ARM_PC_REGNUM, val, BX_WRITE_PC);

  /* r7 <- r8 */
  val = displaced_read_reg (regs, dsc, 8);
  displaced_write_reg (regs, dsc, 7, val, CANNOT_WRITE_PC);

  /* r8 <- tmp[0] */
  displaced_write_reg (regs, dsc, 8, dsc->tmp[0], CANNOT_WRITE_PC);

}

static int
thumb_copy_pop_pc_16bit (struct gdbarch *gdbarch, uint16_t insn1,
			 struct regcache *regs,
			 arm_displaced_step_copy_insn_closure *dsc)
{
  dsc->u.block.regmask = insn1 & 0x00ff;

  /* Rewrite instruction: POP {rX, rY, ...,rZ, PC}
     to :

     (1) register list is full, that is, r0-r7 are used.
     Prepare: tmp[0] <- r8

     POP {r0, r1, ...., r6, r7}; remove PC from reglist
     MOV r8, r7; Move value of r7 to r8;
     POP {r7}; Store PC value into r7.

     Cleanup: PC <- r7, r7 <- r8, r8 <-tmp[0]

     (2) register list is not full, supposing there are N registers in
     register list (except PC, 0 <= N <= 7).
     Prepare: for each i, 0 - N, tmp[i] <- ri.

     POP {r0, r1, ...., rN};

     Cleanup: Set registers in original reglist from r0 - rN.  Restore r0 - rN
     from tmp[] properly.
  */
  displaced_debug_printf ("copying thumb pop {%.8x, pc} insn %.4x",
			  dsc->u.block.regmask, insn1);

  if (dsc->u.block.regmask == 0xff)
    {
      dsc->tmp[0] = displaced_read_reg (regs, dsc, 8);

      dsc->modinsn[0] = (insn1 & 0xfeff); /* POP {r0,r1,...,r6, r7} */
      dsc->modinsn[1] = 0x46b8; /* MOV r8, r7 */
      dsc->modinsn[2] = 0xbc80; /* POP {r7} */

      dsc->numinsns = 3;
      dsc->cleanup = &cleanup_pop_pc_16bit_all;
    }
  else
    {
      unsigned int num_in_list = count_one_bits (dsc->u.block.regmask);
      unsigned int i;
      unsigned int new_regmask;

      for (i = 0; i < num_in_list + 1; i++)
	dsc->tmp[i] = displaced_read_reg (regs, dsc, i);

      new_regmask = (1 << (num_in_list + 1)) - 1;

      displaced_debug_printf ("POP {..., pc}: original reg list %.4x, "
			      "modified list %.4x",
			      (int) dsc->u.block.regmask, new_regmask);

      dsc->u.block.regmask |= 0x8000;
      dsc->u.block.writeback = 0;
      dsc->u.block.cond = INST_AL;

      dsc->modinsn[0] = (insn1 & ~0x1ff) | (new_regmask & 0xff);

      dsc->cleanup = &cleanup_block_load_pc;
    }

  return 0;
}

static void
thumb_process_displaced_16bit_insn (struct gdbarch *gdbarch, uint16_t insn1,
				    struct regcache *regs,
				    arm_displaced_step_copy_insn_closure *dsc)
{
  unsigned short op_bit_12_15 = bits (insn1, 12, 15);
  unsigned short op_bit_10_11 = bits (insn1, 10, 11);
  int err = 0;

  /* 16-bit thumb instructions.  */
  switch (op_bit_12_15)
    {
      /* Shift (imme), add, subtract, move and compare.  */
    case 0: case 1: case 2: case 3:
      err = thumb_copy_unmodified_16bit (gdbarch, insn1,
					 "shift/add/sub/mov/cmp",
					 dsc);
      break;
    case 4:
      switch (op_bit_10_11)
	{
	case 0: /* Data-processing */
	  err = thumb_copy_unmodified_16bit (gdbarch, insn1,
					     "data-processing",
					     dsc);
	  break;
	case 1: /* Special data instructions and branch and exchange.  */
	  {
	    unsigned short op = bits (insn1, 7, 9);
	    if (op == 6 || op == 7) /* BX or BLX */
	      err = thumb_copy_bx_blx_reg (gdbarch, insn1, regs, dsc);
	    else if (bits (insn1, 6, 7) != 0) /* ADD/MOV/CMP high registers.  */
	      err = thumb_copy_alu_reg (gdbarch, insn1, regs, dsc);
	    else
	      err = thumb_copy_unmodified_16bit (gdbarch, insn1, "special data",
						 dsc);
	  }
	  break;
	default: /* LDR (literal) */
	  err = thumb_copy_16bit_ldr_literal (gdbarch, insn1, regs, dsc);
	}
      break;
    case 5: case 6: case 7: case 8: case 9: /* Load/Store single data item */
      err = thumb_copy_unmodified_16bit (gdbarch, insn1, "ldr/str", dsc);
      break;
    case 10:
      if (op_bit_10_11 < 2) /* Generate PC-relative address */
	err = thumb_decode_pc_relative_16bit (gdbarch, insn1, regs, dsc);
      else /* Generate SP-relative address */
	err = thumb_copy_unmodified_16bit (gdbarch, insn1, "sp-relative", dsc);
      break;
    case 11: /* Misc 16-bit instructions */
      {
	switch (bits (insn1, 8, 11))
	  {
	  case 1: case 3:  case 9: case 11: /* CBNZ, CBZ */
	    err = thumb_copy_cbnz_cbz (gdbarch, insn1, regs, dsc);
	    break;
	  case 12: case 13: /* POP */
	    if (bit (insn1, 8)) /* PC is in register list.  */
	      err = thumb_copy_pop_pc_16bit (gdbarch, insn1, regs, dsc);
	    else
	      err = thumb_copy_unmodified_16bit (gdbarch, insn1, "pop", dsc);
	    break;
	  case 15: /* If-Then, and hints */
	    if (bits (insn1, 0, 3))
	      /* If-Then makes up to four following instructions conditional.
		 IT instruction itself is not conditional, so handle it as a
		 common unmodified instruction.  */
	      err = thumb_copy_unmodified_16bit (gdbarch, insn1, "If-Then",
						 dsc);
	    else
	      err = thumb_copy_unmodified_16bit (gdbarch, insn1, "hints", dsc);
	    break;
	  default:
	    err = thumb_copy_unmodified_16bit (gdbarch, insn1, "misc", dsc);
	  }
      }
      break;
    case 12:
      if (op_bit_10_11 < 2) /* Store multiple registers */
	err = thumb_copy_unmodified_16bit (gdbarch, insn1, "stm", dsc);
      else /* Load multiple registers */
	err = thumb_copy_unmodified_16bit (gdbarch, insn1, "ldm", dsc);
      break;
    case 13: /* Conditional branch and supervisor call */
      if (bits (insn1, 9, 11) != 7) /* conditional branch */
	err = thumb_copy_b (gdbarch, insn1, dsc);
      else
	err = thumb_copy_svc (gdbarch, insn1, regs, dsc);
      break;
    case 14: /* Unconditional branch */
      err = thumb_copy_b (gdbarch, insn1, dsc);
      break;
    default:
      err = 1;
    }

  if (err)
    internal_error (_("thumb_process_displaced_16bit_insn: Instruction decode error"));
}

static int
decode_thumb_32bit_ld_mem_hints (struct gdbarch *gdbarch,
				 uint16_t insn1, uint16_t insn2,
				 struct regcache *regs,
				 arm_displaced_step_copy_insn_closure *dsc)
{
  int rt = bits (insn2, 12, 15);
  int rn = bits (insn1, 0, 3);
  int op1 = bits (insn1, 7, 8);

  switch (bits (insn1, 5, 6))
    {
    case 0: /* Load byte and memory hints */
      if (rt == 0xf) /* PLD/PLI */
	{
	  if (rn == 0xf)
	    /* PLD literal or Encoding T3 of PLI(immediate, literal).  */
	    return thumb2_copy_preload (gdbarch, insn1, insn2, regs, dsc);
	  else
	    return thumb_copy_unmodified_32bit (gdbarch, insn1, insn2,
						"pli/pld", dsc);
	}
      else
	{
	  if (rn == 0xf) /* LDRB/LDRSB (literal) */
	    return thumb2_copy_load_literal (gdbarch, insn1, insn2, regs, dsc,
					     1);
	  else
	    return thumb_copy_unmodified_32bit (gdbarch, insn1, insn2,
						"ldrb{reg, immediate}/ldrbt",
						dsc);
	}

      break;
    case 1: /* Load halfword and memory hints.  */
      if (rt == 0xf) /* PLD{W} and Unalloc memory hint.  */
	return thumb_copy_unmodified_32bit (gdbarch, insn1, insn2,
					    "pld/unalloc memhint", dsc);
      else
	{
	  if (rn == 0xf)
	    return thumb2_copy_load_literal (gdbarch, insn1, insn2, regs, dsc,
					     2);
	  else
	    return thumb_copy_unmodified_32bit (gdbarch, insn1, insn2,
						"ldrh/ldrht", dsc);
	}
      break;
    case 2: /* Load word */
      {
	int insn2_bit_8_11 = bits (insn2, 8, 11);

	if (rn == 0xf)
	  return thumb2_copy_load_literal (gdbarch, insn1, insn2, regs, dsc, 4);
	else if (op1 == 0x1) /* Encoding T3 */
	  return thumb2_copy_load_reg_imm (gdbarch, insn1, insn2, regs, dsc,
					   0, 1);
	else /* op1 == 0x0 */
	  {
	    if (insn2_bit_8_11 == 0xc || (insn2_bit_8_11 & 0x9) == 0x9)
	      /* LDR (immediate) */
	      return thumb2_copy_load_reg_imm (gdbarch, insn1, insn2, regs,
					       dsc, bit (insn2, 8), 1);
	    else if (insn2_bit_8_11 == 0xe) /* LDRT */
	      return thumb_copy_unmodified_32bit (gdbarch, insn1, insn2,
						  "ldrt", dsc);
	    else
	      /* LDR (register) */
	      return thumb2_copy_load_reg_imm (gdbarch, insn1, insn2, regs,
					       dsc, 0, 0);
	  }
	break;
      }
    default:
      return thumb_32bit_copy_undef (gdbarch, insn1, insn2, dsc);
      break;
    }
  return 0;
}

static void
thumb_process_displaced_32bit_insn (struct gdbarch *gdbarch, uint16_t insn1,
				    uint16_t insn2, struct regcache *regs,
				    arm_displaced_step_copy_insn_closure *dsc)
{
  int err = 0;
  unsigned short op = bit (insn2, 15);
  unsigned int op1 = bits (insn1, 11, 12);

  switch (op1)
    {
    case 1:
      {
	switch (bits (insn1, 9, 10))
	  {
	  case 0:
	    if (bit (insn1, 6))
	      {
		/* Load/store {dual, exclusive}, table branch.  */
		if (bits (insn1, 7, 8) == 1 && bits (insn1, 4, 5) == 1
		    && bits (insn2, 5, 7) == 0)
		  err = thumb2_copy_table_branch (gdbarch, insn1, insn2, regs,
						  dsc);
		else
		  /* PC is not allowed to use in load/store {dual, exclusive}
		     instructions.  */
		  err = thumb_copy_unmodified_32bit (gdbarch, insn1, insn2,
						     "load/store dual/ex", dsc);
	      }
	    else /* load/store multiple */
	      {
		switch (bits (insn1, 7, 8))
		  {
		  case 0: case 3: /* SRS, RFE */
		    err = thumb_copy_unmodified_32bit (gdbarch, insn1, insn2,
						       "srs/rfe", dsc);
		    break;
		  case 1: case 2: /* LDM/STM/PUSH/POP */
		    err = thumb2_copy_block_xfer (gdbarch, insn1, insn2, regs, dsc);
		    break;
		  }
	      }
	    break;

	  case 1:
	    /* Data-processing (shift register).  */
	    err = thumb2_decode_dp_shift_reg (gdbarch, insn1, insn2, regs,
					      dsc);
	    break;
	  default: /* Coprocessor instructions.  */
	    err = thumb2_decode_svc_copro (gdbarch, insn1, insn2, regs, dsc);
	    break;
	  }
      break;
      }
    case 2: /* op1 = 2 */
      if (op) /* Branch and misc control.  */
	{
	  if (bit (insn2, 14)  /* BLX/BL */
	      || bit (insn2, 12) /* Unconditional branch */
	      || (bits (insn1, 7, 9) != 0x7)) /* Conditional branch */
	    err = thumb2_copy_b_bl_blx (gdbarch, insn1, insn2, regs, dsc);
	  else
	    err = thumb_copy_unmodified_32bit (gdbarch, insn1, insn2,
					       "misc ctrl", dsc);
	}
      else
	{
	  if (bit (insn1, 9)) /* Data processing (plain binary imm).  */
	    {
	      int dp_op = bits (insn1, 4, 8);
	      int rn = bits (insn1, 0, 3);
	      if ((dp_op == 0 || dp_op == 0xa) && rn == 0xf)
		err = thumb_copy_pc_relative_32bit (gdbarch, insn1, insn2,
						    regs, dsc);
	      else
		err = thumb_copy_unmodified_32bit (gdbarch, insn1, insn2,
						   "dp/pb", dsc);
	    }
	  else /* Data processing (modified immediate) */
	    err = thumb_copy_unmodified_32bit (gdbarch, insn1, insn2,
					       "dp/mi", dsc);
	}
      break;
    case 3: /* op1 = 3 */
      switch (bits (insn1, 9, 10))
	{
	case 0:
	  if (bit (insn1, 4))
	    err = decode_thumb_32bit_ld_mem_hints (gdbarch, insn1, insn2,
						   regs, dsc);
	  else /* NEON Load/Store and Store single data item */
	    err = thumb_copy_unmodified_32bit (gdbarch, insn1, insn2,
					       "neon elt/struct load/store",
					       dsc);
	  break;
	case 1: /* op1 = 3, bits (9, 10) == 1 */
	  switch (bits (insn1, 7, 8))
	    {
	    case 0: case 1: /* Data processing (register) */
	      err = thumb_copy_unmodified_32bit (gdbarch, insn1, insn2,
						 "dp(reg)", dsc);
	      break;
	    case 2: /* Multiply and absolute difference */
	      err = thumb_copy_unmodified_32bit (gdbarch, insn1, insn2,
						 "mul/mua/diff", dsc);
	      break;
	    case 3: /* Long multiply and divide */
	      err = thumb_copy_unmodified_32bit (gdbarch, insn1, insn2,
						 "lmul/lmua", dsc);
	      break;
	    }
	  break;
	default: /* Coprocessor instructions */
	  err = thumb2_decode_svc_copro (gdbarch, insn1, insn2, regs, dsc);
	  break;
	}
      break;
    default:
      err = 1;
    }

  if (err)
    internal_error (_("thumb_process_displaced_32bit_insn: Instruction decode error"));

}

static void
thumb_process_displaced_insn (struct gdbarch *gdbarch, CORE_ADDR from,
			      struct regcache *regs,
			      arm_displaced_step_copy_insn_closure *dsc)
{
  enum bfd_endian byte_order_for_code = gdbarch_byte_order_for_code (gdbarch);
  uint16_t insn1
    = read_memory_unsigned_integer (from, 2, byte_order_for_code);

  displaced_debug_printf ("process thumb insn %.4x at %.8lx",
			  insn1, (unsigned long) from);

  dsc->is_thumb = 1;
  dsc->insn_size = thumb_insn_size (insn1);
  if (thumb_insn_size (insn1) == 4)
    {
      uint16_t insn2
	= read_memory_unsigned_integer (from + 2, 2, byte_order_for_code);
      thumb_process_displaced_32bit_insn (gdbarch, insn1, insn2, regs, dsc);
    }
  else
    thumb_process_displaced_16bit_insn (gdbarch, insn1, regs, dsc);
}

void
arm_process_displaced_insn (struct gdbarch *gdbarch, CORE_ADDR from,
			    CORE_ADDR to, struct regcache *regs,
			    arm_displaced_step_copy_insn_closure *dsc)
{
  int err = 0;
  enum bfd_endian byte_order_for_code = gdbarch_byte_order_for_code (gdbarch);
  uint32_t insn;

  /* Most displaced instructions use a 1-instruction scratch space, so set this
     here and override below if/when necessary.  */
  dsc->numinsns = 1;
  dsc->insn_addr = from;
  dsc->scratch_base = to;
  dsc->cleanup = NULL;
  dsc->wrote_to_pc = 0;

  if (!displaced_in_arm_mode (regs))
    return thumb_process_displaced_insn (gdbarch, from, regs, dsc);

  dsc->is_thumb = 0;
  dsc->insn_size = 4;
  insn = read_memory_unsigned_integer (from, 4, byte_order_for_code);
  displaced_debug_printf ("stepping insn %.8lx at %.8lx",
			  (unsigned long) insn, (unsigned long) from);

  if ((insn & 0xf0000000) == 0xf0000000)
    err = arm_decode_unconditional (gdbarch, insn, regs, dsc);
  else switch (((insn & 0x10) >> 4) | ((insn & 0xe000000) >> 24))
    {
    case 0x0: case 0x1: case 0x2: case 0x3:
      err = arm_decode_dp_misc (gdbarch, insn, regs, dsc);
      break;

    case 0x4: case 0x5: case 0x6:
      err = arm_decode_ld_st_word_ubyte (gdbarch, insn, regs, dsc);
      break;

    case 0x7:
      err = arm_decode_media (gdbarch, insn, dsc);
      break;

    case 0x8: case 0x9: case 0xa: case 0xb:
      err = arm_decode_b_bl_ldmstm (gdbarch, insn, regs, dsc);
      break;

    case 0xc: case 0xd: case 0xe: case 0xf:
      err = arm_decode_svc_copro (gdbarch, insn, regs, dsc);
      break;
    }

  if (err)
    internal_error (_("arm_process_displaced_insn: Instruction decode error"));
}

/* Actually set up the scratch space for a displaced instruction.  */

void
arm_displaced_init_closure (struct gdbarch *gdbarch, CORE_ADDR from,
			    CORE_ADDR to,
			    arm_displaced_step_copy_insn_closure *dsc)
{
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);
  unsigned int i, len, offset;
  enum bfd_endian byte_order_for_code = gdbarch_byte_order_for_code (gdbarch);
  int size = dsc->is_thumb ? 2 : 4;
  const gdb_byte *bkp_insn;

  offset = 0;
  /* Poke modified instruction(s).  */
  for (i = 0; i < dsc->numinsns; i++)
    {
      if (size == 4)
	displaced_debug_printf ("writing insn %.8lx at %.8lx",
				dsc->modinsn[i], (unsigned long) to + offset);
      else if (size == 2)
	displaced_debug_printf ("writing insn %.4x at %.8lx",
				(unsigned short) dsc->modinsn[i],
				(unsigned long) to + offset);

      write_memory_unsigned_integer (to + offset, size,
				     byte_order_for_code,
				     dsc->modinsn[i]);
      offset += size;
    }

  /* Choose the correct breakpoint instruction.  */
  if (dsc->is_thumb)
    {
      bkp_insn = tdep->thumb_breakpoint;
      len = tdep->thumb_breakpoint_size;
    }
  else
    {
      bkp_insn = tdep->arm_breakpoint;
      len = tdep->arm_breakpoint_size;
    }

  /* Put breakpoint afterwards.  */
  write_memory (to + offset, bkp_insn, len);

  displaced_debug_printf ("copy %s->%s", paddress (gdbarch, from),
			  paddress (gdbarch, to));
}

/* Entry point for cleaning things up after a displaced instruction has been
   single-stepped.  */

void
arm_displaced_step_fixup (struct gdbarch *gdbarch,
			  struct displaced_step_copy_insn_closure *dsc_,
			  CORE_ADDR from, CORE_ADDR to,
			  struct regcache *regs, bool completed_p)
{
  /* The following block exists as a temporary measure while displaced
     stepping is fixed architecture at a time within GDB.

     In an earlier implementation of displaced stepping, if GDB thought the
     displaced instruction had not been executed then this fix up function
     was never called.  As a consequence, things that should be fixed by
     this function were left in an unfixed state.

     However, it's not as simple as always calling this function; this
     function needs to be updated to decide what should be fixed up based
     on whether the displaced step executed or not, which requires each
     architecture to be considered individually.

     Until this architecture is updated, this block replicates the old
     behaviour; we just restore the program counter register, and leave
     everything else unfixed.  */
  if (!completed_p)
    {
      CORE_ADDR pc = regcache_read_pc (regs);
      pc = from + (pc - to);
      regcache_write_pc (regs, pc);
      return;
    }

  arm_displaced_step_copy_insn_closure *dsc
    = (arm_displaced_step_copy_insn_closure *) dsc_;

  if (dsc->cleanup)
    dsc->cleanup (gdbarch, regs, dsc);

  if (!dsc->wrote_to_pc)
    regcache_cooked_write_unsigned (regs, ARM_PC_REGNUM,
				    dsc->insn_addr + dsc->insn_size);

}

#include "bfd-in2.h"
#include "libcoff.h"

static int
gdb_print_insn_arm (bfd_vma memaddr, disassemble_info *info)
{
  gdb_disassemble_info *di
    = static_cast<gdb_disassemble_info *> (info->application_data);
  struct gdbarch *gdbarch = di->arch ();

  if (arm_pc_is_thumb (gdbarch, memaddr))
    {
      static asymbol *asym;
      static combined_entry_type ce;
      static struct coff_symbol_struct csym;
      static struct bfd fake_bfd;
      static bfd_target fake_target;

      if (csym.native == NULL)
	{
	  /* Create a fake symbol vector containing a Thumb symbol.
	     This is solely so that the code in print_insn_little_arm() 
	     and print_insn_big_arm() in opcodes/arm-dis.c will detect
	     the presence of a Thumb symbol and switch to decoding
	     Thumb instructions.  */

	  fake_target.flavour = bfd_target_coff_flavour;
	  fake_bfd.xvec = &fake_target;
	  ce.u.syment.n_sclass = C_THUMBEXTFUNC;
	  csym.native = &ce;
	  csym.symbol.the_bfd = &fake_bfd;
	  csym.symbol.name = "fake";
	  asym = (asymbol *) & csym;
	}

      memaddr = UNMAKE_THUMB_ADDR (memaddr);
      info->symbols = &asym;
    }
  else
    info->symbols = NULL;

  /* GDB is able to get bfd_mach from the exe_bfd, info->mach is
     accurate, so mark USER_SPECIFIED_MACHINE_TYPE bit.  Otherwise,
     opcodes/arm-dis.c:print_insn reset info->mach, and it will trigger
     the assert on the mismatch of info->mach and
     bfd_get_mach (current_program_space->exec_bfd ()) in
     default_print_insn.  */
  if (current_program_space->exec_bfd () != NULL
      && (current_program_space->exec_bfd ()->arch_info
	  == gdbarch_bfd_arch_info (gdbarch)))
    info->flags |= USER_SPECIFIED_MACHINE_TYPE;

  return default_print_insn (memaddr, info);
}

/* The following define instruction sequences that will cause ARM
   cpu's to take an undefined instruction trap.  These are used to
   signal a breakpoint to GDB.
   
   The newer ARMv4T cpu's are capable of operating in ARM or Thumb
   modes.  A different instruction is required for each mode.  The ARM
   cpu's can also be big or little endian.  Thus four different
   instructions are needed to support all cases.
   
   Note: ARMv4 defines several new instructions that will take the
   undefined instruction trap.  ARM7TDMI is nominally ARMv4T, but does
   not in fact add the new instructions.  The new undefined
   instructions in ARMv4 are all instructions that had no defined
   behaviour in earlier chips.  There is no guarantee that they will
   raise an exception, but may be treated as NOP's.  In practice, it
   may only safe to rely on instructions matching:
   
   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1 
   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
   C C C C 0 1 1 x x x x x x x x x x x x x x x x x x x x 1 x x x x
   
   Even this may only true if the condition predicate is true.  The
   following use a condition predicate of ALWAYS so it is always TRUE.
   
   There are other ways of forcing a breakpoint.  GNU/Linux, RISC iX,
   and NetBSD all use a software interrupt rather than an undefined
   instruction to force a trap.  This can be handled by by the
   abi-specific code during establishment of the gdbarch vector.  */

#define ARM_LE_BREAKPOINT {0xFE,0xDE,0xFF,0xE7}
#define ARM_BE_BREAKPOINT {0xE7,0xFF,0xDE,0xFE}
#define THUMB_LE_BREAKPOINT {0xbe,0xbe}
#define THUMB_BE_BREAKPOINT {0xbe,0xbe}

static const gdb_byte arm_default_arm_le_breakpoint[] = ARM_LE_BREAKPOINT;
static const gdb_byte arm_default_arm_be_breakpoint[] = ARM_BE_BREAKPOINT;
static const gdb_byte arm_default_thumb_le_breakpoint[] = THUMB_LE_BREAKPOINT;
static const gdb_byte arm_default_thumb_be_breakpoint[] = THUMB_BE_BREAKPOINT;

/* Implement the breakpoint_kind_from_pc gdbarch method.  */

static int
arm_breakpoint_kind_from_pc (struct gdbarch *gdbarch, CORE_ADDR *pcptr)
{
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);
  enum bfd_endian byte_order_for_code = gdbarch_byte_order_for_code (gdbarch);

  if (arm_pc_is_thumb (gdbarch, *pcptr))
    {
      *pcptr = UNMAKE_THUMB_ADDR (*pcptr);

      /* If we have a separate 32-bit breakpoint instruction for Thumb-2,
	 check whether we are replacing a 32-bit instruction.  */
      if (tdep->thumb2_breakpoint != NULL)
	{
	  gdb_byte buf[2];

	  if (target_read_memory (*pcptr, buf, 2) == 0)
	    {
	      unsigned short inst1;

	      inst1 = extract_unsigned_integer (buf, 2, byte_order_for_code);
	      if (thumb_insn_size (inst1) == 4)
		return ARM_BP_KIND_THUMB2;
	    }
	}

      return ARM_BP_KIND_THUMB;
    }
  else
    return ARM_BP_KIND_ARM;

}

/* Implement the sw_breakpoint_from_kind gdbarch method.  */

static const gdb_byte *
arm_sw_breakpoint_from_kind (struct gdbarch *gdbarch, int kind, int *size)
{
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  switch (kind)
    {
    case ARM_BP_KIND_ARM:
      *size = tdep->arm_breakpoint_size;
      return tdep->arm_breakpoint;
    case ARM_BP_KIND_THUMB:
      *size = tdep->thumb_breakpoint_size;
      return tdep->thumb_breakpoint;
    case ARM_BP_KIND_THUMB2:
      *size = tdep->thumb2_breakpoint_size;
      return tdep->thumb2_breakpoint;
    default:
      gdb_assert_not_reached ("unexpected arm breakpoint kind");
    }
}

/* Implement the breakpoint_kind_from_current_state gdbarch method.  */

static int
arm_breakpoint_kind_from_current_state (struct gdbarch *gdbarch,
					struct regcache *regcache,
					CORE_ADDR *pcptr)
{
  gdb_byte buf[4];

  /* Check the memory pointed by PC is readable.  */
  if (target_read_memory (regcache_read_pc (regcache), buf, 4) == 0)
    {
      struct arm_get_next_pcs next_pcs_ctx;

      arm_get_next_pcs_ctor (&next_pcs_ctx,
			     &arm_get_next_pcs_ops,
			     gdbarch_byte_order (gdbarch),
			     gdbarch_byte_order_for_code (gdbarch),
			     0,
			     regcache);

      std::vector<CORE_ADDR> next_pcs = arm_get_next_pcs (&next_pcs_ctx);

      /* If MEMADDR is the next instruction of current pc, do the
	 software single step computation, and get the thumb mode by
	 the destination address.  */
      for (CORE_ADDR pc : next_pcs)
	{
	  if (UNMAKE_THUMB_ADDR (pc) == *pcptr)
	    {
	      if (IS_THUMB_ADDR (pc))
		{
		  *pcptr = MAKE_THUMB_ADDR (*pcptr);
		  return arm_breakpoint_kind_from_pc (gdbarch, pcptr);
		}
	      else
		return ARM_BP_KIND_ARM;
	    }
	}
    }

  return arm_breakpoint_kind_from_pc (gdbarch, pcptr);
}

/* Extract from an array REGBUF containing the (raw) register state a
   function return value of type TYPE, and copy that, in virtual
   format, into VALBUF.  */

static void
arm_extract_return_value (struct type *type, struct regcache *regs,
			  gdb_byte *valbuf)
{
  struct gdbarch *gdbarch = regs->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  while (type->code () == TYPE_CODE_RANGE)
    type = check_typedef (type->target_type ());

  if (TYPE_CODE_FLT == type->code ())
    {
      switch (tdep->fp_model)
	{
	case ARM_FLOAT_FPA:
	  {
	    /* The value is in register F0 in internal format.  We need to
	       extract the raw value and then convert it to the desired
	       internal type.  */
	    bfd_byte tmpbuf[ARM_FP_REGISTER_SIZE];

	    regs->cooked_read (ARM_F0_REGNUM, tmpbuf);
	    target_float_convert (tmpbuf, arm_ext_type (gdbarch),
				  valbuf, type);
	  }
	  break;

	case ARM_FLOAT_SOFT_FPA:
	case ARM_FLOAT_SOFT_VFP:
	  /* ARM_FLOAT_VFP can arise if this is a variadic function so
	     not using the VFP ABI code.  */
	case ARM_FLOAT_VFP:
	  regs->cooked_read (ARM_A1_REGNUM, valbuf);
	  if (type->length () > 4)
	    regs->cooked_read (ARM_A1_REGNUM + 1,
			       valbuf + ARM_INT_REGISTER_SIZE);
	  break;

	default:
	  internal_error (_("arm_extract_return_value: "
			    "Floating point model not supported"));
	  break;
	}
    }
  else if (type->code () == TYPE_CODE_INT
	   || type->code () == TYPE_CODE_CHAR
	   || type->code () == TYPE_CODE_BOOL
	   || type->code () == TYPE_CODE_PTR
	   || TYPE_IS_REFERENCE (type)
	   || type->code () == TYPE_CODE_ENUM
	   || is_fixed_point_type (type))
    {
      /* If the type is a plain integer, then the access is
	 straight-forward.  Otherwise we have to play around a bit
	 more.  */
      int len = type->length ();
      int regno = ARM_A1_REGNUM;
      ULONGEST tmp;

      while (len > 0)
	{
	  /* By using store_unsigned_integer we avoid having to do
	     anything special for small big-endian values.  */
	  regcache_cooked_read_unsigned (regs, regno++, &tmp);
	  store_unsigned_integer (valbuf, 
				  (len > ARM_INT_REGISTER_SIZE
				   ? ARM_INT_REGISTER_SIZE : len),
				  byte_order, tmp);
	  len -= ARM_INT_REGISTER_SIZE;
	  valbuf += ARM_INT_REGISTER_SIZE;
	}
    }
  else
    {
      /* For a structure or union the behaviour is as if the value had
	 been stored to word-aligned memory and then loaded into 
	 registers with 32-bit load instruction(s).  */
      int len = type->length ();
      int regno = ARM_A1_REGNUM;
      bfd_byte tmpbuf[ARM_INT_REGISTER_SIZE];

      while (len > 0)
	{
	  regs->cooked_read (regno++, tmpbuf);
	  memcpy (valbuf, tmpbuf,
		  len > ARM_INT_REGISTER_SIZE ? ARM_INT_REGISTER_SIZE : len);
	  len -= ARM_INT_REGISTER_SIZE;
	  valbuf += ARM_INT_REGISTER_SIZE;
	}
    }
}


/* Will a function return an aggregate type in memory or in a
   register?  Return 0 if an aggregate type can be returned in a
   register, 1 if it must be returned in memory.  */

static int
arm_return_in_memory (struct gdbarch *gdbarch, struct type *type)
{
  enum type_code code;

  type = check_typedef (type);

  /* Simple, non-aggregate types (ie not including vectors and
     complex) are always returned in a register (or registers).  */
  code = type->code ();
  if (TYPE_CODE_STRUCT != code && TYPE_CODE_UNION != code
      && TYPE_CODE_ARRAY != code && TYPE_CODE_COMPLEX != code)
    return 0;

  if (TYPE_HAS_DYNAMIC_LENGTH (type))
    return 1;

  if (TYPE_CODE_ARRAY == code && type->is_vector ())
    {
      /* Vector values should be returned using ARM registers if they
	 are not over 16 bytes.  */
      return (type->length () > 16);
    }

  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);
  if (tdep->arm_abi != ARM_ABI_APCS)
    {
      /* The AAPCS says all aggregates not larger than a word are returned
	 in a register.  */
      if (type->length () <= ARM_INT_REGISTER_SIZE
	  && language_pass_by_reference (type).trivially_copyable)
	return 0;

      return 1;
    }
  else
    {
      int nRc;

      /* All aggregate types that won't fit in a register must be returned
	 in memory.  */
      if (type->length () > ARM_INT_REGISTER_SIZE
	  || !language_pass_by_reference (type).trivially_copyable)
	return 1;

      /* In the ARM ABI, "integer" like aggregate types are returned in
	 registers.  For an aggregate type to be integer like, its size
	 must be less than or equal to ARM_INT_REGISTER_SIZE and the
	 offset of each addressable subfield must be zero.  Note that bit
	 fields are not addressable, and all addressable subfields of
	 unions always start at offset zero.

	 This function is based on the behaviour of GCC 2.95.1.
	 See: gcc/arm.c: arm_return_in_memory() for details.

	 Note: All versions of GCC before GCC 2.95.2 do not set up the
	 parameters correctly for a function returning the following
	 structure: struct { float f;}; This should be returned in memory,
	 not a register.  Richard Earnshaw sent me a patch, but I do not
	 know of any way to detect if a function like the above has been
	 compiled with the correct calling convention.  */

      /* Assume all other aggregate types can be returned in a register.
	 Run a check for structures, unions and arrays.  */
      nRc = 0;

      if ((TYPE_CODE_STRUCT == code) || (TYPE_CODE_UNION == code))
	{
	  int i;
	  /* Need to check if this struct/union is "integer" like.  For
	     this to be true, its size must be less than or equal to
	     ARM_INT_REGISTER_SIZE and the offset of each addressable
	     subfield must be zero.  Note that bit fields are not
	     addressable, and unions always start at offset zero.  If any
	     of the subfields is a floating point type, the struct/union
	     cannot be an integer type.  */

	  /* For each field in the object, check:
	     1) Is it FP? --> yes, nRc = 1;
	     2) Is it addressable (bitpos != 0) and
	     not packed (bitsize == 0)?
	     --> yes, nRc = 1
	  */

	  for (i = 0; i < type->num_fields (); i++)
	    {
	      enum type_code field_type_code;

	      field_type_code
		= check_typedef (type->field (i).type ())->code ();

	      /* Is it a floating point type field?  */
	      if (field_type_code == TYPE_CODE_FLT)
		{
		  nRc = 1;
		  break;
		}

	      /* If bitpos != 0, then we have to care about it.  */
	      if (type->field (i).loc_bitpos () != 0)
		{
		  /* Bitfields are not addressable.  If the field bitsize is 
		     zero, then the field is not packed.  Hence it cannot be
		     a bitfield or any other packed type.  */
		  if (type->field (i).bitsize () == 0)
		    {
		      nRc = 1;
		      break;
		    }
		}
	    }
	}

      return nRc;
    }
}

/* Write into appropriate registers a function return value of type
   TYPE, given in virtual format.  */

static void
arm_store_return_value (struct type *type, struct regcache *regs,
			const gdb_byte *valbuf)
{
  struct gdbarch *gdbarch = regs->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  while (type->code () == TYPE_CODE_RANGE)
    type = check_typedef (type->target_type ());

  if (type->code () == TYPE_CODE_FLT)
    {
      gdb_byte buf[ARM_FP_REGISTER_SIZE];
      arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

      switch (tdep->fp_model)
	{
	case ARM_FLOAT_FPA:

	  target_float_convert (valbuf, type, buf, arm_ext_type (gdbarch));
	  regs->cooked_write (ARM_F0_REGNUM, buf);
	  break;

	case ARM_FLOAT_SOFT_FPA:
	case ARM_FLOAT_SOFT_VFP:
	  /* ARM_FLOAT_VFP can arise if this is a variadic function so
	     not using the VFP ABI code.  */
	case ARM_FLOAT_VFP:
	  regs->cooked_write (ARM_A1_REGNUM, valbuf);
	  if (type->length () > 4)
	    regs->cooked_write (ARM_A1_REGNUM + 1,
				valbuf + ARM_INT_REGISTER_SIZE);
	  break;

	default:
	  internal_error (_("arm_store_return_value: Floating "
			    "point model not supported"));
	  break;
	}
    }
  else if (type->code () == TYPE_CODE_INT
	   || type->code () == TYPE_CODE_CHAR
	   || type->code () == TYPE_CODE_BOOL
	   || type->code () == TYPE_CODE_PTR
	   || TYPE_IS_REFERENCE (type)
	   || type->code () == TYPE_CODE_ENUM
	   || is_fixed_point_type (type))
    {
      if (type->length () <= 4)
	{
	  /* Values of one word or less are zero/sign-extended and
	     returned in r0.  */
	  bfd_byte tmpbuf[ARM_INT_REGISTER_SIZE];

	  if (is_fixed_point_type (type))
	    {
	      gdb_mpz unscaled;
	      unscaled.read (gdb::make_array_view (valbuf, type->length ()),
			     byte_order, type->is_unsigned ());
	      unscaled.write (gdb::make_array_view (tmpbuf, sizeof (tmpbuf)),
			      byte_order, type->is_unsigned ());
	    }
	  else
	    {
	      LONGEST val = unpack_long (type, valbuf);
	      store_signed_integer (tmpbuf, ARM_INT_REGISTER_SIZE, byte_order, val);
	    }
	  regs->cooked_write (ARM_A1_REGNUM, tmpbuf);
	}
      else
	{
	  /* Integral values greater than one word are stored in consecutive
	     registers starting with r0.  This will always be a multiple of
	     the regiser size.  */
	  int len = type->length ();
	  int regno = ARM_A1_REGNUM;

	  while (len > 0)
	    {
	      regs->cooked_write (regno++, valbuf);
	      len -= ARM_INT_REGISTER_SIZE;
	      valbuf += ARM_INT_REGISTER_SIZE;
	    }
	}
    }
  else
    {
      /* For a structure or union the behaviour is as if the value had
	 been stored to word-aligned memory and then loaded into 
	 registers with 32-bit load instruction(s).  */
      int len = type->length ();
      int regno = ARM_A1_REGNUM;
      bfd_byte tmpbuf[ARM_INT_REGISTER_SIZE];

      while (len > 0)
	{
	  memcpy (tmpbuf, valbuf,
		  len > ARM_INT_REGISTER_SIZE ? ARM_INT_REGISTER_SIZE : len);
	  regs->cooked_write (regno++, tmpbuf);
	  len -= ARM_INT_REGISTER_SIZE;
	  valbuf += ARM_INT_REGISTER_SIZE;
	}
    }
}


/* Handle function return values.  */

static enum return_value_convention
arm_return_value (struct gdbarch *gdbarch, struct value *function,
		  struct type *valtype, struct regcache *regcache,
		  struct value **read_value, const gdb_byte *writebuf)
{
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);
  struct type *func_type = function ? function->type () : NULL;
  enum arm_vfp_cprc_base_type vfp_base_type;
  int vfp_base_count;

  if (arm_vfp_abi_for_function (gdbarch, func_type)
      && arm_vfp_call_candidate (valtype, &vfp_base_type, &vfp_base_count))
    {
      int reg_char = arm_vfp_cprc_reg_char (vfp_base_type);
      int unit_length = arm_vfp_cprc_unit_length (vfp_base_type);
      int i;

      gdb_byte *readbuf = nullptr;
      if (read_value != nullptr)
	{
	  *read_value = value::allocate (valtype);
	  readbuf = (*read_value)->contents_raw ().data ();
	}

      for (i = 0; i < vfp_base_count; i++)
	{
	  if (reg_char == 'q')
	    {
	      if (writebuf)
		arm_neon_quad_write (gdbarch, regcache, i,
				     writebuf + i * unit_length);

	      if (readbuf)
		arm_neon_quad_read (gdbarch, regcache, i,
				    readbuf + i * unit_length);
	    }
	  else
	    {
	      char name_buf[4];
	      int regnum;

	      xsnprintf (name_buf, sizeof (name_buf), "%c%d", reg_char, i);
	      regnum = user_reg_map_name_to_regnum (gdbarch, name_buf,
						    strlen (name_buf));
	      if (writebuf)
		regcache->cooked_write (regnum, writebuf + i * unit_length);
	      if (readbuf)
		regcache->cooked_read (regnum, readbuf + i * unit_length);
	    }
	}
      return RETURN_VALUE_REGISTER_CONVENTION;
    }

  if (valtype->code () == TYPE_CODE_STRUCT
      || valtype->code () == TYPE_CODE_UNION
      || valtype->code () == TYPE_CODE_ARRAY)
    {
      /* From the AAPCS document:

	 Result return:

	 A Composite Type larger than 4 bytes, or whose size cannot be
	 determined statically by both caller and callee, is stored in memory
	 at an address passed as an extra argument when the function was
	 called (Parameter Passing, rule A.4). The memory to be used for the
	 result may be modified at any point during the function call.

	 Parameter Passing:

	 A.4: If the subroutine is a function that returns a result in memory,
	 then the address for the result is placed in r0 and the NCRN is set
	 to r1.  */
      if (tdep->struct_return == pcc_struct_return
	  || arm_return_in_memory (gdbarch, valtype))
	{
	  if (read_value != nullptr)
	    {
	      CORE_ADDR addr;

	      regcache->cooked_read (ARM_A1_REGNUM, &addr);
	      *read_value = value_at_non_lval (valtype, addr);
	    }
	  return RETURN_VALUE_ABI_RETURNS_ADDRESS;
	}
    }
  else if (valtype->code () == TYPE_CODE_COMPLEX)
    {
      if (arm_return_in_memory (gdbarch, valtype))
	return RETURN_VALUE_STRUCT_CONVENTION;
    }

  if (writebuf)
    arm_store_return_value (valtype, regcache, writebuf);

  if (read_value != nullptr)
    {
      *read_value = value::allocate (valtype);
      gdb_byte *readbuf = (*read_value)->contents_raw ().data ();
      arm_extract_return_value (valtype, regcache, readbuf);
    }

  return RETURN_VALUE_REGISTER_CONVENTION;
}


static int
arm_get_longjmp_target (frame_info_ptr frame, CORE_ADDR *pc)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR jb_addr;
  gdb_byte buf[ARM_INT_REGISTER_SIZE];
  
  jb_addr = get_frame_register_unsigned (frame, ARM_A1_REGNUM);

  if (target_read_memory (jb_addr + tdep->jb_pc * tdep->jb_elt_size, buf,
			  ARM_INT_REGISTER_SIZE))
    return 0;

  *pc = extract_unsigned_integer (buf, ARM_INT_REGISTER_SIZE, byte_order);
  return 1;
}
/* A call to cmse secure entry function "foo" at "a" is modified by
     GNU ld as "b".
     a) bl xxxx <foo>

     <foo>
     xxxx:

     b) bl yyyy <__acle_se_foo>

     section .gnu.sgstubs:
     <foo>
     yyyy: sg   // secure gateway
	   b.w xxxx <__acle_se_foo>  // original_branch_dest

     <__acle_se_foo>
     xxxx:

  When the control at "b", the pc contains "yyyy" (sg address) which is a
  trampoline and does not exist in source code.  This function returns the
  target pc "xxxx".  For more details please refer to section 5.4
  (Entry functions) and section 3.4.4 (C level development flow of secure code)
  of "armv8-m-security-extensions-requirements-on-development-tools-engineering-specification"
  document on www.developer.arm.com.  */

static CORE_ADDR
arm_skip_cmse_entry (CORE_ADDR pc, const char *name, struct objfile *objfile)
{
  int target_len = strlen (name) + strlen ("__acle_se_") + 1;
  char *target_name = (char *) alloca (target_len);
  xsnprintf (target_name, target_len, "%s%s", "__acle_se_", name);

  struct bound_minimal_symbol minsym
   = lookup_minimal_symbol (target_name, NULL, objfile);

  if (minsym.minsym != nullptr)
    return minsym.value_address ();

  return 0;
}

/* Return true when SEC points to ".gnu.sgstubs" section.  */

static bool
arm_is_sgstubs_section (struct obj_section *sec)
{
  return (sec != nullptr
	  && sec->the_bfd_section != nullptr
	  && sec->the_bfd_section->name != nullptr
	  && streq (sec->the_bfd_section->name, ".gnu.sgstubs"));
}

/* Recognize GCC and GNU ld's trampolines.  If we are in a trampoline,
   return the target PC.  Otherwise return 0.  */

CORE_ADDR
arm_skip_stub (frame_info_ptr frame, CORE_ADDR pc)
{
  const char *name;
  int namelen;
  CORE_ADDR start_addr;

  /* Find the starting address and name of the function containing the PC.  */
  if (find_pc_partial_function (pc, &name, &start_addr, NULL) == 0)
    {
      /* Trampoline 'bx reg' doesn't belong to any functions.  Do the
	 check here.  */
      start_addr = arm_skip_bx_reg (frame, pc);
      if (start_addr != 0)
	return start_addr;

      return 0;
    }

  /* If PC is in a Thumb call or return stub, return the address of the
     target PC, which is in a register.  The thunk functions are called
     _call_via_xx, where x is the register name.  The possible names
     are r0-r9, sl, fp, ip, sp, and lr.  ARM RealView has similar
     functions, named __ARM_call_via_r[0-7].  */
  if (startswith (name, "_call_via_")
      || startswith (name, "__ARM_call_via_"))
    {
      /* Use the name suffix to determine which register contains the
	 target PC.  */
      static const char *table[15] =
      {"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
       "r8", "r9", "sl", "fp", "ip", "sp", "lr"
      };
      int regno;
      int offset = strlen (name) - 2;

      for (regno = 0; regno <= 14; regno++)
	if (strcmp (&name[offset], table[regno]) == 0)
	  return get_frame_register_unsigned (frame, regno);
    }

  /* GNU ld generates __foo_from_arm or __foo_from_thumb for
     non-interworking calls to foo.  We could decode the stubs
     to find the target but it's easier to use the symbol table.  */
  namelen = strlen (name);
  if (name[0] == '_' && name[1] == '_'
      && ((namelen > 2 + strlen ("_from_thumb")
	   && startswith (name + namelen - strlen ("_from_thumb"), "_from_thumb"))
	  || (namelen > 2 + strlen ("_from_arm")
	      && startswith (name + namelen - strlen ("_from_arm"), "_from_arm"))))
    {
      char *target_name;
      int target_len = namelen - 2;
      struct bound_minimal_symbol minsym;
      struct objfile *objfile;
      struct obj_section *sec;

      if (name[namelen - 1] == 'b')
	target_len -= strlen ("_from_thumb");
      else
	target_len -= strlen ("_from_arm");

      target_name = (char *) alloca (target_len + 1);
      memcpy (target_name, name + 2, target_len);
      target_name[target_len] = '\0';

      sec = find_pc_section (pc);
      objfile = (sec == NULL) ? NULL : sec->objfile;
      minsym = lookup_minimal_symbol (target_name, NULL, objfile);
      if (minsym.minsym != NULL)
	return minsym.value_address ();
      else
	return 0;
    }

  struct obj_section *section = find_pc_section (pc);

  /* Check whether SECTION points to the ".gnu.sgstubs" section.  */
  if (arm_is_sgstubs_section (section))
    return arm_skip_cmse_entry (pc, name, section->objfile);

  return 0;			/* not a stub */
}

static void
arm_update_current_architecture (void)
{
  /* If the current architecture is not ARM, we have nothing to do.  */
  gdbarch *arch = current_inferior ()->arch ();
  if (gdbarch_bfd_arch_info (arch)->arch != bfd_arch_arm)
    return;

  /* Update the architecture.  */
  gdbarch_info info;
  if (!gdbarch_update_p (info))
    internal_error (_("could not update architecture"));
}

static void
set_fp_model_sfunc (const char *args, int from_tty,
		    struct cmd_list_element *c)
{
  int fp_model;

  for (fp_model = ARM_FLOAT_AUTO; fp_model != ARM_FLOAT_LAST; fp_model++)
    if (strcmp (current_fp_model, fp_model_strings[fp_model]) == 0)
      {
	arm_fp_model = (enum arm_float_model) fp_model;
	break;
      }

  if (fp_model == ARM_FLOAT_LAST)
    internal_error (_("Invalid fp model accepted: %s."),
		    current_fp_model);

  arm_update_current_architecture ();
}

static void
show_fp_model (struct ui_file *file, int from_tty,
	       struct cmd_list_element *c, const char *value)
{
  gdbarch *arch = current_inferior ()->arch ();
  if (arm_fp_model == ARM_FLOAT_AUTO
      && gdbarch_bfd_arch_info (arch)->arch == bfd_arch_arm)
    {
      arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (arch);

      gdb_printf (file, _("\
The current ARM floating point model is \"auto\" (currently \"%s\").\n"),
		  fp_model_strings[tdep->fp_model]);
    }
  else
    gdb_printf (file, _("\
The current ARM floating point model is \"%s\".\n"),
		fp_model_strings[arm_fp_model]);
}

static void
arm_set_abi (const char *args, int from_tty,
	     struct cmd_list_element *c)
{
  int arm_abi;

  for (arm_abi = ARM_ABI_AUTO; arm_abi != ARM_ABI_LAST; arm_abi++)
    if (strcmp (arm_abi_string, arm_abi_strings[arm_abi]) == 0)
      {
	arm_abi_global = (enum arm_abi_kind) arm_abi;
	break;
      }

  if (arm_abi == ARM_ABI_LAST)
    internal_error (_("Invalid ABI accepted: %s."),
		    arm_abi_string);

  arm_update_current_architecture ();
}

static void
arm_show_abi (struct ui_file *file, int from_tty,
	     struct cmd_list_element *c, const char *value)
{
  gdbarch *arch = current_inferior ()->arch ();
  if (arm_abi_global == ARM_ABI_AUTO
      && gdbarch_bfd_arch_info (arch)->arch == bfd_arch_arm)
    {
      arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (arch);

      gdb_printf (file, _("\
The current ARM ABI is \"auto\" (currently \"%s\").\n"),
		  arm_abi_strings[tdep->arm_abi]);
    }
  else
    gdb_printf (file, _("The current ARM ABI is \"%s\".\n"),
		arm_abi_string);
}

static void
arm_show_fallback_mode (struct ui_file *file, int from_tty,
			struct cmd_list_element *c, const char *value)
{
  gdb_printf (file,
	      _("The current execution mode assumed "
		"(when symbols are unavailable) is \"%s\".\n"),
	      arm_fallback_mode_string);
}

static void
arm_show_force_mode (struct ui_file *file, int from_tty,
		     struct cmd_list_element *c, const char *value)
{
  gdb_printf (file,
	      _("The current execution mode assumed "
		"(even when symbols are available) is \"%s\".\n"),
	      arm_force_mode_string);
}

static void
arm_show_unwind_secure_frames (struct ui_file *file, int from_tty,
			 struct cmd_list_element *c, const char *value)
{
  gdb_printf (file,
	      _("Usage of non-secure to secure exception stack unwinding is %s.\n"),
	      arm_unwind_secure_frames ? "on" : "off");
}

/* If the user changes the register disassembly style used for info
   register and other commands, we have to also switch the style used
   in opcodes for disassembly output.  This function is run in the "set
   arm disassembly" command, and does that.  */

static void
set_disassembly_style_sfunc (const char *args, int from_tty,
			     struct cmd_list_element *c)
{
  /* Convert the short style name into the long style name (eg, reg-names-*)
     before calling the generic set_disassembler_options() function.  */
  std::string long_name = std::string ("reg-names-") + disassembly_style;
  set_disassembler_options (&long_name[0]);
}

static void
show_disassembly_style_sfunc (struct ui_file *file, int from_tty,
			      struct cmd_list_element *c, const char *value)
{
  struct gdbarch *gdbarch = get_current_arch ();
  char *options = get_disassembler_options (gdbarch);
  const char *style = "";
  int len = 0;
  const char *opt;

  FOR_EACH_DISASSEMBLER_OPTION (opt, options)
    if (startswith (opt, "reg-names-"))
      {
	style = &opt[strlen ("reg-names-")];
	len = strcspn (style, ",");
      }

  gdb_printf (file, "The disassembly style is \"%.*s\".\n", len, style);
}

/* Return the ARM register name corresponding to register I.  */
static const char *
arm_register_name (struct gdbarch *gdbarch, int i)
{
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  if (is_s_pseudo (gdbarch, i))
    {
      static const char *const s_pseudo_names[] = {
	"s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
	"s8", "s9", "s10", "s11", "s12", "s13", "s14", "s15",
	"s16", "s17", "s18", "s19", "s20", "s21", "s22", "s23",
	"s24", "s25", "s26", "s27", "s28", "s29", "s30", "s31",
      };

      return s_pseudo_names[i - tdep->s_pseudo_base];
    }

  if (is_q_pseudo (gdbarch, i))
    {
      static const char *const q_pseudo_names[] = {
	"q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7",
	"q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15",
      };

      return q_pseudo_names[i - tdep->q_pseudo_base];
    }

  if (is_mve_pseudo (gdbarch, i))
    return "p0";

  /* RA_AUTH_CODE is used for unwinding only.  Do not assign it a name.  */
  if (is_pacbti_pseudo (gdbarch, i))
    return "";

  if (i >= ARRAY_SIZE (arm_register_names))
    /* These registers are only supported on targets which supply
       an XML description.  */
    return "";

  /* Non-pseudo registers.  */
  return arm_register_names[i];
}

/* Test whether the coff symbol specific value corresponds to a Thumb
   function.  */

static int
coff_sym_is_thumb (int val)
{
  return (val == C_THUMBEXT
	  || val == C_THUMBSTAT
	  || val == C_THUMBEXTFUNC
	  || val == C_THUMBSTATFUNC
	  || val == C_THUMBLABEL);
}

/* arm_coff_make_msymbol_special()
   arm_elf_make_msymbol_special()
   
   These functions test whether the COFF or ELF symbol corresponds to
   an address in thumb code, and set a "special" bit in a minimal
   symbol to indicate that it does.  */
   
static void
arm_elf_make_msymbol_special(asymbol *sym, struct minimal_symbol *msym)
{
  elf_symbol_type *elfsym = (elf_symbol_type *) sym;

  if (ARM_GET_SYM_BRANCH_TYPE (elfsym->internal_elf_sym.st_target_internal)
      == ST_BRANCH_TO_THUMB)
    MSYMBOL_SET_SPECIAL (msym);
}

static void
arm_coff_make_msymbol_special(int val, struct minimal_symbol *msym)
{
  if (coff_sym_is_thumb (val))
    MSYMBOL_SET_SPECIAL (msym);
}

static void
arm_record_special_symbol (struct gdbarch *gdbarch, struct objfile *objfile,
			   asymbol *sym)
{
  const char *name = bfd_asymbol_name (sym);
  struct arm_per_bfd *data;
  struct arm_mapping_symbol new_map_sym;

  gdb_assert (name[0] == '$');
  if (name[1] != 'a' && name[1] != 't' && name[1] != 'd')
    return;

  data = arm_bfd_data_key.get (objfile->obfd.get ());
  if (data == NULL)
    data = arm_bfd_data_key.emplace (objfile->obfd.get (),
				     objfile->obfd->section_count);
  arm_mapping_symbol_vec &map
    = data->section_maps[bfd_asymbol_section (sym)->index];

  new_map_sym.value = sym->value;
  new_map_sym.type = name[1];

  /* Insert at the end, the vector will be sorted on first use.  */
  map.push_back (new_map_sym);
}

static void
arm_write_pc (struct regcache *regcache, CORE_ADDR pc)
{
  struct gdbarch *gdbarch = regcache->arch ();
  regcache_cooked_write_unsigned (regcache, ARM_PC_REGNUM, pc);

  /* If necessary, set the T bit.  */
  if (arm_apcs_32)
    {
      ULONGEST val, t_bit;
      regcache_cooked_read_unsigned (regcache, ARM_PS_REGNUM, &val);
      t_bit = arm_psr_thumb_bit (gdbarch);
      if (arm_pc_is_thumb (gdbarch, pc))
	regcache_cooked_write_unsigned (regcache, ARM_PS_REGNUM,
					val | t_bit);
      else
	regcache_cooked_write_unsigned (regcache, ARM_PS_REGNUM,
					val & ~t_bit);
    }
}

/* Read the contents of a NEON quad register, by reading from two
   double registers.  This is used to implement the quad pseudo
   registers, and for argument passing in case the quad registers are
   missing; vectors are passed in quad registers when using the VFP
   ABI, even if a NEON unit is not present.  REGNUM is the index of
   the quad register, in [0, 15].  */

static enum register_status
arm_neon_quad_read (struct gdbarch *gdbarch, readable_regcache *regcache,
		    int regnum, gdb_byte *buf)
{
  char name_buf[4];
  gdb_byte reg_buf[8];
  int double_regnum;
  enum register_status status;

  xsnprintf (name_buf, sizeof (name_buf), "d%d", regnum << 1);
  double_regnum = user_reg_map_name_to_regnum (gdbarch, name_buf,
					       strlen (name_buf));

  status = regcache->raw_read (double_regnum, reg_buf);
  if (status != REG_VALID)
    return status;
  memcpy (buf, reg_buf, 8);

  status = regcache->raw_read (double_regnum + 1, reg_buf);
  if (status != REG_VALID)
    return status;
  memcpy (buf + 8, reg_buf, 8);

  return REG_VALID;
}

/* Read the contents of a NEON quad register, by reading from two double
   registers, and return it as a value.  QUAD_REG_INDEX is the index of the quad
   register, in [0, 15].  */

static value *
arm_neon_quad_read_value (gdbarch *gdbarch, frame_info_ptr next_frame,
			  int pseudo_reg_num, int quad_reg_index)
{
  std::string raw_reg_name = string_printf ("d%d", quad_reg_index << 1);
  int double_regnum
    = user_reg_map_name_to_regnum (gdbarch, raw_reg_name.c_str (),
				   raw_reg_name.length ());

  return pseudo_from_concat_raw (next_frame, pseudo_reg_num, double_regnum,
				 double_regnum + 1);
}

/* Read the contents of the MVE pseudo register REGNUM and return it as a
   value.  */
static value *
arm_mve_pseudo_read_value (gdbarch *gdbarch, frame_info_ptr next_frame,
			   int pseudo_reg_num)
{
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  /* P0 is the first 16 bits of VPR.  */
  return pseudo_from_raw_part (next_frame, pseudo_reg_num,
			       tdep->mve_vpr_regnum, 0);
}

static value *
arm_pseudo_read_value (gdbarch *gdbarch, frame_info_ptr next_frame,
		       const int pseudo_reg_num)
{
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  gdb_assert (pseudo_reg_num >= gdbarch_num_regs (gdbarch));

  if (is_q_pseudo (gdbarch, pseudo_reg_num))
    {
      /* Quad-precision register.  */
      return arm_neon_quad_read_value (gdbarch, next_frame, pseudo_reg_num,
				       pseudo_reg_num - tdep->q_pseudo_base);
    }
  else if (is_mve_pseudo (gdbarch, pseudo_reg_num))
    return arm_mve_pseudo_read_value (gdbarch, next_frame, pseudo_reg_num);
  else
    {
      int s_reg_index = pseudo_reg_num - tdep->s_pseudo_base;

      /* Single-precision register.  */
      gdb_assert (s_reg_index < 32);

      /* s0 is always the least significant half of d0.  */
      int offset;
      if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
	offset = (s_reg_index & 1) ? 0 : 4;
      else
	offset = (s_reg_index & 1) ? 4 : 0;

      std::string raw_reg_name = string_printf ("d%d", s_reg_index >> 1);
      int double_regnum
	= user_reg_map_name_to_regnum (gdbarch, raw_reg_name.c_str (),
				       raw_reg_name.length ());

      return pseudo_from_raw_part (next_frame, pseudo_reg_num, double_regnum,
				   offset);
    }
}

/* Store the contents of BUF to a NEON quad register, by writing to
   two double registers.  This is used to implement the quad pseudo
   registers, and for argument passing in case the quad registers are
   missing; vectors are passed in quad registers when using the VFP
   ABI, even if a NEON unit is not present.  REGNUM is the index
   of the quad register, in [0, 15].  */

static void
arm_neon_quad_write (struct gdbarch *gdbarch, struct regcache *regcache,
		     int regnum, const gdb_byte *buf)
{
  char name_buf[4];
  int double_regnum;

  xsnprintf (name_buf, sizeof (name_buf), "d%d", regnum << 1);
  double_regnum = user_reg_map_name_to_regnum (gdbarch, name_buf,
					       strlen (name_buf));

  regcache->raw_write (double_regnum, buf);
  regcache->raw_write (double_regnum + 1, buf + 8);
}

static void
arm_neon_quad_write (gdbarch *gdbarch, frame_info_ptr next_frame,
		     int quad_reg_index, gdb::array_view<const gdb_byte> buf)
{
  std::string raw_reg_name = string_printf ("d%d", quad_reg_index << 1);
  int double_regnum
    = user_reg_map_name_to_regnum (gdbarch, raw_reg_name.data (),
				   raw_reg_name.length ());

  pseudo_to_concat_raw (next_frame, buf, double_regnum, double_regnum + 1);
}

/* Store the contents of BUF to the MVE pseudo register REGNUM.  */

static void
arm_mve_pseudo_write (gdbarch *gdbarch, frame_info_ptr next_frame,
		      int pseudo_reg_num, gdb::array_view<const gdb_byte> buf)
{
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  /* P0 is the first 16 bits of VPR.  */
  pseudo_to_raw_part(next_frame, buf, tdep->mve_vpr_regnum, 0);
}

static void
arm_pseudo_write (gdbarch *gdbarch, frame_info_ptr next_frame,
		  const int pseudo_reg_num,
		  gdb::array_view<const gdb_byte> buf)
{
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  gdb_assert (pseudo_reg_num >= gdbarch_num_regs (gdbarch));

  if (is_q_pseudo (gdbarch, pseudo_reg_num))
    {
      /* Quad-precision register.  */
      arm_neon_quad_write (gdbarch, next_frame,
			   pseudo_reg_num - tdep->q_pseudo_base, buf);
    }
  else if (is_mve_pseudo (gdbarch, pseudo_reg_num))
    arm_mve_pseudo_write (gdbarch, next_frame, pseudo_reg_num, buf);
  else
    {
      int s_reg_index = pseudo_reg_num - tdep->s_pseudo_base;

      /* Single-precision register.  */
      gdb_assert (s_reg_index < 32);

      /* s0 is always the least significant half of d0.  */
      int offset;
      if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
	offset = (s_reg_index & 1) ? 0 : 4;
      else
	offset = (s_reg_index & 1) ? 4 : 0;

      std::string raw_reg_name = string_printf ("d%d", s_reg_index >> 1);
      int double_regnum
	= user_reg_map_name_to_regnum (gdbarch, raw_reg_name.c_str (),
				       raw_reg_name.length ());

      pseudo_to_raw_part (next_frame, buf, double_regnum, offset);
    }
}

static struct value *
value_of_arm_user_reg (frame_info_ptr frame, const void *baton)
{
  const int *reg_p = (const int *) baton;
  return value_of_register (*reg_p, get_next_frame_sentinel_okay (frame));
}

static enum gdb_osabi
arm_elf_osabi_sniffer (bfd *abfd)
{
  unsigned int elfosabi;
  enum gdb_osabi osabi = GDB_OSABI_UNKNOWN;

  elfosabi = elf_elfheader (abfd)->e_ident[EI_OSABI];

  if (elfosabi == ELFOSABI_ARM)
    /* GNU tools use this value.  Check note sections in this case,
       as well.  */
    {
      for (asection *sect : gdb_bfd_sections (abfd))
	generic_elf_osabi_sniff_abi_tag_sections (abfd, sect, &osabi);
    }

  /* Anything else will be handled by the generic ELF sniffer.  */
  return osabi;
}

static int
arm_register_reggroup_p (struct gdbarch *gdbarch, int regnum,
			 const struct reggroup *group)
{
  /* FPS register's type is INT, but belongs to float_reggroup.  Beside
     this, FPS register belongs to save_regroup, restore_reggroup, and
     all_reggroup, of course.  */
  if (regnum == ARM_FPS_REGNUM)
    return (group == float_reggroup
	    || group == save_reggroup
	    || group == restore_reggroup
	    || group == all_reggroup);
  else
    return default_register_reggroup_p (gdbarch, regnum, group);
}

/* For backward-compatibility we allow two 'g' packet lengths with
   the remote protocol depending on whether FPA registers are
   supplied.  M-profile targets do not have FPA registers, but some
   stubs already exist in the wild which use a 'g' packet which
   supplies them albeit with dummy values.  The packet format which
   includes FPA registers should be considered deprecated for
   M-profile targets.  */

static void
arm_register_g_packet_guesses (struct gdbarch *gdbarch)
{
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  if (tdep->is_m)
    {
      const target_desc *tdesc;

      /* If we know from the executable this is an M-profile target,
	 cater for remote targets whose register set layout is the
	 same as the FPA layout.  */
      tdesc = arm_read_mprofile_description (ARM_M_TYPE_WITH_FPA);
      register_remote_g_packet_guess (gdbarch,
				      ARM_CORE_REGS_SIZE + ARM_FP_REGS_SIZE,
				      tdesc);

      /* The regular M-profile layout.  */
      tdesc = arm_read_mprofile_description (ARM_M_TYPE_M_PROFILE);
      register_remote_g_packet_guess (gdbarch, ARM_CORE_REGS_SIZE,
				      tdesc);

      /* M-profile plus M4F VFP.  */
      tdesc = arm_read_mprofile_description (ARM_M_TYPE_VFP_D16);
      register_remote_g_packet_guess (gdbarch,
				      ARM_CORE_REGS_SIZE + ARM_VFP2_REGS_SIZE,
				      tdesc);
      /* M-profile plus MVE.  */
      tdesc = arm_read_mprofile_description (ARM_M_TYPE_MVE);
      register_remote_g_packet_guess (gdbarch, ARM_CORE_REGS_SIZE
				      + ARM_VFP2_REGS_SIZE
				      + ARM_INT_REGISTER_SIZE, tdesc);

      /* M-profile system (stack pointers).  */
      tdesc = arm_read_mprofile_description (ARM_M_TYPE_SYSTEM);
      register_remote_g_packet_guess (gdbarch, 2 * ARM_INT_REGISTER_SIZE, tdesc);
    }

  /* Otherwise we don't have a useful guess.  */
}

/* Implement the code_of_frame_writable gdbarch method.  */

static int
arm_code_of_frame_writable (struct gdbarch *gdbarch, frame_info_ptr frame)
{
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  if (tdep->is_m && get_frame_type (frame) == SIGTRAMP_FRAME)
    {
      /* M-profile exception frames return to some magic PCs, where
	 isn't writable at all.  */
      return 0;
    }
  else
    return 1;
}

/* Implement gdbarch_gnu_triplet_regexp.  If the arch name is arm then allow it
   to be postfixed by a version (eg armv7hl).  */

static const char *
arm_gnu_triplet_regexp (struct gdbarch *gdbarch)
{
  if (strcmp (gdbarch_bfd_arch_info (gdbarch)->arch_name, "arm") == 0)
    return "arm(v[^- ]*)?";
  return gdbarch_bfd_arch_info (gdbarch)->arch_name;
}

/* Implement the "get_pc_address_flags" gdbarch method.  */

static std::string
arm_get_pc_address_flags (frame_info_ptr frame, CORE_ADDR pc)
{
  if (get_frame_pc_masked (frame))
    return "PAC";

  return "";
}

/* Initialize the current architecture based on INFO.  If possible,
   re-use an architecture from ARCHES, which is a list of
   architectures already created during this debugging session.

   Called e.g. at program startup, when reading a core file, and when
   reading a binary file.  */

static struct gdbarch *
arm_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  struct gdbarch_list *best_arch;
  enum arm_abi_kind arm_abi = arm_abi_global;
  enum arm_float_model fp_model = arm_fp_model;
  tdesc_arch_data_up tdesc_data;
  int i;
  bool is_m = false;
  bool have_sec_ext = false;
  int vfp_register_count = 0;
  bool have_s_pseudos = false, have_q_pseudos = false;
  bool have_wmmx_registers = false;
  bool have_neon = false;
  bool have_fpa_registers = true;
  const struct target_desc *tdesc = info.target_desc;
  bool have_vfp = false;
  bool have_mve = false;
  bool have_pacbti = false;
  int mve_vpr_regnum = -1;
  int register_count = ARM_NUM_REGS;
  bool have_m_profile_msp = false;
  int m_profile_msp_regnum = -1;
  int m_profile_psp_regnum = -1;
  int m_profile_msp_ns_regnum = -1;
  int m_profile_psp_ns_regnum = -1;
  int m_profile_msp_s_regnum = -1;
  int m_profile_psp_s_regnum = -1;
  int tls_regnum = 0;

  /* If we have an object to base this architecture on, try to determine
     its ABI.  */

  if (arm_abi == ARM_ABI_AUTO && info.abfd != NULL)
    {
      int ei_osabi, e_flags;

      switch (bfd_get_flavour (info.abfd))
	{
	case bfd_target_coff_flavour:
	  /* Assume it's an old APCS-style ABI.  */
	  /* XXX WinCE?  */
	  arm_abi = ARM_ABI_APCS;
	  break;

	case bfd_target_elf_flavour:
	  ei_osabi = elf_elfheader (info.abfd)->e_ident[EI_OSABI];
	  e_flags = elf_elfheader (info.abfd)->e_flags;

	  if (ei_osabi == ELFOSABI_ARM)
	    {
	      /* GNU tools used to use this value, but do not for EABI
		 objects.  There's nowhere to tag an EABI version
		 anyway, so assume APCS.  */
	      arm_abi = ARM_ABI_APCS;
	    }
	  else if (ei_osabi == ELFOSABI_NONE || ei_osabi == ELFOSABI_GNU)
	    {
	      int eabi_ver = EF_ARM_EABI_VERSION (e_flags);

	      switch (eabi_ver)
		{
		case EF_ARM_EABI_UNKNOWN:
		  /* Assume GNU tools.  */
		  arm_abi = ARM_ABI_APCS;
		  break;

		case EF_ARM_EABI_VER4:
		case EF_ARM_EABI_VER5:
		  arm_abi = ARM_ABI_AAPCS;
		  /* EABI binaries default to VFP float ordering.
		     They may also contain build attributes that can
		     be used to identify if the VFP argument-passing
		     ABI is in use.  */
		  if (fp_model == ARM_FLOAT_AUTO)
		    {
#ifdef HAVE_ELF
		      switch (bfd_elf_get_obj_attr_int (info.abfd,
							OBJ_ATTR_PROC,
							Tag_ABI_VFP_args))
			{
			case AEABI_VFP_args_base:
			  /* "The user intended FP parameter/result
			     passing to conform to AAPCS, base
			     variant".  */
			  fp_model = ARM_FLOAT_SOFT_VFP;
			  break;
			case AEABI_VFP_args_vfp:
			  /* "The user intended FP parameter/result
			     passing to conform to AAPCS, VFP
			     variant".  */
			  fp_model = ARM_FLOAT_VFP;
			  break;
			case AEABI_VFP_args_toolchain:
			  /* "The user intended FP parameter/result
			     passing to conform to tool chain-specific
			     conventions" - we don't know any such
			     conventions, so leave it as "auto".  */
			  break;
			case AEABI_VFP_args_compatible:
			  /* "Code is compatible with both the base
			     and VFP variants; the user did not permit
			     non-variadic functions to pass FP
			     parameters/results" - leave it as
			     "auto".  */
			  break;
			default:
			  /* Attribute value not mentioned in the
			     November 2012 ABI, so leave it as
			     "auto".  */
			  break;
			}
#else
		      fp_model = ARM_FLOAT_SOFT_VFP;
#endif
		    }
		  break;

		default:
		  /* Leave it as "auto".  */
		  warning (_("unknown ARM EABI version 0x%x"), eabi_ver);
		  break;
		}

#ifdef HAVE_ELF
	      /* Detect M-profile programs.  This only works if the
		 executable file includes build attributes; GCC does
		 copy them to the executable, but e.g. RealView does
		 not.  */
	      int attr_arch
		= bfd_elf_get_obj_attr_int (info.abfd, OBJ_ATTR_PROC,
					    Tag_CPU_arch);
	      int attr_profile
		= bfd_elf_get_obj_attr_int (info.abfd, OBJ_ATTR_PROC,
					    Tag_CPU_arch_profile);

	      /* GCC specifies the profile for v6-M; RealView only
		 specifies the profile for architectures starting with
		 V7 (as opposed to architectures with a tag
		 numerically greater than TAG_CPU_ARCH_V7).  */
	      if (!tdesc_has_registers (tdesc)
		  && (attr_arch == TAG_CPU_ARCH_V6_M
		      || attr_arch == TAG_CPU_ARCH_V6S_M
		      || attr_arch == TAG_CPU_ARCH_V7E_M
		      || attr_arch == TAG_CPU_ARCH_V8M_BASE
		      || attr_arch == TAG_CPU_ARCH_V8M_MAIN
		      || attr_arch == TAG_CPU_ARCH_V8_1M_MAIN
		      || attr_profile == 'M'))
		is_m = true;

	      /* Look for attributes that indicate support for ARMv8.1-m
		 PACBTI.  */
	      if (!tdesc_has_registers (tdesc) && is_m)
		{
		  int attr_pac_extension
		    = bfd_elf_get_obj_attr_int (info.abfd, OBJ_ATTR_PROC,
						Tag_PAC_extension);

		  int attr_bti_extension
		    = bfd_elf_get_obj_attr_int (info.abfd, OBJ_ATTR_PROC,
						Tag_BTI_extension);

		  int attr_pacret_use
		    = bfd_elf_get_obj_attr_int (info.abfd, OBJ_ATTR_PROC,
						Tag_PACRET_use);

		  int attr_bti_use
		    = bfd_elf_get_obj_attr_int (info.abfd, OBJ_ATTR_PROC,
						Tag_BTI_use);

		  if (attr_pac_extension != 0 || attr_bti_extension != 0
		      || attr_pacret_use != 0 || attr_bti_use != 0)
		    have_pacbti = true;
		}
#endif
	    }

	  if (fp_model == ARM_FLOAT_AUTO)
	    {
	      switch (e_flags & (EF_ARM_SOFT_FLOAT | EF_ARM_VFP_FLOAT))
		{
		case 0:
		  /* Leave it as "auto".  Strictly speaking this case
		     means FPA, but almost nobody uses that now, and
		     many toolchains fail to set the appropriate bits
		     for the floating-point model they use.  */
		  break;
		case EF_ARM_SOFT_FLOAT:
		  fp_model = ARM_FLOAT_SOFT_FPA;
		  break;
		case EF_ARM_VFP_FLOAT:
		  fp_model = ARM_FLOAT_VFP;
		  break;
		case EF_ARM_SOFT_FLOAT | EF_ARM_VFP_FLOAT:
		  fp_model = ARM_FLOAT_SOFT_VFP;
		  break;
		}
	    }

	  if (e_flags & EF_ARM_BE8)
	    info.byte_order_for_code = BFD_ENDIAN_LITTLE;

	  break;

	default:
	  /* Leave it as "auto".  */
	  break;
	}
    }

  /* Check any target description for validity.  */
  if (tdesc_has_registers (tdesc))
    {
      /* For most registers we require GDB's default names; but also allow
	 the numeric names for sp / lr / pc, as a convenience.  */
      static const char *const arm_sp_names[] = { "r13", "sp", NULL };
      static const char *const arm_lr_names[] = { "r14", "lr", NULL };
      static const char *const arm_pc_names[] = { "r15", "pc", NULL };

      const struct tdesc_feature *feature;
      int valid_p;

      feature = tdesc_find_feature (tdesc,
				    "org.gnu.gdb.arm.core");
      if (feature == NULL)
	{
	  feature = tdesc_find_feature (tdesc,
					"org.gnu.gdb.arm.m-profile");
	  if (feature == NULL)
	    return NULL;
	  else
	    is_m = true;
	}

      tdesc_data = tdesc_data_alloc ();

      valid_p = 1;
      for (i = 0; i < ARM_SP_REGNUM; i++)
	valid_p &= tdesc_numbered_register (feature, tdesc_data.get (), i,
					    arm_register_names[i]);
      valid_p &= tdesc_numbered_register_choices (feature, tdesc_data.get (),
						  ARM_SP_REGNUM,
						  arm_sp_names);
      valid_p &= tdesc_numbered_register_choices (feature, tdesc_data.get (),
						  ARM_LR_REGNUM,
						  arm_lr_names);
      valid_p &= tdesc_numbered_register_choices (feature, tdesc_data.get (),
						  ARM_PC_REGNUM,
						  arm_pc_names);
      if (is_m)
	valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
					    ARM_PS_REGNUM, "xpsr");
      else
	valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
					    ARM_PS_REGNUM, "cpsr");

      if (!valid_p)
	return NULL;

      if (is_m)
	{
	  feature = tdesc_find_feature (tdesc,
					"org.gnu.gdb.arm.m-system");
	  if (feature != nullptr)
	    {
	      /* MSP */
	      valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
						  register_count, "msp");
	      if (!valid_p)
		{
		  warning (_("M-profile m-system feature is missing required register msp."));
		  return nullptr;
		}
	      have_m_profile_msp = true;
	      m_profile_msp_regnum = register_count++;

	      /* PSP */
	      valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
						  register_count, "psp");
	      if (!valid_p)
		{
		  warning (_("M-profile m-system feature is missing required register psp."));
		  return nullptr;
		}
	      m_profile_psp_regnum = register_count++;
	    }
	}

      feature = tdesc_find_feature (tdesc,
				    "org.gnu.gdb.arm.fpa");
      if (feature != NULL)
	{
	  valid_p = 1;
	  for (i = ARM_F0_REGNUM; i <= ARM_FPS_REGNUM; i++)
	    valid_p &= tdesc_numbered_register (feature, tdesc_data.get (), i,
						arm_register_names[i]);
	  if (!valid_p)
	    return NULL;
	}
      else
	have_fpa_registers = false;

      feature = tdesc_find_feature (tdesc,
				    "org.gnu.gdb.xscale.iwmmxt");
      if (feature != NULL)
	{
	  static const char *const iwmmxt_names[] = {
	    "wR0", "wR1", "wR2", "wR3", "wR4", "wR5", "wR6", "wR7",
	    "wR8", "wR9", "wR10", "wR11", "wR12", "wR13", "wR14", "wR15",
	    "wCID", "wCon", "wCSSF", "wCASF", "", "", "", "",
	    "wCGR0", "wCGR1", "wCGR2", "wCGR3", "", "", "", "",
	  };

	  valid_p = 1;
	  for (i = ARM_WR0_REGNUM; i <= ARM_WR15_REGNUM; i++)
	    valid_p
	      &= tdesc_numbered_register (feature, tdesc_data.get (), i,
					  iwmmxt_names[i - ARM_WR0_REGNUM]);

	  /* Check for the control registers, but do not fail if they
	     are missing.  */
	  for (i = ARM_WC0_REGNUM; i <= ARM_WCASF_REGNUM; i++)
	    tdesc_numbered_register (feature, tdesc_data.get (), i,
				     iwmmxt_names[i - ARM_WR0_REGNUM]);

	  for (i = ARM_WCGR0_REGNUM; i <= ARM_WCGR3_REGNUM; i++)
	    valid_p
	      &= tdesc_numbered_register (feature, tdesc_data.get (), i,
					  iwmmxt_names[i - ARM_WR0_REGNUM]);

	  if (!valid_p)
	    return NULL;

	  have_wmmx_registers = true;
	}

      /* If we have a VFP unit, check whether the single precision registers
	 are present.  If not, then we will synthesize them as pseudo
	 registers.  */
      feature = tdesc_find_feature (tdesc,
				    "org.gnu.gdb.arm.vfp");
      if (feature != NULL)
	{
	  static const char *const vfp_double_names[] = {
	    "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
	    "d8", "d9", "d10", "d11", "d12", "d13", "d14", "d15",
	    "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23",
	    "d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31",
	  };

	  /* Require the double precision registers.  There must be either
	     16 or 32.  */
	  valid_p = 1;
	  for (i = 0; i < 32; i++)
	    {
	      valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
						  ARM_D0_REGNUM + i,
						  vfp_double_names[i]);
	      if (!valid_p)
		break;
	    }
	  if (!valid_p && i == 16)
	    valid_p = 1;

	  /* Also require FPSCR.  */
	  valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
					      ARM_FPSCR_REGNUM, "fpscr");
	  if (!valid_p)
	    return NULL;

	  have_vfp = true;

	  if (tdesc_unnumbered_register (feature, "s0") == 0)
	    have_s_pseudos = true;

	  vfp_register_count = i;

	  /* If we have VFP, also check for NEON.  The architecture allows
	     NEON without VFP (integer vector operations only), but GDB
	     does not support that.  */
	  feature = tdesc_find_feature (tdesc,
					"org.gnu.gdb.arm.neon");
	  if (feature != NULL)
	    {
	      /* NEON requires 32 double-precision registers.  */
	      if (i != 32)
		return NULL;

	      /* If there are quad registers defined by the stub, use
		 their type; otherwise (normally) provide them with
		 the default type.  */
	      if (tdesc_unnumbered_register (feature, "q0") == 0)
		have_q_pseudos = true;
	    }
	}

      /* Check for the TLS register feature.  */
      feature = tdesc_find_feature (tdesc, "org.gnu.gdb.arm.tls");
      if (feature != nullptr)
	{
	  valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
					      register_count, "tpidruro");
	  if (!valid_p)
	    return nullptr;

	  tls_regnum = register_count;
	  register_count++;
	}

      /* Check for MVE after all the checks for GPR's, VFP and Neon.
	 MVE (Helium) is an M-profile extension.  */
      if (is_m)
	{
	  /* Do we have the MVE feature?  */
	  feature = tdesc_find_feature (tdesc,"org.gnu.gdb.arm.m-profile-mve");

	  if (feature != nullptr)
	    {
	      /* If we have MVE, we must always have the VPR register.  */
	      valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
						  register_count, "vpr");
	      if (!valid_p)
		{
		  warning (_("MVE feature is missing required register vpr."));
		  return nullptr;
		}

	      have_mve = true;
	      mve_vpr_regnum = register_count;
	      register_count++;

	      /* We can't have Q pseudo registers available here, as that
		 would mean we have NEON features, and that is only available
		 on A and R profiles.  */
	      gdb_assert (!have_q_pseudos);

	      /* Given we have a M-profile target description, if MVE is
		 enabled and there are VFP registers, we should have Q
		 pseudo registers (Q0 ~ Q7).  */
	      if (have_vfp)
		have_q_pseudos = true;
	    }

	  /* Do we have the ARMv8.1-m PACBTI feature?  */
	  feature = tdesc_find_feature (tdesc,
					"org.gnu.gdb.arm.m-profile-pacbti");
	  if (feature != nullptr)
	    {
	      /* By advertising this feature, the target acknowledges the
		 presence of the ARMv8.1-m PACBTI extensions.

		 We don't care for any particular registers in this group, so
		 the target is free to include whatever it deems appropriate.

		 The expectation is for this feature to include the PAC
		 keys.  */
	      have_pacbti = true;
	    }

	  /* Do we have the Security extension?  */
	  feature = tdesc_find_feature (tdesc,
					"org.gnu.gdb.arm.secext");
	  if (feature != nullptr)
	    {
	      /* Secure/Non-secure stack pointers.  */
	      /* MSP_NS */
	      valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
						 register_count, "msp_ns");
	      if (!valid_p)
		{
		  warning (_("M-profile secext feature is missing required register msp_ns."));
		  return nullptr;
		}
	      m_profile_msp_ns_regnum = register_count++;

	      /* PSP_NS */
	      valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
						 register_count, "psp_ns");
	      if (!valid_p)
		{
		  warning (_("M-profile secext feature is missing required register psp_ns."));
		  return nullptr;
		}
	      m_profile_psp_ns_regnum = register_count++;

	      /* MSP_S */
	      valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
						 register_count, "msp_s");
	      if (!valid_p)
		{
		  warning (_("M-profile secext feature is missing required register msp_s."));
		  return nullptr;
		}
	      m_profile_msp_s_regnum = register_count++;

	      /* PSP_S */
	      valid_p &= tdesc_numbered_register (feature, tdesc_data.get (),
						 register_count, "psp_s");
	      if (!valid_p)
		{
		  warning (_("M-profile secext feature is missing required register psp_s."));
		  return nullptr;
		}
	      m_profile_psp_s_regnum = register_count++;

	      have_sec_ext = true;
	    }

	}
    }

  /* If there is already a candidate, use it.  */
  for (best_arch = gdbarch_list_lookup_by_info (arches, &info);
       best_arch != NULL;
       best_arch = gdbarch_list_lookup_by_info (best_arch->next, &info))
    {
      arm_gdbarch_tdep *tdep
	= gdbarch_tdep<arm_gdbarch_tdep> (best_arch->gdbarch);

      if (arm_abi != ARM_ABI_AUTO && arm_abi != tdep->arm_abi)
	continue;

      if (fp_model != ARM_FLOAT_AUTO && fp_model != tdep->fp_model)
	continue;

      /* There are various other properties in tdep that we do not
	 need to check here: those derived from a target description,
	 since gdbarches with a different target description are
	 automatically disqualified.  */

      /* Do check is_m, though, since it might come from the binary.  */
      if (is_m != tdep->is_m)
	continue;

      /* Also check for ARMv8.1-m PACBTI support, since it might come from
	 the binary.  */
      if (have_pacbti != tdep->have_pacbti)
	continue;

      /* Found a match.  */
      break;
    }

  if (best_arch != NULL)
    return best_arch->gdbarch;

  gdbarch *gdbarch
    = gdbarch_alloc (&info, gdbarch_tdep_up (new arm_gdbarch_tdep));
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  /* Record additional information about the architecture we are defining.
     These are gdbarch discriminators, like the OSABI.  */
  tdep->arm_abi = arm_abi;
  tdep->fp_model = fp_model;
  tdep->is_m = is_m;
  tdep->have_sec_ext = have_sec_ext;
  tdep->have_fpa_registers = have_fpa_registers;
  tdep->have_wmmx_registers = have_wmmx_registers;
  gdb_assert (vfp_register_count == 0
	      || vfp_register_count == 16
	      || vfp_register_count == 32);
  tdep->vfp_register_count = vfp_register_count;
  tdep->have_s_pseudos = have_s_pseudos;
  tdep->have_q_pseudos = have_q_pseudos;
  tdep->have_neon = have_neon;
  tdep->tls_regnum = tls_regnum;

  /* Adjust the MVE feature settings.  */
  if (have_mve)
    {
      tdep->have_mve = true;
      tdep->mve_vpr_regnum = mve_vpr_regnum;
    }

  /* Adjust the PACBTI feature settings.  */
  tdep->have_pacbti = have_pacbti;

  /* Adjust the M-profile stack pointers settings.  */
  if (have_m_profile_msp)
    {
      tdep->m_profile_msp_regnum = m_profile_msp_regnum;
      tdep->m_profile_psp_regnum = m_profile_psp_regnum;
      tdep->m_profile_msp_ns_regnum = m_profile_msp_ns_regnum;
      tdep->m_profile_psp_ns_regnum = m_profile_psp_ns_regnum;
      tdep->m_profile_msp_s_regnum = m_profile_msp_s_regnum;
      tdep->m_profile_psp_s_regnum = m_profile_psp_s_regnum;
    }

  arm_register_g_packet_guesses (gdbarch);

  /* Breakpoints.  */
  switch (info.byte_order_for_code)
    {
    case BFD_ENDIAN_BIG:
      tdep->arm_breakpoint = arm_default_arm_be_breakpoint;
      tdep->arm_breakpoint_size = sizeof (arm_default_arm_be_breakpoint);
      tdep->thumb_breakpoint = arm_default_thumb_be_breakpoint;
      tdep->thumb_breakpoint_size = sizeof (arm_default_thumb_be_breakpoint);

      break;

    case BFD_ENDIAN_LITTLE:
      tdep->arm_breakpoint = arm_default_arm_le_breakpoint;
      tdep->arm_breakpoint_size = sizeof (arm_default_arm_le_breakpoint);
      tdep->thumb_breakpoint = arm_default_thumb_le_breakpoint;
      tdep->thumb_breakpoint_size = sizeof (arm_default_thumb_le_breakpoint);

      break;

    default:
      internal_error (_("arm_gdbarch_init: bad byte order for float format"));
    }

  /* On ARM targets char defaults to unsigned.  */
  set_gdbarch_char_signed (gdbarch, 0);

  /* wchar_t is unsigned under the AAPCS.  */
  if (tdep->arm_abi == ARM_ABI_AAPCS)
    set_gdbarch_wchar_signed (gdbarch, 0);
  else
    set_gdbarch_wchar_signed (gdbarch, 1);

  /* Compute type alignment.  */
  set_gdbarch_type_align (gdbarch, arm_type_align);

  /* Note: for displaced stepping, this includes the breakpoint, and one word
     of additional scratch space.  This setting isn't used for anything beside
     displaced stepping at present.  */
  set_gdbarch_displaced_step_buffer_length
    (gdbarch, 4 * ARM_DISPLACED_MODIFIED_INSNS);
  set_gdbarch_max_insn_length (gdbarch, 4);

  /* This should be low enough for everything.  */
  tdep->lowest_pc = 0x20;
  tdep->jb_pc = -1;	/* Longjump support not enabled by default.  */

  /* The default, for both APCS and AAPCS, is to return small
     structures in registers.  */
  tdep->struct_return = reg_struct_return;

  set_gdbarch_push_dummy_call (gdbarch, arm_push_dummy_call);
  set_gdbarch_frame_align (gdbarch, arm_frame_align);

  if (is_m)
    set_gdbarch_code_of_frame_writable (gdbarch, arm_code_of_frame_writable);

  set_gdbarch_write_pc (gdbarch, arm_write_pc);

  frame_base_set_default (gdbarch, &arm_normal_base);

  /* Address manipulation.  */
  set_gdbarch_addr_bits_remove (gdbarch, arm_addr_bits_remove);

  /* Advance PC across function entry code.  */
  set_gdbarch_skip_prologue (gdbarch, arm_skip_prologue);

  /* Detect whether PC is at a point where the stack has been destroyed.  */
  set_gdbarch_stack_frame_destroyed_p (gdbarch, arm_stack_frame_destroyed_p);

  /* Skip trampolines.  */
  set_gdbarch_skip_trampoline_code (gdbarch, arm_skip_stub);

  /* The stack grows downward.  */
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);

  /* Breakpoint manipulation.  */
  set_gdbarch_breakpoint_kind_from_pc (gdbarch, arm_breakpoint_kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch, arm_sw_breakpoint_from_kind);
  set_gdbarch_breakpoint_kind_from_current_state (gdbarch,
						  arm_breakpoint_kind_from_current_state);

  /* Information about registers, etc.  */
  set_gdbarch_sp_regnum (gdbarch, ARM_SP_REGNUM);
  set_gdbarch_pc_regnum (gdbarch, ARM_PC_REGNUM);
  set_gdbarch_num_regs (gdbarch, register_count);
  set_gdbarch_register_type (gdbarch, arm_register_type);
  set_gdbarch_register_reggroup_p (gdbarch, arm_register_reggroup_p);

  /* This "info float" is FPA-specific.  Use the generic version if we
     do not have FPA.  */
  if (tdep->have_fpa_registers)
    set_gdbarch_print_float_info (gdbarch, arm_print_float_info);

  /* Internal <-> external register number maps.  */
  set_gdbarch_dwarf2_reg_to_regnum (gdbarch, arm_dwarf_reg_to_regnum);
  set_gdbarch_register_sim_regno (gdbarch, arm_register_sim_regno);

  set_gdbarch_register_name (gdbarch, arm_register_name);

  /* Returning results.  */
  set_gdbarch_return_value_as_value (gdbarch, arm_return_value);

  /* Disassembly.  */
  set_gdbarch_print_insn (gdbarch, gdb_print_insn_arm);

  /* Minsymbol frobbing.  */
  set_gdbarch_elf_make_msymbol_special (gdbarch, arm_elf_make_msymbol_special);
  set_gdbarch_coff_make_msymbol_special (gdbarch,
					 arm_coff_make_msymbol_special);
  set_gdbarch_record_special_symbol (gdbarch, arm_record_special_symbol);

  /* Thumb-2 IT block support.  */
  set_gdbarch_adjust_breakpoint_address (gdbarch,
					 arm_adjust_breakpoint_address);

  /* Virtual tables.  */
  set_gdbarch_vbit_in_delta (gdbarch, 1);

  /* Hook in the ABI-specific overrides, if they have been registered.  */
  gdbarch_init_osabi (info, gdbarch);

  dwarf2_frame_set_init_reg (gdbarch, arm_dwarf2_frame_init_reg);

  /* Add some default predicates.  */
  if (is_m)
    frame_unwind_append_unwinder (gdbarch, &arm_m_exception_unwind);
  frame_unwind_append_unwinder (gdbarch, &arm_stub_unwind);
  dwarf2_append_unwinders (gdbarch);
  frame_unwind_append_unwinder (gdbarch, &arm_exidx_unwind);
  frame_unwind_append_unwinder (gdbarch, &arm_epilogue_frame_unwind);
  frame_unwind_append_unwinder (gdbarch, &arm_prologue_unwind);

  /* Now we have tuned the configuration, set a few final things,
     based on what the OS ABI has told us.  */

  /* If the ABI is not otherwise marked, assume the old GNU APCS.  EABI
     binaries are always marked.  */
  if (tdep->arm_abi == ARM_ABI_AUTO)
    tdep->arm_abi = ARM_ABI_APCS;

  /* Watchpoints are not steppable.  */
  set_gdbarch_have_nonsteppable_watchpoint (gdbarch, 1);

  /* We used to default to FPA for generic ARM, but almost nobody
     uses that now, and we now provide a way for the user to force
     the model.  So default to the most useful variant.  */
  if (tdep->fp_model == ARM_FLOAT_AUTO)
    tdep->fp_model = ARM_FLOAT_SOFT_FPA;

  if (tdep->jb_pc >= 0)
    set_gdbarch_get_longjmp_target (gdbarch, arm_get_longjmp_target);

  /* Floating point sizes and format.  */
  set_gdbarch_float_format (gdbarch, floatformats_ieee_single);
  if (tdep->fp_model == ARM_FLOAT_SOFT_FPA || tdep->fp_model == ARM_FLOAT_FPA)
    {
      set_gdbarch_double_format
	(gdbarch, floatformats_ieee_double_littlebyte_bigword);
      set_gdbarch_long_double_format
	(gdbarch, floatformats_ieee_double_littlebyte_bigword);
    }
  else
    {
      set_gdbarch_double_format (gdbarch, floatformats_ieee_double);
      set_gdbarch_long_double_format (gdbarch, floatformats_ieee_double);
    }

  /* Hook used to decorate frames with signed return addresses, only available
     for ARMv8.1-m PACBTI.  */
  if (is_m && have_pacbti)
    set_gdbarch_get_pc_address_flags (gdbarch, arm_get_pc_address_flags);

  if (tdesc_data != nullptr)
    {
      set_tdesc_pseudo_register_name (gdbarch, arm_register_name);

      tdesc_use_registers (gdbarch, tdesc, std::move (tdesc_data));
      register_count = gdbarch_num_regs (gdbarch);

      /* Override tdesc_register_type to adjust the types of VFP
	 registers for NEON.  */
      set_gdbarch_register_type (gdbarch, arm_register_type);
    }

  /* Initialize the pseudo register data.  */
  int num_pseudos = 0;
  if (tdep->have_s_pseudos)
    {
      /* VFP single precision pseudo registers (S0~S31).  */
      tdep->s_pseudo_base = register_count;
      tdep->s_pseudo_count = 32;
      num_pseudos += tdep->s_pseudo_count;

      if (tdep->have_q_pseudos)
	{
	  /* NEON quad precision pseudo registers (Q0~Q15).  */
	  tdep->q_pseudo_base = register_count + num_pseudos;

	  if (have_neon)
	    tdep->q_pseudo_count = 16;
	  else if (have_mve)
	    tdep->q_pseudo_count = ARM_MVE_NUM_Q_REGS;

	  num_pseudos += tdep->q_pseudo_count;
	}
    }

  /* Do we have any MVE pseudo registers?  */
  if (have_mve)
    {
      tdep->mve_pseudo_base = register_count + num_pseudos;
      tdep->mve_pseudo_count = 1;
      num_pseudos += tdep->mve_pseudo_count;
    }

  /* Do we have any ARMv8.1-m PACBTI pseudo registers.  */
  if (have_pacbti)
    {
      tdep->pacbti_pseudo_base = register_count + num_pseudos;
      tdep->pacbti_pseudo_count = 1;
      num_pseudos += tdep->pacbti_pseudo_count;
    }

  /* Set some pseudo register hooks, if we have pseudo registers.  */
  if (tdep->have_s_pseudos || have_mve || have_pacbti)
    {
      set_gdbarch_num_pseudo_regs (gdbarch, num_pseudos);
      set_gdbarch_pseudo_register_read_value (gdbarch, arm_pseudo_read_value);
      set_gdbarch_pseudo_register_write (gdbarch, arm_pseudo_write);
    }

  /* Add standard register aliases.  We add aliases even for those
     names which are used by the current architecture - it's simpler,
     and does no harm, since nothing ever lists user registers.  */
  for (i = 0; i < ARRAY_SIZE (arm_register_aliases); i++)
    user_reg_add (gdbarch, arm_register_aliases[i].name,
		  value_of_arm_user_reg, &arm_register_aliases[i].regnum);

  set_gdbarch_disassembler_options (gdbarch, &arm_disassembler_options);
  set_gdbarch_valid_disassembler_options (gdbarch, disassembler_options_arm ());

  set_gdbarch_gnu_triplet_regexp (gdbarch, arm_gnu_triplet_regexp);

  return gdbarch;
}

static void
arm_dump_tdep (struct gdbarch *gdbarch, struct ui_file *file)
{
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  if (tdep == NULL)
    return;

  gdb_printf (file, _("arm_dump_tdep: fp_model = %i\n"),
	      (int) tdep->fp_model);
  gdb_printf (file, _("arm_dump_tdep: have_fpa_registers = %i\n"),
	      (int) tdep->have_fpa_registers);
  gdb_printf (file, _("arm_dump_tdep: have_wmmx_registers = %i\n"),
	      (int) tdep->have_wmmx_registers);
  gdb_printf (file, _("arm_dump_tdep: vfp_register_count = %i\n"),
	      (int) tdep->vfp_register_count);
  gdb_printf (file, _("arm_dump_tdep: have_s_pseudos = %s\n"),
	      tdep->have_s_pseudos ? "true" : "false");
  gdb_printf (file, _("arm_dump_tdep: s_pseudo_base = %i\n"),
	      (int) tdep->s_pseudo_base);
  gdb_printf (file, _("arm_dump_tdep: s_pseudo_count = %i\n"),
	      (int) tdep->s_pseudo_count);
  gdb_printf (file, _("arm_dump_tdep: have_q_pseudos = %s\n"),
	      tdep->have_q_pseudos ? "true" : "false");
  gdb_printf (file, _("arm_dump_tdep: q_pseudo_base = %i\n"),
	      (int) tdep->q_pseudo_base);
  gdb_printf (file, _("arm_dump_tdep: q_pseudo_count = %i\n"),
	      (int) tdep->q_pseudo_count);
  gdb_printf (file, _("arm_dump_tdep: have_neon = %i\n"),
	      (int) tdep->have_neon);
  gdb_printf (file, _("arm_dump_tdep: have_mve = %s\n"),
	      tdep->have_mve ? "yes" : "no");
  gdb_printf (file, _("arm_dump_tdep: mve_vpr_regnum = %i\n"),
	      tdep->mve_vpr_regnum);
  gdb_printf (file, _("arm_dump_tdep: mve_pseudo_base = %i\n"),
	      tdep->mve_pseudo_base);
  gdb_printf (file, _("arm_dump_tdep: mve_pseudo_count = %i\n"),
	      tdep->mve_pseudo_count);
  gdb_printf (file, _("arm_dump_tdep: m_profile_msp_regnum = %i\n"),
	      tdep->m_profile_msp_regnum);
  gdb_printf (file, _("arm_dump_tdep: m_profile_psp_regnum = %i\n"),
	      tdep->m_profile_psp_regnum);
  gdb_printf (file, _("arm_dump_tdep: m_profile_msp_ns_regnum = %i\n"),
	      tdep->m_profile_msp_ns_regnum);
  gdb_printf (file, _("arm_dump_tdep: m_profile_psp_ns_regnum = %i\n"),
	      tdep->m_profile_psp_ns_regnum);
  gdb_printf (file, _("arm_dump_tdep: m_profile_msp_s_regnum = %i\n"),
	      tdep->m_profile_msp_s_regnum);
  gdb_printf (file, _("arm_dump_tdep: m_profile_psp_s_regnum = %i\n"),
	      tdep->m_profile_psp_s_regnum);
  gdb_printf (file, _("arm_dump_tdep: Lowest pc = 0x%lx\n"),
	      (unsigned long) tdep->lowest_pc);
  gdb_printf (file, _("arm_dump_tdep: have_pacbti = %s\n"),
	      tdep->have_pacbti ? "yes" : "no");
  gdb_printf (file, _("arm_dump_tdep: pacbti_pseudo_base = %i\n"),
	      tdep->pacbti_pseudo_base);
  gdb_printf (file, _("arm_dump_tdep: pacbti_pseudo_count = %i\n"),
	      tdep->pacbti_pseudo_count);
  gdb_printf (file, _("arm_dump_tdep: is_m = %s\n"),
	      tdep->is_m ? "yes" : "no");
}

#if GDB_SELF_TEST
namespace selftests
{
static void arm_record_test (void);
static void arm_analyze_prologue_test ();
}
#endif

void _initialize_arm_tdep ();
void
_initialize_arm_tdep ()
{
  long length;
  int i, j;
  char regdesc[1024], *rdptr = regdesc;
  size_t rest = sizeof (regdesc);

  gdbarch_register (bfd_arch_arm, arm_gdbarch_init, arm_dump_tdep);

  /* Add ourselves to objfile event chain.  */
  gdb::observers::new_objfile.attach (arm_exidx_new_objfile, "arm-tdep");

  /* Register an ELF OS ABI sniffer for ARM binaries.  */
  gdbarch_register_osabi_sniffer (bfd_arch_arm,
				  bfd_target_elf_flavour,
				  arm_elf_osabi_sniffer);

  /* Add root prefix command for all "set arm"/"show arm" commands.  */
  add_setshow_prefix_cmd ("arm", no_class,
			  _("Various ARM-specific commands."),
			  _("Various ARM-specific commands."),
			  &setarmcmdlist, &showarmcmdlist,
			  &setlist, &showlist);

  arm_disassembler_options = xstrdup ("reg-names-std");
  const disasm_options_t *disasm_options
    = &disassembler_options_arm ()->options;
  int num_disassembly_styles = 0;
  for (i = 0; disasm_options->name[i] != NULL; i++)
    if (startswith (disasm_options->name[i], "reg-names-"))
      num_disassembly_styles++;

  /* Initialize the array that will be passed to add_setshow_enum_cmd().  */
  valid_disassembly_styles = XNEWVEC (const char *,
				      num_disassembly_styles + 1);
  for (i = j = 0; disasm_options->name[i] != NULL; i++)
    if (startswith (disasm_options->name[i], "reg-names-"))
      {
	size_t offset = strlen ("reg-names-");
	const char *style = disasm_options->name[i];
	valid_disassembly_styles[j++] = &style[offset];
	if (strcmp (&style[offset], "std") == 0)
	  disassembly_style = &style[offset];
	length = snprintf (rdptr, rest, "%s - %s\n", &style[offset],
			   disasm_options->description[i]);
	rdptr += length;
	rest -= length;
      }
  /* Mark the end of valid options.  */
  valid_disassembly_styles[num_disassembly_styles] = NULL;

  /* Create the help text.  */
  std::string helptext = string_printf ("%s%s%s",
					_("The valid values are:\n"),
					regdesc,
					_("The default is \"std\"."));

  add_setshow_enum_cmd("disassembler", no_class,
		       valid_disassembly_styles, &disassembly_style,
		       _("Set the disassembly style."),
		       _("Show the disassembly style."),
		       helptext.c_str (),
		       set_disassembly_style_sfunc,
		       show_disassembly_style_sfunc,
		       &setarmcmdlist, &showarmcmdlist);

  add_setshow_boolean_cmd ("apcs32", no_class, &arm_apcs_32,
			   _("Set usage of ARM 32-bit mode."),
			   _("Show usage of ARM 32-bit mode."),
			   _("When off, a 26-bit PC will be used."),
			   NULL,
			   NULL, /* FIXME: i18n: Usage of ARM 32-bit
				    mode is %s.  */
			   &setarmcmdlist, &showarmcmdlist);

  /* Add a command to allow the user to force the FPU model.  */
  add_setshow_enum_cmd ("fpu", no_class, fp_model_strings, &current_fp_model,
			_("Set the floating point type."),
			_("Show the floating point type."),
			_("auto - Determine the FP typefrom the OS-ABI.\n\
softfpa - Software FP, mixed-endian doubles on little-endian ARMs.\n\
fpa - FPA co-processor (GCC compiled).\n\
softvfp - Software FP with pure-endian doubles.\n\
vfp - VFP co-processor."),
			set_fp_model_sfunc, show_fp_model,
			&setarmcmdlist, &showarmcmdlist);

  /* Add a command to allow the user to force the ABI.  */
  add_setshow_enum_cmd ("abi", class_support, arm_abi_strings, &arm_abi_string,
			_("Set the ABI."),
			_("Show the ABI."),
			NULL, arm_set_abi, arm_show_abi,
			&setarmcmdlist, &showarmcmdlist);

  /* Add two commands to allow the user to force the assumed
     execution mode.  */
  add_setshow_enum_cmd ("fallback-mode", class_support,
			arm_mode_strings, &arm_fallback_mode_string,
			_("Set the mode assumed when symbols are unavailable."),
			_("Show the mode assumed when symbols are unavailable."),
			NULL, NULL, arm_show_fallback_mode,
			&setarmcmdlist, &showarmcmdlist);
  add_setshow_enum_cmd ("force-mode", class_support,
			arm_mode_strings, &arm_force_mode_string,
			_("Set the mode assumed even when symbols are available."),
			_("Show the mode assumed even when symbols are available."),
			NULL, NULL, arm_show_force_mode,
			&setarmcmdlist, &showarmcmdlist);

  /* Add a command to stop triggering security exceptions when
     unwinding exception stacks.  */
  add_setshow_boolean_cmd ("unwind-secure-frames", no_class, &arm_unwind_secure_frames,
			   _("Set usage of non-secure to secure exception stack unwinding."),
			   _("Show usage of non-secure to secure exception stack unwinding."),
			   _("When on, the debugger can trigger memory access traps."),
			   NULL, arm_show_unwind_secure_frames,
			   &setarmcmdlist, &showarmcmdlist);

  /* Debugging flag.  */
  add_setshow_boolean_cmd ("arm", class_maintenance, &arm_debug,
			   _("Set ARM debugging."),
			   _("Show ARM debugging."),
			   _("When on, arm-specific debugging is enabled."),
			   NULL,
			   NULL, /* FIXME: i18n: "ARM debugging is %s.  */
			   &setdebuglist, &showdebuglist);

#if GDB_SELF_TEST
  selftests::register_test ("arm-record", selftests::arm_record_test);
  selftests::register_test ("arm_analyze_prologue", selftests::arm_analyze_prologue_test);
#endif

}

/* ARM-reversible process record data structures.  */

#define ARM_INSN_SIZE_BYTES 4    
#define THUMB_INSN_SIZE_BYTES 2
#define THUMB2_INSN_SIZE_BYTES 4


/* Position of the bit within a 32-bit ARM instruction
   that defines whether the instruction is a load or store.  */
#define INSN_S_L_BIT_NUM 20

#define REG_ALLOC(REGS, LENGTH, RECORD_BUF) \
	do  \
	  { \
	    unsigned int reg_len = LENGTH; \
	    if (reg_len) \
	      { \
		REGS = XNEWVEC (uint32_t, reg_len); \
		memcpy(&REGS[0], &RECORD_BUF[0], sizeof(uint32_t)*LENGTH); \
	      } \
	  } \
	while (0)

#define MEM_ALLOC(MEMS, LENGTH, RECORD_BUF) \
	do  \
	  { \
	    unsigned int mem_len = LENGTH; \
	    if (mem_len) \
	      { \
		MEMS =  XNEWVEC (struct arm_mem_r, mem_len);  \
		memcpy(&MEMS->len, &RECORD_BUF[0], \
		       sizeof(struct arm_mem_r) * LENGTH); \
	      } \
	  } \
	  while (0)

/* Checks whether insn is already recorded or yet to be decoded. (boolean expression).  */
#define INSN_RECORDED(ARM_RECORD) \
	(0 != (ARM_RECORD)->reg_rec_count || 0 != (ARM_RECORD)->mem_rec_count)

/* ARM memory record structure.  */
struct arm_mem_r
{
  uint32_t len;    /* Record length.  */
  uint32_t addr;   /* Memory address.  */
};

/* ARM instruction record contains opcode of current insn
   and execution state (before entry to decode_insn()),
   contains list of to-be-modified registers and
   memory blocks (on return from decode_insn()).  */

struct arm_insn_decode_record
{
  struct gdbarch *gdbarch;
  struct regcache *regcache;
  CORE_ADDR this_addr;          /* Address of the insn being decoded.  */
  uint32_t arm_insn;            /* Should accommodate thumb.  */
  uint32_t cond;                /* Condition code.  */
  uint32_t opcode;              /* Insn opcode.  */
  uint32_t decode;              /* Insn decode bits.  */
  uint32_t mem_rec_count;       /* No of mem records.  */
  uint32_t reg_rec_count;       /* No of reg records.  */
  uint32_t *arm_regs;           /* Registers to be saved for this record.  */
  struct arm_mem_r *arm_mems;   /* Memory to be saved for this record.  */
};


/* Checks ARM SBZ and SBO mandatory fields.  */

static int
sbo_sbz (uint32_t insn, uint32_t bit_num, uint32_t len, uint32_t sbo)
{
  uint32_t ones = bits (insn, bit_num - 1, (bit_num -1) + (len - 1));

  if (!len)
    return 1;

  if (!sbo)
    ones = ~ones;

  while (ones)
    {
      if (!(ones & sbo))
	{
	  return 0;
	}
      ones = ones >> 1;
    }
  return 1;
}

enum arm_record_result
{
  ARM_RECORD_SUCCESS = 0,
  ARM_RECORD_FAILURE = 1
};

enum arm_record_strx_t
{
  ARM_RECORD_STRH=1,
  ARM_RECORD_STRD
};

enum record_type_t
{
  ARM_RECORD=1,
  THUMB_RECORD,
  THUMB2_RECORD
};


static int
arm_record_strx (arm_insn_decode_record *arm_insn_r, uint32_t *record_buf, 
		 uint32_t *record_buf_mem, arm_record_strx_t str_type)
{

  struct regcache *reg_cache = arm_insn_r->regcache;
  ULONGEST u_regval[2]= {0};

  uint32_t reg_src1 = 0, reg_src2 = 0;
  uint32_t immed_high = 0, immed_low = 0,offset_8 = 0, tgt_mem_addr = 0;

  arm_insn_r->opcode = bits (arm_insn_r->arm_insn, 21, 24);
  arm_insn_r->decode = bits (arm_insn_r->arm_insn, 4, 7);

  if (14 == arm_insn_r->opcode || 10 == arm_insn_r->opcode)
    {
      /* 1) Handle misc store, immediate offset.  */
      immed_low = bits (arm_insn_r->arm_insn, 0, 3);
      immed_high = bits (arm_insn_r->arm_insn, 8, 11);
      reg_src1 = bits (arm_insn_r->arm_insn, 16, 19);
      regcache_raw_read_unsigned (reg_cache, reg_src1,
				  &u_regval[0]);
      if (ARM_PC_REGNUM == reg_src1)
	{
	  /* If R15 was used as Rn, hence current PC+8.  */
	  u_regval[0] = u_regval[0] + 8;
	}
      offset_8 = (immed_high << 4) | immed_low;
      /* Calculate target store address.  */
      if (14 == arm_insn_r->opcode)
	{
	  tgt_mem_addr = u_regval[0] + offset_8;
	}
      else
	{
	  tgt_mem_addr = u_regval[0] - offset_8;
	}
      if (ARM_RECORD_STRH == str_type)
	{
	  record_buf_mem[0] = 2;
	  record_buf_mem[1] = tgt_mem_addr;
	  arm_insn_r->mem_rec_count = 1;
	}
      else if (ARM_RECORD_STRD == str_type)
	{
	  record_buf_mem[0] = 4;
	  record_buf_mem[1] = tgt_mem_addr;
	  record_buf_mem[2] = 4;
	  record_buf_mem[3] = tgt_mem_addr + 4;
	  arm_insn_r->mem_rec_count = 2;
	}
    }
  else if (12 == arm_insn_r->opcode || 8 == arm_insn_r->opcode)
    {
      /* 2) Store, register offset.  */
      /* Get Rm.  */
      reg_src1 = bits (arm_insn_r->arm_insn, 0, 3);
      /* Get Rn.  */
      reg_src2 = bits (arm_insn_r->arm_insn, 16, 19);
      regcache_raw_read_unsigned (reg_cache, reg_src1, &u_regval[0]);
      regcache_raw_read_unsigned (reg_cache, reg_src2, &u_regval[1]);
      if (15 == reg_src2)
	{
	  /* If R15 was used as Rn, hence current PC+8.  */
	  u_regval[0] = u_regval[0] + 8;
	}
      /* Calculate target store address, Rn +/- Rm, register offset.  */
      if (12 == arm_insn_r->opcode)
	{
	  tgt_mem_addr = u_regval[0] + u_regval[1];
	}
      else
	{
	  tgt_mem_addr = u_regval[1] - u_regval[0];
	}
      if (ARM_RECORD_STRH == str_type)
	{
	  record_buf_mem[0] = 2;
	  record_buf_mem[1] = tgt_mem_addr;
	  arm_insn_r->mem_rec_count = 1;
	}
      else if (ARM_RECORD_STRD == str_type)
	{
	  record_buf_mem[0] = 4;
	  record_buf_mem[1] = tgt_mem_addr;
	  record_buf_mem[2] = 4;
	  record_buf_mem[3] = tgt_mem_addr + 4;
	  arm_insn_r->mem_rec_count = 2;
	}
    }
  else if (11 == arm_insn_r->opcode || 15 == arm_insn_r->opcode
	   || 2 == arm_insn_r->opcode  || 6 == arm_insn_r->opcode)
    {
      /* 3) Store, immediate pre-indexed.  */
      /* 5) Store, immediate post-indexed.  */
      immed_low = bits (arm_insn_r->arm_insn, 0, 3);
      immed_high = bits (arm_insn_r->arm_insn, 8, 11);
      offset_8 = (immed_high << 4) | immed_low;
      reg_src1 = bits (arm_insn_r->arm_insn, 16, 19);
      regcache_raw_read_unsigned (reg_cache, reg_src1, &u_regval[0]);
      /* Calculate target store address, Rn +/- Rm, register offset.  */
      if (15 == arm_insn_r->opcode || 6 == arm_insn_r->opcode)
	{
	  tgt_mem_addr = u_regval[0] + offset_8;
	}
      else
	{
	  tgt_mem_addr = u_regval[0] - offset_8;
	}
      if (ARM_RECORD_STRH == str_type)
	{
	  record_buf_mem[0] = 2;
	  record_buf_mem[1] = tgt_mem_addr;
	  arm_insn_r->mem_rec_count = 1;
	}
      else if (ARM_RECORD_STRD == str_type)
	{
	  record_buf_mem[0] = 4;
	  record_buf_mem[1] = tgt_mem_addr;
	  record_buf_mem[2] = 4;
	  record_buf_mem[3] = tgt_mem_addr + 4;
	  arm_insn_r->mem_rec_count = 2;
	}
      /* Record Rn also as it changes.  */
      *(record_buf) = bits (arm_insn_r->arm_insn, 16, 19);
      arm_insn_r->reg_rec_count = 1;
    }
  else if (9 == arm_insn_r->opcode || 13 == arm_insn_r->opcode
	   || 0 == arm_insn_r->opcode || 4 == arm_insn_r->opcode)
    {
      /* 4) Store, register pre-indexed.  */
      /* 6) Store, register post -indexed.  */
      reg_src1 = bits (arm_insn_r->arm_insn, 0, 3);
      reg_src2 = bits (arm_insn_r->arm_insn, 16, 19);
      regcache_raw_read_unsigned (reg_cache, reg_src1, &u_regval[0]);
      regcache_raw_read_unsigned (reg_cache, reg_src2, &u_regval[1]);
      /* Calculate target store address, Rn +/- Rm, register offset.  */
      if (13 == arm_insn_r->opcode || 4 == arm_insn_r->opcode)
	{
	  tgt_mem_addr = u_regval[0] + u_regval[1];
	}
      else
	{
	  tgt_mem_addr = u_regval[1] - u_regval[0];
	}
      if (ARM_RECORD_STRH == str_type)
	{
	  record_buf_mem[0] = 2;
	  record_buf_mem[1] = tgt_mem_addr;
	  arm_insn_r->mem_rec_count = 1;
	}
      else if (ARM_RECORD_STRD == str_type)
	{
	  record_buf_mem[0] = 4;
	  record_buf_mem[1] = tgt_mem_addr;
	  record_buf_mem[2] = 4;
	  record_buf_mem[3] = tgt_mem_addr + 4;
	  arm_insn_r->mem_rec_count = 2;
	}
      /* Record Rn also as it changes.  */
      *(record_buf) = bits (arm_insn_r->arm_insn, 16, 19);
      arm_insn_r->reg_rec_count = 1;
    }
  return 0;
}

/* Handling ARM extension space insns.  */

static int
arm_record_extension_space (arm_insn_decode_record *arm_insn_r)
{
  int ret = 0;  /* Return value: -1:record failure ;  0:success  */
  uint32_t opcode1 = 0, opcode2 = 0, insn_op1 = 0;
  uint32_t record_buf[8], record_buf_mem[8];
  uint32_t reg_src1 = 0;
  struct regcache *reg_cache = arm_insn_r->regcache;
  ULONGEST u_regval = 0;

  gdb_assert (!INSN_RECORDED(arm_insn_r));
  /* Handle unconditional insn extension space.  */

  opcode1 = bits (arm_insn_r->arm_insn, 20, 27);
  opcode2 = bits (arm_insn_r->arm_insn, 4, 7);
  if (arm_insn_r->cond)
    {
      /* PLD has no affect on architectural state, it just affects
	 the caches.  */
      if (5 == ((opcode1 & 0xE0) >> 5))
	{
	  /* BLX(1) */
	  record_buf[0] = ARM_PS_REGNUM;
	  record_buf[1] = ARM_LR_REGNUM;
	  arm_insn_r->reg_rec_count = 2;
	}
      /* STC2, LDC2, MCR2, MRC2, CDP2: <TBD>, co-processor insn.  */
    }


  opcode1 = bits (arm_insn_r->arm_insn, 25, 27);
  if (3 == opcode1 && bit (arm_insn_r->arm_insn, 4))
    {
      ret = -1;
      /* Undefined instruction on ARM V5; need to handle if later 
	 versions define it.  */
    }

  opcode1 = bits (arm_insn_r->arm_insn, 24, 27);
  opcode2 = bits (arm_insn_r->arm_insn, 4, 7);
  insn_op1 = bits (arm_insn_r->arm_insn, 20, 23);

  /* Handle arithmetic insn extension space.  */
  if (!opcode1 && 9 == opcode2 && 1 != arm_insn_r->cond
      && !INSN_RECORDED(arm_insn_r))
    {
      /* Handle MLA(S) and MUL(S).  */
      if (in_inclusive_range (insn_op1, 0U, 3U))
	{
	  record_buf[0] = bits (arm_insn_r->arm_insn, 12, 15);
	  record_buf[1] = ARM_PS_REGNUM;
	  arm_insn_r->reg_rec_count = 2;
	}
      else if (in_inclusive_range (insn_op1, 4U, 15U))
	{
	  /* Handle SMLAL(S), SMULL(S), UMLAL(S), UMULL(S).  */
	  record_buf[0] = bits (arm_insn_r->arm_insn, 16, 19);
	  record_buf[1] = bits (arm_insn_r->arm_insn, 12, 15);
	  record_buf[2] = ARM_PS_REGNUM;
	  arm_insn_r->reg_rec_count = 3;
	}
    }

  opcode1 = bits (arm_insn_r->arm_insn, 26, 27);
  opcode2 = bits (arm_insn_r->arm_insn, 23, 24);
  insn_op1 = bits (arm_insn_r->arm_insn, 21, 22);

  /* Handle control insn extension space.  */

  if (!opcode1 && 2 == opcode2 && !bit (arm_insn_r->arm_insn, 20)
      && 1 != arm_insn_r->cond && !INSN_RECORDED(arm_insn_r))
    {
      if (!bit (arm_insn_r->arm_insn,25))
	{
	  if (!bits (arm_insn_r->arm_insn, 4, 7))
	    {
	      if ((0 == insn_op1) || (2 == insn_op1))
		{
		  /* MRS.  */
		  record_buf[0] = bits (arm_insn_r->arm_insn, 12, 15);
		  arm_insn_r->reg_rec_count = 1;
		}
	      else if (1 == insn_op1)
		{
		  /* CSPR is going to be changed.  */
		  record_buf[0] = ARM_PS_REGNUM;
		  arm_insn_r->reg_rec_count = 1;
		}
	      else if (3 == insn_op1)
		{
		  /* SPSR is going to be changed.  */
		  /* We need to get SPSR value, which is yet to be done.  */
		  return -1;
		}
	    }
	  else if (1 == bits (arm_insn_r->arm_insn, 4, 7))
	    {
	      if (1 == insn_op1)
		{
		  /* BX.  */
		  record_buf[0] = ARM_PS_REGNUM;
		  arm_insn_r->reg_rec_count = 1;
		}
	      else if (3 == insn_op1)
		{
		  /* CLZ.  */
		  record_buf[0] = bits (arm_insn_r->arm_insn, 12, 15);
		  arm_insn_r->reg_rec_count = 1;
		}
	    }
	  else if (3 == bits (arm_insn_r->arm_insn, 4, 7))
	    {
	      /* BLX.  */
	      record_buf[0] = ARM_PS_REGNUM;
	      record_buf[1] = ARM_LR_REGNUM;
	      arm_insn_r->reg_rec_count = 2;
	    }
	  else if (5 == bits (arm_insn_r->arm_insn, 4, 7))
	    {
	      /* QADD, QSUB, QDADD, QDSUB */
	      record_buf[0] = ARM_PS_REGNUM;
	      record_buf[1] = bits (arm_insn_r->arm_insn, 12, 15);
	      arm_insn_r->reg_rec_count = 2;
	    }
	  else if (7 == bits (arm_insn_r->arm_insn, 4, 7))
	    {
	      /* BKPT.  */
	      record_buf[0] = ARM_PS_REGNUM;
	      record_buf[1] = ARM_LR_REGNUM;
	      arm_insn_r->reg_rec_count = 2;

	      /* Save SPSR also;how?  */
	      return -1;
	    }
	  else if(8 == bits (arm_insn_r->arm_insn, 4, 7) 
		  || 10 == bits (arm_insn_r->arm_insn, 4, 7)
		  || 12 == bits (arm_insn_r->arm_insn, 4, 7)
		  || 14 == bits (arm_insn_r->arm_insn, 4, 7)
		 )
	    {
	      if (0 == insn_op1 || 1 == insn_op1)
		{
		  /* SMLA<x><y>, SMLAW<y>, SMULW<y>.  */
		  /* We dont do optimization for SMULW<y> where we
		     need only Rd.  */
		  record_buf[0] = bits (arm_insn_r->arm_insn, 12, 15);
		  record_buf[1] = ARM_PS_REGNUM;
		  arm_insn_r->reg_rec_count = 2;
		}
	      else if (2 == insn_op1)
		{
		  /* SMLAL<x><y>.  */
		  record_buf[0] = bits (arm_insn_r->arm_insn, 12, 15);
		  record_buf[1] = bits (arm_insn_r->arm_insn, 16, 19);
		  arm_insn_r->reg_rec_count = 2;
		}
	      else if (3 == insn_op1)
		{
		  /* SMUL<x><y>.  */
		  record_buf[0] = bits (arm_insn_r->arm_insn, 12, 15);
		  arm_insn_r->reg_rec_count = 1;
		}
	    }
	}
      else
	{
	  /* MSR : immediate form.  */
	  if (1 == insn_op1)
	    {
	      /* CSPR is going to be changed.  */
	      record_buf[0] = ARM_PS_REGNUM;
	      arm_insn_r->reg_rec_count = 1;
	    }
	  else if (3 == insn_op1)
	    {
	      /* SPSR is going to be changed.  */
	      /* we need to get SPSR value, which is yet to be done  */
	      return -1;
	    }
	}
    }

  opcode1 = bits (arm_insn_r->arm_insn, 25, 27);
  opcode2 = bits (arm_insn_r->arm_insn, 20, 24);
  insn_op1 = bits (arm_insn_r->arm_insn, 5, 6);

  /* Handle load/store insn extension space.  */

  if (!opcode1 && bit (arm_insn_r->arm_insn, 7) 
      && bit (arm_insn_r->arm_insn, 4) && 1 != arm_insn_r->cond
      && !INSN_RECORDED(arm_insn_r))
    {
      /* SWP/SWPB.  */
      if (0 == insn_op1)
	{
	  /* These insn, changes register and memory as well.  */
	  /* SWP or SWPB insn.  */
	  /* Get memory address given by Rn.  */
	  reg_src1 = bits (arm_insn_r->arm_insn, 16, 19);
	  regcache_raw_read_unsigned (reg_cache, reg_src1, &u_regval);
	  /* SWP insn ?, swaps word.  */
	  if (8 == arm_insn_r->opcode)
	    {
	      record_buf_mem[0] = 4;
	    }
	  else
	    {
	      /* SWPB insn, swaps only byte.  */
	      record_buf_mem[0] = 1;
	    }
	  record_buf_mem[1] = u_regval;
	  arm_insn_r->mem_rec_count = 1;
	  record_buf[0] = bits (arm_insn_r->arm_insn, 12, 15);
	  arm_insn_r->reg_rec_count = 1;
	}
      else if (1 == insn_op1 && !bit (arm_insn_r->arm_insn, 20))
	{
	  /* STRH.  */
	  arm_record_strx(arm_insn_r, &record_buf[0], &record_buf_mem[0],
			  ARM_RECORD_STRH);
	}
      else if (2 == insn_op1 && !bit (arm_insn_r->arm_insn, 20))
	{
	  /* LDRD.  */
	  record_buf[0] = bits (arm_insn_r->arm_insn, 12, 15);
	  record_buf[1] = record_buf[0] + 1;
	  arm_insn_r->reg_rec_count = 2;
	}
      else if (3 == insn_op1 && !bit (arm_insn_r->arm_insn, 20))
	{
	  /* STRD.  */
	  arm_record_strx(arm_insn_r, &record_buf[0], &record_buf_mem[0],
			ARM_RECORD_STRD);
	}
      else if (bit (arm_insn_r->arm_insn, 20) && insn_op1 <= 3)
	{
	  /* LDRH, LDRSB, LDRSH.  */
	  record_buf[0] = bits (arm_insn_r->arm_insn, 12, 15);
	  arm_insn_r->reg_rec_count = 1;
	}

    }

  opcode1 = bits (arm_insn_r->arm_insn, 23, 27);
  if (24 == opcode1 && bit (arm_insn_r->arm_insn, 21)
      && !INSN_RECORDED(arm_insn_r))
    {
      ret = -1;
      /* Handle coprocessor insn extension space.  */
    }

  /* To be done for ARMv5 and later; as of now we return -1.  */
  if (-1 == ret)
    return ret;

  REG_ALLOC (arm_insn_r->arm_regs, arm_insn_r->reg_rec_count, record_buf);
  MEM_ALLOC (arm_insn_r->arm_mems, arm_insn_r->mem_rec_count, record_buf_mem);

  return ret;
}

/* Handling opcode 000 insns.  */

static int
arm_record_data_proc_misc_ld_str (arm_insn_decode_record *arm_insn_r)
{
  struct regcache *reg_cache = arm_insn_r->regcache;
  uint32_t record_buf[8], record_buf_mem[8];
  ULONGEST u_regval[2] = {0};

  uint32_t reg_src1 = 0;
  uint32_t opcode1 = 0;

  arm_insn_r->opcode = bits (arm_insn_r->arm_insn, 21, 24);
  arm_insn_r->decode = bits (arm_insn_r->arm_insn, 4, 7);
  opcode1 = bits (arm_insn_r->arm_insn, 20, 24);

  if (!((opcode1 & 0x19) == 0x10))
    {
      /* Data-processing (register) and Data-processing (register-shifted
	 register */
      /* Out of 11 shifter operands mode, all the insn modifies destination
	 register, which is specified by 13-16 decode.  */
      record_buf[0] = bits (arm_insn_r->arm_insn, 12, 15);
      record_buf[1] = ARM_PS_REGNUM;
      arm_insn_r->reg_rec_count = 2;
    }
  else if ((arm_insn_r->decode < 8) && ((opcode1 & 0x19) == 0x10))
    {
      /* Miscellaneous instructions */

      if (3 == arm_insn_r->decode && 0x12 == opcode1
	  && sbo_sbz (arm_insn_r->arm_insn, 9, 12, 1))
	{
	  /* Handle BLX, branch and link/exchange.  */
	  if (9 == arm_insn_r->opcode)
	    {
	      /* Branch is chosen by setting T bit of CSPR, bitp[0] of Rm,
		 and R14 stores the return address.  */
	      record_buf[0] = ARM_PS_REGNUM;
	      record_buf[1] = ARM_LR_REGNUM;
	      arm_insn_r->reg_rec_count = 2;
	    }
	}
      else if (7 == arm_insn_r->decode && 0x12 == opcode1)
	{
	  /* Handle enhanced software breakpoint insn, BKPT.  */
	  /* CPSR is changed to be executed in ARM state,  disabling normal
	     interrupts, entering abort mode.  */
	  /* According to high vector configuration PC is set.  */
	  /* user hit breakpoint and type reverse, in
	     that case, we need to go back with previous CPSR and
	     Program Counter.  */
	  record_buf[0] = ARM_PS_REGNUM;
	  record_buf[1] = ARM_LR_REGNUM;
	  arm_insn_r->reg_rec_count = 2;

	  /* Save SPSR also; how?  */
	  return -1;
	}
      else if (1 == arm_insn_r->decode && 0x12 == opcode1
	       && sbo_sbz (arm_insn_r->arm_insn, 9, 12, 1))
	{
	  /* Handle BX, branch and link/exchange.  */
	  /* Branch is chosen by setting T bit of CSPR, bitp[0] of Rm.  */
	  record_buf[0] = ARM_PS_REGNUM;
	  arm_insn_r->reg_rec_count = 1;
	}
      else if (1 == arm_insn_r->decode && 0x16 == opcode1
	       && sbo_sbz (arm_insn_r->arm_insn, 9, 4, 1)
	       && sbo_sbz (arm_insn_r->arm_insn, 17, 4, 1))
	{
	  /* Count leading zeros: CLZ.  */
	  record_buf[0] = bits (arm_insn_r->arm_insn, 12, 15);
	  arm_insn_r->reg_rec_count = 1;
	}
      else if (!bit (arm_insn_r->arm_insn, INSN_S_L_BIT_NUM)
	       && (8 == arm_insn_r->opcode || 10 == arm_insn_r->opcode)
	       && sbo_sbz (arm_insn_r->arm_insn, 17, 4, 1)
	       && sbo_sbz (arm_insn_r->arm_insn, 1, 12, 0))
	{
	  /* Handle MRS insn.  */
	  record_buf[0] = bits (arm_insn_r->arm_insn, 12, 15);
	  arm_insn_r->reg_rec_count = 1;
	}
    }
  else if (9 == arm_insn_r->decode && opcode1 < 0x10)
    {
      /* Multiply and multiply-accumulate */

      /* Handle multiply instructions.  */
      /* MLA, MUL, SMLAL, SMULL, UMLAL, UMULL.  */
      if (0 == arm_insn_r->opcode || 1 == arm_insn_r->opcode)
	{
	  /* Handle MLA and MUL.  */
	  record_buf[0] = bits (arm_insn_r->arm_insn, 16, 19);
	  record_buf[1] = ARM_PS_REGNUM;
	  arm_insn_r->reg_rec_count = 2;
	}
      else if (4 <= arm_insn_r->opcode && 7 >= arm_insn_r->opcode)
	{
	  /* Handle SMLAL, SMULL, UMLAL, UMULL.  */
	  record_buf[0] = bits (arm_insn_r->arm_insn, 16, 19);
	  record_buf[1] = bits (arm_insn_r->arm_insn, 12, 15);
	  record_buf[2] = ARM_PS_REGNUM;
	  arm_insn_r->reg_rec_count = 3;
	}
    }
  else if (9 == arm_insn_r->decode  && opcode1 > 0x10)
    {
      /* Synchronization primitives */

      /* Handling SWP, SWPB.  */
      /* These insn, changes register and memory as well.  */
      /* SWP or SWPB insn.  */

      reg_src1 = bits (arm_insn_r->arm_insn, 16, 19);
      regcache_raw_read_unsigned (reg_cache, reg_src1, &u_regval[0]);
      /* SWP insn ?, swaps word.  */
      if (8 == arm_insn_r->opcode)
	{
	  record_buf_mem[0] = 4;
	}
      else
	{
	  /* SWPB insn, swaps only byte.  */
	  record_buf_mem[0] = 1;
	}
      record_buf_mem[1] = u_regval[0];
      arm_insn_r->mem_rec_count = 1;
      record_buf[0] = bits (arm_insn_r->arm_insn, 12, 15);
      arm_insn_r->reg_rec_count = 1;
    }
  else if (11 == arm_insn_r->decode || 13 == arm_insn_r->decode
	   || 15 == arm_insn_r->decode)
    {
      if ((opcode1 & 0x12) == 2)
	{
	  /* Extra load/store (unprivileged) */
	  return -1;
	}
      else
	{
	  /* Extra load/store */
	  switch (bits (arm_insn_r->arm_insn, 5, 6))
	    {
	    case 1:
	      if ((opcode1 & 0x05) == 0x0 || (opcode1 & 0x05) == 0x4)
		{
		  /* STRH (register), STRH (immediate) */
		  arm_record_strx (arm_insn_r, &record_buf[0],
				   &record_buf_mem[0], ARM_RECORD_STRH);
		}
	      else if ((opcode1 & 0x05) == 0x1)
		{
		  /* LDRH (register) */
		  record_buf[0] = bits (arm_insn_r->arm_insn, 12, 15);
		  arm_insn_r->reg_rec_count = 1;

		  if (bit (arm_insn_r->arm_insn, 21))
		    {
		      /* Write back to Rn.  */
		      record_buf[arm_insn_r->reg_rec_count++]
			= bits (arm_insn_r->arm_insn, 16, 19);
		    }
		}
	      else if ((opcode1 & 0x05) == 0x5)
		{
		  /* LDRH (immediate), LDRH (literal) */
		  int rn = bits (arm_insn_r->arm_insn, 16, 19);

		  record_buf[0] = bits (arm_insn_r->arm_insn, 12, 15);
		  arm_insn_r->reg_rec_count = 1;

		  if (rn != 15)
		    {
		      /*LDRH (immediate) */
		      if (bit (arm_insn_r->arm_insn, 21))
			{
			  /* Write back to Rn.  */
			  record_buf[arm_insn_r->reg_rec_count++] = rn;
			}
		    }
		}
	      else
		return -1;
	      break;
	    case 2:
	      if ((opcode1 & 0x05) == 0x0)
		{
		  /* LDRD (register) */
		  record_buf[0] = bits (arm_insn_r->arm_insn, 12, 15);
		  record_buf[1] = record_buf[0] + 1;
		  arm_insn_r->reg_rec_count = 2;

		  if (bit (arm_insn_r->arm_insn, 21))
		    {
		      /* Write back to Rn.  */
		      record_buf[arm_insn_r->reg_rec_count++]
			= bits (arm_insn_r->arm_insn, 16, 19);
		    }
		}
	      else if ((opcode1 & 0x05) == 0x1)
		{
		  /* LDRSB (register) */
		  record_buf[0] = bits (arm_insn_r->arm_insn, 12, 15);
		  arm_insn_r->reg_rec_count = 1;

		  if (bit (arm_insn_r->arm_insn, 21))
		    {
		      /* Write back to Rn.  */
		      record_buf[arm_insn_r->reg_rec_count++]
			= bits (arm_insn_r->arm_insn, 16, 19);
		    }
		}
	      else if ((opcode1 & 0x05) == 0x4 || (opcode1 & 0x05) == 0x5)
		{
		  /* LDRD (immediate), LDRD (literal), LDRSB (immediate),
		     LDRSB (literal) */
		  int rn = bits (arm_insn_r->arm_insn, 16, 19);

		  record_buf[0] = bits (arm_insn_r->arm_insn, 12, 15);
		  arm_insn_r->reg_rec_count = 1;

		  if (rn != 15)
		    {
		      /*LDRD (immediate), LDRSB (immediate) */
		      if (bit (arm_insn_r->arm_insn, 21))
			{
			  /* Write back to Rn.  */
			  record_buf[arm_insn_r->reg_rec_count++] = rn;
			}
		    }
		}
	      else
		return -1;
	      break;
	    case 3:
	      if ((opcode1 & 0x05) == 0x0)
		{
		  /* STRD (register) */
		  arm_record_strx (arm_insn_r, &record_buf[0],
				   &record_buf_mem[0], ARM_RECORD_STRD);
		}
	      else if ((opcode1 & 0x05) == 0x1)
		{
		  /* LDRSH (register) */
		  record_buf[0] = bits (arm_insn_r->arm_insn, 12, 15);
		  arm_insn_r->reg_rec_count = 1;

		  if (bit (arm_insn_r->arm_insn, 21))
		    {
		      /* Write back to Rn.  */
		      record_buf[arm_insn_r->reg_rec_count++]
			= bits (arm_insn_r->arm_insn, 16, 19);
		    }
		}
	      else if ((opcode1 & 0x05) == 0x4)
		{
		  /* STRD (immediate) */
		  arm_record_strx (arm_insn_r, &record_buf[0],
				   &record_buf_mem[0], ARM_RECORD_STRD);
		}
	      else if ((opcode1 & 0x05) == 0x5)
		{
		  /* LDRSH (immediate), LDRSH (literal) */
		  record_buf[0] = bits (arm_insn_r->arm_insn, 12, 15);
		  arm_insn_r->reg_rec_count = 1;

		  if (bit (arm_insn_r->arm_insn, 21))
		    {
		      /* Write back to Rn.  */
		      record_buf[arm_insn_r->reg_rec_count++]
			= bits (arm_insn_r->arm_insn, 16, 19);
		    }
		}
	      else
		return -1;
	      break;
	    default:
	      return -1;
	    }
	}
    }
  else
    {
      return -1;
    }

  REG_ALLOC (arm_insn_r->arm_regs, arm_insn_r->reg_rec_count, record_buf);
  MEM_ALLOC (arm_insn_r->arm_mems, arm_insn_r->mem_rec_count, record_buf_mem);
  return 0;
}

/* Handling opcode 001 insns.  */

static int
arm_record_data_proc_imm (arm_insn_decode_record *arm_insn_r)
{
  uint32_t record_buf[8], record_buf_mem[8];

  arm_insn_r->opcode = bits (arm_insn_r->arm_insn, 21, 24);
  arm_insn_r->decode = bits (arm_insn_r->arm_insn, 4, 7);

  if ((9 == arm_insn_r->opcode || 11 == arm_insn_r->opcode)
      && 2 == bits (arm_insn_r->arm_insn, 20, 21)
      && sbo_sbz (arm_insn_r->arm_insn, 13, 4, 1)
     )
    {
      /* Handle MSR insn.  */
      if (9 == arm_insn_r->opcode)
	{
	  /* CSPR is going to be changed.  */
	  record_buf[0] = ARM_PS_REGNUM;
	  arm_insn_r->reg_rec_count = 1;
	}
      else
	{
	  /* SPSR is going to be changed.  */
	}
    }
  else if (arm_insn_r->opcode <= 15)
    {
      /* Normal data processing insns.  */
      /* Out of 11 shifter operands mode, all the insn modifies destination
	 register, which is specified by 13-16 decode.  */
      record_buf[0] = bits (arm_insn_r->arm_insn, 12, 15);
      record_buf[1] = ARM_PS_REGNUM;
      arm_insn_r->reg_rec_count = 2;
    }
  else
    {
      return -1;
    }

  REG_ALLOC (arm_insn_r->arm_regs, arm_insn_r->reg_rec_count, record_buf);
  MEM_ALLOC (arm_insn_r->arm_mems, arm_insn_r->mem_rec_count, record_buf_mem);
  return 0;
}

static int
arm_record_media (arm_insn_decode_record *arm_insn_r)
{
  uint32_t record_buf[8];

  switch (bits (arm_insn_r->arm_insn, 22, 24))
    {
    case 0:
      /* Parallel addition and subtraction, signed */
    case 1:
      /* Parallel addition and subtraction, unsigned */
    case 2:
    case 3:
      /* Packing, unpacking, saturation and reversal */
      {
	int rd = bits (arm_insn_r->arm_insn, 12, 15);

	record_buf[arm_insn_r->reg_rec_count++] = rd;
      }
      break;

    case 4:
    case 5:
      /* Signed multiplies */
      {
	int rd = bits (arm_insn_r->arm_insn, 16, 19);
	unsigned int op1 = bits (arm_insn_r->arm_insn, 20, 22);

	record_buf[arm_insn_r->reg_rec_count++] = rd;
	if (op1 == 0x0)
	  record_buf[arm_insn_r->reg_rec_count++] = ARM_PS_REGNUM;
	else if (op1 == 0x4)
	  record_buf[arm_insn_r->reg_rec_count++]
	    = bits (arm_insn_r->arm_insn, 12, 15);
      }
      break;

    case 6:
      {
	if (bit (arm_insn_r->arm_insn, 21)
	    && bits (arm_insn_r->arm_insn, 5, 6) == 0x2)
	  {
	    /* SBFX */
	    record_buf[arm_insn_r->reg_rec_count++]
	      = bits (arm_insn_r->arm_insn, 12, 15);
	  }
	else if (bits (arm_insn_r->arm_insn, 20, 21) == 0x0
		 && bits (arm_insn_r->arm_insn, 5, 7) == 0x0)
	  {
	    /* USAD8 and USADA8 */
	    record_buf[arm_insn_r->reg_rec_count++]
	      = bits (arm_insn_r->arm_insn, 16, 19);
	  }
      }
      break;

    case 7:
      {
	if (bits (arm_insn_r->arm_insn, 20, 21) == 0x3
	    && bits (arm_insn_r->arm_insn, 5, 7) == 0x7)
	  {
	    /* Permanently UNDEFINED */
	    return -1;
	  }
	else
	  {
	    /* BFC, BFI and UBFX */
	    record_buf[arm_insn_r->reg_rec_count++]
	      = bits (arm_insn_r->arm_insn, 12, 15);
	  }
      }
      break;

    default:
      return -1;
    }

  REG_ALLOC (arm_insn_r->arm_regs, arm_insn_r->reg_rec_count, record_buf);

  return 0;
}

/* Handle ARM mode instructions with opcode 010.  */

static int
arm_record_ld_st_imm_offset (arm_insn_decode_record *arm_insn_r)
{
  struct regcache *reg_cache = arm_insn_r->regcache;

  uint32_t reg_base , reg_dest;
  uint32_t offset_12, tgt_mem_addr;
  uint32_t record_buf[8], record_buf_mem[8];
  unsigned char wback;
  ULONGEST u_regval;

  /* Calculate wback.  */
  wback = (bit (arm_insn_r->arm_insn, 24) == 0)
	  || (bit (arm_insn_r->arm_insn, 21) == 1);

  arm_insn_r->reg_rec_count = 0;
  reg_base = bits (arm_insn_r->arm_insn, 16, 19);

  if (bit (arm_insn_r->arm_insn, INSN_S_L_BIT_NUM))
    {
      /* LDR (immediate), LDR (literal), LDRB (immediate), LDRB (literal), LDRBT
	 and LDRT.  */

      reg_dest = bits (arm_insn_r->arm_insn, 12, 15);
      record_buf[arm_insn_r->reg_rec_count++] = reg_dest;

      /* The LDR instruction is capable of doing branching.  If MOV LR, PC
	 preceeds a LDR instruction having R15 as reg_base, it
	 emulates a branch and link instruction, and hence we need to save
	 CPSR and PC as well.  */
      if (ARM_PC_REGNUM == reg_dest)
	record_buf[arm_insn_r->reg_rec_count++] = ARM_PS_REGNUM;

      /* If wback is true, also save the base register, which is going to be
	 written to.  */
      if (wback)
	record_buf[arm_insn_r->reg_rec_count++] = reg_base;
    }
  else
    {
      /* STR (immediate), STRB (immediate), STRBT and STRT.  */

      offset_12 = bits (arm_insn_r->arm_insn, 0, 11);
      regcache_raw_read_unsigned (reg_cache, reg_base, &u_regval);

      /* Handle bit U.  */
      if (bit (arm_insn_r->arm_insn, 23))
	{
	  /* U == 1: Add the offset. */
	  tgt_mem_addr = (uint32_t) u_regval + offset_12;
	}
      else
	{
	  /* U == 0: subtract the offset. */
	  tgt_mem_addr = (uint32_t) u_regval - offset_12;
	}

      /* Bit 22 tells us whether the store instruction writes 1 byte or 4
	 bytes.  */
      if (bit (arm_insn_r->arm_insn, 22))
	{
	  /* STRB and STRBT: 1 byte.  */
	  record_buf_mem[0] = 1;
	}
      else
	{
	  /* STR and STRT: 4 bytes.  */
	  record_buf_mem[0] = 4;
	}

      /* Handle bit P.  */
      if (bit (arm_insn_r->arm_insn, 24))
	record_buf_mem[1] = tgt_mem_addr;
      else
	record_buf_mem[1] = (uint32_t) u_regval;

      arm_insn_r->mem_rec_count = 1;

      /* If wback is true, also save the base register, which is going to be
	 written to.  */
      if (wback)
	record_buf[arm_insn_r->reg_rec_count++] = reg_base;
    }

  REG_ALLOC (arm_insn_r->arm_regs, arm_insn_r->reg_rec_count, record_buf);
  MEM_ALLOC (arm_insn_r->arm_mems, arm_insn_r->mem_rec_count, record_buf_mem);
  return 0;
}

/* Handling opcode 011 insns.  */

static int
arm_record_ld_st_reg_offset (arm_insn_decode_record *arm_insn_r)
{
  struct regcache *reg_cache = arm_insn_r->regcache;

  uint32_t shift_imm = 0;
  uint32_t reg_src1 = 0, reg_src2 = 0, reg_dest = 0;
  uint32_t offset_12 = 0, tgt_mem_addr = 0;
  uint32_t record_buf[8], record_buf_mem[8];

  LONGEST s_word;
  ULONGEST u_regval[2];

  if (bit (arm_insn_r->arm_insn, 4))
    return arm_record_media (arm_insn_r);

  arm_insn_r->opcode = bits (arm_insn_r->arm_insn, 21, 24);
  arm_insn_r->decode = bits (arm_insn_r->arm_insn, 4, 7);

  /* Handle enhanced store insns and LDRD DSP insn,
     order begins according to addressing modes for store insns
     STRH insn.  */

  /* LDR or STR?  */
  if (bit (arm_insn_r->arm_insn, INSN_S_L_BIT_NUM))
    {
      reg_dest = bits (arm_insn_r->arm_insn, 12, 15);
      /* LDR insn has a capability to do branching, if
	 MOV LR, PC is preceded by LDR insn having Rn as R15
	 in that case, it emulates branch and link insn, and hence we
	 need to save CSPR and PC as well.  */
      if (15 != reg_dest)
	{
	  record_buf[0] = bits (arm_insn_r->arm_insn, 12, 15);
	  arm_insn_r->reg_rec_count = 1;
	}
      else
	{
	  record_buf[0] = reg_dest;
	  record_buf[1] = ARM_PS_REGNUM;
	  arm_insn_r->reg_rec_count = 2;
	}
    }
  else
    {
      if (! bits (arm_insn_r->arm_insn, 4, 11))
	{
	  /* Store insn, register offset and register pre-indexed,
	     register post-indexed.  */
	  /* Get Rm.  */
	  reg_src1 = bits (arm_insn_r->arm_insn, 0, 3);
	  /* Get Rn.  */
	  reg_src2 = bits (arm_insn_r->arm_insn, 16, 19);
	  regcache_raw_read_unsigned (reg_cache, reg_src1
				      , &u_regval[0]);
	  regcache_raw_read_unsigned (reg_cache, reg_src2
				      , &u_regval[1]);
	  if (15 == reg_src2)
	    {
	      /* If R15 was used as Rn, hence current PC+8.  */
	      /* Pre-indexed mode doesnt reach here ; illegal insn.  */
		u_regval[0] = u_regval[0] + 8;
	    }
	  /* Calculate target store address, Rn +/- Rm, register offset.  */
	  /* U == 1.  */
	  if (bit (arm_insn_r->arm_insn, 23))
	    {
	      tgt_mem_addr = u_regval[0] + u_regval[1];
	    }
	  else
	    {
	      tgt_mem_addr = u_regval[1] - u_regval[0];
	    }

	  switch (arm_insn_r->opcode)
	    {
	      /* STR.  */
	      case 8:
	      case 12:
	      /* STR.  */    
	      case 9:
	      case 13:
	      /* STRT.  */
	      case 1:
	      case 5:
	      /* STR.  */
	      case 0:
	      case 4:
		record_buf_mem[0] = 4;
	      break;

	      /* STRB.  */
	      case 10:
	      case 14:
	      /* STRB.  */
	      case 11:
	      case 15:
	      /* STRBT.  */    
	      case 3:
	      case 7:
	      /* STRB.  */
	      case 2:
	      case 6:
		record_buf_mem[0] = 1;
	      break;

	      default:
		gdb_assert_not_reached ("no decoding pattern found");
	      break;
	    }
	  record_buf_mem[1] = tgt_mem_addr;
	  arm_insn_r->mem_rec_count = 1;

	  if (9 == arm_insn_r->opcode || 11 == arm_insn_r->opcode
	      || 13 == arm_insn_r->opcode || 15 == arm_insn_r->opcode
	      || 0 == arm_insn_r->opcode || 2 == arm_insn_r->opcode
	      || 4 == arm_insn_r->opcode || 6 == arm_insn_r->opcode
	      || 1 == arm_insn_r->opcode || 3 == arm_insn_r->opcode
	      || 5 == arm_insn_r->opcode || 7 == arm_insn_r->opcode
	     )
	    {
	      /* Rn is going to be changed in pre-indexed mode and
		 post-indexed mode as well.  */
	      record_buf[0] = reg_src2;
	      arm_insn_r->reg_rec_count = 1;
	    }
	}
      else
	{
	  /* Store insn, scaled register offset; scaled pre-indexed.  */
	  offset_12 = bits (arm_insn_r->arm_insn, 5, 6);
	  /* Get Rm.  */
	  reg_src1 = bits (arm_insn_r->arm_insn, 0, 3);
	  /* Get Rn.  */
	  reg_src2 = bits (arm_insn_r->arm_insn, 16, 19);
	  /* Get shift_imm.  */
	  shift_imm = bits (arm_insn_r->arm_insn, 7, 11);
	  regcache_raw_read_unsigned (reg_cache, reg_src1, &u_regval[0]);
	  regcache_raw_read_signed (reg_cache, reg_src1, &s_word);
	  regcache_raw_read_unsigned (reg_cache, reg_src2, &u_regval[1]);
	  /* Offset_12 used as shift.  */
	  switch (offset_12)
	    {
	      case 0:
		/* Offset_12 used as index.  */
		offset_12 = u_regval[0] << shift_imm;
	      break;

	      case 1:
		offset_12 = (!shift_imm) ? 0 : u_regval[0] >> shift_imm;
	      break;

	      case 2:
		if (!shift_imm)
		  {
		    if (bit (u_regval[0], 31))
		      {
			offset_12 = 0xFFFFFFFF;
		      }
		    else
		      {
			offset_12 = 0;
		      }
		  }
		else
		  {
		    /* This is arithmetic shift.  */
		    offset_12 = s_word >> shift_imm;
		  }
		break;

	      case 3:
		if (!shift_imm)
		  {
		    regcache_raw_read_unsigned (reg_cache, ARM_PS_REGNUM,
						&u_regval[1]);
		    /* Get C flag value and shift it by 31.  */
		    offset_12 = (((bit (u_regval[1], 29)) << 31) \
				  | (u_regval[0]) >> 1);
		  }
		else
		  {
		    offset_12 = (u_regval[0] >> shift_imm) \
				| (u_regval[0] <<
				(sizeof(uint32_t) - shift_imm));
		  }
	      break;

	      default:
		gdb_assert_not_reached ("no decoding pattern found");
	      break;
	    }

	  regcache_raw_read_unsigned (reg_cache, reg_src2, &u_regval[1]);
	  /* bit U set.  */
	  if (bit (arm_insn_r->arm_insn, 23))
	    {
	      tgt_mem_addr = u_regval[1] + offset_12;
	    }
	  else
	    {
	      tgt_mem_addr = u_regval[1] - offset_12;
	    }

	  switch (arm_insn_r->opcode)
	    {
	      /* STR.  */
	      case 8:
	      case 12:
	      /* STR.  */    
	      case 9:
	      case 13:
	      /* STRT.  */
	      case 1:
	      case 5:
	      /* STR.  */
	      case 0:
	      case 4:
		record_buf_mem[0] = 4;
	      break;

	      /* STRB.  */
	      case 10:
	      case 14:
	      /* STRB.  */
	      case 11:
	      case 15:
	      /* STRBT.  */    
	      case 3:
	      case 7:
	      /* STRB.  */
	      case 2:
	      case 6:
		record_buf_mem[0] = 1;
	      break;

	      default:
		gdb_assert_not_reached ("no decoding pattern found");
	      break;
	    }
	  record_buf_mem[1] = tgt_mem_addr;
	  arm_insn_r->mem_rec_count = 1;

	  if (9 == arm_insn_r->opcode || 11 == arm_insn_r->opcode
	      || 13 == arm_insn_r->opcode || 15 == arm_insn_r->opcode
	      || 0 == arm_insn_r->opcode || 2 == arm_insn_r->opcode
	      || 4 == arm_insn_r->opcode || 6 == arm_insn_r->opcode
	      || 1 == arm_insn_r->opcode || 3 == arm_insn_r->opcode
	      || 5 == arm_insn_r->opcode || 7 == arm_insn_r->opcode
	     )
	    {
	      /* Rn is going to be changed in register scaled pre-indexed
		 mode,and scaled post indexed mode.  */
	      record_buf[0] = reg_src2;
	      arm_insn_r->reg_rec_count = 1;
	    }
	}
    }

  REG_ALLOC (arm_insn_r->arm_regs, arm_insn_r->reg_rec_count, record_buf);
  MEM_ALLOC (arm_insn_r->arm_mems, arm_insn_r->mem_rec_count, record_buf_mem);
  return 0;
}

/* Handle ARM mode instructions with opcode 100.  */

static int
arm_record_ld_st_multiple (arm_insn_decode_record *arm_insn_r)
{
  struct regcache *reg_cache = arm_insn_r->regcache;
  uint32_t register_count = 0, register_bits;
  uint32_t reg_base, addr_mode;
  uint32_t record_buf[24], record_buf_mem[48];
  uint32_t wback;
  ULONGEST u_regval;

  /* Fetch the list of registers.  */
  register_bits = bits (arm_insn_r->arm_insn, 0, 15);
  arm_insn_r->reg_rec_count = 0;

  /* Fetch the base register that contains the address we are loading data
     to.  */
  reg_base = bits (arm_insn_r->arm_insn, 16, 19);

  /* Calculate wback.  */
  wback = (bit (arm_insn_r->arm_insn, 21) == 1);

  if (bit (arm_insn_r->arm_insn, INSN_S_L_BIT_NUM))
    {
      /* LDM/LDMIA/LDMFD, LDMDA/LDMFA, LDMDB and LDMIB.  */

      /* Find out which registers are going to be loaded from memory.  */
      while (register_bits)
	{
	  if (register_bits & 0x00000001)
	    record_buf[arm_insn_r->reg_rec_count++] = register_count;
	  register_bits = register_bits >> 1;
	  register_count++;
	}

  
      /* If wback is true, also save the base register, which is going to be
	 written to.  */
      if (wback)
	record_buf[arm_insn_r->reg_rec_count++] = reg_base;

      /* Save the CPSR register.  */
      record_buf[arm_insn_r->reg_rec_count++] = ARM_PS_REGNUM;
    }
  else
    {
      /* STM (STMIA, STMEA), STMDA (STMED), STMDB (STMFD) and STMIB (STMFA).  */

      addr_mode = bits (arm_insn_r->arm_insn, 23, 24); 

      regcache_raw_read_unsigned (reg_cache, reg_base, &u_regval);

      /* Find out how many registers are going to be stored to memory.  */
      while (register_bits)
	{
	  if (register_bits & 0x00000001)
	    register_count++;
	  register_bits = register_bits >> 1;
	}

      switch (addr_mode)
	{
	  /* STMDA (STMED): Decrement after.  */
	  case 0:
	  record_buf_mem[1] = (uint32_t) u_regval
			      - register_count * ARM_INT_REGISTER_SIZE + 4;
	  break;
	  /* STM (STMIA, STMEA): Increment after.  */
	  case 1:
	  record_buf_mem[1] = (uint32_t) u_regval;
	  break;
	  /* STMDB (STMFD): Decrement before.  */
	  case 2:
	  record_buf_mem[1] = (uint32_t) u_regval
			      - register_count * ARM_INT_REGISTER_SIZE;
	  break;
	  /* STMIB (STMFA): Increment before.  */
	  case 3:
	  record_buf_mem[1] = (uint32_t) u_regval + ARM_INT_REGISTER_SIZE;
	  break;
	  default:
	    gdb_assert_not_reached ("no decoding pattern found");
	  break;
	}

      record_buf_mem[0] = register_count * ARM_INT_REGISTER_SIZE;
      arm_insn_r->mem_rec_count = 1;

      /* If wback is true, also save the base register, which is going to be
	 written to.  */
      if (wback)
	record_buf[arm_insn_r->reg_rec_count++] = reg_base;
    }

  REG_ALLOC (arm_insn_r->arm_regs, arm_insn_r->reg_rec_count, record_buf);
  MEM_ALLOC (arm_insn_r->arm_mems, arm_insn_r->mem_rec_count, record_buf_mem);
  return 0;
}

/* Handling opcode 101 insns.  */

static int
arm_record_b_bl (arm_insn_decode_record *arm_insn_r)
{
  uint32_t record_buf[8];

  /* Handle B, BL, BLX(1) insns.  */
  /* B simply branches so we do nothing here.  */
  /* Note: BLX(1) doesnt fall here but instead it falls into
     extension space.  */
  if (bit (arm_insn_r->arm_insn, 24))
    {
      record_buf[0] = ARM_LR_REGNUM;
      arm_insn_r->reg_rec_count = 1;
    }

  REG_ALLOC (arm_insn_r->arm_regs, arm_insn_r->reg_rec_count, record_buf);

  return 0;
}

static int
arm_record_unsupported_insn (arm_insn_decode_record *arm_insn_r)
{
  gdb_printf (gdb_stderr,
	      _("Process record does not support instruction "
		"0x%0x at address %s.\n"),arm_insn_r->arm_insn,
	      paddress (arm_insn_r->gdbarch, arm_insn_r->this_addr));

  return -1;
}

/* Record handler for vector data transfer instructions.  */

static int
arm_record_vdata_transfer_insn (arm_insn_decode_record *arm_insn_r)
{
  uint32_t bits_a, bit_c, bit_l, reg_t, reg_v;
  uint32_t record_buf[4];

  reg_t = bits (arm_insn_r->arm_insn, 12, 15);
  reg_v = bits (arm_insn_r->arm_insn, 21, 23);
  bits_a = bits (arm_insn_r->arm_insn, 21, 23);
  bit_l = bit (arm_insn_r->arm_insn, 20);
  bit_c = bit (arm_insn_r->arm_insn, 8);

  /* Handle VMOV instruction.  */
  if (bit_l && bit_c)
    {
      record_buf[0] = reg_t;
      arm_insn_r->reg_rec_count = 1;
    }
  else if (bit_l && !bit_c)
    {
      /* Handle VMOV instruction.  */
      if (bits_a == 0x00)
	{
	  record_buf[0] = reg_t;
	  arm_insn_r->reg_rec_count = 1;
	}
      /* Handle VMRS instruction.  */
      else if (bits_a == 0x07)
	{
	  if (reg_t == 15)
	    reg_t = ARM_PS_REGNUM;

	  record_buf[0] = reg_t;
	  arm_insn_r->reg_rec_count = 1;
	}
    }
  else if (!bit_l && !bit_c)
    {
      /* Handle VMOV instruction.  */
      if (bits_a == 0x00)
	{
	  record_buf[0] = ARM_D0_REGNUM + reg_v;

	  arm_insn_r->reg_rec_count = 1;
	}
      /* Handle VMSR instruction.  */
      else if (bits_a == 0x07)
	{
	  record_buf[0] = ARM_FPSCR_REGNUM;
	  arm_insn_r->reg_rec_count = 1;
	}
    }
  else if (!bit_l && bit_c)
    {
      /* Handle VMOV instruction.  */
      if (!(bits_a & 0x04))
	{
	  record_buf[0] = (reg_v | (bit (arm_insn_r->arm_insn, 7) << 4))
			  + ARM_D0_REGNUM;
	  arm_insn_r->reg_rec_count = 1;
	}
      /* Handle VDUP instruction.  */
      else
	{
	  if (bit (arm_insn_r->arm_insn, 21))
	    {
	      reg_v = reg_v | (bit (arm_insn_r->arm_insn, 7) << 4);
	      record_buf[0] = reg_v + ARM_D0_REGNUM;
	      record_buf[1] = reg_v + ARM_D0_REGNUM + 1;
	      arm_insn_r->reg_rec_count = 2;
	    }
	  else
	    {
	      reg_v = reg_v | (bit (arm_insn_r->arm_insn, 7) << 4);
	      record_buf[0] = reg_v + ARM_D0_REGNUM;
	      arm_insn_r->reg_rec_count = 1;
	    }
	}
    }

  REG_ALLOC (arm_insn_r->arm_regs, arm_insn_r->reg_rec_count, record_buf);
  return 0;
}

/* Record handler for extension register load/store instructions.  */

static int
arm_record_exreg_ld_st_insn (arm_insn_decode_record *arm_insn_r)
{
  uint32_t opcode, single_reg;
  uint8_t op_vldm_vstm;
  uint32_t record_buf[8], record_buf_mem[128];
  ULONGEST u_regval = 0;

  struct regcache *reg_cache = arm_insn_r->regcache;

  opcode = bits (arm_insn_r->arm_insn, 20, 24);
  single_reg = !bit (arm_insn_r->arm_insn, 8);
  op_vldm_vstm = opcode & 0x1b;

  /* Handle VMOV instructions.  */
  if ((opcode & 0x1e) == 0x04)
    {
      if (bit (arm_insn_r->arm_insn, 20)) /* to_arm_registers bit 20? */
	{
	  record_buf[0] = bits (arm_insn_r->arm_insn, 12, 15);
	  record_buf[1] = bits (arm_insn_r->arm_insn, 16, 19);
	  arm_insn_r->reg_rec_count = 2;
	}
      else
	{
	  uint8_t reg_m = bits (arm_insn_r->arm_insn, 0, 3);
	  uint8_t bit_m = bit (arm_insn_r->arm_insn, 5);

	  if (single_reg)
	    {
	      /* The first S register number m is REG_M:M (M is bit 5),
		 the corresponding D register number is REG_M:M / 2, which
		 is REG_M.  */
	      record_buf[arm_insn_r->reg_rec_count++] = ARM_D0_REGNUM + reg_m;
	      /* The second S register number is REG_M:M + 1, the
		 corresponding D register number is (REG_M:M + 1) / 2.
		 IOW, if bit M is 1, the first and second S registers
		 are mapped to different D registers, otherwise, they are
		 in the same D register.  */
	      if (bit_m)
		{
		  record_buf[arm_insn_r->reg_rec_count++]
		    = ARM_D0_REGNUM + reg_m + 1;
		}
	    }
	  else
	    {
	      record_buf[0] = ((bit_m << 4) + reg_m + ARM_D0_REGNUM);
	      arm_insn_r->reg_rec_count = 1;
	    }
	}
    }
  /* Handle VSTM and VPUSH instructions.  */
  else if (op_vldm_vstm == 0x08 || op_vldm_vstm == 0x0a
	   || op_vldm_vstm == 0x12)
    {
      uint32_t start_address, reg_rn, imm_off32, imm_off8, memory_count;
      uint32_t memory_index = 0;

      reg_rn = bits (arm_insn_r->arm_insn, 16, 19);
      regcache_raw_read_unsigned (reg_cache, reg_rn, &u_regval);
      imm_off8 = bits (arm_insn_r->arm_insn, 0, 7);
      imm_off32 = imm_off8 << 2;
      memory_count = imm_off8;

      if (bit (arm_insn_r->arm_insn, 23))
	start_address = u_regval;
      else
	start_address = u_regval - imm_off32;

      if (bit (arm_insn_r->arm_insn, 21))
	{
	  record_buf[0] = reg_rn;
	  arm_insn_r->reg_rec_count = 1;
	}

      while (memory_count > 0)
	{
	  if (single_reg)
	    {
	      record_buf_mem[memory_index] = 4;
	      record_buf_mem[memory_index + 1] = start_address;
	      start_address = start_address + 4;
	      memory_index = memory_index + 2;
	    }
	  else
	    {
	      record_buf_mem[memory_index] = 4;
	      record_buf_mem[memory_index + 1] = start_address;
	      record_buf_mem[memory_index + 2] = 4;
	      record_buf_mem[memory_index + 3] = start_address + 4;
	      start_address = start_address + 8;
	      memory_index = memory_index + 4;
	    }
	  memory_count--;
	}
      arm_insn_r->mem_rec_count = (memory_index >> 1);
    }
  /* Handle VLDM instructions.  */
  else if (op_vldm_vstm == 0x09 || op_vldm_vstm == 0x0b
	   || op_vldm_vstm == 0x13)
    {
      uint32_t reg_count, reg_vd;
      uint32_t reg_index = 0;
      uint32_t bit_d = bit (arm_insn_r->arm_insn, 22);

      reg_vd = bits (arm_insn_r->arm_insn, 12, 15);
      reg_count = bits (arm_insn_r->arm_insn, 0, 7);

      /* REG_VD is the first D register number.  If the instruction
	 loads memory to S registers (SINGLE_REG is TRUE), the register
	 number is (REG_VD << 1 | bit D), so the corresponding D
	 register number is (REG_VD << 1 | bit D) / 2 = REG_VD.  */
      if (!single_reg)
	reg_vd = reg_vd | (bit_d << 4);

      if (bit (arm_insn_r->arm_insn, 21) /* write back */)
	record_buf[reg_index++] = bits (arm_insn_r->arm_insn, 16, 19);

      /* If the instruction loads memory to D register, REG_COUNT should
	 be divided by 2, according to the ARM Architecture Reference
	 Manual.  If the instruction loads memory to S register, divide by
	 2 as well because two S registers are mapped to D register.  */
      reg_count  = reg_count / 2;
      if (single_reg && bit_d)
	{
	  /* Increase the register count if S register list starts from
	     an odd number (bit d is one).  */
	  reg_count++;
	}

      while (reg_count > 0)
	{
	  record_buf[reg_index++] = ARM_D0_REGNUM + reg_vd + reg_count - 1;
	  reg_count--;
	}
      arm_insn_r->reg_rec_count = reg_index;
    }
  /* VSTR Vector store register.  */
  else if ((opcode & 0x13) == 0x10)
    {
      uint32_t start_address, reg_rn, imm_off32, imm_off8;
      uint32_t memory_index = 0;

      reg_rn = bits (arm_insn_r->arm_insn, 16, 19);
      regcache_raw_read_unsigned (reg_cache, reg_rn, &u_regval);
      imm_off8 = bits (arm_insn_r->arm_insn, 0, 7);
      imm_off32 = imm_off8 << 2;

      if (bit (arm_insn_r->arm_insn, 23))
	start_address = u_regval + imm_off32;
      else
	start_address = u_regval - imm_off32;

      if (single_reg)
	{
	  record_buf_mem[memory_index] = 4;
	  record_buf_mem[memory_index + 1] = start_address;
	  arm_insn_r->mem_rec_count = 1;
	}
      else
	{
	  record_buf_mem[memory_index] = 4;
	  record_buf_mem[memory_index + 1] = start_address;
	  record_buf_mem[memory_index + 2] = 4;
	  record_buf_mem[memory_index + 3] = start_address + 4;
	  arm_insn_r->mem_rec_count = 2;
	}
    }
  /* VLDR Vector load register.  */
  else if ((opcode & 0x13) == 0x11)
    {
      uint32_t reg_vd = bits (arm_insn_r->arm_insn, 12, 15);

      if (!single_reg)
	{
	  reg_vd = reg_vd | (bit (arm_insn_r->arm_insn, 22) << 4);
	  record_buf[0] = ARM_D0_REGNUM + reg_vd;
	}
      else
	{
	  reg_vd = (reg_vd << 1) | bit (arm_insn_r->arm_insn, 22);
	  /* Record register D rather than pseudo register S.  */
	  record_buf[0] = ARM_D0_REGNUM + reg_vd / 2;
	}
      arm_insn_r->reg_rec_count = 1;
    }

  REG_ALLOC (arm_insn_r->arm_regs, arm_insn_r->reg_rec_count, record_buf);
  MEM_ALLOC (arm_insn_r->arm_mems, arm_insn_r->mem_rec_count, record_buf_mem);
  return 0;
}

/* Record handler for arm/thumb mode VFP data processing instructions.  */

static int
arm_record_vfp_data_proc_insn (arm_insn_decode_record *arm_insn_r)
{
  uint32_t opc1, opc2, opc3, dp_op_sz, bit_d, reg_vd;
  uint32_t record_buf[4];
  enum insn_types {INSN_T0, INSN_T1, INSN_T2, INSN_T3, INSN_INV};
  enum insn_types curr_insn_type = INSN_INV;

  reg_vd = bits (arm_insn_r->arm_insn, 12, 15);
  opc1 = bits (arm_insn_r->arm_insn, 20, 23);
  opc2 = bits (arm_insn_r->arm_insn, 16, 19);
  opc3 = bits (arm_insn_r->arm_insn, 6, 7);
  dp_op_sz = bit (arm_insn_r->arm_insn, 8);
  bit_d = bit (arm_insn_r->arm_insn, 22);
  /* Mask off the "D" bit.  */
  opc1 = opc1 & ~0x04;

  /* Handle VMLA, VMLS.  */
  if (opc1 == 0x00)
    {
      if (bit (arm_insn_r->arm_insn, 10))
	{
	  if (bit (arm_insn_r->arm_insn, 6))
	    curr_insn_type = INSN_T0;
	  else
	    curr_insn_type = INSN_T1;
	}
      else
	{
	  if (dp_op_sz)
	    curr_insn_type = INSN_T1;
	  else
	    curr_insn_type = INSN_T2;
	}
    }
  /* Handle VNMLA, VNMLS, VNMUL.  */
  else if (opc1 == 0x01)
    {
      if (dp_op_sz)
	curr_insn_type = INSN_T1;
      else
	curr_insn_type = INSN_T2;
    }
  /* Handle VMUL.  */
  else if (opc1 == 0x02 && !(opc3 & 0x01))
    {
      if (bit (arm_insn_r->arm_insn, 10))
	{
	  if (bit (arm_insn_r->arm_insn, 6))
	    curr_insn_type = INSN_T0;
	  else
	    curr_insn_type = INSN_T1;
	}
      else
	{
	  if (dp_op_sz)
	    curr_insn_type = INSN_T1;
	  else
	    curr_insn_type = INSN_T2;
	}
    }
  /* Handle VADD, VSUB.  */
  else if (opc1 == 0x03)
    {
      if (!bit (arm_insn_r->arm_insn, 9))
	{
	  if (bit (arm_insn_r->arm_insn, 6))
	    curr_insn_type = INSN_T0;
	  else
	    curr_insn_type = INSN_T1;
	}
      else
	{
	  if (dp_op_sz)
	    curr_insn_type = INSN_T1;
	  else
	    curr_insn_type = INSN_T2;
	}
    }
  /* Handle VDIV.  */
  else if (opc1 == 0x08)
    {
      if (dp_op_sz)
	curr_insn_type = INSN_T1;
      else
	curr_insn_type = INSN_T2;
    }
  /* Handle all other vfp data processing instructions.  */
  else if (opc1 == 0x0b)
    {
      /* Handle VMOV.  */
      if (!(opc3 & 0x01) || (opc2 == 0x00 && opc3 == 0x01))
	{
	  if (bit (arm_insn_r->arm_insn, 4))
	    {
	      if (bit (arm_insn_r->arm_insn, 6))
		curr_insn_type = INSN_T0;
	      else
		curr_insn_type = INSN_T1;
	    }
	  else
	    {
	      if (dp_op_sz)
		curr_insn_type = INSN_T1;
	      else
		curr_insn_type = INSN_T2;
	    }
	}
      /* Handle VNEG and VABS.  */
      else if ((opc2 == 0x01 && opc3 == 0x01)
	      || (opc2 == 0x00 && opc3 == 0x03))
	{
	  if (!bit (arm_insn_r->arm_insn, 11))
	    {
	      if (bit (arm_insn_r->arm_insn, 6))
		curr_insn_type = INSN_T0;
	      else
		curr_insn_type = INSN_T1;
	    }
	  else
	    {
	      if (dp_op_sz)
		curr_insn_type = INSN_T1;
	      else
		curr_insn_type = INSN_T2;
	    }
	}
      /* Handle VSQRT.  */
      else if (opc2 == 0x01 && opc3 == 0x03)
	{
	  if (dp_op_sz)
	    curr_insn_type = INSN_T1;
	  else
	    curr_insn_type = INSN_T2;
	}
      /* Handle VCVT.  */
      else if (opc2 == 0x07 && opc3 == 0x03)
	{
	  if (!dp_op_sz)
	    curr_insn_type = INSN_T1;
	  else
	    curr_insn_type = INSN_T2;
	}
      else if (opc3 & 0x01)
	{
	  /* Handle VCVT.  */
	  if ((opc2 == 0x08) || (opc2 & 0x0e) == 0x0c)
	    {
	      if (!bit (arm_insn_r->arm_insn, 18))
		curr_insn_type = INSN_T2;
	      else
		{
		  if (dp_op_sz)
		    curr_insn_type = INSN_T1;
		  else
		    curr_insn_type = INSN_T2;
		}
	    }
	  /* Handle VCVT.  */
	  else if ((opc2 & 0x0e) == 0x0a || (opc2 & 0x0e) == 0x0e)
	    {
	      if (dp_op_sz)
		curr_insn_type = INSN_T1;
	      else
		curr_insn_type = INSN_T2;
	    }
	  /* Handle VCVTB, VCVTT.  */
	  else if ((opc2 & 0x0e) == 0x02)
	    curr_insn_type = INSN_T2;
	  /* Handle VCMP, VCMPE.  */
	  else if ((opc2 & 0x0e) == 0x04)
	    curr_insn_type = INSN_T3;
	}
    }

  switch (curr_insn_type)
    {
      case INSN_T0:
	reg_vd = reg_vd | (bit_d << 4);
	record_buf[0] = reg_vd + ARM_D0_REGNUM;
	record_buf[1] = reg_vd + ARM_D0_REGNUM + 1;
	arm_insn_r->reg_rec_count = 2;
	break;

      case INSN_T1:
	reg_vd = reg_vd | (bit_d << 4);
	record_buf[0] = reg_vd + ARM_D0_REGNUM;
	arm_insn_r->reg_rec_count = 1;
	break;

      case INSN_T2:
	reg_vd = (reg_vd << 1) | bit_d;
	record_buf[0] = reg_vd + ARM_D0_REGNUM;
	arm_insn_r->reg_rec_count = 1;
	break;

      case INSN_T3:
	record_buf[0] = ARM_FPSCR_REGNUM;
	arm_insn_r->reg_rec_count = 1;
	break;

      default:
	gdb_assert_not_reached ("no decoding pattern found");
	break;
    }

  REG_ALLOC (arm_insn_r->arm_regs, arm_insn_r->reg_rec_count, record_buf);
  return 0;
}

/* Handling opcode 110 insns.  */

static int
arm_record_asimd_vfp_coproc (arm_insn_decode_record *arm_insn_r)
{
  uint32_t op1, op1_ebit, coproc;

  coproc = bits (arm_insn_r->arm_insn, 8, 11);
  op1 = bits (arm_insn_r->arm_insn, 20, 25);
  op1_ebit = bit (arm_insn_r->arm_insn, 20);

  if ((coproc & 0x0e) == 0x0a)
    {
      /* Handle extension register ld/st instructions.  */
      if (!(op1 & 0x20))
	return arm_record_exreg_ld_st_insn (arm_insn_r);

      /* 64-bit transfers between arm core and extension registers.  */
      if ((op1 & 0x3e) == 0x04)
	return arm_record_exreg_ld_st_insn (arm_insn_r);
    }
  else
    {
      /* Handle coprocessor ld/st instructions.  */
      if (!(op1 & 0x3a))
	{
	  /* Store.  */
	  if (!op1_ebit)
	    return arm_record_unsupported_insn (arm_insn_r);
	  else
	    /* Load.  */
	    return arm_record_unsupported_insn (arm_insn_r);
	}

      /* Move to coprocessor from two arm core registers.  */
      if (op1 == 0x4)
	return arm_record_unsupported_insn (arm_insn_r);

      /* Move to two arm core registers from coprocessor.  */
      if (op1 == 0x5)
	{
	  uint32_t reg_t[2];

	  reg_t[0] = bits (arm_insn_r->arm_insn, 12, 15);
	  reg_t[1] = bits (arm_insn_r->arm_insn, 16, 19);
	  arm_insn_r->reg_rec_count = 2;

	  REG_ALLOC (arm_insn_r->arm_regs, arm_insn_r->reg_rec_count, reg_t);
	  return 0;
       }
    }
  return arm_record_unsupported_insn (arm_insn_r);
}

/* Handling opcode 111 insns.  */

static int
arm_record_coproc_data_proc (arm_insn_decode_record *arm_insn_r)
{
  uint32_t op, op1_ebit, coproc, bits_24_25;
  arm_gdbarch_tdep *tdep
    = gdbarch_tdep<arm_gdbarch_tdep> (arm_insn_r->gdbarch);
  struct regcache *reg_cache = arm_insn_r->regcache;

  arm_insn_r->opcode = bits (arm_insn_r->arm_insn, 24, 27);
  coproc = bits (arm_insn_r->arm_insn, 8, 11);
  op1_ebit = bit (arm_insn_r->arm_insn, 20);
  op = bit (arm_insn_r->arm_insn, 4);
  bits_24_25 = bits (arm_insn_r->arm_insn, 24, 25);

  /* Handle arm SWI/SVC system call instructions.  */
  if (bits_24_25 == 0x3)
    {
      if (tdep->arm_syscall_record != NULL)
	{
	  ULONGEST svc_operand, svc_number;

	  svc_operand = (0x00ffffff & arm_insn_r->arm_insn);

	  if (svc_operand)  /* OABI.  */
	    svc_number = svc_operand - 0x900000;
	  else /* EABI.  */
	    regcache_raw_read_unsigned (reg_cache, 7, &svc_number);

	  return tdep->arm_syscall_record (reg_cache, svc_number);
	}
      else
	{
	  gdb_printf (gdb_stderr, _("no syscall record support\n"));
	  return -1;
	}
    }
  else if (bits_24_25 == 0x02)
    {
      if (op)
	{
	  if ((coproc & 0x0e) == 0x0a)
	    {
	      /* 8, 16, and 32-bit transfer */
	      return arm_record_vdata_transfer_insn (arm_insn_r);
	    }
	  else
	    {
	      if (op1_ebit)
		{
		  /* MRC, MRC2 */
		  uint32_t record_buf[1];

		  record_buf[0] = bits (arm_insn_r->arm_insn, 12, 15);
		  if (record_buf[0] == 15)
		    record_buf[0] = ARM_PS_REGNUM;

		  arm_insn_r->reg_rec_count = 1;
		  REG_ALLOC (arm_insn_r->arm_regs, arm_insn_r->reg_rec_count,
			     record_buf);
		  return 0;
		}
	      else
		{
		  /* MCR, MCR2 */
		  return -1;
		}
	    }
	}
      else
	{
	  if ((coproc & 0x0e) == 0x0a)
	    {
	      /* VFP data-processing instructions.  */
	      return arm_record_vfp_data_proc_insn (arm_insn_r);
	    }
	  else
	    {
	      /* CDP, CDP2 */
	      return -1;
	    }
	}
    }
  else
    {
      unsigned int op1 = bits (arm_insn_r->arm_insn, 20, 25);

      if (op1 == 5)
	{
	  if ((coproc & 0x0e) != 0x0a)
	    {
	      /* MRRC, MRRC2 */
	      return -1;
	    }
	}
      else if (op1 == 4 || op1 == 5)
	{
	  if ((coproc & 0x0e) == 0x0a)
	    {
	      /* 64-bit transfers between ARM core and extension */
	      return -1;
	    }
	  else if (op1 == 4)
	    {
	      /* MCRR, MCRR2 */
	      return -1;
	    }
	}
      else if (op1 == 0 || op1 == 1)
	{
	  /* UNDEFINED */
	  return -1;
	}
      else
	{
	  if ((coproc & 0x0e) == 0x0a)
	    {
	      /* Extension register load/store */
	    }
	  else
	    {
	      /* STC, STC2, LDC, LDC2 */
	    }
	  return -1;
	}
    }

  return -1;
}

/* Handling opcode 000 insns.  */

static int
thumb_record_shift_add_sub (arm_insn_decode_record *thumb_insn_r)
{
  uint32_t record_buf[8];
  uint32_t reg_src1 = 0;

  reg_src1 = bits (thumb_insn_r->arm_insn, 0, 2);

  record_buf[0] = ARM_PS_REGNUM;
  record_buf[1] = reg_src1;
  thumb_insn_r->reg_rec_count = 2;

  REG_ALLOC (thumb_insn_r->arm_regs, thumb_insn_r->reg_rec_count, record_buf);

  return 0;
}


/* Handling opcode 001 insns.  */

static int
thumb_record_add_sub_cmp_mov (arm_insn_decode_record *thumb_insn_r)
{
  uint32_t record_buf[8];
  uint32_t reg_src1 = 0;

  reg_src1 = bits (thumb_insn_r->arm_insn, 8, 10);

  record_buf[0] = ARM_PS_REGNUM;
  record_buf[1] = reg_src1;
  thumb_insn_r->reg_rec_count = 2;

  REG_ALLOC (thumb_insn_r->arm_regs, thumb_insn_r->reg_rec_count, record_buf);

  return 0;
}

/* Handling opcode 010 insns.  */

static int
thumb_record_ld_st_reg_offset (arm_insn_decode_record *thumb_insn_r)
{
  struct regcache *reg_cache =  thumb_insn_r->regcache;
  uint32_t record_buf[8], record_buf_mem[8];

  uint32_t reg_src1 = 0, reg_src2 = 0;
  uint32_t opcode1 = 0, opcode2 = 0, opcode3 = 0;

  ULONGEST u_regval[2] = {0};

  opcode1 = bits (thumb_insn_r->arm_insn, 10, 12);

  if (bit (thumb_insn_r->arm_insn, 12))
    {
      /* Handle load/store register offset.  */
      uint32_t opB = bits (thumb_insn_r->arm_insn, 9, 11);

      if (in_inclusive_range (opB, 4U, 7U))
	{
	  /* LDR(2), LDRB(2) , LDRH(2), LDRSB, LDRSH.  */
	  reg_src1 = bits (thumb_insn_r->arm_insn,0, 2);
	  record_buf[0] = reg_src1;
	  thumb_insn_r->reg_rec_count = 1;
	}
      else if (in_inclusive_range (opB, 0U, 2U))
	{
	  /* STR(2), STRB(2), STRH(2) .  */
	  reg_src1 = bits (thumb_insn_r->arm_insn, 3, 5);
	  reg_src2 = bits (thumb_insn_r->arm_insn, 6, 8);
	  regcache_raw_read_unsigned (reg_cache, reg_src1, &u_regval[0]);
	  regcache_raw_read_unsigned (reg_cache, reg_src2, &u_regval[1]);
	  if (0 == opB)
	    record_buf_mem[0] = 4;    /* STR (2).  */
	  else if (2 == opB)
	    record_buf_mem[0] = 1;    /*  STRB (2).  */
	  else if (1 == opB)
	    record_buf_mem[0] = 2;    /* STRH (2).  */
	  record_buf_mem[1] = u_regval[0] + u_regval[1];
	  thumb_insn_r->mem_rec_count = 1;
	}
    }
  else if (bit (thumb_insn_r->arm_insn, 11))
    {
      /* Handle load from literal pool.  */
      /* LDR(3).  */
      reg_src1 = bits (thumb_insn_r->arm_insn, 8, 10);
      record_buf[0] = reg_src1;
      thumb_insn_r->reg_rec_count = 1;
    }
  else if (opcode1)
    {
      /* Special data instructions and branch and exchange */
      opcode2 = bits (thumb_insn_r->arm_insn, 8, 9);
      opcode3 = bits (thumb_insn_r->arm_insn, 0, 2);
      if ((3 == opcode2) && (!opcode3))
	{
	  /* Branch with exchange.  */
	  record_buf[0] = ARM_PS_REGNUM;
	  thumb_insn_r->reg_rec_count = 1;
	}
      else
	{
	  /* Format 8; special data processing insns.  */
	  record_buf[0] = ARM_PS_REGNUM;
	  record_buf[1] = (bit (thumb_insn_r->arm_insn, 7) << 3
			   | bits (thumb_insn_r->arm_insn, 0, 2));
	  thumb_insn_r->reg_rec_count = 2;
	}
    }
  else
    {
      /* Format 5; data processing insns.  */
      reg_src1 = bits (thumb_insn_r->arm_insn, 0, 2);
      if (bit (thumb_insn_r->arm_insn, 7))
	{
	  reg_src1 = reg_src1 + 8;
	}
      record_buf[0] = ARM_PS_REGNUM;
      record_buf[1] = reg_src1;
      thumb_insn_r->reg_rec_count = 2;
    }

  REG_ALLOC (thumb_insn_r->arm_regs, thumb_insn_r->reg_rec_count, record_buf);
  MEM_ALLOC (thumb_insn_r->arm_mems, thumb_insn_r->mem_rec_count,
	     record_buf_mem);

  return 0;
}

/* Handling opcode 001 insns.  */

static int
thumb_record_ld_st_imm_offset (arm_insn_decode_record *thumb_insn_r)
{
  struct regcache *reg_cache = thumb_insn_r->regcache;
  uint32_t record_buf[8], record_buf_mem[8];

  uint32_t reg_src1 = 0;
  uint32_t opcode = 0, immed_5 = 0;

  ULONGEST u_regval = 0;

  opcode = bits (thumb_insn_r->arm_insn, 11, 12);

  if (opcode)
    {
      /* LDR(1).  */
      reg_src1 = bits (thumb_insn_r->arm_insn, 0, 2);
      record_buf[0] = reg_src1;
      thumb_insn_r->reg_rec_count = 1;
    }
  else
    {
      /* STR(1).  */
      reg_src1 = bits (thumb_insn_r->arm_insn, 3, 5);
      immed_5 = bits (thumb_insn_r->arm_insn, 6, 10);
      regcache_raw_read_unsigned (reg_cache, reg_src1, &u_regval);
      record_buf_mem[0] = 4;
      record_buf_mem[1] = u_regval + (immed_5 * 4);
      thumb_insn_r->mem_rec_count = 1;
    }

  REG_ALLOC (thumb_insn_r->arm_regs, thumb_insn_r->reg_rec_count, record_buf);
  MEM_ALLOC (thumb_insn_r->arm_mems, thumb_insn_r->mem_rec_count, 
	     record_buf_mem);

  return 0;
}

/* Handling opcode 100 insns.  */

static int
thumb_record_ld_st_stack (arm_insn_decode_record *thumb_insn_r)
{
  struct regcache *reg_cache = thumb_insn_r->regcache;
  uint32_t record_buf[8], record_buf_mem[8];

  uint32_t reg_src1 = 0;
  uint32_t opcode = 0, immed_8 = 0, immed_5 = 0;

  ULONGEST u_regval = 0;

  opcode = bits (thumb_insn_r->arm_insn, 11, 12);

  if (3 == opcode)
    {
      /* LDR(4).  */
      reg_src1 = bits (thumb_insn_r->arm_insn, 8, 10);
      record_buf[0] = reg_src1;
      thumb_insn_r->reg_rec_count = 1;
    }
  else if (1 == opcode)
    {
      /* LDRH(1).  */
      reg_src1 = bits (thumb_insn_r->arm_insn, 0, 2);
      record_buf[0] = reg_src1;
      thumb_insn_r->reg_rec_count = 1;
    }
  else if (2 == opcode)
    {
      /* STR(3).  */
      immed_8 = bits (thumb_insn_r->arm_insn, 0, 7);
      regcache_raw_read_unsigned (reg_cache, ARM_SP_REGNUM, &u_regval);
      record_buf_mem[0] = 4;
      record_buf_mem[1] = u_regval + (immed_8 * 4);
      thumb_insn_r->mem_rec_count = 1;
    }
  else if (0 == opcode)
    {
      /* STRH(1).  */
      immed_5 = bits (thumb_insn_r->arm_insn, 6, 10);
      reg_src1 = bits (thumb_insn_r->arm_insn, 3, 5);
      regcache_raw_read_unsigned (reg_cache, reg_src1, &u_regval);
      record_buf_mem[0] = 2;
      record_buf_mem[1] = u_regval + (immed_5 * 2);
      thumb_insn_r->mem_rec_count = 1;
    }

  REG_ALLOC (thumb_insn_r->arm_regs, thumb_insn_r->reg_rec_count, record_buf);
  MEM_ALLOC (thumb_insn_r->arm_mems, thumb_insn_r->mem_rec_count,
	     record_buf_mem);

  return 0;
}

/* Handling opcode 101 insns.  */

static int
thumb_record_misc (arm_insn_decode_record *thumb_insn_r)
{
  struct regcache *reg_cache = thumb_insn_r->regcache;

  uint32_t opcode = 0;
  uint32_t register_bits = 0, register_count = 0;
  uint32_t index = 0, start_address = 0;
  uint32_t record_buf[24], record_buf_mem[48];
  uint32_t reg_src1;

  ULONGEST u_regval = 0;

  opcode = bits (thumb_insn_r->arm_insn, 11, 12);

  if (opcode == 0 || opcode == 1)
    {
      /* ADR and ADD (SP plus immediate) */

      reg_src1 = bits (thumb_insn_r->arm_insn, 8, 10);
      record_buf[0] = reg_src1;
      thumb_insn_r->reg_rec_count = 1;
    }
  else
    {
      /* Miscellaneous 16-bit instructions */
      uint32_t opcode2 = bits (thumb_insn_r->arm_insn, 8, 11);

      switch (opcode2)
	{
	case 6:
	  /* SETEND and CPS */
	  break;
	case 0:
	  /* ADD/SUB (SP plus immediate)  */
	  reg_src1 = bits (thumb_insn_r->arm_insn, 8, 10);
	  record_buf[0] = ARM_SP_REGNUM;
	  thumb_insn_r->reg_rec_count = 1;
	  break;
	case 1: /* fall through  */
	case 3: /* fall through  */
	case 9: /* fall through  */
	case 11:
	  /* CBNZ, CBZ */
	  break;
	case 2:
	  /* SXTH, SXTB, UXTH, UXTB */
	  record_buf[0] = bits (thumb_insn_r->arm_insn, 0, 2);
	  thumb_insn_r->reg_rec_count = 1;
	  break;
	case 4: /* fall through  */
	case 5:
	  /* PUSH.  */
	  register_bits = bits (thumb_insn_r->arm_insn, 0, 7);
	  regcache_raw_read_unsigned (reg_cache, ARM_SP_REGNUM, &u_regval);
	  while (register_bits)
	    {
	      if (register_bits & 0x00000001)
		register_count++;
	      register_bits = register_bits >> 1;
	    }
	  start_address = u_regval -  \
	    (4 * (bit (thumb_insn_r->arm_insn, 8) + register_count));
	  thumb_insn_r->mem_rec_count = register_count;
	  while (register_count)
	    {
	      record_buf_mem[(register_count * 2) - 1] = start_address;
	      record_buf_mem[(register_count * 2) - 2] = 4;
	      start_address = start_address + 4;
	      register_count--;
	    }
	  record_buf[0] = ARM_SP_REGNUM;
	  thumb_insn_r->reg_rec_count = 1;
	  break;
	case 10:
	  /* REV, REV16, REVSH */
	  record_buf[0] = bits (thumb_insn_r->arm_insn, 0, 2);
	  thumb_insn_r->reg_rec_count = 1;
	  break;
	case 12: /* fall through  */
	case 13:
	  /* POP.  */
	  register_bits = bits (thumb_insn_r->arm_insn, 0, 7);
	  while (register_bits)
	    {
	      if (register_bits & 0x00000001)
		record_buf[index++] = register_count;
	      register_bits = register_bits >> 1;
	      register_count++;
	    }
	  record_buf[index++] = ARM_PS_REGNUM;
	  record_buf[index++] = ARM_SP_REGNUM;
	  thumb_insn_r->reg_rec_count = index;
	  break;
	case 0xe:
	  /* BKPT insn.  */
	  /* Handle enhanced software breakpoint insn, BKPT.  */
	  /* CPSR is changed to be executed in ARM state,  disabling normal
	     interrupts, entering abort mode.  */
	  /* According to high vector configuration PC is set.  */
	  /* User hits breakpoint and type reverse, in that case, we need to go back with 
	     previous CPSR and Program Counter.  */
	  record_buf[0] = ARM_PS_REGNUM;
	  record_buf[1] = ARM_LR_REGNUM;
	  thumb_insn_r->reg_rec_count = 2;
	  /* We need to save SPSR value, which is not yet done.  */
	  gdb_printf (gdb_stderr,
		      _("Process record does not support instruction "
			"0x%0x at address %s.\n"),
		      thumb_insn_r->arm_insn,
		      paddress (thumb_insn_r->gdbarch,
				thumb_insn_r->this_addr));
	  return -1;

	case 0xf:
	  /* If-Then, and hints */
	  break;
	default:
	  return -1;
	};
    }

  REG_ALLOC (thumb_insn_r->arm_regs, thumb_insn_r->reg_rec_count, record_buf);
  MEM_ALLOC (thumb_insn_r->arm_mems, thumb_insn_r->mem_rec_count,
	     record_buf_mem);

  return 0;
}

/* Handling opcode 110 insns.  */

static int
thumb_record_ldm_stm_swi (arm_insn_decode_record *thumb_insn_r)
{
  arm_gdbarch_tdep *tdep
    = gdbarch_tdep<arm_gdbarch_tdep>  (thumb_insn_r->gdbarch);
  struct regcache *reg_cache = thumb_insn_r->regcache;

  uint32_t ret = 0; /* function return value: -1:record failure ;  0:success  */
  uint32_t reg_src1 = 0;
  uint32_t opcode1 = 0, opcode2 = 0, register_bits = 0, register_count = 0;
  uint32_t index = 0, start_address = 0;
  uint32_t record_buf[24], record_buf_mem[48];

  ULONGEST u_regval = 0;

  opcode1 = bits (thumb_insn_r->arm_insn, 8, 12);
  opcode2 = bits (thumb_insn_r->arm_insn, 11, 12);

  if (1 == opcode2)
    {

      /* LDMIA.  */
      register_bits = bits (thumb_insn_r->arm_insn, 0, 7);
      /* Get Rn.  */
      reg_src1 = bits (thumb_insn_r->arm_insn, 8, 10);
      while (register_bits)
	{
	  if (register_bits & 0x00000001)
	    record_buf[index++] = register_count;
	  register_bits = register_bits >> 1;
	  register_count++;
	}
      record_buf[index++] = reg_src1;
      thumb_insn_r->reg_rec_count = index;
    }
  else if (0 == opcode2)
    {
      /* It handles both STMIA.  */
      register_bits = bits (thumb_insn_r->arm_insn, 0, 7);
      /* Get Rn.  */
      reg_src1 = bits (thumb_insn_r->arm_insn, 8, 10);
      regcache_raw_read_unsigned (reg_cache, reg_src1, &u_regval);
      while (register_bits)
	{
	  if (register_bits & 0x00000001)
	    register_count++;
	  register_bits = register_bits >> 1;
	}
      start_address = u_regval;
      thumb_insn_r->mem_rec_count = register_count;
      while (register_count)
	{
	  record_buf_mem[(register_count * 2) - 1] = start_address;
	  record_buf_mem[(register_count * 2) - 2] = 4;
	  start_address = start_address + 4;
	  register_count--;
	}
    }
  else if (0x1F == opcode1)
    {
	/* Handle arm syscall insn.  */
	if (tdep->arm_syscall_record != NULL)
	  {
	    regcache_raw_read_unsigned (reg_cache, 7, &u_regval);
	    ret = tdep->arm_syscall_record (reg_cache, u_regval);
	  }
	else
	  {
	    gdb_printf (gdb_stderr, _("no syscall record support\n"));
	    return -1;
	  }
    }

  /* B (1), conditional branch is automatically taken care in process_record,
    as PC is saved there.  */

  REG_ALLOC (thumb_insn_r->arm_regs, thumb_insn_r->reg_rec_count, record_buf);
  MEM_ALLOC (thumb_insn_r->arm_mems, thumb_insn_r->mem_rec_count,
	     record_buf_mem);

  return ret;
}

/* Handling opcode 111 insns.  */

static int
thumb_record_branch (arm_insn_decode_record *thumb_insn_r)
{
  uint32_t record_buf[8];
  uint32_t bits_h = 0;

  bits_h = bits (thumb_insn_r->arm_insn, 11, 12);

  if (2 == bits_h || 3 == bits_h)
    {
      /* BL */
      record_buf[0] = ARM_LR_REGNUM;
      thumb_insn_r->reg_rec_count = 1;
    }
  else if (1 == bits_h)
    {
      /* BLX(1). */
      record_buf[0] = ARM_PS_REGNUM;
      record_buf[1] = ARM_LR_REGNUM;
      thumb_insn_r->reg_rec_count = 2;
    }

  /* B(2) is automatically taken care in process_record, as PC is 
     saved there.  */

  REG_ALLOC (thumb_insn_r->arm_regs, thumb_insn_r->reg_rec_count, record_buf);

  return 0;     
}

/* Handler for thumb2 load/store multiple instructions.  */

static int
thumb2_record_ld_st_multiple (arm_insn_decode_record *thumb2_insn_r)
{
  struct regcache *reg_cache = thumb2_insn_r->regcache;

  uint32_t reg_rn, op;
  uint32_t register_bits = 0, register_count = 0;
  uint32_t index = 0, start_address = 0;
  uint32_t record_buf[24], record_buf_mem[48];

  ULONGEST u_regval = 0;

  reg_rn = bits (thumb2_insn_r->arm_insn, 16, 19);
  op = bits (thumb2_insn_r->arm_insn, 23, 24);

  if (0 == op || 3 == op)
    {
      if (bit (thumb2_insn_r->arm_insn, INSN_S_L_BIT_NUM))
	{
	  /* Handle RFE instruction.  */
	  record_buf[0] = ARM_PS_REGNUM;
	  thumb2_insn_r->reg_rec_count = 1;
	}
      else
	{
	  /* Handle SRS instruction after reading banked SP.  */
	  return arm_record_unsupported_insn (thumb2_insn_r);
	}
    }
  else if (1 == op || 2 == op)
    {
      if (bit (thumb2_insn_r->arm_insn, INSN_S_L_BIT_NUM))
	{
	  /* Handle LDM/LDMIA/LDMFD and LDMDB/LDMEA instructions.  */
	  register_bits = bits (thumb2_insn_r->arm_insn, 0, 15);
	  while (register_bits)
	    {
	      if (register_bits & 0x00000001)
		record_buf[index++] = register_count;

	      register_count++;
	      register_bits = register_bits >> 1;
	    }
	  record_buf[index++] = reg_rn;
	  record_buf[index++] = ARM_PS_REGNUM;
	  thumb2_insn_r->reg_rec_count = index;
	}
      else
	{
	  /* Handle STM/STMIA/STMEA and STMDB/STMFD.  */
	  register_bits = bits (thumb2_insn_r->arm_insn, 0, 15);
	  regcache_raw_read_unsigned (reg_cache, reg_rn, &u_regval);
	  while (register_bits)
	    {
	      if (register_bits & 0x00000001)
		register_count++;

	      register_bits = register_bits >> 1;
	    }

	  if (1 == op)
	    {
	      /* Start address calculation for LDMDB/LDMEA.  */
	      start_address = u_regval;
	    }
	  else if (2 == op)
	    {
	      /* Start address calculation for LDMDB/LDMEA.  */
	      start_address = u_regval - register_count * 4;
	    }

	  thumb2_insn_r->mem_rec_count = register_count;
	  while (register_count)
	    {
	      record_buf_mem[register_count * 2 - 1] = start_address;
	      record_buf_mem[register_count * 2 - 2] = 4;
	      start_address = start_address + 4;
	      register_count--;
	    }
	  record_buf[0] = reg_rn;
	  record_buf[1] = ARM_PS_REGNUM;
	  thumb2_insn_r->reg_rec_count = 2;
	}
    }

  MEM_ALLOC (thumb2_insn_r->arm_mems, thumb2_insn_r->mem_rec_count,
	    record_buf_mem);
  REG_ALLOC (thumb2_insn_r->arm_regs, thumb2_insn_r->reg_rec_count,
	    record_buf);
  return ARM_RECORD_SUCCESS;
}

/* Handler for thumb2 load/store (dual/exclusive) and table branch
   instructions.  */

static int
thumb2_record_ld_st_dual_ex_tbb (arm_insn_decode_record *thumb2_insn_r)
{
  struct regcache *reg_cache = thumb2_insn_r->regcache;

  uint32_t reg_rd, reg_rn, offset_imm;
  uint32_t reg_dest1, reg_dest2;
  uint32_t address, offset_addr;
  uint32_t record_buf[8], record_buf_mem[8];
  uint32_t op1, op2, op3;

  ULONGEST u_regval[2];

  op1 = bits (thumb2_insn_r->arm_insn, 23, 24);
  op2 = bits (thumb2_insn_r->arm_insn, 20, 21);
  op3 = bits (thumb2_insn_r->arm_insn, 4, 7);

  if (bit (thumb2_insn_r->arm_insn, INSN_S_L_BIT_NUM))
    {
      if(!(1 == op1 && 1 == op2 && (0 == op3 || 1 == op3)))
	{
	  reg_dest1 = bits (thumb2_insn_r->arm_insn, 12, 15);
	  record_buf[0] = reg_dest1;
	  record_buf[1] = ARM_PS_REGNUM;
	  thumb2_insn_r->reg_rec_count = 2;
	}

      if (3 == op2 || (op1 & 2) || (1 == op1 && 1 == op2 && 7 == op3))
	{
	  reg_dest2 = bits (thumb2_insn_r->arm_insn, 8, 11);
	  record_buf[2] = reg_dest2;
	  thumb2_insn_r->reg_rec_count = 3;
	}
    }
  else
    {
      reg_rn = bits (thumb2_insn_r->arm_insn, 16, 19);
      regcache_raw_read_unsigned (reg_cache, reg_rn, &u_regval[0]);

      if (0 == op1 && 0 == op2)
	{
	  /* Handle STREX.  */
	  offset_imm = bits (thumb2_insn_r->arm_insn, 0, 7);
	  address = u_regval[0] + (offset_imm * 4);
	  record_buf_mem[0] = 4;
	  record_buf_mem[1] = address;
	  thumb2_insn_r->mem_rec_count = 1;
	  reg_rd = bits (thumb2_insn_r->arm_insn, 0, 3);
	  record_buf[0] = reg_rd;
	  thumb2_insn_r->reg_rec_count = 1;
	}
      else if (1 == op1 && 0 == op2)
	{
	  reg_rd = bits (thumb2_insn_r->arm_insn, 0, 3);
	  record_buf[0] = reg_rd;
	  thumb2_insn_r->reg_rec_count = 1;
	  address = u_regval[0];
	  record_buf_mem[1] = address;

	  if (4 == op3)
	    {
	      /* Handle STREXB.  */
	      record_buf_mem[0] = 1;
	      thumb2_insn_r->mem_rec_count = 1;
	    }
	  else if (5 == op3)
	    {
	      /* Handle STREXH.  */
	      record_buf_mem[0] = 2 ;
	      thumb2_insn_r->mem_rec_count = 1;
	    }
	  else if (7 == op3)
	    {
	      /* Handle STREXD.  */
	      address = u_regval[0];
	      record_buf_mem[0] = 4;
	      record_buf_mem[2] = 4;
	      record_buf_mem[3] = address + 4;
	      thumb2_insn_r->mem_rec_count = 2;
	    }
	}
      else
	{
	  offset_imm = bits (thumb2_insn_r->arm_insn, 0, 7);

	  if (bit (thumb2_insn_r->arm_insn, 24))
	    {
	      if (bit (thumb2_insn_r->arm_insn, 23))
		offset_addr = u_regval[0] + (offset_imm * 4);
	      else
		offset_addr = u_regval[0] - (offset_imm * 4);

	      address = offset_addr;
	    }
	  else
	    address = u_regval[0];

	  record_buf_mem[0] = 4;
	  record_buf_mem[1] = address;
	  record_buf_mem[2] = 4;
	  record_buf_mem[3] = address + 4;
	  thumb2_insn_r->mem_rec_count = 2;
	  record_buf[0] = reg_rn;
	  thumb2_insn_r->reg_rec_count = 1;
	}
    }

  REG_ALLOC (thumb2_insn_r->arm_regs, thumb2_insn_r->reg_rec_count,
	    record_buf);
  MEM_ALLOC (thumb2_insn_r->arm_mems, thumb2_insn_r->mem_rec_count,
	    record_buf_mem);
  return ARM_RECORD_SUCCESS;
}

/* Handler for thumb2 data processing (shift register and modified immediate)
   instructions.  */

static int
thumb2_record_data_proc_sreg_mimm (arm_insn_decode_record *thumb2_insn_r)
{
  uint32_t reg_rd, op;
  uint32_t record_buf[8];

  op = bits (thumb2_insn_r->arm_insn, 21, 24);
  reg_rd = bits (thumb2_insn_r->arm_insn, 8, 11);

  if ((0 == op || 4 == op || 8 == op || 13 == op) && 15 == reg_rd)
    {
      record_buf[0] = ARM_PS_REGNUM;
      thumb2_insn_r->reg_rec_count = 1;
    }
  else
    {
      record_buf[0] = reg_rd;
      record_buf[1] = ARM_PS_REGNUM;
      thumb2_insn_r->reg_rec_count = 2;
    }

  REG_ALLOC (thumb2_insn_r->arm_regs, thumb2_insn_r->reg_rec_count,
	    record_buf);
  return ARM_RECORD_SUCCESS;
}

/* Generic handler for thumb2 instructions which effect destination and PS
   registers.  */

static int
thumb2_record_ps_dest_generic (arm_insn_decode_record *thumb2_insn_r)
{
  uint32_t reg_rd;
  uint32_t record_buf[8];

  reg_rd = bits (thumb2_insn_r->arm_insn, 8, 11);

  record_buf[0] = reg_rd;
  record_buf[1] = ARM_PS_REGNUM;
  thumb2_insn_r->reg_rec_count = 2;

  REG_ALLOC (thumb2_insn_r->arm_regs, thumb2_insn_r->reg_rec_count,
	    record_buf);
  return ARM_RECORD_SUCCESS;
}

/* Handler for thumb2 branch and miscellaneous control instructions.  */

static int
thumb2_record_branch_misc_cntrl (arm_insn_decode_record *thumb2_insn_r)
{
  uint32_t op, op1, op2;
  uint32_t record_buf[8];

  op = bits (thumb2_insn_r->arm_insn, 20, 26);
  op1 = bits (thumb2_insn_r->arm_insn, 12, 14);
  op2 = bits (thumb2_insn_r->arm_insn, 8, 11);

  /* Handle MSR insn.  */
  if (!(op1 & 0x2) && 0x38 == op)
    {
      if (!(op2 & 0x3))
	{
	  /* CPSR is going to be changed.  */
	  record_buf[0] = ARM_PS_REGNUM;
	  thumb2_insn_r->reg_rec_count = 1;
	}
      else
	{
	  arm_record_unsupported_insn(thumb2_insn_r);
	  return -1;
	}
    }
  else if (4 == (op1 & 0x5) || 5 == (op1 & 0x5))
    {
      /* BLX.  */
      record_buf[0] = ARM_PS_REGNUM;
      record_buf[1] = ARM_LR_REGNUM;
      thumb2_insn_r->reg_rec_count = 2;
    }

  REG_ALLOC (thumb2_insn_r->arm_regs, thumb2_insn_r->reg_rec_count,
	    record_buf);
  return ARM_RECORD_SUCCESS;
}

/* Handler for thumb2 store single data item instructions.  */

static int
thumb2_record_str_single_data (arm_insn_decode_record *thumb2_insn_r)
{
  struct regcache *reg_cache = thumb2_insn_r->regcache;

  uint32_t reg_rn, reg_rm, offset_imm, shift_imm;
  uint32_t address, offset_addr;
  uint32_t record_buf[8], record_buf_mem[8];
  uint32_t op1, op2;

  ULONGEST u_regval[2];

  op1 = bits (thumb2_insn_r->arm_insn, 21, 23);
  op2 = bits (thumb2_insn_r->arm_insn, 6, 11);
  reg_rn = bits (thumb2_insn_r->arm_insn, 16, 19);
  regcache_raw_read_unsigned (reg_cache, reg_rn, &u_regval[0]);

  if (bit (thumb2_insn_r->arm_insn, 23))
    {
      /* T2 encoding.  */
      offset_imm = bits (thumb2_insn_r->arm_insn, 0, 11);
      offset_addr = u_regval[0] + offset_imm;
      address = offset_addr;
    }
  else
    {
      /* T3 encoding.  */
      if ((0 == op1 || 1 == op1 || 2 == op1) && !(op2 & 0x20))
	{
	  /* Handle STRB (register).  */
	  reg_rm = bits (thumb2_insn_r->arm_insn, 0, 3);
	  regcache_raw_read_unsigned (reg_cache, reg_rm, &u_regval[1]);
	  shift_imm = bits (thumb2_insn_r->arm_insn, 4, 5);
	  offset_addr = u_regval[1] << shift_imm;
	  address = u_regval[0] + offset_addr;
	}
      else
	{
	  offset_imm = bits (thumb2_insn_r->arm_insn, 0, 7);
	  if (bit (thumb2_insn_r->arm_insn, 10))
	    {
	      if (bit (thumb2_insn_r->arm_insn, 9))
		offset_addr = u_regval[0] + offset_imm;
	      else
		offset_addr = u_regval[0] - offset_imm;

	      address = offset_addr;
	    }
	  else
	    address = u_regval[0];
	}
    }

  switch (op1)
    {
      /* Store byte instructions.  */
      case 4:
      case 0:
	record_buf_mem[0] = 1;
	break;
      /* Store half word instructions.  */
      case 1:
      case 5:
	record_buf_mem[0] = 2;
	break;
      /* Store word instructions.  */
      case 2:
      case 6:
	record_buf_mem[0] = 4;
	break;

      default:
	gdb_assert_not_reached ("no decoding pattern found");
	break;
    }

  record_buf_mem[1] = address;
  thumb2_insn_r->mem_rec_count = 1;
  record_buf[0] = reg_rn;
  thumb2_insn_r->reg_rec_count = 1;

  REG_ALLOC (thumb2_insn_r->arm_regs, thumb2_insn_r->reg_rec_count,
	    record_buf);
  MEM_ALLOC (thumb2_insn_r->arm_mems, thumb2_insn_r->mem_rec_count,
	    record_buf_mem);
  return ARM_RECORD_SUCCESS;
}

/* Handler for thumb2 load memory hints instructions.  */

static int
thumb2_record_ld_mem_hints (arm_insn_decode_record *thumb2_insn_r)
{
  uint32_t record_buf[8];
  uint32_t reg_rt, reg_rn;

  reg_rt = bits (thumb2_insn_r->arm_insn, 12, 15);
  reg_rn = bits (thumb2_insn_r->arm_insn, 16, 19);

  if (ARM_PC_REGNUM != reg_rt)
    {
      record_buf[0] = reg_rt;
      record_buf[1] = reg_rn;
      record_buf[2] = ARM_PS_REGNUM;
      thumb2_insn_r->reg_rec_count = 3;

      REG_ALLOC (thumb2_insn_r->arm_regs, thumb2_insn_r->reg_rec_count,
		record_buf);
      return ARM_RECORD_SUCCESS;
    }

  return ARM_RECORD_FAILURE;
}

/* Handler for thumb2 load word instructions.  */

static int
thumb2_record_ld_word (arm_insn_decode_record *thumb2_insn_r)
{
  uint32_t record_buf[8];

  record_buf[0] = bits (thumb2_insn_r->arm_insn, 12, 15);
  record_buf[1] = ARM_PS_REGNUM;
  thumb2_insn_r->reg_rec_count = 2;

  REG_ALLOC (thumb2_insn_r->arm_regs, thumb2_insn_r->reg_rec_count,
	    record_buf);
  return ARM_RECORD_SUCCESS;
}

/* Handler for thumb2 long multiply, long multiply accumulate, and
   divide instructions.  */

static int
thumb2_record_lmul_lmla_div (arm_insn_decode_record *thumb2_insn_r)
{
  uint32_t opcode1 = 0, opcode2 = 0;
  uint32_t record_buf[8];

  opcode1 = bits (thumb2_insn_r->arm_insn, 20, 22);
  opcode2 = bits (thumb2_insn_r->arm_insn, 4, 7);

  if (0 == opcode1 || 2 == opcode1 || (opcode1 >= 4 && opcode1 <= 6))
    {
      /* Handle SMULL, UMULL, SMULAL.  */
      /* Handle SMLAL(S), SMULL(S), UMLAL(S), UMULL(S).  */
      record_buf[0] = bits (thumb2_insn_r->arm_insn, 16, 19);
      record_buf[1] = bits (thumb2_insn_r->arm_insn, 12, 15);
      record_buf[2] = ARM_PS_REGNUM;
      thumb2_insn_r->reg_rec_count = 3;
    }
  else if (1 == opcode1 || 3 == opcode2)
    {
      /* Handle SDIV and UDIV.  */
      record_buf[0] = bits (thumb2_insn_r->arm_insn, 16, 19);
      record_buf[1] = bits (thumb2_insn_r->arm_insn, 12, 15);
      record_buf[2] = ARM_PS_REGNUM;
      thumb2_insn_r->reg_rec_count = 3;
    }
  else
    return ARM_RECORD_FAILURE;

  REG_ALLOC (thumb2_insn_r->arm_regs, thumb2_insn_r->reg_rec_count,
	    record_buf);
  return ARM_RECORD_SUCCESS;
}

/* Record handler for thumb32 coprocessor instructions.  */

static int
thumb2_record_coproc_insn (arm_insn_decode_record *thumb2_insn_r)
{
  if (bit (thumb2_insn_r->arm_insn, 25))
    return arm_record_coproc_data_proc (thumb2_insn_r);
  else
    return arm_record_asimd_vfp_coproc (thumb2_insn_r);
}

/* Record handler for advance SIMD structure load/store instructions.  */

static int
thumb2_record_asimd_struct_ld_st (arm_insn_decode_record *thumb2_insn_r)
{
  struct regcache *reg_cache = thumb2_insn_r->regcache;
  uint32_t l_bit, a_bit, b_bits;
  uint32_t record_buf[128], record_buf_mem[128];
  uint32_t reg_rn, reg_vd, address, f_elem;
  uint32_t index_r = 0, index_e = 0, bf_regs = 0, index_m = 0, loop_t = 0;
  uint8_t f_ebytes;

  l_bit = bit (thumb2_insn_r->arm_insn, 21);
  a_bit = bit (thumb2_insn_r->arm_insn, 23);
  b_bits = bits (thumb2_insn_r->arm_insn, 8, 11);
  reg_rn = bits (thumb2_insn_r->arm_insn, 16, 19);
  reg_vd = bits (thumb2_insn_r->arm_insn, 12, 15);
  reg_vd = (bit (thumb2_insn_r->arm_insn, 22) << 4) | reg_vd;
  f_ebytes = (1 << bits (thumb2_insn_r->arm_insn, 6, 7));
  f_elem = 8 / f_ebytes;

  if (!l_bit)
    {
      ULONGEST u_regval = 0;
      regcache_raw_read_unsigned (reg_cache, reg_rn, &u_regval);
      address = u_regval;

      if (!a_bit)
	{
	  /* Handle VST1.  */
	  if (b_bits == 0x02 || b_bits == 0x0a || (b_bits & 0x0e) == 0x06)
	    {
	      if (b_bits == 0x07)
		bf_regs = 1;
	      else if (b_bits == 0x0a)
		bf_regs = 2;
	      else if (b_bits == 0x06)
		bf_regs = 3;
	      else if (b_bits == 0x02)
		bf_regs = 4;
	      else
		bf_regs = 0;

	      for (index_r = 0; index_r < bf_regs; index_r++)
		{
		  for (index_e = 0; index_e < f_elem; index_e++)
		    {
		      record_buf_mem[index_m++] = f_ebytes;
		      record_buf_mem[index_m++] = address;
		      address = address + f_ebytes;
		      thumb2_insn_r->mem_rec_count += 1;
		    }
		}
	    }
	  /* Handle VST2.  */
	  else if (b_bits == 0x03 || (b_bits & 0x0e) == 0x08)
	    {
	      if (b_bits == 0x09 || b_bits == 0x08)
		bf_regs = 1;
	      else if (b_bits == 0x03)
		bf_regs = 2;
	      else
		bf_regs = 0;

	      for (index_r = 0; index_r < bf_regs; index_r++)
		for (index_e = 0; index_e < f_elem; index_e++)
		  {
		    for (loop_t = 0; loop_t < 2; loop_t++)
		      {
			record_buf_mem[index_m++] = f_ebytes;
			record_buf_mem[index_m++] = address + (loop_t * f_ebytes);
			thumb2_insn_r->mem_rec_count += 1;
		      }
		    address = address + (2 * f_ebytes);
		  }
	    }
	  /* Handle VST3.  */
	  else if ((b_bits & 0x0e) == 0x04)
	    {
	      for (index_e = 0; index_e < f_elem; index_e++)
		{
		  for (loop_t = 0; loop_t < 3; loop_t++)
		    {
		      record_buf_mem[index_m++] = f_ebytes;
		      record_buf_mem[index_m++] = address + (loop_t * f_ebytes);
		      thumb2_insn_r->mem_rec_count += 1;
		    }
		  address = address + (3 * f_ebytes);
		}
	    }
	  /* Handle VST4.  */
	  else if (!(b_bits & 0x0e))
	    {
	      for (index_e = 0; index_e < f_elem; index_e++)
		{
		  for (loop_t = 0; loop_t < 4; loop_t++)
		    {
		      record_buf_mem[index_m++] = f_ebytes;
		      record_buf_mem[index_m++] = address + (loop_t * f_ebytes);
		      thumb2_insn_r->mem_rec_count += 1;
		    }
		  address = address + (4 * f_ebytes);
		}
	    }
	}
      else
	{
	  uint8_t bft_size = bits (thumb2_insn_r->arm_insn, 10, 11);

	  if (bft_size == 0x00)
	    f_ebytes = 1;
	  else if (bft_size == 0x01)
	    f_ebytes = 2;
	  else if (bft_size == 0x02)
	    f_ebytes = 4;
	  else
	    f_ebytes = 0;

	  /* Handle VST1.  */
	  if (!(b_bits & 0x0b) || b_bits == 0x08)
	    thumb2_insn_r->mem_rec_count = 1;
	  /* Handle VST2.  */
	  else if ((b_bits & 0x0b) == 0x01 || b_bits == 0x09)
	    thumb2_insn_r->mem_rec_count = 2;
	  /* Handle VST3.  */
	  else if ((b_bits & 0x0b) == 0x02 || b_bits == 0x0a)
	    thumb2_insn_r->mem_rec_count = 3;
	  /* Handle VST4.  */
	  else if ((b_bits & 0x0b) == 0x03 || b_bits == 0x0b)
	    thumb2_insn_r->mem_rec_count = 4;

	  for (index_m = 0; index_m < thumb2_insn_r->mem_rec_count; index_m++)
	    {
	      record_buf_mem[index_m] = f_ebytes;
	      record_buf_mem[index_m] = address + (index_m * f_ebytes);
	    }
	}
    }
  else
    {
      if (!a_bit)
	{
	  /* Handle VLD1.  */
	  if (b_bits == 0x02 || b_bits == 0x0a || (b_bits & 0x0e) == 0x06)
	    thumb2_insn_r->reg_rec_count = 1;
	  /* Handle VLD2.  */
	  else if (b_bits == 0x03 || (b_bits & 0x0e) == 0x08)
	    thumb2_insn_r->reg_rec_count = 2;
	  /* Handle VLD3.  */
	  else if ((b_bits & 0x0e) == 0x04)
	    thumb2_insn_r->reg_rec_count = 3;
	  /* Handle VLD4.  */
	  else if (!(b_bits & 0x0e))
	    thumb2_insn_r->reg_rec_count = 4;
	}
      else
	{
	  /* Handle VLD1.  */
	  if (!(b_bits & 0x0b) || b_bits == 0x08 || b_bits == 0x0c)
	    thumb2_insn_r->reg_rec_count = 1;
	  /* Handle VLD2.  */
	  else if ((b_bits & 0x0b) == 0x01 || b_bits == 0x09 || b_bits == 0x0d)
	    thumb2_insn_r->reg_rec_count = 2;
	  /* Handle VLD3.  */
	  else if ((b_bits & 0x0b) == 0x02 || b_bits == 0x0a || b_bits == 0x0e)
	    thumb2_insn_r->reg_rec_count = 3;
	  /* Handle VLD4.  */
	  else if ((b_bits & 0x0b) == 0x03 || b_bits == 0x0b || b_bits == 0x0f)
	    thumb2_insn_r->reg_rec_count = 4;

	  for (index_r = 0; index_r < thumb2_insn_r->reg_rec_count; index_r++)
	    record_buf[index_r] = reg_vd + ARM_D0_REGNUM + index_r;
	}
    }

  if (bits (thumb2_insn_r->arm_insn, 0, 3) != 15)
    {
      record_buf[index_r] = reg_rn;
      thumb2_insn_r->reg_rec_count += 1;
    }

  REG_ALLOC (thumb2_insn_r->arm_regs, thumb2_insn_r->reg_rec_count,
	    record_buf);
  MEM_ALLOC (thumb2_insn_r->arm_mems, thumb2_insn_r->mem_rec_count,
	    record_buf_mem);
  return 0;
}

/* Decodes thumb2 instruction type and invokes its record handler.  */

static unsigned int
thumb2_record_decode_insn_handler (arm_insn_decode_record *thumb2_insn_r)
{
  uint32_t op, op1, op2;

  op = bit (thumb2_insn_r->arm_insn, 15);
  op1 = bits (thumb2_insn_r->arm_insn, 27, 28);
  op2 = bits (thumb2_insn_r->arm_insn, 20, 26);

  if (op1 == 0x01)
    {
      if (!(op2 & 0x64 ))
	{
	  /* Load/store multiple instruction.  */
	  return thumb2_record_ld_st_multiple (thumb2_insn_r);
	}
      else if ((op2 & 0x64) == 0x4)
	{
	  /* Load/store (dual/exclusive) and table branch instruction.  */
	  return thumb2_record_ld_st_dual_ex_tbb (thumb2_insn_r);
	}
      else if ((op2 & 0x60) == 0x20)
	{
	  /* Data-processing (shifted register).  */
	  return thumb2_record_data_proc_sreg_mimm (thumb2_insn_r);
	}
      else if (op2 & 0x40)
	{
	  /* Co-processor instructions.  */
	  return thumb2_record_coproc_insn (thumb2_insn_r);
	}
    }
  else if (op1 == 0x02)
    {
      if (op)
	{
	  /* Branches and miscellaneous control instructions.  */
	  return thumb2_record_branch_misc_cntrl (thumb2_insn_r);
	}
      else if (op2 & 0x20)
	{
	  /* Data-processing (plain binary immediate) instruction.  */
	  return thumb2_record_ps_dest_generic (thumb2_insn_r);
	}
      else
	{
	  /* Data-processing (modified immediate).  */
	  return thumb2_record_data_proc_sreg_mimm (thumb2_insn_r);
	}
    }
  else if (op1 == 0x03)
    {
      if (!(op2 & 0x71 ))
	{
	  /* Store single data item.  */
	  return thumb2_record_str_single_data (thumb2_insn_r);
	}
      else if (!((op2 & 0x71) ^ 0x10))
	{
	  /* Advanced SIMD or structure load/store instructions.  */
	  return thumb2_record_asimd_struct_ld_st (thumb2_insn_r);
	}
      else if (!((op2 & 0x67) ^ 0x01))
	{
	  /* Load byte, memory hints instruction.  */
	  return thumb2_record_ld_mem_hints (thumb2_insn_r);
	}
      else if (!((op2 & 0x67) ^ 0x03))
	{
	  /* Load halfword, memory hints instruction.  */
	  return thumb2_record_ld_mem_hints (thumb2_insn_r);
	}
      else if (!((op2 & 0x67) ^ 0x05))
	{
	  /* Load word instruction.  */
	  return thumb2_record_ld_word (thumb2_insn_r);
	}
      else if (!((op2 & 0x70) ^ 0x20))
	{
	  /* Data-processing (register) instruction.  */
	  return thumb2_record_ps_dest_generic (thumb2_insn_r);
	}
      else if (!((op2 & 0x78) ^ 0x30))
	{
	  /* Multiply, multiply accumulate, abs diff instruction.  */
	  return thumb2_record_ps_dest_generic (thumb2_insn_r);
	}
      else if (!((op2 & 0x78) ^ 0x38))
	{
	  /* Long multiply, long multiply accumulate, and divide.  */
	  return thumb2_record_lmul_lmla_div (thumb2_insn_r);
	}
      else if (op2 & 0x40)
	{
	  /* Co-processor instructions.  */
	  return thumb2_record_coproc_insn (thumb2_insn_r);
	}
   }

  return -1;
}

namespace {
/* Abstract instruction reader.  */

class abstract_instruction_reader
{
public:
  /* Read one instruction of size LEN from address MEMADDR and using
     BYTE_ORDER endianness.  */

  virtual ULONGEST read (CORE_ADDR memaddr, const size_t len,
			 enum bfd_endian byte_order) = 0;
};

/* Instruction reader from real target.  */

class instruction_reader : public abstract_instruction_reader
{
 public:
  ULONGEST read (CORE_ADDR memaddr, const size_t len,
		 enum bfd_endian byte_order) override
  {
    return read_code_unsigned_integer (memaddr, len, byte_order);
  }
};

} // namespace

typedef int (*sti_arm_hdl_fp_t) (arm_insn_decode_record*);

/* Decode arm/thumb insn depending on condition cods and opcodes; and
   dispatch it.  */

static int
decode_insn (abstract_instruction_reader &reader,
	     arm_insn_decode_record *arm_record,
	     record_type_t record_type, uint32_t insn_size)
{

  /* (Starting from numerical 0); bits 25, 26, 27 decodes type of arm
     instruction.  */
  static const sti_arm_hdl_fp_t arm_handle_insn[8] =
  {
    arm_record_data_proc_misc_ld_str,   /* 000.  */
    arm_record_data_proc_imm,           /* 001.  */
    arm_record_ld_st_imm_offset,        /* 010.  */
    arm_record_ld_st_reg_offset,        /* 011.  */
    arm_record_ld_st_multiple,          /* 100.  */
    arm_record_b_bl,                    /* 101.  */
    arm_record_asimd_vfp_coproc,        /* 110.  */
    arm_record_coproc_data_proc         /* 111.  */
  };

  /* (Starting from numerical 0); bits 13,14,15 decodes type of thumb
     instruction.  */
  static const sti_arm_hdl_fp_t thumb_handle_insn[8] =
  { \
    thumb_record_shift_add_sub,        /* 000.  */
    thumb_record_add_sub_cmp_mov,      /* 001.  */
    thumb_record_ld_st_reg_offset,     /* 010.  */
    thumb_record_ld_st_imm_offset,     /* 011.  */
    thumb_record_ld_st_stack,          /* 100.  */
    thumb_record_misc,                 /* 101.  */
    thumb_record_ldm_stm_swi,          /* 110.  */
    thumb_record_branch                /* 111.  */
  };

  uint32_t ret = 0;    /* return value: negative:failure   0:success.  */
  uint32_t insn_id = 0;
  enum bfd_endian code_endian
    = gdbarch_byte_order_for_code (arm_record->gdbarch);
  arm_record->arm_insn
    = reader.read (arm_record->this_addr, insn_size, code_endian);

  if (ARM_RECORD == record_type)
    {
      arm_record->cond = bits (arm_record->arm_insn, 28, 31);
      insn_id = bits (arm_record->arm_insn, 25, 27);

      if (arm_record->cond == 0xf)
	ret = arm_record_extension_space (arm_record);
      else
	{
	  /* If this insn has fallen into extension space
	     then we need not decode it anymore.  */
	  ret = arm_handle_insn[insn_id] (arm_record);
	}
      if (ret != ARM_RECORD_SUCCESS)
	{
	  arm_record_unsupported_insn (arm_record);
	  ret = -1;
	}
    }
  else if (THUMB_RECORD == record_type)
    {
      /* As thumb does not have condition codes, we set negative.  */
      arm_record->cond = -1;
      insn_id = bits (arm_record->arm_insn, 13, 15);
      ret = thumb_handle_insn[insn_id] (arm_record);
      if (ret != ARM_RECORD_SUCCESS)
	{
	  arm_record_unsupported_insn (arm_record);
	  ret = -1;
	}
    }
  else if (THUMB2_RECORD == record_type)
    {
      /* As thumb does not have condition codes, we set negative.  */
      arm_record->cond = -1;

      /* Swap first half of 32bit thumb instruction with second half.  */
      arm_record->arm_insn
	= (arm_record->arm_insn >> 16) | (arm_record->arm_insn << 16);

      ret = thumb2_record_decode_insn_handler (arm_record);

      if (ret != ARM_RECORD_SUCCESS)
	{
	  arm_record_unsupported_insn (arm_record);
	  ret = -1;
	}
    }
  else
    {
      /* Throw assertion.  */
      gdb_assert_not_reached ("not a valid instruction, could not decode");
    }

  return ret;
}

#if GDB_SELF_TEST
namespace selftests {

/* Instruction reader class for selftests.

   For 16-bit Thumb instructions, an array of uint16_t should be used.

   For 32-bit Thumb instructions and regular 32-bit Arm instructions, an array
   of uint32_t should be used.  */

template<typename T>
class instruction_reader_selftest : public abstract_instruction_reader
{
public:
  template<size_t SIZE>
  instruction_reader_selftest (const T (&insns)[SIZE])
    : m_insns (insns), m_insns_size (SIZE)
  {}

  ULONGEST read (CORE_ADDR memaddr, const size_t length,
		 enum bfd_endian byte_order) override
  {
    SELF_CHECK (length == sizeof (T));
    SELF_CHECK (memaddr % sizeof (T) == 0);
    SELF_CHECK ((memaddr / sizeof (T)) < m_insns_size);

    return m_insns[memaddr / sizeof (T)];
  }

private:
  const T *m_insns;
  const size_t m_insns_size;
};

static void
arm_record_test (void)
{
  struct gdbarch_info info;
  info.bfd_arch_info = bfd_scan_arch ("arm");

  struct gdbarch *gdbarch = gdbarch_find_by_info (info);

  SELF_CHECK (gdbarch != NULL);

  /* 16-bit Thumb instructions.  */
  {
    arm_insn_decode_record arm_record;

    memset (&arm_record, 0, sizeof (arm_insn_decode_record));
    arm_record.gdbarch = gdbarch;

    /* Use the endian-free representation of the instructions here.  The test
       will handle endianness conversions.  */
    static const uint16_t insns[] = {
      /* db b2	uxtb	r3, r3 */
      0xb2db,
      /* cd 58	ldr	r5, [r1, r3] */
      0x58cd,
    };

    instruction_reader_selftest<uint16_t> reader (insns);
    int ret = decode_insn (reader, &arm_record, THUMB_RECORD,
			   THUMB_INSN_SIZE_BYTES);

    SELF_CHECK (ret == 0);
    SELF_CHECK (arm_record.mem_rec_count == 0);
    SELF_CHECK (arm_record.reg_rec_count == 1);
    SELF_CHECK (arm_record.arm_regs[0] == 3);

    arm_record.this_addr += 2;
    ret = decode_insn (reader, &arm_record, THUMB_RECORD,
		       THUMB_INSN_SIZE_BYTES);

    SELF_CHECK (ret == 0);
    SELF_CHECK (arm_record.mem_rec_count == 0);
    SELF_CHECK (arm_record.reg_rec_count == 1);
    SELF_CHECK (arm_record.arm_regs[0] == 5);
  }

  /* 32-bit Thumb-2 instructions.  */
  {
    arm_insn_decode_record arm_record;

    memset (&arm_record, 0, sizeof (arm_insn_decode_record));
    arm_record.gdbarch = gdbarch;

    /* Use the endian-free representation of the instruction here.  The test
       will handle endianness conversions.  */
    static const uint32_t insns[] = {
      /* mrc	15, 0, r7, cr13, cr0, {3} */
      0x7f70ee1d,
    };

    instruction_reader_selftest<uint32_t> reader (insns);
    int ret = decode_insn (reader, &arm_record, THUMB2_RECORD,
			   THUMB2_INSN_SIZE_BYTES);

    SELF_CHECK (ret == 0);
    SELF_CHECK (arm_record.mem_rec_count == 0);
    SELF_CHECK (arm_record.reg_rec_count == 1);
    SELF_CHECK (arm_record.arm_regs[0] == 7);
  }

  /* 32-bit instructions.  */
  {
    arm_insn_decode_record arm_record;

    memset (&arm_record, 0, sizeof (arm_insn_decode_record));
    arm_record.gdbarch = gdbarch;

    /* Use the endian-free representation of the instruction here.  The test
       will handle endianness conversions.  */
    static const uint32_t insns[] = {
      /* mov     r5, r0 */
      0xe1a05000,
    };

    instruction_reader_selftest<uint32_t> reader (insns);
    int ret = decode_insn (reader, &arm_record, ARM_RECORD,
			   ARM_INSN_SIZE_BYTES);

    SELF_CHECK (ret == 0);
  }
}

/* Instruction reader from manually cooked instruction sequences.  */

class test_arm_instruction_reader : public arm_instruction_reader
{
public:
  explicit test_arm_instruction_reader (gdb::array_view<const uint32_t> insns)
    : m_insns (insns)
  {}

  uint32_t read (CORE_ADDR memaddr, enum bfd_endian byte_order) const override
  {
    SELF_CHECK (memaddr % 4 == 0);
    SELF_CHECK (memaddr / 4 < m_insns.size ());

    return m_insns[memaddr / 4];
  }

private:
  const gdb::array_view<const uint32_t> m_insns;
};

static void
arm_analyze_prologue_test ()
{
  for (bfd_endian endianness : {BFD_ENDIAN_LITTLE, BFD_ENDIAN_BIG})
    {
      struct gdbarch_info info;
      info.byte_order = endianness;
      info.byte_order_for_code = endianness;
      info.bfd_arch_info = bfd_scan_arch ("arm");

      struct gdbarch *gdbarch = gdbarch_find_by_info (info);

      SELF_CHECK (gdbarch != NULL);

      /* The "sub" instruction contains an immediate value rotate count of 0,
	 which resulted in a 32-bit shift of a 32-bit value, caught by
	 UBSan.  */
      const uint32_t insns[] = {
	  0xe92d4ff0, /* push    {r4, r5, r6, r7, r8, r9, sl, fp, lr} */
	  0xe1a05000, /* mov     r5, r0 */
	  0xe5903020, /* ldr     r3, [r0, #32] */
	  0xe24dd044, /* sub     sp, sp, #68     ; 0x44 */
      };

      test_arm_instruction_reader mem_reader (insns);
      arm_prologue_cache cache;
      arm_cache_init (&cache, gdbarch);

      arm_analyze_prologue (gdbarch, 0, sizeof (insns) - 1, &cache, mem_reader);
    }
}

} // namespace selftests
#endif /* GDB_SELF_TEST */

/* Cleans up local record registers and memory allocations.  */

static void 
deallocate_reg_mem (arm_insn_decode_record *record)
{
  xfree (record->arm_regs);
  xfree (record->arm_mems);    
}


/* Parse the current instruction and record the values of the registers and
   memory that will be changed in current instruction to record_arch_list".
   Return -1 if something is wrong.  */

int
arm_process_record (struct gdbarch *gdbarch, struct regcache *regcache,
		    CORE_ADDR insn_addr)
{

  uint32_t no_of_rec = 0;
  uint32_t ret = 0;  /* return value: -1:record failure ;  0:success  */
  ULONGEST t_bit = 0, insn_id = 0;

  ULONGEST u_regval = 0;

  arm_insn_decode_record arm_record;

  memset (&arm_record, 0, sizeof (arm_insn_decode_record));
  arm_record.regcache = regcache;
  arm_record.this_addr = insn_addr;
  arm_record.gdbarch = gdbarch;


  if (record_debug > 1)
    {
      gdb_printf (gdb_stdlog, "Process record: arm_process_record "
		  "addr = %s\n",
		  paddress (gdbarch, arm_record.this_addr));
    }

  instruction_reader reader;
  enum bfd_endian code_endian
    = gdbarch_byte_order_for_code (arm_record.gdbarch);
  arm_record.arm_insn
    = reader.read (arm_record.this_addr, 2, code_endian);

  /* Check the insn, whether it is thumb or arm one.  */

  t_bit = arm_psr_thumb_bit (arm_record.gdbarch);
  regcache_raw_read_unsigned (arm_record.regcache, ARM_PS_REGNUM, &u_regval);


  if (!(u_regval & t_bit))
    {
      /* We are decoding arm insn.  */
      ret = decode_insn (reader, &arm_record, ARM_RECORD, ARM_INSN_SIZE_BYTES);
    }
  else
    {
      insn_id = bits (arm_record.arm_insn, 11, 15);
      /* is it thumb2 insn?  */
      if ((0x1D == insn_id) || (0x1E == insn_id) || (0x1F == insn_id))
	{
	  ret = decode_insn (reader, &arm_record, THUMB2_RECORD,
			     THUMB2_INSN_SIZE_BYTES);
	}
      else
	{
	  /* We are decoding thumb insn.  */
	  ret = decode_insn (reader, &arm_record, THUMB_RECORD,
			     THUMB_INSN_SIZE_BYTES);
	}
    }

  if (0 == ret)
    {
      /* Record registers.  */
      record_full_arch_list_add_reg (arm_record.regcache, ARM_PC_REGNUM);
      if (arm_record.arm_regs)
	{
	  for (no_of_rec = 0; no_of_rec < arm_record.reg_rec_count; no_of_rec++)
	    {
	      if (record_full_arch_list_add_reg
		  (arm_record.regcache , arm_record.arm_regs[no_of_rec]))
		ret = -1;
	    }
	}
      /* Record memories.  */
      if (arm_record.arm_mems)
	{
	  for (no_of_rec = 0; no_of_rec < arm_record.mem_rec_count; no_of_rec++)
	    {
	      if (record_full_arch_list_add_mem
		  ((CORE_ADDR)arm_record.arm_mems[no_of_rec].addr,
		   arm_record.arm_mems[no_of_rec].len))
		ret = -1;
	    }
	}

      if (record_full_arch_list_add_end ())
	ret = -1;
    }


  deallocate_reg_mem (&arm_record);

  return ret;
}

/* See arm-tdep.h.  */

const target_desc *
arm_read_description (arm_fp_type fp_type, bool tls)
{
  struct target_desc *tdesc = tdesc_arm_list[fp_type][tls];

  if (tdesc == nullptr)
    {
      tdesc = arm_create_target_description (fp_type, tls);
      tdesc_arm_list[fp_type][tls] = tdesc;
    }

  return tdesc;
}

/* See arm-tdep.h.  */

const target_desc *
arm_read_mprofile_description (arm_m_profile_type m_type)
{
  struct target_desc *tdesc = tdesc_arm_mprofile_list[m_type];

  if (tdesc == nullptr)
    {
      tdesc = arm_create_mprofile_target_description (m_type);
      tdesc_arm_mprofile_list[m_type] = tdesc;
    }

  return tdesc;
}
