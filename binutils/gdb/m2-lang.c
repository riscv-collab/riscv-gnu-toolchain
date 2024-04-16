/* Modula 2 language support routines for GDB, the GNU debugger.

   Copyright (C) 1992-2024 Free Software Foundation, Inc.

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
#include "parser-defs.h"
#include "language.h"
#include "varobj.h"
#include "m2-lang.h"
#include "c-lang.h"
#include "valprint.h"
#include "gdbarch.h"
#include "m2-exp.h"

/* A helper function for UNOP_HIGH.  */

struct value *
eval_op_m2_high (struct type *expect_type, struct expression *exp,
		 enum noside noside,
		 struct value *arg1)
{
  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    return arg1;
  else
    {
      arg1 = coerce_ref (arg1);
      struct type *type = check_typedef (arg1->type ());

      if (m2_is_unbounded_array (type))
	{
	  struct value *temp = arg1;

	  type = type->field (1).type ();
	  /* i18n: Do not translate the "_m2_high" part!  */
	  arg1 = value_struct_elt (&temp, {}, "_m2_high", NULL,
				   _("unbounded structure "
				     "missing _m2_high field"));

	  if (arg1->type () != type)
	    arg1 = value_cast (type, arg1);
	}
    }
  return arg1;
}

/* A helper function for BINOP_SUBSCRIPT.  */

struct value *
eval_op_m2_subscript (struct type *expect_type, struct expression *exp,
		      enum noside noside,
		      struct value *arg1, struct value *arg2)
{
  /* If the user attempts to subscript something that is not an
     array or pointer type (like a plain int variable for example),
     then report this as an error.  */

  arg1 = coerce_ref (arg1);
  struct type *type = check_typedef (arg1->type ());

  if (m2_is_unbounded_array (type))
    {
      struct value *temp = arg1;
      type = type->field (0).type ();
      if (type == NULL || (type->code () != TYPE_CODE_PTR))
	error (_("internal error: unbounded "
		 "array structure is unknown"));
      /* i18n: Do not translate the "_m2_contents" part!  */
      arg1 = value_struct_elt (&temp, {}, "_m2_contents", NULL,
			       _("unbounded structure "
				 "missing _m2_contents field"));
	  
      if (arg1->type () != type)
	arg1 = value_cast (type, arg1);

      check_typedef (arg1->type ());
      return value_ind (value_ptradd (arg1, value_as_long (arg2)));
    }
  else
    if (type->code () != TYPE_CODE_ARRAY)
      {
	if (type->name ())
	  error (_("cannot subscript something of type `%s'"),
		 type->name ());
	else
	  error (_("cannot subscript requested type"));
      }

  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    return value::zero (type->target_type (), arg1->lval ());
  else
    return value_subscript (arg1, value_as_long (arg2));
}



/* Single instance of the M2 language.  */

static m2_language m2_language_defn;

/* See language.h.  */

void
m2_language::language_arch_info (struct gdbarch *gdbarch,
				 struct language_arch_info *lai) const
{
  const struct builtin_m2_type *builtin = builtin_m2_type (gdbarch);

  /* Helper function to allow shorter lines below.  */
  auto add  = [&] (struct type * t)
  {
    lai->add_primitive_type (t);
  };

  add (builtin->builtin_char);
  add (builtin->builtin_int);
  add (builtin->builtin_card);
  add (builtin->builtin_real);
  add (builtin->builtin_bool);

  lai->set_string_char_type (builtin->builtin_char);
  lai->set_bool_type (builtin->builtin_bool, "BOOLEAN");
}

/* See language.h.  */

void
m2_language::printchar (int c, struct type *type,
			struct ui_file *stream) const
{
  gdb_puts ("'", stream);
  emitchar (c, type, stream, '\'');
  gdb_puts ("'", stream);
}

/* See language.h.  */

void
m2_language::printstr (struct ui_file *stream, struct type *elttype,
			const gdb_byte *string, unsigned int length,
			const char *encoding, int force_ellipses,
			const struct value_print_options *options) const
{
  unsigned int i;
  unsigned int things_printed = 0;
  int in_quotes = 0;
  int need_comma = 0;

  if (length == 0)
    {
      gdb_puts ("\"\"");
      return;
    }

  unsigned int print_max_chars = get_print_max_chars (options);
  for (i = 0; i < length && things_printed < print_max_chars; ++i)
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
      while (rep1 < length && string[rep1] == string[i])
	{
	  ++rep1;
	  ++reps;
	}

      if (reps > options->repeat_count_threshold)
	{
	  if (in_quotes)
	    {
	      gdb_puts ("\", ", stream);
	      in_quotes = 0;
	    }
	  printchar (string[i], elttype, stream);
	  gdb_printf (stream, " <repeats %u times>", reps);
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
	  emitchar (string[i], elttype, stream, '"');
	  ++things_printed;
	}
    }

  /* Terminate the quotes if necessary.  */
  if (in_quotes)
    gdb_puts ("\"", stream);

  if (force_ellipses || i < length)
    gdb_puts ("...", stream);
}

/* See language.h.  */

void
m2_language::emitchar (int ch, struct type *chtype,
		       struct ui_file *stream, int quoter) const
{
  ch &= 0xFF;			/* Avoid sign bit follies.  */

  if (PRINT_LITERAL_FORM (ch))
    {
      if (ch == '\\' || ch == quoter)
	gdb_puts ("\\", stream);
      gdb_printf (stream, "%c", ch);
    }
  else
    {
      switch (ch)
	{
	case '\n':
	  gdb_puts ("\\n", stream);
	  break;
	case '\b':
	  gdb_puts ("\\b", stream);
	  break;
	case '\t':
	  gdb_puts ("\\t", stream);
	  break;
	case '\f':
	  gdb_puts ("\\f", stream);
	  break;
	case '\r':
	  gdb_puts ("\\r", stream);
	  break;
	case '\033':
	  gdb_puts ("\\e", stream);
	  break;
	case '\007':
	  gdb_puts ("\\a", stream);
	  break;
	default:
	  gdb_printf (stream, "\\%.3o", (unsigned int) ch);
	  break;
	}
    }
}

/* Called during architecture gdbarch initialisation to create language
   specific types.  */

static struct builtin_m2_type *
build_m2_types (struct gdbarch *gdbarch)
{
  struct builtin_m2_type *builtin_m2_type = new struct builtin_m2_type;

  type_allocator alloc (gdbarch);

  /* Modula-2 "pervasive" types.  NOTE:  these can be redefined!!! */
  builtin_m2_type->builtin_int
    = init_integer_type (alloc, gdbarch_int_bit (gdbarch), 0, "INTEGER");
  builtin_m2_type->builtin_card
    = init_integer_type (alloc, gdbarch_int_bit (gdbarch), 1, "CARDINAL");
  builtin_m2_type->builtin_real
    = init_float_type (alloc, gdbarch_float_bit (gdbarch), "REAL",
		       gdbarch_float_format (gdbarch));
  builtin_m2_type->builtin_char
    = init_character_type (alloc, TARGET_CHAR_BIT, 1, "CHAR");
  builtin_m2_type->builtin_bool
    = init_boolean_type (alloc, gdbarch_int_bit (gdbarch), 1, "BOOLEAN");

  return builtin_m2_type;
}

static const registry<gdbarch>::key<struct builtin_m2_type> m2_type_data;

const struct builtin_m2_type *
builtin_m2_type (struct gdbarch *gdbarch)
{
  struct builtin_m2_type *result = m2_type_data.get (gdbarch);
  if (result == nullptr)
    {
      result = build_m2_types (gdbarch);
      m2_type_data.set (gdbarch, result);
    }

  return result;
}
