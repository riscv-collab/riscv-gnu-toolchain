/* DWARF 2 section names.

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

#ifndef GDB_DWARF2_SECT_NAMES_H
#define GDB_DWARF2_SECT_NAMES_H

/* Names for a dwarf2 debugging section.  The field NORMAL is the normal
   section name (usually from the DWARF standard), while the field COMPRESSED
   is the name of compressed sections.  If your object file format doesn't
   support compressed sections, the field COMPRESSED can be NULL.  Likewise,
   the debugging section is not supported, the field NORMAL can be NULL too.
   It doesn't make sense to have a NULL NORMAL field but a non-NULL COMPRESSED
   field.  */

struct dwarf2_section_names {
  const char *normal;
  const char *compressed;

  /* Return true if NAME matches either of this section's names.  */
  bool matches (const char *name) const
  {
    return ((normal != nullptr && strcmp (name, normal) == 0)
	    || (compressed != nullptr && strcmp (name, compressed) == 0));
  }
};

/* List of names for dward2 debugging sections.  Also most object file formats
   use the standardized (ie ELF) names, some (eg XCOFF) have customized names
   due to restrictions.
   The table for the standard names is defined in dwarf2read.c.  Please
   update all instances of dwarf2_debug_sections if you add a field to this
   structure.  It is always safe to use { NULL, NULL } in this case.  */

struct dwarf2_debug_sections {
  struct dwarf2_section_names info;
  struct dwarf2_section_names abbrev;
  struct dwarf2_section_names line;
  struct dwarf2_section_names loc;
  struct dwarf2_section_names loclists;
  struct dwarf2_section_names macinfo;
  struct dwarf2_section_names macro;
  struct dwarf2_section_names str;
  struct dwarf2_section_names str_offsets;
  struct dwarf2_section_names line_str;
  struct dwarf2_section_names ranges;
  struct dwarf2_section_names rnglists;
  struct dwarf2_section_names types;
  struct dwarf2_section_names addr;
  struct dwarf2_section_names frame;
  struct dwarf2_section_names eh_frame;
  struct dwarf2_section_names gdb_index;
  struct dwarf2_section_names debug_names;
  struct dwarf2_section_names debug_aranges;
  /* This field has no meaning, but exists solely to catch changes to
     this structure which are not reflected in some instance.  */
  int sentinel;
};

/* Section names for ELF.  */
extern const struct dwarf2_debug_sections dwarf2_elf_names;

#endif /* GDB_DWARF2_SECT_NAMES_H */
