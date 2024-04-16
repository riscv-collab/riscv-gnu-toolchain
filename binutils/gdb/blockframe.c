/* Get info from stack frames; convert between frames, blocks,
   functions and pc values.

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
#include "symtab.h"
#include "bfd.h"
#include "objfiles.h"
#include "frame.h"
#include "gdbcore.h"
#include "value.h"
#include "target.h"
#include "inferior.h"
#include "annotate.h"
#include "regcache.h"
#include "dummy-frame.h"
#include "command.h"
#include "gdbcmd.h"
#include "block.h"
#include "inline-frame.h"

/* Return the innermost lexical block in execution in a specified
   stack frame.  The frame address is assumed valid.

   If ADDR_IN_BLOCK is non-zero, set *ADDR_IN_BLOCK to the exact code
   address we used to choose the block.  We use this to find a source
   line, to decide which macro definitions are in scope.

   The value returned in *ADDR_IN_BLOCK isn't necessarily the frame's
   PC, and may not really be a valid PC at all.  For example, in the
   caller of a function declared to never return, the code at the
   return address will never be reached, so the call instruction may
   be the very last instruction in the block.  So the address we use
   to choose the block is actually one byte before the return address
   --- hopefully pointing us at the call instruction, or its delay
   slot instruction.  */

const struct block *
get_frame_block (frame_info_ptr frame, CORE_ADDR *addr_in_block)
{
  CORE_ADDR pc;
  const struct block *bl;
  int inline_count;

  if (!get_frame_address_in_block_if_available (frame, &pc))
    return NULL;

  if (addr_in_block)
    *addr_in_block = pc;

  bl = block_for_pc (pc);
  if (bl == NULL)
    return NULL;

  inline_count = frame_inlined_callees (frame);

  while (inline_count > 0)
    {
      if (bl->inlined_p ())
	inline_count--;

      bl = bl->superblock ();
      gdb_assert (bl != NULL);
    }

  return bl;
}

CORE_ADDR
get_pc_function_start (CORE_ADDR pc)
{
  const struct block *bl;
  struct bound_minimal_symbol msymbol;

  bl = block_for_pc (pc);
  if (bl)
    {
      struct symbol *symbol = bl->linkage_function ();

      if (symbol)
	{
	  bl = symbol->value_block ();
	  return bl->entry_pc ();
	}
    }

  msymbol = lookup_minimal_symbol_by_pc (pc);
  if (msymbol.minsym)
    {
      CORE_ADDR fstart = msymbol.value_address ();

      if (find_pc_section (fstart))
	return fstart;
    }

  return 0;
}

/* Return the symbol for the function executing in frame FRAME.  */

struct symbol *
get_frame_function (frame_info_ptr frame)
{
  const struct block *bl = get_frame_block (frame, 0);

  if (bl == NULL)
    return NULL;

  while (bl->function () == NULL && bl->superblock () != NULL)
    bl = bl->superblock ();

  return bl->function ();
}


/* Return the function containing pc value PC in section SECTION.
   Returns 0 if function is not known.  */

struct symbol *
find_pc_sect_function (CORE_ADDR pc, struct obj_section *section)
{
  const struct block *b = block_for_pc_sect (pc, section);

  if (b == 0)
    return 0;
  return b->linkage_function ();
}

/* Return the function containing pc value PC.
   Returns 0 if function is not known.  
   Backward compatibility, no section */

struct symbol *
find_pc_function (CORE_ADDR pc)
{
  return find_pc_sect_function (pc, find_pc_mapped_section (pc));
}

/* See symtab.h.  */

struct symbol *
find_pc_sect_containing_function (CORE_ADDR pc, struct obj_section *section)
{
  const block *bl = block_for_pc_sect (pc, section);

  if (bl == nullptr)
    return nullptr;

  return bl->containing_function ();
}

/* These variables are used to cache the most recent result of
   find_pc_partial_function.

   The addresses cache_pc_function_low and cache_pc_function_high
   record the range in which PC was found during the most recent
   successful lookup.  When the function occupies a single contiguous
   address range, these values correspond to the low and high
   addresses of the function.  (The high address is actually one byte
   beyond the last byte of the function.)  For a function with more
   than one (non-contiguous) range, the range in which PC was found is
   used to set the cache bounds.

   When determining whether or not these cached values apply to a
   particular PC value, PC must be within the range specified by
   cache_pc_function_low and cache_pc_function_high.  In addition to
   PC being in that range, cache_pc_section must also match PC's
   section.  See find_pc_partial_function() for details on both the
   comparison as well as how PC's section is determined.

   The other values aren't used for determining whether the cache
   applies, but are used for setting the outputs from
   find_pc_partial_function.  cache_pc_function_low and
   cache_pc_function_high are used to set outputs as well.  */

static CORE_ADDR cache_pc_function_low = 0;
static CORE_ADDR cache_pc_function_high = 0;
static const general_symbol_info *cache_pc_function_sym = nullptr;
static struct obj_section *cache_pc_function_section = NULL;
static const struct block *cache_pc_function_block = nullptr;

/* Clear cache, e.g. when symbol table is discarded.  */

void
clear_pc_function_cache (void)
{
  cache_pc_function_low = 0;
  cache_pc_function_high = 0;
  cache_pc_function_sym = nullptr;
  cache_pc_function_section = NULL;
  cache_pc_function_block = nullptr;
}

/* See symtab.h.  */

bool
find_pc_partial_function_sym (CORE_ADDR pc,
			      const struct general_symbol_info **sym,
			      CORE_ADDR *address, CORE_ADDR *endaddr,
			      const struct block **block)
{
  struct obj_section *section;
  struct symbol *f;
  struct bound_minimal_symbol msymbol;
  struct compunit_symtab *compunit_symtab = NULL;
  CORE_ADDR mapped_pc;

  /* To ensure that the symbol returned belongs to the correct section
     (and that the last [random] symbol from the previous section
     isn't returned) try to find the section containing PC.  First try
     the overlay code (which by default returns NULL); and second try
     the normal section code (which almost always succeeds).  */
  section = find_pc_overlay (pc);
  if (section == NULL)
    section = find_pc_section (pc);

  mapped_pc = overlay_mapped_address (pc, section);

  if (mapped_pc >= cache_pc_function_low
      && mapped_pc < cache_pc_function_high
      && section == cache_pc_function_section)
    goto return_cached_value;

  msymbol = lookup_minimal_symbol_by_pc_section (mapped_pc, section);
  compunit_symtab = find_pc_sect_compunit_symtab (mapped_pc, section);

  if (compunit_symtab != NULL)
    {
      /* Checking whether the msymbol has a larger value is for the
	 "pathological" case mentioned in stack.c:find_frame_funname.

	 We use BLOCK_ENTRY_PC instead of BLOCK_START_PC for this
	 comparison because the minimal symbol should refer to the
	 function's entry pc which is not necessarily the lowest
	 address of the function.  This will happen when the function
	 has more than one range and the entry pc is not within the
	 lowest range of addresses.  */
      f = find_pc_sect_function (mapped_pc, section);
      if (f != NULL
	  && (msymbol.minsym == NULL
	      || (f->value_block ()->entry_pc ()
		  >= msymbol.value_address ())))
	{
	  const struct block *b = f->value_block ();

	  cache_pc_function_sym = f;
	  cache_pc_function_section = section;
	  cache_pc_function_block = b;

	  /* For blocks occupying contiguous addresses (i.e. no gaps),
	     the low and high cache addresses are simply the start
	     and end of the block.

	     For blocks with non-contiguous ranges, we have to search
	     for the range containing mapped_pc and then use the start
	     and end of that range.

	     This causes the returned *ADDRESS and *ENDADDR values to
	     be limited to the range in which mapped_pc is found.  See
	     comment preceding declaration of find_pc_partial_function
	     in symtab.h for more information.  */

	  if (b->is_contiguous ())
	    {
	      cache_pc_function_low = b->start ();
	      cache_pc_function_high = b->end ();
	    }
	  else
	    {
	      bool found = false;
	      for (const blockrange &range : b->ranges ())
		{
		  if (range.start () <= mapped_pc && mapped_pc < range.end ())
		    {
		      cache_pc_function_low = range.start ();
		      cache_pc_function_high = range.end ();
		      found = true;
		      break;
		    }
		}
	      /* Above loop should exit via the break.  */
	      gdb_assert (found);
	    }


	  goto return_cached_value;
	}
    }

  /* Not in the normal symbol tables, see if the pc is in a known
     section.  If it's not, then give up.  This ensures that anything
     beyond the end of the text seg doesn't appear to be part of the
     last function in the text segment.  */

  if (!section)
    msymbol.minsym = NULL;

  /* Must be in the minimal symbol table.  */
  if (msymbol.minsym == NULL)
    {
      /* No available symbol.  */
      if (sym != nullptr)
	*sym = 0;
      if (address != NULL)
	*address = 0;
      if (endaddr != NULL)
	*endaddr = 0;
      if (block != nullptr)
	*block = nullptr;
      return false;
    }

  cache_pc_function_low = msymbol.value_address ();
  cache_pc_function_sym = msymbol.minsym;
  cache_pc_function_section = section;
  cache_pc_function_high = minimal_symbol_upper_bound (msymbol);
  cache_pc_function_block = nullptr;

 return_cached_value:

  if (address)
    {
      if (pc_in_unmapped_range (pc, section))
	*address = overlay_unmapped_address (cache_pc_function_low, section);
      else
	*address = cache_pc_function_low;
    }

  if (sym != nullptr)
    *sym = cache_pc_function_sym;

  if (endaddr)
    {
      if (pc_in_unmapped_range (pc, section))
	{
	  /* Because the high address is actually beyond the end of
	     the function (and therefore possibly beyond the end of
	     the overlay), we must actually convert (high - 1) and
	     then add one to that.  */

	  *endaddr = 1 + overlay_unmapped_address (cache_pc_function_high - 1,
						   section);
	}
      else
	*endaddr = cache_pc_function_high;
    }

  if (block != nullptr)
    *block = cache_pc_function_block;

  return true;
}

/* See symtab.h.  */

bool
find_pc_partial_function (CORE_ADDR pc, const char **name, CORE_ADDR *address,
			  CORE_ADDR *endaddr, const struct block **block)
{
  const general_symbol_info *gsi;
  bool r = find_pc_partial_function_sym (pc, &gsi, address, endaddr, block);
  if (name != nullptr)
    *name = r ? gsi->linkage_name () : nullptr;
  return r;
}


/* See symtab.h.  */

bool
find_function_entry_range_from_pc (CORE_ADDR pc, const char **name,
				   CORE_ADDR *address, CORE_ADDR *endaddr)
{
  const struct block *block;
  bool status = find_pc_partial_function (pc, name, address, endaddr, &block);

  if (status && block != nullptr && !block->is_contiguous ())
    {
      CORE_ADDR entry_pc = block->entry_pc ();

      for (const blockrange &range : block->ranges ())
	{
	  if (range.start () <= entry_pc && entry_pc < range.end ())
	    {
	      if (address != nullptr)
		*address = range.start ();

	      if (endaddr != nullptr)
		*endaddr = range.end ();

	      return status;
	    }
	}

      /* It's an internal error if we exit the above loop without finding
	 the range.  */
      internal_error (_("Entry block not found in find_function_entry_range_from_pc"));
    }

  return status;
}

/* See symtab.h.  */

struct type *
find_function_type (CORE_ADDR pc)
{
  struct symbol *sym = find_pc_function (pc);

  if (sym != NULL && sym->value_block ()->entry_pc () == pc)
    return sym->type ();

  return NULL;
}

/* See symtab.h.  */

struct type *
find_gnu_ifunc_target_type (CORE_ADDR resolver_funaddr)
{
  struct type *resolver_type = find_function_type (resolver_funaddr);
  if (resolver_type != NULL)
    {
      /* Get the return type of the resolver.  */
      struct type *resolver_ret_type
	= check_typedef (resolver_type->target_type ());

      /* If we found a pointer to function, then the resolved type
	 is the type of the pointed-to function.  */
      if (resolver_ret_type->code () == TYPE_CODE_PTR)
	{
	  struct type *resolved_type
	    = resolver_ret_type->target_type ();
	  if (check_typedef (resolved_type)->code () == TYPE_CODE_FUNC)
	    return resolved_type;
	}
    }

  return NULL;
}

/* Return the innermost stack frame that is executing inside of BLOCK and is
   at least as old as the selected frame. Return NULL if there is no
   such frame.  If BLOCK is NULL, just return NULL.  */

frame_info_ptr
block_innermost_frame (const struct block *block)
{
  if (block == NULL)
    return NULL;

  frame_info_ptr frame = get_selected_frame ();
  while (frame != NULL)
    {
      const struct block *frame_block = get_frame_block (frame, NULL);
      if (frame_block != NULL && block->contains (frame_block))
	return frame;

      frame = get_prev_frame (frame);
    }

  return NULL;
}
