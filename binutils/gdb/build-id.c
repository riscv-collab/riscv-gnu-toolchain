/* build-id-related functions.

   Copyright (C) 1991-2024 Free Software Foundation, Inc.

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
#include "bfd.h"
#include "gdb_bfd.h"
#include "build-id.h"
#include "gdbsupport/gdb_vecs.h"
#include "symfile.h"
#include "objfiles.h"
#include "filenames.h"
#include "gdbcore.h"
#include "cli/cli-style.h"

/* See build-id.h.  */

const struct bfd_build_id *
build_id_bfd_get (bfd *abfd)
{
  /* Dynamic objfiles such as ones created by JIT reader API
     have no underlying bfd structure (that is, objfile->obfd
     is NULL).  */
  if (abfd == nullptr)
    return nullptr;

  if (!bfd_check_format (abfd, bfd_object)
      && !bfd_check_format (abfd, bfd_core))
    return NULL;

  if (abfd->build_id != NULL)
    return abfd->build_id;

  /* No build-id */
  return NULL;
}

/* See build-id.h.  */

int
build_id_verify (bfd *abfd, size_t check_len, const bfd_byte *check)
{
  const struct bfd_build_id *found;
  int retval = 0;

  found = build_id_bfd_get (abfd);

  if (found == NULL)
    warning (_("File \"%s\" has no build-id, file skipped"),
	     bfd_get_filename (abfd));
  else if (found->size != check_len
	   || memcmp (found->data, check, found->size) != 0)
    warning (_("File \"%s\" has a different build-id, file skipped"),
	     bfd_get_filename (abfd));
  else
    retval = 1;

  return retval;
}

/* Helper for build_id_to_debug_bfd.  LINK is a path to a potential
   build-id-based separate debug file, potentially a symlink to the real file.
   If the file exists and matches BUILD_ID, return a BFD reference to it.  */

static gdb_bfd_ref_ptr
build_id_to_debug_bfd_1 (const std::string &link, size_t build_id_len,
			 const bfd_byte *build_id)
{
  if (separate_debug_file_debug)
    {
      gdb_printf (gdb_stdlog, _("  Trying %s..."), link.c_str ());
      gdb_flush (gdb_stdlog);
    }

  /* lrealpath() is expensive even for the usually non-existent files.  */
  gdb::unique_xmalloc_ptr<char> filename_holder;
  const char *filename = nullptr;
  if (is_target_filename (link))
    filename = link.c_str ();
  else if (access (link.c_str (), F_OK) == 0)
    {
      filename_holder.reset (lrealpath (link.c_str ()));
      filename = filename_holder.get ();
    }

  if (filename == NULL)
    {
      if (separate_debug_file_debug)
	gdb_printf (gdb_stdlog,
		    _(" no, unable to compute real path\n"));

      return {};
    }

  /* We expect to be silent on the non-existing files.  */
  gdb_bfd_ref_ptr debug_bfd = gdb_bfd_open (filename, gnutarget);

  if (debug_bfd == NULL)
    {
      if (separate_debug_file_debug)
	gdb_printf (gdb_stdlog, _(" no, unable to open.\n"));

      return {};
    }

  if (!build_id_verify (debug_bfd.get(), build_id_len, build_id))
    {
      if (separate_debug_file_debug)
	gdb_printf (gdb_stdlog, _(" no, build-id does not match.\n"));

      return {};
    }

  if (separate_debug_file_debug)
    gdb_printf (gdb_stdlog, _(" yes!\n"));

  return debug_bfd;
}

/* Common code for finding BFDs of a given build-id.  This function
   works with both debuginfo files (SUFFIX == ".debug") and executable
   files (SUFFIX == "").  */

static gdb_bfd_ref_ptr
build_id_to_bfd_suffix (size_t build_id_len, const bfd_byte *build_id,
			const char *suffix)
{
  /* Keep backward compatibility so that DEBUG_FILE_DIRECTORY being "" will
     cause "/.build-id/..." lookups.  */

  std::vector<gdb::unique_xmalloc_ptr<char>> debugdir_vec
    = dirnames_to_char_ptr_vec (debug_file_directory.c_str ());

  for (const gdb::unique_xmalloc_ptr<char> &debugdir : debugdir_vec)
    {
      const gdb_byte *data = build_id;
      size_t size = build_id_len;

      /* Compute where the file named after the build-id would be.

	 If debugdir is "/usr/lib/debug" and the build-id is abcdef, this will
	 give "/usr/lib/debug/.build-id/ab/cdef.debug".  */
      std::string link = debugdir.get ();
      link += "/.build-id/";

      if (size > 0)
	{
	  size--;
	  string_appendf (link, "%02x/", (unsigned) *data++);
	}

      while (size-- > 0)
	string_appendf (link, "%02x", (unsigned) *data++);

      link += suffix;

      gdb_bfd_ref_ptr debug_bfd
	= build_id_to_debug_bfd_1 (link, build_id_len, build_id);
      if (debug_bfd != NULL)
	return debug_bfd;

      /* Try to look under the sysroot as well.  If the sysroot is
	 "/the/sysroot", it will give
	 "/the/sysroot/usr/lib/debug/.build-id/ab/cdef.debug".  */

      if (!gdb_sysroot.empty ())
	{
	  link = gdb_sysroot + link;
	  debug_bfd = build_id_to_debug_bfd_1 (link, build_id_len, build_id);
	  if (debug_bfd != NULL)
	    return debug_bfd;
	}
    }

  return {};
}

/* See build-id.h.  */

gdb_bfd_ref_ptr
build_id_to_debug_bfd (size_t build_id_len, const bfd_byte *build_id)
{
  return build_id_to_bfd_suffix (build_id_len, build_id, ".debug");
}

/* See build-id.h.  */

gdb_bfd_ref_ptr
build_id_to_exec_bfd (size_t build_id_len, const bfd_byte *build_id)
{
  return build_id_to_bfd_suffix (build_id_len, build_id, "");
}

/* See build-id.h.  */

std::string
find_separate_debug_file_by_buildid (struct objfile *objfile,
				     deferred_warnings *warnings)
{
  const struct bfd_build_id *build_id;

  build_id = build_id_bfd_get (objfile->obfd.get ());
  if (build_id != NULL)
    {
      if (separate_debug_file_debug)
	gdb_printf (gdb_stdlog,
		    _("\nLooking for separate debug info (build-id) for "
		      "%s\n"), objfile_name (objfile));

      gdb_bfd_ref_ptr abfd (build_id_to_debug_bfd (build_id->size,
						   build_id->data));
      /* Prevent looping on a stripped .debug file.  */
      if (abfd != NULL
	  && filename_cmp (bfd_get_filename (abfd.get ()),
			   objfile_name (objfile)) == 0)
	{
	  if (separate_debug_file_debug)
	    gdb_printf (gdb_stdlog, "\"%s\": separate debug info file has no "
			"debug info", bfd_get_filename (abfd.get ()));
	  warnings->warn (_("\"%ps\": separate debug info file has no "
			    "debug info"),
			  styled_string (file_name_style.style (),
					 bfd_get_filename (abfd.get ())));
	}
      else if (abfd != NULL)
	return std::string (bfd_get_filename (abfd.get ()));
    }

  return std::string ();
}
