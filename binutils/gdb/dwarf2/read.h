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

#ifndef DWARF2READ_H
#define DWARF2READ_H

#include <queue>
#include <unordered_map>
#include "dwarf2/comp-unit-head.h"
#include "dwarf2/file-and-dir.h"
#include "dwarf2/index-cache.h"
#include "dwarf2/mapped-index.h"
#include "dwarf2/section.h"
#include "dwarf2/cu.h"
#include "filename-seen-cache.h"
#include "gdbsupport/gdb_obstack.h"
#include "gdbsupport/hash_enum.h"
#include "gdbsupport/function-view.h"
#include "gdbsupport/packed.h"

/* Hold 'maintenance (set|show) dwarf' commands.  */
extern struct cmd_list_element *set_dwarf_cmdlist;
extern struct cmd_list_element *show_dwarf_cmdlist;

struct tu_stats
{
  int nr_uniq_abbrev_tables;
  int nr_symtabs;
  int nr_symtab_sharers;
  int nr_stmt_less_type_units;
  int nr_all_type_units_reallocs;
  int nr_tus;
};

struct dwarf2_cu;
struct dwarf2_debug_sections;
struct dwarf2_per_bfd;
struct dwarf2_per_cu_data;
struct mapped_index;
struct mapped_debug_names;
struct signatured_type;
struct type_unit_group;

/* One item on the queue of compilation units to read in full symbols
   for.  */
struct dwarf2_queue_item
{
  dwarf2_queue_item (dwarf2_per_cu_data *cu, dwarf2_per_objfile *per_objfile,
		     enum language lang)
    : per_cu (cu),
      per_objfile (per_objfile),
      pretend_language (lang)
  {
  }

  ~dwarf2_queue_item ();

  DISABLE_COPY_AND_ASSIGN (dwarf2_queue_item);

  dwarf2_per_cu_data *per_cu;
  dwarf2_per_objfile *per_objfile;
  enum language pretend_language;
};

/* A deleter for dwarf2_per_cu_data that knows to downcast to
   signatured_type as appropriate.  This approach lets us avoid a
   virtual destructor, which saves a bit of space.  */

struct dwarf2_per_cu_data_deleter
{
  void operator() (dwarf2_per_cu_data *data);
};

/* A specialization of unique_ptr for dwarf2_per_cu_data and
   subclasses.  */
typedef std::unique_ptr<dwarf2_per_cu_data, dwarf2_per_cu_data_deleter>
    dwarf2_per_cu_data_up;

/* Persistent data held for a compilation unit, even when not
   processing it.  We put a pointer to this structure in the
   psymtab.  */

struct dwarf2_per_cu_data
{
  dwarf2_per_cu_data ()
    : is_debug_types (false),
      is_dwz (false),
      reading_dwo_directly (false),
      tu_read (false),
      queued (false),
      m_header_read_in (false),
      mark (false),
      files_read (false),
      scanned (false)
  {
  }

  /* The start offset and length of this compilation unit.
     NOTE: Unlike comp_unit_head.length, this length includes
     initial_length_size.
     If the DIE refers to a DWO file, this is always of the original die,
     not the DWO file.  */
  sect_offset sect_off {};

private:
  unsigned int m_length = 0;

  /* DWARF standard version this data has been read from (such as 4 or 5).  */
  unsigned char m_dwarf_version = 0;

public:
  /* Non-zero if this CU is from .debug_types.
     Struct dwarf2_per_cu_data is contained in struct signatured_type iff
     this is non-zero.  */
  unsigned int is_debug_types : 1;

  /* Non-zero if this CU is from the .dwz file.  */
  unsigned int is_dwz : 1;

  /* Non-zero if reading a TU directly from a DWO file, bypassing the stub.
     This flag is only valid if is_debug_types is true.
     We can't read a CU directly from a DWO file: There are required
     attributes in the stub.  */
  unsigned int reading_dwo_directly : 1;

  /* Non-zero if the TU has been read.
     This is used to assist the "Stay in DWO Optimization" for Fission:
     When reading a DWO, it's faster to read TUs from the DWO instead of
     fetching them from random other DWOs (due to comdat folding).
     If the TU has already been read, the optimization is unnecessary
     (and unwise - we don't want to change where gdb thinks the TU lives
     "midflight").
     This flag is only valid if is_debug_types is true.  */
  unsigned int tu_read : 1;

  /* Wrap the following in struct packed instead of bitfields to avoid
     data races when the bitfields end up on the same memory location
     (per C++ memory model).  */

  /* If addresses have been read for this CU (usually from
     .debug_aranges), then this flag is set.  */
  packed<bool, 1> addresses_seen = false;

  /* Flag indicating this compilation unit will be read in before
     any of the current compilation units are processed.  */
  packed<bool, 1> queued;

  /* True if HEADER has been read in.

     Don't access this field directly.  It should be private, but we can't make
     it private at the moment.  */
  mutable packed<bool, 1> m_header_read_in;

  /* A temporary mark bit used when iterating over all CUs in
     expand_symtabs_matching.  */
  packed<unsigned int, 1> mark;

  /* True if we've tried to read the file table.  There will be no
     point in trying to read it again next time.  */
  packed<bool, 1> files_read;

private:
  /* The unit type of this CU.  */
  std::atomic<packed<dwarf_unit_type, 1>> m_unit_type {(dwarf_unit_type)0};

  /* The language of this CU.  */
  std::atomic<packed<language, LANGUAGE_BYTES>> m_lang {language_unknown};

  /* The original DW_LANG_* value of the CU, as provided to us by
     DW_AT_language.  It is interesting to keep this value around in cases where
     we can't use the values from the language enum, as the mapping to them is
     lossy, and, while that is usually fine, things like the index have an
     understandable bias towards not exposing internal GDB structures to the
     outside world, and so prefer to use DWARF constants in their stead. */
  std::atomic<packed<dwarf_source_language, 2>> m_dw_lang
       { (dwarf_source_language) 0 };

public:
  /* True if this CU has been scanned by the indexer; false if
     not.  */
  std::atomic<bool> scanned;

  /* Our index in the unshared "symtabs" vector.  */
  unsigned index = 0;

  /* The section this CU/TU lives in.
     If the DIE refers to a DWO file, this is always the original die,
     not the DWO file.  */
  struct dwarf2_section_info *section = nullptr;

  /* Backlink to the owner of this.  */
  dwarf2_per_bfd *per_bfd = nullptr;

  /* DWARF header of this CU.  Note that dwarf2_cu reads its own version of the
     header, which may differ from this one, since it may pass rcuh_kind::TYPE
     to read_comp_unit_head, whereas for dwarf2_per_cu_data we always pass
     rcuh_kind::COMPILE.

     Don't access this field directly, use the get_header method instead.  It
     should be private, but we can't make it private at the moment.  */
  mutable comp_unit_head m_header;

  /* The file and directory for this CU.  This is cached so that we
     don't need to re-examine the DWO in some situations.  This may be
     nullptr, depending on the CU; for example a partial unit won't
     have one.  */
  std::unique_ptr<file_and_directory> fnd;

  /* The file table.  This can be NULL if there was no file table
     or it's currently not read in.
     NOTE: This points into dwarf2_per_objfile->per_bfd->quick_file_names_table.  */
  struct quick_file_names *file_names = nullptr;

  /* The CUs we import using DW_TAG_imported_unit.  This is filled in
     while reading psymtabs, used to compute the psymtab dependencies,
     and then cleared.  Then it is filled in again while reading full
     symbols, and only deleted when the objfile is destroyed.

     This is also used to work around a difference between the way gold
     generates .gdb_index version <=7 and the way gdb does.  Arguably this
     is a gold bug.  For symbols coming from TUs, gold records in the index
     the CU that includes the TU instead of the TU itself.  This breaks
     dw2_lookup_symbol: It assumes that if the index says symbol X lives
     in CU/TU Y, then one need only expand Y and a subsequent lookup in Y
     will find X.  Alas TUs live in their own symtab, so after expanding CU Y
     we need to look in TU Z to find X.  Fortunately, this is akin to
     DW_TAG_imported_unit, so we just use the same mechanism: For
     .gdb_index version <=7 this also records the TUs that the CU referred
     to.  Concurrently with this change gdb was modified to emit version 8
     indices so we only pay a price for gold generated indices.
     http://sourceware.org/bugzilla/show_bug.cgi?id=15021.

     This currently needs to be a public member due to how
     dwarf2_per_cu_data is allocated and used.  Ideally in future things
     could be refactored to make this private.  Until then please try to
     avoid direct access to this member, and instead use the helper
     functions above.  */
  std::vector <dwarf2_per_cu_data *> *imported_symtabs = nullptr;

  /* Return true of IMPORTED_SYMTABS is empty or not yet allocated.  */
  bool imported_symtabs_empty () const
  {
    return (imported_symtabs == nullptr || imported_symtabs->empty ());
  }

  /* Push P to the back of IMPORTED_SYMTABS, allocated IMPORTED_SYMTABS
     first if required.  */
  void imported_symtabs_push (dwarf2_per_cu_data *p)
  {
    if (imported_symtabs == nullptr)
      imported_symtabs = new std::vector <dwarf2_per_cu_data *>;
    imported_symtabs->push_back (p);
  }

  /* Return the size of IMPORTED_SYMTABS if it is allocated, otherwise
     return 0.  */
  size_t imported_symtabs_size () const
  {
    if (imported_symtabs == nullptr)
      return 0;
    return imported_symtabs->size ();
  }

  /* Delete IMPORTED_SYMTABS and set the pointer back to nullptr.  */
  void imported_symtabs_free ()
  {
    delete imported_symtabs;
    imported_symtabs = nullptr;
  }

  /* Get the header of this per_cu, reading it if necessary.  */
  const comp_unit_head *get_header () const;

  /* Return the address size given in the compilation unit header for
     this CU.  */
  int addr_size () const;

  /* Return the offset size given in the compilation unit header for
     this CU.  */
  int offset_size () const;

  /* Return the DW_FORM_ref_addr size given in the compilation unit
     header for this CU.  */
  int ref_addr_size () const;

  /* Return length of this CU.  */
  unsigned int length () const
  {
    /* Make sure it's set already.  */
    gdb_assert (m_length != 0);
    return m_length;
  }

  void set_length (unsigned int length, bool strict_p = true)
  {
    if (m_length == 0)
      /* Set if not set already.  */
      m_length = length;
    else if (strict_p)
      /* If already set, verify that it's the same value.  */
      gdb_assert (m_length == length);
  }

  /* Return DWARF version number of this CU.  */
  short version () const
  {
    /* Make sure it's set already.  */
    gdb_assert (m_dwarf_version != 0);
    return m_dwarf_version;
  }

  void set_version (short version)
  {
    if (m_dwarf_version == 0)
      /* Set if not set already.  */
      m_dwarf_version = version;
    else
      /* If already set, verify that it's the same value.  */
      gdb_assert (m_dwarf_version == version);
  }

  dwarf_unit_type unit_type (bool strict_p = true) const
  {
    dwarf_unit_type ut = m_unit_type.load ();
    if (strict_p)
      gdb_assert (ut != 0);
    return ut;
  }

  void set_unit_type (dwarf_unit_type unit_type)
  {
    /* Set if not set already.  */
    packed<dwarf_unit_type, 1> nope = (dwarf_unit_type)0;
    if (m_unit_type.compare_exchange_strong (nope, unit_type))
      return;

    /* If already set, verify that it's the same value.  */
    nope = unit_type;
    if (m_unit_type.compare_exchange_strong (nope, unit_type))
      return;
    gdb_assert_not_reached ();
  }

  enum language lang (bool strict_p = true) const
  {
    enum language l = m_lang.load ();
    if (strict_p)
      gdb_assert (l != language_unknown);
    return l;
  }

  /* Return the language of this CU, as a DWARF DW_LANG_* value.  This
     may be 0 in some situations.  */
  dwarf_source_language dw_lang () const
  { return m_dw_lang.load (); }

  /* Set the language of this CU.  LANG is the language in gdb terms,
     and DW_LANG is the language as a DW_LANG_* value.  These may
     differ, as DW_LANG can be 0 for included units, whereas in this
     situation LANG would be set by the importing CU.  */
  void set_lang (enum language lang, dwarf_source_language dw_lang);

  /* Free any cached file names.  */
  void free_cached_file_names ();
};

/* Entry in the signatured_types hash table.  */

struct signatured_type : public dwarf2_per_cu_data
{
  signatured_type (ULONGEST signature)
    : signature (signature)
  {}

  /* The type's signature.  */
  ULONGEST signature;

  /* Offset in the TU of the type's DIE, as read from the TU header.
     If this TU is a DWO stub and the definition lives in a DWO file
     (specified by DW_AT_GNU_dwo_name), this value is unusable.  */
  cu_offset type_offset_in_tu {};

  /* Offset in the section of the type's DIE.
     If the definition lives in a DWO file, this is the offset in the
     .debug_types.dwo section.
     The value is zero until the actual value is known.
     Zero is otherwise not a valid section offset.  */
  sect_offset type_offset_in_section {};

  /* Type units are grouped by their DW_AT_stmt_list entry so that they
     can share them.  This points to the containing symtab.  */
  struct type_unit_group *type_unit_group = nullptr;

  /* Containing DWO unit.
     This field is valid iff per_cu.reading_dwo_directly.  */
  struct dwo_unit *dwo_unit = nullptr;
};

using signatured_type_up = std::unique_ptr<signatured_type>;

/* Some DWARF data can be shared across objfiles who share the same BFD,
   this data is stored in this object.

   Two dwarf2_per_objfile objects representing objfiles sharing the same BFD
   will point to the same instance of dwarf2_per_bfd, unless the BFD requires
   relocation.  */

struct dwarf2_per_bfd
{
  /* Construct a dwarf2_per_bfd for OBFD.  NAMES points to the
     dwarf2 section names, or is NULL if the standard ELF names are
     used.  CAN_COPY is true for formats where symbol
     interposition is possible and so symbol values must follow copy
     relocation rules.  */
  dwarf2_per_bfd (bfd *obfd, const dwarf2_debug_sections *names, bool can_copy);

  ~dwarf2_per_bfd ();

  DISABLE_COPY_AND_ASSIGN (dwarf2_per_bfd);

  /* Return the CU given its index.  */
  dwarf2_per_cu_data *get_cu (int index) const
  {
    return this->all_units[index].get ();
  }

  /* Return the CU given its index in the CU table in the index.  */
  dwarf2_per_cu_data *get_index_cu (int index) const
  {
    if (this->all_comp_units_index_cus.empty ())
      return get_cu (index);

    return this->all_comp_units_index_cus[index];
  }

  dwarf2_per_cu_data *get_index_tu (int index) const
  {
    return this->all_comp_units_index_tus[index];
  }

  /* A convenience function to allocate a dwarf2_per_cu_data.  The
     returned object has its "index" field set properly.  The object
     is allocated on the dwarf2_per_bfd obstack.  */
  dwarf2_per_cu_data_up allocate_per_cu ();

  /* A convenience function to allocate a signatured_type.  The
     returned object has its "index" field set properly.  The object
     is allocated on the dwarf2_per_bfd obstack.  */
  signatured_type_up allocate_signatured_type (ULONGEST signature);

  /* Map all the DWARF section data needed when scanning
     .debug_info.  */
  void map_info_sections (struct objfile *objfile);

private:
  /* This function is mapped across the sections and remembers the
     offset and size of each of the debugging sections we are
     interested in.  */
  void locate_sections (bfd *abfd, asection *sectp,
			const dwarf2_debug_sections &names);

public:
  /* The corresponding BFD.  */
  bfd *obfd;

  /* Objects that can be shared across objfiles may be stored in this
     obstack, while objects that are objfile-specific are stored on
     the objfile obstack.  */
  auto_obstack obstack;

  dwarf2_section_info info {};
  dwarf2_section_info abbrev {};
  dwarf2_section_info line {};
  dwarf2_section_info loc {};
  dwarf2_section_info loclists {};
  dwarf2_section_info macinfo {};
  dwarf2_section_info macro {};
  dwarf2_section_info str {};
  dwarf2_section_info str_offsets {};
  dwarf2_section_info line_str {};
  dwarf2_section_info ranges {};
  dwarf2_section_info rnglists {};
  dwarf2_section_info addr {};
  dwarf2_section_info frame {};
  dwarf2_section_info eh_frame {};
  dwarf2_section_info gdb_index {};
  dwarf2_section_info debug_names {};
  dwarf2_section_info debug_aranges {};

  std::vector<dwarf2_section_info> types;

  /* Table of all the compilation units.  This is used to locate
     the target compilation unit of a particular reference.  */
  std::vector<dwarf2_per_cu_data_up> all_units;

  /* The all_units vector contains both CUs and TUs.  Provide views on the
     vector that are limited to either the CU part or the TU part.  */
  gdb::array_view<dwarf2_per_cu_data_up> all_comp_units;
  gdb::array_view<dwarf2_per_cu_data_up> all_type_units;

  std::vector<dwarf2_per_cu_data*> all_comp_units_index_cus;
  std::vector<dwarf2_per_cu_data*> all_comp_units_index_tus;

  /* Table of struct type_unit_group objects.
     The hash key is the DW_AT_stmt_list value.  */
  htab_up type_unit_groups;

  /* A table mapping .debug_types signatures to its signatured_type entry.
     This is NULL if the .debug_types section hasn't been read in yet.  */
  htab_up signatured_types;

  /* Type unit statistics, to see how well the scaling improvements
     are doing.  */
  struct tu_stats tu_stats {};

  /* A table mapping DW_AT_dwo_name values to struct dwo_file objects.
     This is NULL if the table hasn't been allocated yet.  */
  htab_up dwo_files;

  /* True if we've checked for whether there is a DWP file.  */
  bool dwp_checked = false;

  /* The DWP file if there is one, or NULL.  */
  std::unique_ptr<struct dwp_file> dwp_file;

  /* The shared '.dwz' file, if one exists.  This is used when the
     original data was compressed using 'dwz -m'.  */
  std::optional<std::unique_ptr<struct dwz_file>> dwz_file;

  /* Whether copy relocations are supported by this object format.  */
  bool can_copy;

  /* A flag indicating whether this objfile has a section loaded at a
     VMA of 0.  */
  bool has_section_at_zero = false;

  /* The mapped index, or NULL in the readnow case.  */
  std::unique_ptr<dwarf_scanner_base> index_table;

  /* When using index_table, this keeps track of all quick_file_names entries.
     TUs typically share line table entries with a CU, so we maintain a
     separate table of all line table entries to support the sharing.
     Note that while there can be way more TUs than CUs, we've already
     sorted all the TUs into "type unit groups", grouped by their
     DW_AT_stmt_list value.  Therefore the only sharing done here is with a
     CU and its associated TU group if there is one.  */
  htab_up quick_file_names_table;

  /* The CUs we recently read.  */
  std::vector<dwarf2_per_cu_data *> just_read_cus;

  /* If we loaded the index from an external file, this contains the
     resources associated to the open file, memory mapping, etc.  */
  std::unique_ptr<index_cache_resource> index_cache_res;

  /* Mapping from abstract origin DIE to concrete DIEs that reference it as
     DW_AT_abstract_origin.  */
  std::unordered_map<sect_offset, std::vector<sect_offset>,
		     gdb::hash_enum<sect_offset>>
    abstract_to_concrete;

  /* The address map that is used by the DWARF index code.  */
  struct addrmap *index_addrmap = nullptr;
};

/* An iterator for all_units that is based on index.  This
   approach makes it possible to iterate over all_units safely,
   when some caller in the loop may add new units.  */

class all_units_iterator
{
public:

  all_units_iterator (dwarf2_per_bfd *per_bfd, bool start)
    : m_per_bfd (per_bfd),
      m_index (start ? 0 : per_bfd->all_units.size ())
  {
  }

  all_units_iterator &operator++ ()
  {
    ++m_index;
    return *this;
  }

  dwarf2_per_cu_data *operator* () const
  {
    return m_per_bfd->get_cu (m_index);
  }

  bool operator== (const all_units_iterator &other) const
  {
    return m_index == other.m_index;
  }


  bool operator!= (const all_units_iterator &other) const
  {
    return m_index != other.m_index;
  }

private:

  dwarf2_per_bfd *m_per_bfd;
  size_t m_index;
};

/* A range adapter for the all_units_iterator.  */
class all_units_range
{
public:

  all_units_range (dwarf2_per_bfd *per_bfd)
    : m_per_bfd (per_bfd)
  {
  }

  all_units_iterator begin ()
  {
    return all_units_iterator (m_per_bfd, true);
  }

  all_units_iterator end ()
  {
    return all_units_iterator (m_per_bfd, false);
  }

private:

  dwarf2_per_bfd *m_per_bfd;
};

/* This is the per-objfile data associated with a type_unit_group.  */

struct type_unit_group_unshareable
{
  /* The compunit symtab.
     Type units in a group needn't all be defined in the same source file,
     so we create an essentially anonymous symtab as the compunit symtab.  */
  struct compunit_symtab *compunit_symtab = nullptr;

  /* The number of symtabs from the line header.
     The value here must match line_header.num_file_names.  */
  unsigned int num_symtabs = 0;

  /* The symbol tables for this TU (obtained from the files listed in
     DW_AT_stmt_list).
     WARNING: The order of entries here must match the order of entries
     in the line header.  After the first TU using this type_unit_group, the
     line header for the subsequent TUs is recreated from this.  This is done
     because we need to use the same symtabs for each TU using the same
     DW_AT_stmt_list value.  Also note that symtabs may be repeated here,
     there's no guarantee the line header doesn't have duplicate entries.  */
  struct symtab **symtabs = nullptr;
};

/* Collection of data recorded per objfile.
   This hangs off of dwarf2_objfile_data_key.

   Some DWARF data cannot (currently) be shared across objfiles.  Such
   data is stored in this object.  */

struct dwarf2_per_objfile
{
  dwarf2_per_objfile (struct objfile *objfile, dwarf2_per_bfd *per_bfd)
    : objfile (objfile), per_bfd (per_bfd)
  {}

  ~dwarf2_per_objfile ();

  /* Return pointer to string at .debug_line_str offset as read from BUF.
     BUF is assumed to be in a compilation unit described by CU_HEADER.
     Return *BYTES_READ_PTR count of bytes read from BUF.  */
  const char *read_line_string (const gdb_byte *buf,
				const struct comp_unit_head *cu_header,
				unsigned int *bytes_read_ptr);

  /* Return pointer to string at .debug_line_str offset as read from BUF.
     The offset_size is OFFSET_SIZE.  */
  const char *read_line_string (const gdb_byte *buf,
				unsigned int offset_size);

  /* Return true if the symtab corresponding to PER_CU has been set,
     false otherwise.  */
  bool symtab_set_p (const dwarf2_per_cu_data *per_cu) const;

  /* Return the compunit_symtab associated to PER_CU, if it has been created.  */
  compunit_symtab *get_symtab (const dwarf2_per_cu_data *per_cu) const;

  /* Set the compunit_symtab associated to PER_CU.  */
  void set_symtab (const dwarf2_per_cu_data *per_cu, compunit_symtab *symtab);

  /* Get the type_unit_group_unshareable corresponding to TU_GROUP.  If one
     does not exist, create it.  */
  type_unit_group_unshareable *get_type_unit_group_unshareable
    (type_unit_group *tu_group);

  struct type *get_type_for_signatured_type (signatured_type *sig_type) const;

  void set_type_for_signatured_type (signatured_type *sig_type,
				     struct type *type);

  /* Get the dwarf2_cu matching PER_CU for this objfile.  */
  dwarf2_cu *get_cu (dwarf2_per_cu_data *per_cu);

  /* Set the dwarf2_cu matching PER_CU for this objfile.  */
  void set_cu (dwarf2_per_cu_data *per_cu, std::unique_ptr<dwarf2_cu> cu);

  /* Remove/free the dwarf2_cu matching PER_CU for this objfile.  */
  void remove_cu (dwarf2_per_cu_data *per_cu);

  /* Free all cached compilation units.  */
  void remove_all_cus ();

  /* Increase the age counter on each CU compilation unit and free
     any that are too old.  */
  void age_comp_units ();

  /* Apply any needed adjustments to ADDR, returning an adjusted but
     still unrelocated address.  */
  unrelocated_addr adjust (unrelocated_addr addr);

  /* Apply any needed adjustments to ADDR and then relocate the
     address according to the objfile's section offsets, returning a
     relocated address.  */
  CORE_ADDR relocate (unrelocated_addr addr);

  /* Back link.  */
  struct objfile *objfile;

  /* Pointer to the data that is (possibly) shared between this objfile and
     other objfiles backed by the same BFD.  */
  struct dwarf2_per_bfd *per_bfd;

  /* Table mapping type DIEs to their struct type *.
     This is nullptr if not allocated yet.
     The mapping is done via (CU/TU + DIE offset) -> type.  */
  htab_up die_type_hash;

  /* Table containing line_header indexed by offset and offset_in_dwz.  */
  htab_up line_header_hash;

  /* The CU containing the m_builder in scope.  */
  dwarf2_cu *sym_cu = nullptr;

  /* CUs that are queued to be read.  */
  std::optional<std::queue<dwarf2_queue_item>> queue;

private:
  /* Hold the corresponding compunit_symtab for each CU or TU.  This
     is indexed by dwarf2_per_cu_data::index.  A NULL value means
     that the CU/TU has not been expanded yet.  */
  std::vector<compunit_symtab *> m_symtabs;

  /* Map from a type unit group to the corresponding unshared
     structure.  */
  typedef std::unique_ptr<type_unit_group_unshareable>
    type_unit_group_unshareable_up;

  std::unordered_map<type_unit_group *, type_unit_group_unshareable_up>
    m_type_units;

  /* Map from signatured types to the corresponding struct type.  */
  std::unordered_map<signatured_type *, struct type *> m_type_map;

  /* Map from the objfile-independent dwarf2_per_cu_data instances to the
     corresponding objfile-dependent dwarf2_cu instances.  */
  std::unordered_map<dwarf2_per_cu_data *,
		     std::unique_ptr<dwarf2_cu>> m_dwarf2_cus;
};

/* Converts DWARF language names to GDB language names.  */

enum language dwarf_lang_to_enum_language (unsigned int lang);

/* Get the dwarf2_per_objfile associated to OBJFILE.  */

dwarf2_per_objfile *get_dwarf2_per_objfile (struct objfile *objfile);

/* Return the type of the DIE at DIE_OFFSET in the CU named by
   PER_CU.  */

struct type *dwarf2_get_die_type (cu_offset die_offset,
				  dwarf2_per_cu_data *per_cu,
				  dwarf2_per_objfile *per_objfile);

/* Given an index in .debug_addr, fetch the value.
   NOTE: This can be called during dwarf expression evaluation,
   long after the debug information has been read, and thus per_cu->cu
   may no longer exist.  */

unrelocated_addr dwarf2_read_addr_index (dwarf2_per_cu_data *per_cu,
					 dwarf2_per_objfile *per_objfile,
					 unsigned int addr_index);

/* Return DWARF block referenced by DW_AT_location of DIE at SECT_OFF at PER_CU.
   Returned value is intended for DW_OP_call*.  Returned
   dwarf2_locexpr_baton->data has lifetime of
   PER_CU->DWARF2_PER_OBJFILE->OBJFILE.  */

struct dwarf2_locexpr_baton dwarf2_fetch_die_loc_sect_off
  (sect_offset sect_off, dwarf2_per_cu_data *per_cu,
   dwarf2_per_objfile *per_objfile,
   gdb::function_view<CORE_ADDR ()> get_frame_pc,
   bool resolve_abstract_p = false);

/* Like dwarf2_fetch_die_loc_sect_off, but take a CU
   offset.  */

struct dwarf2_locexpr_baton dwarf2_fetch_die_loc_cu_off
  (cu_offset offset_in_cu, dwarf2_per_cu_data *per_cu,
   dwarf2_per_objfile *per_objfile,
   gdb::function_view<CORE_ADDR ()> get_frame_pc);

/* If the DIE at SECT_OFF in PER_CU has a DW_AT_const_value, return a
   pointer to the constant bytes and set LEN to the length of the
   data.  If memory is needed, allocate it on OBSTACK.  If the DIE
   does not have a DW_AT_const_value, return NULL.  */

extern const gdb_byte *dwarf2_fetch_constant_bytes
  (sect_offset sect_off, dwarf2_per_cu_data *per_cu,
   dwarf2_per_objfile *per_objfile, obstack *obstack,
   LONGEST *len);

/* Return the type of the die at SECT_OFF in PER_CU.  Return NULL if no
   valid type for this die is found.  If VAR_NAME is non-null, and if
   the DIE in question is a variable declaration (definitions are
   excluded), then *VAR_NAME is set to the variable's name.  */

struct type *dwarf2_fetch_die_type_sect_off
  (sect_offset sect_off, dwarf2_per_cu_data *per_cu,
   dwarf2_per_objfile *per_objfile,
   const char **var_name = nullptr);

/* When non-zero, dump line number entries as they are read in.  */
extern unsigned int dwarf_line_debug;

/* Dwarf2 sections that can be accessed by dwarf2_get_section_info.  */
enum dwarf2_section_enum {
  DWARF2_DEBUG_FRAME,
  DWARF2_EH_FRAME
};

extern void dwarf2_get_section_info (struct objfile *,
				     enum dwarf2_section_enum,
				     asection **, const gdb_byte **,
				     bfd_size_type *);

/* Return true if the producer of the inferior is clang.  */
extern bool producer_is_clang (struct dwarf2_cu *cu);

/* Interface for DWARF indexing methods.  */

struct dwarf2_base_index_functions : public quick_symbol_functions
{
  bool has_symbols (struct objfile *objfile) override;

  bool has_unexpanded_symtabs (struct objfile *objfile) override;

  struct symtab *find_last_source_symtab (struct objfile *objfile) override;

  void forget_cached_source_info (struct objfile *objfile) override;

  enum language lookup_global_symbol_language (struct objfile *objfile,
					       const char *name,
					       domain_enum domain,
					       bool *symbol_found_p) override
  {
    *symbol_found_p = false;
    return language_unknown;
  }

  void print_stats (struct objfile *objfile, bool print_bcache) override;

  void expand_all_symtabs (struct objfile *objfile) override;

  /* A helper function that finds the per-cu object from an "adjusted"
     PC -- a PC with the base text offset removed.  */
  virtual dwarf2_per_cu_data *find_per_cu (dwarf2_per_bfd *per_bfd,
					   unrelocated_addr adjusted_pc);

  struct compunit_symtab *find_pc_sect_compunit_symtab
    (struct objfile *objfile, struct bound_minimal_symbol msymbol,
     CORE_ADDR pc, struct obj_section *section, int warn_if_readin)
       override;

  struct compunit_symtab *find_compunit_symtab_by_address
    (struct objfile *objfile, CORE_ADDR address) override
  {
    return nullptr;
  }

  void map_symbol_filenames (struct objfile *objfile,
			     gdb::function_view<symbol_filename_ftype> fun,
			     bool need_fullname) override;
};

/* If FILE_MATCHER is NULL or if PER_CU has
   dwarf2_per_cu_quick_data::MARK set (see
   dw_expand_symtabs_matching_file_matcher), expand the CU and call
   EXPANSION_NOTIFY on it.  */

extern bool dw2_expand_symtabs_matching_one
  (dwarf2_per_cu_data *per_cu,
   dwarf2_per_objfile *per_objfile,
   gdb::function_view<expand_symtabs_file_matcher_ftype> file_matcher,
   gdb::function_view<expand_symtabs_exp_notify_ftype> expansion_notify);

/* Helper for dw2_expand_symtabs_matching that works with a
   mapped_index_base instead of the containing objfile.  This is split
   to a separate function in order to be able to unit test the
   name_components matching using a mock mapped_index_base.  For each
   symbol name that matches, calls MATCH_CALLBACK, passing it the
   symbol's index in the mapped_index_base symbol table.  */

extern bool dw2_expand_symtabs_matching_symbol
  (mapped_index_base &index,
   const lookup_name_info &lookup_name_in,
   gdb::function_view<expand_symtabs_symbol_matcher_ftype> symbol_matcher,
   gdb::function_view<bool (offset_type)> match_callback,
   dwarf2_per_objfile *per_objfile);

/* If FILE_MATCHER is non-NULL, set all the
   dwarf2_per_cu_quick_data::MARK of the current DWARF2_PER_OBJFILE
   that match FILE_MATCHER.  */

extern void dw_expand_symtabs_matching_file_matcher
  (dwarf2_per_objfile *per_objfile,
   gdb::function_view<expand_symtabs_file_matcher_ftype> file_matcher);

/* Return pointer to string at .debug_str offset STR_OFFSET.  */

extern const char *read_indirect_string_at_offset
  (dwarf2_per_objfile *per_objfile, LONGEST str_offset);

/* Allocate a hash table for signatured types.  */

extern htab_up allocate_signatured_type_table ();

/* Return a new dwarf2_per_cu_data allocated on the per-bfd
   obstack, and constructed with the specified field values.  */

extern dwarf2_per_cu_data_up create_cu_from_index_list
  (dwarf2_per_bfd *per_bfd, struct dwarf2_section_info *section,
   int is_dwz, sect_offset sect_off, ULONGEST length);

/* Initialize the views on all_units.  */

extern void finalize_all_units (dwarf2_per_bfd *per_bfd);

/* Create a list of all compilation units in OBJFILE.  */

extern void create_all_units (dwarf2_per_objfile *per_objfile);

/* Create a quick_file_names hash table.  */

extern htab_up create_quick_file_names_table (unsigned int nr_initial_entries);

#endif /* DWARF2READ_H */
