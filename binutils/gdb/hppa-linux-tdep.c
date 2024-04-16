/* Target-dependent code for GNU/Linux running on PA-RISC, for GDB.

   Copyright (C) 2004-2024 Free Software Foundation, Inc.

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
#include "gdbcore.h"
#include "osabi.h"
#include "target.h"
#include "objfiles.h"
#include "solib-svr4.h"
#include "glibc-tdep.h"
#include "frame-unwind.h"
#include "trad-frame.h"
#include "dwarf2/frame.h"
#include "value.h"
#include "regset.h"
#include "regcache.h"
#include "hppa-tdep.h"
#include "linux-tdep.h"
#include "elf/common.h"

/* Map DWARF DBX register numbers to GDB register numbers.  */
static int
hppa_dwarf_reg_to_regnum (struct gdbarch *gdbarch, int reg)
{
  /* The general registers and the sar are the same in both sets.  */
  if (reg >= 0 && reg <= 32)
    return reg;

  /* fr4-fr31 (left and right halves) are mapped from 72.  */
  if (reg >= 72 && reg <= 72 + 28 * 2)
    return HPPA_FP4_REGNUM + (reg - 72);

  return -1;
}

static void
hppa_linux_target_write_pc (struct regcache *regcache, CORE_ADDR v)
{
  /* Probably this should be done by the kernel, but it isn't.  */
  regcache_cooked_write_unsigned (regcache, HPPA_PCOQ_HEAD_REGNUM, v | 0x3);
  regcache_cooked_write_unsigned (regcache,
				  HPPA_PCOQ_TAIL_REGNUM, (v + 4) | 0x3);
}

/* An instruction to match.  */
struct insn_pattern
{
  unsigned int data;            /* See if it matches this....  */
  unsigned int mask;            /* ... with this mask.  */
};

static struct insn_pattern hppa_sigtramp[] = {
  /* ldi 0, %r25 or ldi 1, %r25 */
  { 0x34190000, 0xfffffffd },
  /* ldi __NR_rt_sigreturn, %r20 */
  { 0x3414015a, 0xffffffff },
  /* be,l 0x100(%sr2, %r0), %sr0, %r31 */
  { 0xe4008200, 0xffffffff },
  /* nop */
  { 0x08000240, 0xffffffff },
  { 0, 0 }
};

#define HPPA_MAX_INSN_PATTERN_LEN (4)

/* Return non-zero if the instructions at PC match the series
   described in PATTERN, or zero otherwise.  PATTERN is an array of
   'struct insn_pattern' objects, terminated by an entry whose mask is
   zero.

   When the match is successful, fill INSN[i] with what PATTERN[i]
   matched.  */
static int
insns_match_pattern (struct gdbarch *gdbarch, CORE_ADDR pc,
		     struct insn_pattern *pattern,
		     unsigned int *insn)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int i;
  CORE_ADDR npc = pc;

  for (i = 0; pattern[i].mask; i++)
    {
      gdb_byte buf[4];

      target_read_memory (npc, buf, 4);
      insn[i] = extract_unsigned_integer (buf, 4, byte_order);
      if ((insn[i] & pattern[i].mask) == pattern[i].data)
	npc += 4;
      else
	return 0;
    }
  return 1;
}

/* Signal frames.  */

/* (This is derived from MD_FALLBACK_FRAME_STATE_FOR in gcc.)
 
   Unfortunately, because of various bugs and changes to the kernel,
   we have several cases to deal with.

   In 2.4, the signal trampoline is 4 bytes, and pc should point directly at 
   the beginning of the trampoline and struct rt_sigframe.

   In <= 2.6.5-rc2-pa3, the signal trampoline is 9 bytes, and pc points at
   the 4th word in the trampoline structure.  This is wrong, it should point 
   at the 5th word.  This is fixed in 2.6.5-rc2-pa4.

   To detect these cases, we first take pc, align it to 64-bytes
   to get the beginning of the signal frame, and then check offsets 0, 4
   and 5 to see if we found the beginning of the trampoline.  This will
   tell us how to locate the sigcontext structure.

   Note that with a 2.4 64-bit kernel, the signal context is not properly
   passed back to userspace so the unwind will not work correctly.  */
static CORE_ADDR
hppa_linux_sigtramp_find_sigcontext (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  unsigned int dummy[HPPA_MAX_INSN_PATTERN_LEN];
  int offs = 0;
  int attempt;
  /* offsets to try to find the trampoline */
  static int pcoffs[] = { 0, 4*4, 5*4 };
  /* offsets to the rt_sigframe structure */
  static int sfoffs[] = { 4*4, 10*4, 10*4 };
  CORE_ADDR sp;

  /* Most of the time, this will be correct.  The one case when this will
     fail is if the user defined an alternate stack, in which case the
     beginning of the stack will not be align_down (pc, 64).  */
  sp = align_down (pc, 64);

  /* rt_sigreturn trampoline:
     3419000x ldi 0, %r25 or ldi 1, %r25   (x = 0 or 2)
     3414015a ldi __NR_rt_sigreturn, %r20 
     e4008200 be,l 0x100(%sr2, %r0), %sr0, %r31
     08000240 nop  */

  for (attempt = 0; attempt < ARRAY_SIZE (pcoffs); attempt++)
    {
      if (insns_match_pattern (gdbarch, sp + pcoffs[attempt],
			       hppa_sigtramp, dummy))
	{
	  offs = sfoffs[attempt];
	  break;
	}
    }

  if (offs == 0)
    {
      if (insns_match_pattern (gdbarch, pc, hppa_sigtramp, dummy))
	{
	  /* sigaltstack case: we have no way of knowing which offset to 
	     use in this case; default to new kernel handling.  If this is
	     wrong the unwinding will fail.  */
	  attempt = 2;
	  sp = pc - pcoffs[attempt];
	}
      else
	return 0;
    }

  /* sp + sfoffs[try] points to a struct rt_sigframe, which contains
     a struct siginfo and a struct ucontext.  struct ucontext contains
     a struct sigcontext.  Return an offset to this sigcontext here.  Too 
     bad we cannot include system specific headers :-(.
     sizeof(struct siginfo) == 128
     offsetof(struct ucontext, uc_mcontext) == 24.  */
  return sp + sfoffs[attempt] + 128 + 24;
}

struct hppa_linux_sigtramp_unwind_cache
{
  CORE_ADDR base;
  trad_frame_saved_reg *saved_regs;
};

static struct hppa_linux_sigtramp_unwind_cache *
hppa_linux_sigtramp_frame_unwind_cache (frame_info_ptr this_frame,
					void **this_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct hppa_linux_sigtramp_unwind_cache *info;
  CORE_ADDR pc, scptr;
  int i;

  if (*this_cache)
    return (struct hppa_linux_sigtramp_unwind_cache *) *this_cache;

  info = FRAME_OBSTACK_ZALLOC (struct hppa_linux_sigtramp_unwind_cache);
  *this_cache = info;
  info->saved_regs = trad_frame_alloc_saved_regs (this_frame);

  pc = get_frame_pc (this_frame);
  scptr = hppa_linux_sigtramp_find_sigcontext (gdbarch, pc);

  /* structure of struct sigcontext:
   
     struct sigcontext {
	unsigned long sc_flags;
	unsigned long sc_gr[32]; 
	unsigned long long sc_fr[32];
	unsigned long sc_iasq[2];
	unsigned long sc_iaoq[2];
	unsigned long sc_sar;           */

  /* Skip sc_flags.  */
  scptr += 4;

  /* GR[0] is the psw.  */
  info->saved_regs[HPPA_IPSW_REGNUM].set_addr (scptr);
  scptr += 4;

  /* General registers.  */
  for (i = 1; i < 32; i++)
    {
      info->saved_regs[HPPA_R0_REGNUM + i].set_addr (scptr);
      scptr += 4;
    }

  /* Pad to long long boundary.  */
  scptr += 4;

  /* FP regs; FP0-3 are not restored.  */
  scptr += (8 * 4);

  for (i = 4; i < 32; i++)
    {
      info->saved_regs[HPPA_FP0_REGNUM + (i * 2)].set_addr (scptr);
      scptr += 4;
      info->saved_regs[HPPA_FP0_REGNUM + (i * 2) + 1].set_addr (scptr);
      scptr += 4;
    }

  /* IASQ/IAOQ.  */
  info->saved_regs[HPPA_PCSQ_HEAD_REGNUM].set_addr (scptr);
  scptr += 4;
  info->saved_regs[HPPA_PCSQ_TAIL_REGNUM].set_addr (scptr);
  scptr += 4;

  info->saved_regs[HPPA_PCOQ_HEAD_REGNUM].set_addr (scptr);
  scptr += 4;
  info->saved_regs[HPPA_PCOQ_TAIL_REGNUM].set_addr (scptr);
  scptr += 4;

  info->saved_regs[HPPA_SAR_REGNUM].set_addr (scptr);

  info->base = get_frame_register_unsigned (this_frame, HPPA_SP_REGNUM);

  return info;
}

static void
hppa_linux_sigtramp_frame_this_id (frame_info_ptr this_frame,
				   void **this_prologue_cache,
				   struct frame_id *this_id)
{
  struct hppa_linux_sigtramp_unwind_cache *info
    = hppa_linux_sigtramp_frame_unwind_cache (this_frame, this_prologue_cache);
  *this_id = frame_id_build (info->base, get_frame_pc (this_frame));
}

static struct value *
hppa_linux_sigtramp_frame_prev_register (frame_info_ptr this_frame,
					 void **this_prologue_cache,
					 int regnum)
{
  struct hppa_linux_sigtramp_unwind_cache *info
    = hppa_linux_sigtramp_frame_unwind_cache (this_frame, this_prologue_cache);
  return hppa_frame_prev_register_helper (this_frame,
					  info->saved_regs, regnum);
}

/* hppa-linux always uses "new-style" rt-signals.  The signal handler's return
   address should point to a signal trampoline on the stack.  The signal
   trampoline is embedded in a rt_sigframe structure that is aligned on
   the stack.  We take advantage of the fact that sp must be 64-byte aligned,
   and the trampoline is small, so by rounding down the trampoline address
   we can find the beginning of the struct rt_sigframe.  */
static int
hppa_linux_sigtramp_frame_sniffer (const struct frame_unwind *self,
				   frame_info_ptr this_frame,
				   void **this_prologue_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  CORE_ADDR pc = get_frame_pc (this_frame);

  if (hppa_linux_sigtramp_find_sigcontext (gdbarch, pc))
    return 1;

  return 0;
}

static const struct frame_unwind hppa_linux_sigtramp_frame_unwind = {
  "hppa linux sigtramp",
  SIGTRAMP_FRAME,
  default_frame_unwind_stop_reason,
  hppa_linux_sigtramp_frame_this_id,
  hppa_linux_sigtramp_frame_prev_register,
  NULL,
  hppa_linux_sigtramp_frame_sniffer
};

/* Attempt to find (and return) the global pointer for the given
   function.

   This is a rather nasty bit of code searchs for the .dynamic section
   in the objfile corresponding to the pc of the function we're trying
   to call.  Once it finds the addresses at which the .dynamic section
   lives in the child process, it scans the Elf32_Dyn entries for a
   DT_PLTGOT tag.  If it finds one of these, the corresponding
   d_un.d_ptr value is the global pointer.  */

static CORE_ADDR
hppa_linux_find_global_pointer (struct gdbarch *gdbarch,
				struct value *function)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  struct obj_section *faddr_sect;
  CORE_ADDR faddr;
  
  faddr = value_as_address (function);

  /* Is this a plabel? If so, dereference it to get the gp value.  */
  if (faddr & 2)
    {
      int status;
      gdb_byte buf[4];

      faddr &= ~3;

      status = target_read_memory (faddr + 4, buf, sizeof (buf));
      if (status == 0)
	return extract_unsigned_integer (buf, sizeof (buf), byte_order);
    }

  /* If the address is in the plt section, then the real function hasn't 
     yet been fixed up by the linker so we cannot determine the gp of 
     that function.  */
  if (in_plt_section (faddr))
    return 0;

  faddr_sect = find_pc_section (faddr);
  if (faddr_sect != NULL)
    {
      for (obj_section *osect : faddr_sect->objfile->sections ())
	{
	  if (strcmp (osect->the_bfd_section->name, ".dynamic") == 0)
	    {
	      CORE_ADDR addr, endaddr;

	      addr = osect->addr ();
	      endaddr = osect->endaddr ();

	      while (addr < endaddr)
		{
		  int status;
		  LONGEST tag;
		  gdb_byte buf[4];

		  status = target_read_memory (addr, buf, sizeof (buf));
		  if (status != 0)
		    break;
		  tag = extract_signed_integer (buf, byte_order);

		  if (tag == DT_PLTGOT)
		    {
		      CORE_ADDR global_pointer;

		      status = target_read_memory (addr + 4, buf,
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

		  addr += 8;
		}
	      break;
	    }
	}
    }
  return 0;
}

/*
 * Registers saved in a coredump:
 * gr0..gr31
 * sr0..sr7
 * iaoq0..iaoq1
 * iasq0..iasq1
 * sar, iir, isr, ior, ipsw
 * cr0, cr24..cr31
 * cr8,9,12,13
 * cr10, cr15
 */

static const struct regcache_map_entry hppa_linux_gregmap[] =
  {
    { 32, HPPA_R0_REGNUM },
    { 1, HPPA_SR4_REGNUM+1 },
    { 1, HPPA_SR4_REGNUM+2 },
    { 1, HPPA_SR4_REGNUM+3 },
    { 1, HPPA_SR4_REGNUM+4 },
    { 1, HPPA_SR4_REGNUM },
    { 1, HPPA_SR4_REGNUM+5 },
    { 1, HPPA_SR4_REGNUM+6 },
    { 1, HPPA_SR4_REGNUM+7 },
    { 1, HPPA_PCOQ_HEAD_REGNUM },
    { 1, HPPA_PCOQ_TAIL_REGNUM },
    { 1, HPPA_PCSQ_HEAD_REGNUM },
    { 1, HPPA_PCSQ_TAIL_REGNUM },
    { 1, HPPA_SAR_REGNUM },
    { 1, HPPA_IIR_REGNUM },
    { 1, HPPA_ISR_REGNUM },
    { 1, HPPA_IOR_REGNUM },
    { 1, HPPA_IPSW_REGNUM },
    { 1, HPPA_RCR_REGNUM },
    { 8, HPPA_TR0_REGNUM },
    { 4, HPPA_PID0_REGNUM },
    { 1, HPPA_CCR_REGNUM },
    { 1, HPPA_EIEM_REGNUM },
    { 0 }
  };

static const struct regcache_map_entry hppa_linux_fpregmap[] =
  {
    /* FIXME: Only works for 32-bit mode.  In 64-bit mode there should
       be 32 fpregs, 8 bytes each.  */
    { 64, HPPA_FP0_REGNUM, 4 },
    { 0 }
  };

/* HPPA Linux kernel register set.  */
static const struct regset hppa_linux_regset =
{
  hppa_linux_gregmap,
  regcache_supply_regset, regcache_collect_regset
};

static const struct regset hppa_linux_fpregset =
{
  hppa_linux_fpregmap,
  regcache_supply_regset, regcache_collect_regset
};

static void
hppa_linux_iterate_over_regset_sections (struct gdbarch *gdbarch,
					 iterate_over_regset_sections_cb *cb,
					 void *cb_data,
					 const struct regcache *regcache)
{
  hppa_gdbarch_tdep *tdep = gdbarch_tdep<hppa_gdbarch_tdep> (gdbarch);

  cb (".reg", 80 * tdep->bytes_per_address, 80 * tdep->bytes_per_address,
      &hppa_linux_regset, NULL, cb_data);
  cb (".reg2", 64 * 4, 64 * 4, &hppa_linux_fpregset, NULL, cb_data);
}

static void
hppa_linux_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  hppa_gdbarch_tdep *tdep = gdbarch_tdep<hppa_gdbarch_tdep> (gdbarch);

  linux_init_abi (info, gdbarch, 0);

  /* GNU/Linux is always ELF.  */
  tdep->is_elf = 1;

  tdep->find_global_pointer = hppa_linux_find_global_pointer;

  set_gdbarch_write_pc (gdbarch, hppa_linux_target_write_pc);

  frame_unwind_append_unwinder (gdbarch, &hppa_linux_sigtramp_frame_unwind);

  /* GNU/Linux uses SVR4-style shared libraries.  */
  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, linux_ilp32_fetch_link_map_offsets);

  tdep->in_solib_call_trampoline = hppa_in_solib_call_trampoline;
  set_gdbarch_skip_trampoline_code (gdbarch, hppa_skip_trampoline_code);

  /* GNU/Linux uses the dynamic linker included in the GNU C Library.  */
  set_gdbarch_skip_solib_resolver (gdbarch, glibc_skip_solib_resolver);

  /* On hppa-linux, currently, sizeof(long double) == 8.  There has been
     some discussions to support 128-bit long double, but it requires some
     more work in gcc and glibc first.  */
  set_gdbarch_long_double_bit (gdbarch, 64);
  set_gdbarch_long_double_format (gdbarch, floatformats_ieee_double);

  set_gdbarch_iterate_over_regset_sections
    (gdbarch, hppa_linux_iterate_over_regset_sections);

  set_gdbarch_dwarf2_reg_to_regnum (gdbarch, hppa_dwarf_reg_to_regnum);

  /* Enable TLS support.  */
  set_gdbarch_fetch_tls_load_module_address (gdbarch,
					     svr4_fetch_objfile_link_map);
}

void _initialize_hppa_linux_tdep ();
void
_initialize_hppa_linux_tdep ()
{
  gdbarch_register_osabi (bfd_arch_hppa, 0, GDB_OSABI_LINUX,
			  hppa_linux_init_abi);
  gdbarch_register_osabi (bfd_arch_hppa, bfd_mach_hppa20w,
			  GDB_OSABI_LINUX, hppa_linux_init_abi);
}
