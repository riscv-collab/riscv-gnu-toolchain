/* Rust expression parsing for GDB, the GNU debugger.

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

#include "block.h"
#include "charset.h"
#include "cp-support.h"
#include "gdbsupport/gdb_obstack.h"
#include "gdbsupport/gdb_regex.h"
#include "rust-lang.h"
#include "parser-defs.h"
#include "gdbsupport/selftest.h"
#include "value.h"
#include "gdbarch.h"
#include "rust-exp.h"
#include "inferior.h"

using namespace expr;

/* A regular expression for matching Rust numbers.  This is split up
   since it is very long and this gives us a way to comment the
   sections.  */

static const char number_regex_text[] =
  /* subexpression 1: allows use of alternation, otherwise uninteresting */
  "^("
  /* First comes floating point.  */
  /* Recognize number after the decimal point, with optional
     exponent and optional type suffix.
     subexpression 2: allows "?", otherwise uninteresting
     subexpression 3: if present, type suffix
  */
  "[0-9][0-9_]*\\.[0-9][0-9_]*([eE][-+]?[0-9][0-9_]*)?(f32|f64)?"
#define FLOAT_TYPE1 3
  "|"
  /* Recognize exponent without decimal point, with optional type
     suffix.
     subexpression 4: if present, type suffix
  */
#define FLOAT_TYPE2 4
  "[0-9][0-9_]*[eE][-+]?[0-9][0-9_]*(f32|f64)?"
  "|"
  /* "23." is a valid floating point number, but "23.e5" and
     "23.f32" are not.  So, handle the trailing-. case
     separately.  */
  "[0-9][0-9_]*\\."
  "|"
  /* Finally come integers.
     subexpression 5: text of integer
     subexpression 6: if present, type suffix
     subexpression 7: allows use of alternation, otherwise uninteresting
  */
#define INT_TEXT 5
#define INT_TYPE 6
  "(0x[a-fA-F0-9_]+|0o[0-7_]+|0b[01_]+|[0-9][0-9_]*)"
  "([iu](size|8|16|32|64|128))?"
  ")";
/* The number of subexpressions to allocate space for, including the
   "0th" whole match subexpression.  */
#define NUM_SUBEXPRESSIONS 8

/* The compiled number-matching regex.  */

static regex_t number_regex;

/* The kinds of tokens.  Note that single-character tokens are
   represented by themselves, so for instance '[' is a token.  */
enum token_type : int
{
  /* Make sure to start after any ASCII character.  */
  GDBVAR = 256,
  IDENT,
  COMPLETE,
  INTEGER,
  DECIMAL_INTEGER,
  STRING,
  BYTESTRING,
  FLOAT,
  COMPOUND_ASSIGN,

  /* Keyword tokens.  */
  KW_AS,
  KW_IF,
  KW_TRUE,
  KW_FALSE,
  KW_SUPER,
  KW_SELF,
  KW_MUT,
  KW_EXTERN,
  KW_CONST,
  KW_FN,
  KW_SIZEOF,

  /* Operator tokens.  */
  DOTDOT,
  DOTDOTEQ,
  OROR,
  ANDAND,
  EQEQ,
  NOTEQ,
  LTEQ,
  GTEQ,
  LSH,
  RSH,
  COLONCOLON,
  ARROW,
};

/* A typed integer constant.  */

struct typed_val_int
{
  gdb_mpz val;
  struct type *type;
};

/* A typed floating point constant.  */

struct typed_val_float
{
  float_data val;
  struct type *type;
};

/* A struct of this type is used to describe a token.  */

struct token_info
{
  const char *name;
  int value;
  enum exp_opcode opcode;
};

/* Identifier tokens.  */

static const struct token_info identifier_tokens[] =
{
  { "as", KW_AS, OP_NULL },
  { "false", KW_FALSE, OP_NULL },
  { "if", 0, OP_NULL },
  { "mut", KW_MUT, OP_NULL },
  { "const", KW_CONST, OP_NULL },
  { "self", KW_SELF, OP_NULL },
  { "super", KW_SUPER, OP_NULL },
  { "true", KW_TRUE, OP_NULL },
  { "extern", KW_EXTERN, OP_NULL },
  { "fn", KW_FN, OP_NULL },
  { "sizeof", KW_SIZEOF, OP_NULL },
};

/* Operator tokens, sorted longest first.  */

static const struct token_info operator_tokens[] =
{
  { ">>=", COMPOUND_ASSIGN, BINOP_RSH },
  { "<<=", COMPOUND_ASSIGN, BINOP_LSH },

  { "<<", LSH, OP_NULL },
  { ">>", RSH, OP_NULL },
  { "&&", ANDAND, OP_NULL },
  { "||", OROR, OP_NULL },
  { "==", EQEQ, OP_NULL },
  { "!=", NOTEQ, OP_NULL },
  { "<=", LTEQ, OP_NULL },
  { ">=", GTEQ, OP_NULL },
  { "+=", COMPOUND_ASSIGN, BINOP_ADD },
  { "-=", COMPOUND_ASSIGN, BINOP_SUB },
  { "*=", COMPOUND_ASSIGN, BINOP_MUL },
  { "/=", COMPOUND_ASSIGN, BINOP_DIV },
  { "%=", COMPOUND_ASSIGN, BINOP_REM },
  { "&=", COMPOUND_ASSIGN, BINOP_BITWISE_AND },
  { "|=", COMPOUND_ASSIGN, BINOP_BITWISE_IOR },
  { "^=", COMPOUND_ASSIGN, BINOP_BITWISE_XOR },
  { "..=", DOTDOTEQ, OP_NULL },

  { "::", COLONCOLON, OP_NULL },
  { "..", DOTDOT, OP_NULL },
  { "->", ARROW, OP_NULL }
};

/* An instance of this is created before parsing, and destroyed when
   parsing is finished.  */

struct rust_parser
{
  explicit rust_parser (struct parser_state *state)
    : pstate (state)
  {
  }

  DISABLE_COPY_AND_ASSIGN (rust_parser);

  /* Return the parser's language.  */
  const struct language_defn *language () const
  {
    return pstate->language ();
  }

  /* Return the parser's gdbarch.  */
  struct gdbarch *arch () const
  {
    return pstate->gdbarch ();
  }

  /* A helper to look up a Rust type, or fail.  This only works for
     types defined by rust_language_arch_info.  */

  struct type *get_type (const char *name)
  {
    struct type *type;

    type = language_lookup_primitive_type (language (), arch (), name);
    if (type == NULL)
      error (_("Could not find Rust type %s"), name);
    return type;
  }

  std::string crate_name (const std::string &name);
  std::string super_name (const std::string &ident, unsigned int n_supers);

  int lex_character ();
  int lex_number ();
  int lex_string ();
  int lex_identifier ();
  uint32_t lex_hex (int min, int max);
  uint32_t lex_escape (int is_byte);
  int lex_operator ();
  int lex_one_token ();
  void push_back (char c);

  /* The main interface to lexing.  Lexes one token and updates the
     internal state.  */
  void lex ()
  {
    current_token = lex_one_token ();
  }

  /* Assuming the current token is TYPE, lex the next token.  */
  void assume (int type)
  {
    gdb_assert (current_token == type);
    lex ();
  }

  /* Require the single-character token C, and lex the next token; or
     throw an exception.  */
  void require (char type)
  {
    if (current_token != type)
      error (_("'%c' expected"), type);
    lex ();
  }

  /* Entry point for all parsing.  */
  operation_up parse_entry_point ()
  {
    lex ();
    operation_up result = parse_expr ();
    if (current_token != 0)
      error (_("Syntax error near '%s'"), pstate->prev_lexptr);
    return result;
  }

  operation_up parse_tuple ();
  operation_up parse_array ();
  operation_up name_to_operation (const std::string &name);
  operation_up parse_struct_expr (struct type *type);
  operation_up parse_binop (bool required);
  operation_up parse_range ();
  operation_up parse_expr ();
  operation_up parse_sizeof ();
  operation_up parse_addr ();
  operation_up parse_field (operation_up &&);
  operation_up parse_index (operation_up &&);
  std::vector<operation_up> parse_paren_args ();
  operation_up parse_call (operation_up &&);
  std::vector<struct type *> parse_type_list ();
  std::vector<struct type *> parse_maybe_type_list ();
  struct type *parse_array_type ();
  struct type *parse_slice_type ();
  struct type *parse_pointer_type ();
  struct type *parse_function_type ();
  struct type *parse_tuple_type ();
  struct type *parse_type ();
  std::string parse_path (bool for_expr);
  operation_up parse_string ();
  operation_up parse_tuple_struct (struct type *type);
  operation_up parse_path_expr ();
  operation_up parse_atom (bool required);

  void update_innermost_block (struct block_symbol sym);
  struct block_symbol lookup_symbol (const char *name,
				     const struct block *block,
				     const domain_enum domain);
  struct type *rust_lookup_type (const char *name);

  /* Clear some state.  This is only used for testing.  */
#if GDB_SELF_TEST
  void reset (const char *input)
  {
    pstate->prev_lexptr = nullptr;
    pstate->lexptr = input;
    paren_depth = 0;
    current_token = 0;
    current_int_val = {};
    current_float_val = {};
    current_string_val = {};
    current_opcode = OP_NULL;
  }
#endif /* GDB_SELF_TEST */

  /* Return the token's string value as a string.  */
  std::string get_string () const
  {
    return std::string (current_string_val.ptr, current_string_val.length);
  }

  /* A pointer to this is installed globally.  */
  auto_obstack obstack;

  /* The parser state gdb gave us.  */
  struct parser_state *pstate;

  /* Depth of parentheses.  */
  int paren_depth = 0;

  /* The current token's type.  */
  int current_token = 0;
  /* The current token's payload, if any.  */
  typed_val_int current_int_val {};
  typed_val_float current_float_val {};
  struct stoken current_string_val {};
  enum exp_opcode current_opcode = OP_NULL;

  /* When completing, this may be set to the field operation to
     complete.  */
  operation_up completion_op;
};

/* Return an string referring to NAME, but relative to the crate's
   name.  */

std::string
rust_parser::crate_name (const std::string &name)
{
  std::string crate = rust_crate_for_block (pstate->expression_context_block);

  if (crate.empty ())
    error (_("Could not find crate for current location"));
  return "::" + crate + "::" + name;
}

/* Return a string referring to a "super::" qualified name.  IDENT is
   the base name and N_SUPERS is how many "super::"s were provided.
   N_SUPERS can be zero.  */

std::string
rust_parser::super_name (const std::string &ident, unsigned int n_supers)
{
  const char *scope = "";
  if (pstate->expression_context_block != nullptr)
    scope = pstate->expression_context_block->scope ();
  int offset;

  if (scope[0] == '\0')
    error (_("Couldn't find namespace scope for self::"));

  if (n_supers > 0)
    {
      int len;
      std::vector<int> offsets;
      unsigned int current_len;

      current_len = cp_find_first_component (scope);
      while (scope[current_len] != '\0')
	{
	  offsets.push_back (current_len);
	  gdb_assert (scope[current_len] == ':');
	  /* The "::".  */
	  current_len += 2;
	  current_len += cp_find_first_component (scope
						  + current_len);
	}

      len = offsets.size ();
      if (n_supers >= len)
	error (_("Too many super:: uses from '%s'"), scope);

      offset = offsets[len - n_supers];
    }
  else
    offset = strlen (scope);

  return "::" + std::string (scope, offset) + "::" + ident;
}

/* A helper to appropriately munge NAME and BLOCK depending on the
   presence of a leading "::".  */

static void
munge_name_and_block (const char **name, const struct block **block)
{
  /* If it is a global reference, skip the current block in favor of
     the static block.  */
  if (startswith (*name, "::"))
    {
      *name += 2;
      *block = (*block)->static_block ();
    }
}

/* Like lookup_symbol, but handles Rust namespace conventions, and
   doesn't require field_of_this_result.  */

struct block_symbol
rust_parser::lookup_symbol (const char *name, const struct block *block,
			    const domain_enum domain)
{
  struct block_symbol result;

  munge_name_and_block (&name, &block);

  result = ::lookup_symbol (name, block, domain, NULL);
  if (result.symbol != NULL)
    update_innermost_block (result);
  return result;
}

/* Look up a type, following Rust namespace conventions.  */

struct type *
rust_parser::rust_lookup_type (const char *name)
{
  struct block_symbol result;
  struct type *type;

  const struct block *block = pstate->expression_context_block;
  munge_name_and_block (&name, &block);

  result = ::lookup_symbol (name, block, STRUCT_DOMAIN, NULL);
  if (result.symbol != NULL)
    {
      update_innermost_block (result);
      return result.symbol->type ();
    }

  type = lookup_typename (language (), name, NULL, 1);
  if (type != NULL)
    return type;

  /* Last chance, try a built-in type.  */
  return language_lookup_primitive_type (language (), arch (), name);
}

/* A helper that updates the innermost block as appropriate.  */

void
rust_parser::update_innermost_block (struct block_symbol sym)
{
  if (symbol_read_needs_frame (sym.symbol))
    pstate->block_tracker->update (sym);
}

/* Lex a hex number with at least MIN digits and at most MAX
   digits.  */

uint32_t
rust_parser::lex_hex (int min, int max)
{
  uint32_t result = 0;
  int len = 0;
  /* We only want to stop at MAX if we're lexing a byte escape.  */
  int check_max = min == max;

  while ((check_max ? len <= max : 1)
	 && ((pstate->lexptr[0] >= 'a' && pstate->lexptr[0] <= 'f')
	     || (pstate->lexptr[0] >= 'A' && pstate->lexptr[0] <= 'F')
	     || (pstate->lexptr[0] >= '0' && pstate->lexptr[0] <= '9')))
    {
      result *= 16;
      if (pstate->lexptr[0] >= 'a' && pstate->lexptr[0] <= 'f')
	result = result + 10 + pstate->lexptr[0] - 'a';
      else if (pstate->lexptr[0] >= 'A' && pstate->lexptr[0] <= 'F')
	result = result + 10 + pstate->lexptr[0] - 'A';
      else
	result = result + pstate->lexptr[0] - '0';
      ++pstate->lexptr;
      ++len;
    }

  if (len < min)
    error (_("Not enough hex digits seen"));
  if (len > max)
    {
      gdb_assert (min != max);
      error (_("Overlong hex escape"));
    }

  return result;
}

/* Lex an escape.  IS_BYTE is true if we're lexing a byte escape;
   otherwise we're lexing a character escape.  */

uint32_t
rust_parser::lex_escape (int is_byte)
{
  uint32_t result;

  gdb_assert (pstate->lexptr[0] == '\\');
  ++pstate->lexptr;
  switch (pstate->lexptr[0])
    {
    case 'x':
      ++pstate->lexptr;
      result = lex_hex (2, 2);
      break;

    case 'u':
      if (is_byte)
	error (_("Unicode escape in byte literal"));
      ++pstate->lexptr;
      if (pstate->lexptr[0] != '{')
	error (_("Missing '{' in Unicode escape"));
      ++pstate->lexptr;
      result = lex_hex (1, 6);
      /* Could do range checks here.  */
      if (pstate->lexptr[0] != '}')
	error (_("Missing '}' in Unicode escape"));
      ++pstate->lexptr;
      break;

    case 'n':
      result = '\n';
      ++pstate->lexptr;
      break;
    case 'r':
      result = '\r';
      ++pstate->lexptr;
      break;
    case 't':
      result = '\t';
      ++pstate->lexptr;
      break;
    case '\\':
      result = '\\';
      ++pstate->lexptr;
      break;
    case '0':
      result = '\0';
      ++pstate->lexptr;
      break;
    case '\'':
      result = '\'';
      ++pstate->lexptr;
      break;
    case '"':
      result = '"';
      ++pstate->lexptr;
      break;

    default:
      error (_("Invalid escape \\%c in literal"), pstate->lexptr[0]);
    }

  return result;
}

/* A helper for lex_character.  Search forward for the closing single
   quote, then convert the bytes from the host charset to UTF-32.  */

static uint32_t
lex_multibyte_char (const char *text, int *len)
{
  /* Only look a maximum of 5 bytes for the closing quote.  This is
     the maximum for UTF-8.  */
  int quote;
  gdb_assert (text[0] != '\'');
  for (quote = 1; text[quote] != '\0' && text[quote] != '\''; ++quote)
    ;
  *len = quote;
  /* The caller will issue an error.  */
  if (text[quote] == '\0')
    return 0;

  auto_obstack result;
  convert_between_encodings (host_charset (), HOST_UTF32,
			     (const gdb_byte *) text,
			     quote, 1, &result, translit_none);

  int size = obstack_object_size (&result);
  if (size > 4)
    error (_("overlong character literal"));
  uint32_t value;
  memcpy (&value, obstack_finish (&result), size);
  return value;
}

/* Lex a character constant.  */

int
rust_parser::lex_character ()
{
  int is_byte = 0;
  uint32_t value;

  if (pstate->lexptr[0] == 'b')
    {
      is_byte = 1;
      ++pstate->lexptr;
    }
  gdb_assert (pstate->lexptr[0] == '\'');
  ++pstate->lexptr;
  if (pstate->lexptr[0] == '\'')
    error (_("empty character literal"));
  else if (pstate->lexptr[0] == '\\')
    value = lex_escape (is_byte);
  else
    {
      int len;
      value = lex_multibyte_char (&pstate->lexptr[0], &len);
      pstate->lexptr += len;
    }

  if (pstate->lexptr[0] != '\'')
    error (_("Unterminated character literal"));
  ++pstate->lexptr;

  current_int_val.val = value;
  current_int_val.type = get_type (is_byte ? "u8" : "char");

  return INTEGER;
}

/* Return the offset of the double quote if STR looks like the start
   of a raw string, or 0 if STR does not start a raw string.  */

static int
starts_raw_string (const char *str)
{
  const char *save = str;

  if (str[0] != 'r')
    return 0;
  ++str;
  while (str[0] == '#')
    ++str;
  if (str[0] == '"')
    return str - save;
  return 0;
}

/* Return true if STR looks like the end of a raw string that had N
   hashes at the start.  */

static bool
ends_raw_string (const char *str, int n)
{
  int i;

  gdb_assert (str[0] == '"');
  for (i = 0; i < n; ++i)
    if (str[i + 1] != '#')
      return false;
  return true;
}

/* Lex a string constant.  */

int
rust_parser::lex_string ()
{
  int is_byte = pstate->lexptr[0] == 'b';
  int raw_length;

  if (is_byte)
    ++pstate->lexptr;
  raw_length = starts_raw_string (pstate->lexptr);
  pstate->lexptr += raw_length;
  gdb_assert (pstate->lexptr[0] == '"');
  ++pstate->lexptr;

  while (1)
    {
      uint32_t value;

      if (raw_length > 0)
	{
	  if (pstate->lexptr[0] == '"' && ends_raw_string (pstate->lexptr,
							   raw_length - 1))
	    {
	      /* Exit with lexptr pointing after the final "#".  */
	      pstate->lexptr += raw_length;
	      break;
	    }
	  else if (pstate->lexptr[0] == '\0')
	    error (_("Unexpected EOF in string"));

	  value = pstate->lexptr[0] & 0xff;
	  if (is_byte && value > 127)
	    error (_("Non-ASCII value in raw byte string"));
	  obstack_1grow (&obstack, value);

	  ++pstate->lexptr;
	}
      else if (pstate->lexptr[0] == '"')
	{
	  /* Make sure to skip the quote.  */
	  ++pstate->lexptr;
	  break;
	}
      else if (pstate->lexptr[0] == '\\')
	{
	  value = lex_escape (is_byte);

	  if (is_byte)
	    obstack_1grow (&obstack, value);
	  else
	    convert_between_encodings (HOST_UTF32, "UTF-8",
				       (gdb_byte *) &value,
				       sizeof (value), sizeof (value),
				       &obstack, translit_none);
	}
      else if (pstate->lexptr[0] == '\0')
	error (_("Unexpected EOF in string"));
      else
	{
	  value = pstate->lexptr[0] & 0xff;
	  if (is_byte && value > 127)
	    error (_("Non-ASCII value in byte string"));
	  obstack_1grow (&obstack, value);
	  ++pstate->lexptr;
	}
    }

  current_string_val.length = obstack_object_size (&obstack);
  current_string_val.ptr = (const char *) obstack_finish (&obstack);
  return is_byte ? BYTESTRING : STRING;
}

/* Return true if STRING starts with whitespace followed by a digit.  */

static bool
space_then_number (const char *string)
{
  const char *p = string;

  while (p[0] == ' ' || p[0] == '\t')
    ++p;
  if (p == string)
    return false;

  return *p >= '0' && *p <= '9';
}

/* Return true if C can start an identifier.  */

static bool
rust_identifier_start_p (char c)
{
  return ((c >= 'a' && c <= 'z')
	  || (c >= 'A' && c <= 'Z')
	  || c == '_'
	  || c == '$'
	  /* Allow any non-ASCII character as an identifier.  There
	     doesn't seem to be a need to be picky about this.  */
	  || (c & 0x80) != 0);
}

/* Lex an identifier.  */

int
rust_parser::lex_identifier ()
{
  unsigned int length;
  const struct token_info *token;
  int is_gdb_var = pstate->lexptr[0] == '$';

  bool is_raw = false;
  if (pstate->lexptr[0] == 'r'
      && pstate->lexptr[1] == '#'
      && rust_identifier_start_p (pstate->lexptr[2]))
    {
      is_raw = true;
      pstate->lexptr += 2;
    }

  const char *start = pstate->lexptr;
  gdb_assert (rust_identifier_start_p (pstate->lexptr[0]));

  ++pstate->lexptr;

  /* Allow any non-ASCII character here.  This "handles" UTF-8 by
     passing it through.  */
  while ((pstate->lexptr[0] >= 'a' && pstate->lexptr[0] <= 'z')
	 || (pstate->lexptr[0] >= 'A' && pstate->lexptr[0] <= 'Z')
	 || pstate->lexptr[0] == '_'
	 || (is_gdb_var && pstate->lexptr[0] == '$')
	 || (pstate->lexptr[0] >= '0' && pstate->lexptr[0] <= '9')
	 || (pstate->lexptr[0] & 0x80) != 0)
    ++pstate->lexptr;


  length = pstate->lexptr - start;
  token = NULL;
  if (!is_raw)
    {
      for (const auto &candidate : identifier_tokens)
	{
	  if (length == strlen (candidate.name)
	      && strncmp (candidate.name, start, length) == 0)
	    {
	      token = &candidate;
	      break;
	    }
	}
    }

  if (token != NULL)
    {
      if (token->value == 0)
	{
	  /* Leave the terminating token alone.  */
	  pstate->lexptr = start;
	  return 0;
	}
    }
  else if (token == NULL
	   && !is_raw
	   && (strncmp (start, "thread", length) == 0
	       || strncmp (start, "task", length) == 0)
	   && space_then_number (pstate->lexptr))
    {
      /* "task" or "thread" followed by a number terminates the
	 parse, per gdb rules.  */
      pstate->lexptr = start;
      return 0;
    }

  if (token == NULL || (pstate->parse_completion && pstate->lexptr[0] == '\0'))
    {
      current_string_val.length = length;
      current_string_val.ptr = start;
    }

  if (pstate->parse_completion && pstate->lexptr[0] == '\0')
    {
      /* Prevent rustyylex from returning two COMPLETE tokens.  */
      pstate->prev_lexptr = pstate->lexptr;
      return COMPLETE;
    }

  if (token != NULL)
    return token->value;
  if (is_gdb_var)
    return GDBVAR;
  return IDENT;
}

/* Lex an operator.  */

int
rust_parser::lex_operator ()
{
  const struct token_info *token = NULL;

  for (const auto &candidate : operator_tokens)
    {
      if (strncmp (candidate.name, pstate->lexptr,
		   strlen (candidate.name)) == 0)
	{
	  pstate->lexptr += strlen (candidate.name);
	  token = &candidate;
	  break;
	}
    }

  if (token != NULL)
    {
      current_opcode = token->opcode;
      return token->value;
    }

  return *pstate->lexptr++;
}

/* Lex a number.  */

int
rust_parser::lex_number ()
{
  regmatch_t subexps[NUM_SUBEXPRESSIONS];
  int match;
  int is_integer = 0;
  int could_be_decimal = 1;
  int implicit_i32 = 0;
  const char *type_name = NULL;
  struct type *type;
  int end_index;
  int type_index = -1;
  int i;

  match = regexec (&number_regex, pstate->lexptr, ARRAY_SIZE (subexps),
		   subexps, 0);
  /* Failure means the regexp is broken.  */
  gdb_assert (match == 0);

  if (subexps[INT_TEXT].rm_so != -1)
    {
      /* Integer part matched.  */
      is_integer = 1;
      end_index = subexps[INT_TEXT].rm_eo;
      if (subexps[INT_TYPE].rm_so == -1)
	{
	  type_name = "i32";
	  implicit_i32 = 1;
	}
      else
	{
	  type_index = INT_TYPE;
	  could_be_decimal = 0;
	}
    }
  else if (subexps[FLOAT_TYPE1].rm_so != -1)
    {
      /* Found floating point type suffix.  */
      end_index = subexps[FLOAT_TYPE1].rm_so;
      type_index = FLOAT_TYPE1;
    }
  else if (subexps[FLOAT_TYPE2].rm_so != -1)
    {
      /* Found floating point type suffix.  */
      end_index = subexps[FLOAT_TYPE2].rm_so;
      type_index = FLOAT_TYPE2;
    }
  else
    {
      /* Any other floating point match.  */
      end_index = subexps[0].rm_eo;
      type_name = "f64";
    }

  /* We need a special case if the final character is ".".  In this
     case we might need to parse an integer.  For example, "23.f()" is
     a request for a trait method call, not a syntax error involving
     the floating point number "23.".  */
  gdb_assert (subexps[0].rm_eo > 0);
  if (pstate->lexptr[subexps[0].rm_eo - 1] == '.')
    {
      const char *next = skip_spaces (&pstate->lexptr[subexps[0].rm_eo]);

      if (rust_identifier_start_p (*next) || *next == '.')
	{
	  --subexps[0].rm_eo;
	  is_integer = 1;
	  end_index = subexps[0].rm_eo;
	  type_name = "i32";
	  could_be_decimal = 1;
	  implicit_i32 = 1;
	}
    }

  /* Compute the type name if we haven't already.  */
  std::string type_name_holder;
  if (type_name == NULL)
    {
      gdb_assert (type_index != -1);
      type_name_holder = std::string ((pstate->lexptr
				       + subexps[type_index].rm_so),
				      (subexps[type_index].rm_eo
				       - subexps[type_index].rm_so));
      type_name = type_name_holder.c_str ();
    }

  /* Look up the type.  */
  type = get_type (type_name);

  /* Copy the text of the number and remove the "_"s.  */
  std::string number;
  for (i = 0; i < end_index && pstate->lexptr[i]; ++i)
    {
      if (pstate->lexptr[i] == '_')
	could_be_decimal = 0;
      else
	number.push_back (pstate->lexptr[i]);
    }

  /* Advance past the match.  */
  pstate->lexptr += subexps[0].rm_eo;

  /* Parse the number.  */
  if (is_integer)
    {
      int radix = 10;
      int offset = 0;

      if (number[0] == '0')
	{
	  if (number[1] == 'x')
	    radix = 16;
	  else if (number[1] == 'o')
	    radix = 8;
	  else if (number[1] == 'b')
	    radix = 2;
	  if (radix != 10)
	    {
	      offset = 2;
	      could_be_decimal = 0;
	    }
	}

      if (!current_int_val.val.set (number.c_str () + offset, radix))
	{
	  /* Shouldn't be possible.  */
	  error (_("Invalid integer"));
	}
      if (implicit_i32)
	{
	  static gdb_mpz sixty_three_bit = gdb_mpz::pow (2, 63);
	  static gdb_mpz thirty_one_bit = gdb_mpz::pow (2, 31);

	  if (current_int_val.val >= sixty_three_bit)
	    type = get_type ("i128");
	  else if (current_int_val.val >= thirty_one_bit)
	    type = get_type ("i64");
	}

      current_int_val.type = type;
    }
  else
    {
      current_float_val.type = type;
      bool parsed = parse_float (number.c_str (), number.length (),
				 current_float_val.type,
				 current_float_val.val.data ());
      gdb_assert (parsed);
    }

  return is_integer ? (could_be_decimal ? DECIMAL_INTEGER : INTEGER) : FLOAT;
}

/* The lexer.  */

int
rust_parser::lex_one_token ()
{
  /* Skip all leading whitespace.  */
  while (pstate->lexptr[0] == ' '
	 || pstate->lexptr[0] == '\t'
	 || pstate->lexptr[0] == '\r'
	 || pstate->lexptr[0] == '\n')
    ++pstate->lexptr;

  /* If we hit EOF and we're completing, then return COMPLETE -- maybe
     we're completing an empty string at the end of a field_expr.
     But, we don't want to return two COMPLETE tokens in a row.  */
  if (pstate->lexptr[0] == '\0' && pstate->lexptr == pstate->prev_lexptr)
    return 0;
  pstate->prev_lexptr = pstate->lexptr;
  if (pstate->lexptr[0] == '\0')
    {
      if (pstate->parse_completion)
	{
	  current_string_val.length =0;
	  current_string_val.ptr = "";
	  return COMPLETE;
	}
      return 0;
    }

  if (pstate->lexptr[0] >= '0' && pstate->lexptr[0] <= '9')
    return lex_number ();
  else if (pstate->lexptr[0] == 'b' && pstate->lexptr[1] == '\'')
    return lex_character ();
  else if (pstate->lexptr[0] == 'b' && pstate->lexptr[1] == '"')
    return lex_string ();
  else if (pstate->lexptr[0] == 'b' && starts_raw_string (pstate->lexptr + 1))
    return lex_string ();
  else if (starts_raw_string (pstate->lexptr))
    return lex_string ();
  else if (rust_identifier_start_p (pstate->lexptr[0]))
    return lex_identifier ();
  else if (pstate->lexptr[0] == '"')
    return lex_string ();
  else if (pstate->lexptr[0] == '\'')
    return lex_character ();
  else if (pstate->lexptr[0] == '}' || pstate->lexptr[0] == ']')
    {
      /* Falls through to lex_operator.  */
      --paren_depth;
    }
  else if (pstate->lexptr[0] == '(' || pstate->lexptr[0] == '{')
    {
      /* Falls through to lex_operator.  */
      ++paren_depth;
    }
  else if (pstate->lexptr[0] == ',' && pstate->comma_terminates
	   && paren_depth == 0)
    return 0;

  return lex_operator ();
}

/* Push back a single character to be re-lexed.  */

void
rust_parser::push_back (char c)
{
  /* Can't be called before any lexing.  */
  gdb_assert (pstate->prev_lexptr != NULL);

  --pstate->lexptr;
  gdb_assert (*pstate->lexptr == c);
}



/* Parse a tuple or paren expression.  */

operation_up
rust_parser::parse_tuple ()
{
  assume ('(');

  if (current_token == ')')
    {
      lex ();
      struct type *unit = get_type ("()");
      return make_operation<long_const_operation> (unit, 0);
    }

  operation_up expr = parse_expr ();
  if (current_token == ')')
    {
      /* Parenthesized expression.  */
      lex ();
      return make_operation<rust_parenthesized_operation> (std::move (expr));
    }

  std::vector<operation_up> ops;
  ops.push_back (std::move (expr));
  while (current_token != ')')
    {
      if (current_token != ',')
	error (_("',' or ')' expected"));
      lex ();

      /* A trailing "," is ok.  */
      if (current_token != ')')
	ops.push_back (parse_expr ());
    }

  assume (')');

  error (_("Tuple expressions not supported yet"));
}

/* Parse an array expression.  */

operation_up
rust_parser::parse_array ()
{
  assume ('[');

  if (current_token == KW_MUT)
    lex ();

  operation_up result;
  operation_up expr = parse_expr ();
  if (current_token == ';')
    {
      lex ();
      operation_up rhs = parse_expr ();
      result = make_operation<rust_array_operation> (std::move (expr),
						     std::move (rhs));
    }
  else if (current_token == ',' || current_token == ']')
    {
      std::vector<operation_up> ops;
      ops.push_back (std::move (expr));
      while (current_token != ']')
	{
	  if (current_token != ',')
	    error (_("',' or ']' expected"));
	  lex ();
	  ops.push_back (parse_expr ());
	}
      ops.shrink_to_fit ();
      int len = ops.size () - 1;
      result = make_operation<array_operation> (0, len, std::move (ops));
    }
  else
    error (_("',', ';', or ']' expected"));

  require (']');

  return result;
}

/* Turn a name into an operation.  */

operation_up
rust_parser::name_to_operation (const std::string &name)
{
  struct block_symbol sym = lookup_symbol (name.c_str (),
					   pstate->expression_context_block,
					   VAR_DOMAIN);
  if (sym.symbol != nullptr && sym.symbol->aclass () != LOC_TYPEDEF)
    return make_operation<var_value_operation> (sym);

  struct type *type = nullptr;

  if (sym.symbol != nullptr)
    {
      gdb_assert (sym.symbol->aclass () == LOC_TYPEDEF);
      type = sym.symbol->type ();
    }
  if (type == nullptr)
    type = rust_lookup_type (name.c_str ());
  if (type == nullptr)
    error (_("No symbol '%s' in current context"), name.c_str ());

  if (type->code () == TYPE_CODE_STRUCT && type->num_fields () == 0)
    {
      /* A unit-like struct.  */
      operation_up result (new rust_aggregate_operation (type, {}, {}));
      return result;
    }
  else
    return make_operation<type_operation> (type);
}

/* Parse a struct expression.  */

operation_up
rust_parser::parse_struct_expr (struct type *type)
{
  assume ('{');

  if (type->code () != TYPE_CODE_STRUCT
      || rust_tuple_type_p (type)
      || rust_tuple_struct_type_p (type))
    error (_("Struct expression applied to non-struct type"));

  std::vector<std::pair<std::string, operation_up>> field_v;
  while (current_token != '}' && current_token != DOTDOT)
    {
      if (current_token != IDENT)
	error (_("'}', '..', or identifier expected"));

      std::string name = get_string ();
      lex ();

      operation_up expr;
      if (current_token == ',' || current_token == '}'
	  || current_token == DOTDOT)
	expr = name_to_operation (name);
      else
	{
	  require (':');
	  expr = parse_expr ();
	}
      field_v.emplace_back (std::move (name), std::move (expr));

      /* A trailing "," is ok.  */
      if (current_token == ',')
	lex ();
    }

  operation_up others;
  if (current_token == DOTDOT)
    {
      lex ();
      others = parse_expr ();
    }

  require ('}');

  return make_operation<rust_aggregate_operation> (type,
						   std::move (others),
						   std::move (field_v));
}

/* Used by the operator precedence parser.  */
struct rustop_item
{
  rustop_item (int token_, int precedence_, enum exp_opcode opcode_,
	       operation_up &&op_)
    : token (token_),
      precedence (precedence_),
      opcode (opcode_),
      op (std::move (op_))
  {
  }

  /* The token value.  */
  int token;
  /* Precedence of this operator.  */
  int precedence;
  /* This is used only for assign-modify.  */
  enum exp_opcode opcode;
  /* The right hand side of this operation.  */
  operation_up op;
};

/* An operator precedence parser for binary operations, including
   "as".  */

operation_up
rust_parser::parse_binop (bool required)
{
  /* All the binary  operators.  Each one is of the form
     OPERATION(TOKEN, PRECEDENCE, TYPE)
     TOKEN is the corresponding operator token.
     PRECEDENCE is a value indicating relative precedence.
     TYPE is the operation type corresponding to the operator.
     Assignment operations are handled specially, not via this
     table; they have precedence 0.  */
#define ALL_OPS					\
  OPERATION ('*', 10, mul_operation)		\
  OPERATION ('/', 10, div_operation)		\
  OPERATION ('%', 10, rem_operation)		\
  OPERATION ('@', 9, repeat_operation)		\
  OPERATION ('+', 8, add_operation)		\
  OPERATION ('-', 8, sub_operation)		\
  OPERATION (LSH, 7, lsh_operation)		\
  OPERATION (RSH, 7, rsh_operation)		\
  OPERATION ('&', 6, bitwise_and_operation)	\
  OPERATION ('^', 5, bitwise_xor_operation)	\
  OPERATION ('|', 4, bitwise_ior_operation)	\
  OPERATION (EQEQ, 3, equal_operation)		\
  OPERATION (NOTEQ, 3, notequal_operation)	\
  OPERATION ('<', 3, less_operation)		\
  OPERATION (LTEQ, 3, leq_operation)		\
  OPERATION ('>', 3, gtr_operation)		\
  OPERATION (GTEQ, 3, geq_operation)		\
  OPERATION (ANDAND, 2, logical_and_operation)	\
  OPERATION (OROR, 1, logical_or_operation)

#define ASSIGN_PREC 0

  operation_up start = parse_atom (required);
  if (start == nullptr)
    {
      gdb_assert (!required);
      return start;
    }

  std::vector<rustop_item> operator_stack;
  operator_stack.emplace_back (0, -1, OP_NULL, std::move (start));

  while (true)
    {
      int this_token = current_token;
      enum exp_opcode compound_assign_op = OP_NULL;
      int precedence = -2;

      switch (this_token)
	{
#define OPERATION(TOKEN, PRECEDENCE, TYPE)		\
	  case TOKEN:				\
	    precedence = PRECEDENCE;		\
	    lex ();				\
	    break;

	  ALL_OPS

#undef OPERATION

	case COMPOUND_ASSIGN:
	  compound_assign_op = current_opcode;
	  [[fallthrough]];
	case '=':
	  precedence = ASSIGN_PREC;
	  lex ();
	  break;

	  /* "as" must be handled specially.  */
	case KW_AS:
	  {
	    lex ();
	    rustop_item &lhs = operator_stack.back ();
	    struct type *type = parse_type ();
	    lhs.op = make_operation<unop_cast_operation> (std::move (lhs.op),
							  type);
	  }
	  /* Bypass the rest of the loop.  */
	  continue;

	default:
	  /* Arrange to pop the entire stack.  */
	  precedence = -2;
	  break;
	}

      /* Make sure that assignments are right-associative while other
	 operations are left-associative.  */
      while ((precedence == ASSIGN_PREC
	      ? precedence < operator_stack.back ().precedence
	      : precedence <= operator_stack.back ().precedence)
	     && operator_stack.size () > 1)
	{
	  rustop_item rhs = std::move (operator_stack.back ());
	  operator_stack.pop_back ();

	  rustop_item &lhs = operator_stack.back ();

	  switch (rhs.token)
	    {
#define OPERATION(TOKEN, PRECEDENCE, TYPE)			\
	  case TOKEN:						\
	    lhs.op = make_operation<TYPE> (std::move (lhs.op),	\
					   std::move (rhs.op)); \
	    break;

	      ALL_OPS

#undef OPERATION

	    case '=':
	    case COMPOUND_ASSIGN:
	      {
		if (rhs.token == '=')
		  lhs.op = (make_operation<assign_operation>
			    (std::move (lhs.op), std::move (rhs.op)));
		else
		  lhs.op = (make_operation<assign_modify_operation>
			    (rhs.opcode, std::move (lhs.op),
			     std::move (rhs.op)));

		struct type *unit_type = get_type ("()");

		operation_up nil (new long_const_operation (unit_type, 0));
		lhs.op = (make_operation<comma_operation>
			  (std::move (lhs.op), std::move (nil)));
	      }
	      break;

	    default:
	      gdb_assert_not_reached ("bad binary operator");
	    }
	}

      if (precedence == -2)
	break;

      operator_stack.emplace_back (this_token, precedence, compound_assign_op,
				   parse_atom (true));
    }

  gdb_assert (operator_stack.size () == 1);
  return std::move (operator_stack[0].op);
#undef ALL_OPS
}

/* Parse a range expression.  */

operation_up
rust_parser::parse_range ()
{
  enum range_flag kind = (RANGE_HIGH_BOUND_DEFAULT
			  | RANGE_LOW_BOUND_DEFAULT);

  operation_up lhs;
  if (current_token != DOTDOT && current_token != DOTDOTEQ)
    {
      lhs = parse_binop (true);
      kind &= ~RANGE_LOW_BOUND_DEFAULT;
    }

  if (current_token == DOTDOT)
    kind |= RANGE_HIGH_BOUND_EXCLUSIVE;
  else if (current_token != DOTDOTEQ)
    return lhs;
  lex ();

  /* A "..=" range requires a high bound, but otherwise it is
     optional.  */
  operation_up rhs = parse_binop ((kind & RANGE_HIGH_BOUND_EXCLUSIVE) == 0);
  if (rhs != nullptr)
    kind &= ~RANGE_HIGH_BOUND_DEFAULT;

  return make_operation<rust_range_operation> (kind,
					       std::move (lhs),
					       std::move (rhs));
}

/* Parse an expression.  */

operation_up
rust_parser::parse_expr ()
{
  return parse_range ();
}

/* Parse a sizeof expression.  */

operation_up
rust_parser::parse_sizeof ()
{
  assume (KW_SIZEOF);

  require ('(');
  operation_up result = make_operation<unop_sizeof_operation> (parse_expr ());
  require (')');
  return result;
}

/* Parse an address-of operation.  */

operation_up
rust_parser::parse_addr ()
{
  assume ('&');

  if (current_token == KW_MUT)
    lex ();

  return make_operation<rust_unop_addr_operation> (parse_atom (true));
}

/* Parse a field expression.  */

operation_up
rust_parser::parse_field (operation_up &&lhs)
{
  assume ('.');

  operation_up result;
  switch (current_token)
    {
    case IDENT:
    case COMPLETE:
      {
	bool is_complete = current_token == COMPLETE;
	auto struct_op = new rust_structop (std::move (lhs), get_string ());
	lex ();
	if (is_complete)
	  {
	    completion_op.reset (struct_op);
	    pstate->mark_struct_expression (struct_op);
	    /* Throw to the outermost level of the parser.  */
	    error (_("not really an error"));
	  }
	result.reset (struct_op);
      }
      break;

    case DECIMAL_INTEGER:
      {
	int idx = current_int_val.val.as_integer<int> ();
	result = make_operation<rust_struct_anon> (idx, std::move (lhs));
	lex ();
      }
      break;

    case INTEGER:
      error (_("'_' not allowed in integers in anonymous field references"));

    default:
      error (_("field name expected"));
    }

  return result;
}

/* Parse an index expression.  */

operation_up
rust_parser::parse_index (operation_up &&lhs)
{
  assume ('[');
  operation_up rhs = parse_expr ();
  require (']');

  return make_operation<rust_subscript_operation> (std::move (lhs),
						   std::move (rhs));
}

/* Parse a sequence of comma-separated expressions in parens.  */

std::vector<operation_up>
rust_parser::parse_paren_args ()
{
  assume ('(');

  std::vector<operation_up> args;
  while (current_token != ')')
    {
      if (!args.empty ())
	{
	  if (current_token != ',')
	    error (_("',' or ')' expected"));
	  lex ();
	}

      args.push_back (parse_expr ());
    }

  assume (')');

  return args;
}

/* Parse the parenthesized part of a function call.  */

operation_up
rust_parser::parse_call (operation_up &&lhs)
{
  std::vector<operation_up> args = parse_paren_args ();

  return make_operation<funcall_operation> (std::move (lhs),
					    std::move (args));
}

/* Parse a list of types.  */

std::vector<struct type *>
rust_parser::parse_type_list ()
{
  std::vector<struct type *> result;
  result.push_back (parse_type ());
  while (current_token == ',')
    {
      lex ();
      result.push_back (parse_type ());
    }
  return result;
}

/* Parse a possibly-empty list of types, surrounded in parens.  */

std::vector<struct type *>
rust_parser::parse_maybe_type_list ()
{
  assume ('(');
  std::vector<struct type *> types;
  if (current_token != ')')
    types = parse_type_list ();
  require (')');
  return types;
}

/* Parse an array type.  */

struct type *
rust_parser::parse_array_type ()
{
  assume ('[');
  struct type *elt_type = parse_type ();
  require (';');

  if (current_token != INTEGER && current_token != DECIMAL_INTEGER)
    error (_("integer expected"));
  ULONGEST val = current_int_val.val.as_integer<ULONGEST> ();
  lex ();
  require (']');

  return lookup_array_range_type (elt_type, 0, val - 1);
}

/* Parse a slice type.  */

struct type *
rust_parser::parse_slice_type ()
{
  assume ('&');

  /* Handle &str specially.  This is an important type in Rust.  While
     the compiler does emit the "&str" type in the DWARF, just "str"
     itself isn't always available -- but it's handy if this works
     seamlessly.  */
  if (current_token == IDENT && get_string () == "str")
    {
      lex ();
      return rust_slice_type ("&str", get_type ("u8"), get_type ("usize"));
    }

  bool is_slice = current_token == '[';
  if (is_slice)
    lex ();

  struct type *target = parse_type ();

  if (is_slice)
    {
      require (']');
      return rust_slice_type ("&[*gdb*]", target, get_type ("usize"));
    }

  /* For now we treat &x and *x identically.  */
  return lookup_pointer_type (target);
}

/* Parse a pointer type.  */

struct type *
rust_parser::parse_pointer_type ()
{
  assume ('*');

  if (current_token == KW_MUT || current_token == KW_CONST)
    lex ();

  struct type *target = parse_type ();
  /* For the time being we ignore mut/const.  */
  return lookup_pointer_type (target);
}

/* Parse a function type.  */

struct type *
rust_parser::parse_function_type ()
{
  assume (KW_FN);

  if (current_token != '(')
    error (_("'(' expected"));

  std::vector<struct type *> types = parse_maybe_type_list ();

  if (current_token != ARROW)
    error (_("'->' expected"));
  lex ();

  struct type *result_type = parse_type ();

  struct type **argtypes = nullptr;
  if (!types.empty ())
    argtypes = types.data ();

  result_type = lookup_function_type_with_arguments (result_type,
						     types.size (),
						     argtypes);
  return lookup_pointer_type (result_type);
}

/* Parse a tuple type.  */

struct type *
rust_parser::parse_tuple_type ()
{
  std::vector<struct type *> types = parse_maybe_type_list ();

  auto_obstack obstack;
  obstack_1grow (&obstack, '(');
  for (int i = 0; i < types.size (); ++i)
    {
      std::string type_name = type_to_string (types[i]);

      if (i > 0)
	obstack_1grow (&obstack, ',');
      obstack_grow_str (&obstack, type_name.c_str ());
    }

  obstack_grow_str0 (&obstack, ")");
  const char *name = (const char *) obstack_finish (&obstack);

  /* We don't allow creating new tuple types (yet), but we do allow
     looking up existing tuple types.  */
  struct type *result = rust_lookup_type (name);
  if (result == nullptr)
    error (_("could not find tuple type '%s'"), name);

  return result;
}

/* Parse a type.  */

struct type *
rust_parser::parse_type ()
{
  switch (current_token)
    {
    case '[':
      return parse_array_type ();
    case '&':
      return parse_slice_type ();
    case '*':
      return parse_pointer_type ();
    case KW_FN:
      return parse_function_type ();
    case '(':
      return parse_tuple_type ();
    case KW_SELF:
    case KW_SUPER:
    case COLONCOLON:
    case KW_EXTERN:
    case IDENT:
      {
	std::string path = parse_path (false);
	struct type *result = rust_lookup_type (path.c_str ());
	if (result == nullptr)
	  error (_("No type name '%s' in current context"), path.c_str ());
	return result;
      }
    default:
      error (_("type expected"));
    }
}

/* Parse a path.  */

std::string
rust_parser::parse_path (bool for_expr)
{
  unsigned n_supers = 0;
  int first_token = current_token;

  switch (current_token)
    {
    case KW_SELF:
      lex ();
      if (current_token != COLONCOLON)
	return "self";
      lex ();
      [[fallthrough]];
    case KW_SUPER:
      while (current_token == KW_SUPER)
	{
	  ++n_supers;
	  lex ();
	  if (current_token != COLONCOLON)
	    error (_("'::' expected"));
	  lex ();
	}
      break;

    case COLONCOLON:
      lex ();
      break;

    case KW_EXTERN:
      /* This is a gdb extension to make it possible to refer to items
	 in other crates.  It just bypasses adding the current crate
	 to the front of the name.  */
      lex ();
      break;
    }

  if (current_token != IDENT)
    error (_("identifier expected"));
  std::string path = get_string ();
  bool saw_ident = true;
  lex ();

  /* The condition here lets us enter the loop even if we see
     "ident<...>".  */
  while (current_token == COLONCOLON || current_token == '<')
    {
      if (current_token == COLONCOLON)
	{
	  lex ();
	  saw_ident = false;

	  if (current_token == IDENT)
	    {
	      path = path + "::" + get_string ();
	      lex ();
	      saw_ident = true;
	    }
	  else if (current_token == COLONCOLON)
	    {
	      /* The code below won't detect this scenario.  */
	      error (_("unexpected '::'"));
	    }
	}

      if (current_token != '<')
	continue;

      /* Expression use name::<...>, whereas types use name<...>.  */
      if (for_expr)
	{
	  /* Expressions use "name::<...>", so if we saw an identifier
	     after the "::", we ignore the "<" here.  */
	  if (saw_ident)
	    break;
	}
      else
	{
	  /* Types use "name<...>", so we need to have seen the
	     identifier.  */
	  if (!saw_ident)
	    break;
	}

      lex ();
      std::vector<struct type *> types = parse_type_list ();
      if (current_token == '>')
	lex ();
      else if (current_token == RSH)
	{
	  push_back ('>');
	  lex ();
	}
      else
	error (_("'>' expected"));

      path += "<";
      for (int i = 0; i < types.size (); ++i)
	{
	  if (i > 0)
	    path += ",";
	  path += type_to_string (types[i]);
	}
      path += ">";
      break;
    }

  switch (first_token)
    {
    case KW_SELF:
    case KW_SUPER:
      return super_name (path, n_supers);

    case COLONCOLON:
      return crate_name (path);

    case KW_EXTERN:
      return "::" + path;

    case IDENT:
      return path;

    default:
      gdb_assert_not_reached ("missing case in path parsing");
    }
}

/* Handle the parsing for a string expression.  */

operation_up
rust_parser::parse_string ()
{
  gdb_assert (current_token == STRING);

  /* Wrap the raw string in the &str struct.  */
  struct type *type = rust_lookup_type ("&str");
  if (type == nullptr)
    error (_("Could not find type '&str'"));

  std::vector<std::pair<std::string, operation_up>> field_v;

  size_t len = current_string_val.length;
  operation_up str = make_operation<string_operation> (get_string ());
  operation_up addr
    = make_operation<rust_unop_addr_operation> (std::move (str));
  field_v.emplace_back ("data_ptr", std::move (addr));

  struct type *valtype = get_type ("usize");
  operation_up lenop = make_operation<long_const_operation> (valtype, len);
  field_v.emplace_back ("length", std::move (lenop));

  return make_operation<rust_aggregate_operation> (type,
						   operation_up (),
						   std::move (field_v));
}

/* Parse a tuple struct expression.  */

operation_up
rust_parser::parse_tuple_struct (struct type *type)
{
  std::vector<operation_up> args = parse_paren_args ();

  std::vector<std::pair<std::string, operation_up>> field_v (args.size ());
  for (int i = 0; i < args.size (); ++i)
    field_v[i] = { string_printf ("__%d", i), std::move (args[i]) };

  return (make_operation<rust_aggregate_operation>
	  (type, operation_up (), std::move (field_v)));
}

/* Parse a path expression.  */

operation_up
rust_parser::parse_path_expr ()
{
  std::string path = parse_path (true);

  if (current_token == '{')
    {
      struct type *type = rust_lookup_type (path.c_str ());
      if (type == nullptr)
	error (_("Could not find type '%s'"), path.c_str ());
      
      return parse_struct_expr (type);
    }
  else if (current_token == '(')
    {
      struct type *type = rust_lookup_type (path.c_str ());
      /* If this is actually a tuple struct expression, handle it
	 here.  If it is a call, it will be handled elsewhere.  */
      if (type != nullptr)
	{
	  if (!rust_tuple_struct_type_p (type))
	    error (_("Type %s is not a tuple struct"), path.c_str ());
	  return parse_tuple_struct (type);
	}
    }

  return name_to_operation (path);
}

/* Parse an atom.  "Atom" isn't a Rust term, but this refers to a
   single unitary item in the grammar; but here including some unary
   prefix and postfix expressions.  */

operation_up
rust_parser::parse_atom (bool required)
{
  operation_up result;

  switch (current_token)
    {
    case '(':
      result = parse_tuple ();
      break;

    case '[':
      result = parse_array ();
      break;

    case INTEGER:
    case DECIMAL_INTEGER:
      result = make_operation<long_const_operation> (current_int_val.type,
						     current_int_val.val);
      lex ();
      break;

    case FLOAT:
      result = make_operation<float_const_operation> (current_float_val.type,
						      current_float_val.val);
      lex ();
      break;

    case STRING:
      result = parse_string ();
      lex ();
      break;

    case BYTESTRING:
      result = make_operation<string_operation> (get_string ());
      lex ();
      break;

    case KW_TRUE:
    case KW_FALSE:
      result = make_operation<bool_operation> (current_token == KW_TRUE);
      lex ();
      break;

    case GDBVAR:
      /* This is kind of a hacky approach.  */
      {
	pstate->push_dollar (current_string_val);
	result = pstate->pop ();
	lex ();
      }
      break;

    case KW_SELF:
    case KW_SUPER:
    case COLONCOLON:
    case KW_EXTERN:
    case IDENT:
      result = parse_path_expr ();
      break;

    case '*':
      lex ();
      result = make_operation<rust_unop_ind_operation> (parse_atom (true));
      break;
    case '+':
      lex ();
      result = make_operation<unary_plus_operation> (parse_atom (true));
      break;
    case '-':
      lex ();
      result = make_operation<unary_neg_operation> (parse_atom (true));
      break;
    case '!':
      lex ();
      result = make_operation<rust_unop_compl_operation> (parse_atom (true));
      break;
    case KW_SIZEOF:
      result = parse_sizeof ();
      break;
    case '&':
      result = parse_addr ();
      break;

    default:
      if (!required)
	return {};
      error (_("unexpected token"));
    }

  /* Now parse suffixes.  */
  while (true)
    {
      switch (current_token)
	{
	case '.':
	  result = parse_field (std::move (result));
	  break;

	case '[':
	  result = parse_index (std::move (result));
	  break;

	case '(':
	  result = parse_call (std::move (result));
	  break;

	default:
	  return result;
	}
    }
}



/* The parser as exposed to gdb.  */

int
rust_language::parser (struct parser_state *state) const
{
  rust_parser parser (state);

  operation_up result;
  try
    {
      result = parser.parse_entry_point ();
    }
  catch (const gdb_exception &exc)
    {
      if (state->parse_completion)
	{
	  result = std::move (parser.completion_op);
	  if (result == nullptr)
	    throw;
	}
      else
	throw;
    }

  state->set_operation (std::move (result));

  return 0;
}



#if GDB_SELF_TEST

/* A test helper that lexes a string, expecting a single token.  */

static void
rust_lex_test_one (rust_parser *parser, const char *input, int expected)
{
  int token;

  parser->reset (input);

  token = parser->lex_one_token ();
  SELF_CHECK (token == expected);

  if (token)
    {
      token = parser->lex_one_token ();
      SELF_CHECK (token == 0);
    }
}

/* Test that INPUT lexes as the integer VALUE.  */

static void
rust_lex_int_test (rust_parser *parser, const char *input,
		   ULONGEST value, int kind)
{
  rust_lex_test_one (parser, input, kind);
  SELF_CHECK (parser->current_int_val.val == value);
}

/* Test that INPUT throws an exception with text ERR.  */

static void
rust_lex_exception_test (rust_parser *parser, const char *input,
			 const char *err)
{
  try
    {
      /* The "kind" doesn't matter.  */
      rust_lex_test_one (parser, input, DECIMAL_INTEGER);
      SELF_CHECK (0);
    }
  catch (const gdb_exception_error &except)
    {
      SELF_CHECK (strcmp (except.what (), err) == 0);
    }
}

/* Test that INPUT lexes as the identifier, string, or byte-string
   VALUE.  KIND holds the expected token kind.  */

static void
rust_lex_stringish_test (rust_parser *parser, const char *input,
			 const char *value, int kind)
{
  rust_lex_test_one (parser, input, kind);
  SELF_CHECK (parser->get_string () == value);
}

/* Helper to test that a string parses as a given token sequence.  */

static void
rust_lex_test_sequence (rust_parser *parser, const char *input, int len,
			const int expected[])
{
  int i;

  parser->reset (input);

  for (i = 0; i < len; ++i)
    {
      int token = parser->lex_one_token ();
      SELF_CHECK (token == expected[i]);
    }
}

/* Tests for an integer-parsing corner case.  */

static void
rust_lex_test_trailing_dot (rust_parser *parser)
{
  const int expected1[] = { DECIMAL_INTEGER, '.', IDENT, '(', ')', 0 };
  const int expected2[] = { INTEGER, '.', IDENT, '(', ')', 0 };
  const int expected3[] = { FLOAT, EQEQ, '(', ')', 0 };
  const int expected4[] = { DECIMAL_INTEGER, DOTDOT, DECIMAL_INTEGER, 0 };

  rust_lex_test_sequence (parser, "23.g()", ARRAY_SIZE (expected1), expected1);
  rust_lex_test_sequence (parser, "23_0.g()", ARRAY_SIZE (expected2),
			  expected2);
  rust_lex_test_sequence (parser, "23.==()", ARRAY_SIZE (expected3),
			  expected3);
  rust_lex_test_sequence (parser, "23..25", ARRAY_SIZE (expected4), expected4);
}

/* Tests of completion.  */

static void
rust_lex_test_completion (rust_parser *parser)
{
  const int expected[] = { IDENT, '.', COMPLETE, 0 };

  parser->pstate->parse_completion = 1;

  rust_lex_test_sequence (parser, "something.wha", ARRAY_SIZE (expected),
			  expected);
  rust_lex_test_sequence (parser, "something.", ARRAY_SIZE (expected),
			  expected);

  parser->pstate->parse_completion = 0;
}

/* Test pushback.  */

static void
rust_lex_test_push_back (rust_parser *parser)
{
  int token;

  parser->reset (">>=");

  token = parser->lex_one_token ();
  SELF_CHECK (token == COMPOUND_ASSIGN);
  SELF_CHECK (parser->current_opcode == BINOP_RSH);

  parser->push_back ('=');

  token = parser->lex_one_token ();
  SELF_CHECK (token == '=');

  token = parser->lex_one_token ();
  SELF_CHECK (token == 0);
}

/* Unit test the lexer.  */

static void
rust_lex_tests (void)
{
  /* Set up dummy "parser", so that rust_type works.  */
  parser_state ps (language_def (language_rust), current_inferior ()->arch (),
		   nullptr, 0, 0, nullptr, 0, nullptr);
  rust_parser parser (&ps);

  rust_lex_test_one (&parser, "", 0);
  rust_lex_test_one (&parser, "    \t  \n \r  ", 0);
  rust_lex_test_one (&parser, "thread 23", 0);
  rust_lex_test_one (&parser, "task 23", 0);
  rust_lex_test_one (&parser, "th 104", 0);
  rust_lex_test_one (&parser, "ta 97", 0);

  rust_lex_int_test (&parser, "'z'", 'z', INTEGER);
  rust_lex_int_test (&parser, "'\\xff'", 0xff, INTEGER);
  rust_lex_int_test (&parser, "'\\u{1016f}'", 0x1016f, INTEGER);
  rust_lex_int_test (&parser, "b'z'", 'z', INTEGER);
  rust_lex_int_test (&parser, "b'\\xfe'", 0xfe, INTEGER);
  rust_lex_int_test (&parser, "b'\\xFE'", 0xfe, INTEGER);
  rust_lex_int_test (&parser, "b'\\xfE'", 0xfe, INTEGER);

  /* Test all escapes in both modes.  */
  rust_lex_int_test (&parser, "'\\n'", '\n', INTEGER);
  rust_lex_int_test (&parser, "'\\r'", '\r', INTEGER);
  rust_lex_int_test (&parser, "'\\t'", '\t', INTEGER);
  rust_lex_int_test (&parser, "'\\\\'", '\\', INTEGER);
  rust_lex_int_test (&parser, "'\\0'", '\0', INTEGER);
  rust_lex_int_test (&parser, "'\\''", '\'', INTEGER);
  rust_lex_int_test (&parser, "'\\\"'", '"', INTEGER);

  rust_lex_int_test (&parser, "b'\\n'", '\n', INTEGER);
  rust_lex_int_test (&parser, "b'\\r'", '\r', INTEGER);
  rust_lex_int_test (&parser, "b'\\t'", '\t', INTEGER);
  rust_lex_int_test (&parser, "b'\\\\'", '\\', INTEGER);
  rust_lex_int_test (&parser, "b'\\0'", '\0', INTEGER);
  rust_lex_int_test (&parser, "b'\\''", '\'', INTEGER);
  rust_lex_int_test (&parser, "b'\\\"'", '"', INTEGER);

  rust_lex_exception_test (&parser, "'z", "Unterminated character literal");
  rust_lex_exception_test (&parser, "b'\\x0'", "Not enough hex digits seen");
  rust_lex_exception_test (&parser, "b'\\u{0}'",
			   "Unicode escape in byte literal");
  rust_lex_exception_test (&parser, "'\\x0'", "Not enough hex digits seen");
  rust_lex_exception_test (&parser, "'\\u0'", "Missing '{' in Unicode escape");
  rust_lex_exception_test (&parser, "'\\u{0", "Missing '}' in Unicode escape");
  rust_lex_exception_test (&parser, "'\\u{0000007}", "Overlong hex escape");
  rust_lex_exception_test (&parser, "'\\u{}", "Not enough hex digits seen");
  rust_lex_exception_test (&parser, "'\\Q'", "Invalid escape \\Q in literal");
  rust_lex_exception_test (&parser, "b'\\Q'", "Invalid escape \\Q in literal");

  rust_lex_int_test (&parser, "23", 23, DECIMAL_INTEGER);
  rust_lex_int_test (&parser, "2_344__29", 234429, INTEGER);
  rust_lex_int_test (&parser, "0x1f", 0x1f, INTEGER);
  rust_lex_int_test (&parser, "23usize", 23, INTEGER);
  rust_lex_int_test (&parser, "23i32", 23, INTEGER);
  rust_lex_int_test (&parser, "0x1_f", 0x1f, INTEGER);
  rust_lex_int_test (&parser, "0b1_101011__", 0x6b, INTEGER);
  rust_lex_int_test (&parser, "0o001177i64", 639, INTEGER);
  rust_lex_int_test (&parser, "0x123456789u64", 0x123456789ull, INTEGER);

  rust_lex_test_trailing_dot (&parser);

  rust_lex_test_one (&parser, "23.", FLOAT);
  rust_lex_test_one (&parser, "23.99f32", FLOAT);
  rust_lex_test_one (&parser, "23e7", FLOAT);
  rust_lex_test_one (&parser, "23E-7", FLOAT);
  rust_lex_test_one (&parser, "23e+7", FLOAT);
  rust_lex_test_one (&parser, "23.99e+7f64", FLOAT);
  rust_lex_test_one (&parser, "23.82f32", FLOAT);

  rust_lex_stringish_test (&parser, "hibob", "hibob", IDENT);
  rust_lex_stringish_test (&parser, "hibob__93", "hibob__93", IDENT);
  rust_lex_stringish_test (&parser, "thread", "thread", IDENT);
  rust_lex_stringish_test (&parser, "r#true", "true", IDENT);

  const int expected1[] = { IDENT, DECIMAL_INTEGER, 0 };
  rust_lex_test_sequence (&parser, "r#thread 23", ARRAY_SIZE (expected1),
			  expected1);
  const int expected2[] = { IDENT, '#', 0 };
  rust_lex_test_sequence (&parser, "r#", ARRAY_SIZE (expected2), expected2);

  rust_lex_stringish_test (&parser, "\"string\"", "string", STRING);
  rust_lex_stringish_test (&parser, "\"str\\ting\"", "str\ting", STRING);
  rust_lex_stringish_test (&parser, "\"str\\\"ing\"", "str\"ing", STRING);
  rust_lex_stringish_test (&parser, "r\"str\\ing\"", "str\\ing", STRING);
  rust_lex_stringish_test (&parser, "r#\"str\\ting\"#", "str\\ting", STRING);
  rust_lex_stringish_test (&parser, "r###\"str\\\"ing\"###", "str\\\"ing",
			   STRING);

  rust_lex_stringish_test (&parser, "b\"string\"", "string", BYTESTRING);
  rust_lex_stringish_test (&parser, "b\"\x73tring\"", "string", BYTESTRING);
  rust_lex_stringish_test (&parser, "b\"str\\\"ing\"", "str\"ing", BYTESTRING);
  rust_lex_stringish_test (&parser, "br####\"\\x73tring\"####", "\\x73tring",
			   BYTESTRING);

  for (const auto &candidate : identifier_tokens)
    rust_lex_test_one (&parser, candidate.name, candidate.value);

  for (const auto &candidate : operator_tokens)
    rust_lex_test_one (&parser, candidate.name, candidate.value);

  rust_lex_test_completion (&parser);
  rust_lex_test_push_back (&parser);
}

#endif /* GDB_SELF_TEST */



void _initialize_rust_exp ();
void
_initialize_rust_exp ()
{
  int code = regcomp (&number_regex, number_regex_text, REG_EXTENDED);
  /* If the regular expression was incorrect, it was a programming
     error.  */
  gdb_assert (code == 0);

#if GDB_SELF_TEST
  selftests::register_test ("rust-lex", rust_lex_tests);
#endif
}
