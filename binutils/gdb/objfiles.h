/* Definitions for symbol file management in GDB.

   Copyright (C) 1992-2024 Free Software Foundation, Inc.

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

#if !defined (OBJFILES_H)
#define OBJFILES_H

#include "hashtab.h"
#include "gdbsupport/gdb_obstack.h"
#include "objfile-flags.h"
#include "symfile.h"
#include "progspace.h"
#include "registry.h"
#include "gdb_bfd.h"
#include <atomic>
#include <bitset>
#include <vector>
#include "gdbsupport/next-iterator.h"
#include "gdbsupport/safe-iterator.h"
#include "bcache.h"
#include "gdbarch.h"
#include "gdbsupport/refcounted-object.h"
#include "jit.h"
#include "quick-symbol.h"
#include <forward_list>

struct htab;
struct objfile_data;
struct partial_symbol;

/* This structure maintains information on a per-objfile basis about the
   "entry point" of the objfile, and the scope within which the entry point
   exists.  It is possible that gdb will see more than one objfile that is
   executable, each with its own entry point.

   For example, for dynamically linked executables in SVR4, the dynamic linker
   code is contained within the shared C library, which is actually executable
   and is run by the kernel first when an exec is done of a user executable
   that is dynamically linked.  The dynamic linker within the shared C library
   then maps in the various program segments in the user executable and jumps
   to the user executable's recorded entry point, as if the call had been made
   directly by the kernel.

   The traditional gdb method of using this info was to use the
   recorded entry point to set the entry-file's lowpc and highpc from
   the debugging information, where these values are the starting
   address (inclusive) and ending address (exclusive) of the
   instruction space in the executable which correspond to the
   "startup file", i.e. crt0.o in most cases.  This file is assumed to
   be a startup file and frames with pc's inside it are treated as
   nonexistent.  Setting these variables is necessary so that
   backtraces do not fly off the bottom of the stack.

   NOTE: cagney/2003-09-09: It turns out that this "traditional"
   method doesn't work.  Corinna writes: ``It turns out that the call
   to test for "inside entry file" destroys a meaningful backtrace
   under some conditions.  E.g. the backtrace tests in the asm-source
   testcase are broken for some targets.  In this test the functions
   are all implemented as part of one file and the testcase is not
   necessarily linked with a start file (depending on the target).
   What happens is, that the first frame is printed normally and
   following frames are treated as being inside the entry file then.
   This way, only the #0 frame is printed in the backtrace output.''
   Ref "frame.c" "NOTE: vinschen/2003-04-01".

   Gdb also supports an alternate method to avoid running off the bottom
   of the stack.

   There are two frames that are "special", the frame for the function
   containing the process entry point, since it has no predecessor frame,
   and the frame for the function containing the user code entry point
   (the main() function), since all the predecessor frames are for the
   process startup code.  Since we have no guarantee that the linked
   in startup modules have any debugging information that gdb can use,
   we need to avoid following frame pointers back into frames that might
   have been built in the startup code, as we might get hopelessly
   confused.  However, we almost always have debugging information
   available for main().

   These variables are used to save the range of PC values which are
   valid within the main() function and within the function containing
   the process entry point.  If we always consider the frame for
   main() as the outermost frame when debugging user code, and the
   frame for the process entry point function as the outermost frame
   when debugging startup code, then all we have to do is have
   DEPRECATED_FRAME_CHAIN_VALID return false whenever a frame's
   current PC is within the range specified by these variables.  In
   essence, we set "ceilings" in the frame chain beyond which we will
   not proceed when following the frame chain back up the stack.

   A nice side effect is that we can still debug startup code without
   running off the end of the frame chain, assuming that we have usable
   debugging information in the startup modules, and if we choose to not
   use the block at main, or can't find it for some reason, everything
   still works as before.  And if we have no startup code debugging
   information but we do have usable information for main(), backtraces
   from user code don't go wandering off into the startup code.  */

struct entry_info
{
  /* The unrelocated value we should use for this objfile entry point.  */
  CORE_ADDR entry_point;

  /* The index of the section in which the entry point appears.  */
  int the_bfd_section_index;

  /* Set to 1 iff ENTRY_POINT contains a valid value.  */
  unsigned entry_point_p : 1;

  /* Set to 1 iff this object was initialized.  */
  unsigned initialized : 1;
};

#define SECT_OFF_DATA(objfile) \
     ((objfile->sect_index_data == -1) \
      ? (internal_error (_("sect_index_data not initialized")), -1)	\
      : objfile->sect_index_data)

#define SECT_OFF_RODATA(objfile) \
     ((objfile->sect_index_rodata == -1) \
      ? (internal_error (_("sect_index_rodata not initialized")), -1)	\
      : objfile->sect_index_rodata)

#define SECT_OFF_TEXT(objfile) \
     ((objfile->sect_index_text == -1) \
      ? (internal_error (_("sect_index_text not initialized")), -1)	\
      : objfile->sect_index_text)

/* Sometimes the .bss section is missing from the objfile, so we don't
   want to die here.  Let the users of SECT_OFF_BSS deal with an
   uninitialized section index.  */
#define SECT_OFF_BSS(objfile) (objfile)->sect_index_bss

/* The "objstats" structure provides a place for gdb to record some
   interesting information about its internal state at runtime, on a
   per objfile basis, such as information about the number of symbols
   read, size of string table (if any), etc.  */

struct objstats
{
  /* Number of full symbols read.  */
  int n_syms = 0;

  /* Number of ".stabs" read (if applicable).  */
  int n_stabs = 0;

  /* Number of types.  */
  int n_types = 0;

  /* Size of stringtable, (if applicable).  */
  int sz_strtab = 0;
};

#define OBJSTAT(objfile, expr) (objfile -> stats.expr)
#define OBJSTATS struct objstats stats
extern void print_objfile_statistics (void);

/* Number of entries in the minimal symbol hash table.  */
#define MINIMAL_SYMBOL_HASH_SIZE 2039

/* An iterator for minimal symbols.  */

struct minimal_symbol_iterator
{
  typedef minimal_symbol_iterator self_type;
  typedef struct minimal_symbol *value_type;
  typedef struct minimal_symbol *&reference;
  typedef struct minimal_symbol **pointer;
  typedef std::forward_iterator_tag iterator_category;
  typedef int difference_type;

  explicit minimal_symbol_iterator (struct minimal_symbol *msym)
    : m_msym (msym)
  {
  }

  value_type operator* () const
  {
    return m_msym;
  }

  bool operator== (const self_type &other) const
  {
    return m_msym == other.m_msym;
  }

  bool operator!= (const self_type &other) const
  {
    return m_msym != other.m_msym;
  }

  self_type &operator++ ()
  {
    ++m_msym;
    return *this;
  }

private:
  struct minimal_symbol *m_msym;
};

/* Some objfile data is hung off the BFD.  This enables sharing of the
   data across all objfiles using the BFD.  The data is stored in an
   instance of this structure, and associated with the BFD using the
   registry system.  */

struct objfile_per_bfd_storage
{
  objfile_per_bfd_storage (bfd *bfd)
    : minsyms_read (false), m_bfd (bfd)
  {}

  ~objfile_per_bfd_storage ();

  /* Intern STRING in this object's string cache and return the unique copy.
     The copy has the same lifetime as this object.

     STRING must be null-terminated.  */

  const char *intern (const char *str)
  {
    return (const char *) string_cache.insert (str, strlen (str) + 1);
  }

  /* Same as the above, but for an std::string.  */

  const char *intern (const std::string &str)
  {
    return (const char *) string_cache.insert (str.c_str (), str.size () + 1);
  }

  /* Get the BFD this object is associated to.  */

  bfd *get_bfd () const
  {
    return m_bfd;
  }

  /* The storage has an obstack of its own.  */

  auto_obstack storage_obstack;

  /* String cache.  */

  gdb::bcache string_cache;

  /* The gdbarch associated with the BFD.  Note that this gdbarch is
     determined solely from BFD information, without looking at target
     information.  The gdbarch determined from a running target may
     differ from this e.g. with respect to register types and names.  */

  struct gdbarch *gdbarch = NULL;

  /* Hash table for mapping symbol names to demangled names.  Each
     entry in the hash table is a demangled_name_entry struct, storing the
     language and two consecutive strings, both null-terminated; the first one
     is a mangled or linkage name, and the second is the demangled name or just
     a zero byte if the name doesn't demangle.  */

  htab_up demangled_names_hash;

  /* The per-objfile information about the entry point, the scope (file/func)
     containing the entry point, and the scope of the user's main() func.  */

  entry_info ei {};

  /* The name and language of any "main" found in this objfile.  The
     name can be NULL, which means that the information was not
     recorded.  */

  const char *name_of_main = NULL;
  enum language language_of_main = language_unknown;

  /* Each file contains a pointer to an array of minimal symbols for all
     global symbols that are defined within the file.  The array is
     terminated by a "null symbol", one that has a NULL pointer for the
     name and a zero value for the address.  This makes it easy to walk
     through the array when passed a pointer to somewhere in the middle
     of it.  There is also a count of the number of symbols, which does
     not include the terminating null symbol.  */

  gdb::unique_xmalloc_ptr<minimal_symbol> msymbols;
  int minimal_symbol_count = 0;

  /* The number of minimal symbols read, before any minimal symbol
     de-duplication is applied.  Note in particular that this has only
     a passing relationship with the actual size of the table above;
     use minimal_symbol_count if you need the true size.  */

  int n_minsyms = 0;

  /* This is true if minimal symbols have already been read.  Symbol
     readers can use this to bypass minimal symbol reading.  Also, the
     minimal symbol table management code in minsyms.c uses this to
     suppress new minimal symbols.  You might think that MSYMBOLS or
     MINIMAL_SYMBOL_COUNT could be used for this, but it is possible
     for multiple readers to install minimal symbols into a given
     per-BFD.  */

  bool minsyms_read : 1;

  /* This is a hash table used to index the minimal symbols by (mangled)
     name.  */

  minimal_symbol *msymbol_hash[MINIMAL_SYMBOL_HASH_SIZE] {};

  /* This hash table is used to index the minimal symbols by their
     demangled names.  Uses a language-specific hash function via
     search_name_hash.  */

  minimal_symbol *msymbol_demangled_hash[MINIMAL_SYMBOL_HASH_SIZE] {};

  /* All the different languages of symbols found in the demangled
     hash table.  */
  std::bitset<nr_languages> demangled_hash_languages;

private:
  /* The BFD this object is associated to.  */

  bfd *m_bfd;
};

/* An iterator that first returns a parent objfile, and then each
   separate debug objfile.  */

class separate_debug_iterator
{
public:

  explicit separate_debug_iterator (struct objfile *objfile)
    : m_objfile (objfile),
      m_parent (objfile)
  {
  }

  bool operator!= (const separate_debug_iterator &other)
  {
    return m_objfile != other.m_objfile;
  }

  separate_debug_iterator &operator++ ();

  struct objfile *operator* ()
  {
    return m_objfile;
  }

private:

  struct objfile *m_objfile;
  struct objfile *m_parent;
};

/* A range adapter wrapping separate_debug_iterator.  */

typedef iterator_range<separate_debug_iterator> separate_debug_range;

/* Sections in an objfile.  The section offsets are stored in the
   OBJFILE.  */

struct obj_section
{
  /* Relocation offset applied to the section.  */
  CORE_ADDR offset () const;

  /* Set the relocation offset applied to the section.  */
  void set_offset (CORE_ADDR offset);

  /* The memory address of the section (vma + offset).  */
  CORE_ADDR addr () const
  {
    return bfd_section_vma (this->the_bfd_section) + this->offset ();
  }

  /* The one-passed-the-end memory address of the section
     (vma + size + offset).  */
  CORE_ADDR endaddr () const
  {
    return this->addr () + bfd_section_size (this->the_bfd_section);
  }

  /* BFD section pointer */
  struct bfd_section *the_bfd_section;

  /* Objfile this section is part of.  */
  struct objfile *objfile;

  /* True if this "overlay section" is mapped into an "overlay region".  */
  int ovly_mapped;
};

/* Master structure for keeping track of each file from which
   gdb reads symbols.  There are several ways these get allocated: 1.
   The main symbol file, symfile_objfile, set by the symbol-file command,
   2.  Additional symbol files added by the add-symbol-file command,
   3.  Shared library objfiles, added by ADD_SOLIB,  4.  symbol files
   for modules that were loaded when GDB attached to a remote system
   (see remote-vx.c).

   GDB typically reads symbols twice -- first an initial scan which just
   reads "partial symbols"; these are partial information for the
   static/global symbols in a symbol file.  When later looking up
   symbols, lookup_symbol is used to check if we only have a partial
   symbol and if so, read and expand the full compunit.  */

struct objfile
{
private:

  /* The only way to create an objfile is to call objfile::make.  */
  objfile (gdb_bfd_ref_ptr, const char *, objfile_flags);

public:

  /* Normally you should not call delete.  Instead, call 'unlink' to
     remove it from the program space's list.  In some cases, you may
     need to hold a reference to an objfile that is independent of its
     existence on the program space's list; for this case, the
     destructor must be public so that unique_ptr can reference
     it.  */
  ~objfile ();

  /* Create an objfile.  */
  static objfile *make (gdb_bfd_ref_ptr bfd_, const char *name_,
			objfile_flags flags_, objfile *parent = nullptr);

  /* Remove an objfile from the current program space, and free
     it.  */
  void unlink ();

  DISABLE_COPY_AND_ASSIGN (objfile);

  /* A range adapter that makes it possible to iterate over all
     compunits in one objfile.  */

  compunit_symtab_range compunits ()
  {
    return compunit_symtab_range (compunit_symtabs);
  }

  /* A range adapter that makes it possible to iterate over all
     minimal symbols of an objfile.  */

  typedef iterator_range<minimal_symbol_iterator> msymbols_range;

  /* Return a range adapter for iterating over all minimal
     symbols.  */

  msymbols_range msymbols ()
  {
    auto start = minimal_symbol_iterator (per_bfd->msymbols.get ());
    auto end = minimal_symbol_iterator (per_bfd->msymbols.get ()
					+ per_bfd->minimal_symbol_count);
    return msymbols_range (start, end);
  }

  /* Return a range adapter for iterating over all the separate debug
     objfiles of this objfile.  */

  separate_debug_range separate_debug_objfiles ()
  {
    auto start = separate_debug_iterator (this);
    auto end = separate_debug_iterator (nullptr);
    return separate_debug_range (start, end);
  }

  CORE_ADDR text_section_offset () const
  {
    return section_offsets[SECT_OFF_TEXT (this)];
  }

  CORE_ADDR data_section_offset () const
  {
    return section_offsets[SECT_OFF_DATA (this)];
  }

  /* Intern STRING and return the unique copy.  The copy has the same
     lifetime as the per-BFD object.  */
  const char *intern (const char *str)
  {
    return per_bfd->intern (str);
  }

  /* Intern STRING and return the unique copy.  The copy has the same
     lifetime as the per-BFD object.  */
  const char *intern (const std::string &str)
  {
    return per_bfd->intern (str);
  }

  /* Retrieve the gdbarch associated with this objfile.  */
  struct gdbarch *arch () const
  {
    return per_bfd->gdbarch;
  }

  /* Return true if OBJFILE has partial symbols.  */

  bool has_partial_symbols ();

  /* Look for a separate debug symbol file for this objfile, make use of
     build-id, debug-link, and debuginfod as necessary.  If a suitable
     separate debug symbol file is found then it is loaded using a call to
     symbol_file_add_separate (SYMFILE_FLAGS is passed through unmodified
     to this call) and this function returns true.  If no suitable separate
     debug symbol file is found and loaded then this function returns
     false.  */

  bool find_and_add_separate_symbol_file (symfile_add_flags symfile_flags);

  /* Return true if this objfile has any unexpanded symbols.  A return
     value of false indicates either, that this objfile has all its
     symbols fully expanded (i.e. fully read in), or that this objfile has
     no symbols at all (i.e. no debug information).  */
  bool has_unexpanded_symtabs ();

  /* See quick_symbol_functions.  */
  struct symtab *find_last_source_symtab ();

  /* See quick_symbol_functions.  */
  void forget_cached_source_info ();

  /* Expand and iterate over each "partial" symbol table in OBJFILE
     where the source file is named NAME.

     If NAME is not absolute, a match after a '/' in the symbol table's
     file name will also work, REAL_PATH is NULL then.  If NAME is
     absolute then REAL_PATH is non-NULL absolute file name as resolved
     via gdb_realpath from NAME.

     If a match is found, the "partial" symbol table is expanded.
     Then, this calls iterate_over_some_symtabs (or equivalent) over
     all newly-created symbol tables, passing CALLBACK to it.
     The result of this call is returned.  */
  bool map_symtabs_matching_filename
    (const char *name, const char *real_path,
     gdb::function_view<bool (symtab *)> callback);

  /* Check to see if the symbol is defined in a "partial" symbol table
     of this objfile.  BLOCK_INDEX should be either GLOBAL_BLOCK or
     STATIC_BLOCK, depending on whether we want to search global
     symbols or static symbols.  NAME is the name of the symbol to
     look for.  DOMAIN indicates what sort of symbol to search for.

     Returns the newly-expanded compunit in which the symbol is
     defined, or NULL if no such symbol table exists.  If OBJFILE
     contains !TYPE_OPAQUE symbol prefer its compunit.  If it contains
     only TYPE_OPAQUE symbol(s), return at least that compunit.  */
  struct compunit_symtab *lookup_symbol (block_enum kind, const char *name,
					 domain_enum domain);

  /* See quick_symbol_functions.  */
  void print_stats (bool print_bcache);

  /* See quick_symbol_functions.  */
  void dump ();

  /* Find all the symbols in OBJFILE named FUNC_NAME, and ensure that
     the corresponding symbol tables are loaded.  */
  void expand_symtabs_for_function (const char *func_name);

  /* See quick_symbol_functions.  */
  void expand_all_symtabs ();

  /* Read all symbol tables associated with OBJFILE which have
     symtab_to_fullname equal to FULLNAME.
     This is for the purposes of examining code only, e.g., expand_line_sal.
     The routine may ignore debug info that is known to not be useful with
     code, e.g., DW_TAG_type_unit for dwarf debug info.  */
  void expand_symtabs_with_fullname (const char *fullname);

  /* See quick_symbol_functions.  */
  bool expand_symtabs_matching
    (gdb::function_view<expand_symtabs_file_matcher_ftype> file_matcher,
     const lookup_name_info *lookup_name,
     gdb::function_view<expand_symtabs_symbol_matcher_ftype> symbol_matcher,
     gdb::function_view<expand_symtabs_exp_notify_ftype> expansion_notify,
     block_search_flags search_flags,
     domain_enum domain,
     enum search_domain kind);

  /* See quick_symbol_functions.  */
  struct compunit_symtab *find_pc_sect_compunit_symtab
    (struct bound_minimal_symbol msymbol,
     CORE_ADDR pc,
     struct obj_section *section,
     int warn_if_readin);

  /* See quick_symbol_functions.  */
  void map_symbol_filenames (gdb::function_view<symbol_filename_ftype> fun,
			     bool need_fullname);

  /* See quick_symbol_functions.  */
  void compute_main_name ();

  /* See quick_symbol_functions.  */
  struct compunit_symtab *find_compunit_symtab_by_address (CORE_ADDR address);

  /* See quick_symbol_functions.  */
  enum language lookup_global_symbol_language (const char *name,
					       domain_enum domain,
					       bool *symbol_found_p);

  /* Return the relocation offset applied to SECTION.  */
  CORE_ADDR section_offset (bfd_section *section) const
  {
    /* The section's owner can be nullptr if it is one of the _bfd_std_section
       section.  */
    gdb_assert (section->owner == nullptr || section->owner == this->obfd);

    int idx = gdb_bfd_section_index (this->obfd.get (), section);
    return this->section_offsets[idx];
  }

  /* Set the relocation offset applied to SECTION.  */
  void set_section_offset (bfd_section *section, CORE_ADDR offset)
  {
    /* The section's owner can be nullptr if it is one of the _bfd_std_section
       section.  */
    gdb_assert (section->owner == nullptr || section->owner == this->obfd);

    int idx = gdb_bfd_section_index (this->obfd.get (), section);
    this->section_offsets[idx] = offset;
  }

  class section_iterator
  {
  public:
    section_iterator (const section_iterator &) = default;
    section_iterator (section_iterator &&) = default;
    section_iterator &operator= (const section_iterator &) = default;
    section_iterator &operator= (section_iterator &&) = default;

    typedef section_iterator self_type;
    typedef obj_section *value_type;

    value_type operator* ()
    { return m_iter; }

    section_iterator &operator++ ()
    {
      ++m_iter;
      skip_null ();
      return *this;
    }

    bool operator== (const section_iterator &other) const
    { return m_iter == other.m_iter && m_end == other.m_end; }

    bool operator!= (const section_iterator &other) const
    { return !(*this == other); }

  private:

    friend class objfile;

    section_iterator (obj_section *iter, obj_section *end)
      : m_iter (iter),
	m_end (end)
    {
      skip_null ();
    }

    void skip_null ()
    {
      while (m_iter < m_end && m_iter->the_bfd_section == nullptr)
	++m_iter;
    }

    value_type m_iter;
    value_type m_end;
  };

  iterator_range<section_iterator> sections ()
  {
    return (iterator_range<section_iterator>
	    (section_iterator (sections_start, sections_end),
	     section_iterator (sections_end, sections_end)));
  }

  iterator_range<section_iterator> sections () const
  {
    return (iterator_range<section_iterator>
	    (section_iterator (sections_start, sections_end),
	     section_iterator (sections_end, sections_end)));
  }

public:

  /* The object file's original name as specified by the user,
     made absolute, and tilde-expanded.  However, it is not canonicalized
     (i.e., it has not been passed through gdb_realpath).
     This pointer is never NULL.  This does not have to be freed; it is
     guaranteed to have a lifetime at least as long as the objfile.  */

  const char *original_name = nullptr;

  CORE_ADDR addr_low = 0;

  /* Some flag bits for this objfile.  */

  objfile_flags flags;

  /* The program space associated with this objfile.  */

  struct program_space *pspace;

  /* List of compunits.
     These are used to do symbol lookups and file/line-number lookups.  */

  struct compunit_symtab *compunit_symtabs = nullptr;

  /* The object file's BFD.  Can be null if the objfile contains only
     minimal symbols (e.g. the run time common symbols for SunOS4) or
     if the objfile is a dynamic objfile (e.g. created by JIT reader
     API).  */

  gdb_bfd_ref_ptr obfd;

  /* The per-BFD data.  */

  struct objfile_per_bfd_storage *per_bfd = nullptr;

  /* In some cases, the per_bfd object is owned by this objfile and
     not by the BFD itself.  In this situation, this holds the owning
     pointer.  */

  std::unique_ptr<objfile_per_bfd_storage> per_bfd_storage;

  /* The modification timestamp of the object file, as of the last time
     we read its symbols.  */

  long mtime = 0;

  /* Obstack to hold objects that should be freed when we load a new symbol
     table from this object file.  */

  auto_obstack objfile_obstack;

  /* Structure which keeps track of functions that manipulate objfile's
     of the same type as this objfile.  I.e. the function to read partial
     symbols for example.  Note that this structure is in statically
     allocated memory, and is shared by all objfiles that use the
     object module reader of this type.  */

  const struct sym_fns *sf = nullptr;

  /* The "quick" (aka partial) symbol functions for this symbol
     reader.  */
  std::forward_list<quick_symbol_functions_up> qf;

  /* Per objfile data-pointers required by other GDB modules.  */

  registry<objfile> registry_fields;

  /* Set of relocation offsets to apply to each section.
     The table is indexed by the_bfd_section->index, thus it is generally
     as large as the number of sections in the binary.

     These offsets indicate that all symbols (including partial and
     minimal symbols) which have been read have been relocated by this
     much.  Symbols which are yet to be read need to be relocated by it.  */

  ::section_offsets section_offsets;

  /* Indexes in the section_offsets array.  These are initialized by the
     *_symfile_offsets() family of functions (som_symfile_offsets,
     xcoff_symfile_offsets, default_symfile_offsets).  In theory they
     should correspond to the section indexes used by bfd for the
     current objfile.  The exception to this for the time being is the
     SOM version.

     These are initialized to -1 so that we can later detect if they
     are used w/o being properly assigned to.  */

  int sect_index_text = -1;
  int sect_index_data = -1;
  int sect_index_bss = -1;
  int sect_index_rodata = -1;

  /* These pointers are used to locate the section table, which among
     other things, is used to map pc addresses into sections.
     SECTIONS_START points to the first entry in the table, and
     SECTIONS_END points to the first location past the last entry in
     the table.  The table is stored on the objfile_obstack.  The
     sections are indexed by the BFD section index; but the structure
     data is only valid for certain sections (e.g. non-empty,
     SEC_ALLOC).  */

  struct obj_section *sections_start = nullptr;
  struct obj_section *sections_end = nullptr;

  /* GDB allows to have debug symbols in separate object files.  This is
     used by .gnu_debuglink, ELF build id note and Mach-O OSO.
     Although this is a tree structure, GDB only support one level
     (ie a separate debug for a separate debug is not supported).  Note that
     separate debug object are in the main chain and therefore will be
     visited by objfiles & co iterators.  Separate debug objfile always
     has a non-nul separate_debug_objfile_backlink.  */

  /* Link to the first separate debug object, if any.  */

  struct objfile *separate_debug_objfile = nullptr;

  /* If this is a separate debug object, this is used as a link to the
     actual executable objfile.  */

  struct objfile *separate_debug_objfile_backlink = nullptr;

  /* If this is a separate debug object, this is a link to the next one
     for the same executable objfile.  */

  struct objfile *separate_debug_objfile_link = nullptr;

  /* Place to stash various statistics about this objfile.  */

  OBJSTATS;

  /* A linked list of symbols created when reading template types or
     function templates.  These symbols are not stored in any symbol
     table, so we have to keep them here to relocate them
     properly.  */

  struct symbol *template_symbols = nullptr;

  /* Associate a static link (struct dynamic_prop *) to all blocks (struct
     block *) that have one.

     In the context of nested functions (available in Pascal, Ada and GNU C,
     for instance), a static link (as in DWARF's DW_AT_static_link attribute)
     for a function is a way to get the frame corresponding to the enclosing
     function.

     Very few blocks have a static link, so it's more memory efficient to
     store these here rather than in struct block.  Static links must be
     allocated on the objfile's obstack.  */
  htab_up static_links;

  /* JIT-related data for this objfile, if the objfile is a JITer;
     that is, it produces JITed objfiles.  */
  std::unique_ptr<jiter_objfile_data> jiter_data = nullptr;

  /* JIT-related data for this objfile, if the objfile is JITed;
     that is, it was produced by a JITer.  */
  std::unique_ptr<jited_objfile_data> jited_data = nullptr;

  /* A flag that is set to true if the JIT interface symbols are not
     found in this objfile, so that we can skip the symbol lookup the
     next time.  If an objfile does not have the symbols, it will
     never have them.  */
  bool skip_jit_symbol_lookup = false;

  /* Flag which indicates, when true, that the object format
     potentially supports copy relocations.  ABIs for some
     architectures that use ELF have a copy relocation in which the
     initialization for a global variable defined in a shared object
     will be copied to memory allocated to the main program during
     dynamic linking.  Therefore this flag will be set for ELF
     objfiles.  Other object formats that use the same copy relocation
     mechanism as ELF should set this flag too.  This flag is used in
     conjunction with the minimal_symbol::maybe_copied method.  */
  bool object_format_has_copy_relocs = false;
};

/* A deleter for objfile.  */

struct objfile_deleter
{
  void operator() (objfile *ptr) const
  {
    ptr->unlink ();
  }
};

/* A unique pointer that holds an objfile.  */

typedef std::unique_ptr<objfile, objfile_deleter> objfile_up;

/* Relocation offset applied to the section.  */
inline CORE_ADDR
obj_section::offset () const
{
  return this->objfile->section_offset (this->the_bfd_section);
}

/* Set the relocation offset applied to the section.  */
inline void
obj_section::set_offset (CORE_ADDR offset)
{
  this->objfile->set_section_offset (this->the_bfd_section, offset);
}

/* Declarations for functions defined in objfiles.c */

extern int entry_point_address_query (CORE_ADDR *entry_p);

extern CORE_ADDR entry_point_address (void);

extern void build_objfile_section_table (struct objfile *);

extern void free_objfile_separate_debug (struct objfile *);

extern void objfile_relocate (struct objfile *, const section_offsets &);
extern void objfile_rebase (struct objfile *, CORE_ADDR);

extern int objfile_has_full_symbols (struct objfile *objfile);

extern int objfile_has_symbols (struct objfile *objfile);

extern int have_partial_symbols (void);

extern int have_full_symbols (void);

extern void objfile_set_sym_fns (struct objfile *objfile,
				 const struct sym_fns *sf);

extern void objfiles_changed (void);

/* Return true if ADDR maps into one of the sections of OBJFILE and false
   otherwise.  */

extern bool is_addr_in_objfile (CORE_ADDR addr, const struct objfile *objfile);

/* Return true if ADDRESS maps into one of the sections of a
   OBJF_SHARED objfile of PSPACE and false otherwise.  */

extern bool shared_objfile_contains_address_p (struct program_space *pspace,
					       CORE_ADDR address);

/* This operation deletes all objfile entries that represent solibs that
   weren't explicitly loaded by the user, via e.g., the add-symbol-file
   command.  */

extern void objfile_purge_solibs (void);

/* Functions for dealing with the minimal symbol table, really a misc
   address<->symbol mapping for things we don't have debug symbols for.  */

extern int have_minimal_symbols (void);

extern struct obj_section *find_pc_section (CORE_ADDR pc);

/* Return true if PC is in a section called NAME.  */
extern bool pc_in_section (CORE_ADDR, const char *);

/* Return non-zero if PC is in a SVR4-style procedure linkage table
   section.  */

static inline int
in_plt_section (CORE_ADDR pc)
{
  return (pc_in_section (pc, ".plt")
	  || pc_in_section (pc, ".plt.sec"));
}

/* In normal use, the section map will be rebuilt by find_pc_section
   if objfiles have been added, removed or relocated since it was last
   called.  Calling inhibit_section_map_updates will inhibit this
   behavior until the returned scoped_restore object is destroyed.  If
   you call inhibit_section_map_updates you must ensure that every
   call to find_pc_section in the inhibited region relates to a
   section that is already in the section map and has not since been
   removed or relocated.  */
extern scoped_restore_tmpl<int> inhibit_section_map_updates
    (struct program_space *pspace);

extern void default_iterate_over_objfiles_in_search_order
  (gdbarch *gdbarch, iterate_over_objfiles_in_search_order_cb_ftype cb,
   objfile *current_objfile);

/* Reset the per-BFD storage area on OBJ.  */

void set_objfile_per_bfd (struct objfile *obj);

/* Return canonical name for OBJFILE.
   This is the real file name if the file has been opened.
   Otherwise it is the original name supplied by the user.  */

const char *objfile_name (const struct objfile *objfile);

/* Return the (real) file name of OBJFILE if the file has been opened,
   otherwise return NULL.  */

const char *objfile_filename (const struct objfile *objfile);

/* Return the name to print for OBJFILE in debugging messages.  */

extern const char *objfile_debug_name (const struct objfile *objfile);

/* Return the name of the file format of OBJFILE if the file has been opened,
   otherwise return NULL.  */

const char *objfile_flavour_name (struct objfile *objfile);

/* Set the objfile's notion of the "main" name and language.  */

extern void set_objfile_main_name (struct objfile *objfile,
				   const char *name, enum language lang);

/* Find an integer type SIZE_IN_BYTES bytes in size from OF and return it.
   UNSIGNED_P controls if the integer is unsigned or not.  */
extern struct type *objfile_int_type (struct objfile *of, int size_in_bytes,
				      bool unsigned_p);

extern void objfile_register_static_link
  (struct objfile *objfile,
   const struct block *block,
   const struct dynamic_prop *static_link);

extern const struct dynamic_prop *objfile_lookup_static_link
  (struct objfile *objfile, const struct block *block);

#endif /* !defined (OBJFILES_H) */
