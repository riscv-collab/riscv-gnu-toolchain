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

#include "defs.h"
#include "dwarf2/attribute.h"
#include "dwarf2/stringify.h"
#include "complaints.h"

/* See attribute.h.  */

unrelocated_addr
attribute::as_address () const
{
  unrelocated_addr addr;

  gdb_assert (!requires_reprocessing);

  if (form != DW_FORM_addr && form != DW_FORM_addrx
      && form != DW_FORM_GNU_addr_index)
    {
      /* Aside from a few clearly defined exceptions, attributes that
	 contain an address must always be in DW_FORM_addr form.
	 Unfortunately, some compilers happen to be violating this
	 requirement by encoding addresses using other forms, such
	 as DW_FORM_data4 for example.  For those broken compilers,
	 we try to do our best, without any guarantee of success,
	 to interpret the address correctly.  It would also be nice
	 to generate a complaint, but that would require us to maintain
	 a list of legitimate cases where a non-address form is allowed,
	 as well as update callers to pass in at least the CU's DWARF
	 version.  This is more overhead than what we're willing to
	 expand for a pretty rare case.  */
      addr = (unrelocated_addr) u.unsnd;
    }
  else
    addr = u.addr;

  return addr;
}

/* See attribute.h.  */

bool
attribute::form_is_string () const
{
  return (form == DW_FORM_strp || form == DW_FORM_line_strp
	  || form == DW_FORM_string
	  || form == DW_FORM_strx
	  || form == DW_FORM_strx1
	  || form == DW_FORM_strx2
	  || form == DW_FORM_strx3
	  || form == DW_FORM_strx4
	  || form == DW_FORM_GNU_str_index
	  || form == DW_FORM_GNU_strp_alt);
}

/* See attribute.h.  */

const char *
attribute::as_string () const
{
  gdb_assert (!requires_reprocessing);
  if (form_is_string ())
    return u.str;
  return nullptr;
}

/* See attribute.h.  */

bool
attribute::form_is_block () const
{
  return (form == DW_FORM_block1
	  || form == DW_FORM_block2
	  || form == DW_FORM_block4
	  || form == DW_FORM_block
	  || form == DW_FORM_exprloc
	  || form == DW_FORM_data16);
}

/* See attribute.h.  */

bool
attribute::form_is_section_offset () const
{
  return (form == DW_FORM_data4
	  || form == DW_FORM_data8
	  || form == DW_FORM_sec_offset
	  || form == DW_FORM_loclistx);
}

/* See attribute.h.  */

bool
attribute::form_is_constant () const
{
  switch (form)
    {
    case DW_FORM_sdata:
    case DW_FORM_udata:
    case DW_FORM_data1:
    case DW_FORM_data2:
    case DW_FORM_data4:
    case DW_FORM_data8:
    case DW_FORM_implicit_const:
      return true;
    default:
      return false;
    }
}

/* See attribute.h.  */

void
attribute::get_ref_die_offset_complaint () const
{
  complaint (_("unsupported die ref attribute form: '%s'"),
	     dwarf_form_name (form));
}

/* See attribute.h.  */

LONGEST
attribute::constant_value (int default_value) const
{
  if (form == DW_FORM_sdata || form == DW_FORM_implicit_const)
    return u.snd;
  else if (form == DW_FORM_udata
	   || form == DW_FORM_data1
	   || form == DW_FORM_data2
	   || form == DW_FORM_data4
	   || form == DW_FORM_data8)
    return u.unsnd;
  else
    {
      /* For DW_FORM_data16 see attribute::form_is_constant.  */
      complaint (_("Attribute value is not a constant (%s)"),
		 dwarf_form_name (form));
      return default_value;
    }
}

/* See attribute.h.  */

bool
attribute::form_is_unsigned () const
{
  return (form == DW_FORM_ref_addr
	  || form == DW_FORM_GNU_ref_alt
	  || form == DW_FORM_data2
	  || form == DW_FORM_data4
	  || form == DW_FORM_data8
	  || form == DW_FORM_sec_offset
	  || form == DW_FORM_data1
	  || form == DW_FORM_flag
	  || form == DW_FORM_flag_present
	  || form == DW_FORM_udata
	  || form == DW_FORM_rnglistx
	  || form == DW_FORM_loclistx
	  || form == DW_FORM_ref1
	  || form == DW_FORM_ref2
	  || form == DW_FORM_ref4
	  || form == DW_FORM_ref8
	  || form == DW_FORM_ref_udata);
}

/* See attribute.h.  */

bool
attribute::form_is_signed () const
{
  return form == DW_FORM_sdata || form == DW_FORM_implicit_const;
}

/* See attribute.h.  */

bool
attribute::form_requires_reprocessing () const
{
  return (form == DW_FORM_strx
	  || form == DW_FORM_strx1
	  || form == DW_FORM_strx2
	  || form == DW_FORM_strx3
	  || form == DW_FORM_strx4
	  || form == DW_FORM_GNU_str_index
	  || form == DW_FORM_addrx
	  || form == DW_FORM_GNU_addr_index
	  || form == DW_FORM_rnglistx
	  || form == DW_FORM_loclistx);
}

/* See attribute.h.  */

dwarf_defaulted_attribute
attribute::defaulted () const
{
  LONGEST value = constant_value (-1);

  switch (value)
    {
    case DW_DEFAULTED_no:
    case DW_DEFAULTED_in_class:
    case DW_DEFAULTED_out_of_class:
      return (dwarf_defaulted_attribute) value;
    }

  /* If the form was not constant, we already complained in
     constant_value, so there's no need to complain again.  */
  if (form_is_constant ())
    complaint (_("unrecognized DW_AT_defaulted value (%s)"),
	       plongest (value));
  return DW_DEFAULTED_no;
}

/* See attribute.h.  */

dwarf_virtuality_attribute
attribute::as_virtuality () const
{
  LONGEST value = constant_value (-1);

  switch (value)
    {
    case DW_VIRTUALITY_none:
    case DW_VIRTUALITY_virtual:
    case DW_VIRTUALITY_pure_virtual:
      return (dwarf_virtuality_attribute) value;
    }

  /* If the form was not constant, we already complained in
     constant_value, so there's no need to complain again.  */
  if (form_is_constant ())
    complaint (_("unrecognized DW_AT_virtuality value (%s)"),
	       plongest (value));
  return DW_VIRTUALITY_none;
}

/* See attribute.h.  */

bool
attribute::as_boolean () const
{
  if (form == DW_FORM_flag_present)
    return true;
  else if (form == DW_FORM_flag)
    return u.unsnd != 0;
  return constant_value (0) != 0;
}
