/* RISC-V-specific support for ELF.
   Copyright 2011-2015 Free Software Foundation, Inc.

   Contributed by Andrew Waterman (waterman@cs.berkeley.edu) at UC Berkeley.
   Based on TILE-Gx and MIPS targets.

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
   along with this program; see the file COPYING3. If not,
   see <http://www.gnu.org/licenses/>.  */

#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"
#include "elf-bfd.h"
#include "elf/riscv.h"
#include "opcode/riscv.h"
#include "libiberty.h"
#include "elfxx-riscv.h"
#include <stdint.h>

#define MINUS_ONE ((bfd_vma)0 - 1)

/* The relocation table used for SHT_RELA sections.  */

static reloc_howto_type howto_table[] =
{
  /* No relocation.  */
  HOWTO (R_RISCV_NONE,			/* type */
	 0,				/* rightshift */
	 3,				/* size */
	 0,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_NONE",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 0,				/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* 32 bit relocation.  */
  HOWTO (R_RISCV_32,			/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_32",			/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 0xffffffff,			/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* 64 bit relocation.  */
  HOWTO (R_RISCV_64,			/* type */
	 0,				/* rightshift */
	 4,				/* size */
	 64,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_64",			/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 MINUS_ONE,			/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* Relocation against a local symbol in a shared object.  */
  HOWTO (R_RISCV_RELATIVE,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_RELATIVE",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 0xffffffff,			/* dst_mask */
	 FALSE),			/* pcrel_offset */

  HOWTO (R_RISCV_COPY,			/* type */
	 0,				/* rightshift */
	 0,				/* this one is variable size */
	 0,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_bitfield,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_COPY",		/* name */
	 FALSE,				/* partial_inplace */
	 0x0,         			/* src_mask */
	 0x0,		        	/* dst_mask */
	 FALSE),			/* pcrel_offset */

  HOWTO (R_RISCV_JUMP_SLOT,		/* type */
	 0,				/* rightshift */
	 4,				/* size */
	 64,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_bitfield,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_JUMP_SLOT",		/* name */
	 FALSE,				/* partial_inplace */
	 0x0,         			/* src_mask */
	 0x0,		        	/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* Dynamic TLS relocations.  */
  HOWTO (R_RISCV_TLS_DTPMOD32,		/* type */
	 0,				/* rightshift */
	 4,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc, 	/* special_function */
	 "R_RISCV_TLS_DTPMOD32",	/* name */
	 FALSE,				/* partial_inplace */
	 MINUS_ONE,			/* src_mask */
	 MINUS_ONE,			/* dst_mask */
	 FALSE),			/* pcrel_offset */

  HOWTO (R_RISCV_TLS_DTPMOD64,		/* type */
	 0,				/* rightshift */
	 4,				/* size */
	 64,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc, 	/* special_function */
	 "R_RISCV_TLS_DTPMOD64",	/* name */
	 FALSE,				/* partial_inplace */
	 MINUS_ONE,			/* src_mask */
	 MINUS_ONE,			/* dst_mask */
	 FALSE),			/* pcrel_offset */

  HOWTO (R_RISCV_TLS_DTPREL32,		/* type */
	 0,				/* rightshift */
	 4,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc, 	/* special_function */
	 "R_RISCV_TLS_DTPREL32",	/* name */
	 TRUE,				/* partial_inplace */
	 MINUS_ONE,			/* src_mask */
	 MINUS_ONE,			/* dst_mask */
	 FALSE),			/* pcrel_offset */

  HOWTO (R_RISCV_TLS_DTPREL64,		/* type */
	 0,				/* rightshift */
	 4,				/* size */
	 64,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc, 	/* special_function */
	 "R_RISCV_TLS_DTPREL64",	/* name */
	 TRUE,				/* partial_inplace */
	 MINUS_ONE,			/* src_mask */
	 MINUS_ONE,			/* dst_mask */
	 FALSE),			/* pcrel_offset */

  HOWTO (R_RISCV_TLS_TPREL32,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc, 	/* special_function */
	 "R_RISCV_TLS_TPREL32",		/* name */
	 FALSE,				/* partial_inplace */
	 MINUS_ONE,			/* src_mask */
	 MINUS_ONE,			/* dst_mask */
	 FALSE),			/* pcrel_offset */

  HOWTO (R_RISCV_TLS_TPREL64,		/* type */
	 0,				/* rightshift */
	 4,				/* size */
	 64,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc, 	/* special_function */
	 "R_RISCV_TLS_TPREL64",		/* name */
	 FALSE,				/* partial_inplace */
	 MINUS_ONE,			/* src_mask */
	 MINUS_ONE,			/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* Reserved for future relocs that the dynamic linker must understand.  */
  EMPTY_HOWTO (12),
  EMPTY_HOWTO (13),
  EMPTY_HOWTO (14),
  EMPTY_HOWTO (15),

  /* 12-bit PC-relative branch offset.  */
  HOWTO (R_RISCV_BRANCH,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 TRUE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_signed,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_BRANCH",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_SBTYPE_IMM (-1U),	/* dst_mask */
	 TRUE),				/* pcrel_offset */

  /* 20-bit PC-relative jump offset.  */
  HOWTO (R_RISCV_JAL,			/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 TRUE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
					/* This needs complex overflow
					   detection, because the upper 36
					   bits must match the PC + 4.  */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_JAL",			/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_UJTYPE_IMM (-1U),	/* dst_mask */
	 TRUE),				/* pcrel_offset */

  /* 32-bit PC-relative function call (AUIPC/JALR).  */
  HOWTO (R_RISCV_CALL,			/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 64,				/* bitsize */
	 TRUE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_CALL",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_UTYPE_IMM (-1U) | ((bfd_vma) ENCODE_ITYPE_IMM (-1U) << 32),
					/* dst_mask */
	 TRUE),				/* pcrel_offset */

  /* 32-bit PC-relative function call (AUIPC/JALR).  */
  HOWTO (R_RISCV_CALL_PLT,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 64,				/* bitsize */
	 TRUE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_CALL_PLT",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_UTYPE_IMM (-1U) | ((bfd_vma) ENCODE_ITYPE_IMM (-1U) << 32),
					/* dst_mask */
	 TRUE),				/* pcrel_offset */

  /* High 20 bits of 32-bit PC-relative GOT access.  */
  HOWTO (R_RISCV_GOT_HI20,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 TRUE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_GOT_HI20",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_UTYPE_IMM (-1U),	/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* High 20 bits of 32-bit PC-relative TLS IE GOT access.  */
  HOWTO (R_RISCV_TLS_GOT_HI20,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 TRUE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_TLS_GOT_HI20",	/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_UTYPE_IMM (-1U),	/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* High 20 bits of 32-bit PC-relative TLS GD GOT reference.  */
  HOWTO (R_RISCV_TLS_GD_HI20,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 TRUE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_TLS_GD_HI20",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_UTYPE_IMM (-1U),	/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* High 20 bits of 32-bit PC-relative reference.  */
  HOWTO (R_RISCV_PCREL_HI20,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 TRUE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_PCREL_HI20",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_UTYPE_IMM (-1U),	/* dst_mask */
	 TRUE),				/* pcrel_offset */

  /* Low 12 bits of a 32-bit PC-relative load or add.  */
  HOWTO (R_RISCV_PCREL_LO12_I,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_PCREL_LO12_I",	/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_ITYPE_IMM (-1U),	/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* Low 12 bits of a 32-bit PC-relative store.  */
  HOWTO (R_RISCV_PCREL_LO12_S,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_PCREL_LO12_S",	/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_STYPE_IMM (-1U),	/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* High 20 bits of 32-bit absolute address.  */
  HOWTO (R_RISCV_HI20,			/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_HI20",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_UTYPE_IMM (-1U),	/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* High 12 bits of 32-bit load or add.  */
  HOWTO (R_RISCV_LO12_I,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_LO12_I",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_ITYPE_IMM (-1U),	/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* High 12 bits of 32-bit store.  */
  HOWTO (R_RISCV_LO12_S,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_LO12_S",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_STYPE_IMM (-1U),	/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* High 20 bits of TLS LE thread pointer offset.  */
  HOWTO (R_RISCV_TPREL_HI20,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_signed,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_TPREL_HI20",		/* name */
	 TRUE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_UTYPE_IMM (-1U),	/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* Low 12 bits of TLS LE thread pointer offset for loads and adds.  */
  HOWTO (R_RISCV_TPREL_LO12_I,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_signed,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_TPREL_LO12_I",	/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_ITYPE_IMM (-1U),	/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* Low 12 bits of TLS LE thread pointer offset for stores.  */
  HOWTO (R_RISCV_TPREL_LO12_S,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_signed,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_TPREL_LO12_S",	/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_STYPE_IMM (-1U),	/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* TLS LE thread pointer usage.  */
  HOWTO (R_RISCV_TPREL_ADD,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_TPREL_ADD",		/* name */
	 TRUE,				/* partial_inplace */
	 0,				/* src_mask */
	 0,				/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* 8-bit in-place addition, for local label subtraction.  */
  HOWTO (R_RISCV_ADD8,			/* type */
	 0,				/* rightshift */
	 0,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_ADD8",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 MINUS_ONE,			/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* 16-bit in-place addition, for local label subtraction.  */
  HOWTO (R_RISCV_ADD16,			/* type */
	 0,				/* rightshift */
	 1,				/* size */
	 16,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_ADD16",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 MINUS_ONE,			/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* 32-bit in-place addition, for local label subtraction.  */
  HOWTO (R_RISCV_ADD32,			/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_ADD32",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 MINUS_ONE,			/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* 64-bit in-place addition, for local label subtraction.  */
  HOWTO (R_RISCV_ADD64,			/* type */
	 0,				/* rightshift */
	 4,				/* size */
	 64,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_ADD64",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 MINUS_ONE,			/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* 8-bit in-place addition, for local label subtraction.  */
  HOWTO (R_RISCV_SUB8,			/* type */
	 0,				/* rightshift */
	 0,				/* size */
	 8,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_SUB8",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 MINUS_ONE,			/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* 16-bit in-place addition, for local label subtraction.  */
  HOWTO (R_RISCV_SUB16,			/* type */
	 0,				/* rightshift */
	 1,				/* size */
	 16,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_SUB16",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 MINUS_ONE,			/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* 32-bit in-place addition, for local label subtraction.  */
  HOWTO (R_RISCV_SUB32,			/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_SUB32",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 MINUS_ONE,			/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* 64-bit in-place addition, for local label subtraction.  */
  HOWTO (R_RISCV_SUB64,			/* type */
	 0,				/* rightshift */
	 4,				/* size */
	 64,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_SUB64",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 MINUS_ONE,			/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* GNU extension to record C++ vtable hierarchy */
  HOWTO (R_RISCV_GNU_VTINHERIT,		/* type */
	 0,				/* rightshift */
	 4,				/* size */
	 0,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 NULL,				/* special_function */
	 "R_RISCV_GNU_VTINHERIT",	/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 0,				/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* GNU extension to record C++ vtable member usage */
  HOWTO (R_RISCV_GNU_VTENTRY,		/* type */
	 0,				/* rightshift */
	 4,				/* size */
	 0,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 _bfd_elf_rel_vtable_reloc_fn,	/* special_function */
	 "R_RISCV_GNU_VTENTRY",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 0,				/* dst_mask */
	 FALSE),			/* pcrel_offset */

  /* Indicates an alignment statement.  The addend field encodes how many
     bytes of NOPs follow the statement.  The desired alignment is the
     addend rounded up to the next power of two.  */
  HOWTO (R_RISCV_ALIGN,			/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 0,				/* bitsize */
	 FALSE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_ALIGN",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 0,				/* dst_mask */
	 TRUE),				/* pcrel_offset */

  /* 8-bit PC-relative branch offset.  */
  HOWTO (R_RISCV_RVC_BRANCH,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 TRUE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_signed,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_RVC_BRANCH",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_RVC_B_IMM (-1U),	/* dst_mask */
	 TRUE),				/* pcrel_offset */

  /* 11-bit PC-relative jump offset.  */
  HOWTO (R_RISCV_RVC_JUMP,		/* type */
	 0,				/* rightshift */
	 2,				/* size */
	 32,				/* bitsize */
	 TRUE,				/* pc_relative */
	 0,				/* bitpos */
	 complain_overflow_dont,	/* complain_on_overflow */
					/* This needs complex overflow
					   detection, because the upper 36
					   bits must match the PC + 4.  */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_RISCV_RVC_JUMP",		/* name */
	 FALSE,				/* partial_inplace */
	 0,				/* src_mask */
	 ENCODE_RVC_J_IMM (-1U),	/* dst_mask */
	 TRUE),				/* pcrel_offset */
};

/* A mapping from BFD reloc types to RISC-V ELF reloc types.  */

struct elf_reloc_map {
  bfd_reloc_code_real_type bfd_val;
  enum elf_riscv_reloc_type elf_val;
};

static const struct elf_reloc_map riscv_reloc_map[] =
{
  { BFD_RELOC_NONE, R_RISCV_NONE },
  { BFD_RELOC_32, R_RISCV_32 },
  { BFD_RELOC_64, R_RISCV_64 },
  { BFD_RELOC_RISCV_ADD8, R_RISCV_ADD8 },
  { BFD_RELOC_RISCV_ADD16, R_RISCV_ADD16 },
  { BFD_RELOC_RISCV_ADD32, R_RISCV_ADD32 },
  { BFD_RELOC_RISCV_ADD64, R_RISCV_ADD64 },
  { BFD_RELOC_RISCV_SUB8, R_RISCV_SUB8 },
  { BFD_RELOC_RISCV_SUB16, R_RISCV_SUB16 },
  { BFD_RELOC_RISCV_SUB32, R_RISCV_SUB32 },
  { BFD_RELOC_RISCV_SUB64, R_RISCV_SUB64 },
  { BFD_RELOC_CTOR, R_RISCV_64 },
  { BFD_RELOC_12_PCREL, R_RISCV_BRANCH },
  { BFD_RELOC_RISCV_HI20, R_RISCV_HI20 },
  { BFD_RELOC_RISCV_LO12_I, R_RISCV_LO12_I },
  { BFD_RELOC_RISCV_LO12_S, R_RISCV_LO12_S },
  { BFD_RELOC_RISCV_PCREL_LO12_I, R_RISCV_PCREL_LO12_I },
  { BFD_RELOC_RISCV_PCREL_LO12_S, R_RISCV_PCREL_LO12_S },
  { BFD_RELOC_RISCV_CALL, R_RISCV_CALL },
  { BFD_RELOC_RISCV_CALL_PLT, R_RISCV_CALL_PLT },
  { BFD_RELOC_RISCV_PCREL_HI20, R_RISCV_PCREL_HI20 },
  { BFD_RELOC_RISCV_JMP, R_RISCV_JAL },
  { BFD_RELOC_RISCV_GOT_HI20, R_RISCV_GOT_HI20 },
  { BFD_RELOC_RISCV_TLS_DTPMOD32, R_RISCV_TLS_DTPMOD32 },
  { BFD_RELOC_RISCV_TLS_DTPREL32, R_RISCV_TLS_DTPREL32 },
  { BFD_RELOC_RISCV_TLS_DTPMOD64, R_RISCV_TLS_DTPMOD64 },
  { BFD_RELOC_RISCV_TLS_DTPREL64, R_RISCV_TLS_DTPREL64 },
  { BFD_RELOC_RISCV_TLS_TPREL32, R_RISCV_TLS_TPREL32 },
  { BFD_RELOC_RISCV_TLS_TPREL64, R_RISCV_TLS_TPREL64 },
  { BFD_RELOC_RISCV_TPREL_HI20, R_RISCV_TPREL_HI20 },
  { BFD_RELOC_RISCV_TPREL_ADD, R_RISCV_TPREL_ADD },
  { BFD_RELOC_RISCV_TPREL_LO12_S, R_RISCV_TPREL_LO12_S },
  { BFD_RELOC_RISCV_TPREL_LO12_I, R_RISCV_TPREL_LO12_I },
  { BFD_RELOC_RISCV_TLS_GOT_HI20, R_RISCV_TLS_GOT_HI20 },
  { BFD_RELOC_RISCV_TLS_GD_HI20, R_RISCV_TLS_GD_HI20 },
  { BFD_RELOC_RISCV_ALIGN, R_RISCV_ALIGN },
  { BFD_RELOC_RISCV_RVC_BRANCH, R_RISCV_RVC_BRANCH },
  { BFD_RELOC_RISCV_RVC_JUMP, R_RISCV_RVC_JUMP },
};

/* Given a BFD reloc type, return a howto structure.  */

reloc_howto_type *
riscv_reloc_type_lookup (bfd *abfd ATTRIBUTE_UNUSED,
			 bfd_reloc_code_real_type code)
{
  unsigned int i;

  for (i = 0; i < ARRAY_SIZE (riscv_reloc_map); i++)
    if (riscv_reloc_map[i].bfd_val == code)
      return &howto_table[(int) riscv_reloc_map[i].elf_val];

  bfd_set_error (bfd_error_bad_value);
  return NULL;
}

reloc_howto_type *
riscv_reloc_name_lookup (bfd *abfd ATTRIBUTE_UNUSED, const char *r_name)
{
  unsigned int i;

  for (i = 0; i < ARRAY_SIZE (howto_table); i++)
    if (howto_table[i].name && strcasecmp (howto_table[i].name, r_name) == 0)
      return &howto_table[i];

  return NULL;
}

reloc_howto_type *
riscv_elf_rtype_to_howto (unsigned int r_type)
{
  if (r_type >= ARRAY_SIZE (howto_table))
    {
      (*_bfd_error_handler) (_("unrecognized relocation (0x%x)"), r_type);
      bfd_set_error (bfd_error_bad_value);
      return NULL;
    }
  return &howto_table[r_type];
}
