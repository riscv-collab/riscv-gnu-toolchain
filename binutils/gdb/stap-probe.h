/* SystemTap probe support for GDB.

   Copyright (C) 2012-2024 Free Software Foundation, Inc.

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

#if !defined (STAP_PROBE_H)
#define STAP_PROBE_H 1

#include "parser-defs.h"

/* Structure which holds information about the parsing process of one probe's
   argument.  */

struct stap_parse_info
{
  stap_parse_info (const char *arg_, struct type *arg_type_,
		   const struct language_defn *lang,
		   struct gdbarch *gdbarch)
    : arg (arg_),
      pstate (lang, gdbarch),
      saved_arg (arg_),
      arg_type (arg_type_),
      gdbarch (gdbarch),
      inside_paren_p (0)
  {
  }

  DISABLE_COPY_AND_ASSIGN (stap_parse_info);

  /* The probe's argument in a string format.  */
  const char *arg;

  /* The parser state to be used when generating the expression.  */
  struct expr_builder pstate;

  /* A pointer to the full chain of arguments.  This is useful for printing
     error messages.  The parser functions should not modify this argument
     directly; instead, they should use the ARG pointer above.  */
  const char *saved_arg;

  /* The expected argument type (bitness), as defined in the probe's
     argument.  For instance, if the argument begins with `-8@', it means
     the bitness is 64-bit signed.  In this case, ARG_TYPE would represent
     the type `int64_t'.  */
  struct type *arg_type;

  /* A pointer to the current gdbarch.  */
  struct gdbarch *gdbarch;

  /* Greater than zero if we are inside a parenthesized expression.  Useful
     for knowing when to skip spaces or not.  */
  int inside_paren_p;
};

#endif /* !defined (STAP_PROBE_H) */
