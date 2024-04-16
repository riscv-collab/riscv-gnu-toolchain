/* Support for printing Ada types for GDB, the GNU debugger.
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
#include "bfd.h"
#include "gdbtypes.h"
#include "value.h"
#include "c-lang.h"
#include "cli/cli-style.h"
#include "typeprint.h"
#include "target-float.h"
#include "ada-lang.h"
#include <ctype.h>

static int print_selected_record_field_types (struct type *, struct type *,
					      int, int,
					      struct ui_file *, int, int,
					      const struct type_print_options *);

static int print_record_field_types (struct type *, struct type *,
				     struct ui_file *, int, int,
				     const struct type_print_options *);



static char *name_buffer;
static int name_buffer_len;

/* The (decoded) Ada name of TYPE.  This value persists until the
   next call.  */

static char *
decoded_type_name (struct type *type)
{
  if (ada_type_name (type) == NULL)
    return NULL;
  else
    {
      const char *raw_name = ada_type_name (type);
      char *s, *q;

      if (name_buffer == NULL || name_buffer_len <= strlen (raw_name))
	{
	  name_buffer_len = 16 + 2 * strlen (raw_name);
	  name_buffer = (char *) xrealloc (name_buffer, name_buffer_len);
	}
      strcpy (name_buffer, raw_name);

      s = (char *) strstr (name_buffer, "___");
      if (s != NULL)
	*s = '\0';

      s = name_buffer + strlen (name_buffer) - 1;
      while (s > name_buffer && (s[0] != '_' || s[-1] != '_'))
	s -= 1;

      if (s == name_buffer)
	return name_buffer;

      if (!islower (s[1]))
	return NULL;

      for (s = q = name_buffer; *s != '\0'; q += 1)
	{
	  if (s[0] == '_' && s[1] == '_')
	    {
	      *q = '.';
	      s += 2;
	    }
	  else
	    {
	      *q = *s;
	      s += 1;
	    }
	}
      *q = '\0';
      return name_buffer;
    }
}

/* Return nonzero if TYPE is a subrange type, and its bounds
   are identical to the bounds of its subtype.  */

static int
type_is_full_subrange_of_target_type (struct type *type)
{
  struct type *subtype;

  if (type->code () != TYPE_CODE_RANGE)
    return 0;

  subtype = type->target_type ();
  if (subtype == NULL)
    return 0;

  if (is_dynamic_type (type))
    return 0;

  if (ada_discrete_type_low_bound (type)
      != ada_discrete_type_low_bound (subtype))
    return 0;

  if (ada_discrete_type_high_bound (type)
      != ada_discrete_type_high_bound (subtype))
    return 0;

  return 1;
}

/* Print TYPE on STREAM, preferably as a range if BOUNDS_PREFERRED_P
   is nonzero.  */

static void
print_range (struct type *type, struct ui_file *stream,
	     int bounds_preferred_p)
{
  if (!bounds_preferred_p)
    {
      /* Try stripping all TYPE_CODE_RANGE layers whose bounds
	 are identical to the bounds of their subtype.  When
	 the bounds of both types match, it can allow us to
	 print a range using the name of its base type, which
	 is easier to read.  For instance, we would print...

	     array (character) of ...

	 ... instead of...

	     array ('["00"]' .. '["ff"]') of ...  */
      while (type_is_full_subrange_of_target_type (type))
	type = type->target_type ();
    }

  switch (type->code ())
    {
    case TYPE_CODE_RANGE:
    case TYPE_CODE_ENUM:
      {
	LONGEST lo = 0, hi = 0; /* init for gcc -Wall */
	int got_error = 0;

	try
	  {
	    lo = ada_discrete_type_low_bound (type);
	    hi = ada_discrete_type_high_bound (type);
	  }
	catch (const gdb_exception_error &e)
	  {
	    /* This can happen when the range is dynamic.  Sometimes,
	       resolving dynamic property values requires us to have
	       access to an actual object, which is not available
	       when the user is using the "ptype" command on a type.
	       Print the range as an unbounded range.  */
	    gdb_printf (stream, "<>");
	    got_error = 1;
	  }

	if (!got_error)
	  {
	    ada_print_scalar (type, lo, stream);
	    gdb_printf (stream, " .. ");
	    ada_print_scalar (type, hi, stream);
	  }
      }
      break;
    default:
      gdb_printf (stream, "%.*s",
		  ada_name_prefix_len (type->name ()),
		  type->name ());
      break;
    }
}

/* Print the number or discriminant bound at BOUNDS+*N on STREAM, and
   set *N past the bound and its delimiter, if any.  */

static void
print_range_bound (struct type *type, const char *bounds, int *n,
		   struct ui_file *stream)
{
  LONGEST B;

  if (ada_scan_number (bounds, *n, &B, n))
    {
      /* STABS decodes all range types which bounds are 0 .. -1 as
	 unsigned integers (ie. the type code is TYPE_CODE_INT, not
	 TYPE_CODE_RANGE).  Unfortunately, ada_print_scalar() relies
	 on the unsigned flag to determine whether the bound should
	 be printed as a signed or an unsigned value.  This causes
	 the upper bound of the 0 .. -1 range types to be printed as
	 a very large unsigned number instead of -1.
	 To workaround this stabs deficiency, we replace the TYPE by NULL
	 to indicate default output when we detect that the bound is negative,
	 and the type is a TYPE_CODE_INT.  The bound is negative when
	 'm' is the last character of the number scanned in BOUNDS.  */
      if (bounds[*n - 1] == 'm' && type->code () == TYPE_CODE_INT)
	type = NULL;
      ada_print_scalar (type, B, stream);
      if (bounds[*n] == '_')
	*n += 2;
    }
  else
    {
      int bound_len;
      const char *bound = bounds + *n;
      const char *pend;

      pend = strstr (bound, "__");
      if (pend == NULL)
	*n += bound_len = strlen (bound);
      else
	{
	  bound_len = pend - bound;
	  *n += bound_len + 2;
	}
      gdb_printf (stream, "%.*s", bound_len, bound);
    }
}

/* Assuming NAME[0 .. NAME_LEN-1] is the name of a range type, print
   the value (if found) of the bound indicated by SUFFIX ("___L" or
   "___U") according to the ___XD conventions.  */

static void
print_dynamic_range_bound (struct type *type, const char *name, int name_len,
			   const char *suffix, struct ui_file *stream)
{
  LONGEST B;
  std::string name_buf (name, name_len);
  name_buf += suffix;

  if (get_int_var_value (name_buf.c_str (), B))
    ada_print_scalar (type, B, stream);
  else
    gdb_printf (stream, "?");
}

/* Print RAW_TYPE as a range type, using any bound information
   following the GNAT encoding (if available).

   If BOUNDS_PREFERRED_P is nonzero, force the printing of the range
   using its bounds.  Otherwise, try printing the range without
   printing the value of the bounds, if possible (this is only
   considered a hint, not a guaranty).  */

static void
print_range_type (struct type *raw_type, struct ui_file *stream,
		  int bounds_preferred_p)
{
  const char *name;
  struct type *base_type;
  const char *subtype_info;

  gdb_assert (raw_type != NULL);
  name = raw_type->name ();
  gdb_assert (name != NULL);

  if (raw_type->code () == TYPE_CODE_RANGE)
    base_type = raw_type->target_type ();
  else
    base_type = raw_type;

  subtype_info = strstr (name, "___XD");
  if (subtype_info == NULL)
    print_range (raw_type, stream, bounds_preferred_p);
  else
    {
      int prefix_len = subtype_info - name;
      const char *bounds_str;
      int n;

      subtype_info += 5;
      bounds_str = strchr (subtype_info, '_');
      n = 1;

      if (*subtype_info == 'L')
	{
	  print_range_bound (base_type, bounds_str, &n, stream);
	  subtype_info += 1;
	}
      else
	print_dynamic_range_bound (base_type, name, prefix_len, "___L",
				   stream);

      gdb_printf (stream, " .. ");

      if (*subtype_info == 'U')
	print_range_bound (base_type, bounds_str, &n, stream);
      else
	print_dynamic_range_bound (base_type, name, prefix_len, "___U",
				   stream);
    }
}

/* Print enumerated type TYPE on STREAM.  */

static void
print_enum_type (struct type *type, struct ui_file *stream)
{
  int len = type->num_fields ();
  int i;
  LONGEST lastval;

  gdb_printf (stream, "(");
  stream->wrap_here (1);

  lastval = 0;
  for (i = 0; i < len; i++)
    {
      QUIT;
      if (i)
	gdb_printf (stream, ", ");
      stream->wrap_here (4);
      fputs_styled (ada_enum_name (type->field (i).name ()),
		    variable_name_style.style (), stream);
      if (lastval != type->field (i).loc_enumval ())
	{
	  gdb_printf (stream, " => %s",
		      plongest (type->field (i).loc_enumval ()));
	  lastval = type->field (i).loc_enumval ();
	}
      lastval += 1;
    }
  gdb_printf (stream, ")");
}

/* Print simple (constrained) array type TYPE on STREAM.  LEVEL is the
   recursion (indentation) level, in case the element type itself has
   nested structure, and SHOW is the number of levels of internal
   structure to show (see ada_print_type).  */

static void
print_array_type (struct type *type, struct ui_file *stream, int show,
		  int level, const struct type_print_options *flags)
{
  int bitsize;
  int n_indices;
  struct type *elt_type = NULL;

  if (ada_is_constrained_packed_array_type (type))
    type = ada_coerce_to_simple_array_type (type);

  bitsize = 0;
  gdb_printf (stream, "array (");

  if (type == NULL)
    {
      fprintf_styled (stream, metadata_style.style (),
		      _("<undecipherable array type>"));
      return;
    }

  n_indices = -1;
  if (ada_is_simple_array_type (type))
    {
      struct type *range_desc_type;
      struct type *arr_type;

      range_desc_type = ada_find_parallel_type (type, "___XA");
      ada_fixup_array_indexes_type (range_desc_type);

      bitsize = 0;
      if (range_desc_type == NULL)
	{
	  for (arr_type = type; arr_type->code () == TYPE_CODE_ARRAY; )
	    {
	      if (arr_type != type)
		gdb_printf (stream, ", ");
	      print_range (arr_type->index_type (), stream,
			   0 /* bounds_preferred_p */);
	      if (arr_type->field (0).bitsize () > 0)
		bitsize = arr_type->field (0).bitsize ();
	      /* A multi-dimensional array is represented using a
		 sequence of array types.  If one of these types has a
		 name, then it is not another dimension of the outer
		 array, but rather the element type of the outermost
		 array.  */
	      arr_type = arr_type->target_type ();
	      if (arr_type->name () != nullptr)
		break;
	    }
	}
      else
	{
	  int k;

	  n_indices = range_desc_type->num_fields ();
	  for (k = 0, arr_type = type;
	       k < n_indices;
	       k += 1, arr_type = arr_type->target_type ())
	    {
	      if (k > 0)
		gdb_printf (stream, ", ");
	      print_range_type (range_desc_type->field (k).type (),
				stream, 0 /* bounds_preferred_p */);
	      if (arr_type->field (0).bitsize () > 0)
		bitsize = arr_type->field (0).bitsize ();
	    }
	}
    }
  else
    {
      int i, i0;

      for (i = i0 = ada_array_arity (type); i > 0; i -= 1)
	gdb_printf (stream, "%s<>", i == i0 ? "" : ", ");
    }

  elt_type = ada_array_element_type (type, n_indices);
  gdb_printf (stream, ") of ");
  stream->wrap_here (0);
  ada_print_type (elt_type, "", stream, show == 0 ? 0 : show - 1, level + 1,
		  flags);
  /* Arrays with variable-length elements are never bit-packed in practice but
     compilers have to describe their stride so that we can properly fetch
     individual elements.  Do not say the array is packed in this case.  */
  if (bitsize > 0 && !is_dynamic_type (elt_type))
    gdb_printf (stream, " <packed: %d-bit elements>", bitsize);
}

/* Print the choices encoded by field FIELD_NUM of variant-part TYPE on
   STREAM, assuming that VAL_TYPE (if non-NULL) is the type of the
   values.  Return non-zero if the field is an encoding of
   discriminant values, as in a standard variant record, and 0 if the
   field is not so encoded (as happens with single-component variants
   in types annotated with pragma Unchecked_Union).  */

static int
print_choices (struct type *type, int field_num, struct ui_file *stream,
	       struct type *val_type)
{
  int have_output;
  int p;
  const char *name = type->field (field_num).name ();

  have_output = 0;

  /* Skip over leading 'V': NOTE soon to be obsolete.  */
  if (name[0] == 'V')
    {
      if (!ada_scan_number (name, 1, NULL, &p))
	goto Huh;
    }
  else
    p = 0;

  while (1)
    {
      switch (name[p])
	{
	default:
	  goto Huh;
	case '_':
	case '\0':
	  gdb_printf (stream, " =>");
	  return 1;
	case 'S':
	case 'R':
	case 'O':
	  if (have_output)
	    gdb_printf (stream, " | ");
	  have_output = 1;
	  break;
	}

      switch (name[p])
	{
	case 'S':
	  {
	    LONGEST W;

	    if (!ada_scan_number (name, p + 1, &W, &p))
	      goto Huh;
	    ada_print_scalar (val_type, W, stream);
	    break;
	  }
	case 'R':
	  {
	    LONGEST L, U;

	    if (!ada_scan_number (name, p + 1, &L, &p)
		|| name[p] != 'T' || !ada_scan_number (name, p + 1, &U, &p))
	      goto Huh;
	    ada_print_scalar (val_type, L, stream);
	    gdb_printf (stream, " .. ");
	    ada_print_scalar (val_type, U, stream);
	    break;
	  }
	case 'O':
	  gdb_printf (stream, "others");
	  p += 1;
	  break;
	}
    }

Huh:
  gdb_printf (stream, "? =>");
  return 0;
}

/* A helper for print_variant_clauses that prints the members of
   VAR_TYPE.  DISCR_TYPE is the type of the discriminant (or nullptr
   if not available).  The discriminant is contained in OUTER_TYPE.
   STREAM, LEVEL, SHOW, and FLAGS are the same as for
   ada_print_type.  */

static void
print_variant_clauses (struct type *var_type, struct type *discr_type,
		       struct type *outer_type, struct ui_file *stream,
		       int show, int level,
		       const struct type_print_options *flags)
{
  for (int i = 0; i < var_type->num_fields (); i += 1)
    {
      gdb_printf (stream, "\n%*swhen ", level, "");
      if (print_choices (var_type, i, stream, discr_type))
	{
	  if (print_record_field_types (var_type->field (i).type (),
					outer_type, stream, show, level,
					flags)
	      <= 0)
	    gdb_printf (stream, " null;");
	}
      else
	print_selected_record_field_types (var_type, outer_type, i, i,
					   stream, show, level, flags);
    }
}

/* Assuming that field FIELD_NUM of TYPE represents variants whose
   discriminant is contained in OUTER_TYPE, print its components on STREAM.
   LEVEL is the recursion (indentation) level, in case any of the fields
   themselves have nested structure, and SHOW is the number of levels of 
   internal structure to show (see ada_print_type).  For this purpose,
   fields nested in a variant part are taken to be at the same level as
   the fields immediately outside the variant part.  */

static void
print_variant_clauses (struct type *type, int field_num,
		       struct type *outer_type, struct ui_file *stream,
		       int show, int level,
		       const struct type_print_options *flags)
{
  struct type *var_type, *par_type;
  struct type *discr_type;

  var_type = type->field (field_num).type ();
  discr_type = ada_variant_discrim_type (var_type, outer_type);

  if (var_type->code () == TYPE_CODE_PTR)
    {
      var_type = var_type->target_type ();
      if (var_type == NULL || var_type->code () != TYPE_CODE_UNION)
	return;
    }

  par_type = ada_find_parallel_type (var_type, "___XVU");
  if (par_type != NULL)
    var_type = par_type;

  print_variant_clauses (var_type, discr_type, outer_type, stream, show,
			 level + 4, flags);
}

/* Assuming that field FIELD_NUM of TYPE is a variant part whose
   discriminants are contained in OUTER_TYPE, print a description of it
   on STREAM.  LEVEL is the recursion (indentation) level, in case any of
   the fields themselves have nested structure, and SHOW is the number of
   levels of internal structure to show (see ada_print_type).  For this
   purpose, fields nested in a variant part are taken to be at the same
   level as the fields immediately outside the variant part.  */

static void
print_variant_part (struct type *type, int field_num, struct type *outer_type,
		    struct ui_file *stream, int show, int level,
		    const struct type_print_options *flags)
{
  const char *variant
    = ada_variant_discrim_name (type->field (field_num).type ());
  if (*variant == '\0')
    variant = "?";

  gdb_printf (stream, "\n%*scase %s is", level + 4, "", variant);
  print_variant_clauses (type, field_num, outer_type, stream, show,
			 level + 4, flags);
  gdb_printf (stream, "\n%*send case;", level + 4, "");
}

/* Print a description on STREAM of the fields FLD0 through FLD1 in
   record or union type TYPE, whose discriminants are in OUTER_TYPE.
   LEVEL is the recursion (indentation) level, in case any of the
   fields themselves have nested structure, and SHOW is the number of
   levels of internal structure to show (see ada_print_type).  Does
   not print parent type information of TYPE.  Returns 0 if no fields
   printed, -1 for an incomplete type, else > 0.  Prints each field
   beginning on a new line, but does not put a new line at end.  */

static int
print_selected_record_field_types (struct type *type, struct type *outer_type,
				   int fld0, int fld1,
				   struct ui_file *stream, int show, int level,
				   const struct type_print_options *flags)
{
  int i, flds;

  flds = 0;

  if (fld0 > fld1 && type->is_stub ())
    return -1;

  for (i = fld0; i <= fld1; i += 1)
    {
      QUIT;

      if (ada_is_parent_field (type, i) || ada_is_ignored_field (type, i))
	;
      else if (ada_is_wrapper_field (type, i))
	flds += print_record_field_types (type->field (i).type (), type,
					  stream, show, level, flags);
      else if (ada_is_variant_part (type, i))
	{
	  print_variant_part (type, i, outer_type, stream, show, level, flags);
	  flds = 1;
	}
      else
	{
	  flds += 1;
	  gdb_printf (stream, "\n%*s", level + 4, "");
	  ada_print_type (type->field (i).type (),
			  type->field (i).name (),
			  stream, show - 1, level + 4, flags);
	  gdb_printf (stream, ";");
	}
    }

  return flds;
}

static void print_record_field_types_dynamic
  (const gdb::array_view<variant_part> &parts,
   int from, int to, struct type *type, struct ui_file *stream,
   int show, int level, const struct type_print_options *flags);

/* Print the choices encoded by VARIANT on STREAM.  LEVEL is the
   indentation level.  The type of the discriminant for VARIANT is
   given by DISR_TYPE.  */

static void
print_choices (struct type *discr_type, const variant &variant,
	       struct ui_file *stream, int level)
{
  gdb_printf (stream, "\n%*swhen ", level, "");
  if (variant.is_default ())
    gdb_printf (stream, "others");
  else
    {
      bool first = true;
      for (const discriminant_range &range : variant.discriminants)
	{
	  if (!first)
	    gdb_printf (stream, " | ");
	  first = false;

	  ada_print_scalar (discr_type, range.low, stream);
	  if (range.low != range.high)
	    ada_print_scalar (discr_type, range.high, stream);
	}
    }

  gdb_printf (stream, " =>");
}

/* Print a single variant part, PART, on STREAM.  TYPE is the
   enclosing type.  SHOW, LEVEL, and FLAGS are the usual type-printing
   settings.  This prints information about PART and the fields it
   controls.  It returns the index of the next field that should be
   shown -- that is, one after the last field printed by this
   call.  */

static int
print_variant_part (const variant_part &part,
		    struct type *type, struct ui_file *stream,
		    int show, int level,
		    const struct type_print_options *flags)
{
  struct type *discr_type = nullptr;
  const char *name;
  if (part.discriminant_index == -1)
    name = "?";
  else
    {
      name = type->field (part.discriminant_index).name ();;
      discr_type = type->field (part.discriminant_index).type ();
    }

  gdb_printf (stream, "\n%*scase %s is", level + 4, "", name);

  int last_field = -1;
  for (const variant &variant : part.variants)
    {
      print_choices (discr_type, variant, stream, level + 8);

      if (variant.first_field == variant.last_field)
	gdb_printf (stream, " null;");
      else
	{
	  print_record_field_types_dynamic (variant.parts,
					    variant.first_field,
					    variant.last_field, type, stream,
					    show, level + 8, flags);
	  last_field = variant.last_field;
	}
    }

  gdb_printf (stream, "\n%*send case;", level + 4, "");

  return last_field;
}

/* Print some fields of TYPE to STREAM.  SHOW, LEVEL, and FLAGS are
   the usual type-printing settings.  PARTS is the array of variant
   parts that correspond to the range of fields to be printed.  FROM
   and TO are the range of fields to print.  */

static void
print_record_field_types_dynamic (const gdb::array_view<variant_part> &parts,
				  int from, int to,
				  struct type *type, struct ui_file *stream,
				  int show, int level,
				  const struct type_print_options *flags)
{
  int field = from;

  for (const variant_part &part : parts)
    {
      if (part.variants.empty ())
	continue;

      /* Print any non-varying fields.  */
      int first_varying = part.variants[0].first_field;
      print_selected_record_field_types (type, type, field,
					 first_varying - 1, stream,
					 show, level, flags);

      field = print_variant_part (part, type, stream, show, level, flags);
    }

  /* Print any trailing fields that we were asked to print.  */
  print_selected_record_field_types (type, type, field, to - 1, stream, show,
				     level, flags);
}

/* Print a description on STREAM of all fields of record or union type
   TYPE, as for print_selected_record_field_types, above.  */

static int
print_record_field_types (struct type *type, struct type *outer_type,
			  struct ui_file *stream, int show, int level,
			  const struct type_print_options *flags)
{
  struct dynamic_prop *prop = type->dyn_prop (DYN_PROP_VARIANT_PARTS);
  if (prop != nullptr)
    {
      if (prop->kind () == PROP_TYPE)
	{
	  type = prop->original_type ();
	  prop = type->dyn_prop (DYN_PROP_VARIANT_PARTS);
	}
      gdb_assert (prop->kind () == PROP_VARIANT_PARTS);
      print_record_field_types_dynamic (*prop->variant_parts (),
					0, type->num_fields (),
					type, stream, show, level, flags);
      return type->num_fields ();
    }

  return print_selected_record_field_types (type, outer_type,
					    0, type->num_fields () - 1,
					    stream, show, level, flags);
}
   

/* Print record type TYPE on STREAM.  LEVEL is the recursion (indentation)
   level, in case the element type itself has nested structure, and SHOW is
   the number of levels of internal structure to show (see ada_print_type).  */

static void
print_record_type (struct type *type0, struct ui_file *stream, int show,
		   int level, const struct type_print_options *flags)
{
  struct type *parent_type;
  struct type *type;

  type = ada_find_parallel_type (type0, "___XVE");
  if (type == NULL)
    type = type0;

  parent_type = ada_parent_type (type);
  if (ada_type_name (parent_type) != NULL)
    {
      const char *parent_name = decoded_type_name (parent_type);

      /* If we fail to decode the parent type name, then use the parent
	 type name as is.  Not pretty, but should never happen except
	 when the debugging info is incomplete or incorrect.  This
	 prevents a crash trying to print a NULL pointer.  */
      if (parent_name == NULL)
	parent_name = ada_type_name (parent_type);
      gdb_printf (stream, "new %s with record", parent_name);
    }
  else if (parent_type == NULL && ada_is_tagged_type (type, 0))
    gdb_printf (stream, "tagged record");
  else
    gdb_printf (stream, "record");

  if (show < 0)
    gdb_printf (stream, " ... end record");
  else
    {
      int flds;

      flds = 0;
      if (parent_type != NULL && ada_type_name (parent_type) == NULL)
	flds += print_record_field_types (parent_type, parent_type,
					  stream, show, level, flags);
      flds += print_record_field_types (type, type, stream, show, level,
					flags);

      if (flds > 0)
	gdb_printf (stream, "\n%*send record", level, "");
      else if (flds < 0)
	gdb_printf (stream, _(" <incomplete type> end record"));
      else
	gdb_printf (stream, " null; end record");
    }
}

/* Print the unchecked union type TYPE in something resembling Ada
   format on STREAM.  LEVEL is the recursion (indentation) level
   in case the element type itself has nested structure, and SHOW is the
   number of levels of internal structure to show (see ada_print_type).  */
static void
print_unchecked_union_type (struct type *type, struct ui_file *stream,
			    int show, int level,
			    const struct type_print_options *flags)
{
  if (show < 0)
    gdb_printf (stream, "record (?) is ... end record");
  else if (type->num_fields () == 0)
    gdb_printf (stream, "record (?) is null; end record");
  else
    {
      gdb_printf (stream, "record (?) is\n%*scase ? is", level + 4, "");

      print_variant_clauses (type, nullptr, type, stream, show, level + 8, flags);

      gdb_printf (stream, "\n%*send case;\n%*send record",
		  level + 4, "", level, "");
    }
}



/* Print function or procedure type TYPE on STREAM.  Make it a header
   for function or procedure NAME if NAME is not null.  */

static void
print_func_type (struct type *type, struct ui_file *stream, const char *name,
		 const struct type_print_options *flags)
{
  int i, len = type->num_fields ();

  if (type->target_type () != NULL
      && type->target_type ()->code () == TYPE_CODE_VOID)
    gdb_printf (stream, "procedure");
  else
    gdb_printf (stream, "function");

  if (name != NULL && name[0] != '\0')
    {
      gdb_puts (" ", stream);
      fputs_styled (name, function_name_style.style (), stream);
    }

  if (len > 0)
    {
      gdb_printf (stream, " (");
      for (i = 0; i < len; i += 1)
	{
	  if (i > 0)
	    {
	      gdb_puts ("; ", stream);
	      stream->wrap_here (4);
	    }
	  gdb_printf (stream, "a%d: ", i + 1);
	  ada_print_type (type->field (i).type (), "", stream, -1, 0,
			  flags);
	}
      gdb_printf (stream, ")");
    }

  if (type->target_type () == NULL)
    gdb_printf (stream, " return <unknown return type>");
  else if (type->target_type ()->code () != TYPE_CODE_VOID)
    {
      gdb_printf (stream, " return ");
      ada_print_type (type->target_type (), "", stream, 0, 0, flags);
    }
}


/* Print a description of a type TYPE0.
   Output goes to STREAM (via stdio).
   If VARSTRING is a non-NULL, non-empty string, print as an Ada
       variable/field declaration.
   SHOW+1 is the maximum number of levels of internal type structure
      to show (this applies to record types, enumerated types, and
      array types).
   SHOW is the number of levels of internal type structure to show
      when there is a type name for the SHOWth deepest level (0th is
      outer level).
   When SHOW<0, no inner structure is shown.
   LEVEL indicates level of recursion (for nested definitions).  */

void
ada_print_type (struct type *type0, const char *varstring,
		struct ui_file *stream, int show, int level,
		const struct type_print_options *flags)
{
  if (type0->code () == TYPE_CODE_INTERNAL_FUNCTION)
    {
      c_print_type (type0, "", stream, show, level,
		    language_ada, flags);
      return;
    }

  struct type *type = ada_check_typedef (ada_get_base_type (type0));
  /* If we can decode the original type name, use it.  However, there
     are cases where the original type is an internally-generated type
     with a name that can't be decoded (and whose encoded name might
     not actually bear any relation to the type actually declared in
     the sources). In that case, try using the name of the base type
     in its place.

     Note that we looked at the possibility of always using the name
     of the base type. This does not always work, unfortunately, as
     there are situations where it's the base type which has an
     internally-generated name.  */
  const char *type_name = decoded_type_name (type0);
  if (type_name == nullptr)
    type_name = decoded_type_name (type);
  int is_var_decl = (varstring != NULL && varstring[0] != '\0');

  if (type == NULL)
    {
      if (is_var_decl)
	gdb_printf (stream, "%.*s: ",
		    ada_name_prefix_len (varstring), varstring);
      fprintf_styled (stream, metadata_style.style (), "<null type?>");
      return;
    }

  if (is_var_decl && type->code () != TYPE_CODE_FUNC)
    gdb_printf (stream, "%.*s: ",
		ada_name_prefix_len (varstring), varstring);

  if (type_name != NULL && show <= 0 && !ada_is_aligner_type (type))
    {
      gdb_printf (stream, "%.*s",
		  ada_name_prefix_len (type_name), type_name);
      return;
    }

  if (ada_is_aligner_type (type))
    ada_print_type (ada_aligned_type (type), "", stream, show, level, flags);
  else if (ada_is_constrained_packed_array_type (type)
	   && type->code () != TYPE_CODE_PTR)
    print_array_type (type, stream, show, level, flags);
  else
    switch (type->code ())
      {
      default:
	gdb_printf (stream, "<");
	c_print_type (type, "", stream, show, level, language_ada, flags);
	gdb_printf (stream, ">");
	break;
      case TYPE_CODE_PTR:
      case TYPE_CODE_TYPEDEF:
	/* An __XVL field is not truly a pointer, so don't print
	   "access" in this case.  */
	if (type->code () != TYPE_CODE_PTR
	    || (varstring != nullptr
		&& strstr (varstring, "___XVL") == nullptr))
	  gdb_printf (stream, "access ");
	ada_print_type (type->target_type (), "", stream, show, level,
			flags);
	break;
      case TYPE_CODE_REF:
	gdb_printf (stream, "<ref> ");
	ada_print_type (type->target_type (), "", stream, show, level,
			flags);
	break;
      case TYPE_CODE_ARRAY:
	print_array_type (type, stream, show, level, flags);
	break;
      case TYPE_CODE_BOOL:
	gdb_printf (stream, "(false, true)");
	break;
      case TYPE_CODE_INT:
	{
	  const char *name = ada_type_name (type);

	  if (!ada_is_range_type_name (name))
	    fprintf_styled (stream, metadata_style.style (),
			    _("<%s-byte integer>"),
			    pulongest (type->length ()));
	  else
	    {
	      gdb_printf (stream, "range ");
	      print_range_type (type, stream, 1 /* bounds_preferred_p */);
	    }
	}
	break;
      case TYPE_CODE_RANGE:
	if (is_fixed_point_type (type))
	  {
	    gdb_printf (stream, "<");
	    print_type_fixed_point (type, stream);
	    gdb_printf (stream, ">");
	  }
	else if (ada_is_modular_type (type))
	  gdb_printf (stream, "mod %s", 
		      int_string (ada_modulus (type), 10, 0, 0, 1));
	else
	  {
	    gdb_printf (stream, "range ");
	    print_range (type, stream, 1 /* bounds_preferred_p */);
	  }
	break;
      case TYPE_CODE_FLT:
	fprintf_styled (stream, metadata_style.style (),
			_("<%s-byte float>"),
			pulongest (type->length ()));
	break;
      case TYPE_CODE_ENUM:
	if (show < 0)
	  gdb_printf (stream, "(...)");
	else
	  print_enum_type (type, stream);
	break;
      case TYPE_CODE_STRUCT:
	if (ada_is_array_descriptor_type (type))
	  print_array_type (type, stream, show, level, flags);
	else
	  print_record_type (type, stream, show, level, flags);
	break;
      case TYPE_CODE_UNION:
	print_unchecked_union_type (type, stream, show, level, flags);
	break;
      case TYPE_CODE_FUNC:
	print_func_type (type, stream, varstring, flags);
	break;
      }
}

/* Implement the la_print_typedef language method for Ada.  */

void
ada_print_typedef (struct type *type, struct symbol *new_symbol,
		   struct ui_file *stream)
{
  type = ada_check_typedef (type);
  ada_print_type (type, "", stream, 0, 0, &type_print_raw_options);
}
