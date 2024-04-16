/* Support for printing Go values for GDB, the GNU debugger.

   Copyright (C) 2012-2024 Free Software Foundation, Inc.

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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   NOTE: This currently only provides special support for printing gccgo
   strings.  6g objects are handled in Python.
   The remaining gccgo types may also be handled in Python.
   Strings are handled specially here, at least for now, in case the Python
   support is unavailable.  */

#include "defs.h"
#include "gdbtypes.h"
#include "gdbcore.h"
#include "go-lang.h"
#include "c-lang.h"
#include "valprint.h"
#include "cli/cli-style.h"

/* Print a Go string.

   Note: We assume
   gdb_assert (go_classify_struct_type (type) == GO_TYPE_STRING).  */

static void
print_go_string (struct type *type,
		 LONGEST embedded_offset, CORE_ADDR address,
		 struct ui_file *stream, int recurse,
		 struct value *val,
		 const struct value_print_options *options)
{
  struct gdbarch *gdbarch = type->arch ();
  struct type *elt_ptr_type = type->field (0).type ();
  struct type *elt_type = elt_ptr_type->target_type ();
  LONGEST length;
  /* TODO(dje): The encapsulation of what a pointer is belongs in value.c.
     I.e. If there's going to be unpack_pointer, there should be
     unpack_value_field_as_pointer.  Do this until we can get
     unpack_value_field_as_pointer.  */
  LONGEST addr;
  const gdb_byte *valaddr = val->contents_for_printing ().data ();


  if (! unpack_value_field_as_long (type, valaddr, embedded_offset, 0,
				    val, &addr))
    error (_("Unable to read string address"));

  if (! unpack_value_field_as_long (type, valaddr, embedded_offset, 1,
				    val, &length))
    error (_("Unable to read string length"));

  /* TODO(dje): Print address of struct or actual string?  */
  if (options->addressprint)
    {
      gdb_puts (paddress (gdbarch, addr), stream);
      gdb_puts (" ", stream);
    }

  if (length < 0)
    {
      gdb_printf (_("<invalid length: %ps>"),
		  styled_string (metadata_style.style (),
				 plongest (addr)));
      return;
    }

  /* TODO(dje): Perhaps we should pass "UTF8" for ENCODING.
     The target encoding is a global switch.
     Either choice is problematic.  */
  val_print_string (elt_type, NULL, addr, length, stream, options);
}

/* See go-lang.h.  */

void
go_language::value_print_inner (struct value *val, struct ui_file *stream,
				int recurse,
				const struct value_print_options *options) const
{
  struct type *type = check_typedef (val->type ());

  switch (type->code ())
    {
      case TYPE_CODE_STRUCT:
	{
	  enum go_type go_type = go_classify_struct_type (type);

	  switch (go_type)
	    {
	    case GO_TYPE_STRING:
	      if (! options->raw)
		{
		  print_go_string (type, val->embedded_offset (),
				   val->address (),
				   stream, recurse, val, options);
		  return;
		}
	      break;
	    default:
	      break;
	    }
	}
	[[fallthrough]];

      default:
	c_value_print_inner (val, stream, recurse, options);
	break;
    }
}
