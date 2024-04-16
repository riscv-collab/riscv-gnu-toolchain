/* Perform an inferior function call, for GDB, the GNU debugger.

   Copyright (C) 2003-2024 Free Software Foundation, Inc.

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

#ifndef INFCALL_H
#define INFCALL_H

#include "dummy-frame.h"
#include "gdbsupport/array-view.h"

struct value;
struct type;

/* Determine a function's address and its return type from its value.
   If the function is a GNU ifunc, then return the address of the
   target function, and set *FUNCTION_TYPE to the target function's
   type, and *RETVAL_TYPE to the target function's return type.
   Calls error() if the function is not valid for calling.  */

extern CORE_ADDR find_function_addr (struct value *function, 
				     struct type **retval_type,
				     struct type **function_type = NULL);

/* Perform a function call in the inferior.

   ARGS is a vector of values of arguments.  FUNCTION is a value, the
   function to be called.  Returns a value representing what the
   function returned.  May fail to return, if a breakpoint or signal
   is hit during the execution of the function.

   DEFAULT_RETURN_TYPE is used as function return type if the return
   type is unknown.  This is used when calling functions with no debug
   info.

   ARGS is modified to contain coerced values.  */

extern struct value *call_function_by_hand (struct value *function,
					    type *default_return_type,
					    gdb::array_view<value *> args);

/* Similar to call_function_by_hand and additional call
   register_dummy_frame_dtor with DUMMY_DTOR and DUMMY_DTOR_DATA for the
   created inferior call dummy frame.  */

extern struct value *
  call_function_by_hand_dummy (struct value *function,
			       type *default_return_type,
			       gdb::array_view<value *> args,
			       dummy_frame_dtor_ftype *dummy_dtor,
			       void *dummy_dtor_data);

/* Throw an error indicating that the user tried to call a function
   that has unknown return type.  FUNC_NAME is the name of the
   function to be included in the error message; may be NULL, in which
   case the error message doesn't include a function name.  */

extern void error_call_unknown_return_type (const char *func_name);

#endif
