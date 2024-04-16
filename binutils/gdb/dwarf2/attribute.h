/* DWARF attributes

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

#ifndef GDB_DWARF2_ATTRIBUTE_H
#define GDB_DWARF2_ATTRIBUTE_H

#include "dwarf2.h"
#include "dwarf2/types.h"
#include <optional>

/* Blocks are a bunch of untyped bytes.  */
struct dwarf_block
{
  size_t size;

  /* Valid only if SIZE is not zero.  */
  const gdb_byte *data;
};

/* Attributes have a name and a value.  */
struct attribute
{
  /* Read the given attribute value as an address, taking the
     attribute's form into account.  */
  unrelocated_addr as_address () const;

  /* If the attribute has a string form, return the string value;
     otherwise return NULL.  */
  const char *as_string () const;

  /* Return the block value.  The attribute must have block form.  */
  dwarf_block *as_block () const
  {
    gdb_assert (form_is_block ());
    return u.blk;
  }

  /* Return the signature.  The attribute must have signature
     form.  */
  ULONGEST as_signature () const
  {
    gdb_assert (form == DW_FORM_ref_sig8);
    return u.signature;
  }

  /* Return the signed value.  The attribute must have the appropriate
     form.  */
  LONGEST as_signed () const
  {
    gdb_assert (form_is_signed ());
    return u.snd;
  }

  /* Return the unsigned value, but only for attributes requiring
     reprocessing.  */
  ULONGEST as_unsigned_reprocess () const
  {
    gdb_assert (form_requires_reprocessing ());
    gdb_assert (requires_reprocessing);
    return u.unsnd;
  }

  /* Return the unsigned value.  Requires that the form be an unsigned
     form, and that reprocessing not be needed.  */
  ULONGEST as_unsigned () const
  {
    gdb_assert (form_is_unsigned ());
    gdb_assert (!requires_reprocessing);
    return u.unsnd;
  }

  /* Return true if the value is nonnegative.  Requires that that
     reprocessing not be needed.  */
  bool is_nonnegative () const
  {
    if (form_is_unsigned ())
      return true;
    if (form_is_signed ())
      return as_signed () >= 0;
    return false;
  }

  /* Return the nonnegative value.  Requires that that reprocessing not be
     needed.  */
  ULONGEST as_nonnegative () const
  {
    if (form_is_unsigned ())
      return as_unsigned ();
    if (form_is_signed ())
      return (ULONGEST)as_signed ();
    gdb_assert (false);
  }

  /* Return non-zero if ATTR's value is a section offset --- classes
     lineptr, loclistptr, macptr or rangelistptr --- or zero, otherwise.
     You may use the as_unsigned method to retrieve such offsets.

     Section 7.5.4, "Attribute Encodings", explains that no attribute
     may have a value that belongs to more than one of these classes; it
     would be ambiguous if we did, because we use the same forms for all
     of them.  */

  bool form_is_section_offset () const;

  /* Return non-zero if ATTR's value falls in the 'constant' class, or
     zero otherwise.  When this function returns true, you can apply
     the constant_value method to it.

     However, note that for some attributes you must check
     attr_form_is_section_offset before using this test.  DW_FORM_data4
     and DW_FORM_data8 are members of both the constant class, and of
     the classes that contain offsets into other debug sections
     (lineptr, loclistptr, macptr or rangelistptr).  The DWARF spec says
     that, if an attribute's can be either a constant or one of the
     section offset classes, DW_FORM_data4 and DW_FORM_data8 should be
     taken as section offsets, not constants.

     DW_FORM_data16 is not considered as constant_value cannot handle
     that.  */

  bool form_is_constant () const;

  /* The address is always stored already as sect_offset; despite for
     the forms besides DW_FORM_ref_addr it is stored as cu_offset in
     the DWARF file.  */

  bool form_is_ref () const
  {
    return (form == DW_FORM_ref_addr
	    || form == DW_FORM_ref1
	    || form == DW_FORM_ref2
	    || form == DW_FORM_ref4
	    || form == DW_FORM_ref8
	    || form == DW_FORM_ref_udata
	    || form == DW_FORM_GNU_ref_alt);
  }

  /* Check if the attribute's form is a DW_FORM_block*
     if so return true else false.  */

  bool form_is_block () const;

  /* Check if the attribute's form is a string form.  */
  bool form_is_string () const;

  /* Check if the attribute's form is an unsigned integer form.  */
  bool form_is_unsigned () const;

  /* Check if the attribute's form is a signed integer form.  */
  bool form_is_signed () const;

  /* Check if the attribute's form is a form that requires
     "reprocessing".  */
  bool form_requires_reprocessing () const;

  /* Return DIE offset of this attribute.  Return 0 with complaint if
     the attribute is not of the required kind.  */

  sect_offset get_ref_die_offset () const
  {
    if (form_is_ref ())
      return (sect_offset) u.unsnd;
    get_ref_die_offset_complaint ();
    return {};
  }

  /* Return the constant value held by this attribute.  Return
     DEFAULT_VALUE if the value held by the attribute is not
     constant.  */

  LONGEST constant_value (int default_value) const;

  /* Return true if this attribute holds a canonical string.  In some
     cases, like C++ names, gdb will rewrite the name of a DIE to a
     canonical form.  This makes lookups robust when a name can be
     spelled different ways (e.g., "signed" or "signed int").  This
     flag indicates whether the value has been canonicalized.  */
  bool canonical_string_p () const
  {
    gdb_assert (form_is_string ());
    return string_is_canonical;
  }

  /* Initialize this attribute to hold a non-canonical string
     value.  */
  void set_string_noncanonical (const char *str)
  {
    gdb_assert (form_is_string ());
    u.str = str;
    string_is_canonical = 0;
    requires_reprocessing = 0;
  }

  /* Set the canonical string value for this attribute.  */
  void set_string_canonical (const char *str)
  {
    gdb_assert (form_is_string ());
    u.str = str;
    string_is_canonical = 1;
  }

  /* Set the block value for this attribute.  */
  void set_block (dwarf_block *blk)
  {
    gdb_assert (form_is_block ());
    u.blk = blk;
  }

  /* Set the signature value for this attribute.  */
  void set_signature (ULONGEST signature)
  {
    gdb_assert (form == DW_FORM_ref_sig8);
    u.signature = signature;
  }

  /* Set this attribute to a signed integer.  */
  void set_signed (LONGEST snd)
  {
    gdb_assert (form == DW_FORM_sdata || form == DW_FORM_implicit_const);
    u.snd = snd;
  }

  /* Set this attribute to an unsigned integer.  */
  void set_unsigned (ULONGEST unsnd)
  {
    gdb_assert (form_is_unsigned ());
    u.unsnd = unsnd;
    requires_reprocessing = 0;
  }

  /* Temporarily set this attribute to an unsigned integer.  This is
     used only for those forms that require reprocessing.  */
  void set_unsigned_reprocess (ULONGEST unsnd)
  {
    gdb_assert (form_requires_reprocessing ());
    u.unsnd = unsnd;
    requires_reprocessing = 1;
  }

  /* Set this attribute to an address.  */
  void set_address (unrelocated_addr addr)
  {
    gdb_assert (form == DW_FORM_addr
		|| ((form == DW_FORM_addrx
		     || form == DW_FORM_GNU_addr_index)
		    && requires_reprocessing));
    u.addr = addr;
    requires_reprocessing = 0;
  }

  /* True if this attribute requires reprocessing.  */
  bool requires_reprocessing_p () const
  {
    return requires_reprocessing;
  }

  /* Return the value as one of the recognized enum
     dwarf_defaulted_attribute constants according to DWARF5 spec,
     Table 7.24.  If the value is incorrect, or if this attribute has
     the wrong form, then a complaint is issued and DW_DEFAULTED_no is
     returned.  */
  dwarf_defaulted_attribute defaulted () const;

  /* Return the attribute's value as a dwarf_virtuality_attribute
     constant according to DWARF spec.  An unrecognized value will
     issue a complaint and return DW_VIRTUALITY_none.  */
  dwarf_virtuality_attribute as_virtuality () const;

  /* Return the attribute's value as a boolean.  An unrecognized form
     will issue a complaint and return false.  */
  bool as_boolean () const;

  ENUM_BITFIELD(dwarf_attribute) name : 15;

  /* A boolean that is used for forms that require reprocessing.  A
     form may require data not directly available in the attribute.
     E.g., DW_FORM_strx requires the corresponding
     DW_AT_str_offsets_base.  In this case, the processing for the
     attribute must be done in two passes.  In the first past, this
     flag is set and the value is an unsigned.  In the second pass,
     the unsigned value is turned into the correct value for the form,
     and this flag is cleared.  This flag is unused for other
     forms.  */
  unsigned int requires_reprocessing : 1;

  ENUM_BITFIELD(dwarf_form) form : 15;

  /* Has u.str already been updated by dwarf2_canonicalize_name?  This
     field should be in u.str but it is kept here for better struct
     attribute alignment.  */
  unsigned int string_is_canonical : 1;

  union
    {
      const char *str;
      struct dwarf_block *blk;
      ULONGEST unsnd;
      LONGEST snd;
      unrelocated_addr addr;
      ULONGEST signature;
    }
  u;

private:

  /* Used by get_ref_die_offset to issue a complaint.  */

  void get_ref_die_offset_complaint () const;
};

#endif /* GDB_DWARF2_ATTRIBUTE_H */
