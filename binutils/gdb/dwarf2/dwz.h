/* DWARF DWZ handling for GDB.

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

#ifndef GDB_DWARF2_DWZ_H
#define GDB_DWARF2_DWZ_H

#include "gdb_bfd.h"
#include "dwarf2/index-cache.h"
#include "dwarf2/section.h"

struct dwarf2_per_bfd;

/* This represents a '.dwz' file.  */

struct dwz_file
{
  dwz_file (gdb_bfd_ref_ptr &&bfd)
    : dwz_bfd (std::move (bfd))
  {
  }

  const char *filename () const
  {
    return bfd_get_filename (this->dwz_bfd.get ());
  }

  /* A dwz file can only contain a few sections.  */
  struct dwarf2_section_info abbrev {};
  struct dwarf2_section_info info {};
  struct dwarf2_section_info str {};
  struct dwarf2_section_info line {};
  struct dwarf2_section_info macro {};
  struct dwarf2_section_info gdb_index {};
  struct dwarf2_section_info debug_names {};
  struct dwarf2_section_info types {};

  /* The dwz's BFD.  */
  gdb_bfd_ref_ptr dwz_bfd;

  /* If we loaded the index from an external file, this contains the
     resources associated to the open file, memory mapping, etc.  */
  std::unique_ptr<index_cache_resource> index_cache_res;

  /* Read a string at offset STR_OFFSET in the .debug_str section from
     this dwz file.  Throw an error if the offset is too large.  If
     the string consists of a single NUL byte, return NULL; otherwise
     return a pointer to the string.  */

  const char *read_string (struct objfile *objfile, LONGEST str_offset);
};

/* Return the separate '.dwz' debug file.  If there is no
   .gnu_debugaltlink section in the file, then the result depends on
   REQUIRE: if REQUIRE is true, then error; if REQUIRE is false,
   return NULL.  */

extern dwz_file *dwarf2_get_dwz_file (dwarf2_per_bfd *per_bfd,
				      bool require = false);

/* Open the separate '.dwz' debug file, if needed.  This just sets the
   appropriate field in the per-BFD structure.  If the DWZ file
   exists, the relevant sections are read in as well.  Throws an error
   if the .gnu_debugaltlink section exists but the file cannot be
   found.  */

extern void dwarf2_read_dwz_file (dwarf2_per_objfile *per_objfile);

#endif /* GDB_DWARF2_DWZ_H */
