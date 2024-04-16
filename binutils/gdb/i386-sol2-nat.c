/* Native-dependent code for Solaris x86.

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
#include "regcache.h"

#include <sys/reg.h>
#include <sys/procfs.h>
#include "gregset.h"
#include "target.h"
#include "procfs.h"

/* This file provids the (temporary) glue between the Solaris x86
   target dependent code and the machine independent SVR4 /proc
   support.  */

/* Solaris 10 (Solaris 2.10, SunOS 5.10) and up support two process
   data models, the traditional 32-bit data model (ILP32) and the
   64-bit data model (LP64).  The format of /proc depends on the data
   model of the observer (the controlling process, GDB in our case).
   The Solaris header files conveniently define PR_MODEL_NATIVE to the
   data model of the controlling process.  If its value is
   PR_MODEL_LP64, we know that GDB is being compiled as a 64-bit
   program.

   Note that a 32-bit GDB won't be able to debug a 64-bit target
   process using /proc on Solaris.  */

#if PR_MODEL_NATIVE == PR_MODEL_LP64

#include "amd64-nat.h"
#include "amd64-tdep.h"

/* Mapping between the general-purpose registers in gregset_t format
   and GDB's register cache layout.  */

/* From <sys/regset.h>.  */
static int amd64_sol2_gregset64_reg_offset[] = {
  14 * 8,			/* %rax */
  11 * 8,			/* %rbx */
  13 * 8,			/* %rcx */
  12 * 8,			/* %rdx */
  9 * 8,			/* %rsi */
  8 * 8,			/* %rdi */
  10 * 8,			/* %rbp */
  20 * 8,			/* %rsp */
  7 * 8,			/* %r8 ...  */
  6 * 8,
  5 * 8,
  4 * 8,
  3 * 8,
  2 * 8,
  1 * 8,
  0 * 8,			/* ... %r15 */
  17 * 8,			/* %rip */
  19 * 8,			/* %eflags */
  18 * 8,			/* %cs */
  21 * 8,			/* %ss */
  25 * 8,			/* %ds */
  24 * 8,			/* %es */
  22 * 8,			/* %fs */
  23 * 8			/* %gs */
};

/* 32-bit registers are provided by Solaris in 64-bit format, so just
   give a subset of the list above.  */
static int amd64_sol2_gregset32_reg_offset[] = {
  14 * 8,			/* %eax */
  13 * 8,			/* %ecx */
  12 * 8,			/* %edx */
  11 * 8,			/* %ebx */
  20 * 8,			/* %esp */
  10 * 8,			/* %ebp */
  9 * 8,			/* %esi */
  8 * 8,			/* %edi */
  17 * 8,			/* %eip */
  19 * 8,			/* %eflags */
  18 * 8,			/* %cs */
  21 * 8,			/* %ss */
  25 * 8,			/* %ds */
  24 * 8,			/* %es */
  22 * 8,			/* %fs */
  23 * 8			/* %gs */
};

void
supply_gregset (struct regcache *regcache, const prgregset_t *gregs)
{
  amd64_supply_native_gregset (regcache, gregs, -1);
}

void
supply_fpregset (struct regcache *regcache, const prfpregset_t *fpregs)
{
  amd64_supply_fxsave (regcache, -1, fpregs);
}

void
fill_gregset (const struct regcache *regcache,
	      prgregset_t *gregs, int regnum)
{
  amd64_collect_native_gregset (regcache, gregs, regnum);
}

void
fill_fpregset (const struct regcache *regcache,
	       prfpregset_t *fpregs, int regnum)
{
  amd64_collect_fxsave (regcache, regnum, fpregs);
}

#else /* PR_MODEL_NATIVE != PR_MODEL_LP64 */

#include "i386-tdep.h"
#include "i387-tdep.h"

/* The `/proc' interface divides the target machine's register set up
   into two different sets, the general purpose register set (gregset)
   and the floating-point register set (fpregset).

   The actual structure is, of course, naturally machine dependent, and is
   different for each set of registers.  For the i386 for example, the
   general-purpose register set is typically defined by:

   typedef int gregset_t[19];           (in <sys/regset.h>)

   #define GS   0                       (in <sys/reg.h>)
   #define FS   1
   ...
   #define UESP 17
   #define SS   18

   and the floating-point set by:

   typedef struct fpregset   {
	   union {
		   struct fpchip_state            // fp extension state //
		   {
			   int     state[27];     // 287/387 saved state //
			   int     status;        // status word saved at //
						  // exception //
		   } fpchip_state;
		   struct fp_emul_space           // for emulators //
		   {
			   char    fp_emul[246];
			   char    fp_epad[2];
		   } fp_emul_space;
		   int     f_fpregs[62];          // union of the above //
	   } fp_reg_set;
	   long            f_wregs[33];           // saved weitek state //
   } fpregset_t;

   Incidentally fpchip_state contains the FPU state in the same format
   as used by the "fsave" instruction, and that's the only thing we
   support here.  I don't know how the emulator stores it state.  The
   Weitek stuff definitely isn't supported.

   The routines defined here, provide the packing and unpacking of
   gregset_t and fpregset_t formatted data.  */

/* Mapping between the general-purpose registers in `/proc'
   format and GDB's register array layout.  */
static int regmap[] =
{
  11	/* EAX */,
  10	/* ECX */,
  9	/* EDX */,
  8	/* EBX */,
  17	/* UESP */,
  6	/* EBP */,
  5	/* ESI */,
  4	/* EDI */,
  14	/* EIP */,
  16	/* EFL */,
  15	/* CS */,
  18	/* SS */,
  3	/* DS */,
  2	/* ES */,
  1	/* FS */,
  0	/* GS */
};

/* Fill GDB's register array with the general-purpose register values
   in *GREGSETP.  */

void
supply_gregset (struct regcache *regcache, const gregset_t *gregsetp)
{
  const greg_t *regp = (const greg_t *) gregsetp;
  int regnum;

  for (regnum = 0; regnum < I386_NUM_GREGS; regnum++)
    regcache->raw_supply (regnum, regp + regmap[regnum]);
}

/* Fill register REGNUM (if it is a general-purpose register) in
   *GREGSETPS with the value in GDB's register array.  If REGNUM is -1,
   do this for all registers.  */

void
fill_gregset (const struct regcache *regcache,
	      gregset_t *gregsetp, int regnum)
{
  greg_t *regp = (greg_t *) gregsetp;
  int i;

  for (i = 0; i < I386_NUM_GREGS; i++)
    if (regnum == -1 || regnum == i)
      regcache->raw_collect (i, regp + regmap[i]);
}

/* Fill GDB's register array with the floating-point register values in
   *FPREGSETP.  */

void
supply_fpregset (struct regcache *regcache, const fpregset_t *fpregsetp)
{
  if (gdbarch_fp0_regnum (regcache->arch ()) == 0)
    return;

  i387_supply_fsave (regcache, -1, fpregsetp);
}

/* Fill register REGNO (if it is a floating-point register) in
   *FPREGSETP with the value in GDB's register array.  If REGNO is -1,
   do this for all registers.  */

void
fill_fpregset (const struct regcache *regcache,
	       fpregset_t *fpregsetp, int regno)
{
  if (gdbarch_fp0_regnum (regcache->arch ()) == 0)
    return;

  i387_collect_fsave (regcache, regno, fpregsetp);
}

#endif

void _initialize_amd64_sol2_nat ();
void
_initialize_amd64_sol2_nat ()
{
#if PR_MODEL_NATIVE == PR_MODEL_LP64
  amd64_native_gregset32_reg_offset = amd64_sol2_gregset32_reg_offset;
  amd64_native_gregset32_num_regs =
    ARRAY_SIZE (amd64_sol2_gregset32_reg_offset);
  amd64_native_gregset64_reg_offset = amd64_sol2_gregset64_reg_offset;
  amd64_native_gregset64_num_regs =
    ARRAY_SIZE (amd64_sol2_gregset64_reg_offset);
#endif
}
