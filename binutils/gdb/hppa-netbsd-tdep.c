/* Target-dependent code for NetBSD/hppa

   Copyright (C) 2008-2024 Free Software Foundation, Inc.

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
#include "osabi.h"
#include "regcache.h"
#include "regset.h"

#include "trad-frame.h"
#include "tramp-frame.h"

#include "hppa-tdep.h"
#include "hppa-bsd-tdep.h"
#include "netbsd-tdep.h"
#include "gdbarch.h"

/* From <machine/mcontext.h>.  */
static int hppanbsd_mc_reg_offset[] =
{
  /* r0 ... r31 */
      -1,   1 * 4,   2 * 4,   3 * 4,
   4 * 4,   5 * 4,   6 * 4,   7 * 4,
   8 * 4,   9 * 4,  10 * 4,  11 * 4, 
  12 * 4,  13 * 4,  14 * 4,  15 * 4,
  16 * 4,  17 * 4,  18 * 4,  19 * 4,
  20 * 4,  21 * 4,  22 * 4,  23 * 4,
  24 * 4,  25 * 4,  26 * 4,  27 * 4,
  28 * 4,  29 * 4,  30 * 4,  31 * 4,

  32 * 4,	/* HPPA_SAR_REGNUM */
  35 * 4,	/* HPPA_PCOQ_HEAD_REGNUM */
  33 * 4,	/* HPPA_PCSQ_HEAD_REGNUM */
  36 * 4,	/* HPPA_PCOQ_TAIL_REGNUM */
  34 * 4,	/* HPPA_PCSQ_TAIL_REGNUM */
  -1,		/* HPPA_EIEM_REGNUM */
  -1,		/* HPPA_IIR_REGNUM */
  -1,		/* HPPA_ISR_REGNUM */
  -1,		/* HPPA_IOR_REGNUM */
  0 * 4,	/* HPPA_IPSW_REGNUM */
  -1,		/* spare? */
  41 * 4,	/* HPPA_SR4_REGNUM */
  37 * 4,	/* sr0 */
  38 * 4,	/* sr1 */
  39 * 4,	/* sr2 */
  40 * 4,	/* sr3 */

  /* more tbd */
};

static void hppanbsd_sigtramp_cache_init (const struct tramp_frame *,
					 frame_info_ptr,
					 struct trad_frame_cache *,
					 CORE_ADDR);

static const struct tramp_frame hppanbsd_sigtramp_si4 =
{
  SIGTRAMP_FRAME,
  4,
  {
    { 0xc7d7c012, ULONGEST_MAX },	/*	bb,>=,n %arg3, 30, 1f		*/
    { 0xd6e01c1e, ULONGEST_MAX },	/*	 depwi 0,31,2,%arg3		*/
    { 0x0ee81093, ULONGEST_MAX },	/*	ldw 4(%arg3), %r19		*/
    { 0x0ee01097, ULONGEST_MAX },	/*	ldw 0(%arg3), %arg3		*/
			/* 1: 					*/
    { 0xe8404000, ULONGEST_MAX },	/* 	blr %r0, %rp			*/
    { 0xeae0c002, ULONGEST_MAX },	/*	bv,n %r0(%arg3)			*/
    { 0x08000240, ULONGEST_MAX },	/*	 nop				*/

    { 0x0803025a, ULONGEST_MAX },	/*	copy %r3, %arg0			*/
    { 0x20200801, ULONGEST_MAX },	/*	ldil -40000000, %r1		*/
    { 0xe420e008, ULONGEST_MAX },	/*	be,l 4(%sr7, %r1), %sr0, %r31	*/
    { 0x34160268, ULONGEST_MAX },	/*	 ldi 134, %t1 ; SYS_setcontext	*/

    { 0x081c025a, ULONGEST_MAX },	/*	copy ret0, %arg0		*/
    { 0x20200801, ULONGEST_MAX },	/*	ldil -40000000, %r1		*/
    { 0xe420e008, ULONGEST_MAX },	/*	be,l 4(%sr7, %r1), %sr0, %r31	*/
    { 0x34160002, ULONGEST_MAX },	/*	 ldi 1, %t1 ; SYS_exit		*/
    { TRAMP_SENTINEL_INSN, ULONGEST_MAX }
  },
  hppanbsd_sigtramp_cache_init
};


static void
hppanbsd_sigtramp_cache_init (const struct tramp_frame *self,
			     frame_info_ptr this_frame,
			     struct trad_frame_cache *this_cache,
			     CORE_ADDR func)
{
  CORE_ADDR sp = get_frame_register_unsigned (this_frame, HPPA_SP_REGNUM);
  CORE_ADDR base;
  int *reg_offset;
  int num_regs;
  int i;

  reg_offset = hppanbsd_mc_reg_offset;
  num_regs = ARRAY_SIZE (hppanbsd_mc_reg_offset);

  /* frame pointer */
  base = sp - 0x280;
  /* offsetof(struct sigframe_siginfo, sf_uc) = 128 */
  base += 128;
  /* offsetof(ucontext_t, uc_mcontext) == 40 */
  base += 40;

  for (i = 0; i < num_regs; i++)
    if (reg_offset[i] != -1)
      trad_frame_set_reg_addr (this_cache, i, base + reg_offset[i]);

  /* Construct the frame ID using the function start.  */
  trad_frame_set_id (this_cache, frame_id_build (sp, func));
}

/* Core file support.  */

/* Sizeof `struct reg' in <machine/reg.h>.  */
#define HPPANBSD_SIZEOF_GREGS	(44 * 4)

static int hppanbsd_reg_offset[] =
{
  /* r0 ... r31 */
      -1,   1 * 4,   2 * 4,   3 * 4,
   4 * 4,   5 * 4,   6 * 4,   7 * 4,
   8 * 4,   9 * 4,  10 * 4,  11 * 4, 
  12 * 4,  13 * 4,  14 * 4,  15 * 4,
  16 * 4,  17 * 4,  18 * 4,  19 * 4,
  20 * 4,  21 * 4,  22 * 4,  23 * 4,
  24 * 4,  25 * 4,  26 * 4,  27 * 4,
  28 * 4,  29 * 4,  30 * 4,  31 * 4,

  32 * 4,	/* HPPA_SAR_REGNUM */
  35 * 4,	/* HPPA_PCOQ_HEAD_REGNUM */
  33 * 4,	/* HPPA_PCSQ_HEAD_REGNUM */
  36 * 4,	/* HPPA_PCOQ_TAIL_REGNUM */
  34 * 4,	/* HPPA_PCSQ_TAIL_REGNUM */
  -1,		/* HPPA_EIEM_REGNUM */
  -1,		/* HPPA_IIR_REGNUM */
  -1,		/* HPPA_ISR_REGNUM */
  -1,		/* HPPA_IOR_REGNUM */
  0 * 4,	/* HPPA_IPSW_REGNUM */
};

/* Supply register REGNUM from the buffer specified by GREGS and LEN
   in the general-purpose register set REGSET to register cache
   REGCACHE.  If REGNUM is -1, do this for all registers in REGSET.  */

static void
hppanbsd_supply_gregset (const struct regset *regset,
			 struct regcache *regcache,
			 int regnum, const void *gregs, size_t len)
{
  const gdb_byte *regs = (const gdb_byte *) gregs;
  int i;

  gdb_assert (len >= HPPANBSD_SIZEOF_GREGS);

  for (i = 0; i < ARRAY_SIZE (hppanbsd_reg_offset); i++)
    if (hppanbsd_reg_offset[i] != -1)
      if (regnum == -1 || regnum == i)
	regcache->raw_supply (i, regs + hppanbsd_reg_offset[i]);
}

/* NetBSD/hppa register set.  */

static const struct regset hppanbsd_gregset =
{
  NULL,
  hppanbsd_supply_gregset
};

/* Iterate over supported core file register note sections. */

static void
hppanbsd_iterate_over_regset_sections (struct gdbarch *gdbarch,
				       iterate_over_regset_sections_cb *cb,
				       void *cb_data,
				       const struct regcache *regcache)
{
  cb (".reg", HPPANBSD_SIZEOF_GREGS, HPPANBSD_SIZEOF_GREGS, &hppanbsd_gregset,
      NULL, cb_data);
}

static void
hppanbsd_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  /* Obviously NetBSD is BSD-based.  */
  hppabsd_init_abi (info, gdbarch);

  nbsd_init_abi (info, gdbarch);

  /* Core file support.  */
  set_gdbarch_iterate_over_regset_sections
    (gdbarch, hppanbsd_iterate_over_regset_sections);

  tramp_frame_prepend_unwinder (gdbarch, &hppanbsd_sigtramp_si4);
}

void _initialize_hppanbsd_tdep ();
void
_initialize_hppanbsd_tdep ()
{
  gdbarch_register_osabi (bfd_arch_hppa, 0, GDB_OSABI_NETBSD,
			  hppanbsd_init_abi);
}
