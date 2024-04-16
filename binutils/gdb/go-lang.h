/* Go language support definitions for GDB, the GNU debugger.

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

#if !defined (GO_LANG_H)
#define GO_LANG_H 1

struct type_print_options;

#include "language.h"
#include "gdbtypes.h"
#include "symtab.h"
#include "value.h"

struct parser_state;

struct builtin_go_type
{
  struct type *builtin_void = nullptr;
  struct type *builtin_char = nullptr;
  struct type *builtin_bool = nullptr;
  struct type *builtin_int = nullptr;
  struct type *builtin_uint = nullptr;
  struct type *builtin_uintptr = nullptr;
  struct type *builtin_int8 = nullptr;
  struct type *builtin_int16 = nullptr;
  struct type *builtin_int32 = nullptr;
  struct type *builtin_int64 = nullptr;
  struct type *builtin_uint8 = nullptr;
  struct type *builtin_uint16 = nullptr;
  struct type *builtin_uint32 = nullptr;
  struct type *builtin_uint64 = nullptr;
  struct type *builtin_float32 = nullptr;
  struct type *builtin_float64 = nullptr;
  struct type *builtin_complex64 = nullptr;
  struct type *builtin_complex128 = nullptr;
};

enum go_type
{
  GO_TYPE_NONE, /* Not a Go object.  */
  GO_TYPE_STRING
};

/* Defined in go-lang.c.  */

extern const char *go_main_name (void);

extern enum go_type go_classify_struct_type (struct type *type);

/* Given a symbol, return its package or nullptr if unknown.  */
extern gdb::unique_xmalloc_ptr<char> go_symbol_package_name
     (const struct symbol *sym);

/* Return the package that BLOCK is in, or nullptr if there isn't
   one.  */
extern gdb::unique_xmalloc_ptr<char> go_block_package_name
     (const struct block *block);

extern const struct builtin_go_type *builtin_go_type (struct gdbarch *);

/* Class representing the Go language.  */

class go_language : public language_defn
{
public:
  go_language ()
    : language_defn (language_go)
  { /* Nothing.  */ }

  /* See language.h.  */

  const char *name () const override
  { return "go"; }

  /* See language.h.  */

  const char *natural_name () const override
  { return "Go"; }

  /* See language.h.  */

  void language_arch_info (struct gdbarch *gdbarch,
			   struct language_arch_info *lai) const override;

  /* See language.h.  */

  bool sniff_from_mangled_name
       (const char *mangled, gdb::unique_xmalloc_ptr<char> *demangled)
       const override
  {
    *demangled = demangle_symbol (mangled, 0);
    return *demangled != NULL;
  }

  /* See language.h.  */

  gdb::unique_xmalloc_ptr<char> demangle_symbol (const char *mangled,
						 int options) const override;

  /* See language.h.  */

  void print_type (struct type *type, const char *varstring,
		   struct ui_file *stream, int show, int level,
		   const struct type_print_options *flags) const override;

  /* See language.h.  */

  void value_print_inner
	(struct value *val, struct ui_file *stream, int recurse,
	 const struct value_print_options *options) const override;

  /* See language.h.  */

  int parser (struct parser_state *ps) const override;

  /* See language.h.  */

  bool is_string_type_p (struct type *type) const override
  {
    type = check_typedef (type);
    return (type->code () == TYPE_CODE_STRUCT
	    && go_classify_struct_type (type) == GO_TYPE_STRING);
  }

  /* See language.h.  */

  bool store_sym_names_in_linkage_form_p () const override
  { return true; }
};

#endif /* !defined (GO_LANG_H) */
