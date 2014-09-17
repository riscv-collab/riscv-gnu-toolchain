/* RISC-V ELF support for BFD.
   Copyright 2011-2014 Free Software Foundation, Inc.

   Contributed by Andrw Waterman <waterman@cs.berkeley.edu> at UC Berkeley.
   Based on MIPS ELF support for BFD, by Ian Lance Taylor.

   This file is part of BFD, the Binary File Descriptor library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

/* This file holds definitions specific to the RISCV ELF ABI.  Note
   that most of this is not actually implemented by BFD.  */

#ifndef _ELF_RISCV_H
#define _ELF_RISCV_H

#include "elf/reloc-macros.h"

/* Relocation types.  */
START_RELOC_NUMBERS (elf_riscv_reloc_type)
  RELOC_NUMBER (R_RISCV_NONE, 0)
  RELOC_NUMBER (R_RISCV_32, 2)
  RELOC_NUMBER (R_RISCV_REL32, 3)
  RELOC_NUMBER (R_RISCV_JAL, 4)
  RELOC_NUMBER (R_RISCV_HI20, 5)
  RELOC_NUMBER (R_RISCV_LO12_I, 6)
  RELOC_NUMBER (R_RISCV_LO12_S, 7)
  RELOC_NUMBER (R_RISCV_PCREL_LO12_I, 8)
  RELOC_NUMBER (R_RISCV_PCREL_LO12_S, 9)
  RELOC_NUMBER (R_RISCV_BRANCH, 10)
  RELOC_NUMBER (R_RISCV_CALL, 11)
  RELOC_NUMBER (R_RISCV_PCREL_HI20, 12)
  RELOC_NUMBER (R_RISCV_CALL_PLT, 13)
  RELOC_NUMBER (R_RISCV_64, 18)
  RELOC_NUMBER (R_RISCV_GOT_HI20, 22)
  RELOC_NUMBER (R_RISCV_GOT_LO12, 23)
  RELOC_NUMBER (R_RISCV_COPY, 24)
  RELOC_NUMBER (R_RISCV_JUMP_SLOT, 25)
  /* TLS relocations.  */
  RELOC_NUMBER (R_RISCV_TLS_IE_HI20, 29)
  RELOC_NUMBER (R_RISCV_TLS_IE_LO12, 30)
  RELOC_NUMBER (R_RISCV_TLS_IE_ADD, 31)
  RELOC_NUMBER (R_RISCV_TLS_IE_LO12_I, 32)
  RELOC_NUMBER (R_RISCV_TLS_IE_LO12_S, 33)
  RELOC_NUMBER (R_RISCV_TPREL_HI20, 34)
  RELOC_NUMBER (R_RISCV_TPREL_LO12_I, 35)
  RELOC_NUMBER (R_RISCV_TPREL_LO12_S, 36)
  RELOC_NUMBER (R_RISCV_TPREL_ADD, 37)
  RELOC_NUMBER (R_RISCV_TLS_DTPMOD32, 38)
  RELOC_NUMBER (R_RISCV_TLS_DTPREL32, 39)
  RELOC_NUMBER (R_RISCV_TLS_DTPMOD64, 40)
  RELOC_NUMBER (R_RISCV_TLS_DTPREL64, 41)
  RELOC_NUMBER (R_RISCV_TLS_TPREL32, 47)
  RELOC_NUMBER (R_RISCV_TLS_TPREL64, 48)
  RELOC_NUMBER (R_RISCV_TLS_PCREL_LO12, 50)
  RELOC_NUMBER (R_RISCV_TLS_GOT_HI20, 51)
  RELOC_NUMBER (R_RISCV_TLS_GOT_LO12, 52)
  RELOC_NUMBER (R_RISCV_TLS_GD_HI20, 53)
  RELOC_NUMBER (R_RISCV_TLS_GD_LO12, 54)
  RELOC_NUMBER (R_RISCV_GLOB_DAT, 57)
  RELOC_NUMBER (R_RISCV_ADD32, 58)
  RELOC_NUMBER (R_RISCV_ADD64, 59)
  RELOC_NUMBER (R_RISCV_SUB32, 60)
  RELOC_NUMBER (R_RISCV_SUB64, 61)
  FAKE_RELOC (R_RISCV_max, 62)
END_RELOC_NUMBERS (R_RISCV_maxext)

/* Processor specific flags for the ELF header e_flags field.  */

/* Custom flag definitions. */

#define EF_RISCV_EXT_MASK 0xffff
#define EF_RISCV_EXT_SH 16
#define E_RISCV_EXT_Xcustom 0x0000
#define E_RISCV_EXT_Xhwacha 0x0001
#define E_RISCV_EXT_RESERVED 0xffff

#define EF_GET_RISCV_EXT(x) \
  ((x >> EF_RISCV_EXT_SH) & EF_RISCV_EXT_MASK)

#define EF_SET_RISCV_EXT(x, ext) \
  do { x |= ((ext & EF_RISCV_EXT_MASK) << EF_RISCV_EXT_SH); } while (0)

#define EF_IS_RISCV_EXT_Xcustom(x) \
  (EF_GET_RISCV_EXT(x) == E_RISCV_EXT_Xcustom)

/* A mapping from extension names to elf flags  */

struct riscv_extension_entry
{
  const char* name;
  unsigned int flag;
};

static const struct riscv_extension_entry riscv_extension_map[] =
{
  {"Xcustom", E_RISCV_EXT_Xcustom},
  {"Xhwacha", E_RISCV_EXT_Xhwacha},
};

/* Given an extension name, return an elf flag. */

static inline const char* riscv_elf_flag_to_name(unsigned int flag)
{
  unsigned int i;

  for (i=0; i<sizeof(riscv_extension_map)/sizeof(riscv_extension_map[0]); i++)
    if (riscv_extension_map[i].flag == flag)
      return riscv_extension_map[i].name;

  return NULL;
}

/* Given an elf flag, return an extension name. */

static inline unsigned int riscv_elf_name_to_flag(const char* name)
{
  unsigned int i;

  for (i=0; i<sizeof(riscv_extension_map)/sizeof(riscv_extension_map[0]); i++)
    if (strcmp(riscv_extension_map[i].name, name) == 0)
      return riscv_extension_map[i].flag;

  return E_RISCV_EXT_Xcustom;
}

/* Processor specific section indices.  These sections do not actually
   exist.  Symbols with a st_shndx field corresponding to one of these
   values have a special meaning.  */

/* Defined and allocated common symbol.  Value is virtual address.  If
   relocated, alignment must be preserved.  */
#define SHN_RISCV_ACOMMON	SHN_LORESERVE

/* Defined and allocated text symbol.  Value is virtual address.
   Occur in the dynamic symbol table of Alpha OSF/1 and Irix 5 executables.  */
#define SHN_RISCV_TEXT		(SHN_LORESERVE + 1)

/* Defined and allocated data symbol.  Value is virtual address.
   Occur in the dynamic symbol table of Alpha OSF/1 and Irix 5 executables.  */
#define SHN_RISCV_DATA		(SHN_LORESERVE + 2)

/* Small common symbol.  */
#define SHN_RISCV_SCOMMON	(SHN_LORESERVE + 3)

/* Small undefined symbol.  */
#define SHN_RISCV_SUNDEFINED	(SHN_LORESERVE + 4)

/* Number of local global offset table entries.  */
#define DT_RISCV_LOCAL_GOTNO	0x70000000

/* Number of entries in the .dynsym section.  */
#define DT_RISCV_SYMTABNO	0x70000001

/* Index of first dynamic symbol in global offset table.  */
#define DT_RISCV_GOTSYM		0x70000002

/* Address of the base of the PLTGOT.  */
#define DT_RISCV_PLTGOT         0x70000003

#endif /* _ELF_RISCV_H */
