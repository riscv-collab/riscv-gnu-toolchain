/* GDB/Scheme exception support.

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

/* Notes:

   IWBN to support SRFI 34/35.  At the moment we follow Guile's own
   exception mechanism.

   The non-static functions in this file have prefix gdbscm_ and
   not exscm_ on purpose.  */

#include "defs.h"
#include <signal.h>
#include "guile-internal.h"

/* The <gdb:exception> smob.
   This is used to record and handle Scheme exceptions.
   One important invariant is that <gdb:exception> smobs are never a valid
   result of a function, other than to signify an exception occurred.  */

struct exception_smob
{
  /* This always appears first.  */
  gdb_smob base;

  /* The key and args parameters to "throw".  */
  SCM key;
  SCM args;
};

static const char exception_smob_name[] = "gdb:exception";

/* The tag Guile knows the exception smob by.  */
static scm_t_bits exception_smob_tag;

/* A generic error in struct gdb_exception.
   I.e., not RETURN_QUIT and not MEMORY_ERROR.  */
static SCM error_symbol;

/* An error occurred accessing inferior memory.
   This is not a Scheme programming error.  */
static SCM memory_error_symbol;

/* User interrupt, e.g., RETURN_QUIT in struct gdb_exception.  */
static SCM signal_symbol;

/* A user error, e.g., bad arg to gdb command.  */
static SCM user_error_symbol;

/* Printing the stack is done by first capturing the stack and recording it in
   a <gdb:exception> object with this key and with the ARGS field set to
   (cons real-key (cons stack real-args)).
   See gdbscm_make_exception_with_stack.  */
static SCM with_stack_error_symbol;

/* The key to use for an invalid object exception.  An invalid object is one
   where the underlying object has been removed from GDB.  */
SCM gdbscm_invalid_object_error_symbol;

/* Values for "guile print-stack" as symbols.  */
static SCM none_symbol;
static SCM message_symbol;
static SCM full_symbol;

static const char percent_print_exception_message_name[] =
  "%print-exception-message";

/* Variable containing %print-exception-message.
   It is not defined until late in initialization, after our init routine
   has run.  Cope by looking it up lazily.  */
static SCM percent_print_exception_message_var = SCM_BOOL_F;

static const char percent_print_exception_with_stack_name[] =
  "%print-exception-with-stack";

/* Variable containing %print-exception-with-stack.
   It is not defined until late in initialization, after our init routine
   has run.  Cope by looking it up lazily.  */
static SCM percent_print_exception_with_stack_var = SCM_BOOL_F;

/* Counter to keep track of the number of times we create a <gdb:exception>
   object, for performance monitoring purposes.  */
static unsigned long gdbscm_exception_count = 0;

/* Administrivia for exception smobs.  */

/* The smob "print" function for <gdb:exception>.  */

static int
exscm_print_exception_smob (SCM self, SCM port, scm_print_state *pstate)
{
  exception_smob *e_smob = (exception_smob *) SCM_SMOB_DATA (self);

  gdbscm_printf (port, "#<%s ", exception_smob_name);
  scm_write (e_smob->key, port);
  scm_puts (" ", port);
  scm_write (e_smob->args, port);
  scm_puts (">", port);

  scm_remember_upto_here_1 (self);

  /* Non-zero means success.  */
  return 1;
}

/* (make-exception key args) -> <gdb:exception> */

SCM
gdbscm_make_exception (SCM key, SCM args)
{
  exception_smob *e_smob = (exception_smob *)
    scm_gc_malloc (sizeof (exception_smob), exception_smob_name);
  SCM smob;

  e_smob->key = key;
  e_smob->args = args;
  smob = scm_new_smob (exception_smob_tag, (scm_t_bits) e_smob);
  gdbscm_init_gsmob (&e_smob->base);

  ++gdbscm_exception_count;

  return smob;
}

/* Return non-zero if SCM is a <gdb:exception> object.  */

int
gdbscm_is_exception (SCM scm)
{
  return SCM_SMOB_PREDICATE (exception_smob_tag, scm);
}

/* (exception? scm) -> boolean */

static SCM
gdbscm_exception_p (SCM scm)
{
  return scm_from_bool (gdbscm_is_exception (scm));
}

/* (exception-key <gdb:exception>) -> key */

SCM
gdbscm_exception_key (SCM self)
{
  exception_smob *e_smob;

  SCM_ASSERT_TYPE (gdbscm_is_exception (self), self, SCM_ARG1, FUNC_NAME,
		   "gdb:exception");

  e_smob = (exception_smob *) SCM_SMOB_DATA (self);
  return e_smob->key;
}

/* (exception-args <gdb:exception>) -> arg-list */

SCM
gdbscm_exception_args (SCM self)
{
  exception_smob *e_smob;

  SCM_ASSERT_TYPE (gdbscm_is_exception (self), self, SCM_ARG1, FUNC_NAME,
		   "gdb:exception");

  e_smob = (exception_smob *) SCM_SMOB_DATA (self);
  return e_smob->args;
}

/* Wrap an exception in a <gdb:exception> object that includes STACK.
   gdbscm_print_exception_with_stack knows how to unwrap it.  */

SCM
gdbscm_make_exception_with_stack (SCM key, SCM args, SCM stack)
{
  return gdbscm_make_exception (with_stack_error_symbol,
				scm_cons (key, scm_cons (stack, args)));
}

/* Version of scm_error_scm that creates a gdb:exception object that can later
   be passed to gdbscm_throw.
   KEY is a symbol denoting the kind of error.
   SUBR is either #f or a string marking the function in which the error
   occurred.
   MESSAGE is either #f or the error message string.  It may contain ~a and ~s
   modifiers, provided by ARGS.
   ARGS is a list of args to MESSAGE.
   DATA is an arbitrary object, its value depends on KEY.  The value to pass
   here is a bit underspecified by Guile.  */

SCM
gdbscm_make_error_scm (SCM key, SCM subr, SCM message, SCM args, SCM data)
{
  return gdbscm_make_exception (key, scm_list_4 (subr, message, args, data));
}

/* Version of scm_error that creates a gdb:exception object that can later
   be passed to gdbscm_throw.
   See gdbscm_make_error_scm for a description of the arguments.  */

SCM
gdbscm_make_error (SCM key, const char *subr, const char *message,
		   SCM args, SCM data)
{
  return gdbscm_make_error_scm
    (key,
     subr == NULL ? SCM_BOOL_F : scm_from_latin1_string (subr),
     message == NULL ? SCM_BOOL_F : scm_from_latin1_string (message),
     args, data);
}

/* Version of SCM_ASSERT_TYPE/scm_wrong_type_arg_msg that creates a
   gdb:exception object that can later be passed to gdbscm_throw.  */

SCM
gdbscm_make_type_error (const char *subr, int arg_pos, SCM bad_value,
			const char *expected_type)
{
  gdb::unique_xmalloc_ptr<char> msg;
  SCM result;

  if (arg_pos > 0)
    {
      if (expected_type != NULL)
	{
	  msg = xstrprintf (_("Wrong type argument in position %d"
			      " (expecting %s): ~S"),
			    arg_pos, expected_type);
	}
      else
	{
	  msg = xstrprintf (_("Wrong type argument in position %d: ~S"),
			    arg_pos);
	}
    }
  else
    {
      if (expected_type != NULL)
	{
	  msg = xstrprintf (_("Wrong type argument (expecting %s): ~S"),
			    expected_type);
	}
      else
	msg = xstrprintf (_("Wrong type argument: ~S"));
    }

  result = gdbscm_make_error (scm_arg_type_key, subr, msg.get (),
			      scm_list_1 (bad_value), scm_list_1 (bad_value));
  return result;
}

/* A variant of gdbscm_make_type_error for non-type argument errors.
   ERROR_PREFIX and ERROR are combined to build the error message.
   Care needs to be taken so that the i18n composed form is still
   reasonable, but no one is going to translate these anyway so we don't
   worry too much.
   ERROR_PREFIX may be NULL, ERROR may not be NULL.  */

static SCM
gdbscm_make_arg_error (SCM key, const char *subr, int arg_pos, SCM bad_value,
		       const char *error_prefix, const char *error)
{
  gdb::unique_xmalloc_ptr<char> msg;
  SCM result;

  if (error_prefix != NULL)
    {
      if (arg_pos > 0)
	{
	  msg = xstrprintf (_("%s %s in position %d: ~S"),
			    error_prefix, error, arg_pos);
	}
      else
	msg = xstrprintf (_("%s %s: ~S"), error_prefix, error);
    }
  else
    {
      if (arg_pos > 0)
	msg = xstrprintf (_("%s in position %d: ~S"), error, arg_pos);
      else
	msg = xstrprintf (_("%s: ~S"), error);
    }

  result = gdbscm_make_error (key, subr, msg.get (), scm_list_1 (bad_value),
			      scm_list_1 (bad_value));
  return result;
}

/* Make an invalid-object error <gdb:exception> object.
   OBJECT is the name of the kind of object that is invalid.  */

SCM
gdbscm_make_invalid_object_error (const char *subr, int arg_pos, SCM bad_value,
				  const char *object)
{
  return gdbscm_make_arg_error (gdbscm_invalid_object_error_symbol,
				subr, arg_pos, bad_value,
				_("Invalid object:"), object);
}

/* Throw an invalid-object error.
   OBJECT is the name of the kind of object that is invalid.  */

void
gdbscm_invalid_object_error (const char *subr, int arg_pos, SCM bad_value,
			     const char *object)
{
  SCM exception
    = gdbscm_make_invalid_object_error (subr, arg_pos, bad_value, object);

  gdbscm_throw (exception);
}

/* Make an out-of-range error <gdb:exception> object.  */

SCM
gdbscm_make_out_of_range_error (const char *subr, int arg_pos, SCM bad_value,
				const char *error)
{
  return gdbscm_make_arg_error (scm_out_of_range_key,
				subr, arg_pos, bad_value,
				_("Out of range:"), error);
}

/* Throw an out-of-range error.
   This is the standard Guile out-of-range exception.  */

void
gdbscm_out_of_range_error (const char *subr, int arg_pos, SCM bad_value,
			   const char *error)
{
  SCM exception
    = gdbscm_make_out_of_range_error (subr, arg_pos, bad_value, error);

  gdbscm_throw (exception);
}

/* Make a misc-error <gdb:exception> object.  */

SCM
gdbscm_make_misc_error (const char *subr, int arg_pos, SCM bad_value,
			const char *error)
{
  return gdbscm_make_arg_error (scm_misc_error_key,
				subr, arg_pos, bad_value, NULL, error);
}

/* Throw a misc-error error.  */

void
gdbscm_misc_error (const char *subr, int arg_pos, SCM bad_value,
		   const char *error)
{
  SCM exception = gdbscm_make_misc_error (subr, arg_pos, bad_value, error);

  gdbscm_throw (exception);
}

/* Return a <gdb:exception> object for gdb:memory-error.  */

SCM
gdbscm_make_memory_error (const char *subr, const char *msg, SCM args)
{
  return gdbscm_make_error (memory_error_symbol, subr, msg, args,
			    SCM_EOL);
}

/* Throw a gdb:memory-error exception.  */

void
gdbscm_memory_error (const char *subr, const char *msg, SCM args)
{
  SCM exception = gdbscm_make_memory_error (subr, msg, args);

  gdbscm_throw (exception);
}

/* Return non-zero if KEY is gdb:memory-error.
   Note: This is an excp_matcher_func function.  */

int
gdbscm_memory_error_p (SCM key)
{
  return scm_is_eq (key, memory_error_symbol);
}

/* Return non-zero if KEY is gdb:user-error.
   Note: This is an excp_matcher_func function.  */

int
gdbscm_user_error_p (SCM key)
{
  return scm_is_eq (key, user_error_symbol);
}

/* Wrapper around scm_throw to throw a gdb:exception.
   This function does not return.
   This function cannot be called from inside TRY_CATCH.  */

void
gdbscm_throw (SCM exception)
{
  scm_throw (gdbscm_exception_key (exception),
	     gdbscm_exception_args (exception));
  gdb_assert_not_reached ("scm_throw returned");
}

/* Convert a GDB exception to a <gdb:exception> object.  */

SCM
gdbscm_scm_from_gdb_exception (const gdbscm_gdb_exception &exception)
{
  SCM key;

  if (exception.reason == RETURN_QUIT)
    {
      /* Handle this specially to be consistent with top-repl.scm.  */
      return gdbscm_make_error (signal_symbol, NULL, _("User interrupt"),
				SCM_EOL, scm_list_1 (scm_from_int (SIGINT)));
    }

  if (exception.error == MEMORY_ERROR)
    key = memory_error_symbol;
  else
    key = error_symbol;

  return gdbscm_make_error (key, NULL, "~A",
			    scm_list_1 (gdbscm_scm_from_c_string
					(exception.message)),
			    SCM_BOOL_F);
}

/* Convert a GDB exception to the appropriate Scheme exception and throw it.
   This function does not return.  */

void
gdbscm_throw_gdb_exception (gdbscm_gdb_exception exception)
{
  SCM scm_exception = gdbscm_scm_from_gdb_exception (exception);
  xfree (exception.message);
  gdbscm_throw (scm_exception);
}

/* Print the error message portion of an exception.
   If PORT is #f, use the standard error port.
   KEY cannot be gdb:with-stack.

   Basically this function is just a wrapper around calling
   %print-exception-message.  */

static void
gdbscm_print_exception_message (SCM port, SCM frame, SCM key, SCM args)
{
  SCM printer, status;

  if (gdbscm_is_false (port))
    port = scm_current_error_port ();

  gdb_assert (!scm_is_eq (key, with_stack_error_symbol));

  /* This does not use scm_print_exception because we tweak the output a bit.
     Compare Guile's print-exception with our %print-exception-message for
     details.  */
  if (gdbscm_is_false (percent_print_exception_message_var))
    {
      percent_print_exception_message_var
	= scm_c_private_variable (gdbscm_init_module_name,
				  percent_print_exception_message_name);
      /* If we can't find %print-exception-message, there's a problem on the
	 Scheme side.  Don't kill GDB, just flag an error and leave it at
	 that.  */
      if (gdbscm_is_false (percent_print_exception_message_var))
	{
	  gdbscm_printf (port, _("Error in Scheme exception printing,"
				 " can't find %s.\n"),
			 percent_print_exception_message_name);
	  return;
	}
    }
  printer = scm_variable_ref (percent_print_exception_message_var);

  status = gdbscm_safe_call_4 (printer, port, frame, key, args, NULL);

  /* If that failed still tell the user something.
     But don't use the exception printing machinery!  */
  if (gdbscm_is_exception (status))
    {
      gdbscm_printf (port, _("Error in Scheme exception printing:\n"));
      scm_display (status, port);
      scm_newline (port);
    }
}

/* Print the description of exception KEY, ARGS to PORT, according to the
   setting of "set guile print-stack".
   If PORT is #f, use the standard error port.
   If STACK is #f, never print the stack, regardless of whether printing it
   is enabled.  If STACK is #t, then print it if it is contained in ARGS
   (i.e., KEY is gdb:with-stack).  Otherwise STACK is the result of calling
   scm_make_stack (which will be ignored in favor of the stack in ARGS if
   KEY is gdb:with-stack).
   KEY, ARGS are the standard arguments to scm_throw, et.al.

   Basically this function is just a wrapper around calling
   %print-exception-with-stack.  */

void
gdbscm_print_exception_with_stack (SCM port, SCM stack, SCM key, SCM args)
{
  SCM printer, status;

  if (gdbscm_is_false (port))
    port = scm_current_error_port ();

  if (gdbscm_is_false (percent_print_exception_with_stack_var))
    {
      percent_print_exception_with_stack_var
	= scm_c_private_variable (gdbscm_init_module_name,
				  percent_print_exception_with_stack_name);
      /* If we can't find %print-exception-with-stack, there's a problem on the
	 Scheme side.  Don't kill GDB, just flag an error and leave it at
	 that.  */
      if (gdbscm_is_false (percent_print_exception_with_stack_var))
	{
	  gdbscm_printf (port, _("Error in Scheme exception printing,"
				 " can't find %s.\n"),
			 percent_print_exception_with_stack_name);
	  return;
	}
    }
  printer = scm_variable_ref (percent_print_exception_with_stack_var);

  status = gdbscm_safe_call_4 (printer, port, stack, key, args, NULL);

  /* If that failed still tell the user something.
     But don't use the exception printing machinery!  */
  if (gdbscm_is_exception (status))
    {
      gdbscm_printf (port, _("Error in Scheme exception printing:\n"));
      scm_display (status, port);
      scm_newline (port);
    }
}

/* Print EXCEPTION, a <gdb:exception> object, to PORT.
   If PORT is #f, use the standard error port.  */

void
gdbscm_print_gdb_exception (SCM port, SCM exception)
{
  gdb_assert (gdbscm_is_exception (exception));

  gdbscm_print_exception_with_stack (port, SCM_BOOL_T,
				     gdbscm_exception_key (exception),
				     gdbscm_exception_args (exception));
}

/* Return a string description of <gdb:exception> EXCEPTION.
   If EXCEPTION is a gdb:with-stack exception, unwrap it, a backtrace
   is never returned as part of the result.  */

gdb::unique_xmalloc_ptr<char>
gdbscm_exception_message_to_string (SCM exception)
{
  SCM port = scm_open_output_string ();
  SCM key, args;

  gdb_assert (gdbscm_is_exception (exception));

  key = gdbscm_exception_key (exception);
  args = gdbscm_exception_args (exception);

  if (scm_is_eq (key, with_stack_error_symbol)
      /* Don't crash on a badly generated gdb:with-stack exception.  */
      && scm_is_pair (args)
      && scm_is_pair (scm_cdr (args)))
    {
      key = scm_car (args);
      args = scm_cddr (args);
    }

  gdbscm_print_exception_message (port, SCM_BOOL_F, key, args);
  gdb::unique_xmalloc_ptr<char> result
    = gdbscm_scm_to_c_string (scm_get_output_string (port));
  scm_close_port (port);
  return result;
}

/* Return the value of the "guile print-stack" option as one of:
   'none, 'message, 'full.  */

static SCM
gdbscm_percent_exception_print_style (void)
{
  if (gdbscm_print_excp == gdbscm_print_excp_none)
    return none_symbol;
  if (gdbscm_print_excp == gdbscm_print_excp_message)
    return message_symbol;
  if (gdbscm_print_excp == gdbscm_print_excp_full)
    return full_symbol;
  gdb_assert_not_reached ("bad value for \"guile print-stack\"");
}

/* Return the current <gdb:exception> counter.
   This is for debugging purposes.  */

static SCM
gdbscm_percent_exception_count (void)
{
  return scm_from_ulong (gdbscm_exception_count);
}

/* Initialize the Scheme exception support.  */

static const scheme_function exception_functions[] =
{
  { "make-exception", 2, 0, 0, as_a_scm_t_subr (gdbscm_make_exception),
    "\
Create a <gdb:exception> object.\n\
\n\
  Arguments: key args\n\
    These are the standard key,args arguments of \"throw\"." },

  { "exception?", 1, 0, 0, as_a_scm_t_subr (gdbscm_exception_p),
    "\
Return #t if the object is a <gdb:exception> object." },

  { "exception-key", 1, 0, 0, as_a_scm_t_subr (gdbscm_exception_key),
    "\
Return the exception's key." },

  { "exception-args", 1, 0, 0, as_a_scm_t_subr (gdbscm_exception_args),
    "\
Return the exception's arg list." },

  END_FUNCTIONS
};

static const scheme_function private_exception_functions[] =
{
  { "%exception-print-style", 0, 0, 0,
    as_a_scm_t_subr (gdbscm_percent_exception_print_style),
    "\
Return the value of the \"guile print-stack\" option." },

  { "%exception-count", 0, 0, 0,
    as_a_scm_t_subr (gdbscm_percent_exception_count),
    "\
Return a count of the number of <gdb:exception> objects created.\n\
This is for debugging purposes." },

  END_FUNCTIONS
};

void
gdbscm_initialize_exceptions (void)
{
  exception_smob_tag = gdbscm_make_smob_type (exception_smob_name,
					      sizeof (exception_smob));
  scm_set_smob_print (exception_smob_tag, exscm_print_exception_smob);

  gdbscm_define_functions (exception_functions, 1);
  gdbscm_define_functions (private_exception_functions, 0);

  error_symbol = scm_from_latin1_symbol ("gdb:error");

  memory_error_symbol = scm_from_latin1_symbol ("gdb:memory-error");

  user_error_symbol = scm_from_latin1_symbol ("gdb:user-error");

  gdbscm_invalid_object_error_symbol
    = scm_from_latin1_symbol ("gdb:invalid-object-error");

  with_stack_error_symbol = scm_from_latin1_symbol ("gdb:with-stack");

  /* The text of this symbol is taken from Guile's top-repl.scm.  */
  signal_symbol = scm_from_latin1_symbol ("signal");

  none_symbol = scm_from_latin1_symbol ("none");
  message_symbol = scm_from_latin1_symbol ("message");
  full_symbol = scm_from_latin1_symbol ("full");
}
