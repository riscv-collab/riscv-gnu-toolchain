/* Perform arithmetic and other operations on values, for GDB.

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
#include "value.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "expression.h"
#include "target.h"
#include "language.h"
#include "target-float.h"
#include "infcall.h"
#include "gdbsupport/byte-vector.h"
#include "gdbarch.h"
#include "rust-lang.h"
#include "ada-lang.h"

/* Forward declarations.  */
static struct value *value_subscripted_rvalue (struct value *array,
					       LONGEST index,
					       LONGEST lowerbound);

/* Given a pointer, return the size of its target.
   If the pointer type is void *, then return 1.
   If the target type is incomplete, then error out.
   This isn't a general purpose function, but just a 
   helper for value_ptradd.  */

static LONGEST
find_size_for_pointer_math (struct type *ptr_type)
{
  LONGEST sz = -1;
  struct type *ptr_target;

  gdb_assert (ptr_type->code () == TYPE_CODE_PTR);
  ptr_target = check_typedef (ptr_type->target_type ());

  sz = type_length_units (ptr_target);
  if (sz == 0)
    {
      if (ptr_type->code () == TYPE_CODE_VOID)
	sz = 1;
      else
	{
	  const char *name;
	  
	  name = ptr_target->name ();
	  if (name == NULL)
	    error (_("Cannot perform pointer math on incomplete types, "
		   "try casting to a known type, or void *."));
	  else
	    error (_("Cannot perform pointer math on incomplete type \"%s\", "
		   "try casting to a known type, or void *."), name);
	}
    }
  return sz;
}

/* Given a pointer ARG1 and an integral value ARG2, return the
   result of C-style pointer arithmetic ARG1 + ARG2.  */

struct value *
value_ptradd (struct value *arg1, LONGEST arg2)
{
  struct type *valptrtype;
  LONGEST sz;
  struct value *result;

  arg1 = coerce_array (arg1);
  valptrtype = check_typedef (arg1->type ());
  sz = find_size_for_pointer_math (valptrtype);

  result = value_from_pointer (valptrtype,
			       value_as_address (arg1) + sz * arg2);
  if (arg1->lval () != lval_internalvar)
    result->set_component_location (arg1);
  return result;
}

/* Given two compatible pointer values ARG1 and ARG2, return the
   result of C-style pointer arithmetic ARG1 - ARG2.  */

LONGEST
value_ptrdiff (struct value *arg1, struct value *arg2)
{
  struct type *type1, *type2;
  LONGEST sz;

  arg1 = coerce_array (arg1);
  arg2 = coerce_array (arg2);
  type1 = check_typedef (arg1->type ());
  type2 = check_typedef (arg2->type ());

  gdb_assert (type1->code () == TYPE_CODE_PTR);
  gdb_assert (type2->code () == TYPE_CODE_PTR);

  if (check_typedef (type1->target_type ())->length ()
      != check_typedef (type2->target_type ())->length ())
    error (_("First argument of `-' is a pointer and "
	     "second argument is neither\n"
	     "an integer nor a pointer of the same type."));

  sz = type_length_units (check_typedef (type1->target_type ()));
  if (sz == 0) 
    {
      warning (_("Type size unknown, assuming 1. "
	       "Try casting to a known type, or void *."));
      sz = 1;
    }

  return (value_as_long (arg1) - value_as_long (arg2)) / sz;
}

/* Return the value of ARRAY[IDX].

   ARRAY may be of type TYPE_CODE_ARRAY or TYPE_CODE_STRING.  If the
   current language supports C-style arrays, it may also be TYPE_CODE_PTR.

   See comments in value_coerce_array() for rationale for reason for
   doing lower bounds adjustment here rather than there.
   FIXME:  Perhaps we should validate that the index is valid and if
   verbosity is set, warn about invalid indices (but still use them).  */

struct value *
value_subscript (struct value *array, LONGEST index)
{
  bool c_style = current_language->c_style_arrays_p ();
  struct type *tarray;

  array = coerce_ref (array);
  tarray = check_typedef (array->type ());

  if (tarray->code () == TYPE_CODE_ARRAY
      || tarray->code () == TYPE_CODE_STRING)
    {
      struct type *range_type = tarray->index_type ();
      std::optional<LONGEST> lowerbound = get_discrete_low_bound (range_type);
      if (!lowerbound.has_value ())
	lowerbound = 0;

      if (array->lval () != lval_memory)
	return value_subscripted_rvalue (array, index, *lowerbound);

      std::optional<LONGEST> upperbound
	= get_discrete_high_bound (range_type);

      if (!upperbound.has_value ())
	upperbound = -1;

      if (index >= *lowerbound && index <= *upperbound)
	return value_subscripted_rvalue (array, index, *lowerbound);

      if (!c_style)
	{
	  /* Emit warning unless we have an array of unknown size.
	     An array of unknown size has lowerbound 0 and upperbound -1.  */
	  if (*upperbound > -1)
	    warning (_("array or string index out of range"));
	  /* fall doing C stuff */
	  c_style = true;
	}

      index -= *lowerbound;

      /* Do not try to dereference a pointer to an unavailable value.
	 Instead mock up a new one and give it the original address.  */
      struct type *elt_type = check_typedef (tarray->target_type ());
      LONGEST elt_size = type_length_units (elt_type);
      if (!array->lazy ()
	  && !array->bytes_available (elt_size * index, elt_size))
	{
	  struct value *val = value::allocate (elt_type);
	  val->mark_bytes_unavailable (0, elt_size);
	  val->set_lval (lval_memory);
	  val->set_address (array->address () + elt_size * index);
	  return val;
	}

      array = value_coerce_array (array);
    }

  if (c_style)
    return value_ind (value_ptradd (array, index));
  else
    error (_("not an array or string"));
}

/* Return the value of EXPR[IDX], expr an aggregate rvalue
   (eg, a vector register).  This routine used to promote floats
   to doubles, but no longer does.  */

static struct value *
value_subscripted_rvalue (struct value *array, LONGEST index,
			  LONGEST lowerbound)
{
  struct type *array_type = check_typedef (array->type ());
  struct type *elt_type = array_type->target_type ();
  LONGEST elt_size = type_length_units (elt_type);

  /* Fetch the bit stride and convert it to a byte stride, assuming 8 bits
     in a byte.  */
  LONGEST stride = array_type->bit_stride ();
  if (stride != 0)
    {
      struct gdbarch *arch = elt_type->arch ();
      int unit_size = gdbarch_addressable_memory_unit_size (arch);
      elt_size = stride / (unit_size * 8);
    }

  LONGEST elt_offs = elt_size * (index - lowerbound);
  bool array_upper_bound_undefined
    = array_type->bounds ()->high.kind () == PROP_UNDEFINED;

  if (index < lowerbound
      || (!array_upper_bound_undefined
	  && elt_offs >= type_length_units (array_type))
      || (array->lval () != lval_memory && array_upper_bound_undefined))
    {
      if (type_not_associated (array_type))
	error (_("no such vector element (vector not associated)"));
      else if (type_not_allocated (array_type))
	error (_("no such vector element (vector not allocated)"));
      else
	error (_("no such vector element"));
    }

  if (is_dynamic_type (elt_type))
    {
      CORE_ADDR address;

      address = array->address () + elt_offs;
      elt_type = resolve_dynamic_type (elt_type, {}, address);
    }

  return value_from_component (array, elt_type, elt_offs);
}

/* See value.h.  */

struct value *
value_to_array (struct value *val)
{
  struct type *type = check_typedef (val->type ());
  if (type->code () == TYPE_CODE_ARRAY)
    return val;

  if (type->is_array_like ())
    {
      const language_defn *defn = language_def (type->language ());
      return defn->to_array (val);
    }
  return nullptr;
}


/* Check to see if either argument is a structure, or a reference to
   one.  This is called so we know whether to go ahead with the normal
   binop or look for a user defined function instead.

   For now, we do not overload the `=' operator.  */

int
binop_types_user_defined_p (enum exp_opcode op,
			    struct type *type1, struct type *type2)
{
  if (op == BINOP_ASSIGN)
    return 0;

  type1 = check_typedef (type1);
  if (TYPE_IS_REFERENCE (type1))
    type1 = check_typedef (type1->target_type ());

  type2 = check_typedef (type2);
  if (TYPE_IS_REFERENCE (type2))
    type2 = check_typedef (type2->target_type ());

  return (type1->code () == TYPE_CODE_STRUCT
	  || type2->code () == TYPE_CODE_STRUCT);
}

/* Check to see if either argument is a structure, or a reference to
   one.  This is called so we know whether to go ahead with the normal
   binop or look for a user defined function instead.

   For now, we do not overload the `=' operator.  */

int
binop_user_defined_p (enum exp_opcode op,
		      struct value *arg1, struct value *arg2)
{
  return binop_types_user_defined_p (op, arg1->type (), arg2->type ());
}

/* Check to see if argument is a structure.  This is called so
   we know whether to go ahead with the normal unop or look for a 
   user defined function instead.

   For now, we do not overload the `&' operator.  */

int
unop_user_defined_p (enum exp_opcode op, struct value *arg1)
{
  struct type *type1;

  if (op == UNOP_ADDR)
    return 0;
  type1 = check_typedef (arg1->type ());
  if (TYPE_IS_REFERENCE (type1))
    type1 = check_typedef (type1->target_type ());
  return type1->code () == TYPE_CODE_STRUCT;
}

/* Try to find an operator named OPERATOR which takes NARGS arguments
   specified in ARGS.  If the operator found is a static member operator
   *STATIC_MEMFUNP will be set to 1, and otherwise 0.
   The search if performed through find_overload_match which will handle
   member operators, non member operators, operators imported implicitly or
   explicitly, and perform correct overload resolution in all of the above
   situations or combinations thereof.  */

static struct value *
value_user_defined_cpp_op (gdb::array_view<value *> args, char *oper,
			   int *static_memfuncp, enum noside noside)
{

  struct symbol *symp = NULL;
  struct value *valp = NULL;

  find_overload_match (args, oper, BOTH /* could be method */,
		       &args[0] /* objp */,
		       NULL /* pass NULL symbol since symbol is unknown */,
		       &valp, &symp, static_memfuncp, 0, noside);

  if (valp)
    return valp;

  if (symp)
    {
      /* This is a non member function and does not
	 expect a reference as its first argument
	 rather the explicit structure.  */
      args[0] = value_ind (args[0]);
      return value_of_variable (symp, 0);
    }

  error (_("Could not find %s."), oper);
}

/* Lookup user defined operator NAME.  Return a value representing the
   function, otherwise return NULL.  */

static struct value *
value_user_defined_op (struct value **argp, gdb::array_view<value *> args,
		       char *name, int *static_memfuncp, enum noside noside)
{
  struct value *result = NULL;

  if (current_language->la_language == language_cplus)
    {
      result = value_user_defined_cpp_op (args, name, static_memfuncp,
					  noside);
    }
  else
    result = value_struct_elt (argp, args, name, static_memfuncp,
			       "structure");

  return result;
}

/* We know either arg1 or arg2 is a structure, so try to find the right
   user defined function.  Create an argument vector that calls 
   arg1.operator @ (arg1,arg2) and return that value (where '@' is any
   binary operator which is legal for GNU C++).

   OP is the operator, and if it is BINOP_ASSIGN_MODIFY, then OTHEROP
   is the opcode saying how to modify it.  Otherwise, OTHEROP is
   unused.  */

struct value *
value_x_binop (struct value *arg1, struct value *arg2, enum exp_opcode op,
	       enum exp_opcode otherop, enum noside noside)
{
  char *ptr;
  char tstr[13];
  int static_memfuncp;

  arg1 = coerce_ref (arg1);
  arg2 = coerce_ref (arg2);

  /* now we know that what we have to do is construct our
     arg vector and find the right function to call it with.  */

  if (check_typedef (arg1->type ())->code () != TYPE_CODE_STRUCT)
    error (_("Can't do that binary op on that type"));	/* FIXME be explicit */

  value *argvec_storage[3];
  gdb::array_view<value *> argvec = argvec_storage;

  argvec[1] = value_addr (arg1);
  argvec[2] = arg2;

  /* Make the right function name up.  */
  strcpy (tstr, "operator__");
  ptr = tstr + 8;
  switch (op)
    {
    case BINOP_ADD:
      strcpy (ptr, "+");
      break;
    case BINOP_SUB:
      strcpy (ptr, "-");
      break;
    case BINOP_MUL:
      strcpy (ptr, "*");
      break;
    case BINOP_DIV:
      strcpy (ptr, "/");
      break;
    case BINOP_REM:
      strcpy (ptr, "%");
      break;
    case BINOP_LSH:
      strcpy (ptr, "<<");
      break;
    case BINOP_RSH:
      strcpy (ptr, ">>");
      break;
    case BINOP_BITWISE_AND:
      strcpy (ptr, "&");
      break;
    case BINOP_BITWISE_IOR:
      strcpy (ptr, "|");
      break;
    case BINOP_BITWISE_XOR:
      strcpy (ptr, "^");
      break;
    case BINOP_LOGICAL_AND:
      strcpy (ptr, "&&");
      break;
    case BINOP_LOGICAL_OR:
      strcpy (ptr, "||");
      break;
    case BINOP_MIN:
      strcpy (ptr, "<?");
      break;
    case BINOP_MAX:
      strcpy (ptr, ">?");
      break;
    case BINOP_ASSIGN:
      strcpy (ptr, "=");
      break;
    case BINOP_ASSIGN_MODIFY:
      switch (otherop)
	{
	case BINOP_ADD:
	  strcpy (ptr, "+=");
	  break;
	case BINOP_SUB:
	  strcpy (ptr, "-=");
	  break;
	case BINOP_MUL:
	  strcpy (ptr, "*=");
	  break;
	case BINOP_DIV:
	  strcpy (ptr, "/=");
	  break;
	case BINOP_REM:
	  strcpy (ptr, "%=");
	  break;
	case BINOP_BITWISE_AND:
	  strcpy (ptr, "&=");
	  break;
	case BINOP_BITWISE_IOR:
	  strcpy (ptr, "|=");
	  break;
	case BINOP_BITWISE_XOR:
	  strcpy (ptr, "^=");
	  break;
	case BINOP_MOD:	/* invalid */
	default:
	  error (_("Invalid binary operation specified."));
	}
      break;
    case BINOP_SUBSCRIPT:
      strcpy (ptr, "[]");
      break;
    case BINOP_EQUAL:
      strcpy (ptr, "==");
      break;
    case BINOP_NOTEQUAL:
      strcpy (ptr, "!=");
      break;
    case BINOP_LESS:
      strcpy (ptr, "<");
      break;
    case BINOP_GTR:
      strcpy (ptr, ">");
      break;
    case BINOP_GEQ:
      strcpy (ptr, ">=");
      break;
    case BINOP_LEQ:
      strcpy (ptr, "<=");
      break;
    case BINOP_MOD:		/* invalid */
    default:
      error (_("Invalid binary operation specified."));
    }

  argvec[0] = value_user_defined_op (&arg1, argvec.slice (1), tstr,
				     &static_memfuncp, noside);

  if (argvec[0])
    {
      if (static_memfuncp)
	{
	  argvec[1] = argvec[0];
	  argvec = argvec.slice (1);
	}
      if (argvec[0]->type ()->code () == TYPE_CODE_XMETHOD)
	{
	  /* Static xmethods are not supported yet.  */
	  gdb_assert (static_memfuncp == 0);
	  if (noside == EVAL_AVOID_SIDE_EFFECTS)
	    {
	      struct type *return_type
		= argvec[0]->result_type_of_xmethod (argvec.slice (1));

	      if (return_type == NULL)
		error (_("Xmethod is missing return type."));
	      return value::zero (return_type, arg1->lval ());
	    }
	  return argvec[0]->call_xmethod (argvec.slice (1));
	}
      if (noside == EVAL_AVOID_SIDE_EFFECTS)
	{
	  struct type *return_type;

	  return_type = check_typedef (argvec[0]->type ())->target_type ();
	  return value::zero (return_type, arg1->lval ());
	}
      return call_function_by_hand (argvec[0], NULL,
				    argvec.slice (1, 2 - static_memfuncp));
    }
  throw_error (NOT_FOUND_ERROR,
	       _("member function %s not found"), tstr);
}

/* We know that arg1 is a structure, so try to find a unary user
   defined operator that matches the operator in question.
   Create an argument vector that calls arg1.operator @ (arg1)
   and return that value (where '@' is (almost) any unary operator which
   is legal for GNU C++).  */

struct value *
value_x_unop (struct value *arg1, enum exp_opcode op, enum noside noside)
{
  struct gdbarch *gdbarch = arg1->type ()->arch ();
  char *ptr;
  char tstr[13], mangle_tstr[13];
  int static_memfuncp, nargs;

  arg1 = coerce_ref (arg1);

  /* now we know that what we have to do is construct our
     arg vector and find the right function to call it with.  */

  if (check_typedef (arg1->type ())->code () != TYPE_CODE_STRUCT)
    error (_("Can't do that unary op on that type"));	/* FIXME be explicit */

  value *argvec_storage[3];
  gdb::array_view<value *> argvec = argvec_storage;

  argvec[1] = value_addr (arg1);
  argvec[2] = 0;

  nargs = 1;

  /* Make the right function name up.  */
  strcpy (tstr, "operator__");
  ptr = tstr + 8;
  strcpy (mangle_tstr, "__");
  switch (op)
    {
    case UNOP_PREINCREMENT:
      strcpy (ptr, "++");
      break;
    case UNOP_PREDECREMENT:
      strcpy (ptr, "--");
      break;
    case UNOP_POSTINCREMENT:
      strcpy (ptr, "++");
      argvec[2] = value_from_longest (builtin_type (gdbarch)->builtin_int, 0);
      nargs ++;
      break;
    case UNOP_POSTDECREMENT:
      strcpy (ptr, "--");
      argvec[2] = value_from_longest (builtin_type (gdbarch)->builtin_int, 0);
      nargs ++;
      break;
    case UNOP_LOGICAL_NOT:
      strcpy (ptr, "!");
      break;
    case UNOP_COMPLEMENT:
      strcpy (ptr, "~");
      break;
    case UNOP_NEG:
      strcpy (ptr, "-");
      break;
    case UNOP_PLUS:
      strcpy (ptr, "+");
      break;
    case UNOP_IND:
      strcpy (ptr, "*");
      break;
    case STRUCTOP_PTR:
      strcpy (ptr, "->");
      break;
    default:
      error (_("Invalid unary operation specified."));
    }

  argvec[0] = value_user_defined_op (&arg1, argvec.slice (1, nargs), tstr,
				     &static_memfuncp, noside);

  if (argvec[0])
    {
      if (static_memfuncp)
	{
	  argvec[1] = argvec[0];
	  argvec = argvec.slice (1);
	}
      if (argvec[0]->type ()->code () == TYPE_CODE_XMETHOD)
	{
	  /* Static xmethods are not supported yet.  */
	  gdb_assert (static_memfuncp == 0);
	  if (noside == EVAL_AVOID_SIDE_EFFECTS)
	    {
	      struct type *return_type
		= argvec[0]->result_type_of_xmethod (argvec[1]);

	      if (return_type == NULL)
		error (_("Xmethod is missing return type."));
	      return value::zero (return_type, arg1->lval ());
	    }
	  return argvec[0]->call_xmethod (argvec[1]);
	}
      if (noside == EVAL_AVOID_SIDE_EFFECTS)
	{
	  struct type *return_type;

	  return_type = check_typedef (argvec[0]->type ())->target_type ();
	  return value::zero (return_type, arg1->lval ());
	}
      return call_function_by_hand (argvec[0], NULL,
				    argvec.slice (1, nargs));
    }
  throw_error (NOT_FOUND_ERROR,
	       _("member function %s not found"), tstr);
}


/* Concatenate two values.  One value must be an array; and the other
   value must either be an array with the same element type, or be of
   the array's element type.  */

struct value *
value_concat (struct value *arg1, struct value *arg2)
{
  struct type *type1 = check_typedef (arg1->type ());
  struct type *type2 = check_typedef (arg2->type ());

  if (type1->code () != TYPE_CODE_ARRAY && type2->code () != TYPE_CODE_ARRAY)
    error ("no array provided to concatenation");

  LONGEST low1, high1;
  struct type *elttype1 = type1;
  if (elttype1->code () == TYPE_CODE_ARRAY)
    {
      elttype1 = elttype1->target_type ();
      if (!get_array_bounds (type1, &low1, &high1))
	error (_("could not determine array bounds on left-hand-side of "
		 "array concatenation"));
    }
  else
    {
      low1 = 0;
      high1 = 0;
    }

  LONGEST low2, high2;
  struct type *elttype2 = type2;
  if (elttype2->code () == TYPE_CODE_ARRAY)
    {
      elttype2 = elttype2->target_type ();
      if (!get_array_bounds (type2, &low2, &high2))
	error (_("could not determine array bounds on right-hand-side of "
		 "array concatenation"));
    }
  else
    {
      low2 = 0;
      high2 = 0;
    }

  if (!types_equal (elttype1, elttype2))
    error (_("concatenation with different element types"));

  LONGEST lowbound = current_language->c_style_arrays_p () ? 0 : 1;
  LONGEST n_elts = (high1 - low1 + 1) + (high2 - low2 + 1);
  struct type *atype = lookup_array_range_type (elttype1,
						lowbound,
						lowbound + n_elts - 1);

  struct value *result = value::allocate (atype);
  gdb::array_view<gdb_byte> contents = result->contents_raw ();
  gdb::array_view<const gdb_byte> lhs_contents = arg1->contents ();
  gdb::array_view<const gdb_byte> rhs_contents = arg2->contents ();
  gdb::copy (lhs_contents, contents.slice (0, lhs_contents.size ()));
  gdb::copy (rhs_contents, contents.slice (lhs_contents.size ()));

  return result;
}


/* Obtain argument values for binary operation, converting from
   other types if one of them is not floating point.  */
static void
value_args_as_target_float (struct value *arg1, struct value *arg2,
			    gdb_byte *x, struct type **eff_type_x,
			    gdb_byte *y, struct type **eff_type_y)
{
  struct type *type1, *type2;

  type1 = check_typedef (arg1->type ());
  type2 = check_typedef (arg2->type ());

  /* At least one of the arguments must be of floating-point type.  */
  gdb_assert (is_floating_type (type1) || is_floating_type (type2));

  if (is_floating_type (type1) && is_floating_type (type2)
      && type1->code () != type2->code ())
    /* The DFP extension to the C language does not allow mixing of
     * decimal float types with other float types in expressions
     * (see WDTR 24732, page 12).  */
    error (_("Mixing decimal floating types with "
	     "other floating types is not allowed."));

  /* Obtain value of arg1, converting from other types if necessary.  */

  if (is_floating_type (type1))
    {
      *eff_type_x = type1;
      memcpy (x, arg1->contents ().data (), type1->length ());
    }
  else if (is_integral_type (type1))
    {
      *eff_type_x = type2;
      if (type1->is_unsigned ())
	target_float_from_ulongest (x, *eff_type_x, value_as_long (arg1));
      else
	target_float_from_longest (x, *eff_type_x, value_as_long (arg1));
    }
  else
    error (_("Don't know how to convert from %s to %s."), type1->name (),
	     type2->name ());

  /* Obtain value of arg2, converting from other types if necessary.  */

  if (is_floating_type (type2))
    {
      *eff_type_y = type2;
      memcpy (y, arg2->contents ().data (), type2->length ());
    }
  else if (is_integral_type (type2))
    {
      *eff_type_y = type1;
      if (type2->is_unsigned ())
	target_float_from_ulongest (y, *eff_type_y, value_as_long (arg2));
      else
	target_float_from_longest (y, *eff_type_y, value_as_long (arg2));
    }
  else
    error (_("Don't know how to convert from %s to %s."), type1->name (),
	     type2->name ());
}

/* Assuming at last one of ARG1 or ARG2 is a fixed point value,
   perform the binary operation OP on these two operands, and return
   the resulting value (also as a fixed point).  */

static struct value *
fixed_point_binop (struct value *arg1, struct value *arg2, enum exp_opcode op)
{
  struct type *type1 = check_typedef (arg1->type ());
  struct type *type2 = check_typedef (arg2->type ());
  const struct language_defn *language = current_language;

  struct gdbarch *gdbarch = type1->arch ();
  struct value *val;

  gdb_mpq v1, v2, res;

  gdb_assert (is_fixed_point_type (type1) || is_fixed_point_type (type2));
  if (op == BINOP_MUL || op == BINOP_DIV)
    {
      v1 = value_to_gdb_mpq (arg1);
      v2 = value_to_gdb_mpq (arg2);

      /* The code below uses TYPE1 for the result type, so make sure
	 it is set properly.  */
      if (!is_fixed_point_type (type1))
	type1 = type2;
    }
  else
    {
      if (!is_fixed_point_type (type1))
	{
	  arg1 = value_cast (type2, arg1);
	  type1 = type2;
	}
      if (!is_fixed_point_type (type2))
	{
	  arg2 = value_cast (type1, arg2);
	  type2 = type1;
	}

      v1.read_fixed_point (arg1->contents (),
			   type_byte_order (type1), type1->is_unsigned (),
			   type1->fixed_point_scaling_factor ());
      v2.read_fixed_point (arg2->contents (),
			   type_byte_order (type2), type2->is_unsigned (),
			   type2->fixed_point_scaling_factor ());
    }

  auto fixed_point_to_value = [type1] (const gdb_mpq &fp)
    {
      value *fp_val = value::allocate (type1);

      fp.write_fixed_point
      (fp_val->contents_raw (),
	 type_byte_order (type1),
	 type1->is_unsigned (),
	 type1->fixed_point_scaling_factor ());

      return fp_val;
    };

  switch (op)
    {
    case BINOP_ADD:
      res = v1 + v2;
      val = fixed_point_to_value (res);
      break;

    case BINOP_SUB:
      res = v1 - v2;
      val = fixed_point_to_value (res);
      break;

    case BINOP_MIN:
      val = fixed_point_to_value (std::min (v1, v2));
      break;

    case BINOP_MAX:
      val = fixed_point_to_value (std::max (v1, v2));
      break;

    case BINOP_MUL:
      res = v1 * v2;
      val = fixed_point_to_value (res);
      break;

    case BINOP_DIV:
      if (v2.sgn () == 0)
	error (_("Division by zero"));
      res = v1 / v2;
      val = fixed_point_to_value (res);
      break;

    case BINOP_EQUAL:
      val = value_from_ulongest (language_bool_type (language, gdbarch),
				 v1 == v2 ? 1 : 0);
      break;

    case BINOP_LESS:
      val = value_from_ulongest (language_bool_type (language, gdbarch),
				 v1 < v2 ? 1 : 0);
      break;

    default:
      error (_("Integer-only operation on fixed point number."));
    }

  return val;
}

/* A helper function that finds the type to use for a binary operation
   involving TYPE1 and TYPE2.  */

static struct type *
promotion_type (struct type *type1, struct type *type2)
{
  struct type *result_type;

  if (is_floating_type (type1) || is_floating_type (type2))
    {
      /* If only one type is floating-point, use its type.
	 Otherwise use the bigger type.  */
      if (!is_floating_type (type1))
	result_type = type2;
      else if (!is_floating_type (type2))
	result_type = type1;
      else if (type2->length () > type1->length ())
	result_type = type2;
      else
	result_type = type1;
    }
  else
    {
      /* Integer types.  */
      if (type1->length () > type2->length ())
	result_type = type1;
      else if (type2->length () > type1->length ())
	result_type = type2;
      else if (type1->is_unsigned ())
	result_type = type1;
      else if (type2->is_unsigned ())
	result_type = type2;
      else
	result_type = type1;
    }

  return result_type;
}

static struct value *scalar_binop (struct value *arg1, struct value *arg2,
				   enum exp_opcode op);

/* Perform a binary operation on complex operands.  */

static struct value *
complex_binop (struct value *arg1, struct value *arg2, enum exp_opcode op)
{
  struct type *arg1_type = check_typedef (arg1->type ());
  struct type *arg2_type = check_typedef (arg2->type ());

  struct value *arg1_real, *arg1_imag, *arg2_real, *arg2_imag;
  if (arg1_type->code () == TYPE_CODE_COMPLEX)
    {
      arg1_real = value_real_part (arg1);
      arg1_imag = value_imaginary_part (arg1);
    }
  else
    {
      arg1_real = arg1;
      arg1_imag = value::zero (arg1_type, not_lval);
    }
  if (arg2_type->code () == TYPE_CODE_COMPLEX)
    {
      arg2_real = value_real_part (arg2);
      arg2_imag = value_imaginary_part (arg2);
    }
  else
    {
      arg2_real = arg2;
      arg2_imag = value::zero (arg2_type, not_lval);
    }

  struct type *comp_type = promotion_type (arg1_real->type (),
					   arg2_real->type ());
  if (!can_create_complex_type (comp_type))
    error (_("Argument to complex arithmetic operation not supported."));

  arg1_real = value_cast (comp_type, arg1_real);
  arg1_imag = value_cast (comp_type, arg1_imag);
  arg2_real = value_cast (comp_type, arg2_real);
  arg2_imag = value_cast (comp_type, arg2_imag);

  struct type *result_type = init_complex_type (nullptr, comp_type);

  struct value *result_real, *result_imag;
  switch (op)
    {
    case BINOP_ADD:
    case BINOP_SUB:
      result_real = scalar_binop (arg1_real, arg2_real, op);
      result_imag = scalar_binop (arg1_imag, arg2_imag, op);
      break;

    case BINOP_MUL:
      {
	struct value *x1 = scalar_binop (arg1_real, arg2_real, op);
	struct value *x2 = scalar_binop (arg1_imag, arg2_imag, op);
	result_real = scalar_binop (x1, x2, BINOP_SUB);

	x1 = scalar_binop (arg1_real, arg2_imag, op);
	x2 = scalar_binop (arg1_imag, arg2_real, op);
	result_imag = scalar_binop (x1, x2, BINOP_ADD);
      }
      break;

    case BINOP_DIV:
      {
	if (arg2_type->code () == TYPE_CODE_COMPLEX)
	  {
	    struct value *conjugate = value_complement (arg2);
	    /* We have to reconstruct ARG1, in case the type was
	       promoted.  */
	    arg1 = value_literal_complex (arg1_real, arg1_imag, result_type);

	    struct value *numerator = scalar_binop (arg1, conjugate,
						    BINOP_MUL);
	    arg1_real = value_real_part (numerator);
	    arg1_imag = value_imaginary_part (numerator);

	    struct value *x1 = scalar_binop (arg2_real, arg2_real, BINOP_MUL);
	    struct value *x2 = scalar_binop (arg2_imag, arg2_imag, BINOP_MUL);
	    arg2_real = scalar_binop (x1, x2, BINOP_ADD);
	  }

	result_real = scalar_binop (arg1_real, arg2_real, op);
	result_imag = scalar_binop (arg1_imag, arg2_real, op);
      }
      break;

    case BINOP_EQUAL:
    case BINOP_NOTEQUAL:
      {
	struct value *x1 = scalar_binop (arg1_real, arg2_real, op);
	struct value *x2 = scalar_binop (arg1_imag, arg2_imag, op);

	LONGEST v1 = value_as_long (x1);
	LONGEST v2 = value_as_long (x2);

	if (op == BINOP_EQUAL)
	  v1 = v1 && v2;
	else
	  v1 = v1 || v2;

	return value_from_longest (x1->type (), v1);
      }
      break;

    default:
      error (_("Invalid binary operation on numbers."));
    }

  return value_literal_complex (result_real, result_imag, result_type);
}

/* Return the type's length in bits.  */

static int
type_length_bits (type *type)
{
  int unit_size = gdbarch_addressable_memory_unit_size (type->arch ());
  return unit_size * 8 * type->length ();
}

/* Check whether the RHS value of a shift is valid in C/C++ semantics.
   SHIFT_COUNT is the shift amount, SHIFT_COUNT_TYPE is the type of
   the shift count value, used to determine whether the type is
   signed, and RESULT_TYPE is the result type.  This is used to avoid
   both negative and too-large shift amounts, which are undefined, and
   would crash a GDB built with UBSan.  Depending on the current
   language, if the shift is not valid, this either warns and returns
   false, or errors out.  Returns true and sets NBITS if valid.  */

static bool
check_valid_shift_count (enum exp_opcode op, type *result_type,
			 type *shift_count_type, const gdb_mpz &shift_count,
			 unsigned long &nbits)
{
  if (!shift_count_type->is_unsigned ())
    {
      LONGEST count = shift_count.as_integer<LONGEST> ();
      if (count < 0)
	{
	  auto error_or_warning = [] (const char *msg)
	  {
	    /* Shifts by a negative amount are always an error in Go.  Other
	       languages are more permissive and their compilers just warn or
	       have modes to disable the errors.  */
	    if (current_language->la_language == language_go)
	      error (("%s"), msg);
	    else
	      warning (("%s"), msg);
	  };

	  if (op == BINOP_RSH)
	    error_or_warning (_("right shift count is negative"));
	  else
	    error_or_warning (_("left shift count is negative"));
	  return false;
	}
    }

  nbits = shift_count.as_integer<unsigned long> ();
  if (nbits >= type_length_bits (result_type))
    {
      /* In Go, shifting by large amounts is defined.  Be silent and
	 still return false, as the caller's error path does the right
	 thing for Go.  */
      if (current_language->la_language != language_go)
	{
	  if (op == BINOP_RSH)
	    warning (_("right shift count >= width of type"));
	  else
	    warning (_("left shift count >= width of type"));
	}
      return false;
    }

  return true;
}

/* Perform a binary operation on two operands which have reasonable
   representations as integers or floats.  This includes booleans,
   characters, integers, or floats.
   Does not support addition and subtraction on pointers;
   use value_ptradd, value_ptrsub or value_ptrdiff for those operations.  */

static struct value *
scalar_binop (struct value *arg1, struct value *arg2, enum exp_opcode op)
{
  struct value *val;
  struct type *type1, *type2, *result_type;

  arg1 = coerce_ref (arg1);
  arg2 = coerce_ref (arg2);

  type1 = check_typedef (arg1->type ());
  type2 = check_typedef (arg2->type ());

  if (type1->code () == TYPE_CODE_COMPLEX
      || type2->code () == TYPE_CODE_COMPLEX)
    return complex_binop (arg1, arg2, op);

  if ((!is_floating_value (arg1)
       && !is_integral_type (type1)
       && !is_fixed_point_type (type1))
      || (!is_floating_value (arg2)
	  && !is_integral_type (type2)
	  && !is_fixed_point_type (type2)))
    error (_("Argument to arithmetic operation not a number or boolean."));

  if (is_fixed_point_type (type1) || is_fixed_point_type (type2))
    return fixed_point_binop (arg1, arg2, op);

  if (is_floating_type (type1) || is_floating_type (type2))
    {
      result_type = promotion_type (type1, type2);
      val = value::allocate (result_type);

      struct type *eff_type_v1, *eff_type_v2;
      gdb::byte_vector v1, v2;
      v1.resize (result_type->length ());
      v2.resize (result_type->length ());

      value_args_as_target_float (arg1, arg2,
				  v1.data (), &eff_type_v1,
				  v2.data (), &eff_type_v2);
      target_float_binop (op, v1.data (), eff_type_v1,
			      v2.data (), eff_type_v2,
			  val->contents_raw ().data (), result_type);
    }
  else if (type1->code () == TYPE_CODE_BOOL
	   || type2->code () == TYPE_CODE_BOOL)
    {
      LONGEST v1, v2, v = 0;

      v1 = value_as_long (arg1);
      v2 = value_as_long (arg2);

      switch (op)
	{
	case BINOP_BITWISE_AND:
	  v = v1 & v2;
	  break;

	case BINOP_BITWISE_IOR:
	  v = v1 | v2;
	  break;

	case BINOP_BITWISE_XOR:
	  v = v1 ^ v2;
	  break;
	      
	case BINOP_EQUAL:
	  v = v1 == v2;
	  break;
	  
	case BINOP_NOTEQUAL:
	  v = v1 != v2;
	  break;

	default:
	  error (_("Invalid operation on booleans."));
	}

      result_type = type1;

      val = value::allocate (result_type);
      store_signed_integer (val->contents_raw ().data (),
			    result_type->length (),
			    type_byte_order (result_type),
			    v);
    }
  else
    /* Integral operations here.  */
    {
      /* Determine type length of the result, and if the operation should
	 be done unsigned.  For exponentiation and shift operators,
	 use the length and type of the left operand.  Otherwise,
	 use the signedness of the operand with the greater length.
	 If both operands are of equal length, use unsigned operation
	 if one of the operands is unsigned.  */
      if (op == BINOP_RSH || op == BINOP_LSH || op == BINOP_EXP)
	result_type = type1;
      else
	result_type = promotion_type (type1, type2);

      gdb_mpz v1 = value_as_mpz (arg1);
      gdb_mpz v2 = value_as_mpz (arg2);
      gdb_mpz v;

      switch (op)
	{
	case BINOP_ADD:
	  v = v1 + v2;
	  break;

	case BINOP_SUB:
	  v = v1 - v2;
	  break;

	case BINOP_MUL:
	  v = v1 * v2;
	  break;

	case BINOP_DIV:
	case BINOP_INTDIV:
	  if (v2.sgn () != 0)
	    v = v1 / v2;
	  else
	    error (_("Division by zero"));
	  break;

	case BINOP_EXP:
	  v = v1.pow (v2.as_integer<unsigned long> ());
	  break;

	case BINOP_REM:
	  if (v2.sgn () != 0)
	    v = v1 % v2;
	  else
	    error (_("Division by zero"));
	  break;

	case BINOP_MOD:
	  /* Knuth 1.2.4, integer only.  Note that unlike the C '%' op,
	     v1 mod 0 has a defined value, v1.  */
	  if (v2.sgn () == 0)
	    {
	      v = v1;
	    }
	  else
	    {
	      v = v1 / v2;
	      /* Note floor(v1/v2) == v1/v2 for unsigned.  */
	      v = v1 - (v2 * v);
	    }
	  break;

	case BINOP_LSH:
	  {
	    unsigned long nbits;
	    if (!check_valid_shift_count (op, result_type, type2, v2, nbits))
	      v = 0;
	    else
	      v = v1 << nbits;
	  }
	  break;

	case BINOP_RSH:
	  {
	    unsigned long nbits;
	    if (!check_valid_shift_count (op, result_type, type2, v2, nbits))
	      v = 0;
	    else
	      v = v1 >> nbits;
	  }
	  break;

	case BINOP_BITWISE_AND:
	  v = v1 & v2;
	  break;

	case BINOP_BITWISE_IOR:
	  v = v1 | v2;
	  break;

	case BINOP_BITWISE_XOR:
	  v = v1 ^ v2;
	  break;

	case BINOP_MIN:
	  v = v1 < v2 ? v1 : v2;
	  break;

	case BINOP_MAX:
	  v = v1 > v2 ? v1 : v2;
	  break;

	case BINOP_EQUAL:
	  v = v1 == v2;
	  break;

	case BINOP_NOTEQUAL:
	  v = v1 != v2;
	  break;

	case BINOP_LESS:
	  v = v1 < v2;
	  break;

	case BINOP_GTR:
	  v = v1 > v2;
	  break;

	case BINOP_LEQ:
	  v = v1 <= v2;
	  break;

	case BINOP_GEQ:
	  v = v1 >= v2;
	  break;

	default:
	  error (_("Invalid binary operation on numbers."));
	}

      val = value_from_mpz (result_type, v);
    }

  return val;
}

/* Widen a scalar value SCALAR_VALUE to vector type VECTOR_TYPE by
   replicating SCALAR_VALUE for each element of the vector.  Only scalar
   types that can be cast to the type of one element of the vector are
   acceptable.  The newly created vector value is returned upon success,
   otherwise an error is thrown.  */

struct value *
value_vector_widen (struct value *scalar_value, struct type *vector_type)
{
  /* Widen the scalar to a vector.  */
  struct type *eltype, *scalar_type;
  struct value *elval;
  LONGEST low_bound, high_bound;
  int i;

  vector_type = check_typedef (vector_type);

  gdb_assert (vector_type->code () == TYPE_CODE_ARRAY
	      && vector_type->is_vector ());

  if (!get_array_bounds (vector_type, &low_bound, &high_bound))
    error (_("Could not determine the vector bounds"));

  eltype = check_typedef (vector_type->target_type ());
  elval = value_cast (eltype, scalar_value);

  scalar_type = check_typedef (scalar_value->type ());

  /* If we reduced the length of the scalar then check we didn't loose any
     important bits.  */
  if (eltype->length () < scalar_type->length ()
      && !value_equal (elval, scalar_value))
    error (_("conversion of scalar to vector involves truncation"));

  value *val = value::allocate (vector_type);
  gdb::array_view<gdb_byte> val_contents = val->contents_writeable ();
  int elt_len = eltype->length ();

  for (i = 0; i < high_bound - low_bound + 1; i++)
    /* Duplicate the contents of elval into the destination vector.  */
    copy (elval->contents_all (),
	  val_contents.slice (i * elt_len, elt_len));

  return val;
}

/* Performs a binary operation on two vector operands by calling scalar_binop
   for each pair of vector components.  */

static struct value *
vector_binop (struct value *val1, struct value *val2, enum exp_opcode op)
{
  struct type *type1, *type2, *eltype1, *eltype2;
  int t1_is_vec, t2_is_vec, elsize, i;
  LONGEST low_bound1, high_bound1, low_bound2, high_bound2;

  type1 = check_typedef (val1->type ());
  type2 = check_typedef (val2->type ());

  t1_is_vec = (type1->code () == TYPE_CODE_ARRAY
	       && type1->is_vector ()) ? 1 : 0;
  t2_is_vec = (type2->code () == TYPE_CODE_ARRAY
	       && type2->is_vector ()) ? 1 : 0;

  if (!t1_is_vec || !t2_is_vec)
    error (_("Vector operations are only supported among vectors"));

  if (!get_array_bounds (type1, &low_bound1, &high_bound1)
      || !get_array_bounds (type2, &low_bound2, &high_bound2))
    error (_("Could not determine the vector bounds"));

  eltype1 = check_typedef (type1->target_type ());
  eltype2 = check_typedef (type2->target_type ());
  elsize = eltype1->length ();

  if (eltype1->code () != eltype2->code ()
      || elsize != eltype2->length ()
      || eltype1->is_unsigned () != eltype2->is_unsigned ()
      || low_bound1 != low_bound2 || high_bound1 != high_bound2)
    error (_("Cannot perform operation on vectors with different types"));

  value *val = value::allocate (type1);
  gdb::array_view<gdb_byte> val_contents = val->contents_writeable ();
  scoped_value_mark mark;
  for (i = 0; i < high_bound1 - low_bound1 + 1; i++)
    {
      value *tmp = value_binop (value_subscript (val1, i),
				value_subscript (val2, i), op);
      copy (tmp->contents_all (),
	    val_contents.slice (i * elsize, elsize));
     }

  return val;
}

/* Perform a binary operation on two operands.  */

struct value *
value_binop (struct value *arg1, struct value *arg2, enum exp_opcode op)
{
  struct value *val;
  struct type *type1 = check_typedef (arg1->type ());
  struct type *type2 = check_typedef (arg2->type ());
  int t1_is_vec = (type1->code () == TYPE_CODE_ARRAY
		   && type1->is_vector ());
  int t2_is_vec = (type2->code () == TYPE_CODE_ARRAY
		   && type2->is_vector ());

  if (!t1_is_vec && !t2_is_vec)
    val = scalar_binop (arg1, arg2, op);
  else if (t1_is_vec && t2_is_vec)
    val = vector_binop (arg1, arg2, op);
  else
    {
      /* Widen the scalar operand to a vector.  */
      struct value **v = t1_is_vec ? &arg2 : &arg1;
      struct type *t = t1_is_vec ? type2 : type1;
      
      if (t->code () != TYPE_CODE_FLT
	  && t->code () != TYPE_CODE_DECFLOAT
	  && !is_integral_type (t))
	error (_("Argument to operation not a number or boolean."));

      /* Replicate the scalar value to make a vector value.  */
      *v = value_vector_widen (*v, t1_is_vec ? type1 : type2);

      val = vector_binop (arg1, arg2, op);
    }

  return val;
}

/* See value.h.  */

bool
value_logical_not (struct value *arg1)
{
  int len;
  const gdb_byte *p;
  struct type *type1;

  arg1 = coerce_array (arg1);
  type1 = check_typedef (arg1->type ());

  if (is_floating_value (arg1))
    return target_float_is_zero (arg1->contents ().data (), type1);

  len = type1->length ();
  p = arg1->contents ().data ();

  while (--len >= 0)
    {
      if (*p++)
	break;
    }

  return len < 0;
}

/* Perform a comparison on two string values (whose content are not
   necessarily null terminated) based on their length.  */

static int
value_strcmp (struct value *arg1, struct value *arg2)
{
  int len1 = arg1->type ()->length ();
  int len2 = arg2->type ()->length ();
  const gdb_byte *s1 = arg1->contents ().data ();
  const gdb_byte *s2 = arg2->contents ().data ();
  int i, len = len1 < len2 ? len1 : len2;

  for (i = 0; i < len; i++)
    {
      if (s1[i] < s2[i])
	return -1;
      else if (s1[i] > s2[i])
	return 1;
      else
	continue;
    }

  if (len1 < len2)
    return -1;
  else if (len1 > len2)
    return 1;
  else
    return 0;
}

/* Simulate the C operator == by returning a 1
   iff ARG1 and ARG2 have equal contents.  */

int
value_equal (struct value *arg1, struct value *arg2)
{
  int len;
  const gdb_byte *p1;
  const gdb_byte *p2;
  struct type *type1, *type2;
  enum type_code code1;
  enum type_code code2;
  int is_int1, is_int2;

  arg1 = coerce_array (arg1);
  arg2 = coerce_array (arg2);

  type1 = check_typedef (arg1->type ());
  type2 = check_typedef (arg2->type ());
  code1 = type1->code ();
  code2 = type2->code ();
  is_int1 = is_integral_type (type1);
  is_int2 = is_integral_type (type2);

  if (is_int1 && is_int2)
    return value_true (value_binop (arg1, arg2, BINOP_EQUAL));
  else if ((is_floating_value (arg1) || is_int1)
	   && (is_floating_value (arg2) || is_int2))
    {
      struct type *eff_type_v1, *eff_type_v2;
      gdb::byte_vector v1, v2;
      v1.resize (std::max (type1->length (), type2->length ()));
      v2.resize (std::max (type1->length (), type2->length ()));

      value_args_as_target_float (arg1, arg2,
				  v1.data (), &eff_type_v1,
				  v2.data (), &eff_type_v2);

      return target_float_compare (v1.data (), eff_type_v1,
				   v2.data (), eff_type_v2) == 0;
    }

  /* FIXME: Need to promote to either CORE_ADDR or LONGEST, whichever
     is bigger.  */
  else if (code1 == TYPE_CODE_PTR && is_int2)
    return value_as_address (arg1) == (CORE_ADDR) value_as_long (arg2);
  else if (code2 == TYPE_CODE_PTR && is_int1)
    return (CORE_ADDR) value_as_long (arg1) == value_as_address (arg2);

  else if (code1 == code2
	   && ((len = (int) type1->length ())
	       == (int) type2->length ()))
    {
      p1 = arg1->contents ().data ();
      p2 = arg2->contents ().data ();
      while (--len >= 0)
	{
	  if (*p1++ != *p2++)
	    break;
	}
      return len < 0;
    }
  else if (code1 == TYPE_CODE_STRING && code2 == TYPE_CODE_STRING)
    {
      return value_strcmp (arg1, arg2) == 0;
    }
  else
    error (_("Invalid type combination in equality test."));
}

/* Compare values based on their raw contents.  Useful for arrays since
   value_equal coerces them to pointers, thus comparing just the address
   of the array instead of its contents.  */

int
value_equal_contents (struct value *arg1, struct value *arg2)
{
  struct type *type1, *type2;

  type1 = check_typedef (arg1->type ());
  type2 = check_typedef (arg2->type ());

  return (type1->code () == type2->code ()
	  && type1->length () == type2->length ()
	  && memcmp (arg1->contents ().data (),
		     arg2->contents ().data (),
		     type1->length ()) == 0);
}

/* Simulate the C operator < by returning 1
   iff ARG1's contents are less than ARG2's.  */

int
value_less (struct value *arg1, struct value *arg2)
{
  enum type_code code1;
  enum type_code code2;
  struct type *type1, *type2;
  int is_int1, is_int2;

  arg1 = coerce_array (arg1);
  arg2 = coerce_array (arg2);

  type1 = check_typedef (arg1->type ());
  type2 = check_typedef (arg2->type ());
  code1 = type1->code ();
  code2 = type2->code ();
  is_int1 = is_integral_type (type1);
  is_int2 = is_integral_type (type2);

  if ((is_int1 && is_int2)
      || (is_fixed_point_type (type1) && is_fixed_point_type (type2)))
    return value_true (value_binop (arg1, arg2, BINOP_LESS));
  else if ((is_floating_value (arg1) || is_int1)
	   && (is_floating_value (arg2) || is_int2))
    {
      struct type *eff_type_v1, *eff_type_v2;
      gdb::byte_vector v1, v2;
      v1.resize (std::max (type1->length (), type2->length ()));
      v2.resize (std::max (type1->length (), type2->length ()));

      value_args_as_target_float (arg1, arg2,
				  v1.data (), &eff_type_v1,
				  v2.data (), &eff_type_v2);

      return target_float_compare (v1.data (), eff_type_v1,
				   v2.data (), eff_type_v2) == -1;
    }
  else if (code1 == TYPE_CODE_PTR && code2 == TYPE_CODE_PTR)
    return value_as_address (arg1) < value_as_address (arg2);

  /* FIXME: Need to promote to either CORE_ADDR or LONGEST, whichever
     is bigger.  */
  else if (code1 == TYPE_CODE_PTR && is_int2)
    return value_as_address (arg1) < (CORE_ADDR) value_as_long (arg2);
  else if (code2 == TYPE_CODE_PTR && is_int1)
    return (CORE_ADDR) value_as_long (arg1) < value_as_address (arg2);
  else if (code1 == TYPE_CODE_STRING && code2 == TYPE_CODE_STRING)
    return value_strcmp (arg1, arg2) < 0;
  else
    {
      error (_("Invalid type combination in ordering comparison."));
      return 0;
    }
}

/* See value.h.  */

struct value *
value_pos (struct value *arg1)
{
  struct type *type;

  arg1 = coerce_ref (arg1);
  type = check_typedef (arg1->type ());

  if (is_integral_type (type) || is_floating_value (arg1)
      || (type->code () == TYPE_CODE_ARRAY && type->is_vector ())
      || type->code () == TYPE_CODE_COMPLEX)
    return value_from_contents (type, arg1->contents ().data ());
  else
    error (_("Argument to positive operation not a number."));
}

/* See value.h.  */

struct value *
value_neg (struct value *arg1)
{
  struct type *type;

  arg1 = coerce_ref (arg1);
  type = check_typedef (arg1->type ());

  if (is_integral_type (type) || is_floating_type (type))
    return value_binop (value_from_longest (type, 0), arg1, BINOP_SUB);
  else if (is_fixed_point_type (type))
    return value_binop (value::zero (type, not_lval), arg1, BINOP_SUB);
  else if (type->code () == TYPE_CODE_ARRAY && type->is_vector ())
    {
      struct value *val = value::allocate (type);
      struct type *eltype = check_typedef (type->target_type ());
      int i;
      LONGEST low_bound, high_bound;

      if (!get_array_bounds (type, &low_bound, &high_bound))
	error (_("Could not determine the vector bounds"));

      gdb::array_view<gdb_byte> val_contents = val->contents_writeable ();
      int elt_len = eltype->length ();

      for (i = 0; i < high_bound - low_bound + 1; i++)
	{
	  value *tmp = value_neg (value_subscript (arg1, i));
	  copy (tmp->contents_all (),
		val_contents.slice (i * elt_len, elt_len));
	}
      return val;
    }
  else if (type->code () == TYPE_CODE_COMPLEX)
    {
      struct value *real = value_real_part (arg1);
      struct value *imag = value_imaginary_part (arg1);

      real = value_neg (real);
      imag = value_neg (imag);
      return value_literal_complex (real, imag, type);
    }
  else
    error (_("Argument to negate operation not a number."));
}

/* See value.h.  */

struct value *
value_complement (struct value *arg1)
{
  struct type *type;
  struct value *val;

  arg1 = coerce_ref (arg1);
  type = check_typedef (arg1->type ());

  if (is_integral_type (type))
    {
      gdb_mpz num = value_as_mpz (arg1);
      num.complement ();
      val = value_from_mpz (type, num);
    }
  else if (type->code () == TYPE_CODE_ARRAY && type->is_vector ())
    {
      struct type *eltype = check_typedef (type->target_type ());
      int i;
      LONGEST low_bound, high_bound;

      if (!get_array_bounds (type, &low_bound, &high_bound))
	error (_("Could not determine the vector bounds"));

      val = value::allocate (type);
      gdb::array_view<gdb_byte> val_contents = val->contents_writeable ();
      int elt_len = eltype->length ();

      for (i = 0; i < high_bound - low_bound + 1; i++)
	{
	  value *tmp = value_complement (value_subscript (arg1, i));
	  copy (tmp->contents_all (),
		val_contents.slice (i * elt_len, elt_len));
	}
    }
  else if (type->code () == TYPE_CODE_COMPLEX)
    {
      /* GCC has an extension that treats ~complex as the complex
	 conjugate.  */
      struct value *real = value_real_part (arg1);
      struct value *imag = value_imaginary_part (arg1);

      imag = value_neg (imag);
      return value_literal_complex (real, imag, type);
    }
  else
    error (_("Argument to complement operation not an integer, boolean."));

  return val;
}

/* The INDEX'th bit of SET value whose value_type is TYPE,
   and whose value_contents is valaddr.
   Return -1 if out of range, -2 other error.  */

int
value_bit_index (struct type *type, const gdb_byte *valaddr, int index)
{
  struct gdbarch *gdbarch = type->arch ();
  LONGEST low_bound, high_bound;
  LONGEST word;
  unsigned rel_index;
  struct type *range = type->index_type ();

  if (!get_discrete_bounds (range, &low_bound, &high_bound))
    return -2;
  if (index < low_bound || index > high_bound)
    return -1;
  rel_index = index - low_bound;
  word = extract_unsigned_integer (valaddr + (rel_index / TARGET_CHAR_BIT), 1,
				   type_byte_order (type));
  rel_index %= TARGET_CHAR_BIT;
  if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
    rel_index = TARGET_CHAR_BIT - 1 - rel_index;
  return (word >> rel_index) & 1;
}
