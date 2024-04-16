/* Language independent support for printing types for GDB, the GNU debugger.

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
#include "gdbsupport/gdb_obstack.h"
#include "bfd.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "expression.h"
#include "value.h"
#include "gdbcore.h"
#include "command.h"
#include "gdbcmd.h"
#include "target.h"
#include "language.h"
#include "cp-abi.h"
#include "typeprint.h"
#include "valprint.h"
#include <ctype.h>
#include "cli/cli-utils.h"
#include "extension.h"
#include "completer.h"
#include "cli/cli-style.h"

const struct type_print_options type_print_raw_options =
{
  1,				/* raw */
  1,				/* print_methods */
  1,				/* print_typedefs */
  0,				/* print_offsets */
  0,				/* print_in_hex */
  0,				/* print_nested_type_limit  */
  NULL,				/* local_typedefs */
  NULL,				/* global_table */
  NULL				/* global_printers */
};

/* The default flags for 'ptype' and 'whatis'.  */

static struct type_print_options default_ptype_flags =
{
  0,				/* raw */
  1,				/* print_methods */
  1,				/* print_typedefs */
  0,				/* print_offsets */
  0,				/* print_in_hex */
  0,				/* print_nested_type_limit  */
  NULL,				/* local_typedefs */
  NULL,				/* global_table */
  NULL				/* global_printers */
};



/* See typeprint.h.  */

const int print_offset_data::indentation = 27;

/* See typeprint.h.  */

print_offset_data::print_offset_data (const struct type_print_options *flags)
{
  if (flags != nullptr)
    print_in_hex = flags->print_in_hex;
}

/* See typeprint.h.  */

void
print_offset_data::maybe_print_hole (struct ui_file *stream,
				     unsigned int bitpos,
				     const char *for_what)
{
  /* We check for END_BITPOS > 0 because there is a specific
     scenario when END_BITPOS can be zero and BITPOS can be >
     0: when we are dealing with a struct/class with a virtual method.
     Because of the vtable, the first field of the struct/class will
     have an offset of sizeof (void *) (the size of the vtable).  If
     we do not check for END_BITPOS > 0 here, GDB will report
     a hole before the first field, which is not accurate.  */
  if (end_bitpos > 0 && end_bitpos < bitpos)
    {
      /* If END_BITPOS is smaller than the current type's
	 bitpos, it means there's a hole in the struct, so we report
	 it here.  */
      unsigned int hole = bitpos - end_bitpos;
      unsigned int hole_byte = hole / TARGET_CHAR_BIT;
      unsigned int hole_bit = hole % TARGET_CHAR_BIT;

      if (hole_bit > 0)
	{
	  fprintf_styled (stream, highlight_style.style (),
			  "/* XXX %2u-bit %-7s    */", hole_bit, for_what);
	  gdb_puts ("\n", stream);
	}

      if (hole_byte > 0)
	{
	  fprintf_styled (stream, highlight_style.style (),
			  "/* XXX %2u-byte %-7s   */", hole_byte, for_what);
	  gdb_puts ("\n", stream);
	}
    }
}

/* See typeprint.h.  */

void
print_offset_data::update (struct type *type, unsigned int field_idx,
			   struct ui_file *stream)
{
  if (type->field (field_idx).is_static ())
    {
      print_spaces (indentation, stream);
      return;
    }

  struct type *ftype = check_typedef (type->field (field_idx).type ());
  if (type->code () == TYPE_CODE_UNION)
    {
      /* Since union fields don't have the concept of offsets, we just
	 print their sizes.  */
      gdb_printf (stream, "/*                %6s */",
		  (print_in_hex ?
		   hex_string_custom (ftype->length (), 4) :
		   pulongest (ftype->length ())));
      return;
    }

  unsigned int bitpos = type->field (field_idx).loc_bitpos ();
  unsigned int fieldsize_byte = ftype->length ();
  unsigned int fieldsize_bit = fieldsize_byte * TARGET_CHAR_BIT;

  maybe_print_hole (stream, bitpos, "hole");

  if (type->field (field_idx).is_packed ()
      || offset_bitpos % TARGET_CHAR_BIT != 0)
    {
      /* We're dealing with a bitfield.  Print the bit offset.  */
      fieldsize_bit = type->field (field_idx).bitsize ();

      unsigned real_bitpos = bitpos + offset_bitpos;

      gdb_printf (stream,
		  (print_in_hex ? "/* 0x%04x: 0x%x" : "/* %6u:%2u  "),
		  real_bitpos / TARGET_CHAR_BIT,
		  real_bitpos % TARGET_CHAR_BIT);
    }
  else
    {
      /* The position of the field, relative to the beginning of the
	 struct.  */
      gdb_printf (stream, (print_in_hex ?  "/* 0x%04x" : "/* %6u"),
		  (bitpos + offset_bitpos) / TARGET_CHAR_BIT);

      gdb_printf (stream, "     ");
    }

  gdb_printf (stream, (print_in_hex ? " |  0x%04x */" : " |  %6u */"),
	      fieldsize_byte);

  end_bitpos = bitpos + fieldsize_bit;
}

/* See typeprint.h.  */

void
print_offset_data::finish (struct type *type, int level,
			   struct ui_file *stream)
{
  unsigned int bitpos = type->length () * TARGET_CHAR_BIT;
  maybe_print_hole (stream, bitpos, "padding");

  gdb_puts ("\n", stream);
  print_spaces (level + 4 + print_offset_data::indentation, stream);
  gdb_printf (stream, "/* total size (bytes): %4s */\n",
	      pulongest (type->length ()));
}



/* A hash function for a typedef_field.  */

static hashval_t
hash_typedef_field (const void *p)
{
  const struct decl_field *tf = (const struct decl_field *) p;

  return htab_hash_string (TYPE_SAFE_NAME (tf->type));
}

/* An equality function for a typedef field.  */

static int
eq_typedef_field (const void *a, const void *b)
{
  const struct decl_field *tfa = (const struct decl_field *) a;
  const struct decl_field *tfb = (const struct decl_field *) b;

  return types_equal (tfa->type, tfb->type);
}

/* See typeprint.h.  */

void
typedef_hash_table::recursively_update (struct type *t)
{
  int i;

  for (i = 0; i < TYPE_TYPEDEF_FIELD_COUNT (t); ++i)
    {
      struct decl_field *tdef = &TYPE_TYPEDEF_FIELD (t, i);
      void **slot;

      slot = htab_find_slot (m_table.get (), tdef, INSERT);
      /* Only add a given typedef name once.  Really this shouldn't
	 happen; but it is safe enough to do the updates breadth-first
	 and thus use the most specific typedef.  */
      if (*slot == NULL)
	*slot = tdef;
    }

  /* Recurse into superclasses.  */
  for (i = 0; i < TYPE_N_BASECLASSES (t); ++i)
    recursively_update (TYPE_BASECLASS (t, i));
}

/* See typeprint.h.  */

void
typedef_hash_table::add_template_parameters (struct type *t)
{
  int i;

  for (i = 0; i < TYPE_N_TEMPLATE_ARGUMENTS (t); ++i)
    {
      struct decl_field *tf;
      void **slot;

      /* We only want type-valued template parameters in the hash.  */
      if (TYPE_TEMPLATE_ARGUMENT (t, i)->aclass () != LOC_TYPEDEF)
	continue;

      tf = XOBNEW (&m_storage, struct decl_field);
      tf->name = TYPE_TEMPLATE_ARGUMENT (t, i)->linkage_name ();
      tf->type = TYPE_TEMPLATE_ARGUMENT (t, i)->type ();

      slot = htab_find_slot (m_table.get (), tf, INSERT);
      if (*slot == NULL)
	*slot = tf;
    }
}

/* See typeprint.h.  */

typedef_hash_table::typedef_hash_table ()
  : m_table (htab_create_alloc (10, hash_typedef_field, eq_typedef_field,
				NULL, xcalloc, xfree))
{
}

/* Helper function for typedef_hash_table::copy.  */

static int
copy_typedef_hash_element (void **slot, void *nt)
{
  htab_t new_table = (htab_t) nt;
  void **new_slot;

  new_slot = htab_find_slot (new_table, *slot, INSERT);
  if (*new_slot == NULL)
    *new_slot = *slot;

  return 1;
}

/* See typeprint.h.  */

typedef_hash_table::typedef_hash_table (const typedef_hash_table &table)
{
  m_table.reset (htab_create_alloc (10, hash_typedef_field, eq_typedef_field,
				    NULL, xcalloc, xfree));
  htab_traverse_noresize (table.m_table.get (), copy_typedef_hash_element,
			  m_table.get ());
}

/* Look up the type T in the global typedef hash.  If it is found,
   return the typedef name.  If it is not found, apply the
   type-printers, if any, given by start_script_type_printers and return the
   result.  A NULL return means that the name was not found.  */

const char *
typedef_hash_table::find_global_typedef (const struct type_print_options *flags,
					 struct type *t)
{
  void **slot;
  struct decl_field tf, *new_tf;

  if (flags->global_typedefs == NULL)
    return NULL;

  tf.name = NULL;
  tf.type = t;

  slot = htab_find_slot (flags->global_typedefs->m_table.get (), &tf, INSERT);
  if (*slot != NULL)
    {
      new_tf = (struct decl_field *) *slot;
      return new_tf->name;
    }

  /* Put an entry into the hash table now, in case
     apply_ext_lang_type_printers recurses.  */
  new_tf = XOBNEW (&flags->global_typedefs->m_storage, struct decl_field);
  new_tf->name = NULL;
  new_tf->type = t;

  *slot = new_tf;

  gdb::unique_xmalloc_ptr<char> applied
    = apply_ext_lang_type_printers (flags->global_printers, t);

  if (applied != nullptr)
    new_tf->name = obstack_strdup (&flags->global_typedefs->m_storage,
				   applied.get ());

  return new_tf->name;
}

/* See typeprint.h.  */

const char *
typedef_hash_table::find_typedef (const struct type_print_options *flags,
				  struct type *t)
{
  if (flags->local_typedefs != NULL)
    {
      struct decl_field tf, *found;

      tf.name = NULL;
      tf.type = t;
      htab_t table = flags->local_typedefs->m_table.get ();
      found = (struct decl_field *) htab_find (table, &tf);

      if (found != NULL)
	return found->name;
    }

  return find_global_typedef (flags, t);
}



/* Print a description of a type in the format of a 
   typedef for the current language.
   NEW is the new name for a type TYPE.  */

void
typedef_print (struct type *type, struct symbol *newobj, struct ui_file *stream)
{
  current_language->print_typedef (type, newobj, stream);
}

/* Print a description of a type TYPE in the form of a declaration of a
   variable named VARSTRING.  (VARSTRING is demangled if necessary.)
   Output goes to STREAM (via stdio).
   If SHOW is positive, we show the contents of the outermost level
   of structure even if there is a type name that could be used instead.
   If SHOW is negative, we never show the details of elements' types.  */

void
type_print (struct type *type, const char *varstring, struct ui_file *stream,
	    int show)
{
  current_language->print_type (type, varstring, stream, show, 0,
				&default_ptype_flags);
}

/* Print TYPE to a string, returning it.  The caller is responsible for
   freeing the string.  */

std::string
type_to_string (struct type *type)
{
  try
    {
      string_file stb;

      type_print (type, "", &stb, -1);
      return stb.release ();
    }
  catch (const gdb_exception &except)
    {
    }

  return {};
}

/* See typeprint.h.  */

void
type_print_unknown_return_type (struct ui_file *stream)
{
  fprintf_styled (stream, metadata_style.style (),
		  _("<unknown return type>"));
}

/* See typeprint.h.  */

void
error_unknown_type (const char *sym_print_name)
{
  error (_("'%s' has unknown type; cast it to its declared type"),
	 sym_print_name);
}

/* Print type of EXP, or last thing in value history if EXP == NULL.
   show is passed to type_print.  */

static void
whatis_exp (const char *exp, int show)
{
  struct value *val;
  struct type *real_type = NULL;
  struct type *type;
  int full = 0;
  LONGEST top = -1;
  int using_enc = 0;
  struct value_print_options opts;
  struct type_print_options flags = default_ptype_flags;

  if (exp)
    {
      if (*exp == '/')
	{
	  int seen_one = 0;

	  for (++exp; *exp && !isspace (*exp); ++exp)
	    {
	      switch (*exp)
		{
		case 'r':
		  flags.raw = 1;
		  break;
		case 'm':
		  flags.print_methods = 0;
		  break;
		case 'M':
		  flags.print_methods = 1;
		  break;
		case 't':
		  flags.print_typedefs = 0;
		  break;
		case 'T':
		  flags.print_typedefs = 1;
		  break;
		case 'o':
		  {
		    /* Filter out languages which don't implement the
		       feature.  */
		    if (show > 0
			&& current_language->can_print_type_offsets ())
		      {
			flags.print_offsets = 1;
			flags.print_typedefs = 0;
			flags.print_methods = 0;
		      }
		    break;
		  }
		case 'x':
		  flags.print_in_hex = 1;
		  break;
		case 'd':
		  flags.print_in_hex = 0;
		  break;
		default:
		  error (_("unrecognized flag '%c'"), *exp);
		}
	      seen_one = 1;
	    }

	  if (!*exp && !seen_one)
	    error (_("flag expected"));
	  if (!isspace (*exp))
	    error (_("expected space after format"));
	  exp = skip_spaces (exp);
	}

      expression_up expr = parse_expression (exp);

      /* The behavior of "whatis" depends on whether the user
	 expression names a type directly, or a language expression
	 (including variable names).  If the former, then "whatis"
	 strips one level of typedefs, only.  If an expression,
	 "whatis" prints the type of the expression without stripping
	 any typedef level.  "ptype" always strips all levels of
	 typedefs.  */
      val = expr->evaluate_type ();
      type = val->type ();

      if (show == -1 && expr->first_opcode () == OP_TYPE)
	{
	  /* The user expression names a type directly.  */

	  /* If this is a typedef, then find its immediate target.
	     Use check_typedef to resolve stubs, but ignore its result
	     because we do not want to dig past all typedefs.  */
	  check_typedef (type);
	  if (type->code () == TYPE_CODE_TYPEDEF)
	    type = type->target_type ();

	  /* If the expression is actually a type, then there's no
	     value to fetch the dynamic type from.  */
	  val = NULL;
	}
    }
  else
    {
      val = access_value_history (0);
      type = val->type ();
    }

  if (flags.print_offsets && is_dynamic_type (type))
    {
      warning (_("ptype/o does not work with dynamic types; disabling '/o'"));
      flags.print_offsets = 0;
    }

  get_user_print_options (&opts);
  if (val != NULL && opts.objectprint)
    {
      if (type->is_pointer_or_reference ()
	  && (type->target_type ()->code () == TYPE_CODE_STRUCT))
	real_type = value_rtti_indirect_type (val, &full, &top, &using_enc);
      else if (type->code () == TYPE_CODE_STRUCT)
	real_type = value_rtti_type (val, &full, &top, &using_enc);
    }

  if (flags.print_offsets
      && (type->code () == TYPE_CODE_STRUCT
	  || type->code () == TYPE_CODE_UNION))
    gdb_printf ("/* offset      |    size */  ");

  gdb_printf ("type = ");

  std::unique_ptr<typedef_hash_table> table_holder;
  std::unique_ptr<ext_lang_type_printers> printer_holder;
  if (!flags.raw)
    {
      table_holder.reset (new typedef_hash_table);
      flags.global_typedefs = table_holder.get ();

      printer_holder.reset (new ext_lang_type_printers);
      flags.global_printers = printer_holder.get ();
    }

  if (real_type)
    {
      gdb_printf ("/* real type = ");
      type_print (real_type, "", gdb_stdout, -1);
      if (! full)
	gdb_printf (" (incomplete object)");
      gdb_printf (" */\n");    
    }

  current_language->print_type (type, "", gdb_stdout, show, 0, &flags);
  gdb_printf ("\n");
}

static void
whatis_command (const char *exp, int from_tty)
{
  /* Most of the time users do not want to see all the fields
     in a structure.  If they do they can use the "ptype" command.
     Hence the "-1" below.  */
  whatis_exp (exp, -1);
}

/* TYPENAME is either the name of a type, or an expression.  */

static void
ptype_command (const char *type_name, int from_tty)
{
  whatis_exp (type_name, 1);
}

/* Print integral scalar data VAL, of type TYPE, onto stdio stream STREAM.
   Used to print data from type structures in a specified type.  For example,
   array bounds may be characters or booleans in some languages, and this
   allows the ranges to be printed in their "natural" form rather than as
   decimal integer values.

   FIXME:  This is here simply because only the type printing routines
   currently use it, and it wasn't clear if it really belonged somewhere
   else (like printcmd.c).  There are a lot of other gdb routines that do
   something similar, but they are generally concerned with printing values
   that come from the inferior in target byte order and target size.  */

void
print_type_scalar (struct type *type, LONGEST val, struct ui_file *stream)
{
  unsigned int i;
  unsigned len;

  type = check_typedef (type);

  switch (type->code ())
    {

    case TYPE_CODE_ENUM:
      len = type->num_fields ();
      for (i = 0; i < len; i++)
	{
	  if (type->field (i).loc_enumval () == val)
	    {
	      break;
	    }
	}
      if (i < len)
	{
	  gdb_puts (type->field (i).name (), stream);
	}
      else
	{
	  print_longest (stream, 'd', 0, val);
	}
      break;

    case TYPE_CODE_INT:
      print_longest (stream, type->is_unsigned () ? 'u' : 'd', 0, val);
      break;

    case TYPE_CODE_CHAR:
      current_language->printchar ((unsigned char) val, type, stream);
      break;

    case TYPE_CODE_BOOL:
      gdb_printf (stream, val ? "TRUE" : "FALSE");
      break;

    case TYPE_CODE_RANGE:
      print_type_scalar (type->target_type (), val, stream);
      return;

    case TYPE_CODE_FIXED_POINT:
      print_type_fixed_point (type, stream);
      break;

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
    case TYPE_CODE_RVALUE_REF:
    case TYPE_CODE_NAMESPACE:
      error (_("internal error: unhandled type in print_type_scalar"));
      break;

    default:
      error (_("Invalid type code in symbol table."));
    }
}

/* See typeprint.h.  */

void
print_type_fixed_point (struct type *type, struct ui_file *stream)
{
  std::string small_img = type->fixed_point_scaling_factor ().str ();

  gdb_printf (stream, "%s-byte fixed point (small = %s)",
	      pulongest (type->length ()), small_img.c_str ());
}

/* Dump details of a type specified either directly or indirectly.
   Uses the same sort of type lookup mechanism as ptype_command()
   and whatis_command().  */

void
maintenance_print_type (const char *type_name, int from_tty)
{
  if (type_name != NULL)
    {
      expression_up expr = parse_expression (type_name);
      struct value *val = expr->evaluate_type ();
      struct type *type = val->type ();

      if (type != nullptr)
	recursive_dump_type (type, 0);
    }
}


struct cmd_list_element *setprinttypelist;

struct cmd_list_element *showprinttypelist;

static bool print_methods = true;

static void
set_print_type_methods (const char *args,
			int from_tty, struct cmd_list_element *c)
{
  default_ptype_flags.print_methods = print_methods;
}

static void
show_print_type_methods (struct ui_file *file, int from_tty,
			 struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Printing of methods defined in a class in %s\n"),
	      value);
}

static bool print_typedefs = true;

static void
set_print_type_typedefs (const char *args,
			 int from_tty, struct cmd_list_element *c)
{
  default_ptype_flags.print_typedefs = print_typedefs;
}

static void
show_print_type_typedefs (struct ui_file *file, int from_tty,
			 struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Printing of typedefs defined in a class in %s\n"),
	      value);
}

/* Limit on the number of nested type definitions to print or -1 to print
   all nested type definitions in a class.  By default, we do not print
   nested definitions.  */

static int print_nested_type_limit = 0;

/* Set how many nested type definitions should be printed by the type
   printer.  */

static void
set_print_type_nested_types (const char *args, int from_tty,
			     struct cmd_list_element *c)
{
  default_ptype_flags.print_nested_type_limit = print_nested_type_limit;
}

/* Show how many nested type definitions the type printer will print.  */

static void
show_print_type_nested_types  (struct ui_file *file, int from_tty,
			       struct cmd_list_element *c, const char *value)
{
  if (*value == '0')
    {
      gdb_printf (file,
		  _("Will not print nested types defined in a class\n"));
    }
  else
    {
      gdb_printf (file,
		  _("Will print %s nested types defined in a class\n"),
		  value);
    }
}

/* When printing structs, offsets and sizes of members can be displayed using
   decimal notation or hexadecimal notation.  By default, Decimal notation is
   used.  */

static bool print_offsets_and_sizes_in_hex = false;

/* Set the flags that instructs if sizes and offsets of struct members are
   displayed in hexadecimal or decimal notation.  */

static void
set_print_offsets_and_sizes_in_hex (const char *args,
				    int from_tty, struct cmd_list_element *c)
{
  default_ptype_flags.print_in_hex = print_offsets_and_sizes_in_hex;
}

/* Display whether struct members sizes and offsets are printed
   using decimal or hexadecimal notation.  */

static void
show_print_offsets_and_sizes_in_hex (struct ui_file *file, int from_tty,
				     struct cmd_list_element *c,
				     const char *value)
{
  gdb_printf (file, _("\
Display of struct members offsets and sizes in hexadecimal is %s\n"),
	      value);
}

void _initialize_typeprint ();
void
_initialize_typeprint ()
{
  struct cmd_list_element *c;

  c = add_com ("ptype", class_vars, ptype_command, _("\
Print definition of type TYPE.\n\
Usage: ptype[/FLAGS] TYPE | EXPRESSION\n\
Argument may be any type (for example a type name defined by typedef,\n\
or \"struct STRUCT-TAG\" or \"class CLASS-NAME\" or \"union UNION-TAG\"\n\
or \"enum ENUM-TAG\") or an expression.\n\
The selected stack frame's lexical context is used to look up the name.\n\
Contrary to \"whatis\", \"ptype\" always unrolls any typedefs.\n\
\n\
Available FLAGS are:\n\
  /r    print in \"raw\" form; do not substitute typedefs\n\
  /m    do not print methods defined in a class\n\
  /M    print methods defined in a class\n\
  /t    do not print typedefs defined in a class\n\
  /T    print typedefs defined in a class\n\
  /o    print offsets and sizes of fields in a struct (like pahole)\n\
  /x    use hexadecimal notation when displaying sizes and offsets\n\
	of struct members\n\
  /d    use decimal notation when displaying sizes and offsets\n\
	of struct members "));
  set_cmd_completer (c, expression_completer);

  c = add_com ("whatis", class_vars, whatis_command,
	       _("Print data type of expression EXP.\n\
Only one level of typedefs is unrolled.  See also \"ptype\"."));
  set_cmd_completer (c, expression_completer);

  add_setshow_prefix_cmd ("type", no_class,
			  _("Generic command for showing type-printing settings."),
			  _("Generic command for setting how types print."),
			  &setprinttypelist, &showprinttypelist,
			  &setprintlist, &showprintlist);

  add_setshow_boolean_cmd ("methods", no_class, &print_methods,
			   _("\
Set printing of methods defined in classes."), _("\
Show printing of methods defined in classes."), NULL,
			   set_print_type_methods,
			   show_print_type_methods,
			   &setprinttypelist, &showprinttypelist);
  add_setshow_boolean_cmd ("typedefs", no_class, &print_typedefs,
			   _("\
Set printing of typedefs defined in classes."), _("\
Show printing of typedefs defined in classes."), NULL,
			   set_print_type_typedefs,
			   show_print_type_typedefs,
			   &setprinttypelist, &showprinttypelist);

  add_setshow_zuinteger_unlimited_cmd ("nested-type-limit", no_class,
				       &print_nested_type_limit,
				       _("\
Set the number of recursive nested type definitions to print \
(\"unlimited\" or -1 to show all)."), _("\
Show the number of recursive nested type definitions to print."), NULL,
				       set_print_type_nested_types,
				       show_print_type_nested_types,
				       &setprinttypelist, &showprinttypelist);

  add_setshow_boolean_cmd ("hex", no_class, &print_offsets_and_sizes_in_hex,
			   _("\
Set printing of struct members sizes and offsets using hex notation."), _("\
Show whether sizes and offsets of struct members are printed using hex \
notation."), nullptr, set_print_offsets_and_sizes_in_hex,
			   show_print_offsets_and_sizes_in_hex,
			   &setprinttypelist, &showprinttypelist);
}

/* Print <not allocated> status to stream STREAM.  */

void
val_print_not_allocated (struct ui_file *stream)
{
  fprintf_styled (stream, metadata_style.style (), _("<not allocated>"));
}

/* Print <not associated> status to stream STREAM.  */

void
val_print_not_associated (struct ui_file *stream)
{
  fprintf_styled (stream, metadata_style.style (), _("<not associated>"));
}
