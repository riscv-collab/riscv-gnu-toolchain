/* Parser for linespec for the GNU debugger, GDB.

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
#include "symtab.h"
#include "frame.h"
#include "command.h"
#include "symfile.h"
#include "objfiles.h"
#include "source.h"
#include "demangle.h"
#include "value.h"
#include "completer.h"
#include "cp-abi.h"
#include "cp-support.h"
#include "parser-defs.h"
#include "block.h"
#include "objc-lang.h"
#include "linespec.h"
#include "language.h"
#include "interps.h"
#include "mi/mi-cmds.h"
#include "target.h"
#include "arch-utils.h"
#include <ctype.h>
#include "cli/cli-utils.h"
#include "filenames.h"
#include "ada-lang.h"
#include "stack.h"
#include "location.h"
#include "gdbsupport/function-view.h"
#include "gdbsupport/def-vector.h"
#include <algorithm>
#include "inferior.h"

/* An enumeration of the various things a user might attempt to
   complete for a linespec location.  */

enum class linespec_complete_what
{
  /* Nothing, no possible completion.  */
  NOTHING,

  /* A function/method name.  Due to ambiguity between

       (gdb) b source[TAB]
       source_file.c
       source_function

     this can also indicate a source filename, iff we haven't seen a
     separate source filename component, as in "b source.c:function".  */
  FUNCTION,

  /* A label symbol.  E.g., break file.c:function:LABEL.  */
  LABEL,

  /* An expression.  E.g., "break foo if EXPR", or "break *EXPR".  */
  EXPRESSION,

  /* A linespec keyword ("if"/"thread"/"task"/"-force-condition").
     E.g., "break func threa<tab>".  */
  KEYWORD,
};

/* An address entry is used to ensure that any given location is only
   added to the result a single time.  It holds an address and the
   program space from which the address came.  */

struct address_entry
{
  struct program_space *pspace;
  CORE_ADDR addr;
};

/* A linespec.  Elements of this structure are filled in by a parser
   (either parse_linespec or some other function).  The structure is
   then converted into SALs by convert_linespec_to_sals.  */

struct linespec
{
  /* An explicit location spec describing the SaLs.  */
  explicit_location_spec explicit_loc;

  /* The list of symtabs to search to which to limit the search.

     If explicit.SOURCE_FILENAME is NULL (no user-specified filename),
     FILE_SYMTABS should contain one single NULL member.  This will cause the
     code to use the default symtab.  */
  std::vector<symtab *> file_symtabs;

  /* A list of matching function symbols and minimal symbols.  Both lists
     may be empty if no matching symbols were found.  */
  std::vector<block_symbol> function_symbols;
  std::vector<bound_minimal_symbol> minimal_symbols;

  /* A structure of matching label symbols and the corresponding
     function symbol in which the label was found.  Both may be empty
     or both must be non-empty.  */
  struct
  {
    std::vector<block_symbol> label_symbols;
    std::vector<block_symbol> function_symbols;
  } labels;
};

/* A canonical linespec represented as a symtab-related string.

   Each entry represents the "SYMTAB:SUFFIX" linespec string.
   SYMTAB can be converted for example by symtab_to_fullname or
   symtab_to_filename_for_display as needed.  */

struct linespec_canonical_name
{
  /* Remaining text part of the linespec string.  */
  char *suffix;

  /* If NULL then SUFFIX is the whole linespec string.  */
  struct symtab *symtab;
};

/* An instance of this is used to keep all state while linespec
   operates.  This instance is passed around as a 'this' pointer to
   the various implementation methods.  */

struct linespec_state
{
  /* The language in use during linespec processing.  */
  const struct language_defn *language;

  /* The program space as seen when the module was entered.  */
  struct program_space *program_space;

  /* If not NULL, the search is restricted to just this program
     space.  */
  struct program_space *search_pspace;

  /* The default symtab to use, if no other symtab is specified.  */
  struct symtab *default_symtab;

  /* The default line to use.  */
  int default_line;

  /* The 'funfirstline' value that was passed in to decode_line_1 or
     decode_line_full.  */
  int funfirstline;

  /* Nonzero if we are running in 'list' mode; see decode_line_list.  */
  int list_mode;

  /* The 'canonical' value passed to decode_line_full, or NULL.  */
  struct linespec_result *canonical;

  /* Canonical strings that mirror the std::vector<symtab_and_line> result.  */
  struct linespec_canonical_name *canonical_names;

  /* This is a set of address_entry objects which is used to prevent
     duplicate symbols from being entered into the result.  */
  htab_t addr_set;

  /* Are we building a linespec?  */
  int is_linespec;
};

/* This is a helper object that is used when collecting symbols into a
   result.  */

struct collect_info
{
  /* The linespec object in use.  */
  struct linespec_state *state;

  /* A list of symtabs to which to restrict matches.  */
  const std::vector<symtab *> *file_symtabs;

  /* The result being accumulated.  */
  struct
  {
    std::vector<block_symbol> *symbols;
    std::vector<bound_minimal_symbol> *minimal_symbols;
  } result;

  /* Possibly add a symbol to the results.  */
  virtual bool add_symbol (block_symbol *bsym);
};

bool
collect_info::add_symbol (block_symbol *bsym)
{
  /* In list mode, add all matching symbols, regardless of class.
     This allows the user to type "list a_global_variable".  */
  if (bsym->symbol->aclass () == LOC_BLOCK || this->state->list_mode)
    this->result.symbols->push_back (*bsym);

  /* Continue iterating.  */
  return true;
}

/* Custom collect_info for symbol_searcher.  */

struct symbol_searcher_collect_info
  : collect_info
{
  bool add_symbol (block_symbol *bsym) override
  {
    /* Add everything.  */
    this->result.symbols->push_back (*bsym);

    /* Continue iterating.  */
    return true;
  }
};

/* Token types  */

enum linespec_token_type
{
  /* A keyword  */
  LSTOKEN_KEYWORD = 0,

  /* A colon "separator"  */
  LSTOKEN_COLON,

  /* A string  */
  LSTOKEN_STRING,

  /* A number  */
  LSTOKEN_NUMBER,

  /* A comma  */
  LSTOKEN_COMMA,

  /* EOI (end of input)  */
  LSTOKEN_EOI,

  /* Consumed token  */
  LSTOKEN_CONSUMED
};

/* List of keywords.  This is NULL-terminated so that it can be used
   as enum completer.  */
const char * const linespec_keywords[] = { "if", "thread", "task", "inferior", "-force-condition", NULL };
#define IF_KEYWORD_INDEX 0
#define FORCE_KEYWORD_INDEX 4

/* A token of the linespec lexer  */

struct linespec_token
{
  /* The type of the token  */
  linespec_token_type type;

  /* Data for the token  */
  union
  {
    /* A string, given as a stoken  */
    struct stoken string;

    /* A keyword  */
    const char *keyword;
  } data;
};

#define LS_TOKEN_STOKEN(TOK) (TOK).data.string
#define LS_TOKEN_KEYWORD(TOK) (TOK).data.keyword

/* An instance of the linespec parser.  */

struct linespec_parser
{
  linespec_parser (int flags, const struct language_defn *language,
		   struct program_space *search_pspace,
		   struct symtab *default_symtab,
		   int default_line,
		   struct linespec_result *canonical);

  ~linespec_parser ();

  DISABLE_COPY_AND_ASSIGN (linespec_parser);

  /* Lexer internal data  */
  struct
  {
    /* Save head of input stream.  */
    const char *saved_arg;

    /* Head of the input stream.  */
    const char *stream;
#define PARSER_STREAM(P) ((P)->lexer.stream)

    /* The current token.  */
    linespec_token current;
  } lexer {};

  /* Is the entire linespec quote-enclosed?  */
  int is_quote_enclosed = 0;

  /* The state of the parse.  */
  struct linespec_state state {};
#define PARSER_STATE(PPTR) (&(PPTR)->state)

  /* The result of the parse.  */
  linespec result;
#define PARSER_RESULT(PPTR) (&(PPTR)->result)

  /* What the parser believes the current word point should complete
     to.  */
  linespec_complete_what complete_what = linespec_complete_what::NOTHING;

  /* The completion word point.  The parser advances this as it skips
     tokens.  At some point the input string will end or parsing will
     fail, and then we attempt completion at the captured completion
     word point, interpreting the string at completion_word as
     COMPLETE_WHAT.  */
  const char *completion_word = nullptr;

  /* If the current token was a quoted string, then this is the
     quoting character (either " or ').  */
  int completion_quote_char = 0;

  /* If the current token was a quoted string, then this points at the
     end of the quoted string.  */
  const char *completion_quote_end = nullptr;

  /* If parsing for completion, then this points at the completion
     tracker.  Otherwise, this is NULL.  */
  struct completion_tracker *completion_tracker = nullptr;
};

/* A convenience macro for accessing the explicit location spec result
   of the parser.  */
#define PARSER_EXPLICIT(PPTR) (&PARSER_RESULT ((PPTR))->explicit_loc)

/* Prototypes for local functions.  */

static void iterate_over_file_blocks
  (struct symtab *symtab, const lookup_name_info &name,
   domain_enum domain,
   gdb::function_view<symbol_found_callback_ftype> callback);

static void initialize_defaults (struct symtab **default_symtab,
				 int *default_line);

CORE_ADDR linespec_expression_to_pc (const char **exp_ptr);

static std::vector<symtab_and_line> decode_objc (struct linespec_state *self,
						 linespec *ls,
						 const char *arg);

static std::vector<symtab *> symtabs_from_filename
  (const char *, struct program_space *pspace);

static std::vector<block_symbol> find_label_symbols
  (struct linespec_state *self,
   const std::vector<block_symbol> &function_symbols,
   std::vector<block_symbol> *label_funcs_ret,
   const char *name, bool completion_mode = false);

static void find_linespec_symbols (struct linespec_state *self,
				   const std::vector<symtab *> &file_symtabs,
				   const char *name,
				   symbol_name_match_type name_match_type,
				   std::vector<block_symbol> *symbols,
				   std::vector<bound_minimal_symbol> *minsyms);

static struct line_offset
     linespec_parse_variable (struct linespec_state *self,
			      const char *variable);

static int symbol_to_sal (struct symtab_and_line *result,
			  int funfirstline, struct symbol *sym);

static void add_matching_symbols_to_info (const char *name,
					  symbol_name_match_type name_match_type,
					  enum search_domain search_domain,
					  struct collect_info *info,
					  struct program_space *pspace);

static void add_all_symbol_names_from_pspace
    (struct collect_info *info, struct program_space *pspace,
     const std::vector<const char *> &names, enum search_domain search_domain);

static std::vector<symtab *>
  collect_symtabs_from_filename (const char *file,
				 struct program_space *pspace);

static std::vector<symtab_and_line> decode_digits_ordinary
  (struct linespec_state *self,
   linespec *ls,
   int line,
   const linetable_entry **best_entry);

static std::vector<symtab_and_line> decode_digits_list_mode
  (struct linespec_state *self,
   linespec *ls,
   struct symtab_and_line val);

static void minsym_found (struct linespec_state *self, struct objfile *objfile,
			  struct minimal_symbol *msymbol,
			  std::vector<symtab_and_line> *result);

static bool compare_symbols (const block_symbol &a, const block_symbol &b);

static bool compare_msymbols (const bound_minimal_symbol &a,
			      const bound_minimal_symbol &b);

/* Permitted quote characters for the parser.  This is different from the
   completer's quote characters to allow backward compatibility with the
   previous parser.  */
static const char linespec_quote_characters[] = "\"\'";

/* Lexer functions.  */

/* Lex a number from the input in PARSER.  This only supports
   decimal numbers.

   Return true if input is decimal numbers.  Return false if not.  */

static int
linespec_lexer_lex_number (linespec_parser *parser, linespec_token *tokenp)
{
  tokenp->type = LSTOKEN_NUMBER;
  LS_TOKEN_STOKEN (*tokenp).length = 0;
  LS_TOKEN_STOKEN (*tokenp).ptr = PARSER_STREAM (parser);

  /* Keep any sign at the start of the stream.  */
  if (*PARSER_STREAM (parser) == '+' || *PARSER_STREAM (parser) == '-')
    {
      ++LS_TOKEN_STOKEN (*tokenp).length;
      ++(PARSER_STREAM (parser));
    }

  while (isdigit (*PARSER_STREAM (parser)))
    {
      ++LS_TOKEN_STOKEN (*tokenp).length;
      ++(PARSER_STREAM (parser));
    }

  /* If the next character in the input buffer is not a space, comma,
     quote, or colon, this input does not represent a number.  */
  if (*PARSER_STREAM (parser) != '\0'
      && !isspace (*PARSER_STREAM (parser)) && *PARSER_STREAM (parser) != ','
      && *PARSER_STREAM (parser) != ':'
      && !strchr (linespec_quote_characters, *PARSER_STREAM (parser)))
    {
      PARSER_STREAM (parser) = LS_TOKEN_STOKEN (*tokenp).ptr;
      return 0;
    }

  return 1;
}

/* See linespec.h.  */

const char *
linespec_lexer_lex_keyword (const char *p)
{
  int i;

  if (p != NULL)
    {
      for (i = 0; linespec_keywords[i] != NULL; ++i)
	{
	  int len = strlen (linespec_keywords[i]);

	  /* If P begins with

	     - "thread" or "task" and the next character is
	     whitespace, we may have found a keyword.  It is only a
	     keyword if it is not followed by another keyword.

	     - "-force-condition", the next character may be EOF
	     since this keyword does not take any arguments.  Otherwise,
	     it should be followed by a keyword.

	     - "if", ALWAYS stop the lexer, since it is not possible to
	     predict what is going to appear in the condition, which can
	     only be parsed after SaLs have been found.  */
	  if (strncmp (p, linespec_keywords[i], len) == 0)
	    {
	      int j;

	      if (i == FORCE_KEYWORD_INDEX && p[len] == '\0')
		return linespec_keywords[i];

	      if (!isspace (p[len]))
		continue;

	      if (i == FORCE_KEYWORD_INDEX)
		{
		  p += len;
		  p = skip_spaces (p);
		  for (j = 0; linespec_keywords[j] != NULL; ++j)
		    {
		      int nextlen = strlen (linespec_keywords[j]);

		      if (strncmp (p, linespec_keywords[j], nextlen) == 0
			  && isspace (p[nextlen]))
			return linespec_keywords[i];
		    }
		}
	      else if (i != IF_KEYWORD_INDEX)
		{
		  /* We matched a "thread" or "task".  */
		  p += len;
		  p = skip_spaces (p);
		  for (j = 0; linespec_keywords[j] != NULL; ++j)
		    {
		      int nextlen = strlen (linespec_keywords[j]);

		      if (strncmp (p, linespec_keywords[j], nextlen) == 0
			  && isspace (p[nextlen]))
			return NULL;
		    }
		}

	      return linespec_keywords[i];
	    }
	}
    }

  return NULL;
}

/*  See description in linespec.h.  */

int
is_ada_operator (const char *string)
{
  const struct ada_opname_map *mapping;

  for (mapping = ada_opname_table;
       mapping->encoded != NULL
	 && !startswith (string, mapping->decoded); ++mapping)
    ;

  return mapping->decoded == NULL ? 0 : strlen (mapping->decoded);
}

/* Find QUOTE_CHAR in STRING, accounting for the ':' terminal.  Return
   the location of QUOTE_CHAR, or NULL if not found.  */

static const char *
skip_quote_char (const char *string, char quote_char)
{
  const char *p, *last;

  p = last = find_toplevel_char (string, quote_char);
  while (p && *p != '\0' && *p != ':')
    {
      p = find_toplevel_char (p, quote_char);
      if (p != NULL)
	last = p++;
    }

  return last;
}

/* Make a writable copy of the string given in TOKEN, trimming
   any trailing whitespace.  */

static gdb::unique_xmalloc_ptr<char>
copy_token_string (linespec_token token)
{
  const char *str, *s;

  if (token.type == LSTOKEN_KEYWORD)
    return make_unique_xstrdup (LS_TOKEN_KEYWORD (token));

  str = LS_TOKEN_STOKEN (token).ptr;
  s = remove_trailing_whitespace (str, str + LS_TOKEN_STOKEN (token).length);

  return gdb::unique_xmalloc_ptr<char> (savestring (str, s - str));
}

/* Does P represent the end of a quote-enclosed linespec?  */

static int
is_closing_quote_enclosed (const char *p)
{
  if (strchr (linespec_quote_characters, *p))
    ++p;
  p = skip_spaces ((char *) p);
  return (*p == '\0' || linespec_lexer_lex_keyword (p));
}

/* Find the end of the parameter list that starts with *INPUT.
   This helper function assists with lexing string segments
   which might contain valid (non-terminating) commas.  */

static const char *
find_parameter_list_end (const char *input)
{
  char end_char, start_char;
  int depth;
  const char *p;

  start_char = *input;
  if (start_char == '(')
    end_char = ')';
  else if (start_char == '<')
    end_char = '>';
  else
    return NULL;

  p = input;
  depth = 0;
  while (*p)
    {
      if (*p == start_char)
	++depth;
      else if (*p == end_char)
	{
	  if (--depth == 0)
	    {
	      ++p;
	      break;
	    }
	}
      ++p;
    }

  return p;
}

/* If the [STRING, STRING_LEN) string ends with what looks like a
   keyword, return the keyword start offset in STRING.  Return -1
   otherwise.  */

static size_t
string_find_incomplete_keyword_at_end (const char * const *keywords,
				       const char *string, size_t string_len)
{
  const char *end = string + string_len;
  const char *p = end;

  while (p > string && *p != ' ')
    --p;
  if (p > string)
    {
      p++;
      size_t len = end - p;
      for (size_t i = 0; keywords[i] != NULL; ++i)
	if (strncmp (keywords[i], p, len) == 0)
	  return p - string;
    }

  return -1;
}

/* Lex a string from the input in PARSER.  */

static linespec_token
linespec_lexer_lex_string (linespec_parser *parser)
{
  linespec_token token;
  const char *start = PARSER_STREAM (parser);

  token.type = LSTOKEN_STRING;

  /* If the input stream starts with a quote character, skip to the next
     quote character, regardless of the content.  */
  if (strchr (linespec_quote_characters, *PARSER_STREAM (parser)))
    {
      const char *end;
      char quote_char = *PARSER_STREAM (parser);

      /* Special case: Ada operators.  */
      if (PARSER_STATE (parser)->language->la_language == language_ada
	  && quote_char == '\"')
	{
	  int len = is_ada_operator (PARSER_STREAM (parser));

	  if (len != 0)
	    {
	      /* The input is an Ada operator.  Return the quoted string
		 as-is.  */
	      LS_TOKEN_STOKEN (token).ptr = PARSER_STREAM (parser);
	      LS_TOKEN_STOKEN (token).length = len;
	      PARSER_STREAM (parser) += len;
	      return token;
	    }

	  /* The input does not represent an Ada operator -- fall through
	     to normal quoted string handling.  */
	}

      /* Skip past the beginning quote.  */
      ++(PARSER_STREAM (parser));

      /* Mark the start of the string.  */
      LS_TOKEN_STOKEN (token).ptr = PARSER_STREAM (parser);

      /* Skip to the ending quote.  */
      end = skip_quote_char (PARSER_STREAM (parser), quote_char);

      /* This helps the completer mode decide whether we have a
	 complete string.  */
      parser->completion_quote_char = quote_char;
      parser->completion_quote_end = end;

      /* Error if the input did not terminate properly, unless in
	 completion mode.  */
      if (end == NULL)
	{
	  if (parser->completion_tracker == NULL)
	    error (_("unmatched quote"));

	  /* In completion mode, we'll try to complete the incomplete
	     token.  */
	  token.type = LSTOKEN_STRING;
	  while (*PARSER_STREAM (parser) != '\0')
	    PARSER_STREAM (parser)++;
	  LS_TOKEN_STOKEN (token).length = PARSER_STREAM (parser) - 1 - start;
	}
      else
	{
	  /* Skip over the ending quote and mark the length of the string.  */
	  PARSER_STREAM (parser) = (char *) ++end;
	  LS_TOKEN_STOKEN (token).length = PARSER_STREAM (parser) - 2 - start;
	}
    }
  else
    {
      const char *p;

      /* Otherwise, only identifier characters are permitted.
	 Spaces are the exception.  In general, we keep spaces,
	 but only if the next characters in the input do not resolve
	 to one of the keywords.

	 This allows users to forgo quoting CV-qualifiers, template arguments,
	 and similar common language constructs.  */

      while (1)
	{
	  if (isspace (*PARSER_STREAM (parser)))
	    {
	      p = skip_spaces (PARSER_STREAM (parser));
	      /* When we get here we know we've found something followed by
		 a space (we skip over parens and templates below).
		 So if we find a keyword now, we know it is a keyword and not,
		 say, a function name.  */
	      if (linespec_lexer_lex_keyword (p) != NULL)
		{
		  LS_TOKEN_STOKEN (token).ptr = start;
		  LS_TOKEN_STOKEN (token).length
		    = PARSER_STREAM (parser) - start;
		  return token;
		}

	      /* Advance past the whitespace.  */
	      PARSER_STREAM (parser) = p;
	    }

	  /* If the next character is EOI or (single) ':', the
	     string is complete;  return the token.  */
	  if (*PARSER_STREAM (parser) == 0)
	    {
	      LS_TOKEN_STOKEN (token).ptr = start;
	      LS_TOKEN_STOKEN (token).length = PARSER_STREAM (parser) - start;
	      return token;
	    }
	  else if (PARSER_STREAM (parser)[0] == ':')
	    {
	      /* Do not tokenize the C++ scope operator. */
	      if (PARSER_STREAM (parser)[1] == ':')
		++(PARSER_STREAM (parser));

	      /* Do not tokenize ABI tags such as "[abi:cxx11]".  */
	      else if (PARSER_STREAM (parser) - start > 4
		       && startswith (PARSER_STREAM (parser) - 4, "[abi"))
		{
		  /* Nothing.  */
		}

	      /* Do not tokenify if the input length so far is one
		 (i.e, a single-letter drive name) and the next character
		 is a directory separator.  This allows Windows-style
		 paths to be recognized as filenames without quoting it.  */
	      else if ((PARSER_STREAM (parser) - start) != 1
		       || !IS_DIR_SEPARATOR (PARSER_STREAM (parser)[1]))
		{
		  LS_TOKEN_STOKEN (token).ptr = start;
		  LS_TOKEN_STOKEN (token).length
		    = PARSER_STREAM (parser) - start;
		  return token;
		}
	    }
	  /* Special case: permit quote-enclosed linespecs.  */
	  else if (parser->is_quote_enclosed
		   && strchr (linespec_quote_characters,
			      *PARSER_STREAM (parser))
		   && is_closing_quote_enclosed (PARSER_STREAM (parser)))
	    {
	      LS_TOKEN_STOKEN (token).ptr = start;
	      LS_TOKEN_STOKEN (token).length = PARSER_STREAM (parser) - start;
	      return token;
	    }
	  /* Because commas may terminate a linespec and appear in
	     the middle of valid string input, special cases for
	     '<' and '(' are necessary.  */
	  else if (*PARSER_STREAM (parser) == '<'
		   || *PARSER_STREAM (parser) == '(')
	    {
	      /* Don't interpret 'operator<' / 'operator<<' as a
		 template parameter list though.  */
	      if (*PARSER_STREAM (parser) == '<'
		  && (PARSER_STATE (parser)->language->la_language
		      == language_cplus)
		  && (PARSER_STREAM (parser) - start) >= CP_OPERATOR_LEN)
		{
		  const char *op = PARSER_STREAM (parser);

		  while (op > start && isspace (op[-1]))
		    op--;
		  if (op - start >= CP_OPERATOR_LEN)
		    {
		      op -= CP_OPERATOR_LEN;
		      if (strncmp (op, CP_OPERATOR_STR, CP_OPERATOR_LEN) == 0
			  && (op == start
			      || !(isalnum (op[-1]) || op[-1] == '_')))
			{
			  /* This is an operator name.  Keep going.  */
			  ++(PARSER_STREAM (parser));
			  if (*PARSER_STREAM (parser) == '<')
			    ++(PARSER_STREAM (parser));
			  continue;
			}
		    }
		}

	      const char *end = find_parameter_list_end (PARSER_STREAM (parser));
	      PARSER_STREAM (parser) = end;

	      /* Don't loop around to the normal \0 case above because
		 we don't want to misinterpret a potential keyword at
		 the end of the token when the string isn't
		 "()<>"-balanced.  This handles "b
		 function(thread<tab>" in completion mode.  */
	      if (*end == '\0')
		{
		  LS_TOKEN_STOKEN (token).ptr = start;
		  LS_TOKEN_STOKEN (token).length
		    = PARSER_STREAM (parser) - start;
		  return token;
		}
	      else
		continue;
	    }
	  /* Commas are terminators, but not if they are part of an
	     operator name.  */
	  else if (*PARSER_STREAM (parser) == ',')
	    {
	      if ((PARSER_STATE (parser)->language->la_language
		   == language_cplus)
		  && (PARSER_STREAM (parser) - start) > CP_OPERATOR_LEN)
		{
		  const char *op = strstr (start, CP_OPERATOR_STR);

		  if (op != NULL && is_operator_name (op))
		    {
		      /* This is an operator name.  Keep going.  */
		      ++(PARSER_STREAM (parser));
		      continue;
		    }
		}

	      /* Comma terminates the string.  */
	      LS_TOKEN_STOKEN (token).ptr = start;
	      LS_TOKEN_STOKEN (token).length = PARSER_STREAM (parser) - start;
	      return token;
	    }

	  /* Advance the stream.  */
	  gdb_assert (*(PARSER_STREAM (parser)) != '\0');
	  ++(PARSER_STREAM (parser));
	}
    }

  return token;
}

/* Lex a single linespec token from PARSER.  */

static linespec_token
linespec_lexer_lex_one (linespec_parser *parser)
{
  const char *keyword;

  if (parser->lexer.current.type == LSTOKEN_CONSUMED)
    {
      /* Skip any whitespace.  */
      PARSER_STREAM (parser) = skip_spaces (PARSER_STREAM (parser));

      /* Check for a keyword, they end the linespec.  */
      keyword = linespec_lexer_lex_keyword (PARSER_STREAM (parser));
      if (keyword != NULL)
	{
	  parser->lexer.current.type = LSTOKEN_KEYWORD;
	  LS_TOKEN_KEYWORD (parser->lexer.current) = keyword;
	  /* We do not advance the stream here intentionally:
	     we would like lexing to stop when a keyword is seen.

	     PARSER_STREAM (parser) +=  strlen (keyword);  */

	  return parser->lexer.current;
	}

      /* Handle other tokens.  */
      switch (*PARSER_STREAM (parser))
	{
	case 0:
	  parser->lexer.current.type = LSTOKEN_EOI;
	  break;

	case '+': case '-':
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
	   if (!linespec_lexer_lex_number (parser, &(parser->lexer.current)))
	     parser->lexer.current = linespec_lexer_lex_string (parser);
	  break;

	case ':':
	  /* If we have a scope operator, lex the input as a string.
	     Otherwise, return LSTOKEN_COLON.  */
	  if (PARSER_STREAM (parser)[1] == ':')
	    parser->lexer.current = linespec_lexer_lex_string (parser);
	  else
	    {
	      parser->lexer.current.type = LSTOKEN_COLON;
	      ++(PARSER_STREAM (parser));
	    }
	  break;

	case '\'': case '\"':
	  /* Special case: permit quote-enclosed linespecs.  */
	  if (parser->is_quote_enclosed
	      && is_closing_quote_enclosed (PARSER_STREAM (parser)))
	    {
	      ++(PARSER_STREAM (parser));
	      parser->lexer.current.type = LSTOKEN_EOI;
	    }
	  else
	    parser->lexer.current = linespec_lexer_lex_string (parser);
	  break;

	case ',':
	  parser->lexer.current.type = LSTOKEN_COMMA;
	  LS_TOKEN_STOKEN (parser->lexer.current).ptr
	    = PARSER_STREAM (parser);
	  LS_TOKEN_STOKEN (parser->lexer.current).length = 1;
	  ++(PARSER_STREAM (parser));
	  break;

	default:
	  /* If the input is not a number, it must be a string.
	     [Keywords were already considered above.]  */
	  parser->lexer.current = linespec_lexer_lex_string (parser);
	  break;
	}
    }

  return parser->lexer.current;
}

/* Consume the current token and return the next token in PARSER's
   input stream.  Also advance the completion word for completion
   mode.  */

static linespec_token
linespec_lexer_consume_token (linespec_parser *parser)
{
  gdb_assert (parser->lexer.current.type != LSTOKEN_EOI);

  bool advance_word = (parser->lexer.current.type != LSTOKEN_STRING
		       || *PARSER_STREAM (parser) != '\0');

  /* If we're moving past a string to some other token, it must be the
     quote was terminated.  */
  if (parser->completion_quote_char)
    {
      gdb_assert (parser->lexer.current.type == LSTOKEN_STRING);

      /* If the string was the last (non-EOI) token, we're past the
	 quote, but remember that for later.  */
      if (*PARSER_STREAM (parser) != '\0')
	{
	  parser->completion_quote_char = '\0';
	  parser->completion_quote_end = NULL;;
	}
    }

  parser->lexer.current.type = LSTOKEN_CONSUMED;
  linespec_lexer_lex_one (parser);

  if (parser->lexer.current.type == LSTOKEN_STRING)
    {
      /* Advance the completion word past a potential initial
	 quote-char.  */
      parser->completion_word = LS_TOKEN_STOKEN (parser->lexer.current).ptr;
    }
  else if (advance_word)
    {
      /* Advance the completion word past any whitespace.  */
      parser->completion_word = PARSER_STREAM (parser);
    }

  return parser->lexer.current;
}

/* Return the next token without consuming the current token.  */

static linespec_token
linespec_lexer_peek_token (linespec_parser *parser)
{
  linespec_token next;
  const char *saved_stream = PARSER_STREAM (parser);
  linespec_token saved_token = parser->lexer.current;
  int saved_completion_quote_char = parser->completion_quote_char;
  const char *saved_completion_quote_end = parser->completion_quote_end;
  const char *saved_completion_word = parser->completion_word;

  next = linespec_lexer_consume_token (parser);
  PARSER_STREAM (parser) = saved_stream;
  parser->lexer.current = saved_token;
  parser->completion_quote_char = saved_completion_quote_char;
  parser->completion_quote_end = saved_completion_quote_end;
  parser->completion_word = saved_completion_word;
  return next;
}

/* Helper functions.  */

/* Add SAL to SALS, and also update SELF->CANONICAL_NAMES to reflect
   the new sal, if needed.  If not NULL, SYMNAME is the name of the
   symbol to use when constructing the new canonical name.

   If LITERAL_CANONICAL is non-zero, SYMNAME will be used as the
   canonical name for the SAL.  */

static void
add_sal_to_sals (struct linespec_state *self,
		 std::vector<symtab_and_line> *sals,
		 struct symtab_and_line *sal,
		 const char *symname, int literal_canonical)
{
  sals->push_back (*sal);

  if (self->canonical)
    {
      struct linespec_canonical_name *canonical;

      self->canonical_names = XRESIZEVEC (struct linespec_canonical_name,
					  self->canonical_names,
					  sals->size ());
      canonical = &self->canonical_names[sals->size () - 1];
      if (!literal_canonical && sal->symtab)
	{
	  symtab_to_fullname (sal->symtab);

	  /* Note that the filter doesn't have to be a valid linespec
	     input.  We only apply the ":LINE" treatment to Ada for
	     the time being.  */
	  if (symname != NULL && sal->line != 0
	      && self->language->la_language == language_ada)
	    canonical->suffix = xstrprintf ("%s:%d", symname,
					    sal->line).release ();
	  else if (symname != NULL)
	    canonical->suffix = xstrdup (symname);
	  else
	    canonical->suffix = xstrprintf ("%d", sal->line).release ();
	  canonical->symtab = sal->symtab;
	}
      else
	{
	  if (symname != NULL)
	    canonical->suffix = xstrdup (symname);
	  else
	    canonical->suffix = xstrdup ("<unknown>");
	  canonical->symtab = NULL;
	}
    }
}

/* A hash function for address_entry.  */

static hashval_t
hash_address_entry (const void *p)
{
  const struct address_entry *aep = (const struct address_entry *) p;
  hashval_t hash;

  hash = iterative_hash_object (aep->pspace, 0);
  return iterative_hash_object (aep->addr, hash);
}

/* An equality function for address_entry.  */

static int
eq_address_entry (const void *a, const void *b)
{
  const struct address_entry *aea = (const struct address_entry *) a;
  const struct address_entry *aeb = (const struct address_entry *) b;

  return aea->pspace == aeb->pspace && aea->addr == aeb->addr;
}

/* Check whether the address, represented by PSPACE and ADDR, is
   already in the set.  If so, return 0.  Otherwise, add it and return
   1.  */

static int
maybe_add_address (htab_t set, struct program_space *pspace, CORE_ADDR addr)
{
  struct address_entry e, *p;
  void **slot;

  e.pspace = pspace;
  e.addr = addr;
  slot = htab_find_slot (set, &e, INSERT);
  if (*slot)
    return 0;

  p = XNEW (struct address_entry);
  memcpy (p, &e, sizeof (struct address_entry));
  *slot = p;

  return 1;
}

/* A helper that walks over all matching symtabs in all objfiles and
   calls CALLBACK for each symbol matching NAME.  If SEARCH_PSPACE is
   not NULL, then the search is restricted to just that program
   space.  If INCLUDE_INLINE is true then symbols representing
   inlined instances of functions will be included in the result.  */

static void
iterate_over_all_matching_symtabs
  (struct linespec_state *state,
   const lookup_name_info &lookup_name,
   const domain_enum name_domain,
   enum search_domain search_domain,
   struct program_space *search_pspace, bool include_inline,
   gdb::function_view<symbol_found_callback_ftype> callback)
{
  for (struct program_space *pspace : program_spaces)
    {
      if (search_pspace != NULL && search_pspace != pspace)
	continue;
      if (pspace->executing_startup)
	continue;

      set_current_program_space (pspace);

      for (objfile *objfile : current_program_space->objfiles ())
	{
	  objfile->expand_symtabs_matching (NULL, &lookup_name, NULL, NULL,
					    (SEARCH_GLOBAL_BLOCK
					     | SEARCH_STATIC_BLOCK),
					    UNDEF_DOMAIN,
					    search_domain);

	  for (compunit_symtab *cu : objfile->compunits ())
	    {
	      struct symtab *symtab = cu->primary_filetab ();

	      iterate_over_file_blocks (symtab, lookup_name, name_domain,
					callback);

	      if (include_inline)
		{
		  const struct block *block;
		  int i;
		  const blockvector *bv = symtab->compunit ()->blockvector ();

		  for (i = FIRST_LOCAL_BLOCK; i < bv->num_blocks (); i++)
		    {
		      block = bv->block (i);
		      state->language->iterate_over_symbols
			(block, lookup_name, name_domain,
			 [&] (block_symbol *bsym)
			 {
			   /* Restrict calls to CALLBACK to symbols
			      representing inline symbols only.  */
			   if (bsym->symbol->is_inlined ())
			     return callback (bsym);
			   return true;
			 });
		    }
		}
	    }
	}
    }
}

/* Returns the block to be used for symbol searches from
   the current location.  */

static const struct block *
get_current_search_block (void)
{
  /* get_selected_block can change the current language when there is
     no selected frame yet.  */
  scoped_restore_current_language save_language;
  return get_selected_block (0);
}

/* Iterate over static and global blocks.  */

static void
iterate_over_file_blocks
  (struct symtab *symtab, const lookup_name_info &name,
   domain_enum domain, gdb::function_view<symbol_found_callback_ftype> callback)
{
  const struct block *block;

  for (block = symtab->compunit ()->blockvector ()->static_block ();
       block != NULL;
       block = block->superblock ())
    current_language->iterate_over_symbols (block, name, domain, callback);
}

/* A helper for find_method.  This finds all methods in type T of
   language T_LANG which match NAME.  It adds matching symbol names to
   RESULT_NAMES, and adds T's direct superclasses to SUPERCLASSES.  */

static void
find_methods (struct type *t, enum language t_lang, const char *name,
	      std::vector<const char *> *result_names,
	      std::vector<struct type *> *superclasses)
{
  int ibase;
  const char *class_name = t->name ();

  /* Ignore this class if it doesn't have a name.  This is ugly, but
     unless we figure out how to get the physname without the name of
     the class, then the loop can't do any good.  */
  if (class_name)
    {
      int method_counter;
      lookup_name_info lookup_name (name, symbol_name_match_type::FULL);
      symbol_name_matcher_ftype *symbol_name_compare
	= language_def (t_lang)->get_symbol_name_matcher (lookup_name);

      t = check_typedef (t);

      /* Loop over each method name.  At this level, all overloads of a name
	 are counted as a single name.  There is an inner loop which loops over
	 each overload.  */

      for (method_counter = TYPE_NFN_FIELDS (t) - 1;
	   method_counter >= 0;
	   --method_counter)
	{
	  const char *method_name = TYPE_FN_FIELDLIST_NAME (t, method_counter);

	  if (symbol_name_compare (method_name, lookup_name, NULL))
	    {
	      int field_counter;

	      for (field_counter = (TYPE_FN_FIELDLIST_LENGTH (t, method_counter)
				    - 1);
		   field_counter >= 0;
		   --field_counter)
		{
		  struct fn_field *f;
		  const char *phys_name;

		  f = TYPE_FN_FIELDLIST1 (t, method_counter);
		  if (TYPE_FN_FIELD_STUB (f, field_counter))
		    continue;
		  phys_name = TYPE_FN_FIELD_PHYSNAME (f, field_counter);
		  result_names->push_back (phys_name);
		}
	    }
	}
    }

  for (ibase = 0; ibase < TYPE_N_BASECLASSES (t); ibase++)
    superclasses->push_back (TYPE_BASECLASS (t, ibase));
}

/* The string equivalent of find_toplevel_char.  Returns a pointer
   to the location of NEEDLE in HAYSTACK, ignoring any occurrences
   inside "()" and "<>".  Returns NULL if NEEDLE was not found.  */

static const char *
find_toplevel_string (const char *haystack, const char *needle)
{
  const char *s = haystack;

  do
    {
      s = find_toplevel_char (s, *needle);

      if (s != NULL)
	{
	  /* Found first char in HAYSTACK;  check rest of string.  */
	  if (startswith (s, needle))
	    return s;

	  /* Didn't find it; loop over HAYSTACK, looking for the next
	     instance of the first character of NEEDLE.  */
	  ++s;
	}
    }
  while (s != NULL && *s != '\0');

  /* NEEDLE was not found in HAYSTACK.  */
  return NULL;
}

/* Convert CANONICAL to its string representation using
   symtab_to_fullname for SYMTAB.  */

static std::string
canonical_to_fullform (const struct linespec_canonical_name *canonical)
{
  if (canonical->symtab == NULL)
    return canonical->suffix;
  else
    return string_printf ("%s:%s", symtab_to_fullname (canonical->symtab),
			  canonical->suffix);
}

/* Given FILTERS, a list of canonical names, filter the sals in RESULT
   and store the result in SELF->CANONICAL.  */

static void
filter_results (struct linespec_state *self,
		std::vector<symtab_and_line> *result,
		const std::vector<const char *> &filters)
{
  for (const char *name : filters)
    {
      linespec_sals lsal;

      for (size_t j = 0; j < result->size (); ++j)
	{
	  const struct linespec_canonical_name *canonical;

	  canonical = &self->canonical_names[j];
	  std::string fullform = canonical_to_fullform (canonical);

	  if (name == fullform)
	    lsal.sals.push_back ((*result)[j]);
	}

      if (!lsal.sals.empty ())
	{
	  lsal.canonical = xstrdup (name);
	  self->canonical->lsals.push_back (std::move (lsal));
	}
    }

  self->canonical->pre_expanded = 0;
}

/* Store RESULT into SELF->CANONICAL.  */

static void
convert_results_to_lsals (struct linespec_state *self,
			  std::vector<symtab_and_line> *result)
{
  struct linespec_sals lsal;

  lsal.canonical = NULL;
  lsal.sals = std::move (*result);
  self->canonical->lsals.push_back (std::move (lsal));
}

/* A structure that contains two string representations of a struct
   linespec_canonical_name:
     - one where the symtab's fullname is used;
     - one where the filename followed the "set filename-display"
       setting.  */

struct decode_line_2_item
{
  decode_line_2_item (std::string &&fullform_, std::string &&displayform_,
		      bool selected_)
    : fullform (std::move (fullform_)),
      displayform (std::move (displayform_)),
      selected (selected_)
  {
  }

  /* The form using symtab_to_fullname.  */
  std::string fullform;

  /* The form using symtab_to_filename_for_display.  */
  std::string displayform;

  /* Field is initialized to zero and it is set to one if the user
     requested breakpoint for this entry.  */
  unsigned int selected : 1;
};

/* Helper for std::sort to sort decode_line_2_item entries by
   DISPLAYFORM and secondarily by FULLFORM.  */

static bool
decode_line_2_compare_items (const decode_line_2_item &a,
			     const decode_line_2_item &b)
{
  if (a.displayform != b.displayform)
    return a.displayform < b.displayform;
  return a.fullform < b.fullform;
}

/* Handle multiple results in RESULT depending on SELECT_MODE.  This
   will either return normally, throw an exception on multiple
   results, or present a menu to the user.  On return, the SALS vector
   in SELF->CANONICAL is set up properly.  */

static void
decode_line_2 (struct linespec_state *self,
	       std::vector<symtab_and_line> *result,
	       const char *select_mode)
{
  const char *args;
  const char *prompt;
  int i;
  std::vector<const char *> filters;
  std::vector<struct decode_line_2_item> items;

  gdb_assert (select_mode != multiple_symbols_all);
  gdb_assert (self->canonical != NULL);
  gdb_assert (!result->empty ());

  /* Prepare ITEMS array.  */
  for (i = 0; i < result->size (); ++i)
    {
      const struct linespec_canonical_name *canonical;
      std::string displayform;

      canonical = &self->canonical_names[i];
      gdb_assert (canonical->suffix != NULL);

      std::string fullform = canonical_to_fullform (canonical);

      if (canonical->symtab == NULL)
	displayform = canonical->suffix;
      else
	{
	  const char *fn_for_display;

	  fn_for_display = symtab_to_filename_for_display (canonical->symtab);
	  displayform = string_printf ("%s:%s", fn_for_display,
				       canonical->suffix);
	}

      items.emplace_back (std::move (fullform), std::move (displayform),
			  false);
    }

  /* Sort the list of method names.  */
  std::sort (items.begin (), items.end (), decode_line_2_compare_items);

  /* Remove entries with the same FULLFORM.  */
  items.erase (std::unique (items.begin (), items.end (),
			    [] (const struct decode_line_2_item &a,
				const struct decode_line_2_item &b)
			      {
				return a.fullform == b.fullform;
			      }),
	       items.end ());

  if (select_mode == multiple_symbols_cancel && items.size () > 1)
    error (_("canceled because the command is ambiguous\n"
	     "See set/show multiple-symbol."));
  
  if (select_mode == multiple_symbols_all || items.size () == 1)
    {
      convert_results_to_lsals (self, result);
      return;
    }

  printf_unfiltered (_("[0] cancel\n[1] all\n"));
  for (i = 0; i < items.size (); i++)
    printf_unfiltered ("[%d] %s\n", i + 2, items[i].displayform.c_str ());

  prompt = getenv ("PS2");
  if (prompt == NULL)
    {
      prompt = "> ";
    }

  std::string buffer;
  args = command_line_input (buffer, prompt, "overload-choice");

  if (args == 0 || *args == 0)
    error_no_arg (_("one or more choice numbers"));

  number_or_range_parser parser (args);
  while (!parser.finished ())
    {
      int num = parser.get_number ();

      if (num == 0)
	error (_("canceled"));
      else if (num == 1)
	{
	  /* We intentionally make this result in a single breakpoint,
	     contrary to what older versions of gdb did.  The
	     rationale is that this lets a user get the
	     multiple_symbols_all behavior even with the 'ask'
	     setting; and he can get separate breakpoints by entering
	     "2-57" at the query.  */
	  convert_results_to_lsals (self, result);
	  return;
	}

      num -= 2;
      if (num >= items.size ())
	printf_unfiltered (_("No choice number %d.\n"), num);
      else
	{
	  struct decode_line_2_item *item = &items[num];

	  if (!item->selected)
	    {
	      filters.push_back (item->fullform.c_str ());
	      item->selected = 1;
	    }
	  else
	    {
	      printf_unfiltered (_("duplicate request for %d ignored.\n"),
				 num + 2);
	    }
	}
    }

  filter_results (self, result, filters);
}



/* The parser of linespec itself.  */

/* Throw an appropriate error when SYMBOL is not found (optionally in
   FILENAME).  */

static void ATTRIBUTE_NORETURN
symbol_not_found_error (const char *symbol, const char *filename)
{
  if (symbol == NULL)
    symbol = "";

  if (!have_full_symbols ()
      && !have_partial_symbols ()
      && !have_minimal_symbols ())
    throw_error (NOT_FOUND_ERROR,
		 _("No symbol table is loaded.  Use the \"file\" command."));

  /* If SYMBOL starts with '$', the user attempted to either lookup
     a function/variable in his code starting with '$' or an internal
     variable of that name.  Since we do not know which, be concise and
     explain both possibilities.  */
  if (*symbol == '$')
    {
      if (filename)
	throw_error (NOT_FOUND_ERROR,
		     _("Undefined convenience variable or function \"%s\" "
		       "not defined in \"%s\"."), symbol, filename);
      else
	throw_error (NOT_FOUND_ERROR,
		     _("Undefined convenience variable or function \"%s\" "
		       "not defined."), symbol);
    }
  else
    {
      if (filename)
	throw_error (NOT_FOUND_ERROR,
		     _("Function \"%s\" not defined in \"%s\"."),
		     symbol, filename);
      else
	throw_error (NOT_FOUND_ERROR,
		     _("Function \"%s\" not defined."), symbol);
    }
}

/* Throw an appropriate error when an unexpected token is encountered 
   in the input.  */

static void ATTRIBUTE_NORETURN
unexpected_linespec_error (linespec_parser *parser)
{
  linespec_token token;
  static const char * token_type_strings[]
    = {"keyword", "colon", "string", "number", "comma", "end of input"};

  /* Get the token that generated the error.  */
  token = linespec_lexer_lex_one (parser);

  /* Finally, throw the error.  */
  if (token.type == LSTOKEN_STRING || token.type == LSTOKEN_NUMBER
      || token.type == LSTOKEN_KEYWORD)
    {
      gdb::unique_xmalloc_ptr<char> string = copy_token_string (token);
      throw_error (GENERIC_ERROR,
		   _("malformed linespec error: unexpected %s, \"%s\""),
		   token_type_strings[token.type], string.get ());
    }
  else
    throw_error (GENERIC_ERROR,
		 _("malformed linespec error: unexpected %s"),
		 token_type_strings[token.type]);
}

/* Throw an undefined label error.  */

static void ATTRIBUTE_NORETURN
undefined_label_error (const char *function, const char *label)
{
  if (function != NULL)
    throw_error (NOT_FOUND_ERROR,
		_("No label \"%s\" defined in function \"%s\"."),
		label, function);
  else
    throw_error (NOT_FOUND_ERROR,
		_("No label \"%s\" defined in current function."),
		label);
}

/* Throw a source file not found error.  */

static void ATTRIBUTE_NORETURN
source_file_not_found_error (const char *name)
{
  throw_error (NOT_FOUND_ERROR, _("No source file named %s."), name);
}

/* Unless at EIO, save the current stream position as completion word
   point, and consume the next token.  */

static linespec_token
save_stream_and_consume_token (linespec_parser *parser)
{
  if (linespec_lexer_peek_token (parser).type != LSTOKEN_EOI)
    parser->completion_word = PARSER_STREAM (parser);
  return linespec_lexer_consume_token (parser);
}

/* See description in linespec.h.  */

struct line_offset
linespec_parse_line_offset (const char *string)
{
  const char *start = string;
  struct line_offset line_offset;

  if (*string == '+')
    {
      line_offset.sign = LINE_OFFSET_PLUS;
      ++string;
    }
  else if (*string == '-')
    {
      line_offset.sign = LINE_OFFSET_MINUS;
      ++string;
    }
  else
    line_offset.sign = LINE_OFFSET_NONE;

  if (*string != '\0' && !isdigit (*string))
    error (_("malformed line offset: \"%s\""), start);

  /* Right now, we only allow base 10 for offsets.  */
  line_offset.offset = atoi (string);
  return line_offset;
}

/* In completion mode, if the user is still typing the number, there's
   no possible completion to offer.  But if there's already input past
   the number, setup to expect NEXT.  */

static void
set_completion_after_number (linespec_parser *parser,
			     linespec_complete_what next)
{
  if (*PARSER_STREAM (parser) == ' ')
    {
      parser->completion_word = skip_spaces (PARSER_STREAM (parser) + 1);
      parser->complete_what = next;
    }
  else
    {
      parser->completion_word = PARSER_STREAM (parser);
      parser->complete_what = linespec_complete_what::NOTHING;
    }
}

/* Parse the basic_spec in PARSER's input.  */

static void
linespec_parse_basic (linespec_parser *parser)
{
  gdb::unique_xmalloc_ptr<char> name;
  linespec_token token;

  /* Get the next token.  */
  token = linespec_lexer_lex_one (parser);

  /* If it is EOI or KEYWORD, issue an error.  */
  if (token.type == LSTOKEN_KEYWORD)
    {
      parser->complete_what = linespec_complete_what::NOTHING;
      unexpected_linespec_error (parser);
    }
  else if (token.type == LSTOKEN_EOI)
    {
      unexpected_linespec_error (parser);
    }
  /* If it is a LSTOKEN_NUMBER, we have an offset.  */
  else if (token.type == LSTOKEN_NUMBER)
    {
      set_completion_after_number (parser, linespec_complete_what::KEYWORD);

      /* Record the line offset and get the next token.  */
      name = copy_token_string (token);
      PARSER_EXPLICIT (parser)->line_offset
	= linespec_parse_line_offset (name.get ());

      /* Get the next token.  */
      token = linespec_lexer_consume_token (parser);

      /* If the next token is a comma, stop parsing and return.  */
      if (token.type == LSTOKEN_COMMA)
	{
	  parser->complete_what = linespec_complete_what::NOTHING;
	  return;
	}

      /* If the next token is anything but EOI or KEYWORD, issue
	 an error.  */
      if (token.type != LSTOKEN_KEYWORD && token.type != LSTOKEN_EOI)
	unexpected_linespec_error (parser);
    }

  if (token.type == LSTOKEN_KEYWORD || token.type == LSTOKEN_EOI)
    return;

  /* Next token must be LSTOKEN_STRING.  */
  if (token.type != LSTOKEN_STRING)
    {
      parser->complete_what = linespec_complete_what::NOTHING;
      unexpected_linespec_error (parser);
    }

  /* The current token will contain the name of a function, method,
     or label.  */
  name = copy_token_string (token);

  if (parser->completion_tracker != NULL)
    {
      /* If the function name ends with a ":", then this may be an
	 incomplete "::" scope operator instead of a label separator.
	 E.g.,
	   "b klass:<tab>"
	 which should expand to:
	   "b klass::method()"

	 Do a tentative completion assuming the later.  If we find
	 completions, advance the stream past the colon token and make
	 it part of the function name/token.  */

      if (!parser->completion_quote_char
	  && strcmp (PARSER_STREAM (parser), ":") == 0)
	{
	  completion_tracker tmp_tracker (false);
	  const char *source_filename
	    = PARSER_EXPLICIT (parser)->source_filename.get ();
	  symbol_name_match_type match_type
	    = PARSER_EXPLICIT (parser)->func_name_match_type;

	  linespec_complete_function (tmp_tracker,
				      parser->completion_word,
				      match_type,
				      source_filename);

	  if (tmp_tracker.have_completions ())
	    {
	      PARSER_STREAM (parser)++;
	      LS_TOKEN_STOKEN (token).length++;

	      name.reset (savestring (parser->completion_word,
				      (PARSER_STREAM (parser)
				       - parser->completion_word)));
	    }
	}

      PARSER_EXPLICIT (parser)->function_name = std::move (name);
    }
  else
    {
      std::vector<block_symbol> symbols;
      std::vector<bound_minimal_symbol> minimal_symbols;

      /* Try looking it up as a function/method.  */
      find_linespec_symbols (PARSER_STATE (parser),
			     PARSER_RESULT (parser)->file_symtabs, name.get (),
			     PARSER_EXPLICIT (parser)->func_name_match_type,
			     &symbols, &minimal_symbols);

      if (!symbols.empty () || !minimal_symbols.empty ())
	{
	  PARSER_RESULT (parser)->function_symbols = std::move (symbols);
	  PARSER_RESULT (parser)->minimal_symbols = std::move (minimal_symbols);
	  PARSER_EXPLICIT (parser)->function_name = std::move (name);
	}
      else
	{
	  /* NAME was not a function or a method.  So it must be a label
	     name or user specified variable like "break foo.c:$zippo".  */
	  std::vector<block_symbol> labels
	    = find_label_symbols (PARSER_STATE (parser), {}, &symbols,
				  name.get ());

	  if (!labels.empty ())
	    {
	      PARSER_RESULT (parser)->labels.label_symbols = std::move (labels);
	      PARSER_RESULT (parser)->labels.function_symbols
		  = std::move (symbols);
	      PARSER_EXPLICIT (parser)->label_name = std::move (name);
	    }
	  else if (token.type == LSTOKEN_STRING
		   && *LS_TOKEN_STOKEN (token).ptr == '$')
	    {
	      /* User specified a convenience variable or history value.  */
	      PARSER_EXPLICIT (parser)->line_offset
		= linespec_parse_variable (PARSER_STATE (parser), name.get ());

	      if (PARSER_EXPLICIT (parser)->line_offset.sign == LINE_OFFSET_UNKNOWN)
		{
		  /* The user-specified variable was not valid.  Do not
		     throw an error here.  parse_linespec will do it for us.  */
		  PARSER_EXPLICIT (parser)->function_name = std::move (name);
		  return;
		}
	    }
	  else
	    {
	      /* The name is also not a label.  Abort parsing.  Do not throw
		 an error here.  parse_linespec will do it for us.  */

	      /* Save a copy of the name we were trying to lookup.  */
	      PARSER_EXPLICIT (parser)->function_name = std::move (name);
	      return;
	    }
	}
    }

  int previous_qc = parser->completion_quote_char;

  /* Get the next token.  */
  token = linespec_lexer_consume_token (parser);

  if (token.type == LSTOKEN_EOI)
    {
      if (previous_qc && !parser->completion_quote_char)
	parser->complete_what = linespec_complete_what::KEYWORD;
    }
  else if (token.type == LSTOKEN_COLON)
    {
      /* User specified a label or a lineno.  */
      token = linespec_lexer_consume_token (parser);

      if (token.type == LSTOKEN_NUMBER)
	{
	  /* User specified an offset.  Record the line offset and
	     get the next token.  */
	  set_completion_after_number (parser, linespec_complete_what::KEYWORD);

	  name = copy_token_string (token);
	  PARSER_EXPLICIT (parser)->line_offset
	    = linespec_parse_line_offset (name.get ());

	  /* Get the next token.  */
	  token = linespec_lexer_consume_token (parser);
	}
      else if (token.type == LSTOKEN_EOI && parser->completion_tracker != NULL)
	{
	  parser->complete_what = linespec_complete_what::LABEL;
	}
      else if (token.type == LSTOKEN_STRING)
	{
	  parser->complete_what = linespec_complete_what::LABEL;

	  /* If we have text after the label separated by whitespace
	     (e.g., "b func():lab i<tab>"), don't consider it part of
	     the label.  In completion mode that should complete to
	     "if", in normal mode, the 'i' should be treated as
	     garbage.  */
	  if (parser->completion_quote_char == '\0')
	    {
	      const char *ptr = LS_TOKEN_STOKEN (token).ptr;
	      for (size_t i = 0; i < LS_TOKEN_STOKEN (token).length; i++)
		{
		  if (ptr[i] == ' ')
		    {
		      LS_TOKEN_STOKEN (token).length = i;
		      PARSER_STREAM (parser) = skip_spaces (ptr + i + 1);
		      break;
		    }
		}
	    }

	  if (parser->completion_tracker != NULL)
	    {
	      if (PARSER_STREAM (parser)[-1] == ' ')
		{
		  parser->completion_word = PARSER_STREAM (parser);
		  parser->complete_what = linespec_complete_what::KEYWORD;
		}
	    }
	  else
	    {
	      std::vector<block_symbol> symbols;

	      /* Grab a copy of the label's name and look it up.  */
	      name = copy_token_string (token);
	      std::vector<block_symbol> labels
		= find_label_symbols (PARSER_STATE (parser),
				      PARSER_RESULT (parser)->function_symbols,
				      &symbols, name.get ());

	      if (!labels.empty ())
		{
		  PARSER_RESULT (parser)->labels.label_symbols
		    = std::move (labels);
		  PARSER_RESULT (parser)->labels.function_symbols
		    = std::move (symbols);
		  PARSER_EXPLICIT (parser)->label_name = std::move (name);
		}
	      else
		{
		  /* We don't know what it was, but it isn't a label.  */
		  undefined_label_error
		    (PARSER_EXPLICIT (parser)->function_name.get (),
		     name.get ());
		}

	    }

	  /* Check for a line offset.  */
	  token = save_stream_and_consume_token (parser);
	  if (token.type == LSTOKEN_COLON)
	    {
	      /* Get the next token.  */
	      token = linespec_lexer_consume_token (parser);

	      /* It must be a line offset.  */
	      if (token.type != LSTOKEN_NUMBER)
		unexpected_linespec_error (parser);

	      /* Record the line offset and get the next token.  */
	      name = copy_token_string (token);

	      PARSER_EXPLICIT (parser)->line_offset
		= linespec_parse_line_offset (name.get ());

	      /* Get the next token.  */
	      token = linespec_lexer_consume_token (parser);
	    }
	}
      else
	{
	  /* Trailing ':' in the input. Issue an error.  */
	  unexpected_linespec_error (parser);
	}
    }
}

/* Canonicalize the linespec contained in LS.  The result is saved into
   STATE->canonical.  This function handles both linespec and explicit
   locations.  */

static void
canonicalize_linespec (struct linespec_state *state, const linespec *ls)
{
  /* If canonicalization was not requested, no need to do anything.  */
  if (!state->canonical)
    return;

  /* Save everything as an explicit location.  */
  state->canonical->locspec = ls->explicit_loc.clone ();
  explicit_location_spec *explicit_loc
    = as_explicit_location_spec (state->canonical->locspec.get ());

  if (explicit_loc->label_name != NULL)
    {
      state->canonical->special_display = 1;

      if (explicit_loc->function_name == NULL)
	{
	  /* No function was specified, so add the symbol name.  */
	  gdb_assert (ls->labels.function_symbols.size () == 1);
	  block_symbol s = ls->labels.function_symbols.front ();
	  explicit_loc->function_name
	    = make_unique_xstrdup (s.symbol->natural_name ());
	}
    }

  /* If this location originally came from a linespec, save a string
     representation of it for display and saving to file.  */
  if (state->is_linespec)
    explicit_loc->set_string (explicit_loc->to_linespec ());
}

/* Given a line offset in LS, construct the relevant SALs.  */

static std::vector<symtab_and_line>
create_sals_line_offset (struct linespec_state *self,
			 linespec *ls)
{
  int use_default = 0;

  /* This is where we need to make sure we have good defaults.
     We must guarantee that this section of code is never executed
     when we are called with just a function name, since
     set_default_source_symtab_and_line uses
     select_source_symtab that calls us with such an argument.  */

  if (ls->file_symtabs.size () == 1
      && ls->file_symtabs.front () == nullptr)
    {
      set_current_program_space (self->program_space);

      /* Make sure we have at least a default source line.  */
      set_default_source_symtab_and_line ();
      initialize_defaults (&self->default_symtab, &self->default_line);
      ls->file_symtabs
	= collect_symtabs_from_filename (self->default_symtab->filename,
					 self->search_pspace);
      use_default = 1;
    }

  symtab_and_line val;
  val.line = ls->explicit_loc.line_offset.offset;
  switch (ls->explicit_loc.line_offset.sign)
    {
    case LINE_OFFSET_PLUS:
      if (ls->explicit_loc.line_offset.offset == 0)
	val.line = 5;
      if (use_default)
	val.line = self->default_line + val.line;
      break;

    case LINE_OFFSET_MINUS:
      if (ls->explicit_loc.line_offset.offset == 0)
	val.line = 15;
      if (use_default)
	val.line = self->default_line - val.line;
      else
	val.line = -val.line;
      break;

    case LINE_OFFSET_NONE:
      break;			/* No need to adjust val.line.  */
    }

  std::vector<symtab_and_line> values;
  if (self->list_mode)
    values = decode_digits_list_mode (self, ls, val);
  else
    {
      const linetable_entry *best_entry = NULL;
      int i, j;

      std::vector<symtab_and_line> intermediate_results
	= decode_digits_ordinary (self, ls, val.line, &best_entry);
      if (intermediate_results.empty () && best_entry != NULL)
	intermediate_results = decode_digits_ordinary (self, ls,
						       best_entry->line,
						       &best_entry);

      /* For optimized code, the compiler can scatter one source line
	 across disjoint ranges of PC values, even when no duplicate
	 functions or inline functions are involved.  For example,
	 'for (;;)' inside a non-template, non-inline, and non-ctor-or-dtor
	 function can result in two PC ranges.  In this case, we don't
	 want to set a breakpoint on the first PC of each range.  To filter
	 such cases, we use containing blocks -- for each PC found
	 above, we see if there are other PCs that are in the same
	 block.  If yes, the other PCs are filtered out.  */

      gdb::def_vector<int> filter (intermediate_results.size ());
      gdb::def_vector<const block *> blocks (intermediate_results.size ());

      for (i = 0; i < intermediate_results.size (); ++i)
	{
	  set_current_program_space (intermediate_results[i].pspace);

	  filter[i] = 1;
	  blocks[i] = block_for_pc_sect (intermediate_results[i].pc,
					 intermediate_results[i].section);
	}

      for (i = 0; i < intermediate_results.size (); ++i)
	{
	  if (blocks[i] != NULL)
	    for (j = i + 1; j < intermediate_results.size (); ++j)
	      {
		if (blocks[j] == blocks[i])
		  {
		    filter[j] = 0;
		    break;
		  }
	      }
	}

      for (i = 0; i < intermediate_results.size (); ++i)
	if (filter[i])
	  {
	    struct symbol *sym = (blocks[i]
				  ? blocks[i]->containing_function ()
				  : NULL);

	    if (self->funfirstline)
	      skip_prologue_sal (&intermediate_results[i]);
	    intermediate_results[i].symbol = sym;
	    add_sal_to_sals (self, &values, &intermediate_results[i],
			     sym ? sym->natural_name () : NULL, 0);
	  }
    }

  if (values.empty ())
    {
      if (ls->explicit_loc.source_filename)
	throw_error (NOT_FOUND_ERROR, _("No line %d in file \"%s\"."),
		     val.line, ls->explicit_loc.source_filename.get ());
      else
	throw_error (NOT_FOUND_ERROR, _("No line %d in the current file."),
		     val.line);
    }

  return values;
}

/* Convert the given ADDRESS into SaLs.  */

static std::vector<symtab_and_line>
convert_address_location_to_sals (struct linespec_state *self,
				  CORE_ADDR address)
{
  symtab_and_line sal = find_pc_line (address, 0);
  sal.pc = address;
  sal.section = find_pc_overlay (address);
  sal.explicit_pc = 1;
  sal.symbol = find_pc_sect_containing_function (sal.pc, sal.section);

  std::vector<symtab_and_line> sals;
  add_sal_to_sals (self, &sals, &sal, core_addr_to_string (address), 1);

  return sals;
}

/* Create and return SALs from the linespec LS.  */

static std::vector<symtab_and_line>
convert_linespec_to_sals (struct linespec_state *state, linespec *ls)
{
  std::vector<symtab_and_line> sals;

  if (!ls->labels.label_symbols.empty ())
    {
      /* We have just a bunch of functions/methods or labels.  */
      struct symtab_and_line sal;

      for (const auto &sym : ls->labels.label_symbols)
	{
	  struct program_space *pspace
	    = sym.symbol->symtab ()->compunit ()->objfile ()->pspace;

	  if (symbol_to_sal (&sal, state->funfirstline, sym.symbol)
	      && maybe_add_address (state->addr_set, pspace, sal.pc))
	    add_sal_to_sals (state, &sals, &sal,
			     sym.symbol->natural_name (), 0);
	}
    }
  else if (!ls->function_symbols.empty () || !ls->minimal_symbols.empty ())
    {
      /* We have just a bunch of functions and/or methods.  */
      if (!ls->function_symbols.empty ())
	{
	  /* Sort symbols so that symbols with the same program space are next
	     to each other.  */
	  std::sort (ls->function_symbols.begin (),
		     ls->function_symbols.end (),
		     compare_symbols);

	  for (const auto &sym : ls->function_symbols)
	    {
	      program_space *pspace
		= sym.symbol->symtab ()->compunit ()->objfile ()->pspace;
	      set_current_program_space (pspace);

	      /* Don't skip to the first line of the function if we
		 had found an ifunc minimal symbol for this function,
		 because that means that this function is an ifunc
		 resolver with the same name as the ifunc itself.  */
	      bool found_ifunc = false;

	      if (state->funfirstline
		   && !ls->minimal_symbols.empty ()
		   && sym.symbol->aclass () == LOC_BLOCK)
		{
		  const CORE_ADDR addr
		    = sym.symbol->value_block ()->entry_pc ();

		  for (const auto &elem : ls->minimal_symbols)
		    {
		      if (elem.minsym->type () == mst_text_gnu_ifunc
			  || elem.minsym->type () == mst_data_gnu_ifunc)
			{
			  CORE_ADDR msym_addr = elem.value_address ();
			  if (elem.minsym->type () == mst_data_gnu_ifunc)
			    {
			      struct gdbarch *gdbarch
				= elem.objfile->arch ();
			      msym_addr
				= (gdbarch_convert_from_func_ptr_addr
				   (gdbarch,
				    msym_addr,
				    current_inferior ()->top_target ()));
			    }

			  if (msym_addr == addr)
			    {
			      found_ifunc = true;
			      break;
			    }
			}
		    }
		}

	      if (!found_ifunc)
		{
		  symtab_and_line sal;
		  if (symbol_to_sal (&sal, state->funfirstline, sym.symbol)
		      && maybe_add_address (state->addr_set, pspace, sal.pc))
		    add_sal_to_sals (state, &sals, &sal,
				     sym.symbol->natural_name (), 0);
		}
	    }
	}

      if (!ls->minimal_symbols.empty ())
	{
	  /* Sort minimal symbols by program space, too  */
	  std::sort (ls->minimal_symbols.begin (),
		     ls->minimal_symbols.end (),
		     compare_msymbols);

	  for (const auto &elem : ls->minimal_symbols)
	    {
	      program_space *pspace = elem.objfile->pspace;
	      set_current_program_space (pspace);
	      minsym_found (state, elem.objfile, elem.minsym, &sals);
	    }
	}
    }
  else if (ls->explicit_loc.line_offset.sign != LINE_OFFSET_UNKNOWN)
    {
      /* Only an offset was specified.  */
	sals = create_sals_line_offset (state, ls);

	/* Make sure we have a filename for canonicalization.  */
	if (ls->explicit_loc.source_filename == NULL)
	  {
	    const char *filename = state->default_symtab->filename;

	    /* It may be more appropriate to keep DEFAULT_SYMTAB in its symtab
	       form so that displaying SOURCE_FILENAME can follow the current
	       FILENAME_DISPLAY_STRING setting.  But as it is used only rarely
	       it has been kept for code simplicity only in absolute form.  */
	    ls->explicit_loc.source_filename = make_unique_xstrdup (filename);
	  }
    }
  else
    {
      /* We haven't found any results...  */
      return sals;
    }

  canonicalize_linespec (state, ls);

  if (!sals.empty () && state->canonical != NULL)
    state->canonical->pre_expanded = 1;

  return sals;
}

/* Build RESULT from the explicit location spec components
   SOURCE_FILENAME, FUNCTION_NAME, LABEL_NAME and LINE_OFFSET.  */

static void
convert_explicit_location_spec_to_linespec
  (struct linespec_state *self,
   linespec *result,
   const char *source_filename,
   const char *function_name,
   symbol_name_match_type fname_match_type,
   const char *label_name,
   struct line_offset line_offset)
{
  std::vector<bound_minimal_symbol> minimal_symbols;

  result->explicit_loc.func_name_match_type = fname_match_type;

  if (source_filename != NULL)
    {
      try
	{
	  result->file_symtabs
	    = symtabs_from_filename (source_filename, self->search_pspace);
	}
      catch (const gdb_exception_error &except)
	{
	  source_file_not_found_error (source_filename);
	}
      result->explicit_loc.source_filename
	= make_unique_xstrdup (source_filename);
    }
  else
    {
      /* A NULL entry means to use the default symtab.  */
      result->file_symtabs.push_back (nullptr);
    }

  if (function_name != NULL)
    {
      std::vector<block_symbol> symbols;

      find_linespec_symbols (self, result->file_symtabs,
			     function_name, fname_match_type,
			     &symbols, &minimal_symbols);

      if (symbols.empty () && minimal_symbols.empty ())
	symbol_not_found_error (function_name,
				result->explicit_loc.source_filename.get ());

      result->explicit_loc.function_name
	= make_unique_xstrdup (function_name);
      result->function_symbols = std::move (symbols);
      result->minimal_symbols = std::move (minimal_symbols);
    }

  if (label_name != NULL)
    {
      std::vector<block_symbol> symbols;
      std::vector<block_symbol> labels
	= find_label_symbols (self, result->function_symbols,
			      &symbols, label_name);

      if (labels.empty ())
	undefined_label_error (result->explicit_loc.function_name.get (),
			       label_name);

      result->explicit_loc.label_name = make_unique_xstrdup (label_name);
      result->labels.label_symbols = labels;
      result->labels.function_symbols = std::move (symbols);
    }

  if (line_offset.sign != LINE_OFFSET_UNKNOWN)
    result->explicit_loc.line_offset = line_offset;
}

/* Convert the explicit location EXPLICIT_LOC into SaLs.  */

static std::vector<symtab_and_line>
convert_explicit_location_spec_to_sals
  (struct linespec_state *self,
   linespec *result,
   const explicit_location_spec *explicit_spec)
{
  convert_explicit_location_spec_to_linespec (self, result,
					      explicit_spec->source_filename.get (),
					      explicit_spec->function_name.get (),
					      explicit_spec->func_name_match_type,
					      explicit_spec->label_name.get (),
					      explicit_spec->line_offset);
  return convert_linespec_to_sals (self, result);
}

/* Parse a string that specifies a linespec.

   The basic grammar of linespecs:

   linespec -> var_spec | basic_spec
   var_spec -> '$' (STRING | NUMBER)

   basic_spec -> file_offset_spec | function_spec | label_spec
   file_offset_spec -> opt_file_spec offset_spec
   function_spec -> opt_file_spec function_name_spec opt_label_spec
   label_spec -> label_name_spec

   opt_file_spec -> "" | file_name_spec ':'
   opt_label_spec -> "" | ':' label_name_spec

   file_name_spec -> STRING
   function_name_spec -> STRING
   label_name_spec -> STRING
   function_name_spec -> STRING
   offset_spec -> NUMBER
	       -> '+' NUMBER
	       -> '-' NUMBER

   This may all be followed by several keywords such as "if EXPR",
   which we ignore.

   A comma will terminate parsing.

   The function may be an undebuggable function found in minimal symbol table.

   If the argument FUNFIRSTLINE is nonzero, we want the first line
   of real code inside a function when a function is specified, and it is
   not OK to specify a variable or type to get its line number.

   DEFAULT_SYMTAB specifies the file to use if none is specified.
   It defaults to current_source_symtab.
   DEFAULT_LINE specifies the line number to use for relative
   line numbers (that start with signs).  Defaults to current_source_line.
   If CANONICAL is non-NULL, store an array of strings containing the canonical
   line specs there if necessary.  Currently overloaded member functions and
   line numbers or static functions without a filename yield a canonical
   line spec.  The array and the line spec strings are allocated on the heap,
   it is the callers responsibility to free them.

   Note that it is possible to return zero for the symtab
   if no file is validly specified.  Callers must check that.
   Also, the line number returned may be invalid.  */

/* Parse the linespec in ARG, which must not be nullptr.  MATCH_TYPE
   indicates how function names should be matched.  */

static std::vector<symtab_and_line>
parse_linespec (linespec_parser *parser, const char *arg,
		symbol_name_match_type match_type)
{
  gdb_assert (arg != nullptr);

  struct gdb_exception file_exception;

  /* A special case to start.  It has become quite popular for
     IDEs to work around bugs in the previous parser by quoting
     the entire linespec, so we attempt to deal with this nicely.  */
  parser->is_quote_enclosed = 0;
  if (parser->completion_tracker == NULL
      && !is_ada_operator (arg)
      && *arg != '\0'
      && strchr (linespec_quote_characters, *arg) != NULL)
    {
      const char *end = skip_quote_char (arg + 1, *arg);
      if (end != NULL && is_closing_quote_enclosed (end))
	{
	  /* Here's the special case.  Skip ARG past the initial
	     quote.  */
	  ++arg;
	  parser->is_quote_enclosed = 1;
	}
    }

  parser->lexer.saved_arg = arg;
  parser->lexer.stream = arg;
  parser->completion_word = arg;
  parser->complete_what = linespec_complete_what::FUNCTION;
  PARSER_EXPLICIT (parser)->func_name_match_type = match_type;

  /* Initialize the default symtab and line offset.  */
  initialize_defaults (&PARSER_STATE (parser)->default_symtab,
		       &PARSER_STATE (parser)->default_line);

  /* Objective-C shortcut.  */
  if (parser->completion_tracker == NULL)
    {
      std::vector<symtab_and_line> values
	= decode_objc (PARSER_STATE (parser), PARSER_RESULT (parser), arg);
      if (!values.empty ())
	return values;
    }
  else
    {
      /* "-"/"+" is either an objc selector, or a number.  There's
	 nothing to complete the latter to, so just let the caller
	 complete on functions, which finds objc selectors, if there's
	 any.  */
      if ((arg[0] == '-' || arg[0] == '+') && arg[1] == '\0')
	return {};
    }

  /* Start parsing.  */

  /* Get the first token.  */
  linespec_token token = linespec_lexer_consume_token (parser);

  /* It must be either LSTOKEN_STRING or LSTOKEN_NUMBER.  */
  if (token.type == LSTOKEN_STRING && *LS_TOKEN_STOKEN (token).ptr == '$')
    {
      /* A NULL entry means to use GLOBAL_DEFAULT_SYMTAB.  */
      if (parser->completion_tracker == NULL)
	PARSER_RESULT (parser)->file_symtabs.push_back (nullptr);

      /* User specified a convenience variable or history value.  */
      gdb::unique_xmalloc_ptr<char> var = copy_token_string (token);
      PARSER_EXPLICIT (parser)->line_offset
	= linespec_parse_variable (PARSER_STATE (parser), var.get ());

      /* If a line_offset wasn't found (VAR is the name of a user
	 variable/function), then skip to normal symbol processing.  */
      if (PARSER_EXPLICIT (parser)->line_offset.sign != LINE_OFFSET_UNKNOWN)
	{
	  /* Consume this token.  */
	  linespec_lexer_consume_token (parser);

	  goto convert_to_sals;
	}
    }
  else if (token.type == LSTOKEN_EOI && parser->completion_tracker != NULL)
    {
      /* Let the default linespec_complete_what::FUNCTION kick in.  */
      unexpected_linespec_error (parser);
    }
  else if (token.type != LSTOKEN_STRING && token.type != LSTOKEN_NUMBER)
    {
      parser->complete_what = linespec_complete_what::NOTHING;
      unexpected_linespec_error (parser);
    }

  /* Shortcut: If the next token is not LSTOKEN_COLON, we know that
     this token cannot represent a filename.  */
  token = linespec_lexer_peek_token (parser);

  if (token.type == LSTOKEN_COLON)
    {
      /* Get the current token again and extract the filename.  */
      token = linespec_lexer_lex_one (parser);
      gdb::unique_xmalloc_ptr<char> user_filename = copy_token_string (token);

      /* Check if the input is a filename.  */
      try
	{
	  PARSER_RESULT (parser)->file_symtabs
	    = symtabs_from_filename (user_filename.get (),
				     PARSER_STATE (parser)->search_pspace);
	}
      catch (gdb_exception_error &ex)
	{
	  file_exception = std::move (ex);
	}

      if (file_exception.reason >= 0)
	{
	  /* Symtabs were found for the file.  Record the filename.  */
	  PARSER_EXPLICIT (parser)->source_filename = std::move (user_filename);

	  /* Get the next token.  */
	  token = linespec_lexer_consume_token (parser);

	  /* This is LSTOKEN_COLON; consume it.  */
	  linespec_lexer_consume_token (parser);
	}
      else
	{
	  /* A NULL entry means to use GLOBAL_DEFAULT_SYMTAB.  */
	  PARSER_RESULT (parser)->file_symtabs.push_back (nullptr);
	}
    }
  /* If the next token is not EOI, KEYWORD, or COMMA, issue an error.  */
  else if (parser->completion_tracker == NULL
	   && (token.type != LSTOKEN_EOI && token.type != LSTOKEN_KEYWORD
	       && token.type != LSTOKEN_COMMA))
    {
      /* TOKEN is the _next_ token, not the one currently in the parser.
	 Consuming the token will give the correct error message.  */
      linespec_lexer_consume_token (parser);
      unexpected_linespec_error (parser);
    }
  else
    {
      /* A NULL entry means to use GLOBAL_DEFAULT_SYMTAB.  */
      PARSER_RESULT (parser)->file_symtabs.push_back (nullptr);
    }

  /* Parse the rest of the linespec.  */
  linespec_parse_basic (parser);

  if (parser->completion_tracker == NULL
      && PARSER_RESULT (parser)->function_symbols.empty ()
      && PARSER_RESULT (parser)->labels.label_symbols.empty ()
      && PARSER_EXPLICIT (parser)->line_offset.sign == LINE_OFFSET_UNKNOWN
      && PARSER_RESULT (parser)->minimal_symbols.empty ())
    {
      /* The linespec didn't parse.  Re-throw the file exception if
	 there was one.  */
      if (file_exception.reason < 0)
	throw_exception (std::move (file_exception));

      /* Otherwise, the symbol is not found.  */
      symbol_not_found_error
	(PARSER_EXPLICIT (parser)->function_name.get (),
	 PARSER_EXPLICIT (parser)->source_filename.get ());
    }

 convert_to_sals:

  /* Get the last token and record how much of the input was parsed,
     if necessary.  */
  token = linespec_lexer_lex_one (parser);
  if (token.type != LSTOKEN_EOI && token.type != LSTOKEN_KEYWORD)
    unexpected_linespec_error (parser);
  else if (token.type == LSTOKEN_KEYWORD)
    {
      /* Setup the completion word past the keyword.  Lexing never
	 advances past a keyword automatically, so skip it
	 manually.  */
      parser->completion_word
	= skip_spaces (skip_to_space (PARSER_STREAM (parser)));
      parser->complete_what = linespec_complete_what::EXPRESSION;
    }

  /* Convert the data in PARSER_RESULT to SALs.  */
  if (parser->completion_tracker == NULL)
    return convert_linespec_to_sals (PARSER_STATE (parser),
				     PARSER_RESULT (parser));

  return {};
}


/* A constructor for linespec_state.  */

static void
linespec_state_constructor (struct linespec_state *self,
			    int flags, const struct language_defn *language,
			    struct program_space *search_pspace,
			    struct symtab *default_symtab,
			    int default_line,
			    struct linespec_result *canonical)
{
  memset (self, 0, sizeof (*self));
  self->language = language;
  self->funfirstline = (flags & DECODE_LINE_FUNFIRSTLINE) ? 1 : 0;
  self->list_mode = (flags & DECODE_LINE_LIST_MODE) ? 1 : 0;
  self->search_pspace = search_pspace;
  self->default_symtab = default_symtab;
  self->default_line = default_line;
  self->canonical = canonical;
  self->program_space = current_program_space;
  self->addr_set = htab_create_alloc (10, hash_address_entry, eq_address_entry,
				      xfree, xcalloc, xfree);
  self->is_linespec = 0;
}

/* Initialize a new linespec parser.  */

linespec_parser::linespec_parser (int flags,
				  const struct language_defn *language,
				  struct program_space *search_pspace,
				  struct symtab *default_symtab,
				  int default_line,
				  struct linespec_result *canonical)
{
  lexer.current.type = LSTOKEN_CONSUMED;
  PARSER_EXPLICIT (this)->func_name_match_type
    = symbol_name_match_type::WILD;
  PARSER_EXPLICIT (this)->line_offset.sign = LINE_OFFSET_UNKNOWN;
  linespec_state_constructor (PARSER_STATE (this), flags, language,
			      search_pspace,
			      default_symtab, default_line, canonical);
}

/* A destructor for linespec_state.  */

static void
linespec_state_destructor (struct linespec_state *self)
{
  htab_delete (self->addr_set);
  xfree (self->canonical_names);
}

/* Delete a linespec parser.  */

linespec_parser::~linespec_parser ()
{
  linespec_state_destructor (PARSER_STATE (this));
}

/* See description in linespec.h.  */

void
linespec_lex_to_end (const char **stringp)
{
  linespec_token token;
  const char *orig;

  if (stringp == NULL || *stringp == NULL)
    return;

  linespec_parser parser (0, current_language, NULL, NULL, 0, NULL);
  parser.lexer.saved_arg = *stringp;
  PARSER_STREAM (&parser) = orig = *stringp;

  do
    {
      /* Stop before any comma tokens;  we need it to keep it
	 as the next token in the string.  */
      token = linespec_lexer_peek_token (&parser);
      if (token.type == LSTOKEN_COMMA)
	break;
      token = linespec_lexer_consume_token (&parser);
    }
  while (token.type != LSTOKEN_EOI && token.type != LSTOKEN_KEYWORD);

  *stringp += PARSER_STREAM (&parser) - orig;
}

/* See linespec.h.  */

void
linespec_complete_function (completion_tracker &tracker,
			    const char *function,
			    symbol_name_match_type func_match_type,
			    const char *source_filename)
{
  complete_symbol_mode mode = complete_symbol_mode::LINESPEC;

  if (source_filename != NULL)
    {
      collect_file_symbol_completion_matches (tracker, mode, func_match_type,
					      function, function, source_filename);
    }
  else
    {
      collect_symbol_completion_matches (tracker, mode, func_match_type,
					 function, function);

    }
}

/* Helper for complete_linespec to simplify it.  SOURCE_FILENAME is
   only meaningful if COMPONENT is FUNCTION.  */

static void
complete_linespec_component (linespec_parser *parser,
			     completion_tracker &tracker,
			     const char *text,
			     linespec_complete_what component,
			     const char *source_filename)
{
  if (component == linespec_complete_what::KEYWORD)
    {
      complete_on_enum (tracker, linespec_keywords, text, text);
    }
  else if (component == linespec_complete_what::EXPRESSION)
    {
      const char *word
	= advance_to_expression_complete_word_point (tracker, text);
      complete_expression (tracker, text, word);
    }
  else if (component == linespec_complete_what::FUNCTION)
    {
      completion_list fn_list;

      symbol_name_match_type match_type
	= PARSER_EXPLICIT (parser)->func_name_match_type;
      linespec_complete_function (tracker, text, match_type, source_filename);
      if (source_filename == NULL)
	{
	  /* Haven't seen a source component, like in "b
	     file.c:function[TAB]".  Maybe this wasn't a function, but
	     a filename instead, like "b file.[TAB]".  */
	  fn_list = complete_source_filenames (text);
	}

      /* If we only have a single filename completion, append a ':' for
	 the user, since that's the only thing that can usefully follow
	 the filename.  */
      if (fn_list.size () == 1 && !tracker.have_completions ())
	{
	  char *fn = fn_list[0].release ();

	  /* If we also need to append a quote char, it needs to be
	     appended before the ':'.  Append it now, and make ':' the
	     new "quote" char.  */
	  if (tracker.quote_char ())
	    {
	      char quote_char_str[2] = { (char) tracker.quote_char () };

	      fn = reconcat (fn, fn, quote_char_str, (char *) NULL);
	      tracker.set_quote_char (':');
	    }
	  else
	    fn = reconcat (fn, fn, ":", (char *) NULL);
	  fn_list[0].reset (fn);

	  /* Tell readline to skip appending a space.  */
	  tracker.set_suppress_append_ws (true);
	}
      tracker.add_completions (std::move (fn_list));
    }
}

/* Helper for linespec_complete_label.  Find labels that match
   LABEL_NAME in the function symbols listed in the PARSER, and add
   them to the tracker.  */

static void
complete_label (completion_tracker &tracker,
		linespec_parser *parser,
		const char *label_name)
{
  std::vector<block_symbol> label_function_symbols;
  std::vector<block_symbol> labels
    = find_label_symbols (PARSER_STATE (parser),
			  PARSER_RESULT (parser)->function_symbols,
			  &label_function_symbols,
			  label_name, true);

  for (const auto &label : labels)
    {
      char *match = xstrdup (label.symbol->search_name ());
      tracker.add_completion (gdb::unique_xmalloc_ptr<char> (match));
    }
}

/* See linespec.h.  */

void
linespec_complete_label (completion_tracker &tracker,
			 const struct language_defn *language,
			 const char *source_filename,
			 const char *function_name,
			 symbol_name_match_type func_name_match_type,
			 const char *label_name)
{
  linespec_parser parser (0, language, NULL, NULL, 0, NULL);

  line_offset unknown_offset;

  try
    {
      convert_explicit_location_spec_to_linespec (PARSER_STATE (&parser),
						  PARSER_RESULT (&parser),
						  source_filename,
						  function_name,
						  func_name_match_type,
						  NULL, unknown_offset);
    }
  catch (const gdb_exception_error &ex)
    {
      return;
    }

  complete_label (tracker, &parser, label_name);
}

/* See description in linespec.h.  */

void
linespec_complete (completion_tracker &tracker, const char *text,
		   symbol_name_match_type match_type)
{
  const char *orig = text;

  linespec_parser parser (0, current_language, NULL, NULL, 0, NULL);
  parser.lexer.saved_arg = text;
  PARSER_EXPLICIT (&parser)->func_name_match_type = match_type;
  PARSER_STREAM (&parser) = text;

  parser.completion_tracker = &tracker;
  PARSER_STATE (&parser)->is_linespec = 1;

  /* Parse as much as possible.  parser.completion_word will hold
     furthest completion point we managed to parse to.  */
  try
    {
      parse_linespec (&parser, text, match_type);
    }
  catch (const gdb_exception_error &except)
    {
    }

  if (parser.completion_quote_char != '\0'
      && parser.completion_quote_end != NULL
      && parser.completion_quote_end[1] == '\0')
    {
      /* If completing a quoted string with the cursor right at
	 terminating quote char, complete the completion word without
	 interpretation, so that readline advances the cursor one
	 whitespace past the quote, even if there's no match.  This
	 makes these cases behave the same:

	   before: "b function()"
	   after:  "b function() "

	   before: "b 'function()'"
	   after:  "b 'function()' "

	 and trusts the user in this case:

	   before: "b 'not_loaded_function_yet()'"
	   after:  "b 'not_loaded_function_yet()' "
      */
      parser.complete_what = linespec_complete_what::NOTHING;
      parser.completion_quote_char = '\0';

      gdb::unique_xmalloc_ptr<char> text_copy
	(xstrdup (parser.completion_word));
      tracker.add_completion (std::move (text_copy));
    }

  tracker.set_quote_char (parser.completion_quote_char);

  if (parser.complete_what == linespec_complete_what::LABEL)
    {
      parser.complete_what = linespec_complete_what::NOTHING;

      const char *func_name = PARSER_EXPLICIT (&parser)->function_name.get ();

      std::vector<block_symbol> function_symbols;
      std::vector<bound_minimal_symbol> minimal_symbols;
      find_linespec_symbols (PARSER_STATE (&parser),
			     PARSER_RESULT (&parser)->file_symtabs,
			     func_name, match_type,
			     &function_symbols, &minimal_symbols);

      PARSER_RESULT (&parser)->function_symbols = std::move (function_symbols);
      PARSER_RESULT (&parser)->minimal_symbols = std::move (minimal_symbols);

      complete_label (tracker, &parser, parser.completion_word);
    }
  else if (parser.complete_what == linespec_complete_what::FUNCTION)
    {
      /* While parsing/lexing, we didn't know whether the completion
	 word completes to a unique function/source name already or
	 not.

	 E.g.:
	   "b function() <tab>"
	 may need to complete either to:
	   "b function() const"
	 or to:
	   "b function() if/thread/task"

	 Or, this:
	   "b foo t"
	 may need to complete either to:
	   "b foo template_fun<T>()"
	 with "foo" being the template function's return type, or to:
	   "b foo thread/task"

	 Or, this:
	   "b file<TAB>"
	 may need to complete either to a source file name:
	   "b file.c"
	 or this, also a filename, but a unique completion:
	   "b file.c:"
	 or to a function name:
	   "b file_function"

	 Address that by completing assuming source or function, and
	 seeing if we find a completion that matches exactly the
	 completion word.  If so, then it must be a function (see note
	 below) and we advance the completion word to the end of input
	 and switch to KEYWORD completion mode.

	 Note: if we find a unique completion for a source filename,
	 then it won't match the completion word, because the LCD will
	 contain a trailing ':'.  And if we're completing at or after
	 the ':', then complete_linespec_component won't try to
	 complete on source filenames.  */

      const char *word = parser.completion_word;

      complete_linespec_component
	(&parser, tracker,
	 parser.completion_word,
	 linespec_complete_what::FUNCTION,
	 PARSER_EXPLICIT (&parser)->source_filename.get ());

      parser.complete_what = linespec_complete_what::NOTHING;

      if (tracker.quote_char ())
	{
	  /* The function/file name was not close-quoted, so this
	     can't be a keyword.  Note: complete_linespec_component
	     may have swapped the original quote char for ':' when we
	     get here, but that still indicates the same.  */
	}
      else if (!tracker.have_completions ())
	{
	  size_t key_start;
	  size_t wordlen = strlen (parser.completion_word);

	  key_start
	    = string_find_incomplete_keyword_at_end (linespec_keywords,
						     parser.completion_word,
						     wordlen);

	  if (key_start != -1
	      || (wordlen > 0
		  && parser.completion_word[wordlen - 1] == ' '))
	    {
	      parser.completion_word += key_start;
	      parser.complete_what = linespec_complete_what::KEYWORD;
	    }
	}
      else if (tracker.completes_to_completion_word (word))
	{
	  /* Skip the function and complete on keywords.  */
	  parser.completion_word += strlen (word);
	  parser.complete_what = linespec_complete_what::KEYWORD;
	  tracker.discard_completions ();
	}
    }

  tracker.advance_custom_word_point_by (parser.completion_word - orig);

  complete_linespec_component
    (&parser, tracker,
     parser.completion_word,
     parser.complete_what,
     PARSER_EXPLICIT (&parser)->source_filename.get ());

  /* If we're past the "filename:function:label:offset" linespec, and
     didn't find any match, then assume the user might want to create
     a pending breakpoint anyway and offer the keyword
     completions.  */
  if (!parser.completion_quote_char
      && (parser.complete_what == linespec_complete_what::FUNCTION
	  || parser.complete_what == linespec_complete_what::LABEL
	  || parser.complete_what == linespec_complete_what::NOTHING)
      && !tracker.have_completions ())
    {
      const char *end
	= parser.completion_word + strlen (parser.completion_word);

      if (end > orig && end[-1] == ' ')
	{
	  tracker.advance_custom_word_point_by (end - parser.completion_word);

	  complete_linespec_component (&parser, tracker, end,
				       linespec_complete_what::KEYWORD,
				       NULL);
	}
    }
}

/* A helper function for decode_line_full and decode_line_1 to
   turn LOCSPEC into std::vector<symtab_and_line>.  */

static std::vector<symtab_and_line>
location_spec_to_sals (linespec_parser *parser,
		       const location_spec *locspec)
{
  std::vector<symtab_and_line> result;

  switch (locspec->type ())
    {
    case LINESPEC_LOCATION_SPEC:
      {
	const linespec_location_spec *ls = as_linespec_location_spec (locspec);
	PARSER_STATE (parser)->is_linespec = 1;
	result = parse_linespec (parser, ls->spec_string.get (),
				 ls->match_type);
      }
      break;

    case ADDRESS_LOCATION_SPEC:
      {
	const address_location_spec *addr_spec
	  = as_address_location_spec (locspec);
	const char *addr_string = addr_spec->to_string ();
	CORE_ADDR addr;

	if (addr_string != NULL)
	  {
	    addr = linespec_expression_to_pc (&addr_string);
	    if (PARSER_STATE (parser)->canonical != NULL)
	      PARSER_STATE (parser)->canonical->locspec	= locspec->clone ();
	  }
	else
	  addr = addr_spec->address;

	result = convert_address_location_to_sals (PARSER_STATE (parser),
						   addr);
      }
      break;

    case EXPLICIT_LOCATION_SPEC:
      {
	const explicit_location_spec *explicit_locspec
	  = as_explicit_location_spec (locspec);
	result = convert_explicit_location_spec_to_sals (PARSER_STATE (parser),
							 PARSER_RESULT (parser),
							 explicit_locspec);
      }
      break;

    case PROBE_LOCATION_SPEC:
      /* Probes are handled by their own decoders.  */
      gdb_assert_not_reached ("attempt to decode probe location");
      break;

    default:
      gdb_assert_not_reached ("unhandled location spec type");
    }

  return result;
}

/* See linespec.h.  */

void
decode_line_full (struct location_spec *locspec, int flags,
		  struct program_space *search_pspace,
		  struct symtab *default_symtab,
		  int default_line, struct linespec_result *canonical,
		  const char *select_mode,
		  const char *filter)
{
  std::vector<const char *> filters;
  struct linespec_state *state;

  gdb_assert (canonical != NULL);
  /* The filter only makes sense for 'all'.  */
  gdb_assert (filter == NULL || select_mode == multiple_symbols_all);
  gdb_assert (select_mode == NULL
	      || select_mode == multiple_symbols_all
	      || select_mode == multiple_symbols_ask
	      || select_mode == multiple_symbols_cancel);
  gdb_assert ((flags & DECODE_LINE_LIST_MODE) == 0);

  linespec_parser parser (flags, current_language,
			  search_pspace, default_symtab,
			  default_line, canonical);

  scoped_restore_current_program_space restore_pspace;

  std::vector<symtab_and_line> result = location_spec_to_sals (&parser,
							       locspec);
  state = PARSER_STATE (&parser);

  if (result.size () == 0)
    throw_error (NOT_SUPPORTED_ERROR, _("Location %s not available"),
		 locspec->to_string ());

  gdb_assert (result.size () == 1 || canonical->pre_expanded);
  canonical->pre_expanded = 1;

  /* Arrange for allocated canonical names to be freed.  */
  std::vector<gdb::unique_xmalloc_ptr<char>> hold_names;
  for (int i = 0; i < result.size (); ++i)
    {
      gdb_assert (state->canonical_names[i].suffix != NULL);
      hold_names.emplace_back (state->canonical_names[i].suffix);
    }

  if (select_mode == NULL)
    {
      if (top_level_interpreter ()->interp_ui_out ()->is_mi_like_p ())
	select_mode = multiple_symbols_all;
      else
	select_mode = multiple_symbols_select_mode ();
    }

  if (select_mode == multiple_symbols_all)
    {
      if (filter != NULL)
	{
	  filters.push_back (filter);
	  filter_results (state, &result, filters);
	}
      else
	convert_results_to_lsals (state, &result);
    }
  else
    decode_line_2 (state, &result, select_mode);
}

/* See linespec.h.  */

std::vector<symtab_and_line>
decode_line_1 (const location_spec *locspec, int flags,
	       struct program_space *search_pspace,
	       struct symtab *default_symtab,
	       int default_line)
{
  linespec_parser parser (flags, current_language,
			  search_pspace, default_symtab,
			  default_line, NULL);

  scoped_restore_current_program_space restore_pspace;

  return location_spec_to_sals (&parser, locspec);
}

/* See linespec.h.  */

std::vector<symtab_and_line>
decode_line_with_current_source (const char *string, int flags)
{
  if (string == 0)
    error (_("Empty line specification."));

  /* We use whatever is set as the current source line.  We do not try
     and get a default source symtab+line or it will recursively call us!  */
  symtab_and_line cursal = get_current_source_symtab_and_line ();

  location_spec_up locspec = string_to_location_spec (&string,
						      current_language);
  std::vector<symtab_and_line> sals
    = decode_line_1 (locspec.get (), flags, cursal.pspace, cursal.symtab,
		    cursal.line);

  if (*string)
    error (_("Junk at end of line specification: %s"), string);

  return sals;
}

/* See linespec.h.  */

std::vector<symtab_and_line>
decode_line_with_last_displayed (const char *string, int flags)
{
  if (string == 0)
    error (_("Empty line specification."));

  location_spec_up locspec = string_to_location_spec (&string,
						      current_language);
  std::vector<symtab_and_line> sals
    = (last_displayed_sal_is_valid ()
       ? decode_line_1 (locspec.get (), flags, NULL,
			get_last_displayed_symtab (),
			get_last_displayed_line ())
       : decode_line_1 (locspec.get (), flags, NULL, NULL, 0));

  if (*string)
    error (_("Junk at end of line specification: %s"), string);

  return sals;
}



/* First, some functions to initialize stuff at the beginning of the
   function.  */

static void
initialize_defaults (struct symtab **default_symtab, int *default_line)
{
  if (*default_symtab == 0)
    {
      /* Use whatever we have for the default source line.  We don't use
	 get_current_or_default_symtab_and_line as it can recurse and call
	 us back!  */
      struct symtab_and_line cursal = 
	get_current_source_symtab_and_line ();
      
      *default_symtab = cursal.symtab;
      *default_line = cursal.line;
    }
}



/* Evaluate the expression pointed to by EXP_PTR into a CORE_ADDR,
   advancing EXP_PTR past any parsed text.  */

CORE_ADDR
linespec_expression_to_pc (const char **exp_ptr)
{
  if (current_program_space->executing_startup)
    /* The error message doesn't really matter, because this case
       should only hit during breakpoint reset.  */
    throw_error (NOT_FOUND_ERROR, _("cannot evaluate expressions while "
				    "program space is in startup"));

  (*exp_ptr)++;
  return value_as_address (parse_to_comma_and_eval (exp_ptr));
}



/* Here's where we recognise an Objective-C Selector.  An Objective C
   selector may be implemented by more than one class, therefore it
   may represent more than one method/function.  This gives us a
   situation somewhat analogous to C++ overloading.  If there's more
   than one method that could represent the selector, then use some of
   the existing C++ code to let the user choose one.  */

static std::vector<symtab_and_line>
decode_objc (struct linespec_state *self, linespec *ls, const char *arg)
{
  struct collect_info info;
  std::vector<const char *> symbol_names;
  const char *new_argptr;

  info.state = self;
  std::vector<symtab *> symtabs;
  symtabs.push_back (nullptr);

  info.file_symtabs = &symtabs;

  std::vector<block_symbol> symbols;
  info.result.symbols = &symbols;
  std::vector<bound_minimal_symbol> minimal_symbols;
  info.result.minimal_symbols = &minimal_symbols;

  new_argptr = find_imps (arg, &symbol_names);
  if (symbol_names.empty ())
    return {};

  add_all_symbol_names_from_pspace (&info, NULL, symbol_names,
				    FUNCTIONS_DOMAIN);

  std::vector<symtab_and_line> values;
  if (!symbols.empty () || !minimal_symbols.empty ())
    {
      char *saved_arg;

      saved_arg = (char *) alloca (new_argptr - arg + 1);
      memcpy (saved_arg, arg, new_argptr - arg);
      saved_arg[new_argptr - arg] = '\0';

      ls->explicit_loc.function_name = make_unique_xstrdup (saved_arg);
      ls->function_symbols = std::move (symbols);
      ls->minimal_symbols = std::move (minimal_symbols);
      values = convert_linespec_to_sals (self, ls);

      if (self->canonical)
	{
	  std::string holder;
	  const char *str;

	  self->canonical->pre_expanded = 1;

	  if (ls->explicit_loc.source_filename)
	    {
	      holder = string_printf ("%s:%s",
				      ls->explicit_loc.source_filename.get (),
				      saved_arg);
	      str = holder.c_str ();
	    }
	  else
	    str = saved_arg;

	  self->canonical->locspec
	    = new_linespec_location_spec (&str, symbol_name_match_type::FULL);
	}
    }

  return values;
}

namespace {

/* A function object that serves as symbol_found_callback_ftype
   callback for iterate_over_symbols.  This is used by
   lookup_prefix_sym to collect type symbols.  */
class decode_compound_collector
{
public:
  decode_compound_collector ()
    : m_unique_syms (htab_create_alloc (1, htab_hash_pointer,
					htab_eq_pointer, NULL,
					xcalloc, xfree))
  {
  }

  /* Return all symbols collected.  */
  std::vector<block_symbol> release_symbols ()
  {
    return std::move (m_symbols);
  }

  /* Callable as a symbol_found_callback_ftype callback.  */
  bool operator () (block_symbol *bsym);

private:
  /* A hash table of all symbols we found.  We use this to avoid
     adding any symbol more than once.  */
  htab_up m_unique_syms;

  /* The result vector.  */
  std::vector<block_symbol>  m_symbols;
};

bool
decode_compound_collector::operator () (block_symbol *bsym)
{
  void **slot;
  struct type *t;
  struct symbol *sym = bsym->symbol;

  if (sym->aclass () != LOC_TYPEDEF)
    return true; /* Continue iterating.  */

  t = sym->type ();
  t = check_typedef (t);
  if (t->code () != TYPE_CODE_STRUCT
      && t->code () != TYPE_CODE_UNION
      && t->code () != TYPE_CODE_NAMESPACE)
    return true; /* Continue iterating.  */

  slot = htab_find_slot (m_unique_syms.get (), sym, INSERT);
  if (!*slot)
    {
      *slot = sym;
      m_symbols.push_back (*bsym);
    }

  return true; /* Continue iterating.  */
}

} // namespace

/* Return any symbols corresponding to CLASS_NAME in FILE_SYMTABS.  */

static std::vector<block_symbol>
lookup_prefix_sym (struct linespec_state *state,
		   const std::vector<symtab *> &file_symtabs,
		   const char *class_name)
{
  decode_compound_collector collector;

  lookup_name_info lookup_name (class_name, symbol_name_match_type::FULL);

  for (const auto &elt : file_symtabs)
    {
      if (elt == nullptr)
	{
	  iterate_over_all_matching_symtabs (state, lookup_name,
					     STRUCT_DOMAIN, ALL_DOMAIN,
					     NULL, false, collector);
	  iterate_over_all_matching_symtabs (state, lookup_name,
					     VAR_DOMAIN, ALL_DOMAIN,
					     NULL, false, collector);
	}
      else
	{
	  /* Program spaces that are executing startup should have
	     been filtered out earlier.  */
	  program_space *pspace = elt->compunit ()->objfile ()->pspace;

	  gdb_assert (!pspace->executing_startup);
	  set_current_program_space (pspace);
	  iterate_over_file_blocks (elt, lookup_name, STRUCT_DOMAIN, collector);
	  iterate_over_file_blocks (elt, lookup_name, VAR_DOMAIN, collector);
	}
    }

  return collector.release_symbols ();
}

/* A std::sort comparison function for symbols.  The resulting order does
   not actually matter; we just need to be able to sort them so that
   symbols with the same program space end up next to each other.  */

static bool
compare_symbols (const block_symbol &a, const block_symbol &b)
{
  uintptr_t uia, uib;

  uia = (uintptr_t) a.symbol->symtab ()->compunit ()->objfile ()->pspace;
  uib = (uintptr_t) b.symbol->symtab ()->compunit ()->objfile ()->pspace;

  if (uia < uib)
    return true;
  if (uia > uib)
    return false;

  uia = (uintptr_t) a.symbol;
  uib = (uintptr_t) b.symbol;

  if (uia < uib)
    return true;

  return false;
}

/* Like compare_symbols but for minimal symbols.  */

static bool
compare_msymbols (const bound_minimal_symbol &a, const bound_minimal_symbol &b)
{
  uintptr_t uia, uib;

  uia = (uintptr_t) a.objfile->pspace;
  uib = (uintptr_t) a.objfile->pspace;

  if (uia < uib)
    return true;
  if (uia > uib)
    return false;

  uia = (uintptr_t) a.minsym;
  uib = (uintptr_t) b.minsym;

  if (uia < uib)
    return true;

  return false;
}

/* Look for all the matching instances of each symbol in NAMES.  Only
   instances from PSPACE are considered; other program spaces are
   handled by our caller.  If PSPACE is NULL, then all program spaces
   are considered.  Results are stored into INFO.  */

static void
add_all_symbol_names_from_pspace (struct collect_info *info,
				  struct program_space *pspace,
				  const std::vector<const char *> &names,
				  enum search_domain search_domain)
{
  for (const char *iter : names)
    add_matching_symbols_to_info (iter,
				  symbol_name_match_type::FULL,
				  search_domain, info, pspace);
}

static void
find_superclass_methods (std::vector<struct type *> &&superclasses,
			 const char *name, enum language name_lang,
			 std::vector<const char *> *result_names)
{
  size_t old_len = result_names->size ();

  while (1)
    {
      std::vector<struct type *> new_supers;

      for (type *t : superclasses)
	find_methods (t, name_lang, name, result_names, &new_supers);

      if (result_names->size () != old_len || new_supers.empty ())
	break;

      superclasses = std::move (new_supers);
    }
}

/* This finds the method METHOD_NAME in the class CLASS_NAME whose type is
   given by one of the symbols in SYM_CLASSES.  Matches are returned
   in SYMBOLS (for debug symbols) and MINSYMS (for minimal symbols).  */

static void
find_method (struct linespec_state *self,
	     const std::vector<symtab *> &file_symtabs,
	     const char *class_name, const char *method_name,
	     std::vector<block_symbol> *sym_classes,
	     std::vector<block_symbol> *symbols,
	     std::vector<bound_minimal_symbol> *minsyms)
{
  size_t last_result_len;
  std::vector<struct type *> superclass_vec;
  std::vector<const char *> result_names;
  struct collect_info info;

  /* Sort symbols so that symbols with the same program space are next
     to each other.  */
  std::sort (sym_classes->begin (), sym_classes->end (),
	     compare_symbols);

  info.state = self;
  info.file_symtabs = &file_symtabs;
  info.result.symbols = symbols;
  info.result.minimal_symbols = minsyms;

  /* Iterate over all the types, looking for the names of existing
     methods matching METHOD_NAME.  If we cannot find a direct method in a
     given program space, then we consider inherited methods; this is
     not ideal (ideal would be to respect C++ hiding rules), but it
     seems good enough and is what GDB has historically done.  We only
     need to collect the names because later we find all symbols with
     those names.  This loop is written in a somewhat funny way
     because we collect data across the program space before deciding
     what to do.  */
  last_result_len = 0;
  for (const auto &elt : *sym_classes)
    {
      struct type *t;
      struct program_space *pspace;
      struct symbol *sym = elt.symbol;
      unsigned int ix = &elt - &*sym_classes->begin ();

      /* Program spaces that are executing startup should have
	 been filtered out earlier.  */
      pspace = sym->symtab ()->compunit ()->objfile ()->pspace;
      gdb_assert (!pspace->executing_startup);
      set_current_program_space (pspace);
      t = check_typedef (sym->type ());
      find_methods (t, sym->language (),
		    method_name, &result_names, &superclass_vec);

      /* Handle all items from a single program space at once; and be
	 sure not to miss the last batch.  */
      if (ix == sym_classes->size () - 1
	  || (pspace
	      != (sym_classes->at (ix + 1).symbol->symtab ()
		  ->compunit ()->objfile ()->pspace)))
	{
	  /* If we did not find a direct implementation anywhere in
	     this program space, consider superclasses.  */
	  if (result_names.size () == last_result_len)
	    find_superclass_methods (std::move (superclass_vec), method_name,
				     sym->language (), &result_names);

	  /* We have a list of candidate symbol names, so now we
	     iterate over the symbol tables looking for all
	     matches in this pspace.  */
	  add_all_symbol_names_from_pspace (&info, pspace, result_names,
					    FUNCTIONS_DOMAIN);

	  superclass_vec.clear ();
	  last_result_len = result_names.size ();
	}
    }

  if (!symbols->empty () || !minsyms->empty ())
    return;

  /* Throw an NOT_FOUND_ERROR.  This will be caught by the caller
     and other attempts to locate the symbol will be made.  */
  throw_error (NOT_FOUND_ERROR, _("see caller, this text doesn't matter"));
}



namespace {

/* This function object is a callback for iterate_over_symtabs, used
   when collecting all matching symtabs.  */

class symtab_collector
{
public:
  symtab_collector ()
    : m_symtab_table (htab_create (1, htab_hash_pointer, htab_eq_pointer,
				   NULL))
  {
  }

  /* Callable as a symbol_found_callback_ftype callback.  */
  bool operator () (symtab *sym);

  /* Return an rvalue reference to the collected symtabs.  */
  std::vector<symtab *> &&release_symtabs ()
  {
    return std::move (m_symtabs);
  }

private:
  /* The result vector of symtabs.  */
  std::vector<symtab *> m_symtabs;

  /* This is used to ensure the symtabs are unique.  */
  htab_up m_symtab_table;
};

bool
symtab_collector::operator () (struct symtab *symtab)
{
  void **slot;

  slot = htab_find_slot (m_symtab_table.get (), symtab, INSERT);
  if (!*slot)
    {
      *slot = symtab;
      m_symtabs.push_back (symtab);
    }

  return false;
}

} // namespace

/* Given a file name, return a list of all matching symtabs.  If
   SEARCH_PSPACE is not NULL, the search is restricted to just that
   program space.  */

static std::vector<symtab *>
collect_symtabs_from_filename (const char *file,
			       struct program_space *search_pspace)
{
  symtab_collector collector;

  /* Find that file's data.  */
  if (search_pspace == NULL)
    {
      for (struct program_space *pspace : program_spaces)
	{
	  if (pspace->executing_startup)
	    continue;

	  set_current_program_space (pspace);
	  iterate_over_symtabs (file, collector);
	}
    }
  else
    {
      set_current_program_space (search_pspace);
      iterate_over_symtabs (file, collector);
    }

  return collector.release_symtabs ();
}

/* Return all the symtabs associated to the FILENAME.  If SEARCH_PSPACE is
   not NULL, the search is restricted to just that program space.  */

static std::vector<symtab *>
symtabs_from_filename (const char *filename,
		       struct program_space *search_pspace)
{
  std::vector<symtab *> result
    = collect_symtabs_from_filename (filename, search_pspace);

  if (result.empty ())
    {
      if (!have_full_symbols () && !have_partial_symbols ())
	throw_error (NOT_FOUND_ERROR,
		     _("No symbol table is loaded.  "
		       "Use the \"file\" command."));
      source_file_not_found_error (filename);
    }

  return result;
}

/* See symtab.h.  */

void
symbol_searcher::find_all_symbols (const std::string &name,
				   const struct language_defn *language,
				   enum search_domain search_domain,
				   std::vector<symtab *> *search_symtabs,
				   struct program_space *search_pspace)
{
  symbol_searcher_collect_info info;
  struct linespec_state state;

  memset (&state, 0, sizeof (state));
  state.language = language;
  info.state = &state;

  info.result.symbols = &m_symbols;
  info.result.minimal_symbols = &m_minimal_symbols;
  std::vector<symtab *> all_symtabs;
  if (search_symtabs == nullptr)
    {
      all_symtabs.push_back (nullptr);
      search_symtabs = &all_symtabs;
    }
  info.file_symtabs = search_symtabs;

  add_matching_symbols_to_info (name.c_str (), symbol_name_match_type::WILD,
				search_domain, &info, search_pspace);
}

/* Look up a function symbol named NAME in symtabs FILE_SYMTABS.  Matching
   debug symbols are returned in SYMBOLS.  Matching minimal symbols are
   returned in MINSYMS.  */

static void
find_function_symbols (struct linespec_state *state,
		       const std::vector<symtab *> &file_symtabs, const char *name,
		       symbol_name_match_type name_match_type,
		       std::vector<block_symbol> *symbols,
		       std::vector<bound_minimal_symbol> *minsyms)
{
  struct collect_info info;
  std::vector<const char *> symbol_names;

  info.state = state;
  info.result.symbols = symbols;
  info.result.minimal_symbols = minsyms;
  info.file_symtabs = &file_symtabs;

  /* Try NAME as an Objective-C selector.  */
  find_imps (name, &symbol_names);
  if (!symbol_names.empty ())
    add_all_symbol_names_from_pspace (&info, state->search_pspace,
				      symbol_names, FUNCTIONS_DOMAIN);
  else
    add_matching_symbols_to_info (name, name_match_type, FUNCTIONS_DOMAIN,
				  &info, state->search_pspace);
}

/* Find all symbols named NAME in FILE_SYMTABS, returning debug symbols
   in SYMBOLS and minimal symbols in MINSYMS.  */

static void
find_linespec_symbols (struct linespec_state *state,
		       const std::vector<symtab *> &file_symtabs,
		       const char *lookup_name,
		       symbol_name_match_type name_match_type,
		       std::vector <block_symbol> *symbols,
		       std::vector<bound_minimal_symbol> *minsyms)
{
  gdb::unique_xmalloc_ptr<char> canon
    = cp_canonicalize_string_no_typedefs (lookup_name);
  if (canon != nullptr)
    lookup_name = canon.get ();

  /* It's important to not call expand_symtabs_matching unnecessarily
     as it can really slow things down (by unnecessarily expanding
     potentially 1000s of symtabs, which when debugging some apps can
     cost 100s of seconds).  Avoid this to some extent by *first* calling
     find_function_symbols, and only if that doesn't find anything
     *then* call find_method.  This handles two important cases:
     1) break (anonymous namespace)::foo
     2) break class::method where method is in class (and not a baseclass)  */

  find_function_symbols (state, file_symtabs, lookup_name,
			 name_match_type, symbols, minsyms);

  /* If we were unable to locate a symbol of the same name, try dividing
     the name into class and method names and searching the class and its
     baseclasses.  */
  if (symbols->empty () && minsyms->empty ())
    {
      std::string klass, method;
      const char *last, *p, *scope_op;

      /* See if we can find a scope operator and break this symbol
	 name into namespaces${SCOPE_OPERATOR}class_name and method_name.  */
      scope_op = "::";
      p = find_toplevel_string (lookup_name, scope_op);

      last = NULL;
      while (p != NULL)
	{
	  last = p;
	  p = find_toplevel_string (p + strlen (scope_op), scope_op);
	}

      /* If no scope operator was found, there is nothing more we can do;
	 we already attempted to lookup the entire name as a symbol
	 and failed.  */
      if (last == NULL)
	return;

      /* LOOKUP_NAME points to the class name.
	 LAST points to the method name.  */
      klass = std::string (lookup_name, last - lookup_name);

      /* Skip past the scope operator.  */
      last += strlen (scope_op);
      method = last;

      /* Find a list of classes named KLASS.  */
      std::vector<block_symbol> classes
	= lookup_prefix_sym (state, file_symtabs, klass.c_str ());
      if (!classes.empty ())
	{
	  /* Now locate a list of suitable methods named METHOD.  */
	  try
	    {
	      find_method (state, file_symtabs,
			   klass.c_str (), method.c_str (),
			   &classes, symbols, minsyms);
	    }

	  /* If successful, we're done.  If NOT_FOUND_ERROR
	     was not thrown, rethrow the exception that we did get.  */
	  catch (const gdb_exception_error &except)
	    {
	      if (except.error != NOT_FOUND_ERROR)
		throw;
	    }
	}
    }
}

/* Helper for find_label_symbols.  Find all labels that match name
   NAME in BLOCK.  Return all labels that match in FUNCTION_SYMBOLS.
   Return the actual function symbol in which the label was found in
   LABEL_FUNC_RET.  If COMPLETION_MODE is true, then NAME is
   interpreted as a label name prefix.  Otherwise, only a label named
   exactly NAME match.  */

static void
find_label_symbols_in_block (const struct block *block,
			     const char *name, struct symbol *fn_sym,
			     bool completion_mode,
			     std::vector<block_symbol> *result,
			     std::vector<block_symbol> *label_funcs_ret)
{
  if (completion_mode)
    {
      size_t name_len = strlen (name);

      int (*cmp) (const char *, const char *, size_t);
      cmp = case_sensitivity == case_sensitive_on ? strncmp : strncasecmp;

      for (struct symbol *sym : block_iterator_range (block))
	{
	  if (sym->matches (LABEL_DOMAIN)
	      && cmp (sym->search_name (), name, name_len) == 0)
	    {
	      result->push_back ({sym, block});
	      label_funcs_ret->push_back ({fn_sym, block});
	    }
	}
    }
  else
    {
      struct block_symbol label_sym
	= lookup_symbol (name, block, LABEL_DOMAIN, 0);

      if (label_sym.symbol != NULL)
	{
	  result->push_back (label_sym);
	  label_funcs_ret->push_back ({fn_sym, block});
	}
    }
}

/* Return all labels that match name NAME in FUNCTION_SYMBOLS.

   Return the actual function symbol in which the label was found in
   LABEL_FUNC_RET.  If COMPLETION_MODE is true, then NAME is
   interpreted as a label name prefix.  Otherwise, only labels named
   exactly NAME match.  */


static std::vector<block_symbol>
find_label_symbols (struct linespec_state *self,
		    const std::vector<block_symbol> &function_symbols,
		    std::vector<block_symbol> *label_funcs_ret,
		    const char *name,
		    bool completion_mode)
{
  const struct block *block;
  struct symbol *fn_sym;
  std::vector<block_symbol> result;

  if (function_symbols.empty ())
    {
      set_current_program_space (self->program_space);
      block = get_current_search_block ();

      for (;
	   block && !block->function ();
	   block = block->superblock ())
	;

      if (!block)
	return {};

      fn_sym = block->function ();

      find_label_symbols_in_block (block, name, fn_sym, completion_mode,
				   &result, label_funcs_ret);
    }
  else
    {
      for (const auto &elt : function_symbols)
	{
	  fn_sym = elt.symbol;
	  set_current_program_space
	    (fn_sym->symtab ()->compunit ()->objfile ()->pspace);
	  block = fn_sym->value_block ();

	  find_label_symbols_in_block (block, name, fn_sym, completion_mode,
				       &result, label_funcs_ret);
	}
    }

  return result;
}



/* A helper for create_sals_line_offset that handles the 'list_mode' case.  */

static std::vector<symtab_and_line>
decode_digits_list_mode (struct linespec_state *self,
			 linespec *ls,
			 struct symtab_and_line val)
{
  gdb_assert (self->list_mode);

  std::vector<symtab_and_line> values;

  for (const auto &elt : ls->file_symtabs)
    {
      /* The logic above should ensure this.  */
      gdb_assert (elt != NULL);

      program_space *pspace = elt->compunit ()->objfile ()->pspace;
      set_current_program_space (pspace);

      /* Simplistic search just for the list command.  */
      val.symtab = find_line_symtab (elt, val.line, NULL, NULL);
      if (val.symtab == NULL)
	val.symtab = elt;
      val.pspace = pspace;
      val.pc = 0;
      val.explicit_line = true;

      add_sal_to_sals (self, &values, &val, NULL, 0);
    }

  return values;
}

/* A helper for create_sals_line_offset that iterates over the symtabs
   associated with LS and returns a vector of corresponding symtab_and_line
   structures.  */

static std::vector<symtab_and_line>
decode_digits_ordinary (struct linespec_state *self,
			linespec *ls,
			int line,
			const linetable_entry **best_entry)
{
  std::vector<symtab_and_line> sals;
  for (const auto &elt : ls->file_symtabs)
    {
      std::vector<CORE_ADDR> pcs;

      /* The logic above should ensure this.  */
      gdb_assert (elt != NULL);

      program_space *pspace = elt->compunit ()->objfile ()->pspace;
      set_current_program_space (pspace);

      pcs = find_pcs_for_symtab_line (elt, line, best_entry);
      for (CORE_ADDR pc : pcs)
	{
	  symtab_and_line sal;
	  sal.pspace = pspace;
	  sal.symtab = elt;
	  sal.line = line;
	  sal.explicit_line = true;
	  sal.pc = pc;
	  sals.push_back (std::move (sal));
	}
    }

  return sals;
}



/* Return the line offset represented by VARIABLE.  */

static struct line_offset
linespec_parse_variable (struct linespec_state *self, const char *variable)
{
  int index = 0;
  const char *p;
  line_offset offset;

  p = (variable[1] == '$') ? variable + 2 : variable + 1;
  if (*p == '$')
    ++p;
  while (*p >= '0' && *p <= '9')
    ++p;
  if (!*p)		/* Reached end of token without hitting non-digit.  */
    {
      /* We have a value history reference.  */
      struct value *val_history;

      sscanf ((variable[1] == '$') ? variable + 2 : variable + 1, "%d", &index);
      val_history
	= access_value_history ((variable[1] == '$') ? -index : index);
      if (val_history->type ()->code () != TYPE_CODE_INT)
	error (_("History values used in line "
		 "specs must have integer values."));
      offset.offset = value_as_long (val_history);
      offset.sign = LINE_OFFSET_NONE;
    }
  else
    {
      /* Not all digits -- may be user variable/function or a
	 convenience variable.  */
      LONGEST valx;
      struct internalvar *ivar;

      /* Try it as a convenience variable.  If it is not a convenience
	 variable, return and allow normal symbol lookup to occur.  */
      ivar = lookup_only_internalvar (variable + 1);
	/* If there's no internal variable with that name, let the
	   offset remain as unknown to allow the name to be looked up
	   as a symbol.  */
      if (ivar != nullptr)
	{
	  /* We found a valid variable name.  If it is not an integer,
	     throw an error.  */
	  if (!get_internalvar_integer (ivar, &valx))
	    error (_("Convenience variables used in line "
		     "specs must have integer values."));
	  else
	    {
	      offset.offset = valx;
	      offset.sign = LINE_OFFSET_NONE;
	    }
	}
    }

  return offset;
}


/* We've found a minimal symbol MSYMBOL in OBJFILE to associate with our
   linespec; return the SAL in RESULT.  This function should return SALs
   matching those from find_function_start_sal, otherwise false
   multiple-locations breakpoints could be placed.  */

static void
minsym_found (struct linespec_state *self, struct objfile *objfile,
	      struct minimal_symbol *msymbol,
	      std::vector<symtab_and_line> *result)
{
  bool want_start_sal = false;

  CORE_ADDR func_addr;
  bool is_function = msymbol_is_function (objfile, msymbol, &func_addr);

  if (is_function)
    {
      const char *msym_name = msymbol->linkage_name ();

      if (msymbol->type () == mst_text_gnu_ifunc
	  || msymbol->type () == mst_data_gnu_ifunc)
	want_start_sal = gnu_ifunc_resolve_name (msym_name, &func_addr);
      else
	want_start_sal = true;
    }

  symtab_and_line sal;

  if (is_function && want_start_sal)
    sal = find_function_start_sal (func_addr, NULL, self->funfirstline);
  else
    {
      sal.objfile = objfile;
      sal.msymbol = msymbol;
      /* Store func_addr, not the minsym's address in case this was an
	 ifunc that hasn't been resolved yet.  */
      if (is_function)
	sal.pc = func_addr;
      else
	sal.pc = msymbol->value_address (objfile);
      sal.pspace = current_program_space;
    }

  sal.section = msymbol->obj_section (objfile);

  if (maybe_add_address (self->addr_set, objfile->pspace, sal.pc))
    add_sal_to_sals (self, result, &sal, msymbol->natural_name (), 0);
}

/* Helper for search_minsyms_for_name that adds the symbol to the
   result.  */

static void
add_minsym (struct minimal_symbol *minsym, struct objfile *objfile,
	    struct symtab *symtab, int list_mode,
	    std::vector<struct bound_minimal_symbol> *msyms)
{
  if (symtab != NULL)
    {
      /* We're looking for a label for which we don't have debug
	 info.  */
      CORE_ADDR func_addr;
      if (msymbol_is_function (objfile, minsym, &func_addr))
	{
	  symtab_and_line sal = find_pc_sect_line (func_addr, NULL, 0);

	  if (symtab != sal.symtab)
	    return;
	}
    }

  /* Exclude data symbols when looking for breakpoint locations.  */
  if (!list_mode && !msymbol_is_function (objfile, minsym))
    return;

  msyms->emplace_back (minsym, objfile);
  return;
}

/* Search for minimal symbols called NAME.  If SEARCH_PSPACE
   is not NULL, the search is restricted to just that program
   space.

   If SYMTAB is NULL, search all objfiles, otherwise
   restrict results to the given SYMTAB.  */

static void
search_minsyms_for_name (struct collect_info *info,
			 const lookup_name_info &name,
			 struct program_space *search_pspace,
			 struct symtab *symtab)
{
  std::vector<struct bound_minimal_symbol> minsyms;

  if (symtab == NULL)
    {
      for (struct program_space *pspace : program_spaces)
	{
	  if (search_pspace != NULL && search_pspace != pspace)
	    continue;
	  if (pspace->executing_startup)
	    continue;

	  set_current_program_space (pspace);

	  for (objfile *objfile : current_program_space->objfiles ())
	    {
	      iterate_over_minimal_symbols (objfile, name,
					    [&] (struct minimal_symbol *msym)
					    {
					      add_minsym (msym, objfile, nullptr,
							  info->state->list_mode,
							  &minsyms);
					      return false;
					    });
	    }
	}
    }
  else
    {
      program_space *pspace = symtab->compunit ()->objfile ()->pspace;

      if (search_pspace == NULL || pspace == search_pspace)
	{
	  set_current_program_space (pspace);
	  iterate_over_minimal_symbols
	    (symtab->compunit ()->objfile (), name,
	     [&] (struct minimal_symbol *msym)
	       {
		 add_minsym (msym, symtab->compunit ()->objfile (), symtab,
			     info->state->list_mode, &minsyms);
		 return false;
	       });
	}
    }

  /* Return true if TYPE is a static symbol.  */
  auto msymbol_type_is_static = [] (enum minimal_symbol_type type)
    {
      switch (type)
	{
	case mst_file_text:
	case mst_file_data:
	case mst_file_bss:
	return true;
	default:
	return false;
	}
    };

  /* Add minsyms to the result set, but filter out trampoline symbols
     if we also found extern symbols with the same name.  I.e., don't
     set a breakpoint on both '<foo@plt>' and 'foo', assuming that
     'foo' is the symbol that the plt resolves to.  */
  for (const bound_minimal_symbol &item : minsyms)
    {
      bool skip = false;
      if (item.minsym->type () == mst_solib_trampoline)
	{
	  for (const bound_minimal_symbol &item2 : minsyms)
	    {
	      if (&item2 == &item)
		continue;

	      /* Ignore other trampoline symbols.  */
	      if (item2.minsym->type () == mst_solib_trampoline)
		continue;

	      /* Trampoline symbols can only jump to exported
		 symbols.  */
	      if (msymbol_type_is_static (item2.minsym->type ()))
		continue;

	      if (strcmp (item.minsym->linkage_name (),
			  item2.minsym->linkage_name ()) != 0)
		continue;

	      /* Found a global minsym with the same name as the
		 trampoline.  Don't create a location for this
		 trampoline.  */
	      skip = true;
	      break;
	    }
	}

      if (!skip)
	info->result.minimal_symbols->push_back (item);
    }
}

/* A helper function to add all symbols matching NAME to INFO.  If
   PSPACE is not NULL, the search is restricted to just that program
   space.  */

static void
add_matching_symbols_to_info (const char *name,
			      symbol_name_match_type name_match_type,
			      enum search_domain search_domain,
			      struct collect_info *info,
			      struct program_space *pspace)
{
  lookup_name_info lookup_name (name, name_match_type);

  for (const auto &elt : *info->file_symtabs)
    {
      if (elt == nullptr)
	{
	  iterate_over_all_matching_symtabs (info->state, lookup_name,
					     VAR_DOMAIN, search_domain,
					     pspace, true,
					     [&] (block_symbol *bsym)
	    { return info->add_symbol (bsym); });
	  search_minsyms_for_name (info, lookup_name, pspace, NULL);
	}
      else if (pspace == NULL || pspace == elt->compunit ()->objfile ()->pspace)
	{
	  int prev_len = info->result.symbols->size ();

	  /* Program spaces that are executing startup should have
	     been filtered out earlier.  */
	  program_space *elt_pspace = elt->compunit ()->objfile ()->pspace;
	  gdb_assert (!elt_pspace->executing_startup);
	  set_current_program_space (elt_pspace);
	  iterate_over_file_blocks (elt, lookup_name, VAR_DOMAIN,
				    [&] (block_symbol *bsym)
	    { return info->add_symbol (bsym); });

	  /* If no new symbols were found in this iteration and this symtab
	     is in assembler, we might actually be looking for a label for
	     which we don't have debug info.  Check for a minimal symbol in
	     this case.  */
	  if (prev_len == info->result.symbols->size ()
	      && elt->language () == language_asm)
	    search_minsyms_for_name (info, lookup_name, pspace, elt);
	}
    }
}



/* Now come some functions that are called from multiple places within
   decode_line_1.  */

static int
symbol_to_sal (struct symtab_and_line *result,
	       int funfirstline, struct symbol *sym)
{
  if (sym->aclass () == LOC_BLOCK)
    {
      *result = find_function_start_sal (sym, funfirstline);
      return 1;
    }
  else
    {
      if (sym->aclass () == LOC_LABEL && sym->value_address () != 0)
	{
	  *result = {};
	  result->symtab = sym->symtab ();
	  result->symbol = sym;
	  result->line = sym->line ();
	  result->pc = sym->value_address ();
	  result->pspace = result->symtab->compunit ()->objfile ()->pspace;
	  result->explicit_pc = 1;
	  return 1;
	}
      else if (funfirstline)
	{
	  /* Nothing.  */
	}
      else if (sym->line () != 0)
	{
	  /* We know its line number.  */
	  *result = {};
	  result->symtab = sym->symtab ();
	  result->symbol = sym;
	  result->line = sym->line ();
	  result->pc = sym->value_address ();
	  result->pspace = result->symtab->compunit ()->objfile ()->pspace;
	  return 1;
	}
    }

  return 0;
}

linespec_result::~linespec_result ()
{
  for (linespec_sals &lsal : lsals)
    xfree (lsal.canonical);
}

/* Return the quote characters permitted by the linespec parser.  */

const char *
get_gdb_linespec_parser_quote_characters (void)
{
  return linespec_quote_characters;
}
