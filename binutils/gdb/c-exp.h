/* Definitions for C expressions

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

#ifndef C_EXP_H
#define C_EXP_H

#include "expop.h"
#include "objc-lang.h"

extern struct value *eval_op_objc_selector (struct type *expect_type,
					    struct expression *exp,
					    enum noside noside,
					    const char *sel);
extern struct value *opencl_value_cast (struct type *type, struct value *arg);
extern struct value *eval_opencl_assign (struct type *expect_type,
					 struct expression *exp,
					 enum noside noside,
					 enum exp_opcode op,
					 struct value *arg1,
					 struct value *arg2);
extern struct value *opencl_relop (struct type *expect_type,
				   struct expression *exp,
				   enum noside noside, enum exp_opcode op,
				   struct value *arg1, struct value *arg2);
extern struct value *opencl_logical_not (struct type *expect_type,
					 struct expression *exp,
					 enum noside noside,
					 enum exp_opcode op,
					 struct value *arg);

namespace expr
{

class c_string_operation
  : public tuple_holding_operation<enum c_string_type_values,
				   std::vector<std::string>>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override;

  enum exp_opcode opcode () const override
  { return OP_STRING; }
};

class objc_nsstring_operation
  : public tuple_holding_operation<std::string>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    const std::string &str = std::get<0> (m_storage);
    return value_nsstring (exp->gdbarch, str.c_str (), str.size () + 1);
  }

  enum exp_opcode opcode () const override
  { return OP_OBJC_NSSTRING; }
};

class objc_selector_operation
  : public tuple_holding_operation<std::string>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    return eval_op_objc_selector (expect_type, exp, noside,
				  std::get<0> (m_storage).c_str ());
  }

  enum exp_opcode opcode () const override
  { return OP_OBJC_SELECTOR; }
};

/* An Objective C message call.  */
class objc_msgcall_operation
  : public tuple_holding_operation<CORE_ADDR, operation_up,
				   std::vector<operation_up>>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override;

  enum exp_opcode opcode () const override
  { return OP_OBJC_MSGCALL; }
};

using opencl_cast_type_operation = cxx_cast_operation<UNOP_CAST_TYPE,
						      opencl_value_cast>;

/* Binary operations, as needed for OpenCL.  */
template<enum exp_opcode OP, binary_ftype FUNC,
	 typename BASE = maybe_constant_operation<operation_up, operation_up>>
class opencl_binop_operation
  : public BASE
{
public:

  using BASE::BASE;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    value *lhs
      = std::get<0> (this->m_storage)->evaluate (nullptr, exp, noside);
    value *rhs
      = std::get<1> (this->m_storage)->evaluate (lhs->type (), exp,
						 noside);
    return FUNC (expect_type, exp, noside, OP, lhs, rhs);
  }

  enum exp_opcode opcode () const override
  { return OP; }
};

using opencl_assign_operation = opencl_binop_operation<BINOP_ASSIGN,
						       eval_opencl_assign,
						       assign_operation>;
using opencl_equal_operation = opencl_binop_operation<BINOP_EQUAL,
						      opencl_relop>;
using opencl_notequal_operation = opencl_binop_operation<BINOP_NOTEQUAL,
							 opencl_relop>;
using opencl_less_operation = opencl_binop_operation<BINOP_LESS,
						     opencl_relop>;
using opencl_gtr_operation = opencl_binop_operation<BINOP_GTR,
						    opencl_relop>;
using opencl_geq_operation = opencl_binop_operation<BINOP_GEQ,
						    opencl_relop>;
using opencl_leq_operation = opencl_binop_operation<BINOP_LEQ,
						    opencl_relop>;

using opencl_not_operation = unop_operation<UNOP_LOGICAL_NOT,
					    opencl_logical_not>;

/* STRUCTOP_STRUCT implementation for OpenCL.  */
class opencl_structop_operation
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

/* This handles the "&&" and "||" operations for OpenCL.  */
class opencl_logical_binop_operation
  : public tuple_holding_operation<enum exp_opcode,
				   operation_up, operation_up>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override;

  enum exp_opcode opcode () const override
  { return std::get<0> (m_storage); }
};

/* The ?: ternary operator for OpenCL.  */
class opencl_ternop_cond_operation
  : public tuple_holding_operation<operation_up, operation_up, operation_up>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override;

  enum exp_opcode opcode () const override
  { return TERNOP_COND; }
};

}/* namespace expr */

#endif /* C_EXP_H */
