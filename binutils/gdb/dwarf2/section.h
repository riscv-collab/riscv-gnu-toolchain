/* DWARF 2 low-level section code

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

#ifndef GDB_DWARF2_SECTION_H
#define GDB_DWARF2_SECTION_H

/* A descriptor for dwarf sections.

   S.ASECTION, SIZE are typically initialized when the objfile is first
   scanned.  BUFFER, READIN are filled in later when the section is read.
   If the section contained compressed data then SIZE is updated to record
   the uncompressed size of the section.

   DWP file format V2 introduces a wrinkle that is easiest to handle by
   creating the concept of virtual sections contained within a real section.
   In DWP V2 the sections of the input DWO files are concatenated together
   into one section, but section offsets are kept relative to the original
   input section.
   If this is a virtual dwp-v2 section, S.CONTAINING_SECTION is a backlink to
   the real section this "virtual" section is contained in, and BUFFER,SIZE
   describe the virtual section.  */

struct dwarf2_section_info
{
  /* Return the name of this section.  */
  const char *get_name () const;

  /* Return the containing section of this section, which must be a
     virtual section.  */
  struct dwarf2_section_info *get_containing_section () const;

  /* Return the bfd owner of this section.  */
  struct bfd *get_bfd_owner () const;

  /* Return the bfd section of this section.
     Returns NULL if the section is not present.  */
  asection *get_bfd_section () const;

  /* Return the name of the file this section is in.  */
  const char *get_file_name () const;

  /* Return the id of this section.
     Returns 0 if this section doesn't exist.  */
  int get_id () const;

  /* Return the flags of this section.  This section (or containing
     section if this is a virtual section) must exist.  */
  int get_flags () const;

  /* Return true if this section does not exist or if it has no
     contents. */
  bool empty () const;

  /* Read the contents of this section.
     OBJFILE is the main object file, but not necessarily the file where
     the section comes from.  E.g., for DWO files the bfd of INFO is the bfd
     of the DWO file.
     If the section is compressed, uncompress it before returning.  */
  void read (struct objfile *objfile);

  /* A helper function that returns the size of a section in a safe way.
     If you are positive that the section has been read before using the
     size, then it is safe to refer to the dwarf2_section_info object's
     "size" field directly.  In other cases, you must call this
     function, because for compressed sections the size field is not set
     correctly until the section has been read.  */
  bfd_size_type get_size (struct objfile *objfile)
  {
    if (!readin)
      read (objfile);
    return size;
  }

  /* Issue a complaint that something was outside the bounds of this
     buffer.  */
  void overflow_complaint () const;

  /* Return pointer to string in this section at offset STR_OFFSET
     with error reporting string FORM_NAME.  */
  const char *read_string (struct objfile *objfile, LONGEST str_offset,
			   const char *form_name);

  union
  {
    /* If this is a real section, the bfd section.  */
    asection *section;
    /* If this is a virtual section, pointer to the containing ("real")
       section.  */
    struct dwarf2_section_info *containing_section;
  } s;
  /* Pointer to section data, only valid if readin.  */
  const gdb_byte *buffer;
  /* The size of the section, real or virtual.  */
  bfd_size_type size;
  /* If this is a virtual section, the offset in the real section.
     Only valid if is_virtual.  */
  bfd_size_type virtual_offset;
  /* True if we have tried to read this section.  */
  bool readin;
  /* True if this is a virtual section, False otherwise.
     This specifies which of s.section and s.containing_section to use.  */
  bool is_virtual;
};

#endif /* GDB_DWARF2_SECTION_H */
