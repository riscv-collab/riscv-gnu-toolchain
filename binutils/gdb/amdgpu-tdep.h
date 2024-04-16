/* Target-dependent code for the AMDGPU architectures.

   Copyright (C) 2019-2024 Free Software Foundation, Inc.

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

#ifndef AMDGPU_TDEP_H
#define AMDGPU_TDEP_H

#include "gdbarch.h"

#include <amd-dbgapi/amd-dbgapi.h>
#include <unordered_map>

/* Provide std::unordered_map::Hash for amd_dbgapi_register_id_t.  */
struct register_id_hash
{
  size_t
  operator() (const amd_dbgapi_register_id_t &register_id) const
  {
    return std::hash<decltype (register_id.handle)> () (register_id.handle);
  }
};

/* Provide std::unordered_map::Equal for amd_dbgapi_register_id_t.  */
struct register_id_equal_to
{
  bool
  operator() (const amd_dbgapi_register_id_t &lhs,
	      const amd_dbgapi_register_id_t &rhs) const
  {
    return std::equal_to<decltype (lhs.handle)> () (lhs.handle, rhs.handle);
  }
};

/* AMDGPU architecture specific information.  */
struct amdgpu_gdbarch_tdep : gdbarch_tdep_base
{
  /* This architecture's breakpoint instruction.  */
  gdb::unique_xmalloc_ptr<gdb_byte> breakpoint_instruction_bytes;
  size_t breakpoint_instruction_size;

  /* A vector of register_ids indexed by their equivalent gdb regnum.  */
  std::vector<amd_dbgapi_register_id_t> register_ids;

  /* A vector of register_properties indexed by their equivalent gdb regnum.  */
  std::vector<amd_dbgapi_register_properties_t> register_properties;

  /* A vector of register names indexed by their equivalent gdb regnum.  */
  std::vector<std::string> register_names;

  /* A vector of register types created from the amd-dbgapi type strings,
     indexed by their equivalent gdb regnum.  These are computed lazily by
     amdgpu_register_type, entries that haven't been computed yet are
     nullptr.  */
  std::vector<type *> register_types;

  /* A vector of GDB register numbers indexed by DWARF register number.

     Unused DWARF register numbers map to value -1.  */
  std::vector<int> dwarf_regnum_to_gdb_regnum;

  /* A map of gdb regnums keyed by they equivalent register_id.  */
  std::unordered_map<amd_dbgapi_register_id_t, int, register_id_hash,
		     register_id_equal_to>
    regnum_map;

  /* A map of register_class_ids keyed by their name.  */
  std::unordered_map<std::string, amd_dbgapi_register_class_id_t>
    register_class_map;
};

/* Return true if GDBARCH is of an AMDGPU architecture.  */
bool is_amdgpu_arch (struct gdbarch *gdbarch);

/* Return the amdgpu-specific data associated to ARCH.  */

amdgpu_gdbarch_tdep *get_amdgpu_gdbarch_tdep (gdbarch *arch);

#endif /* AMDGPU_TDEP_H */
