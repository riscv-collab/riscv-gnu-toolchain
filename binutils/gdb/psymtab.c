/* Partial symbol tables.

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

#include "defs.h"
#include "symtab.h"
#include "objfiles.h"
#include "psymtab.h"
#include "block.h"
#include "filenames.h"
#include "source.h"
#include "addrmap.h"
#include "gdbtypes.h"
#include "ui-out.h"
#include "command.h"
#include "readline/tilde.h"
#include "gdbsupport/gdb_regex.h"
#include "dictionary.h"
#include "language.h"
#include "cp-support.h"
#include "gdbcmd.h"
#include <algorithm>
#include <set>
#include "gdbsupport/buildargv.h"

static struct partial_symbol *lookup_partial_symbol (struct objfile *,
						     struct partial_symtab *,
						     const lookup_name_info &,
						     int,
						     domain_enum);

static const char *psymtab_to_fullname (struct partial_symtab *ps);

static struct partial_symbol *find_pc_sect_psymbol (struct objfile *,
						    struct partial_symtab *,
						    CORE_ADDR,
						    struct obj_section *);

static struct compunit_symtab *psymtab_to_symtab (struct objfile *objfile,
						  struct partial_symtab *pst);

psymtab_storage::~psymtab_storage ()
{
  partial_symtab *iter = psymtabs;
  while (iter != nullptr)
    {
      partial_symtab *next = iter->next;
      delete iter;
      iter = next;
    }
}

/* See psymtab.h.  */

void
psymtab_storage::install_psymtab (partial_symtab *pst)
{
  pst->next = psymtabs;
  psymtabs = pst;
}



/* See psymtab.h.  */

psymtab_storage::partial_symtab_range
psymbol_functions::partial_symbols (struct objfile *objfile)
{
  return m_partial_symtabs->range ();
}

/* Find which partial symtab contains PC and SECTION starting at psymtab PST.
   We may find a different psymtab than PST.  See FIND_PC_SECT_PSYMTAB.  */

static struct partial_symtab *
find_pc_sect_psymtab_closer (struct objfile *objfile,
			     CORE_ADDR pc, struct obj_section *section,
			     struct partial_symtab *pst,
			     struct bound_minimal_symbol msymbol)
{
  struct partial_symtab *tpst;
  struct partial_symtab *best_pst = pst;
  CORE_ADDR best_addr = pst->text_low (objfile);

  /* An objfile that has its functions reordered might have
     many partial symbol tables containing the PC, but
     we want the partial symbol table that contains the
     function containing the PC.  */
  if (section == nullptr)
    return pst;

  if (msymbol.minsym == NULL)
    return pst;

  /* The code range of partial symtabs sometimes overlap, so, in
     the loop below, we need to check all partial symtabs and
     find the one that fits better for the given PC address.  We
     select the partial symtab that contains a symbol whose
     address is closest to the PC address.  By closest we mean
     that find_pc_sect_symbol returns the symbol with address
     that is closest and still less than the given PC.  */
  for (tpst = pst; tpst != NULL; tpst = tpst->next)
    {
      if (pc >= tpst->text_low (objfile) && pc < tpst->text_high (objfile))
	{
	  struct partial_symbol *p;
	  CORE_ADDR this_addr;

	  /* NOTE: This assumes that every psymbol has a
	     corresponding msymbol, which is not necessarily
	     true; the debug info might be much richer than the
	     object's symbol table.  */
	  p = find_pc_sect_psymbol (objfile, tpst, pc, section);
	  if (p != NULL
	      && (p->address (objfile) == msymbol.value_address ()))
	    return tpst;

	  /* Also accept the textlow value of a psymtab as a
	     "symbol", to provide some support for partial
	     symbol tables with line information but no debug
	     symbols (e.g. those produced by an assembler).  */
	  if (p != NULL)
	    this_addr = p->address (objfile);
	  else
	    this_addr = tpst->text_low (objfile);

	  /* Check whether it is closer than our current
	     BEST_ADDR.  Since this symbol address is
	     necessarily lower or equal to PC, the symbol closer
	     to PC is the symbol which address is the highest.
	     This way we return the psymtab which contains such
	     best match symbol.  This can help in cases where the
	     symbol information/debuginfo is not complete, like
	     for instance on IRIX6 with gcc, where no debug info
	     is emitted for statics.  (See also the nodebug.exp
	     testcase.)  */
	  if (this_addr > best_addr)
	    {
	      best_addr = this_addr;
	      best_pst = tpst;
	    }
	}
    }
  return best_pst;
}

/* See psymtab.h.  */

struct partial_symtab *
psymbol_functions::find_pc_sect_psymtab (struct objfile *objfile,
					 CORE_ADDR pc,
					 struct obj_section *section,
					 struct bound_minimal_symbol msymbol)
{
  for (partial_symtab *pst : partial_symbols (objfile))
    if (pc >= pst->text_low (objfile) && pc < pst->text_high (objfile))
      {
	struct partial_symtab *best_pst;

	best_pst = find_pc_sect_psymtab_closer (objfile, pc, section, pst,
						msymbol);
	if (best_pst != NULL)
	  return best_pst;
      }

  return NULL;
}

/* Psymtab version of find_pc_sect_compunit_symtab.  See its definition in
   the definition of quick_symbol_functions in symfile.h.  */

struct compunit_symtab *
psymbol_functions::find_pc_sect_compunit_symtab
     (struct objfile *objfile,
      struct bound_minimal_symbol msymbol,
      CORE_ADDR pc,
      struct obj_section *section,
      int warn_if_readin)
{
  struct partial_symtab *ps = find_pc_sect_psymtab (objfile,
						    pc, section,
						    msymbol);
  if (ps != NULL)
    {
      if (warn_if_readin && ps->readin_p (objfile))
	/* Might want to error() here (in case symtab is corrupt and
	   will cause a core dump), but maybe we can successfully
	   continue, so let's not.  */
	warning (_("\
(Internal error: pc %s in read in psymtab, but not in symtab.)\n"),
		 paddress (objfile->arch (), pc));
      psymtab_to_symtab (objfile, ps);
      return ps->get_compunit_symtab (objfile);
    }
  return NULL;
}

/* Find which partial symbol within a psymtab matches PC and SECTION.
   Return NULL if none.  */

static struct partial_symbol *
find_pc_sect_psymbol (struct objfile *objfile,
		      struct partial_symtab *psymtab, CORE_ADDR pc,
		      struct obj_section *section)
{
  struct partial_symbol *best = NULL;
  CORE_ADDR best_pc;
  const CORE_ADDR textlow = psymtab->text_low (objfile);

  gdb_assert (psymtab != NULL);

  /* Cope with programs that start at address 0.  */
  best_pc = (textlow != 0) ? textlow - 1 : 0;

  /* Search the global symbols as well as the static symbols, so that
     find_pc_partial_function doesn't use a minimal symbol and thus
     cache a bad endaddr.  */
  for (partial_symbol *p : psymtab->global_psymbols)
    {
      if (p->domain == VAR_DOMAIN
	  && p->aclass == LOC_BLOCK
	  && pc >= p->address (objfile)
	  && (p->address (objfile) > best_pc
	      || (psymtab->text_low (objfile) == 0
		  && best_pc == 0 && p->address (objfile) == 0)))
	{
	  if (section != NULL)  /* Match on a specific section.  */
	    {
	      if (!matching_obj_sections (p->obj_section (objfile),
					  section))
		continue;
	    }
	  best_pc = p->address (objfile);
	  best = p;
	}
    }

  for (partial_symbol *p : psymtab->static_psymbols)
    {
      if (p->domain == VAR_DOMAIN
	  && p->aclass == LOC_BLOCK
	  && pc >= p->address (objfile)
	  && (p->address (objfile) > best_pc
	      || (psymtab->text_low (objfile) == 0
		  && best_pc == 0 && p->address (objfile) == 0)))
	{
	  if (section != NULL)  /* Match on a specific section.  */
	    {
	      if (!matching_obj_sections (p->obj_section (objfile),
					  section))
		continue;
	    }
	  best_pc = p->address (objfile);
	  best = p;
	}
    }

  return best;
}

/* Psymtab version of lookup_global_symbol_language.  See its definition in
   the definition of quick_symbol_functions in symfile.h.  */

enum language
psymbol_functions::lookup_global_symbol_language (struct objfile *objfile,
						  const char *name,
						  domain_enum domain,
						  bool *symbol_found_p)
{
  *symbol_found_p = false;
  if (objfile->sf == NULL)
    return language_unknown;

  lookup_name_info lookup_name (name, symbol_name_match_type::FULL);

  for (partial_symtab *ps : partial_symbols (objfile))
    {
      struct partial_symbol *psym;
      if (ps->readin_p (objfile))
	continue;

      psym = lookup_partial_symbol (objfile, ps, lookup_name, 1, domain);
      if (psym)
	{
	  *symbol_found_p = true;
	  return psym->ginfo.language ();
	}
    }

  return language_unknown;
}

/* Returns true if PSYM matches LOOKUP_NAME.  */

static bool
psymbol_name_matches (partial_symbol *psym,
		      const lookup_name_info &lookup_name)
{
  const language_defn *lang = language_def (psym->ginfo.language ());
  symbol_name_matcher_ftype *name_match
    = lang->get_symbol_name_matcher (lookup_name);
  return name_match (psym->ginfo.search_name (), lookup_name, NULL);
}

/* Look, in partial_symtab PST, for symbol whose natural name is
   LOOKUP_NAME.  Check the global symbols if GLOBAL, the static
   symbols if not.  */

static struct partial_symbol *
lookup_partial_symbol (struct objfile *objfile,
		       struct partial_symtab *pst,
		       const lookup_name_info &lookup_name,
		       int global, domain_enum domain)
{
  struct partial_symbol **start, **psym;
  struct partial_symbol **top, **real_top, **bottom, **center;
  int length = (global
		? pst->global_psymbols.size ()
		: pst->static_psymbols.size ());
  int do_linear_search = 1;

  if (length == 0)
    return NULL;

  start = (global ?
	   &pst->global_psymbols[0] :
	   &pst->static_psymbols[0]);

  if (global)			/* This means we can use a binary search.  */
    {
      do_linear_search = 0;

      /* Binary search.  This search is guaranteed to end with center
	 pointing at the earliest partial symbol whose name might be
	 correct.  At that point *all* partial symbols with an
	 appropriate name will be checked against the correct
	 domain.  */

      bottom = start;
      top = start + length - 1;
      real_top = top;
      while (top > bottom)
	{
	  center = bottom + (top - bottom) / 2;

	  gdb_assert (center < top);

	  if (strcmp_iw_ordered ((*center)->ginfo.search_name (),
				 lookup_name.c_str ()) >= 0)
	    {
	      top = center;
	    }
	  else
	    {
	      bottom = center + 1;
	    }
	}

      gdb_assert (top == bottom);

      /* For `case_sensitivity == case_sensitive_off' strcmp_iw_ordered will
	 search more exactly than what matches SYMBOL_MATCHES_SEARCH_NAME.  */
      while (top >= start && symbol_matches_search_name (&(*top)->ginfo,
							 lookup_name))
	top--;

      /* Fixup to have a symbol which matches SYMBOL_MATCHES_SEARCH_NAME.  */
      top++;

      while (top <= real_top && symbol_matches_search_name (&(*top)->ginfo,
							    lookup_name))
	{
	  if (symbol_matches_domain ((*top)->ginfo.language (),
				     (*top)->domain, domain))
	    return *top;
	  top++;
	}
    }

  /* Can't use a binary search or else we found during the binary search that
     we should also do a linear search.  */

  if (do_linear_search)
    {
      for (psym = start; psym < start + length; psym++)
	{
	  if (symbol_matches_domain ((*psym)->ginfo.language (),
				     (*psym)->domain, domain)
	      && symbol_matches_search_name (&(*psym)->ginfo, lookup_name))
	    return *psym;
	}
    }

  return NULL;
}

/* Get the symbol table that corresponds to a partial_symtab.
   This is fast after the first time you do it.
   The result will be NULL if the primary symtab has no symbols,
   which can happen.  Otherwise the result is the primary symtab
   that contains PST.  */

static struct compunit_symtab *
psymtab_to_symtab (struct objfile *objfile, struct partial_symtab *pst)
{
  /* If it is a shared psymtab, find an unshared psymtab that includes
     it.  Any such psymtab will do.  */
  while (pst->user != NULL)
    pst = pst->user;

  /* If it's been looked up before, return it.  */
  if (pst->get_compunit_symtab (objfile))
    return pst->get_compunit_symtab (objfile);

  /* If it has not yet been read in, read it.  */
  if (!pst->readin_p (objfile))
    {
      scoped_restore decrementer = increment_reading_symtab ();

      if (info_verbose)
	{
	  gdb_printf (_("Reading in symbols for %s...\n"),
		      pst->filename);
	  gdb_flush (gdb_stdout);
	}

      pst->read_symtab (objfile);
    }

  return pst->get_compunit_symtab (objfile);
}

/* Psymtab version of find_last_source_symtab.  See its definition in
   the definition of quick_symbol_functions in symfile.h.  */

struct symtab *
psymbol_functions::find_last_source_symtab (struct objfile *ofp)
{
  struct partial_symtab *cs_pst = NULL;

  for (partial_symtab *ps : partial_symbols (ofp))
    {
      const char *name = ps->filename;
      int len = strlen (name);

      if (!(len > 2 && (strcmp (&name[len - 2], ".h") == 0
			|| strcmp (name, "<<C++-namespaces>>") == 0)))
	cs_pst = ps;
    }

  if (cs_pst)
    {
      if (cs_pst->readin_p (ofp))
	{
	  internal_error (_("select_source_symtab: "
			  "readin pst found and no symtabs."));
	}
      else
	{
	  struct compunit_symtab *cust = psymtab_to_symtab (ofp, cs_pst);

	  if (cust == NULL)
	    return NULL;
	  return cust->primary_filetab ();
	}
    }
  return NULL;
}

/* Psymtab version of forget_cached_source_info.  See its definition in
   the definition of quick_symbol_functions in symfile.h.  */

void
psymbol_functions::forget_cached_source_info (struct objfile *objfile)
{
  for (partial_symtab *pst : partial_symbols (objfile))
    {
      if (pst->fullname != NULL)
	{
	  xfree (pst->fullname);
	  pst->fullname = NULL;
	}
    }
}

static void
print_partial_symbols (struct gdbarch *gdbarch, struct objfile *objfile,
		       const std::vector<partial_symbol *> &symbols,
		       const char *what, struct ui_file *outfile)
{
  gdb_printf (outfile, "  %s partial symbols:\n", what);
  for (partial_symbol *p : symbols)
    {
      QUIT;
      gdb_printf (outfile, "    `%s'", p->ginfo.linkage_name ());
      if (p->ginfo.demangled_name () != NULL)
	{
	  gdb_printf (outfile, "  `%s'",
		      p->ginfo.demangled_name ());
	}
      gdb_puts (", ", outfile);
      switch (p->domain)
	{
	case UNDEF_DOMAIN:
	  gdb_puts ("undefined domain, ", outfile);
	  break;
	case VAR_DOMAIN:
	  /* This is the usual thing -- don't print it.  */
	  break;
	case STRUCT_DOMAIN:
	  gdb_puts ("struct domain, ", outfile);
	  break;
	case MODULE_DOMAIN:
	  gdb_puts ("module domain, ", outfile);
	  break;
	case LABEL_DOMAIN:
	  gdb_puts ("label domain, ", outfile);
	  break;
	case COMMON_BLOCK_DOMAIN:
	  gdb_puts ("common block domain, ", outfile);
	  break;
	default:
	  gdb_puts ("<invalid domain>, ", outfile);
	  break;
	}
      switch (p->aclass)
	{
	case LOC_UNDEF:
	  gdb_puts ("undefined", outfile);
	  break;
	case LOC_CONST:
	  gdb_puts ("constant int", outfile);
	  break;
	case LOC_STATIC:
	  gdb_puts ("static", outfile);
	  break;
	case LOC_REGISTER:
	  gdb_puts ("register", outfile);
	  break;
	case LOC_ARG:
	  gdb_puts ("pass by value", outfile);
	  break;
	case LOC_REF_ARG:
	  gdb_puts ("pass by reference", outfile);
	  break;
	case LOC_REGPARM_ADDR:
	  gdb_puts ("register address parameter", outfile);
	  break;
	case LOC_LOCAL:
	  gdb_puts ("stack parameter", outfile);
	  break;
	case LOC_TYPEDEF:
	  gdb_puts ("type", outfile);
	  break;
	case LOC_LABEL:
	  gdb_puts ("label", outfile);
	  break;
	case LOC_BLOCK:
	  gdb_puts ("function", outfile);
	  break;
	case LOC_CONST_BYTES:
	  gdb_puts ("constant bytes", outfile);
	  break;
	case LOC_UNRESOLVED:
	  gdb_puts ("unresolved", outfile);
	  break;
	case LOC_OPTIMIZED_OUT:
	  gdb_puts ("optimized out", outfile);
	  break;
	case LOC_COMPUTED:
	  gdb_puts ("computed at runtime", outfile);
	  break;
	default:
	  gdb_puts ("<invalid location>", outfile);
	  break;
	}
      gdb_puts (", ", outfile);
      gdb_puts (paddress (gdbarch, CORE_ADDR (p->unrelocated_address ())),
		outfile);
      gdb_printf (outfile, "\n");
    }
}

static void
dump_psymtab (struct objfile *objfile, struct partial_symtab *psymtab,
	      struct ui_file *outfile)
{
  struct gdbarch *gdbarch = objfile->arch ();
  int i;

  if (psymtab->anonymous)
    {
      gdb_printf (outfile, "\nAnonymous partial symtab (%s) ",
		  psymtab->filename);
    }
  else
    {
      gdb_printf (outfile, "\nPartial symtab for source file %s ",
		  psymtab->filename);
    }
  gdb_printf (outfile, "(object %s)\n\n",
	      host_address_to_string (psymtab));
  gdb_printf (outfile, "  Read from object file %s (%s)\n",
	      objfile_name (objfile),
	      host_address_to_string (objfile));

  if (psymtab->readin_p (objfile))
    gdb_printf
      (outfile,
       "  Full symtab was read (at %s)\n",
       host_address_to_string (psymtab->get_compunit_symtab (objfile)));

  gdb_printf (outfile, "  Symbols cover text addresses ");
  gdb_puts (paddress (gdbarch, psymtab->text_low (objfile)), outfile);
  gdb_printf (outfile, "-");
  gdb_puts (paddress (gdbarch, psymtab->text_high (objfile)), outfile);
  gdb_printf (outfile, "\n");
  gdb_printf (outfile, "  Depends on %d other partial symtabs.\n",
	      psymtab->number_of_dependencies);
  for (i = 0; i < psymtab->number_of_dependencies; i++)
    gdb_printf (outfile, "    %d %s\n", i,
		host_address_to_string (psymtab->dependencies[i]));
  if (psymtab->user != NULL)
    gdb_printf (outfile, "  Shared partial symtab with user %s\n",
		host_address_to_string (psymtab->user));
  if (!psymtab->global_psymbols.empty ())
    {
      print_partial_symbols
	(gdbarch, objfile, psymtab->global_psymbols,
	 "Global", outfile);
    }
  if (!psymtab->static_psymbols.empty ())
    {
      print_partial_symbols
	(gdbarch, objfile, psymtab->static_psymbols,
	 "Static", outfile);
    }
  gdb_printf (outfile, "\n");
}

/* Count the number of partial symbols in OBJFILE.  */

int
psymbol_functions::count_psyms ()
{
  int count = 0;
  for (partial_symtab *pst : m_partial_symtabs->range ())
    {
      count += pst->global_psymbols.size ();
      count += pst->static_psymbols.size ();
    }
  return count;
}

/* Psymtab version of print_stats.  See its definition in
   the definition of quick_symbol_functions in symfile.h.  */

void
psymbol_functions::print_stats (struct objfile *objfile, bool print_bcache)
{
  int i;

  if (!print_bcache)
    {
      int n_psyms = count_psyms ();
      if (n_psyms > 0)
	gdb_printf (_("  Number of \"partial\" symbols read: %d\n"),
		    n_psyms);

      i = 0;
      for (partial_symtab *ps : partial_symbols (objfile))
	{
	  if (!ps->readin_p (objfile))
	    i++;
	}
      gdb_printf (_("  Number of psym tables (not yet expanded): %d\n"),
		  i);
      gdb_printf (_("  Total memory used for psymbol cache: %d\n"),
		  m_partial_symtabs->psymbol_cache.memory_used ());
    }
  else
    {
      gdb_printf (_("Psymbol byte cache statistics:\n"));
      m_partial_symtabs->psymbol_cache.print_statistics
	("partial symbol cache");
    }
}

/* Psymtab version of dump.  See its definition in
   the definition of quick_symbol_functions in symfile.h.  */

void
psymbol_functions::dump (struct objfile *objfile)
{
  struct partial_symtab *psymtab;

  if (m_partial_symtabs->psymtabs)
    {
      gdb_printf ("Psymtabs:\n");
      for (psymtab = m_partial_symtabs->psymtabs;
	   psymtab != NULL;
	   psymtab = psymtab->next)
	gdb_printf ("%s at %s\n",
		    psymtab->filename,
		    host_address_to_string (psymtab));
      gdb_printf ("\n\n");
    }
}

/* Psymtab version of expand_all_symtabs.  See its definition in
   the definition of quick_symbol_functions in symfile.h.  */

void
psymbol_functions::expand_all_symtabs (struct objfile *objfile)
{
  for (partial_symtab *psymtab : partial_symbols (objfile))
    psymtab_to_symtab (objfile, psymtab);
}

/* Psymtab version of map_symbol_filenames.  See its definition in
   the definition of quick_symbol_functions in symfile.h.  */

void
psymbol_functions::map_symbol_filenames
     (struct objfile *objfile,
      gdb::function_view<symbol_filename_ftype> fun,
      bool need_fullname)
{
  for (partial_symtab *ps : partial_symbols (objfile))
    {
      const char *fullname;

      if (ps->readin_p (objfile))
	continue;

      /* We can skip shared psymtabs here, because any file name will be
	 attached to the unshared psymtab.  */
      if (ps->user != NULL)
	continue;

      /* Anonymous psymtabs don't have a file name.  */
      if (ps->anonymous)
	continue;

      QUIT;
      if (need_fullname)
	fullname = psymtab_to_fullname (ps);
      else
	fullname = NULL;
      fun (ps->filename, fullname);
    }
}

/* Finds the fullname that a partial_symtab represents.

   If this functions finds the fullname, it will save it in ps->fullname
   and it will also return the value.

   If this function fails to find the file that this partial_symtab represents,
   NULL will be returned and ps->fullname will be set to NULL.  */

static const char *
psymtab_to_fullname (struct partial_symtab *ps)
{
  gdb_assert (!ps->anonymous);

  /* Use cached copy if we have it.
     We rely on forget_cached_source_info being called appropriately
     to handle cases like the file being moved.  */
  if (ps->fullname == NULL)
    {
      gdb::unique_xmalloc_ptr<char> fullname
	= find_source_or_rewrite (ps->filename, ps->dirname);
      ps->fullname = fullname.release ();
    }

  return ps->fullname;
}

/* A helper for psym_expand_symtabs_matching that handles searching
   included psymtabs.  This returns true if a symbol is found, and
   false otherwise.  It also updates the 'searched_flag' on the
   various psymtabs that it searches.  */

static bool
recursively_search_psymtabs
  (struct partial_symtab *ps,
   struct objfile *objfile,
   block_search_flags search_flags,
   domain_enum domain,
   enum search_domain search,
   const lookup_name_info &lookup_name,
   gdb::function_view<expand_symtabs_symbol_matcher_ftype> sym_matcher)
{
  int keep_going = 1;
  enum psymtab_search_status result = PST_SEARCHED_AND_NOT_FOUND;
  int i;

  if (ps->searched_flag != PST_NOT_SEARCHED)
    return ps->searched_flag == PST_SEARCHED_AND_FOUND;

  /* Recurse into shared psymtabs first, because they may have already
     been searched, and this could save some time.  */
  for (i = 0; i < ps->number_of_dependencies; ++i)
    {
      int r;

      /* Skip non-shared dependencies, these are handled elsewhere.  */
      if (ps->dependencies[i]->user == NULL)
	continue;

      r = recursively_search_psymtabs (ps->dependencies[i],
				       objfile, search_flags, domain, search,
				       lookup_name, sym_matcher);
      if (r != 0)
	{
	  ps->searched_flag = PST_SEARCHED_AND_FOUND;
	  return true;
	}
    }

  partial_symbol **gbound = (ps->global_psymbols.data ()
			     + ps->global_psymbols.size ());
  partial_symbol **sbound = (ps->static_psymbols.data ()
			     + ps->static_psymbols.size ());
  partial_symbol **bound = gbound;

  /* Go through all of the symbols stored in a partial
     symtab in one loop.  */
  partial_symbol **psym = ps->global_psymbols.data ();

  if ((search_flags & SEARCH_GLOBAL_BLOCK) == 0)
    {
      if (ps->static_psymbols.empty ())
	keep_going = 0;
      else
	{
	  psym = ps->static_psymbols.data ();
	  bound = sbound;
	}
    }

  while (keep_going)
    {
      if (psym >= bound)
	{
	  if (bound == gbound && !ps->static_psymbols.empty ()
	      && (search_flags & SEARCH_STATIC_BLOCK) != 0)
	    {
	      psym = ps->static_psymbols.data ();
	      bound = sbound;
	    }
	  else
	    keep_going = 0;
	  continue;
	}
      else
	{
	  QUIT;

	  if ((domain == UNDEF_DOMAIN
	       || symbol_matches_domain ((*psym)->ginfo.language (),
					 (*psym)->domain, domain))
	      && (search == ALL_DOMAIN
		  || (search == MODULES_DOMAIN
		      && (*psym)->domain == MODULE_DOMAIN)
		  || (search == VARIABLES_DOMAIN
		      && (*psym)->aclass != LOC_TYPEDEF
		      && (*psym)->aclass != LOC_BLOCK)
		  || (search == FUNCTIONS_DOMAIN
		      && (*psym)->aclass == LOC_BLOCK)
		  || (search == TYPES_DOMAIN
		      && (*psym)->aclass == LOC_TYPEDEF))
	      && psymbol_name_matches (*psym, lookup_name)
	      && (sym_matcher == NULL
		  || sym_matcher ((*psym)->ginfo.search_name ())))
	    {
	      /* Found a match, so notify our caller.  */
	      result = PST_SEARCHED_AND_FOUND;
	      keep_going = 0;
	    }
	}
      psym++;
    }

  ps->searched_flag = result;
  return result == PST_SEARCHED_AND_FOUND;
}

/* Psymtab version of expand_symtabs_matching.  See its definition in
   the definition of quick_symbol_functions in symfile.h.  */

bool
psymbol_functions::expand_symtabs_matching
  (struct objfile *objfile,
   gdb::function_view<expand_symtabs_file_matcher_ftype> file_matcher,
   const lookup_name_info *lookup_name,
   gdb::function_view<expand_symtabs_symbol_matcher_ftype> symbol_matcher,
   gdb::function_view<expand_symtabs_exp_notify_ftype> expansion_notify,
   block_search_flags search_flags,
   domain_enum domain,
   enum search_domain search)
{
  /* Clear the search flags.  */
  for (partial_symtab *ps : partial_symbols (objfile))
    ps->searched_flag = PST_NOT_SEARCHED;

  std::optional<lookup_name_info> psym_lookup_name;
  if (lookup_name != nullptr)
    psym_lookup_name = lookup_name->make_ignore_params ();

  /* This invariant is documented in quick-functions.h.  */
  gdb_assert (lookup_name != nullptr || symbol_matcher == nullptr);

  for (partial_symtab *ps : m_partial_symtabs->range ())
    {
      QUIT;

      if (ps->readin_p (objfile))
	continue;

      if (file_matcher)
	{
	  bool match;

	  if (ps->anonymous)
	    continue;

	  match = file_matcher (ps->filename, false);
	  if (!match)
	    {
	      /* Before we invoke realpath, which can get expensive when many
		 files are involved, do a quick comparison of the basenames.  */
	      if (basenames_may_differ
		  || file_matcher (lbasename (ps->filename), true))
		match = file_matcher (psymtab_to_fullname (ps), false);
	    }
	  if (!match)
	    continue;
	}

      if (lookup_name == nullptr
	  || recursively_search_psymtabs (ps, objfile, search_flags,
					  domain, search,
					  *psym_lookup_name,
					  symbol_matcher))
	{
	  compunit_symtab *cust = psymtab_to_symtab (objfile, ps);

	  if (cust != nullptr && expansion_notify != nullptr)
	    if (!expansion_notify (cust))
	      return false;
	}
    }

  return true;
}

/* Psymtab version of has_symbols.  See its definition in
   the definition of quick_symbol_functions in symfile.h.  */

bool
psymbol_functions::has_symbols (struct objfile *objfile)
{
  return m_partial_symtabs->psymtabs != NULL;
}

/* See quick_symbol_functions::has_unexpanded_symtabs in quick-symbol.h.  */

bool
psymbol_functions::has_unexpanded_symtabs (struct objfile *objfile)
{
  for (partial_symtab *psymtab : partial_symbols (objfile))
    {
      /* Is this already expanded?  */
      if (psymtab->readin_p (objfile))
	continue;

      /* It has not yet been expanded.  */
      return true;
    }

  return false;
}



/* Partially fill a partial symtab.  It will be completely filled at
   the end of the symbol list.  */

partial_symtab::partial_symtab (const char *filename,
				psymtab_storage *partial_symtabs,
				objfile_per_bfd_storage *objfile_per_bfd,
				unrelocated_addr textlow)
  : partial_symtab (filename, partial_symtabs, objfile_per_bfd)
{
  set_text_low (textlow);
  set_text_high (unrelocated_text_low ()); /* default */
}

/* Perform "finishing up" operations of a partial symtab.  */

void
partial_symtab::end ()
{
  global_psymbols.shrink_to_fit ();
  static_psymbols.shrink_to_fit ();

  /* Sort the global list; don't sort the static list.  */
  std::sort (global_psymbols.begin (),
	     global_psymbols.end (),
	     [] (partial_symbol *s1, partial_symbol *s2)
    {
      return strcmp_iw_ordered (s1->ginfo.search_name (),
				s2->ginfo.search_name ()) < 0;
    });
}

/* See psymtab.h.  */

unsigned long
psymbol_bcache::hash (const void *addr, int length)
{
  unsigned long h = 0;
  struct partial_symbol *psymbol = (struct partial_symbol *) addr;
  unsigned int lang = psymbol->ginfo.language ();
  unsigned int domain = psymbol->domain;
  unsigned int theclass = psymbol->aclass;

  h = fast_hash (&psymbol->ginfo.m_value, sizeof (psymbol->ginfo.m_value), h);
  h = fast_hash (&lang, sizeof (unsigned int), h);
  h = fast_hash (&domain, sizeof (unsigned int), h);
  h = fast_hash (&theclass, sizeof (unsigned int), h);
  /* Note that psymbol names are interned via compute_and_set_names, so
     there's no need to hash the contents of the name here.  */
  h = fast_hash (&psymbol->ginfo.m_name, sizeof (psymbol->ginfo.m_name), h);

  return h;
}

/* See psymtab.h.  */

int
psymbol_bcache::compare (const void *addr1, const void *addr2, int length)
{
  struct partial_symbol *sym1 = (struct partial_symbol *) addr1;
  struct partial_symbol *sym2 = (struct partial_symbol *) addr2;

  return (memcmp (&sym1->ginfo.m_value, &sym2->ginfo.m_value,
		  sizeof (sym1->ginfo.m_value)) == 0
	  && sym1->ginfo.language () == sym2->ginfo.language ()
	  && sym1->domain == sym2->domain
	  && sym1->aclass == sym2->aclass
	  /* Note that psymbol names are interned via
	     compute_and_set_names, so there's no need to compare the
	     contents of the name here.  */
	  && sym1->ginfo.linkage_name () == sym2->ginfo.linkage_name ());
}

/* See psymtab.h.  */

void
partial_symtab::add_psymbol (const partial_symbol &psymbol,
			     psymbol_placement where,
			     psymtab_storage *partial_symtabs,
			     struct objfile *objfile)
{
  bool added;

  /* Stash the partial symbol away in the cache.  */
  partial_symbol *psym
    = ((struct partial_symbol *)
       partial_symtabs->psymbol_cache.insert
       (&psymbol, sizeof (struct partial_symbol), &added));

  /* Do not duplicate global partial symbols.  */
  if (where == psymbol_placement::GLOBAL && !added)
    return;

  /* Save pointer to partial symbol in psymtab, growing symtab if needed.  */
  std::vector<partial_symbol *> &list
    = (where == psymbol_placement::STATIC
       ? static_psymbols
       : global_psymbols);
  list.push_back (psym);
}

/* See psymtab.h.  */

void
partial_symtab::add_psymbol (std::string_view name, bool copy_name,
			     domain_enum domain,
			     enum address_class theclass,
			     short section,
			     psymbol_placement where,
			     unrelocated_addr coreaddr,
			     enum language language,
			     psymtab_storage *partial_symtabs,
			     struct objfile *objfile)
{
  struct partial_symbol psymbol;
  memset (&psymbol, 0, sizeof (psymbol));

  psymbol.set_unrelocated_address (coreaddr);
  psymbol.ginfo.set_section_index (section);
  psymbol.domain = domain;
  psymbol.aclass = theclass;
  psymbol.ginfo.set_language (language, partial_symtabs->obstack ());
  psymbol.ginfo.compute_and_set_names (name, copy_name, objfile->per_bfd);

  add_psymbol (psymbol, where, partial_symtabs, objfile);
}

/* See psymtab.h.  */

partial_symtab::partial_symtab (const char *filename_,
				psymtab_storage *partial_symtabs,
				objfile_per_bfd_storage *objfile_per_bfd)
  : searched_flag (PST_NOT_SEARCHED),
    text_low_valid (0),
    text_high_valid (0)
{
  partial_symtabs->install_psymtab (this);

  filename = objfile_per_bfd->intern (filename_);

  if (symtab_create_debug >= 1)
    {
      /* Be a bit clever with debugging messages, and don't print objfile
	 every time, only when it changes.  */
      static std::string last_bfd_name;
      const char *this_bfd_name
	= bfd_get_filename (objfile_per_bfd->get_bfd ());

      if (last_bfd_name.empty () || last_bfd_name != this_bfd_name)
	{
	  last_bfd_name = this_bfd_name;

	  symtab_create_debug_printf ("creating one or more psymtabs for %s",
				      this_bfd_name);
	}

      symtab_create_debug_printf ("created psymtab %s for module %s",
				  host_address_to_string (this), filename);
    }
}

/* See psymtab.h.  */

void
partial_symtab::expand_dependencies (struct objfile *objfile)
{
  for (int i = 0; i < number_of_dependencies; ++i)
    {
      if (!dependencies[i]->readin_p (objfile)
	  && dependencies[i]->user == NULL)
	{
	  /* Inform about additional files to be read in.  */
	  if (info_verbose)
	    {
	      gdb_puts (" ");
	      gdb_stdout->wrap_here (0);
	      gdb_puts ("and ");
	      gdb_stdout->wrap_here (0);
	      gdb_printf ("%s...", dependencies[i]->filename);
	      gdb_flush (gdb_stdout);
	    }
	  dependencies[i]->expand_psymtab (objfile);
	}
    }
}


void
psymtab_storage::discard_psymtab (struct partial_symtab *pst)
{
  struct partial_symtab **prev_pst;

  /* From dbxread.c:
     Empty psymtabs happen as a result of header files which don't
     have any symbols in them.  There can be a lot of them.  But this
     check is wrong, in that a psymtab with N_SLINE entries but
     nothing else is not empty, but we don't realize that.  Fixing
     that without slowing things down might be tricky.  */

  /* First, snip it out of the psymtab chain.  */

  prev_pst = &psymtabs;
  while ((*prev_pst) != pst)
    prev_pst = &((*prev_pst)->next);
  (*prev_pst) = pst->next;
  delete pst;
}



static void
maintenance_print_psymbols (const char *args, int from_tty)
{
  struct ui_file *outfile = gdb_stdout;
  char *address_arg = NULL, *source_arg = NULL, *objfile_arg = NULL;
  int i, outfile_idx, found;
  CORE_ADDR pc = 0;
  struct obj_section *section = NULL;

  dont_repeat ();

  gdb_argv argv (args);

  for (i = 0; argv != NULL && argv[i] != NULL; ++i)
    {
      if (strcmp (argv[i], "-pc") == 0)
	{
	  if (argv[i + 1] == NULL)
	    error (_("Missing pc value"));
	  address_arg = argv[++i];
	}
      else if (strcmp (argv[i], "-source") == 0)
	{
	  if (argv[i + 1] == NULL)
	    error (_("Missing source file"));
	  source_arg = argv[++i];
	}
      else if (strcmp (argv[i], "-objfile") == 0)
	{
	  if (argv[i + 1] == NULL)
	    error (_("Missing objfile name"));
	  objfile_arg = argv[++i];
	}
      else if (strcmp (argv[i], "--") == 0)
	{
	  /* End of options.  */
	  ++i;
	  break;
	}
      else if (argv[i][0] == '-')
	{
	  /* Future proofing: Don't allow OUTFILE to begin with "-".  */
	  error (_("Unknown option: %s"), argv[i]);
	}
      else
	break;
    }
  outfile_idx = i;

  if (address_arg != NULL && source_arg != NULL)
    error (_("Must specify at most one of -pc and -source"));

  stdio_file arg_outfile;

  if (argv != NULL && argv[outfile_idx] != NULL)
    {
      if (argv[outfile_idx + 1] != NULL)
	error (_("Junk at end of command"));
      gdb::unique_xmalloc_ptr<char> outfile_name
	(tilde_expand (argv[outfile_idx]));
      if (!arg_outfile.open (outfile_name.get (), FOPEN_WT))
	perror_with_name (outfile_name.get ());
      outfile = &arg_outfile;
    }

  if (address_arg != NULL)
    {
      pc = parse_and_eval_address (address_arg);
      /* If we fail to find a section, that's ok, try the lookup anyway.  */
      section = find_pc_section (pc);
    }

  found = 0;
  for (objfile *objfile : current_program_space->objfiles ())
    {
      int printed_objfile_header = 0;
      int print_for_objfile = 1;

      QUIT;
      if (objfile_arg != NULL)
	print_for_objfile
	  = compare_filenames_for_search (objfile_name (objfile),
					  objfile_arg);
      if (!print_for_objfile)
	continue;

      for (const auto &iter : objfile->qf)
	{
	  psymbol_functions *psf
	    = dynamic_cast<psymbol_functions *> (iter.get ());
	  if (psf == nullptr)
	    continue;

	  if (address_arg != NULL)
	    {
	      struct bound_minimal_symbol msymbol;

	      /* We don't assume each pc has a unique objfile (this is for
		 debugging).  */
	      struct partial_symtab *ps
		= psf->find_pc_sect_psymtab (objfile, pc, section, msymbol);
	      if (ps != NULL)
		{
		  if (!printed_objfile_header)
		    {
		      outfile->printf ("\nPartial symtabs for objfile %s\n",
				       objfile_name (objfile));
		      printed_objfile_header = 1;
		    }
		  dump_psymtab (objfile, ps, outfile);
		  found = 1;
		}
	    }
	  else
	    {
	      for (partial_symtab *ps : psf->partial_symbols (objfile))
		{
		  int print_for_source = 0;

		  QUIT;
		  if (source_arg != NULL)
		    {
		      print_for_source
			= compare_filenames_for_search (ps->filename, source_arg);
		      found = 1;
		    }
		  if (source_arg == NULL
		      || print_for_source)
		    {
		      if (!printed_objfile_header)
			{
			  outfile->printf ("\nPartial symtabs for objfile %s\n",
					   objfile_name (objfile));
			  printed_objfile_header = 1;
			}
		      dump_psymtab (objfile, ps, outfile);
		    }
		}
	    }
	}
    }

  if (!found)
    {
      if (address_arg != NULL)
	error (_("No partial symtab for address: %s"), address_arg);
      if (source_arg != NULL)
	error (_("No partial symtab for source file: %s"), source_arg);
    }
}

/* List all the partial symbol tables whose names match REGEXP (optional).  */

static void
maintenance_info_psymtabs (const char *regexp, int from_tty)
{
  if (regexp)
    re_comp (regexp);

  for (struct program_space *pspace : program_spaces)
    for (objfile *objfile : pspace->objfiles ())
      {
	struct gdbarch *gdbarch = objfile->arch ();

	/* We don't want to print anything for this objfile until we
	   actually find a symtab whose name matches.  */
	int printed_objfile_start = 0;

	for (const auto &iter : objfile->qf)
	  {
	    psymbol_functions *psf
	      = dynamic_cast<psymbol_functions *> (iter.get ());
	    if (psf == nullptr)
	      continue;
	    for (partial_symtab *psymtab : psf->partial_symbols (objfile))
	      {
		QUIT;

		if (! regexp
		    || re_exec (psymtab->filename))
		  {
		    if (! printed_objfile_start)
		      {
			gdb_printf ("{ objfile %s ", objfile_name (objfile));
			gdb_stdout->wrap_here (2);
			gdb_printf ("((struct objfile *) %s)\n",
				    host_address_to_string (objfile));
			printed_objfile_start = 1;
		      }

		    gdb_printf ("  { psymtab %s ", psymtab->filename);
		    gdb_stdout->wrap_here (4);
		    gdb_printf ("((struct partial_symtab *) %s)\n",
				host_address_to_string (psymtab));

		    gdb_printf ("    readin %s\n",
				psymtab->readin_p (objfile) ? "yes" : "no");
		    gdb_printf ("    fullname %s\n",
				psymtab->fullname
				? psymtab->fullname : "(null)");
		    gdb_printf ("    text addresses ");
		    gdb_puts (paddress (gdbarch,
					psymtab->text_low (objfile)));
		    gdb_printf (" -- ");
		    gdb_puts (paddress (gdbarch,
					psymtab->text_high (objfile)));
		    gdb_printf ("\n");
		    gdb_printf ("    globals ");
		    if (!psymtab->global_psymbols.empty ())
		      gdb_printf
			("(* (struct partial_symbol **) %s @ %d)\n",
			 host_address_to_string (psymtab->global_psymbols.data ()),
			 (int) psymtab->global_psymbols.size ());
		    else
		      gdb_printf ("(none)\n");
		    gdb_printf ("    statics ");
		    if (!psymtab->static_psymbols.empty ())
		      gdb_printf
			("(* (struct partial_symbol **) %s @ %d)\n",
			 host_address_to_string (psymtab->static_psymbols.data ()),
			 (int) psymtab->static_psymbols.size ());
		    else
		      gdb_printf ("(none)\n");
		    if (psymtab->user)
		      gdb_printf ("    user %s "
				  "((struct partial_symtab *) %s)\n",
				  psymtab->user->filename,
				  host_address_to_string (psymtab->user));
		    gdb_printf ("    dependencies ");
		    if (psymtab->number_of_dependencies)
		      {
			int i;

			gdb_printf ("{\n");
			for (i = 0; i < psymtab->number_of_dependencies; i++)
			  {
			    struct partial_symtab *dep = psymtab->dependencies[i];

			    /* Note the string concatenation there --- no
			       comma.  */
			    gdb_printf ("      psymtab %s "
					"((struct partial_symtab *) %s)\n",
					dep->filename,
					host_address_to_string (dep));
			  }
			gdb_printf ("    }\n");
		      }
		    else
		      gdb_printf ("(none)\n");
		    gdb_printf ("  }\n");
		  }
	      }
	  }

	if (printed_objfile_start)
	  gdb_printf ("}\n");
      }
}

/* Check consistency of currently expanded psymtabs vs symtabs.  */

static void
maintenance_check_psymtabs (const char *ignore, int from_tty)
{
  struct symbol *sym;
  struct compunit_symtab *cust = NULL;
  const struct blockvector *bv;
  const struct block *b;

  for (objfile *objfile : current_program_space->objfiles ())
    {
      for (const auto &iter : objfile->qf)
	{
	  psymbol_functions *psf
	    = dynamic_cast<psymbol_functions *> (iter.get ());
	  if (psf == nullptr)
	    continue;

	  for (partial_symtab *ps : psf->partial_symbols (objfile))
	    {
	      struct gdbarch *gdbarch = objfile->arch ();

	      /* We don't call psymtab_to_symtab here because that may cause symtab
		 expansion.  When debugging a problem it helps if checkers leave
		 things unchanged.  */
	      cust = ps->get_compunit_symtab (objfile);

	      /* First do some checks that don't require the associated symtab.  */
	      if (ps->text_high (objfile) < ps->text_low (objfile))
		{
		  gdb_printf ("Psymtab ");
		  gdb_puts (ps->filename);
		  gdb_printf (" covers bad range ");
		  gdb_puts (paddress (gdbarch, ps->text_low (objfile)));
		  gdb_printf (" - ");
		  gdb_puts (paddress (gdbarch, ps->text_high (objfile)));
		  gdb_printf ("\n");
		  continue;
		}

	      /* Now do checks requiring the associated symtab.  */
	      if (cust == NULL)
		continue;
	      bv = cust->blockvector ();
	      b = bv->static_block ();
	      for (partial_symbol *psym : ps->static_psymbols)
		{
		  /* Skip symbols for inlined functions without address.  These may
		     or may not have a match in the full symtab.  */
		  if (psym->aclass == LOC_BLOCK
		      && psym->ginfo.value_address () == 0)
		    continue;

		  sym = block_lookup_symbol (b, psym->ginfo.search_name (),
					     symbol_name_match_type::SEARCH_NAME,
					     psym->domain);
		  if (!sym)
		    {
		      gdb_printf ("Static symbol `");
		      gdb_puts (psym->ginfo.linkage_name ());
		      gdb_printf ("' only found in ");
		      gdb_puts (ps->filename);
		      gdb_printf (" psymtab\n");
		    }
		}
	      b = bv->global_block ();
	      for (partial_symbol *psym : ps->global_psymbols)
		{
		  sym = block_lookup_symbol (b, psym->ginfo.search_name (),
					     symbol_name_match_type::SEARCH_NAME,
					     psym->domain);
		  if (!sym)
		    {
		      gdb_printf ("Global symbol `");
		      gdb_puts (psym->ginfo.linkage_name ());
		      gdb_printf ("' only found in ");
		      gdb_puts (ps->filename);
		      gdb_printf (" psymtab\n");
		    }
		}
	      if (ps->unrelocated_text_high () != unrelocated_addr (0)
		  && (ps->text_low (objfile) < b->start ()
		      || ps->text_high (objfile) > b->end ()))
		{
		  gdb_printf ("Psymtab ");
		  gdb_puts (ps->filename);
		  gdb_printf (" covers ");
		  gdb_puts (paddress (gdbarch, ps->text_low (objfile)));
		  gdb_printf (" - ");
		  gdb_puts (paddress (gdbarch, ps->text_high (objfile)));
		  gdb_printf (" but symtab covers only ");
		  gdb_puts (paddress (gdbarch, b->start ()));
		  gdb_printf (" - ");
		  gdb_puts (paddress (gdbarch, b->end ()));
		  gdb_printf ("\n");
		}
	    }
	}
    }
}

void _initialize_psymtab ();
void
_initialize_psymtab ()
{
  add_cmd ("psymbols", class_maintenance, maintenance_print_psymbols, _("\
Print dump of current partial symbol definitions.\n\
Usage: mt print psymbols [-objfile OBJFILE] [-pc ADDRESS] [--] [OUTFILE]\n\
       mt print psymbols [-objfile OBJFILE] [-source SOURCE] [--] [OUTFILE]\n\
Entries in the partial symbol table are dumped to file OUTFILE,\n\
or the terminal if OUTFILE is unspecified.\n\
If ADDRESS is provided, dump only the symbols for the file with code at that address.\n\
If SOURCE is provided, dump only that file's symbols.\n\
If OBJFILE is provided, dump only that object file's symbols."),
	   &maintenanceprintlist);

  add_cmd ("psymtabs", class_maintenance, maintenance_info_psymtabs, _("\
List the partial symbol tables for all object files.\n\
This does not include information about individual partial symbols,\n\
just the symbol table structures themselves."),
	   &maintenanceinfolist);

  add_cmd ("check-psymtabs", class_maintenance, maintenance_check_psymtabs,
	   _("\
Check consistency of currently expanded psymtabs versus symtabs."),
	   &maintenancelist);
}
