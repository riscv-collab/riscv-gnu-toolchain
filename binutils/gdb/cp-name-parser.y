/* YACC parser for C++ names, for GDB.

   Copyright (C) 2003-2024 Free Software Foundation, Inc.

   Parts of the lexer are based on c-exp.y from GDB.

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

/* Note that malloc's and realloc's in this file are transformed to
   xmalloc and xrealloc respectively by the same sed command in the
   makefile that remaps any other malloc/realloc inserted by the parser
   generator.  Doing this with #defines and trying to control the interaction
   with include files (<malloc.h> and <stdlib.h> for example) just became
   too messy, particularly when such includes can be inserted at random
   times by the parser generator.  */

/* The Bison manual says that %pure-parser is deprecated, but we use
   it anyway because it also works with Byacc.  That is also why
   this uses %lex-param and %parse-param rather than the simpler
   %param -- Byacc does not support the latter.  */
%pure-parser
%lex-param {struct cpname_state *state}
%parse-param {struct cpname_state *state}

%{

#include "defs.h"

#include <unistd.h>
#include "gdbsupport/gdb-safe-ctype.h"
#include "demangle.h"
#include "cp-support.h"
#include "c-support.h"
#include "parser-defs.h"

#define GDB_YY_REMAP_PREFIX cpname
#include "yy-remap.h"

/* The components built by the parser are allocated ahead of time,
   and cached in this structure.  */

#define ALLOC_CHUNK 100

struct demangle_info {
  int used;
  struct demangle_info *next;
  struct demangle_component comps[ALLOC_CHUNK];
};

%}

%union
  {
    struct demangle_component *comp;
    struct nested {
      struct demangle_component *comp;
      struct demangle_component **last;
    } nested;
    struct {
      struct demangle_component *comp, *last;
    } nested1;
    struct {
      struct demangle_component *comp, **last;
      struct nested fn;
      struct demangle_component *start;
      int fold_flag;
    } abstract;
    int lval;
    const char *opname;
  }

%{

struct cpname_state
{
  /* LEXPTR is the current pointer into our lex buffer.  PREV_LEXPTR
     is the start of the last token lexed, only used for diagnostics.
     ERROR_LEXPTR is the first place an error occurred.  GLOBAL_ERRMSG
     is the first error message encountered.  */

  const char *lexptr, *prev_lexptr, *error_lexptr, *global_errmsg;

  struct demangle_info *demangle_info;

  /* The parse tree created by the parser is stored here after a
     successful parse.  */

  struct demangle_component *global_result;

  struct demangle_component *d_grab ();

  /* Helper functions.  These wrap the demangler tree interface,
     handle allocation from our global store, and return the allocated
     component.  */

  struct demangle_component *fill_comp (enum demangle_component_type d_type,
					struct demangle_component *lhs,
					struct demangle_component *rhs);

  struct demangle_component *make_operator (const char *name, int args);

  struct demangle_component *make_dtor (enum gnu_v3_dtor_kinds kind,
					struct demangle_component *name);

  struct demangle_component *make_builtin_type (const char *name);

  struct demangle_component *make_name (const char *name, int len);

  struct demangle_component *d_qualify (struct demangle_component *lhs,
					int qualifiers, int is_method);

  struct demangle_component *d_int_type (int flags);

  struct demangle_component *d_unary (const char *name,
				      struct demangle_component *lhs);

  struct demangle_component *d_binary (const char *name,
				       struct demangle_component *lhs,
				       struct demangle_component *rhs);

  int parse_number (const char *p, int len, int parsed_float, YYSTYPE *lvalp);
};

struct demangle_component *
cpname_state::d_grab ()
{
  struct demangle_info *more;

  if (demangle_info->used >= ALLOC_CHUNK)
    {
      if (demangle_info->next == NULL)
	{
	  more = XNEW (struct demangle_info);
	  more->next = NULL;
	  demangle_info->next = more;
	}
      else
	more = demangle_info->next;

      more->used = 0;
      demangle_info = more;
    }
  return &demangle_info->comps[demangle_info->used++];
}

/* Flags passed to d_qualify.  */

#define QUAL_CONST 1
#define QUAL_RESTRICT 2
#define QUAL_VOLATILE 4

/* Flags passed to d_int_type.  */

#define INT_CHAR	(1 << 0)
#define INT_SHORT	(1 << 1)
#define INT_LONG	(1 << 2)
#define INT_LLONG	(1 << 3)

#define INT_SIGNED	(1 << 4)
#define INT_UNSIGNED	(1 << 5)

/* Enable yydebug for the stand-alone parser.  */
#ifdef TEST_CPNAMES
# define YYDEBUG	1
#endif

/* Helper functions.  These wrap the demangler tree interface, handle
   allocation from our global store, and return the allocated component.  */

struct demangle_component *
cpname_state::fill_comp (enum demangle_component_type d_type,
			 struct demangle_component *lhs,
			 struct demangle_component *rhs)
{
  struct demangle_component *ret = d_grab ();
  int i;

  i = cplus_demangle_fill_component (ret, d_type, lhs, rhs);
  gdb_assert (i);

  return ret;
}

struct demangle_component *
cpname_state::make_operator (const char *name, int args)
{
  struct demangle_component *ret = d_grab ();
  int i;

  i = cplus_demangle_fill_operator (ret, name, args);
  gdb_assert (i);

  return ret;
}

struct demangle_component *
cpname_state::make_dtor (enum gnu_v3_dtor_kinds kind,
			 struct demangle_component *name)
{
  struct demangle_component *ret = d_grab ();
  int i;

  i = cplus_demangle_fill_dtor (ret, kind, name);
  gdb_assert (i);

  return ret;
}

struct demangle_component *
cpname_state::make_builtin_type (const char *name)
{
  struct demangle_component *ret = d_grab ();
  int i;

  i = cplus_demangle_fill_builtin_type (ret, name);
  gdb_assert (i);

  return ret;
}

struct demangle_component *
cpname_state::make_name (const char *name, int len)
{
  struct demangle_component *ret = d_grab ();
  int i;

  i = cplus_demangle_fill_name (ret, name, len);
  gdb_assert (i);

  return ret;
}

#define d_left(dc) (dc)->u.s_binary.left
#define d_right(dc) (dc)->u.s_binary.right

static int yylex (YYSTYPE *, cpname_state *);
static void yyerror (cpname_state *, const char *);
%}

%type <comp> exp exp1 type start start_opt oper colon_name
%type <comp> unqualified_name colon_ext_name
%type <comp> templ template_arg
%type <comp> builtin_type
%type <comp> typespec_2 array_indicator
%type <comp> colon_ext_only ext_only_name

%type <comp> demangler_special function conversion_op
%type <nested> conversion_op_name

%type <abstract> abstract_declarator direct_abstract_declarator
%type <abstract> abstract_declarator_fn
%type <nested> declarator direct_declarator function_arglist

%type <nested> declarator_1 direct_declarator_1

%type <nested> template_params function_args
%type <nested> ptr_operator

%type <nested1> nested_name

%type <lval> qualifier qualifiers qualifiers_opt

%type <lval> int_part int_seq

%token <comp> INT
%token <comp> FLOAT

%token <comp> NAME
%type <comp> name

%token STRUCT CLASS UNION ENUM SIZEOF UNSIGNED COLONCOLON
%token TEMPLATE
%token ERROR
%token NEW DELETE OPERATOR
%token STATIC_CAST REINTERPRET_CAST DYNAMIC_CAST

/* Special type cases, put in to allow the parser to distinguish different
   legal basetypes.  */
%token SIGNED_KEYWORD LONG SHORT INT_KEYWORD CONST_KEYWORD VOLATILE_KEYWORD DOUBLE_KEYWORD BOOL
%token ELLIPSIS RESTRICT VOID FLOAT_KEYWORD CHAR WCHAR_T

%token <opname> ASSIGN_MODIFY

/* C++ */
%token TRUEKEYWORD
%token FALSEKEYWORD

/* Non-C++ things we get from the demangler.  */
%token <lval> DEMANGLER_SPECIAL
%token CONSTRUCTION_VTABLE CONSTRUCTION_IN

/* Precedence declarations.  */

/* Give NAME lower precedence than COLONCOLON, so that nested_name will
   associate greedily.  */
%nonassoc NAME

/* Give NEW and DELETE lower precedence than ']', because we can not
   have an array of type operator new.  This causes NEW '[' to be
   parsed as operator new[].  */
%nonassoc NEW DELETE

/* Give VOID higher precedence than NAME.  Then we can use %prec NAME
   to prefer (VOID) to (function_args).  */
%nonassoc VOID

/* Give VOID lower precedence than ')' for similar reasons.  */
%nonassoc ')'

%left ','
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

/* We don't need a precedence for '(' in this reduced grammar, and it
   can mask some unpleasant bugs, so disable it for now.  */

%right ARROW '.' '[' /* '(' */
%left COLONCOLON


%%

result		:	start
			{
			  state->global_result = $1;

			  /* Avoid warning about "yynerrs" being unused.  */
			  (void) yynerrs;
			}
		;

start		:	type

		|	demangler_special

		|	function

		;

start_opt	:	/* */
			{ $$ = NULL; }
		|	COLONCOLON start
			{ $$ = $2; }
		;

function
		/* Function with a return type.  declarator_1 is used to prevent
		   ambiguity with the next rule.  */
		:	typespec_2 declarator_1
			{ $$ = $2.comp;
			  *$2.last = $1;
			}

		/* Function without a return type.  We need to use typespec_2
		   to prevent conflicts from qualifiers_opt - harmless.  The
		   start_opt is used to handle "function-local" variables and
		   types.  */
		|	typespec_2 function_arglist start_opt
			{ $$ = state->fill_comp (DEMANGLE_COMPONENT_TYPED_NAME,
					  $1, $2.comp);
			  if ($3)
			    $$ = state->fill_comp (DEMANGLE_COMPONENT_LOCAL_NAME,
						   $$, $3);
			}
		|	colon_ext_only function_arglist start_opt
			{ $$ = state->fill_comp (DEMANGLE_COMPONENT_TYPED_NAME, $1, $2.comp);
			  if ($3) $$ = state->fill_comp (DEMANGLE_COMPONENT_LOCAL_NAME, $$, $3); }

		|	conversion_op_name start_opt
			{ $$ = $1.comp;
			  if ($2) $$ = state->fill_comp (DEMANGLE_COMPONENT_LOCAL_NAME, $$, $2); }
		|	conversion_op_name abstract_declarator_fn
			{ if ($2.last)
			    {
			       /* First complete the abstract_declarator's type using
				  the typespec from the conversion_op_name.  */
			      *$2.last = *$1.last;
			      /* Then complete the conversion_op_name with the type.  */
			      *$1.last = $2.comp;
			    }
			  /* If we have an arglist, build a function type.  */
			  if ($2.fn.comp)
			    $$ = state->fill_comp (DEMANGLE_COMPONENT_TYPED_NAME, $1.comp, $2.fn.comp);
			  else
			    $$ = $1.comp;
			  if ($2.start) $$ = state->fill_comp (DEMANGLE_COMPONENT_LOCAL_NAME, $$, $2.start);
			}
		;

demangler_special
		:	DEMANGLER_SPECIAL start
			{ $$ = state->fill_comp ((enum demangle_component_type) $1, $2, NULL); }
		|	CONSTRUCTION_VTABLE start CONSTRUCTION_IN start
			{ $$ = state->fill_comp (DEMANGLE_COMPONENT_CONSTRUCTION_VTABLE, $2, $4); }
		;

oper	:	OPERATOR NEW
			{
			  /* Match the whitespacing of cplus_demangle_operators.
			     It would abort on unrecognized string otherwise.  */
			  $$ = state->make_operator ("new", 3);
			}
		|	OPERATOR DELETE
			{
			  /* Match the whitespacing of cplus_demangle_operators.
			     It would abort on unrecognized string otherwise.  */
			  $$ = state->make_operator ("delete ", 1);
			}
		|	OPERATOR NEW '[' ']'
			{
			  /* Match the whitespacing of cplus_demangle_operators.
			     It would abort on unrecognized string otherwise.  */
			  $$ = state->make_operator ("new[]", 3);
			}
		|	OPERATOR DELETE '[' ']'
			{
			  /* Match the whitespacing of cplus_demangle_operators.
			     It would abort on unrecognized string otherwise.  */
			  $$ = state->make_operator ("delete[] ", 1);
			}
		|	OPERATOR '+'
			{ $$ = state->make_operator ("+", 2); }
		|	OPERATOR '-'
			{ $$ = state->make_operator ("-", 2); }
		|	OPERATOR '*'
			{ $$ = state->make_operator ("*", 2); }
		|	OPERATOR '/'
			{ $$ = state->make_operator ("/", 2); }
		|	OPERATOR '%'
			{ $$ = state->make_operator ("%", 2); }
		|	OPERATOR '^'
			{ $$ = state->make_operator ("^", 2); }
		|	OPERATOR '&'
			{ $$ = state->make_operator ("&", 2); }
		|	OPERATOR '|'
			{ $$ = state->make_operator ("|", 2); }
		|	OPERATOR '~'
			{ $$ = state->make_operator ("~", 1); }
		|	OPERATOR '!'
			{ $$ = state->make_operator ("!", 1); }
		|	OPERATOR '='
			{ $$ = state->make_operator ("=", 2); }
		|	OPERATOR '<'
			{ $$ = state->make_operator ("<", 2); }
		|	OPERATOR '>'
			{ $$ = state->make_operator (">", 2); }
		|	OPERATOR ASSIGN_MODIFY
			{ $$ = state->make_operator ($2, 2); }
		|	OPERATOR LSH
			{ $$ = state->make_operator ("<<", 2); }
		|	OPERATOR RSH
			{ $$ = state->make_operator (">>", 2); }
		|	OPERATOR EQUAL
			{ $$ = state->make_operator ("==", 2); }
		|	OPERATOR NOTEQUAL
			{ $$ = state->make_operator ("!=", 2); }
		|	OPERATOR LEQ
			{ $$ = state->make_operator ("<=", 2); }
		|	OPERATOR GEQ
			{ $$ = state->make_operator (">=", 2); }
		|	OPERATOR ANDAND
			{ $$ = state->make_operator ("&&", 2); }
		|	OPERATOR OROR
			{ $$ = state->make_operator ("||", 2); }
		|	OPERATOR INCREMENT
			{ $$ = state->make_operator ("++", 1); }
		|	OPERATOR DECREMENT
			{ $$ = state->make_operator ("--", 1); }
		|	OPERATOR ','
			{ $$ = state->make_operator (",", 2); }
		|	OPERATOR ARROW '*'
			{ $$ = state->make_operator ("->*", 2); }
		|	OPERATOR ARROW
			{ $$ = state->make_operator ("->", 2); }
		|	OPERATOR '(' ')'
			{ $$ = state->make_operator ("()", 2); }
		|	OPERATOR '[' ']'
			{ $$ = state->make_operator ("[]", 2); }
		;

		/* Conversion operators.  We don't try to handle some of
		   the wackier demangler output for function pointers,
		   since it's not clear that it's parseable.  */
conversion_op
		:	OPERATOR typespec_2
			{ $$ = state->fill_comp (DEMANGLE_COMPONENT_CONVERSION, $2, NULL); }
		;

conversion_op_name
		:	nested_name conversion_op
			{ $$.comp = $1.comp;
			  d_right ($1.last) = $2;
			  $$.last = &d_left ($2);
			}
		|	conversion_op
			{ $$.comp = $1;
			  $$.last = &d_left ($1);
			}
		|	COLONCOLON nested_name conversion_op
			{ $$.comp = $2.comp;
			  d_right ($2.last) = $3;
			  $$.last = &d_left ($3);
			}
		|	COLONCOLON conversion_op
			{ $$.comp = $2;
			  $$.last = &d_left ($2);
			}
		;

/* DEMANGLE_COMPONENT_NAME */
/* This accepts certain invalid placements of '~'.  */
unqualified_name:	oper
		|	oper '<' template_params '>'
			{ $$ = state->fill_comp (DEMANGLE_COMPONENT_TEMPLATE, $1, $3.comp); }
		|	'~' NAME
			{ $$ = state->make_dtor (gnu_v3_complete_object_dtor, $2); }
		;

/* This rule is used in name and nested_name, and expanded inline there
   for efficiency.  */
/*
scope_id	:	NAME
		|	template
		;
*/

colon_name	:	name
		|	COLONCOLON name
			{ $$ = $2; }
		;

/* DEMANGLE_COMPONENT_QUAL_NAME */
/* DEMANGLE_COMPONENT_CTOR / DEMANGLE_COMPONENT_DTOR ? */
name		:	nested_name NAME %prec NAME
			{ $$ = $1.comp; d_right ($1.last) = $2; }
		|	NAME %prec NAME
		|	nested_name templ %prec NAME
			{ $$ = $1.comp; d_right ($1.last) = $2; }
		|	templ %prec NAME
		;

colon_ext_name	:	colon_name
		|	colon_ext_only
		;

colon_ext_only	:	ext_only_name
		|	COLONCOLON ext_only_name
			{ $$ = $2; }
		;

ext_only_name	:	nested_name unqualified_name
			{ $$ = $1.comp; d_right ($1.last) = $2; }
		|	unqualified_name
		;

nested_name	:	NAME COLONCOLON
			{ $$.comp = state->fill_comp (DEMANGLE_COMPONENT_QUAL_NAME, $1, NULL);
			  $$.last = $$.comp;
			}
		|	nested_name NAME COLONCOLON
			{ $$.comp = $1.comp;
			  d_right ($1.last) = state->fill_comp (DEMANGLE_COMPONENT_QUAL_NAME, $2, NULL);
			  $$.last = d_right ($1.last);
			}
		|	templ COLONCOLON
			{ $$.comp = state->fill_comp (DEMANGLE_COMPONENT_QUAL_NAME, $1, NULL);
			  $$.last = $$.comp;
			}
		|	nested_name templ COLONCOLON
			{ $$.comp = $1.comp;
			  d_right ($1.last) = state->fill_comp (DEMANGLE_COMPONENT_QUAL_NAME, $2, NULL);
			  $$.last = d_right ($1.last);
			}
		;

/* DEMANGLE_COMPONENT_TEMPLATE */
/* DEMANGLE_COMPONENT_TEMPLATE_ARGLIST */
templ	:	NAME '<' template_params '>'
			{ $$ = state->fill_comp (DEMANGLE_COMPONENT_TEMPLATE, $1, $3.comp); }
		;

template_params	:	template_arg
			{ $$.comp = state->fill_comp (DEMANGLE_COMPONENT_TEMPLATE_ARGLIST, $1, NULL);
			$$.last = &d_right ($$.comp); }
		|	template_params ',' template_arg
			{ $$.comp = $1.comp;
			  *$1.last = state->fill_comp (DEMANGLE_COMPONENT_TEMPLATE_ARGLIST, $3, NULL);
			  $$.last = &d_right (*$1.last);
			}
		;

/* "type" is inlined into template_arg and function_args.  */

/* Also an integral constant-expression of integral type, and a
   pointer to member (?) */
template_arg	:	typespec_2
		|	typespec_2 abstract_declarator
			{ $$ = $2.comp;
			  *$2.last = $1;
			}
		|	'&' start
			{ $$ = state->fill_comp (DEMANGLE_COMPONENT_UNARY, state->make_operator ("&", 1), $2); }
		|	'&' '(' start ')'
			{ $$ = state->fill_comp (DEMANGLE_COMPONENT_UNARY, state->make_operator ("&", 1), $3); }
		|	exp
		;

function_args	:	typespec_2
			{ $$.comp = state->fill_comp (DEMANGLE_COMPONENT_ARGLIST, $1, NULL);
			  $$.last = &d_right ($$.comp);
			}
		|	typespec_2 abstract_declarator
			{ *$2.last = $1;
			  $$.comp = state->fill_comp (DEMANGLE_COMPONENT_ARGLIST, $2.comp, NULL);
			  $$.last = &d_right ($$.comp);
			}
		|	function_args ',' typespec_2
			{ *$1.last = state->fill_comp (DEMANGLE_COMPONENT_ARGLIST, $3, NULL);
			  $$.comp = $1.comp;
			  $$.last = &d_right (*$1.last);
			}
		|	function_args ',' typespec_2 abstract_declarator
			{ *$4.last = $3;
			  *$1.last = state->fill_comp (DEMANGLE_COMPONENT_ARGLIST, $4.comp, NULL);
			  $$.comp = $1.comp;
			  $$.last = &d_right (*$1.last);
			}
		|	function_args ',' ELLIPSIS
			{ *$1.last
			    = state->fill_comp (DEMANGLE_COMPONENT_ARGLIST,
					   state->make_builtin_type ("..."),
					   NULL);
			  $$.comp = $1.comp;
			  $$.last = &d_right (*$1.last);
			}
		;

function_arglist:	'(' function_args ')' qualifiers_opt %prec NAME
			{ $$.comp = state->fill_comp (DEMANGLE_COMPONENT_FUNCTION_TYPE, NULL, $2.comp);
			  $$.last = &d_left ($$.comp);
			  $$.comp = state->d_qualify ($$.comp, $4, 1); }
		|	'(' VOID ')' qualifiers_opt
			{ $$.comp = state->fill_comp (DEMANGLE_COMPONENT_FUNCTION_TYPE, NULL, NULL);
			  $$.last = &d_left ($$.comp);
			  $$.comp = state->d_qualify ($$.comp, $4, 1); }
		|	'(' ')' qualifiers_opt
			{ $$.comp = state->fill_comp (DEMANGLE_COMPONENT_FUNCTION_TYPE, NULL, NULL);
			  $$.last = &d_left ($$.comp);
			  $$.comp = state->d_qualify ($$.comp, $3, 1); }
		;

/* Should do something about DEMANGLE_COMPONENT_VENDOR_TYPE_QUAL */
qualifiers_opt	:	/* epsilon */
			{ $$ = 0; }
		|	qualifiers
		;

qualifier	:	RESTRICT
			{ $$ = QUAL_RESTRICT; }
		|	VOLATILE_KEYWORD
			{ $$ = QUAL_VOLATILE; }
		|	CONST_KEYWORD
			{ $$ = QUAL_CONST; }
		;

qualifiers	:	qualifier
		|	qualifier qualifiers
			{ $$ = $1 | $2; }
		;

/* This accepts all sorts of invalid constructions and produces
   invalid output for them - an error would be better.  */

int_part	:	INT_KEYWORD
			{ $$ = 0; }
		|	SIGNED_KEYWORD
			{ $$ = INT_SIGNED; }
		|	UNSIGNED
			{ $$ = INT_UNSIGNED; }
		|	CHAR
			{ $$ = INT_CHAR; }
		|	LONG
			{ $$ = INT_LONG; }
		|	SHORT
			{ $$ = INT_SHORT; }
		;

int_seq		:	int_part
		|	int_seq int_part
			{ $$ = $1 | $2; if ($1 & $2 & INT_LONG) $$ = $1 | INT_LLONG; }
		;

builtin_type	:	int_seq
			{ $$ = state->d_int_type ($1); }
		|	FLOAT_KEYWORD
			{ $$ = state->make_builtin_type ("float"); }
		|	DOUBLE_KEYWORD
			{ $$ = state->make_builtin_type ("double"); }
		|	LONG DOUBLE_KEYWORD
			{ $$ = state->make_builtin_type ("long double"); }
		|	BOOL
			{ $$ = state->make_builtin_type ("bool"); }
		|	WCHAR_T
			{ $$ = state->make_builtin_type ("wchar_t"); }
		|	VOID
			{ $$ = state->make_builtin_type ("void"); }
		;

ptr_operator	:	'*' qualifiers_opt
			{ $$.comp = state->fill_comp (DEMANGLE_COMPONENT_POINTER, NULL, NULL);
			  $$.last = &d_left ($$.comp);
			  $$.comp = state->d_qualify ($$.comp, $2, 0); }
		/* g++ seems to allow qualifiers after the reference?  */
		|	'&'
			{ $$.comp = state->fill_comp (DEMANGLE_COMPONENT_REFERENCE, NULL, NULL);
			  $$.last = &d_left ($$.comp); }
		|	ANDAND
			{ $$.comp = state->fill_comp (DEMANGLE_COMPONENT_RVALUE_REFERENCE, NULL, NULL);
			  $$.last = &d_left ($$.comp); }
		|	nested_name '*' qualifiers_opt
			{ $$.comp = state->fill_comp (DEMANGLE_COMPONENT_PTRMEM_TYPE, $1.comp, NULL);
			  /* Convert the innermost DEMANGLE_COMPONENT_QUAL_NAME to a DEMANGLE_COMPONENT_NAME.  */
			  *$1.last = *d_left ($1.last);
			  $$.last = &d_right ($$.comp);
			  $$.comp = state->d_qualify ($$.comp, $3, 0); }
		|	COLONCOLON nested_name '*' qualifiers_opt
			{ $$.comp = state->fill_comp (DEMANGLE_COMPONENT_PTRMEM_TYPE, $2.comp, NULL);
			  /* Convert the innermost DEMANGLE_COMPONENT_QUAL_NAME to a DEMANGLE_COMPONENT_NAME.  */
			  *$2.last = *d_left ($2.last);
			  $$.last = &d_right ($$.comp);
			  $$.comp = state->d_qualify ($$.comp, $4, 0); }
		;

array_indicator	:	'[' ']'
			{ $$ = state->fill_comp (DEMANGLE_COMPONENT_ARRAY_TYPE, NULL, NULL); }
		|	'[' INT ']'
			{ $$ = state->fill_comp (DEMANGLE_COMPONENT_ARRAY_TYPE, $2, NULL); }
		;

/* Details of this approach inspired by the G++ < 3.4 parser.  */

/* This rule is only used in typespec_2, and expanded inline there for
   efficiency.  */
/*
typespec	:	builtin_type
		|	colon_name
		;
*/

typespec_2	:	builtin_type qualifiers
			{ $$ = state->d_qualify ($1, $2, 0); }
		|	builtin_type
		|	qualifiers builtin_type qualifiers
			{ $$ = state->d_qualify ($2, $1 | $3, 0); }
		|	qualifiers builtin_type
			{ $$ = state->d_qualify ($2, $1, 0); }

		|	name qualifiers
			{ $$ = state->d_qualify ($1, $2, 0); }
		|	name
		|	qualifiers name qualifiers
			{ $$ = state->d_qualify ($2, $1 | $3, 0); }
		|	qualifiers name
			{ $$ = state->d_qualify ($2, $1, 0); }

		|	COLONCOLON name qualifiers
			{ $$ = state->d_qualify ($2, $3, 0); }
		|	COLONCOLON name
			{ $$ = $2; }
		|	qualifiers COLONCOLON name qualifiers
			{ $$ = state->d_qualify ($3, $1 | $4, 0); }
		|	qualifiers COLONCOLON name
			{ $$ = state->d_qualify ($3, $1, 0); }
		;

abstract_declarator
		:	ptr_operator
			{ $$.comp = $1.comp; $$.last = $1.last;
			  $$.fn.comp = NULL; $$.fn.last = NULL; }
		|	ptr_operator abstract_declarator
			{ $$ = $2; $$.fn.comp = NULL; $$.fn.last = NULL;
			  if ($2.fn.comp) { $$.last = $2.fn.last; *$2.last = $2.fn.comp; }
			  *$$.last = $1.comp;
			  $$.last = $1.last; }
		|	direct_abstract_declarator
			{ $$.fn.comp = NULL; $$.fn.last = NULL;
			  if ($1.fn.comp) { $$.last = $1.fn.last; *$1.last = $1.fn.comp; }
			}
		;

direct_abstract_declarator
		:	'(' abstract_declarator ')'
			{ $$ = $2; $$.fn.comp = NULL; $$.fn.last = NULL; $$.fold_flag = 1;
			  if ($2.fn.comp) { $$.last = $2.fn.last; *$2.last = $2.fn.comp; }
			}
		|	direct_abstract_declarator function_arglist
			{ $$.fold_flag = 0;
			  if ($1.fn.comp) { $$.last = $1.fn.last; *$1.last = $1.fn.comp; }
			  if ($1.fold_flag)
			    {
			      *$$.last = $2.comp;
			      $$.last = $2.last;
			    }
			  else
			    $$.fn = $2;
			}
		|	direct_abstract_declarator array_indicator
			{ $$.fn.comp = NULL; $$.fn.last = NULL; $$.fold_flag = 0;
			  if ($1.fn.comp) { $$.last = $1.fn.last; *$1.last = $1.fn.comp; }
			  *$1.last = $2;
			  $$.last = &d_right ($2);
			}
		|	array_indicator
			{ $$.fn.comp = NULL; $$.fn.last = NULL; $$.fold_flag = 0;
			  $$.comp = $1;
			  $$.last = &d_right ($1);
			}
		/* G++ has the following except for () and (type).  Then
		   (type) is handled in regcast_or_absdcl and () is handled
		   in fcast_or_absdcl.

		   However, this is only useful for function types, and
		   generates reduce/reduce conflicts with direct_declarator.
		   We're interested in pointer-to-function types, and in
		   functions, but not in function types - so leave this
		   out.  */
		/* |	function_arglist */
		;

abstract_declarator_fn
		:	ptr_operator
			{ $$.comp = $1.comp; $$.last = $1.last;
			  $$.fn.comp = NULL; $$.fn.last = NULL; $$.start = NULL; }
		|	ptr_operator abstract_declarator_fn
			{ $$ = $2;
			  if ($2.last)
			    *$$.last = $1.comp;
			  else
			    $$.comp = $1.comp;
			  $$.last = $1.last;
			}
		|	direct_abstract_declarator
			{ $$.comp = $1.comp; $$.last = $1.last; $$.fn = $1.fn; $$.start = NULL; }
		|	direct_abstract_declarator function_arglist COLONCOLON start
			{ $$.start = $4;
			  if ($1.fn.comp) { $$.last = $1.fn.last; *$1.last = $1.fn.comp; }
			  if ($1.fold_flag)
			    {
			      *$$.last = $2.comp;
			      $$.last = $2.last;
			    }
			  else
			    $$.fn = $2;
			}
		|	function_arglist start_opt
			{ $$.fn = $1;
			  $$.start = $2;
			  $$.comp = NULL; $$.last = NULL;
			}
		;

type		:	typespec_2
		|	typespec_2 abstract_declarator
			{ $$ = $2.comp;
			  *$2.last = $1;
			}
		;

declarator	:	ptr_operator declarator
			{ $$.comp = $2.comp;
			  $$.last = $1.last;
			  *$2.last = $1.comp; }
		|	direct_declarator
		;

direct_declarator
		:	'(' declarator ')'
			{ $$ = $2; }
		|	direct_declarator function_arglist
			{ $$.comp = $1.comp;
			  *$1.last = $2.comp;
			  $$.last = $2.last;
			}
		|	direct_declarator array_indicator
			{ $$.comp = $1.comp;
			  *$1.last = $2;
			  $$.last = &d_right ($2);
			}
		|	colon_ext_name
			{ $$.comp = state->fill_comp (DEMANGLE_COMPONENT_TYPED_NAME, $1, NULL);
			  $$.last = &d_right ($$.comp);
			}
		;

/* These are similar to declarator and direct_declarator except that they
   do not permit ( colon_ext_name ), which is ambiguous with a function
   argument list.  They also don't permit a few other forms with redundant
   parentheses around the colon_ext_name; any colon_ext_name in parentheses
   must be followed by an argument list or an array indicator, or preceded
   by a pointer.  */
declarator_1	:	ptr_operator declarator_1
			{ $$.comp = $2.comp;
			  $$.last = $1.last;
			  *$2.last = $1.comp; }
		|	colon_ext_name
			{ $$.comp = state->fill_comp (DEMANGLE_COMPONENT_TYPED_NAME, $1, NULL);
			  $$.last = &d_right ($$.comp);
			}
		|	direct_declarator_1

			/* Function local variable or type.  The typespec to
			   our left is the type of the containing function. 
			   This should be OK, because function local types
			   can not be templates, so the return types of their
			   members will not be mangled.  If they are hopefully
			   they'll end up to the right of the ::.  */
		|	colon_ext_name function_arglist COLONCOLON start
			{ $$.comp = state->fill_comp (DEMANGLE_COMPONENT_TYPED_NAME, $1, $2.comp);
			  $$.last = $2.last;
			  $$.comp = state->fill_comp (DEMANGLE_COMPONENT_LOCAL_NAME, $$.comp, $4);
			}
		|	direct_declarator_1 function_arglist COLONCOLON start
			{ $$.comp = $1.comp;
			  *$1.last = $2.comp;
			  $$.last = $2.last;
			  $$.comp = state->fill_comp (DEMANGLE_COMPONENT_LOCAL_NAME, $$.comp, $4);
			}
		;

direct_declarator_1
		:	'(' ptr_operator declarator ')'
			{ $$.comp = $3.comp;
			  $$.last = $2.last;
			  *$3.last = $2.comp; }
		|	direct_declarator_1 function_arglist
			{ $$.comp = $1.comp;
			  *$1.last = $2.comp;
			  $$.last = $2.last;
			}
		|	direct_declarator_1 array_indicator
			{ $$.comp = $1.comp;
			  *$1.last = $2;
			  $$.last = &d_right ($2);
			}
		|	colon_ext_name function_arglist
			{ $$.comp = state->fill_comp (DEMANGLE_COMPONENT_TYPED_NAME, $1, $2.comp);
			  $$.last = $2.last;
			}
		|	colon_ext_name array_indicator
			{ $$.comp = state->fill_comp (DEMANGLE_COMPONENT_TYPED_NAME, $1, $2);
			  $$.last = &d_right ($2);
			}
		;

exp	:	'(' exp1 ')'
		{ $$ = $2; }
	;

/* Silly trick.  Only allow '>' when parenthesized, in order to
   handle conflict with templates.  */
exp1	:	exp
	;

exp1	:	exp '>' exp
		{ $$ = state->d_binary (">", $1, $3); }
	;

/* References.  Not allowed everywhere in template parameters, only
   at the top level, but treat them as expressions in case they are wrapped
   in parentheses.  */
exp1	:	'&' start
		{ $$ = state->fill_comp (DEMANGLE_COMPONENT_UNARY, state->make_operator ("&", 1), $2); }
	|	'&' '(' start ')'
		{ $$ = state->fill_comp (DEMANGLE_COMPONENT_UNARY, state->make_operator ("&", 1), $3); }
	;

/* Expressions, not including the comma operator.  */
exp	:	'-' exp    %prec UNARY
		{ $$ = state->d_unary ("-", $2); }
	;

exp	:	'!' exp    %prec UNARY
		{ $$ = state->d_unary ("!", $2); }
	;

exp	:	'~' exp    %prec UNARY
		{ $$ = state->d_unary ("~", $2); }
	;

/* Casts.  First your normal C-style cast.  If exp is a LITERAL, just change
   its type.  */

exp	:	'(' type ')' exp  %prec UNARY
		{ if ($4->type == DEMANGLE_COMPONENT_LITERAL
		      || $4->type == DEMANGLE_COMPONENT_LITERAL_NEG)
		    {
		      $$ = $4;
		      d_left ($4) = $2;
		    }
		  else
		    $$ = state->fill_comp (DEMANGLE_COMPONENT_UNARY,
				      state->fill_comp (DEMANGLE_COMPONENT_CAST, $2, NULL),
				      $4);
		}
	;

/* Mangling does not differentiate between these, so we don't need to
   either.  */
exp	:	STATIC_CAST '<' type '>' '(' exp1 ')' %prec UNARY
		{ $$ = state->fill_comp (DEMANGLE_COMPONENT_UNARY,
				    state->fill_comp (DEMANGLE_COMPONENT_CAST, $3, NULL),
				    $6);
		}
	;

exp	:	DYNAMIC_CAST '<' type '>' '(' exp1 ')' %prec UNARY
		{ $$ = state->fill_comp (DEMANGLE_COMPONENT_UNARY,
				    state->fill_comp (DEMANGLE_COMPONENT_CAST, $3, NULL),
				    $6);
		}
	;

exp	:	REINTERPRET_CAST '<' type '>' '(' exp1 ')' %prec UNARY
		{ $$ = state->fill_comp (DEMANGLE_COMPONENT_UNARY,
				    state->fill_comp (DEMANGLE_COMPONENT_CAST, $3, NULL),
				    $6);
		}
	;

/* Another form of C++-style cast is "type ( exp1 )".  This creates too many
   conflicts to support.  For a while we supported the simpler
   "typespec_2 ( exp1 )", but that conflicts with "& ( start )" as a
   reference, deep within the wilderness of abstract declarators:
   Qux<int(&(*))> vs Qux<int(&(var))>, a shift-reduce conflict at the
   innermost left parenthesis.  So we do not support function-like casts.
   Fortunately they never appear in demangler output.  */

/* TO INVESTIGATE: ._0 style anonymous names; anonymous namespaces */

/* Binary operators in order of decreasing precedence.  */

exp	:	exp '*' exp
		{ $$ = state->d_binary ("*", $1, $3); }
	;

exp	:	exp '/' exp
		{ $$ = state->d_binary ("/", $1, $3); }
	;

exp	:	exp '%' exp
		{ $$ = state->d_binary ("%", $1, $3); }
	;

exp	:	exp '+' exp
		{ $$ = state->d_binary ("+", $1, $3); }
	;

exp	:	exp '-' exp
		{ $$ = state->d_binary ("-", $1, $3); }
	;

exp	:	exp LSH exp
		{ $$ = state->d_binary ("<<", $1, $3); }
	;

exp	:	exp RSH exp
		{ $$ = state->d_binary (">>", $1, $3); }
	;

exp	:	exp EQUAL exp
		{ $$ = state->d_binary ("==", $1, $3); }
	;

exp	:	exp NOTEQUAL exp
		{ $$ = state->d_binary ("!=", $1, $3); }
	;

exp	:	exp LEQ exp
		{ $$ = state->d_binary ("<=", $1, $3); }
	;

exp	:	exp GEQ exp
		{ $$ = state->d_binary (">=", $1, $3); }
	;

exp	:	exp '<' exp
		{ $$ = state->d_binary ("<", $1, $3); }
	;

exp	:	exp '&' exp
		{ $$ = state->d_binary ("&", $1, $3); }
	;

exp	:	exp '^' exp
		{ $$ = state->d_binary ("^", $1, $3); }
	;

exp	:	exp '|' exp
		{ $$ = state->d_binary ("|", $1, $3); }
	;

exp	:	exp ANDAND exp
		{ $$ = state->d_binary ("&&", $1, $3); }
	;

exp	:	exp OROR exp
		{ $$ = state->d_binary ("||", $1, $3); }
	;

/* Not 100% sure these are necessary, but they're harmless.  */
exp	:	exp ARROW NAME
		{ $$ = state->d_binary ("->", $1, $3); }
	;

exp	:	exp '.' NAME
		{ $$ = state->d_binary (".", $1, $3); }
	;

exp	:	exp '?' exp ':' exp	%prec '?'
		{ $$ = state->fill_comp (DEMANGLE_COMPONENT_TRINARY, state->make_operator ("?", 3),
				    state->fill_comp (DEMANGLE_COMPONENT_TRINARY_ARG1, $1,
						 state->fill_comp (DEMANGLE_COMPONENT_TRINARY_ARG2, $3, $5)));
		}
	;
			  
exp	:	INT
	;

/* Not generally allowed.  */
exp	:	FLOAT
	;

exp	:	SIZEOF '(' type ')'	%prec UNARY
		{
		  /* Match the whitespacing of cplus_demangle_operators.
		     It would abort on unrecognized string otherwise.  */
		  $$ = state->d_unary ("sizeof ", $3);
		}
	;

/* C++.  */
exp     :       TRUEKEYWORD    
		{ struct demangle_component *i;
		  i = state->make_name ("1", 1);
		  $$ = state->fill_comp (DEMANGLE_COMPONENT_LITERAL,
				    state->make_builtin_type ( "bool"),
				    i);
		}
	;

exp     :       FALSEKEYWORD   
		{ struct demangle_component *i;
		  i = state->make_name ("0", 1);
		  $$ = state->fill_comp (DEMANGLE_COMPONENT_LITERAL,
				    state->make_builtin_type ("bool"),
				    i);
		}
	;

/* end of C++.  */

%%

/* Apply QUALIFIERS to LHS and return a qualified component.  IS_METHOD
   is set if LHS is a method, in which case the qualifiers are logically
   applied to "this".  We apply qualifiers in a consistent order; LHS
   may already be qualified; duplicate qualifiers are not created.  */

struct demangle_component *
cpname_state::d_qualify (struct demangle_component *lhs, int qualifiers,
			 int is_method)
{
  struct demangle_component **inner_p;
  enum demangle_component_type type;

  /* For now the order is CONST (innermost), VOLATILE, RESTRICT.  */

#define HANDLE_QUAL(TYPE, MTYPE, QUAL)				\
  if ((qualifiers & QUAL) && (type != TYPE) && (type != MTYPE))	\
    {								\
      *inner_p = fill_comp (is_method ? MTYPE : TYPE,		\
			    *inner_p, NULL);			\
      inner_p = &d_left (*inner_p);				\
      type = (*inner_p)->type;					\
    }								\
  else if (type == TYPE || type == MTYPE)			\
    {								\
      inner_p = &d_left (*inner_p);				\
      type = (*inner_p)->type;					\
    }

  inner_p = &lhs;

  type = (*inner_p)->type;

  HANDLE_QUAL (DEMANGLE_COMPONENT_RESTRICT, DEMANGLE_COMPONENT_RESTRICT_THIS, QUAL_RESTRICT);
  HANDLE_QUAL (DEMANGLE_COMPONENT_VOLATILE, DEMANGLE_COMPONENT_VOLATILE_THIS, QUAL_VOLATILE);
  HANDLE_QUAL (DEMANGLE_COMPONENT_CONST, DEMANGLE_COMPONENT_CONST_THIS, QUAL_CONST);

  return lhs;
}

/* Return a builtin type corresponding to FLAGS.  */

struct demangle_component *
cpname_state::d_int_type (int flags)
{
  const char *name;

  switch (flags)
    {
    case INT_SIGNED | INT_CHAR:
      name = "signed char";
      break;
    case INT_CHAR:
      name = "char";
      break;
    case INT_UNSIGNED | INT_CHAR:
      name = "unsigned char";
      break;
    case 0:
    case INT_SIGNED:
      name = "int";
      break;
    case INT_UNSIGNED:
      name = "unsigned int";
      break;
    case INT_LONG:
    case INT_SIGNED | INT_LONG:
      name = "long";
      break;
    case INT_UNSIGNED | INT_LONG:
      name = "unsigned long";
      break;
    case INT_SHORT:
    case INT_SIGNED | INT_SHORT:
      name = "short";
      break;
    case INT_UNSIGNED | INT_SHORT:
      name = "unsigned short";
      break;
    case INT_LLONG | INT_LONG:
    case INT_SIGNED | INT_LLONG | INT_LONG:
      name = "long long";
      break;
    case INT_UNSIGNED | INT_LLONG | INT_LONG:
      name = "unsigned long long";
      break;
    default:
      return NULL;
    }

  return make_builtin_type (name);
}

/* Wrapper to create a unary operation.  */

struct demangle_component *
cpname_state::d_unary (const char *name, struct demangle_component *lhs)
{
  return fill_comp (DEMANGLE_COMPONENT_UNARY, make_operator (name, 1), lhs);
}

/* Wrapper to create a binary operation.  */

struct demangle_component *
cpname_state::d_binary (const char *name, struct demangle_component *lhs,
			struct demangle_component *rhs)
{
  return fill_comp (DEMANGLE_COMPONENT_BINARY, make_operator (name, 2),
		    fill_comp (DEMANGLE_COMPONENT_BINARY_ARGS, lhs, rhs));
}

/* Find the end of a symbol name starting at LEXPTR.  */

static const char *
symbol_end (const char *lexptr)
{
  const char *p = lexptr;

  while (*p && (c_ident_is_alnum (*p) || *p == '_' || *p == '$' || *p == '.'))
    p++;

  return p;
}

/* Take care of parsing a number (anything that starts with a digit).
   The number starts at P and contains LEN characters.  Store the result in
   YYLVAL.  */

int
cpname_state::parse_number (const char *p, int len, int parsed_float,
			    YYSTYPE *lvalp)
{
  int unsigned_p = 0;

  /* Number of "L" suffixes encountered.  */
  int long_p = 0;

  struct demangle_component *signed_type;
  struct demangle_component *unsigned_type;
  struct demangle_component *type, *name;
  enum demangle_component_type literal_type;

  if (p[0] == '-')
    {
      literal_type = DEMANGLE_COMPONENT_LITERAL_NEG;
      p++;
      len--;
    }
  else
    literal_type = DEMANGLE_COMPONENT_LITERAL;

  if (parsed_float)
    {
      /* It's a float since it contains a point or an exponent.  */
      char c;

      /* The GDB lexer checks the result of scanf at this point.  Not doing
	 this leaves our error checking slightly weaker but only for invalid
	 data.  */

      /* See if it has `f' or `l' suffix (float or long double).  */

      c = TOLOWER (p[len - 1]);

      if (c == 'f')
      	{
      	  len--;
      	  type = make_builtin_type ("float");
      	}
      else if (c == 'l')
	{
	  len--;
	  type = make_builtin_type ("long double");
	}
      else if (ISDIGIT (c) || c == '.')
	type = make_builtin_type ("double");
      else
	return ERROR;

      name = make_name (p, len);
      lvalp->comp = fill_comp (literal_type, type, name);

      return FLOAT;
    }

  /* This treats 0x1 and 1 as different literals.  We also do not
     automatically generate unsigned types.  */

  long_p = 0;
  unsigned_p = 0;
  while (len > 0)
    {
      if (p[len - 1] == 'l' || p[len - 1] == 'L')
	{
	  len--;
	  long_p++;
	  continue;
	}
      if (p[len - 1] == 'u' || p[len - 1] == 'U')
	{
	  len--;
	  unsigned_p++;
	  continue;
	}
      break;
    }

  if (long_p == 0)
    {
      unsigned_type = make_builtin_type ("unsigned int");
      signed_type = make_builtin_type ("int");
    }
  else if (long_p == 1)
    {
      unsigned_type = make_builtin_type ("unsigned long");
      signed_type = make_builtin_type ("long");
    }
  else
    {
      unsigned_type = make_builtin_type ("unsigned long long");
      signed_type = make_builtin_type ("long long");
    }

   if (unsigned_p)
     type = unsigned_type;
   else
     type = signed_type;

   name = make_name (p, len);
   lvalp->comp = fill_comp (literal_type, type, name);

   return INT;
}

static const char backslashable[] = "abefnrtv";
static const char represented[] = "\a\b\e\f\n\r\t\v";

/* Translate the backslash the way we would in the host character set.  */
static int
c_parse_backslash (int host_char, int *target_char)
{
  const char *ix;
  ix = strchr (backslashable, host_char);
  if (! ix)
    return 0;
  else
    *target_char = represented[ix - backslashable];
  return 1;
}

/* Parse a C escape sequence.  STRING_PTR points to a variable
   containing a pointer to the string to parse.  That pointer
   should point to the character after the \.  That pointer
   is updated past the characters we use.  The value of the
   escape sequence is returned.

   A negative value means the sequence \ newline was seen,
   which is supposed to be equivalent to nothing at all.

   If \ is followed by a null character, we return a negative
   value and leave the string pointer pointing at the null character.

   If \ is followed by 000, we return 0 and leave the string pointer
   after the zeros.  A value of 0 does not mean end of string.  */

static int
cp_parse_escape (const char **string_ptr)
{
  int target_char;
  int c = *(*string_ptr)++;
  if (c_parse_backslash (c, &target_char))
    return target_char;
  else
    switch (c)
      {
      case '\n':
	return -2;
      case 0:
	(*string_ptr)--;
	return 0;
      case '^':
	{
	  c = *(*string_ptr)++;

	  if (c == '?')
	    return 0177;
	  else if (c == '\\')
	    target_char = cp_parse_escape (string_ptr);
	  else
	    target_char = c;

	  /* Now target_char is something like `c', and we want to find
	     its control-character equivalent.  */
	  target_char = target_char & 037;

	  return target_char;
	}

      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
	{
	  int i = c - '0';
	  int count = 0;
	  while (++count < 3)
	    {
	      c = (**string_ptr);
	      if (c >= '0' && c <= '7')
		{
		  (*string_ptr)++;
		  i *= 8;
		  i += c - '0';
		}
	      else
		{
		  break;
		}
	    }
	  return i;
	}
      default:
	return c;
      }
}

#define HANDLE_SPECIAL(string, comp)				\
  if (startswith (tokstart, string))				\
    {								\
      state->lexptr = tokstart + sizeof (string) - 1;			\
      lvalp->lval = comp;					\
      return DEMANGLER_SPECIAL;					\
    }

#define HANDLE_TOKEN2(string, token)			\
  if (state->lexptr[1] == string[1])				\
    {							\
      state->lexptr += 2;					\
      lvalp->opname = string;				\
      return token;					\
    }      

#define HANDLE_TOKEN3(string, token)			\
  if (state->lexptr[1] == string[1] && state->lexptr[2] == string[2])	\
    {							\
      state->lexptr += 3;					\
      lvalp->opname = string;				\
      return token;					\
    }      

/* Read one token, getting characters through LEXPTR.  */

static int
yylex (YYSTYPE *lvalp, cpname_state *state)
{
  int c;
  int namelen;
  const char *tokstart;

 retry:
  state->prev_lexptr = state->lexptr;
  tokstart = state->lexptr;

  switch (c = *tokstart)
    {
    case 0:
      return 0;

    case ' ':
    case '\t':
    case '\n':
      state->lexptr++;
      goto retry;

    case '\'':
      /* We either have a character constant ('0' or '\177' for example)
	 or we have a quoted symbol reference ('foo(int,int)' in C++
	 for example). */
      state->lexptr++;
      c = *state->lexptr++;
      if (c == '\\')
	c = cp_parse_escape (&state->lexptr);
      else if (c == '\'')
	{
	  yyerror (state, _("empty character constant"));
	  return ERROR;
	}

      c = *state->lexptr++;
      if (c != '\'')
	{
	  yyerror (state, _("invalid character constant"));
	  return ERROR;
	}

      /* FIXME: We should refer to a canonical form of the character,
	 presumably the same one that appears in manglings - the decimal
	 representation.  But if that isn't in our input then we have to
	 allocate memory for it somewhere.  */
      lvalp->comp
	= state->fill_comp (DEMANGLE_COMPONENT_LITERAL,
			    state->make_builtin_type ("char"),
			    state->make_name (tokstart,
					      state->lexptr - tokstart));

      return INT;

    case '(':
      if (startswith (tokstart, "(anonymous namespace)"))
	{
	  state->lexptr += 21;
	  lvalp->comp = state->make_name ("(anonymous namespace)",
					  sizeof "(anonymous namespace)" - 1);
	  return NAME;
	}
	[[fallthrough]];

    case ')':
    case ',':
      state->lexptr++;
      return c;

    case '.':
      if (state->lexptr[1] == '.' && state->lexptr[2] == '.')
	{
	  state->lexptr += 3;
	  return ELLIPSIS;
	}

      /* Might be a floating point number.  */
      if (state->lexptr[1] < '0' || state->lexptr[1] > '9')
	goto symbol;		/* Nope, must be a symbol. */

      goto try_number;

    case '-':
      HANDLE_TOKEN2 ("-=", ASSIGN_MODIFY);
      HANDLE_TOKEN2 ("--", DECREMENT);
      HANDLE_TOKEN2 ("->", ARROW);

      /* For construction vtables.  This is kind of hokey.  */
      if (startswith (tokstart, "-in-"))
	{
	  state->lexptr += 4;
	  return CONSTRUCTION_IN;
	}

      if (state->lexptr[1] < '0' || state->lexptr[1] > '9')
	{
	  state->lexptr++;
	  return '-';
	}

    try_number:
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
	int hex = 0;

	if (c == '-')
	  p++;

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
	    if (!hex && !got_e && (*p == 'e' || *p == 'E'))
	      got_dot = got_e = 1;
	    /* This test does not include !hex, because a '.' always indicates
	       a decimal floating point number regardless of the radix.

	       NOTE drow/2005-03-09: This comment is not accurate in C99;
	       however, it's not clear that all the floating point support
	       in this file is doing any good here.  */
	    else if (!got_dot && *p == '.')
	      got_dot = 1;
	    else if (got_e && (p[-1] == 'e' || p[-1] == 'E')
		     && (*p == '-' || *p == '+'))
	      /* This is the sign of the exponent, not the end of the
		 number.  */
	      continue;
	    /* We will take any letters or digits.  parse_number will
	       complain if past the radix, or if L or U are not final.  */
	    else if (! ISALNUM (*p))
	      break;
	  }
	toktype = state->parse_number (tokstart, p - tokstart, got_dot|got_e,
				       lvalp);
	if (toktype == ERROR)
	  {
	    char *err_copy = (char *) alloca (p - tokstart + 1);

	    memcpy (err_copy, tokstart, p - tokstart);
	    err_copy[p - tokstart] = 0;
	    yyerror (state, _("invalid number"));
	    return ERROR;
	  }
	state->lexptr = p;
	return toktype;
      }

    case '+':
      HANDLE_TOKEN2 ("+=", ASSIGN_MODIFY);
      HANDLE_TOKEN2 ("++", INCREMENT);
      state->lexptr++;
      return c;
    case '*':
      HANDLE_TOKEN2 ("*=", ASSIGN_MODIFY);
      state->lexptr++;
      return c;
    case '/':
      HANDLE_TOKEN2 ("/=", ASSIGN_MODIFY);
      state->lexptr++;
      return c;
    case '%':
      HANDLE_TOKEN2 ("%=", ASSIGN_MODIFY);
      state->lexptr++;
      return c;
    case '|':
      HANDLE_TOKEN2 ("|=", ASSIGN_MODIFY);
      HANDLE_TOKEN2 ("||", OROR);
      state->lexptr++;
      return c;
    case '&':
      HANDLE_TOKEN2 ("&=", ASSIGN_MODIFY);
      HANDLE_TOKEN2 ("&&", ANDAND);
      state->lexptr++;
      return c;
    case '^':
      HANDLE_TOKEN2 ("^=", ASSIGN_MODIFY);
      state->lexptr++;
      return c;
    case '!':
      HANDLE_TOKEN2 ("!=", NOTEQUAL);
      state->lexptr++;
      return c;
    case '<':
      HANDLE_TOKEN3 ("<<=", ASSIGN_MODIFY);
      HANDLE_TOKEN2 ("<=", LEQ);
      HANDLE_TOKEN2 ("<<", LSH);
      state->lexptr++;
      return c;
    case '>':
      HANDLE_TOKEN3 (">>=", ASSIGN_MODIFY);
      HANDLE_TOKEN2 (">=", GEQ);
      HANDLE_TOKEN2 (">>", RSH);
      state->lexptr++;
      return c;
    case '=':
      HANDLE_TOKEN2 ("==", EQUAL);
      state->lexptr++;
      return c;
    case ':':
      HANDLE_TOKEN2 ("::", COLONCOLON);
      state->lexptr++;
      return c;

    case '[':
    case ']':
    case '?':
    case '@':
    case '~':
    case '{':
    case '}':
    symbol:
      state->lexptr++;
      return c;

    case '"':
      /* These can't occur in C++ names.  */
      yyerror (state, _("unexpected string literal"));
      return ERROR;
    }

  if (!(c == '_' || c == '$' || c_ident_is_alpha (c)))
    {
      /* We must have come across a bad character (e.g. ';').  */
      yyerror (state, _("invalid character"));
      return ERROR;
    }

  /* It's a name.  See how long it is.  */
  namelen = 0;
  do
    c = tokstart[++namelen];
  while (c_ident_is_alnum (c) || c == '_' || c == '$');

  state->lexptr += namelen;

  /* Catch specific keywords.  Notice that some of the keywords contain
     spaces, and are sorted by the length of the first word.  They must
     all include a trailing space in the string comparison.  */
  switch (namelen)
    {
    case 16:
      if (startswith (tokstart, "reinterpret_cast"))
	return REINTERPRET_CAST;
      break;
    case 12:
      if (startswith (tokstart, "construction vtable for "))
	{
	  state->lexptr = tokstart + 24;
	  return CONSTRUCTION_VTABLE;
	}
      if (startswith (tokstart, "dynamic_cast"))
	return DYNAMIC_CAST;
      break;
    case 11:
      if (startswith (tokstart, "static_cast"))
	return STATIC_CAST;
      break;
    case 9:
      HANDLE_SPECIAL ("covariant return thunk to ", DEMANGLE_COMPONENT_COVARIANT_THUNK);
      HANDLE_SPECIAL ("reference temporary for ", DEMANGLE_COMPONENT_REFTEMP);
      break;
    case 8:
      HANDLE_SPECIAL ("typeinfo for ", DEMANGLE_COMPONENT_TYPEINFO);
      HANDLE_SPECIAL ("typeinfo fn for ", DEMANGLE_COMPONENT_TYPEINFO_FN);
      HANDLE_SPECIAL ("typeinfo name for ", DEMANGLE_COMPONENT_TYPEINFO_NAME);
      if (startswith (tokstart, "operator"))
	return OPERATOR;
      if (startswith (tokstart, "restrict"))
	return RESTRICT;
      if (startswith (tokstart, "unsigned"))
	return UNSIGNED;
      if (startswith (tokstart, "template"))
	return TEMPLATE;
      if (startswith (tokstart, "volatile"))
	return VOLATILE_KEYWORD;
      break;
    case 7:
      HANDLE_SPECIAL ("virtual thunk to ", DEMANGLE_COMPONENT_VIRTUAL_THUNK);
      if (startswith (tokstart, "wchar_t"))
	return WCHAR_T;
      break;
    case 6:
      if (startswith (tokstart, "global constructors keyed to "))
	{
	  const char *p;
	  state->lexptr = tokstart + 29;
	  lvalp->lval = DEMANGLE_COMPONENT_GLOBAL_CONSTRUCTORS;
	  /* Find the end of the symbol.  */
	  p = symbol_end (state->lexptr);
	  lvalp->comp = state->make_name (state->lexptr, p - state->lexptr);
	  state->lexptr = p;
	  return DEMANGLER_SPECIAL;
	}
      if (startswith (tokstart, "global destructors keyed to "))
	{
	  const char *p;
	  state->lexptr = tokstart + 28;
	  lvalp->lval = DEMANGLE_COMPONENT_GLOBAL_DESTRUCTORS;
	  /* Find the end of the symbol.  */
	  p = symbol_end (state->lexptr);
	  lvalp->comp = state->make_name (state->lexptr, p - state->lexptr);
	  state->lexptr = p;
	  return DEMANGLER_SPECIAL;
	}

      HANDLE_SPECIAL ("vtable for ", DEMANGLE_COMPONENT_VTABLE);
      if (startswith (tokstart, "delete"))
	return DELETE;
      if (startswith (tokstart, "struct"))
	return STRUCT;
      if (startswith (tokstart, "signed"))
	return SIGNED_KEYWORD;
      if (startswith (tokstart, "sizeof"))
	return SIZEOF;
      if (startswith (tokstart, "double"))
	return DOUBLE_KEYWORD;
      break;
    case 5:
      HANDLE_SPECIAL ("guard variable for ", DEMANGLE_COMPONENT_GUARD);
      if (startswith (tokstart, "false"))
	return FALSEKEYWORD;
      if (startswith (tokstart, "class"))
	return CLASS;
      if (startswith (tokstart, "union"))
	return UNION;
      if (startswith (tokstart, "float"))
	return FLOAT_KEYWORD;
      if (startswith (tokstart, "short"))
	return SHORT;
      if (startswith (tokstart, "const"))
	return CONST_KEYWORD;
      break;
    case 4:
      if (startswith (tokstart, "void"))
	return VOID;
      if (startswith (tokstart, "bool"))
	return BOOL;
      if (startswith (tokstart, "char"))
	return CHAR;
      if (startswith (tokstart, "enum"))
	return ENUM;
      if (startswith (tokstart, "long"))
	return LONG;
      if (startswith (tokstart, "true"))
	return TRUEKEYWORD;
      break;
    case 3:
      HANDLE_SPECIAL ("VTT for ", DEMANGLE_COMPONENT_VTT);
      HANDLE_SPECIAL ("non-virtual thunk to ", DEMANGLE_COMPONENT_THUNK);
      if (startswith (tokstart, "new"))
	return NEW;
      if (startswith (tokstart, "int"))
	return INT_KEYWORD;
      break;
    default:
      break;
    }

  lvalp->comp = state->make_name (tokstart, namelen);
  return NAME;
}

static void
yyerror (cpname_state *state, const char *msg)
{
  if (state->global_errmsg)
    return;

  state->error_lexptr = state->prev_lexptr;
  state->global_errmsg = msg ? msg : "parse error";
}

/* Allocate a chunk of the components we'll need to build a tree.  We
   generally allocate too many components, but the extra memory usage
   doesn't hurt because the trees are temporary and the storage is
   reused.  More may be allocated later, by d_grab.  */
static struct demangle_info *
allocate_info (void)
{
  struct demangle_info *info = XNEW (struct demangle_info);

  info->next = NULL;
  info->used = 0;
  return info;
}

/* See cp-support.h.  */

gdb::unique_xmalloc_ptr<char>
cp_comp_to_string (struct demangle_component *result, int estimated_len)
{
  size_t err;

  char *res = gdb_cplus_demangle_print (DMGL_PARAMS | DMGL_ANSI,
					result, estimated_len, &err);
  return gdb::unique_xmalloc_ptr<char> (res);
}

/* Constructor for demangle_parse_info.  */

demangle_parse_info::demangle_parse_info ()
: info (NULL),
  tree (NULL)
{
  obstack_init (&obstack);
}

/* Destructor for demangle_parse_info.  */

demangle_parse_info::~demangle_parse_info ()
{
  /* Free any allocated chunks of memory for the parse.  */
  while (info != NULL)
    {
      struct demangle_info *next = info->next;

      free (info);
      info = next;
    }

  /* Free any memory allocated during typedef replacement.  */
  obstack_free (&obstack, NULL);
}

/* Merge the two parse trees given by DEST and SRC.  The parse tree
   in SRC is attached to DEST at the node represented by TARGET.

   NOTE 1: Since there is no API to merge obstacks, this function does
   even attempt to try it.  Fortunately, we do not (yet?) need this ability.
   The code will assert if SRC->obstack is not empty.

   NOTE 2: The string from which SRC was parsed must not be freed, since
   this function will place pointers to that string into DEST.  */

void
cp_merge_demangle_parse_infos (struct demangle_parse_info *dest,
			       struct demangle_component *target,
			       struct demangle_parse_info *src)

{
  struct demangle_info *di;

  /* Copy the SRC's parse data into DEST.  */
  *target = *src->tree;
  di = dest->info;
  while (di->next != NULL)
    di = di->next;
  di->next = src->info;

  /* Clear the (pointer to) SRC's parse data so that it is not freed when
     cp_demangled_parse_info_free is called.  */
  src->info = NULL;
}

/* Convert a demangled name to a demangle_component tree.  On success,
   a structure containing the root of the new tree is returned.  On
   error, NULL is returned, and an error message will be set in
   *ERRMSG.  */

struct std::unique_ptr<demangle_parse_info>
cp_demangled_name_to_comp (const char *demangled_name,
			   std::string *errmsg)
{
  cpname_state state;

  state.prev_lexptr = state.lexptr = demangled_name;
  state.error_lexptr = NULL;
  state.global_errmsg = NULL;

  state.demangle_info = allocate_info ();

  auto result = std::make_unique<demangle_parse_info> ();
  result->info = state.demangle_info;

  if (yyparse (&state))
    {
      if (state.global_errmsg && errmsg)
	*errmsg = state.global_errmsg;
      return NULL;
    }

  result->tree = state.global_result;

  return result;
}

#ifdef TEST_CPNAMES

static void
cp_print (struct demangle_component *result)
{
  char *str;
  size_t err = 0;

  str = gdb_cplus_demangle_print (DMGL_PARAMS | DMGL_ANSI, result, 64, &err);
  if (str == NULL)
    return;

  fputs (str, stdout);

  free (str);
}

static char
trim_chars (char *lexptr, char **extra_chars)
{
  char *p = (char *) symbol_end (lexptr);
  char c = 0;

  if (*p)
    {
      c = *p;
      *p = 0;
      *extra_chars = p + 1;
    }

  return c;
}

/* When this file is built as a standalone program, xmalloc comes from
   libiberty --- in which case we have to provide xfree ourselves.  */

void
xfree (void *ptr)
{
  if (ptr != NULL)
    {
      /* Literal `free' would get translated back to xfree again.  */
      CONCAT2 (fr,ee) (ptr);
    }
}

/* GDB normally defines internal_error itself, but when this file is built
   as a standalone program, we must also provide an implementation.  */

void
internal_error (const char *file, int line, const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  fprintf (stderr, "%s:%d: internal error: ", file, line);
  vfprintf (stderr, fmt, ap);
  exit (1);
}

int
main (int argc, char **argv)
{
  char *str2, *extra_chars, c;
  char buf[65536];
  int arg;

  arg = 1;
  if (argv[arg] && strcmp (argv[arg], "--debug") == 0)
    {
      yydebug = 1;
      arg++;
    }

  if (argv[arg] == NULL)
    while (fgets (buf, 65536, stdin) != NULL)
      {
	buf[strlen (buf) - 1] = 0;
	/* Use DMGL_VERBOSE to get expanded standard substitutions.  */
	c = trim_chars (buf, &extra_chars);
	str2 = cplus_demangle (buf, DMGL_PARAMS | DMGL_ANSI | DMGL_VERBOSE);
	if (str2 == NULL)
	  {
	    printf ("Demangling error\n");
	    if (c)
	      printf ("%s%c%s\n", buf, c, extra_chars);
	    else
	      printf ("%s\n", buf);
	    continue;
	  }

	std::string errmsg;
	std::unique_ptr<demangle_parse_info> result
	  = cp_demangled_name_to_comp (str2, &errmsg);
	if (result == NULL)
	  {
	    fputs (errmsg.c_str (), stderr);
	    fputc ('\n', stderr);
	    continue;
	  }

	cp_print (result->tree);

	free (str2);
	if (c)
	  {
	    putchar (c);
	    fputs (extra_chars, stdout);
	  }
	putchar ('\n');
      }
  else
    {
      std::string errmsg;
      std::unique_ptr<demangle_parse_info> result
	= cp_demangled_name_to_comp (argv[arg], &errmsg);
      if (result == NULL)
	{
	  fputs (errmsg.c_str (), stderr);
	  fputc ('\n', stderr);
	  return 0;
	}
      cp_print (result->tree);
      putchar ('\n');
    }
  return 0;
}

#endif
