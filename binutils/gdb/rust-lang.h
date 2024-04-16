/* Rust language support definitions for GDB, the GNU debugger.

   Copyright (C) 2016-2024 Free Software Foundation, Inc.

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

#ifndef RUST_LANG_H
#define RUST_LANG_H

#include "demangle.h"
#include "language.h"
#include "value.h"
#include "c-lang.h"

struct parser_state;
struct type;

/* Return true if TYPE is a tuple type; otherwise false.  */
extern bool rust_tuple_type_p (struct type *type);

/* Return true if TYPE is a tuple struct type; otherwise false.  */
extern bool rust_tuple_struct_type_p (struct type *type);

/* Return true if TYPE is a slice type, otherwise false.  */
extern bool rust_slice_type_p (const struct type *type);

/* Given a block, find the name of the block's crate. Returns an empty
   stringif no crate name can be found.  */
extern std::string rust_crate_for_block (const struct block *block);

/* Returns the last segment of a Rust path like foo::bar::baz.  Will
   not handle cases where the last segment contains generics.  */

extern const char *rust_last_path_segment (const char *path);

/* Create a new slice type.  NAME is the name of the type.  ELT_TYPE
   is the type of the elements of the slice.  USIZE_TYPE is the Rust
   "usize" type to use.  The new type is allocated whereever ELT_TYPE
   is allocated.  */
extern struct type *rust_slice_type (const char *name, struct type *elt_type,
				     struct type *usize_type);

/* Return a new array that holds the contents of the given slice,
   VAL.  */
extern struct value *rust_slice_to_array (struct value *val);

/* Class representing the Rust language.  */

class rust_language : public language_defn
{
public:
  rust_language ()
    : language_defn (language_rust)
  { /* Nothing.  */ }

  /* See language.h.  */

  const char *name () const override
  { return "rust"; }

  /* See language.h.  */

  const char *natural_name () const override
  { return "Rust"; }

  /* See language.h.  */

  const char *get_digit_separator () const override
  { return "_"; }

  /* See language.h.  */

  const std::vector<const char *> &filename_extensions () const override
  {
    static const std::vector<const char *> extensions = { ".rs" };
    return extensions;
  }

  /* See language.h.  */

  void language_arch_info (struct gdbarch *gdbarch,
			   struct language_arch_info *lai) const override;

  /* See language.h.  */

  bool sniff_from_mangled_name
       (const char *mangled, gdb::unique_xmalloc_ptr<char> *demangled)
       const override
  {
    demangled->reset (rust_demangle (mangled, 0));
    return *demangled != NULL;
  }

  /* See language.h.  */

  gdb::unique_xmalloc_ptr<char> demangle_symbol (const char *mangled,
						 int options) const override
  {
    return gdb::unique_xmalloc_ptr<char> (rust_demangle (mangled, options));
  }

  /* See language.h.  */

  bool can_print_type_offsets () const override
  {
    return true;
  }

  /* See language.h.  */

  void print_type (struct type *type, const char *varstring,
		   struct ui_file *stream, int show, int level,
		   const struct type_print_options *flags) const override;

  /* See language.h.  */

  gdb::unique_xmalloc_ptr<char> watch_location_expression
	(struct type *type, CORE_ADDR addr) const override
  {
    type = check_typedef (check_typedef (type)->target_type ());
    std::string name = type_to_string (type);
    return xstrprintf ("*(%s as *mut %s)", core_addr_to_string (addr),
		       name.c_str ());
  }

  /* See language.h.  */

  void value_print_inner
	(struct value *val, struct ui_file *stream, int recurse,
	 const struct value_print_options *options) const override;

  /* See language.h.  */

  void value_print (struct value *val, struct ui_file *stream,
		    const struct value_print_options *options) const override;

  /* See language.h.  */

  struct block_symbol lookup_symbol_nonlocal
	(const char *name, const struct block *block,
	 const domain_enum domain) const override;

  /* See language.h.  */

  int parser (struct parser_state *ps) const override;

  /* See language.h.  */

  void emitchar (int ch, struct type *chtype,
		 struct ui_file *stream, int quoter) const override;

  /* See language.h.  */

  void printchar (int ch, struct type *chtype,
		  struct ui_file *stream) const override
  {
    gdb_puts ("'", stream);
    emitchar (ch, chtype, stream, '\'');
    gdb_puts ("'", stream);
  }

  /* See language.h.  */

  void printstr (struct ui_file *stream, struct type *elttype,
		 const gdb_byte *string, unsigned int length,
		 const char *encoding, int force_ellipses,
		 const struct value_print_options *options) const override;

  /* See language.h.  */

  void print_typedef (struct type *type, struct symbol *new_symbol,
		      struct ui_file *stream) const override
  {
    type = check_typedef (type);
    gdb_printf (stream, "type %s = ", new_symbol->print_name ());
    type_print (type, "", stream, 0);
    gdb_printf (stream, ";");
  }

  /* See language.h.  */

  bool is_string_type_p (struct type *type) const override;

  /* See language.h.  */

  bool is_array_like (struct type *type) const override
  { return rust_slice_type_p (type); }

  /* See language.h.  */

  struct value *to_array (struct value *val) const override
  { return rust_slice_to_array (val); }

  /* See language.h.  */

  bool range_checking_on_by_default () const override
  { return true; }

private:

  /* Helper for value_print_inner, arguments are as for that function.
     Prints structs and untagged unions.  */

  void val_print_struct (struct value *val, struct ui_file *stream,
			 int recurse,
			 const struct value_print_options *options) const;

  /* Helper for value_print_inner, arguments are as for that function.
     Prints discriminated unions (Rust enums).  */

  void print_enum (struct value *val, struct ui_file *stream, int recurse,
		   const struct value_print_options *options) const;
};

#endif /* RUST_LANG_H */
