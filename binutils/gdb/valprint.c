/* Print values for GDB, the GNU debugger.

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
#include "value.h"
#include "gdbcore.h"
#include "gdbcmd.h"
#include "target.h"
#include "language.h"
#include "annotate.h"
#include "valprint.h"
#include "target-float.h"
#include "extension.h"
#include "ada-lang.h"
#include "gdbsupport/gdb_obstack.h"
#include "charset.h"
#include "typeprint.h"
#include <ctype.h>
#include <algorithm>
#include "gdbsupport/byte-vector.h"
#include "cli/cli-option.h"
#include "gdbarch.h"
#include "cli/cli-style.h"
#include "count-one-bits.h"
#include "c-lang.h"
#include "cp-abi.h"
#include "inferior.h"
#include "gdbsupport/selftest.h"
#include "selftest-arch.h"

/* Maximum number of wchars returned from wchar_iterate.  */
#define MAX_WCHARS 4

/* A convenience macro to compute the size of a wchar_t buffer containing X
   characters.  */
#define WCHAR_BUFLEN(X) ((X) * sizeof (gdb_wchar_t))

/* Character buffer size saved while iterating over wchars.  */
#define WCHAR_BUFLEN_MAX WCHAR_BUFLEN (MAX_WCHARS)

/* A structure to encapsulate state information from iterated
   character conversions.  */
struct converted_character
{
  /* The number of characters converted.  */
  int num_chars;

  /* The result of the conversion.  See charset.h for more.  */
  enum wchar_iterate_result result;

  /* The (saved) converted character(s).  */
  gdb_wchar_t chars[WCHAR_BUFLEN_MAX];

  /* The first converted target byte.  */
  const gdb_byte *buf;

  /* The number of bytes converted.  */
  size_t buflen;

  /* How many times this character(s) is repeated.  */
  int repeat_count;
};

/* Command lists for set/show print raw.  */
struct cmd_list_element *setprintrawlist;
struct cmd_list_element *showprintrawlist;

/* Prototypes for local functions */

static void set_input_radix_1 (int, unsigned);

static void set_output_radix_1 (int, unsigned);

static void val_print_type_code_flags (struct type *type,
				       struct value *original_value,
				       int embedded_offset,
				       struct ui_file *stream);

/* Start print_max at this value.  */
#define PRINT_MAX_DEFAULT 200

/* Start print_max_chars at this value (meaning follow print_max).  */
#define PRINT_MAX_CHARS_DEFAULT PRINT_MAX_CHARS_ELEMENTS

/* Start print_max_depth at this value. */
#define PRINT_MAX_DEPTH_DEFAULT 20

struct value_print_options user_print_options =
{
  Val_prettyformat_default,	/* prettyformat */
  false,			/* prettyformat_arrays */
  false,			/* prettyformat_structs */
  false,			/* vtblprint */
  true,				/* unionprint */
  true,				/* addressprint */
  false,			/* nibblesprint */
  false,			/* objectprint */
  PRINT_MAX_DEFAULT,		/* print_max */
  PRINT_MAX_CHARS_DEFAULT,	/* print_max_chars */
  10,				/* repeat_count_threshold */
  0,				/* output_format */
  0,				/* format */
  true,				/* memory_tag_violations */
  false,			/* stop_print_at_null */
  false,			/* print_array_indexes */
  false,			/* deref_ref */
  true,				/* static_field_print */
  true,				/* pascal_static_field_print */
  false,			/* raw */
  false,			/* summary */
  true,				/* symbol_print */
  PRINT_MAX_DEPTH_DEFAULT,	/* max_depth */
};

/* Initialize *OPTS to be a copy of the user print options.  */
void
get_user_print_options (struct value_print_options *opts)
{
  *opts = user_print_options;
}

/* Initialize *OPTS to be a copy of the user print options, but with
   pretty-formatting disabled.  */
void
get_no_prettyformat_print_options (struct value_print_options *opts)
{  
  *opts = user_print_options;
  opts->prettyformat = Val_no_prettyformat;
}

/* Initialize *OPTS to be a copy of the user print options, but using
   FORMAT as the formatting option.  */
void
get_formatted_print_options (struct value_print_options *opts,
			     char format)
{
  *opts = user_print_options;
  opts->format = format;
}

/* Implement 'show print elements'.  */

static void
show_print_max (struct ui_file *file, int from_tty,
		struct cmd_list_element *c, const char *value)
{
  gdb_printf
    (file,
     (user_print_options.print_max_chars != PRINT_MAX_CHARS_ELEMENTS
      ? _("Limit on array elements to print is %s.\n")
      : _("Limit on string chars or array elements to print is %s.\n")),
     value);
}

/* Implement 'show print characters'.  */

static void
show_print_max_chars (struct ui_file *file, int from_tty,
		      struct cmd_list_element *c, const char *value)
{
  gdb_printf (file,
	      _("Limit on string characters to print is %s.\n"),
	      value);
}

/* Default input and output radixes, and output format letter.  */

unsigned input_radix = 10;
static void
show_input_radix (struct ui_file *file, int from_tty,
		  struct cmd_list_element *c, const char *value)
{
  gdb_printf (file,
	      _("Default input radix for entering numbers is %s.\n"),
	      value);
}

unsigned output_radix = 10;
static void
show_output_radix (struct ui_file *file, int from_tty,
		   struct cmd_list_element *c, const char *value)
{
  gdb_printf (file,
	      _("Default output radix for printing of values is %s.\n"),
	      value);
}

/* By default we print arrays without printing the index of each element in
   the array.  This behavior can be changed by setting PRINT_ARRAY_INDEXES.  */

static void
show_print_array_indexes (struct ui_file *file, int from_tty,
			  struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Printing of array indexes is %s.\n"), value);
}

/* Print repeat counts if there are more than this many repetitions of an
   element in an array.  Referenced by the low level language dependent
   print routines.  */

static void
show_repeat_count_threshold (struct ui_file *file, int from_tty,
			     struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Threshold for repeated print elements is %s.\n"),
	      value);
}

/* If nonzero, prints memory tag violations for pointers.  */

static void
show_memory_tag_violations (struct ui_file *file, int from_tty,
			    struct cmd_list_element *c, const char *value)
{
  gdb_printf (file,
	      _("Printing of memory tag violations is %s.\n"),
	      value);
}

/* If nonzero, stops printing of char arrays at first null.  */

static void
show_stop_print_at_null (struct ui_file *file, int from_tty,
			 struct cmd_list_element *c, const char *value)
{
  gdb_printf (file,
	      _("Printing of char arrays to stop "
		"at first null char is %s.\n"),
	      value);
}

/* Controls pretty printing of structures.  */

static void
show_prettyformat_structs (struct ui_file *file, int from_tty,
			  struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Pretty formatting of structures is %s.\n"), value);
}

/* Controls pretty printing of arrays.  */

static void
show_prettyformat_arrays (struct ui_file *file, int from_tty,
			 struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Pretty formatting of arrays is %s.\n"), value);
}

/* If nonzero, causes unions inside structures or other unions to be
   printed.  */

static void
show_unionprint (struct ui_file *file, int from_tty,
		 struct cmd_list_element *c, const char *value)
{
  gdb_printf (file,
	      _("Printing of unions interior to structures is %s.\n"),
	      value);
}

/* Controls the format of printing binary values.  */

static void
show_nibbles (struct ui_file *file, int from_tty,
		       struct cmd_list_element *c, const char *value)
{
  gdb_printf (file,
	      _("Printing binary values in groups is %s.\n"),
	      value);
}

/* If nonzero, causes machine addresses to be printed in certain contexts.  */

static void
show_addressprint (struct ui_file *file, int from_tty,
		   struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Printing of addresses is %s.\n"), value);
}

static void
show_symbol_print (struct ui_file *file, int from_tty,
		   struct cmd_list_element *c, const char *value)
{
  gdb_printf (file,
	      _("Printing of symbols when printing pointers is %s.\n"),
	      value);
}



/* A helper function for val_print.  When printing in "summary" mode,
   we want to print scalar arguments, but not aggregate arguments.
   This function distinguishes between the two.  */

int
val_print_scalar_type_p (struct type *type)
{
  type = check_typedef (type);
  while (TYPE_IS_REFERENCE (type))
    {
      type = type->target_type ();
      type = check_typedef (type);
    }
  switch (type->code ())
    {
    case TYPE_CODE_ARRAY:
    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
    case TYPE_CODE_SET:
    case TYPE_CODE_STRING:
      return 0;
    default:
      return 1;
    }
}

/* A helper function for val_print.  When printing with limited depth we
   want to print string and scalar arguments, but not aggregate arguments.
   This function distinguishes between the two.  */

static bool
val_print_scalar_or_string_type_p (struct type *type,
				   const struct language_defn *language)
{
  return (val_print_scalar_type_p (type)
	  || language->is_string_type_p (type));
}

/* See valprint.h.  */

int
valprint_check_validity (struct ui_file *stream,
			 struct type *type,
			 LONGEST embedded_offset,
			 const struct value *val)
{
  type = check_typedef (type);

  if (type_not_associated (type))
    {
      val_print_not_associated (stream);
      return 0;
    }

  if (type_not_allocated (type))
    {
      val_print_not_allocated (stream);
      return 0;
    }

  if (type->code () != TYPE_CODE_UNION
      && type->code () != TYPE_CODE_STRUCT
      && type->code () != TYPE_CODE_ARRAY)
    {
      if (val->bits_any_optimized_out (TARGET_CHAR_BIT * embedded_offset,
				       TARGET_CHAR_BIT * type->length ()))
	{
	  val_print_optimized_out (val, stream);
	  return 0;
	}

      if (val->bits_synthetic_pointer (TARGET_CHAR_BIT * embedded_offset,
				       TARGET_CHAR_BIT * type->length ()))
	{
	  const int is_ref = type->code () == TYPE_CODE_REF;
	  int ref_is_addressable = 0;

	  if (is_ref)
	    {
	      const struct value *deref_val = coerce_ref_if_computed (val);

	      if (deref_val != NULL)
		ref_is_addressable = deref_val->lval () == lval_memory;
	    }

	  if (!is_ref || !ref_is_addressable)
	    fputs_styled (_("<synthetic pointer>"), metadata_style.style (),
			  stream);

	  /* C++ references should be valid even if they're synthetic.  */
	  return is_ref;
	}

      if (!val->bytes_available (embedded_offset, type->length ()))
	{
	  val_print_unavailable (stream);
	  return 0;
	}
    }

  return 1;
}

void
val_print_optimized_out (const struct value *val, struct ui_file *stream)
{
  if (val != NULL && val->lval () == lval_register)
    val_print_not_saved (stream);
  else
    fprintf_styled (stream, metadata_style.style (), _("<optimized out>"));
}

void
val_print_not_saved (struct ui_file *stream)
{
  fprintf_styled (stream, metadata_style.style (), _("<not saved>"));
}

void
val_print_unavailable (struct ui_file *stream)
{
  fprintf_styled (stream, metadata_style.style (), _("<unavailable>"));
}

void
val_print_invalid_address (struct ui_file *stream)
{
  fprintf_styled (stream, metadata_style.style (), _("<invalid address>"));
}

/* Print a pointer based on the type of its target.

   Arguments to this functions are roughly the same as those in
   generic_val_print.  A difference is that ADDRESS is the address to print,
   with embedded_offset already added.  ELTTYPE represents
   the pointed type after check_typedef.  */

static void
print_unpacked_pointer (struct type *type, struct type *elttype,
			CORE_ADDR address, struct ui_file *stream,
			const struct value_print_options *options)
{
  struct gdbarch *gdbarch = type->arch ();

  if (elttype->code () == TYPE_CODE_FUNC)
    {
      /* Try to print what function it points to.  */
      print_function_pointer_address (options, gdbarch, address, stream);
      return;
    }

  if (options->symbol_print)
    print_address_demangle (options, gdbarch, address, stream, demangle);
  else if (options->addressprint)
    gdb_puts (paddress (gdbarch, address), stream);
}

/* generic_val_print helper for TYPE_CODE_ARRAY.  */

static void
generic_val_print_array (struct value *val,
			 struct ui_file *stream, int recurse,
			 const struct value_print_options *options,
			 const struct
			     generic_val_print_decorations *decorations)
{
  struct type *type = check_typedef (val->type ());
  struct type *unresolved_elttype = type->target_type ();
  struct type *elttype = check_typedef (unresolved_elttype);

  if (type->length () > 0 && unresolved_elttype->length () > 0)
    {
      LONGEST low_bound, high_bound;

      if (!get_array_bounds (type, &low_bound, &high_bound))
	error (_("Could not determine the array high bound"));

      gdb_puts (decorations->array_start, stream);
      value_print_array_elements (val, stream, recurse, options, 0);
      gdb_puts (decorations->array_end, stream);
    }
  else
    {
      /* Array of unspecified length: treat like pointer to first elt.  */
      print_unpacked_pointer (type, elttype, val->address (),
			      stream, options);
    }

}

/* generic_value_print helper for TYPE_CODE_PTR.  */

static void
generic_value_print_ptr (struct value *val, struct ui_file *stream,
			 const struct value_print_options *options)
{

  if (options->format && options->format != 's')
    value_print_scalar_formatted (val, options, 0, stream);
  else
    {
      struct type *type = check_typedef (val->type ());
      struct type *elttype = check_typedef (type->target_type ());
      const gdb_byte *valaddr = val->contents_for_printing ().data ();
      CORE_ADDR addr = unpack_pointer (type, valaddr);

      print_unpacked_pointer (type, elttype, addr, stream, options);
    }
}


/* Print '@' followed by the address contained in ADDRESS_BUFFER.  */

static void
print_ref_address (struct type *type, const gdb_byte *address_buffer,
		  int embedded_offset, struct ui_file *stream)
{
  struct gdbarch *gdbarch = type->arch ();

  if (address_buffer != NULL)
    {
      CORE_ADDR address
	= extract_typed_address (address_buffer + embedded_offset, type);

      gdb_printf (stream, "@");
      gdb_puts (paddress (gdbarch, address), stream);
    }
  /* Else: we have a non-addressable value, such as a DW_AT_const_value.  */
}

/* If VAL is addressable, return the value contents buffer of a value that
   represents a pointer to VAL.  Otherwise return NULL.  */

static const gdb_byte *
get_value_addr_contents (struct value *deref_val)
{
  gdb_assert (deref_val != NULL);

  if (deref_val->lval () == lval_memory)
    return value_addr (deref_val)->contents_for_printing ().data ();
  else
    {
      /* We have a non-addressable value, such as a DW_AT_const_value.  */
      return NULL;
    }
}

/* generic_val_print helper for TYPE_CODE_{RVALUE_,}REF.  */

static void
generic_val_print_ref (struct type *type,
		       int embedded_offset, struct ui_file *stream, int recurse,
		       struct value *original_value,
		       const struct value_print_options *options)
{
  struct type *elttype = check_typedef (type->target_type ());
  struct value *deref_val = NULL;
  const bool value_is_synthetic
    = original_value->bits_synthetic_pointer (TARGET_CHAR_BIT * embedded_offset,
					      TARGET_CHAR_BIT * type->length ());
  const int must_coerce_ref = ((options->addressprint && value_is_synthetic)
			       || options->deref_ref);
  const int type_is_defined = elttype->code () != TYPE_CODE_UNDEF;
  const gdb_byte *valaddr = original_value->contents_for_printing ().data ();

  if (must_coerce_ref && type_is_defined)
    {
      deref_val = coerce_ref_if_computed (original_value);

      if (deref_val != NULL)
	{
	  /* More complicated computed references are not supported.  */
	  gdb_assert (embedded_offset == 0);
	}
      else
	deref_val = value_at (type->target_type (),
			      unpack_pointer (type, valaddr + embedded_offset));
    }
  /* Else, original_value isn't a synthetic reference or we don't have to print
     the reference's contents.

     Notice that for references to TYPE_CODE_STRUCT, 'set print object on' will
     cause original_value to be a not_lval instead of an lval_computed,
     which will make value_bits_synthetic_pointer return false.
     This happens because if options->objectprint is true, c_value_print will
     overwrite original_value's contents with the result of coercing
     the reference through value_addr, and then set its type back to
     TYPE_CODE_REF.  In that case we don't have to coerce the reference again;
     we can simply treat it as non-synthetic and move on.  */

  if (options->addressprint)
    {
      const gdb_byte *address = (value_is_synthetic && type_is_defined
				 ? get_value_addr_contents (deref_val)
				 : valaddr);

      print_ref_address (type, address, embedded_offset, stream);

      if (options->deref_ref)
	gdb_puts (": ", stream);
    }

  if (options->deref_ref)
    {
      if (type_is_defined)
	common_val_print (deref_val, stream, recurse, options,
			  current_language);
      else
	gdb_puts ("???", stream);
    }
}

/* Helper function for generic_val_print_enum.
   This is also used to print enums in TYPE_CODE_FLAGS values.  */

static void
generic_val_print_enum_1 (struct type *type, LONGEST val,
			  struct ui_file *stream)
{
  unsigned int i;
  unsigned int len;

  len = type->num_fields ();
  for (i = 0; i < len; i++)
    {
      QUIT;
      if (val == type->field (i).loc_enumval ())
	{
	  break;
	}
    }
  if (i < len)
    {
      fputs_styled (type->field (i).name (), variable_name_style.style (),
		    stream);
    }
  else if (type->is_flag_enum ())
    {
      int first = 1;

      /* We have a "flag" enum, so we try to decompose it into pieces as
	 appropriate.  The enum may have multiple enumerators representing
	 the same bit, in which case we choose to only print the first one
	 we find.  */
      for (i = 0; i < len; ++i)
	{
	  QUIT;

	  ULONGEST enumval = type->field (i).loc_enumval ();
	  int nbits = count_one_bits_ll (enumval);

	  gdb_assert (nbits == 0 || nbits == 1);

	  if ((val & enumval) != 0)
	    {
	      if (first)
		{
		  gdb_puts ("(", stream);
		  first = 0;
		}
	      else
		gdb_puts (" | ", stream);

	      val &= ~type->field (i).loc_enumval ();
	      fputs_styled (type->field (i).name (),
			    variable_name_style.style (), stream);
	    }
	}

      if (val != 0)
	{
	  /* There are leftover bits, print them.  */
	  if (first)
	    gdb_puts ("(", stream);
	  else
	    gdb_puts (" | ", stream);

	  gdb_puts ("unknown: 0x", stream);
	  print_longest (stream, 'x', 0, val);
	  gdb_puts (")", stream);
	}
      else if (first)
	{
	  /* Nothing has been printed and the value is 0, the enum value must
	     have been 0.  */
	  gdb_puts ("0", stream);
	}
      else
	{
	  /* Something has been printed, close the parenthesis.  */
	  gdb_puts (")", stream);
	}
    }
  else
    print_longest (stream, 'd', 0, val);
}

/* generic_val_print helper for TYPE_CODE_ENUM.  */

static void
generic_val_print_enum (struct type *type,
			int embedded_offset, struct ui_file *stream,
			struct value *original_value,
			const struct value_print_options *options)
{
  LONGEST val;
  struct gdbarch *gdbarch = type->arch ();
  int unit_size = gdbarch_addressable_memory_unit_size (gdbarch);

  gdb_assert (!options->format);

  const gdb_byte *valaddr = original_value->contents_for_printing ().data ();

  val = unpack_long (type, valaddr + embedded_offset * unit_size);

  generic_val_print_enum_1 (type, val, stream);
}

/* generic_val_print helper for TYPE_CODE_FUNC and TYPE_CODE_METHOD.  */

static void
generic_val_print_func (struct type *type,
			int embedded_offset, CORE_ADDR address,
			struct ui_file *stream,
			struct value *original_value,
			const struct value_print_options *options)
{
  struct gdbarch *gdbarch = type->arch ();

  gdb_assert (!options->format);

  /* FIXME, we should consider, at least for ANSI C language,
     eliminating the distinction made between FUNCs and POINTERs to
     FUNCs.  */
  gdb_printf (stream, "{");
  type_print (type, "", stream, -1);
  gdb_printf (stream, "} ");
  /* Try to print what function it points to, and its address.  */
  print_address_demangle (options, gdbarch, address, stream, demangle);
}

/* generic_value_print helper for TYPE_CODE_BOOL.  */

static void
generic_value_print_bool
  (struct value *value, struct ui_file *stream,
   const struct value_print_options *options,
   const struct generic_val_print_decorations *decorations)
{
  if (options->format || options->output_format)
    {
      struct value_print_options opts = *options;
      opts.format = (options->format ? options->format
		     : options->output_format);
      value_print_scalar_formatted (value, &opts, 0, stream);
    }
  else
    {
      const gdb_byte *valaddr = value->contents_for_printing ().data ();
      struct type *type = check_typedef (value->type ());
      LONGEST val = unpack_long (type, valaddr);
      if (val == 0)
	gdb_puts (decorations->false_name, stream);
      else if (val == 1)
	gdb_puts (decorations->true_name, stream);
      else
	print_longest (stream, 'd', 0, val);
    }
}

/* generic_value_print helper for TYPE_CODE_INT.  */

static void
generic_value_print_int (struct value *val, struct ui_file *stream,
			 const struct value_print_options *options)
{
  struct value_print_options opts = *options;

  opts.format = (options->format ? options->format
		 : options->output_format);
  value_print_scalar_formatted (val, &opts, 0, stream);
}

/* generic_value_print helper for TYPE_CODE_CHAR.  */

static void
generic_value_print_char (struct value *value, struct ui_file *stream,
			  const struct value_print_options *options)
{
  if (options->format || options->output_format)
    {
      struct value_print_options opts = *options;

      opts.format = (options->format ? options->format
		     : options->output_format);
      value_print_scalar_formatted (value, &opts, 0, stream);
    }
  else
    {
      struct type *unresolved_type = value->type ();
      struct type *type = check_typedef (unresolved_type);
      const gdb_byte *valaddr = value->contents_for_printing ().data ();

      LONGEST val = unpack_long (type, valaddr);
      if (type->is_unsigned ())
	gdb_printf (stream, "%u", (unsigned int) val);
      else
	gdb_printf (stream, "%d", (int) val);
      gdb_puts (" ", stream);
      current_language->printchar (val, unresolved_type, stream);
    }
}

/* generic_val_print helper for TYPE_CODE_FLT and TYPE_CODE_DECFLOAT.  */

static void
generic_val_print_float (struct type *type, struct ui_file *stream,
			 struct value *original_value,
			 const struct value_print_options *options)
{
  gdb_assert (!options->format);

  const gdb_byte *valaddr = original_value->contents_for_printing ().data ();

  print_floating (valaddr, type, stream);
}

/* generic_val_print helper for TYPE_CODE_FIXED_POINT.  */

static void
generic_val_print_fixed_point (struct value *val, struct ui_file *stream,
			       const struct value_print_options *options)
{
  if (options->format)
    value_print_scalar_formatted (val, options, 0, stream);
  else
    {
      struct type *type = val->type ();

      const gdb_byte *valaddr = val->contents_for_printing ().data ();
      gdb_mpf f;

      f.read_fixed_point (gdb::make_array_view (valaddr, type->length ()),
			  type_byte_order (type), type->is_unsigned (),
			  type->fixed_point_scaling_factor ());

      const char *fmt = type->length () < 4 ? "%.11Fg" : "%.17Fg";
      std::string str = f.str (fmt);
      gdb_printf (stream, "%s", str.c_str ());
    }
}

/* generic_value_print helper for TYPE_CODE_COMPLEX.  */

static void
generic_value_print_complex (struct value *val, struct ui_file *stream,
			     const struct value_print_options *options,
			     const struct generic_val_print_decorations
			       *decorations)
{
  gdb_printf (stream, "%s", decorations->complex_prefix);

  struct value *real_part = value_real_part (val);
  value_print_scalar_formatted (real_part, options, 0, stream);
  gdb_printf (stream, "%s", decorations->complex_infix);

  struct value *imag_part = value_imaginary_part (val);
  value_print_scalar_formatted (imag_part, options, 0, stream);
  gdb_printf (stream, "%s", decorations->complex_suffix);
}

/* generic_value_print helper for TYPE_CODE_MEMBERPTR.  */

static void
generic_value_print_memberptr
  (struct value *val, struct ui_file *stream,
   int recurse,
   const struct value_print_options *options,
   const struct generic_val_print_decorations *decorations)
{
  if (!options->format)
    {
      /* Member pointers are essentially specific to C++, and so if we
	 encounter one, we should print it according to C++ rules.  */
      struct type *type = check_typedef (val->type ());
      const gdb_byte *valaddr = val->contents_for_printing ().data ();
      cp_print_class_member (valaddr, type, stream, "&");
    }
  else
    value_print_scalar_formatted (val, options, 0, stream);
}

/* See valprint.h.  */

void
generic_value_print (struct value *val, struct ui_file *stream, int recurse,
		     const struct value_print_options *options,
		     const struct generic_val_print_decorations *decorations)
{
  struct type *type = val->type ();

  type = check_typedef (type);

  if (is_fixed_point_type (type))
    type = type->fixed_point_type_base_type ();

  /* Widen a subrange to its target type, then use that type's
     printer.  */
  while (type->code () == TYPE_CODE_RANGE)
    {
      type = check_typedef (type->target_type ());
      val = value_cast (type, val);
    }

  switch (type->code ())
    {
    case TYPE_CODE_ARRAY:
      generic_val_print_array (val, stream, recurse, options, decorations);
      break;

    case TYPE_CODE_MEMBERPTR:
      generic_value_print_memberptr (val, stream, recurse, options,
				     decorations);
      break;

    case TYPE_CODE_PTR:
      generic_value_print_ptr (val, stream, options);
      break;

    case TYPE_CODE_REF:
    case TYPE_CODE_RVALUE_REF:
      generic_val_print_ref (type, 0, stream, recurse,
			     val, options);
      break;

    case TYPE_CODE_ENUM:
      if (options->format)
	value_print_scalar_formatted (val, options, 0, stream);
      else
	generic_val_print_enum (type, 0, stream, val, options);
      break;

    case TYPE_CODE_FLAGS:
      if (options->format)
	value_print_scalar_formatted (val, options, 0, stream);
      else
	val_print_type_code_flags (type, val, 0, stream);
      break;

    case TYPE_CODE_FUNC:
    case TYPE_CODE_METHOD:
      if (options->format)
	value_print_scalar_formatted (val, options, 0, stream);
      else
	generic_val_print_func (type, 0, val->address (), stream,
				val, options);
      break;

    case TYPE_CODE_BOOL:
      generic_value_print_bool (val, stream, options, decorations);
      break;

    case TYPE_CODE_INT:
      generic_value_print_int (val, stream, options);
      break;

    case TYPE_CODE_CHAR:
      generic_value_print_char (val, stream, options);
      break;

    case TYPE_CODE_FLT:
    case TYPE_CODE_DECFLOAT:
      if (options->format)
	value_print_scalar_formatted (val, options, 0, stream);
      else
	generic_val_print_float (type, stream, val, options);
      break;

    case TYPE_CODE_FIXED_POINT:
      generic_val_print_fixed_point (val, stream, options);
      break;

    case TYPE_CODE_VOID:
      gdb_puts (decorations->void_name, stream);
      break;

    case TYPE_CODE_ERROR:
      gdb_printf (stream, "%s", TYPE_ERROR_NAME (type));
      break;

    case TYPE_CODE_UNDEF:
      /* This happens (without TYPE_STUB set) on systems which don't use
	 dbx xrefs (NO_DBX_XREFS in gcc) if a file has a "struct foo *bar"
	 and no complete type for struct foo in that file.  */
      fprintf_styled (stream, metadata_style.style (), _("<incomplete type>"));
      break;

    case TYPE_CODE_COMPLEX:
      generic_value_print_complex (val, stream, options, decorations);
      break;

    case TYPE_CODE_METHODPTR:
      cplus_print_method_ptr (val->contents_for_printing ().data (), type,
			      stream);
      break;

    case TYPE_CODE_UNION:
    case TYPE_CODE_STRUCT:
    default:
      error (_("Unhandled type code %d in symbol table."),
	     type->code ());
    }
}

/* Print using the given LANGUAGE the value VAL onto stream STREAM according
   to OPTIONS.

   This is a preferable interface to val_print, above, because it uses
   GDB's value mechanism.  */

void
common_val_print (struct value *value, struct ui_file *stream, int recurse,
		  const struct value_print_options *options,
		  const struct language_defn *language)
{
  if (language->la_language == language_ada)
    /* The value might have a dynamic type, which would cause trouble
       below when trying to extract the value contents (since the value
       size is determined from the type size which is unknown).  So
       get a fixed representation of our value.  */
    value = ada_to_fixed_value (value);

  if (value->lazy ())
    value->fetch_lazy ();

  struct value_print_options local_opts = *options;
  struct type *type = value->type ();
  struct type *real_type = check_typedef (type);

  if (local_opts.prettyformat == Val_prettyformat_default)
    local_opts.prettyformat = (local_opts.prettyformat_structs
			       ? Val_prettyformat : Val_no_prettyformat);

  QUIT;

  if (!valprint_check_validity (stream, real_type, 0, value))
    return;

  if (!options->raw)
    {
      if (apply_ext_lang_val_pretty_printer (value, stream, recurse, options,
					     language))
	return;
    }

  /* Ensure that the type is complete and not just a stub.  If the type is
     only a stub and we can't find and substitute its complete type, then
     print appropriate string and return.  */

  if (real_type->is_stub ())
    {
      fprintf_styled (stream, metadata_style.style (), _("<incomplete type>"));
      return;
    }

  /* Handle summary mode.  If the value is a scalar, print it;
     otherwise, print an ellipsis.  */
  if (options->summary && !val_print_scalar_type_p (type))
    {
      gdb_printf (stream, "...");
      return;
    }

  /* If this value is too deep then don't print it.  */
  if (!val_print_scalar_or_string_type_p (type, language)
      && val_print_check_max_depth (stream, recurse, options, language))
    return;

  try
    {
      language->value_print_inner (value, stream, recurse, &local_opts);
    }
  catch (const gdb_exception_error &except)
    {
      fprintf_styled (stream, metadata_style.style (),
		      _("<error reading variable: %s>"), except.what ());
    }
}

/* See valprint.h.  */

bool
val_print_check_max_depth (struct ui_file *stream, int recurse,
			   const struct value_print_options *options,
			   const struct language_defn *language)
{
  if (options->max_depth > -1 && recurse >= options->max_depth)
    {
      gdb_assert (language->struct_too_deep_ellipsis () != NULL);
      gdb_puts (language->struct_too_deep_ellipsis (), stream);
      return true;
    }

  return false;
}

/* Check whether the value VAL is printable.  Return 1 if it is;
   return 0 and print an appropriate error message to STREAM according to
   OPTIONS if it is not.  */

static int
value_check_printable (struct value *val, struct ui_file *stream,
		       const struct value_print_options *options)
{
  if (val == 0)
    {
      fprintf_styled (stream, metadata_style.style (),
		      _("<address of value unknown>"));
      return 0;
    }

  if (val->entirely_optimized_out ())
    {
      if (options->summary && !val_print_scalar_type_p (val->type ()))
	gdb_printf (stream, "...");
      else
	val_print_optimized_out (val, stream);
      return 0;
    }

  if (val->entirely_unavailable ())
    {
      if (options->summary && !val_print_scalar_type_p (val->type ()))
	gdb_printf (stream, "...");
      else
	val_print_unavailable (stream);
      return 0;
    }

  if (val->type ()->code () == TYPE_CODE_INTERNAL_FUNCTION)
    {
      fprintf_styled (stream, metadata_style.style (),
		      _("<internal function %s>"),
		      value_internal_function_name (val));
      return 0;
    }

  if (type_not_associated (val->type ()))
    {
      val_print_not_associated (stream);
      return 0;
    }

  if (type_not_allocated (val->type ()))
    {
      val_print_not_allocated (stream);
      return 0;
    }

  return 1;
}

/* See valprint.h.  */

void
common_val_print_checked (struct value *val, struct ui_file *stream,
			  int recurse,
			  const struct value_print_options *options,
			  const struct language_defn *language)
{
  if (!value_check_printable (val, stream, options))
    return;
  common_val_print (val, stream, recurse, options, language);
}

/* Print on stream STREAM the value VAL according to OPTIONS.  The value
   is printed using the current_language syntax.  */

void
value_print (struct value *val, struct ui_file *stream,
	     const struct value_print_options *options)
{
  scoped_value_mark free_values;

  if (!value_check_printable (val, stream, options))
    return;

  if (!options->raw)
    {
      int r
	= apply_ext_lang_val_pretty_printer (val, stream, 0, options,
					     current_language);

      if (r)
	return;
    }

  current_language->value_print (val, stream, options);
}

/* Meant to be used in debug sessions, so don't export it in a header file.  */
extern void ATTRIBUTE_UNUSED debug_val (struct value *val);

/* Print VAL.  */

void ATTRIBUTE_UNUSED
debug_val (struct value *val)
{
  value_print (val, gdb_stdlog, &user_print_options);
  gdb_flush (gdb_stdlog);
}

static void
val_print_type_code_flags (struct type *type, struct value *original_value,
			   int embedded_offset, struct ui_file *stream)
{
  const gdb_byte *valaddr = (original_value->contents_for_printing ().data ()
			     + embedded_offset);
  ULONGEST val = unpack_long (type, valaddr);
  int field, nfields = type->num_fields ();
  struct gdbarch *gdbarch = type->arch ();
  struct type *bool_type = builtin_type (gdbarch)->builtin_bool;

  gdb_puts ("[", stream);
  for (field = 0; field < nfields; field++)
    {
      if (type->field (field).name ()[0] != '\0')
	{
	  struct type *field_type = type->field (field).type ();

	  if (field_type == bool_type
	      /* We require boolean types here to be one bit wide.  This is a
		 problematic place to notify the user of an internal error
		 though.  Instead just fall through and print the field as an
		 int.  */
	      && type->field (field).bitsize () == 1)
	    {
	      if (val & ((ULONGEST)1 << type->field (field).loc_bitpos ()))
		gdb_printf
		  (stream, " %ps",
		   styled_string (variable_name_style.style (),
				  type->field (field).name ()));
	    }
	  else
	    {
	      unsigned field_len = type->field (field).bitsize ();
	      ULONGEST field_val = val >> type->field (field).loc_bitpos ();

	      if (field_len < sizeof (ULONGEST) * TARGET_CHAR_BIT)
		field_val &= ((ULONGEST) 1 << field_len) - 1;
	      gdb_printf (stream, " %ps=",
			  styled_string (variable_name_style.style (),
					 type->field (field).name ()));
	      if (field_type->code () == TYPE_CODE_ENUM)
		generic_val_print_enum_1 (field_type, field_val, stream);
	      else
		print_longest (stream, 'd', 0, field_val);
	    }
	}
    }
  gdb_puts (" ]", stream);
}

/* See valprint.h.  */

void
value_print_scalar_formatted (struct value *val,
			      const struct value_print_options *options,
			      int size,
			      struct ui_file *stream)
{
  struct type *type = check_typedef (val->type ());

  gdb_assert (val != NULL);

  /* If we get here with a string format, try again without it.  Go
     all the way back to the language printers, which may call us
     again.  */
  if (options->format == 's')
    {
      struct value_print_options opts = *options;
      opts.format = 0;
      opts.deref_ref = false;
      common_val_print (val, stream, 0, &opts, current_language);
      return;
    }

  /* value_contents_for_printing fetches all VAL's contents.  They are
     needed to check whether VAL is optimized-out or unavailable
     below.  */
  const gdb_byte *valaddr = val->contents_for_printing ().data ();

  /* A scalar object that does not have all bits available can't be
     printed, because all bits contribute to its representation.  */
  if (val->bits_any_optimized_out (0,
				   TARGET_CHAR_BIT * type->length ()))
    val_print_optimized_out (val, stream);
  else if (!val->bytes_available (0, type->length ()))
    val_print_unavailable (stream);
  else
    print_scalar_formatted (valaddr, type, options, size, stream);
}

/* Print a number according to FORMAT which is one of d,u,x,o,b,h,w,g.
   The raison d'etre of this function is to consolidate printing of 
   LONG_LONG's into this one function.  The format chars b,h,w,g are 
   from print_scalar_formatted().  Numbers are printed using C
   format.

   USE_C_FORMAT means to use C format in all cases.  Without it, 
   'o' and 'x' format do not include the standard C radix prefix
   (leading 0 or 0x). 
   
   Hilfinger/2004-09-09: USE_C_FORMAT was originally called USE_LOCAL
   and was intended to request formatting according to the current
   language and would be used for most integers that GDB prints.  The
   exceptional cases were things like protocols where the format of
   the integer is a protocol thing, not a user-visible thing).  The
   parameter remains to preserve the information of what things might
   be printed with language-specific format, should we ever resurrect
   that capability.  */

void
print_longest (struct ui_file *stream, int format, int use_c_format,
	       LONGEST val_long)
{
  const char *val;

  switch (format)
    {
    case 'd':
      val = int_string (val_long, 10, 1, 0, 1); break;
    case 'u':
      val = int_string (val_long, 10, 0, 0, 1); break;
    case 'x':
      val = int_string (val_long, 16, 0, 0, use_c_format); break;
    case 'b':
      val = int_string (val_long, 16, 0, 2, 1); break;
    case 'h':
      val = int_string (val_long, 16, 0, 4, 1); break;
    case 'w':
      val = int_string (val_long, 16, 0, 8, 1); break;
    case 'g':
      val = int_string (val_long, 16, 0, 16, 1); break;
      break;
    case 'o':
      val = int_string (val_long, 8, 0, 0, use_c_format); break;
    default:
      internal_error (_("failed internal consistency check"));
    } 
  gdb_puts (val, stream);
}

/* This used to be a macro, but I don't think it is called often enough
   to merit such treatment.  */
/* Convert a LONGEST to an int.  This is used in contexts (e.g. number of
   arguments to a function, number in a value history, register number, etc.)
   where the value must not be larger than can fit in an int.  */

int
longest_to_int (LONGEST arg)
{
  /* Let the compiler do the work.  */
  int rtnval = (int) arg;

  /* Check for overflows or underflows.  */
  if (sizeof (LONGEST) > sizeof (int))
    {
      if (rtnval != arg)
	{
	  error (_("Value out of range."));
	}
    }
  return (rtnval);
}

/* Print a floating point value of floating-point type TYPE,
   pointed to in GDB by VALADDR, on STREAM.  */

void
print_floating (const gdb_byte *valaddr, struct type *type,
		struct ui_file *stream)
{
  std::string str = target_float_to_string (valaddr, type);
  gdb_puts (str.c_str (), stream);
}

void
print_binary_chars (struct ui_file *stream, const gdb_byte *valaddr,
		    unsigned len, enum bfd_endian byte_order, bool zero_pad,
		    const struct value_print_options *options)
{
  const gdb_byte *p;
  unsigned int i;
  int b;
  bool seen_a_one = false;
  const char *digit_separator = nullptr;

  /* Declared "int" so it will be signed.
     This ensures that right shift will shift in zeros.  */

  const int mask = 0x080;

  if (options->nibblesprint)
    digit_separator = current_language->get_digit_separator();

  if (byte_order == BFD_ENDIAN_BIG)
    {
      for (p = valaddr;
	   p < valaddr + len;
	   p++)
	{
	  /* Every byte has 8 binary characters; peel off
	     and print from the MSB end.  */

	  for (i = 0; i < (HOST_CHAR_BIT * sizeof (*p)); i++)
	    {
	      if (options->nibblesprint && seen_a_one && i % 4 == 0)
		gdb_putc (*digit_separator, stream);

	      if (*p & (mask >> i))
		b = '1';
	      else
		b = '0';

	      if (zero_pad || seen_a_one || b == '1')
		gdb_putc (b, stream);
	      else if (options->nibblesprint)
		{
		  if ((0xf0 & (mask >> i) && (*p & 0xf0))
		      || (0x0f & (mask >> i) && (*p & 0x0f)))
		    gdb_putc (b, stream);
		}

	      if (b == '1')
		seen_a_one = true;
	    }
	}
    }
  else
    {
      for (p = valaddr + len - 1;
	   p >= valaddr;
	   p--)
	{
	  for (i = 0; i < (HOST_CHAR_BIT * sizeof (*p)); i++)
	    {
	      if (options->nibblesprint && seen_a_one && i % 4 == 0)
		gdb_putc (*digit_separator, stream);

	      if (*p & (mask >> i))
		b = '1';
	      else
		b = '0';

	      if (zero_pad || seen_a_one || b == '1')
		gdb_putc (b, stream);
	      else if (options->nibblesprint)
		{
		  if ((0xf0 & (mask >> i) && (*p & 0xf0))
		      || (0x0f & (mask >> i) && (*p & 0x0f)))
		    gdb_putc (b, stream);
		}

	      if (b == '1')
		seen_a_one = true;
	    }
	}
    }

  /* When not zero-padding, ensure that something is printed when the
     input is 0.  */
  if (!zero_pad && !seen_a_one)
    gdb_putc ('0', stream);
}

/* A helper for print_octal_chars that emits a single octal digit,
   optionally suppressing it if is zero and updating SEEN_A_ONE.  */

static void
emit_octal_digit (struct ui_file *stream, bool *seen_a_one, int digit)
{
  if (*seen_a_one || digit != 0)
    gdb_printf (stream, "%o", digit);
  if (digit != 0)
    *seen_a_one = true;
}

/* VALADDR points to an integer of LEN bytes.
   Print it in octal on stream or format it in buf.  */

void
print_octal_chars (struct ui_file *stream, const gdb_byte *valaddr,
		   unsigned len, enum bfd_endian byte_order)
{
  const gdb_byte *p;
  unsigned char octa1, octa2, octa3, carry;
  int cycle;

  /* Octal is 3 bits, which doesn't fit.  Yuk.  So we have to track
   * the extra bits, which cycle every three bytes:
   *
   * Byte side:       0            1             2          3
   *                         |             |            |            |
   * bit number   123 456 78 | 9 012 345 6 | 78 901 234 | 567 890 12 |
   *
   * Octal side:   0   1   carry  3   4  carry ...
   *
   * Cycle number:    0             1            2
   *
   * But of course we are printing from the high side, so we have to
   * figure out where in the cycle we are so that we end up with no
   * left over bits at the end.
   */
#define BITS_IN_OCTAL 3
#define HIGH_ZERO     0340
#define LOW_ZERO      0034
#define CARRY_ZERO    0003
  static_assert (HIGH_ZERO + LOW_ZERO + CARRY_ZERO == 0xff,
		 "cycle zero constants are wrong");
#define HIGH_ONE      0200
#define MID_ONE       0160
#define LOW_ONE       0016
#define CARRY_ONE     0001
  static_assert (HIGH_ONE + MID_ONE + LOW_ONE + CARRY_ONE == 0xff,
		 "cycle one constants are wrong");
#define HIGH_TWO      0300
#define MID_TWO       0070
#define LOW_TWO       0007
  static_assert (HIGH_TWO + MID_TWO + LOW_TWO == 0xff,
		 "cycle two constants are wrong");

  /* For 32 we start in cycle 2, with two bits and one bit carry;
     for 64 in cycle in cycle 1, with one bit and a two bit carry.  */

  cycle = (len * HOST_CHAR_BIT) % BITS_IN_OCTAL;
  carry = 0;

  gdb_puts ("0", stream);
  bool seen_a_one = false;
  if (byte_order == BFD_ENDIAN_BIG)
    {
      for (p = valaddr;
	   p < valaddr + len;
	   p++)
	{
	  switch (cycle)
	    {
	    case 0:
	      /* No carry in, carry out two bits.  */

	      octa1 = (HIGH_ZERO & *p) >> 5;
	      octa2 = (LOW_ZERO & *p) >> 2;
	      carry = (CARRY_ZERO & *p);
	      emit_octal_digit (stream, &seen_a_one, octa1);
	      emit_octal_digit (stream, &seen_a_one, octa2);
	      break;

	    case 1:
	      /* Carry in two bits, carry out one bit.  */

	      octa1 = (carry << 1) | ((HIGH_ONE & *p) >> 7);
	      octa2 = (MID_ONE & *p) >> 4;
	      octa3 = (LOW_ONE & *p) >> 1;
	      carry = (CARRY_ONE & *p);
	      emit_octal_digit (stream, &seen_a_one, octa1);
	      emit_octal_digit (stream, &seen_a_one, octa2);
	      emit_octal_digit (stream, &seen_a_one, octa3);
	      break;

	    case 2:
	      /* Carry in one bit, no carry out.  */

	      octa1 = (carry << 2) | ((HIGH_TWO & *p) >> 6);
	      octa2 = (MID_TWO & *p) >> 3;
	      octa3 = (LOW_TWO & *p);
	      carry = 0;
	      emit_octal_digit (stream, &seen_a_one, octa1);
	      emit_octal_digit (stream, &seen_a_one, octa2);
	      emit_octal_digit (stream, &seen_a_one, octa3);
	      break;

	    default:
	      error (_("Internal error in octal conversion;"));
	    }

	  cycle++;
	  cycle = cycle % BITS_IN_OCTAL;
	}
    }
  else
    {
      for (p = valaddr + len - 1;
	   p >= valaddr;
	   p--)
	{
	  switch (cycle)
	    {
	    case 0:
	      /* Carry out, no carry in */

	      octa1 = (HIGH_ZERO & *p) >> 5;
	      octa2 = (LOW_ZERO & *p) >> 2;
	      carry = (CARRY_ZERO & *p);
	      emit_octal_digit (stream, &seen_a_one, octa1);
	      emit_octal_digit (stream, &seen_a_one, octa2);
	      break;

	    case 1:
	      /* Carry in, carry out */

	      octa1 = (carry << 1) | ((HIGH_ONE & *p) >> 7);
	      octa2 = (MID_ONE & *p) >> 4;
	      octa3 = (LOW_ONE & *p) >> 1;
	      carry = (CARRY_ONE & *p);
	      emit_octal_digit (stream, &seen_a_one, octa1);
	      emit_octal_digit (stream, &seen_a_one, octa2);
	      emit_octal_digit (stream, &seen_a_one, octa3);
	      break;

	    case 2:
	      /* Carry in, no carry out */

	      octa1 = (carry << 2) | ((HIGH_TWO & *p) >> 6);
	      octa2 = (MID_TWO & *p) >> 3;
	      octa3 = (LOW_TWO & *p);
	      carry = 0;
	      emit_octal_digit (stream, &seen_a_one, octa1);
	      emit_octal_digit (stream, &seen_a_one, octa2);
	      emit_octal_digit (stream, &seen_a_one, octa3);
	      break;

	    default:
	      error (_("Internal error in octal conversion;"));
	    }

	  cycle++;
	  cycle = cycle % BITS_IN_OCTAL;
	}
    }

}

/* Possibly negate the integer represented by BYTES.  It contains LEN
   bytes in the specified byte order.  If the integer is negative,
   copy it into OUT_VEC, negate it, and return true.  Otherwise, do
   nothing and return false.  */

static bool
maybe_negate_by_bytes (const gdb_byte *bytes, unsigned len,
		       enum bfd_endian byte_order,
		       gdb::byte_vector *out_vec)
{
  gdb_byte sign_byte;
  gdb_assert (len > 0);
  if (byte_order == BFD_ENDIAN_BIG)
    sign_byte = bytes[0];
  else
    sign_byte = bytes[len - 1];
  if ((sign_byte & 0x80) == 0)
    return false;

  out_vec->resize (len);

  /* Compute -x == 1 + ~x.  */
  if (byte_order == BFD_ENDIAN_LITTLE)
    {
      unsigned carry = 1;
      for (unsigned i = 0; i < len; ++i)
	{
	  unsigned tem = (0xff & ~bytes[i]) + carry;
	  (*out_vec)[i] = tem & 0xff;
	  carry = tem / 256;
	}
    }
  else
    {
      unsigned carry = 1;
      for (unsigned i = len; i > 0; --i)
	{
	  unsigned tem = (0xff & ~bytes[i - 1]) + carry;
	  (*out_vec)[i - 1] = tem & 0xff;
	  carry = tem / 256;
	}
    }

  return true;
}

/* VALADDR points to an integer of LEN bytes.
   Print it in decimal on stream or format it in buf.  */

void
print_decimal_chars (struct ui_file *stream, const gdb_byte *valaddr,
		     unsigned len, bool is_signed,
		     enum bfd_endian byte_order)
{
#define TEN             10
#define CARRY_OUT(  x ) ((x) / TEN)	/* extend char to int */
#define CARRY_LEFT( x ) ((x) % TEN)
#define SHIFT( x )      ((x) << 4)
#define LOW_NIBBLE(  x ) ( (x) & 0x00F)
#define HIGH_NIBBLE( x ) (((x) & 0x0F0) >> 4)

  const gdb_byte *p;
  int carry;
  int decimal_len;
  int i, j, decimal_digits;
  int dummy;
  int flip;

  gdb::byte_vector negated_bytes;
  if (is_signed
      && maybe_negate_by_bytes (valaddr, len, byte_order, &negated_bytes))
    {
      gdb_puts ("-", stream);
      valaddr = negated_bytes.data ();
    }

  /* Base-ten number is less than twice as many digits
     as the base 16 number, which is 2 digits per byte.  */

  decimal_len = len * 2 * 2;
  std::vector<unsigned char> digits (decimal_len, 0);

  /* Ok, we have an unknown number of bytes of data to be printed in
   * decimal.
   *
   * Given a hex number (in nibbles) as XYZ, we start by taking X and
   * decimalizing it as "x1 x2" in two decimal nibbles.  Then we multiply
   * the nibbles by 16, add Y and re-decimalize.  Repeat with Z.
   *
   * The trick is that "digits" holds a base-10 number, but sometimes
   * the individual digits are > 10.
   *
   * Outer loop is per nibble (hex digit) of input, from MSD end to
   * LSD end.
   */
  decimal_digits = 0;		/* Number of decimal digits so far */
  p = (byte_order == BFD_ENDIAN_BIG) ? valaddr : valaddr + len - 1;
  flip = 0;
  while ((byte_order == BFD_ENDIAN_BIG) ? (p < valaddr + len) : (p >= valaddr))
    {
      /*
       * Multiply current base-ten number by 16 in place.
       * Each digit was between 0 and 9, now is between
       * 0 and 144.
       */
      for (j = 0; j < decimal_digits; j++)
	{
	  digits[j] = SHIFT (digits[j]);
	}

      /* Take the next nibble off the input and add it to what
       * we've got in the LSB position.  Bottom 'digit' is now
       * between 0 and 159.
       *
       * "flip" is used to run this loop twice for each byte.
       */
      if (flip == 0)
	{
	  /* Take top nibble.  */

	  digits[0] += HIGH_NIBBLE (*p);
	  flip = 1;
	}
      else
	{
	  /* Take low nibble and bump our pointer "p".  */

	  digits[0] += LOW_NIBBLE (*p);
	  if (byte_order == BFD_ENDIAN_BIG)
	    p++;
	  else
	    p--;
	  flip = 0;
	}

      /* Re-decimalize.  We have to do this often enough
       * that we don't overflow, but once per nibble is
       * overkill.  Easier this way, though.  Note that the
       * carry is often larger than 10 (e.g. max initial
       * carry out of lowest nibble is 15, could bubble all
       * the way up greater than 10).  So we have to do
       * the carrying beyond the last current digit.
       */
      carry = 0;
      for (j = 0; j < decimal_len - 1; j++)
	{
	  digits[j] += carry;

	  /* "/" won't handle an unsigned char with
	   * a value that if signed would be negative.
	   * So extend to longword int via "dummy".
	   */
	  dummy = digits[j];
	  carry = CARRY_OUT (dummy);
	  digits[j] = CARRY_LEFT (dummy);

	  if (j >= decimal_digits && carry == 0)
	    {
	      /*
	       * All higher digits are 0 and we
	       * no longer have a carry.
	       *
	       * Note: "j" is 0-based, "decimal_digits" is
	       *       1-based.
	       */
	      decimal_digits = j + 1;
	      break;
	    }
	}
    }

  /* Ok, now "digits" is the decimal representation, with
     the "decimal_digits" actual digits.  Print!  */

  for (i = decimal_digits - 1; i > 0 && digits[i] == 0; --i)
    ;

  for (; i >= 0; i--)
    {
      gdb_printf (stream, "%1d", digits[i]);
    }
}

/* VALADDR points to an integer of LEN bytes.  Print it in hex on stream.  */

void
print_hex_chars (struct ui_file *stream, const gdb_byte *valaddr,
		 unsigned len, enum bfd_endian byte_order,
		 bool zero_pad)
{
  const gdb_byte *p;

  gdb_puts ("0x", stream);
  if (byte_order == BFD_ENDIAN_BIG)
    {
      p = valaddr;

      if (!zero_pad)
	{
	  /* Strip leading 0 bytes, but be sure to leave at least a
	     single byte at the end.  */
	  for (; p < valaddr + len - 1 && !*p; ++p)
	    ;
	}

      const gdb_byte *first = p;
      for (;
	   p < valaddr + len;
	   p++)
	{
	  /* When not zero-padding, use a different format for the
	     very first byte printed.  */
	  if (!zero_pad && p == first)
	    gdb_printf (stream, "%x", *p);
	  else
	    gdb_printf (stream, "%02x", *p);
	}
    }
  else
    {
      p = valaddr + len - 1;

      if (!zero_pad)
	{
	  /* Strip leading 0 bytes, but be sure to leave at least a
	     single byte at the end.  */
	  for (; p >= valaddr + 1 && !*p; --p)
	    ;
	}

      const gdb_byte *first = p;
      for (;
	   p >= valaddr;
	   p--)
	{
	  /* When not zero-padding, use a different format for the
	     very first byte printed.  */
	  if (!zero_pad && p == first)
	    gdb_printf (stream, "%x", *p);
	  else
	    gdb_printf (stream, "%02x", *p);
	}
    }
}

/* Print function pointer with inferior address ADDRESS onto stdio
   stream STREAM.  */

void
print_function_pointer_address (const struct value_print_options *options,
				struct gdbarch *gdbarch,
				CORE_ADDR address,
				struct ui_file *stream)
{
  CORE_ADDR func_addr = gdbarch_convert_from_func_ptr_addr
    (gdbarch, address, current_inferior ()->top_target ());

  /* If the function pointer is represented by a description, print
     the address of the description.  */
  if (options->addressprint && func_addr != address)
    {
      gdb_puts ("@", stream);
      gdb_puts (paddress (gdbarch, address), stream);
      gdb_puts (": ", stream);
    }
  print_address_demangle (options, gdbarch, func_addr, stream, demangle);
}


/* Print on STREAM using the given OPTIONS the index for the element
   at INDEX of an array whose index type is INDEX_TYPE.  */
    
void  
maybe_print_array_index (struct type *index_type, LONGEST index,
			 struct ui_file *stream,
			 const struct value_print_options *options)
{
  if (!options->print_array_indexes)
    return; 

  current_language->print_array_index (index_type, index, stream, options);
}

/* See valprint.h.  */

void
value_print_array_elements (struct value *val, struct ui_file *stream,
			    int recurse,
			    const struct value_print_options *options,
			    unsigned int i)
{
  unsigned int things_printed = 0;
  unsigned len;
  struct type *elttype, *index_type;
  /* Position of the array element we are examining to see
     whether it is repeated.  */
  unsigned int rep1;
  /* Number of repetitions we have detected so far.  */
  unsigned int reps;
  LONGEST low_bound, high_bound;

  struct type *type = check_typedef (val->type ());

  elttype = type->target_type ();
  unsigned bit_stride = type->bit_stride ();
  if (bit_stride == 0)
    bit_stride = 8 * check_typedef (elttype)->length ();
  index_type = type->index_type ();
  if (index_type->code () == TYPE_CODE_RANGE)
    index_type = index_type->target_type ();

  if (get_array_bounds (type, &low_bound, &high_bound))
    {
      /* The array length should normally be HIGH_BOUND - LOW_BOUND +
	 1.  But we have to be a little extra careful, because some
	 languages such as Ada allow LOW_BOUND to be greater than
	 HIGH_BOUND for empty arrays.  In that situation, the array
	 length is just zero, not negative!  */
      if (low_bound > high_bound)
	len = 0;
      else
	len = high_bound - low_bound + 1;
    }
  else
    {
      warning (_("unable to get bounds of array, assuming null array"));
      low_bound = 0;
      len = 0;
    }

  annotate_array_section_begin (i, elttype);

  for (; i < len && things_printed < options->print_max; i++)
    {
      scoped_value_mark free_values;

      if (i != 0)
	{
	  if (options->prettyformat_arrays)
	    {
	      gdb_printf (stream, ",\n");
	      print_spaces (2 + 2 * recurse, stream);
	    }
	  else
	    gdb_printf (stream, ", ");
	}
      else if (options->prettyformat_arrays)
	{
	  gdb_printf (stream, "\n");
	  print_spaces (2 + 2 * recurse, stream);
	}
      stream->wrap_here (2 + 2 * recurse);
      maybe_print_array_index (index_type, i + low_bound,
			       stream, options);

      struct value *element = val->from_component_bitsize (elttype,
							   bit_stride * i,
							   bit_stride);
      rep1 = i + 1;
      reps = 1;
      /* Only check for reps if repeat_count_threshold is not set to
	 UINT_MAX (unlimited).  */
      if (options->repeat_count_threshold < UINT_MAX)
	{
	  bool unavailable = element->entirely_unavailable ();
	  bool available = element->entirely_available ();

	  while (rep1 < len)
	    {
	      /* When printing large arrays this spot is called frequently, so
		 clean up temporary values asap to prevent allocating a large
		 amount of them.  */
	      scoped_value_mark free_values_inner;
	      struct value *rep_elt
		= val->from_component_bitsize (elttype,
					       rep1 * bit_stride,
					       bit_stride);
	      bool repeated = ((available
				&& rep_elt->entirely_available ()
				&& element->contents_eq (rep_elt))
			       || (unavailable
				   && rep_elt->entirely_unavailable ()));
	      if (!repeated)
		break;
	      ++reps;
	      ++rep1;
	    }
	}

      common_val_print (element, stream, recurse + 1, options,
			current_language);

      if (reps > options->repeat_count_threshold)
	{
	  annotate_elt_rep (reps);
	  gdb_printf (stream, " %p[<repeats %u times>%p]",
		      metadata_style.style ().ptr (), reps, nullptr);
	  annotate_elt_rep_end ();

	  i = rep1 - 1;
	  things_printed += options->repeat_count_threshold;
	}
      else
	{
	  annotate_elt ();
	  things_printed++;
	}
    }
  annotate_array_section_end ();
  if (i < len)
    gdb_printf (stream, "...");
  if (options->prettyformat_arrays)
    {
      gdb_printf (stream, "\n");
      print_spaces (2 * recurse, stream);
    }
}

/* Return true if print_wchar can display W without resorting to a
   numeric escape, false otherwise.  */

static int
wchar_printable (gdb_wchar_t w)
{
  return (gdb_iswprint (w)
	  || w == LCST ('\a') || w == LCST ('\b')
	  || w == LCST ('\f') || w == LCST ('\n')
	  || w == LCST ('\r') || w == LCST ('\t')
	  || w == LCST ('\v'));
}

/* A helper function that converts the contents of STRING to wide
   characters and then appends them to OUTPUT.  */

static void
append_string_as_wide (const char *string,
		       struct obstack *output)
{
  for (; *string; ++string)
    {
      gdb_wchar_t w = gdb_btowc (*string);
      obstack_grow (output, &w, sizeof (gdb_wchar_t));
    }
}

/* Print a wide character W to OUTPUT.  ORIG is a pointer to the
   original (target) bytes representing the character, ORIG_LEN is the
   number of valid bytes.  WIDTH is the number of bytes in a base
   characters of the type.  OUTPUT is an obstack to which wide
   characters are emitted.  QUOTER is a (narrow) character indicating
   the style of quotes surrounding the character to be printed.
   NEED_ESCAPE is an in/out flag which is used to track numeric
   escapes across calls.  */

static void
print_wchar (gdb_wint_t w, const gdb_byte *orig,
	     int orig_len, int width,
	     enum bfd_endian byte_order,
	     struct obstack *output,
	     int quoter, bool *need_escapep)
{
  bool need_escape = *need_escapep;

  *need_escapep = false;

  /* If any additional cases are added to this switch block, then the
     function wchar_printable will likely need updating too.  */
  switch (w)
    {
      case LCST ('\a'):
	obstack_grow_wstr (output, LCST ("\\a"));
	break;
      case LCST ('\b'):
	obstack_grow_wstr (output, LCST ("\\b"));
	break;
      case LCST ('\f'):
	obstack_grow_wstr (output, LCST ("\\f"));
	break;
      case LCST ('\n'):
	obstack_grow_wstr (output, LCST ("\\n"));
	break;
      case LCST ('\r'):
	obstack_grow_wstr (output, LCST ("\\r"));
	break;
      case LCST ('\t'):
	obstack_grow_wstr (output, LCST ("\\t"));
	break;
      case LCST ('\v'):
	obstack_grow_wstr (output, LCST ("\\v"));
	break;
      default:
	{
	  if (gdb_iswprint (w) && !(need_escape && gdb_iswxdigit (w)))
	    {
	      gdb_wchar_t wchar = w;

	      if (w == gdb_btowc (quoter) || w == LCST ('\\'))
		obstack_grow_wstr (output, LCST ("\\"));
	      obstack_grow (output, &wchar, sizeof (gdb_wchar_t));
	    }
	  else
	    {
	      int i;

	      for (i = 0; i + width <= orig_len; i += width)
		{
		  char octal[30];
		  ULONGEST value;

		  value = extract_unsigned_integer (&orig[i], width,
						  byte_order);
		  /* If the value fits in 3 octal digits, print it that
		     way.  Otherwise, print it as a hex escape.  */
		  if (value <= 0777)
		    {
		      xsnprintf (octal, sizeof (octal), "\\%.3o",
				 (int) (value & 0777));
		      *need_escapep = false;
		    }
		  else
		    {
		      xsnprintf (octal, sizeof (octal), "\\x%lx", (long) value);
		      /* A hex escape might require the next character
			 to be escaped, because, unlike with octal,
			 hex escapes have no length limit.  */
		      *need_escapep = true;
		    }
		  append_string_as_wide (octal, output);
		}
	      /* If we somehow have extra bytes, print them now.  */
	      while (i < orig_len)
		{
		  char octal[5];

		  xsnprintf (octal, sizeof (octal), "\\%.3o", orig[i] & 0xff);
		  *need_escapep = false;
		  append_string_as_wide (octal, output);
		  ++i;
		}
	    }
	  break;
	}
    }
}

/* Print the character C on STREAM as part of the contents of a
   literal string whose delimiter is QUOTER.  ENCODING names the
   encoding of C.  */

void
generic_emit_char (int c, struct type *type, struct ui_file *stream,
		   int quoter, const char *encoding)
{
  enum bfd_endian byte_order
    = type_byte_order (type);
  gdb_byte *c_buf;
  bool need_escape = false;

  c_buf = (gdb_byte *) alloca (type->length ());
  pack_long (c_buf, type, c);

  wchar_iterator iter (c_buf, type->length (), encoding, type->length ());

  /* This holds the printable form of the wchar_t data.  */
  auto_obstack wchar_buf;

  while (1)
    {
      int num_chars;
      gdb_wchar_t *chars;
      const gdb_byte *buf;
      size_t buflen;
      int print_escape = 1;
      enum wchar_iterate_result result;

      num_chars = iter.iterate (&result, &chars, &buf, &buflen);
      if (num_chars < 0)
	break;
      if (num_chars > 0)
	{
	  /* If all characters are printable, print them.  Otherwise,
	     we're going to have to print an escape sequence.  We
	     check all characters because we want to print the target
	     bytes in the escape sequence, and we don't know character
	     boundaries there.  */
	  int i;

	  print_escape = 0;
	  for (i = 0; i < num_chars; ++i)
	    if (!wchar_printable (chars[i]))
	      {
		print_escape = 1;
		break;
	      }

	  if (!print_escape)
	    {
	      for (i = 0; i < num_chars; ++i)
		print_wchar (chars[i], buf, buflen,
			     type->length (), byte_order,
			     &wchar_buf, quoter, &need_escape);
	    }
	}

      /* This handles the NUM_CHARS == 0 case as well.  */
      if (print_escape)
	print_wchar (gdb_WEOF, buf, buflen, type->length (),
		     byte_order, &wchar_buf, quoter, &need_escape);
    }

  /* The output in the host encoding.  */
  auto_obstack output;

  convert_between_encodings (INTERMEDIATE_ENCODING, host_charset (),
			     (gdb_byte *) obstack_base (&wchar_buf),
			     obstack_object_size (&wchar_buf),
			     sizeof (gdb_wchar_t), &output, translit_char);
  obstack_1grow (&output, '\0');

  gdb_puts ((const char *) obstack_base (&output), stream);
}

/* Return the repeat count of the next character/byte in ITER,
   storing the result in VEC.  */

static int
count_next_character (wchar_iterator *iter,
		      std::vector<converted_character> *vec)
{
  struct converted_character *current;

  if (vec->empty ())
    {
      struct converted_character tmp;
      gdb_wchar_t *chars;

      tmp.num_chars
	= iter->iterate (&tmp.result, &chars, &tmp.buf, &tmp.buflen);
      if (tmp.num_chars > 0)
	{
	  gdb_assert (tmp.num_chars < MAX_WCHARS);
	  memcpy (tmp.chars, chars, tmp.num_chars * sizeof (gdb_wchar_t));
	}
      vec->push_back (tmp);
    }

  current = &vec->back ();

  /* Count repeated characters or bytes.  */
  current->repeat_count = 1;
  if (current->num_chars == -1)
    {
      /* EOF  */
      return -1;
    }
  else
    {
      gdb_wchar_t *chars;
      struct converted_character d;
      int repeat;

      d.repeat_count = 0;

      while (1)
	{
	  /* Get the next character.  */
	  d.num_chars = iter->iterate (&d.result, &chars, &d.buf, &d.buflen);

	  /* If a character was successfully converted, save the character
	     into the converted character.  */
	  if (d.num_chars > 0)
	    {
	      gdb_assert (d.num_chars < MAX_WCHARS);
	      memcpy (d.chars, chars, WCHAR_BUFLEN (d.num_chars));
	    }

	  /* Determine if the current character is the same as this
	     new character.  */
	  if (d.num_chars == current->num_chars && d.result == current->result)
	    {
	      /* There are two cases to consider:

		 1) Equality of converted character (num_chars > 0)
		 2) Equality of non-converted character (num_chars == 0)  */
	      if ((current->num_chars > 0
		   && memcmp (current->chars, d.chars,
			      WCHAR_BUFLEN (current->num_chars)) == 0)
		  || (current->num_chars == 0
		      && current->buflen == d.buflen
		      && memcmp (current->buf, d.buf, current->buflen) == 0))
		++current->repeat_count;
	      else
		break;
	    }
	  else
	    break;
	}

      /* Push this next converted character onto the result vector.  */
      repeat = current->repeat_count;
      vec->push_back (d);
      return repeat;
    }
}

/* Print the characters in CHARS to the OBSTACK.  QUOTE_CHAR is the quote
   character to use with string output.  WIDTH is the size of the output
   character type.  BYTE_ORDER is the target byte order.  OPTIONS
   is the user's print options.  *FINISHED is set to 0 if we didn't print
   all the elements in CHARS.  */

static void
print_converted_chars_to_obstack (struct obstack *obstack,
				  const std::vector<converted_character> &chars,
				  int quote_char, int width,
				  enum bfd_endian byte_order,
				  const struct value_print_options *options,
				  int *finished)
{
  unsigned int idx, num_elements;
  const converted_character *elem;
  enum {START, SINGLE, REPEAT, INCOMPLETE, FINISH} state, last;
  gdb_wchar_t wide_quote_char = gdb_btowc (quote_char);
  bool need_escape = false;
  const int print_max = options->print_max_chars > 0
      ? options->print_max_chars : options->print_max;

  /* Set the start state.  */
  idx = num_elements = 0;
  last = state = START;
  elem = NULL;

  while (1)
    {
      switch (state)
	{
	case START:
	  /* Nothing to do.  */
	  break;

	case SINGLE:
	  {
	    int j;

	    /* We are outputting a single character
	       (< options->repeat_count_threshold).  */

	    if (last != SINGLE)
	      {
		/* We were outputting some other type of content, so we
		   must output and a comma and a quote.  */
		if (last != START)
		  obstack_grow_wstr (obstack, LCST (", "));
		obstack_grow (obstack, &wide_quote_char, sizeof (gdb_wchar_t));
	      }
	    /* Output the character.  */
	    int repeat_count = elem->repeat_count;
	    if (print_max < repeat_count + num_elements)
	      {
		repeat_count = print_max - num_elements;
		*finished = 0;
	      }
	    for (j = 0; j < repeat_count; ++j)
	      {
		if (elem->result == wchar_iterate_ok)
		  print_wchar (elem->chars[0], elem->buf, elem->buflen, width,
			       byte_order, obstack, quote_char, &need_escape);
		else
		  print_wchar (gdb_WEOF, elem->buf, elem->buflen, width,
			       byte_order, obstack, quote_char, &need_escape);
		num_elements += 1;
	      }
	  }
	  break;

	case REPEAT:
	  {
	    int j;

	    /* We are outputting a character with a repeat count
	       greater than options->repeat_count_threshold.  */

	    if (last == SINGLE)
	      {
		/* We were outputting a single string.  Terminate the
		   string.  */
		obstack_grow (obstack, &wide_quote_char, sizeof (gdb_wchar_t));
	      }
	    if (last != START)
	      obstack_grow_wstr (obstack, LCST (", "));

	    /* Output the character and repeat string.  */
	    obstack_grow_wstr (obstack, LCST ("'"));
	    if (elem->result == wchar_iterate_ok)
	      print_wchar (elem->chars[0], elem->buf, elem->buflen, width,
			   byte_order, obstack, quote_char, &need_escape);
	    else
	      print_wchar (gdb_WEOF, elem->buf, elem->buflen, width,
			   byte_order, obstack, quote_char, &need_escape);
	    obstack_grow_wstr (obstack, LCST ("'"));
	    std::string s = string_printf (_(" <repeats %u times>"),
					   elem->repeat_count);
	    num_elements += elem->repeat_count;
	    for (j = 0; s[j]; ++j)
	      {
		gdb_wchar_t w = gdb_btowc (s[j]);
		obstack_grow (obstack, &w, sizeof (gdb_wchar_t));
	      }
	  }
	  break;

	case INCOMPLETE:
	  /* We are outputting an incomplete sequence.  */
	  if (last == SINGLE)
	    {
	      /* If we were outputting a string of SINGLE characters,
		 terminate the quote.  */
	      obstack_grow (obstack, &wide_quote_char, sizeof (gdb_wchar_t));
	    }
	  if (last != START)
	    obstack_grow_wstr (obstack, LCST (", "));

	  /* Output the incomplete sequence string.  */
	  obstack_grow_wstr (obstack, LCST ("<incomplete sequence "));
	  print_wchar (gdb_WEOF, elem->buf, elem->buflen, width, byte_order,
		       obstack, 0, &need_escape);
	  obstack_grow_wstr (obstack, LCST (">"));
	  num_elements += 1;

	  /* We do not attempt to output anything after this.  */
	  state = FINISH;
	  break;

	case FINISH:
	  /* All done.  If we were outputting a string of SINGLE
	     characters, the string must be terminated.  Otherwise,
	     REPEAT and INCOMPLETE are always left properly terminated.  */
	  if (last == SINGLE)
	    obstack_grow (obstack, &wide_quote_char, sizeof (gdb_wchar_t));

	  return;
	}

      /* Get the next element and state.  */
      last = state;
      if (state != FINISH)
	{
	  elem = &chars[idx++];
	  switch (elem->result)
	    {
	    case wchar_iterate_ok:
	    case wchar_iterate_invalid:
	      if (elem->repeat_count > options->repeat_count_threshold)
		state = REPEAT;
	      else
		state = SINGLE;
	      break;

	    case wchar_iterate_incomplete:
	      state = INCOMPLETE;
	      break;

	    case wchar_iterate_eof:
	      state = FINISH;
	      break;
	    }
	}
    }
}

/* Print the character string STRING, printing at most LENGTH
   characters.  LENGTH is -1 if the string is nul terminated.  TYPE is
   the type of each character.  OPTIONS holds the printing options;
   printing stops early if the number hits print_max_chars; repeat
   counts are printed as appropriate.  Print ellipses at the end if we
   had to stop before printing LENGTH characters, or if FORCE_ELLIPSES.
   QUOTE_CHAR is the character to print at each end of the string.  If
   C_STYLE_TERMINATOR is true, and the last character is 0, then it is
   omitted.  */

void
generic_printstr (struct ui_file *stream, struct type *type, 
		  const gdb_byte *string, unsigned int length, 
		  const char *encoding, int force_ellipses,
		  int quote_char, int c_style_terminator,
		  const struct value_print_options *options)
{
  enum bfd_endian byte_order = type_byte_order (type);
  unsigned int i;
  int width = type->length ();
  int finished = 0;
  struct converted_character *last;

  if (length == -1)
    {
      unsigned long current_char = 1;

      for (i = 0; current_char; ++i)
	{
	  QUIT;
	  current_char = extract_unsigned_integer (string + i * width,
						   width, byte_order);
	}
      length = i;
    }

  /* If the string was not truncated due to `set print elements', and
     the last byte of it is a null, we don't print that, in
     traditional C style.  */
  if (c_style_terminator
      && !force_ellipses
      && length > 0
      && (extract_unsigned_integer (string + (length - 1) * width,
				    width, byte_order) == 0))
    length--;

  if (length == 0)
    {
      gdb_printf (stream, "%c%c", quote_char, quote_char);
      return;
    }

  /* Arrange to iterate over the characters, in wchar_t form.  */
  wchar_iterator iter (string, length * width, encoding, width);
  std::vector<converted_character> converted_chars;

  /* Convert characters until the string is over or the maximum
     number of printed characters has been reached.  */
  i = 0;
  unsigned int print_max_chars = get_print_max_chars (options);
  while (i < print_max_chars)
    {
      int r;

      QUIT;

      /* Grab the next character and repeat count.  */
      r = count_next_character (&iter, &converted_chars);

      /* If less than zero, the end of the input string was reached.  */
      if (r < 0)
	break;

      /* Otherwise, add the count to the total print count and get
	 the next character.  */
      i += r;
    }

  /* Get the last element and determine if the entire string was
     processed.  */
  last = &converted_chars.back ();
  finished = (last->result == wchar_iterate_eof);

  /* Ensure that CONVERTED_CHARS is terminated.  */
  last->result = wchar_iterate_eof;

  /* WCHAR_BUF is the obstack we use to represent the string in
     wchar_t form.  */
  auto_obstack wchar_buf;

  /* Print the output string to the obstack.  */
  print_converted_chars_to_obstack (&wchar_buf, converted_chars, quote_char,
				    width, byte_order, options, &finished);

  if (force_ellipses || !finished)
    obstack_grow_wstr (&wchar_buf, LCST ("..."));

  /* OUTPUT is where we collect `char's for printing.  */
  auto_obstack output;

  convert_between_encodings (INTERMEDIATE_ENCODING, host_charset (),
			     (gdb_byte *) obstack_base (&wchar_buf),
			     obstack_object_size (&wchar_buf),
			     sizeof (gdb_wchar_t), &output, translit_char);
  obstack_1grow (&output, '\0');

  gdb_puts ((const char *) obstack_base (&output), stream);
}

/* Print a string from the inferior, starting at ADDR and printing up to LEN
   characters, of WIDTH bytes a piece, to STREAM.  If LEN is -1, printing
   stops at the first null byte, otherwise printing proceeds (including null
   bytes) until either print_max_chars or LEN characters have been printed,
   whichever is smaller.  ENCODING is the name of the string's
   encoding.  It can be NULL, in which case the target encoding is
   assumed.  */

int
val_print_string (struct type *elttype, const char *encoding,
		  CORE_ADDR addr, int len,
		  struct ui_file *stream,
		  const struct value_print_options *options)
{
  int force_ellipsis = 0;	/* Force ellipsis to be printed if nonzero.  */
  int err;			/* Non-zero if we got a bad read.  */
  int found_nul;		/* Non-zero if we found the nul char.  */
  unsigned int fetchlimit;	/* Maximum number of chars to print.  */
  int bytes_read;
  gdb::unique_xmalloc_ptr<gdb_byte> buffer;	/* Dynamically growable fetch buffer.  */
  struct gdbarch *gdbarch = elttype->arch ();
  enum bfd_endian byte_order = type_byte_order (elttype);
  int width = elttype->length ();

  /* First we need to figure out the limit on the number of characters we are
     going to attempt to fetch and print.  This is actually pretty simple.
     If LEN >= zero, then the limit is the minimum of LEN and print_max_chars.
     If LEN is -1, then the limit is print_max_chars.  This is true regardless
     of whether print_max_chars is zero, UINT_MAX (unlimited), or something in
     between, because finding the null byte (or available memory) is what
     actually limits the fetch.  */

  unsigned int print_max_chars = get_print_max_chars (options);
  fetchlimit = (len == -1
		? print_max_chars
		: std::min ((unsigned) len, print_max_chars));

  err = target_read_string (addr, len, width, fetchlimit,
			    &buffer, &bytes_read);

  addr += bytes_read;

  /* We now have either successfully filled the buffer to fetchlimit,
     or terminated early due to an error or finding a null char when
     LEN is -1.  */

  /* Determine found_nul by looking at the last character read.  */
  found_nul = 0;
  if (bytes_read >= width)
    found_nul = extract_unsigned_integer (buffer.get () + bytes_read - width,
					  width, byte_order) == 0;
  if (len == -1 && !found_nul)
    {
      gdb_byte *peekbuf;

      /* We didn't find a NUL terminator we were looking for.  Attempt
	 to peek at the next character.  If not successful, or it is not
	 a null byte, then force ellipsis to be printed.  */

      peekbuf = (gdb_byte *) alloca (width);

      if (target_read_memory (addr, peekbuf, width) == 0
	  && extract_unsigned_integer (peekbuf, width, byte_order) != 0)
	force_ellipsis = 1;
    }
  else if ((len >= 0 && err != 0) || (len > bytes_read / width))
    {
      /* Getting an error when we have a requested length, or fetching less
	 than the number of characters actually requested, always make us
	 print ellipsis.  */
      force_ellipsis = 1;
    }

  /* If we get an error before fetching anything, don't print a string.
     But if we fetch something and then get an error, print the string
     and then the error message.  */
  if (err == 0 || bytes_read > 0)
    current_language->printstr (stream, elttype, buffer.get (),
				bytes_read / width,
				encoding, force_ellipsis, options);

  if (err != 0)
    {
      std::string str = memory_error_message (TARGET_XFER_E_IO, gdbarch, addr);

      gdb_printf (stream, _("<error: %ps>"),
		  styled_string (metadata_style.style (),
				 str.c_str ()));
    }

  return (bytes_read / width);
}

/* Handle 'show print max-depth'.  */

static void
show_print_max_depth (struct ui_file *file, int from_tty,
		      struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Maximum print depth is %s.\n"), value);
}


/* The 'set input-radix' command writes to this auxiliary variable.
   If the requested radix is valid, INPUT_RADIX is updated; otherwise,
   it is left unchanged.  */

static unsigned input_radix_1 = 10;

/* Validate an input or output radix setting, and make sure the user
   knows what they really did here.  Radix setting is confusing, e.g.
   setting the input radix to "10" never changes it!  */

static void
set_input_radix (const char *args, int from_tty, struct cmd_list_element *c)
{
  set_input_radix_1 (from_tty, input_radix_1);
}

static void
set_input_radix_1 (int from_tty, unsigned radix)
{
  /* We don't currently disallow any input radix except 0 or 1, which don't
     make any mathematical sense.  In theory, we can deal with any input
     radix greater than 1, even if we don't have unique digits for every
     value from 0 to radix-1, but in practice we lose on large radix values.
     We should either fix the lossage or restrict the radix range more.
     (FIXME).  */

  if (radix < 2)
    {
      input_radix_1 = input_radix;
      error (_("Nonsense input radix ``decimal %u''; input radix unchanged."),
	     radix);
    }
  input_radix_1 = input_radix = radix;
  if (from_tty)
    {
      gdb_printf (_("Input radix now set to "
		    "decimal %u, hex %x, octal %o.\n"),
		  radix, radix, radix);
    }
}

/* The 'set output-radix' command writes to this auxiliary variable.
   If the requested radix is valid, OUTPUT_RADIX is updated,
   otherwise, it is left unchanged.  */

static unsigned output_radix_1 = 10;

static void
set_output_radix (const char *args, int from_tty, struct cmd_list_element *c)
{
  set_output_radix_1 (from_tty, output_radix_1);
}

static void
set_output_radix_1 (int from_tty, unsigned radix)
{
  /* Validate the radix and disallow ones that we aren't prepared to
     handle correctly, leaving the radix unchanged.  */
  switch (radix)
    {
    case 16:
      user_print_options.output_format = 'x';	/* hex */
      break;
    case 10:
      user_print_options.output_format = 0;	/* decimal */
      break;
    case 8:
      user_print_options.output_format = 'o';	/* octal */
      break;
    default:
      output_radix_1 = output_radix;
      error (_("Unsupported output radix ``decimal %u''; "
	       "output radix unchanged."),
	     radix);
    }
  output_radix_1 = output_radix = radix;
  if (from_tty)
    {
      gdb_printf (_("Output radix now set to "
		    "decimal %u, hex %x, octal %o.\n"),
		  radix, radix, radix);
    }
}

/* Set both the input and output radix at once.  Try to set the output radix
   first, since it has the most restrictive range.  An radix that is valid as
   an output radix is also valid as an input radix.

   It may be useful to have an unusual input radix.  If the user wishes to
   set an input radix that is not valid as an output radix, he needs to use
   the 'set input-radix' command.  */

static void
set_radix (const char *arg, int from_tty)
{
  unsigned radix;

  radix = (arg == NULL) ? 10 : parse_and_eval_long (arg);
  set_output_radix_1 (0, radix);
  set_input_radix_1 (0, radix);
  if (from_tty)
    {
      gdb_printf (_("Input and output radices now set to "
		    "decimal %u, hex %x, octal %o.\n"),
		  radix, radix, radix);
    }
}

/* Show both the input and output radices.  */

static void
show_radix (const char *arg, int from_tty)
{
  if (from_tty)
    {
      if (input_radix == output_radix)
	{
	  gdb_printf (_("Input and output radices set to "
			"decimal %u, hex %x, octal %o.\n"),
		      input_radix, input_radix, input_radix);
	}
      else
	{
	  gdb_printf (_("Input radix set to decimal "
			"%u, hex %x, octal %o.\n"),
		      input_radix, input_radix, input_radix);
	  gdb_printf (_("Output radix set to decimal "
			"%u, hex %x, octal %o.\n"),
		      output_radix, output_radix, output_radix);
	}
    }
}


/* Controls printing of vtbl's.  */
static void
show_vtblprint (struct ui_file *file, int from_tty,
		struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("\
Printing of C++ virtual function tables is %s.\n"),
	      value);
}

/* Controls looking up an object's derived type using what we find in
   its vtables.  */
static void
show_objectprint (struct ui_file *file, int from_tty,
		  struct cmd_list_element *c,
		  const char *value)
{
  gdb_printf (file, _("\
Printing of object's derived type based on vtable info is %s.\n"),
	      value);
}

static void
show_static_field_print (struct ui_file *file, int from_tty,
			 struct cmd_list_element *c,
			 const char *value)
{
  gdb_printf (file,
	      _("Printing of C++ static members is %s.\n"),
	      value);
}



/* A couple typedefs to make writing the options a bit more
   convenient.  */
using boolean_option_def
  = gdb::option::boolean_option_def<value_print_options>;
using uinteger_option_def
  = gdb::option::uinteger_option_def<value_print_options>;
using pinteger_option_def
  = gdb::option::pinteger_option_def<value_print_options>;

/* Extra literals supported with the `set print characters' and
   `print -characters' commands.  */
static const literal_def print_characters_literals[] =
  {
    { "elements", PRINT_MAX_CHARS_ELEMENTS },
    { "unlimited", PRINT_MAX_CHARS_UNLIMITED, 0 },
    { nullptr }
  };

/* Definitions of options for the "print" and "compile print"
   commands.  */
static const gdb::option::option_def value_print_option_defs[] = {

  boolean_option_def {
    "address",
    [] (value_print_options *opt) { return &opt->addressprint; },
    show_addressprint, /* show_cmd_cb */
    N_("Set printing of addresses."),
    N_("Show printing of addresses."),
    NULL, /* help_doc */
  },

  boolean_option_def {
    "array",
    [] (value_print_options *opt) { return &opt->prettyformat_arrays; },
    show_prettyformat_arrays, /* show_cmd_cb */
    N_("Set pretty formatting of arrays."),
    N_("Show pretty formatting of arrays."),
    NULL, /* help_doc */
  },

  boolean_option_def {
    "array-indexes",
    [] (value_print_options *opt) { return &opt->print_array_indexes; },
    show_print_array_indexes, /* show_cmd_cb */
    N_("Set printing of array indexes."),
    N_("Show printing of array indexes."),
    NULL, /* help_doc */
  },

  boolean_option_def {
    "nibbles",
    [] (value_print_options *opt) { return &opt->nibblesprint; },
    show_nibbles, /* show_cmd_cb */
    N_("Set whether to print binary values in groups of four bits."),
    N_("Show whether to print binary values in groups of four bits."),
    NULL, /* help_doc */
  },

  uinteger_option_def {
    "characters",
    [] (value_print_options *opt) { return &opt->print_max_chars; },
    print_characters_literals,
    show_print_max_chars, /* show_cmd_cb */
    N_("Set limit on string chars to print."),
    N_("Show limit on string chars to print."),
    N_("\"elements\" causes the array element limit to be used.\n"
       "\"unlimited\" causes there to be no limit."),
  },

  uinteger_option_def {
    "elements",
    [] (value_print_options *opt) { return &opt->print_max; },
    uinteger_unlimited_literals,
    show_print_max, /* show_cmd_cb */
    N_("Set limit on array elements to print."),
    N_("Show limit on array elements to print."),
    N_("\"unlimited\" causes there to be no limit.\n"
       "This setting also applies to string chars when \"print characters\"\n"
       "is set to \"elements\"."),
  },

  pinteger_option_def {
    "max-depth",
    [] (value_print_options *opt) { return &opt->max_depth; },
    pinteger_unlimited_literals,
    show_print_max_depth, /* show_cmd_cb */
    N_("Set maximum print depth for nested structures, unions and arrays."),
    N_("Show maximum print depth for nested structures, unions, and arrays."),
    N_("When structures, unions, or arrays are nested beyond this depth then they\n\
will be replaced with either '{...}' or '(...)' depending on the language.\n\
Use \"unlimited\" to print the complete structure.")
  },

  boolean_option_def {
    "memory-tag-violations",
    [] (value_print_options *opt) { return &opt->memory_tag_violations; },
    show_memory_tag_violations, /* show_cmd_cb */
    N_("Set printing of memory tag violations for pointers."),
    N_("Show printing of memory tag violations for pointers."),
    N_("Issue a warning when the printed value is a pointer\n\
whose logical tag doesn't match the allocation tag of the memory\n\
location it points to."),
  },

  boolean_option_def {
    "null-stop",
    [] (value_print_options *opt) { return &opt->stop_print_at_null; },
    show_stop_print_at_null, /* show_cmd_cb */
    N_("Set printing of char arrays to stop at first null char."),
    N_("Show printing of char arrays to stop at first null char."),
    NULL, /* help_doc */
  },

  boolean_option_def {
    "object",
    [] (value_print_options *opt) { return &opt->objectprint; },
    show_objectprint, /* show_cmd_cb */
    _("Set printing of C++ virtual function tables."),
    _("Show printing of C++ virtual function tables."),
    NULL, /* help_doc */
  },

  boolean_option_def {
    "pretty",
    [] (value_print_options *opt) { return &opt->prettyformat_structs; },
    show_prettyformat_structs, /* show_cmd_cb */
    N_("Set pretty formatting of structures."),
    N_("Show pretty formatting of structures."),
    NULL, /* help_doc */
  },

  boolean_option_def {
    "raw-values",
    [] (value_print_options *opt) { return &opt->raw; },
    NULL, /* show_cmd_cb */
    N_("Set whether to print values in raw form."),
    N_("Show whether to print values in raw form."),
    N_("If set, values are printed in raw form, bypassing any\n\
pretty-printers for that value.")
  },

  uinteger_option_def {
    "repeats",
    [] (value_print_options *opt) { return &opt->repeat_count_threshold; },
    uinteger_unlimited_literals,
    show_repeat_count_threshold, /* show_cmd_cb */
    N_("Set threshold for repeated print elements."),
    N_("Show threshold for repeated print elements."),
    N_("\"unlimited\" causes all elements to be individually printed."),
  },

  boolean_option_def {
    "static-members",
    [] (value_print_options *opt) { return &opt->static_field_print; },
    show_static_field_print, /* show_cmd_cb */
    N_("Set printing of C++ static members."),
    N_("Show printing of C++ static members."),
    NULL, /* help_doc */
  },

  boolean_option_def {
    "symbol",
    [] (value_print_options *opt) { return &opt->symbol_print; },
    show_symbol_print, /* show_cmd_cb */
    N_("Set printing of symbol names when printing pointers."),
    N_("Show printing of symbol names when printing pointers."),
    NULL, /* help_doc */
  },

  boolean_option_def {
    "union",
    [] (value_print_options *opt) { return &opt->unionprint; },
    show_unionprint, /* show_cmd_cb */
    N_("Set printing of unions interior to structures."),
    N_("Show printing of unions interior to structures."),
    NULL, /* help_doc */
  },

  boolean_option_def {
    "vtbl",
    [] (value_print_options *opt) { return &opt->vtblprint; },
    show_vtblprint, /* show_cmd_cb */
    N_("Set printing of C++ virtual function tables."),
    N_("Show printing of C++ virtual function tables."),
    NULL, /* help_doc */
  },
};

/* See valprint.h.  */

gdb::option::option_def_group
make_value_print_options_def_group (value_print_options *opts)
{
  return {{value_print_option_defs}, opts};
}

#if GDB_SELF_TEST

/* Test printing of TYPE_CODE_FLAGS values.  */

static void
test_print_flags (gdbarch *arch)
{
  type *flags_type = arch_flags_type (arch, "test_type", 32);
  type *field_type = builtin_type (arch)->builtin_uint32;

  /* Value:  1010 1010
     Fields: CCCB BAAA */
  append_flags_type_field (flags_type, 0, 3, field_type, "A");
  append_flags_type_field (flags_type, 3, 2, field_type, "B");
  append_flags_type_field (flags_type, 5, 3, field_type, "C");

  value *val = value::allocate (flags_type);
  gdb_byte *contents = val->contents_writeable ().data ();
  store_unsigned_integer (contents, 4, gdbarch_byte_order (arch), 0xaa);

  string_file out;
  val_print_type_code_flags (flags_type, val, 0, &out);
  SELF_CHECK (out.string () == "[ A=2 B=1 C=5 ]");
}

#endif

void _initialize_valprint ();
void
_initialize_valprint ()
{
#if GDB_SELF_TEST
  selftests::register_test_foreach_arch ("print-flags", test_print_flags);
#endif

  set_show_commands setshow_print_cmds
    = add_setshow_prefix_cmd ("print", no_class,
			      _("Generic command for setting how things print."),
			      _("Generic command for showing print settings."),
			      &setprintlist, &showprintlist,
			      &setlist, &showlist);
  add_alias_cmd ("p", setshow_print_cmds.set, no_class, 1, &setlist);
  /* Prefer set print to set prompt.  */
  add_alias_cmd ("pr", setshow_print_cmds.set, no_class, 1, &setlist);
  add_alias_cmd ("p", setshow_print_cmds.show, no_class, 1, &showlist);
  add_alias_cmd ("pr", setshow_print_cmds.show, no_class, 1, &showlist);

  set_show_commands setshow_print_raw_cmds
    = add_setshow_prefix_cmd
	("raw", no_class,
	 _("Generic command for setting what things to print in \"raw\" mode."),
	 _("Generic command for showing \"print raw\" settings."),
	 &setprintrawlist, &showprintrawlist, &setprintlist, &showprintlist);
  deprecate_cmd (setshow_print_raw_cmds.set, nullptr);
  deprecate_cmd (setshow_print_raw_cmds.show, nullptr);

  gdb::option::add_setshow_cmds_for_options
    (class_support, &user_print_options, value_print_option_defs,
     &setprintlist, &showprintlist);

  add_setshow_zuinteger_cmd ("input-radix", class_support, &input_radix_1,
			     _("\
Set default input radix for entering numbers."), _("\
Show default input radix for entering numbers."), NULL,
			     set_input_radix,
			     show_input_radix,
			     &setlist, &showlist);

  add_setshow_zuinteger_cmd ("output-radix", class_support, &output_radix_1,
			     _("\
Set default output radix for printing of values."), _("\
Show default output radix for printing of values."), NULL,
			     set_output_radix,
			     show_output_radix,
			     &setlist, &showlist);

  /* The "set radix" and "show radix" commands are special in that
     they are like normal set and show commands but allow two normally
     independent variables to be either set or shown with a single
     command.  So the usual deprecated_add_set_cmd() and [deleted]
     add_show_from_set() commands aren't really appropriate.  */
  /* FIXME: i18n: With the new add_setshow_integer command, that is no
     longer true - show can display anything.  */
  add_cmd ("radix", class_support, set_radix, _("\
Set default input and output number radices.\n\
Use 'set input-radix' or 'set output-radix' to independently set each.\n\
Without an argument, sets both radices back to the default value of 10."),
	   &setlist);
  add_cmd ("radix", class_support, show_radix, _("\
Show the default input and output number radices.\n\
Use 'show input-radix' or 'show output-radix' to independently show each."),
	   &showlist);
}
