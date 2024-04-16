/* Line completion stuff for GDB, the GNU debugger.
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

#include "defs.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "expression.h"
#include "filenames.h"
#include "language.h"
#include "gdbsupport/gdb_signals.h"
#include "target.h"
#include "reggroups.h"
#include "user-regs.h"
#include "arch-utils.h"
#include "location.h"
#include <algorithm>
#include "linespec.h"
#include "cli/cli-decode.h"
#include "gdbsupport/gdb_tilde_expand.h"

/* FIXME: This is needed because of lookup_cmd_1 ().  We should be
   calling a hook instead so we eliminate the CLI dependency.  */
#include "gdbcmd.h"

/* Needed for rl_completer_word_break_characters and for
   rl_filename_completion_function.  */
#include "readline/readline.h"

/* readline defines this.  */
#undef savestring

#include "completer.h"

/* See completer.h.  */

class completion_tracker::completion_hash_entry
{
public:
  /* Constructor.  */
  completion_hash_entry (gdb::unique_xmalloc_ptr<char> name,
			 gdb::unique_xmalloc_ptr<char> lcd)
    : m_name (std::move (name)),
      m_lcd (std::move (lcd))
  {
    /* Nothing.  */
  }

  /* Returns a pointer to the lowest common denominator string.  This
     string will only be valid while this hash entry is still valid as the
     string continues to be owned by this hash entry and will be released
     when this entry is deleted.  */
  char *get_lcd () const
  {
    return m_lcd.get ();
  }

  /* Get, and release the name field from this hash entry.  This can only
     be called once, after which the name field is no longer valid.  This
     should be used to pass ownership of the name to someone else.  */
  char *release_name ()
  {
    return m_name.release ();
  }

  /* Return true of the name in this hash entry is STR.  */
  bool is_name_eq (const char *str) const
  {
    return strcmp (m_name.get (), str) == 0;
  }

  /* Return the hash value based on the name of the entry.  */
  hashval_t hash_name () const
  {
    return htab_hash_string (m_name.get ());
  }

private:

  /* The symbol name stored in this hash entry.  */
  gdb::unique_xmalloc_ptr<char> m_name;

  /* The lowest common denominator string computed for this hash entry.  */
  gdb::unique_xmalloc_ptr<char> m_lcd;
};

/* Misc state that needs to be tracked across several different
   readline completer entry point calls, all related to a single
   completion invocation.  */

struct gdb_completer_state
{
  /* The current completion's completion tracker.  This is a global
     because a tracker can be shared between the handle_brkchars and
     handle_completion phases, which involves different readline
     callbacks.  */
  completion_tracker *tracker = NULL;

  /* Whether the current completion was aborted.  */
  bool aborted = false;
};

/* The current completion state.  */
static gdb_completer_state current_completion;

/* An enumeration of the various things a user might attempt to
   complete for a location.  If you change this, remember to update
   the explicit_options array below too.  */

enum explicit_location_match_type
{
    /* The filename of a source file.  */
    MATCH_SOURCE,

    /* The name of a function or method.  */
    MATCH_FUNCTION,

    /* The fully-qualified name of a function or method.  */
    MATCH_QUALIFIED,

    /* A line number.  */
    MATCH_LINE,

    /* The name of a label.  */
    MATCH_LABEL
};

/* Prototypes for local functions.  */

/* readline uses the word breaks for two things:
   (1) In figuring out where to point the TEXT parameter to the
   rl_completion_entry_function.  Since we don't use TEXT for much,
   it doesn't matter a lot what the word breaks are for this purpose,
   but it does affect how much stuff M-? lists.
   (2) If one of the matches contains a word break character, readline
   will quote it.  That's why we switch between
   current_language->word_break_characters () and
   gdb_completer_command_word_break_characters.  I'm not sure when
   we need this behavior (perhaps for funky characters in C++ 
   symbols?).  */

/* Variables which are necessary for fancy command line editing.  */

/* When completing on command names, we remove '-' and '.' from the list of
   word break characters, since we use it in command names.  If the
   readline library sees one in any of the current completion strings,
   it thinks that the string needs to be quoted and automatically
   supplies a leading quote.  */
static const char gdb_completer_command_word_break_characters[] =
" \t\n!@#$%^&*()+=|~`}{[]\"';:?/><,";

/* When completing on file names, we remove from the list of word
   break characters any characters that are commonly used in file
   names, such as '-', '+', '~', etc.  Otherwise, readline displays
   incorrect completion candidates.  */
/* MS-DOS and MS-Windows use colon as part of the drive spec, and most
   programs support @foo style response files.  */
static const char gdb_completer_file_name_break_characters[] =
#ifdef HAVE_DOS_BASED_FILE_SYSTEM
  " \t\n*|\"';?><@";
#else
  " \t\n*|\"';:?><";
#endif

/* Characters that can be used to quote completion strings.  Note that
   we can't include '"' because the gdb C parser treats such quoted
   sequences as strings.  */
static const char gdb_completer_quote_characters[] = "'";

/* Accessor for some completer data that may interest other files.  */

const char *
get_gdb_completer_quote_characters (void)
{
  return gdb_completer_quote_characters;
}

/* This can be used for functions which don't want to complete on
   symbols but don't want to complete on anything else either.  */

void
noop_completer (struct cmd_list_element *ignore, 
		completion_tracker &tracker,
		const char *text, const char *prefix)
{
}

/* Complete on filenames.  */

void
filename_completer (struct cmd_list_element *ignore,
		    completion_tracker &tracker,
		    const char *text, const char *word)
{
  int subsequent_name;

  subsequent_name = 0;
  while (1)
    {
      gdb::unique_xmalloc_ptr<char> p_rl
	(rl_filename_completion_function (text, subsequent_name));
      if (p_rl == NULL)
	break;
      /* We need to set subsequent_name to a non-zero value before the
	 continue line below, because otherwise, if the first file
	 seen by GDB is a backup file whose name ends in a `~', we
	 will loop indefinitely.  */
      subsequent_name = 1;
      /* Like emacs, don't complete on old versions.  Especially
	 useful in the "source" command.  */
      const char *p = p_rl.get ();
      if (p[strlen (p) - 1] == '~')
	continue;

      /* Readline appends a trailing '/' if the completion is a
	 directory.  If this completion request originated from outside
	 readline (e.g. GDB's 'complete' command), then we append the
	 trailing '/' ourselves now.  */
      if (!tracker.from_readline ())
	{
	  std::string expanded = gdb_tilde_expand (p_rl.get ());
	  struct stat finfo;
	  const bool isdir = (stat (expanded.c_str (), &finfo) == 0
			      && S_ISDIR (finfo.st_mode));
	  if (isdir)
	    p_rl.reset (concat (p_rl.get (), "/", nullptr));
	}

      tracker.add_completion
	(make_completion_match_str (std::move (p_rl), text, word));
    }
#if 0
  /* There is no way to do this just long enough to affect quote
     inserting without also affecting the next completion.  This
     should be fixed in readline.  FIXME.  */
  /* Ensure that readline does the right thing
     with respect to inserting quotes.  */
  rl_completer_word_break_characters = "";
#endif
}

/* The corresponding completer_handle_brkchars
   implementation.  */

static void
filename_completer_handle_brkchars (struct cmd_list_element *ignore,
				    completion_tracker &tracker,
				    const char *text, const char *word)
{
  set_rl_completer_word_break_characters
    (gdb_completer_file_name_break_characters);
}

/* Find the bounds of the current word for completion purposes, and
   return a pointer to the end of the word.  This mimics (and is a
   modified version of) readline's _rl_find_completion_word internal
   function.

   This function skips quoted substrings (characters between matched
   pairs of characters in rl_completer_quote_characters).  We try to
   find an unclosed quoted substring on which to do matching.  If one
   is not found, we use the word break characters to find the
   boundaries of the current word.  QC, if non-null, is set to the
   opening quote character if we found an unclosed quoted substring,
   '\0' otherwise.  DP, if non-null, is set to the value of the
   delimiter character that caused a word break.  */

struct gdb_rl_completion_word_info
{
  const char *word_break_characters;
  const char *quote_characters;
  const char *basic_quote_characters;
};

static const char *
gdb_rl_find_completion_word (struct gdb_rl_completion_word_info *info,
			     int *qc, int *dp,
			     const char *line_buffer)
{
  int scan, end, delimiter, pass_next, isbrk;
  char quote_char;
  const char *brkchars;
  int point = strlen (line_buffer);

  /* The algorithm below does '--point'.  Avoid buffer underflow with
     the empty string.  */
  if (point == 0)
    {
      if (qc != NULL)
	*qc = '\0';
      if (dp != NULL)
	*dp = '\0';
      return line_buffer;
    }

  end = point;
  delimiter = 0;
  quote_char = '\0';

  brkchars = info->word_break_characters;

  if (info->quote_characters != NULL)
    {
      /* We have a list of characters which can be used in pairs to
	 quote substrings for the completer.  Try to find the start of
	 an unclosed quoted substring.  */
      for (scan = pass_next = 0;
	   scan < end;
	   scan++)
	{
	  if (pass_next)
	    {
	      pass_next = 0;
	      continue;
	    }

	  /* Shell-like semantics for single quotes -- don't allow
	     backslash to quote anything in single quotes, especially
	     not the closing quote.  If you don't like this, take out
	     the check on the value of quote_char.  */
	  if (quote_char != '\'' && line_buffer[scan] == '\\')
	    {
	      pass_next = 1;
	      continue;
	    }

	  if (quote_char != '\0')
	    {
	      /* Ignore everything until the matching close quote
		 char.  */
	      if (line_buffer[scan] == quote_char)
		{
		  /* Found matching close.  Abandon this
		     substring.  */
		  quote_char = '\0';
		  point = end;
		}
	    }
	  else if (strchr (info->quote_characters, line_buffer[scan]))
	    {
	      /* Found start of a quoted substring.  */
	      quote_char = line_buffer[scan];
	      point = scan + 1;
	    }
	}
    }

  if (point == end && quote_char == '\0')
    {
      /* We didn't find an unclosed quoted substring upon which to do
	 completion, so use the word break characters to find the
	 substring on which to complete.  */
      while (--point)
	{
	  scan = line_buffer[point];

	  if (strchr (brkchars, scan) != 0)
	    break;
	}
    }

  /* If we are at an unquoted word break, then advance past it.  */
  scan = line_buffer[point];

  if (scan)
    {
      isbrk = strchr (brkchars, scan) != 0;

      if (isbrk)
	{
	  /* If the character that caused the word break was a quoting
	     character, then remember it as the delimiter.  */
	  if (info->basic_quote_characters
	      && strchr (info->basic_quote_characters, scan)
	      && (end - point) > 1)
	    delimiter = scan;

	  point++;
	}
    }

  if (qc != NULL)
    *qc = quote_char;
  if (dp != NULL)
    *dp = delimiter;

  return line_buffer + point;
}

/* Find the completion word point for TEXT, emulating the algorithm
   readline uses to find the word point, using WORD_BREAK_CHARACTERS
   as word break characters.  */

static const char *
advance_to_completion_word (completion_tracker &tracker,
			    const char *word_break_characters,
			    const char *text)
{
  gdb_rl_completion_word_info info;

  info.word_break_characters = word_break_characters;
  info.quote_characters = gdb_completer_quote_characters;
  info.basic_quote_characters = rl_basic_quote_characters;

  int delimiter;
  const char *start
    = gdb_rl_find_completion_word (&info, NULL, &delimiter, text);

  tracker.advance_custom_word_point_by (start - text);

  if (delimiter)
    {
      tracker.set_quote_char (delimiter);
      tracker.set_suppress_append_ws (true);
    }

  return start;
}

/* See completer.h.  */

const char *
advance_to_expression_complete_word_point (completion_tracker &tracker,
					   const char *text)
{
  const char *brk_chars = current_language->word_break_characters ();
  return advance_to_completion_word (tracker, brk_chars, text);
}

/* See completer.h.  */

const char *
advance_to_filename_complete_word_point (completion_tracker &tracker,
					 const char *text)
{
  const char *brk_chars = gdb_completer_file_name_break_characters;
  return advance_to_completion_word (tracker, brk_chars, text);
}

/* See completer.h.  */

bool
completion_tracker::completes_to_completion_word (const char *word)
{
  recompute_lowest_common_denominator ();
  if (m_lowest_common_denominator_unique)
    {
      const char *lcd = m_lowest_common_denominator;

      if (strncmp_iw (word, lcd, strlen (lcd)) == 0)
	{
	  /* Maybe skip the function and complete on keywords.  */
	  size_t wordlen = strlen (word);
	  if (word[wordlen - 1] == ' ')
	    return true;
	}
    }

  return false;
}

/* See completer.h.  */

void
complete_nested_command_line (completion_tracker &tracker, const char *text)
{
  /* Must be called from a custom-word-point completer.  */
  gdb_assert (tracker.use_custom_word_point ());

  /* Disable the custom word point temporarily, because we want to
     probe whether the command we're completing itself uses a custom
     word point.  */
  tracker.set_use_custom_word_point (false);
  size_t save_custom_word_point = tracker.custom_word_point ();

  int quote_char = '\0';
  const char *word = completion_find_completion_word (tracker, text,
						      &quote_char);

  if (tracker.use_custom_word_point ())
    {
      /* The command we're completing uses a custom word point, so the
	 tracker already contains the matches.  We're done.  */
      return;
    }

  /* Restore the custom word point settings.  */
  tracker.set_custom_word_point (save_custom_word_point);
  tracker.set_use_custom_word_point (true);

  /* Run the handle_completions completer phase.  */
  complete_line (tracker, word, text, strlen (text));
}

/* Complete on linespecs, which might be of two possible forms:

       file:line
   or
       symbol+offset

   This is intended to be used in commands that set breakpoints
   etc.  */

static void
complete_files_symbols (completion_tracker &tracker,
			const char *text, const char *word)
{
  completion_list fn_list;
  const char *p;
  int quote_found = 0;
  int quoted = *text == '\'' || *text == '"';
  int quote_char = '\0';
  const char *colon = NULL;
  char *file_to_match = NULL;
  const char *symbol_start = text;
  const char *orig_text = text;

  /* Do we have an unquoted colon, as in "break foo.c:bar"?  */
  for (p = text; *p != '\0'; ++p)
    {
      if (*p == '\\' && p[1] == '\'')
	p++;
      else if (*p == '\'' || *p == '"')
	{
	  quote_found = *p;
	  quote_char = *p++;
	  while (*p != '\0' && *p != quote_found)
	    {
	      if (*p == '\\' && p[1] == quote_found)
		p++;
	      p++;
	    }

	  if (*p == quote_found)
	    quote_found = 0;
	  else
	    break;		/* Hit the end of text.  */
	}
#if HAVE_DOS_BASED_FILE_SYSTEM
      /* If we have a DOS-style absolute file name at the beginning of
	 TEXT, and the colon after the drive letter is the only colon
	 we found, pretend the colon is not there.  */
      else if (p < text + 3 && *p == ':' && p == text + 1 + quoted)
	;
#endif
      else if (*p == ':' && !colon)
	{
	  colon = p;
	  symbol_start = p + 1;
	}
      else if (strchr (current_language->word_break_characters (), *p))
	symbol_start = p + 1;
    }

  if (quoted)
    text++;

  /* Where is the file name?  */
  if (colon)
    {
      char *s;

      file_to_match = (char *) xmalloc (colon - text + 1);
      strncpy (file_to_match, text, colon - text);
      file_to_match[colon - text] = '\0';
      /* Remove trailing colons and quotes from the file name.  */
      for (s = file_to_match + (colon - text);
	   s > file_to_match;
	   s--)
	if (*s == ':' || *s == quote_char)
	  *s = '\0';
    }
  /* If the text includes a colon, they want completion only on a
     symbol name after the colon.  Otherwise, we need to complete on
     symbols as well as on files.  */
  if (colon)
    {
      collect_file_symbol_completion_matches (tracker,
					      complete_symbol_mode::EXPRESSION,
					      symbol_name_match_type::EXPRESSION,
					      symbol_start, word,
					      file_to_match);
      xfree (file_to_match);
    }
  else
    {
      size_t text_len = strlen (text);

      collect_symbol_completion_matches (tracker,
					 complete_symbol_mode::EXPRESSION,
					 symbol_name_match_type::EXPRESSION,
					 symbol_start, word);
      /* If text includes characters which cannot appear in a file
	 name, they cannot be asking for completion on files.  */
      if (strcspn (text,
		   gdb_completer_file_name_break_characters) == text_len)
	fn_list = make_source_files_completion_list (text, text);
    }

  if (!fn_list.empty () && !tracker.have_completions ())
    {
      /* If we only have file names as possible completion, we should
	 bring them in sync with what rl_complete expects.  The
	 problem is that if the user types "break /foo/b TAB", and the
	 possible completions are "/foo/bar" and "/foo/baz"
	 rl_complete expects us to return "bar" and "baz", without the
	 leading directories, as possible completions, because `word'
	 starts at the "b".  But we ignore the value of `word' when we
	 call make_source_files_completion_list above (because that
	 would not DTRT when the completion results in both symbols
	 and file names), so make_source_files_completion_list returns
	 the full "/foo/bar" and "/foo/baz" strings.  This produces
	 wrong results when, e.g., there's only one possible
	 completion, because rl_complete will prepend "/foo/" to each
	 candidate completion.  The loop below removes that leading
	 part.  */
      for (const auto &fn_up: fn_list)
	{
	  char *fn = fn_up.get ();
	  memmove (fn, fn + (word - text), strlen (fn) + 1 - (word - text));
	}
    }

  tracker.add_completions (std::move (fn_list));

  if (!tracker.have_completions ())
    {
      /* No completions at all.  As the final resort, try completing
	 on the entire text as a symbol.  */
      collect_symbol_completion_matches (tracker,
					 complete_symbol_mode::EXPRESSION,
					 symbol_name_match_type::EXPRESSION,
					 orig_text, word);
    }
}

/* See completer.h.  */

completion_list
complete_source_filenames (const char *text)
{
  size_t text_len = strlen (text);

  /* If text includes characters which cannot appear in a file name,
     the user cannot be asking for completion on files.  */
  if (strcspn (text,
	       gdb_completer_file_name_break_characters)
      == text_len)
    return make_source_files_completion_list (text, text);

  return {};
}

/* Complete address and linespec locations.  */

static void
complete_address_and_linespec_locations (completion_tracker &tracker,
					 const char *text,
					 symbol_name_match_type match_type)
{
  if (*text == '*')
    {
      tracker.advance_custom_word_point_by (1);
      text++;
      const char *word
	= advance_to_expression_complete_word_point (tracker, text);
      complete_expression (tracker, text, word);
    }
  else
    {
      linespec_complete (tracker, text, match_type);
    }
}

/* The explicit location options.  Note that indexes into this array
   must match the explicit_location_match_type enumerators.  */

static const char *const explicit_options[] =
  {
    "-source",
    "-function",
    "-qualified",
    "-line",
    "-label",
    NULL
  };

/* The probe modifier options.  These can appear before a location in
   breakpoint commands.  */
static const char *const probe_options[] =
  {
    "-probe",
    "-probe-stap",
    "-probe-dtrace",
    NULL
  };

/* Returns STRING if not NULL, the empty string otherwise.  */

static const char *
string_or_empty (const char *string)
{
  return string != NULL ? string : "";
}

/* A helper function to collect explicit location matches for the given
   LOCATION, which is attempting to match on WORD.  */

static void
collect_explicit_location_matches (completion_tracker &tracker,
				   location_spec *locspec,
				   enum explicit_location_match_type what,
				   const char *word,
				   const struct language_defn *language)
{
  const explicit_location_spec *explicit_loc
    = as_explicit_location_spec (locspec);

  /* True if the option expects an argument.  */
  bool needs_arg = true;

  /* Note, in the various MATCH_* below, we complete on
     explicit_loc->foo instead of WORD, because only the former will
     have already skipped past any quote char.  */
  switch (what)
    {
    case MATCH_SOURCE:
      {
	const char *source
	  = string_or_empty (explicit_loc->source_filename.get ());
	completion_list matches
	  = make_source_files_completion_list (source, source);
	tracker.add_completions (std::move (matches));
      }
      break;

    case MATCH_FUNCTION:
      {
	const char *function
	  = string_or_empty (explicit_loc->function_name.get ());
	linespec_complete_function (tracker, function,
				    explicit_loc->func_name_match_type,
				    explicit_loc->source_filename.get ());
      }
      break;

    case MATCH_QUALIFIED:
      needs_arg = false;
      break;
    case MATCH_LINE:
      /* Nothing to offer.  */
      break;

    case MATCH_LABEL:
      {
	const char *label = string_or_empty (explicit_loc->label_name.get ());
	linespec_complete_label (tracker, language,
				 explicit_loc->source_filename.get (),
				 explicit_loc->function_name.get (),
				 explicit_loc->func_name_match_type,
				 label);
      }
      break;

    default:
      gdb_assert_not_reached ("unhandled explicit_location_match_type");
    }

  if (!needs_arg || tracker.completes_to_completion_word (word))
    {
      tracker.discard_completions ();
      tracker.advance_custom_word_point_by (strlen (word));
      complete_on_enum (tracker, explicit_options, "", "");
      complete_on_enum (tracker, linespec_keywords, "", "");
    }
  else if (!tracker.have_completions ())
    {
      /* Maybe we have an unterminated linespec keyword at the tail of
	 the string.  Try completing on that.  */
      size_t wordlen = strlen (word);
      const char *keyword = word + wordlen;

      if (wordlen > 0 && keyword[-1] != ' ')
	{
	  while (keyword > word && *keyword != ' ')
	    keyword--;
	  /* Don't complete on keywords if we'd be completing on the
	     whole explicit linespec option.  E.g., "b -function
	     thr<tab>" should not complete to the "thread"
	     keyword.  */
	  if (keyword != word)
	    {
	      keyword = skip_spaces (keyword);

	      tracker.advance_custom_word_point_by (keyword - word);
	      complete_on_enum (tracker, linespec_keywords, keyword, keyword);
	    }
	}
      else if (wordlen > 0 && keyword[-1] == ' ')
	{
	  /* Assume that we're maybe past the explicit location
	     argument, and we didn't manage to find any match because
	     the user wants to create a pending breakpoint.  Offer the
	     keyword and explicit location options as possible
	     completions.  */
	  tracker.advance_custom_word_point_by (keyword - word);
	  complete_on_enum (tracker, linespec_keywords, keyword, keyword);
	  complete_on_enum (tracker, explicit_options, keyword, keyword);
	}
    }
}

/* If the next word in *TEXT_P is any of the keywords in KEYWORDS,
   then advance both TEXT_P and the word point in the tracker past the
   keyword and return the (0-based) index in the KEYWORDS array that
   matched.  Otherwise, return -1.  */

static int
skip_keyword (completion_tracker &tracker,
	      const char * const *keywords, const char **text_p)
{
  const char *text = *text_p;
  const char *after = skip_to_space (text);
  size_t len = after - text;

  if (text[len] != ' ')
    return -1;

  int found = -1;
  for (int i = 0; keywords[i] != NULL; i++)
    {
      if (strncmp (keywords[i], text, len) == 0)
	{
	  if (found == -1)
	    found = i;
	  else
	    return -1;
	}
    }

  if (found != -1)
    {
      tracker.advance_custom_word_point_by (len + 1);
      text += len + 1;
      *text_p = text;
      return found;
    }

  return -1;
}

/* A completer function for explicit location specs.  This function
   completes both options ("-source", "-line", etc) and values.  If
   completing a quoted string, then QUOTED_ARG_START and
   QUOTED_ARG_END point to the quote characters.  LANGUAGE is the
   current language.  */

static void
complete_explicit_location_spec (completion_tracker &tracker,
				 location_spec *locspec,
				 const char *text,
				 const language_defn *language,
				 const char *quoted_arg_start,
				 const char *quoted_arg_end)
{
  if (*text != '-')
    return;

  int keyword = skip_keyword (tracker, explicit_options, &text);

  if (keyword == -1)
    {
      complete_on_enum (tracker, explicit_options, text, text);
      /* There are keywords that start with "-".   Include them, too.  */
      complete_on_enum (tracker, linespec_keywords, text, text);
    }
  else
    {
      /* Completing on value.  */
      enum explicit_location_match_type what
	= (explicit_location_match_type) keyword;

      if (quoted_arg_start != NULL && quoted_arg_end != NULL)
	{
	  if (quoted_arg_end[1] == '\0')
	    {
	      /* If completing a quoted string with the cursor right
		 at the terminating quote char, complete the
		 completion word without interpretation, so that
		 readline advances the cursor one whitespace past the
		 quote, even if there's no match.  This makes these
		 cases behave the same:

		   before: "b -function function()"
		   after:  "b -function function() "

		   before: "b -function 'function()'"
		   after:  "b -function 'function()' "

		 and trusts the user in this case:

		   before: "b -function 'not_loaded_function_yet()'"
		   after:  "b -function 'not_loaded_function_yet()' "
	      */
	      tracker.add_completion (make_unique_xstrdup (text));
	    }
	  else if (quoted_arg_end[1] == ' ')
	    {
	      /* We're maybe past the explicit location argument.
		 Skip the argument without interpretation, assuming the
		 user may want to create pending breakpoint.  Offer
		 the keyword and explicit location options as possible
		 completions.  */
	      tracker.advance_custom_word_point_by (strlen (text));
	      complete_on_enum (tracker, linespec_keywords, "", "");
	      complete_on_enum (tracker, explicit_options, "", "");
	    }
	  return;
	}

      /* Now gather matches  */
      collect_explicit_location_matches (tracker, locspec, what, text,
					 language);
    }
}

/* A completer for locations.  */

void
location_completer (struct cmd_list_element *ignore,
		    completion_tracker &tracker,
		    const char *text, const char * /* word */)
{
  int found_probe_option = -1;

  /* If we have a probe modifier, skip it.  This can only appear as
     first argument.  Until we have a specific completer for probes,
     falling back to the linespec completer for the remainder of the
     line is better than nothing.  */
  if (text[0] == '-' && text[1] == 'p')
    found_probe_option = skip_keyword (tracker, probe_options, &text);

  const char *option_text = text;
  int saved_word_point = tracker.custom_word_point ();

  const char *copy = text;

  explicit_completion_info completion_info;
  location_spec_up locspec
    = string_to_explicit_location_spec (&copy, current_language,
					&completion_info);
  if (completion_info.quoted_arg_start != NULL
      && completion_info.quoted_arg_end == NULL)
    {
      /* Found an unbalanced quote.  */
      tracker.set_quote_char (*completion_info.quoted_arg_start);
      tracker.advance_custom_word_point_by (1);
    }

  if (completion_info.saw_explicit_location_spec_option)
    {
      if (*copy != '\0')
	{
	  tracker.advance_custom_word_point_by (copy - text);
	  text = copy;

	  /* We found a terminator at the tail end of the string,
	     which means we're past the explicit location options.  We
	     may have a keyword to complete on.  If we have a whole
	     keyword, then complete whatever comes after as an
	     expression.  This is mainly for the "if" keyword.  If the
	     "thread" and "task" keywords gain their own completers,
	     they should be used here.  */
	  int keyword = skip_keyword (tracker, linespec_keywords, &text);

	  if (keyword == -1)
	    {
	      complete_on_enum (tracker, linespec_keywords, text, text);
	    }
	  else
	    {
	      const char *word
		= advance_to_expression_complete_word_point (tracker, text);
	      complete_expression (tracker, text, word);
	    }
	}
      else
	{
	  tracker.advance_custom_word_point_by (completion_info.last_option
						- text);
	  text = completion_info.last_option;

	  complete_explicit_location_spec (tracker, locspec.get (), text,
					   current_language,
					   completion_info.quoted_arg_start,
					   completion_info.quoted_arg_end);

	}
    }
  /* This is an address or linespec location.  */
  else if (locspec != nullptr)
    {
      /* Handle non-explicit location options.  */

      int keyword = skip_keyword (tracker, explicit_options, &text);
      if (keyword == -1)
	complete_on_enum (tracker, explicit_options, text, text);
      else
	{
	  tracker.advance_custom_word_point_by (copy - text);
	  text = copy;

	  symbol_name_match_type match_type
	    = as_explicit_location_spec (locspec.get ())->func_name_match_type;
	  complete_address_and_linespec_locations (tracker, text, match_type);
	}
    }
  else
    {
      /* No options.  */
      complete_address_and_linespec_locations (tracker, text,
					       symbol_name_match_type::WILD);
    }

  /* Add matches for option names, if either:

     - Some completer above found some matches, but the word point did
       not advance (e.g., "b <tab>" finds all functions, or "b -<tab>"
       matches all objc selectors), or;

     - Some completer above advanced the word point, but found no
       matches.
  */
  if ((text[0] == '-' || text[0] == '\0')
      && (!tracker.have_completions ()
	  || tracker.custom_word_point () == saved_word_point))
    {
      tracker.set_custom_word_point (saved_word_point);
      text = option_text;

      if (found_probe_option == -1)
	complete_on_enum (tracker, probe_options, text, text);
      complete_on_enum (tracker, explicit_options, text, text);
    }
}

/* The corresponding completer_handle_brkchars
   implementation.  */

static void
location_completer_handle_brkchars (struct cmd_list_element *ignore,
				    completion_tracker &tracker,
				    const char *text,
				    const char *word_ignored)
{
  tracker.set_use_custom_word_point (true);

  location_completer (ignore, tracker, text, NULL);
}

/* See completer.h.  */

void
complete_expression (completion_tracker &tracker,
		     const char *text, const char *word)
{
  expression_up exp;
  std::unique_ptr<expr_completion_base> expr_completer;

  /* Perform a tentative parse of the expression, to see whether a
     field completion is required.  */
  try
    {
      exp = parse_expression_for_completion (text, &expr_completer);
    }
  catch (const gdb_exception_error &except)
    {
      return;
    }

  /* Part of the parse_expression_for_completion contract.  */
  gdb_assert ((exp == nullptr) == (expr_completer == nullptr));
  if (expr_completer != nullptr
      && expr_completer->complete (exp.get (), tracker))
    return;

  complete_files_symbols (tracker, text, word);
}

/* Complete on expressions.  Often this means completing on symbol
   names, but some language parsers also have support for completing
   field names.  */

void
expression_completer (struct cmd_list_element *ignore,
		      completion_tracker &tracker,
		      const char *text, const char *word)
{
  complete_expression (tracker, text, word);
}

/* See definition in completer.h.  */

void
set_rl_completer_word_break_characters (const char *break_chars)
{
  rl_completer_word_break_characters = (char *) break_chars;
}

/* Complete on symbols.  */

void
symbol_completer (struct cmd_list_element *ignore,
		  completion_tracker &tracker,
		  const char *text, const char *word)
{
  collect_symbol_completion_matches (tracker, complete_symbol_mode::EXPRESSION,
				     symbol_name_match_type::EXPRESSION,
				     text, word);
}

/* Here are some useful test cases for completion.  FIXME: These
   should be put in the test suite.  They should be tested with both
   M-? and TAB.

   "show output-" "radix"
   "show output" "-radix"
   "p" ambiguous (commands starting with p--path, print, printf, etc.)
   "p "  ambiguous (all symbols)
   "info t foo" no completions
   "info t " no completions
   "info t" ambiguous ("info target", "info terminal", etc.)
   "info ajksdlfk" no completions
   "info ajksdlfk " no completions
   "info" " "
   "info " ambiguous (all info commands)
   "p \"a" no completions (string constant)
   "p 'a" ambiguous (all symbols starting with a)
   "p b-a" ambiguous (all symbols starting with a)
   "p b-" ambiguous (all symbols)
   "file Make" "file" (word break hard to screw up here)
   "file ../gdb.stabs/we" "ird" (needs to not break word at slash)
 */

enum complete_line_internal_reason
{
  /* Preliminary phase, called by gdb_completion_word_break_characters
     function, is used to either:

     #1 - Determine the set of chars that are word delimiters
	  depending on the current command in line_buffer.

     #2 - Manually advance RL_POINT to the "word break" point instead
	  of letting readline do it (based on too-simple character
	  matching).

     Simpler completers that just pass a brkchars array to readline
     (#1 above) must defer generating the completions to the main
     phase (below).  No completion list should be generated in this
     phase.

     OTOH, completers that manually advance the word point(#2 above)
     must set "use_custom_word_point" in the tracker and generate
     their completion in this phase.  Note that this is the convenient
     thing to do since they'll be parsing the input line anyway.  */
  handle_brkchars,

  /* Main phase, called by complete_line function, is used to get the
     list of possible completions.  */
  handle_completions,

  /* Special case when completing a 'help' command.  In this case,
     once sub-command completions are exhausted, we simply return
     NULL.  */
  handle_help,
};

/* Helper for complete_line_internal to simplify it.  */

static void
complete_line_internal_normal_command (completion_tracker &tracker,
				       const char *command, const char *word,
				       const char *cmd_args,
				       complete_line_internal_reason reason,
				       struct cmd_list_element *c)
{
  const char *p = cmd_args;

  if (c->completer == filename_completer)
    {
      /* Many commands which want to complete on file names accept
	 several file names, as in "run foo bar >>baz".  So we don't
	 want to complete the entire text after the command, just the
	 last word.  To this end, we need to find the beginning of the
	 file name by starting at `word' and going backwards.  */
      for (p = word;
	   p > command
	     && strchr (gdb_completer_file_name_break_characters,
			p[-1]) == NULL;
	   p--)
	;
    }

  if (reason == handle_brkchars)
    {
      completer_handle_brkchars_ftype *brkchars_fn;

      if (c->completer_handle_brkchars != NULL)
	brkchars_fn = c->completer_handle_brkchars;
      else
	{
	  brkchars_fn
	    = (completer_handle_brkchars_func_for_completer
	       (c->completer));
	}

      brkchars_fn (c, tracker, p, word);
    }

  if (reason != handle_brkchars && c->completer != NULL)
    (*c->completer) (c, tracker, p, word);
}

/* Internal function used to handle completions.


   TEXT is the caller's idea of the "word" we are looking at.

   LINE_BUFFER is available to be looked at; it contains the entire
   text of the line.  POINT is the offset in that line of the cursor.
   You should pretend that the line ends at POINT.

   See complete_line_internal_reason for description of REASON.  */

static void
complete_line_internal_1 (completion_tracker &tracker,
			  const char *text,
			  const char *line_buffer, int point,
			  complete_line_internal_reason reason)
{
  char *tmp_command;
  const char *p;
  int ignore_help_classes;
  /* Pointer within tmp_command which corresponds to text.  */
  const char *word;
  struct cmd_list_element *c, *result_list;

  /* Choose the default set of word break characters to break
     completions.  If we later find out that we are doing completions
     on command strings (as opposed to strings supplied by the
     individual command completer functions, which can be any string)
     then we will switch to the special word break set for command
     strings, which leaves out the '-' and '.' character used in some
     commands.  */
  set_rl_completer_word_break_characters
    (current_language->word_break_characters ());

  /* Decide whether to complete on a list of gdb commands or on
     symbols.  */
  tmp_command = (char *) alloca (point + 1);
  p = tmp_command;

  /* The help command should complete help aliases.  */
  ignore_help_classes = reason != handle_help;

  strncpy (tmp_command, line_buffer, point);
  tmp_command[point] = '\0';
  if (reason == handle_brkchars)
    {
      gdb_assert (text == NULL);
      word = NULL;
    }
  else
    {
      /* Since text always contains some number of characters leading up
	 to point, we can find the equivalent position in tmp_command
	 by subtracting that many characters from the end of tmp_command.  */
      word = tmp_command + point - strlen (text);
    }

  /* Move P up to the start of the command.  */
  p = skip_spaces (p);

  if (*p == '\0')
    {
      /* An empty line is ambiguous; that is, it could be any
	 command.  */
      c = CMD_LIST_AMBIGUOUS;
      result_list = 0;
    }
  else
    c = lookup_cmd_1 (&p, cmdlist, &result_list, NULL, ignore_help_classes,
		      true);

  /* Move p up to the next interesting thing.  */
  while (*p == ' ' || *p == '\t')
    {
      p++;
    }

  tracker.advance_custom_word_point_by (p - tmp_command);

  if (!c)
    {
      /* It is an unrecognized command.  So there are no
	 possible completions.  */
    }
  else if (c == CMD_LIST_AMBIGUOUS)
    {
      const char *q;

      /* lookup_cmd_1 advances p up to the first ambiguous thing, but
	 doesn't advance over that thing itself.  Do so now.  */
      q = p;
      while (valid_cmd_char_p (*q))
	++q;
      if (q != tmp_command + point)
	{
	  /* There is something beyond the ambiguous
	     command, so there are no possible completions.  For
	     example, "info t " or "info t foo" does not complete
	     to anything, because "info t" can be "info target" or
	     "info terminal".  */
	}
      else
	{
	  /* We're trying to complete on the command which was ambiguous.
	     This we can deal with.  */
	  if (result_list)
	    {
	      if (reason != handle_brkchars)
		complete_on_cmdlist (*result_list->subcommands, tracker, p,
				     word, ignore_help_classes);
	    }
	  else
	    {
	      if (reason != handle_brkchars)
		complete_on_cmdlist (cmdlist, tracker, p, word,
				     ignore_help_classes);
	    }
	  /* Ensure that readline does the right thing with respect to
	     inserting quotes.  */
	  set_rl_completer_word_break_characters
	    (gdb_completer_command_word_break_characters);
	}
    }
  else
    {
      /* We've recognized a full command.  */

      if (p == tmp_command + point)
	{
	  /* There is no non-whitespace in the line beyond the
	     command.  */

	  if (p[-1] == ' ' || p[-1] == '\t')
	    {
	      /* The command is followed by whitespace; we need to
		 complete on whatever comes after command.  */
	      if (c->is_prefix ())
		{
		  /* It is a prefix command; what comes after it is
		     a subcommand (e.g. "info ").  */
		  if (reason != handle_brkchars)
		    complete_on_cmdlist (*c->subcommands, tracker, p, word,
					 ignore_help_classes);

		  /* Ensure that readline does the right thing
		     with respect to inserting quotes.  */
		  set_rl_completer_word_break_characters
		    (gdb_completer_command_word_break_characters);
		}
	      else if (reason == handle_help)
		;
	      else if (c->enums)
		{
		  if (reason != handle_brkchars)
		    complete_on_enum (tracker, c->enums, p, word);
		  set_rl_completer_word_break_characters
		    (gdb_completer_command_word_break_characters);
		}
	      else
		{
		  /* It is a normal command; what comes after it is
		     completed by the command's completer function.  */
		  complete_line_internal_normal_command (tracker,
							 tmp_command, word, p,
							 reason, c);
		}
	    }
	  else
	    {
	      /* The command is not followed by whitespace; we need to
		 complete on the command itself, e.g. "p" which is a
		 command itself but also can complete to "print", "ptype"
		 etc.  */
	      const char *q;

	      /* Find the command we are completing on.  */
	      q = p;
	      while (q > tmp_command)
		{
		  if (valid_cmd_char_p (q[-1]))
		    --q;
		  else
		    break;
		}

	      /* Move the custom word point back too.  */
	      tracker.advance_custom_word_point_by (q - p);

	      if (reason != handle_brkchars)
		complete_on_cmdlist (result_list, tracker, q, word,
				     ignore_help_classes);

	      /* Ensure that readline does the right thing
		 with respect to inserting quotes.  */
	      set_rl_completer_word_break_characters
		(gdb_completer_command_word_break_characters);
	    }
	}
      else if (reason == handle_help)
	;
      else
	{
	  /* There is non-whitespace beyond the command.  */

	  if (c->is_prefix () && !c->allow_unknown)
	    {
	      /* It is an unrecognized subcommand of a prefix command,
		 e.g. "info adsfkdj".  */
	    }
	  else if (c->enums)
	    {
	      if (reason != handle_brkchars)
		complete_on_enum (tracker, c->enums, p, word);
	    }
	  else
	    {
	      /* It is a normal command.  */
	      complete_line_internal_normal_command (tracker,
						     tmp_command, word, p,
						     reason, c);
	    }
	}
    }
}

/* Wrapper around complete_line_internal_1 to handle
   MAX_COMPLETIONS_REACHED_ERROR.  */

static void
complete_line_internal (completion_tracker &tracker,
			const char *text,
			const char *line_buffer, int point,
			complete_line_internal_reason reason)
{
  try
    {
      complete_line_internal_1 (tracker, text, line_buffer, point, reason);
    }
  catch (const gdb_exception_error &except)
    {
      if (except.error != MAX_COMPLETIONS_REACHED_ERROR)
	throw;
    }
}

/* See completer.h.  */

int max_completions = 200;

/* Initial size of the table.  It automagically grows from here.  */
#define INITIAL_COMPLETION_HTAB_SIZE 200

/* See completer.h.  */

completion_tracker::completion_tracker (bool from_readline)
  : m_from_readline (from_readline)
{
  discard_completions ();
}

/* See completer.h.  */

void
completion_tracker::discard_completions ()
{
  xfree (m_lowest_common_denominator);
  m_lowest_common_denominator = NULL;

  m_lowest_common_denominator_unique = false;
  m_lowest_common_denominator_valid = false;

  m_entries_hash.reset (nullptr);

  /* A callback used by the hash table to compare new entries with existing
     entries.  We can't use the standard htab_eq_string function here as the
     key to our hash is just a single string, while the values we store in
     the hash are a struct containing multiple strings.  */
  static auto entry_eq_func
    = [] (const void *first, const void *second) -> int
      {
	/* The FIRST argument is the entry already in the hash table, and
	   the SECOND argument is the new item being inserted.  */
	const completion_hash_entry *entry
	  = (const completion_hash_entry *) first;
	const char *name_str = (const char *) second;

	return entry->is_name_eq (name_str);
      };

  /* Callback used by the hash table to compute the hash value for an
     existing entry.  This is needed when expanding the hash table.  */
  static auto entry_hash_func
    = [] (const void *arg) -> hashval_t
      {
	const completion_hash_entry *entry
	  = (const completion_hash_entry *) arg;
	return entry->hash_name ();
      };

  m_entries_hash.reset
    (htab_create_alloc (INITIAL_COMPLETION_HTAB_SIZE,
			entry_hash_func, entry_eq_func,
			htab_delete_entry<completion_hash_entry>,
			xcalloc, xfree));
}

/* See completer.h.  */

completion_tracker::~completion_tracker ()
{
  xfree (m_lowest_common_denominator);
}

/* See completer.h.  */

bool
completion_tracker::maybe_add_completion
  (gdb::unique_xmalloc_ptr<char> name,
   completion_match_for_lcd *match_for_lcd,
   const char *text, const char *word)
{
  void **slot;

  if (max_completions == 0)
    return false;

  if (htab_elements (m_entries_hash.get ()) >= max_completions)
    return false;

  hashval_t hash = htab_hash_string (name.get ());
  slot = htab_find_slot_with_hash (m_entries_hash.get (), name.get (),
				   hash, INSERT);
  if (*slot == HTAB_EMPTY_ENTRY)
    {
      const char *match_for_lcd_str = NULL;

      if (match_for_lcd != NULL)
	match_for_lcd_str = match_for_lcd->finish ();

      if (match_for_lcd_str == NULL)
	match_for_lcd_str = name.get ();

      gdb::unique_xmalloc_ptr<char> lcd
	= make_completion_match_str (match_for_lcd_str, text, word);

      size_t lcd_len = strlen (lcd.get ());
      *slot = new completion_hash_entry (std::move (name), std::move (lcd));

      m_lowest_common_denominator_valid = false;
      m_lowest_common_denominator_max_length
	= std::max (m_lowest_common_denominator_max_length, lcd_len);
    }

  return true;
}

/* See completer.h.  */

void
completion_tracker::add_completion (gdb::unique_xmalloc_ptr<char> name,
				    completion_match_for_lcd *match_for_lcd,
				    const char *text, const char *word)
{
  if (!maybe_add_completion (std::move (name), match_for_lcd, text, word))
    throw_error (MAX_COMPLETIONS_REACHED_ERROR, _("Max completions reached."));
}

/* See completer.h.  */

void
completion_tracker::add_completions (completion_list &&list)
{
  for (auto &candidate : list)
    add_completion (std::move (candidate));
}

/* See completer.h.  */

void
completion_tracker::remove_completion (const char *name)
{
  hashval_t hash = htab_hash_string (name);
  if (htab_find_slot_with_hash (m_entries_hash.get (), name, hash, NO_INSERT)
      != NULL)
    {
      htab_remove_elt_with_hash (m_entries_hash.get (), name, hash);
      m_lowest_common_denominator_valid = false;
    }
}

/* Helper for the make_completion_match_str overloads.  Returns NULL
   as an indication that we want MATCH_NAME exactly.  It is up to the
   caller to xstrdup that string if desired.  */

static char *
make_completion_match_str_1 (const char *match_name,
			     const char *text, const char *word)
{
  char *newobj;

  if (word == text)
    {
      /* Return NULL as an indication that we want MATCH_NAME
	 exactly.  */
      return NULL;
    }
  else if (word > text)
    {
      /* Return some portion of MATCH_NAME.  */
      newobj = xstrdup (match_name + (word - text));
    }
  else
    {
      /* Return some of WORD plus MATCH_NAME.  */
      size_t len = strlen (match_name);
      newobj = (char *) xmalloc (text - word + len + 1);
      memcpy (newobj, word, text - word);
      memcpy (newobj + (text - word), match_name, len + 1);
    }

  return newobj;
}

/* See completer.h.  */

gdb::unique_xmalloc_ptr<char>
make_completion_match_str (const char *match_name,
			   const char *text, const char *word)
{
  char *newobj = make_completion_match_str_1 (match_name, text, word);
  if (newobj == NULL)
    newobj = xstrdup (match_name);
  return gdb::unique_xmalloc_ptr<char> (newobj);
}

/* See completer.h.  */

gdb::unique_xmalloc_ptr<char>
make_completion_match_str (gdb::unique_xmalloc_ptr<char> &&match_name,
			   const char *text, const char *word)
{
  char *newobj = make_completion_match_str_1 (match_name.get (), text, word);
  if (newobj == NULL)
    return std::move (match_name);
  return gdb::unique_xmalloc_ptr<char> (newobj);
}

/* See complete.h.  */

completion_result
complete (const char *line, char const **word, int *quote_char)
{
  completion_tracker tracker_handle_brkchars (false);
  completion_tracker tracker_handle_completions (false);
  completion_tracker *tracker;

  /* The WORD should be set to the end of word to complete.  We initialize
     to the completion point which is assumed to be at the end of LINE.
     This leaves WORD to be initialized to a sensible value in cases
     completion_find_completion_word() fails i.e., throws an exception.
     See bug 24587. */
  *word = line + strlen (line);

  try
    {
      *word = completion_find_completion_word (tracker_handle_brkchars,
					      line, quote_char);

      /* Completers that provide a custom word point in the
	 handle_brkchars phase also compute their completions then.
	 Completers that leave the completion word handling to readline
	 must be called twice.  */
      if (tracker_handle_brkchars.use_custom_word_point ())
	tracker = &tracker_handle_brkchars;
      else
	{
	  complete_line (tracker_handle_completions, *word, line, strlen (line));
	  tracker = &tracker_handle_completions;
	}
    }
  catch (const gdb_exception &ex)
    {
      return {};
    }

  return tracker->build_completion_result (*word, *word - line, strlen (line));
}


/* Generate completions all at once.  Does nothing if max_completions
   is 0.  If max_completions is non-negative, this will collect at
   most max_completions strings.

   TEXT is the caller's idea of the "word" we are looking at.

   LINE_BUFFER is available to be looked at; it contains the entire
   text of the line.

   POINT is the offset in that line of the cursor.  You
   should pretend that the line ends at POINT.  */

void
complete_line (completion_tracker &tracker,
	       const char *text, const char *line_buffer, int point)
{
  if (max_completions == 0)
    return;
  complete_line_internal (tracker, text, line_buffer, point,
			  handle_completions);
}

/* Complete on command names.  Used by "help".  */

void
command_completer (struct cmd_list_element *ignore, 
		   completion_tracker &tracker,
		   const char *text, const char *word)
{
  complete_line_internal (tracker, word, text,
			  strlen (text), handle_help);
}

/* The corresponding completer_handle_brkchars implementation.  */

static void
command_completer_handle_brkchars (struct cmd_list_element *ignore,
				   completion_tracker &tracker,
				   const char *text, const char *word)
{
  set_rl_completer_word_break_characters
    (gdb_completer_command_word_break_characters);
}

/* Complete on signals.  */

void
signal_completer (struct cmd_list_element *ignore,
		  completion_tracker &tracker,
		  const char *text, const char *word)
{
  size_t len = strlen (word);
  int signum;
  const char *signame;

  for (signum = GDB_SIGNAL_FIRST; signum != GDB_SIGNAL_LAST; ++signum)
    {
      /* Can't handle this, so skip it.  */
      if (signum == GDB_SIGNAL_0)
	continue;

      signame = gdb_signal_to_name ((enum gdb_signal) signum);

      /* Ignore the unknown signal case.  */
      if (!signame || strcmp (signame, "?") == 0)
	continue;

      if (strncasecmp (signame, word, len) == 0)
	tracker.add_completion (make_unique_xstrdup (signame));
    }
}

/* Bit-flags for selecting what the register and/or register-group
   completer should complete on.  */

enum reg_completer_target
  {
    complete_register_names = 0x1,
    complete_reggroup_names = 0x2
  };
DEF_ENUM_FLAGS_TYPE (enum reg_completer_target, reg_completer_targets);

/* Complete register names and/or reggroup names based on the value passed
   in TARGETS.  At least one bit in TARGETS must be set.  */

static void
reg_or_group_completer_1 (completion_tracker &tracker,
			  const char *text, const char *word,
			  reg_completer_targets targets)
{
  size_t len = strlen (word);
  struct gdbarch *gdbarch;
  const char *name;

  gdb_assert ((targets & (complete_register_names
			  | complete_reggroup_names)) != 0);
  gdbarch = get_current_arch ();

  if ((targets & complete_register_names) != 0)
    {
      int i;

      for (i = 0;
	   (name = user_reg_map_regnum_to_name (gdbarch, i)) != NULL;
	   i++)
	{
	  if (*name != '\0' && strncmp (word, name, len) == 0)
	    tracker.add_completion (make_unique_xstrdup (name));
	}
    }

  if ((targets & complete_reggroup_names) != 0)
    {
      for (const struct reggroup *group : gdbarch_reggroups (gdbarch))
	{
	  name = group->name ();
	  if (strncmp (word, name, len) == 0)
	    tracker.add_completion (make_unique_xstrdup (name));
	}
    }
}

/* Perform completion on register and reggroup names.  */

void
reg_or_group_completer (struct cmd_list_element *ignore,
			completion_tracker &tracker,
			const char *text, const char *word)
{
  reg_or_group_completer_1 (tracker, text, word,
			    (complete_register_names
			     | complete_reggroup_names));
}

/* Perform completion on reggroup names.  */

void
reggroup_completer (struct cmd_list_element *ignore,
		    completion_tracker &tracker,
		    const char *text, const char *word)
{
  reg_or_group_completer_1 (tracker, text, word,
			    complete_reggroup_names);
}

/* The default completer_handle_brkchars implementation.  */

static void
default_completer_handle_brkchars (struct cmd_list_element *ignore,
				   completion_tracker &tracker,
				   const char *text, const char *word)
{
  set_rl_completer_word_break_characters
    (current_language->word_break_characters ());
}

/* See definition in completer.h.  */

completer_handle_brkchars_ftype *
completer_handle_brkchars_func_for_completer (completer_ftype *fn)
{
  if (fn == filename_completer)
    return filename_completer_handle_brkchars;

  if (fn == location_completer)
    return location_completer_handle_brkchars;

  if (fn == command_completer)
    return command_completer_handle_brkchars;

  return default_completer_handle_brkchars;
}

/* Used as brkchars when we want to tell readline we have a custom
   word point.  We do that by making our rl_completion_word_break_hook
   set RL_POINT to the desired word point, and return the character at
   the word break point as the break char.  This is two bytes in order
   to fit one break character plus the terminating null.  */
static char gdb_custom_word_point_brkchars[2];

/* Since rl_basic_quote_characters is not completer-specific, we save
   its original value here, in order to be able to restore it in
   gdb_rl_attempted_completion_function.  */
static const char *gdb_org_rl_basic_quote_characters = rl_basic_quote_characters;

/* Get the list of chars that are considered as word breaks
   for the current command.  */

static char *
gdb_completion_word_break_characters_throw ()
{
  /* New completion starting.  Get rid of the previous tracker and
     start afresh.  */
  delete current_completion.tracker;
  current_completion.tracker = new completion_tracker (true);

  completion_tracker &tracker = *current_completion.tracker;

  complete_line_internal (tracker, NULL, rl_line_buffer,
			  rl_point, handle_brkchars);

  if (tracker.use_custom_word_point ())
    {
      gdb_assert (tracker.custom_word_point () > 0);
      rl_point = tracker.custom_word_point () - 1;

      gdb_assert (rl_point >= 0 && rl_point < strlen (rl_line_buffer));

      gdb_custom_word_point_brkchars[0] = rl_line_buffer[rl_point];
      rl_completer_word_break_characters = gdb_custom_word_point_brkchars;
      rl_completer_quote_characters = NULL;

      /* Clear this too, so that if we're completing a quoted string,
	 readline doesn't consider the quote character a delimiter.
	 If we didn't do this, readline would auto-complete {b
	 'fun<tab>} to {'b 'function()'}, i.e., add the terminating
	 \', but, it wouldn't append the separator space either, which
	 is not desirable.  So instead we take care of appending the
	 quote character to the LCD ourselves, in
	 gdb_rl_attempted_completion_function.  Since this global is
	 not just completer-specific, we'll restore it back to the
	 default in gdb_rl_attempted_completion_function.  */
      rl_basic_quote_characters = NULL;
    }

  return (char *) rl_completer_word_break_characters;
}

char *
gdb_completion_word_break_characters ()
{
  /* New completion starting.  */
  current_completion.aborted = false;

  try
    {
      return gdb_completion_word_break_characters_throw ();
    }
  catch (const gdb_exception &ex)
    {
      /* Set this to that gdb_rl_attempted_completion_function knows
	 to abort early.  */
      current_completion.aborted = true;
    }

  return NULL;
}

/* See completer.h.  */

const char *
completion_find_completion_word (completion_tracker &tracker, const char *text,
				 int *quote_char)
{
  size_t point = strlen (text);

  complete_line_internal (tracker, NULL, text, point, handle_brkchars);

  if (tracker.use_custom_word_point ())
    {
      gdb_assert (tracker.custom_word_point () > 0);
      *quote_char = tracker.quote_char ();
      return text + tracker.custom_word_point ();
    }

  gdb_rl_completion_word_info info;

  info.word_break_characters = rl_completer_word_break_characters;
  info.quote_characters = gdb_completer_quote_characters;
  info.basic_quote_characters = rl_basic_quote_characters;

  return gdb_rl_find_completion_word (&info, quote_char, NULL, text);
}

/* See completer.h.  */

void
completion_tracker::recompute_lcd_visitor (completion_hash_entry *entry)
{
  if (!m_lowest_common_denominator_valid)
    {
      /* This is the first lowest common denominator that we are
	 considering, just copy it in.  */
      strcpy (m_lowest_common_denominator, entry->get_lcd ());
      m_lowest_common_denominator_unique = true;
      m_lowest_common_denominator_valid = true;
    }
  else
    {
      /* Find the common denominator between the currently-known lowest
	 common denominator and NEW_MATCH_UP.  That becomes the new lowest
	 common denominator.  */
      size_t i;
      const char *new_match = entry->get_lcd ();

      for (i = 0;
	   (new_match[i] != '\0'
	    && new_match[i] == m_lowest_common_denominator[i]);
	   i++)
	;
      if (m_lowest_common_denominator[i] != new_match[i])
	{
	  m_lowest_common_denominator[i] = '\0';
	  m_lowest_common_denominator_unique = false;
	}
    }
}

/* See completer.h.  */

void
completion_tracker::recompute_lowest_common_denominator ()
{
  /* We've already done this.  */
  if (m_lowest_common_denominator_valid)
    return;

  /* Resize the storage to ensure we have enough space, the plus one gives
     us space for the trailing null terminator we will include.  */
  m_lowest_common_denominator
    = (char *) xrealloc (m_lowest_common_denominator,
			 m_lowest_common_denominator_max_length + 1);

  /* Callback used to visit each entry in the m_entries_hash.  */
  auto visitor_func
    = [] (void **slot, void *info) -> int
      {
	completion_tracker *obj = (completion_tracker *) info;
	completion_hash_entry *entry = (completion_hash_entry *) *slot;
	obj->recompute_lcd_visitor (entry);
	return 1;
      };

  htab_traverse (m_entries_hash.get (), visitor_func, this);
  m_lowest_common_denominator_valid = true;
}

/* See completer.h.  */

void
completion_tracker::advance_custom_word_point_by (int len)
{
  m_custom_word_point += len;
}

/* Build a new C string that is a copy of LCD with the whitespace of
   ORIG/ORIG_LEN preserved.

   Say the user is completing a symbol name, with spaces, like:

     "foo ( i"

   and the resulting completion match is:

     "foo(int)"

   we want to end up with an input line like:

     "foo ( int)"
      ^^^^^^^      => text from LCD [1], whitespace from ORIG preserved.
	     ^^	   => new text from LCD

   [1] - We must take characters from the LCD instead of the original
   text, since some completions want to change upper/lowercase.  E.g.:

     "handle sig<>"

   completes to:

     "handle SIG[QUIT|etc.]"
*/

static char *
expand_preserving_ws (const char *orig, size_t orig_len,
		      const char *lcd)
{
  const char *p_orig = orig;
  const char *orig_end = orig + orig_len;
  const char *p_lcd = lcd;
  std::string res;

  while (p_orig < orig_end)
    {
      if (*p_orig == ' ')
	{
	  while (p_orig < orig_end && *p_orig == ' ')
	    res += *p_orig++;
	  p_lcd = skip_spaces (p_lcd);
	}
      else
	{
	  /* Take characters from the LCD instead of the original
	     text, since some completions change upper/lowercase.
	     E.g.:
	       "handle sig<>"
	     completes to:
	       "handle SIG[QUIT|etc.]"
	  */
	  res += *p_lcd;
	  p_orig++;
	  p_lcd++;
	}
    }

  while (*p_lcd != '\0')
    res += *p_lcd++;

  return xstrdup (res.c_str ());
}

/* See completer.h.  */

completion_result
completion_tracker::build_completion_result (const char *text,
					     int start, int end)
{
  size_t element_count = htab_elements (m_entries_hash.get ());

  if (element_count == 0)
    return {};

  /* +1 for the LCD, and +1 for NULL termination.  */
  char **match_list = XNEWVEC (char *, 1 + element_count + 1);

  /* Build replacement word, based on the LCD.  */

  recompute_lowest_common_denominator ();
  match_list[0]
    = expand_preserving_ws (text, end - start,
			    m_lowest_common_denominator);

  if (m_lowest_common_denominator_unique)
    {
      /* We don't rely on readline appending the quote char as
	 delimiter as then readline wouldn't append the ' ' after the
	 completion.  */
      char buf[2] = { (char) quote_char () };

      match_list[0] = reconcat (match_list[0], match_list[0],
				buf, (char *) NULL);
      match_list[1] = NULL;

      /* If the tracker wants to, or we already have a space at the
	 end of the match, tell readline to skip appending
	 another.  */
      char *match = match_list[0];
      bool completion_suppress_append
	= (suppress_append_ws ()
	   || (match[0] != '\0'
	       && match[strlen (match) - 1] == ' '));

      return completion_result (match_list, 1, completion_suppress_append);
    }
  else
    {
      /* State object used while building the completion list.  */
      struct list_builder
      {
	list_builder (char **ml)
	  : match_list (ml),
	    index (1)
	{ /* Nothing.  */ }

	/* The list we are filling.  */
	char **match_list;

	/* The next index in the list to write to.  */
	int index;
      };
      list_builder builder (match_list);

      /* Visit each entry in m_entries_hash and add it to the completion
	 list, updating the builder state object.  */
      auto func
	= [] (void **slot, void *info) -> int
	  {
	    completion_hash_entry *entry = (completion_hash_entry *) *slot;
	    list_builder *state = (list_builder *) info;

	    state->match_list[state->index] = entry->release_name ();
	    state->index++;
	    return 1;
	  };

      /* Build the completion list and add a null at the end.  */
      htab_traverse_noresize (m_entries_hash.get (), func, &builder);
      match_list[builder.index] = NULL;

      return completion_result (match_list, builder.index - 1, false);
    }
}

/* See completer.h  */

completion_result::completion_result ()
  : match_list (NULL), number_matches (0),
    completion_suppress_append (false)
{}

/* See completer.h  */

completion_result::completion_result (char **match_list_,
				      size_t number_matches_,
				      bool completion_suppress_append_)
  : match_list (match_list_),
    number_matches (number_matches_),
    completion_suppress_append (completion_suppress_append_)
{}

/* See completer.h  */

completion_result::~completion_result ()
{
  reset_match_list ();
}

/* See completer.h  */

completion_result::completion_result (completion_result &&rhs) noexcept
  : match_list (rhs.match_list),
    number_matches (rhs.number_matches)
{
  rhs.match_list = NULL;
  rhs.number_matches = 0;
}

/* See completer.h  */

char **
completion_result::release_match_list ()
{
  char **ret = match_list;
  match_list = NULL;
  return ret;
}

/* See completer.h  */

void
completion_result::sort_match_list ()
{
  if (number_matches > 1)
    {
      /* Element 0 is special (it's the common prefix), leave it
	 be.  */
      std::sort (&match_list[1],
		 &match_list[number_matches + 1],
		 compare_cstrings);
    }
}

/* See completer.h  */

void
completion_result::reset_match_list ()
{
  if (match_list != NULL)
    {
      for (char **p = match_list; *p != NULL; p++)
	xfree (*p);
      xfree (match_list);
      match_list = NULL;
    }
}

/* Helper for gdb_rl_attempted_completion_function, which does most of
   the work.  This is called by readline to build the match list array
   and to determine the lowest common denominator.  The real matches
   list starts at match[1], while match[0] is the slot holding
   readline's idea of the lowest common denominator of all matches,
   which is what readline replaces the completion "word" with.

   TEXT is the caller's idea of the "word" we are looking at, as
   computed in the handle_brkchars phase.

   START is the offset from RL_LINE_BUFFER where TEXT starts.  END is
   the offset from RL_LINE_BUFFER where TEXT ends (i.e., where
   rl_point is).

   You should thus pretend that the line ends at END (relative to
   RL_LINE_BUFFER).

   RL_LINE_BUFFER contains the entire text of the line.  RL_POINT is
   the offset in that line of the cursor.  You should pretend that the
   line ends at POINT.

   Returns NULL if there are no completions.  */

static char **
gdb_rl_attempted_completion_function_throw (const char *text, int start, int end)
{
  /* Completers that provide a custom word point in the
     handle_brkchars phase also compute their completions then.
     Completers that leave the completion word handling to readline
     must be called twice.  If rl_point (i.e., END) is at column 0,
     then readline skips the handle_brkchars phase, and so we create a
     tracker now in that case too.  */
  if (end == 0 || !current_completion.tracker->use_custom_word_point ())
    {
      delete current_completion.tracker;
      current_completion.tracker = new completion_tracker (true);

      complete_line (*current_completion.tracker, text,
		     rl_line_buffer, rl_point);
    }

  completion_tracker &tracker = *current_completion.tracker;

  completion_result result
    = tracker.build_completion_result (text, start, end);

  rl_completion_suppress_append = result.completion_suppress_append;
  return result.release_match_list ();
}

/* Function installed as "rl_attempted_completion_function" readline
   hook.  Wrapper around gdb_rl_attempted_completion_function_throw
   that catches C++ exceptions, which can't cross readline.  */

char **
gdb_rl_attempted_completion_function (const char *text, int start, int end)
{
  /* Restore globals that might have been tweaked in
     gdb_completion_word_break_characters.  */
  rl_basic_quote_characters = gdb_org_rl_basic_quote_characters;

  /* If we end up returning NULL, either on error, or simple because
     there are no matches, inhibit readline's default filename
     completer.  */
  rl_attempted_completion_over = 1;

  /* If the handle_brkchars phase was aborted, don't try
     completing.  */
  if (current_completion.aborted)
    return NULL;

  try
    {
      return gdb_rl_attempted_completion_function_throw (text, start, end);
    }
  catch (const gdb_exception &ex)
    {
    }

  return NULL;
}

/* Skip over the possibly quoted word STR (as defined by the quote
   characters QUOTECHARS and the word break characters BREAKCHARS).
   Returns pointer to the location after the "word".  If either
   QUOTECHARS or BREAKCHARS is NULL, use the same values used by the
   completer.  */

const char *
skip_quoted_chars (const char *str, const char *quotechars,
		   const char *breakchars)
{
  char quote_char = '\0';
  const char *scan;

  if (quotechars == NULL)
    quotechars = gdb_completer_quote_characters;

  if (breakchars == NULL)
    breakchars = current_language->word_break_characters ();

  for (scan = str; *scan != '\0'; scan++)
    {
      if (quote_char != '\0')
	{
	  /* Ignore everything until the matching close quote char.  */
	  if (*scan == quote_char)
	    {
	      /* Found matching close quote.  */
	      scan++;
	      break;
	    }
	}
      else if (strchr (quotechars, *scan))
	{
	  /* Found start of a quoted string.  */
	  quote_char = *scan;
	}
      else if (strchr (breakchars, *scan))
	{
	  break;
	}
    }

  return (scan);
}

/* Skip over the possibly quoted word STR (as defined by the quote
   characters and word break characters used by the completer).
   Returns pointer to the location after the "word".  */

const char *
skip_quoted (const char *str)
{
  return skip_quoted_chars (str, NULL, NULL);
}

/* Return a message indicating that the maximum number of completions
   has been reached and that there may be more.  */

const char *
get_max_completions_reached_message (void)
{
  return _("*** List may be truncated, max-completions reached. ***");
}

/* GDB replacement for rl_display_match_list.
   Readline doesn't provide a clean interface for TUI(curses).
   A hack previously used was to send readline's rl_outstream through a pipe
   and read it from the event loop.  Bleah.  IWBN if readline abstracted
   away all the necessary bits, and this is what this code does.  It
   replicates the parts of readline we need and then adds an abstraction
   layer, currently implemented as struct match_list_displayer, so that both
   CLI and TUI can use it.  We copy all this readline code to minimize
   GDB-specific mods to readline.  Once this code performs as desired then
   we can submit it to the readline maintainers.

   N.B. A lot of the code is the way it is in order to minimize differences
   from readline's copy.  */

/* Not supported here.  */
#undef VISIBLE_STATS

#if defined (HANDLE_MULTIBYTE)
#define MB_INVALIDCH(x) ((x) == (size_t)-1 || (x) == (size_t)-2)
#define MB_NULLWCH(x)   ((x) == 0)
#endif

#define ELLIPSIS_LEN	3

/* gdb version of readline/complete.c:get_y_or_n.
   'y' -> returns 1, and 'n' -> returns 0.
   Also supported: space == 'y', RUBOUT == 'n', ctrl-g == start over.
   If FOR_PAGER is non-zero, then also supported are:
   NEWLINE or RETURN -> returns 2, and 'q' -> returns 0.  */

static int
gdb_get_y_or_n (int for_pager, const struct match_list_displayer *displayer)
{
  int c;

  for (;;)
    {
      RL_SETSTATE (RL_STATE_MOREINPUT);
      c = displayer->read_key (displayer);
      RL_UNSETSTATE (RL_STATE_MOREINPUT);

      if (c == 'y' || c == 'Y' || c == ' ')
	return 1;
      if (c == 'n' || c == 'N' || c == RUBOUT)
	return 0;
      if (c == ABORT_CHAR || c < 0)
	{
	  /* Readline doesn't erase_entire_line here, but without it the
	     --More-- prompt isn't erased and neither is the text entered
	     thus far redisplayed.  */
	  displayer->erase_entire_line (displayer);
	  /* Note: The arguments to rl_abort are ignored.  */
	  rl_abort (0, 0);
	}
      if (for_pager && (c == NEWLINE || c == RETURN))
	return 2;
      if (for_pager && (c == 'q' || c == 'Q'))
	return 0;
      displayer->beep (displayer);
    }
}

/* Pager function for tab-completion.
   This is based on readline/complete.c:_rl_internal_pager.
   LINES is the number of lines of output displayed thus far.
   Returns:
   -1 -> user pressed 'n' or equivalent,
   0 -> user pressed 'y' or equivalent,
   N -> user pressed NEWLINE or equivalent and N is LINES - 1.  */

static int
gdb_display_match_list_pager (int lines,
			      const struct match_list_displayer *displayer)
{
  int i;

  displayer->puts (displayer, "--More--");
  displayer->flush (displayer);
  i = gdb_get_y_or_n (1, displayer);
  displayer->erase_entire_line (displayer);
  if (i == 0)
    return -1;
  else if (i == 2)
    return (lines - 1);
  else
    return 0;
}

/* Return non-zero if FILENAME is a directory.
   Based on readline/complete.c:path_isdir.  */

static int
gdb_path_isdir (const char *filename)
{
  struct stat finfo;

  return (stat (filename, &finfo) == 0 && S_ISDIR (finfo.st_mode));
}

/* Return the portion of PATHNAME that should be output when listing
   possible completions.  If we are hacking filename completion, we
   are only interested in the basename, the portion following the
   final slash.  Otherwise, we return what we were passed.  Since
   printing empty strings is not very informative, if we're doing
   filename completion, and the basename is the empty string, we look
   for the previous slash and return the portion following that.  If
   there's no previous slash, we just return what we were passed.

   Based on readline/complete.c:printable_part.  */

static char *
gdb_printable_part (char *pathname)
{
  char *temp, *x;

  if (rl_filename_completion_desired == 0)	/* don't need to do anything */
    return (pathname);

  temp = strrchr (pathname, '/');
#if defined (__MSDOS__)
  if (temp == 0 && ISALPHA ((unsigned char)pathname[0]) && pathname[1] == ':')
    temp = pathname + 1;
#endif

  if (temp == 0 || *temp == '\0')
    return (pathname);
  /* If the basename is NULL, we might have a pathname like '/usr/src/'.
     Look for a previous slash and, if one is found, return the portion
     following that slash.  If there's no previous slash, just return the
     pathname we were passed. */
  else if (temp[1] == '\0')
    {
      for (x = temp - 1; x > pathname; x--)
	if (*x == '/')
	  break;
      return ((*x == '/') ? x + 1 : pathname);
    }
  else
    return ++temp;
}

/* Compute width of STRING when displayed on screen by print_filename.
   Based on readline/complete.c:fnwidth.  */

static int
gdb_fnwidth (const char *string)
{
  int width, pos;
#if defined (HANDLE_MULTIBYTE)
  mbstate_t ps;
  int left, w;
  size_t clen;
  wchar_t wc;

  left = strlen (string) + 1;
  memset (&ps, 0, sizeof (mbstate_t));
#endif

  width = pos = 0;
  while (string[pos])
    {
      if (CTRL_CHAR (string[pos]) || string[pos] == RUBOUT)
	{
	  width += 2;
	  pos++;
	}
      else
	{
#if defined (HANDLE_MULTIBYTE)
	  clen = mbrtowc (&wc, string + pos, left - pos, &ps);
	  if (MB_INVALIDCH (clen))
	    {
	      width++;
	      pos++;
	      memset (&ps, 0, sizeof (mbstate_t));
	    }
	  else if (MB_NULLWCH (clen))
	    break;
	  else
	    {
	      pos += clen;
	      w = wcwidth (wc);
	      width += (w >= 0) ? w : 1;
	    }
#else
	  width++;
	  pos++;
#endif
	}
    }

  return width;
}

/* Print TO_PRINT, one matching completion.
   PREFIX_BYTES is number of common prefix bytes.
   Based on readline/complete.c:fnprint.  */

static int
gdb_fnprint (const char *to_print, int prefix_bytes,
	     const struct match_list_displayer *displayer)
{
  int printed_len, w;
  const char *s;
#if defined (HANDLE_MULTIBYTE)
  mbstate_t ps;
  const char *end;
  size_t tlen;
  int width;
  wchar_t wc;

  end = to_print + strlen (to_print) + 1;
  memset (&ps, 0, sizeof (mbstate_t));
#endif

  printed_len = 0;

  /* Don't print only the ellipsis if the common prefix is one of the
     possible completions */
  if (to_print[prefix_bytes] == '\0')
    prefix_bytes = 0;

  if (prefix_bytes)
    {
      char ellipsis;

      ellipsis = (to_print[prefix_bytes] == '.') ? '_' : '.';
      for (w = 0; w < ELLIPSIS_LEN; w++)
	displayer->putch (displayer, ellipsis);
      printed_len = ELLIPSIS_LEN;
    }

  s = to_print + prefix_bytes;
  while (*s)
    {
      if (CTRL_CHAR (*s))
	{
	  displayer->putch (displayer, '^');
	  displayer->putch (displayer, UNCTRL (*s));
	  printed_len += 2;
	  s++;
#if defined (HANDLE_MULTIBYTE)
	  memset (&ps, 0, sizeof (mbstate_t));
#endif
	}
      else if (*s == RUBOUT)
	{
	  displayer->putch (displayer, '^');
	  displayer->putch (displayer, '?');
	  printed_len += 2;
	  s++;
#if defined (HANDLE_MULTIBYTE)
	  memset (&ps, 0, sizeof (mbstate_t));
#endif
	}
      else
	{
#if defined (HANDLE_MULTIBYTE)
	  tlen = mbrtowc (&wc, s, end - s, &ps);
	  if (MB_INVALIDCH (tlen))
	    {
	      tlen = 1;
	      width = 1;
	      memset (&ps, 0, sizeof (mbstate_t));
	    }
	  else if (MB_NULLWCH (tlen))
	    break;
	  else
	    {
	      w = wcwidth (wc);
	      width = (w >= 0) ? w : 1;
	    }
	  for (w = 0; w < tlen; ++w)
	    displayer->putch (displayer, s[w]);
	  s += tlen;
	  printed_len += width;
#else
	  displayer->putch (displayer, *s);
	  s++;
	  printed_len++;
#endif
	}
    }

  return printed_len;
}

/* Output TO_PRINT to rl_outstream.  If VISIBLE_STATS is defined and we
   are using it, check for and output a single character for `special'
   filenames.  Return the number of characters we output.
   Based on readline/complete.c:print_filename.  */

static int
gdb_print_filename (char *to_print, char *full_pathname, int prefix_bytes,
		    const struct match_list_displayer *displayer)
{
  int printed_len, extension_char, slen, tlen;
  char *s, c, *new_full_pathname;
  const char *dn;
  extern int _rl_complete_mark_directories;

  extension_char = 0;
  printed_len = gdb_fnprint (to_print, prefix_bytes, displayer);

#if defined (VISIBLE_STATS)
 if (rl_filename_completion_desired && (rl_visible_stats || _rl_complete_mark_directories))
#else
 if (rl_filename_completion_desired && _rl_complete_mark_directories)
#endif
    {
      /* If to_print != full_pathname, to_print is the basename of the
	 path passed.  In this case, we try to expand the directory
	 name before checking for the stat character. */
      if (to_print != full_pathname)
	{
	  /* Terminate the directory name. */
	  c = to_print[-1];
	  to_print[-1] = '\0';

	  /* If setting the last slash in full_pathname to a NUL results in
	     full_pathname being the empty string, we are trying to complete
	     files in the root directory.  If we pass a null string to the
	     bash directory completion hook, for example, it will expand it
	     to the current directory.  We just want the `/'. */
	  if (full_pathname == 0 || *full_pathname == 0)
	    dn = "/";
	  else if (full_pathname[0] != '/')
	    dn = full_pathname;
	  else if (full_pathname[1] == 0)
	    dn = "//";		/* restore trailing slash to `//' */
	  else if (full_pathname[1] == '/' && full_pathname[2] == 0)
	    dn = "/";		/* don't turn /// into // */
	  else
	    dn = full_pathname;
	  s = tilde_expand (dn);
	  if (rl_directory_completion_hook)
	    (*rl_directory_completion_hook) (&s);

	  slen = strlen (s);
	  tlen = strlen (to_print);
	  new_full_pathname = (char *)xmalloc (slen + tlen + 2);
	  strcpy (new_full_pathname, s);
	  if (s[slen - 1] == '/')
	    slen--;
	  else
	    new_full_pathname[slen] = '/';
	  new_full_pathname[slen] = '/';
	  strcpy (new_full_pathname + slen + 1, to_print);

#if defined (VISIBLE_STATS)
	  if (rl_visible_stats)
	    extension_char = stat_char (new_full_pathname);
	  else
#endif
	  if (gdb_path_isdir (new_full_pathname))
	    extension_char = '/';

	  xfree (new_full_pathname);
	  to_print[-1] = c;
	}
      else
	{
	  s = tilde_expand (full_pathname);
#if defined (VISIBLE_STATS)
	  if (rl_visible_stats)
	    extension_char = stat_char (s);
	  else
#endif
	    if (gdb_path_isdir (s))
	      extension_char = '/';
	}

      xfree (s);
      if (extension_char)
	{
	  displayer->putch (displayer, extension_char);
	  printed_len++;
	}
    }

  return printed_len;
}

/* GDB version of readline/complete.c:complete_get_screenwidth.  */

static int
gdb_complete_get_screenwidth (const struct match_list_displayer *displayer)
{
  /* Readline has other stuff here which it's not clear we need.  */
  return displayer->width;
}

extern int _rl_completion_prefix_display_length;
extern int _rl_print_completions_horizontally;

extern "C" int _rl_qsort_string_compare (const void *, const void *);
typedef int QSFUNC (const void *, const void *);

/* GDB version of readline/complete.c:rl_display_match_list.
   See gdb_display_match_list for a description of MATCHES, LEN, MAX.
   Returns non-zero if all matches are displayed.  */

static int
gdb_display_match_list_1 (char **matches, int len, int max,
			  const struct match_list_displayer *displayer)
{
  int count, limit, printed_len, lines, cols;
  int i, j, k, l, common_length, sind;
  char *temp, *t;
  int page_completions = displayer->height != INT_MAX && pagination_enabled;

  /* Find the length of the prefix common to all items: length as displayed
     characters (common_length) and as a byte index into the matches (sind) */
  common_length = sind = 0;
  if (_rl_completion_prefix_display_length > 0)
    {
      t = gdb_printable_part (matches[0]);
      temp = strrchr (t, '/');
      common_length = temp ? gdb_fnwidth (temp) : gdb_fnwidth (t);
      sind = temp ? strlen (temp) : strlen (t);

      if (common_length > _rl_completion_prefix_display_length && common_length > ELLIPSIS_LEN)
	max -= common_length - ELLIPSIS_LEN;
      else
	common_length = sind = 0;
    }

  /* How many items of MAX length can we fit in the screen window? */
  cols = gdb_complete_get_screenwidth (displayer);
  max += 2;
  limit = cols / max;
  if (limit != 1 && (limit * max == cols))
    limit--;

  /* If cols == 0, limit will end up -1 */
  if (cols < displayer->width && limit < 0)
    limit = 1;

  /* Avoid a possible floating exception.  If max > cols,
     limit will be 0 and a divide-by-zero fault will result. */
  if (limit == 0)
    limit = 1;

  /* How many iterations of the printing loop? */
  count = (len + (limit - 1)) / limit;

  /* Watch out for special case.  If LEN is less than LIMIT, then
     just do the inner printing loop.
	   0 < len <= limit  implies  count = 1. */

  /* Sort the items if they are not already sorted. */
  if (rl_ignore_completion_duplicates == 0 && rl_sort_completion_matches)
    qsort (matches + 1, len, sizeof (char *), (QSFUNC *)_rl_qsort_string_compare);

  displayer->crlf (displayer);

  lines = 0;
  if (_rl_print_completions_horizontally == 0)
    {
      /* Print the sorted items, up-and-down alphabetically, like ls. */
      for (i = 1; i <= count; i++)
	{
	  for (j = 0, l = i; j < limit; j++)
	    {
	      if (l > len || matches[l] == 0)
		break;
	      else
		{
		  temp = gdb_printable_part (matches[l]);
		  printed_len = gdb_print_filename (temp, matches[l], sind,
						    displayer);

		  if (j + 1 < limit)
		    for (k = 0; k < max - printed_len; k++)
		      displayer->putch (displayer, ' ');
		}
	      l += count;
	    }
	  displayer->crlf (displayer);
	  lines++;
	  if (page_completions && lines >= (displayer->height - 1) && i < count)
	    {
	      lines = gdb_display_match_list_pager (lines, displayer);
	      if (lines < 0)
		return 0;
	    }
	}
    }
  else
    {
      /* Print the sorted items, across alphabetically, like ls -x. */
      for (i = 1; matches[i]; i++)
	{
	  temp = gdb_printable_part (matches[i]);
	  printed_len = gdb_print_filename (temp, matches[i], sind, displayer);
	  /* Have we reached the end of this line? */
	  if (matches[i+1])
	    {
	      if (i && (limit > 1) && (i % limit) == 0)
		{
		  displayer->crlf (displayer);
		  lines++;
		  if (page_completions && lines >= displayer->height - 1)
		    {
		      lines = gdb_display_match_list_pager (lines, displayer);
		      if (lines < 0)
			return 0;
		    }
		}
	      else
		for (k = 0; k < max - printed_len; k++)
		  displayer->putch (displayer, ' ');
	    }
	}
      displayer->crlf (displayer);
    }

  return 1;
}

/* Utility for displaying completion list matches, used by both CLI and TUI.

   MATCHES is the list of strings, in argv format, LEN is the number of
   strings in MATCHES, and MAX is the length of the longest string in
   MATCHES.  */

void
gdb_display_match_list (char **matches, int len, int max,
			const struct match_list_displayer *displayer)
{
  /* Readline will never call this if complete_line returned NULL.  */
  gdb_assert (max_completions != 0);

  /* complete_line will never return more than this.  */
  if (max_completions > 0)
    gdb_assert (len <= max_completions);

  if (rl_completion_query_items > 0 && len >= rl_completion_query_items)
    {
      char msg[100];

      /* We can't use *query here because they wait for <RET> which is
	 wrong here.  This follows the readline version as closely as possible
	 for compatibility's sake.  See readline/complete.c.  */

      displayer->crlf (displayer);

      xsnprintf (msg, sizeof (msg),
		 "Display all %d possibilities? (y or n)", len);
      displayer->puts (displayer, msg);
      displayer->flush (displayer);

      if (gdb_get_y_or_n (0, displayer) == 0)
	{
	  displayer->crlf (displayer);
	  return;
	}
    }

  if (gdb_display_match_list_1 (matches, len, max, displayer))
    {
      /* Note: MAX_COMPLETIONS may be -1 or zero, but LEN is always > 0.  */
      if (len == max_completions)
	{
	  /* The maximum number of completions has been reached.  Warn the user
	     that there may be more.  */
	  const char *message = get_max_completions_reached_message ();

	  displayer->puts (displayer, message);
	  displayer->crlf (displayer);
	}
    }
}

/* See completer.h.  */

bool
skip_over_slash_fmt (completion_tracker &tracker, const char **args)
{
  const char *text = *args;

  if (text[0] == '/')
    {
      bool in_fmt;
      tracker.set_use_custom_word_point (true);

      if (text[1] == '\0')
	{
	  /* The user tried to complete after typing just the '/' character
	     of the /FMT string.  Step the completer past the '/', but we
	     don't offer any completions.  */
	  in_fmt = true;
	  ++text;
	}
      else
	{
	  /* The user has typed some characters after the '/', we assume
	     this is a complete /FMT string, first skip over it.  */
	  text = skip_to_space (text);

	  if (*text == '\0')
	    {
	      /* We're at the end of the input string.  The user has typed
		 '/FMT' and asked for a completion.  Push an empty
		 completion string, this will cause readline to insert a
		 space so the user now has '/FMT '.  */
	      in_fmt = true;
	      tracker.add_completion (make_unique_xstrdup (text));
	    }
	  else
	    {
	      /* The user has already typed things after the /FMT, skip the
		 whitespace and return false.  Whoever called this function
		 should then try to complete what comes next.  */
	      in_fmt = false;
	      text = skip_spaces (text);
	    }
	}

      tracker.advance_custom_word_point_by (text - *args);
      *args = text;
      return in_fmt;
    }

  return false;
}

void _initialize_completer ();
void
_initialize_completer ()
{
  add_setshow_zuinteger_unlimited_cmd ("max-completions", no_class,
				       &max_completions, _("\
Set maximum number of completion candidates."), _("\
Show maximum number of completion candidates."), _("\
Use this to limit the number of candidates considered\n\
during completion.  Specifying \"unlimited\" or -1\n\
disables limiting.  Note that setting either no limit or\n\
a very large limit can make completion slow."),
				       NULL, NULL, &setlist, &showlist);
}
