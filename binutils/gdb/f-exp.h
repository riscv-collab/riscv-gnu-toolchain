/* Definitions for Fortran expressions

   Copyright (C) 2020-2024 Free Software Foundation, Inc.

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

#ifndef FORTRAN_EXP_H
#define FORTRAN_EXP_H

#include "expop.h"

extern struct value *eval_op_f_abs (struct type *expect_type,
				    struct expression *exp,
				    enum noside noside,
				    enum exp_opcode opcode,
				    struct value *arg1);
extern struct value *eval_op_f_mod (struct type *expect_type,
				    struct expression *exp,
				    enum noside noside,
				    enum exp_opcode opcode,
				    struct value *arg1, struct value *arg2);

/* Implement expression evaluation for Fortran's CEILING intrinsic function
   called with one argument.  For EXPECT_TYPE, EXP, and NOSIDE see
   expression::evaluate (in expression.h).  OPCODE will always be
   FORTRAN_CEILING and ARG1 is the argument passed to CEILING.  */

extern struct value *eval_op_f_ceil (struct type *expect_type,
				     struct expression *exp,
				     enum noside noside,
				     enum exp_opcode opcode,
				     struct value *arg1);

/* Implement expression evaluation for Fortran's CEILING intrinsic function
   called with two arguments.  For EXPECT_TYPE, EXP, and NOSIDE see
   expression::evaluate (in expression.h).  OPCODE will always be
   FORTRAN_CEILING, ARG1 is the first argument passed to CEILING, and KIND_ARG
   is the type corresponding to the KIND parameter passed to CEILING.  */

extern value *eval_op_f_ceil (type *expect_type, expression *exp,
			      noside noside, exp_opcode opcode, value *arg1,
			      type *kind_arg);

/* Implement expression evaluation for Fortran's FLOOR intrinsic function
   called with one argument.  For EXPECT_TYPE, EXP, and NOSIDE see
   expression::evaluate (in expression.h).  OPCODE will always be FORTRAN_FLOOR
   and ARG1 is the argument passed to FLOOR.  */

extern struct value *eval_op_f_floor (struct type *expect_type,
				      struct expression *exp,
				      enum noside noside,
				      enum exp_opcode opcode,
				      struct value *arg1);

/* Implement expression evaluation for Fortran's FLOOR intrinsic function
   called with two arguments.  For EXPECT_TYPE, EXP, and NOSIDE see
   expression::evaluate (in expression.h).  OPCODE will always be
   FORTRAN_FLOOR, ARG1 is the first argument passed to FLOOR, and KIND_ARG is
   the type corresponding to the KIND parameter passed to FLOOR.  */

extern value *eval_op_f_floor (type *expect_type, expression *exp,
			       noside noside, exp_opcode opcode, value *arg1,
			       type *kind_arg);

extern struct value *eval_op_f_modulo (struct type *expect_type,
				       struct expression *exp,
				       enum noside noside,
				       enum exp_opcode opcode,
				       struct value *arg1, struct value *arg2);

/* Implement expression evaluation for Fortran's CMPLX intrinsic function
   called with one argument.  For EXPECT_TYPE, EXP, and NOSIDE see
   expression::evaluate (in expression.h). OPCODE will always be
   FORTRAN_CMPLX and ARG1 is the argument passed to CMPLX if.  */

extern value *eval_op_f_cmplx (type *expect_type, expression *exp,
			       noside noside, exp_opcode opcode, value *arg1);

/* Implement expression evaluation for Fortran's CMPLX intrinsic function
   called with two arguments.  For EXPECT_TYPE, EXP, and NOSIDE see
   expression::evaluate (in expression.h).  OPCODE will always be
   FORTRAN_CMPLX, ARG1 and ARG2 are the arguments passed to CMPLX.  */

extern struct value *eval_op_f_cmplx (struct type *expect_type,
				      struct expression *exp,
				      enum noside noside,
				      enum exp_opcode opcode,
				      struct value *arg1, struct value *arg2);

/* Implement expression evaluation for Fortran's CMPLX intrinsic function
   called with three arguments.  For EXPECT_TYPE, EXP, and NOSIDE see
   expression::evaluate (in expression.h).  OPCODE will always be
   FORTRAN_CMPLX, ARG1 and ARG2 are real and imaginary part passed to CMPLX,
   and KIND_ARG is the type corresponding to the KIND parameter passed to
   CMPLX.  */

extern value *eval_op_f_cmplx (type *expect_type, expression *exp,
			       noside noside, exp_opcode opcode, value *arg1,
			       value *arg2, type *kind_arg);

extern struct value *eval_op_f_kind (struct type *expect_type,
				     struct expression *exp,
				     enum noside noside,
				     enum exp_opcode opcode,
				     struct value *arg1);
extern struct value *eval_op_f_associated (struct type *expect_type,
					   struct expression *exp,
					   enum noside noside,
					   enum exp_opcode opcode,
					   struct value *arg1);
extern struct value *eval_op_f_associated (struct type *expect_type,
					   struct expression *exp,
					   enum noside noside,
					   enum exp_opcode opcode,
					   struct value *arg1,
					   struct value *arg2);
extern struct value * eval_op_f_allocated (struct type *expect_type,
					   struct expression *exp,
					   enum noside noside,
					   enum exp_opcode op,
					   struct value *arg1);
extern struct value * eval_op_f_loc (struct type *expect_type,
				     struct expression *exp,
				     enum noside noside,
				     enum exp_opcode op,
				     struct value *arg1);

/* Implement the evaluation of UNOP_FORTRAN_RANK.  EXPECTED_TYPE, EXP, and
   NOSIDE are as for expression::evaluate (see expression.h).  OP will
   always be UNOP_FORTRAN_RANK, and ARG1 is the argument being passed to
   the expression.   */

extern struct value *eval_op_f_rank (struct type *expect_type,
				     struct expression *exp,
				     enum noside noside,
				     enum exp_opcode op,
				     struct value *arg1);

/* Implement expression evaluation for Fortran's SIZE keyword. For
   EXPECT_TYPE, EXP, and NOSIDE see expression::evaluate (in
   expression.h).  OPCODE will always for FORTRAN_ARRAY_SIZE.  ARG1 is the
   value passed to SIZE if it is only passed a single argument.  For the
   two argument form see the overload of this function below.  */

extern struct value *eval_op_f_array_size (struct type *expect_type,
					   struct expression *exp,
					   enum noside noside,
					   enum exp_opcode opcode,
					   struct value *arg1);

/* An overload of EVAL_OP_F_ARRAY_SIZE above, this version takes two
   arguments, representing the two values passed to Fortran's SIZE
   keyword.  */

extern struct value *eval_op_f_array_size (struct type *expect_type,
					   struct expression *exp,
					   enum noside noside,
					   enum exp_opcode opcode,
					   struct value *arg1,
					   struct value *arg2);

/* Implement expression evaluation for Fortran's SIZE intrinsic function called
   with three arguments.  For EXPECT_TYPE, EXP, and NOSIDE see
   expression::evaluate (in expression.h).  OPCODE will always be
   FORTRAN_ARRAY_SIZE, ARG1 and ARG2 the first two values passed to SIZE, and
   KIND_ARG is the type corresponding to the KIND parameter passed to SIZE.  */

extern value *eval_op_f_array_size (type *expect_type, expression *exp,
				    noside noside, exp_opcode opcode,
				    value *arg1, value *arg2, type *kind_arg);

/* Implement the evaluation of Fortran's SHAPE keyword.  EXPECTED_TYPE,
   EXP, and NOSIDE are as for expression::evaluate (see expression.h).  OP
   will always be UNOP_FORTRAN_SHAPE, and ARG1 is the argument being passed
   to the expression.  */

extern struct value *eval_op_f_array_shape (struct type *expect_type,
					    struct expression *exp,
					    enum noside noside,
					    enum exp_opcode op,
					    struct value *arg1);

namespace expr
{

/* Function prototype for Fortran intrinsic functions taking one argument and
   one kind argument.  */
typedef value *binary_kind_ftype (type *expect_type, expression *exp,
				  noside noside, exp_opcode op, value *arg1,
				  type *kind_arg);

/* Two-argument operation with the second argument being a kind argument.  */
template<exp_opcode OP, binary_kind_ftype FUNC>
class fortran_kind_2arg
  : public tuple_holding_operation<operation_up, type*>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (type *expect_type, expression *exp, noside noside) override
  {
    value *arg1 = std::get<0> (m_storage)->evaluate (nullptr, exp, noside);
    type *kind_arg = std::get<1> (m_storage);
    return FUNC (expect_type, exp, noside, OP, arg1, kind_arg);
  }

  exp_opcode opcode () const override
  { return OP; }
};

/* Function prototype for Fortran intrinsic functions taking two arguments and
   one kind argument.  */
typedef value *ternary_kind_ftype (type *expect_type, expression *exp,
				   noside noside, exp_opcode op, value *arg1,
				   value *arg2, type *kind_arg);

/* Three-argument operation with the third argument being a kind argument.  */
template<exp_opcode OP, ternary_kind_ftype FUNC>
class fortran_kind_3arg
  : public tuple_holding_operation<operation_up, operation_up, type *>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (type *expect_type, expression *exp, noside noside) override
  {
    value *arg1 = std::get<0> (m_storage)->evaluate (nullptr, exp, noside);
    value *arg2 = std::get<1> (m_storage)->evaluate (nullptr, exp, noside);
    type *kind_arg = std::get<2> (m_storage);
    return FUNC (expect_type, exp, noside, OP, arg1, arg2, kind_arg);
  }

  exp_opcode opcode () const override
  { return OP; }
};

using fortran_abs_operation = unop_operation<UNOP_ABS, eval_op_f_abs>;
using fortran_ceil_operation_1arg = unop_operation<FORTRAN_CEILING,
						   eval_op_f_ceil>;
using fortran_ceil_operation_2arg = fortran_kind_2arg<FORTRAN_CEILING,
						      eval_op_f_ceil>;
using fortran_floor_operation_1arg = unop_operation<FORTRAN_FLOOR,
						    eval_op_f_floor>;
using fortran_floor_operation_2arg = fortran_kind_2arg<FORTRAN_FLOOR,
						       eval_op_f_floor>;
using fortran_kind_operation = unop_operation<UNOP_FORTRAN_KIND,
					      eval_op_f_kind>;
using fortran_allocated_operation = unop_operation<UNOP_FORTRAN_ALLOCATED,
						   eval_op_f_allocated>;
using fortran_loc_operation = unop_operation<UNOP_FORTRAN_LOC,
						   eval_op_f_loc>;

using fortran_mod_operation = binop_operation<BINOP_MOD, eval_op_f_mod>;
using fortran_modulo_operation = binop_operation<BINOP_FORTRAN_MODULO,
						 eval_op_f_modulo>;
using fortran_associated_1arg = unop_operation<FORTRAN_ASSOCIATED,
					       eval_op_f_associated>;
using fortran_associated_2arg = binop_operation<FORTRAN_ASSOCIATED,
						eval_op_f_associated>;
using fortran_rank_operation = unop_operation<UNOP_FORTRAN_RANK,
					      eval_op_f_rank>;
using fortran_array_size_1arg = unop_operation<FORTRAN_ARRAY_SIZE,
					       eval_op_f_array_size>;
using fortran_array_size_2arg = binop_operation<FORTRAN_ARRAY_SIZE,
						eval_op_f_array_size>;
using fortran_array_size_3arg = fortran_kind_3arg<FORTRAN_ARRAY_SIZE,
						  eval_op_f_array_size>;
using fortran_array_shape_operation = unop_operation<UNOP_FORTRAN_SHAPE,
						     eval_op_f_array_shape>;
using fortran_cmplx_operation_1arg = unop_operation<FORTRAN_CMPLX,
						    eval_op_f_cmplx>;
using fortran_cmplx_operation_2arg = binop_operation<FORTRAN_CMPLX,
						     eval_op_f_cmplx>;
using fortran_cmplx_operation_3arg = fortran_kind_3arg<FORTRAN_CMPLX,
						     eval_op_f_cmplx>;

/* OP_RANGE for Fortran.  */
class fortran_range_operation
  : public tuple_holding_operation<enum range_flag, operation_up, operation_up,
				   operation_up>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    error (_("ranges not allowed in this context"));
  }

  range_flag get_flags () const
  {
    return std::get<0> (m_storage);
  }

  value *evaluate0 (struct expression *exp, enum noside noside) const
  {
    return std::get<1> (m_storage)->evaluate (nullptr, exp, noside);
  }

  value *evaluate1 (struct expression *exp, enum noside noside) const
  {
    return std::get<2> (m_storage)->evaluate (nullptr, exp, noside);
  }

  value *evaluate2 (struct expression *exp, enum noside noside) const
  {
    return std::get<3> (m_storage)->evaluate (nullptr, exp, noside);
  }

  enum exp_opcode opcode () const override
  { return OP_RANGE; }
};

/* In F77, functions, substring ops and array subscript operations
   cannot be disambiguated at parse time.  This operation handles
   both, deciding which do to at evaluation time.  */
class fortran_undetermined
  : public tuple_holding_operation<operation_up, std::vector<operation_up>>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override;

  enum exp_opcode opcode () const override
  { return OP_F77_UNDETERMINED_ARGLIST; }

private:

  value *value_subarray (value *array, struct expression *exp,
			 enum noside noside);
};

/* Single-argument form of Fortran ubound/lbound intrinsics.  */
class fortran_bound_1arg
  : public tuple_holding_operation<exp_opcode, operation_up>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override;

  enum exp_opcode opcode () const override
  { return std::get<0> (m_storage); }
};

/* Two-argument form of Fortran ubound/lbound intrinsics.  */
class fortran_bound_2arg
  : public tuple_holding_operation<exp_opcode, operation_up, operation_up>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override;

  enum exp_opcode opcode () const override
  { return std::get<0> (m_storage); }
};

/* Three-argument form of Fortran ubound/lbound intrinsics.  */
class fortran_bound_3arg
  : public tuple_holding_operation<exp_opcode, operation_up, operation_up,
				   type *>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (type *expect_type, expression *exp, noside noside) override;

  exp_opcode opcode () const override
  { return std::get<0> (m_storage); }
};

/* Implement STRUCTOP_STRUCT for Fortran.  */
class fortran_structop_operation
  : public structop_base_operation
{
public:

  using structop_base_operation::structop_base_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override;

  enum exp_opcode opcode () const override
  { return STRUCTOP_STRUCT; }
};

} /* namespace expr */

#endif /* FORTRAN_EXP_H */
