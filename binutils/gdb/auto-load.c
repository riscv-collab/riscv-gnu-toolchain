/* GDB routines for supporting auto-loaded scripts.

   Copyright (C) 2012-2024 Free Software Foundation, Inc.

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
#include <ctype.h>
#include "auto-load.h"
#include "progspace.h"
#include "gdbsupport/gdb_regex.h"
#include "ui-out.h"
#include "filenames.h"
#include "command.h"
#include "observable.h"
#include "objfiles.h"
#include "cli/cli-script.h"
#include "gdbcmd.h"
#include "cli/cli-cmds.h"
#include "cli/cli-decode.h"
#include "cli/cli-setshow.h"
#include "readline/tilde.h"
#include "completer.h"
#include "fnmatch.h"
#include "top.h"
#include "gdbsupport/filestuff.h"
#include "extension.h"
#include "gdb/section-scripts.h"
#include <algorithm>
#include "gdbsupport/pathstuff.h"
#include "cli/cli-style.h"

/* The section to look in for auto-loaded scripts (in file formats that
   support sections).
   Each entry in this section is a record that begins with a leading byte
   identifying the record type.
   At the moment we only support one record type: A leading byte of 1,
   followed by the path of a python script to load.  */
#define AUTO_SECTION_NAME ".debug_gdb_scripts"

/* The section to look in for the name of a separate debug file.  */
#define DEBUGLINK_SECTION_NAME ".gnu_debuglink"

static void maybe_print_unsupported_script_warning
  (struct auto_load_pspace_info *, struct objfile *objfile,
   const struct extension_language_defn *language,
   const char *section_name, unsigned offset);

static void maybe_print_script_not_found_warning
  (struct auto_load_pspace_info *, struct objfile *objfile,
   const struct extension_language_defn *language,
   const char *section_name, unsigned offset);

/* See auto-load.h.  */

bool debug_auto_load = false;

/* "show" command for the debug_auto_load configuration variable.  */

static void
show_debug_auto_load (struct ui_file *file, int from_tty,
		      struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Debugging output for files "
		      "of 'set auto-load ...' is %s.\n"),
	      value);
}

/* User-settable option to enable/disable auto-loading of GDB_AUTO_FILE_NAME
   scripts:
   set auto-load gdb-scripts on|off
   This is true if we should auto-load associated scripts when an objfile
   is opened, false otherwise.  */
static bool auto_load_gdb_scripts = true;

/* "show" command for the auto_load_gdb_scripts configuration variable.  */

static void
show_auto_load_gdb_scripts (struct ui_file *file, int from_tty,
			    struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Auto-loading of canned sequences of commands "
		      "scripts is %s.\n"),
	      value);
}

/* See auto-load.h.  */

bool
auto_load_gdb_scripts_enabled (const struct extension_language_defn *extlang)
{
  return auto_load_gdb_scripts;
}

/* Internal-use flag to enable/disable auto-loading.
   This is true if we should auto-load python code when an objfile is opened,
   false otherwise.

   Both auto_load_scripts && global_auto_load must be true to enable
   auto-loading.

   This flag exists to facilitate deferring auto-loading during start-up
   until after ./.gdbinit has been read; it may augment the search directories
   used to find the scripts.  */
bool global_auto_load = true;

/* Auto-load .gdbinit file from the current directory?  */
bool auto_load_local_gdbinit = true;

/* Absolute pathname to the current directory .gdbinit, if it exists.  */
char *auto_load_local_gdbinit_pathname = NULL;

/* if AUTO_LOAD_LOCAL_GDBINIT_PATHNAME has been loaded.  */
bool auto_load_local_gdbinit_loaded = false;

/* "show" command for the auto_load_local_gdbinit configuration variable.  */

static void
show_auto_load_local_gdbinit (struct ui_file *file, int from_tty,
			      struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Auto-loading of .gdbinit script from current "
		      "directory is %s.\n"),
	      value);
}

/* Directory list from which to load auto-loaded scripts.  It is not checked
   for absolute paths but they are strongly recommended.  It is initialized by
   _initialize_auto_load.  */
static std::string auto_load_dir = AUTO_LOAD_DIR;

/* "set" command for the auto_load_dir configuration variable.  */

static void
set_auto_load_dir (const char *args, int from_tty, struct cmd_list_element *c)
{
  /* Setting the variable to "" resets it to the compile time defaults.  */
  if (auto_load_dir.empty ())
    auto_load_dir = AUTO_LOAD_DIR;
}

/* "show" command for the auto_load_dir configuration variable.  */

static void
show_auto_load_dir (struct ui_file *file, int from_tty,
		    struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("List of directories from which to load "
		      "auto-loaded scripts is %s.\n"),
	      value);
}

/* Directory list safe to hold auto-loaded files.  It is not checked for
   absolute paths but they are strongly recommended.  It is initialized by
   _initialize_auto_load.  */
static std::string auto_load_safe_path = AUTO_LOAD_SAFE_PATH;

/* Vector of directory elements of AUTO_LOAD_SAFE_PATH with each one normalized
   by tilde_expand and possibly each entries has added its gdb_realpath
   counterpart.  */
static std::vector<gdb::unique_xmalloc_ptr<char>> auto_load_safe_path_vec;

/* Expand $datadir and $debugdir in STRING according to the rules of
   substitute_path_component.  */

static std::vector<gdb::unique_xmalloc_ptr<char>>
auto_load_expand_dir_vars (const char *string)
{
  char *s = xstrdup (string);
  substitute_path_component (&s, "$datadir", gdb_datadir.c_str ());
  substitute_path_component (&s, "$debugdir", debug_file_directory.c_str ());

  if (debug_auto_load && strcmp (s, string) != 0)
    auto_load_debug_printf ("Expanded $-variables to \"%s\".", s);

  std::vector<gdb::unique_xmalloc_ptr<char>> dir_vec
    = dirnames_to_char_ptr_vec (s);
  xfree(s);

  return dir_vec;
}

/* Update auto_load_safe_path_vec from current AUTO_LOAD_SAFE_PATH.  */

static void
auto_load_safe_path_vec_update (void)
{
  auto_load_debug_printf ("Updating directories of \"%s\".",
			  auto_load_safe_path.c_str ());

  auto_load_safe_path_vec
    = auto_load_expand_dir_vars (auto_load_safe_path.c_str ());
  size_t len = auto_load_safe_path_vec.size ();

  /* Apply tilde_expand and gdb_realpath to each AUTO_LOAD_SAFE_PATH_VEC
     element.  */
  for (size_t i = 0; i < len; i++)
    {
      gdb::unique_xmalloc_ptr<char> &in_vec = auto_load_safe_path_vec[i];
      gdb::unique_xmalloc_ptr<char> expanded (tilde_expand (in_vec.get ()));
      gdb::unique_xmalloc_ptr<char> real_path = gdb_realpath (expanded.get ());

      /* Ensure the current entry is at least tilde_expand-ed.  ORIGINAL makes
	 sure we free the original string.  */
      gdb::unique_xmalloc_ptr<char> original = std::move (in_vec);
      in_vec = std::move (expanded);

      if (debug_auto_load)
	{
	  if (strcmp (in_vec.get (), original.get ()) == 0)
	    auto_load_debug_printf ("Using directory \"%s\".",
				    in_vec.get ());
	  else
	    auto_load_debug_printf ("Resolved directory \"%s\" as \"%s\".",
				    original.get (), in_vec.get ());
	}

      /* If gdb_realpath returns a different content, append it.  */
      if (strcmp (real_path.get (), in_vec.get ()) != 0)
	{
	  auto_load_debug_printf ("And canonicalized as \"%s\".",
				  real_path.get ());

	  auto_load_safe_path_vec.push_back (std::move (real_path));
	}
    }
}

/* Variable gdb_datadir has been set.  Update content depending on $datadir.  */

static void
auto_load_gdb_datadir_changed (void)
{
  auto_load_safe_path_vec_update ();
}

/* "set" command for the auto_load_safe_path configuration variable.  */

static void
set_auto_load_safe_path (const char *args,
			 int from_tty, struct cmd_list_element *c)
{
  /* Setting the variable to "" resets it to the compile time defaults.  */
  if (auto_load_safe_path.empty ())
    auto_load_safe_path = AUTO_LOAD_SAFE_PATH;

  auto_load_safe_path_vec_update ();
}

/* "show" command for the auto_load_safe_path configuration variable.  */

static void
show_auto_load_safe_path (struct ui_file *file, int from_tty,
			  struct cmd_list_element *c, const char *value)
{
  const char *cs;

  /* Check if user has entered either "/" or for example ":".
     But while more complicate content like ":/foo" would still also
     permit any location do not hide those.  */

  for (cs = value; *cs && (*cs == DIRNAME_SEPARATOR || IS_DIR_SEPARATOR (*cs));
       cs++);
  if (*cs == 0)
    gdb_printf (file, _("Auto-load files are safe to load from any "
			"directory.\n"));
  else
    gdb_printf (file, _("List of directories from which it is safe to "
			"auto-load files is %s.\n"),
		value);
}

/* "add-auto-load-safe-path" command for the auto_load_safe_path configuration
   variable.  */

static void
add_auto_load_safe_path (const char *args, int from_tty)
{
  if (args == NULL || *args == 0)
    error (_("\
Directory argument required.\n\
Use 'set auto-load safe-path /' for disabling the auto-load safe-path security.\
"));

  auto_load_safe_path = string_printf ("%s%c%s", auto_load_safe_path.c_str (),
				       DIRNAME_SEPARATOR, args);

  auto_load_safe_path_vec_update ();
}

/* "add-auto-load-scripts-directory" command for the auto_load_dir configuration
   variable.  */

static void
add_auto_load_dir (const char *args, int from_tty)
{
  if (args == NULL || *args == 0)
    error (_("Directory argument required."));

  auto_load_dir = string_printf ("%s%c%s", auto_load_dir.c_str (),
				 DIRNAME_SEPARATOR, args);
}

/* Implementation for filename_is_in_pattern overwriting the caller's FILENAME
   and PATTERN.  */

static int
filename_is_in_pattern_1 (char *filename, char *pattern)
{
  size_t pattern_len = strlen (pattern);
  size_t filename_len = strlen (filename);

  auto_load_debug_printf ("Matching file \"%s\" to pattern \"%s\"",
			  filename, pattern);

  /* Trim trailing slashes ("/") from PATTERN.  Even for "d:\" paths as
     trailing slashes are trimmed also from FILENAME it still matches
     correctly.  */
  while (pattern_len && IS_DIR_SEPARATOR (pattern[pattern_len - 1]))
    pattern_len--;
  pattern[pattern_len] = '\0';

  /* Ensure auto_load_safe_path "/" matches any FILENAME.  On MS-Windows
     platform FILENAME even after gdb_realpath does not have to start with
     IS_DIR_SEPARATOR character, such as the 'C:\x.exe' filename.  */
  if (pattern_len == 0)
    {
      auto_load_debug_printf ("Matched - empty pattern");
      return 1;
    }

  for (;;)
    {
      /* Trim trailing slashes ("/").  PATTERN also has slashes trimmed the
	 same way so they will match.  */
      while (filename_len && IS_DIR_SEPARATOR (filename[filename_len - 1]))
	filename_len--;
      filename[filename_len] = '\0';
      if (filename_len == 0)
	{
	  auto_load_debug_printf ("Not matched - pattern \"%s\".", pattern);
	  return 0;
	}

      if (gdb_filename_fnmatch (pattern, filename, FNM_FILE_NAME | FNM_NOESCAPE)
	  == 0)
	{
	  auto_load_debug_printf ("Matched - file \"%s\" to pattern \"%s\".",
				  filename, pattern);
	  return 1;
	}

      /* Trim trailing FILENAME component.  */
      while (filename_len > 0 && !IS_DIR_SEPARATOR (filename[filename_len - 1]))
	filename_len--;
    }
}

/* Return 1 if FILENAME matches PATTERN or if FILENAME resides in
   a subdirectory of a directory that matches PATTERN.  Return 0 otherwise.
   gdb_realpath normalization is never done here.  */

static ATTRIBUTE_PURE int
filename_is_in_pattern (const char *filename, const char *pattern)
{
  char *filename_copy, *pattern_copy;

  filename_copy = (char *) alloca (strlen (filename) + 1);
  strcpy (filename_copy, filename);
  pattern_copy = (char *) alloca (strlen (pattern) + 1);
  strcpy (pattern_copy, pattern);

  return filename_is_in_pattern_1 (filename_copy, pattern_copy);
}

/* Return 1 if FILENAME belongs to one of directory components of
   AUTO_LOAD_SAFE_PATH_VEC.  Return 0 otherwise.
   auto_load_safe_path_vec_update is never called.
   *FILENAME_REALP may be updated by gdb_realpath of FILENAME.  */

static int
filename_is_in_auto_load_safe_path_vec (const char *filename,
					gdb::unique_xmalloc_ptr<char> *filename_realp)
{
  const char *pattern = NULL;

  for (const gdb::unique_xmalloc_ptr<char> &p : auto_load_safe_path_vec)
    if (*filename_realp == NULL && filename_is_in_pattern (filename, p.get ()))
      {
	pattern = p.get ();
	break;
      }
  
  if (pattern == NULL)
    {
      if (*filename_realp == NULL)
	{
	  *filename_realp = gdb_realpath (filename);
	  if (debug_auto_load && strcmp (filename_realp->get (), filename) != 0)
	    auto_load_debug_printf ("Resolved file \"%s\" as \"%s\".",
				    filename, filename_realp->get ());
	}

      if (strcmp (filename_realp->get (), filename) != 0)
	for (const gdb::unique_xmalloc_ptr<char> &p : auto_load_safe_path_vec)
	  if (filename_is_in_pattern (filename_realp->get (), p.get ()))
	    {
	      pattern = p.get ();
	      break;
	    }
    }

  if (pattern != NULL)
    {
      auto_load_debug_printf ("File \"%s\" matches directory \"%s\".",
			      filename, pattern);
      return 1;
    }

  return 0;
}

/* See auto-load.h.  */

bool
file_is_auto_load_safe (const char *filename)
{
  gdb::unique_xmalloc_ptr<char> filename_real;
  static bool advice_printed = false;

  if (filename_is_in_auto_load_safe_path_vec (filename, &filename_real))
    return true;

  auto_load_safe_path_vec_update ();
  if (filename_is_in_auto_load_safe_path_vec (filename, &filename_real))
    return true;

  warning (_("File \"%ps\" auto-loading has been declined by your "
	     "`auto-load safe-path' set to \"%s\"."),
	   styled_string (file_name_style.style (), filename_real.get ()),
	   auto_load_safe_path.c_str ());

  if (!advice_printed)
    {
      /* Find the existing home directory config file.  */
      struct stat buf;
      std::string home_config = find_gdb_home_config_file (GDBINIT, &buf);
      if (home_config.empty ())
	{
	  /* The user doesn't have an existing home directory config file,
	     so we should suggest a suitable path for them to use.  */
	  std::string config_dir_file
	    = get_standard_config_filename (GDBINIT);
	  if (!config_dir_file.empty ())
	    home_config = config_dir_file;
	  else
	    {
	      const char *homedir = getenv ("HOME");
	      if (homedir == nullptr)
		homedir = "$HOME";
	      home_config = (std::string (homedir) + SLASH_STRING
			     + std::string (GDBINIT));
	    }
	}

      gdb_printf (_("\
To enable execution of this file add\n\
\tadd-auto-load-safe-path %s\n\
line to your configuration file \"%ps\".\n\
To completely disable this security protection add\n\
\tset auto-load safe-path /\n\
line to your configuration file \"%ps\".\n\
For more information about this security protection see the\n\
\"Auto-loading safe path\" section in the GDB manual.  E.g., run from the shell:\n\
\tinfo \"(gdb)Auto-loading safe path\"\n"),
		       filename_real.get (),
		       styled_string (file_name_style.style (),
				      home_config.c_str ()),
		       styled_string (file_name_style.style (),
				      home_config.c_str ()));
      advice_printed = true;
    }

  return false;
}

/* For scripts specified in .debug_gdb_scripts, multiple objfiles may load
   the same script.  There's no point in loading the script multiple times,
   and there can be a lot of objfiles and scripts, so we keep track of scripts
   loaded this way.  */

struct auto_load_pspace_info
{
  /* For each program space we keep track of loaded scripts, both when
     specified as file names and as scripts to be executed directly.  */
  htab_up loaded_script_files;
  htab_up loaded_script_texts;

  /* Non-zero if we've issued the warning about an auto-load script not being
     supported.  We only want to issue this warning once.  */
  bool unsupported_script_warning_printed = false;

  /* Non-zero if we've issued the warning about an auto-load script not being
     found.  We only want to issue this warning once.  */
  bool script_not_found_warning_printed = false;
};

/* Objects of this type are stored in the loaded_script hash table.  */

struct loaded_script
{
  /* Name as provided by the objfile.  */
  const char *name;

  /* Full path name or NULL if script wasn't found (or was otherwise
     inaccessible), or NULL for loaded_script_texts.  */
  const char *full_path;

  /* True if this script has been loaded.  */
  bool loaded;

  const struct extension_language_defn *language;
};

/* Per-program-space data key.  */
static const registry<program_space>::key<auto_load_pspace_info>
     auto_load_pspace_data;

/* Get the current autoload data.  If none is found yet, add it now.  This
   function always returns a valid object.  */

static struct auto_load_pspace_info *
get_auto_load_pspace_data (struct program_space *pspace)
{
  struct auto_load_pspace_info *info;

  info = auto_load_pspace_data.get (pspace);
  if (info == NULL)
    info = auto_load_pspace_data.emplace (pspace);

  return info;
}

/* Hash function for the loaded script hash.  */

static hashval_t
hash_loaded_script_entry (const void *data)
{
  const struct loaded_script *e = (const struct loaded_script *) data;

  return htab_hash_string (e->name) ^ htab_hash_pointer (e->language);
}

/* Equality function for the loaded script hash.  */

static int
eq_loaded_script_entry (const void *a, const void *b)
{
  const struct loaded_script *ea = (const struct loaded_script *) a;
  const struct loaded_script *eb = (const struct loaded_script *) b;

  return strcmp (ea->name, eb->name) == 0 && ea->language == eb->language;
}

/* Initialize the table to track loaded scripts.
   Each entry is hashed by the full path name.  */

static void
init_loaded_scripts_info (struct auto_load_pspace_info *pspace_info)
{
  /* Choose 31 as the starting size of the hash table, somewhat arbitrarily.
     Space for each entry is obtained with one malloc so we can free them
     easily.  */

  pspace_info->loaded_script_files.reset
    (htab_create (31,
		  hash_loaded_script_entry,
		  eq_loaded_script_entry,
		  xfree));
  pspace_info->loaded_script_texts.reset
    (htab_create (31,
		  hash_loaded_script_entry,
		  eq_loaded_script_entry,
		  xfree));

  pspace_info->unsupported_script_warning_printed = false;
  pspace_info->script_not_found_warning_printed = false;
}

/* Wrapper on get_auto_load_pspace_data to also allocate the hash table
   for loading scripts.  */

struct auto_load_pspace_info *
get_auto_load_pspace_data_for_loading (struct program_space *pspace)
{
  struct auto_load_pspace_info *info;

  info = get_auto_load_pspace_data (pspace);
  if (info->loaded_script_files == NULL)
    init_loaded_scripts_info (info);

  return info;
}

/* Add script file NAME in LANGUAGE to hash table of PSPACE_INFO.
   LOADED is true if the script has been (is going to) be loaded, false
   otherwise (such as if it has not been found).
   FULL_PATH is NULL if the script wasn't found.

   The result is true if the script was already in the hash table.  */

static bool
maybe_add_script_file (struct auto_load_pspace_info *pspace_info, bool loaded,
		       const char *name, const char *full_path,
		       const struct extension_language_defn *language)
{
  struct htab *htab = pspace_info->loaded_script_files.get ();
  struct loaded_script **slot, entry;

  entry.name = name;
  entry.language = language;
  slot = (struct loaded_script **) htab_find_slot (htab, &entry, INSERT);
  bool in_hash_table = *slot != NULL;

  /* If this script is not in the hash table, add it.  */

  if (!in_hash_table)
    {
      char *p;

      /* Allocate all space in one chunk so it's easier to free.  */
      *slot = ((struct loaded_script *)
	       xmalloc (sizeof (**slot)
			+ strlen (name) + 1
			+ (full_path != NULL ? (strlen (full_path) + 1) : 0)));
      p = ((char*) *slot) + sizeof (**slot);
      strcpy (p, name);
      (*slot)->name = p;
      if (full_path != NULL)
	{
	  p += strlen (p) + 1;
	  strcpy (p, full_path);
	  (*slot)->full_path = p;
	}
      else
	(*slot)->full_path = NULL;
      (*slot)->loaded = loaded;
      (*slot)->language = language;
    }

  return in_hash_table;
}

/* Add script contents NAME in LANGUAGE to hash table of PSPACE_INFO.
   LOADED is true if the script has been (is going to) be loaded, false
   otherwise (such as if it has not been found).

   The result is true if the script was already in the hash table.  */

static bool
maybe_add_script_text (struct auto_load_pspace_info *pspace_info,
		       bool loaded, const char *name,
		       const struct extension_language_defn *language)
{
  struct htab *htab = pspace_info->loaded_script_texts.get ();
  struct loaded_script **slot, entry;

  entry.name = name;
  entry.language = language;
  slot = (struct loaded_script **) htab_find_slot (htab, &entry, INSERT);
  bool in_hash_table = *slot != NULL;

  /* If this script is not in the hash table, add it.  */

  if (!in_hash_table)
    {
      char *p;

      /* Allocate all space in one chunk so it's easier to free.  */
      *slot = ((struct loaded_script *)
	       xmalloc (sizeof (**slot) + strlen (name) + 1));
      p = ((char*) *slot) + sizeof (**slot);
      strcpy (p, name);
      (*slot)->name = p;
      (*slot)->full_path = NULL;
      (*slot)->loaded = loaded;
      (*slot)->language = language;
    }

  return in_hash_table;
}

/* Clear the table of loaded section scripts.  */

static void
clear_section_scripts (program_space *pspace)
{
  auto_load_pspace_info *info = auto_load_pspace_data.get (pspace);
  if (info != NULL && info->loaded_script_files != NULL)
    auto_load_pspace_data.clear (pspace);
}

/* Look for the auto-load script in LANGUAGE associated with OBJFILE where
   OBJFILE's gdb_realpath is REALNAME and load it.  Return 1 if we found any
   matching script, return 0 otherwise.  */

static int
auto_load_objfile_script_1 (struct objfile *objfile, const char *realname,
			    const struct extension_language_defn *language)
{
  const char *debugfile;
  int retval;
  const char *suffix = ext_lang_auto_load_suffix (language);

  std::string filename = std::string (realname) + suffix;

  gdb_file_up input = gdb_fopen_cloexec (filename.c_str (), "r");
  debugfile = filename.c_str ();

  auto_load_debug_printf ("Attempted file \"%ps\" %s.",
			  styled_string (file_name_style.style (), debugfile),
			  input != nullptr ? "exists" : "does not exist");

  std::string debugfile_holder;
  if (!input)
    {
      /* Also try the same file in a subdirectory of gdb's data
	 directory.  */

      std::vector<gdb::unique_xmalloc_ptr<char>> vec
	= auto_load_expand_dir_vars (auto_load_dir.c_str ());

      auto_load_debug_printf
	("Searching 'set auto-load scripts-directory' path \"%s\".",
	 auto_load_dir.c_str ());

      /* Convert Windows file name from c:/dir/file to /c/dir/file.  */
      if (HAS_DRIVE_SPEC (debugfile))
	filename = (std::string("\\") + debugfile[0]
		    + STRIP_DRIVE_SPEC (debugfile));

      for (const gdb::unique_xmalloc_ptr<char> &dir : vec)
	{
	  /* FILENAME is absolute, so we don't need a "/" here.  */
	  debugfile_holder = dir.get () + filename;
	  debugfile = debugfile_holder.c_str ();

	  input = gdb_fopen_cloexec (debugfile, "r");

	  auto_load_debug_printf ("Attempted file \"%ps\" %s.",
				  styled_string (file_name_style.style (),
						 debugfile),
				  (input != nullptr
				   ? "exists"
				   : "does not exist"));

	  if (input != NULL)
	    break;
	}
    }

  if (input)
    {
      struct auto_load_pspace_info *pspace_info;

      auto_load_debug_printf
	("Loading %s script \"%s\" by extension for objfile \"%s\".",
	 ext_lang_name (language), debugfile, objfile_name (objfile));

      bool is_safe = file_is_auto_load_safe (debugfile);

      /* Add this script to the hash table too so
	 "info auto-load ${lang}-scripts" can print it.  */
      pspace_info
	= get_auto_load_pspace_data_for_loading (objfile->pspace);
      maybe_add_script_file (pspace_info, is_safe, debugfile, debugfile,
			     language);

      /* To preserve existing behaviour we don't check for whether the
	 script was already in the table, and always load it.
	 It's highly unlikely that we'd ever load it twice,
	 and these scripts are required to be idempotent under multiple
	 loads anyway.  */
      if (is_safe)
	{
	  objfile_script_sourcer_func *sourcer
	    = ext_lang_objfile_script_sourcer (language);

	  /* We shouldn't get here if support for the language isn't
	     compiled in.  And the extension language is required to implement
	     this function.  */
	  gdb_assert (sourcer != NULL);
	  sourcer (language, objfile, input.get (), debugfile);
	}

      retval = 1;
    }
  else
    retval = 0;

  return retval;
}

/* Look for the auto-load script in LANGUAGE associated with OBJFILE and load
   it.  */

void
auto_load_objfile_script (struct objfile *objfile,
			  const struct extension_language_defn *language)
{
  gdb::unique_xmalloc_ptr<char> realname
    = gdb_realpath (objfile_name (objfile));

  if (auto_load_objfile_script_1 (objfile, realname.get (), language))
    return;

  /* For Windows/DOS .exe executables, strip the .exe suffix, so that
     FOO-gdb.gdb could be used for FOO.exe, and try again.  */

  size_t len = strlen (realname.get ());
  const size_t lexe = sizeof (".exe") - 1;

  if (len > lexe && strcasecmp (realname.get () + len - lexe, ".exe") == 0)
    {
      len -= lexe;
      realname.get ()[len] = '\0';

      auto_load_debug_printf
	("Stripped .exe suffix, retrying with \"%s\".", realname.get ());

      auto_load_objfile_script_1 (objfile, realname.get (), language);
      return;
    }

  /* If OBJFILE is a separate debug file and its name does not match
     the name given in the parent's .gnu_debuglink section, try to
     find the auto-load script using the parent's path and the
     debuglink name.  */

  struct objfile *parent = objfile->separate_debug_objfile_backlink;
  if (parent != nullptr)
    {
      uint32_t crc32;
      gdb::unique_xmalloc_ptr<char> debuglink
	(bfd_get_debug_link_info (parent->obfd.get (), &crc32));

      if (debuglink.get () != nullptr
	  && strcmp (debuglink.get (), lbasename (realname.get ())) != 0)
	{
	  /* Replace the last component of the parent's path with the
	     debuglink name.  */

	  std::string p_realname = gdb_realpath (objfile_name (parent)).get ();
	  size_t last = p_realname.find_last_of ('/');

	  if (last != std::string::npos)
	    {
	      p_realname.replace (last + 1, std::string::npos,
				  debuglink.get ());

	      auto_load_debug_printf
		("Debug filename mismatch, retrying with \"%s\".",
		 p_realname.c_str ());

	      auto_load_objfile_script_1 (objfile,
					  p_realname.c_str (), language);
	    }
	}
    }
}

/* Subroutine of source_section_scripts to simplify it.
   Load FILE as a script in extension language LANGUAGE.
   The script is from section SECTION_NAME in OBJFILE at offset OFFSET.  */

static void
source_script_file (struct auto_load_pspace_info *pspace_info,
		    struct objfile *objfile,
		    const struct extension_language_defn *language,
		    const char *section_name, unsigned int offset,
		    const char *file)
{
  objfile_script_sourcer_func *sourcer;

  /* Skip this script if support is not compiled in.  */
  sourcer = ext_lang_objfile_script_sourcer (language);
  if (sourcer == NULL)
    {
      /* We don't throw an error, the program is still debuggable.  */
      maybe_print_unsupported_script_warning (pspace_info, objfile, language,
					      section_name, offset);
      /* We *could* still try to open it, but there's no point.  */
      maybe_add_script_file (pspace_info, 0, file, NULL, language);
      return;
    }

  /* Skip this script if auto-loading it has been disabled.  */
  if (!ext_lang_auto_load_enabled (language))
    {
      /* No message is printed, just skip it.  */
      return;
    }

  std::optional<open_script> opened = find_and_open_script (file,
							    1 /*search_path*/);

  if (opened)
    {
      auto_load_debug_printf
	("Loading %s script \"%s\" from section \"%s\" of objfile \"%s\".",
	 ext_lang_name (language), opened->full_path.get (),
	 section_name, objfile_name (objfile));

      if (!file_is_auto_load_safe (opened->full_path.get ()))
	opened.reset ();
    }
  else
    {
      /* If one script isn't found it's not uncommon for more to not be
	 found either.  We don't want to print a message for each script,
	 too much noise.  Instead, we print the warning once and tell the
	 user how to find the list of scripts that weren't loaded.
	 We don't throw an error, the program is still debuggable.

	 IWBN if complaints.c were more general-purpose.  */

      maybe_print_script_not_found_warning (pspace_info, objfile, language,
					    section_name, offset);
    }

  bool in_hash_table
    = maybe_add_script_file (pspace_info, bool (opened), file,
			     (opened ? opened->full_path.get (): NULL),
			     language);

  /* If this file is not currently loaded, load it.  */
  if (opened && !in_hash_table)
    sourcer (language, objfile, opened->stream.get (),
	     opened->full_path.get ());
}

/* Subroutine of source_section_scripts to simplify it.
   Execute SCRIPT as a script in extension language LANG.
   The script is from section SECTION_NAME in OBJFILE at offset OFFSET.  */

static void
execute_script_contents (struct auto_load_pspace_info *pspace_info,
			 struct objfile *objfile,
			 const struct extension_language_defn *language,
			 const char *section_name, unsigned int offset,
			 const char *script)
{
  objfile_script_executor_func *executor;
  const char *newline, *script_text;
  const char *name;

  /* The first line of the script is the name of the script.
     It must not contain any kind of space character.  */
  name = NULL;
  newline = strchr (script, '\n');
  std::string name_holder;
  if (newline != NULL)
    {
      const char *buf, *p;

      /* Put the name in a buffer and validate it.  */
      name_holder = std::string (script, newline - script);
      buf = name_holder.c_str ();
      for (p = buf; *p != '\0'; ++p)
	{
	  if (isspace (*p))
	    break;
	}
      /* We don't allow nameless scripts, they're not helpful to the user.  */
      if (p != buf && *p == '\0')
	name = buf;
    }
  if (name == NULL)
    {
      /* We don't throw an error, the program is still debuggable.  */
      warning (_("\
Missing/bad script name in entry at offset %u in section %s\n\
of file %ps."),
	       offset, section_name,
	       styled_string (file_name_style.style (),
			      objfile_name (objfile)));
      return;
    }
  script_text = newline + 1;

  /* Skip this script if support is not compiled in.  */
  executor = ext_lang_objfile_script_executor (language);
  if (executor == NULL)
    {
      /* We don't throw an error, the program is still debuggable.  */
      maybe_print_unsupported_script_warning (pspace_info, objfile, language,
					      section_name, offset);
      maybe_add_script_text (pspace_info, 0, name, language);
      return;
    }

  /* Skip this script if auto-loading it has been disabled.  */
  if (!ext_lang_auto_load_enabled (language))
    {
      /* No message is printed, just skip it.  */
      return;
    }

  auto_load_debug_printf
    ("Loading %s script \"%s\" from section \"%s\" of objfile \"%s\".",
     ext_lang_name (language), name, section_name, objfile_name (objfile));

  bool is_safe = file_is_auto_load_safe (objfile_name (objfile));

  bool in_hash_table
    = maybe_add_script_text (pspace_info, is_safe, name, language);

  /* If this file is not currently loaded, load it.  */
  if (is_safe && !in_hash_table)
    executor (language, objfile, name, script_text);
}

/* Load scripts specified in OBJFILE.
   START,END delimit a buffer containing a list of nul-terminated
   file names.
   SECTION_NAME is used in error messages.

   Scripts specified as file names are found per normal "source -s" command
   processing.  First the script is looked for in $cwd.  If not found there
   the source search path is used.

   The section contains a list of path names of script files to load or
   actual script contents.  Each entry is nul-terminated.  */

static void
source_section_scripts (struct objfile *objfile, const char *section_name,
			const char *start, const char *end)
{
  auto_load_pspace_info *pspace_info
    = get_auto_load_pspace_data_for_loading (objfile->pspace);

  for (const char *p = start; p < end; ++p)
    {
      const char *entry;
      const struct extension_language_defn *language;
      unsigned int offset = p - start;
      int code = *p;

      switch (code)
	{
	case SECTION_SCRIPT_ID_PYTHON_FILE:
	case SECTION_SCRIPT_ID_PYTHON_TEXT:
	  language = get_ext_lang_defn (EXT_LANG_PYTHON);
	  break;
	case SECTION_SCRIPT_ID_SCHEME_FILE:
	case SECTION_SCRIPT_ID_SCHEME_TEXT:
	  language = get_ext_lang_defn (EXT_LANG_GUILE);
	  break;
	default:
	  warning (_("Invalid entry in %s section"), section_name);
	  /* We could try various heuristics to find the next valid entry,
	     but it's safer to just punt.  */
	  return;
	}
      entry = ++p;

      while (p < end && *p != '\0')
	++p;
      if (p == end)
	{
	  warning (_("Non-nul-terminated entry in %s at offset %u"),
		   section_name, offset);
	  /* Don't load/execute it.  */
	  break;
	}

      switch (code)
	{
	case SECTION_SCRIPT_ID_PYTHON_FILE:
	case SECTION_SCRIPT_ID_SCHEME_FILE:
	  if (p == entry)
	    {
	      warning (_("Empty entry in %s at offset %u"),
		       section_name, offset);
	      continue;
	    }
	  source_script_file (pspace_info, objfile, language,
			      section_name, offset, entry);
	  break;
	case SECTION_SCRIPT_ID_PYTHON_TEXT:
	case SECTION_SCRIPT_ID_SCHEME_TEXT:
	  execute_script_contents (pspace_info, objfile, language,
				   section_name, offset, entry);
	  break;
	}
    }
}

/* Load scripts specified in section SECTION_NAME of OBJFILE.  */

static void
auto_load_section_scripts (struct objfile *objfile, const char *section_name)
{
  bfd *abfd = objfile->obfd.get ();
  asection *scripts_sect;
  bfd_byte *data = NULL;

  scripts_sect = bfd_get_section_by_name (abfd, section_name);
  if (scripts_sect == NULL
      || (bfd_section_flags (scripts_sect) & SEC_HAS_CONTENTS) == 0)
    return;

  if (!bfd_get_full_section_contents (abfd, scripts_sect, &data))
    warning (_("Couldn't read %s section of %ps"),
	     section_name,
	     styled_string (file_name_style.style (),
			    bfd_get_filename (abfd)));
  else
    {
      gdb::unique_xmalloc_ptr<bfd_byte> data_holder (data);

      char *p = (char *) data;
      source_section_scripts (objfile, section_name, p,
			      p + bfd_section_size (scripts_sect));
    }
}

/* Load any auto-loaded scripts for OBJFILE.

   Two flavors of auto-loaded scripts are supported.
   1) based on the path to the objfile
   2) from .debug_gdb_scripts section  */

void
load_auto_scripts_for_objfile (struct objfile *objfile)
{
  /* Return immediately if auto-loading has been globally disabled.
     This is to handle sequencing of operations during gdb startup.
     Also return immediately if OBJFILE was not created from a file
     on the local filesystem.  */
  if (!global_auto_load
      || (objfile->flags & OBJF_NOT_FILENAME) != 0
      || is_target_filename (objfile->original_name))
    return;

  /* Load any extension language scripts for this objfile.
     E.g., foo-gdb.gdb, foo-gdb.py.  */
  auto_load_ext_lang_scripts_for_objfile (objfile);

  /* Load any scripts mentioned in AUTO_SECTION_NAME (.debug_gdb_scripts).  */
  auto_load_section_scripts (objfile, AUTO_SECTION_NAME);
}

/* Collect scripts to be printed in a vec.  */

struct collect_matching_scripts_data
{
  collect_matching_scripts_data (std::vector<loaded_script *> *scripts_p_,
				 const extension_language_defn *language_)
  : scripts_p (scripts_p_), language (language_)
  {}

  std::vector<loaded_script *> *scripts_p;
  const struct extension_language_defn *language;
};

/* Traversal function for htab_traverse.
   Collect the entry if it matches the regexp.  */

static int
collect_matching_scripts (void **slot, void *info)
{
  struct loaded_script *script = (struct loaded_script *) *slot;
  struct collect_matching_scripts_data *data
    = (struct collect_matching_scripts_data *) info;

  if (script->language == data->language && re_exec (script->name))
    data->scripts_p->push_back (script);

  return 1;
}

/* Print SCRIPT.  */

static void
print_script (struct loaded_script *script)
{
  struct ui_out *uiout = current_uiout;

  ui_out_emit_tuple tuple_emitter (uiout, NULL);

  uiout->field_string ("loaded", script->loaded ? "Yes" : "No");
  uiout->field_string ("script", script->name);
  uiout->text ("\n");

  /* If the name isn't the full path, print it too.  */
  if (script->full_path != NULL
      && strcmp (script->name, script->full_path) != 0)
    {
      uiout->text ("\tfull name: ");
      uiout->field_string ("full_path", script->full_path);
      uiout->text ("\n");
    }
}

/* Helper for info_auto_load_scripts to sort the scripts by name.  */

static bool
sort_scripts_by_name (loaded_script *a, loaded_script *b)
{
  return FILENAME_CMP (a->name, b->name) < 0;
}

/* Special internal GDB value of auto_load_info_scripts's PATTERN identify
   the "info auto-load XXX" command has been executed through the general
   "info auto-load" invocation.  Extra newline will be printed if needed.  */
char auto_load_info_scripts_pattern_nl[] = "";

/* Subroutine of auto_load_info_scripts to simplify it.
   Print SCRIPTS.  */

static void
print_scripts (const std::vector<loaded_script *> &scripts)
{
  for (loaded_script *script : scripts)
    print_script (script);
}

/* Implementation for "info auto-load gdb-scripts"
   (and "info auto-load python-scripts").  List scripts in LANGUAGE matching
   PATTERN.  FROM_TTY is the usual GDB boolean for user interactivity.  */

void
auto_load_info_scripts (program_space *pspace, const char *pattern,
			int from_tty, const extension_language_defn *language)
{
  struct ui_out *uiout = current_uiout;

  dont_repeat ();

  auto_load_pspace_info *pspace_info = get_auto_load_pspace_data (pspace);

  if (pattern && *pattern)
    {
      char *re_err = re_comp (pattern);

      if (re_err)
	error (_("Invalid regexp: %s"), re_err);
    }
  else
    {
      re_comp ("");
    }

  /* We need to know the number of rows before we build the table.
     Plus we want to sort the scripts by name.
     So first traverse the hash table collecting the matching scripts.  */

  std::vector<loaded_script *> script_files, script_texts;

  if (pspace_info != NULL && pspace_info->loaded_script_files != NULL)
    {
      collect_matching_scripts_data data (&script_files, language);

      /* Pass a pointer to scripts as VEC_safe_push can realloc space.  */
      htab_traverse_noresize (pspace_info->loaded_script_files.get (),
			      collect_matching_scripts, &data);

      std::sort (script_files.begin (), script_files.end (),
		 sort_scripts_by_name);
    }

  if (pspace_info != NULL && pspace_info->loaded_script_texts != NULL)
    {
      collect_matching_scripts_data data (&script_texts, language);

      /* Pass a pointer to scripts as VEC_safe_push can realloc space.  */
      htab_traverse_noresize (pspace_info->loaded_script_texts.get (),
			      collect_matching_scripts, &data);

      std::sort (script_texts.begin (), script_texts.end (),
		 sort_scripts_by_name);
    }

  int nr_scripts = script_files.size () + script_texts.size ();

  /* Table header shifted right by preceding "gdb-scripts:  " would not match
     its columns.  */
  if (nr_scripts > 0 && pattern == auto_load_info_scripts_pattern_nl)
    uiout->text ("\n");

  {
    ui_out_emit_table table_emitter (uiout, 2, nr_scripts,
				     "AutoLoadedScriptsTable");

    uiout->table_header (7, ui_left, "loaded", "Loaded");
    uiout->table_header (70, ui_left, "script", "Script");
    uiout->table_body ();

    print_scripts (script_files);
    print_scripts (script_texts);
  }

  if (nr_scripts == 0)
    {
      if (pattern && *pattern)
	uiout->message ("No auto-load scripts matching %s.\n", pattern);
      else
	uiout->message ("No auto-load scripts.\n");
    }
}

/* Wrapper for "info auto-load gdb-scripts".  */

static void
info_auto_load_gdb_scripts (const char *pattern, int from_tty)
{
  auto_load_info_scripts (current_program_space, pattern, from_tty,
			  &extension_language_gdb);
}

/* Implement 'info auto-load local-gdbinit'.  */

static void
info_auto_load_local_gdbinit (const char *args, int from_tty)
{
  if (auto_load_local_gdbinit_pathname == NULL)
    gdb_printf (_("Local .gdbinit file was not found.\n"));
  else if (auto_load_local_gdbinit_loaded)
    gdb_printf (_("Local .gdbinit file \"%ps\" has been loaded.\n"),
		styled_string (file_name_style.style (),
			       auto_load_local_gdbinit_pathname));
  else
    gdb_printf (_("Local .gdbinit file \"%ps\" has not been loaded.\n"),
		styled_string (file_name_style.style (),
			       auto_load_local_gdbinit_pathname));
}

/* Print an "unsupported script" warning if it has not already been printed.
   The script is in language LANGUAGE at offset OFFSET in section SECTION_NAME
   of OBJFILE.  */

static void
maybe_print_unsupported_script_warning
  (struct auto_load_pspace_info *pspace_info,
   struct objfile *objfile, const struct extension_language_defn *language,
   const char *section_name, unsigned offset)
{
  if (!pspace_info->unsupported_script_warning_printed)
    {
      warning (_("\
Unsupported auto-load script at offset %u in section %s\n\
of file %ps.\n\
Use `info auto-load %s-scripts [REGEXP]' to list them."),
	       offset, section_name,
	       styled_string (file_name_style.style (),
			      objfile_name (objfile)),
	       ext_lang_name (language));
      pspace_info->unsupported_script_warning_printed = true;
    }
}

/* Return non-zero if SCRIPT_NOT_FOUND_WARNING_PRINTED of PSPACE_INFO was unset
   before calling this function.  Always set SCRIPT_NOT_FOUND_WARNING_PRINTED
   of PSPACE_INFO.  */

static void
maybe_print_script_not_found_warning
  (struct auto_load_pspace_info *pspace_info,
   struct objfile *objfile, const struct extension_language_defn *language,
   const char *section_name, unsigned offset)
{
  if (!pspace_info->script_not_found_warning_printed)
    {
      warning (_("\
Missing auto-load script at offset %u in section %s\n\
of file %ps.\n\
Use `info auto-load %s-scripts [REGEXP]' to list them."),
	       offset, section_name,
	       styled_string (file_name_style.style (),
			      objfile_name (objfile)),
	       ext_lang_name (language));
      pspace_info->script_not_found_warning_printed = true;
    }
}

/* The only valid "set auto-load" argument is off|0|no|disable.  */

static void
set_auto_load_cmd (const char *args, int from_tty)
{
  struct cmd_list_element *list;
  size_t length;

  /* See parse_binary_operation in use by the sub-commands.  */

  length = args ? strlen (args) : 0;

  while (length > 0 && (args[length - 1] == ' ' || args[length - 1] == '\t'))
    length--;

  if (length == 0 || (strncmp (args, "off", length) != 0
		      && strncmp (args, "0", length) != 0
		      && strncmp (args, "no", length) != 0
		      && strncmp (args, "disable", length) != 0))
    error (_("Valid is only global 'set auto-load no'; "
	     "otherwise check the auto-load sub-commands."));

  for (list = *auto_load_set_cmdlist_get (); list != NULL; list = list->next)
    if (list->var->type () == var_boolean)
      {
	gdb_assert (list->type == set_cmd);
	do_set_command (args, from_tty, list);
      }
}

/* Initialize "set auto-load " commands prefix and return it.  */

struct cmd_list_element **
auto_load_set_cmdlist_get (void)
{
  static struct cmd_list_element *retval;

  if (retval == NULL)
    add_prefix_cmd ("auto-load", class_maintenance, set_auto_load_cmd, _("\
Auto-loading specific settings.\n\
Configure various auto-load-specific variables such as\n\
automatic loading of Python scripts."),
		    &retval, 1/*allow-unknown*/, &setlist);

  return &retval;
}

/* Initialize "show auto-load " commands prefix and return it.  */

struct cmd_list_element **
auto_load_show_cmdlist_get (void)
{
  static struct cmd_list_element *retval;

  if (retval == NULL)
    add_show_prefix_cmd ("auto-load", class_maintenance, _("\
Show auto-loading specific settings.\n\
Show configuration of various auto-load-specific variables such as\n\
automatic loading of Python scripts."),
			 &retval, 0/*allow-unknown*/, &showlist);

  return &retval;
}

/* Command "info auto-load" displays whether the various auto-load files have
   been loaded.  This is reimplementation of cmd_show_list which inserts
   newlines at proper places.  */

static void
info_auto_load_cmd (const char *args, int from_tty)
{
  struct cmd_list_element *list;
  struct ui_out *uiout = current_uiout;

  ui_out_emit_tuple tuple_emitter (uiout, "infolist");

  for (list = *auto_load_info_cmdlist_get (); list != NULL; list = list->next)
    {
      ui_out_emit_tuple option_emitter (uiout, "option");

      gdb_assert (!list->is_prefix ());
      gdb_assert (list->type == not_set_cmd);

      uiout->field_string ("name", list->name);
      uiout->text (":  ");
      cmd_func (list, auto_load_info_scripts_pattern_nl, from_tty);
    }
}

/* Initialize "info auto-load " commands prefix and return it.  */

struct cmd_list_element **
auto_load_info_cmdlist_get (void)
{
  static struct cmd_list_element *retval;

  if (retval == NULL)
    add_prefix_cmd ("auto-load", class_info, info_auto_load_cmd, _("\
Print current status of auto-loaded files.\n\
Print whether various files like Python scripts or .gdbinit files have been\n\
found and/or loaded."),
		    &retval, 0/*allow-unknown*/, &infolist);

  return &retval;
}

/* See auto-load.h.  */

gdb::observers::token auto_load_new_objfile_observer_token;

void _initialize_auto_load ();
void
_initialize_auto_load ()
{
  struct cmd_list_element *cmd;
  gdb::unique_xmalloc_ptr<char> scripts_directory_help, gdb_name_help,
    python_name_help, guile_name_help;
  const char *suffix;

  gdb::observers::new_objfile.attach (load_auto_scripts_for_objfile,
				      auto_load_new_objfile_observer_token,
				      "auto-load");
  gdb::observers::all_objfiles_removed.attach (clear_section_scripts,
					       "auto-load");
  add_setshow_boolean_cmd ("gdb-scripts", class_support,
			   &auto_load_gdb_scripts, _("\
Enable or disable auto-loading of canned sequences of commands scripts."), _("\
Show whether auto-loading of canned sequences of commands scripts is enabled."),
			   _("\
If enabled, canned sequences of commands are loaded when the debugger reads\n\
an executable or shared library.\n\
This option has security implications for untrusted inferiors."),
			   NULL, show_auto_load_gdb_scripts,
			   auto_load_set_cmdlist_get (),
			   auto_load_show_cmdlist_get ());

  add_cmd ("gdb-scripts", class_info, info_auto_load_gdb_scripts,
	   _("Print the list of automatically loaded sequences of commands.\n\
Usage: info auto-load gdb-scripts [REGEXP]"),
	   auto_load_info_cmdlist_get ());

  add_setshow_boolean_cmd ("local-gdbinit", class_support,
			   &auto_load_local_gdbinit, _("\
Enable or disable auto-loading of .gdbinit script in current directory."), _("\
Show whether auto-loading .gdbinit script in current directory is enabled."),
			   _("\
If enabled, canned sequences of commands are loaded when debugger starts\n\
from .gdbinit file in current directory.  Such files are deprecated,\n\
use a script associated with inferior executable file instead.\n\
This option has security implications for untrusted inferiors."),
			   NULL, show_auto_load_local_gdbinit,
			   auto_load_set_cmdlist_get (),
			   auto_load_show_cmdlist_get ());

  add_cmd ("local-gdbinit", class_info, info_auto_load_local_gdbinit,
	   _("Print whether current directory .gdbinit file has been loaded.\n\
Usage: info auto-load local-gdbinit"),
	   auto_load_info_cmdlist_get ());

  suffix = ext_lang_auto_load_suffix (get_ext_lang_defn (EXT_LANG_GDB));
  gdb_name_help
    = xstrprintf (_("\
GDB scripts:    OBJFILE%s\n"),
		  suffix);
  python_name_help = NULL;
#ifdef HAVE_PYTHON
  suffix = ext_lang_auto_load_suffix (get_ext_lang_defn (EXT_LANG_PYTHON));
  python_name_help
    = xstrprintf (_("\
Python scripts: OBJFILE%s\n"),
		  suffix);
#endif
  guile_name_help = NULL;
#ifdef HAVE_GUILE
  suffix = ext_lang_auto_load_suffix (get_ext_lang_defn (EXT_LANG_GUILE));
  guile_name_help
    = xstrprintf (_("\
Guile scripts:  OBJFILE%s\n"),
		  suffix);
#endif
  scripts_directory_help
    = xstrprintf (_("\
Automatically loaded scripts are located in one of the directories listed\n\
by this option.\n\
\n\
Script names:\n\
%s%s%s\
\n\
This option is ignored for the kinds of scripts \
having 'set auto-load ... off'.\n\
Directories listed here need to be present also \
in the 'set auto-load safe-path'\n\
option."),
		  gdb_name_help.get (),
		  python_name_help.get () ? python_name_help.get () : "",
		  guile_name_help.get () ? guile_name_help.get () : "");

  add_setshow_optional_filename_cmd ("scripts-directory", class_support,
				     &auto_load_dir, _("\
Set the list of directories from which to load auto-loaded scripts."), _("\
Show the list of directories from which to load auto-loaded scripts."),
				     scripts_directory_help.get (),
				     set_auto_load_dir, show_auto_load_dir,
				     auto_load_set_cmdlist_get (),
				     auto_load_show_cmdlist_get ());
  auto_load_safe_path_vec_update ();
  add_setshow_optional_filename_cmd ("safe-path", class_support,
				     &auto_load_safe_path, _("\
Set the list of files and directories that are safe for auto-loading."), _("\
Show the list of files and directories that are safe for auto-loading."), _("\
Various files loaded automatically for the 'set auto-load ...' options must\n\
be located in one of the directories listed by this option.  Warning will be\n\
printed and file will not be used otherwise.\n\
You can mix both directory and filename entries.\n\
Setting this parameter to an empty list resets it to its default value.\n\
Setting this parameter to '/' (without the quotes) allows any file\n\
for the 'set auto-load ...' options.  Each path entry can be also shell\n\
wildcard pattern; '*' does not match directory separator.\n\
This option is ignored for the kinds of files having 'set auto-load ... off'.\n\
This option has security implications for untrusted inferiors."),
				     set_auto_load_safe_path,
				     show_auto_load_safe_path,
				     auto_load_set_cmdlist_get (),
				     auto_load_show_cmdlist_get ());
  gdb::observers::gdb_datadir_changed.attach (auto_load_gdb_datadir_changed,
					      "auto-load");

  cmd = add_cmd ("add-auto-load-safe-path", class_support,
		 add_auto_load_safe_path,
		 _("Add entries to the list of directories from which it is safe "
		   "to auto-load files.\n\
See the commands 'set auto-load safe-path' and 'show auto-load safe-path' to\n\
access the current full list setting."),
		 &cmdlist);
  set_cmd_completer (cmd, filename_completer);

  cmd = add_cmd ("add-auto-load-scripts-directory", class_support,
		 add_auto_load_dir,
		 _("Add entries to the list of directories from which to load "
		   "auto-loaded scripts.\n\
See the commands 'set auto-load scripts-directory' and\n\
'show auto-load scripts-directory' to access the current full list setting."),
		 &cmdlist);
  set_cmd_completer (cmd, filename_completer);

  add_setshow_boolean_cmd ("auto-load", class_maintenance,
			   &debug_auto_load, _("\
Set auto-load verifications debugging."), _("\
Show auto-load verifications debugging."), _("\
When non-zero, debugging output for files of 'set auto-load ...'\n\
is displayed."),
			    NULL, show_debug_auto_load,
			    &setdebuglist, &showdebuglist);
}
