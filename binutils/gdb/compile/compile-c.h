/* Header file for GDB compile C-language support.
   Copyright (C) 2014-2024 Free Software Foundation, Inc.

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

#ifndef COMPILE_COMPILE_C_H
#define COMPILE_COMPILE_C_H

#include "compile/compile.h"
#include "gdbsupport/enum-flags.h"
#include "gcc-c-plugin.h"

/* enum-flags wrapper.  */

DEF_ENUM_FLAGS_TYPE (enum gcc_qualifiers, gcc_qualifiers_flags);

/* A callback suitable for use as the GCC C symbol oracle.  */

extern gcc_c_oracle_function gcc_convert_symbol;

/* A callback suitable for use as the GCC C address oracle.  */

extern gcc_c_symbol_address_function gcc_symbol_address;

/* A subclass of compile_instance that is specific to the C front
   end.  */

class compile_c_instance : public compile_instance
{
public:
  explicit compile_c_instance (struct gcc_c_context *gcc_c)
    : compile_instance (&gcc_c->base, m_default_cflags),
      m_plugin (gcc_c)
  {
    m_plugin.set_callbacks (gcc_convert_symbol, gcc_symbol_address, this);
  }

  /* Convert a gdb type, TYPE, to a GCC type.

     The new GCC type is returned.  */
  gcc_type convert_type (struct type *type);

  /* Return a handle for the GCC plug-in.  */
  gcc_c_plugin &plugin () { return m_plugin; }

private:
  /* Default compiler flags for C.  */
  static const char *m_default_cflags;

  /* The GCC plug-in.  */
  gcc_c_plugin m_plugin;
};

/* Emit code to compute the address for all the local variables in
   scope at PC in BLOCK.  Returns a malloc'd vector, indexed by gdb
   register number, where each element indicates if the corresponding
   register is needed to compute a local variable.  */

extern std::vector<bool>
  generate_c_for_variable_locations
     (compile_instance *compiler,
      string_file *stream,
      struct gdbarch *gdbarch,
      const struct block *block,
      CORE_ADDR pc);

/* Get the GCC mode attribute value for a given type size.  */

extern const char *c_get_mode_for_size (int size);

/* Given a dynamic property, return an xmallocd name that is used to
   represent its size.  The result must be freed by the caller.  The
   contents of the resulting string will be the same each time for
   each call with the same argument.  */

struct dynamic_prop;
extern std::string c_get_range_decl_name (const struct dynamic_prop *prop);

/* Compute the name of the pointer representing a local symbol's
   address.  */

extern gdb::unique_xmalloc_ptr<char>
  c_symbol_substitution_name (struct symbol *sym);

#endif /* COMPILE_COMPILE_C_H */
