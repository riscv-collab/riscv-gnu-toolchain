/* Cache of styled source file text
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

#ifndef SOURCE_CACHE_H
#define SOURCE_CACHE_H

#include <unordered_map>
#include <unordered_set>

/* This caches two things related to source files.

   First, it caches highlighted source text, keyed by the source
   file's full name.  A size-limited LRU cache is used.

   Highlighting depends on the GNU Source Highlight library.  When not
   available or when highlighting fails for some reason, this cache
   will instead store the un-highlighted source text.

   Second, this will cache the file offsets corresponding to the start
   of each line of a source file.  This cache is not size-limited.  */
class source_cache
{
public:

  source_cache ()
  {
  }

  /* This returns the vector of file offsets for the symtab S,
     computing the vector first if needed.

     On failure, returns false.

     On success, returns true and sets *OFFSETS.  This pointer is not
     guaranteed to remain valid across other calls to get_source_lines
     or get_line_charpos.  */
  bool get_line_charpos (struct symtab *s,
			 const std::vector<off_t> **offsets);

  /* Get the source text for the source file in symtab S.  FIRST_LINE
     and LAST_LINE are the first and last lines to return; line
     numbers are 1-based.  If the file cannot be read, or if the line
     numbers are out of range, false is returned.  Otherwise,
     LINES_OUT is set to the desired text.  The returned text may
     include ANSI terminal escapes.  */
  bool get_source_lines (struct symtab *s, int first_line,
			 int last_line, std::string *lines_out);

  /* Remove all the items from the source cache.  */
  void clear ()
  {
    m_source_map.clear ();
    m_offset_cache.clear ();
    m_no_styling_files.clear ();
  }

private:

  /* One element in the cache.  */
  struct source_text
  {
    /* The full name of the file.  */
    std::string fullname;
    /* The contents of the file.  */
    std::string contents;
  };

  /* A helper function for get_source_lines reads a source file.
     Returns the contents of the file; or throws an exception on
     error.  This also updates m_offset_cache.  */
  std::string get_plain_source_lines (struct symtab *s,
				      const std::string &fullname);

  /* A helper function that the data for the given symtab is entered
     into both caches.  Returns false on error.  */
  bool ensure (struct symtab *s);

  /* The contents of the source text cache.  */
  std::vector<source_text> m_source_map;

  /* The file offset cache.  The key is the full name of the source
     file.  */
  std::unordered_map<std::string, std::vector<off_t>> m_offset_cache;

  /* The list of files where styling failed.  */
  std::unordered_set<std::string> m_no_styling_files;
};

/* The global source cache.  */
extern source_cache g_source_cache;

#endif /* SOURCE_CACHE_H */
