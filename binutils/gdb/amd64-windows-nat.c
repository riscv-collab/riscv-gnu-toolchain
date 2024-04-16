/* Copyright (C) 2008-2024 Free Software Foundation, Inc.

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
#include "windows-nat.h"
#include "x86-nat.h"
#include "amd64-tdep.h"

#include <windows.h>

#define context_offset(x) (offsetof (CONTEXT, x))
const int amd64_mappings[] =
{
  context_offset (Rax),
  context_offset (Rbx),
  context_offset (Rcx),
  context_offset (Rdx),
  context_offset (Rsi),
  context_offset (Rdi),
  context_offset (Rbp),
  context_offset (Rsp),
  context_offset (R8),
  context_offset (R9),
  context_offset (R10),
  context_offset (R11),
  context_offset (R12),
  context_offset (R13),
  context_offset (R14),
  context_offset (R15),
  context_offset (Rip),
  context_offset (EFlags),
  context_offset (SegCs),
  context_offset (SegSs),
  context_offset (SegDs),
  context_offset (SegEs),
  context_offset (SegFs),
  context_offset (SegGs),
  context_offset (FloatSave.FloatRegisters[0]),
  context_offset (FloatSave.FloatRegisters[1]),
  context_offset (FloatSave.FloatRegisters[2]),
  context_offset (FloatSave.FloatRegisters[3]),
  context_offset (FloatSave.FloatRegisters[4]),
  context_offset (FloatSave.FloatRegisters[5]),
  context_offset (FloatSave.FloatRegisters[6]),
  context_offset (FloatSave.FloatRegisters[7]),
  context_offset (FloatSave.ControlWord),
  context_offset (FloatSave.StatusWord),
  context_offset (FloatSave.TagWord),
  context_offset (FloatSave.ErrorSelector),
  context_offset (FloatSave.ErrorOffset),
  context_offset (FloatSave.DataSelector),
  context_offset (FloatSave.DataOffset),
  context_offset (FloatSave.ErrorSelector)
  /* XMM0-7 */ ,
  context_offset (Xmm0),
  context_offset (Xmm1),
  context_offset (Xmm2),
  context_offset (Xmm3),
  context_offset (Xmm4),
  context_offset (Xmm5),
  context_offset (Xmm6),
  context_offset (Xmm7),
  context_offset (Xmm8),
  context_offset (Xmm9),
  context_offset (Xmm10),
  context_offset (Xmm11),
  context_offset (Xmm12),
  context_offset (Xmm13),
  context_offset (Xmm14),
  context_offset (Xmm15),
  /* MXCSR */
  context_offset (FloatSave.MxCsr)
};
#undef context_offset

/* segment_register_p_ftype implementation for amd64.  */

int
amd64_windows_segment_register_p (int regnum)
{
  return regnum >= AMD64_CS_REGNUM && regnum <= AMD64_GS_REGNUM;
}

void _initialize_amd64_windows_nat ();
void
_initialize_amd64_windows_nat ()
{
  x86_set_debug_register_length (8);
}
