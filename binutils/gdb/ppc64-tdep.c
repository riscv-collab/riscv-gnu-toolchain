/* Common target-dependent code for ppc64 GDB, the GNU debugger.

   Copyright (C) 1986-2024 Free Software Foundation, Inc.

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
#include "infrun.h"
#include "ppc-tdep.h"
#include "ppc64-tdep.h"
#include "elf-bfd.h"

/* Macros for matching instructions.  Note that, since all the
   operands are masked off before they're or-ed into the instruction,
   you can use -1 to make masks.  */

#define insn_d(opcd, rts, ra, d)                \
  ((((unsigned (opcd)) & 0x3f) << 26)		\
   | (((unsigned (rts)) & 0x1f) << 21)		\
   | (((unsigned (ra)) & 0x1f) << 16)		\
   | ((unsigned (d)) & 0xffff))

#define insn_ds(opcd, rts, ra, d, xo)           \
  ((((unsigned (opcd)) & 0x3f) << 26)                      \
   | (((unsigned (rts)) & 0x1f) << 21)                     \
   | (((unsigned (ra)) & 0x1f) << 16)                      \
   | ((unsigned (d)) & 0xfffc)                             \
   | ((unsigned (xo)) & 0x3))

#define insn_xfx(opcd, rts, spr, xo)            \
  ((((unsigned (opcd)) & 0x3f) << 26)                      \
   | (((unsigned (rts)) & 0x1f) << 21)                     \
   | (((unsigned (spr)) & 0x1f) << 16)                     \
   | (((unsigned (spr)) & 0x3e0) << 6)                     \
   | (((unsigned (xo)) & 0x3ff) << 1))

#define prefix(a, b, R, do)				   \
  (((0x1) << 26)					   \
   | (((unsigned (a)) & 0x3) << 24)			   \
   | (((unsigned (b)) & 0x1) << 23)			   \
   | (((unsigned (R)) & 0x1) << 20)			   \
   | ((unsigned (do)) & 0x3ffff))

#define insn_md(opcd, ra, rs, sh, me, rc)	       	   \
  ((((unsigned (opcd)) & 0x3f) << 26)			   \
   | (((unsigned (rs)) & 0x1f) << 21)			   \
   | (((unsigned (ra)) & 0x1f) << 16)			   \
   | (((unsigned (sh)) & 0x3e) << 11)			   \
   | (((unsigned (me)) & 0x3f) << 25)			   \
   | (((unsigned (sh)) & 0x1)  << 1)			   \
   | ((unsigned (rc)) & 0x1))

#define insn_x(opcd, rt, ra, rb, opc2)			   \
  ((((unsigned (opcd)) & 0x3f) << 26)			   \
   | (((unsigned (rt)) & 0x1f) << 21)			   \
   | (((unsigned (ra)) & 0x1f) << 16)			   \
   | (((unsigned (rb)) & 0x3e) << 11)			   \
   | (((unsigned (opc2)) & 0x3FF) << 1))

#define insn_xo(opcd, rt, ra, rb, oe, rc, opc2)		   \
  ((((unsigned (opcd)) & 0x3f) << 26)			   \
   | (((unsigned (rt)) & 0x1f) << 21)			   \
   | (((unsigned (ra)) & 0x1f) << 16)			   \
   | (((unsigned (rb)) & 0x3e) << 11)			   \
   | (((unsigned (oe)) & 0x1) << 10)			   \
   | (((unsigned (opc2)) & 0x1FF) << 1)			   \
   | (((unsigned (rc)))))

/* PLT_OFF is the TOC-relative offset of a 64-bit PowerPC PLT entry.
   Return the function's entry point.  */

static CORE_ADDR
ppc64_plt_entry_point (frame_info_ptr frame, CORE_ADDR plt_off)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  CORE_ADDR tocp;

  if (execution_direction == EXEC_REVERSE)
    {
      /* If executing in reverse, r2 will have been stored to the stack.  */
      CORE_ADDR sp = get_frame_register_unsigned (frame,
						  tdep->ppc_gp0_regnum + 1);
      unsigned int sp_off = tdep->elf_abi == POWERPC_ELF_V1 ? 40 : 24;
      tocp = read_memory_unsigned_integer (sp + sp_off, 8, byte_order);
    }
  else
    tocp = get_frame_register_unsigned (frame, tdep->ppc_gp0_regnum + 2);

  /* The first word of the PLT entry is the function entry point.  */
  return read_memory_unsigned_integer (tocp + plt_off, 8, byte_order);
}

static CORE_ADDR
ppc64_plt_pcrel_entry_point (frame_info_ptr frame, CORE_ADDR plt_off,
			     CORE_ADDR pc)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  /* Execution direction doesn't matter, entry is pc + plt_off either way.
     The first word of the PLT entry is the function entry point.  */
  return read_memory_unsigned_integer (pc + plt_off, 8, byte_order);
}

/* Patterns for the standard linkage functions.  These are built by
   build_plt_stub in bfd/elf64-ppc.c.  */

/* Old ELFv1 PLT call stub.  */

static const struct ppc_insn_pattern ppc64_standard_linkage1[] =
  {
    /* addis r12, r2, <any> */
    { insn_d (-1, -1, -1, 0), insn_d (15, 12, 2, 0), 0 },

    /* std r2, 40(r1) */
    { (unsigned) -1, insn_ds (62, 2, 1, 40, 0), 0 },

    /* ld r11, <any>(r12) */
    { insn_ds (-1, -1, -1, 0, -1), insn_ds (58, 11, 12, 0, 0), 0 },

    /* addis r12, r12, 1 <optional> */
    { insn_d (-1, -1, -1, -1), insn_d (15, 12, 12, 1), 1 },

    /* ld r2, <any>(r12) */
    { insn_ds (-1, -1, -1, 0, -1), insn_ds (58, 2, 12, 0, 0), 0 },

    /* addis r12, r12, 1 <optional> */
    { insn_d (-1, -1, -1, -1), insn_d (15, 12, 12, 1), 1 },

    /* mtctr r11 */
    { insn_xfx (-1, -1, -1, -1), insn_xfx (31, 11, 9, 467), 0 },

    /* ld r11, <any>(r12) <optional> */
    { insn_ds (-1, -1, -1, 0, -1), insn_ds (58, 11, 12, 0, 0), 1 },

    /* bctr */
    { (unsigned) -1, 0x4e800420, 0 },

    { 0, 0, 0 }
  };

/* ELFv1 PLT call stub to access PLT entries more than +/- 32k from r2.
   Also supports older stub with different placement of std 2,40(1),
   a stub that omits the std 2,40(1), and both versions of power7
   thread safety read barriers.  Note that there are actually two more
   instructions following "cmpldi r2, 0", "bnectr+" and "b <glink_i>",
   but there isn't any need to match them.  */

static const struct ppc_insn_pattern ppc64_standard_linkage2[] =
  {
    /* std r2, 40(r1) <optional> */
    { (unsigned) -1, insn_ds (62, 2, 1, 40, 0), 1 },

    /* addis r12, r2, <any> */
    { insn_d (-1, -1, -1, 0), insn_d (15, 12, 2, 0), 0 },

    /* std r2, 40(r1) <optional> */
    { (unsigned) -1, insn_ds (62, 2, 1, 40, 0), 1 },

    /* ld r11, <any>(r12) */
    { insn_ds (-1, -1, -1, 0, -1), insn_ds (58, 11, 12, 0, 0), 0 },

    /* addi r12, r12, <any> <optional> */
    { insn_d (-1, -1, -1, 0), insn_d (14, 12, 12, 0), 1 },

    /* mtctr r11 */
    { insn_xfx (-1, -1, -1, -1), insn_xfx (31, 11, 9, 467), 0 },

    /* xor r11, r11, r11 <optional> */
    { (unsigned) -1, 0x7d6b5a78, 1 },

    /* add r12, r12, r11 <optional> */
    { (unsigned) -1, 0x7d8c5a14, 1 },

    /* ld r2, <any>(r12) */
    { insn_ds (-1, -1, -1, 0, -1), insn_ds (58, 2, 12, 0, 0), 0 },

    /* ld r11, <any>(r12) <optional> */
    { insn_ds (-1, -1, -1, 0, -1), insn_ds (58, 11, 12, 0, 0), 1 },

    /* bctr <optional> */
    { (unsigned) -1, 0x4e800420, 1 },

    /* cmpldi r2, 0 <optional> */
    { (unsigned) -1, 0x28220000, 1 },

    { 0, 0, 0 }
  };

/* ELFv1 PLT call stub to access PLT entries within +/- 32k of r2.  */

static const struct ppc_insn_pattern ppc64_standard_linkage3[] =
  {
    /* std r2, 40(r1) <optional> */
    { (unsigned) -1, insn_ds (62, 2, 1, 40, 0), 1 },

    /* ld r11, <any>(r2) */
    { insn_ds (-1, -1, -1, 0, -1), insn_ds (58, 11, 2, 0, 0), 0 },

    /* addi r2, r2, <any> <optional> */
    { insn_d (-1, -1, -1, 0), insn_d (14, 2, 2, 0), 1 },

    /* mtctr r11 */
    { insn_xfx (-1, -1, -1, -1), insn_xfx (31, 11, 9, 467), 0 },

    /* xor r11, r11, r11 <optional> */
    { (unsigned) -1, 0x7d6b5a78, 1 },

    /* add r2, r2, r11 <optional> */
    { (unsigned) -1, 0x7c425a14, 1 },

    /* ld r11, <any>(r2) <optional> */
    { insn_ds (-1, -1, -1, 0, -1), insn_ds (58, 11, 2, 0, 0), 1 },

    /* ld r2, <any>(r2) */
    { insn_ds (-1, -1, -1, 0, -1), insn_ds (58, 2, 2, 0, 0), 0 },

    /* bctr <optional> */
    { (unsigned) -1, 0x4e800420, 1 },

    /* cmpldi r2, 0 <optional> */
    { (unsigned) -1, 0x28220000, 1 },

    { 0, 0, 0 }
  };

/* ELFv1 PLT call stub to access PLT entries more than +/- 32k from r2.
   A more modern variant of ppc64_standard_linkage2 differing in
   register usage.  */

static const struct ppc_insn_pattern ppc64_standard_linkage4[] =
  {
    /* std r2, 40(r1) <optional> */
    { (unsigned) -1, insn_ds (62, 2, 1, 40, 0), 1 },

    /* addis r11, r2, <any> */
    { insn_d (-1, -1, -1, 0), insn_d (15, 11, 2, 0), 0 },

    /* ld r12, <any>(r11) */
    { insn_ds (-1, -1, -1, 0, -1), insn_ds (58, 12, 11, 0, 0), 0 },

    /* addi r11, r11, <any> <optional> */
    { insn_d (-1, -1, -1, 0), insn_d (14, 11, 11, 0), 1 },

    /* mtctr r12 */
    { insn_xfx (-1, -1, -1, -1), insn_xfx (31, 12, 9, 467), 0 },

    /* xor r2, r12, r12 <optional> */
    { (unsigned) -1, 0x7d826278, 1 },

    /* add r11, r11, r2 <optional> */
    { (unsigned) -1, 0x7d6b1214, 1 },

    /* ld r2, <any>(r11) */
    { insn_ds (-1, -1, -1, 0, -1), insn_ds (58, 2, 11, 0, 0), 0 },

    /* ld r11, <any>(r11) <optional> */
    { insn_ds (-1, -1, -1, 0, -1), insn_ds (58, 11, 11, 0, 0), 1 },

    /* bctr <optional> */
    { (unsigned) -1, 0x4e800420, 1 },

    /* cmpldi r2, 0 <optional> */
    { (unsigned) -1, 0x28220000, 1 },

    { 0, 0, 0 }
  };

/* ELFv1 PLT call stub to access PLT entries within +/- 32k of r2.
   A more modern variant of ppc64_standard_linkage3 differing in
   register usage.  */

static const struct ppc_insn_pattern ppc64_standard_linkage5[] =
  {
    /* std r2, 40(r1) <optional> */
    { (unsigned) -1, insn_ds (62, 2, 1, 40, 0), 1 },

    /* ld r12, <any>(r2) */
    { insn_ds (-1, -1, -1, 0, -1), insn_ds (58, 12, 2, 0, 0), 0 },

    /* addi r2, r2, <any> <optional> */
    { insn_d (-1, -1, -1, 0), insn_d (14, 2, 2, 0), 1 },

    /* mtctr r12 */
    { insn_xfx (-1, -1, -1, -1), insn_xfx (31, 12, 9, 467), 0 },

    /* xor r11, r12, r12 <optional> */
    { (unsigned) -1, 0x7d8b6278, 1 },

    /* add r2, r2, r11 <optional> */
    { (unsigned) -1, 0x7c425a14, 1 },

    /* ld r11, <any>(r2) <optional> */
    { insn_ds (-1, -1, -1, 0, -1), insn_ds (58, 11, 2, 0, 0), 1 },

    /* ld r2, <any>(r2) */
    { insn_ds (-1, -1, -1, 0, -1), insn_ds (58, 2, 2, 0, 0), 0 },

    /* bctr <optional> */
    { (unsigned) -1, 0x4e800420, 1 },

    /* cmpldi r2, 0 <optional> */
    { (unsigned) -1, 0x28220000, 1 },

    { 0, 0, 0 }
  };

/* ELFv2 PLT call stub to access PLT entries more than +/- 32k from r2.  */

static const struct ppc_insn_pattern ppc64_standard_linkage6[] =
  {
    /* std r2, 24(r1) <optional> */
    { (unsigned) -1, insn_ds (62, 2, 1, 24, 0), 1 },

    /* addis r11, r2, <any> */
    { insn_d (-1, -1, -1, 0), insn_d (15, 11, 2, 0), 0 },

    /* ld r12, <any>(r11) */
    { insn_ds (-1, -1, -1, 0, -1), insn_ds (58, 12, 11, 0, 0), 0 },

    /* mtctr r12 */
    { insn_xfx (-1, -1, -1, -1), insn_xfx (31, 12, 9, 467), 0 },

    /* bctr */
    { (unsigned) -1, 0x4e800420, 0 },

    { 0, 0, 0 }
  };

/* ELFv2 PLT call stub to access PLT entries within +/- 32k of r2.  */

static const struct ppc_insn_pattern ppc64_standard_linkage7[] =
  {
    /* std r2, 24(r1) <optional> */
    { (unsigned) -1, insn_ds (62, 2, 1, 24, 0), 1 },

    /* ld r12, <any>(r2) */
    { insn_ds (-1, -1, -1, 0, -1), insn_ds (58, 12, 2, 0, 0), 0 },

    /* mtctr r12 */
    { insn_xfx (-1, -1, -1, -1), insn_xfx (31, 12, 9, 467), 0 },

    /* bctr */
    { (unsigned) -1, 0x4e800420, 0 },

    { 0, 0, 0 }
  };

/* ELFv2 PLT call stub to access PLT entries more than +/- 32k from r2,
   supporting fusion.  */

static const struct ppc_insn_pattern ppc64_standard_linkage8[] =
  {
    /* std r2, 24(r1) <optional> */
    { (unsigned) -1, insn_ds (62, 2, 1, 24, 0), 1 },

    /* addis r12, r2, <any> */
    { insn_d (-1, -1, -1, 0), insn_d (15, 12, 2, 0), 0 },

    /* ld r12, <any>(r12) */
    { insn_ds (-1, -1, -1, 0, -1), insn_ds (58, 12, 12, 0, 0), 0 },

    /* mtctr r12 */
    { insn_xfx (-1, -1, -1, -1), insn_xfx (31, 12, 9, 467), 0 },

    /* bctr */
    { (unsigned) -1, 0x4e800420, 0 },

    { 0, 0, 0 }
  };

/* Power 10 ELFv2 PLT call stubs */
static const struct ppc_insn_pattern ppc64_standard_linkage9[] =
  {
    /* std   %r2,0+40(%r1)   <optional> */
    { insn_ds (-1, -1, -1, 0, 1), insn_ds (62, 2, 1, 40, 0), 1 },

    /* pld r12, <any> */
    { prefix (-1, -1, 1, 0), prefix (0, 0, 1, 0), 0 },
    { insn_d (-1, -1, -1, 0), insn_d (57, 12, 0, 0), 0 },

    /* mtctr r12  */
    { insn_xfx (-1, -1, -1, -1), insn_xfx (31, 12, 9, 467), 0 },

    /* bctr   */
    { (unsigned) -1, 0x4e800420, 0 },

    { 0, 0, 0 }
  };

static const struct ppc_insn_pattern ppc64_standard_linkage10[] =
  {
    /* std   %r2,0+40(%r1)    <optional> */
    { insn_ds (-1, -1, -1, 0, 1), insn_ds (62, 2, 1, 40, 0), 1 },

    /* paddi r12,<any> */
    { prefix (-1, -1, 1, 0), prefix (2, 0, 1, 0), 0 },
    { insn_d (-1, -1, -1, 0), insn_d (14, 12, 0, 0), 0 },

    /* mtctr r12  <optional> */
    { insn_xfx (-1, -1, -1, -1), insn_xfx (31, 12, 9, 467), 0 },

    /* bctr   */
    { (unsigned) -1, 0x4e800420, 0 },

    { 0, 0, 0 }
  };

static const struct ppc_insn_pattern ppc64_standard_linkage11[] =
  {
    /* std   %r2,0+40(%r1)   <optional> */
    { insn_ds (-1, -1, -1, 0, 1), insn_ds (62, 2, 1, 40, 0), 1 },

    /* li %r11,0     <optional> */
    { insn_d (-1, -1, -1, 0), insn_d (14, 11, 0, 0), 1 },

    /* sldi  %r11,%r11,34   <eq to rldicr rx,ry,n, 63-n> <optional>  */
    { insn_md (-1, -1, -1, 0, 0, 1), insn_md (30, 11, 11, 34, 63-34, 0), 1 },

    /* paddi r12, <any> */
    { prefix (-1, -1, 1, 0), prefix (2, 0, 1, 0), 0 },
    { insn_d (-1, -1, -1, 0), insn_d (14, 12, 0, 0), 0 },

    /* ldx   %r12,%r11,%r12  <optional> */
    { (unsigned) -1, insn_x (31, 12, 11, 12, 21), 1 },

    /* add   %r12,%r11,%r12  <optional> */
    { (unsigned) -1, insn_xo (31, 12, 11, 12, 0, 0, 40), 1 },

    /* mtctr r12   */
    { insn_xfx (-1, -1, -1, -1), insn_xfx (31, 12, 9, 467), 0 },

    /* bctr   */ // 13, 14, 15, 16
    { (unsigned) -1, 0x4e800420, 0 },

    { 0, 0, 0 }
  };

static const struct ppc_insn_pattern ppc64_standard_linkage12[] =
  {
    /* std   %r2,0+40(%r1)    <optional>  */
    { insn_ds (-1, -1, -1, 0, 1), insn_ds (62, 2, 1, 40, 0), 1 },

    /* lis %r11,xxx@ha <equivalent addis rx, 0, val> */
    /* addis r12, r2, <any> */
    { insn_d (-1, -1, -1, 0), insn_d (15, 12, 2, 0), 0 },

    /* ori   %r11,%r11,xxx@l */
    { insn_d (-1, -1, -1, 0), insn_d (24, 11, 11, 0), 0 },

    /* sldi  %r11,%r11,34 <optional> */
    { (unsigned) -1, insn_md (30, 11, 11, 34, 63-34, 0), 1 },

    /*paddi r12,<any> */
    { prefix (-1, -1, 1, 0), prefix (2, 0, 1, 0), 0 },
    { insn_d (-1, -1, -1, 0), insn_d (14, 12, 0, 0), 0 },

    /* sldi  %r11,%r11,34 <optional> */
    { (unsigned) -1, insn_md (30, 11, 11, 34, 63-34, 0), 1 },

    /* ldx   %r12,%r11,%r12 <optional> */
    { (unsigned) -1, insn_x (31, 12, 11, 12, 21), 1 },

    /* add   %r12,%r11,%r12 <optional> */
    { (unsigned) -1, insn_xo (31, 12, 11, 12, 0, 0, 40), 1 },

    /* mtctr r12  */
    { insn_xfx (-1, -1, -1, -1), insn_xfx (31, 12, 9, 467), 0 },

    /* bctr  */ // 17, 18, 19, 20
    { (unsigned) -1, 0x4e800420, 0 },

    { 0, 0, 0 }
  };

/* When the dynamic linker is doing lazy symbol resolution, the first
   call to a function in another object will go like this:

   - The user's function calls the linkage function:

	100003d4:   4b ff ff ad     bl      10000380 <nnnn.plt_call.printf>
	100003d8:   e8 41 00 28     ld      r2,40(r1)

   - The linkage function loads the entry point and toc pointer from
     the function descriptor in the PLT, and jumps to it:

     <nnnn.plt_call.printf>:
	10000380:   f8 41 00 28     std     r2,40(r1)
	10000384:   e9 62 80 78     ld      r11,-32648(r2)
	10000388:   7d 69 03 a6     mtctr   r11
	1000038c:   e8 42 80 80     ld      r2,-32640(r2)
	10000390:   28 22 00 00     cmpldi  r2,0
	10000394:   4c e2 04 20     bnectr+ 
	10000398:   48 00 03 a0     b       10000738 <printf@plt>

   - But since this is the first time that PLT entry has been used, it
     sends control to its glink entry.  That loads the number of the
     PLT entry and jumps to the common glink0 code:

     <printf@plt>:
	10000738:   38 00 00 01     li      r0,1
	1000073c:   4b ff ff bc     b       100006f8 <__glink_PLTresolve>

   - The common glink0 code then transfers control to the dynamic
     linker's fixup code:

	100006f0:   0000000000010440 .quad plt0 - (. + 16)
     <__glink_PLTresolve>:
	100006f8:   7d 88 02 a6     mflr    r12
	100006fc:   42 9f 00 05     bcl     20,4*cr7+so,10000700
	10000700:   7d 68 02 a6     mflr    r11
	10000704:   e8 4b ff f0     ld      r2,-16(r11)
	10000708:   7d 88 03 a6     mtlr    r12
	1000070c:   7d 82 5a 14     add     r12,r2,r11
	10000710:   e9 6c 00 00     ld      r11,0(r12)
	10000714:   e8 4c 00 08     ld      r2,8(r12)
	10000718:   7d 69 03 a6     mtctr   r11
	1000071c:   e9 6c 00 10     ld      r11,16(r12)
	10000720:   4e 80 04 20     bctr

   Eventually, this code will figure out how to skip all of this,
   including the dynamic linker.  At the moment, we just get through
   the linkage function.  */

/* If the current thread is about to execute a series of instructions
   matching the ppc64_standard_linkage pattern, and INSN is the result
   from that pattern match, return the code address to which the
   standard linkage function will send them.  (This doesn't deal with
   dynamic linker lazy symbol resolution stubs.)  */

static CORE_ADDR
ppc64_standard_linkage1_target (frame_info_ptr frame, unsigned int *insn)
{
  CORE_ADDR plt_off = ((ppc_insn_d_field (insn[0]) << 16)
		       + ppc_insn_ds_field (insn[2]));

  return ppc64_plt_entry_point (frame, plt_off);
}

static CORE_ADDR
ppc64_standard_linkage2_target (frame_info_ptr frame, unsigned int *insn)
{
  CORE_ADDR plt_off = ((ppc_insn_d_field (insn[1]) << 16)
		       + ppc_insn_ds_field (insn[3]));

  return ppc64_plt_entry_point (frame, plt_off);
}

static CORE_ADDR
ppc64_standard_linkage3_target (frame_info_ptr frame, unsigned int *insn)
{
  CORE_ADDR plt_off = ppc_insn_ds_field (insn[1]);

  return ppc64_plt_entry_point (frame, plt_off);
}

static CORE_ADDR
ppc64_standard_linkage4_target (frame_info_ptr frame, unsigned int *insn)
{
  CORE_ADDR plt_off = ((ppc_insn_d_field (insn[1]) << 16)
		       + ppc_insn_ds_field (insn[2]));

  return ppc64_plt_entry_point (frame, plt_off);
}

static CORE_ADDR
ppc64_pcrel_linkage1_target (frame_info_ptr frame, unsigned int *insn,
			     CORE_ADDR pc)
{
  /* insn[0] is for the std instruction.  */
  CORE_ADDR plt_off = ppc_insn_prefix_dform (insn[1], insn[2]);

  return ppc64_plt_pcrel_entry_point (frame, plt_off, pc);
}

static CORE_ADDR
ppc64_pcrel_linkage2_target (frame_info_ptr frame, unsigned int *insn,
			     CORE_ADDR pc)
{
  CORE_ADDR plt_off;

  /* insn[0] is for the std instruction.
     insn[1] is for the  li r11 instruction  */
  plt_off = ppc_insn_prefix_dform (insn[2], insn[3]);

  return ppc64_plt_pcrel_entry_point (frame, plt_off, pc);
}


/* Given that we've begun executing a call trampoline at PC, return
   the entry point of the function the trampoline will go to.

   When the execution direction is EXEC_REVERSE, scan backward to
   check whether we are in the middle of a PLT stub.  */

static CORE_ADDR
ppc64_skip_trampoline_code_1 (frame_info_ptr frame, CORE_ADDR pc)
{
#define MAX(a,b) ((a) > (b) ? (a) : (b))
  unsigned int insns[MAX (MAX (MAX (ARRAY_SIZE (ppc64_standard_linkage1),
				    ARRAY_SIZE (ppc64_standard_linkage2)),
			       MAX (ARRAY_SIZE (ppc64_standard_linkage3),
				    ARRAY_SIZE (ppc64_standard_linkage4))),
		      MAX(MAX (MAX (ARRAY_SIZE (ppc64_standard_linkage5),
				    ARRAY_SIZE (ppc64_standard_linkage6)),
			       MAX (ARRAY_SIZE (ppc64_standard_linkage7),
				    ARRAY_SIZE (ppc64_standard_linkage8))),
			  MAX (MAX (ARRAY_SIZE (ppc64_standard_linkage9),
				    ARRAY_SIZE (ppc64_standard_linkage10)),
			       MAX (ARRAY_SIZE (ppc64_standard_linkage11),
				    ARRAY_SIZE (ppc64_standard_linkage12)))))

		     - 1];
  CORE_ADDR target;
  int scan_limit, i;

  scan_limit = 1;
  /* When reverse-debugging, scan backward to check whether we are
     in the middle of trampoline code.  */
  if (execution_direction == EXEC_REVERSE)
    scan_limit = ARRAY_SIZE (insns) - 1;

  for (i = 0; i < scan_limit; i++)
    {
      if (i < ARRAY_SIZE (ppc64_standard_linkage12) - 1
	  && ppc_insns_match_pattern (frame, pc, ppc64_standard_linkage12, insns))
	pc = ppc64_pcrel_linkage1_target (frame, insns, pc);
      else if (i < ARRAY_SIZE (ppc64_standard_linkage11) - 1
	  && ppc_insns_match_pattern (frame, pc, ppc64_standard_linkage11, insns))
	pc = ppc64_pcrel_linkage2_target (frame, insns, pc);
      else if (i < ARRAY_SIZE (ppc64_standard_linkage10) - 1
	  && ppc_insns_match_pattern (frame, pc, ppc64_standard_linkage10, insns))
	pc = ppc64_pcrel_linkage1_target (frame, insns, pc);
      else if (i < ARRAY_SIZE (ppc64_standard_linkage9) - 1
	  && ppc_insns_match_pattern (frame, pc, ppc64_standard_linkage9, insns))
	pc = ppc64_pcrel_linkage1_target (frame, insns, pc);
      else if (i < ARRAY_SIZE (ppc64_standard_linkage8) - 1
	  && ppc_insns_match_pattern (frame, pc, ppc64_standard_linkage8, insns))
	pc = ppc64_standard_linkage4_target (frame, insns);
      else if (i < ARRAY_SIZE (ppc64_standard_linkage7) - 1
	       && ppc_insns_match_pattern (frame, pc, ppc64_standard_linkage7,
					   insns))
	pc = ppc64_standard_linkage3_target (frame, insns);
      else if (i < ARRAY_SIZE (ppc64_standard_linkage6) - 1
	       && ppc_insns_match_pattern (frame, pc, ppc64_standard_linkage6,
					   insns))
	pc = ppc64_standard_linkage4_target (frame, insns);
      else if (i < ARRAY_SIZE (ppc64_standard_linkage5) - 1
	       && ppc_insns_match_pattern (frame, pc, ppc64_standard_linkage5,
					   insns)
	       && (insns[8] != 0 || insns[9] != 0))
	pc = ppc64_standard_linkage3_target (frame, insns);
      else if (i < ARRAY_SIZE (ppc64_standard_linkage4) - 1
	       && ppc_insns_match_pattern (frame, pc, ppc64_standard_linkage4,
					   insns)
	       && (insns[9] != 0 || insns[10] != 0))
	pc = ppc64_standard_linkage4_target (frame, insns);
      else if (i < ARRAY_SIZE (ppc64_standard_linkage3) - 1
	       && ppc_insns_match_pattern (frame, pc, ppc64_standard_linkage3,
					   insns)
	       && (insns[8] != 0 || insns[9] != 0))
	pc = ppc64_standard_linkage3_target (frame, insns);
      else if (i < ARRAY_SIZE (ppc64_standard_linkage2) - 1
	       && ppc_insns_match_pattern (frame, pc, ppc64_standard_linkage2,
					   insns)
	       && (insns[10] != 0 || insns[11] != 0))
	pc = ppc64_standard_linkage2_target (frame, insns);
      else if (i < ARRAY_SIZE (ppc64_standard_linkage1) - 1
	       && ppc_insns_match_pattern (frame, pc, ppc64_standard_linkage1,
					   insns))
	pc = ppc64_standard_linkage1_target (frame, insns);
      else
	{
	  /* Scan backward one more instruction if it doesn't match.  */
	  pc -= 4;
	  continue;
	}

      /* The PLT descriptor will either point to the already resolved target
	 address, or else to a glink stub.  As the latter carry synthetic @plt
	 symbols, find_solib_trampoline_target should be able to resolve them.  */
      target = find_solib_trampoline_target (frame, pc);
      return target ? target : pc;
  }

  return 0;
}

/* Wrapper of ppc64_skip_trampoline_code_1 checking also
   ppc_elfv2_skip_entrypoint.  */

CORE_ADDR
ppc64_skip_trampoline_code (frame_info_ptr frame, CORE_ADDR pc)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);

  pc = ppc64_skip_trampoline_code_1 (frame, pc);
  if (pc != 0 && gdbarch_skip_entrypoint_p (gdbarch))
    pc = gdbarch_skip_entrypoint (gdbarch, pc);
  return pc;
}

/* Support for convert_from_func_ptr_addr (ARCH, ADDR, TARG) on PPC64
   GNU/Linux.

   Usually a function pointer's representation is simply the address
   of the function.  On GNU/Linux on the PowerPC however, a function
   pointer may be a pointer to a function descriptor.

   For PPC64, a function descriptor is a TOC entry, in a data section,
   which contains three words: the first word is the address of the
   function, the second word is the TOC pointer (r2), and the third word
   is the static chain value.

   Throughout GDB it is currently assumed that a function pointer contains
   the address of the function, which is not easy to fix.  In addition, the
   conversion of a function address to a function pointer would
   require allocation of a TOC entry in the inferior's memory space,
   with all its drawbacks.  To be able to call C++ virtual methods in
   the inferior (which are called via function pointers),
   find_function_addr uses this function to get the function address
   from a function pointer.

   If ADDR points at what is clearly a function descriptor, transform
   it into the address of the corresponding function, if needed.  Be
   conservative, otherwise GDB will do the transformation on any
   random addresses such as occur when there is no symbol table.  */

CORE_ADDR
ppc64_convert_from_func_ptr_addr (struct gdbarch *gdbarch,
					CORE_ADDR addr,
					struct target_ops *targ)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  const struct target_section *s = target_section_by_addr (targ, addr);

  /* Check if ADDR points to a function descriptor.  */
  if (s && strcmp (s->the_bfd_section->name, ".opd") == 0)
    {
      /* There may be relocations that need to be applied to the .opd 
	 section.  Unfortunately, this function may be called at a time
	 where these relocations have not yet been performed -- this can
	 happen for example shortly after a library has been loaded with
	 dlopen, but ld.so has not yet applied the relocations.

	 To cope with both the case where the relocation has been applied,
	 and the case where it has not yet been applied, we do *not* read
	 the (maybe) relocated value from target memory, but we instead
	 read the non-relocated value from the BFD, and apply the relocation
	 offset manually.

	 This makes the assumption that all .opd entries are always relocated
	 by the same offset the section itself was relocated.  This should
	 always be the case for GNU/Linux executables and shared libraries.
	 Note that other kind of object files (e.g. those added via
	 add-symbol-files) will currently never end up here anyway, as this
	 function accesses *target* sections only; only the main exec and
	 shared libraries are ever added to the target.  */

      gdb_byte buf[8];
      int res;

      res = bfd_get_section_contents (s->the_bfd_section->owner,
				      s->the_bfd_section,
				      &buf, addr - s->addr, 8);
      if (res != 0)
	return (extract_unsigned_integer (buf, 8, byte_order)
		- bfd_section_vma (s->the_bfd_section) + s->addr);
   }

  return addr;
}

/* A synthetic 'dot' symbols on ppc64 has the udata.p entry pointing
   back to the original ELF symbol it was derived from.  Get the size
   from that symbol.  */

void
ppc64_elf_make_msymbol_special (asymbol *sym, struct minimal_symbol *msym)
{
  if ((sym->flags & BSF_SYNTHETIC) != 0 && sym->udata.p != NULL)
    {
      elf_symbol_type *elf_sym = (elf_symbol_type *) sym->udata.p;
      msym->set_size (elf_sym->internal_elf_sym.st_size);
    }
}
