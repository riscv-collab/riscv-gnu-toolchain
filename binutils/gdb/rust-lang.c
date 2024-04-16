/* Rust language support routines for GDB, the GNU debugger.

   Copyright (C) 2016-2024 Free Software Foundation, Inc.

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

#include "block.h"
#include "c-lang.h"
#include "charset.h"
#include "cp-support.h"
#include "demangle.h"
#include "gdbarch.h"
#include "infcall.h"
#include "objfiles.h"
#include "rust-lang.h"
#include "typeprint.h"
#include "valprint.h"
#include "varobj.h"
#include <algorithm>
#include <string>
#include <vector>
#include "cli/cli-style.h"
#include "parser-defs.h"
#include "rust-exp.h"

/* See rust-lang.h.  */

const char *
rust_last_path_segment (const char *path)
{
  const char *result = strrchr (path, ':');

  if (result == NULL)
    return path;
  return result + 1;
}

/* See rust-lang.h.  */

std::string
rust_crate_for_block (const struct block *block)
{
  const char *scope = block->scope ();

  if (scope[0] == '\0')
    return std::string ();

  return std::string (scope, cp_find_first_component (scope));
}

/* Return true if TYPE, which must be a struct type, represents a Rust
   enum.  */

static bool
rust_enum_p (struct type *type)
{
  /* is_dynamic_type will return true if any field has a dynamic
     attribute -- but we only want to check the top level.  */
  return TYPE_HAS_VARIANT_PARTS (type);
}

/* Return true if TYPE, which must be an already-resolved enum type,
   has no variants.  */

static bool
rust_empty_enum_p (const struct type *type)
{
  return type->num_fields () == 0;
}

/* Given an already-resolved enum type and contents, find which
   variant is active.  */

static int
rust_enum_variant (struct type *type)
{
  /* The active variant is simply the first non-artificial field.  */
  for (int i = 0; i < type->num_fields (); ++i)
    if (!type->field (i).is_artificial ())
      return i;

  /* Perhaps we could get here by trying to print an Ada variant
     record in Rust mode.  Unlikely, but an error is safer than an
     assert.  */
  error (_("Could not find active enum variant"));
}

/* See rust-lang.h.  */

bool
rust_tuple_type_p (struct type *type)
{
  /* The current implementation is a bit of a hack, but there's
     nothing else in the debuginfo to distinguish a tuple from a
     struct.  */
  return (type->code () == TYPE_CODE_STRUCT
	  && type->name () != NULL
	  && type->name ()[0] == '(');
}

/* Return true if all non-static fields of a structlike type are in a
   sequence like __0, __1, __2.  */

static bool
rust_underscore_fields (struct type *type)
{
  int i, field_number;

  field_number = 0;

  if (type->code () != TYPE_CODE_STRUCT)
    return false;
  for (i = 0; i < type->num_fields (); ++i)
    {
      if (!type->field (i).is_static ())
	{
	  char buf[20];

	  xsnprintf (buf, sizeof (buf), "__%d", field_number);
	  if (strcmp (buf, type->field (i).name ()) != 0)
	    return false;
	  field_number++;
	}
    }
  return true;
}

/* See rust-lang.h.  */

bool
rust_tuple_struct_type_p (struct type *type)
{
  /* This is just an approximation until DWARF can represent Rust more
     precisely.  We exclude zero-length structs because they may not
     be tuple structs, and there's no way to tell.  */
  return type->num_fields () > 0 && rust_underscore_fields (type);
}

/* See rust-lang.h.  */

bool
rust_slice_type_p (const struct type *type)
{
  if (type->code () == TYPE_CODE_STRUCT
      && type->name () != NULL
      && type->num_fields () == 2)
    {
      /* The order of fields doesn't matter.  While it would be nice
	 to check for artificiality here, the Rust compiler doesn't
	 emit this information.  */
      const char *n1 = type->field (0).name ();
      const char *n2 = type->field (1).name ();
      return ((streq (n1, "data_ptr") && streq (n2, "length"))
	      || (streq (n2, "data_ptr") && streq (n1, "length")));
    }
  return false;
}

/* Return true if TYPE is a range type, otherwise false.  */

static bool
rust_range_type_p (struct type *type)
{
  int i;

  if (type->code () != TYPE_CODE_STRUCT
      || type->num_fields () > 2
      || type->name () == NULL
      || strstr (type->name (), "::Range") == NULL)
    return false;

  if (type->num_fields () == 0)
    return true;

  i = 0;
  if (strcmp (type->field (0).name (), "start") == 0)
    {
      if (type->num_fields () == 1)
	return true;
      i = 1;
    }
  else if (type->num_fields () == 2)
    {
      /* First field had to be "start".  */
      return false;
    }

  return strcmp (type->field (i).name (), "end") == 0;
}

/* Return true if TYPE is an inclusive range type, otherwise false.
   This is only valid for types which are already known to be range
   types.  */

static bool
rust_inclusive_range_type_p (struct type *type)
{
  return (strstr (type->name (), "::RangeInclusive") != NULL
	  || strstr (type->name (), "::RangeToInclusive") != NULL);
}

/* Return true if TYPE seems to be the type "u8", otherwise false.  */

static bool
rust_u8_type_p (struct type *type)
{
  return (type->code () == TYPE_CODE_INT
	  && type->is_unsigned ()
	  && type->length () == 1);
}

/* Return true if TYPE is a Rust character type.  */

static bool
rust_chartype_p (struct type *type)
{
  return (type->code () == TYPE_CODE_CHAR
	  && type->length () == 4
	  && type->is_unsigned ());
}

/* If VALUE represents a trait object pointer, return the underlying
   pointer with the correct (i.e., runtime) type.  Otherwise, return
   NULL.  */

static struct value *
rust_get_trait_object_pointer (struct value *value)
{
  struct type *type = check_typedef (value->type ());

  if (type->code () != TYPE_CODE_STRUCT || type->num_fields () != 2)
    return NULL;

  /* Try to be a bit resilient if the ABI changes.  */
  int vtable_field = 0;
  for (int i = 0; i < 2; ++i)
    {
      if (strcmp (type->field (i).name (), "vtable") == 0)
	vtable_field = i;
      else if (strcmp (type->field (i).name (), "pointer") != 0)
	return NULL;
    }

  CORE_ADDR vtable = value_as_address (value_field (value, vtable_field));
  struct symbol *symbol = find_symbol_at_address (vtable);
  if (symbol == NULL || symbol->subclass != SYMBOL_RUST_VTABLE)
    return NULL;

  struct rust_vtable_symbol *vtable_sym
    = static_cast<struct rust_vtable_symbol *> (symbol);
  struct type *pointer_type = lookup_pointer_type (vtable_sym->concrete_type);
  return value_cast (pointer_type, value_field (value, 1 - vtable_field));
}



/* See language.h.  */

void
rust_language::printstr (struct ui_file *stream, struct type *type,
			 const gdb_byte *string, unsigned int length,
			 const char *user_encoding, int force_ellipses,
			 const struct value_print_options *options) const
{
  /* Rust always uses UTF-8, but let the caller override this if need
     be.  */
  const char *encoding = user_encoding;
  if (user_encoding == NULL || !*user_encoding)
    {
      /* In Rust strings, characters are "u8".  */
      if (rust_u8_type_p (type))
	encoding = "UTF-8";
      else
	{
	  /* This is probably some C string, so let's let C deal with
	     it.  */
	  language_defn::printstr (stream, type, string, length,
				   user_encoding, force_ellipses,
				   options);
	  return;
	}
    }

  /* This is not ideal as it doesn't use our character printer.  */
  generic_printstr (stream, type, string, length, encoding, force_ellipses,
		    '"', 0, options);
}



static const struct generic_val_print_decorations rust_decorations =
{
  /* Complex isn't used in Rust, but we provide C-ish values just in
     case.  */
  "",
  " + ",
  " * I",
  "true",
  "false",
  "()",
  "[",
  "]"
};

/* See rust-lang.h.  */

struct value *
rust_slice_to_array (struct value *val)
{
  struct type *type = check_typedef (val->type ());
  /* This must have been checked by the caller.  */
  gdb_assert (rust_slice_type_p (type));

  struct value *base = value_struct_elt (&val, {}, "data_ptr", NULL,
					 "slice");
  struct value *len = value_struct_elt (&val, {}, "length", NULL, "slice");
  LONGEST llen = value_as_long (len);

  struct type *elt_type = base->type ()->target_type ();
  struct type *array_type = lookup_array_range_type (elt_type, 0,
						     llen - 1);
  struct value *array = value::allocate_lazy (array_type);
  array->set_lval (lval_memory);
  array->set_address (value_as_address (base));

  return array;
}

/* Helper function to print a slice.  */

static void
rust_val_print_slice (struct value *val, struct ui_file *stream, int recurse,
		      const struct value_print_options *options)
{
  struct value *base = value_struct_elt (&val, {}, "data_ptr", NULL,
					 "slice");
  struct value *len = value_struct_elt (&val, {}, "length", NULL, "slice");

  struct type *type = check_typedef (val->type ());
  if (strcmp (type->name (), "&str") == 0)
    val_print_string (base->type ()->target_type (), "UTF-8",
		      value_as_address (base), value_as_long (len), stream,
		      options);
  else
    {
      LONGEST llen = value_as_long (len);

      type_print (val->type (), "", stream, -1);
      gdb_printf (stream, " ");

      if (llen == 0)
	gdb_printf (stream, "[]");
      else
	{
	  struct value *array = rust_slice_to_array (val);
	  array->fetch_lazy ();
	  generic_value_print (array, stream, recurse, options,
			       &rust_decorations);
	}
    }
}

/* See rust-lang.h.  */

void
rust_language::val_print_struct
	(struct value *val, struct ui_file *stream, int recurse,
	 const struct value_print_options *options) const
{
  int i;
  int first_field;
  struct type *type = check_typedef (val->type ());

  if (rust_slice_type_p (type))
    {
      rust_val_print_slice (val, stream, recurse, options);
      return;
    }

  bool is_tuple = rust_tuple_type_p (type);
  bool is_tuple_struct = !is_tuple && rust_tuple_struct_type_p (type);
  struct value_print_options opts;

  if (!is_tuple)
    {
      if (type->name () != NULL)
	gdb_printf (stream, "%s", type->name ());

      if (type->num_fields () == 0)
	return;

      if (type->name () != NULL)
	gdb_puts (" ", stream);
    }

  if (is_tuple || is_tuple_struct)
    gdb_puts ("(", stream);
  else
    gdb_puts ("{", stream);

  opts = *options;
  opts.deref_ref = false;

  first_field = 1;
  for (i = 0; i < type->num_fields (); ++i)
    {
      if (type->field (i).is_static ())
	continue;

      if (!first_field)
	gdb_puts (",", stream);

      if (options->prettyformat)
	{
	  gdb_puts ("\n", stream);
	  print_spaces (2 + 2 * recurse, stream);
	}
      else if (!first_field)
	gdb_puts (" ", stream);

      first_field = 0;

      if (!is_tuple && !is_tuple_struct)
	{
	  fputs_styled (type->field (i).name (),
			variable_name_style.style (), stream);
	  gdb_puts (": ", stream);
	}

      common_val_print (value_field (val, i), stream, recurse + 1, &opts,
			this);
    }

  if (options->prettyformat)
    {
      gdb_puts ("\n", stream);
      print_spaces (2 * recurse, stream);
    }

  if (is_tuple || is_tuple_struct)
    gdb_puts (")", stream);
  else
    gdb_puts ("}", stream);
}

/* See rust-lang.h.  */

void
rust_language::print_enum (struct value *val, struct ui_file *stream,
			   int recurse,
			   const struct value_print_options *options) const
{
  struct value_print_options opts = *options;
  struct type *type = check_typedef (val->type ());

  opts.deref_ref = false;

  gdb_assert (rust_enum_p (type));
  gdb::array_view<const gdb_byte> view
    (val->contents_for_printing ().data (),
     val->type ()->length ());
  type = resolve_dynamic_type (type, view, val->address ());

  if (rust_empty_enum_p (type))
    {
      /* Print the enum type name here to be more clear.  */
      gdb_printf (stream, _("%s {%p[<No data fields>%p]}"),
		  type->name (),
		  metadata_style.style ().ptr (), nullptr);
      return;
    }

  int variant_fieldno = rust_enum_variant (type);
  val = val->primitive_field (0, variant_fieldno, type);
  struct type *variant_type = type->field (variant_fieldno).type ();

  int nfields = variant_type->num_fields ();

  bool is_tuple = rust_tuple_struct_type_p (variant_type);

  gdb_printf (stream, "%s", variant_type->name ());
  if (nfields == 0)
    {
      /* In case of a nullary variant like 'None', just output
	 the name. */
      return;
    }

  /* In case of a non-nullary variant, we output 'Foo(x,y,z)'. */
  if (is_tuple)
    gdb_printf (stream, "(");
  else
    {
      /* struct variant.  */
      gdb_printf (stream, "{");
    }

  bool first_field = true;
  for (int j = 0; j < nfields; j++)
    {
      if (!first_field)
	gdb_puts (", ", stream);
      first_field = false;

      if (!is_tuple)
	gdb_printf (stream, "%ps: ",
		    styled_string (variable_name_style.style (),
				   variant_type->field (j).name ()));

      common_val_print (value_field (val, j), stream, recurse + 1, &opts,
			this);
    }

  if (is_tuple)
    gdb_puts (")", stream);
  else
    gdb_puts ("}", stream);
}

/* See language.h.  */

void
rust_language::value_print_inner
	(struct value *val, struct ui_file *stream, int recurse,
	 const struct value_print_options *options) const
{
  struct value_print_options opts = *options;
  opts.deref_ref = true;

  if (opts.prettyformat == Val_prettyformat_default)
    opts.prettyformat = (opts.prettyformat_structs
			 ? Val_prettyformat : Val_no_prettyformat);

  struct type *type = check_typedef (val->type ());
  switch (type->code ())
    {
    case TYPE_CODE_PTR:
      {
	LONGEST low_bound, high_bound;
	
	if (type->target_type ()->code () == TYPE_CODE_ARRAY
	    && rust_u8_type_p (type->target_type ()->target_type ())
	    && get_array_bounds (type->target_type (), &low_bound,
				 &high_bound))
	  {
	    /* We have a pointer to a byte string, so just print
	       that.  */
	    struct type *elttype = check_typedef (type->target_type ());
	    CORE_ADDR addr = value_as_address (val);
	    struct gdbarch *arch = type->arch ();

	    if (opts.addressprint)
	      {
		gdb_puts (paddress (arch, addr), stream);
		gdb_puts (" ", stream);
	      }

	    gdb_puts ("b", stream);
	    val_print_string (elttype->target_type (), "ASCII", addr,
			      high_bound - low_bound + 1, stream,
			      &opts);
	    break;
	  }
      }
      goto generic_print;

    case TYPE_CODE_INT:
      /* Recognize the unit type.  */
      if (type->is_unsigned () && type->length () == 0
	  && type->name () != NULL && strcmp (type->name (), "()") == 0)
	{
	  gdb_puts ("()", stream);
	  break;
	}
      goto generic_print;

    case TYPE_CODE_STRING:
      {
	LONGEST low_bound, high_bound;

	if (!get_array_bounds (type, &low_bound, &high_bound))
	  error (_("Could not determine the array bounds"));

	/* If we see a plain TYPE_CODE_STRING, then we're printing a
	   byte string, hence the choice of "ASCII" as the
	   encoding.  */
	gdb_puts ("b", stream);
	printstr (stream, type->target_type (),
		  val->contents_for_printing ().data (),
		  high_bound - low_bound + 1, "ASCII", 0, &opts);
      }
      break;

    case TYPE_CODE_ARRAY:
      {
	LONGEST low_bound, high_bound;

	if (get_array_bounds (type, &low_bound, &high_bound)
	    && high_bound - low_bound + 1 == 0)
	  gdb_puts ("[]", stream);
	else
	  goto generic_print;
      }
      break;

    case TYPE_CODE_UNION:
      /* Untagged unions are printed as if they are structs.  Since
	 the field bit positions overlap in the debuginfo, the code
	 for printing a union is same as that for a struct, the only
	 difference is that the input type will have overlapping
	 fields.  */
      val_print_struct (val, stream, recurse, &opts);
      break;

    case TYPE_CODE_STRUCT:
      if (rust_enum_p (type))
	print_enum (val, stream, recurse, &opts);
      else
	val_print_struct (val, stream, recurse, &opts);
      break;

    default:
    generic_print:
      /* Nothing special yet.  */
      generic_value_print (val, stream, recurse, &opts, &rust_decorations);
    }
}

/* See language.h.  */

void
rust_language::value_print
	(struct value *val, struct ui_file *stream,
	 const struct value_print_options *options) const
{
  value_print_options opts = *options;
  opts.deref_ref = true;

  struct type *type = check_typedef (val->type ());
  if (type->is_pointer_or_reference ())
    {
      gdb_printf (stream, "(");
      type_print (val->type (), "", stream, -1);
      gdb_printf (stream, ") ");
    }

  return common_val_print (val, stream, 0, &opts, this);
}



static void
rust_internal_print_type (struct type *type, const char *varstring,
			  struct ui_file *stream, int show, int level,
			  const struct type_print_options *flags,
			  bool for_rust_enum, print_offset_data *podata);

/* Print a struct or union typedef.  */
static void
rust_print_struct_def (struct type *type, const char *varstring,
		       struct ui_file *stream, int show, int level,
		       const struct type_print_options *flags,
		       bool for_rust_enum, print_offset_data *podata)
{
  /* Print a tuple type simply.  */
  if (rust_tuple_type_p (type))
    {
      gdb_puts (type->name (), stream);
      return;
    }

  /* If we see a base class, delegate to C.  */
  if (TYPE_N_BASECLASSES (type) > 0)
    c_print_type (type, varstring, stream, show, level, language_rust, flags);

  if (flags->print_offsets)
    {
      /* Temporarily bump the level so that the output lines up
	 correctly.  */
      level += 2;
    }

  /* Compute properties of TYPE here because, in the enum case, the
     rest of the code ends up looking only at the variant part.  */
  const char *tagname = type->name ();
  bool is_tuple_struct = rust_tuple_struct_type_p (type);
  bool is_tuple = rust_tuple_type_p (type);
  bool is_enum = rust_enum_p (type);

  if (for_rust_enum)
    {
      /* Already printing an outer enum, so nothing to print here.  */
    }
  else
    {
      /* This code path is also used by unions and enums.  */
      if (is_enum)
	{
	  gdb_puts ("enum ", stream);
	  dynamic_prop *prop = type->dyn_prop (DYN_PROP_VARIANT_PARTS);
	  if (prop != nullptr && prop->kind () == PROP_TYPE)
	    type = prop->original_type ();
	}
      else if (type->code () == TYPE_CODE_STRUCT)
	gdb_puts ("struct ", stream);
      else
	gdb_puts ("union ", stream);

      if (tagname != NULL)
	gdb_puts (tagname, stream);
    }

  if (type->num_fields () == 0 && !is_tuple)
    return;
  if (for_rust_enum && !flags->print_offsets)
    gdb_puts (is_tuple_struct ? "(" : "{", stream);
  else
    gdb_puts (is_tuple_struct ? " (\n" : " {\n", stream);

  /* When printing offsets, we rearrange the fields into storage
     order.  This lets us show holes more clearly.  We work using
     field indices here because it simplifies calls to
     print_offset_data::update below.  */
  std::vector<int> fields;
  for (int i = 0; i < type->num_fields (); ++i)
    {
      if (type->field (i).is_static ())
	continue;
      if (is_enum && type->field (i).is_artificial ())
	continue;
      fields.push_back (i);
    }
  if (flags->print_offsets)
    std::sort (fields.begin (), fields.end (),
	       [&] (int a, int b)
	       {
		 return (type->field (a).loc_bitpos ()
			 < type->field (b).loc_bitpos ());
	       });

  for (int i : fields)
    {
      QUIT;

      gdb_assert (!type->field (i).is_static ());
      gdb_assert (! (is_enum && type->field (i).is_artificial ()));

      if (flags->print_offsets)
	podata->update (type, i, stream);

      /* We'd like to print "pub" here as needed, but rustc
	 doesn't emit the debuginfo, and our types don't have
	 cplus_struct_type attached.  */

      /* For a tuple struct we print the type but nothing
	 else.  */
      if (!for_rust_enum || flags->print_offsets)
	print_spaces (level + 2, stream);
      if (is_enum)
	fputs_styled (type->field (i).name (), variable_name_style.style (),
		      stream);
      else if (!is_tuple_struct)
	gdb_printf (stream, "%ps: ",
		    styled_string (variable_name_style.style (),
				   type->field (i).name ()));

      rust_internal_print_type (type->field (i).type (), NULL,
				stream, (is_enum ? show : show - 1),
				level + 2, flags, is_enum, podata);
      if (!for_rust_enum || flags->print_offsets)
	gdb_puts (",\n", stream);
      /* Note that this check of "I" is ok because we only sorted the
	 fields by offset when print_offsets was set, so we won't take
	 this branch in that case.  */
      else if (i + 1 < type->num_fields ())
	gdb_puts (", ", stream);
    }

  if (flags->print_offsets)
    {
      /* Undo the temporary level increase we did above.  */
      level -= 2;
      podata->finish (type, level, stream);
      print_spaces (print_offset_data::indentation, stream);
      if (level == 0)
	print_spaces (2, stream);
    }
  if (!for_rust_enum || flags->print_offsets)
    print_spaces (level, stream);
  gdb_puts (is_tuple_struct ? ")" : "}", stream);
}

/* la_print_type implementation for Rust.  */

static void
rust_internal_print_type (struct type *type, const char *varstring,
			  struct ui_file *stream, int show, int level,
			  const struct type_print_options *flags,
			  bool for_rust_enum, print_offset_data *podata)
{
  QUIT;
  if (show <= 0
      && type->name () != NULL)
    {
      /* Rust calls the unit type "void" in its debuginfo,
	 but we don't want to print it as that.  */
      if (type->code () == TYPE_CODE_VOID)
	gdb_puts ("()", stream);
      else
	gdb_puts (type->name (), stream);
      return;
    }

  type = check_typedef (type);
  switch (type->code ())
    {
    case TYPE_CODE_VOID:
      /* If we have an enum, we've already printed the type's
	 unqualified name, and there is nothing else to print
	 here.  */
      if (!for_rust_enum)
	gdb_puts ("()", stream);
      break;

    case TYPE_CODE_FUNC:
      /* Delegate varargs to the C printer.  */
      if (type->has_varargs ())
	goto c_printer;

      gdb_puts ("fn ", stream);
      if (varstring != NULL)
	gdb_puts (varstring, stream);
      gdb_puts ("(", stream);
      for (int i = 0; i < type->num_fields (); ++i)
	{
	  QUIT;
	  if (i > 0)
	    gdb_puts (", ", stream);
	  rust_internal_print_type (type->field (i).type (), "", stream,
				    -1, 0, flags, false, podata);
	}
      gdb_puts (")", stream);
      /* If it returns unit, we can omit the return type.  */
      if (type->target_type ()->code () != TYPE_CODE_VOID)
	{
	  gdb_puts (" -> ", stream);
	  rust_internal_print_type (type->target_type (), "", stream,
				    -1, 0, flags, false, podata);
	}
      break;

    case TYPE_CODE_ARRAY:
      {
	LONGEST low_bound, high_bound;

	gdb_puts ("[", stream);
	rust_internal_print_type (type->target_type (), NULL,
				  stream, show - 1, level, flags, false,
				  podata);

	if (type->bounds ()->high.kind () == PROP_LOCEXPR
	    || type->bounds ()->high.kind () == PROP_LOCLIST)
	  gdb_printf (stream, "; variable length");
	else if (get_array_bounds (type, &low_bound, &high_bound))
	  gdb_printf (stream, "; %s",
		      plongest (high_bound - low_bound + 1));
	gdb_puts ("]", stream);
      }
      break;

    case TYPE_CODE_UNION:
    case TYPE_CODE_STRUCT:
      rust_print_struct_def (type, varstring, stream, show, level, flags,
			     for_rust_enum, podata);
      break;

    case TYPE_CODE_ENUM:
      {
	int len = 0;

	gdb_puts ("enum ", stream);
	if (type->name () != NULL)
	  {
	    gdb_puts (type->name (), stream);
	    gdb_puts (" ", stream);
	    len = strlen (type->name ());
	  }
	gdb_puts ("{\n", stream);

	for (int i = 0; i < type->num_fields (); ++i)
	  {
	    const char *name = type->field (i).name ();

	    QUIT;

	    if (len > 0
		&& strncmp (name, type->name (), len) == 0
		&& name[len] == ':'
		&& name[len + 1] == ':')
	      name += len + 2;
	    gdb_printf (stream, "%*s%ps,\n",
			level + 2, "",
			styled_string (variable_name_style.style (),
				       name));
	  }

	gdb_puts ("}", stream);
      }
      break;

    case TYPE_CODE_PTR:
      {
	if (type->name () != nullptr)
	  gdb_puts (type->name (), stream);
	else
	  {
	    /* We currently can't distinguish between pointers and
	       references.  */
	    gdb_puts ("*mut ", stream);
	    type_print (type->target_type (), "", stream, 0);
	  }
      }
      break;

    default:
    c_printer:
      c_print_type (type, varstring, stream, show, level, language_rust,
		    flags);
    }
}



/* Like arch_composite_type, but uses TYPE to decide how to allocate
   -- either on an obstack or on a gdbarch.  */

static struct type *
rust_composite_type (struct type *original,
		     const char *name,
		     const char *field1, struct type *type1,
		     const char *field2, struct type *type2)
{
  struct type *result = type_allocator (original).new_type ();
  int i, nfields, bitpos;

  nfields = 0;
  if (field1 != NULL)
    ++nfields;
  if (field2 != NULL)
    ++nfields;

  result->set_code (TYPE_CODE_STRUCT);
  result->set_name (name);

  result->alloc_fields (nfields);

  i = 0;
  bitpos = 0;
  if (field1 != NULL)
    {
      struct field *field = &result->field (i);

      field->set_loc_bitpos (bitpos);
      bitpos += type1->length () * TARGET_CHAR_BIT;

      field->set_name (field1);
      field->set_type (type1);
      ++i;
    }
  if (field2 != NULL)
    {
      struct field *field = &result->field (i);
      unsigned align = type_align (type2);

      if (align != 0)
	{
	  int delta;

	  align *= TARGET_CHAR_BIT;
	  delta = bitpos % align;
	  if (delta != 0)
	    bitpos += align - delta;
	}
      field->set_loc_bitpos (bitpos);

      field->set_name (field2);
      field->set_type (type2);
      ++i;
    }

  if (i > 0)
    result->set_length (result->field (i - 1).loc_bitpos () / TARGET_CHAR_BIT
			+ result->field (i - 1).type ()->length ());
  return result;
}

/* See rust-lang.h.  */

struct type *
rust_slice_type (const char *name, struct type *elt_type,
		 struct type *usize_type)
{
  struct type *type;

  elt_type = lookup_pointer_type (elt_type);
  type = rust_composite_type (elt_type, name,
			      "data_ptr", elt_type,
			      "length", usize_type);

  return type;
}



/* A helper for rust_evaluate_subexp that handles OP_RANGE.  */

struct value *
rust_range (struct type *expect_type, struct expression *exp,
	    enum noside noside, enum range_flag kind,
	    struct value *low, struct value *high)
{
  struct value *addrval, *result;
  CORE_ADDR addr;
  struct type *range_type;
  struct type *index_type;
  struct type *temp_type;
  const char *name;

  bool inclusive = !(kind & RANGE_HIGH_BOUND_EXCLUSIVE);

  if (low == NULL)
    {
      if (high == NULL)
	{
	  index_type = NULL;
	  name = "std::ops::RangeFull";
	}
      else
	{
	  index_type = high->type ();
	  name = (inclusive
		  ? "std::ops::RangeToInclusive" : "std::ops::RangeTo");
	}
    }
  else
    {
      if (high == NULL)
	{
	  index_type = low->type ();
	  name = "std::ops::RangeFrom";
	}
      else
	{
	  if (!types_equal (low->type (), high->type ()))
	    error (_("Range expression with different types"));
	  index_type = low->type ();
	  name = inclusive ? "std::ops::RangeInclusive" : "std::ops::Range";
	}
    }

  /* If we don't have an index type, just allocate this on the
     arch.  Here any type will do.  */
  temp_type = (index_type == NULL
	       ? language_bool_type (exp->language_defn, exp->gdbarch)
	       : index_type);
  /* It would be nicer to cache the range type.  */
  range_type = rust_composite_type (temp_type, name,
				    low == NULL ? NULL : "start", index_type,
				    high == NULL ? NULL : "end", index_type);

  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    return value::zero (range_type, lval_memory);

  addrval = value_allocate_space_in_inferior (range_type->length ());
  addr = value_as_long (addrval);
  result = value_at_lazy (range_type, addr);

  if (low != NULL)
    {
      struct value *start = value_struct_elt (&result, {}, "start", NULL,
					      "range");

      value_assign (start, low);
    }

  if (high != NULL)
    {
      struct value *end = value_struct_elt (&result, {}, "end", NULL,
					    "range");

      value_assign (end, high);
    }

  result = value_at_lazy (range_type, addr);
  return result;
}

/* A helper function to compute the range and kind given a range
   value.  TYPE is the type of the range value.  RANGE is the range
   value.  LOW, HIGH, and KIND are out parameters.  The LOW and HIGH
   parameters might be filled in, or might not be, depending on the
   kind of range this is.  KIND will always be set to the appropriate
   value describing the kind of range, and this can be used to
   determine whether LOW or HIGH are valid.  */

static void
rust_compute_range (struct type *type, struct value *range,
		    LONGEST *low, LONGEST *high,
		    range_flags *kind)
{
  int i;

  *low = 0;
  *high = 0;
  *kind = RANGE_LOW_BOUND_DEFAULT | RANGE_HIGH_BOUND_DEFAULT;

  if (type->num_fields () == 0)
    return;

  i = 0;
  if (strcmp (type->field (0).name (), "start") == 0)
    {
      *kind = RANGE_HIGH_BOUND_DEFAULT;
      *low = value_as_long (value_field (range, 0));
      ++i;
    }
  if (type->num_fields () > i
      && strcmp (type->field (i).name (), "end") == 0)
    {
      *kind = (*kind == (RANGE_LOW_BOUND_DEFAULT | RANGE_HIGH_BOUND_DEFAULT)
	       ? RANGE_LOW_BOUND_DEFAULT : RANGE_STANDARD);
      *high = value_as_long (value_field (range, i));

      if (rust_inclusive_range_type_p (type))
	++*high;
    }
}

/* A helper for rust_evaluate_subexp that handles BINOP_SUBSCRIPT.  */

struct value *
rust_subscript (struct type *expect_type, struct expression *exp,
		enum noside noside, bool for_addr,
		struct value *lhs, struct value *rhs)
{
  struct value *result;
  struct type *rhstype;
  LONGEST low, high_bound;
  /* Initialized to appease the compiler.  */
  range_flags kind = RANGE_LOW_BOUND_DEFAULT | RANGE_HIGH_BOUND_DEFAULT;
  LONGEST high = 0;
  int want_slice = 0;

  rhstype = check_typedef (rhs->type ());
  if (rust_range_type_p (rhstype))
    {
      if (!for_addr)
	error (_("Can't take slice of array without '&'"));
      rust_compute_range (rhstype, rhs, &low, &high, &kind);
      want_slice = 1;
    }
  else
    low = value_as_long (rhs);

  struct type *type = check_typedef (lhs->type ());
  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    {
      struct type *base_type = nullptr;
      if (type->code () == TYPE_CODE_ARRAY)
	base_type = type->target_type ();
      else if (rust_slice_type_p (type))
	{
	  for (int i = 0; i < type->num_fields (); ++i)
	    {
	      if (strcmp (type->field (i).name (), "data_ptr") == 0)
		{
		  base_type = type->field (i).type ()->target_type ();
		  break;
		}
	    }
	  if (base_type == nullptr)
	    error (_("Could not find 'data_ptr' in slice type"));
	}
      else if (type->code () == TYPE_CODE_PTR)
	base_type = type->target_type ();
      else
	error (_("Cannot subscript non-array type"));

      struct type *new_type;
      if (want_slice)
	{
	  if (rust_slice_type_p (type))
	    new_type = type;
	  else
	    {
	      struct type *usize
		= language_lookup_primitive_type (exp->language_defn,
						  exp->gdbarch,
						  "usize");
	      new_type = rust_slice_type ("&[*gdb*]", base_type, usize);
	    }
	}
      else
	new_type = base_type;

      return value::zero (new_type, lhs->lval ());
    }
  else
    {
      LONGEST low_bound;
      struct value *base;

      if (type->code () == TYPE_CODE_ARRAY)
	{
	  base = lhs;
	  if (!get_array_bounds (type, &low_bound, &high_bound))
	    error (_("Can't compute array bounds"));
	  if (low_bound != 0)
	    error (_("Found array with non-zero lower bound"));
	  ++high_bound;
	}
      else if (rust_slice_type_p (type))
	{
	  struct value *len;

	  base = value_struct_elt (&lhs, {}, "data_ptr", NULL, "slice");
	  len = value_struct_elt (&lhs, {}, "length", NULL, "slice");
	  low_bound = 0;
	  high_bound = value_as_long (len);
	}
      else if (type->code () == TYPE_CODE_PTR)
	{
	  base = lhs;
	  low_bound = 0;
	  high_bound = LONGEST_MAX;
	}
      else
	error (_("Cannot subscript non-array type"));

      if (want_slice && (kind & RANGE_LOW_BOUND_DEFAULT))
	low = low_bound;
      if (low < 0)
	error (_("Index less than zero"));
      if (low > high_bound)
	error (_("Index greater than length"));

      result = value_subscript (base, low);
    }

  if (for_addr)
    {
      if (want_slice)
	{
	  struct type *usize, *slice;
	  CORE_ADDR addr;
	  struct value *addrval, *tem;

	  if (kind & RANGE_HIGH_BOUND_DEFAULT)
	    high = high_bound;
	  if (high < 0)
	    error (_("High index less than zero"));
	  if (low > high)
	    error (_("Low index greater than high index"));
	  if (high > high_bound)
	    error (_("High index greater than length"));

	  usize = language_lookup_primitive_type (exp->language_defn,
						  exp->gdbarch,
						  "usize");
	  const char *new_name = ((type != nullptr
				   && rust_slice_type_p (type))
				  ? type->name () : "&[*gdb*]");

	  slice = rust_slice_type (new_name, result->type (), usize);

	  addrval = value_allocate_space_in_inferior (slice->length ());
	  addr = value_as_long (addrval);
	  tem = value_at_lazy (slice, addr);

	  value_assign (value_field (tem, 0), value_addr (result));
	  value_assign (value_field (tem, 1),
			value_from_longest (usize, high - low));

	  result = value_at_lazy (slice, addr);
	}
      else
	result = value_addr (result);
    }

  return result;
}

namespace expr
{

struct value *
rust_unop_ind_operation::evaluate (struct type *expect_type,
				   struct expression *exp,
				   enum noside noside)
{
  if (noside != EVAL_NORMAL)
    return unop_ind_operation::evaluate (expect_type, exp, noside);

  struct value *value = std::get<0> (m_storage)->evaluate (nullptr, exp,
							   noside);
  struct value *trait_ptr = rust_get_trait_object_pointer (value);
  if (trait_ptr != NULL)
    value = trait_ptr;

  return value_ind (value);
}

} /* namespace expr */

/* A helper function for UNOP_COMPLEMENT.  */

struct value *
eval_op_rust_complement (struct type *expect_type, struct expression *exp,
			 enum noside noside,
			 enum exp_opcode opcode,
			 struct value *value)
{
  if (value->type ()->code () == TYPE_CODE_BOOL)
    return value_from_longest (value->type (), value_logical_not (value));
  return value_complement (value);
}

/* A helper function for OP_ARRAY.  */

struct value *
eval_op_rust_array (struct type *expect_type, struct expression *exp,
		    enum noside noside,
		    enum exp_opcode opcode,
		    struct value *elt, struct value *ncopies)
{
  int copies = value_as_long (ncopies);
  if (copies < 0)
    error (_("Array with negative number of elements"));

  if (noside == EVAL_NORMAL)
    return value_array (0, std::vector<value *> (copies, elt));
  else
    {
      struct type *arraytype
	= lookup_array_range_type (elt->type (), 0, copies - 1);
      return value::allocate (arraytype);
    }
}

namespace expr
{

struct value *
rust_struct_anon::evaluate (struct type *expect_type,
			    struct expression *exp,
			    enum noside noside)
{
  value *lhs = std::get<1> (m_storage)->evaluate (nullptr, exp, noside);
  int field_number = std::get<0> (m_storage);

  struct type *type = lhs->type ();

  if (type->code () == TYPE_CODE_STRUCT)
    {
      struct type *outer_type = NULL;

      if (rust_enum_p (type))
	{
	  type = resolve_dynamic_type (type, lhs->contents (),
				       lhs->address ());

	  if (rust_empty_enum_p (type))
	    error (_("Cannot access field %d of empty enum %s"),
		   field_number, type->name ());

	  int fieldno = rust_enum_variant (type);
	  lhs = lhs->primitive_field (0, fieldno, type);
	  outer_type = type;
	  type = lhs->type ();
	}

      /* Tuples and tuple structs */
      int nfields = type->num_fields ();

      if (field_number >= nfields || field_number < 0)
	{
	  if (outer_type != NULL)
	    error(_("Cannot access field %d of variant %s::%s, "
		    "there are only %d fields"),
		  field_number, outer_type->name (),
		  rust_last_path_segment (type->name ()),
		  nfields);
	  else
	    error(_("Cannot access field %d of %s, "
		    "there are only %d fields"),
		  field_number, type->name (), nfields);
	}

      /* Tuples are tuple structs too.  */
      if (!rust_tuple_struct_type_p (type))
	{
	  if (outer_type != NULL)
	    error(_("Variant %s::%s is not a tuple variant"),
		  outer_type->name (),
		  rust_last_path_segment (type->name ()));
	  else
	    error(_("Attempting to access anonymous field %d "
		    "of %s, which is not a tuple, tuple struct, or "
		    "tuple-like variant"),
		  field_number, type->name ());
	}

      return lhs->primitive_field (0, field_number, type);
    }
  else
    error(_("Anonymous field access is only allowed on tuples, \
tuple structs, and tuple-like enum variants"));
}

struct value *
rust_structop::evaluate (struct type *expect_type,
			 struct expression *exp,
			 enum noside noside)
{
  value *lhs = std::get<0> (m_storage)->evaluate (nullptr, exp, noside);
  const char *field_name = std::get<1> (m_storage).c_str ();

  struct value *result;
  struct type *type = lhs->type ();
  if (type->code () == TYPE_CODE_STRUCT && rust_enum_p (type))
    {
      type = resolve_dynamic_type (type, lhs->contents (),
				   lhs->address ());

      if (rust_empty_enum_p (type))
	error (_("Cannot access field %s of empty enum %s"),
	       field_name, type->name ());

      int fieldno = rust_enum_variant (type);
      lhs = lhs->primitive_field (0, fieldno, type);

      struct type *outer_type = type;
      type = lhs->type ();
      if (rust_tuple_type_p (type) || rust_tuple_struct_type_p (type))
	error (_("Attempting to access named field %s of tuple "
		 "variant %s::%s, which has only anonymous fields"),
	       field_name, outer_type->name (),
	       rust_last_path_segment (type->name ()));

      try
	{
	  result = value_struct_elt (&lhs, {}, field_name,
				     NULL, "structure");
	}
      catch (const gdb_exception_error &except)
	{
	  error (_("Could not find field %s of struct variant %s::%s"),
		 field_name, outer_type->name (),
		 rust_last_path_segment (type->name ()));
	}
    }
  else
    result = value_struct_elt (&lhs, {}, field_name, NULL, "structure");
  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    result = value::zero (result->type (), result->lval ());
  return result;
}

value *
rust_aggregate_operation::evaluate (struct type *expect_type,
				    struct expression *exp,
				    enum noside noside)
{
  struct type *type = std::get<0> (m_storage);
  CORE_ADDR addr = 0;
  struct value *addrval = NULL;
  value *result;

  if (noside == EVAL_NORMAL)
    {
      addrval = value_allocate_space_in_inferior (type->length ());
      addr = value_as_long (addrval);
      result = value_at_lazy (type, addr);
    }

  if (std::get<1> (m_storage) != nullptr)
    {
      struct value *init = std::get<1> (m_storage)->evaluate (nullptr, exp,
							      noside);

      if (noside == EVAL_NORMAL)
	{
	  /* This isn't quite right but will do for the time
	     being, seeing that we can't implement the Copy
	     trait anyway.  */
	  value_assign (result, init);
	}
    }

  for (const auto &item : std::get<2> (m_storage))
    {
      value *val = item.second->evaluate (nullptr, exp, noside);
      if (noside == EVAL_NORMAL)
	{
	  const char *fieldname = item.first.c_str ();
	  value *field = value_struct_elt (&result, {}, fieldname,
					   nullptr, "structure");
	  value_assign (field, val);
	}
    }

  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    result = value::allocate (type);
  else
    result = value_at_lazy (type, addr);

  return result;
}

value *
rust_structop::evaluate_funcall (struct type *expect_type,
				 struct expression *exp,
				 enum noside noside,
				 const std::vector<operation_up> &ops)
{
  std::vector<struct value *> args (ops.size () + 1);

  /* Evaluate the argument to STRUCTOP_STRUCT, then find its
     type in order to look up the method.  */
  args[0] = std::get<0> (m_storage)->evaluate (nullptr, exp, noside);
  /* We don't yet implement real Deref semantics.  */
  while (args[0]->type ()->code () == TYPE_CODE_PTR)
    args[0] = value_ind (args[0]);

  struct type *type = args[0]->type ();
  if ((type->code () != TYPE_CODE_STRUCT
       && type->code () != TYPE_CODE_UNION
       && type->code () != TYPE_CODE_ENUM)
      || rust_tuple_type_p (type))
    error (_("Method calls only supported on struct or enum types"));
  if (type->name () == NULL)
    error (_("Method call on nameless type"));

  std::string name = (std::string (type->name ()) + "::"
		      + std::get<1> (m_storage));

  const struct block *block = get_selected_block (0);
  struct block_symbol sym = lookup_symbol (name.c_str (), block,
					   VAR_DOMAIN, NULL);
  if (sym.symbol == NULL)
    error (_("Could not find function named '%s'"), name.c_str ());

  struct type *fn_type = sym.symbol->type ();
  if (fn_type->num_fields () == 0)
    error (_("Function '%s' takes no arguments"), name.c_str ());

  if (fn_type->field (0).type ()->code () == TYPE_CODE_PTR)
    args[0] = value_addr (args[0]);

  value *function = address_of_variable (sym.symbol, block);

  for (int i = 0; i < ops.size (); ++i)
    args[i + 1] = ops[i]->evaluate (nullptr, exp, noside);

  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    return value::zero (fn_type->target_type (), not_lval);
  return call_function_by_hand (function, NULL, args);
}

}



/* See language.h.  */

void
rust_language::language_arch_info (struct gdbarch *gdbarch,
				   struct language_arch_info *lai) const
{
  const struct builtin_type *builtin = builtin_type (gdbarch);

  /* Helper function to allow shorter lines below.  */
  auto add  = [&] (struct type * t) -> struct type *
  {
    lai->add_primitive_type (t);
    return t;
  };

  type_allocator alloc (gdbarch);
  struct type *bool_type
    = add (init_boolean_type (alloc, 8, 1, "bool"));
  add (init_character_type (alloc, 32, 1, "char"));
  add (init_integer_type (alloc, 8, 0, "i8"));
  struct type *u8_type
    = add (init_integer_type (alloc, 8, 1, "u8"));
  add (init_integer_type (alloc, 16, 0, "i16"));
  add (init_integer_type (alloc, 16, 1, "u16"));
  add (init_integer_type (alloc, 32, 0, "i32"));
  add (init_integer_type (alloc, 32, 1, "u32"));
  add (init_integer_type (alloc, 64, 0, "i64"));
  add (init_integer_type (alloc, 64, 1, "u64"));
  add (init_integer_type (alloc, 128, 0, "i128"));
  add (init_integer_type (alloc, 128, 1, "u128"));

  unsigned int length = 8 * builtin->builtin_data_ptr->length ();
  add (init_integer_type (alloc, length, 0, "isize"));
  struct type *usize_type
    = add (init_integer_type (alloc, length, 1, "usize"));

  add (init_float_type (alloc, 32, "f32", floatformats_ieee_single));
  add (init_float_type (alloc, 64, "f64", floatformats_ieee_double));
  add (init_integer_type (alloc, 0, 1, "()"));

  struct type *tem = make_cv_type (1, 0, u8_type, NULL);
  add (rust_slice_type ("&str", tem, usize_type));

  lai->set_bool_type (bool_type);
  lai->set_string_char_type (u8_type);
}

/* See language.h.  */

void
rust_language::print_type (struct type *type, const char *varstring,
			   struct ui_file *stream, int show, int level,
			   const struct type_print_options *flags) const
{
  print_offset_data podata (flags);
  rust_internal_print_type (type, varstring, stream, show, level,
			    flags, false, &podata);
}

/* See language.h.  */

void
rust_language::emitchar (int ch, struct type *chtype,
			 struct ui_file *stream, int quoter) const
{
  if (!rust_chartype_p (chtype))
    generic_emit_char (ch, chtype, stream, quoter,
		       target_charset (chtype->arch ()));
  else if (ch == '\\' || ch == quoter)
    gdb_printf (stream, "\\%c", ch);
  else if (ch == '\n')
    gdb_puts ("\\n", stream);
  else if (ch == '\r')
    gdb_puts ("\\r", stream);
  else if (ch == '\t')
    gdb_puts ("\\t", stream);
  else if (ch == '\0')
    gdb_puts ("\\0", stream);
  else if (ch >= 32 && ch <= 127 && isprint (ch))
    gdb_putc (ch, stream);
  else if (ch <= 255)
    gdb_printf (stream, "\\x%02x", ch);
  else
    gdb_printf (stream, "\\u{%06x}", ch);
}

/* See language.h.  */

bool
rust_language::is_string_type_p (struct type *type) const
{
  LONGEST low_bound, high_bound;

  type = check_typedef (type);
  return ((type->code () == TYPE_CODE_STRING)
	  || (type->code () == TYPE_CODE_PTR
	      && (type->target_type ()->code () == TYPE_CODE_ARRAY
		  && rust_u8_type_p (type->target_type ()->target_type ())
		  && get_array_bounds (type->target_type (), &low_bound,
				       &high_bound)))
	  || (type->code () == TYPE_CODE_STRUCT
	      && !rust_enum_p (type)
	      && rust_slice_type_p (type)
	      && strcmp (type->name (), "&str") == 0));
}

/* See language.h.  */

struct block_symbol
rust_language::lookup_symbol_nonlocal
     (const char *name, const struct block *block,
      const domain_enum domain) const
{
  struct block_symbol result = {};

  const char *scope = block == nullptr ? "" : block->scope ();
  symbol_lookup_debug_printf
    ("rust_lookup_symbol_non_local (%s, %s (scope %s), %s)",
     name, host_address_to_string (block), scope,
     domain_name (domain));

  /* Look up bare names in the block's scope.  */
  std::string scopedname;
  if (name[cp_find_first_component (name)] == '\0')
    {
      if (scope[0] != '\0')
	{
	  scopedname = std::string (scope) + "::" + name;
	  name = scopedname.c_str ();
	}
      else
	name = NULL;
    }

  if (name != NULL)
    {
      result = lookup_symbol_in_static_block (name, block, domain);
      if (result.symbol == NULL)
	result = lookup_global_symbol (name, block, domain);
    }
  return result;
}

/* Single instance of the Rust language class.  */

static rust_language rust_language_defn;
