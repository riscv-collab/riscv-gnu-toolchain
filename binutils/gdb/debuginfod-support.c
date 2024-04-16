/* debuginfod utilities for GDB.
   Copyright (C) 2020-2024 Free Software Foundation, Inc.

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
#include "diagnostics.h"
#include <errno.h>
#include "gdbsupport/scoped_fd.h"
#include "debuginfod-support.h"
#include <optional>
#include "cli/cli-cmds.h"
#include "cli/cli-style.h"
#include "cli-out.h"
#include "target.h"

/* Set/show debuginfod commands.  */
static cmd_list_element *set_debuginfod_prefix_list;
static cmd_list_element *show_debuginfod_prefix_list;

/* maint set/show debuginfod commands.  */
static cmd_list_element *maint_set_debuginfod_cmdlist;
static cmd_list_element *maint_show_debuginfod_cmdlist;

static const char debuginfod_on[] = "on";
static const char debuginfod_off[] = "off";
static const char debuginfod_ask[] = "ask";

static const char *debuginfod_enabled_enum[] =
{
  debuginfod_on,
  debuginfod_off,
  debuginfod_ask,
  nullptr
};

static const char *debuginfod_enabled =
#if defined(HAVE_LIBDEBUGINFOD)
  debuginfod_ask;
#else
  debuginfod_off;
#endif

/* Controls whether ELF/DWARF section downloading is enabled.  */
static bool debuginfod_download_sections =
#if defined(HAVE_LIBDEBUGINFOD_FIND_SECTION)
  true;
#else
  false;
#endif

static unsigned int debuginfod_verbose = 1;

#ifndef HAVE_LIBDEBUGINFOD
scoped_fd
debuginfod_source_query (const unsigned char *build_id,
			 int build_id_len,
			 const char *srcpath,
			 gdb::unique_xmalloc_ptr<char> *destname)
{
  return scoped_fd (-ENOSYS);
}

scoped_fd
debuginfod_debuginfo_query (const unsigned char *build_id,
			    int build_id_len,
			    const char *filename,
			    gdb::unique_xmalloc_ptr<char> *destname)
{
  return scoped_fd (-ENOSYS);
}

scoped_fd
debuginfod_exec_query (const unsigned char *build_id,
		       int build_id_len,
		       const char *filename,
		       gdb::unique_xmalloc_ptr<char> *destname)
{
  return scoped_fd (-ENOSYS);
}

scoped_fd
debuginfod_section_query (const unsigned char *build_id,
			  int build_id_len,
			  const char *filename,
			  const char *section_name,
			  gdb::unique_xmalloc_ptr<char> *destname)
{
  return scoped_fd (-ENOSYS);
}
#define NO_IMPL _("Support for debuginfod is not compiled into GDB.")

#else
#include <elfutils/debuginfod.h>

struct user_data
{
  user_data (const char *desc, const char *fname)
    : desc (desc), fname (fname)
  { }

  const char * const desc;
  const char * const fname;
  ui_out::progress_update progress;
};

/* Convert SIZE into a unit suitable for use with progress updates.
   SIZE should in given in bytes and will be converted into KB, MB, GB
   or remain unchanged. UNIT will be set to "B", "KB", "MB" or "GB"
   accordingly.  */

static const char *
get_size_and_unit (double &size)
{
  if (size < 1024)
    /* If size is less than 1 KB then set unit to B.  */
    return "B";

  size /= 1024;
  if (size < 1024)
    /* If size is less than 1 MB then set unit to KB.  */
    return "K";

  size /= 1024;
  if (size < 1024)
    /* If size is less than 1 GB then set unit to MB.  */
    return "M";

  size /= 1024;
  return "G";
}

static int
progressfn (debuginfod_client *c, long cur, long total)
{
  user_data *data = static_cast<user_data *> (debuginfod_get_user_data (c));
  gdb_assert (data != nullptr);

  string_file styled_fname (current_uiout->can_emit_style_escape ());
  fprintf_styled (&styled_fname, file_name_style.style (), "%s",
		  data->fname);

  if (check_quit_flag ())
    {
      gdb_printf ("Cancelling download of %s %s...\n",
		  data->desc, styled_fname.c_str ());
      return 1;
    }

  if (debuginfod_verbose == 0)
    return 0;

  /* Print progress update.  Include the transfer size if available.  */
  if (total > 0)
    {
      /* Transfer size is known.  */
      double howmuch = (double) cur / (double) total;

      if (howmuch >= 0.0 && howmuch <= 1.0)
	{
	  double d_total = (double) total;
	  const char *unit =  get_size_and_unit (d_total);
	  std::string msg = string_printf ("Downloading %0.2f %s %s %s",
					   d_total, unit, data->desc,
					   styled_fname.c_str ());
	  data->progress.update_progress (msg, unit, howmuch, d_total);
	  return 0;
	}
    }

  std::string msg = string_printf ("Downloading %s %s",
				   data->desc, styled_fname.c_str ());
  data->progress.update_progress (msg);
  return 0;
}

/* Cleanup ARG, which is a debuginfod_client pointer.  */

static void
cleanup_debuginfod_client (void *arg)
{
  debuginfod_client *client = static_cast<debuginfod_client *> (arg);
  debuginfod_end (client);
}

/* Return a pointer to the single global debuginfod_client, initialising it
   first if needed.  */

static debuginfod_client *
get_debuginfod_client ()
{
  static debuginfod_client *global_client = nullptr;

  if (global_client == nullptr)
    {
      global_client = debuginfod_begin ();

      if (global_client != nullptr)
	{
	  /* It is important that we cleanup the debuginfod_client object
	     before calling exit.  Some of the libraries used by debuginfod
	     make use of at_exit handlers to perform cleanup.

	     If we wrapped the debuginfod_client in a unique_ptr and relied
	     on its destructor to cleanup then this would be run as part of
	     the global C++ object destructors, which is after the at_exit
	     handlers, which is too late.

	     So instead, we make use of GDB's final cleanup mechanism.  */
	  make_final_cleanup (cleanup_debuginfod_client, global_client);
	  debuginfod_set_progressfn (global_client, progressfn);
	}
    }

  return global_client;
}

/* Check if debuginfod is enabled.  If configured to do so, ask the user
   whether to enable debuginfod.  */

static bool
debuginfod_is_enabled ()
{
  const char *urls = skip_spaces (getenv (DEBUGINFOD_URLS_ENV_VAR));

  if (debuginfod_enabled == debuginfod_off
      || urls == nullptr
      || *urls == '\0')
    return false;

  if (debuginfod_enabled == debuginfod_ask)
    {
      gdb_printf (_("\nThis GDB supports auto-downloading debuginfo " \
		    "from the following URLs:\n"));

      std::string_view url_view (urls);
      while (true)
	{
	  size_t off = url_view.find_first_not_of (' ');
	  if (off == std::string_view::npos)
	    break;
	  url_view = url_view.substr (off);
	  /* g++ 11.2.1 on s390x, g++ 11.3.1 on ppc64le and g++ 11 on
	     hppa seem convinced url_view might be of SIZE_MAX length.
	     And so complains because the length of an array can only
	     be PTRDIFF_MAX.  */
	  DIAGNOSTIC_PUSH
	  DIAGNOSTIC_IGNORE_STRINGOP_OVERREAD
	  off = url_view.find_first_of (' ');
	  DIAGNOSTIC_POP
	  gdb_printf
	    (_("  <%ps>\n"),
	     styled_string (file_name_style.style (),
			    std::string (url_view.substr (0, off)).c_str ()));
	  if (off == std::string_view::npos)
	    break;
	  url_view = url_view.substr (off);
	}

      int resp = nquery (_("Enable debuginfod for this session? "));
      if (!resp)
	{
	  gdb_printf (_("Debuginfod has been disabled.\nTo make this " \
			"setting permanent, add \'set debuginfod " \
			"enabled off\' to .gdbinit.\n"));
	  debuginfod_enabled = debuginfod_off;
	  return false;
	}

      gdb_printf (_("Debuginfod has been enabled.\nTo make this " \
		    "setting permanent, add \'set debuginfod enabled " \
		    "on\' to .gdbinit.\n"));
      debuginfod_enabled = debuginfod_on;
    }

  return true;
}

/* Print the result of the most recent attempted download.  */

static void
print_outcome (int fd, const char *desc, const char *fname)
{
  if (fd < 0 && fd != -ENOENT)
    gdb_printf (_("Download failed: %s.  Continuing without %s %ps.\n"),
		safe_strerror (-fd),
		desc,
		styled_string (file_name_style.style (), fname));
}

/* See debuginfod-support.h  */

scoped_fd
debuginfod_source_query (const unsigned char *build_id,
			 int build_id_len,
			 const char *srcpath,
			 gdb::unique_xmalloc_ptr<char> *destname)
{
  if (!debuginfod_is_enabled ())
    return scoped_fd (-ENOSYS);

  debuginfod_client *c = get_debuginfod_client ();

  if (c == nullptr)
    return scoped_fd (-ENOMEM);

  char *dname = nullptr;
  scoped_fd fd;
  std::optional<target_terminal::scoped_restore_terminal_state> term_state;

  {
    user_data data ("source file", srcpath);

    debuginfod_set_user_data (c, &data);
    if (target_supports_terminal_ours ())
      {
	term_state.emplace ();
	target_terminal::ours ();
      }

    fd = scoped_fd (debuginfod_find_source (c,
					    build_id,
					    build_id_len,
					    srcpath,
					    &dname));
    debuginfod_set_user_data (c, nullptr);
  }

  print_outcome (fd.get (), "source file", srcpath);

  if (fd.get () >= 0)
    destname->reset (dname);

  return fd;
}

/* See debuginfod-support.h  */

scoped_fd
debuginfod_debuginfo_query (const unsigned char *build_id,
			    int build_id_len,
			    const char *filename,
			    gdb::unique_xmalloc_ptr<char> *destname)
{
  if (!debuginfod_is_enabled ())
    return scoped_fd (-ENOSYS);

  debuginfod_client *c = get_debuginfod_client ();

  if (c == nullptr)
    return scoped_fd (-ENOMEM);

  char *dname = nullptr;
  scoped_fd fd;
  std::optional<target_terminal::scoped_restore_terminal_state> term_state;

  {
    user_data data ("separate debug info for", filename);

    debuginfod_set_user_data (c, &data);
    if (target_supports_terminal_ours ())
      {
	term_state.emplace ();
	target_terminal::ours ();
      }

    fd = scoped_fd (debuginfod_find_debuginfo (c, build_id, build_id_len,
					       &dname));
    debuginfod_set_user_data (c, nullptr);
  }

  print_outcome (fd.get (), "separate debug info for", filename);

  if (fd.get () >= 0)
    destname->reset (dname);

  return fd;
}

/* See debuginfod-support.h  */

scoped_fd
debuginfod_exec_query (const unsigned char *build_id,
		       int build_id_len,
		       const char *filename,
		       gdb::unique_xmalloc_ptr<char> *destname)
{
  if (!debuginfod_is_enabled ())
    return scoped_fd (-ENOSYS);

  debuginfod_client *c = get_debuginfod_client ();

  if (c == nullptr)
    return scoped_fd (-ENOMEM);

  char *dname = nullptr;
  scoped_fd fd;
  std::optional<target_terminal::scoped_restore_terminal_state> term_state;

  {
    user_data data ("executable for", filename);

    debuginfod_set_user_data (c, &data);
    if (target_supports_terminal_ours ())
      {
	term_state.emplace ();
	target_terminal::ours ();
      }

    fd = scoped_fd (debuginfod_find_executable (c, build_id, build_id_len,
						&dname));
    debuginfod_set_user_data (c, nullptr);
  }

  print_outcome (fd.get (), "executable for", filename);

  if (fd.get () >= 0)
    destname->reset (dname);

  return fd;
}

/* See debuginfod-support.h  */

scoped_fd
debuginfod_section_query (const unsigned char *build_id,
			  int build_id_len,
			  const char *filename,
			  const char *section_name,
			  gdb::unique_xmalloc_ptr<char> *destname)
{
#if !defined (HAVE_LIBDEBUGINFOD_FIND_SECTION)
  return scoped_fd (-ENOSYS);
#else

  if (!debuginfod_download_sections || !debuginfod_is_enabled ())
    return scoped_fd (-ENOSYS);

  debuginfod_client *c = get_debuginfod_client ();

  if (c == nullptr)
    return scoped_fd (-ENOMEM);

  char *dname = nullptr;
  std::string desc = std::string ("section ") + section_name + " for";
  scoped_fd fd;
  std::optional<target_terminal::scoped_restore_terminal_state> term_state;

  {
    user_data data (desc.c_str (), filename);
    debuginfod_set_user_data (c, &data);
    if (target_supports_terminal_ours ())
      {
	term_state.emplace ();
	target_terminal::ours ();
      }

    fd = scoped_fd (debuginfod_find_section (c, build_id, build_id_len,
					     section_name, &dname));
    debuginfod_set_user_data (c, nullptr);
  }

  print_outcome (fd.get (), desc.c_str (), filename);
  gdb_assert (destname != nullptr);

  if (fd.get () >= 0)
    destname->reset (dname);

  return fd;
#endif /* HAVE_LIBDEBUGINFOD_FIND_SECTION */
}

#endif

/* Set callback for "set debuginfod enabled".  */

static void
set_debuginfod_enabled (const char *value)
{
#if defined(HAVE_LIBDEBUGINFOD)
  debuginfod_enabled = value;
#else
  /* Disabling debuginfod when gdb is not built with it is a no-op.  */
  if (value != debuginfod_off)
    error (NO_IMPL);
#endif
}

/* Get callback for "set debuginfod enabled".  */

static const char *
get_debuginfod_enabled ()
{
  return debuginfod_enabled;
}

/* Show callback for "set debuginfod enabled".  */

static void
show_debuginfod_enabled (ui_file *file, int from_tty, cmd_list_element *cmd,
			 const char *value)
{
  gdb_printf (file,
	      _("Debuginfod functionality is currently set to "
		"\"%s\".\n"), debuginfod_enabled);
}

/* Set callback for "set debuginfod urls".  */

static void
set_debuginfod_urls (const std::string &urls)
{
#if defined(HAVE_LIBDEBUGINFOD)
  if (setenv (DEBUGINFOD_URLS_ENV_VAR, urls.c_str (), 1) != 0)
    warning (_("Unable to set debuginfod URLs: %s"), safe_strerror (errno));
#else
  error (NO_IMPL);
#endif
}

/* Get callback for "set debuginfod urls".  */

static const std::string&
get_debuginfod_urls ()
{
  static std::string urls;
#if defined(HAVE_LIBDEBUGINFOD)
  const char *envvar = getenv (DEBUGINFOD_URLS_ENV_VAR);

  if (envvar != nullptr)
    urls = envvar;
  else
    urls.clear ();
#endif

  return urls;
}

/* Show callback for "set debuginfod urls".  */

static void
show_debuginfod_urls (ui_file *file, int from_tty, cmd_list_element *cmd,
		      const char *value)
{
  if (value[0] == '\0')
    gdb_printf (file, _("Debuginfod URLs have not been set.\n"));
  else
    gdb_printf (file, _("Debuginfod URLs are currently set to:\n%s\n"),
		value);
}

/* Show callback for "set debuginfod verbose".  */

static void
show_debuginfod_verbose_command (ui_file *file, int from_tty,
				 cmd_list_element *cmd, const char *value)
{
  gdb_printf (file, _("Debuginfod verbose output is set to %s.\n"),
	      value);
}

/* Set callback for "maint set debuginfod download-sections".  */

static void
maint_set_debuginfod_download_sections (bool value)
{
#if !defined(HAVE_LIBDEBUGINFOD_FIND_SECTION)
  if (value)
    error (_("Support for section downloading is not compiled into GDB. " \
"Defaulting to \"off\"."));
#endif

  debuginfod_download_sections = value;
}

/* Get callback for "maint set debuginfod download-sections".  */

static bool
maint_get_debuginfod_download_sections ()
{
  return debuginfod_download_sections;
}

/* Register debuginfod commands.  */

void _initialize_debuginfod ();
void
_initialize_debuginfod ()
{
  /* set/show debuginfod */
  add_setshow_prefix_cmd ("debuginfod", class_run,
			  _("Set debuginfod options."),
			  _("Show debuginfod options."),
			  &set_debuginfod_prefix_list,
			  &show_debuginfod_prefix_list,
			  &setlist, &showlist);

  add_setshow_enum_cmd ("enabled", class_run, debuginfod_enabled_enum,
			_("Set whether to use debuginfod."),
			_("Show whether to use debuginfod."),
			_("\
When set to \"on\", enable the use of debuginfod to download missing\n\
debug info and source files.  GDB may also download components of debug\n\
info instead of entire files.  \"off\" disables the use of debuginfod.\n\
When set to \"ask\", prompt whether to enable or disable debuginfod." ),
			set_debuginfod_enabled,
			get_debuginfod_enabled,
			show_debuginfod_enabled,
			&set_debuginfod_prefix_list,
			&show_debuginfod_prefix_list);

  /* set/show debuginfod urls */
  add_setshow_string_noescape_cmd ("urls", class_run, _("\
Set the list of debuginfod server URLs."), _("\
Show the list of debuginfod server URLs."), _("\
Manage the space-separated list of debuginfod server URLs that GDB will query \
when missing debuginfo, executables or source files.\nThe default value is \
copied from the DEBUGINFOD_URLS environment variable."),
				   set_debuginfod_urls,
				   get_debuginfod_urls,
				   show_debuginfod_urls,
				   &set_debuginfod_prefix_list,
				   &show_debuginfod_prefix_list);

  /* set/show debuginfod verbose */
  add_setshow_zuinteger_cmd ("verbose", class_support,
			     &debuginfod_verbose, _("\
Set verbosity of debuginfod output."), _("\
Show debuginfod debugging."), _("\
When set to a non-zero value, display verbose output for each debuginfod \
query.\nTo disable, set to zero.  Verbose output is displayed by default."),
			     nullptr,
			     show_debuginfod_verbose_command,
			     &set_debuginfod_prefix_list,
			     &show_debuginfod_prefix_list);

  /* maint set/show debuginfod.  */
  add_setshow_prefix_cmd ("debuginfod", class_maintenance,
			  _("Set debuginfod specific variables."),
			  _("Show debuginfod specific variables."),
			  &maint_set_debuginfod_cmdlist,
			  &maint_show_debuginfod_cmdlist,
			  &maintenance_set_cmdlist, &maintenance_show_cmdlist);

  /* maint set/show debuginfod download-sections.  */
  add_setshow_boolean_cmd ("download-sections", class_maintenance, _("\
Set whether debuginfod may download individual ELF/DWARF sections."), _("\
Show whether debuginfod may download individual ELF/DWARF sections."), _("\
When enabled, debuginfod may attempt to download individual ELF/DWARF \
sections from debug info files.\nIf disabled, only whole debug info files \
may be downloaded."),
			   maint_set_debuginfod_download_sections,
			   maint_get_debuginfod_download_sections,
			   nullptr,
			   &maint_set_debuginfod_cmdlist,
			   &maint_show_debuginfod_cmdlist);
}
