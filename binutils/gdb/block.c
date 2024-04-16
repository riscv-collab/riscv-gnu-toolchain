/* Block-related functions for the GNU debugger, GDB.

   Copyright (C) 2003-2024 Free Software Foundation, Inc.

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
#include "block.h"
#include "symtab.h"
#include "symfile.h"
#include "gdbsupport/gdb_obstack.h"
#include "cp-support.h"
#include "addrmap.h"
#include "gdbtypes.h"
#include "objfiles.h"

/* This is used by struct block to store namespace-related info for
   C++ files, namely using declarations and the current namespace in
   scope.  */

struct block_namespace_info : public allocate_on_obstack
{
  const char *scope = nullptr;
  struct using_direct *using_decl = nullptr;
};

/* See block.h.  */

struct objfile *
block::objfile () const
{
  const struct global_block *global_block;

  if (function () != nullptr)
    return function ()->objfile ();

  global_block = (struct global_block *) this->global_block ();
  return global_block->compunit_symtab->objfile ();
}

/* See block.  */

struct gdbarch *
block::gdbarch () const
{
  if (function () != nullptr)
    return function ()->arch ();

  return objfile ()->arch ();
}

/* See block.h.  */

bool
block::contains (const struct block *a, bool allow_nested) const
{
  if (a == nullptr)
    return false;

  do
    {
      if (a == this)
	return true;
      /* If A is a function block, then A cannot be contained in B,
	 except if A was inlined.  */
      if (!allow_nested && a->function () != NULL && !a->inlined_p ())
	return false;
      a = a->superblock ();
    }
  while (a != NULL);

  return false;
}

/* See block.h.  */

struct symbol *
block::linkage_function () const
{
  const block *bl = this;

  while ((bl->function () == NULL || bl->inlined_p ())
	 && bl->superblock () != NULL)
    bl = bl->superblock ();

  return bl->function ();
}

/* See block.h.  */

struct symbol *
block::containing_function () const
{
  const block *bl = this;

  while (bl->function () == NULL && bl->superblock () != NULL)
    bl = bl->superblock ();

  return bl->function ();
}

/* See block.h.  */

bool
block::inlined_p () const
{
  return function () != nullptr && function ()->is_inlined ();
}

/* A helper function that checks whether PC is in the blockvector BL.
   It returns the containing block if there is one, or else NULL.  */

static const struct block *
find_block_in_blockvector (const struct blockvector *bl, CORE_ADDR pc)
{
  const struct block *b;
  int bot, top, half;

  /* If we have an addrmap mapping code addresses to blocks, then use
     that.  */
  if (bl->map ())
    return (const struct block *) bl->map ()->find (pc);

  /* Otherwise, use binary search to find the last block that starts
     before PC.
     Note: GLOBAL_BLOCK is block 0, STATIC_BLOCK is block 1.
     They both have the same START,END values.
     Historically this code would choose STATIC_BLOCK over GLOBAL_BLOCK but the
     fact that this choice was made was subtle, now we make it explicit.  */
  gdb_assert (bl->blocks ().size () >= 2);
  bot = STATIC_BLOCK;
  top = bl->blocks ().size ();

  while (top - bot > 1)
    {
      half = (top - bot + 1) >> 1;
      b = bl->block (bot + half);
      if (b->start () <= pc)
	bot += half;
      else
	top = bot + half;
    }

  /* Now search backward for a block that ends after PC.  */

  while (bot >= STATIC_BLOCK)
    {
      b = bl->block (bot);
      if (!(b->start () <= pc))
	return NULL;
      if (b->end () > pc)
	return b;
      bot--;
    }

  return NULL;
}

/* Return the blockvector immediately containing the innermost lexical
   block containing the specified pc value and section, or 0 if there
   is none.  PBLOCK is a pointer to the block.  If PBLOCK is NULL, we
   don't pass this information back to the caller.  */

const struct blockvector *
blockvector_for_pc_sect (CORE_ADDR pc, struct obj_section *section,
			 const struct block **pblock,
			 struct compunit_symtab *cust)
{
  const struct blockvector *bl;
  const struct block *b;

  if (cust == NULL)
    {
      /* First search all symtabs for one whose file contains our pc */
      cust = find_pc_sect_compunit_symtab (pc, section);
      if (cust == NULL)
	return 0;
    }

  bl = cust->blockvector ();

  /* Then search that symtab for the smallest block that wins.  */
  b = find_block_in_blockvector (bl, pc);
  if (b == NULL)
    return NULL;

  if (pblock)
    *pblock = b;
  return bl;
}

/* Return true if the blockvector BV contains PC, false otherwise.  */

int
blockvector_contains_pc (const struct blockvector *bv, CORE_ADDR pc)
{
  return find_block_in_blockvector (bv, pc) != NULL;
}

/* Return call_site for specified PC in GDBARCH.  PC must match exactly, it
   must be the next instruction after call (or after tail call jump).  Throw
   NO_ENTRY_VALUE_ERROR otherwise.  This function never returns NULL.  */

struct call_site *
call_site_for_pc (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  struct compunit_symtab *cust;
  call_site *cs = nullptr;

  /* -1 as tail call PC can be already after the compilation unit range.  */
  cust = find_pc_compunit_symtab (pc - 1);

  if (cust != nullptr)
    cs = cust->find_call_site (pc);

  if (cs == nullptr)
    {
      struct bound_minimal_symbol msym = lookup_minimal_symbol_by_pc (pc);

      /* DW_TAG_gnu_call_site will be missing just if GCC could not determine
	 the call target.  */
      throw_error (NO_ENTRY_VALUE_ERROR,
		   _("DW_OP_entry_value resolving cannot find "
		     "DW_TAG_call_site %s in %s"),
		   paddress (gdbarch, pc),
		   (msym.minsym == NULL ? "???"
		    : msym.minsym->print_name ()));
    }

  return cs;
}

/* Return the blockvector immediately containing the innermost lexical block
   containing the specified pc value, or 0 if there is none.
   Backward compatibility, no section.  */

const struct blockvector *
blockvector_for_pc (CORE_ADDR pc, const struct block **pblock)
{
  return blockvector_for_pc_sect (pc, find_pc_mapped_section (pc),
				  pblock, NULL);
}

/* Return the innermost lexical block containing the specified pc value
   in the specified section, or 0 if there is none.  */

const struct block *
block_for_pc_sect (CORE_ADDR pc, struct obj_section *section)
{
  const struct blockvector *bl;
  const struct block *b;

  bl = blockvector_for_pc_sect (pc, section, &b, NULL);
  if (bl)
    return b;
  return 0;
}

/* Return the innermost lexical block containing the specified pc value,
   or 0 if there is none.  Backward compatibility, no section.  */

const struct block *
block_for_pc (CORE_ADDR pc)
{
  return block_for_pc_sect (pc, find_pc_mapped_section (pc));
}

/* Now come some functions designed to deal with C++ namespace issues.
   The accessors are safe to use even in the non-C++ case.  */

/* See block.h.  */

const char *
block::scope () const
{
  for (const block *block = this;
       block != nullptr;
       block = block->superblock ())
    {
      if (block->m_namespace_info != nullptr
	  && block->m_namespace_info->scope != nullptr)
	return block->m_namespace_info->scope;
    }

  return "";
}

/* See block.h.  */

void
block::initialize_namespace (struct obstack *obstack)
{
  if (m_namespace_info == nullptr)
    m_namespace_info = new (obstack) struct block_namespace_info;
}

/* See block.h.  */

void
block::set_scope (const char *scope, struct obstack *obstack)
{
  if (scope == nullptr || scope[0] == '\0')
    {
      /* Don't bother.  */
      return;
    }

  initialize_namespace (obstack);
  m_namespace_info->scope = scope;
}

/* See block.h.  */

struct using_direct *
block::get_using () const
{
  if (m_namespace_info == nullptr)
    return nullptr;
  else
    return m_namespace_info->using_decl;
}

/* See block.h.  */

void
block::set_using (struct using_direct *using_decl, struct obstack *obstack)
{
  if (using_decl == nullptr)
    {
      /* Don't bother.  */
      return;
    }

  initialize_namespace (obstack);
  m_namespace_info->using_decl = using_decl;
}

/* See block.h.  */

const struct block *
block::static_block () const
{
  if (superblock () == nullptr)
    return nullptr;

  const block *block = this;
  while (block->superblock ()->superblock () != NULL)
    block = block->superblock ();

  return block;
}

/* See block.h.  */

const struct block *
block::global_block () const
{
  const block *block = this;

  while (block->superblock () != NULL)
    block = block->superblock ();

  return block;
}

/* See block.h.  */

const struct block *
block::function_block () const
{
  const block *block = this;

  while (block != nullptr && block->function () == nullptr)
    block = block->superblock ();

  return block;
}

/* See block.h.  */

void
block::set_compunit_symtab (struct compunit_symtab *cu)
{
  struct global_block *gb;

  gdb_assert (superblock () == NULL);
  gb = (struct global_block *) this;
  gdb_assert (gb->compunit_symtab == NULL);
  gb->compunit_symtab = cu;
}

/* See block.h.  */

struct dynamic_prop *
block::static_link () const
{
  struct objfile *objfile = this->objfile ();

  /* Only objfile-owned blocks that materialize top function scopes can have
     static links.  */
  if (objfile == NULL || function () == NULL)
    return NULL;

  return (struct dynamic_prop *) objfile_lookup_static_link (objfile, this);
}

/* Return the compunit of the global block.  */

static struct compunit_symtab *
get_block_compunit_symtab (const struct block *block)
{
  struct global_block *gb;

  gdb_assert (block->superblock () == NULL);
  gb = (struct global_block *) block;
  gdb_assert (gb->compunit_symtab != NULL);
  return gb->compunit_symtab;
}



/* Initialize a block iterator, either to iterate over a single block,
   or, for static and global blocks, all the included symtabs as
   well.  */

static void
initialize_block_iterator (const struct block *block,
			   struct block_iterator *iter,
			   const lookup_name_info *name = nullptr)
{
  enum block_enum which;
  struct compunit_symtab *cu;

  iter->idx = -1;
  iter->name = name;

  if (block->superblock () == NULL)
    {
      which = GLOBAL_BLOCK;
      cu = get_block_compunit_symtab (block);
    }
  else if (block->superblock ()->superblock () == NULL)
    {
      which = STATIC_BLOCK;
      cu = get_block_compunit_symtab (block->superblock ());
    }
  else
    {
      iter->d.block = block;
      /* A signal value meaning that we're iterating over a single
	 block.  */
      iter->which = FIRST_LOCAL_BLOCK;
      return;
    }

  /* If this is an included symtab, find the canonical includer and
     use it instead.  */
  while (cu->user != NULL)
    cu = cu->user;

  /* Putting this check here simplifies the logic of the iterator
     functions.  If there are no included symtabs, we only need to
     search a single block, so we might as well just do that
     directly.  */
  if (cu->includes == NULL)
    {
      iter->d.block = block;
      /* A signal value meaning that we're iterating over a single
	 block.  */
      iter->which = FIRST_LOCAL_BLOCK;
    }
  else
    {
      iter->d.compunit_symtab = cu;
      iter->which = which;
    }
}

/* A helper function that finds the current compunit over whose static
   or global block we should iterate.  */

static struct compunit_symtab *
find_iterator_compunit_symtab (struct block_iterator *iterator)
{
  if (iterator->idx == -1)
    return iterator->d.compunit_symtab;
  return iterator->d.compunit_symtab->includes[iterator->idx];
}

/* Perform a single step for a plain block iterator, iterating across
   symbol tables as needed.  Returns the next symbol, or NULL when
   iteration is complete.  */

static struct symbol *
block_iterator_step (struct block_iterator *iterator, int first)
{
  struct symbol *sym;

  gdb_assert (iterator->which != FIRST_LOCAL_BLOCK);

  while (1)
    {
      if (first)
	{
	  struct compunit_symtab *cust
	    = find_iterator_compunit_symtab (iterator);
	  const struct block *block;

	  /* Iteration is complete.  */
	  if (cust == NULL)
	    return  NULL;

	  block = cust->blockvector ()->block (iterator->which);
	  sym = mdict_iterator_first (block->multidict (),
				      &iterator->mdict_iter);
	}
      else
	sym = mdict_iterator_next (&iterator->mdict_iter);

      if (sym != NULL)
	return sym;

      /* We have finished iterating the appropriate block of one
	 symtab.  Now advance to the next symtab and begin iteration
	 there.  */
      ++iterator->idx;
      first = 1;
    }
}

/* Perform a single step for a "match" block iterator, iterating
   across symbol tables as needed.  Returns the next symbol, or NULL
   when iteration is complete.  */

static struct symbol *
block_iter_match_step (struct block_iterator *iterator,
		       int first)
{
  struct symbol *sym;

  gdb_assert (iterator->which != FIRST_LOCAL_BLOCK);

  while (1)
    {
      if (first)
	{
	  struct compunit_symtab *cust
	    = find_iterator_compunit_symtab (iterator);
	  const struct block *block;

	  /* Iteration is complete.  */
	  if (cust == NULL)
	    return  NULL;

	  block = cust->blockvector ()->block (iterator->which);
	  sym = mdict_iter_match_first (block->multidict (), *iterator->name,
					&iterator->mdict_iter);
	}
      else
	sym = mdict_iter_match_next (*iterator->name, &iterator->mdict_iter);

      if (sym != NULL)
	return sym;

      /* We have finished iterating the appropriate block of one
	 symtab.  Now advance to the next symtab and begin iteration
	 there.  */
      ++iterator->idx;
      first = 1;
    }
}

/* See block.h.  */

struct symbol *
block_iterator_first (const struct block *block,
		      struct block_iterator *iterator,
		      const lookup_name_info *name)
{
  initialize_block_iterator (block, iterator, name);

  if (name == nullptr)
    {
      if (iterator->which == FIRST_LOCAL_BLOCK)
	return mdict_iterator_first (block->multidict (),
				     &iterator->mdict_iter);

      return block_iterator_step (iterator, 1);
    }

  if (iterator->which == FIRST_LOCAL_BLOCK)
    return mdict_iter_match_first (block->multidict (), *name,
				   &iterator->mdict_iter);

  return block_iter_match_step (iterator, 1);
}

/* See block.h.  */

struct symbol *
block_iterator_next (struct block_iterator *iterator)
{
  if (iterator->name == nullptr)
    {
      if (iterator->which == FIRST_LOCAL_BLOCK)
	return mdict_iterator_next (&iterator->mdict_iter);

      return block_iterator_step (iterator, 0);
    }

  if (iterator->which == FIRST_LOCAL_BLOCK)
    return mdict_iter_match_next (*iterator->name, &iterator->mdict_iter);

  return block_iter_match_step (iterator, 0);
}

/* See block.h.  */

bool
best_symbol (struct symbol *a, const domain_enum domain)
{
  return (a->domain () == domain
	  && a->aclass () != LOC_UNRESOLVED);
}

/* See block.h.  */

struct symbol *
better_symbol (struct symbol *a, struct symbol *b, const domain_enum domain)
{
  if (a == NULL)
    return b;
  if (b == NULL)
    return a;

  if (a->domain () == domain && b->domain () != domain)
    return a;

  if (b->domain () == domain && a->domain () != domain)
    return b;

  if (a->aclass () != LOC_UNRESOLVED && b->aclass () == LOC_UNRESOLVED)
    return a;

  if (b->aclass () != LOC_UNRESOLVED && a->aclass () == LOC_UNRESOLVED)
    return b;

  return a;
}

/* See block.h.

   Note that if NAME is the demangled form of a C++ symbol, we will fail
   to find a match during the binary search of the non-encoded names, but
   for now we don't worry about the slight inefficiency of looking for
   a match we'll never find, since it will go pretty quick.  Once the
   binary search terminates, we drop through and do a straight linear
   search on the symbols.  Each symbol which is marked as being a ObjC/C++
   symbol (language_cplus or language_objc set) has both the encoded and
   non-encoded names tested for a match.  */

struct symbol *
block_lookup_symbol (const struct block *block, const char *name,
		     symbol_name_match_type match_type,
		     const domain_enum domain)
{
  lookup_name_info lookup_name (name, match_type);

  if (!block->function ())
    {
      struct symbol *other = NULL;

      for (struct symbol *sym : block_iterator_range (block, &lookup_name))
	{
	  /* See comment related to PR gcc/debug/91507 in
	     block_lookup_symbol_primary.  */
	  if (best_symbol (sym, domain))
	    return sym;
	  /* This is a bit of a hack, but symbol_matches_domain might ignore
	     STRUCT vs VAR domain symbols.  So if a matching symbol is found,
	     make sure there is no "better" matching symbol, i.e., one with
	     exactly the same domain.  PR 16253.  */
	  if (sym->matches (domain))
	    other = better_symbol (other, sym, domain);
	}
      return other;
    }
  else
    {
      /* Note that parameter symbols do not always show up last in the
	 list; this loop makes sure to take anything else other than
	 parameter symbols first; it only uses parameter symbols as a
	 last resort.  Note that this only takes up extra computation
	 time on a match.
	 It's hard to define types in the parameter list (at least in
	 C/C++) so we don't do the same PR 16253 hack here that is done
	 for the !BLOCK_FUNCTION case.  */

      struct symbol *sym_found = NULL;

      for (struct symbol *sym : block_iterator_range (block, &lookup_name))
	{
	  if (sym->matches (domain))
	    {
	      sym_found = sym;
	      if (!sym->is_argument ())
		{
		  break;
		}
	    }
	}
      return (sym_found);	/* Will be NULL if not found.  */
    }
}

/* See block.h.  */

struct symbol *
block_lookup_symbol_primary (const struct block *block, const char *name,
			     const domain_enum domain)
{
  struct symbol *sym, *other;
  struct mdict_iterator mdict_iter;

  lookup_name_info lookup_name (name, symbol_name_match_type::FULL);

  /* Verify BLOCK is STATIC_BLOCK or GLOBAL_BLOCK.  */
  gdb_assert (block->superblock () == NULL
	      || block->superblock ()->superblock () == NULL);

  other = NULL;
  for (sym = mdict_iter_match_first (block->multidict (), lookup_name,
				     &mdict_iter);
       sym != NULL;
       sym = mdict_iter_match_next (lookup_name, &mdict_iter))
    {
      /* With the fix for PR gcc/debug/91507, we get for:
	 ...
	 extern char *zzz[];
	 char *zzz[ ] = {
	   "abc",
	   "cde"
	 };
	 ...
	 DWARF which will result in two entries in the symbol table, a decl
	 with type char *[] and a def with type char *[2].

	 If we return the decl here, we don't get the value of zzz:
	 ...
	 $ gdb a.spec.out -batch -ex "p zzz"
	 $1 = 0x601030 <zzz>
	 ...
	 because we're returning the symbol without location information, and
	 because the fallback that uses the address from the minimal symbols
	 doesn't work either because the type of the decl does not specify a
	 size.

	 To fix this, we prefer def over decl in best_symbol and
	 better_symbol.

	 In absence of the gcc fix, both def and decl have type char *[], so
	 the only option to make this work is improve the fallback to use the
	 size of the minimal symbol.  Filed as PR exp/24989.  */
      if (best_symbol (sym, domain))
	return sym;

      /* This is a bit of a hack, but 'matches' might ignore
	 STRUCT vs VAR domain symbols.  So if a matching symbol is found,
	 make sure there is no "better" matching symbol, i.e., one with
	 exactly the same domain.  PR 16253.  */
      if (sym->matches (domain))
	other = better_symbol (other, sym, domain);
    }

  return other;
}

/* See block.h.  */

struct symbol *
block_find_symbol (const struct block *block, const lookup_name_info &name,
		   const domain_enum domain, struct symbol **stub)
{
  /* Verify BLOCK is STATIC_BLOCK or GLOBAL_BLOCK.  */
  gdb_assert (block->superblock () == NULL
	      || block->superblock ()->superblock () == NULL);

  for (struct symbol *sym : block_iterator_range (block, &name))
    {
      if (!sym->matches (domain))
	continue;

      if (!TYPE_IS_OPAQUE (sym->type ()))
	return sym;

      if (stub != nullptr)
	*stub = sym;
    }
  return nullptr;
}

/* See block.h.  */

struct blockranges *
make_blockranges (struct objfile *objfile,
		  const std::vector<blockrange> &rangevec)
{
  struct blockranges *blr;
  size_t n = rangevec.size();

  blr = (struct blockranges *)
    obstack_alloc (&objfile->objfile_obstack,
		   sizeof (struct blockranges)
		   + (n - 1) * sizeof (struct blockrange));

  blr->nranges = n;
  for (int i = 0; i < n; i++)
    blr->range[i] = rangevec[i];
  return blr;
}

