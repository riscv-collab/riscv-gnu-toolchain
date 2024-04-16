/* Definitions for dealing with stack frames, for GDB, the GNU debugger.

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

#ifndef GDB_FRAME_ID_H
#define GDB_FRAME_ID_H 1

/* Status of a given frame's stack.  */

enum frame_id_stack_status
{
  /* Stack address is invalid.  */
  FID_STACK_INVALID = 0,

  /* Stack address is valid, and is found in the stack_addr field.  */
  FID_STACK_VALID = 1,

  /* Sentinel frame.  */
  FID_STACK_SENTINEL = 2,

  /* Outer frame.  Since a frame's stack address is typically defined as the
     value the stack pointer had prior to the activation of the frame, an outer
     frame doesn't have a stack address.  The frame ids of frames inlined in the
     outer frame are also of this type.  */
  FID_STACK_OUTER = 3,

  /* Stack address is unavailable.  I.e., there's a valid stack, but
     we don't know where it is (because memory or registers we'd
     compute it from were not collected).  */
  FID_STACK_UNAVAILABLE = -1
};

/* The frame object's ID.  This provides a per-frame unique identifier
   that can be used to relocate a `struct frame_info' after a target
   resume or a frame cache destruct.  It of course assumes that the
   inferior hasn't unwound the stack past that frame.  */

struct frame_id
{
  /* The frame's stack address.  This shall be constant through out
     the lifetime of a frame.  Note that this requirement applies to
     not just the function body, but also the prologue and (in theory
     at least) the epilogue.  Since that value needs to fall either on
     the boundary, or within the frame's address range, the frame's
     outer-most address (the inner-most address of the previous frame)
     is used.  Watch out for all the legacy targets that still use the
     function pointer register or stack pointer register.  They are
     wrong.

     This field is valid only if frame_id.stack_status is
     FID_STACK_VALID.  It will be 0 for other
     FID_STACK_... statuses.  */
  CORE_ADDR stack_addr;

  /* The frame's code address.  This shall be constant through out the
     lifetime of the frame.  While the PC (a.k.a. resume address)
     changes as the function is executed, this code address cannot.
     Typically, it is set to the address of the entry point of the
     frame's function (as returned by get_frame_func).

     For inlined functions (INLINE_DEPTH != 0), this is the address of
     the first executed instruction in the block corresponding to the
     inlined function.

     This field is valid only if code_addr_p is true.  Otherwise, this
     frame is considered to have a wildcard code address, i.e. one that
     matches every address value in frame comparisons.  */
  CORE_ADDR code_addr;

  /* The frame's special address.  This shall be constant through out the
     lifetime of the frame.  This is used for architectures that may have
     frames that do not change the stack but are still distinct and have
     some form of distinct identifier (e.g. the ia64 which uses a 2nd
     stack for registers).  This field is treated as unordered - i.e. will
     not be used in frame ordering comparisons.

     This field is valid only if special_addr_p is true.  Otherwise, this
     frame is considered to have a wildcard special address, i.e. one that
     matches every address value in frame comparisons.  */
  CORE_ADDR special_addr;

  /* Flags to indicate the above fields have valid contents.  */
  ENUM_BITFIELD(frame_id_stack_status) stack_status : 3;
  unsigned int code_addr_p : 1;
  unsigned int special_addr_p : 1;

  /* True if this frame was created from addresses given by the user (see
     create_new_frame) rather than through unwinding.  */
  unsigned int user_created_p : 1;

  /* It is non-zero for a frame made up by GDB without stack data
     representation in inferior, such as INLINE_FRAME or TAILCALL_FRAME.
     Caller of inlined function will have it zero, each more inner called frame
     will have it increasingly one, two etc.  Similarly for TAILCALL_FRAME.  */
  int artificial_depth;

  /* Return a string representation of this frame id.  */
  std::string to_string () const;

  /* Returns true when this frame_id and R identify the same
     frame.  */
  bool operator== (const frame_id &r) const;

  /* Inverse of ==.  */
  bool operator!= (const frame_id &r) const
  {
    return !(*this == r);
  }
};

/* Methods for constructing and comparing Frame IDs.  */

/* For convenience.  All fields are zero.  This means "there is no frame".  */
extern const struct frame_id null_frame_id;

/* This means "there is no frame ID, but there is a frame".  It should be
   replaced by best-effort frame IDs for the outermost frame, somehow.
   The implementation is only special_addr_p set.  */
extern const struct frame_id outer_frame_id;

/* Return true if ID represents a sentinel frame.  */
static inline bool
is_sentinel_frame_id (frame_id id)
{
  return id.stack_status == FID_STACK_SENTINEL;
}

#endif /* ifdef GDB_FRAME_ID_H  */
