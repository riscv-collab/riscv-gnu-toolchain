/* Support for printing Fortran types for GDB, the GNU debugger.

   Copyright (C) 1986-2024 Free Software Foundation, Inc.

   Contributed by Motorola.  Adapted from the C version by Farooq Butt
   (fmbutt@engage.sps.mot.com).

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
#include "gdbsupport/gdb_obstack.h"
#include "bfd.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "expression.h"
#include "value.h"
#include "gdbcore.h"
#include "target.h"
#include "f-lang.h"
#include "typeprint.h"
#include "cli/cli-style.h"

/* See f-lang.h.  */

void
f_language::print_typedef (struct type *type, struct symbol *new_symbol,
			   struct ui_file *stream) const
{
  type = check_typedef (type);
  print_type (type, "", stream, 0, 0, &type_print_raw_options);
}

/* See f-lang.h.  */

void
f_language::print_type (struct type *type, const char *varstring,
			struct ui_file *stream, int show, int level,
			const struct type_print_options *flags) const
{
  enum type_code code;

  f_type_print_base (type, stream, show, level);
  code = type->code ();
  if ((varstring != NULL && *varstring != '\0')
      /* Need a space if going to print stars or brackets; but not if we
	 will print just a type name.  */
      || ((show > 0
	   || type->name () == 0)
	  && (code == TYPE_CODE_FUNC
	      || code == TYPE_CODE_METHOD
	      || code == TYPE_CODE_ARRAY
	      || ((code == TYPE_CODE_PTR
		   || code == TYPE_CODE_REF)
		  && (type->target_type ()->code () == TYPE_CODE_FUNC
		      || (type->target_type ()->code ()
			  == TYPE_CODE_METHOD)
		      || (type->target_type ()->code ()
			  == TYPE_CODE_ARRAY))))))
    gdb_puts (" ", stream);
  f_type_print_varspec_prefix (type, stream, show, 0);

  if (varstring != NULL)
    {
      int demangled_args;

      gdb_puts (varstring, stream);

      /* For demangled function names, we have the arglist as part of the name,
	 so don't print an additional pair of ()'s.  */

      demangled_args = (*varstring != '\0'
			&& varstring[strlen (varstring) - 1] == ')');
      f_type_print_varspec_suffix (type, stream, show, 0, demangled_args, 0, false);
   }
}

/* See f-lang.h.  */

void
f_language::f_type_print_varspec_prefix (struct type *type,
					 struct ui_file *stream,
					 int show, int passed_a_ptr) const
{
  if (type == 0)
    return;

  if (type->name () && show <= 0)
    return;

  QUIT;

  switch (type->code ())
    {
    case TYPE_CODE_PTR:
      f_type_print_varspec_prefix (type->target_type (), stream, 0, 1);
      break;

    case TYPE_CODE_FUNC:
      f_type_print_varspec_prefix (type->target_type (), stream, 0, 0);
      if (passed_a_ptr)
	gdb_printf (stream, "(");
      break;

    case TYPE_CODE_ARRAY:
      f_type_print_varspec_prefix (type->target_type (), stream, 0, 0);
      break;

    case TYPE_CODE_UNDEF:
    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
    case TYPE_CODE_NAMELIST:
    case TYPE_CODE_ENUM:
    case TYPE_CODE_INT:
    case TYPE_CODE_FLT:
    case TYPE_CODE_VOID:
    case TYPE_CODE_ERROR:
    case TYPE_CODE_CHAR:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_SET:
    case TYPE_CODE_RANGE:
    case TYPE_CODE_STRING:
    case TYPE_CODE_METHOD:
    case TYPE_CODE_REF:
    case TYPE_CODE_COMPLEX:
    case TYPE_CODE_TYPEDEF:
      /* These types need no prefix.  They are listed here so that
	 gcc -Wall will reveal any types that haven't been handled.  */
      break;
    }
}

/* See f-lang.h.  */

void
f_language::f_type_print_varspec_suffix (struct type *type,
					 struct ui_file *stream,
					 int show, int passed_a_ptr,
					 int demangled_args,
					 int arrayprint_recurse_level,
					 bool print_rank_only) const
{
  /* No static variables are permitted as an error call may occur during
     execution of this function.  */

  if (type == 0)
    return;

  if (type->name () && show <= 0)
    return;

  QUIT;

  switch (type->code ())
    {
    case TYPE_CODE_ARRAY:
      arrayprint_recurse_level++;

      if (arrayprint_recurse_level == 1)
	gdb_printf (stream, "(");

      if (type_not_associated (type))
	print_rank_only = true;
      else if (type_not_allocated (type))
	print_rank_only = true;
      else if ((TYPE_ASSOCIATED_PROP (type)
		&& !TYPE_ASSOCIATED_PROP (type)->is_constant ())
	       || (TYPE_ALLOCATED_PROP (type)
		   && !TYPE_ALLOCATED_PROP (type)->is_constant ())
	       || (TYPE_DATA_LOCATION (type)
		   && !TYPE_DATA_LOCATION (type)->is_constant ()))
	{
	  /* This case exist when we ptype a typename which has the dynamic
	     properties but cannot be resolved as there is no object.  */
	  print_rank_only = true;
	}

      if (type->target_type ()->code () == TYPE_CODE_ARRAY)
	f_type_print_varspec_suffix (type->target_type (), stream, 0,
				     0, 0, arrayprint_recurse_level,
				     print_rank_only);

      if (print_rank_only)
	gdb_printf (stream, ":");
      else
	{
	  LONGEST lower_bound = f77_get_lowerbound (type);
	  if (lower_bound != 1)	/* Not the default.  */
	    gdb_printf (stream, "%s:", plongest (lower_bound));

	  /* Make sure that, if we have an assumed size array, we
	       print out a warning and print the upperbound as '*'.  */

	  if (type->bounds ()->high.kind () == PROP_UNDEFINED)
	    gdb_printf (stream, "*");
	  else
	    {
	      LONGEST upper_bound = f77_get_upperbound (type);

	      gdb_puts (plongest (upper_bound), stream);
	    }
	}

      if (type->target_type ()->code () != TYPE_CODE_ARRAY)
	f_type_print_varspec_suffix (type->target_type (), stream, 0,
				     0, 0, arrayprint_recurse_level,
				     print_rank_only);

      if (arrayprint_recurse_level == 1)
	gdb_printf (stream, ")");
      else
	gdb_printf (stream, ",");
      arrayprint_recurse_level--;
      break;

    case TYPE_CODE_PTR:
    case TYPE_CODE_REF:
      f_type_print_varspec_suffix (type->target_type (), stream, 0, 1, 0,
				   arrayprint_recurse_level, false);
      gdb_printf (stream, " )");
      break;

    case TYPE_CODE_FUNC:
      {
	int i, nfields = type->num_fields ();

	f_type_print_varspec_suffix (type->target_type (), stream, 0,
				     passed_a_ptr, 0,
				     arrayprint_recurse_level, false);
	if (passed_a_ptr)
	  gdb_printf (stream, ") ");
	gdb_printf (stream, "(");
	if (nfields == 0 && type->is_prototyped ())
	  print_type (builtin_f_type (type->arch ())->builtin_void,
		      "", stream, -1, 0, 0);
	else
	  for (i = 0; i < nfields; i++)
	    {
	      if (i > 0)
		{
		  gdb_puts (", ", stream);
		  stream->wrap_here (4);
		}
	      print_type (type->field (i).type (), "", stream, -1, 0, 0);
	    }
	gdb_printf (stream, ")");
      }
      break;

    case TYPE_CODE_UNDEF:
    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
    case TYPE_CODE_NAMELIST:
    case TYPE_CODE_ENUM:
    case TYPE_CODE_INT:
    case TYPE_CODE_FLT:
    case TYPE_CODE_VOID:
    case TYPE_CODE_ERROR:
    case TYPE_CODE_CHAR:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_SET:
    case TYPE_CODE_RANGE:
    case TYPE_CODE_STRING:
    case TYPE_CODE_METHOD:
    case TYPE_CODE_COMPLEX:
    case TYPE_CODE_TYPEDEF:
      /* These types do not need a suffix.  They are listed so that
	 gcc -Wall will report types that may not have been considered.  */
      break;
    }
}

/* See f-lang.h.  */

void
f_language::f_type_print_derivation_info (struct type *type,
					  struct ui_file *stream) const
{
  /* Fortran doesn't support multiple inheritance.  */
  const int i = 0;

  if (TYPE_N_BASECLASSES (type) > 0)
    gdb_printf (stream, ", extends(%s) ::", TYPE_BASECLASS (type, i)->name ());
}

/* See f-lang.h.  */

void
f_language::f_type_print_base (struct type *type, struct ui_file *stream,
			       int show, int level) const
{
  int index;

  QUIT;

  stream->wrap_here (4);
  if (type == NULL)
    {
      fputs_styled ("<type unknown>", metadata_style.style (), stream);
      return;
    }

  /* When SHOW is zero or less, and there is a valid type name, then always
     just print the type name directly from the type.  */

  if ((show <= 0) && (type->name () != NULL))
    {
      const char *prefix = "";
      if (type->code () == TYPE_CODE_UNION)
	prefix = "Type, C_Union :: ";
      else if (type->code () == TYPE_CODE_STRUCT
	       || type->code () == TYPE_CODE_NAMELIST)
	prefix = "Type ";
      gdb_printf (stream, "%*s%s%s", level, "", prefix, type->name ());
      return;
    }

  if (type->code () != TYPE_CODE_TYPEDEF)
    type = check_typedef (type);

  switch (type->code ())
    {
    case TYPE_CODE_TYPEDEF:
      f_type_print_base (type->target_type (), stream, 0, level);
      break;

    case TYPE_CODE_ARRAY:
      f_type_print_base (type->target_type (), stream, show, level);
      break;
    case TYPE_CODE_FUNC:
      if (type->target_type () == NULL)
	type_print_unknown_return_type (stream);
      else
	f_type_print_base (type->target_type (), stream, show, level);
      break;

    case TYPE_CODE_PTR:
      gdb_printf (stream, "%*sPTR TO -> ( ", level, "");
      f_type_print_base (type->target_type (), stream, show, 0);
      break;

    case TYPE_CODE_REF:
      gdb_printf (stream, "%*sREF TO -> ( ", level, "");
      f_type_print_base (type->target_type (), stream, show, 0);
      break;

    case TYPE_CODE_VOID:
      {
	struct type *void_type = builtin_f_type (type->arch ())->builtin_void;
	gdb_printf (stream, "%*s%s", level, "", void_type->name ());
      }
      break;

    case TYPE_CODE_UNDEF:
      gdb_printf (stream, "%*sstruct <unknown>", level, "");
      break;

    case TYPE_CODE_ERROR:
      gdb_printf (stream, "%*s%s", level, "", TYPE_ERROR_NAME (type));
      break;

    case TYPE_CODE_RANGE:
      /* This should not occur.  */
      gdb_printf (stream, "%*s<range type>", level, "");
      break;

    case TYPE_CODE_CHAR:
    case TYPE_CODE_INT:
      /* There may be some character types that attempt to come
	 through as TYPE_CODE_INT since dbxstclass.h is so
	 C-oriented, we must change these to "character" from "char".  */

      if (strcmp (type->name (), "char") == 0)
	gdb_printf (stream, "%*scharacter", level, "");
      else
	goto default_case;
      break;

    case TYPE_CODE_STRING:
      /* Strings may have dynamic upperbounds (lengths) like arrays.  We
	 check specifically for the PROP_CONST case to indicate that the
	 dynamic type has been resolved.  If we arrive here having been
	 asked to print the type of a value with a dynamic type then the
	 bounds will not have been resolved.  */

      if (type->bounds ()->high.is_constant ())
	{
	  LONGEST upper_bound = f77_get_upperbound (type);

	  gdb_printf (stream, "character*%s", pulongest (upper_bound));
	}
      else
	gdb_printf (stream, "%*scharacter*(*)", level, "");
      break;

    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
    case TYPE_CODE_NAMELIST:
      if (type->code () == TYPE_CODE_UNION)
	gdb_printf (stream, "%*sType, C_Union ::", level, "");
      else
	gdb_printf (stream, "%*sType", level, "");

      if (show > 0)
	f_type_print_derivation_info (type, stream);

      gdb_puts (" ", stream);

      gdb_puts (type->name (), stream);

      /* According to the definition,
	 we only print structure elements in case show > 0.  */
      if (show > 0)
	{
	  gdb_puts ("\n", stream);
	  for (index = 0; index < type->num_fields (); index++)
	    {
	      f_type_print_base (type->field (index).type (), stream,
				 show - 1, level + 4);
	      gdb_puts (" :: ", stream);
	      fputs_styled (type->field (index).name (),
			    variable_name_style.style (), stream);
	      f_type_print_varspec_suffix (type->field (index).type (),
					   stream, show - 1, 0, 0, 0, false);
	      gdb_puts ("\n", stream);
	    }
	  gdb_printf (stream, "%*sEnd Type ", level, "");
	  gdb_puts (type->name (), stream);
	}
      break;

    case TYPE_CODE_MODULE:
      gdb_printf (stream, "%*smodule %s", level, "", type->name ());
      break;

    default_case:
    default:
      /* Handle types not explicitly handled by the other cases,
	 such as fundamental types.  For these, just print whatever
	 the type name is, as recorded in the type itself.  If there
	 is no type name, then complain.  */
      if (type->name () != NULL)
	gdb_printf (stream, "%*s%s", level, "", type->name ());
      else
	error (_("Invalid type code (%d) in symbol table."), type->code ());
      break;
    }

  if (TYPE_IS_ALLOCATABLE (type))
    gdb_printf (stream, ", allocatable");
}
