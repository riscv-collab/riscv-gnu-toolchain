/* Unwinder test program for signal frames.

   Copyright 2007-2024 Free Software Foundation, Inc.

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

void sigframe (void);
void setup (void);

void
func (void)
{
}

int
main (void)
{
  setup ();
}

/* Create an imitation signal frame.  This will work on any x86 or
   x86-64 target which uses a version of GAS recent enough for
   .cfi_signal_frame (added 2006-02-27 and included in binutils 2.17).
   The default CIE created by gas suffices to unwind from an empty
   function.  */

/* Note: to make sure that the Dwarf unwinder gets to handle
   the frame, we add an extra 'nop' after the label.  Otherwise,
   the epilogue unwinder will see the 'ret' and grab the frame.  */

asm(".text\n"
    "    .align 8\n"
    "    .globl setup\n"
    "setup:\n"
#if IS_AMD64_REGS_TARGET
    "    pushq $sigframe\n"
#else
    "    push $sigframe\n"
#endif
    "    jmp func\n"
    "\n"
    "    .cfi_startproc\n"
    "    .cfi_signal_frame\n"
    "    nop\n"
    "    .globl sigframe\n"
    "sigframe:\n"
    "    nop\n"
    "    ret\n"
    "    .cfi_endproc");
