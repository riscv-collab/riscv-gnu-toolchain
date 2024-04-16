/* Scheme interface to breakpoints.

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
#include "value.h"
#include "breakpoint.h"
#include "gdbcmd.h"
#include "gdbthread.h"
#include "observable.h"
#include "cli/cli-script.h"
#include "ada-lang.h"
#include "arch-utils.h"
#include "language.h"
#include "guile-internal.h"
#include "location.h"

/* The <gdb:breakpoint> smob.
   N.B.: The name of this struct is known to breakpoint.h.

   Note: Breakpoints are added to gdb using a two step process:
   1) Call make-breakpoint to create a <gdb:breakpoint> object.
   2) Call register-breakpoint! to add the breakpoint to gdb.
   It is done this way so that the constructor, make-breakpoint, doesn't have
   any side-effects.  This means that the smob needs to store everything
   that was passed to make-breakpoint.  */

typedef struct gdbscm_breakpoint_object
{
  /* This always appears first.  */
  gdb_smob base;

  /* Non-zero if this breakpoint was created with make-breakpoint.  */
  int is_scheme_bkpt;

  /* For breakpoints created with make-breakpoint, these are the parameters
     that were passed to make-breakpoint.  These values are not used except
     to register the breakpoint with GDB.  */
  struct
  {
    /* The string representation of the breakpoint.
       Space for this lives in GC space.  */
    char *location;

    /* The kind of breakpoint.
       At the moment this can only be one of bp_breakpoint, bp_watchpoint.  */
    enum bptype type;

    /* If a watchpoint, the kind of watchpoint.  */
    enum target_hw_bp_type access_type;

    /* Non-zero if the breakpoint is an "internal" breakpoint.  */
    int is_internal;

    /* Non-zero if the breakpoint is temporary.  */
    int is_temporary;
  } spec;

  /* The breakpoint number according to gdb.
     For breakpoints created from Scheme, this has the value -1 until the
     breakpoint is registered with gdb.
     This is recorded here because BP will be NULL when deleted.  */
  int number;

  /* The gdb breakpoint object, or NULL if the breakpoint has not been
     registered yet, or has been deleted.  */
  struct breakpoint *bp;

  /* Backlink to our containing <gdb:breakpoint> smob.
     This is needed when we are deleted, we need to unprotect the object
     from GC.  */
  SCM containing_scm;

  /* A stop condition or #f.  */
  SCM stop;
} breakpoint_smob;

static const char breakpoint_smob_name[] = "gdb:breakpoint";

/* The tag Guile knows the breakpoint smob by.  */
static scm_t_bits breakpoint_smob_tag;

/* Variables used to pass information between the breakpoint_smob
   constructor and the breakpoint-created hook function.  */
static SCM pending_breakpoint_scm = SCM_BOOL_F;

/* Keywords used by create-breakpoint!.  */
static SCM type_keyword;
static SCM wp_class_keyword;
static SCM internal_keyword;
static SCM temporary_keyword;

/* Administrivia for breakpoint smobs.  */

/* The smob "free" function for <gdb:breakpoint>.  */

static size_t
bpscm_free_breakpoint_smob (SCM self)
{
  breakpoint_smob *bp_smob = (breakpoint_smob *) SCM_SMOB_DATA (self);

  if (bp_smob->bp)
    bp_smob->bp->scm_bp_object = NULL;

  /* Not necessary, done to catch bugs.  */
  bp_smob->bp = NULL;
  bp_smob->containing_scm = SCM_UNDEFINED;
  bp_smob->stop = SCM_UNDEFINED;

  return 0;
}

/* Return the name of TYPE.
   This doesn't handle all types, just the ones we export.  */

static const char *
bpscm_type_to_string (enum bptype type)
{
  switch (type)
    {
    case bp_none: return "BP_NONE";
    case bp_breakpoint: return "BP_BREAKPOINT";
    case bp_watchpoint: return "BP_WATCHPOINT";
    case bp_hardware_watchpoint: return "BP_HARDWARE_WATCHPOINT";
    case bp_read_watchpoint: return "BP_READ_WATCHPOINT";
    case bp_access_watchpoint: return "BP_ACCESS_WATCHPOINT";
    case bp_catchpoint: return "BP_CATCHPOINT";
    default: return "internal/other";
    }
}

/* Return the name of ENABLE_STATE.  */

static const char *
bpscm_enable_state_to_string (enum enable_state enable_state)
{
  switch (enable_state)
    {
    case bp_disabled: return "disabled";
    case bp_enabled: return "enabled";
    case bp_call_disabled: return "call_disabled";
    default: return "unknown";
    }
}

/* The smob "print" function for <gdb:breakpoint>.  */

static int
bpscm_print_breakpoint_smob (SCM self, SCM port, scm_print_state *pstate)
{
  breakpoint_smob *bp_smob = (breakpoint_smob *) SCM_SMOB_DATA (self);
  struct breakpoint *b = bp_smob->bp;

  gdbscm_printf (port, "#<%s", breakpoint_smob_name);

  /* Only print what we export to the user.
     The rest are possibly internal implementation details.  */

  gdbscm_printf (port, " #%d", bp_smob->number);

  /* Careful, the breakpoint may be invalid.  */
  if (b != NULL)
    {
      gdbscm_printf (port, " %s %s %s",
		     bpscm_type_to_string (b->type),
		     bpscm_enable_state_to_string (b->enable_state),
		     b->silent ? "silent" : "noisy");

      gdbscm_printf (port, " hit:%d", b->hit_count);
      gdbscm_printf (port, " ignore:%d", b->ignore_count);

      if (b->locspec != nullptr)
	{
	  const char *str = b->locspec->to_string ();
	  if (str != nullptr)
	    gdbscm_printf (port, " @%s", str);
	}
    }

  scm_puts (">", port);

  scm_remember_upto_here_1 (self);

  /* Non-zero means success.  */
  return 1;
}

/* Low level routine to create a <gdb:breakpoint> object.  */

static SCM
bpscm_make_breakpoint_smob (void)
{
  breakpoint_smob *bp_smob = (breakpoint_smob *)
    scm_gc_malloc (sizeof (breakpoint_smob), breakpoint_smob_name);
  SCM bp_scm;

  memset (bp_smob, 0, sizeof (*bp_smob));
  bp_smob->number = -1;
  bp_smob->stop = SCM_BOOL_F;
  bp_scm = scm_new_smob (breakpoint_smob_tag, (scm_t_bits) bp_smob);
  bp_smob->containing_scm = bp_scm;
  gdbscm_init_gsmob (&bp_smob->base);

  return bp_scm;
}

/* Return non-zero if we want a Scheme wrapper for breakpoint B.
   If FROM_SCHEME is non-zero,this is called for a breakpoint created
   by the user from Scheme.  Otherwise it is zero.  */

static int
bpscm_want_scm_wrapper_p (struct breakpoint *bp, int from_scheme)
{
  /* Don't create <gdb:breakpoint> objects for internal GDB breakpoints.  */
  if (bp->number < 0 && !from_scheme)
    return 0;

  /* The others are not supported.  */
  if (bp->type != bp_breakpoint
      && bp->type != bp_watchpoint
      && bp->type != bp_hardware_watchpoint
      && bp->type != bp_read_watchpoint
      && bp->type != bp_access_watchpoint
      && bp->type != bp_catchpoint)
    return 0;

  return 1;
}

/* Install the Scheme side of a breakpoint, CONTAINING_SCM, in
   the gdb side BP.  */

static void
bpscm_attach_scm_to_breakpoint (struct breakpoint *bp, SCM containing_scm)
{
  breakpoint_smob *bp_smob;

  bp_smob = (breakpoint_smob *) SCM_SMOB_DATA (containing_scm);
  bp_smob->number = bp->number;
  bp_smob->bp = bp;
  bp_smob->containing_scm = containing_scm;
  bp_smob->bp->scm_bp_object = bp_smob;

  /* The owner of this breakpoint is not in GC-controlled memory, so we need
     to protect it from GC until the breakpoint is deleted.  */
  scm_gc_protect_object (containing_scm);
}

/* Return non-zero if SCM is a breakpoint smob.  */

static int
bpscm_is_breakpoint (SCM scm)
{
  return SCM_SMOB_PREDICATE (breakpoint_smob_tag, scm);
}

/* (breakpoint? scm) -> boolean */

static SCM
gdbscm_breakpoint_p (SCM scm)
{
  return scm_from_bool (bpscm_is_breakpoint (scm));
}

/* Returns the <gdb:breakpoint> object in SELF.
   Throws an exception if SELF is not a <gdb:breakpoint> object.  */

static SCM
bpscm_get_breakpoint_arg_unsafe (SCM self, int arg_pos, const char *func_name)
{
  SCM_ASSERT_TYPE (bpscm_is_breakpoint (self), self, arg_pos, func_name,
		   breakpoint_smob_name);

  return self;
}

/* Returns a pointer to the breakpoint smob of SELF.
   Throws an exception if SELF is not a <gdb:breakpoint> object.  */

static breakpoint_smob *
bpscm_get_breakpoint_smob_arg_unsafe (SCM self, int arg_pos,
				      const char *func_name)
{
  SCM bp_scm = bpscm_get_breakpoint_arg_unsafe (self, arg_pos, func_name);
  breakpoint_smob *bp_smob = (breakpoint_smob *) SCM_SMOB_DATA (bp_scm);

  return bp_smob;
}

/* Return non-zero if breakpoint BP_SMOB is valid.  */

static int
bpscm_is_valid (breakpoint_smob *bp_smob)
{
  return bp_smob->bp != NULL;
}

/* Returns the breakpoint smob in SELF, verifying it's valid.
   Throws an exception if SELF is not a <gdb:breakpoint> object,
   or is invalid.  */

static breakpoint_smob *
bpscm_get_valid_breakpoint_smob_arg_unsafe (SCM self, int arg_pos,
					    const char *func_name)
{
  breakpoint_smob *bp_smob
    = bpscm_get_breakpoint_smob_arg_unsafe (self, arg_pos, func_name);

  if (!bpscm_is_valid (bp_smob))
    {
      gdbscm_invalid_object_error (func_name, arg_pos, self,
				   _("<gdb:breakpoint>"));
    }

  return bp_smob;
}

/* Breakpoint methods.  */

/* (make-breakpoint string [#:type integer] [#:wp-class integer]
    [#:internal boolean] [#:temporary boolean]) -> <gdb:breakpoint>

   The result is the <gdb:breakpoint> Scheme object.
   The breakpoint is not available to be used yet, however.
   It must still be added to gdb with register-breakpoint!.  */

static SCM
gdbscm_make_breakpoint (SCM location_scm, SCM rest)
{
  const SCM keywords[] = {
    type_keyword, wp_class_keyword, internal_keyword,
    temporary_keyword, SCM_BOOL_F
  };
  char *s;
  char *location;
  int type_arg_pos = -1, access_type_arg_pos = -1,
      internal_arg_pos = -1, temporary_arg_pos = -1;
  int type = bp_breakpoint;
  int access_type = hw_write;
  int internal = 0;
  int temporary = 0;
  SCM result;
  breakpoint_smob *bp_smob;

  gdbscm_parse_function_args (FUNC_NAME, SCM_ARG1, keywords, "s#iitt",
			      location_scm, &location, rest,
			      &type_arg_pos, &type,
			      &access_type_arg_pos, &access_type,
			      &internal_arg_pos, &internal,
			      &temporary_arg_pos, &temporary);

  result = bpscm_make_breakpoint_smob ();
  bp_smob = (breakpoint_smob *) SCM_SMOB_DATA (result);

  s = location;
  location = gdbscm_gc_xstrdup (s);
  xfree (s);

  switch (type)
    {
    case bp_breakpoint:
      if (access_type_arg_pos > 0)
	{
	  gdbscm_misc_error (FUNC_NAME, access_type_arg_pos,
			     scm_from_int (access_type),
			     _("access type with breakpoint is not allowed"));
	}
      break;
    case bp_watchpoint:
      switch (access_type)
	{
	case hw_write:
	case hw_access:
	case hw_read:
	  break;
	default:
	  gdbscm_out_of_range_error (FUNC_NAME, access_type_arg_pos,
				     scm_from_int (access_type),
				     _("invalid watchpoint class"));
	}
      break;
    case bp_none:
    case bp_hardware_watchpoint:
    case bp_read_watchpoint:
    case bp_access_watchpoint:
    case bp_catchpoint:
      {
	const char *type_name = bpscm_type_to_string ((enum bptype) type);
	gdbscm_misc_error (FUNC_NAME, type_arg_pos,
			   gdbscm_scm_from_c_string (type_name),
			   _("unsupported breakpoint type"));
      }
      break;
    default:
      gdbscm_out_of_range_error (FUNC_NAME, type_arg_pos,
				 scm_from_int (type),
				 _("invalid breakpoint type"));
    }

  bp_smob->is_scheme_bkpt = 1;
  bp_smob->spec.location = location;
  bp_smob->spec.type = (enum bptype) type;
  bp_smob->spec.access_type = (enum target_hw_bp_type) access_type;
  bp_smob->spec.is_internal = internal;
  bp_smob->spec.is_temporary = temporary;

  return result;
}

/* (register-breakpoint! <gdb:breakpoint>) -> unspecified

   It is an error to register a breakpoint created outside of Guile,
   or an already-registered breakpoint.  */

static SCM
gdbscm_register_breakpoint_x (SCM self)
{
  breakpoint_smob *bp_smob
    = bpscm_get_breakpoint_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  gdbscm_gdb_exception except {};
  const char *location, *copy;

  /* We only support registering breakpoints created with make-breakpoint.  */
  if (!bp_smob->is_scheme_bkpt)
    scm_misc_error (FUNC_NAME, _("not a Scheme breakpoint"), SCM_EOL);

  if (bpscm_is_valid (bp_smob))
    scm_misc_error (FUNC_NAME, _("breakpoint is already registered"), SCM_EOL);

  pending_breakpoint_scm = self;
  location = bp_smob->spec.location;
  copy = skip_spaces (location);
  location_spec_up locspec
    = string_to_location_spec_basic (&copy,
				     current_language,
				     symbol_name_match_type::WILD);

  try
    {
      int internal = bp_smob->spec.is_internal;
      int temporary = bp_smob->spec.is_temporary;

      switch (bp_smob->spec.type)
	{
	case bp_breakpoint:
	  {
	    const breakpoint_ops *ops =
	      breakpoint_ops_for_location_spec (locspec.get (), false);
	    create_breakpoint (get_current_arch (),
			       locspec.get (), NULL, -1, -1, NULL, false,
			       0,
			       temporary, bp_breakpoint,
			       0,
			       AUTO_BOOLEAN_TRUE,
			       ops,
			       0, 1, internal, 0);
	    break;
	  }
	case bp_watchpoint:
	  {
	    enum target_hw_bp_type access_type = bp_smob->spec.access_type;

	    if (access_type == hw_write)
	      watch_command_wrapper (location, 0, internal);
	    else if (access_type == hw_access)
	      awatch_command_wrapper (location, 0, internal);
	    else if (access_type == hw_read)
	      rwatch_command_wrapper (location, 0, internal);
	    else
	      gdb_assert_not_reached ("invalid access type");
	    break;
	  }
	default:
	  gdb_assert_not_reached ("invalid breakpoint type");
	}
    }
  catch (const gdb_exception &ex)
    {
      except = unpack (ex);
    }

  /* Ensure this gets reset, even if there's an error.  */
  pending_breakpoint_scm = SCM_BOOL_F;
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  return SCM_UNSPECIFIED;
}

/* (delete-breakpoint! <gdb:breakpoint>) -> unspecified
   Scheme function which deletes (removes) the underlying GDB breakpoint
   from GDB's list of breakpoints.  This triggers the breakpoint_deleted
   observer which will call gdbscm_breakpoint_deleted; that function cleans
   up the Scheme bits.  */

static SCM
gdbscm_delete_breakpoint_x (SCM self)
{
  breakpoint_smob *bp_smob
    = bpscm_get_valid_breakpoint_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  gdbscm_gdb_exception exc {};
  try
    {
      delete_breakpoint (bp_smob->bp);
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  return SCM_UNSPECIFIED;
}

/* iterate_over_breakpoints function for gdbscm_breakpoints.  */

static void
bpscm_build_bp_list (struct breakpoint *bp, SCM *list)
{
  breakpoint_smob *bp_smob = bp->scm_bp_object;

  /* Lazily create wrappers for breakpoints created outside Scheme.  */

  if (bp_smob == NULL)
    {
      if (bpscm_want_scm_wrapper_p (bp, 0))
	{
	  SCM bp_scm;

	  bp_scm = bpscm_make_breakpoint_smob ();
	  bpscm_attach_scm_to_breakpoint (bp, bp_scm);
	  /* Refetch it.  */
	  bp_smob = bp->scm_bp_object;
	}
    }

  /* Not all breakpoints will have a companion Scheme object.
     Only breakpoints that trigger the created_breakpoint observer call,
     and satisfy certain conditions (see bpscm_want_scm_wrapper_p),
     get a companion object (this includes Scheme-created breakpoints).  */

  if (bp_smob != NULL)
    *list = scm_cons (bp_smob->containing_scm, *list);
}

/* (breakpoints) -> list
   Return a list of all breakpoints.  */

static SCM
gdbscm_breakpoints (void)
{
  SCM list = SCM_EOL;

  for (breakpoint &bp : all_breakpoints ())
    bpscm_build_bp_list (&bp, &list);

  return scm_reverse_x (list, SCM_EOL);
}

/* (breakpoint-valid? <gdb:breakpoint>) -> boolean
   Returns #t if SELF is still valid.  */

static SCM
gdbscm_breakpoint_valid_p (SCM self)
{
  breakpoint_smob *bp_smob
    = bpscm_get_breakpoint_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  return scm_from_bool (bpscm_is_valid (bp_smob));
}

/* (breakpoint-enabled? <gdb:breakpoint>) -> boolean */

static SCM
gdbscm_breakpoint_enabled_p (SCM self)
{
  breakpoint_smob *bp_smob
    = bpscm_get_valid_breakpoint_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  return scm_from_bool (bp_smob->bp->enable_state == bp_enabled);
}

/* (set-breakpoint-enabled? <gdb:breakpoint> boolean) -> unspecified */

static SCM
gdbscm_set_breakpoint_enabled_x (SCM self, SCM newvalue)
{
  breakpoint_smob *bp_smob
    = bpscm_get_valid_breakpoint_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  SCM_ASSERT_TYPE (gdbscm_is_bool (newvalue), newvalue, SCM_ARG2, FUNC_NAME,
		   _("boolean"));

  gdbscm_gdb_exception exc {};
  try
    {
      if (gdbscm_is_true (newvalue))
	enable_breakpoint (bp_smob->bp);
      else
	disable_breakpoint (bp_smob->bp);
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  return SCM_UNSPECIFIED;
}

/* (breakpoint-silent? <gdb:breakpoint>) -> boolean */

static SCM
gdbscm_breakpoint_silent_p (SCM self)
{
  breakpoint_smob *bp_smob
    = bpscm_get_valid_breakpoint_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  return scm_from_bool (bp_smob->bp->silent);
}

/* (set-breakpoint-silent?! <gdb:breakpoint> boolean) -> unspecified */

static SCM
gdbscm_set_breakpoint_silent_x (SCM self, SCM newvalue)
{
  breakpoint_smob *bp_smob
    = bpscm_get_valid_breakpoint_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  SCM_ASSERT_TYPE (gdbscm_is_bool (newvalue), newvalue, SCM_ARG2, FUNC_NAME,
		   _("boolean"));

  gdbscm_gdb_exception exc {};
  try
    {
      breakpoint_set_silent (bp_smob->bp, gdbscm_is_true (newvalue));
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  return SCM_UNSPECIFIED;
}

/* (breakpoint-ignore-count <gdb:breakpoint>) -> integer */

static SCM
gdbscm_breakpoint_ignore_count (SCM self)
{
  breakpoint_smob *bp_smob
    = bpscm_get_valid_breakpoint_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  return scm_from_long (bp_smob->bp->ignore_count);
}

/* (set-breakpoint-ignore-count! <gdb:breakpoint> integer)
     -> unspecified */

static SCM
gdbscm_set_breakpoint_ignore_count_x (SCM self, SCM newvalue)
{
  breakpoint_smob *bp_smob
    = bpscm_get_valid_breakpoint_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  long value;

  SCM_ASSERT_TYPE (scm_is_signed_integer (newvalue, LONG_MIN, LONG_MAX),
		   newvalue, SCM_ARG2, FUNC_NAME, _("integer"));

  value = scm_to_long (newvalue);
  if (value < 0)
    value = 0;

  gdbscm_gdb_exception exc {};
  try
    {
      set_ignore_count (bp_smob->number, (int) value, 0);
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  return SCM_UNSPECIFIED;
}

/* (breakpoint-hit-count <gdb:breakpoint>) -> integer */

static SCM
gdbscm_breakpoint_hit_count (SCM self)
{
  breakpoint_smob *bp_smob
    = bpscm_get_valid_breakpoint_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  return scm_from_long (bp_smob->bp->hit_count);
}

/* (set-breakpoint-hit-count! <gdb:breakpoint> integer) -> unspecified */

static SCM
gdbscm_set_breakpoint_hit_count_x (SCM self, SCM newvalue)
{
  breakpoint_smob *bp_smob
    = bpscm_get_valid_breakpoint_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  long value;

  SCM_ASSERT_TYPE (scm_is_signed_integer (newvalue, LONG_MIN, LONG_MAX),
		   newvalue, SCM_ARG2, FUNC_NAME, _("integer"));

  value = scm_to_long (newvalue);
  if (value < 0)
    value = 0;

  if (value != 0)
    {
      gdbscm_out_of_range_error (FUNC_NAME, SCM_ARG2, newvalue,
				 _("hit-count must be zero"));
    }

  bp_smob->bp->hit_count = 0;

  return SCM_UNSPECIFIED;
}

/* (breakpoint-thread <gdb:breakpoint>) -> integer */

static SCM
gdbscm_breakpoint_thread (SCM self)
{
  breakpoint_smob *bp_smob
    = bpscm_get_valid_breakpoint_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  if (bp_smob->bp->thread == -1)
    return SCM_BOOL_F;

  return scm_from_long (bp_smob->bp->thread);
}

/* (set-breakpoint-thread! <gdb:breakpoint> integer) -> unspecified */

static SCM
gdbscm_set_breakpoint_thread_x (SCM self, SCM newvalue)
{
  breakpoint_smob *bp_smob
    = bpscm_get_valid_breakpoint_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  long id;

  if (scm_is_signed_integer (newvalue, LONG_MIN, LONG_MAX))
    {
      id = scm_to_long (newvalue);
      if (!valid_global_thread_id (id))
	{
	  gdbscm_out_of_range_error (FUNC_NAME, SCM_ARG2, newvalue,
				     _("invalid thread id"));
	}

      if (bp_smob->bp->task != -1)
	scm_misc_error (FUNC_NAME,
			_("cannot set both task and thread attributes"),
			SCM_EOL);
    }
  else if (gdbscm_is_false (newvalue))
    id = -1;
  else
    SCM_ASSERT_TYPE (0, newvalue, SCM_ARG2, FUNC_NAME, _("integer or #f"));

  if (bp_smob->bp->inferior != -1 && id != -1)
    scm_misc_error (FUNC_NAME,
		    _("Cannot have both 'thread' and 'inferior' "
		      "conditions on a breakpoint"), SCM_EOL);

  breakpoint_set_thread (bp_smob->bp, id);

  return SCM_UNSPECIFIED;
}

/* (breakpoint-task <gdb:breakpoint>) -> integer */

static SCM
gdbscm_breakpoint_task (SCM self)
{
  breakpoint_smob *bp_smob
    = bpscm_get_valid_breakpoint_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  if (bp_smob->bp->task == -1)
    return SCM_BOOL_F;

  return scm_from_long (bp_smob->bp->task);
}

/* (set-breakpoint-task! <gdb:breakpoint> integer) -> unspecified */

static SCM
gdbscm_set_breakpoint_task_x (SCM self, SCM newvalue)
{
  breakpoint_smob *bp_smob
    = bpscm_get_valid_breakpoint_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  long id;
  int valid_id = 0;

  if (scm_is_signed_integer (newvalue, LONG_MIN, LONG_MAX))
    {
      id = scm_to_long (newvalue);

      gdbscm_gdb_exception exc {};
      try
	{
	  valid_id = valid_task_id (id);
	}
      catch (const gdb_exception &except)
	{
	  exc = unpack (except);
	}

      GDBSCM_HANDLE_GDB_EXCEPTION (exc);
      if (! valid_id)
	{
	  gdbscm_out_of_range_error (FUNC_NAME, SCM_ARG2, newvalue,
				     _("invalid task id"));
	}

      if (bp_smob->bp->thread != -1)
	scm_misc_error (FUNC_NAME,
			_("cannot set both task and thread attributes"),
			SCM_EOL);
    }
  else if (gdbscm_is_false (newvalue))
    id = -1;
  else
    SCM_ASSERT_TYPE (0, newvalue, SCM_ARG2, FUNC_NAME, _("integer or #f"));

  gdbscm_gdb_exception exc {};
  try
    {
      breakpoint_set_task (bp_smob->bp, id);
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  return SCM_UNSPECIFIED;
}

/* (breakpoint-location <gdb:breakpoint>) -> string */

static SCM
gdbscm_breakpoint_location (SCM self)
{
  breakpoint_smob *bp_smob
    = bpscm_get_valid_breakpoint_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  if (bp_smob->bp->type != bp_breakpoint)
    return SCM_BOOL_F;

  const char *str = bp_smob->bp->locspec->to_string ();
  if (str == nullptr)
    str = "";

  return gdbscm_scm_from_c_string (str);
}

/* (breakpoint-expression <gdb:breakpoint>) -> string
   This is only valid for watchpoints.
   Returns #f for non-watchpoints.  */

static SCM
gdbscm_breakpoint_expression (SCM self)
{
  breakpoint_smob *bp_smob
    = bpscm_get_valid_breakpoint_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  if (!is_watchpoint (bp_smob->bp))
    return SCM_BOOL_F;

  watchpoint *wp = gdb::checked_static_cast<watchpoint *> (bp_smob->bp);

  const char *str = wp->exp_string.get ();
  if (! str)
    str = "";

  return gdbscm_scm_from_c_string (str);
}

/* (breakpoint-condition <gdb:breakpoint>) -> string */

static SCM
gdbscm_breakpoint_condition (SCM self)
{
  breakpoint_smob *bp_smob
    = bpscm_get_valid_breakpoint_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  char *str;

  str = bp_smob->bp->cond_string.get ();
  if (! str)
    return SCM_BOOL_F;

  return gdbscm_scm_from_c_string (str);
}

/* (set-breakpoint-condition! <gdb:breakpoint> string|#f)
   -> unspecified */

static SCM
gdbscm_set_breakpoint_condition_x (SCM self, SCM newvalue)
{
  breakpoint_smob *bp_smob
    = bpscm_get_valid_breakpoint_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  SCM_ASSERT_TYPE (scm_is_string (newvalue) || gdbscm_is_false (newvalue),
		   newvalue, SCM_ARG2, FUNC_NAME,
		   _("string or #f"));

  return gdbscm_wrap ([=]
    {
      gdb::unique_xmalloc_ptr<char> exp
	= (gdbscm_is_false (newvalue)
	   ? nullptr
	   : gdbscm_scm_to_c_string (newvalue));

      set_breakpoint_condition (bp_smob->bp, exp ? exp.get () : "", 0, false);

      return SCM_UNSPECIFIED;
    });
}

/* (breakpoint-stop <gdb:breakpoint>) -> procedure or #f */

static SCM
gdbscm_breakpoint_stop (SCM self)
{
  breakpoint_smob *bp_smob
    = bpscm_get_valid_breakpoint_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  return bp_smob->stop;
}

/* (set-breakpoint-stop! <gdb:breakpoint> procedure|#f)
   -> unspecified */

static SCM
gdbscm_set_breakpoint_stop_x (SCM self, SCM newvalue)
{
  breakpoint_smob *bp_smob
    = bpscm_get_valid_breakpoint_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  const struct extension_language_defn *extlang = NULL;

  SCM_ASSERT_TYPE (gdbscm_is_procedure (newvalue)
		   || gdbscm_is_false (newvalue),
		   newvalue, SCM_ARG2, FUNC_NAME,
		   _("procedure or #f"));

  if (bp_smob->bp->cond_string != NULL)
    extlang = get_ext_lang_defn (EXT_LANG_GDB);
  if (extlang == NULL)
    extlang = get_breakpoint_cond_ext_lang (bp_smob->bp, EXT_LANG_GUILE);
  if (extlang != NULL)
    {
      char *error_text
	= xstrprintf (_("Only one stop condition allowed.  There is"
			" currently a %s stop condition defined for"
			" this breakpoint."),
		      ext_lang_capitalized_name (extlang)).release ();

      scm_dynwind_begin ((scm_t_dynwind_flags) 0);
      gdbscm_dynwind_xfree (error_text);
      gdbscm_out_of_range_error (FUNC_NAME, SCM_ARG1, self, error_text);
      /* The following line, while unnecessary, is present for completeness
	 sake.  */
      scm_dynwind_end ();
    }

  bp_smob->stop = newvalue;

  return SCM_UNSPECIFIED;
}

/* (breakpoint-commands <gdb:breakpoint>) -> string */

static SCM
gdbscm_breakpoint_commands (SCM self)
{
  breakpoint_smob *bp_smob
    = bpscm_get_valid_breakpoint_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct breakpoint *bp;
  SCM result;

  bp = bp_smob->bp;

  if (bp->commands == NULL)
    return SCM_BOOL_F;

  string_file buf;

  gdbscm_gdb_exception exc {};
  try
    {
      ui_out_redirect_pop redir (current_uiout, &buf);
      print_command_lines (current_uiout, breakpoint_commands (bp), 0);
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  result = gdbscm_scm_from_c_string (buf.c_str ());

  return result;
}

/* (breakpoint-type <gdb:breakpoint>) -> integer */

static SCM
gdbscm_breakpoint_type (SCM self)
{
  breakpoint_smob *bp_smob
    = bpscm_get_valid_breakpoint_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  return scm_from_long (bp_smob->bp->type);
}

/* (breakpoint-visible? <gdb:breakpoint>) -> boolean */

static SCM
gdbscm_breakpoint_visible (SCM self)
{
  breakpoint_smob *bp_smob
    = bpscm_get_valid_breakpoint_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  return scm_from_bool (bp_smob->bp->number >= 0);
}

/* (breakpoint-number <gdb:breakpoint>) -> integer */

static SCM
gdbscm_breakpoint_number (SCM self)
{
  breakpoint_smob *bp_smob
    = bpscm_get_valid_breakpoint_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  return scm_from_long (bp_smob->number);
}

/* (breakpoint-temporary? <gdb:breakpoint>) -> boolean */

static SCM
gdbscm_breakpoint_temporary (SCM self)
{
  breakpoint_smob *bp_smob
    = bpscm_get_valid_breakpoint_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  return scm_from_bool (bp_smob->bp->disposition == disp_del
			|| bp_smob->bp->disposition == disp_del_at_next_stop);
}

/* Return TRUE if "stop" has been set for this breakpoint.

   This is the extension_language_ops.breakpoint_has_cond "method".  */

int
gdbscm_breakpoint_has_cond (const struct extension_language_defn *extlang,
			    struct breakpoint *b)
{
  breakpoint_smob *bp_smob = b->scm_bp_object;

  if (bp_smob == NULL)
    return 0;

  return gdbscm_is_procedure (bp_smob->stop);
}

/* Call the "stop" method in the breakpoint class.
   This must only be called if gdbscm_breakpoint_has_cond returns true.
   If the stop method returns #t, the inferior will be stopped at the
   breakpoint.  Otherwise the inferior will be allowed to continue
   (assuming other conditions don't indicate "stop").

   This is the extension_language_ops.breakpoint_cond_says_stop "method".  */

enum ext_lang_bp_stop
gdbscm_breakpoint_cond_says_stop
  (const struct extension_language_defn *extlang, struct breakpoint *b)
{
  breakpoint_smob *bp_smob = b->scm_bp_object;
  SCM predicate_result;
  int stop;

  if (bp_smob == NULL)
    return EXT_LANG_BP_STOP_UNSET;
  if (!gdbscm_is_procedure (bp_smob->stop))
    return EXT_LANG_BP_STOP_UNSET;

  stop = 1;

  predicate_result
    = gdbscm_safe_call_1 (bp_smob->stop, bp_smob->containing_scm, NULL);

  if (gdbscm_is_exception (predicate_result))
    ; /* Exception already printed.  */
  /* If the "stop" function returns #f that means
     the Scheme breakpoint wants GDB to continue.  */
  else if (gdbscm_is_false (predicate_result))
    stop = 0;

  return stop ? EXT_LANG_BP_STOP_YES : EXT_LANG_BP_STOP_NO;
}

/* Event callback functions.  */

/* Callback that is used when a breakpoint is created.
   For breakpoints created by Scheme, i.e., gdbscm_create_breakpoint_x, finish
   object creation by connecting the Scheme wrapper to the gdb object.
   We ignore breakpoints created from gdb or python here, we create the
   Scheme wrapper for those when there's a need to, e.g.,
   gdbscm_breakpoints.  */

static void
bpscm_breakpoint_created (struct breakpoint *bp)
{
  SCM bp_scm;

  if (gdbscm_is_false (pending_breakpoint_scm))
    return;

  /* Verify our caller error checked the user's request.  */
  gdb_assert (bpscm_want_scm_wrapper_p (bp, 1));

  bp_scm = pending_breakpoint_scm;
  pending_breakpoint_scm = SCM_BOOL_F;

  bpscm_attach_scm_to_breakpoint (bp, bp_scm);
}

/* Callback that is used when a breakpoint is deleted.  This will
   invalidate the corresponding Scheme object.  */

static void
bpscm_breakpoint_deleted (struct breakpoint *b)
{
  int num = b->number;
  struct breakpoint *bp;

  /* TODO: Why the lookup?  We have B.  */

  bp = get_breakpoint (num);
  if (bp)
    {
      breakpoint_smob *bp_smob = bp->scm_bp_object;

      if (bp_smob)
	{
	  bp_smob->bp = NULL;
	  bp_smob->number = -1;
	  bp_smob->stop = SCM_BOOL_F;
	  scm_gc_unprotect_object (bp_smob->containing_scm);
	}
    }
}

/* Initialize the Scheme breakpoint code.  */

static const scheme_integer_constant breakpoint_integer_constants[] =
{
  { "BP_NONE", bp_none },
  { "BP_BREAKPOINT", bp_breakpoint },
  { "BP_WATCHPOINT", bp_watchpoint },
  { "BP_HARDWARE_WATCHPOINT", bp_hardware_watchpoint },
  { "BP_READ_WATCHPOINT", bp_read_watchpoint },
  { "BP_ACCESS_WATCHPOINT", bp_access_watchpoint },
  { "BP_CATCHPOINT", bp_catchpoint },

  { "WP_READ", hw_read },
  { "WP_WRITE", hw_write },
  { "WP_ACCESS", hw_access },

  END_INTEGER_CONSTANTS
};

static const scheme_function breakpoint_functions[] =
{
  { "make-breakpoint", 1, 0, 1, as_a_scm_t_subr (gdbscm_make_breakpoint),
    "\
Create a GDB breakpoint object.\n\
\n\
  Arguments:\n\
    location [#:type <type>] [#:wp-class <wp-class>] [#:internal <bool>] [#:temporary <bool>]\n\
  Returns:\n\
    <gdb:breakpoint> object" },

  { "register-breakpoint!", 1, 0, 0,
    as_a_scm_t_subr (gdbscm_register_breakpoint_x),
    "\
Register a <gdb:breakpoint> object with GDB." },

  { "delete-breakpoint!", 1, 0, 0, as_a_scm_t_subr (gdbscm_delete_breakpoint_x),
    "\
Delete the breakpoint from GDB." },

  { "breakpoints", 0, 0, 0, as_a_scm_t_subr (gdbscm_breakpoints),
    "\
Return a list of all GDB breakpoints.\n\
\n\
  Arguments: none" },

  { "breakpoint?", 1, 0, 0, as_a_scm_t_subr (gdbscm_breakpoint_p),
    "\
Return #t if the object is a <gdb:breakpoint> object." },

  { "breakpoint-valid?", 1, 0, 0, as_a_scm_t_subr (gdbscm_breakpoint_valid_p),
    "\
Return #t if the breakpoint has not been deleted from GDB." },

  { "breakpoint-number", 1, 0, 0, as_a_scm_t_subr (gdbscm_breakpoint_number),
    "\
Return the breakpoint's number." },

  { "breakpoint-temporary?", 1, 0, 0, as_a_scm_t_subr (gdbscm_breakpoint_temporary),
    "\
Return #t if the breakpoint is a temporary breakpoint." },

  { "breakpoint-type", 1, 0, 0, as_a_scm_t_subr (gdbscm_breakpoint_type),
    "\
Return the type of the breakpoint." },

  { "breakpoint-visible?", 1, 0, 0, as_a_scm_t_subr (gdbscm_breakpoint_visible),
    "\
Return #t if the breakpoint is visible to the user." },

  { "breakpoint-location", 1, 0, 0,
    as_a_scm_t_subr (gdbscm_breakpoint_location),
    "\
Return the location of the breakpoint as specified by the user." },

  { "breakpoint-expression", 1, 0, 0,
    as_a_scm_t_subr (gdbscm_breakpoint_expression),
    "\
Return the expression of the breakpoint as specified by the user.\n\
Valid for watchpoints only, returns #f for non-watchpoints." },

  { "breakpoint-enabled?", 1, 0, 0,
    as_a_scm_t_subr (gdbscm_breakpoint_enabled_p),
    "\
Return #t if the breakpoint is enabled." },

  { "set-breakpoint-enabled!", 2, 0, 0,
    as_a_scm_t_subr (gdbscm_set_breakpoint_enabled_x),
    "\
Set the breakpoint's enabled state.\n\
\n\
  Arguments: <gdb:breakpoint> boolean" },

  { "breakpoint-silent?", 1, 0, 0, as_a_scm_t_subr (gdbscm_breakpoint_silent_p),
    "\
Return #t if the breakpoint is silent." },

  { "set-breakpoint-silent!", 2, 0, 0,
    as_a_scm_t_subr (gdbscm_set_breakpoint_silent_x),
    "\
Set the breakpoint's silent state.\n\
\n\
  Arguments: <gdb:breakpoint> boolean" },

  { "breakpoint-ignore-count", 1, 0, 0,
    as_a_scm_t_subr (gdbscm_breakpoint_ignore_count),
    "\
Return the breakpoint's \"ignore\" count." },

  { "set-breakpoint-ignore-count!", 2, 0, 0,
    as_a_scm_t_subr (gdbscm_set_breakpoint_ignore_count_x),
    "\
Set the breakpoint's \"ignore\" count.\n\
\n\
  Arguments: <gdb:breakpoint> count" },

  { "breakpoint-hit-count", 1, 0, 0,
    as_a_scm_t_subr (gdbscm_breakpoint_hit_count),
    "\
Return the breakpoint's \"hit\" count." },

  { "set-breakpoint-hit-count!", 2, 0, 0,
    as_a_scm_t_subr (gdbscm_set_breakpoint_hit_count_x),
    "\
Set the breakpoint's \"hit\" count.  The value must be zero.\n\
\n\
  Arguments: <gdb:breakpoint> 0" },

  { "breakpoint-thread", 1, 0, 0, as_a_scm_t_subr (gdbscm_breakpoint_thread),
    "\
Return the breakpoint's global thread id or #f if there isn't one." },

  { "set-breakpoint-thread!", 2, 0, 0,
    as_a_scm_t_subr (gdbscm_set_breakpoint_thread_x),
    "\
Set the global thread id for this breakpoint.\n\
\n\
  Arguments: <gdb:breakpoint> global-thread-id" },

  { "breakpoint-task", 1, 0, 0, as_a_scm_t_subr (gdbscm_breakpoint_task),
    "\
Return the breakpoint's Ada task-id or #f if there isn't one." },

  { "set-breakpoint-task!", 2, 0, 0,
    as_a_scm_t_subr (gdbscm_set_breakpoint_task_x),
    "\
Set the breakpoint's Ada task-id.\n\
\n\
  Arguments: <gdb:breakpoint> task-id" },

  { "breakpoint-condition", 1, 0, 0,
    as_a_scm_t_subr (gdbscm_breakpoint_condition),
    "\
Return the breakpoint's condition as specified by the user.\n\
Return #f if there isn't one." },

  { "set-breakpoint-condition!", 2, 0, 0,
    as_a_scm_t_subr (gdbscm_set_breakpoint_condition_x),
    "\
Set the breakpoint's condition.\n\
\n\
  Arguments: <gdb:breakpoint> condition\n\
    condition: a string" },

  { "breakpoint-stop", 1, 0, 0, as_a_scm_t_subr (gdbscm_breakpoint_stop),
    "\
Return the breakpoint's stop predicate.\n\
Return #f if there isn't one." },

  { "set-breakpoint-stop!", 2, 0, 0,
    as_a_scm_t_subr (gdbscm_set_breakpoint_stop_x),
    "\
Set the breakpoint's stop predicate.\n\
\n\
  Arguments: <gdb:breakpoint> procedure\n\
    procedure: A procedure of one argument, the breakpoint.\n\
      Its result is true if program execution should stop." },

  { "breakpoint-commands", 1, 0, 0,
    as_a_scm_t_subr (gdbscm_breakpoint_commands),
    "\
Return the breakpoint's commands." },

  END_FUNCTIONS
};

void
gdbscm_initialize_breakpoints (void)
{
  breakpoint_smob_tag
    = gdbscm_make_smob_type (breakpoint_smob_name, sizeof (breakpoint_smob));
  scm_set_smob_free (breakpoint_smob_tag, bpscm_free_breakpoint_smob);
  scm_set_smob_print (breakpoint_smob_tag, bpscm_print_breakpoint_smob);

  gdb::observers::breakpoint_created.attach (bpscm_breakpoint_created,
					     "scm-breakpoint");
  gdb::observers::breakpoint_deleted.attach (bpscm_breakpoint_deleted,
					     "scm-breakpoint");

  gdbscm_define_integer_constants (breakpoint_integer_constants, 1);
  gdbscm_define_functions (breakpoint_functions, 1);

  type_keyword = scm_from_latin1_keyword ("type");
  wp_class_keyword = scm_from_latin1_keyword ("wp-class");
  internal_keyword = scm_from_latin1_keyword ("internal");
  temporary_keyword = scm_from_latin1_keyword ("temporary");
}
