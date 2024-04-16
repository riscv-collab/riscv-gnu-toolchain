/* C preprocessor macro expansion for GDB.
   Copyright (C) 2002-2024 Free Software Foundation, Inc.
   Contributed by Red Hat, Inc.

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

#include "defs.h"
#include "gdbsupport/gdb_obstack.h"
#include "macrotab.h"
#include "macroexp.h"
#include "macroscope.h"
#include "c-lang.h"




/* A string type that we can use to refer to substrings of other
   strings.  */

struct shared_macro_buffer
{
  /* An array of characters.  This buffer is a pointer into some
     larger string and thus we can't assume in that the text is
     null-terminated.  */
  const char *text;

  /* The number of characters in the string.  */
  int len;

  /* For detecting token splicing. 

     This is the index in TEXT of the first character of the token
     that abuts the end of TEXT.  If TEXT contains no tokens, then we
     set this equal to LEN.  If TEXT ends in whitespace, then there is
     no token abutting the end of TEXT (it's just whitespace), and
     again, we set this equal to LEN.  We set this to -1 if we don't
     know the nature of TEXT.  */
  int last_token = -1;

  /* If this buffer is holding the result from get_token, then this 
     is non-zero if it is an identifier token, zero otherwise.  */
  int is_identifier = 0;

  shared_macro_buffer ()
    : text (NULL),
      len (0)
  {
  }

  /* Set the macro buffer to refer to the LEN bytes at ADDR, as a
     shared substring.  */
  shared_macro_buffer (const char *addr, int len)
  {
    set_shared (addr, len);
  }

  /* Set the macro buffer to refer to the LEN bytes at ADDR, as a
     shared substring.  */
  void set_shared (const char *addr, int len_)
  {
    text = addr;
    len = len_;
  }
};

/* A string type that we can resize and quickly append to.  */

struct growable_macro_buffer
{
  /* An array of characters.  The first LEN bytes are the real text,
     but there are SIZE bytes allocated to the array.  */
  char *text;

  /* The number of characters in the string.  */
  int len;

  /* The number of characters allocated to the string.  */
  int size;

  /* For detecting token splicing.

     This is the index in TEXT of the first character of the token
     that abuts the end of TEXT.  If TEXT contains no tokens, then we
     set this equal to LEN.  If TEXT ends in whitespace, then there is
     no token abutting the end of TEXT (it's just whitespace), and
     again, we set this equal to LEN.  We set this to -1 if we don't
     know the nature of TEXT.  */
  int last_token = -1;

  /* Set the macro buffer to the empty string, guessing that its
     final contents will fit in N bytes.  (It'll get resized if it
     doesn't, so the guess doesn't have to be right.)  Allocate the
     initial storage with xmalloc.  */
  explicit growable_macro_buffer (int n)
    : len (0),
      size (n)
  {
    if (n > 0)
      text = (char *) xmalloc (n);
    else
      text = NULL;
  }

  DISABLE_COPY_AND_ASSIGN (growable_macro_buffer);

  ~growable_macro_buffer ()
  {
    xfree (text);
  }

  /* Release the text of the buffer to the caller.  */
  gdb::unique_xmalloc_ptr<char> release ()
  {
    gdb_assert (size);
    char *result = text;
    text = NULL;
    return gdb::unique_xmalloc_ptr<char> (result);
  }

  /* Resize the buffer to be at least N bytes long.  */
  void resize_buffer (int n)
  {
    if (size == 0)
      size = n;
    else
      while (size <= n)
	size *= 2;

    text = (char *) xrealloc (text, size);
  }

  /* Append the character C to the buffer.  */
  void appendc (int c)
  {
    int new_len = len + 1;

    if (new_len > size)
      resize_buffer (new_len);

    text[len] = c;
    len = new_len;
  }

  /* Append the COUNT bytes at ADDR to the buffer.  */
  void appendmem (const char *addr, int count)
  {
    int new_len = len + count;

    if (new_len > size)
      resize_buffer (new_len);

    memcpy (text + len, addr, count);
    len = new_len;
  }
};



/* Recognizing preprocessor tokens.  */


int
macro_is_whitespace (int c)
{
  return (c == ' '
	  || c == '\t'
	  || c == '\n'
	  || c == '\v'
	  || c == '\f');
}


int
macro_is_digit (int c)
{
  return ('0' <= c && c <= '9');
}


int
macro_is_identifier_nondigit (int c)
{
  return (c == '_'
	  || ('a' <= c && c <= 'z')
	  || ('A' <= c && c <= 'Z'));
}


static void
set_token (shared_macro_buffer *tok, const char *start, const char *end)
{
  tok->set_shared (start, end - start);
  tok->last_token = 0;

  /* Presumed; get_identifier may overwrite this.  */
  tok->is_identifier = 0;
}


static int
get_comment (shared_macro_buffer *tok, const char *p, const char *end)
{
  if (p + 2 > end)
    return 0;
  else if (p[0] == '/'
	   && p[1] == '*')
    {
      const char *tok_start = p;

      p += 2;

      for (; p < end; p++)
	if (p + 2 <= end
	    && p[0] == '*'
	    && p[1] == '/')
	  {
	    p += 2;
	    set_token (tok, tok_start, p);
	    return 1;
	  }

      error (_("Unterminated comment in macro expansion."));
    }
  else if (p[0] == '/'
	   && p[1] == '/')
    {
      const char *tok_start = p;

      p += 2;
      for (; p < end; p++)
	if (*p == '\n')
	  break;

      set_token (tok, tok_start, p);
      return 1;
    }
  else
    return 0;
}


static int
get_identifier (shared_macro_buffer *tok, const char *p, const char *end)
{
  if (p < end
      && macro_is_identifier_nondigit (*p))
    {
      const char *tok_start = p;

      while (p < end
	     && (macro_is_identifier_nondigit (*p)
		 || macro_is_digit (*p)))
	p++;

      set_token (tok, tok_start, p);
      tok->is_identifier = 1;
      return 1;
    }
  else
    return 0;
}


static int
get_pp_number (shared_macro_buffer *tok, const char *p, const char *end)
{
  if (p < end
      && (macro_is_digit (*p)
	  || (*p == '.'
	      && p + 2 <= end
	      && macro_is_digit (p[1]))))
    {
      const char *tok_start = p;

      while (p < end)
	{
	  if (p + 2 <= end
	      && strchr ("eEpP", *p)
	      && (p[1] == '+' || p[1] == '-'))
	    p += 2;
	  else if (macro_is_digit (*p)
		   || macro_is_identifier_nondigit (*p)
		   || *p == '.')
	    p++;
	  else
	    break;
	}

      set_token (tok, tok_start, p);
      return 1;
    }
  else
    return 0;
}



/* If the text starting at P going up to (but not including) END
   starts with a character constant, set *TOK to point to that
   character constant, and return 1.  Otherwise, return zero.
   Signal an error if it contains a malformed or incomplete character
   constant.  */
static int
get_character_constant (shared_macro_buffer *tok,
			const char *p, const char *end)
{
  /* ISO/IEC 9899:1999 (E)  Section 6.4.4.4  paragraph 1 
     But of course, what really matters is that we handle it the same
     way GDB's C/C++ lexer does.  So we call parse_escape in utils.c
     to handle escape sequences.  */
  if ((p + 1 <= end && *p == '\'')
      || (p + 2 <= end
	  && (p[0] == 'L' || p[0] == 'u' || p[0] == 'U')
	  && p[1] == '\''))
    {
      const char *tok_start = p;
      int char_count = 0;

      if (*p == '\'')
	p++;
      else if (*p == 'L' || *p == 'u' || *p == 'U')
	p += 2;
      else
	gdb_assert_not_reached ("unexpected character constant");

      for (;;)
	{
	  if (p >= end)
	    error (_("Unmatched single quote."));
	  else if (*p == '\'')
	    {
	      if (!char_count)
		error (_("A character constant must contain at least one "
		       "character."));
	      p++;
	      break;
	    }
	  else if (*p == '\\')
	    {
	      const char *s, *o;

	      s = o = ++p;
	      char_count += c_parse_escape (&s, NULL);
	      p += s - o;
	    }
	  else
	    {
	      p++;
	      char_count++;
	    }
	}

      set_token (tok, tok_start, p);
      return 1;
    }
  else
    return 0;
}


/* If the text starting at P going up to (but not including) END
   starts with a string literal, set *TOK to point to that string
   literal, and return 1.  Otherwise, return zero.  Signal an error if
   it contains a malformed or incomplete string literal.  */
static int
get_string_literal (shared_macro_buffer *tok, const char *p, const char *end)
{
  if ((p + 1 <= end
       && *p == '"')
      || (p + 2 <= end
	  && (p[0] == 'L' || p[0] == 'u' || p[0] == 'U')
	  && p[1] == '"'))
    {
      const char *tok_start = p;

      if (*p == '"')
	p++;
      else if (*p == 'L' || *p == 'u' || *p == 'U')
	p += 2;
      else
	gdb_assert_not_reached ("unexpected string literal");

      for (;;)
	{
	  if (p >= end)
	    error (_("Unterminated string in expression."));
	  else if (*p == '"')
	    {
	      p++;
	      break;
	    }
	  else if (*p == '\n')
	    error (_("Newline characters may not appear in string "
		   "constants."));
	  else if (*p == '\\')
	    {
	      const char *s, *o;

	      s = o = ++p;
	      c_parse_escape (&s, NULL);
	      p += s - o;
	    }
	  else
	    p++;
	}

      set_token (tok, tok_start, p);
      return 1;
    }
  else
    return 0;
}


static int
get_punctuator (shared_macro_buffer *tok, const char *p, const char *end)
{
  /* Here, speed is much less important than correctness and clarity.  */

  /* ISO/IEC 9899:1999 (E)  Section 6.4.6  Paragraph 1.
     Note that this table is ordered in a special way.  A punctuator
     which is a prefix of another punctuator must appear after its
     "extension".  Otherwise, the wrong token will be returned.  */
  static const char * const punctuators[] = {
    "[", "]", "(", ")", "{", "}", "?", ";", ",", "~",
    "...", ".",
    "->", "--", "-=", "-",
    "++", "+=", "+",
    "*=", "*",
    "!=", "!",
    "&&", "&=", "&",
    "/=", "/",
    "%>", "%:%:", "%:", "%=", "%",
    "^=", "^",
    "##", "#",
    ":>", ":",
    "||", "|=", "|",
    "<<=", "<<", "<=", "<:", "<%", "<",
    ">>=", ">>", ">=", ">",
    "==", "=",
    0
  };

  int i;

  if (p + 1 <= end)
    {
      for (i = 0; punctuators[i]; i++)
	{
	  const char *punctuator = punctuators[i];

	  if (p[0] == punctuator[0])
	    {
	      int len = strlen (punctuator);

	      if (p + len <= end
		  && ! memcmp (p, punctuator, len))
		{
		  set_token (tok, p, p + len);
		  return 1;
		}
	    }
	}
    }

  return 0;
}


/* Peel the next preprocessor token off of SRC, and put it in TOK.
   Mutate TOK to refer to the first token in SRC, and mutate SRC to
   refer to the text after that token.  The resulting TOK will point
   into the same string SRC does.  Initialize TOK's last_token field.
   Return non-zero if we succeed, or 0 if we didn't find any more
   tokens in SRC.  */

static int
get_token (shared_macro_buffer *tok, shared_macro_buffer *src)
{
  const char *p = src->text;
  const char *end = p + src->len;

  /* From the ISO C standard, ISO/IEC 9899:1999 (E), section 6.4:

     preprocessing-token: 
	 header-name
	 identifier
	 pp-number
	 character-constant
	 string-literal
	 punctuator
	 each non-white-space character that cannot be one of the above

     We don't have to deal with header-name tokens, since those can
     only occur after a #include, which we will never see.  */

  while (p < end)
    if (macro_is_whitespace (*p))
      p++;
    else if (get_comment (tok, p, end))
      p += tok->len;
    else if (get_pp_number (tok, p, end)
	     || get_character_constant (tok, p, end)
	     || get_string_literal (tok, p, end)
	     /* Note: the grammar in the standard seems to be
		ambiguous: L'x' can be either a wide character
		constant, or an identifier followed by a normal
		character constant.  By trying `get_identifier' after
		we try get_character_constant and get_string_literal,
		we give the wide character syntax precedence.  Now,
		since GDB doesn't handle wide character constants
		anyway, is this the right thing to do?  */
	     || get_identifier (tok, p, end)
	     || get_punctuator (tok, p, end))
      {
	/* How many characters did we consume, including whitespace?  */
	int consumed = p - src->text + tok->len;

	src->text += consumed;
	src->len -= consumed;
	return 1;
      }
    else 
      {
	/* We have found a "non-whitespace character that cannot be
	   one of the above."  Make a token out of it.  */
	int consumed;

	set_token (tok, p, p + 1);
	consumed = p - src->text + tok->len;
	src->text += consumed;
	src->len -= consumed;
	return 1;
      }

  return 0;
}



/* Appending token strings, with and without splicing  */


/* Append the macro buffer SRC to the end of DEST, and ensure that
   doing so doesn't splice the token at the end of SRC with the token
   at the beginning of DEST.  SRC and DEST must have their last_token
   fields set.  Upon return, DEST's last_token field is set correctly.

   For example:

   If DEST is "(" and SRC is "y", then we can return with
   DEST set to "(y" --- we've simply appended the two buffers.

   However, if DEST is "x" and SRC is "y", then we must not return
   with DEST set to "xy" --- that would splice the two tokens "x" and
   "y" together to make a single token "xy".  However, it would be
   fine to return with DEST set to "x y".  Similarly, "<" and "<" must
   yield "< <", not "<<", etc.  */
static void
append_tokens_without_splicing (growable_macro_buffer *dest,
				shared_macro_buffer *src)
{
  int original_dest_len = dest->len;
  shared_macro_buffer dest_tail, new_token;

  gdb_assert (src->last_token != -1);
  gdb_assert (dest->last_token != -1);
  
  /* First, just try appending the two, and call get_token to see if
     we got a splice.  */
  dest->appendmem (src->text, src->len);

  /* If DEST originally had no token abutting its end, then we can't
     have spliced anything, so we're done.  */
  if (dest->last_token == original_dest_len)
    {
      dest->last_token = original_dest_len + src->last_token;
      return;
    }

  /* Set DEST_TAIL to point to the last token in DEST, followed by
     all the stuff we just appended.  */
  dest_tail.set_shared (dest->text + dest->last_token,
			dest->len - dest->last_token);

  /* Re-parse DEST's last token.  We know that DEST used to contain
     at least one token, so if it doesn't contain any after the
     append, then we must have spliced "/" and "*" or "/" and "/" to
     make a comment start.  (Just for the record, I got this right
     the first time.  This is not a bug fix.)  */
  if (get_token (&new_token, &dest_tail)
      && (new_token.text + new_token.len
	  == dest->text + original_dest_len))
    {
      /* No splice, so we're done.  */
      dest->last_token = original_dest_len + src->last_token;
      return;
    }

  /* Okay, a simple append caused a splice.  Let's chop dest back to
     its original length and try again, but separate the texts with a
     space.  */
  dest->len = original_dest_len;
  dest->appendc (' ');
  dest->appendmem (src->text, src->len);

  dest_tail.set_shared (dest->text + dest->last_token,
			dest->len - dest->last_token);

  /* Try to re-parse DEST's last token, as above.  */
  if (get_token (&new_token, &dest_tail)
      && (new_token.text + new_token.len
	  == dest->text + original_dest_len))
    {
      /* No splice, so we're done.  */
      dest->last_token = original_dest_len + 1 + src->last_token;
      return;
    }

  /* As far as I know, there's no case where inserting a space isn't
     enough to prevent a splice.  */
  internal_error (_("unable to avoid splicing tokens during macro expansion"));
}

/* Stringify an argument, and insert it into DEST.  ARG is the text to
   stringify; it is LEN bytes long.  */

static void
stringify (growable_macro_buffer *dest, const char *arg, int len)
{
  /* Trim initial whitespace from ARG.  */
  while (len > 0 && macro_is_whitespace (*arg))
    {
      ++arg;
      --len;
    }

  /* Trim trailing whitespace from ARG.  */
  while (len > 0 && macro_is_whitespace (arg[len - 1]))
    --len;

  /* Insert the string.  */
  dest->appendc ('"');
  while (len > 0)
    {
      /* We could try to handle strange cases here, like control
	 characters, but there doesn't seem to be much point.  */
      if (macro_is_whitespace (*arg))
	{
	  /* Replace a sequence of whitespace with a single space.  */
	  dest->appendc (' ');
	  while (len > 1 && macro_is_whitespace (arg[1]))
	    {
	      ++arg;
	      --len;
	    }
	}
      else if (*arg == '\\' || *arg == '"')
	{
	  dest->appendc ('\\');
	  dest->appendc (*arg);
	}
      else
	dest->appendc (*arg);
      ++arg;
      --len;
    }
  dest->appendc ('"');
  dest->last_token = dest->len;
}

/* See macroexp.h.  */

gdb::unique_xmalloc_ptr<char>
macro_stringify (const char *str)
{
  int len = strlen (str);
  growable_macro_buffer buffer (len);

  stringify (&buffer, str, len);
  buffer.appendc ('\0');

  return buffer.release ();
}


/* Expanding macros!  */


/* A singly-linked list of the names of the macros we are currently 
   expanding --- for detecting expansion loops.  */
struct macro_name_list {
  const char *name;
  struct macro_name_list *next;
};


/* Return non-zero if we are currently expanding the macro named NAME,
   according to LIST; otherwise, return zero.

   You know, it would be possible to get rid of all the NO_LOOP
   arguments to these functions by simply generating a new lookup
   function and baton which refuses to find the definition for a
   particular macro, and otherwise delegates the decision to another
   function/baton pair.  But that makes the linked list of excluded
   macros chained through untyped baton pointers, which will make it
   harder to debug.  :(  */
static int
currently_rescanning (struct macro_name_list *list, const char *name)
{
  for (; list; list = list->next)
    if (strcmp (name, list->name) == 0)
      return 1;

  return 0;
}


/* Gather the arguments to a macro expansion.

   NAME is the name of the macro being invoked.  (It's only used for
   printing error messages.)

   Assume that SRC is the text of the macro invocation immediately
   following the macro name.  For example, if we're processing the
   text foo(bar, baz), then NAME would be foo and SRC will be (bar,
   baz).

   If SRC doesn't start with an open paren ( token at all, return
   false, leave SRC unchanged, and don't set *ARGS_PTR to anything.

   If SRC doesn't contain a properly terminated argument list, then
   raise an error.

   For a variadic macro, NARGS holds the number of formal arguments to
   the macro.  For a GNU-style variadic macro, this should be the
   number of named arguments.  For a non-variadic macro, NARGS should
   be -1.

   Otherwise, return true and set *ARGS_PTR to a vector of macro
   buffers referring to the argument texts.  The macro buffers share
   their text with SRC, and their last_token fields are initialized.

   NOTE WELL: if SRC starts with a open paren ( token followed
   immediately by a close paren ) token (e.g., the invocation looks
   like "foo()"), we treat that as one argument, which happens to be
   the empty list of tokens.  The caller should keep in mind that such
   a sequence of tokens is a valid way to invoke one-parameter
   function-like macros, but also a valid way to invoke zero-parameter
   function-like macros.  Eeew.

   Consume the tokens from SRC; after this call, SRC contains the text
   following the invocation.  */

static bool
gather_arguments (const char *name, shared_macro_buffer *src, int nargs,
		  std::vector<shared_macro_buffer> *args_ptr)
{
  shared_macro_buffer tok;
  std::vector<shared_macro_buffer> args;

  /* Does SRC start with an opening paren token?  Read from a copy of
     SRC, so SRC itself is unaffected if we don't find an opening
     paren.  */
  {
    shared_macro_buffer temp (src->text, src->len);

    if (! get_token (&tok, &temp)
	|| tok.len != 1
	|| tok.text[0] != '(')
      return false;
  }

  /* Consume SRC's opening paren.  */
  get_token (&tok, src);

  for (;;)
    {
      shared_macro_buffer *arg;
      int depth;

      /* Initialize the next argument.  */
      args.emplace_back ();
      arg = &args.back ();
      set_token (arg, src->text, src->text);

      /* Gather the argument's tokens.  */
      depth = 0;
      for (;;)
	{
	  if (! get_token (&tok, src))
	    error (_("Malformed argument list for macro `%s'."), name);

	  /* Is tok an opening paren?  */
	  if (tok.len == 1 && tok.text[0] == '(')
	    depth++;

	  /* Is tok is a closing paren?  */
	  else if (tok.len == 1 && tok.text[0] == ')')
	    {
	      /* If it's a closing paren at the top level, then that's
		 the end of the argument list.  */
	      if (depth == 0)
		{
		  /* In the varargs case, the last argument may be
		     missing.  Add an empty argument in this case.  */
		  if (nargs != -1 && args.size () == nargs - 1)
		    {
		      args.emplace_back ();
		      arg = &args.back ();
		      set_token (arg, src->text, src->text);
		    }

		  *args_ptr = std::move (args);
		  return true;
		}

	      depth--;
	    }

	  /* If tok is a comma at top level, then that's the end of
	     the current argument.  However, if we are handling a
	     variadic macro and we are computing the last argument, we
	     want to include the comma and remaining tokens.  */
	  else if (tok.len == 1 && tok.text[0] == ',' && depth == 0
		   && (nargs == -1 || args.size () < nargs))
	    break;

	  /* Extend the current argument to enclose this token.  If
	     this is the current argument's first token, leave out any
	     leading whitespace, just for aesthetics.  */
	  if (arg->len == 0)
	    {
	      arg->text = tok.text;
	      arg->len = tok.len;
	      arg->last_token = 0;
	    }
	  else
	    {
	      arg->len = (tok.text + tok.len) - arg->text;
	      arg->last_token = tok.text - arg->text;
	    }
	}
    }
}


/* The `expand' and `substitute_args' functions both invoke `scan'
   recursively, so we need a forward declaration somewhere.  */
static void scan (growable_macro_buffer *dest,
		  shared_macro_buffer *src,
		  struct macro_name_list *no_loop,
		  const macro_scope &scope);

/* A helper function for substitute_args.
   
   ARGV is a vector of all the arguments; ARGC is the number of
   arguments.  IS_VARARGS is true if the macro being substituted is a
   varargs macro; in this case VA_ARG_NAME is the name of the
   "variable" argument.  VA_ARG_NAME is ignored if IS_VARARGS is
   false.

   If the token TOK is the name of a parameter, return the parameter's
   index.  If TOK is not an argument, return -1.  */

static int
find_parameter (const shared_macro_buffer *tok,
		int is_varargs, const shared_macro_buffer *va_arg_name,
		int argc, const char * const *argv)
{
  int i;

  if (! tok->is_identifier)
    return -1;

  for (i = 0; i < argc; ++i)
    if (tok->len == strlen (argv[i]) 
	&& !memcmp (tok->text, argv[i], tok->len))
      return i;

  if (is_varargs && tok->len == va_arg_name->len
      && ! memcmp (tok->text, va_arg_name->text, tok->len))
    return argc - 1;

  return -1;
}
 
/* Helper function for substitute_args that gets the next token and
   updates the passed-in state variables.  */

static void
get_next_token_for_substitution (shared_macro_buffer *replacement_list,
				 shared_macro_buffer *token,
				 const char **start,
				 shared_macro_buffer *lookahead,
				 const char **lookahead_start,
				 int *lookahead_valid,
				 bool *keep_going)
{
  if (!*lookahead_valid)
    *keep_going = false;
  else
    {
      *keep_going = true;
      *token = *lookahead;
      *start = *lookahead_start;
      *lookahead_start = replacement_list->text;
      *lookahead_valid = get_token (lookahead, replacement_list);
    }
}

/* Given the macro definition DEF, being invoked with the actual
   arguments given by ARGV, substitute the arguments into the
   replacement list, and store the result in DEST.

   IS_VARARGS should be true if DEF is a varargs macro.  In this case,
   VA_ARG_NAME should be the name of the "variable" argument -- either
   __VA_ARGS__ for c99-style varargs, or the final argument name, for
   GNU-style varargs.  If IS_VARARGS is false, this parameter is
   ignored.

   If it is necessary to expand macro invocations in one of the
   arguments, use LOOKUP_FUNC and LOOKUP_BATON to find the macro
   definitions, and don't expand invocations of the macros listed in
   NO_LOOP.  */

static void
substitute_args (growable_macro_buffer *dest,
		 struct macro_definition *def,
		 int is_varargs, const shared_macro_buffer *va_arg_name,
		 const std::vector<shared_macro_buffer> &argv,
		 struct macro_name_list *no_loop,
		 const macro_scope &scope)
{
  /* The token we are currently considering.  */
  shared_macro_buffer tok;
  /* The replacement list's pointer from just before TOK was lexed.  */
  const char *original_rl_start;
  /* We have a single lookahead token to handle token splicing.  */
  shared_macro_buffer lookahead;
  /* The lookahead token might not be valid.  */
  int lookahead_valid;
  /* The replacement list's pointer from just before LOOKAHEAD was
     lexed.  */
  const char *lookahead_rl_start;

  /* A macro buffer for the macro's replacement list.  */
  shared_macro_buffer replacement_list (def->replacement,
					strlen (def->replacement));

  gdb_assert (dest->len == 0);
  dest->last_token = 0;

  original_rl_start = replacement_list.text;
  if (! get_token (&tok, &replacement_list))
    return;
  lookahead_rl_start = replacement_list.text;
  lookahead_valid = get_token (&lookahead, &replacement_list);

  /* __VA_OPT__ state variable.  The states are:
     0 - nothing happening
     1 - saw __VA_OPT__
     >= 2 in __VA_OPT__, the value encodes the parenthesis depth.  */
  unsigned vaopt_state = 0;

  for (bool keep_going = true;
       keep_going;
       get_next_token_for_substitution (&replacement_list,
					&tok,
					&original_rl_start,
					&lookahead,
					&lookahead_rl_start,
					&lookahead_valid,
					&keep_going))
    {
      bool token_is_vaopt = (tok.len == 10
			     && startswith (tok.text, "__VA_OPT__"));

      if (vaopt_state > 0)
	{
	  if (token_is_vaopt)
	    error (_("__VA_OPT__ cannot appear inside __VA_OPT__"));
	  else if (tok.len == 1 && tok.text[0] == '(')
	    {
	      ++vaopt_state;
	      /* We just entered __VA_OPT__, so don't emit this
		 token.  */
	      continue;
	    }
	  else if (vaopt_state == 1)
	    error (_("__VA_OPT__ must be followed by an open parenthesis"));
	  else if (tok.len == 1 && tok.text[0] == ')')
	    {
	      --vaopt_state;
	      if (vaopt_state == 1)
		{
		  /* Done with __VA_OPT__.  */
		  vaopt_state = 0;
		  /* Don't emit.  */
		  continue;
		}
	    }

	  /* If __VA_ARGS__ is empty, then drop the contents of
	     __VA_OPT__.  */
	  if (argv.back ().len == 0)
	    continue;
	}
      else if (token_is_vaopt)
	{
	  if (!is_varargs)
	    error (_("__VA_OPT__ is only valid in a variadic macro"));
	  vaopt_state = 1;
	  /* Don't emit this token.  */
	  continue;
	}

      /* Just for aesthetics.  If we skipped some whitespace, copy
	 that to DEST.  */
      if (tok.text > original_rl_start)
	{
	  dest->appendmem (original_rl_start, tok.text - original_rl_start);
	  dest->last_token = dest->len;
	}

      /* Is this token the stringification operator?  */
      if (tok.len == 1
	  && tok.text[0] == '#')
	{
	  int arg;

	  if (!lookahead_valid)
	    error (_("Stringification operator requires an argument."));

	  arg = find_parameter (&lookahead, is_varargs, va_arg_name,
				def->argc, def->argv);
	  if (arg == -1)
	    error (_("Argument to stringification operator must name "
		     "a macro parameter."));

	  stringify (dest, argv[arg].text, argv[arg].len);

	  /* Read one token and let the loop iteration code handle the
	     rest.  */
	  lookahead_rl_start = replacement_list.text;
	  lookahead_valid = get_token (&lookahead, &replacement_list);
	}
      /* Is this token the splicing operator?  */
      else if (tok.len == 2
	       && tok.text[0] == '#'
	       && tok.text[1] == '#')
	error (_("Stray splicing operator"));
      /* Is the next token the splicing operator?  */
      else if (lookahead_valid
	       && lookahead.len == 2
	       && lookahead.text[0] == '#'
	       && lookahead.text[1] == '#')
	{
	  int finished = 0;
	  int prev_was_comma = 0;

	  /* Note that GCC warns if the result of splicing is not a
	     token.  In the debugger there doesn't seem to be much
	     benefit from doing this.  */

	  /* Insert the first token.  */
	  if (tok.len == 1 && tok.text[0] == ',')
	    prev_was_comma = 1;
	  else
	    {
	      int arg = find_parameter (&tok, is_varargs, va_arg_name,
					def->argc, def->argv);

	      if (arg != -1)
		dest->appendmem (argv[arg].text, argv[arg].len);
	      else
		dest->appendmem (tok.text, tok.len);
	    }

	  /* Apply a possible sequence of ## operators.  */
	  for (;;)
	    {
	      if (! get_token (&tok, &replacement_list))
		error (_("Splicing operator at end of macro"));

	      /* Handle a comma before a ##.  If we are handling
		 varargs, and the token on the right hand side is the
		 varargs marker, and the final argument is empty or
		 missing, then drop the comma.  This is a GNU
		 extension.  There is one ambiguous case here,
		 involving pedantic behavior with an empty argument,
		 but we settle that in favor of GNU-style (GCC uses an
		 option).  If we aren't dealing with varargs, we
		 simply insert the comma.  */
	      if (prev_was_comma)
		{
		  if (! (is_varargs
			 && tok.len == va_arg_name->len
			 && !memcmp (tok.text, va_arg_name->text, tok.len)
			 && argv.back ().len == 0))
		    dest->appendmem (",", 1);
		  prev_was_comma = 0;
		}

	      /* Insert the token.  If it is a parameter, insert the
		 argument.  If it is a comma, treat it specially.  */
	      if (tok.len == 1 && tok.text[0] == ',')
		prev_was_comma = 1;
	      else
		{
		  int arg = find_parameter (&tok, is_varargs, va_arg_name,
					    def->argc, def->argv);

		  if (arg != -1)
		    dest->appendmem (argv[arg].text, argv[arg].len);
		  else
		    dest->appendmem (tok.text, tok.len);
		}

	      /* Now read another token.  If it is another splice, we
		 loop.  */
	      original_rl_start = replacement_list.text;
	      if (! get_token (&tok, &replacement_list))
		{
		  finished = 1;
		  break;
		}

	      if (! (tok.len == 2
		     && tok.text[0] == '#'
		     && tok.text[1] == '#'))
		break;
	    }

	  if (prev_was_comma)
	    {
	      /* We saw a comma.  Insert it now.  */
	      dest->appendmem (",", 1);
	    }

	  dest->last_token = dest->len;
	  if (finished)
	    lookahead_valid = 0;
	  else
	    {
	      /* Set up for the loop iterator.  */
	      lookahead = tok;
	      lookahead_rl_start = original_rl_start;
	      lookahead_valid = 1;
	    }
	}
      else
	{
	  /* Is this token an identifier?  */
	  int substituted = 0;
	  int arg = find_parameter (&tok, is_varargs, va_arg_name,
				    def->argc, def->argv);

	  if (arg != -1)
	    {
	      /* Expand any macro invocations in the argument text,
		 and append the result to dest.  Remember that scan
		 mutates its source, so we need to scan a new buffer
		 referring to the argument's text, not the argument
		 itself.  */
	      shared_macro_buffer arg_src (argv[arg].text, argv[arg].len);
	      scan (dest, &arg_src, no_loop, scope);
	      substituted = 1;
	    }

	  /* If it wasn't a parameter, then just copy it across.  */
	  if (! substituted)
	    append_tokens_without_splicing (dest, &tok);
	}
    }

  if (vaopt_state > 0)
    error (_("Unterminated __VA_OPT__"));
}


/* Expand a call to a macro named ID, whose definition is DEF.  Append
   its expansion to DEST.  SRC is the input text following the ID
   token.  We are currently rescanning the expansions of the macros
   named in NO_LOOP; don't re-expand them.  Use LOOKUP_FUNC and
   LOOKUP_BATON to find definitions for any nested macro references.

   Return 1 if we decided to expand it, zero otherwise.  (If it's a
   function-like macro name that isn't followed by an argument list,
   we don't expand it.)  If we return zero, leave SRC unchanged.  */
static int
expand (const char *id,
	struct macro_definition *def,
	growable_macro_buffer *dest,
	shared_macro_buffer *src,
	struct macro_name_list *no_loop,
	const macro_scope &scope)
{
  struct macro_name_list new_no_loop;

  /* Create a new node to be added to the front of the no-expand list.
     This list is appropriate for re-scanning replacement lists, but
     it is *not* appropriate for scanning macro arguments; invocations
     of the macro whose arguments we are gathering *do* get expanded
     there.  */
  new_no_loop.name = id;
  new_no_loop.next = no_loop;

  /* What kind of macro are we expanding?  */
  if (def->kind == macro_object_like)
    {
      shared_macro_buffer replacement_list (def->replacement,
					    strlen (def->replacement));

      scan (dest, &replacement_list, &new_no_loop, scope);
      return 1;
    }
  else if (def->kind == macro_function_like)
    {
      shared_macro_buffer va_arg_name;
      int is_varargs = 0;

      if (def->argc >= 1)
	{
	  if (strcmp (def->argv[def->argc - 1], "...") == 0)
	    {
	      /* In C99-style varargs, substitution is done using
		 __VA_ARGS__.  */
	      va_arg_name.set_shared ("__VA_ARGS__", strlen ("__VA_ARGS__"));
	      is_varargs = 1;
	    }
	  else
	    {
	      int len = strlen (def->argv[def->argc - 1]);

	      if (len > 3
		  && strcmp (def->argv[def->argc - 1] + len - 3, "...") == 0)
		{
		  /* In GNU-style varargs, the name of the
		     substitution parameter is the name of the formal
		     argument without the "...".  */
		  va_arg_name.set_shared (def->argv[def->argc - 1], len - 3);
		  is_varargs = 1;
		}
	    }
	}

      std::vector<shared_macro_buffer> argv;
      /* If we couldn't find any argument list, then we don't expand
	 this macro.  */
      if (!gather_arguments (id, src, is_varargs ? def->argc : -1,
			     &argv))
	return 0;

      /* Check that we're passing an acceptable number of arguments for
	 this macro.  */
      if (argv.size () != def->argc)
	{
	  if (is_varargs && argv.size () >= def->argc - 1)
	    {
	      /* Ok.  */
	    }
	  /* Remember that a sequence of tokens like "foo()" is a
	     valid invocation of a macro expecting either zero or one
	     arguments.  */
	  else if (! (argv.size () == 1
		      && argv[0].len == 0
		      && def->argc == 0))
	    error (_("Wrong number of arguments to macro `%s' "
		   "(expected %d, got %d)."),
		   id, def->argc, int (argv.size ()));
	}

      /* Note that we don't expand macro invocations in the arguments
	 yet --- we let subst_args take care of that.  Parameters that
	 appear as operands of the stringifying operator "#" or the
	 splicing operator "##" don't get macro references expanded,
	 so we can't really tell whether it's appropriate to macro-
	 expand an argument until we see how it's being used.  */
      growable_macro_buffer substituted (0);
      substitute_args (&substituted, def, is_varargs, &va_arg_name,
		       argv, no_loop, scope);

      /* Now `substituted' is the macro's replacement list, with all
	 argument values substituted into it properly.	Re-scan it for
	 macro references, but don't expand invocations of this macro.

	 We create a new buffer, `substituted_src', which points into
	 `substituted', and scan that.	We can't scan `substituted'
	 itself, since the tokenization process moves the buffer's
	 text pointer around, and we still need to be able to find
	 `substituted's original text buffer after scanning it so we
	 can free it.  */
      shared_macro_buffer substituted_src (substituted.text, substituted.len);
      scan (dest, &substituted_src, &new_no_loop, scope);

      return 1;
    }
  else
    internal_error (_("bad macro definition kind"));
}


/* If the single token in SRC_FIRST followed by the tokens in SRC_REST
   constitute a macro invocation not forbidden in NO_LOOP, append its
   expansion to DEST and return non-zero.  Otherwise, return zero, and
   leave DEST unchanged.

   SRC_FIRST must be a string built by get_token.  */
static int
maybe_expand (growable_macro_buffer *dest,
	      shared_macro_buffer *src_first,
	      shared_macro_buffer *src_rest,
	      struct macro_name_list *no_loop,
	      const macro_scope &scope)
{
  /* Is this token an identifier?  */
  if (src_first->is_identifier)
    {
      /* Make a null-terminated copy of it, since that's what our
	 lookup function expects.  */
      std::string id (src_first->text, src_first->len);

      /* If we're currently re-scanning the result of expanding
	 this macro, don't expand it again.  */
      if (! currently_rescanning (no_loop, id.c_str ()))
	{
	  /* Does this identifier have a macro definition in scope?  */
	  macro_definition *def = standard_macro_lookup (id.c_str (), scope);

	  if (def && expand (id.c_str (), def, dest, src_rest, no_loop, scope))
	    return 1;
	}
    }

  return 0;
}


/* Expand macro references in SRC, appending the results to DEST.
   Assume we are re-scanning the result of expanding the macros named
   in NO_LOOP, and don't try to re-expand references to them.  */

static void
scan (growable_macro_buffer *dest,
      shared_macro_buffer *src,
      struct macro_name_list *no_loop,
      const macro_scope &scope)
{

  for (;;)
    {
      shared_macro_buffer tok;
      const char *original_src_start = src->text;

      /* Find the next token in SRC.  */
      if (! get_token (&tok, src))
	break;

      /* Just for aesthetics.  If we skipped some whitespace, copy
	 that to DEST.  */
      if (tok.text > original_src_start)
	{
	  dest->appendmem (original_src_start, tok.text - original_src_start);
	  dest->last_token = dest->len;
	}

      if (! maybe_expand (dest, &tok, src, no_loop, scope))
	/* We didn't end up expanding tok as a macro reference, so
	   simply append it to dest.  */
	append_tokens_without_splicing (dest, &tok);
    }

  /* Just for aesthetics.  If there was any trailing whitespace in
     src, copy it to dest.  */
  if (src->len)
    {
      dest->appendmem (src->text, src->len);
      dest->last_token = dest->len;
    }
}


gdb::unique_xmalloc_ptr<char>
macro_expand (const char *source, const macro_scope &scope)
{
  shared_macro_buffer src (source, strlen (source));

  growable_macro_buffer dest (0);
  dest.last_token = 0;

  scan (&dest, &src, 0, scope);

  dest.appendc ('\0');

  return dest.release ();
}


gdb::unique_xmalloc_ptr<char>
macro_expand_once (const char *source, const macro_scope &scope)
{
  error (_("Expand-once not implemented yet."));
}

gdb::unique_xmalloc_ptr<char>
macro_expand_next (const char **lexptr, const macro_scope &scope)
{
  shared_macro_buffer tok;

  /* Set up SRC to refer to the input text, pointed to by *lexptr.  */
  shared_macro_buffer src (*lexptr, strlen (*lexptr));

  /* Set up DEST to receive the expansion, if there is one.  */
  growable_macro_buffer dest (0);
  dest.last_token = 0;

  /* Get the text's first preprocessing token.  */
  if (! get_token (&tok, &src))
    return nullptr;

  /* If it's a macro invocation, expand it.  */
  if (maybe_expand (&dest, &tok, &src, 0, scope))
    {
      /* It was a macro invocation!  Package up the expansion as a
	 null-terminated string and return it.  Set *lexptr to the
	 start of the next token in the input.  */
      dest.appendc ('\0');
      *lexptr = src.text;
      return dest.release ();
    }
  else
    {
      /* It wasn't a macro invocation.  */
      return nullptr;
    }
}
