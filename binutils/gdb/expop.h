/* Definitions for expressions in GDB

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

#ifndef EXPOP_H
#define EXPOP_H

#include "c-lang.h"
#include "cp-abi.h"
#include "expression.h"
#include "language.h"
#include "objfiles.h"
#include "gdbsupport/traits.h"
#include "gdbsupport/enum-flags.h"

struct agent_expr;
struct axs_value;

extern void gen_expr_binop (struct expression *exp,
			    enum exp_opcode op,
			    expr::operation *lhs, expr::operation *rhs,
			    struct agent_expr *ax, struct axs_value *value);
extern void gen_expr_structop (struct expression *exp,
			       enum exp_opcode op,
			       expr::operation *lhs,
			       const char *name,
			       struct agent_expr *ax, struct axs_value *value);
extern void gen_expr_unop (struct expression *exp,
			   enum exp_opcode op,
			   expr::operation *lhs,
			   struct agent_expr *ax, struct axs_value *value);

extern struct value *eval_op_scope (struct type *expect_type,
				    struct expression *exp,
				    enum noside noside,
				    struct type *type, const char *string);
extern struct value *eval_op_var_msym_value (struct type *expect_type,
					     struct expression *exp,
					     enum noside noside,
					     bool outermost_p,
					     bound_minimal_symbol msymbol);
extern struct value *eval_op_var_entry_value (struct type *expect_type,
					      struct expression *exp,
					      enum noside noside, symbol *sym);
extern struct value *eval_op_func_static_var (struct type *expect_type,
					      struct expression *exp,
					      enum noside noside,
					      value *func, const char *var);
extern struct value *eval_op_register (struct type *expect_type,
				       struct expression *exp,
				       enum noside noside, const char *name);
extern struct value *eval_op_structop_struct (struct type *expect_type,
					      struct expression *exp,
					      enum noside noside,
					      struct value *arg1,
					      const char *string);
extern struct value *eval_op_structop_ptr (struct type *expect_type,
					   struct expression *exp,
					   enum noside noside,
					   struct value *arg1,
					   const char *string);
extern struct value *eval_op_member (struct type *expect_type,
				     struct expression *exp,
				     enum noside noside,
				     struct value *arg1, struct value *arg2);
extern struct value *eval_op_add (struct type *expect_type,
				  struct expression *exp,
				  enum noside noside,
				  struct value *arg1, struct value *arg2);
extern struct value *eval_op_sub (struct type *expect_type,
				  struct expression *exp,
				  enum noside noside,
				  struct value *arg1, struct value *arg2);
extern struct value *eval_op_binary (struct type *expect_type,
				     struct expression *exp,
				     enum noside noside, enum exp_opcode op,
				     struct value *arg1, struct value *arg2);
extern struct value *eval_op_subscript (struct type *expect_type,
					struct expression *exp,
					enum noside noside, enum exp_opcode op,
					struct value *arg1,
					struct value *arg2);
extern struct value *eval_op_equal (struct type *expect_type,
				    struct expression *exp,
				    enum noside noside, enum exp_opcode op,
				    struct value *arg1,
				    struct value *arg2);
extern struct value *eval_op_notequal (struct type *expect_type,
				       struct expression *exp,
				       enum noside noside, enum exp_opcode op,
				       struct value *arg1,
				       struct value *arg2);
extern struct value *eval_op_less (struct type *expect_type,
				   struct expression *exp,
				   enum noside noside, enum exp_opcode op,
				   struct value *arg1,
				   struct value *arg2);
extern struct value *eval_op_gtr (struct type *expect_type,
				  struct expression *exp,
				  enum noside noside, enum exp_opcode op,
				  struct value *arg1,
				  struct value *arg2);
extern struct value *eval_op_geq (struct type *expect_type,
				  struct expression *exp,
				  enum noside noside, enum exp_opcode op,
				  struct value *arg1,
				  struct value *arg2);
extern struct value *eval_op_leq (struct type *expect_type,
				  struct expression *exp,
				  enum noside noside, enum exp_opcode op,
				  struct value *arg1,
				  struct value *arg2);
extern struct value *eval_op_repeat (struct type *expect_type,
				     struct expression *exp,
				     enum noside noside, enum exp_opcode op,
				     struct value *arg1,
				     struct value *arg2);
extern struct value *eval_op_plus (struct type *expect_type,
				   struct expression *exp,
				   enum noside noside, enum exp_opcode op,
				   struct value *arg1);
extern struct value *eval_op_neg (struct type *expect_type,
				  struct expression *exp,
				  enum noside noside, enum exp_opcode op,
				  struct value *arg1);
extern struct value *eval_op_complement (struct type *expect_type,
					 struct expression *exp,
					 enum noside noside,
					 enum exp_opcode op,
					 struct value *arg1);
extern struct value *eval_op_lognot (struct type *expect_type,
				     struct expression *exp,
				     enum noside noside,
				     enum exp_opcode op,
				     struct value *arg1);
extern struct value *eval_op_preinc (struct type *expect_type,
				     struct expression *exp,
				     enum noside noside,
				     enum exp_opcode op,
				     struct value *arg1);
extern struct value *eval_op_predec (struct type *expect_type,
				     struct expression *exp,
				     enum noside noside,
				     enum exp_opcode op,
				     struct value *arg1);
extern struct value *eval_op_postinc (struct type *expect_type,
				      struct expression *exp,
				      enum noside noside,
				      enum exp_opcode op,
				      struct value *arg1);
extern struct value *eval_op_postdec (struct type *expect_type,
				      struct expression *exp,
				      enum noside noside,
				      enum exp_opcode op,
				      struct value *arg1);
extern struct value *eval_op_ind (struct type *expect_type,
				  struct expression *exp,
				  enum noside noside,
				  struct value *arg1);
extern struct value *eval_op_type (struct type *expect_type,
				   struct expression *exp,
				   enum noside noside, struct type *type);
extern struct value *eval_op_alignof (struct type *expect_type,
				      struct expression *exp,
				      enum noside noside,
				      struct value *arg1);
extern struct value *eval_op_memval (struct type *expect_type,
				     struct expression *exp,
				     enum noside noside,
				     struct value *arg1, struct type *type);
extern struct value *eval_binop_assign_modify (struct type *expect_type,
					       struct expression *exp,
					       enum noside noside,
					       enum exp_opcode op,
					       struct value *arg1,
					       struct value *arg2);

namespace expr
{

class ada_component;

/* The check_objfile overloads are used to check whether a particular
   component of some operation references an objfile.  The passed-in
   objfile will never be a debug objfile.  */

/* See if EXP_OBJFILE matches OBJFILE.  */
static inline bool
check_objfile (struct objfile *exp_objfile, struct objfile *objfile)
{
  if (exp_objfile->separate_debug_objfile_backlink)
    exp_objfile = exp_objfile->separate_debug_objfile_backlink;
  return exp_objfile == objfile;
}

static inline bool
check_objfile (struct type *type, struct objfile *objfile)
{
  struct objfile *ty_objfile = type->objfile_owner ();
  if (ty_objfile != nullptr)
    return check_objfile (ty_objfile, objfile);
  return false;
}

static inline bool
check_objfile (struct symbol *sym, struct objfile *objfile)
{
  return check_objfile (sym->objfile (), objfile);
}

extern bool check_objfile (const struct block *block,
			   struct objfile *objfile);

static inline bool
check_objfile (const block_symbol &sym, struct objfile *objfile)
{
  return (check_objfile (sym.symbol, objfile)
	  || check_objfile (sym.block, objfile));
}

static inline bool
check_objfile (bound_minimal_symbol minsym, struct objfile *objfile)
{
  return check_objfile (minsym.objfile, objfile);
}

static inline bool
check_objfile (internalvar *ivar, struct objfile *objfile)
{
  return false;
}

static inline bool
check_objfile (const std::string &str, struct objfile *objfile)
{
  return false;
}

static inline bool
check_objfile (const operation_up &op, struct objfile *objfile)
{
  return op->uses_objfile (objfile);
}

static inline bool
check_objfile (enum exp_opcode val, struct objfile *objfile)
{
  return false;
}

static inline bool
check_objfile (ULONGEST val, struct objfile *objfile)
{
  return false;
}

static inline bool
check_objfile (const gdb_mpz &val, struct objfile *objfile)
{
  return false;
}

template<typename T>
static inline bool
check_objfile (enum_flags<T> val, struct objfile *objfile)
{
  return false;
}

template<typename T>
static inline bool
check_objfile (const std::vector<T> &collection, struct objfile *objfile)
{
  for (const auto &item : collection)
    {
      if (check_objfile (item, objfile))
	return true;
    }
  return false;
}

template<typename S, typename T>
static inline bool
check_objfile (const std::pair<S, T> &item, struct objfile *objfile)
{
  return (check_objfile (item.first, objfile)
	  || check_objfile (item.second, objfile));
}

extern bool check_objfile (const std::unique_ptr<ada_component> &comp,
			   struct objfile *objfile);

static inline void
dump_for_expression (struct ui_file *stream, int depth,
		     const operation_up &op)
{
  if (op == nullptr)
    gdb_printf (stream, _("%*snullptr\n"), depth, "");
  else
    op->dump (stream, depth);
}

extern void dump_for_expression (struct ui_file *stream, int depth,
				 enum exp_opcode op);
extern void dump_for_expression (struct ui_file *stream, int depth,
				 const std::string &str);
extern void dump_for_expression (struct ui_file *stream, int depth,
				 struct type *type);
extern void dump_for_expression (struct ui_file *stream, int depth,
				 CORE_ADDR addr);
extern void dump_for_expression (struct ui_file *stream, int depth,
				 const gdb_mpz &addr);
extern void dump_for_expression (struct ui_file *stream, int depth,
				 internalvar *ivar);
extern void dump_for_expression (struct ui_file *stream, int depth,
				 symbol *sym);
extern void dump_for_expression (struct ui_file *stream, int depth,
				 const block_symbol &sym);
extern void dump_for_expression (struct ui_file *stream, int depth,
				 bound_minimal_symbol msym);
extern void dump_for_expression (struct ui_file *stream, int depth,
				 const block *bl);
extern void dump_for_expression (struct ui_file *stream, int depth,
				 type_instance_flags flags);
extern void dump_for_expression (struct ui_file *stream, int depth,
				 enum c_string_type_values flags);
extern void dump_for_expression (struct ui_file *stream, int depth,
				 enum range_flag flags);
extern void dump_for_expression (struct ui_file *stream, int depth,
				 const std::unique_ptr<ada_component> &comp);

template<typename T>
void
dump_for_expression (struct ui_file *stream, int depth,
		     const std::vector<T> &vals)
{
  gdb_printf (stream, _("%*sVector:\n"), depth, "");
  for (auto &item : vals)
    dump_for_expression (stream, depth + 1, item);
}

template<typename X, typename Y>
void
dump_for_expression (struct ui_file *stream, int depth,
		     const std::pair<X, Y> &vals)
{
  dump_for_expression (stream, depth, vals.first);
  dump_for_expression (stream, depth, vals.second);
}

/* Base class for most concrete operations.  This class holds data,
   specified via template parameters, and supplies generic
   implementations of the 'dump' and 'uses_objfile' methods.  */
template<typename... Arg>
class tuple_holding_operation : public operation
{
public:

  explicit tuple_holding_operation (Arg... args)
    : m_storage (std::forward<Arg> (args)...)
  {
  }

  DISABLE_COPY_AND_ASSIGN (tuple_holding_operation);

  bool uses_objfile (struct objfile *objfile) const override
  {
    return do_check_objfile<0, Arg...> (objfile, m_storage);
  }

  void dump (struct ui_file *stream, int depth) const override
  {
    dump_for_expression (stream, depth, opcode ());
    do_dump<0, Arg...> (stream, depth + 1, m_storage);
  }

protected:

  /* Storage for the data.  */
  std::tuple<Arg...> m_storage;

private:

  /* do_dump does the work of dumping the data.  */
  template<int I, typename... T>
  typename std::enable_if<I == sizeof... (T), void>::type
  do_dump (struct ui_file *stream, int depth, const std::tuple<T...> &value)
    const
  {
  }

  template<int I, typename... T>
  typename std::enable_if<I < sizeof... (T), void>::type
  do_dump (struct ui_file *stream, int depth, const std::tuple<T...> &value)
    const
  {
    dump_for_expression (stream, depth, std::get<I> (value));
    do_dump<I + 1, T...> (stream, depth, value);
  }

  /* do_check_objfile does the work of checking whether this object
     refers to OBJFILE.  */
  template<int I, typename... T>
  typename std::enable_if<I == sizeof... (T), bool>::type
  do_check_objfile (struct objfile *objfile, const std::tuple<T...> &value)
    const
  {
    return false;
  }

  template<int I, typename... T>
  typename std::enable_if<I < sizeof... (T), bool>::type
  do_check_objfile (struct objfile *objfile, const std::tuple<T...> &value)
    const
  {
    if (check_objfile (std::get<I> (value), objfile))
      return true;
    return do_check_objfile<I + 1, T...> (objfile, value);
  }
};

/* The check_constant overloads are used to decide whether a given
   concrete operation is a constant.  This is done by checking the
   operands.  */

static inline bool
check_constant (const operation_up &item)
{
  return item->constant_p ();
}

static inline bool
check_constant (bound_minimal_symbol msym)
{
  return false;
}

static inline bool
check_constant (struct type *type)
{
  return true;
}

static inline bool
check_constant (const struct block *block)
{
  return true;
}

static inline bool
check_constant (const std::string &str)
{
  return true;
}

static inline bool
check_constant (ULONGEST cst)
{
  return true;
}

static inline bool
check_constant (const gdb_mpz &cst)
{
  return true;
}

static inline bool
check_constant (struct symbol *sym)
{
  enum address_class sc = sym->aclass ();
  return (sc == LOC_BLOCK
	  || sc == LOC_CONST
	  || sc == LOC_CONST_BYTES
	  || sc == LOC_LABEL);
}

static inline bool
check_constant (const block_symbol &sym)
{
  /* We know the block is constant, so we only need to check the
     symbol.  */
  return check_constant (sym.symbol);
}

template<typename T>
static inline bool
check_constant (const std::vector<T> &collection)
{
  for (const auto &item : collection)
    if (!check_constant (item))
      return false;
  return true;
}

template<typename S, typename T>
static inline bool
check_constant (const std::pair<S, T> &item)
{
  return check_constant (item.first) && check_constant (item.second);
}

/* Base class for concrete operations.  This class supplies an
   implementation of 'constant_p' that works by checking the
   operands.  */
template<typename... Arg>
class maybe_constant_operation
  : public tuple_holding_operation<Arg...>
{
public:

  using tuple_holding_operation<Arg...>::tuple_holding_operation;

  bool constant_p () const override
  {
    return do_check_constant<0, Arg...> (this->m_storage);
  }

private:

  template<int I, typename... T>
  typename std::enable_if<I == sizeof... (T), bool>::type
  do_check_constant (const std::tuple<T...> &value) const
  {
    return true;
  }

  template<int I, typename... T>
  typename std::enable_if<I < sizeof... (T), bool>::type
  do_check_constant (const std::tuple<T...> &value) const
  {
    if (!check_constant (std::get<I> (value)))
      return false;
    return do_check_constant<I + 1, T...> (value);
  }
};

/* A floating-point constant.  The constant is encoded in the target
   format.  */

typedef std::array<gdb_byte, 16> float_data;

/* An operation that holds a floating-point constant of a given
   type.

   This does not need the facilities provided by
   tuple_holding_operation, so it does not use it.  */
class float_const_operation
  : public operation
{
public:

  float_const_operation (struct type *type, float_data data)
    : m_type (type),
      m_data (data)
  {
  }

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    return value_from_contents (m_type, m_data.data ());
  }

  enum exp_opcode opcode () const override
  { return OP_FLOAT; }

  bool constant_p () const override
  { return true; }

  void dump (struct ui_file *stream, int depth) const override;

private:

  struct type *m_type;
  float_data m_data;
};

class scope_operation
  : public maybe_constant_operation<struct type *, std::string>
{
public:

  using maybe_constant_operation::maybe_constant_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    return eval_op_scope (expect_type, exp, noside,
			  std::get<0> (m_storage),
			  std::get<1> (m_storage).c_str ());
  }

  value *evaluate_for_address (struct expression *exp,
			       enum noside noside) override;

  value *evaluate_funcall (struct type *expect_type,
			   struct expression *exp,
			   enum noside noside,
			   const std::vector<operation_up> &args) override;

  enum exp_opcode opcode () const override
  { return OP_SCOPE; }

protected:

  void do_generate_ax (struct expression *exp,
		       struct agent_expr *ax,
		       struct axs_value *value,
		       struct type *cast_type)
    override;
};

/* Compute the value of a variable.  */
class var_value_operation
  : public maybe_constant_operation<block_symbol>
{
public:

  using maybe_constant_operation::maybe_constant_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override;

  value *evaluate_with_coercion (struct expression *exp,
				 enum noside noside) override;

  value *evaluate_for_sizeof (struct expression *exp, enum noside noside)
    override;

  value *evaluate_for_cast (struct type *expect_type,
			    struct expression *exp,
			    enum noside noside) override;

  value *evaluate_for_address (struct expression *exp, enum noside noside)
    override;

  value *evaluate_funcall (struct type *expect_type,
			   struct expression *exp,
			   enum noside noside,
			   const std::vector<operation_up> &args) override;

  enum exp_opcode opcode () const override
  { return OP_VAR_VALUE; }

  /* Return the symbol referenced by this object.  */
  symbol *get_symbol () const
  {
    return std::get<0> (m_storage).symbol;
  }

protected:

  void do_generate_ax (struct expression *exp,
		       struct agent_expr *ax,
		       struct axs_value *value,
		       struct type *cast_type)
    override;
};

class long_const_operation
  : public tuple_holding_operation<struct type *, gdb_mpz>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  long_const_operation (struct type *type, LONGEST val)
    : long_const_operation (type, gdb_mpz (val))
  { }

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    return value_from_mpz (std::get<0> (m_storage), std::get<1> (m_storage));
  }

  enum exp_opcode opcode () const override
  { return OP_LONG; }

  bool constant_p () const override
  { return true; }

protected:

  LONGEST as_longest () const
  { return std::get<1> (m_storage).as_integer_truncate<LONGEST> (); }

  void do_generate_ax (struct expression *exp,
		       struct agent_expr *ax,
		       struct axs_value *value,
		       struct type *cast_type)
    override;
};

class var_msym_value_operation
  : public maybe_constant_operation<bound_minimal_symbol>
{
public:

  using maybe_constant_operation::maybe_constant_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    return eval_op_var_msym_value (expect_type, exp, noside, m_outermost,
				   std::get<0> (m_storage));
  }

  value *evaluate_for_sizeof (struct expression *exp, enum noside noside)
    override;

  value *evaluate_for_address (struct expression *exp, enum noside noside)
    override;

  value *evaluate_for_cast (struct type *expect_type,
			    struct expression *exp,
			    enum noside noside) override;

  value *evaluate_funcall (struct type *expect_type,
			   struct expression *exp,
			   enum noside noside,
			   const std::vector<operation_up> &args) override
  {
    const char *name = std::get<0> (m_storage).minsym->print_name ();
    return operation::evaluate_funcall (expect_type, exp, noside, name, args);
  }

  enum exp_opcode opcode () const override
  { return OP_VAR_MSYM_VALUE; }

  void set_outermost () override
  {
    m_outermost = true;
  }

protected:

  /* True if this is the outermost operation in the expression.  */
  bool m_outermost = false;

  void do_generate_ax (struct expression *exp,
		       struct agent_expr *ax,
		       struct axs_value *value,
		       struct type *cast_type)
    override;
};

class var_entry_value_operation
  : public tuple_holding_operation<symbol *>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    return eval_op_var_entry_value (expect_type, exp, noside,
				    std::get<0> (m_storage));
  }

  enum exp_opcode opcode () const override
  { return OP_VAR_ENTRY_VALUE; }
};

class func_static_var_operation
  : public maybe_constant_operation<operation_up, std::string>
{
public:

  using maybe_constant_operation::maybe_constant_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    value *func = std::get<0> (m_storage)->evaluate (nullptr, exp, noside);
    return eval_op_func_static_var (expect_type, exp, noside, func,
				    std::get<1> (m_storage).c_str ());
  }

  enum exp_opcode opcode () const override
  { return OP_FUNC_STATIC_VAR; }
};

class last_operation
  : public tuple_holding_operation<int>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    return access_value_history (std::get<0> (m_storage));
  }

  enum exp_opcode opcode () const override
  { return OP_LAST; }
};

class register_operation
  : public tuple_holding_operation<std::string>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    return eval_op_register (expect_type, exp, noside,
			     std::get<0> (m_storage).c_str ());
  }

  enum exp_opcode opcode () const override
  { return OP_REGISTER; }

  /* Return the name of the register.  */
  const char *get_name () const
  {
    return std::get<0> (m_storage).c_str ();
  }

protected:

  void do_generate_ax (struct expression *exp,
		       struct agent_expr *ax,
		       struct axs_value *value,
		       struct type *cast_type)
    override;
};

class bool_operation
  : public tuple_holding_operation<bool>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    struct type *type = language_bool_type (exp->language_defn, exp->gdbarch);
    return value_from_longest (type, std::get<0> (m_storage));
  }

  enum exp_opcode opcode () const override
  { return OP_BOOL; }

  bool constant_p () const override
  { return true; }
};

class internalvar_operation
  : public tuple_holding_operation<internalvar *>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    return value_of_internalvar (exp->gdbarch,
				 std::get<0> (m_storage));
  }

  internalvar *get_internalvar () const
  {
    return std::get<0> (m_storage);
  }

  enum exp_opcode opcode () const override
  { return OP_INTERNALVAR; }

protected:

  void do_generate_ax (struct expression *exp,
		       struct agent_expr *ax,
		       struct axs_value *value,
		       struct type *cast_type)
    override;
};

class string_operation
  : public tuple_holding_operation<std::string>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override;

  enum exp_opcode opcode () const override
  { return OP_STRING; }
};

class ternop_slice_operation
  : public maybe_constant_operation<operation_up, operation_up, operation_up>
{
public:

  using maybe_constant_operation::maybe_constant_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override;

  enum exp_opcode opcode () const override
  { return TERNOP_SLICE; }
};

class ternop_cond_operation
  : public maybe_constant_operation<operation_up, operation_up, operation_up>
{
public:

  using maybe_constant_operation::maybe_constant_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    struct value *val
      = std::get<0> (m_storage)->evaluate (nullptr, exp, noside);

    if (value_logical_not (val))
      return std::get<2> (m_storage)->evaluate (nullptr, exp, noside);
    return std::get<1> (m_storage)->evaluate (nullptr, exp, noside);
  }

  enum exp_opcode opcode () const override
  { return TERNOP_COND; }

protected:

  void do_generate_ax (struct expression *exp,
		       struct agent_expr *ax,
		       struct axs_value *value,
		       struct type *cast_type)
    override;
};

class complex_operation
  : public maybe_constant_operation<operation_up, operation_up, struct type *>
{
public:

  using maybe_constant_operation::maybe_constant_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    value *real = std::get<0> (m_storage)->evaluate (nullptr, exp, noside);
    value *imag = std::get<1> (m_storage)->evaluate (nullptr, exp, noside);
    return value_literal_complex (real, imag,
				  std::get<2> (m_storage));
  }

  enum exp_opcode opcode () const override
  { return OP_COMPLEX; }
};

class structop_base_operation
  : public tuple_holding_operation<operation_up, std::string>
{
public:

  /* Used for completion.  Return the field name.  */
  const std::string &get_string () const
  {
    return std::get<1> (m_storage);
  }

  value *evaluate_funcall (struct type *expect_type,
			   struct expression *exp,
			   enum noside noside,
			   const std::vector<operation_up> &args) override;

  /* Try to complete this operation in the context of EXP.  TRACKER is
     the completion tracker to update.  Return true if completion was
     possible, false otherwise.  */
  virtual bool complete (struct expression *exp, completion_tracker &tracker)
  {
    return complete (exp, tracker, "");
  }

protected:

  /* Do the work of the public 'complete' method.  PREFIX is prepended
     to each result.  */
  bool complete (struct expression *exp, completion_tracker &tracker,
		 const char *prefix);

  using tuple_holding_operation::tuple_holding_operation;
};

class structop_operation
  : public structop_base_operation
{
public:

  using structop_base_operation::structop_base_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    value *val =std::get<0> (m_storage)->evaluate (nullptr, exp, noside);
    return eval_op_structop_struct (expect_type, exp, noside, val,
				    std::get<1> (m_storage).c_str ());
  }

  enum exp_opcode opcode () const override
  { return STRUCTOP_STRUCT; }

protected:

  void do_generate_ax (struct expression *exp,
		       struct agent_expr *ax,
		       struct axs_value *value,
		       struct type *cast_type)
    override
  {
    gen_expr_structop (exp, STRUCTOP_STRUCT,
		       std::get<0> (this->m_storage).get (),
		       std::get<1> (this->m_storage).c_str (),
		       ax, value);
  }
};

class structop_ptr_operation
  : public structop_base_operation
{
public:

  using structop_base_operation::structop_base_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    value *val = std::get<0> (m_storage)->evaluate (nullptr, exp, noside);
    return eval_op_structop_ptr (expect_type, exp, noside, val,
				 std::get<1> (m_storage).c_str ());
  }

  enum exp_opcode opcode () const override
  { return STRUCTOP_PTR; }

protected:

  void do_generate_ax (struct expression *exp,
		       struct agent_expr *ax,
		       struct axs_value *value,
		       struct type *cast_type)
    override
  {
    gen_expr_structop (exp, STRUCTOP_PTR,
		       std::get<0> (this->m_storage).get (),
		       std::get<1> (this->m_storage).c_str (),
		       ax, value);
  }
};

class structop_member_base
  : public tuple_holding_operation<operation_up, operation_up>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate_funcall (struct type *expect_type,
			   struct expression *exp,
			   enum noside noside,
			   const std::vector<operation_up> &args) override;
};

class structop_member_operation
  : public structop_member_base
{
public:

  using structop_member_base::structop_member_base;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    value *lhs
      = std::get<0> (m_storage)->evaluate_for_address (exp, noside);
    value *rhs
      = std::get<1> (m_storage)->evaluate (nullptr, exp, noside);
    return eval_op_member (expect_type, exp, noside, lhs, rhs);
  }

  enum exp_opcode opcode () const override
  { return STRUCTOP_MEMBER; }
};

class structop_mptr_operation
  : public structop_member_base
{
public:

  using structop_member_base::structop_member_base;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    value *lhs
      = std::get<0> (m_storage)->evaluate (nullptr, exp, noside);
    value *rhs
      = std::get<1> (m_storage)->evaluate (nullptr, exp, noside);
    return eval_op_member (expect_type, exp, noside, lhs, rhs);
  }

  enum exp_opcode opcode () const override
  { return STRUCTOP_MPTR; }
};

class concat_operation
  : public maybe_constant_operation<operation_up, operation_up>
{
public:

  using maybe_constant_operation::maybe_constant_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    value *lhs
      = std::get<0> (m_storage)->evaluate_with_coercion (exp, noside);
    value *rhs
      = std::get<1> (m_storage)->evaluate_with_coercion (exp, noside);
    return value_concat (lhs, rhs);
  }

  enum exp_opcode opcode () const override
  { return BINOP_CONCAT; }
};

class add_operation
  : public maybe_constant_operation<operation_up, operation_up>
{
public:

  using maybe_constant_operation::maybe_constant_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    value *lhs
      = std::get<0> (m_storage)->evaluate_with_coercion (exp, noside);
    value *rhs
      = std::get<1> (m_storage)->evaluate_with_coercion (exp, noside);
    return eval_op_add (expect_type, exp, noside, lhs, rhs);
  }

  enum exp_opcode opcode () const override
  { return BINOP_ADD; }

protected:

  void do_generate_ax (struct expression *exp,
		       struct agent_expr *ax,
		       struct axs_value *value,
		       struct type *cast_type)
    override
  {
    gen_expr_binop (exp, BINOP_ADD,
		    std::get<0> (this->m_storage).get (),
		    std::get<1> (this->m_storage).get (),
		    ax, value);
  }
};

class sub_operation
  : public maybe_constant_operation<operation_up, operation_up>
{
public:

  using maybe_constant_operation::maybe_constant_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    value *lhs
      = std::get<0> (m_storage)->evaluate_with_coercion (exp, noside);
    value *rhs
      = std::get<1> (m_storage)->evaluate_with_coercion (exp, noside);
    return eval_op_sub (expect_type, exp, noside, lhs, rhs);
  }

  enum exp_opcode opcode () const override
  { return BINOP_SUB; }

protected:

  void do_generate_ax (struct expression *exp,
		       struct agent_expr *ax,
		       struct axs_value *value,
		       struct type *cast_type)
    override
  {
    gen_expr_binop (exp, BINOP_SUB,
		    std::get<0> (this->m_storage).get (),
		    std::get<1> (this->m_storage).get (),
		    ax, value);
  }
};

typedef struct value *binary_ftype (struct type *expect_type,
				    struct expression *exp,
				    enum noside noside, enum exp_opcode op,
				    struct value *arg1, struct value *arg2);

template<enum exp_opcode OP, binary_ftype FUNC>
class binop_operation
  : public maybe_constant_operation<operation_up, operation_up>
{
public:

  using maybe_constant_operation::maybe_constant_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    value *lhs
      = std::get<0> (m_storage)->evaluate (nullptr, exp, noside);
    value *rhs
      = std::get<1> (m_storage)->evaluate (nullptr, exp, noside);
    return FUNC (expect_type, exp, noside, OP, lhs, rhs);
  }

  enum exp_opcode opcode () const override
  { return OP; }
};

template<enum exp_opcode OP, binary_ftype FUNC>
class usual_ax_binop_operation
  : public binop_operation<OP, FUNC>
{
public:

  using binop_operation<OP, FUNC>::binop_operation;

protected:

  void do_generate_ax (struct expression *exp,
		       struct agent_expr *ax,
		       struct axs_value *value,
		       struct type *cast_type)
    override
  {
    gen_expr_binop (exp, OP,
		    std::get<0> (this->m_storage).get (),
		    std::get<1> (this->m_storage).get (),
		    ax, value);
  }
};

using exp_operation = binop_operation<BINOP_EXP, eval_op_binary>;
using intdiv_operation = binop_operation<BINOP_INTDIV, eval_op_binary>;
using mod_operation = binop_operation<BINOP_MOD, eval_op_binary>;

using mul_operation = usual_ax_binop_operation<BINOP_MUL, eval_op_binary>;
using div_operation = usual_ax_binop_operation<BINOP_DIV, eval_op_binary>;
using rem_operation = usual_ax_binop_operation<BINOP_REM, eval_op_binary>;
using lsh_operation = usual_ax_binop_operation<BINOP_LSH, eval_op_binary>;
using rsh_operation = usual_ax_binop_operation<BINOP_RSH, eval_op_binary>;
using bitwise_and_operation
     = usual_ax_binop_operation<BINOP_BITWISE_AND, eval_op_binary>;
using bitwise_ior_operation
     = usual_ax_binop_operation<BINOP_BITWISE_IOR, eval_op_binary>;
using bitwise_xor_operation
     = usual_ax_binop_operation<BINOP_BITWISE_XOR, eval_op_binary>;

class subscript_operation
  : public usual_ax_binop_operation<BINOP_SUBSCRIPT, eval_op_subscript>
{
public:
  using usual_ax_binop_operation<BINOP_SUBSCRIPT,
				 eval_op_subscript>::usual_ax_binop_operation;

  value *evaluate_for_sizeof (struct expression *exp,
			      enum noside noside) override;
};

/* Implementation of comparison operations.  */
template<enum exp_opcode OP, binary_ftype FUNC>
class comparison_operation
  : public usual_ax_binop_operation<OP, FUNC>
{
public:

  using usual_ax_binop_operation<OP, FUNC>::usual_ax_binop_operation;

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
};

class equal_operation
  : public comparison_operation<BINOP_EQUAL, eval_op_equal>
{
public:

  using comparison_operation::comparison_operation;

  operation *get_lhs () const
  {
    return std::get<0> (m_storage).get ();
  }

  operation *get_rhs () const
  {
    return std::get<1> (m_storage).get ();
  }
};

using notequal_operation
     = comparison_operation<BINOP_NOTEQUAL, eval_op_notequal>;
using less_operation = comparison_operation<BINOP_LESS, eval_op_less>;
using gtr_operation = comparison_operation<BINOP_GTR, eval_op_gtr>;
using geq_operation = comparison_operation<BINOP_GEQ, eval_op_geq>;
using leq_operation = comparison_operation<BINOP_LEQ, eval_op_leq>;

/* Implement the GDB '@' repeat operator.  */
class repeat_operation
  : public binop_operation<BINOP_REPEAT, eval_op_repeat>
{
  using binop_operation<BINOP_REPEAT, eval_op_repeat>::binop_operation;

protected:

  void do_generate_ax (struct expression *exp,
		       struct agent_expr *ax,
		       struct axs_value *value,
		       struct type *cast_type)
    override;
};

/* C-style comma operator.  */
class comma_operation
  : public maybe_constant_operation<operation_up, operation_up>
{
public:

  using maybe_constant_operation::maybe_constant_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    /* The left-hand-side is only evaluated for side effects, so don't
       bother in other modes.  */
    if (noside == EVAL_NORMAL)
      std::get<0> (m_storage)->evaluate (nullptr, exp, noside);
    return std::get<1> (m_storage)->evaluate (nullptr, exp, noside);
  }

  enum exp_opcode opcode () const override
  { return BINOP_COMMA; }

protected:

  void do_generate_ax (struct expression *exp,
		       struct agent_expr *ax,
		       struct axs_value *value,
		       struct type *cast_type)
    override;
};

typedef struct value *unary_ftype (struct type *expect_type,
				   struct expression *exp,
				   enum noside noside, enum exp_opcode op,
				   struct value *arg1);

/* Base class for unary operations.  */
template<enum exp_opcode OP, unary_ftype FUNC>
class unop_operation
  : public maybe_constant_operation<operation_up>
{
public:

  using maybe_constant_operation::maybe_constant_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    value *val = std::get<0> (m_storage)->evaluate (nullptr, exp, noside);
    return FUNC (expect_type, exp, noside, OP, val);
  }

  enum exp_opcode opcode () const override
  { return OP; }
};

/* Unary operations that can also be turned into agent expressions in
   the "usual" way.  */
template<enum exp_opcode OP, unary_ftype FUNC>
class usual_ax_unop_operation
  : public unop_operation<OP, FUNC>
{
  using unop_operation<OP, FUNC>::unop_operation;

protected:

  void do_generate_ax (struct expression *exp,
		       struct agent_expr *ax,
		       struct axs_value *value,
		       struct type *cast_type)
    override
  {
    gen_expr_unop (exp, OP,
		   std::get<0> (this->m_storage).get (),
		   ax, value);
  }
};

using unary_plus_operation = usual_ax_unop_operation<UNOP_PLUS, eval_op_plus>;
using unary_neg_operation = usual_ax_unop_operation<UNOP_NEG, eval_op_neg>;
using unary_complement_operation
     = usual_ax_unop_operation<UNOP_COMPLEMENT, eval_op_complement>;
using unary_logical_not_operation
     = usual_ax_unop_operation<UNOP_LOGICAL_NOT, eval_op_lognot>;

/* Handle pre- and post- increment and -decrement.  */
template<enum exp_opcode OP, unary_ftype FUNC>
class unop_incr_operation
  : public tuple_holding_operation<operation_up>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    value *val = std::get<0> (m_storage)->evaluate (expect_type, exp, noside);
    return FUNC (expect_type, exp, noside, OP, val);
  }

  enum exp_opcode opcode () const override
  { return OP; }
};

using preinc_operation
     = unop_incr_operation<UNOP_PREINCREMENT, eval_op_preinc>;
using predec_operation
     = unop_incr_operation<UNOP_PREDECREMENT, eval_op_predec>;
using postinc_operation
     = unop_incr_operation<UNOP_POSTINCREMENT, eval_op_postinc>;
using postdec_operation
     = unop_incr_operation<UNOP_POSTDECREMENT, eval_op_postdec>;

/* Base class for implementations of UNOP_IND.  */
class unop_ind_base_operation
  : public tuple_holding_operation<operation_up>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    if (expect_type != nullptr && expect_type->code () == TYPE_CODE_PTR)
      expect_type = check_typedef (expect_type)->target_type ();
    value *val = std::get<0> (m_storage)->evaluate (expect_type, exp, noside);
    return eval_op_ind (expect_type, exp, noside, val);
  }

  value *evaluate_for_address (struct expression *exp,
			       enum noside noside) override;

  value *evaluate_for_sizeof (struct expression *exp,
			      enum noside noside) override;

  enum exp_opcode opcode () const override
  { return UNOP_IND; }
};

/* Ordinary UNOP_IND implementation.  */
class unop_ind_operation
  : public unop_ind_base_operation
{
public:

  using unop_ind_base_operation::unop_ind_base_operation;

protected:

  void do_generate_ax (struct expression *exp,
		       struct agent_expr *ax,
		       struct axs_value *value,
		       struct type *cast_type)
    override
  {
    gen_expr_unop (exp, UNOP_IND,
		   std::get<0> (this->m_storage).get (),
		   ax, value);
  }
};

/* Implement OP_TYPE.  */
class type_operation
  : public tuple_holding_operation<struct type *>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    return eval_op_type (expect_type, exp, noside, std::get<0> (m_storage));
  }

  enum exp_opcode opcode () const override
  { return OP_TYPE; }

  bool constant_p () const override
  { return true; }
};

/* Implement the "typeof" operation.  */
class typeof_operation
  : public maybe_constant_operation<operation_up>
{
public:

  using maybe_constant_operation::maybe_constant_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    if (noside == EVAL_AVOID_SIDE_EFFECTS)
      return std::get<0> (m_storage)->evaluate (nullptr, exp,
						EVAL_AVOID_SIDE_EFFECTS);
    else
      error (_("Attempt to use a type as an expression"));
  }

  enum exp_opcode opcode () const override
  { return OP_TYPEOF; }
};

/* Implement 'decltype'.  */
class decltype_operation
  : public maybe_constant_operation<operation_up>
{
public:

  using maybe_constant_operation::maybe_constant_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    if (noside == EVAL_AVOID_SIDE_EFFECTS)
      {
	value *result
	  = std::get<0> (m_storage)->evaluate (nullptr, exp,
					       EVAL_AVOID_SIDE_EFFECTS);
	enum exp_opcode sub_op = std::get<0> (m_storage)->opcode ();
	if (sub_op == BINOP_SUBSCRIPT
	    || sub_op == STRUCTOP_MEMBER
	    || sub_op == STRUCTOP_MPTR
	    || sub_op == UNOP_IND
	    || sub_op == STRUCTOP_STRUCT
	    || sub_op == STRUCTOP_PTR
	    || sub_op == OP_SCOPE)
	  {
	    struct type *type = result->type ();

	    if (!TYPE_IS_REFERENCE (type))
	      {
		type = lookup_lvalue_reference_type (type);
		result = value::allocate (type);
	      }
	  }

	return result;
      }
    else
      error (_("Attempt to use a type as an expression"));
  }

  enum exp_opcode opcode () const override
  { return OP_DECLTYPE; }
};

/* Implement 'typeid'.  */
class typeid_operation
  : public tuple_holding_operation<operation_up>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    enum exp_opcode sub_op = std::get<0> (m_storage)->opcode ();
    enum noside sub_noside
      = ((sub_op == OP_TYPE || sub_op == OP_DECLTYPE || sub_op == OP_TYPEOF)
	 ? EVAL_AVOID_SIDE_EFFECTS
	 : noside);

    value *result = std::get<0> (m_storage)->evaluate (nullptr, exp,
						       sub_noside);
    if (noside != EVAL_NORMAL)
      return value::allocate (cplus_typeid_type (exp->gdbarch));
    return cplus_typeid (result);
  }

  enum exp_opcode opcode () const override
  { return OP_TYPEID; }
};

/* Implement the address-of operation.  */
class unop_addr_operation
  : public maybe_constant_operation<operation_up>
{
public:

  using maybe_constant_operation::maybe_constant_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    /* C++: check for and handle pointer to members.  */
    return std::get<0> (m_storage)->evaluate_for_address (exp, noside);
  }

  enum exp_opcode opcode () const override
  { return UNOP_ADDR; }

  /* Return the subexpression.  */
  const operation_up &get_expression () const
  {
    return std::get<0> (m_storage);
  }

protected:

  void do_generate_ax (struct expression *exp,
		       struct agent_expr *ax,
		       struct axs_value *value,
		       struct type *cast_type)
    override
  {
    gen_expr_unop (exp, UNOP_ADDR,
		   std::get<0> (this->m_storage).get (),
		   ax, value);
  }
};

/* Implement 'sizeof'.  */
class unop_sizeof_operation
  : public maybe_constant_operation<operation_up>
{
public:

  using maybe_constant_operation::maybe_constant_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    return std::get<0> (m_storage)->evaluate_for_sizeof (exp, noside);
  }

  enum exp_opcode opcode () const override
  { return UNOP_SIZEOF; }

protected:

  void do_generate_ax (struct expression *exp,
		       struct agent_expr *ax,
		       struct axs_value *value,
		       struct type *cast_type)
    override;
};

/* Implement 'alignof'.  */
class unop_alignof_operation
  : public maybe_constant_operation<operation_up>
{
public:

  using maybe_constant_operation::maybe_constant_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    value *val = std::get<0> (m_storage)->evaluate (nullptr, exp,
						    EVAL_AVOID_SIDE_EFFECTS);
    return eval_op_alignof (expect_type, exp, noside, val);
  }

  enum exp_opcode opcode () const override
  { return UNOP_ALIGNOF; }
};

/* Implement UNOP_MEMVAL.  */
class unop_memval_operation
  : public tuple_holding_operation<operation_up, struct type *>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    value *val = std::get<0> (m_storage)->evaluate (expect_type, exp, noside);
    return eval_op_memval (expect_type, exp, noside, val,
			   std::get<1> (m_storage));
  }

  value *evaluate_for_sizeof (struct expression *exp,
			      enum noside noside) override;

  value *evaluate_for_address (struct expression *exp,
			       enum noside noside) override;

  enum exp_opcode opcode () const override
  { return UNOP_MEMVAL; }

  /* Return the type referenced by this object.  */
  struct type *get_type () const
  {
    return std::get<1> (m_storage);
  }

protected:

  void do_generate_ax (struct expression *exp,
		       struct agent_expr *ax,
		       struct axs_value *value,
		       struct type *cast_type)
    override;
};

/* Implement UNOP_MEMVAL_TYPE.  */
class unop_memval_type_operation
  : public tuple_holding_operation<operation_up, operation_up>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    value *typeval
      = std::get<0> (m_storage)->evaluate (expect_type, exp,
					   EVAL_AVOID_SIDE_EFFECTS);
    struct type *type = typeval->type ();
    value *val = std::get<1> (m_storage)->evaluate (expect_type, exp, noside);
    return eval_op_memval (expect_type, exp, noside, val, type);
  }

  value *evaluate_for_sizeof (struct expression *exp,
			      enum noside noside) override;

  value *evaluate_for_address (struct expression *exp,
			       enum noside noside) override;

  enum exp_opcode opcode () const override
  { return UNOP_MEMVAL_TYPE; }

protected:

  void do_generate_ax (struct expression *exp,
		       struct agent_expr *ax,
		       struct axs_value *value,
		       struct type *cast_type)
    override;
};

/* Implement the 'this' expression.  */
class op_this_operation
  : public tuple_holding_operation<>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    return value_of_this (exp->language_defn);
  }

  enum exp_opcode opcode () const override
  { return OP_THIS; }

protected:

  void do_generate_ax (struct expression *exp,
		       struct agent_expr *ax,
		       struct axs_value *value,
		       struct type *cast_type)
    override;
};

/* Implement the "type instance" operation.  */
class type_instance_operation
  : public tuple_holding_operation<type_instance_flags, std::vector<type *>,
				   operation_up>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override;

  enum exp_opcode opcode () const override
  { return TYPE_INSTANCE; }
};

/* The assignment operator.  */
class assign_operation
  : public tuple_holding_operation<operation_up, operation_up>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    value *lhs = std::get<0> (m_storage)->evaluate (nullptr, exp, noside);
    /* Special-case assignments where the left-hand-side is a
       convenience variable -- in these, don't bother setting an
       expected type.  This avoids a weird case where re-assigning a
       string or array to an internal variable could error with "Too
       many array elements".  */
    struct type *xtype = (lhs->lval () == lval_internalvar
			  ? nullptr
			  : lhs->type ());
    value *rhs = std::get<1> (m_storage)->evaluate (xtype, exp, noside);

    if (noside == EVAL_AVOID_SIDE_EFFECTS)
      return lhs;
    if (binop_user_defined_p (BINOP_ASSIGN, lhs, rhs))
      return value_x_binop (lhs, rhs, BINOP_ASSIGN, OP_NULL, noside);
    else
      return value_assign (lhs, rhs);
  }

  enum exp_opcode opcode () const override
  { return BINOP_ASSIGN; }

  /* Return the left-hand-side of the assignment.  */
  operation *get_lhs () const
  {
    return std::get<0> (m_storage).get ();
  }

protected:

  void do_generate_ax (struct expression *exp,
		       struct agent_expr *ax,
		       struct axs_value *value,
		       struct type *cast_type)
    override;
};

/* Assignment with modification, like "+=".  */
class assign_modify_operation
  : public tuple_holding_operation<exp_opcode, operation_up, operation_up>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    value *lhs = std::get<1> (m_storage)->evaluate (nullptr, exp, noside);
    value *rhs = std::get<2> (m_storage)->evaluate (expect_type, exp, noside);
    return eval_binop_assign_modify (expect_type, exp, noside,
				     std::get<0> (m_storage), lhs, rhs);
  }

  enum exp_opcode opcode () const override
  { return BINOP_ASSIGN_MODIFY; }

protected:

  void do_generate_ax (struct expression *exp,
		       struct agent_expr *ax,
		       struct axs_value *value,
		       struct type *cast_type)
    override;
};

/* Not a cast!  Extract a value of a given type from the contents of a
   value.  The new value is extracted from the least significant bytes
   of the old value.  The new value's type must be no bigger than the
   old values type.  */
class unop_extract_operation
  : public maybe_constant_operation<operation_up, struct type *>
{
public:

  using maybe_constant_operation::maybe_constant_operation;

  value *evaluate (struct type *expect_type, struct expression *exp,
		   enum noside noside) override;

  enum exp_opcode opcode () const override
  { return UNOP_EXTRACT; }

  /* Return the type referenced by this object.  */
  struct type *get_type () const
  {
    return std::get<1> (m_storage);
  }

protected:

  void do_generate_ax (struct expression *exp,
		       struct agent_expr *ax,
		       struct axs_value *value,
		       struct type *cast_type) override;
};

/* A type cast.  */
class unop_cast_operation
  : public maybe_constant_operation<operation_up, struct type *>
{
public:

  using maybe_constant_operation::maybe_constant_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    return std::get<0> (m_storage)->evaluate_for_cast (std::get<1> (m_storage),
						       exp, noside);
  }

  enum exp_opcode opcode () const override
  { return UNOP_CAST; }

  /* Return the type referenced by this object.  */
  struct type *get_type () const
  {
    return std::get<1> (m_storage);
  }

protected:

  void do_generate_ax (struct expression *exp,
		       struct agent_expr *ax,
		       struct axs_value *value,
		       struct type *cast_type)
    override;
};

/* A cast, but the type comes from an expression, not a "struct
   type".  */
class unop_cast_type_operation
  : public maybe_constant_operation<operation_up, operation_up>
{
public:

  using maybe_constant_operation::maybe_constant_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    value *val = std::get<0> (m_storage)->evaluate (nullptr, exp,
						    EVAL_AVOID_SIDE_EFFECTS);
    return std::get<1> (m_storage)->evaluate_for_cast (val->type (),
						       exp, noside);
  }

  enum exp_opcode opcode () const override
  { return UNOP_CAST_TYPE; }

protected:

  void do_generate_ax (struct expression *exp,
		       struct agent_expr *ax,
		       struct axs_value *value,
		       struct type *cast_type)
    override;
};

typedef value *cxx_cast_ftype (struct type *, value *);

/* This implements dynamic_cast and reinterpret_cast.  static_cast and
   const_cast are handled by the ordinary case operations.  */
template<exp_opcode OP, cxx_cast_ftype FUNC>
class cxx_cast_operation
  : public maybe_constant_operation<operation_up, operation_up>
{
public:

  using maybe_constant_operation::maybe_constant_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    value *val = std::get<0> (m_storage)->evaluate (nullptr, exp,
						    EVAL_AVOID_SIDE_EFFECTS);
    struct type *type = val->type ();
    value *rhs = std::get<1> (m_storage)->evaluate (type, exp, noside);
    return FUNC (type, rhs);
  }

  enum exp_opcode opcode () const override
  { return OP; }
};

using dynamic_cast_operation = cxx_cast_operation<UNOP_DYNAMIC_CAST,
						  value_dynamic_cast>;
using reinterpret_cast_operation = cxx_cast_operation<UNOP_REINTERPRET_CAST,
						      value_reinterpret_cast>;

/* Multi-dimensional subscripting.  */
class multi_subscript_operation
  : public tuple_holding_operation<operation_up, std::vector<operation_up>>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override;

  enum exp_opcode opcode () const override
  { return MULTI_SUBSCRIPT; }
};

/* The "&&" operator.  */
class logical_and_operation
  : public maybe_constant_operation<operation_up, operation_up>
{
public:

  using maybe_constant_operation::maybe_constant_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override;

  enum exp_opcode opcode () const override
  { return BINOP_LOGICAL_AND; }

protected:

  void do_generate_ax (struct expression *exp,
		       struct agent_expr *ax,
		       struct axs_value *value,
		       struct type *cast_type)
    override;
};

/* The "||" operator.  */
class logical_or_operation
  : public maybe_constant_operation<operation_up, operation_up>
{
public:

  using maybe_constant_operation::maybe_constant_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override;

  enum exp_opcode opcode () const override
  { return BINOP_LOGICAL_OR; }

protected:

  void do_generate_ax (struct expression *exp,
		       struct agent_expr *ax,
		       struct axs_value *value,
		       struct type *cast_type)
    override;
};

/* This class implements ADL (aka Koenig) function calls for C++.  It
   holds the name of the function to call, the block in which the
   lookup should be done, and a vector of arguments.  */
class adl_func_operation
  : public tuple_holding_operation<std::string, const block *,
				   std::vector<operation_up>>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override;

  enum exp_opcode opcode () const override
  { return OP_ADL_FUNC; }
};

/* The OP_ARRAY operation.  */
class array_operation
  : public tuple_holding_operation<int, int, std::vector<operation_up>>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override;

  enum exp_opcode opcode () const override
  { return OP_ARRAY; }

private:

  struct value *evaluate_struct_tuple (struct value *struct_val,
				       struct expression *exp,
				       enum noside noside, int nargs);
};

/* A function call.  This holds the callee operation and the
   arguments.  */
class funcall_operation
  : public tuple_holding_operation<operation_up, std::vector<operation_up>>
{
public:

  using tuple_holding_operation::tuple_holding_operation;

  value *evaluate (struct type *expect_type,
		   struct expression *exp,
		   enum noside noside) override
  {
    return std::get<0> (m_storage)->evaluate_funcall (expect_type, exp, noside,
						      std::get<1> (m_storage));
  }

  enum exp_opcode opcode () const override
  { return OP_FUNCALL; }
};

} /* namespace expr */

#endif /* EXPOP_H */
