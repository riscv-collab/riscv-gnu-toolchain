/* Scheme interface to values.

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
#include "top.h"
#include "arch-utils.h"
#include "charset.h"
#include "cp-abi.h"
#include "target-float.h"
#include "infcall.h"
#include "symtab.h"
#include "language.h"
#include "valprint.h"
#include "value.h"
#include "guile-internal.h"

/* The <gdb:value> smob.  */

struct value_smob
{
  /* This always appears first.  */
  gdb_smob base;

  /* Doubly linked list of values in values_in_scheme.
     IWBN to use a chained_gdb_smob instead, which is doable, it just requires
     a bit more casting than normal.  */
  value_smob *next;
  value_smob *prev;

  struct value *value;

  /* These are cached here to avoid making multiple copies of them.
     Plus computing the dynamic_type can be a bit expensive.
     We use #f to indicate that the value doesn't exist (e.g. value doesn't
     have an address), so we need another value to indicate that we haven't
     computed the value yet.  For this we use SCM_UNDEFINED.  */
  SCM address;
  SCM type;
  SCM dynamic_type;
};

static const char value_smob_name[] = "gdb:value";

/* The tag Guile knows the value smob by.  */
static scm_t_bits value_smob_tag;

/* List of all values which are currently exposed to Scheme. It is
   maintained so that when an objfile is discarded, preserve_values
   can copy the values' types if needed.  */
static value_smob *values_in_scheme;

/* Keywords used by Scheme procedures in this file.  */
static SCM type_keyword;
static SCM encoding_keyword;
static SCM errors_keyword;
static SCM length_keyword;

/* Possible #:errors values.  */
static SCM error_symbol;
static SCM escape_symbol;
static SCM substitute_symbol;

/* Administrivia for value smobs.  */

/* Iterate over all the <gdb:value> objects, calling preserve_one_value on
   each.
   This is the extension_language_ops.preserve_values "method".  */

void
gdbscm_preserve_values (const struct extension_language_defn *extlang,
			struct objfile *objfile, htab_t copied_types)
{
  value_smob *iter;

  for (iter = values_in_scheme; iter; iter = iter->next)
    iter->value->preserve (objfile, copied_types);
}

/* Helper to add a value_smob to the global list.  */

static void
vlscm_remember_scheme_value (value_smob *v_smob)
{
  v_smob->next = values_in_scheme;
  if (v_smob->next)
    v_smob->next->prev = v_smob;
  v_smob->prev = NULL;
  values_in_scheme = v_smob;
}

/* Helper to remove a value_smob from the global list.  */

static void
vlscm_forget_value_smob (value_smob *v_smob)
{
  /* Remove SELF from the global list.  */
  if (v_smob->prev)
    v_smob->prev->next = v_smob->next;
  else
    {
      gdb_assert (values_in_scheme == v_smob);
      values_in_scheme = v_smob->next;
    }
  if (v_smob->next)
    v_smob->next->prev = v_smob->prev;
}

/* The smob "free" function for <gdb:value>.  */

static size_t
vlscm_free_value_smob (SCM self)
{
  value_smob *v_smob = (value_smob *) SCM_SMOB_DATA (self);

  vlscm_forget_value_smob (v_smob);
  v_smob->value->decref ();

  return 0;
}

/* The smob "print" function for <gdb:value>.  */

static int
vlscm_print_value_smob (SCM self, SCM port, scm_print_state *pstate)
{
  value_smob *v_smob = (value_smob *) SCM_SMOB_DATA (self);
  struct value_print_options opts;

  if (pstate->writingp)
    gdbscm_printf (port, "#<%s ", value_smob_name);

  get_user_print_options (&opts);
  opts.deref_ref = false;

  /* pstate->writingp = zero if invoked by display/~A, and nonzero if
     invoked by write/~S.  What to do here may need to evolve.
     IWBN if we could pass an argument to format that would we could use
     instead of writingp.  */
  opts.raw = !!pstate->writingp;

  gdbscm_gdb_exception exc {};
  try
    {
      string_file stb;

      common_val_print (v_smob->value, &stb, 0, &opts, current_language);
      scm_puts (stb.c_str (), port);
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  if (pstate->writingp)
    scm_puts (">", port);

  scm_remember_upto_here_1 (self);

  /* Non-zero means success.  */
  return 1;
}

/* The smob "equalp" function for <gdb:value>.  */

static SCM
vlscm_equal_p_value_smob (SCM v1, SCM v2)
{
  const value_smob *v1_smob = (value_smob *) SCM_SMOB_DATA (v1);
  const value_smob *v2_smob = (value_smob *) SCM_SMOB_DATA (v2);
  int result = 0;

  gdbscm_gdb_exception exc {};
  try
    {
      result = value_equal (v1_smob->value, v2_smob->value);
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  return scm_from_bool (result);
}

/* Low level routine to create a <gdb:value> object.  */

static SCM
vlscm_make_value_smob (void)
{
  value_smob *v_smob = (value_smob *)
    scm_gc_malloc (sizeof (value_smob), value_smob_name);
  SCM v_scm;

  /* These must be filled in by the caller.  */
  v_smob->value = NULL;
  v_smob->prev = NULL;
  v_smob->next = NULL;

  /* These are lazily computed.  */
  v_smob->address = SCM_UNDEFINED;
  v_smob->type = SCM_UNDEFINED;
  v_smob->dynamic_type = SCM_UNDEFINED;

  v_scm = scm_new_smob (value_smob_tag, (scm_t_bits) v_smob);
  gdbscm_init_gsmob (&v_smob->base);

  return v_scm;
}

/* Return non-zero if SCM is a <gdb:value> object.  */

int
vlscm_is_value (SCM scm)
{
  return SCM_SMOB_PREDICATE (value_smob_tag, scm);
}

/* (value? object) -> boolean */

static SCM
gdbscm_value_p (SCM scm)
{
  return scm_from_bool (vlscm_is_value (scm));
}

/* Create a new <gdb:value> object that encapsulates VALUE.
   The value is released from the all_values chain so its lifetime is not
   bound to the execution of a command.  */

SCM
vlscm_scm_from_value (struct value *value)
{
  /* N.B. It's important to not cause any side-effects until we know the
     conversion worked.  */
  SCM v_scm = vlscm_make_value_smob ();
  value_smob *v_smob = (value_smob *) SCM_SMOB_DATA (v_scm);

  v_smob->value = release_value (value).release ();
  vlscm_remember_scheme_value (v_smob);

  return v_scm;
}

/* Create a new <gdb:value> object that encapsulates VALUE.
   The value is not released from the all_values chain.  */

SCM
vlscm_scm_from_value_no_release (struct value *value)
{
  /* N.B. It's important to not cause any side-effects until we know the
     conversion worked.  */
  SCM v_scm = vlscm_make_value_smob ();
  value_smob *v_smob = (value_smob *) SCM_SMOB_DATA (v_scm);

  value->incref ();
  v_smob->value = value;
  vlscm_remember_scheme_value (v_smob);

  return v_scm;
}

/* Returns the <gdb:value> object in SELF.
   Throws an exception if SELF is not a <gdb:value> object.  */

static SCM
vlscm_get_value_arg_unsafe (SCM self, int arg_pos, const char *func_name)
{
  SCM_ASSERT_TYPE (vlscm_is_value (self), self, arg_pos, func_name,
		   value_smob_name);

  return self;
}

/* Returns a pointer to the value smob of SELF.
   Throws an exception if SELF is not a <gdb:value> object.  */

static value_smob *
vlscm_get_value_smob_arg_unsafe (SCM self, int arg_pos, const char *func_name)
{
  SCM v_scm = vlscm_get_value_arg_unsafe (self, arg_pos, func_name);
  value_smob *v_smob = (value_smob *) SCM_SMOB_DATA (v_scm);

  return v_smob;
}

/* Return the value field of V_SCM, an object of type <gdb:value>.
   This exists so that we don't have to export the struct's contents.  */

struct value *
vlscm_scm_to_value (SCM v_scm)
{
  value_smob *v_smob;

  gdb_assert (vlscm_is_value (v_scm));
  v_smob = (value_smob *) SCM_SMOB_DATA (v_scm);
  return v_smob->value;
}

/* Value methods.  */

/* (make-value x [#:type type]) -> <gdb:value> */

static SCM
gdbscm_make_value (SCM x, SCM rest)
{
  const SCM keywords[] = { type_keyword, SCM_BOOL_F };

  int type_arg_pos = -1;
  SCM type_scm = SCM_UNDEFINED;
  gdbscm_parse_function_args (FUNC_NAME, SCM_ARG2, keywords, "#O", rest,
			      &type_arg_pos, &type_scm);

  struct type *type = NULL;
  if (type_arg_pos > 0)
    {
      type_smob *t_smob = tyscm_get_type_smob_arg_unsafe (type_scm,
							  type_arg_pos,
							  FUNC_NAME);
      type = tyscm_type_smob_type (t_smob);
    }

  return gdbscm_wrap ([=]
    {
      scoped_value_mark free_values;

      SCM except_scm;
      struct value *value
	= vlscm_convert_typed_value_from_scheme (FUNC_NAME, SCM_ARG1, x,
						 type_arg_pos, type_scm, type,
						 &except_scm,
						 get_current_arch (),
						 current_language);
      if (value == NULL)
	return except_scm;

      return vlscm_scm_from_value (value);
    });
}

/* (make-lazy-value <gdb:type> address) -> <gdb:value> */

static SCM
gdbscm_make_lazy_value (SCM type_scm, SCM address_scm)
{
  type_smob *t_smob = tyscm_get_type_smob_arg_unsafe (type_scm,
						      SCM_ARG1, FUNC_NAME);
  struct type *type = tyscm_type_smob_type (t_smob);

  ULONGEST address;
  gdbscm_parse_function_args (FUNC_NAME, SCM_ARG2, NULL, "U",
			      address_scm, &address);

  return gdbscm_wrap ([=]
    {
      scoped_value_mark free_values;

      struct value *value = value_from_contents_and_address (type, NULL,
							     address);
      return vlscm_scm_from_value (value);
    });
}

/* (value-optimized-out? <gdb:value>) -> boolean */

static SCM
gdbscm_value_optimized_out_p (SCM self)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  return gdbscm_wrap ([=]
    {
      return scm_from_bool (v_smob->value->optimized_out ());
    });
}

/* (value-address <gdb:value>) -> integer
   Returns #f if the value doesn't have one.  */

static SCM
gdbscm_value_address (SCM self)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct value *value = v_smob->value;

  return gdbscm_wrap ([=]
    {
      if (SCM_UNBNDP (v_smob->address))
	{
	  scoped_value_mark free_values;

	  SCM address = SCM_BOOL_F;

	  try
	    {
	      address = vlscm_scm_from_value (value_addr (value));
	    }
	  catch (const gdb_exception_forced_quit &except)
	    {
	      quit_force (NULL, 0);
	    }
	  catch (const gdb_exception &except)
	    {
	    }

	  if (gdbscm_is_exception (address))
	    return address;

	  v_smob->address = address;
	}

      return v_smob->address;
    });
}

/* (value-dereference <gdb:value>) -> <gdb:value>
   Given a value of a pointer type, apply the C unary * operator to it.  */

static SCM
gdbscm_value_dereference (SCM self)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  return gdbscm_wrap ([=]
    {
      scoped_value_mark free_values;

      struct value *res_val = value_ind (v_smob->value);
      return vlscm_scm_from_value (res_val);
    });
}

/* (value-referenced-value <gdb:value>) -> <gdb:value>
   Given a value of a reference type, return the value referenced.
   The difference between this function and gdbscm_value_dereference is that
   the latter applies * unary operator to a value, which need not always
   result in the value referenced.
   For example, for a value which is a reference to an 'int' pointer ('int *'),
   gdbscm_value_dereference will result in a value of type 'int' while
   gdbscm_value_referenced_value will result in a value of type 'int *'.  */

static SCM
gdbscm_value_referenced_value (SCM self)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct value *value = v_smob->value;

  return gdbscm_wrap ([=]
    {
      scoped_value_mark free_values;

      struct value *res_val;

      switch (check_typedef (value->type ())->code ())
	{
	case TYPE_CODE_PTR:
	  res_val = value_ind (value);
	  break;
	case TYPE_CODE_REF:
	case TYPE_CODE_RVALUE_REF:
	  res_val = coerce_ref (value);
	  break;
	default:
	  error (_("Trying to get the referenced value from a value which is"
		   " neither a pointer nor a reference"));
	}

      return vlscm_scm_from_value (res_val);
    });
}

static SCM
gdbscm_reference_value (SCM self, enum type_code refcode)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct value *value = v_smob->value;

  return gdbscm_wrap ([=]
    {
      scoped_value_mark free_values;

      struct value *res_val = value_ref (value, refcode);
      return vlscm_scm_from_value (res_val);
    });
}

/* (value-reference-value <gdb:value>) -> <gdb:value> */

static SCM
gdbscm_value_reference_value (SCM self)
{
  return gdbscm_reference_value (self, TYPE_CODE_REF);
}

/* (value-rvalue-reference-value <gdb:value>) -> <gdb:value> */

static SCM
gdbscm_value_rvalue_reference_value (SCM self)
{
  return gdbscm_reference_value (self, TYPE_CODE_RVALUE_REF);
}

/* (value-const-value <gdb:value>) -> <gdb:value> */

static SCM
gdbscm_value_const_value (SCM self)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct value *value = v_smob->value;

  return gdbscm_wrap ([=]
    {
      scoped_value_mark free_values;

      struct value *res_val = make_cv_value (1, 0, value);
      return vlscm_scm_from_value (res_val);
    });
}

/* (value-type <gdb:value>) -> <gdb:type> */

static SCM
gdbscm_value_type (SCM self)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct value *value = v_smob->value;

  if (SCM_UNBNDP (v_smob->type))
    v_smob->type = tyscm_scm_from_type (value->type ());

  return v_smob->type;
}

/* (value-dynamic-type <gdb:value>) -> <gdb:type> */

static SCM
gdbscm_value_dynamic_type (SCM self)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct value *value = v_smob->value;
  struct type *type = NULL;

  if (! SCM_UNBNDP (v_smob->dynamic_type))
    return v_smob->dynamic_type;

  gdbscm_gdb_exception exc {};
  try
    {
      scoped_value_mark free_values;

      type = value->type ();
      type = check_typedef (type);

      if (((type->code () == TYPE_CODE_PTR)
	   || (type->code () == TYPE_CODE_REF))
	  && (type->target_type ()->code () == TYPE_CODE_STRUCT))
	{
	  struct value *target;
	  int was_pointer = type->code () == TYPE_CODE_PTR;

	  if (was_pointer)
	    target = value_ind (value);
	  else
	    target = coerce_ref (value);
	  type = value_rtti_type (target, NULL, NULL, NULL);

	  if (type)
	    {
	      if (was_pointer)
		type = lookup_pointer_type (type);
	      else
		type = lookup_lvalue_reference_type (type);
	    }
	}
      else if (type->code () == TYPE_CODE_STRUCT)
	type = value_rtti_type (value, NULL, NULL, NULL);
      else
	{
	  /* Re-use object's static type.  */
	  type = NULL;
	}
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  if (type == NULL)
    v_smob->dynamic_type = gdbscm_value_type (self);
  else
    v_smob->dynamic_type = tyscm_scm_from_type (type);

  return v_smob->dynamic_type;
}

/* A helper function that implements the various cast operators.  */

static SCM
vlscm_do_cast (SCM self, SCM type_scm, enum exp_opcode op,
	       const char *func_name)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct value *value = v_smob->value;
  type_smob *t_smob
    = tyscm_get_type_smob_arg_unsafe (type_scm, SCM_ARG2, FUNC_NAME);
  struct type *type = tyscm_type_smob_type (t_smob);

  return gdbscm_wrap ([=]
    {
      scoped_value_mark free_values;

      struct value *res_val;
      if (op == UNOP_DYNAMIC_CAST)
	res_val = value_dynamic_cast (type, value);
      else if (op == UNOP_REINTERPRET_CAST)
	res_val = value_reinterpret_cast (type, value);
      else
	{
	  gdb_assert (op == UNOP_CAST);
	  res_val = value_cast (type, value);
	}

      return vlscm_scm_from_value (res_val);
    });
}

/* (value-cast <gdb:value> <gdb:type>) -> <gdb:value> */

static SCM
gdbscm_value_cast (SCM self, SCM new_type)
{
  return vlscm_do_cast (self, new_type, UNOP_CAST, FUNC_NAME);
}

/* (value-dynamic-cast <gdb:value> <gdb:type>) -> <gdb:value> */

static SCM
gdbscm_value_dynamic_cast (SCM self, SCM new_type)
{
  return vlscm_do_cast (self, new_type, UNOP_DYNAMIC_CAST, FUNC_NAME);
}

/* (value-reinterpret-cast <gdb:value> <gdb:type>) -> <gdb:value> */

static SCM
gdbscm_value_reinterpret_cast (SCM self, SCM new_type)
{
  return vlscm_do_cast (self, new_type, UNOP_REINTERPRET_CAST, FUNC_NAME);
}

/* (value-field <gdb:value> string) -> <gdb:value>
   Given string name of an element inside structure, return its <gdb:value>
   object.  */

static SCM
gdbscm_value_field (SCM self, SCM field_scm)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  SCM_ASSERT_TYPE (scm_is_string (field_scm), field_scm, SCM_ARG2, FUNC_NAME,
		   _("string"));

  return gdbscm_wrap ([=]
    {
      scoped_value_mark free_values;

      gdb::unique_xmalloc_ptr<char> field = gdbscm_scm_to_c_string (field_scm);

      struct value *tmp = v_smob->value;

      struct value *res_val = value_struct_elt (&tmp, {}, field.get (), NULL,
						"struct/class/union");

      return vlscm_scm_from_value (res_val);
    });
}

/* (value-subscript <gdb:value> integer|<gdb:value>) -> <gdb:value>
   Return the specified value in an array.  */

static SCM
gdbscm_value_subscript (SCM self, SCM index_scm)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct value *value = v_smob->value;
  struct type *type = value->type ();

  SCM_ASSERT (type != NULL, self, SCM_ARG2, FUNC_NAME);

  return gdbscm_wrap ([=]
    {
      scoped_value_mark free_values;

      SCM except_scm;
      struct value *index
	= vlscm_convert_value_from_scheme (FUNC_NAME, SCM_ARG2, index_scm,
					   &except_scm,
					   type->arch (),
					   current_language);
      if (index == NULL)
	return except_scm;

      /* Assume we are attempting an array access, and let the value code
	 throw an exception if the index has an invalid type.
	 Check the value's type is something that can be accessed via
	 a subscript.  */
      struct value *tmp = coerce_ref (value);
      struct type *tmp_type = check_typedef (tmp->type ());
      if (tmp_type->code () != TYPE_CODE_ARRAY
	  && tmp_type->code () != TYPE_CODE_PTR)
	error (_("Cannot subscript requested type"));

      struct value *res_val = value_subscript (tmp, value_as_long (index));
      return vlscm_scm_from_value (res_val);
    });
}

/* (value-call <gdb:value> arg-list) -> <gdb:value>
   Perform an inferior function call on the value.  */

static SCM
gdbscm_value_call (SCM self, SCM args)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct value *function = v_smob->value;
  struct type *ftype = NULL;
  long args_count;
  struct value **vargs = NULL;

  gdbscm_gdb_exception exc {};
  try
    {
      ftype = check_typedef (function->type ());
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  SCM_ASSERT_TYPE (ftype->code () == TYPE_CODE_FUNC, self,
		   SCM_ARG1, FUNC_NAME,
		   _("function (value of TYPE_CODE_FUNC)"));

  SCM_ASSERT_TYPE (gdbscm_is_true (scm_list_p (args)), args,
		   SCM_ARG2, FUNC_NAME, _("list"));

  args_count = scm_ilength (args);
  if (args_count > 0)
    {
      struct gdbarch *gdbarch = get_current_arch ();
      const struct language_defn *language = current_language;
      SCM except_scm;
      long i;

      vargs = XALLOCAVEC (struct value *, args_count);
      for (i = 0; i < args_count; i++)
	{
	  SCM arg = scm_car (args);

	  vargs[i] = vlscm_convert_value_from_scheme (FUNC_NAME,
						      GDBSCM_ARG_NONE, arg,
						      &except_scm,
						      gdbarch, language);
	  if (vargs[i] == NULL)
	    gdbscm_throw (except_scm);

	  args = scm_cdr (args);
	}
      gdb_assert (gdbscm_is_true (scm_null_p (args)));
    }

  return gdbscm_wrap ([=]
    {
      scoped_value_mark free_values;

      auto av = gdb::make_array_view (vargs, args_count);
      value *return_value = call_function_by_hand (function, NULL, av);
      return vlscm_scm_from_value (return_value);
    });
}

/* (value->bytevector <gdb:value>) -> bytevector */

static SCM
gdbscm_value_to_bytevector (SCM self)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct value *value = v_smob->value;
  struct type *type;
  size_t length = 0;
  const gdb_byte *contents = NULL;
  SCM bv;

  type = value->type ();

  gdbscm_gdb_exception exc {};
  try
    {
      type = check_typedef (type);
      length = type->length ();
      contents = value->contents ().data ();
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  bv = scm_c_make_bytevector (length);
  memcpy (SCM_BYTEVECTOR_CONTENTS (bv), contents, length);

  return bv;
}

/* Helper function to determine if a type is "int-like".  */

static int
is_intlike (struct type *type, int ptr_ok)
{
  return (type->code () == TYPE_CODE_INT
	  || type->code () == TYPE_CODE_ENUM
	  || type->code () == TYPE_CODE_BOOL
	  || type->code () == TYPE_CODE_CHAR
	  || (ptr_ok && type->code () == TYPE_CODE_PTR));
}

/* (value->bool <gdb:value>) -> boolean
   Throws an error if the value is not integer-like.  */

static SCM
gdbscm_value_to_bool (SCM self)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct value *value = v_smob->value;
  struct type *type;
  LONGEST l = 0;

  type = value->type ();

  gdbscm_gdb_exception exc {};
  try
    {
      type = check_typedef (type);
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  SCM_ASSERT_TYPE (is_intlike (type, 1), self, SCM_ARG1, FUNC_NAME,
		   _("integer-like gdb value"));

  try
    {
      if (type->code () == TYPE_CODE_PTR)
	l = value_as_address (value);
      else
	l = value_as_long (value);
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  return scm_from_bool (l != 0);
}

/* (value->integer <gdb:value>) -> integer
   Throws an error if the value is not integer-like.  */

static SCM
gdbscm_value_to_integer (SCM self)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct value *value = v_smob->value;
  struct type *type;
  LONGEST l = 0;

  type = value->type ();

  gdbscm_gdb_exception exc {};
  try
    {
      type = check_typedef (type);
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  SCM_ASSERT_TYPE (is_intlike (type, 1), self, SCM_ARG1, FUNC_NAME,
		   _("integer-like gdb value"));

  try
    {
      if (type->code () == TYPE_CODE_PTR)
	l = value_as_address (value);
      else
	l = value_as_long (value);
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  if (type->is_unsigned ())
    return gdbscm_scm_from_ulongest (l);
  else
    return gdbscm_scm_from_longest (l);
}

/* (value->real <gdb:value>) -> real
   Throws an error if the value is not a number.  */

static SCM
gdbscm_value_to_real (SCM self)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct value *value = v_smob->value;
  struct type *type;
  double d = 0;
  struct value *check = nullptr;

  type = value->type ();

  gdbscm_gdb_exception exc {};
  try
    {
      type = check_typedef (type);
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  SCM_ASSERT_TYPE (is_intlike (type, 0) || type->code () == TYPE_CODE_FLT,
		   self, SCM_ARG1, FUNC_NAME, _("number"));

  try
    {
      if (is_floating_value (value))
	{
	  d = target_float_to_host_double (value->contents ().data (),
					   type);
	  check = value_from_host_double (type, d);
	}
      else if (type->is_unsigned ())
	{
	  d = (ULONGEST) value_as_long (value);
	  check = value_from_ulongest (type, (ULONGEST) d);
	}
      else
	{
	  d = value_as_long (value);
	  check = value_from_longest (type, (LONGEST) d);
	}
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  /* TODO: Is there a better way to check if the value fits?  */
  if (!value_equal (value, check))
    gdbscm_out_of_range_error (FUNC_NAME, SCM_ARG1, self,
			       _("number can't be converted to a double"));

  return scm_from_double (d);
}

/* (value->string <gdb:value>
       [#:encoding encoding]
       [#:errors #f | 'error | 'substitute]
       [#:length length])
     -> string
   Return Unicode string with value's contents, which must be a string.

   If ENCODING is not given, the string is assumed to be encoded in
   the target's charset.

   ERRORS is one of #f, 'error or 'substitute.
   An error setting of #f means use the default, which is Guile's
   %default-port-conversion-strategy when using Guile >= 2.0.6, or 'error if
   using an earlier version of Guile.  Earlier versions do not properly
   support obtaining the default port conversion strategy.
   If the default is not one of 'error or 'substitute, 'substitute is used.
   An error setting of "error" causes an exception to be thrown if there's
   a decoding error.  An error setting of "substitute" causes invalid
   characters to be replaced with "?".

   If LENGTH is provided, only fetch string to the length provided.
   LENGTH must be a Scheme integer, it can't be a <gdb:value> integer.  */

static SCM
gdbscm_value_to_string (SCM self, SCM rest)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct value *value = v_smob->value;
  const SCM keywords[] = {
    encoding_keyword, errors_keyword, length_keyword, SCM_BOOL_F
  };
  int encoding_arg_pos = -1, errors_arg_pos = -1, length_arg_pos = -1;
  char *encoding = NULL;
  SCM errors = SCM_BOOL_F;
  /* Avoid an uninitialized warning from gcc.  */
  gdb_byte *buffer_contents = nullptr;
  int length = -1;
  const char *la_encoding = NULL;
  struct type *char_type = NULL;
  SCM result;

  /* The sequencing here, as everywhere else, is important.
     We can't have existing cleanups when a Scheme exception is thrown.  */

  gdbscm_parse_function_args (FUNC_NAME, SCM_ARG2, keywords, "#sOi", rest,
			      &encoding_arg_pos, &encoding,
			      &errors_arg_pos, &errors,
			      &length_arg_pos, &length);

  if (errors_arg_pos > 0
      && errors != SCM_BOOL_F
      && !scm_is_eq (errors, error_symbol)
      && !scm_is_eq (errors, substitute_symbol))
    {
      SCM excp
	= gdbscm_make_out_of_range_error (FUNC_NAME, errors_arg_pos, errors,
					  _("invalid error kind"));

      xfree (encoding);
      gdbscm_throw (excp);
    }
  if (errors == SCM_BOOL_F)
    {
      /* N.B. scm_port_conversion_strategy in Guile versions prior to 2.0.6
	 will throw a Scheme error when passed #f.  */
      if (gdbscm_guile_version_is_at_least (2, 0, 6))
	errors = scm_port_conversion_strategy (SCM_BOOL_F);
      else
	errors = error_symbol;
    }
  /* We don't assume anything about the result of scm_port_conversion_strategy.
     From this point on, if errors is not 'errors, use 'substitute.  */

  gdbscm_gdb_exception exc {};
  try
    {
      gdb::unique_xmalloc_ptr<gdb_byte> buffer;
      c_get_string (value, &buffer, &length, &char_type, &la_encoding);
      buffer_contents = buffer.release ();
    }
  catch (const gdb_exception &except)
    {
      xfree (encoding);
      exc = unpack (except);
    }
  GDBSCM_HANDLE_GDB_EXCEPTION (exc);

  /* If errors is "error", scm_from_stringn may throw a Scheme exception.
     Make sure we don't leak.  This is done via scm_dynwind_begin, et.al.  */

  scm_dynwind_begin ((scm_t_dynwind_flags) 0);

  gdbscm_dynwind_xfree (encoding);
  gdbscm_dynwind_xfree (buffer_contents);

  result = scm_from_stringn ((const char *) buffer_contents,
			     length * char_type->length (),
			     (encoding != NULL && *encoding != '\0'
			      ? encoding
			      : la_encoding),
			     scm_is_eq (errors, error_symbol)
			     ? SCM_FAILED_CONVERSION_ERROR
			     : SCM_FAILED_CONVERSION_QUESTION_MARK);

  scm_dynwind_end ();

  return result;
}

/* (value->lazy-string <gdb:value> [#:encoding encoding] [#:length length])
     -> <gdb:lazy-string>
   Return a Scheme object representing a lazy_string_object type.
   A lazy string is a pointer to a string with an optional encoding and length.
   If ENCODING is not given, the target's charset is used.
   If LENGTH is provided then the length parameter is set to LENGTH.
   Otherwise if the value is an array of known length then the array's length
   is used.  Otherwise the length will be set to -1 (meaning first null of
   appropriate with).
   LENGTH must be a Scheme integer, it can't be a <gdb:value> integer.  */

static SCM
gdbscm_value_to_lazy_string (SCM self, SCM rest)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct value *value = v_smob->value;
  const SCM keywords[] = { encoding_keyword, length_keyword, SCM_BOOL_F };
  int encoding_arg_pos = -1, length_arg_pos = -1;
  char *encoding = NULL;
  int length = -1;
  SCM result = SCM_BOOL_F; /* -Wall */
  gdbscm_gdb_exception except {};

  /* The sequencing here, as everywhere else, is important.
     We can't have existing cleanups when a Scheme exception is thrown.  */

  gdbscm_parse_function_args (FUNC_NAME, SCM_ARG2, keywords, "#si", rest,
			      &encoding_arg_pos, &encoding,
			      &length_arg_pos, &length);

  if (length < -1)
    {
      gdbscm_out_of_range_error (FUNC_NAME, length_arg_pos,
				 scm_from_int (length),
				 _("invalid length"));
    }

  try
    {
      scoped_value_mark free_values;

      struct type *type, *realtype;
      CORE_ADDR addr;

      type = value->type ();
      realtype = check_typedef (type);

      switch (realtype->code ())
	{
	case TYPE_CODE_ARRAY:
	  {
	    LONGEST array_length = -1;
	    LONGEST low_bound, high_bound;

	    /* PR 20786: There's no way to specify an array of length zero.
	       Record a length of [0,-1] which is how Ada does it.  Anything
	       we do is broken, but this one possible solution.  */
	    if (get_array_bounds (realtype, &low_bound, &high_bound))
	      array_length = high_bound - low_bound + 1;
	    if (length == -1)
	      length = array_length;
	    else if (array_length == -1)
	      {
		type = lookup_array_range_type (realtype->target_type (),
						0, length - 1);
	      }
	    else if (length != array_length)
	      {
		/* We need to create a new array type with the
		   specified length.  */
		if (length > array_length)
		  error (_("length is larger than array size"));
		type = lookup_array_range_type (type->target_type (),
						low_bound,
						low_bound + length - 1);
	      }
	    addr = value->address ();
	    break;
	  }
	case TYPE_CODE_PTR:
	  /* If a length is specified we defer creating an array of the
	     specified width until we need to.  */
	  addr = value_as_address (value);
	  break;
	default:
	  /* Should flag an error here.  PR 20769.  */
	  addr = value->address ();
	  break;
	}

      result = lsscm_make_lazy_string (addr, length, encoding, type);
    }
  catch (const gdb_exception &ex)
    {
      except = unpack (ex);
    }

  xfree (encoding);
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  if (gdbscm_is_exception (result))
    gdbscm_throw (result);

  return result;
}

/* (value-lazy? <gdb:value>) -> boolean */

static SCM
gdbscm_value_lazy_p (SCM self)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct value *value = v_smob->value;

  return scm_from_bool (value->lazy ());
}

/* (value-fetch-lazy! <gdb:value>) -> unspecified */

static SCM
gdbscm_value_fetch_lazy_x (SCM self)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct value *value = v_smob->value;

  return gdbscm_wrap ([=]
    {
      if (value->lazy ())
	value->fetch_lazy ();
      return SCM_UNSPECIFIED;
    });
}

/* (value-print <gdb:value>) -> string */

static SCM
gdbscm_value_print (SCM self)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct value *value = v_smob->value;
  struct value_print_options opts;

  get_user_print_options (&opts);
  opts.deref_ref = false;

  string_file stb;

  gdbscm_gdb_exception exc {};
  try
    {
      common_val_print (value, &stb, 0, &opts, current_language);
    }
  catch (const gdb_exception &except)
    {
      exc = unpack (except);
    }

  GDBSCM_HANDLE_GDB_EXCEPTION (exc);
  /* Use SCM_FAILED_CONVERSION_QUESTION_MARK to ensure this doesn't
     throw an error if the encoding fails.
     IWBN to use scm_take_locale_string here, but we'd have to temporarily
     override the default port conversion handler because contrary to
     documentation it doesn't necessarily free the input string.  */
  return scm_from_stringn (stb.c_str (), stb.size (), host_charset (),
			   SCM_FAILED_CONVERSION_QUESTION_MARK);
}

/* (parse-and-eval string) -> <gdb:value>
   Parse a string and evaluate the string as an expression.  */

static SCM
gdbscm_parse_and_eval (SCM expr_scm)
{
  char *expr_str;
  gdbscm_parse_function_args (FUNC_NAME, SCM_ARG1, NULL, "s",
			      expr_scm, &expr_str);

  return gdbscm_wrap ([=]
    {
      scoped_value_mark free_values;
      return vlscm_scm_from_value (parse_and_eval (expr_str));
    });
}

/* (history-ref integer) -> <gdb:value>
   Return the specified value from GDB's value history.  */

static SCM
gdbscm_history_ref (SCM index)
{
  int i;
  gdbscm_parse_function_args (FUNC_NAME, SCM_ARG1, NULL, "i", index, &i);

  return gdbscm_wrap ([=]
    {
      return vlscm_scm_from_value (access_value_history (i));
    });
}

/* (history-append! <gdb:value>) -> index
   Append VALUE to GDB's value history.  Return its index in the history.  */

static SCM
gdbscm_history_append_x (SCM value)
{
  value_smob *v_smob
    = vlscm_get_value_smob_arg_unsafe (value, SCM_ARG1, FUNC_NAME);
  return gdbscm_wrap ([=]
    {
      return scm_from_int (v_smob->value->record_latest ());
    });
}

/* Initialize the Scheme value code.  */

static const scheme_function value_functions[] =
{
  { "value?", 1, 0, 0, as_a_scm_t_subr (gdbscm_value_p),
    "\
Return #t if the object is a <gdb:value> object." },

  { "make-value", 1, 0, 1, as_a_scm_t_subr (gdbscm_make_value),
    "\
Create a <gdb:value> representing object.\n\
Typically this is used to convert numbers and strings to\n\
<gdb:value> objects.\n\
\n\
  Arguments: object [#:type <gdb:type>]" },

  { "value-optimized-out?", 1, 0, 0,
    as_a_scm_t_subr (gdbscm_value_optimized_out_p),
    "\
Return #t if the value has been optimized out." },

  { "value-address", 1, 0, 0, as_a_scm_t_subr (gdbscm_value_address),
    "\
Return the address of the value." },

  { "value-type", 1, 0, 0, as_a_scm_t_subr (gdbscm_value_type),
    "\
Return the type of the value." },

  { "value-dynamic-type", 1, 0, 0, as_a_scm_t_subr (gdbscm_value_dynamic_type),
    "\
Return the dynamic type of the value." },

  { "value-cast", 2, 0, 0, as_a_scm_t_subr (gdbscm_value_cast),
    "\
Cast the value to the supplied type.\n\
\n\
  Arguments: <gdb:value> <gdb:type>" },

  { "value-dynamic-cast", 2, 0, 0, as_a_scm_t_subr (gdbscm_value_dynamic_cast),
    "\
Cast the value to the supplied type, as if by the C++\n\
dynamic_cast operator.\n\
\n\
  Arguments: <gdb:value> <gdb:type>" },

  { "value-reinterpret-cast", 2, 0, 0,
    as_a_scm_t_subr (gdbscm_value_reinterpret_cast),
    "\
Cast the value to the supplied type, as if by the C++\n\
reinterpret_cast operator.\n\
\n\
  Arguments: <gdb:value> <gdb:type>" },

  { "value-dereference", 1, 0, 0, as_a_scm_t_subr (gdbscm_value_dereference),
    "\
Return the result of applying the C unary * operator to the value." },

  { "value-referenced-value", 1, 0, 0,
    as_a_scm_t_subr (gdbscm_value_referenced_value),
    "\
Given a value of a reference type, return the value referenced.\n\
The difference between this function and value-dereference is that\n\
the latter applies * unary operator to a value, which need not always\n\
result in the value referenced.\n\
For example, for a value which is a reference to an 'int' pointer ('int *'),\n\
value-dereference will result in a value of type 'int' while\n\
value-referenced-value will result in a value of type 'int *'." },

  { "value-reference-value", 1, 0, 0,
    as_a_scm_t_subr (gdbscm_value_reference_value),
    "\
Return a <gdb:value> object which is a reference to the given value." },

  { "value-rvalue-reference-value", 1, 0, 0,
    as_a_scm_t_subr (gdbscm_value_rvalue_reference_value),
    "\
Return a <gdb:value> object which is an rvalue reference to the given value." },

  { "value-const-value", 1, 0, 0,
    as_a_scm_t_subr (gdbscm_value_const_value),
    "\
Return a <gdb:value> object which is a 'const' version of the given value." },

  { "value-field", 2, 0, 0, as_a_scm_t_subr (gdbscm_value_field),
    "\
Return the specified field of the value.\n\
\n\
  Arguments: <gdb:value> string" },

  { "value-subscript", 2, 0, 0, as_a_scm_t_subr (gdbscm_value_subscript),
    "\
Return the value of the array at the specified index.\n\
\n\
  Arguments: <gdb:value> integer" },

  { "value-call", 2, 0, 0, as_a_scm_t_subr (gdbscm_value_call),
    "\
Perform an inferior function call taking the value as a pointer to the\n\
function to call.\n\
Each element of the argument list must be a <gdb:value> object or an object\n\
that can be converted to one.\n\
The result is the value returned by the function.\n\
\n\
  Arguments: <gdb:value> arg-list" },

  { "value->bool", 1, 0, 0, as_a_scm_t_subr (gdbscm_value_to_bool),
    "\
Return the Scheme boolean representing the GDB value.\n\
The value must be \"integer like\".  Pointers are ok." },

  { "value->integer", 1, 0, 0, as_a_scm_t_subr (gdbscm_value_to_integer),
    "\
Return the Scheme integer representing the GDB value.\n\
The value must be \"integer like\".  Pointers are ok." },

  { "value->real", 1, 0, 0, as_a_scm_t_subr (gdbscm_value_to_real),
    "\
Return the Scheme real number representing the GDB value.\n\
The value must be a number." },

  { "value->bytevector", 1, 0, 0, as_a_scm_t_subr (gdbscm_value_to_bytevector),
    "\
Return a Scheme bytevector with the raw contents of the GDB value.\n\
No transformation, endian or otherwise, is performed." },

  { "value->string", 1, 0, 1, as_a_scm_t_subr (gdbscm_value_to_string),
    "\
Return the Unicode string of the value's contents.\n\
If ENCODING is not given, the string is assumed to be encoded in\n\
the target's charset.\n\
An error setting \"error\" causes an exception to be thrown if there's\n\
a decoding error.  An error setting of \"substitute\" causes invalid\n\
characters to be replaced with \"?\".  The default is \"error\".\n\
If LENGTH is provided, only fetch string to the length provided.\n\
\n\
  Arguments: <gdb:value>\n\
	     [#:encoding encoding] [#:errors \"error\"|\"substitute\"]\n\
	     [#:length length]" },

  { "value->lazy-string", 1, 0, 1,
    as_a_scm_t_subr (gdbscm_value_to_lazy_string),
    "\
Return a Scheme object representing a lazily fetched Unicode string\n\
of the value's contents.\n\
If ENCODING is not given, the string is assumed to be encoded in\n\
the target's charset.\n\
If LENGTH is provided, only fetch string to the length provided.\n\
\n\
  Arguments: <gdb:value> [#:encoding encoding] [#:length length]" },

  { "value-lazy?", 1, 0, 0, as_a_scm_t_subr (gdbscm_value_lazy_p),
    "\
Return #t if the value is lazy (not fetched yet from the inferior).\n\
A lazy value is fetched when needed, or when the value-fetch-lazy! function\n\
is called." },

  { "make-lazy-value", 2, 0, 0, as_a_scm_t_subr (gdbscm_make_lazy_value),
    "\
Create a <gdb:value> that will be lazily fetched from the target.\n\
\n\
  Arguments: <gdb:type> address" },

  { "value-fetch-lazy!", 1, 0, 0, as_a_scm_t_subr (gdbscm_value_fetch_lazy_x),
    "\
Fetch the value from the inferior, if it was lazy.\n\
The result is \"unspecified\"." },

  { "value-print", 1, 0, 0, as_a_scm_t_subr (gdbscm_value_print),
    "\
Return the string representation (print form) of the value." },

  { "parse-and-eval", 1, 0, 0, as_a_scm_t_subr (gdbscm_parse_and_eval),
    "\
Evaluates string in gdb and returns the result as a <gdb:value> object." },

  { "history-ref", 1, 0, 0, as_a_scm_t_subr (gdbscm_history_ref),
    "\
Return the specified value from GDB's value history." },

  { "history-append!", 1, 0, 0, as_a_scm_t_subr (gdbscm_history_append_x),
    "\
Append the specified value onto GDB's value history." },

  END_FUNCTIONS
};

void
gdbscm_initialize_values (void)
{
  value_smob_tag = gdbscm_make_smob_type (value_smob_name,
					  sizeof (value_smob));
  scm_set_smob_free (value_smob_tag, vlscm_free_value_smob);
  scm_set_smob_print (value_smob_tag, vlscm_print_value_smob);
  scm_set_smob_equalp (value_smob_tag, vlscm_equal_p_value_smob);

  gdbscm_define_functions (value_functions, 1);

  type_keyword = scm_from_latin1_keyword ("type");
  encoding_keyword = scm_from_latin1_keyword ("encoding");
  errors_keyword = scm_from_latin1_keyword ("errors");
  length_keyword = scm_from_latin1_keyword ("length");

  error_symbol = scm_from_latin1_symbol ("error");
  escape_symbol = scm_from_latin1_symbol ("escape");
  substitute_symbol = scm_from_latin1_symbol ("substitute");
}
