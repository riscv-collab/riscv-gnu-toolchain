/* Parse expressions for GDB.

   Copyright (C) 1986-2024 Free Software Foundation, Inc.

   Modified from expread.y by the Department of Computer Science at the
   State University of New York at Buffalo, 1991.

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

/* Parse an expression from text in a string,
   and return the result as a struct expression pointer.
   That structure contains arithmetic operations in reverse polish,
   with constants represented by operations that are followed by special data.
   See expression.h for the details of the format.
   What is important here is that it can be built up sequentially
   during the process of parsing; the lower levels of the tree always
   come first in the result.  */

#include "defs.h"
#include <ctype.h>
#include "arch-utils.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "frame.h"
#include "expression.h"
#include "value.h"
#include "command.h"
#include "language.h"
#include "parser-defs.h"
#include "gdbcmd.h"
#include "symfile.h"
#include "inferior.h"
#include "target-float.h"
#include "block.h"
#include "source.h"
#include "objfiles.h"
#include "user-regs.h"
#include <algorithm>
#include <optional>
#include "c-exp.h"

static unsigned int expressiondebug = 0;
static void
show_expressiondebug (struct ui_file *file, int from_tty,
		      struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Expression debugging is %s.\n"), value);
}


/* True if an expression parser should set yydebug.  */
static bool parser_debug;

static void
show_parserdebug (struct ui_file *file, int from_tty,
		  struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Parser debugging is %s.\n"), value);
}


/* Documented at it's declaration.  */

void
innermost_block_tracker::update (const struct block *b,
				 innermost_block_tracker_types t)
{
  if ((m_types & t) != 0
      && (m_innermost_block == NULL
	  || m_innermost_block->contains (b)))
    m_innermost_block = b;
}



bool
expr_complete_tag::complete (struct expression *exp,
			     completion_tracker &tracker)
{
  collect_symbol_completion_matches_type (tracker, m_name.get (),
					  m_name.get (), m_code);
  return true;
}

/* See parser-defs.h.  */

void
parser_state::mark_struct_expression (expr::structop_base_operation *op)
{
  gdb_assert (parse_completion && m_completion_state == nullptr);
  m_completion_state.reset (new expr_complete_structop (op));
}

/* Indicate that the current parser invocation is completing a tag.
   TAG is the type code of the tag, and PTR and LENGTH represent the
   start of the tag name.  */

void
parser_state::mark_completion_tag (enum type_code tag, const char *ptr,
				   int length)
{
  gdb_assert (parse_completion && m_completion_state == nullptr);
  gdb_assert (tag == TYPE_CODE_UNION
	      || tag == TYPE_CODE_STRUCT
	      || tag == TYPE_CODE_ENUM);
  m_completion_state.reset
    (new expr_complete_tag (tag, make_unique_xstrndup (ptr, length)));
}

/* See parser-defs.h.  */

void
parser_state::push_c_string (int kind, struct stoken_vector *vec)
{
  std::vector<std::string> data (vec->len);
  for (int i = 0; i < vec->len; ++i)
    data[i] = std::string (vec->tokens[i].ptr, vec->tokens[i].length);

  push_new<expr::c_string_operation> ((enum c_string_type_values) kind,
				      std::move (data));
}

/* See parser-defs.h.  */

void
parser_state::push_symbol (const char *name, block_symbol sym)
{
  if (sym.symbol != nullptr)
    {
      if (symbol_read_needs_frame (sym.symbol))
	block_tracker->update (sym);
      push_new<expr::var_value_operation> (sym);
    }
  else
    {
      struct bound_minimal_symbol msymbol = lookup_bound_minimal_symbol (name);
      if (msymbol.minsym != NULL)
	push_new<expr::var_msym_value_operation> (msymbol);
      else if (!have_full_symbols () && !have_partial_symbols ())
	error (_("No symbol table is loaded.  Use the \"file\" command."));
      else
	error (_("No symbol \"%s\" in current context."), name);
    }
}

/* See parser-defs.h.  */

void
parser_state::push_dollar (struct stoken str)
{
  struct block_symbol sym;
  struct bound_minimal_symbol msym;
  struct internalvar *isym = NULL;
  std::string copy;

  /* Handle the tokens $digits; also $ (short for $0) and $$ (short for $$1)
     and $$digits (equivalent to $<-digits> if you could type that).  */

  int negate = 0;
  int i = 1;
  /* Double dollar means negate the number and add -1 as well.
     Thus $$ alone means -1.  */
  if (str.length >= 2 && str.ptr[1] == '$')
    {
      negate = 1;
      i = 2;
    }
  if (i == str.length)
    {
      /* Just dollars (one or two).  */
      i = -negate;
      push_new<expr::last_operation> (i);
      return;
    }
  /* Is the rest of the token digits?  */
  for (; i < str.length; i++)
    if (!(str.ptr[i] >= '0' && str.ptr[i] <= '9'))
      break;
  if (i == str.length)
    {
      i = atoi (str.ptr + 1 + negate);
      if (negate)
	i = -i;
      push_new<expr::last_operation> (i);
      return;
    }

  /* Handle tokens that refer to machine registers:
     $ followed by a register name.  */
  i = user_reg_map_name_to_regnum (gdbarch (),
				   str.ptr + 1, str.length - 1);
  if (i >= 0)
    {
      str.length--;
      str.ptr++;
      push_new<expr::register_operation> (copy_name (str));
      block_tracker->update (expression_context_block,
			     INNERMOST_BLOCK_FOR_REGISTERS);
      return;
    }

  /* Any names starting with $ are probably debugger internal variables.  */

  copy = copy_name (str);
  isym = lookup_only_internalvar (copy.c_str () + 1);
  if (isym)
    {
      push_new<expr::internalvar_operation> (isym);
      return;
    }

  /* On some systems, such as HP-UX and hppa-linux, certain system routines
     have names beginning with $ or $$.  Check for those, first.  */

  sym = lookup_symbol (copy.c_str (), NULL, VAR_DOMAIN, NULL);
  if (sym.symbol)
    {
      push_new<expr::var_value_operation> (sym);
      return;
    }
  msym = lookup_bound_minimal_symbol (copy.c_str ());
  if (msym.minsym)
    {
      push_new<expr::var_msym_value_operation> (msym);
      return;
    }

  /* Any other names are assumed to be debugger internal variables.  */

  push_new<expr::internalvar_operation>
    (create_internalvar (copy.c_str () + 1));
}

/* See parser-defs.h.  */

void
parser_state::parse_error (const char *msg)
{
  if (this->prev_lexptr)
    this->lexptr = this->prev_lexptr;

  if (*this->lexptr == '\0')
    error (_("A %s in expression, near the end of `%s'."),
	   msg, this->start_of_input);
  else
    error (_("A %s in expression, near `%s'."), msg, this->lexptr);
}



const char *
find_template_name_end (const char *p)
{
  int depth = 1;
  int just_seen_right = 0;
  int just_seen_colon = 0;
  int just_seen_space = 0;

  if (!p || (*p != '<'))
    return 0;

  while (*++p)
    {
      switch (*p)
	{
	case '\'':
	case '\"':
	case '{':
	case '}':
	  /* In future, may want to allow these??  */
	  return 0;
	case '<':
	  depth++;		/* start nested template */
	  if (just_seen_colon || just_seen_right || just_seen_space)
	    return 0;		/* but not after : or :: or > or space */
	  break;
	case '>':
	  if (just_seen_colon || just_seen_right)
	    return 0;		/* end a (nested?) template */
	  just_seen_right = 1;	/* but not after : or :: */
	  if (--depth == 0)	/* also disallow >>, insist on > > */
	    return ++p;		/* if outermost ended, return */
	  break;
	case ':':
	  if (just_seen_space || (just_seen_colon > 1))
	    return 0;		/* nested class spec coming up */
	  just_seen_colon++;	/* we allow :: but not :::: */
	  break;
	case ' ':
	  break;
	default:
	  if (!((*p >= 'a' && *p <= 'z') ||	/* allow token chars */
		(*p >= 'A' && *p <= 'Z') ||
		(*p >= '0' && *p <= '9') ||
		(*p == '_') || (*p == ',') ||	/* commas for template args */
		(*p == '&') || (*p == '*') ||	/* pointer and ref types */
		(*p == '(') || (*p == ')') ||	/* function types */
		(*p == '[') || (*p == ']')))	/* array types */
	    return 0;
	}
      if (*p != ' ')
	just_seen_space = 0;
      if (*p != ':')
	just_seen_colon = 0;
      if (*p != '>')
	just_seen_right = 0;
    }
  return 0;
}


/* Return a null-terminated temporary copy of the name of a string token.

   Tokens that refer to names do so with explicit pointer and length,
   so they can share the storage that lexptr is parsing.
   When it is necessary to pass a name to a function that expects
   a null-terminated string, the substring is copied out
   into a separate block of storage.  */

std::string
copy_name (struct stoken token)
{
  return std::string (token.ptr, token.length);
}


/* As for parse_exp_1, except that if VOID_CONTEXT_P, then
   no value is expected from the expression.  */

static expression_up
parse_exp_in_context (const char **stringptr, CORE_ADDR pc,
		      const struct block *block,
		      parser_flags flags,
		      innermost_block_tracker *tracker,
		      std::unique_ptr<expr_completion_base> *completer)
{
  const struct language_defn *lang = NULL;

  if (*stringptr == 0 || **stringptr == 0)
    error_no_arg (_("expression to compute"));

  const struct block *expression_context_block = block;
  CORE_ADDR expression_context_pc = 0;

  innermost_block_tracker local_tracker;
  if (tracker == nullptr)
    tracker = &local_tracker;

  if ((flags & PARSER_LEAVE_BLOCK_ALONE) == 0)
    {
      /* If no context specified, try using the current frame, if any.  */
      if (!expression_context_block)
	expression_context_block
	  = get_selected_block (&expression_context_pc);
      else if (pc == 0)
	expression_context_pc = expression_context_block->entry_pc ();
      else
	expression_context_pc = pc;

      /* Fall back to using the current source static context, if any.  */

      if (!expression_context_block)
	{
	  struct symtab_and_line cursal
	    = get_current_source_symtab_and_line ();

	  if (cursal.symtab)
	    expression_context_block
	      = cursal.symtab->compunit ()->blockvector ()->static_block ();

	  if (expression_context_block)
	    expression_context_pc = expression_context_block->entry_pc ();
	}
    }

  if (language_mode == language_mode_auto && block != NULL)
    {
      /* Find the language associated to the given context block.
	 Default to the current language if it can not be determined.

	 Note that using the language corresponding to the current frame
	 can sometimes give unexpected results.  For instance, this
	 routine is often called several times during the inferior
	 startup phase to re-parse breakpoint expressions after
	 a new shared library has been loaded.  The language associated
	 to the current frame at this moment is not relevant for
	 the breakpoint.  Using it would therefore be silly, so it seems
	 better to rely on the current language rather than relying on
	 the current frame language to parse the expression.  That's why
	 we do the following language detection only if the context block
	 has been specifically provided.  */
      struct symbol *func = block->linkage_function ();

      if (func != NULL)
	lang = language_def (func->language ());
      if (lang == NULL || lang->la_language == language_unknown)
	lang = current_language;
    }
  else
    lang = current_language;

  /* get_current_arch may reset CURRENT_LANGUAGE via select_frame.
     While we need CURRENT_LANGUAGE to be set to LANG (for lookup_symbol
     and others called from *.y) ensure CURRENT_LANGUAGE gets restored
     to the value matching SELECTED_FRAME as set by get_current_arch.  */

  parser_state ps (lang, get_current_arch (), expression_context_block,
		   expression_context_pc, flags, *stringptr,
		   completer != nullptr, tracker);

  scoped_restore_current_language lang_saver;
  set_language (lang->la_language);

  try
    {
      lang->parser (&ps);
    }
  catch (const gdb_exception_error &except)
    {
      /* If parsing for completion, allow this to succeed; but if no
	 expression elements have been written, then there's nothing
	 to do, so fail.  */
      if (! ps.parse_completion || ps.expout->op == nullptr)
	throw;
    }

  expression_up result = ps.release ();
  result->op->set_outermost ();

  if (expressiondebug)
    result->dump (gdb_stdlog);

  if (completer != nullptr)
    *completer = std::move (ps.m_completion_state);
  *stringptr = ps.lexptr;
  return result;
}

/* Read an expression from the string *STRINGPTR points to,
   parse it, and return a pointer to a struct expression that we malloc.
   Use block BLOCK as the lexical context for variable names;
   if BLOCK is zero, use the block of the selected stack frame.
   Meanwhile, advance *STRINGPTR to point after the expression,
   at the first nonwhite character that is not part of the expression
   (possibly a null character).  FLAGS are passed to the parser.  */

expression_up
parse_exp_1 (const char **stringptr, CORE_ADDR pc, const struct block *block,
	     parser_flags flags, innermost_block_tracker *tracker)
{
  return parse_exp_in_context (stringptr, pc, block, flags,
			       tracker, nullptr);
}

/* Parse STRING as an expression, and complain if this fails to use up
   all of the contents of STRING.  TRACKER, if non-null, will be
   updated by the parser.  FLAGS are passed to the parser.  */

expression_up
parse_expression (const char *string, innermost_block_tracker *tracker,
		  parser_flags flags)
{
  expression_up exp = parse_exp_in_context (&string, 0, nullptr, flags,
					    tracker, nullptr);
  if (*string)
    error (_("Junk after end of expression."));
  return exp;
}

/* Same as parse_expression, but using the given language (LANG)
   to parse the expression.  */

expression_up
parse_expression_with_language (const char *string, enum language lang)
{
  std::optional<scoped_restore_current_language> lang_saver;
  if (current_language->la_language != lang)
    {
      lang_saver.emplace ();
      set_language (lang);
    }

  return parse_expression (string);
}

/* Parse STRING as an expression.  If the parse is marked for
   completion, set COMPLETER and return the expression.  In all other
   cases, return NULL.  */

expression_up
parse_expression_for_completion
     (const char *string,
      std::unique_ptr<expr_completion_base> *completer)
{
  expression_up exp;

  try
    {
      exp = parse_exp_in_context (&string, 0, 0, 0, nullptr, completer);
    }
  catch (const gdb_exception_error &except)
    {
      /* Nothing, EXP remains NULL.  */
    }

  /* If we didn't get a completion result, be sure to also not return
     an expression to our caller.  */
  if (*completer == nullptr)
    return nullptr;

  return exp;
}

/* Parse floating point value P of length LEN.
   Return false if invalid, true if valid.
   The successfully parsed number is stored in DATA in
   target format for floating-point type TYPE.

   NOTE: This accepts the floating point syntax that sscanf accepts.  */

bool
parse_float (const char *p, int len,
	     const struct type *type, gdb_byte *data)
{
  return target_float_from_string (data, type, std::string (p, len));
}

/* Return true if the number N_SIGN * N fits in a type with TYPE_BITS and
   TYPE_SIGNED_P.  N_SIGNED is either 1 or -1.  */

bool
fits_in_type (int n_sign, ULONGEST n, int type_bits, bool type_signed_p)
{
  /* Normalize -0.  */
  if (n == 0 && n_sign == -1)
    n_sign = 1;

  if (n_sign == -1 && !type_signed_p)
    /* Can't fit a negative number in an unsigned type.  */
    return false;

  if (type_bits > sizeof (ULONGEST) * 8)
    return true;

  ULONGEST smax = (ULONGEST)1 << (type_bits - 1);
  if (n_sign == -1)
    {
      /* Negative number, signed type.  */
      return (n <= smax);
    }
  else if (n_sign == 1 && type_signed_p)
    {
      /* Positive number, signed type.  */
      return (n < smax);
    }
  else if (n_sign == 1 && !type_signed_p)
    {
      /* Positive number, unsigned type.  */
      return ((n >> 1) >> (type_bits - 1)) == 0;
    }
  else
    gdb_assert_not_reached ("");
}

/* Return true if the number N_SIGN * N fits in a type with TYPE_BITS and
   TYPE_SIGNED_P.  N_SIGNED is either 1 or -1.  */

bool
fits_in_type (int n_sign, const gdb_mpz &n, int type_bits, bool type_signed_p)
{
  /* N must be nonnegative.  */
  gdb_assert (n.sgn () >= 0);

  /* Zero always fits.  */
  /* Normalize -0.  */
  if (n.sgn () == 0)
    return true;

  if (n_sign == -1 && !type_signed_p)
    /* Can't fit a negative number in an unsigned type.  */
    return false;

  gdb_mpz max = gdb_mpz::pow (2, (type_signed_p
				  ? type_bits - 1
				  : type_bits));
  if (n_sign == -1)
    return n <= max;
  return n < max;
}

/* This function avoids direct calls to fprintf 
   in the parser generated debug code.  */
void
parser_fprintf (FILE *x, const char *y, ...)
{ 
  va_list args;

  va_start (args, y);
  if (x == stderr)
    gdb_vprintf (gdb_stderr, y, args); 
  else
    {
      gdb_printf (gdb_stderr, " Unknown FILE used.\n");
      gdb_vprintf (gdb_stderr, y, args);
    }
  va_end (args);
}

void _initialize_parse ();
void
_initialize_parse ()
{
  add_setshow_zuinteger_cmd ("expression", class_maintenance,
			     &expressiondebug,
			     _("Set expression debugging."),
			     _("Show expression debugging."),
			     _("When non-zero, the internal representation "
			       "of expressions will be printed."),
			     NULL,
			     show_expressiondebug,
			     &setdebuglist, &showdebuglist);
  add_setshow_boolean_cmd ("parser", class_maintenance,
			    &parser_debug,
			   _("Set parser debugging."),
			   _("Show parser debugging."),
			   _("When non-zero, expression parser "
			     "tracing will be enabled."),
			    NULL,
			    show_parserdebug,
			    &setdebuglist, &showdebuglist);
}
