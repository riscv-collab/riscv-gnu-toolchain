/* Support for printing C values for GDB, the GNU debugger.

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
#include "c-lang.h"
#include "cp-abi.h"
#include "target.h"
#include "objfiles.h"


/* A helper for c_textual_element_type.  This checks the name of the
   typedef.  This is bogus but it isn't apparent that the compiler
   provides us the help we may need.  */

static int
textual_name (const char *name)
{
  return (!strcmp (name, "wchar_t")
	  || !strcmp (name, "char16_t")
	  || !strcmp (name, "char32_t"));
}

/* Apply a heuristic to decide whether an array of TYPE or a pointer
   to TYPE should be printed as a textual string.  Return non-zero if
   it should, or zero if it should be treated as an array of integers
   or pointer to integers.  FORMAT is the current format letter, or 0
   if none.

   We guess that "char" is a character.  Explicitly signed and
   unsigned character types are also characters.  Integer data from
   vector types is not.  The user can override this by using the /s
   format letter.  */

int
c_textual_element_type (struct type *type, char format)
{
  struct type *true_type, *iter_type;

  if (format != 0 && format != 's')
    return 0;

  /* We also rely on this for its side effect of setting up all the
     typedef pointers.  */
  true_type = check_typedef (type);

  /* TYPE_CODE_CHAR is always textual.  */
  if (true_type->code () == TYPE_CODE_CHAR)
    return 1;

  /* Any other character-like types must be integral.  */
  if (true_type->code () != TYPE_CODE_INT)
    return 0;

  /* We peel typedefs one by one, looking for a match.  */
  iter_type = type;
  while (iter_type)
    {
      /* Check the name of the type.  */
      if (iter_type->name () && textual_name (iter_type->name ()))
	return 1;

      if (iter_type->code () != TYPE_CODE_TYPEDEF)
	break;

      /* Peel a single typedef.  If the typedef doesn't have a target
	 type, we use check_typedef and hope the result is ok -- it
	 might be for C++, where wchar_t is a built-in type.  */
      if (iter_type->target_type ())
	iter_type = iter_type->target_type ();
      else
	iter_type = check_typedef (iter_type);
    }

  if (format == 's')
    {
      /* Print this as a string if we can manage it.  For now, no wide
	 character support.  */
      if (true_type->code () == TYPE_CODE_INT
	  && true_type->length () == 1)
	return 1;
    }
  else
    {
      /* If a one-byte TYPE_CODE_INT is missing the not-a-character
	 flag, then we treat it as text; otherwise, we assume it's
	 being used as data.  */
      if (true_type->code () == TYPE_CODE_INT
	  && true_type->length () == 1
	  && !TYPE_NOTTEXT (true_type))
	return 1;
    }

  return 0;
}

/* Decorations for C.  */

static const struct generic_val_print_decorations c_decorations =
{
  "",
  " + ",
  "i",
  "true",
  "false",
  "void",
  "{",
  "}"
};

/* Print a pointer based on the type of its target.

   Arguments to this functions are roughly the same as those in c_val_print.
   A difference is that ADDRESS is the address to print, with embedded_offset
   already added.  UNRESOLVED_ELTTYPE and ELTTYPE represent the pointed type,
   respectively before and after check_typedef.  */

static void
print_unpacked_pointer (struct type *type, struct type *elttype,
			struct type *unresolved_elttype,
			const gdb_byte *valaddr, int embedded_offset,
			CORE_ADDR address, struct ui_file *stream, int recurse,
			const struct value_print_options *options)
{
  int want_space = 0;
  struct gdbarch *gdbarch = type->arch ();

  if (elttype->code () == TYPE_CODE_FUNC)
    {
      /* Try to print what function it points to.  */
      print_function_pointer_address (options, gdbarch, address, stream);
      return;
    }

  if (options->symbol_print)
    want_space = print_address_demangle (options, gdbarch, address, stream,
					 demangle);
  else if (options->addressprint)
    {
      gdb_puts (paddress (gdbarch, address), stream);
      want_space = 1;
    }

  /* For a pointer to a textual type, also print the string
     pointed to, unless pointer is null.  */

  if (c_textual_element_type (unresolved_elttype, options->format)
      && address != 0)
    {
      if (want_space)
	gdb_puts (" ", stream);
      val_print_string (unresolved_elttype, NULL, address, -1, stream, options);
    }
  else if (cp_is_vtbl_member (type))
    {
      /* Print vtbl's nicely.  */
      CORE_ADDR vt_address = unpack_pointer (type, valaddr + embedded_offset);
      struct bound_minimal_symbol msymbol =
	lookup_minimal_symbol_by_pc (vt_address);

      /* If 'symbol_print' is set, we did the work above.  */
      if (!options->symbol_print
	  && (msymbol.minsym != NULL)
	  && (vt_address == msymbol.value_address ()))
	{
	  if (want_space)
	    gdb_puts (" ", stream);
	  gdb_puts (" <", stream);
	  gdb_puts (msymbol.minsym->print_name (), stream);
	  gdb_puts (">", stream);
	  want_space = 1;
	}

      if (vt_address && options->vtblprint)
	{
	  struct value *vt_val;
	  struct symbol *wsym = NULL;
	  struct type *wtype;

	  if (want_space)
	    gdb_puts (" ", stream);

	  if (msymbol.minsym != NULL)
	    {
	      const char *search_name = msymbol.minsym->search_name ();
	      wsym = lookup_symbol_search_name (search_name, NULL,
						VAR_DOMAIN).symbol;
	    }

	  if (wsym)
	    {
	      wtype = wsym->type ();
	    }
	  else
	    {
	      wtype = unresolved_elttype;
	    }
	  vt_val = value_at (wtype, vt_address);
	  common_val_print (vt_val, stream, recurse + 1, options,
			    current_language);
	  if (options->prettyformat)
	    {
	      gdb_printf (stream, "\n");
	      print_spaces (2 + 2 * recurse, stream);
	    }
	}
    }
}

/* c_value_print helper for TYPE_CODE_ARRAY.  */

static void
c_value_print_array (struct value *val,
		     struct ui_file *stream, int recurse,
		     const struct value_print_options *options)
{
  struct type *type = check_typedef (val->type ());
  CORE_ADDR address = val->address ();
  const gdb_byte *valaddr = val->contents_for_printing ().data ();
  struct type *unresolved_elttype = type->target_type ();
  struct type *elttype = check_typedef (unresolved_elttype);

  if (type->length () > 0 && unresolved_elttype->length () > 0)
    {
      LONGEST low_bound, high_bound;
      int eltlen, len;
      enum bfd_endian byte_order = type_byte_order (type);

      if (!get_array_bounds (type, &low_bound, &high_bound))
	error (_("Could not determine the array high bound"));

      eltlen = elttype->length ();
      len = high_bound - low_bound + 1;

      /* Print arrays of textual chars with a string syntax, as
	 long as the entire array is valid.  */
      if (c_textual_element_type (unresolved_elttype,
				  options->format)
	  && val->bytes_available (0, type->length ())
	  && !val->bits_any_optimized_out (0,
					   TARGET_CHAR_BIT * type->length ()))
	{
	  int force_ellipses = 0;

	  /* If requested, look for the first null char and only
	     print elements up to it.  */
	  if (options->stop_print_at_null)
	    {
	      unsigned int print_max_chars = get_print_max_chars (options);
	      unsigned int temp_len;

	      for (temp_len = 0;
		   (temp_len < len
		    && temp_len < print_max_chars
		    && extract_unsigned_integer (valaddr + temp_len * eltlen,
						 eltlen, byte_order) != 0);
		   ++temp_len)
		;

	      /* Force printstr to print ellipses if
		 we've printed the maximum characters and
		 the next character is not \000.  */
	      if (temp_len == print_max_chars && temp_len < len)
		{
		  ULONGEST ival
		    = extract_unsigned_integer (valaddr + temp_len * eltlen,
						eltlen, byte_order);
		  if (ival != 0)
		    force_ellipses = 1;
		}

	      len = temp_len;
	    }

	  current_language->printstr (stream, unresolved_elttype, valaddr, len,
				      NULL, force_ellipses, options);
	}
      else
	{
	  unsigned int i = 0;
	  gdb_printf (stream, "{");
	  /* If this is a virtual function table, print the 0th
	     entry specially, and the rest of the members
	     normally.  */
	  if (cp_is_vtbl_ptr_type (elttype))
	    {
	      i = 1;
	      gdb_printf (stream, _("%d vtable entries"),
			  len - 1);
	    }
	  value_print_array_elements (val, stream, recurse, options, i);
	  gdb_printf (stream, "}");
	}
    }
  else
    {
      /* Array of unspecified length: treat like pointer to first elt.  */
      print_unpacked_pointer (type, elttype, unresolved_elttype, valaddr,
			      0, address, stream, recurse, options);
    }
}

/* c_value_print_inner helper for TYPE_CODE_PTR.  */

static void
c_value_print_ptr (struct value *val, struct ui_file *stream, int recurse,
		   const struct value_print_options *options)
{
  if (options->format && options->format != 's')
    {
      value_print_scalar_formatted (val, options, 0, stream);
      return;
    }

  struct type *type = check_typedef (val->type ());
  const gdb_byte *valaddr = val->contents_for_printing ().data ();

  if (options->vtblprint && cp_is_vtbl_ptr_type (type))
    {
      /* Print the unmangled name if desired.  */
      /* Print vtable entry - we only get here if we ARE using
	 -fvtable_thunks.  (Otherwise, look under
	 TYPE_CODE_STRUCT.)  */
      CORE_ADDR addr = extract_typed_address (valaddr, type);

      print_function_pointer_address (options, type->arch (), addr, stream);
    }
  else
    {
      struct type *unresolved_elttype = type->target_type ();
      struct type *elttype = check_typedef (unresolved_elttype);
      CORE_ADDR addr = unpack_pointer (type, valaddr);

      print_unpacked_pointer (type, elttype, unresolved_elttype, valaddr,
			      0, addr, stream, recurse, options);
    }
}

/* c_value_print helper for TYPE_CODE_STRUCT and TYPE_CODE_UNION.  */

static void
c_value_print_struct (struct value *val, struct ui_file *stream, int recurse,
		      const struct value_print_options *options)
{
  struct type *type = check_typedef (val->type ());

  if (type->code () == TYPE_CODE_UNION && recurse && !options->unionprint)
    gdb_printf (stream, "{...}");
  else if (options->vtblprint && cp_is_vtbl_ptr_type (type))
    {
      /* Print the unmangled name if desired.  */
      /* Print vtable entry - we only get here if NOT using
	 -fvtable_thunks.  (Otherwise, look under
	 TYPE_CODE_PTR.)  */
      int offset = type->field (VTBL_FNADDR_OFFSET).loc_bitpos () / 8;
      struct type *field_type = type->field (VTBL_FNADDR_OFFSET).type ();
      const gdb_byte *valaddr = val->contents_for_printing ().data ();
      CORE_ADDR addr = extract_typed_address (valaddr + offset, field_type);

      print_function_pointer_address (options, type->arch (), addr, stream);
    }
  else
    cp_print_value_fields (val, stream, recurse, options, NULL, 0);
}

/* c_value_print helper for TYPE_CODE_INT.  */

static void
c_value_print_int (struct value *val, struct ui_file *stream,
		   const struct value_print_options *options)
{
  if (options->format || options->output_format)
    {
      struct value_print_options opts = *options;

      opts.format = (options->format ? options->format
		     : options->output_format);
      value_print_scalar_formatted (val, &opts, 0, stream);
    }
  else
    {
      value_print_scalar_formatted (val, options, 0, stream);
      /* C and C++ has no single byte int type, char is used
	 instead.  Since we don't know whether the value is really
	 intended to be used as an integer or a character, print
	 the character equivalent as well.  */
      struct type *type = val->type ();
      const gdb_byte *valaddr = val->contents_for_printing ().data ();
      if (c_textual_element_type (type, options->format))
	{
	  gdb_puts (" ", stream);
	  current_language->printchar (unpack_long (type, valaddr), type,
				       stream);
	}
    }
}

/* See c-lang.h.  */

void
c_value_print_inner (struct value *val, struct ui_file *stream, int recurse,
		     const struct value_print_options *options)
{
  struct type *type = val->type ();

  type = check_typedef (type);
  switch (type->code ())
    {
    case TYPE_CODE_ARRAY:
      c_value_print_array (val, stream, recurse, options);
      break;

    case TYPE_CODE_PTR:
      c_value_print_ptr (val, stream, recurse, options);
      break;

    case TYPE_CODE_UNION:
    case TYPE_CODE_STRUCT:
      c_value_print_struct (val, stream, recurse, options);
      break;

    case TYPE_CODE_CHAR:
    case TYPE_CODE_INT:
      c_value_print_int (val, stream, options);
      break;

    case TYPE_CODE_METHODPTR:
    case TYPE_CODE_MEMBERPTR:
    case TYPE_CODE_REF:
    case TYPE_CODE_RVALUE_REF:
    case TYPE_CODE_ENUM:
    case TYPE_CODE_FLAGS:
    case TYPE_CODE_FUNC:
    case TYPE_CODE_METHOD:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_RANGE:
    case TYPE_CODE_FLT:
    case TYPE_CODE_DECFLOAT:
    case TYPE_CODE_VOID:
    case TYPE_CODE_ERROR:
    case TYPE_CODE_UNDEF:
    case TYPE_CODE_COMPLEX:
    default:
      generic_value_print (val, stream, recurse, options, &c_decorations);
      break;
    }
}


void
c_value_print (struct value *val, struct ui_file *stream, 
	       const struct value_print_options *options)
{
  struct type *type, *real_type;
  int full, using_enc;
  LONGEST top;
  struct value_print_options opts = *options;

  opts.deref_ref = true;

  /* If it is a pointer, indicate what it points to.

     Print type also if it is a reference.

     C++: if it is a member pointer, we will take care
     of that when we print it.  */

  type = check_typedef (val->type ());

  if (type->is_pointer_or_reference ())
    {
      struct type *original_type = val->type ();

      /* Hack:  remove (char *) for char strings.  Their
	 type is indicated by the quoted string anyway.
	 (Don't use c_textual_element_type here; quoted strings
	 are always exactly (char *), (wchar_t *), or the like.  */
      if (original_type->code () == TYPE_CODE_PTR
	  && original_type->name () == NULL
	  && original_type->target_type ()->name () != NULL
	  && (strcmp (original_type->target_type ()->name (),
		      "char") == 0
	      || textual_name (original_type->target_type ()->name ())))
	{
	  /* Print nothing.  */
	}
      else if (options->objectprint
	       && (type->target_type ()->code () == TYPE_CODE_STRUCT))
	{
	  int is_ref = TYPE_IS_REFERENCE (type);
	  enum type_code refcode = TYPE_CODE_UNDEF;

	  if (is_ref)
	    {
	      val = value_addr (val);
	      refcode = type->code ();
	    }

	  /* Pointer to class, check real type of object.  */
	  gdb_printf (stream, "(");

	  if (val->entirely_available ())
	    {
	      real_type = value_rtti_indirect_type (val, &full, &top,
						    &using_enc);
	      if (real_type)
		{
		  /* RTTI entry found.  */

		  /* Need to adjust pointer value.  */
		  val = value_from_pointer (real_type,
					    value_as_address (val) - top);

		  /* Note: When we look up RTTI entries, we don't get
		     any information on const or volatile
		     attributes.  */
		}
	    }

	  if (is_ref)
	    val = value_ref (value_ind (val), refcode);

	  type = val->type ();
	  type_print (type, "", stream, -1);
	  gdb_printf (stream, ") ");
	}
      else
	{
	  /* normal case */
	  gdb_printf (stream, "(");
	  type_print (val->type (), "", stream, -1);
	  gdb_printf (stream, ") ");
	}
    }

  if (!val->initialized ())
    gdb_printf (stream, " [uninitialized] ");

  if (options->objectprint && (type->code () == TYPE_CODE_STRUCT))
    {
      /* Attempt to determine real type of object.  */
      real_type = value_rtti_type (val, &full, &top, &using_enc);
      if (real_type)
	{
	  /* We have RTTI information, so use it.  */
	  val = value_full_object (val, real_type, 
				   full, top, using_enc);
	  /* In a destructor we might see a real type that is a
	     superclass of the object's type.  In this case it is
	     better to leave the object as-is.  */
	  if (!(full
		&& (real_type->length ()
		    < val->enclosing_type ()->length ())))
	    val = value_cast (real_type, val);
	  gdb_printf (stream, "(%s%s) ",
		      real_type->name (),
		      full ? "" : _(" [incomplete object]"));
	}
      else if (type != check_typedef (val->enclosing_type ()))
	{
	  /* No RTTI information, so let's do our best.  */
	  gdb_printf (stream, "(%s ?) ",
		      val->enclosing_type ()->name ());
	  val = value_cast (val->enclosing_type (), val);
	}
    }

  common_val_print (val, stream, 0, &opts, current_language);
}
