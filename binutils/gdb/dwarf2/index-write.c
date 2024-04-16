/* DWARF index writing support for GDB.

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

#include "defs.h"

#include "dwarf2/index-write.h"

#include "addrmap.h"
#include "cli/cli-decode.h"
#include "gdbsupport/byte-vector.h"
#include "gdbsupport/filestuff.h"
#include "gdbsupport/gdb_unlinker.h"
#include "gdbsupport/pathstuff.h"
#include "gdbsupport/scoped_fd.h"
#include "complaints.h"
#include "dwarf2/index-common.h"
#include "dwarf2/cooked-index.h"
#include "dwarf2.h"
#include "dwarf2/read.h"
#include "dwarf2/dwz.h"
#include "gdb/gdb-index.h"
#include "gdbcmd.h"
#include "objfiles.h"
#include "ada-lang.h"
#include "dwarf2/tag.h"
#include "gdbsupport/gdb_tilde_expand.h"

#include <algorithm>
#include <cmath>
#include <forward_list>
#include <set>
#include <unordered_map>
#include <unordered_set>

/* Ensure only legit values are used.  */
#define DW2_GDB_INDEX_SYMBOL_STATIC_SET_VALUE(cu_index, value) \
  do { \
    gdb_assert ((unsigned int) (value) <= 1); \
    GDB_INDEX_SYMBOL_STATIC_SET_VALUE((cu_index), (value)); \
  } while (0)

/* Ensure only legit values are used.  */
#define DW2_GDB_INDEX_SYMBOL_KIND_SET_VALUE(cu_index, value) \
  do { \
    gdb_assert ((value) >= GDB_INDEX_SYMBOL_KIND_TYPE \
		&& (value) <= GDB_INDEX_SYMBOL_KIND_OTHER); \
    GDB_INDEX_SYMBOL_KIND_SET_VALUE((cu_index), (value)); \
  } while (0)

/* Ensure we don't use more than the allotted number of bits for the CU.  */
#define DW2_GDB_INDEX_CU_SET_VALUE(cu_index, value) \
  do { \
    gdb_assert (((value) & ~GDB_INDEX_CU_MASK) == 0); \
    GDB_INDEX_CU_SET_VALUE((cu_index), (value)); \
  } while (0)

/* The "save gdb-index" command.  */

/* Write SIZE bytes from the buffer pointed to by DATA to FILE, with
   error checking.  */

static void
file_write (FILE *file, const void *data, size_t size)
{
  if (fwrite (data, 1, size, file) != size)
    error (_("couldn't data write to file"));
}

/* Write the contents of VEC to FILE, with error checking.  */

template<typename Elem, typename Alloc>
static void
file_write (FILE *file, const std::vector<Elem, Alloc> &vec)
{
  if (!vec.empty ())
    file_write (file, vec.data (), vec.size () * sizeof (vec[0]));
}

/* In-memory buffer to prepare data to be written later to a file.  */
class data_buf
{
public:
  /* Copy ARRAY to the end of the buffer.  */
  void append_array (gdb::array_view<const gdb_byte> array)
  {
    std::copy (array.begin (), array.end (), grow (array.size ()));
  }

  /* Copy CSTR (a zero-terminated string) to the end of buffer.  The
     terminating zero is appended too.  */
  void append_cstr0 (const char *cstr)
  {
    const size_t size = strlen (cstr) + 1;
    std::copy (cstr, cstr + size, grow (size));
  }

  /* Store INPUT as ULEB128 to the end of buffer.  */
  void append_unsigned_leb128 (ULONGEST input)
  {
    for (;;)
      {
	gdb_byte output = input & 0x7f;
	input >>= 7;
	if (input)
	  output |= 0x80;
	m_vec.push_back (output);
	if (input == 0)
	  break;
      }
  }

  /* Accept a host-format integer in VAL and append it to the buffer
     as a target-format integer which is LEN bytes long.  */
  void append_uint (size_t len, bfd_endian byte_order, ULONGEST val)
  {
    ::store_unsigned_integer (grow (len), len, byte_order, val);
  }

  /* Copy VALUE to the end of the buffer, little-endian.  */
  void append_offset (offset_type value)
  {
    append_uint (sizeof (value), BFD_ENDIAN_LITTLE, value);
  }

  /* Return the size of the buffer.  */
  virtual size_t size () const
  {
    return m_vec.size ();
  }

  /* Return true iff the buffer is empty.  */
  bool empty () const
  {
    return m_vec.empty ();
  }

  /* Write the buffer to FILE.  */
  void file_write (FILE *file) const
  {
    ::file_write (file, m_vec);
  }

private:
  /* Grow SIZE bytes at the end of the buffer.  Returns a pointer to
     the start of the new block.  */
  gdb_byte *grow (size_t size)
  {
    m_vec.resize (m_vec.size () + size);
    return &*(m_vec.end () - size);
  }

  gdb::byte_vector m_vec;
};

/* An entry in the symbol table.  */
struct symtab_index_entry
{
  /* The name of the symbol.  */
  const char *name;
  /* The offset of the name in the constant pool.  */
  offset_type index_offset;
  /* A sorted vector of the indices of all the CUs that hold an object
     of this name.  */
  std::vector<offset_type> cu_indices;

  /* Minimize CU_INDICES, sorting them and removing duplicates as
     appropriate.  */
  void minimize ();
};

/* The symbol table.  This is a power-of-2-sized hash table.  */
struct mapped_symtab
{
  mapped_symtab ()
  {
    m_data.resize (1024);
  }

  /* If there are no elements in the symbol table, then reduce the table
     size to zero.  Otherwise call symtab_index_entry::minimize each entry
     in the symbol table.  */

  void minimize ()
  {
    if (m_element_count == 0)
      m_data.resize (0);

    for (symtab_index_entry &item : m_data)
      item.minimize ();
  }

  /* Add an entry to SYMTAB.  NAME is the name of the symbol.  CU_INDEX is
     the index of the CU in which the symbol appears.  IS_STATIC is one if
     the symbol is static, otherwise zero (global).  */

  void add_index_entry (const char *name, int is_static,
			gdb_index_symbol_kind kind, offset_type cu_index);

  /* When entries are originally added into the data hash the order will
     vary based on the number of worker threads GDB is configured to use.
     This function will rebuild the hash such that the final layout will be
     deterministic regardless of the number of worker threads used.  */

  void sort ();

  /* Access the obstack.  */
  struct obstack *obstack ()
  { return &m_string_obstack; }

private:

  /* Find a slot in SYMTAB for the symbol NAME.  Returns a reference to
     the slot.

     Function is used only during write_hash_table so no index format
     backward compatibility is needed.  */

  symtab_index_entry &find_slot (const char *name);

  /* Expand SYMTAB's hash table.  */

  void hash_expand ();

  /* Return true if the hash table in data needs to grow.  */

  bool hash_needs_expanding () const
  { return 4 * m_element_count / 3 >= m_data.size (); }

  /* A vector that is used as a hash table.  */
  std::vector<symtab_index_entry> m_data;

  /* The number of elements stored in the m_data hash.  */
  offset_type m_element_count = 0;

  /* Temporary storage for names.  */
  auto_obstack m_string_obstack;

public:
  using iterator = decltype (m_data)::iterator;
  using const_iterator = decltype (m_data)::const_iterator;

  iterator begin ()
  { return m_data.begin (); }

  iterator end ()
  { return m_data.end (); }

  const_iterator cbegin ()
  { return m_data.cbegin (); }

  const_iterator cend ()
  { return m_data.cend (); }
};

/* See class definition.  */

symtab_index_entry &
mapped_symtab::find_slot (const char *name)
{
  offset_type index, step, hash = mapped_index_string_hash (INT_MAX, name);

  index = hash & (m_data.size () - 1);
  step = ((hash * 17) & (m_data.size () - 1)) | 1;

  for (;;)
    {
      if (m_data[index].name == NULL
	  || strcmp (name, m_data[index].name) == 0)
	return m_data[index];
      index = (index + step) & (m_data.size () - 1);
    }
}

/* See class definition.  */

void
mapped_symtab::hash_expand ()
{
  auto old_entries = std::move (m_data);

  gdb_assert (m_data.size () == 0);
  m_data.resize (old_entries.size () * 2);

  for (auto &it : old_entries)
    if (it.name != NULL)
      {
	auto &ref = this->find_slot (it.name);
	ref = std::move (it);
      }
}

/* See mapped_symtab class declaration.  */

void mapped_symtab::sort ()
{
  /* Move contents out of this->data vector.  */
  std::vector<symtab_index_entry> original_data = std::move (m_data);

  /* Restore the size of m_data, this will avoid having to expand the hash
     table (and rehash all elements) when we reinsert after sorting.
     However, we do reset the element count, this allows for some sanity
     checking asserts during the reinsert phase.  */
  gdb_assert (m_data.size () == 0);
  m_data.resize (original_data.size ());
  m_element_count = 0;

  /* Remove empty entries from ORIGINAL_DATA, this makes sorting quicker.  */
  auto it = std::remove_if (original_data.begin (), original_data.end (),
			    [] (const symtab_index_entry &entry) -> bool
			    {
			      return entry.name == nullptr;
			    });
  original_data.erase (it, original_data.end ());

  /* Sort the existing contents.  */
  std::sort (original_data.begin (), original_data.end (),
	     [] (const symtab_index_entry &a,
		 const symtab_index_entry &b) -> bool
	     {
	       /* Return true if A is before B.  */
	       gdb_assert (a.name != nullptr);
	       gdb_assert (b.name != nullptr);

	       return strcmp (a.name, b.name) < 0;
	     });

  /* Re-insert each item from the sorted list.  */
  for (auto &entry : original_data)
    {
      /* We know that ORIGINAL_DATA contains no duplicates, this data was
	 taken from a hash table that de-duplicated entries for us, so
	 count this as a new item.

	 As we retained the original size of m_data (see above) then we
	 should never need to grow m_data_ during this re-insertion phase,
	 assert that now.  */
      ++m_element_count;
      gdb_assert (!this->hash_needs_expanding ());

      /* Lookup a slot.  */
      symtab_index_entry &slot = this->find_slot (entry.name);

      /* As discussed above, we should not find duplicates.  */
      gdb_assert (slot.name == nullptr);

      /* Move this item into the slot we found.  */
      slot = std::move (entry);
    }
}

/* See class definition.  */

void
mapped_symtab::add_index_entry (const char *name, int is_static,
				gdb_index_symbol_kind kind,
				offset_type cu_index)
{
  symtab_index_entry *slot = &this->find_slot (name);
  if (slot->name == NULL)
    {
      /* This is a new element in the hash table.  */
      ++this->m_element_count;

      /* We might need to grow the hash table.  */
      if (this->hash_needs_expanding ())
	{
	  this->hash_expand ();

	  /* This element will have a different slot in the new table.  */
	  slot = &this->find_slot (name);

	  /* But it should still be a new element in the hash table.  */
	  gdb_assert (slot->name == nullptr);
	}

      slot->name = name;
      /* index_offset is set later.  */
    }

  offset_type cu_index_and_attrs = 0;
  DW2_GDB_INDEX_CU_SET_VALUE (cu_index_and_attrs, cu_index);
  DW2_GDB_INDEX_SYMBOL_STATIC_SET_VALUE (cu_index_and_attrs, is_static);
  DW2_GDB_INDEX_SYMBOL_KIND_SET_VALUE (cu_index_and_attrs, kind);

  /* We don't want to record an index value twice as we want to avoid the
     duplication.
     We process all global symbols and then all static symbols
     (which would allow us to avoid the duplication by only having to check
     the last entry pushed), but a symbol could have multiple kinds in one CU.
     To keep things simple we don't worry about the duplication here and
     sort and uniquify the list after we've processed all symbols.  */
  slot->cu_indices.push_back (cu_index_and_attrs);
}

/* See symtab_index_entry.  */

void
symtab_index_entry::minimize ()
{
  if (name == nullptr || cu_indices.empty ())
    return;

  std::sort (cu_indices.begin (), cu_indices.end ());
  auto from = std::unique (cu_indices.begin (), cu_indices.end ());
  cu_indices.erase (from, cu_indices.end ());

  /* We don't want to enter a type more than once, so
     remove any such duplicates from the list as well.  When doing
     this, we want to keep the entry from the first CU -- but this is
     implicit due to the sort.  This choice is done because it's
     similar to what gdb historically did for partial symbols.  */
  std::unordered_set<offset_type> seen;
  from = std::remove_if (cu_indices.begin (), cu_indices.end (),
			 [&] (offset_type val)
    {
      gdb_index_symbol_kind kind = GDB_INDEX_SYMBOL_KIND_VALUE (val);
      if (kind != GDB_INDEX_SYMBOL_KIND_TYPE)
	return false;

      val &= ~GDB_INDEX_CU_MASK;
      return !seen.insert (val).second;
    });
  cu_indices.erase (from, cu_indices.end ());
}

/* A form of 'const char *' suitable for container keys.  Only the
   pointer is stored.  The strings themselves are compared, not the
   pointers.  */
class c_str_view
{
public:
  c_str_view (const char *cstr)
    : m_cstr (cstr)
  {}

  bool operator== (const c_str_view &other) const
  {
    return strcmp (m_cstr, other.m_cstr) == 0;
  }

  bool operator< (const c_str_view &other) const
  {
    return strcmp (m_cstr, other.m_cstr) < 0;
  }

  /* Return the underlying C string.  Note, the returned string is
     only a reference with lifetime of this object.  */
  const char *c_str () const
  {
    return m_cstr;
  }

private:
  friend class c_str_view_hasher;
  const char *const m_cstr;
};

/* A std::unordered_map::hasher for c_str_view that uses the right
   hash function for strings in a mapped index.  */
class c_str_view_hasher
{
public:
  size_t operator () (const c_str_view &x) const
  {
    return mapped_index_string_hash (INT_MAX, x.m_cstr);
  }
};

/* A std::unordered_map::hasher for std::vector<>.  */
template<typename T>
class vector_hasher
{
public:
  size_t operator () (const std::vector<T> &key) const
  {
    return iterative_hash (key.data (),
			   sizeof (key.front ()) * key.size (), 0);
  }
};

/* Write the mapped hash table SYMTAB to the data buffer OUTPUT, with
   constant pool entries going into the data buffer CPOOL.  */

static void
write_hash_table (mapped_symtab *symtab, data_buf &output, data_buf &cpool)
{
  {
    /* Elements are sorted vectors of the indices of all the CUs that
       hold an object of this name.  */
    std::unordered_map<std::vector<offset_type>, offset_type,
		       vector_hasher<offset_type>>
      symbol_hash_table;

    /* We add all the index vectors to the constant pool first, to
       ensure alignment is ok.  */
    for (symtab_index_entry &entry : *symtab)
      {
	if (entry.name == NULL)
	  continue;
	gdb_assert (entry.index_offset == 0);

	auto [iter, inserted]
	  = symbol_hash_table.try_emplace (entry.cu_indices,
					   cpool.size ());
	entry.index_offset = iter->second;
	if (inserted)
	  {
	    /* Newly inserted.  */
	    cpool.append_offset (entry.cu_indices.size ());
	    for (const auto index : entry.cu_indices)
	      cpool.append_offset (index);
	  }
      }
  }

  /* Now write out the hash table.  */
  std::unordered_map<c_str_view, offset_type, c_str_view_hasher> str_table;
  for (const auto &entry : *symtab)
    {
      offset_type str_off, vec_off;

      if (entry.name != NULL)
	{
	  const auto insertpair = str_table.emplace (entry.name, cpool.size ());
	  if (insertpair.second)
	    cpool.append_cstr0 (entry.name);
	  str_off = insertpair.first->second;
	  vec_off = entry.index_offset;
	}
      else
	{
	  /* While 0 is a valid constant pool index, it is not valid
	     to have 0 for both offsets.  */
	  str_off = 0;
	  vec_off = 0;
	}

      output.append_offset (str_off);
      output.append_offset (vec_off);
    }
}

using cu_index_map
  = std::unordered_map<const dwarf2_per_cu_data *, unsigned int>;

/* Helper struct for building the address table.  */
struct addrmap_index_data
{
  addrmap_index_data (data_buf &addr_vec_, cu_index_map &cu_index_htab_)
    : addr_vec (addr_vec_),
      cu_index_htab (cu_index_htab_)
  {}

  data_buf &addr_vec;
  cu_index_map &cu_index_htab;

  int operator() (CORE_ADDR start_addr, const void *obj);

  /* True if the previous_* fields are valid.
     We can't write an entry until we see the next entry (since it is only then
     that we know the end of the entry).  */
  bool previous_valid = false;
  /* Index of the CU in the table of all CUs in the index file.  */
  unsigned int previous_cu_index = 0;
  /* Start address of the CU.  */
  CORE_ADDR previous_cu_start = 0;
};

/* Write an address entry to ADDR_VEC.  */

static void
add_address_entry (data_buf &addr_vec,
		   CORE_ADDR start, CORE_ADDR end, unsigned int cu_index)
{
  addr_vec.append_uint (8, BFD_ENDIAN_LITTLE, start);
  addr_vec.append_uint (8, BFD_ENDIAN_LITTLE, end);
  addr_vec.append_offset (cu_index);
}

/* Worker function for traversing an addrmap to build the address table.  */

int
addrmap_index_data::operator() (CORE_ADDR start_addr, const void *obj)
{
  const dwarf2_per_cu_data *per_cu
    = static_cast<const dwarf2_per_cu_data *> (obj);

  if (previous_valid)
    add_address_entry (addr_vec,
		       previous_cu_start, start_addr,
		       previous_cu_index);

  previous_cu_start = start_addr;
  if (per_cu != NULL)
    {
      const auto it = cu_index_htab.find (per_cu);
      gdb_assert (it != cu_index_htab.cend ());
      previous_cu_index = it->second;
      previous_valid = true;
    }
  else
    previous_valid = false;

  return 0;
}

/* Write PER_BFD's address map to ADDR_VEC.
   CU_INDEX_HTAB is used to map addrmap entries to their CU indices
   in the index file.  */

static void
write_address_map (const addrmap *addrmap, data_buf &addr_vec,
		   cu_index_map &cu_index_htab)
{
  struct addrmap_index_data addrmap_index_data (addr_vec, cu_index_htab);

  addrmap->foreach (addrmap_index_data);

  /* It's highly unlikely the last entry (end address = 0xff...ff)
     is valid, but we should still handle it.
     The end address is recorded as the start of the next region, but that
     doesn't work here.  To cope we pass 0xff...ff, this is a rare situation
     anyway.  */
  if (addrmap_index_data.previous_valid)
    add_address_entry (addr_vec,
		       addrmap_index_data.previous_cu_start, (CORE_ADDR) -1,
		       addrmap_index_data.previous_cu_index);
}

/* DWARF-5 .debug_names builder.  */
class debug_names
{
public:
  debug_names (dwarf2_per_bfd *per_bfd, bool is_dwarf64,
	       bfd_endian dwarf5_byte_order)
    : m_dwarf5_byte_order (dwarf5_byte_order),
      m_dwarf32 (dwarf5_byte_order),
      m_dwarf64 (dwarf5_byte_order),
      m_dwarf (is_dwarf64
	       ? static_cast<dwarf &> (m_dwarf64)
	       : static_cast<dwarf &> (m_dwarf32)),
      m_name_table_string_offs (m_dwarf.name_table_string_offs),
      m_name_table_entry_offs (m_dwarf.name_table_entry_offs),
      m_debugstrlookup (per_bfd)
  {}

  int dwarf5_offset_size () const
  {
    const bool dwarf5_is_dwarf64 = &m_dwarf == &m_dwarf64;
    return dwarf5_is_dwarf64 ? 8 : 4;
  }

  /* Is this symbol from DW_TAG_compile_unit or DW_TAG_type_unit?  */
  enum class unit_kind { cu, tu };

  /* Insert one symbol.  */
  void insert (const cooked_index_entry *entry)
  {
    const auto it = m_cu_index_htab.find (entry->per_cu);
    gdb_assert (it != m_cu_index_htab.cend ());
    const char *name = entry->full_name (&m_string_obstack);

    /* This is incorrect but it mirrors gdb's historical behavior; and
       because the current .debug_names generation is also incorrect,
       it seems better to follow what was done before, rather than
       introduce a mismatch between the newer and older gdb.  */
    dwarf_tag tag = entry->tag;
    if (tag != DW_TAG_typedef && tag_is_type (tag))
      tag = DW_TAG_structure_type;
    else if (tag == DW_TAG_enumerator || tag == DW_TAG_constant)
      tag = DW_TAG_variable;

    int cu_index = it->second;
    bool is_static = (entry->flags & IS_STATIC) != 0;
    unit_kind kind = (entry->per_cu->is_debug_types
		      ? unit_kind::tu
		      : unit_kind::cu);

    if (entry->per_cu->lang () == language_ada)
      {
	/* We want to ensure that the Ada main function's name appears
	   verbatim in the index.  However, this name will be of the
	   form "_ada_mumble", and will be rewritten by ada_decode.
	   So, recognize it specially here and add it to the index by
	   hand.  */
	if (strcmp (main_name (), name) == 0)
	  {
	    const auto insertpair
	      = m_name_to_value_set.emplace (c_str_view (name),
					     std::set<symbol_value> ());
	    std::set<symbol_value> &value_set = insertpair.first->second;
	    value_set.emplace (symbol_value (tag, cu_index, is_static, kind));
	  }

	/* In order for the index to work when read back into gdb, it
	   has to supply a funny form of the name: it should be the
	   encoded name, with any suffixes stripped.  Using the
	   ordinary encoded name will not work properly with the
	   searching logic in find_name_components_bounds; nor will
	   using the decoded name.  Furthermore, an Ada "verbatim"
	   name (of the form "<MumBle>") must be entered without the
	   angle brackets.  Note that the current index is unusual,
	   see PR symtab/24820 for details.  */
	std::string decoded = ada_decode (name);
	if (decoded[0] == '<')
	  name = (char *) obstack_copy0 (&m_string_obstack,
					 decoded.c_str () + 1,
					 decoded.length () - 2);
	else
	  name = obstack_strdup (&m_string_obstack,
				 ada_encode (decoded.c_str ()));
      }

    const auto insertpair
      = m_name_to_value_set.emplace (c_str_view (name),
				     std::set<symbol_value> ());
    std::set<symbol_value> &value_set = insertpair.first->second;
    value_set.emplace (symbol_value (tag, cu_index, is_static, kind));
  }

  /* Build all the tables.  All symbols must be already inserted.
     This function does not call file_write, caller has to do it
     afterwards.  */
  void build ()
  {
    /* Verify the build method has not be called twice.  */
    gdb_assert (m_abbrev_table.empty ());
    const size_t name_count = m_name_to_value_set.size ();
    m_bucket_table.resize
      (std::pow (2, std::ceil (std::log2 (name_count * 4 / 3))));
    m_hash_table.reserve (name_count);
    m_name_table_string_offs.reserve (name_count);
    m_name_table_entry_offs.reserve (name_count);

    /* Map each hash of symbol to its name and value.  */
    struct hash_it_pair
    {
      uint32_t hash;
      decltype (m_name_to_value_set)::const_iterator it;
    };
    std::vector<std::forward_list<hash_it_pair>> bucket_hash;
    bucket_hash.resize (m_bucket_table.size ());
    for (decltype (m_name_to_value_set)::const_iterator it
	   = m_name_to_value_set.cbegin ();
	 it != m_name_to_value_set.cend ();
	 ++it)
      {
	const char *const name = it->first.c_str ();
	const uint32_t hash = dwarf5_djb_hash (name);
	hash_it_pair hashitpair;
	hashitpair.hash = hash;
	hashitpair.it = it;
	auto &slot = bucket_hash[hash % bucket_hash.size()];
	slot.push_front (std::move (hashitpair));
      }
    for (size_t bucket_ix = 0; bucket_ix < bucket_hash.size (); ++bucket_ix)
      {
	std::forward_list<hash_it_pair> &hashitlist = bucket_hash[bucket_ix];
	if (hashitlist.empty ())
	  continue;

	/* Sort the items within each bucket.  This ensures that the
	   generated index files will be the same no matter the order in
	   which symbols were added into the index.  */
	hashitlist.sort ([] (const hash_it_pair &a, const hash_it_pair &b)
	{
	  return a.it->first < b.it->first;
	});

	uint32_t &bucket_slot = m_bucket_table[bucket_ix];
	/* The hashes array is indexed starting at 1.  */
	store_unsigned_integer (reinterpret_cast<gdb_byte *> (&bucket_slot),
				sizeof (bucket_slot), m_dwarf5_byte_order,
				m_hash_table.size () + 1);
	for (const hash_it_pair &hashitpair : hashitlist)
	  {
	    m_hash_table.push_back (0);
	    store_unsigned_integer (reinterpret_cast<gdb_byte *>
							(&m_hash_table.back ()),
				    sizeof (m_hash_table.back ()),
				    m_dwarf5_byte_order, hashitpair.hash);
	    const c_str_view &name = hashitpair.it->first;
	    const std::set<symbol_value> &value_set = hashitpair.it->second;
	    m_name_table_string_offs.push_back_reorder
	      (m_debugstrlookup.lookup (name.c_str ()));
	    m_name_table_entry_offs.push_back_reorder (m_entry_pool.size ());
	    gdb_assert (!value_set.empty ());
	    for (const symbol_value &value : value_set)
	      {
		int &idx = m_indexkey_to_idx[index_key (value.dwarf_tag,
							value.is_static,
							value.kind)];
		if (idx == 0)
		  {
		    idx = m_idx_next++;
		    m_abbrev_table.append_unsigned_leb128 (idx);
		    m_abbrev_table.append_unsigned_leb128 (value.dwarf_tag);
		    m_abbrev_table.append_unsigned_leb128
			      (value.kind == unit_kind::cu ? DW_IDX_compile_unit
							   : DW_IDX_type_unit);
		    m_abbrev_table.append_unsigned_leb128 (DW_FORM_udata);
		    m_abbrev_table.append_unsigned_leb128 (value.is_static
							   ? DW_IDX_GNU_internal
							   : DW_IDX_GNU_external);
		    m_abbrev_table.append_unsigned_leb128 (DW_FORM_flag_present);

		    /* Terminate attributes list.  */
		    m_abbrev_table.append_unsigned_leb128 (0);
		    m_abbrev_table.append_unsigned_leb128 (0);
		  }

		m_entry_pool.append_unsigned_leb128 (idx);
		m_entry_pool.append_unsigned_leb128 (value.cu_index);
	      }

	    /* Terminate the list of CUs.  */
	    m_entry_pool.append_unsigned_leb128 (0);
	  }
      }
    gdb_assert (m_hash_table.size () == name_count);

    /* Terminate tags list.  */
    m_abbrev_table.append_unsigned_leb128 (0);
  }

  /* Return .debug_names bucket count.  This must be called only after
     calling the build method.  */
  uint32_t bucket_count () const
  {
    /* Verify the build method has been already called.  */
    gdb_assert (!m_abbrev_table.empty ());
    const uint32_t retval = m_bucket_table.size ();

    /* Check for overflow.  */
    gdb_assert (retval == m_bucket_table.size ());
    return retval;
  }

  /* Return .debug_names names count.  This must be called only after
     calling the build method.  */
  uint32_t name_count () const
  {
    /* Verify the build method has been already called.  */
    gdb_assert (!m_abbrev_table.empty ());
    const uint32_t retval = m_hash_table.size ();

    /* Check for overflow.  */
    gdb_assert (retval == m_hash_table.size ());
    return retval;
  }

  /* Return number of bytes of .debug_names abbreviation table.  This
     must be called only after calling the build method.  */
  uint32_t abbrev_table_bytes () const
  {
    gdb_assert (!m_abbrev_table.empty ());
    return m_abbrev_table.size ();
  }

  /* Return number of bytes the .debug_names section will have.  This
     must be called only after calling the build method.  */
  size_t bytes () const
  {
    /* Verify the build method has been already called.  */
    gdb_assert (!m_abbrev_table.empty ());
    size_t expected_bytes = 0;
    expected_bytes += m_bucket_table.size () * sizeof (m_bucket_table[0]);
    expected_bytes += m_hash_table.size () * sizeof (m_hash_table[0]);
    expected_bytes += m_name_table_string_offs.bytes ();
    expected_bytes += m_name_table_entry_offs.bytes ();
    expected_bytes += m_abbrev_table.size ();
    expected_bytes += m_entry_pool.size ();
    return expected_bytes;
  }

  /* Write .debug_names to FILE_NAMES and .debug_str addition to
     FILE_STR.  This must be called only after calling the build
     method.  */
  void file_write (FILE *file_names, FILE *file_str) const
  {
    /* Verify the build method has been already called.  */
    gdb_assert (!m_abbrev_table.empty ());
    ::file_write (file_names, m_bucket_table);
    ::file_write (file_names, m_hash_table);
    m_name_table_string_offs.file_write (file_names);
    m_name_table_entry_offs.file_write (file_names);
    m_abbrev_table.file_write (file_names);
    m_entry_pool.file_write (file_names);
    m_debugstrlookup.file_write (file_str);
  }

  void add_cu (dwarf2_per_cu_data *per_cu, offset_type index)
  {
    m_cu_index_htab.emplace (per_cu, index);
  }

private:

  /* Storage for symbol names mapping them to their .debug_str section
     offsets.  */
  class debug_str_lookup
  {
  public:

    /* Object constructor to be called for current DWARF2_PER_BFD.
       All .debug_str section strings are automatically stored.  */
    debug_str_lookup (dwarf2_per_bfd *per_bfd)
      : m_abfd (per_bfd->obfd),
	m_per_bfd (per_bfd)
    {
      gdb_assert (per_bfd->str.readin);
      if (per_bfd->str.buffer == NULL)
	return;
      for (const gdb_byte *data = per_bfd->str.buffer;
	   data < (per_bfd->str.buffer
		   + per_bfd->str.size);)
	{
	  const char *const s = reinterpret_cast<const char *> (data);
	  const auto insertpair
	    = m_str_table.emplace (c_str_view (s),
				   data - per_bfd->str.buffer);
	  if (!insertpair.second)
	    complaint (_("Duplicate string \"%s\" in "
			 ".debug_str section [in module %s]"),
		       s, bfd_get_filename (m_abfd));
	  data += strlen (s) + 1;
	}
    }

    /* Return offset of symbol name S in the .debug_str section.  Add
       such symbol to the section's end if it does not exist there
       yet.  */
    size_t lookup (const char *s)
    {
      const auto it = m_str_table.find (c_str_view (s));
      if (it != m_str_table.end ())
	return it->second;
      const size_t offset = (m_per_bfd->str.size
			     + m_str_add_buf.size ());
      m_str_table.emplace (c_str_view (s), offset);
      m_str_add_buf.append_cstr0 (s);
      return offset;
    }

    /* Append the end of the .debug_str section to FILE.  */
    void file_write (FILE *file) const
    {
      m_str_add_buf.file_write (file);
    }

  private:
    std::unordered_map<c_str_view, size_t, c_str_view_hasher> m_str_table;
    bfd *const m_abfd;
    dwarf2_per_bfd *m_per_bfd;

    /* Data to add at the end of .debug_str for new needed symbol names.  */
    data_buf m_str_add_buf;
  };

  /* Container to map used DWARF tags to their .debug_names abbreviation
     tags.  */
  class index_key
  {
  public:
    index_key (int dwarf_tag_, bool is_static_, unit_kind kind_)
      : dwarf_tag (dwarf_tag_), is_static (is_static_), kind (kind_)
    {
    }

    bool
    operator== (const index_key &other) const
    {
      return (dwarf_tag == other.dwarf_tag && is_static == other.is_static
	      && kind == other.kind);
    }

    const int dwarf_tag;
    const bool is_static;
    const unit_kind kind;
  };

  /* Provide std::unordered_map::hasher for index_key.  */
  class index_key_hasher
  {
  public:
    size_t
    operator () (const index_key &key) const
    {
      return (std::hash<int>() (key.dwarf_tag) << 1) | key.is_static;
    }
  };

  /* Parameters of one symbol entry.  */
  class symbol_value
  {
  public:
    const int dwarf_tag, cu_index;
    const bool is_static;
    const unit_kind kind;

    symbol_value (int dwarf_tag_, int cu_index_, bool is_static_,
		  unit_kind kind_)
      : dwarf_tag (dwarf_tag_), cu_index (cu_index_), is_static (is_static_),
	kind (kind_)
    {}

    bool
    operator< (const symbol_value &other) const
    {
#define X(n) \
  do \
    { \
      if (n < other.n) \
	return true; \
      if (n > other.n) \
	return false; \
    } \
  while (0)
      X (dwarf_tag);
      X (is_static);
      X (kind);
      X (cu_index);
#undef X
      return false;
    }
  };

  /* Abstract base class to unify DWARF-32 and DWARF-64 name table
     output.  */
  class offset_vec
  {
  protected:
    const bfd_endian dwarf5_byte_order;
  public:
    explicit offset_vec (bfd_endian dwarf5_byte_order_)
      : dwarf5_byte_order (dwarf5_byte_order_)
    {}

    /* Call std::vector::reserve for NELEM elements.  */
    virtual void reserve (size_t nelem) = 0;

    /* Call std::vector::push_back with store_unsigned_integer byte
       reordering for ELEM.  */
    virtual void push_back_reorder (size_t elem) = 0;

    /* Return expected output size in bytes.  */
    virtual size_t bytes () const = 0;

    /* Write name table to FILE.  */
    virtual void file_write (FILE *file) const = 0;
  };

  /* Template to unify DWARF-32 and DWARF-64 output.  */
  template<typename OffsetSize>
  class offset_vec_tmpl : public offset_vec
  {
  public:
    explicit offset_vec_tmpl (bfd_endian dwarf5_byte_order_)
      : offset_vec (dwarf5_byte_order_)
    {}

    /* Implement offset_vec::reserve.  */
    void reserve (size_t nelem) override
    {
      m_vec.reserve (nelem);
    }

    /* Implement offset_vec::push_back_reorder.  */
    void push_back_reorder (size_t elem) override
    {
      m_vec.push_back (elem);
      /* Check for overflow.  */
      gdb_assert (m_vec.back () == elem);
      store_unsigned_integer (reinterpret_cast<gdb_byte *> (&m_vec.back ()),
			      sizeof (m_vec.back ()), dwarf5_byte_order, elem);
    }

    /* Implement offset_vec::bytes.  */
    size_t bytes () const override
    {
      return m_vec.size () * sizeof (m_vec[0]);
    }

    /* Implement offset_vec::file_write.  */
    void file_write (FILE *file) const override
    {
      ::file_write (file, m_vec);
    }

  private:
    std::vector<OffsetSize> m_vec;
  };

  /* Base class to unify DWARF-32 and DWARF-64 .debug_names output
     respecting name table width.  */
  class dwarf
  {
  public:
    offset_vec &name_table_string_offs, &name_table_entry_offs;

    dwarf (offset_vec &name_table_string_offs_,
	   offset_vec &name_table_entry_offs_)
      : name_table_string_offs (name_table_string_offs_),
	name_table_entry_offs (name_table_entry_offs_)
    {
    }
  };

  /* Template to unify DWARF-32 and DWARF-64 .debug_names output
     respecting name table width.  */
  template<typename OffsetSize>
  class dwarf_tmpl : public dwarf
  {
  public:
    explicit dwarf_tmpl (bfd_endian dwarf5_byte_order_)
      : dwarf (m_name_table_string_offs, m_name_table_entry_offs),
	m_name_table_string_offs (dwarf5_byte_order_),
	m_name_table_entry_offs (dwarf5_byte_order_)
    {}

  private:
    offset_vec_tmpl<OffsetSize> m_name_table_string_offs;
    offset_vec_tmpl<OffsetSize> m_name_table_entry_offs;
  };

  /* Store value of each symbol.  */
  std::unordered_map<c_str_view, std::set<symbol_value>, c_str_view_hasher>
    m_name_to_value_set;

  /* Tables of DWARF-5 .debug_names.  They are in object file byte
     order.  */
  std::vector<uint32_t> m_bucket_table;
  std::vector<uint32_t> m_hash_table;

  const bfd_endian m_dwarf5_byte_order;
  dwarf_tmpl<uint32_t> m_dwarf32;
  dwarf_tmpl<uint64_t> m_dwarf64;
  dwarf &m_dwarf;
  offset_vec &m_name_table_string_offs, &m_name_table_entry_offs;
  debug_str_lookup m_debugstrlookup;

  /* Map each used .debug_names abbreviation tag parameter to its
     index value.  */
  std::unordered_map<index_key, int, index_key_hasher> m_indexkey_to_idx;

  /* Next unused .debug_names abbreviation tag for
     m_indexkey_to_idx.  */
  int m_idx_next = 1;

  /* .debug_names abbreviation table.  */
  data_buf m_abbrev_table;

  /* .debug_names entry pool.  */
  data_buf m_entry_pool;

  /* Temporary storage for Ada names.  */
  auto_obstack m_string_obstack;

  cu_index_map m_cu_index_htab;
};

/* Return iff any of the needed offsets does not fit into 32-bit
   .debug_names section.  */

static bool
check_dwarf64_offsets (dwarf2_per_bfd *per_bfd)
{
  for (const auto &per_cu : per_bfd->all_units)
    {
      if (to_underlying (per_cu->sect_off)
	  >= (static_cast<uint64_t> (1) << 32))
	return true;
    }
  return false;
}

/* Assert that FILE's size is EXPECTED_SIZE.  Assumes file's seek
   position is at the end of the file.  */

static void
assert_file_size (FILE *file, size_t expected_size)
{
  const auto file_size = ftell (file);
  if (file_size == -1)
    perror_with_name (("ftell"));
  gdb_assert (file_size == expected_size);
}

/* Write a gdb index file to OUT_FILE from all the sections passed as
   arguments.  */

static void
write_gdbindex_1 (FILE *out_file,
		  const data_buf &cu_list,
		  const data_buf &types_cu_list,
		  const data_buf &addr_vec,
		  const data_buf &symtab_vec,
		  const data_buf &constant_pool,
		  const data_buf &shortcuts)
{
  data_buf contents;
  const offset_type size_of_header = 7 * sizeof (offset_type);
  uint64_t total_len = size_of_header;

  /* The version number.  */
  contents.append_offset (9);

  /* The offset of the CU list from the start of the file.  */
  contents.append_offset (total_len);
  total_len += cu_list.size ();

  /* The offset of the types CU list from the start of the file.  */
  contents.append_offset (total_len);
  total_len += types_cu_list.size ();

  /* The offset of the address table from the start of the file.  */
  contents.append_offset (total_len);
  total_len += addr_vec.size ();

  /* The offset of the symbol table from the start of the file.  */
  contents.append_offset (total_len);
  total_len += symtab_vec.size ();

  /* The offset of the shortcut table from the start of the file.  */
  contents.append_offset (total_len);
  total_len += shortcuts.size ();

  /* The offset of the constant pool from the start of the file.  */
  contents.append_offset (total_len);
  total_len += constant_pool.size ();

  gdb_assert (contents.size () == size_of_header);

  /* The maximum size of an index file is limited by the maximum value
     capable of being represented by 'offset_type'.  Throw an error if
     that length has been exceeded.  */
  size_t max_size = ~(offset_type) 0;
  if (total_len > max_size)
    error (_("gdb-index maximum file size of %zu exceeded"), max_size);

  if (out_file == nullptr)
    return;

  contents.file_write (out_file);
  cu_list.file_write (out_file);
  types_cu_list.file_write (out_file);
  addr_vec.file_write (out_file);
  symtab_vec.file_write (out_file);
  shortcuts.file_write (out_file);
  constant_pool.file_write (out_file);

  assert_file_size (out_file, total_len);
}

/* Write the contents of the internal "cooked" index.  */

static void
write_cooked_index (cooked_index *table,
		    const cu_index_map &cu_index_htab,
		    struct mapped_symtab *symtab)
{
  for (const cooked_index_entry *entry : table->all_entries ())
    {
      const auto it = cu_index_htab.find (entry->per_cu);
      gdb_assert (it != cu_index_htab.cend ());

      const char *name = entry->full_name (symtab->obstack ());

      if (entry->per_cu->lang () == language_ada)
	{
	  /* In order for the index to work when read back into
	     gdb, it has to use the encoded name, with any
	     suffixes stripped.  */
	  std::string encoded = ada_encode (name, false);
	  name = obstack_strdup (symtab->obstack (), encoded.c_str ());
	}
      else if (entry->per_cu->lang () == language_cplus
	       && (entry->flags & IS_LINKAGE) != 0)
	{
	  /* GDB never put C++ linkage names into .gdb_index.  The
	     theory here is that a linkage name will normally be in
	     the minimal symbols anyway, so including it in the index
	     is usually redundant -- and the cases where it would not
	     be redundant are rare and not worth supporting.  */
	  continue;
	}
      else if ((entry->flags & IS_TYPE_DECLARATION) != 0)
	{
	  /* Don't add type declarations to the index.  */
	  continue;
	}

      gdb_index_symbol_kind kind;
      if (entry->tag == DW_TAG_subprogram
	  || entry->tag == DW_TAG_entry_point)
	kind = GDB_INDEX_SYMBOL_KIND_FUNCTION;
      else if (entry->tag == DW_TAG_variable
	       || entry->tag == DW_TAG_constant
	       || entry->tag == DW_TAG_enumerator)
	kind = GDB_INDEX_SYMBOL_KIND_VARIABLE;
      else if (entry->tag == DW_TAG_module
	       || entry->tag == DW_TAG_common_block)
	kind = GDB_INDEX_SYMBOL_KIND_OTHER;
      else
	kind = GDB_INDEX_SYMBOL_KIND_TYPE;

      symtab->add_index_entry (name, (entry->flags & IS_STATIC) != 0,
			       kind, it->second);
    }
}

/* Write shortcut information.  */

static void
write_shortcuts_table (cooked_index *table, data_buf &shortcuts,
		       data_buf &cpool)
{
  const auto main_info = table->get_main ();
  size_t main_name_offset = 0;
  dwarf_source_language dw_lang = (dwarf_source_language) 0;

  if (main_info != nullptr)
    {
      dw_lang = main_info->per_cu->dw_lang ();

      if (dw_lang != 0)
	{
	  auto_obstack obstack;
	  const auto main_name = main_info->full_name (&obstack, true);

	  main_name_offset = cpool.size ();
	  cpool.append_cstr0 (main_name);
	}
    }

  shortcuts.append_offset (dw_lang);
  shortcuts.append_offset (main_name_offset);
}

/* Write contents of a .gdb_index section for OBJFILE into OUT_FILE.
   If OBJFILE has an associated dwz file, write contents of a .gdb_index
   section for that dwz file into DWZ_OUT_FILE.  If OBJFILE does not have an
   associated dwz file, DWZ_OUT_FILE must be NULL.  */

static void
write_gdbindex (dwarf2_per_bfd *per_bfd, cooked_index *table,
		FILE *out_file, FILE *dwz_out_file)
{
  mapped_symtab symtab;
  data_buf objfile_cu_list;
  data_buf dwz_cu_list;

  /* While we're scanning CU's create a table that maps a dwarf2_per_cu_data
     (which is what addrmap records) to its index (which is what is recorded
     in the index file).  This will later be needed to write the address
     table.  */
  cu_index_map cu_index_htab;
  cu_index_htab.reserve (per_bfd->all_units.size ());

  /* Store out the .debug_type CUs, if any.  */
  data_buf types_cu_list;

  /* The CU list is already sorted, so we don't need to do additional
     work here.  */

  int counter = 0;
  for (int i = 0; i < per_bfd->all_units.size (); ++i)
    {
      dwarf2_per_cu_data *per_cu = per_bfd->all_units[i].get ();

      const auto insertpair = cu_index_htab.emplace (per_cu, counter);
      gdb_assert (insertpair.second);

      /* See enhancement PR symtab/30838.  */
      gdb_assert (!(per_cu->is_dwz && per_cu->is_debug_types));

      /* The all_units list contains CUs read from the objfile as well as
	 from the eventual dwz file.  We need to place the entry in the
	 corresponding index.  */
      data_buf &cu_list = (per_cu->is_debug_types
			   ? types_cu_list
			   : per_cu->is_dwz ? dwz_cu_list : objfile_cu_list);
      cu_list.append_uint (8, BFD_ENDIAN_LITTLE,
			   to_underlying (per_cu->sect_off));
      if (per_cu->is_debug_types)
	{
	  signatured_type *sig_type = (signatured_type *) per_cu;
	  cu_list.append_uint (8, BFD_ENDIAN_LITTLE,
			       to_underlying (sig_type->type_offset_in_tu));
	  cu_list.append_uint (8, BFD_ENDIAN_LITTLE,
			       sig_type->signature);
	}
      else
	cu_list.append_uint (8, BFD_ENDIAN_LITTLE, per_cu->length ());

      ++counter;
    }

  write_cooked_index (table, cu_index_htab, &symtab);

  /* Dump the address map.  */
  data_buf addr_vec;
  for (auto map : table->get_addrmaps ())
    write_address_map (map, addr_vec, cu_index_htab);

  /* Ensure symbol hash is built domestically.  */
  symtab.sort ();

  /* Now that we've processed all symbols we can shrink their cu_indices
     lists.  */
  symtab.minimize ();

  data_buf symtab_vec, constant_pool;

  write_hash_table (&symtab, symtab_vec, constant_pool);

  data_buf shortcuts;
  write_shortcuts_table (table, shortcuts, constant_pool);

  write_gdbindex_1 (out_file, objfile_cu_list, types_cu_list, addr_vec,
		    symtab_vec, constant_pool, shortcuts);

  if (dwz_out_file != NULL)
    write_gdbindex_1 (dwz_out_file, dwz_cu_list, {}, {}, {}, {}, {});
  else
    gdb_assert (dwz_cu_list.empty ());
}

/* DWARF-5 augmentation string for GDB's DW_IDX_GNU_* extension.  */
static const gdb_byte dwarf5_gdb_augmentation[] = { 'G', 'D', 'B', 0 };

/* Write a new .debug_names section for OBJFILE into OUT_FILE, write
   needed addition to .debug_str section to OUT_FILE_STR.  Return how
   many bytes were expected to be written into OUT_FILE.  */

static void
write_debug_names (dwarf2_per_bfd *per_bfd, cooked_index *table,
		   FILE *out_file, FILE *out_file_str)
{
  const bool dwarf5_is_dwarf64 = check_dwarf64_offsets (per_bfd);
  const enum bfd_endian dwarf5_byte_order
    = bfd_big_endian (per_bfd->obfd) ? BFD_ENDIAN_BIG : BFD_ENDIAN_LITTLE;

  /* The CU list is already sorted, so we don't need to do additional
     work here.  Also, the debug_types entries do not appear in
     all_units, but only in their own hash table.  */
  data_buf cu_list;
  data_buf types_cu_list;
  debug_names nametable (per_bfd, dwarf5_is_dwarf64, dwarf5_byte_order);
  int counter = 0;
  int types_counter = 0;
  for (int i = 0; i < per_bfd->all_units.size (); ++i)
    {
      dwarf2_per_cu_data *per_cu = per_bfd->all_units[i].get ();

      int &this_counter = per_cu->is_debug_types ? types_counter : counter;
      data_buf &this_list = per_cu->is_debug_types ? types_cu_list : cu_list;

      nametable.add_cu (per_cu, this_counter);
      this_list.append_uint (nametable.dwarf5_offset_size (),
			     dwarf5_byte_order,
			     to_underlying (per_cu->sect_off));
      ++this_counter;
    }

   /* Verify that all units are represented.  */
  gdb_assert (counter == per_bfd->all_comp_units.size ());
  gdb_assert (types_counter == per_bfd->all_type_units.size ());

  for (const cooked_index_entry *entry : table->all_entries ())
    nametable.insert (entry);

  nametable.build ();

  /* No addr_vec - DWARF-5 uses .debug_aranges generated by GCC.  */

  const offset_type bytes_of_header
    = ((dwarf5_is_dwarf64 ? 12 : 4)
       + 2 + 2 + 7 * 4
       + sizeof (dwarf5_gdb_augmentation));
  size_t expected_bytes = 0;
  expected_bytes += bytes_of_header;
  expected_bytes += cu_list.size ();
  expected_bytes += types_cu_list.size ();
  expected_bytes += nametable.bytes ();
  data_buf header;

  if (!dwarf5_is_dwarf64)
    {
      const uint64_t size64 = expected_bytes - 4;
      gdb_assert (size64 < 0xfffffff0);
      header.append_uint (4, dwarf5_byte_order, size64);
    }
  else
    {
      header.append_uint (4, dwarf5_byte_order, 0xffffffff);
      header.append_uint (8, dwarf5_byte_order, expected_bytes - 12);
    }

  /* The version number.  */
  header.append_uint (2, dwarf5_byte_order, 5);

  /* Padding.  */
  header.append_uint (2, dwarf5_byte_order, 0);

  /* comp_unit_count - The number of CUs in the CU list.  */
  header.append_uint (4, dwarf5_byte_order, counter);

  /* local_type_unit_count - The number of TUs in the local TU
     list.  */
  header.append_uint (4, dwarf5_byte_order, types_counter);

  /* foreign_type_unit_count - The number of TUs in the foreign TU
     list.  */
  header.append_uint (4, dwarf5_byte_order, 0);

  /* bucket_count - The number of hash buckets in the hash lookup
     table.  */
  header.append_uint (4, dwarf5_byte_order, nametable.bucket_count ());

  /* name_count - The number of unique names in the index.  */
  header.append_uint (4, dwarf5_byte_order, nametable.name_count ());

  /* abbrev_table_size - The size in bytes of the abbreviations
     table.  */
  header.append_uint (4, dwarf5_byte_order, nametable.abbrev_table_bytes ());

  /* augmentation_string_size - The size in bytes of the augmentation
     string.  This value is rounded up to a multiple of 4.  */
  static_assert (sizeof (dwarf5_gdb_augmentation) % 4 == 0, "");
  header.append_uint (4, dwarf5_byte_order, sizeof (dwarf5_gdb_augmentation));
  header.append_array (dwarf5_gdb_augmentation);

  gdb_assert (header.size () == bytes_of_header);

  header.file_write (out_file);
  cu_list.file_write (out_file);
  types_cu_list.file_write (out_file);
  nametable.file_write (out_file, out_file_str);

  assert_file_size (out_file, expected_bytes);
}

/* This represents an index file being written (work-in-progress).

   The data is initially written to a temporary file.  When the finalize method
   is called, the file is closed and moved to its final location.

   On failure (if this object is being destroyed with having called finalize),
   the temporary file is closed and deleted.  */

struct index_wip_file
{
  index_wip_file (const char *dir, const char *basename,
		  const char *suffix)
  {
    /* Validate DIR is a valid directory.  */
    struct stat buf;
    if (stat (dir, &buf) == -1)
      perror_with_name (string_printf (_("`%s'"), dir).c_str ());
    if ((buf.st_mode & S_IFDIR) != S_IFDIR)
      error (_("`%s': Is not a directory."), dir);

    filename = (std::string (dir) + SLASH_STRING + basename
		+ suffix);

    filename_temp = make_temp_filename (filename);

    scoped_fd out_file_fd = gdb_mkostemp_cloexec (filename_temp.data (),
						  O_BINARY);
    if (out_file_fd.get () == -1)
      perror_with_name (string_printf (_("couldn't open `%s'"),
				       filename_temp.data ()).c_str ());

    out_file = out_file_fd.to_file ("wb");

    if (out_file == nullptr)
      error (_("Can't open `%s' for writing"), filename_temp.data ());

    unlink_file.emplace (filename_temp.data ());
  }

  void finalize ()
  {
    /* We want to keep the file.  */
    unlink_file->keep ();

    /* Close and move the str file in place.  */
    unlink_file.reset ();
    if (rename (filename_temp.data (), filename.c_str ()) != 0)
      perror_with_name (("rename"));
  }

  std::string filename;
  gdb::char_vector filename_temp;

  /* Order matters here; we want FILE to be closed before
     FILENAME_TEMP is unlinked, because on MS-Windows one cannot
     delete a file that is still open.  So, we wrap the unlinker in an
     optional and emplace it once we know the file name.  */
  std::optional<gdb::unlinker> unlink_file;

  gdb_file_up out_file;
};

/* See dwarf-index-write.h.  */

void
write_dwarf_index (dwarf2_per_bfd *per_bfd, const char *dir,
		   const char *basename, const char *dwz_basename,
		   dw_index_kind index_kind)
{
  if (per_bfd->index_table == nullptr)
    error (_("No debugging symbols"));
  cooked_index *table = per_bfd->index_table->index_for_writing ();

  if (per_bfd->types.size () > 1)
    error (_("Cannot make an index when the file has multiple .debug_types sections"));

  const char *index_suffix = (index_kind == dw_index_kind::DEBUG_NAMES
			      ? INDEX5_SUFFIX : INDEX4_SUFFIX);

  index_wip_file objfile_index_wip (dir, basename, index_suffix);
  std::optional<index_wip_file> dwz_index_wip;

  if (dwz_basename != NULL)
      dwz_index_wip.emplace (dir, dwz_basename, index_suffix);

  if (index_kind == dw_index_kind::DEBUG_NAMES)
    {
      index_wip_file str_wip_file (dir, basename, DEBUG_STR_SUFFIX);

      write_debug_names (per_bfd, table, objfile_index_wip.out_file.get (),
			 str_wip_file.out_file.get ());

      str_wip_file.finalize ();
    }
  else
    write_gdbindex (per_bfd, table, objfile_index_wip.out_file.get (),
		    (dwz_index_wip.has_value ()
		     ? dwz_index_wip->out_file.get () : NULL));

  objfile_index_wip.finalize ();

  if (dwz_index_wip.has_value ())
    dwz_index_wip->finalize ();
}

/* Options structure for the 'save gdb-index' command.  */

struct save_gdb_index_options
{
  bool dwarf_5 = false;
};

/* The option_def list for the 'save gdb-index' command.  */

static const gdb::option::option_def save_gdb_index_options_defs[] = {
  gdb::option::boolean_option_def<save_gdb_index_options> {
    "dwarf-5",
    [] (save_gdb_index_options *opt) { return &opt->dwarf_5; },
    nullptr, /* show_cmd_cb */
    nullptr /* set_doc */
  }
};

/* Create an options_def_group for the 'save gdb-index' command.  */

static gdb::option::option_def_group
make_gdb_save_index_options_def_group (save_gdb_index_options *opts)
{
  return {{save_gdb_index_options_defs}, opts};
}

/* Completer for the "save gdb-index" command.  */

static void
gdb_save_index_cmd_completer (struct cmd_list_element *ignore,
			      completion_tracker &tracker,
			      const char *text, const char *word)
{
  auto grp = make_gdb_save_index_options_def_group (nullptr);
  if (gdb::option::complete_options
      (tracker, &text, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_OPERAND, grp))
    return;

  word = advance_to_filename_complete_word_point (tracker, text);
  filename_completer (ignore, tracker, text, word);
}

/* Implementation of the `save gdb-index' command.

   Note that the .gdb_index file format used by this command is
   documented in the GDB manual.  Any changes here must be documented
   there.  */

static void
save_gdb_index_command (const char *args, int from_tty)
{
  save_gdb_index_options opts;
  const auto group = make_gdb_save_index_options_def_group (&opts);
  gdb::option::process_options
    (&args, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_OPERAND, group);

  if (args == nullptr || *args == '\0')
    error (_("usage: save gdb-index [-dwarf-5] DIRECTORY"));

  std::string directory (gdb_tilde_expand (args));
  dw_index_kind index_kind
    = (opts.dwarf_5 ? dw_index_kind::DEBUG_NAMES : dw_index_kind::GDB_INDEX);

  for (objfile *objfile : current_program_space->objfiles ())
    {
      /* If the objfile does not correspond to an actual file, skip it.  */
      if ((objfile->flags & OBJF_NOT_FILENAME) != 0)
	continue;

      dwarf2_per_objfile *per_objfile = get_dwarf2_per_objfile (objfile);

      if (per_objfile != NULL)
	{
	  try
	    {
	      const char *basename = lbasename (objfile_name (objfile));
	      const dwz_file *dwz = dwarf2_get_dwz_file (per_objfile->per_bfd);
	      const char *dwz_basename = NULL;

	      if (dwz != NULL)
		dwz_basename = lbasename (dwz->filename ());

	      write_dwarf_index (per_objfile->per_bfd, directory.c_str (),
				 basename, dwz_basename, index_kind);
	    }
	  catch (const gdb_exception_error &except)
	    {
	      exception_fprintf (gdb_stderr, except,
				 _("Error while writing index for `%s': "),
				 objfile_name (objfile));
	    }
	}

    }
}

#if GDB_SELF_TEST
#include "gdbsupport/selftest.h"

namespace selftests {

class pretend_data_buf : public data_buf
{
public:
  /* Set the pretend size.  */
  void set_pretend_size (size_t s) {
    m_pretend_size = s;
  }

  /* Override size method of data_buf, returning the pretend size instead.  */
  size_t size () const override {
    return m_pretend_size;
  }

private:
  size_t m_pretend_size = 0;
};

static void
gdb_index ()
{
  pretend_data_buf cu_list;
  pretend_data_buf types_cu_list;
  pretend_data_buf addr_vec;
  pretend_data_buf symtab_vec;
  pretend_data_buf constant_pool;
  pretend_data_buf short_cuts;

  const size_t size_of_header = 7 * sizeof (offset_type);

  /* Test that an overly large index will throw an error.  */
  symtab_vec.set_pretend_size (~(offset_type)0 - size_of_header);
  constant_pool.set_pretend_size (1);

  bool saw_exception = false;
  try
    {
      write_gdbindex_1 (nullptr, cu_list, types_cu_list, addr_vec,
			symtab_vec, constant_pool, short_cuts);
    }
  catch (const gdb_exception_error &e)
    {
      SELF_CHECK (e.reason == RETURN_ERROR);
      SELF_CHECK (e.error == GENERIC_ERROR);
      SELF_CHECK (e.message->find (_("gdb-index maximum file size of"))
		  != std::string::npos);
      SELF_CHECK (e.message->find (_("exceeded")) != std::string::npos);
      saw_exception = true;
    }
  SELF_CHECK (saw_exception);

  /* Test that the largest possible index will not throw an error.  */
  constant_pool.set_pretend_size (0);

  saw_exception = false;
  try
    {
      write_gdbindex_1 (nullptr, cu_list, types_cu_list, addr_vec,
			symtab_vec, constant_pool, short_cuts);
    }
  catch (const gdb_exception_error &e)
    {
      saw_exception = true;
    }
  SELF_CHECK (!saw_exception);
}

} /* selftests namespace.  */
#endif

void _initialize_dwarf_index_write ();
void
_initialize_dwarf_index_write ()
{
#if GDB_SELF_TEST
  selftests::register_test ("gdb_index", selftests::gdb_index);
#endif

  cmd_list_element *c = add_cmd ("gdb-index", class_files,
				 save_gdb_index_command, _("\
Save a gdb-index file.\n\
Usage: save gdb-index [-dwarf-5] DIRECTORY\n\
\n\
No options create one file with .gdb-index extension for pre-DWARF-5\n\
compatible .gdb_index section.  With -dwarf-5 creates two files with\n\
extension .debug_names and .debug_str for DWARF-5 .debug_names section."),
	       &save_cmdlist);
  set_cmd_completer_handle_brkchars (c, gdb_save_index_cmd_completer);
}
