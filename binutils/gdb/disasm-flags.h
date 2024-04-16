/* Disassemble flags for GDB.

   Copyright (C) 2002-2024 Free Software Foundation, Inc.

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

#ifndef DISASM_FLAGS_H
#define DISASM_FLAGS_H

#include "gdbsupport/enum-flags.h"

/* Flags used to control how GDB's disassembler behaves.  */

enum gdb_disassembly_flag : unsigned
  {
    DISASSEMBLY_SOURCE_DEPRECATED = (0x1 << 0),
    DISASSEMBLY_RAW_INSN = (0x1 << 1),
    DISASSEMBLY_OMIT_FNAME = (0x1 << 2),
    DISASSEMBLY_FILENAME = (0x1 << 3),
    DISASSEMBLY_OMIT_PC = (0x1 << 4),
    DISASSEMBLY_SOURCE = (0x1 << 5),
    DISASSEMBLY_SPECULATIVE = (0x1 << 6),
    DISASSEMBLY_RAW_BYTES = (0x1 << 7),
  };
DEF_ENUM_FLAGS_TYPE (enum gdb_disassembly_flag, gdb_disassembly_flags);

#endif /* DISASM_FLAGS_H */

