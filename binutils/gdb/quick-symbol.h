/* "Quick" symbol functions

   Copyright (C) 2021-2024 Free Software Foundation, Inc.

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

#ifndef GDB_QUICK_SYMBOL_H
#define GDB_QUICK_SYMBOL_H

/* Like block_enum, but used as flags to pass to lookup functions.  */

enum block_search_flag_values
{
  SEARCH_GLOBAL_BLOCK = 1,
  SEARCH_STATIC_BLOCK = 2
};

DEF_ENUM_FLAGS_TYPE (enum block_search_flag_values, block_search_flags);

/* Callback for quick_symbol_functions->map_symbol_filenames.  */

typedef void (symbol_filename_ftype) (const char *filename,
				      const char *fullname);

/* Callback for quick_symbol_functions->expand_symtabs_matching
   to match a file name.  */

typedef bool (expand_symtabs_file_matcher_ftype) (const char *filename,
						  bool basenames);

/* Callback for quick_symbol_functions->expand_symtabs_matching
   to match a symbol name.  */

typedef bool (expand_symtabs_symbol_matcher_ftype) (const char *name);

/* Callback for quick_symbol_functions->expand_symtabs_matching
   to be called after a symtab has been expanded.  If this returns
   true, more symtabs are checked; if it returns false, iteration
   stops.  */

typedef bool (expand_symtabs_exp_notify_ftype) (compunit_symtab *symtab);

/* The "quick" symbol functions exist so that symbol readers can
   avoiding an initial read of all the symbols.  For example, symbol
   readers might choose to use the "partial symbol table" utilities,
   which is one implementation of the quick symbol functions.

   The quick symbol functions are generally opaque: the underlying
   representation is hidden from the caller.

   In general, these functions should only look at whatever special
   index the symbol reader creates -- looking through the symbol
   tables themselves is handled by generic code.  If a function is
   defined as returning a "symbol table", this means that the function
   should only return a newly-created symbol table; it should not
   examine pre-existing ones.

   The exact list of functions here was determined in an ad hoc way
   based on gdb's history.  */

struct quick_symbol_functions
{
  virtual ~quick_symbol_functions ()
  {
  }

  /* Return true if this objfile has any "partial" symbols
     available.  */
  virtual bool has_symbols (struct objfile *objfile) = 0;

  /* Return true if OBJFILE has any unexpanded symtabs.  A return value of
     false indicates there are no unexpanded symtabs, this might mean that
     all of the symtabs have been expanded (full debug has been read in),
     or it might been that OBJFILE has no debug information.  */
  virtual bool has_unexpanded_symtabs (struct objfile *objfile) = 0;

  /* Return the symbol table for the "last" file appearing in
     OBJFILE.  */
  virtual struct symtab *find_last_source_symtab (struct objfile *objfile) = 0;

  /* Forget all cached full file names for OBJFILE.  */
  virtual void forget_cached_source_info (struct objfile *objfile) = 0;

  /* Check to see if the global symbol is defined in a "partial" symbol table
     of OBJFILE. NAME is the name of the symbol to look for.  DOMAIN
     indicates what sort of symbol to search for.

     If found, sets *symbol_found_p to true and returns the symbol language.
     defined, or NULL if no such symbol table exists.  */
  virtual enum language lookup_global_symbol_language
       (struct objfile *objfile,
	const char *name,
	domain_enum domain,
	bool *symbol_found_p) = 0;

  /* Print statistics about any indices loaded for OBJFILE.  The
     statistics should be printed to gdb_stdout.  This is used for
     "maint print statistics".  Statistics are printed in two
     sections.  PRINT_BCACHE is false when printing the first section
     of general statistics, and true when printing bcache statistics.  */
  virtual void print_stats (struct objfile *objfile, bool print_bcache) = 0;

  /* Dump any indices loaded for OBJFILE.  The dump should go to
     gdb_stdout.  This is used for "maint print objfiles".  */
  virtual void dump (struct objfile *objfile) = 0;

  /* Read all symbol tables associated with OBJFILE.  */
  virtual void expand_all_symtabs (struct objfile *objfile) = 0;

  /* Expand all symbol tables in OBJFILE matching some criteria.

     FILE_MATCHER is called for each file in OBJFILE.  The file name
     is passed to it.  If the matcher returns false, the file is
     skipped.  If FILE_MATCHER is NULL the file is not skipped.  If
     BASENAMES is true the matcher should consider only file base
     names (the passed file name is already only the lbasename'd
     part).

     If the file is not skipped, and SYMBOL_MATCHER and LOOKUP_NAME are NULL,
     the symbol table is expanded.

     Otherwise, individual symbols are considered.

     If DOMAIN or KIND do not match, the symbol is skipped.
     If DOMAIN is UNDEF_DOMAIN, that is treated as a wildcard.

     If the symbol name does not match LOOKUP_NAME, the symbol is skipped.

     If SYMBOL_MATCHER returns false, then the symbol is skipped.
     Note that if SYMBOL_MATCHER is non-NULL, then LOOKUP_NAME must
     also be provided.

     Otherwise, the symbol's symbol table is expanded and the
     notification function is called.  If the notification function
     returns false, execution stops and this method returns false.
     Otherwise, more files are considered.  This method will return
     true if all calls to the notification function return true.  */
  virtual bool expand_symtabs_matching
    (struct objfile *objfile,
     gdb::function_view<expand_symtabs_file_matcher_ftype> file_matcher,
     const lookup_name_info *lookup_name,
     gdb::function_view<expand_symtabs_symbol_matcher_ftype> symbol_matcher,
     gdb::function_view<expand_symtabs_exp_notify_ftype> expansion_notify,
     block_search_flags search_flags,
     domain_enum domain,
     enum search_domain kind) = 0;

  /* Return the comp unit from OBJFILE that contains PC and
     SECTION.  Return NULL if there is no such compunit.  This
     should return the compunit that contains a symbol whose
     address exactly matches PC, or, if there is no exact match, the
     compunit that contains a symbol whose address is closest to
     PC.  */
  virtual struct compunit_symtab *find_pc_sect_compunit_symtab
    (struct objfile *objfile, struct bound_minimal_symbol msymbol,
     CORE_ADDR pc, struct obj_section *section, int warn_if_readin) = 0;

  /* Return the comp unit from OBJFILE that contains a symbol at
     ADDRESS.  Return NULL if there is no such comp unit.  Unlike
     find_pc_sect_compunit_symtab, any sort of symbol (not just text
     symbols) can be considered, and only exact address matches are
     considered.  */
  virtual struct compunit_symtab *find_compunit_symtab_by_address
    (struct objfile *objfile, CORE_ADDR address) = 0;

  /* Call a callback for every file defined in OBJFILE whose symtab is
     not already read in.  FUN is the callback.  It is passed the
     file's FILENAME and the file's FULLNAME (if need_fullname is
     non-zero).  */
  virtual void map_symbol_filenames
       (struct objfile *objfile,
	gdb::function_view<symbol_filename_ftype> fun,
	bool need_fullname) = 0;

  /* Compute the name and language of the main function for the given
     objfile.  Normally this is done during symbol reading, but this
     method exists in case this work is done in a worker thread and
     must be waited for.  The implementation can call
     set_objfile_main_name if results are found.  */
  virtual void compute_main_name (struct objfile *objfile)
  {
  }
};

typedef std::unique_ptr<quick_symbol_functions> quick_symbol_functions_up;

#endif /* GDB_QUICK_SYMBOL_H */
