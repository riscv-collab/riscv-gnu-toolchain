/* Reading code for .gdb_index

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
#include "read-gdb-index.h"

#include "cli/cli-cmds.h"
#include "complaints.h"
#include "dwz.h"
#include "gdb/gdb-index.h"
#include "gdbsupport/gdb-checked-static-cast.h"
#include "mapped-index.h"
#include "read.h"

/* When true, do not reject deprecated .gdb_index sections.  */
static bool use_deprecated_index_sections = false;

/* This is a view into the index that converts from bytes to an
   offset_type, and allows indexing.  Unaligned bytes are specifically
   allowed here, and handled via unpacking.  */

class offset_view
{
public:
  offset_view () = default;

  explicit offset_view (gdb::array_view<const gdb_byte> bytes)
    : m_bytes (bytes)
  {
  }

  /* Extract the INDEXth offset_type from the array.  */
  offset_type operator[] (size_t index) const
  {
    const gdb_byte *bytes = &m_bytes[index * sizeof (offset_type)];
    return (offset_type) extract_unsigned_integer (bytes,
						   sizeof (offset_type),
						   BFD_ENDIAN_LITTLE);
  }

  /* Return the number of offset_types in this array.  */
  size_t size () const
  {
    return m_bytes.size () / sizeof (offset_type);
  }

  /* Return true if this view is empty.  */
  bool empty () const
  {
    return m_bytes.empty ();
  }

private:
  /* The underlying bytes.  */
  gdb::array_view<const gdb_byte> m_bytes;
};

/* A description of .gdb_index index.  The file format is described in
   a comment by the code that writes the index.  */

struct mapped_gdb_index final : public mapped_index_base
{
  /* Index data format version.  */
  int version = 0;

  /* The address table data.  */
  gdb::array_view<const gdb_byte> address_table;

  /* The symbol table, implemented as a hash table.  */
  offset_view symbol_table;

  /* A pointer to the constant pool.  */
  gdb::array_view<const gdb_byte> constant_pool;

  /* The shortcut table data.  */
  gdb::array_view<const gdb_byte> shortcut_table;

  /* Return the index into the constant pool of the name of the IDXth
     symbol in the symbol table.  */
  offset_type symbol_name_index (offset_type idx) const
  {
    return symbol_table[2 * idx];
  }

  /* Return the index into the constant pool of the CU vector of the
     IDXth symbol in the symbol table.  */
  offset_type symbol_vec_index (offset_type idx) const
  {
    return symbol_table[2 * idx + 1];
  }

  bool symbol_name_slot_invalid (offset_type idx) const override
  {
    return (symbol_name_index (idx) == 0
	    && symbol_vec_index (idx) == 0);
  }

  /* Convenience method to get at the name of the symbol at IDX in the
     symbol table.  */
  const char *symbol_name_at
    (offset_type idx, dwarf2_per_objfile *per_objfile) const override
  {
    return (const char *) (this->constant_pool.data ()
			   + symbol_name_index (idx));
  }

  size_t symbol_name_count () const override
  { return this->symbol_table.size () / 2; }

  quick_symbol_functions_up make_quick_functions () const override;

  bool version_check () const override
  {
    return version >= 8;
  }
};

struct dwarf2_gdb_index : public dwarf2_base_index_functions
{
  /* This dumps minimal information about the index.
     It is called via "mt print objfiles".
     One use is to verify .gdb_index has been loaded by the
     gdb.dwarf2/gdb-index.exp testcase.  */
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

/* This dumps minimal information about the index.
   It is called via "mt print objfiles".
   One use is to verify .gdb_index has been loaded by the
   gdb.dwarf2/gdb-index.exp testcase.  */

void
dwarf2_gdb_index::dump (struct objfile *objfile)
{
  dwarf2_per_objfile *per_objfile = get_dwarf2_per_objfile (objfile);

  mapped_gdb_index *index = (gdb::checked_static_cast<mapped_gdb_index *>
			     (per_objfile->per_bfd->index_table.get ()));
  gdb_printf (".gdb_index: version %d\n", index->version);
  gdb_printf ("\n");
}

/* Helper for dw2_expand_matching symtabs.  Called on each symbol
   matched, to expand corresponding CUs that were marked.  IDX is the
   index of the symbol name that matched.  */

static bool
dw2_expand_marked_cus
  (dwarf2_per_objfile *per_objfile, offset_type idx,
   gdb::function_view<expand_symtabs_file_matcher_ftype> file_matcher,
   gdb::function_view<expand_symtabs_exp_notify_ftype> expansion_notify,
   block_search_flags search_flags,
   search_domain kind)
{
  offset_type vec_len, vec_idx;
  bool global_seen = false;
  mapped_gdb_index &index
    = *(gdb::checked_static_cast<mapped_gdb_index *>
	(per_objfile->per_bfd->index_table.get ()));

  offset_view vec (index.constant_pool.slice (index.symbol_vec_index (idx)));
  vec_len = vec[0];
  for (vec_idx = 0; vec_idx < vec_len; ++vec_idx)
    {
      offset_type cu_index_and_attrs = vec[vec_idx + 1];
      /* This value is only valid for index versions >= 7.  */
      int is_static = GDB_INDEX_SYMBOL_STATIC_VALUE (cu_index_and_attrs);
      gdb_index_symbol_kind symbol_kind =
	GDB_INDEX_SYMBOL_KIND_VALUE (cu_index_and_attrs);
      int cu_index = GDB_INDEX_CU_VALUE (cu_index_and_attrs);
      /* Only check the symbol attributes if they're present.
	 Indices prior to version 7 don't record them,
	 and indices >= 7 may elide them for certain symbols
	 (gold does this).  */
      int attrs_valid =
	(index.version >= 7
	 && symbol_kind != GDB_INDEX_SYMBOL_KIND_NONE);

      /* Work around gold/15646.  */
      if (attrs_valid
	  && !is_static
	  && symbol_kind == GDB_INDEX_SYMBOL_KIND_TYPE)
	{
	  if (global_seen)
	    continue;

	  global_seen = true;
	}

      /* Only check the symbol's kind if it has one.  */
      if (attrs_valid)
	{
	  if (is_static)
	    {
	      if ((search_flags & SEARCH_STATIC_BLOCK) == 0)
		continue;
	    }
	  else
	    {
	      if ((search_flags & SEARCH_GLOBAL_BLOCK) == 0)
		continue;
	    }

	  switch (kind)
	    {
	    case VARIABLES_DOMAIN:
	      if (symbol_kind != GDB_INDEX_SYMBOL_KIND_VARIABLE)
		continue;
	      break;
	    case FUNCTIONS_DOMAIN:
	      if (symbol_kind != GDB_INDEX_SYMBOL_KIND_FUNCTION)
		continue;
	      break;
	    case TYPES_DOMAIN:
	      if (symbol_kind != GDB_INDEX_SYMBOL_KIND_TYPE)
		continue;
	      break;
	    case MODULES_DOMAIN:
	      if (symbol_kind != GDB_INDEX_SYMBOL_KIND_OTHER)
		continue;
	      break;
	    default:
	      break;
	    }
	}

      /* Don't crash on bad data.  */
      if (cu_index >= per_objfile->per_bfd->all_units.size ())
	{
	  complaint (_(".gdb_index entry has bad CU index"
		       " [in module %s]"), objfile_name (per_objfile->objfile));
	  continue;
	}

      dwarf2_per_cu_data *per_cu = per_objfile->per_bfd->get_cu (cu_index);
      if (!dw2_expand_symtabs_matching_one (per_cu, per_objfile, file_matcher,
					    expansion_notify))
	return false;
    }

  return true;
}

bool
dwarf2_gdb_index::expand_symtabs_matching
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

  mapped_gdb_index &index
    = *(gdb::checked_static_cast<mapped_gdb_index *>
	(per_objfile->per_bfd->index_table.get ()));

  bool result
    = dw2_expand_symtabs_matching_symbol (index, *lookup_name,
					  symbol_matcher,
					  [&] (offset_type idx)
    {
      if (!dw2_expand_marked_cus (per_objfile, idx, file_matcher,
				  expansion_notify, search_flags, kind))
	return false;
      return true;
    }, per_objfile);

  return result;
}

quick_symbol_functions_up
mapped_gdb_index::make_quick_functions () const
{
  return quick_symbol_functions_up (new dwarf2_gdb_index);
}

/* A helper function that reads the .gdb_index from BUFFER and fills
   in MAP.  FILENAME is the name of the file containing the data;
   it is used for error reporting.  DEPRECATED_OK is true if it is
   ok to use deprecated sections.

   CU_LIST, CU_LIST_ELEMENTS, TYPES_LIST, and TYPES_LIST_ELEMENTS are
   out parameters that are filled in with information about the CU and
   TU lists in the section.

   Returns true if all went well, false otherwise.  */

static bool
read_gdb_index_from_buffer (const char *filename,
			    bool deprecated_ok,
			    gdb::array_view<const gdb_byte> buffer,
			    mapped_gdb_index *map,
			    const gdb_byte **cu_list,
			    offset_type *cu_list_elements,
			    const gdb_byte **types_list,
			    offset_type *types_list_elements)
{
  const gdb_byte *addr = &buffer[0];
  offset_view metadata (buffer);

  /* Version check.  */
  offset_type version = metadata[0];
  /* Versions earlier than 3 emitted every copy of a psymbol.  This
     causes the index to behave very poorly for certain requests.  Version 3
     contained incomplete addrmap.  So, it seems better to just ignore such
     indices.  */
  if (version < 4)
    {
      static int warning_printed = 0;
      if (!warning_printed)
	{
	  warning (_("Skipping obsolete .gdb_index section in %s."),
		   filename);
	  warning_printed = 1;
	}
      return 0;
    }
  /* Index version 4 uses a different hash function than index version
     5 and later.

     Versions earlier than 6 did not emit psymbols for inlined
     functions.  Using these files will cause GDB not to be able to
     set breakpoints on inlined functions by name, so we ignore these
     indices unless the user has done
     "set use-deprecated-index-sections on".  */
  if (version < 6 && !deprecated_ok)
    {
      static int warning_printed = 0;
      if (!warning_printed)
	{
	  warning (_("\
Skipping deprecated .gdb_index section in %s.\n\
Do \"set use-deprecated-index-sections on\" before the file is read\n\
to use the section anyway."),
		   filename);
	  warning_printed = 1;
	}
      return 0;
    }
  /* Version 7 indices generated by gold refer to the CU for a symbol instead
     of the TU (for symbols coming from TUs),
     http://sourceware.org/bugzilla/show_bug.cgi?id=15021.
     Plus gold-generated indices can have duplicate entries for global symbols,
     http://sourceware.org/bugzilla/show_bug.cgi?id=15646.
     These are just performance bugs, and we can't distinguish gdb-generated
     indices from gold-generated ones, so issue no warning here.  */

  /* Indexes with higher version than the one supported by GDB may be no
     longer backward compatible.  */
  if (version > 9)
    return 0;

  map->version = version;

  int i = 1;
  *cu_list = addr + metadata[i];
  *cu_list_elements = (metadata[i + 1] - metadata[i]) / 8;
  ++i;

  *types_list = addr + metadata[i];
  *types_list_elements = (metadata[i + 1] - metadata[i]) / 8;
  ++i;

  const gdb_byte *address_table = addr + metadata[i];
  const gdb_byte *address_table_end = addr + metadata[i + 1];
  map->address_table
    = gdb::array_view<const gdb_byte> (address_table, address_table_end);
  ++i;

  const gdb_byte *symbol_table = addr + metadata[i];
  const gdb_byte *symbol_table_end = addr + metadata[i + 1];
  map->symbol_table
    = offset_view (gdb::array_view<const gdb_byte> (symbol_table,
						    symbol_table_end));

  ++i;

  if (version >= 9)
    {
      const gdb_byte *shortcut_table = addr + metadata[i];
      const gdb_byte *shortcut_table_end = addr + metadata[i + 1];
      map->shortcut_table
	= gdb::array_view<const gdb_byte> (shortcut_table, shortcut_table_end);
      ++i;
    }

  map->constant_pool = buffer.slice (metadata[i]);

  if (map->constant_pool.empty () && !map->symbol_table.empty ())
    {
      /* An empty constant pool implies that all symbol table entries are
	 empty.  Make map->symbol_table.empty () == true.  */
      map->symbol_table
	= offset_view (gdb::array_view<const gdb_byte> (symbol_table,
							symbol_table));
    }

  return 1;
}

/* A helper for create_cus_from_gdb_index that handles a given list of
   CUs.  */

static void
create_cus_from_gdb_index_list (dwarf2_per_bfd *per_bfd,
				const gdb_byte *cu_list, offset_type n_elements,
				struct dwarf2_section_info *section,
				int is_dwz)
{
  for (offset_type i = 0; i < n_elements; i += 2)
    {
      static_assert (sizeof (ULONGEST) >= 8);

      sect_offset sect_off
	= (sect_offset) extract_unsigned_integer (cu_list, 8, BFD_ENDIAN_LITTLE);
      ULONGEST length = extract_unsigned_integer (cu_list + 8, 8, BFD_ENDIAN_LITTLE);
      cu_list += 2 * 8;

      dwarf2_per_cu_data_up per_cu
	= create_cu_from_index_list (per_bfd, section, is_dwz, sect_off,
				     length);
      per_bfd->all_units.push_back (std::move (per_cu));
    }
}

/* Read the CU list from the mapped index, and use it to create all
   the CU objects for PER_BFD.  */

static void
create_cus_from_gdb_index (dwarf2_per_bfd *per_bfd,
			   const gdb_byte *cu_list, offset_type cu_list_elements,
			   const gdb_byte *dwz_list, offset_type dwz_elements)
{
  gdb_assert (per_bfd->all_units.empty ());
  per_bfd->all_units.reserve ((cu_list_elements + dwz_elements) / 2);

  create_cus_from_gdb_index_list (per_bfd, cu_list, cu_list_elements,
				  &per_bfd->info, 0);

  if (dwz_elements == 0)
    return;

  dwz_file *dwz = dwarf2_get_dwz_file (per_bfd);
  create_cus_from_gdb_index_list (per_bfd, dwz_list, dwz_elements,
				  &dwz->info, 1);
}

/* Create the signatured type hash table from the index.  */

static void
create_signatured_type_table_from_gdb_index
  (dwarf2_per_bfd *per_bfd, struct dwarf2_section_info *section,
   const gdb_byte *bytes, offset_type elements)
{
  htab_up sig_types_hash = allocate_signatured_type_table ();

  for (offset_type i = 0; i < elements; i += 3)
    {
      signatured_type_up sig_type;
      ULONGEST signature;
      void **slot;
      cu_offset type_offset_in_tu;

      static_assert (sizeof (ULONGEST) >= 8);
      sect_offset sect_off
	= (sect_offset) extract_unsigned_integer (bytes, 8, BFD_ENDIAN_LITTLE);
      type_offset_in_tu
	= (cu_offset) extract_unsigned_integer (bytes + 8, 8,
						BFD_ENDIAN_LITTLE);
      signature = extract_unsigned_integer (bytes + 16, 8, BFD_ENDIAN_LITTLE);
      bytes += 3 * 8;

      sig_type = per_bfd->allocate_signatured_type (signature);
      sig_type->type_offset_in_tu = type_offset_in_tu;
      sig_type->section = section;
      sig_type->sect_off = sect_off;

      slot = htab_find_slot (sig_types_hash.get (), sig_type.get (), INSERT);
      *slot = sig_type.get ();

      per_bfd->all_units.emplace_back (sig_type.release ());
    }

  per_bfd->signatured_types = std::move (sig_types_hash);
}

/* Read the address map data from the mapped GDB index, and use it to
   populate the index_addrmap.  */

static void
create_addrmap_from_gdb_index (dwarf2_per_objfile *per_objfile,
			       mapped_gdb_index *index)
{
  dwarf2_per_bfd *per_bfd = per_objfile->per_bfd;
  const gdb_byte *iter, *end;

  addrmap_mutable mutable_map;

  iter = index->address_table.data ();
  end = iter + index->address_table.size ();

  while (iter < end)
    {
      ULONGEST hi, lo, cu_index;
      lo = extract_unsigned_integer (iter, 8, BFD_ENDIAN_LITTLE);
      iter += 8;
      hi = extract_unsigned_integer (iter, 8, BFD_ENDIAN_LITTLE);
      iter += 8;
      cu_index = extract_unsigned_integer (iter, 4, BFD_ENDIAN_LITTLE);
      iter += 4;

      if (lo > hi)
	{
	  complaint (_(".gdb_index address table has invalid range (%s - %s)"),
		     hex_string (lo), hex_string (hi));
	  continue;
	}

      if (cu_index >= per_bfd->all_units.size ())
	{
	  complaint (_(".gdb_index address table has invalid CU number %u"),
		     (unsigned) cu_index);
	  continue;
	}

      lo = (ULONGEST) per_objfile->adjust ((unrelocated_addr) lo);
      hi = (ULONGEST) per_objfile->adjust ((unrelocated_addr) hi);
      mutable_map.set_empty (lo, hi - 1, per_bfd->get_cu (cu_index));
    }

  per_bfd->index_addrmap
    = new (&per_bfd->obstack) addrmap_fixed (&per_bfd->obstack, &mutable_map);
}

/* Sets the name and language of the main function from the shortcut table.  */

static void
set_main_name_from_gdb_index (dwarf2_per_objfile *per_objfile,
			      mapped_gdb_index *index)
{
  const auto expected_size = 2 * sizeof (offset_type);
  if (index->shortcut_table.size () < expected_size)
    /* The data in the section is not present, is corrupted or is in a version
       we don't know about.  Regardless, we can't make use of it.  */
    return;

  auto ptr = index->shortcut_table.data ();
  const auto dw_lang = extract_unsigned_integer (ptr, 4, BFD_ENDIAN_LITTLE);
  if (dw_lang >= DW_LANG_hi_user)
    {
      complaint (_(".gdb_index shortcut table has invalid main language %u"),
		   (unsigned) dw_lang);
      return;
    }
  if (dw_lang == 0)
    {
      /* Don't bother if the language for the main symbol was not known or if
	 there was no main symbol at all when the index was built.  */
      return;
    }
  ptr += 4;

  const auto lang = dwarf_lang_to_enum_language (dw_lang);
  const auto name_offset = extract_unsigned_integer (ptr,
						     sizeof (offset_type),
						     BFD_ENDIAN_LITTLE);
  const auto name = (const char *) (index->constant_pool.data () + name_offset);

  set_objfile_main_name (per_objfile->objfile, name, (enum language) lang);
}

/* See read-gdb-index.h.  */

int
dwarf2_read_gdb_index
  (dwarf2_per_objfile *per_objfile,
   get_gdb_index_contents_ftype get_gdb_index_contents,
   get_gdb_index_contents_dwz_ftype get_gdb_index_contents_dwz)
{
  const gdb_byte *cu_list, *types_list, *dwz_list = NULL;
  offset_type cu_list_elements, types_list_elements, dwz_list_elements = 0;
  struct dwz_file *dwz;
  struct objfile *objfile = per_objfile->objfile;
  dwarf2_per_bfd *per_bfd = per_objfile->per_bfd;

  gdb::array_view<const gdb_byte> main_index_contents
    = get_gdb_index_contents (objfile, per_bfd);

  if (main_index_contents.empty ())
    return 0;

  auto map = std::make_unique<mapped_gdb_index> ();
  if (!read_gdb_index_from_buffer (objfile_name (objfile),
				   use_deprecated_index_sections,
				   main_index_contents, map.get (), &cu_list,
				   &cu_list_elements, &types_list,
				   &types_list_elements))
    return 0;

  /* Don't use the index if it's empty.  */
  if (map->symbol_table.empty ())
    return 0;

  /* If there is a .dwz file, read it so we can get its CU list as
     well.  */
  dwz = dwarf2_get_dwz_file (per_bfd);
  if (dwz != NULL)
    {
      mapped_gdb_index dwz_map;
      const gdb_byte *dwz_types_ignore;
      offset_type dwz_types_elements_ignore;

      gdb::array_view<const gdb_byte> dwz_index_content
	= get_gdb_index_contents_dwz (objfile, dwz);

      if (dwz_index_content.empty ())
	return 0;

      if (!read_gdb_index_from_buffer (bfd_get_filename (dwz->dwz_bfd.get ()),
				       1, dwz_index_content, &dwz_map,
				       &dwz_list, &dwz_list_elements,
				       &dwz_types_ignore,
				       &dwz_types_elements_ignore))
	{
	  warning (_("could not read '.gdb_index' section from %s; skipping"),
		   bfd_get_filename (dwz->dwz_bfd.get ()));
	  return 0;
	}
    }

  create_cus_from_gdb_index (per_bfd, cu_list, cu_list_elements, dwz_list,
			     dwz_list_elements);

  if (types_list_elements)
    {
      /* We can only handle a single .debug_types when we have an
	 index.  */
      if (per_bfd->types.size () > 1)
	{
	  per_bfd->all_units.clear ();
	  return 0;
	}

      dwarf2_section_info *section
	= (per_bfd->types.size () == 1
	   ? &per_bfd->types[0]
	   : &per_bfd->info);

      create_signatured_type_table_from_gdb_index (per_bfd, section, types_list,
						   types_list_elements);
    }

  finalize_all_units (per_bfd);

  create_addrmap_from_gdb_index (per_objfile, map.get ());

  set_main_name_from_gdb_index (per_objfile, map.get ());

  per_bfd->index_table = std::move (map);
  per_bfd->quick_file_names_table =
    create_quick_file_names_table (per_bfd->all_units.size ());

  return 1;
}

void _initialize_read_gdb_index ();

void
_initialize_read_gdb_index ()
{
  add_setshow_boolean_cmd ("use-deprecated-index-sections",
			   no_class, &use_deprecated_index_sections, _("\
Set whether to use deprecated gdb_index sections."), _("\
Show whether to use deprecated gdb_index sections."), _("\
When enabled, deprecated .gdb_index sections are used anyway.\n\
Normally they are ignored either because of a missing feature or\n\
performance issue.\n\
Warning: This option must be enabled before gdb reads the file."),
			   NULL,
			   NULL,
			   &setlist, &showlist);
}
