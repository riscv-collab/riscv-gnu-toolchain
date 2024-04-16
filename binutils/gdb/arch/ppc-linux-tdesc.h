/* Target description declarations shared between gdb, gdbserver and IPA.

   Copyright (C) 2018-2024 Free Software Foundation, Inc.

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

#ifndef ARCH_PPC_LINUX_TDESC_H
#define ARCH_PPC_LINUX_TDESC_H

struct target_desc;

extern const struct target_desc *tdesc_powerpc_32l;
extern const struct target_desc *tdesc_powerpc_altivec32l;
extern const struct target_desc *tdesc_powerpc_vsx32l;
extern const struct target_desc *tdesc_powerpc_isa205_32l;
extern const struct target_desc *tdesc_powerpc_isa205_altivec32l;
extern const struct target_desc *tdesc_powerpc_isa205_vsx32l;
extern const struct target_desc *tdesc_powerpc_isa205_ppr_dscr_vsx32l;
extern const struct target_desc *tdesc_powerpc_isa207_vsx32l;
extern const struct target_desc *tdesc_powerpc_isa207_htm_vsx32l;
extern const struct target_desc *tdesc_powerpc_e500l;

extern const struct target_desc *tdesc_powerpc_64l;
extern const struct target_desc *tdesc_powerpc_altivec64l;
extern const struct target_desc *tdesc_powerpc_vsx64l;
extern const struct target_desc *tdesc_powerpc_isa205_64l;
extern const struct target_desc *tdesc_powerpc_isa205_altivec64l;
extern const struct target_desc *tdesc_powerpc_isa205_vsx64l;
extern const struct target_desc *tdesc_powerpc_isa205_ppr_dscr_vsx64l;
extern const struct target_desc *tdesc_powerpc_isa207_vsx64l;
extern const struct target_desc *tdesc_powerpc_isa207_htm_vsx64l;

#endif /* ARCH_PPC_LINUX_TDESC_H */
