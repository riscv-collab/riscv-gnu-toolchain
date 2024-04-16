/* Definitions for frame address handler, for GDB, the GNU debugger.

   Copyright (C) 2003-2024 Free Software Foundation, Inc.

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
#include "frame-base.h"
#include "frame.h"
#include "gdbsupport/gdb_obstack.h"
#include "gdbarch.h"

/* A default frame base implementations.  If it wasn't for the old
   DEPRECATED_FRAME_LOCALS_ADDRESS and DEPRECATED_FRAME_ARGS_ADDRESS,
   these could be combined into a single function.  All architectures
   really need to override this.  */

static CORE_ADDR
default_frame_base_address (frame_info_ptr this_frame, void **this_cache)
{
  return get_frame_base (this_frame); /* sigh! */
}

static CORE_ADDR
default_frame_locals_address (frame_info_ptr this_frame, void **this_cache)
{
  return default_frame_base_address (this_frame, this_cache);
}

static CORE_ADDR
default_frame_args_address (frame_info_ptr this_frame, void **this_cache)
{
  return default_frame_base_address (this_frame, this_cache);
}

const struct frame_base default_frame_base = {
  NULL, /* No parent.  */
  default_frame_base_address,
  default_frame_locals_address,
  default_frame_args_address
};

struct frame_base_table_entry
{
  frame_base_sniffer_ftype *sniffer;
  struct frame_base_table_entry *next;
};

struct frame_base_table
{
  struct frame_base_table_entry *head = nullptr;
  struct frame_base_table_entry **tail = &head;
  const struct frame_base *default_base = &default_frame_base;
};

static const registry<gdbarch>::key<struct frame_base_table> frame_base_data;

static struct frame_base_table *
get_frame_base_table (struct gdbarch *gdbarch)
{
  struct frame_base_table *table = frame_base_data.get (gdbarch);
  if (table == nullptr)
    table = frame_base_data.emplace (gdbarch);
  return table;
}

void
frame_base_append_sniffer (struct gdbarch *gdbarch,
			   frame_base_sniffer_ftype *sniffer)
{
  struct frame_base_table *table = get_frame_base_table (gdbarch);

  (*table->tail)
    = GDBARCH_OBSTACK_ZALLOC (gdbarch, struct frame_base_table_entry);
  (*table->tail)->sniffer = sniffer;
  table->tail = &(*table->tail)->next;
}

void
frame_base_set_default (struct gdbarch *gdbarch,
			const struct frame_base *default_base)
{
  struct frame_base_table *table = get_frame_base_table (gdbarch);

  table->default_base = default_base;
}

const struct frame_base *
frame_base_find_by_frame (frame_info_ptr this_frame)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct frame_base_table *table = get_frame_base_table (gdbarch);
  struct frame_base_table_entry *entry;

  for (entry = table->head; entry != NULL; entry = entry->next)
    {
      const struct frame_base *desc = NULL;

      desc = entry->sniffer (this_frame);
      if (desc != NULL)
	return desc;
    }
  return table->default_base;
}
