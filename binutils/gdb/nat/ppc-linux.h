/* Copyright (C) 1995-2024 Free Software Foundation, Inc.

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

#ifndef NAT_PPC_LINUX_H
#define NAT_PPC_LINUX_H

#include <asm/ptrace.h>
#include <asm/cputable.h>

/* This sometimes isn't defined.  */
#ifndef PT_ORIG_R3
#define PT_ORIG_R3 34
#endif
#ifndef PT_TRAP
#define PT_TRAP 40
#endif

/* The PPC_FEATURE_* defines should be provided by <asm/cputable.h>.
   If they aren't, we can provide them ourselves (their values are fixed
   because they are part of the kernel ABI).  They are used in the AT_HWCAP
   entry of the AUXV.  */
#ifndef PPC_FEATURE_BOOKE
#define PPC_FEATURE_BOOKE 0x00008000
#endif
#ifndef PPC_FEATURE_HAS_DFP
#define PPC_FEATURE_HAS_DFP	0x00000400  /* Decimal Floating Point.  */
#endif
#ifndef PPC_FEATURE_HAS_VSX
#define PPC_FEATURE_HAS_VSX 0x00000080
#endif
#ifndef PPC_FEATURE_HAS_ALTIVEC
#define PPC_FEATURE_HAS_ALTIVEC 0x10000000
#endif
#ifndef PPC_FEATURE_HAS_SPE
#define PPC_FEATURE_HAS_SPE 0x00800000
#endif
#ifndef PPC_FEATURE2_DSCR
#define PPC_FEATURE2_DSCR 0x20000000
#endif
#ifndef PPC_FEATURE2_ARCH_2_07
#define PPC_FEATURE2_ARCH_2_07 0x80000000
#endif
#ifndef PPC_FEATURE2_TAR
#define PPC_FEATURE2_TAR 0x04000000
#endif
#ifndef PPC_FEATURE2_EBB
#define PPC_FEATURE2_EBB 0x10000000
#endif
#ifndef PPC_FEATURE2_HTM
#define PPC_FEATURE2_HTM 0x40000000
#endif

/* Glibc's headers don't define PTRACE_GETVRREGS so we cannot use a
   configure time check.  Some older glibc's (for instance 2.2.1)
   don't have a specific powerpc version of ptrace.h, and fall back on
   a generic one.  In such cases, sys/ptrace.h defines
   PTRACE_GETFPXREGS and PTRACE_SETFPXREGS to the same numbers that
   ppc kernel's asm/ptrace.h defines PTRACE_GETVRREGS and
   PTRACE_SETVRREGS to be.  This also makes a configury check pretty
   much useless.  */

/* These definitions should really come from the glibc header files,
   but Glibc doesn't know about the vrregs yet.  */
#ifndef PTRACE_GETVRREGS
#define PTRACE_GETVRREGS 18
#define PTRACE_SETVRREGS 19
#endif

/* PTRACE requests for POWER7 VSX registers.  */
#ifndef PTRACE_GETVSXREGS
#define PTRACE_GETVSXREGS 27
#define PTRACE_SETVSXREGS 28
#endif

/* Similarly for the ptrace requests for getting / setting the SPE
   registers (ev0 -- ev31, acc, and spefscr).  See the description of
   gdb_evrregset_t for details.  */
#ifndef PTRACE_GETEVRREGS
#define PTRACE_GETEVRREGS 20
#define PTRACE_SETEVRREGS 21
#endif

/* Target Address Register.  */
#ifndef NT_PPC_TAR
#define NT_PPC_TAR 0x103
#endif

/* Program Priority Register.  */
#ifndef NT_PPC_PPR
#define NT_PPC_PPR 0x104
#endif

/* Data Stream Control Register.  */
#ifndef NT_PPC_DSCR
#define NT_PPC_DSCR 0x105
#endif

/* Event Based Branch Registers.  */
#ifndef NT_PPC_EBB
#define NT_PPC_EBB 0x106
#endif

/* Performance Monitor Registers.  */
#ifndef NT_PPC_PMU
#define NT_PPC_PMU 0x107
#endif

/* TM checkpointed GPR Registers.  */
#ifndef NT_PPC_TM_CGPR
#define NT_PPC_TM_CGPR 0x108
#endif

/* TM checkpointed FPR Registers.  */
#ifndef NT_PPC_TM_CFPR
#define NT_PPC_TM_CFPR 0x109
#endif

/* TM checkpointed VMX Registers.  */
#ifndef NT_PPC_TM_CVMX
#define NT_PPC_TM_CVMX 0x10a
#endif

/* TM checkpointed VSX Registers.  */
#ifndef NT_PPC_TM_CVSX
#define NT_PPC_TM_CVSX 0x10b
#endif

/* TM Special Purpose Registers.  */
#ifndef NT_PPC_TM_SPR
#define NT_PPC_TM_SPR 0x10c
#endif

/* TM checkpointed Target Address Register.  */
#ifndef NT_PPC_TM_CTAR
#define NT_PPC_TM_CTAR 0x10d
#endif

/* TM checkpointed Program Priority Register.  */
#ifndef NT_PPC_TM_CPPR
#define NT_PPC_TM_CPPR 0x10e
#endif

/* TM checkpointed Data Stream Control Register.  */
#ifndef NT_PPC_TM_CDSCR
#define NT_PPC_TM_CDSCR 0x10f
#endif

/* Return the wordsize of the target, either 4 or 8 bytes.  */
int ppc_linux_target_wordsize (int tid);

#endif /* NAT_PPC_LINUX_H */
