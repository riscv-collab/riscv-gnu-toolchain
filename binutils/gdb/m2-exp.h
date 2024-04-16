/* Definitions for Modula-2 expressions

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

#ifndef M2_EXP_H
#define M2_EXP_H

#include "expop.h"

extern struct value *eval_op_m2_high (struct type *expect_type,
				      struct expression *exp,
				      enum noside noside,
				      struct value *arg1);
extern struct value *eval_op_m2_subscript (struct type *expect_type,
					   struct expression *exp,
					   enum noside noside,
					   struct value *arg1,
					   struct value *arg2);

namespace expr
{

/* The Modula-2 "HIGH" operation.  */
class m2_unop_high_operation
  : public tuple_holding_operation<operation_up>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    value *arg1 = std::get<0> (m_storage)->evaluate_with_coercion (exp,
								   noside);
    return eval_op_m2_high (expect_type, exp, noside, arg1);
  }

  enum exp_opcode opcode () const override
  { return UNOP_HIGH; }
};

/* Subscripting for Modula-2.  */
class m2_binop_subscript_operation
  : public tuple_holding_operation<operation_up, operation_up>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    value *arg1 = std::get<0> (m_storage)->evaluate_with_coercion (exp,
								   noside);
    value *arg2 = std::get<1> (m_storage)->evaluate_with_coercion (exp,
								   noside);
    return eval_op_m2_subscript (expect_type, exp, noside, arg1, arg2);
  }

  enum exp_opcode opcode () const override
  { return BINOP_SUBSCRIPT; }
};

} /* namespace expr */

#endif /* M2_EXP_H */
