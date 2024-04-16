/* Code dealing with blocks for GDB.

   Copyright (C) 2003-2024 Free Software Foundation, Inc.

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

#ifndef BLOCK_H
#define BLOCK_H

#include "dictionary.h"
#include "gdbsupport/array-view.h"

/* Opaque declarations.  */

struct symbol;
struct compunit_symtab;
struct block_namespace_info;
struct using_direct;
struct obstack;
struct addrmap;

/* Blocks can occupy non-contiguous address ranges.  When this occurs,
   startaddr and endaddr within struct block (still) specify the lowest
   and highest addresses of all ranges, but each individual range is
   specified by the addresses in struct blockrange.  */

struct blockrange
{
  blockrange (CORE_ADDR start, CORE_ADDR end)
    : m_start (start),
      m_end (end)
  {
  }

  /* Return this blockrange's start address.  */
  CORE_ADDR start () const
  { return m_start; }

  /* Set this blockrange's start address.  */
  void set_start (CORE_ADDR start)
  { m_start = start; }

  /* Return this blockrange's end address.  */
  CORE_ADDR end () const
  { return m_end; }

  /* Set this blockrange's end address.  */
  void set_end (CORE_ADDR end)
  { m_end = end; }

  /* Lowest address in this range.  */

  CORE_ADDR m_start;

  /* One past the highest address in the range.  */

  CORE_ADDR m_end;
};

/* Two or more non-contiguous ranges in the same order as that provided
   via the debug info.  */

struct blockranges
{
  int nranges;
  struct blockrange range[1];
};

/* All of the name-scope contours of the program
   are represented by `struct block' objects.
   All of these objects are pointed to by the blockvector.

   Each block represents one name scope.
   Each lexical context has its own block.

   The blockvector begins with some special blocks.
   The GLOBAL_BLOCK contains all the symbols defined in this compilation
   whose scope is the entire program linked together.
   The STATIC_BLOCK contains all the symbols whose scope is the
   entire compilation excluding other separate compilations.
   Blocks starting with the FIRST_LOCAL_BLOCK are not special.

   Each block records a range of core addresses for the code that
   is in the scope of the block.  The STATIC_BLOCK and GLOBAL_BLOCK
   give, for the range of code, the entire range of code produced
   by the compilation that the symbol segment belongs to.

   The blocks appear in the blockvector
   in order of increasing starting-address,
   and, within that, in order of decreasing ending-address.

   This implies that within the body of one function
   the blocks appear in the order of a depth-first tree walk.  */

struct block : public allocate_on_obstack
{
  /* Return this block's start address.  */
  CORE_ADDR start () const
  { return m_start; }

  /* Set this block's start address.  */
  void set_start (CORE_ADDR start)
  { m_start = start; }

  /* Return this block's end address.  */
  CORE_ADDR end () const
  { return m_end; }

  /* Set this block's end address.  */
  void set_end (CORE_ADDR end)
  { m_end = end; }

  /* Return this block's function symbol.  */
  symbol *function () const
  { return m_function; }

  /* Set this block's function symbol.  */
  void set_function (symbol *function)
  { m_function = function; }

  /* Return this block's superblock.  */
  const block *superblock () const
  { return m_superblock; }

  /* Set this block's superblock.  */
  void set_superblock (const block *superblock)
  { m_superblock = superblock; }

  /* Return this block's multidict.  */
  multidictionary *multidict () const
  { return m_multidict; }

  /* Return an iterator range for this block's multidict.  */
  iterator_range<mdict_iterator_wrapper> multidict_symbols () const
  { return iterator_range<mdict_iterator_wrapper> (m_multidict); }

  /* Set this block's multidict.  */
  void set_multidict (multidictionary *multidict)
  { m_multidict = multidict; }

  /* Return a view on this block's ranges.  */
  gdb::array_view<blockrange> ranges ()
  {
    if (m_ranges == nullptr)
      return {};
    else
      return gdb::make_array_view (m_ranges->range, m_ranges->nranges);
  }

  /* Const version of the above.  */
  gdb::array_view<const blockrange> ranges () const
  {
    if (m_ranges == nullptr)
      return {};
    else
      return gdb::make_array_view (m_ranges->range, m_ranges->nranges);
  }

  /* Set this block's ranges array.  */
  void set_ranges (blockranges *ranges)
  { m_ranges = ranges; }

  /* Return true if all addresses within this block are contiguous.  */
  bool is_contiguous () const
  { return this->ranges ().size () <= 1; }

  /* Return the "entry PC" of this block.

     The entry PC is the lowest (start) address for the block when all addresses
     within the block are contiguous.  If non-contiguous, then use the start
     address for the first range in the block.

     At the moment, this almost matches what DWARF specifies as the entry
     pc.  (The missing bit is support for DW_AT_entry_pc which should be
     preferred over range data and the low_pc.)

     Once support for DW_AT_entry_pc is added, I expect that an entry_pc
     field will be added to one of these data structures.  Once that's done,
     the entry_pc field can be set from the dwarf reader (and other readers
     too).  ENTRY_PC can then be redefined to be less DWARF-centric.  */

  CORE_ADDR entry_pc () const
  {
    if (this->is_contiguous ())
      return this->start ();
    else
      return this->ranges ()[0].start ();
  }

  /* Return the objfile of this block.  */

  struct objfile *objfile () const;

  /* Return the architecture of this block.  */

  struct gdbarch *gdbarch () const;

  /* Return true if BL represents an inlined function.  */

  bool inlined_p () const;

  /* This returns the namespace that this block is enclosed in, or ""
     if it isn't enclosed in a namespace at all.  This travels the
     chain of superblocks looking for a scope, if necessary.  */

  const char *scope () const;

  /* Set this block's scope member to SCOPE; if needed, allocate
     memory via OBSTACK.  (It won't make a copy of SCOPE, however, so
     that already has to be allocated correctly.)  */

  void set_scope (const char *scope, struct obstack *obstack);

  /* This returns the using directives list associated with this
     block, if any.  */

  struct using_direct *get_using () const;

  /* Set this block's using member to USING; if needed, allocate
     memory via OBSTACK.  (It won't make a copy of USING, however, so
     that already has to be allocated correctly.)  */

  void set_using (struct using_direct *using_decl, struct obstack *obstack);

  /* Return the symbol for the function which contains a specified
     lexical block, described by a struct block.  The return value
     will not be an inlined function; the containing function will be
     returned instead.  */

  struct symbol *linkage_function () const;

  /* Return the symbol for the function which contains a specified
     block, described by a struct block.  The return value will be the
     closest enclosing function, which might be an inline
     function.  */

  struct symbol *containing_function () const;

  /* Return the static block associated with this block.  Return NULL
     if block is a global block.  */

  const struct block *static_block () const;

  /* Return true if this block is a static block.  */

  bool is_static_block () const
  {
    const block *sup = superblock ();
    if (sup == nullptr)
      return false;
    return sup->is_global_block ();
  }

  /* Return the static block associated with block.  */

  const struct block *global_block () const;

  /* Return true if this block is a global block.  */

  bool is_global_block () const
  { return superblock () == nullptr; }

  /* Return the function block for this block.  Returns nullptr if
     there is no enclosing function, i.e., if this block is a static
     or global block.  */

  const struct block *function_block () const;

  /* Set the compunit of this block, which must be a global block.  */

  void set_compunit_symtab (struct compunit_symtab *);

  /* Return a property to evaluate the static link associated to this
     block.

     In the context of nested functions (available in Pascal, Ada and
     GNU C, for instance), a static link (as in DWARF's
     DW_AT_static_link attribute) for a function is a way to get the
     frame corresponding to the enclosing function.

     Note that only objfile-owned and function-level blocks can have a
     static link.  Return NULL if there is no such property.  */

  struct dynamic_prop *static_link () const;

  /* Return true if block A is lexically nested within this block, or
     if A and this block have the same pc range.  Return false
     otherwise.  If ALLOW_NESTED is true, then block A is considered
     to be in this block if A is in a nested function in this block's
     function.  If ALLOW_NESTED is false (the default), then blocks in
     nested functions are not considered to be contained.  */

  bool contains (const struct block *a, bool allow_nested = false) const;

private:

  /* If the namespace_info is NULL, allocate it via OBSTACK and
     initialize its members to zero.  */
  void initialize_namespace (struct obstack *obstack);

  /* Addresses in the executable code that are in this block.  */

  CORE_ADDR m_start = 0;
  CORE_ADDR m_end = 0;

  /* The symbol that names this block, if the block is the body of a
     function (real or inlined); otherwise, zero.  */

  struct symbol *m_function = nullptr;

  /* The `struct block' for the containing block, or 0 if none.

     The superblock of a top-level local block (i.e. a function in the
     case of C) is the STATIC_BLOCK.  The superblock of the
     STATIC_BLOCK is the GLOBAL_BLOCK.  */

  const struct block *m_superblock = nullptr;

  /* This is used to store the symbols in the block.  */

  struct multidictionary *m_multidict = nullptr;

  /* Contains information about namespace-related info relevant to this block:
     using directives and the current namespace scope.  */

  struct block_namespace_info *m_namespace_info = nullptr;

  /* Address ranges for blocks with non-contiguous ranges.  If this
     is NULL, then there is only one range which is specified by
     startaddr and endaddr above.  */

  struct blockranges *m_ranges = nullptr;
};

/* The global block is singled out so that we can provide a back-link
   to the compunit symtab.  */

struct global_block : public block
{
  /* This holds a pointer to the compunit symtab holding this block.  */

  struct compunit_symtab *compunit_symtab = nullptr;
};

struct blockvector
{
  /* Return a view on the blocks of this blockvector.  */
  gdb::array_view<struct block *> blocks ()
  {
    return gdb::array_view<struct block *> (m_blocks, m_num_blocks);
  }

  /* Const version of the above.  */
  gdb::array_view<const struct block *const> blocks () const
  {
    const struct block **blocks = (const struct block **) m_blocks;
    return gdb::array_view<const struct block *const> (blocks, m_num_blocks);
  }

  /* Return the block at index I.  */
  struct block *block (size_t i)
  { return this->blocks ()[i]; }

  /* Const version of the above.  */
  const struct block *block (size_t i) const
  { return this->blocks ()[i]; }

  /* Set the block at index I.  */
  void set_block (int i, struct block *block)
  { m_blocks[i] = block; }

  /* Set the number of blocks of this blockvector.

     The storage of blocks is done using a flexible array member, so the number
     of blocks set here must agree with what was effectively allocated.  */
  void set_num_blocks (int num_blocks)
  { m_num_blocks = num_blocks; }

  /* Return the number of blocks in this blockvector.  */
  int num_blocks () const
  { return m_num_blocks; }

  /* Return the global block of this blockvector.  */
  struct block *global_block ()
  { return this->block (GLOBAL_BLOCK); }

  /* Const version of the above.  */
  const struct block *global_block () const
  { return this->block (GLOBAL_BLOCK); }

  /* Return the static block of this blockvector.  */
  struct block *static_block ()
  { return this->block (STATIC_BLOCK); }

  /* Const version of the above.  */
  const struct block *static_block () const
  { return this->block (STATIC_BLOCK); }

  /* Return the address -> block map of this blockvector.  */
  addrmap *map ()
  { return m_map; }

  /* Const version of the above.  */
  const addrmap *map () const
  { return m_map; }

  /* Set this blockvector's address -> block map.  */
  void set_map (addrmap *map)
  { m_map = map; }

private:
  /* An address map mapping addresses to blocks in this blockvector.
     This pointer is zero if the blocks' start and end addresses are
     enough.  */
  struct addrmap *m_map;

  /* Number of blocks in the list.  */
  int m_num_blocks;

  /* The blocks themselves.  */
  struct block *m_blocks[1];
};

extern const struct blockvector *blockvector_for_pc (CORE_ADDR,
					       const struct block **);

extern const struct blockvector *
  blockvector_for_pc_sect (CORE_ADDR, struct obj_section *,
			   const struct block **, struct compunit_symtab *);

extern int blockvector_contains_pc (const struct blockvector *bv, CORE_ADDR pc);

extern struct call_site *call_site_for_pc (struct gdbarch *gdbarch,
					   CORE_ADDR pc);

extern const struct block *block_for_pc (CORE_ADDR);

extern const struct block *block_for_pc_sect (CORE_ADDR, struct obj_section *);

/* A block iterator.  This structure should be treated as though it
   were opaque; it is only defined here because we want to support
   stack allocation of iterators.  */

struct block_iterator
{
  /* If we're iterating over a single block, this holds the block.
     Otherwise, it holds the canonical compunit.  */

  union
  {
    struct compunit_symtab *compunit_symtab;
    const struct block *block;
  } d;

  /* If we're trying to match a name, this will be non-NULL.  */
  const lookup_name_info *name;

  /* If we're iterating over a single block, this is always -1.
     Otherwise, it holds the index of the current "included" symtab in
     the canonical symtab (that is, d.symtab->includes[idx]), with -1
     meaning the canonical symtab itself.  */

  int idx;

  /* Which block, either static or global, to iterate over.  If this
     is FIRST_LOCAL_BLOCK, then we are iterating over a single block.
     This is used to select which field of 'd' is in use.  */

  enum block_enum which;

  /* The underlying multidictionary iterator.  */

  struct mdict_iterator mdict_iter;
};

/* Initialize ITERATOR to point at the first symbol in BLOCK, and
   return that first symbol, or NULL if BLOCK is empty.  If NAME is
   not NULL, only return symbols matching that name.  */

extern struct symbol *block_iterator_first
     (const struct block *block,
      struct block_iterator *iterator,
      const lookup_name_info *name = nullptr);

/* Advance ITERATOR, and return the next symbol, or NULL if there are
   no more symbols.  Don't call this if you've previously received
   NULL from block_iterator_first or block_iterator_next on this
   iteration.  */

extern struct symbol *block_iterator_next (struct block_iterator *iterator);

/* An iterator that wraps a block_iterator.  The naming here is
   unfortunate, but block_iterator was named before gdb switched to
   C++.  */
struct block_iterator_wrapper
{
  typedef block_iterator_wrapper self_type;
  typedef struct symbol *value_type;

  explicit block_iterator_wrapper (const struct block *block,
				   const lookup_name_info *name = nullptr)
    : m_sym (block_iterator_first (block, &m_iter, name))
  {
  }

  block_iterator_wrapper ()
    : m_sym (nullptr)
  {
  }

  value_type operator* () const
  {
    return m_sym;
  }

  bool operator== (const self_type &other) const
  {
    return m_sym == other.m_sym;
  }

  bool operator!= (const self_type &other) const
  {
    return m_sym != other.m_sym;
  }

  self_type &operator++ ()
  {
    m_sym = block_iterator_next (&m_iter);
    return *this;
  }

private:

  struct symbol *m_sym;
  struct block_iterator m_iter;
};

/* An iterator range for block_iterator_wrapper.  */

typedef iterator_range<block_iterator_wrapper> block_iterator_range;

/* Return true if symbol A is the best match possible for DOMAIN.  */

extern bool best_symbol (struct symbol *a, const domain_enum domain);

/* Return symbol B if it is a better match than symbol A for DOMAIN.
   Otherwise return A.  */

extern struct symbol *better_symbol (struct symbol *a, struct symbol *b,
				     const domain_enum domain);

/* Search BLOCK for symbol NAME in DOMAIN.  */

extern struct symbol *block_lookup_symbol (const struct block *block,
					   const char *name,
					   symbol_name_match_type match_type,
					   const domain_enum domain);

/* Search BLOCK for symbol NAME in DOMAIN but only in primary symbol table of
   BLOCK.  BLOCK must be STATIC_BLOCK or GLOBAL_BLOCK.  Function is useful if
   one iterates all global/static blocks of an objfile.  */

extern struct symbol *block_lookup_symbol_primary (const struct block *block,
						   const char *name,
						   const domain_enum domain);

/* Find symbol NAME in BLOCK and in DOMAIN.  This will return a
   matching symbol whose type is not a "opaque", see TYPE_IS_OPAQUE.
   If STUB is non-NULL, an otherwise matching symbol whose type is a
   opaque will be stored here.  */

extern struct symbol *block_find_symbol (const struct block *block,
					 const lookup_name_info &name,
					 const domain_enum domain,
					 struct symbol **stub);

/* Given a vector of pairs, allocate and build an obstack allocated
   blockranges struct for a block.  */
struct blockranges *make_blockranges (struct objfile *objfile,
				      const std::vector<blockrange> &rangevec);

#endif /* BLOCK_H */
