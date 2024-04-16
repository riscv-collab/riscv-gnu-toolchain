/* Support for printing Modula 2 types for GDB, the GNU debugger.
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
#include "language.h"
#include "gdbsupport/gdb_obstack.h"
#include "bfd.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "expression.h"
#include "value.h"
#include "gdbcore.h"
#include "m2-lang.h"
#include "target.h"
#include "language.h"
#include "demangle.h"
#include "c-lang.h"
#include "typeprint.h"
#include "cp-abi.h"
#include "cli/cli-style.h"

static void m2_print_bounds (struct type *type,
			     struct ui_file *stream, int show, int level,
			     int print_high);

static void m2_typedef (struct type *, struct ui_file *, int, int,
			const struct type_print_options *);
static void m2_array (struct type *, struct ui_file *, int, int,
		      const struct type_print_options *);
static void m2_pointer (struct type *, struct ui_file *, int, int,
			const struct type_print_options *);
static void m2_ref (struct type *, struct ui_file *, int, int,
		    const struct type_print_options *);
static void m2_procedure (struct type *, struct ui_file *, int, int,
			  const struct type_print_options *);
static void m2_union (struct type *, struct ui_file *);
static void m2_enum (struct type *, struct ui_file *, int, int);
static void m2_range (struct type *, struct ui_file *, int, int,
		      const struct type_print_options *);
static void m2_type_name (struct type *type, struct ui_file *stream);
static void m2_short_set (struct type *type, struct ui_file *stream,
			  int show, int level);
static int m2_long_set (struct type *type, struct ui_file *stream,
			int show, int level, const struct type_print_options *flags);
static int m2_unbounded_array (struct type *type, struct ui_file *stream,
			       int show, int level,
			       const struct type_print_options *flags);
static void m2_record_fields (struct type *type, struct ui_file *stream,
			      int show, int level, const struct type_print_options *flags);
static void m2_unknown (const char *s, struct type *type,
			struct ui_file *stream, int show, int level);

int m2_is_long_set (struct type *type);
int m2_is_long_set_of_type (struct type *type, struct type **of_type);
int m2_is_unbounded_array (struct type *type);


void
m2_print_type (struct type *type, const char *varstring,
	       struct ui_file *stream,
	       int show, int level,
	       const struct type_print_options *flags)
{
  type = check_typedef (type);

  QUIT;

  stream->wrap_here (4);
  if (type == NULL)
    {
      fputs_styled (_("<type unknown>"), metadata_style.style (), stream);
      return;
    }

  switch (type->code ())
    {
    case TYPE_CODE_SET:
      m2_short_set(type, stream, show, level);
      break;

    case TYPE_CODE_STRUCT:
      if (m2_long_set (type, stream, show, level, flags)
	  || m2_unbounded_array (type, stream, show, level, flags))
	break;
      m2_record_fields (type, stream, show, level, flags);
      break;

    case TYPE_CODE_TYPEDEF:
      m2_typedef (type, stream, show, level, flags);
      break;

    case TYPE_CODE_ARRAY:
      m2_array (type, stream, show, level, flags);
      break;

    case TYPE_CODE_PTR:
      m2_pointer (type, stream, show, level, flags);
      break;

    case TYPE_CODE_REF:
      m2_ref (type, stream, show, level, flags);
      break;

    case TYPE_CODE_METHOD:
      m2_unknown (_("method"), type, stream, show, level);
      break;

    case TYPE_CODE_FUNC:
      m2_procedure (type, stream, show, level, flags);
      break;

    case TYPE_CODE_UNION:
      m2_union (type, stream);
      break;

    case TYPE_CODE_ENUM:
      m2_enum (type, stream, show, level);
      break;

    case TYPE_CODE_VOID:
      break;

    case TYPE_CODE_UNDEF:
      /* i18n: Do not translate the "struct" part!  */
      m2_unknown (_("undef"), type, stream, show, level);
      break;

    case TYPE_CODE_ERROR:
      m2_unknown (_("error"), type, stream, show, level);
      break;

    case TYPE_CODE_RANGE:
      m2_range (type, stream, show, level, flags);
      break;

    default:
      m2_type_name (type, stream);
      break;
    }
}

/* Print a typedef using M2 syntax.  TYPE is the underlying type.
   NEW_SYMBOL is the symbol naming the type.  STREAM is the stream on
   which to print.  */

void
m2_language::print_typedef (struct type *type, struct symbol *new_symbol,
			    struct ui_file *stream) const
{
  type = check_typedef (type);
  gdb_printf (stream, "TYPE ");
  if (!new_symbol->type ()->name ()
      || strcmp ((new_symbol->type ())->name (),
		 new_symbol->linkage_name ()) != 0)
    gdb_printf (stream, "%s = ", new_symbol->print_name ());
  else
    gdb_printf (stream, "<builtin> = ");
  type_print (type, "", stream, 0);
  gdb_printf (stream, ";");
}

/* m2_type_name - if a, type, has a name then print it.  */

void
m2_type_name (struct type *type, struct ui_file *stream)
{
  if (type->name () != NULL)
    gdb_puts (type->name (), stream);
}

/* m2_range - displays a Modula-2 subrange type.  */

void
m2_range (struct type *type, struct ui_file *stream, int show,
	  int level, const struct type_print_options *flags)
{
  if (type->bounds ()->high.const_val () == type->bounds ()->low.const_val ())
    {
      /* FIXME: type::target_type used to be TYPE_DOMAIN_TYPE but that was
	 wrong.  Not sure if type::target_type is correct though.  */
      m2_print_type (type->target_type (), "", stream, show, level,
		     flags);
    }
  else
    {
      struct type *target = type->target_type ();

      gdb_printf (stream, "[");
      print_type_scalar (target, type->bounds ()->low.const_val (), stream);
      gdb_printf (stream, "..");
      print_type_scalar (target, type->bounds ()->high.const_val (), stream);
      gdb_printf (stream, "]");
    }
}

static void
m2_typedef (struct type *type, struct ui_file *stream, int show,
	    int level, const struct type_print_options *flags)
{
  if (type->name () != NULL)
    {
      gdb_puts (type->name (), stream);
      gdb_puts (" = ", stream);
    }
  m2_print_type (type->target_type (), "", stream, show, level, flags);
}

/* m2_array - prints out a Modula-2 ARRAY ... OF type.  */

static void m2_array (struct type *type, struct ui_file *stream,
		      int show, int level, const struct type_print_options *flags)
{
  gdb_printf (stream, "ARRAY [");
  if (type->target_type ()->length () > 0
      && type->bounds ()->high.is_constant ())
    {
      if (type->index_type () != 0)
	{
	  m2_print_bounds (type->index_type (), stream, show, -1, 0);
	  gdb_printf (stream, "..");
	  m2_print_bounds (type->index_type (), stream, show, -1, 1);
	}
      else
	gdb_puts (pulongest ((type->length ()
			     / type->target_type ()->length ())),
		  stream);
    }
  gdb_printf (stream, "] OF ");
  m2_print_type (type->target_type (), "", stream, show, level, flags);
}

static void
m2_pointer (struct type *type, struct ui_file *stream, int show,
	    int level, const struct type_print_options *flags)
{
  if (TYPE_CONST (type))
    gdb_printf (stream, "[...] : ");
  else
    gdb_printf (stream, "POINTER TO ");

  m2_print_type (type->target_type (), "", stream, show, level, flags);
}

static void
m2_ref (struct type *type, struct ui_file *stream, int show,
	int level, const struct type_print_options *flags)
{
  gdb_printf (stream, "VAR");
  m2_print_type (type->target_type (), "", stream, show, level, flags);
}

static void
m2_unknown (const char *s, struct type *type, struct ui_file *stream,
	    int show, int level)
{
  gdb_printf (stream, "%s %s", s, _("is unknown"));
}

static void m2_union (struct type *type, struct ui_file *stream)
{
  gdb_printf (stream, "union");
}

static void
m2_procedure (struct type *type, struct ui_file *stream,
	      int show, int level, const struct type_print_options *flags)
{
  gdb_printf (stream, "PROCEDURE ");
  m2_type_name (type, stream);
  if (type->target_type () == NULL
      || type->target_type ()->code () != TYPE_CODE_VOID)
    {
      int i, len = type->num_fields ();

      gdb_printf (stream, " (");
      for (i = 0; i < len; i++)
	{
	  if (i > 0)
	    {
	      gdb_puts (", ", stream);
	      stream->wrap_here (4);
	    }
	  m2_print_type (type->field (i).type (), "", stream, -1, 0, flags);
	}
      gdb_printf (stream, ") : ");
      if (type->target_type () != NULL)
	m2_print_type (type->target_type (), "", stream, 0, 0, flags);
      else
	type_print_unknown_return_type (stream);
    }
}

static void
m2_print_bounds (struct type *type,
		 struct ui_file *stream, int show, int level,
		 int print_high)
{
  struct type *target = type->target_type ();

  if (type->num_fields () == 0)
    return;

  if (print_high)
    print_type_scalar (target, type->bounds ()->high.const_val (), stream);
  else
    print_type_scalar (target, type->bounds ()->low.const_val (), stream);
}

static void
m2_short_set (struct type *type, struct ui_file *stream, int show, int level)
{
  gdb_printf(stream, "SET [");
  m2_print_bounds (type->index_type (), stream,
		   show - 1, level, 0);

  gdb_printf(stream, "..");
  m2_print_bounds (type->index_type (), stream,
		   show - 1, level, 1);
  gdb_printf(stream, "]");
}

int
m2_is_long_set (struct type *type)
{
  LONGEST previous_high = 0;  /* Unnecessary initialization
				 keeps gcc -Wall happy.  */
  int len, i;
  struct type *range;

  if (type->code () == TYPE_CODE_STRUCT)
    {

      /* check if all fields of the RECORD are consecutive sets.  */

      len = type->num_fields ();
      for (i = TYPE_N_BASECLASSES (type); i < len; i++)
	{
	  if (type->field (i).type () == NULL)
	    return 0;
	  if (type->field (i).type ()->code () != TYPE_CODE_SET)
	    return 0;
	  if (type->field (i).name () != NULL
	      && (strcmp (type->field (i).name (), "") != 0))
	    return 0;
	  range = type->field (i).type ()->index_type ();
	  if ((i > TYPE_N_BASECLASSES (type))
	      && previous_high + 1 != range->bounds ()->low.const_val ())
	    return 0;
	  previous_high = range->bounds ()->high.const_val ();
	}
      return len>0;
    }
  return 0;
}

/* m2_get_discrete_bounds - a wrapper for get_discrete_bounds which
			    understands that CHARs might be signed.
			    This should be integrated into gdbtypes.c
			    inside get_discrete_bounds.  */

static bool
m2_get_discrete_bounds (struct type *type, LONGEST *lowp, LONGEST *highp)
{
  type = check_typedef (type);
  switch (type->code ())
    {
    case TYPE_CODE_CHAR:
      if (type->length () < sizeof (LONGEST))
	{
	  if (!type->is_unsigned ())
	    {
	      *lowp = -(1 << (type->length () * TARGET_CHAR_BIT - 1));
	      *highp = -*lowp - 1;
	      return 0;
	    }
	}
      [[fallthrough]];
    default:
      return get_discrete_bounds (type, lowp, highp);
    }
}

/* m2_is_long_set_of_type - returns TRUE if the long set was declared as
			    SET OF <oftype> of_type is assigned to the
			    subtype.  */

int
m2_is_long_set_of_type (struct type *type, struct type **of_type)
{
  int len, i;
  struct type *range;
  struct type *target;
  LONGEST l1, l2;
  LONGEST h1, h2;

  if (type->code () == TYPE_CODE_STRUCT)
    {
      len = type->num_fields ();
      i = TYPE_N_BASECLASSES (type);
      if (len == 0)
	return 0;
      range = type->field (i).type ()->index_type ();
      target = range->target_type ();

      l1 = type->field (i).type ()->bounds ()->low.const_val ();
      h1 = type->field (len - 1).type ()->bounds ()->high.const_val ();
      *of_type = target;
      if (m2_get_discrete_bounds (target, &l2, &h2))
	return (l1 == l2 && h1 == h2);
      error (_("long_set failed to find discrete bounds for its subtype"));
      return 0;
    }
  error (_("expecting long_set"));
  return 0;
}

static int
m2_long_set (struct type *type, struct ui_file *stream, int show, int level,
	     const struct type_print_options *flags)
{
  struct type *of_type;
  int i;
  int len = type->num_fields ();
  LONGEST low;
  LONGEST high;

  if (m2_is_long_set (type))
    {
      if (type->name () != NULL)
	{
	  gdb_puts (type->name (), stream);
	  if (show == 0)
	    return 1;
	  gdb_puts (" = ", stream);
	}

      if (get_long_set_bounds (type, &low, &high))
	{
	  gdb_printf(stream, "SET OF ");
	  i = TYPE_N_BASECLASSES (type);
	  if (m2_is_long_set_of_type (type, &of_type))
	    m2_print_type (of_type, "", stream, show - 1, level, flags);
	  else
	    {
	      gdb_printf(stream, "[");
	      m2_print_bounds (type->field (i).type ()->index_type (),
			       stream, show - 1, level, 0);

	      gdb_printf(stream, "..");

	      m2_print_bounds (type->field (len - 1).type ()->index_type (),
			       stream, show - 1, level, 1);
	      gdb_printf(stream, "]");
	    }
	}
      else
	/* i18n: Do not translate the "SET OF" part!  */
	gdb_printf(stream, _("SET OF <unknown>"));

      return 1;
    }
  return 0;
}

/* m2_is_unbounded_array - returns TRUE if, type, should be regarded
			   as a Modula-2 unbounded ARRAY type.  */

int
m2_is_unbounded_array (struct type *type)
{
  if (type->code () == TYPE_CODE_STRUCT)
    {
      /*
       *  check if we have a structure with exactly two fields named
       *  _m2_contents and _m2_high.  It also checks to see if the
       *  type of _m2_contents is a pointer.  The type::target_type
       *  of the pointer determines the unbounded ARRAY OF type.
       */
      if (type->num_fields () != 2)
	return 0;
      if (strcmp (type->field (0).name (), "_m2_contents") != 0)
	return 0;
      if (strcmp (type->field (1).name (), "_m2_high") != 0)
	return 0;
      if (type->field (0).type ()->code () != TYPE_CODE_PTR)
	return 0;
      return 1;
    }
  return 0;
}

/* m2_unbounded_array - if the struct type matches a Modula-2 unbounded
			parameter type then display the type as an
			ARRAY OF type.  Returns TRUE if an unbounded
			array type was detected.  */

static int
m2_unbounded_array (struct type *type, struct ui_file *stream, int show,
		    int level, const struct type_print_options *flags)
{
  if (m2_is_unbounded_array (type))
    {
      if (show > 0)
	{
	  gdb_puts ("ARRAY OF ", stream);
	  m2_print_type (type->field (0).type ()->target_type (),
			 "", stream, 0, level, flags);
	}
      return 1;
    }
  return 0;
}

void
m2_record_fields (struct type *type, struct ui_file *stream, int show,
		  int level, const struct type_print_options *flags)
{
  /* Print the tag if it exists.  */
  if (type->name () != NULL)
    {
      if (!startswith (type->name (), "$$"))
	{
	  gdb_puts (type->name (), stream);
	  if (show > 0)
	    gdb_printf (stream, " = ");
	}
    }
  stream->wrap_here (4);
  if (show < 0)
    {
      if (type->code () == TYPE_CODE_STRUCT)
	gdb_printf (stream, "RECORD ... END ");
      else if (type->code () == TYPE_CODE_UNION)
	gdb_printf (stream, "CASE ... END ");
    }
  else if (show > 0)
    {
      int i;
      int len = type->num_fields ();

      if (type->code () == TYPE_CODE_STRUCT)
	gdb_printf (stream, "RECORD\n");
      else if (type->code () == TYPE_CODE_UNION)
	/* i18n: Do not translate "CASE" and "OF".  */
	gdb_printf (stream, _("CASE <variant> OF\n"));

      for (i = TYPE_N_BASECLASSES (type); i < len; i++)
	{
	  QUIT;

	  print_spaces (level + 4, stream);
	  fputs_styled (type->field (i).name (),
			variable_name_style.style (), stream);
	  gdb_puts (" : ", stream);
	  m2_print_type (type->field (i).type (),
			 "",
			 stream, 0, level + 4, flags);
	  if (type->field (i).is_packed ())
	    {
	      /* It is a bitfield.  This code does not attempt
		 to look at the bitpos and reconstruct filler,
		 unnamed fields.  This would lead to misleading
		 results if the compiler does not put out fields
		 for such things (I don't know what it does).  */
	      gdb_printf (stream, " : %d", type->field (i).bitsize ());
	    }
	  gdb_printf (stream, ";\n");
	}
      
      gdb_printf (stream, "%*sEND ", level, "");
    }
}

void
m2_enum (struct type *type, struct ui_file *stream, int show, int level)
{
  LONGEST lastval;
  int i, len;

  if (show < 0)
    {
      /* If we just printed a tag name, no need to print anything else.  */
      if (type->name () == NULL)
	gdb_printf (stream, "(...)");
    }
  else if (show > 0 || type->name () == NULL)
    {
      gdb_printf (stream, "(");
      len = type->num_fields ();
      lastval = 0;
      for (i = 0; i < len; i++)
	{
	  QUIT;
	  if (i > 0)
	    gdb_printf (stream, ", ");
	  stream->wrap_here (4);
	  fputs_styled (type->field (i).name (),
			variable_name_style.style (), stream);
	  if (lastval != type->field (i).loc_enumval ())
	    {
	      gdb_printf (stream, " = %s",
			  plongest (type->field (i).loc_enumval ()));
	      lastval = type->field (i).loc_enumval ();
	    }
	  lastval++;
	}
      gdb_printf (stream, ")");
    }
}
