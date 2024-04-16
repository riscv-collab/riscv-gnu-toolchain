/* DWARF stringify code

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
#include "dwarf2.h"
#include "dwarf2/stringify.h"

/* A convenience function that returns an "unknown" DWARF name,
   including the value of V.  STR is the name of the entity being
   printed, e.g., "TAG".  */

static const char *
dwarf_unknown (const char *str, unsigned v)
{
  char *cell = get_print_cell ();
  xsnprintf (cell, PRINT_CELL_SIZE, "DW_%s_<unknown: %u>", str, v);
  return cell;
}

/* See stringify.h.  */

const char *
dwarf_tag_name (unsigned tag)
{
  const char *name = get_DW_TAG_name (tag);

  if (name == NULL)
    return dwarf_unknown ("TAG", tag);

  return name;
}

/* See stringify.h.  */

const char *
dwarf_attr_name (unsigned attr)
{
  const char *name;

#ifdef MIPS /* collides with DW_AT_HP_block_index */
  if (attr == DW_AT_MIPS_fde)
    return "DW_AT_MIPS_fde";
#else
  if (attr == DW_AT_HP_block_index)
    return "DW_AT_HP_block_index";
#endif

  name = get_DW_AT_name (attr);

  if (name == NULL)
    return dwarf_unknown ("AT", attr);

  return name;
}

/* See stringify.h.  */

const char *
dwarf_form_name (unsigned form)
{
  const char *name = get_DW_FORM_name (form);

  if (name == NULL)
    return dwarf_unknown ("FORM", form);

  return name;
}

/* See stringify.h.  */

const char *
dwarf_bool_name (unsigned mybool)
{
  if (mybool)
    return "TRUE";
  else
    return "FALSE";
}

/* See stringify.h.  */

const char *
dwarf_type_encoding_name (unsigned enc)
{
  const char *name = get_DW_ATE_name (enc);

  if (name == NULL)
    return dwarf_unknown ("ATE", enc);

  return name;
}

/* See stringify.h.  */

const char *
dwarf_unit_type_name (int unit_type)
{
  const char *name = get_DW_UT_name (unit_type);

  if (name == nullptr)
    return dwarf_unknown ("UT", unit_type);

  return name;
}
