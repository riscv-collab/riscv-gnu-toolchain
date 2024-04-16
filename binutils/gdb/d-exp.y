/* YACC parser for D expressions, for GDB.

   Copyright (C) 2014-2024 Free Software Foundation, Inc.

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

/* This file is derived from c-exp.y, jv-exp.y.  */

/* Parse a D expression from text in a string,
   and return the result as a struct expression pointer.
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
#include "d-lang.h"
#include "charset.h"
#include "block.h"
#include "type-stack.h"
#include "expop.h"

#define parse_type(ps) builtin_type (ps->gdbarch ())
#define parse_d_type(ps) builtin_d_type (ps->gdbarch ())

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror,
   etc).  */
#define GDB_YY_REMAP_PREFIX d_
#include "yy-remap.h"

/* The state of the parser, used internally when we are parsing the
   expression.  */

static struct parser_state *pstate = NULL;

/* The current type stack.  */
static struct type_stack *type_stack;

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
    struct {
      LONGEST val;
      struct type *type;
    } typed_val_int;
    struct {
      gdb_byte val[16];
      struct type *type;
    } typed_val_float;
    struct symbol *sym;
    struct type *tval;
    struct typed_stoken tsval;
    struct stoken sval;
    struct ttype tsym;
    struct symtoken ssym;
    int ival;
    int voidval;
    enum exp_opcode opcode;
    struct stoken_vector svec;
  }

%{
/* YYSTYPE gets defined by %union */
static int parse_number (struct parser_state *, const char *,
			 int, int, YYSTYPE *);
%}

%token <sval> IDENTIFIER UNKNOWN_NAME
%token <tsym> TYPENAME
%token <voidval> COMPLETE

/* A NAME_OR_INT is a symbol which is not known in the symbol table,
   but which would parse as a valid number in the current input radix.
   E.g. "c" when input_radix==16.  Depending on the parse, it will be
   turned into a name or into a number.  */

%token <sval> NAME_OR_INT

%token <typed_val_int> INTEGER_LITERAL
%token <typed_val_float> FLOAT_LITERAL
%token <tsval> CHARACTER_LITERAL
%token <tsval> STRING_LITERAL

%type <svec> StringExp
%type <tval> BasicType TypeExp
%type <sval> IdentifierExp
%type <ival> ArrayLiteral

%token ENTRY
%token ERROR

/* Keywords that have a constant value.  */
%token TRUE_KEYWORD FALSE_KEYWORD NULL_KEYWORD
/* Class 'super' accessor.  */
%token SUPER_KEYWORD
/* Properties.  */
%token CAST_KEYWORD SIZEOF_KEYWORD
%token TYPEOF_KEYWORD TYPEID_KEYWORD
%token INIT_KEYWORD
/* Comparison keywords.  */
/* Type storage classes.  */
%token IMMUTABLE_KEYWORD CONST_KEYWORD SHARED_KEYWORD
/* Non-scalar type keywords.  */
%token STRUCT_KEYWORD UNION_KEYWORD
%token CLASS_KEYWORD INTERFACE_KEYWORD
%token ENUM_KEYWORD TEMPLATE_KEYWORD
%token DELEGATE_KEYWORD FUNCTION_KEYWORD

%token <sval> DOLLAR_VARIABLE

%token <opcode> ASSIGN_MODIFY

%left ','
%right '=' ASSIGN_MODIFY
%right '?'
%left OROR
%left ANDAND
%left '|'
%left '^'
%left '&'
%left EQUAL NOTEQUAL '<' '>' LEQ GEQ
%right LSH RSH
%left '+' '-'
%left '*' '/' '%'
%right HATHAT
%left IDENTITY NOTIDENTITY
%right INCREMENT DECREMENT
%right '.' '[' '('
%token DOTDOT


%%

start   :
	Expression
|	TypeExp
;

/* Expressions, including the comma operator.  */

Expression:
	CommaExpression
;

CommaExpression:
	AssignExpression
|	AssignExpression ',' CommaExpression
		{ pstate->wrap2<comma_operation> (); }
;

AssignExpression:
	ConditionalExpression
|	ConditionalExpression '=' AssignExpression
		{ pstate->wrap2<assign_operation> (); }
|	ConditionalExpression ASSIGN_MODIFY AssignExpression
		{
		  operation_up rhs = pstate->pop ();
		  operation_up lhs = pstate->pop ();
		  pstate->push_new<assign_modify_operation>
		    ($2, std::move (lhs), std::move (rhs));
		}
;

ConditionalExpression:
	OrOrExpression
|	OrOrExpression '?' Expression ':' ConditionalExpression
		{
		  operation_up last = pstate->pop ();
		  operation_up mid = pstate->pop ();
		  operation_up first = pstate->pop ();
		  pstate->push_new<ternop_cond_operation>
		    (std::move (first), std::move (mid),
		     std::move (last));
		}
;

OrOrExpression:
	AndAndExpression
|	OrOrExpression OROR AndAndExpression
		{ pstate->wrap2<logical_or_operation> (); }
;

AndAndExpression:
	OrExpression
|	AndAndExpression ANDAND OrExpression
		{ pstate->wrap2<logical_and_operation> (); }
;

OrExpression:
	XorExpression
|	OrExpression '|' XorExpression
		{ pstate->wrap2<bitwise_ior_operation> (); }
;

XorExpression:
	AndExpression
|	XorExpression '^' AndExpression
		{ pstate->wrap2<bitwise_xor_operation> (); }
;

AndExpression:
	CmpExpression
|	AndExpression '&' CmpExpression
		{ pstate->wrap2<bitwise_and_operation> (); }
;

CmpExpression:
	ShiftExpression
|	EqualExpression
|	IdentityExpression
|	RelExpression
;

EqualExpression:
	ShiftExpression EQUAL ShiftExpression
		{ pstate->wrap2<equal_operation> (); }
|	ShiftExpression NOTEQUAL ShiftExpression
		{ pstate->wrap2<notequal_operation> (); }
;

IdentityExpression:
	ShiftExpression IDENTITY ShiftExpression
		{ pstate->wrap2<equal_operation> (); }
|	ShiftExpression NOTIDENTITY ShiftExpression
		{ pstate->wrap2<notequal_operation> (); }
;

RelExpression:
	ShiftExpression '<' ShiftExpression
		{ pstate->wrap2<less_operation> (); }
|	ShiftExpression LEQ ShiftExpression
		{ pstate->wrap2<leq_operation> (); }
|	ShiftExpression '>' ShiftExpression
		{ pstate->wrap2<gtr_operation> (); }
|	ShiftExpression GEQ ShiftExpression
		{ pstate->wrap2<geq_operation> (); }
;

ShiftExpression:
	AddExpression
|	ShiftExpression LSH AddExpression
		{ pstate->wrap2<lsh_operation> (); }
|	ShiftExpression RSH AddExpression
		{ pstate->wrap2<rsh_operation> (); }
;

AddExpression:
	MulExpression
|	AddExpression '+' MulExpression
		{ pstate->wrap2<add_operation> (); }
|	AddExpression '-' MulExpression
		{ pstate->wrap2<sub_operation> (); }
|	AddExpression '~' MulExpression
		{ pstate->wrap2<concat_operation> (); }
;

MulExpression:
	UnaryExpression
|	MulExpression '*' UnaryExpression
		{ pstate->wrap2<mul_operation> (); }
|	MulExpression '/' UnaryExpression
		{ pstate->wrap2<div_operation> (); }
|	MulExpression '%' UnaryExpression
		{ pstate->wrap2<rem_operation> (); }

UnaryExpression:
	'&' UnaryExpression
		{ pstate->wrap<unop_addr_operation> (); }
|	INCREMENT UnaryExpression
		{ pstate->wrap<preinc_operation> (); }
|	DECREMENT UnaryExpression
		{ pstate->wrap<predec_operation> (); }
|	'*' UnaryExpression
		{ pstate->wrap<unop_ind_operation> (); }
|	'-' UnaryExpression
		{ pstate->wrap<unary_neg_operation> (); }
|	'+' UnaryExpression
		{ pstate->wrap<unary_plus_operation> (); }
|	'!' UnaryExpression
		{ pstate->wrap<unary_logical_not_operation> (); }
|	'~' UnaryExpression
		{ pstate->wrap<unary_complement_operation> (); }
|	TypeExp '.' SIZEOF_KEYWORD
		{ pstate->wrap<unop_sizeof_operation> (); }
|	CastExpression
|	PowExpression
;

CastExpression:
	CAST_KEYWORD '(' TypeExp ')' UnaryExpression
		{ pstate->wrap2<unop_cast_type_operation> (); }
	/* C style cast is illegal D, but is still recognised in
	   the grammar, so we keep this around for convenience.  */
|	'(' TypeExp ')' UnaryExpression
		{ pstate->wrap2<unop_cast_type_operation> (); }
;

PowExpression:
	PostfixExpression
|	PostfixExpression HATHAT UnaryExpression
		{ pstate->wrap2<exp_operation> (); }
;

PostfixExpression:
	PrimaryExpression
|	PostfixExpression '.' COMPLETE
		{
		  structop_base_operation *op
		    = new structop_ptr_operation (pstate->pop (), "");
		  pstate->mark_struct_expression (op);
		  pstate->push (operation_up (op));
		}
|	PostfixExpression '.' IDENTIFIER
		{
		  pstate->push_new<structop_operation>
		    (pstate->pop (), copy_name ($3));
		}
|	PostfixExpression '.' IDENTIFIER COMPLETE
		{
		  structop_base_operation *op
		    = new structop_operation (pstate->pop (), copy_name ($3));
		  pstate->mark_struct_expression (op);
		  pstate->push (operation_up (op));
		}
|	PostfixExpression '.' SIZEOF_KEYWORD
		{ pstate->wrap<unop_sizeof_operation> (); }
|	PostfixExpression INCREMENT
		{ pstate->wrap<postinc_operation> (); }
|	PostfixExpression DECREMENT
		{ pstate->wrap<postdec_operation> (); }
|	CallExpression
|	IndexExpression
|	SliceExpression
;

ArgumentList:
	AssignExpression
		{ pstate->arglist_len = 1; }
|	ArgumentList ',' AssignExpression
		{ pstate->arglist_len++; }
;

ArgumentList_opt:
	/* EMPTY */
		{ pstate->arglist_len = 0; }
|	ArgumentList
;

CallExpression:
	PostfixExpression '('
		{ pstate->start_arglist (); }
	ArgumentList_opt ')'
		{
		  std::vector<operation_up> args
		    = pstate->pop_vector (pstate->end_arglist ());
		  pstate->push_new<funcall_operation>
		    (pstate->pop (), std::move (args));
		}
;

IndexExpression:
	PostfixExpression '[' ArgumentList ']'
		{ if (pstate->arglist_len > 0)
		    {
		      std::vector<operation_up> args
			= pstate->pop_vector (pstate->arglist_len);
		      pstate->push_new<multi_subscript_operation>
			(pstate->pop (), std::move (args));
		    }
		  else
		    pstate->wrap2<subscript_operation> ();
		}
;

SliceExpression:
	PostfixExpression '[' ']'
		{ /* Do nothing.  */ }
|	PostfixExpression '[' AssignExpression DOTDOT AssignExpression ']'
		{
		  operation_up last = pstate->pop ();
		  operation_up mid = pstate->pop ();
		  operation_up first = pstate->pop ();
		  pstate->push_new<ternop_slice_operation>
		    (std::move (first), std::move (mid),
		     std::move (last));
		}
;

PrimaryExpression:
	'(' Expression ')'
		{ /* Do nothing.  */ }
|	IdentifierExp
		{ struct bound_minimal_symbol msymbol;
		  std::string copy = copy_name ($1);
		  struct field_of_this_result is_a_field_of_this;
		  struct block_symbol sym;

		  /* Handle VAR, which could be local or global.  */
		  sym = lookup_symbol (copy.c_str (),
				       pstate->expression_context_block,
				       VAR_DOMAIN, &is_a_field_of_this);
		  if (sym.symbol && sym.symbol->aclass () != LOC_TYPEDEF)
		    {
		      if (symbol_read_needs_frame (sym.symbol))
			pstate->block_tracker->update (sym);
		      pstate->push_new<var_value_operation> (sym);
		    }
		  else if (is_a_field_of_this.type != NULL)
		     {
		      /* It hangs off of `this'.  Must not inadvertently convert from a
			 method call to data ref.  */
		      pstate->block_tracker->update (sym);
		      operation_up thisop
			= make_operation<op_this_operation> ();
		      pstate->push_new<structop_ptr_operation>
			(std::move (thisop), std::move (copy));
		    }
		  else
		    {
		      /* Lookup foreign name in global static symbols.  */
		      msymbol = lookup_bound_minimal_symbol (copy.c_str ());
		      if (msymbol.minsym != NULL)
			pstate->push_new<var_msym_value_operation> (msymbol);
		      else if (!have_full_symbols () && !have_partial_symbols ())
			error (_("No symbol table is loaded.  Use the \"file\" command"));
		      else
			error (_("No symbol \"%s\" in current context."),
			       copy.c_str ());
		    }
		  }
|	TypeExp '.' IdentifierExp
			{ struct type *type = check_typedef ($1);

			  /* Check if the qualified name is in the global
			     context.  However if the symbol has not already
			     been resolved, it's not likely to be found.  */
			  if (type->code () == TYPE_CODE_MODULE)
			    {
			      struct block_symbol sym;
			      const char *type_name = TYPE_SAFE_NAME (type);
			      int type_name_len = strlen (type_name);
			      std::string name
				= string_printf ("%.*s.%.*s",
						 type_name_len, type_name,
						 $3.length, $3.ptr);

			      sym =
				lookup_symbol (name.c_str (),
					       (const struct block *) NULL,
					       VAR_DOMAIN, NULL);
			      pstate->push_symbol (name.c_str (), sym);
			    }
			  else
			    {
			      /* Check if the qualified name resolves as a member
				 of an aggregate or an enum type.  */
			      if (!type_aggregate_p (type))
				error (_("`%s' is not defined as an aggregate type."),
				       TYPE_SAFE_NAME (type));

			      pstate->push_new<scope_operation>
				(type, copy_name ($3));
			    }
			}
|	DOLLAR_VARIABLE
		{ pstate->push_dollar ($1); }
|	NAME_OR_INT
		{ YYSTYPE val;
		  parse_number (pstate, $1.ptr, $1.length, 0, &val);
		  pstate->push_new<long_const_operation>
		    (val.typed_val_int.type, val.typed_val_int.val); }
|	NULL_KEYWORD
		{ struct type *type = parse_d_type (pstate)->builtin_void;
		  type = lookup_pointer_type (type);
		  pstate->push_new<long_const_operation> (type, 0); }
|	TRUE_KEYWORD
		{ pstate->push_new<bool_operation> (true); }
|	FALSE_KEYWORD
		{ pstate->push_new<bool_operation> (false); }
|	INTEGER_LITERAL
		{ pstate->push_new<long_const_operation> ($1.type, $1.val); }
|	FLOAT_LITERAL
		{
		  float_data data;
		  std::copy (std::begin ($1.val), std::end ($1.val),
			     std::begin (data));
		  pstate->push_new<float_const_operation> ($1.type, data);
		}
|	CHARACTER_LITERAL
		{ struct stoken_vector vec;
		  vec.len = 1;
		  vec.tokens = &$1;
		  pstate->push_c_string (0, &vec); }
|	StringExp
		{ int i;
		  pstate->push_c_string (0, &$1);
		  for (i = 0; i < $1.len; ++i)
		    free ($1.tokens[i].ptr);
		  free ($1.tokens); }
|	ArrayLiteral
		{
		  std::vector<operation_up> args
		    = pstate->pop_vector ($1);
		  pstate->push_new<array_operation>
		    (0, $1 - 1, std::move (args));
		}
|	TYPEOF_KEYWORD '(' Expression ')'
		{ pstate->wrap<typeof_operation> (); }
;

ArrayLiteral:
	'[' ArgumentList_opt ']'
		{ $$ = pstate->arglist_len; }
;

IdentifierExp:
	IDENTIFIER
;

StringExp:
	STRING_LITERAL
		{ /* We copy the string here, and not in the
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
|	StringExp STRING_LITERAL
		{ /* Note that we NUL-terminate here, but just
		     for convenience.  */
		  char *p;
		  ++$$.len;
		  $$.tokens
		    = XRESIZEVEC (struct typed_stoken, $$.tokens, $$.len);

		  p = (char *) malloc ($2.length + 1);
		  memcpy (p, $2.ptr, $2.length + 1);

		  $$.tokens[$$.len - 1].type = $2.type;
		  $$.tokens[$$.len - 1].length = $2.length;
		  $$.tokens[$$.len - 1].ptr = p;
		}
;

TypeExp:
	'(' TypeExp ')'
		{ /* Do nothing.  */ }
|	BasicType
		{ pstate->push_new<type_operation> ($1); }
|	BasicType BasicType2
		{ $$ = type_stack->follow_types ($1);
		  pstate->push_new<type_operation> ($$);
		}
;

BasicType2:
	'*'
		{ type_stack->push (tp_pointer); }
|	'*' BasicType2
		{ type_stack->push (tp_pointer); }
|	'[' INTEGER_LITERAL ']'
		{ type_stack->push ($2.val);
		  type_stack->push (tp_array); }
|	'[' INTEGER_LITERAL ']' BasicType2
		{ type_stack->push ($2.val);
		  type_stack->push (tp_array); }
;

BasicType:
	TYPENAME
		{ $$ = $1.type; }
;

%%

/* Return true if the type is aggregate-like.  */

static int
type_aggregate_p (struct type *type)
{
  return (type->code () == TYPE_CODE_STRUCT
	  || type->code () == TYPE_CODE_UNION
	  || type->code () == TYPE_CODE_MODULE
	  || (type->code () == TYPE_CODE_ENUM
	      && type->is_declared_class ()));
}

/* Take care of parsing a number (anything that starts with a digit).
   Set yylval and return the token type; update lexptr.
   LEN is the number of characters in it.  */

/*** Needs some error checking for the float case ***/

static int
parse_number (struct parser_state *ps, const char *p,
	      int len, int parsed_float, YYSTYPE *putithere)
{
  ULONGEST n = 0;
  ULONGEST prevn = 0;
  ULONGEST un;

  int i = 0;
  int c;
  int base = input_radix;
  int unsigned_p = 0;
  int long_p = 0;

  /* We have found a "L" or "U" suffix.  */
  int found_suffix = 0;

  ULONGEST high_bit;
  struct type *signed_type;
  struct type *unsigned_type;

  if (parsed_float)
    {
      char *s, *sp;

      /* Strip out all embedded '_' before passing to parse_float.  */
      s = (char *) alloca (len + 1);
      sp = s;
      while (len-- > 0)
	{
	  if (*p != '_')
	    *sp++ = *p;
	  p++;
	}
      *sp = '\0';
      len = strlen (s);

      /* Check suffix for `i' , `fi' or `li' (idouble, ifloat or ireal).  */
      if (len >= 1 && tolower (s[len - 1]) == 'i')
	{
	  if (len >= 2 && tolower (s[len - 2]) == 'f')
	    {
	      putithere->typed_val_float.type
		= parse_d_type (ps)->builtin_ifloat;
	      len -= 2;
	    }
	  else if (len >= 2 && tolower (s[len - 2]) == 'l')
	    {
	      putithere->typed_val_float.type
		= parse_d_type (ps)->builtin_ireal;
	      len -= 2;
	    }
	  else
	    {
	      putithere->typed_val_float.type
		= parse_d_type (ps)->builtin_idouble;
	      len -= 1;
	    }
	}
      /* Check suffix for `f' or `l'' (float or real).  */
      else if (len >= 1 && tolower (s[len - 1]) == 'f')
	{
	  putithere->typed_val_float.type
	    = parse_d_type (ps)->builtin_float;
	  len -= 1;
	}
      else if (len >= 1 && tolower (s[len - 1]) == 'l')
	{
	  putithere->typed_val_float.type
	    = parse_d_type (ps)->builtin_real;
	  len -= 1;
	}
      /* Default type if no suffix.  */
      else
	{
	  putithere->typed_val_float.type
	    = parse_d_type (ps)->builtin_double;
	}

      if (!parse_float (s, len,
			putithere->typed_val_float.type,
			putithere->typed_val_float.val))
	return ERROR;

      return FLOAT_LITERAL;
    }

  /* Handle base-switching prefixes 0x, 0b, 0 */
  if (p[0] == '0')
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

      default:
	base = 8;
	break;
      }

  while (len-- > 0)
    {
      c = *p++;
      if (c == '_')
	continue;	/* Ignore embedded '_'.  */
      if (c >= 'A' && c <= 'Z')
	c += 'a' - 'A';
      if (c != 'l' && c != 'u')
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
	  else if (c == 'l' && long_p == 0)
	    {
	      long_p = 1;
	      found_suffix = 1;
	    }
	  else if (c == 'u' && unsigned_p == 0)
	    {
	      unsigned_p = 1;
	      found_suffix = 1;
	    }
	  else
	    return ERROR;	/* Char not a digit */
	}
      if (i >= base)
	return ERROR;		/* Invalid digit in this base.  */
      /* Portably test for integer overflow.  */
      if (c != 'l' && c != 'u')
	{
	  ULONGEST n2 = prevn * base;
	  if ((n2 / base != prevn) || (n2 + i < prevn))
	    error (_("Numeric constant too large."));
	}
      prevn = n;
    }

  /* An integer constant is an int or a long.  An L suffix forces it to
     be long, and a U suffix forces it to be unsigned.  To figure out
     whether it fits, we shift it right and see whether anything remains.
     Note that we can't shift sizeof (LONGEST) * HOST_CHAR_BIT bits or
     more in one operation, because many compilers will warn about such a
     shift (which always produces a zero result).  To deal with the case
     where it is we just always shift the value more than once, with fewer
     bits each time.  */
  un = (ULONGEST) n >> 2;
  if (long_p == 0 && (un >> 30) == 0)
    {
      high_bit = ((ULONGEST) 1) << 31;
      signed_type = parse_d_type (ps)->builtin_int;
      /* For decimal notation, keep the sign of the worked out type.  */
      if (base == 10 && !unsigned_p)
	unsigned_type = parse_d_type (ps)->builtin_long;
      else
	unsigned_type = parse_d_type (ps)->builtin_uint;
    }
  else
    {
      int shift;
      if (sizeof (ULONGEST) * HOST_CHAR_BIT < 64)
	/* A long long does not fit in a LONGEST.  */
	shift = (sizeof (ULONGEST) * HOST_CHAR_BIT - 1);
      else
	shift = 63;
      high_bit = (ULONGEST) 1 << shift;
      signed_type = parse_d_type (ps)->builtin_long;
      unsigned_type = parse_d_type (ps)->builtin_ulong;
    }

  putithere->typed_val_int.val = n;

  /* If the high bit of the worked out type is set then this number
     has to be unsigned_type.  */
  if (unsigned_p || (n & high_bit))
    putithere->typed_val_int.type = unsigned_type;
  else
    putithere->typed_val_int.type = signed_type;

  return INTEGER_LITERAL;
}

/* Temporary obstack used for holding strings.  */
static struct obstack tempbuf;
static int tempbuf_init;

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

  /* Skip the quote.  */
  quote = *tokptr;
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
      if (quote == '"' || quote == '`')
	error (_("Unterminated string in expression."));
      else
	error (_("Unmatched single quote."));
    }
  ++tokptr;

  /* FIXME: should instead use own language string_type enum
     and handle D-specific string suffixes here. */
  if (quote == '\'')
    value->type = C_CHAR;
  else
    value->type = C_STRING;

  value->ptr = (char *) obstack_base (&tempbuf);
  value->length = obstack_object_size (&tempbuf);

  *outptr = tokptr;

  return quote == '\'' ? CHARACTER_LITERAL : STRING_LITERAL;
}

struct d_token
{
  const char *oper;
  int token;
  enum exp_opcode opcode;
};

static const struct d_token tokentab3[] =
  {
    {"^^=", ASSIGN_MODIFY, BINOP_EXP},
    {"<<=", ASSIGN_MODIFY, BINOP_LSH},
    {">>=", ASSIGN_MODIFY, BINOP_RSH},
  };

static const struct d_token tokentab2[] =
  {
    {"+=", ASSIGN_MODIFY, BINOP_ADD},
    {"-=", ASSIGN_MODIFY, BINOP_SUB},
    {"*=", ASSIGN_MODIFY, BINOP_MUL},
    {"/=", ASSIGN_MODIFY, BINOP_DIV},
    {"%=", ASSIGN_MODIFY, BINOP_REM},
    {"|=", ASSIGN_MODIFY, BINOP_BITWISE_IOR},
    {"&=", ASSIGN_MODIFY, BINOP_BITWISE_AND},
    {"^=", ASSIGN_MODIFY, BINOP_BITWISE_XOR},
    {"++", INCREMENT, OP_NULL},
    {"--", DECREMENT, OP_NULL},
    {"&&", ANDAND, OP_NULL},
    {"||", OROR, OP_NULL},
    {"^^", HATHAT, OP_NULL},
    {"<<", LSH, OP_NULL},
    {">>", RSH, OP_NULL},
    {"==", EQUAL, OP_NULL},
    {"!=", NOTEQUAL, OP_NULL},
    {"<=", LEQ, OP_NULL},
    {">=", GEQ, OP_NULL},
    {"..", DOTDOT, OP_NULL},
  };

/* Identifier-like tokens.  */
static const struct d_token ident_tokens[] =
  {
    {"is", IDENTITY, OP_NULL},
    {"!is", NOTIDENTITY, OP_NULL},

    {"cast", CAST_KEYWORD, OP_NULL},
    {"const", CONST_KEYWORD, OP_NULL},
    {"immutable", IMMUTABLE_KEYWORD, OP_NULL},
    {"shared", SHARED_KEYWORD, OP_NULL},
    {"super", SUPER_KEYWORD, OP_NULL},

    {"null", NULL_KEYWORD, OP_NULL},
    {"true", TRUE_KEYWORD, OP_NULL},
    {"false", FALSE_KEYWORD, OP_NULL},

    {"init", INIT_KEYWORD, OP_NULL},
    {"sizeof", SIZEOF_KEYWORD, OP_NULL},
    {"typeof", TYPEOF_KEYWORD, OP_NULL},
    {"typeid", TYPEID_KEYWORD, OP_NULL},

    {"delegate", DELEGATE_KEYWORD, OP_NULL},
    {"function", FUNCTION_KEYWORD, OP_NULL},
    {"struct", STRUCT_KEYWORD, OP_NULL},
    {"union", UNION_KEYWORD, OP_NULL},
    {"class", CLASS_KEYWORD, OP_NULL},
    {"interface", INTERFACE_KEYWORD, OP_NULL},
    {"enum", ENUM_KEYWORD, OP_NULL},
    {"template", TEMPLATE_KEYWORD, OP_NULL},
  };

/* This is set if a NAME token appeared at the very end of the input
   string, with no whitespace separating the name from the EOF.  This
   is used only when parsing to do field name completion.  */
static int saw_name_at_eof;

/* This is set if the previously-returned token was a structure operator.
   This is used only when parsing to do field name completion.  */
static int last_was_structop;

/* Depth of parentheses.  */
static int paren_depth;

/* Read one token, getting characters through lexptr.  */

static int
lex_one_token (struct parser_state *par_state)
{
  int c;
  int namelen;
  const char *tokstart;
  int saw_structop = last_was_structop;

  last_was_structop = 0;

 retry:

  pstate->prev_lexptr = pstate->lexptr;

  tokstart = pstate->lexptr;
  /* See if it is a special token of length 3.  */
  for (const auto &token : tokentab3)
    if (strncmp (tokstart, token.oper, 3) == 0)
      {
	pstate->lexptr += 3;
	yylval.opcode = token.opcode;
	return token.token;
      }

  /* See if it is a special token of length 2.  */
  for (const auto &token : tokentab2)
    if (strncmp (tokstart, token.oper, 2) == 0)
      {
	pstate->lexptr += 2;
	yylval.opcode = token.opcode;
	return token.token;
      }

  switch (c = *tokstart)
    {
    case 0:
      /* If we're parsing for field name completion, and the previous
	 token allows such completion, return a COMPLETE token.
	 Otherwise, we were already scanning the original text, and
	 we're really done.  */
      if (saw_name_at_eof)
	{
	  saw_name_at_eof = 0;
	  return COMPLETE;
	}
      else if (saw_structop)
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
      return c;

    case ']':
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
	{
	  if (pstate->parse_completion)
	    last_was_structop = 1;
	  goto symbol;		/* Nope, must be a symbol.  */
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
	int got_dot = 0, got_e = 0, toktype;
	const char *p = tokstart;
	int hex = input_radix > 10;

	if (c == '0' && (p[1] == 'x' || p[1] == 'X'))
	  {
	    p += 2;
	    hex = 1;
	  }

	for (;; ++p)
	  {
	    /* Hex exponents start with 'p', because 'e' is a valid hex
	       digit and thus does not indicate a floating point number
	       when the radix is hex.  */
	    if ((!hex && !got_e && tolower (p[0]) == 'e')
		|| (hex && !got_e && tolower (p[0] == 'p')))
	      got_dot = got_e = 1;
	    /* A '.' always indicates a decimal floating point number
	       regardless of the radix.  If we have a '..' then its the
	       end of the number and the beginning of a slice.  */
	    else if (!got_dot && (p[0] == '.' && p[1] != '.'))
		got_dot = 1;
	    /* This is the sign of the exponent, not the end of the number.  */
	    else if (got_e && (tolower (p[-1]) == 'e' || tolower (p[-1]) == 'p')
		     && (*p == '-' || *p == '+'))
	      continue;
	    /* We will take any letters or digits, ignoring any embedded '_'.
	       parse_number will complain if past the radix, or if L or U are
	       not final.  */
	    else if ((*p < '0' || *p > '9') && (*p != '_')
		     && ((*p < 'a' || *p > 'z') && (*p < 'A' || *p > 'Z')))
	      break;
	  }

	toktype = parse_number (par_state, tokstart, p - tokstart,
				got_dot|got_e, &yylval);
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
	size_t len = strlen ("entry");

	while (isspace (*p))
	  p++;
	if (strncmp (p, "entry", len) == 0 && !isalnum (p[len])
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

    case '\'':
    case '"':
    case '`':
      {
	int host_len;
	int result = parse_string_or_char (tokstart, &pstate->lexptr,
					   &yylval.tsval, &host_len);
	if (result == CHARACTER_LITERAL)
	  {
	    if (host_len == 0)
	      error (_("Empty character constant."));
	    else if (host_len > 2 && c == '\'')
	      {
		++tokstart;
		namelen = pstate->lexptr - tokstart - 1;
		goto tryname;
	      }
	    else if (host_len > 1)
	      error (_("Invalid character constant."));
	  }
	return result;
      }
    }

  if (!(c == '_' || c == '$'
	|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
    /* We must have come across a bad character (e.g. ';').  */
    error (_("Invalid character '%c' in expression"), c);

  /* It's a name.  See how long it is.  */
  namelen = 0;
  for (c = tokstart[namelen];
       (c == '_' || c == '$' || (c >= '0' && c <= '9')
	|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));)
    c = tokstart[++namelen];

  /* The token "if" terminates the expression and is NOT
     removed from the input stream.  */
  if (namelen == 2 && tokstart[0] == 'i' && tokstart[1] == 'f')
    return 0;

  /* For the same reason (breakpoint conditions), "thread N"
     terminates the expression.  "thread" could be an identifier, but
     an identifier is never followed by a number without intervening
     punctuation.  "task" is similar.  Handle abbreviations of these,
     similarly to breakpoint.c:find_condition_and_thread.  */
  if (namelen >= 1
      && (strncmp (tokstart, "thread", namelen) == 0
	  || strncmp (tokstart, "task", namelen) == 0)
      && (tokstart[namelen] == ' ' || tokstart[namelen] == '\t'))
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
	/* It is ok to always set this, even though we don't always
	   strictly need to.  */
	yylval.opcode = token.opcode;
	return token.token;
      }

  if (*tokstart == '$')
    return DOLLAR_VARIABLE;

  yylval.tsym.type
    = language_lookup_primitive_type (par_state->language (),
				      par_state->gdbarch (), copy.c_str ());
  if (yylval.tsym.type != NULL)
    return TYPENAME;

  /* Input names that aren't symbols but ARE valid hex numbers,
     when the input radix permits them, can be names or numbers
     depending on the parse.  Note we support radixes > 16 here.  */
  if ((tokstart[0] >= 'a' && tokstart[0] < 'a' + input_radix - 10)
      || (tokstart[0] >= 'A' && tokstart[0] < 'A' + input_radix - 10))
    {
      YYSTYPE newlval;	/* Its value is ignored.  */
      int hextype = parse_number (par_state, tokstart, namelen, 0, &newlval);
      if (hextype == INTEGER_LITERAL)
	return NAME_OR_INT;
    }

  if (pstate->parse_completion && *pstate->lexptr == '\0')
    saw_name_at_eof = 1;

  return IDENTIFIER;
}

/* An object of this type is pushed on a FIFO by the "outer" lexer.  */
struct d_token_and_value
{
  int token;
  YYSTYPE value;
};


/* A FIFO of tokens that have been read but not yet returned to the
   parser.  */
static std::vector<d_token_and_value> token_fifo;

/* Non-zero if the lexer should return tokens from the FIFO.  */
static int popping;

/* Temporary storage for yylex; this holds symbol names as they are
   built up.  */
static auto_obstack name_obstack;

/* Classify an IDENTIFIER token.  The contents of the token are in `yylval'.
   Updates yylval and returns the new token type.  BLOCK is the block
   in which lookups start; this can be NULL to mean the global scope.  */

static int
classify_name (struct parser_state *par_state, const struct block *block)
{
  struct block_symbol sym;
  struct field_of_this_result is_a_field_of_this;

  std::string copy = copy_name (yylval.sval);

  sym = lookup_symbol (copy.c_str (), block, VAR_DOMAIN, &is_a_field_of_this);
  if (sym.symbol && sym.symbol->aclass () == LOC_TYPEDEF)
    {
      yylval.tsym.type = sym.symbol->type ();
      return TYPENAME;
    }
  else if (sym.symbol == NULL)
    {
      /* Look-up first for a module name, then a type.  */
      sym = lookup_symbol (copy.c_str (), block, MODULE_DOMAIN, NULL);
      if (sym.symbol == NULL)
	sym = lookup_symbol (copy.c_str (), block, STRUCT_DOMAIN, NULL);

      if (sym.symbol != NULL)
	{
	  yylval.tsym.type = sym.symbol->type ();
	  return TYPENAME;
	}

      return UNKNOWN_NAME;
    }

  return IDENTIFIER;
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
    return classify_name (par_state, block);

  type = check_typedef (context);
  if (!type_aggregate_p (type))
    return ERROR;

  std::string copy = copy_name (yylval.ssym.stoken);
  yylval.ssym.sym = d_lookup_nested_symbol (type, copy.c_str (), block);

  if (yylval.ssym.sym.symbol == NULL)
    return ERROR;

  if (yylval.ssym.sym.symbol->aclass () == LOC_TYPEDEF)
    {
      yylval.tsym.type = yylval.ssym.sym.symbol->type ();
      return TYPENAME;
    }

  return IDENTIFIER;
}

/* The outer level of a two-level lexer.  This calls the inner lexer
   to return tokens.  It then either returns these tokens, or
   aggregates them into a larger token.  This lets us work around a
   problem in our parsing approach, where the parser could not
   distinguish between qualified names and qualified types at the
   right point.  */

static int
yylex (void)
{
  d_token_and_value current;
  int last_was_dot;
  struct type *context_type = NULL;
  int last_to_examine, next_to_examine, checkpoint;
  const struct block *search_block;

  if (popping && !token_fifo.empty ())
    goto do_pop;
  popping = 0;

  /* Read the first token and decide what to do.  */
  current.token = lex_one_token (pstate);
  if (current.token != IDENTIFIER && current.token != '.')
    return current.token;

  /* Read any sequence of alternating "." and identifier tokens into
     the token FIFO.  */
  current.value = yylval;
  token_fifo.push_back (current);
  last_was_dot = current.token == '.';

  while (1)
    {
      current.token = lex_one_token (pstate);
      current.value = yylval;
      token_fifo.push_back (current);

      if ((last_was_dot && current.token != IDENTIFIER)
	  || (!last_was_dot && current.token != '.'))
	break;

      last_was_dot = !last_was_dot;
    }
  popping = 1;

  /* We always read one extra token, so compute the number of tokens
     to examine accordingly.  */
  last_to_examine = token_fifo.size () - 2;
  next_to_examine = 0;

  current = token_fifo[next_to_examine];
  ++next_to_examine;

  /* If we are not dealing with a typename, now is the time to find out.  */
  if (current.token == IDENTIFIER)
    {
      yylval = current.value;
      current.token = classify_name (pstate, pstate->expression_context_block);
      current.value = yylval;
    }

  /* If the IDENTIFIER is not known, it could be a package symbol,
     first try building up a name until we find the qualified module.  */
  if (current.token == UNKNOWN_NAME)
    {
      name_obstack.clear ();
      obstack_grow (&name_obstack, current.value.sval.ptr,
		    current.value.sval.length);

      last_was_dot = 0;

      while (next_to_examine <= last_to_examine)
	{
	  d_token_and_value next;

	  next = token_fifo[next_to_examine];
	  ++next_to_examine;

	  if (next.token == IDENTIFIER && last_was_dot)
	    {
	      /* Update the partial name we are constructing.  */
	      obstack_grow_str (&name_obstack, ".");
	      obstack_grow (&name_obstack, next.value.sval.ptr,
			    next.value.sval.length);

	      yylval.sval.ptr = (char *) obstack_base (&name_obstack);
	      yylval.sval.length = obstack_object_size (&name_obstack);

	      current.token = classify_name (pstate,
					     pstate->expression_context_block);
	      current.value = yylval;

	      /* We keep going until we find a TYPENAME.  */
	      if (current.token == TYPENAME)
		{
		  /* Install it as the first token in the FIFO.  */
		  token_fifo[0] = current;
		  token_fifo.erase (token_fifo.begin () + 1,
				    token_fifo.begin () + next_to_examine);
		  break;
		}
	    }
	  else if (next.token == '.' && !last_was_dot)
	    last_was_dot = 1;
	  else
	    {
	      /* We've reached the end of the name.  */
	      break;
	    }
	}

      /* Reset our current token back to the start, if we found nothing
	 this means that we will just jump to do pop.  */
      current = token_fifo[0];
      next_to_examine = 1;
    }
  if (current.token != TYPENAME && current.token != '.')
    goto do_pop;

  name_obstack.clear ();
  checkpoint = 0;
  if (current.token == '.')
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

  last_was_dot = current.token == '.';

  while (next_to_examine <= last_to_examine)
    {
      d_token_and_value next;

      next = token_fifo[next_to_examine];
      ++next_to_examine;

      if (next.token == IDENTIFIER && last_was_dot)
	{
	  int classification;

	  yylval = next.value;
	  classification = classify_inner_name (pstate, search_block,
						context_type);
	  /* We keep going until we either run out of names, or until
	     we have a qualified name which is not a type.  */
	  if (classification != TYPENAME && classification != IDENTIFIER)
	    break;

	  /* Accept up to this token.  */
	  checkpoint = next_to_examine;

	  /* Update the partial name we are constructing.  */
	  if (context_type != NULL)
	    {
	      /* We don't want to put a leading "." into the name.  */
	      obstack_grow_str (&name_obstack, ".");
	    }
	  obstack_grow (&name_obstack, next.value.sval.ptr,
			next.value.sval.length);

	  yylval.sval.ptr = (char *) obstack_base (&name_obstack);
	  yylval.sval.length = obstack_object_size (&name_obstack);
	  current.value = yylval;
	  current.token = classification;

	  last_was_dot = 0;

	  if (classification == IDENTIFIER)
	    break;

	  context_type = yylval.tsym.type;
	}
      else if (next.token == '.' && !last_was_dot)
	last_was_dot = 1;
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
d_parse (struct parser_state *par_state)
{
  /* Setting up the parser state.  */
  scoped_restore pstate_restore = make_scoped_restore (&pstate);
  gdb_assert (par_state != NULL);
  pstate = par_state;

  scoped_restore restore_yydebug = make_scoped_restore (&yydebug,
							par_state->debug);

  struct type_stack stack;
  scoped_restore restore_type_stack = make_scoped_restore (&type_stack,
							   &stack);

  /* Initialize some state used by the lexer.  */
  last_was_structop = 0;
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

static void
yyerror (const char *msg)
{
  pstate->parse_error (msg);
}

