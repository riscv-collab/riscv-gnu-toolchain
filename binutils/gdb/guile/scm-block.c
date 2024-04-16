/* Scheme interface to blocks.

   Copyright (C) 2008-2024 Free Software Foundation, Inc.

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

/* See README file in this directory for implementation notes, coding
   conventions, et.al.  */

#include "defs.h"
#include "block.h"
#include "dictionary.h"
#include "objfiles.h"
#include "source.h"
#include "symtab.h"
#include "guile-internal.h"

/* A smob describing a gdb block.  */

struct block_smob
{
  /* This always appears first.
     We want blocks to be eq?-able.  And we need to be able to invalidate
     blocks when the associated objfile is deleted.  */
  eqable_gdb_smob base;

  /* The GDB block structure that represents a frame's code block.  */
  const struct block *block;

  /* The backing object file.  There is no direct relationship in GDB
     between a block and an object file.  When a block is created also
     store a pointer to the object file for later use.  */
  struct objfile *objfile;
};

/* To iterate over block symbols from Scheme we need to store
   struct block_iterator somewhere.  This is stored in the "progress" field
   of <gdb:iterator>.  We store the block object in iterator_smob.object,
   so we don't store it here.

   Remember: While iterating over block symbols, you must continually check
   whether the block is still valid.  */

struct block_syms_progress_smob
{
  /* This always appears first.  */
  gdb_smob base;

  /* The iterator for that block.  */
  struct block_iterator iter;

  /* Has the iterator been initialized flag.  */
  int initialized_p;
};

static const char block_smob_name[] = "gdb:block";
static const char block_syms_progress_smob_name[] = "gdb:block-symbols-iterator";

/* The tag Guile knows the block smobs by.  */
static scm_t_bits block_smob_tag;
static scm_t_bits block_syms_progress_smob_tag;

/* The "next!" block syms iterator method.  */
static SCM bkscm_next_symbol_x_proc;

/* This is called when an objfile is about to be freed.
   Invalidate the block as further actions on the block would result
   in bad data.  All access to b_smob->block should be gated by
   checks to ensure the block is (still) valid.  */
struct bkscm_deleter
{
  /* Helper function for bkscm_del_objfile_blocks to mark the block
     as invalid.  */

  static int
  bkscm_mark_block_invalid (void **slot, void *info)
  {
    block_smob *b_smob = (block_smob *) *slot;

    b_smob->block = NULL;
    b_smob->objfile = NULL;
    return 1;
  }

  void operator() (htab_t htab)
  {
    gdb_assert (htab != nullptr);
    htab_traverse_noresize (htab, bkscm_mark_block_invalid, NULL);
    htab_delete (htab);
  }
};

static const registry<objfile>::key<htab, bkscm_deleter>
     bkscm_objfile_data_key;

/* Administrivia for block smobs.  */

/* Helper function to hash a block_smob.  */

static hashval_t
bkscm_hash_block_smob (const void *p)
{
  const block_smob *b_smob = (const block_smob *) p;

  return htab_hash_pointer (b_smob->block);
}

/* Helper function to compute equality of block_smobs.  */

static int
bkscm_eq_block_smob (const void *ap, const void *bp)
{
  const block_smob *a = (const block_smob *) ap;
  const block_smob *b = (const block_smob *) bp;

  return (a->block == b->block
	  && a->block != NULL);
}

/* Return the struct block pointer -> SCM mapping table.
   It is created if necessary.  */

static htab_t
bkscm_objfile_block_map (struct objfile *objfile)
{
  htab_t htab = bkscm_objfile_data_key.get (objfile);

  if (htab == NULL)
    {
      htab = gdbscm_create_eqable_gsmob_ptr_map (bkscm_hash_block_smob,
						 bkscm_eq_block_smob);
      bkscm_objfile_data_key.set (objfile, htab);
    }

  return htab;
}

/* The smob "free" function for <gdb:block>.  */

static size_t
bkscm_free_block_smob (SCM self)
{
  block_smob *b_smob = (block_smob *) SCM_SMOB_DATA (self);

  if (b_smob->block != NULL)
    {
      htab_t htab = bkscm_objfile_block_map (b_smob->objfile);

      gdbscm_clear_eqable_gsmob_ptr_slot (htab, &b_smob->base);
    }

  /* Not necessary, done to catch bugs.  */
  b_smob->block = NULL;
  b_smob->objfile = NULL;

  return 0;
}

/* The smob "print" function for <gdb:block>.  */

static int
bkscm_print_block_smob (SCM self, SCM port, scm_print_state *pstate)
{
  block_smob *b_smob = (block_smob *) SCM_SMOB_DATA (self);
  const struct block *b = b_smob->block;

  gdbscm_printf (port, "#<%s", block_smob_name);

  if (b->superblock () == NULL)
    gdbscm_printf (port, " global");
  else if (b->superblock ()->superblock () == NULL)
    gdbscm_printf (port, " static");

  if (b->function () != NULL)
    gdbscm_printf (port, " %s", b->function ()->print_name ());

  gdbscm_printf (port, " %s-%s",
		 hex_string (b->start ()), hex_string (b->end ()));

  scm_puts (">", port);

  scm_remember_upto_here_1 (self);

  /* Non-zero means success.  */
  return 1;
}

/* Low level routine to create a <gdb:block> object.  */

static SCM
bkscm_make_block_smob (void)
{
  block_smob *b_smob = (block_smob *)
    scm_gc_malloc (sizeof (block_smob), block_smob_name);
  SCM b_scm;

  b_smob->block = NULL;
  b_smob->objfile = NULL;
  b_scm = scm_new_smob (block_smob_tag, (scm_t_bits) b_smob);
  gdbscm_init_eqable_gsmob (&b_smob->base, b_scm);

  return b_scm;
}

/* Returns non-zero if SCM is a <gdb:block> object.  */

static int
bkscm_is_block (SCM scm)
{
  return SCM_SMOB_PREDICATE (block_smob_tag, scm);
}

/* (block? scm) -> boolean */

static SCM
gdbscm_block_p (SCM scm)
{
  return scm_from_bool (bkscm_is_block (scm));
}

/* Return the existing object that encapsulates BLOCK, or create a new
   <gdb:block> object.  */

SCM
bkscm_scm_from_block (const struct block *block, struct objfile *objfile)
{
  htab_t htab;
  eqable_gdb_smob **slot;
  block_smob *b_smob, b_smob_for_lookup;
  SCM b_scm;

  /* If we've already created a gsmob for this block, return it.
     This makes blocks eq?-able.  */
  htab = bkscm_objfile_block_map (objfile);
  b_smob_for_lookup.block = block;
  slot = gdbscm_find_eqable_gsmob_ptr_slot (htab, &b_smob_for_lookup.base);
  if (*slot != NULL)
    return (*slot)->containing_scm;

  b_scm = bkscm_make_block_smob ();
  b_smob = (block_smob *) SCM_SMOB_DATA (b_scm);
  b_smob->block = block;
  b_smob->objfile = objfile;
  gdbscm_fill_eqable_gsmob_ptr_slot (slot, &b_smob->base);

  return b_scm;
}

/* Returns the <gdb:block> object in SELF.
   Throws an exception if SELF is not a <gdb:block> object.  */

static SCM
bkscm_get_block_arg_unsafe (SCM self, int arg_pos, const char *func_name)
{
  SCM_ASSERT_TYPE (bkscm_is_block (self), self, arg_pos, func_name,
		   block_smob_name);

  return self;
}

/* Returns a pointer to the block smob of SELF.
   Throws an exception if SELF is not a <gdb:block> object.  */

static block_smob *
bkscm_get_block_smob_arg_unsafe (SCM self, int arg_pos, const char *func_name)
{
  SCM b_scm = bkscm_get_block_arg_unsafe (self, arg_pos, func_name);
  block_smob *b_smob = (block_smob *) SCM_SMOB_DATA (b_scm);

  return b_smob;
}

/* Returns non-zero if block B_SMOB is valid.  */

static int
bkscm_is_valid (block_smob *b_smob)
{
  return b_smob->block != NULL;
}

/* Returns the block smob in SELF, verifying it's valid.
   Throws an exception if SELF is not a <gdb:block> object or is invalid.  */

static block_smob *
bkscm_get_valid_block_smob_arg_unsafe (SCM self, int arg_pos,
				       const char *func_name)
{
  block_smob *b_smob
    = bkscm_get_block_smob_arg_unsafe (self, arg_pos, func_name);

  if (!bkscm_is_valid (b_smob))
    {
      gdbscm_invalid_object_error (func_name, arg_pos, self,
				   _("<gdb:block>"));
    }

  return b_smob;
}

/* Returns the block smob contained in SCM or NULL if SCM is not a
   <gdb:block> object.
   If there is an error a <gdb:exception> object is stored in *EXCP.  */

static block_smob *
bkscm_get_valid_block (SCM scm, int arg_pos, const char *func_name, SCM *excp)
{
  block_smob *b_smob;

  if (!bkscm_is_block (scm))
    {
      *excp = gdbscm_make_type_error (func_name, arg_pos, scm,
				      block_smob_name);
      return NULL;
    }

  b_smob = (block_smob *) SCM_SMOB_DATA (scm);
  if (!bkscm_is_valid (b_smob))
    {
      *excp = gdbscm_make_invalid_object_error (func_name, arg_pos, scm,
						_("<gdb:block>"));
      return NULL;
    }

  return b_smob;
}

/* Returns the struct block that is wrapped by BLOCK_SCM.
   If BLOCK_SCM is not a block, or is an invalid block, then NULL is returned
   and a <gdb:exception> object is stored in *EXCP.  */

const struct block *
bkscm_scm_to_block (SCM block_scm, int arg_pos, const char *func_name,
		    SCM *excp)
{
  block_smob *b_smob;

  b_smob = bkscm_get_valid_block (block_scm, arg_pos, func_name, excp);

  if (b_smob != NULL)
    return b_smob->block;
  return NULL;
}


/* Block methods.  */

/* (block-valid? <gdb:block>) -> boolean
   Returns #t if SELF still exists in GDB.  */

static SCM
gdbscm_block_valid_p (SCM self)
{
  block_smob *b_smob
    = bkscm_get_block_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  return scm_from_bool (bkscm_is_valid (b_smob));
}

/* (block-start <gdb:block>) -> address */

static SCM
gdbscm_block_start (SCM self)
{
  block_smob *b_smob
    = bkscm_get_valid_block_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  const struct block *block = b_smob->block;

  return gdbscm_scm_from_ulongest (block->start ());
}

/* (block-end <gdb:block>) -> address */

static SCM
gdbscm_block_end (SCM self)
{
  block_smob *b_smob
    = bkscm_get_valid_block_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  const struct block *block = b_smob->block;

  return gdbscm_scm_from_ulongest (block->end ());
}

/* (block-function <gdb:block>) -> <gdb:symbol> */

static SCM
gdbscm_block_function (SCM self)
{
  block_smob *b_smob
    = bkscm_get_valid_block_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  const struct block *block = b_smob->block;
  struct symbol *sym;

  sym = block->function ();

  if (sym != NULL)
    return syscm_scm_from_symbol (sym);
  return SCM_BOOL_F;
}

/* (block-superblock <gdb:block>) -> <gdb:block> */

static SCM
gdbscm_block_superblock (SCM self)
{
  block_smob *b_smob
    = bkscm_get_valid_block_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  const struct block *block = b_smob->block;
  const struct block *super_block;

  super_block = block->superblock ();

  if (super_block)
    return bkscm_scm_from_block (super_block, b_smob->objfile);
  return SCM_BOOL_F;
}

/* (block-global-block <gdb:block>) -> <gdb:block>
   Returns the global block associated to this block.  */

static SCM
gdbscm_block_global_block (SCM self)
{
  block_smob *b_smob
    = bkscm_get_valid_block_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  const struct block *block = b_smob->block;
  const struct block *global_block;

  global_block = block->global_block ();

  return bkscm_scm_from_block (global_block, b_smob->objfile);
}

/* (block-static-block <gdb:block>) -> <gdb:block>
   Returns the static block associated to this block.
   Returns #f if we cannot get the static block (this is the global block).  */

static SCM
gdbscm_block_static_block (SCM self)
{
  block_smob *b_smob
    = bkscm_get_valid_block_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  const struct block *block = b_smob->block;
  const struct block *static_block;

  if (block->superblock () == NULL)
    return SCM_BOOL_F;

  static_block = block->static_block ();

  return bkscm_scm_from_block (static_block, b_smob->objfile);
}

/* (block-global? <gdb:block>) -> boolean
   Returns #t if this block object is a global block.  */

static SCM
gdbscm_block_global_p (SCM self)
{
  block_smob *b_smob
    = bkscm_get_valid_block_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  const struct block *block = b_smob->block;

  return scm_from_bool (block->superblock () == NULL);
}

/* (block-static? <gdb:block>) -> boolean
   Returns #t if this block object is a static block.  */

static SCM
gdbscm_block_static_p (SCM self)
{
  block_smob *b_smob
    = bkscm_get_valid_block_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  const struct block *block = b_smob->block;

  if (block->superblock () != NULL
      && block->superblock ()->superblock () == NULL)
    return SCM_BOOL_T;
  return SCM_BOOL_F;
}

/* (block-symbols <gdb:block>) -> list of <gdb:symbol objects
   Returns a list of symbols of the block.  */

static SCM
gdbscm_block_symbols (SCM self)
{
  block_smob *b_smob
    = bkscm_get_valid_block_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  const struct block *block = b_smob->block;
  SCM result;

  result = SCM_EOL;

  for (struct symbol *sym : block_iterator_range (block))
    {
      SCM s_scm = syscm_scm_from_symbol (sym);

      result = scm_cons (s_scm, result);
    }

  return scm_reverse_x (result, SCM_EOL);
}

/* The <gdb:block-symbols-iterator> object,
   for iterating over all symbols in a block.  */

/* The smob "print" function for <gdb:block-symbols-iterator>.  */

static int
bkscm_print_block_syms_progress_smob (SCM self, SCM port,
				      scm_print_state *pstate)
{
  block_syms_progress_smob *i_smob
    = (block_syms_progress_smob *) SCM_SMOB_DATA (self);

  gdbscm_printf (port, "#<%s", block_syms_progress_smob_name);

  if (i_smob->initialized_p)
    {
      switch (i_smob->iter.which)
	{
	case GLOBAL_BLOCK:
	case STATIC_BLOCK:
	  {
	    struct compunit_symtab *cust;

	    gdbscm_printf (port, " %s", 
			   i_smob->iter.which == GLOBAL_BLOCK
			   ? "global" : "static");
	    if (i_smob->iter.idx != -1)
	      gdbscm_printf (port, " @%d", i_smob->iter.idx);
	    cust = (i_smob->iter.idx == -1
		    ? i_smob->iter.d.compunit_symtab
		    : i_smob->iter.d.compunit_symtab->includes[i_smob->iter.idx]);
	    gdbscm_printf (port, " %s",
			   symtab_to_filename_for_display
			     (cust->primary_filetab ()));
	    break;
	  }
	case FIRST_LOCAL_BLOCK:
	  gdbscm_printf (port, " single block");
	  break;
	}
    }
  else
    gdbscm_printf (port, " !initialized");

  scm_puts (">", port);

  scm_remember_upto_here_1 (self);

  /* Non-zero means success.  */
  return 1;
}

/* Low level routine to create a <gdb:block-symbols-progress> object.  */

static SCM
bkscm_make_block_syms_progress_smob (void)
{
  block_syms_progress_smob *i_smob = (block_syms_progress_smob *)
    scm_gc_malloc (sizeof (block_syms_progress_smob),
		   block_syms_progress_smob_name);
  SCM smob;

  memset (&i_smob->iter, 0, sizeof (i_smob->iter));
  i_smob->initialized_p = 0;
  smob = scm_new_smob (block_syms_progress_smob_tag, (scm_t_bits) i_smob);
  gdbscm_init_gsmob (&i_smob->base);

  return smob;
}

/* Returns non-zero if SCM is a <gdb:block-symbols-progress> object.  */

static int
bkscm_is_block_syms_progress (SCM scm)
{
  return SCM_SMOB_PREDICATE (block_syms_progress_smob_tag, scm);
}

/* (block-symbols-progress? scm) -> boolean */

static SCM
bkscm_block_syms_progress_p (SCM scm)
{
  return scm_from_bool (bkscm_is_block_syms_progress (scm));
}

/* (make-block-symbols-iterator <gdb:block>) -> <gdb:iterator>
   Return a <gdb:iterator> object for iterating over the symbols of SELF.  */

static SCM
gdbscm_make_block_syms_iter (SCM self)
{
  /* Call for side effects.  */
  bkscm_get_valid_block_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  SCM progress, iter;

  progress = bkscm_make_block_syms_progress_smob ();

  iter = gdbscm_make_iterator (self, progress, bkscm_next_symbol_x_proc);

  return iter;
}

/* Returns the next symbol in the iteration through the block's dictionary,
   or (end-of-iteration).
   This is the iterator_smob.next_x method.  */

static SCM
gdbscm_block_next_symbol_x (SCM self)
{
  SCM progress, iter_scm, block_scm;
  iterator_smob *iter_smob;
  block_smob *b_smob;
  const struct block *block;
  block_syms_progress_smob *p_smob;
  struct symbol *sym;

  iter_scm = itscm_get_iterator_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  iter_smob = (iterator_smob *) SCM_SMOB_DATA (iter_scm);

  block_scm = itscm_iterator_smob_object (iter_smob);
  b_smob = bkscm_get_valid_block_smob_arg_unsafe (block_scm,
						  SCM_ARG1, FUNC_NAME);
  block = b_smob->block;

  progress = itscm_iterator_smob_progress (iter_smob);

  SCM_ASSERT_TYPE (bkscm_is_block_syms_progress (progress),
		   progress, SCM_ARG1, FUNC_NAME,
		   block_syms_progress_smob_name);
  p_smob = (block_syms_progress_smob *) SCM_SMOB_DATA (progress);

  if (!p_smob->initialized_p)
    {
      sym = block_iterator_first (block, &p_smob->iter);
      p_smob->initialized_p = 1;
    }
  else
    sym = block_iterator_next (&p_smob->iter);

  if (sym == NULL)
    return gdbscm_end_of_iteration ();

  return syscm_scm_from_symbol (sym);
}

/* (lookup-block address) -> <gdb:block>
   Returns the innermost lexical block containing the specified pc value,
   or #f if there is none.  */

static SCM
gdbscm_lookup_block (SCM pc_scm)
{
  CORE_ADDR pc;
  const struct block *block = NULL;
  struct compunit_symtab *cust = NULL;

  gdbscm_parse_function_args (FUNC_NAME, SCM_ARG1, NULL, "U", pc_scm, &pc);

  gdbscm_gdb_exception exc {};
  try
    {
      cust = find_pc_compunit_symtab (pc);

      if (cust != NULL && cust->objfile () != NULL)
	block = block_for_pc (pc);
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  if (cust == NULL || cust->objfile () == NULL)
    {
      gdbscm_out_of_range_error (FUNC_NAME, SCM_ARG1, pc_scm,
				 _("cannot locate object file for block"));
    }

  if (block != NULL)
    return bkscm_scm_from_block (block, cust->objfile ());
  return SCM_BOOL_F;
}

/* Initialize the Scheme block support.  */

static const scheme_function block_functions[] =
{
  { "block?", 1, 0, 0, as_a_scm_t_subr (gdbscm_block_p),
    "\
Return #t if the object is a <gdb:block> object." },

  { "block-valid?", 1, 0, 0, as_a_scm_t_subr (gdbscm_block_valid_p),
    "\
Return #t if the block is valid.\n\
A block becomes invalid when its objfile is freed." },

  { "block-start", 1, 0, 0, as_a_scm_t_subr (gdbscm_block_start),
    "\
Return the start address of the block." },

  { "block-end", 1, 0, 0, as_a_scm_t_subr (gdbscm_block_end),
    "\
Return the end address of the block." },

  { "block-function", 1, 0, 0, as_a_scm_t_subr (gdbscm_block_function),
    "\
Return the gdb:symbol object of the function containing the block\n\
or #f if the block does not live in any function." },

  { "block-superblock", 1, 0, 0, as_a_scm_t_subr (gdbscm_block_superblock),
    "\
Return the superblock (parent block) of the block." },

  { "block-global-block", 1, 0, 0, as_a_scm_t_subr (gdbscm_block_global_block),
    "\
Return the global block of the block." },

  { "block-static-block", 1, 0, 0, as_a_scm_t_subr (gdbscm_block_static_block),
    "\
Return the static block of the block." },

  { "block-global?", 1, 0, 0, as_a_scm_t_subr (gdbscm_block_global_p),
    "\
Return #t if block is a global block." },

  { "block-static?", 1, 0, 0, as_a_scm_t_subr (gdbscm_block_static_p),
    "\
Return #t if block is a static block." },

  { "block-symbols", 1, 0, 0, as_a_scm_t_subr (gdbscm_block_symbols),
    "\
Return a list of all symbols (as <gdb:symbol> objects) in the block." },

  { "make-block-symbols-iterator", 1, 0, 0,
    as_a_scm_t_subr (gdbscm_make_block_syms_iter),
    "\
Return a <gdb:iterator> object for iterating over all symbols in the block." },

  { "block-symbols-progress?", 1, 0, 0,
    as_a_scm_t_subr (bkscm_block_syms_progress_p),
    "\
Return #t if the object is a <gdb:block-symbols-progress> object." },

  { "lookup-block", 1, 0, 0, as_a_scm_t_subr (gdbscm_lookup_block),
    "\
Return the innermost GDB block containing the address or #f if none found.\n\
\n\
  Arguments:\n\
    address: the address to lookup" },

  END_FUNCTIONS
};

void
gdbscm_initialize_blocks (void)
{
  block_smob_tag
    = gdbscm_make_smob_type (block_smob_name, sizeof (block_smob));
  scm_set_smob_free (block_smob_tag, bkscm_free_block_smob);
  scm_set_smob_print (block_smob_tag, bkscm_print_block_smob);

  block_syms_progress_smob_tag
    = gdbscm_make_smob_type (block_syms_progress_smob_name,
			     sizeof (block_syms_progress_smob));
  scm_set_smob_print (block_syms_progress_smob_tag,
		      bkscm_print_block_syms_progress_smob);

  gdbscm_define_functions (block_functions, 1);

  /* This function is "private".  */
  bkscm_next_symbol_x_proc
    = scm_c_define_gsubr ("%block-next-symbol!", 1, 0, 0,
			  as_a_scm_t_subr (gdbscm_block_next_symbol_x));
  scm_set_procedure_property_x (bkscm_next_symbol_x_proc,
				gdbscm_documentation_symbol,
				gdbscm_scm_from_c_string ("\
Internal function to assist the block symbols iterator."));
}
