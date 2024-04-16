/* Definitions for reading symbol files into GDB.

   Copyright (C) 1990-2024 Free Software Foundation, Inc.

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

#if !defined (SYMFILE_H)
#define SYMFILE_H

/* This file requires that you first include "bfd.h".  */
#include "symtab.h"
#include "probe.h"
#include "symfile-add-flags.h"
#include "objfile-flags.h"
#include "gdb_bfd.h"
#include "gdbsupport/function-view.h"
#include "target-section.h"
#include "quick-symbol.h"

/* Opaque declarations.  */
struct target_section;
struct objfile;
struct obj_section;
struct obstack;
struct block;
struct value;
class frame_info_ptr;
struct agent_expr;
struct axs_value;
class probe;

struct other_sections
{
  other_sections (CORE_ADDR addr_, std::string &&name_, int sectindex_)
    : addr (addr_),
      name (std::move (name_)),
      sectindex (sectindex_)
  {
  }

  other_sections (other_sections &&other) = default;

  DISABLE_COPY_AND_ASSIGN (other_sections);

  CORE_ADDR addr;
  std::string name;

  /* SECTINDEX must be valid for associated BFD or set to -1.
     See syms_from_objfile_1 for an exception to this rule.
   */
  int sectindex;
};

/* Define an array of addresses to accommodate non-contiguous dynamic
   loading of modules.  This is for use when entering commands, so we
   can keep track of the section names until we read the file and can
   map them to bfd sections.  This structure is also used by solib.c
   to communicate the section addresses in shared objects to
   symbol_file_add ().  */

typedef std::vector<other_sections> section_addr_info;

/* A table listing the load segments in a symfile, and which segment
   each BFD section belongs to.  */
struct symfile_segment_data
{
  struct segment
  {
    segment (CORE_ADDR base, CORE_ADDR size)
      : base (base), size (size)
    {}

    /* The original base address the segment.  */
    CORE_ADDR base;

    /* The memory size of the segment.  */
    CORE_ADDR size;
  };

  /* The segments present in this file.  If there are
     two, the text segment is the first one and the data segment
     is the second one.  */
  std::vector<segment> segments;

  /* This is an array of entries recording which segment contains each BFD
     section.  SEGMENT_INFO[I] is S+1 if the I'th BFD section belongs to segment
     S, or zero if it is not in any segment.  */
  std::vector<int> segment_info;
};

using symfile_segment_data_up = std::unique_ptr<symfile_segment_data>;

/* Structure of functions used for probe support.  If one of these functions
   is provided, all must be.  */

struct sym_probe_fns
{
  /* If non-NULL, return a reference to vector of probe objects.  */
  const std::vector<std::unique_ptr<probe>> &(*sym_get_probes)
    (struct objfile *);
};

/* Structure to keep track of symbol reading functions for various
   object file types.  */

struct sym_fns
{
  /* Initializes anything that is global to the entire symbol table.
     It is called during symbol_file_add, when we begin debugging an
     entirely new program.  */

  void (*sym_new_init) (struct objfile *);

  /* Reads any initial information from a symbol file, and initializes
     the struct sym_fns SF in preparation for sym_read().  It is
     called every time we read a symbol file for any reason.  */

  void (*sym_init) (struct objfile *);

  /* sym_read (objfile, symfile_flags) Reads a symbol file into a psymtab
     (or possibly a symtab).  OBJFILE is the objfile struct for the
     file we are reading.  SYMFILE_FLAGS are the flags passed to
     symbol_file_add & co.  */

  void (*sym_read) (struct objfile *, symfile_add_flags);

  /* Called when we are finished with an objfile.  Should do all
     cleanup that is specific to the object file format for the
     particular objfile.  */

  void (*sym_finish) (struct objfile *);


  /* This function produces a file-dependent section_offsets
     structure, allocated in the objfile's storage.

     The section_addr_info structure contains the offset of loadable and
     allocated sections, relative to the absolute offsets found in the BFD.  */

  void (*sym_offsets) (struct objfile *, const section_addr_info &);

  /* This function produces a format-independent description of
     the segments of ABFD.  Each segment is a unit of the file
     which may be relocated independently.  */

  symfile_segment_data_up (*sym_segments) (bfd *abfd);

  /* This function should read the linetable from the objfile when
     the line table cannot be read while processing the debugging
     information.  */

  void (*sym_read_linetable) (struct objfile *);

  /* Relocate the contents of a debug section SECTP.  The
     contents are stored in BUF if it is non-NULL, or returned in a
     malloc'd buffer otherwise.  */

  bfd_byte *(*sym_relocate) (struct objfile *, asection *sectp, bfd_byte *buf);

  /* If non-NULL, this objfile has probe support, and all the probe
     functions referred to here will be non-NULL.  */
  const struct sym_probe_fns *sym_probe_fns;
};

extern section_addr_info
  build_section_addr_info_from_objfile (const struct objfile *objfile);

extern void relative_addr_info_to_section_offsets
  (section_offsets &section_offsets, const section_addr_info &addrs);

extern void addr_info_make_relative (section_addr_info *addrs,
				     bfd *abfd);

/* The default version of sym_fns.sym_offsets for readers that don't
   do anything special.  */

extern void default_symfile_offsets (struct objfile *objfile,
				     const section_addr_info &);

/* The default version of sym_fns.sym_segments for readers that don't
   do anything special.  */

extern symfile_segment_data_up default_symfile_segments (bfd *abfd);

/* The default version of sym_fns.sym_relocate for readers that don't
   do anything special.  */

extern bfd_byte *default_symfile_relocate (struct objfile *objfile,
					   asection *sectp, bfd_byte *buf);

extern struct symtab *allocate_symtab
  (struct compunit_symtab *cust, const char *filename, const char *id)
  ATTRIBUTE_NONNULL (1);

/* Same as the above, but passes FILENAME for ID.  */

static inline struct symtab *
allocate_symtab (struct compunit_symtab *cust, const char *filename)
{
  return allocate_symtab (cust, filename, filename);
}

extern struct compunit_symtab *allocate_compunit_symtab (struct objfile *,
							 const char *)
  ATTRIBUTE_NONNULL (1);

extern void add_compunit_symtab_to_objfile (struct compunit_symtab *cu);

extern void add_symtab_fns (enum bfd_flavour flavour, const struct sym_fns *);

extern void clear_symtab_users (symfile_add_flags add_flags);

extern enum language deduce_language_from_filename (const char *);

/* Map the filename extension EXT to the language LANG.  Any previous
   association of EXT will be removed.  EXT will be copied by this
   function.  */
extern void add_filename_language (const char *ext, enum language lang);

extern struct objfile *symbol_file_add (const char *, symfile_add_flags,
					section_addr_info *, objfile_flags);

extern struct objfile *symbol_file_add_from_bfd (const gdb_bfd_ref_ptr &,
						 const char *, symfile_add_flags,
						 section_addr_info *,
						 objfile_flags, struct objfile *parent);

extern void symbol_file_add_separate (const gdb_bfd_ref_ptr &, const char *,
				      symfile_add_flags, struct objfile *);

/* Find separate debuginfo for OBJFILE (using .gnu_debuglink section).
   Returns pathname, or an empty string.

   Any warnings generated as part of this lookup are added to WARNINGS.  If
   some other mechanism can be used to lookup the debug information then
   the warning will not be shown, however, if GDB fails to find suitable
   debug information using any approach, then any warnings will be
   printed.  */

extern std::string find_separate_debug_file_by_debuglink
  (struct objfile *objfile, deferred_warnings *warnings);

/* Build (allocate and populate) a section_addr_info struct from an
   existing section table.  */

extern section_addr_info
    build_section_addr_info_from_section_table (const std::vector<target_section> &table);

			/*   Variables   */

/* If true, shared library symbols will be added automatically
   when the inferior is created, new libraries are loaded, or when
   attaching to the inferior.  This is almost always what users will
   want to have happen; but for very large programs, the startup time
   will be excessive, and so if this is a problem, the user can clear
   this flag and then add the shared library symbols as needed.  Note
   that there is a potential for confusion, since if the shared
   library symbols are not loaded, commands like "info fun" will *not*
   report all the functions that are actually present.  */

extern bool auto_solib_add;

/* From symfile.c */

extern void set_initial_language (void);

extern gdb_bfd_ref_ptr symfile_bfd_open (const char *);

/* Like symfile_bfd_open, but will not throw an exception on error.
   Instead, it issues a warning and returns nullptr.  */

extern gdb_bfd_ref_ptr symfile_bfd_open_no_error (const char *) noexcept;

extern int get_section_index (struct objfile *, const char *);

extern int print_symbol_loading_p (int from_tty, int mainline, int full);

/* Utility functions for overlay sections: */
extern enum overlay_debugging_state
{
  ovly_off,
  ovly_on,
  ovly_auto
} overlay_debugging;
extern int overlay_cache_invalid;

/* Return the "mapped" overlay section containing the PC.  */
extern struct obj_section *find_pc_mapped_section (CORE_ADDR);

/* Return any overlay section containing the PC (even in its LMA
   region).  */
extern struct obj_section *find_pc_overlay (CORE_ADDR);

/* Return true if the section is an overlay.  */
extern int section_is_overlay (struct obj_section *);

/* Return true if the overlay section is currently "mapped".  */
extern int section_is_mapped (struct obj_section *);

/* Return true if pc belongs to section's VMA.  */
extern bool pc_in_mapped_range (CORE_ADDR, struct obj_section *);

/* Return true if pc belongs to section's LMA.  */
extern bool pc_in_unmapped_range (CORE_ADDR, struct obj_section *);

/* Map an address from a section's LMA to its VMA.  */
extern CORE_ADDR overlay_mapped_address (CORE_ADDR, struct obj_section *);

/* Map an address from a section's VMA to its LMA.  */
extern CORE_ADDR overlay_unmapped_address (CORE_ADDR, struct obj_section *);

/* Convert an address in an overlay section (force into VMA range).  */
extern CORE_ADDR symbol_overlayed_address (CORE_ADDR, struct obj_section *);

/* Load symbols from a file.  */
extern void symbol_file_add_main (const char *args,
				  symfile_add_flags add_flags);

/* Clear GDB symbol tables.  */
extern void symbol_file_clear (int from_tty);

/* Default overlay update function.  */
extern void simple_overlay_update (struct obj_section *);

extern bfd_byte *symfile_relocate_debug_section (struct objfile *, asection *,
						 bfd_byte *);

extern int symfile_map_offsets_to_segments (bfd *,
					    const struct symfile_segment_data *,
					    section_offsets &,
					    int, const CORE_ADDR *);
symfile_segment_data_up get_symfile_segment_data (bfd *abfd);

extern scoped_restore_tmpl<int> increment_reading_symtab (void);

bool expand_symtabs_matching
  (gdb::function_view<expand_symtabs_file_matcher_ftype> file_matcher,
   const lookup_name_info &lookup_name,
   gdb::function_view<expand_symtabs_symbol_matcher_ftype> symbol_matcher,
   gdb::function_view<expand_symtabs_exp_notify_ftype> expansion_notify,
   block_search_flags search_flags,
   enum search_domain kind);

void map_symbol_filenames (gdb::function_view<symbol_filename_ftype> fun,
			   bool need_fullname);

/* Target-agnostic function to load the sections of an executable into memory.

   ARGS should be in the form "EXECUTABLE [OFFSET]", where OFFSET is an
   optional offset to apply to each section.  */
extern void generic_load (const char *args, int from_tty);

/* From minidebug.c.  */

extern gdb_bfd_ref_ptr find_separate_debug_file_in_section (struct objfile *);

/* True if we are printing debug output about separate debug info files.  */

extern bool separate_debug_file_debug;

/* Read full symbols immediately.  */

extern int readnow_symbol_files;

/* Never read full symbols.  */

extern int readnever_symbol_files;

#endif /* !defined(SYMFILE_H) */
