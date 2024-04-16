/* Low-level DWARF 2 reading code

   Copyright (C) 1994-2024 Free Software Foundation, Inc.

   Adapted by Gary Funck (gary@intrepid.com), Intrepid Technology,
   Inc.  with support from Florida State University (under contract
   with the Ada Joint Program Office), and Silicon Graphics, Inc.
   Initial contribution by Brent Benson, Harris Computer Systems, Inc.,
   based on Fred Fish's (Cygnus Support) implementation of DWARF 1
   support.

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

#ifndef GDB_DWARF2_COMP_UNIT_H
#define GDB_DWARF2_COMP_UNIT_H

#include "dwarf2.h"
#include "dwarf2/leb.h"
#include "dwarf2/types.h"

struct dwarf2_per_objfile;

/* The data in a compilation unit header, after target2host
   translation, looks like this.  */
struct comp_unit_head
{
private:
  unsigned int m_length = 0;
public:
  unsigned char version = 0;
  unsigned char addr_size = 0;
  unsigned char signed_addr_p = 0;
  sect_offset abbrev_sect_off {};

  /* Size of file offsets; either 4 or 8.  */
  unsigned int offset_size = 0;

  /* Size of the length field; either 4 or 12.  */
  unsigned int initial_length_size = 0;

  enum dwarf_unit_type unit_type {};

  /* Offset to first die in this cu from the start of the cu.
     This will be the first byte following the compilation unit header.  */
  cu_offset first_die_cu_offset {};

  /* Offset to the first byte of this compilation unit header in the
     .debug_info section, for resolving relative reference dies.  */
  sect_offset sect_off {};

  /* For types, offset in the type's DIE of the type defined by this TU.  */
  cu_offset type_cu_offset_in_tu {};

  /* 64-bit signature of this unit. For type units, it denotes the signature of
     the type (DW_UT_type in DWARF 4, additionally DW_UT_split_type in DWARF 5).
     Also used in DWARF 5, to denote the dwo id when the unit type is
     DW_UT_skeleton or DW_UT_split_compile.  */
  ULONGEST signature = 0;

  void set_length (unsigned int length)
  {
    m_length = length;
  }

  /* Return the total length of the CU described by this header, including the
     initial length field.  */
  unsigned int get_length_with_initial () const
  {
    return m_length + initial_length_size;
  }

  /* Return the total length of the CU described by this header, excluding the
     initial length field.  */
  unsigned int get_length_without_initial () const
  {
    return m_length;
  }

  /* Return TRUE if OFF is within this CU.  */
  bool offset_in_cu_p (sect_offset off) const
  {
    sect_offset bottom = sect_off;
    sect_offset top = sect_off + get_length_with_initial ();
    return off >= bottom && off < top;
  }

  /* Read an offset from the data stream.  The size of the offset is
     given by cu_header->offset_size.  */
  LONGEST read_offset (bfd *abfd, const gdb_byte *buf,
		       unsigned int *bytes_read) const
  {
    LONGEST offset = ::read_offset (abfd, buf, offset_size);
    *bytes_read = offset_size;
    return offset;
  }

  /* Read an address from BUF.  BYTES_READ is updated.  */
  unrelocated_addr read_address (bfd *abfd, const gdb_byte *buf,
				 unsigned int *bytes_read) const;
};

/* Expected enum dwarf_unit_type for read_comp_unit_head.  */
enum class rcuh_kind { COMPILE, TYPE };

/* Read in the comp unit header information from the debug_info at info_ptr.
   Use rcuh_kind::COMPILE as the default type if not known by the caller.
   NOTE: This leaves members offset, first_die_offset to be filled in
   by the caller.  */
extern const gdb_byte *read_comp_unit_head
  (struct comp_unit_head *cu_header,
   const gdb_byte *info_ptr,
   struct dwarf2_section_info *section,
   rcuh_kind section_kind);

/* Read in a CU/TU header and perform some basic error checking.
   The contents of the header are stored in HEADER.
   The result is a pointer to the start of the first DIE.  */
extern const gdb_byte *read_and_check_comp_unit_head
  (dwarf2_per_objfile *per_objfile,
   struct comp_unit_head *header,
   struct dwarf2_section_info *section,
   struct dwarf2_section_info *abbrev_section,
   const gdb_byte *info_ptr,
   rcuh_kind section_kind);

#endif /* GDB_DWARF2_COMP_UNIT_H */
