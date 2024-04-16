/* Definitions for expressions stored in reversed prefix form, for GDB.

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

#if !defined (EXPRESSION_H)
#define EXPRESSION_H 1

#include "gdbtypes.h"
#include "symtab.h"

/* While parsing expressions we need to track the innermost lexical block
   that we encounter.  In some situations we need to track the innermost
   block just for symbols, and in other situations we want to track the
   innermost block for symbols and registers.  These flags are used by the
   innermost block tracker to control which blocks we consider for the
   innermost block.  These flags can be combined together as needed.  */

enum innermost_block_tracker_type
{
  /* Track the innermost block for symbols within an expression.  */
  INNERMOST_BLOCK_FOR_SYMBOLS = (1 << 0),

  /* Track the innermost block for registers within an expression.  */
  INNERMOST_BLOCK_FOR_REGISTERS = (1 << 1)
};
DEF_ENUM_FLAGS_TYPE (enum innermost_block_tracker_type,
		     innermost_block_tracker_types);

enum exp_opcode : uint8_t
  {
#define OP(name) name ,

#include "std-operator.def"

#undef OP
  };

/* Values of NOSIDE argument to eval_subexp.  */

enum noside
  {
    EVAL_NORMAL,
    EVAL_AVOID_SIDE_EFFECTS	/* Don't modify any variables or
				   call any functions.  The value
				   returned will have the correct
				   type, and will have an
				   approximately correct lvalue
				   type (inaccuracy: anything that is
				   listed as being in a register in
				   the function in which it was
				   declared will be lval_register).
				   Ideally this would not even read
				   target memory, but currently it
				   does in many situations.  */
  };

struct expression;
struct agent_expr;
struct axs_value;
struct type;
struct ui_file;

namespace expr
{

class operation;
typedef std::unique_ptr<operation> operation_up;

/* Base class for an operation.  An operation is a single component of
   an expression.  */

class operation
{
protected:

  operation () = default;
  DISABLE_COPY_AND_ASSIGN (operation);

public:

  virtual ~operation () = default;

  /* Evaluate this operation.  */
  virtual value *evaluate (struct type *expect_type,
			   struct expression *exp,
			   enum noside noside) = 0;

  /* Evaluate this operation in a context where C-like coercion is
     needed.  */
  virtual value *evaluate_with_coercion (struct expression *exp,
					 enum noside noside)
  {
    return evaluate (nullptr, exp, noside);
  }

  /* Evaluate this expression in the context of a cast to
     EXPECT_TYPE.  */
  virtual value *evaluate_for_cast (struct type *expect_type,
				    struct expression *exp,
				    enum noside noside);

  /* Evaluate this expression in the context of a sizeof
     operation.  */
  virtual value *evaluate_for_sizeof (struct expression *exp,
				      enum noside noside);

  /* Evaluate this expression in the context of an address-of
     operation.  Must return the address.  */
  virtual value *evaluate_for_address (struct expression *exp,
				       enum noside noside);

  /* Evaluate a function call, with this object as the callee.
     EXPECT_TYPE, EXP, and NOSIDE have the same meaning as in
     'evaluate'.  ARGS holds the operations that should be evaluated
     to get the arguments to the call.  */
  virtual value *evaluate_funcall (struct type *expect_type,
				   struct expression *exp,
				   enum noside noside,
				   const std::vector<operation_up> &args)
  {
    /* Defer to the helper overload.  */
    return evaluate_funcall (expect_type, exp, noside, nullptr, args);
  }

  /* True if this is a constant expression.  */
  virtual bool constant_p () const
  { return false; }

  /* Return true if this operation uses OBJFILE (and will become
     dangling when OBJFILE is unloaded), otherwise return false.
     OBJFILE must not be a separate debug info file.  */
  virtual bool uses_objfile (struct objfile *objfile) const
  { return false; }

  /* Generate agent expression bytecodes for this operation.  */
  void generate_ax (struct expression *exp, struct agent_expr *ax,
		    struct axs_value *value,
		    struct type *cast_type = nullptr);

  /* Return the opcode that is implemented by this operation.  */
  virtual enum exp_opcode opcode () const = 0;

  /* Print this operation to STREAM.  */
  virtual void dump (struct ui_file *stream, int depth) const = 0;

  /* Call to indicate that this is the outermost operation in the
     expression.  This should almost never be overridden.  */
  virtual void set_outermost () { }

protected:

  /* A helper overload that wraps evaluate_subexp_do_call.  */
  value *evaluate_funcall (struct type *expect_type,
			   struct expression *exp,
			   enum noside noside,
			   const char *function_name,
			   const std::vector<operation_up> &args);

  /* Called by generate_ax to do the work for this particular
     operation.  */
  virtual void do_generate_ax (struct expression *exp,
			       struct agent_expr *ax,
			       struct axs_value *value,
			       struct type *cast_type)
  {
    error (_("Cannot translate to agent expression"));
  }
};

/* A helper function for creating an operation_up, given a type.  */
template<typename T, typename... Arg>
operation_up
make_operation (Arg... args)
{
  return operation_up (new T (std::forward<Arg> (args)...));
}

}

struct expression
{
  expression (const struct language_defn *lang, struct gdbarch *arch)
    : language_defn (lang),
      gdbarch (arch)
  {
  }

  DISABLE_COPY_AND_ASSIGN (expression);

  /* Return the opcode for the outermost sub-expression of this
     expression.  */
  enum exp_opcode first_opcode () const
  {
    return op->opcode ();
  }

  /* Dump the expression to STREAM.  */
  void dump (struct ui_file *stream)
  {
    op->dump (stream, 0);
  }

  /* Return true if this expression uses OBJFILE (and will become
     dangling when OBJFILE is unloaded), otherwise return false.
     OBJFILE must not be a separate debug info file.  */
  bool uses_objfile (struct objfile *objfile) const;

  /* Evaluate the expression.  EXPECT_TYPE is the context type of the
     expression; normally this should be nullptr.  NOSIDE controls how
     evaluation is performed.  */
  struct value *evaluate (struct type *expect_type = nullptr,
			  enum noside noside = EVAL_NORMAL);

  /* Evaluate an expression, avoiding all memory references
     and getting a value whose type alone is correct.  */
  struct value *evaluate_type ()
  { return evaluate (nullptr, EVAL_AVOID_SIDE_EFFECTS); }

  /* Language it was entered in.  */
  const struct language_defn *language_defn;
  /* Architecture it was parsed in.  */
  struct gdbarch *gdbarch;
  expr::operation_up op;
};

typedef std::unique_ptr<expression> expression_up;

/* When parsing expressions we track the innermost block that was
   referenced.  */

class innermost_block_tracker
{
public:
  innermost_block_tracker (innermost_block_tracker_types types
			   = INNERMOST_BLOCK_FOR_SYMBOLS)
    : m_types (types),
      m_innermost_block (NULL)
  { /* Nothing.  */ }

  /* Update the stored innermost block if the new block B is more inner
     than the currently stored block, or if no block is stored yet.  The
     type T tells us whether the block B was for a symbol or for a
     register.  The stored innermost block is only updated if the type T is
     a type we are interested in, the types we are interested in are held
     in M_TYPES and set during RESET.  */
  void update (const struct block *b, innermost_block_tracker_types t);

  /* Overload of main UPDATE method which extracts the block from BS.  */
  void update (const struct block_symbol &bs)
  {
    update (bs.block, INNERMOST_BLOCK_FOR_SYMBOLS);
  }

  /* Return the stored innermost block.  Can be nullptr if no symbols or
     registers were found during an expression parse, and so no innermost
     block was defined.  */
  const struct block *block () const
  {
    return m_innermost_block;
  }

private:
  /* The type of innermost block being looked for.  */
  innermost_block_tracker_types m_types;

  /* The currently stored innermost block found while parsing an
     expression.  */
  const struct block *m_innermost_block;
};

/* Flags that can affect the parsers.  */

enum parser_flag
{
  /* This flag is set if the expression is being evaluated in a
     context where a 'void' result type is expected.  Parsers are free
     to ignore this, or to use it to help with overload resolution
     decisions.  */
  PARSER_VOID_CONTEXT = (1 << 0),

  /* This flag is set if a top-level comma terminates the
     expression.  */
  PARSER_COMMA_TERMINATES = (1 << 1),

  /* This flag is set if the parser should print debugging output as
     it parses.  For yacc-based parsers, this translates to setting
     yydebug.  */
  PARSER_DEBUG = (1 << 2),

  /* Normally the expression-parsing functions like parse_exp_1 will
     attempt to find a context block if one is not passed in.  If set,
     this flag suppresses this search and uses a null context for the
     parse.  */
  PARSER_LEAVE_BLOCK_ALONE = (1 << 3),
};
DEF_ENUM_FLAGS_TYPE (enum parser_flag, parser_flags);

/* From parse.c */

extern expression_up parse_expression (const char *,
				       innermost_block_tracker * = nullptr,
				       parser_flags flags = 0);

extern expression_up parse_expression_with_language (const char *string,
						     enum language lang);


class completion_tracker;

/* Base class for expression completion.  An instance of this
   represents a completion request from the parser.  */
struct expr_completion_base
{
  /* Perform this object's completion.  EXP is the expression in which
     the completion occurs.  TRACKER is the tracker to update with the
     results.  Return true if completion was possible (even if no
     completions were found), false to fall back to ordinary
     expression completion (i.e., symbol names).  */
  virtual bool complete (struct expression *exp,
			 completion_tracker &tracker) = 0;

  virtual ~expr_completion_base () = default;
};

extern expression_up parse_expression_for_completion
     (const char *, std::unique_ptr<expr_completion_base> *completer);

extern expression_up parse_exp_1 (const char **, CORE_ADDR pc,
				  const struct block *,
				  parser_flags flags,
				  innermost_block_tracker * = nullptr);

/* From eval.c */

/* Evaluate a function call.  The function to be called is in CALLEE and
   the arguments passed to the function are in ARGVEC.
   FUNCTION_NAME is the name of the function, if known.
   DEFAULT_RETURN_TYPE is used as the function's return type if the return
   type is unknown.  */

extern struct value *evaluate_subexp_do_call (expression *exp,
					      enum noside noside,
					      value *callee,
					      gdb::array_view<value *> argvec,
					      const char *function_name,
					      type *default_return_type);

/* In an OP_RANGE expression, either bound could be empty, indicating
   that its value is by default that of the corresponding bound of the
   array or string.  Also, the upper end of the range can be exclusive
   or inclusive.  So we have six sorts of subrange.  This enumeration
   type is to identify this.  */

enum range_flag : unsigned
{
  /* This is a standard range.  Both the lower and upper bounds are
     defined, and the bounds are inclusive.  */
  RANGE_STANDARD = 0,

  /* The low bound was not given.  */
  RANGE_LOW_BOUND_DEFAULT = 1 << 0,

  /* The high bound was not given.  */
  RANGE_HIGH_BOUND_DEFAULT = 1 << 1,

  /* The high bound of this range is exclusive.  */
  RANGE_HIGH_BOUND_EXCLUSIVE = 1 << 2,

  /* The range has a stride.  */
  RANGE_HAS_STRIDE = 1 << 3,
};

DEF_ENUM_FLAGS_TYPE (enum range_flag, range_flags);

#endif /* !defined (EXPRESSION_H) */
