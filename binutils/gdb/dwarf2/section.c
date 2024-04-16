/* DWARF 2 low-level section code

   Copyright (C) 1994-2024 Free Software Foundation, Inc.

   Adapted by Gary Funck (gary@intrepid.com), Intrepid Technology,
   Inc.  with support from Florida State University (under contract
   with the Ada Joint Program Office), and Silicon Graphics, Inc.
   Initial contribution by Brent Benson, Harris Computer Systems, Inc.,
   based on Fred Fish's (Cygnus Support) implementation of DWARF 1
   support.

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
#include "dwarf2/section.h"
#include "gdb_bfd.h"
#include "objfiles.h"
#include "complaints.h"

void
dwarf2_section_info::overflow_complaint () const
{
  complaint (_("debug info runs off end of %s section"
	       " [in module %s]"),
	     get_name (), get_file_name ());
}

struct dwarf2_section_info *
dwarf2_section_info::get_containing_section () const
{
  gdb_assert (is_virtual);
  return s.containing_section;
}

struct bfd *
dwarf2_section_info::get_bfd_owner () const
{
  const dwarf2_section_info *section = this;
  if (is_virtual)
    {
      section = get_containing_section ();
      gdb_assert (!section->is_virtual);
    }
  gdb_assert (section->s.section != nullptr);
  return section->s.section->owner;
}

asection *
dwarf2_section_info::get_bfd_section () const
{
  const dwarf2_section_info *section = this;
  if (section->is_virtual)
    {
      section = get_containing_section ();
      gdb_assert (!section->is_virtual);
    }
  return section->s.section;
}

const char *
dwarf2_section_info::get_name () const
{
  asection *sectp = get_bfd_section ();

  gdb_assert (sectp != NULL);
  return bfd_section_name (sectp);
}

const char *
dwarf2_section_info::get_file_name () const
{
  bfd *abfd = get_bfd_owner ();

  gdb_assert (abfd != nullptr);
  return bfd_get_filename (abfd);
}

int
dwarf2_section_info::get_id () const
{
  asection *sectp = get_bfd_section ();

  if (sectp == NULL)
    return 0;
  return sectp->id;
}

int
dwarf2_section_info::get_flags () const
{
  asection *sectp = get_bfd_section ();

  gdb_assert (sectp != NULL);
  return bfd_section_flags (sectp);
}

bool
dwarf2_section_info::empty () const
{
  if (is_virtual)
    return size == 0;
  return s.section == NULL || size == 0;
}

void
dwarf2_section_info::read (struct objfile *objfile)
{
  asection *sectp;
  bfd *abfd;
  gdb_byte *buf, *retbuf;

  if (readin)
    return;
  buffer = NULL;
  readin = true;

  if (empty ())
    return;

  sectp = get_bfd_section ();

  /* If this is a virtual section we need to read in the real one first.  */
  if (is_virtual)
    {
      struct dwarf2_section_info *containing_section =
	get_containing_section ();

      gdb_assert (sectp != NULL);
      if ((sectp->flags & SEC_RELOC) != 0)
	{
	  error (_("Dwarf Error: DWP format V2 with relocations is not"
		   " supported in section %s [in module %s]"),
		 get_name (), get_file_name ());
	}
      containing_section->read (objfile);
      /* Other code should have already caught virtual sections that don't
	 fit.  */
      gdb_assert (virtual_offset + size <= containing_section->size);
      /* If the real section is empty or there was a problem reading the
	 section we shouldn't get here.  */
      gdb_assert (containing_section->buffer != NULL);
      buffer = containing_section->buffer + virtual_offset;
      return;
    }

  /* If the section has relocations, we must read it ourselves.
     Otherwise we attach it to the BFD.  */
  if ((sectp->flags & SEC_RELOC) == 0)
    {
      buffer = gdb_bfd_map_section (sectp, &size);
      return;
    }

  buf = (gdb_byte *) obstack_alloc (&objfile->objfile_obstack, size);
  buffer = buf;

  /* When debugging .o files, we may need to apply relocations; see
     http://sourceware.org/ml/gdb-patches/2002-04/msg00136.html .
     We never compress sections in .o files, so we only need to
     try this when the section is not compressed.  */
  retbuf = symfile_relocate_debug_section (objfile, sectp, buf);
  if (retbuf != NULL)
    {
      buffer = retbuf;
      return;
    }

  abfd = get_bfd_owner ();
  gdb_assert (abfd != NULL);

  if (bfd_seek (abfd, sectp->filepos, SEEK_SET) != 0
      || bfd_read (buf, size, abfd) != size)
    {
      error (_("Dwarf Error: Can't read DWARF data"
	       " in section %s [in module %s]"),
	     bfd_section_name (sectp), bfd_get_filename (abfd));
    }
}

const char *
dwarf2_section_info::read_string (struct objfile *objfile, LONGEST str_offset,
				  const char *form_name)
{
  read (objfile);
  if (buffer == NULL)
    {
      if (get_bfd_section () == nullptr)
	error (_("Dwarf Error: %s used without required section"),
	       form_name);
      else
	error (_("Dwarf Error: %s used without %s section [in module %s]"),
	       form_name, get_name (), get_file_name ());
    }
  if (str_offset >= size)
    error (_("%s pointing outside of %s section [in module %s]"),
	   form_name, get_name (), get_file_name ());
  gdb_assert (HOST_CHAR_BIT == 8);
  if (buffer[str_offset] == '\0')
    return NULL;
  return (const char *) (buffer + str_offset);
}
