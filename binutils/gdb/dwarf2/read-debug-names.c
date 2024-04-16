/* Reading code for .debug_names

   Copyright (C) 2023-2024 Free Software Foundation, Inc.

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
#include "read-debug-names.h"
#include "dwarf2/aranges.h"

#include "complaints.h"
#include "cp-support.h"
#include "dwz.h"
#include "mapped-index.h"
#include "read.h"
#include "stringify.h"

/* A description of the mapped .debug_names.
   Uninitialized map has CU_COUNT 0.  */

struct mapped_debug_names final : public mapped_index_base
{
  bfd_endian dwarf5_byte_order;
  bool dwarf5_is_dwarf64;
  bool augmentation_is_gdb;
  uint8_t offset_size;
  uint32_t cu_count = 0;
  uint32_t tu_count, bucket_count, name_count;
  const gdb_byte *cu_table_reordered, *tu_table_reordered;
  const uint32_t *bucket_table_reordered, *hash_table_reordered;
  const gdb_byte *name_table_string_offs_reordered;
  const gdb_byte *name_table_entry_offs_reordered;
  const gdb_byte *entry_pool;

  struct index_val
  {
    ULONGEST dwarf_tag;
    struct attr
    {
      /* Attribute name DW_IDX_*.  */
      ULONGEST dw_idx;

      /* Attribute form DW_FORM_*.  */
      ULONGEST form;

      /* Value if FORM is DW_FORM_implicit_const.  */
      LONGEST implicit_const;
    };
    std::vector<attr> attr_vec;
  };

  std::unordered_map<ULONGEST, index_val> abbrev_map;

  const char *namei_to_name
    (uint32_t namei, dwarf2_per_objfile *per_objfile) const;

  /* Implementation of the mapped_index_base virtual interface, for
     the name_components cache.  */

  const char *symbol_name_at
    (offset_type idx, dwarf2_per_objfile *per_objfile) const override
  { return namei_to_name (idx, per_objfile); }

  size_t symbol_name_count () const override
  { return this->name_count; }

  quick_symbol_functions_up make_quick_functions () const override;
};

struct dwarf2_debug_names_index : public dwarf2_base_index_functions
{
  void dump (struct objfile *objfile) override;

  bool expand_symtabs_matching
    (struct objfile *objfile,
     gdb::function_view<expand_symtabs_file_matcher_ftype> file_matcher,
     const lookup_name_info *lookup_name,
     gdb::function_view<expand_symtabs_symbol_matcher_ftype> symbol_matcher,
     gdb::function_view<expand_symtabs_exp_notify_ftype> expansion_notify,
     block_search_flags search_flags,
     domain_enum domain,
     enum search_domain kind) override;
};

quick_symbol_functions_up
mapped_debug_names::make_quick_functions () const
{
  return quick_symbol_functions_up (new dwarf2_debug_names_index);
}

/* Check the signatured type hash table from .debug_names.  */

static bool
check_signatured_type_table_from_debug_names
  (dwarf2_per_objfile *per_objfile,
   const mapped_debug_names &map,
   struct dwarf2_section_info *section)
{
  struct objfile *objfile = per_objfile->objfile;
  dwarf2_per_bfd *per_bfd = per_objfile->per_bfd;
  int nr_cus = per_bfd->all_comp_units.size ();
  int nr_cus_tus = per_bfd->all_units.size ();

  section->read (objfile);

  uint32_t j = nr_cus;
  for (uint32_t i = 0; i < map.tu_count; ++i)
    {
      sect_offset sect_off
	= (sect_offset) (extract_unsigned_integer
			 (map.tu_table_reordered + i * map.offset_size,
			  map.offset_size,
			  map.dwarf5_byte_order));

      bool found = false;
      for (; j < nr_cus_tus; j++)
	if (per_bfd->get_cu (j)->sect_off == sect_off)
	  {
	    found = true;
	    break;
	  }
      if (!found)
	{
	  warning (_("Section .debug_names has incorrect entry in TU table,"
		     " ignoring .debug_names."));
	  return false;
	}
      per_bfd->all_comp_units_index_tus.push_back (per_bfd->get_cu (j));
    }
  return true;
}

/* Read the address map data from DWARF-5 .debug_aranges, and use it to
   populate the index_addrmap.  */

static void
create_addrmap_from_aranges (dwarf2_per_objfile *per_objfile,
			     struct dwarf2_section_info *section)
{
  dwarf2_per_bfd *per_bfd = per_objfile->per_bfd;

  addrmap_mutable mutable_map;
  deferred_warnings warnings;

  section->read (per_objfile->objfile);
  if (read_addrmap_from_aranges (per_objfile, section, &mutable_map,
				 &warnings))
    per_bfd->index_addrmap
      = new (&per_bfd->obstack) addrmap_fixed (&per_bfd->obstack,
					       &mutable_map);

  warnings.emit ();
}

/* DWARF-5 debug_names reader.  */

/* DWARF-5 augmentation string for GDB's DW_IDX_GNU_* extension.  */
static const gdb_byte dwarf5_augmentation[] = { 'G', 'D', 'B', 0 };

/* A helper function that reads the .debug_names section in SECTION
   and fills in MAP.  FILENAME is the name of the file containing the
   section; it is used for error reporting.

   Returns true if all went well, false otherwise.  */

static bool
read_debug_names_from_section (struct objfile *objfile,
			       const char *filename,
			       struct dwarf2_section_info *section,
			       mapped_debug_names &map)
{
  if (section->empty ())
    return false;

  /* Older elfutils strip versions could keep the section in the main
     executable while splitting it for the separate debug info file.  */
  if ((section->get_flags () & SEC_HAS_CONTENTS) == 0)
    return false;

  section->read (objfile);

  map.dwarf5_byte_order = gdbarch_byte_order (objfile->arch ());

  const gdb_byte *addr = section->buffer;

  bfd *const abfd = section->get_bfd_owner ();

  unsigned int bytes_read;
  LONGEST length = read_initial_length (abfd, addr, &bytes_read);
  addr += bytes_read;

  map.dwarf5_is_dwarf64 = bytes_read != 4;
  map.offset_size = map.dwarf5_is_dwarf64 ? 8 : 4;
  if (bytes_read + length != section->size)
    {
      /* There may be multiple per-CU indices.  */
      warning (_("Section .debug_names in %s length %s does not match "
		 "section length %s, ignoring .debug_names."),
	       filename, plongest (bytes_read + length),
	       pulongest (section->size));
      return false;
    }

  /* The version number.  */
  uint16_t version = read_2_bytes (abfd, addr);
  addr += 2;
  if (version != 5)
    {
      warning (_("Section .debug_names in %s has unsupported version %d, "
		 "ignoring .debug_names."),
	       filename, version);
      return false;
    }

  /* Padding.  */
  uint16_t padding = read_2_bytes (abfd, addr);
  addr += 2;
  if (padding != 0)
    {
      warning (_("Section .debug_names in %s has unsupported padding %d, "
		 "ignoring .debug_names."),
	       filename, padding);
      return false;
    }

  /* comp_unit_count - The number of CUs in the CU list.  */
  map.cu_count = read_4_bytes (abfd, addr);
  addr += 4;

  /* local_type_unit_count - The number of TUs in the local TU
     list.  */
  map.tu_count = read_4_bytes (abfd, addr);
  addr += 4;

  /* foreign_type_unit_count - The number of TUs in the foreign TU
     list.  */
  uint32_t foreign_tu_count = read_4_bytes (abfd, addr);
  addr += 4;
  if (foreign_tu_count != 0)
    {
      warning (_("Section .debug_names in %s has unsupported %lu foreign TUs, "
		 "ignoring .debug_names."),
	       filename, static_cast<unsigned long> (foreign_tu_count));
      return false;
    }

  /* bucket_count - The number of hash buckets in the hash lookup
     table.  */
  map.bucket_count = read_4_bytes (abfd, addr);
  addr += 4;

  /* name_count - The number of unique names in the index.  */
  map.name_count = read_4_bytes (abfd, addr);
  addr += 4;

  /* abbrev_table_size - The size in bytes of the abbreviations
     table.  */
  uint32_t abbrev_table_size = read_4_bytes (abfd, addr);
  addr += 4;

  /* augmentation_string_size - The size in bytes of the augmentation
     string.  This value is rounded up to a multiple of 4.  */
  uint32_t augmentation_string_size = read_4_bytes (abfd, addr);
  addr += 4;
  map.augmentation_is_gdb = ((augmentation_string_size
			      == sizeof (dwarf5_augmentation))
			     && memcmp (addr, dwarf5_augmentation,
					sizeof (dwarf5_augmentation)) == 0);
  augmentation_string_size += (-augmentation_string_size) & 3;
  addr += augmentation_string_size;

  /* List of CUs */
  map.cu_table_reordered = addr;
  addr += map.cu_count * map.offset_size;

  /* List of Local TUs */
  map.tu_table_reordered = addr;
  addr += map.tu_count * map.offset_size;

  /* Hash Lookup Table */
  map.bucket_table_reordered = reinterpret_cast<const uint32_t *> (addr);
  addr += map.bucket_count * 4;
  map.hash_table_reordered = reinterpret_cast<const uint32_t *> (addr);
  addr += map.name_count * 4;

  /* Name Table */
  map.name_table_string_offs_reordered = addr;
  addr += map.name_count * map.offset_size;
  map.name_table_entry_offs_reordered = addr;
  addr += map.name_count * map.offset_size;

  const gdb_byte *abbrev_table_start = addr;
  for (;;)
    {
      const ULONGEST index_num = read_unsigned_leb128 (abfd, addr, &bytes_read);
      addr += bytes_read;
      if (index_num == 0)
	break;

      const auto insertpair
	= map.abbrev_map.emplace (index_num, mapped_debug_names::index_val ());
      if (!insertpair.second)
	{
	  warning (_("Section .debug_names in %s has duplicate index %s, "
		     "ignoring .debug_names."),
		   filename, pulongest (index_num));
	  return false;
	}
      mapped_debug_names::index_val &indexval = insertpair.first->second;
      indexval.dwarf_tag = read_unsigned_leb128 (abfd, addr, &bytes_read);
      addr += bytes_read;

      for (;;)
	{
	  mapped_debug_names::index_val::attr attr;
	  attr.dw_idx = read_unsigned_leb128 (abfd, addr, &bytes_read);
	  addr += bytes_read;
	  attr.form = read_unsigned_leb128 (abfd, addr, &bytes_read);
	  addr += bytes_read;
	  if (attr.form == DW_FORM_implicit_const)
	    {
	      attr.implicit_const = read_signed_leb128 (abfd, addr,
							&bytes_read);
	      addr += bytes_read;
	    }
	  if (attr.dw_idx == 0 && attr.form == 0)
	    break;
	  indexval.attr_vec.push_back (std::move (attr));
	}
    }
  if (addr != abbrev_table_start + abbrev_table_size)
    {
      warning (_("Section .debug_names in %s has abbreviation_table "
		 "of size %s vs. written as %u, ignoring .debug_names."),
	       filename, plongest (addr - abbrev_table_start),
	       abbrev_table_size);
      return false;
    }
  map.entry_pool = addr;

  return true;
}

/* A helper for check_cus_from_debug_names that handles the MAP's CU
   list.  */

static bool
check_cus_from_debug_names_list (dwarf2_per_bfd *per_bfd,
				 const mapped_debug_names &map,
				 dwarf2_section_info &section,
				 bool is_dwz)
{
  int nr_cus = per_bfd->all_comp_units.size ();

  if (!map.augmentation_is_gdb)
    {
      uint32_t j = 0;
      for (uint32_t i = 0; i < map.cu_count; ++i)
	{
	  sect_offset sect_off
	    = (sect_offset) (extract_unsigned_integer
			     (map.cu_table_reordered + i * map.offset_size,
			      map.offset_size,
			      map.dwarf5_byte_order));
	  bool found = false;
	  for (; j < nr_cus; j++)
	    if (per_bfd->get_cu (j)->sect_off == sect_off)
	      {
		found = true;
		break;
	      }
	  if (!found)
	    {
	      warning (_("Section .debug_names has incorrect entry in CU table,"
			 " ignoring .debug_names."));
	      return false;
	    }
	  per_bfd->all_comp_units_index_cus.push_back (per_bfd->get_cu (j));
	}
      return true;
    }

  if (map.cu_count != nr_cus)
    {
      warning (_("Section .debug_names has incorrect number of CUs in CU table,"
		 " ignoring .debug_names."));
      return false;
    }

  for (uint32_t i = 0; i < map.cu_count; ++i)
    {
      sect_offset sect_off
	= (sect_offset) (extract_unsigned_integer
			 (map.cu_table_reordered + i * map.offset_size,
			  map.offset_size,
			  map.dwarf5_byte_order));
      if (sect_off != per_bfd->get_cu (i)->sect_off)
	{
	  warning (_("Section .debug_names has incorrect entry in CU table,"
		     " ignoring .debug_names."));
	  return false;
	}
    }

  return true;
}

/* Read the CU list from the mapped index, and use it to create all
   the CU objects for this dwarf2_per_objfile.  */

static bool
check_cus_from_debug_names (dwarf2_per_bfd *per_bfd,
			    const mapped_debug_names &map,
			    const mapped_debug_names &dwz_map)
{
  if (!check_cus_from_debug_names_list (per_bfd, map, per_bfd->info,
					false /* is_dwz */))
    return false;

  if (dwz_map.cu_count == 0)
    return true;

  dwz_file *dwz = dwarf2_get_dwz_file (per_bfd);
  return check_cus_from_debug_names_list (per_bfd, dwz_map, dwz->info,
					  true /* is_dwz */);
}

/* See read-debug-names.h.  */

bool
dwarf2_read_debug_names (dwarf2_per_objfile *per_objfile)
{
  auto map = std::make_unique<mapped_debug_names> ();
  mapped_debug_names dwz_map;
  struct objfile *objfile = per_objfile->objfile;
  dwarf2_per_bfd *per_bfd = per_objfile->per_bfd;

  if (!read_debug_names_from_section (objfile, objfile_name (objfile),
				      &per_bfd->debug_names, *map))
    return false;

  /* Don't use the index if it's empty.  */
  if (map->name_count == 0)
    return false;

  /* If there is a .dwz file, read it so we can get its CU list as
     well.  */
  dwz_file *dwz = dwarf2_get_dwz_file (per_bfd);
  if (dwz != NULL)
    {
      if (!read_debug_names_from_section (objfile,
					  bfd_get_filename (dwz->dwz_bfd.get ()),
					  &dwz->debug_names, dwz_map))
	{
	  warning (_("could not read '.debug_names' section from %s; skipping"),
		   bfd_get_filename (dwz->dwz_bfd.get ()));
	  return false;
	}
    }

  create_all_units (per_objfile);
  if (!check_cus_from_debug_names (per_bfd, *map, dwz_map))
    {
      per_bfd->all_units.clear ();
      return false;
    }

  if (map->tu_count != 0)
    {
      /* We can only handle a single .debug_types when we have an
	 index.  */
      if (per_bfd->types.size () > 1)
	{
	  per_bfd->all_units.clear ();
	  return false;
	}

      dwarf2_section_info *section
	= (per_bfd->types.size () == 1
	   ? &per_bfd->types[0]
	   : &per_bfd->info);

      if (!check_signatured_type_table_from_debug_names
	     (per_objfile, *map, section))
	{
	  per_bfd->all_units.clear ();
	  return false;
	}
    }

  create_addrmap_from_aranges (per_objfile, &per_bfd->debug_aranges);

  per_bfd->index_table = std::move (map);
  per_bfd->quick_file_names_table =
    create_quick_file_names_table (per_bfd->all_units.size ());

  return true;
}

/* Type used to manage iterating over all CUs looking for a symbol for
   .debug_names.  */

class dw2_debug_names_iterator
{
public:
  dw2_debug_names_iterator (const mapped_debug_names &map,
			    block_search_flags block_index,
			    domain_enum domain,
			    const char *name, dwarf2_per_objfile *per_objfile)
    : m_map (map), m_block_index (block_index), m_domain (domain),
      m_addr (find_vec_in_debug_names (map, name, per_objfile)),
      m_per_objfile (per_objfile)
  {}

  dw2_debug_names_iterator (const mapped_debug_names &map,
			    search_domain search, uint32_t namei,
			    dwarf2_per_objfile *per_objfile,
			    domain_enum domain = UNDEF_DOMAIN)
    : m_map (map),
      m_domain (domain),
      m_search (search),
      m_addr (find_vec_in_debug_names (map, namei, per_objfile)),
      m_per_objfile (per_objfile)
  {}

  dw2_debug_names_iterator (const mapped_debug_names &map,
			    block_search_flags block_index, domain_enum domain,
			    uint32_t namei, dwarf2_per_objfile *per_objfile)
    : m_map (map), m_block_index (block_index), m_domain (domain),
      m_addr (find_vec_in_debug_names (map, namei, per_objfile)),
      m_per_objfile (per_objfile)
  {}

  /* Return the next matching CU or NULL if there are no more.  */
  dwarf2_per_cu_data *next ();

private:
  static const gdb_byte *find_vec_in_debug_names (const mapped_debug_names &map,
						  const char *name,
						  dwarf2_per_objfile *per_objfile);
  static const gdb_byte *find_vec_in_debug_names (const mapped_debug_names &map,
						  uint32_t namei,
						  dwarf2_per_objfile *per_objfile);

  /* The internalized form of .debug_names.  */
  const mapped_debug_names &m_map;

  /* Restrict the search to these blocks.  */
  block_search_flags m_block_index = (SEARCH_GLOBAL_BLOCK
				      | SEARCH_STATIC_BLOCK);

  /* The kind of symbol we're looking for.  */
  const domain_enum m_domain = UNDEF_DOMAIN;
  const search_domain m_search = ALL_DOMAIN;

  /* The list of CUs from the index entry of the symbol, or NULL if
     not found.  */
  const gdb_byte *m_addr;

  dwarf2_per_objfile *m_per_objfile;
};

const char *
mapped_debug_names::namei_to_name
  (uint32_t namei, dwarf2_per_objfile *per_objfile) const
{
  const ULONGEST namei_string_offs
    = extract_unsigned_integer ((name_table_string_offs_reordered
				 + namei * offset_size),
				offset_size,
				dwarf5_byte_order);
  return read_indirect_string_at_offset (per_objfile, namei_string_offs);
}

/* Find a slot in .debug_names for the object named NAME.  If NAME is
   found, return pointer to its pool data.  If NAME cannot be found,
   return NULL.  */

const gdb_byte *
dw2_debug_names_iterator::find_vec_in_debug_names
  (const mapped_debug_names &map, const char *name,
   dwarf2_per_objfile *per_objfile)
{
  int (*cmp) (const char *, const char *);

  gdb::unique_xmalloc_ptr<char> without_params;
  if (current_language->la_language == language_cplus
      || current_language->la_language == language_fortran
      || current_language->la_language == language_d)
    {
      /* NAME is already canonical.  Drop any qualifiers as
	 .debug_names does not contain any.  */

      if (strchr (name, '(') != NULL)
	{
	  without_params = cp_remove_params (name);
	  if (without_params != NULL)
	    name = without_params.get ();
	}
    }

  cmp = (case_sensitivity == case_sensitive_on ? strcmp : strcasecmp);

  const uint32_t full_hash = dwarf5_djb_hash (name);
  uint32_t namei
    = extract_unsigned_integer (reinterpret_cast<const gdb_byte *>
				(map.bucket_table_reordered
				 + (full_hash % map.bucket_count)), 4,
				map.dwarf5_byte_order);
  if (namei == 0)
    return NULL;
  --namei;
  if (namei >= map.name_count)
    {
      complaint (_("Wrong .debug_names with name index %u but name_count=%u "
		   "[in module %s]"),
		 namei, map.name_count,
		 objfile_name (per_objfile->objfile));
      return NULL;
    }

  for (;;)
    {
      const uint32_t namei_full_hash
	= extract_unsigned_integer (reinterpret_cast<const gdb_byte *>
				    (map.hash_table_reordered + namei), 4,
				    map.dwarf5_byte_order);
      if (full_hash % map.bucket_count != namei_full_hash % map.bucket_count)
	return NULL;

      if (full_hash == namei_full_hash)
	{
	  const char *const namei_string = map.namei_to_name (namei, per_objfile);

#if 0 /* An expensive sanity check.  */
	  if (namei_full_hash != dwarf5_djb_hash (namei_string))
	    {
	      complaint (_("Wrong .debug_names hash for string at index %u "
			   "[in module %s]"),
			 namei, objfile_name (dwarf2_per_objfile->objfile));
	      return NULL;
	    }
#endif

	  if (cmp (namei_string, name) == 0)
	    {
	      const ULONGEST namei_entry_offs
		= extract_unsigned_integer ((map.name_table_entry_offs_reordered
					     + namei * map.offset_size),
					    map.offset_size, map.dwarf5_byte_order);
	      return map.entry_pool + namei_entry_offs;
	    }
	}

      ++namei;
      if (namei >= map.name_count)
	return NULL;
    }
}

const gdb_byte *
dw2_debug_names_iterator::find_vec_in_debug_names
  (const mapped_debug_names &map, uint32_t namei, dwarf2_per_objfile *per_objfile)
{
  if (namei >= map.name_count)
    {
      complaint (_("Wrong .debug_names with name index %u but name_count=%u "
		   "[in module %s]"),
		 namei, map.name_count,
		 objfile_name (per_objfile->objfile));
      return NULL;
    }

  const ULONGEST namei_entry_offs
    = extract_unsigned_integer ((map.name_table_entry_offs_reordered
				 + namei * map.offset_size),
				map.offset_size, map.dwarf5_byte_order);
  return map.entry_pool + namei_entry_offs;
}

/* See dw2_debug_names_iterator.  */

dwarf2_per_cu_data *
dw2_debug_names_iterator::next ()
{
  if (m_addr == NULL)
    return NULL;

  dwarf2_per_bfd *per_bfd = m_per_objfile->per_bfd;
  struct objfile *objfile = m_per_objfile->objfile;
  bfd *const abfd = objfile->obfd.get ();

 again:

  unsigned int bytes_read;
  const ULONGEST abbrev = read_unsigned_leb128 (abfd, m_addr, &bytes_read);
  m_addr += bytes_read;
  if (abbrev == 0)
    return NULL;

  const auto indexval_it = m_map.abbrev_map.find (abbrev);
  if (indexval_it == m_map.abbrev_map.cend ())
    {
      complaint (_("Wrong .debug_names undefined abbrev code %s "
		   "[in module %s]"),
		 pulongest (abbrev), objfile_name (objfile));
      return NULL;
    }
  const mapped_debug_names::index_val &indexval = indexval_it->second;
  enum class symbol_linkage {
    unknown,
    static_,
    extern_,
  } symbol_linkage_ = symbol_linkage::unknown;
  dwarf2_per_cu_data *per_cu = NULL;
  for (const mapped_debug_names::index_val::attr &attr : indexval.attr_vec)
    {
      ULONGEST ull;
      switch (attr.form)
	{
	case DW_FORM_implicit_const:
	  ull = attr.implicit_const;
	  break;
	case DW_FORM_flag_present:
	  ull = 1;
	  break;
	case DW_FORM_udata:
	  ull = read_unsigned_leb128 (abfd, m_addr, &bytes_read);
	  m_addr += bytes_read;
	  break;
	case DW_FORM_ref4:
	  ull = read_4_bytes (abfd, m_addr);
	  m_addr += 4;
	  break;
	case DW_FORM_ref8:
	  ull = read_8_bytes (abfd, m_addr);
	  m_addr += 8;
	  break;
	case DW_FORM_ref_sig8:
	  ull = read_8_bytes (abfd, m_addr);
	  m_addr += 8;
	  break;
	default:
	  complaint (_("Unsupported .debug_names form %s [in module %s]"),
		     dwarf_form_name (attr.form),
		     objfile_name (objfile));
	  return NULL;
	}
      switch (attr.dw_idx)
	{
	case DW_IDX_compile_unit:
	  {
	    /* Don't crash on bad data.  */
	    if (ull >= per_bfd->all_comp_units.size ())
	      {
		complaint (_(".debug_names entry has bad CU index %s"
			     " [in module %s]"),
			   pulongest (ull),
			   objfile_name (objfile));
		continue;
	      }
	  }
	  per_cu = per_bfd->get_index_cu (ull);
	  break;
	case DW_IDX_type_unit:
	  /* Don't crash on bad data.  */
	  if (ull >= per_bfd->all_type_units.size ())
	    {
	      complaint (_(".debug_names entry has bad TU index %s"
			   " [in module %s]"),
			 pulongest (ull),
			 objfile_name (objfile));
	      continue;
	    }
	  {
	    per_cu = per_bfd->get_index_tu (ull);
	  }
	  break;
	case DW_IDX_die_offset:
	  /* In a per-CU index (as opposed to a per-module index), index
	     entries without CU attribute implicitly refer to the single CU.  */
	  if (per_cu == NULL)
	    per_cu = per_bfd->get_index_cu (0);
	  break;
	case DW_IDX_GNU_internal:
	  if (!m_map.augmentation_is_gdb)
	    break;
	  symbol_linkage_ = symbol_linkage::static_;
	  break;
	case DW_IDX_GNU_external:
	  if (!m_map.augmentation_is_gdb)
	    break;
	  symbol_linkage_ = symbol_linkage::extern_;
	  break;
	}
    }

  /* Skip if we couldn't find a valid CU/TU index.  */
  if (per_cu == nullptr)
    goto again;

  /* Skip if already read in.  */
  if (m_per_objfile->symtab_set_p (per_cu))
    goto again;

  /* Check static vs global.  */
  if (symbol_linkage_ != symbol_linkage::unknown)
    {
      if (symbol_linkage_ == symbol_linkage::static_)
	{
	  if ((m_block_index & SEARCH_STATIC_BLOCK) == 0)
	    goto again;
	}
      else
	{
	  if ((m_block_index & SEARCH_GLOBAL_BLOCK) == 0)
	    goto again;
	}
    }

  /* Match dw2_symtab_iter_next, symbol_kind
     and debug_names::psymbol_tag.  */
  switch (m_domain)
    {
    case VAR_DOMAIN:
      switch (indexval.dwarf_tag)
	{
	case DW_TAG_variable:
	case DW_TAG_subprogram:
	/* Some types are also in VAR_DOMAIN.  */
	case DW_TAG_typedef:
	case DW_TAG_structure_type:
	  break;
	default:
	  goto again;
	}
      break;
    case STRUCT_DOMAIN:
      switch (indexval.dwarf_tag)
	{
	case DW_TAG_typedef:
	case DW_TAG_structure_type:
	  break;
	default:
	  goto again;
	}
      break;
    case LABEL_DOMAIN:
      switch (indexval.dwarf_tag)
	{
	case 0:
	case DW_TAG_variable:
	  break;
	default:
	  goto again;
	}
      break;
    case MODULE_DOMAIN:
      switch (indexval.dwarf_tag)
	{
	case DW_TAG_module:
	  break;
	default:
	  goto again;
	}
      break;
    default:
      break;
    }

  /* Match dw2_expand_symtabs_matching, symbol_kind and
     debug_names::psymbol_tag.  */
  switch (m_search)
    {
    case VARIABLES_DOMAIN:
      switch (indexval.dwarf_tag)
	{
	case DW_TAG_variable:
	  break;
	default:
	  goto again;
	}
      break;
    case FUNCTIONS_DOMAIN:
      switch (indexval.dwarf_tag)
	{
	case DW_TAG_subprogram:
	  break;
	default:
	  goto again;
	}
      break;
    case TYPES_DOMAIN:
      switch (indexval.dwarf_tag)
	{
	case DW_TAG_typedef:
	case DW_TAG_structure_type:
	  break;
	default:
	  goto again;
	}
      break;
    case MODULES_DOMAIN:
      switch (indexval.dwarf_tag)
	{
	case DW_TAG_module:
	  break;
	default:
	  goto again;
	}
    default:
      break;
    }

  return per_cu;
}

/* This dumps minimal information about .debug_names.  It is called
   via "mt print objfiles".  The gdb.dwarf2/gdb-index.exp testcase
   uses this to verify that .debug_names has been loaded.  */

void
dwarf2_debug_names_index::dump (struct objfile *objfile)
{
  gdb_printf (".debug_names: exists\n");
}

bool
dwarf2_debug_names_index::expand_symtabs_matching
  (struct objfile *objfile,
   gdb::function_view<expand_symtabs_file_matcher_ftype> file_matcher,
   const lookup_name_info *lookup_name,
   gdb::function_view<expand_symtabs_symbol_matcher_ftype> symbol_matcher,
   gdb::function_view<expand_symtabs_exp_notify_ftype> expansion_notify,
   block_search_flags search_flags,
   domain_enum domain,
   enum search_domain kind)
{
  dwarf2_per_objfile *per_objfile = get_dwarf2_per_objfile (objfile);

  dw_expand_symtabs_matching_file_matcher (per_objfile, file_matcher);

  /* This invariant is documented in quick-functions.h.  */
  gdb_assert (lookup_name != nullptr || symbol_matcher == nullptr);
  if (lookup_name == nullptr)
    {
      for (dwarf2_per_cu_data *per_cu
	     : all_units_range (per_objfile->per_bfd))
	{
	  QUIT;

	  if (!dw2_expand_symtabs_matching_one (per_cu, per_objfile,
						file_matcher,
						expansion_notify))
	    return false;
	}
      return true;
    }

  mapped_debug_names &map
    = *(gdb::checked_static_cast<mapped_debug_names *>
	(per_objfile->per_bfd->index_table.get ()));

  bool result
    = dw2_expand_symtabs_matching_symbol (map, *lookup_name,
					  symbol_matcher,
					  [&] (offset_type namei)
    {
      /* The name was matched, now expand corresponding CUs that were
	 marked.  */
      dw2_debug_names_iterator iter (map, kind, namei, per_objfile, domain);

      struct dwarf2_per_cu_data *per_cu;
      while ((per_cu = iter.next ()) != NULL)
	if (!dw2_expand_symtabs_matching_one (per_cu, per_objfile,
					      file_matcher,
					      expansion_notify))
	  return false;
      return true;
    }, per_objfile);

  return result;
}
