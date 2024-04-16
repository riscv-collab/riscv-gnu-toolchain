/* Virtual tail call frames unwinder for GDB.

   Copyright (C) 2010-2024 Free Software Foundation, Inc.

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
#include "frame.h"
#include "dwarf2/frame-tailcall.h"
#include "dwarf2/loc.h"
#include "frame-unwind.h"
#include "block.h"
#include "hashtab.h"
#include "gdbtypes.h"
#include "regcache.h"
#include "value.h"
#include "dwarf2/frame.h"
#include "gdbarch.h"

/* Contains struct tailcall_cache indexed by next_bottom_frame.  */
static htab_t cache_htab;

/* Associate structure of the unwinder to call_site_chain.  Lifetime of this
   structure is maintained by REFC decremented by dealloc_cache, all of them
   get deleted during reinit_frame_cache.  */
struct tailcall_cache
{
  /* It must be the first one of this struct.  It is the furthest callee.  */
  frame_info *next_bottom_frame;

  /* Reference count.  The whole chain of virtual tail call frames shares one
     tailcall_cache.  */
  int refc;

  /* Associated found virtual tail call frames chain, it is never NULL.  */
  struct call_site_chain *chain;

  /* Cached pretended_chain_levels result.  */
  int chain_levels;

  /* Unwound PC from the top (caller) frame, as it is not contained
     in CHAIN.  */
  CORE_ADDR prev_pc;

  /* Compensate SP in caller frames appropriately.  prev_sp and
     entry_cfa_sp_offset are valid only if PREV_SP_P.  PREV_SP is SP at the top
     (caller) frame.  ENTRY_CFA_SP_OFFSET is shift of SP in tail call frames
     against next_bottom_frame SP.  */
  unsigned prev_sp_p : 1;
  CORE_ADDR prev_sp;
  LONGEST entry_cfa_sp_offset;
};

/* hash_f for htab_create_alloc of cache_htab.  */

static hashval_t
cache_hash (const void *arg)
{
  const struct tailcall_cache *cache = (const struct tailcall_cache *) arg;

  return htab_hash_pointer (cache->next_bottom_frame);
}

/* eq_f for htab_create_alloc of cache_htab.  */

static int
cache_eq (const void *arg1, const void *arg2)
{
  const struct tailcall_cache *cache1 = (const struct tailcall_cache *) arg1;
  const struct tailcall_cache *cache2 = (const struct tailcall_cache *) arg2;

  return cache1->next_bottom_frame == cache2->next_bottom_frame;
}

/* Create new tailcall_cache for NEXT_BOTTOM_FRAME, NEXT_BOTTOM_FRAME must not
   yet have been indexed by cache_htab.  Caller holds one reference of the new
   tailcall_cache.  */

static struct tailcall_cache *
cache_new_ref1 (frame_info_ptr next_bottom_frame)
{
  struct tailcall_cache *cache = XCNEW (struct tailcall_cache);
  void **slot;

  cache->next_bottom_frame = next_bottom_frame.get ();
  cache->refc = 1;

  slot = htab_find_slot (cache_htab, cache, INSERT);
  gdb_assert (*slot == NULL);
  *slot = cache;

  return cache;
}

/* Create new reference to CACHE.  */

static void
cache_ref (struct tailcall_cache *cache)
{
  gdb_assert (cache->refc > 0);

  cache->refc++;
}

/* Drop reference to CACHE, possibly fully freeing it and unregistering it from
   cache_htab.  */

static void
cache_unref (struct tailcall_cache *cache)
{
  gdb_assert (cache->refc > 0);

  if (!--cache->refc)
    {
      gdb_assert (htab_find_slot (cache_htab, cache, NO_INSERT) != NULL);
      htab_remove_elt (cache_htab, cache);

      xfree (cache->chain);
      xfree (cache);
    }
}

/* Return 1 if FI is a non-bottom (not the callee) tail call frame.  Otherwise
   return 0.  */

static int
frame_is_tailcall (frame_info_ptr fi)
{
  return frame_unwinder_is (fi, &dwarf2_tailcall_frame_unwind);
}

/* Try to find tailcall_cache in cache_htab if FI is a part of its virtual tail
   call chain.  Otherwise return NULL.  No new reference is created.  */

static struct tailcall_cache *
cache_find (frame_info_ptr fi)
{
  struct tailcall_cache *cache;
  struct tailcall_cache search;
  void **slot;

  while (frame_is_tailcall (fi))
    {
      fi = get_next_frame (fi);
      gdb_assert (fi != NULL);
    }

  search.next_bottom_frame = fi.get();
  search.refc = 1;
  slot = htab_find_slot (cache_htab, &search, NO_INSERT);
  if (slot == NULL)
    return NULL;

  cache = (struct tailcall_cache *) *slot;
  gdb_assert (cache != NULL);
  return cache;
}

/* Number of virtual frames between THIS_FRAME and CACHE->NEXT_BOTTOM_FRAME.
   If THIS_FRAME is CACHE-> NEXT_BOTTOM_FRAME return -1.  */

static int
existing_next_levels (frame_info_ptr this_frame,
		      struct tailcall_cache *cache)
{
  int retval = (frame_relative_level (this_frame)
		- frame_relative_level (frame_info_ptr (cache->next_bottom_frame)) - 1);

  gdb_assert (retval >= -1);

  return retval;
}

/* The number of virtual tail call frames in CHAIN.  With no virtual tail call
   frames the function would return 0 (but CHAIN does not exist in such
   case).  */

static int
pretended_chain_levels (struct call_site_chain *chain)
{
  int chain_levels;

  gdb_assert (chain != NULL);

  if (chain->callers == chain->length && chain->callees == chain->length)
    return chain->length;

  chain_levels = chain->callers + chain->callees;
  gdb_assert (chain_levels <= chain->length);

  return chain_levels;
}

/* Implementation of frame_this_id_ftype.  THIS_CACHE must be already
   initialized with tailcall_cache, THIS_FRAME must be a part of THIS_CACHE.

   Specific virtual tail call frames are tracked by INLINE_DEPTH.  */

static void
tailcall_frame_this_id (frame_info_ptr this_frame, void **this_cache,
			struct frame_id *this_id)
{
  struct tailcall_cache *cache = (struct tailcall_cache *) *this_cache;
  frame_info_ptr next_frame;

  /* Tail call does not make sense for a sentinel frame.  */
  next_frame = get_next_frame (this_frame);
  gdb_assert (next_frame != NULL);

  *this_id = get_frame_id (next_frame);
  (*this_id).code_addr = get_frame_pc (this_frame);
  (*this_id).code_addr_p = true;
  (*this_id).artificial_depth = (cache->chain_levels
				 - existing_next_levels (this_frame, cache));
  gdb_assert ((*this_id).artificial_depth > 0);
}

/* Find PC to be unwound from THIS_FRAME.  THIS_FRAME must be a part of
   CACHE.  */

static CORE_ADDR
pretend_pc (frame_info_ptr this_frame, struct tailcall_cache *cache)
{
  int next_levels = existing_next_levels (this_frame, cache);
  struct call_site_chain *chain = cache->chain;

  gdb_assert (chain != NULL);

  next_levels++;
  gdb_assert (next_levels >= 0);

  if (next_levels < chain->callees)
    return chain->call_site[chain->length - next_levels - 1]->pc ();
  next_levels -= chain->callees;

  /* Otherwise CHAIN->CALLEES are already covered by CHAIN->CALLERS.  */
  if (chain->callees != chain->length)
    {
      if (next_levels < chain->callers)
	return chain->call_site[chain->callers - next_levels - 1]->pc ();
      next_levels -= chain->callers;
    }

  gdb_assert (next_levels == 0);
  return cache->prev_pc;
}

/* Implementation of frame_prev_register_ftype.  If no specific register
   override is supplied NULL is returned (this is incompatible with
   frame_prev_register_ftype semantics).  next_bottom_frame and tail call
   frames unwind the NULL case differently.  */

struct value *
dwarf2_tailcall_prev_register_first (frame_info_ptr this_frame,
				     void **tailcall_cachep, int regnum)
{
  struct gdbarch *this_gdbarch = get_frame_arch (this_frame);
  struct tailcall_cache *cache = (struct tailcall_cache *) *tailcall_cachep;
  CORE_ADDR addr;

  if (regnum == gdbarch_pc_regnum (this_gdbarch))
    addr = pretend_pc (this_frame, cache);
  else if (cache->prev_sp_p && regnum == gdbarch_sp_regnum (this_gdbarch))
    {
      int next_levels = existing_next_levels (this_frame, cache);

      if (next_levels == cache->chain_levels - 1)
	addr = cache->prev_sp;
      else
	addr = dwarf2_frame_cfa (this_frame) - cache->entry_cfa_sp_offset;
    }
  else
    return NULL;

  return frame_unwind_got_address (this_frame, regnum, addr);
}

/* Implementation of frame_prev_register_ftype for tail call frames.  Register
   set of virtual tail call frames is assumed to be the one of the top (caller)
   frame - assume unchanged register value for NULL from
   dwarf2_tailcall_prev_register_first.  */

static struct value *
tailcall_frame_prev_register (frame_info_ptr this_frame,
			       void **this_cache, int regnum)
{
  struct tailcall_cache *cache = (struct tailcall_cache *) *this_cache;
  struct value *val;

  gdb_assert (this_frame != cache->next_bottom_frame);

  val = dwarf2_tailcall_prev_register_first (this_frame, this_cache, regnum);
  if (val)
    return val;

  return frame_unwind_got_register (this_frame, regnum, regnum);
}

/* Implementation of frame_sniffer_ftype.  It will never find a new chain, use
   dwarf2_tailcall_sniffer_first for the bottom (callee) frame.  It will find
   all the predecessing virtual tail call frames, it will return false when
   there exist no more tail call frames in this chain.  */

static int
tailcall_frame_sniffer (const struct frame_unwind *self,
			 frame_info_ptr this_frame, void **this_cache)
{
  frame_info_ptr next_frame;
  int next_levels;
  struct tailcall_cache *cache;

  if (!dwarf2_frame_unwinders_enabled_p)
    return 0;

  /* Inner tail call element does not make sense for a sentinel frame.  */
  next_frame = get_next_frame (this_frame);
  if (next_frame == NULL)
    return 0;

  cache = cache_find (next_frame);
  if (cache == NULL)
    return 0;

  cache_ref (cache);

  next_levels = existing_next_levels (this_frame, cache);

  /* NEXT_LEVELS is -1 only in dwarf2_tailcall_sniffer_first.  */
  gdb_assert (next_levels >= 0);
  gdb_assert (next_levels <= cache->chain_levels);

  if (next_levels == cache->chain_levels)
    {
      cache_unref (cache);
      return 0;
    }

  *this_cache = cache;
  return 1;
}

/* The initial "sniffer" whether THIS_FRAME is a bottom (callee) frame of a new
   chain to create.  Keep TAILCALL_CACHEP NULL if it did not find any chain,
   initialize it otherwise.  No tail call chain is created if there are no
   unambiguous virtual tail call frames to report.
   
   ENTRY_CFA_SP_OFFSETP is NULL if no special SP handling is possible,
   otherwise *ENTRY_CFA_SP_OFFSETP is the number of bytes to subtract from tail
   call frames frame base to get the SP value there - to simulate return
   address pushed on the stack.  */

void
dwarf2_tailcall_sniffer_first (frame_info_ptr this_frame,
			       void **tailcall_cachep,
			       const LONGEST *entry_cfa_sp_offsetp)
{
  CORE_ADDR prev_pc = 0, prev_sp = 0;	/* GCC warning.  */
  int prev_sp_p = 0;
  CORE_ADDR this_pc;
  struct gdbarch *prev_gdbarch;
  gdb::unique_xmalloc_ptr<call_site_chain> chain;
  struct tailcall_cache *cache;

  gdb_assert (*tailcall_cachep == NULL);

  /* PC may be after the function if THIS_FRAME calls noreturn function,
     get_frame_address_in_block will decrease it by 1 in such case.  */
  this_pc = get_frame_address_in_block (this_frame);

  try
    {
      int sp_regnum;

      prev_gdbarch = frame_unwind_arch (this_frame);

      /* Simulate frame_unwind_pc without setting this_frame->prev_pc.p.  */
      prev_pc = gdbarch_unwind_pc (prev_gdbarch, this_frame);

      /* call_site_find_chain can throw an exception.  */
      chain = call_site_find_chain (prev_gdbarch, prev_pc, this_pc);

      if (entry_cfa_sp_offsetp != NULL)
	{
	  sp_regnum = gdbarch_sp_regnum (prev_gdbarch);
	  if (sp_regnum != -1)
	    {
	      prev_sp = frame_unwind_register_unsigned (this_frame, sp_regnum);
	      prev_sp_p = 1;
	    }
	}
    }
  catch (const gdb_exception_error &except)
    {
      if (entry_values_debug)
	exception_print (gdb_stdout, except);

      switch (except.error)
	{
	case NO_ENTRY_VALUE_ERROR:
	  /* Thrown by call_site_find_chain.  */
	case MEMORY_ERROR:
	case OPTIMIZED_OUT_ERROR:
	case NOT_AVAILABLE_ERROR:
	  /* These can normally happen when we try to access an
	     optimized out or unavailable register, either in a
	     physical register or spilled to memory.  */
	  return;
	}

      /* Let unexpected errors propagate.  */
      throw;
    }

  /* Ambiguous unwind or unambiguous unwind verified as matching.  */
  if (chain == NULL || chain->length == 0)
    return;

  cache = cache_new_ref1 (this_frame);
  *tailcall_cachep = cache;
  cache->chain = chain.release ();
  cache->prev_pc = prev_pc;
  cache->chain_levels = pretended_chain_levels (cache->chain);
  cache->prev_sp_p = prev_sp_p;
  if (cache->prev_sp_p)
    {
      cache->prev_sp = prev_sp;
      cache->entry_cfa_sp_offset = *entry_cfa_sp_offsetp;
    }
  gdb_assert (cache->chain_levels > 0);
}

/* Implementation of frame_dealloc_cache_ftype.  It can be called even for the
   bottom chain frame from dwarf2_frame_dealloc_cache which is not a real
   TAILCALL_FRAME.  */

static void
tailcall_frame_dealloc_cache (frame_info *self, void *this_cache)
{
  struct tailcall_cache *cache = (struct tailcall_cache *) this_cache;

  cache_unref (cache);
}

/* Implementation of frame_prev_arch_ftype.  We assume all the virtual tail
   call frames have gdbarch of the bottom (callee) frame.  */

static struct gdbarch *
tailcall_frame_prev_arch (frame_info_ptr this_frame,
			  void **this_prologue_cache)
{
  struct tailcall_cache *cache = (struct tailcall_cache *) *this_prologue_cache;

  return get_frame_arch (frame_info_ptr (cache->next_bottom_frame));
}

/* Virtual tail call frame unwinder if dwarf2_tailcall_sniffer_first finds
   a chain to create.  */

const struct frame_unwind dwarf2_tailcall_frame_unwind =
{
  "dwarf2 tailcall",
  TAILCALL_FRAME,
  default_frame_unwind_stop_reason,
  tailcall_frame_this_id,
  tailcall_frame_prev_register,
  NULL,
  tailcall_frame_sniffer,
  tailcall_frame_dealloc_cache,
  tailcall_frame_prev_arch
};

void _initialize_tailcall_frame ();
void
_initialize_tailcall_frame ()
{
  cache_htab = htab_create_alloc (50, cache_hash, cache_eq, NULL, xcalloc,
				  xfree);
}
