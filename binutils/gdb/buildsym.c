/* Support routines for building symbol tables in GDB's internal format.
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

#include "defs.h"
#include "buildsym-legacy.h"
#include "bfd.h"
#include "gdbsupport/gdb_obstack.h"
#include "gdbsupport/pathstuff.h"
#include "symtab.h"
#include "symfile.h"
#include "objfiles.h"
#include "gdbtypes.h"
#include "complaints.h"
#include "expression.h"
#include "filenames.h"
#include "macrotab.h"
#include "demangle.h"
#include "block.h"
#include "cp-support.h"
#include "dictionary.h"
#include <algorithm>

/* For cleanup_undefined_stabs_types and finish_global_stabs (somewhat
   questionable--see comment where we call them).  */

#include "stabsread.h"

/* List of blocks already made (lexical contexts already closed).
   This is used at the end to make the blockvector.  */

struct pending_block
  {
    struct pending_block *next;
    struct block *block;
  };

buildsym_compunit::buildsym_compunit (struct objfile *objfile_,
				      const char *name,
				      const char *comp_dir_,
				      const char *name_for_id,
				      enum language language_,
				      CORE_ADDR last_addr)
  : m_objfile (objfile_),
    m_last_source_file (name == nullptr ? nullptr : xstrdup (name)),
    m_comp_dir (comp_dir_ == nullptr ? "" : comp_dir_),
    m_language (language_),
    m_last_source_start_addr (last_addr)
{
  /* Allocate the compunit symtab now.  The caller needs it to allocate
     non-primary symtabs.  It is also needed by get_macro_table.  */
  m_compunit_symtab = allocate_compunit_symtab (m_objfile, name);

  /* Build the subfile for NAME (the main source file) so that we can record
     a pointer to it for later.
     IMPORTANT: Do not allocate a struct symtab for NAME here.
     It can happen that the debug info provides a different path to NAME than
     DIRNAME,NAME.  We cope with this in watch_main_source_file_lossage but
     that only works if the main_subfile doesn't have a symtab yet.  */
  start_subfile (name, name_for_id);
  /* Save this so that we don't have to go looking for it at the end
     of the subfiles list.  */
  m_main_subfile = m_current_subfile;
}

buildsym_compunit::~buildsym_compunit ()
{
  struct subfile *subfile, *nextsub;

  if (m_pending_macros != nullptr)
    free_macro_table (m_pending_macros);

  for (subfile = m_subfiles;
       subfile != NULL;
       subfile = nextsub)
    {
      nextsub = subfile->next;
      delete subfile;
    }

  struct pending *next, *next1;

  for (next = m_file_symbols; next != NULL; next = next1)
    {
      next1 = next->next;
      xfree ((void *) next);
    }

  for (next = m_global_symbols; next != NULL; next = next1)
    {
      next1 = next->next;
      xfree ((void *) next);
    }
}

struct macro_table *
buildsym_compunit::get_macro_table ()
{
  if (m_pending_macros == nullptr)
    m_pending_macros = new_macro_table (&m_objfile->per_bfd->storage_obstack,
					&m_objfile->per_bfd->string_cache,
					m_compunit_symtab);
  return m_pending_macros;
}

/* Maintain the lists of symbols and blocks.  */

/* Add a symbol to one of the lists of symbols.  */

void
add_symbol_to_list (struct symbol *symbol, struct pending **listhead)
{
  struct pending *link;

  /* If this is an alias for another symbol, don't add it.  */
  if (symbol->linkage_name () && symbol->linkage_name ()[0] == '#')
    return;

  /* We keep PENDINGSIZE symbols in each link of the list.  If we
     don't have a link with room in it, add a new link.  */
  if (*listhead == NULL || (*listhead)->nsyms == PENDINGSIZE)
    {
      link = XNEW (struct pending);
      link->next = *listhead;
      *listhead = link;
      link->nsyms = 0;
    }

  (*listhead)->symbol[(*listhead)->nsyms++] = symbol;
}

/* Find a symbol named NAME on a LIST.  NAME need not be
   '\0'-terminated; LENGTH is the length of the name.  */

struct symbol *
find_symbol_in_list (struct pending *list, char *name, int length)
{
  int j;
  const char *pp;

  while (list != NULL)
    {
      for (j = list->nsyms; --j >= 0;)
	{
	  pp = list->symbol[j]->linkage_name ();
	  if (*pp == *name && strncmp (pp, name, length) == 0
	      && pp[length] == '\0')
	    {
	      return (list->symbol[j]);
	    }
	}
      list = list->next;
    }
  return (NULL);
}

/* Record BLOCK on the list of all blocks in the file.  Put it after
   OPBLOCK, or at the beginning if opblock is NULL.  This puts the
   block in the list after all its subblocks.  */

void
buildsym_compunit::record_pending_block (struct block *block,
					 struct pending_block *opblock)
{
  struct pending_block *pblock;

  pblock = XOBNEW (&m_pending_block_obstack, struct pending_block);
  pblock->block = block;
  if (opblock)
    {
      pblock->next = opblock->next;
      opblock->next = pblock;
    }
  else
    {
      pblock->next = m_pending_blocks;
      m_pending_blocks = pblock;
    }
}

/* Take one of the lists of symbols and make a block from it.  Keep
   the order the symbols have in the list (reversed from the input
   file).  Put the block on the list of pending blocks.  */

struct block *
buildsym_compunit::finish_block_internal
    (struct symbol *symbol,
     struct pending **listhead,
     struct pending_block *old_blocks,
     const struct dynamic_prop *static_link,
     CORE_ADDR start, CORE_ADDR end,
     int is_global, int expandable)
{
  struct gdbarch *gdbarch = m_objfile->arch ();
  struct pending *next, *next1;
  struct block *block;
  struct pending_block *pblock;
  struct pending_block *opblock;

  if (is_global)
    block = new (&m_objfile->objfile_obstack) global_block;
  else
    block = new (&m_objfile->objfile_obstack) struct block;

  if (symbol)
    {
      block->set_multidict
	(mdict_create_linear (&m_objfile->objfile_obstack, *listhead));
    }
  else
    {
      if (expandable)
	{
	  block->set_multidict
	    (mdict_create_hashed_expandable (m_language));
	  mdict_add_pending (block->multidict (), *listhead);
	}
      else
	{
	  block->set_multidict
	    (mdict_create_hashed (&m_objfile->objfile_obstack, *listhead));
	}
    }

  block->set_start (start);
  block->set_end (end);

  /* Put the block in as the value of the symbol that names it.  */

  if (symbol)
    {
      struct type *ftype = symbol->type ();
      symbol->set_value_block (block);
      symbol->set_section_index (SECT_OFF_TEXT (m_objfile));
      block->set_function (symbol);

      if (ftype->num_fields () <= 0)
	{
	  /* No parameter type information is recorded with the
	     function's type.  Set that from the type of the
	     parameter symbols.  */
	  int nparams = 0, iparams;

	  /* Here we want to directly access the dictionary, because
	     we haven't fully initialized the block yet.  */
	  for (struct symbol *sym : block->multidict_symbols ())
	    {
	      if (sym->is_argument ())
		nparams++;
	    }
	  if (nparams > 0)
	    {
	      ftype->alloc_fields (nparams);

	      iparams = 0;
	      /* Here we want to directly access the dictionary, because
		 we haven't fully initialized the block yet.  */
	      for (struct symbol *sym : block->multidict_symbols ())
		{
		  if (iparams == nparams)
		    break;

		  if (sym->is_argument ())
		    {
		      ftype->field (iparams).set_type (sym->type ());
		      ftype->field (iparams).set_is_artificial (false);
		      iparams++;
		    }
		}
	    }
	}
    }
  else
    block->set_function (nullptr);

  if (static_link != NULL)
    objfile_register_static_link (m_objfile, block, static_link);

  /* Now free the links of the list, and empty the list.  */

  for (next = *listhead; next; next = next1)
    {
      next1 = next->next;
      xfree (next);
    }
  *listhead = NULL;

  /* Check to be sure that the blocks have an end address that is
     greater than starting address.  */

  if (block->end () < block->start ())
    {
      if (symbol)
	{
	  complaint (_("block end address less than block "
		       "start address in %s (patched it)"),
		     symbol->print_name ());
	}
      else
	{
	  complaint (_("block end address %s less than block "
		       "start address %s (patched it)"),
		     paddress (gdbarch, block->end ()),
		     paddress (gdbarch, block->start ()));
	}
      /* Better than nothing.  */
      block->set_end (block->start ());
    }

  /* Install this block as the superblock of all blocks made since the
     start of this scope that don't have superblocks yet.  */

  opblock = NULL;
  for (pblock = m_pending_blocks;
       pblock && pblock != old_blocks; 
       pblock = pblock->next)
    {
      if (pblock->block->superblock () == NULL)
	{
	  /* Check to be sure the blocks are nested as we receive
	     them.  If the compiler/assembler/linker work, this just
	     burns a small amount of time.

	     Skip blocks which correspond to a function; they're not
	     physically nested inside this other blocks, only
	     lexically nested.  */
	  if (pblock->block->function () == NULL
	      && (pblock->block->start () < block->start ()
		  || pblock->block->end () > block->end ()))
	    {
	      if (symbol)
		{
		  complaint (_("inner block not inside outer block in %s"),
			     symbol->print_name ());
		}
	      else
		{
		  complaint (_("inner block (%s-%s) not "
			       "inside outer block (%s-%s)"),
			     paddress (gdbarch, pblock->block->start ()),
			     paddress (gdbarch, pblock->block->end ()),
			     paddress (gdbarch, block->start ()),
			     paddress (gdbarch, block->end ()));
		}

	      if (pblock->block->start () < block->start ())
		pblock->block->set_start (block->start ());

	      if (pblock->block->end () > block->end ())
		pblock->block->set_end (block->end ());
	    }
	  pblock->block->set_superblock (block);
	}
      opblock = pblock;
    }

  block->set_using ((is_global
		     ? m_global_using_directives
		     : m_local_using_directives),
		    &m_objfile->objfile_obstack);
  if (is_global)
    m_global_using_directives = NULL;
  else
    m_local_using_directives = NULL;

  record_pending_block (block, opblock);

  return block;
}

struct block *
buildsym_compunit::finish_block (struct symbol *symbol,
				 struct pending_block *old_blocks,
				 const struct dynamic_prop *static_link,
				 CORE_ADDR start, CORE_ADDR end)
{
  return finish_block_internal (symbol, &m_local_symbols,
				old_blocks, static_link, start, end, 0, 0);
}

/* Record that the range of addresses from START to END_INCLUSIVE
   (inclusive, like it says) belongs to BLOCK.  BLOCK's start and end
   addresses must be set already.  You must apply this function to all
   BLOCK's children before applying it to BLOCK.

   If a call to this function complicates the picture beyond that
   already provided by BLOCK_START and BLOCK_END, then we create an
   address map for the block.  */
void
buildsym_compunit::record_block_range (struct block *block,
				       CORE_ADDR start,
				       CORE_ADDR end_inclusive)
{
  /* If this is any different from the range recorded in the block's
     own BLOCK_START and BLOCK_END, then note that the address map has
     become interesting.  Note that even if this block doesn't have
     any "interesting" ranges, some later block might, so we still
     need to record this block in the addrmap.  */
  if (start != block->start ()
      || end_inclusive + 1 != block->end ())
    m_pending_addrmap_interesting = true;

  m_pending_addrmap.set_empty (start, end_inclusive, block);
}

struct blockvector *
buildsym_compunit::make_blockvector ()
{
  struct pending_block *next;
  struct blockvector *blockvector;
  int i;

  /* Count the length of the list of blocks.  */

  for (next = m_pending_blocks, i = 0; next; next = next->next, i++)
    {
    }

  blockvector = (struct blockvector *)
    obstack_alloc (&m_objfile->objfile_obstack,
		   (sizeof (struct blockvector)
		    + (i - 1) * sizeof (struct block *)));

  /* Copy the blocks into the blockvector.  This is done in reverse
     order, which happens to put the blocks into the proper order
     (ascending starting address).  finish_block has hair to insert
     each block into the list after its subblocks in order to make
     sure this is true.  */

  blockvector->set_num_blocks (i);
  for (next = m_pending_blocks; next; next = next->next)
    blockvector->set_block (--i, next->block);

  free_pending_blocks ();

  /* If we needed an address map for this symtab, record it in the
     blockvector.  */
  if (m_pending_addrmap_interesting)
    blockvector->set_map
      (new (&m_objfile->objfile_obstack) addrmap_fixed
       (&m_objfile->objfile_obstack, &m_pending_addrmap));
  else
    blockvector->set_map (nullptr);

  /* Some compilers output blocks in the wrong order, but we depend on
     their being in the right order so we can binary search.  Check the
     order and moan about it.
     Note: Remember that the first two blocks are the global and static
     blocks.  We could special case that fact and begin checking at block 2.
     To avoid making that assumption we do not.  */
  if (blockvector->num_blocks () > 1)
    {
      for (i = 1; i < blockvector->num_blocks (); i++)
	{
	  if (blockvector->block (i - 1)->start ()
	      > blockvector->block (i)->start ())
	    {
	      CORE_ADDR start
		= blockvector->block (i)->start ();

	      complaint (_("block at %s out of order"),
			 hex_string ((LONGEST) start));
	    }
	}
    }

  return (blockvector);
}

/* See buildsym.h.  */

void
buildsym_compunit::start_subfile (const char *name, const char *name_for_id)
{
  /* See if this subfile is already registered.  */

  symtab_create_debug_printf ("name = %s, name_for_id = %s", name, name_for_id);

  for (subfile *subfile = m_subfiles; subfile; subfile = subfile->next)
    if (FILENAME_CMP (subfile->name_for_id.c_str (), name_for_id) == 0)
      {
	symtab_create_debug_printf ("found existing symtab with name_for_id %s",
				    subfile->name_for_id.c_str ());
	m_current_subfile = subfile;
	return;
      }

  /* This subfile is not known.  Add an entry for it.  */

  subfile_up subfile (new struct subfile);
  subfile->name = name;
  subfile->name_for_id = name_for_id;

  m_current_subfile = subfile.get ();

  /* Default the source language to whatever can be deduced from the
     filename.  If nothing can be deduced (such as for a C/C++ include
     file with a ".h" extension), then inherit whatever language the
     previous subfile had.  This kludgery is necessary because there
     is no standard way in some object formats to record the source
     language.  Also, when symtabs are allocated we try to deduce a
     language then as well, but it is too late for us to use that
     information while reading symbols, since symtabs aren't allocated
     until after all the symbols have been processed for a given
     source file.  */

  subfile->language = deduce_language_from_filename (subfile->name.c_str ());
  if (subfile->language == language_unknown && m_subfiles != nullptr)
    subfile->language = m_subfiles->language;

  /* If the filename of this subfile ends in .C, then change the
     language of any pending subfiles from C to C++.  We also accept
     any other C++ suffixes accepted by deduce_language_from_filename.  */
  /* Likewise for f2c.  */

  if (!subfile->name.empty ())
    {
      struct subfile *s;
      language sublang = deduce_language_from_filename (subfile->name.c_str ());

      if (sublang == language_cplus || sublang == language_fortran)
	for (s = m_subfiles; s != NULL; s = s->next)
	  if (s->language == language_c)
	    s->language = sublang;
    }

  /* And patch up this file if necessary.  */
  if (subfile->language == language_c
      && m_subfiles != nullptr
      && (m_subfiles->language == language_cplus
	  || m_subfiles->language == language_fortran))
    subfile->language = m_subfiles->language;

  /* Link this subfile at the front of the subfile list.  */
  subfile->next = m_subfiles;
  m_subfiles = subfile.release ();
}

/* For stabs readers, the first N_SO symbol is assumed to be the
   source file name, and the subfile struct is initialized using that
   assumption.  If another N_SO symbol is later seen, immediately
   following the first one, then the first one is assumed to be the
   directory name and the second one is really the source file name.

   So we have to patch up the subfile struct by moving the old name
   value to dirname and remembering the new name.  Some sanity
   checking is performed to ensure that the state of the subfile
   struct is reasonable and that the old name we are assuming to be a
   directory name actually is (by checking for a trailing '/').  */

void
buildsym_compunit::patch_subfile_names (struct subfile *subfile,
					const char *name)
{
  if (subfile != NULL
      && m_comp_dir.empty ()
      && !subfile->name.empty ()
      && IS_DIR_SEPARATOR (subfile->name.back ()))
    {
      m_comp_dir = std::move (subfile->name);
      subfile->name = name;
      subfile->name_for_id = name;
      set_last_source_file (name);

      /* Default the source language to whatever can be deduced from
	 the filename.  If nothing can be deduced (such as for a C/C++
	 include file with a ".h" extension), then inherit whatever
	 language the previous subfile had.  This kludgery is
	 necessary because there is no standard way in some object
	 formats to record the source language.  Also, when symtabs
	 are allocated we try to deduce a language then as well, but
	 it is too late for us to use that information while reading
	 symbols, since symtabs aren't allocated until after all the
	 symbols have been processed for a given source file.  */

      subfile->language
	= deduce_language_from_filename (subfile->name.c_str ());
      if (subfile->language == language_unknown
	  && subfile->next != NULL)
	{
	  subfile->language = subfile->next->language;
	}
    }
}

/* Handle the N_BINCL and N_EINCL symbol types that act like N_SOL for
   switching source files (different subfiles, as we call them) within
   one object file, but using a stack rather than in an arbitrary
   order.  */

void
buildsym_compunit::push_subfile ()
{
  gdb_assert (m_current_subfile != NULL);
  gdb_assert (!m_current_subfile->name.empty ());
  m_subfile_stack.push_back (m_current_subfile->name.c_str ());
}

const char *
buildsym_compunit::pop_subfile ()
{
  gdb_assert (!m_subfile_stack.empty ());
  const char *name = m_subfile_stack.back ();
  m_subfile_stack.pop_back ();
  return name;
}

/* Add a linetable entry for line number LINE and address PC to the
   line vector for SUBFILE.  */

void
buildsym_compunit::record_line (struct subfile *subfile, int line,
				unrelocated_addr pc, linetable_entry_flags flags)
{
  m_have_line_numbers = true;

  /* Normally, we treat lines as unsorted.  But the end of sequence
     marker is special.  We sort line markers at the same PC by line
     number, so end of sequence markers (which have line == 0) appear
     first.  This is right if the marker ends the previous function,
     and there is no padding before the next function.  But it is
     wrong if the previous line was empty and we are now marking a
     switch to a different subfile.  We must leave the end of sequence
     marker at the end of this group of lines, not sort the empty line
     to after the marker.  The easiest way to accomplish this is to
     delete any empty lines from our table, if they are followed by
     end of sequence markers.  All we lose is the ability to set
     breakpoints at some lines which contain no instructions
     anyway.  */
  if (line == 0)
    {
      std::optional<int> last_line;

      while (!subfile->line_vector_entries.empty ())
	{
	  linetable_entry *last = &subfile->line_vector_entries.back ();
	  last_line = last->line;

	  if (last->unrelocated_pc () != pc)
	    break;

	  subfile->line_vector_entries.pop_back ();
	}

      /* Ignore an end-of-sequence marker marking an empty sequence.  */
      if (!last_line.has_value () || *last_line == 0)
	return;
    }

  subfile->line_vector_entries.emplace_back ();
  linetable_entry &e = subfile->line_vector_entries.back ();
  e.line = line;
  e.is_stmt = (flags & LEF_IS_STMT) != 0;
  e.set_unrelocated_pc (pc);
  e.prologue_end = (flags & LEF_PROLOGUE_END) != 0;
  e.epilogue_begin = (flags & LEF_EPILOGUE_BEGIN) != 0;
}


/* Subroutine of end_compunit_symtab to simplify it.  Look for a subfile that
   matches the main source file's basename.  If there is only one, and
   if the main source file doesn't have any symbol or line number
   information, then copy this file's symtab and line_vector to the
   main source file's subfile and discard the other subfile.  This can
   happen because of a compiler bug or from the user playing games
   with #line or from things like a distributed build system that
   manipulates the debug info.  This can also happen from an innocent
   symlink in the paths, we don't canonicalize paths here.  */

void
buildsym_compunit::watch_main_source_file_lossage ()
{
  struct subfile *mainsub, *subfile;

  /* Get the main source file.  */
  mainsub = m_main_subfile;

  /* If the main source file doesn't have any line number or symbol
     info, look for an alias in another subfile.  */

  if (mainsub->line_vector_entries.empty ()
      && mainsub->symtab == NULL)
    {
      const char *mainbase = lbasename (mainsub->name.c_str ());
      int nr_matches = 0;
      struct subfile *prevsub;
      struct subfile *mainsub_alias = NULL;
      struct subfile *prev_mainsub_alias = NULL;

      prevsub = NULL;
      for (subfile = m_subfiles;
	   subfile != NULL;
	   subfile = subfile->next)
	{
	  if (subfile == mainsub)
	    continue;
	  if (filename_cmp (lbasename (subfile->name.c_str ()), mainbase) == 0)
	    {
	      ++nr_matches;
	      mainsub_alias = subfile;
	      prev_mainsub_alias = prevsub;
	    }
	  prevsub = subfile;
	}

      if (nr_matches == 1)
	{
	  gdb_assert (mainsub_alias != NULL && mainsub_alias != mainsub);

	  /* Found a match for the main source file.
	     Copy its line_vector and symtab to the main subfile
	     and then discard it.  */

	  symtab_create_debug_printf ("using subfile %s as the main subfile",
				      mainsub_alias->name.c_str ());

	  mainsub->line_vector_entries
	    = std::move (mainsub_alias->line_vector_entries);
	  mainsub->symtab = mainsub_alias->symtab;

	  if (prev_mainsub_alias == NULL)
	    m_subfiles = mainsub_alias->next;
	  else
	    prev_mainsub_alias->next = mainsub_alias->next;

	  delete mainsub_alias;
	}
    }
}

/* Implementation of the first part of end_compunit_symtab.  It allows modifying
   STATIC_BLOCK before it gets finalized by
   end_compunit_symtab_from_static_block.  If the returned value is NULL there
   is no blockvector created for this symtab (you still must call
   end_compunit_symtab_from_static_block).

   END_ADDR is the same as for end_compunit_symtab: the address of the end of
   the file's text.

   If EXPANDABLE is non-zero the STATIC_BLOCK dictionary is made
   expandable.

   If REQUIRED is non-zero, then a symtab is created even if it does
   not contain any symbols.  */

struct block *
buildsym_compunit::end_compunit_symtab_get_static_block (CORE_ADDR end_addr,
							 int expandable,
							 int required)
{
  /* Finish the lexical context of the last function in the file; pop
     the context stack.  */

  if (!m_context_stack.empty ())
    {
      struct context_stack cstk = pop_context ();

      /* Make a block for the local symbols within.  */
      finish_block (cstk.name, cstk.old_blocks, NULL,
		    cstk.start_addr, end_addr);

      if (!m_context_stack.empty ())
	{
	  /* This is said to happen with SCO.  The old coffread.c
	     code simply emptied the context stack, so we do the
	     same.  FIXME: Find out why it is happening.  This is not
	     believed to happen in most cases (even for coffread.c);
	     it used to be an abort().  */
	  complaint (_("Context stack not empty in end_compunit_symtab"));
	  m_context_stack.clear ();
	}
    }

  /* Executables may have out of order pending blocks; sort the
     pending blocks.  */
  if (m_pending_blocks != nullptr)
    {
      struct pending_block *pb;

      std::vector<block *> barray;

      for (pb = m_pending_blocks; pb != NULL; pb = pb->next)
	barray.push_back (pb->block);

      /* Sort blocks by start address in descending order.  Blocks with the
	 same start address must remain in the original order to preserve
	 inline function caller/callee relationships.  */
      std::stable_sort (barray.begin (), barray.end (),
			[] (const block *a, const block *b)
			{
			  return a->start () > b->start ();
			});

      int i = 0;
      for (pb = m_pending_blocks; pb != NULL; pb = pb->next)
	pb->block = barray[i++];
    }

  /* Cleanup any undefined types that have been left hanging around
     (this needs to be done before the finish_blocks so that
     file_symbols is still good).

     Both cleanup_undefined_stabs_types and finish_global_stabs are stabs
     specific, but harmless for other symbol readers, since on gdb
     startup or when finished reading stabs, the state is set so these
     are no-ops.  FIXME: Is this handled right in case of QUIT?  Can
     we make this cleaner?  */

  cleanup_undefined_stabs_types (m_objfile);
  finish_global_stabs (m_objfile);

  if (!required
      && m_pending_blocks == NULL
      && m_file_symbols == NULL
      && m_global_symbols == NULL
      && !m_have_line_numbers
      && m_pending_macros == NULL
      && m_global_using_directives == NULL)
    {
      /* Ignore symtabs that have no functions with real debugging info.  */
      return NULL;
    }
  else
    {
      /* Define the STATIC_BLOCK.  */
      return finish_block_internal (NULL, get_file_symbols (), NULL, NULL,
				    m_last_source_start_addr,
				    end_addr, 0, expandable);
    }
}

/* Subroutine of end_compunit_symtab_from_static_block to simplify it.
   Handle the "have blockvector" case.
   See end_compunit_symtab_from_static_block for a description of the
   arguments.  */

struct compunit_symtab *
buildsym_compunit::end_compunit_symtab_with_blockvector
  (struct block *static_block, int expandable)
{
  struct compunit_symtab *cu = m_compunit_symtab;
  struct blockvector *blockvector;
  struct subfile *subfile;
  CORE_ADDR end_addr;

  gdb_assert (static_block != NULL);
  gdb_assert (m_subfiles != NULL);

  end_addr = static_block->end ();

  /* Create the GLOBAL_BLOCK and build the blockvector.  */
  finish_block_internal (NULL, get_global_symbols (), NULL, NULL,
			 m_last_source_start_addr, end_addr,
			 1, expandable);
  blockvector = make_blockvector ();

  /* Read the line table if it has to be read separately.
     This is only used by xcoffread.c.  */
  if (m_objfile->sf->sym_read_linetable != NULL)
    m_objfile->sf->sym_read_linetable (m_objfile);

  /* Handle the case where the debug info specifies a different path
     for the main source file.  It can cause us to lose track of its
     line number information.  */
  watch_main_source_file_lossage ();

  /* Now create the symtab objects proper, if not already done,
     one for each subfile.  */

  for (subfile = m_subfiles;
       subfile != NULL;
       subfile = subfile->next)
    {
      if (!subfile->line_vector_entries.empty ())
	{
	  /* Like the pending blocks, the line table may be scrambled
	     in reordered executables.  Sort it.  It is important to
	     preserve the order of lines at the same address, as this
	     maintains the inline function caller/callee
	     relationships, this is why std::stable_sort is used.  */
	  std::stable_sort (subfile->line_vector_entries.begin (),
			    subfile->line_vector_entries.end ());
	}

      /* Allocate a symbol table if necessary.  */
      if (subfile->symtab == NULL)
	subfile->symtab = allocate_symtab (cu, subfile->name.c_str (),
					   subfile->name_for_id.c_str ());

      struct symtab *symtab = subfile->symtab;

      /* Fill in its components.  */

      if (!subfile->line_vector_entries.empty ())
	{
	  /* Reallocate the line table on the objfile obstack.  */
	  size_t n_entries = subfile->line_vector_entries.size ();
	  size_t entry_array_size = n_entries * sizeof (struct linetable_entry);
	  int linetablesize = sizeof (struct linetable) + entry_array_size;

	  struct linetable *new_table
	    = XOBNEWVAR (&m_objfile->objfile_obstack, struct linetable,
			 linetablesize);

	  new_table->nitems = n_entries;
	  memcpy (new_table->item,
		  subfile->line_vector_entries.data (), entry_array_size);

	  symtab->set_linetable (new_table);
	}
      else
	symtab->set_linetable (nullptr);

      /* Use whatever language we have been using for this
	 subfile, not the one that was deduced in allocate_symtab
	 from the filename.  We already did our own deducing when
	 we created the subfile, and we may have altered our
	 opinion of what language it is from things we found in
	 the symbols.  */
      symtab->set_language (subfile->language);
    }

  /* Make sure the filetab of main_subfile is the primary filetab of the CU.  */
  cu->set_primary_filetab (m_main_subfile->symtab);

  /* Fill out the compunit symtab.  */

  if (!m_comp_dir.empty ())
    {
      /* Reallocate the dirname on the symbol obstack.  */
      cu->set_dirname (obstack_strdup (&m_objfile->objfile_obstack,
				       m_comp_dir.c_str ()));
    }

  /* Save the debug format string (if any) in the symtab.  */
  cu->set_debugformat (m_debugformat);

  /* Similarly for the producer.  */
  cu->set_producer (m_producer);

  cu->set_blockvector (blockvector);
  {
    struct block *b = blockvector->global_block ();

    b->set_compunit_symtab (cu);
  }

  cu->set_macro_table (release_macros ());

  /* Default any symbols without a specified symtab to the primary symtab.  */
  {
    int block_i;

    /* The main source file's symtab.  */
    struct symtab *symtab = cu->primary_filetab ();

    for (block_i = 0; block_i < blockvector->num_blocks (); block_i++)
      {
	struct block *block = blockvector->block (block_i);

	/* Inlined functions may have symbols not in the global or
	   static symbol lists.  */
	if (block->function () != nullptr
	    && block->function ()->symtab () == nullptr)
	    block->function ()->set_symtab (symtab);

	/* Note that we only want to fix up symbols from the local
	   blocks, not blocks coming from included symtabs.  That is
	   why we use an mdict iterator here and not a block
	   iterator.  */
	for (struct symbol *sym : block->multidict_symbols ())
	  if (sym->symtab () == NULL)
	    sym->set_symtab (symtab);
      }
  }

  add_compunit_symtab_to_objfile (cu);

  return cu;
}

/* Implementation of the second part of end_compunit_symtab.  Pass STATIC_BLOCK
   as value returned by end_compunit_symtab_get_static_block.

   If EXPANDABLE is non-zero the GLOBAL_BLOCK dictionary is made
   expandable.  */

struct compunit_symtab *
buildsym_compunit::end_compunit_symtab_from_static_block
  (struct block *static_block, int expandable)
{
  struct compunit_symtab *cu;

  if (static_block == NULL)
    {
      /* Handle the "no blockvector" case.
	 When this happens there is nothing to record, so there's nothing
	 to do: memory will be freed up later.

	 Note: We won't be adding a compunit to the objfile's list of
	 compunits, so there's nothing to unchain.  However, since each symtab
	 is added to the objfile's obstack we can't free that space.
	 We could do better, but this is believed to be a sufficiently rare
	 event.  */
      cu = NULL;
    }
  else
    cu = end_compunit_symtab_with_blockvector (static_block, expandable);

  return cu;
}

/* Finish the symbol definitions for one main source file, close off
   all the lexical contexts for that file (creating struct block's for
   them), then make the struct symtab for that file and put it in the
   list of all such.

   END_ADDR is the address of the end of the file's text.

   Note that it is possible for end_compunit_symtab() to return NULL.  In
   particular, for the DWARF case at least, it will return NULL when
   it finds a compilation unit that has exactly one DIE, a
   TAG_compile_unit DIE.  This can happen when we link in an object
   file that was compiled from an empty source file.  Returning NULL
   is probably not the correct thing to do, because then gdb will
   never know about this empty file (FIXME).

   If you need to modify STATIC_BLOCK before it is finalized you should
   call end_compunit_symtab_get_static_block and
   end_compunit_symtab_from_static_block yourself.  */

struct compunit_symtab *
buildsym_compunit::end_compunit_symtab (CORE_ADDR end_addr)
{
  struct block *static_block;

  static_block = end_compunit_symtab_get_static_block (end_addr, 0, 0);
  return end_compunit_symtab_from_static_block (static_block, 0);
}

/* Same as end_compunit_symtab except create a symtab that can be later added
   to.  */

struct compunit_symtab *
buildsym_compunit::end_expandable_symtab (CORE_ADDR end_addr)
{
  struct block *static_block;

  static_block = end_compunit_symtab_get_static_block (end_addr, 1, 0);
  return end_compunit_symtab_from_static_block (static_block, 1);
}

/* Subroutine of augment_type_symtab to simplify it.
   Attach the main source file's symtab to all symbols in PENDING_LIST that
   don't have one.  */

static void
set_missing_symtab (struct pending *pending_list,
		    struct compunit_symtab *cu)
{
  struct pending *pending;
  int i;

  for (pending = pending_list; pending != NULL; pending = pending->next)
    {
      for (i = 0; i < pending->nsyms; ++i)
	{
	  if (pending->symbol[i]->symtab () == NULL)
	    pending->symbol[i]->set_symtab (cu->primary_filetab ());
	}
    }
}

/* Same as end_compunit_symtab, but for the case where we're adding more symbols
   to an existing symtab that is known to contain only type information.
   This is the case for DWARF4 Type Units.  */

void
buildsym_compunit::augment_type_symtab ()
{
  struct compunit_symtab *cust = m_compunit_symtab;
  struct blockvector *blockvector = cust->blockvector ();

  if (!m_context_stack.empty ())
    complaint (_("Context stack not empty in augment_type_symtab"));
  if (m_pending_blocks != NULL)
    complaint (_("Blocks in a type symtab"));
  if (m_pending_macros != NULL)
    complaint (_("Macro in a type symtab"));
  if (m_have_line_numbers)
    complaint (_("Line numbers recorded in a type symtab"));

  if (m_file_symbols != NULL)
    {
      struct block *block = blockvector->static_block ();

      /* First mark any symbols without a specified symtab as belonging
	 to the primary symtab.  */
      set_missing_symtab (m_file_symbols, cust);

      mdict_add_pending (block->multidict (), m_file_symbols);
    }

  if (m_global_symbols != NULL)
    {
      struct block *block = blockvector->global_block ();

      /* First mark any symbols without a specified symtab as belonging
	 to the primary symtab.  */
      set_missing_symtab (m_global_symbols, cust);

      mdict_add_pending (block->multidict (), m_global_symbols);
    }
}

/* Push a context block.  Args are an identifying nesting level
   (checkable when you pop it), and the starting PC address of this
   context.  */

struct context_stack *
buildsym_compunit::push_context (int desc, CORE_ADDR valu)
{
  m_context_stack.emplace_back ();
  struct context_stack *newobj = &m_context_stack.back ();

  newobj->depth = desc;
  newobj->locals = m_local_symbols;
  newobj->old_blocks = m_pending_blocks;
  newobj->start_addr = valu;
  newobj->local_using_directives = m_local_using_directives;
  newobj->name = NULL;

  m_local_symbols = NULL;
  m_local_using_directives = NULL;

  return newobj;
}

/* Pop a context block.  Returns the address of the context block just
   popped.  */

struct context_stack
buildsym_compunit::pop_context ()
{
  gdb_assert (!m_context_stack.empty ());
  struct context_stack result = m_context_stack.back ();
  m_context_stack.pop_back ();
  return result;
}
