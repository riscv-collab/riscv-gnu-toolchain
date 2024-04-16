/* Dynamic architecture support for GDB, the GNU debugger.

   Copyright (C) 1998-2024 Free Software Foundation, Inc.

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


#ifndef GDBARCH_H
#define GDBARCH_H

#include <vector>
#include "frame.h"
#include "dis-asm.h"
#include "gdbsupport/gdb_obstack.h"
#include "infrun.h"
#include "osabi.h"
#include "displaced-stepping.h"
#include "gdbsupport/gdb-checked-static-cast.h"
#include "registry.h"

struct floatformat;
struct ui_file;
struct value;
struct objfile;
struct obj_section;
struct minimal_symbol;
struct regcache;
struct reggroup;
struct regset;
struct disassemble_info;
struct target_ops;
struct obstack;
struct bp_target_info;
struct target_desc;
struct symbol;
struct syscall;
struct agent_expr;
struct axs_value;
struct stap_parse_info;
struct expr_builder;
struct ravenscar_arch_ops;
struct mem_range;
struct syscalls_info;
struct thread_info;
struct ui_out;
struct inferior;
struct x86_xsave_layout;

#include "regcache.h"

/* The base class for every architecture's tdep sub-class.  The virtual
   destructor ensures the class has RTTI information, which allows
   gdb::checked_static_cast to be used in the gdbarch_tdep function.  */

struct gdbarch_tdep_base
{
  virtual ~gdbarch_tdep_base() = default;
};

using gdbarch_tdep_up = std::unique_ptr<gdbarch_tdep_base>;

/* Callback type for the 'iterate_over_objfiles_in_search_order'
   gdbarch  method.  */

using iterate_over_objfiles_in_search_order_cb_ftype
  = gdb::function_view<bool(objfile *)>;

/* Callback type for regset section iterators.  The callback usually
   invokes the REGSET's supply or collect method, to which it must
   pass a buffer - for collects this buffer will need to be created using
   COLLECT_SIZE, for supply the existing buffer being read from should
   be at least SUPPLY_SIZE.  SECT_NAME is a BFD section name, and HUMAN_NAME
   is used for diagnostic messages.  CB_DATA should have been passed
   unchanged through the iterator.  */

typedef void (iterate_over_regset_sections_cb)
  (const char *sect_name, int supply_size, int collect_size,
   const struct regset *regset, const char *human_name, void *cb_data);

/* For a function call, does the function return a value using a
   normal value return or a structure return - passing a hidden
   argument pointing to storage.  For the latter, there are two
   cases: language-mandated structure return and target ABI
   structure return.  */

enum function_call_return_method
{
  /* Standard value return.  */
  return_method_normal = 0,

  /* Language ABI structure return.  This is handled
     by passing the return location as the first parameter to
     the function, even preceding "this".  */
  return_method_hidden_param,

  /* Target ABI struct return.  This is target-specific; for instance,
     on ia64 the first argument is passed in out0 but the hidden
     structure return pointer would normally be passed in r8.  */
  return_method_struct,
};

enum class memtag_type
{
  /* Logical tag, the tag that is stored in unused bits of a pointer to a
     virtual address.  */
  logical = 0,

  /* Allocation tag, the tag that is associated with every granule of memory in
     the physical address space.  Allocation tags are used to validate memory
     accesses via pointers containing logical tags.  */
  allocation,
};

/* Callback types for 'read_core_file_mappings' gdbarch method.  */

using read_core_file_mappings_pre_loop_ftype =
  gdb::function_view<void (ULONGEST count)>;

using read_core_file_mappings_loop_ftype =
  gdb::function_view<void (int num,
			   ULONGEST start,
			   ULONGEST end,
			   ULONGEST file_ofs,
			   const char *filename,
			   const bfd_build_id *build_id)>;

/* Possible values for gdbarch_call_dummy_location.  */
enum call_dummy_location_type
{
  ON_STACK,
  AT_ENTRY_POINT,
};

#include "gdbarch-gen.h"

/* An internal function that should _only_ be called from gdbarch_tdep.
   Returns the gdbarch_tdep_base field held within GDBARCH.  */

extern struct gdbarch_tdep_base *gdbarch_tdep_1 (struct gdbarch *gdbarch);

/* Return the gdbarch_tdep_base object held within GDBARCH cast to the type
   TDepType, which should be a sub-class of gdbarch_tdep_base.

   When GDB is compiled in maintainer mode a run-time check is performed
   that the gdbarch_tdep_base within GDBARCH really is of type TDepType.
   When GDB is compiled in release mode the run-time check is not
   performed, and we assume the caller knows what they are doing.  */

template<typename TDepType>
static inline TDepType *
gdbarch_tdep (struct gdbarch *gdbarch)
{
  struct gdbarch_tdep_base *tdep = gdbarch_tdep_1 (gdbarch);
  return gdb::checked_static_cast<TDepType *> (tdep);
}

/* Mechanism for co-ordinating the selection of a specific
   architecture.

   GDB targets (*-tdep.c) can register an interest in a specific
   architecture.  Other GDB components can register a need to maintain
   per-architecture data.

   The mechanisms below ensures that there is only a loose connection
   between the set-architecture command and the various GDB
   components.  Each component can independently register their need
   to maintain architecture specific data with gdbarch.

   Pragmatics:

   Previously, a single TARGET_ARCHITECTURE_HOOK was provided.  It
   didn't scale.

   The more traditional mega-struct containing architecture specific
   data for all the various GDB components was also considered.  Since
   GDB is built from a variable number of (fairly independent)
   components it was determined that the global approach was not
   applicable.  */


/* Register a new architectural family with GDB.

   Register support for the specified ARCHITECTURE with GDB.  When
   gdbarch determines that the specified architecture has been
   selected, the corresponding INIT function is called.

   --

   The INIT function takes two parameters: INFO which contains the
   information available to gdbarch about the (possibly new)
   architecture; ARCHES which is a list of the previously created
   ``struct gdbarch'' for this architecture.

   The INFO parameter is, as far as possible, be pre-initialized with
   information obtained from INFO.ABFD or the global defaults.

   The ARCHES parameter is a linked list (sorted most recently used)
   of all the previously created architures for this architecture
   family.  The (possibly NULL) ARCHES->gdbarch can used to access
   values from the previously selected architecture for this
   architecture family.

   The INIT function shall return any of: NULL - indicating that it
   doesn't recognize the selected architecture; an existing ``struct
   gdbarch'' from the ARCHES list - indicating that the new
   architecture is just a synonym for an earlier architecture (see
   gdbarch_list_lookup_by_info()); a newly created ``struct gdbarch''
   - that describes the selected architecture (see gdbarch_alloc()).

   The DUMP_TDEP function shall print out all target specific values.
   Care should be taken to ensure that the function works in both the
   multi-arch and non- multi-arch cases.  */

struct gdbarch_list
{
  struct gdbarch *gdbarch;
  struct gdbarch_list *next;
};

struct gdbarch_info
{
  const struct bfd_arch_info *bfd_arch_info = nullptr;

  enum bfd_endian byte_order = BFD_ENDIAN_UNKNOWN;

  enum bfd_endian byte_order_for_code = BFD_ENDIAN_UNKNOWN;

  bfd *abfd = nullptr;

  /* Architecture-specific target description data.  */
  struct tdesc_arch_data *tdesc_data = nullptr;

  enum gdb_osabi osabi = GDB_OSABI_UNKNOWN;

  const struct target_desc *target_desc = nullptr;
};

typedef struct gdbarch *(gdbarch_init_ftype) (struct gdbarch_info info, struct gdbarch_list *arches);
typedef void (gdbarch_dump_tdep_ftype) (struct gdbarch *gdbarch, struct ui_file *file);
typedef bool (gdbarch_supports_arch_info_ftype) (const struct bfd_arch_info *);

extern void gdbarch_register (enum bfd_architecture architecture,
			      gdbarch_init_ftype *init,
			      gdbarch_dump_tdep_ftype *dump_tdep = nullptr,
			      gdbarch_supports_arch_info_ftype *supports_arch_info = nullptr);

/* Return true if ARCH is initialized.  */

bool gdbarch_initialized_p (gdbarch *arch);

/* Return a vector of the valid architecture names.  Since architectures are
   registered during the _initialize phase this function only returns useful
   information once initialization has been completed.  */

extern std::vector<const char *> gdbarch_printable_names ();


/* Helper function.  Search the list of ARCHES for a GDBARCH that
   matches the information provided by INFO.  */

extern struct gdbarch_list *gdbarch_list_lookup_by_info (struct gdbarch_list *arches, const struct gdbarch_info *info);


/* Helper function.  Create a preliminary ``struct gdbarch''.  Perform
   basic initialization using values obtained from the INFO and TDEP
   parameters.  set_gdbarch_*() functions are called to complete the
   initialization of the object.  */

extern struct gdbarch *gdbarch_alloc (const struct gdbarch_info *info,
				      gdbarch_tdep_up tdep);


/* Helper function.  Free a partially-constructed ``struct gdbarch''.
   It is assumed that the caller frees the ``struct
   gdbarch_tdep''.  */

extern void gdbarch_free (struct gdbarch *);

struct gdbarch_deleter
{
  void operator() (gdbarch *arch) const
  { gdbarch_free (arch); }
};

using gdbarch_up = std::unique_ptr<gdbarch, gdbarch_deleter>;

/* Get the obstack owned by ARCH.  */

extern obstack *gdbarch_obstack (gdbarch *arch);

/* Helper function.  Allocate memory from the ``struct gdbarch''
   obstack.  The memory is freed when the corresponding architecture
   is also freed.  */

#define GDBARCH_OBSTACK_CALLOC(GDBARCH, NR, TYPE)   obstack_calloc<TYPE> (gdbarch_obstack ((GDBARCH)), (NR))

#define GDBARCH_OBSTACK_ZALLOC(GDBARCH, TYPE)   obstack_zalloc<TYPE> (gdbarch_obstack ((GDBARCH)))

/* Duplicate STRING, returning an equivalent string that's allocated on the
   obstack associated with GDBARCH.  The string is freed when the corresponding
   architecture is also freed.  */

extern char *gdbarch_obstack_strdup (struct gdbarch *arch, const char *string);

/* Helper function.  Force an update of the current architecture.

   The actual architecture selected is determined by INFO, ``(gdb) set
   architecture'' et.al., the existing architecture and BFD's default
   architecture.  INFO should be initialized to zero and then selected
   fields should be updated.

   Returns non-zero if the update succeeds.  */

extern int gdbarch_update_p (struct gdbarch_info info);


/* Helper function.  Find an architecture matching info.

   INFO should have relevant fields set, and then finished using
   gdbarch_info_fill.

   Returns the corresponding architecture, or NULL if no matching
   architecture was found.  */

extern struct gdbarch *gdbarch_find_by_info (struct gdbarch_info info);

/* A registry adaptor for gdbarch.  This arranges to store the
   registry in the gdbarch.  */
template<>
struct registry_accessor<gdbarch>
{
  static registry<gdbarch> *get (gdbarch *arch);
};

/* Set the dynamic target-system-dependent parameters (architecture,
   byte-order, ...) using information found in the BFD.  */

extern void set_gdbarch_from_file (bfd *);


/* Initialize the current architecture to the "first" one we find on
   our list.  */

extern void initialize_current_architecture (void);

/* gdbarch trace variable */
extern unsigned int gdbarch_debug;

extern void gdbarch_dump (struct gdbarch *gdbarch, struct ui_file *file);

/* Return the number of cooked registers (raw + pseudo) for ARCH.  */

static inline int
gdbarch_num_cooked_regs (gdbarch *arch)
{
  return gdbarch_num_regs (arch) + gdbarch_num_pseudo_regs (arch);
}

#endif
