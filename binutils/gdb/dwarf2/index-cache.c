/* Caching of GDB/DWARF index files.

   Copyright (C) 1994-2024 Free Software Foundation, Inc.

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
#include "dwarf2/index-cache.h"

#include "build-id.h"
#include "cli/cli-cmds.h"
#include "cli/cli-decode.h"
#include "command.h"
#include "gdbsupport/scoped_mmap.h"
#include "gdbsupport/pathstuff.h"
#include "dwarf2/index-write.h"
#include "dwarf2/read.h"
#include "dwarf2/dwz.h"
#include "objfiles.h"
#include "gdbsupport/selftest.h"
#include <string>
#include <stdlib.h>

/* When set to true, show debug messages about the index cache.  */
static bool debug_index_cache = false;

#define index_cache_debug(FMT, ...)					       \
  debug_prefixed_printf_cond_nofunc (debug_index_cache, "index-cache", \
				     FMT, ## __VA_ARGS__)

/* The index cache directory, used for "set/show index-cache directory".  */
static std::string index_cache_directory;

/* See dwarf-index.cache.h.  */
index_cache global_index_cache;

/* set/show index-cache commands.  */
static cmd_list_element *set_index_cache_prefix_list;
static cmd_list_element *show_index_cache_prefix_list;

/* Default destructor of index_cache_resource.  */
index_cache_resource::~index_cache_resource () = default;

/* See dwarf-index-cache.h.  */

void
index_cache::set_directory (std::string dir)
{
  gdb_assert (!dir.empty ());

  m_dir = std::move (dir);

  index_cache_debug ("now using directory %s", m_dir.c_str ());
}

/* See dwarf-index-cache.h.  */

void
index_cache::enable ()
{
  index_cache_debug ("enabling (%s)", m_dir.c_str ());

  m_enabled = true;
}

/* See dwarf-index-cache.h.  */

void
index_cache::disable ()
{
  index_cache_debug ("disabling");

  m_enabled = false;
}

 /* See index-cache.h.  */

index_cache_store_context::index_cache_store_context (const index_cache &ic,
						      dwarf2_per_bfd *per_bfd)
  :  m_enabled (ic.enabled ())
{
  if (!m_enabled)
    return;

  /* Get build id of objfile.  */
  const bfd_build_id *build_id = build_id_bfd_get (per_bfd->obfd);
  if (build_id == nullptr)
    {
      index_cache_debug ("objfile %s has no build id",
			 bfd_get_filename (per_bfd->obfd));
      m_enabled = false;
      return;
    }
  build_id_str = build_id_to_string (build_id);

  /* Get build id of dwz file, if present.  */
  const dwz_file *dwz = dwarf2_get_dwz_file (per_bfd);

  if (dwz != nullptr)
    {
      const bfd_build_id *dwz_build_id = build_id_bfd_get (dwz->dwz_bfd.get ());

      if (dwz_build_id == nullptr)
	{
	  index_cache_debug ("dwz objfile %s has no build id",
			     dwz->filename ());
	  m_enabled = false;
	  return;
	}

      dwz_build_id_str = build_id_to_string (dwz_build_id);
    }

  if (ic.m_dir.empty ())
    {
      warning (_("The index cache directory name is empty, skipping store."));
      m_enabled = false;
      return;
    }

  try
    {
      /* Try to create the containing directory.  */
      if (!mkdir_recursive (ic.m_dir.c_str ()))
	{
	  warning (_("index cache: could not make cache directory: %s"),
		   safe_strerror (errno));
	  m_enabled = false;
	  return;
	}
    }
  catch (const gdb_exception_error &except)
    {
      index_cache_debug ("couldn't store index cache for objfile %s: %s",
			 bfd_get_filename (per_bfd->obfd), except.what ());
      m_enabled = false;
    }
}

/* See dwarf-index-cache.h.  */

void
index_cache::store (dwarf2_per_bfd *per_bfd,
		    const index_cache_store_context &ctx)
{
  if (!ctx.m_enabled)
    return;

  const char *dwz_build_id_ptr = (ctx.dwz_build_id_str.has_value ()
				  ? ctx.dwz_build_id_str->c_str ()
				  : nullptr);

  try
    {
      index_cache_debug ("writing index cache for objfile %s",
			 bfd_get_filename (per_bfd->obfd));

      /* Write the index itself to the directory, using the build id as the
	 filename.  */
      write_dwarf_index (per_bfd, m_dir.c_str (),
			 ctx.build_id_str.c_str (), dwz_build_id_ptr,
			 dw_index_kind::GDB_INDEX);
    }
  catch (const gdb_exception_error &except)
    {
      index_cache_debug ("couldn't store index cache for objfile %s: %s",
			 bfd_get_filename (per_bfd->obfd), except.what ());
    }
}

#if HAVE_SYS_MMAN_H

/* Hold the resources for an mmapped index file.  */

struct index_cache_resource_mmap final : public index_cache_resource
{
  /* Try to mmap FILENAME.  Throw an exception on failure, including if the
     file doesn't exist. */
  index_cache_resource_mmap (const char *filename)
    : mapping (mmap_file (filename))
  {}

  scoped_mmap mapping;
};

/* See dwarf-index-cache.h.  */

gdb::array_view<const gdb_byte>
index_cache::lookup_gdb_index (const bfd_build_id *build_id,
			       std::unique_ptr<index_cache_resource> *resource)
{
  if (!enabled ())
    return {};

  if (m_dir.empty ())
    {
      warning (_("The index cache directory name is empty, skipping cache "
		 "lookup."));
      return {};
    }

  /* Compute where we would expect a gdb index file for this build id to be.  */
  std::string filename = make_index_filename (build_id, INDEX4_SUFFIX);

  try
    {
      index_cache_debug ("trying to read %s",
			 filename.c_str ());

      /* Try to map that file.  */
      index_cache_resource_mmap *mmap_resource
	= new index_cache_resource_mmap (filename.c_str ());

      /* Yay, it worked!  Hand the resource to the caller.  */
      resource->reset (mmap_resource);

      return gdb::array_view<const gdb_byte>
	  ((const gdb_byte *) mmap_resource->mapping.get (),
	   mmap_resource->mapping.size ());
    }
  catch (const gdb_exception_error &except)
    {
      index_cache_debug ("couldn't read %s: %s",
			 filename.c_str (), except.what ());
    }

  return {};
}

#else /* !HAVE_SYS_MMAN_H */

/* See dwarf-index-cache.h.  This is a no-op on unsupported systems.  */

gdb::array_view<const gdb_byte>
index_cache::lookup_gdb_index (const bfd_build_id *build_id,
			       std::unique_ptr<index_cache_resource> *resource)
{
  return {};
}

#endif

/* See dwarf-index-cache.h.  */

std::string
index_cache::make_index_filename (const bfd_build_id *build_id,
				  const char *suffix) const
{
  std::string build_id_str = build_id_to_string (build_id);

  return m_dir + SLASH_STRING + build_id_str + suffix;
}

/* True when we are executing "show index-cache".  This is used to improve the
   printout a little bit.  */
static bool in_show_index_cache_command = false;

/* "show index-cache" handler.  */

static void
show_index_cache_command (const char *arg, int from_tty)
{
  /* Note that we are executing "show index-cache".  */
  auto restore_flag = make_scoped_restore (&in_show_index_cache_command, true);

  /* Call all "show index-cache" subcommands.  */
  cmd_show_list (show_index_cache_prefix_list, from_tty);

  gdb_printf ("\n");
  gdb_printf
    (_("The index cache is currently %s.\n"),
     global_index_cache.enabled () ? _("enabled") : _("disabled"));
}

/* "set/show index-cache enabled" set callback.  */

static void
set_index_cache_enabled_command (bool value)
{
  if (value)
    global_index_cache.enable ();
  else
    global_index_cache.disable ();
}

/* "set/show index-cache enabled" get callback.  */

static bool
get_index_cache_enabled_command ()
{
  return global_index_cache.enabled ();
}

/* "set/show index-cache enabled" show callback.  */

static void
show_index_cache_enabled_command (ui_file *stream, int from_tty,
				  cmd_list_element *cmd, const char *value)
{
  gdb_printf (stream, _("The index cache is %s.\n"), value);
}

/* "set index-cache directory" handler.  */

static void
set_index_cache_directory_command (const char *arg, int from_tty,
				   cmd_list_element *element)
{
  /* Make sure the index cache directory is absolute and tilde-expanded.  */
  index_cache_directory = gdb_abspath (index_cache_directory.c_str ());
  global_index_cache.set_directory (index_cache_directory);
}

/* "show index-cache stats" handler.  */

static void
show_index_cache_stats_command (const char *arg, int from_tty)
{
  const char *indent = "";

  /* If this command is invoked through "show index-cache", make the display a
     bit nicer.  */
  if (in_show_index_cache_command)
    {
      indent = "  ";
      gdb_printf ("\n");
    }

  gdb_printf (_("%s  Cache hits (this session): %u\n"),
	      indent, global_index_cache.n_hits ());
  gdb_printf (_("%sCache misses (this session): %u\n"),
	      indent, global_index_cache.n_misses ());
}

void _initialize_index_cache ();
void
_initialize_index_cache ()
{
  /* Set the default index cache directory.  */
  std::string cache_dir = get_standard_cache_dir ();
  if (!cache_dir.empty ())
    {
      index_cache_directory = cache_dir;
      global_index_cache.set_directory (std::move (cache_dir));
    }
  else
    warning (_("Couldn't determine a path for the index cache directory."));

  /* set index-cache */
  add_basic_prefix_cmd ("index-cache", class_files,
			_("Set index-cache options."),
			&set_index_cache_prefix_list,
			false, &setlist);

  /* show index-cache */
  add_prefix_cmd ("index-cache", class_files, show_index_cache_command,
		  _("Show index-cache options."), &show_index_cache_prefix_list,
		  false, &showlist);

  /* set/show index-cache enabled */
  set_show_commands setshow_index_cache_enabled_cmds
    = add_setshow_boolean_cmd ("enabled", class_files,
			       _("Enable the index cache."),
			       _("Show whether the index cache is enabled."),
			       _("When on, enable the use of the index cache."),
			       set_index_cache_enabled_command,
			       get_index_cache_enabled_command,
			       show_index_cache_enabled_command,
			       &set_index_cache_prefix_list,
			       &show_index_cache_prefix_list);

  /* set index-cache on */
  cmd_list_element *set_index_cache_on_cmd
    = add_alias_cmd ("on", setshow_index_cache_enabled_cmds.set, class_files,
		     false, &set_index_cache_prefix_list);
  deprecate_cmd (set_index_cache_on_cmd, "set index-cache enabled on");
  set_index_cache_on_cmd->default_args = "on";

  /* set index-cache off */
  cmd_list_element *set_index_cache_off_cmd
    = add_alias_cmd ("off", setshow_index_cache_enabled_cmds.set, class_files,
		     false, &set_index_cache_prefix_list);
  deprecate_cmd (set_index_cache_off_cmd, "set index-cache enabled off");
  set_index_cache_off_cmd->default_args = "off";

  /* set index-cache directory */
  add_setshow_filename_cmd ("directory", class_files, &index_cache_directory,
			    _("Set the directory of the index cache."),
			    _("Show the directory of the index cache."),
			    NULL,
			    set_index_cache_directory_command, NULL,
			    &set_index_cache_prefix_list,
			    &show_index_cache_prefix_list);

  /* show index-cache stats */
  add_cmd ("stats", class_files, show_index_cache_stats_command,
	   _("Show some stats about the index cache."),
	   &show_index_cache_prefix_list);

  /* set debug index-cache */
  add_setshow_boolean_cmd ("index-cache", class_maintenance,
			   &debug_index_cache,
			   _("Set display of index-cache debug messages."),
			   _("Show display of index-cache debug messages."),
			   _("\
When non-zero, debugging output for the index cache is displayed."),
			    NULL, NULL,
			    &setdebuglist, &showdebuglist);
}
