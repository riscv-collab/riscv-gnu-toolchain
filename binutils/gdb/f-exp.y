
/* YACC parser for Fortran expressions, for GDB.
   Copyright (C) 1986-2024 Free Software Foundation, Inc.

   Contributed by Motorola.  Adapted from the C parser by Farooq Butt
   (fmbutt@engage.sps.mot.com).

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

/* This was blantantly ripped off the C expression parser, please 
   be aware of that as you look at its basic structure -FMB */ 

/* Parse a F77 expression from text in a string,
   and return the result as a  struct expression  pointer.
   That structure contains arithmetic operations in reverse polish,
   with constants represented by operations that are followed by special data.
   See expression.h for the details of the format.
   What is important here is that it can be built up sequentially
   during the process of parsing; the lower levels of the tree always
   come first in the result.

   Note that malloc's and realloc's in this file are transformed to
   xmalloc and xrealloc respectively by the same sed command in the
   makefile that remaps any other malloc/realloc inserted by the parser
   generator.  Doing this with #defines and trying to control the interaction
   with include files (<malloc.h> and <stdlib.h> for example) just became
   too messy, particularly when such includes can be inserted at random
   times by the parser generator.  */
   
%{

#include "defs.h"
#include "expression.h"
#include "value.h"
#include "parser-defs.h"
#include "language.h"
#include "f-lang.h"
#include "block.h"
#include <ctype.h>
#include <algorithm>
#include "type-stack.h"
#include "f-exp.h"

#define parse_type(ps) builtin_type (ps->gdbarch ())
#define parse_f_type(ps) builtin_f_type (ps->gdbarch ())

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror,
   etc).  */
#define GDB_YY_REMAP_PREFIX f_
#include "yy-remap.h"

/* The state of the parser, used internally when we are parsing the
   expression.  */

static struct parser_state *pstate = NULL;

/* Depth of parentheses.  */
static int paren_depth;

/* The current type stack.  */
static struct type_stack *type_stack;

int yyparse (void);

static int yylex (void);

static void yyerror (const char *);

static void growbuf_by_size (int);

static int match_string_literal (void);

static void push_kind_type (LONGEST val, struct type *type);

static struct type *convert_to_kind_type (struct type *basetype, int kind);

static void wrap_unop_intrinsic (exp_opcode opcode);

static void wrap_binop_intrinsic (exp_opcode opcode);

static void wrap_ternop_intrinsic (exp_opcode opcode);

template<typename T>
static void fortran_wrap2_kind (type *base_type);

template<typename T>
static void fortran_wrap3_kind (type *base_type);

using namespace expr;
%}

/* Although the yacc "value" of an expression is not used,
   since the result is stored in the structure being created,
   other node types do have values.  */

%union
  {
    LONGEST lval;
    struct {
      LONGEST val;
      struct type *type;
    } typed_val;
    struct {
      gdb_byte val[16];
      struct type *type;
    } typed_val_float;
    struct symbol *sym;
    struct type *tval;
    struct stoken sval;
    struct ttype tsym;
    struct symtoken ssym;
    int voidval;
    enum exp_opcode opcode;
    struct internalvar *ivar;

    struct type **tvec;
    int *ivec;
  }

%{
/* YYSTYPE gets defined by %union */
static int parse_number (struct parser_state *, const char *, int,
			 int, YYSTYPE *);
%}

%type <voidval> exp  type_exp start variable 
%type <tval> type typebase
%type <tvec> nonempty_typelist
/* %type <bval> block */

/* Fancy type parsing.  */
%type <voidval> func_mod direct_abs_decl abs_decl
%type <tval> ptype

%token <typed_val> INT
%token <typed_val_float> FLOAT

/* Both NAME and TYPENAME tokens represent symbols in the input,
   and both convey their data as strings.
   But a TYPENAME is a string that happens to be defined as a typedef
   or builtin type name (such as int or char)
   and a NAME is any other symbol.
   Contexts where this distinction is not important can use the
   nonterminal "name", which matches either NAME or TYPENAME.  */

%token <sval> STRING_LITERAL
%token <lval> BOOLEAN_LITERAL
%token <ssym> NAME 
%token <tsym> TYPENAME
%token <voidval> COMPLETE
%type <sval> name
%type <ssym> name_not_typename

/* A NAME_OR_INT is a symbol which is not known in the symbol table,
   but which would parse as a valid number in the current input radix.
   E.g. "c" when input_radix==16.  Depending on the parse, it will be
   turned into a name or into a number.  */

%token <ssym> NAME_OR_INT 

%token SIZEOF KIND
%token ERROR

/* Special type cases, put in to allow the parser to distinguish different
   legal basetypes.  */
%token INT_S1_KEYWORD INT_S2_KEYWORD INT_KEYWORD INT_S4_KEYWORD INT_S8_KEYWORD
%token LOGICAL_S1_KEYWORD LOGICAL_S2_KEYWORD LOGICAL_KEYWORD LOGICAL_S4_KEYWORD
%token LOGICAL_S8_KEYWORD
%token REAL_KEYWORD REAL_S4_KEYWORD REAL_S8_KEYWORD REAL_S16_KEYWORD
%token COMPLEX_KEYWORD COMPLEX_S4_KEYWORD COMPLEX_S8_KEYWORD
%token COMPLEX_S16_KEYWORD
%token BOOL_AND BOOL_OR BOOL_NOT   
%token SINGLE DOUBLE PRECISION
%token <lval> CHARACTER 

%token <sval> DOLLAR_VARIABLE

%token <opcode> ASSIGN_MODIFY
%token <opcode> UNOP_INTRINSIC BINOP_INTRINSIC
%token <opcode> UNOP_OR_BINOP_INTRINSIC UNOP_OR_BINOP_OR_TERNOP_INTRINSIC

%left ','
%left ABOVE_COMMA
%right '=' ASSIGN_MODIFY
%right '?'
%left BOOL_OR
%right BOOL_NOT
%left BOOL_AND
%left '|'
%left '^'
%left '&'
%left EQUAL NOTEQUAL
%left LESSTHAN GREATERTHAN LEQ GEQ
%left LSH RSH
%left '@'
%left '+' '-'
%left '*' '/'
%right STARSTAR
%right '%'
%right UNARY 
%right '('


%%

start   :	exp
	|	type_exp
	;

type_exp:	type
			{ pstate->push_new<type_operation> ($1); }
	;

exp     :       '(' exp ')'
			{ }
	;

/* Expressions, not including the comma operator.  */
exp	:	'*' exp    %prec UNARY
			{ pstate->wrap<unop_ind_operation> (); }
	;

exp	:	'&' exp    %prec UNARY
			{ pstate->wrap<unop_addr_operation> (); }
	;

exp	:	'-' exp    %prec UNARY
			{ pstate->wrap<unary_neg_operation> (); }
	;

exp	:	BOOL_NOT exp    %prec UNARY
			{ pstate->wrap<unary_logical_not_operation> (); }
	;

exp	:	'~' exp    %prec UNARY
			{ pstate->wrap<unary_complement_operation> (); }
	;

exp	:	SIZEOF exp       %prec UNARY
			{ pstate->wrap<unop_sizeof_operation> (); }
	;

exp	:	KIND '(' exp ')'       %prec UNARY
			{ pstate->wrap<fortran_kind_operation> (); }
	;

/* No more explicit array operators, we treat everything in F77 as 
   a function call.  The disambiguation as to whether we are 
   doing a subscript operation or a function call is done 
   later in eval.c.  */

exp	:	exp '(' 
			{ pstate->start_arglist (); }
		arglist ')'	
			{
			  std::vector<operation_up> args
			    = pstate->pop_vector (pstate->end_arglist ());
			  pstate->push_new<fortran_undetermined>
			    (pstate->pop (), std::move (args));
			}
	;

exp	:	UNOP_INTRINSIC '(' exp ')'
			{
			  wrap_unop_intrinsic ($1);
			}
	;

exp	:	BINOP_INTRINSIC '(' exp ',' exp ')'
			{
			  wrap_binop_intrinsic ($1);
			}
	;

exp	:	UNOP_OR_BINOP_INTRINSIC '('
			{ pstate->start_arglist (); }
		arglist ')'
			{
			  const int n = pstate->end_arglist ();

			  switch (n)
			    {
			    case 1:
			      wrap_unop_intrinsic ($1);
			      break;
			    case 2:
			      wrap_binop_intrinsic ($1);
			      break;
			    default:
			      gdb_assert_not_reached
				("wrong number of arguments for intrinsics");
			    }
			}

exp	:	UNOP_OR_BINOP_OR_TERNOP_INTRINSIC '('
			{ pstate->start_arglist (); }
		arglist ')'
			{
			  const int n = pstate->end_arglist ();

			  switch (n)
			    {
			    case 1:
			      wrap_unop_intrinsic ($1);
			      break;
			    case 2:
			      wrap_binop_intrinsic ($1);
			      break;
			    case 3:
			      wrap_ternop_intrinsic ($1);
			      break;
			    default:
			      gdb_assert_not_reached
				("wrong number of arguments for intrinsics");
			    }
			}
	;

arglist	:
	;

arglist	:	exp
			{ pstate->arglist_len = 1; }
	;

arglist :	subrange
			{ pstate->arglist_len = 1; }
	;
   
arglist	:	arglist ',' exp   %prec ABOVE_COMMA
			{ pstate->arglist_len++; }
	;

arglist	:	arglist ',' subrange   %prec ABOVE_COMMA
			{ pstate->arglist_len++; }
	;

/* There are four sorts of subrange types in F90.  */

subrange:	exp ':' exp	%prec ABOVE_COMMA
			{
			  operation_up high = pstate->pop ();
			  operation_up low = pstate->pop ();
			  pstate->push_new<fortran_range_operation>
			    (RANGE_STANDARD, std::move (low),
			     std::move (high), operation_up ());
			}
	;

subrange:	exp ':'	%prec ABOVE_COMMA
			{
			  operation_up low = pstate->pop ();
			  pstate->push_new<fortran_range_operation>
			    (RANGE_HIGH_BOUND_DEFAULT, std::move (low),
			     operation_up (), operation_up ());
			}
	;

subrange:	':' exp	%prec ABOVE_COMMA
			{
			  operation_up high = pstate->pop ();
			  pstate->push_new<fortran_range_operation>
			    (RANGE_LOW_BOUND_DEFAULT, operation_up (),
			     std::move (high), operation_up ());
			}
	;

subrange:	':'	%prec ABOVE_COMMA
			{
			  pstate->push_new<fortran_range_operation>
			    (RANGE_LOW_BOUND_DEFAULT
			     | RANGE_HIGH_BOUND_DEFAULT,
			     operation_up (), operation_up (),
			     operation_up ());
			}
	;

/* And each of the four subrange types can also have a stride.  */
subrange:	exp ':' exp ':' exp	%prec ABOVE_COMMA
			{
			  operation_up stride = pstate->pop ();
			  operation_up high = pstate->pop ();
			  operation_up low = pstate->pop ();
			  pstate->push_new<fortran_range_operation>
			    (RANGE_STANDARD | RANGE_HAS_STRIDE,
			     std::move (low), std::move (high),
			     std::move (stride));
			}
	;

subrange:	exp ':' ':' exp	%prec ABOVE_COMMA
			{
			  operation_up stride = pstate->pop ();
			  operation_up low = pstate->pop ();
			  pstate->push_new<fortran_range_operation>
			    (RANGE_HIGH_BOUND_DEFAULT
			     | RANGE_HAS_STRIDE,
			     std::move (low), operation_up (),
			     std::move (stride));
			}
	;

subrange:	':' exp ':' exp	%prec ABOVE_COMMA
			{
			  operation_up stride = pstate->pop ();
			  operation_up high = pstate->pop ();
			  pstate->push_new<fortran_range_operation>
			    (RANGE_LOW_BOUND_DEFAULT
			     | RANGE_HAS_STRIDE,
			     operation_up (), std::move (high),
			     std::move (stride));
			}
	;

subrange:	':' ':' exp	%prec ABOVE_COMMA
			{
			  operation_up stride = pstate->pop ();
			  pstate->push_new<fortran_range_operation>
			    (RANGE_LOW_BOUND_DEFAULT
			     | RANGE_HIGH_BOUND_DEFAULT
			     | RANGE_HAS_STRIDE,
			     operation_up (), operation_up (),
			     std::move (stride));
			}
	;

complexnum:     exp ',' exp 
			{ }                          
	;

exp	:	'(' complexnum ')'
			{
			  operation_up rhs = pstate->pop ();
			  operation_up lhs = pstate->pop ();
			  pstate->push_new<complex_operation>
			    (std::move (lhs), std::move (rhs),
			     parse_f_type (pstate)->builtin_complex_s16);
			}
	;

exp	:	'(' type ')' exp  %prec UNARY
			{
			  pstate->push_new<unop_cast_operation>
			    (pstate->pop (), $2);
			}
	;

exp     :       exp '%' name
			{
			  pstate->push_new<fortran_structop_operation>
			    (pstate->pop (), copy_name ($3));
			}
	;

exp     :       exp '%' name COMPLETE
			{
			  structop_base_operation *op
			    = new fortran_structop_operation (pstate->pop (),
							      copy_name ($3));
			  pstate->mark_struct_expression (op);
			  pstate->push (operation_up (op));
			}
	;

exp     :       exp '%' COMPLETE
			{
			  structop_base_operation *op
			    = new fortran_structop_operation (pstate->pop (),
							      "");
			  pstate->mark_struct_expression (op);
			  pstate->push (operation_up (op));
			}
	;

/* Binary operators in order of decreasing precedence.  */

exp	:	exp '@' exp
			{ pstate->wrap2<repeat_operation> (); }
	;

exp	:	exp STARSTAR exp
			{ pstate->wrap2<exp_operation> (); }
	;

exp	:	exp '*' exp
			{ pstate->wrap2<mul_operation> (); }
	;

exp	:	exp '/' exp
			{ pstate->wrap2<div_operation> (); }
	;

exp	:	exp '+' exp
			{ pstate->wrap2<add_operation> (); }
	;

exp	:	exp '-' exp
			{ pstate->wrap2<sub_operation> (); }
	;

exp	:	exp LSH exp
			{ pstate->wrap2<lsh_operation> (); }
	;

exp	:	exp RSH exp
			{ pstate->wrap2<rsh_operation> (); }
	;

exp	:	exp EQUAL exp
			{ pstate->wrap2<equal_operation> (); }
	;

exp	:	exp NOTEQUAL exp
			{ pstate->wrap2<notequal_operation> (); }
	;

exp	:	exp LEQ exp
			{ pstate->wrap2<leq_operation> (); }
	;

exp	:	exp GEQ exp
			{ pstate->wrap2<geq_operation> (); }
	;

exp	:	exp LESSTHAN exp
			{ pstate->wrap2<less_operation> (); }
	;

exp	:	exp GREATERTHAN exp
			{ pstate->wrap2<gtr_operation> (); }
	;

exp	:	exp '&' exp
			{ pstate->wrap2<bitwise_and_operation> (); }
	;

exp	:	exp '^' exp
			{ pstate->wrap2<bitwise_xor_operation> (); }
	;

exp	:	exp '|' exp
			{ pstate->wrap2<bitwise_ior_operation> (); }
	;

exp     :       exp BOOL_AND exp
			{ pstate->wrap2<logical_and_operation> (); }
	;


exp	:	exp BOOL_OR exp
			{ pstate->wrap2<logical_or_operation> (); }
	;

exp	:	exp '=' exp
			{ pstate->wrap2<assign_operation> (); }
	;

exp	:	exp ASSIGN_MODIFY exp
			{
			  operation_up rhs = pstate->pop ();
			  operation_up lhs = pstate->pop ();
			  pstate->push_new<assign_modify_operation>
			    ($2, std::move (lhs), std::move (rhs));
			}
	;

exp	:	INT
			{
			  pstate->push_new<long_const_operation>
			    ($1.type, $1.val);
			}
	;

exp	:	NAME_OR_INT
			{ YYSTYPE val;
			  parse_number (pstate, $1.stoken.ptr,
					$1.stoken.length, 0, &val);
			  pstate->push_new<long_const_operation>
			    (val.typed_val.type,
			     val.typed_val.val);
			}
	;

exp	:	FLOAT
			{
			  float_data data;
			  std::copy (std::begin ($1.val), std::end ($1.val),
				     std::begin (data));
			  pstate->push_new<float_const_operation> ($1.type, data);
			}
	;

exp	:	variable
	;

exp	:	DOLLAR_VARIABLE
			{ pstate->push_dollar ($1); }
	;

exp	:	SIZEOF '(' type ')'	%prec UNARY
			{
			  $3 = check_typedef ($3);
			  pstate->push_new<long_const_operation>
			    (parse_f_type (pstate)->builtin_integer,
			     $3->length ());
			}
	;

exp     :       BOOLEAN_LITERAL
			{ pstate->push_new<bool_operation> ($1); }
	;

exp	:	STRING_LITERAL
			{
			  pstate->push_new<string_operation>
			    (copy_name ($1));
			}
	;

variable:	name_not_typename
			{ struct block_symbol sym = $1.sym;
			  std::string name = copy_name ($1.stoken);
			  pstate->push_symbol (name.c_str (), sym);
			}
	;


type    :       ptype
	;

ptype	:	typebase
	|	typebase abs_decl
		{
		  /* This is where the interesting stuff happens.  */
		  int done = 0;
		  int array_size;
		  struct type *follow_type = $1;
		  struct type *range_type;
		  
		  while (!done)
		    switch (type_stack->pop ())
		      {
		      case tp_end:
			done = 1;
			break;
		      case tp_pointer:
			follow_type = lookup_pointer_type (follow_type);
			break;
		      case tp_reference:
			follow_type = lookup_lvalue_reference_type (follow_type);
			break;
		      case tp_array:
			array_size = type_stack->pop_int ();
			if (array_size != -1)
			  {
			    struct type *idx_type
			      = parse_f_type (pstate)->builtin_integer;
			    type_allocator alloc (idx_type);
			    range_type =
			      create_static_range_type (alloc, idx_type,
							0, array_size - 1);
			    follow_type = create_array_type (alloc,
							     follow_type,
							     range_type);
			  }
			else
			  follow_type = lookup_pointer_type (follow_type);
			break;
		      case tp_function:
			follow_type = lookup_function_type (follow_type);
			break;
		      case tp_kind:
			{
			  int kind_val = type_stack->pop_int ();
			  follow_type
			    = convert_to_kind_type (follow_type, kind_val);
			}
			break;
		      }
		  $$ = follow_type;
		}
	;

abs_decl:	'*'
			{ type_stack->push (tp_pointer); $$ = 0; }
	|	'*' abs_decl
			{ type_stack->push (tp_pointer); $$ = $2; }
	|	'&'
			{ type_stack->push (tp_reference); $$ = 0; }
	|	'&' abs_decl
			{ type_stack->push (tp_reference); $$ = $2; }
	|	direct_abs_decl
	;

direct_abs_decl: '(' abs_decl ')'
			{ $$ = $2; }
	| 	'(' KIND '=' INT ')'
			{ push_kind_type ($4.val, $4.type); }
	|	'*' INT
			{ push_kind_type ($2.val, $2.type); }
	| 	direct_abs_decl func_mod
			{ type_stack->push (tp_function); }
	|	func_mod
			{ type_stack->push (tp_function); }
	;

func_mod:	'(' ')'
			{ $$ = 0; }
	|	'(' nonempty_typelist ')'
			{ free ($2); $$ = 0; }
	;

typebase  /* Implements (approximately): (type-qualifier)* type-specifier */
	:	TYPENAME
			{ $$ = $1.type; }
	|	INT_S1_KEYWORD
			{ $$ = parse_f_type (pstate)->builtin_integer_s1; }
	|	INT_S2_KEYWORD
			{ $$ = parse_f_type (pstate)->builtin_integer_s2; }
	|	INT_KEYWORD
			{ $$ = parse_f_type (pstate)->builtin_integer; }
	|	INT_S4_KEYWORD
			{ $$ = parse_f_type (pstate)->builtin_integer; }
	|	INT_S8_KEYWORD
			{ $$ = parse_f_type (pstate)->builtin_integer_s8; }
	|	CHARACTER 
			{ $$ = parse_f_type (pstate)->builtin_character; }
	|	LOGICAL_S1_KEYWORD 
			{ $$ = parse_f_type (pstate)->builtin_logical_s1; }
	|	LOGICAL_S2_KEYWORD
			{ $$ = parse_f_type (pstate)->builtin_logical_s2; }
	|	LOGICAL_KEYWORD
			{ $$ = parse_f_type (pstate)->builtin_logical; }
	|	LOGICAL_S4_KEYWORD
			{ $$ = parse_f_type (pstate)->builtin_logical; }
	|	LOGICAL_S8_KEYWORD
			{ $$ = parse_f_type (pstate)->builtin_logical_s8; }
	|	REAL_KEYWORD 
			{ $$ = parse_f_type (pstate)->builtin_real; }
	|	REAL_S4_KEYWORD
			{ $$ = parse_f_type (pstate)->builtin_real; }
	|       REAL_S8_KEYWORD
			{ $$ = parse_f_type (pstate)->builtin_real_s8; }
	|	REAL_S16_KEYWORD
			{ $$ = parse_f_type (pstate)->builtin_real_s16; }
	|	COMPLEX_KEYWORD
			{ $$ = parse_f_type (pstate)->builtin_complex; }
	|	COMPLEX_S4_KEYWORD
			{ $$ = parse_f_type (pstate)->builtin_complex; }
	|	COMPLEX_S8_KEYWORD
			{ $$ = parse_f_type (pstate)->builtin_complex_s8; }
	|	COMPLEX_S16_KEYWORD 
			{ $$ = parse_f_type (pstate)->builtin_complex_s16; }
	|	SINGLE PRECISION
			{ $$ = parse_f_type (pstate)->builtin_real;}
	|	DOUBLE PRECISION
			{ $$ = parse_f_type (pstate)->builtin_real_s8;}
	|	SINGLE COMPLEX_KEYWORD
			{ $$ = parse_f_type (pstate)->builtin_complex;}
	|	DOUBLE COMPLEX_KEYWORD
			{ $$ = parse_f_type (pstate)->builtin_complex_s8;}
	;

nonempty_typelist
	:	type
		{ $$ = (struct type **) malloc (sizeof (struct type *) * 2);
		  $<ivec>$[0] = 1;	/* Number of types in vector */
		  $$[1] = $1;
		}
	|	nonempty_typelist ',' type
		{ int len = sizeof (struct type *) * (++($<ivec>1[0]) + 1);
		  $$ = (struct type **) realloc ((char *) $1, len);
		  $$[$<ivec>$[0]] = $3;
		}
	;

name
	:	NAME
		{ $$ = $1.stoken; }
	|	TYPENAME
		{ $$ = $1.stoken; }
	;

name_not_typename :	NAME
/* These would be useful if name_not_typename was useful, but it is just
   a fake for "variable", so these cause reduce/reduce conflicts because
   the parser can't tell whether NAME_OR_INT is a name_not_typename (=variable,
   =exp) or just an exp.  If name_not_typename was ever used in an lvalue
   context where only a name could occur, this might be useful.
  	|	NAME_OR_INT
   */
	;

%%

/* Called to match intrinsic function calls with one argument to their
   respective implementation and push the operation.  */

static void
wrap_unop_intrinsic (exp_opcode code)
{
  switch (code)
    {
    case UNOP_ABS:
      pstate->wrap<fortran_abs_operation> ();
      break;
    case FORTRAN_FLOOR:
      pstate->wrap<fortran_floor_operation_1arg> ();
      break;
    case FORTRAN_CEILING:
      pstate->wrap<fortran_ceil_operation_1arg> ();
      break;
    case UNOP_FORTRAN_ALLOCATED:
      pstate->wrap<fortran_allocated_operation> ();
      break;
    case UNOP_FORTRAN_RANK:
      pstate->wrap<fortran_rank_operation> ();
      break;
    case UNOP_FORTRAN_SHAPE:
      pstate->wrap<fortran_array_shape_operation> ();
      break;
    case UNOP_FORTRAN_LOC:
      pstate->wrap<fortran_loc_operation> ();
      break;
    case FORTRAN_ASSOCIATED:
      pstate->wrap<fortran_associated_1arg> ();
      break;
    case FORTRAN_ARRAY_SIZE:
      pstate->wrap<fortran_array_size_1arg> ();
      break;
    case FORTRAN_CMPLX:
      pstate->wrap<fortran_cmplx_operation_1arg> ();
      break;
    case FORTRAN_LBOUND:
    case FORTRAN_UBOUND:
      pstate->push_new<fortran_bound_1arg> (code, pstate->pop ());
      break;
    default:
      gdb_assert_not_reached ("unhandled intrinsic");
    }
}

/* Called to match intrinsic function calls with two arguments to their
   respective implementation and push the operation.  */

static void
wrap_binop_intrinsic (exp_opcode code)
{
  switch (code)
    {
    case FORTRAN_FLOOR:
      fortran_wrap2_kind<fortran_floor_operation_2arg>
	(parse_f_type (pstate)->builtin_integer);
      break;
    case FORTRAN_CEILING:
      fortran_wrap2_kind<fortran_ceil_operation_2arg>
	(parse_f_type (pstate)->builtin_integer);
      break;
    case BINOP_MOD:
      pstate->wrap2<fortran_mod_operation> ();
      break;
    case BINOP_FORTRAN_MODULO:
      pstate->wrap2<fortran_modulo_operation> ();
      break;
    case FORTRAN_CMPLX:
      pstate->wrap2<fortran_cmplx_operation_2arg> ();
      break;
    case FORTRAN_ASSOCIATED:
      pstate->wrap2<fortran_associated_2arg> ();
      break;
    case FORTRAN_ARRAY_SIZE:
      pstate->wrap2<fortran_array_size_2arg> ();
      break;
    case FORTRAN_LBOUND:
    case FORTRAN_UBOUND:
      {
	operation_up arg2 = pstate->pop ();
	operation_up arg1 = pstate->pop ();
	pstate->push_new<fortran_bound_2arg> (code, std::move (arg1),
					      std::move (arg2));
      }
      break;
    default:
      gdb_assert_not_reached ("unhandled intrinsic");
    }
}

/* Called to match intrinsic function calls with three arguments to their
   respective implementation and push the operation.  */

static void
wrap_ternop_intrinsic (exp_opcode code)
{
  switch (code)
    {
    case FORTRAN_LBOUND:
    case FORTRAN_UBOUND:
      {
	operation_up kind_arg = pstate->pop ();
	operation_up arg2 = pstate->pop ();
	operation_up arg1 = pstate->pop ();

	value *val = kind_arg->evaluate (nullptr, pstate->expout.get (),
					 EVAL_AVOID_SIDE_EFFECTS);
	gdb_assert (val != nullptr);

	type *follow_type
	  = convert_to_kind_type (parse_f_type (pstate)->builtin_integer,
				  value_as_long (val));

	pstate->push_new<fortran_bound_3arg> (code, std::move (arg1),
					      std::move (arg2), follow_type);
      }
      break;
    case FORTRAN_ARRAY_SIZE:
      fortran_wrap3_kind<fortran_array_size_3arg>
	(parse_f_type (pstate)->builtin_integer);
      break;
    case FORTRAN_CMPLX:
      fortran_wrap3_kind<fortran_cmplx_operation_3arg>
	(parse_f_type (pstate)->builtin_complex);
      break;
    default:
      gdb_assert_not_reached ("unhandled intrinsic");
    }
}

/* A helper that pops two operations (similar to wrap2), evaluates the last one
   assuming it is a kind parameter, and wraps them in some other operation
   pushing it to the stack.  */

template<typename T>
static void
fortran_wrap2_kind (type *base_type)
{
  operation_up kind_arg = pstate->pop ();
  operation_up arg = pstate->pop ();

  value *val = kind_arg->evaluate (nullptr, pstate->expout.get (),
				   EVAL_AVOID_SIDE_EFFECTS);
  gdb_assert (val != nullptr);

  type *follow_type = convert_to_kind_type (base_type, value_as_long (val));

  pstate->push_new<T> (std::move (arg), follow_type);
}

/* A helper that pops three operations, evaluates the last one assuming it is a
   kind parameter, and wraps them in some other operation pushing it to the
   stack.  */

template<typename T>
static void
fortran_wrap3_kind (type *base_type)
{
  operation_up kind_arg = pstate->pop ();
  operation_up arg2 = pstate->pop ();
  operation_up arg1 = pstate->pop ();

  value *val = kind_arg->evaluate (nullptr, pstate->expout.get (),
				   EVAL_AVOID_SIDE_EFFECTS);
  gdb_assert (val != nullptr);

  type *follow_type = convert_to_kind_type (base_type, value_as_long (val));

  pstate->push_new<T> (std::move (arg1), std::move (arg2), follow_type);
}

/* Take care of parsing a number (anything that starts with a digit).
   Set yylval and return the token type; update lexptr.
   LEN is the number of characters in it.  */

/*** Needs some error checking for the float case ***/

static int
parse_number (struct parser_state *par_state,
	      const char *p, int len, int parsed_float, YYSTYPE *putithere)
{
  ULONGEST n = 0;
  ULONGEST prevn = 0;
  int c;
  int base = input_radix;
  int unsigned_p = 0;
  int long_p = 0;
  ULONGEST high_bit;
  struct type *signed_type;
  struct type *unsigned_type;

  if (parsed_float)
    {
      /* It's a float since it contains a point or an exponent.  */
      /* [dD] is not understood as an exponent by parse_float,
	 change it to 'e'.  */
      char *tmp, *tmp2;

      tmp = xstrdup (p);
      for (tmp2 = tmp; *tmp2; ++tmp2)
	if (*tmp2 == 'd' || *tmp2 == 'D')
	  *tmp2 = 'e';

      /* FIXME: Should this use different types?  */
      putithere->typed_val_float.type = parse_f_type (pstate)->builtin_real_s8;
      bool parsed = parse_float (tmp, len,
				 putithere->typed_val_float.type,
				 putithere->typed_val_float.val);
      free (tmp);
      return parsed? FLOAT : ERROR;
    }

  /* Handle base-switching prefixes 0x, 0t, 0d, 0 */
  if (p[0] == '0' && len > 1)
    switch (p[1])
      {
      case 'x':
      case 'X':
	if (len >= 3)
	  {
	    p += 2;
	    base = 16;
	    len -= 2;
	  }
	break;
	
      case 't':
      case 'T':
      case 'd':
      case 'D':
	if (len >= 3)
	  {
	    p += 2;
	    base = 10;
	    len -= 2;
	  }
	break;
	
      default:
	base = 8;
	break;
      }
  
  while (len-- > 0)
    {
      c = *p++;
      if (isupper (c))
	c = tolower (c);
      if (len == 0 && c == 'l')
	long_p = 1;
      else if (len == 0 && c == 'u')
	unsigned_p = 1;
      else
	{
	  int i;
	  if (c >= '0' && c <= '9')
	    i = c - '0';
	  else if (c >= 'a' && c <= 'f')
	    i = c - 'a' + 10;
	  else
	    return ERROR;	/* Char not a digit */
	  if (i >= base)
	    return ERROR;		/* Invalid digit in this base */
	  n *= base;
	  n += i;
	}
      /* Test for overflow.  */
      if (prevn == 0 && n == 0)
	;
      else if (RANGE_CHECK && prevn >= n)
	range_error (_("Overflow on numeric constant."));
      prevn = n;
    }
  
  /* If the number is too big to be an int, or it's got an l suffix
     then it's a long.  Work out if this has to be a long by
     shifting right and seeing if anything remains, and the
     target int size is different to the target long size.
     
     In the expression below, we could have tested
     (n >> gdbarch_int_bit (parse_gdbarch))
     to see if it was zero,
     but too many compilers warn about that, when ints and longs
     are the same size.  So we shift it twice, with fewer bits
     each time, for the same result.  */

  int bits_available;
  if ((gdbarch_int_bit (par_state->gdbarch ())
       != gdbarch_long_bit (par_state->gdbarch ())
       && ((n >> 2)
	   >> (gdbarch_int_bit (par_state->gdbarch ())-2))) /* Avoid
							    shift warning */
      || long_p)
    {
      bits_available = gdbarch_long_bit (par_state->gdbarch ());
      unsigned_type = parse_type (par_state)->builtin_unsigned_long;
      signed_type = parse_type (par_state)->builtin_long;
  }
  else 
    {
      bits_available = gdbarch_int_bit (par_state->gdbarch ());
      unsigned_type = parse_type (par_state)->builtin_unsigned_int;
      signed_type = parse_type (par_state)->builtin_int;
    }    
  high_bit = ((ULONGEST)1) << (bits_available - 1);
  
  if (RANGE_CHECK
      && ((n >> 2) >> (bits_available - 2)))
    range_error (_("Overflow on numeric constant."));

  putithere->typed_val.val = n;
  
  /* If the high bit of the worked out type is set then this number
     has to be unsigned.  */
  
  if (unsigned_p || (n & high_bit)) 
    putithere->typed_val.type = unsigned_type;
  else 
    putithere->typed_val.type = signed_type;
  
  return INT;
}

/* Called to setup the type stack when we encounter a '(kind=N)' type
   modifier, performs some bounds checking on 'N' and then pushes this to
   the type stack followed by the 'tp_kind' marker.  */
static void
push_kind_type (LONGEST val, struct type *type)
{
  int ival;

  if (type->is_unsigned ())
    {
      ULONGEST uval = static_cast <ULONGEST> (val);
      if (uval > INT_MAX)
	error (_("kind value out of range"));
      ival = static_cast <int> (uval);
    }
  else
    {
      if (val > INT_MAX || val < 0)
	error (_("kind value out of range"));
      ival = static_cast <int> (val);
    }

  type_stack->push (ival);
  type_stack->push (tp_kind);
}

/* Called when a type has a '(kind=N)' modifier after it, for example
   'character(kind=1)'.  The BASETYPE is the type described by 'character'
   in our example, and KIND is the integer '1'.  This function returns a
   new type that represents the basetype of a specific kind.  */
static struct type *
convert_to_kind_type (struct type *basetype, int kind)
{
  if (basetype == parse_f_type (pstate)->builtin_character)
    {
      /* Character of kind 1 is a special case, this is the same as the
	 base character type.  */
      if (kind == 1)
	return parse_f_type (pstate)->builtin_character;
    }
  else if (basetype == parse_f_type (pstate)->builtin_complex)
    {
      if (kind == 4)
	return parse_f_type (pstate)->builtin_complex;
      else if (kind == 8)
	return parse_f_type (pstate)->builtin_complex_s8;
      else if (kind == 16)
	return parse_f_type (pstate)->builtin_complex_s16;
    }
  else if (basetype == parse_f_type (pstate)->builtin_real)
    {
      if (kind == 4)
	return parse_f_type (pstate)->builtin_real;
      else if (kind == 8)
	return parse_f_type (pstate)->builtin_real_s8;
      else if (kind == 16)
	return parse_f_type (pstate)->builtin_real_s16;
    }
  else if (basetype == parse_f_type (pstate)->builtin_logical)
    {
      if (kind == 1)
	return parse_f_type (pstate)->builtin_logical_s1;
      else if (kind == 2)
	return parse_f_type (pstate)->builtin_logical_s2;
      else if (kind == 4)
	return parse_f_type (pstate)->builtin_logical;
      else if (kind == 8)
	return parse_f_type (pstate)->builtin_logical_s8;
    }
  else if (basetype == parse_f_type (pstate)->builtin_integer)
    {
      if (kind == 1)
	return parse_f_type (pstate)->builtin_integer_s1;
      else if (kind == 2)
	return parse_f_type (pstate)->builtin_integer_s2;
      else if (kind == 4)
	return parse_f_type (pstate)->builtin_integer;
      else if (kind == 8)
	return parse_f_type (pstate)->builtin_integer_s8;
    }

  error (_("unsupported kind %d for type %s"),
	 kind, TYPE_SAFE_NAME (basetype));

  /* Should never get here.  */
  return nullptr;
}

struct f_token
{
  /* The string to match against.  */
  const char *oper;

  /* The lexer token to return.  */
  int token;

  /* The expression opcode to embed within the token.  */
  enum exp_opcode opcode;

  /* When this is true the string in OPER is matched exactly including
     case, when this is false OPER is matched case insensitively.  */
  bool case_sensitive;
};

/* List of Fortran operators.  */

static const struct f_token fortran_operators[] =
{
  { ".and.", BOOL_AND, OP_NULL, false },
  { ".or.", BOOL_OR, OP_NULL, false },
  { ".not.", BOOL_NOT, OP_NULL, false },
  { ".eq.", EQUAL, OP_NULL, false },
  { ".eqv.", EQUAL, OP_NULL, false },
  { ".neqv.", NOTEQUAL, OP_NULL, false },
  { ".xor.", NOTEQUAL, OP_NULL, false },
  { "==", EQUAL, OP_NULL, false },
  { ".ne.", NOTEQUAL, OP_NULL, false },
  { "/=", NOTEQUAL, OP_NULL, false },
  { ".le.", LEQ, OP_NULL, false },
  { "<=", LEQ, OP_NULL, false },
  { ".ge.", GEQ, OP_NULL, false },
  { ">=", GEQ, OP_NULL, false },
  { ".gt.", GREATERTHAN, OP_NULL, false },
  { ">", GREATERTHAN, OP_NULL, false },
  { ".lt.", LESSTHAN, OP_NULL, false },
  { "<", LESSTHAN, OP_NULL, false },
  { "**", STARSTAR, BINOP_EXP, false },
};

/* Holds the Fortran representation of a boolean, and the integer value we
   substitute in when one of the matching strings is parsed.  */
struct f77_boolean_val
{
  /* The string representing a Fortran boolean.  */
  const char *name;

  /* The integer value to replace it with.  */
  int value;
};

/* The set of Fortran booleans.  These are matched case insensitively.  */
static const struct f77_boolean_val boolean_values[]  =
{
  { ".true.", 1 },
  { ".false.", 0 }
};

static const struct f_token f_intrinsics[] =
{
  /* The following correspond to actual functions in Fortran and are case
     insensitive.  */
  { "kind", KIND, OP_NULL, false },
  { "abs", UNOP_INTRINSIC, UNOP_ABS, false },
  { "mod", BINOP_INTRINSIC, BINOP_MOD, false },
  { "floor", UNOP_OR_BINOP_INTRINSIC, FORTRAN_FLOOR, false },
  { "ceiling", UNOP_OR_BINOP_INTRINSIC, FORTRAN_CEILING, false },
  { "modulo", BINOP_INTRINSIC, BINOP_FORTRAN_MODULO, false },
  { "cmplx", UNOP_OR_BINOP_OR_TERNOP_INTRINSIC, FORTRAN_CMPLX, false },
  { "lbound", UNOP_OR_BINOP_OR_TERNOP_INTRINSIC, FORTRAN_LBOUND, false },
  { "ubound", UNOP_OR_BINOP_OR_TERNOP_INTRINSIC, FORTRAN_UBOUND, false },
  { "allocated", UNOP_INTRINSIC, UNOP_FORTRAN_ALLOCATED, false },
  { "associated", UNOP_OR_BINOP_INTRINSIC, FORTRAN_ASSOCIATED, false },
  { "rank", UNOP_INTRINSIC, UNOP_FORTRAN_RANK, false },
  { "size", UNOP_OR_BINOP_OR_TERNOP_INTRINSIC, FORTRAN_ARRAY_SIZE, false },
  { "shape", UNOP_INTRINSIC, UNOP_FORTRAN_SHAPE, false },
  { "loc", UNOP_INTRINSIC, UNOP_FORTRAN_LOC, false },
  { "sizeof", SIZEOF, OP_NULL, false },
};

static const f_token f_keywords[] =
{
  /* Historically these have always been lowercase only in GDB.  */
  { "character", CHARACTER, OP_NULL, true },
  { "complex", COMPLEX_KEYWORD, OP_NULL, true },
  { "complex_4", COMPLEX_S4_KEYWORD, OP_NULL, true },
  { "complex_8", COMPLEX_S8_KEYWORD, OP_NULL, true },
  { "complex_16", COMPLEX_S16_KEYWORD, OP_NULL, true },
  { "integer_1", INT_S1_KEYWORD, OP_NULL, true },
  { "integer_2", INT_S2_KEYWORD, OP_NULL, true },
  { "integer_4", INT_S4_KEYWORD, OP_NULL, true },
  { "integer", INT_KEYWORD, OP_NULL, true },
  { "integer_8", INT_S8_KEYWORD, OP_NULL, true },
  { "logical_1", LOGICAL_S1_KEYWORD, OP_NULL, true },
  { "logical_2", LOGICAL_S2_KEYWORD, OP_NULL, true },
  { "logical", LOGICAL_KEYWORD, OP_NULL, true },
  { "logical_4", LOGICAL_S4_KEYWORD, OP_NULL, true },
  { "logical_8", LOGICAL_S8_KEYWORD, OP_NULL, true },
  { "real", REAL_KEYWORD, OP_NULL, true },
  { "real_4", REAL_S4_KEYWORD, OP_NULL, true },
  { "real_8", REAL_S8_KEYWORD, OP_NULL, true },
  { "real_16", REAL_S16_KEYWORD, OP_NULL, true },
  { "single", SINGLE, OP_NULL, true },
  { "double", DOUBLE, OP_NULL, true },
  { "precision", PRECISION, OP_NULL, true },
};

/* Implementation of a dynamically expandable buffer for processing input
   characters acquired through lexptr and building a value to return in
   yylval.  Ripped off from ch-exp.y */ 

static char *tempbuf;		/* Current buffer contents */
static int tempbufsize;		/* Size of allocated buffer */
static int tempbufindex;	/* Current index into buffer */

#define GROWBY_MIN_SIZE 64	/* Minimum amount to grow buffer by */

#define CHECKBUF(size) \
  do { \
    if (tempbufindex + (size) >= tempbufsize) \
      { \
	growbuf_by_size (size); \
      } \
  } while (0);


/* Grow the static temp buffer if necessary, including allocating the
   first one on demand.  */

static void
growbuf_by_size (int count)
{
  int growby;

  growby = std::max (count, GROWBY_MIN_SIZE);
  tempbufsize += growby;
  if (tempbuf == NULL)
    tempbuf = (char *) malloc (tempbufsize);
  else
    tempbuf = (char *) realloc (tempbuf, tempbufsize);
}

/* Blatantly ripped off from ch-exp.y. This routine recognizes F77 
   string-literals.
   
   Recognize a string literal.  A string literal is a nonzero sequence
   of characters enclosed in matching single quotes, except that
   a single character inside single quotes is a character literal, which
   we reject as a string literal.  To embed the terminator character inside
   a string, it is simply doubled (I.E. 'this''is''one''string') */

static int
match_string_literal (void)
{
  const char *tokptr = pstate->lexptr;

  for (tempbufindex = 0, tokptr++; *tokptr != '\0'; tokptr++)
    {
      CHECKBUF (1);
      if (*tokptr == *pstate->lexptr)
	{
	  if (*(tokptr + 1) == *pstate->lexptr)
	    tokptr++;
	  else
	    break;
	}
      tempbuf[tempbufindex++] = *tokptr;
    }
  if (*tokptr == '\0'					/* no terminator */
      || tempbufindex == 0)				/* no string */
    return 0;
  else
    {
      tempbuf[tempbufindex] = '\0';
      yylval.sval.ptr = tempbuf;
      yylval.sval.length = tempbufindex;
      pstate->lexptr = ++tokptr;
      return STRING_LITERAL;
    }
}

/* This is set if a NAME token appeared at the very end of the input
   string, with no whitespace separating the name from the EOF.  This
   is used only when parsing to do field name completion.  */
static bool saw_name_at_eof;

/* This is set if the previously-returned token was a structure
   operator '%'.  */
static bool last_was_structop;

/* Read one token, getting characters through lexptr.  */

static int
yylex (void)
{
  int c;
  int namelen;
  unsigned int token;
  const char *tokstart;
  bool saw_structop = last_was_structop;

  last_was_structop = false;

 retry:
 
  pstate->prev_lexptr = pstate->lexptr;
 
  tokstart = pstate->lexptr;

  /* First of all, let us make sure we are not dealing with the
     special tokens .true. and .false. which evaluate to 1 and 0.  */

  if (*pstate->lexptr == '.')
    {
      for (const auto &candidate : boolean_values)
	{
	  if (strncasecmp (tokstart, candidate.name,
			   strlen (candidate.name)) == 0)
	    {
	      pstate->lexptr += strlen (candidate.name);
	      yylval.lval = candidate.value;
	      return BOOLEAN_LITERAL;
	    }
	}
    }

  /* See if it is a Fortran operator.  */
  for (const auto &candidate : fortran_operators)
    if (strncasecmp (tokstart, candidate.oper,
		     strlen (candidate.oper)) == 0)
      {
	gdb_assert (!candidate.case_sensitive);
	pstate->lexptr += strlen (candidate.oper);
	yylval.opcode = candidate.opcode;
	return candidate.token;
      }

  switch (c = *tokstart)
    {
    case 0:
      if (saw_name_at_eof)
	{
	  saw_name_at_eof = false;
	  return COMPLETE;
	}
      else if (pstate->parse_completion && saw_structop)
	return COMPLETE;
      return 0;
      
    case ' ':
    case '\t':
    case '\n':
      pstate->lexptr++;
      goto retry;
      
    case '\'':
      token = match_string_literal ();
      if (token != 0)
	return (token);
      break;
      
    case '(':
      paren_depth++;
      pstate->lexptr++;
      return c;
      
    case ')':
      if (paren_depth == 0)
	return 0;
      paren_depth--;
      pstate->lexptr++;
      return c;
      
    case ',':
      if (pstate->comma_terminates && paren_depth == 0)
	return 0;
      pstate->lexptr++;
      return c;
      
    case '.':
      /* Might be a floating point number.  */
      if (pstate->lexptr[1] < '0' || pstate->lexptr[1] > '9')
	goto symbol;		/* Nope, must be a symbol.  */
      [[fallthrough]];
      
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      {
	/* It's a number.  */
	int got_dot = 0, got_e = 0, got_d = 0, toktype;
	const char *p = tokstart;
	int hex = input_radix > 10;
	
	if (c == '0' && (p[1] == 'x' || p[1] == 'X'))
	  {
	    p += 2;
	    hex = 1;
	  }
	else if (c == '0' && (p[1]=='t' || p[1]=='T'
			      || p[1]=='d' || p[1]=='D'))
	  {
	    p += 2;
	    hex = 0;
	  }
	
	for (;; ++p)
	  {
	    if (!hex && !got_e && (*p == 'e' || *p == 'E'))
	      got_dot = got_e = 1;
	    else if (!hex && !got_d && (*p == 'd' || *p == 'D'))
	      got_dot = got_d = 1;
	    else if (!hex && !got_dot && *p == '.')
	      got_dot = 1;
	    else if (((got_e && (p[-1] == 'e' || p[-1] == 'E'))
		     || (got_d && (p[-1] == 'd' || p[-1] == 'D')))
		     && (*p == '-' || *p == '+'))
	      /* This is the sign of the exponent, not the end of the
		 number.  */
	      continue;
	    /* We will take any letters or digits.  parse_number will
	       complain if past the radix, or if L or U are not final.  */
	    else if ((*p < '0' || *p > '9')
		     && ((*p < 'a' || *p > 'z')
			 && (*p < 'A' || *p > 'Z')))
	      break;
	  }
	toktype = parse_number (pstate, tokstart, p - tokstart,
				got_dot|got_e|got_d,
				&yylval);
	if (toktype == ERROR)
	  {
	    char *err_copy = (char *) alloca (p - tokstart + 1);
	    
	    memcpy (err_copy, tokstart, p - tokstart);
	    err_copy[p - tokstart] = 0;
	    error (_("Invalid number \"%s\"."), err_copy);
	  }
	pstate->lexptr = p;
	return toktype;
      }

    case '%':
      last_was_structop = true;
      [[fallthrough]];
    case '+':
    case '-':
    case '*':
    case '/':
    case '|':
    case '&':
    case '^':
    case '~':
    case '!':
    case '@':
    case '<':
    case '>':
    case '[':
    case ']':
    case '?':
    case ':':
    case '=':
    case '{':
    case '}':
    symbol:
      pstate->lexptr++;
      return c;
    }
  
  if (!(c == '_' || c == '$' || c ==':'
	|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
    /* We must have come across a bad character (e.g. ';').  */
    error (_("Invalid character '%c' in expression."), c);
  
  namelen = 0;
  for (c = tokstart[namelen];
       (c == '_' || c == '$' || c == ':' || (c >= '0' && c <= '9')
	|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')); 
       c = tokstart[++namelen]);
  
  /* The token "if" terminates the expression and is NOT 
     removed from the input stream.  */
  
  if (namelen == 2 && tokstart[0] == 'i' && tokstart[1] == 'f')
    return 0;
  
  pstate->lexptr += namelen;
  
  /* Catch specific keywords.  */

  for (const auto &keyword : f_keywords)
    if (strlen (keyword.oper) == namelen
	&& ((!keyword.case_sensitive
	     && strncasecmp (tokstart, keyword.oper, namelen) == 0)
	    || (keyword.case_sensitive
		&& strncmp (tokstart, keyword.oper, namelen) == 0)))
      {
	yylval.opcode = keyword.opcode;
	return keyword.token;
      }

  yylval.sval.ptr = tokstart;
  yylval.sval.length = namelen;
  
  if (*tokstart == '$')
    return DOLLAR_VARIABLE;

  /* Use token-type TYPENAME for symbols that happen to be defined
     currently as names of types; NAME for other symbols.
     The caller is not constrained to care about the distinction.  */
  {
    std::string tmp = copy_name (yylval.sval);
    struct block_symbol result;
    const domain_enum lookup_domains[] =
    {
      STRUCT_DOMAIN,
      VAR_DOMAIN,
      MODULE_DOMAIN
    };
    int hextype;

    for (const auto &domain : lookup_domains)
      {
	result = lookup_symbol (tmp.c_str (), pstate->expression_context_block,
				domain, NULL);
	if (result.symbol && result.symbol->aclass () == LOC_TYPEDEF)
	  {
	    yylval.tsym.type = result.symbol->type ();
	    return TYPENAME;
	  }

	if (result.symbol)
	  break;
      }

    yylval.tsym.type
      = language_lookup_primitive_type (pstate->language (),
					pstate->gdbarch (), tmp.c_str ());
    if (yylval.tsym.type != NULL)
      return TYPENAME;

    /* This is post the symbol search as symbols can hide intrinsics.  Also,
       give Fortran intrinsics priority over C symbols.  This prevents
       non-Fortran symbols from hiding intrinsics, for example abs.  */
    if (!result.symbol || result.symbol->language () != language_fortran)
      for (const auto &intrinsic : f_intrinsics)
	{
	  gdb_assert (!intrinsic.case_sensitive);
	  if (strlen (intrinsic.oper) == namelen
	      && strncasecmp (tokstart, intrinsic.oper, namelen) == 0)
	    {
	      yylval.opcode = intrinsic.opcode;
	      return intrinsic.token;
	    }
	}

    /* Input names that aren't symbols but ARE valid hex numbers,
       when the input radix permits them, can be names or numbers
       depending on the parse.  Note we support radixes > 16 here.  */
    if (!result.symbol
	&& ((tokstart[0] >= 'a' && tokstart[0] < 'a' + input_radix - 10)
	    || (tokstart[0] >= 'A' && tokstart[0] < 'A' + input_radix - 10)))
      {
 	YYSTYPE newlval;	/* Its value is ignored.  */
	hextype = parse_number (pstate, tokstart, namelen, 0, &newlval);
	if (hextype == INT)
	  {
	    yylval.ssym.sym = result;
	    yylval.ssym.is_a_field_of_this = false;
	    return NAME_OR_INT;
	  }
      }

    if (pstate->parse_completion && *pstate->lexptr == '\0')
      saw_name_at_eof = true;

    /* Any other kind of symbol */
    yylval.ssym.sym = result;
    yylval.ssym.is_a_field_of_this = false;
    return NAME;
  }
}

int
f_language::parser (struct parser_state *par_state) const
{
  /* Setting up the parser state.  */
  scoped_restore pstate_restore = make_scoped_restore (&pstate);
  scoped_restore restore_yydebug = make_scoped_restore (&yydebug,
							par_state->debug);
  gdb_assert (par_state != NULL);
  pstate = par_state;
  last_was_structop = false;
  saw_name_at_eof = false;
  paren_depth = 0;

  struct type_stack stack;
  scoped_restore restore_type_stack = make_scoped_restore (&type_stack,
							   &stack);

  int result = yyparse ();
  if (!result)
    pstate->set_operation (pstate->pop ());
  return result;
}

static void
yyerror (const char *msg)
{
  pstate->parse_error (msg);
}
