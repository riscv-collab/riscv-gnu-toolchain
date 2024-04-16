/* KVX ELF support for BFD.

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

#ifndef _ELF_KVX_H
#define _ELF_KVX_H

#include "elf/reloc-macros.h"

START_RELOC_NUMBERS (elf_kvx_reloc_type)
    RELOC_NUMBER (R_KVX_NONE,                                  0)
    RELOC_NUMBER (R_KVX_16,                                    1)
    RELOC_NUMBER (R_KVX_32,                                    2)
    RELOC_NUMBER (R_KVX_64,                                    3)
    RELOC_NUMBER (R_KVX_S16_PCREL,                             4)
    RELOC_NUMBER (R_KVX_PCREL17,                               5)
    RELOC_NUMBER (R_KVX_PCREL27,                               6)
    RELOC_NUMBER (R_KVX_32_PCREL,                              7)
    RELOC_NUMBER (R_KVX_S37_PCREL_LO10,                        8)
    RELOC_NUMBER (R_KVX_S37_PCREL_UP27,                        9)
    RELOC_NUMBER (R_KVX_S43_PCREL_LO10,                       10)
    RELOC_NUMBER (R_KVX_S43_PCREL_UP27,                       11)
    RELOC_NUMBER (R_KVX_S43_PCREL_EX6,                        12)
    RELOC_NUMBER (R_KVX_S64_PCREL_LO10,                       13)
    RELOC_NUMBER (R_KVX_S64_PCREL_UP27,                       14)
    RELOC_NUMBER (R_KVX_S64_PCREL_EX27,                       15)
    RELOC_NUMBER (R_KVX_64_PCREL,                             16)
    RELOC_NUMBER (R_KVX_S16,                                  17)
    RELOC_NUMBER (R_KVX_S32_LO5,                              18)
    RELOC_NUMBER (R_KVX_S32_UP27,                             19)
    RELOC_NUMBER (R_KVX_S37_LO10,                             20)
    RELOC_NUMBER (R_KVX_S37_UP27,                             21)
    RELOC_NUMBER (R_KVX_S37_GOTOFF_LO10,                      22)
    RELOC_NUMBER (R_KVX_S37_GOTOFF_UP27,                      23)
    RELOC_NUMBER (R_KVX_S43_GOTOFF_LO10,                      24)
    RELOC_NUMBER (R_KVX_S43_GOTOFF_UP27,                      25)
    RELOC_NUMBER (R_KVX_S43_GOTOFF_EX6,                       26)
    RELOC_NUMBER (R_KVX_32_GOTOFF,                            27)
    RELOC_NUMBER (R_KVX_64_GOTOFF,                            28)
    RELOC_NUMBER (R_KVX_32_GOT,                               29)
    RELOC_NUMBER (R_KVX_S37_GOT_LO10,                         30)
    RELOC_NUMBER (R_KVX_S37_GOT_UP27,                         31)
    RELOC_NUMBER (R_KVX_S43_GOT_LO10,                         32)
    RELOC_NUMBER (R_KVX_S43_GOT_UP27,                         33)
    RELOC_NUMBER (R_KVX_S43_GOT_EX6,                          34)
    RELOC_NUMBER (R_KVX_64_GOT,                               35)
    RELOC_NUMBER (R_KVX_GLOB_DAT,                             36)
    RELOC_NUMBER (R_KVX_COPY,                                 37)
    RELOC_NUMBER (R_KVX_JMP_SLOT,                             38)
    RELOC_NUMBER (R_KVX_RELATIVE,                             39)
    RELOC_NUMBER (R_KVX_S43_LO10,                             40)
    RELOC_NUMBER (R_KVX_S43_UP27,                             41)
    RELOC_NUMBER (R_KVX_S43_EX6,                              42)
    RELOC_NUMBER (R_KVX_S64_LO10,                             43)
    RELOC_NUMBER (R_KVX_S64_UP27,                             44)
    RELOC_NUMBER (R_KVX_S64_EX27,                             45)
    RELOC_NUMBER (R_KVX_S37_GOTADDR_LO10,                     46)
    RELOC_NUMBER (R_KVX_S37_GOTADDR_UP27,                     47)
    RELOC_NUMBER (R_KVX_S43_GOTADDR_LO10,                     48)
    RELOC_NUMBER (R_KVX_S43_GOTADDR_UP27,                     49)
    RELOC_NUMBER (R_KVX_S43_GOTADDR_EX6,                      50)
    RELOC_NUMBER (R_KVX_S64_GOTADDR_LO10,                     51)
    RELOC_NUMBER (R_KVX_S64_GOTADDR_UP27,                     52)
    RELOC_NUMBER (R_KVX_S64_GOTADDR_EX27,                     53)
    RELOC_NUMBER (R_KVX_64_DTPMOD,                            54)
    RELOC_NUMBER (R_KVX_64_DTPOFF,                            55)
    RELOC_NUMBER (R_KVX_S37_TLS_DTPOFF_LO10,                  56)
    RELOC_NUMBER (R_KVX_S37_TLS_DTPOFF_UP27,                  57)
    RELOC_NUMBER (R_KVX_S43_TLS_DTPOFF_LO10,                  58)
    RELOC_NUMBER (R_KVX_S43_TLS_DTPOFF_UP27,                  59)
    RELOC_NUMBER (R_KVX_S43_TLS_DTPOFF_EX6,                   60)
    RELOC_NUMBER (R_KVX_S37_TLS_GD_LO10,                      61)
    RELOC_NUMBER (R_KVX_S37_TLS_GD_UP27,                      62)
    RELOC_NUMBER (R_KVX_S43_TLS_GD_LO10,                      63)
    RELOC_NUMBER (R_KVX_S43_TLS_GD_UP27,                      64)
    RELOC_NUMBER (R_KVX_S43_TLS_GD_EX6,                       65)
    RELOC_NUMBER (R_KVX_S37_TLS_LD_LO10,                      66)
    RELOC_NUMBER (R_KVX_S37_TLS_LD_UP27,                      67)
    RELOC_NUMBER (R_KVX_S43_TLS_LD_LO10,                      68)
    RELOC_NUMBER (R_KVX_S43_TLS_LD_UP27,                      69)
    RELOC_NUMBER (R_KVX_S43_TLS_LD_EX6,                       70)
    RELOC_NUMBER (R_KVX_64_TPOFF,                             71)
    RELOC_NUMBER (R_KVX_S37_TLS_IE_LO10,                      72)
    RELOC_NUMBER (R_KVX_S37_TLS_IE_UP27,                      73)
    RELOC_NUMBER (R_KVX_S43_TLS_IE_LO10,                      74)
    RELOC_NUMBER (R_KVX_S43_TLS_IE_UP27,                      75)
    RELOC_NUMBER (R_KVX_S43_TLS_IE_EX6,                       76)
    RELOC_NUMBER (R_KVX_S37_TLS_LE_LO10,                      77)
    RELOC_NUMBER (R_KVX_S37_TLS_LE_UP27,                      78)
    RELOC_NUMBER (R_KVX_S43_TLS_LE_LO10,                      79)
    RELOC_NUMBER (R_KVX_S43_TLS_LE_UP27,                      80)
    RELOC_NUMBER (R_KVX_S43_TLS_LE_EX6,                       81)
    RELOC_NUMBER (R_KVX_8,                                    82)
END_RELOC_NUMBERS (R_KVX_end)

#include "kvx_elfids.h"

#endif
