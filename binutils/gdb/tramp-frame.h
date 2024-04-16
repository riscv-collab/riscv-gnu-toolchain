/* Signal trampoline unwinder.

   Copyright (C) 2004-2024 Free Software Foundation, Inc.

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

#ifndef TRAMP_FRAME_H
#define TRAMP_FRAME_H

#include "frame.h"
#include "frame-unwind.h"	/* For frame_prev_arch_ftype.  */

class frame_info_ptr;
struct trad_frame_cache;

/* A trampoline consists of a small sequence of instructions placed at
   an unspecified location in the inferior's address space.  The only
   identifying attribute of the trampoline's address is that it does
   not fall inside an object file's section.

   The only way to identify a trampoline is to perform a brute force
   examination of the instructions at and around the PC.

   This module provides a convenient interface for performing that
   operation.  */

/* A trampoline descriptor.  */

/* Magic instruction that to mark the end of the signal trampoline
   instruction sequence.  */
#define TRAMP_SENTINEL_INSN ULONGEST_MAX

struct tramp_frame
{
  /* The trampoline's type, some a signal trampolines, some are normal
     call-frame trampolines (aka thunks).  */
  enum frame_type frame_type;
  /* The trampoline's entire instruction sequence.  It consists of a
     bytes/mask pair.  Search for this in the inferior at or around
     the frame's PC.  It is assumed that the PC is INSN_SIZE aligned,
     and that each element of TRAMP contains one INSN_SIZE
     instruction.  It is also assumed that INSN[0] contains the first
     instruction of the trampoline and hence the address of the
     instruction matching INSN[0] is the trampoline's "func" address.
     The instruction sequence is terminated by
     TRAMP_SENTINEL_INSN.  */
  int insn_size;
  struct
  {
    ULONGEST bytes;
    ULONGEST mask;
  } insn[48];
  /* Initialize a trad-frame cache corresponding to the tramp-frame.
     FUNC is the address of the instruction TRAMP[0] in memory.  */
  void (*init) (const struct tramp_frame *self,
		frame_info_ptr this_frame,
		struct trad_frame_cache *this_cache,
		CORE_ADDR func);
  /* Return non-zero if the tramp-frame is valid for the PC requested.
     Adjust the PC to point to the address to check the instruction
     sequence against if required.  If this is NULL, then the tramp-frame
     is valid for any PC.  */
  int (*validate) (const struct tramp_frame *self,
		   frame_info_ptr this_frame,
		   CORE_ADDR *pc);

  /* Given the current frame in THIS_FRAME and a frame cache in FRAME_CACHE,
     return the architecture of the previous frame.  */
  frame_prev_arch_ftype *prev_arch;
};

void tramp_frame_prepend_unwinder (struct gdbarch *gdbarch,
				   const struct tramp_frame *tramp);

#endif
