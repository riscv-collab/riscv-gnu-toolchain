/* Header file for GDB compile command and supporting functions.
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

#ifndef COMPILE_COMPILE_INTERNAL_H
#define COMPILE_COMPILE_INTERNAL_H

#include "gcc-c-interface.h"
#include "gdbsupport/gdb-hashtab.h"

/* Debugging flag for the "compile" family of commands.  */

extern bool compile_debug;

/* Define header and footers for different scopes.  */

/* A simple scope just declares a function named "_gdb_expr", takes no
   arguments and returns no value.  */

#define COMPILE_I_SIMPLE_REGISTER_STRUCT_TAG "__gdb_regs"
#define COMPILE_I_SIMPLE_REGISTER_ARG_NAME "__regs"
#define COMPILE_I_SIMPLE_REGISTER_DUMMY "_dummy"
#define COMPILE_I_PRINT_OUT_ARG_TYPE "void *"
#define COMPILE_I_PRINT_OUT_ARG "__gdb_out_param"
#define COMPILE_I_EXPR_VAL "__gdb_expr_val"
#define COMPILE_I_EXPR_PTR_TYPE "__gdb_expr_ptr_type"

/* A "type" to indicate a NULL type.  */

const gcc_type GCC_TYPE_NONE = (gcc_type) -1;

/* Call gdbarch_register_name (GDBARCH, REGNUM) and convert its result
   to a form suitable for the compiler source.  The register names
   should not clash with inferior defined macros. */

extern std::string compile_register_name_mangled (struct gdbarch *gdbarch,
						  int regnum);

/* Convert compiler source register name to register number of
   GDBARCH.  Returned value is always >= 0, function throws an error
   for non-matching REG_NAME.  */

extern int compile_register_name_demangle (struct gdbarch *gdbarch,
					   const char *reg_name);

/* Type used to hold and pass around the source and object file names
   to use for compilation.  */
class compile_file_names
{
public:
  compile_file_names (std::string source_file, std::string object_file)
    : m_source_file (source_file), m_object_file (object_file)
  {}

  /* Provide read-only views only.  Return 'const char *' instead of
     std::string to avoid having to use c_str() everywhere in client
     code.  */

  const char *source_file () const
  { return m_source_file.c_str (); }

  const char *object_file () const
  { return m_object_file.c_str (); }

private:
  /* Storage for the file names.  */
  std::string m_source_file;
  std::string m_object_file;
};

#endif /* COMPILE_COMPILE_INTERNAL_H */
