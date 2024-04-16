/* YACC parser for Ada expressions, for GDB.
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

/* Parse an Ada expression from text in a string,
   and return the result as a  struct expression  pointer.
   That structure contains arithmetic operations in reverse polish,
   with constants represented by operations that are followed by special data.
   See expression.h for the details of the format.
   What is important here is that it can be built up sequentially
   during the process of parsing; the lower levels of the tree always
   come first in the result.

   malloc's and realloc's in this file are transformed to
   xmalloc and xrealloc respectively by the same sed command in the
   makefile that remaps any other malloc/realloc inserted by the parser
   generator.  Doing this with #defines and trying to control the interaction
   with include files (<malloc.h> and <stdlib.h> for example) just became
   too messy, particularly when such includes can be inserted at random
   times by the parser generator.  */

%{

#include "defs.h"
#include <ctype.h>
#include "expression.h"
#include "value.h"
#include "parser-defs.h"
#include "language.h"
#include "ada-lang.h"
#include "frame.h"
#include "block.h"
#include "ada-exp.h"

#define parse_type(ps) builtin_type (ps->gdbarch ())

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror,
   etc).  */
#define GDB_YY_REMAP_PREFIX ada_
#include "yy-remap.h"

struct name_info {
  struct symbol *sym;
  struct minimal_symbol *msym;
  const struct block *block;
  struct stoken stoken;
};

/* The state of the parser, used internally when we are parsing the
   expression.  */

static struct parser_state *pstate = NULL;

/* The original expression string.  */
static const char *original_expr;

/* We don't have a good way to manage non-POD data in Yacc, so store
   values here.  The storage here is only valid for the duration of
   the parse.  */
static std::vector<std::unique_ptr<gdb_mpz>> int_storage;

int yyparse (void);

static int yylex (void);

static void yyerror (const char *);

static void write_int (struct parser_state *, LONGEST, struct type *);

static void write_object_renaming (struct parser_state *,
				   const struct block *, const char *, int,
				   const char *, int);

static struct type* write_var_or_type (struct parser_state *,
				       const struct block *, struct stoken);
static struct type *write_var_or_type_completion (struct parser_state *,
						  const struct block *,
						  struct stoken);

static void write_name_assoc (struct parser_state *, struct stoken);

static const struct block *block_lookup (const struct block *, const char *);

static void write_ambiguous_var (struct parser_state *,
				 const struct block *, const char *, int);

static struct type *type_for_char (struct parser_state *, ULONGEST);

static struct type *type_system_address (struct parser_state *);

static std::string find_completion_bounds (struct parser_state *);

using namespace expr;

/* Handle Ada type resolution for OP.  DEPROCEDURE_P and CONTEXT_TYPE
   are passed to the resolve method, if called.  */
static operation_up
resolve (operation_up &&op, bool deprocedure_p, struct type *context_type)
{
  operation_up result = std::move (op);
  ada_resolvable *res = dynamic_cast<ada_resolvable *> (result.get ());
  if (res != nullptr)
    return res->replace (std::move (result),
			 pstate->expout.get (),
			 deprocedure_p,
			 pstate->parse_completion,
			 pstate->block_tracker,
			 context_type);
  return result;
}

/* Like parser_state::pop, but handles Ada type resolution.
   DEPROCEDURE_P and CONTEXT_TYPE are passed to the resolve method, if
   called.  */
static operation_up
ada_pop (bool deprocedure_p = true, struct type *context_type = nullptr)
{
  /* Of course it's ok to call parser_state::pop here... */
  return resolve (pstate->pop (), deprocedure_p, context_type);
}

/* Like parser_state::wrap, but use ada_pop to pop the value.  */
template<typename T>
void
ada_wrap ()
{
  operation_up arg = ada_pop ();
  pstate->push_new<T> (std::move (arg));
}

/* Create and push an address-of operation, as appropriate for Ada.
   If TYPE is not NULL, the resulting operation will be wrapped in a
   cast to TYPE.  */
static void
ada_addrof (struct type *type = nullptr)
{
  operation_up arg = ada_pop (false);
  operation_up addr = make_operation<unop_addr_operation> (std::move (arg));
  operation_up wrapped
    = make_operation<ada_wrapped_operation> (std::move (addr));
  if (type != nullptr)
    wrapped = make_operation<unop_cast_operation> (std::move (wrapped), type);
  pstate->push (std::move (wrapped));
}

/* Handle operator overloading.  Either returns a function all
   operation wrapping the arguments, or it returns null, leaving the
   caller to construct the appropriate operation.  If RHS is null, a
   unary operator is assumed.  */
static operation_up
maybe_overload (enum exp_opcode op, operation_up &lhs, operation_up &rhs)
{
  struct value *args[2];

  int nargs = 1;
  args[0] = lhs->evaluate (nullptr, pstate->expout.get (),
			   EVAL_AVOID_SIDE_EFFECTS);
  if (rhs == nullptr)
    args[1] = nullptr;
  else
    {
      args[1] = rhs->evaluate (nullptr, pstate->expout.get (),
			       EVAL_AVOID_SIDE_EFFECTS);
      ++nargs;
    }

  block_symbol fn = ada_find_operator_symbol (op, pstate->parse_completion,
					      nargs, args);
  if (fn.symbol == nullptr)
    return {};

  if (symbol_read_needs_frame (fn.symbol))
    pstate->block_tracker->update (fn.block, INNERMOST_BLOCK_FOR_SYMBOLS);
  operation_up callee = make_operation<ada_var_value_operation> (fn);

  std::vector<operation_up> argvec;
  argvec.push_back (std::move (lhs));
  if (rhs != nullptr)
    argvec.push_back (std::move (rhs));
  return make_operation<ada_funcall_operation> (std::move (callee),
						std::move (argvec));
}

/* Like parser_state::wrap, but use ada_pop to pop the value, and
   handle unary overloading.  */
template<typename T>
void
ada_wrap_overload (enum exp_opcode op)
{
  operation_up arg = ada_pop ();
  operation_up empty;

  operation_up call = maybe_overload (op, arg, empty);
  if (call == nullptr)
    call = make_operation<T> (std::move (arg));
  pstate->push (std::move (call));
}

/* A variant of parser_state::wrap2 that uses ada_pop to pop both
   operands, and then pushes a new Ada-wrapped operation of the
   template type T.  */
template<typename T>
void
ada_un_wrap2 (enum exp_opcode op)
{
  operation_up rhs = ada_pop ();
  operation_up lhs = ada_pop ();

  operation_up wrapped = maybe_overload (op, lhs, rhs);
  if (wrapped == nullptr)
    {
      wrapped = make_operation<T> (std::move (lhs), std::move (rhs));
      wrapped = make_operation<ada_wrapped_operation> (std::move (wrapped));
    }
  pstate->push (std::move (wrapped));
}

/* A variant of parser_state::wrap2 that uses ada_pop to pop both
   operands.  Unlike ada_un_wrap2, ada_wrapped_operation is not
   used.  */
template<typename T>
void
ada_wrap2 (enum exp_opcode op)
{
  operation_up rhs = ada_pop ();
  operation_up lhs = ada_pop ();
  operation_up call = maybe_overload (op, lhs, rhs);
  if (call == nullptr)
    call = make_operation<T> (std::move (lhs), std::move (rhs));
  pstate->push (std::move (call));
}

/* A variant of parser_state::wrap2 that uses ada_pop to pop both
   operands.  OP is also passed to the constructor of the new binary
   operation.  */
template<typename T>
void
ada_wrap_op (enum exp_opcode op)
{
  operation_up rhs = ada_pop ();
  operation_up lhs = ada_pop ();
  operation_up call = maybe_overload (op, lhs, rhs);
  if (call == nullptr)
    call = make_operation<T> (op, std::move (lhs), std::move (rhs));
  pstate->push (std::move (call));
}

/* Pop three operands using ada_pop, then construct a new ternary
   operation of type T and push it.  */
template<typename T>
void
ada_wrap3 ()
{
  operation_up rhs = ada_pop ();
  operation_up mid = ada_pop ();
  operation_up lhs = ada_pop ();
  pstate->push_new<T> (std::move (lhs), std::move (mid), std::move (rhs));
}

/* Pop NARGS operands, then a callee operand, and use these to
   construct and push a new Ada function call operation.  */
static void
ada_funcall (int nargs)
{
  /* We use the ordinary pop here, because we're going to do
     resolution in a separate step, in order to handle array
     indices.  */
  std::vector<operation_up> args = pstate->pop_vector (nargs);
  /* Call parser_state::pop here, because we don't want to
     function-convert the callee slot of a call we're already
     constructing.  */
  operation_up callee = pstate->pop ();

  ada_var_value_operation *vvo
    = dynamic_cast<ada_var_value_operation *> (callee.get ());
  int array_arity = 0;
  struct type *callee_t = nullptr;
  if (vvo == nullptr
      || vvo->get_symbol ()->domain () != UNDEF_DOMAIN)
    {
      struct value *callee_v = callee->evaluate (nullptr,
						 pstate->expout.get (),
						 EVAL_AVOID_SIDE_EFFECTS);
      callee_t = ada_check_typedef (callee_v->type ());
      array_arity = ada_array_arity (callee_t);
    }

  for (int i = 0; i < nargs; ++i)
    {
      struct type *subtype = nullptr;
      if (i < array_arity)
	subtype = ada_index_type (callee_t, i + 1, "array type");
      args[i] = resolve (std::move (args[i]), true, subtype);
    }

  std::unique_ptr<ada_funcall_operation> funcall
    (new ada_funcall_operation (std::move (callee), std::move (args)));
  funcall->resolve (pstate->expout.get (), true, pstate->parse_completion,
		    pstate->block_tracker, nullptr);
  pstate->push (std::move (funcall));
}

/* The components being constructed during this parse.  */
static std::vector<ada_component_up> components;

/* Create a new ada_component_up of the indicated type and arguments,
   and push it on the global 'components' vector.  */
template<typename T, typename... Arg>
void
push_component (Arg... args)
{
  components.emplace_back (new T (std::forward<Arg> (args)...));
}

/* Examine the final element of the 'components' vector, and return it
   as a pointer to an ada_choices_component.  The caller is
   responsible for ensuring that the final element is in fact an
   ada_choices_component.  */
static ada_choices_component *
choice_component ()
{
  ada_component *last = components.back ().get ();
  return gdb::checked_static_cast<ada_choices_component *> (last);
}

/* Pop the most recent component from the global stack, and return
   it.  */
static ada_component_up
pop_component ()
{
  ada_component_up result = std::move (components.back ());
  components.pop_back ();
  return result;
}

/* Pop the N most recent components from the global stack, and return
   them in a vector.  */
static std::vector<ada_component_up>
pop_components (int n)
{
  std::vector<ada_component_up> result (n);
  for (int i = 1; i <= n; ++i)
    result[n - i] = pop_component ();
  return result;
}

/* The associations being constructed during this parse.  */
static std::vector<ada_association_up> associations;

/* Create a new ada_association_up of the indicated type and
   arguments, and push it on the global 'associations' vector.  */
template<typename T, typename... Arg>
void
push_association (Arg... args)
{
  associations.emplace_back (new T (std::forward<Arg> (args)...));
}

/* Pop the most recent association from the global stack, and return
   it.  */
static ada_association_up
pop_association ()
{
  ada_association_up result = std::move (associations.back ());
  associations.pop_back ();
  return result;
}

/* Pop the N most recent associations from the global stack, and
   return them in a vector.  */
static std::vector<ada_association_up>
pop_associations (int n)
{
  std::vector<ada_association_up> result (n);
  for (int i = 1; i <= n; ++i)
    result[n - i] = pop_association ();
  return result;
}

/* Expression completer for attributes.  */
struct ada_tick_completer : public expr_completion_base
{
  explicit ada_tick_completer (std::string &&name)
    : m_name (std::move (name))
  {
  }

  bool complete (struct expression *exp,
		 completion_tracker &tracker) override;

private:

  std::string m_name;
};

/* Make a new ada_tick_completer and wrap it in a unique pointer.  */
static std::unique_ptr<expr_completion_base>
make_tick_completer (struct stoken tok)
{
  return (std::unique_ptr<expr_completion_base>
	  (new ada_tick_completer (std::string (tok.ptr, tok.length))));
}

/* A convenience typedef.  */
typedef std::unique_ptr<ada_assign_operation> ada_assign_up;

/* The stack of currently active assignment expressions.  This is used
   to implement '@', the target name symbol.  */
static std::vector<ada_assign_up> assignments;

%}

%union
  {
    LONGEST lval;
    struct {
      const gdb_mpz *val;
      struct type *type;
    } typed_val;
    struct {
      LONGEST val;
      struct type *type;
    } typed_char;
    struct {
      gdb_byte val[16];
      struct type *type;
    } typed_val_float;
    struct type *tval;
    struct stoken sval;
    const struct block *bval;
    struct internalvar *ivar;
  }

%type <lval> positional_list component_groups component_associations
%type <lval> aggregate_component_list 
%type <tval> var_or_type type_prefix opt_type_prefix

%token <typed_val> INT NULL_PTR
%token <typed_char> CHARLIT
%token <typed_val_float> FLOAT
%token TRUEKEYWORD FALSEKEYWORD
%token COLONCOLON
%token <sval> STRING NAME DOT_ID TICK_COMPLETE DOT_COMPLETE NAME_COMPLETE
%type <bval> block
%type <lval> arglist tick_arglist

/* Special type cases, put in to allow the parser to distinguish different
   legal basetypes.  */
%token <sval> DOLLAR_VARIABLE

%nonassoc ASSIGN
%left _AND_ OR XOR THEN ELSE
%left '=' NOTEQUAL '<' '>' LEQ GEQ IN DOTDOT
%left '@'
%left '+' '-' '&'
%left UNARY
%left '*' '/' MOD REM
%right STARSTAR ABS NOT

/* Artificial token to give NAME => ... and NAME | priority over reducing 
   NAME to <primary> and to give <primary>' priority over reducing <primary>
   to <simple_exp>. */
%nonassoc VAR

%nonassoc ARROW '|'

%right TICK_ACCESS TICK_ADDRESS TICK_FIRST TICK_LAST TICK_LENGTH
%right TICK_MAX TICK_MIN TICK_MODULUS
%right TICK_POS TICK_RANGE TICK_SIZE TICK_TAG TICK_VAL
%right TICK_COMPLETE TICK_ENUM_REP TICK_ENUM_VAL
 /* The following are right-associative only so that reductions at this
    precedence have lower precedence than '.' and '('.  The syntax still
    forces a.b.c, e.g., to be LEFT-associated.  */
%right '.' '(' '[' DOT_ID DOT_COMPLETE

%token NEW OTHERS


%%

start   :	exp1
	;

/* Expressions, including the sequencing operator.  */
exp1	:	exp
	|	exp1 ';' exp
			{ ada_wrap2<comma_operation> (BINOP_COMMA); }
	| 	primary ASSIGN
			{
			  assignments.emplace_back
			    (new ada_assign_operation (ada_pop (), nullptr));
			}
		exp   /* Extension for convenience */
			{
			  ada_assign_up assign
			    = std::move (assignments.back ());
			  assignments.pop_back ();
			  value *lhs_val = (assign->eval_for_resolution
					    (pstate->expout.get ()));

			  operation_up rhs = pstate->pop ();
			  rhs = resolve (std::move (rhs), true,
					 lhs_val->type ());

			  assign->set_rhs (std::move (rhs));
			  pstate->push (std::move (assign));
			}
	;

/* Expressions, not including the sequencing operator.  */

primary :	primary DOT_ID
			{
			  if (strcmp ($2.ptr, "all") == 0)
			    ada_wrap<ada_unop_ind_operation> ();
			  else
			    {
			      operation_up arg = ada_pop ();
			      pstate->push_new<ada_structop_operation>
				(std::move (arg), copy_name ($2));
			    }
			}
	;

primary :	primary DOT_COMPLETE
			{
			  /* This is done even for ".all", because
			     that might be a prefix.  */
			  operation_up arg = ada_pop ();
			  ada_structop_operation *str_op
			    = (new ada_structop_operation
			       (std::move (arg), copy_name ($2)));
			  str_op->set_prefix (find_completion_bounds (pstate));
			  pstate->push (operation_up (str_op));
			  pstate->mark_struct_expression (str_op);
			}
	;

primary :	primary '(' arglist ')'
			{ ada_funcall ($3); }
	|	var_or_type '(' arglist ')'
			{
			  if ($1 != NULL)
			    {
			      if ($3 != 1)
				error (_("Invalid conversion"));
			      operation_up arg = ada_pop ();
			      pstate->push_new<unop_cast_operation>
				(std::move (arg), $1);
			    }
			  else
			    ada_funcall ($3);
			}
	;

primary :	var_or_type '\'' '(' exp ')'
			{
			  if ($1 == NULL)
			    error (_("Type required for qualification"));
			  operation_up arg = ada_pop (true,
						      check_typedef ($1));
			  pstate->push_new<ada_qual_operation>
			    (std::move (arg), $1);
			}
	;

primary :
		primary '(' simple_exp DOTDOT simple_exp ')'
			{ ada_wrap3<ada_ternop_slice_operation> (); }
	|	var_or_type '(' simple_exp DOTDOT simple_exp ')'
			{ if ($1 == NULL) 
			    ada_wrap3<ada_ternop_slice_operation> ();
			  else
			    error (_("Cannot slice a type"));
			}
	;

primary :	'(' exp1 ')'	{ }
	;

/* The following rule causes a conflict with the type conversion
       var_or_type (exp)
   To get around it, we give '(' higher priority and add bridge rules for 
       var_or_type (exp, exp, ...)
       var_or_type (exp .. exp)
   We also have the action for  var_or_type(exp) generate a function call
   when the first symbol does not denote a type. */

primary :	var_or_type	%prec VAR
			{ if ($1 != NULL)
			    pstate->push_new<type_operation> ($1);
			}
	;

primary :	DOLLAR_VARIABLE /* Various GDB extensions */
			{ pstate->push_dollar ($1); }
	;

primary :     	aggregate
			{
			  pstate->push_new<ada_aggregate_operation>
			    (pop_component ());
			}
	;        

primary :	'@'
			{
			  if (assignments.empty ())
			    error (_("the target name symbol ('@') may only "
				     "appear in an assignment context"));
			  ada_assign_operation *current
			    = assignments.back ().get ();
			  pstate->push_new<ada_target_operation> (current);
			}
	;

simple_exp : 	primary
	;

simple_exp :	'-' simple_exp    %prec UNARY
			{ ada_wrap_overload<ada_neg_operation> (UNOP_NEG); }
	;

simple_exp :	'+' simple_exp    %prec UNARY
			{
			  operation_up arg = ada_pop ();
			  operation_up empty;

			  /* If an overloaded operator was found, use
			     it.  Otherwise, unary + has no effect and
			     the argument can be pushed instead.  */
			  operation_up call = maybe_overload (UNOP_PLUS, arg,
							      empty);
			  if (call != nullptr)
			    arg = std::move (call);
			  pstate->push (std::move (arg));
			}
	;

simple_exp :	NOT simple_exp    %prec UNARY
			{
			  ada_wrap_overload<unary_logical_not_operation>
			    (UNOP_LOGICAL_NOT);
			}
	;

simple_exp :    ABS simple_exp	   %prec UNARY
			{ ada_wrap_overload<ada_abs_operation> (UNOP_ABS); }
	;

arglist	:		{ $$ = 0; }
	;

arglist	:	exp
			{ $$ = 1; }
	|	NAME ARROW exp
			{ $$ = 1; }
	|	arglist ',' exp
			{ $$ = $1 + 1; }
	|	arglist ',' NAME ARROW exp
			{ $$ = $1 + 1; }
	;

primary :	'{' var_or_type '}' primary  %prec '.'
		/* GDB extension */
			{ 
			  if ($2 == NULL)
			    error (_("Type required within braces in coercion"));
			  operation_up arg = ada_pop ();
			  pstate->push_new<unop_memval_operation>
			    (std::move (arg), $2);
			}
	;

/* Binary operators in order of decreasing precedence.  */

simple_exp 	: 	simple_exp STARSTAR simple_exp
			{ ada_wrap2<ada_binop_exp_operation> (BINOP_EXP); }
	;

simple_exp	:	simple_exp '*' simple_exp
			{ ada_wrap2<ada_binop_mul_operation> (BINOP_MUL); }
	;

simple_exp	:	simple_exp '/' simple_exp
			{ ada_wrap2<ada_binop_div_operation> (BINOP_DIV); }
	;

simple_exp	:	simple_exp REM simple_exp /* May need to be fixed to give correct Ada REM */
			{ ada_wrap2<ada_binop_rem_operation> (BINOP_REM); }
	;

simple_exp	:	simple_exp MOD simple_exp
			{ ada_wrap2<ada_binop_mod_operation> (BINOP_MOD); }
	;

simple_exp	:	simple_exp '@' simple_exp	/* GDB extension */
			{ ada_wrap2<repeat_operation> (BINOP_REPEAT); }
	;

simple_exp	:	simple_exp '+' simple_exp
			{ ada_wrap_op<ada_binop_addsub_operation> (BINOP_ADD); }
	;

simple_exp	:	simple_exp '&' simple_exp
			{ ada_wrap2<ada_concat_operation> (BINOP_CONCAT); }
	;

simple_exp	:	simple_exp '-' simple_exp
			{ ada_wrap_op<ada_binop_addsub_operation> (BINOP_SUB); }
	;

relation :	simple_exp
	;

relation :	simple_exp '=' simple_exp
			{ ada_wrap_op<ada_binop_equal_operation> (BINOP_EQUAL); }
	;

relation :	simple_exp NOTEQUAL simple_exp
			{ ada_wrap_op<ada_binop_equal_operation> (BINOP_NOTEQUAL); }
	;

relation :	simple_exp LEQ simple_exp
			{ ada_un_wrap2<leq_operation> (BINOP_LEQ); }
	;

relation :	simple_exp IN simple_exp DOTDOT simple_exp
			{ ada_wrap3<ada_ternop_range_operation> (); }
	|       simple_exp IN primary TICK_RANGE tick_arglist
			{
			  operation_up rhs = ada_pop ();
			  operation_up lhs = ada_pop ();
			  pstate->push_new<ada_binop_in_bounds_operation>
			    (std::move (lhs), std::move (rhs), $5);
			}
 	|	simple_exp IN var_or_type	%prec TICK_ACCESS
			{ 
			  if ($3 == NULL)
			    error (_("Right operand of 'in' must be type"));
			  operation_up arg = ada_pop ();
			  pstate->push_new<ada_unop_range_operation>
			    (std::move (arg), $3);
			}
	|	simple_exp NOT IN simple_exp DOTDOT simple_exp
			{ ada_wrap3<ada_ternop_range_operation> ();
			  ada_wrap<unary_logical_not_operation> (); }
	|       simple_exp NOT IN primary TICK_RANGE tick_arglist
			{
			  operation_up rhs = ada_pop ();
			  operation_up lhs = ada_pop ();
			  pstate->push_new<ada_binop_in_bounds_operation>
			    (std::move (lhs), std::move (rhs), $6);
			  ada_wrap<unary_logical_not_operation> ();
			}
 	|	simple_exp NOT IN var_or_type	%prec TICK_ACCESS
			{ 
			  if ($4 == NULL)
			    error (_("Right operand of 'in' must be type"));
			  operation_up arg = ada_pop ();
			  pstate->push_new<ada_unop_range_operation>
			    (std::move (arg), $4);
			  ada_wrap<unary_logical_not_operation> ();
			}
	;

relation :	simple_exp GEQ simple_exp
			{ ada_un_wrap2<geq_operation> (BINOP_GEQ); }
	;

relation :	simple_exp '<' simple_exp
			{ ada_un_wrap2<less_operation> (BINOP_LESS); }
	;

relation :	simple_exp '>' simple_exp
			{ ada_un_wrap2<gtr_operation> (BINOP_GTR); }
	;

exp	:	relation
	|	and_exp
	|	and_then_exp
	|	or_exp
	|	or_else_exp
	|	xor_exp
	;

and_exp :
		relation _AND_ relation 
			{ ada_wrap2<ada_bitwise_and_operation>
			    (BINOP_BITWISE_AND); }
	|	and_exp _AND_ relation
			{ ada_wrap2<ada_bitwise_and_operation>
			    (BINOP_BITWISE_AND); }
	;

and_then_exp :
	       relation _AND_ THEN relation
			{ ada_wrap2<logical_and_operation>
			    (BINOP_LOGICAL_AND); }
	|	and_then_exp _AND_ THEN relation
			{ ada_wrap2<logical_and_operation>
			    (BINOP_LOGICAL_AND); }
	;

or_exp :
		relation OR relation 
			{ ada_wrap2<ada_bitwise_ior_operation>
			    (BINOP_BITWISE_IOR); }
	|	or_exp OR relation
			{ ada_wrap2<ada_bitwise_ior_operation>
			    (BINOP_BITWISE_IOR); }
	;

or_else_exp :
	       relation OR ELSE relation
			{ ada_wrap2<logical_or_operation> (BINOP_LOGICAL_OR); }
	|      or_else_exp OR ELSE relation
			{ ada_wrap2<logical_or_operation> (BINOP_LOGICAL_OR); }
	;

xor_exp :       relation XOR relation
			{ ada_wrap2<ada_bitwise_xor_operation>
			    (BINOP_BITWISE_XOR); }
	|	xor_exp XOR relation
			{ ada_wrap2<ada_bitwise_xor_operation>
			    (BINOP_BITWISE_XOR); }
	;

/* Primaries can denote types (OP_TYPE).  In cases such as 
   primary TICK_ADDRESS, where a type would be invalid, it will be
   caught when evaluate_subexp in ada-lang.c tries to evaluate the
   primary, expecting a value.  Precedence rules resolve the ambiguity
   in NAME TICK_ACCESS in favor of shifting to form a var_or_type.  A
   construct such as aType'access'access will again cause an error when
   aType'access evaluates to a type that evaluate_subexp attempts to 
   evaluate. */
primary :	primary TICK_ACCESS
			{ ada_addrof (); }
	|	primary TICK_ADDRESS
			{ ada_addrof (type_system_address (pstate)); }
	|	primary TICK_COMPLETE
			{
			  pstate->mark_completion (make_tick_completer ($2));
			}
	|	primary TICK_FIRST tick_arglist
			{
			  operation_up arg = ada_pop ();
			  pstate->push_new<ada_unop_atr_operation>
			    (std::move (arg), OP_ATR_FIRST, $3);
			}
	|	primary TICK_LAST tick_arglist
			{
			  operation_up arg = ada_pop ();
			  pstate->push_new<ada_unop_atr_operation>
			    (std::move (arg), OP_ATR_LAST, $3);
			}
	| 	primary TICK_LENGTH tick_arglist
			{
			  operation_up arg = ada_pop ();
			  pstate->push_new<ada_unop_atr_operation>
			    (std::move (arg), OP_ATR_LENGTH, $3);
			}
	|       primary TICK_SIZE
			{ ada_wrap<ada_atr_size_operation> (); }
	|	primary TICK_TAG
			{ ada_wrap<ada_atr_tag_operation> (); }
	|       opt_type_prefix TICK_MIN '(' exp ',' exp ')'
			{ ada_wrap2<ada_binop_min_operation> (BINOP_MIN); }
	|       opt_type_prefix TICK_MAX '(' exp ',' exp ')'
			{ ada_wrap2<ada_binop_max_operation> (BINOP_MAX); }
	| 	opt_type_prefix TICK_POS '(' exp ')'
			{ ada_wrap<ada_pos_operation> (); }
	|	type_prefix TICK_VAL '(' exp ')'
			{
			  operation_up arg = ada_pop ();
			  pstate->push_new<ada_atr_val_operation>
			    ($1, std::move (arg));
			}
	|	type_prefix TICK_ENUM_REP '(' exp ')'
			{
			  operation_up arg = ada_pop (true, $1);
			  pstate->push_new<ada_atr_enum_rep_operation>
			    ($1, std::move (arg));
			}
	|	type_prefix TICK_ENUM_VAL '(' exp ')'
			{
			  operation_up arg = ada_pop (true, $1);
			  pstate->push_new<ada_atr_enum_val_operation>
			    ($1, std::move (arg));
			}
	|	type_prefix TICK_MODULUS
			{
			  struct type *type_arg = check_typedef ($1);
			  if (!ada_is_modular_type (type_arg))
			    error (_("'modulus must be applied to modular type"));
			  write_int (pstate, ada_modulus (type_arg),
				     type_arg->target_type ());
			}
	;

tick_arglist :			%prec '('
			{ $$ = 1; }
	| 	'(' INT ')'
			{ $$ = $2.val->as_integer<LONGEST> (); }
	;

type_prefix :
		var_or_type
			{ 
			  if ($1 == NULL)
			    error (_("Prefix must be type"));
			  $$ = $1;
			}
	;

opt_type_prefix :
		type_prefix
			{ $$ = $1; }
	| 	/* EMPTY */
			{ $$ = parse_type (pstate)->builtin_void; }
	;


primary	:	INT
			{
			  pstate->push_new<long_const_operation> ($1.type, *$1.val);
			  ada_wrap<ada_wrapped_operation> ();
			}
	;

primary	:	CHARLIT
			{
			  pstate->push_new<ada_char_operation> ($1.type, $1.val);
			}
	;

primary	:	FLOAT
			{
			  float_data data;
			  std::copy (std::begin ($1.val), std::end ($1.val),
				     std::begin (data));
			  pstate->push_new<float_const_operation>
			    ($1.type, data);
			  ada_wrap<ada_wrapped_operation> ();
			}
	;

primary	:	NULL_PTR
			{
			  struct type *null_ptr_type
			    = lookup_pointer_type (parse_type (pstate)->builtin_int0);
			  write_int (pstate, 0, null_ptr_type);
			}
	;

primary	:	STRING
			{ 
			  pstate->push_new<ada_string_operation>
			    (copy_name ($1));
			}
	;

primary :	TRUEKEYWORD
			{
			  write_int (pstate, 1,
				     parse_type (pstate)->builtin_bool);
			}
	|	FALSEKEYWORD
			{
			  write_int (pstate, 0,
				     parse_type (pstate)->builtin_bool);
			}
	;

primary	: 	NEW NAME
			{ error (_("NEW not implemented.")); }
	;

var_or_type:	NAME   	    %prec VAR
				{ $$ = write_var_or_type (pstate, NULL, $1); }
	|	NAME_COMPLETE %prec VAR
				{
				  $$ = write_var_or_type_completion (pstate,
								     NULL,
								     $1);
				}
	|	block NAME  %prec VAR
				{ $$ = write_var_or_type (pstate, $1, $2); }
	|	block NAME_COMPLETE  %prec VAR
				{
				  $$ = write_var_or_type_completion (pstate,
								     $1,
								     $2);
				}
	|       NAME TICK_ACCESS 
			{ 
			  $$ = write_var_or_type (pstate, NULL, $1);
			  if ($$ == NULL)
			    ada_addrof ();
			  else
			    $$ = lookup_pointer_type ($$);
			}
	|	block NAME TICK_ACCESS
			{ 
			  $$ = write_var_or_type (pstate, $1, $2);
			  if ($$ == NULL)
			    ada_addrof ();
			  else
			    $$ = lookup_pointer_type ($$);
			}
	;

/* GDB extension */
block   :       NAME COLONCOLON
			{ $$ = block_lookup (NULL, $1.ptr); }
	|	block NAME COLONCOLON
			{ $$ = block_lookup ($1, $2.ptr); }
	;

aggregate :
		'(' aggregate_component_list ')'  
			{
			  std::vector<ada_component_up> components
			    = pop_components ($2);

			  push_component<ada_aggregate_component>
			    (std::move (components));
			}
	;

aggregate_component_list :
		component_groups	 { $$ = $1; }
	|	positional_list exp
			{
			  push_component<ada_positional_component>
			    ($1, ada_pop ());
			  $$ = $1 + 1;
			}
	|	positional_list component_groups
					 { $$ = $1 + $2; }
	;

positional_list :
		exp ','
			{
			  push_component<ada_positional_component>
			    (0, ada_pop ());
			  $$ = 1;
			} 
	|	positional_list exp ','
			{
			  push_component<ada_positional_component>
			    ($1, ada_pop ());
			  $$ = $1 + 1; 
			}
	;

component_groups:
		others			 { $$ = 1; }
	|	component_group		 { $$ = 1; }
	|	component_group ',' component_groups
					 { $$ = $3 + 1; }
	;

others 	:	OTHERS ARROW exp
			{
			  push_component<ada_others_component> (ada_pop ());
			}
	;

component_group :
		component_associations
			{
			  ada_choices_component *choices = choice_component ();
			  choices->set_associations (pop_associations ($1));
			}
	;

/* We use this somewhat obscure definition in order to handle NAME => and
   NAME | differently from exp => and exp |.  ARROW and '|' have a precedence
   above that of the reduction of NAME to var_or_type.  By delaying 
   decisions until after the => or '|', we convert the ambiguity to a 
   resolved shift/reduce conflict. */
component_associations :
		NAME ARROW exp
			{
			  push_component<ada_choices_component> (ada_pop ());
			  write_name_assoc (pstate, $1);
			  $$ = 1;
			}
	|	simple_exp ARROW exp
			{
			  push_component<ada_choices_component> (ada_pop ());
			  push_association<ada_name_association> (ada_pop ());
			  $$ = 1;
			}
	|	simple_exp DOTDOT simple_exp ARROW exp
			{
			  push_component<ada_choices_component> (ada_pop ());
			  operation_up rhs = ada_pop ();
			  operation_up lhs = ada_pop ();
			  push_association<ada_discrete_range_association>
			    (std::move (lhs), std::move (rhs));
			  $$ = 1;
			}
	|	NAME '|' component_associations
			{
			  write_name_assoc (pstate, $1);
			  $$ = $3 + 1;
			}
	|	simple_exp '|' component_associations
			{
			  push_association<ada_name_association> (ada_pop ());
			  $$ = $3 + 1;
			}
	|	simple_exp DOTDOT simple_exp '|' component_associations

			{
			  operation_up rhs = ada_pop ();
			  operation_up lhs = ada_pop ();
			  push_association<ada_discrete_range_association>
			    (std::move (lhs), std::move (rhs));
			  $$ = $5 + 1;
			}
	;

/* Some extensions borrowed from C, for the benefit of those who find they
   can't get used to Ada notation in GDB.  */

primary	:	'*' primary		%prec '.'
			{ ada_wrap<ada_unop_ind_operation> (); }
	|	'&' primary		%prec '.'
			{ ada_addrof (); }
	|	primary '[' exp ']'
			{
			  ada_wrap2<subscript_operation> (BINOP_SUBSCRIPT);
			  ada_wrap<ada_wrapped_operation> ();
			}
	;

%%

/* yylex defined in ada-lex.c: Reads one token, getting characters */
/* through lexptr.  */

/* Remap normal flex interface names (yylex) as well as gratuitiously */
/* global symbol names, so we can have multiple flex-generated parsers */
/* in gdb.  */

/* (See note above on previous definitions for YACC.) */

#define yy_create_buffer ada_yy_create_buffer
#define yy_delete_buffer ada_yy_delete_buffer
#define yy_init_buffer ada_yy_init_buffer
#define yy_load_buffer_state ada_yy_load_buffer_state
#define yy_switch_to_buffer ada_yy_switch_to_buffer
#define yyrestart ada_yyrestart
#define yytext ada_yytext

static struct obstack temp_parse_space;

/* The following kludge was found necessary to prevent conflicts between */
/* defs.h and non-standard stdlib.h files.  */
#define qsort __qsort__dummy
#include "ada-lex.c"

int
ada_parse (struct parser_state *par_state)
{
  /* Setting up the parser state.  */
  scoped_restore pstate_restore = make_scoped_restore (&pstate);
  gdb_assert (par_state != NULL);
  pstate = par_state;
  original_expr = par_state->lexptr;

  scoped_restore restore_yydebug = make_scoped_restore (&yydebug,
							par_state->debug);

  lexer_init (yyin);		/* (Re-)initialize lexer.  */
  obstack_free (&temp_parse_space, NULL);
  obstack_init (&temp_parse_space);
  components.clear ();
  associations.clear ();
  int_storage.clear ();
  assignments.clear ();

  int result = yyparse ();
  if (!result)
    {
      struct type *context_type = nullptr;
      if (par_state->void_context_p)
	context_type = parse_type (par_state)->builtin_void;
      pstate->set_operation (ada_pop (true, context_type));
    }
  return result;
}

static void
yyerror (const char *msg)
{
  pstate->parse_error (msg);
}

/* Emit expression to access an instance of SYM, in block BLOCK (if
   non-NULL).  */

static void
write_var_from_sym (struct parser_state *par_state, block_symbol sym)
{
  if (symbol_read_needs_frame (sym.symbol))
    par_state->block_tracker->update (sym.block, INNERMOST_BLOCK_FOR_SYMBOLS);

  par_state->push_new<ada_var_value_operation> (sym);
}

/* Write integer or boolean constant ARG of type TYPE.  */

static void
write_int (struct parser_state *par_state, LONGEST arg, struct type *type)
{
  pstate->push_new<long_const_operation> (type, arg);
  ada_wrap<ada_wrapped_operation> ();
}

/* Emit expression corresponding to the renamed object named 
   designated by RENAMED_ENTITY[0 .. RENAMED_ENTITY_LEN-1] in the
   context of ORIG_LEFT_CONTEXT, to which is applied the operations
   encoded by RENAMING_EXPR.  MAX_DEPTH is the maximum number of
   cascaded renamings to allow.  If ORIG_LEFT_CONTEXT is null, it
   defaults to the currently selected block. ORIG_SYMBOL is the 
   symbol that originally encoded the renaming.  It is needed only
   because its prefix also qualifies any index variables used to index
   or slice an array.  It should not be necessary once we go to the
   new encoding entirely (FIXME pnh 7/20/2007).  */

static void
write_object_renaming (struct parser_state *par_state,
		       const struct block *orig_left_context,
		       const char *renamed_entity, int renamed_entity_len,
		       const char *renaming_expr, int max_depth)
{
  char *name;
  enum { SIMPLE_INDEX, LOWER_BOUND, UPPER_BOUND } slice_state;
  struct block_symbol sym_info;

  if (max_depth <= 0)
    error (_("Could not find renamed symbol"));

  if (orig_left_context == NULL)
    orig_left_context = get_selected_block (NULL);

  name = obstack_strndup (&temp_parse_space, renamed_entity,
			  renamed_entity_len);
  ada_lookup_encoded_symbol (name, orig_left_context, VAR_DOMAIN, &sym_info);
  if (sym_info.symbol == NULL)
    error (_("Could not find renamed variable: %s"), ada_decode (name).c_str ());
  else if (sym_info.symbol->aclass () == LOC_TYPEDEF)
    /* We have a renaming of an old-style renaming symbol.  Don't
       trust the block information.  */
    sym_info.block = orig_left_context;

  {
    const char *inner_renamed_entity;
    int inner_renamed_entity_len;
    const char *inner_renaming_expr;

    switch (ada_parse_renaming (sym_info.symbol, &inner_renamed_entity,
				&inner_renamed_entity_len,
				&inner_renaming_expr))
      {
      case ADA_NOT_RENAMING:
	write_var_from_sym (par_state, sym_info);
	break;
      case ADA_OBJECT_RENAMING:
	write_object_renaming (par_state, sym_info.block,
			       inner_renamed_entity, inner_renamed_entity_len,
			       inner_renaming_expr, max_depth - 1);
	break;
      default:
	goto BadEncoding;
      }
  }

  slice_state = SIMPLE_INDEX;
  while (*renaming_expr == 'X')
    {
      renaming_expr += 1;

      switch (*renaming_expr) {
      case 'A':
	renaming_expr += 1;
	ada_wrap<ada_unop_ind_operation> ();
	break;
      case 'L':
	slice_state = LOWER_BOUND;
	[[fallthrough]];
      case 'S':
	renaming_expr += 1;
	if (isdigit (*renaming_expr))
	  {
	    char *next;
	    long val = strtol (renaming_expr, &next, 10);
	    if (next == renaming_expr)
	      goto BadEncoding;
	    renaming_expr = next;
	    write_int (par_state, val, parse_type (par_state)->builtin_int);
	  }
	else
	  {
	    const char *end;
	    char *index_name;
	    struct block_symbol index_sym_info;

	    end = strchr (renaming_expr, 'X');
	    if (end == NULL)
	      end = renaming_expr + strlen (renaming_expr);

	    index_name = obstack_strndup (&temp_parse_space, renaming_expr,
					  end - renaming_expr);
	    renaming_expr = end;

	    ada_lookup_encoded_symbol (index_name, orig_left_context,
				       VAR_DOMAIN, &index_sym_info);
	    if (index_sym_info.symbol == NULL)
	      error (_("Could not find %s"), index_name);
	    else if (index_sym_info.symbol->aclass () == LOC_TYPEDEF)
	      /* Index is an old-style renaming symbol.  */
	      index_sym_info.block = orig_left_context;
	    write_var_from_sym (par_state, index_sym_info);
	  }
	if (slice_state == SIMPLE_INDEX)
	  ada_funcall (1);
	else if (slice_state == LOWER_BOUND)
	  slice_state = UPPER_BOUND;
	else if (slice_state == UPPER_BOUND)
	  {
	    ada_wrap3<ada_ternop_slice_operation> ();
	    slice_state = SIMPLE_INDEX;
	  }
	break;

      case 'R':
	{
	  const char *end;

	  renaming_expr += 1;

	  if (slice_state != SIMPLE_INDEX)
	    goto BadEncoding;
	  end = strchr (renaming_expr, 'X');
	  if (end == NULL)
	    end = renaming_expr + strlen (renaming_expr);

	  operation_up arg = ada_pop ();
	  pstate->push_new<ada_structop_operation>
	    (std::move (arg), std::string (renaming_expr,
					   end - renaming_expr));
	  renaming_expr = end;
	  break;
	}

      default:
	goto BadEncoding;
      }
    }
  if (slice_state == SIMPLE_INDEX)
    return;

 BadEncoding:
  error (_("Internal error in encoding of renaming declaration"));
}

static const struct block*
block_lookup (const struct block *context, const char *raw_name)
{
  const char *name;
  struct symtab *symtab;
  const struct block *result = NULL;

  std::string name_storage;
  if (raw_name[0] == '\'')
    {
      raw_name += 1;
      name = raw_name;
    }
  else
    {
      name_storage = ada_encode (raw_name);
      name = name_storage.c_str ();
    }

  std::vector<struct block_symbol> syms
    = ada_lookup_symbol_list (name, context, VAR_DOMAIN);

  if (context == NULL
      && (syms.empty () || syms[0].symbol->aclass () != LOC_BLOCK))
    symtab = lookup_symtab (name);
  else
    symtab = NULL;

  if (symtab != NULL)
    result = symtab->compunit ()->blockvector ()->static_block ();
  else if (syms.empty () || syms[0].symbol->aclass () != LOC_BLOCK)
    {
      if (context == NULL)
	error (_("No file or function \"%s\"."), raw_name);
      else
	error (_("No function \"%s\" in specified context."), raw_name);
    }
  else
    {
      if (syms.size () > 1)
	warning (_("Function name \"%s\" ambiguous here"), raw_name);
      result = syms[0].symbol->value_block ();
    }

  return result;
}

static struct symbol*
select_possible_type_sym (const std::vector<struct block_symbol> &syms)
{
  int i;
  int preferred_index;
  struct type *preferred_type;
	  
  preferred_index = -1; preferred_type = NULL;
  for (i = 0; i < syms.size (); i += 1)
    switch (syms[i].symbol->aclass ())
      {
      case LOC_TYPEDEF:
	if (ada_prefer_type (syms[i].symbol->type (), preferred_type))
	  {
	    preferred_index = i;
	    preferred_type = syms[i].symbol->type ();
	  }
	break;
      case LOC_REGISTER:
      case LOC_ARG:
      case LOC_REF_ARG:
      case LOC_REGPARM_ADDR:
      case LOC_LOCAL:
      case LOC_COMPUTED:
	return NULL;
      default:
	break;
      }
  if (preferred_type == NULL)
    return NULL;
  return syms[preferred_index].symbol;
}

static struct type*
find_primitive_type (struct parser_state *par_state, const char *name)
{
  struct type *type;
  type = language_lookup_primitive_type (par_state->language (),
					 par_state->gdbarch (),
					 name);
  if (type == NULL && strcmp ("system__address", name) == 0)
    type = type_system_address (par_state);

  if (type != NULL)
    {
      /* Check to see if we have a regular definition of this
	 type that just didn't happen to have been read yet.  */
      struct symbol *sym;
      char *expanded_name = 
	(char *) alloca (strlen (name) + sizeof ("standard__"));
      strcpy (expanded_name, "standard__");
      strcat (expanded_name, name);
      sym = ada_lookup_symbol (expanded_name, NULL, VAR_DOMAIN).symbol;
      if (sym != NULL && sym->aclass () == LOC_TYPEDEF)
	type = sym->type ();
    }

  return type;
}

static int
chop_selector (const char *name, int end)
{
  int i;
  for (i = end - 1; i > 0; i -= 1)
    if (name[i] == '.' || (name[i] == '_' && name[i+1] == '_'))
      return i;
  return -1;
}

/* If NAME is a string beginning with a separator (either '__', or
   '.'), chop this separator and return the result; else, return
   NAME.  */

static const char *
chop_separator (const char *name)
{
  if (*name == '.')
   return name + 1;

  if (name[0] == '_' && name[1] == '_')
    return name + 2;

  return name;
}

/* Given that SELS is a string of the form (<sep><identifier>)*, where
   <sep> is '__' or '.', write the indicated sequence of
   STRUCTOP_STRUCT expression operators.  Returns a pointer to the
   last operation that was pushed.  */
static ada_structop_operation *
write_selectors (struct parser_state *par_state, const char *sels)
{
  ada_structop_operation *result = nullptr;
  while (*sels != '\0')
    {
      const char *p = chop_separator (sels);
      sels = p;
      while (*sels != '\0' && *sels != '.' 
	     && (sels[0] != '_' || sels[1] != '_'))
	sels += 1;
      operation_up arg = ada_pop ();
      result = new ada_structop_operation (std::move (arg),
					   std::string (p, sels - p));
      pstate->push (operation_up (result));
    }
  return result;
}

/* Write a variable access (OP_VAR_VALUE) to ambiguous encoded name
   NAME[0..LEN-1], in block context BLOCK, to be resolved later.  Writes
   a temporary symbol that is valid until the next call to ada_parse.
   */
static void
write_ambiguous_var (struct parser_state *par_state,
		     const struct block *block, const char *name, int len)
{
  struct symbol *sym = new (&temp_parse_space) symbol ();

  sym->set_domain (UNDEF_DOMAIN);
  sym->set_linkage_name (obstack_strndup (&temp_parse_space, name, len));
  sym->set_language (language_ada, nullptr);

  block_symbol bsym { sym, block };
  par_state->push_new<ada_var_value_operation> (bsym);
}

/* A convenient wrapper around ada_get_field_index that takes
   a non NUL-terminated FIELD_NAME0 and a FIELD_NAME_LEN instead
   of a NUL-terminated field name.  */

static int
ada_nget_field_index (const struct type *type, const char *field_name0,
		      int field_name_len, int maybe_missing)
{
  char *field_name = (char *) alloca ((field_name_len + 1) * sizeof (char));

  strncpy (field_name, field_name0, field_name_len);
  field_name[field_name_len] = '\0';
  return ada_get_field_index (type, field_name, maybe_missing);
}

/* If encoded_field_name is the name of a field inside symbol SYM,
   then return the type of that field.  Otherwise, return NULL.

   This function is actually recursive, so if ENCODED_FIELD_NAME
   doesn't match one of the fields of our symbol, then try to see
   if ENCODED_FIELD_NAME could not be a succession of field names
   (in other words, the user entered an expression of the form
   TYPE_NAME.FIELD1.FIELD2.FIELD3), in which case we evaluate
   each field name sequentially to obtain the desired field type.
   In case of failure, we return NULL.  */

static struct type *
get_symbol_field_type (struct symbol *sym, const char *encoded_field_name)
{
  const char *field_name = encoded_field_name;
  const char *subfield_name;
  struct type *type = sym->type ();
  int fieldno;

  if (type == NULL || field_name == NULL)
    return NULL;
  type = check_typedef (type);

  while (field_name[0] != '\0')
    {
      field_name = chop_separator (field_name);

      fieldno = ada_get_field_index (type, field_name, 1);
      if (fieldno >= 0)
	return type->field (fieldno).type ();

      subfield_name = field_name;
      while (*subfield_name != '\0' && *subfield_name != '.' 
	     && (subfield_name[0] != '_' || subfield_name[1] != '_'))
	subfield_name += 1;

      if (subfield_name[0] == '\0')
	return NULL;

      fieldno = ada_nget_field_index (type, field_name,
				      subfield_name - field_name, 1);
      if (fieldno < 0)
	return NULL;

      type = type->field (fieldno).type ();
      field_name = subfield_name;
    }

  return NULL;
}

/* Look up NAME0 (an unencoded identifier or dotted name) in BLOCK (or 
   expression_block_context if NULL).  If it denotes a type, return
   that type.  Otherwise, write expression code to evaluate it as an
   object and return NULL. In this second case, NAME0 will, in general,
   have the form <name>(.<selector_name>)*, where <name> is an object
   or renaming encoded in the debugging data.  Calls error if no
   prefix <name> matches a name in the debugging data (i.e., matches
   either a complete name or, as a wild-card match, the final 
   identifier).  */

static struct type*
write_var_or_type (struct parser_state *par_state,
		   const struct block *block, struct stoken name0)
{
  int depth;
  char *encoded_name;
  int name_len;

  if (block == NULL)
    block = par_state->expression_context_block;

  std::string name_storage = ada_encode (name0.ptr);
  name_len = name_storage.size ();
  encoded_name = obstack_strndup (&temp_parse_space, name_storage.c_str (),
				  name_len);
  for (depth = 0; depth < MAX_RENAMING_CHAIN_LENGTH; depth += 1)
    {
      int tail_index;
      
      tail_index = name_len;
      while (tail_index > 0)
	{
	  struct symbol *type_sym;
	  struct symbol *renaming_sym;
	  const char* renaming;
	  int renaming_len;
	  const char* renaming_expr;
	  int terminator = encoded_name[tail_index];

	  encoded_name[tail_index] = '\0';
	  /* In order to avoid double-encoding, we want to only pass
	     the decoded form to lookup functions.  */
	  std::string decoded_name = ada_decode (encoded_name);
	  encoded_name[tail_index] = terminator;

	  std::vector<struct block_symbol> syms
	    = ada_lookup_symbol_list (decoded_name.c_str (), block, VAR_DOMAIN);

	  type_sym = select_possible_type_sym (syms);

	  if (type_sym != NULL)
	    renaming_sym = type_sym;
	  else if (syms.size () == 1)
	    renaming_sym = syms[0].symbol;
	  else 
	    renaming_sym = NULL;

	  switch (ada_parse_renaming (renaming_sym, &renaming,
				      &renaming_len, &renaming_expr))
	    {
	    case ADA_NOT_RENAMING:
	      break;
	    case ADA_PACKAGE_RENAMING:
	    case ADA_EXCEPTION_RENAMING:
	    case ADA_SUBPROGRAM_RENAMING:
	      {
		int alloc_len = renaming_len + name_len - tail_index + 1;
		char *new_name
		  = (char *) obstack_alloc (&temp_parse_space, alloc_len);
		strncpy (new_name, renaming, renaming_len);
		strcpy (new_name + renaming_len, encoded_name + tail_index);
		encoded_name = new_name;
		name_len = renaming_len + name_len - tail_index;
		goto TryAfterRenaming;
	      }	
	    case ADA_OBJECT_RENAMING:
	      write_object_renaming (par_state, block, renaming, renaming_len,
				     renaming_expr, MAX_RENAMING_CHAIN_LENGTH);
	      write_selectors (par_state, encoded_name + tail_index);
	      return NULL;
	    default:
	      internal_error (_("impossible value from ada_parse_renaming"));
	    }

	  if (type_sym != NULL)
	    {
	      struct type *field_type;
	      
	      if (tail_index == name_len)
		return type_sym->type ();

	      /* We have some extraneous characters after the type name.
		 If this is an expression "TYPE_NAME.FIELD0.[...].FIELDN",
		 then try to get the type of FIELDN.  */
	      field_type
		= get_symbol_field_type (type_sym, encoded_name + tail_index);
	      if (field_type != NULL)
		return field_type;
	      else 
		error (_("Invalid attempt to select from type: \"%s\"."),
		       name0.ptr);
	    }
	  else if (tail_index == name_len && syms.empty ())
	    {
	      struct type *type = find_primitive_type (par_state,
						       encoded_name);

	      if (type != NULL)
		return type;
	    }

	  if (syms.size () == 1)
	    {
	      write_var_from_sym (par_state, syms[0]);
	      write_selectors (par_state, encoded_name + tail_index);
	      return NULL;
	    }
	  else if (syms.empty ())
	    {
	      struct objfile *objfile = nullptr;
	      if (block != nullptr)
		objfile = block->objfile ();

	      struct bound_minimal_symbol msym
		= ada_lookup_simple_minsym (decoded_name.c_str (), objfile);
	      if (msym.minsym != NULL)
		{
		  par_state->push_new<ada_var_msym_value_operation> (msym);
		  /* Maybe cause error here rather than later? FIXME? */
		  write_selectors (par_state, encoded_name + tail_index);
		  return NULL;
		}

	      if (tail_index == name_len
		  && strncmp (encoded_name, "standard__", 
			      sizeof ("standard__") - 1) == 0)
		error (_("No definition of \"%s\" found."), name0.ptr);

	      tail_index = chop_selector (encoded_name, tail_index);
	    } 
	  else
	    {
	      write_ambiguous_var (par_state, block, encoded_name,
				   tail_index);
	      write_selectors (par_state, encoded_name + tail_index);
	      return NULL;
	    }
	}

      if (!have_full_symbols () && !have_partial_symbols () && block == NULL)
	error (_("No symbol table is loaded.  Use the \"file\" command."));
      if (block == par_state->expression_context_block)
	error (_("No definition of \"%s\" in current context."), name0.ptr);
      else
	error (_("No definition of \"%s\" in specified context."), name0.ptr);
      
    TryAfterRenaming: ;
    }

  error (_("Could not find renamed symbol \"%s\""), name0.ptr);

}

/* Because ada_completer_word_break_characters does not contain '.' --
   and it cannot easily be added, this breaks other completions -- we
   have to recreate the completion word-splitting here, so that we can
   provide a prefix that is then used when completing field names.
   Without this, an attempt like "complete print abc.d" will give a
   result like "print def" rather than "print abc.def".  */

static std::string
find_completion_bounds (struct parser_state *par_state)
{
  const char *end = pstate->lexptr;
  /* First the end of the prefix.  Here we stop at the token start or
     at '.' or space.  */
  for (; end > original_expr && end[-1] != '.' && !isspace (end[-1]); --end)
    {
      /* Nothing.  */
    }
  /* Now find the start of the prefix.  */
  const char *ptr = end;
  /* Here we allow '.'.  */
  for (;
       ptr > original_expr && (ptr[-1] == '.'
			       || ptr[-1] == '_'
			       || (ptr[-1] >= 'a' && ptr[-1] <= 'z')
			       || (ptr[-1] >= 'A' && ptr[-1] <= 'Z')
			       || (ptr[-1] & 0xff) >= 0x80);
       --ptr)
    {
      /* Nothing.  */
    }
  /* ... except, skip leading spaces.  */
  ptr = skip_spaces (ptr);

  return std::string (ptr, end);
}

/* A wrapper for write_var_or_type that is used specifically when
   completion is requested for the last of a sequence of
   identifiers.  */

static struct type *
write_var_or_type_completion (struct parser_state *par_state,
			      const struct block *block, struct stoken name0)
{
  int tail_index = chop_selector (name0.ptr, name0.length);
  /* If there's no separator, just defer to ordinary symbol
     completion.  */
  if (tail_index == -1)
    return write_var_or_type (par_state, block, name0);

  std::string copy (name0.ptr, tail_index);
  struct type *type = write_var_or_type (par_state, block,
					 { copy.c_str (),
					   (int) copy.length () });
  /* For completion purposes, it's enough that we return a type
     here.  */
  if (type != nullptr)
    return type;

  ada_structop_operation *op = write_selectors (par_state,
						name0.ptr + tail_index);
  op->set_prefix (find_completion_bounds (par_state));
  par_state->mark_struct_expression (op);
  return nullptr;
}

/* Write a left side of a component association (e.g., NAME in NAME =>
   exp).  If NAME has the form of a selected component, write it as an
   ordinary expression.  If it is a simple variable that unambiguously
   corresponds to exactly one symbol that does not denote a type or an
   object renaming, also write it normally as an OP_VAR_VALUE.
   Otherwise, write it as an OP_NAME.

   Unfortunately, we don't know at this point whether NAME is supposed
   to denote a record component name or the value of an array index.
   Therefore, it is not appropriate to disambiguate an ambiguous name
   as we normally would, nor to replace a renaming with its referent.
   As a result, in the (one hopes) rare case that one writes an
   aggregate such as (R => 42) where R renames an object or is an
   ambiguous name, one must write instead ((R) => 42). */
   
static void
write_name_assoc (struct parser_state *par_state, struct stoken name)
{
  if (strchr (name.ptr, '.') == NULL)
    {
      std::vector<struct block_symbol> syms
	= ada_lookup_symbol_list (name.ptr,
				  par_state->expression_context_block,
				  VAR_DOMAIN);

      if (syms.size () != 1 || syms[0].symbol->aclass () == LOC_TYPEDEF)
	pstate->push_new<ada_string_operation> (copy_name (name));
      else
	write_var_from_sym (par_state, syms[0]);
    }
  else
    if (write_var_or_type (par_state, NULL, name) != NULL)
      error (_("Invalid use of type."));

  push_association<ada_name_association> (ada_pop ());
}

static struct type *
type_for_char (struct parser_state *par_state, ULONGEST value)
{
  if (value <= 0xff)
    return language_string_char_type (par_state->language (),
				      par_state->gdbarch ());
  else if (value <= 0xffff)
    return language_lookup_primitive_type (par_state->language (),
					   par_state->gdbarch (),
					   "wide_character");
  return language_lookup_primitive_type (par_state->language (),
					 par_state->gdbarch (),
					 "wide_wide_character");
}

static struct type *
type_system_address (struct parser_state *par_state)
{
  struct type *type 
    = language_lookup_primitive_type (par_state->language (),
				      par_state->gdbarch (),
				      "system__address");
  return  type != NULL ? type : parse_type (par_state)->builtin_data_ptr;
}

void _initialize_ada_exp ();
void
_initialize_ada_exp ()
{
  obstack_init (&temp_parse_space);
}
