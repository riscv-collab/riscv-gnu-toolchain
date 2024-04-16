/* Support for printing Ada values for GDB, the GNU debugger.

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
#include <ctype.h>
#include "gdbtypes.h"
#include "expression.h"
#include "value.h"
#include "valprint.h"
#include "language.h"
#include "annotate.h"
#include "ada-lang.h"
#include "target-float.h"
#include "cli/cli-style.h"
#include "gdbarch.h"

static int print_field_values (struct value *, struct value *,
			       struct ui_file *, int,
			       const struct value_print_options *,
			       int, const struct language_defn *);



/* Assuming TYPE is a simple array type, prints its lower bound on STREAM,
   if non-standard (i.e., other than 1 for numbers, other than lower bound
   of index type for enumerated type).  Returns 1 if something printed,
   otherwise 0.  */

static int
print_optional_low_bound (struct ui_file *stream, struct type *type,
			  const struct value_print_options *options)
{
  struct type *index_type;
  LONGEST low_bound;
  LONGEST high_bound;

  if (options->print_array_indexes)
    return 0;

  if (!get_array_bounds (type, &low_bound, &high_bound))
    return 0;

  /* If this is an empty array, then don't print the lower bound.
     That would be confusing, because we would print the lower bound,
     followed by... nothing!  */
  if (low_bound > high_bound)
    return 0;

  index_type = type->index_type ();

  while (index_type->code () == TYPE_CODE_RANGE)
    {
      /* We need to know what the base type is, in order to do the
	 appropriate check below.  Otherwise, if this is a subrange
	 of an enumerated type, where the underlying value of the
	 first element is typically 0, we might test the low bound
	 against the wrong value.  */
      index_type = index_type->target_type ();
    }

  /* Don't print the lower bound if it's the default one.  */
  switch (index_type->code ())
    {
    case TYPE_CODE_BOOL:
    case TYPE_CODE_CHAR:
      if (low_bound == 0)
	return 0;
      break;
    case TYPE_CODE_ENUM:
      if (low_bound == 0)
	return 0;
      low_bound = index_type->field (low_bound).loc_enumval ();
      break;
    case TYPE_CODE_UNDEF:
      index_type = NULL;
      [[fallthrough]];
    default:
      if (low_bound == 1)
	return 0;
      break;
    }

  ada_print_scalar (index_type, low_bound, stream);
  gdb_printf (stream, " => ");
  return 1;
}

/*  Version of val_print_array_elements for GNAT-style packed arrays.
    Prints elements of packed array of type TYPE from VALADDR on
    STREAM.  Formats according to OPTIONS and separates with commas.
    RECURSE is the recursion (nesting) level.  TYPE must have been
    decoded (as by ada_coerce_to_simple_array).  */

static void
val_print_packed_array_elements (struct type *type, const gdb_byte *valaddr,
				 int offset, struct ui_file *stream,
				 int recurse,
				 const struct value_print_options *options)
{
  unsigned int i;
  unsigned int things_printed = 0;
  unsigned len;
  struct type *elttype, *index_type;
  unsigned long bitsize = type->field (0).bitsize ();
  LONGEST low = 0;

  scoped_value_mark mark;

  elttype = type->target_type ();
  index_type = type->index_type ();

  {
    LONGEST high;

    if (!get_discrete_bounds (index_type, &low, &high))
      len = 1;
    else if (low > high)
      {
	/* The array length should normally be HIGH_POS - LOW_POS + 1.
	   But in Ada we allow LOW_POS to be greater than HIGH_POS for
	   empty arrays.  In that situation, the array length is just zero,
	   not negative!  */
	len = 0;
      }
    else
      len = high - low + 1;
  }

  if (index_type->code () == TYPE_CODE_RANGE)
    index_type = index_type->target_type ();

  i = 0;
  annotate_array_section_begin (i, elttype);

  while (i < len && things_printed < options->print_max)
    {
      /* Both this outer loop and the inner loop that checks for
	 duplicates may allocate many values.  To avoid using too much
	 memory, both spots release values as they work.  */
      scoped_value_mark outer_free_values;

      struct value *v0, *v1;
      int i0;

      if (i != 0)
	{
	  if (options->prettyformat_arrays)
	    {
	      gdb_printf (stream, ",\n");
	      print_spaces (2 + 2 * recurse, stream);
	    }
	  else
	    {
	      gdb_printf (stream, ", ");
	    }
	}
      else if (options->prettyformat_arrays)
	{
	  gdb_printf (stream, "\n");
	  print_spaces (2 + 2 * recurse, stream);
	}
      stream->wrap_here (2 + 2 * recurse);
      maybe_print_array_index (index_type, i + low, stream, options);

      i0 = i;
      v0 = ada_value_primitive_packed_val (NULL, valaddr + offset,
					   (i0 * bitsize) / HOST_CHAR_BIT,
					   (i0 * bitsize) % HOST_CHAR_BIT,
					   bitsize, elttype);
      while (1)
	{
	  /* Make sure to free any values in the inner loop.  */
	  scoped_value_mark free_values;

	  i += 1;
	  if (i >= len)
	    break;
	  v1 = ada_value_primitive_packed_val (NULL, valaddr + offset,
					       (i * bitsize) / HOST_CHAR_BIT,
					       (i * bitsize) % HOST_CHAR_BIT,
					       bitsize, elttype);
	  if (check_typedef (v0->type ())->length ()
	      != check_typedef (v1->type ())->length ())
	    break;
	  if (!v0->contents_eq (v0->embedded_offset (),
				v1, v1->embedded_offset (),
				check_typedef (v0->type ())->length ()))
	    break;
	}

      if (i - i0 > options->repeat_count_threshold)
	{
	  struct value_print_options opts = *options;

	  opts.deref_ref = false;
	  common_val_print (v0, stream, recurse + 1, &opts, current_language);
	  annotate_elt_rep (i - i0);
	  gdb_printf (stream, _(" %p[<repeats %u times>%p]"),
		      metadata_style.style ().ptr (), i - i0, nullptr);
	  annotate_elt_rep_end ();

	}
      else
	{
	  int j;
	  struct value_print_options opts = *options;

	  opts.deref_ref = false;
	  for (j = i0; j < i; j += 1)
	    {
	      if (j > i0)
		{
		  if (options->prettyformat_arrays)
		    {
		      gdb_printf (stream, ",\n");
		      print_spaces (2 + 2 * recurse, stream);
		    }
		  else
		    {
		      gdb_printf (stream, ", ");
		    }
		  stream->wrap_here (2 + 2 * recurse);
		  maybe_print_array_index (index_type, j + low,
					   stream, options);
		}
	      common_val_print (v0, stream, recurse + 1, &opts,
				current_language);
	      annotate_elt ();
	    }
	}
      things_printed += i - i0;
    }
  annotate_array_section_end ();
  if (i < len)
    {
      gdb_printf (stream, "...");
    }
}

/* Print the character C on STREAM as part of the contents of a literal
   string whose delimiter is QUOTER.  TYPE_LEN is the length in bytes
   of the character.  */

void
ada_emit_char (int c, struct type *type, struct ui_file *stream,
	       int quoter, int type_len)
{
  /* If this character fits in the normal ASCII range, and is
     a printable character, then print the character as if it was
     an ASCII character, even if this is a wide character.
     The UCHAR_MAX check is necessary because the isascii function
     requires that its argument have a value of an unsigned char,
     or EOF (EOF is obviously not printable).  */
  if (c <= UCHAR_MAX && isascii (c) && isprint (c))
    {
      if (c == quoter && c == '"')
	gdb_printf (stream, "\"\"");
      else
	gdb_printf (stream, "%c", c);
    }
  else
    {
      /* Follow GNAT's lead here and only use 6 digits for
	 wide_wide_character.  */
      gdb_printf (stream, "[\"%0*x\"]", std::min (6, type_len * 2), c);
    }
}

/* Character #I of STRING, given that TYPE_LEN is the size in bytes
   of a character.  */

static int
char_at (const gdb_byte *string, int i, int type_len,
	 enum bfd_endian byte_order)
{
  if (type_len == 1)
    return string[i];
  else
    return (int) extract_unsigned_integer (string + type_len * i,
					   type_len, byte_order);
}

/* Print a floating-point value of type TYPE, pointed to in GDB by
   VALADDR, on STREAM.  Use Ada formatting conventions: there must be
   a decimal point, and at least one digit before and after the
   point.  We use the GNAT format for NaNs and infinities.  */

static void
ada_print_floating (const gdb_byte *valaddr, struct type *type,
		    struct ui_file *stream)
{
  string_file tmp_stream;

  print_floating (valaddr, type, &tmp_stream);

  std::string s = tmp_stream.release ();
  size_t skip_count = 0;

  /* Don't try to modify a result representing an error.  */
  if (s[0] == '<')
    {
      gdb_puts (s.c_str (), stream);
      return;
    }

  /* Modify for Ada rules.  */

  size_t pos = s.find ("inf");
  if (pos == std::string::npos)
    pos = s.find ("Inf");
  if (pos == std::string::npos)
    pos = s.find ("INF");
  if (pos != std::string::npos)
    s.replace (pos, 3, "Inf");

  if (pos == std::string::npos)
    {
      pos = s.find ("nan");
      if (pos == std::string::npos)
	pos = s.find ("NaN");
      if (pos == std::string::npos)
	pos = s.find ("Nan");
      if (pos != std::string::npos)
	{
	  s[pos] = s[pos + 2] = 'N';
	  if (s[0] == '-')
	    skip_count = 1;
	}
    }

  if (pos == std::string::npos
      && s.find ('.') == std::string::npos)
    {
      pos = s.find ('e');
      if (pos == std::string::npos)
	gdb_printf (stream, "%s.0", s.c_str ());
      else
	gdb_printf (stream, "%.*s.0%s", (int) pos, s.c_str (), &s[pos]);
    }
  else
    gdb_printf (stream, "%s", &s[skip_count]);
}

void
ada_printchar (int c, struct type *type, struct ui_file *stream)
{
  gdb_puts ("'", stream);
  ada_emit_char (c, type, stream, '\'', type->length ());
  gdb_puts ("'", stream);
}

/* [From print_type_scalar in typeprint.c].   Print VAL on STREAM in a
   form appropriate for TYPE, if non-NULL.  If TYPE is NULL, print VAL
   like a default signed integer.  */

void
ada_print_scalar (struct type *type, LONGEST val, struct ui_file *stream)
{
  if (!type)
    {
      print_longest (stream, 'd', 0, val);
      return;
    }

  type = ada_check_typedef (type);

  switch (type->code ())
    {

    case TYPE_CODE_ENUM:
      {
	std::optional<LONGEST> posn = discrete_position (type, val);
	if (posn.has_value ())
	  fputs_styled (ada_enum_name (type->field (*posn).name ()),
			variable_name_style.style (), stream);
	else
	  print_longest (stream, 'd', 0, val);
      }
      break;

    case TYPE_CODE_INT:
      print_longest (stream, type->is_unsigned () ? 'u' : 'd', 0, val);
      break;

    case TYPE_CODE_CHAR:
      current_language->printchar (val, type, stream);
      break;

    case TYPE_CODE_BOOL:
      gdb_printf (stream, val ? "true" : "false");
      break;

    case TYPE_CODE_RANGE:
      ada_print_scalar (type->target_type (), val, stream);
      return;

    case TYPE_CODE_UNDEF:
    case TYPE_CODE_PTR:
    case TYPE_CODE_ARRAY:
    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
    case TYPE_CODE_FUNC:
    case TYPE_CODE_FLT:
    case TYPE_CODE_VOID:
    case TYPE_CODE_SET:
    case TYPE_CODE_STRING:
    case TYPE_CODE_ERROR:
    case TYPE_CODE_MEMBERPTR:
    case TYPE_CODE_METHODPTR:
    case TYPE_CODE_METHOD:
    case TYPE_CODE_REF:
      warning (_("internal error: unhandled type in ada_print_scalar"));
      break;

    default:
      error (_("Invalid type code in symbol table."));
    }
}

/* Print the character string STRING, printing at most LENGTH characters.
   Printing stops early if the number hits print_max; repeat counts
   are printed as appropriate.  Print ellipses at the end if we
   had to stop before printing LENGTH characters, or if FORCE_ELLIPSES.
   TYPE_LEN is the length (1 or 2) of the character type.  */

static void
printstr (struct ui_file *stream, struct type *elttype, const gdb_byte *string,
	  unsigned int length, int force_ellipses, int type_len,
	  const struct value_print_options *options)
{
  enum bfd_endian byte_order = type_byte_order (elttype);
  unsigned int i;
  unsigned int things_printed = 0;
  int in_quotes = 0;
  int need_comma = 0;

  if (length == 0)
    {
      gdb_puts ("\"\"", stream);
      return;
    }

  unsigned int print_max_chars = get_print_max_chars (options);
  for (i = 0; i < length && things_printed < print_max_chars; i += 1)
    {
      /* Position of the character we are examining
	 to see whether it is repeated.  */
      unsigned int rep1;
      /* Number of repetitions we have detected so far.  */
      unsigned int reps;

      QUIT;

      if (need_comma)
	{
	  gdb_puts (", ", stream);
	  need_comma = 0;
	}

      rep1 = i + 1;
      reps = 1;
      while (rep1 < length
	     && char_at (string, rep1, type_len, byte_order)
		== char_at (string, i, type_len, byte_order))
	{
	  rep1 += 1;
	  reps += 1;
	}

      if (reps > options->repeat_count_threshold)
	{
	  if (in_quotes)
	    {
	      gdb_puts ("\", ", stream);
	      in_quotes = 0;
	    }
	  gdb_puts ("'", stream);
	  ada_emit_char (char_at (string, i, type_len, byte_order),
			 elttype, stream, '\'', type_len);
	  gdb_puts ("'", stream);
	  gdb_printf (stream, _(" %p[<repeats %u times>%p]"),
		      metadata_style.style ().ptr (), reps, nullptr);
	  i = rep1 - 1;
	  things_printed += options->repeat_count_threshold;
	  need_comma = 1;
	}
      else
	{
	  if (!in_quotes)
	    {
	      gdb_puts ("\"", stream);
	      in_quotes = 1;
	    }
	  ada_emit_char (char_at (string, i, type_len, byte_order),
			 elttype, stream, '"', type_len);
	  things_printed += 1;
	}
    }

  /* Terminate the quotes if necessary.  */
  if (in_quotes)
    gdb_puts ("\"", stream);

  if (force_ellipses || i < length)
    gdb_puts ("...", stream);
}

void
ada_printstr (struct ui_file *stream, struct type *type,
	      const gdb_byte *string, unsigned int length,
	      const char *encoding, int force_ellipses,
	      const struct value_print_options *options)
{
  printstr (stream, type, string, length, force_ellipses, type->length (),
	    options);
}

static int
print_variant_part (struct value *value, int field_num,
		    struct value *outer_value,
		    struct ui_file *stream, int recurse,
		    const struct value_print_options *options,
		    int comma_needed,
		    const struct language_defn *language)
{
  struct type *type = value->type ();
  struct type *var_type = type->field (field_num).type ();
  int which = ada_which_variant_applies (var_type, outer_value);

  if (which < 0)
    return 0;

  struct value *variant_field = value_field (value, field_num);
  struct value *active_component = value_field (variant_field, which);
  return print_field_values (active_component, outer_value, stream, recurse,
			     options, comma_needed, language);
}

/* Print out fields of VALUE.

   STREAM, RECURSE, and OPTIONS have the same meanings as in
   ada_print_value and ada_value_print.

   OUTER_VALUE gives the enclosing record (used to get discriminant
   values when printing variant parts).

   COMMA_NEEDED is 1 if fields have been printed at the current recursion
   level, so that a comma is needed before any field printed by this
   call.

   Returns 1 if COMMA_NEEDED or any fields were printed.  */

static int
print_field_values (struct value *value, struct value *outer_value,
		    struct ui_file *stream, int recurse,
		    const struct value_print_options *options,
		    int comma_needed,
		    const struct language_defn *language)
{
  int i, len;

  struct type *type = value->type ();
  len = type->num_fields ();

  for (i = 0; i < len; i += 1)
    {
      if (ada_is_ignored_field (type, i))
	continue;

      if (ada_is_wrapper_field (type, i))
	{
	  struct value *field_val = ada_value_primitive_field (value, 0,
							       i, type);
	  comma_needed =
	    print_field_values (field_val, field_val,
				stream, recurse, options,
				comma_needed, language);
	  continue;
	}
      else if (ada_is_variant_part (type, i))
	{
	  comma_needed =
	    print_variant_part (value, i, outer_value, stream, recurse,
				options, comma_needed, language);
	  continue;
	}

      if (comma_needed)
	gdb_printf (stream, ", ");
      comma_needed = 1;

      if (options->prettyformat)
	{
	  gdb_printf (stream, "\n");
	  print_spaces (2 + 2 * recurse, stream);
	}
      else
	{
	  stream->wrap_here (2 + 2 * recurse);
	}

      annotate_field_begin (type->field (i).type ());
      gdb_printf (stream, "%.*s",
		  ada_name_prefix_len (type->field (i).name ()),
		  type->field (i).name ());
      annotate_field_name_end ();
      gdb_puts (" => ", stream);
      annotate_field_value ();

      if (type->field (i).is_packed ())
	{
	  /* Bitfields require special handling, especially due to byte
	     order problems.  */
	  if (type->field (i).is_ignored ())
	    {
	      fputs_styled (_("<optimized out or zero length>"),
			    metadata_style.style (), stream);
	    }
	  else
	    {
	      struct value *v;
	      int bit_pos = type->field (i).loc_bitpos ();
	      int bit_size = type->field (i).bitsize ();
	      struct value_print_options opts;

	      v = ada_value_primitive_packed_val
		    (value, nullptr,
		     bit_pos / HOST_CHAR_BIT,
		     bit_pos % HOST_CHAR_BIT,
		     bit_size, type->field (i).type ());
	      opts = *options;
	      opts.deref_ref = false;
	      common_val_print (v, stream, recurse + 1, &opts, language);
	    }
	}
      else
	{
	  struct value_print_options opts = *options;

	  opts.deref_ref = false;

	  struct value *v = value_field (value, i);
	  common_val_print (v, stream, recurse + 1, &opts, language);
	}
      annotate_field_end ();
    }

  return comma_needed;
}

/* Implement Ada val_print'ing for the case where TYPE is
   a TYPE_CODE_ARRAY of characters.  */

static void
ada_val_print_string (struct type *type, const gdb_byte *valaddr,
		      int offset_aligned,
		      struct ui_file *stream, int recurse,
		      const struct value_print_options *options)
{
  enum bfd_endian byte_order = type_byte_order (type);
  struct type *elttype = type->target_type ();
  unsigned int eltlen;
  unsigned int len;

  /* We know that ELTTYPE cannot possibly be null, because we assume
     that we're called only when TYPE is a string-like type.
     Similarly, the size of ELTTYPE should also be non-null, since
     it's a character-like type.  */
  gdb_assert (elttype != NULL);
  gdb_assert (elttype->length () != 0);

  eltlen = elttype->length ();
  len = type->length () / eltlen;

  /* If requested, look for the first null char and only print
     elements up to it.  */
  if (options->stop_print_at_null)
    {
      unsigned int print_max_chars = get_print_max_chars (options);
      int temp_len;

      /* Look for a NULL char.  */
      for (temp_len = 0;
	   (temp_len < len
	    && temp_len < print_max_chars
	    && char_at (valaddr + offset_aligned,
			temp_len, eltlen, byte_order) != 0);
	   temp_len += 1);
      len = temp_len;
    }

  printstr (stream, elttype, valaddr + offset_aligned, len, 0,
	    eltlen, options);
}

/* Implement Ada value_print'ing for the case where TYPE is a
   TYPE_CODE_PTR.  */

static void
ada_value_print_ptr (struct value *val,
		     struct ui_file *stream, int recurse,
		     const struct value_print_options *options)
{
  if (!options->format
      && val->type ()->target_type ()->code () == TYPE_CODE_INT
      && val->type ()->target_type ()->length () == 0)
    {
      gdb_puts ("null", stream);
      return;
    }

  common_val_print (val, stream, recurse, options, language_def (language_c));

  struct type *type = ada_check_typedef (val->type ());
  if (ada_is_tag_type (type))
    {
      gdb::unique_xmalloc_ptr<char> name = ada_tag_name (val);

      if (name != NULL)
	gdb_printf (stream, " (%s)", name.get ());
    }
}

/* Implement Ada val_print'ing for the case where TYPE is
   a TYPE_CODE_INT or TYPE_CODE_RANGE.  */

static void
ada_value_print_num (struct value *val, struct ui_file *stream, int recurse,
		     const struct value_print_options *options)
{
  struct type *type = ada_check_typedef (val->type ());
  const gdb_byte *valaddr = val->contents_for_printing ().data ();

  if (type->code () == TYPE_CODE_RANGE
      && (type->target_type ()->code () == TYPE_CODE_ENUM
	  || type->target_type ()->code () == TYPE_CODE_BOOL
	  || type->target_type ()->code () == TYPE_CODE_CHAR))
    {
      /* For enum-valued ranges, we want to recurse, because we'll end
	 up printing the constant's name rather than its numeric
	 value.  Character and fixed-point types are also printed
	 differently, so recurse for those as well.  */
      struct type *target_type = type->target_type ();
      val = value_cast (target_type, val);
      common_val_print (val, stream, recurse + 1, options,
			language_def (language_ada));
      return;
    }
  else
    {
      int format = (options->format ? options->format
		    : options->output_format);

      if (format)
	{
	  struct value_print_options opts = *options;

	  opts.format = format;
	  value_print_scalar_formatted (val, &opts, 0, stream);
	}
      else if (ada_is_system_address_type (type))
	{
	  /* FIXME: We want to print System.Address variables using
	     the same format as for any access type.  But for some
	     reason GNAT encodes the System.Address type as an int,
	     so we have to work-around this deficiency by handling
	     System.Address values as a special case.  */

	  struct gdbarch *gdbarch = type->arch ();
	  struct type *ptr_type = builtin_type (gdbarch)->builtin_data_ptr;
	  CORE_ADDR addr = extract_typed_address (valaddr, ptr_type);

	  gdb_printf (stream, "(");
	  type_print (type, "", stream, -1);
	  gdb_printf (stream, ") ");
	  gdb_puts (paddress (gdbarch, addr), stream);
	}
      else
	{
	  value_print_scalar_formatted (val, options, 0, stream);
	  if (ada_is_character_type (type))
	    {
	      LONGEST c;

	      gdb_puts (" ", stream);
	      c = unpack_long (type, valaddr);
	      ada_printchar (c, type, stream);
	    }
	}
      return;
    }
}

/* Implement Ada val_print'ing for the case where TYPE is
   a TYPE_CODE_ENUM.  */

static void
ada_val_print_enum (struct value *value, struct ui_file *stream, int recurse,
		    const struct value_print_options *options)
{
  LONGEST val;

  if (options->format)
    {
      value_print_scalar_formatted (value, options, 0, stream);
      return;
    }

  struct type *type = ada_check_typedef (value->type ());
  const gdb_byte *valaddr = value->contents_for_printing ().data ();
  int offset_aligned = ada_aligned_value_addr (type, valaddr) - valaddr;

  val = unpack_long (type, valaddr + offset_aligned);
  std::optional<LONGEST> posn = discrete_position (type, val);
  if (posn.has_value ())
    {
      const char *name = ada_enum_name (type->field (*posn).name ());

      if (name[0] == '\'')
	gdb_printf (stream, "%ld %ps", (long) val,
		    styled_string (variable_name_style.style (),
				   name));
      else
	fputs_styled (name, variable_name_style.style (), stream);
    }
  else
    print_longest (stream, 'd', 0, val);
}

/* Implement Ada val_print'ing for the case where the type is
   TYPE_CODE_STRUCT or TYPE_CODE_UNION.  */

static void
ada_val_print_struct_union (struct value *value,
			    struct ui_file *stream,
			    int recurse,
			    const struct value_print_options *options)
{
  gdb_printf (stream, "(");

  if (print_field_values (value, value, stream, recurse, options,
			  0, language_def (language_ada)) != 0
      && options->prettyformat)
    {
      gdb_printf (stream, "\n");
      print_spaces (2 * recurse, stream);
    }

  gdb_printf (stream, ")");
}

/* Implement Ada value_print'ing for the case where TYPE is a
   TYPE_CODE_ARRAY.  */

static void
ada_value_print_array (struct value *val, struct ui_file *stream, int recurse,
		       const struct value_print_options *options)
{
  struct type *type = ada_check_typedef (val->type ());

  /* For an array of characters, print with string syntax.  */
  if (ada_is_string_type (type)
      && (options->format == 0 || options->format == 's'))
    {
      const gdb_byte *valaddr = val->contents_for_printing ().data ();
      int offset_aligned = ada_aligned_value_addr (type, valaddr) - valaddr;

      ada_val_print_string (type, valaddr, offset_aligned, stream, recurse,
			    options);
      return;
    }

  gdb_printf (stream, "(");
  print_optional_low_bound (stream, type, options);

  if (val->entirely_optimized_out ())
    val_print_optimized_out (val, stream);
  else if (type->field (0).bitsize () > 0)
    {
      const gdb_byte *valaddr = val->contents_for_printing ().data ();
      int offset_aligned = ada_aligned_value_addr (type, valaddr) - valaddr;
      val_print_packed_array_elements (type, valaddr, offset_aligned,
				       stream, recurse, options);
    }
  else
    value_print_array_elements (val, stream, recurse, options, 0);
  gdb_printf (stream, ")");
}

/* Implement Ada val_print'ing for the case where TYPE is
   a TYPE_CODE_REF.  */

static void
ada_val_print_ref (struct type *type, const gdb_byte *valaddr,
		   int offset, int offset_aligned, CORE_ADDR address,
		   struct ui_file *stream, int recurse,
		   struct value *original_value,
		   const struct value_print_options *options)
{
  /* For references, the debugger is expected to print the value as
     an address if DEREF_REF is null.  But printing an address in place
     of the object value would be confusing to an Ada programmer.
     So, for Ada values, we print the actual dereferenced value
     regardless.  */
  struct type *elttype = check_typedef (type->target_type ());
  struct value *deref_val;
  CORE_ADDR deref_val_int;

  if (elttype->code () == TYPE_CODE_UNDEF)
    {
      fputs_styled ("<ref to undefined type>", metadata_style.style (),
		    stream);
      return;
    }

  deref_val = coerce_ref_if_computed (original_value);
  if (deref_val)
    {
      if (ada_is_tagged_type (deref_val->type (), 1))
	deref_val = ada_tag_value_at_base_address (deref_val);

      common_val_print (deref_val, stream, recurse + 1, options,
			language_def (language_ada));
      return;
    }

  deref_val_int = unpack_pointer (type, valaddr + offset_aligned);
  if (deref_val_int == 0)
    {
      gdb_puts ("(null)", stream);
      return;
    }

  deref_val
    = ada_value_ind (value_from_pointer (lookup_pointer_type (elttype),
					 deref_val_int));
  if (ada_is_tagged_type (deref_val->type (), 1))
    deref_val = ada_tag_value_at_base_address (deref_val);

  if (deref_val->lazy ())
    deref_val->fetch_lazy ();

  common_val_print (deref_val, stream, recurse + 1,
		    options, language_def (language_ada));
}

/* See the comment on ada_value_print.  This function differs in that
   it does not catch evaluation errors (leaving that to its
   caller).  */

void
ada_value_print_inner (struct value *val, struct ui_file *stream, int recurse,
		       const struct value_print_options *options)
{
  struct type *type = ada_check_typedef (val->type ());

  if (ada_is_array_descriptor_type (type)
      || (ada_is_constrained_packed_array_type (type)
	  && type->code () != TYPE_CODE_PTR))
    {
      /* If this is a reference, coerce it now.  This helps taking
	 care of the case where ADDRESS is meaningless because
	 original_value was not an lval.  */
      val = coerce_ref (val);
      val = ada_get_decoded_value (val);
      if (val == nullptr)
	{
	  gdb_assert (type->code () == TYPE_CODE_TYPEDEF);
	  gdb_printf (stream, "0x0");
	  return;
	}
    }
  else
    val = ada_to_fixed_value (val);

  type = val->type ();
  struct type *saved_type = type;

  const gdb_byte *valaddr = val->contents_for_printing ().data ();
  CORE_ADDR address = val->address ();
  gdb::array_view<const gdb_byte> view
    = gdb::make_array_view (valaddr, type->length ());
  type = ada_check_typedef (resolve_dynamic_type (type, view, address));
  if (type != saved_type)
    {
      val = val->copy ();
      val->deprecated_set_type (type);
    }

  if (is_fixed_point_type (type))
    type = type->fixed_point_type_base_type ();

  switch (type->code ())
    {
    default:
      common_val_print (val, stream, recurse, options,
			language_def (language_c));
      break;

    case TYPE_CODE_PTR:
      ada_value_print_ptr (val, stream, recurse, options);
      break;

    case TYPE_CODE_INT:
    case TYPE_CODE_RANGE:
      ada_value_print_num (val, stream, recurse, options);
      break;

    case TYPE_CODE_ENUM:
      ada_val_print_enum (val, stream, recurse, options);
      break;

    case TYPE_CODE_FLT:
      if (options->format)
	{
	  common_val_print (val, stream, recurse, options,
			    language_def (language_c));
	  break;
	}

      ada_print_floating (valaddr, type, stream);
      break;

    case TYPE_CODE_UNION:
    case TYPE_CODE_STRUCT:
      ada_val_print_struct_union (val, stream, recurse, options);
      break;

    case TYPE_CODE_ARRAY:
      ada_value_print_array (val, stream, recurse, options);
      return;

    case TYPE_CODE_REF:
      ada_val_print_ref (type, valaddr, 0, 0,
			 address, stream, recurse, val,
			 options);
      break;
    }
}

void
ada_value_print (struct value *val0, struct ui_file *stream,
		 const struct value_print_options *options)
{
  struct value *val = ada_to_fixed_value (val0);
  struct type *type = ada_check_typedef (val->type ());
  struct value_print_options opts;

  /* If it is a pointer, indicate what it points to; but not for
     "void *" pointers.  */
  if (type->code () == TYPE_CODE_PTR
      && !(type->target_type ()->code () == TYPE_CODE_INT
	   && type->target_type ()->length () == 0))
    {
      /* Hack:  don't print (char *) for char strings.  Their
	 type is indicated by the quoted string anyway.  */
      if (type->target_type ()->length () != sizeof (char)
	  || type->target_type ()->code () != TYPE_CODE_INT
	  || type->target_type ()->is_unsigned ())
	{
	  gdb_printf (stream, "(");
	  type_print (type, "", stream, -1);
	  gdb_printf (stream, ") ");
	}
    }
  else if (ada_is_array_descriptor_type (type))
    {
      /* We do not print the type description unless TYPE is an array
	 access type (this is encoded by the compiler as a typedef to
	 a fat pointer - hence the check against TYPE_CODE_TYPEDEF).  */
      if (type->code () == TYPE_CODE_TYPEDEF)
	{
	  gdb_printf (stream, "(");
	  type_print (type, "", stream, -1);
	  gdb_printf (stream, ") ");
	}
    }

  opts = *options;
  opts.deref_ref = true;
  common_val_print (val, stream, 0, &opts, current_language);
}
