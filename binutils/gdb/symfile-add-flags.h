/* Definition of symfile add flags.

   Copyright (C) 1990-2024 Free Software Foundation, Inc.

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

#if !defined (SYMFILE_ADD_FLAGS_H)
#define SYMFILE_ADD_FLAGS_H

#include "gdbsupport/enum-flags.h"

/* This enum encodes bit-flags passed as ADD_FLAGS parameter to
   symbol_file_add, etc.  Defined in a separate file to break circular
   header dependencies.  */

enum symfile_add_flag : unsigned
  {
    /* Be chatty about what you are doing.  */
    SYMFILE_VERBOSE = 1 << 1,

    /* This is the main symbol file (as opposed to symbol file for
       dynamically loaded code).  */
    SYMFILE_MAINLINE = 1 << 2,

    /* Do not call breakpoint_re_set when adding this symbol file.  */
    SYMFILE_DEFER_BP_RESET = 1 << 3,

    /* Do not immediately read symbols for this file.  By default,
       symbols are read when the objfile is created.  */
    SYMFILE_NO_READ = 1 << 4,

    /* The new objfile should be marked OBJF_NOT_FILENAME.  */
    SYMFILE_NOT_FILENAME = 1 << 5,

    /* If SYMFILE_VERBOSE (interpreted as from_tty) and SYMFILE_ALWAYS_CONFIRM,
       always ask user to confirm loading the symbol file.
       Without this flag, symbol_file_add_with_addrs asks a confirmation only
       for a main symbol file replacing a file having symbols.  */
    SYMFILE_ALWAYS_CONFIRM = 1 << 6,
 };

DEF_ENUM_FLAGS_TYPE (enum symfile_add_flag, symfile_add_flags);

#endif /* !defined(SYMFILE_ADD_FLAGS_H) */
