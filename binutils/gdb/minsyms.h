/* Minimal symbol table definitions for GDB.

   Copyright (C) 2011-2024 Free Software Foundation, Inc.

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

#ifndef MINSYMS_H
#define MINSYMS_H

struct type;

/* Several lookup functions return both a minimal symbol and the
   objfile in which it is found.  This structure is used in these
   cases.  */

struct bound_minimal_symbol
{
  bound_minimal_symbol (struct minimal_symbol *msym, struct objfile *objf)
    : minsym (msym),
      objfile (objf)
  {
  }

  bound_minimal_symbol () = default;

  /* Return the address of the minimal symbol in the context of the objfile.  */

  CORE_ADDR value_address () const
  {
    return this->minsym->value_address (this->objfile);
  }

  /* The minimal symbol that was found, or NULL if no minimal symbol
     was found.  */

  struct minimal_symbol *minsym = nullptr;

  /* If MINSYM is not NULL, then this is the objfile in which the
     symbol is defined.  */

  struct objfile *objfile = nullptr;

  /* Return the obj_section from OBJFILE for MINSYM.  */

  struct obj_section *obj_section () const
  {
    return minsym->obj_section (objfile);
  }
};

/* This header declares most of the API for dealing with minimal
   symbols and minimal symbol tables.  A few things are declared
   elsewhere; see below.

   A minimal symbol is a symbol for which there is no direct debug
   information.  For example, for an ELF binary, minimal symbols are
   created from the ELF symbol table.

   For the definition of the minimal symbol structure, see struct
   minimal_symbol in symtab.h.

   Minimal symbols are stored in tables attached to an objfile; see
   objfiles.h for details.  Code should generally treat these tables
   as opaque and use functions provided by minsyms.c to inspect them.
*/

struct msym_bunch;

/* An RAII-based object that is used to record minimal symbols while
   they are being read.  */
class minimal_symbol_reader
{
 public:

  /* Prepare to start collecting minimal symbols.  This should be
     called by a symbol reader to initialize the minimal symbol
     module.  */

  explicit minimal_symbol_reader (struct objfile *);

  ~minimal_symbol_reader ();

  /* Install the minimal symbols that have been collected into the
     given objfile.  */

  void install ();

  /* Record a new minimal symbol.  This is the "full" entry point;
     simpler convenience entry points are also provided below.
   
     This returns a new minimal symbol.  It is ok to modify the returned
     minimal symbol (though generally not necessary).  It is not ok,
     though, to stash the pointer anywhere; as minimal symbols may be
     moved after creation.  The memory for the returned minimal symbol
     is still owned by the minsyms.c code, and should not be freed.
   
     Arguments are:

     NAME - the symbol's name
     COPY_NAME - if true, the minsym code must make a copy of NAME.  If
     false, then NAME must be NUL-terminated, and must have a lifetime
     that is at least as long as OBJFILE's lifetime.
     ADDRESS - the address of the symbol
     MS_TYPE - the type of the symbol
     SECTION - the symbol's section
  */

  struct minimal_symbol *record_full (std::string_view name,
				      bool copy_name,
				      unrelocated_addr address,
				      enum minimal_symbol_type ms_type,
				      int section);

  /* Like record_full, but:
     - computes the length of NAME
     - passes COPY_NAME = true,
     - and passes a default SECTION, depending on the type

     This variant does not return the new symbol.  */

  void record (const char *name, unrelocated_addr address,
	       enum minimal_symbol_type ms_type);

  /* Like record_full, but:
     - computes the length of NAME
     - passes COPY_NAME = true.

     This variant does not return the new symbol.  */

  void record_with_info (const char *name, unrelocated_addr address,
			 enum minimal_symbol_type ms_type,
			 int section)
  {
    record_full (name, true, address, ms_type, section);
  }

 private:

  DISABLE_COPY_AND_ASSIGN (minimal_symbol_reader);

  struct objfile *m_objfile;

  /* Bunch currently being filled up.
     The next field points to chain of filled bunches.  */

  struct msym_bunch *m_msym_bunch;

  /* Number of slots filled in current bunch.  */

  int m_msym_bunch_index;

  /* Total number of minimal symbols recorded so far for the
     objfile.  */

  int m_msym_count;
};



/* Return whether MSYMBOL is a function/method.  If FUNC_ADDRESS_P is
   non-NULL, and the MSYMBOL is a function, then *FUNC_ADDRESS_P is
   set to the function's address, already resolved if MINSYM points to
   a function descriptor.  */

bool msymbol_is_function (struct objfile *objfile,
			  minimal_symbol *minsym,
			  CORE_ADDR *func_address_p = NULL);

/* Compute a hash code for the string argument.  Unlike htab_hash_string,
   this is a case-insensitive hash to support "set case-sensitive off".  */

unsigned int msymbol_hash (const char *);

/* Like msymbol_hash, but compute a hash code that is compatible with
   strcmp_iw.  */

unsigned int msymbol_hash_iw (const char *);

/* Compute the next hash value from previous HASH and the character C.  This
   is only a GDB in-memory computed value with no external files compatibility
   requirements.  */

#define SYMBOL_HASH_NEXT(hash, c)			\
  ((hash) * 67 + TOLOWER ((unsigned char) (c)) - 113)



/* Look through all the current minimal symbol tables and find the
   first minimal symbol that matches NAME.  If OBJF is non-NULL, limit
   the search to that objfile.  If SFILE is non-NULL, the only
   file-scope symbols considered will be from that source file (global
   symbols are still preferred).  Returns a bound minimal symbol that
   matches, or an empty bound minimal symbol if no match is found.  */

struct bound_minimal_symbol lookup_minimal_symbol (const char *,
						   const char *,
						   struct objfile *);

/* Like lookup_minimal_symbol, but searches all files and
   objfiles.  */

struct bound_minimal_symbol lookup_bound_minimal_symbol (const char *);

/* Look through all the current minimal symbol tables and find the
   first minimal symbol that matches NAME and has text type.  If OBJF
   is non-NULL, limit the search to that objfile.  Returns a bound
   minimal symbol that matches, or an "empty" bound minimal symbol
   otherwise.

   This function only searches the mangled (linkage) names.  */

struct bound_minimal_symbol lookup_minimal_symbol_text (const char *,
							struct objfile *);

/* Look through the minimal symbols in OBJF (and its separate debug
   objfiles) for a global (not file-local) minsym whose linkage name
   is NAME.  This is somewhat similar to lookup_minimal_symbol_text,
   only data symbols (not text symbols) are considered, and a non-NULL
   objfile is not accepted.  Returns a bound minimal symbol that
   matches, or an "empty" bound minimal symbol otherwise.  */

extern struct bound_minimal_symbol lookup_minimal_symbol_linkage
  (const char *name, struct objfile *objf)
  ATTRIBUTE_NONNULL (1) ATTRIBUTE_NONNULL (2);

/* A variant of lookup_minimal_symbol_linkage that iterates over all
   objfiles.  If ONLY_MAIN is true, then only an objfile with
   OBJF_MAINLINE will be considered.  */

extern struct bound_minimal_symbol lookup_minimal_symbol_linkage
  (const char *name, bool only_main)
  ATTRIBUTE_NONNULL (1);

/* Look through all the current minimal symbol tables and find the
   first minimal symbol that matches NAME and PC.  If OBJF is non-NULL,
   limit the search to that objfile.  Returns a pointer to the minimal
   symbol that matches, or NULL if no match is found.  */

struct minimal_symbol *lookup_minimal_symbol_by_pc_name
    (CORE_ADDR, const char *, struct objfile *);

enum class lookup_msym_prefer
{
  /* Prefer mst_text symbols.  */
  TEXT,

  /* Prefer mst_solib_trampoline symbols when there are text and
     trampoline symbols at the same address.  Otherwise prefer
     mst_text symbols.  */
  TRAMPOLINE,

  /* Prefer mst_text_gnu_ifunc symbols when there are text and ifunc
     symbols at the same address.  Otherwise prefer mst_text
     symbols.  */
  GNU_IFUNC,
};

/* Search through the minimal symbol table for each objfile and find
   the symbol whose address is the largest address that is still less
   than or equal to PC_IN, and which matches SECTION.  A matching symbol
   must either be zero sized and have address PC_IN, or PC_IN must fall
   within the range of addresses covered by the matching symbol.

   If SECTION is NULL, this uses the result of find_pc_section
   instead.

   The result has a non-NULL 'minsym' member if such a symbol is
   found, or NULL if PC is not in a suitable range.

   See definition of lookup_msym_prefer for description of PREFER.  By
   default mst_text symbols are preferred.

   If the PREVIOUS pointer is non-NULL, and no matching symbol is found,
   then the contents will be set to reference the closest symbol before
   PC_IN.  */

struct bound_minimal_symbol lookup_minimal_symbol_by_pc_section
  (CORE_ADDR pc_in,
   struct obj_section *section,
   lookup_msym_prefer prefer = lookup_msym_prefer::TEXT,
   bound_minimal_symbol *previous = nullptr);

/* Backward compatibility: search through the minimal symbol table 
   for a matching PC (no section given).
   
   This is a wrapper that calls lookup_minimal_symbol_by_pc_section
   with a NULL section argument.  */

struct bound_minimal_symbol lookup_minimal_symbol_by_pc (CORE_ADDR);

/* Iterate over all the minimal symbols in the objfile OBJF which
   match NAME.  Both the ordinary and demangled names of each symbol
   are considered.  The caller is responsible for canonicalizing NAME,
   should that need to be done.

   For each matching symbol, CALLBACK is called with the symbol.  */

void iterate_over_minimal_symbols
    (struct objfile *objf, const lookup_name_info &name,
     gdb::function_view<bool (struct minimal_symbol *)> callback);

/* Compute the upper bound of MINSYM.  The upper bound is the last
   address thought to be part of the symbol.  If the symbol has a
   size, it is used.  Otherwise use the lesser of the next minimal
   symbol in the same section, or the end of the section, as the end
   of the function.  */

CORE_ADDR minimal_symbol_upper_bound (struct bound_minimal_symbol minsym);

/* Return the type of MSYMBOL, a minimal symbol of OBJFILE.  If
   ADDRESS_P is not NULL, set it to the MSYMBOL's resolved
   address.  */

type *find_minsym_type_and_address (minimal_symbol *msymbol, objfile *objf,
				    CORE_ADDR *address_p);

#endif /* MINSYMS_H */
