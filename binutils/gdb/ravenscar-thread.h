/* Ada Ravenscar thread support.

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

#ifndef RAVENSCAR_THREAD_H
#define RAVENSCAR_THREAD_H

/* Architecture-specific hooks.  */

struct ravenscar_arch_ops
{
  ravenscar_arch_ops (gdb::array_view<const int> offsets_,
		      int first_stack = -1,
		      int last_stack = -1,
		      int v_init = -1,
		      int fpu_offset = -1,
		      int first_fp = -1,
		      int last_fp = -1)
    : offsets (offsets_),
      first_stack_register (first_stack),
      last_stack_register (last_stack),
      v_init_offset (v_init),
      fpu_context_offset (fpu_offset),
      first_fp_register (first_fp),
      last_fp_register (last_fp)
  {
    /* These must either both be -1 or both be valid.  */
    gdb_assert ((first_stack_register == -1) == (last_stack_register == -1));
    /* They must also be ordered.  */
    gdb_assert (last_stack_register >= first_stack_register);
    /* These must either all be -1 or all be valid.  */
    gdb_assert ((v_init_offset == -1) == (fpu_context_offset == -1)
		&& (fpu_context_offset == -1) == (first_fp_register == -1)
		&& (first_fp_register == -1) == (last_fp_register == -1));
  }

  /* Return true if this architecture implements on-demand floating
     point.  */
  bool on_demand_fp () const
  { return v_init_offset != -1; }

  /* Return true if REGNUM is a floating-point register for this
     target.  If this target does not use the on-demand FP scheme,
     this will always return false.  */
  bool is_fp_register (int regnum) const
  {
    return regnum >= first_fp_register && regnum <= last_fp_register;
  }

  /* Return the offset, in the current task context, of the byte
     indicating whether the FPU has been initialized for the task.
     This can only be called when the architecture implements
     on-demand floating-point.  */
  int get_v_init_offset () const
  {
    gdb_assert (on_demand_fp ());
    return v_init_offset;
  }

  /* Return the offset, in the current task context, of the FPU
     context.  This can only be called when the architecture
     implements on-demand floating-point.  */
  int get_fpu_context_offset () const
  {
    gdb_assert (on_demand_fp ());
    return fpu_context_offset;
  }

  void fetch_register (struct regcache *recache, int regnum) const;
  void store_register (struct regcache *recache, int regnum) const;

private:

  /* An array where the indices are register numbers and the contents
     are offsets.  The offsets are either in the thread descriptor or
     the stack, depending on the other fields.  An offset of -1 means
     that the corresponding register is not stored.  */
  const gdb::array_view<const int> offsets;

  /* If these are -1, then all registers for this architecture are
     stored in the thread descriptor.  Otherwise, these mark a range
     of registers that are stored on the stack.  */
  const int first_stack_register;
  const int last_stack_register;

  /* If these are -1, there is no special treatment for floating-point
     registers -- they are handled, or not, just like all other
     registers.

     Otherwise, they must all not be -1, and the target is one that
     uses on-demand FP initialization.  V_INIT_OFFSET is the offset of
     a boolean field in the context that indicates whether the FP
     registers have been initialized for this task.
     FPU_CONTEXT_OFFSET is the offset of the FPU context from the task
     context.  (This is needed to check whether the FPU registers have
     been saved.)  FIRST_FP_REGISTER and LAST_FP_REGISTER are the
     register numbers of the first and last (inclusive) floating point
     registers.  */
  const int v_init_offset;
  const int fpu_context_offset;
  const int first_fp_register;
  const int last_fp_register;

  /* Helper function to supply one register.  */
  void supply_one_register (struct regcache *regcache, int regnum,
			    CORE_ADDR descriptor,
			    CORE_ADDR stack_base) const;
  /* Helper function to store one register.  */
  void store_one_register (struct regcache *regcache, int regnum,
			   CORE_ADDR descriptor,
			   CORE_ADDR stack_base) const;
  /* Helper function to find stack address where registers are stored.
     This must be called with the stack pointer already supplied in
     the register cache.  */
  CORE_ADDR get_stack_base (struct regcache *) const;
};

#endif /* !defined (RAVENSCAR_THREAD_H) */
