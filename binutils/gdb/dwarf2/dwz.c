/* DWARF DWZ handling for GDB.

   Copyright (C) 2003-2024 Free Software Foundation, Inc.

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
#include "dwarf2/dwz.h"

#include "build-id.h"
#include "debuginfod-support.h"
#include "dwarf2/read.h"
#include "dwarf2/sect-names.h"
#include "filenames.h"
#include "gdb_bfd.h"
#include "gdbcore.h"
#include "gdbsupport/pathstuff.h"
#include "gdbsupport/scoped_fd.h"
#include "run-on-main-thread.h"

const char *
dwz_file::read_string (struct objfile *objfile, LONGEST str_offset)
{
  str.read (objfile);

  if (str.buffer == NULL)
    error (_("DW_FORM_GNU_strp_alt used without .debug_str "
	     "section [in module %s]"),
	   bfd_get_filename (dwz_bfd.get ()));
  if (str_offset >= str.size)
    error (_("DW_FORM_GNU_strp_alt pointing outside of "
	     ".debug_str section [in module %s]"),
	   bfd_get_filename (dwz_bfd.get ()));
  gdb_assert (HOST_CHAR_BIT == 8);
  if (str.buffer[str_offset] == '\0')
    return NULL;
  return (const char *) (str.buffer + str_offset);
}

/* A helper function to find the sections for a .dwz file.  */

static void
locate_dwz_sections (struct objfile *objfile, bfd *abfd, asection *sectp,
		     dwz_file *dwz_file)
{
  dwarf2_section_info *sect = nullptr;

  /* Note that we only support the standard ELF names, because .dwz
     is ELF-only (at the time of writing).  */
  if (dwarf2_elf_names.abbrev.matches (sectp->name))
    sect = &dwz_file->abbrev;
  else if (dwarf2_elf_names.info.matches (sectp->name))
    sect = &dwz_file->info;
  else if (dwarf2_elf_names.str.matches (sectp->name))
    sect = &dwz_file->str;
  else if (dwarf2_elf_names.line.matches (sectp->name))
    sect = &dwz_file->line;
  else if (dwarf2_elf_names.macro.matches (sectp->name))
    sect = &dwz_file->macro;
  else if (dwarf2_elf_names.gdb_index.matches (sectp->name))
    sect = &dwz_file->gdb_index;
  else if (dwarf2_elf_names.debug_names.matches (sectp->name))
    sect = &dwz_file->debug_names;
  else if (dwarf2_elf_names.types.matches (sectp->name))
    sect = &dwz_file->types;

  if (sect != nullptr)
    {
      sect->s.section = sectp;
      sect->size = bfd_section_size (sectp);
      sect->read (objfile);
    }
}

/* Attempt to find a .dwz file (whose full path is represented by
   FILENAME) in all of the specified debug file directories provided.

   Return the equivalent gdb_bfd_ref_ptr of the .dwz file found, or
   nullptr if it could not find anything.  */

static gdb_bfd_ref_ptr
dwz_search_other_debugdirs (std::string &filename, bfd_byte *buildid,
			    size_t buildid_len)
{
  /* Let's assume that the path represented by FILENAME has the
     "/.dwz/" subpath in it.  This is what (most) GNU/Linux
     distributions do, anyway.  */
  size_t dwz_pos = filename.find ("/.dwz/");

  if (dwz_pos == std::string::npos)
    return nullptr;

  /* This is an obvious assertion, but it's here more to educate
     future readers of this code that FILENAME at DWZ_POS *must*
     contain a directory separator.  */
  gdb_assert (IS_DIR_SEPARATOR (filename[dwz_pos]));

  gdb_bfd_ref_ptr dwz_bfd;
  std::vector<gdb::unique_xmalloc_ptr<char>> debugdir_vec
    = dirnames_to_char_ptr_vec (debug_file_directory.c_str ());

  for (const gdb::unique_xmalloc_ptr<char> &debugdir : debugdir_vec)
    {
      /* The idea is to iterate over the
	 debug file directories provided by the user and
	 replace the hard-coded path in the "filename" by each
	 debug-file-directory.

	 For example, suppose that filename is:

	   /usr/lib/debug/.dwz/foo.dwz

	 And suppose that we have "$HOME/bar" as the
	 debug-file-directory.  We would then adjust filename
	 to look like:

	   $HOME/bar/.dwz/foo.dwz

	 which would hopefully allow us to find the alt debug
	 file.  */
      std::string ddir = debugdir.get ();

      if (ddir.empty ())
	continue;

      /* Make sure the current debug-file-directory ends with a
	 directory separator.  This is needed because, if FILENAME
	 contains something like "/usr/lib/abcde/.dwz/foo.dwz" and
	 DDIR is "/usr/lib/abc", then could wrongfully skip it
	 below.  */
      if (!IS_DIR_SEPARATOR (ddir.back ()))
	ddir += SLASH_STRING;

      /* Check whether the beginning of FILENAME is DDIR.  If it is,
	 then we are dealing with a file which we already attempted to
	 open before, so we just skip it and continue processing the
	 remaining debug file directories.  */
      if (filename.size () > ddir.size ()
	  && filename.compare (0, ddir.size (), ddir) == 0)
	continue;

      /* Replace FILENAME's default debug-file-directory with
	 DDIR.  */
      std::string new_filename = ddir + &filename[dwz_pos + 1];

      dwz_bfd = gdb_bfd_open (new_filename.c_str (), gnutarget);

      if (dwz_bfd == nullptr)
	continue;

      if (!build_id_verify (dwz_bfd.get (), buildid_len, buildid))
	{
	  dwz_bfd.reset (nullptr);
	  continue;
	}

      /* Found it.  */
      break;
    }

  return dwz_bfd;
}

/* See dwz.h.  */

void
dwarf2_read_dwz_file (dwarf2_per_objfile *per_objfile)
{
  bfd_size_type buildid_len_arg;
  size_t buildid_len;
  bfd_byte *buildid;
  dwarf2_per_bfd *per_bfd = per_objfile->per_bfd;

  /* This may query the user via the debuginfod support, so it may
     only be run in the main thread.  */
  gdb_assert (is_main_thread ());

  /* Set this early, so that on error it remains NULL.  */
  per_bfd->dwz_file.emplace (nullptr);

  bfd_set_error (bfd_error_no_error);
  gdb::unique_xmalloc_ptr<char> data
    (bfd_get_alt_debug_link_info (per_bfd->obfd,
				  &buildid_len_arg, &buildid));
  if (data == NULL)
    {
      if (bfd_get_error () == bfd_error_no_error)
	return;
      error (_("could not read '.gnu_debugaltlink' section: %s"),
	     bfd_errmsg (bfd_get_error ()));
    }

  gdb::unique_xmalloc_ptr<bfd_byte> buildid_holder (buildid);

  buildid_len = (size_t) buildid_len_arg;

  std::string filename = data.get ();

  if (!IS_ABSOLUTE_PATH (filename.c_str ()))
    {
      gdb::unique_xmalloc_ptr<char> abs
	= gdb_realpath (bfd_get_filename (per_bfd->obfd));

      filename = ldirname (abs.get ()) + SLASH_STRING + filename;
    }

  /* First try the file name given in the section.  If that doesn't
     work, try to use the build-id instead.  */
  gdb_bfd_ref_ptr dwz_bfd (gdb_bfd_open (filename.c_str (), gnutarget));
  if (dwz_bfd != NULL)
    {
      if (!build_id_verify (dwz_bfd.get (), buildid_len, buildid))
	dwz_bfd.reset (nullptr);
    }

  if (dwz_bfd == NULL)
    dwz_bfd = build_id_to_debug_bfd (buildid_len, buildid);

  if (dwz_bfd == nullptr)
    {
      /* If the user has provided us with different
	 debug file directories, we can try them in order.  */
      dwz_bfd = dwz_search_other_debugdirs (filename, buildid, buildid_len);
    }

  if (dwz_bfd == nullptr)
    {
      gdb::unique_xmalloc_ptr<char> alt_filename;
      const char *origname = bfd_get_filename (per_bfd->obfd);

      scoped_fd fd (debuginfod_debuginfo_query (buildid,
						buildid_len,
						origname,
						&alt_filename));

      if (fd.get () >= 0)
	{
	  /* File successfully retrieved from server.  */
	  dwz_bfd = gdb_bfd_open (alt_filename.get (), gnutarget);

	  if (dwz_bfd == nullptr)
	    warning (_("File \"%s\" from debuginfod cannot be opened as bfd"),
		     alt_filename.get ());
	  else if (!build_id_verify (dwz_bfd.get (), buildid_len, buildid))
	    dwz_bfd.reset (nullptr);
	}
    }

  if (dwz_bfd == NULL)
    error (_("could not find '.gnu_debugaltlink' file for %s"),
	   bfd_get_filename (per_bfd->obfd));

  std::unique_ptr<struct dwz_file> result
    (new struct dwz_file (std::move (dwz_bfd)));

  for (asection *sec : gdb_bfd_sections (result->dwz_bfd))
    locate_dwz_sections (per_objfile->objfile, result->dwz_bfd.get (),
			 sec, result.get ());

  gdb_bfd_record_inclusion (per_bfd->obfd, result->dwz_bfd.get ());
  bfd_cache_close (result->dwz_bfd.get ());

  per_bfd->dwz_file = std::move (result);
}

/* See dwz.h.  */

struct dwz_file *
dwarf2_get_dwz_file (dwarf2_per_bfd *per_bfd, bool require)
{
  gdb_assert (!require || per_bfd->dwz_file.has_value ());

  dwz_file *result = nullptr;
  if (per_bfd->dwz_file.has_value ())
    {
      result = per_bfd->dwz_file->get ();
      if (require && result == nullptr)
	error (_("could not read '.gnu_debugaltlink' section"));
    }
  return result;
}
