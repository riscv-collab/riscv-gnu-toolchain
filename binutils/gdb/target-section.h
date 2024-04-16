/* Target sections.

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

#ifndef GDB_TARGET_SECTION_H
#define GDB_TARGET_SECTION_H

struct bfd;
struct objfile;
struct shobj;

/* A union representing the possible owner types of a target_section.  */

union target_section_owner
{
  target_section_owner () : m_v (nullptr) {}
  target_section_owner (const bfd *bfd) : bfd (bfd) {}
  target_section_owner (const objfile *objfile) : objfile (objfile) {}
  target_section_owner (const shobj *shobj) : shobj (shobj) {}

  /* Use this to access the type-erased version of the owner, for
     comparisons, printing, etc.  We don't access the M_V member
     directly, because pedantically it is not valid to access a
     non-active union member.  */
  const void *v () const
  {
    void *tmp;
    memcpy (&tmp, this, sizeof (*this));
    return tmp;
  }

  const struct bfd *bfd;
  const struct objfile *objfile;
  const struct shobj *shobj;

private:
  const void *m_v;
};

/* Struct target_section maps address ranges to file sections.  It is
   mostly used with BFD files, but can be used without (e.g. for handling
   raw disks, or files not in formats handled by BFD).  */

struct target_section
{
  target_section (CORE_ADDR addr_, CORE_ADDR end_, struct bfd_section *sect_,
		  target_section_owner owner_ = {})
    : addr (addr_),
      endaddr (end_),
      the_bfd_section (sect_),
      owner (owner_)
  {
  }

  /* Lowest address in section.  */
  CORE_ADDR addr;
  /* Highest address in section, plus 1.  */
  CORE_ADDR endaddr;

  /* The BFD section.  */
  struct bfd_section *the_bfd_section;

  /* The "owner" of the section.

     It is set by add_target_sections and used by remove_target_sections.
     For example, for executables it is a pointer to exec_bfd and
     for shlibs it is the shobj pointer.  */
  target_section_owner owner;
};

#endif /* GDB_TARGET_SECTION_H */
