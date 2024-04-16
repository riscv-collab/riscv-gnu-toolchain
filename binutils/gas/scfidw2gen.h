/* scfidw2gen.h - Support for emitting synthesized Dwarf2 CFI.
   Copyright (C) 2023 Free Software Foundation, Inc.

   This file is part of GAS, the GNU Assembler.

   GAS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GAS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GAS; see the file COPYING.  If not, write to the Free
   Software Foundation, 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */

#ifndef SCFIDW2GEN_H
#define SCFIDW2GEN_H

#include "as.h"
#include "dwarf2.h"

extern const pseudo_typeS scfi_pseudo_table[];

void scfi_dot_cfi_startproc (const symbolS *start_sym);
void scfi_dot_cfi_endproc (const symbolS *end_sym);
void scfi_dot_cfi (int arg, unsigned reg1, unsigned reg2, offsetT offset,
		   const char *name, const symbolS *advloc);

#endif

