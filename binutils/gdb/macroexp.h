/* Interface to C preprocessor macro expansion for GDB.
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


#ifndef MACROEXP_H
#define MACROEXP_H

struct macro_scope;

/* Expand any preprocessor macros in SOURCE (a null-terminated string), and
   return the expanded text.

   Use SCOPE to find identifiers' preprocessor definitions.

   The result is a null-terminated string.  */
gdb::unique_xmalloc_ptr<char> macro_expand (const char *source,
					    const macro_scope &scope);

/* Expand all preprocessor macro references that appear explicitly in SOURCE
   (a null-terminated string), but do not expand any new macro references
   introduced by that first level of expansion.

   Use SCOPE to find identifiers' preprocessor definitions.

   The result is a null-terminated string.  */
gdb::unique_xmalloc_ptr<char> macro_expand_once (const char *source,
						 const macro_scope &scope);

/* If the null-terminated string pointed to by *LEXPTR begins with a
   macro invocation, return the result of expanding that invocation as
   a null-terminated string, and set *LEXPTR to the next character
   after the invocation.  The result is completely expanded; it
   contains no further macro invocations.

   Otherwise, if *LEXPTR does not start with a macro invocation,
   return nullptr, and leave *LEXPTR unchanged.

   Use SCOPE to find macro definitions.

   If this function returns a string, the caller is responsible for
   freeing it, using xfree.

   We need this expand-one-token-at-a-time interface in order to
   accommodate GDB's C expression parser, which may not consume the
   entire string.  When the user enters a command like

      (gdb) break *func+20 if x == 5

   the parser is expected to consume `func+20', and then stop when it
   sees the "if".  But of course, "if" appearing in a character string
   or as part of a larger identifier doesn't count.  So you pretty
   much have to do tokenization to find the end of the string that
   needs to be macro-expanded.  Our C/C++ tokenizer isn't really
   designed to be called by anything but the yacc parser engine.  */
gdb::unique_xmalloc_ptr<char> macro_expand_next (const char **lexptr,
						 const macro_scope &scope);

/* Functions to classify characters according to cpp rules.  */

int macro_is_whitespace (int c);
int macro_is_identifier_nondigit (int c);
int macro_is_digit (int c);


/* Stringify STR according to C rules and return a null-terminated string.  */
gdb::unique_xmalloc_ptr<char> macro_stringify (const char *str);

#endif /* MACROEXP_H */
