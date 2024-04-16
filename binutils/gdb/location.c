/* Data structures and API for location specs in GDB.
   Copyright (C) 2013-2024 Free Software Foundation, Inc.

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
#include "gdbsupport/gdb_assert.h"
#include "gdbsupport/gdb-checked-static-cast.h"
#include "location.h"
#include "symtab.h"
#include "language.h"
#include "linespec.h"
#include "cli/cli-utils.h"
#include "probe.h"
#include "cp-support.h"

#include <ctype.h>
#include <string.h>

static std::string
  explicit_to_string_internal (bool as_linespec,
			       const explicit_location_spec *explicit_loc);

/* Return a xstrdup of STR if not NULL, otherwise return NULL.  */

static char *
maybe_xstrdup (const char *str)
{
  return (str != nullptr ? xstrdup (str) : nullptr);
}

probe_location_spec::probe_location_spec (std::string &&probe)
  : location_spec (PROBE_LOCATION_SPEC, std::move (probe))
{
}

location_spec_up
probe_location_spec::clone () const
{
  return location_spec_up (new probe_location_spec (*this));
}

bool
probe_location_spec::empty_p () const
{
  return false;
}

std::string probe_location_spec::compute_string () const
{
  return std::move (m_as_string);
}

/* A "normal" linespec.  */
linespec_location_spec::linespec_location_spec
  (const char **linespec, symbol_name_match_type match_type_)
  : location_spec (LINESPEC_LOCATION_SPEC),
    match_type (match_type_)
{
  if (*linespec != NULL)
    {
      const char *p;
      const char *orig = *linespec;

      linespec_lex_to_end (linespec);
      p = remove_trailing_whitespace (orig, *linespec);

      /* If there is no valid linespec then this will leave the
	 spec_string as nullptr.  This behaviour is relied on in the
	 breakpoint setting code, where spec_string being nullptr means
	 to use the default breakpoint location.  */
      if ((p - orig) > 0)
	spec_string.reset (savestring (orig, p - orig));
    }
}

location_spec_up
linespec_location_spec::clone () const
{
  return location_spec_up (new linespec_location_spec (*this));
}

bool
linespec_location_spec::empty_p () const
{
  return false;
}

linespec_location_spec::linespec_location_spec
  (const linespec_location_spec &other)
  : location_spec (other),
    match_type (other.match_type),
    spec_string (maybe_xstrdup (other.spec_string.get ()))
{
}

std::string
linespec_location_spec::compute_string () const
{
  if (spec_string != nullptr)
    {
      if (match_type == symbol_name_match_type::FULL)
	return std::string ("-qualified ") + spec_string.get ();
      else
	return spec_string.get ();
    }
  return {};
}

address_location_spec::address_location_spec (CORE_ADDR addr,
					      const char *addr_string,
					      int addr_string_len)
  : location_spec (ADDRESS_LOCATION_SPEC),
    address (addr)
{
  if (addr_string != nullptr)
    m_as_string = std::string (addr_string, addr_string_len);
}

location_spec_up
address_location_spec::clone () const
{
  return location_spec_up (new address_location_spec (*this));
}

bool
address_location_spec::empty_p () const
{
  return false;
}

address_location_spec::address_location_spec
  (const address_location_spec &other)
  : location_spec (other),
    address (other.address)
{
}

std::string
address_location_spec::compute_string () const
{
  const char *addr_string = core_addr_to_string (address);
  return std::string ("*") + addr_string;
}

explicit_location_spec::explicit_location_spec (const char *function_name)
  : location_spec (EXPLICIT_LOCATION_SPEC),
    function_name (maybe_xstrdup (function_name))
{
}

explicit_location_spec::explicit_location_spec
  (const explicit_location_spec &other)
  : location_spec (other),
    source_filename (maybe_xstrdup (other.source_filename.get ())),
    function_name (maybe_xstrdup (other.function_name.get ())),
    func_name_match_type (other.func_name_match_type),
    label_name (maybe_xstrdup (other.label_name.get ())),
    line_offset (other.line_offset)
{
}

location_spec_up
explicit_location_spec::clone () const
{
  return location_spec_up (new explicit_location_spec (*this));
}

bool
explicit_location_spec::empty_p () const
{
  return (source_filename == nullptr
	  && function_name == nullptr
	  && label_name == nullptr
	  && line_offset.sign == LINE_OFFSET_UNKNOWN);
}

std::string
explicit_location_spec::compute_string () const
{
  return explicit_to_string_internal (false, this);
}

/* See description in location.h.  */

location_spec_up
new_linespec_location_spec (const char **linespec,
			    symbol_name_match_type match_type)
{
  return location_spec_up (new linespec_location_spec (linespec,
						       match_type));
}

/* See description in location.h.  */

const linespec_location_spec *
as_linespec_location_spec (const location_spec *locspec)
{
  gdb_assert (locspec->type () == LINESPEC_LOCATION_SPEC);
  return gdb::checked_static_cast<const linespec_location_spec *> (locspec);
}

/* See description in location.h.  */

location_spec_up
new_address_location_spec (CORE_ADDR addr, const char *addr_string,
			   int addr_string_len)
{
  return location_spec_up (new address_location_spec (addr, addr_string,
						      addr_string_len));
}

/* See description in location.h.  */

const address_location_spec *
as_address_location_spec (const location_spec *locspec)
{
  gdb_assert (locspec->type () == ADDRESS_LOCATION_SPEC);
  return gdb::checked_static_cast<const address_location_spec *> (locspec);
}

/* See description in location.h.  */

location_spec_up
new_probe_location_spec (std::string &&probe)
{
  return location_spec_up (new probe_location_spec (std::move (probe)));
}

/* See description in location.h.  */

const probe_location_spec *
as_probe_location_spec (const location_spec *locspec)
{
  gdb_assert (locspec->type () == PROBE_LOCATION_SPEC);
  return gdb::checked_static_cast<const probe_location_spec *> (locspec);
}

/* See description in location.h.  */

const explicit_location_spec *
as_explicit_location_spec (const location_spec *locspec)
{
  gdb_assert (locspec->type () == EXPLICIT_LOCATION_SPEC);
  return gdb::checked_static_cast<const explicit_location_spec *> (locspec);
}

/* See description in location.h.  */

explicit_location_spec *
as_explicit_location_spec (location_spec *locspec)
{
  gdb_assert (locspec->type () == EXPLICIT_LOCATION_SPEC);
  return gdb::checked_static_cast<explicit_location_spec *> (locspec);
}

/* Return a string representation of the explicit location spec in
   EXPLICIT_LOCSPEC.

   AS_LINESPEC is true if this string should be a linespec.  Otherwise
   it will be output in explicit form.  */

static std::string
explicit_to_string_internal (bool as_linespec,
			     const explicit_location_spec *explicit_loc)
{
  bool need_space = false;
  char space = as_linespec ? ':' : ' ';
  string_file buf;

  if (explicit_loc->source_filename != NULL)
    {
      if (!as_linespec)
	buf.puts ("-source ");
      buf.puts (explicit_loc->source_filename.get ());
      need_space = true;
    }

  if (explicit_loc->function_name != NULL)
    {
      if (need_space)
	buf.putc (space);
      if (explicit_loc->func_name_match_type == symbol_name_match_type::FULL)
	buf.puts ("-qualified ");
      if (!as_linespec)
	buf.puts ("-function ");
      buf.puts (explicit_loc->function_name.get ());
      need_space = true;
    }

  if (explicit_loc->label_name != NULL)
    {
      if (need_space)
	buf.putc (space);
      if (!as_linespec)
	buf.puts ("-label ");
      buf.puts (explicit_loc->label_name.get ());
      need_space = true;
    }

  if (explicit_loc->line_offset.sign != LINE_OFFSET_UNKNOWN)
    {
      if (need_space)
	buf.putc (space);
      if (!as_linespec)
	buf.puts ("-line ");
      buf.printf ("%s%d",
		  (explicit_loc->line_offset.sign == LINE_OFFSET_NONE ? ""
		   : (explicit_loc->line_offset.sign
		      == LINE_OFFSET_PLUS ? "+" : "-")),
		  explicit_loc->line_offset.offset);
    }

  return buf.release ();
}

/* See description in location.h.  */

std::string
explicit_location_spec::to_linespec () const
{
  return explicit_to_string_internal (true, this);
}

/* Find an instance of the quote character C in the string S that is
   outside of all single- and double-quoted strings (i.e., any quoting
   other than C).  */

static const char *
find_end_quote (const char *s, char end_quote_char)
{
  /* zero if we're not in quotes;
     '"' if we're in a double-quoted string;
     '\'' if we're in a single-quoted string.  */
  char nested_quote_char = '\0';

  for (const char *scan = s; *scan != '\0'; scan++)
    {
      if (nested_quote_char != '\0')
	{
	  if (*scan == nested_quote_char)
	    nested_quote_char = '\0';
	  else if (scan[0] == '\\' && *(scan + 1) != '\0')
	    scan++;
	}
      else if (*scan == end_quote_char && nested_quote_char == '\0')
	return scan;
      else if (*scan == '"' || *scan == '\'')
	nested_quote_char = *scan;
    }

  return 0;
}

/* A lexer for explicit location specs.  This function will advance
   INP past any strings that it lexes.  Returns a malloc'd copy of the
   lexed string or NULL if no lexing was done.  */

static gdb::unique_xmalloc_ptr<char>
explicit_location_spec_lex_one (const char **inp,
				const struct language_defn *language,
				explicit_completion_info *completion_info)
{
  const char *start = *inp;

  if (*start == '\0')
    return NULL;

  /* If quoted, skip to the ending quote.  */
  if (strchr (get_gdb_linespec_parser_quote_characters (), *start))
    {
      if (completion_info != NULL)
	completion_info->quoted_arg_start = start;

      const char *end = find_end_quote (start + 1, *start);

      if (end == NULL)
	{
	  if (completion_info == NULL)
	    error (_("Unmatched quote, %s."), start);

	  end = start + strlen (start);
	  *inp = end;
	  return gdb::unique_xmalloc_ptr<char> (savestring (start + 1,
							    *inp - start - 1));
	}

      if (completion_info != NULL)
	completion_info->quoted_arg_end = end;
      *inp = end + 1;
      return gdb::unique_xmalloc_ptr<char> (savestring (start + 1,
							*inp - start - 2));
    }

  /* If the input starts with '-' or '+', the string ends with the next
     whitespace or comma.  */
  if (*start == '-' || *start == '+')
    {
      while (*inp[0] != '\0' && *inp[0] != ',' && !isspace (*inp[0]))
	++(*inp);
    }
  else
    {
      /* Handle numbers first, stopping at the next whitespace or ','.  */
      while (isdigit (*inp[0]))
	++(*inp);
      if (*inp[0] == '\0' || isspace (*inp[0]) || *inp[0] == ',')
	return gdb::unique_xmalloc_ptr<char> (savestring (start,
							  *inp - start));

      /* Otherwise stop at the next occurrence of whitespace, '\0',
	 keyword, or ','.  */
      *inp = start;
      while ((*inp)[0]
	     && (*inp)[0] != ','
	     && !(isspace ((*inp)[0])
		  || linespec_lexer_lex_keyword (&(*inp)[1])))
	{
	  /* Special case: C++ operator,.  */
	  if (language->la_language == language_cplus
	      && startswith (*inp, CP_OPERATOR_STR))
	    (*inp) += CP_OPERATOR_LEN;
	  ++(*inp);
	}
    }

  if (*inp - start > 0)
    return gdb::unique_xmalloc_ptr<char> (savestring (start, *inp - start));

  return NULL;
}

/* Return true if COMMA points past "operator".  START is the start of
   the line that COMMAND points to, hence when reading backwards, we
   must not read any character before START.  */

static bool
is_cp_operator (const char *start, const char *comma)
{
  if (comma != NULL
      && (comma - start) >= CP_OPERATOR_LEN)
    {
      const char *p = comma;

      while (p > start && isspace (p[-1]))
	p--;
      if (p - start >= CP_OPERATOR_LEN)
	{
	  p -= CP_OPERATOR_LEN;
	  if (strncmp (p, CP_OPERATOR_STR, CP_OPERATOR_LEN) == 0
	      && (p == start
		  || !(isalnum (p[-1]) || p[-1] == '_')))
	    {
	      return true;
	    }
	}
    }
  return false;
}

/* When scanning the input string looking for the next explicit
   location spec option/delimiter, we jump to the next option by looking
   for ",", and "-".  Such a character can also appear in C++ symbols
   like "operator," and "operator-".  So when we find such a
   character, we call this function to check if we found such a
   symbol, meaning we had a false positive for an option string.  In
   that case, we keep looking for the next delimiter, until we find
   one that is not a false positive, or we reach end of string.  FOUND
   is the character that scanning found (either '-' or ','), and START
   is the start of the line that FOUND points to, hence when reading
   backwards, we must not read any character before START.  Returns a
   pointer to the next non-false-positive delimiter character, or NULL
   if none was found.  */

static const char *
skip_op_false_positives (const char *start, const char *found)
{
  while (found != NULL && is_cp_operator (start, found))
    {
      if (found[0] == '-' && found[1] == '-')
	start = found + 2;
      else
	start = found + 1;
      found = find_toplevel_char (start, *found);
    }

  return found;
}

/* Assuming both FIRST and NEW_TOK point into the same string, return
   the pointer that is closer to the start of the string.  If FIRST is
   NULL, returns NEW_TOK.  If NEW_TOK is NULL, returns FIRST.  */

static const char *
first_of (const char *first, const char *new_tok)
{
  if (first == NULL)
    return new_tok;
  else if (new_tok != NULL && new_tok < first)
    return new_tok;
  else
    return first;
}

/* A lexer for functions in explicit location specs.  This function will
   advance INP past a function until the next option, or until end of
   string.  Returns a malloc'd copy of the lexed string or NULL if no
   lexing was done.  */

static gdb::unique_xmalloc_ptr<char>
explicit_location_spec_lex_one_function
  (const char **inp,
   const struct language_defn *language,
   explicit_completion_info *completion_info)
{
  const char *start = *inp;

  if (*start == '\0')
    return NULL;

  /* If quoted, skip to the ending quote.  */
  if (strchr (get_gdb_linespec_parser_quote_characters (), *start))
    {
      char quote_char = *start;

      /* If the input is not an Ada operator, skip to the matching
	 closing quote and return the string.  */
      if (!(language->la_language == language_ada
	    && quote_char == '\"' && is_ada_operator (start)))
	{
	  if (completion_info != NULL)
	    completion_info->quoted_arg_start = start;

	  const char *end = find_toplevel_char (start + 1, quote_char);

	  if (end == NULL)
	    {
	      if (completion_info == NULL)
		error (_("Unmatched quote, %s."), start);

	      end = start + strlen (start);
	      *inp = end;
	      char *saved = savestring (start + 1, *inp - start - 1);
	      return gdb::unique_xmalloc_ptr<char> (saved);
	    }

	  if (completion_info != NULL)
	    completion_info->quoted_arg_end = end;
	  *inp = end + 1;
	  char *saved = savestring (start + 1, *inp - start - 2);
	  return gdb::unique_xmalloc_ptr<char> (saved);
	}
    }

  const char *comma = find_toplevel_char (start, ',');

  /* If we have "-function -myfunction", or perhaps better example,
     "-function -[BasicClass doIt]" (objc selector), treat
     "-myfunction" as the function name.  I.e., skip the first char if
     it is an hyphen.  Don't skip the first char always, because we
     may have C++ "operator<", and find_toplevel_char needs to see the
     'o' in that case.  */
  const char *hyphen
    = (*start == '-'
       ? find_toplevel_char (start + 1, '-')
       : find_toplevel_char (start, '-'));

  /* Check for C++ "operator," and "operator-".  */
  comma = skip_op_false_positives (start, comma);
  hyphen = skip_op_false_positives (start, hyphen);

  /* Pick the one that appears first.  */
  const char *end = first_of (hyphen, comma);

  /* See if a linespec keyword appears first.  */
  const char *s = start;
  const char *ws = find_toplevel_char (start, ' ');
  while (ws != NULL && linespec_lexer_lex_keyword (ws + 1) == NULL)
    {
      s = ws + 1;
      ws = find_toplevel_char (s, ' ');
    }
  if (ws != NULL)
    end = first_of (end, ws + 1);

  /* If we don't have any terminator, then take the whole string.  */
  if (end == NULL)
    end = start + strlen (start);

  /* Trim whitespace at the end.  */
  while (end > start && end[-1] == ' ')
    end--;

  *inp = end;

  if (*inp - start > 0)
    return gdb::unique_xmalloc_ptr<char> (savestring (start, *inp - start));

  return NULL;
}

/* See description in location.h.  */

location_spec_up
string_to_explicit_location_spec (const char **argp,
				  const struct language_defn *language,
				  explicit_completion_info *completion_info)
{
  /* It is assumed that input beginning with '-' and a non-digit
     character is an explicit location.  "-p" is reserved, though,
     for probe locations.  */
  if (argp == NULL
      || *argp == NULL
      || *argp[0] != '-'
      || !isalpha ((*argp)[1])
      || ((*argp)[0] == '-' && (*argp)[1] == 'p'))
    return NULL;

  std::unique_ptr<explicit_location_spec> locspec
    (new explicit_location_spec ());

  /* Process option/argument pairs.  dprintf_command
     requires that processing stop on ','.  */
  while ((*argp)[0] != '\0' && (*argp)[0] != ',')
    {
      int len;
      const char *start;

      /* Clear these on each iteration, since they should be filled
	 with info about the last option.  */
      if (completion_info != NULL)
	{
	  completion_info->quoted_arg_start = NULL;
	  completion_info->quoted_arg_end = NULL;
	}

      /* If *ARGP starts with a keyword, stop processing
	 options.  */
      if (linespec_lexer_lex_keyword (*argp) != NULL)
	break;

      /* Mark the start of the string in case we need to rewind.  */
      start = *argp;

      if (completion_info != NULL)
	completion_info->last_option = start;

      /* Get the option string.  */
      gdb::unique_xmalloc_ptr<char> opt
	= explicit_location_spec_lex_one (argp, language, NULL);

      /* Use the length of the option to allow abbreviations.  */
      len = strlen (opt.get ());

      /* Get the argument string.  */
      *argp = skip_spaces (*argp);

      /* All options have a required argument.  Checking for this
	 required argument is deferred until later.  */
      gdb::unique_xmalloc_ptr<char> oarg;
      /* True if we have an argument.  This is required because we'll
	 move from OARG before checking whether we have an
	 argument.  */
      bool have_oarg = false;

      /* True if the option needs an argument.  */
      bool need_oarg = false;

      /* Convenience to consistently set both OARG/HAVE_OARG from
	 ARG.  */
      auto set_oarg = [&] (gdb::unique_xmalloc_ptr<char> arg)
	{
	  if (completion_info != NULL)
	    {
	      /* We do this here because the set of options that take
		 arguments matches the set of explicit location
		 options.  */
	      completion_info->saw_explicit_location_spec_option = true;
	    }
	  oarg = std::move (arg);
	  have_oarg = oarg != NULL;
	  need_oarg = true;
	};

      if (strncmp (opt.get (), "-source", len) == 0)
	{
	  set_oarg (explicit_location_spec_lex_one (argp, language,
						    completion_info));
	  locspec->source_filename = std::move (oarg);
	}
      else if (strncmp (opt.get (), "-function", len) == 0)
	{
	  set_oarg (explicit_location_spec_lex_one_function (argp, language,
							     completion_info));
	  locspec->function_name = std::move (oarg);
	}
      else if (strncmp (opt.get (), "-qualified", len) == 0)
	{
	  locspec->func_name_match_type = symbol_name_match_type::FULL;
	}
      else if (strncmp (opt.get (), "-line", len) == 0)
	{
	  set_oarg (explicit_location_spec_lex_one (argp, language, NULL));
	  *argp = skip_spaces (*argp);
	  if (have_oarg)
	    {
	      locspec->line_offset = linespec_parse_line_offset (oarg.get ());
	      continue;
	    }
	}
      else if (strncmp (opt.get (), "-label", len) == 0)
	{
	  set_oarg (explicit_location_spec_lex_one (argp, language,
						    completion_info));
	  locspec->label_name = std::move (oarg);
	}
      /* Only emit an "invalid argument" error for options
	 that look like option strings.  */
      else if (opt.get ()[0] == '-' && !isdigit (opt.get ()[1]))
	{
	  if (completion_info == NULL)
	    error (_("invalid explicit location argument, \"%s\""), opt.get ());
	}
      else
	{
	  /* End of the explicit location specification.
	     Stop parsing and return whatever explicit location was
	     parsed.  */
	  *argp = start;
	  break;
	}

      *argp = skip_spaces (*argp);

      /* It's a little lame to error after the fact, but in this
	 case, it provides a much better user experience to issue
	 the "invalid argument" error before any missing
	 argument error.  */
      if (need_oarg && !have_oarg && completion_info == NULL)
	error (_("missing argument for \"%s\""), opt.get ());
    }

  /* One special error check:  If a source filename was given
     without offset, function, or label, issue an error.  */
  if (locspec->source_filename != NULL
      && locspec->function_name == NULL
      && locspec->label_name == NULL
      && (locspec->line_offset.sign == LINE_OFFSET_UNKNOWN)
      && completion_info == NULL)
    {
      error (_("Source filename requires function, label, or "
	       "line offset."));
    }

  return location_spec_up (locspec.release ());
}

/* See description in location.h.  */

location_spec_up
string_to_location_spec_basic (const char **stringp,
			       const struct language_defn *language,
			       symbol_name_match_type match_type)
{
  location_spec_up locspec;
  const char *cs;

  /* Try the input as a probe spec.  */
  cs = *stringp;
  if (cs != NULL && probe_linespec_to_static_ops (&cs) != NULL)
    {
      locspec = new_probe_location_spec (*stringp);
      *stringp += strlen (*stringp);
    }
  else
    {
      /* Try an address location spec.  */
      if (*stringp != NULL && **stringp == '*')
	{
	  const char *arg, *orig;
	  CORE_ADDR addr;

	  orig = arg = *stringp;
	  addr = linespec_expression_to_pc (&arg);
	  locspec = new_address_location_spec (addr, orig, arg - orig);
	  *stringp += arg - orig;
	}
      else
	{
	  /* Everything else is a linespec.  */
	  locspec = new_linespec_location_spec (stringp, match_type);
	}
    }

  return locspec;
}

/* See description in location.h.  */

location_spec_up
string_to_location_spec (const char **stringp,
			 const struct language_defn *language,
			 symbol_name_match_type match_type)
{
  const char *arg, *orig;

  /* Try an explicit location spec.  */
  orig = arg = *stringp;
  location_spec_up locspec
    = string_to_explicit_location_spec (&arg, language, NULL);
  if (locspec != nullptr)
    {
      /* It was a valid explicit location.  Advance STRINGP to
	 the end of input.  */
      *stringp += arg - orig;

      /* If the user really specified a location spec, then we're
	 done.  */
      if (!locspec->empty_p ())
	return locspec;

      /* Otherwise, the user _only_ specified optional flags like
	 "-qualified", otherwise string_to_explicit_location_spec
	 would have thrown an error.  Save the flags for "basic"
	 linespec parsing below and discard the explicit location
	 spec.  */
      explicit_location_spec *xloc
	= gdb::checked_static_cast<explicit_location_spec *> (locspec.get ());
      match_type = xloc->func_name_match_type;
    }

  /* Everything else is a "basic" linespec, address, or probe location
     spec.  */
  return string_to_location_spec_basic (stringp, language, match_type);
}
