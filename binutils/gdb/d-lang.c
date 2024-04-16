/* D language support routines for GDB, the GNU debugger.

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

#include "defs.h"
#include "symtab.h"
#include "language.h"
#include "varobj.h"
#include "d-lang.h"
#include "c-lang.h"
#include "demangle.h"
#include "cp-support.h"
#include "gdbarch.h"
#include "parser-defs.h"

/* The name of the symbol to use to get the name of the main subprogram.  */
static const char D_MAIN[] = "D main";

/* Function returning the special symbol name used by D for the main
   procedure in the main program if it is found in minimal symbol list.
   This function tries to find minimal symbols so that it finds them even
   if the program was compiled without debugging information.  */

const char *
d_main_name (void)
{
  struct bound_minimal_symbol msym;

  msym = lookup_minimal_symbol (D_MAIN, NULL, NULL);
  if (msym.minsym != NULL)
    return D_MAIN;

  /* No known entry procedure found, the main program is probably not D.  */
  return NULL;
}

/* Implements the la_demangle language_defn routine for language D.  */

gdb::unique_xmalloc_ptr<char>
d_demangle (const char *symbol, int options)
{
  return gdb_demangle (symbol, options | DMGL_DLANG);
}

/* Class representing the D language.  */

class d_language : public language_defn
{
public:
  d_language ()
    : language_defn (language_d)
  { /* Nothing.  */ }

  /* See language.h.  */

  const char *name () const override
  { return "d"; }

  /* See language.h.  */

  const char *natural_name () const override
  { return "D"; }

  /* See language.h.  */

  const std::vector<const char *> &filename_extensions () const override
  {
    static const std::vector<const char *> extensions = { ".d" };
    return extensions;
  }

  /* See language.h.  */
  void language_arch_info (struct gdbarch *gdbarch,
			   struct language_arch_info *lai) const override
  {
    const struct builtin_d_type *builtin = builtin_d_type (gdbarch);

    /* Helper function to allow shorter lines below.  */
    auto add  = [&] (struct type * t)
    {
      lai->add_primitive_type (t);
    };

    add (builtin->builtin_void);
    add (builtin->builtin_bool);
    add (builtin->builtin_byte);
    add (builtin->builtin_ubyte);
    add (builtin->builtin_short);
    add (builtin->builtin_ushort);
    add (builtin->builtin_int);
    add (builtin->builtin_uint);
    add (builtin->builtin_long);
    add (builtin->builtin_ulong);
    add (builtin->builtin_cent);
    add (builtin->builtin_ucent);
    add (builtin->builtin_float);
    add (builtin->builtin_double);
    add (builtin->builtin_real);
    add (builtin->builtin_ifloat);
    add (builtin->builtin_idouble);
    add (builtin->builtin_ireal);
    add (builtin->builtin_cfloat);
    add (builtin->builtin_cdouble);
    add (builtin->builtin_creal);
    add (builtin->builtin_char);
    add (builtin->builtin_wchar);
    add (builtin->builtin_dchar);

    lai->set_string_char_type (builtin->builtin_char);
    lai->set_bool_type (builtin->builtin_bool, "bool");
  }

  /* See language.h.  */
  bool sniff_from_mangled_name
       (const char *mangled,
	gdb::unique_xmalloc_ptr<char> *demangled) const override
  {
    *demangled = d_demangle (mangled, 0);
    return *demangled != NULL;
  }

  /* See language.h.  */

  gdb::unique_xmalloc_ptr<char> demangle_symbol (const char *mangled,
						 int options) const override
  {
    return d_demangle (mangled, options);
  }

  /* See language.h.  */

  bool can_print_type_offsets () const override
  {
    return true;
  }

  /* See language.h.  */

  void print_type (struct type *type, const char *varstring,
		   struct ui_file *stream, int show, int level,
		   const struct type_print_options *flags) const override
  {
    c_print_type (type, varstring, stream, show, level, la_language, flags);
  }

  /* See language.h.  */

  void value_print_inner
	(struct value *val, struct ui_file *stream, int recurse,
	 const struct value_print_options *options) const override
  {
    return d_value_print_inner (val, stream, recurse, options);
  }

  /* See language.h.  */

  struct block_symbol lookup_symbol_nonlocal
	(const char *name, const struct block *block,
	 const domain_enum domain) const override
  {
    return d_lookup_symbol_nonlocal (this, name, block, domain);
  }

  /* See language.h.  */

  int parser (struct parser_state *ps) const override
  {
    return d_parse (ps);
  }

  /* See language.h.  */

  const char *name_of_this () const override
  { return "this"; }
};

/* Single instance of the D language class.  */

static d_language d_language_defn;

/* Build all D language types for the specified architecture.  */

static struct builtin_d_type *
build_d_types (struct gdbarch *gdbarch)
{
  struct builtin_d_type *builtin_d_type = new struct builtin_d_type;

  /* Basic types.  */
  type_allocator alloc (gdbarch);
  builtin_d_type->builtin_void = builtin_type (gdbarch)->builtin_void;
  builtin_d_type->builtin_bool
    = init_boolean_type (alloc, 8, 1, "bool");
  builtin_d_type->builtin_byte
    = init_integer_type (alloc, 8, 0, "byte");
  builtin_d_type->builtin_ubyte
    = init_integer_type (alloc, 8, 1, "ubyte");
  builtin_d_type->builtin_short
    = init_integer_type (alloc, 16, 0, "short");
  builtin_d_type->builtin_ushort
    = init_integer_type (alloc, 16, 1, "ushort");
  builtin_d_type->builtin_int
    = init_integer_type (alloc, 32, 0, "int");
  builtin_d_type->builtin_uint
    = init_integer_type (alloc, 32, 1, "uint");
  builtin_d_type->builtin_long
    = init_integer_type (alloc, 64, 0, "long");
  builtin_d_type->builtin_ulong
    = init_integer_type (alloc, 64, 1, "ulong");
  builtin_d_type->builtin_cent
    = init_integer_type (alloc, 128, 0, "cent");
  builtin_d_type->builtin_ucent
    = init_integer_type (alloc, 128, 1, "ucent");
  builtin_d_type->builtin_float
    = init_float_type (alloc, gdbarch_float_bit (gdbarch),
		       "float", gdbarch_float_format (gdbarch));
  builtin_d_type->builtin_double
    = init_float_type (alloc, gdbarch_double_bit (gdbarch),
		       "double", gdbarch_double_format (gdbarch));
  builtin_d_type->builtin_real
    = init_float_type (alloc, gdbarch_long_double_bit (gdbarch),
		       "real", gdbarch_long_double_format (gdbarch));

  builtin_d_type->builtin_byte->set_instance_flags
    (builtin_d_type->builtin_byte->instance_flags ()
     | TYPE_INSTANCE_FLAG_NOTTEXT);

  builtin_d_type->builtin_ubyte->set_instance_flags
    (builtin_d_type->builtin_ubyte->instance_flags ()
     | TYPE_INSTANCE_FLAG_NOTTEXT);

  /* Imaginary and complex types.  */
  builtin_d_type->builtin_ifloat
    = init_float_type (alloc, gdbarch_float_bit (gdbarch),
		       "ifloat", gdbarch_float_format (gdbarch));
  builtin_d_type->builtin_idouble
    = init_float_type (alloc, gdbarch_double_bit (gdbarch),
		       "idouble", gdbarch_double_format (gdbarch));
  builtin_d_type->builtin_ireal
    = init_float_type (alloc, gdbarch_long_double_bit (gdbarch),
		       "ireal", gdbarch_long_double_format (gdbarch));
  builtin_d_type->builtin_cfloat
    = init_complex_type ("cfloat", builtin_d_type->builtin_float);
  builtin_d_type->builtin_cdouble
    = init_complex_type ("cdouble", builtin_d_type->builtin_double);
  builtin_d_type->builtin_creal
    = init_complex_type ("creal", builtin_d_type->builtin_real);

  /* Character types.  */
  builtin_d_type->builtin_char
    = init_character_type (alloc, 8, 1, "char");
  builtin_d_type->builtin_wchar
    = init_character_type (alloc, 16, 1, "wchar");
  builtin_d_type->builtin_dchar
    = init_character_type (alloc, 32, 1, "dchar");

  return builtin_d_type;
}

static const registry<gdbarch>::key<struct builtin_d_type> d_type_data;

/* Return the D type table for the specified architecture.  */

const struct builtin_d_type *
builtin_d_type (struct gdbarch *gdbarch)
{
  struct builtin_d_type *result = d_type_data.get (gdbarch);
  if (result == nullptr)
    {
      result = build_d_types (gdbarch);
      d_type_data.set (gdbarch, result);
    }

  return result;
}
