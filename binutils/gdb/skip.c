/* Skipping uninteresting files and functions while stepping.

   Copyright (C) 2011-2024 Free Software Foundation, Inc.

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
#include "skip.h"
#include "value.h"
#include "valprint.h"
#include "ui-out.h"
#include "symtab.h"
#include "gdbcmd.h"
#include "command.h"
#include "completer.h"
#include "stack.h"
#include "cli/cli-utils.h"
#include "arch-utils.h"
#include "linespec.h"
#include "objfiles.h"
#include "breakpoint.h"
#include "source.h"
#include "filenames.h"
#include "fnmatch.h"
#include "gdbsupport/gdb_regex.h"
#include <optional>
#include <list>
#include "cli/cli-style.h"
#include "gdbsupport/buildargv.h"

/* True if we want to print debug printouts related to file/function
   skipping. */
static bool debug_skip = false;

class skiplist_entry
{
public:
  /* Create a skiplist_entry object and add it to the chain.  */
  static void add_entry (bool file_is_glob,
			 std::string &&file,
			 bool function_is_regexp,
			 std::string &&function);

  /* Return true if the skip entry has a file or glob-style file
     pattern that matches FUNCTION_SAL.  */
  bool skip_file_p (const symtab_and_line &function_sal) const;

  /* Return true if the skip entry has a function or function regexp
     that matches FUNCTION_NAME.  */
  bool skip_function_p (const char *function_name) const;

  /* Getters.  */
  int number () const { return m_number; };
  bool enabled () const { return m_enabled; };
  bool file_is_glob () const { return m_file_is_glob; }
  const std::string &file () const { return m_file; }
  const std::string &function () const { return m_function; }
  bool function_is_regexp () const { return m_function_is_regexp; }

  /* Setters.  */
  void enable () { m_enabled = true; };
  void disable () { m_enabled = false; };

  /* Disable copy.  */
  skiplist_entry (const skiplist_entry &) = delete;
  void operator= (const skiplist_entry &) = delete;

private:
  /* Key that grants access to the constructor.  */
  struct private_key {};
public:
  /* Public so we can construct with container::emplace_back.  Since
     it requires a private class key, it can't be called from outside.
     Use the add_entry static factory method to construct instead.  */
  skiplist_entry (bool file_is_glob, std::string &&file,
		  bool function_is_regexp, std::string &&function,
		  private_key);

private:
  /* Return true if we're stopped at a file to be skipped.  */
  bool do_skip_file_p (const symtab_and_line &function_sal) const;

  /* Return true if we're stopped at a globbed file to be skipped.  */
  bool do_skip_gfile_p (const symtab_and_line &function_sal) const;

private: /* data */
  int m_number = -1;

  /* True if FILE is a glob-style pattern.
     Otherwise it is the plain file name (possibly with directories).  */
  bool m_file_is_glob;

  /* The name of the file or empty if no name.  */
  std::string m_file;

  /* True if FUNCTION is a regexp.
     Otherwise it is a plain function name (possibly with arguments,
     for C++).  */
  bool m_function_is_regexp;

  /* The name of the function or empty if no name.  */
  std::string m_function;

  /* If this is a function regexp, the compiled form.  */
  std::optional<compiled_regex> m_compiled_function_regexp;

  /* Enabled/disabled state.  */
  bool m_enabled = true;
};

static std::list<skiplist_entry> skiplist_entries;
static int highest_skiplist_entry_num = 0;

skiplist_entry::skiplist_entry (bool file_is_glob,
				std::string &&file,
				bool function_is_regexp,
				std::string &&function,
				private_key)
  : m_file_is_glob (file_is_glob),
    m_file (std::move (file)),
    m_function_is_regexp (function_is_regexp),
    m_function (std::move (function))
{
  gdb_assert (!m_file.empty () || !m_function.empty ());

  if (m_file_is_glob)
    gdb_assert (!m_file.empty ());

  if (m_function_is_regexp)
    {
      gdb_assert (!m_function.empty ());
      m_compiled_function_regexp.emplace (m_function.c_str (),
					  REG_NOSUB | REG_EXTENDED,
					  _("regexp"));
    }
}

void
skiplist_entry::add_entry (bool file_is_glob, std::string &&file,
			   bool function_is_regexp, std::string &&function)
{
  skiplist_entries.emplace_back (file_is_glob,
				 std::move (file),
				 function_is_regexp,
				 std::move (function),
				 private_key {});

  /* Incremented after push_back, in case push_back throws.  */
  skiplist_entries.back ().m_number = ++highest_skiplist_entry_num;
}

static void
skip_file_command (const char *arg, int from_tty)
{
  struct symtab *symtab;
  const char *filename = NULL;

  /* If no argument was given, try to default to the last
     displayed codepoint.  */
  if (arg == NULL)
    {
      symtab = get_last_displayed_symtab ();
      if (symtab == NULL)
	error (_("No default file now."));

      /* It is not a typo, symtab_to_filename_for_display would be needlessly
	 ambiguous.  */
      filename = symtab_to_fullname (symtab);
    }
  else
    filename = arg;

  skiplist_entry::add_entry (false, std::string (filename),
			     false, std::string ());

  gdb_printf (_("File %s will be skipped when stepping.\n"), filename);
}

/* Create a skiplist entry for the given function NAME and add it to the
   list.  */

static void
skip_function (const char *name)
{
  skiplist_entry::add_entry (false, std::string (), false, std::string (name));

  gdb_printf (_("Function %s will be skipped when stepping.\n"), name);
}

static void
skip_function_command (const char *arg, int from_tty)
{
  /* Default to the current function if no argument is given.  */
  if (arg == NULL)
    {
      frame_info_ptr fi = get_selected_frame (_("No default function now."));
      struct symbol *sym = get_frame_function (fi);
      const char *name = NULL;

      if (sym != NULL)
	name = sym->print_name ();
      else
	error (_("No function found containing current program point %s."),
	       paddress (get_current_arch (), get_frame_pc (fi)));
      skip_function (name);
      return;
    }

  skip_function (arg);
}

/* Process "skip ..." that does not match "skip file" or "skip function".  */

static void
skip_command (const char *arg, int from_tty)
{
  const char *file = NULL;
  const char *gfile = NULL;
  const char *function = NULL;
  const char *rfunction = NULL;
  int i;

  if (arg == NULL)
    {
      skip_function_command (arg, from_tty);
      return;
    }

  gdb_argv argv (arg);

  for (i = 0; argv[i] != NULL; ++i)
    {
      const char *p = argv[i];
      const char *value = argv[i + 1];

      if (strcmp (p, "-fi") == 0
	  || strcmp (p, "-file") == 0)
	{
	  if (value == NULL)
	    error (_("Missing value for %s option."), p);
	  file = value;
	  ++i;
	}
      else if (strcmp (p, "-gfi") == 0
	       || strcmp (p, "-gfile") == 0)
	{
	  if (value == NULL)
	    error (_("Missing value for %s option."), p);
	  gfile = value;
	  ++i;
	}
      else if (strcmp (p, "-fu") == 0
	       || strcmp (p, "-function") == 0)
	{
	  if (value == NULL)
	    error (_("Missing value for %s option."), p);
	  function = value;
	  ++i;
	}
      else if (strcmp (p, "-rfu") == 0
	       || strcmp (p, "-rfunction") == 0)
	{
	  if (value == NULL)
	    error (_("Missing value for %s option."), p);
	  rfunction = value;
	  ++i;
	}
      else if (*p == '-')
	error (_("Invalid skip option: %s"), p);
      else if (i == 0)
	{
	  /* Assume the user entered "skip FUNCTION-NAME".
	     FUNCTION-NAME may be `foo (int)', and therefore we pass the
	     complete original arg to skip_function command as if the user
	     typed "skip function arg".  */
	  skip_function_command (arg, from_tty);
	  return;
	}
      else
	error (_("Invalid argument: %s"), p);
    }

  if (file != NULL && gfile != NULL)
    error (_("Cannot specify both -file and -gfile."));

  if (function != NULL && rfunction != NULL)
    error (_("Cannot specify both -function and -rfunction."));

  /* This shouldn't happen as "skip" by itself gets punted to
     skip_function_command.  */
  gdb_assert (file != NULL || gfile != NULL
	      || function != NULL || rfunction != NULL);

  std::string entry_file;
  if (file != NULL)
    entry_file = file;
  else if (gfile != NULL)
    entry_file = gfile;

  std::string entry_function;
  if (function != NULL)
    entry_function = function;
  else if (rfunction != NULL)
    entry_function = rfunction;

  skiplist_entry::add_entry (gfile != NULL, std::move (entry_file),
			     rfunction != NULL, std::move (entry_function));

  /* I18N concerns drive some of the choices here (we can't piece together
     the output too much).  OTOH we want to keep this simple.  Therefore the
     only polish we add to the output is to append "(s)" to "File" or
     "Function" if they're a glob/regexp.  */
  {
    const char *file_to_print = file != NULL ? file : gfile;
    const char *function_to_print = function != NULL ? function : rfunction;
    const char *file_text = gfile != NULL ? _("File(s)") : _("File");
    const char *lower_file_text = gfile != NULL ? _("file(s)") : _("file");
    const char *function_text
      = rfunction != NULL ? _("Function(s)") : _("Function");

    if (function_to_print == NULL)
      {
	gdb_printf (_("%s %s will be skipped when stepping.\n"),
		    file_text, file_to_print);
      }
    else if (file_to_print == NULL)
      {
	gdb_printf (_("%s %s will be skipped when stepping.\n"),
		    function_text, function_to_print);
      }
    else
      {
	gdb_printf (_("%s %s in %s %s will be skipped"
		      " when stepping.\n"),
		    function_text, function_to_print,
		    lower_file_text, file_to_print);
      }
  }
}

static void
info_skip_command (const char *arg, int from_tty)
{
  int num_printable_entries = 0;
  struct value_print_options opts;

  get_user_print_options (&opts);

  /* Count the number of rows in the table and see if we need space for a
     64-bit address anywhere.  */
  for (const skiplist_entry &e : skiplist_entries)
    if (arg == NULL || number_is_in_list (arg, e.number ()))
      num_printable_entries++;

  if (num_printable_entries == 0)
    {
      if (arg == NULL)
	current_uiout->message (_("Not skipping any files or functions.\n"));
      else
	current_uiout->message (
	  _("No skiplist entries found with number %s.\n"), arg);

      return;
    }

  ui_out_emit_table table_emitter (current_uiout, 6, num_printable_entries,
				   "SkiplistTable");

  current_uiout->table_header (5, ui_left, "number", "Num");   /* 1 */
  current_uiout->table_header (3, ui_left, "enabled", "Enb");  /* 2 */
  current_uiout->table_header (4, ui_right, "regexp", "Glob"); /* 3 */
  current_uiout->table_header (20, ui_left, "file", "File");   /* 4 */
  current_uiout->table_header (2, ui_right, "regexp", "RE");   /* 5 */
  current_uiout->table_header (40, ui_noalign, "function", "Function"); /* 6 */
  current_uiout->table_body ();

  for (const skiplist_entry &e : skiplist_entries)
    {
      QUIT;
      if (arg != NULL && !number_is_in_list (arg, e.number ()))
	continue;

      ui_out_emit_tuple tuple_emitter (current_uiout, "blklst-entry");
      current_uiout->field_signed ("number", e.number ()); /* 1 */

      if (e.enabled ())
	current_uiout->field_string ("enabled", "y"); /* 2 */
      else
	current_uiout->field_string ("enabled", "n"); /* 2 */

      if (e.file_is_glob ())
	current_uiout->field_string ("regexp", "y"); /* 3 */
      else
	current_uiout->field_string ("regexp", "n"); /* 3 */

      current_uiout->field_string ("file",
				   e.file ().empty () ? "<none>"
				   : e.file ().c_str (),
				   e.file ().empty ()
				   ? metadata_style.style ()
				   : file_name_style.style ()); /* 4 */
      if (e.function_is_regexp ())
	current_uiout->field_string ("regexp", "y"); /* 5 */
      else
	current_uiout->field_string ("regexp", "n"); /* 5 */

      current_uiout->field_string ("function",
				   e.function ().empty () ? "<none>"
				   : e.function ().c_str (),
				   e.function ().empty ()
				   ? metadata_style.style ()
				   : function_name_style.style ()); /* 6 */

      current_uiout->text ("\n");
    }
}

static void
skip_enable_command (const char *arg, int from_tty)
{
  bool found = false;

  for (skiplist_entry &e : skiplist_entries)
    if (arg == NULL || number_is_in_list (arg, e.number ()))
      {
	e.enable ();
	found = true;
      }

  if (!found)
    error (_("No skiplist entries found with number %s."), arg);
}

static void
skip_disable_command (const char *arg, int from_tty)
{
  bool found = false;

  for (skiplist_entry &e : skiplist_entries)
    if (arg == NULL || number_is_in_list (arg, e.number ()))
      {
	e.disable ();
	found = true;
      }

  if (!found)
    error (_("No skiplist entries found with number %s."), arg);
}

static void
skip_delete_command (const char *arg, int from_tty)
{
  bool found = false;

  for (auto it = skiplist_entries.begin (),
	 end = skiplist_entries.end ();
       it != end;)
    {
      const skiplist_entry &e = *it;

      if (arg == NULL || number_is_in_list (arg, e.number ()))
	{
	  it = skiplist_entries.erase (it);
	  found = true;
	}
      else
	++it;
    }

  if (!found)
    error (_("No skiplist entries found with number %s."), arg);
}

bool
skiplist_entry::do_skip_file_p (const symtab_and_line &function_sal) const
{
  if (debug_skip)
    gdb_printf (gdb_stdlog,
		"skip: checking if file %s matches non-glob %s...",
		function_sal.symtab->filename, m_file.c_str ());

  bool result;

  /* Check first sole SYMTAB->FILENAME.  It may not be a substring of
     symtab_to_fullname as it may contain "./" etc.  */
  if (compare_filenames_for_search (function_sal.symtab->filename,
				    m_file.c_str ()))
    result = true;

  /* Before we invoke realpath, which can get expensive when many
     files are involved, do a quick comparison of the basenames.  */
  else if (!basenames_may_differ
	   && filename_cmp (lbasename (function_sal.symtab->filename),
			    lbasename (m_file.c_str ())) != 0)
    result = false;
  else
    {
      /* Note: symtab_to_fullname caches its result, thus we don't have to.  */
      const char *fullname = symtab_to_fullname (function_sal.symtab);

      result = compare_filenames_for_search (fullname, m_file.c_str ());
    }

  if (debug_skip)
    gdb_printf (gdb_stdlog, result ? "yes.\n" : "no.\n");

  return result;
}

bool
skiplist_entry::do_skip_gfile_p (const symtab_and_line &function_sal) const
{
  if (debug_skip)
    gdb_printf (gdb_stdlog,
		"skip: checking if file %s matches glob %s...",
		function_sal.symtab->filename, m_file.c_str ());

  bool result;

  /* Check first sole SYMTAB->FILENAME.  It may not be a substring of
     symtab_to_fullname as it may contain "./" etc.  */
  if (gdb_filename_fnmatch (m_file.c_str (), function_sal.symtab->filename,
			    FNM_FILE_NAME | FNM_NOESCAPE) == 0)
    result = true;

  /* Before we invoke symtab_to_fullname, which is expensive, do a quick
     comparison of the basenames.
     Note that we assume that lbasename works with glob-style patterns.
     If the basename of the glob pattern is something like "*.c" then this
     isn't much of a win.  Oh well.  */
  else if (!basenames_may_differ
      && gdb_filename_fnmatch (lbasename (m_file.c_str ()),
			       lbasename (function_sal.symtab->filename),
			       FNM_FILE_NAME | FNM_NOESCAPE) != 0)
    result = false;
  else
    {
      /* Note: symtab_to_fullname caches its result, thus we don't have to.  */
      const char *fullname = symtab_to_fullname (function_sal.symtab);

      result = compare_glob_filenames_for_search (fullname, m_file.c_str ());
    }

  if (debug_skip)
    gdb_printf (gdb_stdlog, result ? "yes.\n" : "no.\n");

  return result;
}

bool
skiplist_entry::skip_file_p (const symtab_and_line &function_sal) const
{
  if (m_file.empty ())
    return false;

  if (function_sal.symtab == NULL)
    return false;

  if (m_file_is_glob)
    return do_skip_gfile_p (function_sal);
  else
    return do_skip_file_p (function_sal);
}

bool
skiplist_entry::skip_function_p (const char *function_name) const
{
  if (m_function.empty ())
    return false;

  bool result;

  if (m_function_is_regexp)
    {
      if (debug_skip)
	gdb_printf (gdb_stdlog,
		    "skip: checking if function %s matches regex %s...",
		    function_name, m_function.c_str ());

      gdb_assert (m_compiled_function_regexp);
      result
	= (m_compiled_function_regexp->exec (function_name, 0, NULL, 0) == 0);
    }
  else
    {
      if (debug_skip)
	gdb_printf (gdb_stdlog,
		    ("skip: checking if function %s matches non-regex "
		     "%s..."),
		    function_name, m_function.c_str ());
      result = (strcmp_iw (function_name, m_function.c_str ()) == 0);
    }

  if (debug_skip)
    gdb_printf (gdb_stdlog, result ? "yes.\n" : "no.\n");

  return result;
}

/* See skip.h.  */

bool
function_name_is_marked_for_skip (const char *function_name,
				  const symtab_and_line &function_sal)
{
  if (function_name == NULL)
    return false;

  for (const skiplist_entry &e : skiplist_entries)
    {
      if (!e.enabled ())
	continue;

      bool skip_by_file = e.skip_file_p (function_sal);
      bool skip_by_function = e.skip_function_p (function_name);

      /* If both file and function must match, make sure we don't errantly
	 exit if only one of them match.  */
      if (!e.file ().empty () && !e.function ().empty ())
	{
	  if (skip_by_file && skip_by_function)
	    return true;
	}
      /* Only one of file/function is specified.  */
      else if (skip_by_file || skip_by_function)
	return true;
    }

  return false;
}

/* Completer for skip numbers.  */

static void
complete_skip_number (cmd_list_element *cmd,
		      completion_tracker &completer,
		      const char *text, const char *word)
{
  size_t word_len = strlen (word);

  for (const skiplist_entry &entry : skiplist_entries)
    {
      gdb::unique_xmalloc_ptr<char> name = xstrprintf ("%d", entry.number ());
      if (strncmp (word, name.get (), word_len) == 0)
	completer.add_completion (std::move (name));
    }
}

void _initialize_step_skip ();
void
_initialize_step_skip ()
{
  static struct cmd_list_element *skiplist = NULL;
  struct cmd_list_element *c;

  add_prefix_cmd ("skip", class_breakpoint, skip_command, _("\
Ignore a function while stepping.\n\
\n\
Usage: skip [FUNCTION-NAME]\n\
       skip [FILE-SPEC] [FUNCTION-SPEC]\n\
If no arguments are given, ignore the current function.\n\
\n\
FILE-SPEC is one of:\n\
       -fi|-file FILE-NAME\n\
       -gfi|-gfile GLOB-FILE-PATTERN\n\
FUNCTION-SPEC is one of:\n\
       -fu|-function FUNCTION-NAME\n\
       -rfu|-rfunction FUNCTION-NAME-REGULAR-EXPRESSION"),
		  &skiplist, 1, &cmdlist);

  c = add_cmd ("file", class_breakpoint, skip_file_command, _("\
Ignore a file while stepping.\n\
Usage: skip file [FILE-NAME]\n\
If no filename is given, ignore the current file."),
	       &skiplist);
  set_cmd_completer (c, filename_completer);

  c = add_cmd ("function", class_breakpoint, skip_function_command, _("\
Ignore a function while stepping.\n\
Usage: skip function [FUNCTION-NAME]\n\
If no function name is given, skip the current function."),
	       &skiplist);
  set_cmd_completer (c, location_completer);

  c = add_cmd ("enable", class_breakpoint, skip_enable_command, _("\
Enable skip entries.\n\
Usage: skip enable [NUMBER | RANGE]...\n\
You can specify numbers (e.g. \"skip enable 1 3\"),\n\
ranges (e.g. \"skip enable 4-8\"), or both (e.g. \"skip enable 1 3 4-8\").\n\n\
If you don't specify any numbers or ranges, we'll enable all skip entries."),
	       &skiplist);
  set_cmd_completer (c, complete_skip_number);

  c = add_cmd ("disable", class_breakpoint, skip_disable_command, _("\
Disable skip entries.\n\
Usage: skip disable [NUMBER | RANGE]...\n\
You can specify numbers (e.g. \"skip disable 1 3\"),\n\
ranges (e.g. \"skip disable 4-8\"), or both (e.g. \"skip disable 1 3 4-8\").\n\n\
If you don't specify any numbers or ranges, we'll disable all skip entries."),
	       &skiplist);
  set_cmd_completer (c, complete_skip_number);

  c = add_cmd ("delete", class_breakpoint, skip_delete_command, _("\
Delete skip entries.\n\
Usage: skip delete [NUMBER | RANGES]...\n\
You can specify numbers (e.g. \"skip delete 1 3\"),\n\
ranges (e.g. \"skip delete 4-8\"), or both (e.g. \"skip delete 1 3 4-8\").\n\n\
If you don't specify any numbers or ranges, we'll delete all skip entries."),
	       &skiplist);
  set_cmd_completer (c, complete_skip_number);

  add_info ("skip", info_skip_command, _("\
Display the status of skips.\n\
Usage: info skip [NUMBER | RANGES]...\n\
You can specify numbers (e.g. \"info skip 1 3\"), \n\
ranges (e.g. \"info skip 4-8\"), or both (e.g. \"info skip 1 3 4-8\").\n\n\
If you don't specify any numbers or ranges, we'll show all skips."));
  set_cmd_completer (c, complete_skip_number);

  add_setshow_boolean_cmd ("skip", class_maintenance,
			   &debug_skip, _("\
Set whether to print the debug output about skipping files and functions."),
			   _("\
Show whether the debug output about skipping files and functions is printed."),
			   _("\
When non-zero, debug output about skipping files and functions is displayed."),
			   NULL, NULL,
			   &setdebuglist, &showdebuglist);
}
