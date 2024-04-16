/* Caching of GDB/DWARF index files.

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

#ifndef DWARF_INDEX_CACHE_H
#define DWARF_INDEX_CACHE_H

#include "dwarf2/index-common.h"
#include "gdbsupport/array-view.h"
#include "symfile.h"

class dwarf2_per_bfd;
class index_cache;

/* Base of the classes used to hold the resources of the indices loaded from
   the cache (e.g. mmapped files).  */

struct index_cache_resource
{
  virtual ~index_cache_resource () = 0;
};

/* Information to be captured in the main thread, and to be used by worker
   threads during store ().  */

struct index_cache_store_context
{
  friend class index_cache;

  index_cache_store_context (const index_cache &ic, dwarf2_per_bfd *per_bfd);

private:
  /* Captured value of enabled ().  */
  bool m_enabled;

  /* Captured value of build id.  */
  std::string build_id_str;

  /* Captured value of dwz build id.  */
  std::optional<std::string> dwz_build_id_str;
};

/* Class to manage the access to the DWARF index cache.  */

class index_cache
{
  friend struct index_cache_store_context;
public:
  /* Change the directory used to save/load index files.  */
  void set_directory (std::string dir);

  /* Return true if the usage of the cache is enabled.  */
  bool enabled () const
  {
    return m_enabled;
  }

  /* Enable the cache.  */
  void enable ();

  /* Disable the cache.  */
  void disable ();

  /* Store an index for the specified object file in the cache.  */
  void store (dwarf2_per_bfd *per_bfd,
	      const index_cache_store_context &);

  /* Look for an index file matching BUILD_ID.  If found, return the contents
     as an array_view and store the underlying resources (allocated memory,
     mapped file, etc) in RESOURCE.  The returned array_view is valid as long
     as RESOURCE is not destroyed.

     If no matching index file is found, return an empty array view.  */
  gdb::array_view<const gdb_byte>
  lookup_gdb_index (const bfd_build_id *build_id,
		    std::unique_ptr<index_cache_resource> *resource);

  /* Return the number of cache hits.  */
  unsigned int n_hits () const
  { return m_n_hits; }

  /* Record a cache hit.  */
  void hit ()
  {
    if (enabled ())
      m_n_hits++;
  }

  /* Return the number of cache misses.  */
  unsigned int n_misses () const
  { return m_n_misses; }

  /* Record a cache miss.  */
  void miss ()
  {
    if (enabled ())
      m_n_misses++;
  }

private:

  /* Compute the absolute filename where the index of the objfile with build
     id BUILD_ID will be stored.  SUFFIX is appended at the end of the
     filename.  */
  std::string make_index_filename (const bfd_build_id *build_id,
				   const char *suffix) const;

  /* The base directory where we are storing and looking up index files.  */
  std::string m_dir;

  /* Whether the cache is enabled.  */
  bool m_enabled = false;

  /* Number of cache hits and misses during this GDB session.  */
  unsigned int m_n_hits = 0;
  unsigned int m_n_misses = 0;
};

/* The global instance of the index cache.  */
extern index_cache global_index_cache;

#endif /* DWARF_INDEX_CACHE_H */
