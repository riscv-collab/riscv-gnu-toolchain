/* Perform an inferior function call, for GDB, the GNU debugger.

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
#include "infcall.h"
#include "breakpoint.h"
#include "tracepoint.h"
#include "target.h"
#include "regcache.h"
#include "inferior.h"
#include "infrun.h"
#include "block.h"
#include "gdbcore.h"
#include "language.h"
#include "objfiles.h"
#include "gdbcmd.h"
#include "command.h"
#include "dummy-frame.h"
#include "ada-lang.h"
#include "f-lang.h"
#include "gdbthread.h"
#include "event-top.h"
#include "observable.h"
#include "top.h"
#include "ui.h"
#include "interps.h"
#include "thread-fsm.h"
#include <algorithm>
#include "gdbsupport/scope-exit.h"
#include <list>

/* True if we are debugging inferior calls.  */

static bool debug_infcall = false;

/* Print an "infcall" debug statement.  */

#define infcall_debug_printf(fmt, ...) \
  debug_prefixed_printf_cond (debug_infcall, "infcall", fmt, ##__VA_ARGS__)

/* Print "infcall" enter/exit debug statements.  */

#define INFCALL_SCOPED_DEBUG_ENTER_EXIT \
  scoped_debug_enter_exit (debug_infcall, "infcall")

/* Print "infcall" start/end debug statements.  */

#define INFCALL_SCOPED_DEBUG_START_END(fmt, ...) \
  scoped_debug_start_end (debug_infrun, "infcall", fmt, ##__VA_ARGS__)

/* Implement 'show debug infcall'.  */

static void
show_debug_infcall (struct ui_file *file, int from_tty,
		   struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Inferior call debugging is %s.\n"), value);
}

/* If we can't find a function's name from its address,
   we print this instead.  */
#define RAW_FUNCTION_ADDRESS_FORMAT "at 0x%s"
#define RAW_FUNCTION_ADDRESS_SIZE (sizeof (RAW_FUNCTION_ADDRESS_FORMAT) \
				   + 2 * sizeof (CORE_ADDR))

/* NOTE: cagney/2003-04-16: What's the future of this code?

   GDB needs an asynchronous expression evaluator, that means an
   asynchronous inferior function call implementation, and that in
   turn means restructuring the code so that it is event driven.  */

static bool may_call_functions_p = true;
static void
show_may_call_functions_p (struct ui_file *file, int from_tty,
			   struct cmd_list_element *c,
			   const char *value)
{
  gdb_printf (file,
	      _("Permission to call functions in the program is %s.\n"),
	      value);
}

/* How you should pass arguments to a function depends on whether it
   was defined in K&R style or prototype style.  If you define a
   function using the K&R syntax that takes a `float' argument, then
   callers must pass that argument as a `double'.  If you define the
   function using the prototype syntax, then you must pass the
   argument as a `float', with no promotion.

   Unfortunately, on certain older platforms, the debug info doesn't
   indicate reliably how each function was defined.  A function type's
   TYPE_PROTOTYPED flag may be clear, even if the function was defined
   in prototype style.  When calling a function whose TYPE_PROTOTYPED
   flag is clear, GDB consults this flag to decide what to do.

   For modern targets, it is proper to assume that, if the prototype
   flag is clear, that can be trusted: `float' arguments should be
   promoted to `double'.  For some older targets, if the prototype
   flag is clear, that doesn't tell us anything.  The default is to
   trust the debug information; the user can override this behavior
   with "set coerce-float-to-double 0".  */

static bool coerce_float_to_double_p = true;
static void
show_coerce_float_to_double_p (struct ui_file *file, int from_tty,
			       struct cmd_list_element *c, const char *value)
{
  gdb_printf (file,
	      _("Coercion of floats to doubles "
		"when calling functions is %s.\n"),
	      value);
}

/* This boolean tells what gdb should do if a signal is received while
   in a function called from gdb (call dummy).  If set, gdb unwinds
   the stack and restore the context to what as it was before the
   call.

   The default is to stop in the frame where the signal was received.  */

static bool unwind_on_signal_p = false;
static void
show_unwind_on_signal_p (struct ui_file *file, int from_tty,
			 struct cmd_list_element *c, const char *value)
{
  gdb_printf (file,
	      _("Unwinding of stack if a signal is "
		"received while in a call dummy is %s.\n"),
	      value);
}

/* This boolean tells what gdb should do if a std::terminate call is
   made while in a function called from gdb (call dummy).
   As the confines of a single dummy stack prohibit out-of-frame
   handlers from handling a raised exception, and as out-of-frame
   handlers are common in C++, this can lead to no handler being found
   by the unwinder, and a std::terminate call.  This is a false positive.
   If set, gdb unwinds the stack and restores the context to what it
   was before the call.

   The default is to unwind the frame if a std::terminate call is
   made.  */

static bool unwind_on_terminating_exception_p = true;

static void
show_unwind_on_terminating_exception_p (struct ui_file *file, int from_tty,
					struct cmd_list_element *c,
					const char *value)

{
  gdb_printf (file,
	      _("Unwind stack if a C++ exception is "
		"unhandled while in a call dummy is %s.\n"),
	      value);
}

/* Perform the standard coercions that are specified
   for arguments to be passed to C, Ada or Fortran functions.

   If PARAM_TYPE is non-NULL, it is the expected parameter type.
   IS_PROTOTYPED is non-zero if the function declaration is prototyped.  */

static struct value *
value_arg_coerce (struct gdbarch *gdbarch, struct value *arg,
		  struct type *param_type, int is_prototyped)
{
  const struct builtin_type *builtin = builtin_type (gdbarch);
  struct type *arg_type = check_typedef (arg->type ());
  struct type *type
    = param_type ? check_typedef (param_type) : arg_type;

  /* Perform any Ada- and Fortran-specific coercion first.  */
  if (current_language->la_language == language_ada)
    arg = ada_convert_actual (arg, type);
  else if (current_language->la_language == language_fortran)
    type = fortran_preserve_arg_pointer (arg, type);

  /* Force the value to the target if we will need its address.  At
     this point, we could allocate arguments on the stack instead of
     calling malloc if we knew that their addresses would not be
     saved by the called function.  */
  arg = value_coerce_to_target (arg);

  switch (type->code ())
    {
    case TYPE_CODE_REF:
    case TYPE_CODE_RVALUE_REF:
      {
	struct value *new_value;

	if (TYPE_IS_REFERENCE (arg_type))
	  return value_cast_pointers (type, arg, 0);

	/* Cast the value to the reference's target type, and then
	   convert it back to a reference.  This will issue an error
	   if the value was not previously in memory - in some cases
	   we should clearly be allowing this, but how?  */
	new_value = value_cast (type->target_type (), arg);
	new_value = value_ref (new_value, type->code ());
	return new_value;
      }
    case TYPE_CODE_INT:
    case TYPE_CODE_CHAR:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_ENUM:
      /* If we don't have a prototype, coerce to integer type if necessary.  */
      if (!is_prototyped)
	{
	  if (type->length () < builtin->builtin_int->length ())
	    type = builtin->builtin_int;
	}
      /* Currently all target ABIs require at least the width of an integer
	 type for an argument.  We may have to conditionalize the following
	 type coercion for future targets.  */
      if (type->length () < builtin->builtin_int->length ())
	type = builtin->builtin_int;
      break;
    case TYPE_CODE_FLT:
      if (!is_prototyped && coerce_float_to_double_p)
	{
	  if (type->length () < builtin->builtin_double->length ())
	    type = builtin->builtin_double;
	  else if (type->length () > builtin->builtin_double->length ())
	    type = builtin->builtin_long_double;
	}
      break;
    case TYPE_CODE_FUNC:
      type = lookup_pointer_type (type);
      break;
    case TYPE_CODE_ARRAY:
      /* Arrays are coerced to pointers to their first element, unless
	 they are vectors, in which case we want to leave them alone,
	 because they are passed by value.  */
      if (current_language->c_style_arrays_p ())
	if (!type->is_vector ())
	  type = lookup_pointer_type (type->target_type ());
      break;
    case TYPE_CODE_UNDEF:
    case TYPE_CODE_PTR:
    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
    case TYPE_CODE_VOID:
    case TYPE_CODE_SET:
    case TYPE_CODE_RANGE:
    case TYPE_CODE_STRING:
    case TYPE_CODE_ERROR:
    case TYPE_CODE_MEMBERPTR:
    case TYPE_CODE_METHODPTR:
    case TYPE_CODE_METHOD:
    case TYPE_CODE_COMPLEX:
    default:
      break;
    }

  return value_cast (type, arg);
}

/* See infcall.h.  */

CORE_ADDR
find_function_addr (struct value *function,
		    struct type **retval_type,
		    struct type **function_type)
{
  struct type *ftype = check_typedef (function->type ());
  struct gdbarch *gdbarch = ftype->arch ();
  struct type *value_type = NULL;
  /* Initialize it just to avoid a GCC false warning.  */
  CORE_ADDR funaddr = 0;

  /* If it's a member function, just look at the function
     part of it.  */

  /* Determine address to call.  */
  if (ftype->code () == TYPE_CODE_FUNC
      || ftype->code () == TYPE_CODE_METHOD)
    funaddr = function->address ();
  else if (ftype->code () == TYPE_CODE_PTR)
    {
      funaddr = value_as_address (function);
      ftype = check_typedef (ftype->target_type ());
      if (ftype->code () == TYPE_CODE_FUNC
	  || ftype->code () == TYPE_CODE_METHOD)
	funaddr = gdbarch_convert_from_func_ptr_addr
	  (gdbarch, funaddr, current_inferior ()->top_target());
    }
  if (ftype->code () == TYPE_CODE_FUNC
      || ftype->code () == TYPE_CODE_METHOD)
    {
      if (ftype->is_gnu_ifunc ())
	{
	  CORE_ADDR resolver_addr = funaddr;

	  /* Resolve the ifunc.  Note this may call the resolver
	     function in the inferior.  */
	  funaddr = gnu_ifunc_resolve_addr (gdbarch, resolver_addr);

	  /* Skip querying the function symbol if no RETVAL_TYPE or
	     FUNCTION_TYPE have been asked for.  */
	  if (retval_type != NULL || function_type != NULL)
	    {
	      type *target_ftype = find_function_type (funaddr);
	      /* If we don't have debug info for the target function,
		 see if we can instead extract the target function's
		 type from the type that the resolver returns.  */
	      if (target_ftype == NULL)
		target_ftype = find_gnu_ifunc_target_type (resolver_addr);
	      if (target_ftype != NULL)
		{
		  value_type = check_typedef (target_ftype)->target_type ();
		  ftype = target_ftype;
		}
	    }
	}
      else
	value_type = ftype->target_type ();
    }
  else if (ftype->code () == TYPE_CODE_INT)
    {
      /* Handle the case of functions lacking debugging info.
	 Their values are characters since their addresses are char.  */
      if (ftype->length () == 1)
	funaddr = value_as_address (value_addr (function));
      else
	{
	  /* Handle function descriptors lacking debug info.  */
	  int found_descriptor = 0;

	  funaddr = 0;	/* pacify "gcc -Werror" */
	  if (function->lval () == lval_memory)
	    {
	      CORE_ADDR nfunaddr;

	      funaddr = value_as_address (value_addr (function));
	      nfunaddr = funaddr;
	      funaddr = gdbarch_convert_from_func_ptr_addr
		(gdbarch, funaddr, current_inferior ()->top_target ());
	      if (funaddr != nfunaddr)
		found_descriptor = 1;
	    }
	  if (!found_descriptor)
	    /* Handle integer used as address of a function.  */
	    funaddr = (CORE_ADDR) value_as_long (function);
	}
    }
  else
    error (_("Invalid data type for function to be called."));

  if (retval_type != NULL)
    *retval_type = value_type;
  if (function_type != NULL)
    *function_type = ftype;
  return funaddr + gdbarch_deprecated_function_start_offset (gdbarch);
}

/* For CALL_DUMMY_ON_STACK, push a breakpoint sequence that the called
   function returns to.  */

static CORE_ADDR
push_dummy_code (struct gdbarch *gdbarch,
		 CORE_ADDR sp, CORE_ADDR funaddr,
		 gdb::array_view<value *> args,
		 struct type *value_type,
		 CORE_ADDR *real_pc, CORE_ADDR *bp_addr,
		 struct regcache *regcache)
{
  gdb_assert (gdbarch_push_dummy_code_p (gdbarch));

  return gdbarch_push_dummy_code (gdbarch, sp, funaddr,
				  args.data (), args.size (),
				  value_type, real_pc, bp_addr,
				  regcache);
}

/* See infcall.h.  */

void
error_call_unknown_return_type (const char *func_name)
{
  if (func_name != NULL)
    error (_("'%s' has unknown return type; "
	     "cast the call to its declared return type"),
	   func_name);
  else
    error (_("function has unknown return type; "
	     "cast the call to its declared return type"));
}

/* Fetch the name of the function at FUNADDR.
   This is used in printing an error message for call_function_by_hand.
   BUF is used to print FUNADDR in hex if the function name cannot be
   determined.  It must be large enough to hold formatted result of
   RAW_FUNCTION_ADDRESS_FORMAT.  */

static const char *
get_function_name (CORE_ADDR funaddr, char *buf, int buf_size)
{
  {
    struct symbol *symbol = find_pc_function (funaddr);

    if (symbol)
      return symbol->print_name ();
  }

  {
    /* Try the minimal symbols.  */
    struct bound_minimal_symbol msymbol = lookup_minimal_symbol_by_pc (funaddr);

    if (msymbol.minsym)
      return msymbol.minsym->print_name ();
  }

  {
    std::string tmp = string_printf (_(RAW_FUNCTION_ADDRESS_FORMAT),
				     hex_string (funaddr));

    gdb_assert (tmp.length () + 1 <= buf_size);
    return strcpy (buf, tmp.c_str ());
  }
}

/* All the meta data necessary to extract the call's return value.  */

struct call_return_meta_info
{
  /* The caller frame's architecture.  */
  struct gdbarch *gdbarch;

  /* The called function.  */
  struct value *function;

  /* The return value's type.  */
  struct type *value_type;

  /* Are we returning a value using a structure return or a normal
     value return?  */
  int struct_return_p;

  /* If using a structure return, this is the structure's address.  */
  CORE_ADDR struct_addr;
};

/* Extract the called function's return value.  */

static struct value *
get_call_return_value (struct call_return_meta_info *ri)
{
  struct value *retval = NULL;
  thread_info *thr = inferior_thread ();
  bool stack_temporaries = thread_stack_temporaries_enabled_p (thr);

  if (ri->value_type->code () == TYPE_CODE_VOID)
    retval = value::allocate (ri->value_type);
  else if (ri->struct_return_p)
    {
      if (stack_temporaries)
	{
	  retval = value_from_contents_and_address (ri->value_type, NULL,
						    ri->struct_addr);
	  push_thread_stack_temporary (thr, retval);
	}
      else
	retval = value_at_non_lval (ri->value_type, ri->struct_addr);
    }
  else
    {
      gdbarch_return_value_as_value (ri->gdbarch, ri->function, ri->value_type,
				     get_thread_regcache (inferior_thread ()),
				     &retval, NULL);
      if (stack_temporaries && class_or_union_p (ri->value_type))
	{
	  /* Values of class type returned in registers are copied onto
	     the stack and their lval_type set to lval_memory.  This is
	     required because further evaluation of the expression
	     could potentially invoke methods on the return value
	     requiring GDB to evaluate the "this" pointer.  To evaluate
	     the this pointer, GDB needs the memory address of the
	     value.  */
	  retval->force_lval (ri->struct_addr);
	  push_thread_stack_temporary (thr, retval);
	}
    }

  gdb_assert (retval != NULL);
  return retval;
}

/* Data for the FSM that manages an infcall.  It's main job is to
   record the called function's return value.  */

struct call_thread_fsm : public thread_fsm
{
  /* All the info necessary to be able to extract the return
     value.  */
  struct call_return_meta_info return_meta_info;

  /* The called function's return value.  This is extracted from the
     target before the dummy frame is popped.  */
  struct value *return_value = nullptr;

  /* The top level that started the infcall (and is synchronously
     waiting for it to end).  */
  struct ui *waiting_ui;

  call_thread_fsm (struct ui *waiting_ui, struct interp *cmd_interp,
		   struct gdbarch *gdbarch, struct value *function,
		   struct type *value_type,
		   int struct_return_p, CORE_ADDR struct_addr);

  bool should_stop (struct thread_info *thread) override;

  bool should_notify_stop () override;
};

/* Allocate a new call_thread_fsm object.  */

call_thread_fsm::call_thread_fsm (struct ui *waiting_ui,
				  struct interp *cmd_interp,
				  struct gdbarch *gdbarch,
				  struct value *function,
				  struct type *value_type,
				  int struct_return_p, CORE_ADDR struct_addr)
  : thread_fsm (cmd_interp),
    waiting_ui (waiting_ui)
{
  return_meta_info.gdbarch = gdbarch;
  return_meta_info.function = function;
  return_meta_info.value_type = value_type;
  return_meta_info.struct_return_p = struct_return_p;
  return_meta_info.struct_addr = struct_addr;
}

/* Implementation of should_stop method for infcalls.  */

bool
call_thread_fsm::should_stop (struct thread_info *thread)
{
  INFCALL_SCOPED_DEBUG_ENTER_EXIT;

  if (stop_stack_dummy == STOP_STACK_DUMMY)
    {
      /* Done.  */
      set_finished ();

      /* Stash the return value before the dummy frame is popped and
	 registers are restored to what they were before the
	 call..  */
      return_value = get_call_return_value (&return_meta_info);
    }

  /* We are always going to stop this thread, but we might not be planning
     to call call normal_stop, which is only done if should_notify_stop
     returns true.

     As normal_stop is responsible for calling async_enable_stdin, which
     would break us out of wait_sync_command_done, then, if we don't plan
     to call normal_stop, we should call async_enable_stdin here instead.

     Unlike normal_stop, we only call async_enable_stdin on WAITING_UI, but
     that is sufficient for wait_sync_command_done.  */
  if (!this->should_notify_stop ())
    {
      scoped_restore save_ui = make_scoped_restore (&current_ui, waiting_ui);
      gdb_assert (current_ui->prompt_state == PROMPT_BLOCKED);
      async_enable_stdin ();
    }

  return true;
}

/* Implementation of should_notify_stop method for infcalls.  */

bool
call_thread_fsm::should_notify_stop ()
{
  INFCALL_SCOPED_DEBUG_ENTER_EXIT;

  if (finished_p ())
    {
      /* Infcall succeeded.  Be silent and proceed with evaluating the
	 expression.  */
      infcall_debug_printf ("inferior call has finished, don't notify");
      return false;
    }

  infcall_debug_printf ("inferior call didn't complete fully");

  if (stopped_by_random_signal && unwind_on_signal_p)
    {
      infcall_debug_printf ("unwind-on-signal is on, don't notify");
      return false;
    }

  if (stop_stack_dummy == STOP_STD_TERMINATE
      && unwind_on_terminating_exception_p)
    {
      infcall_debug_printf ("unwind-on-terminating-exception is on, don't notify");
      return false;
    }

  /* Something wrong happened.  E.g., an unexpected breakpoint
     triggered, or a signal was intercepted.  Notify the stop.  */
  return true;
}

/* Subroutine of call_function_by_hand to simplify it.
   Start up the inferior and wait for it to stop.
   Return the exception if there's an error, or an exception with
   reason >= 0 if there's no error.

   This is done inside a TRY_CATCH so the caller needn't worry about
   thrown errors.  The caller should rethrow if there's an error.  */

static struct gdb_exception
run_inferior_call (std::unique_ptr<call_thread_fsm> sm,
		   struct thread_info *call_thread, CORE_ADDR real_pc)
{
  INFCALL_SCOPED_DEBUG_ENTER_EXIT;

  struct gdb_exception caught_error;
  ptid_t call_thread_ptid = call_thread->ptid;
  int was_running = call_thread->state == THREAD_RUNNING;

  infcall_debug_printf ("call function at %s in thread %s, was_running = %d",
			core_addr_to_string (real_pc),
			call_thread_ptid.to_string ().c_str (),
			was_running);

  current_ui->unregister_file_handler ();

  scoped_restore restore_in_infcall
    = make_scoped_restore (&call_thread->control.in_infcall, 1);

  clear_proceed_status (0);

  /* Associate the FSM with the thread after clear_proceed_status
     (otherwise it'd clear this FSM).  */
  call_thread->set_thread_fsm (std::move (sm));

  disable_watchpoints_before_interactive_call_start ();

  /* We want to print return value, please...  */
  call_thread->control.proceed_to_finish = 1;

  try
    {
      /* Infcalls run synchronously, in the foreground.  */
      scoped_restore restore_prompt_state
	= make_scoped_restore (&current_ui->prompt_state, PROMPT_BLOCKED);

      /* So that we don't print the prompt prematurely in
	 fetch_inferior_event.  */
      scoped_restore restore_ui_async
	= make_scoped_restore (&current_ui->async, 0);

      proceed (real_pc, GDB_SIGNAL_0);

      infrun_debug_show_threads ("non-exited threads after proceed for inferior-call",
				 all_non_exited_threads ());

      /* Inferior function calls are always synchronous, even if the
	 target supports asynchronous execution.  */
      wait_sync_command_done ();

      infcall_debug_printf ("inferior call completed successfully");
    }
  catch (gdb_exception &e)
    {
      infcall_debug_printf ("exception while making inferior call (%d): %s",
			    e.reason, e.what ());
      caught_error = std::move (e);
    }

  infcall_debug_printf ("thread is now: %s",
			inferior_ptid.to_string ().c_str ());

  /* After the inferior call finished, async_enable_stdin has been
     called, either from normal_stop or from
     call_thread_fsm::should_stop, and the prompt state has been
     restored by the scoped_restore in the try block above.

     If the inferior call finished successfully, then we should
     disable stdin as we don't know yet whether the inferior will be
     stopping.  Calling async_disable_stdin restores things to how
     they were when this function was called.

     If the inferior call didn't complete successfully, then
     normal_stop has already been called, and we know for sure that we
     are going to present this stop to the user.  In this case, we
     call async_enable_stdin.  This changes the prompt state to
     PROMPT_NEEDED.

     If the previous prompt state was PROMPT_NEEDED, then as
     async_enable_stdin has already been called, nothing additional
     needs to be done here.  */
  if (current_ui->prompt_state == PROMPT_BLOCKED)
    {
      if (call_thread->thread_fsm ()->finished_p ())
	async_disable_stdin ();
      else
	async_enable_stdin ();
    }

  /* If the infcall does NOT succeed, normal_stop will have already
     finished the thread states.  However, on success, normal_stop
     defers here, so that we can set back the thread states to what
     they were before the call.  Note that we must also finish the
     state of new threads that might have spawned while the call was
     running.  The main cases to handle are:

     - "(gdb) print foo ()", or any other command that evaluates an
     expression at the prompt.  (The thread was marked stopped before.)

     - "(gdb) break foo if return_false()" or similar cases where we
     do an infcall while handling an event (while the thread is still
     marked running).  In this example, whether the condition
     evaluates true and thus we'll present a user-visible stop is
     decided elsewhere.  */
  if (!was_running
      && call_thread_ptid == inferior_ptid
      && stop_stack_dummy == STOP_STACK_DUMMY)
    finish_thread_state (call_thread->inf->process_target (),
			 user_visible_resume_ptid (0));

  enable_watchpoints_after_interactive_call_stop ();

  /* Call breakpoint_auto_delete on the current contents of the bpstat
     of inferior call thread.
     If all error()s out of proceed ended up calling normal_stop
     (and perhaps they should; it already does in the special case
     of error out of resume()), then we wouldn't need this.  */
  if (caught_error.reason < 0)
    {
      if (call_thread->state != THREAD_EXITED)
	breakpoint_auto_delete (call_thread->control.stop_bpstat);
    }

  return caught_error;
}

/* Reserve space on the stack for a value of the given type.
   Return the address of the allocated space.
   Make certain that the value is correctly aligned.
   The SP argument is modified.  */

static CORE_ADDR
reserve_stack_space (const type *values_type, CORE_ADDR &sp)
{
  frame_info_ptr frame = get_current_frame ();
  struct gdbarch *gdbarch = get_frame_arch (frame);
  CORE_ADDR addr = 0;

  if (gdbarch_inner_than (gdbarch, 1, 2))
    {
      /* Stack grows downward.  Align STRUCT_ADDR and SP after
	 making space.  */
      sp -= values_type->length ();
      if (gdbarch_frame_align_p (gdbarch))
	sp = gdbarch_frame_align (gdbarch, sp);
      addr = sp;
    }
  else
    {
      /* Stack grows upward.  Align the frame, allocate space, and
	 then again, re-align the frame???  */
      if (gdbarch_frame_align_p (gdbarch))
	sp = gdbarch_frame_align (gdbarch, sp);
      addr = sp;
      sp += values_type->length ();
      if (gdbarch_frame_align_p (gdbarch))
	sp = gdbarch_frame_align (gdbarch, sp);
    }

  return addr;
}

/* The data structure which keeps a destructor function and
   its implicit 'this' parameter.  */

struct destructor_info
{
  destructor_info (struct value *function, struct value *self)
    : function (function), self (self) { }

  struct value *function;
  struct value *self;
};


/* Auxiliary function that takes a list of destructor functions
   with their 'this' parameters, and invokes the functions.  */

static void
call_destructors (const std::list<destructor_info> &dtors_to_invoke,
		  struct type *default_return_type)
{
  for (auto vals : dtors_to_invoke)
    {
      call_function_by_hand (vals.function, default_return_type,
			     gdb::make_array_view (&(vals.self), 1));
    }
}

/* See infcall.h.  */

struct value *
call_function_by_hand (struct value *function,
		       type *default_return_type,
		       gdb::array_view<value *> args)
{
  return call_function_by_hand_dummy (function, default_return_type,
				      args, NULL, NULL);
}

/* All this stuff with a dummy frame may seem unnecessarily complicated
   (why not just save registers in GDB?).  The purpose of pushing a dummy
   frame which looks just like a real frame is so that if you call a
   function and then hit a breakpoint (get a signal, etc), "backtrace"
   will look right.  Whether the backtrace needs to actually show the
   stack at the time the inferior function was called is debatable, but
   it certainly needs to not display garbage.  So if you are contemplating
   making dummy frames be different from normal frames, consider that.  */

/* Perform a function call in the inferior.
   ARGS is a vector of values of arguments.
   FUNCTION is a value, the function to be called.
   Returns a value representing what the function returned.
   May fail to return, if a breakpoint or signal is hit
   during the execution of the function.

   ARGS is modified to contain coerced values.  */

struct value *
call_function_by_hand_dummy (struct value *function,
			     type *default_return_type,
			     gdb::array_view<value *> args,
			     dummy_frame_dtor_ftype *dummy_dtor,
			     void *dummy_dtor_data)
{
  INFCALL_SCOPED_DEBUG_ENTER_EXIT;

  CORE_ADDR sp;
  struct type *target_values_type;
  function_call_return_method return_method = return_method_normal;
  CORE_ADDR struct_addr = 0;
  CORE_ADDR real_pc;
  CORE_ADDR bp_addr;
  struct frame_id dummy_id;
  frame_info_ptr frame;
  struct gdbarch *gdbarch;
  ptid_t call_thread_ptid;
  struct gdb_exception e;
  char name_buf[RAW_FUNCTION_ADDRESS_SIZE];

  if (!may_call_functions_p)
    error (_("Cannot call functions in the program: "
	     "may-call-functions is off."));

  if (!target_has_execution ())
    noprocess ();

  if (get_traceframe_number () >= 0)
    error (_("May not call functions while looking at trace frames."));

  if (execution_direction == EXEC_REVERSE)
    error (_("Cannot call functions in reverse mode."));

  /* We're going to run the target, and inspect the thread's state
     afterwards.  Hold a strong reference so that the pointer remains
     valid even if the thread exits.  */
  thread_info_ref call_thread
    = thread_info_ref::new_reference (inferior_thread ());

  bool stack_temporaries = thread_stack_temporaries_enabled_p (call_thread.get ());

  frame = get_current_frame ();
  gdbarch = get_frame_arch (frame);

  if (!gdbarch_push_dummy_call_p (gdbarch))
    error (_("This target does not support function calls."));

  /* Find the function type and do a sanity check.  */
  type *ftype;
  type *values_type;
  CORE_ADDR funaddr = find_function_addr (function, &values_type, &ftype);

  if (is_nocall_function (ftype))
    error (_("Cannot call the function '%s' which does not follow the "
	     "target calling convention."),
	   get_function_name (funaddr, name_buf, sizeof (name_buf)));

  if (values_type == NULL || values_type->is_stub ())
    values_type = default_return_type;
  if (values_type == NULL)
    {
      const char *name = get_function_name (funaddr,
					    name_buf, sizeof (name_buf));
      error (_("'%s' has unknown return type; "
	       "cast the call to its declared return type"),
	     name);
    }

  values_type = check_typedef (values_type);

  if (args.size () < ftype->num_fields ())
    error (_("Too few arguments in function call."));

  infcall_debug_printf ("calling %s", get_function_name (funaddr, name_buf,
							 sizeof (name_buf)));

  /* A holder for the inferior status.
     This is only needed while we're preparing the inferior function call.  */
  infcall_control_state_up inf_status (save_infcall_control_state ());

  /* Save the caller's registers and other state associated with the
     inferior itself so that they can be restored once the
     callee returns.  To allow nested calls the registers are (further
     down) pushed onto a dummy frame stack.  This unique pointer
     is released once the regcache has been pushed).  */
  infcall_suspend_state_up caller_state (save_infcall_suspend_state ());

  /* Ensure that the initial SP is correctly aligned.  */
  {
    CORE_ADDR old_sp = get_frame_sp (frame);

    if (gdbarch_frame_align_p (gdbarch))
      {
	sp = gdbarch_frame_align (gdbarch, old_sp);
	/* NOTE: cagney/2003-08-13: Skip the "red zone".  For some
	   ABIs, a function can use memory beyond the inner most stack
	   address.  AMD64 called that region the "red zone".  Skip at
	   least the "red zone" size before allocating any space on
	   the stack.  */
	if (gdbarch_inner_than (gdbarch, 1, 2))
	  sp -= gdbarch_frame_red_zone_size (gdbarch);
	else
	  sp += gdbarch_frame_red_zone_size (gdbarch);
	/* Still aligned?  */
	gdb_assert (sp == gdbarch_frame_align (gdbarch, sp));
	/* NOTE: cagney/2002-09-18:
	   
	   On a RISC architecture, a void parameterless generic dummy
	   frame (i.e., no parameters, no result) typically does not
	   need to push anything the stack and hence can leave SP and
	   FP.  Similarly, a frameless (possibly leaf) function does
	   not push anything on the stack and, hence, that too can
	   leave FP and SP unchanged.  As a consequence, a sequence of
	   void parameterless generic dummy frame calls to frameless
	   functions will create a sequence of effectively identical
	   frames (SP, FP and TOS and PC the same).  This, not
	   surprisingly, results in what appears to be a stack in an
	   infinite loop --- when GDB tries to find a generic dummy
	   frame on the internal dummy frame stack, it will always
	   find the first one.

	   To avoid this problem, the code below always grows the
	   stack.  That way, two dummy frames can never be identical.
	   It does burn a few bytes of stack but that is a small price
	   to pay :-).  */
	if (sp == old_sp)
	  {
	    if (gdbarch_inner_than (gdbarch, 1, 2))
	      /* Stack grows down.  */
	      sp = gdbarch_frame_align (gdbarch, old_sp - 1);
	    else
	      /* Stack grows up.  */
	      sp = gdbarch_frame_align (gdbarch, old_sp + 1);
	  }
	/* SP may have underflown address zero here from OLD_SP.  Memory access
	   functions will probably fail in such case but that is a target's
	   problem.  */
      }
    else
      /* FIXME: cagney/2002-09-18: Hey, you loose!

	 Who knows how badly aligned the SP is!

	 If the generic dummy frame ends up empty (because nothing is
	 pushed) GDB won't be able to correctly perform back traces.
	 If a target is having trouble with backtraces, first thing to
	 do is add FRAME_ALIGN() to the architecture vector.  If that
	 fails, try dummy_id().

	 If the ABI specifies a "Red Zone" (see the doco) the code
	 below will quietly trash it.  */
      sp = old_sp;

    /* Skip over the stack temporaries that might have been generated during
       the evaluation of an expression.  */
    if (stack_temporaries)
      {
	struct value *lastval;

	lastval = get_last_thread_stack_temporary (call_thread.get ());
	if (lastval != NULL)
	  {
	    CORE_ADDR lastval_addr = lastval->address ();

	    if (gdbarch_inner_than (gdbarch, 1, 2))
	      {
		gdb_assert (sp >= lastval_addr);
		sp = lastval_addr;
	      }
	    else
	      {
		gdb_assert (sp <= lastval_addr);
		sp = lastval_addr + lastval->type ()->length ();
	      }

	    if (gdbarch_frame_align_p (gdbarch))
	      sp = gdbarch_frame_align (gdbarch, sp);
	  }
      }
  }

  /* Are we returning a value using a structure return?  */

  if (gdbarch_return_in_first_hidden_param_p (gdbarch, values_type))
    {
      return_method = return_method_hidden_param;

      /* Tell the target specific argument pushing routine not to
	 expect a value.  */
      target_values_type = builtin_type (gdbarch)->builtin_void;
    }
  else
    {
      if (using_struct_return (gdbarch, function, values_type))
	return_method = return_method_struct;
      target_values_type = values_type;
    }

  gdb::observers::inferior_call_pre.notify (inferior_ptid, funaddr);

  /* Determine the location of the breakpoint (and possibly other
     stuff) that the called function will return to.  The SPARC, for a
     function returning a structure or union, needs to make space for
     not just the breakpoint but also an extra word containing the
     size (?) of the structure being passed.  */

  switch (gdbarch_call_dummy_location (gdbarch))
    {
    case ON_STACK:
      {
	const gdb_byte *bp_bytes;
	CORE_ADDR bp_addr_as_address;
	int bp_size;

	/* Be careful BP_ADDR is in inferior PC encoding while
	   BP_ADDR_AS_ADDRESS is a plain memory address.  */

	sp = push_dummy_code (gdbarch, sp, funaddr, args,
			      target_values_type, &real_pc, &bp_addr,
			      get_thread_regcache (inferior_thread ()));

	/* Write a legitimate instruction at the point where the infcall
	   breakpoint is going to be inserted.  While this instruction
	   is never going to be executed, a user investigating the
	   memory from GDB would see this instruction instead of random
	   uninitialized bytes.  We chose the breakpoint instruction
	   as it may look as the most logical one to the user and also
	   valgrind 3.7.0 needs it for proper vgdb inferior calls.

	   If software breakpoints are unsupported for this target we
	   leave the user visible memory content uninitialized.  */

	bp_addr_as_address = bp_addr;
	bp_bytes = gdbarch_breakpoint_from_pc (gdbarch, &bp_addr_as_address,
					       &bp_size);
	if (bp_bytes != NULL)
	  write_memory (bp_addr_as_address, bp_bytes, bp_size);
      }
      break;
    case AT_ENTRY_POINT:
      {
	CORE_ADDR dummy_addr;

	real_pc = funaddr;
	dummy_addr = entry_point_address ();

	/* A call dummy always consists of just a single breakpoint, so
	   its address is the same as the address of the dummy.

	   The actual breakpoint is inserted separatly so there is no need to
	   write that out.  */
	bp_addr = dummy_addr;
	break;
      }
    default:
      internal_error (_("bad switch"));
    }

  /* Coerce the arguments and handle pass-by-reference.
     We want to remember the destruction required for pass-by-ref values.
     For these, store the dtor function and the 'this' argument
     in DTORS_TO_INVOKE.  */
  std::list<destructor_info> dtors_to_invoke;

  for (int i = args.size () - 1; i >= 0; i--)
    {
      int prototyped;
      struct type *param_type;

      /* FIXME drow/2002-05-31: Should just always mark methods as
	 prototyped.  Can we respect TYPE_VARARGS?  Probably not.  */
      if (ftype->code () == TYPE_CODE_METHOD)
	prototyped = 1;
      else if (ftype->target_type () == NULL && ftype->num_fields () == 0
	       && default_return_type != NULL)
	{
	  /* Calling a no-debug function with the return type
	     explicitly cast.  Assume the function is prototyped,
	     with a prototype matching the types of the arguments.
	     E.g., with:
	     float mult (float v1, float v2) { return v1 * v2; }
	     This:
	     (gdb) p (float) mult (2.0f, 3.0f)
	     Is a simpler alternative to:
	     (gdb) p ((float (*) (float, float)) mult) (2.0f, 3.0f)
	  */
	  prototyped = 1;
	}
      else if (i < ftype->num_fields ())
	prototyped = ftype->is_prototyped ();
      else
	prototyped = 0;

      if (i < ftype->num_fields ())
	param_type = ftype->field (i).type ();
      else
	param_type = NULL;

      value *original_arg = args[i];
      args[i] = value_arg_coerce (gdbarch, args[i],
				  param_type, prototyped);

      if (param_type == NULL)
	continue;

      auto info = language_pass_by_reference (param_type);
      if (!info.copy_constructible)
	error (_("expression cannot be evaluated because the type '%s' "
		 "is not copy constructible"), param_type->name ());

      if (!info.destructible)
	error (_("expression cannot be evaluated because the type '%s' "
		 "is not destructible"), param_type->name ());

      if (info.trivially_copyable)
	continue;

      /* Make a copy of the argument on the stack.  If the argument is
	 trivially copy ctor'able, copy bit by bit.  Otherwise, call
	 the copy ctor to initialize the clone.  */
      CORE_ADDR addr = reserve_stack_space (param_type, sp);
      value *clone
	= value_from_contents_and_address (param_type, nullptr, addr);
      push_thread_stack_temporary (call_thread.get (), clone);
      value *clone_ptr
	= value_from_pointer (lookup_pointer_type (param_type), addr);

      if (info.trivially_copy_constructible)
	{
	  int length = param_type->length ();
	  write_memory (addr, args[i]->contents ().data (), length);
	}
      else
	{
	  value *copy_ctor;
	  value *cctor_args[2] = { clone_ptr, original_arg };
	  find_overload_match (gdb::make_array_view (cctor_args, 2),
			       param_type->name (), METHOD,
			       &clone_ptr, nullptr, &copy_ctor, nullptr,
			       nullptr, 0, EVAL_NORMAL);

	  if (copy_ctor == nullptr)
	    error (_("expression cannot be evaluated because a copy "
		     "constructor for the type '%s' could not be found "
		     "(maybe inlined?)"), param_type->name ());

	  call_function_by_hand (copy_ctor, default_return_type,
				 gdb::make_array_view (cctor_args, 2));
	}

      /* If the argument has a destructor, remember it so that we
	 invoke it after the infcall is complete.  */
      if (!info.trivially_destructible)
	{
	  /* Looking up the function via overload resolution does not
	     work because the compiler (in particular, gcc) adds an
	     artificial int parameter in some cases.  So we look up
	     the function by using the "~" name.  This should be OK
	     because there can be only one dtor definition.  */
	  const char *dtor_name = nullptr;
	  for (int fieldnum = 0;
	       fieldnum < TYPE_NFN_FIELDS (param_type);
	       fieldnum++)
	    {
	      fn_field *fn
		= TYPE_FN_FIELDLIST1 (param_type, fieldnum);
	      const char *field_name
		= TYPE_FN_FIELDLIST_NAME (param_type, fieldnum);

	      if (field_name[0] == '~')
		dtor_name = TYPE_FN_FIELD_PHYSNAME (fn, 0);
	    }

	  if (dtor_name == nullptr)
	    error (_("expression cannot be evaluated because a destructor "
		     "for the type '%s' could not be found "
		     "(maybe inlined?)"), param_type->name ());

	  value *dtor
	    = find_function_in_inferior (dtor_name, 0);

	  /* Insert the dtor to the front of the list to call them
	     in reverse order later.  */
	  dtors_to_invoke.emplace_front (dtor, clone_ptr);
	}

      args[i] = clone_ptr;
    }

  /* Reserve space for the return structure to be written on the
     stack, if necessary.

     While evaluating expressions, we reserve space on the stack for
     return values of class type even if the language ABI and the target
     ABI do not require that the return value be passed as a hidden first
     argument.  This is because we want to store the return value as an
     on-stack temporary while the expression is being evaluated.  This
     enables us to have chained function calls in expressions.

     Keeping the return values as on-stack temporaries while the expression
     is being evaluated is OK because the thread is stopped until the
     expression is completely evaluated.  */

  if (return_method != return_method_normal
      || (stack_temporaries && class_or_union_p (values_type)))
    struct_addr = reserve_stack_space (values_type, sp);

  std::vector<struct value *> new_args;
  if (return_method == return_method_hidden_param)
    {
      /* Add the new argument to the front of the argument list.  */
      new_args.reserve (1 + args.size ());
      new_args.push_back
	(value_from_pointer (lookup_pointer_type (values_type), struct_addr));
      new_args.insert (new_args.end (), args.begin (), args.end ());
      args = new_args;
    }

  /* Create the dummy stack frame.  Pass in the call dummy address as,
     presumably, the ABI code knows where, in the call dummy, the
     return address should be pointed.  */
  sp = gdbarch_push_dummy_call (gdbarch, function,
				get_thread_regcache (inferior_thread ()),
				bp_addr, args.size (), args.data (),
				sp, return_method, struct_addr);

  /* Set up a frame ID for the dummy frame so we can pass it to
     set_momentary_breakpoint.  We need to give the breakpoint a frame
     ID so that the breakpoint code can correctly re-identify the
     dummy breakpoint.  */
  /* Sanity.  The exact same SP value is returned by PUSH_DUMMY_CALL,
     saved as the dummy-frame TOS, and used by dummy_id to form
     the frame ID's stack address.  */
  dummy_id = frame_id_build (sp, bp_addr);

  /* Create a momentary breakpoint at the return address of the
     inferior.  That way it breaks when it returns.  */

  {
    symtab_and_line sal;
    sal.pspace = current_program_space;
    sal.pc = bp_addr;
    sal.section = find_pc_overlay (sal.pc);

    /* Sanity.  The exact same SP value is returned by
       PUSH_DUMMY_CALL, saved as the dummy-frame TOS, and used by
       dummy_id to form the frame ID's stack address.  */
    breakpoint *bpt
      = set_momentary_breakpoint (gdbarch, sal,
				  dummy_id, bp_call_dummy).release ();

    /* set_momentary_breakpoint invalidates FRAME.  */
    frame = NULL;

    bpt->disposition = disp_del;
    gdb_assert (bpt->related_breakpoint == bpt);

    breakpoint *longjmp_b = set_longjmp_breakpoint_for_call_dummy ();
    if (longjmp_b)
      {
	/* Link BPT into the chain of LONGJMP_B.  */
	bpt->related_breakpoint = longjmp_b;
	while (longjmp_b->related_breakpoint != bpt->related_breakpoint)
	  longjmp_b = longjmp_b->related_breakpoint;
	longjmp_b->related_breakpoint = bpt;
      }
  }

  /* Create a breakpoint in std::terminate.
     If a C++ exception is raised in the dummy-frame, and the
     exception handler is (normally, and expected to be) out-of-frame,
     the default C++ handler will (wrongly) be called in an inferior
     function call.  This is wrong, as an exception can be  normally
     and legally handled out-of-frame.  The confines of the dummy frame
     prevent the unwinder from finding the correct handler (or any
     handler, unless it is in-frame).  The default handler calls
     std::terminate.  This will kill the inferior.  Assert that
     terminate should never be called in an inferior function
     call.  Place a momentary breakpoint in the std::terminate function
     and if triggered in the call, rewind.  */
  if (unwind_on_terminating_exception_p)
    set_std_terminate_breakpoint ();

  /* Everything's ready, push all the info needed to restore the
     caller (and identify the dummy-frame) onto the dummy-frame
     stack.  */
  dummy_frame_push (caller_state.release (), &dummy_id, call_thread.get ());
  if (dummy_dtor != NULL)
    register_dummy_frame_dtor (dummy_id, call_thread.get (),
			       dummy_dtor, dummy_dtor_data);

  /* Register a clean-up for unwind_on_terminating_exception_breakpoint.  */
  SCOPE_EXIT { delete_std_terminate_breakpoint (); };

  /* The stopped_by_random_signal variable is global.  If we are here
     as part of a breakpoint condition check then the global will have
     already been setup as part of the original breakpoint stop.  By
     making the inferior call the global will be changed when GDB
     handles the stop after the inferior call.  Avoid confusion by
     restoring the current value after the inferior call.  */
  scoped_restore restore_stopped_by_random_signal
    = make_scoped_restore (&stopped_by_random_signal, 0);

  /* - SNIP - SNIP - SNIP - SNIP - SNIP - SNIP - SNIP - SNIP - SNIP -
     If you're looking to implement asynchronous dummy-frames, then
     just below is the place to chop this function in two..  */

  {
    /* Save the current FSM.  We'll override it.  */
    std::unique_ptr<thread_fsm> saved_sm = call_thread->release_thread_fsm ();
    struct call_thread_fsm *sm;

    /* Save this thread's ptid, we need it later but the thread
       may have exited.  */
    call_thread_ptid = call_thread->ptid;

    /* Run the inferior until it stops.  */

    /* Create the FSM used to manage the infcall.  It tells infrun to
       not report the stop to the user, and captures the return value
       before the dummy frame is popped.  run_inferior_call registers
       it with the thread ASAP.  */
    sm = new call_thread_fsm (current_ui, command_interp (),
			      gdbarch, function,
			      values_type,
			      return_method != return_method_normal,
			      struct_addr);
    {
      std::unique_ptr<call_thread_fsm> sm_up (sm);
      e = run_inferior_call (std::move (sm_up), call_thread.get (), real_pc);
    }

    if (e.reason < 0)
      infcall_debug_printf ("after inferior call, exception (%d): %s",
			    e.reason, e.what ());
    infcall_debug_printf ("after inferior call, thread state is: %s",
			  thread_state_string (call_thread->state));

    gdb::observers::inferior_call_post.notify (call_thread_ptid, funaddr);


    /* As the inferior call failed, we are about to throw an error, which
       will be caught and printed somewhere else in GDB.  We want new threads
       to be printed before the error message, otherwise it looks odd; the
       threads appear after GDB has reported a stop.  */
    update_thread_list ();

    if (call_thread->state != THREAD_EXITED)
      {
	/* The FSM should still be the same.  */
	gdb_assert (call_thread->thread_fsm () == sm);

	if (call_thread->thread_fsm ()->finished_p ())
	  {
	    struct value *retval;

	    infcall_debug_printf ("call completed");

	    /* The inferior call is successful.  Pop the dummy frame,
	       which runs its destructors and restores the inferior's
	       suspend state, and restore the inferior control
	       state.  */
	    dummy_frame_pop (dummy_id, call_thread.get ());
	    restore_infcall_control_state (inf_status.release ());

	    /* Get the return value.  */
	    retval = sm->return_value;

	    /* Restore the original FSM and clean up / destroy the call FSM.
	       Doing it in this order ensures that if the call to clean_up
	       throws, the original FSM is properly restored.  */
	    {
	      std::unique_ptr<thread_fsm> finalizing
		= call_thread->release_thread_fsm ();
	      call_thread->set_thread_fsm (std::move (saved_sm));

	      finalizing->clean_up (call_thread.get ());
	    }

	    maybe_remove_breakpoints ();

	    gdb_assert (retval != NULL);

	    /* Destruct the pass-by-ref argument clones.  */
	    call_destructors (dtors_to_invoke, default_return_type);

	    return retval;
	  }
	else
	  infcall_debug_printf ("call did not complete");

	/* Didn't complete.  Clean up / destroy the call FSM, and restore the
	   previous state machine, and handle the error.  */
	{
	  std::unique_ptr<thread_fsm> finalizing
	    = call_thread->release_thread_fsm ();
	  call_thread->set_thread_fsm (std::move (saved_sm));

	  finalizing->clean_up (call_thread.get ());
	}
      }
  }

  /* Rethrow an error if we got one trying to run the inferior.  */

  if (e.reason < 0)
    {
      const char *name = get_function_name (funaddr,
					    name_buf, sizeof (name_buf));

      discard_infcall_control_state (inf_status.release ());

      /* We could discard the dummy frame here if the program exited,
	 but it will get garbage collected the next time the program is
	 run anyway.  */

      switch (e.reason)
	{
	case RETURN_ERROR:
	  throw_error (e.error, _("%s\n\
An error occurred while in a function called from GDB.\n\
Evaluation of the expression containing the function\n\
(%s) will be abandoned.\n\
When the function is done executing, GDB will silently stop."),
		       e.what (), name);
	case RETURN_QUIT:
	default:
	  throw_exception (std::move (e));
	}
    }

  /* If the program has exited, or we stopped at a different thread,
     exit and inform the user.  */

  if (! target_has_execution ())
    {
      const char *name = get_function_name (funaddr,
					    name_buf, sizeof (name_buf));

      /* If we try to restore the inferior status,
	 we'll crash as the inferior is no longer running.  */
      discard_infcall_control_state (inf_status.release ());

      /* We could discard the dummy frame here given that the program exited,
	 but it will get garbage collected the next time the program is
	 run anyway.  */

      error (_("The program being debugged exited while in a function "
	       "called from GDB.\n"
	       "Evaluation of the expression containing the function\n"
	       "(%s) will be abandoned."),
	     name);
    }

  if (call_thread_ptid != inferior_ptid)
    {
      const char *name = get_function_name (funaddr,
					    name_buf, sizeof (name_buf));

      /* We've switched threads.  This can happen if another thread gets a
	 signal or breakpoint while our thread was running.
	 There's no point in restoring the inferior status,
	 we're in a different thread.  */
      discard_infcall_control_state (inf_status.release ());
      /* Keep the dummy frame record, if the user switches back to the
	 thread with the hand-call, we'll need it.  */
      if (stopped_by_random_signal)
	error (_("\
The program received a signal in another thread while\n\
making a function call from GDB.\n\
Evaluation of the expression containing the function\n\
(%s) will be abandoned.\n\
When the function is done executing, GDB will silently stop."),
	       name);
      else
	error (_("\
The program stopped in another thread while making a function call from GDB.\n\
Evaluation of the expression containing the function\n\
(%s) will be abandoned.\n\
When the function is done executing, GDB will silently stop."),
	       name);
    }

    {
      /* Make a copy as NAME may be in an objfile freed by dummy_frame_pop.  */
      std::string name = get_function_name (funaddr, name_buf,
					    sizeof (name_buf));

      if (stopped_by_random_signal)
	{
	  /* We stopped inside the FUNCTION because of a random
	     signal.  Further execution of the FUNCTION is not
	     allowed.  */

	  if (unwind_on_signal_p)
	    {
	      /* The user wants the context restored.  */

	      /* Capture details of the signal so we can include them in
		 the error message.  Calling dummy_frame_pop will restore
		 the previous stop signal details.  */
	      gdb_signal stop_signal = call_thread->stop_signal ();

	      /* We must get back to the frame we were before the
		 dummy call.  */
	      dummy_frame_pop (dummy_id, call_thread.get ());

	      /* We also need to restore inferior status to that before the
		 dummy call.  */
	      restore_infcall_control_state (inf_status.release ());

	      /* FIXME: Insert a bunch of wrap_here; name can be very
		 long if it's a C++ name with arguments and stuff.  */
	      error (_("\
The program being debugged received signal %s, %s\n\
while in a function called from GDB.  GDB has restored the context\n\
to what it was before the call.  To change this behavior use\n\
\"set unwindonsignal off\".  Evaluation of the expression containing\n\
the function (%s) will be abandoned."),
		     gdb_signal_to_name (stop_signal),
		     gdb_signal_to_string (stop_signal),
		     name.c_str ());
	    }
	  else
	    {
	      /* The user wants to stay in the frame where we stopped
		 (default).
		 Discard inferior status, we're not at the same point
		 we started at.  */
	      discard_infcall_control_state (inf_status.release ());

	      /* FIXME: Insert a bunch of wrap_here; name can be very
		 long if it's a C++ name with arguments and stuff.  */
	      error (_("\
The program being debugged was signaled while in a function called from GDB.\n\
GDB remains in the frame where the signal was received.\n\
To change this behavior use \"set unwindonsignal on\".\n\
Evaluation of the expression containing the function\n\
(%s) will be abandoned.\n\
When the function is done executing, GDB will silently stop."),
		     name.c_str ());
	    }
	}

      if (stop_stack_dummy == STOP_STD_TERMINATE)
	{
	  /* We must get back to the frame we were before the dummy
	     call.  */
	  dummy_frame_pop (dummy_id, call_thread.get ());

	  /* We also need to restore inferior status to that before
	     the dummy call.  */
	  restore_infcall_control_state (inf_status.release ());

	  error (_("\
The program being debugged entered a std::terminate call, most likely\n\
caused by an unhandled C++ exception.  GDB blocked this call in order\n\
to prevent the program from being terminated, and has restored the\n\
context to its original state before the call.\n\
To change this behaviour use \"set unwind-on-terminating-exception off\".\n\
Evaluation of the expression containing the function (%s)\n\
will be abandoned."),
		 name.c_str ());
	}
      else if (stop_stack_dummy == STOP_NONE)
	{

	  /* We hit a breakpoint inside the FUNCTION.
	     Keep the dummy frame, the user may want to examine its state.
	     Discard inferior status, we're not at the same point
	     we started at.  */
	  discard_infcall_control_state (inf_status.release ());

	  /* The following error message used to say "The expression
	     which contained the function call has been discarded."
	     It is a hard concept to explain in a few words.  Ideally,
	     GDB would be able to resume evaluation of the expression
	     when the function finally is done executing.  Perhaps
	     someday this will be implemented (it would not be easy).  */
	  /* FIXME: Insert a bunch of wrap_here; name can be very long if it's
	     a C++ name with arguments and stuff.  */
	  error (_("\
The program being debugged stopped while in a function called from GDB.\n\
Evaluation of the expression containing the function\n\
(%s) will be abandoned.\n\
When the function is done executing, GDB will silently stop."),
		 name.c_str ());
	}

    }

  /* The above code errors out, so ...  */
  gdb_assert_not_reached ("... should not be here");
}

void _initialize_infcall ();
void
_initialize_infcall ()
{
  add_setshow_boolean_cmd ("may-call-functions", no_class,
			   &may_call_functions_p, _("\
Set permission to call functions in the program."), _("\
Show permission to call functions in the program."), _("\
When this permission is on, GDB may call functions in the program.\n\
Otherwise, any sort of attempt to call a function in the program\n\
will result in an error."),
			   NULL,
			   show_may_call_functions_p,
			   &setlist, &showlist);

  add_setshow_boolean_cmd ("coerce-float-to-double", class_obscure,
			   &coerce_float_to_double_p, _("\
Set coercion of floats to doubles when calling functions."), _("\
Show coercion of floats to doubles when calling functions."), _("\
Variables of type float should generally be converted to doubles before\n\
calling an unprototyped function, and left alone when calling a prototyped\n\
function.  However, some older debug info formats do not provide enough\n\
information to determine that a function is prototyped.  If this flag is\n\
set, GDB will perform the conversion for a function it considers\n\
unprototyped.\n\
The default is to perform the conversion."),
			   NULL,
			   show_coerce_float_to_double_p,
			   &setlist, &showlist);

  add_setshow_boolean_cmd ("unwindonsignal", no_class,
			   &unwind_on_signal_p, _("\
Set unwinding of stack if a signal is received while in a call dummy."), _("\
Show unwinding of stack if a signal is received while in a call dummy."), _("\
The unwindonsignal lets the user determine what gdb should do if a signal\n\
is received while in a function called from gdb (call dummy).  If set, gdb\n\
unwinds the stack and restore the context to what as it was before the call.\n\
The default is to stop in the frame where the signal was received."),
			   NULL,
			   show_unwind_on_signal_p,
			   &setlist, &showlist);

  add_setshow_boolean_cmd ("unwind-on-terminating-exception", no_class,
			   &unwind_on_terminating_exception_p, _("\
Set unwinding of stack if std::terminate is called while in call dummy."), _("\
Show unwinding of stack if std::terminate() is called while in a call dummy."),
			   _("\
The unwind on terminating exception flag lets the user determine\n\
what gdb should do if a std::terminate() call is made from the\n\
default exception handler.  If set, gdb unwinds the stack and restores\n\
the context to what it was before the call.  If unset, gdb allows the\n\
std::terminate call to proceed.\n\
The default is to unwind the frame."),
			   NULL,
			   show_unwind_on_terminating_exception_p,
			   &setlist, &showlist);

  add_setshow_boolean_cmd
    ("infcall", class_maintenance, &debug_infcall,
     _("Set inferior call debugging."),
     _("Show inferior call debugging."),
     _("When on, inferior function call specific debugging is enabled."),
     NULL, show_debug_infcall, &setdebuglist, &showdebuglist);
}
