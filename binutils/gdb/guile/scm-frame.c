/* Scheme interface to stack frames.

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
#include "frame.h"
#include "inferior.h"
#include "objfiles.h"
#include "symfile.h"
#include "symtab.h"
#include "stack.h"
#include "user-regs.h"
#include "value.h"
#include "guile-internal.h"

/* The <gdb:frame> smob.  */

struct frame_smob
{
  /* This always appears first.  */
  eqable_gdb_smob base;

  struct frame_id frame_id;
  struct gdbarch *gdbarch;

  /* Frames are tracked by inferior.
     We need some place to put the eq?-able hash table, and this feels as
     good a place as any.  Frames in one inferior shouldn't be considered
     equal to frames in a different inferior.  The frame becomes invalid if
     this becomes NULL (the inferior has been deleted from gdb).
     It's easier to relax restrictions than impose them after the fact.
     N.B. It is an outstanding question whether a frame survives reruns of
     the inferior.  Intuitively the answer is "No", but currently a frame
     also survives, e.g., multiple invocations of the same function from
     the same point.  Even different threads can have the same frame, e.g.,
     if a thread dies and a new thread gets the same stack.  */
  struct inferior *inferior;

  /* Marks that the FRAME_ID member actually holds the ID of the frame next
     to this, and not this frame's ID itself.  This is a hack to permit Scheme
     frame objects which represent invalid frames (i.e., the last frame_info
     in a corrupt stack).  The problem arises from the fact that this code
     relies on FRAME_ID to uniquely identify a frame, which is not always true
     for the last "frame" in a corrupt stack (it can have a null ID, or the
     same ID as the  previous frame).  Whenever get_prev_frame returns NULL, we
     record the frame_id of the next frame and set FRAME_ID_IS_NEXT to 1.  */
  int frame_id_is_next;
};

static const char frame_smob_name[] = "gdb:frame";

/* The tag Guile knows the frame smob by.  */
static scm_t_bits frame_smob_tag;

/* Keywords used in argument passing.  */
static SCM block_keyword;

/* This is called when an inferior is about to be freed.
   Invalidate the frame as further actions on the frame could result
   in bad data.  All access to the frame should be gated by
   frscm_get_frame_smob_arg_unsafe which will raise an exception on
   invalid frames.  */
struct frscm_deleter
{
  /* Helper function for frscm_del_inferior_frames to mark the frame
     as invalid.  */

  static int
  frscm_mark_frame_invalid (void **slot, void *info)
  {
    frame_smob *f_smob = (frame_smob *) *slot;

    f_smob->inferior = NULL;
    return 1;
  }

  void operator() (htab_t htab)
  {
    gdb_assert (htab != nullptr);
    htab_traverse_noresize (htab, frscm_mark_frame_invalid, NULL);
    htab_delete (htab);
  }
};

static const registry<inferior>::key<htab, frscm_deleter>
    frscm_inferior_data_key;

/* Administrivia for frame smobs.  */

/* Helper function to hash a frame_smob.  */

static hashval_t
frscm_hash_frame_smob (const void *p)
{
  const frame_smob *f_smob = (const frame_smob *) p;
  const struct frame_id *fid = &f_smob->frame_id;
  hashval_t hash = htab_hash_pointer (f_smob->inferior);

  if (fid->stack_status == FID_STACK_VALID)
    hash = iterative_hash (&fid->stack_addr, sizeof (fid->stack_addr), hash);
  if (fid->code_addr_p)
    hash = iterative_hash (&fid->code_addr, sizeof (fid->code_addr), hash);
  if (fid->special_addr_p)
    hash = iterative_hash (&fid->special_addr, sizeof (fid->special_addr),
			   hash);

  return hash;
}

/* Helper function to compute equality of frame_smobs.  */

static int
frscm_eq_frame_smob (const void *ap, const void *bp)
{
  const frame_smob *a = (const frame_smob *) ap;
  const frame_smob *b = (const frame_smob *) bp;

  return (a->frame_id == b->frame_id
	  && a->inferior == b->inferior
	  && a->inferior != NULL);
}

/* Return the frame -> SCM mapping table.
   It is created if necessary.  */

static htab_t
frscm_inferior_frame_map (struct inferior *inferior)
{
  htab_t htab = frscm_inferior_data_key.get (inferior);

  if (htab == NULL)
    {
      htab = gdbscm_create_eqable_gsmob_ptr_map (frscm_hash_frame_smob,
						 frscm_eq_frame_smob);
      frscm_inferior_data_key.set (inferior, htab);
    }

  return htab;
}

/* The smob "free" function for <gdb:frame>.  */

static size_t
frscm_free_frame_smob (SCM self)
{
  frame_smob *f_smob = (frame_smob *) SCM_SMOB_DATA (self);

  if (f_smob->inferior != NULL)
    {
      htab_t htab = frscm_inferior_frame_map (f_smob->inferior);

      gdbscm_clear_eqable_gsmob_ptr_slot (htab, &f_smob->base);
    }

  /* Not necessary, done to catch bugs.  */
  f_smob->inferior = NULL;

  return 0;
}

/* The smob "print" function for <gdb:frame>.  */

static int
frscm_print_frame_smob (SCM self, SCM port, scm_print_state *pstate)
{
  frame_smob *f_smob = (frame_smob *) SCM_SMOB_DATA (self);

  gdbscm_printf (port, "#<%s %s>",
		 frame_smob_name,
		 f_smob->frame_id.to_string ().c_str ());
  scm_remember_upto_here_1 (self);

  /* Non-zero means success.  */
  return 1;
}

/* Low level routine to create a <gdb:frame> object.  */

static SCM
frscm_make_frame_smob (void)
{
  frame_smob *f_smob = (frame_smob *)
    scm_gc_malloc (sizeof (frame_smob), frame_smob_name);
  SCM f_scm;

  f_smob->frame_id = null_frame_id;
  f_smob->gdbarch = NULL;
  f_smob->inferior = NULL;
  f_smob->frame_id_is_next = 0;
  f_scm = scm_new_smob (frame_smob_tag, (scm_t_bits) f_smob);
  gdbscm_init_eqable_gsmob (&f_smob->base, f_scm);

  return f_scm;
}

/* Return non-zero if SCM is a <gdb:frame> object.  */

int
frscm_is_frame (SCM scm)
{
  return SCM_SMOB_PREDICATE (frame_smob_tag, scm);
}

/* (frame? object) -> boolean */

static SCM
gdbscm_frame_p (SCM scm)
{
  return scm_from_bool (frscm_is_frame (scm));
}

/* Create a new <gdb:frame> object that encapsulates FRAME.
   Returns a <gdb:exception> object if there is an error.  */

static SCM
frscm_scm_from_frame (struct frame_info *frame, struct inferior *inferior)
{
  frame_smob *f_smob, f_smob_for_lookup;
  SCM f_scm;
  htab_t htab;
  eqable_gdb_smob **slot;
  struct frame_id frame_id = null_frame_id;
  struct gdbarch *gdbarch = NULL;
  int frame_id_is_next = 0;

  /* If we've already created a gsmob for this frame, return it.
     This makes frames eq?-able.  */
  htab = frscm_inferior_frame_map (inferior);
  f_smob_for_lookup.frame_id = get_frame_id (frame_info_ptr (frame));
  f_smob_for_lookup.inferior = inferior;
  slot = gdbscm_find_eqable_gsmob_ptr_slot (htab, &f_smob_for_lookup.base);
  if (*slot != NULL)
    return (*slot)->containing_scm;

  try
    {
      frame_info_ptr frame_ptr (frame);

      /* Try to get the previous frame, to determine if this is the last frame
	 in a corrupt stack.  If so, we need to store the frame_id of the next
	 frame and not of this one (which is possibly invalid).  */
      if (get_prev_frame (frame_ptr) == NULL
	  && get_frame_unwind_stop_reason (frame_ptr) != UNWIND_NO_REASON
	  && get_next_frame (frame_ptr) != NULL)
	{
	  frame_id = get_frame_id (get_next_frame (frame_ptr));
	  frame_id_is_next = 1;
	}
      else
	{
	  frame_id = get_frame_id (frame_ptr);
	  frame_id_is_next = 0;
	}
      gdbarch = get_frame_arch (frame_ptr);
    }
  catch (const gdb_exception &except)
    {
      return gdbscm_scm_from_gdb_exception (unpack (except));
    }

  f_scm = frscm_make_frame_smob ();
  f_smob = (frame_smob *) SCM_SMOB_DATA (f_scm);
  f_smob->frame_id = frame_id;
  f_smob->gdbarch = gdbarch;
  f_smob->inferior = inferior;
  f_smob->frame_id_is_next = frame_id_is_next;

  gdbscm_fill_eqable_gsmob_ptr_slot (slot, &f_smob->base);

  return f_scm;
}

/* Create a new <gdb:frame> object that encapsulates FRAME.
   A Scheme exception is thrown if there is an error.  */

static SCM
frscm_scm_from_frame_unsafe (struct frame_info *frame,
			     struct inferior *inferior)
{
  SCM f_scm = frscm_scm_from_frame (frame, inferior);

  if (gdbscm_is_exception (f_scm))
    gdbscm_throw (f_scm);

  return f_scm;
}

/* Returns the <gdb:frame> object in SELF.
   Throws an exception if SELF is not a <gdb:frame> object.  */

static SCM
frscm_get_frame_arg_unsafe (SCM self, int arg_pos, const char *func_name)
{
  SCM_ASSERT_TYPE (frscm_is_frame (self), self, arg_pos, func_name,
		   frame_smob_name);

  return self;
}

/* There is no gdbscm_scm_to_frame function because translating
   a frame SCM object to a struct frame_info * can throw a GDB error.
   Thus code working with frames has to handle both Scheme errors (e.g., the
   object is not a frame) and GDB errors (e.g., the frame lookup failed).

   To help keep things clear we split what would be gdbscm_scm_to_frame
   into two:

   frscm_get_frame_smob_arg_unsafe
     - throws a Scheme error if object is not a frame,
       or if the inferior is gone or is no longer current

   frscm_frame_smob_to_frame
     - may throw a gdb error if the conversion fails
     - it's not clear when it will and won't throw a GDB error,
       but for robustness' sake we assume that whenever we call out to GDB
       a GDB error may get thrown (and thus the call must be wrapped in a
       TRY_CATCH)  */

/* Returns the frame_smob for the object wrapped by FRAME_SCM.
   A Scheme error is thrown if FRAME_SCM is not a frame.  */

frame_smob *
frscm_get_frame_smob_arg_unsafe (SCM self, int arg_pos, const char *func_name)
{
  SCM f_scm = frscm_get_frame_arg_unsafe (self, arg_pos, func_name);
  frame_smob *f_smob = (frame_smob *) SCM_SMOB_DATA (f_scm);

  if (f_smob->inferior == NULL)
    {
      gdbscm_invalid_object_error (func_name, arg_pos, self,
				   _("inferior"));
    }
  if (f_smob->inferior != current_inferior ())
    scm_misc_error (func_name, _("inferior has changed"), SCM_EOL);

  return f_smob;
}

/* Returns the frame_info object wrapped by F_SMOB.
   If the frame doesn't exist anymore (the frame id doesn't
   correspond to any frame in the inferior), returns NULL.
   This function calls GDB routines, so don't assume a GDB error will
   not be thrown.  */

struct frame_info_ptr
frscm_frame_smob_to_frame (frame_smob *f_smob)
{
  frame_info_ptr frame = frame_find_by_id (f_smob->frame_id);
  if (frame == NULL)
    return NULL;

  if (f_smob->frame_id_is_next)
    frame = get_prev_frame (frame);

  return frame;
}


/* Frame methods.  */

/* (frame-valid? <gdb:frame>) -> bool
   Returns #t if the frame corresponding to the frame_id of this
   object still exists in the inferior.  */

static SCM
gdbscm_frame_valid_p (SCM self)
{
  frame_smob *f_smob;
  bool result = false;

  f_smob = frscm_get_frame_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  gdbscm_gdb_exception exc {};
  try
    {
      frame_info_ptr frame = frscm_frame_smob_to_frame (f_smob);
      result = frame != nullptr;
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  return scm_from_bool (result);
}

/* (frame-name <gdb:frame>) -> string
   Returns the name of the function corresponding to this frame,
   or #f if there is no function.  */

static SCM
gdbscm_frame_name (SCM self)
{
  frame_smob *f_smob;
  gdb::unique_xmalloc_ptr<char> name;
  enum language lang = language_minimal;
  bool found = false;
  SCM result;

  f_smob = frscm_get_frame_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  gdbscm_gdb_exception exc {};
  try
    {
      frame_info_ptr frame = frscm_frame_smob_to_frame (f_smob);
      if (frame != NULL)
	{
	  found = true;
	  name = find_frame_funname (frame, &lang, NULL);
	}
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  if (!found)
    {
      gdbscm_invalid_object_error (FUNC_NAME, SCM_ARG1, self,
				   _("<gdb:frame>"));
    }

  if (name != NULL)
    result = gdbscm_scm_from_c_string (name.get ());
  else
    result = SCM_BOOL_F;

  return result;
}

/* (frame-type <gdb:frame>) -> integer
   Returns the frame type, namely one of the gdb:*_FRAME constants.  */

static SCM
gdbscm_frame_type (SCM self)
{
  frame_smob *f_smob;
  enum frame_type type = NORMAL_FRAME;
  bool found = false;

  f_smob = frscm_get_frame_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  gdbscm_gdb_exception exc {};
  try
    {
      frame_info_ptr frame = frscm_frame_smob_to_frame (f_smob);
      if (frame != NULL)
	{
	  found = true;
	  type = get_frame_type (frame);
	}
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  if (!found)
    {
      gdbscm_invalid_object_error (FUNC_NAME, SCM_ARG1, self,
				   _("<gdb:frame>"));
    }

  return scm_from_int (type);
}

/* (frame-arch <gdb:frame>) -> <gdb:architecture>
   Returns the frame's architecture as a gdb:architecture object.  */

static SCM
gdbscm_frame_arch (SCM self)
{
  frame_smob *f_smob;
  bool found = false;

  f_smob = frscm_get_frame_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  gdbscm_gdb_exception exc {};
  try
    {
      frame_info_ptr frame = frscm_frame_smob_to_frame (f_smob);
      found = frame != nullptr;
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  if (!found)
    {
      gdbscm_invalid_object_error (FUNC_NAME, SCM_ARG1, self,
				   _("<gdb:frame>"));
    }

  return arscm_scm_from_arch (f_smob->gdbarch);
}

/* (frame-unwind-stop-reason <gdb:frame>) -> integer
   Returns one of the gdb:FRAME_UNWIND_* constants.  */

static SCM
gdbscm_frame_unwind_stop_reason (SCM self)
{
  frame_smob *f_smob;
  bool found = false;
  enum unwind_stop_reason stop_reason = UNWIND_NO_REASON;

  f_smob = frscm_get_frame_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  gdbscm_gdb_exception exc {};
  try
    {
      frame_info_ptr frame = frscm_frame_smob_to_frame (f_smob);
      if (frame != nullptr)
	{
	  found = true;
	  stop_reason = get_frame_unwind_stop_reason (frame);
	}
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  if (!found)
    {
      gdbscm_invalid_object_error (FUNC_NAME, SCM_ARG1, self,
				   _("<gdb:frame>"));
    }

  return scm_from_int (stop_reason);
}

/* (frame-pc <gdb:frame>) -> integer
   Returns the frame's resume address.  */

static SCM
gdbscm_frame_pc (SCM self)
{
  frame_smob *f_smob;
  CORE_ADDR pc = 0;
  bool found = false;

  f_smob = frscm_get_frame_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  gdbscm_gdb_exception exc {};
  try
    {
      frame_info_ptr frame = frscm_frame_smob_to_frame (f_smob);
      if (frame != NULL)
	{
	  pc = get_frame_pc (frame);
	  found = true;
	}
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  if (!found)
    {
      gdbscm_invalid_object_error (FUNC_NAME, SCM_ARG1, self,
				   _("<gdb:frame>"));
    }

  return gdbscm_scm_from_ulongest (pc);
}

/* (frame-block <gdb:frame>) -> <gdb:block>
   Returns the frame's code block, or #f if one cannot be found.  */

static SCM
gdbscm_frame_block (SCM self)
{
  frame_smob *f_smob;
  const struct block *block = NULL, *fn_block;
  bool found = false;

  f_smob = frscm_get_frame_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  gdbscm_gdb_exception exc {};
  try
    {
      frame_info_ptr frame = frscm_frame_smob_to_frame (f_smob);
      if (frame != NULL)
	{
	  found = true;
	  block = get_frame_block (frame, NULL);
	}
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  if (!found)
    {
      gdbscm_invalid_object_error (FUNC_NAME, SCM_ARG1, self,
				   _("<gdb:frame>"));
    }

  for (fn_block = block;
       fn_block != NULL && fn_block->function () == NULL;
       fn_block = fn_block->superblock ())
    continue;

  if (block == NULL || fn_block == NULL || fn_block->function () == NULL)
    {
      scm_misc_error (FUNC_NAME, _("cannot find block for frame"),
		      scm_list_1 (self));
    }

  if (block != NULL)
    {
      return bkscm_scm_from_block
	(block, fn_block->function ()->objfile ());
    }

  return SCM_BOOL_F;
}

/* (frame-function <gdb:frame>) -> <gdb:symbol>
   Returns the symbol for the function corresponding to this frame,
   or #f if there isn't one.  */

static SCM
gdbscm_frame_function (SCM self)
{
  frame_smob *f_smob;
  struct symbol *sym = NULL;
  bool found = false;

  f_smob = frscm_get_frame_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  gdbscm_gdb_exception exc {};
  try
    {
      frame_info_ptr frame = frscm_frame_smob_to_frame (f_smob);
      if (frame != NULL)
	{
	  found = true;
	  sym = find_pc_function (get_frame_address_in_block (frame));
	}
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  if (!found)
    {
      gdbscm_invalid_object_error (FUNC_NAME, SCM_ARG1, self,
				   _("<gdb:frame>"));
    }

  if (sym != NULL)
    return syscm_scm_from_symbol (sym);

  return SCM_BOOL_F;
}

/* (frame-older <gdb:frame>) -> <gdb:frame>
   Returns the frame immediately older (outer) to this frame,
   or #f if there isn't one.  */

static SCM
gdbscm_frame_older (SCM self)
{
  frame_smob *f_smob;
  struct frame_info *prev = NULL;
  bool found = false;

  f_smob = frscm_get_frame_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  gdbscm_gdb_exception exc {};
  try
    {
      frame_info_ptr frame = frscm_frame_smob_to_frame (f_smob);
      if (frame != NULL)
	{
	  found = true;
	  prev = get_prev_frame (frame).get ();
	}
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  if (!found)
    {
      gdbscm_invalid_object_error (FUNC_NAME, SCM_ARG1, self,
				   _("<gdb:frame>"));
    }

  if (prev != NULL)
    return frscm_scm_from_frame_unsafe (prev, f_smob->inferior);

  return SCM_BOOL_F;
}

/* (frame-newer <gdb:frame>) -> <gdb:frame>
   Returns the frame immediately newer (inner) to this frame,
   or #f if there isn't one.  */

static SCM
gdbscm_frame_newer (SCM self)
{
  frame_smob *f_smob;
  struct frame_info *next = NULL;
  bool found = false;

  f_smob = frscm_get_frame_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  gdbscm_gdb_exception exc {};
  try
    {
      frame_info_ptr frame = frscm_frame_smob_to_frame (f_smob);
      if (frame != NULL)
	{
	  found = true;
	  next = get_next_frame (frame).get ();
	}
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  if (!found)
    {
      gdbscm_invalid_object_error (FUNC_NAME, SCM_ARG1, self,
				   _("<gdb:frame>"));
    }

  if (next != NULL)
    return frscm_scm_from_frame_unsafe (next, f_smob->inferior);

  return SCM_BOOL_F;
}

/* (frame-sal <gdb:frame>) -> <gdb:sal>
   Returns the frame's symtab and line.  */

static SCM
gdbscm_frame_sal (SCM self)
{
  frame_smob *f_smob;
  struct symtab_and_line sal;
  bool found = false;

  f_smob = frscm_get_frame_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  gdbscm_gdb_exception exc {};
  try
    {
      frame_info_ptr frame = frscm_frame_smob_to_frame (f_smob);
      if (frame != NULL)
	{
	  found = true;
	  sal = find_frame_sal (frame);
	}
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  if (!found)
    {
      gdbscm_invalid_object_error (FUNC_NAME, SCM_ARG1, self,
				   _("<gdb:frame>"));
    }

  return stscm_scm_from_sal (sal);
}

/* (frame-read-register <gdb:frame> string) -> <gdb:value>
   The register argument must be a string.  */

static SCM
gdbscm_frame_read_register (SCM self, SCM register_scm)
{
  char *register_str;
  struct value *value = NULL;
  bool found = false;
  frame_smob *f_smob;

  f_smob = frscm_get_frame_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  gdbscm_parse_function_args (FUNC_NAME, SCM_ARG2, NULL, "s",
			      register_scm, &register_str);

  gdbscm_gdb_exception except {};

  try
    {
      int regnum;

      frame_info_ptr frame = frscm_frame_smob_to_frame (f_smob);
      if (frame)
	{
	  found = true;
	  regnum = user_reg_map_name_to_regnum (get_frame_arch (frame),
						register_str,
						strlen (register_str));
	  if (regnum >= 0)
	    value = value_of_register (regnum,
				       get_next_frame_sentinel_okay (frame));
	}
    }
  catch (const gdb_exception &ex)
    {
      except = unpack (ex);
    }

  xfree (register_str);
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  if (!found)
    {
      gdbscm_invalid_object_error (FUNC_NAME, SCM_ARG1, self,
				   _("<gdb:frame>"));
    }

  if (value == NULL)
    {
      gdbscm_out_of_range_error (FUNC_NAME, SCM_ARG2, register_scm,
				 _("unknown register"));
    }

  return vlscm_scm_from_value (value);
}

/* (frame-read-var <gdb:frame> <gdb:symbol>) -> <gdb:value>
   (frame-read-var <gdb:frame> string [#:block <gdb:block>]) -> <gdb:value>
   If the optional block argument is provided start the search from that block,
   otherwise search from the frame's current block (determined by examining
   the resume address of the frame).  The variable argument must be a string
   or an instance of a <gdb:symbol>.  The block argument must be an instance of
   <gdb:block>.  */

static SCM
gdbscm_frame_read_var (SCM self, SCM symbol_scm, SCM rest)
{
  SCM keywords[] = { block_keyword, SCM_BOOL_F };
  frame_smob *f_smob;
  int block_arg_pos = -1;
  SCM block_scm = SCM_UNDEFINED;
  struct frame_info *frame = NULL;
  struct symbol *var = NULL;
  const struct block *block = NULL;
  struct value *value = NULL;

  f_smob = frscm_get_frame_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  gdbscm_gdb_exception exc {};
  try
    {
      frame = frscm_frame_smob_to_frame (f_smob).get ();
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  if (frame == NULL)
    {
      gdbscm_invalid_object_error (FUNC_NAME, SCM_ARG1, self,
				   _("<gdb:frame>"));
    }

  gdbscm_parse_function_args (FUNC_NAME, SCM_ARG3, keywords, "#O",
			      rest, &block_arg_pos, &block_scm);

  if (syscm_is_symbol (symbol_scm))
    {
      var = syscm_get_valid_symbol_arg_unsafe (symbol_scm, SCM_ARG2,
					       FUNC_NAME);
      SCM_ASSERT (SCM_UNBNDP (block_scm), block_scm, SCM_ARG3, FUNC_NAME);
    }
  else if (scm_is_string (symbol_scm))
    {
      gdbscm_gdb_exception except {};

      if (! SCM_UNBNDP (block_scm))
	{
	  SCM except_scm;

	  gdb_assert (block_arg_pos > 0);
	  block = bkscm_scm_to_block (block_scm, block_arg_pos, FUNC_NAME,
				      &except_scm);
	  if (block == NULL)
	    gdbscm_throw (except_scm);
	}

      {
	gdb::unique_xmalloc_ptr<char> var_name
	  (gdbscm_scm_to_c_string (symbol_scm));
	/* N.B. Between here and the end of the scope, don't do anything
	   to cause a Scheme exception.  */

	try
	  {
	    struct block_symbol lookup_sym;

	    if (block == NULL)
	      block = get_frame_block (frame_info_ptr (frame), NULL);
	    lookup_sym = lookup_symbol (var_name.get (), block, VAR_DOMAIN,
					NULL);
	    var = lookup_sym.symbol;
	    block = lookup_sym.block;
	  }
	catch (const gdb_exception &ex)
	  {
	    except = unpack (ex);
	  }
      }

      GDBSCM_HANDLE_GDB_EXCEPTION (except);

      if (var == NULL)
	gdbscm_out_of_range_error (FUNC_NAME, 0, symbol_scm,
				   _("variable not found"));
    }
  else
    {
      /* Use SCM_ASSERT_TYPE for more consistent error messages.  */
      SCM_ASSERT_TYPE (0, symbol_scm, SCM_ARG1, FUNC_NAME,
		       _("gdb:symbol or string"));
    }

  try
    {
      value = read_var_value (var, block, frame_info_ptr (frame));
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  return vlscm_scm_from_value (value);
}

/* (frame-select <gdb:frame>) -> unspecified
   Select this frame.  */

static SCM
gdbscm_frame_select (SCM self)
{
  frame_smob *f_smob;
  bool found = false;

  f_smob = frscm_get_frame_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  gdbscm_gdb_exception exc {};
  try
    {
      frame_info_ptr frame = frscm_frame_smob_to_frame (f_smob);
      if (frame != NULL)
	{
	  found = true;
	  select_frame (frame);
	}
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  if (!found)
    {
      gdbscm_invalid_object_error (FUNC_NAME, SCM_ARG1, self,
				   _("<gdb:frame>"));
    }

  return SCM_UNSPECIFIED;
}

/* (newest-frame) -> <gdb:frame>
   Returns the newest frame.  */

static SCM
gdbscm_newest_frame (void)
{
  struct frame_info *frame = NULL;

  gdbscm_gdb_exception exc {};
  try
    {
      frame = get_current_frame ().get ();
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  return frscm_scm_from_frame_unsafe (frame, current_inferior ());
}

/* (selected-frame) -> <gdb:frame>
   Returns the selected frame.  */

static SCM
gdbscm_selected_frame (void)
{
  struct frame_info *frame = NULL;

  gdbscm_gdb_exception exc {};
  try
    {
      frame = get_selected_frame (_("No frame is currently selected")).get ();
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  return frscm_scm_from_frame_unsafe (frame, current_inferior ());
}

/* (unwind-stop-reason-string integer) -> string
   Return a string explaining the unwind stop reason.  */

static SCM
gdbscm_unwind_stop_reason_string (SCM reason_scm)
{
  int reason;
  const char *str;

  gdbscm_parse_function_args (FUNC_NAME, SCM_ARG1, NULL, "i",
			      reason_scm, &reason);

  if (reason < UNWIND_FIRST || reason > UNWIND_LAST)
    scm_out_of_range (FUNC_NAME, reason_scm);

  str = unwind_stop_reason_to_string ((enum unwind_stop_reason) reason);
  return gdbscm_scm_from_c_string (str);
}

/* Initialize the Scheme frame support.  */

static const scheme_integer_constant frame_integer_constants[] =
{
#define ENTRY(X) { #X, X }

  ENTRY (NORMAL_FRAME),
  ENTRY (DUMMY_FRAME),
  ENTRY (INLINE_FRAME),
  ENTRY (TAILCALL_FRAME),
  ENTRY (SIGTRAMP_FRAME),
  ENTRY (ARCH_FRAME),
  ENTRY (SENTINEL_FRAME),

#undef ENTRY

#define SET(name, description) \
  { "FRAME_" #name, name },
#include "unwind_stop_reasons.def"
#undef SET

  END_INTEGER_CONSTANTS
};

static const scheme_function frame_functions[] =
{
  { "frame?", 1, 0, 0, as_a_scm_t_subr (gdbscm_frame_p),
    "\
Return #t if the object is a <gdb:frame> object." },

  { "frame-valid?", 1, 0, 0, as_a_scm_t_subr (gdbscm_frame_valid_p),
    "\
Return #t if the object is a valid <gdb:frame> object.\n\
Frames become invalid when the inferior returns to its caller." },

  { "frame-name", 1, 0, 0, as_a_scm_t_subr (gdbscm_frame_name),
    "\
Return the name of the function corresponding to this frame,\n\
or #f if there is no function." },

  { "frame-arch", 1, 0, 0, as_a_scm_t_subr (gdbscm_frame_arch),
    "\
Return the frame's architecture as a <gdb:arch> object." },

  { "frame-type", 1, 0, 0, as_a_scm_t_subr (gdbscm_frame_type),
    "\
Return the frame type, namely one of the gdb:*_FRAME constants." },

  { "frame-unwind-stop-reason", 1, 0, 0,
    as_a_scm_t_subr (gdbscm_frame_unwind_stop_reason),
    "\
Return one of the gdb:FRAME_UNWIND_* constants explaining why\n\
it's not possible to find frames older than this." },

  { "frame-pc", 1, 0, 0, as_a_scm_t_subr (gdbscm_frame_pc),
    "\
Return the frame's resume address." },

  { "frame-block", 1, 0, 0, as_a_scm_t_subr (gdbscm_frame_block),
    "\
Return the frame's code block, or #f if one cannot be found." },

  { "frame-function", 1, 0, 0, as_a_scm_t_subr (gdbscm_frame_function),
    "\
Return the <gdb:symbol> for the function corresponding to this frame,\n\
or #f if there isn't one." },

  { "frame-older", 1, 0, 0, as_a_scm_t_subr (gdbscm_frame_older),
    "\
Return the frame immediately older (outer) to this frame,\n\
or #f if there isn't one." },

  { "frame-newer", 1, 0, 0, as_a_scm_t_subr (gdbscm_frame_newer),
    "\
Return the frame immediately newer (inner) to this frame,\n\
or #f if there isn't one." },

  { "frame-sal", 1, 0, 0, as_a_scm_t_subr (gdbscm_frame_sal),
    "\
Return the frame's symtab-and-line <gdb:sal> object." },

  { "frame-read-var", 2, 0, 1, as_a_scm_t_subr (gdbscm_frame_read_var),
    "\
Return the value of the symbol in the frame.\n\
\n\
  Arguments: <gdb:frame> <gdb:symbol>\n\
	 Or: <gdb:frame> string [#:block <gdb:block>]" },

  { "frame-read-register", 2, 0, 0,
    as_a_scm_t_subr (gdbscm_frame_read_register),
    "\
Return the value of the register in the frame.\n\
\n\
  Arguments: <gdb:frame> string" },

  { "frame-select", 1, 0, 0, as_a_scm_t_subr (gdbscm_frame_select),
    "\
Select this frame." },

  { "newest-frame", 0, 0, 0, as_a_scm_t_subr (gdbscm_newest_frame),
    "\
Return the newest frame." },

  { "selected-frame", 0, 0, 0, as_a_scm_t_subr (gdbscm_selected_frame),
    "\
Return the selected frame." },

  { "unwind-stop-reason-string", 1, 0, 0,
    as_a_scm_t_subr (gdbscm_unwind_stop_reason_string),
    "\
Return a string explaining the unwind stop reason.\n\
\n\
  Arguments: integer (the result of frame-unwind-stop-reason)" },

  END_FUNCTIONS
};

void
gdbscm_initialize_frames (void)
{
  frame_smob_tag
    = gdbscm_make_smob_type (frame_smob_name, sizeof (frame_smob));
  scm_set_smob_free (frame_smob_tag, frscm_free_frame_smob);
  scm_set_smob_print (frame_smob_tag, frscm_print_frame_smob);

  gdbscm_define_integer_constants (frame_integer_constants, 1);
  gdbscm_define_functions (frame_functions, 1);

  block_keyword = scm_from_latin1_keyword ("block");
}
