/* Portable <regex.h>.
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

#ifndef GDB_REGEX_H
#define GDB_REGEX_H 1

# include "xregex.h"

/* A compiled regex.  This is mainly a wrapper around regex_t.  The
   the constructor throws on regcomp error and the destructor is
   responsible for calling regfree.  The former means that it's not
   possible to create an instance of compiled_regex that isn't
   compiled, hence the name.  */
class compiled_regex
{
public:
  /* Compile a regexp and throw an exception on error, including
     MESSAGE.  REGEX and MESSAGE must not be NULL.  */
  compiled_regex (const char *regex, int cflags,
		  const char *message)
    ATTRIBUTE_NONNULL (2) ATTRIBUTE_NONNULL (4);

  ~compiled_regex ();

  DISABLE_COPY_AND_ASSIGN (compiled_regex);

  /* Wrapper around ::regexec.  */
  int exec (const char *string,
	    size_t nmatch, regmatch_t pmatch[],
	    int eflags) const;

  /* Wrapper around ::re_search.  (Not const because re_search's
     regex_t parameter isn't either.)  */
  int search (const char *string, int size, int startpos,
	      int range, struct re_registers *regs);

private:
  /* The compiled pattern.  */
  regex_t m_pattern;
};

#endif /* not GDB_REGEX_H */
