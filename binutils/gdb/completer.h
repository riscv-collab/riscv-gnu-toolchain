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

#if !defined (COMPLETER_H)
#define COMPLETER_H 1

#include "gdbsupport/gdb-hashtab.h"
#include "gdbsupport/gdb_vecs.h"
#include "command.h"

/* Types of functions in struct match_list_displayer.  */

struct match_list_displayer;

typedef void mld_crlf_ftype (const struct match_list_displayer *);
typedef void mld_putch_ftype (const struct match_list_displayer *, int);
typedef void mld_puts_ftype (const struct match_list_displayer *,
			     const char *);
typedef void mld_flush_ftype (const struct match_list_displayer *);
typedef void mld_erase_entire_line_ftype (const struct match_list_displayer *);
typedef void mld_beep_ftype (const struct match_list_displayer *);
typedef int mld_read_key_ftype (const struct match_list_displayer *);

/* Interface between CLI/TUI and gdb_match_list_displayer.  */

struct match_list_displayer
{
  /* The screen dimensions to work with when displaying matches.  */
  int height, width;

  /* Print cr,lf.  */
  mld_crlf_ftype *crlf;

  /* Not "putc" to avoid issues where it is a stdio macro.  Sigh.  */
  mld_putch_ftype *putch;

  /* Print a string.  */
  mld_puts_ftype *puts;

  /* Flush all accumulated output.  */
  mld_flush_ftype *flush;

  /* Erase the currently line on the terminal (but don't discard any text the
     user has entered, readline may shortly re-print it).  */
  mld_erase_entire_line_ftype *erase_entire_line;

  /* Ring the bell.  */
  mld_beep_ftype *beep;

  /* Read one key.  */
  mld_read_key_ftype *read_key;
};

/* A list of completion candidates.  Each element is a malloc string,
   because ownership of the strings is transferred to readline, which
   calls free on each element.  */
typedef std::vector<gdb::unique_xmalloc_ptr<char>> completion_list;

/* The result of a successful completion match.  When doing symbol
   comparison, we use the symbol search name for the symbol name match
   check, but the matched name that is shown to the user may be
   different.  For example, Ada uses encoded names for lookup, but
   then wants to decode the symbol name to show to the user, and also
   in some cases wrap the matched name in "<sym>" (meaning we can't
   always use the symbol's print name).  */

class completion_match
{
public:
  /* Get the completion match result.  See m_match/m_storage's
     descriptions.  */
  const char *match ()
  { return m_match; }

  /* Set the completion match result.  See m_match/m_storage's
     descriptions.  */
  void set_match (const char *match)
  { m_match = match; }

  /* Get temporary storage for generating a match result, dynamically.
     The built string is only good until the next clear() call.  I.e.,
     good until the next symbol comparison.  */
  std::string &storage ()
  { return m_storage; }

  /* Prepare for another completion matching sequence.  */
  void clear ()
  {
    m_match = NULL;
    m_storage.clear ();
  }

private:
  /* The completion match result.  This can either be a pointer into
     M_STORAGE string, or it can be a pointer into the some other
     string that outlives the completion matching sequence (usually, a
     pointer to a symbol's name).  */
  const char *m_match;

  /* Storage a symbol comparison routine can use for generating a
     match result, dynamically.  The built string is only good until
     the next clear() call.  I.e., good until the next symbol
     comparison.  */
  std::string m_storage;
};

/* The result of a successful completion match, but for least common
   denominator (LCD) computation.  Some completers provide matches
   that don't start with the completion "word".  E.g., completing on
   "b push_ba" on a C++ program usually completes to
   std::vector<...>::push_back, std::string::push_back etc.  In such
   case, the symbol comparison routine will set the LCD match to point
   into the "push_back" substring within the symbol's name string.
   Also, in some cases, the symbol comparison routine will want to
   ignore parts of the symbol name for LCD purposes, such as for
   example symbols with abi tags in C++.  In such cases, the symbol
   comparison routine will set MARK_IGNORED_RANGE to mark the ignored
   substrings of the matched string.  The resulting LCD string with
   the ignored parts stripped out is computed at the end of a
   completion match sequence iff we had a positive match.  */

class completion_match_for_lcd
{
public:
  /* Get the resulting LCD, after a successful match.  */
  const char *match ()
  { return m_match; }

  /* Set the match for LCD.  See m_match's description.  */
  void set_match (const char *match)
  { m_match = match; }

  /* Mark the range between [BEGIN, END) as ignored.  */
  void mark_ignored_range (const char *begin, const char *end)
  {
    gdb_assert (begin < end);
    gdb_assert (m_ignored_ranges.empty ()
		|| m_ignored_ranges.back ().second < begin);
    m_ignored_ranges.emplace_back (begin, end);
  }

  /* Get the resulting LCD, after a successful match.  If there are
     ignored ranges, then this builds a new string with the ignored
     parts removed (and stores it internally).  As such, the result of
     this call is only good for the current completion match
     sequence.  */
  const char *finish ()
  {
    if (m_ignored_ranges.empty ())
      return m_match;
    else
      {
	m_finished_storage.clear ();

	gdb_assert (m_ignored_ranges.back ().second
		    <= (m_match + strlen (m_match)));

	const char *prev = m_match;
	for (const auto &range : m_ignored_ranges)
	  {
	    gdb_assert (prev < range.first);
	    gdb_assert (range.second > range.first);
	    m_finished_storage.append (prev, range.first);
	    prev = range.second;
	  }
	m_finished_storage.append (prev);

	return m_finished_storage.c_str ();
      }
  }

  /* Prepare for another completion matching sequence.  */
  void clear ()
  {
    m_match = NULL;
    m_ignored_ranges.clear ();
  }

  /* Return true if this object has had no match data set since its
     creation, or the last call to clear.  */
  bool empty () const
  {
    return m_match == nullptr && m_ignored_ranges.empty ();
  }

private:
  /* The completion match result for LCD.  This is usually either a
     pointer into to a substring within a symbol's name, or to the
     storage of the pairing completion_match object.  */
  const char *m_match;

  /* The ignored substring ranges within M_MATCH.  E.g., if we were
     looking for completion matches for C++ functions starting with
       "functio"
     and successfully match:
       "function[abi:cxx11](int)"
     the ignored ranges vector will contain an entry that delimits the
     "[abi:cxx11]" substring, such that calling finish() results in:
       "function(int)"
   */
  std::vector<std::pair<const char *, const char *>> m_ignored_ranges;

  /* Storage used by the finish() method, if it has to compute a new
     string.  */
  std::string m_finished_storage;
};

/* Convenience aggregate holding info returned by the symbol name
   matching routines (see symbol_name_matcher_ftype).  */
struct completion_match_result
{
  /* The completion match candidate.  */
  completion_match match;

  /* The completion match, for LCD computation purposes.  */
  completion_match_for_lcd match_for_lcd;

  /* Convenience that sets both MATCH and MATCH_FOR_LCD.  M_FOR_LCD is
     optional.  If not specified, defaults to M.  */
  void set_match (const char *m, const char *m_for_lcd = NULL)
  {
    match.set_match (m);
    if (m_for_lcd == NULL)
      match_for_lcd.set_match (m);
    else
      match_for_lcd.set_match (m_for_lcd);
  }
};

/* The final result of a completion that is handed over to either
   readline or the "completion" command (which pretends to be
   readline).  Mainly a wrapper for a readline-style match list array,
   though other bits of info are included too.  */

struct completion_result
{
  /* Create an empty result.  */
  completion_result ();

  /* Create a result.  */
  completion_result (char **match_list, size_t number_matches,
		     bool completion_suppress_append);

  /* Destroy a result.  */
  ~completion_result ();

  DISABLE_COPY_AND_ASSIGN (completion_result);

  /* Move a result.  */
  completion_result (completion_result &&rhs) noexcept;

  /* Release ownership of the match list array.  */
  char **release_match_list ();

  /* Sort the match list.  */
  void sort_match_list ();

private:
  /* Destroy the match list array and its contents.  */
  void reset_match_list ();

public:
  /* (There's no point in making these fields private, since the whole
     point of this wrapper is to build data in the layout expected by
     readline.  Making them private would require adding getters for
     the "complete" command, which would expose the same
     implementation details anyway.)  */

  /* The match list array, in the format that readline expects.
     match_list[0] contains the common prefix.  The real match list
     starts at index 1.  The list is NULL terminated.  If there's only
     one match, then match_list[1] is NULL.  If there are no matches,
     then this is NULL.  */
  char **match_list;
  /* The number of matched completions in MATCH_LIST.  Does not
     include the NULL terminator or the common prefix.  */
  size_t number_matches;

  /* Whether readline should suppress appending a whitespace, when
     there's only one possible completion.  */
  bool completion_suppress_append;
};

/* Object used by completers to build a completion match list to hand
   over to readline.  It tracks:

   - How many unique completions have been generated, to terminate
     completion list generation early if the list has grown to a size
     so large as to be useless.  This helps avoid GDB seeming to lock
     up in the event the user requests to complete on something vague
     that necessitates the time consuming expansion of many symbol
     tables.

   - The completer's idea of least common denominator (aka the common
     prefix) between all completion matches to hand over to readline.
     Some completers provide matches that don't start with the
     completion "word".  E.g., completing on "b push_ba" on a C++
     program usually completes to std::vector<...>::push_back,
     std::string::push_back etc.  If all matches happen to start with
     "std::", then readline would figure out that the lowest common
     denominator is "std::", and thus would do a partial completion
     with that.  I.e., it would replace "push_ba" in the input buffer
     with "std::", losing the original "push_ba", which is obviously
     undesirable.  To avoid that, such completers pass the substring
     of the match that matters for common denominator computation as
     MATCH_FOR_LCD argument to add_completion.  The end result is
     passed to readline in gdb_rl_attempted_completion_function.

   - The custom word point to hand over to readline, for completers
     that parse the input string in order to dynamically adjust
     themselves depending on exactly what they're completing.  E.g.,
     the linespec completer needs to bypass readline's too-simple word
     breaking algorithm.
*/
class completion_tracker
{
public:
  explicit completion_tracker (bool from_readline);
  ~completion_tracker ();

  DISABLE_COPY_AND_ASSIGN (completion_tracker);

  /* Add the completion NAME to the list of generated completions if
     it is not there already.  If too many completions were already
     found, this throws an error.  */
  void add_completion (gdb::unique_xmalloc_ptr<char> name,
		       completion_match_for_lcd *match_for_lcd = NULL,
		       const char *text = NULL, const char *word = NULL);

  /* Add all completions matches in LIST.  Elements are moved out of
     LIST.  */
  void add_completions (completion_list &&list);

  /* Remove completion matching NAME from the completion list, does nothing
     if NAME is not already in the completion list.  */
  void remove_completion (const char *name);

  /* Set the quote char to be appended after a unique completion is
     added to the input line.  Set to '\0' to clear.  See
     m_quote_char's description.  */
  void set_quote_char (int quote_char)
  { m_quote_char = quote_char; }

  /* The quote char to be appended after a unique completion is added
     to the input line.  Returns '\0' if no quote char has been set.
     See m_quote_char's description.  */
  int quote_char () { return m_quote_char; }

  /* Tell the tracker that the current completer wants to provide a
     custom word point instead of a list of a break chars, in the
     handle_brkchars phase.  Such completers must also compute their
     completions then.  */
  void set_use_custom_word_point (bool enable)
  { m_use_custom_word_point = enable; }

  /* Whether the current completer computes a custom word point.  */
  bool use_custom_word_point () const
  { return m_use_custom_word_point; }

  /* The custom word point.  */
  int custom_word_point () const
  { return m_custom_word_point; }

  /* Set the custom word point to POINT.  */
  void set_custom_word_point (int point)
  { m_custom_word_point = point; }

  /* Advance the custom word point by LEN.  */
  void advance_custom_word_point_by (int len);

  /* Whether to tell readline to skip appending a whitespace after the
     completion.  See m_suppress_append_ws.  */
  bool suppress_append_ws () const
  { return m_suppress_append_ws; }

  /* Set whether to tell readline to skip appending a whitespace after
     the completion.  See m_suppress_append_ws.  */
  void set_suppress_append_ws (bool suppress)
  { m_suppress_append_ws = suppress; }

  /* Return true if we only have one completion, and it matches
     exactly the completion word.  I.e., completing results in what we
     already have.  */
  bool completes_to_completion_word (const char *word);

  /* Get a reference to the shared (between all the multiple symbol
     name comparison calls) completion_match_result object, ready for
     another symbol name match sequence.  */
  completion_match_result &reset_completion_match_result ()
  {
    completion_match_result &res = m_completion_match_result;

    /* Clear any previous match.  */
    res.match.clear ();
    res.match_for_lcd.clear ();
    return m_completion_match_result;
  }

  /* True if we have any completion match recorded.  */
  bool have_completions () const
  { return htab_elements (m_entries_hash.get ()) > 0; }

  /* Discard the current completion match list and the current
     LCD.  */
  void discard_completions ();

  /* Build a completion_result containing the list of completion
     matches to hand over to readline.  The parameters are as in
     rl_attempted_completion_function.  */
  completion_result build_completion_result (const char *text,
					     int start, int end);

  /* Tells if the completion task is triggered by readline.  See
     m_from_readline.  */
  bool from_readline () const
  { return m_from_readline; }

private:

  /* The type that we place into the m_entries_hash hash table.  */
  class completion_hash_entry;

  /* Add the completion NAME to the list of generated completions if
     it is not there already.  If false is returned, too many
     completions were found.  */
  bool maybe_add_completion (gdb::unique_xmalloc_ptr<char> name,
			     completion_match_for_lcd *match_for_lcd,
			     const char *text, const char *word);

  /* Ensure that the lowest common denominator held in the member variable
     M_LOWEST_COMMON_DENOMINATOR is valid.  This method must be called if
     there is any chance that new completions have been added to the
     tracker before the lowest common denominator is read.  */
  void recompute_lowest_common_denominator ();

  /* Callback used from recompute_lowest_common_denominator, called for
     every entry in m_entries_hash.  */
  void recompute_lcd_visitor (completion_hash_entry *entry);

  /* Completion match outputs returned by the symbol name matching
     routines (see symbol_name_matcher_ftype).  These results are only
     valid for a single match call.  This is here in order to be able
     to conveniently share the same storage among all the calls to the
     symbol name matching routines.  */
  completion_match_result m_completion_match_result;

  /* The completion matches found so far, in a hash table, for
     duplicate elimination as entries are added.  Otherwise the user
     is left scratching his/her head: readline and complete_command
     will remove duplicates, and if removal of duplicates there brings
     the total under max_completions the user may think gdb quit
     searching too early.  */
  htab_up m_entries_hash;

  /* If non-zero, then this is the quote char that needs to be
     appended after completion (iff we have a unique completion).  We
     don't rely on readline appending the quote char as delimiter as
     then readline wouldn't append the ' ' after the completion.
     I.e., we want this:

      before tab: "b 'function("
      after tab:  "b 'function()' "
  */
  int m_quote_char = '\0';

  /* If true, the completer has its own idea of "word" point, and
     doesn't want to rely on readline computing it based on brkchars.
     Set in the handle_brkchars phase.  */
  bool m_use_custom_word_point = false;

  /* The completer's idea of where the "word" we were looking at is
     relative to RL_LINE_BUFFER.  This is advanced in the
     handle_brkchars phase as the completer discovers potential
     completable words.  */
  int m_custom_word_point = 0;

  /* If true, tell readline to skip appending a whitespace after the
     completion.  Automatically set if we have a unique completion
     that already has a space at the end.  A completer may also
     explicitly set this.  E.g., the linespec completer sets this when
     the completion ends with the ":" separator between filename and
     function name.  */
  bool m_suppress_append_ws = false;

  /* Our idea of lowest common denominator to hand over to readline.
     See intro.  */
  char *m_lowest_common_denominator = NULL;

  /* If true, the LCD is unique.  I.e., all completions had the same
     MATCH_FOR_LCD substring, even if the completions were different.
     For example, if "break function<tab>" found "a::function()" and
     "b::function()", the LCD will be "function()" in both cases and
     so we want to tell readline to complete the line with
     "function()", instead of showing all the possible
     completions.  */
  bool m_lowest_common_denominator_unique = false;

  /* True if the value in M_LOWEST_COMMON_DENOMINATOR is correct.  This is
     set to true each time RECOMPUTE_LOWEST_COMMON_DENOMINATOR is called,
     and reset to false whenever a new completion is added.  */
  bool m_lowest_common_denominator_valid = false;

  /* To avoid calls to xrealloc in RECOMPUTE_LOWEST_COMMON_DENOMINATOR, we
     track the maximum possible size of the lowest common denominator,
     which we know as each completion is added.  */
  size_t m_lowest_common_denominator_max_length = 0;

  /* Indicates that the completions are to be displayed by readline
     interactively. The 'complete' command is a way to generate completions
     not to be displayed by readline.  */
  bool m_from_readline;
};

/* Return a string to hand off to readline as a completion match
   candidate, potentially composed of parts of MATCH_NAME and of
   TEXT/WORD.  For a description of TEXT/WORD see completer_ftype.  */

extern gdb::unique_xmalloc_ptr<char>
  make_completion_match_str (const char *match_name,
			     const char *text, const char *word);

/* Like above, but takes ownership of MATCH_NAME (i.e., can
   reuse/return it).  */

extern gdb::unique_xmalloc_ptr<char>
  make_completion_match_str (gdb::unique_xmalloc_ptr<char> &&match_name,
			     const char *text, const char *word);

extern void gdb_display_match_list (char **matches, int len, int max,
				    const struct match_list_displayer *);

extern const char *get_max_completions_reached_message (void);

extern void complete_line (completion_tracker &tracker,
			   const char *text,
			   const char *line_buffer,
			   int point);

/* Complete LINE and return completion results.  For completion purposes,
   cursor position is assumed to be at the end of LINE.  WORD is set to
   the end of word to complete.  QUOTE_CHAR is set to the opening quote
   character if we found an unclosed quoted substring, '\0' otherwise.  */
extern completion_result
  complete (const char *line, char const **word, int *quote_char);

/* Find the bounds of the word in TEXT for completion purposes, and
   return a pointer to the end of the word.  Calls the completion
   machinery for a handle_brkchars phase (using TRACKER) to figure out
   the right work break characters for the command in TEXT.
   QUOTE_CHAR, if non-null, is set to the opening quote character if
   we found an unclosed quoted substring, '\0' otherwise.  */
extern const char *completion_find_completion_word (completion_tracker &tracker,
						    const char *text,
						    int *quote_char);


/* Assuming TEXT is an expression in the current language, find the
   completion word point for TEXT, emulating the algorithm readline
   uses to find the word point, using the current language's word
   break characters.  */
const char *advance_to_expression_complete_word_point
  (completion_tracker &tracker, const char *text);

/* Assuming TEXT is an filename, find the completion word point for
   TEXT, emulating the algorithm readline uses to find the word
   point.  */
extern const char *advance_to_filename_complete_word_point
  (completion_tracker &tracker, const char *text);

extern char **gdb_rl_attempted_completion_function (const char *text,
						    int start, int end);

extern void noop_completer (struct cmd_list_element *,
			    completion_tracker &tracker,
			    const char *, const char *);

extern void filename_completer (struct cmd_list_element *,
				completion_tracker &tracker,
				const char *, const char *);

extern void expression_completer (struct cmd_list_element *,
				  completion_tracker &tracker,
				  const char *, const char *);

extern void location_completer (struct cmd_list_element *,
				completion_tracker &tracker,
				const char *, const char *);

extern void symbol_completer (struct cmd_list_element *,
			      completion_tracker &tracker,
			      const char *, const char *);

extern void command_completer (struct cmd_list_element *,
			       completion_tracker &tracker,
			       const char *, const char *);

extern void signal_completer (struct cmd_list_element *,
			      completion_tracker &tracker,
			      const char *, const char *);

extern void reg_or_group_completer (struct cmd_list_element *,
				    completion_tracker &tracker,
				    const char *, const char *);

extern void reggroup_completer (struct cmd_list_element *,
				completion_tracker &tracker,
				const char *, const char *);

extern const char *get_gdb_completer_quote_characters (void);

extern char *gdb_completion_word_break_characters (void);

/* Set the word break characters array to BREAK_CHARS.  This function
   is useful as const-correct alternative to direct assignment to
   rl_completer_word_break_characters, which is "char *",
   not "const char *".  */
extern void set_rl_completer_word_break_characters (const char *break_chars);

/* Get the matching completer_handle_brkchars_ftype function for FN.
   FN is one of the core completer functions above (filename,
   location, symbol, etc.).  This function is useful for cases when
   the completer doesn't know the type of the completion until some
   calculation is done (e.g., for Python functions).  */

extern completer_handle_brkchars_ftype *
  completer_handle_brkchars_func_for_completer (completer_ftype *fn);

/* Exported to linespec.c */

/* Return a list of all source files whose names begin with matching
   TEXT.  */
extern completion_list complete_source_filenames (const char *text);

/* Complete on expressions.  Often this means completing on symbol
   names, but some language parsers also have support for completing
   field names.  */
extern void complete_expression (completion_tracker &tracker,
				 const char *text, const char *word);

/* Called by custom word point completers that want to recurse into
   the completion machinery to complete a command.  Used to complete
   COMMAND in "thread apply all COMMAND", for example.  Note that
   unlike command_completer, this fully recurses into the proper
   completer for COMMAND, so that e.g.,

     (gdb) thread apply all print -[TAB]

   does the right thing and show the print options.  */
extern void complete_nested_command_line (completion_tracker &tracker,
					  const char *text);

extern const char *skip_quoted_chars (const char *, const char *,
				      const char *);

extern const char *skip_quoted (const char *);

/* Called from command completion function to skip over /FMT
   specifications, allowing the rest of the line to be completed.  Returns
   true if the /FMT is at the end of the current line and there is nothing
   left to complete, otherwise false is returned.

   In either case *ARGS can be updated to point after any part of /FMT that
   is present.

   This function is designed so that trying to complete '/' will offer no
   completions, the user needs to insert the format specification
   themselves.  Trying to complete '/FMT' (where FMT is any non-empty set
   of alpha-numeric characters) will cause readline to insert a single
   space, setting the user up to enter the expression.  */

extern bool skip_over_slash_fmt (completion_tracker &tracker,
				 const char **args);

/* Maximum number of candidates to consider before the completer
   bails by throwing MAX_COMPLETIONS_REACHED_ERROR.  Negative values
   disable limiting.  */

extern int max_completions;

#endif /* defined (COMPLETER_H) */
