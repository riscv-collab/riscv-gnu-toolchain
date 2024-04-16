/* Build symbol tables in GDB's internal format.
   Copyright (C) 1986-2024 Free Software Foundation, Inc.

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

#if !defined (BUILDSYM_H)
#define BUILDSYM_H 1

#include "gdbsupport/gdb_obstack.h"
#include "symtab.h"
#include "addrmap.h"

struct objfile;
struct symbol;
struct addrmap;
struct compunit_symtab;
enum language;

/* This module provides definitions used for creating and adding to
   the symbol table.  These routines are called from various symbol-
   file-reading routines.

   They originated in dbxread.c of gdb-4.2, and were split out to
   make xcoffread.c more maintainable by sharing code.  */

struct block;
struct pending_block;

struct dynamic_prop;

/* The list of sub-source-files within the current individual
   compilation.  Each file gets its own symtab with its own linetable
   and associated info, but they all share one blockvector.  */

struct subfile
{
  subfile () = default;

  /* There's nothing wrong with copying a subfile, but we don't need to, so use
     this to avoid copying one by mistake.  */
  DISABLE_COPY_AND_ASSIGN (subfile);

  struct subfile *next = nullptr;
  std::string name;

  /* This field is analoguous in function to symtab::filename_for_id.

     It is used to look up existing subfiles in calls to start_subfile.  */
  std::string name_for_id;

  std::vector<linetable_entry> line_vector_entries;
  enum language language = language_unknown;
  struct symtab *symtab = nullptr;
};

using subfile_up = std::unique_ptr<subfile>;

/* Record the symbols defined for each context in a list.  We don't
   create a struct block for the context until we know how long to
   make it.  */

#define PENDINGSIZE 100

struct pending
  {
    struct pending *next;
    int nsyms;
    struct symbol *symbol[PENDINGSIZE];
  };

/* Stack representing unclosed lexical contexts (that will become
   blocks, eventually).  */

struct context_stack
  {
    /* Outer locals at the time we entered */

    struct pending *locals;

    /* Pending using directives at the time we entered.  */

    struct using_direct *local_using_directives;

    /* Pointer into blocklist as of entry */

    struct pending_block *old_blocks;

    /* Name of function, if any, defining context */

    struct symbol *name;

    /* Expression that computes the frame base of the lexically enclosing
       function, if any.  NULL otherwise.  */

    struct dynamic_prop *static_link;

    /* PC where this context starts */

    CORE_ADDR start_addr;

    /* Temp slot for exception handling.  */

    CORE_ADDR end_addr;

    /* For error-checking matching push/pop */

    int depth;

  };

/* Flags associated with a linetable entry.  */

enum linetable_entry_flag : unsigned
{
  /* Indicates this PC is a good location to place a breakpoint at LINE.  */
  LEF_IS_STMT = 1 << 1,

  /* Indicates this PC is a good location to place a breakpoint at the first
     instruction past a function prologue.  */
  LEF_PROLOGUE_END = 1 << 2,

  /* Indicated that this PC is part of the epilogue of a function, making
     software watchpoints unreliable.  */
  LEF_EPILOGUE_BEGIN = 1 << 3,
};
DEF_ENUM_FLAGS_TYPE (enum linetable_entry_flag, linetable_entry_flags);


/* Buildsym's counterpart to struct compunit_symtab.  */

struct buildsym_compunit
{
  /* Start recording information about a primary source file (IOW, not an
     included source file).

     COMP_DIR is the directory in which the compilation unit was compiled
     (or NULL if not known).

     NAME and NAME_FOR_ID have the same purpose as for the start_subfile
     method.  */

  buildsym_compunit (struct objfile *objfile_, const char *name,
		     const char *comp_dir_, const char *name_for_id,
		     enum language language_, CORE_ADDR last_addr);

  /* Same as above, but passes NAME for NAME_FOR_ID.  */

  buildsym_compunit (struct objfile *objfile_, const char *name,
		     const char *comp_dir_, enum language language_,
		     CORE_ADDR last_addr)
    : buildsym_compunit (objfile_, name, comp_dir_, name, language_, last_addr)
  {}

  /* Reopen an existing compunit_symtab so that additional symbols can
     be added to it.  Arguments are as for the main constructor.  CUST
     is the expandable compunit_symtab to be reopened.  */

  buildsym_compunit (struct objfile *objfile_, const char *name,
		     const char *comp_dir_, enum language language_,
		     CORE_ADDR last_addr, struct compunit_symtab *cust)
    : m_objfile (objfile_),
      m_last_source_file (name == nullptr ? nullptr : xstrdup (name)),
      m_comp_dir (comp_dir_ == nullptr ? "" : comp_dir_),
      m_compunit_symtab (cust),
      m_language (language_),
      m_last_source_start_addr (last_addr)
  {
  }

  ~buildsym_compunit ();

  DISABLE_COPY_AND_ASSIGN (buildsym_compunit);

  void set_last_source_file (const char *name)
  {
    char *new_name = name == NULL ? NULL : xstrdup (name);
    m_last_source_file.reset (new_name);
  }

  const char *get_last_source_file ()
  {
    return m_last_source_file.get ();
  }

  struct macro_table *get_macro_table ();

  struct macro_table *release_macros ()
  {
    struct macro_table *result = m_pending_macros;
    m_pending_macros = nullptr;
    return result;
  }

  /* This function is called to discard any pending blocks.  */

  void free_pending_blocks ()
  {
    m_pending_block_obstack.clear ();
    m_pending_blocks = nullptr;
  }

  struct block *finish_block (struct symbol *symbol,
			      struct pending_block *old_blocks,
			      const struct dynamic_prop *static_link,
			      CORE_ADDR start, CORE_ADDR end);

  void record_block_range (struct block *block,
			   CORE_ADDR start, CORE_ADDR end_inclusive);

  /* Start recording information about source code that comes from a source
     file.  This sets the current subfile, creating it if necessary.

     NAME is the user-visible name of the subfile.

     NAME_FOR_ID is a name that must be stable between the different calls to
     start_subfile referring to the same file (it is used for looking up
     existing subfiles).  It can be equal to NAME if NAME follows that rule.  */
  void start_subfile (const char *name, const char *name_for_id);

  /* Same as above, but passes NAME for NAME_FOR_ID.  */

  void start_subfile (const char *name)
  {
    return start_subfile (name, name);
  }

  void patch_subfile_names (struct subfile *subfile, const char *name);

  void push_subfile ();

  const char *pop_subfile ();

  void record_line (struct subfile *subfile, int line, unrelocated_addr pc,
		    linetable_entry_flags flags);

  struct compunit_symtab *get_compunit_symtab ()
  {
    return m_compunit_symtab;
  }

  void set_last_source_start_addr (CORE_ADDR addr)
  {
    m_last_source_start_addr = addr;
  }

  CORE_ADDR get_last_source_start_addr ()
  {
    return m_last_source_start_addr;
  }

  struct using_direct **get_local_using_directives ()
  {
    return &m_local_using_directives;
  }

  void set_local_using_directives (struct using_direct *new_local)
  {
    m_local_using_directives = new_local;
  }

  struct using_direct **get_global_using_directives ()
  {
    return &m_global_using_directives;
  }

  bool outermost_context_p () const
  {
    return m_context_stack.empty ();
  }

  struct context_stack *get_current_context_stack ()
  {
    if (m_context_stack.empty ())
      return nullptr;
    return &m_context_stack.back ();
  }

  int get_context_stack_depth () const
  {
    return m_context_stack.size ();
  }

  struct subfile *get_current_subfile ()
  {
    return m_current_subfile;
  }

  struct pending **get_local_symbols ()
  {
    return &m_local_symbols;
  }

  struct pending **get_file_symbols ()
  {
    return &m_file_symbols;
  }

  struct pending **get_global_symbols ()
  {
    return &m_global_symbols;
  }

  void record_debugformat (const char *format)
  {
    m_debugformat = format;
  }

  void record_producer (const char *producer)
  {
    m_producer = producer;
  }

  struct context_stack *push_context (int desc, CORE_ADDR valu);

  struct context_stack pop_context ();

  struct block *end_compunit_symtab_get_static_block
    (CORE_ADDR end_addr, int expandable, int required);

  struct compunit_symtab *end_compunit_symtab_from_static_block
    (struct block *static_block, int expandable);

  struct compunit_symtab *end_compunit_symtab (CORE_ADDR end_addr);

  struct compunit_symtab *end_expandable_symtab (CORE_ADDR end_addr);

  void augment_type_symtab ();

private:

  void record_pending_block (struct block *block, struct pending_block *opblock);

  struct block *finish_block_internal (struct symbol *symbol,
				       struct pending **listhead,
				       struct pending_block *old_blocks,
				       const struct dynamic_prop *static_link,
				       CORE_ADDR start, CORE_ADDR end,
				       int is_global, int expandable);

  struct blockvector *make_blockvector ();

  void watch_main_source_file_lossage ();

  struct compunit_symtab *end_compunit_symtab_with_blockvector
    (struct block *static_block, int expandable);

  /* The objfile we're reading debug info from.  */
  struct objfile *m_objfile;

  /* List of subfiles (source files).
     Files are added to the front of the list.
     This is important mostly for the language determination hacks we use,
     which iterate over previously added files.  */
  struct subfile *m_subfiles = nullptr;

  /* The subfile of the main source file.  */
  struct subfile *m_main_subfile = nullptr;

  /* Name of source file whose symbol data we are now processing.  This
     comes from a symbol of type N_SO for stabs.  For DWARF it comes
     from the DW_AT_name attribute of a DW_TAG_compile_unit DIE.  */
  gdb::unique_xmalloc_ptr<char> m_last_source_file;

  /* E.g., DW_AT_comp_dir if DWARF.  Space for this is malloc'd.  */
  std::string m_comp_dir;

  /* Space for this is not malloc'd, and is assumed to have at least
     the same lifetime as objfile.  */
  const char *m_producer = nullptr;

  /* Space for this is not malloc'd, and is assumed to have at least
     the same lifetime as objfile.  */
  const char *m_debugformat = nullptr;

  /* The compunit we are building.  */
  struct compunit_symtab *m_compunit_symtab = nullptr;

  /* Language of this compunit_symtab.  */
  enum language m_language;

  /* The macro table for the compilation unit whose symbols we're
     currently reading.  */
  struct macro_table *m_pending_macros = nullptr;

  /* True if symtab has line number info.  This prevents an otherwise
     empty symtab from being tossed.  */
  bool m_have_line_numbers = false;

  /* Core address of start of text of current source file.  This too
     comes from the N_SO symbol.  For Dwarf it typically comes from the
     DW_AT_low_pc attribute of a DW_TAG_compile_unit DIE.  */
  CORE_ADDR m_last_source_start_addr;

  /* Stack of subfile names.  */
  std::vector<const char *> m_subfile_stack;

  /* The "using" directives local to lexical context.  */
  struct using_direct *m_local_using_directives = nullptr;

  /* Global "using" directives.  */
  struct using_direct *m_global_using_directives = nullptr;

  /* The stack of contexts that are pushed by push_context and popped
     by pop_context.  */
  std::vector<struct context_stack> m_context_stack;

  struct subfile *m_current_subfile = nullptr;

  /* The mutable address map for the compilation unit whose symbols
     we're currently reading.  The symtabs' shared blockvector will
     point to a fixed copy of this.  */
  struct addrmap_mutable m_pending_addrmap;

  /* True if we recorded any ranges in the addrmap that are different
     from those in the blockvector already.  We set this to false when
     we start processing a symfile, and if it's still false at the
     end, then we just toss the addrmap.  */
  bool m_pending_addrmap_interesting = false;

  /* An obstack used for allocating pending blocks.  */
  auto_obstack m_pending_block_obstack;

  /* Pointer to the head of a linked list of symbol blocks which have
     already been finalized (lexical contexts already closed) and which
     are just waiting to be built into a blockvector when finalizing the
     associated symtab.  */
  struct pending_block *m_pending_blocks = nullptr;

  /* Pending static symbols and types at the top level.  */
  struct pending *m_file_symbols = nullptr;

  /* Pending global functions and variables.  */
  struct pending *m_global_symbols = nullptr;

  /* Pending symbols that are local to the lexical context.  */
  struct pending *m_local_symbols = nullptr;
};



extern void add_symbol_to_list (struct symbol *symbol,
				struct pending **listhead);

extern struct symbol *find_symbol_in_list (struct pending *list,
					   char *name, int length);

#endif /* defined (BUILDSYM_H) */
