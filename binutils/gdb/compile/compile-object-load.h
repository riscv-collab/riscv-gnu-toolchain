/* Header file to load module for 'compile' command.
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

#ifndef COMPILE_COMPILE_OBJECT_LOAD_H
#define COMPILE_COMPILE_OBJECT_LOAD_H

#include "compile-internal.h"
#include <list>

struct munmap_list
{
public:

  munmap_list () = default;
  ~munmap_list ();

  DISABLE_COPY_AND_ASSIGN (munmap_list);

  munmap_list &operator= (munmap_list &&) = default;
  munmap_list (munmap_list &&) = default;

  /* Add a region to the list.  */
  void add (CORE_ADDR addr, CORE_ADDR size);

private:

  /* Track inferior memory reserved by inferior mmap.  */

  struct munmap_item
  {
    CORE_ADDR addr, size;
  };

  std::vector<munmap_item> items;
};

struct compile_module
{
  compile_module () = default;

  DISABLE_COPY_AND_ASSIGN (compile_module);

  compile_module &operator= (compile_module &&other) = default;
  compile_module (compile_module &&other) = default;

  /* objfile for the compiled module.  */
  struct objfile *objfile;

  /* .c file OBJFILE was built from.  */
  std::string source_file;

  /* Inferior function GCC_FE_WRAPPER_FUNCTION.  */
  struct symbol *func_sym;

  /* Inferior registers address or NULL if the inferior function does not
     require any.  */
  CORE_ADDR regs_addr;

  /* The "scope" of this compilation.  */
  enum compile_i_scope_types scope;

  /* User data for SCOPE in use.  */
  void *scope_data;

  /* Inferior parameter out value type or NULL if the inferior function does not
     have one.  */
  struct type *out_value_type;

  /* If the inferior function has an out value, this is its address.
     Otherwise it is zero.  */
  CORE_ADDR out_value_addr;

  /* Track inferior memory reserved by inferior mmap.  */
  struct munmap_list munmap_list;
};

/* A unique pointer for a compile_module.  */
typedef std::unique_ptr<compile_module> compile_module_up;

extern compile_module_up compile_object_load
  (const compile_file_names &fnames,
   enum compile_i_scope_types scope, void *scope_data);

#endif /* COMPILE_COMPILE_OBJECT_LOAD_H */
