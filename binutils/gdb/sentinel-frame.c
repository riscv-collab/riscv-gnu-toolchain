/* Code dealing with register stack frames, for GDB, the GNU debugger.

   Copyright (C) 1986-2024 Free Software Foundation, Inc.

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
#include "regcache.h"
#include "sentinel-frame.h"
#include "inferior.h"
#include "frame-unwind.h"

struct frame_unwind_cache
{
  struct regcache *regcache;
};

void *
sentinel_frame_cache (struct regcache *regcache)
{
  struct frame_unwind_cache *cache = 
    FRAME_OBSTACK_ZALLOC (struct frame_unwind_cache);

  cache->regcache = regcache;
  return cache;
}

/* Here the register value is taken direct from the register cache.  */

static struct value *
sentinel_frame_prev_register (frame_info_ptr this_frame,
			      void **this_prologue_cache,
			      int regnum)
{
  struct frame_unwind_cache *cache
    = (struct frame_unwind_cache *) *this_prologue_cache;
  struct value *value;

  frame_id this_frame_id = get_frame_id (this_frame);
  gdb_assert (is_sentinel_frame_id (this_frame_id));

  value = cache->regcache->cooked_read_value (regnum);

  return value;
}

static void
sentinel_frame_this_id (frame_info_ptr this_frame,
			void **this_prologue_cache,
			struct frame_id *this_id)
{
  /* The sentinel frame is used as a starting point for creating the
     previous (inner most) frame.  That frame's THIS_ID method will be
     called to determine the inner most frame's ID.  Not this one.  */
  internal_error (_("sentinel_frame_this_id called"));
}

static struct gdbarch *
sentinel_frame_prev_arch (frame_info_ptr this_frame,
			  void **this_prologue_cache)
{
  struct frame_unwind_cache *cache
    = (struct frame_unwind_cache *) *this_prologue_cache;

  return cache->regcache->arch ();
}

const struct frame_unwind sentinel_frame_unwind =
{
  "sentinel",
  SENTINEL_FRAME,
  default_frame_unwind_stop_reason,
  sentinel_frame_this_id,
  sentinel_frame_prev_register,
  NULL,
  NULL,
  NULL,
  sentinel_frame_prev_arch,
};
