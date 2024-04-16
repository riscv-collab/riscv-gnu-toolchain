/* Evaluate expressions for GDB.

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
#include "symtab.h"
#include "gdbtypes.h"
#include "value.h"
#include "expression.h"
#include "target.h"
#include "frame.h"
#include "gdbthread.h"
#include "language.h"
#include "cp-abi.h"
#include "infcall.h"
#include "objc-lang.h"
#include "block.h"
#include "parser-defs.h"
#include "cp-support.h"
#include "ui-out.h"
#include "regcache.h"
#include "user-regs.h"
#include "valprint.h"
#include "gdbsupport/gdb_obstack.h"
#include "objfiles.h"
#include "typeprint.h"
#include <ctype.h>
#include "expop.h"
#include "c-exp.h"
#include "inferior.h"


/* Parse the string EXP as a C expression, evaluate it,
   and return the result as a number.  */

CORE_ADDR
parse_and_eval_address (const char *exp)
{
  expression_up expr = parse_expression (exp);

  return value_as_address (expr->evaluate ());
}

/* Like parse_and_eval_address, but treats the value of the expression
   as an integer, not an address, returns a LONGEST, not a CORE_ADDR.  */
LONGEST
parse_and_eval_long (const char *exp)
{
  expression_up expr = parse_expression (exp);

  return value_as_long (expr->evaluate ());
}

struct value *
parse_and_eval (const char *exp, parser_flags flags)
{
  expression_up expr = parse_expression (exp, nullptr, flags);

  return expr->evaluate ();
}

/* Parse up to a comma (or to a closeparen)
   in the string EXPP as an expression, evaluate it, and return the value.
   EXPP is advanced to point to the comma.  */

struct value *
parse_to_comma_and_eval (const char **expp)
{
  expression_up expr = parse_exp_1 (expp, 0, nullptr,
				    PARSER_COMMA_TERMINATES);

  return expr->evaluate ();
}


/* See expression.h.  */

bool
expression::uses_objfile (struct objfile *objfile) const
{
  gdb_assert (objfile->separate_debug_objfile_backlink == nullptr);
  return op->uses_objfile (objfile);
}

/* See expression.h.  */

struct value *
expression::evaluate (struct type *expect_type, enum noside noside)
{
  std::optional<enable_thread_stack_temporaries> stack_temporaries;
  if (target_has_execution () && inferior_ptid != null_ptid
      && language_defn->la_language == language_cplus
      && !thread_stack_temporaries_enabled_p (inferior_thread ()))
    stack_temporaries.emplace (inferior_thread ());

  struct value *retval = op->evaluate (expect_type, this, noside);

  if (stack_temporaries.has_value ()
      && value_in_thread_stack_temporaries (retval, inferior_thread ()))
    retval = retval->non_lval ();

  return retval;
}

/* Find the current value of a watchpoint on EXP.  Return the value in
   *VALP and *RESULTP and the chain of intermediate and final values
   in *VAL_CHAIN.  RESULTP and VAL_CHAIN may be NULL if the caller does
   not need them.

   If PRESERVE_ERRORS is true, then exceptions are passed through.
   Otherwise, if PRESERVE_ERRORS is false, then if a memory error
   occurs while evaluating the expression, *RESULTP will be set to
   NULL.  *RESULTP may be a lazy value, if the result could not be
   read from memory.  It is used to determine whether a value is
   user-specified (we should watch the whole value) or intermediate
   (we should watch only the bit used to locate the final value).

   If the final value, or any intermediate value, could not be read
   from memory, *VALP will be set to NULL.  *VAL_CHAIN will still be
   set to any referenced values.  *VALP will never be a lazy value.
   This is the value which we store in struct breakpoint.

   If VAL_CHAIN is non-NULL, the values put into *VAL_CHAIN will be
   released from the value chain.  If VAL_CHAIN is NULL, all generated
   values will be left on the value chain.  */

void
fetch_subexp_value (struct expression *exp,
		    expr::operation *op,
		    struct value **valp, struct value **resultp,
		    std::vector<value_ref_ptr> *val_chain,
		    bool preserve_errors)
{
  struct value *mark, *new_mark, *result;

  *valp = NULL;
  if (resultp)
    *resultp = NULL;
  if (val_chain)
    val_chain->clear ();

  /* Evaluate the expression.  */
  mark = value_mark ();
  result = NULL;

  try
    {
      result = op->evaluate (nullptr, exp, EVAL_NORMAL);
    }
  catch (const gdb_exception &ex)
    {
      /* Ignore memory errors if we want watchpoints pointing at
	 inaccessible memory to still be created; otherwise, throw the
	 error to some higher catcher.  */
      switch (ex.error)
	{
	case MEMORY_ERROR:
	  if (!preserve_errors)
	    break;
	  [[fallthrough]];
	default:
	  throw;
	  break;
	}
    }

  new_mark = value_mark ();
  if (mark == new_mark)
    return;
  if (resultp)
    *resultp = result;

  /* Make sure it's not lazy, so that after the target stops again we
     have a non-lazy previous value to compare with.  */
  if (result != NULL)
    {
      if (!result->lazy ())
	*valp = result;
      else
	{

	  try
	    {
	      result->fetch_lazy ();
	      *valp = result;
	    }
	  catch (const gdb_exception_error &except)
	    {
	    }
	}
    }

  if (val_chain)
    {
      /* Return the chain of intermediate values.  We use this to
	 decide which addresses to watch.  */
      *val_chain = value_release_to_mark (mark);
    }
}

/* Promote value ARG1 as appropriate before performing a unary operation
   on this argument.
   If the result is not appropriate for any particular language then it
   needs to patch this function.  */

void
unop_promote (const struct language_defn *language, struct gdbarch *gdbarch,
	      struct value **arg1)
{
  struct type *type1;

  *arg1 = coerce_ref (*arg1);
  type1 = check_typedef ((*arg1)->type ());

  if (is_integral_type (type1))
    {
      switch (language->la_language)
	{
	default:
	  /* Perform integral promotion for ANSI C/C++.
	     If not appropriate for any particular language
	     it needs to modify this function.  */
	  {
	    struct type *builtin_int = builtin_type (gdbarch)->builtin_int;

	    if (type1->length () < builtin_int->length ())
	      *arg1 = value_cast (builtin_int, *arg1);
	  }
	  break;
	}
    }
}

/* Promote values ARG1 and ARG2 as appropriate before performing a binary
   operation on those two operands.
   If the result is not appropriate for any particular language then it
   needs to patch this function.  */

void
binop_promote (const struct language_defn *language, struct gdbarch *gdbarch,
	       struct value **arg1, struct value **arg2)
{
  struct type *promoted_type = NULL;
  struct type *type1;
  struct type *type2;

  *arg1 = coerce_ref (*arg1);
  *arg2 = coerce_ref (*arg2);

  type1 = check_typedef ((*arg1)->type ());
  type2 = check_typedef ((*arg2)->type ());

  if ((type1->code () != TYPE_CODE_FLT
       && type1->code () != TYPE_CODE_DECFLOAT
       && !is_integral_type (type1))
      || (type2->code () != TYPE_CODE_FLT
	  && type2->code () != TYPE_CODE_DECFLOAT
	  && !is_integral_type (type2)))
    return;

  if (is_fixed_point_type (type1) || is_fixed_point_type (type2))
    return;

  if (type1->code () == TYPE_CODE_DECFLOAT
      || type2->code () == TYPE_CODE_DECFLOAT)
    {
      /* No promotion required.  */
    }
  else if (type1->code () == TYPE_CODE_FLT
	   || type2->code () == TYPE_CODE_FLT)
    {
      switch (language->la_language)
	{
	case language_c:
	case language_cplus:
	case language_asm:
	case language_objc:
	case language_opencl:
	  /* No promotion required.  */
	  break;

	default:
	  /* For other languages the result type is unchanged from gdb
	     version 6.7 for backward compatibility.
	     If either arg was long double, make sure that value is also long
	     double.  Otherwise use double.  */
	  if (type1->length () * 8 > gdbarch_double_bit (gdbarch)
	      || type2->length () * 8 > gdbarch_double_bit (gdbarch))
	    promoted_type = builtin_type (gdbarch)->builtin_long_double;
	  else
	    promoted_type = builtin_type (gdbarch)->builtin_double;
	  break;
	}
    }
  else if (type1->code () == TYPE_CODE_BOOL
	   && type2->code () == TYPE_CODE_BOOL)
    {
      /* No promotion required.  */
    }
  else
    /* Integral operations here.  */
    /* FIXME: Also mixed integral/booleans, with result an integer.  */
    {
      const struct builtin_type *builtin = builtin_type (gdbarch);
      unsigned int promoted_len1 = type1->length ();
      unsigned int promoted_len2 = type2->length ();
      int is_unsigned1 = type1->is_unsigned ();
      int is_unsigned2 = type2->is_unsigned ();
      unsigned int result_len;
      int unsigned_operation;

      /* Determine type length and signedness after promotion for
	 both operands.  */
      if (promoted_len1 < builtin->builtin_int->length ())
	{
	  is_unsigned1 = 0;
	  promoted_len1 = builtin->builtin_int->length ();
	}
      if (promoted_len2 < builtin->builtin_int->length ())
	{
	  is_unsigned2 = 0;
	  promoted_len2 = builtin->builtin_int->length ();
	}

      if (promoted_len1 > promoted_len2)
	{
	  unsigned_operation = is_unsigned1;
	  result_len = promoted_len1;
	}
      else if (promoted_len2 > promoted_len1)
	{
	  unsigned_operation = is_unsigned2;
	  result_len = promoted_len2;
	}
      else
	{
	  unsigned_operation = is_unsigned1 || is_unsigned2;
	  result_len = promoted_len1;
	}

      switch (language->la_language)
	{
	case language_opencl:
	  if (result_len
	      <= lookup_signed_typename (language, "int")->length())
	    {
	      promoted_type =
		(unsigned_operation
		 ? lookup_unsigned_typename (language, "int")
		 : lookup_signed_typename (language, "int"));
	    }
	  else if (result_len
		   <= lookup_signed_typename (language, "long")->length())
	    {
	      promoted_type =
		(unsigned_operation
		 ? lookup_unsigned_typename (language, "long")
		 : lookup_signed_typename (language,"long"));
	    }
	  break;
	default:
	  if (result_len <= builtin->builtin_int->length ())
	    {
	      promoted_type = (unsigned_operation
			       ? builtin->builtin_unsigned_int
			       : builtin->builtin_int);
	    }
	  else if (result_len <= builtin->builtin_long->length ())
	    {
	      promoted_type = (unsigned_operation
			       ? builtin->builtin_unsigned_long
			       : builtin->builtin_long);
	    }
	  else if (result_len <= builtin->builtin_long_long->length ())
	    {
	      promoted_type = (unsigned_operation
			       ? builtin->builtin_unsigned_long_long
			       : builtin->builtin_long_long);
	    }
	  else
	    {
	      promoted_type = (unsigned_operation
			       ? builtin->builtin_uint128
			       : builtin->builtin_int128);
	    }
	  break;
	}
    }

  if (promoted_type)
    {
      /* Promote both operands to common type.  */
      *arg1 = value_cast (promoted_type, *arg1);
      *arg2 = value_cast (promoted_type, *arg2);
    }
}

static int
ptrmath_type_p (const struct language_defn *lang, struct type *type)
{
  type = check_typedef (type);
  if (TYPE_IS_REFERENCE (type))
    type = type->target_type ();

  switch (type->code ())
    {
    case TYPE_CODE_PTR:
    case TYPE_CODE_FUNC:
      return 1;

    case TYPE_CODE_ARRAY:
      return type->is_vector () ? 0 : lang->c_style_arrays_p ();

    default:
      return 0;
    }
}

/* Represents a fake method with the given parameter types.  This is
   used by the parser to construct a temporary "expected" type for
   method overload resolution.  FLAGS is used as instance flags of the
   new type, in order to be able to make the new type represent a
   const/volatile overload.  */

class fake_method
{
public:
  fake_method (type_instance_flags flags,
	       int num_types, struct type **param_types);
  ~fake_method ();

  /* The constructed type.  */
  struct type *type () { return &m_type; }

private:
  struct type m_type {};
  main_type m_main_type {};
};

fake_method::fake_method (type_instance_flags flags,
			  int num_types, struct type **param_types)
{
  struct type *type = &m_type;

  TYPE_MAIN_TYPE (type) = &m_main_type;
  type->set_length (1);
  type->set_code (TYPE_CODE_METHOD);
  TYPE_CHAIN (type) = type;
  type->set_instance_flags (flags);
  if (num_types > 0)
    {
      if (param_types[num_types - 1] == NULL)
	{
	  --num_types;
	  type->set_has_varargs (true);
	}
      else if (check_typedef (param_types[num_types - 1])->code ()
	       == TYPE_CODE_VOID)
	{
	  --num_types;
	  /* Caller should have ensured this.  */
	  gdb_assert (num_types == 0);
	  type->set_is_prototyped (true);
	}
    }

  /* We don't use TYPE_ZALLOC here to allocate space as TYPE is owned by
     neither an objfile nor a gdbarch.  As a result we must manually
     allocate memory for auxiliary fields, and free the memory ourselves
     when we are done with it.  */
  type->set_num_fields (num_types);
  type->set_fields
    ((struct field *) xzalloc (sizeof (struct field) * num_types));

  while (num_types-- > 0)
    type->field (num_types).set_type (param_types[num_types]);
}

fake_method::~fake_method ()
{
  xfree (m_type.fields ());
}

namespace expr
{

value *
type_instance_operation::evaluate (struct type *expect_type,
				   struct expression *exp,
				   enum noside noside)
{
  type_instance_flags flags = std::get<0> (m_storage);
  std::vector<type *> &types = std::get<1> (m_storage);

  fake_method fake_expect_type (flags, types.size (), types.data ());
  return std::get<2> (m_storage)->evaluate (fake_expect_type.type (),
					    exp, noside);
}

}

/* Helper for evaluating an OP_VAR_VALUE.  */

value *
evaluate_var_value (enum noside noside, const block *blk, symbol *var)
{
  /* JYG: We used to just return value::zero of the symbol type if
     we're asked to avoid side effects.  Otherwise we return
     value_of_variable (...).  However I'm not sure if
     value_of_variable () has any side effect.  We need a full value
     object returned here for whatis_exp () to call evaluate_type ()
     and then pass the full value to value_rtti_target_type () if we
     are dealing with a pointer or reference to a base class and print
     object is on.  */

  struct value *ret = NULL;

  try
    {
      ret = value_of_variable (var, blk);
    }

  catch (const gdb_exception_error &except)
    {
      if (noside != EVAL_AVOID_SIDE_EFFECTS)
	throw;

      ret = value::zero (var->type (), not_lval);
    }

  return ret;
}

namespace expr

{

value *
var_value_operation::evaluate (struct type *expect_type,
			       struct expression *exp,
			       enum noside noside)
{
  symbol *var = std::get<0> (m_storage).symbol;
  if (var->type ()->code () == TYPE_CODE_ERROR)
    error_unknown_type (var->print_name ());
  return evaluate_var_value (noside, std::get<0> (m_storage).block, var);
}

} /* namespace expr */

/* Helper for evaluating an OP_VAR_MSYM_VALUE.  */

value *
evaluate_var_msym_value (enum noside noside,
			 struct objfile *objfile, minimal_symbol *msymbol)
{
  CORE_ADDR address;
  type *the_type = find_minsym_type_and_address (msymbol, objfile, &address);

  if (noside == EVAL_AVOID_SIDE_EFFECTS && !the_type->is_gnu_ifunc ())
    return value::zero (the_type, not_lval);
  else
    return value_at_lazy (the_type, address);
}

/* See expression.h.  */

value *
evaluate_subexp_do_call (expression *exp, enum noside noside,
			 value *callee,
			 gdb::array_view<value *> argvec,
			 const char *function_name,
			 type *default_return_type)
{
  if (callee == NULL)
    error (_("Cannot evaluate function -- may be inlined"));
  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    {
      /* If the return type doesn't look like a function type,
	 call an error.  This can happen if somebody tries to turn
	 a variable into a function call.  */

      type *ftype = callee->type ();

      if (ftype->code () == TYPE_CODE_INTERNAL_FUNCTION)
	{
	  /* We don't know anything about what the internal
	     function might return, but we have to return
	     something.  */
	  return value::zero (builtin_type (exp->gdbarch)->builtin_int,
			     not_lval);
	}
      else if (ftype->code () == TYPE_CODE_XMETHOD)
	{
	  type *return_type = callee->result_type_of_xmethod (argvec);

	  if (return_type == NULL)
	    error (_("Xmethod is missing return type."));
	  return value::zero (return_type, not_lval);
	}
      else if (ftype->code () == TYPE_CODE_FUNC
	       || ftype->code () == TYPE_CODE_METHOD)
	{
	  if (ftype->is_gnu_ifunc ())
	    {
	      CORE_ADDR address = callee->address ();
	      type *resolved_type = find_gnu_ifunc_target_type (address);

	      if (resolved_type != NULL)
		ftype = resolved_type;
	    }

	  type *return_type = ftype->target_type ();

	  if (return_type == NULL)
	    return_type = default_return_type;

	  if (return_type == NULL)
	    error_call_unknown_return_type (function_name);

	  return value::allocate (return_type);
	}
      else
	error (_("Expression of type other than "
		 "\"Function returning ...\" used as function"));
    }
  switch (callee->type ()->code ())
    {
    case TYPE_CODE_INTERNAL_FUNCTION:
      return call_internal_function (exp->gdbarch, exp->language_defn,
				     callee, argvec.size (), argvec.data ());
    case TYPE_CODE_XMETHOD:
      return callee->call_xmethod (argvec);
    default:
      return call_function_by_hand (callee, default_return_type, argvec);
    }
}

namespace expr
{

value *
operation::evaluate_funcall (struct type *expect_type,
			     struct expression *exp,
			     enum noside noside,
			     const char *function_name,
			     const std::vector<operation_up> &args)
{
  std::vector<value *> vals (args.size ());

  value *callee = evaluate_with_coercion (exp, noside);
  struct type *type = callee->type ();
  if (type->code () == TYPE_CODE_PTR)
    type = type->target_type ();
  for (int i = 0; i < args.size (); ++i)
    {
      if (i < type->num_fields ())
	vals[i] = args[i]->evaluate (type->field (i).type (), exp, noside);
      else
	vals[i] = args[i]->evaluate_with_coercion (exp, noside);
    }

  return evaluate_subexp_do_call (exp, noside, callee, vals,
				  function_name, expect_type);
}

value *
var_value_operation::evaluate_funcall (struct type *expect_type,
				       struct expression *exp,
				       enum noside noside,
				       const std::vector<operation_up> &args)
{
  if (!overload_resolution
      || exp->language_defn->la_language != language_cplus)
    return operation::evaluate_funcall (expect_type, exp, noside, args);

  std::vector<value *> argvec (args.size ());
  for (int i = 0; i < args.size (); ++i)
    argvec[i] = args[i]->evaluate_with_coercion (exp, noside);

  struct symbol *symp;
  find_overload_match (argvec, NULL, NON_METHOD,
		       NULL, std::get<0> (m_storage).symbol,
		       NULL, &symp, NULL, 0, noside);

  if (symp->type ()->code () == TYPE_CODE_ERROR)
    error_unknown_type (symp->print_name ());
  value *callee = evaluate_var_value (noside, std::get<0> (m_storage).block,
				      symp);

  return evaluate_subexp_do_call (exp, noside, callee, argvec,
				  nullptr, expect_type);
}

value *
scope_operation::evaluate_funcall (struct type *expect_type,
				   struct expression *exp,
				   enum noside noside,
				   const std::vector<operation_up> &args)
{
  if (!overload_resolution
      || exp->language_defn->la_language != language_cplus)
    return operation::evaluate_funcall (expect_type, exp, noside, args);

  /* Unpack it locally so we can properly handle overload
     resolution.  */
  const std::string &name = std::get<1> (m_storage);
  struct type *type = std::get<0> (m_storage);

  symbol *function = NULL;
  const char *function_name = NULL;
  std::vector<value *> argvec (1 + args.size ());
  if (type->code () == TYPE_CODE_NAMESPACE)
    {
      function = cp_lookup_symbol_namespace (type->name (),
					     name.c_str (),
					     get_selected_block (0),
					     VAR_DOMAIN).symbol;
      if (function == NULL)
	error (_("No symbol \"%s\" in namespace \"%s\"."),
	       name.c_str (), type->name ());
    }
  else
    {
      gdb_assert (type->code () == TYPE_CODE_STRUCT
		  || type->code () == TYPE_CODE_UNION);
      function_name = name.c_str ();

      /* We need a properly typed value for method lookup.  */
      argvec[0] = value::zero (type, lval_memory);
    }

  for (int i = 0; i < args.size (); ++i)
    argvec[i + 1] = args[i]->evaluate_with_coercion (exp, noside);
  gdb::array_view<value *> arg_view = argvec;

  value *callee = nullptr;
  if (function_name != nullptr)
    {
      int static_memfuncp;

      find_overload_match (arg_view, function_name, METHOD,
			   &argvec[0], nullptr, &callee, nullptr,
			   &static_memfuncp, 0, noside);
      if (!static_memfuncp)
	{
	  /* For the time being, we don't handle this.  */
	  error (_("Call to overloaded function %s requires "
		   "`this' pointer"),
		 function_name);
	}

      arg_view = arg_view.slice (1);
    }
  else
    {
      symbol *symp;
      arg_view = arg_view.slice (1);
      find_overload_match (arg_view, nullptr,
			   NON_METHOD, nullptr, function,
			   nullptr, &symp, nullptr, 1, noside);
      callee = value_of_variable (symp, get_selected_block (0));
    }

  return evaluate_subexp_do_call (exp, noside, callee, arg_view,
				  nullptr, expect_type);
}

value *
structop_member_base::evaluate_funcall (struct type *expect_type,
					struct expression *exp,
					enum noside noside,
					const std::vector<operation_up> &args)
{
  /* First, evaluate the structure into lhs.  */
  value *lhs;
  if (opcode () == STRUCTOP_MEMBER)
    lhs = std::get<0> (m_storage)->evaluate_for_address (exp, noside);
  else
    lhs = std::get<0> (m_storage)->evaluate (nullptr, exp, noside);

  std::vector<value *> vals (args.size () + 1);
  gdb::array_view<value *> val_view = vals;
  /* If the function is a virtual function, then the aggregate
     value (providing the structure) plays its part by providing
     the vtable.  Otherwise, it is just along for the ride: call
     the function directly.  */
  value *rhs = std::get<1> (m_storage)->evaluate (nullptr, exp, noside);
  value *callee;

  type *a1_type = check_typedef (rhs->type ());
  if (a1_type->code () == TYPE_CODE_METHODPTR)
    {
      if (noside == EVAL_AVOID_SIDE_EFFECTS)
	callee = value::zero (a1_type->target_type (), not_lval);
      else
	callee = cplus_method_ptr_to_value (&lhs, rhs);

      vals[0] = lhs;
    }
  else if (a1_type->code () == TYPE_CODE_MEMBERPTR)
    {
      struct type *type_ptr
	= lookup_pointer_type (TYPE_SELF_TYPE (a1_type));
      struct type *target_type_ptr
	= lookup_pointer_type (a1_type->target_type ());

      /* Now, convert this value to an address.  */
      lhs = value_cast (type_ptr, lhs);

      long mem_offset = value_as_long (rhs);

      callee = value_from_pointer (target_type_ptr,
				   value_as_long (lhs) + mem_offset);
      callee = value_ind (callee);

      val_view = val_view.slice (1);
    }
  else
    error (_("Non-pointer-to-member value used in pointer-to-member "
	     "construct"));

  for (int i = 0; i < args.size (); ++i)
    vals[i + 1] = args[i]->evaluate_with_coercion (exp, noside);

  return evaluate_subexp_do_call (exp, noside, callee, val_view,
				  nullptr, expect_type);

}

value *
structop_base_operation::evaluate_funcall
     (struct type *expect_type, struct expression *exp, enum noside noside,
      const std::vector<operation_up> &args)
{
  /* Allocate space for the function call arguments, Including space for a
     `this' pointer at the start.  */
  std::vector<value *> vals (args.size () + 1);
  /* First, evaluate the structure into vals[0].  */
  enum exp_opcode op = opcode ();
  if (op == STRUCTOP_STRUCT)
    {
      /* If v is a variable in a register, and the user types
	 v.method (), this will produce an error, because v has no
	 address.

	 A possible way around this would be to allocate a copy of
	 the variable on the stack, copy in the contents, call the
	 function, and copy out the contents.  I.e. convert this
	 from call by reference to call by copy-return (or
	 whatever it's called).  However, this does not work
	 because it is not the same: the method being called could
	 stash a copy of the address, and then future uses through
	 that address (after the method returns) would be expected
	 to use the variable itself, not some copy of it.  */
      vals[0] = std::get<0> (m_storage)->evaluate_for_address (exp, noside);
    }
  else
    {
      vals[0] = std::get<0> (m_storage)->evaluate (nullptr, exp, noside);
      /* Check to see if the operator '->' has been overloaded.
	 If the operator has been overloaded replace vals[0] with the
	 value returned by the custom operator and continue
	 evaluation.  */
      while (unop_user_defined_p (op, vals[0]))
	{
	  struct value *value = nullptr;
	  try
	    {
	      value = value_x_unop (vals[0], op, noside);
	    }
	  catch (const gdb_exception_error &except)
	    {
	      if (except.error == NOT_FOUND_ERROR)
		break;
	      else
		throw;
	    }

	  vals[0] = value;
	}
    }

  /* Evaluate the arguments.  The '+ 1' here is to allow for the `this'
     pointer we placed into vals[0].  */
  for (int i = 0; i < args.size (); ++i)
    vals[i + 1] = args[i]->evaluate_with_coercion (exp, noside);

  /* The array view includes the `this' pointer.  */
  gdb::array_view<value *> arg_view (vals);

  int static_memfuncp;
  value *callee;
  const char *tstr = std::get<1> (m_storage).c_str ();
  if (overload_resolution
      && exp->language_defn->la_language == language_cplus)
    {
      /* Language is C++, do some overload resolution before
	 evaluation.  */
      value *val0 = vals[0];
      find_overload_match (arg_view, tstr, METHOD,
			   &val0, nullptr, &callee, nullptr,
			   &static_memfuncp, 0, noside);
      vals[0] = val0;
    }
  else
    /* Non-C++ case -- or no overload resolution.  */
    {
      struct value *temp = vals[0];

      callee = value_struct_elt (&temp, arg_view, tstr,
				 &static_memfuncp,
				 op == STRUCTOP_STRUCT
				 ? "structure" : "structure pointer");
      /* value_struct_elt updates temp with the correct value of the
	 ``this'' pointer if necessary, so modify it to reflect any
	 ``this'' changes.  */
      vals[0] = value_from_longest (lookup_pointer_type (temp->type ()),
				    temp->address ()
				    + temp->embedded_offset ());
    }

  /* Take out `this' if needed.  */
  if (static_memfuncp)
    arg_view = arg_view.slice (1);

  return evaluate_subexp_do_call (exp, noside, callee, arg_view,
				  nullptr, expect_type);
}

/* Helper for structop_base_operation::complete which recursively adds
   field and method names from TYPE, a struct or union type, to the
   OUTPUT list.  PREFIX is prepended to each result.  */

static void
add_struct_fields (struct type *type, completion_list &output,
		   const char *fieldname, int namelen, const char *prefix)
{
  int i;
  int computed_type_name = 0;
  const char *type_name = NULL;

  type = check_typedef (type);
  for (i = 0; i < type->num_fields (); ++i)
    {
      if (i < TYPE_N_BASECLASSES (type))
	add_struct_fields (TYPE_BASECLASS (type, i),
			   output, fieldname, namelen, prefix);
      else if (type->field (i).name ())
	{
	  if (type->field (i).name ()[0] != '\0')
	    {
	      if (! strncmp (type->field (i).name (),
			     fieldname, namelen))
		output.emplace_back (concat (prefix, type->field (i).name (),
					     nullptr));
	    }
	  else if (type->field (i).type ()->code () == TYPE_CODE_UNION)
	    {
	      /* Recurse into anonymous unions.  */
	      add_struct_fields (type->field (i).type (),
				 output, fieldname, namelen, prefix);
	    }
	}
    }

  for (i = TYPE_NFN_FIELDS (type) - 1; i >= 0; --i)
    {
      const char *name = TYPE_FN_FIELDLIST_NAME (type, i);

      if (name && ! strncmp (name, fieldname, namelen))
	{
	  if (!computed_type_name)
	    {
	      type_name = type->name ();
	      computed_type_name = 1;
	    }
	  /* Omit constructors from the completion list.  */
	  if (!type_name || strcmp (type_name, name))
	    output.emplace_back (concat (prefix, name, nullptr));
	}
    }
}

/* See expop.h.  */

bool
structop_base_operation::complete (struct expression *exp,
				   completion_tracker &tracker,
				   const char *prefix)
{
  const std::string &fieldname = std::get<1> (m_storage);

  value *lhs = std::get<0> (m_storage)->evaluate (nullptr, exp,
						  EVAL_AVOID_SIDE_EFFECTS);
  struct type *type = lhs->type ();
  for (;;)
    {
      type = check_typedef (type);
      if (!type->is_pointer_or_reference ())
	break;
      type = type->target_type ();
    }

  if (type->code () == TYPE_CODE_UNION
      || type->code () == TYPE_CODE_STRUCT)
    {
      completion_list result;

      add_struct_fields (type, result, fieldname.c_str (),
			 fieldname.length (), prefix);
      tracker.add_completions (std::move (result));
      return true;
    }

  return false;
}

} /* namespace expr */

/* Return true if type is integral or reference to integral */

static bool
is_integral_or_integral_reference (struct type *type)
{
  if (is_integral_type (type))
    return true;

  type = check_typedef (type);
  return (type != nullptr
	  && TYPE_IS_REFERENCE (type)
	  && is_integral_type (type->target_type ()));
}

/* Helper function that implements the body of OP_SCOPE.  */

struct value *
eval_op_scope (struct type *expect_type, struct expression *exp,
	       enum noside noside,
	       struct type *type, const char *string)
{
  struct value *arg1 = value_aggregate_elt (type, string, expect_type,
					    0, noside);
  if (arg1 == NULL)
    error (_("There is no field named %s"), string);
  return arg1;
}

/* Helper function that implements the body of OP_VAR_ENTRY_VALUE.  */

struct value *
eval_op_var_entry_value (struct type *expect_type, struct expression *exp,
			 enum noside noside, symbol *sym)
{
  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    return value::zero (sym->type (), not_lval);

  if (SYMBOL_COMPUTED_OPS (sym) == NULL
      || SYMBOL_COMPUTED_OPS (sym)->read_variable_at_entry == NULL)
    error (_("Symbol \"%s\" does not have any specific entry value"),
	   sym->print_name ());

  frame_info_ptr frame = get_selected_frame (NULL);
  return SYMBOL_COMPUTED_OPS (sym)->read_variable_at_entry (sym, frame);
}

/* Helper function that implements the body of OP_VAR_MSYM_VALUE.  */

struct value *
eval_op_var_msym_value (struct type *expect_type, struct expression *exp,
			enum noside noside, bool outermost_p,
			bound_minimal_symbol msymbol)
{
  value *val = evaluate_var_msym_value (noside, msymbol.objfile,
					msymbol.minsym);

  struct type *type = val->type ();
  if (type->code () == TYPE_CODE_ERROR
      && (noside != EVAL_AVOID_SIDE_EFFECTS || !outermost_p))
    error_unknown_type (msymbol.minsym->print_name ());
  return val;
}

/* Helper function that implements the body of OP_FUNC_STATIC_VAR.  */

struct value *
eval_op_func_static_var (struct type *expect_type, struct expression *exp,
			 enum noside noside,
			 value *func, const char *var)
{
  CORE_ADDR addr = func->address ();
  const block *blk = block_for_pc (addr);
  struct block_symbol sym = lookup_symbol (var, blk, VAR_DOMAIN, NULL);
  if (sym.symbol == NULL)
    error (_("No symbol \"%s\" in specified context."), var);
  return evaluate_var_value (noside, sym.block, sym.symbol);
}

/* Helper function that implements the body of OP_REGISTER.  */

struct value *
eval_op_register (struct type *expect_type, struct expression *exp,
		  enum noside noside, const char *name)
{
  int regno;
  struct value *val;

  regno = user_reg_map_name_to_regnum (exp->gdbarch,
				       name, strlen (name));
  if (regno == -1)
    error (_("Register $%s not available."), name);

  /* In EVAL_AVOID_SIDE_EFFECTS mode, we only need to return
     a value with the appropriate register type.  Unfortunately,
     we don't have easy access to the type of user registers.
     So for these registers, we fetch the register value regardless
     of the evaluation mode.  */
  if (noside == EVAL_AVOID_SIDE_EFFECTS
      && regno < gdbarch_num_cooked_regs (exp->gdbarch))
    val = value::zero (register_type (exp->gdbarch, regno), not_lval);
  else
    val = value_of_register
      (regno, get_next_frame_sentinel_okay (get_selected_frame ()));
  if (val == NULL)
    error (_("Value of register %s not available."), name);
  else
    return val;
}

namespace expr
{

value *
string_operation::evaluate (struct type *expect_type,
			    struct expression *exp,
			    enum noside noside)
{
  const std::string &str = std::get<0> (m_storage);
  struct type *type = language_string_char_type (exp->language_defn,
						 exp->gdbarch);
  return value_string (str.c_str (), str.size (), type);
}

struct value *
ternop_slice_operation::evaluate (struct type *expect_type,
				  struct expression *exp,
				  enum noside noside)
{
  struct value *array
    = std::get<0> (m_storage)->evaluate (nullptr, exp, noside);
  struct value *low
    = std::get<1> (m_storage)->evaluate (nullptr, exp, noside);
  struct value *upper
    = std::get<2> (m_storage)->evaluate (nullptr, exp, noside);

  int lowbound = value_as_long (low);
  int upperbound = value_as_long (upper);
  return value_slice (array, lowbound, upperbound - lowbound + 1);
}

} /* namespace expr */

/* Helper function that implements the body of OP_OBJC_SELECTOR.  */

struct value *
eval_op_objc_selector (struct type *expect_type, struct expression *exp,
		       enum noside noside,
		       const char *sel)
{
  struct type *selector_type = builtin_type (exp->gdbarch)->builtin_data_ptr;
  return value_from_longest (selector_type,
			     lookup_child_selector (exp->gdbarch, sel));
}

/* A helper function for STRUCTOP_STRUCT.  */

struct value *
eval_op_structop_struct (struct type *expect_type, struct expression *exp,
			 enum noside noside,
			 struct value *arg1, const char *string)
{
  struct value *arg3 = value_struct_elt (&arg1, {}, string,
					 NULL, "structure");
  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    arg3 = value::zero (arg3->type (), arg3->lval ());
  return arg3;
}

/* A helper function for STRUCTOP_PTR.  */

struct value *
eval_op_structop_ptr (struct type *expect_type, struct expression *exp,
		      enum noside noside,
		      struct value *arg1, const char *string)
{
  /* Check to see if operator '->' has been overloaded.  If so replace
     arg1 with the value returned by evaluating operator->().  */
  while (unop_user_defined_p (STRUCTOP_PTR, arg1))
    {
      struct value *value = NULL;
      try
	{
	  value = value_x_unop (arg1, STRUCTOP_PTR, noside);
	}

      catch (const gdb_exception_error &except)
	{
	  if (except.error == NOT_FOUND_ERROR)
	    break;
	  else
	    throw;
	}

      arg1 = value;
    }

  /* JYG: if print object is on we need to replace the base type
     with rtti type in order to continue on with successful
     lookup of member / method only available in the rtti type.  */
  {
    struct type *arg_type = arg1->type ();
    struct type *real_type;
    int full, using_enc;
    LONGEST top;
    struct value_print_options opts;

    get_user_print_options (&opts);
    if (opts.objectprint && arg_type->target_type ()
	&& (arg_type->target_type ()->code () == TYPE_CODE_STRUCT))
      {
	real_type = value_rtti_indirect_type (arg1, &full, &top,
					      &using_enc);
	if (real_type)
	  arg1 = value_cast (real_type, arg1);
      }
  }

  struct value *arg3 = value_struct_elt (&arg1, {}, string,
					 NULL, "structure pointer");
  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    arg3 = value::zero (arg3->type (), arg3->lval ());
  return arg3;
}

/* A helper function for STRUCTOP_MEMBER.  */

struct value *
eval_op_member (struct type *expect_type, struct expression *exp,
		enum noside noside,
		struct value *arg1, struct value *arg2)
{
  long mem_offset;

  struct value *arg3;
  struct type *type = check_typedef (arg2->type ());
  switch (type->code ())
    {
    case TYPE_CODE_METHODPTR:
      if (noside == EVAL_AVOID_SIDE_EFFECTS)
	return value::zero (type->target_type (), not_lval);
      else
	{
	  arg2 = cplus_method_ptr_to_value (&arg1, arg2);
	  gdb_assert (arg2->type ()->code () == TYPE_CODE_PTR);
	  return value_ind (arg2);
	}

    case TYPE_CODE_MEMBERPTR:
      /* Now, convert these values to an address.  */
      if (check_typedef (arg1->type ())->code () != TYPE_CODE_PTR)
	arg1 = value_addr (arg1);
      arg1 = value_cast_pointers (lookup_pointer_type (TYPE_SELF_TYPE (type)),
				  arg1, 1);

      mem_offset = value_as_long (arg2);

      arg3 = value_from_pointer (lookup_pointer_type (type->target_type ()),
				 value_as_long (arg1) + mem_offset);
      return value_ind (arg3);

    default:
      error (_("non-pointer-to-member value used "
	       "in pointer-to-member construct"));
    }
}

/* A helper function for BINOP_ADD.  */

struct value *
eval_op_add (struct type *expect_type, struct expression *exp,
	     enum noside noside,
	     struct value *arg1, struct value *arg2)
{
  if (binop_user_defined_p (BINOP_ADD, arg1, arg2))
    return value_x_binop (arg1, arg2, BINOP_ADD, OP_NULL, noside);
  else if (ptrmath_type_p (exp->language_defn, arg1->type ())
	   && is_integral_or_integral_reference (arg2->type ()))
    return value_ptradd (arg1, value_as_long (arg2));
  else if (ptrmath_type_p (exp->language_defn, arg2->type ())
	   && is_integral_or_integral_reference (arg1->type ()))
    return value_ptradd (arg2, value_as_long (arg1));
  else
    {
      binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg2);
      return value_binop (arg1, arg2, BINOP_ADD);
    }
}

/* A helper function for BINOP_SUB.  */

struct value *
eval_op_sub (struct type *expect_type, struct expression *exp,
	     enum noside noside,
	     struct value *arg1, struct value *arg2)
{
  if (binop_user_defined_p (BINOP_SUB, arg1, arg2))
    return value_x_binop (arg1, arg2, BINOP_SUB, OP_NULL, noside);
  else if (ptrmath_type_p (exp->language_defn, arg1->type ())
	   && ptrmath_type_p (exp->language_defn, arg2->type ()))
    {
      /* FIXME -- should be ptrdiff_t */
      struct type *type = builtin_type (exp->gdbarch)->builtin_long;
      return value_from_longest (type, value_ptrdiff (arg1, arg2));
    }
  else if (ptrmath_type_p (exp->language_defn, arg1->type ())
	   && is_integral_or_integral_reference (arg2->type ()))
    return value_ptradd (arg1, - value_as_long (arg2));
  else
    {
      binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg2);
      return value_binop (arg1, arg2, BINOP_SUB);
    }
}

/* Helper function for several different binary operations.  */

struct value *
eval_op_binary (struct type *expect_type, struct expression *exp,
		enum noside noside, enum exp_opcode op,
		struct value *arg1, struct value *arg2)
{
  if (binop_user_defined_p (op, arg1, arg2))
    return value_x_binop (arg1, arg2, op, OP_NULL, noside);
  else
    {
      /* If EVAL_AVOID_SIDE_EFFECTS and we're dividing by zero,
	 fudge arg2 to avoid division-by-zero, the caller is
	 (theoretically) only looking for the type of the result.  */
      if (noside == EVAL_AVOID_SIDE_EFFECTS
	  /* ??? Do we really want to test for BINOP_MOD here?
	     The implementation of value_binop gives it a well-defined
	     value.  */
	  && (op == BINOP_DIV
	      || op == BINOP_INTDIV
	      || op == BINOP_REM
	      || op == BINOP_MOD)
	  && value_logical_not (arg2))
	{
	  struct value *v_one;

	  v_one = value_one (arg2->type ());
	  binop_promote (exp->language_defn, exp->gdbarch, &arg1, &v_one);
	  return value_binop (arg1, v_one, op);
	}
      else
	{
	  /* For shift and integer exponentiation operations,
	     only promote the first argument.  */
	  if ((op == BINOP_LSH || op == BINOP_RSH || op == BINOP_EXP)
	      && is_integral_type (arg2->type ()))
	    unop_promote (exp->language_defn, exp->gdbarch, &arg1);
	  else
	    binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg2);

	  return value_binop (arg1, arg2, op);
	}
    }
}

/* A helper function for BINOP_SUBSCRIPT.  */

struct value *
eval_op_subscript (struct type *expect_type, struct expression *exp,
		   enum noside noside, enum exp_opcode op,
		   struct value *arg1, struct value *arg2)
{
  if (binop_user_defined_p (op, arg1, arg2))
    return value_x_binop (arg1, arg2, op, OP_NULL, noside);
  else
    {
      /* If the user attempts to subscript something that is not an
	 array or pointer type (like a plain int variable for example),
	 then report this as an error.  */

      arg1 = coerce_ref (arg1);
      struct type *type = check_typedef (arg1->type ());
      if (type->code () != TYPE_CODE_ARRAY
	  && type->code () != TYPE_CODE_PTR)
	{
	  if (type->name ())
	    error (_("cannot subscript something of type `%s'"),
		   type->name ());
	  else
	    error (_("cannot subscript requested type"));
	}

      if (noside == EVAL_AVOID_SIDE_EFFECTS)
	return value::zero (type->target_type (), arg1->lval ());
      else
	return value_subscript (arg1, value_as_long (arg2));
    }
}

/* A helper function for BINOP_EQUAL.  */

struct value *
eval_op_equal (struct type *expect_type, struct expression *exp,
	       enum noside noside, enum exp_opcode op,
	       struct value *arg1, struct value *arg2)
{
  if (binop_user_defined_p (op, arg1, arg2))
    {
      return value_x_binop (arg1, arg2, op, OP_NULL, noside);
    }
  else
    {
      binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg2);
      int tem = value_equal (arg1, arg2);
      struct type *type = language_bool_type (exp->language_defn,
					      exp->gdbarch);
      return value_from_longest (type, (LONGEST) tem);
    }
}

/* A helper function for BINOP_NOTEQUAL.  */

struct value *
eval_op_notequal (struct type *expect_type, struct expression *exp,
		  enum noside noside, enum exp_opcode op,
		  struct value *arg1, struct value *arg2)
{
  if (binop_user_defined_p (op, arg1, arg2))
    {
      return value_x_binop (arg1, arg2, op, OP_NULL, noside);
    }
  else
    {
      binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg2);
      int tem = value_equal (arg1, arg2);
      struct type *type = language_bool_type (exp->language_defn,
					      exp->gdbarch);
      return value_from_longest (type, (LONGEST) ! tem);
    }
}

/* A helper function for BINOP_LESS.  */

struct value *
eval_op_less (struct type *expect_type, struct expression *exp,
	      enum noside noside, enum exp_opcode op,
	      struct value *arg1, struct value *arg2)
{
  if (binop_user_defined_p (op, arg1, arg2))
    {
      return value_x_binop (arg1, arg2, op, OP_NULL, noside);
    }
  else
    {
      binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg2);
      int tem = value_less (arg1, arg2);
      struct type *type = language_bool_type (exp->language_defn,
					      exp->gdbarch);
      return value_from_longest (type, (LONGEST) tem);
    }
}

/* A helper function for BINOP_GTR.  */

struct value *
eval_op_gtr (struct type *expect_type, struct expression *exp,
	     enum noside noside, enum exp_opcode op,
	     struct value *arg1, struct value *arg2)
{
  if (binop_user_defined_p (op, arg1, arg2))
    {
      return value_x_binop (arg1, arg2, op, OP_NULL, noside);
    }
  else
    {
      binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg2);
      int tem = value_less (arg2, arg1);
      struct type *type = language_bool_type (exp->language_defn,
					      exp->gdbarch);
      return value_from_longest (type, (LONGEST) tem);
    }
}

/* A helper function for BINOP_GEQ.  */

struct value *
eval_op_geq (struct type *expect_type, struct expression *exp,
	     enum noside noside, enum exp_opcode op,
	     struct value *arg1, struct value *arg2)
{
  if (binop_user_defined_p (op, arg1, arg2))
    {
      return value_x_binop (arg1, arg2, op, OP_NULL, noside);
    }
  else
    {
      binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg2);
      int tem = value_less (arg2, arg1) || value_equal (arg1, arg2);
      struct type *type = language_bool_type (exp->language_defn,
					      exp->gdbarch);
      return value_from_longest (type, (LONGEST) tem);
    }
}

/* A helper function for BINOP_LEQ.  */

struct value *
eval_op_leq (struct type *expect_type, struct expression *exp,
	     enum noside noside, enum exp_opcode op,
	     struct value *arg1, struct value *arg2)
{
  if (binop_user_defined_p (op, arg1, arg2))
    {
      return value_x_binop (arg1, arg2, op, OP_NULL, noside);
    }
  else
    {
      binop_promote (exp->language_defn, exp->gdbarch, &arg1, &arg2);
      int tem = value_less (arg1, arg2) || value_equal (arg1, arg2);
      struct type *type = language_bool_type (exp->language_defn,
					      exp->gdbarch);
      return value_from_longest (type, (LONGEST) tem);
    }
}

/* A helper function for BINOP_REPEAT.  */

struct value *
eval_op_repeat (struct type *expect_type, struct expression *exp,
		enum noside noside, enum exp_opcode op,
		struct value *arg1, struct value *arg2)
{
  struct type *type = check_typedef (arg2->type ());
  if (type->code () != TYPE_CODE_INT
      && type->code () != TYPE_CODE_ENUM)
    error (_("Non-integral right operand for \"@\" operator."));
  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    {
      return allocate_repeat_value (arg1->type (),
				    longest_to_int (value_as_long (arg2)));
    }
  else
    return value_repeat (arg1, longest_to_int (value_as_long (arg2)));
}

/* A helper function for UNOP_PLUS.  */

struct value *
eval_op_plus (struct type *expect_type, struct expression *exp,
	      enum noside noside, enum exp_opcode op,
	      struct value *arg1)
{
  if (unop_user_defined_p (op, arg1))
    return value_x_unop (arg1, op, noside);
  else
    {
      unop_promote (exp->language_defn, exp->gdbarch, &arg1);
      return value_pos (arg1);
    }
}

/* A helper function for UNOP_NEG.  */

struct value *
eval_op_neg (struct type *expect_type, struct expression *exp,
	     enum noside noside, enum exp_opcode op,
	     struct value *arg1)
{
  if (unop_user_defined_p (op, arg1))
    return value_x_unop (arg1, op, noside);
  else
    {
      unop_promote (exp->language_defn, exp->gdbarch, &arg1);
      return value_neg (arg1);
    }
}

/* A helper function for UNOP_COMPLEMENT.  */

struct value *
eval_op_complement (struct type *expect_type, struct expression *exp,
		    enum noside noside, enum exp_opcode op,
		    struct value *arg1)
{
  if (unop_user_defined_p (UNOP_COMPLEMENT, arg1))
    return value_x_unop (arg1, UNOP_COMPLEMENT, noside);
  else
    {
      unop_promote (exp->language_defn, exp->gdbarch, &arg1);
      return value_complement (arg1);
    }
}

/* A helper function for UNOP_LOGICAL_NOT.  */

struct value *
eval_op_lognot (struct type *expect_type, struct expression *exp,
		enum noside noside, enum exp_opcode op,
		struct value *arg1)
{
  if (unop_user_defined_p (op, arg1))
    return value_x_unop (arg1, op, noside);
  else
    {
      struct type *type = language_bool_type (exp->language_defn,
					      exp->gdbarch);
      return value_from_longest (type, (LONGEST) value_logical_not (arg1));
    }
}

/* A helper function for UNOP_IND.  */

struct value *
eval_op_ind (struct type *expect_type, struct expression *exp,
	     enum noside noside,
	     struct value *arg1)
{
  struct type *type = check_typedef (arg1->type ());
  if (type->code () == TYPE_CODE_METHODPTR
      || type->code () == TYPE_CODE_MEMBERPTR)
    error (_("Attempt to dereference pointer "
	     "to member without an object"));
  if (unop_user_defined_p (UNOP_IND, arg1))
    return value_x_unop (arg1, UNOP_IND, noside);
  else if (noside == EVAL_AVOID_SIDE_EFFECTS)
    {
      type = check_typedef (arg1->type ());

      /* If the type pointed to is dynamic then in order to resolve the
	 dynamic properties we must actually dereference the pointer.
	 There is a risk that this dereference will have side-effects
	 in the inferior, but being able to print accurate type
	 information seems worth the risk. */
      if (!type->is_pointer_or_reference ()
	  || !is_dynamic_type (type->target_type ()))
	{
	  if (type->is_pointer_or_reference ()
	      /* In C you can dereference an array to get the 1st elt.  */
	      || type->code () == TYPE_CODE_ARRAY)
	    return value::zero (type->target_type (),
			       lval_memory);
	  else if (type->code () == TYPE_CODE_INT)
	    /* GDB allows dereferencing an int.  */
	    return value::zero (builtin_type (exp->gdbarch)->builtin_int,
			       lval_memory);
	  else
	    error (_("Attempt to take contents of a non-pointer value."));
	}
    }

  /* Allow * on an integer so we can cast it to whatever we want.
     This returns an int, which seems like the most C-like thing to
     do.  "long long" variables are rare enough that
     BUILTIN_TYPE_LONGEST would seem to be a mistake.  */
  if (type->code () == TYPE_CODE_INT)
    return value_at_lazy (builtin_type (exp->gdbarch)->builtin_int,
			  (CORE_ADDR) value_as_address (arg1));
  return value_ind (arg1);
}

/* A helper function for UNOP_ALIGNOF.  */

struct value *
eval_op_alignof (struct type *expect_type, struct expression *exp,
		 enum noside noside,
		 struct value *arg1)
{
  struct type *type = arg1->type ();
  /* FIXME: This should be size_t.  */
  struct type *size_type = builtin_type (exp->gdbarch)->builtin_int;
  ULONGEST align = type_align (type);
  if (align == 0)
    error (_("could not determine alignment of type"));
  return value_from_longest (size_type, align);
}

/* A helper function for UNOP_MEMVAL.  */

struct value *
eval_op_memval (struct type *expect_type, struct expression *exp,
		enum noside noside,
		struct value *arg1, struct type *type)
{
  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    return value::zero (type, lval_memory);
  else
    return value_at_lazy (type, value_as_address (arg1));
}

/* A helper function for UNOP_PREINCREMENT.  */

struct value *
eval_op_preinc (struct type *expect_type, struct expression *exp,
		enum noside noside, enum exp_opcode op,
		struct value *arg1)
{
  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    return arg1;
  else if (unop_user_defined_p (op, arg1))
    {
      return value_x_unop (arg1, op, noside);
    }
  else
    {
      struct value *arg2;
      if (ptrmath_type_p (exp->language_defn, arg1->type ()))
	arg2 = value_ptradd (arg1, 1);
      else
	{
	  struct value *tmp = arg1;

	  arg2 = value_one (arg1->type ());
	  binop_promote (exp->language_defn, exp->gdbarch, &tmp, &arg2);
	  arg2 = value_binop (tmp, arg2, BINOP_ADD);
	}

      return value_assign (arg1, arg2);
    }
}

/* A helper function for UNOP_PREDECREMENT.  */

struct value *
eval_op_predec (struct type *expect_type, struct expression *exp,
		enum noside noside, enum exp_opcode op,
		struct value *arg1)
{
  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    return arg1;
  else if (unop_user_defined_p (op, arg1))
    {
      return value_x_unop (arg1, op, noside);
    }
  else
    {
      struct value *arg2;
      if (ptrmath_type_p (exp->language_defn, arg1->type ()))
	arg2 = value_ptradd (arg1, -1);
      else
	{
	  struct value *tmp = arg1;

	  arg2 = value_one (arg1->type ());
	  binop_promote (exp->language_defn, exp->gdbarch, &tmp, &arg2);
	  arg2 = value_binop (tmp, arg2, BINOP_SUB);
	}

      return value_assign (arg1, arg2);
    }
}

/* A helper function for UNOP_POSTINCREMENT.  */

struct value *
eval_op_postinc (struct type *expect_type, struct expression *exp,
		 enum noside noside, enum exp_opcode op,
		 struct value *arg1)
{
  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    return arg1;
  else if (unop_user_defined_p (op, arg1))
    {
      return value_x_unop (arg1, op, noside);
    }
  else
    {
      struct value *arg3 = arg1->non_lval ();
      struct value *arg2;

      if (ptrmath_type_p (exp->language_defn, arg1->type ()))
	arg2 = value_ptradd (arg1, 1);
      else
	{
	  struct value *tmp = arg1;

	  arg2 = value_one (arg1->type ());
	  binop_promote (exp->language_defn, exp->gdbarch, &tmp, &arg2);
	  arg2 = value_binop (tmp, arg2, BINOP_ADD);
	}

      value_assign (arg1, arg2);
      return arg3;
    }
}

/* A helper function for UNOP_POSTDECREMENT.  */

struct value *
eval_op_postdec (struct type *expect_type, struct expression *exp,
		 enum noside noside, enum exp_opcode op,
		 struct value *arg1)
{
  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    return arg1;
  else if (unop_user_defined_p (op, arg1))
    {
      return value_x_unop (arg1, op, noside);
    }
  else
    {
      struct value *arg3 = arg1->non_lval ();
      struct value *arg2;

      if (ptrmath_type_p (exp->language_defn, arg1->type ()))
	arg2 = value_ptradd (arg1, -1);
      else
	{
	  struct value *tmp = arg1;

	  arg2 = value_one (arg1->type ());
	  binop_promote (exp->language_defn, exp->gdbarch, &tmp, &arg2);
	  arg2 = value_binop (tmp, arg2, BINOP_SUB);
	}

      value_assign (arg1, arg2);
      return arg3;
    }
}

/* A helper function for OP_TYPE.  */

struct value *
eval_op_type (struct type *expect_type, struct expression *exp,
	      enum noside noside, struct type *type)
{
  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    return value::allocate (type);
  else
    error (_("Attempt to use a type name as an expression"));
}

/* A helper function for BINOP_ASSIGN_MODIFY.  */

struct value *
eval_binop_assign_modify (struct type *expect_type, struct expression *exp,
			  enum noside noside, enum exp_opcode op,
			  struct value *arg1, struct value *arg2)
{
  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    return arg1;
  if (binop_user_defined_p (op, arg1, arg2))
    return value_x_binop (arg1, arg2, BINOP_ASSIGN_MODIFY, op, noside);
  else if (op == BINOP_ADD && ptrmath_type_p (exp->language_defn,
					      arg1->type ())
	   && is_integral_type (arg2->type ()))
    arg2 = value_ptradd (arg1, value_as_long (arg2));
  else if (op == BINOP_SUB && ptrmath_type_p (exp->language_defn,
					      arg1->type ())
	   && is_integral_type (arg2->type ()))
    arg2 = value_ptradd (arg1, - value_as_long (arg2));
  else
    {
      struct value *tmp = arg1;

      /* For shift and integer exponentiation operations,
	 only promote the first argument.  */
      if ((op == BINOP_LSH || op == BINOP_RSH || op == BINOP_EXP)
	  && is_integral_type (arg2->type ()))
	unop_promote (exp->language_defn, exp->gdbarch, &tmp);
      else
	binop_promote (exp->language_defn, exp->gdbarch, &tmp, &arg2);

      arg2 = value_binop (tmp, arg2, op);
    }
  return value_assign (arg1, arg2);
}

/* Note that ARGS needs 2 empty slots up front and must end with a
   null pointer.  */
static struct value *
eval_op_objc_msgcall (struct type *expect_type, struct expression *exp,
		      enum noside noside, CORE_ADDR selector,
		      value *target, gdb::array_view<value *> args)
{
  CORE_ADDR responds_selector = 0;
  CORE_ADDR method_selector = 0;

  int struct_return = 0;

  struct value *msg_send = NULL;
  struct value *msg_send_stret = NULL;
  int gnu_runtime = 0;

  struct value *method = NULL;
  struct value *called_method = NULL;

  struct type *selector_type = NULL;
  struct type *long_type;
  struct type *type;

  struct value *ret = NULL;
  CORE_ADDR addr = 0;

  value *argvec[5];

  long_type = builtin_type (exp->gdbarch)->builtin_long;
  selector_type = builtin_type (exp->gdbarch)->builtin_data_ptr;

  if (value_as_long (target) == 0)
    return value_from_longest (long_type, 0);

  if (lookup_minimal_symbol ("objc_msg_lookup", 0, 0).minsym)
    gnu_runtime = 1;

  /* Find the method dispatch (Apple runtime) or method lookup
     (GNU runtime) function for Objective-C.  These will be used
     to lookup the symbol information for the method.  If we
     can't find any symbol information, then we'll use these to
     call the method, otherwise we can call the method
     directly.  The msg_send_stret function is used in the special
     case of a method that returns a structure (Apple runtime
     only).  */
  if (gnu_runtime)
    {
      type = selector_type;

      type = lookup_function_type (type);
      type = lookup_pointer_type (type);
      type = lookup_function_type (type);
      type = lookup_pointer_type (type);

      msg_send = find_function_in_inferior ("objc_msg_lookup", NULL);
      msg_send_stret
	= find_function_in_inferior ("objc_msg_lookup", NULL);

      msg_send = value_from_pointer (type, value_as_address (msg_send));
      msg_send_stret = value_from_pointer (type,
					   value_as_address (msg_send_stret));
    }
  else
    {
      msg_send = find_function_in_inferior ("objc_msgSend", NULL);
      /* Special dispatcher for methods returning structs.  */
      msg_send_stret
	= find_function_in_inferior ("objc_msgSend_stret", NULL);
    }

  /* Verify the target object responds to this method.  The
     standard top-level 'Object' class uses a different name for
     the verification method than the non-standard, but more
     often used, 'NSObject' class.  Make sure we check for both.  */

  responds_selector
    = lookup_child_selector (exp->gdbarch, "respondsToSelector:");
  if (responds_selector == 0)
    responds_selector
      = lookup_child_selector (exp->gdbarch, "respondsTo:");

  if (responds_selector == 0)
    error (_("no 'respondsTo:' or 'respondsToSelector:' method"));

  method_selector
    = lookup_child_selector (exp->gdbarch, "methodForSelector:");
  if (method_selector == 0)
    method_selector
      = lookup_child_selector (exp->gdbarch, "methodFor:");

  if (method_selector == 0)
    error (_("no 'methodFor:' or 'methodForSelector:' method"));

  /* Call the verification method, to make sure that the target
     class implements the desired method.  */

  argvec[0] = msg_send;
  argvec[1] = target;
  argvec[2] = value_from_longest (long_type, responds_selector);
  argvec[3] = value_from_longest (long_type, selector);
  argvec[4] = 0;

  ret = call_function_by_hand (argvec[0], NULL, {argvec + 1, 3});
  if (gnu_runtime)
    {
      /* Function objc_msg_lookup returns a pointer.  */
      argvec[0] = ret;
      ret = call_function_by_hand (argvec[0], NULL, {argvec + 1, 3});
    }
  if (value_as_long (ret) == 0)
    error (_("Target does not respond to this message selector."));

  /* Call "methodForSelector:" method, to get the address of a
     function method that implements this selector for this
     class.  If we can find a symbol at that address, then we
     know the return type, parameter types etc.  (that's a good
     thing).  */

  argvec[0] = msg_send;
  argvec[1] = target;
  argvec[2] = value_from_longest (long_type, method_selector);
  argvec[3] = value_from_longest (long_type, selector);
  argvec[4] = 0;

  ret = call_function_by_hand (argvec[0], NULL, {argvec + 1, 3});
  if (gnu_runtime)
    {
      argvec[0] = ret;
      ret = call_function_by_hand (argvec[0], NULL, {argvec + 1, 3});
    }

  /* ret should now be the selector.  */

  addr = value_as_long (ret);
  if (addr)
    {
      struct symbol *sym = NULL;

      /* The address might point to a function descriptor;
	 resolve it to the actual code address instead.  */
      addr = gdbarch_convert_from_func_ptr_addr
	(exp->gdbarch, addr, current_inferior ()->top_target ());

      /* Is it a high_level symbol?  */
      sym = find_pc_function (addr);
      if (sym != NULL)
	method = value_of_variable (sym, 0);
    }

  /* If we found a method with symbol information, check to see
     if it returns a struct.  Otherwise assume it doesn't.  */

  if (method)
    {
      CORE_ADDR funaddr;
      struct type *val_type;

      funaddr = find_function_addr (method, &val_type);

      block_for_pc (funaddr);

      val_type = check_typedef (val_type);

      if ((val_type == NULL)
	  || (val_type->code () == TYPE_CODE_ERROR))
	{
	  if (expect_type != NULL)
	    val_type = expect_type;
	}

      struct_return = using_struct_return (exp->gdbarch, method,
					   val_type);
    }
  else if (expect_type != NULL)
    {
      struct_return = using_struct_return (exp->gdbarch, NULL,
					   check_typedef (expect_type));
    }

  /* Found a function symbol.  Now we will substitute its
     value in place of the message dispatcher (obj_msgSend),
     so that we call the method directly instead of thru
     the dispatcher.  The main reason for doing this is that
     we can now evaluate the return value and parameter values
     according to their known data types, in case we need to
     do things like promotion, dereferencing, special handling
     of structs and doubles, etc.

     We want to use the type signature of 'method', but still
     jump to objc_msgSend() or objc_msgSend_stret() to better
     mimic the behavior of the runtime.  */

  if (method)
    {
      if (method->type ()->code () != TYPE_CODE_FUNC)
	error (_("method address has symbol information "
		 "with non-function type; skipping"));

      /* Create a function pointer of the appropriate type, and
	 replace its value with the value of msg_send or
	 msg_send_stret.  We must use a pointer here, as
	 msg_send and msg_send_stret are of pointer type, and
	 the representation may be different on systems that use
	 function descriptors.  */
      if (struct_return)
	called_method
	  = value_from_pointer (lookup_pointer_type (method->type ()),
				value_as_address (msg_send_stret));
      else
	called_method
	  = value_from_pointer (lookup_pointer_type (method->type ()),
				value_as_address (msg_send));
    }
  else
    {
      if (struct_return)
	called_method = msg_send_stret;
      else
	called_method = msg_send;
    }


  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    {
      /* If the return type doesn't look like a function type,
	 call an error.  This can happen if somebody tries to
	 turn a variable into a function call.  This is here
	 because people often want to call, eg, strcmp, which
	 gdb doesn't know is a function.  If gdb isn't asked for
	 it's opinion (ie. through "whatis"), it won't offer
	 it.  */

      struct type *callee_type = called_method->type ();

      if (callee_type && callee_type->code () == TYPE_CODE_PTR)
	callee_type = callee_type->target_type ();
      callee_type = callee_type->target_type ();

      if (callee_type)
	{
	  if ((callee_type->code () == TYPE_CODE_ERROR) && expect_type)
	    return value::allocate (expect_type);
	  else
	    return value::allocate (callee_type);
	}
      else
	error (_("Expression of type other than "
		 "\"method returning ...\" used as a method"));
    }

  /* Now depending on whether we found a symbol for the method,
     we will either call the runtime dispatcher or the method
     directly.  */

  args[0] = target;
  args[1] = value_from_longest (long_type, selector);

  if (gnu_runtime && (method != NULL))
    {
      /* Function objc_msg_lookup returns a pointer.  */
      struct type *tem_type = called_method->type ();
      tem_type = lookup_pointer_type (lookup_function_type (tem_type));
      called_method->deprecated_set_type (tem_type);
      called_method = call_function_by_hand (called_method, NULL, args);
    }

  return call_function_by_hand (called_method, NULL, args);
}

/* Helper function for MULTI_SUBSCRIPT.  */

static struct value *
eval_multi_subscript (struct type *expect_type, struct expression *exp,
		      enum noside noside, value *arg1,
		      gdb::array_view<value *> args)
{
  for (value *arg2 : args)
    {
      if (binop_user_defined_p (MULTI_SUBSCRIPT, arg1, arg2))
	{
	  arg1 = value_x_binop (arg1, arg2, MULTI_SUBSCRIPT, OP_NULL, noside);
	}
      else
	{
	  arg1 = coerce_ref (arg1);
	  struct type *type = check_typedef (arg1->type ());

	  switch (type->code ())
	    {
	    case TYPE_CODE_PTR:
	    case TYPE_CODE_ARRAY:
	    case TYPE_CODE_STRING:
	      arg1 = value_subscript (arg1, value_as_long (arg2));
	      break;

	    default:
	      if (type->name ())
		error (_("cannot subscript something of type `%s'"),
		       type->name ());
	      else
		error (_("cannot subscript requested type"));
	    }
	}
    }
  return (arg1);
}

namespace expr
{

value *
objc_msgcall_operation::evaluate (struct type *expect_type,
				  struct expression *exp,
				  enum noside noside)
{
  enum noside sub_no_side = EVAL_NORMAL;
  struct type *selector_type = builtin_type (exp->gdbarch)->builtin_data_ptr;

  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    sub_no_side = EVAL_NORMAL;
  else
    sub_no_side = noside;
  value *target
    = std::get<1> (m_storage)->evaluate (selector_type, exp, sub_no_side);

  if (value_as_long (target) == 0)
    sub_no_side = EVAL_AVOID_SIDE_EFFECTS;
  else
    sub_no_side = noside;
  std::vector<operation_up> &args = std::get<2> (m_storage);
  value **argvec = XALLOCAVEC (struct value *, args.size () + 3);
  argvec[0] = nullptr;
  argvec[1] = nullptr;
  for (int i = 0; i < args.size (); ++i)
    argvec[i + 2] = args[i]->evaluate_with_coercion (exp, sub_no_side);
  argvec[args.size () + 2] = nullptr;

  return eval_op_objc_msgcall (expect_type, exp, noside, std::
			       get<0> (m_storage), target,
			       gdb::make_array_view (argvec,
						     args.size () + 3));
}

value *
multi_subscript_operation::evaluate (struct type *expect_type,
				     struct expression *exp,
				     enum noside noside)
{
  value *arg1 = std::get<0> (m_storage)->evaluate_with_coercion (exp, noside);
  std::vector<operation_up> &values = std::get<1> (m_storage);
  value **argvec = XALLOCAVEC (struct value *, values.size ());
  for (int ix = 0; ix < values.size (); ++ix)
    argvec[ix] = values[ix]->evaluate_with_coercion (exp, noside);
  return eval_multi_subscript (expect_type, exp, noside, arg1,
			       gdb::make_array_view (argvec, values.size ()));
}

value *
logical_and_operation::evaluate (struct type *expect_type,
				 struct expression *exp,
				 enum noside noside)
{
  value *arg1 = std::get<0> (m_storage)->evaluate (nullptr, exp, noside);

  value *arg2 = std::get<1> (m_storage)->evaluate (nullptr, exp,
						   EVAL_AVOID_SIDE_EFFECTS);

  if (binop_user_defined_p (BINOP_LOGICAL_AND, arg1, arg2))
    {
      arg2 = std::get<1> (m_storage)->evaluate (nullptr, exp, noside);
      return value_x_binop (arg1, arg2, BINOP_LOGICAL_AND, OP_NULL, noside);
    }
  else
    {
      bool tem = value_logical_not (arg1);
      if (!tem)
	{
	  arg2 = std::get<1> (m_storage)->evaluate (nullptr, exp, noside);
	  tem = value_logical_not (arg2);
	}
      struct type *type = language_bool_type (exp->language_defn,
					      exp->gdbarch);
      return value_from_longest (type, !tem);
    }
}

value *
logical_or_operation::evaluate (struct type *expect_type,
				struct expression *exp,
				enum noside noside)
{
  value *arg1 = std::get<0> (m_storage)->evaluate (nullptr, exp, noside);

  value *arg2 = std::get<1> (m_storage)->evaluate (nullptr, exp,
						   EVAL_AVOID_SIDE_EFFECTS);

  if (binop_user_defined_p (BINOP_LOGICAL_OR, arg1, arg2))
    {
      arg2 = std::get<1> (m_storage)->evaluate (nullptr, exp, noside);
      return value_x_binop (arg1, arg2, BINOP_LOGICAL_OR, OP_NULL, noside);
    }
  else
    {
      bool tem = value_logical_not (arg1);
      if (tem)
	{
	  arg2 = std::get<1> (m_storage)->evaluate (nullptr, exp, noside);
	  tem = value_logical_not (arg2);
	}

      struct type *type = language_bool_type (exp->language_defn,
					      exp->gdbarch);
      return value_from_longest (type, !tem);
    }
}

value *
adl_func_operation::evaluate (struct type *expect_type,
			      struct expression *exp,
			      enum noside noside)
{
  std::vector<operation_up> &arg_ops = std::get<2> (m_storage);
  std::vector<value *> args (arg_ops.size ());
  for (int i = 0; i < arg_ops.size (); ++i)
    args[i] = arg_ops[i]->evaluate_with_coercion (exp, noside);

  struct symbol *symp;
  find_overload_match (args, std::get<0> (m_storage).c_str (),
		       NON_METHOD,
		       nullptr, nullptr,
		       nullptr, &symp, nullptr, 0, noside);
  if (symp->type ()->code () == TYPE_CODE_ERROR)
    error_unknown_type (symp->print_name ());
  value *callee = evaluate_var_value (noside, std::get<1> (m_storage), symp);
  return evaluate_subexp_do_call (exp, noside, callee, args,
				  nullptr, expect_type);

}

/* This function evaluates brace-initializers (in C/C++) for
   structure types.  */

struct value *
array_operation::evaluate_struct_tuple (struct value *struct_val,
					struct expression *exp,
					enum noside noside, int nargs)
{
  const std::vector<operation_up> &in_args = std::get<2> (m_storage);
  struct type *struct_type = check_typedef (struct_val->type ());
  struct type *field_type;
  int fieldno = -1;

  int idx = 0;
  while (--nargs >= 0)
    {
      struct value *val = NULL;
      int bitpos, bitsize;
      bfd_byte *addr;

      fieldno++;
      /* Skip static fields.  */
      while (fieldno < struct_type->num_fields ()
	     && struct_type->field (fieldno).is_static ())
	fieldno++;
      if (fieldno >= struct_type->num_fields ())
	error (_("too many initializers"));
      field_type = struct_type->field (fieldno).type ();
      if (field_type->code () == TYPE_CODE_UNION
	  && struct_type->field (fieldno).name ()[0] == '0')
	error (_("don't know which variant you want to set"));

      /* Here, struct_type is the type of the inner struct,
	 while substruct_type is the type of the inner struct.
	 These are the same for normal structures, but a variant struct
	 contains anonymous union fields that contain substruct fields.
	 The value fieldno is the index of the top-level (normal or
	 anonymous union) field in struct_field, while the value
	 subfieldno is the index of the actual real (named inner) field
	 in substruct_type.  */

      field_type = struct_type->field (fieldno).type ();
      if (val == 0)
	val = in_args[idx++]->evaluate (field_type, exp, noside);

      /* Now actually set the field in struct_val.  */

      /* Assign val to field fieldno.  */
      if (val->type () != field_type)
	val = value_cast (field_type, val);

      bitsize = struct_type->field (fieldno).bitsize ();
      bitpos = struct_type->field (fieldno).loc_bitpos ();
      addr = struct_val->contents_writeable ().data () + bitpos / 8;
      if (bitsize)
	modify_field (struct_type, addr,
		      value_as_long (val), bitpos % 8, bitsize);
      else
	memcpy (addr, val->contents ().data (),
		val->type ()->length ());

    }
  return struct_val;
}

value *
array_operation::evaluate (struct type *expect_type,
			   struct expression *exp,
			   enum noside noside)
{
  const int provided_low_bound = std::get<0> (m_storage);
  const std::vector<operation_up> &in_args = std::get<2> (m_storage);
  const int nargs = std::get<1> (m_storage) - provided_low_bound + 1;
  struct type *type = expect_type ? check_typedef (expect_type) : nullptr;

  if (expect_type != nullptr
      && type->code () == TYPE_CODE_STRUCT)
    {
      struct value *rec = value::allocate (expect_type);

      memset (rec->contents_raw ().data (), '\0', type->length ());
      return evaluate_struct_tuple (rec, exp, noside, nargs);
    }

  if (expect_type != nullptr
      && type->code () == TYPE_CODE_ARRAY)
    {
      struct type *range_type = type->index_type ();
      struct type *element_type = type->target_type ();
      struct value *array = value::allocate (expect_type);
      int element_size = check_typedef (element_type)->length ();
      LONGEST low_bound, high_bound;

      if (!get_discrete_bounds (range_type, &low_bound, &high_bound))
	{
	  low_bound = 0;
	  high_bound = (type->length () / element_size) - 1;
	}
      if (low_bound + nargs - 1 > high_bound)
	error (_("Too many array elements"));
      memset (array->contents_raw ().data (), 0, expect_type->length ());
      for (int idx = 0; idx < nargs; ++idx)
	{
	  struct value *element;

	  element = in_args[idx]->evaluate (element_type, exp, noside);
	  if (element->type () != element_type)
	    element = value_cast (element_type, element);
	  memcpy (array->contents_raw ().data () + idx * element_size,
		  element->contents ().data (),
		  element_size);
	}
      return array;
    }

  if (expect_type != nullptr
      && type->code () == TYPE_CODE_SET)
    {
      struct value *set = value::allocate (expect_type);
      gdb_byte *valaddr = set->contents_raw ().data ();
      struct type *element_type = type->index_type ();
      struct type *check_type = element_type;
      LONGEST low_bound, high_bound;

      /* Get targettype of elementtype.  */
      while (check_type->code () == TYPE_CODE_RANGE
	     || check_type->code () == TYPE_CODE_TYPEDEF)
	check_type = check_type->target_type ();

      if (!get_discrete_bounds (element_type, &low_bound, &high_bound))
	error (_("(power)set type with unknown size"));
      memset (valaddr, '\0', type->length ());
      for (int idx = 0; idx < nargs; idx++)
	{
	  LONGEST range_low, range_high;
	  struct type *range_low_type, *range_high_type;
	  struct value *elem_val;

	  elem_val = in_args[idx]->evaluate (element_type, exp, noside);
	  range_low_type = range_high_type = elem_val->type ();
	  range_low = range_high = value_as_long (elem_val);

	  /* Check types of elements to avoid mixture of elements from
	     different types. Also check if type of element is "compatible"
	     with element type of powerset.  */
	  if (range_low_type->code () == TYPE_CODE_RANGE)
	    range_low_type = range_low_type->target_type ();
	  if (range_high_type->code () == TYPE_CODE_RANGE)
	    range_high_type = range_high_type->target_type ();
	  if ((range_low_type->code () != range_high_type->code ())
	      || (range_low_type->code () == TYPE_CODE_ENUM
		  && (range_low_type != range_high_type)))
	    /* different element modes.  */
	    error (_("POWERSET tuple elements of different mode"));
	  if ((check_type->code () != range_low_type->code ())
	      || (check_type->code () == TYPE_CODE_ENUM
		  && range_low_type != check_type))
	    error (_("incompatible POWERSET tuple elements"));
	  if (range_low > range_high)
	    {
	      warning (_("empty POWERSET tuple range"));
	      continue;
	    }
	  if (range_low < low_bound || range_high > high_bound)
	    error (_("POWERSET tuple element out of range"));
	  range_low -= low_bound;
	  range_high -= low_bound;
	  for (; range_low <= range_high; range_low++)
	    {
	      int bit_index = (unsigned) range_low % TARGET_CHAR_BIT;

	      if (gdbarch_byte_order (exp->gdbarch) == BFD_ENDIAN_BIG)
		bit_index = TARGET_CHAR_BIT - 1 - bit_index;
	      valaddr[(unsigned) range_low / TARGET_CHAR_BIT]
		|= 1 << bit_index;
	    }
	}
      return set;
    }

  std::vector<value *> argvec (nargs);
  for (int tem = 0; tem < nargs; tem++)
    {
      /* Ensure that array expressions are coerced into pointer
	 objects.  */
      argvec[tem] = in_args[tem]->evaluate_with_coercion (exp, noside);
    }
  return value_array (provided_low_bound, argvec);
}

value *
unop_extract_operation::evaluate (struct type *expect_type,
				  struct expression *exp,
				  enum noside noside)
{
  value *old_value = std::get<0> (m_storage)->evaluate (nullptr, exp, noside);
  struct type *type = get_type ();

  if (type->length () > old_value->type ()->length ())
    error (_("length type is larger than the value type"));

  struct value *result = value::allocate (type);
  old_value->contents_copy (result, 0, 0, type->length ());
  return result;
}

}


/* Helper for evaluate_subexp_for_address.  */

static value *
evaluate_subexp_for_address_base (struct expression *exp, enum noside noside,
				  value *x)
{
  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    {
      struct type *type = check_typedef (x->type ());

      if (TYPE_IS_REFERENCE (type))
	return value::zero (lookup_pointer_type (type->target_type ()),
			   not_lval);
      else if (x->lval () == lval_memory || value_must_coerce_to_target (x))
	return value::zero (lookup_pointer_type (x->type ()),
			   not_lval);
      else
	error (_("Attempt to take address of "
		 "value not located in memory."));
    }
  return value_addr (x);
}

namespace expr
{

value *
operation::evaluate_for_cast (struct type *expect_type,
			      struct expression *exp,
			      enum noside noside)
{
  value *val = evaluate (expect_type, exp, noside);
  return value_cast (expect_type, val);
}

value *
operation::evaluate_for_address (struct expression *exp, enum noside noside)
{
  value *val = evaluate (nullptr, exp, noside);
  return evaluate_subexp_for_address_base (exp, noside, val);
}

value *
scope_operation::evaluate_for_address (struct expression *exp,
				       enum noside noside)
{
  value *x = value_aggregate_elt (std::get<0> (m_storage),
				  std::get<1> (m_storage).c_str (),
				  NULL, 1, noside);
  if (x == NULL)
    error (_("There is no field named %s"), std::get<1> (m_storage).c_str ());
  return x;
}

value *
unop_ind_base_operation::evaluate_for_address (struct expression *exp,
					       enum noside noside)
{
  value *x = std::get<0> (m_storage)->evaluate (nullptr, exp, noside);

  /* We can't optimize out "&*" if there's a user-defined operator*.  */
  if (unop_user_defined_p (UNOP_IND, x))
    {
      x = value_x_unop (x, UNOP_IND, noside);
      return evaluate_subexp_for_address_base (exp, noside, x);
    }

  return coerce_array (x);
}

value *
var_msym_value_operation::evaluate_for_address (struct expression *exp,
						enum noside noside)
{
  const bound_minimal_symbol &b = std::get<0> (m_storage);
  value *val = evaluate_var_msym_value (noside, b.objfile, b.minsym);
  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    {
      struct type *type = lookup_pointer_type (val->type ());
      return value::zero (type, not_lval);
    }
  else
    return value_addr (val);
}

value *
unop_memval_operation::evaluate_for_address (struct expression *exp,
					     enum noside noside)
{
  return value_cast (lookup_pointer_type (std::get<1> (m_storage)),
		     std::get<0> (m_storage)->evaluate (nullptr, exp, noside));
}

value *
unop_memval_type_operation::evaluate_for_address (struct expression *exp,
						  enum noside noside)
{
  value *typeval = std::get<0> (m_storage)->evaluate (nullptr, exp,
						      EVAL_AVOID_SIDE_EFFECTS);
  struct type *type = typeval->type ();
  return value_cast (lookup_pointer_type (type),
		     std::get<1> (m_storage)->evaluate (nullptr, exp, noside));
}

value *
var_value_operation::evaluate_for_address (struct expression *exp,
					   enum noside noside)
{
  symbol *var = std::get<0> (m_storage).symbol;

  /* C++: The "address" of a reference should yield the address
   * of the object pointed to.  Let value_addr() deal with it.  */
  if (TYPE_IS_REFERENCE (var->type ()))
    return operation::evaluate_for_address (exp, noside);

  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    {
      struct type *type = lookup_pointer_type (var->type ());
      enum address_class sym_class = var->aclass ();

      if (sym_class == LOC_CONST
	  || sym_class == LOC_CONST_BYTES
	  || sym_class == LOC_REGISTER)
	error (_("Attempt to take address of register or constant."));

      return value::zero (type, not_lval);
    }
  else
    return address_of_variable (var, std::get<0> (m_storage).block);
}

value *
var_value_operation::evaluate_with_coercion (struct expression *exp,
					     enum noside noside)
{
  struct symbol *var = std::get<0> (m_storage).symbol;
  struct type *type = check_typedef (var->type ());
  if (type->code () == TYPE_CODE_ARRAY
      && !type->is_vector ()
      && CAST_IS_CONVERSION (exp->language_defn))
    {
      struct value *val = address_of_variable (var,
					       std::get<0> (m_storage).block);
      return value_cast (lookup_pointer_type (type->target_type ()), val);
    }
  return evaluate (nullptr, exp, noside);
}

}

/* Helper function for evaluating the size of a type.  */

static value *
evaluate_subexp_for_sizeof_base (struct expression *exp, struct type *type)
{
  /* FIXME: This should be size_t.  */
  struct type *size_type = builtin_type (exp->gdbarch)->builtin_int;
  /* $5.3.3/2 of the C++ Standard (n3290 draft) says of sizeof:
     "When applied to a reference or a reference type, the result is
     the size of the referenced type."  */
  type = check_typedef (type);
  if (exp->language_defn->la_language == language_cplus
      && (TYPE_IS_REFERENCE (type)))
    type = check_typedef (type->target_type ());
  return value_from_longest (size_type, (LONGEST) type->length ());
}

namespace expr
{

value *
operation::evaluate_for_sizeof (struct expression *exp, enum noside noside)
{
  value *val = evaluate (nullptr, exp, EVAL_AVOID_SIDE_EFFECTS);
  return evaluate_subexp_for_sizeof_base (exp, val->type ());
}

value *
var_msym_value_operation::evaluate_for_sizeof (struct expression *exp,
					       enum noside noside)

{
  const bound_minimal_symbol &b = std::get<0> (m_storage);
  value *mval = evaluate_var_msym_value (noside, b.objfile, b.minsym);

  struct type *type = mval->type ();
  if (type->code () == TYPE_CODE_ERROR)
    error_unknown_type (b.minsym->print_name ());

  /* FIXME: This should be size_t.  */
  struct type *size_type = builtin_type (exp->gdbarch)->builtin_int;
  return value_from_longest (size_type, type->length ());
}

value *
subscript_operation::evaluate_for_sizeof (struct expression *exp,
					  enum noside noside)
{
  if (noside == EVAL_NORMAL)
    {
      value *val = std::get<0> (m_storage)->evaluate (nullptr, exp,
						      EVAL_AVOID_SIDE_EFFECTS);
      struct type *type = check_typedef (val->type ());
      if (type->code () == TYPE_CODE_ARRAY)
	{
	  type = check_typedef (type->target_type ());
	  if (type->code () == TYPE_CODE_ARRAY)
	    {
	      type = type->index_type ();
	      /* Only re-evaluate the right hand side if the resulting type
		 is a variable length type.  */
	      if (type->bounds ()->flag_bound_evaluated)
		{
		  val = evaluate (nullptr, exp, EVAL_NORMAL);
		  /* FIXME: This should be size_t.  */
		  struct type *size_type
		    = builtin_type (exp->gdbarch)->builtin_int;
		  return value_from_longest
		    (size_type, (LONGEST) val->type ()->length ());
		}
	    }
	}
    }

  return operation::evaluate_for_sizeof (exp, noside);
}

value *
unop_ind_base_operation::evaluate_for_sizeof (struct expression *exp,
					      enum noside noside)
{
  value *val = std::get<0> (m_storage)->evaluate (nullptr, exp,
						  EVAL_AVOID_SIDE_EFFECTS);
  struct type *type = check_typedef (val->type ());
  if (!type->is_pointer_or_reference ()
      && type->code () != TYPE_CODE_ARRAY)
    error (_("Attempt to take contents of a non-pointer value."));
  type = type->target_type ();
  if (is_dynamic_type (type))
    type = value_ind (val)->type ();
  /* FIXME: This should be size_t.  */
  struct type *size_type = builtin_type (exp->gdbarch)->builtin_int;
  return value_from_longest (size_type, (LONGEST) type->length ());
}

value *
unop_memval_operation::evaluate_for_sizeof (struct expression *exp,
					    enum noside noside)
{
  return evaluate_subexp_for_sizeof_base (exp, std::get<1> (m_storage));
}

value *
unop_memval_type_operation::evaluate_for_sizeof (struct expression *exp,
						 enum noside noside)
{
  value *typeval = std::get<0> (m_storage)->evaluate (nullptr, exp,
						      EVAL_AVOID_SIDE_EFFECTS);
  return evaluate_subexp_for_sizeof_base (exp, typeval->type ());
}

value *
var_value_operation::evaluate_for_sizeof (struct expression *exp,
					  enum noside noside)
{
  struct type *type = std::get<0> (m_storage).symbol->type ();
  if (is_dynamic_type (type))
    {
      value *val = evaluate (nullptr, exp, EVAL_NORMAL);
      type = val->type ();
      if (type->code () == TYPE_CODE_ARRAY)
	{
	  /* FIXME: This should be size_t.  */
	  struct type *size_type = builtin_type (exp->gdbarch)->builtin_int;
	  if (type_not_allocated (type) || type_not_associated (type))
	    return value::zero (size_type, not_lval);
	  else if (is_dynamic_type (type->index_type ())
		   && type->bounds ()->high.kind () == PROP_UNDEFINED)
	    return value::allocate_optimized_out (size_type);
	}
    }
  return evaluate_subexp_for_sizeof_base (exp, type);
}

value *
var_msym_value_operation::evaluate_for_cast (struct type *to_type,
					     struct expression *exp,
					     enum noside noside)
{
  if (noside == EVAL_AVOID_SIDE_EFFECTS)
    return value::zero (to_type, not_lval);

  const bound_minimal_symbol &b = std::get<0> (m_storage);
  value *val = evaluate_var_msym_value (noside, b.objfile, b.minsym);

  val = value_cast (to_type, val);

  /* Don't allow e.g. '&(int)var_with_no_debug_info'.  */
  if (val->lval () == lval_memory)
    {
      if (val->lazy ())
	val->fetch_lazy ();
      val->set_lval (not_lval);
    }
  return val;
}

value *
var_value_operation::evaluate_for_cast (struct type *to_type,
					struct expression *exp,
					enum noside noside)
{
  value *val = evaluate_var_value (noside,
				   std::get<0> (m_storage).block,
				   std::get<0> (m_storage).symbol);

  val = value_cast (to_type, val);

  /* Don't allow e.g. '&(int)var_with_no_debug_info'.  */
  if (val->lval () == lval_memory)
    {
      if (val->lazy ())
	val->fetch_lazy ();
      val->set_lval (not_lval);
    }
  return val;
}

}

/* Parse a type expression in the string [P..P+LENGTH).  */

struct type *
parse_and_eval_type (const char *p, int length)
{
  char *tmp = (char *) alloca (length + 4);

  tmp[0] = '(';
  memcpy (tmp + 1, p, length);
  tmp[length + 1] = ')';
  tmp[length + 2] = '0';
  tmp[length + 3] = '\0';
  expression_up expr = parse_expression (tmp);
  expr::unop_cast_operation *op
    = dynamic_cast<expr::unop_cast_operation *> (expr->op.get ());
  if (op == nullptr)
    error (_("Internal error in eval_type."));
  return op->get_type ();
}
