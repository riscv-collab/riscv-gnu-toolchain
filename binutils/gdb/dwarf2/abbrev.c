/* DWARF 2 abbreviations

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

#include "defs.h"
#include "dwarf2/read.h"
#include "dwarf2/abbrev.h"
#include "dwarf2/leb.h"
#include "bfd.h"

/* Hash function for an abbrev.  */

static hashval_t
hash_abbrev (const void *item)
{
  const struct abbrev_info *info = (const struct abbrev_info *) item;
  /* Warning: if you change this next line, you must also update the
     other code in this class using the _with_hash functions.  */
  return info->number;
}

/* Comparison function for abbrevs.  */

static int
eq_abbrev (const void *lhs, const void *rhs)
{
  const struct abbrev_info *l_info = (const struct abbrev_info *) lhs;
  const struct abbrev_info *r_info = (const struct abbrev_info *) rhs;
  return l_info->number == r_info->number;
}

/* Abbreviation tables.

   In DWARF version 2, the description of the debugging information is
   stored in a separate .debug_abbrev section.  Before we read any
   dies from a section we read in all abbreviations and install them
   in a hash table.  */

abbrev_table::abbrev_table (sect_offset off, struct dwarf2_section_info *sect)
  : sect_off (off),
    section (sect),
    m_abbrevs (htab_create_alloc (20, hash_abbrev, eq_abbrev,
				  nullptr, xcalloc, xfree))
{
}

/* Add an abbreviation to the table.  */

void
abbrev_table::add_abbrev (struct abbrev_info *abbrev)
{
  void **slot = htab_find_slot_with_hash (m_abbrevs.get (), abbrev,
					  abbrev->number, INSERT);
  *slot = abbrev;
}

/* Helper function that returns true if a DIE with the given tag might
   plausibly be indexed.  */

static bool
tag_interesting_for_index (dwarf_tag tag)
{
  switch (tag)
    {
    case DW_TAG_array_type:
    case DW_TAG_base_type:
    case DW_TAG_class_type:
    case DW_TAG_constant:
    case DW_TAG_entry_point:
    case DW_TAG_enumeration_type:
    case DW_TAG_enumerator:
    case DW_TAG_imported_declaration:
    case DW_TAG_imported_unit:
    case DW_TAG_inlined_subroutine:
    case DW_TAG_interface_type:
    case DW_TAG_module:
    case DW_TAG_namespace:
    case DW_TAG_ptr_to_member_type:
    case DW_TAG_set_type:
    case DW_TAG_string_type:
    case DW_TAG_structure_type:
    case DW_TAG_subprogram:
    case DW_TAG_subrange_type:
    case DW_TAG_generic_subrange:
    case DW_TAG_subroutine_type:
    case DW_TAG_typedef:
    case DW_TAG_union_type:
    case DW_TAG_unspecified_type:
    case DW_TAG_variable:
      return true;
    }

  return false;
}

/* Read in an abbrev table.  */

abbrev_table_up
abbrev_table::read (struct dwarf2_section_info *section,
		    sect_offset sect_off)
{
  bfd *abfd = section->get_bfd_owner ();
  const gdb_byte *abbrev_ptr;
  struct abbrev_info *cur_abbrev;

  abbrev_table_up abbrev_table (new struct abbrev_table (sect_off, section));
  struct obstack *obstack = &abbrev_table->m_abbrev_obstack;

  /* Caller must ensure this.  */
  gdb_assert (section->readin);
  abbrev_ptr = section->buffer + to_underlying (sect_off);

  while (true)
    {
      unsigned int bytes_read;
      /* Loop until we reach an abbrev number of 0.  */
      unsigned int abbrev_number = read_unsigned_leb128 (abfd, abbrev_ptr,
							 &bytes_read);
      if (abbrev_number == 0)
	break;
      abbrev_ptr += bytes_read;

      /* Start without any attrs.  */
      obstack_blank (obstack, offsetof (abbrev_info, attrs));
      cur_abbrev = (struct abbrev_info *) obstack_base (obstack);

      /* Read in abbrev header.  */
      cur_abbrev->number = abbrev_number;
      cur_abbrev->tag
	= (enum dwarf_tag) read_unsigned_leb128 (abfd, abbrev_ptr,
						 &bytes_read);
      abbrev_ptr += bytes_read;
      cur_abbrev->has_children = read_1_byte (abfd, abbrev_ptr);
      abbrev_ptr += 1;

      unsigned int size = 0;
      unsigned int sibling_offset = -1;
      bool is_csize = true;

      bool has_hardcoded_declaration = false;
      bool has_specification_or_origin = false;
      bool has_name = false;
      bool has_linkage_name = false;
      bool has_external = false;

      /* Now read in declarations.  */
      int num_attrs = 0;
      for (;;)
	{
	  struct attr_abbrev cur_attr;

	  cur_attr.name
	    = (enum dwarf_attribute) read_unsigned_leb128 (abfd, abbrev_ptr,
							   &bytes_read);
	  abbrev_ptr += bytes_read;
	  cur_attr.form
	    = (enum dwarf_form) read_unsigned_leb128 (abfd, abbrev_ptr,
						      &bytes_read);
	  abbrev_ptr += bytes_read;
	  if (cur_attr.form == DW_FORM_implicit_const)
	    {
	      cur_attr.implicit_const = read_signed_leb128 (abfd, abbrev_ptr,
							    &bytes_read);
	      abbrev_ptr += bytes_read;
	    }
	  else
	    cur_attr.implicit_const = -1;

	  if (cur_attr.name == 0)
	    break;

	  switch (cur_attr.name)
	    {
	    case DW_AT_declaration:
	      if (cur_attr.form == DW_FORM_flag_present)
		has_hardcoded_declaration = true;
	      break;

	    case DW_AT_external:
	      has_external = true;
	      break;

	    case DW_AT_specification:
	    case DW_AT_abstract_origin:
	    case DW_AT_extension:
	      has_specification_or_origin = true;
	      break;

	    case DW_AT_name:
	      has_name = true;
	      break;

	    case DW_AT_MIPS_linkage_name:
	    case DW_AT_linkage_name:
	      has_linkage_name = true;
	      break;

	    case DW_AT_sibling:
	      if (is_csize && cur_attr.form == DW_FORM_ref4)
		sibling_offset = size;
	      break;
	    }

	  switch (cur_attr.form)
	    {
	    case DW_FORM_data1:
	    case DW_FORM_ref1:
	    case DW_FORM_flag:
	    case DW_FORM_strx1:
	      size += 1;
	      break;
	    case DW_FORM_flag_present:
	    case DW_FORM_implicit_const:
	      break;
	    case DW_FORM_data2:
	    case DW_FORM_ref2:
	    case DW_FORM_strx2:
	      size += 2;
	      break;
	    case DW_FORM_strx3:
	      size += 3;
	      break;
	    case DW_FORM_data4:
	    case DW_FORM_ref4:
	    case DW_FORM_strx4:
	      size += 4;
	      break;
	    case DW_FORM_data8:
	    case DW_FORM_ref8:
	    case DW_FORM_ref_sig8:
	      size += 8;
	      break;
	    case DW_FORM_data16:
	      size += 16;
	      break;

	    default:
	      is_csize = false;
	      break;
	    }

	  ++num_attrs;
	  obstack_grow (obstack, &cur_attr, sizeof (cur_attr));
	}

      cur_abbrev = (struct abbrev_info *) obstack_finish (obstack);
      cur_abbrev->num_attrs = num_attrs;

      if (!has_name && !has_linkage_name && !has_specification_or_origin)
	{
	  /* Some anonymous DIEs are worth examining.  */
	  cur_abbrev->interesting
	    = (cur_abbrev->tag == DW_TAG_namespace
	       || cur_abbrev->tag == DW_TAG_enumeration_type);
	}
      else if ((cur_abbrev->tag == DW_TAG_structure_type
		|| cur_abbrev->tag == DW_TAG_class_type
		|| cur_abbrev->tag == DW_TAG_union_type)
	       && cur_abbrev->has_children)
	{
	  /* We have to record this as interesting, regardless of how
	     DW_AT_declaration is set, so that any subsequent
	     DW_AT_specification pointing at a child of this will get
	     the correct scope.  */
	  cur_abbrev->interesting = true;
	}
      else if (has_hardcoded_declaration
	       && (cur_abbrev->tag != DW_TAG_variable || !has_external))
	cur_abbrev->interesting = false;
      else if (!tag_interesting_for_index (cur_abbrev->tag))
	cur_abbrev->interesting = false;
      else
	cur_abbrev->interesting = true;

      /* If there are no children, and the abbrev has a constant size,
	 then we don't care about the sibling offset, because it's
	 simple to just skip the entire DIE without reading a sibling
	 offset.  */
      if ((!cur_abbrev->has_children && is_csize)
	  /* Overflow.  */
	  || sibling_offset != (unsigned short) sibling_offset)
	sibling_offset = -1;
      cur_abbrev->size_if_constant = is_csize ? size : 0;
      cur_abbrev->sibling_offset = sibling_offset;

      abbrev_table->add_abbrev (cur_abbrev);
    }

  return abbrev_table;
}
