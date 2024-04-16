/* GDB routines for manipulating the minimal symbol tables.
   Copyright (C) 1992-2024 Free Software Foundation, Inc.
   Contributed by Cygnus Support, using pieces from other GDB modules.

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


/* This file contains support routines for creating, manipulating, and
   destroying minimal symbol tables.

   Minimal symbol tables are used to hold some very basic information about
   all defined global symbols (text, data, bss, abs, etc).  The only two
   required pieces of information are the symbol's name and the address
   associated with that symbol.

   In many cases, even if a file was compiled with no special options for
   debugging at all, as long as was not stripped it will contain sufficient
   information to build useful minimal symbol tables using this structure.

   Even when a file contains enough debugging information to build a full
   symbol table, these minimal symbols are still useful for quickly mapping
   between names and addresses, and vice versa.  They are also sometimes used
   to figure out what full symbol table entries need to be read in.  */


#include "defs.h"
#include <ctype.h>
#include "symtab.h"
#include "bfd.h"
#include "filenames.h"
#include "symfile.h"
#include "objfiles.h"
#include "demangle.h"
#include "value.h"
#include "cp-abi.h"
#include "target.h"
#include "cp-support.h"
#include "language.h"
#include "cli/cli-utils.h"
#include "gdbsupport/symbol.h"
#include <algorithm>
#include "gdbsupport/gdb-safe-ctype.h"
#include "gdbsupport/parallel-for.h"
#include "inferior.h"

#if CXX_STD_THREAD
#include <mutex>
#endif

/* Return true if MINSYM is a cold clone symbol.
   Recognize f.i. these symbols (mangled/demangled):
   - _ZL3foov.cold
     foo() [clone .cold]
   - _ZL9do_rpo_vnP8functionP8edge_defP11bitmap_headbb.cold.138
     do_rpo_vn(function*, edge_def*, bitmap_head*, bool, bool)	\
       [clone .cold.138].  */

static bool
msymbol_is_cold_clone (minimal_symbol *minsym)
{
  const char *name = minsym->natural_name ();
  size_t name_len = strlen (name);
  if (name_len < 1)
    return false;

  const char *last = &name[name_len - 1];
  if (*last != ']')
    return false;

  const char *suffix = " [clone .cold";
  size_t suffix_len = strlen (suffix);
  const char *found = strstr (name, suffix);
  if (found == nullptr)
    return false;

  const char *start = &found[suffix_len];
  if (*start == ']')
    return true;

  if (*start != '.')
    return false;

  const char *p;
  for (p = start + 1; p <= last; ++p)
    {
      if (*p >= '0' && *p <= '9')
	continue;
      break;
    }

  if (p == last)
    return true;

  return false;
}

/* See minsyms.h.  */

bool
msymbol_is_function (struct objfile *objfile, minimal_symbol *minsym,
		     CORE_ADDR *func_address_p)
{
  CORE_ADDR msym_addr = minsym->value_address (objfile);

  switch (minsym->type ())
    {
    case mst_slot_got_plt:
    case mst_data:
    case mst_bss:
    case mst_abs:
    case mst_file_data:
    case mst_file_bss:
    case mst_data_gnu_ifunc:
      {
	struct gdbarch *gdbarch = objfile->arch ();
	CORE_ADDR pc = gdbarch_convert_from_func_ptr_addr
	  (gdbarch, msym_addr, current_inferior ()->top_target ());
	if (pc != msym_addr)
	  {
	    if (func_address_p != NULL)
	      *func_address_p = pc;
	    return true;
	  }
	return false;
      }
    case mst_file_text:
      /* Ignore function symbol that is not a function entry.  */
      if (msymbol_is_cold_clone (minsym))
	return false;
      [[fallthrough]];
    default:
      if (func_address_p != NULL)
	*func_address_p = msym_addr;
      return true;
    }
}

/* Accumulate the minimal symbols for each objfile in bunches of BUNCH_SIZE.
   At the end, copy them all into one newly allocated array.  */

#define BUNCH_SIZE 127

struct msym_bunch
  {
    struct msym_bunch *next;
    struct minimal_symbol contents[BUNCH_SIZE];
  };

/* See minsyms.h.  */

unsigned int
msymbol_hash_iw (const char *string)
{
  unsigned int hash = 0;

  while (*string && *string != '(')
    {
      string = skip_spaces (string);
      if (*string && *string != '(')
	{
	  hash = SYMBOL_HASH_NEXT (hash, *string);
	  ++string;
	}
    }
  return hash;
}

/* See minsyms.h.  */

unsigned int
msymbol_hash (const char *string)
{
  unsigned int hash = 0;

  for (; *string; ++string)
    hash = SYMBOL_HASH_NEXT (hash, *string);
  return hash;
}

/* Add the minimal symbol SYM to an objfile's minsym hash table, TABLE.  */
static void
add_minsym_to_hash_table (struct minimal_symbol *sym,
			  struct minimal_symbol **table,
			  unsigned int hash_value)
{
  if (sym->hash_next == NULL)
    {
      unsigned int hash = hash_value % MINIMAL_SYMBOL_HASH_SIZE;

      sym->hash_next = table[hash];
      table[hash] = sym;
    }
}

/* Add the minimal symbol SYM to an objfile's minsym demangled hash table,
   TABLE.  */
static void
add_minsym_to_demangled_hash_table (struct minimal_symbol *sym,
				    struct objfile *objfile,
				    unsigned int hash_value)
{
  if (sym->demangled_hash_next == NULL)
    {
      objfile->per_bfd->demangled_hash_languages.set (sym->language ());

      struct minimal_symbol **table
	= objfile->per_bfd->msymbol_demangled_hash;
      unsigned int hash_index = hash_value % MINIMAL_SYMBOL_HASH_SIZE;
      sym->demangled_hash_next = table[hash_index];
      table[hash_index] = sym;
    }
}

/* Worker object for lookup_minimal_symbol.  Stores temporary results
   while walking the symbol tables.  */

struct found_minimal_symbols
{
  /* External symbols are best.  */
  bound_minimal_symbol external_symbol;

  /* File-local symbols are next best.  */
  bound_minimal_symbol file_symbol;

  /* Symbols for shared library trampolines are next best.  */
  bound_minimal_symbol trampoline_symbol;

  /* Called when a symbol name matches.  Check if the minsym is a
     better type than what we had already found, and record it in one
     of the members fields if so.  Returns true if we collected the
     real symbol, in which case we can stop searching.  */
  bool maybe_collect (const char *sfile, objfile *objf,
		      minimal_symbol *msymbol);
};

/* See declaration above.  */

bool
found_minimal_symbols::maybe_collect (const char *sfile,
				      struct objfile *objfile,
				      minimal_symbol *msymbol)
{
  switch (msymbol->type ())
    {
    case mst_file_text:
    case mst_file_data:
    case mst_file_bss:
      if (sfile == NULL
	  || filename_cmp (msymbol->filename, sfile) == 0)
	{
	  file_symbol.minsym = msymbol;
	  file_symbol.objfile = objfile;
	}
      break;

    case mst_solib_trampoline:

      /* If a trampoline symbol is found, we prefer to keep
	 looking for the *real* symbol.  If the actual symbol
	 is not found, then we'll use the trampoline
	 entry.  */
      if (trampoline_symbol.minsym == NULL)
	{
	  trampoline_symbol.minsym = msymbol;
	  trampoline_symbol.objfile = objfile;
	}
      break;

    case mst_unknown:
    default:
      external_symbol.minsym = msymbol;
      external_symbol.objfile = objfile;
      /* We have the real symbol.  No use looking further.  */
      return true;
    }

  /* Keep looking.  */
  return false;
}

/* Walk the mangled name hash table, and pass each symbol whose name
   matches LOOKUP_NAME according to NAMECMP to FOUND.  */

static void
lookup_minimal_symbol_mangled (const char *lookup_name,
			       const char *sfile,
			       struct objfile *objfile,
			       struct minimal_symbol **table,
			       unsigned int hash,
			       int (*namecmp) (const char *, const char *),
			       found_minimal_symbols &found)
{
  for (minimal_symbol *msymbol = table[hash];
       msymbol != NULL;
       msymbol = msymbol->hash_next)
    {
      const char *symbol_name = msymbol->linkage_name ();

      if (namecmp (symbol_name, lookup_name) == 0
	  && found.maybe_collect (sfile, objfile, msymbol))
	return;
    }
}

/* Walk the demangled name hash table, and pass each symbol whose name
   matches LOOKUP_NAME according to MATCHER to FOUND.  */

static void
lookup_minimal_symbol_demangled (const lookup_name_info &lookup_name,
				 const char *sfile,
				 struct objfile *objfile,
				 struct minimal_symbol **table,
				 unsigned int hash,
				 symbol_name_matcher_ftype *matcher,
				 found_minimal_symbols &found)
{
  for (minimal_symbol *msymbol = table[hash];
       msymbol != NULL;
       msymbol = msymbol->demangled_hash_next)
    {
      const char *symbol_name = msymbol->search_name ();

      if (matcher (symbol_name, lookup_name, NULL)
	  && found.maybe_collect (sfile, objfile, msymbol))
	return;
    }
}

/* Look through all the current minimal symbol tables and find the
   first minimal symbol that matches NAME.  If OBJF is non-NULL, limit
   the search to that objfile.  If SFILE is non-NULL, the only file-scope
   symbols considered will be from that source file (global symbols are
   still preferred).  Returns a pointer to the minimal symbol that
   matches, or NULL if no match is found.

   Note:  One instance where there may be duplicate minimal symbols with
   the same name is when the symbol tables for a shared library and the
   symbol tables for an executable contain global symbols with the same
   names (the dynamic linker deals with the duplication).

   It's also possible to have minimal symbols with different mangled
   names, but identical demangled names.  For example, the GNU C++ v3
   ABI requires the generation of two (or perhaps three) copies of
   constructor functions --- "in-charge", "not-in-charge", and
   "allocate" copies; destructors may be duplicated as well.
   Obviously, there must be distinct mangled names for each of these,
   but the demangled names are all the same: S::S or S::~S.  */

struct bound_minimal_symbol
lookup_minimal_symbol (const char *name, const char *sfile,
		       struct objfile *objf)
{
  found_minimal_symbols found;

  unsigned int mangled_hash = msymbol_hash (name) % MINIMAL_SYMBOL_HASH_SIZE;

  auto *mangled_cmp
    = (case_sensitivity == case_sensitive_on
       ? strcmp
       : strcasecmp);

  if (sfile != NULL)
    sfile = lbasename (sfile);

  lookup_name_info lookup_name (name, symbol_name_match_type::FULL);

  for (objfile *objfile : current_program_space->objfiles ())
    {
      if (found.external_symbol.minsym != NULL)
	break;

      if (objf == NULL || objf == objfile
	  || objf == objfile->separate_debug_objfile_backlink)
	{
	  symbol_lookup_debug_printf ("lookup_minimal_symbol (%s, %s, %s)",
				      name, sfile != NULL ? sfile : "NULL",
				      objfile_debug_name (objfile));

	  /* Do two passes: the first over the ordinary hash table,
	     and the second over the demangled hash table.  */
	  lookup_minimal_symbol_mangled (name, sfile, objfile,
					 objfile->per_bfd->msymbol_hash,
					 mangled_hash, mangled_cmp, found);

	  /* If not found, try the demangled hash table.  */
	  if (found.external_symbol.minsym == NULL)
	    {
	      /* Once for each language in the demangled hash names
		 table (usually just zero or one languages).  */
	      for (unsigned iter = 0; iter < nr_languages; ++iter)
		{
		  if (!objfile->per_bfd->demangled_hash_languages.test (iter))
		    continue;
		  enum language lang = (enum language) iter;

		  unsigned int hash
		    = (lookup_name.search_name_hash (lang)
		       % MINIMAL_SYMBOL_HASH_SIZE);

		  symbol_name_matcher_ftype *match
		    = language_def (lang)->get_symbol_name_matcher
							(lookup_name);
		  struct minimal_symbol **msymbol_demangled_hash
		    = objfile->per_bfd->msymbol_demangled_hash;

		  lookup_minimal_symbol_demangled (lookup_name, sfile, objfile,
						   msymbol_demangled_hash,
						   hash, match, found);

		  if (found.external_symbol.minsym != NULL)
		    break;
		}
	    }
	}
    }

  /* External symbols are best.  */
  if (found.external_symbol.minsym != NULL)
    {
      if (symbol_lookup_debug)
	{
	  minimal_symbol *minsym = found.external_symbol.minsym;

	  symbol_lookup_debug_printf
	    ("lookup_minimal_symbol (...) = %s (external)",
	     host_address_to_string (minsym));
	}
      return found.external_symbol;
    }

  /* File-local symbols are next best.  */
  if (found.file_symbol.minsym != NULL)
    {
      if (symbol_lookup_debug)
	{
	  minimal_symbol *minsym = found.file_symbol.minsym;

	  symbol_lookup_debug_printf
	    ("lookup_minimal_symbol (...) = %s (file-local)",
	     host_address_to_string (minsym));
	}
      return found.file_symbol;
    }

  /* Symbols for shared library trampolines are next best.  */
  if (found.trampoline_symbol.minsym != NULL)
    {
      if (symbol_lookup_debug)
	{
	  minimal_symbol *minsym = found.trampoline_symbol.minsym;

	  symbol_lookup_debug_printf
	    ("lookup_minimal_symbol (...) = %s (trampoline)",
	     host_address_to_string (minsym));
	}

      return found.trampoline_symbol;
    }

  /* Not found.  */
  symbol_lookup_debug_printf ("lookup_minimal_symbol (...) = NULL");
  return {};
}

/* See minsyms.h.  */

struct bound_minimal_symbol
lookup_bound_minimal_symbol (const char *name)
{
  return lookup_minimal_symbol (name, NULL, NULL);
}

/* See gdbsupport/symbol.h.  */

int
find_minimal_symbol_address (const char *name, CORE_ADDR *addr,
			     struct objfile *objfile)
{
  struct bound_minimal_symbol sym
    = lookup_minimal_symbol (name, NULL, objfile);

  if (sym.minsym != NULL)
    *addr = sym.value_address ();

  return sym.minsym == NULL;
}

/* Get the lookup name form best suitable for linkage name
   matching.  */

static const char *
linkage_name_str (const lookup_name_info &lookup_name)
{
  /* Unlike most languages (including C++), Ada uses the
     encoded/linkage name as the search name recorded in symbols.  So
     if debugging in Ada mode, prefer the Ada-encoded name.  This also
     makes Ada's verbatim match syntax ("<...>") work, because
     "lookup_name.name()" includes the "<>"s, while
     "lookup_name.ada().lookup_name()" is the encoded name with "<>"s
     stripped.  */
  if (current_language->la_language == language_ada)
    return lookup_name.ada ().lookup_name ().c_str ();

  return lookup_name.c_str ();
}

/* See minsyms.h.  */

void
iterate_over_minimal_symbols
    (struct objfile *objf, const lookup_name_info &lookup_name,
     gdb::function_view<bool (struct minimal_symbol *)> callback)
{
  /* The first pass is over the ordinary hash table.  */
    {
      const char *name = linkage_name_str (lookup_name);
      unsigned int hash = msymbol_hash (name) % MINIMAL_SYMBOL_HASH_SIZE;
      auto *mangled_cmp
	= (case_sensitivity == case_sensitive_on
	   ? strcmp
	   : strcasecmp);

      for (minimal_symbol *iter = objf->per_bfd->msymbol_hash[hash];
	   iter != NULL;
	   iter = iter->hash_next)
	{
	  if (mangled_cmp (iter->linkage_name (), name) == 0)
	    if (callback (iter))
	      return;
	}
    }

  /* The second pass is over the demangled table.  Once for each
     language in the demangled hash names table (usually just zero or
     one).  */
  for (unsigned liter = 0; liter < nr_languages; ++liter)
    {
      if (!objf->per_bfd->demangled_hash_languages.test (liter))
	continue;

      enum language lang = (enum language) liter;
      const language_defn *lang_def = language_def (lang);
      symbol_name_matcher_ftype *name_match
	= lang_def->get_symbol_name_matcher (lookup_name);

      unsigned int hash
	= lookup_name.search_name_hash (lang) % MINIMAL_SYMBOL_HASH_SIZE;
      for (minimal_symbol *iter = objf->per_bfd->msymbol_demangled_hash[hash];
	   iter != NULL;
	   iter = iter->demangled_hash_next)
	if (name_match (iter->search_name (), lookup_name, NULL))
	  if (callback (iter))
	    return;
    }
}

/* See minsyms.h.  */

bound_minimal_symbol
lookup_minimal_symbol_linkage (const char *name, struct objfile *objf)
{
  unsigned int hash = msymbol_hash (name) % MINIMAL_SYMBOL_HASH_SIZE;

  for (objfile *objfile : objf->separate_debug_objfiles ())
    {
      for (minimal_symbol *msymbol = objfile->per_bfd->msymbol_hash[hash];
	   msymbol != NULL;
	   msymbol = msymbol->hash_next)
	{
	  if (strcmp (msymbol->linkage_name (), name) == 0
	      && (msymbol->type () == mst_data
		  || msymbol->type () == mst_bss))
	    return {msymbol, objfile};
	}
    }

  return {};
}

/* See minsyms.h.  */

struct bound_minimal_symbol
lookup_minimal_symbol_linkage (const char *name, bool only_main)
{
  for (objfile *objfile : current_program_space->objfiles ())
    {
      if (objfile->separate_debug_objfile_backlink != nullptr)
	continue;

      if (only_main && (objfile->flags & OBJF_MAINLINE) == 0)
	continue;

      bound_minimal_symbol minsym = lookup_minimal_symbol_linkage (name,
								   objfile);
      if (minsym.minsym != nullptr)
	return minsym;
    }

  return {};
}

/* See minsyms.h.  */

struct bound_minimal_symbol
lookup_minimal_symbol_text (const char *name, struct objfile *objf)
{
  struct minimal_symbol *msymbol;
  struct bound_minimal_symbol found_symbol;
  struct bound_minimal_symbol found_file_symbol;

  unsigned int hash = msymbol_hash (name) % MINIMAL_SYMBOL_HASH_SIZE;

  auto search = [&] (struct objfile *objfile)
  {
    for (msymbol = objfile->per_bfd->msymbol_hash[hash];
	 msymbol != NULL && found_symbol.minsym == NULL;
	 msymbol = msymbol->hash_next)
      {
	if (strcmp (msymbol->linkage_name (), name) == 0 &&
	    (msymbol->type () == mst_text
	     || msymbol->type () == mst_text_gnu_ifunc
	     || msymbol->type () == mst_file_text))
	  {
	    switch (msymbol->type ())
	      {
	      case mst_file_text:
		found_file_symbol.minsym = msymbol;
		found_file_symbol.objfile = objfile;
		break;
	      default:
		found_symbol.minsym = msymbol;
		found_symbol.objfile = objfile;
		break;
	      }
	  }
      }
  };

  if (objf == nullptr)
    {
      for (objfile *objfile : current_program_space->objfiles ())
	{
	  if (found_symbol.minsym != NULL)
	    break;
	  search (objfile);
	}
    }
  else
    {
      for (objfile *objfile : objf->separate_debug_objfiles ())
	{
	  if (found_symbol.minsym != NULL)
	    break;
	  search (objfile);
	}
    }

  /* External symbols are best.  */
  if (found_symbol.minsym)
    return found_symbol;

  /* File-local symbols are next best.  */
  return found_file_symbol;
}

/* See minsyms.h.  */

struct minimal_symbol *
lookup_minimal_symbol_by_pc_name (CORE_ADDR pc, const char *name,
				  struct objfile *objf)
{
  struct minimal_symbol *msymbol;

  unsigned int hash = msymbol_hash (name) % MINIMAL_SYMBOL_HASH_SIZE;

  for (objfile *objfile : current_program_space->objfiles ())
    {
      if (objf == NULL || objf == objfile
	  || objf == objfile->separate_debug_objfile_backlink)
	{
	  for (msymbol = objfile->per_bfd->msymbol_hash[hash];
	       msymbol != NULL;
	       msymbol = msymbol->hash_next)
	    {
	      if (msymbol->value_address (objfile) == pc
		  && strcmp (msymbol->linkage_name (), name) == 0)
		return msymbol;
	    }
	}
    }

  return NULL;
}

/* A helper function that makes *PC section-relative.  This searches
   the sections of OBJFILE and if *PC is in a section, it subtracts
   the section offset, stores the result into UNREL_ADDR, and returns
   true.  Otherwise it returns false.  */

static int
frob_address (struct objfile *objfile, CORE_ADDR pc,
	      unrelocated_addr *unrel_addr)
{
  for (obj_section *iter : objfile->sections ())
    {
      if (pc >= iter->addr () && pc < iter->endaddr ())
	{
	  *unrel_addr = unrelocated_addr (pc - iter->offset ());
	  return 1;
	}
    }

  return 0;
}

/* Helper for lookup_minimal_symbol_by_pc_section.  Convert a
   lookup_msym_prefer to a minimal_symbol_type.  */

static minimal_symbol_type
msym_prefer_to_msym_type (lookup_msym_prefer prefer)
{
  switch (prefer)
    {
    case lookup_msym_prefer::TEXT:
      return mst_text;
    case lookup_msym_prefer::TRAMPOLINE:
      return mst_solib_trampoline;
    case lookup_msym_prefer::GNU_IFUNC:
      return mst_text_gnu_ifunc;
    }

  /* Assert here instead of in a default switch case above so that
     -Wswitch warns if a new enumerator is added.  */
  gdb_assert_not_reached ("unhandled lookup_msym_prefer");
}

/* See minsyms.h.

   Note that we need to look through ALL the minimal symbol tables
   before deciding on the symbol that comes closest to the specified PC.
   This is because objfiles can overlap, for example objfile A has .text
   at 0x100 and .data at 0x40000 and objfile B has .text at 0x234 and
   .data at 0x40048.  */

bound_minimal_symbol
lookup_minimal_symbol_by_pc_section (CORE_ADDR pc_in, struct obj_section *section,
				     lookup_msym_prefer prefer,
				     bound_minimal_symbol *previous)
{
  int lo;
  int hi;
  int newobj;
  struct minimal_symbol *msymbol;
  struct minimal_symbol *best_symbol = NULL;
  struct objfile *best_objfile = NULL;
  struct bound_minimal_symbol result;

  if (previous != nullptr)
    {
      previous->minsym = nullptr;
      previous->objfile = nullptr;
    }

  if (section == NULL)
    {
      section = find_pc_section (pc_in);
      if (section == NULL)
	return {};
    }

  minimal_symbol_type want_type = msym_prefer_to_msym_type (prefer);

  /* We can not require the symbol found to be in section, because
     e.g. IRIX 6.5 mdebug relies on this code returning an absolute
     symbol - but find_pc_section won't return an absolute section and
     hence the code below would skip over absolute symbols.  We can
     still take advantage of the call to find_pc_section, though - the
     object file still must match.  In case we have separate debug
     files, search both the file and its separate debug file.  There's
     no telling which one will have the minimal symbols.  */

  gdb_assert (section != NULL);

  for (objfile *objfile : section->objfile->separate_debug_objfiles ())
    {
      CORE_ADDR pc = pc_in;

      /* If this objfile has a minimal symbol table, go search it
	 using a binary search.  */

      if (objfile->per_bfd->minimal_symbol_count > 0)
	{
	  int best_zero_sized = -1;

	  msymbol = objfile->per_bfd->msymbols.get ();
	  lo = 0;
	  hi = objfile->per_bfd->minimal_symbol_count - 1;

	  /* This code assumes that the minimal symbols are sorted by
	     ascending address values.  If the pc value is greater than or
	     equal to the first symbol's address, then some symbol in this
	     minimal symbol table is a suitable candidate for being the
	     "best" symbol.  This includes the last real symbol, for cases
	     where the pc value is larger than any address in this vector.

	     By iterating until the address associated with the current
	     hi index (the endpoint of the test interval) is less than
	     or equal to the desired pc value, we accomplish two things:
	     (1) the case where the pc value is larger than any minimal
	     symbol address is trivially solved, (2) the address associated
	     with the hi index is always the one we want when the iteration
	     terminates.  In essence, we are iterating the test interval
	     down until the pc value is pushed out of it from the high end.

	     Warning: this code is trickier than it would appear at first.  */

	  unrelocated_addr unrel_pc;
	  if (frob_address (objfile, pc, &unrel_pc)
	      && unrel_pc >= msymbol[lo].unrelocated_address ())
	    {
	      while (msymbol[hi].unrelocated_address () > unrel_pc)
		{
		  /* pc is still strictly less than highest address.  */
		  /* Note "new" will always be >= lo.  */
		  newobj = (lo + hi) / 2;
		  if ((msymbol[newobj].unrelocated_address () >= unrel_pc)
		      || (lo == newobj))
		    {
		      hi = newobj;
		    }
		  else
		    {
		      lo = newobj;
		    }
		}

	      /* If we have multiple symbols at the same address, we want
		 hi to point to the last one.  That way we can find the
		 right symbol if it has an index greater than hi.  */
	      while (hi < objfile->per_bfd->minimal_symbol_count - 1
		     && (msymbol[hi].unrelocated_address ()
			 == msymbol[hi + 1].unrelocated_address ()))
		hi++;

	      /* Skip various undesirable symbols.  */
	      while (hi >= 0)
		{
		  /* Skip any absolute symbols.  This is apparently
		     what adb and dbx do, and is needed for the CM-5.
		     There are two known possible problems: (1) on
		     ELF, apparently end, edata, etc. are absolute.
		     Not sure ignoring them here is a big deal, but if
		     we want to use them, the fix would go in
		     elfread.c.  (2) I think shared library entry
		     points on the NeXT are absolute.  If we want
		     special handling for this it probably should be
		     triggered by a special mst_abs_or_lib or some
		     such.  */

		  if (msymbol[hi].type () == mst_abs)
		    {
		      hi--;
		      continue;
		    }

		  /* If SECTION was specified, skip any symbol from
		     wrong section.  */
		  if (section
		      /* Some types of debug info, such as COFF,
			 don't fill the bfd_section member, so don't
			 throw away symbols on those platforms.  */
		      && msymbol[hi].obj_section (objfile) != nullptr
		      && (!matching_obj_sections
			  (msymbol[hi].obj_section (objfile),
			   section)))
		    {
		      hi--;
		      continue;
		    }

		  /* If we are looking for a trampoline and this is a
		     text symbol, or the other way around, check the
		     preceding symbol too.  If they are otherwise
		     identical prefer that one.  */
		  if (hi > 0
		      && msymbol[hi].type () != want_type
		      && msymbol[hi - 1].type () == want_type
		      && (msymbol[hi].size () == msymbol[hi - 1].size ())
		      && (msymbol[hi].unrelocated_address ()
			  == msymbol[hi - 1].unrelocated_address ())
		      && (msymbol[hi].obj_section (objfile)
			  == msymbol[hi - 1].obj_section (objfile)))
		    {
		      hi--;
		      continue;
		    }

		  /* If the minimal symbol has a zero size, save it
		     but keep scanning backwards looking for one with
		     a non-zero size.  A zero size may mean that the
		     symbol isn't an object or function (e.g. a
		     label), or it may just mean that the size was not
		     specified.  */
		  if (msymbol[hi].size () == 0)
		    {
		      if (best_zero_sized == -1)
			best_zero_sized = hi;
		      hi--;
		      continue;
		    }

		  /* If we are past the end of the current symbol, try
		     the previous symbol if it has a larger overlapping
		     size.  This happens on i686-pc-linux-gnu with glibc;
		     the nocancel variants of system calls are inside
		     the cancellable variants, but both have sizes.  */
		  if (hi > 0
		      && msymbol[hi].size () != 0
		      && unrel_pc >= msymbol[hi].unrelocated_end_address ()
		      && unrel_pc < msymbol[hi - 1].unrelocated_end_address ())
		    {
		      hi--;
		      continue;
		    }

		  /* Otherwise, this symbol must be as good as we're going
		     to get.  */
		  break;
		}

	      /* If HI has a zero size, and best_zero_sized is set,
		 then we had two or more zero-sized symbols; prefer
		 the first one we found (which may have a higher
		 address).  Also, if we ran off the end, be sure
		 to back up.  */
	      if (best_zero_sized != -1
		  && (hi < 0 || msymbol[hi].size () == 0))
		hi = best_zero_sized;

	      /* If the minimal symbol has a non-zero size, and this
		 PC appears to be outside the symbol's contents, then
		 refuse to use this symbol.  If we found a zero-sized
		 symbol with an address greater than this symbol's,
		 use that instead.  We assume that if symbols have
		 specified sizes, they do not overlap.  */

	      if (hi >= 0
		  && msymbol[hi].size () != 0
		  && unrel_pc >= msymbol[hi].unrelocated_end_address ())
		{
		  if (best_zero_sized != -1)
		    hi = best_zero_sized;
		  else
		    {
		      /* If needed record this symbol as the closest
			 previous symbol.  */
		      if (previous != nullptr)
			{
			  if (previous->minsym == nullptr
			      || (msymbol[hi].unrelocated_address ()
				  > previous->minsym->unrelocated_address ()))
			    {
			      previous->minsym = &msymbol[hi];
			      previous->objfile = objfile;
			    }
			}
		      /* Go on to the next object file.  */
		      continue;
		    }
		}

	      /* The minimal symbol indexed by hi now is the best one in this
		 objfile's minimal symbol table.  See if it is the best one
		 overall.  */

	      if (hi >= 0
		  && ((best_symbol == NULL) ||
		      (best_symbol->unrelocated_address () <
		       msymbol[hi].unrelocated_address ())))
		{
		  best_symbol = &msymbol[hi];
		  best_objfile = objfile;
		}
	    }
	}
    }

  result.minsym = best_symbol;
  result.objfile = best_objfile;
  return result;
}

/* See minsyms.h.  */

struct bound_minimal_symbol
lookup_minimal_symbol_by_pc (CORE_ADDR pc)
{
  return lookup_minimal_symbol_by_pc_section (pc, NULL);
}

/* Return non-zero iff PC is in an STT_GNU_IFUNC function resolver.  */

bool
in_gnu_ifunc_stub (CORE_ADDR pc)
{
  bound_minimal_symbol msymbol
    = lookup_minimal_symbol_by_pc_section (pc, NULL,
					   lookup_msym_prefer::GNU_IFUNC);
  return msymbol.minsym && msymbol.minsym->type () == mst_text_gnu_ifunc;
}

/* See elf_gnu_ifunc_resolve_addr for its real implementation.  */

static CORE_ADDR
stub_gnu_ifunc_resolve_addr (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  error (_("GDB cannot resolve STT_GNU_IFUNC symbol at address %s without "
	   "the ELF support compiled in."),
	 paddress (gdbarch, pc));
}

/* See elf_gnu_ifunc_resolve_name for its real implementation.  */

static bool
stub_gnu_ifunc_resolve_name (const char *function_name,
			     CORE_ADDR *function_address_p)
{
  error (_("GDB cannot resolve STT_GNU_IFUNC symbol \"%s\" without "
	   "the ELF support compiled in."),
	 function_name);
}

/* See elf_gnu_ifunc_resolver_stop for its real implementation.  */

static void
stub_gnu_ifunc_resolver_stop (code_breakpoint *b)
{
  internal_error (_("elf_gnu_ifunc_resolver_stop cannot be reached."));
}

/* See elf_gnu_ifunc_resolver_return_stop for its real implementation.  */

static void
stub_gnu_ifunc_resolver_return_stop (code_breakpoint *b)
{
  internal_error (_("elf_gnu_ifunc_resolver_return_stop cannot be reached."));
}

/* See elf_gnu_ifunc_fns for its real implementation.  */

static const struct gnu_ifunc_fns stub_gnu_ifunc_fns =
{
  stub_gnu_ifunc_resolve_addr,
  stub_gnu_ifunc_resolve_name,
  stub_gnu_ifunc_resolver_stop,
  stub_gnu_ifunc_resolver_return_stop,
};

/* A placeholder for &elf_gnu_ifunc_fns.  */

const struct gnu_ifunc_fns *gnu_ifunc_fns_p = &stub_gnu_ifunc_fns;



/* Return leading symbol character for a BFD.  If BFD is NULL,
   return the leading symbol character from the main objfile.  */

static int
get_symbol_leading_char (bfd *abfd)
{
  if (abfd != NULL)
    return bfd_get_symbol_leading_char (abfd);
  if (current_program_space->symfile_object_file != NULL)
    {
      objfile *objf = current_program_space->symfile_object_file;
      if (objf->obfd != NULL)
	return bfd_get_symbol_leading_char (objf->obfd.get ());
    }
  return 0;
}

/* See minsyms.h.  */

minimal_symbol_reader::minimal_symbol_reader (struct objfile *obj)
: m_objfile (obj),
  m_msym_bunch (NULL),
  /* Note that presetting m_msym_bunch_index to BUNCH_SIZE causes the
     first call to save a minimal symbol to allocate the memory for
     the first bunch.  */
  m_msym_bunch_index (BUNCH_SIZE),
  m_msym_count (0)
{
}

/* Discard the currently collected minimal symbols, if any.  If we wish
   to save them for later use, we must have already copied them somewhere
   else before calling this function.  */

minimal_symbol_reader::~minimal_symbol_reader ()
{
  struct msym_bunch *next;

  while (m_msym_bunch != NULL)
    {
      next = m_msym_bunch->next;
      xfree (m_msym_bunch);
      m_msym_bunch = next;
    }
}

/* See minsyms.h.  */

void
minimal_symbol_reader::record (const char *name, unrelocated_addr address,
			       enum minimal_symbol_type ms_type)
{
  int section;

  switch (ms_type)
    {
    case mst_text:
    case mst_text_gnu_ifunc:
    case mst_file_text:
    case mst_solib_trampoline:
      section = SECT_OFF_TEXT (m_objfile);
      break;
    case mst_data:
    case mst_data_gnu_ifunc:
    case mst_file_data:
      section = SECT_OFF_DATA (m_objfile);
      break;
    case mst_bss:
    case mst_file_bss:
      section = SECT_OFF_BSS (m_objfile);
      break;
    default:
      section = -1;
    }

  record_with_info (name, address, ms_type, section);
}

/* Convert an enumerator of type minimal_symbol_type to its string
   representation.  */

static const char *
mst_str (minimal_symbol_type t)
{
#define MST_TO_STR(x) case x: return #x;
  switch (t)
  {
    MST_TO_STR (mst_unknown);
    MST_TO_STR (mst_text);
    MST_TO_STR (mst_text_gnu_ifunc);
    MST_TO_STR (mst_slot_got_plt);
    MST_TO_STR (mst_data);
    MST_TO_STR (mst_bss);
    MST_TO_STR (mst_abs);
    MST_TO_STR (mst_solib_trampoline);
    MST_TO_STR (mst_file_text);
    MST_TO_STR (mst_file_data);
    MST_TO_STR (mst_file_bss);

    default:
      return "mst_???";
  }
#undef MST_TO_STR
}

/* See minsyms.h.  */

struct minimal_symbol *
minimal_symbol_reader::record_full (std::string_view name,
				    bool copy_name, unrelocated_addr address,
				    enum minimal_symbol_type ms_type,
				    int section)
{
  struct msym_bunch *newobj;
  struct minimal_symbol *msymbol;

  /* Don't put gcc_compiled, __gnu_compiled_cplus, and friends into
     the minimal symbols, because if there is also another symbol
     at the same address (e.g. the first function of the file),
     lookup_minimal_symbol_by_pc would have no way of getting the
     right one.  */
  if (ms_type == mst_file_text && name[0] == 'g'
      && (name == GCC_COMPILED_FLAG_SYMBOL
	  || name == GCC2_COMPILED_FLAG_SYMBOL))
    return (NULL);

  /* It's safe to strip the leading char here once, since the name
     is also stored stripped in the minimal symbol table.  */
  if (name[0] == get_symbol_leading_char (m_objfile->obfd.get ()))
    name = name.substr (1);

  if (ms_type == mst_file_text && startswith (name, "__gnu_compiled"))
    return (NULL);

  symtab_create_debug_printf_v ("recording minsym:  %-21s  %18s  %4d  %.*s",
				mst_str (ms_type),
				hex_string (LONGEST (address)),
				section, (int) name.size (), name.data ());

  if (m_msym_bunch_index == BUNCH_SIZE)
    {
      newobj = XCNEW (struct msym_bunch);
      m_msym_bunch_index = 0;
      newobj->next = m_msym_bunch;
      m_msym_bunch = newobj;
    }
  msymbol = &m_msym_bunch->contents[m_msym_bunch_index];
  msymbol->set_language (language_unknown,
			 &m_objfile->per_bfd->storage_obstack);

  if (copy_name)
    msymbol->m_name = obstack_strndup (&m_objfile->per_bfd->storage_obstack,
				       name.data (), name.size ());
  else
    msymbol->m_name = name.data ();

  msymbol->set_unrelocated_address (address);
  msymbol->set_section_index (section);

  msymbol->set_type (ms_type);

  /* If we already read minimal symbols for this objfile, then don't
     ever allocate a new one.  */
  if (!m_objfile->per_bfd->minsyms_read)
    {
      m_msym_bunch_index++;
      m_objfile->per_bfd->n_minsyms++;
    }
  m_msym_count++;
  return msymbol;
}

/* Compare two minimal symbols by address and return true if FN1's address
   is less than FN2's, so that we sort into unsigned numeric order.
   Within groups with the same address, sort by name.  */

static inline bool
minimal_symbol_is_less_than (const minimal_symbol &fn1,
			     const minimal_symbol &fn2)
{
  if ((&fn1)->unrelocated_address () < (&fn2)->unrelocated_address ())
    {
      return true;		/* addr 1 is less than addr 2.  */
    }
  else if ((&fn1)->unrelocated_address () > (&fn2)->unrelocated_address ())
    {
      return false;		/* addr 1 is greater than addr 2.  */
    }
  else
    /* addrs are equal: sort by name */
    {
      const char *name1 = fn1.linkage_name ();
      const char *name2 = fn2.linkage_name ();

      if (name1 && name2)	/* both have names */
	return strcmp (name1, name2) < 0;
      else if (name2)
	return true;		/* fn1 has no name, so it is "less".  */
      else if (name1)		/* fn2 has no name, so it is "less".  */
	return false;
      else
	return false;		/* Neither has a name, so they're equal.  */
    }
}

/* Compact duplicate entries out of a minimal symbol table by walking
   through the table and compacting out entries with duplicate addresses
   and matching names.  Return the number of entries remaining.

   On entry, the table resides between msymbol[0] and msymbol[mcount].
   On exit, it resides between msymbol[0] and msymbol[result_count].

   When files contain multiple sources of symbol information, it is
   possible for the minimal symbol table to contain many duplicate entries.
   As an example, SVR4 systems use ELF formatted object files, which
   usually contain at least two different types of symbol tables (a
   standard ELF one and a smaller dynamic linking table), as well as
   DWARF debugging information for files compiled with -g.

   Without compacting, the minimal symbol table for gdb itself contains
   over a 1000 duplicates, about a third of the total table size.  Aside
   from the potential trap of not noticing that two successive entries
   identify the same location, this duplication impacts the time required
   to linearly scan the table, which is done in a number of places.  So we
   just do one linear scan here and toss out the duplicates.

   Since the different sources of information for each symbol may
   have different levels of "completeness", we may have duplicates
   that have one entry with type "mst_unknown" and the other with a
   known type.  So if the one we are leaving alone has type mst_unknown,
   overwrite its type with the type from the one we are compacting out.  */

static int
compact_minimal_symbols (struct minimal_symbol *msymbol, int mcount,
			 struct objfile *objfile)
{
  struct minimal_symbol *copyfrom;
  struct minimal_symbol *copyto;

  if (mcount > 0)
    {
      copyfrom = copyto = msymbol;
      while (copyfrom < msymbol + mcount - 1)
	{
	  if (copyfrom->unrelocated_address ()
	      == (copyfrom + 1)->unrelocated_address ()
	      && (copyfrom->section_index ()
		  == (copyfrom + 1)->section_index ())
	      && strcmp (copyfrom->linkage_name (),
			 (copyfrom + 1)->linkage_name ()) == 0)
	    {
	      if ((copyfrom + 1)->type () == mst_unknown)
		(copyfrom + 1)->set_type (copyfrom->type ());

	      copyfrom++;
	    }
	  else
	    *copyto++ = *copyfrom++;
	}
      *copyto++ = *copyfrom++;
      mcount = copyto - msymbol;
    }
  return (mcount);
}

static void
clear_minimal_symbol_hash_tables (struct objfile *objfile)
{
  for (size_t i = 0; i < MINIMAL_SYMBOL_HASH_SIZE; i++)
    {
      objfile->per_bfd->msymbol_hash[i] = 0;
      objfile->per_bfd->msymbol_demangled_hash[i] = 0;
    }
}

/* This struct is used to store values we compute for msymbols on the
   background threads but don't need to keep around long term.  */
struct computed_hash_values
{
  /* Length of the linkage_name of the symbol.  */
  size_t name_length;
  /* Hash code (using fast_hash) of the linkage_name.  */
  hashval_t mangled_name_hash;
  /* The msymbol_hash of the linkage_name.  */
  unsigned int minsym_hash;
  /* The msymbol_hash of the search_name.  */
  unsigned int minsym_demangled_hash;
};

/* Build (or rebuild) the minimal symbol hash tables.  This is necessary
   after compacting or sorting the table since the entries move around
   thus causing the internal minimal_symbol pointers to become jumbled.  */
  
static void
build_minimal_symbol_hash_tables
  (struct objfile *objfile,
   const std::vector<computed_hash_values>& hash_values)
{
  int i;
  struct minimal_symbol *msym;

  /* (Re)insert the actual entries.  */
  int mcount = objfile->per_bfd->minimal_symbol_count;
  for ((i = 0,
	msym = objfile->per_bfd->msymbols.get ());
       i < mcount;
       i++, msym++)
    {
      msym->hash_next = 0;
      add_minsym_to_hash_table (msym, objfile->per_bfd->msymbol_hash,
				hash_values[i].minsym_hash);

      msym->demangled_hash_next = 0;
      if (msym->search_name () != msym->linkage_name ())
	add_minsym_to_demangled_hash_table
	  (msym, objfile, hash_values[i].minsym_demangled_hash);
    }
}

/* Add the minimal symbols in the existing bunches to the objfile's official
   minimal symbol table.  In most cases there is no minimal symbol table yet
   for this objfile, and the existing bunches are used to create one.  Once
   in a while (for shared libraries for example), we add symbols (e.g. common
   symbols) to an existing objfile.  */

void
minimal_symbol_reader::install ()
{
  int mcount;
  struct msym_bunch *bunch;
  struct minimal_symbol *msymbols;
  int alloc_count;

  if (m_objfile->per_bfd->minsyms_read)
    return;

  if (m_msym_count > 0)
    {
      symtab_create_debug_printf ("installing %d minimal symbols of objfile %s",
				  m_msym_count, objfile_name (m_objfile));

      /* Allocate enough space, into which we will gather the bunches
	 of new and existing minimal symbols, sort them, and then
	 compact out the duplicate entries.  Once we have a final
	 table, we will give back the excess space.  */

      alloc_count = m_msym_count + m_objfile->per_bfd->minimal_symbol_count;
      gdb::unique_xmalloc_ptr<minimal_symbol>
	msym_holder (XNEWVEC (minimal_symbol, alloc_count));
      msymbols = msym_holder.get ();

      /* Copy in the existing minimal symbols, if there are any.  */

      if (m_objfile->per_bfd->minimal_symbol_count)
	memcpy (msymbols, m_objfile->per_bfd->msymbols.get (),
		m_objfile->per_bfd->minimal_symbol_count
		* sizeof (struct minimal_symbol));

      /* Walk through the list of minimal symbol bunches, adding each symbol
	 to the new contiguous array of symbols.  Note that we start with the
	 current, possibly partially filled bunch (thus we use the current
	 msym_bunch_index for the first bunch we copy over), and thereafter
	 each bunch is full.  */

      mcount = m_objfile->per_bfd->minimal_symbol_count;

      for (bunch = m_msym_bunch; bunch != NULL; bunch = bunch->next)
	{
	  memcpy (&msymbols[mcount], &bunch->contents[0],
		  m_msym_bunch_index * sizeof (struct minimal_symbol));
	  mcount += m_msym_bunch_index;
	  m_msym_bunch_index = BUNCH_SIZE;
	}

      /* Sort the minimal symbols by address.  */

      std::sort (msymbols, msymbols + mcount, minimal_symbol_is_less_than);

      /* Compact out any duplicates, and free up whatever space we are
	 no longer using.  */

      mcount = compact_minimal_symbols (msymbols, mcount, m_objfile);
      msym_holder.reset (XRESIZEVEC (struct minimal_symbol,
				     msym_holder.release (),
				     mcount));

      /* Attach the minimal symbol table to the specified objfile.
	 The strings themselves are also located in the storage_obstack
	 of this objfile.  */

      if (m_objfile->per_bfd->minimal_symbol_count != 0)
	clear_minimal_symbol_hash_tables (m_objfile);

      m_objfile->per_bfd->minimal_symbol_count = mcount;
      m_objfile->per_bfd->msymbols = std::move (msym_holder);

#if CXX_STD_THREAD
      /* Mutex that is used when modifying or accessing the demangled
	 hash table.  */
      std::mutex demangled_mutex;
#endif

      std::vector<computed_hash_values> hash_values (mcount);

      msymbols = m_objfile->per_bfd->msymbols.get ();
      /* Arbitrarily require at least 10 elements in a thread.  */
      gdb::parallel_for_each (10, &msymbols[0], &msymbols[mcount],
	 [&] (minimal_symbol *start, minimal_symbol *end)
	 {
	   for (minimal_symbol *msym = start; msym < end; ++msym)
	     {
	       size_t idx = msym - msymbols;
	       hash_values[idx].name_length = strlen (msym->linkage_name ());
	       if (!msym->name_set)
		 {
		   /* This will be freed later, by compute_and_set_names.  */
		   gdb::unique_xmalloc_ptr<char> demangled_name
		     = symbol_find_demangled_name (msym, msym->linkage_name ());
		   msym->set_demangled_name
		     (demangled_name.release (),
		      &m_objfile->per_bfd->storage_obstack);
		   msym->name_set = 1;
		 }
	       /* This mangled_name_hash computation has to be outside of
		  the name_set check, or compute_and_set_names below will
		  be called with an invalid hash value.  */
	       hash_values[idx].mangled_name_hash
		 = fast_hash (msym->linkage_name (),
			      hash_values[idx].name_length);
	       hash_values[idx].minsym_hash
		 = msymbol_hash (msym->linkage_name ());
	       /* We only use this hash code if the search name differs
		  from the linkage name.  See the code in
		  build_minimal_symbol_hash_tables.  */
	       if (msym->search_name () != msym->linkage_name ())
		 hash_values[idx].minsym_demangled_hash
		   = search_name_hash (msym->language (), msym->search_name ());
	     }
	   {
	     /* To limit how long we hold the lock, we only acquire it here
		and not while we demangle the names above.  */
#if CXX_STD_THREAD
	     std::lock_guard<std::mutex> guard (demangled_mutex);
#endif
	     for (minimal_symbol *msym = start; msym < end; ++msym)
	       {
		 size_t idx = msym - msymbols;
		 msym->compute_and_set_names
		   (std::string_view (msym->linkage_name (),
				      hash_values[idx].name_length),
		    false,
		    m_objfile->per_bfd,
		    hash_values[idx].mangled_name_hash);
	       }
	   }
	 });

      build_minimal_symbol_hash_tables (m_objfile, hash_values);
    }
}

/* Check if PC is in a shared library trampoline code stub.
   Return minimal symbol for the trampoline entry or NULL if PC is not
   in a trampoline code stub.  */

static struct minimal_symbol *
lookup_solib_trampoline_symbol_by_pc (CORE_ADDR pc)
{
  bound_minimal_symbol msymbol
    = lookup_minimal_symbol_by_pc_section (pc, NULL,
					   lookup_msym_prefer::TRAMPOLINE);

  if (msymbol.minsym != NULL
      && msymbol.minsym->type () == mst_solib_trampoline)
    return msymbol.minsym;
  return NULL;
}

/* If PC is in a shared library trampoline code stub, return the
   address of the `real' function belonging to the stub.
   Return 0 if PC is not in a trampoline code stub or if the real
   function is not found in the minimal symbol table.

   We may fail to find the right function if a function with the
   same name is defined in more than one shared library, but this
   is considered bad programming style.  We could return 0 if we find
   a duplicate function in case this matters someday.  */

CORE_ADDR
find_solib_trampoline_target (frame_info_ptr frame, CORE_ADDR pc)
{
  struct minimal_symbol *tsymbol = lookup_solib_trampoline_symbol_by_pc (pc);

  if (tsymbol != NULL)
    {
      for (objfile *objfile : current_program_space->objfiles ())
	{
	  for (minimal_symbol *msymbol : objfile->msymbols ())
	    {
	      /* Also handle minimal symbols pointing to function
		 descriptors.  */
	      if ((msymbol->type () == mst_text
		   || msymbol->type () == mst_text_gnu_ifunc
		   || msymbol->type () == mst_data
		   || msymbol->type () == mst_data_gnu_ifunc)
		  && strcmp (msymbol->linkage_name (),
			     tsymbol->linkage_name ()) == 0)
		{
		  CORE_ADDR func;

		  /* Ignore data symbols that are not function
		     descriptors.  */
		  if (msymbol_is_function (objfile, msymbol, &func))
		    return func;
		}
	    }
	}
    }
  return 0;
}

/* See minsyms.h.  */

CORE_ADDR
minimal_symbol_upper_bound (struct bound_minimal_symbol minsym)
{
  short section;
  struct obj_section *obj_section;
  CORE_ADDR result;
  struct minimal_symbol *iter, *msymbol;

  gdb_assert (minsym.minsym != NULL);

  /* If the minimal symbol has a size, use it.  Otherwise use the
     lesser of the next minimal symbol in the same section, or the end
     of the section, as the end of the function.  */

  if (minsym.minsym->size () != 0)
    return minsym.value_address () + minsym.minsym->size ();

  /* Step over other symbols at this same address, and symbols in
     other sections, to find the next symbol in this section with a
     different address.  */

  struct minimal_symbol *past_the_end
    = (minsym.objfile->per_bfd->msymbols.get ()
       + minsym.objfile->per_bfd->minimal_symbol_count);
  msymbol = minsym.minsym;
  section = msymbol->section_index ();
  for (iter = msymbol + 1; iter != past_the_end; ++iter)
    {
      if ((iter->unrelocated_address ()
	   != msymbol->unrelocated_address ())
	  && iter->section_index () == section)
	break;
    }

  obj_section = minsym.obj_section ();
  if (iter != past_the_end
      && (iter->value_address (minsym.objfile)
	  < obj_section->endaddr ()))
    result = iter->value_address (minsym.objfile);
  else
    /* We got the start address from the last msymbol in the objfile.
       So the end address is the end of the section.  */
    result = obj_section->endaddr ();

  return result;
}

/* See minsyms.h.  */

type *
find_minsym_type_and_address (minimal_symbol *msymbol,
			      struct objfile *objfile,
			      CORE_ADDR *address_p)
{
  bound_minimal_symbol bound_msym = {msymbol, objfile};
  struct obj_section *section = msymbol->obj_section (objfile);
  enum minimal_symbol_type type = msymbol->type ();

  bool is_tls = (section != NULL
		 && section->the_bfd_section->flags & SEC_THREAD_LOCAL);

  /* The minimal symbol might point to a function descriptor;
     resolve it to the actual code address instead.  */
  CORE_ADDR addr;
  if (is_tls)
    {
      /* Addresses of TLS symbols are really offsets into a
	 per-objfile/per-thread storage block.  */
      addr = CORE_ADDR (bound_msym.minsym->unrelocated_address ());
    }
  else if (msymbol_is_function (objfile, msymbol, &addr))
    {
      if (addr != bound_msym.value_address ())
	{
	  /* This means we resolved a function descriptor, and we now
	     have an address for a code/text symbol instead of a data
	     symbol.  */
	  if (msymbol->type () == mst_data_gnu_ifunc)
	    type = mst_text_gnu_ifunc;
	  else
	    type = mst_text;
	  section = NULL;
	}
    }
  else
    addr = bound_msym.value_address ();

  if (overlay_debugging)
    addr = symbol_overlayed_address (addr, section);

  if (is_tls)
    {
      /* Skip translation if caller does not need the address.  */
      if (address_p != NULL)
	*address_p = target_translate_tls_address (objfile, addr);
      return builtin_type (objfile)->nodebug_tls_symbol;
    }

  if (address_p != NULL)
    *address_p = addr;

  switch (type)
    {
    case mst_text:
    case mst_file_text:
    case mst_solib_trampoline:
      return builtin_type (objfile)->nodebug_text_symbol;

    case mst_text_gnu_ifunc:
      return builtin_type (objfile)->nodebug_text_gnu_ifunc_symbol;

    case mst_data:
    case mst_file_data:
    case mst_bss:
    case mst_file_bss:
      return builtin_type (objfile)->nodebug_data_symbol;

    case mst_slot_got_plt:
      return builtin_type (objfile)->nodebug_got_plt_symbol;

    default:
      return builtin_type (objfile)->nodebug_unknown_symbol;
    }
}
