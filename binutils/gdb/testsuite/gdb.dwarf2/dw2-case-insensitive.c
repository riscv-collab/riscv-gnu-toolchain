/* This testcase is part of GDB, the GNU debugger.

   Copyright 2011-2024 Free Software Foundation, Inc.

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

/* Target-specific way of forcing an instruction label.  */
#ifdef __mips__
#define START_INSNS asm (".insn");
#else
#define START_INSNS
#endif

/* Use DW_LANG_Fortran90 for case insensitive DWARF.  */
asm (".globl cu_text_start");
asm ("cu_text_start:");
START_INSNS

asm (".globl FUNC_lang_start");
asm (".p2align 4");
asm ("FUNC_lang_start:");
START_INSNS

void
FUNC_lang (void)
{
}

asm (".globl FUNC_lang_end");
asm ("FUNC_lang_end:");

/* Symbol is present only in ELF .symtab.  */

void
FUNC_symtab (void)
{
}

int
main (void)
{
  FUNC_lang ();
  FUNC_symtab ();
  return 0;
}

asm (".globl cu_text_end");
asm ("cu_text_end:");
