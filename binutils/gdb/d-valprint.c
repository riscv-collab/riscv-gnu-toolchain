/* Support for printing D values for GDB, the GNU debugger.

   Copyright (C) 2008-2024 Free Software Foundation, Inc.

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
#include "gdbtypes.h"
#include "gdbcore.h"
#include "d-lang.h"
#include "c-lang.h"

/* Assuming that TYPE is a TYPE_CODE_STRUCT, verify that TYPE is a
   dynamic array, and then print its value to STREAM.  Return zero if
   TYPE is a dynamic array, non-zero otherwise.  */

static int
dynamic_array_type (struct type *type,
		    LONGEST embedded_offset, CORE_ADDR address,
		    struct ui_file *stream, int recurse,
		    struct value *val,
		    const struct value_print_options *options)
{
  if (type->num_fields () == 2
      && type->field (0).type ()->code () == TYPE_CODE_INT
      && strcmp (type->field (0).name (), "length") == 0
      && strcmp (type->field (1).name (), "ptr") == 0
      && !val->bits_any_optimized_out (TARGET_CHAR_BIT * embedded_offset,
				       TARGET_CHAR_BIT * type->length ()))
    {
      CORE_ADDR addr;
      struct type *elttype;
      struct type *true_type;
      struct type *ptr_type;
      struct value *ival;
      int length;
      const gdb_byte *valaddr = val->contents_for_printing ().data ();

      length = unpack_field_as_long (type, valaddr + embedded_offset, 0);

      ptr_type = type->field (1).type ();
      elttype = check_typedef (ptr_type->target_type ());
      addr = unpack_pointer (ptr_type,
			     valaddr + type->field (1).loc_bitpos () / 8
			     + embedded_offset);
      true_type = check_typedef (elttype);

      true_type = lookup_array_range_type (true_type, 0, length - 1);
      ival = value_at (true_type, addr);
      true_type = ival->type ();

      d_value_print_inner (ival, stream, recurse + 1, options);
      return 0;
    }
  return 1;
}

/* See d-lang.h.  */

void
d_value_print_inner (struct value *val, struct ui_file *stream, int recurse,
		     const struct value_print_options *options)
{
  int ret;

  struct type *type = check_typedef (val->type ());
  switch (type->code ())
    {
      case TYPE_CODE_STRUCT:
	ret = dynamic_array_type (type, val->embedded_offset (),
				  val->address (),
				  stream, recurse, val, options);
	if (ret == 0)
	  break;
	[[fallthrough]];
      default:
	c_value_print_inner (val, stream, recurse, options);
	break;
    }
}
