/* Definitions for BFD wrappers used by GDB.

   Copyright (C) 2011-2024 Free Software Foundation, Inc.

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

#ifndef GDB_BFD_H
#define GDB_BFD_H

#include "registry.h"
#include "gdbsupport/byte-vector.h"
#include "gdbsupport/function-view.h"
#include "gdbsupport/gdb_ref_ptr.h"
#include "gdbsupport/iterator-range.h"
#include "gdbsupport/next-iterator.h"

/* A registry adaptor for BFD.  This arranges to store the registry in
   gdb's per-BFD data, which is stored as the bfd_usrdata.  */
template<>
struct registry_accessor<bfd>
{
  static registry<bfd> *get (bfd *abfd);
};

/* If supplied a path starting with this sequence, gdb_bfd_open will
   open BFDs using target fileio operations.  */

#define TARGET_SYSROOT_PREFIX "target:"

/* Returns nonzero if NAME starts with TARGET_SYSROOT_PREFIX, zero
   otherwise.  */

int is_target_filename (const char *name);

/* An overload for strings.  */

static inline int
is_target_filename (const std::string &name)
{
  return is_target_filename (name.c_str ());
}

/* Returns nonzero if the filename associated with ABFD starts with
   TARGET_SYSROOT_PREFIX, zero otherwise.  */

int gdb_bfd_has_target_filename (struct bfd *abfd);

/* Increment the reference count of ABFD.  It is fine for ABFD to be
   NULL; in this case the function does nothing.  */

void gdb_bfd_ref (struct bfd *abfd);

/* Decrement the reference count of ABFD.  If this is the last
   reference, ABFD will be freed.  If ABFD is NULL, this function does
   nothing.  */

void gdb_bfd_unref (struct bfd *abfd);

/* A policy class for gdb::ref_ptr for BFD reference counting.  */
struct gdb_bfd_ref_policy
{
  static void incref (struct bfd *abfd)
  {
    gdb_bfd_ref (abfd);
  }

  static void decref (struct bfd *abfd)
  {
    gdb_bfd_unref (abfd);
  }
};

/* A gdb::ref_ptr that has been specialized for BFD objects.  */
typedef gdb::ref_ptr<struct bfd, gdb_bfd_ref_policy> gdb_bfd_ref_ptr;

/* Open a read-only (FOPEN_RB) BFD given arguments like bfd_fopen.
   If NAME starts with TARGET_SYSROOT_PREFIX then the BFD will be
   opened using target fileio operations if necessary.  Returns NULL
   on error.  On success, returns a new reference to the BFD.  BFDs
   returned by this call are shared among all callers opening the same
   file.  If FD is not -1, then after this call it is owned by BFD.
   If the BFD was not accessed using target fileio operations then the
   filename associated with the BFD and accessible with
   bfd_get_filename will not be exactly NAME but rather NAME with
   TARGET_SYSROOT_PREFIX stripped.  If WARN_IF_SLOW is true, print a
   warning message if the file is being accessed over a link that may
   be slow.  */

gdb_bfd_ref_ptr gdb_bfd_open (const char *name, const char *target,
			      int fd = -1, bool warn_if_slow = true);

/* Mark the CHILD BFD as being a member of PARENT.  Also, increment
   the reference count of CHILD.  Calling this function ensures that
   as along as CHILD remains alive, PARENT will as well.  Both CHILD
   and PARENT must be non-NULL.  This can be called more than once
   with the same arguments; but it is not allowed to call it for a
   single CHILD with different values for PARENT.  */

void gdb_bfd_mark_parent (bfd *child, bfd *parent);

/* Mark INCLUDEE as being included by INCLUDER.
   This is used to associate the life time of INCLUDEE with INCLUDER.
   For example, with Fission, one file can refer to debug info in another
   file, and internal tables we build for the main file (INCLUDER) may refer
   to data contained in INCLUDEE.  Therefore we want to keep INCLUDEE around
   at least as long as INCLUDER exists.

   Note that this is different than gdb_bfd_mark_parent because in our case
   lifetime tracking is based on the "parent" whereas in gdb_bfd_mark_parent
   lifetime tracking is based on the "child".  Plus in our case INCLUDEE could
   have multiple different "parents".  */

void gdb_bfd_record_inclusion (bfd *includer, bfd *includee);

/* Try to read or map the contents of the section SECT.  If successful, the
   section data is returned and *SIZE is set to the size of the section data;
   this may not be the same as the size according to bfd_section_size if the
   section was compressed.  The returned section data is associated with the BFD
   and will be destroyed when the BFD is destroyed.  There is no other way to
   free it; for temporary uses of section data, see bfd_malloc_and_get_section.
   SECT may not have relocations.  If there is an error reading the section,
   this issues a warning, sets *SIZE to 0, and returns NULL.  */

const gdb_byte *gdb_bfd_map_section (asection *section, bfd_size_type *size);

/* Compute the CRC for ABFD.  The CRC is used to find and verify
   separate debug files.  When successful, this fills in *CRC_OUT and
   returns 1.  Otherwise, this issues a warning and returns 0.  */

int gdb_bfd_crc (struct bfd *abfd, unsigned long *crc_out);



/* A wrapper for bfd_fopen that initializes the gdb-specific reference
   count.  */

gdb_bfd_ref_ptr gdb_bfd_fopen (const char *, const char *, const char *, int);

/* A wrapper for bfd_openr that initializes the gdb-specific reference
   count.  */

gdb_bfd_ref_ptr gdb_bfd_openr (const char *, const char *);

/* A wrapper for bfd_openw that initializes the gdb-specific reference
   count.  */

gdb_bfd_ref_ptr gdb_bfd_openw (const char *, const char *);

/* The base class for BFD "iovec" implementations.  This is used by
   gdb_bfd_openr_iovec and enables better type safety.  */

class gdb_bfd_iovec_base
{
protected:

  gdb_bfd_iovec_base () = default;

public:

  virtual ~gdb_bfd_iovec_base () = default;

  /* The "read" callback.  */
  virtual file_ptr read (bfd *abfd, void *buffer, file_ptr nbytes,
			 file_ptr offset) = 0;

  /* The "stat" callback.  */
  virtual int stat (struct bfd *abfd, struct stat *sb) = 0;
};

/* The type of the function used to open a new iovec-based BFD.  */
using gdb_iovec_opener_ftype
     = gdb::function_view<gdb_bfd_iovec_base * (bfd *)>;

/* A type-safe wrapper for bfd_openr_iovec.  */

gdb_bfd_ref_ptr gdb_bfd_openr_iovec (const char *filename, const char *target,
				     gdb_iovec_opener_ftype open_fn);

/* A wrapper for bfd_openr_next_archived_file that initializes the
   gdb-specific reference count.  */

gdb_bfd_ref_ptr gdb_bfd_openr_next_archived_file (bfd *archive, bfd *previous);




/* Return the index of the BFD section SECTION.  Ordinarily this is
   just the section's index, but for some special sections, like
   bfd_com_section_ptr, it will be a synthesized value.  */

int gdb_bfd_section_index (bfd *abfd, asection *section);


/* Like bfd_count_sections, but include any possible global sections,
   like bfd_com_section_ptr.  */

int gdb_bfd_count_sections (bfd *abfd);

/* Return true if any section requires relocations, false
   otherwise.  */

int gdb_bfd_requires_relocations (bfd *abfd);

/* Alternative to bfd_get_full_section_contents that returns the section
   contents in *CONTENTS, instead of an allocated buffer.

   Return true on success, false otherwise.  */

bool gdb_bfd_get_full_section_contents (bfd *abfd, asection *section,
					gdb::byte_vector *contents);

/* Create and initialize a BFD handle from a target in-memory range.  The
   BFD starts at ADDR and is SIZE bytes long.  TARGET is the BFD target
   name as used in bfd_find_target.  */

gdb_bfd_ref_ptr gdb_bfd_open_from_target_memory (CORE_ADDR addr, ULONGEST size,
						 const char *target);

/* Range adapter for a BFD's sections.

   To be used as:

     for (asection *sect : gdb_bfd_all_sections (bfd))
       ... use SECT ...
 */

using gdb_bfd_section_range = next_range<asection>;

static inline gdb_bfd_section_range
gdb_bfd_sections (bfd *abfd)
{
  return gdb_bfd_section_range (abfd->sections);
}

static inline gdb_bfd_section_range
gdb_bfd_sections (const gdb_bfd_ref_ptr &abfd)
{
  return gdb_bfd_section_range (abfd->sections);
};

/* A wrapper for bfd_errmsg to produce a more helpful error message
   in the case of bfd_error_file_ambiguously recognized.
   MATCHING, if non-NULL, is the corresponding argument to
   bfd_check_format_matches, and will be freed.  */

extern std::string gdb_bfd_errmsg (bfd_error_type error_tag, char **matching);

/* A wrapper for bfd_init that also handles setting up for
   multi-threading.  */

extern void gdb_bfd_init ();

#endif /* GDB_BFD_H */
