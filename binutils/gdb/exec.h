/* Work with executable files, for GDB, the GNU debugger.

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

#ifndef EXEC_H
#define EXEC_H

#include "target.h"
#include "progspace.h"
#include "memrange.h"
#include "symfile-add-flags.h"

struct target_section;
struct target_ops;
struct bfd;
struct objfile;

/* Builds a section table, given args BFD.  */

extern std::vector<target_section> build_section_table (struct bfd *);

/* VFORK_CHILD is a child vforked and its program space is shared with its
   parent.  This pushes the exec target on that inferior's target stack if
   there are sections in the program space's section table.  */

extern void exec_on_vfork (inferior *vfork_child);

/* Read from mappable read-only sections of BFD executable files.
   Return TARGET_XFER_OK, if read is successful.  Return
   TARGET_XFER_EOF if read is done.  Return TARGET_XFER_E_IO
   otherwise.  */

extern enum target_xfer_status
  exec_read_partial_read_only (gdb_byte *readbuf, ULONGEST offset,
			       ULONGEST len, ULONGEST *xfered_len);

/* Read or write from mappable sections of BFD executable files.

   Request to transfer up to LEN 8-bit bytes of the target sections
   defined by SECTIONS and SECTIONS_END.  The OFFSET specifies the
   starting address.

   The MATCH_CB predicate is optional; when provided it will be called
   for each section under consideration.  When MATCH_CB evaluates as
   true, the section remains under consideration; a false result
   removes it from consideration for performing the memory transfers
   noted above.  See memory_xfer_partial_1() in target.c for an
   example.

   Return the number of bytes actually transfered, or zero when no
   data is available for the requested range.

   This function is intended to be used from target_xfer_partial
   implementations.  See target_read and target_write for more
   information.

   One, and only one, of readbuf or writebuf must be non-NULL.  */

extern enum target_xfer_status
  section_table_xfer_memory_partial (gdb_byte *,
				     const gdb_byte *,
				     ULONGEST, ULONGEST, ULONGEST *,
				     const std::vector<target_section> &,
				     gdb::function_view<bool
				       (const struct target_section *)> match_cb
					 = nullptr);

/* Read from mappable read-only sections of BFD executable files.
   Similar to exec_read_partial_read_only, but return
   TARGET_XFER_UNAVAILABLE if data is unavailable.  */

extern enum target_xfer_status
  section_table_read_available_memory (gdb_byte *readbuf, ULONGEST offset,
				       ULONGEST len, ULONGEST *xfered_len);

/* Set the loaded address of a section.  */
extern void exec_set_section_address (const char *, int, CORE_ADDR);

/* Prints info about all sections defined in the TABLE.  ABFD is
   special cased --- it's filename is omitted; if it is the executable
   file, its entry point is printed.  */

extern void print_section_info (const std::vector<target_section> *table,
				bfd *abfd);

/* Helper function that attempts to open the symbol file at EXEC_FILE_HOST.
   If successful, it proceeds to add the symbol file as the main symbol file.

   ADD_FLAGS is passed on to the function adding the symbol file.  */
extern void try_open_exec_file (const char *exec_file_host,
				struct inferior *inf,
				symfile_add_flags add_flags);
#endif
