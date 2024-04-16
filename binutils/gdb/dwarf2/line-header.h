/* DWARF 2 debugging format support for GDB.

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

#ifndef DWARF2_LINE_HEADER_H
#define DWARF2_LINE_HEADER_H

/* dir_index is 1-based in DWARF 4 and before, and is 0-based in DWARF 5 and
   later.  */
typedef int dir_index;

/* file_name_index is 1-based in DWARF 4 and before, and is 0-based in DWARF 5
   and later.  */
typedef int file_name_index;

struct line_header;

struct file_entry
{
  file_entry () = default;

  file_entry (const char *name_, file_name_index index_, dir_index d_index_,
	      unsigned int mod_time_, unsigned int length_)
    : name (name_),
      index (index_),
      d_index (d_index_),
      mod_time (mod_time_),
      length (length_)
  {}

  /* Return the include directory at D_INDEX stored in LH.  Returns
     NULL if D_INDEX is out of bounds.  */
  const char *include_dir (const line_header *lh) const;

  /* The file name.  Note this is an observing pointer.  The memory is
     owned by debug_line_buffer.  */
  const char *name {};

  /* The index of this file in the file table.  */
  file_name_index index {};

  /* The directory index (1-based).  */
  dir_index d_index {};

  unsigned int mod_time {};

  unsigned int length {};

  /* The associated symbol table, if any.  */
  struct symtab *symtab {};
};

/* The line number information for a compilation unit (found in the
   .debug_line section) begins with a "statement program header",
   which contains the following information.  */
struct line_header
{
  /* COMP_DIR is the value of the DW_AT_comp_dir attribute of the compilation
     unit in the context of which we are reading this line header, or nullptr
     if unknown or not applicable.  */
  explicit line_header (const char *comp_dir)
    : offset_in_dwz {}, m_comp_dir (comp_dir)
  {}

  /* This constructor should only be used to create line_header instances to do
     hash table lookups.  */
  line_header (sect_offset sect_off, bool offset_in_dwz)
    : sect_off (sect_off),
      offset_in_dwz (offset_in_dwz)
  {}

  /* Add an entry to the include directory table.  */
  void add_include_dir (const char *include_dir);

  /* Add an entry to the file name table.  */
  void add_file_name (const char *name, dir_index d_index,
		      unsigned int mod_time, unsigned int length);

  /* Return the include dir at INDEX (0-based in DWARF 5 and 1-based before).
     Returns NULL if INDEX is out of bounds.  */
  const char *include_dir_at (dir_index index) const
  {
    int vec_index;
    if (version >= 5)
      vec_index = index;
    else
      vec_index = index - 1;
    if (vec_index < 0 || vec_index >= m_include_dirs.size ())
      return NULL;
    return m_include_dirs[vec_index];
  }

  bool is_valid_file_index (int file_index) const
  {
    if (version >= 5)
      return 0 <= file_index && file_index < file_names_size ();
    return 1 <= file_index && file_index <= file_names_size ();
  }

  /* Return the file name at INDEX (0-based in DWARF 5 and 1-based before).
     Returns NULL if INDEX is out of bounds.  */
  file_entry *file_name_at (file_name_index index)
  {
    int vec_index;
    if (version >= 5)
      vec_index = index;
    else
      vec_index = index - 1;
    if (vec_index < 0 || vec_index >= m_file_names.size ())
      return NULL;
    return &m_file_names[vec_index];
  }

  /* A const overload of the same.  */
  const file_entry *file_name_at (file_name_index index) const
  {
    line_header *lh = const_cast<line_header *> (this);
    return lh->file_name_at (index);
  }

  /* The indexes are 0-based in DWARF 5 and 1-based in DWARF 4. Therefore,
     this method should only be used to iterate through all file entries in an
     index-agnostic manner.  */
  std::vector<file_entry> &file_names ()
  { return m_file_names; }
  /* A const overload of the same.  */
  const std::vector<file_entry> &file_names () const
  { return m_file_names; }

  /* Offset of line number information in .debug_line section.  */
  sect_offset sect_off {};

  /* OFFSET is for struct dwz_file associated with dwarf2_per_objfile.  */
  unsigned offset_in_dwz : 1; /* Can't initialize bitfields in-class.  */

  unsigned short version {};
  unsigned char minimum_instruction_length {};
  unsigned char maximum_ops_per_instruction {};
  unsigned char default_is_stmt {};
  int line_base {};
  unsigned char line_range {};
  unsigned char opcode_base {};

  /* standard_opcode_lengths[i] is the number of operands for the
     standard opcode whose value is i.  This means that
     standard_opcode_lengths[0] is unused, and the last meaningful
     element is standard_opcode_lengths[opcode_base - 1].  */
  std::unique_ptr<unsigned char[]> standard_opcode_lengths;

  int file_names_size () const
  { return m_file_names.size(); }

  /* The start and end of the statement program following this
     header.  These point into dwarf2_per_objfile->line_buffer.  */
  const gdb_byte *statement_program_start {}, *statement_program_end {};

  /* Return the most "complete" file name for FILE possible.

     This means prepending the directory and compilation directory, as needed,
     until we get an absolute path.  */
  std::string file_file_name (const file_entry &fe) const;

  /* Return the compilation directory of the compilation unit in the context of
     which this line header is read.  Return nullptr if non applicable.  */
  const char *comp_dir () const
  { return m_comp_dir; }

 private:
  /* The include_directories table.  Note these are observing
     pointers.  The memory is owned by debug_line_buffer.  */
  std::vector<const char *> m_include_dirs;

  /* The file_names table. This is private because the meaning of indexes
     differs among DWARF versions (The first valid index is 1 in DWARF 4 and
     before, and is 0 in DWARF 5 and later).  So the client should use
     file_name_at method for access.  */
  std::vector<file_entry> m_file_names;

  /* Compilation directory of the compilation unit in the context of which this
     line header is read.  nullptr if unknown or not applicable.  */
  const char *m_comp_dir = nullptr;
};

typedef std::unique_ptr<line_header> line_header_up;

inline const char *
file_entry::include_dir (const line_header *lh) const
{
  return lh->include_dir_at (d_index);
}

/* Read the statement program header starting at SECT_OFF in SECTION.
   Return line_header.  Returns nullptr if there is a problem reading
   the header, e.g., if it has a version we don't understand.

   NOTE: the strings in the include directory and file name tables of
   the returned object point into the dwarf line section buffer,
   and must not be freed.  */

extern line_header_up dwarf_decode_line_header
  (sect_offset sect_off, bool is_dwz, dwarf2_per_objfile *per_objfile,
   struct dwarf2_section_info *section, const struct comp_unit_head *cu_header,
   const char *comp_dir);

#endif /* DWARF2_LINE_HEADER_H */
