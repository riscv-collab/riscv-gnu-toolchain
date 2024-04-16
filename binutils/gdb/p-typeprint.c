/* Support for printing Pascal types for GDB, the GNU debugger.
   Copyright (C) 2000-2024 Free Software Foundation, Inc.

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

/* This file is derived from p-typeprint.c */

#include "defs.h"
#include "gdbsupport/gdb_obstack.h"
#include "bfd.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "expression.h"
#include "value.h"
#include "gdbcore.h"
#include "target.h"
#include "language.h"
#include "p-lang.h"
#include "typeprint.h"
#include "gdb-demangle.h"
#include <ctype.h>
#include "cli/cli-style.h"

/* See language.h.  */

void
pascal_language::print_type (struct type *type, const char *varstring,
			     struct ui_file *stream, int show, int level,
			     const struct type_print_options *flags) const
{
  enum type_code code;
  int demangled_args;

  code = type->code ();

  if (show > 0)
    type = check_typedef (type);

  if ((code == TYPE_CODE_FUNC
       || code == TYPE_CODE_METHOD))
    {
      type_print_varspec_prefix (type, stream, show, 0, flags);
    }
  /* first the name */
  gdb_puts (varstring, stream);

  if ((varstring != NULL && *varstring != '\0')
      && !(code == TYPE_CODE_FUNC
	   || code == TYPE_CODE_METHOD))
    {
      gdb_puts (" : ", stream);
    }

  if (!(code == TYPE_CODE_FUNC
	|| code == TYPE_CODE_METHOD))
    {
      type_print_varspec_prefix (type, stream, show, 0, flags);
    }

  type_print_base (type, stream, show, level, flags);
  /* For demangled function names, we have the arglist as part of the name,
     so don't print an additional pair of ()'s.  */

  demangled_args = varstring ? strchr (varstring, '(') != NULL : 0;
  type_print_varspec_suffix (type, stream, show, 0, demangled_args,
				    flags);

}

/* See language.h.  */

void
pascal_language::print_typedef (struct type *type, struct symbol *new_symbol,
				struct ui_file *stream) const
{
  type = check_typedef (type);
  gdb_printf (stream, "type ");
  gdb_printf (stream, "%s = ", new_symbol->print_name ());
  type_print (type, "", stream, 0);
  gdb_printf (stream, ";");
}

/* See p-lang.h.  */

void
pascal_language::type_print_derivation_info (struct ui_file *stream,
					     struct type *type) const
{
  const char *name;
  int i;

  for (i = 0; i < TYPE_N_BASECLASSES (type); i++)
    {
      gdb_puts (i == 0 ? ": " : ", ", stream);
      gdb_printf (stream, "%s%s ",
		  BASETYPE_VIA_PUBLIC (type, i) ? "public" : "private",
		  BASETYPE_VIA_VIRTUAL (type, i) ? " virtual" : "");
      name = TYPE_BASECLASS (type, i)->name ();
      gdb_printf (stream, "%s", name ? name : "(null)");
    }
  if (i > 0)
    {
      gdb_puts (" ", stream);
    }
}

/* See p-lang.h.  */

void
pascal_language::type_print_method_args (const char *physname,
					 const char *methodname,
					 struct ui_file *stream) const
{
  int is_constructor = (startswith (physname, "__ct__"));
  int is_destructor = (startswith (physname, "__dt__"));

  if (is_constructor || is_destructor)
    {
      physname += 6;
    }

  gdb_puts (methodname, stream);

  if (physname && (*physname != 0))
    {
      gdb_puts (" (", stream);
      /* We must demangle this.  */
      while (isdigit (physname[0]))
	{
	  int len = 0;
	  int i, j;
	  char *argname;

	  while (isdigit (physname[len]))
	    {
	      len++;
	    }
	  i = strtol (physname, &argname, 0);
	  physname += len;

	  for (j = 0; j < i; ++j)
	    gdb_putc (physname[j], stream);

	  physname += i;
	  if (physname[0] != 0)
	    {
	      gdb_puts (", ", stream);
	    }
	}
      gdb_puts (")", stream);
    }
}

/* See p-lang.h.  */

void
pascal_language::type_print_varspec_prefix (struct type *type,
					    struct ui_file *stream,
					    int show, int passed_a_ptr,
					    const struct type_print_options *flags) const
{
  if (type == 0)
    return;

  if (type->name () && show <= 0)
    return;

  QUIT;

  switch (type->code ())
    {
    case TYPE_CODE_PTR:
      gdb_printf (stream, "^");
      type_print_varspec_prefix (type->target_type (), stream, 0, 1,
					flags);
      break;			/* Pointer should be handled normally
				   in pascal.  */

    case TYPE_CODE_METHOD:
      if (passed_a_ptr)
	gdb_printf (stream, "(");
      if (type->target_type () != NULL
	  && type->target_type ()->code () != TYPE_CODE_VOID)
	{
	  gdb_printf (stream, "function  ");
	}
      else
	{
	  gdb_printf (stream, "procedure ");
	}

      if (passed_a_ptr)
	{
	  gdb_printf (stream, " ");
	  type_print_base (TYPE_SELF_TYPE (type),
				  stream, 0, passed_a_ptr, flags);
	  gdb_printf (stream, "::");
	}
      break;

    case TYPE_CODE_REF:
      type_print_varspec_prefix (type->target_type (), stream, 0, 1,
				 flags);
      gdb_printf (stream, "&");
      break;

    case TYPE_CODE_FUNC:
      if (passed_a_ptr)
	gdb_printf (stream, "(");

      if (type->target_type () != NULL
	  && type->target_type ()->code () != TYPE_CODE_VOID)
	{
	  gdb_printf (stream, "function  ");
	}
      else
	{
	  gdb_printf (stream, "procedure ");
	}

      break;

    case TYPE_CODE_ARRAY:
      if (passed_a_ptr)
	gdb_printf (stream, "(");
      gdb_printf (stream, "array ");
      if (type->target_type ()->length () > 0
	  && type->bounds ()->high.is_constant ())
	gdb_printf (stream, "[%s..%s] ",
		    plongest (type->bounds ()->low.const_val ()),
		    plongest (type->bounds ()->high.const_val ()));
      gdb_printf (stream, "of ");
      break;
    }
}

/* See p-lang.h.  */

void
pascal_language::print_func_args (struct type *type, struct ui_file *stream,
				  const struct type_print_options *flags) const
{
  int i, len = type->num_fields ();

  if (len)
    {
      gdb_printf (stream, "(");
    }
  for (i = 0; i < len; i++)
    {
      if (i > 0)
	{
	  gdb_puts (", ", stream);
	  stream->wrap_here (4);
	}
      /*  Can we find if it is a var parameter ??
	  if ( TYPE_FIELD(type, i) == )
	  {
	  gdb_printf (stream, "var ");
	  } */
      print_type (type->field (i).type (), ""	/* TYPE_FIELD_NAME
						   seems invalid!  */
			 ,stream, -1, 0, flags);
    }
  if (len)
    {
      gdb_printf (stream, ")");
    }
}

/* See p-lang.h.  */

void
pascal_language::type_print_func_varspec_suffix  (struct type *type,
						  struct ui_file *stream,
						  int show, int passed_a_ptr,
						  int demangled_args,
						  const struct type_print_options *flags) const
{
  if (type->target_type () == NULL
      || type->target_type ()->code () != TYPE_CODE_VOID)
    {
      gdb_printf (stream, " : ");
      type_print_varspec_prefix (type->target_type (),
					stream, 0, 0, flags);

      if (type->target_type () == NULL)
	type_print_unknown_return_type (stream);
      else
	type_print_base (type->target_type (), stream, show, 0,
				flags);

      type_print_varspec_suffix (type->target_type (), stream, 0,
				 passed_a_ptr, 0, flags);
    }
}

/* See p-lang.h.  */

void
pascal_language::type_print_varspec_suffix (struct type *type,
					    struct ui_file *stream,
					    int show, int passed_a_ptr,
					    int demangled_args,
					    const struct type_print_options *flags) const
{
  if (type == 0)
    return;

  if (type->name () && show <= 0)
    return;

  QUIT;

  switch (type->code ())
    {
    case TYPE_CODE_ARRAY:
      if (passed_a_ptr)
	gdb_printf (stream, ")");
      break;

    case TYPE_CODE_METHOD:
      if (passed_a_ptr)
	gdb_printf (stream, ")");
      type_print_method_args ("", "", stream);
      type_print_func_varspec_suffix (type, stream, show,
					     passed_a_ptr, 0, flags);
      break;

    case TYPE_CODE_PTR:
    case TYPE_CODE_REF:
      type_print_varspec_suffix (type->target_type (),
				 stream, 0, 1, 0, flags);
      break;

    case TYPE_CODE_FUNC:
      if (passed_a_ptr)
	gdb_printf (stream, ")");
      if (!demangled_args)
	print_func_args (type, stream, flags);
      type_print_func_varspec_suffix (type, stream, show,
					     passed_a_ptr, 0, flags);
      break;
    }
}

/* See p-lang.h.  */

void
pascal_language::type_print_base (struct type *type, struct ui_file *stream, int show,
				  int level, const struct type_print_options *flags) const
{
  int i;
  int len;
  LONGEST lastval;
  enum
    {
      s_none, s_public, s_private, s_protected
    }
  section_type;

  QUIT;
  stream->wrap_here (4);
  if (type == NULL)
    {
      fputs_styled ("<type unknown>", metadata_style.style (), stream);
      return;
    }

  /* void pointer */
  if ((type->code () == TYPE_CODE_PTR)
      && (type->target_type ()->code () == TYPE_CODE_VOID))
    {
      gdb_puts (type->name () ? type->name () : "pointer",
		stream);
      return;
    }
  /* When SHOW is zero or less, and there is a valid type name, then always
     just print the type name directly from the type.  */

  if (show <= 0
      && type->name () != NULL)
    {
      gdb_puts (type->name (), stream);
      return;
    }

  type = check_typedef (type);

  switch (type->code ())
    {
    case TYPE_CODE_TYPEDEF:
    case TYPE_CODE_PTR:
    case TYPE_CODE_REF:
      type_print_base (type->target_type (), stream, show, level,
		       flags);
      break;

    case TYPE_CODE_ARRAY:
      print_type (type->target_type (), NULL, stream, 0, 0, flags);
      break;

    case TYPE_CODE_FUNC:
    case TYPE_CODE_METHOD:
      break;
    case TYPE_CODE_STRUCT:
      if (type->name () != NULL)
	{
	  gdb_puts (type->name (), stream);
	  gdb_puts (" = ", stream);
	}
      if (HAVE_CPLUS_STRUCT (type))
	{
	  gdb_printf (stream, "class ");
	}
      else
	{
	  gdb_printf (stream, "record ");
	}
      goto struct_union;

    case TYPE_CODE_UNION:
      if (type->name () != NULL)
	{
	  gdb_puts (type->name (), stream);
	  gdb_puts (" = ", stream);
	}
      gdb_printf (stream, "case <?> of ");

    struct_union:
      stream->wrap_here (4);
      if (show < 0)
	{
	  /* If we just printed a tag name, no need to print anything else.  */
	  if (type->name () == NULL)
	    gdb_printf (stream, "{...}");
	}
      else if (show > 0 || type->name () == NULL)
	{
	  type_print_derivation_info (stream, type);

	  gdb_printf (stream, "\n");
	  if ((type->num_fields () == 0) && (TYPE_NFN_FIELDS (type) == 0))
	    {
	      if (type->is_stub ())
		gdb_printf (stream, "%*s<incomplete type>\n",
			    level + 4, "");
	      else
		gdb_printf (stream, "%*s<no data fields>\n",
			    level + 4, "");
	    }

	  /* Start off with no specific section type, so we can print
	     one for the first field we find, and use that section type
	     thereafter until we find another type.  */

	  section_type = s_none;

	  /* If there is a base class for this type,
	     do not print the field that it occupies.  */

	  len = type->num_fields ();
	  for (i = TYPE_N_BASECLASSES (type); i < len; i++)
	    {
	      QUIT;
	      /* Don't print out virtual function table.  */
	      if ((startswith (type->field (i).name (), "_vptr"))
		  && is_cplus_marker ((type->field (i).name ())[5]))
		continue;

	      /* If this is a pascal object or class we can print the
		 various section labels.  */

	      if (HAVE_CPLUS_STRUCT (type))
		{
		  field &fld = type->field (i);

		  if (fld.is_protected ())
		    {
		      if (section_type != s_protected)
			{
			  section_type = s_protected;
			  gdb_printf (stream, "%*sprotected\n",
				      level + 2, "");
			}
		    }
		  else if (fld.is_private ())
		    {
		      if (section_type != s_private)
			{
			  section_type = s_private;
			  gdb_printf (stream, "%*sprivate\n",
				      level + 2, "");
			}
		    }
		  else
		    {
		      if (section_type != s_public)
			{
			  section_type = s_public;
			  gdb_printf (stream, "%*spublic\n",
				      level + 2, "");
			}
		    }
		}

	      print_spaces (level + 4, stream);
	      if (type->field (i).is_static ())
		gdb_printf (stream, "static ");
	      print_type (type->field (i).type (),
				 type->field (i).name (),
				 stream, show - 1, level + 4, flags);
	      if (!type->field (i).is_static ()
		  && type->field (i).is_packed ())
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

	  /* If there are both fields and methods, put a space between.  */
	  len = TYPE_NFN_FIELDS (type);
	  if (len && section_type != s_none)
	    gdb_printf (stream, "\n");

	  /* Object pascal: print out the methods.  */

	  for (i = 0; i < len; i++)
	    {
	      struct fn_field *f = TYPE_FN_FIELDLIST1 (type, i);
	      int j, len2 = TYPE_FN_FIELDLIST_LENGTH (type, i);
	      const char *method_name = TYPE_FN_FIELDLIST_NAME (type, i);

	      /* this is GNU C++ specific
		 how can we know constructor/destructor?
		 It might work for GNU pascal.  */
	      for (j = 0; j < len2; j++)
		{
		  const char *physname = TYPE_FN_FIELD_PHYSNAME (f, j);

		  int is_constructor = (startswith (physname, "__ct__"));
		  int is_destructor = (startswith (physname, "__dt__"));

		  QUIT;
		  if (TYPE_FN_FIELD_PROTECTED (f, j))
		    {
		      if (section_type != s_protected)
			{
			  section_type = s_protected;
			  gdb_printf (stream, "%*sprotected\n",
				      level + 2, "");
			}
		    }
		  else if (TYPE_FN_FIELD_PRIVATE (f, j))
		    {
		      if (section_type != s_private)
			{
			  section_type = s_private;
			  gdb_printf (stream, "%*sprivate\n",
				      level + 2, "");
			}
		    }
		  else
		    {
		      if (section_type != s_public)
			{
			  section_type = s_public;
			  gdb_printf (stream, "%*spublic\n",
				      level + 2, "");
			}
		    }

		  print_spaces (level + 4, stream);
		  if (TYPE_FN_FIELD_STATIC_P (f, j))
		    gdb_printf (stream, "static ");
		  if (TYPE_FN_FIELD_TYPE (f, j)->target_type () == 0)
		    {
		      /* Keep GDB from crashing here.  */
		      gdb_printf (stream, "<undefined type> %s;\n",
				  TYPE_FN_FIELD_PHYSNAME (f, j));
		      break;
		    }

		  if (is_constructor)
		    {
		      gdb_printf (stream, "constructor ");
		    }
		  else if (is_destructor)
		    {
		      gdb_printf (stream, "destructor  ");
		    }
		  else if (TYPE_FN_FIELD_TYPE (f, j)->target_type () != 0
			   && (TYPE_FN_FIELD_TYPE(f, j)->target_type ()->code ()
			       != TYPE_CODE_VOID))
		    {
		      gdb_printf (stream, "function  ");
		    }
		  else
		    {
		      gdb_printf (stream, "procedure ");
		    }
		  /* This does not work, no idea why !!  */

		  type_print_method_args (physname, method_name, stream);

		  if (TYPE_FN_FIELD_TYPE (f, j)->target_type () != 0
		      && (TYPE_FN_FIELD_TYPE(f, j)->target_type ()->code ()
			  != TYPE_CODE_VOID))
		    {
		      gdb_puts (" : ", stream);
		      type_print (TYPE_FN_FIELD_TYPE (f, j)->target_type (),
				  "", stream, -1);
		    }
		  if (TYPE_FN_FIELD_VIRTUAL_P (f, j))
		    gdb_printf (stream, "; virtual");

		  gdb_printf (stream, ";\n");
		}
	    }
	  gdb_printf (stream, "%*send", level, "");
	}
      break;

    case TYPE_CODE_ENUM:
      if (type->name () != NULL)
	{
	  gdb_puts (type->name (), stream);
	  if (show > 0)
	    gdb_puts (" ", stream);
	}
      /* enum is just defined by
	 type enume_name = (enum_member1,enum_member2,...)  */
      gdb_printf (stream, " = ");
      stream->wrap_here (4);
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
	      if (i)
		gdb_printf (stream, ", ");
	      stream->wrap_here (4);
	      gdb_puts (type->field (i).name (), stream);
	      if (lastval != type->field (i).loc_enumval ())
		{
		  gdb_printf (stream,
			      " := %s",
			      plongest (type->field (i).loc_enumval ()));
		  lastval = type->field (i).loc_enumval ();
		}
	      lastval++;
	    }
	  gdb_printf (stream, ")");
	}
      break;

    case TYPE_CODE_VOID:
      gdb_printf (stream, "void");
      break;

    case TYPE_CODE_UNDEF:
      gdb_printf (stream, "record <unknown>");
      break;

    case TYPE_CODE_ERROR:
      gdb_printf (stream, "%s", TYPE_ERROR_NAME (type));
      break;

      /* this probably does not work for enums.  */
    case TYPE_CODE_RANGE:
      {
	struct type *target = type->target_type ();

	print_type_scalar (target, type->bounds ()->low.const_val (), stream);
	gdb_puts ("..", stream);
	print_type_scalar (target, type->bounds ()->high.const_val (), stream);
      }
      break;

    case TYPE_CODE_SET:
      gdb_puts ("set of ", stream);
      print_type (type->index_type (), "", stream,
			 show - 1, level, flags);
      break;

    case TYPE_CODE_STRING:
      gdb_puts ("String", stream);
      break;

    default:
      /* Handle types not explicitly handled by the other cases,
	 such as fundamental types.  For these, just print whatever
	 the type name is, as recorded in the type itself.  If there
	 is no type name, then complain.  */
      if (type->name () != NULL)
	{
	  gdb_puts (type->name (), stream);
	}
      else
	{
	  /* At least for dump_symtab, it is important that this not be
	     an error ().  */
	  fprintf_styled (stream, metadata_style.style (),
			  "<invalid unnamed pascal type code %d>",
			  type->code ());
	}
      break;
    }
}
