/* Cache of styled source file text
   Copyright (C) 2018-2024 Free Software Foundation, Inc.

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
#include "source-cache.h"
#include "gdbsupport/scoped_fd.h"
#include "source.h"
#include "cli/cli-style.h"
#include "symtab.h"
#include "objfiles.h"
#include "exec.h"
#include "cli/cli-cmds.h"

#ifdef HAVE_SOURCE_HIGHLIGHT
/* If Gnulib redirects 'open' and 'close' to its replacements
   'rpl_open' and 'rpl_close' via cpp macros, including <fstream>
   below with those macros in effect will cause unresolved externals
   when GDB is linked.  Happens, e.g., in the MinGW build.  */
#undef open
#undef close
#include <sstream>
#include <srchilite/sourcehighlight.h>
#include <srchilite/langmap.h>
#include <srchilite/settings.h>
#endif

#if GDB_SELF_TEST
#include "gdbsupport/selftest.h"
#endif

/* The number of source files we'll cache.  */

#define MAX_ENTRIES 5

/* See source-cache.h.  */

source_cache g_source_cache;

/* When this is true we will use the GNU Source Highlight to add styling to
   source code (assuming the library is available).  This is initialized to
   true (if appropriate) in _initialize_source_cache below.  */

static bool use_gnu_source_highlight;

/* The "maint show gnu-source-highlight enabled" command. */

static void
show_use_gnu_source_highlight_enabled  (struct ui_file *file, int from_tty,
					struct cmd_list_element *c,
					const char *value)
{
  gdb_printf (file,
	      _("Use of GNU Source Highlight library is \"%s\".\n"),
	      value);
}

/* The "maint set gnu-source-highlight enabled" command.  */

static void
set_use_gnu_source_highlight_enabled (const char *ignore_args,
				      int from_tty,
				      struct cmd_list_element *c)
{
#ifndef HAVE_SOURCE_HIGHLIGHT
  /* If the library is not available and the user tried to enable use of
     the library, then disable use of the library, and give an error.  */
  if (use_gnu_source_highlight)
    {
      use_gnu_source_highlight = false;
      error (_("the GNU Source Highlight library is not available"));
    }
#else
  /* We (might) have just changed how we style source code, discard any
     previously cached contents.  */
  forget_cached_source_info ();
#endif
}

/* See source-cache.h.  */

std::string
source_cache::get_plain_source_lines (struct symtab *s,
				      const std::string &fullname)
{
  scoped_fd desc (open_source_file (s));
  if (desc.get () < 0)
    perror_with_name (symtab_to_filename_for_display (s), -desc.get ());

  struct stat st;
  if (fstat (desc.get (), &st) < 0)
    perror_with_name (symtab_to_filename_for_display (s));

  std::string lines;
  lines.resize (st.st_size);
  if (myread (desc.get (), &lines[0], lines.size ()) < 0)
    perror_with_name (symtab_to_filename_for_display (s));

  time_t mtime = 0;
  if (s->compunit ()->objfile () != NULL
      && s->compunit ()->objfile ()->obfd != NULL)
    mtime = s->compunit ()->objfile ()->mtime;
  else if (current_program_space->exec_bfd ())
    mtime = current_program_space->ebfd_mtime;

  if (mtime && mtime < st.st_mtime)
    warning (_("Source file is more recent than executable."));

  std::vector<off_t> offsets;
  offsets.push_back (0);
  for (size_t offset = lines.find ('\n');
       offset != std::string::npos;
       offset = lines.find ('\n', offset))
    {
      ++offset;
      /* A newline at the end does not start a new line.  It would
	 seem simpler to just strip the newline in this function, but
	 then "list" won't print the final newline.  */
      if (offset != lines.size ())
	offsets.push_back (offset);
    }

  offsets.shrink_to_fit ();
  m_offset_cache.emplace (fullname, std::move (offsets));

  return lines;
}

#ifdef HAVE_SOURCE_HIGHLIGHT

/* Return the Source Highlight language name, given a gdb language
   LANG.  Returns NULL if the language is not known.  */

static const char *
get_language_name (enum language lang)
{
  switch (lang)
    {
    case language_c:
    case language_objc:
      return "c.lang";

    case language_cplus:
      return "cpp.lang";

    case language_d:
      return "d.lang";

    case language_go:
      return "go.lang";

    case language_fortran:
      return "fortran.lang";

    case language_m2:
      /* Not handled by Source Highlight.  */
      break;

    case language_asm:
      return "asm.lang";

    case language_pascal:
      return "pascal.lang";

    case language_opencl:
      /* Not handled by Source Highlight.  */
      break;

    case language_rust:
      return "rust.lang";

    case language_ada:
      return "ada.lang";

    default:
      break;
    }

  return nullptr;
}

#endif /* HAVE_SOURCE_HIGHLIGHT */

/* Try to highlight CONTENTS from file FULLNAME in language LANG using
   the GNU source-higlight library.  Return true if highlighting
   succeeded.  */

static bool
try_source_highlight (std::string &contents ATTRIBUTE_UNUSED,
		      enum language lang ATTRIBUTE_UNUSED,
		      const std::string &fullname ATTRIBUTE_UNUSED)
{
#ifdef HAVE_SOURCE_HIGHLIGHT
  if (!use_gnu_source_highlight)
    return false;

  const char *lang_name = get_language_name (lang);

  /* The global source highlight object, or null if one was
     never constructed.  This is stored here rather than in
     the class so that we don't need to include anything or do
     conditional compilation in source-cache.h.  */
  static srchilite::SourceHighlight *highlighter;

  /* The global source highlight language map object.  */
  static srchilite::LangMap *langmap;

  bool styled = false;
  try
    {
      if (highlighter == nullptr)
	{
	  highlighter = new srchilite::SourceHighlight ("esc.outlang");
	  highlighter->setStyleFile ("esc.style");

	  const std::string &datadir = srchilite::Settings::retrieveDataDir ();
	  langmap = new srchilite::LangMap (datadir, "lang.map");
	}

      std::string detected_lang;
      if (lang_name == nullptr)
	{
	  detected_lang = langmap->getMappedFileNameFromFileName (fullname);
	  if (detected_lang.empty ())
	    return false;
	  lang_name = detected_lang.c_str ();
	}

      std::istringstream input (contents);
      std::ostringstream output;
      highlighter->highlight (input, output, lang_name, fullname);
      contents = std::move (output).str ();
      styled = true;
    }
  catch (...)
    {
      /* Source Highlight will throw an exception if
	 highlighting fails.  One possible reason it can fail
	 is if the language is unknown -- which matters to gdb
	 because Rust support wasn't added until after 3.1.8.
	 Ignore exceptions here.  */
    }

  return styled;
#else
  return false;
#endif /* HAVE_SOURCE_HIGHLIGHT */
}

#ifdef HAVE_SOURCE_HIGHLIGHT
#if GDB_SELF_TEST
namespace selftests
{
static void gnu_source_highlight_test ()
{
  const std::string prog
    = ("int\n"
       "foo (void)\n"
       "{\n"
       "  return 0;\n"
       "}\n");
  const std::string fullname = "test.c";
  std::string styled_prog;

  bool res = false;
  bool saw_exception = false;
  styled_prog = prog;
  try
    {
      res = try_source_highlight (styled_prog, language_c, fullname);
    }
  catch (...)
    {
      saw_exception = true;
    }

  SELF_CHECK (!saw_exception);
  if (res)
    SELF_CHECK (prog.size () < styled_prog.size ());
  else
    SELF_CHECK (prog == styled_prog);
}
}
#endif /* GDB_SELF_TEST */
#endif /* HAVE_SOURCE_HIGHLIGHT */

/* See source-cache.h.  */

bool
source_cache::ensure (struct symtab *s)
{
  std::string fullname = symtab_to_fullname (s);

  size_t size = m_source_map.size ();
  for (int i = 0; i < size; ++i)
    {
      if (m_source_map[i].fullname == fullname)
	{
	  /* This should always hold, because we create the file offsets
	     when reading the file.  */
	  gdb_assert (m_offset_cache.find (fullname)
		      != m_offset_cache.end ());
	  /* Not strictly LRU, but at least ensure that the most
	     recently used entry is always the last candidate for
	     deletion.  Note that this property is relied upon by at
	     least one caller.  */
	  if (i != size - 1)
	    std::swap (m_source_map[i], m_source_map[size - 1]);
	  return true;
	}
    }

  std::string contents;
  try
    {
      contents = get_plain_source_lines (s, fullname);
    }
  catch (const gdb_exception_error &e)
    {
      /* If 's' is not found, an exception is thrown.  */
      return false;
    }

  if (source_styling && gdb_stdout->can_emit_style_escape ()
      && m_no_styling_files.count (fullname) == 0)
    {
      bool already_styled
	= try_source_highlight (contents, s->language (), fullname);

      if (!already_styled)
	{
	  std::optional<std::string> ext_contents;
	  ext_contents = ext_lang_colorize (fullname, contents);
	  if (ext_contents.has_value ())
	    {
	      contents = std::move (*ext_contents);
	      already_styled = true;
	    }
	}

      if (!already_styled)
	{
	  /* Styling failed.  Styling can fail for instance for these
	     reasons:
	     - the language is not supported.
	     - the language cannot not be auto-detected from the file name.
	     - no stylers available.

	     Since styling failed, don't try styling the file again after it
	     drops from the cache.

	     Note that clearing the source cache also clears
	     m_no_styling_files.  */
	  m_no_styling_files.insert (fullname);
	}
    }

  source_text result = { std::move (fullname), std::move (contents) };
  m_source_map.push_back (std::move (result));

  if (m_source_map.size () > MAX_ENTRIES)
    {
      auto iter = m_source_map.begin ();
      m_offset_cache.erase (iter->fullname);
      m_source_map.erase (iter);
    }

  return true;
}

/* See source-cache.h.  */

bool
source_cache::get_line_charpos (struct symtab *s,
				const std::vector<off_t> **offsets)
{
  std::string fullname = symtab_to_fullname (s);

  auto iter = m_offset_cache.find (fullname);
  if (iter == m_offset_cache.end ())
    {
      if (!ensure (s))
	return false;
      iter = m_offset_cache.find (fullname);
      /* cache_source_text ensured this was entered.  */
      gdb_assert (iter != m_offset_cache.end ());
    }

  *offsets = &iter->second;
  return true;
}

/* A helper function that extracts the desired source lines from TEXT,
   putting them into LINES_OUT.  The arguments are as for
   get_source_lines.  Returns true on success, false if the line
   numbers are invalid.  */

static bool
extract_lines (const std::string &text, int first_line, int last_line,
	       std::string *lines_out)
{
  int lineno = 1;
  std::string::size_type pos = 0;
  std::string::size_type first_pos = std::string::npos;

  while (pos != std::string::npos && lineno <= last_line)
    {
      std::string::size_type new_pos = text.find ('\n', pos);

      if (lineno == first_line)
	first_pos = pos;

      pos = new_pos;
      if (lineno == last_line || pos == std::string::npos)
	{
	  /* A newline at the end does not start a new line.  */
	  if (first_pos == std::string::npos
	      || first_pos == text.size ())
	    return false;
	  if (pos == std::string::npos)
	    pos = text.size ();
	  else
	    ++pos;
	  *lines_out = text.substr (first_pos, pos - first_pos);
	  return true;
	}
      ++lineno;
      ++pos;
    }

  return false;
}

/* See source-cache.h.  */

bool
source_cache::get_source_lines (struct symtab *s, int first_line,
				int last_line, std::string *lines)
{
  if (first_line < 1 || last_line < 1 || first_line > last_line)
    return false;

  if (!ensure (s))
    return false;

  return extract_lines (m_source_map.back ().contents,
			first_line, last_line, lines);
}

/* Implement 'maint flush source-cache' command.  */

static void
source_cache_flush_command (const char *command, int from_tty)
{
  forget_cached_source_info ();
  gdb_printf (_("Source cache flushed.\n"));
}

#if GDB_SELF_TEST
namespace selftests
{
static void extract_lines_test ()
{
  std::string input_text = "abc\ndef\nghi\njkl\n";
  std::string result;

  SELF_CHECK (extract_lines (input_text, 1, 1, &result)
	      && result == "abc\n");
  SELF_CHECK (!extract_lines (input_text, 2, 1, &result));
  SELF_CHECK (extract_lines (input_text, 1, 2, &result)
	      && result == "abc\ndef\n");
  SELF_CHECK (extract_lines ("abc", 1, 1, &result)
	      && result == "abc");
}
}
#endif

void _initialize_source_cache ();
void
_initialize_source_cache ()
{
  add_cmd ("source-cache", class_maintenance, source_cache_flush_command,
	   _("Force gdb to flush its source code cache."),
	   &maintenanceflushlist);

  /* All the 'maint set|show gnu-source-highlight' sub-commands.  */
  static struct cmd_list_element *maint_set_gnu_source_highlight_cmdlist;
  static struct cmd_list_element *maint_show_gnu_source_highlight_cmdlist;

  /* Adds 'maint set|show gnu-source-highlight'.  */
  add_setshow_prefix_cmd ("gnu-source-highlight", class_maintenance,
			  _("Set gnu-source-highlight specific variables."),
			  _("Show gnu-source-highlight specific variables."),
			  &maint_set_gnu_source_highlight_cmdlist,
			  &maint_show_gnu_source_highlight_cmdlist,
			  &maintenance_set_cmdlist,
			  &maintenance_show_cmdlist);

  /* Adds 'maint set|show gnu-source-highlight enabled'.  */
  add_setshow_boolean_cmd ("enabled", class_maintenance,
			   &use_gnu_source_highlight, _("\
Set whether the GNU Source Highlight library should be used."), _("\
Show whether the GNU Source Highlight library is being used."),_("\
When enabled, GDB will use the GNU Source Highlight library to apply\n\
styling to source code lines that are shown."),
			   set_use_gnu_source_highlight_enabled,
			   show_use_gnu_source_highlight_enabled,
			   &maint_set_gnu_source_highlight_cmdlist,
			   &maint_show_gnu_source_highlight_cmdlist);

  /* Enable use of GNU Source Highlight library, if we have it.  */
#ifdef HAVE_SOURCE_HIGHLIGHT
  use_gnu_source_highlight = true;
#endif

#if GDB_SELF_TEST
  selftests::register_test ("source-cache", selftests::extract_lines_test);
#ifdef HAVE_SOURCE_HIGHLIGHT
  selftests::register_test ("gnu-source-highlight",
			    selftests::gnu_source_highlight_test);
#endif
#endif
}
