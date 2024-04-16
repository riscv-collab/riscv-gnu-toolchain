/* Things needed for both reading and writing DWARF indices.

   Copyright (C) 1994-2024 Free Software Foundation, Inc.

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

#ifndef DWARF_INDEX_COMMON_H
#define DWARF_INDEX_COMMON_H

/* The suffix for an index file.  */
#define INDEX4_SUFFIX ".gdb-index"
#define INDEX5_SUFFIX ".debug_names"
#define DEBUG_STR_SUFFIX ".debug_str"

/* All offsets in the index are of this type.  It must be
   architecture-independent.  */
typedef uint32_t offset_type;

/* Unpack a 32-bit little-endian value.  */

static inline offset_type
gdb_index_unpack (const gdb_byte *value)
{
  return (offset_type) extract_unsigned_integer (value, sizeof (offset_type),
						 BFD_ENDIAN_LITTLE);
}

/* The hash function for strings in the mapped index.  This is the same as
   SYMBOL_HASH_NEXT, but we keep a separate copy to maintain control over the
   implementation.  This is necessary because the hash function is tied to the
   format of the mapped index file.  The hash values do not have to match with
   SYMBOL_HASH_NEXT.

   Use INT_MAX for INDEX_VERSION if you generate the current index format.  */

hashval_t mapped_index_string_hash (int index_version, const void *p);

/* Symbol name hashing function as specified by DWARF-5.  */

uint32_t dwarf5_djb_hash (const char *str_);

/* Symbol name hashing function as specified by DWARF-5.  */

uint32_t dwarf5_djb_hash (std::string_view str_);

#endif /* DWARF_INDEX_COMMON_H */
