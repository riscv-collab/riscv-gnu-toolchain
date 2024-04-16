/* DWARF 2 abbrev table cache

   Copyright (C) 2022-2024 Free Software Foundation, Inc.

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
#include "dwarf2/read.h"
#include "dwarf2/abbrev-cache.h"

/* Hash function for an abbrev table.  */

hashval_t
abbrev_cache::hash_table (const void *item)
{
  const struct abbrev_table *table = (const struct abbrev_table *) item;
  return to_underlying (table->sect_off);
}

/* Comparison function for abbrev table.  */

int
abbrev_cache::eq_table (const void *lhs, const void *rhs)
{
  const struct abbrev_table *l_table = (const struct abbrev_table *) lhs;
  const search_key *key = (const search_key *) rhs;
  return (l_table->section == key->section
	  && l_table->sect_off == key->offset);
}

abbrev_cache::abbrev_cache ()
  : m_tables (htab_create_alloc (20, hash_table, eq_table,
				 htab_delete_entry<abbrev_table>,
				 xcalloc, xfree))
{
}

void
abbrev_cache::add (abbrev_table_up table)
{
  /* We allow this as a convenience to the caller.  */
  if (table == nullptr)
    return;

  search_key key = { table->section, table->sect_off };
  void **slot = htab_find_slot_with_hash (m_tables.get (), &key,
					  to_underlying (table->sect_off),
					  INSERT);
  /* If this one already existed, then it should have been reused.  */
  gdb_assert (*slot == nullptr);
  *slot = (void *) table.release ();
}
