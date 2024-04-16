/* Low level support for x86 (i386 and x86-64), shared between gdbserver
   and IPA.

   Copyright (C) 2016-2024 Free Software Foundation, Inc.

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

#ifndef GDBSERVER_LINUX_X86_TDESC_H
#define GDBSERVER_LINUX_X86_TDESC_H

/* Note: since IPA obviously knows what ABI it's running on (i386 vs x86_64
   vs x32), it's sufficient to pass only the register set here.  This,
   together with the ABI known at IPA compile time, maps to a tdesc.  */

enum x86_linux_tdesc {
  X86_TDESC_MMX = 0,
  X86_TDESC_SSE = 1,
  X86_TDESC_AVX = 2,
  X86_TDESC_MPX = 3,
  X86_TDESC_AVX_MPX = 4,
  X86_TDESC_AVX_AVX512 = 5,
  X86_TDESC_AVX_MPX_AVX512_PKU = 6,
  X86_TDESC_LAST = 7,
};

#if defined __i386__ || !defined IN_PROCESS_AGENT
int i386_get_ipa_tdesc_idx (const struct target_desc *tdesc);
#endif

#if defined __x86_64__ && !defined IN_PROCESS_AGENT
int amd64_get_ipa_tdesc_idx (const struct target_desc *tdesc);
#endif

const struct target_desc *i386_get_ipa_tdesc (int idx);

#ifdef __x86_64__
const struct target_desc *amd64_linux_read_description (uint64_t xcr0,
							bool is_x32);
#endif

const struct target_desc *i386_linux_read_description (uint64_t xcr0);

#endif /* GDBSERVER_LINUX_X86_TDESC_H */
