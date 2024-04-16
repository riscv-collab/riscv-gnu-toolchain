/* Caching code for GDB, the GNU debugger.

   Copyright (C) 1992-2024 Free Software Foundation, Inc.

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
#include "dcache.h"
#include "gdbcmd.h"
#include "gdbcore.h"
#include "target-dcache.h"
#include "inferior.h"
#include "splay-tree.h"
#include "gdbarch.h"

/* Commands with a prefix of `{set,show} dcache'.  */
static struct cmd_list_element *dcache_set_list = NULL;
static struct cmd_list_element *dcache_show_list = NULL;

/* The data cache could lead to incorrect results because it doesn't
   know about volatile variables, thus making it impossible to debug
   functions which use memory mapped I/O devices.  Set the nocache
   memory region attribute in those cases.

   In general the dcache speeds up performance.  Some speed improvement
   comes from the actual caching mechanism, but the major gain is in
   the reduction of the remote protocol overhead; instead of reading
   or writing a large area of memory in 4 byte requests, the cache
   bundles up the requests into LINE_SIZE chunks, reducing overhead
   significantly.  This is most useful when accessing a large amount
   of data, such as when performing a backtrace.

   The cache is a splay tree along with a linked list for replacement.
   Each block caches a LINE_SIZE area of memory.  Within each line we
   remember the address of the line (which must be a multiple of
   LINE_SIZE) and the actual data block.

   Lines are only allocated as needed, so DCACHE_SIZE really specifies the
   *maximum* number of lines in the cache.

   At present, the cache is write-through rather than writeback: as soon
   as data is written to the cache, it is also immediately written to
   the target.  Therefore, cache lines are never "dirty".  Whether a given
   line is valid or not depends on where it is stored in the dcache_struct;
   there is no per-block valid flag.  */

/* NOTE: Interaction of dcache and memory region attributes

   As there is no requirement that memory region attributes be aligned
   to or be a multiple of the dcache page size, dcache_read_line() and
   dcache_write_line() must break up the page by memory region.  If a
   chunk does not have the cache attribute set, an invalid memory type
   is set, etc., then the chunk is skipped.  Those chunks are handled
   in target_xfer_memory() (or target_xfer_memory_partial()).

   This doesn't occur very often.  The most common occurrence is when
   the last bit of the .text segment and the first bit of the .data
   segment fall within the same dcache page with a ro/cacheable memory
   region defined for the .text segment and a rw/non-cacheable memory
   region defined for the .data segment.  */

/* The maximum number of lines stored.  The total size of the cache is
   equal to DCACHE_SIZE times LINE_SIZE.  */
#define DCACHE_DEFAULT_SIZE 4096
static unsigned dcache_size = DCACHE_DEFAULT_SIZE;

/* The default size of a cache line.  Smaller values reduce the time taken to
   read a single byte and make the cache more granular, but increase
   overhead and reduce the effectiveness of the cache as a prefetcher.  */
#define DCACHE_DEFAULT_LINE_SIZE 64
static unsigned dcache_line_size = DCACHE_DEFAULT_LINE_SIZE;

/* Each cache block holds LINE_SIZE bytes of data
   starting at a multiple-of-LINE_SIZE address.  */

#define LINE_SIZE_MASK(dcache)  ((dcache->line_size - 1))
#define XFORM(dcache, x) 	((x) & LINE_SIZE_MASK (dcache))
#define MASK(dcache, x)         ((x) & ~LINE_SIZE_MASK (dcache))

struct dcache_block
{
  /* For least-recently-allocated and free lists.  */
  struct dcache_block *prev;
  struct dcache_block *next;

  CORE_ADDR addr;		/* address of data */
  int refs;			/* # hits */
  gdb_byte data[1];		/* line_size bytes at given address */
};

struct dcache_struct
{
  splay_tree tree;
  struct dcache_block *oldest; /* least-recently-allocated list.  */

  /* The free list is maintained identically to OLDEST to simplify
     the code: we only need one set of accessors.  */
  struct dcache_block *freelist;

  /* The number of in-use lines in the cache.  */
  int size;
  CORE_ADDR line_size;  /* current line_size.  */

  /* The ptid of last inferior to use cache or null_ptid.  */
  ptid_t ptid;

  /* The process target of last inferior to use the cache or
     nullptr.  */
  process_stratum_target *proc_target;
};

typedef void (block_func) (struct dcache_block *block, void *param);

static struct dcache_block *dcache_hit (DCACHE *dcache, CORE_ADDR addr);

static int dcache_read_line (DCACHE *dcache, struct dcache_block *db);

static struct dcache_block *dcache_alloc (DCACHE *dcache, CORE_ADDR addr);

static bool dcache_enabled_p = false; /* OBSOLETE */

static void
show_dcache_enabled_p (struct ui_file *file, int from_tty,
		       struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Deprecated remotecache flag is %s.\n"), value);
}

/* Add BLOCK to circular block list BLIST, behind the block at *BLIST.
   *BLIST is not updated (unless it was previously NULL of course).
   This is for the least-recently-allocated list's sake:
   BLIST points to the oldest block.
   ??? This makes for poor cache usage of the free list,
   but is it measurable?  */

static void
append_block (struct dcache_block **blist, struct dcache_block *block)
{
  if (*blist)
    {
      block->next = *blist;
      block->prev = (*blist)->prev;
      block->prev->next = block;
      (*blist)->prev = block;
      /* We don't update *BLIST here to maintain the invariant that for the
	 least-recently-allocated list *BLIST points to the oldest block.  */
    }
  else
    {
      block->next = block;
      block->prev = block;
      *blist = block;
    }
}

/* Remove BLOCK from circular block list BLIST.  */

static void
remove_block (struct dcache_block **blist, struct dcache_block *block)
{
  if (block->next == block)
    {
      *blist = NULL;
    }
  else
    {
      block->next->prev = block->prev;
      block->prev->next = block->next;
      /* If we removed the block *BLIST points to, shift it to the next block
	 to maintain the invariant that for the least-recently-allocated list
	 *BLIST points to the oldest block.  */
      if (*blist == block)
	*blist = block->next;
    }
}

/* Iterate over all elements in BLIST, calling FUNC.
   PARAM is passed to FUNC.
   FUNC may remove the block it's passed, but only that block.  */

static void
for_each_block (struct dcache_block **blist, block_func *func, void *param)
{
  struct dcache_block *db;

  if (*blist == NULL)
    return;

  db = *blist;
  do
    {
      struct dcache_block *next = db->next;

      func (db, param);
      db = next;
    }
  while (*blist && db != *blist);
}

/* BLOCK_FUNC routine for dcache_free.  */

static void
free_block (struct dcache_block *block, void *param)
{
  xfree (block);
}

/* Free a data cache.  */

void
dcache_free (DCACHE *dcache)
{
  splay_tree_delete (dcache->tree);
  for_each_block (&dcache->oldest, free_block, NULL);
  for_each_block (&dcache->freelist, free_block, NULL);
  xfree (dcache);
}


/* BLOCK_FUNC function for dcache_invalidate.
   This doesn't remove the block from the oldest list on purpose.
   dcache_invalidate will do it later.  */

static void
invalidate_block (struct dcache_block *block, void *param)
{
  DCACHE *dcache = (DCACHE *) param;

  splay_tree_remove (dcache->tree, (splay_tree_key) block->addr);
  append_block (&dcache->freelist, block);
}

/* Free all the data cache blocks, thus discarding all cached data.  */

void
dcache_invalidate (DCACHE *dcache)
{
  for_each_block (&dcache->oldest, invalidate_block, dcache);

  dcache->oldest = NULL;
  dcache->size = 0;
  dcache->ptid = null_ptid;
  dcache->proc_target = nullptr;

  if (dcache->line_size != dcache_line_size)
    {
      /* We've been asked to use a different line size.
	 All of our freelist blocks are now the wrong size, so free them.  */

      for_each_block (&dcache->freelist, free_block, dcache);
      dcache->freelist = NULL;
      dcache->line_size = dcache_line_size;
    }
}

/* Invalidate the line associated with ADDR.  */

static void
dcache_invalidate_line (DCACHE *dcache, CORE_ADDR addr)
{
  struct dcache_block *db = dcache_hit (dcache, addr);

  if (db)
    {
      splay_tree_remove (dcache->tree, (splay_tree_key) db->addr);
      remove_block (&dcache->oldest, db);
      append_block (&dcache->freelist, db);
      --dcache->size;
    }
}

/* If addr is present in the dcache, return the address of the block
   containing it.  Otherwise return NULL.  */

static struct dcache_block *
dcache_hit (DCACHE *dcache, CORE_ADDR addr)
{
  struct dcache_block *db;

  splay_tree_node node = splay_tree_lookup (dcache->tree,
					    (splay_tree_key) MASK (dcache, addr));

  if (!node)
    return NULL;

  db = (struct dcache_block *) node->value;
  db->refs++;
  return db;
}

/* Fill a cache line from target memory.
   The result is 1 for success, 0 if the (entire) cache line
   wasn't readable.  */

static int
dcache_read_line (DCACHE *dcache, struct dcache_block *db)
{
  CORE_ADDR memaddr;
  gdb_byte *myaddr;
  int len;
  int res;
  int reg_len;
  struct mem_region *region;

  len = dcache->line_size;
  memaddr = db->addr;
  myaddr  = db->data;

  while (len > 0)
    {
      /* Don't overrun if this block is right at the end of the region.  */
      region = lookup_mem_region (memaddr);
      if (region->hi == 0 || memaddr + len < region->hi)
	reg_len = len;
      else
	reg_len = region->hi - memaddr;

      /* Skip non-readable regions.  The cache attribute can be ignored,
	 since we may be loading this for a stack access.  */
      if (region->attrib.mode == MEM_WO)
	{
	  memaddr += reg_len;
	  myaddr  += reg_len;
	  len     -= reg_len;
	  continue;
	}

      res = target_read_raw_memory (memaddr, myaddr, reg_len);
      if (res != 0)
	return 0;

      memaddr += reg_len;
      myaddr += reg_len;
      len -= reg_len;
    }

  return 1;
}

/* Get a free cache block, put or keep it on the valid list,
   and return its address.  */

static struct dcache_block *
dcache_alloc (DCACHE *dcache, CORE_ADDR addr)
{
  struct dcache_block *db;

  if (dcache->size >= dcache_size)
    {
      /* Evict the least recently allocated line.  */
      db = dcache->oldest;
      remove_block (&dcache->oldest, db);

      splay_tree_remove (dcache->tree, (splay_tree_key) db->addr);
    }
  else
    {
      db = dcache->freelist;
      if (db)
	remove_block (&dcache->freelist, db);
      else
	db = ((struct dcache_block *)
	      xmalloc (offsetof (struct dcache_block, data)
		       + dcache->line_size));

      dcache->size++;
    }

  db->addr = MASK (dcache, addr);
  db->refs = 0;

  /* Put DB at the end of the list, it's the newest.  */
  append_block (&dcache->oldest, db);

  splay_tree_insert (dcache->tree, (splay_tree_key) db->addr,
		     (splay_tree_value) db);

  return db;
}

/* Using the data cache DCACHE, store in *PTR the contents of the byte at
   address ADDR in the remote machine.  

   Returns 1 for success, 0 for error.  */

static int
dcache_peek_byte (DCACHE *dcache, CORE_ADDR addr, gdb_byte *ptr)
{
  struct dcache_block *db = dcache_hit (dcache, addr);

  if (!db)
    {
      db = dcache_alloc (dcache, addr);

      if (!dcache_read_line (dcache, db))
	 return 0;
    }

  *ptr = db->data[XFORM (dcache, addr)];
  return 1;
}

/* Write the byte at PTR into ADDR in the data cache.

   The caller should have written the data through to target memory
   already.

   If ADDR is not in cache, this function does nothing; writing to an
   area of memory which wasn't present in the cache doesn't cause it
   to be loaded in.  */

static void
dcache_poke_byte (DCACHE *dcache, CORE_ADDR addr, const gdb_byte *ptr)
{
  struct dcache_block *db = dcache_hit (dcache, addr);

  if (db)
    db->data[XFORM (dcache, addr)] = *ptr;
}

static int
dcache_splay_tree_compare (splay_tree_key a, splay_tree_key b)
{
  if (a > b)
    return 1;
  else if (a == b)
    return 0;
  else
    return -1;
}

/* Allocate and initialize a data cache.  */

DCACHE *
dcache_init (void)
{
  DCACHE *dcache = XNEW (DCACHE);

  dcache->tree = splay_tree_new (dcache_splay_tree_compare,
				 NULL,
				 NULL);

  dcache->oldest = NULL;
  dcache->freelist = NULL;
  dcache->size = 0;
  dcache->line_size = dcache_line_size;
  dcache->ptid = null_ptid;
  dcache->proc_target = nullptr;

  return dcache;
}


/* Read LEN bytes from dcache memory at MEMADDR, transferring to
   debugger address MYADDR.  If the data is presently cached, this
   fills the cache.  Arguments/return are like the target_xfer_partial
   interface.  */

enum target_xfer_status
dcache_read_memory_partial (struct target_ops *ops, DCACHE *dcache,
			    CORE_ADDR memaddr, gdb_byte *myaddr,
			    ULONGEST len, ULONGEST *xfered_len)
{
  ULONGEST i;

  /* If this is a different thread from what we've recorded, flush the
     cache.  */

  process_stratum_target *proc_target = current_inferior ()->process_target ();
  if (proc_target != dcache->proc_target || inferior_ptid != dcache->ptid)
    {
      dcache_invalidate (dcache);
      dcache->ptid = inferior_ptid;
      dcache->proc_target = proc_target;
    }

  for (i = 0; i < len; i++)
    {
      if (!dcache_peek_byte (dcache, memaddr + i, myaddr + i))
	{
	  /* That failed.  Discard its cache line so we don't have a
	     partially read line.  */
	  dcache_invalidate_line (dcache, memaddr + i);
	  break;
	}
    }

  if (i == 0)
    {
      /* Even though reading the whole line failed, we may be able to
	 read a piece starting where the caller wanted.  */
      return raw_memory_xfer_partial (ops, myaddr, NULL, memaddr, len,
				      xfered_len);
    }
  else
    {
      *xfered_len = i;
      return TARGET_XFER_OK;
    }
}

/* FIXME: There would be some benefit to making the cache write-back and
   moving the writeback operation to a higher layer, as it could occur
   after a sequence of smaller writes have been completed (as when a stack
   frame is constructed for an inferior function call).  Note that only
   moving it up one level to target_xfer_memory[_partial]() is not
   sufficient since we want to coalesce memory transfers that are
   "logically" connected but not actually a single call to one of the
   memory transfer functions.  */

/* Just update any cache lines which are already present.  This is
   called by the target_xfer_partial machinery when writing raw
   memory.  */

void
dcache_update (DCACHE *dcache, enum target_xfer_status status,
	       CORE_ADDR memaddr, const gdb_byte *myaddr,
	       ULONGEST len)
{
  ULONGEST i;

  for (i = 0; i < len; i++)
    if (status == TARGET_XFER_OK)
      dcache_poke_byte (dcache, memaddr + i, myaddr + i);
    else
      {
	/* Discard the whole cache line so we don't have a partially
	   valid line.  */
	dcache_invalidate_line (dcache, memaddr + i);
      }
}

/* Print DCACHE line INDEX.  */

static void
dcache_print_line (DCACHE *dcache, int index)
{
  splay_tree_node n;
  struct dcache_block *db;
  int i, j;

  if (dcache == NULL)
    {
      gdb_printf (_("No data cache available.\n"));
      return;
    }

  n = splay_tree_min (dcache->tree);

  for (i = index; i > 0; --i)
    {
      if (!n)
	break;
      n = splay_tree_successor (dcache->tree, n->key);
    }

  if (!n)
    {
      gdb_printf (_("No such cache line exists.\n"));
      return;
    }
    
  db = (struct dcache_block *) n->value;

  gdb_printf (_("Line %d: address %s [%d hits]\n"),
	      index, paddress (current_inferior ()->arch (), db->addr),
	      db->refs);

  for (j = 0; j < dcache->line_size; j++)
    {
      gdb_printf ("%02x ", db->data[j]);

      /* Print a newline every 16 bytes (48 characters).  */
      if ((j % 16 == 15) && (j != dcache->line_size - 1))
	gdb_printf ("\n");
    }
  gdb_printf ("\n");
}

/* Parse EXP and show the info about DCACHE.  */

static void
dcache_info_1 (DCACHE *dcache, const char *exp)
{
  splay_tree_node n;
  int i, refcount;

  if (exp)
    {
      char *linestart;

      i = strtol (exp, &linestart, 10);
      if (linestart == exp || i < 0)
	{
	  gdb_printf (_("Usage: info dcache [LINENUMBER]\n"));
	  return;
	}

      dcache_print_line (dcache, i);
      return;
    }

  gdb_printf (_("Dcache %u lines of %u bytes each.\n"),
	      dcache_size,
	      dcache ? (unsigned) dcache->line_size
	      : dcache_line_size);

  if (dcache == NULL || dcache->ptid == null_ptid)
    {
      gdb_printf (_("No data cache available.\n"));
      return;
    }

  gdb_printf (_("Contains data for %s\n"),
	      target_pid_to_str (dcache->ptid).c_str ());

  refcount = 0;

  n = splay_tree_min (dcache->tree);
  i = 0;

  while (n)
    {
      struct dcache_block *db = (struct dcache_block *) n->value;

      gdb_printf (_("Line %d: address %s [%d hits]\n"),
		  i, paddress (current_inferior ()->arch (), db->addr),
		  db->refs);
      i++;
      refcount += db->refs;

      n = splay_tree_successor (dcache->tree, n->key);
    }

  gdb_printf (_("Cache state: %d active lines, %d hits\n"), i, refcount);
}

static void
info_dcache_command (const char *exp, int tty)
{
  dcache_info_1 (target_dcache_get (current_program_space->aspace), exp);
}

static void
set_dcache_size (const char *args, int from_tty,
		 struct cmd_list_element *c)
{
  if (dcache_size == 0)
    {
      dcache_size = DCACHE_DEFAULT_SIZE;
      error (_("Dcache size must be greater than 0."));
    }
  target_dcache_invalidate (current_program_space->aspace);
}

static void
set_dcache_line_size (const char *args, int from_tty,
		      struct cmd_list_element *c)
{
  if (dcache_line_size < 2
      || (dcache_line_size & (dcache_line_size - 1)) != 0)
    {
      unsigned d = dcache_line_size;
      dcache_line_size = DCACHE_DEFAULT_LINE_SIZE;
      error (_("Invalid dcache line size: %u (must be power of 2)."), d);
    }
  target_dcache_invalidate (current_program_space->aspace);
}

void _initialize_dcache ();
void
_initialize_dcache ()
{
  add_setshow_boolean_cmd ("remotecache", class_support,
			   &dcache_enabled_p, _("\
Set cache use for remote targets."), _("\
Show cache use for remote targets."), _("\
This used to enable the data cache for remote targets.  The cache\n\
functionality is now controlled by the memory region system and the\n\
\"stack-cache\" flag; \"remotecache\" now does nothing and\n\
exists only for compatibility reasons."),
			   NULL,
			   show_dcache_enabled_p,
			   &setlist, &showlist);

  add_info ("dcache", info_dcache_command,
	    _("\
Print information on the dcache performance.\n\
Usage: info dcache [LINENUMBER]\n\
With no arguments, this command prints the cache configuration and a\n\
summary of each line in the cache.  With an argument, dump\"\n\
the contents of the given line."));

  add_setshow_prefix_cmd ("dcache", class_obscure,
			  _("\
Use this command to set number of lines in dcache and line-size."),
			  ("Show dcache settings."),
			  &dcache_set_list, &dcache_show_list,
			  &setlist, &showlist);

  add_setshow_zuinteger_cmd ("line-size", class_obscure,
			     &dcache_line_size, _("\
Set dcache line size in bytes (must be power of 2)."), _("\
Show dcache line size."),
			     NULL,
			     set_dcache_line_size,
			     NULL,
			     &dcache_set_list, &dcache_show_list);
  add_setshow_zuinteger_cmd ("size", class_obscure,
			     &dcache_size, _("\
Set number of dcache lines."), _("\
Show number of dcache lines."),
			     NULL,
			     set_dcache_size,
			     NULL,
			     &dcache_set_list, &dcache_show_list);
}
