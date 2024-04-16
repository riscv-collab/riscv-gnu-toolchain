/* Public partial symbol table definitions.

   Copyright (C) 2009-2024 Free Software Foundation, Inc.

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

#ifndef PSYMTAB_H
#define PSYMTAB_H

#include "objfiles.h"
#include <string_view>
#include "gdbsupport/gdb_obstack.h"
#include "symfile.h"
#include "gdbsupport/next-iterator.h"
#include "bcache.h"

/* Specialization of bcache to store partial symbols.  */

struct psymbol_bcache : public gdb::bcache
{
  /* Calculate a hash code for the given partial symbol.  The hash is
     calculated using the symbol's value, language, domain, class
     and name.  These are the values which are set by
     add_psymbol_to_bcache.  */
  unsigned long hash (const void *addr, int length) override;

  /* Returns true if the symbol LEFT equals the symbol RIGHT.
     For the comparison this function uses a symbols value,
     language, domain, class and name.  */
  int compare (const void *left, const void *right, int length) override;
};

/* An instance of this class manages the partial symbol tables and
   partial symbols for a given objfile.

   The core psymtab functions -- those in psymtab.c -- arrange for
   nearly all psymtab- and psymbol-related allocations to happen
   either in the psymtab_storage object (either on its obstack or in
   other memory managed by this class), or on the per-BFD object.  The
   only link from the psymtab storage object back to the objfile (or
   objfile_obstack) that is made by the core psymtab code is the
   compunit_symtab member in the standard_psymtab -- and a given
   symbol reader can avoid this by implementing its own subclasses of
   partial_symtab.

   However, it is up to each symbol reader to maintain this invariant
   in other ways, if it wants to reuse psymtabs across multiple
   objfiles.  The main issue here is ensuring that read_symtab_private
   does not point into objfile_obstack.  */

class psymtab_storage
{
public:
  psymtab_storage () = default;
  ~psymtab_storage ();

  DISABLE_COPY_AND_ASSIGN (psymtab_storage);

  /* Discard all partial symbol tables starting with "psymtabs" and
     proceeding until "to" has been discarded.  */

  void discard_psymtabs_to (struct partial_symtab *to)
  {
    while (psymtabs != to)
      discard_psymtab (psymtabs);
  }

  /* Discard the partial symbol table.  */

  void discard_psymtab (struct partial_symtab *pst);

  /* Return the obstack that is used for storage by this object.  */

  struct obstack *obstack ()
  {
    if (!m_obstack.has_value ())
      m_obstack.emplace ();
    return &*m_obstack;
  }

  /* Allocate storage for the "dependencies" field of a psymtab.
     NUMBER says how many dependencies there are.  */

  struct partial_symtab **allocate_dependencies (int number)
  {
    return OBSTACK_CALLOC (obstack (), number, struct partial_symtab *);
  }

  /* Install a psymtab on the psymtab list.  This transfers ownership
     of PST to this object.  */

  void install_psymtab (partial_symtab *pst);

  using partial_symtab_range = next_range<partial_symtab>;

  /* A range adapter that makes it possible to iterate over all
     psymtabs in one objfile.  */

  partial_symtab_range range ()
  {
    return partial_symtab_range (psymtabs);
  }


  /* Each objfile points to a linked list of partial symtabs derived from
     this file, one partial symtab structure for each compilation unit
     (source file).  */

  struct partial_symtab *psymtabs = nullptr;

  /* A byte cache where we can stash arbitrary "chunks" of bytes that
     will not change.  */

  psymbol_bcache psymbol_cache;

private:

  /* The obstack where allocations are made.  This is lazily allocated
     so that we don't waste memory when there are no psymtabs.  */

  std::optional<auto_obstack> m_obstack;
};

/* A partial_symbol records the name, domain, and address class of
   symbols whose types we have not parsed yet.  For functions, it also
   contains their memory address, so we can find them from a PC value.
   Each partial_symbol sits in a partial_symtab, all of which are chained
   on a  partial symtab list and which points to the corresponding
   normal symtab once the partial_symtab has been referenced.  */

/* This structure is space critical.  See space comments at the top of
   symtab.h.  */

struct partial_symbol
{
  /* Return the section for this partial symbol, or nullptr if no
     section has been set.  */
  struct obj_section *obj_section (struct objfile *objfile) const
  {
    return ginfo.obj_section (objfile);
  }

  /* Return the unrelocated address of this partial symbol.  */
  unrelocated_addr unrelocated_address () const
  {
    return ginfo.unrelocated_address ();
  }

  /* Return the address of this partial symbol, relocated according to
     the offsets provided in OBJFILE.  */
  CORE_ADDR address (const struct objfile *objfile) const
  {
    return (CORE_ADDR (ginfo.unrelocated_address ())
	    + objfile->section_offsets[ginfo.section_index ()]);
  }

  /* Set the address of this partial symbol.  The address must be
     unrelocated.  */
  void set_unrelocated_address (unrelocated_addr addr)
  {
    ginfo.set_unrelocated_address (addr);
  }

  /* Note that partial_symbol does not derive from general_symbol_info
     due to the bcache.  See add_psymbol_to_bcache.  */

  struct general_symbol_info ginfo;

  /* Name space code.  */

  ENUM_BITFIELD(domain_enum) domain : SYMBOL_DOMAIN_BITS;

  /* Address class (for info_symbols).  Note that we don't allow
     synthetic "aclass" values here at present, simply because there's
     no need.  */

  ENUM_BITFIELD(address_class) aclass : SYMBOL_ACLASS_BITS;
};

/* A convenience enum to give names to some constants used when
   searching psymtabs.  This is internal to psymtab and should not be
   used elsewhere.  */

enum psymtab_search_status
  {
    PST_NOT_SEARCHED,
    PST_SEARCHED_AND_FOUND,
    PST_SEARCHED_AND_NOT_FOUND
  };

/* Specify whether a partial psymbol should be allocated on the global
   list or the static list.  */

enum class psymbol_placement
{
  STATIC,
  GLOBAL
};

/* Each source file that has not been fully read in is represented by
   a partial_symtab.  This contains the information on where in the
   executable the debugging symbols for a specific file are, and a
   list of names of global symbols which are located in this file.
   They are all chained on partial symtab lists.

   Even after the source file has been read into a symtab, the
   partial_symtab remains around.  */

struct partial_symtab
{
  /* Allocate a new partial symbol table.

     FILENAME (which must be non-NULL) is the filename of this partial
     symbol table; it is copied into the appropriate storage.  The
     partial symtab will also be installed using
     psymtab_storage::install.  */

  partial_symtab (const char *filename,
		  psymtab_storage *partial_symtabs,
		  objfile_per_bfd_storage *objfile_per_bfd)
    ATTRIBUTE_NONNULL (2) ATTRIBUTE_NONNULL (3);

  /* Like the above, but also sets the initial text low and text high
     from the ADDR argument, and sets the global- and
     static-offsets.  */

  partial_symtab (const char *filename,
		  psymtab_storage *partial_symtabs,
		  objfile_per_bfd_storage *objfile_per_bfd,
		  unrelocated_addr addr)
    ATTRIBUTE_NONNULL (2) ATTRIBUTE_NONNULL (3);

  virtual ~partial_symtab ()
  {
  }

  /* Psymtab expansion is done in two steps:
     - a call to read_symtab
     - while that call is in progress, calls to expand_psymtab can be made,
       both for this psymtab, and its dependencies.
     This makes a distinction between a toplevel psymtab (for which both
     read_symtab and expand_psymtab will be called) and a non-toplevel
     psymtab (for which only expand_psymtab will be called). The
     distinction can be used f.i. to do things before and after all
     dependencies of a top-level psymtab have been expanded.

     Read the full symbol table corresponding to this partial symbol
     table.  Typically calls expand_psymtab.  */
  virtual void read_symtab (struct objfile *) = 0;

  /* Expand the full symbol table for this partial symbol table.  Typically
     calls expand_dependencies.  */
  virtual void expand_psymtab (struct objfile *) = 0;

  /* Ensure that all the dependencies are read in.  Calls
     expand_psymtab for each non-shared dependency.  */
  void expand_dependencies (struct objfile *);

  /* Return true if the symtab corresponding to this psymtab has been
     read in in the context of this objfile.  */
  virtual bool readin_p (struct objfile *) const = 0;

  /* Return a pointer to the compunit allocated for this source file
     in the context of this objfile.

     Return nullptr if the compunit was not read in or if there was no
     symtab.  */
  virtual struct compunit_symtab *get_compunit_symtab
    (struct objfile *) const = 0;

  /* Return the unrelocated low text address of this
     partial_symtab.  */
  unrelocated_addr unrelocated_text_low () const
  {
    return m_text_low;
  }

  /* Return the unrelocated_addr high text address of this
     partial_symtab.  */
  unrelocated_addr unrelocated_text_high () const
  {
    return m_text_high;
  }

  /* Return the relocated low text address of this partial_symtab.  */
  CORE_ADDR text_low (struct objfile *objfile) const
  {
    return CORE_ADDR (m_text_low) + objfile->text_section_offset ();
  }

  /* Return the relocated high text address of this partial_symtab.  */
  CORE_ADDR text_high (struct objfile *objfile) const
  {
    return CORE_ADDR (m_text_high) + objfile->text_section_offset ();
  }

  /* Set the low text address of this partial_symtab.  */
  void set_text_low (unrelocated_addr addr)
  {
    m_text_low = addr;
    text_low_valid = 1;
  }

  /* Set the high text address of this partial_symtab.  */
  void set_text_high (unrelocated_addr addr)
  {
    m_text_high = addr;
    text_high_valid = 1;
  }

  /* Return true if this symtab is empty -- meaning that it contains
     no symbols.  It may still have dependencies.  */
  bool empty () const
  {
    return global_psymbols.empty () && static_psymbols.empty ();
  }

  /* Add a symbol to this partial symbol table of OBJFILE.

     If COPY_NAME is true, make a copy of NAME, otherwise use the passed
     reference.

     THECLASS is the type of symbol.

     SECTION is the index of the section of OBJFILE in which the symbol is found.

     WHERE determines whether the symbol goes in the list of static or global
     partial symbols.

     COREADDR is the address of the symbol.  For partial symbols that don't have
     an address, zero is passed.

     LANGUAGE is the language from which the symbol originates.  This will
     influence, amongst other things, how the symbol name is demangled. */

  void add_psymbol (std::string_view name,
		    bool copy_name, domain_enum domain,
		    enum address_class theclass,
		    short section,
		    psymbol_placement where,
		    unrelocated_addr coreaddr,
		    enum language language,
		    psymtab_storage *partial_symtabs,
		    struct objfile *objfile);

  /* Add a symbol to this partial symbol table of OBJFILE.  The psymbol
     must be fully constructed, and the names must be set and intern'd
     as appropriate.  */

  void add_psymbol (const partial_symbol &psym,
		    psymbol_placement where,
		    psymtab_storage *partial_symtabs,
		    struct objfile *objfile);


  /* Indicate that this partial symtab is complete.  */

  void end ();

  /* Chain of all existing partial symtabs.  */

  struct partial_symtab *next = nullptr;

  /* Name of the source file which this partial_symtab defines,
     or if the psymtab is anonymous then a descriptive name for
     debugging purposes, or "".  It must not be NULL.  */

  const char *filename = nullptr;

  /* Full path of the source file.  NULL if not known.  */

  char *fullname = nullptr;

  /* Directory in which it was compiled, or NULL if we don't know.  */

  const char *dirname = nullptr;

  /* Range of text addresses covered by this file; texthigh is the
     beginning of the next section.  Do not refer directly to these
     fields.  Instead, use the accessors.  The validity of these
     fields is determined by the text_low_valid and text_high_valid
     fields; these are located later in this structure for better
     packing.  */

  unrelocated_addr m_text_low {};
  unrelocated_addr m_text_high {};

  /* If NULL, this is an ordinary partial symbol table.

     If non-NULL, this holds a single includer of this partial symbol
     table, and this partial symbol table is a shared one.

     A shared psymtab is one that is referenced by multiple other
     psymtabs, and which conceptually has its contents directly
     included in those.

     Shared psymtabs have special semantics.  When a search finds a
     symbol in a shared table, we instead return one of the non-shared
     tables that include this one.

     A shared psymtabs can be referred to by other shared ones.

     The psymtabs that refer to a shared psymtab will list the shared
     psymtab in their 'dependencies' array.

     In DWARF terms, a shared psymtab is a DW_TAG_partial_unit; but
     of course using a name based on that would be too confusing, so
     "shared" was chosen instead.

     Only a single user is needed because, when expanding a shared
     psymtab, we only need to expand its "canonical" non-shared user.
     The choice of which one should be canonical is left to the
     debuginfo reader; it can be arbitrary.  */

  struct partial_symtab *user = nullptr;

  /* Array of pointers to all of the partial_symtab's which this one
     depends on.  Since this array can only be set to previous or
     the current (?) psymtab, this dependency tree is guaranteed not
     to have any loops.  "depends on" means that symbols must be read
     for the dependencies before being read for this psymtab; this is
     for type references in stabs, where if foo.c includes foo.h, declarations
     in foo.h may use type numbers defined in foo.c.  For other debugging
     formats there may be no need to use dependencies.  */

  struct partial_symtab **dependencies = nullptr;

  int number_of_dependencies = 0;

  /* Global symbol list.  This list will be sorted after readin to
     improve access.  Binary search will be the usual method of
     finding a symbol within it.  */

  std::vector<partial_symbol *> global_psymbols;

  /* Static symbol list.  This list will *not* be sorted after readin;
     to find a symbol in it, exhaustive search must be used.  This is
     reasonable because searches through this list will eventually
     lead to either the read in of a files symbols for real (assumed
     to take a *lot* of time; check) or an error (and we don't care
     how long errors take).  */

  std::vector<partial_symbol *> static_psymbols;

  /* True if the name of this partial symtab is not a source file name.  */

  bool anonymous = false;

  /* A flag that is temporarily used when searching psymtabs.  */

  ENUM_BITFIELD (psymtab_search_status) searched_flag : 2;

  /* Validity of the m_text_low and m_text_high fields.  */

  unsigned int text_low_valid : 1;
  unsigned int text_high_valid : 1;
};

/* A partial symtab that tracks the "readin" and "compunit_symtab"
   information in the ordinary way -- by storing it directly in this
   object.  */
struct standard_psymtab : public partial_symtab
{
  standard_psymtab (const char *filename,
		    psymtab_storage *partial_symtabs,
		    objfile_per_bfd_storage *objfile_per_bfd)
    : partial_symtab (filename, partial_symtabs, objfile_per_bfd)
  {
  }

  standard_psymtab (const char *filename,
		    psymtab_storage *partial_symtabs,
		    objfile_per_bfd_storage *objfile_per_bfd,
		    unrelocated_addr addr)
    : partial_symtab (filename, partial_symtabs, objfile_per_bfd, addr)
  {
  }

  bool readin_p (struct objfile *) const override
  {
    return readin;
  }

  struct compunit_symtab *get_compunit_symtab (struct objfile *) const override
  {
    return compunit_symtab;
  }

  /* True if the symtab corresponding to this psymtab has been
     readin.  */

  bool readin = false;

  /* Pointer to compunit eventually allocated for this source file, 0 if
     !readin or if we haven't looked for the symtab after it was readin.  */

  struct compunit_symtab *compunit_symtab = nullptr;
};

/* A partial_symtab that works in the historical db way.  This should
   not be used in new code, but exists to transition the somewhat
   unmaintained "legacy" debug formats.  */

struct legacy_psymtab : public standard_psymtab
{
  legacy_psymtab (const char *filename,
		  psymtab_storage *partial_symtabs,
		  objfile_per_bfd_storage *objfile_per_bfd)
    : standard_psymtab (filename, partial_symtabs, objfile_per_bfd)
  {
  }

  legacy_psymtab (const char *filename,
		  psymtab_storage *partial_symtabs,
		  objfile_per_bfd_storage *objfile_per_bfd,
		  unrelocated_addr addr)
    : standard_psymtab (filename, partial_symtabs, objfile_per_bfd, addr)
  {
  }

  void read_symtab (struct objfile *objf) override
  {
    if (legacy_read_symtab)
      (*legacy_read_symtab) (this, objf);
  }

  void expand_psymtab (struct objfile *objf) override
  {
    (*legacy_expand_psymtab) (this, objf);
  }

  /* Pointer to function which will read in the symtab corresponding to
     this psymtab.  */

  void (*legacy_read_symtab) (legacy_psymtab *, struct objfile *) = nullptr;

  /* Pointer to function which will actually expand this psymtab into
     a full symtab.  */

  void (*legacy_expand_psymtab) (legacy_psymtab *, struct objfile *) = nullptr;

  /* Information that lets read_symtab() locate the part of the symbol table
     that this psymtab corresponds to.  This information is private to the
     format-dependent symbol reading routines.  For further detail examine
     the various symbol reading modules.  */

  void *read_symtab_private = nullptr;
};

/* Used when recording partial symbol tables.  On destruction,
   discards any partial symbol tables that have been built.  However,
   the tables can be kept by calling the "keep" method.  */
class psymtab_discarder
{
 public:

  psymtab_discarder (psymtab_storage *partial_symtabs)
    : m_partial_symtabs (partial_symtabs),
      m_psymtab (partial_symtabs->psymtabs)
  {
  }

  ~psymtab_discarder ()
  {
    if (m_partial_symtabs != nullptr)
      m_partial_symtabs->discard_psymtabs_to (m_psymtab);
  }

  /* Keep any partial symbol tables that were built.  */
  void keep ()
  {
    m_partial_symtabs = nullptr;
  }

 private:

  /* The partial symbol storage object.  */
  psymtab_storage *m_partial_symtabs;
  /* How far back to free.  */
  struct partial_symtab *m_psymtab;
};

/* An implementation of quick_symbol_functions, specialized for
   partial symbols.  */
struct psymbol_functions : public quick_symbol_functions
{
  explicit psymbol_functions (const std::shared_ptr<psymtab_storage> &storage)
    : m_partial_symtabs (storage)
  {
  }

  psymbol_functions ()
    : m_partial_symtabs (new psymtab_storage)
  {
  }

  bool has_symbols (struct objfile *objfile) override;

  bool has_unexpanded_symtabs (struct objfile *objfile) override;

  struct symtab *find_last_source_symtab (struct objfile *objfile) override;

  void forget_cached_source_info (struct objfile *objfile) override;

  enum language lookup_global_symbol_language (struct objfile *objfile,
					       const char *name,
					       domain_enum domain,
					       bool *symbol_found_p) override;

  void print_stats (struct objfile *objfile, bool print_bcache) override;

  void dump (struct objfile *objfile) override;

  void expand_all_symtabs (struct objfile *objfile) override;

  bool expand_symtabs_matching
    (struct objfile *objfile,
     gdb::function_view<expand_symtabs_file_matcher_ftype> file_matcher,
     const lookup_name_info *lookup_name,
     gdb::function_view<expand_symtabs_symbol_matcher_ftype> symbol_matcher,
     gdb::function_view<expand_symtabs_exp_notify_ftype> expansion_notify,
     block_search_flags search_flags,
     domain_enum domain,
     enum search_domain kind) override;

  struct compunit_symtab *find_pc_sect_compunit_symtab
    (struct objfile *objfile, struct bound_minimal_symbol msymbol,
     CORE_ADDR pc, struct obj_section *section, int warn_if_readin) override;

  struct compunit_symtab *find_compunit_symtab_by_address
    (struct objfile *objfile, CORE_ADDR address) override
  {
    return nullptr;
  }

  void map_symbol_filenames (struct objfile *objfile,
			     gdb::function_view<symbol_filename_ftype> fun,
			     bool need_fullname) override;

  /* Return a range adapter for the psymtabs.  */
  psymtab_storage::partial_symtab_range partial_symbols
       (struct objfile *objfile);

  /* Return the partial symbol storage associated with this
     object.  */
  const std::shared_ptr<psymtab_storage> &get_partial_symtabs () const
  {
    return m_partial_symtabs;
  }

  /* Replace the partial symbol table storage in this object with
     SYMS.  */
  void set_partial_symtabs (const std::shared_ptr<psymtab_storage> &syms)
  {
    m_partial_symtabs = syms;
  }

  /* Find which partial symtab contains PC and SECTION.  Return NULL if
     none.  We return the psymtab that contains a symbol whose address
     exactly matches PC, or, if we cannot find an exact match, the
     psymtab that contains a symbol whose address is closest to PC.  */

  struct partial_symtab *find_pc_sect_psymtab
       (struct objfile *objfile,
	CORE_ADDR pc,
	struct obj_section *section,
	struct bound_minimal_symbol msymbol);

private:

  /* Count the number of partial symbols in *THIS.  */
  int count_psyms ();

  /* Storage for the partial symbols.  */
  std::shared_ptr<psymtab_storage> m_partial_symtabs;
};

#endif /* PSYMTAB_H */
