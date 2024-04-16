/* Native-dependent code for OpenBSD/amd64.

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
#include "gdbcore.h"
#include "regcache.h"
#include "target.h"

#include "amd64-tdep.h"
#include "amd64-bsd-nat.h"
#include "amd64-nat.h"
#include "obsd-nat.h"

/* Mapping between the general-purpose registers in OpenBSD/amd64
   `struct reg' format and GDB's register cache layout for
   OpenBSD/i386.

   Note that most (if not all) OpenBSD/amd64 registers are 64-bit,
   while the OpenBSD/i386 registers are all 32-bit, but since we're
   little-endian we get away with that.  */

/* From <machine/reg.h>.  */
static int amd64obsd32_r_reg_offset[] =
{
  14 * 8,			/* %eax */
  3 * 8,			/* %ecx */
  2 * 8,			/* %edx */
  13 * 8,			/* %ebx */
  15 * 8,			/* %esp */
  12 * 8,			/* %ebp */
  1 * 8,			/* %esi */
  0 * 8,			/* %edi */
  16 * 8,			/* %eip */
  17 * 8,			/* %eflags */
  18 * 8,			/* %cs */
  19 * 8,			/* %ss */
  20 * 8,			/* %ds */
  21 * 8,			/* %es */
  22 * 8,			/* %fs */
  23 * 8			/* %gs */
};


/* Support for debugging kernel virtual memory images.  */

#include <sys/types.h>
#include <machine/frame.h>
#include <machine/pcb.h>

#include "bsd-kvm.h"

static int
amd64obsd_supply_pcb (struct regcache *regcache, struct pcb *pcb)
{
  struct switchframe sf;
  int regnum;

  /* The following is true for OpenBSD 3.5:

     The pcb contains the stack pointer at the point of the context
     switch in cpu_switch().  At that point we have a stack frame as
     described by `struct switchframe', which for OpenBSD 3.5 has the
     following layout:

     interrupt level
     %r15
     %r14
     %r13
     %r12
     %rbp
     %rbx
     return address

     Together with %rsp in the pcb, this accounts for all callee-saved
     registers specified by the psABI.  From this information we
     reconstruct the register state as it would look when we just
     returned from cpu_switch().

     For core dumps the pcb is saved by savectx().  In that case the
     stack frame only contains the return address, and there is no way
     to recover the other registers.  */

  /* The stack pointer shouldn't be zero.  */
  if (pcb->pcb_rsp == 0)
    return 0;

  /* Read the stack frame, and check its validity.  */
  read_memory (pcb->pcb_rsp, (gdb_byte *) &sf, sizeof sf);
  if (sf.sf_rbp == pcb->pcb_rbp)
    {
      /* Yes, we have a frame that matches cpu_switch().  */
      pcb->pcb_rsp += sizeof (struct switchframe);
      regcache->raw_supply (12, &sf.sf_r12);
      regcache->raw_supply (13, &sf.sf_r13);
      regcache->raw_supply (14, &sf.sf_r14);
      regcache->raw_supply (15, &sf.sf_r15);
      regcache->raw_supply (AMD64_RBX_REGNUM, &sf.sf_rbx);
      regcache->raw_supply (AMD64_RIP_REGNUM, &sf.sf_rip);
    }
  else
    {
      /* No, the pcb must have been last updated by savectx().  */
      pcb->pcb_rsp += 8;
      regcache->raw_supply (AMD64_RIP_REGNUM, &sf);
    }

  regcache->raw_supply (AMD64_RSP_REGNUM, &pcb->pcb_rsp);
  regcache->raw_supply (AMD64_RBP_REGNUM, &pcb->pcb_rbp);

  return 1;
}

static amd64_bsd_nat_target<obsd_nat_target> the_amd64_obsd_nat_target;

void _initialize_amd64obsd_nat ();
void
_initialize_amd64obsd_nat ()
{
  amd64_native_gregset32_reg_offset = amd64obsd32_r_reg_offset;
  amd64_native_gregset32_num_regs = ARRAY_SIZE (amd64obsd32_r_reg_offset);
  amd64_native_gregset64_reg_offset = amd64obsd_r_reg_offset;

  add_inf_child_target (&the_amd64_obsd_nat_target);

  /* Support debugging kernel virtual memory images.  */
  bsd_kvm_add_target (amd64obsd_supply_pcb);
}
