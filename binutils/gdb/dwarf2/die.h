/* DWARF DIEs

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

#ifndef GDB_DWARF2_DIE_H
#define GDB_DWARF2_DIE_H

#include "complaints.h"
#include "dwarf2/attribute.h"

/* This data structure holds a complete die structure.  */
struct die_info
{
  /* Allocate a new die_info on OBSTACK.  NUM_ATTRS is the number of
     attributes that are needed.  */
  static die_info *allocate (struct obstack *obstack, int num_attrs);

  /* Trivial hash function for die_info: the hash value of a DIE is
     its offset in .debug_info for this objfile.  */
  static hashval_t hash (const void *item);

  /* Trivial comparison function for die_info structures: two DIEs
     are equal if they have the same offset.  */
  static int eq (const void *item_lhs, const void *item_rhs);

  /* Dump this DIE and any children to MAX_LEVEL.  They are written to
     gdb_stdlog.  Note this is called from the pdie user command in
     gdb-gdb.gdb.  */
  void dump (int max_level);

  /* Shallowly dump this DIE to gdb_stderr.  */
  void error_dump ();

  /* Return the named attribute or NULL if not there, but do not
     follow DW_AT_specification, etc.  */
  struct attribute *attr (dwarf_attribute name)
  {
    for (unsigned i = 0; i < num_attrs; ++i)
      if (attrs[i].name == name)
	return &attrs[i];
    return NULL;
  }

  /* Return the address base of the compile unit, which, if exists, is
     stored either at the attribute DW_AT_GNU_addr_base, or
     DW_AT_addr_base.  */
  std::optional<ULONGEST> addr_base ()
  {
    for (unsigned i = 0; i < num_attrs; ++i)
      if (attrs[i].name == DW_AT_addr_base
	   || attrs[i].name == DW_AT_GNU_addr_base)
	{
	  if (attrs[i].form_is_unsigned ())
	    {
	      /* If both exist, just use the first one.  */
	      return attrs[i].as_unsigned ();
	    }
	  complaint (_("address base attribute (offset %s) as wrong form"),
		     sect_offset_str (sect_off));
	}
    return std::optional<ULONGEST> ();
  }

  /* Return the base address of the compile unit into the .debug_ranges section,
     which, if exists, is stored in the DW_AT_GNU_ranges_base attribute.  This
     value is only relevant in pre-DWARF 5 split-unit scenarios.  */
  ULONGEST gnu_ranges_base ()
  {
    for (unsigned i = 0; i < num_attrs; ++i)
      if (attrs[i].name == DW_AT_GNU_ranges_base)
	{
	  if (attrs[i].form_is_unsigned ())
	    return attrs[i].as_unsigned ();

	  complaint (_("ranges base attribute (offset %s) has wrong form"),
		     sect_offset_str (sect_off));
	}

    return 0;
  }

  /* Return the rnglists base of the compile unit, which, if exists, is stored
     in the DW_AT_rnglists_base attribute.  */
  ULONGEST rnglists_base ()
  {
    for (unsigned i = 0; i < num_attrs; ++i)
      if (attrs[i].name == DW_AT_rnglists_base)
	{
	  if (attrs[i].form_is_unsigned ())
	    return attrs[i].as_unsigned ();

	  complaint (_("rnglists base attribute (offset %s) has wrong form"),
		     sect_offset_str (sect_off));
	}

    return 0;
  }

  /* DWARF-2 tag for this DIE.  */
  ENUM_BITFIELD(dwarf_tag) tag : 16;

  /* Number of attributes */
  unsigned char num_attrs;

  /* True if we're presently building the full type name for the
     type derived from this DIE.  */
  unsigned char building_fullname : 1;

  /* True if this die is in process.  PR 16581.  */
  unsigned char in_process : 1;

  /* True if this DIE has children.  */
  unsigned char has_children : 1;

  /* Abbrev number */
  unsigned int abbrev;

  /* Offset in .debug_info or .debug_types section.  */
  sect_offset sect_off;

  /* The dies in a compilation unit form an n-ary tree.  PARENT
     points to this die's parent; CHILD points to the first child of
     this node; and all the children of a given node are chained
     together via their SIBLING fields.  */
  struct die_info *child;	/* Its first child, if any.  */
  struct die_info *sibling;	/* Its next sibling, if any.  */
  struct die_info *parent;	/* Its parent, if any.  */

  /* An array of attributes, with NUM_ATTRS elements.  There may be
     zero, but it's not common and zero-sized arrays are not
     sufficiently portable C.  */
  struct attribute attrs[1];
};

#endif /* GDB_DWARF2_DIE_H */
