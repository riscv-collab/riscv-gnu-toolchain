/* Low level support for ppc, shared between gdbserver and IPA.

   Copyright (C) 2016-2024 Free Software Foundation, Inc.

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

#ifndef GDBSERVER_LINUX_PPC_TDESC_INIT_H
#define GDBSERVER_LINUX_PPC_TDESC_INIT_H

/* Note: since IPA obviously knows what ABI it's running on (32 vs 64),
   it's sufficient to pass only the register set here.  This, together with
   the ABI known at IPA compile time, maps to a tdesc.  */

enum ppc_linux_tdesc {
  PPC_TDESC_BASE,
  PPC_TDESC_ALTIVEC,
  PPC_TDESC_CELL,  /* No longer used, but kept to avoid ABI changes.  */
  PPC_TDESC_VSX,
  PPC_TDESC_ISA205,
  PPC_TDESC_ISA205_ALTIVEC,
  PPC_TDESC_ISA205_VSX,
  PPC_TDESC_ISA205_PPR_DSCR_VSX,
  PPC_TDESC_ISA207_VSX,
  PPC_TDESC_ISA207_HTM_VSX,
  PPC_TDESC_E500,
};

#if !defined __powerpc64__ || !defined IN_PROCESS_AGENT

/* Defined in auto-generated file powerpc-32l.c.  */
void init_registers_powerpc_32l (void);

/* Defined in auto-generated file powerpc-altivec32l.c.  */
void init_registers_powerpc_altivec32l (void);

/* Defined in auto-generated file powerpc-vsx32l.c.  */
void init_registers_powerpc_vsx32l (void);

/* Defined in auto-generated file powerpc-isa205-32l.c.  */
void init_registers_powerpc_isa205_32l (void);

/* Defined in auto-generated file powerpc-isa205-altivec32l.c.  */
void init_registers_powerpc_isa205_altivec32l (void);

/* Defined in auto-generated file powerpc-isa205-vsx32l.c.  */
void init_registers_powerpc_isa205_vsx32l (void);

/* Defined in auto-generated file powerpc-isa205-ppr-dscr-vsx32l.c.  */
void init_registers_powerpc_isa205_ppr_dscr_vsx32l (void);

/* Defined in auto-generated file powerpc-isa207-vsx32l.c.  */
void init_registers_powerpc_isa207_vsx32l (void);

/* Defined in auto-generated file powerpc-isa207-htm-vsx32l.c.  */
void init_registers_powerpc_isa207_htm_vsx32l (void);

/* Defined in auto-generated file powerpc-e500l.c.  */
void init_registers_powerpc_e500l (void);

#endif

#if defined __powerpc64__

/* Defined in auto-generated file powerpc-64l.c.  */
void init_registers_powerpc_64l (void);

/* Defined in auto-generated file powerpc-altivec64l.c.  */
void init_registers_powerpc_altivec64l (void);

/* Defined in auto-generated file powerpc-vsx64l.c.  */
void init_registers_powerpc_vsx64l (void);

/* Defined in auto-generated file powerpc-isa205-64l.c.  */
void init_registers_powerpc_isa205_64l (void);

/* Defined in auto-generated file powerpc-isa205-altivec64l.c.  */
void init_registers_powerpc_isa205_altivec64l (void);

/* Defined in auto-generated file powerpc-isa205-vsx64l.c.  */
void init_registers_powerpc_isa205_vsx64l (void);

/* Defined in auto-generated file powerpc-isa205-ppr-dscr-vsx64l.c.  */
void init_registers_powerpc_isa205_ppr_dscr_vsx64l (void);

/* Defined in auto-generated file powerpc-isa207-vsx64l.c.  */
void init_registers_powerpc_isa207_vsx64l (void);

/* Defined in auto-generated file powerpc-isa207-htm-vsx64l.c.  */
void init_registers_powerpc_isa207_htm_vsx64l (void);

#endif

#endif /* GDBSERVER_LINUX_PPC_TDESC_INIT_H */
