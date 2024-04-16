/* YACC parser for Pascal expressions, for GDB.
   Copyright (C) 2000-2024 Free Software Foundation, Inc.

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

/* This file is derived from c-exp.y */

/* Parse a Pascal expression from text in a string,
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

/* Known bugs or limitations:
    - pascal string operations are not supported at all.
    - there are some problems with boolean types.
    - Pascal type hexadecimal constants are not supported
      because they conflict with the internal variables format.
   Probably also lots of other problems, less well defined PM.  */
%{

#include "defs.h"
#include <ctype.h>
#include "expression.h"
#include "value.h"
#include "parser-defs.h"
#include "language.h"
#include "p-lang.h"
#include "block.h"
#include "expop.h"

#define parse_type(ps) builtin_type (ps->gdbarch ())

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror,
   etc).  */
#define GDB_YY_REMAP_PREFIX pascal_
#include "yy-remap.h"

/* The state of the parser, used internally when we are parsing the
   expression.  */

static struct parser_state *pstate = NULL;

/* Depth of parentheses.  */
static int paren_depth;

int yyparse (void);

static int yylex (void);

static void yyerror (const char *);

static char *uptok (const char *, int);

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
    struct symbol *sym;
    struct type *tval;
    struct stoken sval;
    struct ttype tsym;
    struct symtoken ssym;
    int voidval;
    const struct block *bval;
    enum exp_opcode opcode;
    struct internalvar *ivar;

    struct type **tvec;
    int *ivec;
  }

%{
/* YYSTYPE gets defined by %union */
static int parse_number (struct parser_state *,
			 const char *, int, int, YYSTYPE *);

static struct type *current_type;
static int leftdiv_is_integer;
static void push_current_type (void);
static void pop_current_type (void);
static int search_field;
%}

%type <voidval> exp exp1 type_exp start normal_start variable qualified_name
%type <tval> type typebase
/* %type <bval> block */

/* Fancy type parsing.  */
%type <tval> ptype

%token <typed_val_int> INT
%token <typed_val_float> FLOAT

/* Both NAME and TYPENAME tokens represent symbols in the input,
   and both convey their data as strings.
   But a TYPENAME is a string that happens to be defined as a typedef
   or builtin type name (such as int or char)
   and a NAME is any other symbol.
   Contexts where this distinction is not important can use the
   nonterminal "name", which matches either NAME or TYPENAME.  */

%token <sval> STRING
%token <sval> FIELDNAME
%token <voidval> COMPLETE
%token <ssym> NAME /* BLOCKNAME defined below to give it higher precedence.  */
%token <tsym> TYPENAME
%type <sval> name
%type <ssym> name_not_typename

/* A NAME_OR_INT is a symbol which is not known in the symbol table,
   but which would parse as a valid number in the current input radix.
   E.g. "c" when input_radix==16.  Depending on the parse, it will be
   turned into a name or into a number.  */

%token <ssym> NAME_OR_INT

%token STRUCT CLASS SIZEOF COLONCOLON
%token ERROR

/* Special type cases, put in to allow the parser to distinguish different
   legal basetypes.  */

%token <sval> DOLLAR_VARIABLE


/* Object pascal */
%token THIS
%token <lval> TRUEKEYWORD FALSEKEYWORD

%left ','
%left ABOVE_COMMA
%right ASSIGN
%left NOT
%left OR
%left XOR
%left ANDAND
%left '=' NOTEQUAL
%left '<' '>' LEQ GEQ
%left LSH RSH DIV MOD
%left '@'
%left '+' '-'
%left '*' '/'
%right UNARY INCREMENT DECREMENT
%right ARROW '.' '[' '('
%left '^'
%token <ssym> BLOCKNAME
%type <bval> block
%left COLONCOLON


%%

start   :	{ current_type = NULL;
		  search_field = 0;
		  leftdiv_is_integer = 0;
		}
		normal_start {}
	;

normal_start	:
		exp1
	|	type_exp
	;

type_exp:	type
			{
			  pstate->push_new<type_operation> ($1);
			  current_type = $1; } ;

/* Expressions, including the comma operator.  */
exp1	:	exp
	|	exp1 ',' exp
			{ pstate->wrap2<comma_operation> (); }
	;

/* Expressions, not including the comma operator.  */
exp	:	exp '^'   %prec UNARY
			{ pstate->wrap<unop_ind_operation> ();
			  if (current_type)
			    current_type = current_type->target_type (); }
	;

exp	:	'@' exp    %prec UNARY
			{ pstate->wrap<unop_addr_operation> ();
			  if (current_type)
			    current_type = TYPE_POINTER_TYPE (current_type); }
	;

exp	:	'-' exp    %prec UNARY
			{ pstate->wrap<unary_neg_operation> (); }
	;

exp	:	NOT exp    %prec UNARY
			{ pstate->wrap<unary_logical_not_operation> (); }
	;

exp	:	INCREMENT '(' exp ')'   %prec UNARY
			{ pstate->wrap<preinc_operation> (); }
	;

exp	:	DECREMENT  '(' exp ')'   %prec UNARY
			{ pstate->wrap<predec_operation> (); }
	;


field_exp	:	exp '.'	%prec UNARY
			{ search_field = 1; }
	;

exp	:	field_exp FIELDNAME
			{
			  pstate->push_new<structop_operation>
			    (pstate->pop (), copy_name ($2));
			  search_field = 0;
			  if (current_type)
			    {
			      while (current_type->code ()
				     == TYPE_CODE_PTR)
				current_type =
				  current_type->target_type ();
			      current_type = lookup_struct_elt_type (
				current_type, $2.ptr, 0);
			    }
			 }
	;


exp	:	field_exp name
			{
			  pstate->push_new<structop_operation>
			    (pstate->pop (), copy_name ($2));
			  search_field = 0;
			  if (current_type)
			    {
			      while (current_type->code ()
				     == TYPE_CODE_PTR)
				current_type =
				  current_type->target_type ();
			      current_type = lookup_struct_elt_type (
				current_type, $2.ptr, 0);
			    }
			}
	;
exp	:	field_exp  name COMPLETE
			{
			  structop_base_operation *op
			    = new structop_ptr_operation (pstate->pop (),
							  copy_name ($2));
			  pstate->mark_struct_expression (op);
			  pstate->push (operation_up (op));
			}
	;
exp	:	field_exp COMPLETE
			{
			  structop_base_operation *op
			    = new structop_ptr_operation (pstate->pop (), "");
			  pstate->mark_struct_expression (op);
			  pstate->push (operation_up (op));
			}
	;

exp	:	exp '['
			/* We need to save the current_type value.  */
			{ const char *arrayname;
			  int arrayfieldindex
			    = pascal_is_string_type (current_type, NULL, NULL,
						     NULL, NULL, &arrayname);
			  if (arrayfieldindex)
			    {
			      current_type
				= (current_type
				   ->field (arrayfieldindex - 1).type ());
			      pstate->push_new<structop_operation>
				(pstate->pop (), arrayname);
			    }
			  push_current_type ();  }
		exp1 ']'
			{ pop_current_type ();
			  pstate->wrap2<subscript_operation> ();
			  if (current_type)
			    current_type = current_type->target_type (); }
	;

exp	:	exp '('
			/* This is to save the value of arglist_len
			   being accumulated by an outer function call.  */
			{ push_current_type ();
			  pstate->start_arglist (); }
		arglist ')'	%prec ARROW
			{
			  std::vector<operation_up> args
			    = pstate->pop_vector (pstate->end_arglist ());
			  pstate->push_new<funcall_operation>
			    (pstate->pop (), std::move (args));
			  pop_current_type ();
			  if (current_type)
			    current_type = current_type->target_type ();
			}
	;

arglist	:
	 | exp
			{ pstate->arglist_len = 1; }
	 | arglist ',' exp   %prec ABOVE_COMMA
			{ pstate->arglist_len++; }
	;

exp	:	type '(' exp ')' %prec UNARY
			{ if (current_type)
			    {
			      /* Allow automatic dereference of classes.  */
			      if ((current_type->code () == TYPE_CODE_PTR)
				  && (current_type->target_type ()->code () == TYPE_CODE_STRUCT)
				  && (($1)->code () == TYPE_CODE_STRUCT))
				pstate->wrap<unop_ind_operation> ();
			    }
			  pstate->push_new<unop_cast_operation>
			    (pstate->pop (), $1);
			  current_type = $1; }
	;

exp	:	'(' exp1 ')'
			{ }
	;

/* Binary operators in order of decreasing precedence.  */

exp	:	exp '*' exp
			{ pstate->wrap2<mul_operation> (); }
	;

exp	:	exp '/' {
			  if (current_type && is_integral_type (current_type))
			    leftdiv_is_integer = 1;
			}
		exp
			{
			  if (leftdiv_is_integer && current_type
			      && is_integral_type (current_type))
			    {
			      pstate->push_new<unop_cast_operation>
				(pstate->pop (),
				 parse_type (pstate)->builtin_long_double);
			      current_type
				= parse_type (pstate)->builtin_long_double;
			      leftdiv_is_integer = 0;
			    }

			  pstate->wrap2<div_operation> ();
			}
	;

exp	:	exp DIV exp
			{ pstate->wrap2<intdiv_operation> (); }
	;

exp	:	exp MOD exp
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

exp	:	exp '=' exp
			{
			  pstate->wrap2<equal_operation> ();
			  current_type = parse_type (pstate)->builtin_bool;
			}
	;

exp	:	exp NOTEQUAL exp
			{
			  pstate->wrap2<notequal_operation> ();
			  current_type = parse_type (pstate)->builtin_bool;
			}
	;

exp	:	exp LEQ exp
			{
			  pstate->wrap2<leq_operation> ();
			  current_type = parse_type (pstate)->builtin_bool;
			}
	;

exp	:	exp GEQ exp
			{
			  pstate->wrap2<geq_operation> ();
			  current_type = parse_type (pstate)->builtin_bool;
			}
	;

exp	:	exp '<' exp
			{
			  pstate->wrap2<less_operation> ();
			  current_type = parse_type (pstate)->builtin_bool;
			}
	;

exp	:	exp '>' exp
			{
			  pstate->wrap2<gtr_operation> ();
			  current_type = parse_type (pstate)->builtin_bool;
			}
	;

exp	:	exp ANDAND exp
			{ pstate->wrap2<bitwise_and_operation> (); }
	;

exp	:	exp XOR exp
			{ pstate->wrap2<bitwise_xor_operation> (); }
	;

exp	:	exp OR exp
			{ pstate->wrap2<bitwise_ior_operation> (); }
	;

exp	:	exp ASSIGN exp
			{ pstate->wrap2<assign_operation> (); }
	;

exp	:	TRUEKEYWORD
			{
			  pstate->push_new<bool_operation> ($1);
			  current_type = parse_type (pstate)->builtin_bool;
			}
	;

exp	:	FALSEKEYWORD
			{
			  pstate->push_new<bool_operation> ($1);
			  current_type = parse_type (pstate)->builtin_bool;
			}
	;

exp	:	INT
			{
			  pstate->push_new<long_const_operation>
			    ($1.type, $1.val);
			  current_type = $1.type;
			}
	;

exp	:	NAME_OR_INT
			{ YYSTYPE val;
			  parse_number (pstate, $1.stoken.ptr,
					$1.stoken.length, 0, &val);
			  pstate->push_new<long_const_operation>
			    (val.typed_val_int.type,
			     val.typed_val_int.val);
			  current_type = val.typed_val_int.type;
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
			{
			  pstate->push_dollar ($1);

			  /* $ is the normal prefix for pascal
			     hexadecimal values but this conflicts
			     with the GDB use for debugger variables
			     so in expression to enter hexadecimal
			     values we still need to use C syntax with
			     0xff */
			  std::string tmp ($1.ptr, $1.length);
			  /* Handle current_type.  */
			  struct internalvar *intvar
			    = lookup_only_internalvar (tmp.c_str () + 1);
			  if (intvar != nullptr)
			    {
			      scoped_value_mark mark;

			      value *val
				= value_of_internalvar (pstate->gdbarch (),
							intvar);
			      current_type = val->type ();
			    }
 			}
 	;

exp	:	SIZEOF '(' type ')'	%prec UNARY
			{
			  current_type = parse_type (pstate)->builtin_int;
			  $3 = check_typedef ($3);
			  pstate->push_new<long_const_operation>
			    (parse_type (pstate)->builtin_int,
			     $3->length ()); }
	;

exp	:	SIZEOF  '(' exp ')'      %prec UNARY
			{ pstate->wrap<unop_sizeof_operation> ();
			  current_type = parse_type (pstate)->builtin_int; }

exp	:	STRING
			{ /* C strings are converted into array constants with
			     an explicit null byte added at the end.  Thus
			     the array upper bound is the string length.
			     There is no such thing in C as a completely empty
			     string.  */
			  const char *sp = $1.ptr; int count = $1.length;

			  std::vector<operation_up> args (count + 1);
			  for (int i = 0; i < count; ++i)
			    args[i] = (make_operation<long_const_operation>
				       (parse_type (pstate)->builtin_char,
					*sp++));
			  args[count] = (make_operation<long_const_operation>
					 (parse_type (pstate)->builtin_char,
					  '\0'));
			  pstate->push_new<array_operation>
			    (0, $1.length, std::move (args));
			}
	;

/* Object pascal  */
exp	:	THIS
			{
			  struct value * this_val;
			  struct type * this_type;
			  pstate->push_new<op_this_operation> ();
			  /* We need type of this.  */
			  this_val
			    = value_of_this_silent (pstate->language ());
			  if (this_val)
			    this_type = this_val->type ();
			  else
			    this_type = NULL;
			  if (this_type)
			    {
			      if (this_type->code () == TYPE_CODE_PTR)
				{
				  this_type = this_type->target_type ();
				  pstate->wrap<unop_ind_operation> ();
				}
			    }

			  current_type = this_type;
			}
	;

/* end of object pascal.  */

block	:	BLOCKNAME
			{
			  if ($1.sym.symbol != 0)
			      $$ = $1.sym.symbol->value_block ();
			  else
			    {
			      std::string copy = copy_name ($1.stoken);
			      struct symtab *tem =
				  lookup_symtab (copy.c_str ());
			      if (tem)
				$$ = (tem->compunit ()->blockvector ()
				      ->static_block ());
			      else
				error (_("No file or function \"%s\"."),
				       copy.c_str ());
			    }
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

variable:	block COLONCOLON name
			{ struct block_symbol sym;

			  std::string copy = copy_name ($3);
			  sym = lookup_symbol (copy.c_str (), $1,
					       VAR_DOMAIN, NULL);
			  if (sym.symbol == 0)
			    error (_("No symbol \"%s\" in specified context."),
				   copy.c_str ());

			  pstate->push_new<var_value_operation> (sym);
			}
	;

qualified_name:	typebase COLONCOLON name
			{
			  struct type *type = $1;

			  if (type->code () != TYPE_CODE_STRUCT
			      && type->code () != TYPE_CODE_UNION)
			    error (_("`%s' is not defined as an aggregate type."),
				   type->name ());

			  pstate->push_new<scope_operation>
			    (type, copy_name ($3));
			}
	;

variable:	qualified_name
	|	COLONCOLON name
			{
			  std::string name = copy_name ($2);

			  struct block_symbol sym
			    = lookup_symbol (name.c_str (), nullptr,
					     VAR_DOMAIN, nullptr);
			  pstate->push_symbol (name.c_str (), sym);
			}
	;

variable:	name_not_typename
			{ struct block_symbol sym = $1.sym;

			  if (sym.symbol)
			    {
			      if (symbol_read_needs_frame (sym.symbol))
				pstate->block_tracker->update (sym);

			      pstate->push_new<var_value_operation> (sym);
			      current_type = sym.symbol->type (); }
			  else if ($1.is_a_field_of_this)
			    {
			      struct value * this_val;
			      struct type * this_type;
			      /* Object pascal: it hangs off of `this'.  Must
				 not inadvertently convert from a method call
				 to data ref.  */
			      pstate->block_tracker->update (sym);
			      operation_up thisop
				= make_operation<op_this_operation> ();
			      pstate->push_new<structop_operation>
				(std::move (thisop), copy_name ($1.stoken));
			      /* We need type of this.  */
			      this_val
				= value_of_this_silent (pstate->language ());
			      if (this_val)
				this_type = this_val->type ();
			      else
				this_type = NULL;
			      if (this_type)
				current_type = lookup_struct_elt_type (
				  this_type,
				  copy_name ($1.stoken).c_str (), 0);
			      else
				current_type = NULL;
			    }
			  else
			    {
			      struct bound_minimal_symbol msymbol;
			      std::string arg = copy_name ($1.stoken);

			      msymbol =
				lookup_bound_minimal_symbol (arg.c_str ());
			      if (msymbol.minsym != NULL)
				pstate->push_new<var_msym_value_operation>
				  (msymbol);
			      else if (!have_full_symbols ()
				       && !have_partial_symbols ())
				error (_("No symbol table is loaded.  "
				       "Use the \"file\" command."));
			      else
				error (_("No symbol \"%s\" in current context."),
				       arg.c_str ());
			    }
			}
	;


ptype	:	typebase
	;

/* We used to try to recognize more pointer to member types here, but
   that didn't work (shift/reduce conflicts meant that these rules never
   got executed).  The problem is that
     int (foo::bar::baz::bizzle)
   is a function type but
     int (foo::bar::baz::bizzle::*)
   is a pointer to member type.  Stroustrup loses again!  */

type	:	ptype
	;

typebase  /* Implements (approximately): (type-qualifier)* type-specifier */
	:	'^' typebase
			{ $$ = lookup_pointer_type ($2); }
	|	TYPENAME
			{ $$ = $1.type; }
	|	STRUCT name
			{ $$
			    = lookup_struct (copy_name ($2).c_str (),
					     pstate->expression_context_block);
			}
	|	CLASS name
			{ $$
			    = lookup_struct (copy_name ($2).c_str (),
					     pstate->expression_context_block);
			}
	/* "const" and "volatile" are curently ignored.  A type qualifier
	   after the type is handled in the ptype rule.  I think these could
	   be too.  */
	;

name	:	NAME { $$ = $1.stoken; }
	|	BLOCKNAME { $$ = $1.stoken; }
	|	TYPENAME { $$ = $1.stoken; }
	|	NAME_OR_INT  { $$ = $1.stoken; }
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
	;

%%

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

  int i = 0;
  int c;
  int base = input_radix;
  int unsigned_p = 0;

  /* Number of "L" suffixes encountered.  */
  int long_p = 0;

  /* We have found a "L" or "U" suffix.  */
  int found_suffix = 0;

  if (parsed_float)
    {
      /* Handle suffixes: 'f' for float, 'l' for long double.
	 FIXME: This appears to be an extension -- do we want this?  */
      if (len >= 1 && tolower (p[len - 1]) == 'f')
	{
	  putithere->typed_val_float.type
	    = parse_type (par_state)->builtin_float;
	  len--;
	}
      else if (len >= 1 && tolower (p[len - 1]) == 'l')
	{
	  putithere->typed_val_float.type
	    = parse_type (par_state)->builtin_long_double;
	  len--;
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
      return FLOAT;
    }

  /* Handle base-switching prefixes 0x, 0t, 0d, 0.  */
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
	  else
	    return ERROR;	/* Char not a digit */
	}
      if (i >= base)
	return ERROR;		/* Invalid digit in this base.  */

      if (c != 'l' && c != 'u')
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
  bool have_signed = !unsigned_p;
  bool have_int = long_p == 0;
  bool have_long = long_p <= 1;
  if (have_int && have_signed && fits_in_type (1, n, int_bits, true))
    putithere->typed_val_int.type = parse_type (par_state)->builtin_int;
  else if (have_int && fits_in_type (1, n, int_bits, false))
    putithere->typed_val_int.type
      = parse_type (par_state)->builtin_unsigned_int;
  else if (have_long && have_signed && fits_in_type (1, n, long_bits, true))
    putithere->typed_val_int.type = parse_type (par_state)->builtin_long;
  else if (have_long && fits_in_type (1, n, long_bits, false))
    putithere->typed_val_int.type
      = parse_type (par_state)->builtin_unsigned_long;
  else if (have_signed && fits_in_type (1, n, long_long_bits, true))
    putithere->typed_val_int.type
      = parse_type (par_state)->builtin_long_long;
  else if (fits_in_type (1, n, long_long_bits, false))
    putithere->typed_val_int.type
      = parse_type (par_state)->builtin_unsigned_long_long;
  else
    error (_("Numeric constant too large."));
  putithere->typed_val_int.val = n;

   return INT;
}


struct type_push
{
  struct type *stored;
  struct type_push *next;
};

static struct type_push *tp_top = NULL;

static void
push_current_type (void)
{
  struct type_push *tpnew;
  tpnew = (struct type_push *) malloc (sizeof (struct type_push));
  tpnew->next = tp_top;
  tpnew->stored = current_type;
  current_type = NULL;
  tp_top = tpnew;
}

static void
pop_current_type (void)
{
  struct type_push *tp = tp_top;
  if (tp)
    {
      current_type = tp->stored;
      tp_top = tp->next;
      free (tp);
    }
}

struct p_token
{
  const char *oper;
  int token;
  enum exp_opcode opcode;
};

static const struct p_token tokentab3[] =
  {
    {"shr", RSH, OP_NULL},
    {"shl", LSH, OP_NULL},
    {"and", ANDAND, OP_NULL},
    {"div", DIV, OP_NULL},
    {"not", NOT, OP_NULL},
    {"mod", MOD, OP_NULL},
    {"inc", INCREMENT, OP_NULL},
    {"dec", DECREMENT, OP_NULL},
    {"xor", XOR, OP_NULL}
  };

static const struct p_token tokentab2[] =
  {
    {"or", OR, OP_NULL},
    {"<>", NOTEQUAL, OP_NULL},
    {"<=", LEQ, OP_NULL},
    {">=", GEQ, OP_NULL},
    {":=", ASSIGN, OP_NULL},
    {"::", COLONCOLON, OP_NULL} };

/* Allocate uppercased var: */
/* make an uppercased copy of tokstart.  */
static char *
uptok (const char *tokstart, int namelen)
{
  int i;
  char *uptokstart = (char *)malloc(namelen+1);
  for (i = 0;i <= namelen;i++)
    {
      if ((tokstart[i]>='a' && tokstart[i]<='z'))
	uptokstart[i] = tokstart[i]-('a'-'A');
      else
	uptokstart[i] = tokstart[i];
    }
  uptokstart[namelen]='\0';
  return uptokstart;
}

/* Read one token, getting characters through lexptr.  */

static int
yylex (void)
{
  int c;
  int namelen;
  const char *tokstart;
  char *uptokstart;
  const char *tokptr;
  int explen, tempbufindex;
  static char *tempbuf;
  static int tempbufsize;

 retry:

  pstate->prev_lexptr = pstate->lexptr;

  tokstart = pstate->lexptr;
  explen = strlen (pstate->lexptr);

  /* See if it is a special token of length 3.  */
  if (explen > 2)
    for (const auto &token : tokentab3)
      if (strncasecmp (tokstart, token.oper, 3) == 0
	  && (!isalpha (token.oper[0]) || explen == 3
	      || (!isalpha (tokstart[3])
		  && !isdigit (tokstart[3]) && tokstart[3] != '_')))
	{
	  pstate->lexptr += 3;
	  yylval.opcode = token.opcode;
	  return token.token;
	}

  /* See if it is a special token of length 2.  */
  if (explen > 1)
    for (const auto &token : tokentab2)
      if (strncasecmp (tokstart, token.oper, 2) == 0
	  && (!isalpha (token.oper[0]) || explen == 2
	      || (!isalpha (tokstart[2])
		  && !isdigit (tokstart[2]) && tokstart[2] != '_')))
	{
	  pstate->lexptr += 2;
	  yylval.opcode = token.opcode;
	  return token.token;
	}

  switch (c = *tokstart)
    {
    case 0:
      if (search_field && pstate->parse_completion)
	return COMPLETE;
      else
       return 0;

    case ' ':
    case '\t':
    case '\n':
      pstate->lexptr++;
      goto retry;

    case '\'':
      /* We either have a character constant ('0' or '\177' for example)
	 or we have a quoted symbol reference ('foo(int,int)' in object pascal
	 for example).  */
      pstate->lexptr++;
      c = *pstate->lexptr++;
      if (c == '\\')
	c = parse_escape (pstate->gdbarch (), &pstate->lexptr);
      else if (c == '\'')
	error (_("Empty character constant."));

      yylval.typed_val_int.val = c;
      yylval.typed_val_int.type = parse_type (pstate)->builtin_char;

      c = *pstate->lexptr++;
      if (c != '\'')
	{
	  namelen = skip_quoted (tokstart) - tokstart;
	  if (namelen > 2)
	    {
	      pstate->lexptr = tokstart + namelen;
	      if (pstate->lexptr[-1] != '\'')
		error (_("Unmatched single quote."));
	      namelen -= 2;
	      tokstart++;
	      uptokstart = uptok(tokstart,namelen);
	      goto tryname;
	    }
	  error (_("Invalid character constant."));
	}
      return INT;

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
	{
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
	else if (c == '0' && (p[1]=='t' || p[1]=='T'
			      || p[1]=='d' || p[1]=='D'))
	  {
	    p += 2;
	    hex = 0;
	  }

	for (;; ++p)
	  {
	    /* This test includes !hex because 'e' is a valid hex digit
	       and thus does not indicate a floating point number when
	       the radix is hex.  */
	    if (!hex && !got_e && (*p == 'e' || *p == 'E'))
	      got_dot = got_e = 1;
	    /* This test does not include !hex, because a '.' always indicates
	       a decimal floating point number regardless of the radix.  */
	    else if (!got_dot && *p == '.')
	      got_dot = 1;
	    else if (got_e && (p[-1] == 'e' || p[-1] == 'E')
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
	toktype = parse_number (pstate, tokstart,
				p - tokstart, got_dot | got_e, &yylval);
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

    case '"':

      /* Build the gdb internal form of the input string in tempbuf,
	 translating any standard C escape forms seen.  Note that the
	 buffer is null byte terminated *only* for the convenience of
	 debugging gdb itself and printing the buffer contents when
	 the buffer contains no embedded nulls.  Gdb does not depend
	 upon the buffer being null byte terminated, it uses the length
	 string instead.  This allows gdb to handle C strings (as well
	 as strings in other languages) with embedded null bytes.  */

      tokptr = ++tokstart;
      tempbufindex = 0;

      do {
	/* Grow the static temp buffer if necessary, including allocating
	   the first one on demand.  */
	if (tempbufindex + 1 >= tempbufsize)
	  {
	    tempbuf = (char *) realloc (tempbuf, tempbufsize += 64);
	  }

	switch (*tokptr)
	  {
	  case '\0':
	  case '"':
	    /* Do nothing, loop will terminate.  */
	    break;
	  case '\\':
	    ++tokptr;
	    c = parse_escape (pstate->gdbarch (), &tokptr);
	    if (c == -1)
	      {
		continue;
	      }
	    tempbuf[tempbufindex++] = c;
	    break;
	  default:
	    tempbuf[tempbufindex++] = *tokptr++;
	    break;
	  }
      } while ((*tokptr != '"') && (*tokptr != '\0'));
      if (*tokptr++ != '"')
	{
	  error (_("Unterminated string in expression."));
	}
      tempbuf[tempbufindex] = '\0';	/* See note above.  */
      yylval.sval.ptr = tempbuf;
      yylval.sval.length = tempbufindex;
      pstate->lexptr = tokptr;
      return (STRING);
    }

  if (!(c == '_' || c == '$'
	|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
    /* We must have come across a bad character (e.g. ';').  */
    error (_("Invalid character '%c' in expression."), c);

  /* It's a name.  See how long it is.  */
  namelen = 0;
  for (c = tokstart[namelen];
       (c == '_' || c == '$' || (c >= '0' && c <= '9')
	|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '<');)
    {
      /* Template parameter lists are part of the name.
	 FIXME: This mishandles `print $a<4&&$a>3'.  */
      if (c == '<')
	{
	  int i = namelen;
	  int nesting_level = 1;
	  while (tokstart[++i])
	    {
	      if (tokstart[i] == '<')
		nesting_level++;
	      else if (tokstart[i] == '>')
		{
		  if (--nesting_level == 0)
		    break;
		}
	    }
	  if (tokstart[i] == '>')
	    namelen = i;
	  else
	    break;
	}

      /* do NOT uppercase internals because of registers !!!  */
      c = tokstart[++namelen];
    }

  uptokstart = uptok(tokstart,namelen);

  /* The token "if" terminates the expression and is NOT
     removed from the input stream.  */
  if (namelen == 2 && uptokstart[0] == 'I' && uptokstart[1] == 'F')
    {
      free (uptokstart);
      return 0;
    }

  pstate->lexptr += namelen;

  tryname:

  /* Catch specific keywords.  Should be done with a data structure.  */
  switch (namelen)
    {
    case 6:
      if (strcmp (uptokstart, "OBJECT") == 0)
	{
	  free (uptokstart);
	  return CLASS;
	}
      if (strcmp (uptokstart, "RECORD") == 0)
	{
	  free (uptokstart);
	  return STRUCT;
	}
      if (strcmp (uptokstart, "SIZEOF") == 0)
	{
	  free (uptokstart);
	  return SIZEOF;
	}
      break;
    case 5:
      if (strcmp (uptokstart, "CLASS") == 0)
	{
	  free (uptokstart);
	  return CLASS;
	}
      if (strcmp (uptokstart, "FALSE") == 0)
	{
	  yylval.lval = 0;
	  free (uptokstart);
	  return FALSEKEYWORD;
	}
      break;
    case 4:
      if (strcmp (uptokstart, "TRUE") == 0)
	{
	  yylval.lval = 1;
	  free (uptokstart);
  	  return TRUEKEYWORD;
	}
      if (strcmp (uptokstart, "SELF") == 0)
	{
	  /* Here we search for 'this' like
	     inserted in FPC stabs debug info.  */
	  static const char this_name[] = "this";

	  if (lookup_symbol (this_name, pstate->expression_context_block,
			     VAR_DOMAIN, NULL).symbol)
	    {
	      free (uptokstart);
	      return THIS;
	    }
	}
      break;
    default:
      break;
    }

  yylval.sval.ptr = tokstart;
  yylval.sval.length = namelen;

  if (*tokstart == '$')
    {
      free (uptokstart);
      return DOLLAR_VARIABLE;
    }

  /* Use token-type BLOCKNAME for symbols that happen to be defined as
     functions or symtabs.  If this is not so, then ...
     Use token-type TYPENAME for symbols that happen to be defined
     currently as names of types; NAME for other symbols.
     The caller is not constrained to care about the distinction.  */
  {
    std::string tmp = copy_name (yylval.sval);
    struct symbol *sym;
    struct field_of_this_result is_a_field_of_this;
    int is_a_field = 0;
    int hextype;

    is_a_field_of_this.type = NULL;
    if (search_field && current_type)
      is_a_field = (lookup_struct_elt_type (current_type,
					    tmp.c_str (), 1) != NULL);
    if (is_a_field)
      sym = NULL;
    else
      sym = lookup_symbol (tmp.c_str (), pstate->expression_context_block,
			   VAR_DOMAIN, &is_a_field_of_this).symbol;
    /* second chance uppercased (as Free Pascal does).  */
    if (!sym && is_a_field_of_this.type == NULL && !is_a_field)
      {
       for (int i = 0; i <= namelen; i++)
	 {
	   if ((tmp[i] >= 'a' && tmp[i] <= 'z'))
	     tmp[i] -= ('a'-'A');
	 }
       if (search_field && current_type)
	 is_a_field = (lookup_struct_elt_type (current_type,
					       tmp.c_str (), 1) != NULL);
       if (is_a_field)
	 sym = NULL;
       else
	 sym = lookup_symbol (tmp.c_str (), pstate->expression_context_block,
			      VAR_DOMAIN, &is_a_field_of_this).symbol;
      }
    /* Third chance Capitalized (as GPC does).  */
    if (!sym && is_a_field_of_this.type == NULL && !is_a_field)
      {
       for (int i = 0; i <= namelen; i++)
	 {
	   if (i == 0)
	     {
	      if ((tmp[i] >= 'a' && tmp[i] <= 'z'))
		tmp[i] -= ('a'-'A');
	     }
	   else
	   if ((tmp[i] >= 'A' && tmp[i] <= 'Z'))
	     tmp[i] -= ('A'-'a');
	  }
       if (search_field && current_type)
	 is_a_field = (lookup_struct_elt_type (current_type,
					       tmp.c_str (), 1) != NULL);
       if (is_a_field)
	 sym = NULL;
       else
	 sym = lookup_symbol (tmp.c_str (), pstate->expression_context_block,
			      VAR_DOMAIN, &is_a_field_of_this).symbol;
      }

    if (is_a_field || (is_a_field_of_this.type != NULL))
      {
	tempbuf = (char *) realloc (tempbuf, namelen + 1);
	strncpy (tempbuf, tmp.c_str (), namelen);
	tempbuf [namelen] = 0;
	yylval.sval.ptr = tempbuf;
	yylval.sval.length = namelen;
	yylval.ssym.sym.symbol = NULL;
	yylval.ssym.sym.block = NULL;
	free (uptokstart);
	yylval.ssym.is_a_field_of_this = is_a_field_of_this.type != NULL;
	if (is_a_field)
	  return FIELDNAME;
	else
	  return NAME;
      }
    /* Call lookup_symtab, not lookup_partial_symtab, in case there are
       no psymtabs (coff, xcoff, or some future change to blow away the
       psymtabs once once symbols are read).  */
    if ((sym && sym->aclass () == LOC_BLOCK)
	|| lookup_symtab (tmp.c_str ()))
      {
	yylval.ssym.sym.symbol = sym;
	yylval.ssym.sym.block = NULL;
	yylval.ssym.is_a_field_of_this = is_a_field_of_this.type != NULL;
	free (uptokstart);
	return BLOCKNAME;
      }
    if (sym && sym->aclass () == LOC_TYPEDEF)
	{
#if 1
	  /* Despite the following flaw, we need to keep this code enabled.
	     Because we can get called from check_stub_method, if we don't
	     handle nested types then it screws many operations in any
	     program which uses nested types.  */
	  /* In "A::x", if x is a member function of A and there happens
	     to be a type (nested or not, since the stabs don't make that
	     distinction) named x, then this code incorrectly thinks we
	     are dealing with nested types rather than a member function.  */

	  const char *p;
	  const char *namestart;
	  struct symbol *best_sym;

	  /* Look ahead to detect nested types.  This probably should be
	     done in the grammar, but trying seemed to introduce a lot
	     of shift/reduce and reduce/reduce conflicts.  It's possible
	     that it could be done, though.  Or perhaps a non-grammar, but
	     less ad hoc, approach would work well.  */

	  /* Since we do not currently have any way of distinguishing
	     a nested type from a non-nested one (the stabs don't tell
	     us whether a type is nested), we just ignore the
	     containing type.  */

	  p = pstate->lexptr;
	  best_sym = sym;
	  while (1)
	    {
	      /* Skip whitespace.  */
	      while (*p == ' ' || *p == '\t' || *p == '\n')
		++p;
	      if (*p == ':' && p[1] == ':')
		{
		  /* Skip the `::'.  */
		  p += 2;
		  /* Skip whitespace.  */
		  while (*p == ' ' || *p == '\t' || *p == '\n')
		    ++p;
		  namestart = p;
		  while (*p == '_' || *p == '$' || (*p >= '0' && *p <= '9')
			 || (*p >= 'a' && *p <= 'z')
			 || (*p >= 'A' && *p <= 'Z'))
		    ++p;
		  if (p != namestart)
		    {
		      struct symbol *cur_sym;
		      /* As big as the whole rest of the expression, which is
			 at least big enough.  */
		      char *ncopy
			= (char *) alloca (tmp.size () + strlen (namestart)
					   + 3);
		      char *tmp1;

		      tmp1 = ncopy;
		      memcpy (tmp1, tmp.c_str (), tmp.size ());
		      tmp1 += tmp.size ();
		      memcpy (tmp1, "::", 2);
		      tmp1 += 2;
		      memcpy (tmp1, namestart, p - namestart);
		      tmp1[p - namestart] = '\0';
		      cur_sym
			= lookup_symbol (ncopy,
					 pstate->expression_context_block,
					 VAR_DOMAIN, NULL).symbol;
		      if (cur_sym)
			{
			  if (cur_sym->aclass () == LOC_TYPEDEF)
			    {
			      best_sym = cur_sym;
			      pstate->lexptr = p;
			    }
			  else
			    break;
			}
		      else
			break;
		    }
		  else
		    break;
		}
	      else
		break;
	    }

	  yylval.tsym.type = best_sym->type ();
#else /* not 0 */
	  yylval.tsym.type = sym->type ();
#endif /* not 0 */
	  free (uptokstart);
	  return TYPENAME;
	}
    yylval.tsym.type
      = language_lookup_primitive_type (pstate->language (),
					pstate->gdbarch (), tmp.c_str ());
    if (yylval.tsym.type != NULL)
      {
	free (uptokstart);
	return TYPENAME;
      }

    /* Input names that aren't symbols but ARE valid hex numbers,
       when the input radix permits them, can be names or numbers
       depending on the parse.  Note we support radixes > 16 here.  */
    if (!sym
	&& ((tokstart[0] >= 'a' && tokstart[0] < 'a' + input_radix - 10)
	    || (tokstart[0] >= 'A' && tokstart[0] < 'A' + input_radix - 10)))
      {
 	YYSTYPE newlval;	/* Its value is ignored.  */
	hextype = parse_number (pstate, tokstart, namelen, 0, &newlval);
	if (hextype == INT)
	  {
	    yylval.ssym.sym.symbol = sym;
	    yylval.ssym.sym.block = NULL;
	    yylval.ssym.is_a_field_of_this = is_a_field_of_this.type != NULL;
	    free (uptokstart);
	    return NAME_OR_INT;
	  }
      }

    free(uptokstart);
    /* Any other kind of symbol.  */
    yylval.ssym.sym.symbol = sym;
    yylval.ssym.sym.block = NULL;
    return NAME;
  }
}

/* See language.h.  */

int
pascal_language::parser (struct parser_state *par_state) const
{
  /* Setting up the parser state.  */
  scoped_restore pstate_restore = make_scoped_restore (&pstate);
  gdb_assert (par_state != NULL);
  pstate = par_state;
  paren_depth = 0;

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
