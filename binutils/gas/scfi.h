/* scfi.h - Support for synthesizing CFI for asm.
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

#ifndef SCFI_H
#define SCFI_H

#include "as.h"
#include "ginsn.h"

void scfi_ops_cleanup (scfi_opS **head);

/* Some SCFI ops are not synthesized and are only added externally when parsing
   the assembler input.  Two examples are CFI_label, and CFI_signal_frame.  */
void scfi_op_add_cfi_label (ginsnS *ginsn, const char *name);
void scfi_op_add_signal_frame (ginsnS *ginsn);

int scfi_emit_dw2cfi (const symbolS *func);

int scfi_synthesize_dw2cfi (const symbolS *func, gcfgS *gcfg, gbbS *root_bb);

#endif /* SCFI_H.  */
