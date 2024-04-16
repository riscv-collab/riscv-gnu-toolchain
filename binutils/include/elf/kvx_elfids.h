/* DO NOT EDIT!  -*- buffer-read-only: t -*-  vi:set ro: */

/* KVX ELF IDs definitions.

   Copyright (C) 2009-2024 Free Software Foundation, Inc.
   Contributed by Kalray SA.

   This file is part of GNU Binutils.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the license, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING3. If not,
   see <http://www.gnu.org/licenses/>.  */

/* This file holds definitions specific to the KV3 ELF IDs. */

#ifndef _KVX_ELFIDS_H_
#define _KVX_ELFIDS_H_

/* 	 16.15 	  8.7  4.3  0 */
/* +----------------------------+ */
/* |      CUT | CORE  |PIC |ABI | */
/* +----------------------------+ */


#define KVX_CUT_MASK 0x00ff0000
#define KVX_CORE_MASK 0x0000ff00
#define KVX_ABI_MASK 0x000000ff
#define KVX_MACH_MASK (KVX_CUT_MASK | KVX_CORE_MASK | KVX_ABI_MASK)

/*
 * Machine private data :
 * - byte 0 = ABI specific (PIC, OS, ...)
 *   - bit 0..3 = ABI ident
 *   - bit 4    = 32/64 bits addressing
 *   - bit 5    = PIC
 * - byte 1 = Core info :
 *   - bits 0..3 = Core Major Version
 *   - bit  4..7 = Core Minor Version
 */

/* Core */
#define ELF_KVX_CORE_BIT_SHIFT  (8)
#define ELF_KVX_CORE_MASK       (0x7f<<ELF_KVX_CORE_BIT_SHIFT)

#define ELF_KVX_CORE_MAJOR_MASK (0x0F << ELF_KVX_CORE_BIT_SHIFT)
#define ELF_KVX_CORE_MINOR_MASK (0xF0 << ELF_KVX_CORE_BIT_SHIFT)
#define ELF_KVX_CORE_MAJOR_SHIFT (0 + ELF_KVX_CORE_BIT_SHIFT)
#define ELF_KVX_CORE_MINOR_SHIFT (4 + ELF_KVX_CORE_BIT_SHIFT)

#define ELF_KVX_CORE_KV3         (0x03 << ELF_KVX_CORE_BIT_SHIFT)
#define ELF_KVX_CORE_KV4         (0x04 << ELF_KVX_CORE_BIT_SHIFT)

#define ELF_KVX_CORE_KV3_1      (ELF_KVX_CORE_KV3 | (1 << (ELF_KVX_CORE_MINOR_SHIFT)))
#define ELF_KVX_CORE_KV3_2      (ELF_KVX_CORE_KV3 | (2 << (ELF_KVX_CORE_MINOR_SHIFT)))
#define ELF_KVX_CORE_KV4_1      (ELF_KVX_CORE_KV4 | (1 << (ELF_KVX_CORE_MINOR_SHIFT)))

#define ELF_KVX_IS_KV3(flags)   (((flags) & ELF_KVX_CORE_MAJOR_MASK) == (ELF_KVX_CORE_KV3))
#define ELF_KVX_IS_KV4(flags)   (((flags) & ELF_KVX_CORE_MAJOR_MASK) == (ELF_KVX_CORE_KV4))
#define ELF_KVX_CHECK_CORE(flags,m) (((flags) & ELF_KVX_CORE_MASK)==(m))

#define ELF_KVX_ABI_MASK         (0xFF)

#define ELF_KVX_ABI_IDENT_MASK   (0x7)
#define ELF_KVX_ABI_REGULAR      (0x1)
#define ELF_KVX_ABI_UNDEF        (0x0)

#define ELF_KVX_ABI_64B_ADDR_BIT (0x08)

#define ELF_KVX_ABI_PIC_BIT      (0x10)

#endif /* _KVX_ELFIDS_H_ */
