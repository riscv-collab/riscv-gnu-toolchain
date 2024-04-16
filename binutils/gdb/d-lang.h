/* D language support definitions for GDB, the GNU debugger.

   Copyright (C) 2005-2024 Free Software Foundation, Inc.

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

#if !defined (D_LANG_H)
#define D_LANG_H 1

#include "symtab.h"

/* Language specific builtin types for D.  Any additional types added
   should be kept in sync with enum d_primitive_types, where these
   types are documented.  */

struct builtin_d_type
{
  struct type *builtin_void = nullptr;
  struct type *builtin_bool = nullptr;
  struct type *builtin_byte = nullptr;
  struct type *builtin_ubyte = nullptr;
  struct type *builtin_short = nullptr;
  struct type *builtin_ushort = nullptr;
  struct type *builtin_int = nullptr;
  struct type *builtin_uint = nullptr;
  struct type *builtin_long = nullptr;
  struct type *builtin_ulong = nullptr;
  struct type *builtin_cent = nullptr;
  struct type *builtin_ucent = nullptr;
  struct type *builtin_float = nullptr;
  struct type *builtin_double = nullptr;
  struct type *builtin_real = nullptr;
  struct type *builtin_ifloat = nullptr;
  struct type *builtin_idouble = nullptr;
  struct type *builtin_ireal = nullptr;
  struct type *builtin_cfloat = nullptr;
  struct type *builtin_cdouble = nullptr;
  struct type *builtin_creal = nullptr;
  struct type *builtin_char = nullptr;
  struct type *builtin_wchar = nullptr;
  struct type *builtin_dchar = nullptr;
};

/* Defined in d-exp.y.  */

extern int d_parse (struct parser_state *);

/* Defined in d-lang.c  */

extern const char *d_main_name (void);

extern gdb::unique_xmalloc_ptr<char> d_demangle (const char *mangled,
						 int options);

extern const struct builtin_d_type *builtin_d_type (struct gdbarch *);

/* Defined in d-namespace.c  */

extern struct block_symbol d_lookup_symbol_nonlocal (const struct language_defn *,
						     const char *,
						     const struct block *,
						     const domain_enum);

extern struct block_symbol d_lookup_nested_symbol (struct type *, const char *,
						   const struct block *);

/* Implement la_value_print_inner for D.  */

extern void d_value_print_inner (struct value *val,
				 struct ui_file *stream, int recurse,
				 const struct value_print_options *options);

#endif /* !defined (D_LANG_H) */
