/* DWARF 2 debugging format support for GDB.

   Copyright (C) 1994-2024 Free Software Foundation, Inc.

   Adapted by Gary Funck (gary@intrepid.com), Intrepid Technology,
   Inc.  with support from Florida State University (under contract
   with the Ada Joint Program Office), and Silicon Graphics, Inc.
   Initial contribution by Brent Benson, Harris Computer Systems, Inc.,
   based on Fred Fish's (Cygnus Support) implementation of DWARF 1
   support.

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

/* FIXME: Various die-reading functions need to be more careful with
   reading off the end of the section.
   E.g., load_partial_dies, read_partial_die.  */

#include "defs.h"
#include "dwarf2/read.h"
#include "dwarf2/abbrev.h"
#include "dwarf2/aranges.h"
#include "dwarf2/attribute.h"
#include "dwarf2/comp-unit-head.h"
#include "dwarf2/cu.h"
#include "dwarf2/index-cache.h"
#include "dwarf2/index-common.h"
#include "dwarf2/leb.h"
#include "dwarf2/line-header.h"
#include "dwarf2/dwz.h"
#include "dwarf2/macro.h"
#include "dwarf2/die.h"
#include "dwarf2/read-debug-names.h"
#include "dwarf2/read-gdb-index.h"
#include "dwarf2/sect-names.h"
#include "dwarf2/stringify.h"
#include "dwarf2/public.h"
#include "bfd.h"
#include "elf-bfd.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "objfiles.h"
#include "dwarf2.h"
#include "demangle.h"
#include "gdb-demangle.h"
#include "filenames.h"
#include "language.h"
#include "complaints.h"
#include "dwarf2/expr.h"
#include "dwarf2/loc.h"
#include "cp-support.h"
#include "hashtab.h"
#include "command.h"
#include "gdbcmd.h"
#include "block.h"
#include "addrmap.h"
#include "typeprint.h"
#include "c-lang.h"
#include "go-lang.h"
#include "valprint.h"
#include "gdbcore.h"
#include "gdb/gdb-index.h"
#include "gdb_bfd.h"
#include "f-lang.h"
#include "source.h"
#include "build-id.h"
#include "namespace.h"
#include "gdbsupport/function-view.h"
#include <optional>
#include "gdbsupport/underlying.h"
#include "gdbsupport/hash_enum.h"
#include "filename-seen-cache.h"
#include "producer.h"
#include <fcntl.h>
#include <algorithm>
#include <unordered_map>
#include "gdbsupport/selftest.h"
#include "rust-lang.h"
#include "gdbsupport/pathstuff.h"
#include "count-one-bits.h"
#include <unordered_set>
#include "dwarf2/abbrev-cache.h"
#include "cooked-index.h"
#include "split-name.h"
#include "gdbsupport/thread-pool.h"
#include "run-on-main-thread.h"

/* When == 1, print basic high level tracing messages.
   When > 1, be more verbose.
   This is in contrast to the low level DIE reading of dwarf_die_debug.  */
static unsigned int dwarf_read_debug = 0;

/* Print a "dwarf-read" debug statement if dwarf_read_debug is >= 1.  */

#define dwarf_read_debug_printf(fmt, ...) \
  debug_prefixed_printf_cond (dwarf_read_debug >= 1, "dwarf-read", fmt, \
			      ##__VA_ARGS__)

/* Print a "dwarf-read" debug statement if dwarf_read_debug is >= 2.  */

#define dwarf_read_debug_printf_v(fmt, ...) \
  debug_prefixed_printf_cond (dwarf_read_debug >= 2, "dwarf-read", fmt, \
			      ##__VA_ARGS__)

/* When non-zero, dump DIEs after they are read in.  */
static unsigned int dwarf_die_debug = 0;

/* When non-zero, dump line number entries as they are read in.  */
unsigned int dwarf_line_debug = 0;

/* When true, cross-check physname against demangler.  */
static bool check_physname = false;

/* This is used to store the data that is always per objfile.  */
static const registry<objfile>::key<dwarf2_per_objfile>
     dwarf2_objfile_data_key;

/* These are used to store the dwarf2_per_bfd objects.

   objfiles having the same BFD, which doesn't require relocations, are going to
   share a dwarf2_per_bfd object, which is held in the _bfd_data_key version.

   Other objfiles are not going to share a dwarf2_per_bfd with any other
   objfiles, so they'll have their own version kept in the _objfile_data_key
   version.  */
static const registry<bfd>::key<dwarf2_per_bfd> dwarf2_per_bfd_bfd_data_key;
static const registry<objfile>::key<dwarf2_per_bfd>
  dwarf2_per_bfd_objfile_data_key;

/* The "aclass" indices for various kinds of computed DWARF symbols.  */

static int dwarf2_locexpr_index;
static int dwarf2_loclist_index;
static int ada_imported_index;
static int dwarf2_locexpr_block_index;
static int dwarf2_loclist_block_index;
static int ada_block_index;

static bool producer_is_gas_lt_2_38 (struct dwarf2_cu *cu);

/* Size of .debug_loclists section header for 32-bit DWARF format.  */
#define LOCLIST_HEADER_SIZE32 12

/* Size of .debug_loclists section header for 64-bit DWARF format.  */
#define LOCLIST_HEADER_SIZE64 20

/* Size of .debug_rnglists section header for 32-bit DWARF format.  */
#define RNGLIST_HEADER_SIZE32 12

/* Size of .debug_rnglists section header for 64-bit DWARF format.  */
#define RNGLIST_HEADER_SIZE64 20

/* See dwarf2/read.h.  */

dwarf2_per_objfile *
get_dwarf2_per_objfile (struct objfile *objfile)
{
  return dwarf2_objfile_data_key.get (objfile);
}

/* Default names of the debugging sections.  */

/* Note that if the debugging section has been compressed, it might
   have a name like .zdebug_info.  */

const struct dwarf2_debug_sections dwarf2_elf_names =
{
  { ".debug_info", ".zdebug_info" },
  { ".debug_abbrev", ".zdebug_abbrev" },
  { ".debug_line", ".zdebug_line" },
  { ".debug_loc", ".zdebug_loc" },
  { ".debug_loclists", ".zdebug_loclists" },
  { ".debug_macinfo", ".zdebug_macinfo" },
  { ".debug_macro", ".zdebug_macro" },
  { ".debug_str", ".zdebug_str" },
  { ".debug_str_offsets", ".zdebug_str_offsets" },
  { ".debug_line_str", ".zdebug_line_str" },
  { ".debug_ranges", ".zdebug_ranges" },
  { ".debug_rnglists", ".zdebug_rnglists" },
  { ".debug_types", ".zdebug_types" },
  { ".debug_addr", ".zdebug_addr" },
  { ".debug_frame", ".zdebug_frame" },
  { ".eh_frame", NULL },
  { ".gdb_index", ".zgdb_index" },
  { ".debug_names", ".zdebug_names" },
  { ".debug_aranges", ".zdebug_aranges" },
  23
};

/* List of DWO/DWP sections.  */

static const struct dwop_section_names
{
  struct dwarf2_section_names abbrev_dwo;
  struct dwarf2_section_names info_dwo;
  struct dwarf2_section_names line_dwo;
  struct dwarf2_section_names loc_dwo;
  struct dwarf2_section_names loclists_dwo;
  struct dwarf2_section_names macinfo_dwo;
  struct dwarf2_section_names macro_dwo;
  struct dwarf2_section_names rnglists_dwo;
  struct dwarf2_section_names str_dwo;
  struct dwarf2_section_names str_offsets_dwo;
  struct dwarf2_section_names types_dwo;
  struct dwarf2_section_names cu_index;
  struct dwarf2_section_names tu_index;
}
dwop_section_names =
{
  { ".debug_abbrev.dwo", ".zdebug_abbrev.dwo" },
  { ".debug_info.dwo", ".zdebug_info.dwo" },
  { ".debug_line.dwo", ".zdebug_line.dwo" },
  { ".debug_loc.dwo", ".zdebug_loc.dwo" },
  { ".debug_loclists.dwo", ".zdebug_loclists.dwo" },
  { ".debug_macinfo.dwo", ".zdebug_macinfo.dwo" },
  { ".debug_macro.dwo", ".zdebug_macro.dwo" },
  { ".debug_rnglists.dwo", ".zdebug_rnglists.dwo" },
  { ".debug_str.dwo", ".zdebug_str.dwo" },
  { ".debug_str_offsets.dwo", ".zdebug_str_offsets.dwo" },
  { ".debug_types.dwo", ".zdebug_types.dwo" },
  { ".debug_cu_index", ".zdebug_cu_index" },
  { ".debug_tu_index", ".zdebug_tu_index" },
};

/* local data types */

/* The location list and range list sections (.debug_loclists & .debug_rnglists)
   begin with a header,  which contains the following information.  */
struct loclists_rnglists_header
{
  /* A 4-byte or 12-byte length containing the length of the
     set of entries for this compilation unit, not including the
     length field itself.  */
  unsigned int length;

  /* A 2-byte version identifier.  */
  short version;

  /* A 1-byte unsigned integer containing the size in bytes of an address on
     the target system.  */
  unsigned char addr_size;

  /* A 1-byte unsigned integer containing the size in bytes of a segment selector
     on the target system.  */
  unsigned char segment_collector_size;

  /* A 4-byte count of the number of offsets that follow the header.  */
  unsigned int offset_entry_count;
};

/* A struct that can be used as a hash key for tables based on DW_AT_stmt_list.
   This includes type_unit_group and quick_file_names.  */

struct stmt_list_hash
{
  /* The DWO unit this table is from or NULL if there is none.  */
  struct dwo_unit *dwo_unit;

  /* Offset in .debug_line or .debug_line.dwo.  */
  sect_offset line_sect_off;
};

/* Each element of dwarf2_per_bfd->type_unit_groups is a pointer to
   an object of this type.  This contains elements of type unit groups
   that can be shared across objfiles.  The non-shareable parts are in
   type_unit_group_unshareable.  */

struct type_unit_group
{
  /* The data used to construct the hash key.  */
  struct stmt_list_hash hash {};
};

/* These sections are what may appear in a (real or virtual) DWO file.  */

struct dwo_sections
{
  struct dwarf2_section_info abbrev;
  struct dwarf2_section_info line;
  struct dwarf2_section_info loc;
  struct dwarf2_section_info loclists;
  struct dwarf2_section_info macinfo;
  struct dwarf2_section_info macro;
  struct dwarf2_section_info rnglists;
  struct dwarf2_section_info str;
  struct dwarf2_section_info str_offsets;
  /* In the case of a virtual DWO file, these two are unused.  */
  struct dwarf2_section_info info;
  std::vector<dwarf2_section_info> types;
};

/* CUs/TUs in DWP/DWO files.  */

struct dwo_unit
{
  /* Backlink to the containing struct dwo_file.  */
  struct dwo_file *dwo_file;

  /* The "id" that distinguishes this CU/TU.
     .debug_info calls this "dwo_id", .debug_types calls this "signature".
     Since signatures came first, we stick with it for consistency.  */
  ULONGEST signature;

  /* The section this CU/TU lives in, in the DWO file.  */
  struct dwarf2_section_info *section;

  /* Same as dwarf2_per_cu_data:{sect_off,length} but in the DWO section.  */
  sect_offset sect_off;
  unsigned int length;

  /* For types, offset in the type's DIE of the type defined by this TU.  */
  cu_offset type_offset_in_tu;
};

/* include/dwarf2.h defines the DWP section codes.
   It defines a max value but it doesn't define a min value, which we
   use for error checking, so provide one.  */

enum dwp_v2_section_ids
{
  DW_SECT_MIN = 1
};

/* Data for one DWO file.

   This includes virtual DWO files (a virtual DWO file is a DWO file as it
   appears in a DWP file).  DWP files don't really have DWO files per se -
   comdat folding of types "loses" the DWO file they came from, and from
   a high level view DWP files appear to contain a mass of random types.
   However, to maintain consistency with the non-DWP case we pretend DWP
   files contain virtual DWO files, and we assign each TU with one virtual
   DWO file (generally based on the line and abbrev section offsets -
   a heuristic that seems to work in practice).  */

struct dwo_file
{
  dwo_file () = default;
  DISABLE_COPY_AND_ASSIGN (dwo_file);

  /* The DW_AT_GNU_dwo_name or DW_AT_dwo_name attribute.
     For virtual DWO files the name is constructed from the section offsets
     of abbrev,line,loc,str_offsets so that we combine virtual DWO files
     from related CU+TUs.  */
  std::string dwo_name;

  /* The DW_AT_comp_dir attribute.  */
  const char *comp_dir = nullptr;

  /* The bfd, when the file is open.  Otherwise this is NULL.
     This is unused(NULL) for virtual DWO files where we use dwp_file.dbfd.  */
  gdb_bfd_ref_ptr dbfd;

  /* The sections that make up this DWO file.
     Remember that for virtual DWO files in DWP V2 or DWP V5, these are virtual
     sections (for lack of a better name).  */
  struct dwo_sections sections {};

  /* The CUs in the file.
     Each element is a struct dwo_unit. Multiple CUs per DWO are supported as
     an extension to handle LLVM's Link Time Optimization output (where
     multiple source files may be compiled into a single object/dwo pair). */
  htab_up cus;

  /* Table of TUs in the file.
     Each element is a struct dwo_unit.  */
  htab_up tus;
};

/* These sections are what may appear in a DWP file.  */

struct dwp_sections
{
  /* These are used by all DWP versions (1, 2 and 5).  */
  struct dwarf2_section_info str;
  struct dwarf2_section_info cu_index;
  struct dwarf2_section_info tu_index;

  /* These are only used by DWP version 2 and version 5 files.
     In DWP version 1 the .debug_info.dwo, .debug_types.dwo, and other
     sections are referenced by section number, and are not recorded here.
     In DWP version 2 or 5 there is at most one copy of all these sections,
     each section being (effectively) comprised of the concatenation of all of
     the individual sections that exist in the version 1 format.
     To keep the code simple we treat each of these concatenated pieces as a
     section itself (a virtual section?).  */
  struct dwarf2_section_info abbrev;
  struct dwarf2_section_info info;
  struct dwarf2_section_info line;
  struct dwarf2_section_info loc;
  struct dwarf2_section_info loclists;
  struct dwarf2_section_info macinfo;
  struct dwarf2_section_info macro;
  struct dwarf2_section_info rnglists;
  struct dwarf2_section_info str_offsets;
  struct dwarf2_section_info types;
};

/* These sections are what may appear in a virtual DWO file in DWP version 1.
   A virtual DWO file is a DWO file as it appears in a DWP file.  */

struct virtual_v1_dwo_sections
{
  struct dwarf2_section_info abbrev;
  struct dwarf2_section_info line;
  struct dwarf2_section_info loc;
  struct dwarf2_section_info macinfo;
  struct dwarf2_section_info macro;
  struct dwarf2_section_info str_offsets;
  /* Each DWP hash table entry records one CU or one TU.
     That is recorded here, and copied to dwo_unit.section.  */
  struct dwarf2_section_info info_or_types;
};

/* Similar to virtual_v1_dwo_sections, but for DWP version 2 or 5.
   In version 2, the sections of the DWO files are concatenated together
   and stored in one section of that name.  Thus each ELF section contains
   several "virtual" sections.  */

struct virtual_v2_or_v5_dwo_sections
{
  bfd_size_type abbrev_offset;
  bfd_size_type abbrev_size;

  bfd_size_type line_offset;
  bfd_size_type line_size;

  bfd_size_type loc_offset;
  bfd_size_type loc_size;

  bfd_size_type loclists_offset;
  bfd_size_type loclists_size;

  bfd_size_type macinfo_offset;
  bfd_size_type macinfo_size;

  bfd_size_type macro_offset;
  bfd_size_type macro_size;

  bfd_size_type rnglists_offset;
  bfd_size_type rnglists_size;

  bfd_size_type str_offsets_offset;
  bfd_size_type str_offsets_size;

  /* Each DWP hash table entry records one CU or one TU.
     That is recorded here, and copied to dwo_unit.section.  */
  bfd_size_type info_or_types_offset;
  bfd_size_type info_or_types_size;
};

/* Contents of DWP hash tables.  */

struct dwp_hash_table
{
  uint32_t version, nr_columns;
  uint32_t nr_units, nr_slots;
  const gdb_byte *hash_table, *unit_table;
  union
  {
    struct
    {
      const gdb_byte *indices;
    } v1;
    struct
    {
      /* This is indexed by column number and gives the id of the section
	 in that column.  */
#define MAX_NR_V2_DWO_SECTIONS \
  (1 /* .debug_info or .debug_types */ \
   + 1 /* .debug_abbrev */ \
   + 1 /* .debug_line */ \
   + 1 /* .debug_loc */ \
   + 1 /* .debug_str_offsets */ \
   + 1 /* .debug_macro or .debug_macinfo */)
      int section_ids[MAX_NR_V2_DWO_SECTIONS];
      const gdb_byte *offsets;
      const gdb_byte *sizes;
    } v2;
    struct
    {
      /* This is indexed by column number and gives the id of the section
	 in that column.  */
#define MAX_NR_V5_DWO_SECTIONS \
  (1 /* .debug_info */ \
   + 1 /* .debug_abbrev */ \
   + 1 /* .debug_line */ \
   + 1 /* .debug_loclists */ \
   + 1 /* .debug_str_offsets */ \
   + 1 /* .debug_macro */ \
   + 1 /* .debug_rnglists */)
      int section_ids[MAX_NR_V5_DWO_SECTIONS];
      const gdb_byte *offsets;
      const gdb_byte *sizes;
    } v5;
  } section_pool;
};

/* Data for one DWP file.  */

struct dwp_file
{
  dwp_file (const char *name_, gdb_bfd_ref_ptr &&abfd)
    : name (name_),
      dbfd (std::move (abfd))
  {
  }

  /* Name of the file.  */
  const char *name;

  /* File format version.  */
  int version = 0;

  /* The bfd.  */
  gdb_bfd_ref_ptr dbfd;

  /* Section info for this file.  */
  struct dwp_sections sections {};

  /* Table of CUs in the file.  */
  const struct dwp_hash_table *cus = nullptr;

  /* Table of TUs in the file.  */
  const struct dwp_hash_table *tus = nullptr;

  /* Tables of loaded CUs/TUs.  Each entry is a struct dwo_unit *.  */
  htab_up loaded_cus;
  htab_up loaded_tus;

  /* Table to map ELF section numbers to their sections.
     This is only needed for the DWP V1 file format.  */
  unsigned int num_sections = 0;
  asection **elf_sections = nullptr;
};

/* Struct used to pass misc. parameters to read_die_and_children, et
   al.  which are used for both .debug_info and .debug_types dies.
   All parameters here are unchanging for the life of the call.  This
   struct exists to abstract away the constant parameters of die reading.  */

struct die_reader_specs
{
  /* The bfd of die_section.  */
  bfd *abfd;

  /* The CU of the DIE we are parsing.  */
  struct dwarf2_cu *cu;

  /* Non-NULL if reading a DWO file (including one packaged into a DWP).  */
  struct dwo_file *dwo_file;

  /* The section the die comes from.
     This is either .debug_info or .debug_types, or the .dwo variants.  */
  struct dwarf2_section_info *die_section;

  /* die_section->buffer.  */
  const gdb_byte *buffer;

  /* The end of the buffer.  */
  const gdb_byte *buffer_end;

  /* The abbreviation table to use when reading the DIEs.  */
  struct abbrev_table *abbrev_table;
};

/* A subclass of die_reader_specs that holds storage and has complex
   constructor and destructor behavior.  */

class cutu_reader : public die_reader_specs
{
public:

  cutu_reader (dwarf2_per_cu_data *this_cu,
	       dwarf2_per_objfile *per_objfile,
	       struct abbrev_table *abbrev_table,
	       dwarf2_cu *existing_cu,
	       bool skip_partial,
	       abbrev_cache *cache = nullptr);

  explicit cutu_reader (struct dwarf2_per_cu_data *this_cu,
			dwarf2_per_objfile *per_objfile,
			struct dwarf2_cu *parent_cu = nullptr,
			struct dwo_file *dwo_file = nullptr);

  DISABLE_COPY_AND_ASSIGN (cutu_reader);

  cutu_reader (cutu_reader &&) = default;

  const gdb_byte *info_ptr = nullptr;
  struct die_info *comp_unit_die = nullptr;
  bool dummy_p = false;

  /* Release the new CU, putting it on the chain.  This cannot be done
     for dummy CUs.  */
  void keep ();

  /* Release the abbrev table, transferring ownership to the
     caller.  */
  abbrev_table_up release_abbrev_table ()
  {
    return std::move (m_abbrev_table_holder);
  }

private:
  void init_tu_and_read_dwo_dies (dwarf2_per_cu_data *this_cu,
				  dwarf2_per_objfile *per_objfile,
				  dwarf2_cu *existing_cu);

  struct dwarf2_per_cu_data *m_this_cu;
  std::unique_ptr<dwarf2_cu> m_new_cu;

  /* The ordinary abbreviation table.  */
  abbrev_table_up m_abbrev_table_holder;

  /* The DWO abbreviation table.  */
  abbrev_table_up m_dwo_abbrev_table;
};

/* FIXME: We might want to set this from BFD via bfd_arch_bits_per_byte,
   but this would require a corresponding change in unpack_field_as_long
   and friends.  */
static int bits_per_byte = 8;

struct variant_part_builder;

/* When reading a variant, we track a bit more information about the
   field, and store it in an object of this type.  */

struct variant_field
{
  int first_field = -1;
  int last_field = -1;

  /* A variant can contain other variant parts.  */
  std::vector<variant_part_builder> variant_parts;

  /* If we see a DW_TAG_variant, then this will be set if this is the
     default branch.  */
  bool default_branch = false;
  /* If we see a DW_AT_discr_value, then this will be the discriminant
     value.  */
  ULONGEST discriminant_value = 0;
  /* If we see a DW_AT_discr_list, then this is a pointer to the list
     data.  */
  struct dwarf_block *discr_list_data = nullptr;
};

/* This represents a DW_TAG_variant_part.  */

struct variant_part_builder
{
  /* The offset of the discriminant field.  */
  sect_offset discriminant_offset {};

  /* Variants that are direct children of this variant part.  */
  std::vector<variant_field> variants;

  /* True if we're currently reading a variant.  */
  bool processing_variant = false;
};

struct nextfield
{
  /* Variant parts need to find the discriminant, which is a DIE
     reference.  We track the section offset of each field to make
     this link.  */
  sect_offset offset;
  struct field field {};
};

struct fnfieldlist
{
  const char *name = nullptr;
  std::vector<struct fn_field> fnfields;
};

/* The routines that read and process dies for a C struct or C++ class
   pass lists of data member fields and lists of member function fields
   in an instance of a field_info structure, as defined below.  */
struct field_info
{
  /* List of data member and baseclasses fields.  */
  std::vector<struct nextfield> fields;
  std::vector<struct nextfield> baseclasses;

  /* Member function fieldlist array, contains name of possibly overloaded
     member function, number of overloaded member functions and a pointer
     to the head of the member function field chain.  */
  std::vector<struct fnfieldlist> fnfieldlists;

  /* typedefs defined inside this class.  TYPEDEF_FIELD_LIST contains head of
     a NULL terminated list of TYPEDEF_FIELD_LIST_COUNT elements.  */
  std::vector<struct decl_field> typedef_field_list;

  /* Nested types defined by this class and the number of elements in this
     list.  */
  std::vector<struct decl_field> nested_types_list;

  /* If non-null, this is the variant part we are currently
     reading.  */
  variant_part_builder *current_variant_part = nullptr;
  /* This holds all the top-level variant parts attached to the type
     we're reading.  */
  std::vector<variant_part_builder> variant_parts;

  /* Return the total number of fields (including baseclasses).  */
  int nfields () const
  {
    return fields.size () + baseclasses.size ();
  }
};

/* Loaded secondary compilation units are kept in memory until they
   have not been referenced for the processing of this many
   compilation units.  Set this to zero to disable caching.  Cache
   sizes of up to at least twenty will improve startup time for
   typical inter-CU-reference binaries, at an obvious memory cost.  */
static int dwarf_max_cache_age = 5;
static void
show_dwarf_max_cache_age (struct ui_file *file, int from_tty,
			  struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("The upper bound on the age of cached "
		      "DWARF compilation units is %s.\n"),
	      value);
}

/* When true, wait for DWARF reading to be complete.  */
static bool dwarf_synchronous = false;

/* "Show" callback for "maint set dwarf synchronous".  */
static void
show_dwarf_synchronous (struct ui_file *file, int from_tty,
			struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Whether DWARF reading is synchronous is %s.\n"),
	      value);
}

/* local function prototypes */

static void dwarf2_find_base_address (struct die_info *die,
				      struct dwarf2_cu *cu);

static void build_type_psymtabs_reader (cutu_reader *reader,
					cooked_index_storage *storage);

static void var_decode_location (struct attribute *attr,
				 struct symbol *sym,
				 struct dwarf2_cu *cu);

static unsigned int peek_abbrev_code (bfd *, const gdb_byte *);

static const gdb_byte *read_attribute (const struct die_reader_specs *,
				       struct attribute *,
				       const struct attr_abbrev *,
				       const gdb_byte *,
				       bool allow_reprocess = true);

/* Note that the default for TAG is chosen because it only matters
   when reading the top-level DIE, and that function is careful to
   pass the correct tag.  */
static void read_attribute_reprocess (const struct die_reader_specs *reader,
				      struct attribute *attr,
				      dwarf_tag tag = DW_TAG_padding);

static unrelocated_addr read_addr_index (struct dwarf2_cu *cu,
					 unsigned int addr_index);

static sect_offset read_abbrev_offset (dwarf2_per_objfile *per_objfile,
				       dwarf2_section_info *, sect_offset);

static const char *read_indirect_string
  (dwarf2_per_objfile *per_objfile, bfd *, const gdb_byte *,
   const struct comp_unit_head *, unsigned int *);

static unrelocated_addr read_addr_index_from_leb128 (struct dwarf2_cu *,
						     const gdb_byte *,
						     unsigned int *);

static const char *read_dwo_str_index (const struct die_reader_specs *reader,
				       ULONGEST str_index);

static const char *read_stub_str_index (struct dwarf2_cu *cu,
					ULONGEST str_index);

static struct attribute *dwarf2_attr (struct die_info *, unsigned int,
				      struct dwarf2_cu *);

static const char *dwarf2_string_attr (struct die_info *die, unsigned int name,
				       struct dwarf2_cu *cu);

static const char *dwarf2_dwo_name (struct die_info *die, struct dwarf2_cu *cu);

static int dwarf2_flag_true_p (struct die_info *die, unsigned name,
			       struct dwarf2_cu *cu);

static int die_is_declaration (struct die_info *, struct dwarf2_cu *cu);

static struct die_info *die_specification (struct die_info *die,
					   struct dwarf2_cu **);

static line_header_up dwarf_decode_line_header (sect_offset sect_off,
						struct dwarf2_cu *cu,
						const char *comp_dir);

static void dwarf_decode_lines (struct line_header *,
				struct dwarf2_cu *,
				unrelocated_addr, int decode_mapping);

static void dwarf2_start_subfile (dwarf2_cu *cu, const file_entry &fe,
				  const line_header &lh);

static struct symbol *new_symbol (struct die_info *, struct type *,
				  struct dwarf2_cu *, struct symbol * = NULL);

static void dwarf2_const_value (const struct attribute *, struct symbol *,
				struct dwarf2_cu *);

static void dwarf2_const_value_attr (const struct attribute *attr,
				     struct type *type,
				     const char *name,
				     struct obstack *obstack,
				     struct dwarf2_cu *cu, LONGEST *value,
				     const gdb_byte **bytes,
				     struct dwarf2_locexpr_baton **baton);

static struct type *read_subrange_index_type (struct die_info *die,
					      struct dwarf2_cu *cu);

static struct type *die_type (struct die_info *, struct dwarf2_cu *);

static int need_gnat_info (struct dwarf2_cu *);

static struct type *die_descriptive_type (struct die_info *,
					  struct dwarf2_cu *);

static void set_descriptive_type (struct type *, struct die_info *,
				  struct dwarf2_cu *);

static struct type *die_containing_type (struct die_info *,
					 struct dwarf2_cu *);

static struct type *lookup_die_type (struct die_info *, const struct attribute *,
				     struct dwarf2_cu *);

static struct type *read_type_die (struct die_info *, struct dwarf2_cu *);

static struct type *read_type_die_1 (struct die_info *, struct dwarf2_cu *);

static const char *determine_prefix (struct die_info *die, struct dwarf2_cu *);

static char *typename_concat (struct obstack *obs, const char *prefix,
			      const char *suffix, int physname,
			      struct dwarf2_cu *cu);

static void read_file_scope (struct die_info *, struct dwarf2_cu *);

static void read_type_unit_scope (struct die_info *, struct dwarf2_cu *);

static void read_func_scope (struct die_info *, struct dwarf2_cu *);

static void read_lexical_block_scope (struct die_info *, struct dwarf2_cu *);

static void read_call_site_scope (struct die_info *die, struct dwarf2_cu *cu);

static void read_variable (struct die_info *die, struct dwarf2_cu *cu);

/* Return the .debug_loclists section to use for cu.  */
static struct dwarf2_section_info *cu_debug_loc_section (struct dwarf2_cu *cu);

/* Return the .debug_rnglists section to use for cu.  */
static struct dwarf2_section_info *cu_debug_rnglists_section
  (struct dwarf2_cu *cu, dwarf_tag tag);

/* How dwarf2_get_pc_bounds constructed its *LOWPC and *HIGHPC return
   values.  Keep the items ordered with increasing constraints compliance.  */
enum pc_bounds_kind
{
  /* No attribute DW_AT_low_pc, DW_AT_high_pc or DW_AT_ranges was found.  */
  PC_BOUNDS_NOT_PRESENT,

  /* Some of the attributes DW_AT_low_pc, DW_AT_high_pc or DW_AT_ranges
     were present but they do not form a valid range of PC addresses.  */
  PC_BOUNDS_INVALID,

  /* Discontiguous range was found - that is DW_AT_ranges was found.  */
  PC_BOUNDS_RANGES,

  /* Contiguous range was found - DW_AT_low_pc and DW_AT_high_pc were found.  */
  PC_BOUNDS_HIGH_LOW,
};

static enum pc_bounds_kind dwarf2_get_pc_bounds (struct die_info *,
						 unrelocated_addr *,
						 unrelocated_addr *,
						 struct dwarf2_cu *,
						 addrmap *,
						 void *);

static void get_scope_pc_bounds (struct die_info *,
				 unrelocated_addr *, unrelocated_addr *,
				 struct dwarf2_cu *);

static void dwarf2_record_block_ranges (struct die_info *, struct block *,
					struct dwarf2_cu *);

static void dwarf2_add_field (struct field_info *, struct die_info *,
			      struct dwarf2_cu *);

static void dwarf2_attach_fields_to_type (struct field_info *,
					  struct type *, struct dwarf2_cu *);

static void dwarf2_add_member_fn (struct field_info *,
				  struct die_info *, struct type *,
				  struct dwarf2_cu *);

static void dwarf2_attach_fn_fields_to_type (struct field_info *,
					     struct type *,
					     struct dwarf2_cu *);

static void process_structure_scope (struct die_info *, struct dwarf2_cu *);

static void read_common_block (struct die_info *, struct dwarf2_cu *);

static void read_namespace (struct die_info *die, struct dwarf2_cu *);

static void read_module (struct die_info *die, struct dwarf2_cu *cu);

static struct using_direct **using_directives (struct dwarf2_cu *cu);

static void read_import_statement (struct die_info *die, struct dwarf2_cu *);

static bool read_alias (struct die_info *die, struct dwarf2_cu *cu);

static struct type *read_module_type (struct die_info *die,
				      struct dwarf2_cu *cu);

static const char *namespace_name (struct die_info *die,
				   int *is_anonymous, struct dwarf2_cu *);

static void process_enumeration_scope (struct die_info *, struct dwarf2_cu *);

static bool decode_locdesc (struct dwarf_block *, struct dwarf2_cu *,
			    CORE_ADDR *addr);

static enum dwarf_array_dim_ordering read_array_order (struct die_info *,
						       struct dwarf2_cu *);

static struct die_info *read_die_and_siblings_1
  (const struct die_reader_specs *, const gdb_byte *, const gdb_byte **,
   struct die_info *);

static struct die_info *read_die_and_siblings (const struct die_reader_specs *,
					       const gdb_byte *info_ptr,
					       const gdb_byte **new_info_ptr,
					       struct die_info *parent);

static const gdb_byte *read_full_die_1 (const struct die_reader_specs *,
					struct die_info **, const gdb_byte *,
					int, bool);

static const gdb_byte *read_toplevel_die (const struct die_reader_specs *,
					  struct die_info **,
					  const gdb_byte *,
					  gdb::array_view<attribute *>  = {});

static void process_die (struct die_info *, struct dwarf2_cu *);

static const char *dwarf2_canonicalize_name (const char *, struct dwarf2_cu *,
					     struct objfile *);

static const char *dwarf2_name (struct die_info *die, struct dwarf2_cu *);

static const char *dwarf2_full_name (const char *name,
				     struct die_info *die,
				     struct dwarf2_cu *cu);

static const char *dwarf2_physname (const char *name, struct die_info *die,
				    struct dwarf2_cu *cu);

static struct die_info *dwarf2_extension (struct die_info *die,
					  struct dwarf2_cu **);

static void store_in_ref_table (struct die_info *,
				struct dwarf2_cu *);

static struct die_info *follow_die_ref_or_sig (struct die_info *,
					       const struct attribute *,
					       struct dwarf2_cu **);

static struct die_info *follow_die_ref (struct die_info *,
					const struct attribute *,
					struct dwarf2_cu **);

static struct die_info *follow_die_sig (struct die_info *,
					const struct attribute *,
					struct dwarf2_cu **);

static struct type *get_signatured_type (struct die_info *, ULONGEST,
					 struct dwarf2_cu *);

static struct type *get_DW_AT_signature_type (struct die_info *,
					      const struct attribute *,
					      struct dwarf2_cu *);

static void load_full_type_unit (dwarf2_per_cu_data *per_cu,
				 dwarf2_per_objfile *per_objfile);

static void read_signatured_type (signatured_type *sig_type,
				  dwarf2_per_objfile *per_objfile);

static int attr_to_dynamic_prop (const struct attribute *attr,
				 struct die_info *die, struct dwarf2_cu *cu,
				 struct dynamic_prop *prop, struct type *type);

/* memory allocation interface */

static struct dwarf_block *dwarf_alloc_block (struct dwarf2_cu *);

static void dwarf_decode_macros (struct dwarf2_cu *, unsigned int, int);

static void fill_in_loclist_baton (struct dwarf2_cu *cu,
				   struct dwarf2_loclist_baton *baton,
				   const struct attribute *attr);

static void dwarf2_symbol_mark_computed (const struct attribute *attr,
					 struct symbol *sym,
					 struct dwarf2_cu *cu,
					 int is_block);

static const gdb_byte *skip_one_die (const struct die_reader_specs *reader,
				     const gdb_byte *info_ptr,
				     const struct abbrev_info *abbrev,
				     bool do_skip_children = true);

static struct dwarf2_per_cu_data *dwarf2_find_containing_comp_unit
  (sect_offset sect_off, unsigned int offset_in_dwz,
   dwarf2_per_bfd *per_bfd);

static void prepare_one_comp_unit (struct dwarf2_cu *cu,
				   struct die_info *comp_unit_die,
				   enum language pretend_language);

static struct type *set_die_type (struct die_info *, struct type *,
				  struct dwarf2_cu *, bool = false);

static void load_full_comp_unit (dwarf2_per_cu_data *per_cu,
				 dwarf2_per_objfile *per_objfile,
				 dwarf2_cu *existing_cu,
				 bool skip_partial,
				 enum language pretend_language);

static void process_full_comp_unit (dwarf2_cu *cu,
				    enum language pretend_language);

static void process_full_type_unit (dwarf2_cu *cu,
				    enum language pretend_language);

static struct type *get_die_type_at_offset (sect_offset,
					    dwarf2_per_cu_data *per_cu,
					    dwarf2_per_objfile *per_objfile);

static struct type *get_die_type (struct die_info *die, struct dwarf2_cu *cu);

static void queue_comp_unit (dwarf2_per_cu_data *per_cu,
			     dwarf2_per_objfile *per_objfile,
			     enum language pretend_language);

static void process_queue (dwarf2_per_objfile *per_objfile);

static bool is_ada_import_or_export (dwarf2_cu *cu, const char *name,
				     const char *linkagename);

/* Class, the destructor of which frees all allocated queue entries.  This
   will only have work to do if an error was thrown while processing the
   dwarf.  If no error was thrown then the queue entries should have all
   been processed, and freed, as we went along.  */

class dwarf2_queue_guard
{
public:
  explicit dwarf2_queue_guard (dwarf2_per_objfile *per_objfile)
    : m_per_objfile (per_objfile)
  {
    gdb_assert (!m_per_objfile->queue.has_value ());

    m_per_objfile->queue.emplace ();
  }

  /* Free any entries remaining on the queue.  There should only be
     entries left if we hit an error while processing the dwarf.  */
  ~dwarf2_queue_guard ()
  {
    gdb_assert (m_per_objfile->queue.has_value ());

    m_per_objfile->queue.reset ();
  }

  DISABLE_COPY_AND_ASSIGN (dwarf2_queue_guard);

private:
  dwarf2_per_objfile *m_per_objfile;
};

dwarf2_queue_item::~dwarf2_queue_item ()
{
  /* Anything still marked queued is likely to be in an
     inconsistent state, so discard it.  */
  if (per_cu->queued)
    {
      per_objfile->remove_cu (per_cu);
      per_cu->queued = 0;
    }
}

/* See dwarf2/read.h.  */

void
dwarf2_per_cu_data_deleter::operator() (dwarf2_per_cu_data *data)
{
  if (data->is_debug_types)
    delete static_cast<signatured_type *> (data);
  else
    delete data;
}

static file_and_directory &find_file_and_directory
     (struct die_info *die, struct dwarf2_cu *cu);

static const char *compute_include_file_name
     (const struct line_header *lh,
      const file_entry &fe,
      const file_and_directory &cu_info,
      std::string &name_holder);

static htab_up allocate_dwo_unit_table ();

static struct dwo_unit *lookup_dwo_unit_in_dwp
  (dwarf2_per_objfile *per_objfile, struct dwp_file *dwp_file,
   const char *comp_dir, ULONGEST signature, int is_debug_types);

static struct dwp_file *get_dwp_file (dwarf2_per_objfile *per_objfile);

static struct dwo_unit *lookup_dwo_comp_unit
  (dwarf2_cu *cu, const char *dwo_name, const char *comp_dir,
   ULONGEST signature);

static struct dwo_unit *lookup_dwo_type_unit
  (dwarf2_cu *cu, const char *dwo_name, const char *comp_dir);

static void queue_and_load_all_dwo_tus (dwarf2_cu *cu);

/* A unique pointer to a dwo_file.  */

typedef std::unique_ptr<struct dwo_file> dwo_file_up;

static void process_cu_includes (dwarf2_per_objfile *per_objfile);

static void check_producer (struct dwarf2_cu *cu);

/* Various complaints about symbol reading that don't abort the process.  */

static void
dwarf2_debug_line_missing_file_complaint (void)
{
  complaint (_(".debug_line section has line data without a file"));
}

static void
dwarf2_debug_line_missing_end_sequence_complaint (void)
{
  complaint (_(".debug_line section has line "
	       "program sequence without an end"));
}

static void
dwarf2_complex_location_expr_complaint (void)
{
  complaint (_("location expression too complex"));
}

static void
dwarf2_const_value_length_mismatch_complaint (const char *arg1, int arg2,
					      int arg3)
{
  complaint (_("const value length mismatch for '%s', got %d, expected %d"),
	     arg1, arg2, arg3);
}

static void
dwarf2_invalid_attrib_class_complaint (const char *arg1, const char *arg2)
{
  complaint (_("invalid attribute class or form for '%s' in '%s'"),
	     arg1, arg2);
}

/* See read.h.  */

unrelocated_addr
dwarf2_per_objfile::adjust (unrelocated_addr addr)
{
  CORE_ADDR baseaddr = objfile->text_section_offset ();
  CORE_ADDR tem = (CORE_ADDR) addr + baseaddr;
  tem = gdbarch_adjust_dwarf2_addr (objfile->arch (), tem);
  return (unrelocated_addr) (tem - baseaddr);
}

/* See read.h.  */

CORE_ADDR
dwarf2_per_objfile::relocate (unrelocated_addr addr)
{
  CORE_ADDR baseaddr = objfile->text_section_offset ();
  CORE_ADDR tem = (CORE_ADDR) addr + baseaddr;
  return gdbarch_adjust_dwarf2_addr (objfile->arch (), tem);
}

/* Hash function for line_header_hash.  */

static hashval_t
line_header_hash (const struct line_header *ofs)
{
  return to_underlying (ofs->sect_off) ^ ofs->offset_in_dwz;
}

/* Hash function for htab_create_alloc_ex for line_header_hash.  */

static hashval_t
line_header_hash_voidp (const void *item)
{
  const struct line_header *ofs = (const struct line_header *) item;

  return line_header_hash (ofs);
}

/* Equality function for line_header_hash.  */

static int
line_header_eq_voidp (const void *item_lhs, const void *item_rhs)
{
  const struct line_header *ofs_lhs = (const struct line_header *) item_lhs;
  const struct line_header *ofs_rhs = (const struct line_header *) item_rhs;

  return (ofs_lhs->sect_off == ofs_rhs->sect_off
	  && ofs_lhs->offset_in_dwz == ofs_rhs->offset_in_dwz);
}

/* See declaration.  */

dwarf2_per_bfd::dwarf2_per_bfd (bfd *obfd, const dwarf2_debug_sections *names,
				bool can_copy_)
  : obfd (obfd),
    can_copy (can_copy_)
{
  if (names == NULL)
    names = &dwarf2_elf_names;

  for (asection *sec = obfd->sections; sec != NULL; sec = sec->next)
    locate_sections (obfd, sec, *names);
}

dwarf2_per_bfd::~dwarf2_per_bfd ()
{
  /* Data from the per-BFD may be needed when finalizing the cooked
     index table, so wait here while this happens.  */
  if (index_table != nullptr)
    index_table->wait_completely ();

  for (auto &per_cu : all_units)
    {
      per_cu->imported_symtabs_free ();
      per_cu->free_cached_file_names ();
    }

  /* Everything else should be on this->obstack.  */
}

/* See read.h.  */

void
dwarf2_per_objfile::remove_all_cus ()
{
  gdb_assert (!queue.has_value ());

  m_dwarf2_cus.clear ();
}

/* A helper class that calls free_cached_comp_units on
   destruction.  */

class free_cached_comp_units
{
public:

  explicit free_cached_comp_units (dwarf2_per_objfile *per_objfile)
    : m_per_objfile (per_objfile)
  {
  }

  ~free_cached_comp_units ()
  {
    m_per_objfile->remove_all_cus ();
  }

  DISABLE_COPY_AND_ASSIGN (free_cached_comp_units);

private:

  dwarf2_per_objfile *m_per_objfile;
};

/* See read.h.  */

bool
dwarf2_per_objfile::symtab_set_p (const dwarf2_per_cu_data *per_cu) const
{
  if (per_cu->index < this->m_symtabs.size ())
    return this->m_symtabs[per_cu->index] != nullptr;
  return false;
}

/* See read.h.  */

compunit_symtab *
dwarf2_per_objfile::get_symtab (const dwarf2_per_cu_data *per_cu) const
{
  if (per_cu->index < this->m_symtabs.size ())
    return this->m_symtabs[per_cu->index];
  return nullptr;
}

/* See read.h.  */

void
dwarf2_per_objfile::set_symtab (const dwarf2_per_cu_data *per_cu,
				compunit_symtab *symtab)
{
  if (per_cu->index >= this->m_symtabs.size ())
    this->m_symtabs.resize (per_cu->index + 1);
  gdb_assert (this->m_symtabs[per_cu->index] == nullptr);
  this->m_symtabs[per_cu->index] = symtab;
}

/* Helper function for dwarf2_initialize_objfile that creates the
   per-BFD object.  */

static bool
dwarf2_has_info (struct objfile *objfile,
		 const struct dwarf2_debug_sections *names,
		 bool can_copy)
{
  if (objfile->flags & OBJF_READNEVER)
    return false;

  dwarf2_per_objfile *per_objfile = get_dwarf2_per_objfile (objfile);

  if (per_objfile == NULL)
    {
      dwarf2_per_bfd *per_bfd;

      /* We can share a "dwarf2_per_bfd" with other objfiles if the
	 BFD doesn't require relocations.

	 We don't share with objfiles for which -readnow was requested,
	 because it would complicate things when loading the same BFD with
	 -readnow and then without -readnow.  */
      if (!gdb_bfd_requires_relocations (objfile->obfd.get ())
	  && (objfile->flags & OBJF_READNOW) == 0)
	{
	  /* See if one has been created for this BFD yet.  */
	  per_bfd = dwarf2_per_bfd_bfd_data_key.get (objfile->obfd.get ());

	  if (per_bfd == nullptr)
	    {
	      /* No, create it now.  */
	      per_bfd = new dwarf2_per_bfd (objfile->obfd.get (), names,
					    can_copy);
	      dwarf2_per_bfd_bfd_data_key.set (objfile->obfd.get (), per_bfd);
	    }
	}
      else
	{
	  /* No sharing possible, create one specifically for this objfile.  */
	  per_bfd = new dwarf2_per_bfd (objfile->obfd.get (), names, can_copy);
	  dwarf2_per_bfd_objfile_data_key.set (objfile, per_bfd);
	}

      per_objfile = dwarf2_objfile_data_key.emplace (objfile, objfile, per_bfd);
    }

  return (!per_objfile->per_bfd->info.is_virtual
	  && per_objfile->per_bfd->info.s.section != NULL
	  && !per_objfile->per_bfd->abbrev.is_virtual
	  && per_objfile->per_bfd->abbrev.s.section != NULL);
}

/* See declaration.  */

void
dwarf2_per_bfd::locate_sections (bfd *abfd, asection *sectp,
				 const dwarf2_debug_sections &names)
{
  flagword aflag = bfd_section_flags (sectp);

  if ((aflag & SEC_HAS_CONTENTS) == 0)
    {
    }
  else if (elf_section_data (sectp)->this_hdr.sh_size
	   > bfd_get_file_size (abfd))
    {
      bfd_size_type size = elf_section_data (sectp)->this_hdr.sh_size;
      warning (_("Discarding section %s which has a section size (%s"
		 ") larger than the file size [in module %s]"),
	       bfd_section_name (sectp), phex_nz (size, sizeof (size)),
	       bfd_get_filename (abfd));
    }
  else if (names.info.matches (sectp->name))
    {
      this->info.s.section = sectp;
      this->info.size = bfd_section_size (sectp);
    }
  else if (names.abbrev.matches (sectp->name))
    {
      this->abbrev.s.section = sectp;
      this->abbrev.size = bfd_section_size (sectp);
    }
  else if (names.line.matches (sectp->name))
    {
      this->line.s.section = sectp;
      this->line.size = bfd_section_size (sectp);
    }
  else if (names.loc.matches (sectp->name))
    {
      this->loc.s.section = sectp;
      this->loc.size = bfd_section_size (sectp);
    }
  else if (names.loclists.matches (sectp->name))
    {
      this->loclists.s.section = sectp;
      this->loclists.size = bfd_section_size (sectp);
    }
  else if (names.macinfo.matches (sectp->name))
    {
      this->macinfo.s.section = sectp;
      this->macinfo.size = bfd_section_size (sectp);
    }
  else if (names.macro.matches (sectp->name))
    {
      this->macro.s.section = sectp;
      this->macro.size = bfd_section_size (sectp);
    }
  else if (names.str.matches (sectp->name))
    {
      this->str.s.section = sectp;
      this->str.size = bfd_section_size (sectp);
    }
  else if (names.str_offsets.matches (sectp->name))
    {
      this->str_offsets.s.section = sectp;
      this->str_offsets.size = bfd_section_size (sectp);
    }
  else if (names.line_str.matches (sectp->name))
    {
      this->line_str.s.section = sectp;
      this->line_str.size = bfd_section_size (sectp);
    }
  else if (names.addr.matches (sectp->name))
    {
      this->addr.s.section = sectp;
      this->addr.size = bfd_section_size (sectp);
    }
  else if (names.frame.matches (sectp->name))
    {
      this->frame.s.section = sectp;
      this->frame.size = bfd_section_size (sectp);
    }
  else if (names.eh_frame.matches (sectp->name))
    {
      this->eh_frame.s.section = sectp;
      this->eh_frame.size = bfd_section_size (sectp);
    }
  else if (names.ranges.matches (sectp->name))
    {
      this->ranges.s.section = sectp;
      this->ranges.size = bfd_section_size (sectp);
    }
  else if (names.rnglists.matches (sectp->name))
    {
      this->rnglists.s.section = sectp;
      this->rnglists.size = bfd_section_size (sectp);
    }
  else if (names.types.matches (sectp->name))
    {
      struct dwarf2_section_info type_section;

      memset (&type_section, 0, sizeof (type_section));
      type_section.s.section = sectp;
      type_section.size = bfd_section_size (sectp);

      this->types.push_back (type_section);
    }
  else if (names.gdb_index.matches (sectp->name))
    {
      this->gdb_index.s.section = sectp;
      this->gdb_index.size = bfd_section_size (sectp);
    }
  else if (names.debug_names.matches (sectp->name))
    {
      this->debug_names.s.section = sectp;
      this->debug_names.size = bfd_section_size (sectp);
    }
  else if (names.debug_aranges.matches (sectp->name))
    {
      this->debug_aranges.s.section = sectp;
      this->debug_aranges.size = bfd_section_size (sectp);
    }

  if ((bfd_section_flags (sectp) & (SEC_LOAD | SEC_ALLOC))
      && bfd_section_vma (sectp) == 0)
    this->has_section_at_zero = true;
}

/* Fill in SECTP, BUFP and SIZEP with section info, given OBJFILE and
   SECTION_NAME.  */

void
dwarf2_get_section_info (struct objfile *objfile,
			 enum dwarf2_section_enum sect,
			 asection **sectp, const gdb_byte **bufp,
			 bfd_size_type *sizep)
{
  dwarf2_per_objfile *per_objfile = get_dwarf2_per_objfile (objfile);
  struct dwarf2_section_info *info;

  /* We may see an objfile without any DWARF, in which case we just
     return nothing.  */
  if (per_objfile == NULL)
    {
      *sectp = NULL;
      *bufp = NULL;
      *sizep = 0;
      return;
    }
  switch (sect)
    {
    case DWARF2_DEBUG_FRAME:
      info = &per_objfile->per_bfd->frame;
      break;
    case DWARF2_EH_FRAME:
      info = &per_objfile->per_bfd->eh_frame;
      break;
    default:
      gdb_assert_not_reached ("unexpected section");
    }

  info->read (objfile);

  *sectp = info->get_bfd_section ();
  *bufp = info->buffer;
  *sizep = info->size;
}

/* See dwarf2/read.h.  */

void
dwarf2_per_bfd::map_info_sections (struct objfile *objfile)
{
  info.read (objfile);
  abbrev.read (objfile);
  line.read (objfile);
  str.read (objfile);
  str_offsets.read (objfile);
  line_str.read (objfile);
  ranges.read (objfile);
  rnglists.read (objfile);
  addr.read (objfile);
  debug_aranges.read (objfile);

  for (auto &section : types)
    section.read (objfile);
}


/* DWARF quick_symbol_functions support.  */

/* TUs can share .debug_line entries, and there can be a lot more TUs than
   unique line tables, so we maintain a separate table of all .debug_line
   derived entries to support the sharing.
   All the quick functions need is the list of file names.  We discard the
   line_header when we're done and don't need to record it here.  */
struct quick_file_names
{
  /* The data used to construct the hash key.  */
  struct stmt_list_hash hash;

  /* The number of entries in file_names, real_names.  */
  unsigned int num_file_names;

  /* The CU directory, as given by DW_AT_comp_dir.  May be
     nullptr.  */
  const char *comp_dir;

  /* The file names from the line table, after being run through
     file_full_name.  */
  const char **file_names;

  /* The file names from the line table after being run through
     gdb_realpath.  These are computed lazily.  */
  const char **real_names;
};

/* With OBJF_READNOW, the DWARF reader expands all CUs immediately.
   It's handy in this case to have an empty implementation of the
   quick symbol functions, to avoid special cases in the rest of the
   code.  */

struct readnow_functions : public dwarf2_base_index_functions
{
  void dump (struct objfile *objfile) override
  {
  }

  bool expand_symtabs_matching
    (struct objfile *objfile,
     gdb::function_view<expand_symtabs_file_matcher_ftype> file_matcher,
     const lookup_name_info *lookup_name,
     gdb::function_view<expand_symtabs_symbol_matcher_ftype> symbol_matcher,
     gdb::function_view<expand_symtabs_exp_notify_ftype> expansion_notify,
     block_search_flags search_flags,
     domain_enum domain,
     enum search_domain kind) override
  {
    return true;
  }
};

/* Utility hash function for a stmt_list_hash.  */

static hashval_t
hash_stmt_list_entry (const struct stmt_list_hash *stmt_list_hash)
{
  hashval_t v = 0;

  if (stmt_list_hash->dwo_unit != NULL)
    v += (uintptr_t) stmt_list_hash->dwo_unit->dwo_file;
  v += to_underlying (stmt_list_hash->line_sect_off);
  return v;
}

/* Utility equality function for a stmt_list_hash.  */

static int
eq_stmt_list_entry (const struct stmt_list_hash *lhs,
		    const struct stmt_list_hash *rhs)
{
  if ((lhs->dwo_unit != NULL) != (rhs->dwo_unit != NULL))
    return 0;
  if (lhs->dwo_unit != NULL
      && lhs->dwo_unit->dwo_file != rhs->dwo_unit->dwo_file)
    return 0;

  return lhs->line_sect_off == rhs->line_sect_off;
}

/* Hash function for a quick_file_names.  */

static hashval_t
hash_file_name_entry (const void *e)
{
  const struct quick_file_names *file_data
    = (const struct quick_file_names *) e;

  return hash_stmt_list_entry (&file_data->hash);
}

/* Equality function for a quick_file_names.  */

static int
eq_file_name_entry (const void *a, const void *b)
{
  const struct quick_file_names *ea = (const struct quick_file_names *) a;
  const struct quick_file_names *eb = (const struct quick_file_names *) b;

  return eq_stmt_list_entry (&ea->hash, &eb->hash);
}

/* See read.h.  */

htab_up
create_quick_file_names_table (unsigned int nr_initial_entries)
{
  return htab_up (htab_create_alloc (nr_initial_entries,
				     hash_file_name_entry, eq_file_name_entry,
				     nullptr, xcalloc, xfree));
}

/* Read in CU (dwarf2_cu object) for PER_CU in the context of PER_OBJFILE.  This
   function is unrelated to symtabs, symtab would have to be created afterwards.
   You should call age_cached_comp_units after processing the CU.  */

static dwarf2_cu *
load_cu (dwarf2_per_cu_data *per_cu, dwarf2_per_objfile *per_objfile,
	 bool skip_partial)
{
  if (per_cu->is_debug_types)
    load_full_type_unit (per_cu, per_objfile);
  else
    load_full_comp_unit (per_cu, per_objfile, per_objfile->get_cu (per_cu),
			 skip_partial, language_minimal);

  dwarf2_cu *cu = per_objfile->get_cu (per_cu);
  if (cu == nullptr)
    return nullptr;  /* Dummy CU.  */

  dwarf2_find_base_address (cu->dies, cu);

  return cu;
}

/* Read in the symbols for PER_CU in the context of PER_OBJFILE.  */

static void
dw2_do_instantiate_symtab (dwarf2_per_cu_data *per_cu,
			   dwarf2_per_objfile *per_objfile, bool skip_partial)
{
  {
    /* The destructor of dwarf2_queue_guard frees any entries left on
       the queue.  After this point we're guaranteed to leave this function
       with the dwarf queue empty.  */
    dwarf2_queue_guard q_guard (per_objfile);

    if (!per_objfile->symtab_set_p (per_cu))
      {
	queue_comp_unit (per_cu, per_objfile, language_minimal);
	dwarf2_cu *cu = load_cu (per_cu, per_objfile, skip_partial);

	/* If we just loaded a CU from a DWO, and we're working with an index
	   that may badly handle TUs, load all the TUs in that DWO as well.
	   http://sourceware.org/bugzilla/show_bug.cgi?id=15021  */
	if (!per_cu->is_debug_types
	    && cu != NULL
	    && cu->dwo_unit != NULL
	    && per_objfile->per_bfd->index_table != NULL
	    && !per_objfile->per_bfd->index_table->version_check ()
	    /* DWP files aren't supported yet.  */
	    && get_dwp_file (per_objfile) == NULL)
	  queue_and_load_all_dwo_tus (cu);
      }

    process_queue (per_objfile);
  }

  /* Age the cache, releasing compilation units that have not
     been used recently.  */
  per_objfile->age_comp_units ();
}

/* Ensure that the symbols for PER_CU have been read in.  DWARF2_PER_OBJFILE is
   the per-objfile for which this symtab is instantiated.

   Returns the resulting symbol table.  */

static struct compunit_symtab *
dw2_instantiate_symtab (dwarf2_per_cu_data *per_cu,
			dwarf2_per_objfile *per_objfile,
			bool skip_partial)
{
  if (!per_objfile->symtab_set_p (per_cu))
    {
      free_cached_comp_units freer (per_objfile);
      scoped_restore decrementer = increment_reading_symtab ();
      dw2_do_instantiate_symtab (per_cu, per_objfile, skip_partial);
      process_cu_includes (per_objfile);
    }

  return per_objfile->get_symtab (per_cu);
}

/* See read.h.  */

dwarf2_per_cu_data_up
dwarf2_per_bfd::allocate_per_cu ()
{
  dwarf2_per_cu_data_up result (new dwarf2_per_cu_data);
  result->per_bfd = this;
  result->index = all_units.size ();
  return result;
}

/* See read.h.  */

signatured_type_up
dwarf2_per_bfd::allocate_signatured_type (ULONGEST signature)
{
  signatured_type_up result (new signatured_type (signature));
  result->per_bfd = this;
  result->index = all_units.size ();
  result->is_debug_types = true;
  tu_stats.nr_tus++;
  return result;
}

/* See read.h.  */

dwarf2_per_cu_data_up
create_cu_from_index_list (dwarf2_per_bfd *per_bfd,
			   struct dwarf2_section_info *section,
			   int is_dwz,
			   sect_offset sect_off, ULONGEST length)
{
  dwarf2_per_cu_data_up the_cu = per_bfd->allocate_per_cu ();
  the_cu->sect_off = sect_off;
  the_cu->set_length (length);
  the_cu->section = section;
  the_cu->is_dwz = is_dwz;
  return the_cu;
}

/* die_reader_func for dw2_get_file_names.  */

static void
dw2_get_file_names_reader (const struct die_reader_specs *reader,
			   struct die_info *comp_unit_die)
{
  struct dwarf2_cu *cu = reader->cu;
  struct dwarf2_per_cu_data *this_cu = cu->per_cu;
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct dwarf2_per_cu_data *lh_cu;
  struct attribute *attr;
  void **slot;
  struct quick_file_names *qfn;

  gdb_assert (! this_cu->is_debug_types);

  this_cu->files_read = true;
  /* Our callers never want to match partial units -- instead they
     will match the enclosing full CU.  */
  if (comp_unit_die->tag == DW_TAG_partial_unit)
    return;

  lh_cu = this_cu;
  slot = NULL;

  line_header_up lh;
  sect_offset line_offset {};

  file_and_directory &fnd = find_file_and_directory (comp_unit_die, cu);

  attr = dwarf2_attr (comp_unit_die, DW_AT_stmt_list, cu);
  if (attr != nullptr && attr->form_is_unsigned ())
    {
      struct quick_file_names find_entry;

      line_offset = (sect_offset) attr->as_unsigned ();

      /* We may have already read in this line header (TU line header sharing).
	 If we have we're done.  */
      find_entry.hash.dwo_unit = cu->dwo_unit;
      find_entry.hash.line_sect_off = line_offset;
      slot = htab_find_slot (per_objfile->per_bfd->quick_file_names_table.get (),
			     &find_entry, INSERT);
      if (*slot != NULL)
	{
	  lh_cu->file_names = (struct quick_file_names *) *slot;
	  return;
	}

      lh = dwarf_decode_line_header (line_offset, cu, fnd.get_comp_dir ());
    }

  int offset = 0;
  if (!fnd.is_unknown ())
    ++offset;
  else if (lh == nullptr)
    return;

  qfn = XOBNEW (&per_objfile->per_bfd->obstack, struct quick_file_names);
  qfn->hash.dwo_unit = cu->dwo_unit;
  qfn->hash.line_sect_off = line_offset;
  /* There may not be a DW_AT_stmt_list.  */
  if (slot != nullptr)
    *slot = qfn;

  std::vector<const char *> include_names;
  if (lh != nullptr)
    {
      for (const auto &entry : lh->file_names ())
	{
	  std::string name_holder;
	  const char *include_name =
	    compute_include_file_name (lh.get (), entry, fnd, name_holder);
	  if (include_name != nullptr)
	    {
	      include_name = per_objfile->objfile->intern (include_name);
	      include_names.push_back (include_name);
	    }
	}
    }

  qfn->num_file_names = offset + include_names.size ();
  qfn->comp_dir = fnd.intern_comp_dir (per_objfile->objfile);
  qfn->file_names =
    XOBNEWVEC (&per_objfile->per_bfd->obstack, const char *,
	       qfn->num_file_names);
  if (offset != 0)
    qfn->file_names[0] = per_objfile->objfile->intern (fnd.get_name ());

  if (!include_names.empty ())
    memcpy (&qfn->file_names[offset], include_names.data (),
	    include_names.size () * sizeof (const char *));

  qfn->real_names = NULL;

  lh_cu->file_names = qfn;
}

/* A helper for the "quick" functions which attempts to read the line
   table for THIS_CU.  */

static struct quick_file_names *
dw2_get_file_names (dwarf2_per_cu_data *this_cu,
		    dwarf2_per_objfile *per_objfile)
{
  /* This should never be called for TUs.  */
  gdb_assert (! this_cu->is_debug_types);

  if (this_cu->files_read)
    return this_cu->file_names;

  cutu_reader reader (this_cu, per_objfile);
  if (!reader.dummy_p)
    dw2_get_file_names_reader (&reader, reader.comp_unit_die);

  return this_cu->file_names;
}

/* A helper for the "quick" functions which computes and caches the
   real path for a given file name from the line table.  */

static const char *
dw2_get_real_path (dwarf2_per_objfile *per_objfile,
		   struct quick_file_names *qfn, int index)
{
  if (qfn->real_names == NULL)
    qfn->real_names = OBSTACK_CALLOC (&per_objfile->per_bfd->obstack,
				      qfn->num_file_names, const char *);

  if (qfn->real_names[index] == NULL)
    {
      const char *dirname = nullptr;

      if (!IS_ABSOLUTE_PATH (qfn->file_names[index]))
	dirname = qfn->comp_dir;

      gdb::unique_xmalloc_ptr<char> fullname;
      fullname = find_source_or_rewrite (qfn->file_names[index], dirname);

      qfn->real_names[index] = fullname.release ();
    }

  return qfn->real_names[index];
}

struct symtab *
dwarf2_base_index_functions::find_last_source_symtab (struct objfile *objfile)
{
  dwarf2_per_objfile *per_objfile = get_dwarf2_per_objfile (objfile);
  dwarf2_per_cu_data *dwarf_cu
    = per_objfile->per_bfd->all_units.back ().get ();
  compunit_symtab *cust = dw2_instantiate_symtab (dwarf_cu, per_objfile, false);

  if (cust == NULL)
    return NULL;

  return cust->primary_filetab ();
}

/* See read.h.  */

void
dwarf2_per_cu_data::free_cached_file_names ()
{
  if (fnd != nullptr)
    fnd->forget_fullname ();

  if (per_bfd == nullptr)
    return;

  struct quick_file_names *file_data = file_names;
  if (file_data != nullptr && file_data->real_names != nullptr)
    {
      for (int i = 0; i < file_data->num_file_names; ++i)
	{
	  xfree ((void *) file_data->real_names[i]);
	  file_data->real_names[i] = nullptr;
	}
    }
}

void
dwarf2_base_index_functions::forget_cached_source_info
     (struct objfile *objfile)
{
  dwarf2_per_objfile *per_objfile = get_dwarf2_per_objfile (objfile);

  for (auto &per_cu : per_objfile->per_bfd->all_units)
    per_cu->free_cached_file_names ();
}

void
dwarf2_base_index_functions::print_stats (struct objfile *objfile,
					  bool print_bcache)
{
  if (print_bcache)
    return;

  dwarf2_per_objfile *per_objfile = get_dwarf2_per_objfile (objfile);
  int total = per_objfile->per_bfd->all_units.size ();
  int count = 0;

  for (int i = 0; i < total; ++i)
    {
      dwarf2_per_cu_data *per_cu = per_objfile->per_bfd->get_cu (i);

      if (!per_objfile->symtab_set_p (per_cu))
	++count;
    }
  gdb_printf (_("  Number of read CUs: %d\n"), total - count);
  gdb_printf (_("  Number of unread CUs: %d\n"), count);
}

void
dwarf2_base_index_functions::expand_all_symtabs (struct objfile *objfile)
{
  dwarf2_per_objfile *per_objfile = get_dwarf2_per_objfile (objfile);
  int total_units = per_objfile->per_bfd->all_units.size ();

  for (int i = 0; i < total_units; ++i)
    {
      dwarf2_per_cu_data *per_cu = per_objfile->per_bfd->get_cu (i);

      /* We don't want to directly expand a partial CU, because if we
	 read it with the wrong language, then assertion failures can
	 be triggered later on.  See PR symtab/23010.  So, tell
	 dw2_instantiate_symtab to skip partial CUs -- any important
	 partial CU will be read via DW_TAG_imported_unit anyway.  */
      dw2_instantiate_symtab (per_cu, per_objfile, true);
    }
}


/* Starting from a search name, return the string that finds the upper
   bound of all strings that start with SEARCH_NAME in a sorted name
   list.  Returns the empty string to indicate that the upper bound is
   the end of the list.  */

static std::string
make_sort_after_prefix_name (const char *search_name)
{
  /* When looking to complete "func", we find the upper bound of all
     symbols that start with "func" by looking for where we'd insert
     the closest string that would follow "func" in lexicographical
     order.  Usually, that's "func"-with-last-character-incremented,
     i.e. "fund".  Mind non-ASCII characters, though.  Usually those
     will be UTF-8 multi-byte sequences, but we can't be certain.
     Especially mind the 0xff character, which is a valid character in
     non-UTF-8 source character sets (e.g. Latin1 'ÿ'), and we can't
     rule out compilers allowing it in identifiers.  Note that
     conveniently, strcmp/strcasecmp are specified to compare
     characters interpreted as unsigned char.  So what we do is treat
     the whole string as a base 256 number composed of a sequence of
     base 256 "digits" and add 1 to it.  I.e., adding 1 to 0xff wraps
     to 0, and carries 1 to the following more-significant position.
     If the very first character in SEARCH_NAME ends up incremented
     and carries/overflows, then the upper bound is the end of the
     list.  The string after the empty string is also the empty
     string.

     Some examples of this operation:

       SEARCH_NAME  => "+1" RESULT

       "abc"              => "abd"
       "ab\xff"           => "ac"
       "\xff" "a" "\xff"  => "\xff" "b"
       "\xff"             => ""
       "\xff\xff"         => ""
       ""                 => ""

     Then, with these symbols for example:

      func
      func1
      fund

     completing "func" looks for symbols between "func" and
     "func"-with-last-character-incremented, i.e. "fund" (exclusive),
     which finds "func" and "func1", but not "fund".

     And with:

      funcÿ     (Latin1 'ÿ' [0xff])
      funcÿ1
      fund

     completing "funcÿ" looks for symbols between "funcÿ" and "fund"
     (exclusive), which finds "funcÿ" and "funcÿ1", but not "fund".

     And with:

      ÿÿ        (Latin1 'ÿ' [0xff])
      ÿÿ1

     completing "ÿ" or "ÿÿ" looks for symbols between between "ÿÿ" and
     the end of the list.
  */
  std::string after = search_name;
  while (!after.empty () && (unsigned char) after.back () == 0xff)
    after.pop_back ();
  if (!after.empty ())
    after.back () = (unsigned char) after.back () + 1;
  return after;
}

/* See declaration.  */

std::pair<std::vector<name_component>::const_iterator,
	  std::vector<name_component>::const_iterator>
mapped_index_base::find_name_components_bounds
  (const lookup_name_info &lookup_name_without_params, language lang,
   dwarf2_per_objfile *per_objfile) const
{
  auto *name_cmp
    = this->name_components_casing == case_sensitive_on ? strcmp : strcasecmp;

  const char *lang_name
    = lookup_name_without_params.language_lookup_name (lang);

  /* Comparison function object for lower_bound that matches against a
     given symbol name.  */
  auto lookup_compare_lower = [&] (const name_component &elem,
				   const char *name)
    {
      const char *elem_qualified = this->symbol_name_at (elem.idx, per_objfile);
      const char *elem_name = elem_qualified + elem.name_offset;
      return name_cmp (elem_name, name) < 0;
    };

  /* Comparison function object for upper_bound that matches against a
     given symbol name.  */
  auto lookup_compare_upper = [&] (const char *name,
				   const name_component &elem)
    {
      const char *elem_qualified = this->symbol_name_at (elem.idx, per_objfile);
      const char *elem_name = elem_qualified + elem.name_offset;
      return name_cmp (name, elem_name) < 0;
    };

  auto begin = this->name_components.begin ();
  auto end = this->name_components.end ();

  /* Find the lower bound.  */
  auto lower = [&] ()
    {
      if (lookup_name_without_params.completion_mode () && lang_name[0] == '\0')
	return begin;
      else
	return std::lower_bound (begin, end, lang_name, lookup_compare_lower);
    } ();

  /* Find the upper bound.  */
  auto upper = [&] ()
    {
      if (lookup_name_without_params.completion_mode ())
	{
	  /* In completion mode, we want UPPER to point past all
	     symbols names that have the same prefix.  I.e., with
	     these symbols, and completing "func":

	      function        << lower bound
	      function1
	      other_function  << upper bound

	     We find the upper bound by looking for the insertion
	     point of "func"-with-last-character-incremented,
	     i.e. "fund".  */
	  std::string after = make_sort_after_prefix_name (lang_name);
	  if (after.empty ())
	    return end;
	  return std::lower_bound (lower, end, after.c_str (),
				   lookup_compare_lower);
	}
      else
	return std::upper_bound (lower, end, lang_name, lookup_compare_upper);
    } ();

  return {lower, upper};
}

/* See declaration.  */

void
mapped_index_base::build_name_components (dwarf2_per_objfile *per_objfile)
{
  if (!this->name_components.empty ())
    return;

  this->name_components_casing = case_sensitivity;
  auto *name_cmp
    = this->name_components_casing == case_sensitive_on ? strcmp : strcasecmp;

  /* The code below only knows how to break apart components of C++
     symbol names (and other languages that use '::' as
     namespace/module separator) and Ada symbol names.  */
  auto count = this->symbol_name_count ();
  for (offset_type idx = 0; idx < count; idx++)
    {
      if (this->symbol_name_slot_invalid (idx))
	continue;

      const char *name = this->symbol_name_at (idx, per_objfile);

      /* Add each name component to the name component table.  */
      unsigned int previous_len = 0;

      if (strstr (name, "::") != nullptr)
	{
	  for (unsigned int current_len = cp_find_first_component (name);
	       name[current_len] != '\0';
	       current_len += cp_find_first_component (name + current_len))
	    {
	      gdb_assert (name[current_len] == ':');
	      this->name_components.push_back ({previous_len, idx});
	      /* Skip the '::'.  */
	      current_len += 2;
	      previous_len = current_len;
	    }
	}
      else
	{
	  /* Handle the Ada encoded (aka mangled) form here.  */
	  for (const char *iter = strstr (name, "__");
	       iter != nullptr;
	       iter = strstr (iter, "__"))
	    {
	      this->name_components.push_back ({previous_len, idx});
	      iter += 2;
	      previous_len = iter - name;
	    }
	}

      this->name_components.push_back ({previous_len, idx});
    }

  /* Sort name_components elements by name.  */
  auto name_comp_compare = [&] (const name_component &left,
				const name_component &right)
    {
      const char *left_qualified
	= this->symbol_name_at (left.idx, per_objfile);
      const char *right_qualified
	= this->symbol_name_at (right.idx, per_objfile);

      const char *left_name = left_qualified + left.name_offset;
      const char *right_name = right_qualified + right.name_offset;

      return name_cmp (left_name, right_name) < 0;
    };

  std::sort (this->name_components.begin (),
	     this->name_components.end (),
	     name_comp_compare);
}

/* See read.h.  */

bool
dw2_expand_symtabs_matching_symbol
  (mapped_index_base &index,
   const lookup_name_info &lookup_name_in,
   gdb::function_view<expand_symtabs_symbol_matcher_ftype> symbol_matcher,
   gdb::function_view<bool (offset_type)> match_callback,
   dwarf2_per_objfile *per_objfile)
{
  lookup_name_info lookup_name_without_params
    = lookup_name_in.make_ignore_params ();

  /* Build the symbol name component sorted vector, if we haven't
     yet.  */
  index.build_name_components (per_objfile);

  /* The same symbol may appear more than once in the range though.
     E.g., if we're looking for symbols that complete "w", and we have
     a symbol named "w1::w2", we'll find the two name components for
     that same symbol in the range.  To be sure we only call the
     callback once per symbol, we first collect the symbol name
     indexes that matched in a temporary vector and ignore
     duplicates.  */
  std::vector<offset_type> matches;

  struct name_and_matcher
  {
    symbol_name_matcher_ftype *matcher;
    const char *name;

    bool operator== (const name_and_matcher &other) const
    {
      return matcher == other.matcher && strcmp (name, other.name) == 0;
    }
  };

  /* A vector holding all the different symbol name matchers, for all
     languages.  */
  std::vector<name_and_matcher> matchers;

  for (int i = 0; i < nr_languages; i++)
    {
      enum language lang_e = (enum language) i;

      const language_defn *lang = language_def (lang_e);
      symbol_name_matcher_ftype *name_matcher
	= lang->get_symbol_name_matcher (lookup_name_without_params);

      name_and_matcher key {
	 name_matcher,
	 lookup_name_without_params.language_lookup_name (lang_e)
      };

      /* Don't insert the same comparison routine more than once.
	 Note that we do this linear walk.  This is not a problem in
	 practice because the number of supported languages is
	 low.  */
      if (std::find (matchers.begin (), matchers.end (), key)
	  != matchers.end ())
	continue;
      matchers.push_back (std::move (key));

      auto bounds
	= index.find_name_components_bounds (lookup_name_without_params,
					     lang_e, per_objfile);

      /* Now for each symbol name in range, check to see if we have a name
	 match, and if so, call the MATCH_CALLBACK callback.  */

      for (; bounds.first != bounds.second; ++bounds.first)
	{
	  const char *qualified
	    = index.symbol_name_at (bounds.first->idx, per_objfile);

	  if (!name_matcher (qualified, lookup_name_without_params, NULL)
	      || (symbol_matcher != NULL && !symbol_matcher (qualified)))
	    continue;

	  matches.push_back (bounds.first->idx);
	}
    }

  std::sort (matches.begin (), matches.end ());

  /* Finally call the callback, once per match.  */
  ULONGEST prev = -1;
  bool result = true;
  for (offset_type idx : matches)
    {
      if (prev != idx)
	{
	  if (!match_callback (idx))
	    {
	      result = false;
	      break;
	    }
	  prev = idx;
	}
    }

  /* Above we use a type wider than idx's for 'prev', since 0 and
     (offset_type)-1 are both possible values.  */
  static_assert (sizeof (prev) > sizeof (offset_type), "");

  return result;
}

#if GDB_SELF_TEST

namespace selftests { namespace dw2_expand_symtabs_matching {

/* A mock .gdb_index/.debug_names-like name index table, enough to
   exercise dw2_expand_symtabs_matching_symbol, which works with the
   mapped_index_base interface.  Builds an index from the symbol list
   passed as parameter to the constructor.  */
class mock_mapped_index : public mapped_index_base
{
public:
  mock_mapped_index (gdb::array_view<const char *> symbols)
    : m_symbol_table (symbols)
  {}

  DISABLE_COPY_AND_ASSIGN (mock_mapped_index);

  /* Return the number of names in the symbol table.  */
  size_t symbol_name_count () const override
  {
    return m_symbol_table.size ();
  }

  /* Get the name of the symbol at IDX in the symbol table.  */
  const char *symbol_name_at
    (offset_type idx, dwarf2_per_objfile *per_objfile) const override
  {
    return m_symbol_table[idx];
  }

  quick_symbol_functions_up make_quick_functions () const override
  {
    return nullptr;
  }

private:
  gdb::array_view<const char *> m_symbol_table;
};

/* Convenience function that converts a NULL pointer to a "<null>"
   string, to pass to print routines.  */

static const char *
string_or_null (const char *str)
{
  return str != NULL ? str : "<null>";
}

/* Check if a lookup_name_info built from
   NAME/MATCH_TYPE/COMPLETION_MODE matches the symbols in the mock
   index.  EXPECTED_LIST is the list of expected matches, in expected
   matching order.  If no match expected, then an empty list is
   specified.  Returns true on success.  On failure prints a warning
   indicating the file:line that failed, and returns false.  */

static bool
check_match (const char *file, int line,
	     mock_mapped_index &mock_index,
	     const char *name, symbol_name_match_type match_type,
	     bool completion_mode,
	     std::initializer_list<const char *> expected_list,
	     dwarf2_per_objfile *per_objfile)
{
  lookup_name_info lookup_name (name, match_type, completion_mode);

  bool matched = true;

  auto mismatch = [&] (const char *expected_str,
		       const char *got)
  {
    warning (_("%s:%d: match_type=%s, looking-for=\"%s\", "
	       "expected=\"%s\", got=\"%s\"\n"),
	     file, line,
	     (match_type == symbol_name_match_type::FULL
	      ? "FULL" : "WILD"),
	     name, string_or_null (expected_str), string_or_null (got));
    matched = false;
  };

  auto expected_it = expected_list.begin ();
  auto expected_end = expected_list.end ();

  dw2_expand_symtabs_matching_symbol (mock_index, lookup_name,
				      nullptr,
				      [&] (offset_type idx)
  {
    const char *matched_name = mock_index.symbol_name_at (idx, per_objfile);
    const char *expected_str
      = expected_it == expected_end ? NULL : *expected_it++;

    if (expected_str == NULL || strcmp (expected_str, matched_name) != 0)
      mismatch (expected_str, matched_name);
    return true;
  }, per_objfile);

  const char *expected_str
  = expected_it == expected_end ? NULL : *expected_it++;
  if (expected_str != NULL)
    mismatch (expected_str, NULL);

  return matched;
}

/* The symbols added to the mock mapped_index for testing (in
   canonical form).  */
static const char *test_symbols[] = {
  "function",
  "std::bar",
  "std::zfunction",
  "std::zfunction2",
  "w1::w2",
  "ns::foo<char*>",
  "ns::foo<int>",
  "ns::foo<long>",
  "ns2::tmpl<int>::foo2",
  "(anonymous namespace)::A::B::C",

  /* These are used to check that the increment-last-char in the
     matching algorithm for completion doesn't match "t1_fund" when
     completing "t1_func".  */
  "t1_func",
  "t1_func1",
  "t1_fund",
  "t1_fund1",

  /* A UTF-8 name with multi-byte sequences to make sure that
     cp-name-parser understands this as a single identifier ("função"
     is "function" in PT).  */
  (const char *)u8"u8função",

  /* Test a symbol name that ends with a 0xff character, which is a
     valid character in non-UTF-8 source character sets (e.g. Latin1
     'ÿ'), and we can't rule out compilers allowing it in identifiers.
     We test this because the completion algorithm finds the upper
     bound of symbols by looking for the insertion point of
     "func"-with-last-character-incremented, i.e. "fund", and adding 1
     to 0xff should wraparound and carry to the previous character.
     See comments in make_sort_after_prefix_name.  */
  "yfunc\377",

  /* Some more symbols with \377 (0xff).  See above.  */
  "\377",
  "\377\377123",

  /* A name with all sorts of complications.  Starts with "z" to make
     it easier for the completion tests below.  */
#define Z_SYM_NAME \
  "z::std::tuple<(anonymous namespace)::ui*, std::bar<(anonymous namespace)::ui> >" \
    "::tuple<(anonymous namespace)::ui*, " \
    "std::default_delete<(anonymous namespace)::ui>, void>"

  Z_SYM_NAME
};

/* Returns true if the mapped_index_base::find_name_component_bounds
   method finds EXPECTED_SYMS in INDEX when looking for SEARCH_NAME,
   in completion mode.  */

static bool
check_find_bounds_finds (mapped_index_base &index,
			 const char *search_name,
			 gdb::array_view<const char *> expected_syms,
			 dwarf2_per_objfile *per_objfile)
{
  lookup_name_info lookup_name (search_name,
				symbol_name_match_type::FULL, true);

  auto bounds = index.find_name_components_bounds (lookup_name,
						   language_cplus,
						   per_objfile);

  size_t distance = std::distance (bounds.first, bounds.second);
  if (distance != expected_syms.size ())
    return false;

  for (size_t exp_elem = 0; exp_elem < distance; exp_elem++)
    {
      auto nc_elem = bounds.first + exp_elem;
      const char *qualified = index.symbol_name_at (nc_elem->idx, per_objfile);
      if (strcmp (qualified, expected_syms[exp_elem]) != 0)
	return false;
    }

  return true;
}

/* Test the lower-level mapped_index::find_name_component_bounds
   method.  */

static void
test_mapped_index_find_name_component_bounds ()
{
  mock_mapped_index mock_index (test_symbols);

  mock_index.build_name_components (NULL /* per_objfile */);

  /* Test the lower-level mapped_index::find_name_component_bounds
     method in completion mode.  */
  {
    static const char *expected_syms[] = {
      "t1_func",
      "t1_func1",
    };

    SELF_CHECK (check_find_bounds_finds
		  (mock_index, "t1_func", expected_syms,
		   NULL /* per_objfile */));
  }

  /* Check that the increment-last-char in the name matching algorithm
     for completion doesn't get confused with Ansi1 'ÿ' / 0xff.  See
     make_sort_after_prefix_name.  */
  {
    static const char *expected_syms1[] = {
      "\377",
      "\377\377123",
    };
    SELF_CHECK (check_find_bounds_finds
		  (mock_index, "\377", expected_syms1, NULL /* per_objfile */));

    static const char *expected_syms2[] = {
      "\377\377123",
    };
    SELF_CHECK (check_find_bounds_finds
		  (mock_index, "\377\377", expected_syms2,
		   NULL /* per_objfile */));
  }
}

/* Test dw2_expand_symtabs_matching_symbol.  */

static void
test_dw2_expand_symtabs_matching_symbol ()
{
  mock_mapped_index mock_index (test_symbols);

  /* We let all tests run until the end even if some fails, for debug
     convenience.  */
  bool any_mismatch = false;

  /* Create the expected symbols list (an initializer_list).  Needed
     because lists have commas, and we need to pass them to CHECK,
     which is a macro.  */
#define EXPECT(...) { __VA_ARGS__ }

  /* Wrapper for check_match that passes down the current
     __FILE__/__LINE__.  */
#define CHECK_MATCH(NAME, MATCH_TYPE, COMPLETION_MODE, EXPECTED_LIST)	\
  any_mismatch |= !check_match (__FILE__, __LINE__,			\
				mock_index,				\
				NAME, MATCH_TYPE, COMPLETION_MODE,	\
				EXPECTED_LIST, NULL)

  /* Identity checks.  */
  for (const char *sym : test_symbols)
    {
      /* Should be able to match all existing symbols.  */
      CHECK_MATCH (sym, symbol_name_match_type::FULL, false,
		   EXPECT (sym));

      /* Should be able to match all existing symbols with
	 parameters.  */
      std::string with_params = std::string (sym) + "(int)";
      CHECK_MATCH (with_params.c_str (), symbol_name_match_type::FULL, false,
		   EXPECT (sym));

      /* Should be able to match all existing symbols with
	 parameters and qualifiers.  */
      with_params = std::string (sym) + " ( int ) const";
      CHECK_MATCH (with_params.c_str (), symbol_name_match_type::FULL, false,
		   EXPECT (sym));

      /* This should really find sym, but cp-name-parser.y doesn't
	 know about lvalue/rvalue qualifiers yet.  */
      with_params = std::string (sym) + " ( int ) &&";
      CHECK_MATCH (with_params.c_str (), symbol_name_match_type::FULL, false,
		   {});
    }

  /* Check that the name matching algorithm for completion doesn't get
     confused with Latin1 'ÿ' / 0xff.  See
     make_sort_after_prefix_name.  */
  {
    static const char str[] = "\377";
    CHECK_MATCH (str, symbol_name_match_type::FULL, true,
		 EXPECT ("\377", "\377\377123"));
  }

  /* Check that the increment-last-char in the matching algorithm for
     completion doesn't match "t1_fund" when completing "t1_func".  */
  {
    static const char str[] = "t1_func";
    CHECK_MATCH (str, symbol_name_match_type::FULL, true,
		 EXPECT ("t1_func", "t1_func1"));
  }

  /* Check that completion mode works at each prefix of the expected
     symbol name.  */
  {
    static const char str[] = "function(int)";
    size_t len = strlen (str);
    std::string lookup;

    for (size_t i = 1; i < len; i++)
      {
	lookup.assign (str, i);
	CHECK_MATCH (lookup.c_str (), symbol_name_match_type::FULL, true,
		     EXPECT ("function"));
      }
  }

  /* While "w" is a prefix of both components, the match function
     should still only be called once.  */
  {
    CHECK_MATCH ("w", symbol_name_match_type::FULL, true,
		 EXPECT ("w1::w2"));
    CHECK_MATCH ("w", symbol_name_match_type::WILD, true,
		 EXPECT ("w1::w2"));
  }

  /* Same, with a "complicated" symbol.  */
  {
    static const char str[] = Z_SYM_NAME;
    size_t len = strlen (str);
    std::string lookup;

    for (size_t i = 1; i < len; i++)
      {
	lookup.assign (str, i);
	CHECK_MATCH (lookup.c_str (), symbol_name_match_type::FULL, true,
		     EXPECT (Z_SYM_NAME));
      }
  }

  /* In FULL mode, an incomplete symbol doesn't match.  */
  {
    CHECK_MATCH ("std::zfunction(int", symbol_name_match_type::FULL, false,
		 {});
  }

  /* A complete symbol with parameters matches any overload, since the
     index has no overload info.  */
  {
    CHECK_MATCH ("std::zfunction(int)", symbol_name_match_type::FULL, true,
		 EXPECT ("std::zfunction", "std::zfunction2"));
    CHECK_MATCH ("zfunction(int)", symbol_name_match_type::WILD, true,
		 EXPECT ("std::zfunction", "std::zfunction2"));
    CHECK_MATCH ("zfunc", symbol_name_match_type::WILD, true,
		 EXPECT ("std::zfunction", "std::zfunction2"));
  }

  /* Check that whitespace is ignored appropriately.  A symbol with a
     template argument list. */
  {
    static const char expected[] = "ns::foo<int>";
    CHECK_MATCH ("ns :: foo < int > ", symbol_name_match_type::FULL, false,
		 EXPECT (expected));
    CHECK_MATCH ("foo < int > ", symbol_name_match_type::WILD, false,
		 EXPECT (expected));
  }

  /* Check that whitespace is ignored appropriately.  A symbol with a
     template argument list that includes a pointer.  */
  {
    static const char expected[] = "ns::foo<char*>";
    /* Try both completion and non-completion modes.  */
    static const bool completion_mode[2] = {false, true};
    for (size_t i = 0; i < 2; i++)
      {
	CHECK_MATCH ("ns :: foo < char * >", symbol_name_match_type::FULL,
		     completion_mode[i], EXPECT (expected));
	CHECK_MATCH ("foo < char * >", symbol_name_match_type::WILD,
		     completion_mode[i], EXPECT (expected));

	CHECK_MATCH ("ns :: foo < char * > (int)", symbol_name_match_type::FULL,
		     completion_mode[i], EXPECT (expected));
	CHECK_MATCH ("foo < char * > (int)", symbol_name_match_type::WILD,
		     completion_mode[i], EXPECT (expected));
      }
  }

  {
    /* Check method qualifiers are ignored.  */
    static const char expected[] = "ns::foo<char*>";
    CHECK_MATCH ("ns :: foo < char * >  ( int ) const",
		 symbol_name_match_type::FULL, true, EXPECT (expected));
    CHECK_MATCH ("ns :: foo < char * >  ( int ) &&",
		 symbol_name_match_type::FULL, true, EXPECT (expected));
    CHECK_MATCH ("foo < char * >  ( int ) const",
		 symbol_name_match_type::WILD, true, EXPECT (expected));
    CHECK_MATCH ("foo < char * >  ( int ) &&",
		 symbol_name_match_type::WILD, true, EXPECT (expected));
  }

  /* Test lookup names that don't match anything.  */
  {
    CHECK_MATCH ("bar2", symbol_name_match_type::WILD, false,
		 {});

    CHECK_MATCH ("doesntexist", symbol_name_match_type::FULL, false,
		 {});
  }

  /* Some wild matching tests, exercising "(anonymous namespace)",
     which should not be confused with a parameter list.  */
  {
    static const char *syms[] = {
      "A::B::C",
      "B::C",
      "C",
      "A :: B :: C ( int )",
      "B :: C ( int )",
      "C ( int )",
    };

    for (const char *s : syms)
      {
	CHECK_MATCH (s, symbol_name_match_type::WILD, false,
		     EXPECT ("(anonymous namespace)::A::B::C"));
      }
  }

  {
    static const char expected[] = "ns2::tmpl<int>::foo2";
    CHECK_MATCH ("tmp", symbol_name_match_type::WILD, true,
		 EXPECT (expected));
    CHECK_MATCH ("tmpl<", symbol_name_match_type::WILD, true,
		 EXPECT (expected));
  }

  SELF_CHECK (!any_mismatch);

#undef EXPECT
#undef CHECK_MATCH
}

static void
run_test ()
{
  test_mapped_index_find_name_component_bounds ();
  test_dw2_expand_symtabs_matching_symbol ();
}

}} // namespace selftests::dw2_expand_symtabs_matching

#endif /* GDB_SELF_TEST */

/* See read.h.  */

bool
dw2_expand_symtabs_matching_one
  (dwarf2_per_cu_data *per_cu,
   dwarf2_per_objfile *per_objfile,
   gdb::function_view<expand_symtabs_file_matcher_ftype> file_matcher,
   gdb::function_view<expand_symtabs_exp_notify_ftype> expansion_notify)
{
  if (file_matcher == NULL || per_cu->mark)
    {
      bool symtab_was_null = !per_objfile->symtab_set_p (per_cu);

      compunit_symtab *symtab
	= dw2_instantiate_symtab (per_cu, per_objfile, false);
      gdb_assert (symtab != nullptr);

      if (expansion_notify != NULL && symtab_was_null)
	return expansion_notify (symtab);
    }
  return true;
}

/* See read.h.  */

void
dw_expand_symtabs_matching_file_matcher
  (dwarf2_per_objfile *per_objfile,
   gdb::function_view<expand_symtabs_file_matcher_ftype> file_matcher)
{
  if (file_matcher == NULL)
    return;

  htab_up visited_found (htab_create_alloc (10, htab_hash_pointer,
					    htab_eq_pointer,
					    NULL, xcalloc, xfree));
  htab_up visited_not_found (htab_create_alloc (10, htab_hash_pointer,
						htab_eq_pointer,
						NULL, xcalloc, xfree));

  /* The rule is CUs specify all the files, including those used by
     any TU, so there's no need to scan TUs here.  */

  for (const auto &per_cu : per_objfile->per_bfd->all_units)
    {
      QUIT;

      if (per_cu->is_debug_types)
	continue;
      per_cu->mark = 0;

      /* We only need to look at symtabs not already expanded.  */
      if (per_objfile->symtab_set_p (per_cu.get ()))
	continue;

      if (per_cu->fnd != nullptr)
	{
	  file_and_directory *fnd = per_cu->fnd.get ();

	  if (file_matcher (fnd->get_name (), false))
	    {
	      per_cu->mark = 1;
	      continue;
	    }

	  /* Before we invoke realpath, which can get expensive when many
	     files are involved, do a quick comparison of the basenames.  */
	  if ((basenames_may_differ
	       || file_matcher (lbasename (fnd->get_name ()), true))
	      && file_matcher (fnd->get_fullname (), false))
	    {
	      per_cu->mark = 1;
	      continue;
	    }
	}

      quick_file_names *file_data = dw2_get_file_names (per_cu.get (),
							per_objfile);
      if (file_data == NULL)
	continue;

      if (htab_find (visited_not_found.get (), file_data) != NULL)
	continue;
      else if (htab_find (visited_found.get (), file_data) != NULL)
	{
	  per_cu->mark = 1;
	  continue;
	}

      for (int j = 0; j < file_data->num_file_names; ++j)
	{
	  const char *this_real_name;

	  if (file_matcher (file_data->file_names[j], false))
	    {
	      per_cu->mark = 1;
	      break;
	    }

	  /* Before we invoke realpath, which can get expensive when many
	     files are involved, do a quick comparison of the basenames.  */
	  if (!basenames_may_differ
	      && !file_matcher (lbasename (file_data->file_names[j]),
				true))
	    continue;

	  this_real_name = dw2_get_real_path (per_objfile, file_data, j);
	  if (file_matcher (this_real_name, false))
	    {
	      per_cu->mark = 1;
	      break;
	    }
	}

      void **slot = htab_find_slot (per_cu->mark
				    ? visited_found.get ()
				    : visited_not_found.get (),
				    file_data, INSERT);
      *slot = file_data;
    }
}


/* A helper for dw2_find_pc_sect_compunit_symtab which finds the most specific
   symtab.  */

static struct compunit_symtab *
recursively_find_pc_sect_compunit_symtab (struct compunit_symtab *cust,
					  CORE_ADDR pc)
{
  int i;

  if (cust->blockvector () != nullptr
      && blockvector_contains_pc (cust->blockvector (), pc))
    return cust;

  if (cust->includes == NULL)
    return NULL;

  for (i = 0; cust->includes[i]; ++i)
    {
      struct compunit_symtab *s = cust->includes[i];

      s = recursively_find_pc_sect_compunit_symtab (s, pc);
      if (s != NULL)
	return s;
    }

  return NULL;
}

dwarf2_per_cu_data *
dwarf2_base_index_functions::find_per_cu (dwarf2_per_bfd *per_bfd,
					  unrelocated_addr adjusted_pc)
{
  if (per_bfd->index_addrmap == nullptr)
    return nullptr;

  void *obj = per_bfd->index_addrmap->find ((CORE_ADDR) adjusted_pc);
  return static_cast<dwarf2_per_cu_data *> (obj);
}

struct compunit_symtab *
dwarf2_base_index_functions::find_pc_sect_compunit_symtab
     (struct objfile *objfile,
      struct bound_minimal_symbol msymbol,
      CORE_ADDR pc,
      struct obj_section *section,
      int warn_if_readin)
{
  struct compunit_symtab *result;

  dwarf2_per_objfile *per_objfile = get_dwarf2_per_objfile (objfile);

  CORE_ADDR baseaddr = objfile->text_section_offset ();
  struct dwarf2_per_cu_data *data
    = find_per_cu (per_objfile->per_bfd, (unrelocated_addr) (pc - baseaddr));
  if (data == nullptr)
    return nullptr;

  if (warn_if_readin && per_objfile->symtab_set_p (data))
    warning (_("(Internal error: pc %s in read in CU, but not in symtab.)"),
	     paddress (objfile->arch (), pc));

  result = recursively_find_pc_sect_compunit_symtab
    (dw2_instantiate_symtab (data, per_objfile, false), pc);

  if (warn_if_readin && result == nullptr)
    warning (_("(Error: pc %s in address map, but not in symtab.)"),
	     paddress (objfile->arch (), pc));

  return result;
}

void
dwarf2_base_index_functions::map_symbol_filenames
     (struct objfile *objfile,
      gdb::function_view<symbol_filename_ftype> fun,
      bool need_fullname)
{
  dwarf2_per_objfile *per_objfile = get_dwarf2_per_objfile (objfile);

  /* Use caches to ensure we only call FUN once for each filename.  */
  filename_seen_cache filenames_cache;
  std::unordered_set<quick_file_names *> qfn_cache;

  /* The rule is CUs specify all the files, including those used by any TU,
     so there's no need to scan TUs here.  We can ignore file names coming
     from already-expanded CUs.  It is possible that an expanded CU might
     reuse the file names data from a currently unexpanded CU, in this
     case we don't want to report the files from the unexpanded CU.  */

  for (const auto &per_cu : per_objfile->per_bfd->all_units)
    {
      if (!per_cu->is_debug_types
	  && per_objfile->symtab_set_p (per_cu.get ()))
	{
	  if (per_cu->file_names != nullptr)
	    qfn_cache.insert (per_cu->file_names);
	}
    }

  for (dwarf2_per_cu_data *per_cu
	 : all_units_range (per_objfile->per_bfd))
    {
      /* We only need to look at symtabs not already expanded.  */
      if (per_cu->is_debug_types || per_objfile->symtab_set_p (per_cu))
	continue;

      if (per_cu->fnd != nullptr)
	{
	  file_and_directory *fnd = per_cu->fnd.get ();

	  const char *filename = fnd->get_name ();
	  const char *key = filename;
	  const char *fullname = nullptr;

	  if (need_fullname)
	    {
	      fullname = fnd->get_fullname ();
	      key = fullname;
	    }

	  if (!filenames_cache.seen (key))
	    fun (filename, fullname);
	}

      quick_file_names *file_data = dw2_get_file_names (per_cu, per_objfile);
      if (file_data == nullptr
	  || qfn_cache.find (file_data) != qfn_cache.end ())
	continue;

      for (int j = 0; j < file_data->num_file_names; ++j)
	{
	  const char *filename = file_data->file_names[j];
	  const char *key = filename;
	  const char *fullname = nullptr;

	  if (need_fullname)
	    {
	      fullname = dw2_get_real_path (per_objfile, file_data, j);
	      key = fullname;
	    }

	  if (!filenames_cache.seen (key))
	    fun (filename, fullname);
	}
    }
}

bool
dwarf2_base_index_functions::has_symbols (struct objfile *objfile)
{
  return true;
}

/* See quick_symbol_functions::has_unexpanded_symtabs in quick-symbol.h.  */

bool
dwarf2_base_index_functions::has_unexpanded_symtabs (struct objfile *objfile)
{
  dwarf2_per_objfile *per_objfile = get_dwarf2_per_objfile (objfile);

  for (const auto &per_cu : per_objfile->per_bfd->all_units)
    {
      /* Is this already expanded?  */
      if (per_objfile->symtab_set_p (per_cu.get ()))
	continue;

      /* It has not yet been expanded.  */
      return true;
    }

  return false;
}

/* Get the content of the .gdb_index section of OBJ.  SECTION_OWNER should point
   to either a dwarf2_per_bfd or dwz_file object.  */

template <typename T>
static gdb::array_view<const gdb_byte>
get_gdb_index_contents_from_section (objfile *obj, T *section_owner)
{
  dwarf2_section_info *section = &section_owner->gdb_index;

  if (section->empty ())
    return {};

  /* Older elfutils strip versions could keep the section in the main
     executable while splitting it for the separate debug info file.  */
  if ((section->get_flags () & SEC_HAS_CONTENTS) == 0)
    return {};

  section->read (obj);

  /* dwarf2_section_info::size is a bfd_size_type, while
     gdb::array_view works with size_t.  On 32-bit hosts, with
     --enable-64-bit-bfd, bfd_size_type is a 64-bit type, while size_t
     is 32-bit.  So we need an explicit narrowing conversion here.
     This is fine, because it's impossible to allocate or mmap an
     array/buffer larger than what size_t can represent.  */
  return gdb::make_array_view (section->buffer, section->size);
}

/* Lookup the index cache for the contents of the index associated to
   DWARF2_OBJ.  */

static gdb::array_view<const gdb_byte>
get_gdb_index_contents_from_cache (objfile *obj, dwarf2_per_bfd *dwarf2_per_bfd)
{
  const bfd_build_id *build_id = build_id_bfd_get (obj->obfd.get ());
  if (build_id == nullptr)
    return {};

  return global_index_cache.lookup_gdb_index (build_id,
					      &dwarf2_per_bfd->index_cache_res);
}

/* Same as the above, but for DWZ.  */

static gdb::array_view<const gdb_byte>
get_gdb_index_contents_from_cache_dwz (objfile *obj, dwz_file *dwz)
{
  const bfd_build_id *build_id = build_id_bfd_get (dwz->dwz_bfd.get ());
  if (build_id == nullptr)
    return {};

  return global_index_cache.lookup_gdb_index (build_id, &dwz->index_cache_res);
}

static quick_symbol_functions_up make_cooked_index_funcs
     (dwarf2_per_objfile *);

/* See dwarf2/public.h.  */

bool
dwarf2_initialize_objfile (struct objfile *objfile,
			   const struct dwarf2_debug_sections *names,
			   bool can_copy)
{
  if (!dwarf2_has_info (objfile, names, can_copy))
    return false;

  dwarf2_per_objfile *per_objfile = get_dwarf2_per_objfile (objfile);
  dwarf2_per_bfd *per_bfd = per_objfile->per_bfd;

  dwarf_read_debug_printf ("called");

  /* Try to fetch any potential dwz file early, while still on the
     main thread.  */
  try
    {
      dwarf2_read_dwz_file (per_objfile);
    }
  catch (const gdb_exception_error &err)
    {
      warning (_("%s"), err.what ());
    }

  /* If we're about to read full symbols, don't bother with the
     indices.  In this case we also don't care if some other debug
     format is making psymtabs, because they are all about to be
     expanded anyway.  */
  if ((objfile->flags & OBJF_READNOW))
    {
      dwarf_read_debug_printf ("readnow requested");

      create_all_units (per_objfile);
      per_bfd->quick_file_names_table
	= create_quick_file_names_table (per_bfd->all_units.size ());

      objfile->qf.emplace_front (new readnow_functions);
    }
  /* Was a GDB index already read when we processed an objfile sharing
     PER_BFD?  */
  else if (per_bfd->index_table != nullptr)
    {
      dwarf_read_debug_printf ("re-using symbols");
      objfile->qf.push_front (per_bfd->index_table->make_quick_functions ());
    }
  else if (dwarf2_read_debug_names (per_objfile))
    {
      dwarf_read_debug_printf ("found debug names");
      objfile->qf.push_front
	(per_bfd->index_table->make_quick_functions ());
    }
  else if (dwarf2_read_gdb_index (per_objfile,
				  get_gdb_index_contents_from_section<struct dwarf2_per_bfd>,
				  get_gdb_index_contents_from_section<dwz_file>))
    {
      dwarf_read_debug_printf ("found gdb index from file");
      objfile->qf.push_front (per_bfd->index_table->make_quick_functions ());
    }
  /* ... otherwise, try to find the index in the index cache.  */
  else if (dwarf2_read_gdb_index (per_objfile,
			     get_gdb_index_contents_from_cache,
			     get_gdb_index_contents_from_cache_dwz))
    {
      dwarf_read_debug_printf ("found gdb index from cache");
      global_index_cache.hit ();
      objfile->qf.push_front (per_bfd->index_table->make_quick_functions ());
    }
  else
    {
      global_index_cache.miss ();
      objfile->qf.push_front (make_cooked_index_funcs (per_objfile));
    }
  return true;
}



/* Find the base address of the compilation unit for range lists and
   location lists.  It will normally be specified by DW_AT_low_pc.
   In DWARF-3 draft 4, the base address could be overridden by
   DW_AT_entry_pc.  It's been removed, but GCC still uses this for
   compilation units with discontinuous ranges.  */

static void
dwarf2_find_base_address (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *attr;

  cu->base_address.reset ();

  attr = dwarf2_attr (die, DW_AT_entry_pc, cu);
  if (attr != nullptr)
    cu->base_address = attr->as_address ();
  else
    {
      attr = dwarf2_attr (die, DW_AT_low_pc, cu);
      if (attr != nullptr)
	cu->base_address = attr->as_address ();
    }
}

/* Helper function that returns the proper abbrev section for
   THIS_CU.  */

static struct dwarf2_section_info *
get_abbrev_section_for_cu (struct dwarf2_per_cu_data *this_cu)
{
  struct dwarf2_section_info *abbrev;
  dwarf2_per_bfd *per_bfd = this_cu->per_bfd;

  if (this_cu->is_dwz)
    abbrev = &dwarf2_get_dwz_file (per_bfd, true)->abbrev;
  else
    abbrev = &per_bfd->abbrev;

  return abbrev;
}

/* Fetch the abbreviation table offset from a comp or type unit header.  */

static sect_offset
read_abbrev_offset (dwarf2_per_objfile *per_objfile,
		    struct dwarf2_section_info *section,
		    sect_offset sect_off)
{
  bfd *abfd = section->get_bfd_owner ();
  const gdb_byte *info_ptr;
  unsigned int initial_length_size, offset_size;
  uint16_t version;

  section->read (per_objfile->objfile);
  info_ptr = section->buffer + to_underlying (sect_off);
  read_initial_length (abfd, info_ptr, &initial_length_size);
  offset_size = initial_length_size == 4 ? 4 : 8;
  info_ptr += initial_length_size;

  version = read_2_bytes (abfd, info_ptr);
  info_ptr += 2;
  if (version >= 5)
    {
      /* Skip unit type and address size.  */
      info_ptr += 2;
    }

  return (sect_offset) read_offset (abfd, info_ptr, offset_size);
}

static hashval_t
hash_signatured_type (const void *item)
{
  const struct signatured_type *sig_type
    = (const struct signatured_type *) item;

  /* This drops the top 32 bits of the signature, but is ok for a hash.  */
  return sig_type->signature;
}

static int
eq_signatured_type (const void *item_lhs, const void *item_rhs)
{
  const struct signatured_type *lhs = (const struct signatured_type *) item_lhs;
  const struct signatured_type *rhs = (const struct signatured_type *) item_rhs;

  return lhs->signature == rhs->signature;
}

/* See read.h.  */

htab_up
allocate_signatured_type_table ()
{
  return htab_up (htab_create_alloc (41,
				     hash_signatured_type,
				     eq_signatured_type,
				     NULL, xcalloc, xfree));
}

/* A helper for create_debug_types_hash_table.  Read types from SECTION
   and fill them into TYPES_HTAB.  It will process only type units,
   therefore DW_UT_type.  */

static void
create_debug_type_hash_table (dwarf2_per_objfile *per_objfile,
			      struct dwo_file *dwo_file,
			      dwarf2_section_info *section, htab_up &types_htab,
			      rcuh_kind section_kind)
{
  struct objfile *objfile = per_objfile->objfile;
  struct dwarf2_section_info *abbrev_section;
  bfd *abfd;
  const gdb_byte *info_ptr, *end_ptr;

  abbrev_section = &dwo_file->sections.abbrev;

  dwarf_read_debug_printf ("Reading %s for %s",
			   section->get_name (),
			   abbrev_section->get_file_name ());

  section->read (objfile);
  info_ptr = section->buffer;

  if (info_ptr == NULL)
    return;

  /* We can't set abfd until now because the section may be empty or
     not present, in which case the bfd is unknown.  */
  abfd = section->get_bfd_owner ();

  /* We don't use cutu_reader here because we don't need to read
     any dies: the signature is in the header.  */

  end_ptr = info_ptr + section->size;
  while (info_ptr < end_ptr)
    {
      signatured_type_up sig_type;
      struct dwo_unit *dwo_tu;
      void **slot;
      const gdb_byte *ptr = info_ptr;
      struct comp_unit_head header;
      unsigned int length;

      sect_offset sect_off = (sect_offset) (ptr - section->buffer);

      /* Initialize it due to a false compiler warning.  */
      header.signature = -1;
      header.type_cu_offset_in_tu = (cu_offset) -1;

      /* We need to read the type's signature in order to build the hash
	 table, but we don't need anything else just yet.  */

      ptr = read_and_check_comp_unit_head (per_objfile, &header, section,
					   abbrev_section, ptr, section_kind);

      length = header.get_length_with_initial ();

      /* Skip dummy type units.  */
      if (ptr >= info_ptr + length
	  || peek_abbrev_code (abfd, ptr) == 0
	  || (header.unit_type != DW_UT_type
	      && header.unit_type != DW_UT_split_type))
	{
	  info_ptr += length;
	  continue;
	}

      if (types_htab == NULL)
	types_htab = allocate_dwo_unit_table ();

      dwo_tu = OBSTACK_ZALLOC (&per_objfile->per_bfd->obstack, dwo_unit);
      dwo_tu->dwo_file = dwo_file;
      dwo_tu->signature = header.signature;
      dwo_tu->type_offset_in_tu = header.type_cu_offset_in_tu;
      dwo_tu->section = section;
      dwo_tu->sect_off = sect_off;
      dwo_tu->length = length;

      slot = htab_find_slot (types_htab.get (), dwo_tu, INSERT);
      gdb_assert (slot != NULL);
      if (*slot != NULL)
	complaint (_("debug type entry at offset %s is duplicate to"
		     " the entry at offset %s, signature %s"),
		   sect_offset_str (sect_off),
		   sect_offset_str (dwo_tu->sect_off),
		   hex_string (header.signature));
      *slot = dwo_tu;

      dwarf_read_debug_printf_v ("  offset %s, signature %s",
				 sect_offset_str (sect_off),
				 hex_string (header.signature));

      info_ptr += length;
    }
}

/* Create the hash table of all entries in the .debug_types
   (or .debug_types.dwo) section(s).
   DWO_FILE is a pointer to the DWO file object.

   The result is a pointer to the hash table or NULL if there are no types.

   Note: This function processes DWO files only, not DWP files.  */

static void
create_debug_types_hash_table (dwarf2_per_objfile *per_objfile,
			       struct dwo_file *dwo_file,
			       gdb::array_view<dwarf2_section_info> type_sections,
			       htab_up &types_htab)
{
  for (dwarf2_section_info &section : type_sections)
    create_debug_type_hash_table (per_objfile, dwo_file, &section, types_htab,
				  rcuh_kind::TYPE);
}

/* Add an entry for signature SIG to dwarf2_per_objfile->per_bfd->signatured_types.
   If SLOT is non-NULL, it is the entry to use in the hash table.
   Otherwise we find one.  */

static struct signatured_type *
add_type_unit (dwarf2_per_objfile *per_objfile, ULONGEST sig, void **slot)
{
  if (per_objfile->per_bfd->all_units.size ()
      == per_objfile->per_bfd->all_units.capacity ())
    ++per_objfile->per_bfd->tu_stats.nr_all_type_units_reallocs;

  signatured_type_up sig_type_holder
    = per_objfile->per_bfd->allocate_signatured_type (sig);
  signatured_type *sig_type = sig_type_holder.get ();

  per_objfile->per_bfd->all_units.emplace_back
    (sig_type_holder.release ());

  if (slot == NULL)
    {
      slot = htab_find_slot (per_objfile->per_bfd->signatured_types.get (),
			     sig_type, INSERT);
    }
  gdb_assert (*slot == NULL);
  *slot = sig_type;
  /* The rest of sig_type must be filled in by the caller.  */
  return sig_type;
}

/* Subroutine of lookup_dwo_signatured_type and lookup_dwp_signatured_type.
   Fill in SIG_ENTRY with DWO_ENTRY.  */

static void
fill_in_sig_entry_from_dwo_entry (dwarf2_per_objfile *per_objfile,
				  struct signatured_type *sig_entry,
				  struct dwo_unit *dwo_entry)
{
  dwarf2_per_bfd *per_bfd = per_objfile->per_bfd;

  /* Make sure we're not clobbering something we don't expect to.  */
  gdb_assert (! sig_entry->queued);
  gdb_assert (per_objfile->get_cu (sig_entry) == NULL);
  gdb_assert (!per_objfile->symtab_set_p (sig_entry));
  gdb_assert (sig_entry->signature == dwo_entry->signature);
  gdb_assert (to_underlying (sig_entry->type_offset_in_section) == 0
	      || (to_underlying (sig_entry->type_offset_in_section)
		  == to_underlying (dwo_entry->type_offset_in_tu)));
  gdb_assert (sig_entry->type_unit_group == NULL);
  gdb_assert (sig_entry->dwo_unit == NULL
	      || sig_entry->dwo_unit == dwo_entry);

  sig_entry->section = dwo_entry->section;
  sig_entry->sect_off = dwo_entry->sect_off;
  sig_entry->set_length (dwo_entry->length, false);
  sig_entry->reading_dwo_directly = 1;
  sig_entry->per_bfd = per_bfd;
  sig_entry->type_offset_in_tu = dwo_entry->type_offset_in_tu;
  sig_entry->dwo_unit = dwo_entry;
}

/* Subroutine of lookup_signatured_type.
   If we haven't read the TU yet, create the signatured_type data structure
   for a TU to be read in directly from a DWO file, bypassing the stub.
   This is the "Stay in DWO Optimization": When there is no DWP file and we're
   using .gdb_index, then when reading a CU we want to stay in the DWO file
   containing that CU.  Otherwise we could end up reading several other DWO
   files (due to comdat folding) to process the transitive closure of all the
   mentioned TUs, and that can be slow.  The current DWO file will have every
   type signature that it needs.
   We only do this for .gdb_index because in the psymtab case we already have
   to read all the DWOs to build the type unit groups.  */

static struct signatured_type *
lookup_dwo_signatured_type (struct dwarf2_cu *cu, ULONGEST sig)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct dwo_file *dwo_file;
  struct dwo_unit find_dwo_entry, *dwo_entry;
  void **slot;

  gdb_assert (cu->dwo_unit);

  /* If TU skeletons have been removed then we may not have read in any
     TUs yet.  */
  if (per_objfile->per_bfd->signatured_types == NULL)
    per_objfile->per_bfd->signatured_types = allocate_signatured_type_table ();

  /* We only ever need to read in one copy of a signatured type.
     Use the global signatured_types array to do our own comdat-folding
     of types.  If this is the first time we're reading this TU, and
     the TU has an entry in .gdb_index, replace the recorded data from
     .gdb_index with this TU.  */

  signatured_type find_sig_entry (sig);
  slot = htab_find_slot (per_objfile->per_bfd->signatured_types.get (),
			 &find_sig_entry, INSERT);
  signatured_type *sig_entry = (struct signatured_type *) *slot;

  /* We can get here with the TU already read, *or* in the process of being
     read.  Don't reassign the global entry to point to this DWO if that's
     the case.  Also note that if the TU is already being read, it may not
     have come from a DWO, the program may be a mix of Fission-compiled
     code and non-Fission-compiled code.  */

  /* Have we already tried to read this TU?
     Note: sig_entry can be NULL if the skeleton TU was removed (thus it
     needn't exist in the global table yet).  */
  if (sig_entry != NULL && sig_entry->tu_read)
    return sig_entry;

  /* Note: cu->dwo_unit is the dwo_unit that references this TU, not the
     dwo_unit of the TU itself.  */
  dwo_file = cu->dwo_unit->dwo_file;

  /* Ok, this is the first time we're reading this TU.  */
  if (dwo_file->tus == NULL)
    return NULL;
  find_dwo_entry.signature = sig;
  dwo_entry = (struct dwo_unit *) htab_find (dwo_file->tus.get (),
					     &find_dwo_entry);
  if (dwo_entry == NULL)
    return NULL;

  /* If the global table doesn't have an entry for this TU, add one.  */
  if (sig_entry == NULL)
    sig_entry = add_type_unit (per_objfile, sig, slot);

  if (sig_entry->dwo_unit == nullptr)
    fill_in_sig_entry_from_dwo_entry (per_objfile, sig_entry, dwo_entry);
  sig_entry->tu_read = 1;
  return sig_entry;
}

/* Subroutine of lookup_signatured_type.
   Look up the type for signature SIG, and if we can't find SIG in .gdb_index
   then try the DWP file.  If the TU stub (skeleton) has been removed then
   it won't be in .gdb_index.  */

static struct signatured_type *
lookup_dwp_signatured_type (struct dwarf2_cu *cu, ULONGEST sig)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct dwp_file *dwp_file = get_dwp_file (per_objfile);
  struct dwo_unit *dwo_entry;
  void **slot;

  gdb_assert (cu->dwo_unit);
  gdb_assert (dwp_file != NULL);

  /* If TU skeletons have been removed then we may not have read in any
     TUs yet.  */
  if (per_objfile->per_bfd->signatured_types == NULL)
    per_objfile->per_bfd->signatured_types = allocate_signatured_type_table ();

  signatured_type find_sig_entry (sig);
  slot = htab_find_slot (per_objfile->per_bfd->signatured_types.get (),
			 &find_sig_entry, INSERT);
  signatured_type *sig_entry = (struct signatured_type *) *slot;

  /* Have we already tried to read this TU?
     Note: sig_entry can be NULL if the skeleton TU was removed (thus it
     needn't exist in the global table yet).  */
  if (sig_entry != NULL)
    return sig_entry;

  if (dwp_file->tus == NULL)
    return NULL;
  dwo_entry = lookup_dwo_unit_in_dwp (per_objfile, dwp_file, NULL, sig,
				      1 /* is_debug_types */);
  if (dwo_entry == NULL)
    return NULL;

  sig_entry = add_type_unit (per_objfile, sig, slot);
  fill_in_sig_entry_from_dwo_entry (per_objfile, sig_entry, dwo_entry);

  return sig_entry;
}

/* Lookup a signature based type for DW_FORM_ref_sig8.
   Returns NULL if signature SIG is not present in the table.
   It is up to the caller to complain about this.  */

static struct signatured_type *
lookup_signatured_type (struct dwarf2_cu *cu, ULONGEST sig)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;

  if (cu->dwo_unit)
    {
      /* We're in a DWO/DWP file, and we're using .gdb_index.
	 These cases require special processing.  */
      if (get_dwp_file (per_objfile) == NULL)
	return lookup_dwo_signatured_type (cu, sig);
      else
	return lookup_dwp_signatured_type (cu, sig);
    }
  else
    {
      if (per_objfile->per_bfd->signatured_types == NULL)
	return NULL;
      signatured_type find_entry (sig);
      return ((struct signatured_type *)
	      htab_find (per_objfile->per_bfd->signatured_types.get (),
			 &find_entry));
    }
}

/* Low level DIE reading support.  */

/* Initialize a die_reader_specs struct from a dwarf2_cu struct.  */

static void
init_cu_die_reader (struct die_reader_specs *reader,
		    struct dwarf2_cu *cu,
		    struct dwarf2_section_info *section,
		    struct dwo_file *dwo_file,
		    struct abbrev_table *abbrev_table)
{
  gdb_assert (section->readin && section->buffer != NULL);
  reader->abfd = section->get_bfd_owner ();
  reader->cu = cu;
  reader->dwo_file = dwo_file;
  reader->die_section = section;
  reader->buffer = section->buffer;
  reader->buffer_end = section->buffer + section->size;
  reader->abbrev_table = abbrev_table;
}

/* Subroutine of cutu_reader to simplify it.
   Read in the rest of a CU/TU top level DIE from DWO_UNIT.
   There's just a lot of work to do, and cutu_reader is big enough
   already.

   STUB_COMP_UNIT_DIE is for the stub DIE, we copy over certain attributes
   from it to the DIE in the DWO.  If NULL we are skipping the stub.
   STUB_COMP_DIR is similar to STUB_COMP_UNIT_DIE: When reading a TU directly
   from the DWO file, bypassing the stub, it contains the DW_AT_comp_dir
   attribute of the referencing CU.  At most one of STUB_COMP_UNIT_DIE and
   STUB_COMP_DIR may be non-NULL.
   *RESULT_READER,*RESULT_INFO_PTR,*RESULT_COMP_UNIT_DIE
   are filled in with the info of the DIE from the DWO file.
   *RESULT_DWO_ABBREV_TABLE will be filled in with the abbrev table allocated
   from the dwo.  Since *RESULT_READER references this abbrev table, it must be
   kept around for at least as long as *RESULT_READER.

   The result is non-zero if a valid (non-dummy) DIE was found.  */

static int
read_cutu_die_from_dwo (dwarf2_cu *cu,
			struct dwo_unit *dwo_unit,
			struct die_info *stub_comp_unit_die,
			const char *stub_comp_dir,
			struct die_reader_specs *result_reader,
			const gdb_byte **result_info_ptr,
			struct die_info **result_comp_unit_die,
			abbrev_table_up *result_dwo_abbrev_table)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  dwarf2_per_cu_data *per_cu = cu->per_cu;
  struct objfile *objfile = per_objfile->objfile;
  bfd *abfd;
  const gdb_byte *begin_info_ptr, *info_ptr;
  struct dwarf2_section_info *dwo_abbrev_section;

  /* At most one of these may be provided.  */
  gdb_assert ((stub_comp_unit_die != NULL) + (stub_comp_dir != NULL) <= 1);

  /* These attributes aren't processed until later: DW_AT_stmt_list,
     DW_AT_low_pc, DW_AT_high_pc, DW_AT_ranges, DW_AT_comp_dir.
     However, these attributes are found in the stub which we won't
     have later.  In order to not impose this complication on the rest
     of the code, we read them here and copy them to the DWO CU/TU
     die.  */

  /* We store them all in an array.  */
  struct attribute *attributes[5] {};
  /* Next available element of the attributes array.  */
  int next_attr_idx = 0;

  /* Push an element into ATTRIBUTES.  */
  auto push_back = [&] (struct attribute *attr)
    {
      gdb_assert (next_attr_idx < ARRAY_SIZE (attributes));
      if (attr != nullptr)
	attributes[next_attr_idx++] = attr;
    };

  if (stub_comp_unit_die != NULL)
    {
      /* For TUs in DWO files, the DW_AT_stmt_list attribute lives in the
	 DWO file.  */
      if (!per_cu->is_debug_types)
	push_back (dwarf2_attr (stub_comp_unit_die, DW_AT_stmt_list, cu));
      push_back (dwarf2_attr (stub_comp_unit_die, DW_AT_low_pc, cu));
      push_back (dwarf2_attr (stub_comp_unit_die, DW_AT_high_pc, cu));
      push_back (dwarf2_attr (stub_comp_unit_die, DW_AT_ranges, cu));
      push_back (dwarf2_attr (stub_comp_unit_die, DW_AT_comp_dir, cu));

      cu->addr_base = stub_comp_unit_die->addr_base ();

      /* There should be a DW_AT_GNU_ranges_base attribute here (if needed).
	 We need the value before we can process DW_AT_ranges values from the
	 DWO.  */
      cu->gnu_ranges_base = stub_comp_unit_die->gnu_ranges_base ();

      /* For DWARF5: record the DW_AT_rnglists_base value from the skeleton.  If
	 there are attributes of form DW_FORM_rnglistx in the skeleton, they'll
	 need the rnglists base.  Attributes of form DW_FORM_rnglistx in the
	 split unit don't use it, as the DWO has its own .debug_rnglists.dwo
	 section.  */
      cu->rnglists_base = stub_comp_unit_die->rnglists_base ();
    }
  else if (stub_comp_dir != NULL)
    {
      /* Reconstruct the comp_dir attribute to simplify the code below.  */
      struct attribute *comp_dir = OBSTACK_ZALLOC (&cu->comp_unit_obstack,
						   struct attribute);
      comp_dir->name = DW_AT_comp_dir;
      comp_dir->form = DW_FORM_string;
      comp_dir->set_string_noncanonical (stub_comp_dir);
      push_back (comp_dir);
    }

  /* Set up for reading the DWO CU/TU.  */
  cu->dwo_unit = dwo_unit;
  dwarf2_section_info *section = dwo_unit->section;
  section->read (objfile);
  abfd = section->get_bfd_owner ();
  begin_info_ptr = info_ptr = (section->buffer
			       + to_underlying (dwo_unit->sect_off));
  dwo_abbrev_section = &dwo_unit->dwo_file->sections.abbrev;

  if (per_cu->is_debug_types)
    {
      signatured_type *sig_type = (struct signatured_type *) per_cu;

      info_ptr = read_and_check_comp_unit_head (per_objfile, &cu->header,
						section, dwo_abbrev_section,
						info_ptr, rcuh_kind::TYPE);
      /* This is not an assert because it can be caused by bad debug info.  */
      if (sig_type->signature != cu->header.signature)
	{
	  error (_("Dwarf Error: signature mismatch %s vs %s while reading"
		   " TU at offset %s [in module %s]"),
		 hex_string (sig_type->signature),
		 hex_string (cu->header.signature),
		 sect_offset_str (dwo_unit->sect_off),
		 bfd_get_filename (abfd));
	}
      gdb_assert (dwo_unit->sect_off == cu->header.sect_off);
      /* For DWOs coming from DWP files, we don't know the CU length
	 nor the type's offset in the TU until now.  */
      dwo_unit->length = cu->header.get_length_with_initial ();
      dwo_unit->type_offset_in_tu = cu->header.type_cu_offset_in_tu;

      /* Establish the type offset that can be used to lookup the type.
	 For DWO files, we don't know it until now.  */
      sig_type->type_offset_in_section
	= dwo_unit->sect_off + to_underlying (dwo_unit->type_offset_in_tu);
    }
  else
    {
      info_ptr = read_and_check_comp_unit_head (per_objfile, &cu->header,
						section, dwo_abbrev_section,
						info_ptr, rcuh_kind::COMPILE);
      gdb_assert (dwo_unit->sect_off == cu->header.sect_off);
      /* For DWOs coming from DWP files, we don't know the CU length
	 until now.  */
      dwo_unit->length = cu->header.get_length_with_initial ();
    }

  dwo_abbrev_section->read (objfile);
  *result_dwo_abbrev_table
    = abbrev_table::read (dwo_abbrev_section, cu->header.abbrev_sect_off);
  init_cu_die_reader (result_reader, cu, section, dwo_unit->dwo_file,
		      result_dwo_abbrev_table->get ());

  /* Read in the die, filling in the attributes from the stub.  This
     has the benefit of simplifying the rest of the code - all the
     work to maintain the illusion of a single
     DW_TAG_{compile,type}_unit DIE is done here.  */
  info_ptr = read_toplevel_die (result_reader, result_comp_unit_die, info_ptr,
				gdb::make_array_view (attributes,
						      next_attr_idx));

  /* Skip dummy compilation units.  */
  if (info_ptr >= begin_info_ptr + dwo_unit->length
      || peek_abbrev_code (abfd, info_ptr) == 0)
    return 0;

  *result_info_ptr = info_ptr;
  return 1;
}

/* Return the signature of the compile unit, if found. In DWARF 4 and before,
   the signature is in the DW_AT_GNU_dwo_id attribute. In DWARF 5 and later, the
   signature is part of the header.  */
static std::optional<ULONGEST>
lookup_dwo_id (struct dwarf2_cu *cu, struct die_info* comp_unit_die)
{
  if (cu->header.version >= 5)
    return cu->header.signature;
  struct attribute *attr;
  attr = dwarf2_attr (comp_unit_die, DW_AT_GNU_dwo_id, cu);
  if (attr == nullptr || !attr->form_is_unsigned ())
    return std::optional<ULONGEST> ();
  return attr->as_unsigned ();
}

/* Subroutine of cutu_reader to simplify it.
   Look up the DWO unit specified by COMP_UNIT_DIE of THIS_CU.
   Returns NULL if the specified DWO unit cannot be found.  */

static struct dwo_unit *
lookup_dwo_unit (dwarf2_cu *cu, die_info *comp_unit_die, const char *dwo_name)
{
#if CXX_STD_THREAD
  /* We need a lock here to handle the DWO hash table.  */
  static std::mutex dwo_lock;

  std::lock_guard<std::mutex> guard (dwo_lock);
#endif

  dwarf2_per_cu_data *per_cu = cu->per_cu;
  struct dwo_unit *dwo_unit;
  const char *comp_dir;

  gdb_assert (cu != NULL);

  /* Yeah, we look dwo_name up again, but it simplifies the code.  */
  dwo_name = dwarf2_dwo_name (comp_unit_die, cu);
  comp_dir = dwarf2_string_attr (comp_unit_die, DW_AT_comp_dir, cu);

  if (per_cu->is_debug_types)
    dwo_unit = lookup_dwo_type_unit (cu, dwo_name, comp_dir);
  else
    {
      std::optional<ULONGEST> signature = lookup_dwo_id (cu, comp_unit_die);

      if (!signature.has_value ())
	error (_("Dwarf Error: missing dwo_id for dwo_name %s"
		 " [in module %s]"),
	       dwo_name, bfd_get_filename (per_cu->per_bfd->obfd));

      dwo_unit = lookup_dwo_comp_unit (cu, dwo_name, comp_dir, *signature);
    }

  return dwo_unit;
}

/* Subroutine of cutu_reader to simplify it.
   See it for a description of the parameters.
   Read a TU directly from a DWO file, bypassing the stub.  */

void
cutu_reader::init_tu_and_read_dwo_dies (dwarf2_per_cu_data *this_cu,
					dwarf2_per_objfile *per_objfile,
					dwarf2_cu *existing_cu)
{
  struct signatured_type *sig_type;

  /* Verify we can do the following downcast, and that we have the
     data we need.  */
  gdb_assert (this_cu->is_debug_types && this_cu->reading_dwo_directly);
  sig_type = (struct signatured_type *) this_cu;
  gdb_assert (sig_type->dwo_unit != NULL);

  dwarf2_cu *cu;

  if (existing_cu != nullptr)
    {
      cu = existing_cu;
      gdb_assert (cu->dwo_unit == sig_type->dwo_unit);
      /* There's no need to do the rereading_dwo_cu handling that
	 cutu_reader does since we don't read the stub.  */
    }
  else
    {
      /* If an existing_cu is provided, a dwarf2_cu must not exist for this_cu
	 in per_objfile yet.  */
      gdb_assert (per_objfile->get_cu (this_cu) == nullptr);
      m_new_cu.reset (new dwarf2_cu (this_cu, per_objfile));
      cu = m_new_cu.get ();
    }

  /* A future optimization, if needed, would be to use an existing
     abbrev table.  When reading DWOs with skeletonless TUs, all the TUs
     could share abbrev tables.  */

  if (read_cutu_die_from_dwo (cu, sig_type->dwo_unit,
			      NULL /* stub_comp_unit_die */,
			      sig_type->dwo_unit->dwo_file->comp_dir,
			      this, &info_ptr,
			      &comp_unit_die,
			      &m_dwo_abbrev_table) == 0)
    {
      /* Dummy die.  */
      dummy_p = true;
    }
}

/* Initialize a CU (or TU) and read its DIEs.
   If the CU defers to a DWO file, read the DWO file as well.

   ABBREV_TABLE, if non-NULL, is the abbreviation table to use.
   Otherwise the table specified in the comp unit header is read in and used.
   This is an optimization for when we already have the abbrev table.

   If EXISTING_CU is non-NULL, then use it.  Otherwise, a new CU is
   allocated.  */

cutu_reader::cutu_reader (dwarf2_per_cu_data *this_cu,
			  dwarf2_per_objfile *per_objfile,
			  struct abbrev_table *abbrev_table,
			  dwarf2_cu *existing_cu,
			  bool skip_partial,
			  abbrev_cache *cache)
  : die_reader_specs {},
    m_this_cu (this_cu)
{
  struct objfile *objfile = per_objfile->objfile;
  struct dwarf2_section_info *section = this_cu->section;
  bfd *abfd = section->get_bfd_owner ();
  const gdb_byte *begin_info_ptr;
  struct signatured_type *sig_type = NULL;
  struct dwarf2_section_info *abbrev_section;
  /* Non-zero if CU currently points to a DWO file and we need to
     reread it.  When this happens we need to reread the skeleton die
     before we can reread the DWO file (this only applies to CUs, not TUs).  */
  int rereading_dwo_cu = 0;

  if (dwarf_die_debug)
    gdb_printf (gdb_stdlog, "Reading %s unit at offset %s\n",
		this_cu->is_debug_types ? "type" : "comp",
		sect_offset_str (this_cu->sect_off));

  /* If we're reading a TU directly from a DWO file, including a virtual DWO
     file (instead of going through the stub), short-circuit all of this.  */
  if (this_cu->reading_dwo_directly)
    {
      /* Narrow down the scope of possibilities to have to understand.  */
      gdb_assert (this_cu->is_debug_types);
      gdb_assert (abbrev_table == NULL);
      init_tu_and_read_dwo_dies (this_cu, per_objfile, existing_cu);
      return;
    }

  /* This is cheap if the section is already read in.  */
  section->read (objfile);

  begin_info_ptr = info_ptr = section->buffer + to_underlying (this_cu->sect_off);

  abbrev_section = get_abbrev_section_for_cu (this_cu);

  dwarf2_cu *cu;

  if (existing_cu != nullptr)
    {
      cu = existing_cu;
      /* If this CU is from a DWO file we need to start over, we need to
	 refetch the attributes from the skeleton CU.
	 This could be optimized by retrieving those attributes from when we
	 were here the first time: the previous comp_unit_die was stored in
	 comp_unit_obstack.  But there's no data yet that we need this
	 optimization.  */
      if (cu->dwo_unit != NULL)
	rereading_dwo_cu = 1;
    }
  else
    {
      /* If an existing_cu is provided, a dwarf2_cu must not exist for
	 this_cu in per_objfile yet.  Here, CACHE doubles as a flag to
	 let us know that the CU is being scanned using the parallel
	 indexer.  This assert is avoided in this case because (1) it
	 is irrelevant, and (2) the get_cu method is not
	 thread-safe.  */
      gdb_assert (cache != nullptr
		  || per_objfile->get_cu (this_cu) == nullptr);
      m_new_cu.reset (new dwarf2_cu (this_cu, per_objfile));
      cu = m_new_cu.get ();
    }

  /* Get the header.  */
  if (to_underlying (cu->header.first_die_cu_offset) != 0 && !rereading_dwo_cu)
    {
      /* We already have the header, there's no need to read it in again.  */
      info_ptr += to_underlying (cu->header.first_die_cu_offset);
    }
  else
    {
      if (this_cu->is_debug_types)
	{
	  info_ptr = read_and_check_comp_unit_head (per_objfile, &cu->header,
						    section, abbrev_section,
						    info_ptr, rcuh_kind::TYPE);

	  /* Since per_cu is the first member of struct signatured_type,
	     we can go from a pointer to one to a pointer to the other.  */
	  sig_type = (struct signatured_type *) this_cu;
	  gdb_assert (sig_type->signature == cu->header.signature);
	  gdb_assert (sig_type->type_offset_in_tu
		      == cu->header.type_cu_offset_in_tu);
	  gdb_assert (this_cu->sect_off == cu->header.sect_off);

	  /* LENGTH has not been set yet for type units if we're
	     using .gdb_index.  */
	  this_cu->set_length (cu->header.get_length_with_initial ());

	  /* Establish the type offset that can be used to lookup the type.  */
	  sig_type->type_offset_in_section =
	    this_cu->sect_off + to_underlying (sig_type->type_offset_in_tu);

	  this_cu->set_version (cu->header.version);
	}
      else
	{
	  info_ptr = read_and_check_comp_unit_head (per_objfile, &cu->header,
						    section, abbrev_section,
						    info_ptr,
						    rcuh_kind::COMPILE);

	  gdb_assert (this_cu->sect_off == cu->header.sect_off);
	  this_cu->set_length (cu->header.get_length_with_initial ());
	  this_cu->set_version (cu->header.version);
	}
    }

  /* Skip dummy compilation units.  */
  if (info_ptr >= begin_info_ptr + this_cu->length ()
      || peek_abbrev_code (abfd, info_ptr) == 0)
    {
      dummy_p = true;
      return;
    }

  /* If we don't have them yet, read the abbrevs for this compilation unit.
     And if we need to read them now, make sure they're freed when we're
     done.  */
  if (abbrev_table != NULL)
    gdb_assert (cu->header.abbrev_sect_off == abbrev_table->sect_off);
  else
    {
      if (cache != nullptr)
	abbrev_table = cache->find (abbrev_section,
				    cu->header.abbrev_sect_off);
      if (abbrev_table == nullptr)
	{
	  abbrev_section->read (objfile);
	  m_abbrev_table_holder
	    = abbrev_table::read (abbrev_section, cu->header.abbrev_sect_off);
	  abbrev_table = m_abbrev_table_holder.get ();
	}
    }

  /* Read the top level CU/TU die.  */
  init_cu_die_reader (this, cu, section, NULL, abbrev_table);
  info_ptr = read_toplevel_die (this, &comp_unit_die, info_ptr);

  if (skip_partial && comp_unit_die->tag == DW_TAG_partial_unit)
    {
      dummy_p = true;
      return;
    }

  /* If we are in a DWO stub, process it and then read in the "real" CU/TU
     from the DWO file.  read_cutu_die_from_dwo will allocate the abbreviation
     table from the DWO file and pass the ownership over to us.  It will be
     referenced from READER, so we must make sure to free it after we're done
     with READER.

     Note that if USE_EXISTING_OK != 0, and THIS_CU->cu already contains a
     DWO CU, that this test will fail (the attribute will not be present).  */
  const char *dwo_name = dwarf2_dwo_name (comp_unit_die, cu);
  if (dwo_name != nullptr)
    {
      struct dwo_unit *dwo_unit;
      struct die_info *dwo_comp_unit_die;

      if (comp_unit_die->has_children)
	{
	  complaint (_("compilation unit with DW_AT_GNU_dwo_name"
		       " has children (offset %s) [in module %s]"),
		     sect_offset_str (this_cu->sect_off),
		     bfd_get_filename (abfd));
	}
      dwo_unit = lookup_dwo_unit (cu, comp_unit_die, dwo_name);
      if (dwo_unit != NULL)
	{
	  if (read_cutu_die_from_dwo (cu, dwo_unit,
				      comp_unit_die, NULL,
				      this, &info_ptr,
				      &dwo_comp_unit_die,
				      &m_dwo_abbrev_table) == 0)
	    {
	      /* Dummy die.  */
	      dummy_p = true;
	      return;
	    }
	  comp_unit_die = dwo_comp_unit_die;
	}
      else
	{
	  /* Yikes, we couldn't find the rest of the DIE, we only have
	     the stub.  A complaint has already been logged.  There's
	     not much more we can do except pass on the stub DIE to
	     die_reader_func.  We don't want to throw an error on bad
	     debug info.  */
	}
    }
}

void
cutu_reader::keep ()
{
  /* Done, clean up.  */
  gdb_assert (!dummy_p);
  if (m_new_cu != NULL)
    {
      /* Save this dwarf2_cu in the per_objfile.  The per_objfile owns it
	 now.  */
      dwarf2_per_objfile *per_objfile = m_new_cu->per_objfile;
      per_objfile->set_cu (m_this_cu, std::move (m_new_cu));
    }
}

/* Read CU/TU THIS_CU but do not follow DW_AT_GNU_dwo_name (DW_AT_dwo_name)
   if present. DWO_FILE, if non-NULL, is the DWO file to read (the caller is
   assumed to have already done the lookup to find the DWO file).

   The caller is required to fill in THIS_CU->section, THIS_CU->offset, and
   THIS_CU->is_debug_types, but nothing else.

   We fill in THIS_CU->length.

   THIS_CU->cu is always freed when done.
   This is done in order to not leave THIS_CU->cu in a state where we have
   to care whether it refers to the "main" CU or the DWO CU.

   When parent_cu is passed, it is used to provide a default value for
   str_offsets_base and addr_base from the parent.  */

cutu_reader::cutu_reader (dwarf2_per_cu_data *this_cu,
			  dwarf2_per_objfile *per_objfile,
			  struct dwarf2_cu *parent_cu,
			  struct dwo_file *dwo_file)
  : die_reader_specs {},
    m_this_cu (this_cu)
{
  struct objfile *objfile = per_objfile->objfile;
  struct dwarf2_section_info *section = this_cu->section;
  bfd *abfd = section->get_bfd_owner ();
  struct dwarf2_section_info *abbrev_section;
  const gdb_byte *begin_info_ptr, *info_ptr;

  if (dwarf_die_debug)
    gdb_printf (gdb_stdlog, "Reading %s unit at offset %s\n",
		this_cu->is_debug_types ? "type" : "comp",
		sect_offset_str (this_cu->sect_off));

  gdb_assert (per_objfile->get_cu (this_cu) == nullptr);

  abbrev_section = (dwo_file != NULL
		    ? &dwo_file->sections.abbrev
		    : get_abbrev_section_for_cu (this_cu));

  /* This is cheap if the section is already read in.  */
  section->read (objfile);

  m_new_cu.reset (new dwarf2_cu (this_cu, per_objfile));

  begin_info_ptr = info_ptr = section->buffer + to_underlying (this_cu->sect_off);
  info_ptr = read_and_check_comp_unit_head (per_objfile, &m_new_cu->header,
					    section, abbrev_section, info_ptr,
					    (this_cu->is_debug_types
					     ? rcuh_kind::TYPE
					     : rcuh_kind::COMPILE));

  if (parent_cu != nullptr)
    {
      m_new_cu->str_offsets_base = parent_cu->str_offsets_base;
      m_new_cu->addr_base = parent_cu->addr_base;
    }
  this_cu->set_length (m_new_cu->header.get_length_with_initial ());

  /* Skip dummy compilation units.  */
  if (info_ptr >= begin_info_ptr + this_cu->length ()
      || peek_abbrev_code (abfd, info_ptr) == 0)
    {
      dummy_p = true;
      return;
    }

  abbrev_section->read (objfile);
  m_abbrev_table_holder
    = abbrev_table::read (abbrev_section, m_new_cu->header.abbrev_sect_off);

  init_cu_die_reader (this, m_new_cu.get (), section, dwo_file,
		      m_abbrev_table_holder.get ());
  info_ptr = read_toplevel_die (this, &comp_unit_die, info_ptr);
}


/* Type Unit Groups.

   Type Unit Groups are a way to collapse the set of all TUs (type units) into
   a more manageable set.  The grouping is done by DW_AT_stmt_list entry
   so that all types coming from the same compilation (.o file) are grouped
   together.  A future step could be to put the types in the same symtab as
   the CU the types ultimately came from.  */

static hashval_t
hash_type_unit_group (const void *item)
{
  const struct type_unit_group *tu_group
    = (const struct type_unit_group *) item;

  return hash_stmt_list_entry (&tu_group->hash);
}

static int
eq_type_unit_group (const void *item_lhs, const void *item_rhs)
{
  const struct type_unit_group *lhs = (const struct type_unit_group *) item_lhs;
  const struct type_unit_group *rhs = (const struct type_unit_group *) item_rhs;

  return eq_stmt_list_entry (&lhs->hash, &rhs->hash);
}

/* Allocate a hash table for type unit groups.  */

static htab_up
allocate_type_unit_groups_table ()
{
  return htab_up (htab_create_alloc (3,
				     hash_type_unit_group,
				     eq_type_unit_group,
				     htab_delete_entry<type_unit_group>,
				     xcalloc, xfree));
}

/* Type units that don't have DW_AT_stmt_list are grouped into their own
   partial symtabs.  We combine several TUs per psymtab to not let the size
   of any one psymtab grow too big.  */
#define NO_STMT_LIST_TYPE_UNIT_PSYMTAB (1 << 31)
#define NO_STMT_LIST_TYPE_UNIT_PSYMTAB_SIZE 10

/* Helper routine for get_type_unit_group.
   Create the type_unit_group object used to hold one or more TUs.  */

static std::unique_ptr<type_unit_group>
create_type_unit_group (struct dwarf2_cu *cu, sect_offset line_offset_struct)
{
  auto tu_group = std::make_unique<type_unit_group> ();

  tu_group->hash.dwo_unit = cu->dwo_unit;
  tu_group->hash.line_sect_off = line_offset_struct;

  return tu_group;
}

/* Look up the type_unit_group for type unit CU, and create it if necessary.
   STMT_LIST is a DW_AT_stmt_list attribute.  */

static struct type_unit_group *
get_type_unit_group (struct dwarf2_cu *cu, const struct attribute *stmt_list)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct tu_stats *tu_stats = &per_objfile->per_bfd->tu_stats;
  struct type_unit_group *tu_group;
  void **slot;
  unsigned int line_offset;
  struct type_unit_group type_unit_group_for_lookup;

  if (per_objfile->per_bfd->type_unit_groups == NULL)
    per_objfile->per_bfd->type_unit_groups = allocate_type_unit_groups_table ();

  /* Do we need to create a new group, or can we use an existing one?  */

  if (stmt_list != nullptr && stmt_list->form_is_unsigned ())
    {
      line_offset = stmt_list->as_unsigned ();
      ++tu_stats->nr_symtab_sharers;
    }
  else
    {
      /* Ugh, no stmt_list.  Rare, but we have to handle it.
	 We can do various things here like create one group per TU or
	 spread them over multiple groups to split up the expansion work.
	 To avoid worst case scenarios (too many groups or too large groups)
	 we, umm, group them in bunches.  */
      line_offset = (NO_STMT_LIST_TYPE_UNIT_PSYMTAB
		     | (tu_stats->nr_stmt_less_type_units
			/ NO_STMT_LIST_TYPE_UNIT_PSYMTAB_SIZE));
      ++tu_stats->nr_stmt_less_type_units;
    }

  type_unit_group_for_lookup.hash.dwo_unit = cu->dwo_unit;
  type_unit_group_for_lookup.hash.line_sect_off = (sect_offset) line_offset;
  slot = htab_find_slot (per_objfile->per_bfd->type_unit_groups.get (),
			 &type_unit_group_for_lookup, INSERT);
  if (*slot == nullptr)
    {
      sect_offset line_offset_struct = (sect_offset) line_offset;
      std::unique_ptr<type_unit_group> grp
	= create_type_unit_group (cu, line_offset_struct);
      *slot = grp.release ();
      ++tu_stats->nr_symtabs;
    }

  tu_group = (struct type_unit_group *) *slot;
  gdb_assert (tu_group != nullptr);
  return tu_group;
}


cooked_index_storage::cooked_index_storage ()
  : m_reader_hash (htab_create_alloc (10, hash_cutu_reader,
				      eq_cutu_reader,
				      htab_delete_entry<cutu_reader>,
				      xcalloc, xfree)),
    m_index (new cooked_index_shard)
{
}

cutu_reader *
cooked_index_storage::get_reader (dwarf2_per_cu_data *per_cu)
{
  int index = per_cu->index;
  return (cutu_reader *) htab_find_with_hash (m_reader_hash.get (),
					      &index, index);
}

cutu_reader *
cooked_index_storage::preserve (std::unique_ptr<cutu_reader> reader)
{
  m_abbrev_cache.add (reader->release_abbrev_table ());

  int index = reader->cu->per_cu->index;
  void **slot = htab_find_slot_with_hash (m_reader_hash.get (), &index,
					  index, INSERT);
  gdb_assert (*slot == nullptr);
  cutu_reader *result = reader.get ();
  *slot = reader.release ();
  return result;
}

/* Hash function for a cutu_reader.  */
hashval_t
cooked_index_storage::hash_cutu_reader (const void *a)
{
  const cutu_reader *reader = (const cutu_reader *) a;
  return reader->cu->per_cu->index;
}

/* Equality function for cutu_reader.  */
int
cooked_index_storage::eq_cutu_reader (const void *a, const void *b)
{
  const cutu_reader *ra = (const cutu_reader *) a;
  const int *rb = (const int *) b;
  return ra->cu->per_cu->index == *rb;
}

/* An instance of this is created to index a CU.  */

class cooked_indexer
{
public:

  cooked_indexer (cooked_index_storage *storage,
		  dwarf2_per_cu_data *per_cu,
		  enum language language)
    : m_index_storage (storage),
      m_per_cu (per_cu),
      m_language (language)
  {
  }

  DISABLE_COPY_AND_ASSIGN (cooked_indexer);

  /* Index the given CU.  */
  void make_index (cutu_reader *reader);

private:

  /* A helper function to turn a section offset into an address that
     can be used in an addrmap.  */
  CORE_ADDR form_addr (sect_offset offset, bool is_dwz)
  {
    CORE_ADDR value = to_underlying (offset);
    if (is_dwz)
      value |= ((CORE_ADDR) 1) << (8 * sizeof (CORE_ADDR) - 1);
    return value;
  }

  /* A helper function to scan the PC bounds of READER and record them
     in the storage's addrmap.  */
  void check_bounds (cutu_reader *reader);

  /* Ensure that the indicated CU exists.  The cutu_reader for it is
     returned.  FOR_SCANNING is true if the caller intends to scan all
     the DIEs in the CU; when false, this use is assumed to be to look
     up just a single DIE.  */
  cutu_reader *ensure_cu_exists (cutu_reader *reader,
				 dwarf2_per_objfile *per_objfile,
				 sect_offset sect_off,
				 bool is_dwz,
				 bool for_scanning);

  /* Index DIEs in the READER starting at INFO_PTR.  PARENT_ENTRY is
     the entry for the enclosing scope (nullptr at top level).  FULLY
     is true when a full scan must be done -- in some languages,
     function scopes must be fully explored in order to find nested
     functions.  This returns a pointer to just after the spot where
     reading stopped.  */
  const gdb_byte *index_dies (cutu_reader *reader,
			      const gdb_byte *info_ptr,
			      const cooked_index_entry *parent_entry,
			      bool fully);

  /* Scan the attributes for a given DIE and update the out
     parameters.  Returns a pointer to the byte after the DIE.  */
  const gdb_byte *scan_attributes (dwarf2_per_cu_data *scanning_per_cu,
				   cutu_reader *reader,
				   const gdb_byte *watermark_ptr,
				   const gdb_byte *info_ptr,
				   const abbrev_info *abbrev,
				   const char **name,
				   const char **linkage_name,
				   cooked_index_flag *flags,
				   sect_offset *sibling_offset,
				   const cooked_index_entry **parent_entry,
				   CORE_ADDR *maybe_defer,
				   bool for_specification);

  /* Handle DW_TAG_imported_unit, by scanning the DIE to find
     DW_AT_import, and then scanning the referenced CU.  Returns a
     pointer to the byte after the DIE.  */
  const gdb_byte *index_imported_unit (cutu_reader *reader,
				       const gdb_byte *info_ptr,
				       const abbrev_info *abbrev);

  /* Recursively read DIEs, recording the section offsets in
     m_die_range_map and then calling index_dies.  */
  const gdb_byte *recurse (cutu_reader *reader,
			   const gdb_byte *info_ptr,
			   const cooked_index_entry *parent_entry,
			   bool fully);

  /* The storage object, where the results are kept.  */
  cooked_index_storage *m_index_storage;
  /* The CU that we are reading on behalf of.  This object might be
     asked to index one CU but to treat the results as if they come
     from some including CU; in this case the including CU would be
     recorded here.  */
  dwarf2_per_cu_data *m_per_cu;
  /* The language that we're assuming when reading.  */
  enum language m_language;

  /* An addrmap that maps from section offsets (see the form_addr
     method) to newly-created entries.  See m_deferred_entries to
     understand this.  */
  addrmap_mutable m_die_range_map;

  /* The generated DWARF can sometimes have the declaration for a
     method in a class (or perhaps namespace) scope, with the
     definition appearing outside this scope... just one of the many
     bad things about DWARF.  In order to handle this situation, we
     defer certain entries until the end of scanning, at which point
     we'll know the containing context of all the DIEs that we might
     have scanned.  This vector stores these deferred entries.  */
  std::vector<cooked_index_entry *> m_deferred_entries;
};

/* Subroutine of dwarf2_build_psymtabs_hard to simplify it.
   Process compilation unit THIS_CU for a psymtab.  */

static void
process_psymtab_comp_unit (dwarf2_per_cu_data *this_cu,
			   dwarf2_per_objfile *per_objfile,
			   cooked_index_storage *storage)
{
  cutu_reader reader (this_cu, per_objfile, nullptr, nullptr, false,
		      storage->get_abbrev_cache ());

  if (reader.comp_unit_die == nullptr)
    return;

  if (reader.dummy_p)
    {
      /* Nothing.  */
    }
  else if (this_cu->is_debug_types)
    build_type_psymtabs_reader (&reader, storage);
  else if (reader.comp_unit_die->tag != DW_TAG_partial_unit)
    {
      bool nope = false;
      if (this_cu->scanned.compare_exchange_strong (nope, true))
	{
	  prepare_one_comp_unit (reader.cu, reader.comp_unit_die,
				 language_minimal);
	  gdb_assert (storage != nullptr);
	  cooked_indexer indexer (storage, this_cu, reader.cu->lang ());
	  indexer.make_index (&reader);
	}
    }
}

/* Reader function for build_type_psymtabs.  */

static void
build_type_psymtabs_reader (cutu_reader *reader,
			    cooked_index_storage *storage)
{
  struct dwarf2_cu *cu = reader->cu;
  struct dwarf2_per_cu_data *per_cu = cu->per_cu;
  struct die_info *type_unit_die = reader->comp_unit_die;

  gdb_assert (per_cu->is_debug_types);

  if (! type_unit_die->has_children)
    return;

  prepare_one_comp_unit (cu, type_unit_die, language_minimal);

  gdb_assert (storage != nullptr);
  cooked_indexer indexer (storage, per_cu, cu->lang ());
  indexer.make_index (reader);
}

/* Struct used to sort TUs by their abbreviation table offset.  */

struct tu_abbrev_offset
{
  tu_abbrev_offset (signatured_type *sig_type_, sect_offset abbrev_offset_)
  : sig_type (sig_type_), abbrev_offset (abbrev_offset_)
  {}

  /* This is used when sorting.  */
  bool operator< (const tu_abbrev_offset &other) const
  {
    return abbrev_offset < other.abbrev_offset;
  }

  signatured_type *sig_type;
  sect_offset abbrev_offset;
};

/* Efficiently read all the type units.

   The efficiency is because we sort TUs by the abbrev table they use and
   only read each abbrev table once.  In one program there are 200K TUs
   sharing 8K abbrev tables.

   The main purpose of this function is to support building the
   dwarf2_per_objfile->per_bfd->type_unit_groups table.
   TUs typically share the DW_AT_stmt_list of the CU they came from, so we
   can collapse the search space by grouping them by stmt_list.
   The savings can be significant, in the same program from above the 200K TUs
   share 8K stmt_list tables.

   FUNC is expected to call get_type_unit_group, which will create the
   struct type_unit_group if necessary and add it to
   dwarf2_per_objfile->per_bfd->type_unit_groups.  */

static void
build_type_psymtabs (dwarf2_per_objfile *per_objfile,
		     cooked_index_storage *storage)
{
  struct tu_stats *tu_stats = &per_objfile->per_bfd->tu_stats;
  abbrev_table_up abbrev_table;
  sect_offset abbrev_offset;

  /* It's up to the caller to not call us multiple times.  */
  gdb_assert (per_objfile->per_bfd->type_unit_groups == NULL);

  if (per_objfile->per_bfd->all_type_units.size () == 0)
    return;

  /* TUs typically share abbrev tables, and there can be way more TUs than
     abbrev tables.  Sort by abbrev table to reduce the number of times we
     read each abbrev table in.
     Alternatives are to punt or to maintain a cache of abbrev tables.
     This is simpler and efficient enough for now.

     Later we group TUs by their DW_AT_stmt_list value (as this defines the
     symtab to use).  Typically TUs with the same abbrev offset have the same
     stmt_list value too so in practice this should work well.

     The basic algorithm here is:

      sort TUs by abbrev table
      for each TU with same abbrev table:
	read abbrev table if first user
	read TU top level DIE
	  [IWBN if DWO skeletons had DW_AT_stmt_list]
	call FUNC  */

  dwarf_read_debug_printf ("Building type unit groups ...");

  /* Sort in a separate table to maintain the order of all_units
     for .gdb_index: TU indices directly index all_type_units.  */
  std::vector<tu_abbrev_offset> sorted_by_abbrev;
  sorted_by_abbrev.reserve (per_objfile->per_bfd->all_type_units.size ());

  for (const auto &cu : per_objfile->per_bfd->all_units)
    {
      if (cu->is_debug_types)
	{
	  auto sig_type = static_cast<signatured_type *> (cu.get ());
	  sorted_by_abbrev.emplace_back
	    (sig_type, read_abbrev_offset (per_objfile, sig_type->section,
					   sig_type->sect_off));
	}
    }

  std::sort (sorted_by_abbrev.begin (), sorted_by_abbrev.end ());

  abbrev_offset = (sect_offset) ~(unsigned) 0;

  for (const tu_abbrev_offset &tu : sorted_by_abbrev)
    {
      /* Switch to the next abbrev table if necessary.  */
      if (abbrev_table == NULL
	  || tu.abbrev_offset != abbrev_offset)
	{
	  abbrev_offset = tu.abbrev_offset;
	  per_objfile->per_bfd->abbrev.read (per_objfile->objfile);
	  abbrev_table =
	    abbrev_table::read (&per_objfile->per_bfd->abbrev, abbrev_offset);
	  ++tu_stats->nr_uniq_abbrev_tables;
	}

      cutu_reader reader (tu.sig_type, per_objfile,
			  abbrev_table.get (), nullptr, false);
      if (!reader.dummy_p)
	build_type_psymtabs_reader (&reader, storage);
    }
}

/* Print collected type unit statistics.  */

static void
print_tu_stats (dwarf2_per_objfile *per_objfile)
{
  struct tu_stats *tu_stats = &per_objfile->per_bfd->tu_stats;

  dwarf_read_debug_printf ("Type unit statistics:");
  dwarf_read_debug_printf ("  %d TUs", tu_stats->nr_tus);
  dwarf_read_debug_printf ("  %d uniq abbrev tables",
			   tu_stats->nr_uniq_abbrev_tables);
  dwarf_read_debug_printf ("  %d symtabs from stmt_list entries",
			   tu_stats->nr_symtabs);
  dwarf_read_debug_printf ("  %d symtab sharers",
			   tu_stats->nr_symtab_sharers);
  dwarf_read_debug_printf ("  %d type units without a stmt_list",
			   tu_stats->nr_stmt_less_type_units);
  dwarf_read_debug_printf ("  %d all_type_units reallocs",
			   tu_stats->nr_all_type_units_reallocs);
}

struct skeleton_data
{
  dwarf2_per_objfile *per_objfile;
  cooked_index_storage *storage;
};

/* Traversal function for process_skeletonless_type_unit.
   Read a TU in a DWO file and build partial symbols for it.  */

static int
process_skeletonless_type_unit (void **slot, void *info)
{
  struct dwo_unit *dwo_unit = (struct dwo_unit *) *slot;
  skeleton_data *data = (skeleton_data *) info;

  /* If this TU doesn't exist in the global table, add it and read it in.  */

  if (data->per_objfile->per_bfd->signatured_types == NULL)
    data->per_objfile->per_bfd->signatured_types
      = allocate_signatured_type_table ();

  signatured_type find_entry (dwo_unit->signature);
  slot = htab_find_slot (data->per_objfile->per_bfd->signatured_types.get (),
			 &find_entry, INSERT);
  /* If we've already seen this type there's nothing to do.  What's happening
     is we're doing our own version of comdat-folding here.  */
  if (*slot != NULL)
    return 1;

  /* This does the job that create_all_units would have done for
     this TU.  */
  signatured_type *entry
    = add_type_unit (data->per_objfile, dwo_unit->signature, slot);
  fill_in_sig_entry_from_dwo_entry (data->per_objfile, entry, dwo_unit);
  *slot = entry;

  /* This does the job that build_type_psymtabs would have done.  */
  cutu_reader reader (entry, data->per_objfile, nullptr, nullptr, false);
  if (!reader.dummy_p)
    build_type_psymtabs_reader (&reader, data->storage);

  return 1;
}

/* Traversal function for process_skeletonless_type_units.  */

static int
process_dwo_file_for_skeletonless_type_units (void **slot, void *info)
{
  struct dwo_file *dwo_file = (struct dwo_file *) *slot;

  if (dwo_file->tus != NULL)
    htab_traverse_noresize (dwo_file->tus.get (),
			    process_skeletonless_type_unit, info);

  return 1;
}

/* Scan all TUs of DWO files, verifying we've processed them.
   This is needed in case a TU was emitted without its skeleton.
   Note: This can't be done until we know what all the DWO files are.  */

static void
process_skeletonless_type_units (dwarf2_per_objfile *per_objfile,
				 cooked_index_storage *storage)
{
  skeleton_data data { per_objfile, storage };

  /* Skeletonless TUs in DWP files without .gdb_index is not supported yet.  */
  if (get_dwp_file (per_objfile) == NULL
      && per_objfile->per_bfd->dwo_files != NULL)
    {
      htab_traverse_noresize (per_objfile->per_bfd->dwo_files.get (),
			      process_dwo_file_for_skeletonless_type_units,
			      &data);
    }
}

cooked_index_worker::cooked_index_worker (dwarf2_per_objfile *per_objfile)
  : m_per_objfile (per_objfile)
{
  gdb_assert (is_main_thread ());

  struct objfile *objfile = per_objfile->objfile;
  dwarf2_per_bfd *per_bfd = per_objfile->per_bfd;

  dwarf_read_debug_printf ("Building psymtabs of objfile %s ...",
			   objfile_name (objfile));

  per_bfd->map_info_sections (objfile);
}

void
cooked_index_worker::start ()
{
  gdb::thread_pool::g_thread_pool->post_task ([=] ()
  {
    this->start_reading ();
  });
}

void
cooked_index_worker::process_cus (size_t task_number, unit_iterator first,
				  unit_iterator end)
{
  SCOPE_EXIT { bfd_thread_cleanup (); };

  /* Ensure that complaints are handled correctly.  */
  complaint_interceptor complaint_handler;

  std::vector<gdb_exception> errors;
  cooked_index_storage thread_storage;
  for (auto inner = first; inner != end; ++inner)
    {
      dwarf2_per_cu_data *per_cu = inner->get ();
      try
	{
	  process_psymtab_comp_unit (per_cu, m_per_objfile, &thread_storage);
	}
      catch (gdb_exception &except)
	{
	  errors.push_back (std::move (except));
	}
    }

  m_results[task_number] = result_type (thread_storage.release (),
					complaint_handler.release (),
					std::move (errors));
}

void
cooked_index_worker::done_reading ()
{
  /* Only handle the scanning results here.  Complaints and exceptions
     can only be dealt with on the main thread.  */
  std::vector<std::unique_ptr<cooked_index_shard>> indexes;
  for (auto &one_result : m_results)
    indexes.push_back (std::move (std::get<0> (one_result)));

  /* This has to wait until we read the CUs, we need the list of DWOs.  */
  process_skeletonless_type_units (m_per_objfile, &m_index_storage);

  indexes.push_back (m_index_storage.release ());
  indexes.shrink_to_fit ();

  dwarf2_per_bfd *per_bfd = m_per_objfile->per_bfd;
  cooked_index *table
    = (gdb::checked_static_cast<cooked_index *>
       (per_bfd->index_table.get ()));
  table->set_contents (std::move (indexes));
}

void
cooked_index_worker::start_reading ()
{
  SCOPE_EXIT { bfd_thread_cleanup (); };

  try
    {
      do_reading ();
    }
  catch (const gdb_exception &exc)
    {
      m_failed = exc;
      set (cooked_state::CACHE_DONE);
    }
}

void
cooked_index_worker::do_reading ()
{
  dwarf2_per_bfd *per_bfd = m_per_objfile->per_bfd;

  create_all_units (m_per_objfile);
  build_type_psymtabs (m_per_objfile, &m_index_storage);

  per_bfd->quick_file_names_table
    = create_quick_file_names_table (per_bfd->all_units.size ());
  if (!per_bfd->debug_aranges.empty ())
    read_addrmap_from_aranges (m_per_objfile, &per_bfd->debug_aranges,
			       m_index_storage.get_addrmap (),
			       &m_warnings);

  /* We want to balance the load between the worker threads.  This is
     done by using the size of each CU as a rough estimate of how
     difficult it will be to operate on.  This isn't ideal -- for
     example if dwz is used, the early CUs will all tend to be
     "included" and won't be parsed independently.  However, this
     heuristic works well for typical compiler output.  */

  size_t total_size = 0;
  for (const auto &per_cu : per_bfd->all_units)
    total_size += per_cu->length ();

  /* How many worker threads we plan to use.  We may not actually use
     this many.  We use 1 as the minimum to avoid division by zero,
     and anyway in the N==0 case the work will be done
     synchronously.  */
  const size_t n_worker_threads
    = std::max (gdb::thread_pool::g_thread_pool->thread_count (), (size_t) 1);

  /* How much effort should be put into each worker.  */
  const size_t size_per_thread
    = std::max (total_size / n_worker_threads, (size_t) 1);

  /* Work is done in a task group.  */
  gdb::task_group workers ([this] ()
  {
    this->done_reading ();
  });

  auto end = per_bfd->all_units.end ();
  size_t task_count = 0;
  for (auto iter = per_bfd->all_units.begin (); iter != end; )
    {
      auto last = iter;
      /* Put all remaining CUs into the last task.  */
      if (task_count == n_worker_threads - 1)
	last = end;
      else
	{
	  size_t chunk_size = 0;
	  for (; last != end && chunk_size < size_per_thread; ++last)
	    chunk_size += (*last)->length ();
	}

      gdb_assert (iter != last);
      workers.add_task ([=] ()
	{
	  process_cus (task_count, iter, last);
	});

      ++task_count;
      iter = last;
    }

  m_results.resize (task_count);
  workers.start ();
}

bool
cooked_index_worker::wait (cooked_state desired_state, bool allow_quit)
{
  bool done;
#if CXX_STD_THREAD
  {
    std::unique_lock<std::mutex> lock (m_mutex);

    /* This may be called from a non-main thread -- this functionality
       is needed for the index cache -- but in this case we require
       that the desired state already have been attained.  */
    gdb_assert (is_main_thread () || desired_state <= m_state);

    while (desired_state > m_state)
      {
	if (allow_quit)
	  {
	    std::chrono::milliseconds duration { 15 };
	    if (m_cond.wait_for (lock, duration) == std::cv_status::timeout)
	      QUIT;
	  }
	else
	  m_cond.wait (lock);
      }
    done = m_state == cooked_state::CACHE_DONE;
  }
#else
  /* Without threads, all the work is done immediately on the main
     thread, and there is never anything to wait for.  */
  done = true;
#endif /* CXX_STD_THREAD */

  /* Only the main thread is allowed to report complaints and the
     like.  */
  if (!is_main_thread ())
    return false;

  if (m_reported)
    return done;
  m_reported = true;

  /* Emit warnings first, maybe they were emitted before an exception
     (if any) was thrown.  */
  m_warnings.emit ();

  if (m_failed.has_value ())
    {
      /* start_reading failed -- report it.  */
      exception_print (gdb_stderr, *m_failed);
      m_failed.reset ();
      return done;
    }

  /* Only show a given exception a single time.  */
  std::unordered_set<gdb_exception> seen_exceptions;
  for (auto &one_result : m_results)
    {
      re_emit_complaints (std::get<1> (one_result));
      for (auto &one_exc : std::get<2> (one_result))
	if (seen_exceptions.insert (one_exc).second)
	  exception_print (gdb_stderr, one_exc);
    }

  if (dwarf_read_debug > 0)
    print_tu_stats (m_per_objfile);

  struct objfile *objfile = m_per_objfile->objfile;
  dwarf2_per_bfd *per_bfd = m_per_objfile->per_bfd;
  cooked_index *table
    = (gdb::checked_static_cast<cooked_index *>
       (per_bfd->index_table.get ()));

  auto_obstack temp_storage;
  enum language lang = language_unknown;
  const char *main_name = table->get_main_name (&temp_storage, &lang);
  if (main_name != nullptr)
    set_objfile_main_name (objfile, main_name, lang);

  dwarf_read_debug_printf ("Done building psymtabs of %s",
			   objfile_name (objfile));

  return done;
}

void
cooked_index_worker::set (cooked_state desired_state)
{
  gdb_assert (desired_state != cooked_state::INITIAL);

#if CXX_STD_THREAD
  std::lock_guard<std::mutex> guard (m_mutex);
  gdb_assert (desired_state > m_state);
  m_state = desired_state;
  m_cond.notify_one ();
#else
  /* Without threads, all the work is done immediately on the main
     thread, and there is never anything to do.  */
#endif /* CXX_STD_THREAD */
}

static void
read_comp_units_from_section (dwarf2_per_objfile *per_objfile,
			      struct dwarf2_section_info *section,
			      struct dwarf2_section_info *abbrev_section,
			      unsigned int is_dwz,
			      htab_up &types_htab,
			      rcuh_kind section_kind)
{
  const gdb_byte *info_ptr;
  struct objfile *objfile = per_objfile->objfile;

  dwarf_read_debug_printf ("Reading %s for %s",
			   section->get_name (),
			   section->get_file_name ());

  section->read (objfile);

  info_ptr = section->buffer;

  while (info_ptr < section->buffer + section->size)
    {
      dwarf2_per_cu_data_up this_cu;

      sect_offset sect_off = (sect_offset) (info_ptr - section->buffer);

      comp_unit_head cu_header;
      read_and_check_comp_unit_head (per_objfile, &cu_header, section,
				     abbrev_section, info_ptr,
				     section_kind);

      /* Save the compilation unit for later lookup.  */
      if (cu_header.unit_type != DW_UT_type)
	this_cu = per_objfile->per_bfd->allocate_per_cu ();
      else
	{
	  if (types_htab == nullptr)
	    types_htab = allocate_signatured_type_table ();

	  auto sig_type = per_objfile->per_bfd->allocate_signatured_type
	    (cu_header.signature);
	  signatured_type *sig_ptr = sig_type.get ();
	  sig_type->type_offset_in_tu = cu_header.type_cu_offset_in_tu;
	  this_cu.reset (sig_type.release ());

	  void **slot = htab_find_slot (types_htab.get (), sig_ptr, INSERT);
	  gdb_assert (slot != nullptr);
	  if (*slot != nullptr)
	    complaint (_("debug type entry at offset %s is duplicate to"
			 " the entry at offset %s, signature %s"),
		       sect_offset_str (sect_off),
		       sect_offset_str (sig_ptr->sect_off),
		       hex_string (sig_ptr->signature));
	  *slot = sig_ptr;
	}
      this_cu->sect_off = sect_off;
      this_cu->set_length (cu_header.get_length_with_initial ());
      this_cu->is_dwz = is_dwz;
      this_cu->section = section;
      /* Init this asap, to avoid a data race in the set_version in
	 cutu_reader::cutu_reader (which may be run in parallel for the cooked
	 index case).  */
      this_cu->set_version (cu_header.version);

      info_ptr = info_ptr + this_cu->length ();
      per_objfile->per_bfd->all_units.push_back (std::move (this_cu));
    }
}

/* Initialize the views on all_units.  */

void
finalize_all_units (dwarf2_per_bfd *per_bfd)
{
  size_t nr_tus = per_bfd->tu_stats.nr_tus;
  size_t nr_cus = per_bfd->all_units.size () - nr_tus;
  gdb::array_view<dwarf2_per_cu_data_up> tmp = per_bfd->all_units;
  per_bfd->all_comp_units = tmp.slice (0, nr_cus);
  per_bfd->all_type_units = tmp.slice (nr_cus, nr_tus);
}

/* See read.h.  */

void
create_all_units (dwarf2_per_objfile *per_objfile)
{
  htab_up types_htab;
  gdb_assert (per_objfile->per_bfd->all_units.empty ());

  read_comp_units_from_section (per_objfile, &per_objfile->per_bfd->info,
				&per_objfile->per_bfd->abbrev, 0,
				types_htab, rcuh_kind::COMPILE);
  for (dwarf2_section_info &section : per_objfile->per_bfd->types)
    read_comp_units_from_section (per_objfile, &section,
				  &per_objfile->per_bfd->abbrev, 0,
				  types_htab, rcuh_kind::TYPE);

  dwz_file *dwz = dwarf2_get_dwz_file (per_objfile->per_bfd);
  if (dwz != NULL)
    {
      read_comp_units_from_section (per_objfile, &dwz->info, &dwz->abbrev, 1,
				    types_htab, rcuh_kind::COMPILE);

      if (!dwz->types.empty ())
	{
	  /* See enhancement PR symtab/30838.  */
	  error (_("Dwarf Error: .debug_types section not supported in dwz file"));
	}
    }

  per_objfile->per_bfd->signatured_types = std::move (types_htab);

  finalize_all_units (per_objfile->per_bfd);
}

/* Return the initial uleb128 in the die at INFO_PTR.  */

static unsigned int
peek_abbrev_code (bfd *abfd, const gdb_byte *info_ptr)
{
  unsigned int bytes_read;

  return read_unsigned_leb128 (abfd, info_ptr, &bytes_read);
}

/* Read the initial uleb128 in the die at INFO_PTR in compilation unit
   READER::CU.  Use READER::ABBREV_TABLE to lookup any abbreviation.

   Return the corresponding abbrev, or NULL if the number is zero (indicating
   an empty DIE).  In either case *BYTES_READ will be set to the length of
   the initial number.  */

static const struct abbrev_info *
peek_die_abbrev (const die_reader_specs &reader,
		 const gdb_byte *info_ptr, unsigned int *bytes_read)
{
  dwarf2_cu *cu = reader.cu;
  bfd *abfd = reader.abfd;
  unsigned int abbrev_number
    = read_unsigned_leb128 (abfd, info_ptr, bytes_read);

  if (abbrev_number == 0)
    return NULL;

  const abbrev_info *abbrev
    = reader.abbrev_table->lookup_abbrev (abbrev_number);
  if (!abbrev)
    {
      error (_("Dwarf Error: Could not find abbrev number %d in %s"
	       " at offset %s [in module %s]"),
	     abbrev_number, cu->per_cu->is_debug_types ? "TU" : "CU",
	     sect_offset_str (cu->header.sect_off), bfd_get_filename (abfd));
    }

  return abbrev;
}

/* Scan the debug information for CU starting at INFO_PTR in buffer BUFFER.
   Returns a pointer to the end of a series of DIEs, terminated by an empty
   DIE.  Any children of the skipped DIEs will also be skipped.  */

static const gdb_byte *
skip_children (const struct die_reader_specs *reader, const gdb_byte *info_ptr)
{
  while (1)
    {
      unsigned int bytes_read;
      const abbrev_info *abbrev = peek_die_abbrev (*reader, info_ptr,
						   &bytes_read);

      if (abbrev == NULL)
	return info_ptr + bytes_read;
      else
	info_ptr = skip_one_die (reader, info_ptr + bytes_read, abbrev);
    }
}

/* Scan the debug information for CU starting at INFO_PTR in buffer BUFFER.
   INFO_PTR should point just after the initial uleb128 of a DIE, and the
   abbrev corresponding to that skipped uleb128 should be passed in
   ABBREV.
   
   If DO_SKIP_CHILDREN is true, or if the DIE has no children, this
   returns a pointer to this DIE's sibling, skipping any children.
   Otherwise, returns a pointer to the DIE's first child.  */

static const gdb_byte *
skip_one_die (const struct die_reader_specs *reader, const gdb_byte *info_ptr,
	      const struct abbrev_info *abbrev, bool do_skip_children)
{
  unsigned int bytes_read;
  struct attribute attr;
  bfd *abfd = reader->abfd;
  struct dwarf2_cu *cu = reader->cu;
  const gdb_byte *buffer = reader->buffer;
  const gdb_byte *buffer_end = reader->buffer_end;
  unsigned int form, i;

  if (do_skip_children && abbrev->sibling_offset != (unsigned short) -1)
    {
      /* We only handle DW_FORM_ref4 here.  */
      const gdb_byte *sibling_data = info_ptr + abbrev->sibling_offset;
      unsigned int offset = read_4_bytes (abfd, sibling_data);
      const gdb_byte *sibling_ptr
	= buffer + to_underlying (cu->header.sect_off) + offset;
      if (sibling_ptr >= info_ptr && sibling_ptr < reader->buffer_end)
	return sibling_ptr;
      /* Fall through to the slow way.  */
    }
  else if (abbrev->size_if_constant != 0)
    {
      info_ptr += abbrev->size_if_constant;
      if (do_skip_children && abbrev->has_children)
	return skip_children (reader, info_ptr);
      return info_ptr;
    }

  for (i = 0; i < abbrev->num_attrs; i++)
    {
      /* The only abbrev we care about is DW_AT_sibling.  */
      if (do_skip_children && abbrev->attrs[i].name == DW_AT_sibling)
	{
	  /* Note there is no need for the extra work of
	     "reprocessing" here, so we pass false for that
	     argument.  */
	  read_attribute (reader, &attr, &abbrev->attrs[i], info_ptr, false);
	  if (attr.form == DW_FORM_ref_addr)
	    complaint (_("ignoring absolute DW_AT_sibling"));
	  else
	    {
	      sect_offset off = attr.get_ref_die_offset ();
	      const gdb_byte *sibling_ptr = buffer + to_underlying (off);

	      if (sibling_ptr < info_ptr)
		complaint (_("DW_AT_sibling points backwards"));
	      else if (sibling_ptr > reader->buffer_end)
		reader->die_section->overflow_complaint ();
	      else
		return sibling_ptr;
	    }
	}

      /* If it isn't DW_AT_sibling, skip this attribute.  */
      form = abbrev->attrs[i].form;
    skip_attribute:
      switch (form)
	{
	case DW_FORM_ref_addr:
	  /* In DWARF 2, DW_FORM_ref_addr is address sized; in DWARF 3
	     and later it is offset sized.  */
	  if (cu->header.version == 2)
	    info_ptr += cu->header.addr_size;
	  else
	    info_ptr += cu->header.offset_size;
	  break;
	case DW_FORM_GNU_ref_alt:
	  info_ptr += cu->header.offset_size;
	  break;
	case DW_FORM_addr:
	  info_ptr += cu->header.addr_size;
	  break;
	case DW_FORM_data1:
	case DW_FORM_ref1:
	case DW_FORM_flag:
	case DW_FORM_strx1:
	  info_ptr += 1;
	  break;
	case DW_FORM_flag_present:
	case DW_FORM_implicit_const:
	  break;
	case DW_FORM_data2:
	case DW_FORM_ref2:
	case DW_FORM_strx2:
	  info_ptr += 2;
	  break;
	case DW_FORM_strx3:
	  info_ptr += 3;
	  break;
	case DW_FORM_data4:
	case DW_FORM_ref4:
	case DW_FORM_strx4:
	  info_ptr += 4;
	  break;
	case DW_FORM_data8:
	case DW_FORM_ref8:
	case DW_FORM_ref_sig8:
	  info_ptr += 8;
	  break;
	case DW_FORM_data16:
	  info_ptr += 16;
	  break;
	case DW_FORM_string:
	  read_direct_string (abfd, info_ptr, &bytes_read);
	  info_ptr += bytes_read;
	  break;
	case DW_FORM_sec_offset:
	case DW_FORM_strp:
	case DW_FORM_GNU_strp_alt:
	  info_ptr += cu->header.offset_size;
	  break;
	case DW_FORM_exprloc:
	case DW_FORM_block:
	  info_ptr += read_unsigned_leb128 (abfd, info_ptr, &bytes_read);
	  info_ptr += bytes_read;
	  break;
	case DW_FORM_block1:
	  info_ptr += 1 + read_1_byte (abfd, info_ptr);
	  break;
	case DW_FORM_block2:
	  info_ptr += 2 + read_2_bytes (abfd, info_ptr);
	  break;
	case DW_FORM_block4:
	  info_ptr += 4 + read_4_bytes (abfd, info_ptr);
	  break;
	case DW_FORM_addrx:
	case DW_FORM_strx:
	case DW_FORM_sdata:
	case DW_FORM_udata:
	case DW_FORM_ref_udata:
	case DW_FORM_GNU_addr_index:
	case DW_FORM_GNU_str_index:
	case DW_FORM_rnglistx:
	case DW_FORM_loclistx:
	  info_ptr = safe_skip_leb128 (info_ptr, buffer_end);
	  break;
	case DW_FORM_indirect:
	  form = read_unsigned_leb128 (abfd, info_ptr, &bytes_read);
	  info_ptr += bytes_read;
	  /* We need to continue parsing from here, so just go back to
	     the top.  */
	  goto skip_attribute;

	default:
	  error (_("Dwarf Error: Cannot handle %s "
		   "in DWARF reader [in module %s]"),
		 dwarf_form_name (form),
		 bfd_get_filename (abfd));
	}
    }

  if (do_skip_children && abbrev->has_children)
    return skip_children (reader, info_ptr);
  else
    return info_ptr;
}

/* Reading in full CUs.  */

/* Add PER_CU to the queue.  */

static void
queue_comp_unit (dwarf2_per_cu_data *per_cu,
		 dwarf2_per_objfile *per_objfile,
		 enum language pretend_language)
{
  per_cu->queued = 1;

  gdb_assert (per_objfile->queue.has_value ());
  per_objfile->queue->emplace (per_cu, per_objfile, pretend_language);
}

/* If PER_CU is not yet expanded of queued for expansion, add it to the queue.

   If DEPENDENT_CU is non-NULL, it has a reference to PER_CU so add a
   dependency.

   Return true if maybe_queue_comp_unit requires the caller to load the CU's
   DIEs, false otherwise.

   Explanation: there is an invariant that if a CU is queued for expansion
   (present in `dwarf2_per_bfd::queue`), then its DIEs are loaded
   (a dwarf2_cu object exists for this CU, and `dwarf2_per_objfile::get_cu`
   returns non-nullptr).  If the CU gets enqueued by this function but its DIEs
   are not yet loaded, the the caller must load the CU's DIEs to ensure the
   invariant is respected.

   The caller is therefore not required to load the CU's DIEs (we return false)
   if:

     - the CU is already expanded, and therefore does not get enqueued
     - the CU gets enqueued for expansion, but its DIEs are already loaded

   Note that the caller should not use this function's return value as an
   indicator of whether the CU's DIEs are loaded right now, it should check
   that by calling `dwarf2_per_objfile::get_cu` instead.  */

static int
maybe_queue_comp_unit (struct dwarf2_cu *dependent_cu,
		       dwarf2_per_cu_data *per_cu,
		       dwarf2_per_objfile *per_objfile,
		       enum language pretend_language)
{
  /* Mark the dependence relation so that we don't flush PER_CU
     too early.  */
  if (dependent_cu != NULL)
    dependent_cu->add_dependence (per_cu);

  /* If it's already on the queue, we have nothing to do.  */
  if (per_cu->queued)
    {
      /* Verify the invariant that if a CU is queued for expansion, its DIEs are
	 loaded.  */
      gdb_assert (per_objfile->get_cu (per_cu) != nullptr);

      /* If the CU is queued for expansion, it should not already be
	 expanded.  */
      gdb_assert (!per_objfile->symtab_set_p (per_cu));

      /* The DIEs are already loaded, the caller doesn't need to do it.  */
      return 0;
    }

  bool queued = false;
  if (!per_objfile->symtab_set_p (per_cu))
    {
      /* Add it to the queue.  */
      queue_comp_unit (per_cu, per_objfile,  pretend_language);
      queued = true;
    }

  /* If the compilation unit is already loaded, just mark it as
     used.  */
  dwarf2_cu *cu = per_objfile->get_cu (per_cu);
  if (cu != nullptr)
    cu->last_used = 0;

  /* Ask the caller to load the CU's DIEs if the CU got enqueued for expansion
     and the DIEs are not already loaded.  */
  return queued && cu == nullptr;
}

/* Process the queue.  */

static void
process_queue (dwarf2_per_objfile *per_objfile)
{
  dwarf_read_debug_printf ("Expanding one or more symtabs of objfile %s ...",
			   objfile_name (per_objfile->objfile));

  /* The queue starts out with one item, but following a DIE reference
     may load a new CU, adding it to the end of the queue.  */
  while (!per_objfile->queue->empty ())
    {
      dwarf2_queue_item &item = per_objfile->queue->front ();
      dwarf2_per_cu_data *per_cu = item.per_cu;

      if (!per_objfile->symtab_set_p (per_cu))
	{
	  dwarf2_cu *cu = per_objfile->get_cu (per_cu);

	  /* Skip dummy CUs.  */
	  if (cu != nullptr)
	    {
	      unsigned int debug_print_threshold;
	      char buf[100];

	      if (per_cu->is_debug_types)
		{
		  struct signatured_type *sig_type =
		    (struct signatured_type *) per_cu;

		  sprintf (buf, "TU %s at offset %s",
			   hex_string (sig_type->signature),
			   sect_offset_str (per_cu->sect_off));
		  /* There can be 100s of TUs.
		     Only print them in verbose mode.  */
		  debug_print_threshold = 2;
		}
	      else
		{
		  sprintf (buf, "CU at offset %s",
			   sect_offset_str (per_cu->sect_off));
		  debug_print_threshold = 1;
		}

	      if (dwarf_read_debug >= debug_print_threshold)
		dwarf_read_debug_printf ("Expanding symtab of %s", buf);

	      if (per_cu->is_debug_types)
		process_full_type_unit (cu, item.pretend_language);
	      else
		process_full_comp_unit (cu, item.pretend_language);

	      if (dwarf_read_debug >= debug_print_threshold)
		dwarf_read_debug_printf ("Done expanding %s", buf);
	    }
	}

      per_cu->queued = 0;
      per_objfile->queue->pop ();
    }

  dwarf_read_debug_printf ("Done expanding symtabs of %s.",
			   objfile_name (per_objfile->objfile));
}

/* Load the DIEs associated with PER_CU into memory.

   In some cases, the caller, while reading partial symbols, will need to load
   the full symbols for the CU for some reason.  It will already have a
   dwarf2_cu object for THIS_CU and pass it as EXISTING_CU, so it can be re-used
   rather than creating a new one.  */

static void
load_full_comp_unit (dwarf2_per_cu_data *this_cu,
		     dwarf2_per_objfile *per_objfile,
		     dwarf2_cu *existing_cu,
		     bool skip_partial,
		     enum language pretend_language)
{
  gdb_assert (! this_cu->is_debug_types);

  cutu_reader reader (this_cu, per_objfile, NULL, existing_cu, skip_partial);
  if (reader.dummy_p)
    return;

  struct dwarf2_cu *cu = reader.cu;
  const gdb_byte *info_ptr = reader.info_ptr;

  gdb_assert (cu->die_hash == NULL);
  cu->die_hash =
    htab_create_alloc_ex (cu->header.get_length_without_initial () / 12,
			  die_info::hash,
			  die_info::eq,
			  NULL,
			  &cu->comp_unit_obstack,
			  hashtab_obstack_allocate,
			  dummy_obstack_deallocate);

  if (reader.comp_unit_die->has_children)
    reader.comp_unit_die->child
      = read_die_and_siblings (&reader, reader.info_ptr,
			       &info_ptr, reader.comp_unit_die);
  cu->dies = reader.comp_unit_die;
  /* comp_unit_die is not stored in die_hash, no need.  */

  /* We try not to read any attributes in this function, because not
     all CUs needed for references have been loaded yet, and symbol
     table processing isn't initialized.  But we have to set the CU language,
     or we won't be able to build types correctly.
     Similarly, if we do not read the producer, we can not apply
     producer-specific interpretation.  */
  prepare_one_comp_unit (cu, cu->dies, pretend_language);

  reader.keep ();
}

/* Add a DIE to the delayed physname list.  */

static void
add_to_method_list (struct type *type, int fnfield_index, int index,
		    const char *name, struct die_info *die,
		    struct dwarf2_cu *cu)
{
  struct delayed_method_info mi;
  mi.type = type;
  mi.fnfield_index = fnfield_index;
  mi.index = index;
  mi.name = name;
  mi.die = die;
  cu->method_list.push_back (mi);
}

/* Check whether [PHYSNAME, PHYSNAME+LEN) ends with a modifier like
   "const" / "volatile".  If so, decrements LEN by the length of the
   modifier and return true.  Otherwise return false.  */

template<size_t N>
static bool
check_modifier (const char *physname, size_t &len, const char (&mod)[N])
{
  size_t mod_len = sizeof (mod) - 1;
  if (len > mod_len && startswith (physname + (len - mod_len), mod))
    {
      len -= mod_len;
      return true;
    }
  return false;
}

/* Compute the physnames of any methods on the CU's method list.

   The computation of method physnames is delayed in order to avoid the
   (bad) condition that one of the method's formal parameters is of an as yet
   incomplete type.  */

static void
compute_delayed_physnames (struct dwarf2_cu *cu)
{
  /* Only C++ delays computing physnames.  */
  if (cu->method_list.empty ())
    return;
  gdb_assert (cu->lang () == language_cplus);

  for (const delayed_method_info &mi : cu->method_list)
    {
      const char *physname;
      struct fn_fieldlist *fn_flp
	= &TYPE_FN_FIELDLIST (mi.type, mi.fnfield_index);
      physname = dwarf2_physname (mi.name, mi.die, cu);
      TYPE_FN_FIELD_PHYSNAME (fn_flp->fn_fields, mi.index)
	= physname ? physname : "";

      /* Since there's no tag to indicate whether a method is a
	 const/volatile overload, extract that information out of the
	 demangled name.  */
      if (physname != NULL)
	{
	  size_t len = strlen (physname);

	  while (1)
	    {
	      if (physname[len - 1] == ')') /* shortcut */
		break;
	      else if (check_modifier (physname, len, " const"))
		TYPE_FN_FIELD_CONST (fn_flp->fn_fields, mi.index) = 1;
	      else if (check_modifier (physname, len, " volatile"))
		TYPE_FN_FIELD_VOLATILE (fn_flp->fn_fields, mi.index) = 1;
	      else
		break;
	    }
	}
    }

  /* The list is no longer needed.  */
  cu->method_list.clear ();
}

/* Go objects should be embedded in a DW_TAG_module DIE,
   and it's not clear if/how imported objects will appear.
   To keep Go support simple until that's worked out,
   go back through what we've read and create something usable.
   We could do this while processing each DIE, and feels kinda cleaner,
   but that way is more invasive.
   This is to, for example, allow the user to type "p var" or "b main"
   without having to specify the package name, and allow lookups
   of module.object to work in contexts that use the expression
   parser.  */

static void
fixup_go_packaging (struct dwarf2_cu *cu)
{
  gdb::unique_xmalloc_ptr<char> package_name;
  struct pending *list;
  int i;

  for (list = *cu->get_builder ()->get_global_symbols ();
       list != NULL;
       list = list->next)
    {
      for (i = 0; i < list->nsyms; ++i)
	{
	  struct symbol *sym = list->symbol[i];

	  if (sym->language () == language_go
	      && sym->aclass () == LOC_BLOCK)
	    {
	      gdb::unique_xmalloc_ptr<char> this_package_name
		= go_symbol_package_name (sym);

	      if (this_package_name == NULL)
		continue;
	      if (package_name == NULL)
		package_name = std::move (this_package_name);
	      else
		{
		  struct objfile *objfile = cu->per_objfile->objfile;
		  if (strcmp (package_name.get (), this_package_name.get ()) != 0)
		    complaint (_("Symtab %s has objects from two different Go packages: %s and %s"),
			       (sym->symtab () != NULL
				? symtab_to_filename_for_display
				(sym->symtab ())
				: objfile_name (objfile)),
			       this_package_name.get (), package_name.get ());
		}
	    }
	}
    }

  if (package_name != NULL)
    {
      struct objfile *objfile = cu->per_objfile->objfile;
      const char *saved_package_name = objfile->intern (package_name.get ());
      struct type *type
	= type_allocator (objfile, cu->lang ()).new_type (TYPE_CODE_MODULE, 0,
							  saved_package_name);
      struct symbol *sym;

      sym = new (&objfile->objfile_obstack) symbol;
      sym->set_language (language_go, &objfile->objfile_obstack);
      sym->compute_and_set_names (saved_package_name, false, objfile->per_bfd);
      /* This is not VAR_DOMAIN because we want a way to ensure a lookup of,
	 e.g., "main" finds the "main" module and not C's main().  */
      sym->set_domain (STRUCT_DOMAIN);
      sym->set_aclass_index (LOC_TYPEDEF);
      sym->set_type (type);

      add_symbol_to_list (sym, cu->get_builder ()->get_global_symbols ());
    }
}

/* Allocate a fully-qualified name consisting of the two parts on the
   obstack.  */

static const char *
rust_fully_qualify (struct obstack *obstack, const char *p1, const char *p2)
{
  return obconcat (obstack, p1, "::", p2, (char *) NULL);
}

/* A helper that allocates a variant part to attach to a Rust enum
   type.  OBSTACK is where the results should be allocated.  TYPE is
   the type we're processing.  DISCRIMINANT_INDEX is the index of the
   discriminant.  It must be the index of one of the fields of TYPE,
   or -1 to mean there is no discriminant (univariant enum).
   DEFAULT_INDEX is the index of the default field; or -1 if there is
   no default.  RANGES is indexed by "effective" field number (the
   field index, but omitting the discriminant and default fields) and
   must hold the discriminant values used by the variants.  Note that
   RANGES must have a lifetime at least as long as OBSTACK -- either
   already allocated on it, or static.  */

static void
alloc_rust_variant (struct obstack *obstack, struct type *type,
		    int discriminant_index, int default_index,
		    gdb::array_view<discriminant_range> ranges)
{
  /* When DISCRIMINANT_INDEX == -1, we have a univariant enum.  */
  gdb_assert (discriminant_index == -1
	      || (discriminant_index >= 0
		  && discriminant_index < type->num_fields ()));
  gdb_assert (default_index == -1
	      || (default_index >= 0 && default_index < type->num_fields ()));

  /* We have one variant for each non-discriminant field.  */
  int n_variants = type->num_fields ();
  if (discriminant_index != -1)
    --n_variants;

  variant *variants = new (obstack) variant[n_variants];
  int var_idx = 0;
  int range_idx = 0;
  for (int i = 0; i < type->num_fields (); ++i)
    {
      if (i == discriminant_index)
	continue;

      variants[var_idx].first_field = i;
      variants[var_idx].last_field = i + 1;

      /* The default field does not need a range, but other fields do.
	 We skipped the discriminant above.  */
      if (i != default_index)
	{
	  variants[var_idx].discriminants = ranges.slice (range_idx, 1);
	  ++range_idx;
	}

      ++var_idx;
    }

  gdb_assert (range_idx == ranges.size ());
  gdb_assert (var_idx == n_variants);

  variant_part *part = new (obstack) variant_part;
  part->discriminant_index = discriminant_index;
  /* If there is no discriminant, then whether it is signed is of no
     consequence.  */
  part->is_unsigned
    = (discriminant_index == -1
       ? false
       : type->field (discriminant_index).type ()->is_unsigned ());
  part->variants = gdb::array_view<variant> (variants, n_variants);

  void *storage = obstack_alloc (obstack, sizeof (gdb::array_view<variant_part>));
  gdb::array_view<variant_part> *prop_value
    = new (storage) gdb::array_view<variant_part> (part, 1);

  struct dynamic_prop prop;
  prop.set_variant_parts (prop_value);

  type->add_dyn_prop (DYN_PROP_VARIANT_PARTS, prop);
}

/* Some versions of rustc emitted enums in an unusual way.

   Ordinary enums were emitted as unions.  The first element of each
   structure in the union was named "RUST$ENUM$DISR".  This element
   held the discriminant.

   These versions of Rust also implemented the "non-zero"
   optimization.  When the enum had two values, and one is empty and
   the other holds a pointer that cannot be zero, the pointer is used
   as the discriminant, with a zero value meaning the empty variant.
   Here, the union's first member is of the form
   RUST$ENCODED$ENUM$<fieldno>$<fieldno>$...$<variantname>
   where the fieldnos are the indices of the fields that should be
   traversed in order to find the field (which may be several fields deep)
   and the variantname is the name of the variant of the case when the
   field is zero.

   This function recognizes whether TYPE is of one of these forms,
   and, if so, smashes it to be a variant type.  */

static void
quirk_rust_enum (struct type *type, struct objfile *objfile)
{
  gdb_assert (type->code () == TYPE_CODE_UNION);

  /* We don't need to deal with empty enums.  */
  if (type->num_fields () == 0)
    return;

#define RUST_ENUM_PREFIX "RUST$ENCODED$ENUM$"
  if (type->num_fields () == 1
      && startswith (type->field (0).name (), RUST_ENUM_PREFIX))
    {
      const char *name = type->field (0).name () + strlen (RUST_ENUM_PREFIX);

      /* Decode the field name to find the offset of the
	 discriminant.  */
      ULONGEST bit_offset = 0;
      struct type *field_type = type->field (0).type ();
      while (name[0] >= '0' && name[0] <= '9')
	{
	  char *tail;
	  unsigned long index = strtoul (name, &tail, 10);
	  name = tail;
	  if (*name != '$'
	      || index >= field_type->num_fields ()
	      || (field_type->field (index).loc_kind ()
		  != FIELD_LOC_KIND_BITPOS))
	    {
	      complaint (_("Could not parse Rust enum encoding string \"%s\""
			   "[in module %s]"),
			 type->field (0).name (),
			 objfile_name (objfile));
	      return;
	    }
	  ++name;

	  bit_offset += field_type->field (index).loc_bitpos ();
	  field_type = field_type->field (index).type ();
	}

      /* Smash this type to be a structure type.  We have to do this
	 because the type has already been recorded.  */
      type->set_code (TYPE_CODE_STRUCT);
      /* Save the field we care about.  */
      struct field saved_field = type->field (0);
      type->alloc_fields (3);

      /* Put the discriminant at index 0.  */
      type->field (0).set_type (field_type);
      type->field (0).set_is_artificial (true);
      type->field (0).set_name ("<<discriminant>>");
      type->field (0).set_loc_bitpos (bit_offset);

      /* The order of fields doesn't really matter, so put the real
	 field at index 1 and the data-less field at index 2.  */
      type->field (1) = saved_field;
      type->field (1).set_name
	(rust_last_path_segment (type->field (1).type ()->name ()));
      type->field (1).type ()->set_name
	(rust_fully_qualify (&objfile->objfile_obstack, type->name (),
			     type->field (1).name ()));

      const char *dataless_name
	= rust_fully_qualify (&objfile->objfile_obstack, type->name (),
			      name);
      struct type *dataless_type
	= type_allocator (type).new_type (TYPE_CODE_VOID, 0,
					  dataless_name);
      type->field (2).set_type (dataless_type);
      /* NAME points into the original discriminant name, which
	 already has the correct lifetime.  */
      type->field (2).set_name (name);
      type->field (2).set_loc_bitpos (0);

      /* Indicate that this is a variant type.  */
      static discriminant_range ranges[1] = { { 0, 0 } };
      alloc_rust_variant (&objfile->objfile_obstack, type, 0, 1, ranges);
    }
  /* A union with a single anonymous field is probably an old-style
     univariant enum.  */
  else if (type->num_fields () == 1 && streq (type->field (0).name (), ""))
    {
      /* Smash this type to be a structure type.  We have to do this
	 because the type has already been recorded.  */
      type->set_code (TYPE_CODE_STRUCT);

      struct type *field_type = type->field (0).type ();
      const char *variant_name
	= rust_last_path_segment (field_type->name ());
      type->field (0).set_name (variant_name);
      field_type->set_name
	(rust_fully_qualify (&objfile->objfile_obstack,
			     type->name (), variant_name));

      alloc_rust_variant (&objfile->objfile_obstack, type, -1, 0, {});
    }
  else
    {
      struct type *disr_type = nullptr;
      for (int i = 0; i < type->num_fields (); ++i)
	{
	  disr_type = type->field (i).type ();

	  if (disr_type->code () != TYPE_CODE_STRUCT)
	    {
	      /* All fields of a true enum will be structs.  */
	      return;
	    }
	  else if (disr_type->num_fields () == 0)
	    {
	      /* Could be data-less variant, so keep going.  */
	      disr_type = nullptr;
	    }
	  else if (strcmp (disr_type->field (0).name (),
			   "RUST$ENUM$DISR") != 0)
	    {
	      /* Not a Rust enum.  */
	      return;
	    }
	  else
	    {
	      /* Found one.  */
	      break;
	    }
	}

      /* If we got here without a discriminant, then it's probably
	 just a union.  */
      if (disr_type == nullptr)
	return;

      /* Smash this type to be a structure type.  We have to do this
	 because the type has already been recorded.  */
      type->set_code (TYPE_CODE_STRUCT);

      /* Make space for the discriminant field.  */
      struct field *disr_field = &disr_type->field (0);
      field *new_fields
	= (struct field *) TYPE_ZALLOC (type, ((type->num_fields () + 1)
					       * sizeof (struct field)));
      memcpy (new_fields + 1, type->fields (),
	      type->num_fields () * sizeof (struct field));
      type->set_fields (new_fields);
      type->set_num_fields (type->num_fields () + 1);

      /* Install the discriminant at index 0 in the union.  */
      type->field (0) = *disr_field;
      type->field (0).set_is_artificial (true);
      type->field (0).set_name ("<<discriminant>>");

      /* We need a way to find the correct discriminant given a
	 variant name.  For convenience we build a map here.  */
      struct type *enum_type = disr_field->type ();
      std::unordered_map<std::string, ULONGEST> discriminant_map;
      for (int i = 0; i < enum_type->num_fields (); ++i)
	{
	  if (enum_type->field (i).loc_kind () == FIELD_LOC_KIND_ENUMVAL)
	    {
	      const char *name
		= rust_last_path_segment (enum_type->field (i).name ());
	      discriminant_map[name] = enum_type->field (i).loc_enumval ();
	    }
	}

      int n_fields = type->num_fields ();
      /* We don't need a range entry for the discriminant, but we do
	 need one for every other field, as there is no default
	 variant.  */
      discriminant_range *ranges = XOBNEWVEC (&objfile->objfile_obstack,
					      discriminant_range,
					      n_fields - 1);
      /* Skip the discriminant here.  */
      for (int i = 1; i < n_fields; ++i)
	{
	  /* Find the final word in the name of this variant's type.
	     That name can be used to look up the correct
	     discriminant.  */
	  const char *variant_name
	    = rust_last_path_segment (type->field (i).type ()->name ());

	  auto iter = discriminant_map.find (variant_name);
	  if (iter != discriminant_map.end ())
	    {
	      ranges[i - 1].low = iter->second;
	      ranges[i - 1].high = iter->second;
	    }

	  /* In Rust, each element should have the size of the
	     enclosing enum.  */
	  type->field (i).type ()->set_length (type->length ());

	  /* Remove the discriminant field, if it exists.  */
	  struct type *sub_type = type->field (i).type ();
	  if (sub_type->num_fields () > 0)
	    {
	      sub_type->set_num_fields (sub_type->num_fields () - 1);
	      sub_type->set_fields (sub_type->fields () + 1);
	    }
	  type->field (i).set_name (variant_name);
	  sub_type->set_name
	    (rust_fully_qualify (&objfile->objfile_obstack,
				 type->name (), variant_name));
	}

      /* Indicate that this is a variant type.  */
      alloc_rust_variant (&objfile->objfile_obstack, type, 0, -1,
			  gdb::array_view<discriminant_range> (ranges,
							       n_fields - 1));
    }
}

/* Rewrite some Rust unions to be structures with variants parts.  */

static void
rust_union_quirks (struct dwarf2_cu *cu)
{
  gdb_assert (cu->lang () == language_rust);
  for (type *type_ : cu->rust_unions)
    quirk_rust_enum (type_, cu->per_objfile->objfile);
  /* We don't need this any more.  */
  cu->rust_unions.clear ();
}

/* See read.h.  */

type_unit_group_unshareable *
dwarf2_per_objfile::get_type_unit_group_unshareable (type_unit_group *tu_group)
{
  auto iter = this->m_type_units.find (tu_group);
  if (iter != this->m_type_units.end ())
    return iter->second.get ();

  type_unit_group_unshareable_up uniq (new type_unit_group_unshareable);
  type_unit_group_unshareable *result = uniq.get ();
  this->m_type_units[tu_group] = std::move (uniq);
  return result;
}

struct type *
dwarf2_per_objfile::get_type_for_signatured_type
  (signatured_type *sig_type) const
{
  auto iter = this->m_type_map.find (sig_type);
  if (iter == this->m_type_map.end ())
    return nullptr;

  return iter->second;
}

void dwarf2_per_objfile::set_type_for_signatured_type
  (signatured_type *sig_type, struct type *type)
{
  gdb_assert (this->m_type_map.find (sig_type) == this->m_type_map.end ());

  this->m_type_map[sig_type] = type;
}

/* A helper function for computing the list of all symbol tables
   included by PER_CU.  */

static void
recursively_compute_inclusions (std::vector<compunit_symtab *> *result,
				htab_t all_children, htab_t all_type_symtabs,
				dwarf2_per_cu_data *per_cu,
				dwarf2_per_objfile *per_objfile,
				struct compunit_symtab *immediate_parent)
{
  void **slot = htab_find_slot (all_children, per_cu, INSERT);
  if (*slot != NULL)
    {
      /* This inclusion and its children have been processed.  */
      return;
    }

  *slot = per_cu;

  /* Only add a CU if it has a symbol table.  */
  compunit_symtab *cust = per_objfile->get_symtab (per_cu);
  if (cust != NULL)
    {
      /* If this is a type unit only add its symbol table if we haven't
	 seen it yet (type unit per_cu's can share symtabs).  */
      if (per_cu->is_debug_types)
	{
	  slot = htab_find_slot (all_type_symtabs, cust, INSERT);
	  if (*slot == NULL)
	    {
	      *slot = cust;
	      result->push_back (cust);
	      if (cust->user == NULL)
		cust->user = immediate_parent;
	    }
	}
      else
	{
	  result->push_back (cust);
	  if (cust->user == NULL)
	    cust->user = immediate_parent;
	}
    }

  if (!per_cu->imported_symtabs_empty ())
    for (dwarf2_per_cu_data *ptr : *per_cu->imported_symtabs)
      {
	recursively_compute_inclusions (result, all_children,
					all_type_symtabs, ptr, per_objfile,
					cust);
      }
}

/* Compute the compunit_symtab 'includes' fields for the compunit_symtab of
   PER_CU.  */

static void
compute_compunit_symtab_includes (dwarf2_per_cu_data *per_cu,
				  dwarf2_per_objfile *per_objfile)
{
  gdb_assert (! per_cu->is_debug_types);

  if (!per_cu->imported_symtabs_empty ())
    {
      int len;
      std::vector<compunit_symtab *> result_symtabs;
      compunit_symtab *cust = per_objfile->get_symtab (per_cu);

      /* If we don't have a symtab, we can just skip this case.  */
      if (cust == NULL)
	return;

      htab_up all_children (htab_create_alloc (1, htab_hash_pointer,
					       htab_eq_pointer,
					       NULL, xcalloc, xfree));
      htab_up all_type_symtabs (htab_create_alloc (1, htab_hash_pointer,
						   htab_eq_pointer,
						   NULL, xcalloc, xfree));

      for (dwarf2_per_cu_data *ptr : *per_cu->imported_symtabs)
	{
	  recursively_compute_inclusions (&result_symtabs, all_children.get (),
					  all_type_symtabs.get (), ptr,
					  per_objfile, cust);
	}

      /* Now we have a transitive closure of all the included symtabs.  */
      len = result_symtabs.size ();
      cust->includes
	= XOBNEWVEC (&per_objfile->objfile->objfile_obstack,
		     struct compunit_symtab *, len + 1);
      memcpy (cust->includes, result_symtabs.data (),
	      len * sizeof (compunit_symtab *));
      cust->includes[len] = NULL;
    }
}

/* Compute the 'includes' field for the symtabs of all the CUs we just
   read.  */

static void
process_cu_includes (dwarf2_per_objfile *per_objfile)
{
  for (dwarf2_per_cu_data *iter : per_objfile->per_bfd->just_read_cus)
    {
      if (! iter->is_debug_types)
	compute_compunit_symtab_includes (iter, per_objfile);
    }

  per_objfile->per_bfd->just_read_cus.clear ();
}

/* Generate full symbol information for CU, whose DIEs have
   already been loaded into memory.  */

static void
process_full_comp_unit (dwarf2_cu *cu, enum language pretend_language)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  unrelocated_addr lowpc, highpc;
  struct compunit_symtab *cust;
  struct block *static_block;
  CORE_ADDR addr;

  /* Clear the list here in case something was left over.  */
  cu->method_list.clear ();

  dwarf2_find_base_address (cu->dies, cu);

  /* Before we start reading the top-level DIE, ensure it has a valid tag
     type.  */
  switch (cu->dies->tag)
    {
    case DW_TAG_compile_unit:
    case DW_TAG_partial_unit:
    case DW_TAG_type_unit:
      break;
    default:
      error (_("Dwarf Error: unexpected tag '%s' at offset %s [in module %s]"),
	     dwarf_tag_name (cu->dies->tag),
	     sect_offset_str (cu->per_cu->sect_off),
	     objfile_name (per_objfile->objfile));
    }

  /* Do line number decoding in read_file_scope () */
  process_die (cu->dies, cu);

  /* For now fudge the Go package.  */
  if (cu->lang () == language_go)
    fixup_go_packaging (cu);

  /* Now that we have processed all the DIEs in the CU, all the types
     should be complete, and it should now be safe to compute all of the
     physnames.  */
  compute_delayed_physnames (cu);

  if (cu->lang () == language_rust)
    rust_union_quirks (cu);

  /* Some compilers don't define a DW_AT_high_pc attribute for the
     compilation unit.  If the DW_AT_high_pc is missing, synthesize
     it, by scanning the DIE's below the compilation unit.  */
  get_scope_pc_bounds (cu->dies, &lowpc, &highpc, cu);

  addr = per_objfile->relocate (highpc);
  static_block
    = cu->get_builder ()->end_compunit_symtab_get_static_block (addr, 0, 1);

  /* If the comp unit has DW_AT_ranges, it may have discontiguous ranges.
     Also, DW_AT_ranges may record ranges not belonging to any child DIEs
     (such as virtual method tables).  Record the ranges in STATIC_BLOCK's
     addrmap to help ensure it has an accurate map of pc values belonging to
     this comp unit.  */
  dwarf2_record_block_ranges (cu->dies, static_block, cu);

  cust = cu->get_builder ()->end_compunit_symtab_from_static_block
    (static_block, 0);

  if (cust != NULL)
    {
      int gcc_4_minor = producer_is_gcc_ge_4 (cu->producer);

      /* Set symtab language to language from DW_AT_language.  If the
	 compilation is from a C file generated by language preprocessors, do
	 not set the language if it was already deduced by start_subfile.  */
      if (!(cu->lang () == language_c
	    && cust->primary_filetab ()->language () != language_unknown))
	cust->primary_filetab ()->set_language (cu->lang ());

      /* GCC-4.0 has started to support -fvar-tracking.  GCC-3.x still can
	 produce DW_AT_location with location lists but it can be possibly
	 invalid without -fvar-tracking.  Still up to GCC-4.4.x incl. 4.4.0
	 there were bugs in prologue debug info, fixed later in GCC-4.5
	 by "unwind info for epilogues" patch (which is not directly related).

	 For -gdwarf-4 type units LOCATIONS_VALID indication is fortunately not
	 needed, it would be wrong due to missing DW_AT_producer there.

	 Still one can confuse GDB by using non-standard GCC compilation
	 options - this waits on GCC PR other/32998 (-frecord-gcc-switches).
	 */
      if (cu->has_loclist && gcc_4_minor >= 5)
	cust->set_locations_valid (true);

      int major, minor;
      if (cu->producer != nullptr
	  && producer_is_gcc (cu->producer, &major, &minor)
	  && (major < 4 || (major == 4 && minor < 5)))
	/* Don't trust gcc < 4.5.x.  */
	cust->set_epilogue_unwind_valid (false);
      else
	cust->set_epilogue_unwind_valid (true);

      cust->set_call_site_htab (cu->call_site_htab);
    }

  per_objfile->set_symtab (cu->per_cu, cust);

  /* Push it for inclusion processing later.  */
  per_objfile->per_bfd->just_read_cus.push_back (cu->per_cu);

  /* Not needed any more.  */
  cu->reset_builder ();
}

/* Generate full symbol information for type unit CU, whose DIEs have
   already been loaded into memory.  */

static void
process_full_type_unit (dwarf2_cu *cu,
			enum language pretend_language)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct compunit_symtab *cust;
  struct signatured_type *sig_type;

  gdb_assert (cu->per_cu->is_debug_types);
  sig_type = (struct signatured_type *) cu->per_cu;

  /* Clear the list here in case something was left over.  */
  cu->method_list.clear ();

  /* The symbol tables are set up in read_type_unit_scope.  */
  process_die (cu->dies, cu);

  /* For now fudge the Go package.  */
  if (cu->lang () == language_go)
    fixup_go_packaging (cu);

  /* Now that we have processed all the DIEs in the CU, all the types
     should be complete, and it should now be safe to compute all of the
     physnames.  */
  compute_delayed_physnames (cu);

  if (cu->lang () == language_rust)
    rust_union_quirks (cu);

  /* TUs share symbol tables.
     If this is the first TU to use this symtab, complete the construction
     of it with end_expandable_symtab.  Otherwise, complete the addition of
     this TU's symbols to the existing symtab.  */
  type_unit_group_unshareable *tug_unshare =
    per_objfile->get_type_unit_group_unshareable (sig_type->type_unit_group);
  if (tug_unshare->compunit_symtab == NULL)
    {
      buildsym_compunit *builder = cu->get_builder ();
      cust = builder->end_expandable_symtab (0);
      tug_unshare->compunit_symtab = cust;

      if (cust != NULL)
	{
	  /* Set symtab language to language from DW_AT_language.  If the
	     compilation is from a C file generated by language preprocessors,
	     do not set the language if it was already deduced by
	     start_subfile.  */
	  if (!(cu->lang () == language_c
		&& cust->primary_filetab ()->language () != language_c))
	    cust->primary_filetab ()->set_language (cu->lang ());
	}
    }
  else
    {
      cu->get_builder ()->augment_type_symtab ();
      cust = tug_unshare->compunit_symtab;
    }

  per_objfile->set_symtab (cu->per_cu, cust);

  /* Not needed any more.  */
  cu->reset_builder ();
}

/* Process an imported unit DIE.  */

static void
process_imported_unit_die (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *attr;

  /* For now we don't handle imported units in type units.  */
  if (cu->per_cu->is_debug_types)
    {
      error (_("Dwarf Error: DW_TAG_imported_unit is not"
	       " supported in type units [in module %s]"),
	     objfile_name (cu->per_objfile->objfile));
    }

  attr = dwarf2_attr (die, DW_AT_import, cu);
  if (attr != NULL)
    {
      sect_offset sect_off = attr->get_ref_die_offset ();
      bool is_dwz = (attr->form == DW_FORM_GNU_ref_alt || cu->per_cu->is_dwz);
      dwarf2_per_objfile *per_objfile = cu->per_objfile;
      dwarf2_per_cu_data *per_cu
	= dwarf2_find_containing_comp_unit (sect_off, is_dwz,
					    per_objfile->per_bfd);

      /* We're importing a C++ compilation unit with tag DW_TAG_compile_unit
	 into another compilation unit, at root level.  Regard this as a hint,
	 and ignore it.  This is a best effort, it only works if unit_type and
	 lang are already set.  */
      if (die->parent && die->parent->parent == NULL
	  && per_cu->unit_type (false) == DW_UT_compile
	  && per_cu->lang (false) == language_cplus)
	return;

      /* If necessary, add it to the queue and load its DIEs.  */
      if (maybe_queue_comp_unit (cu, per_cu, per_objfile,
				 cu->lang ()))
	load_full_comp_unit (per_cu, per_objfile, per_objfile->get_cu (per_cu),
			     false, cu->lang ());

      cu->per_cu->imported_symtabs_push (per_cu);
    }
}

/* RAII object that represents a process_die scope: i.e.,
   starts/finishes processing a DIE.  */
class process_die_scope
{
public:
  process_die_scope (die_info *die, dwarf2_cu *cu)
    : m_die (die), m_cu (cu)
  {
    /* We should only be processing DIEs not already in process.  */
    gdb_assert (!m_die->in_process);
    m_die->in_process = true;
  }

  ~process_die_scope ()
  {
    m_die->in_process = false;

    /* If we're done processing the DIE for the CU that owns the line
       header, we don't need the line header anymore.  */
    if (m_cu->line_header_die_owner == m_die)
      {
	delete m_cu->line_header;
	m_cu->line_header = NULL;
	m_cu->line_header_die_owner = NULL;
      }
  }

private:
  die_info *m_die;
  dwarf2_cu *m_cu;
};

/* Process a die and its children.  */

static void
process_die (struct die_info *die, struct dwarf2_cu *cu)
{
  process_die_scope scope (die, cu);

  switch (die->tag)
    {
    case DW_TAG_padding:
      break;
    case DW_TAG_compile_unit:
    case DW_TAG_partial_unit:
      read_file_scope (die, cu);
      break;
    case DW_TAG_type_unit:
      read_type_unit_scope (die, cu);
      break;
    case DW_TAG_subprogram:
      /* Nested subprograms in Fortran get a prefix.  */
      if (cu->lang () == language_fortran
	  && die->parent != NULL
	  && die->parent->tag == DW_TAG_subprogram)
	cu->processing_has_namespace_info = true;
      [[fallthrough]];
      /* Fall through.  */
    case DW_TAG_entry_point:
    case DW_TAG_inlined_subroutine:
      read_func_scope (die, cu);
      break;
    case DW_TAG_lexical_block:
    case DW_TAG_try_block:
    case DW_TAG_catch_block:
      read_lexical_block_scope (die, cu);
      break;
    case DW_TAG_call_site:
    case DW_TAG_GNU_call_site:
      read_call_site_scope (die, cu);
      break;
    case DW_TAG_class_type:
    case DW_TAG_interface_type:
    case DW_TAG_structure_type:
    case DW_TAG_union_type:
    case DW_TAG_namelist:
      process_structure_scope (die, cu);
      break;
    case DW_TAG_enumeration_type:
      process_enumeration_scope (die, cu);
      break;

    /* These dies have a type, but processing them does not create
       a symbol or recurse to process the children.  Therefore we can
       read them on-demand through read_type_die.  */
    case DW_TAG_subroutine_type:
    case DW_TAG_set_type:
    case DW_TAG_pointer_type:
    case DW_TAG_ptr_to_member_type:
    case DW_TAG_reference_type:
    case DW_TAG_rvalue_reference_type:
    case DW_TAG_string_type:
      break;

    case DW_TAG_array_type:
      /* We only need to handle this case for Ada -- in other
	 languages, it's normal for the compiler to emit a typedef
	 instead.  */
      if (cu->lang () != language_ada)
	break;
      [[fallthrough]];
    case DW_TAG_base_type:
    case DW_TAG_subrange_type:
    case DW_TAG_generic_subrange:
    case DW_TAG_typedef:
      /* Add a typedef symbol for the type definition, if it has a
	 DW_AT_name.  */
      new_symbol (die, read_type_die (die, cu), cu);
      break;
    case DW_TAG_common_block:
      read_common_block (die, cu);
      break;
    case DW_TAG_common_inclusion:
      break;
    case DW_TAG_namespace:
      cu->processing_has_namespace_info = true;
      read_namespace (die, cu);
      break;
    case DW_TAG_module:
      cu->processing_has_namespace_info = true;
      read_module (die, cu);
      break;
    case DW_TAG_imported_declaration:
      cu->processing_has_namespace_info = true;
      if (read_alias (die, cu))
	break;
      /* The declaration is neither a global namespace nor a variable
	 alias.  */
      [[fallthrough]];
    case DW_TAG_imported_module:
      cu->processing_has_namespace_info = true;
      if (die->child != NULL && (die->tag == DW_TAG_imported_declaration
				 || cu->lang () != language_fortran))
	complaint (_("Tag '%s' has unexpected children"),
		   dwarf_tag_name (die->tag));
      read_import_statement (die, cu);
      break;

    case DW_TAG_imported_unit:
      process_imported_unit_die (die, cu);
      break;

    case DW_TAG_variable:
      read_variable (die, cu);
      break;

    default:
      new_symbol (die, NULL, cu);
      break;
    }
}

/* DWARF name computation.  */

/* A helper function for dwarf2_compute_name which determines whether DIE
   needs to have the name of the scope prepended to the name listed in the
   die.  */

static int
die_needs_namespace (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *attr;

  switch (die->tag)
    {
    case DW_TAG_namespace:
    case DW_TAG_typedef:
    case DW_TAG_class_type:
    case DW_TAG_interface_type:
    case DW_TAG_structure_type:
    case DW_TAG_union_type:
    case DW_TAG_enumeration_type:
    case DW_TAG_enumerator:
    case DW_TAG_subprogram:
    case DW_TAG_inlined_subroutine:
    case DW_TAG_entry_point:
    case DW_TAG_member:
    case DW_TAG_imported_declaration:
      return 1;

    case DW_TAG_variable:
    case DW_TAG_constant:
      /* We only need to prefix "globally" visible variables.  These include
	 any variable marked with DW_AT_external or any variable that
	 lives in a namespace.  [Variables in anonymous namespaces
	 require prefixing, but they are not DW_AT_external.]  */

      if (dwarf2_attr (die, DW_AT_specification, cu))
	{
	  struct dwarf2_cu *spec_cu = cu;

	  return die_needs_namespace (die_specification (die, &spec_cu),
				      spec_cu);
	}

      attr = dwarf2_attr (die, DW_AT_external, cu);
      if (attr == NULL && die->parent->tag != DW_TAG_namespace
	  && die->parent->tag != DW_TAG_module)
	return 0;
      /* A variable in a lexical block of some kind does not need a
	 namespace, even though in C++ such variables may be external
	 and have a mangled name.  */
      if (die->parent->tag ==  DW_TAG_lexical_block
	  || die->parent->tag ==  DW_TAG_try_block
	  || die->parent->tag ==  DW_TAG_catch_block
	  || die->parent->tag == DW_TAG_subprogram)
	return 0;
      return 1;

    default:
      return 0;
    }
}

/* Return the DIE's linkage name attribute, either DW_AT_linkage_name
   or DW_AT_MIPS_linkage_name.  Returns NULL if the attribute is not
   defined for the given DIE.  */

static struct attribute *
dw2_linkage_name_attr (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *attr;

  attr = dwarf2_attr (die, DW_AT_linkage_name, cu);
  if (attr == NULL)
    attr = dwarf2_attr (die, DW_AT_MIPS_linkage_name, cu);

  return attr;
}

/* Return the DIE's linkage name as a string, either DW_AT_linkage_name
   or DW_AT_MIPS_linkage_name.  Returns NULL if the attribute is not
   defined for the given DIE.  */

static const char *
dw2_linkage_name (struct die_info *die, struct dwarf2_cu *cu)
{
  const char *linkage_name;

  linkage_name = dwarf2_string_attr (die, DW_AT_linkage_name, cu);
  if (linkage_name == NULL)
    linkage_name = dwarf2_string_attr (die, DW_AT_MIPS_linkage_name, cu);

  /* rustc emits invalid values for DW_AT_linkage_name.  Ignore these.
     See https://github.com/rust-lang/rust/issues/32925.  */
  if (cu->lang () == language_rust && linkage_name != NULL
      && strchr (linkage_name, '{') != NULL)
    linkage_name = NULL;

  return linkage_name;
}

/* Compute the fully qualified name of DIE in CU.  If PHYSNAME is nonzero,
   compute the physname for the object, which include a method's:
   - formal parameters (C++),
   - receiver type (Go),

   The term "physname" is a bit confusing.
   For C++, for example, it is the demangled name.
   For Go, for example, it's the mangled name.

   For Ada, return the DIE's linkage name rather than the fully qualified
   name.  PHYSNAME is ignored..

   The result is allocated on the objfile->per_bfd's obstack and
   canonicalized.  */

static const char *
dwarf2_compute_name (const char *name,
		     struct die_info *die, struct dwarf2_cu *cu,
		     int physname)
{
  struct objfile *objfile = cu->per_objfile->objfile;

  if (name == NULL)
    name = dwarf2_name (die, cu);

  enum language lang = cu->lang ();

  /* For Fortran GDB prefers DW_AT_*linkage_name for the physname if present
     but otherwise compute it by typename_concat inside GDB.
     FIXME: Actually this is not really true, or at least not always true.
     It's all very confusing.  compute_and_set_names doesn't try to demangle
     Fortran names because there is no mangling standard.  So new_symbol
     will set the demangled name to the result of dwarf2_full_name, and it is
     the demangled name that GDB uses if it exists.  */
  if (lang == language_ada
      || (lang == language_fortran && physname))
    {
      /* For Ada unit, we prefer the linkage name over the name, as
	 the former contains the exported name, which the user expects
	 to be able to reference.  Ideally, we want the user to be able
	 to reference this entity using either natural or linkage name,
	 but we haven't started looking at this enhancement yet.  */
      const char *linkage_name = dw2_linkage_name (die, cu);

      if (linkage_name != NULL)
	return linkage_name;
    }

  /* These are the only languages we know how to qualify names in.  */
  if (name != NULL
      && (lang == language_cplus
	  || lang == language_fortran || lang == language_d
	  || lang == language_rust))
    {
      if (die_needs_namespace (die, cu))
	{
	  const char *prefix;

	  string_file buf;

	  prefix = determine_prefix (die, cu);
	  if (*prefix != '\0')
	    {
	      gdb::unique_xmalloc_ptr<char> prefixed_name
		(typename_concat (NULL, prefix, name, physname, cu));

	      buf.puts (prefixed_name.get ());
	    }
	  else
	    buf.puts (name);

	  /* Template parameters may be specified in the DIE's DW_AT_name, or
	     as children with DW_TAG_template_type_param or
	     DW_TAG_value_type_param.  If the latter, add them to the name
	     here.  If the name already has template parameters, then
	     skip this step; some versions of GCC emit both, and
	     it is more efficient to use the pre-computed name.

	     Something to keep in mind about this process: it is very
	     unlikely, or in some cases downright impossible, to produce
	     something that will match the mangled name of a function.
	     If the definition of the function has the same debug info,
	     we should be able to match up with it anyway.  But fallbacks
	     using the minimal symbol, for instance to find a method
	     implemented in a stripped copy of libstdc++, will not work.
	     If we do not have debug info for the definition, we will have to
	     match them up some other way.

	     When we do name matching there is a related problem with function
	     templates; two instantiated function templates are allowed to
	     differ only by their return types, which we do not add here.  */

	  if (lang == language_cplus && strchr (name, '<') == NULL)
	    {
	      struct attribute *attr;
	      struct die_info *child;
	      int first = 1;

	      die->building_fullname = 1;

	      for (child = die->child; child != NULL; child = child->sibling)
		{
		  struct type *type;
		  LONGEST value;
		  const gdb_byte *bytes;
		  struct dwarf2_locexpr_baton *baton;
		  struct value *v;

		  if (child->tag != DW_TAG_template_type_param
		      && child->tag != DW_TAG_template_value_param)
		    continue;

		  if (first)
		    {
		      buf.puts ("<");
		      first = 0;
		    }
		  else
		    buf.puts (", ");

		  attr = dwarf2_attr (child, DW_AT_type, cu);
		  if (attr == NULL)
		    {
		      complaint (_("template parameter missing DW_AT_type"));
		      buf.puts ("UNKNOWN_TYPE");
		      continue;
		    }
		  type = die_type (child, cu);

		  if (child->tag == DW_TAG_template_type_param)
		    {
		      cu->language_defn->print_type (type, "", &buf, -1, 0,
						     &type_print_raw_options);
		      continue;
		    }

		  attr = dwarf2_attr (child, DW_AT_const_value, cu);
		  if (attr == NULL)
		    {
		      complaint (_("template parameter missing "
				   "DW_AT_const_value"));
		      buf.puts ("UNKNOWN_VALUE");
		      continue;
		    }

		  dwarf2_const_value_attr (attr, type, name,
					   &cu->comp_unit_obstack, cu,
					   &value, &bytes, &baton);

		  if (type->has_no_signedness ())
		    /* GDB prints characters as NUMBER 'CHAR'.  If that's
		       changed, this can use value_print instead.  */
		    cu->language_defn->printchar (value, type, &buf);
		  else
		    {
		      struct value_print_options opts;

		      if (baton != NULL)
			v = dwarf2_evaluate_loc_desc (type, NULL,
						      baton->data,
						      baton->size,
						      baton->per_cu,
						      baton->per_objfile);
		      else if (bytes != NULL)
			{
			  v = value::allocate (type);
			  memcpy (v->contents_writeable ().data (), bytes,
				  type->length ());
			}
		      else
			v = value_from_longest (type, value);

		      /* Specify decimal so that we do not depend on
			 the radix.  */
		      get_formatted_print_options (&opts, 'd');
		      opts.raw = true;
		      value_print (v, &buf, &opts);
		      release_value (v);
		    }
		}

	      die->building_fullname = 0;

	      if (!first)
		{
		  /* Close the argument list, with a space if necessary
		     (nested templates).  */
		  if (!buf.empty () && buf.string ().back () == '>')
		    buf.puts (" >");
		  else
		    buf.puts (">");
		}
	    }

	  /* For C++ methods, append formal parameter type
	     information, if PHYSNAME.  */

	  if (physname && die->tag == DW_TAG_subprogram
	      && lang == language_cplus)
	    {
	      struct type *type = read_type_die (die, cu);

	      c_type_print_args (type, &buf, 1, lang,
				 &type_print_raw_options);

	      if (lang == language_cplus)
		{
		  /* Assume that an artificial first parameter is
		     "this", but do not crash if it is not.  RealView
		     marks unnamed (and thus unused) parameters as
		     artificial; there is no way to differentiate
		     the two cases.  */
		  if (type->num_fields () > 0
		      && type->field (0).is_artificial ()
		      && type->field (0).type ()->code () == TYPE_CODE_PTR
		      && TYPE_CONST (type->field (0).type ()->target_type ()))
		    buf.puts (" const");
		}
	    }

	  const std::string &intermediate_name = buf.string ();

	  const char *canonical_name
	    = dwarf2_canonicalize_name (intermediate_name.c_str (), cu,
					objfile);

	  /* If we only computed INTERMEDIATE_NAME, or if
	     INTERMEDIATE_NAME is already canonical, then we need to
	     intern it.  */
	  if (canonical_name == NULL || canonical_name == intermediate_name.c_str ())
	    name = objfile->intern (intermediate_name);
	  else
	    name = canonical_name;
	}
    }

  return name;
}

/* Return the fully qualified name of DIE, based on its DW_AT_name.
   If scope qualifiers are appropriate they will be added.  The result
   will be allocated on the storage_obstack, or NULL if the DIE does
   not have a name.  NAME may either be from a previous call to
   dwarf2_name or NULL.

   The output string will be canonicalized (if C++).  */

static const char *
dwarf2_full_name (const char *name, struct die_info *die, struct dwarf2_cu *cu)
{
  return dwarf2_compute_name (name, die, cu, 0);
}

/* Construct a physname for the given DIE in CU.  NAME may either be
   from a previous call to dwarf2_name or NULL.  The result will be
   allocated on the objfile_objstack or NULL if the DIE does not have a
   name.

   The output string will be canonicalized (if C++).  */

static const char *
dwarf2_physname (const char *name, struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  const char *retval, *mangled = NULL, *canon = NULL;
  int need_copy = 1;

  /* In this case dwarf2_compute_name is just a shortcut not building anything
     on its own.  */
  if (!die_needs_namespace (die, cu))
    return dwarf2_compute_name (name, die, cu, 1);

  if (cu->lang () != language_rust)
    mangled = dw2_linkage_name (die, cu);

  /* DW_AT_linkage_name is missing in some cases - depend on what GDB
     has computed.  */
  gdb::unique_xmalloc_ptr<char> demangled;
  if (mangled != NULL)
    {
      if (cu->language_defn->store_sym_names_in_linkage_form_p ())
	{
	  /* Do nothing (do not demangle the symbol name).  */
	}
      else
	{
	  /* Use DMGL_RET_DROP for C++ template functions to suppress
	     their return type.  It is easier for GDB users to search
	     for such functions as `name(params)' than `long name(params)'.
	     In such case the minimal symbol names do not match the full
	     symbol names but for template functions there is never a need
	     to look up their definition from their declaration so
	     the only disadvantage remains the minimal symbol variant
	     `long name(params)' does not have the proper inferior type.  */
	  demangled = gdb_demangle (mangled, (DMGL_PARAMS | DMGL_ANSI
					      | DMGL_RET_DROP));
	}
      if (demangled)
	canon = demangled.get ();
      else
	{
	  canon = mangled;
	  need_copy = 0;
	}
    }

  if (canon == NULL || check_physname)
    {
      const char *physname = dwarf2_compute_name (name, die, cu, 1);

      if (canon != NULL && strcmp (physname, canon) != 0)
	{
	  /* It may not mean a bug in GDB.  The compiler could also
	     compute DW_AT_linkage_name incorrectly.  But in such case
	     GDB would need to be bug-to-bug compatible.  */

	  complaint (_("Computed physname <%s> does not match demangled <%s> "
		       "(from linkage <%s>) - DIE at %s [in module %s]"),
		     physname, canon, mangled, sect_offset_str (die->sect_off),
		     objfile_name (objfile));

	  /* Prefer DW_AT_linkage_name (in the CANON form) - when it
	     is available here - over computed PHYSNAME.  It is safer
	     against both buggy GDB and buggy compilers.  */

	  retval = canon;
	}
      else
	{
	  retval = physname;
	  need_copy = 0;
	}
    }
  else
    retval = canon;

  if (need_copy)
    retval = objfile->intern (retval);

  return retval;
}

/* Inspect DIE in CU for a namespace alias or a variable with alias
   attribute.  If one exists, record a new symbol for it.

   Returns true if an alias was recorded, false otherwise.  */

static bool
read_alias (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *attr;

  /* If the die does not have a name, this is neither a namespace
     alias nor a variable alias.  */
  attr = dwarf2_attr (die, DW_AT_name, cu);
  if (attr != NULL)
    {
      int num;
      struct die_info *d = die;
      struct dwarf2_cu *imported_cu = cu;

      /* If the compiler has nested DW_AT_imported_declaration DIEs,
	 keep inspecting DIEs until we hit the underlying import.  */
#define MAX_NESTED_IMPORTED_DECLARATIONS 100
      for (num = 0; num  < MAX_NESTED_IMPORTED_DECLARATIONS; ++num)
	{
	  attr = dwarf2_attr (d, DW_AT_import, cu);
	  if (attr == NULL)
	    break;

	  d = follow_die_ref (d, attr, &imported_cu);
	  if (d->tag != DW_TAG_imported_declaration)
	    break;
	}

      if (num == MAX_NESTED_IMPORTED_DECLARATIONS)
	{
	  complaint (_("DIE at %s has too many recursively imported "
		       "declarations"), sect_offset_str (d->sect_off));
	  return false;
	}

      if (attr != NULL)
	{
	  struct type *type;
	  if (d->tag == DW_TAG_variable)
	    {
	      /* This declaration is a C/C++ global variable alias.
		 Add a symbol for it whose type is the same as the
		 aliased variable's.  */
	      type = die_type (d, imported_cu);
	      struct symbol *sym = new_symbol (die, type, cu);
	      attr = dwarf2_attr (d, DW_AT_location, imported_cu);
	      sym->set_aclass_index (LOC_UNRESOLVED);
	      if (attr != nullptr)
		var_decode_location (attr, sym, cu);
	      return true;
	    }
	  else
	    {
	      sect_offset sect_off = attr->get_ref_die_offset ();
	      type = get_die_type_at_offset (sect_off, cu->per_cu,
					     cu->per_objfile);
	      if (type != nullptr && type->code () == TYPE_CODE_NAMESPACE)
		{
		  /* This declaration is a global namespace alias. Add
		     a symbol for it whose type is the aliased
		     namespace.  */
		  new_symbol (die, type, cu);
		  return true;
		}
	    }
	}
    }
  return false;
}

/* Return the using directives repository (global or local?) to use in the
   current context for CU.

   For Ada, imported declarations can materialize renamings, which *may* be
   global.  However it is impossible (for now?) in DWARF to distinguish
   "external" imported declarations and "static" ones.  As all imported
   declarations seem to be static in all other languages, make them all CU-wide
   global only in Ada.  */

static struct using_direct **
using_directives (struct dwarf2_cu *cu)
{
  if (cu->lang () == language_ada
      && cu->get_builder ()->outermost_context_p ())
    return cu->get_builder ()->get_global_using_directives ();
  else
    return cu->get_builder ()->get_local_using_directives ();
}

/* Read the DW_ATTR_decl_line attribute for the given DIE in the
   given CU.  If the format is not recognized or the attribute is
   not present, set it to 0.  */

static unsigned int
read_decl_line (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *decl_line = dwarf2_attr (die, DW_AT_decl_line, cu);
  if (decl_line == nullptr)
    return 0;
  if (decl_line->form_is_constant ())
    {
      LONGEST val = decl_line->constant_value (0);
      if (0 <= val && val <= UINT_MAX)
	return (unsigned int) val;

      complaint (_("Declared line for using directive is too large"));
      return 0;
    }

  complaint (_("Declared line for using directive is of incorrect format"));
  return 0;
}

/* Read the import statement specified by the given die and record it.  */

static void
read_import_statement (struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  struct attribute *import_attr;
  struct die_info *imported_die, *child_die;
  struct dwarf2_cu *imported_cu;
  const char *imported_name;
  const char *imported_name_prefix;
  const char *canonical_name;
  const char *import_alias;
  const char *imported_declaration = NULL;
  const char *import_prefix;
  std::vector<const char *> excludes;

  import_attr = dwarf2_attr (die, DW_AT_import, cu);
  if (import_attr == NULL)
    {
      complaint (_("Tag '%s' has no DW_AT_import"),
		 dwarf_tag_name (die->tag));
      return;
    }

  imported_cu = cu;
  imported_die = follow_die_ref_or_sig (die, import_attr, &imported_cu);
  imported_name = dwarf2_name (imported_die, imported_cu);
  if (imported_name == NULL)
    {
      /* GCC bug: https://bugzilla.redhat.com/show_bug.cgi?id=506524

	The import in the following code:
	namespace A
	  {
	    typedef int B;
	  }

	int main ()
	  {
	    using A::B;
	    B b;
	    return b;
	  }

	...
	 <2><51>: Abbrev Number: 3 (DW_TAG_imported_declaration)
	    <52>   DW_AT_decl_file   : 1
	    <53>   DW_AT_decl_line   : 6
	    <54>   DW_AT_import      : <0x75>
	 <2><58>: Abbrev Number: 4 (DW_TAG_typedef)
	    <59>   DW_AT_name        : B
	    <5b>   DW_AT_decl_file   : 1
	    <5c>   DW_AT_decl_line   : 2
	    <5d>   DW_AT_type        : <0x6e>
	...
	 <1><75>: Abbrev Number: 7 (DW_TAG_base_type)
	    <76>   DW_AT_byte_size   : 4
	    <77>   DW_AT_encoding    : 5        (signed)

	imports the wrong die ( 0x75 instead of 0x58 ).
	This case will be ignored until the gcc bug is fixed.  */
      return;
    }

  /* Figure out the local name after import.  */
  import_alias = dwarf2_name (die, cu);

  /* Figure out where the statement is being imported to.  */
  import_prefix = determine_prefix (die, cu);

  /* Figure out what the scope of the imported die is and prepend it
     to the name of the imported die.  */
  imported_name_prefix = determine_prefix (imported_die, imported_cu);

  if (imported_die->tag != DW_TAG_namespace
      && imported_die->tag != DW_TAG_module)
    {
      imported_declaration = imported_name;
      canonical_name = imported_name_prefix;
    }
  else if (strlen (imported_name_prefix) > 0)
    canonical_name = obconcat (&objfile->objfile_obstack,
			       imported_name_prefix,
			       (cu->lang () == language_d
				? "."
				: "::"),
			       imported_name, (char *) NULL);
  else
    canonical_name = imported_name;

  if (die->tag == DW_TAG_imported_module
      && cu->lang () == language_fortran)
    for (child_die = die->child; child_die && child_die->tag;
	 child_die = child_die->sibling)
      {
	/* DWARF-4: A Fortran use statement with a “rename list” may be
	   represented by an imported module entry with an import attribute
	   referring to the module and owned entries corresponding to those
	   entities that are renamed as part of being imported.  */

	if (child_die->tag != DW_TAG_imported_declaration)
	  {
	    complaint (_("child DW_TAG_imported_declaration expected "
			 "- DIE at %s [in module %s]"),
		       sect_offset_str (child_die->sect_off),
		       objfile_name (objfile));
	    continue;
	  }

	import_attr = dwarf2_attr (child_die, DW_AT_import, cu);
	if (import_attr == NULL)
	  {
	    complaint (_("Tag '%s' has no DW_AT_import"),
		       dwarf_tag_name (child_die->tag));
	    continue;
	  }

	imported_cu = cu;
	imported_die = follow_die_ref_or_sig (child_die, import_attr,
					      &imported_cu);
	imported_name = dwarf2_name (imported_die, imported_cu);
	if (imported_name == NULL)
	  {
	    complaint (_("child DW_TAG_imported_declaration has unknown "
			 "imported name - DIE at %s [in module %s]"),
		       sect_offset_str (child_die->sect_off),
		       objfile_name (objfile));
	    continue;
	  }

	excludes.push_back (imported_name);

	process_die (child_die, cu);
      }

  add_using_directive (using_directives (cu),
		       import_prefix,
		       canonical_name,
		       import_alias,
		       imported_declaration,
		       excludes,
		       read_decl_line (die, cu),
		       0,
		       &objfile->objfile_obstack);
}

/* ICC<14 does not output the required DW_AT_declaration on incomplete
   types, but gives them a size of zero.  Starting with version 14,
   ICC is compatible with GCC.  */

static bool
producer_is_icc_lt_14 (struct dwarf2_cu *cu)
{
  if (!cu->checked_producer)
    check_producer (cu);

  return cu->producer_is_icc_lt_14;
}

/* ICC generates a DW_AT_type for C void functions.  This was observed on
   ICC 14.0.5.212, and appears to be against the DWARF spec (V5 3.3.2)
   which says that void functions should not have a DW_AT_type.  */

static bool
producer_is_icc (struct dwarf2_cu *cu)
{
  if (!cu->checked_producer)
    check_producer (cu);

  return cu->producer_is_icc;
}

/* Check for possibly missing DW_AT_comp_dir with relative .debug_line
   directory paths.  GCC SVN r127613 (new option -fdebug-prefix-map) fixed
   this, it was first present in GCC release 4.3.0.  */

static bool
producer_is_gcc_lt_4_3 (struct dwarf2_cu *cu)
{
  if (!cu->checked_producer)
    check_producer (cu);

  return cu->producer_is_gcc_lt_4_3;
}

/* See dwarf2/read.h.  */
bool
producer_is_clang (struct dwarf2_cu *cu)
{
  if (!cu->checked_producer)
    check_producer (cu);

  return cu->producer_is_clang;
}

static file_and_directory &
find_file_and_directory (struct die_info *die, struct dwarf2_cu *cu)
{
  if (cu->per_cu->fnd != nullptr)
    return *cu->per_cu->fnd;

  /* Find the filename.  Do not use dwarf2_name here, since the filename
     is not a source language identifier.  */
  file_and_directory res (dwarf2_string_attr (die, DW_AT_name, cu),
			  dwarf2_string_attr (die, DW_AT_comp_dir, cu));

  if (res.get_comp_dir () == nullptr
      && producer_is_gcc_lt_4_3 (cu)
      && res.get_name () != nullptr
      && IS_ABSOLUTE_PATH (res.get_name ()))
    {
      res.set_comp_dir (ldirname (res.get_name ()));
      res.set_name (make_unique_xstrdup (lbasename (res.get_name ())));
    }

  cu->per_cu->fnd.reset (new file_and_directory (std::move (res)));
  return *cu->per_cu->fnd;
}

/* Handle DW_AT_stmt_list for a compilation unit.
   DIE is the DW_TAG_compile_unit die for CU.
   COMP_DIR is the compilation directory.  LOWPC is passed to
   dwarf_decode_lines.  See dwarf_decode_lines comments about it.  */

static void
handle_DW_AT_stmt_list (struct die_info *die, struct dwarf2_cu *cu,
			const file_and_directory &fnd, unrelocated_addr lowpc,
			bool have_code) /* ARI: editCase function */
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct attribute *attr;
  hashval_t line_header_local_hash;
  void **slot;
  int decode_mapping;

  gdb_assert (! cu->per_cu->is_debug_types);

  attr = dwarf2_attr (die, DW_AT_stmt_list, cu);
  if (attr == NULL || !attr->form_is_unsigned ())
    return;

  sect_offset line_offset = (sect_offset) attr->as_unsigned ();

  /* The line header hash table is only created if needed (it exists to
     prevent redundant reading of the line table for partial_units).
     If we're given a partial_unit, we'll need it.  If we're given a
     compile_unit, then use the line header hash table if it's already
     created, but don't create one just yet.  */

  if (per_objfile->line_header_hash == NULL
      && die->tag == DW_TAG_partial_unit)
    {
      per_objfile->line_header_hash
	.reset (htab_create_alloc (127, line_header_hash_voidp,
				   line_header_eq_voidp,
				   htab_delete_entry<line_header>,
				   xcalloc, xfree));
    }

  line_header line_header_local (line_offset, cu->per_cu->is_dwz);
  line_header_local_hash = line_header_hash (&line_header_local);
  if (per_objfile->line_header_hash != NULL)
    {
      slot = htab_find_slot_with_hash (per_objfile->line_header_hash.get (),
				       &line_header_local,
				       line_header_local_hash, NO_INSERT);

      /* For DW_TAG_compile_unit we need info like symtab::linetable which
	 is not present in *SLOT (since if there is something in *SLOT then
	 it will be for a partial_unit).  */
      if (die->tag == DW_TAG_partial_unit && slot != NULL)
	{
	  gdb_assert (*slot != NULL);
	  cu->line_header = (struct line_header *) *slot;
	  return;
	}
    }

  /* dwarf_decode_line_header does not yet provide sufficient information.
     We always have to call also dwarf_decode_lines for it.  */
  line_header_up lh = dwarf_decode_line_header (line_offset, cu,
						fnd.get_comp_dir ());
  if (lh == NULL)
    return;

  cu->line_header = lh.release ();
  cu->line_header_die_owner = die;

  if (per_objfile->line_header_hash == NULL)
    slot = NULL;
  else
    {
      slot = htab_find_slot_with_hash (per_objfile->line_header_hash.get (),
				       &line_header_local,
				       line_header_local_hash, INSERT);
      gdb_assert (slot != NULL);
    }
  if (slot != NULL && *slot == NULL)
    {
      /* This newly decoded line number information unit will be owned
	 by line_header_hash hash table.  */
      *slot = cu->line_header;
      cu->line_header_die_owner = NULL;
    }
  else
    {
      /* We cannot free any current entry in (*slot) as that struct line_header
	 may be already used by multiple CUs.  Create only temporary decoded
	 line_header for this CU - it may happen at most once for each line
	 number information unit.  And if we're not using line_header_hash
	 then this is what we want as well.  */
      gdb_assert (die->tag != DW_TAG_partial_unit);
    }
  decode_mapping = (die->tag != DW_TAG_partial_unit);
  /* The have_code check is here because, if LOWPC and HIGHPC are both 0x0,
     then there won't be any interesting code in the CU, but a check later on
     (in lnp_state_machine::check_line_address) will fail to properly exclude
     an entry that was removed via --gc-sections.  */
  dwarf_decode_lines (cu->line_header, cu, lowpc, decode_mapping && have_code);
}

/* Process DW_TAG_compile_unit or DW_TAG_partial_unit.  */

static void
read_file_scope (struct die_info *die, struct dwarf2_cu *cu)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  CORE_ADDR lowpc;
  struct attribute *attr;
  struct die_info *child_die;

  prepare_one_comp_unit (cu, die, cu->lang ());

  unrelocated_addr unrel_low, unrel_high;
  get_scope_pc_bounds (die, &unrel_low, &unrel_high, cu);

  /* If we didn't find a lowpc, set it to highpc to avoid complaints
     from finish_block.  */
  if (unrel_low == ((unrelocated_addr) -1))
    unrel_low = unrel_high;
  lowpc = per_objfile->relocate (unrel_low);

  file_and_directory &fnd = find_file_and_directory (die, cu);

  /* GAS supports generating dwarf-5 info starting version 2.35.  Versions
     2.35-2.37 generate an incorrect CU name attribute: it's relative,
     implicitly prefixing it with the compilation dir.  Work around this by
     prefixing it with the source dir instead.  */
  if (cu->header.version == 5 && !IS_ABSOLUTE_PATH (fnd.get_name ())
      && producer_is_gas_lt_2_38 (cu))
    {
      attr = dwarf2_attr (die, DW_AT_stmt_list, cu);
      if (attr != nullptr && attr->form_is_unsigned ())
	{
	  sect_offset line_offset = (sect_offset) attr->as_unsigned ();
	  line_header_up lh = dwarf_decode_line_header (line_offset, cu,
							fnd.get_comp_dir ());
	  if (lh->version == 5 && lh->is_valid_file_index (1))
	    {
	      std::string dir = lh->include_dir_at (1);
	      fnd.set_comp_dir (std::move (dir));
	    }
	}
    }

  cu->start_compunit_symtab (fnd.get_name (), fnd.intern_comp_dir (objfile),
			     lowpc);

  gdb_assert (per_objfile->sym_cu == nullptr);
  scoped_restore restore_sym_cu
    = make_scoped_restore (&per_objfile->sym_cu, cu);

  /* Decode line number information if present.  We do this before
     processing child DIEs, so that the line header table is available
     for DW_AT_decl_file.  */
  handle_DW_AT_stmt_list (die, cu, fnd, unrel_low, unrel_low != unrel_high);

  /* Process all dies in compilation unit.  */
  if (die->child != NULL)
    {
      child_die = die->child;
      while (child_die && child_die->tag)
	{
	  process_die (child_die, cu);
	  child_die = child_die->sibling;
	}
    }
  per_objfile->sym_cu = nullptr;

  /* Decode macro information, if present.  Dwarf 2 macro information
     refers to information in the line number info statement program
     header, so we can only read it if we've read the header
     successfully.  */
  attr = dwarf2_attr (die, DW_AT_macros, cu);
  if (attr == NULL)
    attr = dwarf2_attr (die, DW_AT_GNU_macros, cu);
  if (attr != nullptr && attr->form_is_unsigned () && cu->line_header)
    {
      if (dwarf2_attr (die, DW_AT_macro_info, cu))
	complaint (_("CU refers to both DW_AT_macros and DW_AT_macro_info"));

      dwarf_decode_macros (cu, attr->as_unsigned (), 1);
    }
  else
    {
      attr = dwarf2_attr (die, DW_AT_macro_info, cu);
      if (attr != nullptr && attr->form_is_unsigned () && cu->line_header)
	{
	  unsigned int macro_offset = attr->as_unsigned ();

	  dwarf_decode_macros (cu, macro_offset, 0);
	}
    }
}

void
dwarf2_cu::setup_type_unit_groups (struct die_info *die)
{
  struct type_unit_group *tu_group;
  int first_time;
  struct attribute *attr;
  unsigned int i;
  struct signatured_type *sig_type;

  gdb_assert (per_cu->is_debug_types);
  sig_type = (struct signatured_type *) per_cu;

  attr = dwarf2_attr (die, DW_AT_stmt_list, this);

  /* If we're using .gdb_index (includes -readnow) then
     per_cu->type_unit_group may not have been set up yet.  */
  if (sig_type->type_unit_group == NULL)
    sig_type->type_unit_group = get_type_unit_group (this, attr);
  tu_group = sig_type->type_unit_group;

  /* If we've already processed this stmt_list there's no real need to
     do it again, we could fake it and just recreate the part we need
     (file name,index -> symtab mapping).  If data shows this optimization
     is useful we can do it then.  */
  type_unit_group_unshareable *tug_unshare
    = per_objfile->get_type_unit_group_unshareable (tu_group);
  first_time = tug_unshare->compunit_symtab == NULL;

  /* We have to handle the case of both a missing DW_AT_stmt_list or bad
     debug info.  */
  line_header_up lh;
  if (attr != NULL && attr->form_is_unsigned ())
    {
      sect_offset line_offset = (sect_offset) attr->as_unsigned ();
      lh = dwarf_decode_line_header (line_offset, this, nullptr);
    }
  if (lh == NULL)
    {
      if (first_time)
	start_compunit_symtab ("", NULL, 0);
      else
	{
	  gdb_assert (tug_unshare->symtabs == NULL);
	  gdb_assert (m_builder == nullptr);
	  struct compunit_symtab *cust = tug_unshare->compunit_symtab;
	  m_builder.reset (new struct buildsym_compunit
			   (cust->objfile (), "",
			    cust->dirname (),
			    cust->language (),
			    0, cust));
	  list_in_scope = get_builder ()->get_file_symbols ();
	}
      return;
    }

  line_header = lh.release ();
  line_header_die_owner = die;

  if (first_time)
    {
      struct compunit_symtab *cust = start_compunit_symtab ("", NULL, 0);

      /* Note: We don't assign tu_group->compunit_symtab yet because we're
	 still initializing it, and our caller (a few levels up)
	 process_full_type_unit still needs to know if this is the first
	 time.  */

      tug_unshare->symtabs
	= XOBNEWVEC (&cust->objfile ()->objfile_obstack,
		     struct symtab *, line_header->file_names_size ());

      auto &file_names = line_header->file_names ();
      for (i = 0; i < file_names.size (); ++i)
	{
	  file_entry &fe = file_names[i];
	  dwarf2_start_subfile (this, fe, *line_header);
	  buildsym_compunit *b = get_builder ();
	  subfile *sf = b->get_current_subfile ();

	  if (sf->symtab == nullptr)
	    {
	      /* NOTE: start_subfile will recognize when it's been
		 passed a file it has already seen.  So we can't
		 assume there's a simple mapping from
		 cu->line_header->file_names to subfiles, plus
		 cu->line_header->file_names may contain dups.  */
	      const char *name = sf->name.c_str ();
	      const char *name_for_id = sf->name_for_id.c_str ();
	      sf->symtab = allocate_symtab (cust, name, name_for_id);
	    }

	  fe.symtab = b->get_current_subfile ()->symtab;
	  tug_unshare->symtabs[i] = fe.symtab;
	}
    }
  else
    {
      gdb_assert (m_builder == nullptr);
      struct compunit_symtab *cust = tug_unshare->compunit_symtab;
      m_builder.reset (new struct buildsym_compunit
		       (cust->objfile (), "",
			cust->dirname (),
			cust->language (),
			0, cust));
      list_in_scope = get_builder ()->get_file_symbols ();

      auto &file_names = line_header->file_names ();
      for (i = 0; i < file_names.size (); ++i)
	{
	  file_entry &fe = file_names[i];
	  fe.symtab = tug_unshare->symtabs[i];
	}
    }

  /* The main symtab is allocated last.  Type units don't have DW_AT_name
     so they don't have a "real" (so to speak) symtab anyway.
     There is later code that will assign the main symtab to all symbols
     that don't have one.  We need to handle the case of a symbol with a
     missing symtab (DW_AT_decl_file) anyway.  */
}

/* Process DW_TAG_type_unit.
   For TUs we want to skip the first top level sibling if it's not the
   actual type being defined by this TU.  In this case the first top
   level sibling is there to provide context only.  */

static void
read_type_unit_scope (struct die_info *die, struct dwarf2_cu *cu)
{
  struct die_info *child_die;

  prepare_one_comp_unit (cu, die, language_minimal);

  /* Initialize (or reinitialize) the machinery for building symtabs.
     We do this before processing child DIEs, so that the line header table
     is available for DW_AT_decl_file.  */
  cu->setup_type_unit_groups (die);

  if (die->child != NULL)
    {
      child_die = die->child;
      while (child_die && child_die->tag)
	{
	  process_die (child_die, cu);
	  child_die = child_die->sibling;
	}
    }
}

/* DWO/DWP files.

   http://gcc.gnu.org/wiki/DebugFission
   http://gcc.gnu.org/wiki/DebugFissionDWP

   To simplify handling of both DWO files ("object" files with the DWARF info)
   and DWP files (a file with the DWOs packaged up into one file), we treat
   DWP files as having a collection of virtual DWO files.  */

/* A helper function to hash two file names.  This is a separate
   function because the hash table uses a search with a different
   type.  The second file may be NULL.  */

static hashval_t
hash_two_files (const char *one, const char *two)
{
  hashval_t hash = htab_hash_string (one);
  if (two != nullptr)
    hash += htab_hash_string (two);
  return hash;
}

static hashval_t
hash_dwo_file (const void *item)
{
  const struct dwo_file *dwo_file = (const struct dwo_file *) item;
  return hash_two_files (dwo_file->dwo_name.c_str (), dwo_file->comp_dir);
}

/* This is used when looking up entries in the DWO hash table.  */

struct dwo_file_search
{
  /* Name of the DWO to look for.  */
  const char *dwo_name;
  /* Compilation directory to look for.  */
  const char *comp_dir;

  /* Return a hash value compatible with the table.  */
  hashval_t hash () const
  {
    return hash_two_files (dwo_name, comp_dir);
  }
};

static int
eq_dwo_file (const void *item_lhs, const void *item_rhs)
{
  const struct dwo_file *lhs = (const struct dwo_file *) item_lhs;
  const struct dwo_file_search *rhs
    = (const struct dwo_file_search *) item_rhs;

  if (lhs->dwo_name != rhs->dwo_name)
    return 0;
  if (lhs->comp_dir == NULL || rhs->comp_dir == NULL)
    return lhs->comp_dir == rhs->comp_dir;
  return strcmp (lhs->comp_dir, rhs->comp_dir) == 0;
}

/* Allocate a hash table for DWO files.  */

static htab_up
allocate_dwo_file_hash_table ()
{
  return htab_up (htab_create_alloc (41,
				     hash_dwo_file,
				     eq_dwo_file,
				     htab_delete_entry<dwo_file>,
				     xcalloc, xfree));
}

/* Lookup DWO file DWO_NAME.  */

static void **
lookup_dwo_file_slot (dwarf2_per_objfile *per_objfile,
		      const char *dwo_name,
		      const char *comp_dir)
{
  struct dwo_file_search find_entry;
  void **slot;

  if (per_objfile->per_bfd->dwo_files == NULL)
    per_objfile->per_bfd->dwo_files = allocate_dwo_file_hash_table ();

  find_entry.dwo_name = dwo_name;
  find_entry.comp_dir = comp_dir;
  slot = htab_find_slot_with_hash (per_objfile->per_bfd->dwo_files.get (),
				   &find_entry, find_entry.hash (),
				   INSERT);

  return slot;
}

static hashval_t
hash_dwo_unit (const void *item)
{
  const struct dwo_unit *dwo_unit = (const struct dwo_unit *) item;

  /* This drops the top 32 bits of the id, but is ok for a hash.  */
  return dwo_unit->signature;
}

static int
eq_dwo_unit (const void *item_lhs, const void *item_rhs)
{
  const struct dwo_unit *lhs = (const struct dwo_unit *) item_lhs;
  const struct dwo_unit *rhs = (const struct dwo_unit *) item_rhs;

  /* The signature is assumed to be unique within the DWO file.
     So while object file CU dwo_id's always have the value zero,
     that's OK, assuming each object file DWO file has only one CU,
     and that's the rule for now.  */
  return lhs->signature == rhs->signature;
}

/* Allocate a hash table for DWO CUs,TUs.
   There is one of these tables for each of CUs,TUs for each DWO file.  */

static htab_up
allocate_dwo_unit_table ()
{
  /* Start out with a pretty small number.
     Generally DWO files contain only one CU and maybe some TUs.  */
  return htab_up (htab_create_alloc (3,
				     hash_dwo_unit,
				     eq_dwo_unit,
				     NULL, xcalloc, xfree));
}

/* die_reader_func for create_dwo_cu.  */

static void
create_dwo_cu_reader (const struct die_reader_specs *reader,
		      const gdb_byte *info_ptr,
		      struct die_info *comp_unit_die,
		      struct dwo_file *dwo_file,
		      struct dwo_unit *dwo_unit)
{
  struct dwarf2_cu *cu = reader->cu;
  sect_offset sect_off = cu->per_cu->sect_off;
  struct dwarf2_section_info *section = cu->per_cu->section;

  std::optional<ULONGEST> signature = lookup_dwo_id (cu, comp_unit_die);
  if (!signature.has_value ())
    {
      complaint (_("Dwarf Error: debug entry at offset %s is missing"
		   " its dwo_id [in module %s]"),
		 sect_offset_str (sect_off), dwo_file->dwo_name.c_str ());
      return;
    }

  dwo_unit->dwo_file = dwo_file;
  dwo_unit->signature = *signature;
  dwo_unit->section = section;
  dwo_unit->sect_off = sect_off;
  dwo_unit->length = cu->per_cu->length ();

  dwarf_read_debug_printf ("  offset %s, dwo_id %s",
			   sect_offset_str (sect_off),
			   hex_string (dwo_unit->signature));
}

/* Create the dwo_units for the CUs in a DWO_FILE.
   Note: This function processes DWO files only, not DWP files.  */

static void
create_cus_hash_table (dwarf2_per_objfile *per_objfile,
		       dwarf2_cu *cu, struct dwo_file &dwo_file,
		       dwarf2_section_info &section, htab_up &cus_htab)
{
  struct objfile *objfile = per_objfile->objfile;
  dwarf2_per_bfd *per_bfd = per_objfile->per_bfd;
  const gdb_byte *info_ptr, *end_ptr;

  section.read (objfile);
  info_ptr = section.buffer;

  if (info_ptr == NULL)
    return;

  dwarf_read_debug_printf ("Reading %s for %s:",
			   section.get_name (),
			   section.get_file_name ());

  end_ptr = info_ptr + section.size;
  while (info_ptr < end_ptr)
    {
      struct dwarf2_per_cu_data per_cu;
      struct dwo_unit read_unit {};
      struct dwo_unit *dwo_unit;
      void **slot;
      sect_offset sect_off = (sect_offset) (info_ptr - section.buffer);

      per_cu.per_bfd = per_bfd;
      per_cu.is_debug_types = 0;
      per_cu.sect_off = sect_offset (info_ptr - section.buffer);
      per_cu.section = &section;

      cutu_reader reader (&per_cu, per_objfile, cu, &dwo_file);
      if (!reader.dummy_p)
	create_dwo_cu_reader (&reader, reader.info_ptr, reader.comp_unit_die,
			      &dwo_file, &read_unit);
      info_ptr += per_cu.length ();

      // If the unit could not be parsed, skip it.
      if (read_unit.dwo_file == NULL)
	continue;

      if (cus_htab == NULL)
	cus_htab = allocate_dwo_unit_table ();

      dwo_unit = OBSTACK_ZALLOC (&per_bfd->obstack,
				 struct dwo_unit);
      *dwo_unit = read_unit;
      slot = htab_find_slot (cus_htab.get (), dwo_unit, INSERT);
      gdb_assert (slot != NULL);
      if (*slot != NULL)
	{
	  const struct dwo_unit *dup_cu = (const struct dwo_unit *)*slot;
	  sect_offset dup_sect_off = dup_cu->sect_off;

	  complaint (_("debug cu entry at offset %s is duplicate to"
		       " the entry at offset %s, signature %s"),
		     sect_offset_str (sect_off), sect_offset_str (dup_sect_off),
		     hex_string (dwo_unit->signature));
	}
      *slot = (void *)dwo_unit;
    }
}

/* DWP file .debug_{cu,tu}_index section format:
   [ref: http://gcc.gnu.org/wiki/DebugFissionDWP]
   [ref: http://dwarfstd.org/doc/DWARF5.pdf, sect 7.3.5 "DWARF Package Files"]

   DWP Versions 1 & 2 are older, pre-standard format versions.  The first
   officially standard DWP format was published with DWARF v5 and is called
   Version 5.  There are no versions 3 or 4.

   DWP Version 1:

   Both index sections have the same format, and serve to map a 64-bit
   signature to a set of section numbers.  Each section begins with a header,
   followed by a hash table of 64-bit signatures, a parallel table of 32-bit
   indexes, and a pool of 32-bit section numbers.  The index sections will be
   aligned at 8-byte boundaries in the file.

   The index section header consists of:

    V, 32 bit version number
    -, 32 bits unused
    N, 32 bit number of compilation units or type units in the index
    M, 32 bit number of slots in the hash table

   Numbers are recorded using the byte order of the application binary.

   The hash table begins at offset 16 in the section, and consists of an array
   of M 64-bit slots.  Each slot contains a 64-bit signature (using the byte
   order of the application binary).  Unused slots in the hash table are 0.
   (We rely on the extreme unlikeliness of a signature being exactly 0.)

   The parallel table begins immediately after the hash table
   (at offset 16 + 8 * M from the beginning of the section), and consists of an
   array of 32-bit indexes (using the byte order of the application binary),
   corresponding 1-1 with slots in the hash table.  Each entry in the parallel
   table contains a 32-bit index into the pool of section numbers.  For unused
   hash table slots, the corresponding entry in the parallel table will be 0.

   The pool of section numbers begins immediately following the hash table
   (at offset 16 + 12 * M from the beginning of the section).  The pool of
   section numbers consists of an array of 32-bit words (using the byte order
   of the application binary).  Each item in the array is indexed starting
   from 0.  The hash table entry provides the index of the first section
   number in the set.  Additional section numbers in the set follow, and the
   set is terminated by a 0 entry (section number 0 is not used in ELF).

   In each set of section numbers, the .debug_info.dwo or .debug_types.dwo
   section must be the first entry in the set, and the .debug_abbrev.dwo must
   be the second entry. Other members of the set may follow in any order.

   ---

   DWP Versions 2 and 5:

   DWP Versions 2 and 5 combine all the .debug_info, etc. sections into one,
   and the entries in the index tables are now offsets into these sections.
   CU offsets begin at 0.  TU offsets begin at the size of the .debug_info
   section.

   Index Section Contents:
    Header
    Hash Table of Signatures   dwp_hash_table.hash_table
    Parallel Table of Indices  dwp_hash_table.unit_table
    Table of Section Offsets   dwp_hash_table.{v2|v5}.{section_ids,offsets}
    Table of Section Sizes     dwp_hash_table.{v2|v5}.sizes

   The index section header consists of:

    V, 32 bit version number
    L, 32 bit number of columns in the table of section offsets
    N, 32 bit number of compilation units or type units in the index
    M, 32 bit number of slots in the hash table

   Numbers are recorded using the byte order of the application binary.

   The hash table has the same format as version 1.
   The parallel table of indices has the same format as version 1,
   except that the entries are origin-1 indices into the table of sections
   offsets and the table of section sizes.

   The table of offsets begins immediately following the parallel table
   (at offset 16 + 12 * M from the beginning of the section).  The table is
   a two-dimensional array of 32-bit words (using the byte order of the
   application binary), with L columns and N+1 rows, in row-major order.
   Each row in the array is indexed starting from 0.  The first row provides
   a key to the remaining rows: each column in this row provides an identifier
   for a debug section, and the offsets in the same column of subsequent rows
   refer to that section.  The section identifiers for Version 2 are:

    DW_SECT_INFO         1  .debug_info.dwo
    DW_SECT_TYPES        2  .debug_types.dwo
    DW_SECT_ABBREV       3  .debug_abbrev.dwo
    DW_SECT_LINE         4  .debug_line.dwo
    DW_SECT_LOC          5  .debug_loc.dwo
    DW_SECT_STR_OFFSETS  6  .debug_str_offsets.dwo
    DW_SECT_MACINFO      7  .debug_macinfo.dwo
    DW_SECT_MACRO        8  .debug_macro.dwo

   The section identifiers for Version 5 are:

    DW_SECT_INFO_V5         1  .debug_info.dwo
    DW_SECT_RESERVED_V5     2  --
    DW_SECT_ABBREV_V5       3  .debug_abbrev.dwo
    DW_SECT_LINE_V5         4  .debug_line.dwo
    DW_SECT_LOCLISTS_V5     5  .debug_loclists.dwo
    DW_SECT_STR_OFFSETS_V5  6  .debug_str_offsets.dwo
    DW_SECT_MACRO_V5        7  .debug_macro.dwo
    DW_SECT_RNGLISTS_V5     8  .debug_rnglists.dwo

   The offsets provided by the CU and TU index sections are the base offsets
   for the contributions made by each CU or TU to the corresponding section
   in the package file.  Each CU and TU header contains an abbrev_offset
   field, used to find the abbreviations table for that CU or TU within the
   contribution to the .debug_abbrev.dwo section for that CU or TU, and should
   be interpreted as relative to the base offset given in the index section.
   Likewise, offsets into .debug_line.dwo from DW_AT_stmt_list attributes
   should be interpreted as relative to the base offset for .debug_line.dwo,
   and offsets into other debug sections obtained from DWARF attributes should
   also be interpreted as relative to the corresponding base offset.

   The table of sizes begins immediately following the table of offsets.
   Like the table of offsets, it is a two-dimensional array of 32-bit words,
   with L columns and N rows, in row-major order.  Each row in the array is
   indexed starting from 1 (row 0 is shared by the two tables).

   ---

   Hash table lookup is handled the same in version 1 and 2:

   We assume that N and M will not exceed 2^32 - 1.
   The size of the hash table, M, must be 2^k such that 2^k > 3*N/2.

   Given a 64-bit compilation unit signature or a type signature S, an entry
   in the hash table is located as follows:

   1) Calculate a primary hash H = S & MASK(k), where MASK(k) is a mask with
      the low-order k bits all set to 1.

   2) Calculate a secondary hash H' = (((S >> 32) & MASK(k)) | 1).

   3) If the hash table entry at index H matches the signature, use that
      entry.  If the hash table entry at index H is unused (all zeroes),
      terminate the search: the signature is not present in the table.

   4) Let H = (H + H') modulo M. Repeat at Step 3.

   Because M > N and H' and M are relatively prime, the search is guaranteed
   to stop at an unused slot or find the match.  */

/* Create a hash table to map DWO IDs to their CU/TU entry in
   .debug_{info,types}.dwo in DWP_FILE.
   Returns NULL if there isn't one.
   Note: This function processes DWP files only, not DWO files.  */

static struct dwp_hash_table *
create_dwp_hash_table (dwarf2_per_objfile *per_objfile,
		       struct dwp_file *dwp_file, int is_debug_types)
{
  struct objfile *objfile = per_objfile->objfile;
  bfd *dbfd = dwp_file->dbfd.get ();
  const gdb_byte *index_ptr, *index_end;
  struct dwarf2_section_info *index;
  uint32_t version, nr_columns, nr_units, nr_slots;
  struct dwp_hash_table *htab;

  if (is_debug_types)
    index = &dwp_file->sections.tu_index;
  else
    index = &dwp_file->sections.cu_index;

  if (index->empty ())
    return NULL;
  index->read (objfile);

  index_ptr = index->buffer;
  index_end = index_ptr + index->size;

  /* For Version 5, the version is really 2 bytes of data & 2 bytes of padding.
     For now it's safe to just read 4 bytes (particularly as it's difficult to
     tell if you're dealing with Version 5 before you've read the version).   */
  version = read_4_bytes (dbfd, index_ptr);
  index_ptr += 4;
  if (version == 2 || version == 5)
    nr_columns = read_4_bytes (dbfd, index_ptr);
  else
    nr_columns = 0;
  index_ptr += 4;
  nr_units = read_4_bytes (dbfd, index_ptr);
  index_ptr += 4;
  nr_slots = read_4_bytes (dbfd, index_ptr);
  index_ptr += 4;

  if (version != 1 && version != 2 && version != 5)
    {
      error (_("Dwarf Error: unsupported DWP file version (%s)"
	       " [in module %s]"),
	     pulongest (version), dwp_file->name);
    }
  if (nr_slots != (nr_slots & -nr_slots))
    {
      error (_("Dwarf Error: number of slots in DWP hash table (%s)"
	       " is not power of 2 [in module %s]"),
	     pulongest (nr_slots), dwp_file->name);
    }

  htab = OBSTACK_ZALLOC (&per_objfile->per_bfd->obstack, struct dwp_hash_table);
  htab->version = version;
  htab->nr_columns = nr_columns;
  htab->nr_units = nr_units;
  htab->nr_slots = nr_slots;
  htab->hash_table = index_ptr;
  htab->unit_table = htab->hash_table + sizeof (uint64_t) * nr_slots;

  /* Exit early if the table is empty.  */
  if (nr_slots == 0 || nr_units == 0
      || (version == 2 && nr_columns == 0)
      || (version == 5 && nr_columns == 0))
    {
      /* All must be zero.  */
      if (nr_slots != 0 || nr_units != 0
	  || (version == 2 && nr_columns != 0)
	  || (version == 5 && nr_columns != 0))
	{
	  complaint (_("Empty DWP but nr_slots,nr_units,nr_columns not"
		       " all zero [in modules %s]"),
		     dwp_file->name);
	}
      return htab;
    }

  if (version == 1)
    {
      htab->section_pool.v1.indices =
	htab->unit_table + sizeof (uint32_t) * nr_slots;
      /* It's harder to decide whether the section is too small in v1.
	 V1 is deprecated anyway so we punt.  */
    }
  else if (version == 2)
    {
      const gdb_byte *ids_ptr = htab->unit_table + sizeof (uint32_t) * nr_slots;
      int *ids = htab->section_pool.v2.section_ids;
      size_t sizeof_ids = sizeof (htab->section_pool.v2.section_ids);
      /* Reverse map for error checking.  */
      int ids_seen[DW_SECT_MAX + 1];
      int i;

      if (nr_columns < 2)
	{
	  error (_("Dwarf Error: bad DWP hash table, too few columns"
		   " in section table [in module %s]"),
		 dwp_file->name);
	}
      if (nr_columns > MAX_NR_V2_DWO_SECTIONS)
	{
	  error (_("Dwarf Error: bad DWP hash table, too many columns"
		   " in section table [in module %s]"),
		 dwp_file->name);
	}
      memset (ids, 255, sizeof_ids);
      memset (ids_seen, 255, sizeof (ids_seen));
      for (i = 0; i < nr_columns; ++i)
	{
	  int id = read_4_bytes (dbfd, ids_ptr + i * sizeof (uint32_t));

	  if (id < DW_SECT_MIN || id > DW_SECT_MAX)
	    {
	      error (_("Dwarf Error: bad DWP hash table, bad section id %d"
		       " in section table [in module %s]"),
		     id, dwp_file->name);
	    }
	  if (ids_seen[id] != -1)
	    {
	      error (_("Dwarf Error: bad DWP hash table, duplicate section"
		       " id %d in section table [in module %s]"),
		     id, dwp_file->name);
	    }
	  ids_seen[id] = i;
	  ids[i] = id;
	}
      /* Must have exactly one info or types section.  */
      if (((ids_seen[DW_SECT_INFO] != -1)
	   + (ids_seen[DW_SECT_TYPES] != -1))
	  != 1)
	{
	  error (_("Dwarf Error: bad DWP hash table, missing/duplicate"
		   " DWO info/types section [in module %s]"),
		 dwp_file->name);
	}
      /* Must have an abbrev section.  */
      if (ids_seen[DW_SECT_ABBREV] == -1)
	{
	  error (_("Dwarf Error: bad DWP hash table, missing DWO abbrev"
		   " section [in module %s]"),
		 dwp_file->name);
	}
      htab->section_pool.v2.offsets = ids_ptr + sizeof (uint32_t) * nr_columns;
      htab->section_pool.v2.sizes =
	htab->section_pool.v2.offsets + (sizeof (uint32_t)
					 * nr_units * nr_columns);
      if ((htab->section_pool.v2.sizes + (sizeof (uint32_t)
					  * nr_units * nr_columns))
	  > index_end)
	{
	  error (_("Dwarf Error: DWP index section is corrupt (too small)"
		   " [in module %s]"),
		 dwp_file->name);
	}
    }
  else /* version == 5  */
    {
      const gdb_byte *ids_ptr = htab->unit_table + sizeof (uint32_t) * nr_slots;
      int *ids = htab->section_pool.v5.section_ids;
      size_t sizeof_ids = sizeof (htab->section_pool.v5.section_ids);
      /* Reverse map for error checking.  */
      int ids_seen[DW_SECT_MAX_V5 + 1];

      if (nr_columns < 2)
	{
	  error (_("Dwarf Error: bad DWP hash table, too few columns"
		   " in section table [in module %s]"),
		 dwp_file->name);
	}
      if (nr_columns > MAX_NR_V5_DWO_SECTIONS)
	{
	  error (_("Dwarf Error: bad DWP hash table, too many columns"
		   " in section table [in module %s]"),
		 dwp_file->name);
	}
      memset (ids, 255, sizeof_ids);
      memset (ids_seen, 255, sizeof (ids_seen));
      for (int i = 0; i < nr_columns; ++i)
	{
	  int id = read_4_bytes (dbfd, ids_ptr + i * sizeof (uint32_t));

	  if (id < DW_SECT_MIN || id > DW_SECT_MAX_V5)
	    {
	      error (_("Dwarf Error: bad DWP hash table, bad section id %d"
		       " in section table [in module %s]"),
		     id, dwp_file->name);
	    }
	  if (ids_seen[id] != -1)
	    {
	      error (_("Dwarf Error: bad DWP hash table, duplicate section"
		       " id %d in section table [in module %s]"),
		     id, dwp_file->name);
	    }
	  ids_seen[id] = i;
	  ids[i] = id;
	}
      /* Must have seen an info section.  */
      if (ids_seen[DW_SECT_INFO_V5] == -1)
	{
	  error (_("Dwarf Error: bad DWP hash table, missing/duplicate"
		   " DWO info/types section [in module %s]"),
		 dwp_file->name);
	}
      /* Must have an abbrev section.  */
      if (ids_seen[DW_SECT_ABBREV_V5] == -1)
	{
	  error (_("Dwarf Error: bad DWP hash table, missing DWO abbrev"
		   " section [in module %s]"),
		 dwp_file->name);
	}
      htab->section_pool.v5.offsets = ids_ptr + sizeof (uint32_t) * nr_columns;
      htab->section_pool.v5.sizes
	= htab->section_pool.v5.offsets + (sizeof (uint32_t)
					 * nr_units * nr_columns);
      if ((htab->section_pool.v5.sizes + (sizeof (uint32_t)
					  * nr_units * nr_columns))
	  > index_end)
	{
	  error (_("Dwarf Error: DWP index section is corrupt (too small)"
		   " [in module %s]"),
		 dwp_file->name);
	}
    }

  return htab;
}

/* Update SECTIONS with the data from SECTP.

   This function is like the other "locate" section routines, but in
   this context the sections to read comes from the DWP V1 hash table,
   not the full ELF section table.

   The result is non-zero for success, or zero if an error was found.  */

static int
locate_v1_virtual_dwo_sections (asection *sectp,
				struct virtual_v1_dwo_sections *sections)
{
  const struct dwop_section_names *names = &dwop_section_names;

  if (names->abbrev_dwo.matches (sectp->name))
    {
      /* There can be only one.  */
      if (sections->abbrev.s.section != NULL)
	return 0;
      sections->abbrev.s.section = sectp;
      sections->abbrev.size = bfd_section_size (sectp);
    }
  else if (names->info_dwo.matches (sectp->name)
	   || names->types_dwo.matches (sectp->name))
    {
      /* There can be only one.  */
      if (sections->info_or_types.s.section != NULL)
	return 0;
      sections->info_or_types.s.section = sectp;
      sections->info_or_types.size = bfd_section_size (sectp);
    }
  else if (names->line_dwo.matches (sectp->name))
    {
      /* There can be only one.  */
      if (sections->line.s.section != NULL)
	return 0;
      sections->line.s.section = sectp;
      sections->line.size = bfd_section_size (sectp);
    }
  else if (names->loc_dwo.matches (sectp->name))
    {
      /* There can be only one.  */
      if (sections->loc.s.section != NULL)
	return 0;
      sections->loc.s.section = sectp;
      sections->loc.size = bfd_section_size (sectp);
    }
  else if (names->macinfo_dwo.matches (sectp->name))
    {
      /* There can be only one.  */
      if (sections->macinfo.s.section != NULL)
	return 0;
      sections->macinfo.s.section = sectp;
      sections->macinfo.size = bfd_section_size (sectp);
    }
  else if (names->macro_dwo.matches (sectp->name))
    {
      /* There can be only one.  */
      if (sections->macro.s.section != NULL)
	return 0;
      sections->macro.s.section = sectp;
      sections->macro.size = bfd_section_size (sectp);
    }
  else if (names->str_offsets_dwo.matches (sectp->name))
    {
      /* There can be only one.  */
      if (sections->str_offsets.s.section != NULL)
	return 0;
      sections->str_offsets.s.section = sectp;
      sections->str_offsets.size = bfd_section_size (sectp);
    }
  else
    {
      /* No other kind of section is valid.  */
      return 0;
    }

  return 1;
}

/* Create a dwo_unit object for the DWO unit with signature SIGNATURE.
   UNIT_INDEX is the index of the DWO unit in the DWP hash table.
   COMP_DIR is the DW_AT_comp_dir attribute of the referencing CU.
   This is for DWP version 1 files.  */

static struct dwo_unit *
create_dwo_unit_in_dwp_v1 (dwarf2_per_objfile *per_objfile,
			   struct dwp_file *dwp_file,
			   uint32_t unit_index,
			   const char *comp_dir,
			   ULONGEST signature, int is_debug_types)
{
  const struct dwp_hash_table *dwp_htab =
    is_debug_types ? dwp_file->tus : dwp_file->cus;
  bfd *dbfd = dwp_file->dbfd.get ();
  const char *kind = is_debug_types ? "TU" : "CU";
  struct dwo_file *dwo_file;
  struct dwo_unit *dwo_unit;
  struct virtual_v1_dwo_sections sections;
  void **dwo_file_slot;
  int i;

  gdb_assert (dwp_file->version == 1);

  dwarf_read_debug_printf ("Reading %s %s/%s in DWP V1 file: %s",
			   kind, pulongest (unit_index), hex_string (signature),
			   dwp_file->name);

  /* Fetch the sections of this DWO unit.
     Put a limit on the number of sections we look for so that bad data
     doesn't cause us to loop forever.  */

#define MAX_NR_V1_DWO_SECTIONS \
  (1 /* .debug_info or .debug_types */ \
   + 1 /* .debug_abbrev */ \
   + 1 /* .debug_line */ \
   + 1 /* .debug_loc */ \
   + 1 /* .debug_str_offsets */ \
   + 1 /* .debug_macro or .debug_macinfo */ \
   + 1 /* trailing zero */)

  memset (&sections, 0, sizeof (sections));

  for (i = 0; i < MAX_NR_V1_DWO_SECTIONS; ++i)
    {
      asection *sectp;
      uint32_t section_nr =
	read_4_bytes (dbfd,
		      dwp_htab->section_pool.v1.indices
		      + (unit_index + i) * sizeof (uint32_t));

      if (section_nr == 0)
	break;
      if (section_nr >= dwp_file->num_sections)
	{
	  error (_("Dwarf Error: bad DWP hash table, section number too large"
		   " [in module %s]"),
		 dwp_file->name);
	}

      sectp = dwp_file->elf_sections[section_nr];
      if (! locate_v1_virtual_dwo_sections (sectp, &sections))
	{
	  error (_("Dwarf Error: bad DWP hash table, invalid section found"
		   " [in module %s]"),
		 dwp_file->name);
	}
    }

  if (i < 2
      || sections.info_or_types.empty ()
      || sections.abbrev.empty ())
    {
      error (_("Dwarf Error: bad DWP hash table, missing DWO sections"
	       " [in module %s]"),
	     dwp_file->name);
    }
  if (i == MAX_NR_V1_DWO_SECTIONS)
    {
      error (_("Dwarf Error: bad DWP hash table, too many DWO sections"
	       " [in module %s]"),
	     dwp_file->name);
    }

  /* It's easier for the rest of the code if we fake a struct dwo_file and
     have dwo_unit "live" in that.  At least for now.

     The DWP file can be made up of a random collection of CUs and TUs.
     However, for each CU + set of TUs that came from the same original DWO
     file, we can combine them back into a virtual DWO file to save space
     (fewer struct dwo_file objects to allocate).  Remember that for really
     large apps there can be on the order of 8K CUs and 200K TUs, or more.  */

  std::string virtual_dwo_name =
    string_printf ("virtual-dwo/%d-%d-%d-%d",
		   sections.abbrev.get_id (),
		   sections.line.get_id (),
		   sections.loc.get_id (),
		   sections.str_offsets.get_id ());
  /* Can we use an existing virtual DWO file?  */
  dwo_file_slot = lookup_dwo_file_slot (per_objfile, virtual_dwo_name.c_str (),
					comp_dir);
  /* Create one if necessary.  */
  if (*dwo_file_slot == NULL)
    {
      dwarf_read_debug_printf ("Creating virtual DWO: %s",
			       virtual_dwo_name.c_str ());

      dwo_file = new struct dwo_file;
      dwo_file->dwo_name = std::move (virtual_dwo_name);
      dwo_file->comp_dir = comp_dir;
      dwo_file->sections.abbrev = sections.abbrev;
      dwo_file->sections.line = sections.line;
      dwo_file->sections.loc = sections.loc;
      dwo_file->sections.macinfo = sections.macinfo;
      dwo_file->sections.macro = sections.macro;
      dwo_file->sections.str_offsets = sections.str_offsets;
      /* The "str" section is global to the entire DWP file.  */
      dwo_file->sections.str = dwp_file->sections.str;
      /* The info or types section is assigned below to dwo_unit,
	 there's no need to record it in dwo_file.
	 Also, we can't simply record type sections in dwo_file because
	 we record a pointer into the vector in dwo_unit.  As we collect more
	 types we'll grow the vector and eventually have to reallocate space
	 for it, invalidating all copies of pointers into the previous
	 contents.  */
      *dwo_file_slot = dwo_file;
    }
  else
    {
      dwarf_read_debug_printf ("Using existing virtual DWO: %s",
			       virtual_dwo_name.c_str ());

      dwo_file = (struct dwo_file *) *dwo_file_slot;
    }

  dwo_unit = OBSTACK_ZALLOC (&per_objfile->per_bfd->obstack, struct dwo_unit);
  dwo_unit->dwo_file = dwo_file;
  dwo_unit->signature = signature;
  dwo_unit->section =
    XOBNEW (&per_objfile->per_bfd->obstack, struct dwarf2_section_info);
  *dwo_unit->section = sections.info_or_types;
  /* dwo_unit->{offset,length,type_offset_in_tu} are set later.  */

  return dwo_unit;
}

/* Subroutine of create_dwo_unit_in_dwp_v2 and create_dwo_unit_in_dwp_v5 to
   simplify them.  Given a pointer to the containing section SECTION, and
   OFFSET,SIZE of the piece within that section used by a TU/CU, return a
   virtual section of just that piece.  */

static struct dwarf2_section_info
create_dwp_v2_or_v5_section (dwarf2_per_objfile *per_objfile,
			     struct dwarf2_section_info *section,
			     bfd_size_type offset, bfd_size_type size)
{
  struct dwarf2_section_info result;
  asection *sectp;

  gdb_assert (section != NULL);
  gdb_assert (!section->is_virtual);

  memset (&result, 0, sizeof (result));
  result.s.containing_section = section;
  result.is_virtual = true;

  if (size == 0)
    return result;

  sectp = section->get_bfd_section ();

  /* Flag an error if the piece denoted by OFFSET,SIZE is outside the
     bounds of the real section.  This is a pretty-rare event, so just
     flag an error (easier) instead of a warning and trying to cope.  */
  if (sectp == NULL
      || offset + size > bfd_section_size (sectp))
    {
      error (_("Dwarf Error: Bad DWP V2 or V5 section info, doesn't fit"
	       " in section %s [in module %s]"),
	     sectp ? bfd_section_name (sectp) : "<unknown>",
	     objfile_name (per_objfile->objfile));
    }

  result.virtual_offset = offset;
  result.size = size;
  return result;
}

/* Create a dwo_unit object for the DWO unit with signature SIGNATURE.
   UNIT_INDEX is the index of the DWO unit in the DWP hash table.
   COMP_DIR is the DW_AT_comp_dir attribute of the referencing CU.
   This is for DWP version 2 files.  */

static struct dwo_unit *
create_dwo_unit_in_dwp_v2 (dwarf2_per_objfile *per_objfile,
			   struct dwp_file *dwp_file,
			   uint32_t unit_index,
			   const char *comp_dir,
			   ULONGEST signature, int is_debug_types)
{
  const struct dwp_hash_table *dwp_htab =
    is_debug_types ? dwp_file->tus : dwp_file->cus;
  bfd *dbfd = dwp_file->dbfd.get ();
  const char *kind = is_debug_types ? "TU" : "CU";
  struct dwo_file *dwo_file;
  struct dwo_unit *dwo_unit;
  struct virtual_v2_or_v5_dwo_sections sections;
  void **dwo_file_slot;
  int i;

  gdb_assert (dwp_file->version == 2);

  dwarf_read_debug_printf ("Reading %s %s/%s in DWP V2 file: %s",
			   kind, pulongest (unit_index), hex_string (signature),
			   dwp_file->name);

  /* Fetch the section offsets of this DWO unit.  */

  memset (&sections, 0, sizeof (sections));

  for (i = 0; i < dwp_htab->nr_columns; ++i)
    {
      uint32_t offset = read_4_bytes (dbfd,
				      dwp_htab->section_pool.v2.offsets
				      + (((unit_index - 1) * dwp_htab->nr_columns
					  + i)
					 * sizeof (uint32_t)));
      uint32_t size = read_4_bytes (dbfd,
				    dwp_htab->section_pool.v2.sizes
				    + (((unit_index - 1) * dwp_htab->nr_columns
					+ i)
				       * sizeof (uint32_t)));

      switch (dwp_htab->section_pool.v2.section_ids[i])
	{
	case DW_SECT_INFO:
	case DW_SECT_TYPES:
	  sections.info_or_types_offset = offset;
	  sections.info_or_types_size = size;
	  break;
	case DW_SECT_ABBREV:
	  sections.abbrev_offset = offset;
	  sections.abbrev_size = size;
	  break;
	case DW_SECT_LINE:
	  sections.line_offset = offset;
	  sections.line_size = size;
	  break;
	case DW_SECT_LOC:
	  sections.loc_offset = offset;
	  sections.loc_size = size;
	  break;
	case DW_SECT_STR_OFFSETS:
	  sections.str_offsets_offset = offset;
	  sections.str_offsets_size = size;
	  break;
	case DW_SECT_MACINFO:
	  sections.macinfo_offset = offset;
	  sections.macinfo_size = size;
	  break;
	case DW_SECT_MACRO:
	  sections.macro_offset = offset;
	  sections.macro_size = size;
	  break;
	}
    }

  /* It's easier for the rest of the code if we fake a struct dwo_file and
     have dwo_unit "live" in that.  At least for now.

     The DWP file can be made up of a random collection of CUs and TUs.
     However, for each CU + set of TUs that came from the same original DWO
     file, we can combine them back into a virtual DWO file to save space
     (fewer struct dwo_file objects to allocate).  Remember that for really
     large apps there can be on the order of 8K CUs and 200K TUs, or more.  */

  std::string virtual_dwo_name =
    string_printf ("virtual-dwo/%ld-%ld-%ld-%ld",
		   (long) (sections.abbrev_size ? sections.abbrev_offset : 0),
		   (long) (sections.line_size ? sections.line_offset : 0),
		   (long) (sections.loc_size ? sections.loc_offset : 0),
		   (long) (sections.str_offsets_size
			   ? sections.str_offsets_offset : 0));
  /* Can we use an existing virtual DWO file?  */
  dwo_file_slot = lookup_dwo_file_slot (per_objfile, virtual_dwo_name.c_str (),
					comp_dir);
  /* Create one if necessary.  */
  if (*dwo_file_slot == NULL)
    {
      dwarf_read_debug_printf ("Creating virtual DWO: %s",
			       virtual_dwo_name.c_str ());

      dwo_file = new struct dwo_file;
      dwo_file->dwo_name = std::move (virtual_dwo_name);
      dwo_file->comp_dir = comp_dir;
      dwo_file->sections.abbrev =
	create_dwp_v2_or_v5_section (per_objfile, &dwp_file->sections.abbrev,
				     sections.abbrev_offset,
				     sections.abbrev_size);
      dwo_file->sections.line =
	create_dwp_v2_or_v5_section (per_objfile, &dwp_file->sections.line,
				     sections.line_offset,
				     sections.line_size);
      dwo_file->sections.loc =
	create_dwp_v2_or_v5_section (per_objfile, &dwp_file->sections.loc,
				     sections.loc_offset, sections.loc_size);
      dwo_file->sections.macinfo =
	create_dwp_v2_or_v5_section (per_objfile, &dwp_file->sections.macinfo,
				     sections.macinfo_offset,
				     sections.macinfo_size);
      dwo_file->sections.macro =
	create_dwp_v2_or_v5_section (per_objfile, &dwp_file->sections.macro,
				     sections.macro_offset,
				     sections.macro_size);
      dwo_file->sections.str_offsets =
	create_dwp_v2_or_v5_section (per_objfile,
				     &dwp_file->sections.str_offsets,
				     sections.str_offsets_offset,
				     sections.str_offsets_size);
      /* The "str" section is global to the entire DWP file.  */
      dwo_file->sections.str = dwp_file->sections.str;
      /* The info or types section is assigned below to dwo_unit,
	 there's no need to record it in dwo_file.
	 Also, we can't simply record type sections in dwo_file because
	 we record a pointer into the vector in dwo_unit.  As we collect more
	 types we'll grow the vector and eventually have to reallocate space
	 for it, invalidating all copies of pointers into the previous
	 contents.  */
      *dwo_file_slot = dwo_file;
    }
  else
    {
      dwarf_read_debug_printf ("Using existing virtual DWO: %s",
			       virtual_dwo_name.c_str ());

      dwo_file = (struct dwo_file *) *dwo_file_slot;
    }

  dwo_unit = OBSTACK_ZALLOC (&per_objfile->per_bfd->obstack, struct dwo_unit);
  dwo_unit->dwo_file = dwo_file;
  dwo_unit->signature = signature;
  dwo_unit->section =
    XOBNEW (&per_objfile->per_bfd->obstack, struct dwarf2_section_info);
  *dwo_unit->section = create_dwp_v2_or_v5_section
			 (per_objfile,
			  is_debug_types
			  ? &dwp_file->sections.types
			  : &dwp_file->sections.info,
			  sections.info_or_types_offset,
			  sections.info_or_types_size);
  /* dwo_unit->{offset,length,type_offset_in_tu} are set later.  */

  return dwo_unit;
}

/* Create a dwo_unit object for the DWO unit with signature SIGNATURE.
   UNIT_INDEX is the index of the DWO unit in the DWP hash table.
   COMP_DIR is the DW_AT_comp_dir attribute of the referencing CU.
   This is for DWP version 5 files.  */

static struct dwo_unit *
create_dwo_unit_in_dwp_v5 (dwarf2_per_objfile *per_objfile,
			   struct dwp_file *dwp_file,
			   uint32_t unit_index,
			   const char *comp_dir,
			   ULONGEST signature, int is_debug_types)
{
  const struct dwp_hash_table *dwp_htab
    = is_debug_types ? dwp_file->tus : dwp_file->cus;
  bfd *dbfd = dwp_file->dbfd.get ();
  const char *kind = is_debug_types ? "TU" : "CU";
  struct dwo_file *dwo_file;
  struct dwo_unit *dwo_unit;
  struct virtual_v2_or_v5_dwo_sections sections {};
  void **dwo_file_slot;

  gdb_assert (dwp_file->version == 5);

  dwarf_read_debug_printf ("Reading %s %s/%s in DWP V5 file: %s",
			   kind, pulongest (unit_index), hex_string (signature),
			   dwp_file->name);

  /* Fetch the section offsets of this DWO unit.  */

  /*  memset (&sections, 0, sizeof (sections)); */

  for (int i = 0; i < dwp_htab->nr_columns; ++i)
    {
      uint32_t offset = read_4_bytes (dbfd,
				      dwp_htab->section_pool.v5.offsets
				      + (((unit_index - 1)
					  * dwp_htab->nr_columns
					  + i)
					 * sizeof (uint32_t)));
      uint32_t size = read_4_bytes (dbfd,
				    dwp_htab->section_pool.v5.sizes
				    + (((unit_index - 1) * dwp_htab->nr_columns
					+ i)
				       * sizeof (uint32_t)));

      switch (dwp_htab->section_pool.v5.section_ids[i])
	{
	  case DW_SECT_ABBREV_V5:
	    sections.abbrev_offset = offset;
	    sections.abbrev_size = size;
	    break;
	  case DW_SECT_INFO_V5:
	    sections.info_or_types_offset = offset;
	    sections.info_or_types_size = size;
	    break;
	  case DW_SECT_LINE_V5:
	    sections.line_offset = offset;
	    sections.line_size = size;
	    break;
	  case DW_SECT_LOCLISTS_V5:
	    sections.loclists_offset = offset;
	    sections.loclists_size = size;
	    break;
	  case DW_SECT_MACRO_V5:
	    sections.macro_offset = offset;
	    sections.macro_size = size;
	    break;
	  case DW_SECT_RNGLISTS_V5:
	    sections.rnglists_offset = offset;
	    sections.rnglists_size = size;
	    break;
	  case DW_SECT_STR_OFFSETS_V5:
	    sections.str_offsets_offset = offset;
	    sections.str_offsets_size = size;
	    break;
	  case DW_SECT_RESERVED_V5:
	  default:
	    break;
	}
    }

  /* It's easier for the rest of the code if we fake a struct dwo_file and
     have dwo_unit "live" in that.  At least for now.

     The DWP file can be made up of a random collection of CUs and TUs.
     However, for each CU + set of TUs that came from the same original DWO
     file, we can combine them back into a virtual DWO file to save space
     (fewer struct dwo_file objects to allocate).  Remember that for really
     large apps there can be on the order of 8K CUs and 200K TUs, or more.  */

  std::string virtual_dwo_name =
    string_printf ("virtual-dwo/%ld-%ld-%ld-%ld-%ld-%ld",
		 (long) (sections.abbrev_size ? sections.abbrev_offset : 0),
		 (long) (sections.line_size ? sections.line_offset : 0),
		 (long) (sections.loclists_size ? sections.loclists_offset : 0),
		 (long) (sections.str_offsets_size
			    ? sections.str_offsets_offset : 0),
		 (long) (sections.macro_size ? sections.macro_offset : 0),
		 (long) (sections.rnglists_size ? sections.rnglists_offset: 0));
  /* Can we use an existing virtual DWO file?  */
  dwo_file_slot = lookup_dwo_file_slot (per_objfile,
					virtual_dwo_name.c_str (),
					comp_dir);
  /* Create one if necessary.  */
  if (*dwo_file_slot == NULL)
    {
      dwarf_read_debug_printf ("Creating virtual DWO: %s",
			       virtual_dwo_name.c_str ());

      dwo_file = new struct dwo_file;
      dwo_file->dwo_name = std::move (virtual_dwo_name);
      dwo_file->comp_dir = comp_dir;
      dwo_file->sections.abbrev =
	create_dwp_v2_or_v5_section (per_objfile,
				     &dwp_file->sections.abbrev,
				     sections.abbrev_offset,
				     sections.abbrev_size);
      dwo_file->sections.line =
	create_dwp_v2_or_v5_section (per_objfile,
				     &dwp_file->sections.line,
				     sections.line_offset, sections.line_size);
      dwo_file->sections.macro =
	create_dwp_v2_or_v5_section (per_objfile,
				     &dwp_file->sections.macro,
				     sections.macro_offset,
				     sections.macro_size);
      dwo_file->sections.loclists =
	create_dwp_v2_or_v5_section (per_objfile,
				     &dwp_file->sections.loclists,
				     sections.loclists_offset,
				     sections.loclists_size);
      dwo_file->sections.rnglists =
	create_dwp_v2_or_v5_section (per_objfile,
				     &dwp_file->sections.rnglists,
				     sections.rnglists_offset,
				     sections.rnglists_size);
      dwo_file->sections.str_offsets =
	create_dwp_v2_or_v5_section (per_objfile,
				     &dwp_file->sections.str_offsets,
				     sections.str_offsets_offset,
				     sections.str_offsets_size);
      /* The "str" section is global to the entire DWP file.  */
      dwo_file->sections.str = dwp_file->sections.str;
      /* The info or types section is assigned below to dwo_unit,
	 there's no need to record it in dwo_file.
	 Also, we can't simply record type sections in dwo_file because
	 we record a pointer into the vector in dwo_unit.  As we collect more
	 types we'll grow the vector and eventually have to reallocate space
	 for it, invalidating all copies of pointers into the previous
	 contents.  */
      *dwo_file_slot = dwo_file;
    }
  else
    {
      dwarf_read_debug_printf ("Using existing virtual DWO: %s",
			       virtual_dwo_name.c_str ());

      dwo_file = (struct dwo_file *) *dwo_file_slot;
    }

  dwo_unit = OBSTACK_ZALLOC (&per_objfile->per_bfd->obstack, struct dwo_unit);
  dwo_unit->dwo_file = dwo_file;
  dwo_unit->signature = signature;
  dwo_unit->section
    = XOBNEW (&per_objfile->per_bfd->obstack, struct dwarf2_section_info);
  *dwo_unit->section = create_dwp_v2_or_v5_section (per_objfile,
					      &dwp_file->sections.info,
					      sections.info_or_types_offset,
					      sections.info_or_types_size);
  /* dwo_unit->{offset,length,type_offset_in_tu} are set later.  */

  return dwo_unit;
}

/* Lookup the DWO unit with SIGNATURE in DWP_FILE.
   Returns NULL if the signature isn't found.  */

static struct dwo_unit *
lookup_dwo_unit_in_dwp (dwarf2_per_objfile *per_objfile,
			struct dwp_file *dwp_file, const char *comp_dir,
			ULONGEST signature, int is_debug_types)
{
  const struct dwp_hash_table *dwp_htab =
    is_debug_types ? dwp_file->tus : dwp_file->cus;
  bfd *dbfd = dwp_file->dbfd.get ();
  uint32_t mask = dwp_htab->nr_slots - 1;
  uint32_t hash = signature & mask;
  uint32_t hash2 = ((signature >> 32) & mask) | 1;
  unsigned int i;
  void **slot;
  struct dwo_unit find_dwo_cu;

  memset (&find_dwo_cu, 0, sizeof (find_dwo_cu));
  find_dwo_cu.signature = signature;
  slot = htab_find_slot (is_debug_types
			 ? dwp_file->loaded_tus.get ()
			 : dwp_file->loaded_cus.get (),
			 &find_dwo_cu, INSERT);

  if (*slot != NULL)
    return (struct dwo_unit *) *slot;

  /* Use a for loop so that we don't loop forever on bad debug info.  */
  for (i = 0; i < dwp_htab->nr_slots; ++i)
    {
      ULONGEST signature_in_table;

      signature_in_table =
	read_8_bytes (dbfd, dwp_htab->hash_table + hash * sizeof (uint64_t));
      if (signature_in_table == signature)
	{
	  uint32_t unit_index =
	    read_4_bytes (dbfd,
			  dwp_htab->unit_table + hash * sizeof (uint32_t));

	  if (dwp_file->version == 1)
	    {
	      *slot = create_dwo_unit_in_dwp_v1 (per_objfile, dwp_file,
						 unit_index, comp_dir,
						 signature, is_debug_types);
	    }
	  else if (dwp_file->version == 2)
	    {
	      *slot = create_dwo_unit_in_dwp_v2 (per_objfile, dwp_file,
						 unit_index, comp_dir,
						 signature, is_debug_types);
	    }
	  else /* version == 5  */
	    {
	      *slot = create_dwo_unit_in_dwp_v5 (per_objfile, dwp_file,
						 unit_index, comp_dir,
						 signature, is_debug_types);
	    }
	  return (struct dwo_unit *) *slot;
	}
      if (signature_in_table == 0)
	return NULL;
      hash = (hash + hash2) & mask;
    }

  error (_("Dwarf Error: bad DWP hash table, lookup didn't terminate"
	   " [in module %s]"),
	 dwp_file->name);
}

/* Subroutine of open_dwo_file,open_dwp_file to simplify them.
   Open the file specified by FILE_NAME and hand it off to BFD for
   preliminary analysis.  Return a newly initialized bfd *, which
   includes a canonicalized copy of FILE_NAME.
   If IS_DWP is TRUE, we're opening a DWP file, otherwise a DWO file.
   SEARCH_CWD is true if the current directory is to be searched.
   It will be searched before debug-file-directory.
   If successful, the file is added to the bfd include table of the
   objfile's bfd (see gdb_bfd_record_inclusion).
   If unable to find/open the file, return NULL.
   NOTE: This function is derived from symfile_bfd_open.  */

static gdb_bfd_ref_ptr
try_open_dwop_file (dwarf2_per_objfile *per_objfile,
		    const char *file_name, int is_dwp, int search_cwd)
{
  int desc;
  /* Blech.  OPF_TRY_CWD_FIRST also disables searching the path list if
     FILE_NAME contains a '/'.  So we can't use it.  Instead prepend "."
     to debug_file_directory.  */
  const char *search_path;
  static const char dirname_separator_string[] = { DIRNAME_SEPARATOR, '\0' };

  gdb::unique_xmalloc_ptr<char> search_path_holder;
  if (search_cwd)
    {
      if (!debug_file_directory.empty ())
	{
	  search_path_holder.reset (concat (".", dirname_separator_string,
					    debug_file_directory.c_str (),
					    (char *) NULL));
	  search_path = search_path_holder.get ();
	}
      else
	search_path = ".";
    }
  else
    search_path = debug_file_directory.c_str ();

  /* Add the path for the executable binary to the list of search paths.  */
  std::string objfile_dir = ldirname (objfile_name (per_objfile->objfile));
  search_path_holder.reset (concat (objfile_dir.c_str (),
				    dirname_separator_string,
				    search_path, nullptr));
  search_path = search_path_holder.get ();

  openp_flags flags = OPF_RETURN_REALPATH;
  if (is_dwp)
    flags |= OPF_SEARCH_IN_PATH;

  gdb::unique_xmalloc_ptr<char> absolute_name;
  desc = openp (search_path, flags, file_name,
		O_RDONLY | O_BINARY, &absolute_name);
  if (desc < 0)
    return NULL;

  gdb_bfd_ref_ptr sym_bfd (gdb_bfd_open (absolute_name.get (),
					 gnutarget, desc));
  if (sym_bfd == NULL)
    return NULL;

  if (!bfd_check_format (sym_bfd.get (), bfd_object))
    return NULL;

  /* Success.  Record the bfd as having been included by the objfile's bfd.
     This is important because things like demangled_names_hash lives in the
     objfile's per_bfd space and may have references to things like symbol
     names that live in the DWO/DWP file's per_bfd space.  PR 16426.  */
  gdb_bfd_record_inclusion (per_objfile->objfile->obfd.get (), sym_bfd.get ());

  return sym_bfd;
}

/* Try to open DWO file FILE_NAME.
   COMP_DIR is the DW_AT_comp_dir attribute.
   The result is the bfd handle of the file.
   If there is a problem finding or opening the file, return NULL.
   Upon success, the canonicalized path of the file is stored in the bfd,
   same as symfile_bfd_open.  */

static gdb_bfd_ref_ptr
open_dwo_file (dwarf2_per_objfile *per_objfile,
	       const char *file_name, const char *comp_dir)
{
  if (IS_ABSOLUTE_PATH (file_name))
    return try_open_dwop_file (per_objfile, file_name,
			       0 /*is_dwp*/, 0 /*search_cwd*/);

  /* Before trying the search path, try DWO_NAME in COMP_DIR.  */

  if (comp_dir != NULL)
    {
      std::string path_to_try = path_join (comp_dir, file_name);

      /* NOTE: If comp_dir is a relative path, this will also try the
	 search path, which seems useful.  */
      gdb_bfd_ref_ptr abfd (try_open_dwop_file
	(per_objfile, path_to_try.c_str (), 0 /*is_dwp*/, 1 /*search_cwd*/));

      if (abfd != NULL)
	return abfd;
    }

  /* That didn't work, try debug-file-directory, which, despite its name,
     is a list of paths.  */

  if (debug_file_directory.empty ())
    return NULL;

  return try_open_dwop_file (per_objfile, file_name,
			     0 /*is_dwp*/, 1 /*search_cwd*/);
}

/* This function is mapped across the sections and remembers the offset and
   size of each of the DWO debugging sections we are interested in.  */

static void
dwarf2_locate_dwo_sections (struct objfile *objfile, bfd *abfd,
			    asection *sectp, dwo_sections *dwo_sections)
{
  const struct dwop_section_names *names = &dwop_section_names;

  struct dwarf2_section_info *dw_sect = nullptr;

  if (names->abbrev_dwo.matches (sectp->name))
    dw_sect = &dwo_sections->abbrev;
  else if (names->info_dwo.matches (sectp->name))
    dw_sect = &dwo_sections->info;
  else if (names->line_dwo.matches (sectp->name))
    dw_sect = &dwo_sections->line;
  else if (names->loc_dwo.matches (sectp->name))
    dw_sect = &dwo_sections->loc;
  else if (names->loclists_dwo.matches (sectp->name))
    dw_sect = &dwo_sections->loclists;
  else if (names->macinfo_dwo.matches (sectp->name))
    dw_sect = &dwo_sections->macinfo;
  else if (names->macro_dwo.matches (sectp->name))
    dw_sect = &dwo_sections->macro;
  else if (names->rnglists_dwo.matches (sectp->name))
    dw_sect = &dwo_sections->rnglists;
  else if (names->str_dwo.matches (sectp->name))
    dw_sect = &dwo_sections->str;
  else if (names->str_offsets_dwo.matches (sectp->name))
    dw_sect = &dwo_sections->str_offsets;
  else if (names->types_dwo.matches (sectp->name))
    {
      struct dwarf2_section_info type_section;

      memset (&type_section, 0, sizeof (type_section));
      dwo_sections->types.push_back (type_section);
      dw_sect = &dwo_sections->types.back ();
    }

  if (dw_sect != nullptr)
    {
      dw_sect->s.section = sectp;
      dw_sect->size = bfd_section_size (sectp);
      dw_sect->read (objfile);
    }
}

/* Initialize the use of the DWO file specified by DWO_NAME and referenced
   by PER_CU.  This is for the non-DWP case.
   The result is NULL if DWO_NAME can't be found.  */

static struct dwo_file *
open_and_init_dwo_file (dwarf2_cu *cu, const char *dwo_name,
			const char *comp_dir)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;

  gdb_bfd_ref_ptr dbfd = open_dwo_file (per_objfile, dwo_name, comp_dir);
  if (dbfd == NULL)
    {
      dwarf_read_debug_printf ("DWO file not found: %s", dwo_name);

      return NULL;
    }

  dwo_file_up dwo_file (new struct dwo_file);
  dwo_file->dwo_name = dwo_name;
  dwo_file->comp_dir = comp_dir;
  dwo_file->dbfd = std::move (dbfd);

  for (asection *sec : gdb_bfd_sections (dwo_file->dbfd))
    dwarf2_locate_dwo_sections (per_objfile->objfile, dwo_file->dbfd.get (),
				sec, &dwo_file->sections);

  create_cus_hash_table (per_objfile, cu, *dwo_file, dwo_file->sections.info,
			 dwo_file->cus);

  if (cu->per_cu->version () < 5)
    {
      create_debug_types_hash_table (per_objfile, dwo_file.get (),
				     dwo_file->sections.types, dwo_file->tus);
    }
  else
    {
      create_debug_type_hash_table (per_objfile, dwo_file.get (),
				    &dwo_file->sections.info, dwo_file->tus,
				    rcuh_kind::COMPILE);
    }

  dwarf_read_debug_printf ("DWO file found: %s", dwo_name);

  bfd_cache_close (dwo_file->dbfd.get ());

  return dwo_file.release ();
}

/* This function is mapped across the sections and remembers the offset and
   size of each of the DWP debugging sections common to version 1 and 2 that
   we are interested in.  */

static void
dwarf2_locate_common_dwp_sections (struct objfile *objfile, bfd *abfd,
				   asection *sectp, dwp_file *dwp_file)
{
  const struct dwop_section_names *names = &dwop_section_names;
  unsigned int elf_section_nr = elf_section_data (sectp)->this_idx;

  /* Record the ELF section number for later lookup: this is what the
     .debug_cu_index,.debug_tu_index tables use in DWP V1.  */
  gdb_assert (elf_section_nr < dwp_file->num_sections);
  dwp_file->elf_sections[elf_section_nr] = sectp;

  /* Look for specific sections that we need.  */
  struct dwarf2_section_info *dw_sect = nullptr;
  if (names->str_dwo.matches (sectp->name))
    dw_sect = &dwp_file->sections.str;
  else if (names->cu_index.matches (sectp->name))
    dw_sect = &dwp_file->sections.cu_index;
  else if (names->tu_index.matches (sectp->name))
    dw_sect = &dwp_file->sections.tu_index;

  if (dw_sect != nullptr)
    {
      dw_sect->s.section = sectp;
      dw_sect->size = bfd_section_size (sectp);
      dw_sect->read (objfile);
    }
}

/* This function is mapped across the sections and remembers the offset and
   size of each of the DWP version 2 debugging sections that we are interested
   in.  This is split into a separate function because we don't know if we
   have version 1 or 2 or 5 until we parse the cu_index/tu_index sections.  */

static void
dwarf2_locate_v2_dwp_sections (struct objfile *objfile, bfd *abfd,
			       asection *sectp, void *dwp_file_ptr)
{
  struct dwp_file *dwp_file = (struct dwp_file *) dwp_file_ptr;
  const struct dwop_section_names *names = &dwop_section_names;
  unsigned int elf_section_nr = elf_section_data (sectp)->this_idx;

  /* Record the ELF section number for later lookup: this is what the
     .debug_cu_index,.debug_tu_index tables use in DWP V1.  */
  gdb_assert (elf_section_nr < dwp_file->num_sections);
  dwp_file->elf_sections[elf_section_nr] = sectp;

  /* Look for specific sections that we need.  */
  struct dwarf2_section_info *dw_sect = nullptr;
  if (names->abbrev_dwo.matches (sectp->name))
    dw_sect = &dwp_file->sections.abbrev;
  else if (names->info_dwo.matches (sectp->name))
    dw_sect = &dwp_file->sections.info;
  else if (names->line_dwo.matches (sectp->name))
    dw_sect = &dwp_file->sections.line;
  else if (names->loc_dwo.matches (sectp->name))
    dw_sect = &dwp_file->sections.loc;
  else if (names->macinfo_dwo.matches (sectp->name))
    dw_sect = &dwp_file->sections.macinfo;
  else if (names->macro_dwo.matches (sectp->name))
    dw_sect = &dwp_file->sections.macro;
  else if (names->str_offsets_dwo.matches (sectp->name))
    dw_sect = &dwp_file->sections.str_offsets;
  else if (names->types_dwo.matches (sectp->name))
    dw_sect = &dwp_file->sections.types;

  if (dw_sect != nullptr)
    {
      dw_sect->s.section = sectp;
      dw_sect->size = bfd_section_size (sectp);
      dw_sect->read (objfile);
    }
}

/* This function is mapped across the sections and remembers the offset and
   size of each of the DWP version 5 debugging sections that we are interested
   in.  This is split into a separate function because we don't know if we
   have version 1 or 2 or 5 until we parse the cu_index/tu_index sections.  */

static void
dwarf2_locate_v5_dwp_sections (struct objfile *objfile, bfd *abfd,
			       asection *sectp, void *dwp_file_ptr)
{
  struct dwp_file *dwp_file = (struct dwp_file *) dwp_file_ptr;
  const struct dwop_section_names *names = &dwop_section_names;
  unsigned int elf_section_nr = elf_section_data (sectp)->this_idx;

  /* Record the ELF section number for later lookup: this is what the
     .debug_cu_index,.debug_tu_index tables use in DWP V1.  */
  gdb_assert (elf_section_nr < dwp_file->num_sections);
  dwp_file->elf_sections[elf_section_nr] = sectp;

  /* Look for specific sections that we need.  */
  struct dwarf2_section_info *dw_sect = nullptr;
  if (names->abbrev_dwo.matches (sectp->name))
    dw_sect = &dwp_file->sections.abbrev;
  else if (names->info_dwo.matches (sectp->name))
    dw_sect = &dwp_file->sections.info;
  else if (names->line_dwo.matches (sectp->name))
    dw_sect = &dwp_file->sections.line;
  else if (names->loclists_dwo.matches (sectp->name))
    dw_sect = &dwp_file->sections.loclists;
  else if (names->macro_dwo.matches (sectp->name))
    dw_sect = &dwp_file->sections.macro;
  else if (names->rnglists_dwo.matches (sectp->name))
    dw_sect = &dwp_file->sections.rnglists;
  else if (names->str_offsets_dwo.matches (sectp->name))
    dw_sect = &dwp_file->sections.str_offsets;

  if (dw_sect != nullptr)
    {
      dw_sect->s.section = sectp;
      dw_sect->size = bfd_section_size (sectp);
      dw_sect->read (objfile);
    }
}

/* Hash function for dwp_file loaded CUs/TUs.  */

static hashval_t
hash_dwp_loaded_cutus (const void *item)
{
  const struct dwo_unit *dwo_unit = (const struct dwo_unit *) item;

  /* This drops the top 32 bits of the signature, but is ok for a hash.  */
  return dwo_unit->signature;
}

/* Equality function for dwp_file loaded CUs/TUs.  */

static int
eq_dwp_loaded_cutus (const void *a, const void *b)
{
  const struct dwo_unit *dua = (const struct dwo_unit *) a;
  const struct dwo_unit *dub = (const struct dwo_unit *) b;

  return dua->signature == dub->signature;
}

/* Allocate a hash table for dwp_file loaded CUs/TUs.  */

static htab_up
allocate_dwp_loaded_cutus_table ()
{
  return htab_up (htab_create_alloc (3,
				     hash_dwp_loaded_cutus,
				     eq_dwp_loaded_cutus,
				     NULL, xcalloc, xfree));
}

/* Try to open DWP file FILE_NAME.
   The result is the bfd handle of the file.
   If there is a problem finding or opening the file, return NULL.
   Upon success, the canonicalized path of the file is stored in the bfd,
   same as symfile_bfd_open.  */

static gdb_bfd_ref_ptr
open_dwp_file (dwarf2_per_objfile *per_objfile, const char *file_name)
{
  gdb_bfd_ref_ptr abfd (try_open_dwop_file (per_objfile, file_name,
					    1 /*is_dwp*/,
					    1 /*search_cwd*/));
  if (abfd != NULL)
    return abfd;

  /* Work around upstream bug 15652.
     http://sourceware.org/bugzilla/show_bug.cgi?id=15652
     [Whether that's a "bug" is debatable, but it is getting in our way.]
     We have no real idea where the dwp file is, because gdb's realpath-ing
     of the executable's path may have discarded the needed info.
     [IWBN if the dwp file name was recorded in the executable, akin to
     .gnu_debuglink, but that doesn't exist yet.]
     Strip the directory from FILE_NAME and search again.  */
  if (!debug_file_directory.empty ())
    {
      /* Don't implicitly search the current directory here.
	 If the user wants to search "." to handle this case,
	 it must be added to debug-file-directory.  */
      return try_open_dwop_file (per_objfile, lbasename (file_name),
				 1 /*is_dwp*/,
				 0 /*search_cwd*/);
    }

  return NULL;
}

/* Initialize the use of the DWP file for the current objfile.
   By convention the name of the DWP file is ${objfile}.dwp.
   The result is NULL if it can't be found.  */

static std::unique_ptr<struct dwp_file>
open_and_init_dwp_file (dwarf2_per_objfile *per_objfile)
{
  struct objfile *objfile = per_objfile->objfile;

  /* Try to find first .dwp for the binary file before any symbolic links
     resolving.  */

  /* If the objfile is a debug file, find the name of the real binary
     file and get the name of dwp file from there.  */
  std::string dwp_name;
  if (objfile->separate_debug_objfile_backlink != NULL)
    {
      struct objfile *backlink = objfile->separate_debug_objfile_backlink;
      const char *backlink_basename = lbasename (backlink->original_name);

      dwp_name = ldirname (objfile->original_name) + SLASH_STRING + backlink_basename;
    }
  else
    dwp_name = objfile->original_name;

  dwp_name += ".dwp";

  gdb_bfd_ref_ptr dbfd (open_dwp_file (per_objfile, dwp_name.c_str ()));
  if (dbfd == NULL
      && strcmp (objfile->original_name, objfile_name (objfile)) != 0)
    {
      /* Try to find .dwp for the binary file after gdb_realpath resolving.  */
      dwp_name = objfile_name (objfile);
      dwp_name += ".dwp";
      dbfd = open_dwp_file (per_objfile, dwp_name.c_str ());
    }

  if (dbfd == NULL)
    {
      dwarf_read_debug_printf ("DWP file not found: %s", dwp_name.c_str ());

      return std::unique_ptr<dwp_file> ();
    }

  const char *name = bfd_get_filename (dbfd.get ());
  std::unique_ptr<struct dwp_file> dwp_file
    (new struct dwp_file (name, std::move (dbfd)));

  dwp_file->num_sections = elf_numsections (dwp_file->dbfd);
  dwp_file->elf_sections =
    OBSTACK_CALLOC (&per_objfile->per_bfd->obstack,
		    dwp_file->num_sections, asection *);

  for (asection *sec : gdb_bfd_sections (dwp_file->dbfd))
    dwarf2_locate_common_dwp_sections (objfile, dwp_file->dbfd.get (), sec,
				       dwp_file.get ());

  dwp_file->cus = create_dwp_hash_table (per_objfile, dwp_file.get (), 0);

  dwp_file->tus = create_dwp_hash_table (per_objfile, dwp_file.get (), 1);

  /* The DWP file version is stored in the hash table.  Oh well.  */
  if (dwp_file->cus && dwp_file->tus
      && dwp_file->cus->version != dwp_file->tus->version)
    {
      /* Technically speaking, we should try to limp along, but this is
	 pretty bizarre.  We use pulongest here because that's the established
	 portability solution (e.g, we cannot use %u for uint32_t).  */
      error (_("Dwarf Error: DWP file CU version %s doesn't match"
	       " TU version %s [in DWP file %s]"),
	     pulongest (dwp_file->cus->version),
	     pulongest (dwp_file->tus->version), dwp_name.c_str ());
    }

  if (dwp_file->cus)
    dwp_file->version = dwp_file->cus->version;
  else if (dwp_file->tus)
    dwp_file->version = dwp_file->tus->version;
  else
    dwp_file->version = 2;

  for (asection *sec : gdb_bfd_sections (dwp_file->dbfd))
    {
      if (dwp_file->version == 2)
	dwarf2_locate_v2_dwp_sections (objfile, dwp_file->dbfd.get (), sec,
				       dwp_file.get ());
      else
	dwarf2_locate_v5_dwp_sections (objfile, dwp_file->dbfd.get (), sec,
				       dwp_file.get ());
    }

  dwp_file->loaded_cus = allocate_dwp_loaded_cutus_table ();
  dwp_file->loaded_tus = allocate_dwp_loaded_cutus_table ();

  dwarf_read_debug_printf ("DWP file found: %s", dwp_file->name);
  dwarf_read_debug_printf ("    %s CUs, %s TUs",
			   pulongest (dwp_file->cus ? dwp_file->cus->nr_units : 0),
			   pulongest (dwp_file->tus ? dwp_file->tus->nr_units : 0));

  bfd_cache_close (dwp_file->dbfd.get ());

  return dwp_file;
}

/* Wrapper around open_and_init_dwp_file, only open it once.  */

static struct dwp_file *
get_dwp_file (dwarf2_per_objfile *per_objfile)
{
  if (!per_objfile->per_bfd->dwp_checked)
    {
      per_objfile->per_bfd->dwp_file = open_and_init_dwp_file (per_objfile);
      per_objfile->per_bfd->dwp_checked = 1;
    }
  return per_objfile->per_bfd->dwp_file.get ();
}

/* Subroutine of lookup_dwo_comp_unit, lookup_dwo_type_unit.
   Look up the CU/TU with signature SIGNATURE, either in DWO file DWO_NAME
   or in the DWP file for the objfile, referenced by THIS_UNIT.
   If non-NULL, comp_dir is the DW_AT_comp_dir attribute.
   IS_DEBUG_TYPES is non-zero if reading a TU, otherwise read a CU.

   This is called, for example, when wanting to read a variable with a
   complex location.  Therefore we don't want to do file i/o for every call.
   Therefore we don't want to look for a DWO file on every call.
   Therefore we first see if we've already seen SIGNATURE in a DWP file,
   then we check if we've already seen DWO_NAME, and only THEN do we check
   for a DWO file.

   The result is a pointer to the dwo_unit object or NULL if we didn't find it
   (dwo_id mismatch or couldn't find the DWO/DWP file).  */

static struct dwo_unit *
lookup_dwo_cutu (dwarf2_cu *cu, const char *dwo_name, const char *comp_dir,
		 ULONGEST signature, int is_debug_types)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  const char *kind = is_debug_types ? "TU" : "CU";
  void **dwo_file_slot;
  struct dwo_file *dwo_file;
  struct dwp_file *dwp_file;

  /* First see if there's a DWP file.
     If we have a DWP file but didn't find the DWO inside it, don't
     look for the original DWO file.  It makes gdb behave differently
     depending on whether one is debugging in the build tree.  */

  dwp_file = get_dwp_file (per_objfile);
  if (dwp_file != NULL)
    {
      const struct dwp_hash_table *dwp_htab =
	is_debug_types ? dwp_file->tus : dwp_file->cus;

      if (dwp_htab != NULL)
	{
	  struct dwo_unit *dwo_cutu =
	    lookup_dwo_unit_in_dwp (per_objfile, dwp_file, comp_dir, signature,
				    is_debug_types);

	  if (dwo_cutu != NULL)
	    {
	      dwarf_read_debug_printf ("Virtual DWO %s %s found: @%s",
				       kind, hex_string (signature),
				       host_address_to_string (dwo_cutu));

	      return dwo_cutu;
	    }
	}
    }
  else
    {
      /* No DWP file, look for the DWO file.  */

      dwo_file_slot = lookup_dwo_file_slot (per_objfile, dwo_name, comp_dir);
      if (*dwo_file_slot == NULL)
	{
	  /* Read in the file and build a table of the CUs/TUs it contains.  */
	  *dwo_file_slot = open_and_init_dwo_file (cu, dwo_name, comp_dir);
	}
      /* NOTE: This will be NULL if unable to open the file.  */
      dwo_file = (struct dwo_file *) *dwo_file_slot;

      if (dwo_file != NULL)
	{
	  struct dwo_unit *dwo_cutu = NULL;

	  if (is_debug_types && dwo_file->tus)
	    {
	      struct dwo_unit find_dwo_cutu;

	      memset (&find_dwo_cutu, 0, sizeof (find_dwo_cutu));
	      find_dwo_cutu.signature = signature;
	      dwo_cutu
		= (struct dwo_unit *) htab_find (dwo_file->tus.get (),
						 &find_dwo_cutu);
	    }
	  else if (!is_debug_types && dwo_file->cus)
	    {
	      struct dwo_unit find_dwo_cutu;

	      memset (&find_dwo_cutu, 0, sizeof (find_dwo_cutu));
	      find_dwo_cutu.signature = signature;
	      dwo_cutu = (struct dwo_unit *)htab_find (dwo_file->cus.get (),
						       &find_dwo_cutu);
	    }

	  if (dwo_cutu != NULL)
	    {
	      dwarf_read_debug_printf ("DWO %s %s(%s) found: @%s",
				       kind, dwo_name, hex_string (signature),
				       host_address_to_string (dwo_cutu));

	      return dwo_cutu;
	    }
	}
    }

  /* We didn't find it.  This could mean a dwo_id mismatch, or
     someone deleted the DWO/DWP file, or the search path isn't set up
     correctly to find the file.  */

  dwarf_read_debug_printf ("DWO %s %s(%s) not found",
			   kind, dwo_name, hex_string (signature));

  /* This is a warning and not a complaint because it can be caused by
     pilot error (e.g., user accidentally deleting the DWO).  */
  {
    /* Print the name of the DWP file if we looked there, helps the user
       better diagnose the problem.  */
    std::string dwp_text;

    if (dwp_file != NULL)
      dwp_text = string_printf (" [in DWP file %s]",
				lbasename (dwp_file->name));

    warning (_("Could not find DWO %s %s(%s)%s referenced by %s at offset %s"
	       " [in module %s]"),
	     kind, dwo_name, hex_string (signature), dwp_text.c_str (), kind,
	     sect_offset_str (cu->per_cu->sect_off), objfile_name (objfile));
  }
  return NULL;
}

/* Lookup the DWO CU DWO_NAME/SIGNATURE referenced from THIS_CU.
   See lookup_dwo_cutu_unit for details.  */

static struct dwo_unit *
lookup_dwo_comp_unit (dwarf2_cu *cu, const char *dwo_name, const char *comp_dir,
		      ULONGEST signature)
{
  gdb_assert (!cu->per_cu->is_debug_types);

  return lookup_dwo_cutu (cu, dwo_name, comp_dir, signature, 0);
}

/* Lookup the DWO TU DWO_NAME/SIGNATURE referenced from THIS_TU.
   See lookup_dwo_cutu_unit for details.  */

static struct dwo_unit *
lookup_dwo_type_unit (dwarf2_cu *cu, const char *dwo_name, const char *comp_dir)
{
  gdb_assert (cu->per_cu->is_debug_types);

  signatured_type *sig_type = (signatured_type *) cu->per_cu;

  return lookup_dwo_cutu (cu, dwo_name, comp_dir, sig_type->signature, 1);
}

/* Traversal function for queue_and_load_all_dwo_tus.  */

static int
queue_and_load_dwo_tu (void **slot, void *info)
{
  struct dwo_unit *dwo_unit = (struct dwo_unit *) *slot;
  dwarf2_cu *cu = (dwarf2_cu *) info;
  ULONGEST signature = dwo_unit->signature;
  signatured_type *sig_type = lookup_dwo_signatured_type (cu, signature);

  if (sig_type != NULL)
    {
      /* We pass NULL for DEPENDENT_CU because we don't yet know if there's
	 a real dependency of PER_CU on SIG_TYPE.  That is detected later
	 while processing PER_CU.  */
      if (maybe_queue_comp_unit (NULL, sig_type, cu->per_objfile,
				 cu->lang ()))
	load_full_type_unit (sig_type, cu->per_objfile);
      cu->per_cu->imported_symtabs_push (sig_type);
    }

  return 1;
}

/* Queue all TUs contained in the DWO of CU to be read in.
   The DWO may have the only definition of the type, though it may not be
   referenced anywhere in PER_CU.  Thus we have to load *all* its TUs.
   http://sourceware.org/bugzilla/show_bug.cgi?id=15021  */

static void
queue_and_load_all_dwo_tus (dwarf2_cu *cu)
{
  struct dwo_unit *dwo_unit;
  struct dwo_file *dwo_file;

  gdb_assert (cu != nullptr);
  gdb_assert (!cu->per_cu->is_debug_types);
  gdb_assert (get_dwp_file (cu->per_objfile) == nullptr);

  dwo_unit = cu->dwo_unit;
  gdb_assert (dwo_unit != NULL);

  dwo_file = dwo_unit->dwo_file;
  if (dwo_file->tus != NULL)
    htab_traverse_noresize (dwo_file->tus.get (), queue_and_load_dwo_tu, cu);
}

/* Read in various DIEs.  */

/* DW_AT_abstract_origin inherits whole DIEs (not just their attributes).
   Inherit only the children of the DW_AT_abstract_origin DIE not being
   already referenced by DW_AT_abstract_origin from the children of the
   current DIE.  */

static void
inherit_abstract_dies (struct die_info *die, struct dwarf2_cu *cu)
{
  attribute *attr = dwarf2_attr (die, DW_AT_abstract_origin, cu);
  if (attr == nullptr)
    return;

  /* Note that following die references may follow to a die in a
     different CU.  */
  dwarf2_cu *origin_cu = cu;

  /* Parent of DIE - referenced by DW_AT_abstract_origin.  */
  die_info *origin_die = follow_die_ref (die, attr, &origin_cu);

  /* We're inheriting ORIGIN's children into the scope we'd put DIE's
     symbols in.  */
  struct pending **origin_previous_list_in_scope = origin_cu->list_in_scope;
  origin_cu->list_in_scope = cu->list_in_scope;

  if (die->tag != origin_die->tag
      && !(die->tag == DW_TAG_inlined_subroutine
	   && origin_die->tag == DW_TAG_subprogram))
    complaint (_("DIE %s and its abstract origin %s have different tags"),
	       sect_offset_str (die->sect_off),
	       sect_offset_str (origin_die->sect_off));

  /* Find if the concrete and abstract trees are structurally the
     same.  This is a shallow traversal and it is not bullet-proof;
     the compiler can trick the debugger into believing that the trees
     are isomorphic, whereas they actually are not.  However, the
     likelihood of this happening is pretty low, and a full-fledged
     check would be an overkill.  */
  bool are_isomorphic = true;
  die_info *concrete_child = die->child;
  die_info *abstract_child = origin_die->child;
  while (concrete_child != nullptr || abstract_child != nullptr)
    {
      if (concrete_child == nullptr
	  || abstract_child == nullptr
	  || concrete_child->tag != abstract_child->tag)
	{
	  are_isomorphic = false;
	  break;
	}

      concrete_child = concrete_child->sibling;
      abstract_child = abstract_child->sibling;
    }

  /* Walk the origin's children in parallel to the concrete children.
     This helps match an origin child in case the debug info misses
     DW_AT_abstract_origin attributes.  Keep in mind that the abstract
     origin tree may not have the same tree structure as the concrete
     DIE, though.  */
  die_info *corresponding_abstract_child
    = are_isomorphic ? origin_die->child : nullptr;

  std::vector<sect_offset> offsets;

  for (die_info *child_die = die->child;
       child_die && child_die->tag;
       child_die = child_die->sibling)
    {
      /* We are trying to process concrete instance entries:
	 DW_TAG_call_site DIEs indeed have a DW_AT_abstract_origin tag, but
	 it's not relevant to our analysis here. i.e. detecting DIEs that are
	 present in the abstract instance but not referenced in the concrete
	 one.  */
      if (child_die->tag == DW_TAG_call_site
	  || child_die->tag == DW_TAG_GNU_call_site)
	{
	  if (are_isomorphic)
	    corresponding_abstract_child
	      = corresponding_abstract_child->sibling;
	  continue;
	}

      /* For each CHILD_DIE, find the corresponding child of
	 ORIGIN_DIE.  If there is more than one layer of
	 DW_AT_abstract_origin, follow them all; there shouldn't be,
	 but GCC versions at least through 4.4 generate this (GCC PR
	 40573).  */
      die_info *child_origin_die = child_die;
      dwarf2_cu *child_origin_cu = cu;
      while (true)
	{
	  attr = dwarf2_attr (child_origin_die, DW_AT_abstract_origin,
			      child_origin_cu);
	  if (attr == nullptr)
	    break;

	  die_info *prev_child_origin_die = child_origin_die;
	  child_origin_die = follow_die_ref (child_origin_die, attr,
					     &child_origin_cu);

	  if (prev_child_origin_die == child_origin_die)
	    {
	      /* Handle DIE with self-reference.  */
	      break;
	    }
	}

      /* If missing DW_AT_abstract_origin, try the corresponding child
	 of the origin.  Clang emits such lexical scopes.  */
      if (child_origin_die == child_die
	  && dwarf2_attr (child_die, DW_AT_abstract_origin, cu) == nullptr
	  && are_isomorphic
	  && child_die->tag == DW_TAG_lexical_block)
	child_origin_die = corresponding_abstract_child;

      /* According to DWARF3 3.3.8.2 #3 new entries without their abstract
	 counterpart may exist.  */
      if (child_origin_die != child_die)
	{
	  if (child_die->tag != child_origin_die->tag
	      && !(child_die->tag == DW_TAG_inlined_subroutine
		   && child_origin_die->tag == DW_TAG_subprogram))
	    complaint (_("Child DIE %s and its abstract origin %s have "
			 "different tags"),
		       sect_offset_str (child_die->sect_off),
		       sect_offset_str (child_origin_die->sect_off));
	  if (child_origin_die->parent != origin_die)
	    complaint (_("Child DIE %s and its abstract origin %s have "
			 "different parents"),
		       sect_offset_str (child_die->sect_off),
		       sect_offset_str (child_origin_die->sect_off));
	  else
	    offsets.push_back (child_origin_die->sect_off);
	}

      if (are_isomorphic)
	corresponding_abstract_child = corresponding_abstract_child->sibling;
    }

  if (!offsets.empty ())
    {
      std::sort (offsets.begin (), offsets.end ());

      for (auto offsets_it = offsets.begin () + 1;
	   offsets_it < offsets.end ();
	   ++offsets_it)
	if (*(offsets_it - 1) == *offsets_it)
	  complaint (_("Multiple children of DIE %s refer "
		       "to DIE %s as their abstract origin"),
		     sect_offset_str (die->sect_off),
		     sect_offset_str (*offsets_it));
    }

  auto offsets_it = offsets.begin ();
  die_info *origin_child_die = origin_die->child;
  while (origin_child_die != nullptr && origin_child_die->tag != 0)
    {
      /* Is ORIGIN_CHILD_DIE referenced by any of the DIE children?  */
      while (offsets_it < offsets.end ()
	     && *offsets_it < origin_child_die->sect_off)
	++offsets_it;

      if (offsets_it == offsets.end ()
	  || *offsets_it > origin_child_die->sect_off)
	{
	  /* Found that ORIGIN_CHILD_DIE is really not referenced.
	     Check whether we're already processing ORIGIN_CHILD_DIE.
	     This can happen with mutually referenced abstract_origins.
	     PR 16581.  */
	  if (!origin_child_die->in_process)
	    process_die (origin_child_die, origin_cu);
	}

      origin_child_die = origin_child_die->sibling;
    }

  origin_cu->list_in_scope = origin_previous_list_in_scope;

  if (cu != origin_cu)
    compute_delayed_physnames (origin_cu);
}

/* Return TRUE if the given DIE is the program's "main".  DWARF 4 has
   defined a dedicated DW_AT_main_subprogram attribute to indicate the
   starting function of the program, however with older versions the
   DW_CC_program value of the DW_AT_calling_convention attribute was
   used instead as the only means available.  We handle both variants.  */

static bool
dwarf2_func_is_main_p (struct die_info *die, struct dwarf2_cu *cu)
{
  if (dwarf2_flag_true_p (die, DW_AT_main_subprogram, cu))
    return true;
  struct attribute *attr = dwarf2_attr (die, DW_AT_calling_convention, cu);
  return (attr != nullptr
	  && attr->constant_value (DW_CC_normal) == DW_CC_program);
}

/* A helper to handle Ada's "Pragma Import" feature when it is applied
   to a function.  */

static bool
check_ada_pragma_import (struct die_info *die, struct dwarf2_cu *cu)
{
  /* A Pragma Import will have both a name and a linkage name.  */
  const char *name = dwarf2_name (die, cu);
  if (name == nullptr)
    return false;

  const char *linkage_name = dw2_linkage_name (die, cu);
  /* Disallow the special Ada symbols.  */
  if (!is_ada_import_or_export (cu, name, linkage_name))
    return false;

  /* A Pragma Import will be a declaration, while a Pragma Export will
     not be.  */
  if (!die_is_declaration (die, cu))
    return false;

  new_symbol (die, read_type_die (die, cu), cu);
  return true;
}

static void
read_func_scope (struct die_info *die, struct dwarf2_cu *cu)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  struct gdbarch *gdbarch = objfile->arch ();
  struct context_stack *newobj;
  CORE_ADDR lowpc;
  CORE_ADDR highpc;
  struct die_info *child_die;
  struct attribute *attr, *call_line, *call_file;
  const char *name;
  struct block *block;
  int inlined_func = (die->tag == DW_TAG_inlined_subroutine);
  std::vector<struct symbol *> template_args;
  struct template_symbol *templ_func = NULL;

  if (inlined_func)
    {
      /* If we do not have call site information, we can't show the
	 caller of this inlined function.  That's too confusing, so
	 only use the scope for local variables.  */
      call_line = dwarf2_attr (die, DW_AT_call_line, cu);
      call_file = dwarf2_attr (die, DW_AT_call_file, cu);
      if (call_line == NULL || call_file == NULL)
	{
	  read_lexical_block_scope (die, cu);
	  return;
	}
    }

  name = dwarf2_name (die, cu);
  if (name == nullptr)
    name = dw2_linkage_name (die, cu);

  /* Ignore functions with missing or empty names.  These are actually
     illegal according to the DWARF standard.  */
  if (name == NULL)
    {
      complaint (_("missing name for subprogram DIE at %s"),
		 sect_offset_str (die->sect_off));
      return;
    }

  if (check_ada_pragma_import (die, cu))
    {
      /* We already made the symbol for the Pragma Import, and because
	 it is a declaration, we know it won't have any other
	 important information, so we can simply return.  */
      return;
    }

  /* Ignore functions with missing or invalid low and high pc attributes.  */
  unrelocated_addr unrel_low, unrel_high;
  if (dwarf2_get_pc_bounds (die, &unrel_low, &unrel_high, cu, nullptr, nullptr)
      <= PC_BOUNDS_INVALID)
    {
      if (have_complaint ())
	{
	  attr = dwarf2_attr (die, DW_AT_external, cu);
	  bool external_p = attr != nullptr && attr->as_boolean ();
	  attr = dwarf2_attr (die, DW_AT_inline, cu);
	  bool inlined_p
	    = (attr != nullptr
	       && attr->is_nonnegative ()
	       && (attr->as_nonnegative () == DW_INL_inlined
		   || attr->as_nonnegative () == DW_INL_declared_inlined));
	  attr = dwarf2_attr (die, DW_AT_declaration, cu);
	  bool decl_p = attr != nullptr && attr->as_boolean ();
	  if (!external_p && !inlined_p && !decl_p)
	    complaint (_("cannot get low and high bounds "
			 "for subprogram DIE at %s"),
		       sect_offset_str (die->sect_off));
	}
      return;
    }

  lowpc = per_objfile->relocate (unrel_low);
  highpc = per_objfile->relocate (unrel_high);

  /* If we have any template arguments, then we must allocate a
     different sort of symbol.  */
  for (child_die = die->child; child_die; child_die = child_die->sibling)
    {
      if (child_die->tag == DW_TAG_template_type_param
	  || child_die->tag == DW_TAG_template_value_param)
	{
	  templ_func = new (&objfile->objfile_obstack) template_symbol;
	  templ_func->subclass = SYMBOL_TEMPLATE;
	  break;
	}
    }

  gdb_assert (cu->get_builder () != nullptr);
  newobj = cu->get_builder ()->push_context (0, lowpc);
  newobj->name = new_symbol (die, read_type_die (die, cu), cu, templ_func);

  if (dwarf2_func_is_main_p (die, cu))
    set_objfile_main_name (objfile, newobj->name->linkage_name (),
			   cu->lang ());

  /* If there is a location expression for DW_AT_frame_base, record
     it.  */
  attr = dwarf2_attr (die, DW_AT_frame_base, cu);
  if (attr != nullptr)
    dwarf2_symbol_mark_computed (attr, newobj->name, cu, 1);

  /* If there is a location for the static link, record it.  */
  newobj->static_link = NULL;
  attr = dwarf2_attr (die, DW_AT_static_link, cu);
  if (attr != nullptr)
    {
      newobj->static_link
	= XOBNEW (&objfile->objfile_obstack, struct dynamic_prop);
      attr_to_dynamic_prop (attr, die, cu, newobj->static_link,
			    cu->addr_type ());
    }

  cu->list_in_scope = cu->get_builder ()->get_local_symbols ();

  if (die->child != NULL)
    {
      child_die = die->child;
      while (child_die && child_die->tag)
	{
	  if (child_die->tag == DW_TAG_template_type_param
	      || child_die->tag == DW_TAG_template_value_param)
	    {
	      struct symbol *arg = new_symbol (child_die, NULL, cu);

	      if (arg != NULL)
		template_args.push_back (arg);
	    }
	  else
	    process_die (child_die, cu);
	  child_die = child_die->sibling;
	}
    }

  inherit_abstract_dies (die, cu);

  /* If we have a DW_AT_specification, we might need to import using
     directives from the context of the specification DIE.  See the
     comment in determine_prefix.  */
  if (cu->lang () == language_cplus
      && dwarf2_attr (die, DW_AT_specification, cu))
    {
      struct dwarf2_cu *spec_cu = cu;
      struct die_info *spec_die = die_specification (die, &spec_cu);

      while (spec_die)
	{
	  child_die = spec_die->child;
	  while (child_die && child_die->tag)
	    {
	      if (child_die->tag == DW_TAG_imported_module)
		process_die (child_die, spec_cu);
	      child_die = child_die->sibling;
	    }

	  /* In some cases, GCC generates specification DIEs that
	     themselves contain DW_AT_specification attributes.  */
	  spec_die = die_specification (spec_die, &spec_cu);
	}
    }

  struct context_stack cstk = cu->get_builder ()->pop_context ();
  /* Make a block for the local symbols within.  */
  block = cu->get_builder ()->finish_block (cstk.name, cstk.old_blocks,
				     cstk.static_link, lowpc, highpc);

  /* For C++, set the block's scope.  */
  if ((cu->lang () == language_cplus
       || cu->lang () == language_fortran
       || cu->lang () == language_d
       || cu->lang () == language_rust)
      && cu->processing_has_namespace_info)
    block->set_scope (determine_prefix (die, cu),
		      &objfile->objfile_obstack);

  /* If we have address ranges, record them.  */
  dwarf2_record_block_ranges (die, block, cu);

  gdbarch_make_symbol_special (gdbarch, cstk.name, objfile);

  /* Attach template arguments to function.  */
  if (!template_args.empty ())
    {
      gdb_assert (templ_func != NULL);

      templ_func->n_template_arguments = template_args.size ();
      templ_func->template_arguments
	= XOBNEWVEC (&objfile->objfile_obstack, struct symbol *,
		     templ_func->n_template_arguments);
      memcpy (templ_func->template_arguments,
	      template_args.data (),
	      (templ_func->n_template_arguments * sizeof (struct symbol *)));

      /* Make sure that the symtab is set on the new symbols.  Even
	 though they don't appear in this symtab directly, other parts
	 of gdb assume that symbols do, and this is reasonably
	 true.  */
      for (symbol *sym : template_args)
	sym->set_symtab (templ_func->symtab ());
    }

  /* In C++, we can have functions nested inside functions (e.g., when
     a function declares a class that has methods).  This means that
     when we finish processing a function scope, we may need to go
     back to building a containing block's symbol lists.  */
  *cu->get_builder ()->get_local_symbols () = cstk.locals;
  cu->get_builder ()->set_local_using_directives (cstk.local_using_directives);

  /* If we've finished processing a top-level function, subsequent
     symbols go in the file symbol list.  */
  if (cu->get_builder ()->outermost_context_p ())
    cu->list_in_scope = cu->get_builder ()->get_file_symbols ();
}

/* Process all the DIES contained within a lexical block scope.  Start
   a new scope, process the dies, and then close the scope.  */

static void
read_lexical_block_scope (struct die_info *die, struct dwarf2_cu *cu)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  CORE_ADDR lowpc, highpc;
  struct die_info *child_die;

  /* Ignore blocks with missing or invalid low and high pc attributes.  */
  /* ??? Perhaps consider discontiguous blocks defined by DW_AT_ranges
     as multiple lexical blocks?  Handling children in a sane way would
     be nasty.  Might be easier to properly extend generic blocks to
     describe ranges.  */
  unrelocated_addr unrel_low, unrel_high;
  switch (dwarf2_get_pc_bounds (die, &unrel_low, &unrel_high, cu,
				nullptr, nullptr))
    {
    case PC_BOUNDS_NOT_PRESENT:
      /* DW_TAG_lexical_block has no attributes, process its children as if
	 there was no wrapping by that DW_TAG_lexical_block.
	 GCC does no longer produces such DWARF since GCC r224161.  */
      for (child_die = die->child;
	   child_die != NULL && child_die->tag;
	   child_die = child_die->sibling)
	{
	  /* We might already be processing this DIE.  This can happen
	     in an unusual circumstance -- where a subroutine A
	     appears lexically in another subroutine B, but A actually
	     inlines B.  The recursion is broken here, rather than in
	     inherit_abstract_dies, because it seems better to simply
	     drop concrete children here.  */
	  if (!child_die->in_process)
	    process_die (child_die, cu);
	}
      return;
    case PC_BOUNDS_INVALID:
      return;
    }
  lowpc = per_objfile->relocate (unrel_low);
  highpc = per_objfile->relocate (unrel_high);

  cu->get_builder ()->push_context (0, lowpc);
  if (die->child != NULL)
    {
      child_die = die->child;
      while (child_die && child_die->tag)
	{
	  process_die (child_die, cu);
	  child_die = child_die->sibling;
	}
    }
  inherit_abstract_dies (die, cu);
  struct context_stack cstk = cu->get_builder ()->pop_context ();

  if (*cu->get_builder ()->get_local_symbols () != NULL
      || (*cu->get_builder ()->get_local_using_directives ()) != NULL)
    {
      struct block *block
	= cu->get_builder ()->finish_block (0, cstk.old_blocks, NULL,
				     cstk.start_addr, highpc);

      /* Note that recording ranges after traversing children, as we
	 do here, means that recording a parent's ranges entails
	 walking across all its children's ranges as they appear in
	 the address map, which is quadratic behavior.

	 It would be nicer to record the parent's ranges before
	 traversing its children, simply overriding whatever you find
	 there.  But since we don't even decide whether to create a
	 block until after we've traversed its children, that's hard
	 to do.  */
      dwarf2_record_block_ranges (die, block, cu);
    }
  *cu->get_builder ()->get_local_symbols () = cstk.locals;
  cu->get_builder ()->set_local_using_directives (cstk.local_using_directives);
}

static void dwarf2_ranges_read_low_addrs
     (unsigned offset,
      struct dwarf2_cu *cu,
      dwarf_tag tag,
      std::vector<unrelocated_addr> &result);

/* Read in DW_TAG_call_site and insert it to CU->call_site_htab.  */

static void
read_call_site_scope (struct die_info *die, struct dwarf2_cu *cu)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  struct gdbarch *gdbarch = objfile->arch ();
  struct attribute *attr;
  void **slot;
  int nparams;
  struct die_info *child_die;

  attr = dwarf2_attr (die, DW_AT_call_return_pc, cu);
  if (attr == NULL)
    {
      /* This was a pre-DWARF-5 GNU extension alias
	 for DW_AT_call_return_pc.  */
      attr = dwarf2_attr (die, DW_AT_low_pc, cu);
    }
  if (!attr)
    {
      complaint (_("missing DW_AT_call_return_pc for DW_TAG_call_site "
		   "DIE %s [in module %s]"),
		 sect_offset_str (die->sect_off), objfile_name (objfile));
      return;
    }
  unrelocated_addr pc = per_objfile->adjust (attr->as_address ());

  if (cu->call_site_htab == NULL)
    cu->call_site_htab = htab_create_alloc_ex (16, call_site::hash,
					       call_site::eq, NULL,
					       &objfile->objfile_obstack,
					       hashtab_obstack_allocate, NULL);
  struct call_site call_site_local (pc, nullptr, nullptr);
  slot = htab_find_slot (cu->call_site_htab, &call_site_local, INSERT);
  if (*slot != NULL)
    {
      complaint (_("Duplicate PC %s for DW_TAG_call_site "
		   "DIE %s [in module %s]"),
		 paddress (gdbarch, (CORE_ADDR) pc), sect_offset_str (die->sect_off),
		 objfile_name (objfile));
      return;
    }

  /* Count parameters at the caller.  */

  nparams = 0;
  for (child_die = die->child; child_die && child_die->tag;
       child_die = child_die->sibling)
    {
      if (child_die->tag != DW_TAG_call_site_parameter
	  && child_die->tag != DW_TAG_GNU_call_site_parameter)
	{
	  complaint (_("Tag %d is not DW_TAG_call_site_parameter in "
		       "DW_TAG_call_site child DIE %s [in module %s]"),
		     child_die->tag, sect_offset_str (child_die->sect_off),
		     objfile_name (objfile));
	  continue;
	}

      nparams++;
    }

  struct call_site *call_site
    = new (XOBNEWVAR (&objfile->objfile_obstack,
		      struct call_site,
		      sizeof (*call_site) + sizeof (call_site->parameter[0]) * nparams))
    struct call_site (pc, cu->per_cu, per_objfile);
  *slot = call_site;

  /* We never call the destructor of call_site, so we must ensure it is
     trivially destructible.  */
  static_assert(std::is_trivially_destructible<struct call_site>::value);

  if (dwarf2_flag_true_p (die, DW_AT_call_tail_call, cu)
      || dwarf2_flag_true_p (die, DW_AT_GNU_tail_call, cu))
    {
      struct die_info *func_die;

      /* Skip also over DW_TAG_inlined_subroutine.  */
      for (func_die = die->parent;
	   func_die && func_die->tag != DW_TAG_subprogram
	   && func_die->tag != DW_TAG_subroutine_type;
	   func_die = func_die->parent);

      /* DW_AT_call_all_calls is a superset
	 of DW_AT_call_all_tail_calls.  */
      if (func_die
	  && !dwarf2_flag_true_p (func_die, DW_AT_call_all_calls, cu)
	  && !dwarf2_flag_true_p (func_die, DW_AT_GNU_all_call_sites, cu)
	  && !dwarf2_flag_true_p (func_die, DW_AT_call_all_tail_calls, cu)
	  && !dwarf2_flag_true_p (func_die, DW_AT_GNU_all_tail_call_sites, cu))
	{
	  /* TYPE_TAIL_CALL_LIST is not interesting in functions where it is
	     not complete.  But keep CALL_SITE for look ups via call_site_htab,
	     both the initial caller containing the real return address PC and
	     the final callee containing the current PC of a chain of tail
	     calls do not need to have the tail call list complete.  But any
	     function candidate for a virtual tail call frame searched via
	     TYPE_TAIL_CALL_LIST must have the tail call list complete to be
	     determined unambiguously.  */
	}
      else
	{
	  struct type *func_type = NULL;

	  if (func_die)
	    func_type = get_die_type (func_die, cu);
	  if (func_type != NULL)
	    {
	      gdb_assert (func_type->code () == TYPE_CODE_FUNC);

	      /* Enlist this call site to the function.  */
	      call_site->tail_call_next = TYPE_TAIL_CALL_LIST (func_type);
	      TYPE_TAIL_CALL_LIST (func_type) = call_site;
	    }
	  else
	    complaint (_("Cannot find function owning DW_TAG_call_site "
			 "DIE %s [in module %s]"),
		       sect_offset_str (die->sect_off), objfile_name (objfile));
	}
    }

  attr = dwarf2_attr (die, DW_AT_call_target, cu);
  if (attr == NULL)
    attr = dwarf2_attr (die, DW_AT_GNU_call_site_target, cu);
  if (attr == NULL)
    attr = dwarf2_attr (die, DW_AT_call_origin, cu);
  if (attr == NULL)
    {
      /* This was a pre-DWARF-5 GNU extension alias for DW_AT_call_origin.  */
      attr = dwarf2_attr (die, DW_AT_abstract_origin, cu);
    }

  call_site->target.set_loc_dwarf_block (nullptr);
  if (!attr || (attr->form_is_block () && attr->as_block ()->size == 0))
    /* Keep NULL DWARF_BLOCK.  */;
  else if (attr->form_is_block ())
    {
      struct dwarf2_locexpr_baton *dlbaton;
      struct dwarf_block *block = attr->as_block ();

      dlbaton = XOBNEW (&objfile->objfile_obstack, struct dwarf2_locexpr_baton);
      dlbaton->data = block->data;
      dlbaton->size = block->size;
      dlbaton->per_objfile = per_objfile;
      dlbaton->per_cu = cu->per_cu;

      call_site->target.set_loc_dwarf_block (dlbaton);
    }
  else if (attr->form_is_ref ())
    {
      struct dwarf2_cu *target_cu = cu;
      struct die_info *target_die;

      target_die = follow_die_ref (die, attr, &target_cu);
      gdb_assert (target_cu->per_objfile->objfile == objfile);

      struct attribute *ranges_attr
	= dwarf2_attr (target_die, DW_AT_ranges, target_cu);

      if (die_is_declaration (target_die, target_cu))
	{
	  const char *target_physname;

	  /* Prefer the mangled name; otherwise compute the demangled one.  */
	  target_physname = dw2_linkage_name (target_die, target_cu);
	  if (target_physname == NULL)
	    target_physname = dwarf2_physname (NULL, target_die, target_cu);
	  if (target_physname == NULL)
	    complaint (_("DW_AT_call_target target DIE has invalid "
			 "physname, for referencing DIE %s [in module %s]"),
		       sect_offset_str (die->sect_off), objfile_name (objfile));
	  else
	    call_site->target.set_loc_physname (target_physname);
	}
      else if (ranges_attr != nullptr && ranges_attr->form_is_unsigned ())
	{
	  ULONGEST ranges_offset = (ranges_attr->as_unsigned ()
				    + target_cu->gnu_ranges_base);
	  std::vector<unrelocated_addr> addresses;
	  dwarf2_ranges_read_low_addrs (ranges_offset, target_cu,
					target_die->tag, addresses);
	  unrelocated_addr *saved = XOBNEWVEC (&objfile->objfile_obstack,
					       unrelocated_addr,
					       addresses.size ());
	  std::copy (addresses.begin (), addresses.end (), saved);
	  call_site->target.set_loc_array (addresses.size (), saved);
	}
      else
	{
	  unrelocated_addr lowpc;

	  /* DW_AT_entry_pc should be preferred.  */
	  if (dwarf2_get_pc_bounds (target_die, &lowpc, NULL, target_cu,
				    nullptr, nullptr)
	      <= PC_BOUNDS_INVALID)
	    complaint (_("DW_AT_call_target target DIE has invalid "
			 "low pc, for referencing DIE %s [in module %s]"),
		       sect_offset_str (die->sect_off), objfile_name (objfile));
	  else
	    {
	      lowpc = per_objfile->adjust (lowpc);
	      call_site->target.set_loc_physaddr (lowpc);
	    }
	}
    }
  else
    complaint (_("DW_TAG_call_site DW_AT_call_target is neither "
		 "block nor reference, for DIE %s [in module %s]"),
	       sect_offset_str (die->sect_off), objfile_name (objfile));

  for (child_die = die->child;
       child_die && child_die->tag;
       child_die = child_die->sibling)
    {
      struct call_site_parameter *parameter;
      struct attribute *loc, *origin;

      if (child_die->tag != DW_TAG_call_site_parameter
	  && child_die->tag != DW_TAG_GNU_call_site_parameter)
	{
	  /* Already printed the complaint above.  */
	  continue;
	}

      gdb_assert (call_site->parameter_count < nparams);
      parameter = &call_site->parameter[call_site->parameter_count];

      /* DW_AT_location specifies the register number or DW_AT_abstract_origin
	 specifies DW_TAG_formal_parameter.  Value of the data assumed for the
	 register is contained in DW_AT_call_value.  */

      loc = dwarf2_attr (child_die, DW_AT_location, cu);
      origin = dwarf2_attr (child_die, DW_AT_call_parameter, cu);
      if (origin == NULL)
	{
	  /* This was a pre-DWARF-5 GNU extension alias
	     for DW_AT_call_parameter.  */
	  origin = dwarf2_attr (child_die, DW_AT_abstract_origin, cu);
	}
      if (loc == NULL && origin != NULL && origin->form_is_ref ())
	{
	  parameter->kind = CALL_SITE_PARAMETER_PARAM_OFFSET;

	  sect_offset sect_off = origin->get_ref_die_offset ();
	  if (!cu->header.offset_in_cu_p (sect_off))
	    {
	      /* As DW_OP_GNU_parameter_ref uses CU-relative offset this
		 binding can be done only inside one CU.  Such referenced DIE
		 therefore cannot be even moved to DW_TAG_partial_unit.  */
	      complaint (_("DW_AT_call_parameter offset is not in CU for "
			   "DW_TAG_call_site child DIE %s [in module %s]"),
			 sect_offset_str (child_die->sect_off),
			 objfile_name (objfile));
	      continue;
	    }
	  parameter->u.param_cu_off
	    = (cu_offset) (sect_off - cu->header.sect_off);
	}
      else if (loc == NULL || origin != NULL || !loc->form_is_block ())
	{
	  complaint (_("No DW_FORM_block* DW_AT_location for "
		       "DW_TAG_call_site child DIE %s [in module %s]"),
		     sect_offset_str (child_die->sect_off), objfile_name (objfile));
	  continue;
	}
      else
	{
	  struct dwarf_block *block = loc->as_block ();

	  parameter->u.dwarf_reg = dwarf_block_to_dwarf_reg
	    (block->data, &block->data[block->size]);
	  if (parameter->u.dwarf_reg != -1)
	    parameter->kind = CALL_SITE_PARAMETER_DWARF_REG;
	  else if (dwarf_block_to_sp_offset (gdbarch, block->data,
				    &block->data[block->size],
					     &parameter->u.fb_offset))
	    parameter->kind = CALL_SITE_PARAMETER_FB_OFFSET;
	  else
	    {
	      complaint (_("Only single DW_OP_reg or DW_OP_fbreg is supported "
			   "for DW_FORM_block* DW_AT_location is supported for "
			   "DW_TAG_call_site child DIE %s "
			   "[in module %s]"),
			 sect_offset_str (child_die->sect_off),
			 objfile_name (objfile));
	      continue;
	    }
	}

      attr = dwarf2_attr (child_die, DW_AT_call_value, cu);
      if (attr == NULL)
	attr = dwarf2_attr (child_die, DW_AT_GNU_call_site_value, cu);
      if (attr == NULL || !attr->form_is_block ())
	{
	  complaint (_("No DW_FORM_block* DW_AT_call_value for "
		       "DW_TAG_call_site child DIE %s [in module %s]"),
		     sect_offset_str (child_die->sect_off),
		     objfile_name (objfile));
	  continue;
	}

      struct dwarf_block *block = attr->as_block ();
      parameter->value = block->data;
      parameter->value_size = block->size;

      /* Parameters are not pre-cleared by memset above.  */
      parameter->data_value = NULL;
      parameter->data_value_size = 0;
      call_site->parameter_count++;

      attr = dwarf2_attr (child_die, DW_AT_call_data_value, cu);
      if (attr == NULL)
	attr = dwarf2_attr (child_die, DW_AT_GNU_call_site_data_value, cu);
      if (attr != nullptr)
	{
	  if (!attr->form_is_block ())
	    complaint (_("No DW_FORM_block* DW_AT_call_data_value for "
			 "DW_TAG_call_site child DIE %s [in module %s]"),
		       sect_offset_str (child_die->sect_off),
		       objfile_name (objfile));
	  else
	    {
	      block = attr->as_block ();
	      parameter->data_value = block->data;
	      parameter->data_value_size = block->size;
	    }
	}
    }
}

/* Helper function for read_variable.  If DIE represents a virtual
   table, then return the type of the concrete object that is
   associated with the virtual table.  Otherwise, return NULL.  */

static struct type *
rust_containing_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *attr = dwarf2_attr (die, DW_AT_type, cu);
  if (attr == NULL)
    return NULL;

  /* Find the type DIE.  */
  struct die_info *type_die = NULL;
  struct dwarf2_cu *type_cu = cu;

  if (attr->form_is_ref ())
    type_die = follow_die_ref (die, attr, &type_cu);
  if (type_die == NULL)
    return NULL;

  if (dwarf2_attr (type_die, DW_AT_containing_type, type_cu) == NULL)
    return NULL;
  return die_containing_type (type_die, type_cu);
}

/* Read a variable (DW_TAG_variable) DIE and create a new symbol.  */

static void
read_variable (struct die_info *die, struct dwarf2_cu *cu)
{
  struct rust_vtable_symbol *storage = NULL;

  if (cu->lang () == language_rust)
    {
      struct type *containing_type = rust_containing_type (die, cu);

      if (containing_type != NULL)
	{
	  struct objfile *objfile = cu->per_objfile->objfile;

	  storage = new (&objfile->objfile_obstack) rust_vtable_symbol;
	  storage->concrete_type = containing_type;
	  storage->subclass = SYMBOL_RUST_VTABLE;
	}
    }

  struct symbol *res = new_symbol (die, NULL, cu, storage);
  struct attribute *abstract_origin
    = dwarf2_attr (die, DW_AT_abstract_origin, cu);
  struct attribute *loc = dwarf2_attr (die, DW_AT_location, cu);
  if (res == NULL && loc && abstract_origin)
    {
      /* We have a variable without a name, but with a location and an abstract
	 origin.  This may be a concrete instance of an abstract variable
	 referenced from an DW_OP_GNU_variable_value, so save it to find it back
	 later.  */
      struct dwarf2_cu *origin_cu = cu;
      struct die_info *origin_die
	= follow_die_ref (die, abstract_origin, &origin_cu);
      dwarf2_per_objfile *per_objfile = cu->per_objfile;
      per_objfile->per_bfd->abstract_to_concrete
	[origin_die->sect_off].push_back (die->sect_off);
    }
}

/* Call CALLBACK from DW_AT_ranges attribute value OFFSET
   reading .debug_rnglists.
   Callback's type should be:
    void (CORE_ADDR range_beginning, CORE_ADDR range_end)
   Return true if the attributes are present and valid, otherwise,
   return false.  */

template <typename Callback>
static bool
dwarf2_rnglists_process (unsigned offset, struct dwarf2_cu *cu,
			 dwarf_tag tag, Callback &&callback)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  bfd *obfd = objfile->obfd.get ();
  /* Base address selection entry.  */
  std::optional<unrelocated_addr> base;
  const gdb_byte *buffer;
  bool overflow = false;
  ULONGEST addr_index;
  struct dwarf2_section_info *rnglists_section;

  base = cu->base_address;
  rnglists_section = cu_debug_rnglists_section (cu, tag);
  rnglists_section->read (objfile);

  if (offset >= rnglists_section->size)
    {
      complaint (_("Offset %d out of bounds for DW_AT_ranges attribute"),
		 offset);
      return false;
    }
  buffer = rnglists_section->buffer + offset;

  while (1)
    {
      /* Initialize it due to a false compiler warning.  */
      unrelocated_addr range_beginning = {}, range_end = {};
      const gdb_byte *buf_end = (rnglists_section->buffer
				 + rnglists_section->size);
      unsigned int bytes_read;

      if (buffer == buf_end)
	{
	  overflow = true;
	  break;
	}
      const auto rlet = static_cast<enum dwarf_range_list_entry>(*buffer++);
      switch (rlet)
	{
	case DW_RLE_end_of_list:
	  break;
	case DW_RLE_base_address:
	  if (buffer + cu->header.addr_size > buf_end)
	    {
	      overflow = true;
	      break;
	    }
	  base = cu->header.read_address (obfd, buffer, &bytes_read);
	  buffer += bytes_read;
	  break;
	case DW_RLE_base_addressx:
	  addr_index = read_unsigned_leb128 (obfd, buffer, &bytes_read);
	  buffer += bytes_read;
	  base = read_addr_index (cu, addr_index);
	  break;
	case DW_RLE_start_length:
	  if (buffer + cu->header.addr_size > buf_end)
	    {
	      overflow = true;
	      break;
	    }
	  range_beginning = cu->header.read_address (obfd, buffer,
						     &bytes_read);
	  buffer += bytes_read;
	  range_end
	    = (unrelocated_addr) ((CORE_ADDR) range_beginning
				  + read_unsigned_leb128 (obfd, buffer,
							  &bytes_read));
	  buffer += bytes_read;
	  if (buffer > buf_end)
	    {
	      overflow = true;
	      break;
	    }
	  break;
	case DW_RLE_startx_length:
	  addr_index = read_unsigned_leb128 (obfd, buffer, &bytes_read);
	  buffer += bytes_read;
	  range_beginning = read_addr_index (cu, addr_index);
	  if (buffer > buf_end)
	    {
	      overflow = true;
	      break;
	    }
	  range_end
	    = (unrelocated_addr) ((CORE_ADDR) range_beginning
				  + read_unsigned_leb128 (obfd, buffer,
							  &bytes_read));
	  buffer += bytes_read;
	  break;
	case DW_RLE_offset_pair:
	  range_beginning = (unrelocated_addr) read_unsigned_leb128 (obfd, buffer,
								     &bytes_read);
	  buffer += bytes_read;
	  if (buffer > buf_end)
	    {
	      overflow = true;
	      break;
	    }
	  range_end = (unrelocated_addr) read_unsigned_leb128 (obfd, buffer,
							       &bytes_read);
	  buffer += bytes_read;
	  if (buffer > buf_end)
	    {
	      overflow = true;
	      break;
	    }
	  break;
	case DW_RLE_start_end:
	  if (buffer + 2 * cu->header.addr_size > buf_end)
	    {
	      overflow = true;
	      break;
	    }
	  range_beginning = cu->header.read_address (obfd, buffer,
						     &bytes_read);
	  buffer += bytes_read;
	  range_end = cu->header.read_address (obfd, buffer, &bytes_read);
	  buffer += bytes_read;
	  break;
	case DW_RLE_startx_endx:
	  addr_index = read_unsigned_leb128 (obfd, buffer, &bytes_read);
	  buffer += bytes_read;
	  range_beginning = read_addr_index (cu, addr_index);
	  if (buffer > buf_end)
	    {
	      overflow = true;
	      break;
	    }
	  addr_index = read_unsigned_leb128 (obfd, buffer, &bytes_read);
	  buffer += bytes_read;
	  range_end = read_addr_index (cu, addr_index);
	  break;
	default:
	  complaint (_("Invalid .debug_rnglists data (no base address)"));
	  return false;
	}
      if (rlet == DW_RLE_end_of_list || overflow)
	break;
      if (rlet == DW_RLE_base_address)
	continue;

      if (range_beginning > range_end)
	{
	  /* Inverted range entries are invalid.  */
	  complaint (_("Invalid .debug_rnglists data (inverted range)"));
	  return false;
	}

      /* Empty range entries have no effect.  */
      if (range_beginning == range_end)
	continue;

      /* Only DW_RLE_offset_pair needs the base address added.  */
      if (rlet == DW_RLE_offset_pair)
	{
	  if (!base.has_value ())
	    {
	      /* We have no valid base address for the DW_RLE_offset_pair.  */
	      complaint (_("Invalid .debug_rnglists data (no base address for "
			   "DW_RLE_offset_pair)"));
	      return false;
	    }

	  range_beginning = (unrelocated_addr) ((CORE_ADDR) range_beginning
						+ (CORE_ADDR) *base);
	  range_end = (unrelocated_addr) ((CORE_ADDR) range_end
					  + (CORE_ADDR) *base);
	}

      /* A not-uncommon case of bad debug info.
	 Don't pollute the addrmap with bad data.  */
      if (range_beginning == (unrelocated_addr) 0
	  && !per_objfile->per_bfd->has_section_at_zero)
	{
	  complaint (_(".debug_rnglists entry has start address of zero"
		       " [in module %s]"), objfile_name (objfile));
	  continue;
	}

      callback (range_beginning, range_end);
    }

  if (overflow)
    {
      complaint (_("Offset %d is not terminated "
		   "for DW_AT_ranges attribute"),
		 offset);
      return false;
    }

  return true;
}

/* Call CALLBACK from DW_AT_ranges attribute value OFFSET reading .debug_ranges.
   Callback's type should be:
    void (unrelocated_addr range_beginning, unrelocated_addr range_end)
   Return 1 if the attributes are present and valid, otherwise, return 0.  */

template <typename Callback>
static int
dwarf2_ranges_process (unsigned offset, struct dwarf2_cu *cu, dwarf_tag tag,
		       Callback &&callback)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  struct comp_unit_head *cu_header = &cu->header;
  bfd *obfd = objfile->obfd.get ();
  unsigned int addr_size = cu_header->addr_size;
  CORE_ADDR mask = ~(~(CORE_ADDR)1 << (addr_size * 8 - 1));
  /* Base address selection entry.  */
  std::optional<unrelocated_addr> base;
  unsigned int dummy;
  const gdb_byte *buffer;

  if (cu_header->version >= 5)
    return dwarf2_rnglists_process (offset, cu, tag, callback);

  base = cu->base_address;

  per_objfile->per_bfd->ranges.read (objfile);
  if (offset >= per_objfile->per_bfd->ranges.size)
    {
      complaint (_("Offset %d out of bounds for DW_AT_ranges attribute"),
		 offset);
      return 0;
    }
  buffer = per_objfile->per_bfd->ranges.buffer + offset;

  while (1)
    {
      unrelocated_addr range_beginning, range_end;

      range_beginning = cu->header.read_address (obfd, buffer, &dummy);
      buffer += addr_size;
      range_end = cu->header.read_address (obfd, buffer, &dummy);
      buffer += addr_size;
      offset += 2 * addr_size;

      /* An end of list marker is a pair of zero addresses.  */
      if (range_beginning == (unrelocated_addr) 0
	  && range_end == (unrelocated_addr) 0)
	/* Found the end of list entry.  */
	break;

      /* Each base address selection entry is a pair of 2 values.
	 The first is the largest possible address, the second is
	 the base address.  Check for a base address here.  */
      if (((CORE_ADDR) range_beginning & mask) == mask)
	{
	  /* If we found the largest possible address, then we already
	     have the base address in range_end.  */
	  base = range_end;
	  continue;
	}

      if (!base.has_value ())
	{
	  /* We have no valid base address for the ranges
	     data.  */
	  complaint (_("Invalid .debug_ranges data (no base address)"));
	  return 0;
	}

      if (range_beginning > range_end)
	{
	  /* Inverted range entries are invalid.  */
	  complaint (_("Invalid .debug_ranges data (inverted range)"));
	  return 0;
	}

      /* Empty range entries have no effect.  */
      if (range_beginning == range_end)
	continue;

      range_beginning = (unrelocated_addr) ((CORE_ADDR) range_beginning
					    + (CORE_ADDR) *base);
      range_end = (unrelocated_addr) ((CORE_ADDR) range_end
				      + (CORE_ADDR) *base);

      /* A not-uncommon case of bad debug info.
	 Don't pollute the addrmap with bad data.  */
      if (range_beginning == (unrelocated_addr) 0
	  && !per_objfile->per_bfd->has_section_at_zero)
	{
	  complaint (_(".debug_ranges entry has start address of zero"
		       " [in module %s]"), objfile_name (objfile));
	  continue;
	}

      callback (range_beginning, range_end);
    }

  return 1;
}

/* Get low and high pc attributes from DW_AT_ranges attribute value OFFSET.
   Return 1 if the attributes are present and valid, otherwise, return 0.
   TAG is passed to dwarf2_ranges_process.  If MAP is not NULL, then
   ranges in MAP are set, using DATUM as the value.  */

static int
dwarf2_ranges_read (unsigned offset, unrelocated_addr *low_return,
		    unrelocated_addr *high_return, struct dwarf2_cu *cu,
		    addrmap *map, void *datum, dwarf_tag tag)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  int low_set = 0;
  unrelocated_addr low = {};
  unrelocated_addr high = {};
  int retval;

  retval = dwarf2_ranges_process (offset, cu, tag,
    [&] (unrelocated_addr range_beginning, unrelocated_addr range_end)
    {
      if (map != nullptr)
	{
	  unrelocated_addr lowpc;
	  unrelocated_addr highpc;

	  lowpc = per_objfile->adjust (range_beginning);
	  highpc = per_objfile->adjust (range_end);
	  /* addrmap only accepts CORE_ADDR, so we must cast here.  */
	  map->set_empty ((CORE_ADDR) lowpc, (CORE_ADDR) highpc - 1, datum);
	}

      /* FIXME: This is recording everything as a low-high
	 segment of consecutive addresses.  We should have a
	 data structure for discontiguous block ranges
	 instead.  */
      if (! low_set)
	{
	  low = range_beginning;
	  high = range_end;
	  low_set = 1;
	}
      else
	{
	  if (range_beginning < low)
	    low = range_beginning;
	  if (range_end > high)
	    high = range_end;
	}
    });
  if (!retval)
    return 0;

  if (! low_set)
    /* If the first entry is an end-of-list marker, the range
       describes an empty scope, i.e. no instructions.  */
    return 0;

  if (low_return)
    *low_return = low;
  if (high_return)
    *high_return = high;
  return 1;
}

/* Process ranges and fill in a vector of the low PC values only.  */

static void
dwarf2_ranges_read_low_addrs (unsigned offset, struct dwarf2_cu *cu,
			      dwarf_tag tag,
			      std::vector<unrelocated_addr> &result)
{
  dwarf2_ranges_process (offset, cu, tag,
			 [&] (unrelocated_addr start, unrelocated_addr end)
    {
      result.push_back (start);
    });
}

/* Determine the low and high pc of a DW_TAG_entry_point.  */

static pc_bounds_kind
dwarf2_get_pc_bounds_entry_point (die_info *die, unrelocated_addr *low,
				  unrelocated_addr *high, dwarf2_cu *cu)
{
  gdb_assert (low != nullptr);
  gdb_assert (high != nullptr);

  if (die->parent->tag != DW_TAG_subprogram)
    {
      complaint (_("DW_TAG_entry_point not embedded in DW_TAG_subprogram"));
      return PC_BOUNDS_INVALID;
    }

  /* A DW_TAG_entry_point is embedded in an subprogram.  Therefore, we can use
     the highpc from its enveloping subprogram and get the lowpc from
     DWARF.  */
  const enum pc_bounds_kind bounds_kind = dwarf2_get_pc_bounds (die->parent,
								low, high,
								cu, nullptr,
								nullptr);
  if (bounds_kind == PC_BOUNDS_INVALID || bounds_kind == PC_BOUNDS_NOT_PRESENT)
    return bounds_kind;

  attribute *attr_low = dwarf2_attr (die, DW_AT_low_pc, cu);
  if (!attr_low)
    {
      complaint (_("DW_TAG_entry_point is missing DW_AT_low_pc"));
      return PC_BOUNDS_INVALID;
    }
  *low = attr_low->as_address ();
  return bounds_kind;
}

/* Determine the low and high pc using the DW_AT_low_pc and DW_AT_high_pc or
   DW_AT_ranges attributes of a DIE.  */

static pc_bounds_kind
dwarf_get_pc_bounds_ranges_or_highlow_pc (die_info *die, unrelocated_addr *low,
					  unrelocated_addr *high, dwarf2_cu *cu,
					  addrmap *map, void *datum)
{
  gdb_assert (low != nullptr);
  gdb_assert (high != nullptr);

  struct attribute *attr;
  struct attribute *attr_high;
  enum pc_bounds_kind ret;

  attr_high = dwarf2_attr (die, DW_AT_high_pc, cu);
  if (attr_high)
    {
      attr = dwarf2_attr (die, DW_AT_low_pc, cu);
      if (attr != nullptr)
	{
	  *low = attr->as_address ();
	  *high = attr_high->as_address ();
	  if (cu->header.version >= 4 && attr_high->form_is_constant ())
	    *high = (unrelocated_addr) ((ULONGEST) *high + (ULONGEST) *low);

	  /* Found consecutive range of addresses.  */
	  ret = PC_BOUNDS_HIGH_LOW;
	}
      else
	{
	  /* Found high w/o low attribute.  */
	  ret = PC_BOUNDS_INVALID;
	}
    }
  else
    {
      attr = dwarf2_attr (die, DW_AT_ranges, cu);
      if (attr != nullptr && attr->form_is_unsigned ())
	{
	  /* Offset in the .debug_ranges or .debug_rnglist section (depending
	     on DWARF version).  */
	  ULONGEST ranges_offset = attr->as_unsigned ();

	  /* See dwarf2_cu::gnu_ranges_base's doc for why we might want to add
	     this value.  */
	  if (die->tag != DW_TAG_compile_unit)
	    ranges_offset += cu->gnu_ranges_base;

	  /* Value of the DW_AT_ranges attribute is the offset in the
	     .debug_ranges section.  */
	  if (!dwarf2_ranges_read (ranges_offset, low, high, cu,
				   map, datum, die->tag))
	    return PC_BOUNDS_INVALID;
	  /* Found discontinuous range of addresses.  */
	  ret = PC_BOUNDS_RANGES;
	}
      else
	{
	  /* Could not find high_pc or ranges attributed and thus no bounds
	     pair.  */
	  ret = PC_BOUNDS_NOT_PRESENT;
	}
    }

    return ret;
}

/* Get low and high pc attributes from a die.  See enum pc_bounds_kind
   definition for the return value.  *LOWPC and *HIGHPC are set iff
   neither PC_BOUNDS_NOT_PRESENT nor PC_BOUNDS_INVALID are returned.  */

static enum pc_bounds_kind
dwarf2_get_pc_bounds (struct die_info *die, unrelocated_addr *lowpc,
		      unrelocated_addr *highpc, struct dwarf2_cu *cu,
		      addrmap *map, void *datum)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;

  unrelocated_addr low = {};
  unrelocated_addr high = {};
  enum pc_bounds_kind ret;

  if (die->tag == DW_TAG_entry_point)
    ret = dwarf2_get_pc_bounds_entry_point (die, &low, &high, cu);
  else
    ret = dwarf_get_pc_bounds_ranges_or_highlow_pc (die, &low, &high, cu, map,
						    datum);

  if (ret == PC_BOUNDS_NOT_PRESENT || ret == PC_BOUNDS_INVALID)
    return ret;

  /* partial_die_info::read has also the strict LOW < HIGH requirement.  */
  if (high <= low)
    return PC_BOUNDS_INVALID;

  /* When using the GNU linker, .gnu.linkonce. sections are used to
     eliminate duplicate copies of functions and vtables and such.
     The linker will arbitrarily choose one and discard the others.
     The AT_*_pc values for such functions refer to local labels in
     these sections.  If the section from that file was discarded, the
     labels are not in the output, so the relocs get a value of 0.
     If this is a discarded function, mark the pc bounds as invalid,
     so that GDB will ignore it.  */
  if (low == (unrelocated_addr) 0
      && !per_objfile->per_bfd->has_section_at_zero)
    return PC_BOUNDS_INVALID;

  gdb_assert (lowpc != nullptr);
  *lowpc = low;
  if (highpc != nullptr)
    *highpc = high;
  return ret;
}

/* Assuming that DIE represents a subprogram DIE or a lexical block, get
   its low and high PC addresses.  Do nothing if these addresses could not
   be determined.  Otherwise, set LOWPC to the low address if it is smaller,
   and HIGHPC to the high address if greater than HIGHPC.  */

static void
dwarf2_get_subprogram_pc_bounds (struct die_info *die,
				 unrelocated_addr *lowpc,
				 unrelocated_addr *highpc,
				 struct dwarf2_cu *cu)
{
  unrelocated_addr low, high;
  struct die_info *child = die->child;

  if (dwarf2_get_pc_bounds (die, &low, &high, cu, nullptr, nullptr)
      >= PC_BOUNDS_RANGES)
    {
      *lowpc = std::min (*lowpc, low);
      *highpc = std::max (*highpc, high);
    }

  /* If the language does not allow nested subprograms (either inside
     subprograms or lexical blocks), we're done.  */
  if (cu->lang () != language_ada)
    return;

  /* Check all the children of the given DIE.  If it contains nested
     subprograms, then check their pc bounds.  Likewise, we need to
     check lexical blocks as well, as they may also contain subprogram
     definitions.  */
  while (child && child->tag)
    {
      if (child->tag == DW_TAG_subprogram
	  || child->tag == DW_TAG_lexical_block)
	dwarf2_get_subprogram_pc_bounds (child, lowpc, highpc, cu);
      child = child->sibling;
    }
}

/* Get the low and high pc's represented by the scope DIE, and store
   them in *LOWPC and *HIGHPC.  If the correct values can't be
   determined, set *LOWPC to -1 and *HIGHPC to 0.  */

static void
get_scope_pc_bounds (struct die_info *die,
		     unrelocated_addr *lowpc, unrelocated_addr *highpc,
		     struct dwarf2_cu *cu)
{
  unrelocated_addr best_low = (unrelocated_addr) -1;
  unrelocated_addr best_high = {};
  unrelocated_addr current_low, current_high;

  if (dwarf2_get_pc_bounds (die, &current_low, &current_high, cu,
			    nullptr, nullptr)
      >= PC_BOUNDS_RANGES)
    {
      best_low = current_low;
      best_high = current_high;
    }
  else
    {
      struct die_info *child = die->child;

      while (child && child->tag)
	{
	  switch (child->tag) {
	  case DW_TAG_subprogram:
	    dwarf2_get_subprogram_pc_bounds (child, &best_low, &best_high, cu);
	    break;
	  case DW_TAG_namespace:
	  case DW_TAG_module:
	    /* FIXME: carlton/2004-01-16: Should we do this for
	       DW_TAG_class_type/DW_TAG_structure_type, too?  I think
	       that current GCC's always emit the DIEs corresponding
	       to definitions of methods of classes as children of a
	       DW_TAG_compile_unit or DW_TAG_namespace (as opposed to
	       the DIEs giving the declarations, which could be
	       anywhere).  But I don't see any reason why the
	       standards says that they have to be there.  */
	    get_scope_pc_bounds (child, &current_low, &current_high, cu);

	    if (current_low != ((unrelocated_addr) -1))
	      {
		best_low = std::min (best_low, current_low);
		best_high = std::max (best_high, current_high);
	      }
	    break;
	  default:
	    /* Ignore.  */
	    break;
	  }

	  child = child->sibling;
	}
    }

  *lowpc = best_low;
  *highpc = best_high;
}

/* Record the address ranges for BLOCK, offset by BASEADDR, as given
   in DIE.  */

static void
dwarf2_record_block_ranges (struct die_info *die, struct block *block,
			    struct dwarf2_cu *cu)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  struct attribute *attr;
  struct attribute *attr_high;

  attr_high = dwarf2_attr (die, DW_AT_high_pc, cu);
  if (attr_high)
    {
      attr = dwarf2_attr (die, DW_AT_low_pc, cu);
      if (attr != nullptr)
	{
	  unrelocated_addr unrel_low = attr->as_address ();
	  unrelocated_addr unrel_high = attr_high->as_address ();

	  if (cu->header.version >= 4 && attr_high->form_is_constant ())
	    unrel_high = (unrelocated_addr) ((ULONGEST) unrel_high
					     + (ULONGEST) unrel_low);

	  CORE_ADDR low = per_objfile->relocate (unrel_low);
	  CORE_ADDR high = per_objfile->relocate (unrel_high);
	  cu->get_builder ()->record_block_range (block, low, high - 1);
	}
    }

  attr = dwarf2_attr (die, DW_AT_ranges, cu);
  if (attr != nullptr && attr->form_is_unsigned ())
    {
      /* Offset in the .debug_ranges or .debug_rnglist section (depending
	 on DWARF version).  */
      ULONGEST ranges_offset = attr->as_unsigned ();

      /* See dwarf2_cu::gnu_ranges_base's doc for why we might want to add
	 this value.  */
      if (die->tag != DW_TAG_compile_unit)
	ranges_offset += cu->gnu_ranges_base;

      std::vector<blockrange> blockvec;
      dwarf2_ranges_process (ranges_offset, cu, die->tag,
	[&] (unrelocated_addr start, unrelocated_addr end)
	{
	  CORE_ADDR abs_start = per_objfile->relocate (start);
	  CORE_ADDR abs_end = per_objfile->relocate (end);
	  cu->get_builder ()->record_block_range (block, abs_start,
						  abs_end - 1);
	  blockvec.emplace_back (abs_start, abs_end);
	});

      block->set_ranges (make_blockranges (objfile, blockvec));
    }
}

/* Check whether the producer field indicates either of GCC < 4.6, or the
   Intel C/C++ compiler, and cache the result in CU.  */

static void
check_producer (struct dwarf2_cu *cu)
{
  int major, minor;

  if (cu->producer == NULL)
    {
      /* For unknown compilers expect their behavior is DWARF version
	 compliant.

	 GCC started to support .debug_types sections by -gdwarf-4 since
	 gcc-4.5.x.  As the .debug_types sections are missing DW_AT_producer
	 for their space efficiency GDB cannot workaround gcc-4.5.x -gdwarf-4
	 combination.  gcc-4.5.x -gdwarf-4 binaries have DW_AT_accessibility
	 interpreted incorrectly by GDB now - GCC PR debug/48229.  */
    }
  else if (producer_is_gcc (cu->producer, &major, &minor))
    {
      cu->producer_is_gxx_lt_4_6 = major < 4 || (major == 4 && minor < 6);
      cu->producer_is_gcc_lt_4_3 = major < 4 || (major == 4 && minor < 3);
      cu->producer_is_gcc_11 = major == 11;
    }
  else if (producer_is_icc (cu->producer, &major, &minor))
    {
      cu->producer_is_icc = true;
      cu->producer_is_icc_lt_14 = major < 14;
    }
  else if (startswith (cu->producer, "CodeWarrior S12/L-ISA"))
    cu->producer_is_codewarrior = true;
  else if (producer_is_clang (cu->producer, &major, &minor))
    cu->producer_is_clang = true;
  else if (producer_is_gas (cu->producer, &major, &minor))
    {
      cu->producer_is_gas_lt_2_38 = major < 2 || (major == 2 && minor < 38);
      cu->producer_is_gas_2_39 = major == 2 && minor == 39;
    }
  else
    {
      /* For other non-GCC compilers, expect their behavior is DWARF version
	 compliant.  */
    }

  cu->checked_producer = true;
}

/* Check for GCC PR debug/45124 fix which is not present in any G++ version up
   to 4.5.any while it is present already in G++ 4.6.0 - the PR has been fixed
   during 4.6.0 experimental.  */

static bool
producer_is_gxx_lt_4_6 (struct dwarf2_cu *cu)
{
  if (!cu->checked_producer)
    check_producer (cu);

  return cu->producer_is_gxx_lt_4_6;
}


/* Codewarrior (at least as of version 5.0.40) generates dwarf line information
   with incorrect is_stmt attributes.  */

static bool
producer_is_codewarrior (struct dwarf2_cu *cu)
{
  if (!cu->checked_producer)
    check_producer (cu);

  return cu->producer_is_codewarrior;
}

static bool
producer_is_gas_lt_2_38 (struct dwarf2_cu *cu)
{
  if (!cu->checked_producer)
    check_producer (cu);

  return cu->producer_is_gas_lt_2_38;
}

static bool
producer_is_gas_2_39 (struct dwarf2_cu *cu)
{
  if (!cu->checked_producer)
    check_producer (cu);

  return cu->producer_is_gas_2_39;
}

/* Return the accessibility of DIE, as given by DW_AT_accessibility.
   If that attribute is not available, return the appropriate
   default.  */

static enum dwarf_access_attribute
dwarf2_access_attribute (struct die_info *die, struct dwarf2_cu *cu)
{
  attribute *attr = dwarf2_attr (die, DW_AT_accessibility, cu);
  if (attr != nullptr)
    {
      LONGEST value = attr->constant_value (-1);
      if (value == DW_ACCESS_public
	  || value == DW_ACCESS_protected
	  || value == DW_ACCESS_private)
	return (dwarf_access_attribute) value;
      complaint (_("Unhandled DW_AT_accessibility value (%s)"),
		 plongest (value));
    }

  if (cu->header.version < 3 || producer_is_gxx_lt_4_6 (cu))
    {
      /* The default DWARF 2 accessibility for members is public, the default
	 accessibility for inheritance is private.  */

      if (die->tag != DW_TAG_inheritance)
	return DW_ACCESS_public;
      else
	return DW_ACCESS_private;
    }
  else
    {
      /* DWARF 3+ defines the default accessibility a different way.  The same
	 rules apply now for DW_TAG_inheritance as for the members and it only
	 depends on the container kind.  */

      if (die->parent->tag == DW_TAG_class_type)
	return DW_ACCESS_private;
      else
	return DW_ACCESS_public;
    }
}

/* Look for DW_AT_data_member_location or DW_AT_data_bit_offset.  Set
   *OFFSET to the byte offset.  If the attribute was not found return
   0, otherwise return 1.  If it was found but could not properly be
   handled, set *OFFSET to 0.  */

static int
handle_member_location (struct die_info *die, struct dwarf2_cu *cu,
			LONGEST *offset)
{
  struct attribute *attr;

  attr = dwarf2_attr (die, DW_AT_data_member_location, cu);
  if (attr != NULL)
    {
      *offset = 0;
      CORE_ADDR temp;

      /* Note that we do not check for a section offset first here.
	 This is because DW_AT_data_member_location is new in DWARF 4,
	 so if we see it, we can assume that a constant form is really
	 a constant and not a section offset.  */
      if (attr->form_is_constant ())
	*offset = attr->constant_value (0);
      else if (attr->form_is_section_offset ())
	dwarf2_complex_location_expr_complaint ();
      else if (attr->form_is_block ()
	       && decode_locdesc (attr->as_block (), cu, &temp))
	{
	  *offset = temp;
	}
      else
	dwarf2_complex_location_expr_complaint ();

      return 1;
    }
  else
    {
      attr = dwarf2_attr (die, DW_AT_data_bit_offset, cu);
      if (attr != nullptr)
	{
	  *offset = attr->constant_value (0);
	  return 1;
	}
    }

  return 0;
}

/* Look for DW_AT_data_member_location or DW_AT_data_bit_offset and
   store the results in FIELD.  */

static void
handle_member_location (struct die_info *die, struct dwarf2_cu *cu,
			struct field *field)
{
  struct attribute *attr;

  attr = dwarf2_attr (die, DW_AT_data_member_location, cu);
  if (attr != NULL)
    {
      if (attr->form_is_constant ())
	{
	  LONGEST offset = attr->constant_value (0);

	  /* Work around this GCC 11 bug, where it would erroneously use -1
	     data member locations, instead of 0:

	       Negative DW_AT_data_member_location
	       https://gcc.gnu.org/bugzilla/show_bug.cgi?id=101378
	     */
	  if (offset == -1 && cu->producer_is_gcc_11)
	    {
	      complaint (_("DW_AT_data_member_location value of -1, assuming 0"));
	      offset = 0;
	    }

	  field->set_loc_bitpos (offset * bits_per_byte);
	}
      else if (attr->form_is_section_offset ())
	dwarf2_complex_location_expr_complaint ();
      else if (attr->form_is_block ())
	{
	  CORE_ADDR offset;
	  if (decode_locdesc (attr->as_block (), cu, &offset))
	    field->set_loc_bitpos (offset * bits_per_byte);
	  else
	    {
	      dwarf2_per_objfile *per_objfile = cu->per_objfile;
	      struct objfile *objfile = per_objfile->objfile;
	      struct dwarf2_locexpr_baton *dlbaton
		= XOBNEW (&objfile->objfile_obstack,
			  struct dwarf2_locexpr_baton);
	      dlbaton->data = attr->as_block ()->data;
	      dlbaton->size = attr->as_block ()->size;
	      /* When using this baton, we want to compute the address
		 of the field, not the value.  This is why
		 is_reference is set to false here.  */
	      dlbaton->is_reference = false;
	      dlbaton->per_objfile = per_objfile;
	      dlbaton->per_cu = cu->per_cu;

	      field->set_loc_dwarf_block (dlbaton);
	    }
	}
      else
	dwarf2_complex_location_expr_complaint ();
    }
  else
    {
      attr = dwarf2_attr (die, DW_AT_data_bit_offset, cu);
      if (attr != nullptr)
	field->set_loc_bitpos (attr->constant_value (0));
    }
}

/* Add an aggregate field to the field list.  */

static void
dwarf2_add_field (struct field_info *fip, struct die_info *die,
		  struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  struct gdbarch *gdbarch = objfile->arch ();
  struct nextfield *new_field;
  struct attribute *attr;
  struct field *fp;
  const char *fieldname = "";

  if (die->tag == DW_TAG_inheritance)
    {
      fip->baseclasses.emplace_back ();
      new_field = &fip->baseclasses.back ();
    }
  else
    {
      fip->fields.emplace_back ();
      new_field = &fip->fields.back ();
    }

  new_field->offset = die->sect_off;

  switch (dwarf2_access_attribute (die, cu))
    {
    case DW_ACCESS_public:
      break;
    case DW_ACCESS_private:
      new_field->field.set_accessibility (accessibility::PRIVATE);
      break;
    case DW_ACCESS_protected:
      new_field->field.set_accessibility (accessibility::PROTECTED);
      break;
    default:
      gdb_assert_not_reached ("invalid accessibility");
    }

  attr = dwarf2_attr (die, DW_AT_virtuality, cu);
  if (attr != nullptr && attr->as_virtuality ())
    new_field->field.set_virtual ();

  fp = &new_field->field;

  if ((die->tag == DW_TAG_member || die->tag == DW_TAG_namelist_item)
      && !die_is_declaration (die, cu))
    {
      if (die->tag == DW_TAG_namelist_item)
	{
	  /* Typically, DW_TAG_namelist_item are references to namelist items.
	     If so, follow that reference.  */
	  struct attribute *attr1 = dwarf2_attr (die, DW_AT_namelist_item, cu);
	  struct die_info *item_die = nullptr;
	  struct dwarf2_cu *item_cu = cu;
	  if (attr1->form_is_ref ())
	    item_die = follow_die_ref (die, attr1, &item_cu);
	  if (item_die != nullptr)
	    die = item_die;
	}
      /* Data member other than a C++ static data member.  */

      /* Get type of field.  */
      fp->set_type (die_type (die, cu));

      fp->set_loc_bitpos (0);

      /* Get bit size of field (zero if none).  */
      attr = dwarf2_attr (die, DW_AT_bit_size, cu);
      if (attr != nullptr)
	fp->set_bitsize (attr->constant_value (0));
      else
	fp->set_bitsize (0);

      /* Get bit offset of field.  */
      handle_member_location (die, cu, fp);
      attr = dwarf2_attr (die, DW_AT_bit_offset, cu);
      if (attr != nullptr && attr->form_is_constant ())
	{
	  if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
	    {
	      /* For big endian bits, the DW_AT_bit_offset gives the
		 additional bit offset from the MSB of the containing
		 anonymous object to the MSB of the field.  We don't
		 have to do anything special since we don't need to
		 know the size of the anonymous object.  */
	      fp->set_loc_bitpos (fp->loc_bitpos () + attr->constant_value (0));
	    }
	  else
	    {
	      /* For little endian bits, compute the bit offset to the
		 MSB of the anonymous object, subtract off the number of
		 bits from the MSB of the field to the MSB of the
		 object, and then subtract off the number of bits of
		 the field itself.  The result is the bit offset of
		 the LSB of the field.  */
	      int anonymous_size;
	      int bit_offset = attr->constant_value (0);

	      attr = dwarf2_attr (die, DW_AT_byte_size, cu);
	      if (attr != nullptr && attr->form_is_constant ())
		{
		  /* The size of the anonymous object containing
		     the bit field is explicit, so use the
		     indicated size (in bytes).  */
		  anonymous_size = attr->constant_value (0);
		}
	      else
		{
		  /* The size of the anonymous object containing
		     the bit field must be inferred from the type
		     attribute of the data member containing the
		     bit field.  */
		  anonymous_size = fp->type ()->length ();
		}
	      fp->set_loc_bitpos (fp->loc_bitpos ()
				  + anonymous_size * bits_per_byte
				  - bit_offset - fp->bitsize ());
	    }
	}

      /* Get name of field.  */
      fieldname = dwarf2_name (die, cu);
      if (fieldname == NULL)
	fieldname = "";

      /* The name is already allocated along with this objfile, so we don't
	 need to duplicate it for the type.  */
      fp->set_name (fieldname);

      /* Change accessibility for artificial fields (e.g. virtual table
	 pointer or virtual base class pointer) to private.  */
      if (dwarf2_attr (die, DW_AT_artificial, cu))
	{
	  fp->set_is_artificial (true);
	  fp->set_accessibility (accessibility::PRIVATE);
	}
    }
  else if (die->tag == DW_TAG_member || die->tag == DW_TAG_variable)
    {
      /* C++ static member.  */

      /* NOTE: carlton/2002-11-05: It should be a DW_TAG_member that
	 is a declaration, but all versions of G++ as of this writing
	 (so through at least 3.2.1) incorrectly generate
	 DW_TAG_variable tags.  */

      const char *physname;

      /* Get name of field.  */
      fieldname = dwarf2_name (die, cu);
      if (fieldname == NULL)
	return;

      attr = dwarf2_attr (die, DW_AT_const_value, cu);
      if (attr
	  /* Only create a symbol if this is an external value.
	     new_symbol checks this and puts the value in the global symbol
	     table, which we want.  If it is not external, new_symbol
	     will try to put the value in cu->list_in_scope which is wrong.  */
	  && dwarf2_flag_true_p (die, DW_AT_external, cu))
	{
	  /* A static const member, not much different than an enum as far as
	     we're concerned, except that we can support more types.  */
	  new_symbol (die, NULL, cu);
	}

      /* Get physical name.  */
      physname = dwarf2_physname (fieldname, die, cu);

      /* The name is already allocated along with this objfile, so we don't
	 need to duplicate it for the type.  */
      fp->set_loc_physname (physname ? physname : "");
      fp->set_type (die_type (die, cu));
      fp->set_name (fieldname);
    }
  else if (die->tag == DW_TAG_inheritance)
    {
      /* C++ base class field.  */
      handle_member_location (die, cu, fp);
      fp->set_bitsize (0);
      fp->set_type (die_type (die, cu));
      fp->set_name (fp->type ()->name ());
    }
  else
    gdb_assert_not_reached ("missing case in dwarf2_add_field");
}

/* Can the type given by DIE define another type?  */

static bool
type_can_define_types (const struct die_info *die)
{
  switch (die->tag)
    {
    case DW_TAG_typedef:
    case DW_TAG_class_type:
    case DW_TAG_structure_type:
    case DW_TAG_union_type:
    case DW_TAG_enumeration_type:
      return true;

    default:
      return false;
    }
}

/* Add a type definition defined in the scope of the FIP's class.  */

static void
dwarf2_add_type_defn (struct field_info *fip, struct die_info *die,
		      struct dwarf2_cu *cu)
{
  struct decl_field fp;
  memset (&fp, 0, sizeof (fp));

  gdb_assert (type_can_define_types (die));

  /* Get name of field.  NULL is okay here, meaning an anonymous type.  */
  fp.name = dwarf2_name (die, cu);
  fp.type = read_type_die (die, cu);

  /* Save accessibility.  */
  dwarf_access_attribute accessibility = dwarf2_access_attribute (die, cu);
  switch (accessibility)
    {
    case DW_ACCESS_public:
      /* The assumed value if neither private nor protected.  */
      break;
    case DW_ACCESS_private:
      fp.accessibility = accessibility::PRIVATE;
      break;
    case DW_ACCESS_protected:
      fp.accessibility = accessibility::PROTECTED;
      break;
    }

  if (die->tag == DW_TAG_typedef)
    fip->typedef_field_list.push_back (fp);
  else
    fip->nested_types_list.push_back (fp);
}

/* A convenience typedef that's used when finding the discriminant
   field for a variant part.  */
typedef std::unordered_map<sect_offset, int, gdb::hash_enum<sect_offset>>
  offset_map_type;

/* Compute the discriminant range for a given variant.  OBSTACK is
   where the results will be stored.  VARIANT is the variant to
   process.  IS_UNSIGNED indicates whether the discriminant is signed
   or unsigned.  */

static const gdb::array_view<discriminant_range>
convert_variant_range (struct obstack *obstack, const variant_field &variant,
		       bool is_unsigned)
{
  std::vector<discriminant_range> ranges;

  if (variant.default_branch)
    return {};

  if (variant.discr_list_data == nullptr)
    {
      discriminant_range r
	= {variant.discriminant_value, variant.discriminant_value};
      ranges.push_back (r);
    }
  else
    {
      gdb::array_view<const gdb_byte> data (variant.discr_list_data->data,
					    variant.discr_list_data->size);
      while (!data.empty ())
	{
	  if (data[0] != DW_DSC_range && data[0] != DW_DSC_label)
	    {
	      complaint (_("invalid discriminant marker: %d"), data[0]);
	      break;
	    }
	  bool is_range = data[0] == DW_DSC_range;
	  data = data.slice (1);

	  ULONGEST low, high;
	  unsigned int bytes_read;

	  if (data.empty ())
	    {
	      complaint (_("DW_AT_discr_list missing low value"));
	      break;
	    }
	  if (is_unsigned)
	    low = read_unsigned_leb128 (nullptr, data.data (), &bytes_read);
	  else
	    low = (ULONGEST) read_signed_leb128 (nullptr, data.data (),
						 &bytes_read);
	  data = data.slice (bytes_read);

	  if (is_range)
	    {
	      if (data.empty ())
		{
		  complaint (_("DW_AT_discr_list missing high value"));
		  break;
		}
	      if (is_unsigned)
		high = read_unsigned_leb128 (nullptr, data.data (),
					     &bytes_read);
	      else
		high = (LONGEST) read_signed_leb128 (nullptr, data.data (),
						     &bytes_read);
	      data = data.slice (bytes_read);
	    }
	  else
	    high = low;

	  ranges.push_back ({ low, high });
	}
    }

  discriminant_range *result = XOBNEWVEC (obstack, discriminant_range,
					  ranges.size ());
  std::copy (ranges.begin (), ranges.end (), result);
  return gdb::array_view<discriminant_range> (result, ranges.size ());
}

static const gdb::array_view<variant_part> create_variant_parts
  (struct obstack *obstack,
   const offset_map_type &offset_map,
   struct field_info *fi,
   const std::vector<variant_part_builder> &variant_parts);

/* Fill in a "struct variant" for a given variant field.  RESULT is
   the variant to fill in.  OBSTACK is where any needed allocations
   will be done.  OFFSET_MAP holds the mapping from section offsets to
   fields for the type.  FI describes the fields of the type we're
   processing.  FIELD is the variant field we're converting.  */

static void
create_one_variant (variant &result, struct obstack *obstack,
		    const offset_map_type &offset_map,
		    struct field_info *fi, const variant_field &field)
{
  result.discriminants = convert_variant_range (obstack, field, false);
  result.first_field = field.first_field + fi->baseclasses.size ();
  result.last_field = field.last_field + fi->baseclasses.size ();
  result.parts = create_variant_parts (obstack, offset_map, fi,
				       field.variant_parts);
}

/* Fill in a "struct variant_part" for a given variant part.  RESULT
   is the variant part to fill in.  OBSTACK is where any needed
   allocations will be done.  OFFSET_MAP holds the mapping from
   section offsets to fields for the type.  FI describes the fields of
   the type we're processing.  BUILDER is the variant part to be
   converted.  */

static void
create_one_variant_part (variant_part &result,
			 struct obstack *obstack,
			 const offset_map_type &offset_map,
			 struct field_info *fi,
			 const variant_part_builder &builder)
{
  auto iter = offset_map.find (builder.discriminant_offset);
  if (iter == offset_map.end ())
    {
      result.discriminant_index = -1;
      /* Doesn't matter.  */
      result.is_unsigned = false;
    }
  else
    {
      result.discriminant_index = iter->second;
      result.is_unsigned
	= fi->fields[result.discriminant_index].field.type ()->is_unsigned ();
    }

  size_t n = builder.variants.size ();
  variant *output = new (obstack) variant[n];
  for (size_t i = 0; i < n; ++i)
    create_one_variant (output[i], obstack, offset_map, fi,
			builder.variants[i]);

  result.variants = gdb::array_view<variant> (output, n);
}

/* Create a vector of variant parts that can be attached to a type.
   OBSTACK is where any needed allocations will be done.  OFFSET_MAP
   holds the mapping from section offsets to fields for the type.  FI
   describes the fields of the type we're processing.  VARIANT_PARTS
   is the vector to convert.  */

static const gdb::array_view<variant_part>
create_variant_parts (struct obstack *obstack,
		      const offset_map_type &offset_map,
		      struct field_info *fi,
		      const std::vector<variant_part_builder> &variant_parts)
{
  if (variant_parts.empty ())
    return {};

  size_t n = variant_parts.size ();
  variant_part *result = new (obstack) variant_part[n];
  for (size_t i = 0; i < n; ++i)
    create_one_variant_part (result[i], obstack, offset_map, fi,
			     variant_parts[i]);

  return gdb::array_view<variant_part> (result, n);
}

/* Compute the variant part vector for FIP, attaching it to TYPE when
   done.  */

static void
add_variant_property (struct field_info *fip, struct type *type,
		      struct dwarf2_cu *cu)
{
  /* Map section offsets of fields to their field index.  Note the
     field index here does not take the number of baseclasses into
     account.  */
  offset_map_type offset_map;
  for (int i = 0; i < fip->fields.size (); ++i)
    offset_map[fip->fields[i].offset] = i;

  struct objfile *objfile = cu->per_objfile->objfile;
  gdb::array_view<const variant_part> parts
    = create_variant_parts (&objfile->objfile_obstack, offset_map, fip,
			    fip->variant_parts);

  struct dynamic_prop prop;
  prop.set_variant_parts ((gdb::array_view<variant_part> *)
			  obstack_copy (&objfile->objfile_obstack, &parts,
					sizeof (parts)));

  type->add_dyn_prop (DYN_PROP_VARIANT_PARTS, prop);
}

/* Create the vector of fields, and attach it to the type.  */

static void
dwarf2_attach_fields_to_type (struct field_info *fip, struct type *type,
			      struct dwarf2_cu *cu)
{
  int nfields = fip->nfields ();

  /* Record the field count, allocate space for the array of fields,
     and create blank accessibility bitfields if necessary.  */
  type->alloc_fields (nfields);

  if (!fip->baseclasses.empty () && cu->lang () != language_ada)
    {
      ALLOCATE_CPLUS_STRUCT_TYPE (type);
      TYPE_N_BASECLASSES (type) = fip->baseclasses.size ();
    }

  if (!fip->variant_parts.empty ())
    add_variant_property (fip, type, cu);

  /* Copy the saved-up fields into the field vector.  */
  for (int i = 0; i < nfields; ++i)
    {
      struct nextfield &field
	= ((i < fip->baseclasses.size ()) ? fip->baseclasses[i]
	   : fip->fields[i - fip->baseclasses.size ()]);

      type->field (i) = field.field;
    }
}

/* Return true if this member function is a constructor, false
   otherwise.  */

static int
dwarf2_is_constructor (struct die_info *die, struct dwarf2_cu *cu)
{
  const char *fieldname;
  const char *type_name;
  int len;

  if (die->parent == NULL)
    return 0;

  if (die->parent->tag != DW_TAG_structure_type
      && die->parent->tag != DW_TAG_union_type
      && die->parent->tag != DW_TAG_class_type)
    return 0;

  fieldname = dwarf2_name (die, cu);
  type_name = dwarf2_name (die->parent, cu);
  if (fieldname == NULL || type_name == NULL)
    return 0;

  len = strlen (fieldname);
  return (strncmp (fieldname, type_name, len) == 0
	  && (type_name[len] == '\0' || type_name[len] == '<'));
}

/* Add a member function to the proper fieldlist.  */

static void
dwarf2_add_member_fn (struct field_info *fip, struct die_info *die,
		      struct type *type, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  struct attribute *attr;
  int i;
  struct fnfieldlist *flp = nullptr;
  struct fn_field *fnp;
  const char *fieldname;
  struct type *this_type;

  if (cu->lang () == language_ada)
    error (_("unexpected member function in Ada type"));

  /* Get name of member function.  */
  fieldname = dwarf2_name (die, cu);
  if (fieldname == NULL)
    return;

  /* Look up member function name in fieldlist.  */
  for (i = 0; i < fip->fnfieldlists.size (); i++)
    {
      if (strcmp (fip->fnfieldlists[i].name, fieldname) == 0)
	{
	  flp = &fip->fnfieldlists[i];
	  break;
	}
    }

  /* Create a new fnfieldlist if necessary.  */
  if (flp == nullptr)
    {
      fip->fnfieldlists.emplace_back ();
      flp = &fip->fnfieldlists.back ();
      flp->name = fieldname;
      i = fip->fnfieldlists.size () - 1;
    }

  /* Create a new member function field and add it to the vector of
     fnfieldlists.  */
  flp->fnfields.emplace_back ();
  fnp = &flp->fnfields.back ();

  /* Delay processing of the physname until later.  */
  if (cu->lang () == language_cplus)
    add_to_method_list (type, i, flp->fnfields.size () - 1, fieldname,
			die, cu);
  else
    {
      const char *physname = dwarf2_physname (fieldname, die, cu);
      fnp->physname = physname ? physname : "";
    }

  fnp->type = type_allocator (objfile, cu->lang ()).new_type ();
  this_type = read_type_die (die, cu);
  if (this_type && this_type->code () == TYPE_CODE_FUNC)
    {
      int nparams = this_type->num_fields ();

      /* TYPE is the domain of this method, and THIS_TYPE is the type
	   of the method itself (TYPE_CODE_METHOD).  */
      smash_to_method_type (fnp->type, type,
			    this_type->target_type (),
			    this_type->fields (),
			    this_type->num_fields (),
			    this_type->has_varargs ());

      /* Handle static member functions.
	 Dwarf2 has no clean way to discern C++ static and non-static
	 member functions.  G++ helps GDB by marking the first
	 parameter for non-static member functions (which is the this
	 pointer) as artificial.  We obtain this information from
	 read_subroutine_type via TYPE_FIELD_ARTIFICIAL.  */
      if (nparams == 0 || this_type->field (0).is_artificial () == 0)
	fnp->voffset = VOFFSET_STATIC;
    }
  else
    complaint (_("member function type missing for '%s'"),
	       dwarf2_full_name (fieldname, die, cu));

  /* Get fcontext from DW_AT_containing_type if present.  */
  if (dwarf2_attr (die, DW_AT_containing_type, cu) != NULL)
    fnp->fcontext = die_containing_type (die, cu);

  /* dwarf2 doesn't have stubbed physical names, so the setting of is_const and
     is_volatile is irrelevant, as it is needed by gdb_mangle_name only.  */

  /* Get accessibility.  */
  dwarf_access_attribute accessibility = dwarf2_access_attribute (die, cu);
  switch (accessibility)
    {
    case DW_ACCESS_private:
      fnp->accessibility = accessibility::PRIVATE;
      break;
    case DW_ACCESS_protected:
      fnp->accessibility = accessibility::PROTECTED;
      break;
    }

  /* Check for artificial methods.  */
  attr = dwarf2_attr (die, DW_AT_artificial, cu);
  if (attr && attr->as_boolean ())
    fnp->is_artificial = 1;

  /* Check for defaulted methods.  */
  attr = dwarf2_attr (die, DW_AT_defaulted, cu);
  if (attr != nullptr)
    fnp->defaulted = attr->defaulted ();

  /* Check for deleted methods.  */
  attr = dwarf2_attr (die, DW_AT_deleted, cu);
  if (attr != nullptr && attr->as_boolean ())
    fnp->is_deleted = 1;

  fnp->is_constructor = dwarf2_is_constructor (die, cu);

  /* Get index in virtual function table if it is a virtual member
     function.  For older versions of GCC, this is an offset in the
     appropriate virtual table, as specified by DW_AT_containing_type.
     For everyone else, it is an expression to be evaluated relative
     to the object address.  */

  attr = dwarf2_attr (die, DW_AT_vtable_elem_location, cu);
  if (attr != nullptr)
    {
      if (attr->form_is_block () && attr->as_block ()->size > 0)
	{
	  struct dwarf_block *block = attr->as_block ();
	  CORE_ADDR offset;

	  if (block->data[0] == DW_OP_constu
	      && decode_locdesc (block, cu, &offset))
	    {
	      /* "Old"-style GCC.  See
		 https://gcc.gnu.org/bugzilla/show_bug.cgi?id=44126
		 for discussion.  This was known and a patch available
		 in 2010, but as of 2023, both GCC and clang still
		 emit this.  */
	      fnp->voffset = offset + 2;
	    }
	  else if ((block->data[0] == DW_OP_deref
		    || (block->size > 1
			&& block->data[0] == DW_OP_deref_size
			&& block->data[1] == cu->header.addr_size))
		   && decode_locdesc (block, cu, &offset))
	    {
	      fnp->voffset = offset;
	      if ((fnp->voffset % cu->header.addr_size) != 0)
		dwarf2_complex_location_expr_complaint ();
	      else
		fnp->voffset /= cu->header.addr_size;
	      fnp->voffset += 2;
	    }
	  else
	    dwarf2_complex_location_expr_complaint ();

	  if (!fnp->fcontext)
	    {
	      /* If there is no `this' field and no DW_AT_containing_type,
		 we cannot actually find a base class context for the
		 vtable!  */
	      if (this_type->num_fields () == 0
		  || !this_type->field (0).is_artificial ())
		{
		  complaint (_("cannot determine context for virtual member "
			       "function \"%s\" (offset %s)"),
			     fieldname, sect_offset_str (die->sect_off));
		}
	      else
		{
		  fnp->fcontext = this_type->field (0).type ()->target_type ();
		}
	    }
	}
      else if (attr->form_is_section_offset ())
	{
	  dwarf2_complex_location_expr_complaint ();
	}
      else
	{
	  dwarf2_invalid_attrib_class_complaint ("DW_AT_vtable_elem_location",
						 fieldname);
	}
    }
  else
    {
      attr = dwarf2_attr (die, DW_AT_virtuality, cu);
      if (attr != nullptr && attr->as_virtuality () != DW_VIRTUALITY_none)
	{
	  /* GCC does this, as of 2008-08-25; PR debug/37237.  */
	  complaint (_("Member function \"%s\" (offset %s) is virtual "
		       "but the vtable offset is not specified"),
		     fieldname, sect_offset_str (die->sect_off));
	  ALLOCATE_CPLUS_STRUCT_TYPE (type);
	  TYPE_CPLUS_DYNAMIC (type) = 1;
	}
    }
}

/* Create the vector of member function fields, and attach it to the type.  */

static void
dwarf2_attach_fn_fields_to_type (struct field_info *fip, struct type *type,
				 struct dwarf2_cu *cu)
{
  if (cu->lang () == language_ada)
    error (_("unexpected member functions in Ada type"));

  ALLOCATE_CPLUS_STRUCT_TYPE (type);
  TYPE_FN_FIELDLISTS (type) = (struct fn_fieldlist *)
    TYPE_ZALLOC (type,
		 sizeof (struct fn_fieldlist) * fip->fnfieldlists.size ());

  for (int i = 0; i < fip->fnfieldlists.size (); i++)
    {
      struct fnfieldlist &nf = fip->fnfieldlists[i];
      struct fn_fieldlist *fn_flp = &TYPE_FN_FIELDLIST (type, i);

      TYPE_FN_FIELDLIST_NAME (type, i) = nf.name;
      TYPE_FN_FIELDLIST_LENGTH (type, i) = nf.fnfields.size ();
      /* No need to zero-initialize, initialization is done by the copy in
	 the loop below.  */
      fn_flp->fn_fields = (struct fn_field *)
	TYPE_ALLOC (type, sizeof (struct fn_field) * nf.fnfields.size ());

      for (int k = 0; k < nf.fnfields.size (); ++k)
	fn_flp->fn_fields[k] = nf.fnfields[k];
    }

  TYPE_NFN_FIELDS (type) = fip->fnfieldlists.size ();
}

/* Returns non-zero if NAME is the name of a vtable member in CU's
   language, zero otherwise.  */
static int
is_vtable_name (const char *name, struct dwarf2_cu *cu)
{
  static const char vptr[] = "_vptr";

  /* Look for the C++ form of the vtable.  */
  if (startswith (name, vptr) && is_cplus_marker (name[sizeof (vptr) - 1]))
    return 1;

  return 0;
}

/* GCC outputs unnamed structures that are really pointers to member
   functions, with the ABI-specified layout.  If TYPE describes
   such a structure, smash it into a member function type.

   GCC shouldn't do this; it should just output pointer to member DIEs.
   This is GCC PR debug/28767.  */

static void
quirk_gcc_member_function_pointer (struct type *type, struct objfile *objfile)
{
  struct type *pfn_type, *self_type, *new_type;

  /* Check for a structure with no name and two children.  */
  if (type->code () != TYPE_CODE_STRUCT || type->num_fields () != 2)
    return;

  /* Check for __pfn and __delta members.  */
  if (type->field (0).name () == NULL
      || strcmp (type->field (0).name (), "__pfn") != 0
      || type->field (1).name () == NULL
      || strcmp (type->field (1).name (), "__delta") != 0)
    return;

  /* Find the type of the method.  */
  pfn_type = type->field (0).type ();
  if (pfn_type == NULL
      || pfn_type->code () != TYPE_CODE_PTR
      || pfn_type->target_type ()->code () != TYPE_CODE_FUNC)
    return;

  /* Look for the "this" argument.  */
  pfn_type = pfn_type->target_type ();
  if (pfn_type->num_fields () == 0
      /* || pfn_type->field (0).type () == NULL */
      || pfn_type->field (0).type ()->code () != TYPE_CODE_PTR)
    return;

  self_type = pfn_type->field (0).type ()->target_type ();
  new_type = type_allocator (type).new_type ();
  smash_to_method_type (new_type, self_type, pfn_type->target_type (),
			pfn_type->fields (), pfn_type->num_fields (),
			pfn_type->has_varargs ());
  smash_to_methodptr_type (type, new_type);
}

/* Helper for quirk_ada_thick_pointer.  If TYPE is an array type that
   requires rewriting, then copy it and return the updated copy.
   Otherwise return nullptr.  */

static struct type *
rewrite_array_type (struct type *type)
{
  if (type->code () != TYPE_CODE_ARRAY)
    return nullptr;

  struct type *index_type = type->index_type ();
  range_bounds *current_bounds = index_type->bounds ();

  /* Handle multi-dimensional arrays.  */
  struct type *new_target = rewrite_array_type (type->target_type ());
  if (new_target == nullptr)
    {
      /* Maybe we don't need to rewrite this array.  */
      if (current_bounds->low.is_constant ()
	  && current_bounds->high.is_constant ())
	return nullptr;
    }

  /* Either the target type was rewritten, or the bounds have to be
     updated.  Either way we want to copy the type and update
     everything.  */
  struct type *copy = copy_type (type);
  copy->copy_fields (type);
  if (new_target != nullptr)
    copy->set_target_type (new_target);

  struct type *index_copy = copy_type (index_type);
  range_bounds *bounds
    = (struct range_bounds *) TYPE_ZALLOC (index_copy,
					   sizeof (range_bounds));
  *bounds = *current_bounds;
  bounds->low.set_const_val (1);
  bounds->high.set_const_val (0);
  index_copy->set_bounds (bounds);
  copy->set_index_type (index_copy);

  return copy;
}

/* While some versions of GCC will generate complicated DWARF for an
   array (see quirk_ada_thick_pointer), more recent versions were
   modified to emit an explicit thick pointer structure.  However, in
   this case, the array still has DWARF expressions for its ranges,
   and these must be ignored.  */

static void
quirk_ada_thick_pointer_struct (struct die_info *die, struct dwarf2_cu *cu,
				struct type *type)
{
  gdb_assert (cu->lang () == language_ada);

  /* Check for a structure with two children.  */
  if (type->code () != TYPE_CODE_STRUCT || type->num_fields () != 2)
    return;

  /* Check for P_ARRAY and P_BOUNDS members.  */
  if (type->field (0).name () == NULL
      || strcmp (type->field (0).name (), "P_ARRAY") != 0
      || type->field (1).name () == NULL
      || strcmp (type->field (1).name (), "P_BOUNDS") != 0)
    return;

  /* Make sure we're looking at a pointer to an array.  */
  if (type->field (0).type ()->code () != TYPE_CODE_PTR)
    return;

  /* The Ada code already knows how to handle these types, so all that
     we need to do is turn the bounds into static bounds.  However, we
     don't want to rewrite existing array or index types in-place,
     because those may be referenced in other contexts where this
     rewriting is undesirable.  */
  struct type *new_ary_type
    = rewrite_array_type (type->field (0).type ()->target_type ());
  if (new_ary_type != nullptr)
    type->field (0).set_type (lookup_pointer_type (new_ary_type));
}

/* If the DIE has a DW_AT_alignment attribute, return its value, doing
   appropriate error checking and issuing complaints if there is a
   problem.  */

static ULONGEST
get_alignment (struct dwarf2_cu *cu, struct die_info *die)
{
  struct attribute *attr = dwarf2_attr (die, DW_AT_alignment, cu);

  if (attr == nullptr)
    return 0;

  if (!attr->form_is_constant ())
    {
      complaint (_("DW_AT_alignment must have constant form"
		   " - DIE at %s [in module %s]"),
		 sect_offset_str (die->sect_off),
		 objfile_name (cu->per_objfile->objfile));
      return 0;
    }

  LONGEST val = attr->constant_value (0);
  if (val < 0)
    {
      complaint (_("DW_AT_alignment value must not be negative"
		   " - DIE at %s [in module %s]"),
		 sect_offset_str (die->sect_off),
		 objfile_name (cu->per_objfile->objfile));
      return 0;
    }
  ULONGEST align = val;

  if (align == 0)
    {
      complaint (_("DW_AT_alignment value must not be zero"
		   " - DIE at %s [in module %s]"),
		 sect_offset_str (die->sect_off),
		 objfile_name (cu->per_objfile->objfile));
      return 0;
    }
  if ((align & (align - 1)) != 0)
    {
      complaint (_("DW_AT_alignment value must be a power of 2"
		   " - DIE at %s [in module %s]"),
		 sect_offset_str (die->sect_off),
		 objfile_name (cu->per_objfile->objfile));
      return 0;
    }

  return align;
}

/* If the DIE has a DW_AT_alignment attribute, use its value to set
   the alignment for TYPE.  */

static void
maybe_set_alignment (struct dwarf2_cu *cu, struct die_info *die,
		     struct type *type)
{
  if (!set_type_align (type, get_alignment (cu, die)))
    complaint (_("DW_AT_alignment value too large"
		 " - DIE at %s [in module %s]"),
	       sect_offset_str (die->sect_off),
	       objfile_name (cu->per_objfile->objfile));
}

/* Check if the given VALUE is a valid enum dwarf_calling_convention
   constant for a type, according to DWARF5 spec, Table 5.5.  */

static bool
is_valid_DW_AT_calling_convention_for_type (ULONGEST value)
{
  switch (value)
    {
    case DW_CC_normal:
    case DW_CC_pass_by_reference:
    case DW_CC_pass_by_value:
      return true;

    default:
      complaint (_("unrecognized DW_AT_calling_convention value "
		   "(%s) for a type"), pulongest (value));
      return false;
    }
}

/* Check if the given VALUE is a valid enum dwarf_calling_convention
   constant for a subroutine, according to DWARF5 spec, Table 3.3, and
   also according to GNU-specific values (see include/dwarf2.h).  */

static bool
is_valid_DW_AT_calling_convention_for_subroutine (ULONGEST value)
{
  switch (value)
    {
    case DW_CC_normal:
    case DW_CC_program:
    case DW_CC_nocall:
      return true;

    case DW_CC_GNU_renesas_sh:
    case DW_CC_GNU_borland_fastcall_i386:
    case DW_CC_GDB_IBM_OpenCL:
      return true;

    default:
      complaint (_("unrecognized DW_AT_calling_convention value "
		   "(%s) for a subroutine"), pulongest (value));
      return false;
    }
}

/* Called when we find the DIE that starts a structure or union scope
   (definition) to create a type for the structure or union.  Fill in
   the type's name and general properties; the members will not be
   processed until process_structure_scope.  A symbol table entry for
   the type will also not be done until process_structure_scope (assuming
   the type has a name).

   NOTE: we need to call these functions regardless of whether or not the
   DIE has a DW_AT_name attribute, since it might be an anonymous
   structure or union.  This gets the type entered into our set of
   user defined types.  */

static struct type *
read_structure_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  struct type *type;
  struct attribute *attr;
  const char *name;

  /* If the definition of this type lives in .debug_types, read that type.
     Don't follow DW_AT_specification though, that will take us back up
     the chain and we want to go down.  */
  attr = die->attr (DW_AT_signature);
  if (attr != nullptr)
    {
      type = get_DW_AT_signature_type (die, attr, cu);

      /* The type's CU may not be the same as CU.
	 Ensure TYPE is recorded with CU in die_type_hash.  */
      return set_die_type (die, type, cu);
    }

  type = type_allocator (objfile, cu->lang ()).new_type ();
  INIT_CPLUS_SPECIFIC (type);

  name = dwarf2_name (die, cu);
  if (name != NULL)
    {
      if (cu->lang  () == language_cplus
	  || cu->lang () == language_d
	  || cu->lang () == language_rust)
	{
	  const char *full_name = dwarf2_full_name (name, die, cu);

	  /* dwarf2_full_name might have already finished building the DIE's
	     type.  If so, there is no need to continue.  */
	  if (get_die_type (die, cu) != NULL)
	    return get_die_type (die, cu);

	  type->set_name (full_name);
	}
      else
	{
	  /* The name is already allocated along with this objfile, so
	     we don't need to duplicate it for the type.  */
	  type->set_name (name);
	}
    }

  if (die->tag == DW_TAG_structure_type)
    {
      type->set_code (TYPE_CODE_STRUCT);
    }
  else if (die->tag == DW_TAG_union_type)
    {
      type->set_code (TYPE_CODE_UNION);
    }
  else if (die->tag == DW_TAG_namelist)
    {
      type->set_code (TYPE_CODE_NAMELIST);
    }
  else
    {
      type->set_code (TYPE_CODE_STRUCT);
    }

  if (cu->lang () == language_cplus && die->tag == DW_TAG_class_type)
    type->set_is_declared_class (true);

  /* Store the calling convention in the type if it's available in
     the die.  Otherwise the calling convention remains set to
     the default value DW_CC_normal.  */
  attr = dwarf2_attr (die, DW_AT_calling_convention, cu);
  if (attr != nullptr
      && is_valid_DW_AT_calling_convention_for_type (attr->constant_value (0)))
    {
      ALLOCATE_CPLUS_STRUCT_TYPE (type);
      TYPE_CPLUS_CALLING_CONVENTION (type)
	= (enum dwarf_calling_convention) (attr->constant_value (0));
    }

  attr = dwarf2_attr (die, DW_AT_byte_size, cu);
  if (attr != nullptr)
    {
      if (attr->form_is_constant ())
	type->set_length (attr->constant_value (0));
      else
	{
	  struct dynamic_prop prop;
	  if (attr_to_dynamic_prop (attr, die, cu, &prop, cu->addr_type ()))
	    type->add_dyn_prop (DYN_PROP_BYTE_SIZE, prop);

	  type->set_length (0);
	}
    }
  else
    type->set_length (0);

  maybe_set_alignment (cu, die, type);

  if (producer_is_icc_lt_14 (cu) && (type->length () == 0))
    {
      /* ICC<14 does not output the required DW_AT_declaration on
	 incomplete types, but gives them a size of zero.  */
      type->set_is_stub (true);
    }
  else
    type->set_stub_is_supported (true);

  if (die_is_declaration (die, cu))
    type->set_is_stub (true);
  else if (attr == NULL && die->child == NULL
	   && producer_is_realview (cu->producer))
    /* RealView does not output the required DW_AT_declaration
       on incomplete types.  */
    type->set_is_stub (true);

  /* We need to add the type field to the die immediately so we don't
     infinitely recurse when dealing with pointers to the structure
     type within the structure itself.  */
  set_die_type (die, type, cu);

  /* set_die_type should be already done.  */
  set_descriptive_type (type, die, cu);

  return type;
}

static void handle_struct_member_die
  (struct die_info *child_die,
   struct type *type,
   struct field_info *fi,
   std::vector<struct symbol *> *template_args,
   struct dwarf2_cu *cu);

/* A helper for handle_struct_member_die that handles
   DW_TAG_variant_part.  */

static void
handle_variant_part (struct die_info *die, struct type *type,
		     struct field_info *fi,
		     std::vector<struct symbol *> *template_args,
		     struct dwarf2_cu *cu)
{
  variant_part_builder *new_part;
  if (fi->current_variant_part == nullptr)
    {
      fi->variant_parts.emplace_back ();
      new_part = &fi->variant_parts.back ();
    }
  else if (!fi->current_variant_part->processing_variant)
    {
      complaint (_("nested DW_TAG_variant_part seen "
		   "- DIE at %s [in module %s]"),
		 sect_offset_str (die->sect_off),
		 objfile_name (cu->per_objfile->objfile));
      return;
    }
  else
    {
      variant_field &current = fi->current_variant_part->variants.back ();
      current.variant_parts.emplace_back ();
      new_part = &current.variant_parts.back ();
    }

  /* When we recurse, we want callees to add to this new variant
     part.  */
  scoped_restore save_current_variant_part
    = make_scoped_restore (&fi->current_variant_part, new_part);

  struct attribute *discr = dwarf2_attr (die, DW_AT_discr, cu);
  if (discr == NULL)
    {
      /* It's a univariant form, an extension we support.  */
    }
  else if (discr->form_is_ref ())
    {
      struct dwarf2_cu *target_cu = cu;
      struct die_info *target_die = follow_die_ref (die, discr, &target_cu);

      new_part->discriminant_offset = target_die->sect_off;
    }
  else
    {
      complaint (_("DW_AT_discr does not have DIE reference form"
		   " - DIE at %s [in module %s]"),
		 sect_offset_str (die->sect_off),
		 objfile_name (cu->per_objfile->objfile));
    }

  for (die_info *child_die = die->child;
       child_die != NULL;
       child_die = child_die->sibling)
    handle_struct_member_die (child_die, type, fi, template_args, cu);
}

/* A helper for handle_struct_member_die that handles
   DW_TAG_variant.  */

static void
handle_variant (struct die_info *die, struct type *type,
		struct field_info *fi,
		std::vector<struct symbol *> *template_args,
		struct dwarf2_cu *cu)
{
  if (fi->current_variant_part == nullptr)
    {
      complaint (_("saw DW_TAG_variant outside DW_TAG_variant_part "
		   "- DIE at %s [in module %s]"),
		 sect_offset_str (die->sect_off),
		 objfile_name (cu->per_objfile->objfile));
      return;
    }
  if (fi->current_variant_part->processing_variant)
    {
      complaint (_("nested DW_TAG_variant seen "
		   "- DIE at %s [in module %s]"),
		 sect_offset_str (die->sect_off),
		 objfile_name (cu->per_objfile->objfile));
      return;
    }

  scoped_restore save_processing_variant
    = make_scoped_restore (&fi->current_variant_part->processing_variant,
			   true);

  fi->current_variant_part->variants.emplace_back ();
  variant_field &variant = fi->current_variant_part->variants.back ();
  variant.first_field = fi->fields.size ();

  /* In a variant we want to get the discriminant and also add a
     field for our sole member child.  */
  struct attribute *discr = dwarf2_attr (die, DW_AT_discr_value, cu);
  if (discr == nullptr || !discr->form_is_constant ())
    {
      discr = dwarf2_attr (die, DW_AT_discr_list, cu);
      if (discr == nullptr || discr->as_block ()->size == 0)
	variant.default_branch = true;
      else
	variant.discr_list_data = discr->as_block ();
    }
  else
    variant.discriminant_value = discr->constant_value (0);

  for (die_info *variant_child = die->child;
       variant_child != NULL;
       variant_child = variant_child->sibling)
    handle_struct_member_die (variant_child, type, fi, template_args, cu);

  variant.last_field = fi->fields.size ();
}

/* A helper for process_structure_scope that handles a single member
   DIE.  */

static void
handle_struct_member_die (struct die_info *child_die, struct type *type,
			  struct field_info *fi,
			  std::vector<struct symbol *> *template_args,
			  struct dwarf2_cu *cu)
{
  if (child_die->tag == DW_TAG_member
      || child_die->tag == DW_TAG_variable
      || child_die->tag == DW_TAG_namelist_item)
    {
      /* NOTE: carlton/2002-11-05: A C++ static data member
	 should be a DW_TAG_member that is a declaration, but
	 all versions of G++ as of this writing (so through at
	 least 3.2.1) incorrectly generate DW_TAG_variable
	 tags for them instead.  */
      dwarf2_add_field (fi, child_die, cu);
    }
  else if (child_die->tag == DW_TAG_subprogram)
    {
      /* Rust doesn't have member functions in the C++ sense.
	 However, it does emit ordinary functions as children
	 of a struct DIE.  */
      if (cu->lang () == language_rust)
	read_func_scope (child_die, cu);
      else
	{
	  /* C++ member function.  */
	  dwarf2_add_member_fn (fi, child_die, type, cu);
	}
    }
  else if (child_die->tag == DW_TAG_inheritance)
    {
      /* C++ base class field.  */
      dwarf2_add_field (fi, child_die, cu);
    }
  else if (type_can_define_types (child_die))
    dwarf2_add_type_defn (fi, child_die, cu);
  else if (child_die->tag == DW_TAG_template_type_param
	   || child_die->tag == DW_TAG_template_value_param)
    {
      struct symbol *arg = new_symbol (child_die, NULL, cu);

      if (arg != NULL)
	template_args->push_back (arg);
    }
  else if (child_die->tag == DW_TAG_variant_part)
    handle_variant_part (child_die, type, fi, template_args, cu);
  else if (child_die->tag == DW_TAG_variant)
    handle_variant (child_die, type, fi, template_args, cu);
}

/* Finish creating a structure or union type, including filling in its
   members and creating a symbol for it. This function also handles Fortran
   namelist variables, their items or members and creating a symbol for
   them.  */

static void
process_structure_scope (struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  struct die_info *child_die;
  struct type *type;

  type = get_die_type (die, cu);
  if (type == NULL)
    type = read_structure_type (die, cu);

  bool has_template_parameters = false;
  if (die->child != NULL && ! die_is_declaration (die, cu))
    {
      struct field_info fi;
      std::vector<struct symbol *> template_args;

      child_die = die->child;

      while (child_die && child_die->tag)
	{
	  handle_struct_member_die (child_die, type, &fi, &template_args, cu);
	  child_die = child_die->sibling;
	}

      /* Attach template arguments to type.  */
      if (!template_args.empty ())
	{
	  has_template_parameters = true;
	  ALLOCATE_CPLUS_STRUCT_TYPE (type);
	  TYPE_N_TEMPLATE_ARGUMENTS (type) = template_args.size ();
	  TYPE_TEMPLATE_ARGUMENTS (type)
	    = XOBNEWVEC (&objfile->objfile_obstack,
			 struct symbol *,
			 TYPE_N_TEMPLATE_ARGUMENTS (type));
	  memcpy (TYPE_TEMPLATE_ARGUMENTS (type),
		  template_args.data (),
		  (TYPE_N_TEMPLATE_ARGUMENTS (type)
		   * sizeof (struct symbol *)));
	}

      /* Attach fields and member functions to the type.  */
      if (fi.nfields () > 0)
	dwarf2_attach_fields_to_type (&fi, type, cu);
      if (!fi.fnfieldlists.empty ())
	{
	  dwarf2_attach_fn_fields_to_type (&fi, type, cu);

	  /* Get the type which refers to the base class (possibly this
	     class itself) which contains the vtable pointer for the current
	     class from the DW_AT_containing_type attribute.  This use of
	     DW_AT_containing_type is a GNU extension.  */

	  if (dwarf2_attr (die, DW_AT_containing_type, cu) != NULL)
	    {
	      struct type *t = die_containing_type (die, cu);

	      set_type_vptr_basetype (type, t);
	      if (type == t)
		{
		  int i;

		  /* Our own class provides vtbl ptr.  */
		  for (i = t->num_fields () - 1;
		       i >= TYPE_N_BASECLASSES (t);
		       --i)
		    {
		      const char *fieldname = t->field (i).name ();

		      if (is_vtable_name (fieldname, cu))
			{
			  set_type_vptr_fieldno (type, i);
			  break;
			}
		    }

		  /* Complain if virtual function table field not found.  */
		  if (i < TYPE_N_BASECLASSES (t))
		    complaint (_("virtual function table pointer "
				 "not found when defining class '%s'"),
			       type->name () ? type->name () : "");
		}
	      else
		{
		  set_type_vptr_fieldno (type, TYPE_VPTR_FIELDNO (t));
		}
	    }
	  else if (cu->producer
		   && startswith (cu->producer, "IBM(R) XL C/C++ Advanced Edition"))
	    {
	      /* The IBM XLC compiler does not provide direct indication
		 of the containing type, but the vtable pointer is
		 always named __vfp.  */

	      int i;

	      for (i = type->num_fields () - 1;
		   i >= TYPE_N_BASECLASSES (type);
		   --i)
		{
		  if (strcmp (type->field (i).name (), "__vfp") == 0)
		    {
		      set_type_vptr_fieldno (type, i);
		      set_type_vptr_basetype (type, type);
		      break;
		    }
		}
	    }
	}

      /* Copy fi.typedef_field_list linked list elements content into the
	 allocated array TYPE_TYPEDEF_FIELD_ARRAY (type).  */
      if (!fi.typedef_field_list.empty ())
	{
	  int count = fi.typedef_field_list.size ();

	  ALLOCATE_CPLUS_STRUCT_TYPE (type);
	  /* No zero-initialization is needed, the elements are initialized by
	     the copy in the loop below.  */
	  TYPE_TYPEDEF_FIELD_ARRAY (type)
	    = ((struct decl_field *)
	       TYPE_ALLOC (type,
			   sizeof (TYPE_TYPEDEF_FIELD (type, 0)) * count));
	  TYPE_TYPEDEF_FIELD_COUNT (type) = count;

	  for (int i = 0; i < fi.typedef_field_list.size (); ++i)
	    TYPE_TYPEDEF_FIELD (type, i) = fi.typedef_field_list[i];
	}

      /* Copy fi.nested_types_list linked list elements content into the
	 allocated array TYPE_NESTED_TYPES_ARRAY (type).  */
      if (!fi.nested_types_list.empty ()
	  && cu->lang () != language_ada)
	{
	  int count = fi.nested_types_list.size ();

	  ALLOCATE_CPLUS_STRUCT_TYPE (type);
	  /* No zero-initialization is needed, the elements are initialized by
	     the copy in the loop below.  */
	  TYPE_NESTED_TYPES_ARRAY (type)
	    = ((struct decl_field *)
	       TYPE_ALLOC (type, sizeof (struct decl_field) * count));
	  TYPE_NESTED_TYPES_COUNT (type) = count;

	  for (int i = 0; i < fi.nested_types_list.size (); ++i)
	    TYPE_NESTED_TYPES_FIELD (type, i) = fi.nested_types_list[i];
	}
    }

  quirk_gcc_member_function_pointer (type, objfile);
  if (cu->lang () == language_rust && die->tag == DW_TAG_union_type)
    cu->rust_unions.push_back (type);
  else if (cu->lang () == language_ada)
    quirk_ada_thick_pointer_struct (die, cu, type);

  /* NOTE: carlton/2004-03-16: GCC 3.4 (or at least one of its
     snapshots) has been known to create a die giving a declaration
     for a class that has, as a child, a die giving a definition for a
     nested class.  So we have to process our children even if the
     current die is a declaration.  Normally, of course, a declaration
     won't have any children at all.  */

  child_die = die->child;

  while (child_die != NULL && child_die->tag)
    {
      if (child_die->tag == DW_TAG_member
	  || child_die->tag == DW_TAG_variable
	  || child_die->tag == DW_TAG_inheritance
	  || child_die->tag == DW_TAG_template_value_param
	  || child_die->tag == DW_TAG_template_type_param)
	{
	  /* Do nothing.  */
	}
      else
	process_die (child_die, cu);

      child_die = child_die->sibling;
    }

  /* Do not consider external references.  According to the DWARF standard,
     these DIEs are identified by the fact that they have no byte_size
     attribute, and a declaration attribute.  */
  if (dwarf2_attr (die, DW_AT_byte_size, cu) != NULL
      || !die_is_declaration (die, cu)
      || dwarf2_attr (die, DW_AT_signature, cu) != NULL)
    {
      struct symbol *sym = new_symbol (die, type, cu);

      if (has_template_parameters)
	{
	  struct symtab *symtab;
	  if (sym != nullptr)
	    symtab = sym->symtab ();
	  else if (cu->line_header != nullptr)
	    {
	      /* Any related symtab will do.  */
	      symtab
		= cu->line_header->file_names ()[0].symtab;
	    }
	  else
	    {
	      symtab = nullptr;
	      complaint (_("could not find suitable "
			   "symtab for template parameter"
			   " - DIE at %s [in module %s]"),
			 sect_offset_str (die->sect_off),
			 objfile_name (objfile));
	    }

	  if (symtab != nullptr)
	    {
	      /* Make sure that the symtab is set on the new symbols.
		 Even though they don't appear in this symtab directly,
		 other parts of gdb assume that symbols do, and this is
		 reasonably true.  */
	      for (int i = 0; i < TYPE_N_TEMPLATE_ARGUMENTS (type); ++i)
		TYPE_TEMPLATE_ARGUMENT (type, i)->set_symtab (symtab);
	    }
	}
    }
}

/* Assuming DIE is an enumeration type, and TYPE is its associated
   type, update TYPE using some information only available in DIE's
   children.  In particular, the fields are computed.  */

static void
update_enumeration_type_from_children (struct die_info *die,
				       struct type *type,
				       struct dwarf2_cu *cu)
{
  struct die_info *child_die;
  int unsigned_enum = 1;
  int flag_enum = 1;

  auto_obstack obstack;
  std::vector<struct field> fields;

  for (child_die = die->child;
       child_die != NULL && child_die->tag;
       child_die = child_die->sibling)
    {
      struct attribute *attr;
      LONGEST value;
      const gdb_byte *bytes;
      struct dwarf2_locexpr_baton *baton;
      const char *name;

      if (child_die->tag != DW_TAG_enumerator)
	continue;

      attr = dwarf2_attr (child_die, DW_AT_const_value, cu);
      if (attr == NULL)
	continue;

      name = dwarf2_name (child_die, cu);
      if (name == NULL)
	name = "<anonymous enumerator>";

      dwarf2_const_value_attr (attr, type, name, &obstack, cu,
			       &value, &bytes, &baton);
      if (value < 0)
	{
	  unsigned_enum = 0;
	  flag_enum = 0;
	}
      else
	{
	  if (count_one_bits_ll (value) >= 2)
	    flag_enum = 0;
	}

      fields.emplace_back ();
      struct field &field = fields.back ();
      field.set_name (dwarf2_physname (name, child_die, cu));
      field.set_loc_enumval (value);
    }

  if (!fields.empty ())
    type->copy_fields (fields);
  else
    flag_enum = 0;

  if (unsigned_enum)
    type->set_is_unsigned (true);

  if (flag_enum)
    type->set_is_flag_enum (true);
}

/* Given a DW_AT_enumeration_type die, set its type.  We do not
   complete the type's fields yet, or create any symbols.  */

static struct type *
read_enumeration_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  struct type *type;
  struct attribute *attr;
  const char *name;

  /* If the definition of this type lives in .debug_types, read that type.
     Don't follow DW_AT_specification though, that will take us back up
     the chain and we want to go down.  */
  attr = die->attr (DW_AT_signature);
  if (attr != nullptr)
    {
      type = get_DW_AT_signature_type (die, attr, cu);

      /* The type's CU may not be the same as CU.
	 Ensure TYPE is recorded with CU in die_type_hash.  */
      return set_die_type (die, type, cu);
    }

  type = type_allocator (objfile, cu->lang ()).new_type ();

  type->set_code (TYPE_CODE_ENUM);
  name = dwarf2_full_name (NULL, die, cu);
  if (name != NULL)
    type->set_name (name);

  attr = dwarf2_attr (die, DW_AT_type, cu);
  if (attr != NULL)
    {
      struct type *underlying_type = die_type (die, cu);

      type->set_target_type (underlying_type);
    }

  attr = dwarf2_attr (die, DW_AT_byte_size, cu);
  if (attr != nullptr)
    type->set_length (attr->constant_value (0));
  else
    type->set_length (0);

  maybe_set_alignment (cu, die, type);

  /* The enumeration DIE can be incomplete.  In Ada, any type can be
     declared as private in the package spec, and then defined only
     inside the package body.  Such types are known as Taft Amendment
     Types.  When another package uses such a type, an incomplete DIE
     may be generated by the compiler.  */
  if (die_is_declaration (die, cu))
    type->set_is_stub (true);

  /* If this type has an underlying type that is not a stub, then we
     may use its attributes.  We always use the "unsigned" attribute
     in this situation, because ordinarily we guess whether the type
     is unsigned -- but the guess can be wrong and the underlying type
     can tell us the reality.  However, we defer to a local size
     attribute if one exists, because this lets the compiler override
     the underlying type if needed.  */
  if (type->target_type () != NULL && !type->target_type ()->is_stub ())
    {
      struct type *underlying_type = type->target_type ();
      underlying_type = check_typedef (underlying_type);

      type->set_is_unsigned (underlying_type->is_unsigned ());

      if (type->length () == 0)
	type->set_length (underlying_type->length ());

      if (TYPE_RAW_ALIGN (type) == 0
	  && TYPE_RAW_ALIGN (underlying_type) != 0)
	set_type_align (type, TYPE_RAW_ALIGN (underlying_type));
    }

  type->set_is_declared_class (dwarf2_flag_true_p (die, DW_AT_enum_class, cu));

  set_die_type (die, type, cu);

  /* Finish the creation of this type by using the enum's children.
     Note that, as usual, this must come after set_die_type to avoid
     infinite recursion when trying to compute the names of the
     enumerators.  */
  update_enumeration_type_from_children (die, type, cu);

  return type;
}

/* Given a pointer to a die which begins an enumeration, process all
   the dies that define the members of the enumeration, and create the
   symbol for the enumeration type.

   NOTE: We reverse the order of the element list.  */

static void
process_enumeration_scope (struct die_info *die, struct dwarf2_cu *cu)
{
  struct type *this_type;

  this_type = get_die_type (die, cu);
  if (this_type == NULL)
    this_type = read_enumeration_type (die, cu);

  if (die->child != NULL)
    {
      struct die_info *child_die;
      const char *name;

      child_die = die->child;
      while (child_die && child_die->tag)
	{
	  if (child_die->tag != DW_TAG_enumerator)
	    {
	      process_die (child_die, cu);
	    }
	  else
	    {
	      name = dwarf2_name (child_die, cu);
	      if (name)
		new_symbol (child_die, this_type, cu);
	    }

	  child_die = child_die->sibling;
	}
    }

  /* If we are reading an enum from a .debug_types unit, and the enum
     is a declaration, and the enum is not the signatured type in the
     unit, then we do not want to add a symbol for it.  Adding a
     symbol would in some cases obscure the true definition of the
     enum, giving users an incomplete type when the definition is
     actually available.  Note that we do not want to do this for all
     enums which are just declarations, because C++0x allows forward
     enum declarations.  */
  if (cu->per_cu->is_debug_types
      && die_is_declaration (die, cu))
    {
      struct signatured_type *sig_type;

      sig_type = (struct signatured_type *) cu->per_cu;
      gdb_assert (to_underlying (sig_type->type_offset_in_section) != 0);
      if (sig_type->type_offset_in_section != die->sect_off)
	return;
    }

  new_symbol (die, this_type, cu);
}

/* Helper function for quirk_ada_thick_pointer that examines a bounds
   expression for an index type and finds the corresponding field
   offset in the hidden "P_BOUNDS" structure.  Returns true on success
   and updates *FIELD, false if it fails to recognize an
   expression.  */

static bool
recognize_bound_expression (struct die_info *die, enum dwarf_attribute name,
			    int *bounds_offset, struct field *field,
			    struct dwarf2_cu *cu)
{
  struct attribute *attr = dwarf2_attr (die, name, cu);
  if (attr == nullptr || !attr->form_is_block ())
    return false;

  const struct dwarf_block *block = attr->as_block ();
  const gdb_byte *start = block->data;
  const gdb_byte *end = block->data + block->size;

  /* The expression to recognize generally looks like:

     (DW_OP_push_object_address; DW_OP_plus_uconst: 8; DW_OP_deref;
     DW_OP_plus_uconst: 4; DW_OP_deref_size: 4)

     However, the second "plus_uconst" may be missing:

     (DW_OP_push_object_address; DW_OP_plus_uconst: 8; DW_OP_deref;
     DW_OP_deref_size: 4)

     This happens when the field is at the start of the structure.

     Also, the final deref may not be sized:

     (DW_OP_push_object_address; DW_OP_plus_uconst: 4; DW_OP_deref;
     DW_OP_deref)

     This happens when the size of the index type happens to be the
     same as the architecture's word size.  This can occur with or
     without the second plus_uconst.  */

  if (end - start < 2)
    return false;
  if (*start++ != DW_OP_push_object_address)
    return false;
  if (*start++ != DW_OP_plus_uconst)
    return false;

  uint64_t this_bound_off;
  start = gdb_read_uleb128 (start, end, &this_bound_off);
  if (start == nullptr || (int) this_bound_off != this_bound_off)
    return false;
  /* Update *BOUNDS_OFFSET if needed, or alternatively verify that it
     is consistent among all bounds.  */
  if (*bounds_offset == -1)
    *bounds_offset = this_bound_off;
  else if (*bounds_offset != this_bound_off)
    return false;

  if (start == end || *start++ != DW_OP_deref)
    return false;

  int offset = 0;
  if (start ==end)
    return false;
  else if (*start == DW_OP_deref_size || *start == DW_OP_deref)
    {
      /* This means an offset of 0.  */
    }
  else if (*start++ != DW_OP_plus_uconst)
    return false;
  else
    {
      /* The size is the parameter to DW_OP_plus_uconst.  */
      uint64_t val;
      start = gdb_read_uleb128 (start, end, &val);
      if (start == nullptr)
	return false;
      if ((int) val != val)
	return false;
      offset = val;
    }

  if (start == end)
    return false;

  uint64_t size;
  if (*start == DW_OP_deref_size)
    {
      start = gdb_read_uleb128 (start + 1, end, &size);
      if (start == nullptr)
	return false;
    }
  else if (*start == DW_OP_deref)
    {
      size = cu->header.addr_size;
      ++start;
    }
  else
    return false;

  field->set_loc_bitpos (8 * offset);
  if (size != field->type ()->length ())
    field->set_bitsize (8 * size);

  return true;
}

/* With -fgnat-encodings=minimal, gcc will emit some unusual DWARF for
   some kinds of Ada arrays:

   <1><11db>: Abbrev Number: 7 (DW_TAG_array_type)
      <11dc>   DW_AT_name        : (indirect string, offset: 0x1bb8): string
      <11e0>   DW_AT_data_location: 2 byte block: 97 6
	  (DW_OP_push_object_address; DW_OP_deref)
      <11e3>   DW_AT_type        : <0x1173>
      <11e7>   DW_AT_sibling     : <0x1201>
   <2><11eb>: Abbrev Number: 8 (DW_TAG_subrange_type)
      <11ec>   DW_AT_type        : <0x1206>
      <11f0>   DW_AT_lower_bound : 6 byte block: 97 23 8 6 94 4
	  (DW_OP_push_object_address; DW_OP_plus_uconst: 8; DW_OP_deref;
	   DW_OP_deref_size: 4)
      <11f7>   DW_AT_upper_bound : 8 byte block: 97 23 8 6 23 4 94 4
	  (DW_OP_push_object_address; DW_OP_plus_uconst: 8; DW_OP_deref;
	   DW_OP_plus_uconst: 4; DW_OP_deref_size: 4)

   This actually represents a "thick pointer", which is a structure
   with two elements: one that is a pointer to the array data, and one
   that is a pointer to another structure; this second structure holds
   the array bounds.

   This returns a new type on success, or nullptr if this didn't
   recognize the type.  */

static struct type *
quirk_ada_thick_pointer (struct die_info *die, struct dwarf2_cu *cu,
			 struct type *type)
{
  struct attribute *attr = dwarf2_attr (die, DW_AT_data_location, cu);
  /* So far we've only seen this with block form.  */
  if (attr == nullptr || !attr->form_is_block ())
    return nullptr;

  /* Note that this will fail if the structure layout is changed by
     the compiler.  However, we have no good way to recognize some
     other layout, because we don't know what expression the compiler
     might choose to emit should this happen.  */
  struct dwarf_block *blk = attr->as_block ();
  if (blk->size != 2
      || blk->data[0] != DW_OP_push_object_address
      || blk->data[1] != DW_OP_deref)
    return nullptr;

  int bounds_offset = -1;
  int max_align = -1;
  std::vector<struct field> range_fields;
  for (struct die_info *child_die = die->child;
       child_die;
       child_die = child_die->sibling)
    {
      if (child_die->tag == DW_TAG_subrange_type)
	{
	  struct type *underlying = read_subrange_index_type (child_die, cu);

	  int this_align = type_align (underlying);
	  if (this_align > max_align)
	    max_align = this_align;

	  range_fields.emplace_back ();
	  range_fields.emplace_back ();

	  struct field &lower = range_fields[range_fields.size () - 2];
	  struct field &upper = range_fields[range_fields.size () - 1];

	  lower.set_type (underlying);
	  lower.set_is_artificial (true);

	  upper.set_type (underlying);
	  upper.set_is_artificial (true);

	  if (!recognize_bound_expression (child_die, DW_AT_lower_bound,
					   &bounds_offset, &lower, cu)
	      || !recognize_bound_expression (child_die, DW_AT_upper_bound,
					      &bounds_offset, &upper, cu))
	    return nullptr;
	}
    }

  /* This shouldn't really happen, but double-check that we found
     where the bounds are stored.  */
  if (bounds_offset == -1)
    return nullptr;

  struct objfile *objfile = cu->per_objfile->objfile;
  for (int i = 0; i < range_fields.size (); i += 2)
    {
      char name[20];

      /* Set the name of each field in the bounds.  */
      xsnprintf (name, sizeof (name), "LB%d", i / 2);
      range_fields[i].set_name (objfile->intern (name));
      xsnprintf (name, sizeof (name), "UB%d", i / 2);
      range_fields[i + 1].set_name (objfile->intern (name));
    }

  type_allocator alloc (objfile, cu->lang ());
  struct type *bounds = alloc.new_type ();
  bounds->set_code (TYPE_CODE_STRUCT);

  bounds->copy_fields (range_fields);

  int last_fieldno = range_fields.size () - 1;
  int bounds_size = (bounds->field (last_fieldno).loc_bitpos () / 8
		     + bounds->field (last_fieldno).type ()->length ());
  bounds->set_length (align_up (bounds_size, max_align));

  /* Rewrite the existing array type in place.  Specifically, we
     remove any dynamic properties we might have read, and we replace
     the index types.  */
  struct type *iter = type;
  for (int i = 0; i < range_fields.size (); i += 2)
    {
      gdb_assert (iter->code () == TYPE_CODE_ARRAY);
      iter->main_type->dyn_prop_list = nullptr;
      iter->set_index_type
	(create_static_range_type (alloc, bounds->field (i).type (), 1, 0));
      iter = iter->target_type ();
    }

  struct type *result = type_allocator (objfile, cu->lang ()).new_type ();
  result->set_code (TYPE_CODE_STRUCT);

  result->alloc_fields (2);

  /* The names are chosen to coincide with what the compiler does with
     -fgnat-encodings=all, which the Ada code in gdb already
     understands.  */
  result->field (0).set_name ("P_ARRAY");
  result->field (0).set_type (lookup_pointer_type (type));

  result->field (1).set_name ("P_BOUNDS");
  result->field (1).set_type (lookup_pointer_type (bounds));
  result->field (1).set_loc_bitpos (8 * bounds_offset);

  result->set_name (type->name ());
  result->set_length (result->field (0).type ()->length ()
		      + result->field (1).type ()->length ());

  return result;
}

/* Extract all information from a DW_TAG_array_type DIE and put it in
   the DIE's type field.  For now, this only handles one dimensional
   arrays.  */

static struct type *
read_array_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  struct die_info *child_die;
  struct type *type;
  struct type *element_type, *range_type, *index_type;
  struct attribute *attr;
  const char *name;
  struct dynamic_prop *byte_stride_prop = NULL;
  unsigned int bit_stride = 0;

  element_type = die_type (die, cu);

  /* The die_type call above may have already set the type for this DIE.  */
  type = get_die_type (die, cu);
  if (type)
    return type;

  attr = dwarf2_attr (die, DW_AT_byte_stride, cu);
  if (attr != NULL)
    {
      int stride_ok;
      struct type *prop_type = cu->addr_sized_int_type (false);

      byte_stride_prop
	= (struct dynamic_prop *) alloca (sizeof (struct dynamic_prop));
      stride_ok = attr_to_dynamic_prop (attr, die, cu, byte_stride_prop,
					prop_type);
      if (!stride_ok)
	{
	  complaint (_("unable to read array DW_AT_byte_stride "
		       " - DIE at %s [in module %s]"),
		     sect_offset_str (die->sect_off),
		     objfile_name (cu->per_objfile->objfile));
	  /* Ignore this attribute.  We will likely not be able to print
	     arrays of this type correctly, but there is little we can do
	     to help if we cannot read the attribute's value.  */
	  byte_stride_prop = NULL;
	}
    }

  attr = dwarf2_attr (die, DW_AT_bit_stride, cu);
  if (attr != NULL)
    bit_stride = attr->constant_value (0);

  /* Irix 6.2 native cc creates array types without children for
     arrays with unspecified length.  */
  if (die->child == NULL)
    {
      index_type = builtin_type (objfile)->builtin_int;
      type_allocator alloc (objfile, cu->lang ());
      range_type = create_static_range_type (alloc, index_type, 0, -1);
      type = create_array_type_with_stride (alloc, element_type, range_type,
					    byte_stride_prop, bit_stride);
      return set_die_type (die, type, cu);
    }

  std::vector<struct type *> range_types;
  child_die = die->child;
  while (child_die && child_die->tag)
    {
      if (child_die->tag == DW_TAG_subrange_type
	  || child_die->tag == DW_TAG_generic_subrange)
	{
	  struct type *child_type = read_type_die (child_die, cu);

	  if (child_type != NULL)
	    {
	      /* The range type was successfully read.  Save it for the
		 array type creation.  */
	      range_types.push_back (child_type);
	    }
	}
      child_die = child_die->sibling;
    }

  if (range_types.empty ())
    {
      complaint (_("unable to find array range - DIE at %s [in module %s]"),
		 sect_offset_str (die->sect_off),
		 objfile_name (cu->per_objfile->objfile));
      return NULL;
    }

  /* Dwarf2 dimensions are output from left to right, create the
     necessary array types in backwards order.  */

  type = element_type;

  type_allocator alloc (cu->per_objfile->objfile, cu->lang ());
  if (read_array_order (die, cu) == DW_ORD_col_major)
    {
      int i = 0;

      while (i < range_types.size ())
	{
	  type = create_array_type_with_stride (alloc, type, range_types[i++],
						byte_stride_prop, bit_stride);
	  type->set_is_multi_dimensional (true);
	  bit_stride = 0;
	  byte_stride_prop = nullptr;
	}
    }
  else
    {
      size_t ndim = range_types.size ();
      while (ndim-- > 0)
	{
	  type = create_array_type_with_stride (alloc, type, range_types[ndim],
						byte_stride_prop, bit_stride);
	  type->set_is_multi_dimensional (true);
	  bit_stride = 0;
	  byte_stride_prop = nullptr;
	}
    }

  /* Clear the flag on the outermost array type.  */
  type->set_is_multi_dimensional (false);
  gdb_assert (type != element_type);

  /* Understand Dwarf2 support for vector types (like they occur on
     the PowerPC w/ AltiVec).  Gcc just adds another attribute to the
     array type.  This is not part of the Dwarf2/3 standard yet, but a
     custom vendor extension.  The main difference between a regular
     array and the vector variant is that vectors are passed by value
     to functions.  */
  attr = dwarf2_attr (die, DW_AT_GNU_vector, cu);
  if (attr != nullptr)
    make_vector_type (type);

  /* The DIE may have DW_AT_byte_size set.  For example an OpenCL
     implementation may choose to implement triple vectors using this
     attribute.  */
  attr = dwarf2_attr (die, DW_AT_byte_size, cu);
  if (attr != nullptr && attr->form_is_unsigned ())
    {
      if (attr->as_unsigned () >= type->length ())
	type->set_length (attr->as_unsigned ());
      else
	complaint (_("DW_AT_byte_size for array type smaller "
		     "than the total size of elements"));
    }

  name = dwarf2_name (die, cu);
  if (name)
    type->set_name (name);

  maybe_set_alignment (cu, die, type);

  struct type *replacement_type = nullptr;
  if (cu->lang () == language_ada)
    {
      replacement_type = quirk_ada_thick_pointer (die, cu, type);
      if (replacement_type != nullptr)
	type = replacement_type;
    }

  /* Install the type in the die.  */
  set_die_type (die, type, cu, replacement_type != nullptr);

  /* set_die_type should be already done.  */
  set_descriptive_type (type, die, cu);

  return type;
}

static enum dwarf_array_dim_ordering
read_array_order (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *attr;

  attr = dwarf2_attr (die, DW_AT_ordering, cu);

  if (attr != nullptr)
    {
      LONGEST val = attr->constant_value (-1);
      if (val == DW_ORD_row_major || val == DW_ORD_col_major)
	return (enum dwarf_array_dim_ordering) val;
    }

  /* GNU F77 is a special case, as at 08/2004 array type info is the
     opposite order to the dwarf2 specification, but data is still
     laid out as per normal fortran.

     FIXME: dsl/2004-8-20: If G77 is ever fixed, this will also need
     version checking.  */

  if (cu->lang () == language_fortran
      && cu->producer && strstr (cu->producer, "GNU F77"))
    {
      return DW_ORD_row_major;
    }

  switch (cu->language_defn->array_ordering ())
    {
    case array_column_major:
      return DW_ORD_col_major;
    case array_row_major:
    default:
      return DW_ORD_row_major;
    };
}

/* Extract all information from a DW_TAG_set_type DIE and put it in
   the DIE's type field.  */

static struct type *
read_set_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct type *domain_type, *set_type;
  struct attribute *attr;

  domain_type = die_type (die, cu);

  /* The die_type call above may have already set the type for this DIE.  */
  set_type = get_die_type (die, cu);
  if (set_type)
    return set_type;

  type_allocator alloc (cu->per_objfile->objfile, cu->lang ());
  set_type = create_set_type (alloc, domain_type);

  attr = dwarf2_attr (die, DW_AT_byte_size, cu);
  if (attr != nullptr && attr->form_is_unsigned ())
    set_type->set_length (attr->as_unsigned ());

  maybe_set_alignment (cu, die, set_type);

  return set_die_type (die, set_type, cu);
}

/* A helper for read_common_block that creates a locexpr baton.
   SYM is the symbol which we are marking as computed.
   COMMON_DIE is the DIE for the common block.
   COMMON_LOC is the location expression attribute for the common
   block itself.
   MEMBER_LOC is the location expression attribute for the particular
   member of the common block that we are processing.
   CU is the CU from which the above come.  */

static void
mark_common_block_symbol_computed (struct symbol *sym,
				   struct die_info *common_die,
				   struct attribute *common_loc,
				   struct attribute *member_loc,
				   struct dwarf2_cu *cu)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  struct dwarf2_locexpr_baton *baton;
  gdb_byte *ptr;
  unsigned int cu_off;
  enum bfd_endian byte_order = gdbarch_byte_order (objfile->arch ());
  LONGEST offset = 0;

  gdb_assert (common_loc && member_loc);
  gdb_assert (common_loc->form_is_block ());
  gdb_assert (member_loc->form_is_block ()
	      || member_loc->form_is_constant ());

  baton = XOBNEW (&objfile->objfile_obstack, struct dwarf2_locexpr_baton);
  baton->per_objfile = per_objfile;
  baton->per_cu = cu->per_cu;
  gdb_assert (baton->per_cu);

  baton->size = 5 /* DW_OP_call4 */ + 1 /* DW_OP_plus */;

  if (member_loc->form_is_constant ())
    {
      offset = member_loc->constant_value (0);
      baton->size += 1 /* DW_OP_addr */ + cu->header.addr_size;
    }
  else
    baton->size += member_loc->as_block ()->size;

  ptr = (gdb_byte *) obstack_alloc (&objfile->objfile_obstack, baton->size);
  baton->data = ptr;

  *ptr++ = DW_OP_call4;
  cu_off = common_die->sect_off - cu->per_cu->sect_off;
  store_unsigned_integer (ptr, 4, byte_order, cu_off);
  ptr += 4;

  if (member_loc->form_is_constant ())
    {
      *ptr++ = DW_OP_addr;
      store_unsigned_integer (ptr, cu->header.addr_size, byte_order, offset);
      ptr += cu->header.addr_size;
    }
  else
    {
      /* We have to copy the data here, because DW_OP_call4 will only
	 use a DW_AT_location attribute.  */
      struct dwarf_block *block = member_loc->as_block ();
      memcpy (ptr, block->data, block->size);
      ptr += block->size;
    }

  *ptr++ = DW_OP_plus;
  gdb_assert (ptr - baton->data == baton->size);

  SYMBOL_LOCATION_BATON (sym) = baton;
  sym->set_aclass_index (dwarf2_locexpr_index);
}

/* Create appropriate locally-scoped variables for all the
   DW_TAG_common_block entries.  Also create a struct common_block
   listing all such variables for `info common'.  COMMON_BLOCK_DOMAIN
   is used to separate the common blocks name namespace from regular
   variable names.  */

static void
read_common_block (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *attr;

  attr = dwarf2_attr (die, DW_AT_location, cu);
  if (attr != nullptr)
    {
      /* Support the .debug_loc offsets.  */
      if (attr->form_is_block ())
	{
	  /* Ok.  */
	}
      else if (attr->form_is_section_offset ())
	{
	  dwarf2_complex_location_expr_complaint ();
	  attr = NULL;
	}
      else
	{
	  dwarf2_invalid_attrib_class_complaint ("DW_AT_location",
						 "common block member");
	  attr = NULL;
	}
    }

  if (die->child != NULL)
    {
      struct objfile *objfile = cu->per_objfile->objfile;
      struct die_info *child_die;
      size_t n_entries = 0, size;
      struct common_block *common_block;
      struct symbol *sym;

      for (child_die = die->child;
	   child_die && child_die->tag;
	   child_die = child_die->sibling)
	++n_entries;

      size = (sizeof (struct common_block)
	      + (n_entries - 1) * sizeof (struct symbol *));
      common_block
	= (struct common_block *) obstack_alloc (&objfile->objfile_obstack,
						 size);
      memset (common_block->contents, 0, n_entries * sizeof (struct symbol *));
      common_block->n_entries = 0;

      for (child_die = die->child;
	   child_die && child_die->tag;
	   child_die = child_die->sibling)
	{
	  /* Create the symbol in the DW_TAG_common_block block in the current
	     symbol scope.  */
	  sym = new_symbol (child_die, NULL, cu);
	  if (sym != NULL)
	    {
	      struct attribute *member_loc;

	      common_block->contents[common_block->n_entries++] = sym;

	      member_loc = dwarf2_attr (child_die, DW_AT_data_member_location,
					cu);
	      if (member_loc)
		{
		  /* GDB has handled this for a long time, but it is
		     not specified by DWARF.  It seems to have been
		     emitted by gfortran at least as recently as:
		     http://gcc.gnu.org/bugzilla/show_bug.cgi?id=23057.  */
		  complaint (_("Variable in common block has "
			       "DW_AT_data_member_location "
			       "- DIE at %s [in module %s]"),
			       sect_offset_str (child_die->sect_off),
			     objfile_name (objfile));

		  if (member_loc->form_is_section_offset ())
		    dwarf2_complex_location_expr_complaint ();
		  else if (member_loc->form_is_constant ()
			   || member_loc->form_is_block ())
		    {
		      if (attr != nullptr)
			mark_common_block_symbol_computed (sym, die, attr,
							   member_loc, cu);
		    }
		  else
		    dwarf2_complex_location_expr_complaint ();
		}
	    }
	}

      sym = new_symbol (die, builtin_type (objfile)->builtin_void, cu);
      sym->set_value_common_block (common_block);
    }
}

/* Create a type for a C++ namespace.  */

static struct type *
read_namespace_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  const char *previous_prefix, *name;
  int is_anonymous;
  struct type *type;

  /* For extensions, reuse the type of the original namespace.  */
  if (dwarf2_attr (die, DW_AT_extension, cu) != NULL)
    {
      struct die_info *ext_die;
      struct dwarf2_cu *ext_cu = cu;

      ext_die = dwarf2_extension (die, &ext_cu);
      type = read_type_die (ext_die, ext_cu);

      /* EXT_CU may not be the same as CU.
	 Ensure TYPE is recorded with CU in die_type_hash.  */
      return set_die_type (die, type, cu);
    }

  name = namespace_name (die, &is_anonymous, cu);

  /* Now build the name of the current namespace.  */

  previous_prefix = determine_prefix (die, cu);
  if (previous_prefix[0] != '\0')
    name = typename_concat (&objfile->objfile_obstack,
			    previous_prefix, name, 0, cu);

  /* Create the type.  */
  type = type_allocator (objfile, cu->lang ()).new_type (TYPE_CODE_NAMESPACE,
							 0, name);

  return set_die_type (die, type, cu);
}

/* Read a namespace scope.  */

static void
read_namespace (struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  int is_anonymous;

  /* Add a symbol associated to this if we haven't seen the namespace
     before.  Also, add a using directive if it's an anonymous
     namespace.  */

  if (dwarf2_attr (die, DW_AT_extension, cu) == NULL)
    {
      struct type *type;

      type = read_type_die (die, cu);
      new_symbol (die, type, cu);

      namespace_name (die, &is_anonymous, cu);
      if (is_anonymous)
	{
	  const char *previous_prefix = determine_prefix (die, cu);

	  std::vector<const char *> excludes;
	  add_using_directive (using_directives (cu),
			       previous_prefix, type->name (), NULL,
			       NULL, excludes,
			       read_decl_line (die, cu),
			       0, &objfile->objfile_obstack);
	}
    }

  if (die->child != NULL)
    {
      struct die_info *child_die = die->child;

      while (child_die && child_die->tag)
	{
	  process_die (child_die, cu);
	  child_die = child_die->sibling;
	}
    }
}

/* Read a Fortran module as type.  This DIE can be only a declaration used for
   imported module.  Still we need that type as local Fortran "use ... only"
   declaration imports depend on the created type in determine_prefix.  */

static struct type *
read_module_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  const char *module_name;
  struct type *type;

  module_name = dwarf2_name (die, cu);
  type = type_allocator (objfile, cu->lang ()).new_type (TYPE_CODE_MODULE,
							 0, module_name);

  return set_die_type (die, type, cu);
}

/* Read a Fortran module.  */

static void
read_module (struct die_info *die, struct dwarf2_cu *cu)
{
  struct die_info *child_die = die->child;
  struct type *type;

  type = read_type_die (die, cu);
  new_symbol (die, type, cu);

  while (child_die && child_die->tag)
    {
      process_die (child_die, cu);
      child_die = child_die->sibling;
    }
}

/* Return the name of the namespace represented by DIE.  Set
   *IS_ANONYMOUS to tell whether or not the namespace is an anonymous
   namespace.  */

static const char *
namespace_name (struct die_info *die, int *is_anonymous, struct dwarf2_cu *cu)
{
  struct die_info *current_die;
  const char *name = NULL;

  /* Loop through the extensions until we find a name.  */

  for (current_die = die;
       current_die != NULL;
       current_die = dwarf2_extension (die, &cu))
    {
      /* We don't use dwarf2_name here so that we can detect the absence
	 of a name -> anonymous namespace.  */
      name = dwarf2_string_attr (die, DW_AT_name, cu);

      if (name != NULL)
	break;
    }

  /* Is it an anonymous namespace?  */

  *is_anonymous = (name == NULL);
  if (*is_anonymous)
    name = CP_ANONYMOUS_NAMESPACE_STR;

  return name;
}

/* Extract all information from a DW_TAG_pointer_type DIE and add to
   the user defined type vector.  */

static struct type *
read_tag_pointer_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct gdbarch *gdbarch = cu->per_objfile->objfile->arch ();
  struct comp_unit_head *cu_header = &cu->header;
  struct type *type;
  struct attribute *attr_byte_size;
  struct attribute *attr_address_class;
  int byte_size, addr_class;
  struct type *target_type;

  target_type = die_type (die, cu);

  /* The die_type call above may have already set the type for this DIE.  */
  type = get_die_type (die, cu);
  if (type)
    return type;

  type = lookup_pointer_type (target_type);

  attr_byte_size = dwarf2_attr (die, DW_AT_byte_size, cu);
  if (attr_byte_size)
    byte_size = attr_byte_size->constant_value (cu_header->addr_size);
  else
    byte_size = cu_header->addr_size;

  attr_address_class = dwarf2_attr (die, DW_AT_address_class, cu);
  if (attr_address_class)
    addr_class = attr_address_class->constant_value (DW_ADDR_none);
  else
    addr_class = DW_ADDR_none;

  ULONGEST alignment = get_alignment (cu, die);

  /* If the pointer size, alignment, or address class is different
     than the default, create a type variant marked as such and set
     the length accordingly.  */
  if (type->length () != byte_size
      || (alignment != 0 && TYPE_RAW_ALIGN (type) != 0
	  && alignment != TYPE_RAW_ALIGN (type))
      || addr_class != DW_ADDR_none)
    {
      if (gdbarch_address_class_type_flags_p (gdbarch))
	{
	  type_instance_flags type_flags
	    = gdbarch_address_class_type_flags (gdbarch, byte_size,
						addr_class);
	  gdb_assert ((type_flags & ~TYPE_INSTANCE_FLAG_ADDRESS_CLASS_ALL)
		      == 0);
	  type = make_type_with_address_space (type, type_flags);
	}
      else if (type->length () != byte_size)
	{
	  complaint (_("invalid pointer size %d"), byte_size);
	}
      else if (TYPE_RAW_ALIGN (type) != alignment)
	{
	  complaint (_("Invalid DW_AT_alignment"
		       " - DIE at %s [in module %s]"),
		     sect_offset_str (die->sect_off),
		     objfile_name (cu->per_objfile->objfile));
	}
      else
	{
	  /* Should we also complain about unhandled address classes?  */
	}
    }

  type->set_length (byte_size);
  set_type_align (type, alignment);
  return set_die_type (die, type, cu);
}

/* Extract all information from a DW_TAG_ptr_to_member_type DIE and add to
   the user defined type vector.  */

static struct type *
read_tag_ptr_to_member_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct type *type;
  struct type *to_type;
  struct type *domain;

  to_type = die_type (die, cu);
  domain = die_containing_type (die, cu);

  /* The calls above may have already set the type for this DIE.  */
  type = get_die_type (die, cu);
  if (type)
    return type;

  if (check_typedef (to_type)->code () == TYPE_CODE_METHOD)
    type = lookup_methodptr_type (to_type);
  else if (check_typedef (to_type)->code () == TYPE_CODE_FUNC)
    {
      struct type *new_type
	= type_allocator (cu->per_objfile->objfile, cu->lang ()).new_type ();

      smash_to_method_type (new_type, domain, to_type->target_type (),
			    to_type->fields (), to_type->num_fields (),
			    to_type->has_varargs ());
      type = lookup_methodptr_type (new_type);
    }
  else
    type = lookup_memberptr_type (to_type, domain);

  return set_die_type (die, type, cu);
}

/* Extract all information from a DW_TAG_{rvalue_,}reference_type DIE and add to
   the user defined type vector.  */

static struct type *
read_tag_reference_type (struct die_info *die, struct dwarf2_cu *cu,
			  enum type_code refcode)
{
  struct comp_unit_head *cu_header = &cu->header;
  struct type *type, *target_type;
  struct attribute *attr;

  gdb_assert (refcode == TYPE_CODE_REF || refcode == TYPE_CODE_RVALUE_REF);

  target_type = die_type (die, cu);

  /* The die_type call above may have already set the type for this DIE.  */
  type = get_die_type (die, cu);
  if (type)
    return type;

  type = lookup_reference_type (target_type, refcode);
  attr = dwarf2_attr (die, DW_AT_byte_size, cu);
  if (attr != nullptr)
    type->set_length (attr->constant_value (cu_header->addr_size));
  else
    type->set_length (cu_header->addr_size);

  maybe_set_alignment (cu, die, type);
  return set_die_type (die, type, cu);
}

/* Add the given cv-qualifiers to the element type of the array.  GCC
   outputs DWARF type qualifiers that apply to an array, not the
   element type.  But GDB relies on the array element type to carry
   the cv-qualifiers.  This mimics section 6.7.3 of the C99
   specification.  */

static struct type *
add_array_cv_type (struct die_info *die, struct dwarf2_cu *cu,
		   struct type *base_type, int cnst, int voltl)
{
  struct type *el_type, *inner_array;

  base_type = copy_type (base_type);
  inner_array = base_type;

  while (inner_array->target_type ()->code () == TYPE_CODE_ARRAY)
    {
      inner_array->set_target_type (copy_type (inner_array->target_type ()));
      inner_array = inner_array->target_type ();
    }

  el_type = inner_array->target_type ();
  cnst |= TYPE_CONST (el_type);
  voltl |= TYPE_VOLATILE (el_type);
  inner_array->set_target_type (make_cv_type (cnst, voltl, el_type, NULL));

  return set_die_type (die, base_type, cu);
}

static struct type *
read_tag_const_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct type *base_type, *cv_type;

  base_type = die_type (die, cu);

  /* The die_type call above may have already set the type for this DIE.  */
  cv_type = get_die_type (die, cu);
  if (cv_type)
    return cv_type;

  /* In case the const qualifier is applied to an array type, the element type
     is so qualified, not the array type (section 6.7.3 of C99).  */
  if (base_type->code () == TYPE_CODE_ARRAY)
    return add_array_cv_type (die, cu, base_type, 1, 0);

  cv_type = make_cv_type (1, TYPE_VOLATILE (base_type), base_type, 0);
  return set_die_type (die, cv_type, cu);
}

static struct type *
read_tag_volatile_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct type *base_type, *cv_type;

  base_type = die_type (die, cu);

  /* The die_type call above may have already set the type for this DIE.  */
  cv_type = get_die_type (die, cu);
  if (cv_type)
    return cv_type;

  /* In case the volatile qualifier is applied to an array type, the
     element type is so qualified, not the array type (section 6.7.3
     of C99).  */
  if (base_type->code () == TYPE_CODE_ARRAY)
    return add_array_cv_type (die, cu, base_type, 0, 1);

  cv_type = make_cv_type (TYPE_CONST (base_type), 1, base_type, 0);
  return set_die_type (die, cv_type, cu);
}

/* Handle DW_TAG_restrict_type.  */

static struct type *
read_tag_restrict_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct type *base_type, *cv_type;

  base_type = die_type (die, cu);

  /* The die_type call above may have already set the type for this DIE.  */
  cv_type = get_die_type (die, cu);
  if (cv_type)
    return cv_type;

  cv_type = make_restrict_type (base_type);
  return set_die_type (die, cv_type, cu);
}

/* Handle DW_TAG_atomic_type.  */

static struct type *
read_tag_atomic_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct type *base_type, *cv_type;

  base_type = die_type (die, cu);

  /* The die_type call above may have already set the type for this DIE.  */
  cv_type = get_die_type (die, cu);
  if (cv_type)
    return cv_type;

  cv_type = make_atomic_type (base_type);
  return set_die_type (die, cv_type, cu);
}

/* Extract all information from a DW_TAG_string_type DIE and add to
   the user defined type vector.  It isn't really a user defined type,
   but it behaves like one, with other DIE's using an AT_user_def_type
   attribute to reference it.  */

static struct type *
read_tag_string_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  struct gdbarch *gdbarch = objfile->arch ();
  struct type *type, *range_type, *index_type, *char_type;
  struct attribute *attr;
  struct dynamic_prop prop;
  bool length_is_constant = true;
  LONGEST length;

  /* There are a couple of places where bit sizes might be made use of
     when parsing a DW_TAG_string_type, however, no producer that we know
     of make use of these.  Handling bit sizes that are a multiple of the
     byte size is easy enough, but what about other bit sizes?  Lets deal
     with that problem when we have to.  Warn about these attributes being
     unsupported, then parse the type and ignore them like we always
     have.  */
  if (dwarf2_attr (die, DW_AT_bit_size, cu) != nullptr
      || dwarf2_attr (die, DW_AT_string_length_bit_size, cu) != nullptr)
    {
      static bool warning_printed = false;
      if (!warning_printed)
	{
	  warning (_("DW_AT_bit_size and DW_AT_string_length_bit_size not "
		     "currently supported on DW_TAG_string_type."));
	  warning_printed = true;
	}
    }

  attr = dwarf2_attr (die, DW_AT_string_length, cu);
  if (attr != nullptr && !attr->form_is_constant ())
    {
      /* The string length describes the location at which the length of
	 the string can be found.  The size of the length field can be
	 specified with one of the attributes below.  */
      struct type *prop_type;
      struct attribute *len
	= dwarf2_attr (die, DW_AT_string_length_byte_size, cu);
      if (len == nullptr)
	len = dwarf2_attr (die, DW_AT_byte_size, cu);
      if (len != nullptr && len->form_is_constant ())
	{
	  /* Pass 0 as the default as we know this attribute is constant
	     and the default value will not be returned.  */
	  LONGEST sz = len->constant_value (0);
	  prop_type = objfile_int_type (objfile, sz, true);
	}
      else
	{
	  /* If the size is not specified then we assume it is the size of
	     an address on this target.  */
	  prop_type = cu->addr_sized_int_type (true);
	}

      /* Convert the attribute into a dynamic property.  */
      if (!attr_to_dynamic_prop (attr, die, cu, &prop, prop_type))
	length = 1;
      else
	length_is_constant = false;
    }
  else if (attr != nullptr)
    {
      /* This DW_AT_string_length just contains the length with no
	 indirection.  There's no need to create a dynamic property in this
	 case.  Pass 0 for the default value as we know it will not be
	 returned in this case.  */
      length = attr->constant_value (0);
    }
  else if ((attr = dwarf2_attr (die, DW_AT_byte_size, cu)) != nullptr)
    {
      /* We don't currently support non-constant byte sizes for strings.  */
      length = attr->constant_value (1);
    }
  else
    {
      /* Use 1 as a fallback length if we have nothing else.  */
      length = 1;
    }

  index_type = builtin_type (objfile)->builtin_int;
  type_allocator alloc (objfile, cu->lang ());
  if (length_is_constant)
    range_type = create_static_range_type (alloc, index_type, 1, length);
  else
    {
      struct dynamic_prop low_bound;

      low_bound.set_const_val (1);
      range_type = create_range_type (alloc, index_type, &low_bound, &prop, 0);
    }
  char_type = language_string_char_type (cu->language_defn, gdbarch);
  type = create_string_type (alloc, char_type, range_type);

  return set_die_type (die, type, cu);
}

/* Assuming that DIE corresponds to a function, returns nonzero
   if the function is prototyped.  */

static int
prototyped_function_p (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *attr;

  attr = dwarf2_attr (die, DW_AT_prototyped, cu);
  if (attr && attr->as_boolean ())
    return 1;

  /* The DWARF standard implies that the DW_AT_prototyped attribute
     is only meaningful for C, but the concept also extends to other
     languages that allow unprototyped functions (Eg: Objective C).
     For all other languages, assume that functions are always
     prototyped.  */
  if (cu->lang () != language_c
      && cu->lang () != language_objc
      && cu->lang () != language_opencl)
    return 1;

  /* RealView does not emit DW_AT_prototyped.  We can not distinguish
     prototyped and unprototyped functions; default to prototyped,
     since that is more common in modern code (and RealView warns
     about unprototyped functions).  */
  if (producer_is_realview (cu->producer))
    return 1;

  return 0;
}

/* Handle DIES due to C code like:

   struct foo
   {
   int (*funcp)(int a, long l);
   int b;
   };

   ('funcp' generates a DW_TAG_subroutine_type DIE).  */

static struct type *
read_subroutine_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  struct type *type;		/* Type that this function returns.  */
  struct type *ftype;		/* Function that returns above type.  */
  struct attribute *attr;

  type = die_type (die, cu);

  if (type->code () == TYPE_CODE_VOID
      && !type->is_stub ()
      && die->child == nullptr
      && producer_is_gas_2_39 (cu))
    {
      /* Work around PR gas/29517, pretend we have an DW_TAG_unspecified_type
	 return type.  */
      type = (type_allocator (cu->per_objfile->objfile, cu->lang ())
	      .new_type (TYPE_CODE_VOID, 0, nullptr));
      type->set_is_stub (true);
    }

  /* The die_type call above may have already set the type for this DIE.  */
  ftype = get_die_type (die, cu);
  if (ftype)
    return ftype;

  ftype = lookup_function_type (type);

  if (prototyped_function_p (die, cu))
    ftype->set_is_prototyped (true);

  /* Store the calling convention in the type if it's available in
     the subroutine die.  Otherwise set the calling convention to
     the default value DW_CC_normal.  */
  attr = dwarf2_attr (die, DW_AT_calling_convention, cu);
  if (attr != nullptr
      && is_valid_DW_AT_calling_convention_for_subroutine (attr->constant_value (0)))
    TYPE_CALLING_CONVENTION (ftype)
      = (enum dwarf_calling_convention) attr->constant_value (0);
  else if (cu->producer && strstr (cu->producer, "IBM XL C for OpenCL"))
    TYPE_CALLING_CONVENTION (ftype) = DW_CC_GDB_IBM_OpenCL;
  else
    TYPE_CALLING_CONVENTION (ftype) = DW_CC_normal;

  /* Record whether the function returns normally to its caller or not
     if the DWARF producer set that information.  */
  attr = dwarf2_attr (die, DW_AT_noreturn, cu);
  if (attr && attr->as_boolean ())
    TYPE_NO_RETURN (ftype) = 1;

  /* We need to add the subroutine type to the die immediately so
     we don't infinitely recurse when dealing with parameters
     declared as the same subroutine type.  */
  set_die_type (die, ftype, cu);

  if (die->child != NULL)
    {
      struct type *void_type = builtin_type (objfile)->builtin_void;
      struct die_info *child_die;
      int nparams, iparams;

      /* Count the number of parameters.
	 FIXME: GDB currently ignores vararg functions, but knows about
	 vararg member functions.  */
      nparams = 0;
      child_die = die->child;
      while (child_die && child_die->tag)
	{
	  if (child_die->tag == DW_TAG_formal_parameter)
	    nparams++;
	  else if (child_die->tag == DW_TAG_unspecified_parameters)
	    ftype->set_has_varargs (true);

	  child_die = child_die->sibling;
	}

      /* Allocate storage for parameters and fill them in.  */
      ftype->alloc_fields (nparams);

      /* TYPE_FIELD_TYPE must never be NULL.  Pre-fill the array to ensure it
	 even if we error out during the parameters reading below.  */
      for (iparams = 0; iparams < nparams; iparams++)
	ftype->field (iparams).set_type (void_type);

      iparams = 0;
      child_die = die->child;
      while (child_die && child_die->tag)
	{
	  if (child_die->tag == DW_TAG_formal_parameter)
	    {
	      struct type *arg_type;

	      /* DWARF version 2 has no clean way to discern C++
		 static and non-static member functions.  G++ helps
		 GDB by marking the first parameter for non-static
		 member functions (which is the this pointer) as
		 artificial.  We pass this information to
		 dwarf2_add_member_fn via TYPE_FIELD_ARTIFICIAL.

		 DWARF version 3 added DW_AT_object_pointer, which GCC
		 4.5 does not yet generate.  */
	      attr = dwarf2_attr (child_die, DW_AT_artificial, cu);
	      if (attr != nullptr)
		ftype->field (iparams).set_is_artificial (attr->as_boolean ());
	      else
		ftype->field (iparams).set_is_artificial (false);
	      arg_type = die_type (child_die, cu);

	      /* RealView does not mark THIS as const, which the testsuite
		 expects.  GCC marks THIS as const in method definitions,
		 but not in the class specifications (GCC PR 43053).  */
	      if (cu->lang () == language_cplus
		  && !TYPE_CONST (arg_type)
		  && ftype->field (iparams).is_artificial ())
		{
		  int is_this = 0;
		  struct dwarf2_cu *arg_cu = cu;
		  const char *name = dwarf2_name (child_die, cu);

		  attr = dwarf2_attr (die, DW_AT_object_pointer, cu);
		  if (attr != nullptr)
		    {
		      /* If the compiler emits this, use it.  */
		      if (follow_die_ref (die, attr, &arg_cu) == child_die)
			is_this = 1;
		    }
		  else if (name && strcmp (name, "this") == 0)
		    /* Function definitions will have the argument names.  */
		    is_this = 1;
		  else if (name == NULL && iparams == 0)
		    /* Declarations may not have the names, so like
		       elsewhere in GDB, assume an artificial first
		       argument is "this".  */
		    is_this = 1;

		  if (is_this)
		    arg_type = make_cv_type (1, TYPE_VOLATILE (arg_type),
					     arg_type, 0);
		}

	      ftype->field (iparams).set_type (arg_type);
	      iparams++;
	    }
	  child_die = child_die->sibling;
	}
    }

  return ftype;
}

static struct type *
read_typedef (struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  const char *name = dwarf2_full_name (NULL, die, cu);
  struct type *this_type;
  struct gdbarch *gdbarch = objfile->arch ();
  struct type *target_type = die_type (die, cu);

  if (gdbarch_dwarf2_omit_typedef_p (gdbarch, target_type, cu->producer, name))
    {
      /* The long double is defined as a base type in C.  GCC creates a long
	 double typedef with target-type _Float128 for the long double to
	 identify it as the IEEE Float128 value.  This is a GCC hack since the
	 DWARF doesn't distinguish between the IBM long double and IEEE
	 128-bit float.	 Replace the GCC workaround for the long double
	 typedef with the actual type information copied from the target-type
	 with the correct long double base type name.  */
      this_type = copy_type (target_type);
      this_type->set_name (name);
      set_die_type (die, this_type, cu);
      return this_type;
    }

  type_allocator alloc (objfile, cu->lang ());
  this_type = alloc.new_type (TYPE_CODE_TYPEDEF, 0, name);
  this_type->set_target_is_stub (true);
  set_die_type (die, this_type, cu);
  if (target_type != this_type)
    this_type->set_target_type (target_type);
  else
    {
      /* Self-referential typedefs are, it seems, not allowed by the DWARF
	 spec and cause infinite loops in GDB.  */
      complaint (_("Self-referential DW_TAG_typedef "
		   "- DIE at %s [in module %s]"),
		 sect_offset_str (die->sect_off), objfile_name (objfile));
      this_type->set_target_type (nullptr);
    }
  if (name == NULL)
    {
      /* Gcc-7 and before supports -feliminate-dwarf2-dups, which generates
	 anonymous typedefs, which is, strictly speaking, invalid DWARF.
	 Handle these by just returning the target type, rather than
	 constructing an anonymous typedef type and trying to handle this
	 elsewhere.  */
      set_die_type (die, target_type, cu);
      return target_type;
    }
  return this_type;
}

/* Helper for get_dwarf2_rational_constant that computes the value of
   a given gmp_mpz given an attribute.  */

static void
get_mpz (struct dwarf2_cu *cu, gdb_mpz *value, struct attribute *attr)
{
  /* GCC will sometimes emit a 16-byte constant value as a DWARF
     location expression that pushes an implicit value.  */
  if (attr->form == DW_FORM_exprloc)
    {
      dwarf_block *blk = attr->as_block ();
      if (blk->size > 0 && blk->data[0] == DW_OP_implicit_value)
	{
	  uint64_t len;
	  const gdb_byte *ptr = safe_read_uleb128 (blk->data + 1,
						   blk->data + blk->size,
						   &len);
	  if (ptr - blk->data + len <= blk->size)
	    {
	      value->read (gdb::make_array_view (ptr, len),
			   bfd_big_endian (cu->per_objfile->objfile->obfd.get ())
			   ? BFD_ENDIAN_BIG : BFD_ENDIAN_LITTLE,
			   true);
	      return;
	    }
	}

      /* On failure set it to 1.  */
      *value = gdb_mpz (1);
    }
  else if (attr->form_is_block ())
    {
      dwarf_block *blk = attr->as_block ();
      value->read (gdb::make_array_view (blk->data, blk->size),
		   bfd_big_endian (cu->per_objfile->objfile->obfd.get ())
		   ? BFD_ENDIAN_BIG : BFD_ENDIAN_LITTLE,
		   true);
    }
  else if (attr->form_is_unsigned ())
    *value = gdb_mpz (attr->as_unsigned ());
  else
    *value = gdb_mpz (attr->constant_value (1));
}

/* Assuming DIE is a rational DW_TAG_constant, read the DIE's
   numerator and denominator into NUMERATOR and DENOMINATOR (resp).

   If the numerator and/or numerator attribute is missing,
   a complaint is filed, and NUMERATOR and DENOMINATOR are left
   untouched.  */

static void
get_dwarf2_rational_constant (struct die_info *die, struct dwarf2_cu *cu,
			      gdb_mpz *numerator, gdb_mpz *denominator)
{
  struct attribute *num_attr, *denom_attr;

  num_attr = dwarf2_attr (die, DW_AT_GNU_numerator, cu);
  if (num_attr == nullptr)
    complaint (_("DW_AT_GNU_numerator missing in %s DIE at %s"),
	       dwarf_tag_name (die->tag), sect_offset_str (die->sect_off));

  denom_attr = dwarf2_attr (die, DW_AT_GNU_denominator, cu);
  if (denom_attr == nullptr)
    complaint (_("DW_AT_GNU_denominator missing in %s DIE at %s"),
	       dwarf_tag_name (die->tag), sect_offset_str (die->sect_off));

  if (num_attr == nullptr || denom_attr == nullptr)
    return;

  get_mpz (cu, numerator, num_attr);
  get_mpz (cu, denominator, denom_attr);
}

/* Same as get_dwarf2_rational_constant, but extracting an unsigned
   rational constant, rather than a signed one.

   If the rational constant has a negative value, a complaint
   is filed, and NUMERATOR and DENOMINATOR are left untouched.  */

static void
get_dwarf2_unsigned_rational_constant (struct die_info *die,
				       struct dwarf2_cu *cu,
				       gdb_mpz *numerator,
				       gdb_mpz *denominator)
{
  gdb_mpz num (1);
  gdb_mpz denom (1);

  get_dwarf2_rational_constant (die, cu, &num, &denom);
  if (num < 0 && denom < 0)
    {
      num.negate ();
      denom.negate ();
    }
  else if (num < 0)
    {
      complaint (_("unexpected negative value for DW_AT_GNU_numerator"
		   " in DIE at %s"),
		 sect_offset_str (die->sect_off));
      return;
    }
  else if (denom < 0)
    {
      complaint (_("unexpected negative value for DW_AT_GNU_denominator"
		   " in DIE at %s"),
		 sect_offset_str (die->sect_off));
      return;
    }

  *numerator = std::move (num);
  *denominator = std::move (denom);
}

/* Assuming that ENCODING is a string whose contents starting at the
   K'th character is "_nn" where "nn" is a decimal number, scan that
   number and set RESULT to the value. K is updated to point to the
   character immediately following the number.

   If the string does not conform to the format described above, false
   is returned, and K may or may not be changed.  */

static bool
ada_get_gnat_encoded_number (const char *encoding, int &k, gdb_mpz *result)
{
  /* The next character should be an underscore ('_') followed
     by a digit.  */
  if (encoding[k] != '_' || !isdigit (encoding[k + 1]))
    return false;

  /* Skip the underscore.  */
  k++;
  int start = k;

  /* Determine the number of digits for our number.  */
  while (isdigit (encoding[k]))
    k++;
  if (k == start)
    return false;

  std::string copy (&encoding[start], k - start);
  return result->set (copy.c_str (), 10);
}

/* Scan two numbers from ENCODING at OFFSET, assuming the string is of
   the form _NN_DD, where NN and DD are decimal numbers.  Set NUM and
   DENOM, update OFFSET, and return true on success.  Return false on
   failure.  */

static bool
ada_get_gnat_encoded_ratio (const char *encoding, int &offset,
			    gdb_mpz *num, gdb_mpz *denom)
{
  if (!ada_get_gnat_encoded_number (encoding, offset, num))
    return false;
  return ada_get_gnat_encoded_number (encoding, offset, denom);
}

/* Assuming DIE corresponds to a fixed point type, finish the creation
   of the corresponding TYPE by setting its type-specific data.  CU is
   the DIE's CU.  SUFFIX is the "XF" type name suffix coming from GNAT
   encodings.  It is nullptr if the GNAT encoding should be
   ignored.  */

static void
finish_fixed_point_type (struct type *type, const char *suffix,
			 struct die_info *die, struct dwarf2_cu *cu)
{
  gdb_assert (type->code () == TYPE_CODE_FIXED_POINT
	      && TYPE_SPECIFIC_FIELD (type) == TYPE_SPECIFIC_FIXED_POINT);

  /* If GNAT encodings are preferred, don't examine the
     attributes.  */
  struct attribute *attr = nullptr;
  if (suffix == nullptr)
    {
      attr = dwarf2_attr (die, DW_AT_binary_scale, cu);
      if (attr == nullptr)
	attr = dwarf2_attr (die, DW_AT_decimal_scale, cu);
      if (attr == nullptr)
	attr = dwarf2_attr (die, DW_AT_small, cu);
    }

  /* Numerator and denominator of our fixed-point type's scaling factor.
     The default is a scaling factor of 1, which we use as a fallback
     when we are not able to decode it (problem with the debugging info,
     unsupported forms, bug in GDB, etc...).  Using that as the default
     allows us to at least print the unscaled value, which might still
     be useful to a user.  */
  gdb_mpz scale_num (1);
  gdb_mpz scale_denom (1);

  if (attr == nullptr)
    {
      int offset = 0;
      if (suffix != nullptr
	  && ada_get_gnat_encoded_ratio (suffix, offset, &scale_num,
					 &scale_denom)
	  /* The number might be encoded as _nn_dd_nn_dd, where the
	     second ratio is the 'small value.  In this situation, we
	     want the second value.  */
	  && (suffix[offset] != '_'
	      || ada_get_gnat_encoded_ratio (suffix, offset, &scale_num,
					     &scale_denom)))
	{
	  /* Found it.  */
	}
      else
	{
	  /* Scaling factor not found.  Assume a scaling factor of 1,
	     and hope for the best.  At least the user will be able to
	     see the encoded value.  */
	  scale_num = 1;
	  scale_denom = 1;
	  complaint (_("no scale found for fixed-point type (DIE at %s)"),
		     sect_offset_str (die->sect_off));
	}
    }
  else if (attr->name == DW_AT_binary_scale)
    {
      LONGEST scale_exp = attr->constant_value (0);
      gdb_mpz &num_or_denom = scale_exp > 0 ? scale_num : scale_denom;

      num_or_denom <<= std::abs (scale_exp);
    }
  else if (attr->name == DW_AT_decimal_scale)
    {
      LONGEST scale_exp = attr->constant_value (0);
      gdb_mpz &num_or_denom = scale_exp > 0 ? scale_num : scale_denom;

      num_or_denom = gdb_mpz::pow (10, std::abs (scale_exp));
    }
  else if (attr->name == DW_AT_small)
    {
      struct die_info *scale_die;
      struct dwarf2_cu *scale_cu = cu;

      scale_die = follow_die_ref (die, attr, &scale_cu);
      if (scale_die->tag == DW_TAG_constant)
	get_dwarf2_unsigned_rational_constant (scale_die, scale_cu,
					       &scale_num, &scale_denom);
      else
	complaint (_("%s DIE not supported as target of DW_AT_small attribute"
		     " (DIE at %s)"),
		   dwarf_tag_name (die->tag), sect_offset_str (die->sect_off));
    }
  else
    {
      complaint (_("unsupported scale attribute %s for fixed-point type"
		   " (DIE at %s)"),
		 dwarf_attr_name (attr->name),
		 sect_offset_str (die->sect_off));
    }

  type->fixed_point_info ().scaling_factor = gdb_mpq (scale_num, scale_denom);
}

/* The gnat-encoding suffix for fixed point.  */

#define GNAT_FIXED_POINT_SUFFIX "___XF_"

/* If NAME encodes an Ada fixed-point type, return a pointer to the
   "XF" suffix of the name.  The text after this is what encodes the
   'small and 'delta information.  Otherwise, return nullptr.  */

static const char *
gnat_encoded_fixed_point_type_info (const char *name)
{
  return strstr (name, GNAT_FIXED_POINT_SUFFIX);
}

/* Allocate a floating-point type of size BITS and name NAME.  Pass NAME_HINT
   (which may be different from NAME) to the architecture back-end to allow
   it to guess the correct format if necessary.  */

static struct type *
dwarf2_init_float_type (struct dwarf2_cu *cu, int bits, const char *name,
			const char *name_hint, enum bfd_endian byte_order)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  struct gdbarch *gdbarch = objfile->arch ();
  const struct floatformat **format;
  struct type *type;

  type_allocator alloc (objfile, cu->lang ());
  format = gdbarch_floatformat_for_type (gdbarch, name_hint, bits);
  if (format)
    type = init_float_type (alloc, bits, name, format, byte_order);
  else
    type = alloc.new_type (TYPE_CODE_ERROR, bits, name);

  return type;
}

/* Allocate an integer type of size BITS and name NAME.  */

static struct type *
dwarf2_init_integer_type (struct dwarf2_cu *cu, int bits, int unsigned_p,
			  const char *name)
{
  struct type *type;
  struct objfile *objfile = cu->per_objfile->objfile;

  /* Versions of Intel's C Compiler generate an integer type called "void"
     instead of using DW_TAG_unspecified_type.  This has been seen on
     at least versions 14, 17, and 18.  */
  if (bits == 0 && producer_is_icc (cu) && name != nullptr
      && strcmp (name, "void") == 0)
    type = builtin_type (objfile)->builtin_void;
  else
    {
      type_allocator alloc (objfile, cu->lang ());
      type = init_integer_type (alloc, bits, unsigned_p, name);
    }

  return type;
}

/* Return true if DIE has a DW_AT_small attribute whose value is
   a constant rational, where both the numerator and denominator
   are equal to zero.

   CU is the DIE's Compilation Unit.  */

static bool
has_zero_over_zero_small_attribute (struct die_info *die,
				    struct dwarf2_cu *cu)
{
  struct attribute *attr = dwarf2_attr (die, DW_AT_small, cu);
  if (attr == nullptr)
    return false;

  struct dwarf2_cu *scale_cu = cu;
  struct die_info *scale_die
    = follow_die_ref (die, attr, &scale_cu);

  if (scale_die->tag != DW_TAG_constant)
    return false;

  gdb_mpz num (1), denom (1);
  get_dwarf2_rational_constant (scale_die, cu, &num, &denom);
  return num == 0 && denom == 0;
}

/* Initialise and return a floating point type of size BITS suitable for
   use as a component of a complex number.  The NAME_HINT is passed through
   when initialising the floating point type and is the name of the complex
   type.

   As DWARF doesn't currently provide an explicit name for the components
   of a complex number, but it can be helpful to have these components
   named, we try to select a suitable name based on the size of the
   component.  */
static struct type *
dwarf2_init_complex_target_type (struct dwarf2_cu *cu,
				 int bits, const char *name_hint,
				 enum bfd_endian byte_order)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  gdbarch *gdbarch = objfile->arch ();
  struct type *tt = nullptr;

  /* Try to find a suitable floating point builtin type of size BITS.
     We're going to use the name of this type as the name for the complex
     target type that we are about to create.  */
  switch (cu->lang ())
    {
    case language_fortran:
      switch (bits)
	{
	case 32:
	  tt = builtin_f_type (gdbarch)->builtin_real;
	  break;
	case 64:
	  tt = builtin_f_type (gdbarch)->builtin_real_s8;
	  break;
	case 96:	/* The x86-32 ABI specifies 96-bit long double.  */
	case 128:
	  tt = builtin_f_type (gdbarch)->builtin_real_s16;
	  break;
	}
      break;
    default:
      switch (bits)
	{
	case 32:
	  tt = builtin_type (gdbarch)->builtin_float;
	  break;
	case 64:
	  tt = builtin_type (gdbarch)->builtin_double;
	  break;
	case 96:	/* The x86-32 ABI specifies 96-bit long double.  */
	case 128:
	  tt = builtin_type (gdbarch)->builtin_long_double;
	  break;
	}
      break;
    }

  /* If the type we found doesn't match the size we were looking for, then
     pretend we didn't find a type at all, the complex target type we
     create will then be nameless.  */
  if (tt != nullptr && tt->length () * TARGET_CHAR_BIT != bits)
    tt = nullptr;

  const char *name = (tt == nullptr) ? nullptr : tt->name ();
  return dwarf2_init_float_type (cu, bits, name, name_hint, byte_order);
}

/* Find a representation of a given base type and install
   it in the TYPE field of the die.  */

static struct type *
read_base_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  struct type *type;
  struct attribute *attr;
  int encoding = 0, bits = 0;
  const char *name;
  gdbarch *arch;

  attr = dwarf2_attr (die, DW_AT_encoding, cu);
  if (attr != nullptr && attr->form_is_constant ())
    encoding = attr->constant_value (0);
  attr = dwarf2_attr (die, DW_AT_byte_size, cu);
  if (attr != nullptr)
    bits = attr->constant_value (0) * TARGET_CHAR_BIT;
  name = dwarf2_name (die, cu);
  if (!name)
    complaint (_("DW_AT_name missing from DW_TAG_base_type"));

  arch = objfile->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (arch);

  attr = dwarf2_attr (die, DW_AT_endianity, cu);
  if (attr != nullptr && attr->form_is_constant ())
    {
      int endianity = attr->constant_value (0);

      switch (endianity)
	{
	case DW_END_big:
	  byte_order = BFD_ENDIAN_BIG;
	  break;
	case DW_END_little:
	  byte_order = BFD_ENDIAN_LITTLE;
	  break;
	default:
	  complaint (_("DW_AT_endianity has unrecognized value %d"), endianity);
	  break;
	}
    }

  if ((encoding == DW_ATE_signed_fixed || encoding == DW_ATE_unsigned_fixed)
      && cu->lang () == language_ada
      && has_zero_over_zero_small_attribute (die, cu))
    {
      /* brobecker/2018-02-24: This is a fixed point type for which
	 the scaling factor is represented as fraction whose value
	 does not make sense (zero divided by zero), so we should
	 normally never see these.  However, there is a small category
	 of fixed point types for which GNAT is unable to provide
	 the scaling factor via the standard DWARF mechanisms, and
	 for which the info is provided via the GNAT encodings instead.
	 This is likely what this DIE is about.  */
      encoding = (encoding == DW_ATE_signed_fixed
		  ? DW_ATE_signed
		  : DW_ATE_unsigned);
    }

  /* With GNAT encodings, fixed-point information will be encoded in
     the type name.  Note that this can also occur with the above
     zero-over-zero case, which is why this is a separate "if" rather
     than an "else if".  */
  const char *gnat_encoding_suffix = nullptr;
  if ((encoding == DW_ATE_signed || encoding == DW_ATE_unsigned)
      && cu->lang () == language_ada
      && name != nullptr)
    {
      gnat_encoding_suffix = gnat_encoded_fixed_point_type_info (name);
      if (gnat_encoding_suffix != nullptr)
	{
	  gdb_assert (startswith (gnat_encoding_suffix,
				  GNAT_FIXED_POINT_SUFFIX));
	  name = obstack_strndup (&cu->per_objfile->objfile->objfile_obstack,
				  name, gnat_encoding_suffix - name);
	  /* Use -1 here so that SUFFIX points at the "_" after the
	     "XF".  */
	  gnat_encoding_suffix += strlen (GNAT_FIXED_POINT_SUFFIX) - 1;

	  encoding = (encoding == DW_ATE_signed
		      ? DW_ATE_signed_fixed
		      : DW_ATE_unsigned_fixed);
	}
    }

  type_allocator alloc (objfile, cu->lang ());
  switch (encoding)
    {
      case DW_ATE_address:
	/* Turn DW_ATE_address into a void * pointer.  */
	type = alloc.new_type (TYPE_CODE_VOID, TARGET_CHAR_BIT, NULL);
	type = init_pointer_type (alloc, bits, name, type);
	break;
      case DW_ATE_boolean:
	type = init_boolean_type (alloc, bits, 1, name);
	break;
      case DW_ATE_complex_float:
	type = dwarf2_init_complex_target_type (cu, bits / 2, name,
						byte_order);
	if (type->code () == TYPE_CODE_ERROR)
	  {
	    if (name == nullptr)
	      {
		struct obstack *obstack
		  = &cu->per_objfile->objfile->objfile_obstack;
		name = obconcat (obstack, "_Complex ", type->name (),
				 nullptr);
	      }
	    type = alloc.new_type (TYPE_CODE_ERROR, bits, name);
	  }
	else
	  type = init_complex_type (name, type);
	break;
      case DW_ATE_decimal_float:
	type = init_decfloat_type (alloc, bits, name);
	break;
      case DW_ATE_float:
	type = dwarf2_init_float_type (cu, bits, name, name, byte_order);
	break;
      case DW_ATE_signed:
	type = dwarf2_init_integer_type (cu, bits, 0, name);
	break;
      case DW_ATE_unsigned:
	if (cu->lang () == language_fortran
	    && name
	    && startswith (name, "character("))
	  type = init_character_type (alloc, bits, 1, name);
	else
	  type = dwarf2_init_integer_type (cu, bits, 1, name);
	break;
      case DW_ATE_signed_char:
	if (cu->lang () == language_ada
	    || cu->lang () == language_m2
	    || cu->lang () == language_pascal
	    || cu->lang () == language_fortran)
	  type = init_character_type (alloc, bits, 0, name);
	else
	  type = dwarf2_init_integer_type (cu, bits, 0, name);
	break;
      case DW_ATE_unsigned_char:
	if (cu->lang () == language_ada
	    || cu->lang () == language_m2
	    || cu->lang () == language_pascal
	    || cu->lang () == language_fortran
	    || cu->lang () == language_rust)
	  type = init_character_type (alloc, bits, 1, name);
	else
	  type = dwarf2_init_integer_type (cu, bits, 1, name);
	break;
      case DW_ATE_UTF:
	{
	  type = init_character_type (alloc, bits, 1, name);
	  return set_die_type (die, type, cu);
	}
	break;
      case DW_ATE_signed_fixed:
	type = init_fixed_point_type (alloc, bits, 0, name);
	finish_fixed_point_type (type, gnat_encoding_suffix, die, cu);
	break;
      case DW_ATE_unsigned_fixed:
	type = init_fixed_point_type (alloc, bits, 1, name);
	finish_fixed_point_type (type, gnat_encoding_suffix, die, cu);
	break;

      default:
	complaint (_("unsupported DW_AT_encoding: '%s'"),
		   dwarf_type_encoding_name (encoding));
	type = alloc.new_type (TYPE_CODE_ERROR, bits, name);
	break;
    }

  if (type->code () == TYPE_CODE_INT
      && name != nullptr
      && strcmp (name, "char") == 0)
    type->set_has_no_signedness (true);

  maybe_set_alignment (cu, die, type);

  type->set_endianity_is_not_default (gdbarch_byte_order (arch) != byte_order);

  if (TYPE_SPECIFIC_FIELD (type) == TYPE_SPECIFIC_INT)
    {
      attr = dwarf2_attr (die, DW_AT_bit_size, cu);
      if (attr != nullptr && attr->form_is_constant ())
	{
	  unsigned real_bit_size = attr->constant_value (0);
	  if (real_bit_size >= 0 && real_bit_size <= 8 * type->length ())
	    {
	      attr = dwarf2_attr (die, DW_AT_data_bit_offset, cu);
	      /* Only use the attributes if they make sense together.  */
	      if (attr == nullptr
		  || (attr->form_is_constant ()
		      && attr->constant_value (0) >= 0
		      && (attr->constant_value (0) + real_bit_size
			  <= 8 * type->length ())))
		{
		  TYPE_MAIN_TYPE (type)->type_specific.int_stuff.bit_size
		    = real_bit_size;
		  if (attr != nullptr)
		    TYPE_MAIN_TYPE (type)->type_specific.int_stuff.bit_offset
		      = attr->constant_value (0);
		}
	    }
	}
    }

  return set_die_type (die, type, cu);
}

/* A helper function that returns the name of DIE, if it refers to a
   variable declaration.  */

static const char *
var_decl_name (struct die_info *die, struct dwarf2_cu *cu)
{
  if (die->tag != DW_TAG_variable)
    return nullptr;

  attribute *attr = dwarf2_attr (die, DW_AT_declaration, cu);
  if (attr == nullptr || !attr->as_boolean ())
    return nullptr;

  attr = dwarf2_attr (die, DW_AT_name, cu);
  if (attr == nullptr)
    return nullptr;
  return attr->as_string ();
}

/* Parse dwarf attribute if it's a block, reference or constant and put the
   resulting value of the attribute into struct bound_prop.
   Returns 1 if ATTR could be resolved into PROP, 0 otherwise.  */

static int
attr_to_dynamic_prop (const struct attribute *attr, struct die_info *die,
		      struct dwarf2_cu *cu, struct dynamic_prop *prop,
		      struct type *default_type)
{
  struct dwarf2_property_baton *baton;
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  struct obstack *obstack = &objfile->objfile_obstack;

  gdb_assert (default_type != NULL);

  if (attr == NULL || prop == NULL)
    return 0;

  if (attr->form_is_block ())
    {
      baton = XOBNEW (obstack, struct dwarf2_property_baton);
      baton->property_type = default_type;
      baton->locexpr.per_cu = cu->per_cu;
      baton->locexpr.per_objfile = per_objfile;

      struct dwarf_block *block;
      if (attr->form == DW_FORM_data16)
	{
	  size_t data_size = 16;
	  block = XOBNEW (obstack, struct dwarf_block);
	  block->size = (data_size
			 + 2 /* Extra bytes for DW_OP and arg.  */);
	  gdb_byte *data = XOBNEWVEC (obstack, gdb_byte, block->size);
	  data[0] = DW_OP_implicit_value;
	  data[1] = data_size;
	  memcpy (&data[2], attr->as_block ()->data, data_size);
	  block->data = data;
	}
      else
	block = attr->as_block ();

      baton->locexpr.size = block->size;
      baton->locexpr.data = block->data;
      switch (attr->name)
	{
	case DW_AT_string_length:
	  baton->locexpr.is_reference = true;
	  break;
	default:
	  baton->locexpr.is_reference = false;
	  break;
	}

      prop->set_locexpr (baton);
      gdb_assert (prop->baton () != NULL);
    }
  else if (attr->form_is_ref ())
    {
      struct dwarf2_cu *target_cu = cu;
      struct die_info *target_die;
      struct attribute *target_attr;

      target_die = follow_die_ref (die, attr, &target_cu);
      target_attr = dwarf2_attr (target_die, DW_AT_location, target_cu);
      if (target_attr == NULL)
	target_attr = dwarf2_attr (target_die, DW_AT_data_member_location,
				   target_cu);
      if (target_attr == nullptr)
	target_attr = dwarf2_attr (target_die, DW_AT_data_bit_offset,
				   target_cu);
      if (target_attr == NULL)
	{
	  const char *name = var_decl_name (target_die, target_cu);
	  if (name != nullptr)
	    {
	      prop->set_variable_name (name);
	      return 1;
	    }
	  return 0;
	}

      switch (target_attr->name)
	{
	  case DW_AT_location:
	    if (target_attr->form_is_section_offset ())
	      {
		baton = XOBNEW (obstack, struct dwarf2_property_baton);
		baton->property_type = die_type (target_die, target_cu);
		fill_in_loclist_baton (cu, &baton->loclist, target_attr);
		prop->set_loclist (baton);
		gdb_assert (prop->baton () != NULL);
	      }
	    else if (target_attr->form_is_block ())
	      {
		baton = XOBNEW (obstack, struct dwarf2_property_baton);
		baton->property_type = die_type (target_die, target_cu);
		baton->locexpr.per_cu = cu->per_cu;
		baton->locexpr.per_objfile = per_objfile;
		struct dwarf_block *block = target_attr->as_block ();
		baton->locexpr.size = block->size;
		baton->locexpr.data = block->data;
		baton->locexpr.is_reference = true;
		prop->set_locexpr (baton);
		gdb_assert (prop->baton () != NULL);
	      }
	    else
	      {
		dwarf2_invalid_attrib_class_complaint ("DW_AT_location",
						       "dynamic property");
		return 0;
	      }
	    break;
	  case DW_AT_data_member_location:
	  case DW_AT_data_bit_offset:
	    {
	      LONGEST offset;

	      if (!handle_member_location (target_die, target_cu, &offset))
		return 0;

	      baton = XOBNEW (obstack, struct dwarf2_property_baton);
	      baton->property_type = read_type_die (target_die->parent,
						      target_cu);
	      baton->offset_info.offset = offset;
	      baton->offset_info.type = die_type (target_die, target_cu);
	      prop->set_addr_offset (baton);
	      break;
	    }
	}
    }
  else if (attr->form_is_constant ())
    prop->set_const_val (attr->constant_value (0));
  else if (attr->form_is_section_offset ())
    {
      switch (attr->name)
	{
	case DW_AT_string_length:
	  baton = XOBNEW (obstack, struct dwarf2_property_baton);
	  baton->property_type = default_type;
	  fill_in_loclist_baton (cu, &baton->loclist, attr);
	  prop->set_loclist (baton);
	  gdb_assert (prop->baton () != NULL);
	  break;
	default:
	  goto invalid;
	}
    }
  else
    goto invalid;

  return 1;

 invalid:
  dwarf2_invalid_attrib_class_complaint (dwarf_form_name (attr->form),
					 dwarf2_name (die, cu));
  return 0;
}

/* See read.h.  */

/* Read the DW_AT_type attribute for a sub-range.  If this attribute is not
   present (which is valid) then compute the default type based on the
   compilation units address size.  */

static struct type *
read_subrange_index_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct type *index_type = die_type (die, cu);

  /* Dwarf-2 specifications explicitly allows to create subrange types
     without specifying a base type.
     In that case, the base type must be set to the type of
     the lower bound, upper bound or count, in that order, if any of these
     three attributes references an object that has a type.
     If no base type is found, the Dwarf-2 specifications say that
     a signed integer type of size equal to the size of an address should
     be used.
     For the following C code: `extern char gdb_int [];'
     GCC produces an empty range DIE.
     FIXME: muller/2010-05-28: Possible references to object for low bound,
     high bound or count are not yet handled by this code.  */
  if (index_type->code () == TYPE_CODE_VOID)
    index_type = cu->addr_sized_int_type (false);

  return index_type;
}

/* Read the given DW_AT_subrange DIE.  */

static struct type *
read_subrange_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct type *base_type, *orig_base_type;
  struct type *range_type;
  struct attribute *attr;
  struct dynamic_prop low, high;
  int low_default_is_valid;
  int high_bound_is_count = 0;
  const char *name;

  orig_base_type = read_subrange_index_type (die, cu);

  /* If ORIG_BASE_TYPE is a typedef, it will not be TYPE_UNSIGNED,
     whereas the real type might be.  So, we use ORIG_BASE_TYPE when
     creating the range type, but we use the result of check_typedef
     when examining properties of the type.  */
  base_type = check_typedef (orig_base_type);

  /* The die_type call above may have already set the type for this DIE.  */
  range_type = get_die_type (die, cu);
  if (range_type)
    return range_type;

  high.set_const_val (0);

  /* Set LOW_DEFAULT_IS_VALID if current language and DWARF version allow
     omitting DW_AT_lower_bound.  */
  switch (cu->lang ())
    {
    case language_c:
    case language_cplus:
      low.set_const_val (0);
      low_default_is_valid = 1;
      break;
    case language_fortran:
      low.set_const_val (1);
      low_default_is_valid = 1;
      break;
    case language_d:
    case language_objc:
    case language_rust:
      low.set_const_val (0);
      low_default_is_valid = (cu->header.version >= 4);
      break;
    case language_ada:
    case language_m2:
    case language_pascal:
      low.set_const_val (1);
      low_default_is_valid = (cu->header.version >= 4);
      break;
    default:
      low.set_const_val (0);
      low_default_is_valid = 0;
      break;
    }

  attr = dwarf2_attr (die, DW_AT_lower_bound, cu);
  if (attr != nullptr)
    attr_to_dynamic_prop (attr, die, cu, &low, base_type);
  else if (!low_default_is_valid)
    complaint (_("Missing DW_AT_lower_bound "
				      "- DIE at %s [in module %s]"),
	       sect_offset_str (die->sect_off),
	       objfile_name (cu->per_objfile->objfile));

  struct attribute *attr_ub, *attr_count;
  attr = attr_ub = dwarf2_attr (die, DW_AT_upper_bound, cu);
  if (!attr_to_dynamic_prop (attr, die, cu, &high, base_type))
    {
      attr = attr_count = dwarf2_attr (die, DW_AT_count, cu);
      if (attr_to_dynamic_prop (attr, die, cu, &high, base_type))
	{
	  /* If bounds are constant do the final calculation here.  */
	  if (low.is_constant () && high.is_constant ())
	    high.set_const_val (low.const_val () + high.const_val () - 1);
	  else
	    high_bound_is_count = 1;
	}
      else
	{
	  if (attr_ub != NULL)
	    complaint (_("Unresolved DW_AT_upper_bound "
			 "- DIE at %s [in module %s]"),
		       sect_offset_str (die->sect_off),
		       objfile_name (cu->per_objfile->objfile));
	  if (attr_count != NULL)
	    complaint (_("Unresolved DW_AT_count "
			 "- DIE at %s [in module %s]"),
		       sect_offset_str (die->sect_off),
		       objfile_name (cu->per_objfile->objfile));
	}
    }

  LONGEST bias = 0;
  struct attribute *bias_attr = dwarf2_attr (die, DW_AT_GNU_bias, cu);
  if (bias_attr != nullptr && bias_attr->form_is_constant ())
    bias = bias_attr->constant_value (0);

  /* Normally, the DWARF producers are expected to use a signed
     constant form (Eg. DW_FORM_sdata) to express negative bounds.
     But this is unfortunately not always the case, as witnessed
     with GCC, for instance, where the ambiguous DW_FORM_dataN form
     is used instead.  To work around that ambiguity, we treat
     the bounds as signed, and thus sign-extend their values, when
     the base type is signed.

     Skip it if the base type's length is larger than ULONGEST, to avoid
     the undefined behavior of a too large left shift.  We don't really handle
     constants larger than 8 bytes anyway, at the moment.  */

  if (base_type->length () <= sizeof (ULONGEST))
    {
      ULONGEST negative_mask
	= -((ULONGEST) 1 << (base_type->length () * TARGET_CHAR_BIT - 1));

      if (low.is_constant ()
	  && !base_type->is_unsigned () && (low.const_val () & negative_mask))
	low.set_const_val (low.const_val () | negative_mask);

      if (high.is_constant ()
	  && !base_type->is_unsigned () && (high.const_val () & negative_mask))
	high.set_const_val (high.const_val () | negative_mask);
    }

  /* Check for bit and byte strides.  */
  struct dynamic_prop byte_stride_prop;
  attribute *attr_byte_stride = dwarf2_attr (die, DW_AT_byte_stride, cu);
  if (attr_byte_stride != nullptr)
    {
      struct type *prop_type = cu->addr_sized_int_type (false);
      attr_to_dynamic_prop (attr_byte_stride, die, cu, &byte_stride_prop,
			    prop_type);
    }

  struct dynamic_prop bit_stride_prop;
  attribute *attr_bit_stride = dwarf2_attr (die, DW_AT_bit_stride, cu);
  if (attr_bit_stride != nullptr)
    {
      /* It only makes sense to have either a bit or byte stride.  */
      if (attr_byte_stride != nullptr)
	{
	  complaint (_("Found DW_AT_bit_stride and DW_AT_byte_stride "
		       "- DIE at %s [in module %s]"),
		     sect_offset_str (die->sect_off),
		     objfile_name (cu->per_objfile->objfile));
	  attr_bit_stride = nullptr;
	}
      else
	{
	  struct type *prop_type = cu->addr_sized_int_type (false);
	  attr_to_dynamic_prop (attr_bit_stride, die, cu, &bit_stride_prop,
				prop_type);
	}
    }

  type_allocator alloc (cu->per_objfile->objfile, cu->lang ());
  if (attr_byte_stride != nullptr
      || attr_bit_stride != nullptr)
    {
      bool byte_stride_p = (attr_byte_stride != nullptr);
      struct dynamic_prop *stride
	= byte_stride_p ? &byte_stride_prop : &bit_stride_prop;

      range_type
	= create_range_type_with_stride (alloc, orig_base_type, &low,
					 &high, bias, stride, byte_stride_p);
    }
  else
    range_type = create_range_type (alloc, orig_base_type, &low, &high, bias);

  if (high_bound_is_count)
    range_type->bounds ()->flag_upper_bound_is_count = 1;

  /* Ada expects an empty array on no boundary attributes.  */
  if (attr == NULL && cu->lang () != language_ada)
    range_type->bounds ()->high.set_undefined ();

  name = dwarf2_name (die, cu);
  if (name)
    range_type->set_name (name);

  attr = dwarf2_attr (die, DW_AT_byte_size, cu);
  if (attr != nullptr)
    range_type->set_length (attr->constant_value (0));

  maybe_set_alignment (cu, die, range_type);

  set_die_type (die, range_type, cu);

  /* set_die_type should be already done.  */
  set_descriptive_type (range_type, die, cu);

  return range_type;
}

static struct type *
read_unspecified_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct type *type;

  type = (type_allocator (cu->per_objfile->objfile, cu->lang ())
	  .new_type (TYPE_CODE_VOID, 0, nullptr));
  type->set_name (dwarf2_name (die, cu));

  /* In Ada, an unspecified type is typically used when the description
     of the type is deferred to a different unit.  When encountering
     such a type, we treat it as a stub, and try to resolve it later on,
     when needed.
     Mark this as a stub type for all languages though.  */
  type->set_is_stub (true);

  return set_die_type (die, type, cu);
}

/* Read a single die and all its descendents.  Set the die's sibling
   field to NULL; set other fields in the die correctly, and set all
   of the descendents' fields correctly.  Set *NEW_INFO_PTR to the
   location of the info_ptr after reading all of those dies.  PARENT
   is the parent of the die in question.  */

static struct die_info *
read_die_and_children (const struct die_reader_specs *reader,
		       const gdb_byte *info_ptr,
		       const gdb_byte **new_info_ptr,
		       struct die_info *parent)
{
  struct die_info *die;
  const gdb_byte *cur_ptr;

  cur_ptr = read_full_die_1 (reader, &die, info_ptr, 0, true);
  if (die == NULL)
    {
      *new_info_ptr = cur_ptr;
      return NULL;
    }
  store_in_ref_table (die, reader->cu);

  if (die->has_children)
    die->child = read_die_and_siblings_1 (reader, cur_ptr, new_info_ptr, die);
  else
    {
      die->child = NULL;
      *new_info_ptr = cur_ptr;
    }

  die->sibling = NULL;
  die->parent = parent;
  return die;
}

/* Read a die, all of its descendents, and all of its siblings; set
   all of the fields of all of the dies correctly.  Arguments are as
   in read_die_and_children.  */

static struct die_info *
read_die_and_siblings_1 (const struct die_reader_specs *reader,
			 const gdb_byte *info_ptr,
			 const gdb_byte **new_info_ptr,
			 struct die_info *parent)
{
  struct die_info *first_die, *last_sibling;
  const gdb_byte *cur_ptr;

  cur_ptr = info_ptr;
  first_die = last_sibling = NULL;

  while (1)
    {
      struct die_info *die
	= read_die_and_children (reader, cur_ptr, &cur_ptr, parent);

      if (die == NULL)
	{
	  *new_info_ptr = cur_ptr;
	  return first_die;
	}

      if (!first_die)
	first_die = die;
      else
	last_sibling->sibling = die;

      last_sibling = die;
    }
}

/* Read a die, all of its descendents, and all of its siblings; set
   all of the fields of all of the dies correctly.  Arguments are as
   in read_die_and_children.
   This the main entry point for reading a DIE and all its children.  */

static struct die_info *
read_die_and_siblings (const struct die_reader_specs *reader,
		       const gdb_byte *info_ptr,
		       const gdb_byte **new_info_ptr,
		       struct die_info *parent)
{
  struct die_info *die = read_die_and_siblings_1 (reader, info_ptr,
						  new_info_ptr, parent);

  if (dwarf_die_debug)
    {
      gdb_printf (gdb_stdlog,
		  "Read die from %s@0x%x of %s:\n",
		  reader->die_section->get_name (),
		  (unsigned) (info_ptr - reader->die_section->buffer),
		  bfd_get_filename (reader->abfd));
      die->dump (dwarf_die_debug);
    }

  return die;
}

/* Read a die and all its attributes, leave space for NUM_EXTRA_ATTRS
   attributes.
   The caller is responsible for filling in the extra attributes
   and updating (*DIEP)->num_attrs.
   Set DIEP to point to a newly allocated die with its information,
   except for its child, sibling, and parent fields.  */

static const gdb_byte *
read_full_die_1 (const struct die_reader_specs *reader,
		 struct die_info **diep, const gdb_byte *info_ptr,
		 int num_extra_attrs, bool allow_reprocess)
{
  unsigned int abbrev_number, bytes_read, i;
  const struct abbrev_info *abbrev;
  struct die_info *die;
  struct dwarf2_cu *cu = reader->cu;
  bfd *abfd = reader->abfd;

  sect_offset sect_off = (sect_offset) (info_ptr - reader->buffer);
  abbrev_number = read_unsigned_leb128 (abfd, info_ptr, &bytes_read);
  info_ptr += bytes_read;
  if (!abbrev_number)
    {
      *diep = NULL;
      return info_ptr;
    }

  abbrev = reader->abbrev_table->lookup_abbrev (abbrev_number);
  if (!abbrev)
    error (_("Dwarf Error: could not find abbrev number %d [in module %s]"),
	   abbrev_number,
	   bfd_get_filename (abfd));

  die = die_info::allocate (&cu->comp_unit_obstack,
			    abbrev->num_attrs + num_extra_attrs);
  die->sect_off = sect_off;
  die->tag = abbrev->tag;
  die->abbrev = abbrev_number;
  die->has_children = abbrev->has_children;

  /* Make the result usable.
     The caller needs to update num_attrs after adding the extra
     attributes.  */
  die->num_attrs = abbrev->num_attrs;

  for (i = 0; i < abbrev->num_attrs; ++i)
    info_ptr = read_attribute (reader, &die->attrs[i], &abbrev->attrs[i],
			       info_ptr, allow_reprocess);

  *diep = die;
  return info_ptr;
}

/* Read a die and all its attributes.
   Set DIEP to point to a newly allocated die with its information,
   except for its child, sibling, and parent fields.  */

static const gdb_byte *
read_toplevel_die (const struct die_reader_specs *reader,
		   struct die_info **diep, const gdb_byte *info_ptr,
		   gdb::array_view<attribute *> extra_attrs)
{
  const gdb_byte *result;
  struct dwarf2_cu *cu = reader->cu;

  result = read_full_die_1 (reader, diep, info_ptr, extra_attrs.size (),
			    false);

  /* Copy in the extra attributes, if any.  */
  attribute *next = &(*diep)->attrs[(*diep)->num_attrs];
  for (attribute *extra : extra_attrs)
    *next++ = *extra;

  struct attribute *attr = (*diep)->attr (DW_AT_str_offsets_base);
  if (attr != nullptr && attr->form_is_unsigned ())
    cu->str_offsets_base = attr->as_unsigned ();

  attr = (*diep)->attr (DW_AT_loclists_base);
  if (attr != nullptr)
    cu->loclist_base = attr->as_unsigned ();

  auto maybe_addr_base = (*diep)->addr_base ();
  if (maybe_addr_base.has_value ())
    cu->addr_base = *maybe_addr_base;

  attr = (*diep)->attr (DW_AT_rnglists_base);
  if (attr != nullptr)
    cu->rnglists_base = attr->as_unsigned ();

  for (int i = 0; i < (*diep)->num_attrs; ++i)
    {
      if ((*diep)->attrs[i].form_requires_reprocessing ())
	read_attribute_reprocess (reader, &(*diep)->attrs[i], (*diep)->tag);
    }

  (*diep)->num_attrs += extra_attrs.size ();

  if (dwarf_die_debug)
    {
      gdb_printf (gdb_stdlog,
		  "Read die from %s@0x%x of %s:\n",
		  reader->die_section->get_name (),
		  (unsigned) (info_ptr - reader->die_section->buffer),
		  bfd_get_filename (reader->abfd));
      (*diep)->dump (dwarf_die_debug);
    }

  return result;
}


void
cooked_indexer::check_bounds (cutu_reader *reader)
{
  if (reader->cu->per_cu->addresses_seen)
    return;

  dwarf2_cu *cu = reader->cu;

  unrelocated_addr best_lowpc = {}, best_highpc = {};
  /* Possibly set the default values of LOWPC and HIGHPC from
     `DW_AT_ranges'.  */
  dwarf2_find_base_address (reader->comp_unit_die, cu);
  enum pc_bounds_kind cu_bounds_kind
    = dwarf2_get_pc_bounds (reader->comp_unit_die, &best_lowpc, &best_highpc,
			    cu, m_index_storage->get_addrmap (), cu->per_cu);
  if (cu_bounds_kind == PC_BOUNDS_HIGH_LOW && best_lowpc < best_highpc)
    {
      dwarf2_per_objfile *per_objfile = cu->per_objfile;
      unrelocated_addr low = per_objfile->adjust (best_lowpc);
      unrelocated_addr high = per_objfile->adjust (best_highpc);
      /* Store the contiguous range if it is not empty; it can be
	 empty for CUs with no code.  addrmap requires CORE_ADDR, so
	 we cast here.  */
      m_index_storage->get_addrmap ()->set_empty ((CORE_ADDR) low,
						  (CORE_ADDR) high - 1,
						  cu->per_cu);

      cu->per_cu->addresses_seen = true;
    }
}

/* Helper function that returns true if TAG can have a linkage
   name.  */

static bool
tag_can_have_linkage_name (enum dwarf_tag tag)
{
  switch (tag)
    {
    case DW_TAG_variable:
    case DW_TAG_subprogram:
      return true;

    default:
      return false;
    }
}

cutu_reader *
cooked_indexer::ensure_cu_exists (cutu_reader *reader,
				  dwarf2_per_objfile *per_objfile,
				  sect_offset sect_off, bool is_dwz,
				  bool for_scanning)
{
  /* Lookups for type unit references are always in the CU, and
     cross-CU references will crash.  */
  if (reader->cu->per_cu->is_dwz == is_dwz
      && reader->cu->header.offset_in_cu_p (sect_off))
    return reader;

  dwarf2_per_cu_data *per_cu
    = dwarf2_find_containing_comp_unit (sect_off, is_dwz,
					per_objfile->per_bfd);

  /* When scanning, we only want to visit a given CU a single time.
     Doing this check here avoids self-imports as well.  */
  if (for_scanning)
    {
      bool nope = false;
      if (!per_cu->scanned.compare_exchange_strong (nope, true))
	return nullptr;
    }
  if (per_cu == m_per_cu)
    return reader;

  cutu_reader *result = m_index_storage->get_reader (per_cu);
  if (result == nullptr)
    {
      cutu_reader new_reader (per_cu, per_objfile, nullptr, nullptr, false,
			      m_index_storage->get_abbrev_cache ());

      prepare_one_comp_unit (new_reader.cu, new_reader.comp_unit_die,
			     language_minimal);
      std::unique_ptr<cutu_reader> copy
	(new cutu_reader (std::move (new_reader)));
      result = m_index_storage->preserve (std::move (copy));
    }

  if (result->dummy_p || !result->comp_unit_die->has_children)
    return nullptr;

  if (for_scanning)
    check_bounds (result);

  return result;
}

const gdb_byte *
cooked_indexer::scan_attributes (dwarf2_per_cu_data *scanning_per_cu,
				 cutu_reader *reader,
				 const gdb_byte *watermark_ptr,
				 const gdb_byte *info_ptr,
				 const abbrev_info *abbrev,
				 const char **name,
				 const char **linkage_name,
				 cooked_index_flag *flags,
				 sect_offset *sibling_offset,
				 const cooked_index_entry **parent_entry,
				 CORE_ADDR *maybe_defer,
				 bool for_specification)
{
  bool origin_is_dwz = false;
  bool is_declaration = false;
  sect_offset origin_offset {};

  std::optional<unrelocated_addr> low_pc;
  std::optional<unrelocated_addr> high_pc;
  bool high_pc_relative = false;

  for (int i = 0; i < abbrev->num_attrs; ++i)
    {
      attribute attr;
      info_ptr = read_attribute (reader, &attr, &abbrev->attrs[i], info_ptr);

      /* Store the data if it is of an attribute we want to keep in a
	 partial symbol table.  */
      switch (attr.name)
	{
	case DW_AT_name:
	  switch (abbrev->tag)
	    {
	    case DW_TAG_compile_unit:
	    case DW_TAG_partial_unit:
	    case DW_TAG_type_unit:
	      /* Compilation units have a DW_AT_name that is a filename, not
		 a source language identifier.  */
	      break;

	    default:
	      if (*name == nullptr)
		*name = attr.as_string ();
	      break;
	    }
	  break;

	case DW_AT_linkage_name:
	case DW_AT_MIPS_linkage_name:
	  /* Note that both forms of linkage name might appear.  We
	     assume they will be the same, and we only store the last
	     one we see.  */
	  if (*linkage_name == nullptr)
	    *linkage_name = attr.as_string ();
	  break;

	/* DWARF 4 has defined a dedicated DW_AT_main_subprogram
	   attribute to indicate the starting function of the program...  */
	case DW_AT_main_subprogram:
	  if (attr.as_boolean ())
	    *flags |= IS_MAIN;
	  break;

	/* ... however with older versions the DW_CC_program value of
	   the DW_AT_calling_convention attribute was used instead as
	   the only means available.  We handle both variants then.  */
	case DW_AT_calling_convention:
	  if (attr.constant_value (DW_CC_normal) == DW_CC_program)
	    *flags |= IS_MAIN;
	  break;

	case DW_AT_declaration:
	  is_declaration = attr.as_boolean ();
	  break;

	case DW_AT_sibling:
	  if (sibling_offset != nullptr)
	    *sibling_offset = attr.get_ref_die_offset ();
	  break;

	case DW_AT_specification:
	case DW_AT_abstract_origin:
	case DW_AT_extension:
	  origin_offset = attr.get_ref_die_offset ();
	  origin_is_dwz = attr.form == DW_FORM_GNU_ref_alt;
	  break;

	case DW_AT_external:
	  if (attr.as_boolean ())
	    *flags &= ~IS_STATIC;
	  break;

	case DW_AT_enum_class:
	  if (attr.as_boolean ())
	    *flags |= IS_ENUM_CLASS;
	  break;

	case DW_AT_low_pc:
	  low_pc = attr.as_address ();
	  break;

	case DW_AT_high_pc:
	  high_pc = attr.as_address ();
	  if (reader->cu->header.version >= 4 && attr.form_is_constant ())
	    high_pc_relative = true;
	  break;

	case DW_AT_location:
	  if (!scanning_per_cu->addresses_seen && attr.form_is_block ())
	    {
	      struct dwarf_block *locdesc = attr.as_block ();
	      CORE_ADDR addr;
	      if (decode_locdesc (locdesc, reader->cu, &addr)
		  && (addr != 0
		      || reader->cu->per_objfile->per_bfd->has_section_at_zero))
		{
		  low_pc = (unrelocated_addr) addr;
		  /* For variables, we don't want to try decoding the
		     type just to find the size -- for gdb's purposes
		     we only need the address of a variable.  */
		  high_pc = (unrelocated_addr) (addr + 1);
		  high_pc_relative = false;
		}
	    }
	  break;

	case DW_AT_ranges:
	  if (!scanning_per_cu->addresses_seen)
	    {
	      /* Offset in the .debug_ranges or .debug_rnglist section
		 (depending on DWARF version).  */
	      ULONGEST ranges_offset = attr.as_unsigned ();

	      /* See dwarf2_cu::gnu_ranges_base's doc for why we might
		 want to add this value.  */
	      ranges_offset += reader->cu->gnu_ranges_base;

	      unrelocated_addr lowpc, highpc;
	      dwarf2_ranges_read (ranges_offset, &lowpc, &highpc, reader->cu,
				  m_index_storage->get_addrmap (),
				  scanning_per_cu, abbrev->tag);
	    }
	  break;
	}
    }

  /* We don't want to examine declarations, but if we found a
     declaration when handling DW_AT_specification or the like, then
     that is ok.  Similarly, we allow an external variable without a
     location; those are resolved via minimal symbols.  */
  if (is_declaration && !for_specification
      && !(abbrev->tag == DW_TAG_variable && (*flags & IS_STATIC) == 0))
    {
      /* We always want to recurse into some types, but we may not
	 want to treat them as definitions.  */
      if ((abbrev->tag == DW_TAG_class_type
	   || abbrev->tag == DW_TAG_structure_type
	   || abbrev->tag == DW_TAG_union_type)
	  && abbrev->has_children)
	*flags |= IS_TYPE_DECLARATION;
      else
	{
	  *linkage_name = nullptr;
	  *name = nullptr;
	}
    }
  else if ((*name == nullptr
	    || (*linkage_name == nullptr
		&& tag_can_have_linkage_name (abbrev->tag))
	    || (*parent_entry == nullptr && m_language != language_c))
	   && origin_offset != sect_offset (0))
    {
      cutu_reader *new_reader
	= ensure_cu_exists (reader, reader->cu->per_objfile, origin_offset,
			    origin_is_dwz, false);
      if (new_reader != nullptr)
	{
	  const gdb_byte *new_info_ptr = (new_reader->buffer
					  + to_underlying (origin_offset));

	  if (new_reader->cu == reader->cu
	      && new_info_ptr > watermark_ptr
	      && *parent_entry == nullptr)
	    *maybe_defer = form_addr (origin_offset, origin_is_dwz);
	  else if (*parent_entry == nullptr)
	    {
	      CORE_ADDR lookup = form_addr (origin_offset, origin_is_dwz);
	      void *obj = m_die_range_map.find (lookup);
	      *parent_entry = static_cast <cooked_index_entry *> (obj);
	    }

	  unsigned int bytes_read;
	  const abbrev_info *new_abbrev = peek_die_abbrev (*new_reader,
							   new_info_ptr,
							   &bytes_read);
	  new_info_ptr += bytes_read;

	  if (new_reader->cu == reader->cu && new_info_ptr == watermark_ptr)
	    {
	      /* Self-reference, we're done.  */
	    }
	  else
	    scan_attributes (scanning_per_cu, new_reader, new_info_ptr,
			     new_info_ptr, new_abbrev, name, linkage_name,
			     flags, nullptr, parent_entry, maybe_defer, true);
	}
    }

  if (!for_specification)
    {
      if (m_language == language_ada
	  && *linkage_name == nullptr)
	*linkage_name = *name;

      if (!scanning_per_cu->addresses_seen
	  && low_pc.has_value ()
	  && (reader->cu->per_objfile->per_bfd->has_section_at_zero
	      || *low_pc != (unrelocated_addr) 0)
	  && high_pc.has_value ())
	{
	  if (high_pc_relative)
	    high_pc = (unrelocated_addr) ((ULONGEST) *high_pc
					  + (ULONGEST) *low_pc);

	  if (*high_pc > *low_pc)
	    {
	      dwarf2_per_objfile *per_objfile = reader->cu->per_objfile;
	      unrelocated_addr lo = per_objfile->adjust (*low_pc);
	      unrelocated_addr hi = per_objfile->adjust (*high_pc);
	      /* Need CORE_ADDR casts for addrmap.  */
	      m_index_storage->get_addrmap ()->set_empty ((CORE_ADDR) lo,
							  (CORE_ADDR) hi - 1,
							  scanning_per_cu);
	    }
	}

      if (abbrev->tag == DW_TAG_module || abbrev->tag == DW_TAG_namespace)
	*flags &= ~IS_STATIC;

      if (abbrev->tag == DW_TAG_namespace && *name == nullptr)
	*name = "(anonymous namespace)";

      if (m_language == language_cplus
	  && (abbrev->tag == DW_TAG_class_type
	      || abbrev->tag == DW_TAG_interface_type
	      || abbrev->tag == DW_TAG_structure_type
	      || abbrev->tag == DW_TAG_union_type
	      || abbrev->tag == DW_TAG_enumeration_type
	      || abbrev->tag == DW_TAG_enumerator))
	*flags &= ~IS_STATIC;
    }

  return info_ptr;
}

const gdb_byte *
cooked_indexer::index_imported_unit (cutu_reader *reader,
				     const gdb_byte *info_ptr,
				     const abbrev_info *abbrev)
{
  sect_offset sect_off {};
  bool is_dwz = false;

  for (int i = 0; i < abbrev->num_attrs; ++i)
    {
      /* Note that we never need to reprocess attributes here.  */
      attribute attr;
      info_ptr = read_attribute (reader, &attr, &abbrev->attrs[i], info_ptr);

      if (attr.name == DW_AT_import)
	{
	  sect_off = attr.get_ref_die_offset ();
	  is_dwz = (attr.form == DW_FORM_GNU_ref_alt
		    || reader->cu->per_cu->is_dwz);
	}
    }

  /* Did not find DW_AT_import.  */
  if (sect_off == sect_offset (0))
    return info_ptr;

  dwarf2_per_objfile *per_objfile = reader->cu->per_objfile;
  cutu_reader *new_reader = ensure_cu_exists (reader, per_objfile, sect_off,
					      is_dwz, true);
  if (new_reader != nullptr)
    {
      index_dies (new_reader, new_reader->info_ptr, nullptr, false);

      reader->cu->add_dependence (new_reader->cu->per_cu);
    }

  return info_ptr;
}

const gdb_byte *
cooked_indexer::recurse (cutu_reader *reader,
			 const gdb_byte *info_ptr,
			 const cooked_index_entry *parent_entry,
			 bool fully)
{
  info_ptr = index_dies (reader, info_ptr, parent_entry, fully);

  if (parent_entry != nullptr)
    {
      /* Both start and end are inclusive, so use both "+ 1" and "- 1" to
	 limit the range to the children of parent_entry.  */
      CORE_ADDR start = form_addr (parent_entry->die_offset + 1,
				   reader->cu->per_cu->is_dwz);
      CORE_ADDR end = form_addr (sect_offset (info_ptr - 1 - reader->buffer),
				 reader->cu->per_cu->is_dwz);
      m_die_range_map.set_empty (start, end, (void *) parent_entry);
    }

  return info_ptr;
}

const gdb_byte *
cooked_indexer::index_dies (cutu_reader *reader,
			    const gdb_byte *info_ptr,
			    const cooked_index_entry *parent_entry,
			    bool fully)
{
  const gdb_byte *end_ptr = (reader->buffer
			     + to_underlying (reader->cu->header.sect_off)
			     + reader->cu->header.get_length_with_initial ());

  while (info_ptr < end_ptr)
    {
      sect_offset this_die = (sect_offset) (info_ptr - reader->buffer);
      unsigned int bytes_read;
      const abbrev_info *abbrev = peek_die_abbrev (*reader, info_ptr,
						   &bytes_read);
      info_ptr += bytes_read;
      if (abbrev == nullptr)
	break;

      if (abbrev->tag == DW_TAG_imported_unit)
	{
	  info_ptr = index_imported_unit (reader, info_ptr, abbrev);
	  continue;
	}

      if (!abbrev->interesting)
	{
	  info_ptr = skip_one_die (reader, info_ptr, abbrev, !fully);
	  if (fully && abbrev->has_children)
	    info_ptr = index_dies (reader, info_ptr, parent_entry, fully);
	  continue;
	}

      const char *name = nullptr;
      const char *linkage_name = nullptr;
      CORE_ADDR defer = 0;
      cooked_index_flag flags = IS_STATIC;
      sect_offset sibling {};
      const cooked_index_entry *this_parent_entry = parent_entry;

      /* The scope of a DW_TAG_entry_point cooked_index_entry is the one of
	 its surrounding subroutine.  */
      if (abbrev->tag == DW_TAG_entry_point)
	this_parent_entry = parent_entry->get_parent ();
      info_ptr = scan_attributes (reader->cu->per_cu, reader, info_ptr,
				  info_ptr, abbrev, &name, &linkage_name,
				  &flags, &sibling, &this_parent_entry,
				  &defer, false);

      if (abbrev->tag == DW_TAG_namespace
	  && m_language == language_cplus
	  && strcmp (name, "::") == 0)
	{
	  /* GCC 4.0 and 4.1 had a bug (PR c++/28460) where they
	     generated bogus DW_TAG_namespace DIEs with a name of "::"
	     for the global namespace.  Work around this problem
	     here.  */
	  name = nullptr;
	}

      cooked_index_entry *this_entry = nullptr;
      if (name != nullptr)
	{
	  if (defer != 0)
	    {
	      this_entry
		= m_index_storage->add (this_die, abbrev->tag,
					flags | IS_PARENT_DEFERRED, name,
					defer, m_per_cu);
	      m_deferred_entries.push_back (this_entry);
	    }
	  else
	    this_entry
	      = m_index_storage->add (this_die, abbrev->tag, flags, name,
				      this_parent_entry, m_per_cu);
	}

      if (linkage_name != nullptr)
	{
	  /* We only want this to be "main" if it has a linkage name
	     but not an ordinary name.  */
	  if (name != nullptr)
	    flags = flags & ~IS_MAIN;
	  /* Set the IS_LINKAGE on for everything except when functions
	     have linkage name present but name is absent.  */
	  if (name != nullptr
	      || (abbrev->tag != DW_TAG_subprogram
		  && abbrev->tag != DW_TAG_inlined_subroutine
		  && abbrev->tag != DW_TAG_entry_point))
	    flags = flags | IS_LINKAGE;
	  m_index_storage->add (this_die, abbrev->tag, flags,
				linkage_name, nullptr, m_per_cu);
	}

      if (abbrev->has_children)
	{
	  switch (abbrev->tag)
	    {
	    case DW_TAG_class_type:
	    case DW_TAG_interface_type:
	    case DW_TAG_structure_type:
	    case DW_TAG_union_type:
	      if (m_language != language_c && this_entry != nullptr)
		{
		  info_ptr = recurse (reader, info_ptr, this_entry, fully);
		  continue;
		}
	      break;

	    case DW_TAG_enumeration_type:
	      /* We need to recurse even for an anonymous enumeration.
		 Which scope we record as the parent scope depends on
		 whether we're reading an "enum class".  If so, we use
		 the enum itself as the parent, yielding names like
		 "enum_class::enumerator"; otherwise we inject the
		 names into our own parent scope.  */
	      info_ptr = recurse (reader, info_ptr,
				  ((flags & IS_ENUM_CLASS) == 0)
				  ? parent_entry
				  : this_entry,
				  fully);
	      continue;

	    case DW_TAG_module:
	      if (this_entry == nullptr)
		break;
	      [[fallthrough]];
	    case DW_TAG_namespace:
	      /* We don't check THIS_ENTRY for a namespace, to handle
		 the ancient G++ workaround pointed out above.  */
	      info_ptr = recurse (reader, info_ptr, this_entry, fully);
	      continue;

	    case DW_TAG_subprogram:
	      if ((m_language == language_fortran
		   || m_language == language_ada)
		  && this_entry != nullptr)
		{
		  info_ptr = recurse (reader, info_ptr, this_entry, true);
		  continue;
		}
	      break;
	    }

	  if (sibling != sect_offset (0))
	    {
	      const gdb_byte *sibling_ptr
		= reader->buffer + to_underlying (sibling);

	      if (sibling_ptr < info_ptr)
		complaint (_("DW_AT_sibling points backwards"));
	      else if (sibling_ptr > reader->buffer_end)
		reader->die_section->overflow_complaint ();
	      else
		info_ptr = sibling_ptr;
	    }
	  else
	    info_ptr = skip_children (reader, info_ptr);
	}
    }

  return info_ptr;
}

void
cooked_indexer::make_index (cutu_reader *reader)
{
  check_bounds (reader);
  find_file_and_directory (reader->comp_unit_die, reader->cu);
  if (!reader->comp_unit_die->has_children)
    return;
  index_dies (reader, reader->info_ptr, nullptr, false);

  for (const auto &entry : m_deferred_entries)
    {
      void *obj = m_die_range_map.find (entry->get_deferred_parent ());
      cooked_index_entry *parent = static_cast<cooked_index_entry *> (obj);
      entry->resolve_parent (parent);
    }
}

/* An implementation of quick_symbol_functions for the cooked DWARF
   index.  */

struct cooked_index_functions : public dwarf2_base_index_functions
{
  cooked_index *wait (struct objfile *objfile, bool allow_quit)
  {
    dwarf2_per_objfile *per_objfile = get_dwarf2_per_objfile (objfile);
    cooked_index *table
      = (gdb::checked_static_cast<cooked_index *>
	 (per_objfile->per_bfd->index_table.get ()));
    table->wait (cooked_state::MAIN_AVAILABLE, allow_quit);
    return table;
  }

  dwarf2_per_cu_data *find_per_cu (dwarf2_per_bfd *per_bfd,
				   unrelocated_addr adjusted_pc) override;

  struct compunit_symtab *find_compunit_symtab_by_address
    (struct objfile *objfile, CORE_ADDR address) override;

  bool has_unexpanded_symtabs (struct objfile *objfile) override
  {
    wait (objfile, true);
    return dwarf2_base_index_functions::has_unexpanded_symtabs (objfile);
  }

  struct symtab *find_last_source_symtab (struct objfile *objfile) override
  {
    wait (objfile, true);
    return dwarf2_base_index_functions::find_last_source_symtab (objfile);
  }

  void forget_cached_source_info (struct objfile *objfile) override
  {
    wait (objfile, true);
    dwarf2_base_index_functions::forget_cached_source_info (objfile);
  }

  void print_stats (struct objfile *objfile, bool print_bcache) override
  {
    wait (objfile, true);
    dwarf2_base_index_functions::print_stats (objfile, print_bcache);
  }

  void dump (struct objfile *objfile) override
  {
    cooked_index *index = wait (objfile, true);
    gdb_printf ("Cooked index in use:\n");
    gdb_printf ("\n");
    index->dump (objfile->arch ());
  }

  void expand_all_symtabs (struct objfile *objfile) override
  {
    wait (objfile, true);
    dwarf2_base_index_functions::expand_all_symtabs (objfile);
  }

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
     CORE_ADDR pc, struct obj_section *section, int warn_if_readin) override
  {
    wait (objfile, true);
    return (dwarf2_base_index_functions::find_pc_sect_compunit_symtab
	    (objfile, msymbol, pc, section, warn_if_readin));
  }

  void map_symbol_filenames
       (struct objfile *objfile,
	gdb::function_view<symbol_filename_ftype> fun,
	bool need_fullname) override
  {
    wait (objfile, true);
    return (dwarf2_base_index_functions::map_symbol_filenames
	    (objfile, fun, need_fullname));
  }

  void compute_main_name (struct objfile *objfile) override
  {
    wait (objfile, false);
  }
};

dwarf2_per_cu_data *
cooked_index_functions::find_per_cu (dwarf2_per_bfd *per_bfd,
				     unrelocated_addr adjusted_pc)
{
  cooked_index *table
    = (gdb::checked_static_cast<cooked_index *>
       (per_bfd->index_table.get ()));
  return table->lookup (adjusted_pc);
}

struct compunit_symtab *
cooked_index_functions::find_compunit_symtab_by_address
     (struct objfile *objfile, CORE_ADDR address)
{
  if (objfile->sect_index_data == -1)
    return nullptr;

  dwarf2_per_objfile *per_objfile = get_dwarf2_per_objfile (objfile);
  cooked_index *table = wait (objfile, true);

  CORE_ADDR baseaddr = objfile->data_section_offset ();
  dwarf2_per_cu_data *per_cu
    = table->lookup ((unrelocated_addr) (address - baseaddr));
  if (per_cu == nullptr)
    return nullptr;

  return dw2_instantiate_symtab (per_cu, per_objfile, false);
}

bool
cooked_index_functions::expand_symtabs_matching
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

  cooked_index *table = wait (objfile, true);

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

  lookup_name_info lookup_name_without_params
    = lookup_name->make_ignore_params ();
  bool completing = lookup_name->completion_mode ();

  /* Unique styles of language splitting.  */
  static const enum language unique_styles[] =
  {
    /* No splitting is also a style.  */
    language_c,
    /* This includes Rust.  */
    language_cplus,
    /* This includes Go.  */
    language_d,
    language_ada
  };

  for (enum language lang : unique_styles)
    {
      std::vector<std::string_view> name_vec
	= lookup_name_without_params.split_name (lang);
      std::string last_name (name_vec.back ());

      for (const cooked_index_entry *entry : table->find (last_name,
							  completing))
	{
	  QUIT;

	  /* No need to consider symbols from expanded CUs.  */
	  if (per_objfile->symtab_set_p (entry->per_cu))
	    continue;

	  /* If file-matching was done, we don't need to consider
	     symbols from unmarked CUs.  */
	  if (file_matcher != nullptr && !entry->per_cu->mark)
	    continue;

	  /* See if the symbol matches the type filter.  */
	  if (!entry->matches (search_flags)
	      || !entry->matches (domain)
	      || !entry->matches (kind))
	    continue;

	  /* We've found the base name of the symbol; now walk its
	     parentage chain, ensuring that each component
	     matches.  */
	  bool found = true;

	  const cooked_index_entry *parent = entry->get_parent ();
	  for (int i = name_vec.size () - 1; i > 0; --i)
	    {
	      /* If we ran out of entries, or if this segment doesn't
		 match, this did not match.  */
	      if (parent == nullptr
		  || strncmp (parent->name, name_vec[i - 1].data (),
			      name_vec[i - 1].length ()) != 0)
		{
		  found = false;
		  break;
		}

	      parent = parent->get_parent ();
	    }

	  if (!found)
	    continue;

	  /* Might have been looking for "a::b" and found
	     "x::a::b".  */
	  if (symbol_matcher == nullptr)
	    {
	      symbol_name_match_type match_type
		= lookup_name_without_params.match_type ();
	      if ((match_type == symbol_name_match_type::FULL
		   || (lang != language_ada
		       && match_type == symbol_name_match_type::EXPRESSION))
		  && parent != nullptr)
		continue;
	    }
	  else
	    {
	      auto_obstack temp_storage;
	      const char *full_name = entry->full_name (&temp_storage);
	      if (!symbol_matcher (full_name))
		continue;
	    }

	  if (!dw2_expand_symtabs_matching_one (entry->per_cu, per_objfile,
						file_matcher,
						expansion_notify))
	    return false;
	}
    }

  return true;
}

/* Return a new cooked_index_functions object.  */

static quick_symbol_functions_up
make_cooked_index_funcs (dwarf2_per_objfile *per_objfile)
{
  /* Set the index table early so that sharing works even while
     scanning; and then start the scanning.  */
  dwarf2_per_bfd *per_bfd = per_objfile->per_bfd;
  cooked_index *idx = new cooked_index (per_objfile);
  per_bfd->index_table.reset (idx);
  /* Don't start reading until after 'index_table' is set.  This
     avoids races.  */
  idx->start_reading ();

  if (dwarf_synchronous)
    idx->wait_completely ();

  return quick_symbol_functions_up (new cooked_index_functions);
}

quick_symbol_functions_up
cooked_index::make_quick_functions () const
{
  return quick_symbol_functions_up (new cooked_index_functions);
}



/* Read the .debug_loclists or .debug_rnglists header (they are the same format)
   contents from the given SECTION in the HEADER.

   HEADER_OFFSET is the offset of the header in the section.  */
static void
read_loclists_rnglists_header (struct loclists_rnglists_header *header,
			       struct dwarf2_section_info *section,
			       sect_offset header_offset)
{
  unsigned int bytes_read;
  bfd *abfd = section->get_bfd_owner ();
  const gdb_byte *info_ptr = section->buffer + to_underlying (header_offset);

  header->length = read_initial_length (abfd, info_ptr, &bytes_read);
  info_ptr += bytes_read;

  header->version = read_2_bytes (abfd, info_ptr);
  info_ptr += 2;

  header->addr_size = read_1_byte (abfd, info_ptr);
  info_ptr += 1;

  header->segment_collector_size = read_1_byte (abfd, info_ptr);
  info_ptr += 1;

  header->offset_entry_count = read_4_bytes (abfd, info_ptr);
}

/* Return the DW_AT_loclists_base value for the CU.  */
static ULONGEST
lookup_loclist_base (struct dwarf2_cu *cu)
{
  /* For the .dwo unit, the loclist_base points to the first offset following
     the header. The header consists of the following entities-
     1. Unit Length (4 bytes for 32 bit DWARF format, and 12 bytes for the 64
	 bit format)
     2. version (2 bytes)
     3. address size (1 byte)
     4. segment selector size (1 byte)
     5. offset entry count (4 bytes)
     These sizes are derived as per the DWARFv5 standard.  */
  if (cu->dwo_unit != nullptr)
    {
      if (cu->header.initial_length_size == 4)
	 return LOCLIST_HEADER_SIZE32;
      return LOCLIST_HEADER_SIZE64;
    }
  return cu->loclist_base;
}

/* Given a DW_FORM_loclistx value LOCLIST_INDEX, fetch the offset from the
   array of offsets in the .debug_loclists section.  */

static sect_offset
read_loclist_index (struct dwarf2_cu *cu, ULONGEST loclist_index)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  bfd *abfd = objfile->obfd.get ();
  ULONGEST loclist_header_size =
    (cu->header.initial_length_size == 4 ? LOCLIST_HEADER_SIZE32
     : LOCLIST_HEADER_SIZE64);
  ULONGEST loclist_base = lookup_loclist_base (cu);

  /* Offset in .debug_loclists of the offset for LOCLIST_INDEX.  */
  ULONGEST start_offset =
    loclist_base + loclist_index * cu->header.offset_size;

  /* Get loclists section.  */
  struct dwarf2_section_info *section = cu_debug_loc_section (cu);

  /* Read the loclists section content.  */
  section->read (objfile);
  if (section->buffer == NULL)
    error (_("DW_FORM_loclistx used without .debug_loclists "
	     "section [in module %s]"), objfile_name (objfile));

  /* DW_AT_loclists_base points after the .debug_loclists contribution header,
     so if loclist_base is smaller than the header size, we have a problem.  */
  if (loclist_base < loclist_header_size)
    error (_("DW_AT_loclists_base is smaller than header size [in module %s]"),
	   objfile_name (objfile));

  /* Read the header of the loclists contribution.  */
  struct loclists_rnglists_header header;
  read_loclists_rnglists_header (&header, section,
				 (sect_offset) (loclist_base - loclist_header_size));

  /* Verify the loclist index is valid.  */
  if (loclist_index >= header.offset_entry_count)
    error (_("DW_FORM_loclistx pointing outside of "
	     ".debug_loclists offset array [in module %s]"),
	   objfile_name (objfile));

  /* Validate that reading won't go beyond the end of the section.  */
  if (start_offset + cu->header.offset_size > section->size)
    error (_("Reading DW_FORM_loclistx index beyond end of"
	     ".debug_loclists section [in module %s]"),
	   objfile_name (objfile));

  const gdb_byte *info_ptr = section->buffer + start_offset;

  if (cu->header.offset_size == 4)
    return (sect_offset) (bfd_get_32 (abfd, info_ptr) + loclist_base);
  else
    return (sect_offset) (bfd_get_64 (abfd, info_ptr) + loclist_base);
}

/* Given a DW_FORM_rnglistx value RNGLIST_INDEX, fetch the offset from the
   array of offsets in the .debug_rnglists section.  */

static sect_offset
read_rnglist_index (struct dwarf2_cu *cu, ULONGEST rnglist_index,
		    dwarf_tag tag)
{
  struct dwarf2_per_objfile *dwarf2_per_objfile = cu->per_objfile;
  struct objfile *objfile = dwarf2_per_objfile->objfile;
  bfd *abfd = objfile->obfd.get ();
  ULONGEST rnglist_header_size =
    (cu->header.initial_length_size == 4 ? RNGLIST_HEADER_SIZE32
     : RNGLIST_HEADER_SIZE64);

  /* When reading a DW_FORM_rnglistx from a DWO, we read from the DWO's
     .debug_rnglists.dwo section.  The rnglists base given in the skeleton
     doesn't apply.  */
  ULONGEST rnglist_base =
      (cu->dwo_unit != nullptr) ? rnglist_header_size : cu->rnglists_base;

  /* Offset in .debug_rnglists of the offset for RNGLIST_INDEX.  */
  ULONGEST start_offset =
    rnglist_base + rnglist_index * cu->header.offset_size;

  /* Get rnglists section.  */
  struct dwarf2_section_info *section = cu_debug_rnglists_section (cu, tag);

  /* Read the rnglists section content.  */
  section->read (objfile);
  if (section->buffer == nullptr)
    error (_("DW_FORM_rnglistx used without .debug_rnglists section "
	     "[in module %s]"),
	   objfile_name (objfile));

  /* DW_AT_rnglists_base points after the .debug_rnglists contribution header,
     so if rnglist_base is smaller than the header size, we have a problem.  */
  if (rnglist_base < rnglist_header_size)
    error (_("DW_AT_rnglists_base is smaller than header size [in module %s]"),
	   objfile_name (objfile));

  /* Read the header of the rnglists contribution.  */
  struct loclists_rnglists_header header;
  read_loclists_rnglists_header (&header, section,
				 (sect_offset) (rnglist_base - rnglist_header_size));

  /* Verify the rnglist index is valid.  */
  if (rnglist_index >= header.offset_entry_count)
    error (_("DW_FORM_rnglistx index pointing outside of "
	     ".debug_rnglists offset array [in module %s]"),
	   objfile_name (objfile));

  /* Validate that reading won't go beyond the end of the section.  */
  if (start_offset + cu->header.offset_size > section->size)
    error (_("Reading DW_FORM_rnglistx index beyond end of"
	     ".debug_rnglists section [in module %s]"),
	   objfile_name (objfile));

  const gdb_byte *info_ptr = section->buffer + start_offset;

  if (cu->header.offset_size == 4)
    return (sect_offset) (read_4_bytes (abfd, info_ptr) + rnglist_base);
  else
    return (sect_offset) (read_8_bytes (abfd, info_ptr) + rnglist_base);
}

/* Process the attributes that had to be skipped in the first round. These
   attributes are the ones that need str_offsets_base or addr_base attributes.
   They could not have been processed in the first round, because at the time
   the values of str_offsets_base or addr_base may not have been known.  */
static void
read_attribute_reprocess (const struct die_reader_specs *reader,
			  struct attribute *attr, dwarf_tag tag)
{
  struct dwarf2_cu *cu = reader->cu;
  switch (attr->form)
    {
      case DW_FORM_addrx:
      case DW_FORM_GNU_addr_index:
	attr->set_address (read_addr_index (cu,
					    attr->as_unsigned_reprocess ()));
	break;
      case DW_FORM_loclistx:
	{
	  sect_offset loclists_sect_off
	    = read_loclist_index (cu, attr->as_unsigned_reprocess ());

	  attr->set_unsigned (to_underlying (loclists_sect_off));
	}
	break;
      case DW_FORM_rnglistx:
	{
	  sect_offset rnglists_sect_off
	    = read_rnglist_index (cu, attr->as_unsigned_reprocess (), tag);

	  attr->set_unsigned (to_underlying (rnglists_sect_off));
	}
	break;
      case DW_FORM_strx:
      case DW_FORM_strx1:
      case DW_FORM_strx2:
      case DW_FORM_strx3:
      case DW_FORM_strx4:
      case DW_FORM_GNU_str_index:
	{
	  unsigned int str_index = attr->as_unsigned_reprocess ();
	  gdb_assert (!attr->canonical_string_p ());
	  if (reader->dwo_file != NULL)
	    attr->set_string_noncanonical (read_dwo_str_index (reader,
							       str_index));
	  else
	    attr->set_string_noncanonical (read_stub_str_index (cu,
								str_index));
	  break;
	}
      default:
	gdb_assert_not_reached ("Unexpected DWARF form.");
    }
}

/* Read an attribute value described by an attribute form.  */

static const gdb_byte *
read_attribute_value (const struct die_reader_specs *reader,
		      struct attribute *attr, unsigned form,
		      LONGEST implicit_const, const gdb_byte *info_ptr,
		      bool allow_reprocess)
{
  struct dwarf2_cu *cu = reader->cu;
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  bfd *abfd = reader->abfd;
  struct comp_unit_head *cu_header = &cu->header;
  unsigned int bytes_read;
  struct dwarf_block *blk;

  attr->form = (enum dwarf_form) form;
  switch (form)
    {
    case DW_FORM_ref_addr:
      if (cu_header->version == 2)
	attr->set_unsigned ((ULONGEST) cu_header->read_address (abfd, info_ptr,
								&bytes_read));
      else
	attr->set_unsigned (cu_header->read_offset (abfd, info_ptr,
						    &bytes_read));
      info_ptr += bytes_read;
      break;
    case DW_FORM_GNU_ref_alt:
      attr->set_unsigned (cu_header->read_offset (abfd, info_ptr,
						  &bytes_read));
      info_ptr += bytes_read;
      break;
    case DW_FORM_addr:
      {
	unrelocated_addr addr = cu_header->read_address (abfd, info_ptr,
							 &bytes_read);
	addr = per_objfile->adjust (addr);
	attr->set_address (addr);
	info_ptr += bytes_read;
      }
      break;
    case DW_FORM_block2:
      blk = dwarf_alloc_block (cu);
      blk->size = read_2_bytes (abfd, info_ptr);
      info_ptr += 2;
      blk->data = read_n_bytes (abfd, info_ptr, blk->size);
      info_ptr += blk->size;
      attr->set_block (blk);
      break;
    case DW_FORM_block4:
      blk = dwarf_alloc_block (cu);
      blk->size = read_4_bytes (abfd, info_ptr);
      info_ptr += 4;
      blk->data = read_n_bytes (abfd, info_ptr, blk->size);
      info_ptr += blk->size;
      attr->set_block (blk);
      break;
    case DW_FORM_data2:
      attr->set_unsigned (read_2_bytes (abfd, info_ptr));
      info_ptr += 2;
      break;
    case DW_FORM_data4:
      attr->set_unsigned (read_4_bytes (abfd, info_ptr));
      info_ptr += 4;
      break;
    case DW_FORM_data8:
      attr->set_unsigned (read_8_bytes (abfd, info_ptr));
      info_ptr += 8;
      break;
    case DW_FORM_data16:
      blk = dwarf_alloc_block (cu);
      blk->size = 16;
      blk->data = read_n_bytes (abfd, info_ptr, 16);
      info_ptr += 16;
      attr->set_block (blk);
      break;
    case DW_FORM_sec_offset:
      attr->set_unsigned (cu_header->read_offset (abfd, info_ptr,
						  &bytes_read));
      info_ptr += bytes_read;
      break;
    case DW_FORM_loclistx:
      {
	attr->set_unsigned_reprocess (read_unsigned_leb128 (abfd, info_ptr,
							    &bytes_read));
	info_ptr += bytes_read;
	if (allow_reprocess)
	  read_attribute_reprocess (reader, attr);
      }
      break;
    case DW_FORM_string:
      attr->set_string_noncanonical (read_direct_string (abfd, info_ptr,
							 &bytes_read));
      info_ptr += bytes_read;
      break;
    case DW_FORM_strp:
      if (!cu->per_cu->is_dwz)
	{
	  attr->set_string_noncanonical
	    (read_indirect_string (per_objfile,
				   abfd, info_ptr, cu_header,
				   &bytes_read));
	  info_ptr += bytes_read;
	  break;
	}
      [[fallthrough]];
    case DW_FORM_line_strp:
      if (!cu->per_cu->is_dwz)
	{
	  attr->set_string_noncanonical
	    (per_objfile->read_line_string (info_ptr, cu_header,
					    &bytes_read));
	  info_ptr += bytes_read;
	  break;
	}
      [[fallthrough]];
    case DW_FORM_GNU_strp_alt:
      {
	dwz_file *dwz = dwarf2_get_dwz_file (per_objfile->per_bfd, true);
	LONGEST str_offset = cu_header->read_offset (abfd, info_ptr,
						     &bytes_read);

	attr->set_string_noncanonical
	  (dwz->read_string (objfile, str_offset));
	info_ptr += bytes_read;
      }
      break;
    case DW_FORM_exprloc:
    case DW_FORM_block:
      blk = dwarf_alloc_block (cu);
      blk->size = read_unsigned_leb128 (abfd, info_ptr, &bytes_read);
      info_ptr += bytes_read;
      blk->data = read_n_bytes (abfd, info_ptr, blk->size);
      info_ptr += blk->size;
      attr->set_block (blk);
      break;
    case DW_FORM_block1:
      blk = dwarf_alloc_block (cu);
      blk->size = read_1_byte (abfd, info_ptr);
      info_ptr += 1;
      blk->data = read_n_bytes (abfd, info_ptr, blk->size);
      info_ptr += blk->size;
      attr->set_block (blk);
      break;
    case DW_FORM_data1:
    case DW_FORM_flag:
      attr->set_unsigned (read_1_byte (abfd, info_ptr));
      info_ptr += 1;
      break;
    case DW_FORM_flag_present:
      attr->set_unsigned (1);
      break;
    case DW_FORM_sdata:
      attr->set_signed (read_signed_leb128 (abfd, info_ptr, &bytes_read));
      info_ptr += bytes_read;
      break;
    case DW_FORM_rnglistx:
      {
	attr->set_unsigned_reprocess (read_unsigned_leb128 (abfd, info_ptr,
							    &bytes_read));
	info_ptr += bytes_read;
	if (allow_reprocess)
	  read_attribute_reprocess (reader, attr);
      }
      break;
    case DW_FORM_udata:
      attr->set_unsigned (read_unsigned_leb128 (abfd, info_ptr, &bytes_read));
      info_ptr += bytes_read;
      break;
    case DW_FORM_ref1:
      attr->set_unsigned ((to_underlying (cu_header->sect_off)
			   + read_1_byte (abfd, info_ptr)));
      info_ptr += 1;
      break;
    case DW_FORM_ref2:
      attr->set_unsigned ((to_underlying (cu_header->sect_off)
			   + read_2_bytes (abfd, info_ptr)));
      info_ptr += 2;
      break;
    case DW_FORM_ref4:
      attr->set_unsigned ((to_underlying (cu_header->sect_off)
			   + read_4_bytes (abfd, info_ptr)));
      info_ptr += 4;
      break;
    case DW_FORM_ref8:
      attr->set_unsigned ((to_underlying (cu_header->sect_off)
			   + read_8_bytes (abfd, info_ptr)));
      info_ptr += 8;
      break;
    case DW_FORM_ref_sig8:
      attr->set_signature (read_8_bytes (abfd, info_ptr));
      info_ptr += 8;
      break;
    case DW_FORM_ref_udata:
      attr->set_unsigned ((to_underlying (cu_header->sect_off)
			   + read_unsigned_leb128 (abfd, info_ptr,
						   &bytes_read)));
      info_ptr += bytes_read;
      break;
    case DW_FORM_indirect:
      form = read_unsigned_leb128 (abfd, info_ptr, &bytes_read);
      info_ptr += bytes_read;
      if (form == DW_FORM_implicit_const)
	{
	  implicit_const = read_signed_leb128 (abfd, info_ptr, &bytes_read);
	  info_ptr += bytes_read;
	}
      info_ptr = read_attribute_value (reader, attr, form, implicit_const,
				       info_ptr, allow_reprocess);
      break;
    case DW_FORM_implicit_const:
      attr->set_signed (implicit_const);
      break;
    case DW_FORM_addrx:
    case DW_FORM_GNU_addr_index:
      attr->set_unsigned_reprocess (read_unsigned_leb128 (abfd, info_ptr,
							  &bytes_read));
      info_ptr += bytes_read;
      if (allow_reprocess)
	read_attribute_reprocess (reader, attr);
      break;
    case DW_FORM_strx:
    case DW_FORM_strx1:
    case DW_FORM_strx2:
    case DW_FORM_strx3:
    case DW_FORM_strx4:
    case DW_FORM_GNU_str_index:
      {
	ULONGEST str_index;
	if (form == DW_FORM_strx1)
	  {
	    str_index = read_1_byte (abfd, info_ptr);
	    info_ptr += 1;
	  }
	else if (form == DW_FORM_strx2)
	  {
	    str_index = read_2_bytes (abfd, info_ptr);
	    info_ptr += 2;
	  }
	else if (form == DW_FORM_strx3)
	  {
	    str_index = read_3_bytes (abfd, info_ptr);
	    info_ptr += 3;
	  }
	else if (form == DW_FORM_strx4)
	  {
	    str_index = read_4_bytes (abfd, info_ptr);
	    info_ptr += 4;
	  }
	else
	  {
	    str_index = read_unsigned_leb128 (abfd, info_ptr, &bytes_read);
	    info_ptr += bytes_read;
	  }
	attr->set_unsigned_reprocess (str_index);
	if (allow_reprocess)
	  read_attribute_reprocess (reader, attr);
      }
      break;
    default:
      error (_("Dwarf Error: Cannot handle %s in DWARF reader [in module %s]"),
	     dwarf_form_name (form),
	     bfd_get_filename (abfd));
    }

  /* Super hack.  */
  if (cu->per_cu->is_dwz && attr->form_is_ref ())
    attr->form = DW_FORM_GNU_ref_alt;

  /* We have seen instances where the compiler tried to emit a byte
     size attribute of -1 which ended up being encoded as an unsigned
     0xffffffff.  Although 0xffffffff is technically a valid size value,
     an object of this size seems pretty unlikely so we can relatively
     safely treat these cases as if the size attribute was invalid and
     treat them as zero by default.  */
  if (attr->name == DW_AT_byte_size
      && form == DW_FORM_data4
      && attr->as_unsigned () >= 0xffffffff)
    {
      complaint
	(_("Suspicious DW_AT_byte_size value treated as zero instead of %s"),
	 hex_string (attr->as_unsigned ()));
      attr->set_unsigned (0);
    }

  return info_ptr;
}

/* Read an attribute described by an abbreviated attribute.  */

static const gdb_byte *
read_attribute (const struct die_reader_specs *reader,
		struct attribute *attr, const struct attr_abbrev *abbrev,
		const gdb_byte *info_ptr,
		bool allow_reprocess)
{
  attr->name = abbrev->name;
  attr->string_is_canonical = 0;
  return read_attribute_value (reader, attr, abbrev->form,
			       abbrev->implicit_const, info_ptr,
			       allow_reprocess);
}

/* See read.h.  */

const char *
read_indirect_string_at_offset (dwarf2_per_objfile *per_objfile,
				LONGEST str_offset)
{
  return per_objfile->per_bfd->str.read_string (per_objfile->objfile,
						str_offset, "DW_FORM_strp");
}

/* Return pointer to string at .debug_str offset as read from BUF.
   BUF is assumed to be in a compilation unit described by CU_HEADER.
   Return *BYTES_READ_PTR count of bytes read from BUF.  */

static const char *
read_indirect_string (dwarf2_per_objfile *per_objfile, bfd *abfd,
		      const gdb_byte *buf,
		      const struct comp_unit_head *cu_header,
		      unsigned int *bytes_read_ptr)
{
  LONGEST str_offset = cu_header->read_offset (abfd, buf, bytes_read_ptr);

  return read_indirect_string_at_offset (per_objfile, str_offset);
}

/* See read.h.  */

const char *
dwarf2_per_objfile::read_line_string (const gdb_byte *buf,
				      unsigned int offset_size)
{
  bfd *abfd = objfile->obfd.get ();
  ULONGEST str_offset = read_offset (abfd, buf, offset_size);

  return per_bfd->line_str.read_string (objfile, str_offset, "DW_FORM_line_strp");
}

/* See read.h.  */

const char *
dwarf2_per_objfile::read_line_string (const gdb_byte *buf,
				      const struct comp_unit_head *cu_header,
				      unsigned int *bytes_read_ptr)
{
  bfd *abfd = objfile->obfd.get ();
  LONGEST str_offset = cu_header->read_offset (abfd, buf, bytes_read_ptr);

  return per_bfd->line_str.read_string (objfile, str_offset, "DW_FORM_line_strp");
}

/* Given index ADDR_INDEX in .debug_addr, fetch the value.
   ADDR_BASE is the DW_AT_addr_base (DW_AT_GNU_addr_base) attribute or zero.
   ADDR_SIZE is the size of addresses from the CU header.  */

static unrelocated_addr
read_addr_index_1 (dwarf2_per_objfile *per_objfile, unsigned int addr_index,
		   std::optional<ULONGEST> addr_base, int addr_size)
{
  struct objfile *objfile = per_objfile->objfile;
  bfd *abfd = objfile->obfd.get ();
  const gdb_byte *info_ptr;
  ULONGEST addr_base_or_zero = addr_base.has_value () ? *addr_base : 0;

  per_objfile->per_bfd->addr.read (objfile);
  if (per_objfile->per_bfd->addr.buffer == NULL)
    error (_("DW_FORM_addr_index used without .debug_addr section [in module %s]"),
	   objfile_name (objfile));
  if (addr_base_or_zero + addr_index * addr_size
      >= per_objfile->per_bfd->addr.size)
    error (_("DW_FORM_addr_index pointing outside of "
	     ".debug_addr section [in module %s]"),
	   objfile_name (objfile));
  info_ptr = (per_objfile->per_bfd->addr.buffer + addr_base_or_zero
	      + addr_index * addr_size);
  if (addr_size == 4)
    return (unrelocated_addr) bfd_get_32 (abfd, info_ptr);
  else
    return (unrelocated_addr) bfd_get_64 (abfd, info_ptr);
}

/* Given index ADDR_INDEX in .debug_addr, fetch the value.  */

static unrelocated_addr
read_addr_index (struct dwarf2_cu *cu, unsigned int addr_index)
{
  return read_addr_index_1 (cu->per_objfile, addr_index,
			    cu->addr_base, cu->header.addr_size);
}

/* Given a pointer to an leb128 value, fetch the value from .debug_addr.  */

static unrelocated_addr
read_addr_index_from_leb128 (struct dwarf2_cu *cu, const gdb_byte *info_ptr,
			     unsigned int *bytes_read)
{
  bfd *abfd = cu->per_objfile->objfile->obfd.get ();
  unsigned int addr_index = read_unsigned_leb128 (abfd, info_ptr, bytes_read);

  return read_addr_index (cu, addr_index);
}

/* See read.h.  */

unrelocated_addr
dwarf2_read_addr_index (dwarf2_per_cu_data *per_cu,
			dwarf2_per_objfile *per_objfile,
			unsigned int addr_index)
{
  struct dwarf2_cu *cu = per_objfile->get_cu (per_cu);
  std::optional<ULONGEST> addr_base;
  int addr_size;

  /* We need addr_base and addr_size.
     If we don't have PER_CU->cu, we have to get it.
     Nasty, but the alternative is storing the needed info in PER_CU,
     which at this point doesn't seem justified: it's not clear how frequently
     it would get used and it would increase the size of every PER_CU.
     Entry points like dwarf2_per_cu_addr_size do a similar thing
     so we're not in uncharted territory here.
     Alas we need to be a bit more complicated as addr_base is contained
     in the DIE.

     We don't need to read the entire CU(/TU).
     We just need the header and top level die.

     IWBN to use the aging mechanism to let us lazily later discard the CU.
     For now we skip this optimization.  */

  if (cu != NULL)
    {
      addr_base = cu->addr_base;
      addr_size = cu->header.addr_size;
    }
  else
    {
      cutu_reader reader (per_cu, per_objfile, nullptr, nullptr, false);
      addr_base = reader.cu->addr_base;
      addr_size = reader.cu->header.addr_size;
    }

  return read_addr_index_1 (per_objfile, addr_index, addr_base, addr_size);
}

/* Given a DW_FORM_GNU_str_index value STR_INDEX, fetch the string.
   STR_SECTION, STR_OFFSETS_SECTION can be from a Fission stub or a
   DWO file.  */

static const char *
read_str_index (struct dwarf2_cu *cu,
		struct dwarf2_section_info *str_section,
		struct dwarf2_section_info *str_offsets_section,
		ULONGEST str_offsets_base, ULONGEST str_index,
		unsigned offset_size)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  const char *objf_name = objfile_name (objfile);
  bfd *abfd = objfile->obfd.get ();
  const gdb_byte *info_ptr;
  ULONGEST str_offset;
  static const char form_name[] = "DW_FORM_GNU_str_index or DW_FORM_strx";

  str_section->read (objfile);
  str_offsets_section->read (objfile);
  if (str_section->buffer == NULL)
    error (_("%s used without %s section"
	     " in CU at offset %s [in module %s]"),
	   form_name, str_section->get_name (),
	   sect_offset_str (cu->header.sect_off), objf_name);
  if (str_offsets_section->buffer == NULL)
    error (_("%s used without %s section"
	     " in CU at offset %s [in module %s]"),
	   form_name, str_section->get_name (),
	   sect_offset_str (cu->header.sect_off), objf_name);
  info_ptr = (str_offsets_section->buffer
	      + str_offsets_base
	      + str_index * offset_size);
  if (offset_size == 4)
    str_offset = bfd_get_32 (abfd, info_ptr);
  else
    str_offset = bfd_get_64 (abfd, info_ptr);
  if (str_offset >= str_section->size)
    error (_("Offset from %s pointing outside of"
	     " .debug_str.dwo section in CU at offset %s [in module %s]"),
	   form_name, sect_offset_str (cu->header.sect_off), objf_name);
  return (const char *) (str_section->buffer + str_offset);
}

/* Given a DW_FORM_GNU_str_index from a DWO file, fetch the string.  */

static const char *
read_dwo_str_index (const struct die_reader_specs *reader, ULONGEST str_index)
{
  unsigned offset_size;
  ULONGEST str_offsets_base;
  if (reader->cu->header.version >= 5)
    {
      /* We have a DWARF5 CU with a reference to a .debug_str_offsets section,
	 so assume the .debug_str_offsets section is DWARF5 as well, and
	 parse the header.  FIXME: Parse the header only once.  */
      unsigned int bytes_read = 0;
      bfd *abfd = reader->dwo_file->sections.str_offsets.get_bfd_owner ();
      const gdb_byte *p = reader->dwo_file->sections.str_offsets.buffer;

      /* Header: Initial length.  */
      read_initial_length (abfd, p + bytes_read, &bytes_read);

      /* Determine offset_size based on the .debug_str_offsets header.  */
      const bool dwarf5_is_dwarf64 = bytes_read != 4;
      offset_size = dwarf5_is_dwarf64 ? 8 : 4;

      /* Header: Version.  */
      unsigned version = read_2_bytes (abfd, p + bytes_read);
      bytes_read += 2;

      if (version <= 4)
	{
	  /* We'd like one warning here about ignoring the section, but
	     because we parse the header more than once (see FIXME above)
	     we'd have many warnings, so use a complaint instead, which at
	     least has a limit. */
	  complaint (_("Section .debug_str_offsets in %s has unsupported"
		       " version %d, use empty string."),
		     reader->dwo_file->dwo_name.c_str (), version);
	  return "";
	}

      /* Header: Padding.  */
      bytes_read += 2;

      str_offsets_base = bytes_read;
    }
  else
    {
      /* We have a pre-DWARF5 CU with a reference to a .debug_str_offsets
	 section, assume the .debug_str_offsets section is pre-DWARF5 as
	 well, which doesn't have a header.  */
      str_offsets_base = 0;

      /* Determine offset_size based on the .debug_info header.  */
      offset_size = reader->cu->header.offset_size;
  }

  return read_str_index (reader->cu,
			 &reader->dwo_file->sections.str,
			 &reader->dwo_file->sections.str_offsets,
			 str_offsets_base, str_index, offset_size);
}

/* Given a DW_FORM_GNU_str_index from a Fission stub, fetch the string.  */

static const char *
read_stub_str_index (struct dwarf2_cu *cu, ULONGEST str_index)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  const char *objf_name = objfile_name (objfile);
  static const char form_name[] = "DW_FORM_GNU_str_index";
  static const char str_offsets_attr_name[] = "DW_AT_str_offsets";

  if (!cu->str_offsets_base.has_value ())
    error (_("%s used in Fission stub without %s"
	     " in CU at offset 0x%lx [in module %s]"),
	   form_name, str_offsets_attr_name,
	   (long) cu->header.offset_size, objf_name);

  return read_str_index (cu,
			 &cu->per_objfile->per_bfd->str,
			 &cu->per_objfile->per_bfd->str_offsets,
			 *cu->str_offsets_base, str_index,
			 cu->header.offset_size);
}

/* Return the length of an LEB128 number in BUF.  */

static int
leb128_size (const gdb_byte *buf)
{
  const gdb_byte *begin = buf;
  gdb_byte byte;

  while (1)
    {
      byte = *buf++;
      if ((byte & 128) == 0)
	return buf - begin;
    }
}

/* Converts DWARF language names to GDB language names.  */

enum language
dwarf_lang_to_enum_language (unsigned int lang)
{
  enum language language;

  switch (lang)
    {
    case DW_LANG_C89:
    case DW_LANG_C99:
    case DW_LANG_C11:
    case DW_LANG_C:
    case DW_LANG_UPC:
      language = language_c;
      break;
    case DW_LANG_Java:
    case DW_LANG_C_plus_plus:
    case DW_LANG_C_plus_plus_11:
    case DW_LANG_C_plus_plus_14:
      language = language_cplus;
      break;
    case DW_LANG_D:
      language = language_d;
      break;
    case DW_LANG_Fortran77:
    case DW_LANG_Fortran90:
    case DW_LANG_Fortran95:
    case DW_LANG_Fortran03:
    case DW_LANG_Fortran08:
      language = language_fortran;
      break;
    case DW_LANG_Go:
      language = language_go;
      break;
    case DW_LANG_Mips_Assembler:
      language = language_asm;
      break;
    case DW_LANG_Ada83:
    case DW_LANG_Ada95:
      language = language_ada;
      break;
    case DW_LANG_Modula2:
      language = language_m2;
      break;
    case DW_LANG_Pascal83:
      language = language_pascal;
      break;
    case DW_LANG_ObjC:
      language = language_objc;
      break;
    case DW_LANG_Rust:
    case DW_LANG_Rust_old:
      language = language_rust;
      break;
    case DW_LANG_OpenCL:
      language = language_opencl;
      break;
    case DW_LANG_Cobol74:
    case DW_LANG_Cobol85:
    default:
      language = language_minimal;
      break;
    }

  return language;
}

/* Return the named attribute or NULL if not there.  */

static struct attribute *
dwarf2_attr (struct die_info *die, unsigned int name, struct dwarf2_cu *cu)
{
  for (;;)
    {
      unsigned int i;
      struct attribute *spec = NULL;

      for (i = 0; i < die->num_attrs; ++i)
	{
	  if (die->attrs[i].name == name)
	    return &die->attrs[i];
	  if (die->attrs[i].name == DW_AT_specification
	      || die->attrs[i].name == DW_AT_abstract_origin)
	    spec = &die->attrs[i];
	}

      if (!spec)
	break;

      struct die_info *prev_die = die;
      die = follow_die_ref (die, spec, &cu);
      if (die == prev_die)
	/* Self-reference, we're done.  */
	break;
    }

  return NULL;
}

/* Return the string associated with a string-typed attribute, or NULL if it
   is either not found or is of an incorrect type.  */

static const char *
dwarf2_string_attr (struct die_info *die, unsigned int name, struct dwarf2_cu *cu)
{
  struct attribute *attr;
  const char *str = NULL;

  attr = dwarf2_attr (die, name, cu);

  if (attr != NULL)
    {
      str = attr->as_string ();
      if (str == nullptr)
	complaint (_("string type expected for attribute %s for "
		     "DIE at %s in module %s"),
		   dwarf_attr_name (name), sect_offset_str (die->sect_off),
		   objfile_name (cu->per_objfile->objfile));
    }

  return str;
}

/* Return the dwo name or NULL if not present. If present, it is in either
   DW_AT_GNU_dwo_name or DW_AT_dwo_name attribute.  */
static const char *
dwarf2_dwo_name (struct die_info *die, struct dwarf2_cu *cu)
{
  const char *dwo_name = dwarf2_string_attr (die, DW_AT_GNU_dwo_name, cu);
  if (dwo_name == nullptr)
    dwo_name = dwarf2_string_attr (die, DW_AT_dwo_name, cu);
  return dwo_name;
}

/* Return non-zero iff the attribute NAME is defined for the given DIE,
   and holds a non-zero value.  This function should only be used for
   DW_FORM_flag or DW_FORM_flag_present attributes.  */

static int
dwarf2_flag_true_p (struct die_info *die, unsigned name, struct dwarf2_cu *cu)
{
  struct attribute *attr = dwarf2_attr (die, name, cu);

  return attr != nullptr && attr->as_boolean ();
}

static int
die_is_declaration (struct die_info *die, struct dwarf2_cu *cu)
{
  /* A DIE is a declaration if it has a DW_AT_declaration attribute
     which value is non-zero.  However, we have to be careful with
     DIEs having a DW_AT_specification attribute, because dwarf2_attr()
     (via dwarf2_flag_true_p) follows this attribute.  So we may
     end up accidently finding a declaration attribute that belongs
     to a different DIE referenced by the specification attribute,
     even though the given DIE does not have a declaration attribute.  */
  return (dwarf2_flag_true_p (die, DW_AT_declaration, cu)
	  && dwarf2_attr (die, DW_AT_specification, cu) == NULL);
}

/* Return the die giving the specification for DIE, if there is
   one.  *SPEC_CU is the CU containing DIE on input, and the CU
   containing the return value on output.  If there is no
   specification, but there is an abstract origin, that is
   returned.  */

static struct die_info *
die_specification (struct die_info *die, struct dwarf2_cu **spec_cu)
{
  struct attribute *spec_attr = dwarf2_attr (die, DW_AT_specification,
					     *spec_cu);

  if (spec_attr == NULL)
    spec_attr = dwarf2_attr (die, DW_AT_abstract_origin, *spec_cu);

  if (spec_attr == NULL)
    return NULL;
  else
    return follow_die_ref (die, spec_attr, spec_cu);
}

/* A convenience function to find the proper .debug_line section for a CU.  */

static struct dwarf2_section_info *
get_debug_line_section (struct dwarf2_cu *cu)
{
  struct dwarf2_section_info *section;
  dwarf2_per_objfile *per_objfile = cu->per_objfile;

  /* For TUs in DWO files, the DW_AT_stmt_list attribute lives in the
     DWO file.  */
  if (cu->dwo_unit && cu->per_cu->is_debug_types)
    section = &cu->dwo_unit->dwo_file->sections.line;
  else if (cu->per_cu->is_dwz)
    {
      dwz_file *dwz = dwarf2_get_dwz_file (per_objfile->per_bfd, true);

      section = &dwz->line;
    }
  else
    section = &per_objfile->per_bfd->line;

  return section;
}

/* Read the statement program header starting at OFFSET in
   .debug_line, or .debug_line.dwo.  Return a pointer
   to a struct line_header, allocated using xmalloc.
   Returns NULL if there is a problem reading the header, e.g., if it
   has a version we don't understand.

   NOTE: the strings in the include directory and file name tables of
   the returned object point into the dwarf line section buffer,
   and must not be freed.  */

static line_header_up
dwarf_decode_line_header (sect_offset sect_off, struct dwarf2_cu *cu,
			  const char *comp_dir)
{
  struct dwarf2_section_info *section;
  dwarf2_per_objfile *per_objfile = cu->per_objfile;

  section = get_debug_line_section (cu);
  section->read (per_objfile->objfile);
  if (section->buffer == NULL)
    {
      if (cu->dwo_unit && cu->per_cu->is_debug_types)
	complaint (_("missing .debug_line.dwo section"));
      else
	complaint (_("missing .debug_line section"));
      return 0;
    }

  return dwarf_decode_line_header (sect_off, cu->per_cu->is_dwz,
				   per_objfile, section, &cu->header,
				   comp_dir);
}

/* Subroutine of dwarf_decode_lines to simplify it.
   Return the file name for the given file_entry.
   CU_INFO describes the CU's DW_AT_name and DW_AT_comp_dir.
   If space for the result is malloc'd, *NAME_HOLDER will be set.
   Returns NULL if FILE_INDEX should be ignored, i.e., it is
   equivalent to CU_INFO.  */

static const char *
compute_include_file_name (const struct line_header *lh, const file_entry &fe,
			   const file_and_directory &cu_info,
			   std::string &name_holder)
{
  const char *include_name = fe.name;
  const char *include_name_to_compare = include_name;

  const char *dir_name = fe.include_dir (lh);

  std::string hold_compare;
  if (!IS_ABSOLUTE_PATH (include_name)
      && (dir_name != nullptr || cu_info.get_comp_dir () != nullptr))
    {
      /* Avoid creating a duplicate name for CU_INFO.
	 We do this by comparing INCLUDE_NAME and CU_INFO.
	 Before we do the comparison, however, we need to account
	 for DIR_NAME and COMP_DIR.
	 First prepend dir_name (if non-NULL).  If we still don't
	 have an absolute path prepend comp_dir (if non-NULL).
	 However, the directory we record in the include-file's
	 psymtab does not contain COMP_DIR (to match the
	 corresponding symtab(s)).

	 Example:

	 bash$ cd /tmp
	 bash$ gcc -g ./hello.c
	 include_name = "hello.c"
	 dir_name = "."
	 DW_AT_comp_dir = comp_dir = "/tmp"
	 DW_AT_name = "./hello.c"

      */

      if (dir_name != NULL)
	{
	  name_holder = path_join (dir_name, include_name);
	  include_name = name_holder.c_str ();
	  include_name_to_compare = include_name;
	}
      if (!IS_ABSOLUTE_PATH (include_name)
	  && cu_info.get_comp_dir () != nullptr)
	{
	  hold_compare = path_join (cu_info.get_comp_dir (), include_name);
	  include_name_to_compare = hold_compare.c_str ();
	}
    }

  std::string copied_name;
  const char *cu_filename = cu_info.get_name ();
  if (!IS_ABSOLUTE_PATH (cu_filename) && cu_info.get_comp_dir () != nullptr)
    {
      copied_name = path_join (cu_info.get_comp_dir (), cu_filename);
      cu_filename = copied_name.c_str ();
    }

  if (FILENAME_CMP (include_name_to_compare, cu_filename) == 0)
    return nullptr;
  return include_name;
}

/* State machine to track the state of the line number program.  */

class lnp_state_machine
{
public:
  /* Initialize a machine state for the start of a line number
     program.  */
  lnp_state_machine (struct dwarf2_cu *cu, gdbarch *arch, line_header *lh);

  file_entry *current_file ()
  {
    /* lh->file_names is 0-based, but the file name numbers in the
       statement program are 1-based.  */
    return m_line_header->file_name_at (m_file);
  }

  /* Record the line in the state machine.  END_SEQUENCE is true if
     we're processing the end of a sequence.  */
  void record_line (bool end_sequence);

  /* Check ADDRESS is -1, or zero and less than UNRELOCATED_LOWPC, and if true
     nop-out rest of the lines in this sequence.  */
  void check_line_address (struct dwarf2_cu *cu,
			   const gdb_byte *line_ptr,
			   unrelocated_addr unrelocated_lowpc,
			   unrelocated_addr address);

  void handle_set_discriminator (unsigned int discriminator)
  {
    m_discriminator = discriminator;
    m_line_has_non_zero_discriminator |= discriminator != 0;
  }

  /* Handle DW_LNE_set_address.  */
  void handle_set_address (unrelocated_addr address)
  {
    m_op_index = 0;
    m_address
      = (unrelocated_addr) gdbarch_adjust_dwarf2_line (m_gdbarch,
						       (CORE_ADDR) address,
						       false);
  }

  /* Handle DW_LNS_advance_pc.  */
  void handle_advance_pc (CORE_ADDR adjust);

  /* Handle a special opcode.  */
  void handle_special_opcode (unsigned char op_code);

  /* Handle DW_LNS_advance_line.  */
  void handle_advance_line (int line_delta)
  {
    advance_line (line_delta);
  }

  /* Handle DW_LNS_set_file.  */
  void handle_set_file (file_name_index file);

  /* Handle DW_LNS_negate_stmt.  */
  void handle_negate_stmt ()
  {
    m_flags ^= LEF_IS_STMT;
  }

  /* Handle DW_LNS_const_add_pc.  */
  void handle_const_add_pc ();

  /* Handle DW_LNS_fixed_advance_pc.  */
  void handle_fixed_advance_pc (CORE_ADDR addr_adj)
  {
    addr_adj = gdbarch_adjust_dwarf2_line (m_gdbarch, addr_adj, true);
    m_address = (unrelocated_addr) ((CORE_ADDR) m_address + addr_adj);
    m_op_index = 0;
  }

  /* Handle DW_LNS_copy.  */
  void handle_copy ()
  {
    record_line (false);
    m_discriminator = 0;
    m_flags &= ~LEF_PROLOGUE_END;
    m_flags &= ~LEF_EPILOGUE_BEGIN;
  }

  /* Handle DW_LNE_end_sequence.  */
  void handle_end_sequence ()
  {
    m_currently_recording_lines = true;
  }

  /* Handle DW_LNS_set_prologue_end.  */
  void handle_set_prologue_end ()
  {
    m_flags |= LEF_PROLOGUE_END;
  }

  void handle_set_epilogue_begin ()
  {
    m_flags |= LEF_EPILOGUE_BEGIN;
  }

private:
  /* Advance the line by LINE_DELTA.  */
  void advance_line (int line_delta)
  {
    m_line += line_delta;

    if (line_delta != 0)
      m_line_has_non_zero_discriminator = m_discriminator != 0;
  }

  struct dwarf2_cu *m_cu;

  gdbarch *m_gdbarch;

  /* The line number header.  */
  line_header *m_line_header;

  /* These are part of the standard DWARF line number state machine,
     and initialized according to the DWARF spec.  */

  unsigned char m_op_index = 0;
  /* The line table index of the current file.  */
  file_name_index m_file = 1;
  unsigned int m_line = 1;

  /* These are initialized in the constructor.  */

  unrelocated_addr m_address;
  linetable_entry_flags m_flags;
  unsigned int m_discriminator = 0;

  /* Additional bits of state we need to track.  */

  /* The last file that we called dwarf2_start_subfile for.
     This is only used for TLLs.  */
  unsigned int m_last_file = 0;
  /* The last file a line number was recorded for.  */
  struct subfile *m_last_subfile = NULL;

  /* The address of the last line entry.  */
  unrelocated_addr m_last_address;

  /* Set to true when a previous line at the same address (using
     m_last_address) had LEF_IS_STMT set in m_flags.  This is reset to false
     when a line entry at a new address (m_address different to
     m_last_address) is processed.  */
  bool m_stmt_at_address = false;

  /* When true, record the lines we decode.  */
  bool m_currently_recording_lines = true;

  /* The last line number that was recorded, used to coalesce
     consecutive entries for the same line.  This can happen, for
     example, when discriminators are present.  PR 17276.  */
  unsigned int m_last_line = 0;
  bool m_line_has_non_zero_discriminator = false;
};

void
lnp_state_machine::handle_advance_pc (CORE_ADDR adjust)
{
  CORE_ADDR addr_adj = (((m_op_index + adjust)
			 / m_line_header->maximum_ops_per_instruction)
			* m_line_header->minimum_instruction_length);
  addr_adj = gdbarch_adjust_dwarf2_line (m_gdbarch, addr_adj, true);
  m_address = (unrelocated_addr) ((CORE_ADDR) m_address + addr_adj);
  m_op_index = ((m_op_index + adjust)
		% m_line_header->maximum_ops_per_instruction);
}

void
lnp_state_machine::handle_special_opcode (unsigned char op_code)
{
  unsigned char adj_opcode = op_code - m_line_header->opcode_base;
  unsigned char adj_opcode_d = adj_opcode / m_line_header->line_range;
  unsigned char adj_opcode_r = adj_opcode % m_line_header->line_range;
  CORE_ADDR addr_adj = (((m_op_index + adj_opcode_d)
			 / m_line_header->maximum_ops_per_instruction)
			* m_line_header->minimum_instruction_length);
  addr_adj = gdbarch_adjust_dwarf2_line (m_gdbarch, addr_adj, true);
  m_address = (unrelocated_addr) ((CORE_ADDR) m_address + addr_adj);
  m_op_index = ((m_op_index + adj_opcode_d)
		% m_line_header->maximum_ops_per_instruction);

  int line_delta = m_line_header->line_base + adj_opcode_r;
  advance_line (line_delta);
  record_line (false);
  m_discriminator = 0;
  m_flags &= ~LEF_PROLOGUE_END;
  m_flags &= ~LEF_EPILOGUE_BEGIN;
}

void
lnp_state_machine::handle_set_file (file_name_index file)
{
  m_file = file;

  const file_entry *fe = current_file ();
  if (fe == NULL)
    dwarf2_debug_line_missing_file_complaint ();
  else
    {
      m_last_subfile = m_cu->get_builder ()->get_current_subfile ();
      m_line_has_non_zero_discriminator = m_discriminator != 0;
      dwarf2_start_subfile (m_cu, *fe, *m_line_header);
    }
}

void
lnp_state_machine::handle_const_add_pc ()
{
  CORE_ADDR adjust
    = (255 - m_line_header->opcode_base) / m_line_header->line_range;

  CORE_ADDR addr_adj
    = (((m_op_index + adjust)
	/ m_line_header->maximum_ops_per_instruction)
       * m_line_header->minimum_instruction_length);

  addr_adj = gdbarch_adjust_dwarf2_line (m_gdbarch, addr_adj, true);
  m_address = (unrelocated_addr) ((CORE_ADDR) m_address + addr_adj);
  m_op_index = ((m_op_index + adjust)
		% m_line_header->maximum_ops_per_instruction);
}

/* Return non-zero if we should add LINE to the line number table.
   LINE is the line to add, LAST_LINE is the last line that was added,
   LAST_SUBFILE is the subfile for LAST_LINE.
   LINE_HAS_NON_ZERO_DISCRIMINATOR is non-zero if LINE has ever
   had a non-zero discriminator.

   We have to be careful in the presence of discriminators.
   E.g., for this line:

     for (i = 0; i < 100000; i++);

   clang can emit four line number entries for that one line,
   each with a different discriminator.
   See gdb.dwarf2/dw2-single-line-discriminators.exp for an example.

   However, we want gdb to coalesce all four entries into one.
   Otherwise the user could stepi into the middle of the line and
   gdb would get confused about whether the pc really was in the
   middle of the line.

   Things are further complicated by the fact that two consecutive
   line number entries for the same line is a heuristic used by gcc
   to denote the end of the prologue.  So we can't just discard duplicate
   entries, we have to be selective about it.  The heuristic we use is
   that we only collapse consecutive entries for the same line if at least
   one of those entries has a non-zero discriminator.  PR 17276.

   Note: Addresses in the line number state machine can never go backwards
   within one sequence, thus this coalescing is ok.  */

static int
dwarf_record_line_p (struct dwarf2_cu *cu,
		     unsigned int line, unsigned int last_line,
		     int line_has_non_zero_discriminator,
		     struct subfile *last_subfile)
{
  if (cu->get_builder ()->get_current_subfile () != last_subfile)
    return 1;
  if (line != last_line)
    return 1;
  /* Same line for the same file that we've seen already.
     As a last check, for pr 17276, only record the line if the line
     has never had a non-zero discriminator.  */
  if (!line_has_non_zero_discriminator)
    return 1;
  return 0;
}

/* Use the CU's builder to record line number LINE beginning at
   address ADDRESS in the line table of subfile SUBFILE.  */

static void
dwarf_record_line_1 (struct gdbarch *gdbarch, struct subfile *subfile,
		     unsigned int line, unrelocated_addr address,
		     linetable_entry_flags flags,
		     struct dwarf2_cu *cu)
{
  unrelocated_addr addr
    = unrelocated_addr (gdbarch_addr_bits_remove (gdbarch,
						  (CORE_ADDR) address));

  if (dwarf_line_debug)
    {
      gdb_printf (gdb_stdlog,
		  "Recording line %u, file %s, address %s\n",
		  line, lbasename (subfile->name.c_str ()),
		  paddress (gdbarch, (CORE_ADDR) address));
    }

  if (cu != nullptr)
    cu->get_builder ()->record_line (subfile, line, addr, flags);
}

/* Subroutine of dwarf_decode_lines_1 to simplify it.
   Mark the end of a set of line number records.
   The arguments are the same as for dwarf_record_line_1.
   If SUBFILE is NULL the request is ignored.  */

static void
dwarf_finish_line (struct gdbarch *gdbarch, struct subfile *subfile,
		   unrelocated_addr address, struct dwarf2_cu *cu)
{
  if (subfile == NULL)
    return;

  if (dwarf_line_debug)
    {
      gdb_printf (gdb_stdlog,
		  "Finishing current line, file %s, address %s\n",
		  lbasename (subfile->name.c_str ()),
		  paddress (gdbarch, (CORE_ADDR) address));
    }

  dwarf_record_line_1 (gdbarch, subfile, 0, address, LEF_IS_STMT, cu);
}

void
lnp_state_machine::record_line (bool end_sequence)
{
  if (dwarf_line_debug)
    {
      gdb_printf (gdb_stdlog,
		  "Processing actual line %u: file %u,"
		  " address %s, is_stmt %u, prologue_end %u,"
		  " epilogue_begin %u, discrim %u%s\n",
		  m_line, m_file,
		  paddress (m_gdbarch, (CORE_ADDR) m_address),
		  (m_flags & LEF_IS_STMT) != 0,
		  (m_flags & LEF_PROLOGUE_END) != 0,
		  (m_flags & LEF_EPILOGUE_BEGIN) != 0,
		  m_discriminator,
		  (end_sequence ? "\t(end sequence)" : ""));
    }

  file_entry *fe = current_file ();

  if (fe == NULL)
    dwarf2_debug_line_missing_file_complaint ();
  /* For now we ignore lines not starting on an instruction boundary.
     But not when processing end_sequence for compatibility with the
     previous version of the code.  */
  else if (m_op_index == 0 || end_sequence)
    {
      /* When we switch files we insert an end maker in the first file,
	 switch to the second file and add a new line entry.  The
	 problem is that the end marker inserted in the first file will
	 discard any previous line entries at the same address.  If the
	 line entries in the first file are marked as is-stmt, while
	 the new line in the second file is non-stmt, then this means
	 the end marker will discard is-stmt lines so we can have a
	 non-stmt line.  This means that there are less addresses at
	 which the user can insert a breakpoint.

	 To improve this we track the last address in m_last_address,
	 and whether we have seen an is-stmt at this address.  Then
	 when switching files, if we have seen a stmt at the current
	 address, and we are switching to create a non-stmt line, then
	 discard the new line.  */
      bool file_changed
	= m_last_subfile != m_cu->get_builder ()->get_current_subfile ();
      bool ignore_this_line
	= ((file_changed && !end_sequence && m_last_address == m_address
	    && ((m_flags & LEF_IS_STMT) == 0)
	    && m_stmt_at_address)
	   || (!end_sequence && m_line == 0));

      if ((file_changed && !ignore_this_line) || end_sequence)
	{
	  dwarf_finish_line (m_gdbarch, m_last_subfile, m_address,
			     m_currently_recording_lines ? m_cu : nullptr);
	}

      if (!end_sequence && !ignore_this_line)
	{
	  linetable_entry_flags lte_flags = m_flags;
	  if (producer_is_codewarrior (m_cu))
	    lte_flags |= LEF_IS_STMT;

	  if (dwarf_record_line_p (m_cu, m_line, m_last_line,
				   m_line_has_non_zero_discriminator,
				   m_last_subfile))
	    {
	      buildsym_compunit *builder = m_cu->get_builder ();
	      dwarf_record_line_1 (m_gdbarch,
				   builder->get_current_subfile (),
				   m_line, m_address, lte_flags,
				   m_currently_recording_lines ? m_cu : nullptr);
	    }
	  m_last_subfile = m_cu->get_builder ()->get_current_subfile ();
	  m_last_line = m_line;
	}
    }

  /* Track whether we have seen any IS_STMT true at m_address in case we
     have multiple line table entries all at m_address.  */
  if (m_last_address != m_address)
    {
      m_stmt_at_address = false;
      m_last_address = m_address;
    }
  m_stmt_at_address |= (m_flags & LEF_IS_STMT) != 0;
}

lnp_state_machine::lnp_state_machine (struct dwarf2_cu *cu, gdbarch *arch,
				      line_header *lh)
  : m_cu (cu),
    m_gdbarch (arch),
    m_line_header (lh),
    /* Call `gdbarch_adjust_dwarf2_line' on the initial 0 address as
       if there was a line entry for it so that the backend has a
       chance to adjust it and also record it in case it needs it.
       This is currently used by MIPS code,
       cf. `mips_adjust_dwarf2_line'.  */
    m_address ((unrelocated_addr) gdbarch_adjust_dwarf2_line (arch, 0, 0)),
    m_flags (lh->default_is_stmt ? LEF_IS_STMT : (linetable_entry_flags) 0),
    m_last_address (m_address)
{
}

void
lnp_state_machine::check_line_address (struct dwarf2_cu *cu,
				       const gdb_byte *line_ptr,
				       unrelocated_addr unrelocated_lowpc,
				       unrelocated_addr address)
{
  /* Linkers resolve a symbolic relocation referencing a GC'd function to 0 or
     -1.  If ADDRESS is 0, ignoring the opcode will err if the text section is
     located at 0x0.  In this case, additionally check that if
     ADDRESS < UNRELOCATED_LOWPC.  */

  if ((address == (unrelocated_addr) 0 && address < unrelocated_lowpc)
      || address == (unrelocated_addr) -1)
    {
      /* This line table is for a function which has been
	 GCd by the linker.  Ignore it.  PR gdb/12528 */

      struct objfile *objfile = cu->per_objfile->objfile;
      long line_offset = line_ptr - get_debug_line_section (cu)->buffer;

      complaint (_(".debug_line address at offset 0x%lx is 0 [in module %s]"),
		 line_offset, objfile_name (objfile));
      m_currently_recording_lines = false;
      /* Note: m_currently_recording_lines is left as false until we see
	 DW_LNE_end_sequence.  */
    }
}

/* Subroutine of dwarf_decode_lines to simplify it.
   Process the line number information in LH.  */

static void
dwarf_decode_lines_1 (struct line_header *lh, struct dwarf2_cu *cu,
		      unrelocated_addr lowpc)
{
  const gdb_byte *line_ptr, *extended_end;
  const gdb_byte *line_end;
  unsigned int bytes_read, extended_len;
  unsigned char op_code, extended_op;
  struct objfile *objfile = cu->per_objfile->objfile;
  bfd *abfd = objfile->obfd.get ();
  struct gdbarch *gdbarch = objfile->arch ();

  line_ptr = lh->statement_program_start;
  line_end = lh->statement_program_end;

  /* Read the statement sequences until there's nothing left.  */
  while (line_ptr < line_end)
    {
      /* The DWARF line number program state machine.  Reset the state
	 machine at the start of each sequence.  */
      lnp_state_machine state_machine (cu, gdbarch, lh);
      bool end_sequence = false;

      /* Start a subfile for the current file of the state
	 machine.  */
      const file_entry *fe = state_machine.current_file ();

      if (fe != NULL)
	dwarf2_start_subfile (cu, *fe, *lh);

      /* Decode the table.  */
      while (line_ptr < line_end && !end_sequence)
	{
	  op_code = read_1_byte (abfd, line_ptr);
	  line_ptr += 1;

	  if (op_code >= lh->opcode_base)
	    {
	      /* Special opcode.  */
	      state_machine.handle_special_opcode (op_code);
	    }
	  else switch (op_code)
	    {
	    case DW_LNS_extended_op:
	      extended_len = read_unsigned_leb128 (abfd, line_ptr,
						   &bytes_read);
	      line_ptr += bytes_read;
	      extended_end = line_ptr + extended_len;
	      extended_op = read_1_byte (abfd, line_ptr);
	      line_ptr += 1;
	      if (DW_LNE_lo_user <= extended_op
		  && extended_op <= DW_LNE_hi_user)
		{
		  /* Vendor extension, ignore.  */
		  line_ptr = extended_end;
		  break;
		}
	      switch (extended_op)
		{
		case DW_LNE_end_sequence:
		  state_machine.handle_end_sequence ();
		  end_sequence = true;
		  break;
		case DW_LNE_set_address:
		  {
		    unrelocated_addr address
		      = cu->header.read_address (abfd, line_ptr, &bytes_read);
		    line_ptr += bytes_read;

		    state_machine.check_line_address (cu, line_ptr, lowpc,
						      address);
		    state_machine.handle_set_address (address);
		  }
		  break;
		case DW_LNE_define_file:
		  {
		    const char *cur_file;
		    unsigned int mod_time, length;
		    dir_index dindex;

		    cur_file = read_direct_string (abfd, line_ptr,
						   &bytes_read);
		    line_ptr += bytes_read;
		    dindex = (dir_index)
		      read_unsigned_leb128 (abfd, line_ptr, &bytes_read);
		    line_ptr += bytes_read;
		    mod_time =
		      read_unsigned_leb128 (abfd, line_ptr, &bytes_read);
		    line_ptr += bytes_read;
		    length =
		      read_unsigned_leb128 (abfd, line_ptr, &bytes_read);
		    line_ptr += bytes_read;
		    lh->add_file_name (cur_file, dindex, mod_time, length);
		  }
		  break;
		case DW_LNE_set_discriminator:
		  {
		    /* The discriminator is not interesting to the
		       debugger; just ignore it.  We still need to
		       check its value though:
		       if there are consecutive entries for the same
		       (non-prologue) line we want to coalesce them.
		       PR 17276.  */
		    unsigned int discr
		      = read_unsigned_leb128 (abfd, line_ptr, &bytes_read);
		    line_ptr += bytes_read;

		    state_machine.handle_set_discriminator (discr);
		  }
		  break;
		default:
		  complaint (_("mangled .debug_line section"));
		  return;
		}
	      /* Make sure that we parsed the extended op correctly.  If e.g.
		 we expected a different address size than the producer used,
		 we may have read the wrong number of bytes.  */
	      if (line_ptr != extended_end)
		{
		  complaint (_("mangled .debug_line section"));
		  return;
		}
	      break;
	    case DW_LNS_copy:
	      state_machine.handle_copy ();
	      break;
	    case DW_LNS_advance_pc:
	      {
		CORE_ADDR adjust
		  = read_unsigned_leb128 (abfd, line_ptr, &bytes_read);
		line_ptr += bytes_read;

		state_machine.handle_advance_pc (adjust);
	      }
	      break;
	    case DW_LNS_advance_line:
	      {
		int line_delta
		  = read_signed_leb128 (abfd, line_ptr, &bytes_read);
		line_ptr += bytes_read;

		state_machine.handle_advance_line (line_delta);
	      }
	      break;
	    case DW_LNS_set_file:
	      {
		file_name_index file
		  = (file_name_index) read_unsigned_leb128 (abfd, line_ptr,
							    &bytes_read);
		line_ptr += bytes_read;

		state_machine.handle_set_file (file);
	      }
	      break;
	    case DW_LNS_set_column:
	      (void) read_unsigned_leb128 (abfd, line_ptr, &bytes_read);
	      line_ptr += bytes_read;
	      break;
	    case DW_LNS_negate_stmt:
	      state_machine.handle_negate_stmt ();
	      break;
	    case DW_LNS_set_basic_block:
	      break;
	    /* Add to the address register of the state machine the
	       address increment value corresponding to special opcode
	       255.  I.e., this value is scaled by the minimum
	       instruction length since special opcode 255 would have
	       scaled the increment.  */
	    case DW_LNS_const_add_pc:
	      state_machine.handle_const_add_pc ();
	      break;
	    case DW_LNS_fixed_advance_pc:
	      {
		CORE_ADDR addr_adj = read_2_bytes (abfd, line_ptr);
		line_ptr += 2;

		state_machine.handle_fixed_advance_pc (addr_adj);
	      }
	      break;
	    case DW_LNS_set_prologue_end:
	      state_machine.handle_set_prologue_end ();
	      break;
	    case DW_LNS_set_epilogue_begin:
	      state_machine.handle_set_epilogue_begin ();
	      break;
	    default:
	      {
		/* Unknown standard opcode, ignore it.  */
		int i;

		for (i = 0; i < lh->standard_opcode_lengths[op_code]; i++)
		  {
		    (void) read_unsigned_leb128 (abfd, line_ptr, &bytes_read);
		    line_ptr += bytes_read;
		  }
	      }
	    }
	}

      if (!end_sequence)
	dwarf2_debug_line_missing_end_sequence_complaint ();

      /* We got a DW_LNE_end_sequence (or we ran off the end of the buffer,
	 in which case we still finish recording the last line).  */
      state_machine.record_line (true);
    }
}

/* Decode the Line Number Program (LNP) for the given line_header
   structure and CU.  The actual information extracted and the type
   of structures created from the LNP depends on the value of PST.

   FND holds the CU file name and directory, if known.
   It is used for relative paths in the line table.

   NOTE: It is important that psymtabs have the same file name (via
   strcmp) as the corresponding symtab.  Since the directory is not
   used in the name of the symtab we don't use it in the name of the
   psymtabs we create.  E.g. expand_line_sal requires this when
   finding psymtabs to expand.  A good testcase for this is
   mb-inline.exp.

   LOWPC is the lowest address in CU (or 0 if not known).

   Boolean DECODE_MAPPING specifies we need to fully decode .debug_line
   for its PC<->lines mapping information.  Otherwise only the filename
   table is read in.  */

static void
dwarf_decode_lines (struct line_header *lh, struct dwarf2_cu *cu,
		    unrelocated_addr lowpc, int decode_mapping)
{
  if (decode_mapping)
    dwarf_decode_lines_1 (lh, cu, lowpc);

  /* Make sure a symtab is created for every file, even files
     which contain only variables (i.e. no code with associated
     line numbers).  */
  buildsym_compunit *builder = cu->get_builder ();
  struct compunit_symtab *cust = builder->get_compunit_symtab ();

  for (auto &fe : lh->file_names ())
    {
      dwarf2_start_subfile (cu, fe, *lh);
      subfile *sf = builder->get_current_subfile ();

      if (sf->symtab == nullptr)
	sf->symtab = allocate_symtab (cust, sf->name.c_str (),
				      sf->name_for_id.c_str ());

      fe.symtab = sf->symtab;
    }
}

/* Start a subfile for DWARF.  FILENAME is the name of the file and
   DIRNAME the name of the source directory which contains FILENAME
   or NULL if not known.
   This routine tries to keep line numbers from identical absolute and
   relative file names in a common subfile.

   Using the `list' example from the GDB testsuite, which resides in
   /srcdir and compiling it with Irix6.2 cc in /compdir using a filename
   of /srcdir/list0.c yields the following debugging information for list0.c:

   DW_AT_name:          /srcdir/list0.c
   DW_AT_comp_dir:      /compdir
   files.files[0].name: list0.h
   files.files[0].dir:  /srcdir
   files.files[1].name: list0.c
   files.files[1].dir:  /srcdir

   The line number information for list0.c has to end up in a single
   subfile, so that `break /srcdir/list0.c:1' works as expected.
   start_subfile will ensure that this happens provided that we pass the
   concatenation of files.files[1].dir and files.files[1].name as the
   subfile's name.  */

static void
dwarf2_start_subfile (dwarf2_cu *cu, const file_entry &fe,
		      const line_header &lh)
{
  std::string filename_holder;
  const char *filename = fe.name;
  const char *dirname = lh.include_dir_at (fe.d_index);

  /* In order not to lose the line information directory,
     we concatenate it to the filename when it makes sense.
     Note that the Dwarf3 standard says (speaking of filenames in line
     information): ``The directory index is ignored for file names
     that represent full path names''.  Thus ignoring dirname in the
     `else' branch below isn't an issue.  */

  if (!IS_ABSOLUTE_PATH (filename) && dirname != NULL)
    {
      filename_holder = path_join (dirname, filename);
      filename = filename_holder.c_str ();
    }

  std::string filename_for_id = lh.file_file_name (fe);
  cu->get_builder ()->start_subfile (filename, filename_for_id.c_str ());
}

static void
var_decode_location (struct attribute *attr, struct symbol *sym,
		     struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  struct comp_unit_head *cu_header = &cu->header;

  /* NOTE drow/2003-01-30: There used to be a comment and some special
     code here to turn a symbol with DW_AT_external and a
     SYMBOL_VALUE_ADDRESS of 0 into a LOC_UNRESOLVED symbol.  This was
     necessary for platforms (maybe Alpha, certainly PowerPC GNU/Linux
     with some versions of binutils) where shared libraries could have
     relocations against symbols in their debug information - the
     minimal symbol would have the right address, but the debug info
     would not.  It's no longer necessary, because we will explicitly
     apply relocations when we read in the debug information now.  */

  /* A DW_AT_location attribute with no contents indicates that a
     variable has been optimized away.  */
  if (attr->form_is_block () && attr->as_block ()->size == 0)
    {
      sym->set_aclass_index (LOC_OPTIMIZED_OUT);
      return;
    }

  /* Handle one degenerate form of location expression specially, to
     preserve GDB's previous behavior when section offsets are
     specified.  If this is just a DW_OP_addr, DW_OP_addrx, or
     DW_OP_GNU_addr_index then mark this symbol as LOC_STATIC.  */

  if (attr->form_is_block ())
    {
      struct dwarf_block *block = attr->as_block ();

      if ((block->data[0] == DW_OP_addr
	   && block->size == 1 + cu_header->addr_size)
	  || ((block->data[0] == DW_OP_GNU_addr_index
	       || block->data[0] == DW_OP_addrx)
	      && (block->size
		  == 1 + leb128_size (&block->data[1]))))
	{
	  unsigned int dummy;

	  unrelocated_addr tem;
	  if (block->data[0] == DW_OP_addr)
	    tem = cu->header.read_address (objfile->obfd.get (),
					   block->data + 1,
					   &dummy);
	  else
	    tem = read_addr_index_from_leb128 (cu, block->data + 1, &dummy);
	  sym->set_value_address ((CORE_ADDR) tem);
	  sym->set_aclass_index (LOC_STATIC);
	  fixup_symbol_section (sym, objfile);
	  sym->set_value_address
	    (sym->value_address ()
	     + objfile->section_offsets[sym->section_index ()]);
	  return;
	}
    }

  /* NOTE drow/2002-01-30: It might be worthwhile to have a static
     expression evaluator, and use LOC_COMPUTED only when necessary
     (i.e. when the value of a register or memory location is
     referenced, or a thread-local block, etc.).  Then again, it might
     not be worthwhile.  I'm assuming that it isn't unless performance
     or memory numbers show me otherwise.  */

  dwarf2_symbol_mark_computed (attr, sym, cu, 0);

  if (SYMBOL_COMPUTED_OPS (sym)->location_has_loclist)
    cu->has_loclist = true;
}

/* A helper function to add an "export" symbol.  The new symbol starts
   as a clone of ORIG, but is modified to defer to the symbol named
   ORIG_NAME.  The original symbol uses the name given in the source
   code, and the symbol that is created here uses the linkage name as
   its name.  See ada-imported.c.  */

static void
add_ada_export_symbol (struct symbol *orig, const char *new_name,
		       const char *orig_name, struct dwarf2_cu *cu,
		       struct pending **list_to_add)
{
  struct symbol *copy
    = new (&cu->per_objfile->objfile->objfile_obstack) symbol (*orig);
  copy->set_linkage_name (new_name);
  SYMBOL_LOCATION_BATON (copy) = (void *) orig_name;
  copy->set_aclass_index (copy->aclass () == LOC_BLOCK
			  ? ada_block_index
			  : ada_imported_index);
  add_symbol_to_list (copy, list_to_add);
}

/* A helper function that decides if a given symbol is an Ada Pragma
   Import or Pragma Export.  */

static bool
is_ada_import_or_export (dwarf2_cu *cu, const char *name,
			 const char *linkagename)
{
  return (cu->lang () == language_ada
	  && linkagename != nullptr
	  && !streq (name, linkagename)
	  /* The following exclusions are necessary because symbols
	     with names or linkage names that match here will meet the
	     other criteria but are not in fact caused by Pragma
	     Import or Pragma Export, and applying the import/export
	     treatment to them will introduce problems.  Some of these
	     checks only apply to functions, but it is simpler and
	     harmless to always do them all.  */
	  && !startswith (name, "__builtin")
	  && !startswith (linkagename, "___ghost_")
	  && !startswith (linkagename, "__gnat")
	  && !startswith (linkagename, "_ada_")
	  && !streq (linkagename, "adainit"));
}

/* Given a pointer to a DWARF information entry, figure out if we need
   to make a symbol table entry for it, and if so, create a new entry
   and return a pointer to it.
   If TYPE is NULL, determine symbol type from the die, otherwise
   used the passed type.
   If SPACE is not NULL, use it to hold the new symbol.  If it is
   NULL, allocate a new symbol on the objfile's obstack.  */

static struct symbol *
new_symbol (struct die_info *die, struct type *type, struct dwarf2_cu *cu,
	    struct symbol *space)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  struct symbol *sym = NULL;
  const char *name;
  struct attribute *attr = NULL;
  struct attribute *attr2 = NULL;
  struct pending **list_to_add = NULL;

  int inlined_func = (die->tag == DW_TAG_inlined_subroutine);

  name = dwarf2_name (die, cu);
  if (name == nullptr && (die->tag == DW_TAG_subprogram
			  || die->tag == DW_TAG_inlined_subroutine
			  || die->tag == DW_TAG_entry_point))
    name = dw2_linkage_name (die, cu);

  if (name)
    {
      int suppress_add = 0;

      if (space)
	sym = space;
      else
	sym = new (&objfile->objfile_obstack) symbol;
      OBJSTAT (objfile, n_syms++);

      /* Cache this symbol's name and the name's demangled form (if any).  */
      sym->set_language (cu->lang (), &objfile->objfile_obstack);
      /* Fortran does not have mangling standard and the mangling does differ
	 between gfortran, iFort etc.  */
      const char *physname
	= (cu->lang () == language_fortran
	   ? dwarf2_full_name (name, die, cu)
	   : dwarf2_physname (name, die, cu));
      const char *linkagename = dw2_linkage_name (die, cu);

      if (linkagename == nullptr || cu->lang () == language_ada)
	sym->set_linkage_name (physname);
      else
	{
	  sym->set_demangled_name (physname, &objfile->objfile_obstack);
	  sym->set_linkage_name (linkagename);
	}

      /* Handle DW_AT_artificial.  */
      attr = dwarf2_attr (die, DW_AT_artificial, cu);
      if (attr != nullptr)
	sym->set_is_artificial (attr->as_boolean ());

      /* Default assumptions.
	 Use the passed type or decode it from the die.  */
      sym->set_domain (VAR_DOMAIN);
      sym->set_aclass_index (LOC_OPTIMIZED_OUT);
      if (type != NULL)
	sym->set_type (type);
      else
	sym->set_type (die_type (die, cu));
      attr = dwarf2_attr (die,
			  inlined_func ? DW_AT_call_line : DW_AT_decl_line,
			  cu);
      if (attr != nullptr)
	sym->set_line (attr->constant_value (0));

      attr = dwarf2_attr (die,
			  inlined_func ? DW_AT_call_file : DW_AT_decl_file,
			  cu);
      if (attr != nullptr && attr->is_nonnegative ())
	{
	  file_name_index file_index
	    = (file_name_index) attr->as_nonnegative ();
	  struct file_entry *fe;

	  if (cu->line_header != NULL)
	    fe = cu->line_header->file_name_at (file_index);
	  else
	    fe = NULL;

	  if (fe == NULL)
	    complaint (_("file index out of range"));
	  else
	    sym->set_symtab (fe->symtab);
	}

      switch (die->tag)
	{
	case DW_TAG_label:
	  attr = dwarf2_attr (die, DW_AT_low_pc, cu);
	  if (attr != nullptr)
	    {
	      CORE_ADDR addr = per_objfile->relocate (attr->as_address ());
	      sym->set_section_index (SECT_OFF_TEXT (objfile));
	      sym->set_value_address (addr);
	      sym->set_aclass_index (LOC_LABEL);
	    }
	  else
	    sym->set_aclass_index (LOC_OPTIMIZED_OUT);
	  sym->set_type (builtin_type (objfile)->builtin_core_addr);
	  sym->set_domain (LABEL_DOMAIN);
	  add_symbol_to_list (sym, cu->list_in_scope);
	  break;
	case DW_TAG_entry_point:
	  /* SYMBOL_BLOCK_VALUE (sym) will be filled in later by
	     finish_block.  */
	  sym->set_aclass_index (LOC_BLOCK);
	  /* DW_TAG_entry_point provides an additional entry_point to an
	     existing sub_program.  Therefore, we inherit the "external"
	     attribute from the sub_program to which the entry_point
	     belongs to.  */
	  attr2 = dwarf2_attr (die->parent, DW_AT_external, cu);
	  if (attr2 != nullptr && attr2->as_boolean ())
	    list_to_add = cu->get_builder ()->get_global_symbols ();
	  else
	    list_to_add = cu->list_in_scope;
	  break;
	case DW_TAG_subprogram:
	  /* SYMBOL_BLOCK_VALUE (sym) will be filled in later by
	     finish_block.  */
	  sym->set_aclass_index (LOC_BLOCK);
	  attr2 = dwarf2_attr (die, DW_AT_external, cu);
	  if ((attr2 != nullptr && attr2->as_boolean ())
	      || cu->lang () == language_ada
	      || cu->lang () == language_fortran)
	    {
	      /* Subprograms marked external are stored as a global symbol.
		 Ada and Fortran subprograms, whether marked external or
		 not, are always stored as a global symbol, because we want
		 to be able to access them globally.  For instance, we want
		 to be able to break on a nested subprogram without having
		 to specify the context.  */
	      list_to_add = cu->get_builder ()->get_global_symbols ();
	    }
	  else
	    {
	      list_to_add = cu->list_in_scope;
	    }

	  if (is_ada_import_or_export (cu, name, linkagename))
	    {
	      /* This is either a Pragma Import or Export.  They can
		 be distinguished by the declaration flag.  */
	      sym->set_linkage_name (name);
	      if (die_is_declaration (die, cu))
		{
		  /* For Import, create a symbol using the source
		     name, and have it refer to the linkage name.  */
		  SYMBOL_LOCATION_BATON (sym) = (void *) linkagename;
		  sym->set_aclass_index (ada_block_index);
		}
	      else
		{
		  /* For Export, create a symbol using the source
		     name, then create a second symbol that refers
		     back to it.  */
		  add_ada_export_symbol (sym, linkagename, name, cu,
					 list_to_add);
		}
	    }
	  break;
	case DW_TAG_inlined_subroutine:
	  /* SYMBOL_BLOCK_VALUE (sym) will be filled in later by
	     finish_block.  */
	  sym->set_aclass_index (LOC_BLOCK);
	  sym->set_is_inlined (1);
	  list_to_add = cu->list_in_scope;
	  break;
	case DW_TAG_template_value_param:
	  suppress_add = 1;
	  [[fallthrough]];
	case DW_TAG_constant:
	case DW_TAG_variable:
	case DW_TAG_member:
	  /* Compilation with minimal debug info may result in
	     variables with missing type entries.  Change the
	     misleading `void' type to something sensible.  */
	  if (sym->type ()->code () == TYPE_CODE_VOID)
	    sym->set_type (builtin_type (objfile)->builtin_int);

	  attr = dwarf2_attr (die, DW_AT_const_value, cu);
	  /* In the case of DW_TAG_member, we should only be called for
	     static const members.  */
	  if (die->tag == DW_TAG_member)
	    {
	      /* dwarf2_add_field uses die_is_declaration,
		 so we do the same.  */
	      gdb_assert (die_is_declaration (die, cu));
	      gdb_assert (attr);
	    }
	  if (attr != nullptr)
	    {
	      dwarf2_const_value (attr, sym, cu);
	      attr2 = dwarf2_attr (die, DW_AT_external, cu);
	      if (!suppress_add)
		{
		  if (attr2 != nullptr && attr2->as_boolean ())
		    list_to_add = cu->get_builder ()->get_global_symbols ();
		  else
		    list_to_add = cu->list_in_scope;
		}
	      break;
	    }
	  attr = dwarf2_attr (die, DW_AT_location, cu);
	  if (attr != nullptr)
	    {
	      var_decode_location (attr, sym, cu);
	      attr2 = dwarf2_attr (die, DW_AT_external, cu);

	      /* Fortran explicitly imports any global symbols to the local
		 scope by DW_TAG_common_block.  */
	      if (cu->lang () == language_fortran && die->parent
		  && die->parent->tag == DW_TAG_common_block)
		attr2 = NULL;

	      if (sym->aclass () == LOC_STATIC
		  && sym->value_address () == 0
		  && !per_objfile->per_bfd->has_section_at_zero)
		{
		  /* When a static variable is eliminated by the linker,
		     the corresponding debug information is not stripped
		     out, but the variable address is set to null;
		     do not add such variables into symbol table.  */
		}
	      else if (attr2 != nullptr && attr2->as_boolean ())
		{
		  if (sym->aclass () == LOC_STATIC
		      && (objfile->flags & OBJF_MAINLINE) == 0
		      && per_objfile->per_bfd->can_copy)
		    {
		      /* A global static variable might be subject to
			 copy relocation.  We first check for a local
			 minsym, though, because maybe the symbol was
			 marked hidden, in which case this would not
			 apply.  */
		      bound_minimal_symbol found
			= (lookup_minimal_symbol_linkage
			   (sym->linkage_name (), objfile));
		      if (found.minsym != nullptr)
			sym->maybe_copied = 1;
		    }

		  /* A variable with DW_AT_external is never static,
		     but it may be block-scoped.  */
		  list_to_add
		    = ((cu->list_in_scope
			== cu->get_builder ()->get_file_symbols ())
		       ? cu->get_builder ()->get_global_symbols ()
		       : cu->list_in_scope);
		}
	      else
		list_to_add = cu->list_in_scope;

	      if (list_to_add != nullptr
		  && is_ada_import_or_export (cu, name, linkagename))
		{
		  /* This is a Pragma Export.  A Pragma Import won't
		     be seen here, because it will not have a location
		     and so will be handled below.  */
		  add_ada_export_symbol (sym, name, linkagename, cu,
					 list_to_add);
		}
	    }
	  else
	    {
	      /* We do not know the address of this symbol.
		 If it is an external symbol and we have type information
		 for it, enter the symbol as a LOC_UNRESOLVED symbol.
		 The address of the variable will then be determined from
		 the minimal symbol table whenever the variable is
		 referenced.  */
	      attr2 = dwarf2_attr (die, DW_AT_external, cu);

	      /* Fortran explicitly imports any global symbols to the local
		 scope by DW_TAG_common_block.  */
	      if (cu->lang () == language_fortran && die->parent
		  && die->parent->tag == DW_TAG_common_block)
		{
		  /* SYMBOL_CLASS doesn't matter here because
		     read_common_block is going to reset it.  */
		  if (!suppress_add)
		    list_to_add = cu->list_in_scope;
		}
	      else if (is_ada_import_or_export (cu, name, linkagename))
		{
		  /* This is a Pragma Import.  A Pragma Export won't
		     be seen here, because it will have a location and
		     so will be handled above.  */
		  sym->set_linkage_name (name);
		  list_to_add = cu->list_in_scope;
		  SYMBOL_LOCATION_BATON (sym) = (void *) linkagename;
		  sym->set_aclass_index (ada_imported_index);
		}
	      else if (attr2 != nullptr && attr2->as_boolean ()
		       && dwarf2_attr (die, DW_AT_type, cu) != NULL)
		{
		  /* A variable with DW_AT_external is never static, but it
		     may be block-scoped.  */
		  list_to_add
		    = ((cu->list_in_scope
			== cu->get_builder ()->get_file_symbols ())
		       ? cu->get_builder ()->get_global_symbols ()
		       : cu->list_in_scope);

		  sym->set_aclass_index (LOC_UNRESOLVED);
		}
	      else if (!die_is_declaration (die, cu))
		{
		  /* Use the default LOC_OPTIMIZED_OUT class.  */
		  gdb_assert (sym->aclass () == LOC_OPTIMIZED_OUT);
		  if (!suppress_add)
		    list_to_add = cu->list_in_scope;
		}
	    }
	  break;
	case DW_TAG_formal_parameter:
	  {
	    /* If we are inside a function, mark this as an argument.  If
	       not, we might be looking at an argument to an inlined function
	       when we do not have enough information to show inlined frames;
	       pretend it's a local variable in that case so that the user can
	       still see it.  */
	    struct context_stack *curr
	      = cu->get_builder ()->get_current_context_stack ();
	    if (curr != nullptr && curr->name != nullptr)
	      sym->set_is_argument (1);
	    attr = dwarf2_attr (die, DW_AT_location, cu);
	    if (attr != nullptr)
	      {
		var_decode_location (attr, sym, cu);
	      }
	    attr = dwarf2_attr (die, DW_AT_const_value, cu);
	    if (attr != nullptr)
	      {
		dwarf2_const_value (attr, sym, cu);
	      }

	    list_to_add = cu->list_in_scope;
	  }
	  break;
	case DW_TAG_unspecified_parameters:
	  /* From varargs functions; gdb doesn't seem to have any
	     interest in this information, so just ignore it for now.
	     (FIXME?) */
	  break;
	case DW_TAG_template_type_param:
	  suppress_add = 1;
	  [[fallthrough]];
	case DW_TAG_class_type:
	case DW_TAG_interface_type:
	case DW_TAG_structure_type:
	case DW_TAG_union_type:
	case DW_TAG_set_type:
	case DW_TAG_enumeration_type:
	case DW_TAG_namelist:
	  if (die->tag == DW_TAG_namelist)
	    {
	      sym->set_aclass_index (LOC_STATIC);
	      sym->set_domain (VAR_DOMAIN);
	    }
	  else
	    {
	      sym->set_aclass_index (LOC_TYPEDEF);
	      sym->set_domain (STRUCT_DOMAIN);
	    }

	  /* NOTE: carlton/2003-11-10: C++ class symbols shouldn't
	     really ever be static objects: otherwise, if you try
	     to, say, break of a class's method and you're in a file
	     which doesn't mention that class, it won't work unless
	     the check for all static symbols in lookup_symbol_aux
	     saves you.  See the OtherFileClass tests in
	     gdb.c++/namespace.exp.  */

	  if (!suppress_add)
	    {
	      buildsym_compunit *builder = cu->get_builder ();
	      list_to_add
		= (cu->list_in_scope == builder->get_file_symbols ()
		   && cu->lang () == language_cplus
		   ? builder->get_global_symbols ()
		   : cu->list_in_scope);

	      /* The semantics of C++ state that "struct foo {
		 ... }" also defines a typedef for "foo".  */
	      if (cu->lang () == language_cplus
		  || cu->lang () == language_ada
		  || cu->lang () == language_d
		  || cu->lang () == language_rust)
		{
		  /* The symbol's name is already allocated along
		     with this objfile, so we don't need to
		     duplicate it for the type.  */
		  if (sym->type ()->name () == 0)
		    sym->type ()->set_name (sym->search_name ());
		}
	    }
	  break;
	case DW_TAG_unspecified_type:
	  if (cu->lang () == language_ada)
	    break;
	  [[fallthrough]];
	case DW_TAG_typedef:
	case DW_TAG_array_type:
	case DW_TAG_base_type:
	case DW_TAG_subrange_type:
	case DW_TAG_generic_subrange:
	  sym->set_aclass_index (LOC_TYPEDEF);
	  sym->set_domain (VAR_DOMAIN);
	  list_to_add = cu->list_in_scope;
	  break;
	case DW_TAG_enumerator:
	  attr = dwarf2_attr (die, DW_AT_const_value, cu);
	  if (attr != nullptr)
	    {
	      dwarf2_const_value (attr, sym, cu);
	    }

	  /* NOTE: carlton/2003-11-10: See comment above in the
	     DW_TAG_class_type, etc. block.  */

	  list_to_add
	    = (cu->list_in_scope == cu->get_builder ()->get_file_symbols ()
	       && cu->lang () == language_cplus
	       ? cu->get_builder ()->get_global_symbols ()
	       : cu->list_in_scope);
	  break;
	case DW_TAG_imported_declaration:
	case DW_TAG_namespace:
	  sym->set_aclass_index (LOC_TYPEDEF);
	  list_to_add = cu->get_builder ()->get_global_symbols ();
	  break;
	case DW_TAG_module:
	  sym->set_aclass_index (LOC_TYPEDEF);
	  sym->set_domain (MODULE_DOMAIN);
	  list_to_add = cu->get_builder ()->get_global_symbols ();
	  break;
	case DW_TAG_common_block:
	  sym->set_aclass_index (LOC_COMMON_BLOCK);
	  sym->set_domain (COMMON_BLOCK_DOMAIN);
	  add_symbol_to_list (sym, cu->list_in_scope);
	  break;
	default:
	  /* Not a tag we recognize.  Hopefully we aren't processing
	     trash data, but since we must specifically ignore things
	     we don't recognize, there is nothing else we should do at
	     this point.  */
	  complaint (_("unsupported tag: '%s'"),
		     dwarf_tag_name (die->tag));
	  break;
	}

      if (suppress_add)
	{
	  sym->hash_next = objfile->template_symbols;
	  objfile->template_symbols = sym;
	  list_to_add = NULL;
	}

      if (list_to_add != NULL)
	add_symbol_to_list (sym, list_to_add);

      /* For the benefit of old versions of GCC, check for anonymous
	 namespaces based on the demangled name.  */
      if (!cu->processing_has_namespace_info
	  && cu->lang () == language_cplus)
	cp_scan_for_anonymous_namespaces (cu->get_builder (), sym, objfile);
    }
  return (sym);
}

/* Given an attr with a DW_FORM_dataN value in host byte order,
   zero-extend it as appropriate for the symbol's type.  The DWARF
   standard (v4) is not entirely clear about the meaning of using
   DW_FORM_dataN for a constant with a signed type, where the type is
   wider than the data.  The conclusion of a discussion on the DWARF
   list was that this is unspecified.  We choose to always zero-extend
   because that is the interpretation long in use by GCC.  */

static gdb_byte *
dwarf2_const_value_data (const struct attribute *attr, struct obstack *obstack,
			 struct dwarf2_cu *cu, LONGEST *value, int bits)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  enum bfd_endian byte_order = bfd_big_endian (objfile->obfd.get ()) ?
				BFD_ENDIAN_BIG : BFD_ENDIAN_LITTLE;
  LONGEST l = attr->constant_value (0);

  if (bits < sizeof (*value) * 8)
    {
      l &= ((LONGEST) 1 << bits) - 1;
      *value = l;
    }
  else if (bits == sizeof (*value) * 8)
    *value = l;
  else
    {
      gdb_byte *bytes = (gdb_byte *) obstack_alloc (obstack, bits / 8);
      store_unsigned_integer (bytes, bits / 8, byte_order, l);
      return bytes;
    }

  return NULL;
}

/* Read a constant value from an attribute.  Either set *VALUE, or if
   the value does not fit in *VALUE, set *BYTES - either already
   allocated on the objfile obstack, or newly allocated on OBSTACK,
   or, set *BATON, if we translated the constant to a location
   expression.  */

static void
dwarf2_const_value_attr (const struct attribute *attr, struct type *type,
			 const char *name, struct obstack *obstack,
			 struct dwarf2_cu *cu,
			 LONGEST *value, const gdb_byte **bytes,
			 struct dwarf2_locexpr_baton **baton)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  struct comp_unit_head *cu_header = &cu->header;
  struct dwarf_block *blk;
  enum bfd_endian byte_order = (bfd_big_endian (objfile->obfd.get ()) ?
				BFD_ENDIAN_BIG : BFD_ENDIAN_LITTLE);

  *value = 0;
  *bytes = NULL;
  *baton = NULL;

  switch (attr->form)
    {
    case DW_FORM_addr:
    case DW_FORM_addrx:
    case DW_FORM_GNU_addr_index:
      {
	gdb_byte *data;

	if (type->length () != cu_header->addr_size)
	  dwarf2_const_value_length_mismatch_complaint (name,
							cu_header->addr_size,
							type->length ());
	/* Symbols of this form are reasonably rare, so we just
	   piggyback on the existing location code rather than writing
	   a new implementation of symbol_computed_ops.  */
	*baton = XOBNEW (obstack, struct dwarf2_locexpr_baton);
	(*baton)->per_objfile = per_objfile;
	(*baton)->per_cu = cu->per_cu;
	gdb_assert ((*baton)->per_cu);

	(*baton)->size = 2 + cu_header->addr_size;
	data = (gdb_byte *) obstack_alloc (obstack, (*baton)->size);
	(*baton)->data = data;

	data[0] = DW_OP_addr;
	store_unsigned_integer (&data[1], cu_header->addr_size,
				byte_order, (ULONGEST) attr->as_address ());
	data[cu_header->addr_size + 1] = DW_OP_stack_value;
      }
      break;
    case DW_FORM_string:
    case DW_FORM_strp:
    case DW_FORM_strx:
    case DW_FORM_GNU_str_index:
    case DW_FORM_GNU_strp_alt:
      /* The string is already allocated on the objfile obstack, point
	 directly to it.  */
      *bytes = (const gdb_byte *) attr->as_string ();
      break;
    case DW_FORM_block1:
    case DW_FORM_block2:
    case DW_FORM_block4:
    case DW_FORM_block:
    case DW_FORM_exprloc:
    case DW_FORM_data16:
      blk = attr->as_block ();
      if (type->length () != blk->size)
	dwarf2_const_value_length_mismatch_complaint (name, blk->size,
						      type->length ());
      *bytes = blk->data;
      break;

      /* The DW_AT_const_value attributes are supposed to carry the
	 symbol's value "represented as it would be on the target
	 architecture."  By the time we get here, it's already been
	 converted to host endianness, so we just need to sign- or
	 zero-extend it as appropriate.  */
    case DW_FORM_data1:
      *bytes = dwarf2_const_value_data (attr, obstack, cu, value, 8);
      break;
    case DW_FORM_data2:
      *bytes = dwarf2_const_value_data (attr, obstack, cu, value, 16);
      break;
    case DW_FORM_data4:
      *bytes = dwarf2_const_value_data (attr, obstack, cu, value, 32);
      break;
    case DW_FORM_data8:
      *bytes = dwarf2_const_value_data (attr, obstack, cu, value, 64);
      break;

    case DW_FORM_sdata:
    case DW_FORM_implicit_const:
      *value = attr->as_signed ();
      break;

    case DW_FORM_udata:
      *value = attr->as_unsigned ();
      break;

    default:
      complaint (_("unsupported const value attribute form: '%s'"),
		 dwarf_form_name (attr->form));
      *value = 0;
      break;
    }
}


/* Copy constant value from an attribute to a symbol.  */

static void
dwarf2_const_value (const struct attribute *attr, struct symbol *sym,
		    struct dwarf2_cu *cu)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  LONGEST value;
  const gdb_byte *bytes;
  struct dwarf2_locexpr_baton *baton;

  dwarf2_const_value_attr (attr, sym->type (),
			   sym->print_name (),
			   &objfile->objfile_obstack, cu,
			   &value, &bytes, &baton);

  if (baton != NULL)
    {
      SYMBOL_LOCATION_BATON (sym) = baton;
      sym->set_aclass_index (dwarf2_locexpr_index);
    }
  else if (bytes != NULL)
    {
      sym->set_value_bytes (bytes);
      sym->set_aclass_index (LOC_CONST_BYTES);
    }
  else
    {
      sym->set_value_longest (value);
      sym->set_aclass_index (LOC_CONST);
    }
}

/* Return the type of the die in question using its DW_AT_type attribute.  */

static struct type *
die_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *type_attr;

  type_attr = dwarf2_attr (die, DW_AT_type, cu);
  if (!type_attr)
    {
      struct objfile *objfile = cu->per_objfile->objfile;
      /* A missing DW_AT_type represents a void type.  */
      return builtin_type (objfile)->builtin_void;
    }

  return lookup_die_type (die, type_attr, cu);
}

/* True iff CU's producer generates GNAT Ada auxiliary information
   that allows to find parallel types through that information instead
   of having to do expensive parallel lookups by type name.  */

static int
need_gnat_info (struct dwarf2_cu *cu)
{
  /* Assume that the Ada compiler was GNAT, which always produces
     the auxiliary information.  */
  return (cu->lang () == language_ada);
}

/* Return the auxiliary type of the die in question using its
   DW_AT_GNAT_descriptive_type attribute.  Returns NULL if the
   attribute is not present.  */

static struct type *
die_descriptive_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *type_attr;

  type_attr = dwarf2_attr (die, DW_AT_GNAT_descriptive_type, cu);
  if (!type_attr)
    return NULL;

  return lookup_die_type (die, type_attr, cu);
}

/* If DIE has a descriptive_type attribute, then set the TYPE's
   descriptive type accordingly.  */

static void
set_descriptive_type (struct type *type, struct die_info *die,
		      struct dwarf2_cu *cu)
{
  struct type *descriptive_type = die_descriptive_type (die, cu);

  if (descriptive_type)
    {
      ALLOCATE_GNAT_AUX_TYPE (type);
      TYPE_DESCRIPTIVE_TYPE (type) = descriptive_type;
    }
}

/* Return the containing type of the die in question using its
   DW_AT_containing_type attribute.  */

static struct type *
die_containing_type (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *type_attr;
  struct objfile *objfile = cu->per_objfile->objfile;

  type_attr = dwarf2_attr (die, DW_AT_containing_type, cu);
  if (!type_attr)
    error (_("Dwarf Error: Problem turning containing type into gdb type "
	     "[in module %s]"), objfile_name (objfile));

  return lookup_die_type (die, type_attr, cu);
}

/* Return an error marker type to use for the ill formed type in DIE/CU.  */

static struct type *
build_error_marker_type (struct dwarf2_cu *cu, struct die_info *die)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  char *saved;

  std::string message
    = string_printf (_("<unknown type in %s, CU %s, DIE %s>"),
		     objfile_name (objfile),
		     sect_offset_str (cu->header.sect_off),
		     sect_offset_str (die->sect_off));
  saved = obstack_strdup (&objfile->objfile_obstack, message);

  return type_allocator (objfile, cu->lang ()).new_type (TYPE_CODE_ERROR,
							 0, saved);
}

/* Look up the type of DIE in CU using its type attribute ATTR.
   ATTR must be one of: DW_AT_type, DW_AT_GNAT_descriptive_type,
   DW_AT_containing_type.
   If there is no type substitute an error marker.  */

static struct type *
lookup_die_type (struct die_info *die, const struct attribute *attr,
		 struct dwarf2_cu *cu)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  struct type *this_type;

  gdb_assert (attr->name == DW_AT_type
	      || attr->name == DW_AT_GNAT_descriptive_type
	      || attr->name == DW_AT_containing_type);

  /* First see if we have it cached.  */

  if (attr->form == DW_FORM_GNU_ref_alt)
    {
      struct dwarf2_per_cu_data *per_cu;
      sect_offset sect_off = attr->get_ref_die_offset ();

      per_cu = dwarf2_find_containing_comp_unit (sect_off, 1,
						 per_objfile->per_bfd);
      this_type = get_die_type_at_offset (sect_off, per_cu, per_objfile);
    }
  else if (attr->form_is_ref ())
    {
      sect_offset sect_off = attr->get_ref_die_offset ();

      this_type = get_die_type_at_offset (sect_off, cu->per_cu, per_objfile);
    }
  else if (attr->form == DW_FORM_ref_sig8)
    {
      ULONGEST signature = attr->as_signature ();

      return get_signatured_type (die, signature, cu);
    }
  else
    {
      complaint (_("Dwarf Error: Bad type attribute %s in DIE"
		   " at %s [in module %s]"),
		 dwarf_attr_name (attr->name), sect_offset_str (die->sect_off),
		 objfile_name (objfile));
      return build_error_marker_type (cu, die);
    }

  /* If not cached we need to read it in.  */

  if (this_type == NULL)
    {
      struct die_info *type_die = NULL;
      struct dwarf2_cu *type_cu = cu;

      if (attr->form_is_ref ())
	type_die = follow_die_ref (die, attr, &type_cu);
      if (type_die == NULL)
	return build_error_marker_type (cu, die);
      /* If we find the type now, it's probably because the type came
	 from an inter-CU reference and the type's CU got expanded before
	 ours.  */
      this_type = read_type_die (type_die, type_cu);
    }

  /* If we still don't have a type use an error marker.  */

  if (this_type == NULL)
    return build_error_marker_type (cu, die);

  return this_type;
}

/* Return the type in DIE, CU.
   Returns NULL for invalid types.

   This first does a lookup in die_type_hash,
   and only reads the die in if necessary.

   NOTE: This can be called when reading in partial or full symbols.  */

static struct type *
read_type_die (struct die_info *die, struct dwarf2_cu *cu)
{
  struct type *this_type;

  this_type = get_die_type (die, cu);
  if (this_type)
    return this_type;

  return read_type_die_1 (die, cu);
}

/* Read the type in DIE, CU.
   Returns NULL for invalid types.  */

static struct type *
read_type_die_1 (struct die_info *die, struct dwarf2_cu *cu)
{
  struct type *this_type = NULL;

  switch (die->tag)
    {
    case DW_TAG_class_type:
    case DW_TAG_interface_type:
    case DW_TAG_structure_type:
    case DW_TAG_union_type:
      this_type = read_structure_type (die, cu);
      break;
    case DW_TAG_enumeration_type:
      this_type = read_enumeration_type (die, cu);
      break;
    case DW_TAG_entry_point:
    case DW_TAG_subprogram:
    case DW_TAG_subroutine_type:
    case DW_TAG_inlined_subroutine:
      this_type = read_subroutine_type (die, cu);
      break;
    case DW_TAG_array_type:
      this_type = read_array_type (die, cu);
      break;
    case DW_TAG_set_type:
      this_type = read_set_type (die, cu);
      break;
    case DW_TAG_pointer_type:
      this_type = read_tag_pointer_type (die, cu);
      break;
    case DW_TAG_ptr_to_member_type:
      this_type = read_tag_ptr_to_member_type (die, cu);
      break;
    case DW_TAG_reference_type:
      this_type = read_tag_reference_type (die, cu, TYPE_CODE_REF);
      break;
    case DW_TAG_rvalue_reference_type:
      this_type = read_tag_reference_type (die, cu, TYPE_CODE_RVALUE_REF);
      break;
    case DW_TAG_const_type:
      this_type = read_tag_const_type (die, cu);
      break;
    case DW_TAG_volatile_type:
      this_type = read_tag_volatile_type (die, cu);
      break;
    case DW_TAG_restrict_type:
      this_type = read_tag_restrict_type (die, cu);
      break;
    case DW_TAG_string_type:
      this_type = read_tag_string_type (die, cu);
      break;
    case DW_TAG_typedef:
      this_type = read_typedef (die, cu);
      break;
    case DW_TAG_generic_subrange:
    case DW_TAG_subrange_type:
      this_type = read_subrange_type (die, cu);
      break;
    case DW_TAG_base_type:
      this_type = read_base_type (die, cu);
      break;
    case DW_TAG_unspecified_type:
      this_type = read_unspecified_type (die, cu);
      break;
    case DW_TAG_namespace:
      this_type = read_namespace_type (die, cu);
      break;
    case DW_TAG_module:
      this_type = read_module_type (die, cu);
      break;
    case DW_TAG_atomic_type:
      this_type = read_tag_atomic_type (die, cu);
      break;
    default:
      complaint (_("unexpected tag in read_type_die: '%s'"),
		 dwarf_tag_name (die->tag));
      break;
    }

  return this_type;
}

/* See if we can figure out if the class lives in a namespace.  We do
   this by looking for a member function; its demangled name will
   contain namespace info, if there is any.
   Return the computed name or NULL.
   Space for the result is allocated on the objfile's obstack.
   This is the full-die version of guess_partial_die_structure_name.
   In this case we know DIE has no useful parent.  */

static const char *
guess_full_die_structure_name (struct die_info *die, struct dwarf2_cu *cu)
{
  struct die_info *spec_die;
  struct dwarf2_cu *spec_cu;
  struct die_info *child;
  struct objfile *objfile = cu->per_objfile->objfile;

  spec_cu = cu;
  spec_die = die_specification (die, &spec_cu);
  if (spec_die != NULL)
    {
      die = spec_die;
      cu = spec_cu;
    }

  for (child = die->child;
       child != NULL;
       child = child->sibling)
    {
      if (child->tag == DW_TAG_subprogram)
	{
	  const char *linkage_name = dw2_linkage_name (child, cu);

	  if (linkage_name != NULL)
	    {
	      gdb::unique_xmalloc_ptr<char> actual_name
		(cu->language_defn->class_name_from_physname (linkage_name));
	      const char *name = NULL;

	      if (actual_name != NULL)
		{
		  const char *die_name = dwarf2_name (die, cu);

		  if (die_name != NULL
		      && strcmp (die_name, actual_name.get ()) != 0)
		    {
		      /* Strip off the class name from the full name.
			 We want the prefix.  */
		      int die_name_len = strlen (die_name);
		      int actual_name_len = strlen (actual_name.get ());
		      const char *ptr = actual_name.get ();

		      /* Test for '::' as a sanity check.  */
		      if (actual_name_len > die_name_len + 2
			  && ptr[actual_name_len - die_name_len - 1] == ':')
			name = obstack_strndup (
			  &objfile->per_bfd->storage_obstack,
			  ptr, actual_name_len - die_name_len - 2);
		    }
		}
	      return name;
	    }
	}
    }

  return NULL;
}

/* GCC might emit a nameless typedef that has a linkage name.  Determine the
   prefix part in such case.  See
   http://gcc.gnu.org/bugzilla/show_bug.cgi?id=47510.  */

static const char *
anonymous_struct_prefix (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *attr;
  const char *base;

  if (die->tag != DW_TAG_class_type && die->tag != DW_TAG_interface_type
      && die->tag != DW_TAG_structure_type && die->tag != DW_TAG_union_type)
    return NULL;

  if (dwarf2_string_attr (die, DW_AT_name, cu) != NULL)
    return NULL;

  attr = dw2_linkage_name_attr (die, cu);
  const char *attr_name = attr->as_string ();
  if (attr == NULL || attr_name == NULL)
    return NULL;

  /* dwarf2_name had to be already called.  */
  gdb_assert (attr->canonical_string_p ());

  /* Strip the base name, keep any leading namespaces/classes.  */
  base = strrchr (attr_name, ':');
  if (base == NULL || base == attr_name || base[-1] != ':')
    return "";

  struct objfile *objfile = cu->per_objfile->objfile;
  return obstack_strndup (&objfile->per_bfd->storage_obstack,
			  attr_name,
			  &base[-1] - attr_name);
}

/* Return the name of the namespace/class that DIE is defined within,
   or "" if we can't tell.  The caller should not xfree the result.

   For example, if we're within the method foo() in the following
   code:

   namespace N {
     class C {
       void foo () {
       }
     };
   }

   then determine_prefix on foo's die will return "N::C".  */

static const char *
determine_prefix (struct die_info *die, struct dwarf2_cu *cu)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct die_info *parent, *spec_die;
  struct dwarf2_cu *spec_cu;
  struct type *parent_type;
  const char *retval;

  if (cu->lang () != language_cplus
      && cu->lang () != language_fortran
      && cu->lang () != language_d
      && cu->lang () != language_rust)
    return "";

  retval = anonymous_struct_prefix (die, cu);
  if (retval)
    return retval;

  /* We have to be careful in the presence of DW_AT_specification.
     For example, with GCC 3.4, given the code

     namespace N {
       void foo() {
	 // Definition of N::foo.
       }
     }

     then we'll have a tree of DIEs like this:

     1: DW_TAG_compile_unit
       2: DW_TAG_namespace        // N
	 3: DW_TAG_subprogram     // declaration of N::foo
       4: DW_TAG_subprogram       // definition of N::foo
	    DW_AT_specification   // refers to die #3

     Thus, when processing die #4, we have to pretend that we're in
     the context of its DW_AT_specification, namely the context of die
     #3.  */
  spec_cu = cu;
  spec_die = die_specification (die, &spec_cu);
  if (spec_die == NULL)
    parent = die->parent;
  else
    {
      parent = spec_die->parent;
      cu = spec_cu;
    }

  if (parent == NULL)
    return "";
  else if (parent->building_fullname)
    {
      const char *name;
      const char *parent_name;

      /* It has been seen on RealView 2.2 built binaries,
	 DW_TAG_template_type_param types actually _defined_ as
	 children of the parent class:

	 enum E {};
	 template class <class Enum> Class{};
	 Class<enum E> class_e;

	 1: DW_TAG_class_type (Class)
	   2: DW_TAG_enumeration_type (E)
	     3: DW_TAG_enumerator (enum1:0)
	     3: DW_TAG_enumerator (enum2:1)
	     ...
	   2: DW_TAG_template_type_param
	      DW_AT_type  DW_FORM_ref_udata (E)

	 Besides being broken debug info, it can put GDB into an
	 infinite loop.  Consider:

	 When we're building the full name for Class<E>, we'll start
	 at Class, and go look over its template type parameters,
	 finding E.  We'll then try to build the full name of E, and
	 reach here.  We're now trying to build the full name of E,
	 and look over the parent DIE for containing scope.  In the
	 broken case, if we followed the parent DIE of E, we'd again
	 find Class, and once again go look at its template type
	 arguments, etc., etc.  Simply don't consider such parent die
	 as source-level parent of this die (it can't be, the language
	 doesn't allow it), and break the loop here.  */
      name = dwarf2_name (die, cu);
      parent_name = dwarf2_name (parent, cu);
      complaint (_("template param type '%s' defined within parent '%s'"),
		 name ? name : "<unknown>",
		 parent_name ? parent_name : "<unknown>");
      return "";
    }
  else
    switch (parent->tag)
      {
      case DW_TAG_namespace:
	parent_type = read_type_die (parent, cu);
	/* GCC 4.0 and 4.1 had a bug (PR c++/28460) where they generated bogus
	   DW_TAG_namespace DIEs with a name of "::" for the global namespace.
	   Work around this problem here.  */
	if (cu->lang () == language_cplus
	    && strcmp (parent_type->name (), "::") == 0)
	  return "";
	/* We give a name to even anonymous namespaces.  */
	return parent_type->name ();
      case DW_TAG_class_type:
      case DW_TAG_interface_type:
      case DW_TAG_structure_type:
      case DW_TAG_union_type:
      case DW_TAG_module:
	parent_type = read_type_die (parent, cu);
	if (parent_type->name () != NULL)
	  return parent_type->name ();
	else
	  /* An anonymous structure is only allowed non-static data
	     members; no typedefs, no member functions, et cetera.
	     So it does not need a prefix.  */
	  return "";
      case DW_TAG_compile_unit:
      case DW_TAG_partial_unit:
	/* gcc-4.5 -gdwarf-4 can drop the enclosing namespace.  Cope.  */
	if (cu->lang () == language_cplus
	    && !per_objfile->per_bfd->types.empty ()
	    && die->child != NULL
	    && (die->tag == DW_TAG_class_type
		|| die->tag == DW_TAG_structure_type
		|| die->tag == DW_TAG_union_type))
	  {
	    const char *name = guess_full_die_structure_name (die, cu);
	    if (name != NULL)
	      return name;
	  }
	return "";
      case DW_TAG_subprogram:
	/* Nested subroutines in Fortran get a prefix with the name
	   of the parent's subroutine.  Entry points are prefixed by the
	   parent's namespace.  */
	if (cu->lang () == language_fortran)
	  {
	    if ((die->tag ==  DW_TAG_subprogram)
		&& (dwarf2_name (parent, cu) != NULL))
	      return dwarf2_name (parent, cu);
	    else if (die->tag == DW_TAG_entry_point)
	      return determine_prefix (parent, cu);
	  }
	return "";
      case DW_TAG_enumeration_type:
	parent_type = read_type_die (parent, cu);
	if (parent_type->is_declared_class ())
	  {
	    if (parent_type->name () != NULL)
	      return parent_type->name ();
	    return "";
	  }
	[[fallthrough]];
      default:
	return determine_prefix (parent, cu);
      }
}

/* Return a newly-allocated string formed by concatenating PREFIX and SUFFIX
   with appropriate separator.  If PREFIX or SUFFIX is NULL or empty, then
   simply copy the SUFFIX or PREFIX, respectively.  If OBS is non-null, perform
   an obconcat, otherwise allocate storage for the result.  The CU argument is
   used to determine the language and hence, the appropriate separator.  */

#define MAX_SEP_LEN 7  /* strlen ("__") + strlen ("_MOD_")  */

static char *
typename_concat (struct obstack *obs, const char *prefix, const char *suffix,
		 int physname, struct dwarf2_cu *cu)
{
  const char *lead = "";
  const char *sep;

  if (suffix == NULL || suffix[0] == '\0'
      || prefix == NULL || prefix[0] == '\0')
    sep = "";
  else if (cu->lang () == language_d)
    {
      /* For D, the 'main' function could be defined in any module, but it
	 should never be prefixed.  */
      if (strcmp (suffix, "D main") == 0)
	{
	  prefix = "";
	  sep = "";
	}
      else
	sep = ".";
    }
  else if (cu->lang () == language_fortran && physname)
    {
      /* This is gfortran specific mangling.  Normally DW_AT_linkage_name or
	 DW_AT_MIPS_linkage_name is preferred and used instead.  */

      lead = "__";
      sep = "_MOD_";
    }
  else
    sep = "::";

  if (prefix == NULL)
    prefix = "";
  if (suffix == NULL)
    suffix = "";

  if (obs == NULL)
    {
      char *retval
	= ((char *)
	   xmalloc (strlen (prefix) + MAX_SEP_LEN + strlen (suffix) + 1));

      strcpy (retval, lead);
      strcat (retval, prefix);
      strcat (retval, sep);
      strcat (retval, suffix);
      return retval;
    }
  else
    {
      /* We have an obstack.  */
      return obconcat (obs, lead, prefix, sep, suffix, (char *) NULL);
    }
}

/* Return a generic name for a DW_TAG_template_type_param or
   DW_TAG_template_value_param tag, missing a DW_AT_name attribute.  We do this
   per parent, so each function/class/struct template will have their own set
   of template parameters named <unnnamed0>, <unnamed1>, ... where the
   enumeration starts at 0 and represents the position of the template tag in
   the list of unnamed template tags for this parent, counting both, type and
   value tags.  */

static const char *
unnamed_template_tag_name (die_info *die, dwarf2_cu *cu)
{
  if (die->parent == nullptr)
    return nullptr;

  /* Count the parent types unnamed template type and value children until, we
     arrive at our entry.  */
  size_t nth_unnamed = 0;

  die_info *child = die->parent->child;
  while (child != die)
  {
    gdb_assert (child != nullptr);
    if (child->tag == DW_TAG_template_type_param
	|| child->tag == DW_TAG_template_value_param)
      {
	if (dwarf2_attr (child, DW_AT_name, cu) == nullptr)
	  ++nth_unnamed;
      }
    child = child->sibling;
  }

  const std::string name_str = "<unnamed" + std::to_string (nth_unnamed) + ">";
  return cu->per_objfile->objfile->intern (name_str.c_str ());
}

/* Get name of a die, return NULL if not found.  */

static const char *
dwarf2_canonicalize_name (const char *name, struct dwarf2_cu *cu,
			  struct objfile *objfile)
{
  if (name == nullptr)
    return name;

  if (cu->lang () == language_cplus)
    {
      gdb::unique_xmalloc_ptr<char> canon_name
	= cp_canonicalize_string (name);

      if (canon_name != nullptr)
	name = objfile->intern (canon_name.get ());
    }
  else if (cu->lang () == language_c)
    {
      gdb::unique_xmalloc_ptr<char> canon_name
	= c_canonicalize_name (name);

      if (canon_name != nullptr)
	name = objfile->intern (canon_name.get ());
    }

  return name;
}

/* Get name of a die, return NULL if not found.
   Anonymous namespaces are converted to their magic string.  */

static const char *
dwarf2_name (struct die_info *die, struct dwarf2_cu *cu)
{
  struct attribute *attr;
  struct objfile *objfile = cu->per_objfile->objfile;

  attr = dwarf2_attr (die, DW_AT_name, cu);
  const char *attr_name = attr == nullptr ? nullptr : attr->as_string ();
  if (attr_name == nullptr
      && die->tag != DW_TAG_namespace
      && die->tag != DW_TAG_class_type
      && die->tag != DW_TAG_interface_type
      && die->tag != DW_TAG_structure_type
      && die->tag != DW_TAG_namelist
      && die->tag != DW_TAG_union_type
      && die->tag != DW_TAG_template_type_param
      && die->tag != DW_TAG_template_value_param)
    return NULL;

  switch (die->tag)
    {
      /* A member's name should not be canonicalized.  This is a bit
	 of a hack, in that normally it should not be possible to run
	 into this situation; however, the dw2-unusual-field-names.exp
	 test creates custom DWARF that does.  */
    case DW_TAG_member:
    case DW_TAG_compile_unit:
    case DW_TAG_partial_unit:
      /* Compilation units have a DW_AT_name that is a filename, not
	 a source language identifier.  */
    case DW_TAG_enumeration_type:
    case DW_TAG_enumerator:
      /* These tags always have simple identifiers already; no need
	 to canonicalize them.  */
      return attr_name;

    case DW_TAG_namespace:
      if (attr_name != nullptr)
	return attr_name;
      return CP_ANONYMOUS_NAMESPACE_STR;

    /* DWARF does not actually require template tags to have a name.  */
    case DW_TAG_template_type_param:
    case DW_TAG_template_value_param:
      if (attr_name == nullptr)
	return unnamed_template_tag_name (die, cu);
      [[fallthrough]];
    case DW_TAG_class_type:
    case DW_TAG_interface_type:
    case DW_TAG_structure_type:
    case DW_TAG_union_type:
    case DW_TAG_namelist:
      /* Some GCC versions emit spurious DW_AT_name attributes for unnamed
	 structures or unions.  These were of the form "._%d" in GCC 4.1,
	 or simply "<anonymous struct>" or "<anonymous union>" in GCC 4.3
	 and GCC 4.4.  We work around this problem by ignoring these.  */
      if (attr_name != nullptr
	  && (startswith (attr_name, "._")
	      || startswith (attr_name, "<anonymous")))
	return NULL;

      /* GCC might emit a nameless typedef that has a linkage name.  See
	 http://gcc.gnu.org/bugzilla/show_bug.cgi?id=47510.  */
      if (!attr || attr_name == NULL)
	{
	  attr = dw2_linkage_name_attr (die, cu);
	  attr_name = attr == nullptr ? nullptr : attr->as_string ();
	  if (attr == NULL || attr_name == NULL)
	    return NULL;

	  /* Avoid demangling attr_name the second time on a second
	     call for the same DIE.  */
	  if (!attr->canonical_string_p ())
	    {
	      gdb::unique_xmalloc_ptr<char> demangled
		(gdb_demangle (attr_name, DMGL_TYPES));
	      if (demangled == nullptr)
		return nullptr;

	      attr->set_string_canonical (objfile->intern (demangled.get ()));
	      attr_name = attr->as_string ();
	    }

	  /* Strip any leading namespaces/classes, keep only the
	     base name.  DW_AT_name for named DIEs does not
	     contain the prefixes.  */
	  const char *base = strrchr (attr_name, ':');
	  if (base && base > attr_name && base[-1] == ':')
	    return &base[1];
	  else
	    return attr_name;
	}
      break;

    default:
      break;
    }

  if (!attr->canonical_string_p ())
    attr->set_string_canonical (dwarf2_canonicalize_name (attr_name, cu,
							  objfile));
  return attr->as_string ();
}

/* Return the die that this die in an extension of, or NULL if there
   is none.  *EXT_CU is the CU containing DIE on input, and the CU
   containing the return value on output.  */

static struct die_info *
dwarf2_extension (struct die_info *die, struct dwarf2_cu **ext_cu)
{
  struct attribute *attr;

  attr = dwarf2_attr (die, DW_AT_extension, *ext_cu);
  if (attr == NULL)
    return NULL;

  return follow_die_ref (die, attr, ext_cu);
}

static void
store_in_ref_table (struct die_info *die, struct dwarf2_cu *cu)
{
  void **slot;

  slot = htab_find_slot_with_hash (cu->die_hash, die,
				   to_underlying (die->sect_off),
				   INSERT);

  *slot = die;
}

/* Follow reference or signature attribute ATTR of SRC_DIE.
   On entry *REF_CU is the CU of SRC_DIE.
   On exit *REF_CU is the CU of the result.  */

static struct die_info *
follow_die_ref_or_sig (struct die_info *src_die, const struct attribute *attr,
		       struct dwarf2_cu **ref_cu)
{
  struct die_info *die;

  if (attr->form_is_ref ())
    die = follow_die_ref (src_die, attr, ref_cu);
  else if (attr->form == DW_FORM_ref_sig8)
    die = follow_die_sig (src_die, attr, ref_cu);
  else
    {
      src_die->error_dump ();
      error (_("Dwarf Error: Expected reference attribute [in module %s]"),
	     objfile_name ((*ref_cu)->per_objfile->objfile));
    }

  return die;
}

/* Follow reference OFFSET.
   On entry *REF_CU is the CU of the source die referencing OFFSET.
   On exit *REF_CU is the CU of the result.
   Returns NULL if OFFSET is invalid.  */

static struct die_info *
follow_die_offset (sect_offset sect_off, int offset_in_dwz,
		   struct dwarf2_cu **ref_cu)
{
  struct die_info temp_die;
  struct dwarf2_cu *target_cu, *cu = *ref_cu;
  dwarf2_per_objfile *per_objfile = cu->per_objfile;

  gdb_assert (cu->per_cu != NULL);

  target_cu = cu;

  dwarf_read_debug_printf_v ("source CU offset: %s, target offset: %s, "
			     "source CU contains target offset: %d",
			     sect_offset_str (cu->per_cu->sect_off),
			     sect_offset_str (sect_off),
			     cu->header.offset_in_cu_p (sect_off));

  if (cu->per_cu->is_debug_types)
    {
      /* .debug_types CUs cannot reference anything outside their CU.
	 If they need to, they have to reference a signatured type via
	 DW_FORM_ref_sig8.  */
      if (!cu->header.offset_in_cu_p (sect_off))
	return NULL;
    }
  else if (offset_in_dwz != cu->per_cu->is_dwz
	   || !cu->header.offset_in_cu_p (sect_off))
    {
      struct dwarf2_per_cu_data *per_cu;

      per_cu = dwarf2_find_containing_comp_unit (sect_off, offset_in_dwz,
						 per_objfile->per_bfd);

      dwarf_read_debug_printf_v ("target CU offset: %s, "
				 "target CU DIEs loaded: %d",
				 sect_offset_str (per_cu->sect_off),
				 per_objfile->get_cu (per_cu) != nullptr);

      /* If necessary, add it to the queue and load its DIEs.

	 Even if maybe_queue_comp_unit doesn't require us to load the CU's DIEs,
	 it doesn't mean they are currently loaded.  Since we require them
	 to be loaded, we must check for ourselves.  */
      if (maybe_queue_comp_unit (cu, per_cu, per_objfile, cu->lang ())
	  || per_objfile->get_cu (per_cu) == nullptr)
	load_full_comp_unit (per_cu, per_objfile, per_objfile->get_cu (per_cu),
			     false, cu->lang ());

      target_cu = per_objfile->get_cu (per_cu);
      gdb_assert (target_cu != nullptr);
    }
  else if (cu->dies == NULL)
    {
      /* We're loading full DIEs during partial symbol reading.  */
      load_full_comp_unit (cu->per_cu, per_objfile, cu, false,
			   language_minimal);
    }

  *ref_cu = target_cu;
  temp_die.sect_off = sect_off;

  return (struct die_info *) htab_find_with_hash (target_cu->die_hash,
						  &temp_die,
						  to_underlying (sect_off));
}

/* Follow reference attribute ATTR of SRC_DIE.
   On entry *REF_CU is the CU of SRC_DIE.
   On exit *REF_CU is the CU of the result.  */

static struct die_info *
follow_die_ref (struct die_info *src_die, const struct attribute *attr,
		struct dwarf2_cu **ref_cu)
{
  sect_offset sect_off = attr->get_ref_die_offset ();
  struct dwarf2_cu *cu = *ref_cu;
  struct die_info *die;

  if (attr->form != DW_FORM_GNU_ref_alt && src_die->sect_off == sect_off)
    {
      /* Self-reference, we're done.  */
      return src_die;
    }

  die = follow_die_offset (sect_off,
			   (attr->form == DW_FORM_GNU_ref_alt
			    || cu->per_cu->is_dwz),
			   ref_cu);
  if (!die)
    error (_("Dwarf Error: Cannot find DIE at %s referenced from DIE "
	   "at %s [in module %s]"),
	   sect_offset_str (sect_off), sect_offset_str (src_die->sect_off),
	   objfile_name (cu->per_objfile->objfile));

  return die;
}

/* See read.h.  */

struct dwarf2_locexpr_baton
dwarf2_fetch_die_loc_sect_off (sect_offset sect_off,
			       dwarf2_per_cu_data *per_cu,
			       dwarf2_per_objfile *per_objfile,
			       gdb::function_view<CORE_ADDR ()> get_frame_pc,
			       bool resolve_abstract_p)
{
  struct die_info *die;
  struct attribute *attr;
  struct dwarf2_locexpr_baton retval;
  struct objfile *objfile = per_objfile->objfile;

  dwarf2_cu *cu = per_objfile->get_cu (per_cu);
  if (cu == nullptr)
    cu = load_cu (per_cu, per_objfile, false);

  if (cu == nullptr)
    {
      /* We shouldn't get here for a dummy CU, but don't crash on the user.
	 Instead just throw an error, not much else we can do.  */
      error (_("Dwarf Error: Dummy CU at %s referenced in module %s"),
	     sect_offset_str (sect_off), objfile_name (objfile));
    }

  die = follow_die_offset (sect_off, per_cu->is_dwz, &cu);
  if (!die)
    error (_("Dwarf Error: Cannot find DIE at %s referenced in module %s"),
	   sect_offset_str (sect_off), objfile_name (objfile));

  attr = dwarf2_attr (die, DW_AT_location, cu);
  if (!attr && resolve_abstract_p
      && (per_objfile->per_bfd->abstract_to_concrete.find (die->sect_off)
	  != per_objfile->per_bfd->abstract_to_concrete.end ()))
    {
      CORE_ADDR pc = get_frame_pc ();

      for (const auto &cand_off
	     : per_objfile->per_bfd->abstract_to_concrete[die->sect_off])
	{
	  struct dwarf2_cu *cand_cu = cu;
	  struct die_info *cand
	    = follow_die_offset (cand_off, per_cu->is_dwz, &cand_cu);
	  if (!cand
	      || !cand->parent
	      || cand->parent->tag != DW_TAG_subprogram)
	    continue;

	  unrelocated_addr unrel_low, unrel_high;
	  get_scope_pc_bounds (cand->parent, &unrel_low, &unrel_high, cu);
	  if (unrel_low == ((unrelocated_addr) -1))
	    continue;
	  CORE_ADDR pc_low = per_objfile->relocate (unrel_low);
	  CORE_ADDR pc_high = per_objfile->relocate (unrel_high);
	  if (!(pc_low <= pc && pc < pc_high))
	    continue;

	  die = cand;
	  attr = dwarf2_attr (die, DW_AT_location, cu);
	  break;
	}
    }

  if (!attr)
    {
      /* DWARF: "If there is no such attribute, then there is no effect.".
	 DATA is ignored if SIZE is 0.  */

      retval.data = NULL;
      retval.size = 0;
    }
  else if (attr->form_is_section_offset ())
    {
      struct dwarf2_loclist_baton loclist_baton;
      CORE_ADDR pc = get_frame_pc ();
      size_t size;

      fill_in_loclist_baton (cu, &loclist_baton, attr);

      retval.data = dwarf2_find_location_expression (&loclist_baton,
						     &size, pc);
      retval.size = size;
    }
  else
    {
      if (!attr->form_is_block ())
	error (_("Dwarf Error: DIE at %s referenced in module %s "
		 "is neither DW_FORM_block* nor DW_FORM_exprloc"),
	       sect_offset_str (sect_off), objfile_name (objfile));

      struct dwarf_block *block = attr->as_block ();
      retval.data = block->data;
      retval.size = block->size;
    }
  retval.per_objfile = per_objfile;
  retval.per_cu = cu->per_cu;

  per_objfile->age_comp_units ();

  return retval;
}

/* See read.h.  */

struct dwarf2_locexpr_baton
dwarf2_fetch_die_loc_cu_off (cu_offset offset_in_cu,
			     dwarf2_per_cu_data *per_cu,
			     dwarf2_per_objfile *per_objfile,
			     gdb::function_view<CORE_ADDR ()> get_frame_pc)
{
  sect_offset sect_off = per_cu->sect_off + to_underlying (offset_in_cu);

  return dwarf2_fetch_die_loc_sect_off (sect_off, per_cu, per_objfile,
					get_frame_pc);
}

/* Write a constant of a given type as target-ordered bytes into
   OBSTACK.  */

static const gdb_byte *
write_constant_as_bytes (struct obstack *obstack,
			 enum bfd_endian byte_order,
			 struct type *type,
			 ULONGEST value,
			 LONGEST *len)
{
  gdb_byte *result;

  *len = type->length ();
  result = (gdb_byte *) obstack_alloc (obstack, *len);
  store_unsigned_integer (result, *len, byte_order, value);

  return result;
}

/* See read.h.  */

const gdb_byte *
dwarf2_fetch_constant_bytes (sect_offset sect_off,
			     dwarf2_per_cu_data *per_cu,
			     dwarf2_per_objfile *per_objfile,
			     obstack *obstack,
			     LONGEST *len)
{
  struct die_info *die;
  struct attribute *attr;
  const gdb_byte *result = NULL;
  struct type *type;
  LONGEST value;
  enum bfd_endian byte_order;
  struct objfile *objfile = per_objfile->objfile;

  dwarf2_cu *cu = per_objfile->get_cu (per_cu);
  if (cu == nullptr)
    cu = load_cu (per_cu, per_objfile, false);

  if (cu == nullptr)
    {
      /* We shouldn't get here for a dummy CU, but don't crash on the user.
	 Instead just throw an error, not much else we can do.  */
      error (_("Dwarf Error: Dummy CU at %s referenced in module %s"),
	     sect_offset_str (sect_off), objfile_name (objfile));
    }

  die = follow_die_offset (sect_off, per_cu->is_dwz, &cu);
  if (!die)
    error (_("Dwarf Error: Cannot find DIE at %s referenced in module %s"),
	   sect_offset_str (sect_off), objfile_name (objfile));

  attr = dwarf2_attr (die, DW_AT_const_value, cu);
  if (attr == NULL)
    return NULL;

  byte_order = (bfd_big_endian (objfile->obfd.get ())
		? BFD_ENDIAN_BIG : BFD_ENDIAN_LITTLE);

  switch (attr->form)
    {
    case DW_FORM_addr:
    case DW_FORM_addrx:
    case DW_FORM_GNU_addr_index:
      {
	gdb_byte *tem;

	*len = cu->header.addr_size;
	tem = (gdb_byte *) obstack_alloc (obstack, *len);
	store_unsigned_integer (tem, *len, byte_order,
				(ULONGEST) attr->as_address ());
	result = tem;
      }
      break;
    case DW_FORM_string:
    case DW_FORM_strp:
    case DW_FORM_strx:
    case DW_FORM_GNU_str_index:
    case DW_FORM_GNU_strp_alt:
      /* The string is already allocated on the objfile obstack, point
	 directly to it.  */
      {
	const char *attr_name = attr->as_string ();
	result = (const gdb_byte *) attr_name;
	*len = strlen (attr_name);
      }
      break;
    case DW_FORM_block1:
    case DW_FORM_block2:
    case DW_FORM_block4:
    case DW_FORM_block:
    case DW_FORM_exprloc:
    case DW_FORM_data16:
      {
	struct dwarf_block *block = attr->as_block ();
	result = block->data;
	*len = block->size;
      }
      break;

      /* The DW_AT_const_value attributes are supposed to carry the
	 symbol's value "represented as it would be on the target
	 architecture."  By the time we get here, it's already been
	 converted to host endianness, so we just need to sign- or
	 zero-extend it as appropriate.  */
    case DW_FORM_data1:
      type = die_type (die, cu);
      result = dwarf2_const_value_data (attr, obstack, cu, &value, 8);
      if (result == NULL)
	result = write_constant_as_bytes (obstack, byte_order,
					  type, value, len);
      break;
    case DW_FORM_data2:
      type = die_type (die, cu);
      result = dwarf2_const_value_data (attr, obstack, cu, &value, 16);
      if (result == NULL)
	result = write_constant_as_bytes (obstack, byte_order,
					  type, value, len);
      break;
    case DW_FORM_data4:
      type = die_type (die, cu);
      result = dwarf2_const_value_data (attr, obstack, cu, &value, 32);
      if (result == NULL)
	result = write_constant_as_bytes (obstack, byte_order,
					  type, value, len);
      break;
    case DW_FORM_data8:
      type = die_type (die, cu);
      result = dwarf2_const_value_data (attr, obstack, cu, &value, 64);
      if (result == NULL)
	result = write_constant_as_bytes (obstack, byte_order,
					  type, value, len);
      break;

    case DW_FORM_sdata:
    case DW_FORM_implicit_const:
      type = die_type (die, cu);
      result = write_constant_as_bytes (obstack, byte_order,
					type, attr->as_signed (), len);
      break;

    case DW_FORM_udata:
      type = die_type (die, cu);
      result = write_constant_as_bytes (obstack, byte_order,
					type, attr->as_unsigned (), len);
      break;

    default:
      complaint (_("unsupported const value attribute form: '%s'"),
		 dwarf_form_name (attr->form));
      break;
    }

  return result;
}

/* See read.h.  */

struct type *
dwarf2_fetch_die_type_sect_off (sect_offset sect_off,
				dwarf2_per_cu_data *per_cu,
				dwarf2_per_objfile *per_objfile,
				const char **var_name)
{
  struct die_info *die;

  dwarf2_cu *cu = per_objfile->get_cu (per_cu);
  if (cu == nullptr)
    cu = load_cu (per_cu, per_objfile, false);

  if (cu == nullptr)
    return nullptr;

  die = follow_die_offset (sect_off, per_cu->is_dwz, &cu);
  if (!die)
    return NULL;

  if (var_name != nullptr)
    *var_name = var_decl_name (die, cu);
  return die_type (die, cu);
}

/* See read.h.  */

struct type *
dwarf2_get_die_type (cu_offset die_offset,
		     dwarf2_per_cu_data *per_cu,
		     dwarf2_per_objfile *per_objfile)
{
  sect_offset die_offset_sect = per_cu->sect_off + to_underlying (die_offset);
  return get_die_type_at_offset (die_offset_sect, per_cu, per_objfile);
}

/* Follow type unit SIG_TYPE referenced by SRC_DIE.
   On entry *REF_CU is the CU of SRC_DIE.
   On exit *REF_CU is the CU of the result.
   Returns NULL if the referenced DIE isn't found.  */

static struct die_info *
follow_die_sig_1 (struct die_info *src_die, struct signatured_type *sig_type,
		  struct dwarf2_cu **ref_cu)
{
  struct die_info temp_die;
  struct dwarf2_cu *sig_cu;
  struct die_info *die;
  dwarf2_per_objfile *per_objfile = (*ref_cu)->per_objfile;


  /* While it might be nice to assert sig_type->type == NULL here,
     we can get here for DW_AT_imported_declaration where we need
     the DIE not the type.  */

  /* If necessary, add it to the queue and load its DIEs.

     Even if maybe_queue_comp_unit doesn't require us to load the CU's DIEs,
     it doesn't mean they are currently loaded.  Since we require them
     to be loaded, we must check for ourselves.  */
  if (maybe_queue_comp_unit (*ref_cu, sig_type, per_objfile,
			     language_minimal)
      || per_objfile->get_cu (sig_type) == nullptr)
    read_signatured_type (sig_type, per_objfile);

  sig_cu = per_objfile->get_cu (sig_type);
  gdb_assert (sig_cu != NULL);
  gdb_assert (to_underlying (sig_type->type_offset_in_section) != 0);
  temp_die.sect_off = sig_type->type_offset_in_section;
  die = (struct die_info *) htab_find_with_hash (sig_cu->die_hash, &temp_die,
						 to_underlying (temp_die.sect_off));
  if (die)
    {
      /* For .gdb_index version 7 keep track of included TUs.
	 http://sourceware.org/bugzilla/show_bug.cgi?id=15021.  */
      if (per_objfile->per_bfd->index_table != NULL
	  && !per_objfile->per_bfd->index_table->version_check ())
	{
	  (*ref_cu)->per_cu->imported_symtabs_push (sig_cu->per_cu);
	}

      *ref_cu = sig_cu;
      return die;
    }

  return NULL;
}

/* Follow signatured type referenced by ATTR in SRC_DIE.
   On entry *REF_CU is the CU of SRC_DIE.
   On exit *REF_CU is the CU of the result.
   The result is the DIE of the type.
   If the referenced type cannot be found an error is thrown.  */

static struct die_info *
follow_die_sig (struct die_info *src_die, const struct attribute *attr,
		struct dwarf2_cu **ref_cu)
{
  ULONGEST signature = attr->as_signature ();
  struct signatured_type *sig_type;
  struct die_info *die;

  gdb_assert (attr->form == DW_FORM_ref_sig8);

  sig_type = lookup_signatured_type (*ref_cu, signature);
  /* sig_type will be NULL if the signatured type is missing from
     the debug info.  */
  if (sig_type == NULL)
    {
      error (_("Dwarf Error: Cannot find signatured DIE %s referenced"
	       " from DIE at %s [in module %s]"),
	     hex_string (signature), sect_offset_str (src_die->sect_off),
	     objfile_name ((*ref_cu)->per_objfile->objfile));
    }

  die = follow_die_sig_1 (src_die, sig_type, ref_cu);
  if (die == NULL)
    {
      src_die->error_dump ();
      error (_("Dwarf Error: Problem reading signatured DIE %s referenced"
	       " from DIE at %s [in module %s]"),
	     hex_string (signature), sect_offset_str (src_die->sect_off),
	     objfile_name ((*ref_cu)->per_objfile->objfile));
    }

  return die;
}

/* Get the type specified by SIGNATURE referenced in DIE/CU,
   reading in and processing the type unit if necessary.  */

static struct type *
get_signatured_type (struct die_info *die, ULONGEST signature,
		     struct dwarf2_cu *cu)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct signatured_type *sig_type;
  struct dwarf2_cu *type_cu;
  struct die_info *type_die;
  struct type *type;

  sig_type = lookup_signatured_type (cu, signature);
  /* sig_type will be NULL if the signatured type is missing from
     the debug info.  */
  if (sig_type == NULL)
    {
      complaint (_("Dwarf Error: Cannot find signatured DIE %s referenced"
		   " from DIE at %s [in module %s]"),
		 hex_string (signature), sect_offset_str (die->sect_off),
		 objfile_name (per_objfile->objfile));
      return build_error_marker_type (cu, die);
    }

  /* If we already know the type we're done.  */
  type = per_objfile->get_type_for_signatured_type (sig_type);
  if (type != nullptr)
    return type;

  type_cu = cu;
  type_die = follow_die_sig_1 (die, sig_type, &type_cu);
  if (type_die != NULL)
    {
      /* N.B. We need to call get_die_type to ensure only one type for this DIE
	 is created.  This is important, for example, because for c++ classes
	 we need TYPE_NAME set which is only done by new_symbol.  Blech.  */
      type = read_type_die (type_die, type_cu);
      if (type == NULL)
	{
	  complaint (_("Dwarf Error: Cannot build signatured type %s"
		       " referenced from DIE at %s [in module %s]"),
		     hex_string (signature), sect_offset_str (die->sect_off),
		     objfile_name (per_objfile->objfile));
	  type = build_error_marker_type (cu, die);
	}
    }
  else
    {
      complaint (_("Dwarf Error: Problem reading signatured DIE %s referenced"
		   " from DIE at %s [in module %s]"),
		 hex_string (signature), sect_offset_str (die->sect_off),
		 objfile_name (per_objfile->objfile));
      type = build_error_marker_type (cu, die);
    }

  per_objfile->set_type_for_signatured_type (sig_type, type);

  return type;
}

/* Get the type specified by the DW_AT_signature ATTR in DIE/CU,
   reading in and processing the type unit if necessary.  */

static struct type *
get_DW_AT_signature_type (struct die_info *die, const struct attribute *attr,
			  struct dwarf2_cu *cu) /* ARI: editCase function */
{
  /* Yes, DW_AT_signature can use a non-ref_sig8 reference.  */
  if (attr->form_is_ref ())
    {
      struct dwarf2_cu *type_cu = cu;
      struct die_info *type_die = follow_die_ref (die, attr, &type_cu);

      return read_type_die (type_die, type_cu);
    }
  else if (attr->form == DW_FORM_ref_sig8)
    {
      return get_signatured_type (die, attr->as_signature (), cu);
    }
  else
    {
      dwarf2_per_objfile *per_objfile = cu->per_objfile;

      complaint (_("Dwarf Error: DW_AT_signature has bad form %s in DIE"
		   " at %s [in module %s]"),
		 dwarf_form_name (attr->form), sect_offset_str (die->sect_off),
		 objfile_name (per_objfile->objfile));
      return build_error_marker_type (cu, die);
    }
}

/* Load the DIEs associated with type unit PER_CU into memory.  */

static void
load_full_type_unit (dwarf2_per_cu_data *per_cu,
		     dwarf2_per_objfile *per_objfile)
{
  struct signatured_type *sig_type;

  /* We have the per_cu, but we need the signatured_type.
     Fortunately this is an easy translation.  */
  gdb_assert (per_cu->is_debug_types);
  sig_type = (struct signatured_type *) per_cu;

  gdb_assert (per_objfile->get_cu (per_cu) == nullptr);

  read_signatured_type (sig_type, per_objfile);

  gdb_assert (per_objfile->get_cu (per_cu) != nullptr);
}

/* Read in a signatured type and build its CU and DIEs.
   If the type is a stub for the real type in a DWO file,
   read in the real type from the DWO file as well.  */

static void
read_signatured_type (signatured_type *sig_type,
		      dwarf2_per_objfile *per_objfile)
{
  gdb_assert (sig_type->is_debug_types);
  gdb_assert (per_objfile->get_cu (sig_type) == nullptr);

  cutu_reader reader (sig_type, per_objfile, nullptr, nullptr, false);

  if (!reader.dummy_p)
    {
      struct dwarf2_cu *cu = reader.cu;
      const gdb_byte *info_ptr = reader.info_ptr;

      gdb_assert (cu->die_hash == NULL);
      cu->die_hash =
	htab_create_alloc_ex (cu->header.get_length_without_initial () / 12,
			      die_info::hash,
			      die_info::eq,
			      NULL,
			      &cu->comp_unit_obstack,
			      hashtab_obstack_allocate,
			      dummy_obstack_deallocate);

      if (reader.comp_unit_die->has_children)
	reader.comp_unit_die->child
	  = read_die_and_siblings (&reader, info_ptr, &info_ptr,
				   reader.comp_unit_die);
      cu->dies = reader.comp_unit_die;
      /* comp_unit_die is not stored in die_hash, no need.  */

      /* We try not to read any attributes in this function, because
	 not all CUs needed for references have been loaded yet, and
	 symbol table processing isn't initialized.  But we have to
	 set the CU language, or we won't be able to build types
	 correctly.  Similarly, if we do not read the producer, we can
	 not apply producer-specific interpretation.  */
      prepare_one_comp_unit (cu, cu->dies, language_minimal);

      reader.keep ();
    }

  sig_type->tu_read = 1;
}

/* Decode simple location descriptions.

   Given a pointer to a DWARF block that defines a location, compute
   the location.  Returns true if the expression was computable by
   this function, false otherwise.  On a true return, *RESULT is set.

   Note that this function does not implement a full DWARF expression
   evaluator.  Instead, it is used for a few limited purposes:

   - Getting the address of a symbol that has a constant address.  For
   example, if a symbol has a location like "DW_OP_addr", the address
   can be extracted.

   - Getting the offset of a virtual function in its vtable.  There
   are two forms of this, one of which involves DW_OP_deref -- so this
   function handles derefs in a peculiar way to make this 'work'.
   (Probably this area should be rewritten.)

   - Getting the offset of a field, when it is constant.

   Opcodes that cannot be part of a constant expression, for example
   those involving registers, simply result in a return of
   'false'.

   This function may emit a complaint.  */

static bool
decode_locdesc (struct dwarf_block *blk, struct dwarf2_cu *cu,
		CORE_ADDR *result)
{
  struct objfile *objfile = cu->per_objfile->objfile;
  size_t i;
  size_t size = blk->size;
  const gdb_byte *data = blk->data;
  CORE_ADDR stack[64];
  int stacki;
  unsigned int bytes_read;
  gdb_byte op;

  *result = 0;
  i = 0;
  stacki = 0;
  stack[stacki] = 0;
  stack[++stacki] = 0;

  while (i < size)
    {
      op = data[i++];
      switch (op)
	{
	case DW_OP_lit0:
	case DW_OP_lit1:
	case DW_OP_lit2:
	case DW_OP_lit3:
	case DW_OP_lit4:
	case DW_OP_lit5:
	case DW_OP_lit6:
	case DW_OP_lit7:
	case DW_OP_lit8:
	case DW_OP_lit9:
	case DW_OP_lit10:
	case DW_OP_lit11:
	case DW_OP_lit12:
	case DW_OP_lit13:
	case DW_OP_lit14:
	case DW_OP_lit15:
	case DW_OP_lit16:
	case DW_OP_lit17:
	case DW_OP_lit18:
	case DW_OP_lit19:
	case DW_OP_lit20:
	case DW_OP_lit21:
	case DW_OP_lit22:
	case DW_OP_lit23:
	case DW_OP_lit24:
	case DW_OP_lit25:
	case DW_OP_lit26:
	case DW_OP_lit27:
	case DW_OP_lit28:
	case DW_OP_lit29:
	case DW_OP_lit30:
	case DW_OP_lit31:
	  stack[++stacki] = op - DW_OP_lit0;
	  break;

	case DW_OP_addr:
	  stack[++stacki]
	    = (CORE_ADDR) cu->header.read_address (objfile->obfd.get (),
						   &data[i],
						   &bytes_read);
	  i += bytes_read;
	  break;

	case DW_OP_const1u:
	  stack[++stacki] = read_1_byte (objfile->obfd.get (), &data[i]);
	  i += 1;
	  break;

	case DW_OP_const1s:
	  stack[++stacki] = read_1_signed_byte (objfile->obfd.get (), &data[i]);
	  i += 1;
	  break;

	case DW_OP_const2u:
	  stack[++stacki] = read_2_bytes (objfile->obfd.get (), &data[i]);
	  i += 2;
	  break;

	case DW_OP_const2s:
	  stack[++stacki] = read_2_signed_bytes (objfile->obfd.get (), &data[i]);
	  i += 2;
	  break;

	case DW_OP_const4u:
	  stack[++stacki] = read_4_bytes (objfile->obfd.get (), &data[i]);
	  i += 4;
	  break;

	case DW_OP_const4s:
	  stack[++stacki] = read_4_signed_bytes (objfile->obfd.get (), &data[i]);
	  i += 4;
	  break;

	case DW_OP_const8u:
	  stack[++stacki] = read_8_bytes (objfile->obfd.get (), &data[i]);
	  i += 8;
	  break;

	case DW_OP_constu:
	  stack[++stacki] = read_unsigned_leb128 (NULL, (data + i),
						  &bytes_read);
	  i += bytes_read;
	  break;

	case DW_OP_consts:
	  stack[++stacki] = read_signed_leb128 (NULL, (data + i), &bytes_read);
	  i += bytes_read;
	  break;

	case DW_OP_dup:
	  stack[stacki + 1] = stack[stacki];
	  stacki++;
	  break;

	case DW_OP_plus:
	  stack[stacki - 1] += stack[stacki];
	  stacki--;
	  break;

	case DW_OP_plus_uconst:
	  stack[stacki] += read_unsigned_leb128 (NULL, (data + i),
						 &bytes_read);
	  i += bytes_read;
	  break;

	case DW_OP_minus:
	  stack[stacki - 1] -= stack[stacki];
	  stacki--;
	  break;

	case DW_OP_deref:
	  /* If we're not the last op, then we definitely can't encode
	     this using GDB's address_class enum.  This is valid for partial
	     global symbols, although the variable's address will be bogus
	     in the psymtab.  */
	  if (i < size)
	    return false;
	  break;

	case DW_OP_addrx:
	case DW_OP_GNU_addr_index:
	case DW_OP_GNU_const_index:
	  stack[++stacki]
	    = (CORE_ADDR) read_addr_index_from_leb128 (cu, &data[i],
						       &bytes_read);
	  i += bytes_read;
	  break;

	default:
	  return false;
	}

      /* Enforce maximum stack depth of SIZE-1 to avoid writing
	 outside of the allocated space.  Also enforce minimum>0.  */
      if (stacki >= ARRAY_SIZE (stack) - 1)
	{
	  complaint (_("location description stack overflow"));
	  return false;
	}

      if (stacki <= 0)
	{
	  complaint (_("location description stack underflow"));
	  return false;
	}
    }

  *result = stack[stacki];
  return true;
}

/* memory allocation interface */

static struct dwarf_block *
dwarf_alloc_block (struct dwarf2_cu *cu)
{
  return XOBNEW (&cu->comp_unit_obstack, struct dwarf_block);
}



/* Macro support.  */

/* An overload of dwarf_decode_macros that finds the correct section
   and ensures it is read in before calling the other overload.  */

static void
dwarf_decode_macros (struct dwarf2_cu *cu, unsigned int offset,
		     int section_is_gnu)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  const struct line_header *lh = cu->line_header;
  unsigned int offset_size = cu->header.offset_size;
  struct dwarf2_section_info *section;
  const char *section_name;

  if (cu->dwo_unit != nullptr)
    {
      if (section_is_gnu)
	{
	  section = &cu->dwo_unit->dwo_file->sections.macro;
	  section_name = ".debug_macro.dwo";
	}
      else
	{
	  section = &cu->dwo_unit->dwo_file->sections.macinfo;
	  section_name = ".debug_macinfo.dwo";
	}
    }
  else
    {
      if (section_is_gnu)
	{
	  section = &per_objfile->per_bfd->macro;
	  section_name = ".debug_macro";
	}
      else
	{
	  section = &per_objfile->per_bfd->macinfo;
	  section_name = ".debug_macinfo";
	}
    }

  section->read (objfile);
  if (section->buffer == nullptr)
    {
      complaint (_("missing %s section"), section_name);
      return;
    }

  buildsym_compunit *builder = cu->get_builder ();

  struct dwarf2_section_info *str_offsets_section;
  struct dwarf2_section_info *str_section;
  std::optional<ULONGEST> str_offsets_base;

  if (cu->dwo_unit != nullptr)
    {
      str_offsets_section = &cu->dwo_unit->dwo_file
			       ->sections.str_offsets;
      str_section = &cu->dwo_unit->dwo_file->sections.str;
      str_offsets_base = cu->header.addr_size;
    }
  else
    {
      str_offsets_section = &per_objfile->per_bfd->str_offsets;
      str_section = &per_objfile->per_bfd->str;
      str_offsets_base = cu->str_offsets_base;
    }

  dwarf_decode_macros (per_objfile, builder, section, lh,
		       offset_size, offset, str_section, str_offsets_section,
		       str_offsets_base, section_is_gnu, cu);
}

/* Return the .debug_loc section to use for CU.
   For DWO files use .debug_loc.dwo.  */

static struct dwarf2_section_info *
cu_debug_loc_section (struct dwarf2_cu *cu)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;

  if (cu->dwo_unit)
    {
      struct dwo_sections *sections = &cu->dwo_unit->dwo_file->sections;

      return cu->header.version >= 5 ? &sections->loclists : &sections->loc;
    }
  return (cu->header.version >= 5 ? &per_objfile->per_bfd->loclists
				  : &per_objfile->per_bfd->loc);
}

/* Return the .debug_rnglists section to use for CU.  */
static struct dwarf2_section_info *
cu_debug_rnglists_section (struct dwarf2_cu *cu, dwarf_tag tag)
{
  if (cu->header.version < 5)
    error (_(".debug_rnglists section cannot be used in DWARF %d"),
	   cu->header.version);
  struct dwarf2_per_objfile *dwarf2_per_objfile = cu->per_objfile;

  /* Make sure we read the .debug_rnglists section from the file that
     contains the DW_AT_ranges attribute we are reading.  Normally that
     would be the .dwo file, if there is one.  However for DW_TAG_compile_unit
     or DW_TAG_skeleton unit, we always want to read from objfile/linked
     program.  */
  if (cu->dwo_unit != nullptr
      && tag != DW_TAG_compile_unit
      && tag != DW_TAG_skeleton_unit)
    {
      struct dwo_sections *sections = &cu->dwo_unit->dwo_file->sections;

      if (sections->rnglists.size > 0)
	return &sections->rnglists;
      else
	error (_(".debug_rnglists section is missing from .dwo file."));
    }
  return &dwarf2_per_objfile->per_bfd->rnglists;
}

/* A helper function that fills in a dwarf2_loclist_baton.  */

static void
fill_in_loclist_baton (struct dwarf2_cu *cu,
		       struct dwarf2_loclist_baton *baton,
		       const struct attribute *attr)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct dwarf2_section_info *section = cu_debug_loc_section (cu);

  section->read (per_objfile->objfile);

  baton->per_objfile = per_objfile;
  baton->per_cu = cu->per_cu;
  gdb_assert (baton->per_cu);
  /* We don't know how long the location list is, but make sure we
     don't run off the edge of the section.  */
  baton->size = section->size - attr->as_unsigned ();
  baton->data = section->buffer + attr->as_unsigned ();
  if (cu->base_address.has_value ())
    baton->base_address = *cu->base_address;
  else
    baton->base_address = {};
  baton->from_dwo = cu->dwo_unit != NULL;
}

static void
dwarf2_symbol_mark_computed (const struct attribute *attr, struct symbol *sym,
			     struct dwarf2_cu *cu, int is_block)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct objfile *objfile = per_objfile->objfile;
  struct dwarf2_section_info *section = cu_debug_loc_section (cu);

  if (attr->form_is_section_offset ()
      /* .debug_loc{,.dwo} may not exist at all, or the offset may be outside
	 the section.  If so, fall through to the complaint in the
	 other branch.  */
      && attr->as_unsigned () < section->get_size (objfile))
    {
      struct dwarf2_loclist_baton *baton;

      baton = XOBNEW (&objfile->objfile_obstack, struct dwarf2_loclist_baton);

      fill_in_loclist_baton (cu, baton, attr);

      if (!cu->base_address.has_value ())
	complaint (_("Location list used without "
		     "specifying the CU base address."));

      sym->set_aclass_index ((is_block
			      ? dwarf2_loclist_block_index
			      : dwarf2_loclist_index));
      SYMBOL_LOCATION_BATON (sym) = baton;
    }
  else
    {
      struct dwarf2_locexpr_baton *baton;

      baton = XOBNEW (&objfile->objfile_obstack, struct dwarf2_locexpr_baton);
      baton->per_objfile = per_objfile;
      baton->per_cu = cu->per_cu;
      gdb_assert (baton->per_cu);

      if (attr->form_is_block ())
	{
	  /* Note that we're just copying the block's data pointer
	     here, not the actual data.  We're still pointing into the
	     info_buffer for SYM's objfile; right now we never release
	     that buffer, but when we do clean up properly this may
	     need to change.  */
	  struct dwarf_block *block = attr->as_block ();
	  baton->size = block->size;
	  baton->data = block->data;
	}
      else
	{
	  dwarf2_invalid_attrib_class_complaint ("location description",
						 sym->natural_name ());
	  baton->size = 0;
	}

      sym->set_aclass_index ((is_block
			      ? dwarf2_locexpr_block_index
			      : dwarf2_locexpr_index));
      SYMBOL_LOCATION_BATON (sym) = baton;
    }
}

/* See read.h.  */

const comp_unit_head *
dwarf2_per_cu_data::get_header () const
{
  if (!m_header_read_in)
    {
      const gdb_byte *info_ptr
	= this->section->buffer + to_underlying (this->sect_off);

      read_comp_unit_head (&m_header, info_ptr, this->section,
			   rcuh_kind::COMPILE);

      m_header_read_in = true;
    }

  return &m_header;
}

/* See read.h.  */

int
dwarf2_per_cu_data::addr_size () const
{
  return this->get_header ()->addr_size;
}

/* See read.h.  */

int
dwarf2_per_cu_data::offset_size () const
{
  return this->get_header ()->offset_size;
}

/* See read.h.  */

int
dwarf2_per_cu_data::ref_addr_size () const
{
  const comp_unit_head *header = this->get_header ();

  if (header->version == 2)
    return header->addr_size;
  else
    return header->offset_size;
}

/* See read.h.  */

void
dwarf2_per_cu_data::set_lang (enum language lang,
			      dwarf_source_language dw_lang)
{
  if (unit_type () == DW_UT_partial)
    return;

  /* Set if not set already.  */
  packed<language, LANGUAGE_BYTES> new_value = lang;
  packed<language, LANGUAGE_BYTES> old_value = m_lang.exchange (new_value);
  /* If already set, verify that it's the same value.  */
  gdb_assert (old_value == language_unknown || old_value == lang);

  packed<dwarf_source_language, 2> new_dw = dw_lang;
  packed<dwarf_source_language, 2> old_dw = m_dw_lang.exchange (new_dw);
  gdb_assert (old_dw == 0 || old_dw == dw_lang);
}

/* A helper function for dwarf2_find_containing_comp_unit that returns
   the index of the result, and that searches a vector.  It will
   return a result even if the offset in question does not actually
   occur in any CU.  This is separate so that it can be unit
   tested.  */

static int
dwarf2_find_containing_comp_unit
  (sect_offset sect_off,
   unsigned int offset_in_dwz,
   const std::vector<dwarf2_per_cu_data_up> &all_units)
{
  int low, high;

  low = 0;
  high = all_units.size () - 1;
  while (high > low)
    {
      struct dwarf2_per_cu_data *mid_cu;
      int mid = low + (high - low) / 2;

      mid_cu = all_units[mid].get ();
      if (mid_cu->is_dwz > offset_in_dwz
	  || (mid_cu->is_dwz == offset_in_dwz
	      && mid_cu->sect_off + mid_cu->length () > sect_off))
	high = mid;
      else
	low = mid + 1;
    }
  gdb_assert (low == high);
  return low;
}

/* Locate the .debug_info compilation unit from CU's objfile which contains
   the DIE at OFFSET.  Raises an error on failure.  */

static struct dwarf2_per_cu_data *
dwarf2_find_containing_comp_unit (sect_offset sect_off,
				  unsigned int offset_in_dwz,
				  dwarf2_per_bfd *per_bfd)
{
  int low = dwarf2_find_containing_comp_unit
    (sect_off, offset_in_dwz, per_bfd->all_units);
  dwarf2_per_cu_data *this_cu = per_bfd->all_units[low].get ();

  if (this_cu->is_dwz != offset_in_dwz || this_cu->sect_off > sect_off)
    {
      if (low == 0 || this_cu->is_dwz != offset_in_dwz)
	error (_("Dwarf Error: could not find partial DIE containing "
	       "offset %s [in module %s]"),
	       sect_offset_str (sect_off),
	       bfd_get_filename (per_bfd->obfd));

      gdb_assert (per_bfd->all_units[low-1]->sect_off
		  <= sect_off);
      return per_bfd->all_units[low - 1].get ();
    }
  else
    {
      if (low == per_bfd->all_units.size () - 1
	  && sect_off >= this_cu->sect_off + this_cu->length ())
	error (_("invalid dwarf2 offset %s"), sect_offset_str (sect_off));
      gdb_assert (sect_off < this_cu->sect_off + this_cu->length ());
      return this_cu;
    }
}

#if GDB_SELF_TEST

namespace selftests {
namespace find_containing_comp_unit {

static void
run_test ()
{
  dwarf2_per_cu_data_up one (new dwarf2_per_cu_data);
  dwarf2_per_cu_data *one_ptr = one.get ();
  dwarf2_per_cu_data_up two (new dwarf2_per_cu_data);
  dwarf2_per_cu_data *two_ptr = two.get ();
  dwarf2_per_cu_data_up three (new dwarf2_per_cu_data);
  dwarf2_per_cu_data *three_ptr = three.get ();
  dwarf2_per_cu_data_up four (new dwarf2_per_cu_data);
  dwarf2_per_cu_data *four_ptr = four.get ();

  one->set_length (5);
  two->sect_off = sect_offset (one->length ());
  two->set_length (7);

  three->set_length (5);
  three->is_dwz = 1;
  four->sect_off = sect_offset (three->length ());
  four->set_length (7);
  four->is_dwz = 1;

  std::vector<dwarf2_per_cu_data_up> units;
  units.push_back (std::move (one));
  units.push_back (std::move (two));
  units.push_back (std::move (three));
  units.push_back (std::move (four));

  int result;

  result = dwarf2_find_containing_comp_unit (sect_offset (0), 0, units);
  SELF_CHECK (units[result].get () == one_ptr);
  result = dwarf2_find_containing_comp_unit (sect_offset (3), 0, units);
  SELF_CHECK (units[result].get () == one_ptr);
  result = dwarf2_find_containing_comp_unit (sect_offset (5), 0, units);
  SELF_CHECK (units[result].get () == two_ptr);

  result = dwarf2_find_containing_comp_unit (sect_offset (0), 1, units);
  SELF_CHECK (units[result].get () == three_ptr);
  result = dwarf2_find_containing_comp_unit (sect_offset (3), 1, units);
  SELF_CHECK (units[result].get () == three_ptr);
  result = dwarf2_find_containing_comp_unit (sect_offset (5), 1, units);
  SELF_CHECK (units[result].get () == four_ptr);
}

}
}

#endif /* GDB_SELF_TEST */

/* Initialize basic fields of dwarf_cu CU according to DIE COMP_UNIT_DIE.  */

static void
prepare_one_comp_unit (struct dwarf2_cu *cu, struct die_info *comp_unit_die,
		       enum language pretend_language)
{
  struct attribute *attr;

  cu->producer = dwarf2_string_attr (comp_unit_die, DW_AT_producer, cu);

  /* Set the language we're debugging.  */
  attr = dwarf2_attr (comp_unit_die, DW_AT_language, cu);
  enum language lang;
  dwarf_source_language dw_lang = (dwarf_source_language) 0;
  if (cu->producer != nullptr
      && strstr (cu->producer, "IBM XL C for OpenCL") != NULL)
    {
      /* The XLCL doesn't generate DW_LANG_OpenCL because this
	 attribute is not standardised yet.  As a workaround for the
	 language detection we fall back to the DW_AT_producer
	 string.  */
      lang = language_opencl;
      dw_lang = DW_LANG_OpenCL;
    }
  else if (cu->producer != nullptr
	   && strstr (cu->producer, "GNU Go ") != NULL)
    {
      /* Similar hack for Go.  */
      lang = language_go;
      dw_lang = DW_LANG_Go;
    }
  else if (attr != nullptr)
    {
      lang = dwarf_lang_to_enum_language (attr->constant_value (0));
      dw_lang = (dwarf_source_language) attr->constant_value (0);
    }
  else
    lang = pretend_language;

  cu->language_defn = language_def (lang);

  switch (comp_unit_die->tag)
    {
    case DW_TAG_compile_unit:
      cu->per_cu->set_unit_type (DW_UT_compile);
      break;
    case DW_TAG_partial_unit:
      cu->per_cu->set_unit_type (DW_UT_partial);
      break;
    case DW_TAG_type_unit:
      cu->per_cu->set_unit_type (DW_UT_type);
      break;
    default:
      error (_("Dwarf Error: unexpected tag '%s' at offset %s"),
	     dwarf_tag_name (comp_unit_die->tag),
	     sect_offset_str (cu->per_cu->sect_off));
    }

  cu->per_cu->set_lang (lang, dw_lang);
}

/* See read.h.  */

dwarf2_cu *
dwarf2_per_objfile::get_cu (dwarf2_per_cu_data *per_cu)
{
  auto it = m_dwarf2_cus.find (per_cu);
  if (it == m_dwarf2_cus.end ())
    return nullptr;

  return it->second.get ();
}

/* See read.h.  */

void
dwarf2_per_objfile::set_cu (dwarf2_per_cu_data *per_cu,
			    std::unique_ptr<dwarf2_cu> cu)
{
  gdb_assert (this->get_cu (per_cu) == nullptr);

  m_dwarf2_cus[per_cu] = std::move (cu);
}

/* See read.h.  */

void
dwarf2_per_objfile::age_comp_units ()
{
  dwarf_read_debug_printf_v ("running");

  /* This is not expected to be called in the middle of CU expansion.  There is
     an invariant that if a CU is in the CUs-to-expand queue, its DIEs are
     loaded in memory.  Calling age_comp_units while the queue is in use could
     make us free the DIEs for a CU that is in the queue and therefore break
     that invariant.  */
  gdb_assert (!queue.has_value ());

  /* Start by clearing all marks.  */
  for (const auto &pair : m_dwarf2_cus)
    pair.second->clear_mark ();

  /* Traverse all CUs, mark them and their dependencies if used recently
     enough.  */
  for (const auto &pair : m_dwarf2_cus)
    {
      dwarf2_cu *cu = pair.second.get ();

      cu->last_used++;
      if (cu->last_used <= dwarf_max_cache_age)
	cu->mark ();
    }

  /* Delete all CUs still not marked.  */
  for (auto it = m_dwarf2_cus.begin (); it != m_dwarf2_cus.end ();)
    {
      dwarf2_cu *cu = it->second.get ();

      if (!cu->is_marked ())
	{
	  dwarf_read_debug_printf_v ("deleting old CU %s",
				     sect_offset_str (cu->per_cu->sect_off));
	  it = m_dwarf2_cus.erase (it);
	}
      else
	it++;
    }
}

/* See read.h.  */

void
dwarf2_per_objfile::remove_cu (dwarf2_per_cu_data *per_cu)
{
  auto it = m_dwarf2_cus.find (per_cu);
  if (it == m_dwarf2_cus.end ())
    return;

  m_dwarf2_cus.erase (it);
}

dwarf2_per_objfile::~dwarf2_per_objfile ()
{
  remove_all_cus ();
}

/* A set of CU "per_cu" pointer, DIE offset, and GDB type pointer.
   We store these in a hash table separate from the DIEs, and preserve them
   when the DIEs are flushed out of cache.

   The CU "per_cu" pointer is needed because offset alone is not enough to
   uniquely identify the type.  A file may have multiple .debug_types sections,
   or the type may come from a DWO file.  Furthermore, while it's more logical
   to use per_cu->section+offset, with Fission the section with the data is in
   the DWO file but we don't know that section at the point we need it.
   We have to use something in dwarf2_per_cu_data (or the pointer to it)
   because we can enter the lookup routine, get_die_type_at_offset, from
   outside this file, and thus won't necessarily have PER_CU->cu.
   Fortunately, PER_CU is stable for the life of the objfile.  */

struct dwarf2_per_cu_offset_and_type
{
  const struct dwarf2_per_cu_data *per_cu;
  sect_offset sect_off;
  struct type *type;
};

/* Hash function for a dwarf2_per_cu_offset_and_type.  */

static hashval_t
per_cu_offset_and_type_hash (const void *item)
{
  const struct dwarf2_per_cu_offset_and_type *ofs
    = (const struct dwarf2_per_cu_offset_and_type *) item;

  return (uintptr_t) ofs->per_cu + to_underlying (ofs->sect_off);
}

/* Equality function for a dwarf2_per_cu_offset_and_type.  */

static int
per_cu_offset_and_type_eq (const void *item_lhs, const void *item_rhs)
{
  const struct dwarf2_per_cu_offset_and_type *ofs_lhs
    = (const struct dwarf2_per_cu_offset_and_type *) item_lhs;
  const struct dwarf2_per_cu_offset_and_type *ofs_rhs
    = (const struct dwarf2_per_cu_offset_and_type *) item_rhs;

  return (ofs_lhs->per_cu == ofs_rhs->per_cu
	  && ofs_lhs->sect_off == ofs_rhs->sect_off);
}

/* Set the type associated with DIE to TYPE.  Save it in CU's hash
   table if necessary.  For convenience, return TYPE.

   The DIEs reading must have careful ordering to:
    * Not cause infinite loops trying to read in DIEs as a prerequisite for
      reading current DIE.
    * Not trying to dereference contents of still incompletely read in types
      while reading in other DIEs.
    * Enable referencing still incompletely read in types just by a pointer to
      the type without accessing its fields.

   Therefore caller should follow these rules:
     * Try to fetch any prerequisite types we may need to build this DIE type
       before building the type and calling set_die_type.
     * After building type call set_die_type for current DIE as soon as
       possible before fetching more types to complete the current type.
     * Make the type as complete as possible before fetching more types.  */

static struct type *
set_die_type (struct die_info *die, struct type *type, struct dwarf2_cu *cu,
	      bool skip_data_location)
{
  dwarf2_per_objfile *per_objfile = cu->per_objfile;
  struct dwarf2_per_cu_offset_and_type **slot, ofs;
  struct objfile *objfile = per_objfile->objfile;
  struct attribute *attr;
  struct dynamic_prop prop;

  /* For Ada types, make sure that the gnat-specific data is always
     initialized (if not already set).  There are a few types where
     we should not be doing so, because the type-specific area is
     already used to hold some other piece of info (eg: TYPE_CODE_FLT
     where the type-specific area is used to store the floatformat).
     But this is not a problem, because the gnat-specific information
     is actually not needed for these types.  */
  if (need_gnat_info (cu)
      && type->code () != TYPE_CODE_FUNC
      && type->code () != TYPE_CODE_FLT
      && type->code () != TYPE_CODE_METHODPTR
      && type->code () != TYPE_CODE_MEMBERPTR
      && type->code () != TYPE_CODE_METHOD
      && type->code () != TYPE_CODE_FIXED_POINT
      && !HAVE_GNAT_AUX_INFO (type))
    INIT_GNAT_SPECIFIC (type);

  /* Read DW_AT_allocated and set in type.  */
  attr = dwarf2_attr (die, DW_AT_allocated, cu);
  if (attr != NULL)
    {
      struct type *prop_type = cu->addr_sized_int_type (false);
      if (attr_to_dynamic_prop (attr, die, cu, &prop, prop_type))
	type->add_dyn_prop (DYN_PROP_ALLOCATED, prop);
    }

  /* Read DW_AT_associated and set in type.  */
  attr = dwarf2_attr (die, DW_AT_associated, cu);
  if (attr != NULL)
    {
      struct type *prop_type = cu->addr_sized_int_type (false);
      if (attr_to_dynamic_prop (attr, die, cu, &prop, prop_type))
	type->add_dyn_prop (DYN_PROP_ASSOCIATED, prop);
    }

  /* Read DW_AT_rank and set in type.  */
  attr = dwarf2_attr (die, DW_AT_rank, cu);
  if (attr != NULL)
    {
      struct type *prop_type = cu->addr_sized_int_type (false);
      if (attr_to_dynamic_prop (attr, die, cu, &prop, prop_type))
	type->add_dyn_prop (DYN_PROP_RANK, prop);
    }

  /* Read DW_AT_data_location and set in type.  */
  if (!skip_data_location)
    {
      attr = dwarf2_attr (die, DW_AT_data_location, cu);
      if (attr_to_dynamic_prop (attr, die, cu, &prop, cu->addr_type ()))
	type->add_dyn_prop (DYN_PROP_DATA_LOCATION, prop);
    }

  if (per_objfile->die_type_hash == NULL)
    per_objfile->die_type_hash
      = htab_up (htab_create_alloc (127,
				    per_cu_offset_and_type_hash,
				    per_cu_offset_and_type_eq,
				    NULL, xcalloc, xfree));

  ofs.per_cu = cu->per_cu;
  ofs.sect_off = die->sect_off;
  ofs.type = type;
  slot = (struct dwarf2_per_cu_offset_and_type **)
    htab_find_slot (per_objfile->die_type_hash.get (), &ofs, INSERT);
  if (*slot)
    complaint (_("A problem internal to GDB: DIE %s has type already set"),
	       sect_offset_str (die->sect_off));
  *slot = XOBNEW (&objfile->objfile_obstack,
		  struct dwarf2_per_cu_offset_and_type);
  **slot = ofs;
  return type;
}

/* Look up the type for the die at SECT_OFF in PER_CU in die_type_hash,
   or return NULL if the die does not have a saved type.  */

static struct type *
get_die_type_at_offset (sect_offset sect_off,
			dwarf2_per_cu_data *per_cu,
			dwarf2_per_objfile *per_objfile)
{
  struct dwarf2_per_cu_offset_and_type *slot, ofs;

  if (per_objfile->die_type_hash == NULL)
    return NULL;

  ofs.per_cu = per_cu;
  ofs.sect_off = sect_off;
  slot = ((struct dwarf2_per_cu_offset_and_type *)
	  htab_find (per_objfile->die_type_hash.get (), &ofs));
  if (slot)
    return slot->type;
  else
    return NULL;
}

/* Look up the type for DIE in CU in die_type_hash,
   or return NULL if DIE does not have a saved type.  */

static struct type *
get_die_type (struct die_info *die, struct dwarf2_cu *cu)
{
  return get_die_type_at_offset (die->sect_off, cu->per_cu, cu->per_objfile);
}

struct cmd_list_element *set_dwarf_cmdlist;
struct cmd_list_element *show_dwarf_cmdlist;

static void
show_check_physname (struct ui_file *file, int from_tty,
		     struct cmd_list_element *c, const char *value)
{
  gdb_printf (file,
	      _("Whether to check \"physname\" is %s.\n"),
	      value);
}

void _initialize_dwarf2_read ();
void
_initialize_dwarf2_read ()
{
  add_setshow_prefix_cmd ("dwarf", class_maintenance,
			  _("\
Set DWARF specific variables.\n\
Configure DWARF variables such as the cache size."),
			  _("\
Show DWARF specific variables.\n\
Show DWARF variables such as the cache size."),
			  &set_dwarf_cmdlist, &show_dwarf_cmdlist,
			  &maintenance_set_cmdlist, &maintenance_show_cmdlist);

  add_setshow_zinteger_cmd ("max-cache-age", class_obscure,
			    &dwarf_max_cache_age, _("\
Set the upper bound on the age of cached DWARF compilation units."), _("\
Show the upper bound on the age of cached DWARF compilation units."), _("\
A higher limit means that cached compilation units will be stored\n\
in memory longer, and more total memory will be used.  Zero disables\n\
caching, which can slow down startup."),
			    NULL,
			    show_dwarf_max_cache_age,
			    &set_dwarf_cmdlist,
			    &show_dwarf_cmdlist);

  add_setshow_boolean_cmd ("synchronous", class_obscure,
			    &dwarf_synchronous, _("\
Set whether DWARF is read synchronously."), _("\
Show whether DWARF is read synchronously."), _("\
By default, DWARF information is read in worker threads,\n\
and gdb will not generally wait for the reading to complete\n\
before continuing with other work, for example presenting a\n\
prompt to the user.\n\
Enabling this setting will cause the DWARF reader to always wait\n\
for debug info processing to be finished before gdb can proceed."),
			    nullptr,
			    show_dwarf_synchronous,
			    &set_dwarf_cmdlist,
			    &show_dwarf_cmdlist);

  add_setshow_zuinteger_cmd ("dwarf-read", no_class, &dwarf_read_debug, _("\
Set debugging of the DWARF reader."), _("\
Show debugging of the DWARF reader."), _("\
When enabled (non-zero), debugging messages are printed during DWARF\n\
reading and symtab expansion.  A value of 1 (one) provides basic\n\
information.  A value greater than 1 provides more verbose information."),
			    NULL,
			    NULL,
			    &setdebuglist, &showdebuglist);

  add_setshow_zuinteger_cmd ("dwarf-die", no_class, &dwarf_die_debug, _("\
Set debugging of the DWARF DIE reader."), _("\
Show debugging of the DWARF DIE reader."), _("\
When enabled (non-zero), DIEs are dumped after they are read in.\n\
The value is the maximum depth to print."),
			     NULL,
			     NULL,
			     &setdebuglist, &showdebuglist);

  add_setshow_zuinteger_cmd ("dwarf-line", no_class, &dwarf_line_debug, _("\
Set debugging of the dwarf line reader."), _("\
Show debugging of the dwarf line reader."), _("\
When enabled (non-zero), line number entries are dumped as they are read in.\n\
A value of 1 (one) provides basic information.\n\
A value greater than 1 provides more verbose information."),
			     NULL,
			     NULL,
			     &setdebuglist, &showdebuglist);

  add_setshow_boolean_cmd ("check-physname", no_class, &check_physname, _("\
Set cross-checking of \"physname\" code against demangler."), _("\
Show cross-checking of \"physname\" code against demangler."), _("\
When enabled, GDB's internal \"physname\" code is checked against\n\
the demangler."),
			   NULL, show_check_physname,
			   &setdebuglist, &showdebuglist);

  dwarf2_locexpr_index = register_symbol_computed_impl (LOC_COMPUTED,
							&dwarf2_locexpr_funcs);
  dwarf2_loclist_index = register_symbol_computed_impl (LOC_COMPUTED,
							&dwarf2_loclist_funcs);
  ada_imported_index = register_symbol_computed_impl (LOC_COMPUTED,
						      &ada_imported_funcs);

  dwarf2_locexpr_block_index = register_symbol_block_impl (LOC_BLOCK,
					&dwarf2_block_frame_base_locexpr_funcs);
  dwarf2_loclist_block_index = register_symbol_block_impl (LOC_BLOCK,
					&dwarf2_block_frame_base_loclist_funcs);
  ada_block_index = register_symbol_block_impl (LOC_BLOCK,
						&ada_function_alias_funcs);

#if GDB_SELF_TEST
  selftests::register_test ("dw2_expand_symtabs_matching",
			    selftests::dw2_expand_symtabs_matching::run_test);
  selftests::register_test ("dwarf2_find_containing_comp_unit",
			    selftests::find_containing_comp_unit::run_test);
#endif
}
