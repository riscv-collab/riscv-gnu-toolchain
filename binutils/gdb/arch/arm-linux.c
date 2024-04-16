/* Common target dependent code for GNU/Linux on ARM systems.

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

#include "gdbsupport/common-defs.h"
#include "gdbsupport/common-regcache.h"
#include "arch/arm.h"
#include "arm-linux.h"
#include "arch/arm-get-next-pcs.h"

/* Calculate the offset from stack pointer of the pc register on the stack
   in the case of a sigreturn or sigreturn_rt syscall.  */
int
arm_linux_sigreturn_next_pc_offset (unsigned long sp,
				    unsigned long sp_data,
				    unsigned long svc_number,
				    int is_sigreturn)
{
  /* Offset of R0 register.  */
  int r0_offset = 0;
  /* Offset of PC register.  */
  int pc_offset = 0;

  if (is_sigreturn)
    {
      if (sp_data == ARM_NEW_SIGFRAME_MAGIC)
	r0_offset = ARM_UCONTEXT_SIGCONTEXT + ARM_SIGCONTEXT_R0;
      else
	r0_offset = ARM_SIGCONTEXT_R0;
    }
  else
    {
      if (sp_data == sp + ARM_OLD_RT_SIGFRAME_SIGINFO)
	r0_offset = ARM_OLD_RT_SIGFRAME_UCONTEXT;
      else
	r0_offset = ARM_NEW_RT_SIGFRAME_UCONTEXT;

      r0_offset += ARM_UCONTEXT_SIGCONTEXT + ARM_SIGCONTEXT_R0;
    }

  pc_offset = r0_offset + ARM_INT_REGISTER_SIZE * ARM_PC_REGNUM;

  return pc_offset;
}

/* Implementation of "fixup" method of struct arm_get_next_pcs_ops
   for arm-linux.  */

CORE_ADDR
arm_linux_get_next_pcs_fixup (struct arm_get_next_pcs *self,
			      CORE_ADDR nextpc)
{
  /* The Linux kernel offers some user-mode helpers in a high page.  We can
     not read this page (as of 2.6.23), and even if we could then we
     couldn't set breakpoints in it, and even if we could then the atomic
     operations would fail when interrupted.  They are all (tail) called
     as functions and return to the address in LR.  However, when GDB single
     step this instruction, this instruction isn't executed yet, and LR
     may not be updated yet.  In other words, GDB can get the target
     address from LR if this instruction isn't BL or BLX.  */
  if (nextpc > 0xffff0000)
    {
      int bl_blx_p = 0;
      CORE_ADDR pc = regcache_read_pc (self->regcache);
      int pc_incr = 0;

      if (self->ops->is_thumb (self))
	{
	  unsigned short inst1
	    = self->ops->read_mem_uint (pc, 2, self->byte_order_for_code);

	  if (bits (inst1, 8, 15) == 0x47 && bit (inst1, 7))
	    {
	      /* BLX Rm */
	      bl_blx_p = 1;
	      pc_incr = 2;
	    }
	  else if (thumb_insn_size (inst1) == 4)
	    {
	      unsigned short inst2;

	      inst2 = self->ops->read_mem_uint (pc + 2, 2,
						self->byte_order_for_code);

	      if ((inst1 & 0xf800) == 0xf000 && bits (inst2, 14, 15) == 0x3)
		{
		  /* BL <label> and BLX <label> */
		  bl_blx_p = 1;
		  pc_incr = 4;
		}
	    }

	  pc_incr = MAKE_THUMB_ADDR (pc_incr);
	}
      else
	{
	  unsigned int insn
	    = self->ops->read_mem_uint (pc, 4, self->byte_order_for_code);

	  if (bits (insn, 28, 31) == INST_NV)
	    {
	      if (bits (insn, 25, 27) == 0x5) /* BLX <label> */
		bl_blx_p = 1;
	    }
	  else
	    {
	      if (bits (insn, 24, 27) == 0xb  /* BL <label> */
		  || bits (insn, 4, 27) == 0x12fff3 /* BLX Rm */)
		bl_blx_p = 1;
	    }

	  pc_incr = 4;
	}

      /* If the instruction BL or BLX, the target address is the following
	 instruction of BL or BLX, otherwise, the target address is in LR
	 already.  */
      if (bl_blx_p)
	nextpc = pc + pc_incr;
      else
	nextpc = regcache_raw_get_unsigned (self->regcache, ARM_LR_REGNUM);
    }
  return nextpc;
}
