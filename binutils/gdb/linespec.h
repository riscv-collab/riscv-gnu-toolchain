/* Header for GDB line completion.
   Copyright (C) 2000-2024 Free Software Foundation, Inc.

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

#if !defined (LINESPEC_H)
#define LINESPEC_H 1

struct symtab;

#include "location.h"

/* Flags to pass to decode_line_1 and decode_line_full.  */

enum decode_line_flags
  {
    /* Set this flag if you want the resulting SALs to describe the
       first line of indicated functions.  */
    DECODE_LINE_FUNFIRSTLINE = 1,

    /* Set this flag if you want "list mode".  In this mode, a
       FILE:LINE linespec will always return a result, and such
       linespecs will not be expanded to all matches.  */
    DECODE_LINE_LIST_MODE = 2
  };

/* decode_line_full returns a vector of these.  */

struct linespec_sals
{
  /* This is the location corresponding to the sals contained in this
     object.  It can be passed as the FILTER argument to future calls
     to decode_line_full.  This is freed by the linespec_result
     destructor.  */
  char *canonical;

  /* Sals.  */
  std::vector<symtab_and_line> sals;
};

/* An instance of this may be filled in by decode_line_1.  The caller
   must make copies of any data that it needs to keep.  */

struct linespec_result
{
  linespec_result () = default;
  ~linespec_result ();

  DISABLE_COPY_AND_ASSIGN (linespec_result);

  /* If true, the linespec should be displayed to the user.  This
     is used by "unusual" linespecs where the ordinary `info break'
     display mechanism would do the wrong thing.  */
  bool special_display = false;

  /* If true, the linespec result should be considered to be a
     "pre-expanded" multi-location linespec.  A pre-expanded linespec
     holds all matching locations in a single linespec_sals
     object.  */
  bool pre_expanded = false;

  /* If PRE_EXPANDED is true, this is set to the location spec
     entered by the user.  */
  location_spec_up locspec;

  /* The sals.  The vector will be freed by the destructor.  */
  std::vector<linespec_sals> lsals;
};

/* Decode a linespec using the provided default symtab and line.  */

extern std::vector<symtab_and_line>
	decode_line_1 (const location_spec *locspec, int flags,
		       struct program_space *search_pspace,
		       struct symtab *default_symtab, int default_line);

/* Parse LOCSPEC and return results.  This is the "full"
   interface to this module, which handles multiple results
   properly.

   For FLAGS, see decode_line_flags.  DECODE_LINE_LIST_MODE is not
   valid for this function.

   If SEARCH_PSPACE is not NULL, symbol search is restricted to just
   that program space.

   DEFAULT_SYMTAB and DEFAULT_LINE describe the default location.
   DEFAULT_SYMTAB can be NULL, in which case the current symtab and
   line are used.

   CANONICAL is where the results are stored.  It must not be NULL.

   SELECT_MODE must be one of the multiple_symbols_* constants, or
   NULL.  It determines how multiple results will be handled.  If
   NULL, the appropriate CLI value will be used.

   FILTER can either be NULL or a string holding a canonical name.
   This is only valid when SELECT_MODE is multiple_symbols_all.

   Multiple results are handled differently depending on the
   arguments:

   . With multiple_symbols_cancel, an exception is thrown.

   . With multiple_symbols_ask, a menu is presented to the user.  The
   user may select none, in which case an exception is thrown; or all,
   which is handled like multiple_symbols_all, below.  Otherwise,
   CANONICAL->SALS will have one entry for each name the user chose.

   . With multiple_symbols_all, CANONICAL->SALS will have a single
   entry describing all the matching locations.  If FILTER is
   non-NULL, then only locations whose canonical name is equal (in the
   strcmp sense) to FILTER will be returned; all others will be
   filtered out.  */

extern void decode_line_full (struct location_spec *locspec, int flags,
			      struct program_space *search_pspace,
			      struct symtab *default_symtab, int default_line,
			      struct linespec_result *canonical,
			      const char *select_mode,
			      const char *filter);

/* Given a string, return the line specified by it, using the current
   source symtab and line as defaults.
   This is for commands like "list" and "breakpoint".  */

extern std::vector<symtab_and_line> decode_line_with_current_source
    (const char *, int);

/* Given a string, return the line specified by it, using the last displayed
   codepoint's values as defaults, or nothing if they aren't valid.  */

extern std::vector<symtab_and_line> decode_line_with_last_displayed
    (const char *, int);

/* Does P represent one of the keywords?  If so, return
   the keyword.  If not, return NULL.  */

extern const char *linespec_lexer_lex_keyword (const char *p);

/* Parse a line offset from STRING.  */

extern struct line_offset linespec_parse_line_offset (const char *string);

/* Return the quote characters permitted by the linespec parser.  */

extern const char *get_gdb_linespec_parser_quote_characters (void);

/* Does STRING represent an Ada operator?  If so, return the length
   of the decoded operator name.  If not, return 0.  */

extern int is_ada_operator (const char *string);

/* Find the end of the (first) linespec pointed to by *STRINGP.
   STRINGP will be advanced to this point.  */

extern void linespec_lex_to_end (const char **stringp);

extern const char * const linespec_keywords[];

/* Complete a linespec.  */

extern void linespec_complete (completion_tracker &tracker,
			       const char *text,
			       symbol_name_match_type match_type);

/* Complete a function symbol, in linespec mode, according to
   FUNC_MATCH_TYPE.  If SOURCE_FILENAME is non-NULL, limits completion
   to the list of functions defined in source files that match
   SOURCE_FILENAME.  */

extern void linespec_complete_function (completion_tracker &tracker,
					const char *function,
					symbol_name_match_type func_match_type,
					const char *source_filename);

/* Complete a label symbol, in linespec mode.  Only labels of
   functions named FUNCTION_NAME are considered.  If SOURCE_FILENAME
   is non-NULL, limits completion to labels of functions defined in
   source files that match SOURCE_FILENAME.  */

extern void linespec_complete_label (completion_tracker &tracker,
				     const struct language_defn *language,
				     const char *source_filename,
				     const char *function_name,
				     symbol_name_match_type name_match_type,
				     const char *label_name);

/* Evaluate the expression pointed to by EXP_PTR into a CORE_ADDR,
   advancing EXP_PTR past any parsed text.  */

extern CORE_ADDR linespec_expression_to_pc (const char **exp_ptr);
#endif /* defined (LINESPEC_H) */
