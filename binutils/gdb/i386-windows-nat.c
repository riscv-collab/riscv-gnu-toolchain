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
#include "i386-tdep.h"

#include <windows.h>

#ifdef __x86_64__
#define CONTEXT WOW64_CONTEXT
#endif
#define context_offset(x) ((int)(size_t)&(((CONTEXT *)NULL)->x))
const int i386_mappings[] =
{
  context_offset (Eax),
  context_offset (Ecx),
  context_offset (Edx),
  context_offset (Ebx),
  context_offset (Esp),
  context_offset (Ebp),
  context_offset (Esi),
  context_offset (Edi),
  context_offset (Eip),
  context_offset (EFlags),
  context_offset (SegCs),
  context_offset (SegSs),
  context_offset (SegDs),
  context_offset (SegEs),
  context_offset (SegFs),
  context_offset (SegGs),
  context_offset (FloatSave.RegisterArea[0 * 10]),
  context_offset (FloatSave.RegisterArea[1 * 10]),
  context_offset (FloatSave.RegisterArea[2 * 10]),
  context_offset (FloatSave.RegisterArea[3 * 10]),
  context_offset (FloatSave.RegisterArea[4 * 10]),
  context_offset (FloatSave.RegisterArea[5 * 10]),
  context_offset (FloatSave.RegisterArea[6 * 10]),
  context_offset (FloatSave.RegisterArea[7 * 10]),
  context_offset (FloatSave.ControlWord),
  context_offset (FloatSave.StatusWord),
  context_offset (FloatSave.TagWord),
  context_offset (FloatSave.ErrorSelector),
  context_offset (FloatSave.ErrorOffset),
  context_offset (FloatSave.DataSelector),
  context_offset (FloatSave.DataOffset),
  context_offset (FloatSave.ErrorSelector)
  /* XMM0-7 */ ,
  context_offset (ExtendedRegisters[10*16]),
  context_offset (ExtendedRegisters[11*16]),
  context_offset (ExtendedRegisters[12*16]),
  context_offset (ExtendedRegisters[13*16]),
  context_offset (ExtendedRegisters[14*16]),
  context_offset (ExtendedRegisters[15*16]),
  context_offset (ExtendedRegisters[16*16]),
  context_offset (ExtendedRegisters[17*16]),
  /* MXCSR */
  context_offset (ExtendedRegisters[24])
};
#undef context_offset
#undef CONTEXT

/* segment_register_p_ftype implementation for x86.  */

int
i386_windows_segment_register_p (int regnum)
{
  return regnum >= I386_CS_REGNUM && regnum <= I386_GS_REGNUM;
}

void _initialize_i386_windows_nat ();
void
_initialize_i386_windows_nat ()
{
#ifndef __x86_64__
  x86_set_debug_register_length (4);
#endif
}
