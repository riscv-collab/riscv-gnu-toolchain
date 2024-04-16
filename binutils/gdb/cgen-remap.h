/* Copyright (C) 2023-2024 Free Software Foundation, Inc.

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

#ifndef CGEN_REMAP_H
#define CGEN_REMAP_H

/* Remap cgen interface names, so we can have multiple cgen generated include
   files in gdb without violating c++ ODR.  */

/* Define GDB_CGEN_REMAP_PREFIX to the desired remapping prefix before
   including this file.  */
#ifndef GDB_CGEN_REMAP_PREFIX
# error "GDB_CGEN_REMAP_PREFIX not defined"
#endif

#define GDB_CGEN_REMAP_2(PREFIX, SYM) PREFIX ## _ ## SYM
#define GDB_CGEN_REMAP_1(PREFIX, SYM) GDB_CGEN_REMAP_2 (PREFIX, SYM)
#define GDB_CGEN_REMAP(SYM) GDB_CGEN_REMAP_1 (GDB_CGEN_REMAP_PREFIX, SYM)

#define cgen_operand_type	GDB_CGEN_REMAP (cgen_operand_type)
#define cgen_hw_type		GDB_CGEN_REMAP (cgen_hw_type)
#define cgen_ifld		GDB_CGEN_REMAP (cgen_ifld)
#define cgen_insn		GDB_CGEN_REMAP (cgen_insn)
#define cgen_cpu_desc		GDB_CGEN_REMAP (cgen_cpu_desc)
#define cgen_fields		GDB_CGEN_REMAP (cgen_fields)
#define cgen_insn_list		GDB_CGEN_REMAP (cgen_insn_list)
#define cgen_maybe_multi_ifield	GDB_CGEN_REMAP (cgen_maybe_multi_ifield)
#define CGEN_OPINST             GDB_CGEN_REMAP (CGEN_OPINST)
#define CGEN_IFMT_IFLD		GDB_CGEN_REMAP (CGEN_IFMT_IFLD)
#define CGEN_INSN_ATTR_TYPE	GDB_CGEN_REMAP (CGEN_INSN_ATTR_TYPE)
#define CGEN_IBASE		GDB_CGEN_REMAP (CGEN_IBASE)
#define CGEN_HW_ENTRY		GDB_CGEN_REMAP (CGEN_HW_ENTRY)
#define CGEN_HW_TABLE		GDB_CGEN_REMAP (CGEN_HW_TABLE)
#define CGEN_INSN_TABLE		GDB_CGEN_REMAP (CGEN_INSN_TABLE)
#define CGEN_OPERAND_TABLE	GDB_CGEN_REMAP (CGEN_OPERAND_TABLE)
#define CGEN_OPERAND		GDB_CGEN_REMAP (CGEN_OPERAND)
#define CGEN_MAYBE_MULTI_IFLD	GDB_CGEN_REMAP (CGEN_MAYBE_MULTI_IFLD)

#endif /* CGEN_REMAP_H */
