/* Target-dependent code for GNU/Linux running on the Fujitsu FR-V,
   for GDB.

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
#include "target.h"
#include "frame.h"
#include "osabi.h"
#include "regcache.h"
#include "elf-bfd.h"
#include "elf/frv.h"
#include "frv-tdep.h"
#include "trad-frame.h"
#include "frame-unwind.h"
#include "regset.h"
#include "linux-tdep.h"
#include "gdbarch.h"

/* Define the size (in bytes) of an FR-V instruction.  */
static const int frv_instr_size = 4;

enum {
  NORMAL_SIGTRAMP = 1,
  RT_SIGTRAMP = 2
};

static int
frv_linux_pc_in_sigtramp (struct gdbarch *gdbarch, CORE_ADDR pc,
			  const char *name)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  gdb_byte buf[frv_instr_size];
  LONGEST instr;
  int retval = 0;

  if (target_read_memory (pc, buf, sizeof buf) != 0)
    return 0;

  instr = extract_unsigned_integer (buf, sizeof buf, byte_order);

  if (instr == 0x8efc0077)	/* setlos #__NR_sigreturn, gr7 */
    retval = NORMAL_SIGTRAMP;
  else if (instr == 0x8efc00ad)	/* setlos #__NR_rt_sigreturn, gr7 */
    retval = RT_SIGTRAMP;
  else
    return 0;

  if (target_read_memory (pc + frv_instr_size, buf, sizeof buf) != 0)
    return 0;
  instr = extract_unsigned_integer (buf, sizeof buf, byte_order);
  if (instr != 0xc0700000)	/* tira	gr0, 0 */
    return 0;

  /* If we get this far, we'll return a non-zero value, either
     NORMAL_SIGTRAMP (1) or RT_SIGTRAMP (2).  */
  return retval;
}

/* Given NEXT_FRAME, the "callee" frame of the sigtramp frame that we
   wish to decode, and REGNO, one of the frv register numbers defined
   in frv-tdep.h, return the address of the saved register (corresponding
   to REGNO) in the sigtramp frame.  Return -1 if the register is not
   found in the sigtramp frame.  The magic numbers in the code below
   were computed by examining the following kernel structs:

   From arch/frv/kernel/signal.c:

      struct sigframe
      {
	      void (*pretcode)(void);
	      int sig;
	      struct sigcontext sc;
	      unsigned long extramask[_NSIG_WORDS-1];
	      uint32_t retcode[2];
      };

      struct rt_sigframe
      {
	      void (*pretcode)(void);
	      int sig;
	      struct siginfo *pinfo;
	      void *puc;
	      struct siginfo info;
	      struct ucontext uc;
	      uint32_t retcode[2];
      };

   From include/asm-frv/ucontext.h:

      struct ucontext {
	      unsigned long		uc_flags;
	      struct ucontext		*uc_link;
	      stack_t			uc_stack;
	      struct sigcontext	uc_mcontext;
	      sigset_t		uc_sigmask;
      };

   From include/asm-frv/signal.h:

      typedef struct sigaltstack {
	      void *ss_sp;
	      int ss_flags;
	      size_t ss_size;
      } stack_t;

   From include/asm-frv/sigcontext.h:

      struct sigcontext {
	      struct user_context	sc_context;
	      unsigned long		sc_oldmask;
      } __attribute__((aligned(8)));

   From include/asm-frv/registers.h:
      struct user_int_regs
      {
	      unsigned long		psr;
	      unsigned long		isr;
	      unsigned long		ccr;
	      unsigned long		cccr;
	      unsigned long		lr;
	      unsigned long		lcr;
	      unsigned long		pc;
	      unsigned long		__status;
	      unsigned long		syscallno;
	      unsigned long		orig_gr8;
	      unsigned long		gner[2];
	      unsigned long long	iacc[1];

	      union {
		      unsigned long	tbr;
		      unsigned long	gr[64];
	      };
      };

      struct user_fpmedia_regs
      {
	      unsigned long	fr[64];
	      unsigned long	fner[2];
	      unsigned long	msr[2];
	      unsigned long	acc[8];
	      unsigned char	accg[8];
	      unsigned long	fsr[1];
      };

      struct user_context
      {
	      struct user_int_regs		i;
	      struct user_fpmedia_regs	f;

	      void *extension;
      } __attribute__((aligned(8)));  */

static LONGEST
frv_linux_sigcontext_reg_addr (frame_info_ptr this_frame, int regno,
			       CORE_ADDR *sc_addr_cache_ptr)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR sc_addr;

  if (sc_addr_cache_ptr && *sc_addr_cache_ptr)
    {
      sc_addr = *sc_addr_cache_ptr;
    }
  else
    {
      CORE_ADDR pc, sp;
      gdb_byte buf[4];
      int tramp_type;

      pc = get_frame_pc (this_frame);
      tramp_type = frv_linux_pc_in_sigtramp (gdbarch, pc, 0);

      get_frame_register (this_frame, sp_regnum, buf);
      sp = extract_unsigned_integer (buf, sizeof buf, byte_order);

      if (tramp_type == NORMAL_SIGTRAMP)
	{
	  /* For a normal sigtramp frame, the sigcontext struct starts
	     at SP + 8.  */
	  sc_addr = sp + 8;
	}
      else if (tramp_type == RT_SIGTRAMP)
	{
	  /* For a realtime sigtramp frame, SP + 12 contains a pointer
	     to a ucontext struct.  The ucontext struct contains a
	     sigcontext struct starting 24 bytes in.  (The offset of
	     uc_mcontext within struct ucontext is derived as follows: 
	     stack_t is a 12-byte struct and struct sigcontext is
	     8-byte aligned.  This gives an offset of 8 + 12 + 4 (for
	     padding) = 24.)  */
	  if (target_read_memory (sp + 12, buf, sizeof buf) != 0)
	    {
	      warning (_("Can't read realtime sigtramp frame."));
	      return 0;
	    }
	  sc_addr = extract_unsigned_integer (buf, sizeof buf, byte_order);
	  sc_addr += 24;
	}
      else
	internal_error (_("not a signal trampoline"));

      if (sc_addr_cache_ptr)
	*sc_addr_cache_ptr = sc_addr;
    }

  switch (regno)
    {
    case psr_regnum :
      return sc_addr + 0;
    /* sc_addr + 4 has "isr", the Integer Status Register.  */
    case ccr_regnum :
      return sc_addr + 8;
    case cccr_regnum :
      return sc_addr + 12;
    case lr_regnum :
      return sc_addr + 16;
    case lcr_regnum :
      return sc_addr + 20;
    case pc_regnum :
      return sc_addr + 24;
    /* sc_addr + 28 is __status, the exception status.
       sc_addr + 32 is syscallno, the syscall number or -1.
       sc_addr + 36 is orig_gr8, the original syscall arg #1.
       sc_addr + 40 is gner[0].
       sc_addr + 44 is gner[1].  */
    case iacc0h_regnum :
      return sc_addr + 48;
    case iacc0l_regnum :
      return sc_addr + 52;
    default : 
      if (first_gpr_regnum <= regno && regno <= last_gpr_regnum)
	return sc_addr + 56 + 4 * (regno - first_gpr_regnum);
      else if (first_fpr_regnum <= regno && regno <= last_fpr_regnum)
	return sc_addr + 312 + 4 * (regno - first_fpr_regnum);
      else
	return -1;  /* not saved.  */
    }
}

/* Signal trampolines.  */

static struct trad_frame_cache *
frv_linux_sigtramp_frame_cache (frame_info_ptr this_frame,
				void **this_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  struct trad_frame_cache *cache;
  CORE_ADDR addr;
  gdb_byte buf[4];
  int regnum;
  CORE_ADDR sc_addr_cache_val = 0;
  struct frame_id this_id;

  if (*this_cache)
    return (struct trad_frame_cache *) *this_cache;

  cache = trad_frame_cache_zalloc (this_frame);

  /* FIXME: cagney/2004-05-01: This is is long standing broken code.
     The frame ID's code address should be the start-address of the
     signal trampoline and not the current PC within that
     trampoline.  */
  get_frame_register (this_frame, sp_regnum, buf);
  addr = extract_unsigned_integer (buf, sizeof buf, byte_order);
  this_id = frame_id_build (addr, get_frame_pc (this_frame));
  trad_frame_set_id (cache, this_id);

  for (regnum = 0; regnum < frv_num_regs; regnum++)
    {
      LONGEST reg_addr = frv_linux_sigcontext_reg_addr (this_frame, regnum,
							&sc_addr_cache_val);
      if (reg_addr != -1)
	trad_frame_set_reg_addr (cache, regnum, reg_addr);
    }

  *this_cache = cache;
  return cache;
}

static void
frv_linux_sigtramp_frame_this_id (frame_info_ptr this_frame,
				  void **this_cache,
				  struct frame_id *this_id)
{
  struct trad_frame_cache *cache
    = frv_linux_sigtramp_frame_cache (this_frame, this_cache);
  trad_frame_get_id (cache, this_id);
}

static struct value *
frv_linux_sigtramp_frame_prev_register (frame_info_ptr this_frame,
					void **this_cache, int regnum)
{
  /* Make sure we've initialized the cache.  */
  struct trad_frame_cache *cache
    = frv_linux_sigtramp_frame_cache (this_frame, this_cache);
  return trad_frame_get_register (cache, this_frame, regnum);
}

static int
frv_linux_sigtramp_frame_sniffer (const struct frame_unwind *self,
				  frame_info_ptr this_frame,
				  void **this_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  CORE_ADDR pc = get_frame_pc (this_frame);
  const char *name;

  find_pc_partial_function (pc, &name, NULL, NULL);
  if (frv_linux_pc_in_sigtramp (gdbarch, pc, name))
    return 1;

  return 0;
}

static const struct frame_unwind frv_linux_sigtramp_frame_unwind =
{
  "frv linux sigtramp",
  SIGTRAMP_FRAME,
  default_frame_unwind_stop_reason,
  frv_linux_sigtramp_frame_this_id,
  frv_linux_sigtramp_frame_prev_register,
  NULL,
  frv_linux_sigtramp_frame_sniffer
};

/* The FRV kernel defines ELF_NGREG as 46.  We add 2 in order to include
   the loadmap addresses in the register set.  (See below for more info.)  */
#define FRV_ELF_NGREG (46 + 2)
typedef unsigned char frv_elf_greg_t[4];
typedef struct { frv_elf_greg_t reg[FRV_ELF_NGREG]; } frv_elf_gregset_t;

typedef unsigned char frv_elf_fpreg_t[4];
typedef struct
{
  frv_elf_fpreg_t fr[64];
  frv_elf_fpreg_t fner[2];
  frv_elf_fpreg_t msr[2];
  frv_elf_fpreg_t acc[8];
  unsigned char accg[8];
  frv_elf_fpreg_t fsr[1];
} frv_elf_fpregset_t;

/* Register maps.  */

static const struct regcache_map_entry frv_linux_gregmap[] =
  {
    { 1, psr_regnum, 4 },
    { 1, REGCACHE_MAP_SKIP, 4 }, /* isr */
    { 1, ccr_regnum, 4 },
    { 1, cccr_regnum, 4 },
    { 1, lr_regnum, 4 },
    { 1, lcr_regnum, 4 },
    { 1, pc_regnum, 4 },
    { 1, REGCACHE_MAP_SKIP, 4 }, /* __status */
    { 1, REGCACHE_MAP_SKIP, 4 }, /* syscallno */
    { 1, REGCACHE_MAP_SKIP, 4 }, /* orig_gr8 */
    { 1, gner0_regnum, 4 },
    { 1, gner1_regnum, 4 },
    { 1, REGCACHE_MAP_SKIP, 8 }, /* iacc0 */
    { 1, tbr_regnum, 4 },
    { 31, first_gpr_regnum + 1, 4 }, /* gr1 ... gr31 */

    /* Technically, the loadmap addresses are not part of `pr_reg' as
       found in the elf_prstatus struct.  The fields which communicate
       the loadmap address appear (by design) immediately after
       `pr_reg' though, and the BFD function elf32_frv_grok_prstatus()
       has been implemented to include these fields in the register
       section that it extracts from the core file.  So, for our
       purposes, they may be viewed as registers.  */

    { 1, fdpic_loadmap_exec_regnum, 4 },
    { 1, fdpic_loadmap_interp_regnum, 4 },
    { 0 }
  };

static const struct regcache_map_entry frv_linux_fpregmap[] =
  {
    { 64, first_fpr_regnum, 4 }, /* fr0 ... fr63 */
    { 1, fner0_regnum, 4 },
    { 1, fner1_regnum, 4 },
    { 1, msr0_regnum, 4 },
    { 1, msr1_regnum, 4 },
    { 8, acc0_regnum, 4 },	/* acc0 ... acc7 */
    { 1, accg0123_regnum, 4 },
    { 1, accg4567_regnum, 4 },
    { 1, fsr0_regnum, 4 },
    { 0 }
  };

/* Unpack an frv_elf_gregset_t into GDB's register cache.  */

static void 
frv_linux_supply_gregset (const struct regset *regset,
			  struct regcache *regcache,
			  int regnum, const void *gregs, size_t len)
{
  int regi;

  /* gr0 always contains 0.  Also, the kernel passes the TBR value in
     this slot.  */
  regcache->raw_supply_zeroed (first_gpr_regnum);

  /* Fill gr32, ..., gr63 with zeros. */
  for (regi = first_gpr_regnum + 32; regi <= last_gpr_regnum; regi++)
    regcache->raw_supply_zeroed (regi);

  regcache_supply_regset (regset, regcache, regnum, gregs, len);
}

/* FRV Linux kernel register sets.  */

static const struct regset frv_linux_gregset =
{
  frv_linux_gregmap,
  frv_linux_supply_gregset, regcache_collect_regset
};

static const struct regset frv_linux_fpregset =
{
  frv_linux_fpregmap,
  regcache_supply_regset, regcache_collect_regset
};

static void
frv_linux_iterate_over_regset_sections (struct gdbarch *gdbarch,
					iterate_over_regset_sections_cb *cb,
					void *cb_data,
					const struct regcache *regcache)
{
  cb (".reg", sizeof (frv_elf_gregset_t), sizeof (frv_elf_gregset_t),
      &frv_linux_gregset, NULL, cb_data);
  cb (".reg2", sizeof (frv_elf_fpregset_t), sizeof (frv_elf_fpregset_t),
      &frv_linux_fpregset, NULL, cb_data);
}


static void
frv_linux_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  linux_init_abi (info, gdbarch, 0);

  /* Set the sigtramp frame sniffer.  */
  frame_unwind_append_unwinder (gdbarch, &frv_linux_sigtramp_frame_unwind); 

  set_gdbarch_iterate_over_regset_sections
    (gdbarch, frv_linux_iterate_over_regset_sections);
}

static enum gdb_osabi
frv_linux_elf_osabi_sniffer (bfd *abfd)
{
  int elf_flags;

  elf_flags = elf_elfheader (abfd)->e_flags;

  /* Assume GNU/Linux if using the FDPIC ABI.  If/when another OS shows
     up that uses this ABI, we'll need to start using .note sections
     or some such.  */
  if (elf_flags & EF_FRV_FDPIC)
    return GDB_OSABI_LINUX;
  else
    return GDB_OSABI_UNKNOWN;
}

void _initialize_frv_linux_tdep ();
void
_initialize_frv_linux_tdep ()
{
  gdbarch_register_osabi (bfd_arch_frv, 0, GDB_OSABI_LINUX,
			  frv_linux_init_abi);
  gdbarch_register_osabi_sniffer (bfd_arch_frv,
				  bfd_target_elf_flavour,
				  frv_linux_elf_osabi_sniffer);
}
