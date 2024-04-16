/* Multiple source language support for GDB.

   Copyright (C) 1991-2024 Free Software Foundation, Inc.

   Contributed by the Department of Computer Science at the State University
   of New York at Buffalo.

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

/* This file contains functions that return things that are specific
   to languages.  Each function should examine current_language if necessary,
   and return the appropriate result.  */

/* FIXME:  Most of these would be better organized as macros which
   return data out of a "language-specific" struct pointer that is set
   whenever the working language changes.  That would be a lot faster.  */

#include "defs.h"
#include <ctype.h>
#include "symtab.h"
#include "gdbtypes.h"
#include "value.h"
#include "gdbcmd.h"
#include "expression.h"
#include "language.h"
#include "varobj.h"
#include "target.h"
#include "parser-defs.h"
#include "demangle.h"
#include "symfile.h"
#include "cp-support.h"
#include "frame.h"
#include "c-lang.h"
#include <algorithm>
#include "gdbarch.h"

static void set_range_case (void);

/* range_mode ==
   range_mode_auto:   range_check set automatically to default of language.
   range_mode_manual: range_check set manually by user.  */

enum range_mode
  {
    range_mode_auto, range_mode_manual
  };

/* case_mode ==
   case_mode_auto:   case_sensitivity set upon selection of scope.
   case_mode_manual: case_sensitivity set only by user.  */

enum case_mode
  {
    case_mode_auto, case_mode_manual
  };

/* The current (default at startup) state of type and range checking.
   (If the modes are set to "auto", though, these are changed based
   on the default language at startup, and then again based on the
   language of the first source file.  */

static enum range_mode range_mode = range_mode_auto;
enum range_check range_check = range_check_off;
static enum case_mode case_mode = case_mode_auto;
enum case_sensitivity case_sensitivity = case_sensitive_on;

/* The current language and language_mode (see language.h).  */

static const struct language_defn *global_current_language;
static lazily_set_language_ftype *lazy_language_setter;
enum language_mode language_mode = language_mode_auto;

/* See language.h.  */

const struct language_defn *
get_current_language ()
{
  if (lazy_language_setter != nullptr)
    {
      /* Avoid recursive calls -- set_language refers to
	 current_language.  */
      lazily_set_language_ftype *call = lazy_language_setter;
      lazy_language_setter = nullptr;
      call ();
    }
  return global_current_language;
}

void
lazily_set_language (lazily_set_language_ftype *fun)
{
  lazy_language_setter = fun;
}

scoped_restore_current_language::scoped_restore_current_language ()
  : m_lang (global_current_language),
    m_fun (lazy_language_setter)
{
}

scoped_restore_current_language::~scoped_restore_current_language ()
{
  /* If both are NULL, then that means dont_restore was called.  */
  if (m_lang != nullptr || m_fun != nullptr)
    {
      global_current_language = m_lang;
      lazy_language_setter = m_fun;
      if (lazy_language_setter == nullptr)
	set_range_case ();
    }
}

/* The language that the user expects to be typing in (the language
   of main(), or the last language we notified them about, or C).  */

const struct language_defn *expected_language;

/* Define the array containing all languages.  */

const struct language_defn *language_defn::languages[nr_languages];

/* The current values of the "set language/range/case-sensitive" enum
   commands.  */
static const char *range;
static const char *case_sensitive;

/* See language.h.  */
const char lang_frame_mismatch_warn[] =
N_("Warning: the current language does not match this frame.");

/* This page contains the functions corresponding to GDB commands
   and their helpers.  */

/* Show command.  Display a warning if the language set
   does not match the frame.  */
static void
show_language_command (struct ui_file *file, int from_tty,
		       struct cmd_list_element *c, const char *value)
{
  enum language flang;		/* The language of the frame.  */

  if (language_mode == language_mode_auto)
    gdb_printf (file,
		_("The current source language is "
		  "\"auto; currently %s\".\n"),
		current_language->name ());
  else
    gdb_printf (file,
		_("The current source language is \"%s\".\n"),
		current_language->name ());

  if (has_stack_frames ())
    {
      frame_info_ptr frame;

      frame = get_selected_frame (NULL);
      flang = get_frame_language (frame);
      if (flang != language_unknown
	  && language_mode == language_mode_manual
	  && current_language->la_language != flang)
	gdb_printf (file, "%s\n", _(lang_frame_mismatch_warn));
    }
}

/* Set callback for the "set/show language" setting.  */

static void
set_language (const char *language)
{
  enum language flang = language_unknown;

  /* "local" is a synonym of "auto".  */
  if (strcmp (language, "auto") == 0
      || strcmp (language, "local") == 0)
    {
      /* Enter auto mode.  Set to the current frame's language, if
	 known, or fallback to the initial language.  */
      language_mode = language_mode_auto;
      try
	{
	  frame_info_ptr frame;

	  frame = get_selected_frame (NULL);
	  flang = get_frame_language (frame);
	}
      catch (const gdb_exception_error &ex)
	{
	  flang = language_unknown;
	}

      if (flang != language_unknown)
	set_language (flang);
      else
	set_initial_language ();

      expected_language = current_language;
      return;
    }

  /* Search the list of languages for a match.  */
  for (const auto &lang : language_defn::languages)
    {
      if (strcmp (lang->name (), language) != 0)
	continue;

      /* Found it!  Go into manual mode, and use this language.  */
      language_mode = language_mode_manual;
      lazy_language_setter = nullptr;
      global_current_language = lang;
      set_range_case ();
      expected_language = lang;
      return;
    }

  internal_error ("Couldn't find language `%s' in known languages list.",
		  language);
}

/* Get callback for the "set/show language" setting.  */

static const char *
get_language ()
{
  if (language_mode == language_mode_auto)
    return "auto";

  return current_language->name ();
}

/* Show command.  Display a warning if the range setting does
   not match the current language.  */
static void
show_range_command (struct ui_file *file, int from_tty,
		    struct cmd_list_element *c, const char *value)
{
  if (range_mode == range_mode_auto)
    {
      const char *tmp;

      switch (range_check)
	{
	case range_check_on:
	  tmp = "on";
	  break;
	case range_check_off:
	  tmp = "off";
	  break;
	case range_check_warn:
	  tmp = "warn";
	  break;
	default:
	  internal_error ("Unrecognized range check setting.");
	}

      gdb_printf (file,
		  _("Range checking is \"auto; currently %s\".\n"),
		  tmp);
    }
  else
    gdb_printf (file, _("Range checking is \"%s\".\n"),
		value);

  if (range_check == range_check_warn
      || ((range_check == range_check_on)
	  != current_language->range_checking_on_by_default ()))
    warning (_("the current range check setting "
	       "does not match the language."));
}

/* Set command.  Change the setting for range checking.  */
static void
set_range_command (const char *ignore,
		   int from_tty, struct cmd_list_element *c)
{
  if (strcmp (range, "on") == 0)
    {
      range_check = range_check_on;
      range_mode = range_mode_manual;
    }
  else if (strcmp (range, "warn") == 0)
    {
      range_check = range_check_warn;
      range_mode = range_mode_manual;
    }
  else if (strcmp (range, "off") == 0)
    {
      range_check = range_check_off;
      range_mode = range_mode_manual;
    }
  else if (strcmp (range, "auto") == 0)
    {
      range_mode = range_mode_auto;
      set_range_case ();
      return;
    }
  else
    {
      internal_error (_("Unrecognized range check setting: \"%s\""), range);
    }
  if (range_check == range_check_warn
      || ((range_check == range_check_on)
	  != current_language->range_checking_on_by_default ()))
    warning (_("the current range check setting "
	       "does not match the language."));
}

/* Show command.  Display a warning if the case sensitivity setting does
   not match the current language.  */
static void
show_case_command (struct ui_file *file, int from_tty,
		   struct cmd_list_element *c, const char *value)
{
  if (case_mode == case_mode_auto)
    {
      const char *tmp = NULL;

      switch (case_sensitivity)
	{
	case case_sensitive_on:
	  tmp = "on";
	  break;
	case case_sensitive_off:
	  tmp = "off";
	  break;
	default:
	  internal_error ("Unrecognized case-sensitive setting.");
	}

      gdb_printf (file,
		  _("Case sensitivity in "
		    "name search is \"auto; currently %s\".\n"),
		  tmp);
    }
  else
    gdb_printf (file,
		_("Case sensitivity in name search is \"%s\".\n"),
		value);

  if (case_sensitivity != current_language->case_sensitivity ())
    warning (_("the current case sensitivity setting does not match "
	       "the language."));
}

/* Set command.  Change the setting for case sensitivity.  */

static void
set_case_command (const char *ignore, int from_tty, struct cmd_list_element *c)
{
   if (strcmp (case_sensitive, "on") == 0)
     {
       case_sensitivity = case_sensitive_on;
       case_mode = case_mode_manual;
     }
   else if (strcmp (case_sensitive, "off") == 0)
     {
       case_sensitivity = case_sensitive_off;
       case_mode = case_mode_manual;
     }
   else if (strcmp (case_sensitive, "auto") == 0)
     {
       case_mode = case_mode_auto;
       set_range_case ();
       return;
     }
   else
     {
       internal_error ("Unrecognized case-sensitive setting: \"%s\"",
		       case_sensitive);
     }

   if (case_sensitivity != current_language->case_sensitivity ())
     warning (_("the current case sensitivity setting does not match "
		"the language."));
}

/* Set the status of range and type checking and case sensitivity based on
   the current modes and the current language.
   If SHOW is non-zero, then print out the current language,
   type and range checking status.  */
static void
set_range_case (void)
{
  if (range_mode == range_mode_auto)
    range_check = (current_language->range_checking_on_by_default ()
		   ? range_check_on : range_check_off);

  if (case_mode == case_mode_auto)
    case_sensitivity = current_language->case_sensitivity ();
}

/* See language.h.  */

void
set_language (enum language lang)
{
  lazy_language_setter = nullptr;
  global_current_language = language_def (lang);
  set_range_case ();
}


/* See language.h.  */

void
language_info ()
{
  if (expected_language == current_language)
    return;

  expected_language = current_language;
  gdb_printf (_("Current language:  %s\n"), get_language ());
  show_language_command (gdb_stdout, 1, NULL, NULL);
}

/* This page contains functions for the printing out of
   error messages that occur during type- and range-
   checking.  */

/* This is called when a language fails a range-check.  The
   first argument should be a printf()-style format string, and the
   rest of the arguments should be its arguments.  If range_check is
   range_check_on, an error is printed;  if range_check_warn, a warning;
   otherwise just the message.  */

void
range_error (const char *string,...)
{
  va_list args;

  va_start (args, string);
  switch (range_check)
    {
    case range_check_warn:
      vwarning (string, args);
      break;
    case range_check_on:
      verror (string, args);
      break;
    case range_check_off:
      /* FIXME: cagney/2002-01-30: Should this function print anything
	 when range error is off?  */
      gdb_vprintf (gdb_stderr, string, args);
      gdb_printf (gdb_stderr, "\n");
      break;
    default:
      internal_error (_("bad switch"));
    }
  va_end (args);
}


/* This page contains miscellaneous functions.  */

/* Return the language enum for a given language string.  */

enum language
language_enum (const char *str)
{
  for (const auto &lang : language_defn::languages)
    if (strcmp (lang->name (), str) == 0)
      return lang->la_language;

  return language_unknown;
}

/* Return the language struct for a given language enum.  */

const struct language_defn *
language_def (enum language lang)
{
  const struct language_defn *l = language_defn::languages[lang];
  gdb_assert (l != nullptr);
  return l;
}

/* Return the language as a string.  */

const char *
language_str (enum language lang)
{
  return language_def (lang)->name ();
}



/* Build and install the "set language LANG" command.  */

static void
add_set_language_command ()
{
  static const char **language_names;

  /* Build the language names array, to be used as enumeration in the
     "set language" enum command.  +3 for "auto", "local" and NULL
     termination.  */
  language_names = new const char *[ARRAY_SIZE (language_defn::languages) + 3];

  /* Display "auto", "local" and "unknown" first, and then the rest,
     alpha sorted.  */
  const char **language_names_p = language_names;
  *language_names_p++ = "auto";
  *language_names_p++ = "local";
  *language_names_p++ = language_def (language_unknown)->name ();
  const char **sort_begin = language_names_p;
  for (const auto &lang : language_defn::languages)
    {
      /* Already handled above.  */
      if (lang->la_language == language_unknown)
	continue;
      *language_names_p++ = lang->name ();
    }
  *language_names_p = NULL;
  std::sort (sort_begin, language_names_p, compare_cstrings);

  /* Add the filename extensions.  */
  for (const auto &lang : language_defn::languages)
    for (const char * const &ext : lang->filename_extensions ())
      add_filename_language (ext, lang->la_language);

  /* Build the "help set language" docs.  */
  string_file doc;

  doc.printf (_("Set the current source language.\n"
		"The currently understood settings are:\n\nlocal or "
		"auto    Automatic setting based on source file"));

  for (const auto &lang : language_defn::languages)
    {
      /* Already dealt with these above.  */
      if (lang->la_language == language_unknown)
	continue;

      /* Note that we add the newline at the front, so we don't wind
	 up with a trailing newline.  */
      doc.printf ("\n%-16s Use the %s language",
		  lang->name (),
		  lang->natural_name ());
    }

  add_setshow_enum_cmd ("language", class_support,
			language_names,
			doc.c_str (),
			_("Show the current source language."),
			NULL,
			set_language,
			get_language,
			show_language_command,
			&setlist, &showlist);
}

/* Iterate through all registered languages looking for and calling
   any non-NULL struct language_defn.skip_trampoline() functions.
   Return the result from the first that returns non-zero, or 0 if all
   `fail'.  */
CORE_ADDR 
skip_language_trampoline (const frame_info_ptr &frame, CORE_ADDR pc)
{
  for (const auto &lang : language_defn::languages)
    {
      CORE_ADDR real_pc = lang->skip_trampoline (frame, pc);

      if (real_pc != 0)
	return real_pc;
    }

  return 0;
}

/* Return information about whether TYPE should be passed
   (and returned) by reference at the language level.  */

struct language_pass_by_ref_info
language_pass_by_reference (struct type *type)
{
  return current_language->pass_by_reference_info (type);
}

/* Return the default string containing the list of characters
   delimiting words.  This is a reasonable default value that
   most languages should be able to use.  */

const char *
default_word_break_characters (void)
{
  return " \t\n!@#$%^&*()+=|~`}{[]\"';:?/>.<,-";
}

/* See language.h.  */

void
language_defn::print_array_index (struct type *index_type, LONGEST index,
				  struct ui_file *stream,
				  const value_print_options *options) const
{
  struct value *index_value = value_from_longest (index_type, index);

  gdb_printf (stream, "[");
  value_print (index_value, stream, options);
  gdb_printf (stream, "] = ");
}

/* See language.h.  */

gdb::unique_xmalloc_ptr<char>
language_defn::watch_location_expression (struct type *type,
					  CORE_ADDR addr) const
{
  /* Generates an expression that assumes a C like syntax is valid.  */
  type = check_typedef (check_typedef (type)->target_type ());
  std::string name = type_to_string (type);
  return xstrprintf ("* (%s *) %s", name.c_str (), core_addr_to_string (addr));
}

/* See language.h.  */

void
language_defn::value_print (struct value *val, struct ui_file *stream,
	       const struct value_print_options *options) const
{
  return c_value_print (val, stream, options);
}

/* See language.h.  */

int
language_defn::parser (struct parser_state *ps) const
{
  return c_parse (ps);
}

/* See language.h.  */

void
language_defn::value_print_inner
	(struct value *val, struct ui_file *stream, int recurse,
	 const struct value_print_options *options) const
{
  return c_value_print_inner (val, stream, recurse, options);
}

/* See language.h.  */

void
language_defn::print_typedef (struct type *type, struct symbol *new_symbol,
			      struct ui_file *stream) const
{
  c_print_typedef (type, new_symbol, stream);
}

/* See language.h.  */

bool
language_defn::is_string_type_p (struct type *type) const
{
  return c_is_string_type_p (type);
}

/* See language.h.  */

std::unique_ptr<compile_instance>
language_defn::get_compile_instance () const
{
  return {};
}

/* The default implementation of the get_symbol_name_matcher_inner method
   from the language_defn class.  Matches with strncmp_iw.  */

static bool
default_symbol_name_matcher (const char *symbol_search_name,
			     const lookup_name_info &lookup_name,
			     completion_match_result *comp_match_res)
{
  std::string_view name = lookup_name.name ();
  completion_match_for_lcd *match_for_lcd
    = (comp_match_res != NULL ? &comp_match_res->match_for_lcd : NULL);
  strncmp_iw_mode mode = (lookup_name.completion_mode ()
			  ? strncmp_iw_mode::NORMAL
			  : strncmp_iw_mode::MATCH_PARAMS);

  if (strncmp_iw_with_mode (symbol_search_name, name.data (), name.size (),
			    mode, language_minimal, match_for_lcd) == 0)
    {
      if (comp_match_res != NULL)
	comp_match_res->set_match (symbol_search_name);
      return true;
    }
  else
    return false;
}

/* See language.h.  */

symbol_name_matcher_ftype *
language_defn::get_symbol_name_matcher
	(const lookup_name_info &lookup_name) const
{
  /* If currently in Ada mode, and the lookup name is wrapped in
     '<...>', hijack all symbol name comparisons using the Ada
     matcher, which handles the verbatim matching.  */
  if (current_language->la_language == language_ada
      && lookup_name.ada ().verbatim_p ())
    return current_language->get_symbol_name_matcher_inner (lookup_name);

  return this->get_symbol_name_matcher_inner (lookup_name);
}

/* See language.h.  */

symbol_name_matcher_ftype *
language_defn::get_symbol_name_matcher_inner
	(const lookup_name_info &lookup_name) const
{
  return default_symbol_name_matcher;
}

/* See language.h.  */

const struct lang_varobj_ops *
language_defn::varobj_ops () const
{
  /* The ops for the C language are suitable for the vast majority of the
     supported languages.  */
  return &c_varobj_ops;
}

/* Class representing the "unknown" language.  */

class unknown_language : public language_defn
{
public:
  unknown_language () : language_defn (language_unknown)
  { /* Nothing.  */ }

  /* See language.h.  */
  void language_arch_info (struct gdbarch *gdbarch,
			   struct language_arch_info *lai) const override
  {
    lai->set_string_char_type (builtin_type (gdbarch)->builtin_char);
    lai->set_bool_type (builtin_type (gdbarch)->builtin_int);
  }

  /* See language.h.  */

  void print_type (struct type *type, const char *varstring,
		   struct ui_file *stream, int show, int level,
		   const struct type_print_options *flags) const override
  {
    error (_("type printing not implemented for language \"%s\""),
	   natural_name ());
  }

  /* See language.h.  */

  gdb::unique_xmalloc_ptr<char> demangle_symbol (const char *mangled,
						 int options) const override
  {
    /* The auto language just uses the C++ demangler.  */
    return gdb_demangle (mangled, options);
  }

  /* See language.h.  */

  void value_print (struct value *val, struct ui_file *stream,
		    const struct value_print_options *options) const override
  {
    error (_("value printing not implemented for language \"%s\""),
	   natural_name ());
  }

  /* See language.h.  */

  void value_print_inner
	(struct value *val, struct ui_file *stream, int recurse,
	 const struct value_print_options *options) const override
  {
    error (_("inner value printing not implemented for language \"%s\""),
	   natural_name ());
  }

  /* See language.h.  */

  int parser (struct parser_state *ps) const override
  {
    error (_("expression parsing not implemented for language \"%s\""),
	   natural_name ());
  }

  /* See language.h.  */

  void emitchar (int ch, struct type *chtype,
		 struct ui_file *stream, int quoter) const override
  {
    error (_("emit character not implemented for language \"%s\""),
	   natural_name ());
  }

  /* See language.h.  */

  void printchar (int ch, struct type *chtype,
		  struct ui_file *stream) const override
  {
    error (_("print character not implemented for language \"%s\""),
	   natural_name ());
  }

  /* See language.h.  */

  void printstr (struct ui_file *stream, struct type *elttype,
		 const gdb_byte *string, unsigned int length,
		 const char *encoding, int force_ellipses,
		 const struct value_print_options *options) const override
  {
    error (_("print string not implemented for language \"%s\""),
	   natural_name ());
  }

  /* See language.h.  */

  void print_typedef (struct type *type, struct symbol *new_symbol,
		      struct ui_file *stream) const override
  {
    error (_("print typedef not implemented for language \"%s\""),
	   natural_name ());
  }

  /* See language.h.  */

  bool is_string_type_p (struct type *type) const override
  {
    type = check_typedef (type);
    while (type->code () == TYPE_CODE_REF)
      {
	type = type->target_type ();
	type = check_typedef (type);
      }
    return (type->code () == TYPE_CODE_STRING);
  }

  /* See language.h.  */

  const char *name_of_this () const override
  { return "this"; }

  /* See language.h.  */

  const char *name () const override
  { return "unknown"; }

  /* See language.h.  */

  const char *natural_name () const override
  { return "Unknown"; }

  /* See language.h.  */

  bool store_sym_names_in_linkage_form_p () const override
  { return true; }
};

/* Single instance of the unknown language class.  */

static unknown_language unknown_language_defn;


/* Per-architecture language information.  */

struct language_gdbarch
{
  /* A vector of per-language per-architecture info.  Indexed by "enum
     language".  */
  struct language_arch_info arch_info[nr_languages];
};

static const registry<gdbarch>::key<language_gdbarch> language_gdbarch_data;

static language_gdbarch *
get_language_gdbarch (struct gdbarch *gdbarch)
{
  struct language_gdbarch *l = language_gdbarch_data.get (gdbarch);
  if (l == nullptr)
    {
      l = new struct language_gdbarch;
      for (const auto &lang : language_defn::languages)
	{
	  gdb_assert (lang != nullptr);
	  lang->language_arch_info (gdbarch, &l->arch_info[lang->la_language]);
	}
      language_gdbarch_data.set (gdbarch, l);
    }

  return l;
}

/* See language.h.  */

struct type *
language_string_char_type (const struct language_defn *la,
			   struct gdbarch *gdbarch)
{
  struct language_gdbarch *ld = get_language_gdbarch (gdbarch);
  return ld->arch_info[la->la_language].string_char_type ();
}

/* See language.h.  */

struct value *
language_defn::value_string (struct gdbarch *gdbarch,
			     const char *ptr, ssize_t len) const
{
  struct type *type = language_string_char_type (this, gdbarch);
  return value_cstring (ptr, len, type);
}

/* See language.h.  */

struct type *
language_bool_type (const struct language_defn *la,
		    struct gdbarch *gdbarch)
{
  struct language_gdbarch *ld = get_language_gdbarch (gdbarch);
  return ld->arch_info[la->la_language].bool_type ();
}

/* See language.h.  */

struct type *
language_arch_info::bool_type () const
{
  if (m_bool_type_name != nullptr)
    {
      struct symbol *sym;

      sym = lookup_symbol (m_bool_type_name, NULL, VAR_DOMAIN, NULL).symbol;
      if (sym != nullptr)
	{
	  struct type *type = sym->type ();
	  if (type != nullptr && type->code () == TYPE_CODE_BOOL)
	    return type;
	}
    }

  return m_bool_type_default;
}

/* See language.h.  */

struct symbol *
language_arch_info::type_and_symbol::alloc_type_symbol
	(enum language lang, struct type *type)
{
  struct symbol *symbol;
  struct gdbarch *gdbarch;
  gdb_assert (!type->is_objfile_owned ());
  gdbarch = type->arch_owner ();
  symbol = new (gdbarch_obstack (gdbarch)) struct symbol ();
  symbol->m_name = type->name ();
  symbol->set_language (lang, nullptr);
  symbol->owner.arch = gdbarch;
  symbol->set_is_objfile_owned (0);
  symbol->set_section_index (0);
  symbol->set_type (type);
  symbol->set_domain (VAR_DOMAIN);
  symbol->set_aclass_index (LOC_TYPEDEF);
  return symbol;
}

/* See language.h.  */

language_arch_info::type_and_symbol *
language_arch_info::lookup_primitive_type_and_symbol (const char *name)
{
  for (struct type_and_symbol &tas : primitive_types_and_symbols)
    {
      if (strcmp (tas.type ()->name (), name) == 0)
	return &tas;
    }

  return nullptr;
}

/* See language.h.  */

struct type *
language_arch_info::lookup_primitive_type (const char *name)
{
  type_and_symbol *tas = lookup_primitive_type_and_symbol (name);
  if (tas != nullptr)
    return tas->type ();
  return nullptr;
}

/* See language.h.  */

struct type *
language_arch_info::lookup_primitive_type
  (gdb::function_view<bool (struct type *)> filter)
{
  for (struct type_and_symbol &tas : primitive_types_and_symbols)
    {
      if (filter (tas.type ()))
	return tas.type ();
    }

  return nullptr;
}

/* See language.h.  */

struct symbol *
language_arch_info::lookup_primitive_type_as_symbol (const char *name,
						     enum language lang)
{
  type_and_symbol *tas = lookup_primitive_type_and_symbol (name);
  if (tas != nullptr)
    return tas->symbol (lang);
  return nullptr;
}

/* Helper for the language_lookup_primitive_type overloads to forward
   to the corresponding language's lookup_primitive_type overload.  */

template<typename T>
static struct type *
language_lookup_primitive_type_1 (const struct language_defn *la,
				  struct gdbarch *gdbarch,
				  T arg)
{
  struct language_gdbarch *ld = get_language_gdbarch (gdbarch);
  return ld->arch_info[la->la_language].lookup_primitive_type (arg);
}

/* See language.h.  */

struct type *
language_lookup_primitive_type (const struct language_defn *la,
				struct gdbarch *gdbarch,
				const char *name)
{
  return language_lookup_primitive_type_1 (la, gdbarch, name);
}

/* See language.h.  */

struct type *
language_lookup_primitive_type (const struct language_defn *la,
				struct gdbarch *gdbarch,
				gdb::function_view<bool (struct type *)> filter)
{
  return language_lookup_primitive_type_1 (la, gdbarch, filter);
}

/* See language.h.  */

struct symbol *
language_lookup_primitive_type_as_symbol (const struct language_defn *la,
					  struct gdbarch *gdbarch,
					  const char *name)
{
  struct language_gdbarch *ld = get_language_gdbarch (gdbarch);
  struct language_arch_info *lai = &ld->arch_info[la->la_language];

  symbol_lookup_debug_printf
    ("language = \"%s\", gdbarch @ %s, type = \"%s\")",
     la->name (), host_address_to_string (gdbarch), name);

  struct symbol *sym
    = lai->lookup_primitive_type_as_symbol (name, la->la_language);

  symbol_lookup_debug_printf ("found symbol @ %s",
			      host_address_to_string (sym));

  /* Note: The result of symbol lookup is normally a symbol *and* the block
     it was found in.  Builtin types don't live in blocks.  We *could* give
     them one, but there is no current need so to keep things simple symbol
     lookup is extended to allow for BLOCK_FOUND to be NULL.  */

  return sym;
}

/* Initialize the language routines.  */

void _initialize_language ();
void
_initialize_language ()
{
  static const char *const type_or_range_names[]
    = { "on", "off", "warn", "auto", NULL };

  static const char *const case_sensitive_names[]
    = { "on", "off", "auto", NULL };

  /* GDB commands for language specific stuff.  */

  set_show_commands setshow_check_cmds
    = add_setshow_prefix_cmd ("check", no_class,
			      _("Set the status of the type/range checker."),
			      _("Show the status of the type/range checker."),
			      &setchecklist, &showchecklist,
			      &setlist, &showlist);
  add_alias_cmd ("c", setshow_check_cmds.set, no_class, 1, &setlist);
  add_alias_cmd ("ch", setshow_check_cmds.set, no_class, 1, &setlist);
  add_alias_cmd ("c", setshow_check_cmds.show, no_class, 1, &showlist);
  add_alias_cmd ("ch", setshow_check_cmds.show, no_class, 1, &showlist);

  range = type_or_range_names[3];
  gdb_assert (strcmp (range, "auto") == 0);
  add_setshow_enum_cmd ("range", class_support, type_or_range_names,
			&range,
			_("Set range checking (on/warn/off/auto)."),
			_("Show range checking (on/warn/off/auto)."),
			NULL, set_range_command,
			show_range_command,
			&setchecklist, &showchecklist);

  case_sensitive = case_sensitive_names[2];
  gdb_assert (strcmp (case_sensitive, "auto") == 0);
  add_setshow_enum_cmd ("case-sensitive", class_support, case_sensitive_names,
			&case_sensitive, _("\
Set case sensitivity in name search (on/off/auto)."), _("\
Show case sensitivity in name search (on/off/auto)."), _("\
For Fortran the default is off; for other languages the default is on."),
			set_case_command,
			show_case_command,
			&setlist, &showlist);

  add_set_language_command ();
}
