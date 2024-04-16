/* YACC parser for C expressions, for GDB.
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

/* Parse a C expression from text in a string,
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
#include <ctype.h>
#include "expression.h"
#include "value.h"
#include "parser-defs.h"
#include "language.h"
#include "c-lang.h"
#include "c-support.h"
#include "charset.h"
#include "block.h"
#include "cp-support.h"
#include "macroscope.h"
#include "objc-lang.h"
#include "typeprint.h"
#include "cp-abi.h"
#include "type-stack.h"
#include "target-float.h"
#include "c-exp.h"

#define parse_type(ps) builtin_type (ps->gdbarch ())

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror,
   etc).  */
#define GDB_YY_REMAP_PREFIX c_
#include "yy-remap.h"

/* The state of the parser, used internally when we are parsing the
   expression.  */

static struct parser_state *pstate = NULL;

/* Data that must be held for the duration of a parse.  */

struct c_parse_state
{
  /* These are used to hold type lists and type stacks that are
     allocated during the parse.  */
  std::vector<std::unique_ptr<std::vector<struct type *>>> type_lists;
  std::vector<std::unique_ptr<struct type_stack>> type_stacks;

  /* Storage for some strings allocated during the parse.  */
  std::vector<gdb::unique_xmalloc_ptr<char>> strings;

  /* When we find that lexptr (the global var defined in parse.c) is
     pointing at a macro invocation, we expand the invocation, and call
     scan_macro_expansion to save the old lexptr here and point lexptr
     into the expanded text.  When we reach the end of that, we call
     end_macro_expansion to pop back to the value we saved here.  The
     macro expansion code promises to return only fully-expanded text,
     so we don't need to "push" more than one level.

     This is disgusting, of course.  It would be cleaner to do all macro
     expansion beforehand, and then hand that to lexptr.  But we don't
     really know where the expression ends.  Remember, in a command like

     (gdb) break *ADDRESS if CONDITION

     we evaluate ADDRESS in the scope of the current frame, but we
     evaluate CONDITION in the scope of the breakpoint's location.  So
     it's simply wrong to try to macro-expand the whole thing at once.  */
  const char *macro_original_text = nullptr;

  /* We save all intermediate macro expansions on this obstack for the
     duration of a single parse.  The expansion text may sometimes have
     to live past the end of the expansion, due to yacc lookahead.
     Rather than try to be clever about saving the data for a single
     token, we simply keep it all and delete it after parsing has
     completed.  */
  auto_obstack expansion_obstack;

  /* The type stack.  */
  struct type_stack type_stack;
};

/* This is set and cleared in c_parse.  */

static struct c_parse_state *cpstate;

int yyparse (void);

static int yylex (void);

static void yyerror (const char *);

static int type_aggregate_p (struct type *);

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
    } typed_val_int;
    struct {
      gdb_byte val[16];
      struct type *type;
    } typed_val_float;
    struct type *tval;
    struct stoken sval;
    struct typed_stoken tsval;
    struct ttype tsym;
    struct symtoken ssym;
    int voidval;
    const struct block *bval;
    enum exp_opcode opcode;

    struct stoken_vector svec;
    std::vector<struct type *> *tvec;

    struct type_stack *type_stack;

    struct objc_class_str theclass;
  }

%{
/* YYSTYPE gets defined by %union */
static int parse_number (struct parser_state *par_state,
			 const char *, int, int, YYSTYPE *);
static struct stoken operator_stoken (const char *);
static struct stoken typename_stoken (const char *);
static void check_parameter_typelist (std::vector<struct type *> *);

#if defined(YYBISON) && YYBISON < 30800
static void c_print_token (FILE *file, int type, YYSTYPE value);
#define YYPRINT(FILE, TYPE, VALUE) c_print_token (FILE, TYPE, VALUE)
#endif
%}

%type <voidval> exp exp1 type_exp start variable qualified_name lcurly function_method
%type <lval> rcurly
%type <tval> type typebase scalar_type
%type <tvec> nonempty_typelist func_mod parameter_typelist
/* %type <bval> block */

/* Fancy type parsing.  */
%type <tval> ptype
%type <lval> array_mod
%type <tval> conversion_type_id

%type <type_stack> ptr_operator_ts abs_decl direct_abs_decl

%token <typed_val_int> INT COMPLEX_INT
%token <typed_val_float> FLOAT COMPLEX_FLOAT

/* Both NAME and TYPENAME tokens represent symbols in the input,
   and both convey their data as strings.
   But a TYPENAME is a string that happens to be defined as a typedef
   or builtin type name (such as int or char)
   and a NAME is any other symbol.
   Contexts where this distinction is not important can use the
   nonterminal "name", which matches either NAME or TYPENAME.  */

%token <tsval> STRING
%token <sval> NSSTRING		/* ObjC Foundation "NSString" literal */
%token SELECTOR			/* ObjC "@selector" pseudo-operator   */
%token <tsval> CHAR
%token <ssym> NAME /* BLOCKNAME defined below to give it higher precedence. */
%token <ssym> UNKNOWN_CPP_NAME
%token <voidval> COMPLETE
%token <tsym> TYPENAME
%token <theclass> CLASSNAME	/* ObjC Class name */
%type <sval> name field_name
%type <svec> string_exp
%type <ssym> name_not_typename
%type <tsym> type_name

 /* This is like a '[' token, but is only generated when parsing
    Objective C.  This lets us reuse the same parser without
    erroneously parsing ObjC-specific expressions in C.  */
%token OBJC_LBRAC

/* A NAME_OR_INT is a symbol which is not known in the symbol table,
   but which would parse as a valid number in the current input radix.
   E.g. "c" when input_radix==16.  Depending on the parse, it will be
   turned into a name or into a number.  */

%token <ssym> NAME_OR_INT

%token OPERATOR
%token STRUCT CLASS UNION ENUM SIZEOF ALIGNOF UNSIGNED COLONCOLON
%token TEMPLATE
%token ERROR
%token NEW DELETE
%type <sval> oper
%token REINTERPRET_CAST DYNAMIC_CAST STATIC_CAST CONST_CAST
%token ENTRY
%token TYPEOF
%token DECLTYPE
%token TYPEID

/* Special type cases, put in to allow the parser to distinguish different
   legal basetypes.  */
%token SIGNED_KEYWORD LONG SHORT INT_KEYWORD CONST_KEYWORD VOLATILE_KEYWORD DOUBLE_KEYWORD
%token RESTRICT ATOMIC
%token FLOAT_KEYWORD COMPLEX

%token <sval> DOLLAR_VARIABLE

%token <opcode> ASSIGN_MODIFY

/* C++ */
%token TRUEKEYWORD
%token FALSEKEYWORD


%left ','
%left ABOVE_COMMA
%right '=' ASSIGN_MODIFY
%right '?'
%left OROR
%left ANDAND
%left '|'
%left '^'
%left '&'
%left EQUAL NOTEQUAL
%left '<' '>' LEQ GEQ
%left LSH RSH
%left '@'
%left '+' '-'
%left '*' '/' '%'
%right UNARY INCREMENT DECREMENT
%right ARROW ARROW_STAR '.' DOT_STAR '[' OBJC_LBRAC '('
%token <ssym> BLOCKNAME
%token <bval> FILENAME
%type <bval> block
%left COLONCOLON

%token DOTDOTDOT


%%

start   :	exp1
	|	type_exp
	;

type_exp:	type
			{
			  pstate->push_new<type_operation> ($1);
			}
	|	TYPEOF '(' exp ')'
			{
			  pstate->wrap<typeof_operation> ();
			}
	|	TYPEOF '(' type ')'
			{
			  pstate->push_new<type_operation> ($3);
			}
	|	DECLTYPE '(' exp ')'
			{
			  pstate->wrap<decltype_operation> ();
			}
	;

/* Expressions, including the comma operator.  */
exp1	:	exp
	|	exp1 ',' exp
			{ pstate->wrap2<comma_operation> (); }
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

exp	:	'+' exp    %prec UNARY
			{ pstate->wrap<unary_plus_operation> (); }
	;

exp	:	'!' exp    %prec UNARY
			{
			  if (pstate->language ()->la_language
			      == language_opencl)
			    pstate->wrap<opencl_not_operation> ();
			  else
			    pstate->wrap<unary_logical_not_operation> ();
			}
	;

exp	:	'~' exp    %prec UNARY
			{ pstate->wrap<unary_complement_operation> (); }
	;

exp	:	INCREMENT exp    %prec UNARY
			{ pstate->wrap<preinc_operation> (); }
	;

exp	:	DECREMENT exp    %prec UNARY
			{ pstate->wrap<predec_operation> (); }
	;

exp	:	exp INCREMENT    %prec UNARY
			{ pstate->wrap<postinc_operation> (); }
	;

exp	:	exp DECREMENT    %prec UNARY
			{ pstate->wrap<postdec_operation> (); }
	;

exp	:	TYPEID '(' exp ')' %prec UNARY
			{ pstate->wrap<typeid_operation> (); }
	;

exp	:	TYPEID '(' type_exp ')' %prec UNARY
			{ pstate->wrap<typeid_operation> (); }
	;

exp	:	SIZEOF exp       %prec UNARY
			{ pstate->wrap<unop_sizeof_operation> (); }
	;

exp	:	ALIGNOF '(' type_exp ')'	%prec UNARY
			{ pstate->wrap<unop_alignof_operation> (); }
	;

exp	:	exp ARROW field_name
			{
			  pstate->push_new<structop_ptr_operation>
			    (pstate->pop (), copy_name ($3));
			}
	;

exp	:	exp ARROW field_name COMPLETE
			{
			  structop_base_operation *op
			    = new structop_ptr_operation (pstate->pop (),
							  copy_name ($3));
			  pstate->mark_struct_expression (op);
			  pstate->push (operation_up (op));
			}
	;

exp	:	exp ARROW COMPLETE
			{
			  structop_base_operation *op
			    = new structop_ptr_operation (pstate->pop (), "");
			  pstate->mark_struct_expression (op);
			  pstate->push (operation_up (op));
			}
	;

exp	:	exp ARROW '~' name
			{
			  pstate->push_new<structop_ptr_operation>
			    (pstate->pop (), "~" + copy_name ($4));
			}
	;

exp	:	exp ARROW '~' name COMPLETE
			{
			  structop_base_operation *op
			    = new structop_ptr_operation (pstate->pop (),
							  "~" + copy_name ($4));
			  pstate->mark_struct_expression (op);
			  pstate->push (operation_up (op));
			}
	;

exp	:	exp ARROW qualified_name
			{ /* exp->type::name becomes exp->*(&type::name) */
			  /* Note: this doesn't work if name is a
			     static member!  FIXME */
			  pstate->wrap<unop_addr_operation> ();
			  pstate->wrap2<structop_mptr_operation> (); }
	;

exp	:	exp ARROW_STAR exp
			{ pstate->wrap2<structop_mptr_operation> (); }
	;

exp	:	exp '.' field_name
			{
			  if (pstate->language ()->la_language
			      == language_opencl)
			    pstate->push_new<opencl_structop_operation>
			      (pstate->pop (), copy_name ($3));
			  else
			    pstate->push_new<structop_operation>
			      (pstate->pop (), copy_name ($3));
			}
	;

exp	:	exp '.' field_name COMPLETE
			{
			  structop_base_operation *op
			    = new structop_operation (pstate->pop (),
						      copy_name ($3));
			  pstate->mark_struct_expression (op);
			  pstate->push (operation_up (op));
			}
	;

exp	:	exp '.' COMPLETE
			{
			  structop_base_operation *op
			    = new structop_operation (pstate->pop (), "");
			  pstate->mark_struct_expression (op);
			  pstate->push (operation_up (op));
			}
	;

exp	:	exp '.' '~' name
			{
			  pstate->push_new<structop_operation>
			    (pstate->pop (), "~" + copy_name ($4));
			}
	;

exp	:	exp '.' '~' name COMPLETE
			{
			  structop_base_operation *op
			    = new structop_operation (pstate->pop (),
						      "~" + copy_name ($4));
			  pstate->mark_struct_expression (op);
			  pstate->push (operation_up (op));
			}
	;

exp	:	exp '.' qualified_name
			{ /* exp.type::name becomes exp.*(&type::name) */
			  /* Note: this doesn't work if name is a
			     static member!  FIXME */
			  pstate->wrap<unop_addr_operation> ();
			  pstate->wrap2<structop_member_operation> (); }
	;

exp	:	exp DOT_STAR exp
			{ pstate->wrap2<structop_member_operation> (); }
	;

exp	:	exp '[' exp1 ']'
			{ pstate->wrap2<subscript_operation> (); }
	;

exp	:	exp OBJC_LBRAC exp1 ']'
			{ pstate->wrap2<subscript_operation> (); }
	;

/*
 * The rules below parse ObjC message calls of the form:
 *	'[' target selector {':' argument}* ']'
 */

exp	: 	OBJC_LBRAC TYPENAME
			{
			  CORE_ADDR theclass;

			  std::string copy = copy_name ($2.stoken);
			  theclass = lookup_objc_class (pstate->gdbarch (),
							copy.c_str ());
			  if (theclass == 0)
			    error (_("%s is not an ObjC Class"),
				   copy.c_str ());
			  pstate->push_new<long_const_operation>
			    (parse_type (pstate)->builtin_int,
			     (LONGEST) theclass);
			  start_msglist();
			}
		msglist ']'
			{ end_msglist (pstate); }
	;

exp	:	OBJC_LBRAC CLASSNAME
			{
			  pstate->push_new<long_const_operation>
			    (parse_type (pstate)->builtin_int,
			     (LONGEST) $2.theclass);
			  start_msglist();
			}
		msglist ']'
			{ end_msglist (pstate); }
	;

exp	:	OBJC_LBRAC exp
			{ start_msglist(); }
		msglist ']'
			{ end_msglist (pstate); }
	;

msglist :	name
			{ add_msglist(&$1, 0); }
	|	msgarglist
	;

msgarglist :	msgarg
	|	msgarglist msgarg
	;

msgarg	:	name ':' exp
			{ add_msglist(&$1, 1); }
	|	':' exp	/* Unnamed arg.  */
			{ add_msglist(0, 1);   }
	|	',' exp	/* Variable number of args.  */
			{ add_msglist(0, 0);   }
	;

exp	:	exp '('
			/* This is to save the value of arglist_len
			   being accumulated by an outer function call.  */
			{ pstate->start_arglist (); }
		arglist ')'	%prec ARROW
			{
			  std::vector<operation_up> args
			    = pstate->pop_vector (pstate->end_arglist ());
			  pstate->push_new<funcall_operation>
			    (pstate->pop (), std::move (args));
			}
	;

/* This is here to disambiguate with the production for
   "func()::static_var" further below, which uses
   function_method_void.  */
exp	:	exp '(' ')' %prec ARROW
			{
			  pstate->push_new<funcall_operation>
			    (pstate->pop (), std::vector<operation_up> ());
			}
	;


exp	:	UNKNOWN_CPP_NAME '('
			{
			  /* This could potentially be a an argument defined
			     lookup function (Koenig).  */
			  /* This is to save the value of arglist_len
			     being accumulated by an outer function call.  */
			  pstate->start_arglist ();
			}
		arglist ')'	%prec ARROW
			{
			  std::vector<operation_up> args
			    = pstate->pop_vector (pstate->end_arglist ());
			  pstate->push_new<adl_func_operation>
			    (copy_name ($1.stoken),
			     pstate->expression_context_block,
			     std::move (args));
			}
	;

lcurly	:	'{'
			{ pstate->start_arglist (); }
	;

arglist	:
	;

arglist	:	exp
			{ pstate->arglist_len = 1; }
	;

arglist	:	arglist ',' exp   %prec ABOVE_COMMA
			{ pstate->arglist_len++; }
	;

function_method:       exp '(' parameter_typelist ')' const_or_volatile
			{
			  std::vector<struct type *> *type_list = $3;
			  /* Save the const/volatile qualifiers as
			     recorded by the const_or_volatile
			     production's actions.  */
			  type_instance_flags flags
			    = (cpstate->type_stack
			       .follow_type_instance_flags ());
			  pstate->push_new<type_instance_operation>
			    (flags, std::move (*type_list),
			     pstate->pop ());
			}
	;

function_method_void:	    exp '(' ')' const_or_volatile
		       {
			  type_instance_flags flags
			    = (cpstate->type_stack
			       .follow_type_instance_flags ());
			  pstate->push_new<type_instance_operation>
			    (flags, std::vector<type *> (), pstate->pop ());
		       }
       ;

exp     :       function_method
	;

/* Normally we must interpret "func()" as a function call, instead of
   a type.  The user needs to write func(void) to disambiguate.
   However, in the "func()::static_var" case, there's no
   ambiguity.  */
function_method_void_or_typelist: function_method
	|               function_method_void
	;

exp     :       function_method_void_or_typelist COLONCOLON name
			{
			  pstate->push_new<func_static_var_operation>
			    (pstate->pop (), copy_name ($3));
			}
	;

rcurly	:	'}'
			{ $$ = pstate->end_arglist () - 1; }
	;
exp	:	lcurly arglist rcurly	%prec ARROW
			{
			  std::vector<operation_up> args
			    = pstate->pop_vector ($3 + 1);
			  pstate->push_new<array_operation> (0, $3,
							     std::move (args));
			}
	;

exp	:	lcurly type_exp rcurly exp  %prec UNARY
			{ pstate->wrap2<unop_memval_type_operation> (); }
	;

exp	:	'(' type_exp ')' exp  %prec UNARY
			{
			  if (pstate->language ()->la_language
			      == language_opencl)
			    pstate->wrap2<opencl_cast_type_operation> ();
			  else
			    pstate->wrap2<unop_cast_type_operation> ();
			}
	;

exp	:	'(' exp1 ')'
			{ }
	;

/* Binary operators in order of decreasing precedence.  */

exp	:	exp '@' exp
			{ pstate->wrap2<repeat_operation> (); }
	;

exp	:	exp '*' exp
			{ pstate->wrap2<mul_operation> (); }
	;

exp	:	exp '/' exp
			{ pstate->wrap2<div_operation> (); }
	;

exp	:	exp '%' exp
			{ pstate->wrap2<rem_operation> (); }
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
			{
			  if (pstate->language ()->la_language
			      == language_opencl)
			    pstate->wrap2<opencl_equal_operation> ();
			  else
			    pstate->wrap2<equal_operation> ();
			}
	;

exp	:	exp NOTEQUAL exp
			{
			  if (pstate->language ()->la_language
			      == language_opencl)
			    pstate->wrap2<opencl_notequal_operation> ();
			  else
			    pstate->wrap2<notequal_operation> ();
			}
	;

exp	:	exp LEQ exp
			{
			  if (pstate->language ()->la_language
			      == language_opencl)
			    pstate->wrap2<opencl_leq_operation> ();
			  else
			    pstate->wrap2<leq_operation> ();
			}
	;

exp	:	exp GEQ exp
			{
			  if (pstate->language ()->la_language
			      == language_opencl)
			    pstate->wrap2<opencl_geq_operation> ();
			  else
			    pstate->wrap2<geq_operation> ();
			}
	;

exp	:	exp '<' exp
			{
			  if (pstate->language ()->la_language
			      == language_opencl)
			    pstate->wrap2<opencl_less_operation> ();
			  else
			    pstate->wrap2<less_operation> ();
			}
	;

exp	:	exp '>' exp
			{
			  if (pstate->language ()->la_language
			      == language_opencl)
			    pstate->wrap2<opencl_gtr_operation> ();
			  else
			    pstate->wrap2<gtr_operation> ();
			}
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

exp	:	exp ANDAND exp
			{
			  if (pstate->language ()->la_language
			      == language_opencl)
			    {
			      operation_up rhs = pstate->pop ();
			      operation_up lhs = pstate->pop ();
			      pstate->push_new<opencl_logical_binop_operation>
				(BINOP_LOGICAL_AND, std::move (lhs),
				 std::move (rhs));
			    }
			  else
			    pstate->wrap2<logical_and_operation> ();
			}
	;

exp	:	exp OROR exp
			{
			  if (pstate->language ()->la_language
			      == language_opencl)
			    {
			      operation_up rhs = pstate->pop ();
			      operation_up lhs = pstate->pop ();
			      pstate->push_new<opencl_logical_binop_operation>
				(BINOP_LOGICAL_OR, std::move (lhs),
				 std::move (rhs));
			    }
			  else
			    pstate->wrap2<logical_or_operation> ();
			}
	;

exp	:	exp '?' exp ':' exp	%prec '?'
			{
			  operation_up last = pstate->pop ();
			  operation_up mid = pstate->pop ();
			  operation_up first = pstate->pop ();
			  if (pstate->language ()->la_language
			      == language_opencl)
			    pstate->push_new<opencl_ternop_cond_operation>
			      (std::move (first), std::move (mid),
			       std::move (last));
			  else
			    pstate->push_new<ternop_cond_operation>
			      (std::move (first), std::move (mid),
			       std::move (last));
			}
	;

exp	:	exp '=' exp
			{
			  if (pstate->language ()->la_language
			      == language_opencl)
			    pstate->wrap2<opencl_assign_operation> ();
			  else
			    pstate->wrap2<assign_operation> ();
			}
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

exp	:	COMPLEX_INT
			{
			  operation_up real
			    = (make_operation<long_const_operation>
			       ($1.type->target_type (), 0));
			  operation_up imag
			    = (make_operation<long_const_operation>
			       ($1.type->target_type (), $1.val));
			  pstate->push_new<complex_operation>
			    (std::move (real), std::move (imag), $1.type);
			}
	;

exp	:	CHAR
			{
			  struct stoken_vector vec;
			  vec.len = 1;
			  vec.tokens = &$1;
			  pstate->push_c_string ($1.type, &vec);
			}
	;

exp	:	NAME_OR_INT
			{ YYSTYPE val;
			  parse_number (pstate, $1.stoken.ptr,
					$1.stoken.length, 0, &val);
			  pstate->push_new<long_const_operation>
			    (val.typed_val_int.type,
			     val.typed_val_int.val);
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

exp	:	COMPLEX_FLOAT
			{
			  struct type *underlying = $1.type->target_type ();

			  float_data val;
			  target_float_from_host_double (val.data (),
							 underlying, 0);
			  operation_up real
			    = (make_operation<float_const_operation>
			       (underlying, val));

			  std::copy (std::begin ($1.val), std::end ($1.val),
				     std::begin (val));
			  operation_up imag
			    = (make_operation<float_const_operation>
			       (underlying, val));

			  pstate->push_new<complex_operation>
			    (std::move (real), std::move (imag),
			     $1.type);
			}
	;

exp	:	variable
	;

exp	:	DOLLAR_VARIABLE
			{
			  pstate->push_dollar ($1);
			}
	;

exp	:	SELECTOR '(' name ')'
			{
			  pstate->push_new<objc_selector_operation>
			    (copy_name ($3));
			}
	;

exp	:	SIZEOF '(' type ')'	%prec UNARY
			{ struct type *type = $3;
			  struct type *int_type
			    = lookup_signed_typename (pstate->language (),
						      "int");
			  type = check_typedef (type);

			    /* $5.3.3/2 of the C++ Standard (n3290 draft)
			       says of sizeof:  "When applied to a reference
			       or a reference type, the result is the size of
			       the referenced type."  */
			  if (TYPE_IS_REFERENCE (type))
			    type = check_typedef (type->target_type ());

			  pstate->push_new<long_const_operation>
			    (int_type, type->length ());
			}
	;

exp	:	REINTERPRET_CAST '<' type_exp '>' '(' exp ')' %prec UNARY
			{ pstate->wrap2<reinterpret_cast_operation> (); }
	;

exp	:	STATIC_CAST '<' type_exp '>' '(' exp ')' %prec UNARY
			{ pstate->wrap2<unop_cast_type_operation> (); }
	;

exp	:	DYNAMIC_CAST '<' type_exp '>' '(' exp ')' %prec UNARY
			{ pstate->wrap2<dynamic_cast_operation> (); }
	;

exp	:	CONST_CAST '<' type_exp '>' '(' exp ')' %prec UNARY
			{ /* We could do more error checking here, but
			     it doesn't seem worthwhile.  */
			  pstate->wrap2<unop_cast_type_operation> (); }
	;

string_exp:
		STRING
			{
			  /* We copy the string here, and not in the
			     lexer, to guarantee that we do not leak a
			     string.  Note that we follow the
			     NUL-termination convention of the
			     lexer.  */
			  struct typed_stoken *vec = XNEW (struct typed_stoken);
			  $$.len = 1;
			  $$.tokens = vec;

			  vec->type = $1.type;
			  vec->length = $1.length;
			  vec->ptr = (char *) malloc ($1.length + 1);
			  memcpy (vec->ptr, $1.ptr, $1.length + 1);
			}

	|	string_exp STRING
			{
			  /* Note that we NUL-terminate here, but just
			     for convenience.  */
			  char *p;
			  ++$$.len;
			  $$.tokens = XRESIZEVEC (struct typed_stoken,
						  $$.tokens, $$.len);

			  p = (char *) malloc ($2.length + 1);
			  memcpy (p, $2.ptr, $2.length + 1);

			  $$.tokens[$$.len - 1].type = $2.type;
			  $$.tokens[$$.len - 1].length = $2.length;
			  $$.tokens[$$.len - 1].ptr = p;
			}
		;

exp	:	string_exp
			{
			  int i;
			  c_string_type type = C_STRING;

			  for (i = 0; i < $1.len; ++i)
			    {
			      switch ($1.tokens[i].type)
				{
				case C_STRING:
				  break;
				case C_WIDE_STRING:
				case C_STRING_16:
				case C_STRING_32:
				  if (type != C_STRING
				      && type != $1.tokens[i].type)
				    error (_("Undefined string concatenation."));
				  type = (enum c_string_type_values) $1.tokens[i].type;
				  break;
				default:
				  /* internal error */
				  internal_error ("unrecognized type in string concatenation");
				}
			    }

			  pstate->push_c_string (type, &$1);
			  for (i = 0; i < $1.len; ++i)
			    free ($1.tokens[i].ptr);
			  free ($1.tokens);
			}
	;

exp     :	NSSTRING	/* ObjC NextStep NSString constant
				 * of the form '@' '"' string '"'.
				 */
			{
			  pstate->push_new<objc_nsstring_operation>
			    (copy_name ($1));
			}
	;

/* C++.  */
exp     :       TRUEKEYWORD
			{ pstate->push_new<long_const_operation>
			    (parse_type (pstate)->builtin_bool, 1);
			}
	;

exp     :       FALSEKEYWORD
			{ pstate->push_new<long_const_operation>
			    (parse_type (pstate)->builtin_bool, 0);
			}
	;

/* end of C++.  */

block	:	BLOCKNAME
			{
			  if ($1.sym.symbol)
			    $$ = $1.sym.symbol->value_block ();
			  else
			    error (_("No file or function \"%s\"."),
				   copy_name ($1.stoken).c_str ());
			}
	|	FILENAME
			{
			  $$ = $1;
			}
	;

block	:	block COLONCOLON name
			{
			  std::string copy = copy_name ($3);
			  struct symbol *tem
			    = lookup_symbol (copy.c_str (), $1,
					     VAR_DOMAIN, NULL).symbol;

			  if (!tem || tem->aclass () != LOC_BLOCK)
			    error (_("No function \"%s\" in specified context."),
				   copy.c_str ());
			  $$ = tem->value_block (); }
	;

variable:	name_not_typename ENTRY
			{ struct symbol *sym = $1.sym.symbol;

			  if (sym == NULL || !sym->is_argument ()
			      || !symbol_read_needs_frame (sym))
			    error (_("@entry can be used only for function "
				     "parameters, not for \"%s\""),
				   copy_name ($1.stoken).c_str ());

			  pstate->push_new<var_entry_value_operation> (sym);
			}
	;

variable:	block COLONCOLON name
			{
			  std::string copy = copy_name ($3);
			  struct block_symbol sym
			    = lookup_symbol (copy.c_str (), $1,
					     VAR_DOMAIN, NULL);

			  if (sym.symbol == 0)
			    error (_("No symbol \"%s\" in specified context."),
				   copy.c_str ());
			  if (symbol_read_needs_frame (sym.symbol))
			    pstate->block_tracker->update (sym);

			  pstate->push_new<var_value_operation> (sym);
			}
	;

qualified_name:	TYPENAME COLONCOLON name
			{
			  struct type *type = $1.type;
			  type = check_typedef (type);
			  if (!type_aggregate_p (type))
			    error (_("`%s' is not defined as an aggregate type."),
				   TYPE_SAFE_NAME (type));

			  pstate->push_new<scope_operation> (type,
							     copy_name ($3));
			}
	|	TYPENAME COLONCOLON '~' name
			{
			  struct type *type = $1.type;

			  type = check_typedef (type);
			  if (!type_aggregate_p (type))
			    error (_("`%s' is not defined as an aggregate type."),
				   TYPE_SAFE_NAME (type));
			  std::string name = "~" + std::string ($4.ptr,
								$4.length);

			  /* Check for valid destructor name.  */
			  destructor_name_p (name.c_str (), $1.type);
			  pstate->push_new<scope_operation> (type,
							     std::move (name));
			}
	|	TYPENAME COLONCOLON name COLONCOLON name
			{
			  std::string copy = copy_name ($3);
			  error (_("No type \"%s\" within class "
				   "or namespace \"%s\"."),
				 copy.c_str (), TYPE_SAFE_NAME ($1.type));
			}
	;

variable:	qualified_name
	|	COLONCOLON name_not_typename
			{
			  std::string name = copy_name ($2.stoken);
			  struct block_symbol sym
			    = lookup_symbol (name.c_str (),
					     (const struct block *) NULL,
					     VAR_DOMAIN, NULL);
			  pstate->push_symbol (name.c_str (), sym);
			}
	;

variable:	name_not_typename
			{ struct block_symbol sym = $1.sym;

			  if (sym.symbol)
			    {
			      if (symbol_read_needs_frame (sym.symbol))
				pstate->block_tracker->update (sym);

			      /* If we found a function, see if it's
				 an ifunc resolver that has the same
				 address as the ifunc symbol itself.
				 If so, prefer the ifunc symbol.  */

			      bound_minimal_symbol resolver
				= find_gnu_ifunc (sym.symbol);
			      if (resolver.minsym != NULL)
				pstate->push_new<var_msym_value_operation>
				  (resolver);
			      else
				pstate->push_new<var_value_operation> (sym);
			    }
			  else if ($1.is_a_field_of_this)
			    {
			      /* C++: it hangs off of `this'.  Must
				 not inadvertently convert from a method call
				 to data ref.  */
			      pstate->block_tracker->update (sym);
			      operation_up thisop
				= make_operation<op_this_operation> ();
			      pstate->push_new<structop_ptr_operation>
				(std::move (thisop), copy_name ($1.stoken));
			    }
			  else
			    {
			      std::string arg = copy_name ($1.stoken);

			      bound_minimal_symbol msymbol
				= lookup_bound_minimal_symbol (arg.c_str ());
			      if (msymbol.minsym == NULL)
				{
				  if (!have_full_symbols () && !have_partial_symbols ())
				    error (_("No symbol table is loaded.  Use the \"file\" command."));
				  else
				    error (_("No symbol \"%s\" in current context."),
					   arg.c_str ());
				}

			      /* This minsym might be an alias for
				 another function.  See if we can find
				 the debug symbol for the target, and
				 if so, use it instead, since it has
				 return type / prototype info.  This
				 is important for example for "p
				 *__errno_location()".  */
			      symbol *alias_target
				= ((msymbol.minsym->type () != mst_text_gnu_ifunc
				    && msymbol.minsym->type () != mst_data_gnu_ifunc)
				   ? find_function_alias_target (msymbol)
				   : NULL);
			      if (alias_target != NULL)
				{
				  block_symbol bsym { alias_target,
				    alias_target->value_block () };
				  pstate->push_new<var_value_operation> (bsym);
				}
			      else
				pstate->push_new<var_msym_value_operation>
				  (msymbol);
			    }
			}
	;

const_or_volatile: const_or_volatile_noopt
	|
	;

single_qualifier:
		CONST_KEYWORD
			{ cpstate->type_stack.insert (tp_const); }
	| 	VOLATILE_KEYWORD
			{ cpstate->type_stack.insert (tp_volatile); }
	| 	ATOMIC
			{ cpstate->type_stack.insert (tp_atomic); }
	| 	RESTRICT
			{ cpstate->type_stack.insert (tp_restrict); }
	|	'@' NAME
		{
		  cpstate->type_stack.insert (pstate,
					      copy_name ($2.stoken).c_str ());
		}
	|	'@' UNKNOWN_CPP_NAME
		{
		  cpstate->type_stack.insert (pstate,
					      copy_name ($2.stoken).c_str ());
		}
	;

qualifier_seq_noopt:
		single_qualifier
	| 	qualifier_seq_noopt single_qualifier
	;

qualifier_seq:
		qualifier_seq_noopt
	|
	;

ptr_operator:
		ptr_operator '*'
			{ cpstate->type_stack.insert (tp_pointer); }
		qualifier_seq
	|	'*'
			{ cpstate->type_stack.insert (tp_pointer); }
		qualifier_seq
	|	'&'
			{ cpstate->type_stack.insert (tp_reference); }
	|	'&' ptr_operator
			{ cpstate->type_stack.insert (tp_reference); }
	|       ANDAND
			{ cpstate->type_stack.insert (tp_rvalue_reference); }
	|       ANDAND ptr_operator
			{ cpstate->type_stack.insert (tp_rvalue_reference); }
	;

ptr_operator_ts: ptr_operator
			{
			  $$ = cpstate->type_stack.create ();
			  cpstate->type_stacks.emplace_back ($$);
			}
	;

abs_decl:	ptr_operator_ts direct_abs_decl
			{ $$ = $2->append ($1); }
	|	ptr_operator_ts
	|	direct_abs_decl
	;

direct_abs_decl: '(' abs_decl ')'
			{ $$ = $2; }
	|	direct_abs_decl array_mod
			{
			  cpstate->type_stack.push ($1);
			  cpstate->type_stack.push ($2);
			  cpstate->type_stack.push (tp_array);
			  $$ = cpstate->type_stack.create ();
			  cpstate->type_stacks.emplace_back ($$);
			}
	|	array_mod
			{
			  cpstate->type_stack.push ($1);
			  cpstate->type_stack.push (tp_array);
			  $$ = cpstate->type_stack.create ();
			  cpstate->type_stacks.emplace_back ($$);
			}

	| 	direct_abs_decl func_mod
			{
			  cpstate->type_stack.push ($1);
			  cpstate->type_stack.push ($2);
			  $$ = cpstate->type_stack.create ();
			  cpstate->type_stacks.emplace_back ($$);
			}
	|	func_mod
			{
			  cpstate->type_stack.push ($1);
			  $$ = cpstate->type_stack.create ();
			  cpstate->type_stacks.emplace_back ($$);
			}
	;

array_mod:	'[' ']'
			{ $$ = -1; }
	|	OBJC_LBRAC ']'
			{ $$ = -1; }
	|	'[' INT ']'
			{ $$ = $2.val; }
	|	OBJC_LBRAC INT ']'
			{ $$ = $2.val; }
	;

func_mod:	'(' ')'
			{
			  $$ = new std::vector<struct type *>;
			  cpstate->type_lists.emplace_back ($$);
			}
	|	'(' parameter_typelist ')'
			{ $$ = $2; }
	;

/* We used to try to recognize pointer to member types here, but
   that didn't work (shift/reduce conflicts meant that these rules never
   got executed).  The problem is that
     int (foo::bar::baz::bizzle)
   is a function type but
     int (foo::bar::baz::bizzle::*)
   is a pointer to member type.  Stroustrup loses again!  */

type	:	ptype
	;

/* A helper production that recognizes scalar types that can validly
   be used with _Complex.  */

scalar_type:
		INT_KEYWORD
			{ $$ = lookup_signed_typename (pstate->language (),
						       "int"); }
	|	LONG
			{ $$ = lookup_signed_typename (pstate->language (),
						       "long"); }
	|	SHORT
			{ $$ = lookup_signed_typename (pstate->language (),
						       "short"); }
	|	LONG INT_KEYWORD
			{ $$ = lookup_signed_typename (pstate->language (),
						       "long"); }
	|	LONG SIGNED_KEYWORD INT_KEYWORD
			{ $$ = lookup_signed_typename (pstate->language (),
						       "long"); }
	|	LONG SIGNED_KEYWORD
			{ $$ = lookup_signed_typename (pstate->language (),
						       "long"); }
	|	SIGNED_KEYWORD LONG INT_KEYWORD
			{ $$ = lookup_signed_typename (pstate->language (),
						       "long"); }
	|	UNSIGNED LONG INT_KEYWORD
			{ $$ = lookup_unsigned_typename (pstate->language (),
							 "long"); }
	|	LONG UNSIGNED INT_KEYWORD
			{ $$ = lookup_unsigned_typename (pstate->language (),
							 "long"); }
	|	LONG UNSIGNED
			{ $$ = lookup_unsigned_typename (pstate->language (),
							 "long"); }
	|	LONG LONG
			{ $$ = lookup_signed_typename (pstate->language (),
						       "long long"); }
	|	LONG LONG INT_KEYWORD
			{ $$ = lookup_signed_typename (pstate->language (),
						       "long long"); }
	|	LONG LONG SIGNED_KEYWORD INT_KEYWORD
			{ $$ = lookup_signed_typename (pstate->language (),
						       "long long"); }
	|	LONG LONG SIGNED_KEYWORD
			{ $$ = lookup_signed_typename (pstate->language (),
						       "long long"); }
	|	SIGNED_KEYWORD LONG LONG
			{ $$ = lookup_signed_typename (pstate->language (),
						       "long long"); }
	|	SIGNED_KEYWORD LONG LONG INT_KEYWORD
			{ $$ = lookup_signed_typename (pstate->language (),
						       "long long"); }
	|	UNSIGNED LONG LONG
			{ $$ = lookup_unsigned_typename (pstate->language (),
							 "long long"); }
	|	UNSIGNED LONG LONG INT_KEYWORD
			{ $$ = lookup_unsigned_typename (pstate->language (),
							 "long long"); }
	|	LONG LONG UNSIGNED
			{ $$ = lookup_unsigned_typename (pstate->language (),
							 "long long"); }
	|	LONG LONG UNSIGNED INT_KEYWORD
			{ $$ = lookup_unsigned_typename (pstate->language (),
							 "long long"); }
	|	SHORT INT_KEYWORD
			{ $$ = lookup_signed_typename (pstate->language (),
						       "short"); }
	|	SHORT SIGNED_KEYWORD INT_KEYWORD
			{ $$ = lookup_signed_typename (pstate->language (),
						       "short"); }
	|	SHORT SIGNED_KEYWORD
			{ $$ = lookup_signed_typename (pstate->language (),
						       "short"); }
	|	UNSIGNED SHORT INT_KEYWORD
			{ $$ = lookup_unsigned_typename (pstate->language (),
							 "short"); }
	|	SHORT UNSIGNED
			{ $$ = lookup_unsigned_typename (pstate->language (),
							 "short"); }
	|	SHORT UNSIGNED INT_KEYWORD
			{ $$ = lookup_unsigned_typename (pstate->language (),
							 "short"); }
	|	DOUBLE_KEYWORD
			{ $$ = lookup_typename (pstate->language (),
						"double",
						NULL,
						0); }
	|	FLOAT_KEYWORD
			{ $$ = lookup_typename (pstate->language (),
						"float",
						NULL,
						0); }
	|	LONG DOUBLE_KEYWORD
			{ $$ = lookup_typename (pstate->language (),
						"long double",
						NULL,
						0); }
	|	UNSIGNED type_name
			{ $$ = lookup_unsigned_typename (pstate->language (),
							 $2.type->name ()); }
	|	UNSIGNED
			{ $$ = lookup_unsigned_typename (pstate->language (),
							 "int"); }
	|	SIGNED_KEYWORD type_name
			{ $$ = lookup_signed_typename (pstate->language (),
						       $2.type->name ()); }
	|	SIGNED_KEYWORD
			{ $$ = lookup_signed_typename (pstate->language (),
						       "int"); }
	;

/* Implements (approximately): (type-qualifier)* type-specifier.

   When type-specifier is only ever a single word, like 'float' then these
   arrive as pre-built TYPENAME tokens thanks to the classify_name
   function.  However, when a type-specifier can contain multiple words,
   for example 'double' can appear as just 'double' or 'long double', and
   similarly 'long' can appear as just 'long' or in 'long double', then
   these type-specifiers are parsed into their own tokens in the function
   lex_one_token and the ident_tokens array.  These separate tokens are all
   recognised here.  */
typebase
	:	TYPENAME
			{ $$ = $1.type; }
	|	scalar_type
			{ $$ = $1; }
	|	COMPLEX scalar_type
			{
			  $$ = init_complex_type (nullptr, $2);
			}
	|	STRUCT name
			{ $$
			    = lookup_struct (copy_name ($2).c_str (),
					     pstate->expression_context_block);
			}
	|	STRUCT COMPLETE
			{
			  pstate->mark_completion_tag (TYPE_CODE_STRUCT,
						       "", 0);
			  $$ = NULL;
			}
	|	STRUCT name COMPLETE
			{
			  pstate->mark_completion_tag (TYPE_CODE_STRUCT,
						       $2.ptr, $2.length);
			  $$ = NULL;
			}
	|	CLASS name
			{ $$ = lookup_struct
			    (copy_name ($2).c_str (),
			     pstate->expression_context_block);
			}
	|	CLASS COMPLETE
			{
			  pstate->mark_completion_tag (TYPE_CODE_STRUCT,
						       "", 0);
			  $$ = NULL;
			}
	|	CLASS name COMPLETE
			{
			  pstate->mark_completion_tag (TYPE_CODE_STRUCT,
						       $2.ptr, $2.length);
			  $$ = NULL;
			}
	|	UNION name
			{ $$
			    = lookup_union (copy_name ($2).c_str (),
					    pstate->expression_context_block);
			}
	|	UNION COMPLETE
			{
			  pstate->mark_completion_tag (TYPE_CODE_UNION,
						       "", 0);
			  $$ = NULL;
			}
	|	UNION name COMPLETE
			{
			  pstate->mark_completion_tag (TYPE_CODE_UNION,
						       $2.ptr, $2.length);
			  $$ = NULL;
			}
	|	ENUM name
			{ $$ = lookup_enum (copy_name ($2).c_str (),
					    pstate->expression_context_block);
			}
	|	ENUM COMPLETE
			{
			  pstate->mark_completion_tag (TYPE_CODE_ENUM, "", 0);
			  $$ = NULL;
			}
	|	ENUM name COMPLETE
			{
			  pstate->mark_completion_tag (TYPE_CODE_ENUM, $2.ptr,
						       $2.length);
			  $$ = NULL;
			}
		/* It appears that this rule for templates is never
		   reduced; template recognition happens by lookahead
		   in the token processing code in yylex. */
	|	TEMPLATE name '<' type '>'
			{ $$ = lookup_template_type
			    (copy_name($2).c_str (), $4,
			     pstate->expression_context_block);
			}
	|	qualifier_seq_noopt typebase
			{ $$ = cpstate->type_stack.follow_types ($2); }
	|	typebase qualifier_seq_noopt
			{ $$ = cpstate->type_stack.follow_types ($1); }
	;

type_name:	TYPENAME
	|	INT_KEYWORD
		{
		  $$.stoken.ptr = "int";
		  $$.stoken.length = 3;
		  $$.type = lookup_signed_typename (pstate->language (),
						    "int");
		}
	|	LONG
		{
		  $$.stoken.ptr = "long";
		  $$.stoken.length = 4;
		  $$.type = lookup_signed_typename (pstate->language (),
						    "long");
		}
	|	SHORT
		{
		  $$.stoken.ptr = "short";
		  $$.stoken.length = 5;
		  $$.type = lookup_signed_typename (pstate->language (),
						    "short");
		}
	;

parameter_typelist:
		nonempty_typelist
			{ check_parameter_typelist ($1); }
	|	nonempty_typelist ',' DOTDOTDOT
			{
			  $1->push_back (NULL);
			  check_parameter_typelist ($1);
			  $$ = $1;
			}
	;

nonempty_typelist
	:	type
		{
		  std::vector<struct type *> *typelist
		    = new std::vector<struct type *>;
		  cpstate->type_lists.emplace_back (typelist);

		  typelist->push_back ($1);
		  $$ = typelist;
		}
	|	nonempty_typelist ',' type
		{
		  $1->push_back ($3);
		  $$ = $1;
		}
	;

ptype	:	typebase
	|	ptype abs_decl
		{
		  cpstate->type_stack.push ($2);
		  $$ = cpstate->type_stack.follow_types ($1);
		}
	;

conversion_type_id: typebase conversion_declarator
		{ $$ = cpstate->type_stack.follow_types ($1); }
	;

conversion_declarator:  /* Nothing.  */
	| ptr_operator conversion_declarator
	;

const_and_volatile: 	CONST_KEYWORD VOLATILE_KEYWORD
	| 		VOLATILE_KEYWORD CONST_KEYWORD
	;

const_or_volatile_noopt:  	const_and_volatile
			{ cpstate->type_stack.insert (tp_const);
			  cpstate->type_stack.insert (tp_volatile);
			}
	| 		CONST_KEYWORD
			{ cpstate->type_stack.insert (tp_const); }
	| 		VOLATILE_KEYWORD
			{ cpstate->type_stack.insert (tp_volatile); }
	;

oper:	OPERATOR NEW
			{ $$ = operator_stoken (" new"); }
	|	OPERATOR DELETE
			{ $$ = operator_stoken (" delete"); }
	|	OPERATOR NEW '[' ']'
			{ $$ = operator_stoken (" new[]"); }
	|	OPERATOR DELETE '[' ']'
			{ $$ = operator_stoken (" delete[]"); }
	|	OPERATOR NEW OBJC_LBRAC ']'
			{ $$ = operator_stoken (" new[]"); }
	|	OPERATOR DELETE OBJC_LBRAC ']'
			{ $$ = operator_stoken (" delete[]"); }
	|	OPERATOR '+'
			{ $$ = operator_stoken ("+"); }
	|	OPERATOR '-'
			{ $$ = operator_stoken ("-"); }
	|	OPERATOR '*'
			{ $$ = operator_stoken ("*"); }
	|	OPERATOR '/'
			{ $$ = operator_stoken ("/"); }
	|	OPERATOR '%'
			{ $$ = operator_stoken ("%"); }
	|	OPERATOR '^'
			{ $$ = operator_stoken ("^"); }
	|	OPERATOR '&'
			{ $$ = operator_stoken ("&"); }
	|	OPERATOR '|'
			{ $$ = operator_stoken ("|"); }
	|	OPERATOR '~'
			{ $$ = operator_stoken ("~"); }
	|	OPERATOR '!'
			{ $$ = operator_stoken ("!"); }
	|	OPERATOR '='
			{ $$ = operator_stoken ("="); }
	|	OPERATOR '<'
			{ $$ = operator_stoken ("<"); }
	|	OPERATOR '>'
			{ $$ = operator_stoken (">"); }
	|	OPERATOR ASSIGN_MODIFY
			{ const char *op = " unknown";
			  switch ($2)
			    {
			    case BINOP_RSH:
			      op = ">>=";
			      break;
			    case BINOP_LSH:
			      op = "<<=";
			      break;
			    case BINOP_ADD:
			      op = "+=";
			      break;
			    case BINOP_SUB:
			      op = "-=";
			      break;
			    case BINOP_MUL:
			      op = "*=";
			      break;
			    case BINOP_DIV:
			      op = "/=";
			      break;
			    case BINOP_REM:
			      op = "%=";
			      break;
			    case BINOP_BITWISE_IOR:
			      op = "|=";
			      break;
			    case BINOP_BITWISE_AND:
			      op = "&=";
			      break;
			    case BINOP_BITWISE_XOR:
			      op = "^=";
			      break;
			    default:
			      break;
			    }

			  $$ = operator_stoken (op);
			}
	|	OPERATOR LSH
			{ $$ = operator_stoken ("<<"); }
	|	OPERATOR RSH
			{ $$ = operator_stoken (">>"); }
	|	OPERATOR EQUAL
			{ $$ = operator_stoken ("=="); }
	|	OPERATOR NOTEQUAL
			{ $$ = operator_stoken ("!="); }
	|	OPERATOR LEQ
			{ $$ = operator_stoken ("<="); }
	|	OPERATOR GEQ
			{ $$ = operator_stoken (">="); }
	|	OPERATOR ANDAND
			{ $$ = operator_stoken ("&&"); }
	|	OPERATOR OROR
			{ $$ = operator_stoken ("||"); }
	|	OPERATOR INCREMENT
			{ $$ = operator_stoken ("++"); }
	|	OPERATOR DECREMENT
			{ $$ = operator_stoken ("--"); }
	|	OPERATOR ','
			{ $$ = operator_stoken (","); }
	|	OPERATOR ARROW_STAR
			{ $$ = operator_stoken ("->*"); }
	|	OPERATOR ARROW
			{ $$ = operator_stoken ("->"); }
	|	OPERATOR '(' ')'
			{ $$ = operator_stoken ("()"); }
	|	OPERATOR '[' ']'
			{ $$ = operator_stoken ("[]"); }
	|	OPERATOR OBJC_LBRAC ']'
			{ $$ = operator_stoken ("[]"); }
	|	OPERATOR conversion_type_id
			{
			  string_file buf;
			  c_print_type ($2, NULL, &buf, -1, 0,
					pstate->language ()->la_language,
					&type_print_raw_options);
			  std::string name = buf.release ();

			  /* This also needs canonicalization.  */
			  gdb::unique_xmalloc_ptr<char> canon
			    = cp_canonicalize_string (name.c_str ());
			  if (canon != nullptr)
			    name = canon.get ();
			  $$ = operator_stoken ((" " + name).c_str ());
			}
	;

/* This rule exists in order to allow some tokens that would not normally
   match the 'name' rule to appear as fields within a struct.  The example
   that initially motivated this was the RISC-V target which models the
   floating point registers as a union with fields called 'float' and
   'double'.  */
field_name
	:	name
	|	DOUBLE_KEYWORD { $$ = typename_stoken ("double"); }
	|	FLOAT_KEYWORD { $$ = typename_stoken ("float"); }
	|	INT_KEYWORD { $$ = typename_stoken ("int"); }
	|	LONG { $$ = typename_stoken ("long"); }
	|	SHORT { $$ = typename_stoken ("short"); }
	|	SIGNED_KEYWORD { $$ = typename_stoken ("signed"); }
	|	UNSIGNED { $$ = typename_stoken ("unsigned"); }
	;

name	:	NAME { $$ = $1.stoken; }
	|	BLOCKNAME { $$ = $1.stoken; }
	|	TYPENAME { $$ = $1.stoken; }
	|	NAME_OR_INT  { $$ = $1.stoken; }
	|	UNKNOWN_CPP_NAME  { $$ = $1.stoken; }
	|	oper { $$ = $1; }
	;

name_not_typename :	NAME
	|	BLOCKNAME
/* These would be useful if name_not_typename was useful, but it is just
   a fake for "variable", so these cause reduce/reduce conflicts because
   the parser can't tell whether NAME_OR_INT is a name_not_typename (=variable,
   =exp) or just an exp.  If name_not_typename was ever used in an lvalue
   context where only a name could occur, this might be useful.
  	|	NAME_OR_INT
 */
	|	oper
			{
			  struct field_of_this_result is_a_field_of_this;

			  $$.stoken = $1;
			  $$.sym
			    = lookup_symbol ($1.ptr,
					     pstate->expression_context_block,
					     VAR_DOMAIN,
					     &is_a_field_of_this);
			  $$.is_a_field_of_this
			    = is_a_field_of_this.type != NULL;
			}
	|	UNKNOWN_CPP_NAME
	;

%%

/* Returns a stoken of the operator name given by OP (which does not
   include the string "operator").  */

static struct stoken
operator_stoken (const char *op)
{
  struct stoken st = { NULL, 0 };
  char *buf;

  st.length = CP_OPERATOR_LEN + strlen (op);
  buf = (char *) malloc (st.length + 1);
  strcpy (buf, CP_OPERATOR_STR);
  strcat (buf, op);
  st.ptr = buf;

  /* The toplevel (c_parse) will free the memory allocated here.  */
  cpstate->strings.emplace_back (buf);
  return st;
};

/* Returns a stoken of the type named TYPE.  */

static struct stoken
typename_stoken (const char *type)
{
  struct stoken st = { type, 0 };
  st.length = strlen (type);
  return st;
};

/* Return true if the type is aggregate-like.  */

static int
type_aggregate_p (struct type *type)
{
  return (type->code () == TYPE_CODE_STRUCT
	  || type->code () == TYPE_CODE_UNION
	  || type->code () == TYPE_CODE_NAMESPACE
	  || (type->code () == TYPE_CODE_ENUM
	      && type->is_declared_class ()));
}

/* Validate a parameter typelist.  */

static void
check_parameter_typelist (std::vector<struct type *> *params)
{
  struct type *type;
  int ix;

  for (ix = 0; ix < params->size (); ++ix)
    {
      type = (*params)[ix];
      if (type != NULL && check_typedef (type)->code () == TYPE_CODE_VOID)
	{
	  if (ix == 0)
	    {
	      if (params->size () == 1)
		{
		  /* Ok.  */
		  break;
		}
	      error (_("parameter types following 'void'"));
	    }
	  else
	    error (_("'void' invalid as parameter type"));
	}
    }
}

/* Take care of parsing a number (anything that starts with a digit).
   Set yylval and return the token type; update lexptr.
   LEN is the number of characters in it.  */

/*** Needs some error checking for the float case ***/

static int
parse_number (struct parser_state *par_state,
	      const char *buf, int len, int parsed_float, YYSTYPE *putithere)
{
  ULONGEST n = 0;
  ULONGEST prevn = 0;

  int i = 0;
  int c;
  int base = input_radix;
  int unsigned_p = 0;

  /* Number of "L" suffixes encountered.  */
  int long_p = 0;

  /* Imaginary number.  */
  bool imaginary_p = false;

  /* We have found a "L" or "U" (or "i") suffix.  */
  int found_suffix = 0;

  char *p;

  p = (char *) alloca (len);
  memcpy (p, buf, len);

  if (parsed_float)
    {
      if (len >= 1 && p[len - 1] == 'i')
	{
	  imaginary_p = true;
	  --len;
	}

      /* Handle suffixes for decimal floating-point: "df", "dd" or "dl".  */
      if (len >= 2 && p[len - 2] == 'd' && p[len - 1] == 'f')
	{
	  putithere->typed_val_float.type
	    = parse_type (par_state)->builtin_decfloat;
	  len -= 2;
	}
      else if (len >= 2 && p[len - 2] == 'd' && p[len - 1] == 'd')
	{
	  putithere->typed_val_float.type
	    = parse_type (par_state)->builtin_decdouble;
	  len -= 2;
	}
      else if (len >= 2 && p[len - 2] == 'd' && p[len - 1] == 'l')
	{
	  putithere->typed_val_float.type
	    = parse_type (par_state)->builtin_declong;
	  len -= 2;
	}
      /* Handle suffixes: 'f' for float, 'l' for long double.  */
      else if (len >= 1 && TOLOWER (p[len - 1]) == 'f')
	{
	  putithere->typed_val_float.type
	    = parse_type (par_state)->builtin_float;
	  len -= 1;
	}
      else if (len >= 1 && TOLOWER (p[len - 1]) == 'l')
	{
	  putithere->typed_val_float.type
	    = parse_type (par_state)->builtin_long_double;
	  len -= 1;
	}
      /* Default type for floating-point literals is double.  */
      else
	{
	  putithere->typed_val_float.type
	    = parse_type (par_state)->builtin_double;
	}

      if (!parse_float (p, len,
			putithere->typed_val_float.type,
			putithere->typed_val_float.val))
	return ERROR;

      if (imaginary_p)
	putithere->typed_val_float.type
	  = init_complex_type (nullptr, putithere->typed_val_float.type);

      return imaginary_p ? COMPLEX_FLOAT : FLOAT;
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

      case 'b':
      case 'B':
	if (len >= 3)
	  {
	    p += 2;
	    base = 2;
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
      if (c >= 'A' && c <= 'Z')
	c += 'a' - 'A';
      if (c != 'l' && c != 'u' && c != 'i')
	n *= base;
      if (c >= '0' && c <= '9')
	{
	  if (found_suffix)
	    return ERROR;
	  n += i = c - '0';
	}
      else
	{
	  if (base > 10 && c >= 'a' && c <= 'f')
	    {
	      if (found_suffix)
		return ERROR;
	      n += i = c - 'a' + 10;
	    }
	  else if (c == 'l')
	    {
	      ++long_p;
	      found_suffix = 1;
	    }
	  else if (c == 'u')
	    {
	      unsigned_p = 1;
	      found_suffix = 1;
	    }
	  else if (c == 'i')
	    {
	      imaginary_p = true;
	      found_suffix = 1;
	    }
	  else
	    return ERROR;	/* Char not a digit */
	}
      if (i >= base)
	return ERROR;		/* Invalid digit in this base */

      if (c != 'l' && c != 'u' && c != 'i')
	{
	  /* Test for overflow.  */
	  if (prevn == 0 && n == 0)
	    ;
	  else if (prevn >= n)
	    error (_("Numeric constant too large."));
	}
      prevn = n;
    }

  /* An integer constant is an int, a long, or a long long.  An L
     suffix forces it to be long; an LL suffix forces it to be long
     long.  If not forced to a larger size, it gets the first type of
     the above that it fits in.  To figure out whether it fits, we
     shift it right and see whether anything remains.  Note that we
     can't shift sizeof (LONGEST) * HOST_CHAR_BIT bits or more in one
     operation, because many compilers will warn about such a shift
     (which always produces a zero result).  Sometimes gdbarch_int_bit
     or gdbarch_long_bit will be that big, sometimes not.  To deal with
     the case where it is we just always shift the value more than
     once, with fewer bits each time.  */
  int int_bits = gdbarch_int_bit (par_state->gdbarch ());
  int long_bits = gdbarch_long_bit (par_state->gdbarch ());
  int long_long_bits = gdbarch_long_long_bit (par_state->gdbarch ());
  bool have_signed
    /* No 'u' suffix.  */
    = !unsigned_p;
  bool have_unsigned
    = ((/* 'u' suffix.  */
	unsigned_p)
       || (/* Not a decimal.  */
	   base != 10)
       || (/* Allowed as a convenience, in case decimal doesn't fit in largest
	      signed type.  */
	   !fits_in_type (1, n, long_long_bits, true)));
  bool have_int
    /* No 'l' or 'll' suffix.  */
    = long_p == 0;
  bool have_long
    /* No 'll' suffix.  */
    = long_p <= 1;
  if (have_int && have_signed && fits_in_type (1, n, int_bits, true))
    putithere->typed_val_int.type = parse_type (par_state)->builtin_int;
  else if (have_int && have_unsigned && fits_in_type (1, n, int_bits, false))
    putithere->typed_val_int.type
      = parse_type (par_state)->builtin_unsigned_int;
  else if (have_long && have_signed && fits_in_type (1, n, long_bits, true))
    putithere->typed_val_int.type = parse_type (par_state)->builtin_long;
  else if (have_long && have_unsigned && fits_in_type (1, n, long_bits, false))
    putithere->typed_val_int.type
      = parse_type (par_state)->builtin_unsigned_long;
  else if (have_signed && fits_in_type (1, n, long_long_bits, true))
    putithere->typed_val_int.type
      = parse_type (par_state)->builtin_long_long;
  else if (have_unsigned && fits_in_type (1, n, long_long_bits, false))
    putithere->typed_val_int.type
      = parse_type (par_state)->builtin_unsigned_long_long;
  else
    error (_("Numeric constant too large."));
  putithere->typed_val_int.val = n;

   if (imaginary_p)
     putithere->typed_val_int.type
       = init_complex_type (nullptr, putithere->typed_val_int.type);

   return imaginary_p ? COMPLEX_INT : INT;
}

/* Temporary obstack used for holding strings.  */
static struct obstack tempbuf;
static int tempbuf_init;

/* Parse a C escape sequence.  The initial backslash of the sequence
   is at (*PTR)[-1].  *PTR will be updated to point to just after the
   last character of the sequence.  If OUTPUT is not NULL, the
   translated form of the escape sequence will be written there.  If
   OUTPUT is NULL, no output is written and the call will only affect
   *PTR.  If an escape sequence is expressed in target bytes, then the
   entire sequence will simply be copied to OUTPUT.  Return 1 if any
   character was emitted, 0 otherwise.  */

int
c_parse_escape (const char **ptr, struct obstack *output)
{
  const char *tokptr = *ptr;
  int result = 1;

  /* Some escape sequences undergo character set conversion.  Those we
     translate here.  */
  switch (*tokptr)
    {
      /* Hex escapes do not undergo character set conversion, so keep
	 the escape sequence for later.  */
    case 'x':
      if (output)
	obstack_grow_str (output, "\\x");
      ++tokptr;
      if (!ISXDIGIT (*tokptr))
	error (_("\\x escape without a following hex digit"));
      while (ISXDIGIT (*tokptr))
	{
	  if (output)
	    obstack_1grow (output, *tokptr);
	  ++tokptr;
	}
      break;

      /* Octal escapes do not undergo character set conversion, so
	 keep the escape sequence for later.  */
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
      {
	int i;
	if (output)
	  obstack_grow_str (output, "\\");
	for (i = 0;
	     i < 3 && ISDIGIT (*tokptr) && *tokptr != '8' && *tokptr != '9';
	     ++i)
	  {
	    if (output)
	      obstack_1grow (output, *tokptr);
	    ++tokptr;
	  }
      }
      break;

      /* We handle UCNs later.  We could handle them here, but that
	 would mean a spurious error in the case where the UCN could
	 be converted to the target charset but not the host
	 charset.  */
    case 'u':
    case 'U':
      {
	char c = *tokptr;
	int i, len = c == 'U' ? 8 : 4;
	if (output)
	  {
	    obstack_1grow (output, '\\');
	    obstack_1grow (output, *tokptr);
	  }
	++tokptr;
	if (!ISXDIGIT (*tokptr))
	  error (_("\\%c escape without a following hex digit"), c);
	for (i = 0; i < len && ISXDIGIT (*tokptr); ++i)
	  {
	    if (output)
	      obstack_1grow (output, *tokptr);
	    ++tokptr;
	  }
      }
      break;

      /* We must pass backslash through so that it does not
	 cause quoting during the second expansion.  */
    case '\\':
      if (output)
	obstack_grow_str (output, "\\\\");
      ++tokptr;
      break;

      /* Escapes which undergo conversion.  */
    case 'a':
      if (output)
	obstack_1grow (output, '\a');
      ++tokptr;
      break;
    case 'b':
      if (output)
	obstack_1grow (output, '\b');
      ++tokptr;
      break;
    case 'f':
      if (output)
	obstack_1grow (output, '\f');
      ++tokptr;
      break;
    case 'n':
      if (output)
	obstack_1grow (output, '\n');
      ++tokptr;
      break;
    case 'r':
      if (output)
	obstack_1grow (output, '\r');
      ++tokptr;
      break;
    case 't':
      if (output)
	obstack_1grow (output, '\t');
      ++tokptr;
      break;
    case 'v':
      if (output)
	obstack_1grow (output, '\v');
      ++tokptr;
      break;

      /* GCC extension.  */
    case 'e':
      if (output)
	obstack_1grow (output, HOST_ESCAPE_CHAR);
      ++tokptr;
      break;

      /* Backslash-newline expands to nothing at all.  */
    case '\n':
      ++tokptr;
      result = 0;
      break;

      /* A few escapes just expand to the character itself.  */
    case '\'':
    case '\"':
    case '?':
      /* GCC extensions.  */
    case '(':
    case '{':
    case '[':
    case '%':
      /* Unrecognized escapes turn into the character itself.  */
    default:
      if (output)
	obstack_1grow (output, *tokptr);
      ++tokptr;
      break;
    }
  *ptr = tokptr;
  return result;
}

/* Parse a string or character literal from TOKPTR.  The string or
   character may be wide or unicode.  *OUTPTR is set to just after the
   end of the literal in the input string.  The resulting token is
   stored in VALUE.  This returns a token value, either STRING or
   CHAR, depending on what was parsed.  *HOST_CHARS is set to the
   number of host characters in the literal.  */

static int
parse_string_or_char (const char *tokptr, const char **outptr,
		      struct typed_stoken *value, int *host_chars)
{
  int quote;
  c_string_type type;
  int is_objc = 0;

  /* Build the gdb internal form of the input string in tempbuf.  Note
     that the buffer is null byte terminated *only* for the
     convenience of debugging gdb itself and printing the buffer
     contents when the buffer contains no embedded nulls.  Gdb does
     not depend upon the buffer being null byte terminated, it uses
     the length string instead.  This allows gdb to handle C strings
     (as well as strings in other languages) with embedded null
     bytes */

  if (!tempbuf_init)
    tempbuf_init = 1;
  else
    obstack_free (&tempbuf, NULL);
  obstack_init (&tempbuf);

  /* Record the string type.  */
  if (*tokptr == 'L')
    {
      type = C_WIDE_STRING;
      ++tokptr;
    }
  else if (*tokptr == 'u')
    {
      type = C_STRING_16;
      ++tokptr;
    }
  else if (*tokptr == 'U')
    {
      type = C_STRING_32;
      ++tokptr;
    }
  else if (*tokptr == '@')
    {
      /* An Objective C string.  */
      is_objc = 1;
      type = C_STRING;
      ++tokptr;
    }
  else
    type = C_STRING;

  /* Skip the quote.  */
  quote = *tokptr;
  if (quote == '\'')
    type |= C_CHAR;
  ++tokptr;

  *host_chars = 0;

  while (*tokptr)
    {
      char c = *tokptr;
      if (c == '\\')
	{
	  ++tokptr;
	  *host_chars += c_parse_escape (&tokptr, &tempbuf);
	}
      else if (c == quote)
	break;
      else
	{
	  obstack_1grow (&tempbuf, c);
	  ++tokptr;
	  /* FIXME: this does the wrong thing with multi-byte host
	     characters.  We could use mbrlen here, but that would
	     make "set host-charset" a bit less useful.  */
	  ++*host_chars;
	}
    }

  if (*tokptr != quote)
    {
      if (quote == '"')
	error (_("Unterminated string in expression."));
      else
	error (_("Unmatched single quote."));
    }
  ++tokptr;

  value->type = type;
  value->ptr = (char *) obstack_base (&tempbuf);
  value->length = obstack_object_size (&tempbuf);

  *outptr = tokptr;

  return quote == '"' ? (is_objc ? NSSTRING : STRING) : CHAR;
}

/* This is used to associate some attributes with a token.  */

enum token_flag
{
  /* If this bit is set, the token is C++-only.  */

  FLAG_CXX = 1,

  /* If this bit is set, the token is C-only.  */

  FLAG_C = 2,

  /* If this bit is set, the token is conditional: if there is a
     symbol of the same name, then the token is a symbol; otherwise,
     the token is a keyword.  */

  FLAG_SHADOW = 4
};
DEF_ENUM_FLAGS_TYPE (enum token_flag, token_flags);

struct c_token
{
  const char *oper;
  int token;
  enum exp_opcode opcode;
  token_flags flags;
};

static const struct c_token tokentab3[] =
  {
    {">>=", ASSIGN_MODIFY, BINOP_RSH, 0},
    {"<<=", ASSIGN_MODIFY, BINOP_LSH, 0},
    {"->*", ARROW_STAR, OP_NULL, FLAG_CXX},
    {"...", DOTDOTDOT, OP_NULL, 0}
  };

static const struct c_token tokentab2[] =
  {
    {"+=", ASSIGN_MODIFY, BINOP_ADD, 0},
    {"-=", ASSIGN_MODIFY, BINOP_SUB, 0},
    {"*=", ASSIGN_MODIFY, BINOP_MUL, 0},
    {"/=", ASSIGN_MODIFY, BINOP_DIV, 0},
    {"%=", ASSIGN_MODIFY, BINOP_REM, 0},
    {"|=", ASSIGN_MODIFY, BINOP_BITWISE_IOR, 0},
    {"&=", ASSIGN_MODIFY, BINOP_BITWISE_AND, 0},
    {"^=", ASSIGN_MODIFY, BINOP_BITWISE_XOR, 0},
    {"++", INCREMENT, OP_NULL, 0},
    {"--", DECREMENT, OP_NULL, 0},
    {"->", ARROW, OP_NULL, 0},
    {"&&", ANDAND, OP_NULL, 0},
    {"||", OROR, OP_NULL, 0},
    /* "::" is *not* only C++: gdb overrides its meaning in several
       different ways, e.g., 'filename'::func, function::variable.  */
    {"::", COLONCOLON, OP_NULL, 0},
    {"<<", LSH, OP_NULL, 0},
    {">>", RSH, OP_NULL, 0},
    {"==", EQUAL, OP_NULL, 0},
    {"!=", NOTEQUAL, OP_NULL, 0},
    {"<=", LEQ, OP_NULL, 0},
    {">=", GEQ, OP_NULL, 0},
    {".*", DOT_STAR, OP_NULL, FLAG_CXX}
  };

/* Identifier-like tokens.  Only type-specifiers than can appear in
   multi-word type names (for example 'double' can appear in 'long
   double') need to be listed here.  type-specifiers that are only ever
   single word (like 'char') are handled by the classify_name function.  */
static const struct c_token ident_tokens[] =
  {
    {"unsigned", UNSIGNED, OP_NULL, 0},
    {"template", TEMPLATE, OP_NULL, FLAG_CXX},
    {"volatile", VOLATILE_KEYWORD, OP_NULL, 0},
    {"struct", STRUCT, OP_NULL, 0},
    {"signed", SIGNED_KEYWORD, OP_NULL, 0},
    {"sizeof", SIZEOF, OP_NULL, 0},
    {"_Alignof", ALIGNOF, OP_NULL, 0},
    {"alignof", ALIGNOF, OP_NULL, FLAG_CXX},
    {"double", DOUBLE_KEYWORD, OP_NULL, 0},
    {"float", FLOAT_KEYWORD, OP_NULL, 0},
    {"false", FALSEKEYWORD, OP_NULL, FLAG_CXX},
    {"class", CLASS, OP_NULL, FLAG_CXX},
    {"union", UNION, OP_NULL, 0},
    {"short", SHORT, OP_NULL, 0},
    {"const", CONST_KEYWORD, OP_NULL, 0},
    {"restrict", RESTRICT, OP_NULL, FLAG_C | FLAG_SHADOW},
    {"__restrict__", RESTRICT, OP_NULL, 0},
    {"__restrict", RESTRICT, OP_NULL, 0},
    {"_Atomic", ATOMIC, OP_NULL, 0},
    {"enum", ENUM, OP_NULL, 0},
    {"long", LONG, OP_NULL, 0},
    {"_Complex", COMPLEX, OP_NULL, 0},
    {"__complex__", COMPLEX, OP_NULL, 0},

    {"true", TRUEKEYWORD, OP_NULL, FLAG_CXX},
    {"int", INT_KEYWORD, OP_NULL, 0},
    {"new", NEW, OP_NULL, FLAG_CXX},
    {"delete", DELETE, OP_NULL, FLAG_CXX},
    {"operator", OPERATOR, OP_NULL, FLAG_CXX},

    {"and", ANDAND, OP_NULL, FLAG_CXX},
    {"and_eq", ASSIGN_MODIFY, BINOP_BITWISE_AND, FLAG_CXX},
    {"bitand", '&', OP_NULL, FLAG_CXX},
    {"bitor", '|', OP_NULL, FLAG_CXX},
    {"compl", '~', OP_NULL, FLAG_CXX},
    {"not", '!', OP_NULL, FLAG_CXX},
    {"not_eq", NOTEQUAL, OP_NULL, FLAG_CXX},
    {"or", OROR, OP_NULL, FLAG_CXX},
    {"or_eq", ASSIGN_MODIFY, BINOP_BITWISE_IOR, FLAG_CXX},
    {"xor", '^', OP_NULL, FLAG_CXX},
    {"xor_eq", ASSIGN_MODIFY, BINOP_BITWISE_XOR, FLAG_CXX},

    {"const_cast", CONST_CAST, OP_NULL, FLAG_CXX },
    {"dynamic_cast", DYNAMIC_CAST, OP_NULL, FLAG_CXX },
    {"static_cast", STATIC_CAST, OP_NULL, FLAG_CXX },
    {"reinterpret_cast", REINTERPRET_CAST, OP_NULL, FLAG_CXX },

    {"__typeof__", TYPEOF, OP_TYPEOF, 0 },
    {"__typeof", TYPEOF, OP_TYPEOF, 0 },
    {"typeof", TYPEOF, OP_TYPEOF, FLAG_SHADOW },
    {"__decltype", DECLTYPE, OP_DECLTYPE, FLAG_CXX },
    {"decltype", DECLTYPE, OP_DECLTYPE, FLAG_CXX | FLAG_SHADOW },

    {"typeid", TYPEID, OP_TYPEID, FLAG_CXX}
  };


static void
scan_macro_expansion (const char *expansion)
{
  /* We'd better not be trying to push the stack twice.  */
  gdb_assert (! cpstate->macro_original_text);

  /* Copy to the obstack.  */
  const char *copy = obstack_strdup (&cpstate->expansion_obstack, expansion);

  /* Save the old lexptr value, so we can return to it when we're done
     parsing the expanded text.  */
  cpstate->macro_original_text = pstate->lexptr;
  pstate->lexptr = copy;
}

static int
scanning_macro_expansion (void)
{
  return cpstate->macro_original_text != 0;
}

static void
finished_macro_expansion (void)
{
  /* There'd better be something to pop back to.  */
  gdb_assert (cpstate->macro_original_text);

  /* Pop back to the original text.  */
  pstate->lexptr = cpstate->macro_original_text;
  cpstate->macro_original_text = 0;
}

/* Return true iff the token represents a C++ cast operator.  */

static int
is_cast_operator (const char *token, int len)
{
  return (! strncmp (token, "dynamic_cast", len)
	  || ! strncmp (token, "static_cast", len)
	  || ! strncmp (token, "reinterpret_cast", len)
	  || ! strncmp (token, "const_cast", len));
}

/* The scope used for macro expansion.  */
static struct macro_scope *expression_macro_scope;

/* This is set if a NAME token appeared at the very end of the input
   string, with no whitespace separating the name from the EOF.  This
   is used only when parsing to do field name completion.  */
static int saw_name_at_eof;

/* This is set if the previously-returned token was a structure
   operator -- either '.' or ARROW.  */
static bool last_was_structop;

/* Depth of parentheses.  */
static int paren_depth;

/* Read one token, getting characters through lexptr.  */

static int
lex_one_token (struct parser_state *par_state, bool *is_quoted_name)
{
  int c;
  int namelen;
  const char *tokstart;
  bool saw_structop = last_was_structop;

  last_was_structop = false;
  *is_quoted_name = false;

 retry:

  /* Check if this is a macro invocation that we need to expand.  */
  if (! scanning_macro_expansion ())
    {
      gdb::unique_xmalloc_ptr<char> expanded
	= macro_expand_next (&pstate->lexptr, *expression_macro_scope);

      if (expanded != nullptr)
	scan_macro_expansion (expanded.get ());
    }

  pstate->prev_lexptr = pstate->lexptr;

  tokstart = pstate->lexptr;
  /* See if it is a special token of length 3.  */
  for (const auto &token : tokentab3)
    if (strncmp (tokstart, token.oper, 3) == 0)
      {
	if ((token.flags & FLAG_CXX) != 0
	    && par_state->language ()->la_language != language_cplus)
	  break;
	gdb_assert ((token.flags & FLAG_C) == 0);

	pstate->lexptr += 3;
	yylval.opcode = token.opcode;
	return token.token;
      }

  /* See if it is a special token of length 2.  */
  for (const auto &token : tokentab2)
    if (strncmp (tokstart, token.oper, 2) == 0)
      {
	if ((token.flags & FLAG_CXX) != 0
	    && par_state->language ()->la_language != language_cplus)
	  break;
	gdb_assert ((token.flags & FLAG_C) == 0);

	pstate->lexptr += 2;
	yylval.opcode = token.opcode;
	if (token.token == ARROW)
	  last_was_structop = 1;
	return token.token;
      }

  switch (c = *tokstart)
    {
    case 0:
      /* If we were just scanning the result of a macro expansion,
	 then we need to resume scanning the original text.
	 If we're parsing for field name completion, and the previous
	 token allows such completion, return a COMPLETE token.
	 Otherwise, we were already scanning the original text, and
	 we're really done.  */
      if (scanning_macro_expansion ())
	{
	  finished_macro_expansion ();
	  goto retry;
	}
      else if (saw_name_at_eof)
	{
	  saw_name_at_eof = 0;
	  return COMPLETE;
	}
      else if (par_state->parse_completion && saw_structop)
	return COMPLETE;
      else
	return 0;

    case ' ':
    case '\t':
    case '\n':
      pstate->lexptr++;
      goto retry;

    case '[':
    case '(':
      paren_depth++;
      pstate->lexptr++;
      if (par_state->language ()->la_language == language_objc
	  && c == '[')
	return OBJC_LBRAC;
      return c;

    case ']':
    case ')':
      if (paren_depth == 0)
	return 0;
      paren_depth--;
      pstate->lexptr++;
      return c;

    case ',':
      if (pstate->comma_terminates
	  && paren_depth == 0
	  && ! scanning_macro_expansion ())
	return 0;
      pstate->lexptr++;
      return c;

    case '.':
      /* Might be a floating point number.  */
      if (pstate->lexptr[1] < '0' || pstate->lexptr[1] > '9')
	{
	  last_was_structop = true;
	  goto symbol;		/* Nope, must be a symbol. */
	}
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
	int got_dot = 0, got_e = 0, got_p = 0, toktype;
	const char *p = tokstart;
	int hex = input_radix > 10;

	if (c == '0' && (p[1] == 'x' || p[1] == 'X'))
	  {
	    p += 2;
	    hex = 1;
	  }
	else if (c == '0' && (p[1]=='t' || p[1]=='T' || p[1]=='d' || p[1]=='D'))
	  {
	    p += 2;
	    hex = 0;
	  }

	for (;; ++p)
	  {
	    /* This test includes !hex because 'e' is a valid hex digit
	       and thus does not indicate a floating point number when
	       the radix is hex.  */
	    if (!hex && !got_e && !got_p && (*p == 'e' || *p == 'E'))
	      got_dot = got_e = 1;
	    else if (!got_e && !got_p && (*p == 'p' || *p == 'P'))
	      got_dot = got_p = 1;
	    /* This test does not include !hex, because a '.' always indicates
	       a decimal floating point number regardless of the radix.  */
	    else if (!got_dot && *p == '.')
	      got_dot = 1;
	    else if (((got_e && (p[-1] == 'e' || p[-1] == 'E'))
		      || (got_p && (p[-1] == 'p' || p[-1] == 'P')))
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
	toktype = parse_number (par_state, tokstart, p - tokstart,
				got_dot | got_e | got_p, &yylval);
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

    case '@':
      {
	const char *p = &tokstart[1];

	if (par_state->language ()->la_language == language_objc)
	  {
	    size_t len = strlen ("selector");

	    if (strncmp (p, "selector", len) == 0
		&& (p[len] == '\0' || ISSPACE (p[len])))
	      {
		pstate->lexptr = p + len;
		return SELECTOR;
	      }
	    else if (*p == '"')
	      goto parse_string;
	  }

	while (ISSPACE (*p))
	  p++;
	size_t len = strlen ("entry");
	if (strncmp (p, "entry", len) == 0 && !c_ident_is_alnum (p[len])
	    && p[len] != '_')
	  {
	    pstate->lexptr = &p[len];
	    return ENTRY;
	  }
      }
      [[fallthrough]];
    case '+':
    case '-':
    case '*':
    case '/':
    case '%':
    case '|':
    case '&':
    case '^':
    case '~':
    case '!':
    case '<':
    case '>':
    case '?':
    case ':':
    case '=':
    case '{':
    case '}':
    symbol:
      pstate->lexptr++;
      return c;

    case 'L':
    case 'u':
    case 'U':
      if (tokstart[1] != '"' && tokstart[1] != '\'')
	break;
      [[fallthrough]];
    case '\'':
    case '"':

    parse_string:
      {
	int host_len;
	int result = parse_string_or_char (tokstart, &pstate->lexptr,
					   &yylval.tsval, &host_len);
	if (result == CHAR)
	  {
	    if (host_len == 0)
	      error (_("Empty character constant."));
	    else if (host_len > 2 && c == '\'')
	      {
		++tokstart;
		namelen = pstate->lexptr - tokstart - 1;
		*is_quoted_name = true;

		goto tryname;
	      }
	    else if (host_len > 1)
	      error (_("Invalid character constant."));
	  }
	return result;
      }
    }

  if (!(c == '_' || c == '$' || c_ident_is_alpha (c)))
    /* We must have come across a bad character (e.g. ';').  */
    error (_("Invalid character '%c' in expression."), c);

  /* It's a name.  See how long it is.  */
  namelen = 0;
  for (c = tokstart[namelen];
       (c == '_' || c == '$' || c_ident_is_alnum (c) || c == '<');)
    {
      /* Template parameter lists are part of the name.
	 FIXME: This mishandles `print $a<4&&$a>3'.  */

      if (c == '<')
	{
	  if (! is_cast_operator (tokstart, namelen))
	    {
	      /* Scan ahead to get rest of the template specification.  Note
		 that we look ahead only when the '<' adjoins non-whitespace
		 characters; for comparison expressions, e.g. "a < b > c",
		 there must be spaces before the '<', etc. */
	      const char *p = find_template_name_end (tokstart + namelen);

	      if (p)
		namelen = p - tokstart;
	    }
	  break;
	}
      c = tokstart[++namelen];
    }

  /* The token "if" terminates the expression and is NOT removed from
     the input stream.  It doesn't count if it appears in the
     expansion of a macro.  */
  if (namelen == 2
      && tokstart[0] == 'i'
      && tokstart[1] == 'f'
      && ! scanning_macro_expansion ())
    {
      return 0;
    }

  /* For the same reason (breakpoint conditions), "thread N"
     terminates the expression.  "thread" could be an identifier, but
     an identifier is never followed by a number without intervening
     punctuation.  "task" is similar.  Handle abbreviations of these,
     similarly to breakpoint.c:find_condition_and_thread.  */
  if (namelen >= 1
      && (strncmp (tokstart, "thread", namelen) == 0
	  || strncmp (tokstart, "task", namelen) == 0)
      && (tokstart[namelen] == ' ' || tokstart[namelen] == '\t')
      && ! scanning_macro_expansion ())
    {
      const char *p = tokstart + namelen + 1;

      while (*p == ' ' || *p == '\t')
	p++;
      if (*p >= '0' && *p <= '9')
	return 0;
    }

  pstate->lexptr += namelen;

  tryname:

  yylval.sval.ptr = tokstart;
  yylval.sval.length = namelen;

  /* Catch specific keywords.  */
  std::string copy = copy_name (yylval.sval);
  for (const auto &token : ident_tokens)
    if (copy == token.oper)
      {
	if ((token.flags & FLAG_CXX) != 0
	    && par_state->language ()->la_language != language_cplus)
	  break;
	if ((token.flags & FLAG_C) != 0
	    && par_state->language ()->la_language != language_c
	    && par_state->language ()->la_language != language_objc)
	  break;

	if ((token.flags & FLAG_SHADOW) != 0)
	  {
	    struct field_of_this_result is_a_field_of_this;

	    if (lookup_symbol (copy.c_str (),
			       pstate->expression_context_block,
			       VAR_DOMAIN,
			       (par_state->language ()->la_language
				== language_cplus ? &is_a_field_of_this
				: NULL)).symbol
		!= NULL)
	      {
		/* The keyword is shadowed.  */
		break;
	      }
	  }

	/* It is ok to always set this, even though we don't always
	   strictly need to.  */
	yylval.opcode = token.opcode;
	return token.token;
      }

  if (*tokstart == '$')
    return DOLLAR_VARIABLE;

  if (pstate->parse_completion && *pstate->lexptr == '\0')
    saw_name_at_eof = 1;

  yylval.ssym.stoken = yylval.sval;
  yylval.ssym.sym.symbol = NULL;
  yylval.ssym.sym.block = NULL;
  yylval.ssym.is_a_field_of_this = 0;
  return NAME;
}

/* An object of this type is pushed on a FIFO by the "outer" lexer.  */
struct c_token_and_value
{
  int token;
  YYSTYPE value;
};

/* A FIFO of tokens that have been read but not yet returned to the
   parser.  */
static std::vector<c_token_and_value> token_fifo;

/* Non-zero if the lexer should return tokens from the FIFO.  */
static int popping;

/* Temporary storage for c_lex; this holds symbol names as they are
   built up.  */
static auto_obstack name_obstack;

/* Classify a NAME token.  The contents of the token are in `yylval'.
   Updates yylval and returns the new token type.  BLOCK is the block
   in which lookups start; this can be NULL to mean the global scope.
   IS_QUOTED_NAME is non-zero if the name token was originally quoted
   in single quotes.  IS_AFTER_STRUCTOP is true if this name follows
   a structure operator -- either '.' or ARROW  */

static int
classify_name (struct parser_state *par_state, const struct block *block,
	       bool is_quoted_name, bool is_after_structop)
{
  struct block_symbol bsym;
  struct field_of_this_result is_a_field_of_this;

  std::string copy = copy_name (yylval.sval);

  /* Initialize this in case we *don't* use it in this call; that way
     we can refer to it unconditionally below.  */
  memset (&is_a_field_of_this, 0, sizeof (is_a_field_of_this));

  bsym = lookup_symbol (copy.c_str (), block, VAR_DOMAIN,
			par_state->language ()->name_of_this ()
			? &is_a_field_of_this : NULL);

  if (bsym.symbol && bsym.symbol->aclass () == LOC_BLOCK)
    {
      yylval.ssym.sym = bsym;
      yylval.ssym.is_a_field_of_this = is_a_field_of_this.type != NULL;
      return BLOCKNAME;
    }
  else if (!bsym.symbol)
    {
      /* If we found a field of 'this', we might have erroneously
	 found a constructor where we wanted a type name.  Handle this
	 case by noticing that we found a constructor and then look up
	 the type tag instead.  */
      if (is_a_field_of_this.type != NULL
	  && is_a_field_of_this.fn_field != NULL
	  && TYPE_FN_FIELD_CONSTRUCTOR (is_a_field_of_this.fn_field->fn_fields,
					0))
	{
	  struct field_of_this_result inner_is_a_field_of_this;

	  bsym = lookup_symbol (copy.c_str (), block, STRUCT_DOMAIN,
				&inner_is_a_field_of_this);
	  if (bsym.symbol != NULL)
	    {
	      yylval.tsym.type = bsym.symbol->type ();
	      return TYPENAME;
	    }
	}

      /* If we found a field on the "this" object, or we are looking
	 up a field on a struct, then we want to prefer it over a
	 filename.  However, if the name was quoted, then it is better
	 to check for a filename or a block, since this is the only
	 way the user has of requiring the extension to be used.  */
      if ((is_a_field_of_this.type == NULL && !is_after_structop) 
	  || is_quoted_name)
	{
	  /* See if it's a file name. */
	  struct symtab *symtab;

	  symtab = lookup_symtab (copy.c_str ());
	  if (symtab)
	    {
	      yylval.bval
		= symtab->compunit ()->blockvector ()->static_block ();

	      return FILENAME;
	    }
	}
    }

  if (bsym.symbol && bsym.symbol->aclass () == LOC_TYPEDEF)
    {
      yylval.tsym.type = bsym.symbol->type ();
      return TYPENAME;
    }

  /* See if it's an ObjC classname.  */
  if (par_state->language ()->la_language == language_objc && !bsym.symbol)
    {
      CORE_ADDR Class = lookup_objc_class (par_state->gdbarch (),
					   copy.c_str ());
      if (Class)
	{
	  struct symbol *sym;

	  yylval.theclass.theclass = Class;
	  sym = lookup_struct_typedef (copy.c_str (),
				       par_state->expression_context_block, 1);
	  if (sym)
	    yylval.theclass.type = sym->type ();
	  return CLASSNAME;
	}
    }

  /* Input names that aren't symbols but ARE valid hex numbers, when
     the input radix permits them, can be names or numbers depending
     on the parse.  Note we support radixes > 16 here.  */
  if (!bsym.symbol
      && ((copy[0] >= 'a' && copy[0] < 'a' + input_radix - 10)
	  || (copy[0] >= 'A' && copy[0] < 'A' + input_radix - 10)))
    {
      YYSTYPE newlval;	/* Its value is ignored.  */
      int hextype = parse_number (par_state, copy.c_str (), yylval.sval.length,
				  0, &newlval);

      if (hextype == INT)
	{
	  yylval.ssym.sym = bsym;
	  yylval.ssym.is_a_field_of_this = is_a_field_of_this.type != NULL;
	  return NAME_OR_INT;
	}
    }

  /* Any other kind of symbol */
  yylval.ssym.sym = bsym;
  yylval.ssym.is_a_field_of_this = is_a_field_of_this.type != NULL;

  if (bsym.symbol == NULL
      && par_state->language ()->la_language == language_cplus
      && is_a_field_of_this.type == NULL
      && lookup_minimal_symbol (copy.c_str (), NULL, NULL).minsym == NULL)
    return UNKNOWN_CPP_NAME;

  return NAME;
}

/* Like classify_name, but used by the inner loop of the lexer, when a
   name might have already been seen.  CONTEXT is the context type, or
   NULL if this is the first component of a name.  */

static int
classify_inner_name (struct parser_state *par_state,
		     const struct block *block, struct type *context)
{
  struct type *type;

  if (context == NULL)
    return classify_name (par_state, block, false, false);

  type = check_typedef (context);
  if (!type_aggregate_p (type))
    return ERROR;

  std::string copy = copy_name (yylval.ssym.stoken);
  /* N.B. We assume the symbol can only be in VAR_DOMAIN.  */
  yylval.ssym.sym = cp_lookup_nested_symbol (type, copy.c_str (), block,
					     VAR_DOMAIN);

  /* If no symbol was found, search for a matching base class named
     COPY.  This will allow users to enter qualified names of class members
     relative to the `this' pointer.  */
  if (yylval.ssym.sym.symbol == NULL)
    {
      struct type *base_type = cp_find_type_baseclass_by_name (type,
							       copy.c_str ());

      if (base_type != NULL)
	{
	  yylval.tsym.type = base_type;
	  return TYPENAME;
	}

      return ERROR;
    }

  switch (yylval.ssym.sym.symbol->aclass ())
    {
    case LOC_BLOCK:
    case LOC_LABEL:
      /* cp_lookup_nested_symbol might have accidentally found a constructor
	 named COPY when we really wanted a base class of the same name.
	 Double-check this case by looking for a base class.  */
      {
	struct type *base_type
	  = cp_find_type_baseclass_by_name (type, copy.c_str ());

	if (base_type != NULL)
	  {
	    yylval.tsym.type = base_type;
	    return TYPENAME;
	  }
      }
      return ERROR;

    case LOC_TYPEDEF:
      yylval.tsym.type = yylval.ssym.sym.symbol->type ();
      return TYPENAME;

    default:
      return NAME;
    }
  internal_error (_("not reached"));
}

/* The outer level of a two-level lexer.  This calls the inner lexer
   to return tokens.  It then either returns these tokens, or
   aggregates them into a larger token.  This lets us work around a
   problem in our parsing approach, where the parser could not
   distinguish between qualified names and qualified types at the
   right point.

   This approach is still not ideal, because it mishandles template
   types.  See the comment in lex_one_token for an example.  However,
   this is still an improvement over the earlier approach, and will
   suffice until we move to better parsing technology.  */

static int
yylex (void)
{
  c_token_and_value current;
  int first_was_coloncolon, last_was_coloncolon;
  struct type *context_type = NULL;
  int last_to_examine, next_to_examine, checkpoint;
  const struct block *search_block;
  bool is_quoted_name, last_lex_was_structop;

  if (popping && !token_fifo.empty ())
    goto do_pop;
  popping = 0;

  last_lex_was_structop = last_was_structop;

  /* Read the first token and decide what to do.  Most of the
     subsequent code is C++-only; but also depends on seeing a "::" or
     name-like token.  */
  current.token = lex_one_token (pstate, &is_quoted_name);
  if (current.token == NAME)
    current.token = classify_name (pstate, pstate->expression_context_block,
				   is_quoted_name, last_lex_was_structop);
  if (pstate->language ()->la_language != language_cplus
      || (current.token != TYPENAME && current.token != COLONCOLON
	  && current.token != FILENAME))
    return current.token;

  /* Read any sequence of alternating "::" and name-like tokens into
     the token FIFO.  */
  current.value = yylval;
  token_fifo.push_back (current);
  last_was_coloncolon = current.token == COLONCOLON;
  while (1)
    {
      bool ignore;

      /* We ignore quoted names other than the very first one.
	 Subsequent ones do not have any special meaning.  */
      current.token = lex_one_token (pstate, &ignore);
      current.value = yylval;
      token_fifo.push_back (current);

      if ((last_was_coloncolon && current.token != NAME)
	  || (!last_was_coloncolon && current.token != COLONCOLON))
	break;
      last_was_coloncolon = !last_was_coloncolon;
    }
  popping = 1;

  /* We always read one extra token, so compute the number of tokens
     to examine accordingly.  */
  last_to_examine = token_fifo.size () - 2;
  next_to_examine = 0;

  current = token_fifo[next_to_examine];
  ++next_to_examine;

  name_obstack.clear ();
  checkpoint = 0;
  if (current.token == FILENAME)
    search_block = current.value.bval;
  else if (current.token == COLONCOLON)
    search_block = NULL;
  else
    {
      gdb_assert (current.token == TYPENAME);
      search_block = pstate->expression_context_block;
      obstack_grow (&name_obstack, current.value.sval.ptr,
		    current.value.sval.length);
      context_type = current.value.tsym.type;
      checkpoint = 1;
    }

  first_was_coloncolon = current.token == COLONCOLON;
  last_was_coloncolon = first_was_coloncolon;

  while (next_to_examine <= last_to_examine)
    {
      c_token_and_value next;

      next = token_fifo[next_to_examine];
      ++next_to_examine;

      if (next.token == NAME && last_was_coloncolon)
	{
	  int classification;

	  yylval = next.value;
	  classification = classify_inner_name (pstate, search_block,
						context_type);
	  /* We keep going until we either run out of names, or until
	     we have a qualified name which is not a type.  */
	  if (classification != TYPENAME && classification != NAME)
	    break;

	  /* Accept up to this token.  */
	  checkpoint = next_to_examine;

	  /* Update the partial name we are constructing.  */
	  if (context_type != NULL)
	    {
	      /* We don't want to put a leading "::" into the name.  */
	      obstack_grow_str (&name_obstack, "::");
	    }
	  obstack_grow (&name_obstack, next.value.sval.ptr,
			next.value.sval.length);

	  yylval.sval.ptr = (const char *) obstack_base (&name_obstack);
	  yylval.sval.length = obstack_object_size (&name_obstack);
	  current.value = yylval;
	  current.token = classification;

	  last_was_coloncolon = 0;

	  if (classification == NAME)
	    break;

	  context_type = yylval.tsym.type;
	}
      else if (next.token == COLONCOLON && !last_was_coloncolon)
	last_was_coloncolon = 1;
      else
	{
	  /* We've reached the end of the name.  */
	  break;
	}
    }

  /* If we have a replacement token, install it as the first token in
     the FIFO, and delete the other constituent tokens.  */
  if (checkpoint > 0)
    {
      current.value.sval.ptr
	= obstack_strndup (&cpstate->expansion_obstack,
			   current.value.sval.ptr,
			   current.value.sval.length);

      token_fifo[0] = current;
      if (checkpoint > 1)
	token_fifo.erase (token_fifo.begin () + 1,
			  token_fifo.begin () + checkpoint);
    }

 do_pop:
  current = token_fifo[0];
  token_fifo.erase (token_fifo.begin ());
  yylval = current.value;
  return current.token;
}

int
c_parse (struct parser_state *par_state)
{
  /* Setting up the parser state.  */
  scoped_restore pstate_restore = make_scoped_restore (&pstate);
  gdb_assert (par_state != NULL);
  pstate = par_state;

  c_parse_state cstate;
  scoped_restore cstate_restore = make_scoped_restore (&cpstate, &cstate);

  gdb::unique_xmalloc_ptr<struct macro_scope> macro_scope;

  if (par_state->expression_context_block)
    macro_scope
      = sal_macro_scope (find_pc_line (par_state->expression_context_pc, 0));
  else
    macro_scope = default_macro_scope ();
  if (! macro_scope)
    macro_scope = user_macro_scope ();

  scoped_restore restore_macro_scope
    = make_scoped_restore (&expression_macro_scope, macro_scope.get ());

  scoped_restore restore_yydebug = make_scoped_restore (&yydebug,
							par_state->debug);

  /* Initialize some state used by the lexer.  */
  last_was_structop = false;
  saw_name_at_eof = 0;
  paren_depth = 0;

  token_fifo.clear ();
  popping = 0;
  name_obstack.clear ();

  int result = yyparse ();
  if (!result)
    pstate->set_operation (pstate->pop ());
  return result;
}

#if defined(YYBISON) && YYBISON < 30800


/* This is called via the YYPRINT macro when parser debugging is
   enabled.  It prints a token's value.  */

static void
c_print_token (FILE *file, int type, YYSTYPE value)
{
  switch (type)
    {
    case INT:
      parser_fprintf (file, "typed_val_int<%s, %s>",
		      TYPE_SAFE_NAME (value.typed_val_int.type),
		      pulongest (value.typed_val_int.val));
      break;

    case CHAR:
    case STRING:
      {
	char *copy = (char *) alloca (value.tsval.length + 1);

	memcpy (copy, value.tsval.ptr, value.tsval.length);
	copy[value.tsval.length] = '\0';

	parser_fprintf (file, "tsval<type=%d, %s>", value.tsval.type, copy);
      }
      break;

    case NSSTRING:
    case DOLLAR_VARIABLE:
      parser_fprintf (file, "sval<%s>", copy_name (value.sval).c_str ());
      break;

    case TYPENAME:
      parser_fprintf (file, "tsym<type=%s, name=%s>",
		      TYPE_SAFE_NAME (value.tsym.type),
		      copy_name (value.tsym.stoken).c_str ());
      break;

    case NAME:
    case UNKNOWN_CPP_NAME:
    case NAME_OR_INT:
    case BLOCKNAME:
      parser_fprintf (file, "ssym<name=%s, sym=%s, field_of_this=%d>",
		       copy_name (value.ssym.stoken).c_str (),
		       (value.ssym.sym.symbol == NULL
			? "(null)" : value.ssym.sym.symbol->print_name ()),
		       value.ssym.is_a_field_of_this);
      break;

    case FILENAME:
      parser_fprintf (file, "bval<%s>", host_address_to_string (value.bval));
      break;
    }
}

#endif

static void
yyerror (const char *msg)
{
  pstate->parse_error (msg);
}
