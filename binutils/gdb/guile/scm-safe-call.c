/* GDB/Scheme support for safe calls into the Guile interpreter.

   Copyright (C) 2014-2024 Free Software Foundation, Inc.

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
#include "filenames.h"
#include "guile-internal.h"
#include "gdbsupport/pathstuff.h"

/* Struct to marshall args to scscm_safe_call_body.  */

struct c_data
{
  const char *(*func) (void *);
  void *data;
  /* An error message or NULL for success.  */
  const char *result;
};

/* Struct to marshall args through gdbscm_with_catch.  */

struct with_catch_data
{
  scm_t_catch_body func;
  void *data;
  scm_t_catch_handler unwind_handler;
  scm_t_catch_handler pre_unwind_handler;

  /* If EXCP_MATCHER is non-NULL, it is an excp_matcher_func function.
     If the exception is recognized by it, the exception is recorded as is,
     without wrapping it in gdb:with-stack.  */
  excp_matcher_func *excp_matcher;

  SCM stack;
  SCM catch_result;
};

/* The "body" argument to scm_i_with_continuation_barrier.
   Invoke the user-supplied function.  */

static SCM
scscm_safe_call_body (void *d)
{
  struct c_data *data = (struct c_data *) d;

  data->result = data->func (data->data);

  return SCM_UNSPECIFIED;
}

/* A "pre-unwind handler" to scm_c_catch that prints the exception
   according to "set guile print-stack".  */

static SCM
scscm_printing_pre_unwind_handler (void *data, SCM key, SCM args)
{
  SCM stack = scm_make_stack (SCM_BOOL_T, scm_list_1 (scm_from_int (2)));

  gdbscm_print_exception_with_stack (SCM_BOOL_F, stack, key, args);

  return SCM_UNSPECIFIED;
}

/* A no-op unwind handler.  */

static SCM
scscm_nop_unwind_handler (void *data, SCM key, SCM args)
{
  return SCM_UNSPECIFIED;
}

/* The "pre-unwind handler" to scm_c_catch that records the exception
   for possible later printing.  We do this in the pre-unwind handler because
   we want the stack to include point where the exception occurred.

   If DATA is non-NULL, it is an excp_matcher_func function.
   If the exception is recognized by it, the exception is recorded as is,
   without wrapping it in gdb:with-stack.  */

static SCM
scscm_recording_pre_unwind_handler (void *datap, SCM key, SCM args)
{
  struct with_catch_data *data = (struct with_catch_data *) datap;
  excp_matcher_func *matcher = data->excp_matcher;

  if (matcher != NULL && matcher (key))
    return SCM_UNSPECIFIED;

  /* There's no need to record the whole stack if we're not going to print it.
     However, convention is to still print the stack frame in which the
     exception occurred, even if we're not going to print a full backtrace.
     For now, keep it simple.  */

  data->stack = scm_make_stack (SCM_BOOL_T, scm_list_1 (scm_from_int (2)));

  /* IWBN if we could return the <gdb:exception> here and skip the unwind
     handler, but it doesn't work that way.  If we want to return a
     <gdb:exception> object from the catch it needs to come from the unwind
     handler.  So what we do is save the stack for later use by the unwind
     handler.  */

  return SCM_UNSPECIFIED;
}

/* Part two of the recording unwind handler.
   Here we take the stack saved from the pre-unwind handler and create
   the <gdb:exception> object.  */

static SCM
scscm_recording_unwind_handler (void *datap, SCM key, SCM args)
{
  struct with_catch_data *data = (struct with_catch_data *) datap;

  /* We need to record the stack in the exception since we're about to
     throw and lose the location that got the exception.  We do this by
     wrapping the exception + stack in a new exception.  */

  if (gdbscm_is_true (data->stack))
    return gdbscm_make_exception_with_stack (key, args, data->stack);

  return gdbscm_make_exception (key, args);
}

/* Ugh. :-(
   Guile doesn't export scm_i_with_continuation_barrier which is exactly
   what we need.  To cope, have our own wrapper around scm_c_catch and
   pass this as the "body" argument to scm_c_with_continuation_barrier.
   Darn darn darn.  */

static void *
gdbscm_with_catch (void *data)
{
  struct with_catch_data *d = (struct with_catch_data *) data;

  d->catch_result
    = scm_c_catch (SCM_BOOL_T,
		   d->func, d->data,
		   d->unwind_handler, d,
		   d->pre_unwind_handler, d);

#if HAVE_GUILE_MANUAL_FINALIZATION
  scm_run_finalizers ();
#endif

  return NULL;
}

/* A wrapper around scm_with_guile that prints backtraces and exceptions
   according to "set guile print-stack".
   The result if NULL if no exception occurred, otherwise it is a statically
   allocated error message (caller must *not* free).  */

const char *
gdbscm_with_guile (const char *(*func) (void *), void *data)
{
  struct c_data c_data;
  struct with_catch_data catch_data;

  c_data.func = func;
  c_data.data = data;
  /* Set this now in case an exception is thrown.  */
  c_data.result = _("Error while executing Scheme code.");

  catch_data.func = scscm_safe_call_body;
  catch_data.data = &c_data;
  catch_data.unwind_handler = scscm_nop_unwind_handler;
  catch_data.pre_unwind_handler = scscm_printing_pre_unwind_handler;
  catch_data.excp_matcher = NULL;
  catch_data.stack = SCM_BOOL_F;
  catch_data.catch_result = SCM_UNSPECIFIED;

  scm_with_guile (gdbscm_with_catch, &catch_data);

  return c_data.result;
}

/* Another wrapper of scm_with_guile for use by the safe call/apply routines
   in this file, as well as for general purpose calling other functions safely.
   For these we want to record the exception, but leave the possible printing
   of it to later.  */

SCM
gdbscm_call_guile (SCM (*func) (void *), void *data,
		   excp_matcher_func *ok_excps)
{
  struct with_catch_data catch_data;

  catch_data.func = func;
  catch_data.data = data;
  catch_data.unwind_handler = scscm_recording_unwind_handler;
  catch_data.pre_unwind_handler = scscm_recording_pre_unwind_handler;
  catch_data.excp_matcher = ok_excps;
  catch_data.stack = SCM_BOOL_F;
  catch_data.catch_result = SCM_UNSPECIFIED;

#if 0
  scm_c_with_continuation_barrier (gdbscm_with_catch, &catch_data);
#else
  scm_with_guile (gdbscm_with_catch, &catch_data);
#endif

  return catch_data.catch_result;
}

/* Utilities to safely call Scheme code, catching all exceptions, and
   preventing continuation capture.
   The result is the result of calling the function, or if an exception occurs
   then the result is a <gdb:exception> smob, which can be tested for with
   gdbscm_is_exception.  */

/* Helper for gdbscm_safe_call_0.  */

static SCM
scscm_call_0_body (void *argsp)
{
  SCM *args = (SCM *) argsp;

  return scm_call_0 (args[0]);
}

SCM
gdbscm_safe_call_0 (SCM proc, excp_matcher_func *ok_excps)
{
  SCM args[] = { proc };

  return gdbscm_call_guile (scscm_call_0_body, args, ok_excps);
}

/* Helper for gdbscm_safe_call_1.  */

static SCM
scscm_call_1_body (void *argsp)
{
  SCM *args = (SCM *) argsp;

  return scm_call_1 (args[0], args[1]);
}

SCM
gdbscm_safe_call_1 (SCM proc, SCM arg0, excp_matcher_func *ok_excps)
{
  SCM args[] = { proc, arg0 };

  return gdbscm_call_guile (scscm_call_1_body, args, ok_excps);
}

/* Helper for gdbscm_safe_call_2.  */

static SCM
scscm_call_2_body (void *argsp)
{
  SCM *args = (SCM *) argsp;

  return scm_call_2 (args[0], args[1], args[2]);
}

SCM
gdbscm_safe_call_2 (SCM proc, SCM arg0, SCM arg1, excp_matcher_func *ok_excps)
{
  SCM args[] = { proc, arg0, arg1 };

  return gdbscm_call_guile (scscm_call_2_body, args, ok_excps);
}

/* Helper for gdbscm_safe_call_3.  */

static SCM
scscm_call_3_body (void *argsp)
{
  SCM *args = (SCM *) argsp;

  return scm_call_3 (args[0], args[1], args[2], args[3]);
}

SCM
gdbscm_safe_call_3 (SCM proc, SCM arg1, SCM arg2, SCM arg3,
		    excp_matcher_func *ok_excps)
{
  SCM args[] = { proc, arg1, arg2, arg3 };

  return gdbscm_call_guile (scscm_call_3_body, args, ok_excps);
}

/* Helper for gdbscm_safe_call_4.  */

static SCM
scscm_call_4_body (void *argsp)
{
  SCM *args = (SCM *) argsp;

  return scm_call_4 (args[0], args[1], args[2], args[3], args[4]);
}

SCM
gdbscm_safe_call_4 (SCM proc, SCM arg1, SCM arg2, SCM arg3, SCM arg4,
		    excp_matcher_func *ok_excps)
{
  SCM args[] = { proc, arg1, arg2, arg3, arg4 };

  return gdbscm_call_guile (scscm_call_4_body, args, ok_excps);
}

/* Helper for gdbscm_safe_apply_1.  */

static SCM
scscm_apply_1_body (void *argsp)
{
  SCM *args = (SCM *) argsp;

  return scm_apply_1 (args[0], args[1], args[2]);
}

SCM
gdbscm_safe_apply_1 (SCM proc, SCM arg0, SCM rest, excp_matcher_func *ok_excps)
{
  SCM args[] = { proc, arg0, rest };

  return gdbscm_call_guile (scscm_apply_1_body, args, ok_excps);
}

/* Utilities to call Scheme code, not catching exceptions, and
   not preventing continuation capture.
   The result is the result of calling the function.
   If an exception occurs then Guile is left to handle the exception,
   unwinding the stack as appropriate.

   USE THESE WITH CARE.
   Typically these are called from functions that implement Scheme procedures,
   and we don't want to catch the exception; otherwise it will get printed
   twice: once when first caught and once if it ends up being rethrown and the
   rethrow reaches the top repl, which will confuse the user.

   While these calls just pass the call off to the corresponding Guile
   procedure, all such calls are routed through these ones to:
   a) provide a place to put hooks or whatnot in if we need to,
   b) add "unsafe" to the name to alert the reader.  */

SCM
gdbscm_unsafe_call_1 (SCM proc, SCM arg0)
{
  return scm_call_1 (proc, arg0);
}

/* Utilities for safely evaluating a Scheme expression string.  */

struct eval_scheme_string_data
{
  const char *string;
  int display_result;
};

/* Wrapper to eval a C string in the Guile interpreter.
   This is passed to gdbscm_with_guile.  */

static const char *
scscm_eval_scheme_string (void *datap)
{
  struct eval_scheme_string_data *data
    = (struct eval_scheme_string_data *) datap;
  SCM result = scm_c_eval_string (data->string);

  if (data->display_result && !scm_is_eq (result, SCM_UNSPECIFIED))
    {
      SCM port = scm_current_output_port ();

      scm_write (result, port);
      scm_newline (port);
    }

  /* If we get here the eval succeeded.  */
  return NULL;
}

/* Evaluate EXPR in the Guile interpreter, catching all exceptions
   and preventing continuation capture.
   The result is NULL if no exception occurred.  Otherwise, the exception is
   printed according to "set guile print-stack" and the result is an error
   message.  */

gdb::unique_xmalloc_ptr<char>
gdbscm_safe_eval_string (const char *string, int display_result)
{
  struct eval_scheme_string_data data = { string, display_result };
  const char *result;

  result = gdbscm_with_guile (scscm_eval_scheme_string, (void *) &data);

  if (result != NULL)
    return make_unique_xstrdup (result);
  return NULL;
}

/* Utilities for safely loading Scheme scripts.  */

/* Helper function for gdbscm_safe_source_scheme_script.  */

static const char *
scscm_source_scheme_script (void *data)
{
  const char *filename = (const char *) data;

  /* The Guile docs don't specify what the result is.
     Maybe it's SCM_UNSPECIFIED, but the docs should specify that. :-) */
  scm_c_primitive_load_path (filename);

  /* If we get here the load succeeded.  */
  return NULL;
}

/* Try to load a script, catching all exceptions,
   and preventing continuation capture.
   The result is NULL if the load succeeded.  Otherwise, the exception is
   printed according to "set guile print-stack" and the result is an error
   message allocated with malloc, caller must free.  */

gdb::unique_xmalloc_ptr<char>
gdbscm_safe_source_script (const char *filename)
{
  /* scm_c_primitive_load_path only looks in %load-path for files with
     relative paths.  An alternative could be to temporarily add "." to
     %load-path, but we don't want %load-path to be searched.  At least not
     by default.  This function is invoked by the "source" GDB command which
     already has its own path search support.  */
  gdb::unique_xmalloc_ptr<char> abs_filename;
  const char *result;

  if (!IS_ABSOLUTE_PATH (filename))
    {
      abs_filename = gdb_realpath (filename);
      filename = abs_filename.get ();
    }

  result = gdbscm_with_guile (scscm_source_scheme_script,
			      (void *) filename);

  if (result != NULL)
    return make_unique_xstrdup (result);
  return NULL;
}

/* Utility for entering an interactive Guile repl.  */

void
gdbscm_enter_repl (void)
{
  /* It's unfortunate to have to resort to something like this, but
     scm_shell doesn't return.  :-(  I found this code on guile-user@.  */
  gdbscm_safe_call_1 (scm_c_public_ref ("system repl repl", "start-repl"),
		      scm_from_latin1_symbol ("scheme"), NULL);
}
