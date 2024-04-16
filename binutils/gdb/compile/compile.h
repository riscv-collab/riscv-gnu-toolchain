/* Header file for Compile and inject module.

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

#ifndef COMPILE_COMPILE_H
#define COMPILE_COMPILE_H

#include "gcc-c-interface.h"

struct ui_file;
struct gdbarch;
struct dwarf2_per_cu_data;
struct dwarf2_per_objfile;
struct symbol;
struct dynamic_prop;

/* An object of this type holds state associated with a given
   compilation job.  */

class compile_instance
{
public:
  compile_instance (struct gcc_base_context *gcc_fe, const char *options);

  virtual ~compile_instance ()
  {
    m_gcc_fe->ops->destroy (m_gcc_fe);
  }

  /* Returns the GCC options to be passed during compilation.  */
  const std::string &gcc_target_options () const
  {
    return m_gcc_target_options;
  }

  /* Query the type cache for TYPE, returning the compiler's
     type for it in RET.  */
  bool get_cached_type (struct type *type, gcc_type *ret) const;

  /* Insert GCC_TYPE into the type cache for TYPE.

     It is ok for a given type to be inserted more than once, provided that
     the exact same association is made each time.  */
  void insert_type (struct type *type, gcc_type gcc_type);

  /* Associate SYMBOL with some error text.  */
  void insert_symbol_error (const struct symbol *sym, const char *text);

  /* Emit the error message corresponding to SYM, if one exists, and
     arrange for it not to be emitted again.  */
  void error_symbol_once (const struct symbol *sym);

  /* These currently just forward to the underlying ops
     vtable.  */

  /* Set the plug-in print callback.  */
  void set_print_callback (void (*print_function) (void *, const char *),
			   void *datum);

  /* Return the plug-in's front-end version.  */
  unsigned int version () const;

  /* Set the plug-in's verbosity level.  Nop for GCC_FE_VERSION_0.  */
  void set_verbose (int level);

  /* Set the plug-in driver program.  Nop for GCC_FE_VERSION_0.  */
  void set_driver_filename (const char *filename);

  /* Set the regular expression used to match the configury triplet
     prefix to the compiler.  Nop for GCC_FE_VERSION_0.  */
  void set_triplet_regexp (const char *regexp);

  /* Set compilation arguments.  REGEXP is only used for protocol
     version GCC_FE_VERSION_0.  */
  gdb::unique_xmalloc_ptr<char> set_arguments (int argc, char **argv,
					       const char *regexp = NULL);

  /* Set the filename of the program to compile.  Nop for GCC_FE_VERSION_0.  */
  void set_source_file (const char *filename);

  /* Compile the previously specified source file to FILENAME.
     VERBOSE_LEVEL is only used for protocol version GCC_FE_VERSION_0.  */
  bool compile (const char *filename, int verbose_level = -1);

  /* Set the scope type for this compile.  */
  void set_scope (enum compile_i_scope_types scope)
  {
    m_scope = scope;
  }

  /* Return the scope type.  */
  enum compile_i_scope_types scope () const
  {
    return m_scope;
  }

  /* Set the block to be used for symbol searches.  */
  void set_block (const struct block *block)
  {
    m_block = block;
  }

  /* Return the search block.  */
  const struct block *block () const
  {
    return m_block;
  }

protected:

  /* The GCC front end.  */
  struct gcc_base_context *m_gcc_fe;

  /* The "scope" of this compilation.  */
  enum compile_i_scope_types m_scope;

  /* The block in which an expression is being parsed.  */
  const struct block *m_block;

  /* Specify "-std=gnu11", "-std=gnu++11" or similar.  These options are put
     after CU's DW_AT_producer compilation options to override them.  */
  std::string m_gcc_target_options;

  /* Map from gdb types to gcc types.  */
  htab_up m_type_map;

  /* Map from gdb symbols to gcc error messages to emit.  */
  htab_up m_symbol_err_map;
};

/* Public function that is called from compile_control case in the
   expression command.  GDB returns either a CMD, or a CMD_STRING, but
   never both.  */

extern void eval_compile_command (struct command_line *cmd,
				  const char *cmd_string,
				  enum compile_i_scope_types scope,
				  void *scope_data);

/* Compile a DWARF location expression to C, suitable for use by the
   compiler.

   STREAM is the stream where the code should be written.

   RESULT_NAME is the name of a variable in the resulting C code.  The
   result of the expression will be assigned to this variable.

   SYM is the symbol corresponding to this expression.
   PC is the location at which the expression is being evaluated.
   ARCH is the architecture to use.

   REGISTERS_USED is an out parameter which is updated to note which
   registers were needed by this expression.

   ADDR_SIZE is the DWARF address size to use.

   OPT_PTR and OP_END are the bounds of the DWARF expression.

   PER_CU is the per-CU object used for looking up various other
   things.

   PER_OBJFILE is the per-objfile object also used for looking up various other
   things.  */

extern void compile_dwarf_expr_to_c (string_file *stream,
				     const char *result_name,
				     struct symbol *sym,
				     CORE_ADDR pc,
				     struct gdbarch *arch,
				     std::vector<bool> &registers_used,
				     unsigned int addr_size,
				     const gdb_byte *op_ptr,
				     const gdb_byte *op_end,
				     dwarf2_per_cu_data *per_cu,
				     dwarf2_per_objfile *per_objfile);

/* Compile a DWARF bounds expression to C, suitable for use by the
   compiler.

   STREAM is the stream where the code should be written.

   RESULT_NAME is the name of a variable in the resulting C code.  The
   result of the expression will be assigned to this variable.

   PROP is the dynamic property for which we're compiling.

   SYM is the symbol corresponding to this expression.
   PC is the location at which the expression is being evaluated.
   ARCH is the architecture to use.

   REGISTERS_USED is an out parameter which is updated to note which
   registers were needed by this expression.

   ADDR_SIZE is the DWARF address size to use.

   OPT_PTR and OP_END are the bounds of the DWARF expression.

   PER_CU is the per-CU object used for looking up various other
   things.

   PER_OBJFILE is the per-objfile object also used for looking up various other
   things.  */

extern void compile_dwarf_bounds_to_c (string_file *stream,
				       const char *result_name,
				       const struct dynamic_prop *prop,
				       struct symbol *sym, CORE_ADDR pc,
				       struct gdbarch *arch,
				       std::vector<bool> &registers_used,
				       unsigned int addr_size,
				       const gdb_byte *op_ptr,
				       const gdb_byte *op_end,
				       dwarf2_per_cu_data *per_cu,
				       dwarf2_per_objfile *per_objfile);

extern void compile_print_value (struct value *val, void *data_voidp);

/* Command element for the 'compile' command.  */
extern cmd_list_element *compile_cmd_element;

#endif /* COMPILE_COMPILE_H */
