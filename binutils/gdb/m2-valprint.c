/* Support for printing Modula 2 values for GDB, the GNU debugger.

   Copyright (C) 1986-2024 Free Software Foundation, Inc.

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
#include "symtab.h"
#include "gdbtypes.h"
#include "expression.h"
#include "value.h"
#include "valprint.h"
#include "language.h"
#include "typeprint.h"
#include "c-lang.h"
#include "m2-lang.h"
#include "target.h"
#include "cli/cli-style.h"

static int print_unpacked_pointer (struct type *type,
				   CORE_ADDR address, CORE_ADDR addr,
				   const struct value_print_options *options,
				   struct ui_file *stream);
static void
m2_print_array_contents (struct value *val,
			 struct ui_file *stream, int recurse,
			 const struct value_print_options *options,
			 int len);


/* get_long_set_bounds - assigns the bounds of the long set to low and
			 high.  */

int
get_long_set_bounds (struct type *type, LONGEST *low, LONGEST *high)
{
  int len, i;

  if (type->code () == TYPE_CODE_STRUCT)
    {
      len = type->num_fields ();
      i = TYPE_N_BASECLASSES (type);
      if (len == 0)
	return 0;
      *low = type->field (i).type ()->bounds ()->low.const_val ();
      *high = type->field (len - 1).type ()->bounds ()->high.const_val ();
      return 1;
    }
  error (_("expecting long_set"));
  return 0;
}

static void
m2_print_long_set (struct type *type, const gdb_byte *valaddr,
		   int embedded_offset, CORE_ADDR address,
		   struct ui_file *stream)
{
  int empty_set        = 1;
  int element_seen     = 0;
  LONGEST previous_low = 0;
  LONGEST previous_high= 0;
  LONGEST i, low_bound, high_bound;
  LONGEST field_low, field_high;
  struct type *range;
  int len, field;
  struct type *target;
  int bitval;

  type = check_typedef (type);

  gdb_printf (stream, "{");
  len = type->num_fields ();
  if (get_long_set_bounds (type, &low_bound, &high_bound))
    {
      field = TYPE_N_BASECLASSES (type);
      range = type->field (field).type ()->index_type ();
    }
  else
    {
      fprintf_styled (stream, metadata_style.style (),
		      " %s }", _("<unknown bounds of set>"));
      return;
    }

  target = range->target_type ();

  if (get_discrete_bounds (range, &field_low, &field_high))
    {
      for (i = low_bound; i <= high_bound; i++)
	{
	  bitval = value_bit_index (type->field (field).type (),
				    (type->field (field).loc_bitpos () / 8) +
				    valaddr + embedded_offset, i);
	  if (bitval < 0)
	    error (_("bit test is out of range"));
	  else if (bitval > 0)
	    {
	      previous_high = i;
	      if (! element_seen)
		{
		  if (! empty_set)
		    gdb_printf (stream, ", ");
		  print_type_scalar (target, i, stream);
		  empty_set    = 0;
		  element_seen = 1;
		  previous_low = i;
		}
	    }
	  else
	    {
	      /* bit is not set */
	      if (element_seen)
		{
		  if (previous_low+1 < previous_high)
		    gdb_printf (stream, "..");
		  if (previous_low+1 < previous_high)
		    print_type_scalar (target, previous_high, stream);
		  element_seen = 0;
		}
	    }
	  if (i == field_high)
	    {
	      field++;
	      if (field == len)
		break;
	      range = type->field (field).type ()->index_type ();
	      if (!get_discrete_bounds (range, &field_low, &field_high))
		break;
	      target = range->target_type ();
	    }
	}
      if (element_seen)
	{
	  if (previous_low+1 < previous_high)
	    {
	      gdb_printf (stream, "..");
	      print_type_scalar (target, previous_high, stream);
	    }
	  element_seen = 0;
	}
      gdb_printf (stream, "}");
    }
}

static void
m2_print_unbounded_array (struct value *value,
			  struct ui_file *stream, int recurse,
			  const struct value_print_options *options)
{
  CORE_ADDR addr;
  LONGEST len;
  struct value *val;

  struct type *type = check_typedef (value->type ());
  const gdb_byte *valaddr = value->contents_for_printing ().data ();

  addr = unpack_pointer (type->field (0).type (),
			 (type->field (0).loc_bitpos () / 8) +
			 valaddr);

  val = value_at_lazy (type->field (0).type ()->target_type (),
		       addr);
  len = unpack_field_as_long (type, valaddr, 1);

  gdb_printf (stream, "{");  
  m2_print_array_contents (val, stream, recurse, options, len);
  gdb_printf (stream, ", HIGH = %d}", (int) len);
}

static int
print_unpacked_pointer (struct type *type,
			CORE_ADDR address, CORE_ADDR addr,
			const struct value_print_options *options,
			struct ui_file *stream)
{
  struct gdbarch *gdbarch = type->arch ();
  struct type *elttype = check_typedef (type->target_type ());
  int want_space = 0;

  if (elttype->code () == TYPE_CODE_FUNC)
    {
      /* Try to print what function it points to.  */
      print_function_pointer_address (options, gdbarch, addr, stream);
      /* Return value is irrelevant except for string pointers.  */
      return 0;
    }

  if (options->addressprint && options->format != 's')
    {
      gdb_puts (paddress (gdbarch, address), stream);
      want_space = 1;
    }

  /* For a pointer to char or unsigned char, also print the string
     pointed to, unless pointer is null.  */

  if (elttype->length () == 1
      && elttype->code () == TYPE_CODE_INT
      && (options->format == 0 || options->format == 's')
      && addr != 0)
    {
      if (want_space)
	gdb_puts (" ", stream);
      return val_print_string (type->target_type (), NULL, addr, -1,
			       stream, options);
    }
  
  return 0;
}

static void
print_variable_at_address (struct type *type,
			   const gdb_byte *valaddr,
			   struct ui_file *stream,
			   int recurse,
			   const struct value_print_options *options)
{
  struct gdbarch *gdbarch = type->arch ();
  CORE_ADDR addr = unpack_pointer (type, valaddr);
  struct type *elttype = check_typedef (type->target_type ());

  gdb_printf (stream, "[");
  gdb_puts (paddress (gdbarch, addr), stream);
  gdb_printf (stream, "] : ");
  
  if (elttype->code () != TYPE_CODE_UNDEF)
    {
      struct value *deref_val =
	value_at (type->target_type (), unpack_pointer (type, valaddr));

      common_val_print (deref_val, stream, recurse, options, current_language);
    }
  else
    gdb_puts ("???", stream);
}


/* m2_print_array_contents - prints out the contents of an
			     array up to a max_print values.
			     It prints arrays of char as a string
			     and all other data types as comma
			     separated values.  */

static void
m2_print_array_contents (struct value *val,
			 struct ui_file *stream, int recurse,
			 const struct value_print_options *options,
			 int len)
{
  struct type *type = check_typedef (val->type ());

  if (type->length () > 0)
    {
      /* For an array of chars, print with string syntax.  */
      if (type->length () == 1 &&
	  ((type->code () == TYPE_CODE_INT)
	   || ((current_language->la_language == language_m2)
	       && (type->code () == TYPE_CODE_CHAR)))
	  && (options->format == 0 || options->format == 's'))
	val_print_string (type, NULL, val->address (), len+1, stream,
			  options);
      else
	{
	  gdb_printf (stream, "{");
	  value_print_array_elements (val, stream, recurse, options, 0);
	  gdb_printf (stream, "}");
	}
    }
}

/* Decorations for Modula 2.  */

static const struct generic_val_print_decorations m2_decorations =
{
  "",
  " + ",
  " * I",
  "TRUE",
  "FALSE",
  "void",
  "{",
  "}"
};

/* See m2-lang.h.  */

void
m2_language::value_print_inner (struct value *val, struct ui_file *stream,
				int recurse,
				const struct value_print_options *options) const
{
  unsigned len;
  struct type *elttype;
  CORE_ADDR addr;
  const gdb_byte *valaddr = val->contents_for_printing ().data ();
  const CORE_ADDR address = val->address ();

  struct type *type = check_typedef (val->type ());
  switch (type->code ())
    {
    case TYPE_CODE_ARRAY:
      if (type->length () > 0 && type->target_type ()->length () > 0)
	{
	  elttype = check_typedef (type->target_type ());
	  len = type->length () / elttype->length ();
	  /* For an array of chars, print with string syntax.  */
	  if (elttype->length () == 1 &&
	      ((elttype->code () == TYPE_CODE_INT)
	       || ((current_language->la_language == language_m2)
		   && (elttype->code () == TYPE_CODE_CHAR)))
	      && (options->format == 0 || options->format == 's'))
	    {
	      /* If requested, look for the first null char and only print
		 elements up to it.  */
	      if (options->stop_print_at_null)
		{
		  unsigned int print_max_chars = get_print_max_chars (options);
		  unsigned int temp_len;

		  /* Look for a NULL char.  */
		  for (temp_len = 0;
		       (valaddr[temp_len]
			&& temp_len < len
			&& temp_len < print_max_chars);
		       temp_len++);
		  len = temp_len;
		}

	      printstr (stream, type->target_type (), valaddr, len,
			NULL, 0, options);
	    }
	  else
	    {
	      gdb_printf (stream, "{");
	      value_print_array_elements (val, stream, recurse,
					  options, 0);
	      gdb_printf (stream, "}");
	    }
	  break;
	}
      /* Array of unspecified length: treat like pointer to first elt.  */
      print_unpacked_pointer (type, address, address, options, stream);
      break;

    case TYPE_CODE_PTR:
      if (TYPE_CONST (type))
	print_variable_at_address (type, valaddr, stream, recurse, options);
      else if (options->format && options->format != 's')
	value_print_scalar_formatted (val, options, 0, stream);
      else
	{
	  addr = unpack_pointer (type, valaddr);
	  print_unpacked_pointer (type, addr, address, options, stream);
	}
      break;

    case TYPE_CODE_UNION:
      if (recurse && !options->unionprint)
	{
	  gdb_printf (stream, "{...}");
	  break;
	}
      [[fallthrough]];
    case TYPE_CODE_STRUCT:
      if (m2_is_long_set (type))
	m2_print_long_set (type, valaddr, 0, address, stream);
      else if (m2_is_unbounded_array (type))
	m2_print_unbounded_array (val, stream, recurse, options);
      else
	cp_print_value_fields (val, stream, recurse, options, NULL, 0);
      break;

    case TYPE_CODE_SET:
      elttype = type->index_type ();
      elttype = check_typedef (elttype);
      if (elttype->is_stub ())
	{
	  fprintf_styled (stream, metadata_style.style (),
			  _("<incomplete type>"));
	  break;
	}
      else
	{
	  struct type *range = elttype;
	  LONGEST low_bound, high_bound;
	  int i;
	  int need_comma = 0;

	  gdb_puts ("{", stream);

	  i = get_discrete_bounds (range, &low_bound, &high_bound) ? 0 : -1;
	maybe_bad_bstring:
	  if (i < 0)
	    {
	      fputs_styled (_("<error value>"), metadata_style.style (),
			    stream);
	      goto done;
	    }

	  for (i = low_bound; i <= high_bound; i++)
	    {
	      int element = value_bit_index (type, valaddr, i);

	      if (element < 0)
		{
		  i = element;
		  goto maybe_bad_bstring;
		}
	      if (element)
		{
		  if (need_comma)
		    gdb_puts (", ", stream);
		  print_type_scalar (range, i, stream);
		  need_comma = 1;

		  if (i + 1 <= high_bound
		      && value_bit_index (type, valaddr, ++i))
		    {
		      int j = i;

		      gdb_puts ("..", stream);
		      while (i + 1 <= high_bound
			     && value_bit_index (type, valaddr, ++i))
			j = i;
		      print_type_scalar (range, j, stream);
		    }
		}
	    }
	done:
	  gdb_puts ("}", stream);
	}
      break;

    case TYPE_CODE_RANGE:
      if (type->length () == type->target_type ()->length ())
	{
	  struct value *v = value_cast (type->target_type (), val);
	  value_print_inner (v, stream, recurse, options);
	  break;
	}
      [[fallthrough]];

    case TYPE_CODE_REF:
    case TYPE_CODE_ENUM:
    case TYPE_CODE_FUNC:
    case TYPE_CODE_INT:
    case TYPE_CODE_FLT:
    case TYPE_CODE_METHOD:
    case TYPE_CODE_VOID:
    case TYPE_CODE_ERROR:
    case TYPE_CODE_UNDEF:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_CHAR:
    default:
      generic_value_print (val, stream, recurse, options, &m2_decorations);
      break;
    }
}
