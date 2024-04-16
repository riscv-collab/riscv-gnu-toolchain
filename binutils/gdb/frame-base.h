/* Definitions for a frame base, for GDB, the GNU debugger.

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

#if !defined (FRAME_BASE_H)
#define FRAME_BASE_H 1

class frame_info_ptr;
struct frame_id;
struct frame_unwind;
struct frame_base;
struct gdbarch;
struct regcache;

/* Assuming the frame chain: (outer) prev <-> this <-> next (inner);
   and that this is a `normal frame'; use THIS frame, and implicitly
   the NEXT frame's register unwind method, to determine the address
   of THIS frame's `base'.

   The exact meaning of `base' is highly dependant on the type of the
   debug info.  It is assumed that dwarf2, stabs, ... will each
   provide their own methods.

   A typical implementation will return the same value for base,
   locals-base and args-base.  That value, however, will likely be
   different to the frame ID's stack address.  */

/* A generic base address.  */

typedef CORE_ADDR (frame_this_base_ftype) (frame_info_ptr this_frame,
					   void **this_base_cache);

/* The base address of the frame's local variables.  */

typedef CORE_ADDR (frame_this_locals_ftype) (frame_info_ptr this_frame,
					     void **this_base_cache);

/* The base address of the frame's arguments / parameters.  */

typedef CORE_ADDR (frame_this_args_ftype) (frame_info_ptr this_frame,
					   void **this_base_cache);

struct frame_base
{
  /* If non-NULL, a low-level unwinder that shares its implementation
     with this high-level frame-base method.  */
  const struct frame_unwind *unwind;
  frame_this_base_ftype *this_base;
  frame_this_locals_ftype *this_locals;
  frame_this_args_ftype *this_args;
};

/* Given THIS frame, return the frame base methods for THIS frame,
   or NULL if it can't handle THIS frame.  */

typedef const struct frame_base *(frame_base_sniffer_ftype) (frame_info_ptr this_frame);

/* Append a frame base sniffer to the list.  The sniffers are polled
   in the order that they are appended.  */

extern void frame_base_append_sniffer (struct gdbarch *gdbarch,
				       frame_base_sniffer_ftype *sniffer);

/* Set the default frame base.  If all else fails, this one is
   returned.  If this isn't set, the default is to use legacy code
   that uses things like the frame ID's base (ulgh!).  */

extern void frame_base_set_default (struct gdbarch *gdbarch,
				    const struct frame_base *def);

/* Iterate through the list of frame base handlers until one returns
   an implementation.  */

extern const struct frame_base *frame_base_find_by_frame (frame_info_ptr this_frame);

#endif
