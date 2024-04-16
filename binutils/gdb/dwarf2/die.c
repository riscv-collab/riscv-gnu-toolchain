/* DWARF DIEs

   Copyright (C) 1994-2024 Free Software Foundation, Inc.

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
#include "dwarf2/die.h"
#include "dwarf2/stringify.h"

/* See die.h.  */

struct die_info *
die_info::allocate (struct obstack *obstack, int num_attrs)
{
  size_t size = sizeof (struct die_info);

  if (num_attrs > 1)
    size += (num_attrs - 1) * sizeof (struct attribute);

  struct die_info *die = (struct die_info *) obstack_alloc (obstack, size);
  memset (die, 0, size);
  return die;
}

/* See die.h.  */

hashval_t
die_info::hash (const void *item)
{
  const struct die_info *die = (const struct die_info *) item;

  return to_underlying (die->sect_off);
}

/* See die.h.  */

int
die_info::eq (const void *item_lhs, const void *item_rhs)
{
  const struct die_info *die_lhs = (const struct die_info *) item_lhs;
  const struct die_info *die_rhs = (const struct die_info *) item_rhs;

  return die_lhs->sect_off == die_rhs->sect_off;
}

static void
dump_die_shallow (struct ui_file *f, int indent, struct die_info *die)
{
  unsigned int i;

  gdb_printf (f, "%*sDie: %s (abbrev %d, offset %s)\n",
	      indent, "",
	      dwarf_tag_name (die->tag), die->abbrev,
	      sect_offset_str (die->sect_off));

  if (die->parent != NULL)
    gdb_printf (f, "%*s  parent at offset: %s\n",
		indent, "",
		sect_offset_str (die->parent->sect_off));

  gdb_printf (f, "%*s  has children: %s\n",
	      indent, "",
	      dwarf_bool_name (die->child != NULL));

  gdb_printf (f, "%*s  attributes:\n", indent, "");

  for (i = 0; i < die->num_attrs; ++i)
    {
      gdb_printf (f, "%*s    %s (%s) ",
		  indent, "",
		  dwarf_attr_name (die->attrs[i].name),
		  dwarf_form_name (die->attrs[i].form));

      switch (die->attrs[i].form)
	{
	case DW_FORM_addr:
	case DW_FORM_addrx:
	case DW_FORM_GNU_addr_index:
	  gdb_printf (f, "address: ");
	  gdb_puts (hex_string ((CORE_ADDR) die->attrs[i].as_address ()), f);
	  break;
	case DW_FORM_block2:
	case DW_FORM_block4:
	case DW_FORM_block:
	case DW_FORM_block1:
	  gdb_printf (f, "block: size %s",
		      pulongest (die->attrs[i].as_block ()->size));
	  break;
	case DW_FORM_exprloc:
	  gdb_printf (f, "expression: size %s",
		      pulongest (die->attrs[i].as_block ()->size));
	  break;
	case DW_FORM_data16:
	  gdb_printf (f, "constant of 16 bytes");
	  break;
	case DW_FORM_ref_addr:
	  gdb_printf (f, "ref address: ");
	  gdb_puts (hex_string (die->attrs[i].as_unsigned ()), f);
	  break;
	case DW_FORM_GNU_ref_alt:
	  gdb_printf (f, "alt ref address: ");
	  gdb_puts (hex_string (die->attrs[i].as_unsigned ()), f);
	  break;
	case DW_FORM_ref1:
	case DW_FORM_ref2:
	case DW_FORM_ref4:
	case DW_FORM_ref8:
	case DW_FORM_ref_udata:
	  gdb_printf (f, "constant ref: 0x%lx (adjusted)",
		      (long) (die->attrs[i].as_unsigned ()));
	  break;
	case DW_FORM_data1:
	case DW_FORM_data2:
	case DW_FORM_data4:
	case DW_FORM_data8:
	case DW_FORM_udata:
	  gdb_printf (f, "constant: %s",
		      pulongest (die->attrs[i].as_unsigned ()));
	  break;
	case DW_FORM_sec_offset:
	  gdb_printf (f, "section offset: %s",
		      pulongest (die->attrs[i].as_unsigned ()));
	  break;
	case DW_FORM_ref_sig8:
	  gdb_printf (f, "signature: %s",
		      hex_string (die->attrs[i].as_signature ()));
	  break;
	case DW_FORM_string:
	case DW_FORM_strp:
	case DW_FORM_line_strp:
	case DW_FORM_strx:
	case DW_FORM_GNU_str_index:
	case DW_FORM_GNU_strp_alt:
	  gdb_printf (f, "string: \"%s\" (%s canonicalized)",
		      die->attrs[i].as_string ()
		      ? die->attrs[i].as_string () : "",
		      die->attrs[i].canonical_string_p () ? "is" : "not");
	  break;
	case DW_FORM_flag:
	  if (die->attrs[i].as_boolean ())
	    gdb_printf (f, "flag: TRUE");
	  else
	    gdb_printf (f, "flag: FALSE");
	  break;
	case DW_FORM_flag_present:
	  gdb_printf (f, "flag: TRUE");
	  break;
	case DW_FORM_indirect:
	  /* The reader will have reduced the indirect form to
	     the "base form" so this form should not occur.  */
	  gdb_printf (f,
		      "unexpected attribute form: DW_FORM_indirect");
	  break;
	case DW_FORM_sdata:
	case DW_FORM_implicit_const:
	  gdb_printf (f, "constant: %s",
		      plongest (die->attrs[i].as_signed ()));
	  break;
	default:
	  gdb_printf (f, "unsupported attribute form: %d.",
		      die->attrs[i].form);
	  break;
	}
      gdb_printf (f, "\n");
    }
}

static void
dump_die_1 (struct ui_file *f, int level, int max_level, struct die_info *die)
{
  int indent = level * 4;

  gdb_assert (die != NULL);

  if (level >= max_level)
    return;

  dump_die_shallow (f, indent, die);

  if (die->child != NULL)
    {
      gdb_printf (f, "%*s  Children:", indent, "");
      if (level + 1 < max_level)
	{
	  gdb_printf (f, "\n");
	  dump_die_1 (f, level + 1, max_level, die->child);
	}
      else
	{
	  gdb_printf (f,
		      " [not printed, max nesting level reached]\n");
	}
    }

  if (die->sibling != NULL && level > 0)
    {
      dump_die_1 (f, level, max_level, die->sibling);
    }
}

/* See die.h.  */

void
die_info::dump (int max_level)
{
  dump_die_1 (gdb_stdlog, 0, max_level, this);
}

/* See die.h.  */

void
die_info::error_dump ()
{
  dump_die_shallow (gdb_stderr, 0, this);
}
