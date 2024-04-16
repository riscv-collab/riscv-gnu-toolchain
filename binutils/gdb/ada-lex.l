/* FLEX lexer for Ada expressions, for GDB. -*- c++ -*-
   Copyright (C) 1994-2024 Free Software Foundation, Inc.

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

/*----------------------------------------------------------------------*/

/* The converted version of this file is to be included in ada-exp.y, */
/* the Ada parser for gdb.  The function yylex obtains characters from */
/* the global pointer lexptr.  It returns a syntactic category for */
/* each successive token and places a semantic value into yylval */
/* (ada-lval), defined by the parser.   */

DIG	[0-9]
NUM10	({DIG}({DIG}|_)*)
HEXDIG	[0-9a-f]
NUM16	({HEXDIG}({HEXDIG}|_)*)
OCTDIG	[0-7]
LETTER	[a-z_]
ID	({LETTER}({LETTER}|{DIG}|[\x80-\xff])*|"<"{LETTER}({LETTER}|{DIG})*">")
WHITE	[ \t\n]
TICK	("'"{WHITE}*)
GRAPHIC [a-z0-9 #&'()*+,-./:;<>=_|!$%?@\[\]\\^`{}~]
OPER    ([-+*/=<>&]|"<="|">="|"**"|"/="|"and"|"or"|"xor"|"not"|"mod"|"rem"|"abs")

EXP	(e[+-]{NUM10})
POSEXP  (e"+"?{NUM10})

/* This must agree with COMPLETION_CHAR below.  See the comment there
   for the explanation.  */
COMPLETE "\001"
NOT_COMPLETE [^\001]

%{

#include "diagnostics.h"

/* Some old versions of flex generate code that uses the "register" keyword,
   which clang warns about.  This was observed for example with flex 2.5.35,
   as shipped with macOS 10.12.  The same happens with flex 2.5.37 and g++ 11
   which defaults to ISO C++17, that does not allow register storage class
   specifiers.  */
DIAGNOSTIC_PUSH
DIAGNOSTIC_IGNORE_DEPRECATED_REGISTER

#define NUMERAL_WIDTH 256
#define LONGEST_SIGN ((ULONGEST) 1 << (sizeof(LONGEST) * HOST_CHAR_BIT - 1))

/* Temporary staging for numeric literals.  */
static char numbuf[NUMERAL_WIDTH];
 static void canonicalizeNumeral (char *s1, const char *);
static struct stoken processString (const char*, int);
static int processInt (struct parser_state *, const char *, const char *,
		       const char *);
static int processReal (struct parser_state *, const char *);
static struct stoken processId (const char *, int);
static int processAttribute (const char *);
static int find_dot_all (const char *);
static void rewind_to_char (int);

#undef YY_DECL
#define YY_DECL static int yylex ( void )

/* Flex generates a static function "input" which is not used.
   Defining YY_NO_INPUT comments it out.  */
#define YY_NO_INPUT

/* When completing, we'll return a special character at the end of the
   input, to signal the completion position to the lexer.  This is
   done because flex does not have a generally useful way to detect
   EOF in a pattern.  This variable records whether the special
   character has been emitted.  */
static bool returned_complete = false;

/* The character we use to represent the completion point.  */
#define COMPLETE_CHAR '\001'

#undef YY_INPUT
#define YY_INPUT(BUF, RESULT, MAX_SIZE)					\
  if ( *pstate->lexptr == '\000' )					\
    {									\
      if (pstate->parse_completion && !returned_complete)		\
	{								\
	  returned_complete = true;					\
	  *(BUF) = COMPLETE_CHAR;					\
	  (RESULT) = 1;							\
	}								\
      else								\
	(RESULT) = YY_NULL;						\
    }									\
  else									\
    {									\
      *(BUF) = *pstate->lexptr == COMPLETE_CHAR ? ' ' : *pstate->lexptr; \
      (RESULT) = 1;							\
      pstate->lexptr += 1;						\
    }

/* Depth of parentheses.  */
static int paren_depth;

%}

%option case-insensitive interactive nodefault noyywrap

%s BEFORE_QUAL_QUOTE

%%

{WHITE}		 { }

"--".*		 { yyterminate(); }

{NUM10}{POSEXP}  {
		   canonicalizeNumeral (numbuf, yytext);
		   char *e_ptr = strrchr (numbuf, 'e');
		   *e_ptr = '\0';
		   return processInt (pstate, nullptr, numbuf, e_ptr + 1);
		 }

{NUM10}          {
		   canonicalizeNumeral (numbuf, yytext);
		   return processInt (pstate, NULL, numbuf, NULL);
		 }

{NUM10}"#"{HEXDIG}({HEXDIG}|_)*"#"{POSEXP} {
		   canonicalizeNumeral (numbuf, yytext);
		   char *e_ptr = strrchr (numbuf, 'e');
		   *e_ptr = '\0';
		   return processInt (pstate, numbuf,
				      strchr (numbuf, '#') + 1,
				      e_ptr + 1);
		 }

	/* The "llf" is a gdb extension to allow a floating-point
	   constant to be written in some other base.  The
	   floating-point number is formed by reinterpreting the
	   bytes, allowing direct control over the bits.  */
{NUM10}(l{0,2}f)?"#"{HEXDIG}({HEXDIG}|_)*"#" {
		   canonicalizeNumeral (numbuf, yytext);
		   return processInt (pstate, numbuf, strchr (numbuf, '#') + 1,
				      NULL);
		 }

"0x"{HEXDIG}+	{
		  canonicalizeNumeral (numbuf, yytext+2);
		  return processInt (pstate, "16#", numbuf, NULL);
		}


{NUM10}"."{NUM10}{EXP} {
		   canonicalizeNumeral (numbuf, yytext);
		   return processReal (pstate, numbuf);
		}

{NUM10}"."{NUM10} {
		   canonicalizeNumeral (numbuf, yytext);
		   return processReal (pstate, numbuf);
		}

{NUM10}"#"{NUM16}"."{NUM16}"#"{EXP} {
                   error (_("Based real literals not implemented yet."));
		}

{NUM10}"#"{NUM16}"."{NUM16}"#" {
                   error (_("Based real literals not implemented yet."));
		}

<INITIAL>"'"({GRAPHIC}|\")"'" {
		   yylval.typed_char.val = yytext[1];
		   yylval.typed_char.type = type_for_char (pstate, yytext[1]);
		   return CHARLIT;
		}

<INITIAL>"'[\""{HEXDIG}{2,}"\"]'"   {
                   ULONGEST v = strtoulst (yytext+3, nullptr, 16);
		   yylval.typed_char.val = v;
                   yylval.typed_char.type = type_for_char (pstate, v);
		   return CHARLIT;
		}

	/* Note that we don't handle bracket sequences of more than 2
	   digits here.  Currently there's no support for wide or
	   wide-wide strings.  */
\"({GRAPHIC}|"[\""({HEXDIG}{2,}|\")"\"]")*\"   {
	           yylval.sval = processString (yytext+1, yyleng-2);
		   return STRING;
		}

\"              {
                   error (_("ill-formed or non-terminated string literal"));
		}


if		{
                  rewind_to_char ('i');
		  return 0;
		}

task            {
                  rewind_to_char ('t');
		  return 0;
		}

thread{WHITE}+{DIG} {
                  /* This keyword signals the end of the expression and
                     will be processed separately.  */
                  rewind_to_char ('t');
		  return 0;
		}

	/* ADA KEYWORDS */

abs		{ return ABS; }
and		{ return _AND_; }
else		{ return ELSE; }
in		{ return IN; }
mod		{ return MOD; }
new		{ return NEW; }
not		{ return NOT; }
null		{ return NULL_PTR; }
or		{ return OR; }
others          { return OTHERS; }
rem		{ return REM; }
then		{ return THEN; }
xor		{ return XOR; }

	/* BOOLEAN "KEYWORDS" */

 /* True and False are not keywords in Ada, but rather enumeration constants.
    However, the boolean type is no longer represented as an enum, so True
    and False are no longer defined in symbol tables.  We compromise by
    making them keywords (when bare). */

true		{ return TRUEKEYWORD; }
false		{ return FALSEKEYWORD; }

        /* ATTRIBUTES */

{TICK}([a-z][a-z_]*)?{COMPLETE}? { BEGIN INITIAL; return processAttribute (yytext); }

	/* PUNCTUATION */

"=>"		{ return ARROW; }
".."		{ return DOTDOT; }
"**"		{ return STARSTAR; }
":="		{ return ASSIGN; }
"/="		{ return NOTEQUAL; }
"<="		{ return LEQ; }
">="		{ return GEQ; }

<BEFORE_QUAL_QUOTE>"'"/{NOT_COMPLETE} { BEGIN INITIAL; return '\''; }

[-&*+{}@/:<>=|;\[\]] { return yytext[0]; }

","		{ if (paren_depth == 0 && pstate->comma_terminates)
		    {
		      rewind_to_char (',');
		      return 0;
		    }
		  else
		    return ',';
		}

"("		{ paren_depth += 1; return '('; }
")"		{ if (paren_depth == 0)
		    {
		      rewind_to_char (')');
		      return 0;
		    }
		  else
 		    {
		      paren_depth -= 1;
		      return ')';
		    }
		}

"."{WHITE}*{ID}{COMPLETE}? {
	 	  yylval.sval = processId (yytext+1, yyleng-1);
		  if (yytext[yyleng - 1] == COMPLETE_CHAR)
		    return DOT_COMPLETE;
	          return DOT_ID;
		}

"."{WHITE}*{COMPLETE} {
		  yylval.sval.ptr = "";
		  yylval.sval.length = 0;
		  return DOT_COMPLETE;
		}

{ID}({WHITE}*"."{WHITE}*({ID}|\"{OPER}\"))*(" "*"'"|{COMPLETE})?  {
                  int all_posn = find_dot_all (yytext);

                  if (all_posn == -1 && yytext[yyleng-1] == '\'')
		    {
		      BEGIN BEFORE_QUAL_QUOTE;
		      yyless (yyleng-1);
		    }
                  else if (all_posn >= 0)
		    yyless (all_posn);
		  bool is_completion = yytext[yyleng - 1] == COMPLETE_CHAR;
                  yylval.sval = processId (yytext, yyleng);
                  return is_completion ? NAME_COMPLETE : NAME;
               }


	/* GDB EXPRESSION CONSTRUCTS  */

"'"[^']+"'"{WHITE}*:: {
                  yyless (yyleng - 2);
		  yylval.sval = processId (yytext, yyleng);
		  return NAME;
		}

"::"            { return COLONCOLON; }

	/* REGISTERS AND GDB CONVENIENCE VARIABLES */

"$"({LETTER}|{DIG}|"$")*  {
		  yylval.sval.ptr = yytext;
		  yylval.sval.length = yyleng;
		  return DOLLAR_VARIABLE;
		}

	/* CATCH-ALL ERROR CASE */

.		{ error (_("Invalid character '%s' in expression."), yytext); }
%%

#include <ctype.h>
/* Initialize the lexer for processing new expression. */

static void
lexer_init (FILE *inp)
{
  BEGIN INITIAL;
  paren_depth = 0;
  returned_complete = false;
  yyrestart (inp);
}


/* Copy S2 to S1, removing all underscores, and downcasing all letters.  */

static void
canonicalizeNumeral (char *s1, const char *s2)
{
  for (; *s2 != '\000'; s2 += 1)
    {
      if (*s2 != '_')
	{
	  *s1 = tolower(*s2);
	  s1 += 1;
	}
    }
  s1[0] = '\000';
}

/* Interprets the prefix of NUM that consists of digits of the given BASE
   as an integer of that BASE, with the string EXP as an exponent.
   Puts value in yylval, and returns INT, if the string is valid.  Causes
   an error if the number is improperly formatted.   BASE, if NULL, defaults
   to "10", and EXP to "1".  The EXP does not contain a leading 'e' or 'E'.
 */

static int
processInt (struct parser_state *par_state, const char *base0,
	    const char *num0, const char *exp0)
{
  long exp;
  int base;
  /* For the based literal with an "f" prefix, we'll return a
     floating-point number.  This counts the the number of "l"s seen,
     to decide the width of the floating-point number to return.  -1
     means no "f".  */
  int floating_point_l_count = -1;

  if (base0 == NULL)
    base = 10;
  else
    {
      char *end_of_base;
      base = strtol (base0, &end_of_base, 10);
      if (base < 2 || base > 16)
	error (_("Invalid base: %d."), base);
      while (*end_of_base == 'l')
	{
	  ++floating_point_l_count;
	  ++end_of_base;
	}
      /* This assertion is ensured by the pattern.  */
      gdb_assert (floating_point_l_count == -1 || *end_of_base == 'f');
      if (*end_of_base == 'f')
	{
	  ++end_of_base;
	  ++floating_point_l_count;
	}
      /* This assertion is ensured by the pattern.  */
      gdb_assert (*end_of_base == '#');
    }

  if (exp0 == NULL)
    exp = 0;
  else
    exp = strtol(exp0, (char **) NULL, 10);

  gdb_mpz result;
  while (isxdigit (*num0))
    {
      int dig = fromhex (*num0);
      if (dig >= base)
	error (_("Invalid digit `%c' in based literal"), *num0);
      result *= base;
      result += dig;
      ++num0;
    }

  while (exp > 0)
    {
      result *= base;
      exp -= 1;
    }

  if (floating_point_l_count > -1)
    {
      struct type *fp_type;
      if (floating_point_l_count == 0)
	fp_type = language_lookup_primitive_type (par_state->language (),
						  par_state->gdbarch (),
						  "float");
      else if (floating_point_l_count == 1)
	fp_type = language_lookup_primitive_type (par_state->language (),
						  par_state->gdbarch (),
						  "long_float");
      else
	{
	  /* This assertion is ensured by the pattern.  */
	  gdb_assert (floating_point_l_count == 2);
	  fp_type = language_lookup_primitive_type (par_state->language (),
						    par_state->gdbarch (),
						    "long_long_float");
	}

      yylval.typed_val_float.type = fp_type;
      result.write (gdb::make_array_view (yylval.typed_val_float.val,
					  fp_type->length ()),
		    type_byte_order (fp_type),
		    true);

      return FLOAT;
    }

  int_storage.emplace_back (new gdb_mpz (std::move (result)));
  const gdb_mpz *value = int_storage.back ().get ();

  int int_bits = gdbarch_int_bit (par_state->gdbarch ());
  int long_bits = gdbarch_long_bit (par_state->gdbarch ());
  int long_long_bits = gdbarch_long_long_bit (par_state->gdbarch ());

  if (fits_in_type (1, *value, int_bits, true))
    yylval.typed_val.type = parse_type (par_state)->builtin_int;
  else if (fits_in_type (1, *value, long_bits, true))
    yylval.typed_val.type = parse_type (par_state)->builtin_long;
  else if (fits_in_type (1, *value, long_bits, false))
    yylval.typed_val.type
      = builtin_type (par_state->gdbarch ())->builtin_unsigned_long;
  else if (fits_in_type (1, *value, long_long_bits, true))
    yylval.typed_val.type = parse_type (par_state)->builtin_long_long;
  else if (fits_in_type (1, *value, long_long_bits, false))
    yylval.typed_val.type
      = builtin_type (par_state->gdbarch ())->builtin_unsigned_long_long;
  else if (fits_in_type (1, *value, 128, true))
    yylval.typed_val.type
      = language_lookup_primitive_type (par_state->language (),
					par_state->gdbarch (),
					"long_long_long_integer");
  else if (fits_in_type (1, *value, 128, false))
    yylval.typed_val.type
      = language_lookup_primitive_type (par_state->language (),
					par_state->gdbarch (),
					"unsigned_long_long_long_integer");
  else
    error (_("Integer literal out of range"));

  yylval.typed_val.val = value;
  return INT;
}

static int
processReal (struct parser_state *par_state, const char *num0)
{
  yylval.typed_val_float.type = parse_type (par_state)->builtin_long_double;

  bool parsed = parse_float (num0, strlen (num0),
			     yylval.typed_val_float.type,
			     yylval.typed_val_float.val);
  gdb_assert (parsed);
  return FLOAT;
}


/* Store a canonicalized version of NAME0[0..LEN-1] in yylval.ssym.  The
   resulting string is valid until the next call to ada_parse.  If
   NAME0 contains the substring "___", it is assumed to be already
   encoded and the resulting name is equal to it.  Similarly, if the name
   starts with '<', it is copied verbatim.  Otherwise, it differs
   from NAME0 in that:
    + Characters between '...' are transfered verbatim to yylval.ssym.
    + Trailing "'" characters in quoted sequences are removed (a leading quote is
      preserved to indicate that the name is not to be GNAT-encoded).
    + Unquoted whitespace is removed.
    + Unquoted alphabetic characters are mapped to lower case.
   Result is returned as a struct stoken, but for convenience, the string
   is also null-terminated.  Result string valid until the next call of
   ada_parse.
 */
static struct stoken
processId (const char *name0, int len)
{
  char *name = (char *) obstack_alloc (&temp_parse_space, len + 11);
  int i0, i;
  struct stoken result;

  result.ptr = name;
  while (len > 0 && isspace (name0[len-1]))
    len -= 1;

  if (name0[0] == '<' || strstr (name0, "___") != NULL)
    {
      strncpy (name, name0, len);
      name[len] = '\000';
      result.length = len;
      return result;
    }

  bool in_quotes = false;
  i = i0 = 0;
  while (i0 < len)
    {
      if (name0[i0] == COMPLETE_CHAR)
	{
	  /* Just ignore.  */
	  ++i0;
	}
      else if (in_quotes)
	name[i++] = name0[i0++];
      else if (isalnum (name0[i0]))
	{
	  name[i] = tolower (name0[i0]);
	  i += 1; i0 += 1;
	}
      else if (isspace (name0[i0]))
	i0 += 1;
      else if (name0[i0] == '\'')
	{
	  /* Copy the starting quote, but not the ending quote.  */
	  if (!in_quotes)
	    name[i++] = name0[i0++];
	  in_quotes = !in_quotes;
	}
      else
	name[i++] = name0[i0++];
    }
  name[i] = '\000';

  result.length = i;
  return result;
}

/* Return TEXT[0..LEN-1], a string literal without surrounding quotes,
   with special hex character notations replaced with characters. 
   Result valid until the next call to ada_parse.  */

static struct stoken
processString (const char *text, int len)
{
  const char *p;
  char *q;
  const char *lim = text + len;
  struct stoken result;

  q = (char *) obstack_alloc (&temp_parse_space, len);
  result.ptr = q;
  p = text;
  while (p < lim)
    {
      if (p[0] == '[' && p[1] == '"' && p+2 < lim)
         {
           if (p[2] == '"')  /* "...["""]... */
             {
               *q = '"';
	       p += 4;
	     }
           else
	     {
	       const char *end;
	       ULONGEST chr = strtoulst (p + 2, &end, 16);
	       if (chr > 0xff)
		 error (_("wide strings are not yet supported"));
	       *q = (char) chr;
	       p = end + 1;
	     }
         }
       else
         *q = *p;
       q += 1;
       p += 1;
     }
  result.length = q - result.ptr;
  return result;
}

/* Returns the position within STR of the '.' in a
   '.{WHITE}*all' component of a dotted name, or -1 if there is none.
   Note: we actually don't need this routine, since 'all' can never be an
   Ada identifier.  Thus, looking up foo.all or foo.all.x as a name
   must fail, and will eventually be interpreted as (foo).all or
   (foo).all.x.  However, this does avoid an extraneous lookup. */

static int
find_dot_all (const char *str)
{
  int i;

  for (i = 0; str[i] != '\000'; i++)
    if (str[i] == '.')
      {
	int i0 = i;

	do
	  i += 1;
	while (isspace (str[i]));

	if (strncasecmp (str + i, "all", 3) == 0
	    && !isalnum (str[i + 3]) && str[i + 3] != '_')
	  return i0;
      }
  return -1;
}

/* Returns non-zero iff string SUBSEQ matches a subsequence of STR, ignoring
   case.  */

static int
subseqMatch (const char *subseq, const char *str)
{
  if (subseq[0] == '\0')
    return 1;
  else if (str[0] == '\0')
    return 0;
  else if (tolower (subseq[0]) == tolower (str[0]))
    return subseqMatch (subseq+1, str+1) || subseqMatch (subseq, str+1);
  else
    return subseqMatch (subseq, str+1);
}


static struct { const char *name; int code; }
attributes[] = {
  { "address", TICK_ADDRESS },
  { "unchecked_access", TICK_ACCESS },
  { "unrestricted_access", TICK_ACCESS },
  { "access", TICK_ACCESS },
  { "first", TICK_FIRST },
  { "last", TICK_LAST },
  { "length", TICK_LENGTH },
  { "max", TICK_MAX },
  { "min", TICK_MIN },
  { "modulus", TICK_MODULUS },
  { "pos", TICK_POS },
  { "range", TICK_RANGE },
  { "size", TICK_SIZE },
  { "tag", TICK_TAG },
  { "val", TICK_VAL },
  { "enum_rep", TICK_ENUM_REP },
  { "enum_val", TICK_ENUM_VAL },
};

/* Return the syntactic code corresponding to the attribute name or
   abbreviation STR.  */

static int
processAttribute (const char *str)
{
  gdb_assert (*str == '\'');
  ++str;
  while (isspace (*str))
    ++str;

  int len = strlen (str);
  if (len > 0 && str[len - 1] == COMPLETE_CHAR)
    {
      /* This is enforced by YY_INPUT.  */
      gdb_assert (pstate->parse_completion);
      yylval.sval.ptr = obstack_strndup (&temp_parse_space, str, len - 1);
      yylval.sval.length = len - 1;
      return TICK_COMPLETE;
    }

  for (const auto &item : attributes)
    if (strcasecmp (str, item.name) == 0)
      return item.code;

  std::optional<int> found;
  for (const auto &item : attributes)
    if (subseqMatch (str, item.name))
      {
	if (!found.has_value ())
	  found = item.code;
	else
	  error (_("ambiguous attribute name: `%s'"), str);
      }
  if (!found.has_value ())
    error (_("unrecognized attribute: `%s'"), str);

  return *found;
}

bool
ada_tick_completer::complete (struct expression *exp,
			      completion_tracker &tracker)
{
  completion_list output;
  for (const auto &item : attributes)
    {
      if (strncasecmp (item.name, m_name.c_str (), m_name.length ()) == 0)
	output.emplace_back (xstrdup (item.name));
    }
  tracker.add_completions (std::move (output));
  return true;
}

/* Back up lexptr by yyleng and then to the rightmost occurrence of
   character CH, case-folded (there must be one).  WARNING: since
   lexptr points to the next input character that Flex has not yet
   transferred to its internal buffer, the use of this function
   depends on the assumption that Flex calls YY_INPUT only when it is
   logically necessary to do so (thus, there is no reading ahead
   farther than needed to identify the next token.)  */

static void
rewind_to_char (int ch)
{
  pstate->lexptr -= yyleng;
  while (toupper (*pstate->lexptr) != toupper (ch))
    pstate->lexptr -= 1;
  yyrestart (NULL);
}

/* Dummy definition to suppress warnings about unused static definitions. */
typedef void (*dummy_function) ();
dummy_function ada_flex_use[] = 
{ 
  (dummy_function) yyunput
};

DIAGNOSTIC_POP
