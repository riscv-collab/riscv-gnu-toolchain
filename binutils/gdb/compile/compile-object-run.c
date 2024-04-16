/* Call module for 'compile' command.

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

#include "defs.h"
#include "compile-object-run.h"
#include "value.h"
#include "infcall.h"
#include "objfiles.h"
#include "compile-internal.h"
#include "dummy-frame.h"
#include "block.h"
#include "valprint.h"
#include "compile.h"

/* Helper for do_module_cleanup.  */

struct do_module_cleanup
{
  do_module_cleanup (int *ptr, compile_module_up &&mod)
    : executedp (ptr),
      module (std::move (mod))
  {
  }

  DISABLE_COPY_AND_ASSIGN (do_module_cleanup);

  /* Boolean to set true upon a call of do_module_cleanup.
     The pointer may be NULL.  */
  int *executedp;

  /* The compile module.  */
  compile_module_up module;
};

/* Cleanup everything after the inferior function dummy frame gets
   discarded.  */

static dummy_frame_dtor_ftype do_module_cleanup;
static void
do_module_cleanup (void *arg, int registers_valid)
{
  struct do_module_cleanup *data = (struct do_module_cleanup *) arg;

  if (data->executedp != NULL)
    {
      *data->executedp = 1;

      /* This code cannot be in compile_object_run as OUT_VALUE_TYPE
	 no longer exists there.  */
      if (data->module->scope == COMPILE_I_PRINT_ADDRESS_SCOPE
	  || data->module->scope == COMPILE_I_PRINT_VALUE_SCOPE)
	{
	  struct value *addr_value;
	  struct type *ptr_type
	    = lookup_pointer_type (data->module->out_value_type);

	  addr_value = value_from_pointer (ptr_type,
					   data->module->out_value_addr);

	  /* SCOPE_DATA would be stale unless EXECUTEDP != NULL.  */
	  compile_print_value (value_ind (addr_value),
			       data->module->scope_data);
	}
    }

  objfile *objfile = data->module->objfile;
  gdb_assert (objfile != nullptr);

  /* We have to make a copy of the name so that we can unlink the
     underlying file -- removing the objfile will cause the name to be
     freed, so we can't simply keep a reference to it.  */
  std::string objfile_name_s = objfile_name (objfile);

  objfile->unlink ();

  /* It may be a bit too pervasive in this dummy_frame dtor callback.  */
  clear_symtab_users (0);

  /* Delete the .c file.  */
  unlink (data->module->source_file.c_str ());

  /* Delete the .o file.  */
  unlink (objfile_name_s.c_str ());

  delete data;
}

/* Create a copy of FUNC_TYPE that is independent of OBJFILE.  */

static type *
create_copied_type_recursive (objfile *objfile, type *func_type)
{
  htab_up copied_types = create_copied_types_hash ();
  func_type = copy_type_recursive (func_type, copied_types.get ());
  return func_type;
}

/* Perform inferior call of MODULE.  This function may throw an error.
   This function may leave files referenced by MODULE on disk until
   the inferior call dummy frame is discarded.  This function may throw errors.
   Thrown errors and left MODULE files are unrelated events.  Caller must no
   longer touch MODULE's memory after this function has been called.  */

void
compile_object_run (compile_module_up &&module)
{
  struct value *func_val;
  struct do_module_cleanup *data;
  int dtor_found, executed = 0;
  struct symbol *func_sym = module->func_sym;
  CORE_ADDR regs_addr = module->regs_addr;
  struct objfile *objfile = module->objfile;

  data = new struct do_module_cleanup (&executed, std::move (module));

  try
    {
      struct type *func_type = func_sym->type ();
      int current_arg = 0;
      struct value **vargs;

      /* OBJFILE may disappear while FUNC_TYPE is still in use as a
	 result of the call to DO_MODULE_CLEANUP below, so we need a copy
	 that does not depend on the objfile in anyway.  */
      func_type = create_copied_type_recursive (objfile, func_type);

      gdb_assert (func_type->code () == TYPE_CODE_FUNC);
      func_val = value_from_pointer (lookup_pointer_type (func_type),
				   func_sym->value_block ()->entry_pc ());

      vargs = XALLOCAVEC (struct value *, func_type->num_fields ());
      if (func_type->num_fields () >= 1)
	{
	  gdb_assert (regs_addr != 0);
	  vargs[current_arg] = value_from_pointer
			  (func_type->field (current_arg).type (), regs_addr);
	  ++current_arg;
	}
      if (func_type->num_fields () >= 2)
	{
	  gdb_assert (data->module->out_value_addr != 0);
	  vargs[current_arg] = value_from_pointer
	       (func_type->field (current_arg).type (),
		data->module->out_value_addr);
	  ++current_arg;
	}
      gdb_assert (current_arg == func_type->num_fields ());
      auto args = gdb::make_array_view (vargs, func_type->num_fields ());
      call_function_by_hand_dummy (func_val, NULL, args,
				   do_module_cleanup, data);
    }
  catch (const gdb_exception_error &ex)
    {
      /* In the case of DTOR_FOUND or in the case of EXECUTED nothing
	 needs to be done.  */
      dtor_found = find_dummy_frame_dtor (do_module_cleanup, data);
      if (!executed)
	data->executedp = NULL;
      gdb_assert (!(dtor_found && executed));
      if (!dtor_found && !executed)
	do_module_cleanup (data, 0);
      throw;
    }

  dtor_found = find_dummy_frame_dtor (do_module_cleanup, data);
  gdb_assert (!dtor_found && executed);
}
