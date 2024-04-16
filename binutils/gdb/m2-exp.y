/* YACC grammar for Modula-2 expressions, for GDB.
   Copyright (C) 1986-2024 Free Software Foundation, Inc.
   Generated from expread.y (now c-exp.y) and contributed by the Department
   of Computer Science at the State University of New York at Buffalo, 1991.

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

/* Parse a Modula-2 expression from text in a string,
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
#include "language.h"
#include "value.h"
#include "parser-defs.h"
#include "m2-lang.h"
#include "block.h"
#include "m2-exp.h"

#define parse_type(ps) builtin_type (ps->gdbarch ())
#define parse_m2_type(ps) builtin_m2_type (ps->gdbarch ())

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror,
   etc).  */
#define GDB_YY_REMAP_PREFIX m2_
#include "yy-remap.h"

/* The state of the parser, used internally when we are parsing the
   expression.  */

static struct parser_state *pstate = NULL;

int yyparse (void);

static int yylex (void);

static void yyerror (const char *);

static int parse_number (int);

/* The sign of the number being parsed.  */
static int number_sign = 1;

using namespace expr;
%}

/* Although the yacc "value" of an expression is not used,
   since the result is stored in the structure being created,
   other node types do have values.  */

%union
  {
    LONGEST lval;
    ULONGEST ulval;
    gdb_byte val[16];
    struct symbol *sym;
    struct type *tval;
    struct stoken sval;
    int voidval;
    const struct block *bval;
    enum exp_opcode opcode;
    struct internalvar *ivar;

    struct type **tvec;
    int *ivec;
  }

%type <voidval> exp type_exp start set
%type <voidval> variable
%type <tval> type
%type <bval> block 
%type <sym> fblock 

%token <lval> INT HEX ERROR
%token <ulval> UINT M2_TRUE M2_FALSE CHAR
%token <val> FLOAT

/* Both NAME and TYPENAME tokens represent symbols in the input,
   and both convey their data as strings.
   But a TYPENAME is a string that happens to be defined as a typedef
   or builtin type name (such as int or char)
   and a NAME is any other symbol.

   Contexts where this distinction is not important can use the
   nonterminal "name", which matches either NAME or TYPENAME.  */

%token <sval> STRING
%token <sval> NAME BLOCKNAME IDENT VARNAME
%token <sval> TYPENAME

%token SIZE CAP ORD HIGH ABS MIN_FUNC MAX_FUNC FLOAT_FUNC VAL CHR ODD TRUNC
%token TSIZE
%token INC DEC INCL EXCL

/* The GDB scope operator */
%token COLONCOLON

%token <sval> DOLLAR_VARIABLE

/* M2 tokens */
%left ','
%left ABOVE_COMMA
%nonassoc ASSIGN
%left '<' '>' LEQ GEQ '=' NOTEQUAL '#' IN
%left OROR
%left LOGICAL_AND '&'
%left '@'
%left '+' '-'
%left '*' '/' DIV MOD
%right UNARY
%right '^' DOT '[' '('
%right NOT '~'
%left COLONCOLON QID
/* This is not an actual token ; it is used for precedence. 
%right QID
*/


%%

start   :	exp
	|	type_exp
	;

type_exp:	type
		{ pstate->push_new<type_operation> ($1); }
	;

/* Expressions */

exp     :       exp '^'   %prec UNARY
			{ pstate->wrap<unop_ind_operation> (); }
	;

exp	:	'-'
			{ number_sign = -1; }
		exp    %prec UNARY
			{ number_sign = 1;
			  pstate->wrap<unary_neg_operation> (); }
	;

exp	:	'+' exp    %prec UNARY
		{ pstate->wrap<unary_plus_operation> (); }
	;

exp	:	not_exp exp %prec UNARY
			{ pstate->wrap<unary_logical_not_operation> (); }
	;

not_exp	:	NOT
	|	'~'
	;

exp	:	CAP '(' exp ')'
			{ error (_("CAP function is not implemented")); }
	;

exp	:	ORD '(' exp ')'
			{ error (_("ORD function is not implemented")); }
	;

exp	:	ABS '(' exp ')'
			{ error (_("ABS function is not implemented")); }
	;

exp	: 	HIGH '(' exp ')'
			{ pstate->wrap<m2_unop_high_operation> (); }
	;

exp 	:	MIN_FUNC '(' type ')'
			{ error (_("MIN function is not implemented")); }
	;

exp	: 	MAX_FUNC '(' type ')'
			{ error (_("MAX function is not implemented")); }
	;

exp	:	FLOAT_FUNC '(' exp ')'
			{ error (_("FLOAT function is not implemented")); }
	;

exp	:	VAL '(' type ',' exp ')'
			{ error (_("VAL function is not implemented")); }
	;

exp	:	CHR '(' exp ')'
			{ error (_("CHR function is not implemented")); }
	;

exp	:	ODD '(' exp ')'
			{ error (_("ODD function is not implemented")); }
	;

exp	:	TRUNC '(' exp ')'
			{ error (_("TRUNC function is not implemented")); }
	;

exp	:	TSIZE '(' exp ')'
			{ pstate->wrap<unop_sizeof_operation> (); }
	;

exp	:	SIZE exp       %prec UNARY
			{ pstate->wrap<unop_sizeof_operation> (); }
	;


exp	:	INC '(' exp ')'
			{ pstate->wrap<preinc_operation> (); }
	;

exp	:	INC '(' exp ',' exp ')'
			{
			  operation_up rhs = pstate->pop ();
			  operation_up lhs = pstate->pop ();
			  pstate->push_new<assign_modify_operation>
			    (BINOP_ADD, std::move (lhs), std::move (rhs));
			}
	;

exp	:	DEC '(' exp ')'
			{ pstate->wrap<predec_operation> (); }
	;

exp	:	DEC '(' exp ',' exp ')'
			{
			  operation_up rhs = pstate->pop ();
			  operation_up lhs = pstate->pop ();
			  pstate->push_new<assign_modify_operation>
			    (BINOP_SUB, std::move (lhs), std::move (rhs));
			}
	;

exp	:	exp DOT NAME
			{
			  pstate->push_new<structop_operation>
			    (pstate->pop (), copy_name ($3));
			}
;

exp	:	set
	;

exp	:	exp IN set
			{ error (_("Sets are not implemented."));}
	;

exp	:	INCL '(' exp ',' exp ')'
			{ error (_("Sets are not implemented."));}
	;

exp	:	EXCL '(' exp ',' exp ')'
			{ error (_("Sets are not implemented."));}
	;

set	:	'{' arglist '}'
			{ error (_("Sets are not implemented."));}
	|	type '{' arglist '}'
			{ error (_("Sets are not implemented."));}
	;


/* Modula-2 array subscript notation [a,b,c...].  */
exp     :       exp '['
			/* This function just saves the number of arguments
			   that follow in the list.  It is *not* specific to
			   function types */
			{ pstate->start_arglist(); }
		non_empty_arglist ']'  %prec DOT
			{
			  gdb_assert (pstate->arglist_len > 0);
			  std::vector<operation_up> args
			    = pstate->pop_vector (pstate->end_arglist ());
			  pstate->push_new<multi_subscript_operation>
			    (pstate->pop (), std::move (args));
			}
	;

exp	:	exp '('
			/* This is to save the value of arglist_len
			   being accumulated by an outer function call.  */
			{ pstate->start_arglist (); }
		arglist ')'	%prec DOT
			{
			  std::vector<operation_up> args
			    = pstate->pop_vector (pstate->end_arglist ());
			  pstate->push_new<funcall_operation>
			    (pstate->pop (), std::move (args));
			}
	;

arglist	:
	;

arglist	:	exp
			{ pstate->arglist_len = 1; }
	;

arglist	:	arglist ',' exp   %prec ABOVE_COMMA
			{ pstate->arglist_len++; }
	;

non_empty_arglist
	:       exp
			{ pstate->arglist_len = 1; }
	;

non_empty_arglist
	:       non_empty_arglist ',' exp %prec ABOVE_COMMA
     	       	    	{ pstate->arglist_len++; }
     	;

/* GDB construct */
exp	:	'{' type '}' exp  %prec UNARY
			{
			  pstate->push_new<unop_memval_operation>
			    (pstate->pop (), $2);
			}
	;

exp     :       type '(' exp ')' %prec UNARY
			{
			  pstate->push_new<unop_cast_operation>
			    (pstate->pop (), $1);
			}
	;

exp	:	'(' exp ')'
			{ }
	;

/* Binary operators in order of decreasing precedence.  Note that some
   of these operators are overloaded!  (ie. sets) */

/* GDB construct */
exp	:	exp '@' exp
			{ pstate->wrap2<repeat_operation> (); }
	;

exp	:	exp '*' exp
			{ pstate->wrap2<mul_operation> (); }
	;

exp	:	exp '/' exp
			{ pstate->wrap2<div_operation> (); }
	;

exp     :       exp DIV exp
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

exp	:	exp '=' exp
			{ pstate->wrap2<equal_operation> (); }
	;

exp	:	exp NOTEQUAL exp
			{ pstate->wrap2<notequal_operation> (); }
	|       exp '#' exp
			{ pstate->wrap2<notequal_operation> (); }
	;

exp	:	exp LEQ exp
			{ pstate->wrap2<leq_operation> (); }
	;

exp	:	exp GEQ exp
			{ pstate->wrap2<geq_operation> (); }
	;

exp	:	exp '<' exp
			{ pstate->wrap2<less_operation> (); }
	;

exp	:	exp '>' exp
			{ pstate->wrap2<gtr_operation> (); }
	;

exp	:	exp LOGICAL_AND exp
			{ pstate->wrap2<logical_and_operation> (); }
	;

exp	:	exp OROR exp
			{ pstate->wrap2<logical_or_operation> (); }
	;

exp	:	exp ASSIGN exp
			{ pstate->wrap2<assign_operation> (); }
	;


/* Constants */

exp	:	M2_TRUE
			{ pstate->push_new<bool_operation> ($1); }
	;

exp	:	M2_FALSE
			{ pstate->push_new<bool_operation> ($1); }
	;

exp	:	INT
			{
			  pstate->push_new<long_const_operation>
			    (parse_m2_type (pstate)->builtin_int, $1);
			}
	;

exp	:	UINT
			{
			  pstate->push_new<long_const_operation>
			    (parse_m2_type (pstate)->builtin_card, $1);
			}
	;

exp	:	CHAR
			{
			  pstate->push_new<long_const_operation>
			    (parse_m2_type (pstate)->builtin_char, $1);
			}
	;


exp	:	FLOAT
			{
			  float_data data;
			  std::copy (std::begin ($1), std::end ($1),
				     std::begin (data));
			  pstate->push_new<float_const_operation>
			    (parse_m2_type (pstate)->builtin_real, data);
			}
	;

exp	:	variable
	;

exp	:	SIZE '(' type ')'	%prec UNARY
			{
			  pstate->push_new<long_const_operation>
			    (parse_m2_type (pstate)->builtin_int,
			     $3->length ());
			}
	;

exp	:	STRING
			{ error (_("strings are not implemented")); }
	;

/* This will be used for extensions later.  Like adding modules.  */
block	:	fblock	
			{ $$ = $1->value_block (); }
	;

fblock	:	BLOCKNAME
			{ struct symbol *sym
			    = lookup_symbol (copy_name ($1).c_str (),
					     pstate->expression_context_block,
					     VAR_DOMAIN, 0).symbol;
			  $$ = sym;}
	;
			     

/* GDB scope operator */
fblock	:	block COLONCOLON BLOCKNAME
			{ struct symbol *tem
			    = lookup_symbol (copy_name ($3).c_str (), $1,
					     VAR_DOMAIN, 0).symbol;
			  if (!tem || tem->aclass () != LOC_BLOCK)
			    error (_("No function \"%s\" in specified context."),
				   copy_name ($3).c_str ());
			  $$ = tem;
			}
	;

/* Useful for assigning to PROCEDURE variables */
variable:	fblock
			{
			  block_symbol sym { $1, nullptr };
			  pstate->push_new<var_value_operation> (sym);
			}
	;

/* GDB internal ($foo) variable */
variable:	DOLLAR_VARIABLE
			{ pstate->push_dollar ($1); }
	;

/* GDB scope operator */
variable:	block COLONCOLON NAME
			{ struct block_symbol sym
			    = lookup_symbol (copy_name ($3).c_str (), $1,
					     VAR_DOMAIN, 0);

			  if (sym.symbol == 0)
			    error (_("No symbol \"%s\" in specified context."),
				   copy_name ($3).c_str ());
			  if (symbol_read_needs_frame (sym.symbol))
			    pstate->block_tracker->update (sym);

			  pstate->push_new<var_value_operation> (sym);
			}
	;

/* Base case for variables.  */
variable:	NAME
			{ struct block_symbol sym;
			  struct field_of_this_result is_a_field_of_this;

			  std::string name = copy_name ($1);
			  sym
			    = lookup_symbol (name.c_str (),
					     pstate->expression_context_block,
					     VAR_DOMAIN,
					     &is_a_field_of_this);

			  pstate->push_symbol (name.c_str (), sym);
			}
	;

type
	:	TYPENAME
			{ $$
			    = lookup_typename (pstate->language (),
					       copy_name ($1).c_str (),
					       pstate->expression_context_block,
					       0);
			}

	;

%%

/* Take care of parsing a number (anything that starts with a digit).
   Set yylval and return the token type; update lexptr.
   LEN is the number of characters in it.  */

/*** Needs some error checking for the float case ***/

static int
parse_number (int olen)
{
  const char *p = pstate->lexptr;
  ULONGEST n = 0;
  ULONGEST prevn = 0;
  int c,i,ischar=0;
  int base = input_radix;
  int len = olen;

  if(p[len-1] == 'H')
  {
     base = 16;
     len--;
  }
  else if(p[len-1] == 'C' || p[len-1] == 'B')
  {
     base = 8;
     ischar = p[len-1] == 'C';
     len--;
  }

  /* Scan the number */
  for (c = 0; c < len; c++)
  {
    if (p[c] == '.' && base == 10)
      {
	/* It's a float since it contains a point.  */
	if (!parse_float (p, len,
			  parse_m2_type (pstate)->builtin_real,
			  yylval.val))
	  return ERROR;

	pstate->lexptr += len;
	return FLOAT;
      }
    if (p[c] == '.' && base != 10)
       error (_("Floating point numbers must be base 10."));
    if (base == 10 && (p[c] < '0' || p[c] > '9'))
       error (_("Invalid digit \'%c\' in number."),p[c]);
 }

  while (len-- > 0)
    {
      c = *p++;
      n *= base;
      if( base == 8 && (c == '8' || c == '9'))
	 error (_("Invalid digit \'%c\' in octal number."),c);
      if (c >= '0' && c <= '9')
	i = c - '0';
      else
	{
	  if (base == 16 && c >= 'A' && c <= 'F')
	    i = c - 'A' + 10;
	  else
	     return ERROR;
	}
      n+=i;
      if(i >= base)
	 return ERROR;
      if (n == 0 && prevn == 0)
	;
      else if (RANGE_CHECK && prevn >= n)
	range_error (_("Overflow on numeric constant."));

	 prevn=n;
    }

  pstate->lexptr = p;
  if(*p == 'B' || *p == 'C' || *p == 'H')
     pstate->lexptr++;			/* Advance past B,C or H */

  if (ischar)
  {
     yylval.ulval = n;
     return CHAR;
  }

  int int_bits = gdbarch_int_bit (pstate->gdbarch ());
  bool have_signed = number_sign == -1;
  bool have_unsigned = number_sign == 1;
  if (have_signed && fits_in_type (number_sign, n, int_bits, true))
    {
      yylval.lval = n;
      return INT;
    }
  else if (have_unsigned && fits_in_type (number_sign, n, int_bits, false))
    {
      yylval.ulval = n;
      return UINT;
    }
  else
    error (_("Overflow on numeric constant."));
}


/* Some tokens */

static struct
{
   char name[2];
   int token;
} tokentab2[] =
{
    { {'<', '>'},    NOTEQUAL 	},
    { {':', '='},    ASSIGN	},
    { {'<', '='},    LEQ	},
    { {'>', '='},    GEQ	},
    { {':', ':'},    COLONCOLON },

};

/* Some specific keywords */

struct keyword {
   char keyw[10];
   int token;
};

static struct keyword keytab[] =
{
    {"OR" ,   OROR	 },
    {"IN",    IN         },/* Note space after IN */
    {"AND",   LOGICAL_AND},
    {"ABS",   ABS	 },
    {"CHR",   CHR	 },
    {"DEC",   DEC	 },
    {"NOT",   NOT	 },
    {"DIV",   DIV    	 },
    {"INC",   INC	 },
    {"MAX",   MAX_FUNC	 },
    {"MIN",   MIN_FUNC	 },
    {"MOD",   MOD	 },
    {"ODD",   ODD	 },
    {"CAP",   CAP	 },
    {"ORD",   ORD	 },
    {"VAL",   VAL	 },
    {"EXCL",  EXCL	 },
    {"HIGH",  HIGH       },
    {"INCL",  INCL	 },
    {"SIZE",  SIZE       },
    {"FLOAT", FLOAT_FUNC },
    {"TRUNC", TRUNC	 },
    {"TSIZE", SIZE       },
};


/* Depth of parentheses.  */
static int paren_depth;

/* Read one token, getting characters through lexptr.  */

/* This is where we will check to make sure that the language and the
   operators used are compatible  */

static int
yylex (void)
{
  int c;
  int namelen;
  int i;
  const char *tokstart;
  char quote;

 retry:

  pstate->prev_lexptr = pstate->lexptr;

  tokstart = pstate->lexptr;


  /* See if it is a special token of length 2 */
  for( i = 0 ; i < (int) (sizeof tokentab2 / sizeof tokentab2[0]) ; i++)
     if (strncmp (tokentab2[i].name, tokstart, 2) == 0)
     {
	pstate->lexptr += 2;
	return tokentab2[i].token;
     }

  switch (c = *tokstart)
    {
    case 0:
      return 0;

    case ' ':
    case '\t':
    case '\n':
      pstate->lexptr++;
      goto retry;

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
      if (pstate->lexptr[1] >= '0' && pstate->lexptr[1] <= '9')
	break;			/* Falls into number code.  */
      else
      {
	 pstate->lexptr++;
	 return DOT;
      }

/* These are character tokens that appear as-is in the YACC grammar */
    case '+':
    case '-':
    case '*':
    case '/':
    case '^':
    case '<':
    case '>':
    case '[':
    case ']':
    case '=':
    case '{':
    case '}':
    case '#':
    case '@':
    case '~':
    case '&':
      pstate->lexptr++;
      return c;

    case '\'' :
    case '"':
      quote = c;
      for (namelen = 1; (c = tokstart[namelen]) != quote && c != '\0'; namelen++)
	if (c == '\\')
	  {
	    c = tokstart[++namelen];
	    if (c >= '0' && c <= '9')
	      {
		c = tokstart[++namelen];
		if (c >= '0' && c <= '9')
		  c = tokstart[++namelen];
	      }
	  }
      if(c != quote)
	 error (_("Unterminated string or character constant."));
      yylval.sval.ptr = tokstart + 1;
      yylval.sval.length = namelen - 1;
      pstate->lexptr += namelen + 1;

      if(namelen == 2)  	/* Single character */
      {
	   yylval.ulval = tokstart[1];
	   return CHAR;
      }
      else
	 return STRING;
    }

  /* Is it a number?  */
  /* Note:  We have already dealt with the case of the token '.'.
     See case '.' above.  */
  if ((c >= '0' && c <= '9'))
    {
      /* It's a number.  */
      int got_dot = 0, got_e = 0;
      const char *p = tokstart;
      int toktype;

      for (++p ;; ++p)
	{
	  if (!got_e && (*p == 'e' || *p == 'E'))
	    got_dot = got_e = 1;
	  else if (!got_dot && *p == '.')
	    got_dot = 1;
	  else if (got_e && (p[-1] == 'e' || p[-1] == 'E')
		   && (*p == '-' || *p == '+'))
	    /* This is the sign of the exponent, not the end of the
	       number.  */
	    continue;
	  else if ((*p < '0' || *p > '9') &&
		   (*p < 'A' || *p > 'F') &&
		   (*p != 'H'))  /* Modula-2 hexadecimal number */
	    break;
	}
	toktype = parse_number (p - tokstart);
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

  if (!(c == '_' || c == '$'
	|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
    /* We must have come across a bad character (e.g. ';').  */
    error (_("Invalid character '%c' in expression."), c);

  /* It's a name.  See how long it is.  */
  namelen = 0;
  for (c = tokstart[namelen];
       (c == '_' || c == '$' || (c >= '0' && c <= '9')
	|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
       c = tokstart[++namelen])
    ;

  /* The token "if" terminates the expression and is NOT
     removed from the input stream.  */
  if (namelen == 2 && tokstart[0] == 'i' && tokstart[1] == 'f')
    {
      return 0;
    }

  pstate->lexptr += namelen;

  /*  Lookup special keywords */
  for(i = 0 ; i < (int) (sizeof(keytab) / sizeof(keytab[0])) ; i++)
     if (namelen == strlen (keytab[i].keyw)
	 && strncmp (tokstart, keytab[i].keyw, namelen) == 0)
	   return keytab[i].token;

  yylval.sval.ptr = tokstart;
  yylval.sval.length = namelen;

  if (*tokstart == '$')
    return DOLLAR_VARIABLE;

  /* Use token-type BLOCKNAME for symbols that happen to be defined as
     functions.  If this is not so, then ...
     Use token-type TYPENAME for symbols that happen to be defined
     currently as names of types; NAME for other symbols.
     The caller is not constrained to care about the distinction.  */
 {
    std::string tmp = copy_name (yylval.sval);
    struct symbol *sym;

    if (lookup_symtab (tmp.c_str ()))
      return BLOCKNAME;
    sym = lookup_symbol (tmp.c_str (), pstate->expression_context_block,
			 VAR_DOMAIN, 0).symbol;
    if (sym && sym->aclass () == LOC_BLOCK)
      return BLOCKNAME;
    if (lookup_typename (pstate->language (),
			 tmp.c_str (), pstate->expression_context_block, 1))
      return TYPENAME;

    if(sym)
    {
      switch(sym->aclass ())
       {
       case LOC_STATIC:
       case LOC_REGISTER:
       case LOC_ARG:
       case LOC_REF_ARG:
       case LOC_REGPARM_ADDR:
       case LOC_LOCAL:
       case LOC_CONST:
       case LOC_CONST_BYTES:
       case LOC_OPTIMIZED_OUT:
       case LOC_COMPUTED:
	  return NAME;

       case LOC_TYPEDEF:
	  return TYPENAME;

       case LOC_BLOCK:
	  return BLOCKNAME;

       case LOC_UNDEF:
	  error (_("internal:  Undefined class in m2lex()"));

       case LOC_LABEL:
       case LOC_UNRESOLVED:
	  error (_("internal:  Unforseen case in m2lex()"));

       default:
	  error (_("unhandled token in m2lex()"));
	  break;
       }
    }
    else
    {
       /* Built-in BOOLEAN type.  This is sort of a hack.  */
       if (startswith (tokstart, "TRUE"))
       {
	  yylval.ulval = 1;
	  return M2_TRUE;
       }
       else if (startswith (tokstart, "FALSE"))
       {
	  yylval.ulval = 0;
	  return M2_FALSE;
       }
    }

    /* Must be another type of name...  */
    return NAME;
 }
}

int
m2_language::parser (struct parser_state *par_state) const
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
