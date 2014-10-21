/* RISC-V-specific support for ELF.
   Copyright 2011-2014 Free Software Foundation, Inc.

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
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

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
  HOWTO (R_RISCV_NONE,		/* type */
	 0,			/* rightshift */
	 0,			/* size (0 = byte, 1 = short, 2 = long) */
	 0,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_RISCV_NONE",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* 32 bit relocation.  */
  HOWTO (R_RISCV_32,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_RISCV_32",		/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* 64 bit relocation.  */
  HOWTO (R_RISCV_64,		/* type */
	 0,			/* rightshift */
	 4,			/* size (0 = byte, 1 = short, 2 = long) */
	 64,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_RISCV_64",		/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 MINUS_ONE,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* Relocation against a local symbol in a shared object.  */
  HOWTO (R_RISCV_RELATIVE,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_RISCV_RELATIVE",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0xffffffff,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_RISCV_COPY,		/* type */
	 0,			/* rightshift */
	 0,			/* this one is variable size */
	 0,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_RISCV_COPY",		/* name */
	 FALSE,			/* partial_inplace */
	 0x0,         		/* src_mask */
	 0x0,		        /* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_RISCV_JUMP_SLOT,	/* type */
	 0,			/* rightshift */
	 4,			/* size (0 = byte, 1 = short, 2 = long) */
	 64,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_bitfield, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_RISCV_JUMP_SLOT",	/* name */
	 FALSE,			/* partial_inplace */
	 0x0,         		/* src_mask */
	 0x0,		        /* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* Dynamic TLS relocations.  */
  HOWTO (R_RISCV_TLS_DTPMOD32,	/* type */
	 0,			/* rightshift */
	 4,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* special_function */
	 "R_RISCV_TLS_DTPMOD32", /* name */
	 FALSE,			/* partial_inplace */
	 MINUS_ONE,		/* src_mask */
	 MINUS_ONE,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_RISCV_TLS_DTPMOD64,	/* type */
	 0,			/* rightshift */
	 4,			/* size (0 = byte, 1 = short, 2 = long) */
	 64,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* special_function */
	 "R_RISCV_TLS_DTPMOD64", /* name */
	 FALSE,			/* partial_inplace */
	 MINUS_ONE,		/* src_mask */
	 MINUS_ONE,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_RISCV_TLS_DTPREL32,	/* type */
	 0,			/* rightshift */
	 4,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* special_function */
	 "R_RISCV_TLS_DTPREL32",	/* name */
	 TRUE,			/* partial_inplace */
	 MINUS_ONE,		/* src_mask */
	 MINUS_ONE,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_RISCV_TLS_DTPREL64,	/* type */
	 0,			/* rightshift */
	 4,			/* size (0 = byte, 1 = short, 2 = long) */
	 64,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* special_function */
	 "R_RISCV_TLS_DTPREL64",	/* name */
	 TRUE,			/* partial_inplace */
	 MINUS_ONE,		/* src_mask */
	 MINUS_ONE,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_RISCV_TLS_TPREL32,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* special_function */
	 "R_RISCV_TLS_TPREL32",	/* name */
	 FALSE,			/* partial_inplace */
	 MINUS_ONE,		/* src_mask */
	 MINUS_ONE,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  HOWTO (R_RISCV_TLS_TPREL64,	/* type */
	 0,			/* rightshift */
	 4,			/* size (0 = byte, 1 = short, 2 = long) */
	 64,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* special_function */
	 "R_RISCV_TLS_TPREL64",	/* name */
	 FALSE,			/* partial_inplace */
	 MINUS_ONE,		/* src_mask */
	 MINUS_ONE,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  EMPTY_HOWTO (12),
  EMPTY_HOWTO (13),
  EMPTY_HOWTO (14),
  EMPTY_HOWTO (15),

  /* 12-bit PC-relative branch offset.  */
  HOWTO (R_RISCV_BRANCH,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_RISCV_BRANCH",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_SBTYPE_IMM(-1U),/* dst_mask */
	 TRUE),			/* pcrel_offset */

  /* 20-bit PC-relative jump offset.  */
  HOWTO (R_RISCV_JAL,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
				/* This needs complex overflow
				   detection, because the upper 36
				   bits must match the PC + 4.  */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_RISCV_JAL",		/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_UJTYPE_IMM(-1U),	/* dst_mask */
	 TRUE),		/* pcrel_offset */

  /* 32-bit PC-relative function call (AUIPC/JALR).  */
  HOWTO (R_RISCV_CALL,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 64,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_RISCV_CALL",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_UTYPE_IMM(-1U) | ((bfd_vma) ENCODE_ITYPE_IMM(-1U) << 32),	/* dst_mask */
	 TRUE),			/* pcrel_offset */

  /* 32-bit PC-relative function call (AUIPC/JALR).  */
  HOWTO (R_RISCV_CALL_PLT,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 64,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_RISCV_CALL_PLT",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_UTYPE_IMM(-1U) | ((bfd_vma) ENCODE_ITYPE_IMM(-1U) << 32),	/* dst_mask */
	 TRUE),			/* pcrel_offset */

  /* High 20 bits of 32-bit PC-relative GOT access.  */
  HOWTO (R_RISCV_GOT_HI20,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_RISCV_GOT_HI20",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_UTYPE_IMM(-1U),	/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* High 20 bits of 32-bit PC-relative TLS IE GOT access.  */
  HOWTO (R_RISCV_TLS_GOT_HI20,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_RISCV_TLS_GOT_HI20",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_UTYPE_IMM(-1U),	/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* High 20 bits of 32-bit PC-relative TLS GD GOT reference.  */
  HOWTO (R_RISCV_TLS_GD_HI20,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_RISCV_TLS_GD_HI20",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_UTYPE_IMM(-1U),	/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* High 20 bits of 32-bit PC-relative reference.  */
  HOWTO (R_RISCV_PCREL_HI20,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 TRUE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_RISCV_PCREL_HI20",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_UTYPE_IMM(-1U),	/* dst_mask */
	 TRUE),			/* pcrel_offset */

  /* Low 12 bits of a 32-bit PC-relative load or add.  */
  HOWTO (R_RISCV_PCREL_LO12_I,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_RISCV_PCREL_LO12_I",/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_ITYPE_IMM(-1U),	/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* Low 12 bits of a 32-bit PC-relative store.  */
  HOWTO (R_RISCV_PCREL_LO12_S,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_RISCV_PCREL_LO12_S",/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_STYPE_IMM(-1U),	/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* High 20 bits of 32-bit absolute address.  */
  HOWTO (R_RISCV_HI20,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_RISCV_HI20",		/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_UTYPE_IMM(-1U),	/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* High 12 bits of 32-bit load or add.  */
  HOWTO (R_RISCV_LO12_I,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_RISCV_LO12_I",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_ITYPE_IMM(-1U),	/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* High 12 bits of 32-bit store.  */
  HOWTO (R_RISCV_LO12_S,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_RISCV_LO12_S",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_STYPE_IMM(-1U),	/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* High 20 bits of TLS IE GOT access in non-PIC code.  */
  HOWTO (R_RISCV_TLS_IE_HI20,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,	/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* special_function */
	 "R_RISCV_TLS_IE_HI20",	/* name */
	 TRUE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_UTYPE_IMM(-1U),	/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* Low 12 bits of TLS IE GOT access in non-PIC code.  */
  HOWTO (R_RISCV_TLS_IE_LO12,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* special_function */
	 "R_RISCV_TLS_IE_LO12_I",/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_ITYPE_IMM(-1U),	/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* TLS IE thread pointer usage.  */
  HOWTO (R_RISCV_TLS_IE_ADD,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 bfd_elf_generic_reloc, /* special_function */
	 "R_RISCV_TLS_IE_ADD",	/* name */
	 TRUE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* TLS IE low-part relocation for relaxation.  */
  HOWTO (R_RISCV_TLS_IE_LO12_I,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* special_function */
	 "R_RISCV_TLS_IE_LO12_I",/* name */
	 TRUE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* TLS IE low-part relocation for relaxation.  */
  HOWTO (R_RISCV_TLS_IE_LO12_S,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* special_function */
	 "R_RISCV_TLS_IE_LO12_S",/* name */
	 TRUE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* High 20 bits of TLS LE thread pointer offset.  */
  HOWTO (R_RISCV_TPREL_HI20,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,	/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* special_function */
	 "R_RISCV_TPREL_HI20",	/* name */
	 TRUE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_UTYPE_IMM(-1U),	/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* Low 12 bits of TLS LE thread pointer offset for loads and adds.  */
  HOWTO (R_RISCV_TPREL_LO12_I,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* special_function */
	 "R_RISCV_TPREL_LO12_I",/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_ITYPE_IMM(-1U),	/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* Low 12 bits of TLS LE thread pointer offset for stores.  */
  HOWTO (R_RISCV_TPREL_LO12_S,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_signed, /* complain_on_overflow */
	 bfd_elf_generic_reloc, /* special_function */
	 "R_RISCV_TPREL_LO12_S",/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 ENCODE_STYPE_IMM(-1U),	/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* TLS LE thread pointer usage.  */
  HOWTO (R_RISCV_TPREL_ADD,	/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 bfd_elf_generic_reloc, /* special_function */
	 "R_RISCV_TPREL_ADD",	/* name */
	 TRUE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* 32 bit in-place addition, for local label subtraction.  */
  HOWTO (R_RISCV_ADD32,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_RISCV_ADD32",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 MINUS_ONE,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* 64 bit in-place addition, for local label subtraction.  */
  HOWTO (R_RISCV_ADD64,		/* type */
	 0,			/* rightshift */
	 4,			/* size (0 = byte, 1 = short, 2 = long) */
	 64,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_RISCV_ADD64",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 MINUS_ONE,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* 32 bit in-place addition, for local label subtraction.  */
  HOWTO (R_RISCV_SUB32,		/* type */
	 0,			/* rightshift */
	 2,			/* size (0 = byte, 1 = short, 2 = long) */
	 32,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_RISCV_SUB32",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 MINUS_ONE,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* 64 bit in-place addition, for local label subtraction.  */
  HOWTO (R_RISCV_SUB64,		/* type */
	 0,			/* rightshift */
	 4,			/* size (0 = byte, 1 = short, 2 = long) */
	 64,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont, /* complain_on_overflow */
	 bfd_elf_generic_reloc,	/* special_function */
	 "R_RISCV_SUB64",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 MINUS_ONE,		/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* GNU extension to record C++ vtable hierarchy */
  HOWTO (R_RISCV_GNU_VTINHERIT, /* type */
	 0,			/* rightshift */
	 4,			/* size (0 = byte, 1 = short, 2 = long) */
	 0,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 NULL,			/* special_function */
	 "R_RISCV_GNU_VTINHERIT", /* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE),		/* pcrel_offset */

  /* GNU extension to record C++ vtable member usage */
  HOWTO (R_RISCV_GNU_VTENTRY,	/* type */
	 0,			/* rightshift */
	 4,			/* size (0 = byte, 1 = short, 2 = long) */
	 0,			/* bitsize */
	 FALSE,			/* pc_relative */
	 0,			/* bitpos */
	 complain_overflow_dont,/* complain_on_overflow */
	 _bfd_elf_rel_vtable_reloc_fn, /* special_function */
	 "R_RISCV_GNU_VTENTRY",	/* name */
	 FALSE,			/* partial_inplace */
	 0,			/* src_mask */
	 0,			/* dst_mask */
	 FALSE),		/* pcrel_offset */
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
  { BFD_RELOC_RISCV_ADD32, R_RISCV_ADD32 },
  { BFD_RELOC_RISCV_ADD64, R_RISCV_ADD64 },
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
  { BFD_RELOC_RISCV_TLS_IE_HI20, R_RISCV_TLS_IE_HI20 },
  { BFD_RELOC_RISCV_TLS_IE_LO12, R_RISCV_TLS_IE_LO12 },
  { BFD_RELOC_RISCV_TLS_IE_ADD, R_RISCV_TLS_IE_ADD },
  { BFD_RELOC_RISCV_TLS_IE_LO12_S, R_RISCV_TLS_IE_LO12_S },
  { BFD_RELOC_RISCV_TLS_IE_LO12_I, R_RISCV_TLS_IE_LO12_I },
  { BFD_RELOC_RISCV_TLS_GOT_HI20, R_RISCV_TLS_GOT_HI20 },
  { BFD_RELOC_RISCV_TLS_GD_HI20, R_RISCV_TLS_GD_HI20 },
};

#define ABI_64_P(abfd) \
  (get_elf_backend_data (abfd)->s->elfclass == ELFCLASS64)

#define RISCV_ELF_LOG_WORD_BYTES(abfd) \
  (ABI_64_P (abfd) ? 3 : 2)

#define RISCV_ELF_WORD_BYTES(abfd) \
  (1 << RISCV_ELF_LOG_WORD_BYTES (abfd))

#define RISCV_ELF_RELA_BYTES(htab)				\
  (ABI_64_P ((info)->output_bfd) ? sizeof (Elf64_External_Rela)	\
   : sizeof (Elf32_External_Rela))

#define ELF_R_TYPE(r_info) \
  ((r_info) & 0xFF)

#define ELF_R_SYM(abfd, r_info) \
  (ABI_64_P (abfd) ? ELF64_R_SYM (r_info) : ELF32_R_SYM (r_info))

#define ELF_R_INFO(abfd, index, type)	\
  (ABI_64_P (abfd) ? ELF64_R_INFO (index, type) \
		   : (bfd_vma) ELF32_R_INFO (index, type))

#define ELF_PUT_WORD(bfd, val, ptr) \
  (ABI_64_P (bfd) ? bfd_put_64 (bfd, val, ptr) : bfd_put_32 (bfd, val, ptr))

/* The name of the dynamic interpreter.  This is put in the .interp
   section.  */

#define ELF64_DYNAMIC_INTERPRETER "/lib/ld.so.1"
#define ELF32_DYNAMIC_INTERPRETER "/lib32/ld.so.1"

/* The RISC-V linker needs to keep track of the number of relocs that it
   decides to copy as dynamic relocs in check_relocs for each symbol.
   This is so that it can later discard them if they are found to be
   unnecessary.  We store the information in a field extending the
   regular ELF linker hash table.  */

struct riscv_elf_dyn_relocs
{
  struct riscv_elf_dyn_relocs *next;

  /* The input section of the reloc.  */
  asection *sec;

  /* Total number of relocs copied for the input section.  */
  bfd_size_type count;

  /* Number of pc-relative relocs copied for the input section.  */
  bfd_size_type pc_count;
};

/* RISC-V ELF linker hash entry.  */

struct riscv_elf_link_hash_entry
{
  struct elf_link_hash_entry elf;

  /* Track dynamic relocs copied for this symbol.  */
  struct riscv_elf_dyn_relocs *dyn_relocs;

#define GOT_UNKNOWN     0
#define GOT_NORMAL      1
#define GOT_TLS_GD      2
#define GOT_TLS_IE      4
  unsigned char tls_type;
};

#define riscv_elf_hash_entry(ent) \
  ((struct riscv_elf_link_hash_entry *)(ent))

struct _bfd_riscv_elf_obj_tdata
{
  struct elf_obj_tdata root;

  /* tls_type for each local got entry.  */
  char *local_got_tls_type;
};

#define _bfd_riscv_elf_tdata(abfd) \
  ((struct _bfd_riscv_elf_obj_tdata *) (abfd)->tdata.any)

#define _bfd_riscv_elf_local_got_tls_type(abfd) \
  (_bfd_riscv_elf_tdata (abfd)->local_got_tls_type)

#define is_riscv_elf(bfd)				\
  (bfd_get_flavour (bfd) == bfd_target_elf_flavour	\
   && elf_tdata (bfd) != NULL				\
   && elf_object_id (bfd) == RISCV_ELF_DATA)

#include "elf/common.h"
#include "elf/internal.h"

struct riscv_elf_link_hash_table
{
  struct elf_link_hash_table elf;

  /* Short-cuts to get to dynamic linker sections.  */
  asection *sdynbss;
  asection *srelbss;

  /* Small local sym to section mapping cache.  */
  struct sym_cache sym_cache;
};


/* Get the RISC-V ELF linker hash table from a link_info structure.  */
#define riscv_elf_hash_table(p) \
  (elf_hash_table_id ((struct elf_link_hash_table *) ((p)->hash)) \
  == RISCV_ELF_DATA ? ((struct riscv_elf_link_hash_table *) ((p)->hash)) : NULL)

static reloc_howto_type *
riscv_elf_rtype_to_howto (unsigned int r_type)
{
  if ((unsigned int)r_type >= ARRAY_SIZE (howto_table))
    {
      (*_bfd_error_handler)(_("unrecognized relocation (0x%x)"), r_type);
      bfd_set_error (bfd_error_bad_value);
      return NULL;
    }
  return &howto_table[r_type];
}

void
riscv_info_to_howto_rela (bfd *abfd ATTRIBUTE_UNUSED,
			  arelent *cache_ptr,
			  Elf_Internal_Rela *dst)
{
  cache_ptr->howto = riscv_elf_rtype_to_howto (ELF_R_TYPE (dst->r_info));
}

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
riscv_reloc_name_lookup (bfd *abfd ATTRIBUTE_UNUSED,
				 const char *r_name)
{
  unsigned int i;

  for (i = 0; i < ARRAY_SIZE (howto_table); i++)
    if (howto_table[i].name && strcasecmp (howto_table[i].name, r_name) == 0)
      return &howto_table[i];

  return NULL;
}

static void
riscv_elf_append_rela (bfd *abfd, asection *s, Elf_Internal_Rela *rel)
{
  const struct elf_backend_data *bed;
  bfd_byte *loc;

  bed = get_elf_backend_data (abfd);
  loc = s->contents + (s->reloc_count++ * bed->s->sizeof_rela);
  bed->s->swap_reloca_out (abfd, rel, loc);
}

/* PLT/GOT stuff */

#define PLT_HEADER_INSNS 8
#define PLT_ENTRY_INSNS 4
#define PLT_HEADER_SIZE (PLT_HEADER_INSNS * 4)
#define PLT_ENTRY_SIZE (PLT_ENTRY_INSNS * 4)

#define GOT_ENTRY_SIZE(info) RISCV_ELF_WORD_BYTES ((info)->output_bfd)

#define GOTPLT_HEADER_SIZE(info) (2 * GOT_ENTRY_SIZE (info))

#define sec_addr(sec) ((sec)->output_section->vma + (sec)->output_offset)

static bfd_vma
riscv_elf_got_plt_val (bfd_vma plt_index, struct bfd_link_info *info)
{
  return sec_addr (riscv_elf_hash_table (info)->elf.sgotplt)
	 + GOTPLT_HEADER_SIZE (info)
	 + (plt_index * GOT_ENTRY_SIZE (info));
}

#define X_V0 16
#define X_V1 17
#define X_T0 26
#define X_T1 27
#define X_T2 28

#define MATCH_LREG(abfd) (ABI_64_P (abfd) ? MATCH_LD : MATCH_LW)
#define MATCH_SREG(abfd) (ABI_64_P (abfd) ? MATCH_SD : MATCH_SW)

/* The format of the first PLT entry.  */

static void
riscv_make_plt0_entry(bfd* abfd, bfd_vma gotplt_addr, bfd_vma addr,
		      uint32_t entry[PLT_HEADER_INSNS])
{
  int regbytes = ABI_64_P(abfd) ? 8 : 4;

  /* auipc  t2, %hi(.got.plt)
     sub    v0, v0, v1               # shifted .got.plt offset + hdr size + 12
     l[w|d] v1, %lo(.got.plt)(t2)    # _dl_runtime_resolve
     addi   v0, v0, -(hdr size + 12) # shifted .got.plt offset
     addi   t0, t2, %lo(.got.plt)    # &.got.plt
     srli   t1, v0, log2(16/PTRSIZE) # .got.plt offset
     l[w|d] t0, PTRSIZE(t0)          # link map
     jr     v1 */

  entry[0] = RISCV_UTYPE(AUIPC, X_T2, RISCV_PCREL_HIGH_PART(gotplt_addr, addr));
  entry[1] = RISCV_RTYPE(SUB, X_V0, X_V0, X_V1);
  entry[2] = RISCV_ITYPE(LREG(abfd), X_V1, X_T2, RISCV_PCREL_LOW_PART(gotplt_addr, addr));
  entry[3] = RISCV_ITYPE(ADDI, X_V0, X_V0, -(PLT_HEADER_SIZE + 12));
  entry[4] = RISCV_ITYPE(ADDI, X_T0, X_T2, RISCV_PCREL_LOW_PART(gotplt_addr, addr));
  entry[5] = RISCV_ITYPE(SRLI, X_T1, X_V0, regbytes == 4 ? 2 : 1);
  entry[6] = RISCV_ITYPE(LREG(abfd), X_T0, X_T0, regbytes);
  entry[7] = RISCV_ITYPE(JALR, 0, X_V1, 0);
}

/* The format of subsequent PLT entries.  */

static void
riscv_make_plt_entry(bfd* abfd, bfd_vma got_address, bfd_vma addr,
		     uint32_t *entry)
{
  /* auipc  v0, %hi(.got.plt entry)
     l[w|d] v1, %lo(.got.plt entry)(v0)
     jalr   v0, v1
     nop */

  entry[0] = RISCV_UTYPE(AUIPC, X_V0, RISCV_PCREL_HIGH_PART(got_address, addr));
  entry[1] = RISCV_ITYPE(LREG(abfd),  X_V1, X_V0, RISCV_PCREL_LOW_PART(got_address, addr));
  entry[2] = RISCV_ITYPE(JALR, X_V0, X_V1, 0);
  entry[3] = RISCV_NOP;
}

/* Create an entry in an RISC-V ELF linker hash table.  */

static struct bfd_hash_entry *
link_hash_newfunc (struct bfd_hash_entry *entry,
		   struct bfd_hash_table *table, const char *string)
{
  /* Allocate the structure if it has not already been allocated by a
     subclass.  */
  if (entry == NULL)
    {
      entry =
	bfd_hash_allocate (table,
			   sizeof (struct riscv_elf_link_hash_entry));
      if (entry == NULL)
	return entry;
    }

  /* Call the allocation method of the superclass.  */
  entry = _bfd_elf_link_hash_newfunc (entry, table, string);
  if (entry != NULL)
    {
      struct riscv_elf_link_hash_entry *eh;

      eh = (struct riscv_elf_link_hash_entry *) entry;
      eh->dyn_relocs = NULL;
      eh->tls_type = GOT_UNKNOWN;
    }

  return entry;
}

/* Create a RISC-V ELF linker hash table.  */

struct bfd_link_hash_table *
riscv_elf_link_hash_table_create (bfd *abfd)
{
  struct riscv_elf_link_hash_table *ret;
  bfd_size_type amt = sizeof (struct riscv_elf_link_hash_table);

  ret = (struct riscv_elf_link_hash_table *) bfd_zmalloc (amt);
  if (ret == NULL)
    return NULL;

  if (!_bfd_elf_link_hash_table_init (&ret->elf, abfd, link_hash_newfunc,
				      sizeof (struct riscv_elf_link_hash_entry),
				      RISCV_ELF_DATA))
    {
      free (ret);
      return NULL;
    }

  return &ret->elf.root;
}

/* Create the .got section.  */

static bfd_boolean
riscv_elf_create_got_section (bfd *abfd, struct bfd_link_info *info)
{
  flagword flags;
  asection *s, *s_got;
  struct elf_link_hash_entry *h;
  const struct elf_backend_data *bed = get_elf_backend_data (abfd);
  struct elf_link_hash_table *htab = elf_hash_table (info);

  /* This function may be called more than once.  */
  s = bfd_get_linker_section (abfd, ".got");
  if (s != NULL)
    return TRUE;

  flags = bed->dynamic_sec_flags;

  s = bfd_make_section_anyway_with_flags (abfd,
					  (bed->rela_plts_and_copies_p
					   ? ".rela.got" : ".rel.got"),
					  (bed->dynamic_sec_flags
					   | SEC_READONLY));
  if (s == NULL
      || ! bfd_set_section_alignment (abfd, s, bed->s->log_file_align))
    return FALSE;
  htab->srelgot = s;

  s = s_got = bfd_make_section_anyway_with_flags (abfd, ".got", flags);
  if (s == NULL
      || !bfd_set_section_alignment (abfd, s, bed->s->log_file_align))
    return FALSE;
  htab->sgot = s;

  /* The first bit of the global offset table is the header.  */
  s->size += bed->got_header_size;

  if (bed->want_got_plt)
    {
      s = bfd_make_section_anyway_with_flags (abfd, ".got.plt", flags);
      if (s == NULL
	  || !bfd_set_section_alignment (abfd, s,
					 bed->s->log_file_align))
	return FALSE;
      htab->sgotplt = s;

      /* Reserve room for the header.  */
      s->size += GOTPLT_HEADER_SIZE (info);
    }

  if (bed->want_got_sym)
    {
      /* Define the symbol _GLOBAL_OFFSET_TABLE_ at the start of the .got
	 section.  We don't do this in the linker script because we don't want
	 to define the symbol if we are not creating a global offset
	 table.  */
      h = _bfd_elf_define_linkage_sym (abfd, info, s_got,
				       "_GLOBAL_OFFSET_TABLE_");
      elf_hash_table (info)->hgot = h;
      if (h == NULL)
	return FALSE;
    }

  return TRUE;
}

/* Create .plt, .rela.plt, .got, .got.plt, .rela.got, .dynbss, and
   .rela.bss sections in DYNOBJ, and set up shortcuts to them in our
   hash table.  */

bfd_boolean
riscv_elf_create_dynamic_sections (bfd *dynobj,
				   struct bfd_link_info *info)
{
  struct riscv_elf_link_hash_table *htab;

  htab = riscv_elf_hash_table (info);
  BFD_ASSERT (htab != NULL);

  if (!riscv_elf_create_got_section (dynobj, info))
    return FALSE;

  if (!_bfd_elf_create_dynamic_sections (dynobj, info))
    return FALSE;

  htab->sdynbss = bfd_get_linker_section (dynobj, ".dynbss");
  if (!info->shared)
    htab->srelbss = bfd_get_linker_section (dynobj, ".rela.bss");

  if (!htab->elf.splt || !htab->elf.srelplt || !htab->sdynbss
      || (!info->shared && !htab->srelbss))
    abort ();

  return TRUE;
}

/* Copy the extra info we tack onto an elf_link_hash_entry.  */

void
riscv_elf_copy_indirect_symbol (struct bfd_link_info *info,
				struct elf_link_hash_entry *dir,
				struct elf_link_hash_entry *ind)
{
  struct riscv_elf_link_hash_entry *edir, *eind;

  edir = (struct riscv_elf_link_hash_entry *) dir;
  eind = (struct riscv_elf_link_hash_entry *) ind;

  if (eind->dyn_relocs != NULL)
    {
      if (edir->dyn_relocs != NULL)
	{
	  struct riscv_elf_dyn_relocs **pp;
	  struct riscv_elf_dyn_relocs *p;

	  /* Add reloc counts against the indirect sym to the direct sym
	     list.  Merge any entries against the same section.  */
	  for (pp = &eind->dyn_relocs; (p = *pp) != NULL; )
	    {
	      struct riscv_elf_dyn_relocs *q;

	      for (q = edir->dyn_relocs; q != NULL; q = q->next)
		if (q->sec == p->sec)
		  {
		    q->pc_count += p->pc_count;
		    q->count += p->count;
		    *pp = p->next;
		    break;
		  }
	      if (q == NULL)
		pp = &p->next;
	    }
	  *pp = edir->dyn_relocs;
	}

      edir->dyn_relocs = eind->dyn_relocs;
      eind->dyn_relocs = NULL;
    }

  if (ind->root.type == bfd_link_hash_indirect
      && dir->got.refcount <= 0)
    {
      edir->tls_type = eind->tls_type;
      eind->tls_type = GOT_UNKNOWN;
    }
  _bfd_elf_link_hash_copy_indirect (info, dir, ind);
}

/* Look through the relocs for a section during the first phase, and
   allocate space in the global offset table or procedure linkage
   table.  */

bfd_boolean
riscv_elf_check_relocs (bfd *abfd, struct bfd_link_info *info,
			asection *sec, const Elf_Internal_Rela *relocs)
{
  struct riscv_elf_link_hash_table *htab;
  Elf_Internal_Shdr *symtab_hdr;
  struct elf_link_hash_entry **sym_hashes;
  const Elf_Internal_Rela *rel;
  asection *sreloc = NULL;

  if (info->relocatable)
    return TRUE;

  htab = riscv_elf_hash_table (info);
  symtab_hdr = &elf_tdata (abfd)->symtab_hdr;
  sym_hashes = elf_sym_hashes (abfd);

  if (htab->elf.dynobj == NULL)
    htab->elf.dynobj = abfd;

  for (rel = relocs; rel < relocs + sec->reloc_count; rel++)
    {
      unsigned int r_type;
      unsigned long r_symndx;
      struct elf_link_hash_entry *h;
      int tls_type, old_tls_type;

      r_symndx = ELF_R_SYM (abfd, rel->r_info);
      r_type = ELF_R_TYPE (rel->r_info);

      if (r_symndx >= NUM_SHDR_ENTRIES (symtab_hdr))
	{
	  (*_bfd_error_handler) (_("%B: bad symbol index: %d"),
				 abfd, r_symndx);
	  return FALSE;
	}

      if (r_symndx < symtab_hdr->sh_info)
	h = NULL;
      else
	{
	  h = sym_hashes[r_symndx - symtab_hdr->sh_info];
	  while (h->root.type == bfd_link_hash_indirect
		 || h->root.type == bfd_link_hash_warning)
	    h = (struct elf_link_hash_entry *) h->root.u.i.link;

	  /* PR15323, ref flags aren't set for references in the same
	     object.  */
	  h->root.non_ir_ref = 1;
	}

      switch (r_type)
	{
	case R_RISCV_TLS_GD_HI20:
	  tls_type = GOT_TLS_GD;
	  goto have_got_reference;

	case R_RISCV_TLS_IE_HI20:
	  if (info->shared)
	    goto illegal_static_reloc;
	  tls_type = GOT_TLS_IE;
	  goto have_got_reference;

	case R_RISCV_TLS_GOT_HI20:
	  if (info->shared)
	    info->flags |= DF_STATIC_TLS;
	  tls_type = GOT_TLS_IE;
	  goto have_got_reference;

	case R_RISCV_GOT_HI20:
	  tls_type = GOT_NORMAL;
	  /* Fall through.  */

	have_got_reference:
	  /* This symbol requires a global offset table entry.  */
	  if (h != NULL)
	    {
	      h->got.refcount += 1;
	      old_tls_type = riscv_elf_hash_entry(h)->tls_type;
	      riscv_elf_hash_entry(h)->tls_type |= tls_type;
	    }
	  else
	    {
	      bfd_signed_vma *local_got_refcounts;

	      /* This is a global offset table entry for a local symbol.  */
	      local_got_refcounts = elf_local_got_refcounts (abfd);
	      if (local_got_refcounts == NULL)
		{
		  bfd_size_type size;

		  size = symtab_hdr->sh_info;
		  size *= (sizeof (bfd_signed_vma) + sizeof(char));
		  local_got_refcounts = ((bfd_signed_vma *)
					 bfd_zalloc (abfd, size));
		  if (local_got_refcounts == NULL)
		    return FALSE;
		  elf_local_got_refcounts (abfd) = local_got_refcounts;
		  _bfd_riscv_elf_local_got_tls_type (abfd)
		    = (char *) (local_got_refcounts + symtab_hdr->sh_info);
		}
	      local_got_refcounts[r_symndx] += 1;
	      old_tls_type = _bfd_riscv_elf_local_got_tls_type (abfd) [r_symndx];
	      _bfd_riscv_elf_local_got_tls_type (abfd) [r_symndx] |= tls_type;
	    }

	  if (((old_tls_type | tls_type) & GOT_NORMAL)
	      && ((old_tls_type | tls_type) & (GOT_TLS_IE | GOT_TLS_GD)))
	    {
	      (*_bfd_error_handler)
		(_("%B: `%s' accessed both as normal and thread local symbol"),
		 abfd, h ? h->root.root.string : "<local>");
	      return FALSE;
	    }

	  if (htab->elf.sgot == NULL)
	    {
	      if (!riscv_elf_create_got_section (htab->elf.dynobj, info))
		return FALSE;
	    }
	  break;

	case R_RISCV_CALL_PLT:
	  /* This symbol requires a procedure linkage table entry.  We
	     actually build the entry in adjust_dynamic_symbol,
	     because this might be a case of linking PIC code without
	     linking in any dynamic objects, in which case we don't
	     need to generate a procedure linkage table after all.  */

	  if (h != NULL)
	    {
	      h->needs_plt = 1;
	      h->plt.refcount += 1;
	    }
	  break;

	case R_RISCV_HI20:
	  if (info->shared
	      && (h == NULL || strcmp (h->root.root.string, "_DYNAMIC") != 0))
	    {
	    illegal_static_reloc:
	      /* Absolute relocs don't ordinarily belong in shared libs, but
		 we make an exception for _DYNAMIC for ld.so's purpose.  */
	      (*_bfd_error_handler)
		(_("%B: relocation %s against `%s' can not be used when making a shared object; recompile with -fPIC"),
		  abfd, howto_table[r_type].name,
		  h != NULL ? h->root.root.string : "a local symbol");
	      bfd_set_error (bfd_error_bad_value);
	      return FALSE;
	    }
	  /* Fall through.  */

	case R_RISCV_PCREL_HI20:
	case R_RISCV_COPY:
	case R_RISCV_JUMP_SLOT:
	case R_RISCV_RELATIVE:
	case R_RISCV_64:
	case R_RISCV_32:
	case R_RISCV_BRANCH:
	case R_RISCV_CALL:
	case R_RISCV_JAL:
	  if (h != NULL)
	    h->non_got_ref = 1;

	  if (h != NULL && !info->shared)
	    {
	      /* We may need a .plt entry if the function this reloc
		 refers to is in a shared lib.  */
	      h->plt.refcount += 1;
	    }

	  /* If we are creating a shared library, and this is a reloc
	     against a global symbol, or a non PC relative reloc
	     against a local symbol, then we need to copy the reloc
	     into the shared library.  However, if we are linking with
	     -Bsymbolic, we do not need to copy a reloc against a
	     global symbol which is defined in an object we are
	     including in the link (i.e., DEF_REGULAR is set).  At
	     this point we have not seen all the input files, so it is
	     possible that DEF_REGULAR is not set now but will be set
	     later (it is never cleared).  In case of a weak definition,
	     DEF_REGULAR may be cleared later by a strong definition in
	     a shared library.  We account for that possibility below by
	     storing information in the relocs_copied field of the hash
	     table entry.  A similar situation occurs when creating
	     shared libraries and symbol visibility changes render the
	     symbol local.

	     If on the other hand, we are creating an executable, we
	     may need to keep relocations for symbols satisfied by a
	     dynamic library if we manage to avoid copy relocs for the
	     symbol.  */
	  if ((info->shared
	       && (sec->flags & SEC_ALLOC) != 0
	       && (! howto_table[r_type].pc_relative
		   || (h != NULL
		       && (! info->symbolic
			   || h->root.type == bfd_link_hash_defweak
			   || !h->def_regular))))
	      || (!info->shared
		  && (sec->flags & SEC_ALLOC) != 0
		  && h != NULL
		  && (h->root.type == bfd_link_hash_defweak
		      || !h->def_regular)))
	    {
	      struct riscv_elf_dyn_relocs *p;
	      struct riscv_elf_dyn_relocs **head;

	      /* When creating a shared object, we must copy these
		 relocs into the output file.  We create a reloc
		 section in dynobj and make room for the reloc.  */
	      if (sreloc == NULL)
		{
		  sreloc = _bfd_elf_make_dynamic_reloc_section
		    (sec, htab->elf.dynobj, RISCV_ELF_LOG_WORD_BYTES (abfd),
		    abfd, /*rela?*/ TRUE);

		  if (sreloc == NULL)
		    return FALSE;
		}

	      /* If this is a global symbol, we count the number of
		 relocations we need for this symbol.  */
	      if (h != NULL)
		head = &((struct riscv_elf_link_hash_entry *) h)->dyn_relocs;
	      else
		{
		  /* Track dynamic relocs needed for local syms too.
		     We really need local syms available to do this
		     easily.  Oh well.  */

		  asection *s;
		  void *vpp;
		  Elf_Internal_Sym *isym;

		  isym = bfd_sym_from_r_symndx (&htab->sym_cache,
						abfd, r_symndx);
		  if (isym == NULL)
		    return FALSE;

		  s = bfd_section_from_elf_index (abfd, isym->st_shndx);
		  if (s == NULL)
		    s = sec;

		  vpp = &elf_section_data (s)->local_dynrel;
		  head = (struct riscv_elf_dyn_relocs **) vpp;
		}

	      p = *head;
	      if (p == NULL || p->sec != sec)
		{
		  bfd_size_type amt = sizeof *p;
		  p = ((struct riscv_elf_dyn_relocs *)
		       bfd_alloc (htab->elf.dynobj, amt));
		  if (p == NULL)
		    return FALSE;
		  p->next = *head;
		  *head = p;
		  p->sec = sec;
		  p->count = 0;
		  p->pc_count = 0;
		}

	      p->count += 1;
	      p->pc_count += howto_table[r_type].pc_relative;
	    }

	  break;

	case R_RISCV_GNU_VTINHERIT:
	  if (!bfd_elf_gc_record_vtinherit (abfd, sec, h, rel->r_offset))
	    return FALSE;
	  break;

	case R_RISCV_GNU_VTENTRY:
	  if (!bfd_elf_gc_record_vtentry (abfd, sec, h, rel->r_addend))
	    return FALSE;
	  break;

	default:
	  break;
	}
    }

  return TRUE;
}

asection *
riscv_elf_gc_mark_hook (asection *sec,
			struct bfd_link_info *info,
			Elf_Internal_Rela *rel,
			struct elf_link_hash_entry *h,
			Elf_Internal_Sym *sym)
{
  if (h != NULL)
    switch (ELF_R_TYPE (rel->r_info))
      {
      case R_RISCV_GNU_VTINHERIT:
      case R_RISCV_GNU_VTENTRY:
	return NULL;
      }

  return _bfd_elf_gc_mark_hook (sec, info, rel, h, sym);
}

/* Update the got entry reference counts for the section being removed.  */
bfd_boolean
riscv_elf_gc_sweep_hook (bfd *abfd, struct bfd_link_info *info,
			 asection *sec, const Elf_Internal_Rela *relocs)
{
  const Elf_Internal_Rela *rel, *relend;
  Elf_Internal_Shdr *symtab_hdr = &elf_symtab_hdr (abfd);
  struct elf_link_hash_entry **sym_hashes = elf_sym_hashes (abfd);
  bfd_signed_vma *local_got_refcounts = elf_local_got_refcounts (abfd);

  if (info->relocatable)
    return TRUE;

  elf_section_data (sec)->local_dynrel = NULL;

  for (rel = relocs, relend = relocs + sec->reloc_count; rel < relend; rel++)
    {
      unsigned long r_symndx;
      struct elf_link_hash_entry *h = NULL;

      r_symndx = ELF_R_SYM (abfd, rel->r_info);
      if (r_symndx >= symtab_hdr->sh_info)
	{
	  struct riscv_elf_link_hash_entry *eh;
	  struct riscv_elf_dyn_relocs **pp;
	  struct riscv_elf_dyn_relocs *p;

	  h = sym_hashes[r_symndx - symtab_hdr->sh_info];
	  while (h->root.type == bfd_link_hash_indirect
		 || h->root.type == bfd_link_hash_warning)
	    h = (struct elf_link_hash_entry *) h->root.u.i.link;
	  eh = (struct riscv_elf_link_hash_entry *) h;
	  for (pp = &eh->dyn_relocs; (p = *pp) != NULL; pp = &p->next)
	    if (p->sec == sec)
	      {
		/* Everything must go for SEC.  */
		*pp = p->next;
		break;
	      }
	}

      switch (ELF_R_TYPE (rel->r_info))
	{
	case R_RISCV_GOT_HI20:
	case R_RISCV_TLS_GOT_HI20:
	case R_RISCV_TLS_GD_HI20:
	case R_RISCV_TLS_IE_HI20:
	  if (h != NULL)
	    {
	      if (h->got.refcount > 0)
		h->got.refcount--;
	    }
	  else
	    {
	      if (local_got_refcounts &&
		  local_got_refcounts[r_symndx] > 0)
		local_got_refcounts[r_symndx]--;
	    }
	  break;

	case R_RISCV_HI20:
	case R_RISCV_PCREL_HI20:
	case R_RISCV_COPY:
	case R_RISCV_JUMP_SLOT:
	case R_RISCV_RELATIVE:
	case R_RISCV_64:
	case R_RISCV_32:
	case R_RISCV_BRANCH:
	case R_RISCV_CALL:
	case R_RISCV_JAL:
	  if (info->shared)
	    break;
	  /* Fall through.  */

	case R_RISCV_CALL_PLT:
	  if (h != NULL)
	    {
	      if (h->plt.refcount > 0)
		h->plt.refcount--;
	    }
	  break;

	default:
	  break;
	}
    }

  return TRUE;
}

/* Adjust a symbol defined by a dynamic object and referenced by a
   regular object.  The current definition is in some section of the
   dynamic object, but we're not including those sections.  We have to
   change the definition to something the rest of the link can
   understand.  */

bfd_boolean
riscv_elf_adjust_dynamic_symbol (struct bfd_link_info *info,
				 struct elf_link_hash_entry *h)
{
  struct riscv_elf_link_hash_table *htab;
  struct riscv_elf_link_hash_entry * eh;
  struct riscv_elf_dyn_relocs *p;
  bfd *dynobj;
  asection *s;

  htab = riscv_elf_hash_table (info);
  BFD_ASSERT (htab != NULL);

  dynobj = htab->elf.dynobj;

  /* Make sure we know what is going on here.  */
  BFD_ASSERT (dynobj != NULL
	      && (h->needs_plt
		  || h->u.weakdef != NULL
		  || (h->def_dynamic
		      && h->ref_regular
		      && !h->def_regular)));

  /* If this is a function, put it in the procedure linkage table.  We
     will fill in the contents of the procedure linkage table later
     (although we could actually do it here).  */
  if (h->type == STT_FUNC || h->needs_plt)
    {
      if (h->plt.refcount <= 0
	  || SYMBOL_CALLS_LOCAL (info, h)
	  || (ELF_ST_VISIBILITY (h->other) != STV_DEFAULT
	      && h->root.type == bfd_link_hash_undefweak))
	{
	  /* This case can occur if we saw a R_RISCV_CALL_PLT reloc in an
	     input file, but the symbol was never referred to by a dynamic
	     object, or if all references were garbage collected.  In such
	     a case, we don't actually need to build a PLT entry.  */
	  h->plt.offset = (bfd_vma) -1;
	  h->needs_plt = 0;
	}

      return TRUE;
    }
  else
    h->plt.offset = (bfd_vma) -1;

  /* If this is a weak symbol, and there is a real definition, the
     processor independent code will have arranged for us to see the
     real definition first, and we can just use the same value.  */
  if (h->u.weakdef != NULL)
    {
      BFD_ASSERT (h->u.weakdef->root.type == bfd_link_hash_defined
		  || h->u.weakdef->root.type == bfd_link_hash_defweak);
      h->root.u.def.section = h->u.weakdef->root.u.def.section;
      h->root.u.def.value = h->u.weakdef->root.u.def.value;
      return TRUE;
    }

  /* This is a reference to a symbol defined by a dynamic object which
     is not a function.  */

  /* If we are creating a shared library, we must presume that the
     only references to the symbol are via the global offset table.
     For such cases we need not do anything here; the relocations will
     be handled correctly by relocate_section.  */
  if (info->shared)
    return TRUE;

  /* If there are no references to this symbol that do not use the
     GOT, we don't need to generate a copy reloc.  */
  if (!h->non_got_ref)
    return TRUE;

  /* If -z nocopyreloc was given, we won't generate them either.  */
  if (info->nocopyreloc)
    {
      h->non_got_ref = 0;
      return TRUE;
    }

  eh = (struct riscv_elf_link_hash_entry *) h;
  for (p = eh->dyn_relocs; p != NULL; p = p->next)
    {
      s = p->sec->output_section;
      if (s != NULL && (s->flags & SEC_READONLY) != 0)
	break;
    }

  /* If we didn't find any dynamic relocs in read-only sections, then
     we'll be keeping the dynamic relocs and avoiding the copy reloc.  */
  if (p == NULL)
    {
      h->non_got_ref = 0;
      return TRUE;
    }

  /* We must allocate the symbol in our .dynbss section, which will
     become part of the .bss section of the executable.  There will be
     an entry for this symbol in the .dynsym section.  The dynamic
     object will contain position independent code, so all references
     from the dynamic object to this symbol will go through the global
     offset table.  The dynamic linker will use the .dynsym entry to
     determine the address it must put in the global offset table, so
     both the dynamic object and the regular object will refer to the
     same memory location for the variable.  */

  /* We must generate a R_RISCV_COPY reloc to tell the dynamic linker
     to copy the initial value out of the dynamic object and into the
     runtime process image.  We need to remember the offset into the
     .rel.bss section we are going to use.  */
  if ((h->root.u.def.section->flags & SEC_ALLOC) != 0 && h->size != 0)
    {
      htab->srelbss->size += RISCV_ELF_RELA_BYTES (info);
      h->needs_copy = 1;
    }

  return _bfd_elf_adjust_dynamic_copy (h, htab->sdynbss);
}

/* Allocate space in .plt, .got and associated reloc sections for
   dynamic relocs.  */

static bfd_boolean
allocate_dynrelocs (struct elf_link_hash_entry *h, void *inf)
{
  struct bfd_link_info *info;
  struct riscv_elf_link_hash_table *htab;
  struct riscv_elf_link_hash_entry *eh;
  struct riscv_elf_dyn_relocs *p;

  if (h->root.type == bfd_link_hash_indirect)
    return TRUE;

  info = (struct bfd_link_info *) inf;
  htab = riscv_elf_hash_table (info);
  BFD_ASSERT (htab != NULL);

  if (htab->elf.dynamic_sections_created
      && h->plt.refcount > 0)
    {
      /* Make sure this symbol is output as a dynamic symbol.
	 Undefined weak syms won't yet be marked as dynamic.  */
      if (h->dynindx == -1
	  && !h->forced_local)
	{
	  if (! bfd_elf_link_record_dynamic_symbol (info, h))
	    return FALSE;
	}

      if (WILL_CALL_FINISH_DYNAMIC_SYMBOL (1, info->shared, h))
	{
	  asection *s = htab->elf.splt;

	  if (s->size == 0)
	    s->size = PLT_HEADER_SIZE;

	  h->plt.offset = s->size;

	  /* Make room for this entry.  */
	  s->size += PLT_ENTRY_SIZE;

	  /* We also need to make an entry in the .got.plt section.  */
	  htab->elf.sgotplt->size += GOT_ENTRY_SIZE (info);

	  /* We also need to make an entry in the .rela.plt section.  */
	  htab->elf.srelplt->size += RISCV_ELF_RELA_BYTES (info);

	  /* If this symbol is not defined in a regular file, and we are
	     not generating a shared library, then set the symbol to this
	     location in the .plt.  This is required to make function
	     pointers compare as equal between the normal executable and
	     the shared library.  */
	  if (! info->shared
	      && !h->def_regular)
	    {
	      h->root.u.def.section = s;
	      h->root.u.def.value = h->plt.offset;
	    }
	}
      else
	{
	  h->plt.offset = (bfd_vma) -1;
	  h->needs_plt = 0;
	}
    }
  else
    {
      h->plt.offset = (bfd_vma) -1;
      h->needs_plt = 0;
    }

  if (h->got.refcount > 0)
    {
      asection *s;
      bfd_boolean dyn;
      int tls_type = riscv_elf_hash_entry(h)->tls_type;

      /* Make sure this symbol is output as a dynamic symbol.
	 Undefined weak syms won't yet be marked as dynamic.  */
      if (h->dynindx == -1
	  && !h->forced_local)
	{
	  if (! bfd_elf_link_record_dynamic_symbol (info, h))
	    return FALSE;
	}

      s = htab->elf.sgot;
      h->got.offset = s->size;
      dyn = htab->elf.dynamic_sections_created;
      if (tls_type & (GOT_TLS_GD | GOT_TLS_IE))
	{
	  /* TLS_GD needs two dynamic relocs and two GOT slots.  */
	  if (tls_type & GOT_TLS_GD)
	    {
	      s->size += 2 * RISCV_ELF_WORD_BYTES (info->output_bfd);
	      htab->elf.srelgot->size += 2 * RISCV_ELF_RELA_BYTES (info);
	    }

	  /* TLS_IE needs one dynamic reloc and one GOT slot.  */
	  if (tls_type & GOT_TLS_IE)
	    {
	      s->size += RISCV_ELF_WORD_BYTES (info->output_bfd);
	      htab->elf.srelgot->size += RISCV_ELF_RELA_BYTES (info);
	    }
	}
      else
	{
	  s->size += RISCV_ELF_WORD_BYTES (info->output_bfd);
	  if (WILL_CALL_FINISH_DYNAMIC_SYMBOL (dyn, info->shared, h))
	    htab->elf.srelgot->size += RISCV_ELF_RELA_BYTES (info);
	}
    }
  else
    h->got.offset = (bfd_vma) -1;

  eh = (struct riscv_elf_link_hash_entry *) h;
  if (eh->dyn_relocs == NULL)
    return TRUE;

  /* In the shared -Bsymbolic case, discard space allocated for
     dynamic pc-relative relocs against symbols which turn out to be
     defined in regular objects.  For the normal shared case, discard
     space for pc-relative relocs that have become local due to symbol
     visibility changes.  */

  if (info->shared)
    {
      if (SYMBOL_CALLS_LOCAL (info, h))
	{
	  struct riscv_elf_dyn_relocs **pp;

	  for (pp = &eh->dyn_relocs; (p = *pp) != NULL; )
	    {
	      p->count -= p->pc_count;
	      p->pc_count = 0;
	      if (p->count == 0)
		*pp = p->next;
	      else
		pp = &p->next;
	    }
	}

      /* Also discard relocs on undefined weak syms with non-default
	 visibility.  */
      if (eh->dyn_relocs != NULL
	  && h->root.type == bfd_link_hash_undefweak)
	{
	  if (ELF_ST_VISIBILITY (h->other) != STV_DEFAULT)
	    eh->dyn_relocs = NULL;

	  /* Make sure undefined weak symbols are output as a dynamic
	     symbol in PIEs.  */
	  else if (h->dynindx == -1
		   && !h->forced_local)
	    {
	      if (! bfd_elf_link_record_dynamic_symbol (info, h))
		return FALSE;
	    }
	}
    }
  else
    {
      /* For the non-shared case, discard space for relocs against
	 symbols which turn out to need copy relocs or are not
	 dynamic.  */

      if (!h->non_got_ref
	  && ((h->def_dynamic
	       && !h->def_regular)
	      || (htab->elf.dynamic_sections_created
		  && (h->root.type == bfd_link_hash_undefweak
		      || h->root.type == bfd_link_hash_undefined))))
	{
	  /* Make sure this symbol is output as a dynamic symbol.
	     Undefined weak syms won't yet be marked as dynamic.  */
	  if (h->dynindx == -1
	      && !h->forced_local)
	    {
	      if (! bfd_elf_link_record_dynamic_symbol (info, h))
		return FALSE;
	    }

	  /* If that succeeded, we know we'll be keeping all the
	     relocs.  */
	  if (h->dynindx != -1)
	    goto keep;
	}

      eh->dyn_relocs = NULL;

    keep: ;
    }

  /* Finally, allocate space.  */
  for (p = eh->dyn_relocs; p != NULL; p = p->next)
    {
      asection *sreloc = elf_section_data (p->sec)->sreloc;
      sreloc->size += p->count * RISCV_ELF_RELA_BYTES (info);
    }

  return TRUE;
}

/* Find any dynamic relocs that apply to read-only sections.  */

static bfd_boolean
readonly_dynrelocs (struct elf_link_hash_entry *h, void *inf)
{
  struct riscv_elf_link_hash_entry *eh;
  struct riscv_elf_dyn_relocs *p;

  eh = (struct riscv_elf_link_hash_entry *) h;
  for (p = eh->dyn_relocs; p != NULL; p = p->next)
    {
      asection *s = p->sec->output_section;

      if (s != NULL && (s->flags & SEC_READONLY) != 0)
	{
	  ((struct bfd_link_info *) inf)->flags |= DF_TEXTREL;

	  /* Short-circuit the traversal.  */
	  return FALSE;
	}
    }
  return TRUE;
}

bfd_boolean
riscv_elf_size_dynamic_sections (bfd *output_bfd, struct bfd_link_info *info)
{
  struct riscv_elf_link_hash_table *htab;
  bfd *dynobj;
  asection *s;
  bfd *ibfd;

  htab = riscv_elf_hash_table (info);
  BFD_ASSERT (htab != NULL);
  dynobj = htab->elf.dynobj;
  BFD_ASSERT (dynobj != NULL);

  if (elf_hash_table (info)->dynamic_sections_created)
    {
      /* Set the contents of the .interp section to the interpreter.  */
      if (info->executable)
	{
	  const char *interpreter = ABI_64_P (output_bfd)
	    ? ELF64_DYNAMIC_INTERPRETER : ELF32_DYNAMIC_INTERPRETER;
	  s = bfd_get_linker_section (dynobj, ".interp");
	  BFD_ASSERT (s != NULL);
	  s->size = strlen (interpreter) + 1;
	  s->contents = (unsigned char *) interpreter;
	}
    }

  /* Set up .got offsets for local syms, and space for local dynamic
     relocs.  */
  for (ibfd = info->input_bfds; ibfd != NULL; ibfd = ibfd->link_next)
    {
      bfd_signed_vma *local_got;
      bfd_signed_vma *end_local_got;
      char *local_tls_type;
      bfd_size_type locsymcount;
      Elf_Internal_Shdr *symtab_hdr;
      asection *srel;

      if (! is_riscv_elf (ibfd))
	continue;

      for (s = ibfd->sections; s != NULL; s = s->next)
	{
	  struct riscv_elf_dyn_relocs *p;

	  for (p = elf_section_data (s)->local_dynrel; p != NULL; p = p->next)
	    {
	      if (!bfd_is_abs_section (p->sec)
		  && bfd_is_abs_section (p->sec->output_section))
		{
		  /* Input section has been discarded, either because
		     it is a copy of a linkonce section or due to
		     linker script /DISCARD/, so we'll be discarding
		     the relocs too.  */
		}
	      else if (p->count != 0)
		{
		  srel = elf_section_data (p->sec)->sreloc;
		  srel->size += p->count * RISCV_ELF_RELA_BYTES (info);
		  if ((p->sec->output_section->flags & SEC_READONLY) != 0)
		    info->flags |= DF_TEXTREL;
		}
	    }
	}

      local_got = elf_local_got_refcounts (ibfd);
      if (!local_got)
	continue;

      symtab_hdr = &elf_symtab_hdr (ibfd);
      locsymcount = symtab_hdr->sh_info;
      end_local_got = local_got + locsymcount;
      local_tls_type = _bfd_riscv_elf_local_got_tls_type (ibfd);
      s = htab->elf.sgot;
      srel = htab->elf.srelgot;
      for (; local_got < end_local_got; ++local_got, ++local_tls_type)
	{
	  if (*local_got > 0)
	    {
	      *local_got = s->size;
	      s->size += RISCV_ELF_WORD_BYTES (output_bfd);
	      if (*local_tls_type & GOT_TLS_GD)
		s->size += RISCV_ELF_WORD_BYTES (output_bfd);
	      if (info->shared
		  || (*local_tls_type & (GOT_TLS_GD | GOT_TLS_IE)))
		srel->size += RISCV_ELF_RELA_BYTES (info);
	    }
	  else
	    *local_got = (bfd_vma) -1;
	}
    }

  /* Allocate global sym .plt and .got entries, and space for global
     sym dynamic relocs.  */
  elf_link_hash_traverse (&htab->elf, allocate_dynrelocs, info);

  if (htab->elf.sgotplt)
    {
      struct elf_link_hash_entry *got;
      got = elf_link_hash_lookup (elf_hash_table (info),
				  "_GLOBAL_OFFSET_TABLE_",
				  FALSE, FALSE, FALSE);

      /* Don't allocate .got.plt section if there are no GOT nor PLT
	 entries and there is no refeence to _GLOBAL_OFFSET_TABLE_.  */
      if ((got == NULL
	   || !got->ref_regular_nonweak)
	  && (htab->elf.sgotplt->size
	      == (unsigned)GOTPLT_HEADER_SIZE (info))
	  && (htab->elf.splt == NULL
	      || htab->elf.splt->size == 0)
	  && (htab->elf.sgot == NULL
	      || (htab->elf.sgot->size
		  == get_elf_backend_data (output_bfd)->got_header_size)))
	htab->elf.sgotplt->size = 0;
    }

  /* The check_relocs and adjust_dynamic_symbol entry points have
     determined the sizes of the various dynamic sections.  Allocate
     memory for them.  */
  for (s = dynobj->sections; s != NULL; s = s->next)
    {
      if ((s->flags & SEC_LINKER_CREATED) == 0)
	continue;

      if (s == htab->elf.splt
	  || s == htab->elf.sgot
	  || s == htab->elf.sgotplt
	  || s == htab->sdynbss)
	{
	  /* Strip this section if we don't need it; see the
	     comment below.  */
	}
      else if (strncmp (s->name, ".rela", 5) == 0)
	{
	  if (s->size != 0)
	    {
	      /* We use the reloc_count field as a counter if we need
		 to copy relocs into the output file.  */
	      s->reloc_count = 0;
	    }
	}
      else
	{
	  /* It's not one of our sections.  */
	  continue;
	}

      if (s->size == 0)
	{
	  /* If we don't need this section, strip it from the
	     output file.  This is mostly to handle .rela.bss and
	     .rela.plt.  We must create both sections in
	     create_dynamic_sections, because they must be created
	     before the linker maps input sections to output
	     sections.  The linker does that before
	     adjust_dynamic_symbol is called, and it is that
	     function which decides whether anything needs to go
	     into these sections.  */
	  s->flags |= SEC_EXCLUDE;
	  continue;
	}

      if ((s->flags & SEC_HAS_CONTENTS) == 0)
	continue;

      /* Allocate memory for the section contents.  Zero the memory
	 for the benefit of .rela.plt, which has 4 unused entries
	 at the beginning, and we don't want garbage.  */
      s->contents = (bfd_byte *) bfd_zalloc (dynobj, s->size);
      if (s->contents == NULL)
	return FALSE;
    }

  if (elf_hash_table (info)->dynamic_sections_created)
    {
      /* Add some entries to the .dynamic section.  We fill in the
	 values later, in riscv_elf_finish_dynamic_sections, but we
	 must add the entries now so that we get the correct size for
	 the .dynamic section.  The DT_DEBUG entry is filled in by the
	 dynamic linker and used by the debugger.  */
#define add_dynamic_entry(TAG, VAL) \
  _bfd_elf_add_dynamic_entry (info, TAG, VAL)

      if (info->executable)
	{
	  if (!add_dynamic_entry (DT_DEBUG, 0))
	    return FALSE;
	}

      if (htab->elf.srelplt->size != 0)
	{
	  if (!add_dynamic_entry (DT_PLTGOT, 0)
	      || !add_dynamic_entry (DT_PLTRELSZ, 0)
	      || !add_dynamic_entry (DT_PLTREL, DT_RELA)
	      || !add_dynamic_entry (DT_JMPREL, 0))
	    return FALSE;
	}

      if (!add_dynamic_entry (DT_RELA, 0)
	  || !add_dynamic_entry (DT_RELASZ, 0)
	  || !add_dynamic_entry (DT_RELAENT, RISCV_ELF_RELA_BYTES (info)))
	return FALSE;

      /* If any dynamic relocs apply to a read-only section,
	 then we need a DT_TEXTREL entry.  */
      if ((info->flags & DF_TEXTREL) == 0)
	elf_link_hash_traverse (&htab->elf, readonly_dynrelocs, info);

      if (info->flags & DF_TEXTREL)
	{
	  if (!add_dynamic_entry (DT_TEXTREL, 0))
	    return FALSE;
	}
    }
#undef add_dynamic_entry

  return TRUE;
}

#define TP_OFFSET 0
#define DTP_OFFSET 0x800

/* Return the relocation value for a TLS dtp-relative reloc.  */

static bfd_vma
dtpoff (struct bfd_link_info *info, bfd_vma address)
{
  /* If tls_sec is NULL, we should have signalled an error already.  */
  if (elf_hash_table (info)->tls_sec == NULL)
    return 0;
  return address - elf_hash_table (info)->tls_sec->vma - DTP_OFFSET;
}

/* Return the relocation value for a static TLS tp-relative relocation.  */

static bfd_vma
tpoff (struct bfd_link_info *info, bfd_vma address)
{
  /* If tls_sec is NULL, we should have signalled an error already.  */
  if (elf_hash_table (info)->tls_sec == NULL)
    return 0;
  return address - elf_hash_table (info)->tls_sec->vma - TP_OFFSET;
}

/* Return the global pointer's value, or 0 if it is not in use.  */

static bfd_vma
riscv_global_pointer_value (struct bfd_link_info *info)
{
  struct bfd_link_hash_entry *h;

  h = bfd_link_hash_lookup (info->hash, "_gp", FALSE, FALSE, TRUE);
  if (h == NULL || h->type != bfd_link_hash_defined)
    return 0;

  return h->u.def.value + sec_addr (h->u.def.section);
}

/* Emplace a static relocation.  */

static bfd_reloc_status_type
perform_relocation (const reloc_howto_type *howto,
		    const Elf_Internal_Rela *rel,
		    bfd_vma value,
		    asection *input_section,
		    bfd *input_bfd,
		    bfd_byte *contents)
{
  if (howto->pc_relative)
    value -= sec_addr (input_section) + rel->r_offset;
  value += rel->r_addend;

  switch (ELF_R_TYPE (rel->r_info))
    {
    case R_RISCV_HI20:
    case R_RISCV_TPREL_HI20:
    case R_RISCV_PCREL_HI20:
    case R_RISCV_GOT_HI20:
    case R_RISCV_TLS_GOT_HI20:
    case R_RISCV_TLS_GD_HI20:
    case R_RISCV_TLS_IE_HI20:
      value = ENCODE_UTYPE_IMM (RISCV_CONST_HIGH_PART (value));
      break;

    case R_RISCV_LO12_I:
    case R_RISCV_TPREL_LO12_I:
    case R_RISCV_PCREL_LO12_I:
    case R_RISCV_TLS_IE_LO12:
      value = ENCODE_ITYPE_IMM (value);
      break;

    case R_RISCV_LO12_S:
    case R_RISCV_TPREL_LO12_S:
    case R_RISCV_PCREL_LO12_S:
      value = ENCODE_STYPE_IMM (value);
      break;

    case R_RISCV_CALL:
    case R_RISCV_CALL_PLT:
      if (!VALID_UTYPE_IMM (RISCV_CONST_HIGH_PART (value)))
	return bfd_reloc_overflow;
      value = ENCODE_UTYPE_IMM (RISCV_CONST_HIGH_PART (value))
	      | (ENCODE_ITYPE_IMM (value) << 32);
      break;

    case R_RISCV_JAL:
      if (!VALID_UJTYPE_IMM (value))
	return bfd_reloc_overflow;
      value = ENCODE_UJTYPE_IMM (value);
      break;

    case R_RISCV_BRANCH:
      if (!VALID_SBTYPE_IMM (value))
	return bfd_reloc_overflow;
      value = ENCODE_SBTYPE_IMM (value);
      break;

    case R_RISCV_32:
    case R_RISCV_64:
    case R_RISCV_ADD32:
    case R_RISCV_ADD64:
    case R_RISCV_SUB32:
    case R_RISCV_SUB64:
    case R_RISCV_TLS_DTPREL32:
    case R_RISCV_TLS_DTPREL64:
      break;

    default:
      return bfd_reloc_notsupported;
    }

  bfd_vma word = bfd_get (howto->bitsize, input_bfd, contents + rel->r_offset);
  word = (word & ~howto->dst_mask) | (value & howto->dst_mask);
  bfd_put (howto->bitsize, input_bfd, word, contents + rel->r_offset);

  return bfd_reloc_ok;
}

/* Remember all PC-relative high-part relocs we've encountered to help us
   later resolve the corresponding low-part relocs.  */

typedef struct {
  bfd_vma address;
  bfd_vma value;
} riscv_pcrel_hi_reloc;

typedef struct riscv_pcrel_lo_reloc {
  asection *input_section;
  struct bfd_link_info *info;
  reloc_howto_type *howto;
  const Elf_Internal_Rela *reloc;
  bfd_vma addr;
  const char *name;
  bfd_byte *contents;
  struct riscv_pcrel_lo_reloc *next;
} riscv_pcrel_lo_reloc;

typedef struct {
  htab_t hi_relocs;
  riscv_pcrel_lo_reloc *lo_relocs;
} riscv_pcrel_relocs;

static hashval_t
riscv_pcrel_reloc_hash (const void *entry)
{
  const riscv_pcrel_hi_reloc *e = entry;
  return (hashval_t)(e->address >> 2);
}

static bfd_boolean
riscv_pcrel_reloc_eq (const void *entry1, const void *entry2)
{
  const riscv_pcrel_hi_reloc *e1 = entry1, *e2 = entry2;
  return e1->address == e2->address;
}

static bfd_boolean
riscv_init_pcrel_relocs (riscv_pcrel_relocs *p)
{

  p->lo_relocs = NULL;
  p->hi_relocs = htab_create (1024, riscv_pcrel_reloc_hash,
			      riscv_pcrel_reloc_eq, free);
  return p->hi_relocs != NULL;
}

static void
riscv_free_pcrel_relocs (riscv_pcrel_relocs *p)
{
  riscv_pcrel_lo_reloc *cur = p->lo_relocs;
  while (cur != NULL)
    {
      riscv_pcrel_lo_reloc *next = cur->next;
      free (cur);
      cur = next;
    }

  htab_delete (p->hi_relocs);
}

static bfd_boolean
riscv_record_pcrel_hi_reloc (riscv_pcrel_relocs *p, bfd_vma addr, bfd_vma value)
{
  riscv_pcrel_hi_reloc entry = {addr, value - addr};
  riscv_pcrel_hi_reloc **slot =
    (riscv_pcrel_hi_reloc **) htab_find_slot (p->hi_relocs, &entry, INSERT);
  BFD_ASSERT (*slot == NULL);
  *slot = (riscv_pcrel_hi_reloc *) bfd_malloc (sizeof (riscv_pcrel_hi_reloc));
  if (*slot == NULL)
    return FALSE;
  **slot = entry;
  return TRUE;
}

static bfd_boolean
riscv_record_pcrel_lo_reloc (riscv_pcrel_relocs *p,
			     asection *input_section,
			     struct bfd_link_info *info,
			     reloc_howto_type *howto,
			     const Elf_Internal_Rela *reloc,
			     bfd_vma addr,
			     const char *name,
			     bfd_byte *contents)
{
  riscv_pcrel_lo_reloc *entry;
  entry = (riscv_pcrel_lo_reloc *) bfd_malloc (sizeof (riscv_pcrel_lo_reloc));
  if (entry == NULL)
    return FALSE;
  *entry = (riscv_pcrel_lo_reloc) {input_section, info, howto, reloc, addr,
				   name, contents, p->lo_relocs};
  p->lo_relocs = entry;
  return TRUE;
}

static bfd_boolean
riscv_resolve_pcrel_lo_relocs (riscv_pcrel_relocs *p)
{
  riscv_pcrel_lo_reloc *r;
  for (r = p->lo_relocs; r != NULL; r = r->next)
    {
      bfd *input_bfd = r->input_section->owner;
      riscv_pcrel_hi_reloc search = {r->addr, 0};
      riscv_pcrel_hi_reloc *entry = htab_find (p->hi_relocs, &search);
      if (entry == NULL)
	return ((*r->info->callbacks->reloc_overflow)
		 (r->info, NULL, r->name, r->howto->name, (bfd_vma) 0,
		  input_bfd, r->input_section, r->reloc->r_offset));

      perform_relocation (r->howto, r->reloc, entry->value, r->input_section,
			  input_bfd, r->contents);
    }

  return TRUE;
}

/* Relocate a RISC-V ELF section.

   The RELOCATE_SECTION function is called by the new ELF backend linker
   to handle the relocations for a section.

   The relocs are always passed as Rela structures.

   This function is responsible for adjusting the section contents as
   necessary, and (if generating a relocatable output file) adjusting
   the reloc addend as necessary.

   This function does not have to worry about setting the reloc
   address or the reloc symbol index.

   LOCAL_SYMS is a pointer to the swapped in local symbols.

   LOCAL_SECTIONS is an array giving the section in the input file
   corresponding to the st_shndx field of each local symbol.

   The global hash table entry for the global symbols can be found
   via elf_sym_hashes (input_bfd).

   When generating relocatable output, this function must handle
   STB_LOCAL/STT_SECTION symbols specially.  The output symbol is
   going to be the section symbol corresponding to the output
   section, which means that the addend must be adjusted
   accordingly.  */

bfd_boolean
riscv_elf_relocate_section (bfd *output_bfd, struct bfd_link_info *info,
			    bfd *input_bfd, asection *input_section,
			    bfd_byte *contents, Elf_Internal_Rela *relocs,
			    Elf_Internal_Sym *local_syms,
			    asection **local_sections)
{
  Elf_Internal_Rela *rel;
  Elf_Internal_Rela *relend;
  riscv_pcrel_relocs pcrel_relocs;
  bfd_boolean ret = FALSE;
  asection *sreloc = elf_section_data (input_section)->sreloc;
  struct riscv_elf_link_hash_table *htab = riscv_elf_hash_table (info);
  Elf_Internal_Shdr *symtab_hdr = &elf_symtab_hdr (input_bfd);
  struct elf_link_hash_entry **sym_hashes = elf_sym_hashes (input_bfd);
  bfd_vma *local_got_offsets = elf_local_got_offsets (input_bfd);

  if (!riscv_init_pcrel_relocs (&pcrel_relocs))
    return FALSE;

  relend = relocs + input_section->reloc_count;
  for (rel = relocs; rel < relend; rel++)
    {
      unsigned long r_symndx;
      struct elf_link_hash_entry *h;
      Elf_Internal_Sym *sym;
      asection *sec;
      bfd_vma relocation;
      bfd_reloc_status_type r = bfd_reloc_ok;
      const char *name;
      bfd_vma off, ie_off;
      bfd_boolean unresolved_reloc, is_ie = FALSE;
      bfd_vma pc = sec_addr (input_section) + rel->r_offset;
      int r_type = ELF_R_TYPE (rel->r_info), tls_type;
      reloc_howto_type *howto = riscv_elf_rtype_to_howto (r_type);
      const char *msg = NULL;

      if (r_type == R_RISCV_GNU_VTINHERIT || r_type == R_RISCV_GNU_VTENTRY)
	continue;

      /* This is a final link.  */
      r_symndx = ELF_R_SYM (output_bfd, rel->r_info);
      h = NULL;
      sym = NULL;
      sec = NULL;
      unresolved_reloc = FALSE;
      if (r_symndx < symtab_hdr->sh_info)
	{
	  sym = local_syms + r_symndx;
	  sec = local_sections[r_symndx];
	  relocation = _bfd_elf_rela_local_sym (output_bfd, sym, &sec, rel);
	}
      else
	{
	  bfd_boolean warned;

	  RELOC_FOR_GLOBAL_SYMBOL (info, input_bfd, input_section, rel,
				   r_symndx, symtab_hdr, sym_hashes,
				   h, sec, relocation,
				   unresolved_reloc, warned);
	  if (warned)
	    {
	      /* To avoid generating warning messages about truncated
		 relocations, set the relocation's address to be the same as
		 the start of this section.  */
	      if (input_section->output_section != NULL)
		relocation = input_section->output_section->vma;
	      else
		relocation = 0;
	    }
	}

      if (sec != NULL && discarded_section (sec))
	RELOC_AGAINST_DISCARDED_SECTION (info, input_bfd, input_section,
					 rel, 1, relend, howto, 0, contents);

      if (info->relocatable)
	continue;

      if (h != NULL)
	name = h->root.root.string;
      else
	{
	  name = (bfd_elf_string_from_elf_section
		  (input_bfd, symtab_hdr->sh_link, sym->st_name));
	  if (name == NULL || *name == '\0')
	    name = bfd_section_name (input_bfd, sec);
	}

      switch (r_type)
	{
	case R_RISCV_NONE:
	case R_RISCV_TPREL_ADD:
	case R_RISCV_TLS_IE_ADD:
	case R_RISCV_TLS_IE_LO12_I:
	case R_RISCV_TLS_IE_LO12_S:
	case R_RISCV_COPY:
	case R_RISCV_JUMP_SLOT:
	case R_RISCV_RELATIVE:
	  /* These require nothing of us at all.  */
	  continue;

	case R_RISCV_BRANCH:
	case R_RISCV_HI20:
	  /* These require no special handling beyond perform_relocation.  */
	  break;

	case R_RISCV_GOT_HI20:
	  if (h != NULL)
	    {
	      bfd_boolean dyn;

	      off = h->got.offset;
	      BFD_ASSERT (off != (bfd_vma) -1);
	      dyn = elf_hash_table (info)->dynamic_sections_created;

	      if (! WILL_CALL_FINISH_DYNAMIC_SYMBOL (dyn, info->shared, h)
		  || (info->shared
		      && SYMBOL_REFERENCES_LOCAL (info, h)))
		{
		  /* This is actually a static link, or it is a
		     -Bsymbolic link and the symbol is defined
		     locally, or the symbol was forced to be local
		     because of a version file.  We must initialize
		     this entry in the global offset table.  Since the
		     offset must always be a multiple of the word size,
		     we use the least significant bit to record whether
		     we have initialized it already.

		     When doing a dynamic link, we create a .rela.got
		     relocation entry to initialize the value.  This
		     is done in the finish_dynamic_symbol routine.  */
		  if ((off & 1) != 0)
		    off &= ~1;
		  else
		    {
		      ELF_PUT_WORD (output_bfd, relocation,
				    htab->elf.sgot->contents + off);
		      h->got.offset |= 1;
		    }
		}
	      else
		unresolved_reloc = FALSE;
	    }
	  else
	    {
	      BFD_ASSERT (local_got_offsets != NULL
			  && local_got_offsets[r_symndx] != (bfd_vma) -1);

	      off = local_got_offsets[r_symndx];

	      /* The offset must always be a multiple of 8 on 64-bit.
		 We use the least significant bit to record
		 whether we have already processed this entry.  */
	      if ((off & 1) != 0)
		off &= ~1;
	      else
		{
		  if (info->shared)
		    {
		      asection *s;
		      Elf_Internal_Rela outrel;

		      /* We need to generate a R_RISCV_RELATIVE reloc
			 for the dynamic linker.  */
		      s = htab->elf.srelgot;
		      BFD_ASSERT (s != NULL);

		      outrel.r_offset = sec_addr (htab->elf.sgot) + off;
		      outrel.r_info =
			ELF_R_INFO (output_bfd, 0, R_RISCV_RELATIVE);
		      outrel.r_addend = relocation;
		      relocation = 0;
		      riscv_elf_append_rela (output_bfd, s, &outrel);
		    }

		  ELF_PUT_WORD (output_bfd, relocation,
				htab->elf.sgot->contents + off);
		  local_got_offsets[r_symndx] |= 1;
		}
	    }
	  relocation = sec_addr (htab->elf.sgot) + off;
	  if (!riscv_record_pcrel_hi_reloc (&pcrel_relocs, pc, relocation))
	    r = bfd_reloc_overflow;
	  break;

	case R_RISCV_ADD32:
	case R_RISCV_ADD64:
	  {
	    bfd_vma old_value = bfd_get (howto->bitsize, input_bfd,
					 contents + rel->r_offset);
	    relocation = old_value + relocation;
	  }
	  break;

	case R_RISCV_SUB32:
	case R_RISCV_SUB64:
	  {
	    bfd_vma old_value = bfd_get (howto->bitsize, input_bfd,
					 contents + rel->r_offset);
	    relocation = old_value - relocation;
	  }
	  break;

	case R_RISCV_CALL_PLT:
	case R_RISCV_CALL:
	case R_RISCV_JAL:
	  if (info->shared && h != NULL && h->plt.offset != MINUS_ONE)
	    {
	      /* Refer to the PLT entry.  */
	      relocation = sec_addr (htab->elf.splt) + h->plt.offset;
	      unresolved_reloc = FALSE;
	    }
	  break;

	case R_RISCV_TPREL_HI20:
	  relocation = tpoff (info, relocation);
	  break;

	case R_RISCV_TPREL_LO12_I:
	case R_RISCV_TPREL_LO12_S:
	  relocation = tpoff (info, relocation);
	  if (VALID_ITYPE_IMM (relocation + rel->r_addend))
	    {
	      /* We can use tp as the base register.  */
	      bfd_vma insn = bfd_get_32 (input_bfd, contents + rel->r_offset);
	      insn &= ~(OP_MASK_RS1 << OP_SH_RS1);
	      insn |= TP_REG << OP_SH_RS1;
	      bfd_put_32 (input_bfd, insn, contents + rel->r_offset);
	    }
	  break;

	case R_RISCV_LO12_I:
	case R_RISCV_LO12_S:
	  {
	    bfd_vma gp = riscv_global_pointer_value (info);
	    if (VALID_ITYPE_IMM (relocation + rel->r_addend - gp))
	      {
		/* We can use gp or x0 as the base register.  */
		rel->r_addend -= gp;
		bfd_vma insn = bfd_get_32 (input_bfd, contents + rel->r_offset);
		insn &= ~(OP_MASK_RS1 << OP_SH_RS1);
		if (gp != 0)
		  insn |= GP_REG << OP_SH_RS1;
		bfd_put_32 (input_bfd, insn, contents + rel->r_offset);
	      }
	    break;
	  }

	case R_RISCV_PCREL_HI20:
	  if (!riscv_record_pcrel_hi_reloc (&pcrel_relocs, pc,
					    relocation + rel->r_addend))
	    r = bfd_reloc_overflow;
	  break;

	case R_RISCV_PCREL_LO12_I:
	case R_RISCV_PCREL_LO12_S:
	  if (riscv_record_pcrel_lo_reloc (&pcrel_relocs, input_section, info,
					   howto, rel, relocation, name,
					   contents))
	    continue;
	  r = bfd_reloc_overflow;
	  break;

	case R_RISCV_TLS_DTPREL32:
	case R_RISCV_TLS_DTPREL64:
	  relocation = dtpoff (info, relocation);
	  break;

	case R_RISCV_32:
	case R_RISCV_64:
	  if ((input_section->flags & SEC_ALLOC) == 0)
	    break;

	  if ((info->shared
	       && (h == NULL
		   || ELF_ST_VISIBILITY (h->other) == STV_DEFAULT
		   || h->root.type != bfd_link_hash_undefweak)
	       && (! howto->pc_relative
		   || !SYMBOL_CALLS_LOCAL (info, h)))
	      || (!info->shared
		  && h != NULL
		  && h->dynindx != -1
		  && !h->non_got_ref
		  && ((h->def_dynamic
		       && !h->def_regular)
		      || h->root.type == bfd_link_hash_undefweak
		      || h->root.type == bfd_link_hash_undefined)))
	    {
	      Elf_Internal_Rela outrel;
	      bfd_boolean skip;

	      /* When generating a shared object, these relocations
		 are copied into the output file to be resolved at run
		 time.  */

	      outrel.r_offset =
		_bfd_elf_section_offset (output_bfd, info, input_section,
					 rel->r_offset);
	      skip = outrel.r_offset >= (bfd_vma) -2;
	      outrel.r_offset += sec_addr (input_section);

	      if (skip)
		memset (&outrel, 0, sizeof outrel);
	      else if (h != NULL && h->dynindx != -1
		       && !(info->shared
			    && SYMBOLIC_BIND (info, h)
			    && h->def_regular))
		{
		  outrel.r_info = ELF_R_INFO (output_bfd, h->dynindx, r_type);
		  outrel.r_addend = rel->r_addend;
		}
	      else
		{
		  outrel.r_info = ELF_R_INFO (output_bfd, 0, R_RISCV_RELATIVE);
		  outrel.r_addend = relocation + rel->r_addend;
		}

	      riscv_elf_append_rela (output_bfd, sreloc, &outrel);
	      continue;
	    }
	  break;

	case R_RISCV_TLS_IE_HI20:
	case R_RISCV_TLS_IE_LO12:
	case R_RISCV_TLS_GOT_HI20:
	  is_ie = TRUE;
	  /* Fall through.  */

	case R_RISCV_TLS_GD_HI20:
	  if (h != NULL)
	    {
	      tls_type = riscv_elf_hash_entry(h)->tls_type;
	      off = h->got.offset;
	      h->got.offset |= 1;
	    }
	  else
	    {
	      BFD_ASSERT (local_got_offsets != NULL);
	      tls_type =
		_bfd_riscv_elf_local_got_tls_type (input_bfd) [r_symndx];
	      off = local_got_offsets[r_symndx];
	      local_got_offsets[r_symndx] |= 1;
	    }

	  BFD_ASSERT (tls_type & (GOT_TLS_IE | GOT_TLS_GD));
	  /* If this symbol is referenced by both GD and IE TLS, the IE
	     reference's GOT slot follows the GD reference's slots.  */
	  ie_off = 0;
	  if ((tls_type & GOT_TLS_GD) && (tls_type & GOT_TLS_IE))
	    ie_off = 2 * GOT_ENTRY_SIZE (info);

	  if ((off & 1) != 0)
	    off &= ~1;
	  else
	    {
	      Elf_Internal_Rela outrel;
	      int indx = 0;
	      bfd_boolean need_relocs = FALSE;

	      if (htab->elf.srelgot == NULL)
		abort ();

	      if (h != NULL)
	      {
	        bfd_boolean dyn;
	        dyn = htab->elf.dynamic_sections_created;

		if (WILL_CALL_FINISH_DYNAMIC_SYMBOL (dyn, info->shared, h)
		    && (!info->shared
			|| !SYMBOL_REFERENCES_LOCAL (info, h)))
		  {
		    indx = h->dynindx;
		  }
	      }

	      /* The GOT entries have not been initialized yet.  Do it
	         now, and emit any relocations.  */
	      if ((info->shared || indx != 0)
		  && (h == NULL
		      || ELF_ST_VISIBILITY (h->other) == STV_DEFAULT
		      || h->root.type != bfd_link_hash_undefweak))
		    need_relocs = TRUE;

	      if (tls_type & GOT_TLS_GD)
		{
		  if (need_relocs)
		    {
		      int r1 = ABI_64_P (output_bfd) ? R_RISCV_TLS_DTPMOD64
			: R_RISCV_TLS_DTPMOD32;

		      outrel.r_offset = sec_addr (htab->elf.sgot) + off;
		      outrel.r_addend = 0;
		      outrel.r_info = ELF_R_INFO (output_bfd, indx, r1);
		      ELF_PUT_WORD (output_bfd, 0,
				    htab->elf.sgot->contents + off);
		      riscv_elf_append_rela (output_bfd, htab->elf.srelgot, &outrel);
		      if (indx == 0)
		        {
			  BFD_ASSERT (! unresolved_reloc);
			  ELF_PUT_WORD (output_bfd,
					dtpoff (info, relocation),
					(htab->elf.sgot->contents + off +
					 RISCV_ELF_WORD_BYTES (output_bfd)));
		        }
		      else
		        {
			  r1 = ABI_64_P (output_bfd) ? R_RISCV_TLS_DTPREL64
			     : R_RISCV_TLS_DTPREL32;
			  ELF_PUT_WORD (output_bfd, 0,
					(htab->elf.sgot->contents + off +
					 RISCV_ELF_WORD_BYTES (output_bfd)));
		          outrel.r_info = ELF_R_INFO (output_bfd, indx, r1);
		          outrel.r_offset += RISCV_ELF_WORD_BYTES (output_bfd);
		          riscv_elf_append_rela (output_bfd, htab->elf.srelgot, &outrel);
		        }
		    }
		  else
		    {
		      /* If we are not emitting relocations for a
		         general dynamic reference, then we must be in a
		         static link or an executable link with the
		         symbol binding locally.  Mark it as belonging
		         to module 1, the executable.  */
		      ELF_PUT_WORD (output_bfd, 1,
				    htab->elf.sgot->contents + off);
		      ELF_PUT_WORD (output_bfd,
				    dtpoff (info, relocation),
				    htab->elf.sgot->contents + off +
				    RISCV_ELF_WORD_BYTES (output_bfd));
		   }
		}

	      if (tls_type & GOT_TLS_IE)
		{
		  if (need_relocs)
		    {
		      int r1 = ABI_64_P (output_bfd) ? R_RISCV_TLS_TPREL64
			     : R_RISCV_TLS_TPREL32;

		      ELF_PUT_WORD (output_bfd, 0,
				    htab->elf.sgot->contents + off + ie_off);
		      outrel.r_offset = sec_addr (htab->elf.sgot)
				       + off + ie_off;
		      outrel.r_addend = 0;
		      if (indx == 0)
		        outrel.r_addend = tpoff (info, relocation);
		      outrel.r_info = ELF_R_INFO (output_bfd, indx, r1);
		      riscv_elf_append_rela (output_bfd, htab->elf.srelgot, &outrel);
		    }
		  else
		    {
		      ELF_PUT_WORD (output_bfd, tpoff (info, relocation),
				    htab->elf.sgot->contents + off + ie_off);
		    }
		}
	    }

	  BFD_ASSERT (off < (bfd_vma) -2);
	  relocation = sec_addr (htab->elf.sgot) + off + (is_ie ? ie_off : 0);
	  if (!riscv_record_pcrel_hi_reloc (&pcrel_relocs, pc, relocation))
	    r = bfd_reloc_overflow;
	  unresolved_reloc = FALSE;
	  break;

	default:
	  r = bfd_reloc_notsupported;
	}

      /* Dynamic relocs are not propagated for SEC_DEBUGGING sections
	 because such sections are not SEC_ALLOC and thus ld.so will
	 not process them.  */
      if (unresolved_reloc
	  && !((input_section->flags & SEC_DEBUGGING) != 0
	       && h->def_dynamic)
	  && _bfd_elf_section_offset (output_bfd, info, input_section,
				      rel->r_offset) != (bfd_vma) -1)
	{
	  (*_bfd_error_handler)
	    (_("%B(%A+0x%lx): unresolvable %s relocation against symbol `%s'"),
	     input_bfd,
	     input_section,
	     (long) rel->r_offset,
	     howto->name,
	     h->root.root.string);
	  continue;
	}

      if (r == bfd_reloc_ok)
	r = perform_relocation (howto, rel, relocation, input_section,
				input_bfd, contents);

      switch (r)
	{
	case bfd_reloc_ok:
	  continue;
      
	case bfd_reloc_overflow:
	  r = info->callbacks->reloc_overflow
	    (info, (h ? &h->root : NULL), name, howto->name,
	     (bfd_vma) 0, input_bfd, input_section, rel->r_offset);
	  break;
      
	case bfd_reloc_undefined:
	  r = info->callbacks->undefined_symbol
	    (info, name, input_bfd, input_section, rel->r_offset,
	     TRUE);
	  break;
      
	case bfd_reloc_outofrange:
	  msg = _("internal error: out of range error");
	  break;
      
	case bfd_reloc_notsupported:
	  msg = _("internal error: unsupported relocation error");
	  break;
      
	case bfd_reloc_dangerous:
	  msg = _("internal error: dangerous relocation");
	  break;
      
	default:
	  msg = _("internal error: unknown error");
	  break;
	}
      
      if (msg)
	r = info->callbacks->warning
	  (info, msg, name, input_bfd, input_section, rel->r_offset);
      goto out;
    }

  ret = riscv_resolve_pcrel_lo_relocs (&pcrel_relocs);
out:
  riscv_free_pcrel_relocs (&pcrel_relocs);
  return ret;
}

/* Finish up dynamic symbol handling.  We set the contents of various
   dynamic sections here.  */

bfd_boolean
riscv_elf_finish_dynamic_symbol (bfd *output_bfd,
				 struct bfd_link_info *info,
				 struct elf_link_hash_entry *h,
				 Elf_Internal_Sym *sym)
{
  struct riscv_elf_link_hash_table *htab = riscv_elf_hash_table (info);
  const struct elf_backend_data *bed = get_elf_backend_data (output_bfd);

  if (h->plt.offset != (bfd_vma) -1)
    {
      /* We've decided to create a PLT entry for this symbol.  */
      bfd_byte *loc;
      bfd_vma i, header_address, plt_idx, got_address;
      uint32_t plt_entry[PLT_ENTRY_INSNS];
      Elf_Internal_Rela rela;

      BFD_ASSERT (h->dynindx != -1);

      /* Calculate the address of the PLT header.  */
      header_address = sec_addr (htab->elf.splt);

      /* Calculate the index of the entry.  */
      plt_idx = (h->plt.offset - PLT_HEADER_SIZE) / PLT_ENTRY_SIZE;

      /* Calculate the address of the .got.plt entry.  */
      got_address = riscv_elf_got_plt_val (plt_idx, info);

      /* Find out where the .plt entry should go.  */
      loc = htab->elf.splt->contents + h->plt.offset;

      /* Fill in the PLT entry itself.  */
      riscv_make_plt_entry (output_bfd, got_address,
			    header_address + h->plt.offset, plt_entry);
      for (i = 0; i < PLT_ENTRY_INSNS; i++)
	bfd_put_32 (output_bfd, plt_entry[i], loc + 4*i);

      /* Fill in the initial value of the .got.plt entry.  */
      loc = htab->elf.sgotplt->contents
	    + (got_address - sec_addr (htab->elf.sgotplt));
      ELF_PUT_WORD (output_bfd, sec_addr (htab->elf.splt), loc);

      /* Fill in the entry in the .rela.plt section.  */
      rela.r_offset = got_address;
      rela.r_addend = 0;
      rela.r_info = ELF_R_INFO (output_bfd, h->dynindx, R_RISCV_JUMP_SLOT);

      loc = htab->elf.srelplt->contents + plt_idx * RISCV_ELF_RELA_BYTES (info);
      bed->s->swap_reloca_out (output_bfd, &rela, loc);

      if (!h->def_regular)
	{
	  /* Mark the symbol as undefined, rather than as defined in
	     the .plt section.  Leave the value alone.  */
	  sym->st_shndx = SHN_UNDEF;
	  /* If the symbol is weak, we do need to clear the value.
	     Otherwise, the PLT entry would provide a definition for
	     the symbol even if the symbol wasn't defined anywhere,
	     and so the symbol would never be NULL.  */
	  if (!h->ref_regular_nonweak)
	    sym->st_value = 0;
	}
    }

  if (h->got.offset != (bfd_vma) -1
      && !(riscv_elf_hash_entry(h)->tls_type & (GOT_TLS_GD | GOT_TLS_IE)))
    {
      asection *sgot;
      asection *srela;
      Elf_Internal_Rela rela;

      /* This symbol has an entry in the GOT.  Set it up.  */

      sgot = htab->elf.sgot;
      srela = htab->elf.srelgot;
      BFD_ASSERT (sgot != NULL && srela != NULL);

      rela.r_offset = sec_addr (sgot) + (h->got.offset &~ (bfd_vma) 1);

      /* If this is a -Bsymbolic link, and the symbol is defined
	 locally, we just want to emit a RELATIVE reloc.  Likewise if
	 the symbol was forced to be local because of a version file.
	 The entry in the global offset table will already have been
	 initialized in the relocate_section function.  */
      if (info->shared
	  && (info->symbolic || h->dynindx == -1)
	  && h->def_regular)
	{
	  asection *sec = h->root.u.def.section;
	  rela.r_info = ELF_R_INFO (output_bfd, 0, R_RISCV_RELATIVE);
	  rela.r_addend = (h->root.u.def.value
			   + sec->output_section->vma
			   + sec->output_offset);
	}
      else
	{
	  BFD_ASSERT (h->dynindx != -1);
	  int r_type = ABI_64_P (output_bfd) ? R_RISCV_64 : R_RISCV_32;
	  rela.r_info = ELF_R_INFO (output_bfd, h->dynindx, r_type);
	  rela.r_addend = 0;
	}

      ELF_PUT_WORD (output_bfd, 0,
		    sgot->contents + (h->got.offset & ~(bfd_vma) 1));
      riscv_elf_append_rela (output_bfd, srela, &rela);
    }

  if (h->needs_copy)
    {
      Elf_Internal_Rela rela;

      /* This symbols needs a copy reloc.  Set it up.  */
      BFD_ASSERT (h->dynindx != -1);

      rela.r_offset = sec_addr (h->root.u.def.section) + h->root.u.def.value;
      rela.r_info = ELF_R_INFO (output_bfd, h->dynindx, R_RISCV_COPY);
      rela.r_addend = 0;
      riscv_elf_append_rela (output_bfd, htab->srelbss, &rela);
    }

  /* Mark some specially defined symbols as absolute.  */
  if (h == htab->elf.hdynamic
      || (h == htab->elf.hgot || h == htab->elf.hplt))
    sym->st_shndx = SHN_ABS;

  return TRUE;
}

/* Finish up the dynamic sections.  */

static bfd_boolean
riscv_finish_dyn (bfd *output_bfd, struct bfd_link_info *info,
		  bfd *dynobj, asection *sdyn)
{
  struct riscv_elf_link_hash_table *htab = riscv_elf_hash_table (info);
  const struct elf_backend_data *bed = get_elf_backend_data (output_bfd);
  size_t dynsize = bed->s->sizeof_dyn;
  bfd_byte *dyncon, *dynconend;

  dynconend = sdyn->contents + sdyn->size;
  for (dyncon = sdyn->contents; dyncon < dynconend; dyncon += dynsize)
    {
      Elf_Internal_Dyn dyn;
      asection *s;

      bed->s->swap_dyn_in (dynobj, dyncon, &dyn);

      switch (dyn.d_tag)
	{
	case DT_PLTGOT:
	  s = htab->elf.sgotplt;
	  dyn.d_un.d_ptr = s->output_section->vma + s->output_offset;
	  break;
	case DT_JMPREL:
	  s = htab->elf.srelplt;
	  dyn.d_un.d_ptr = s->output_section->vma + s->output_offset;
	  break;
	case DT_PLTRELSZ:
	  s = htab->elf.srelplt;
	  dyn.d_un.d_val = s->size;
	  break;
	default:
	  continue;
	}

      bed->s->swap_dyn_out (output_bfd, &dyn, dyncon);
    }
  return TRUE;
}

bfd_boolean
riscv_elf_finish_dynamic_sections (bfd *output_bfd,
				   struct bfd_link_info *info)
{
  bfd *dynobj;
  asection *sdyn;
  struct riscv_elf_link_hash_table *htab;

  htab = riscv_elf_hash_table (info);
  BFD_ASSERT (htab != NULL);
  dynobj = htab->elf.dynobj;

  sdyn = bfd_get_linker_section (dynobj, ".dynamic");

  if (elf_hash_table (info)->dynamic_sections_created)
    {
      asection *splt;
      bfd_boolean ret;

      splt = htab->elf.splt;
      BFD_ASSERT (splt != NULL && sdyn != NULL);

      ret = riscv_finish_dyn (output_bfd, info, dynobj, sdyn);

      if (ret != TRUE)
	return ret;

      /* Fill in the head and tail entries in the procedure linkage table.  */
      if (splt->size > 0)
	{
	  int i;
	  uint32_t plt_header[PLT_HEADER_INSNS];
	  riscv_make_plt0_entry (output_bfd, sec_addr (htab->elf.sgotplt),
				 sec_addr (splt), plt_header);

	  for (i = 0; i < PLT_HEADER_INSNS; i++)
	    bfd_put_32 (output_bfd, plt_header[i], splt->contents + 4*i);
	}

      elf_section_data (splt->output_section)->this_hdr.sh_entsize
	= PLT_ENTRY_SIZE;
    }

  if (htab->elf.sgotplt)
    {
      if (bfd_is_abs_section (htab->elf.sgotplt->output_section))
	{
	  (*_bfd_error_handler)
	    (_("discarded output section: `%A'"), htab->elf.sgotplt);
	  return FALSE;
	}

      if (htab->elf.sgotplt->size > 0)
	{
	  /* Write the first two entries in .got.plt, needed for the dynamic
	     linker.  */
	  ELF_PUT_WORD (output_bfd, (bfd_vma) -1, htab->elf.sgotplt->contents);
	  ELF_PUT_WORD (output_bfd, (bfd_vma) 0,
			htab->elf.sgotplt->contents + GOT_ENTRY_SIZE (info));
	}

      elf_section_data (htab->elf.sgotplt->output_section)->this_hdr.sh_entsize =
	GOT_ENTRY_SIZE (info);
    }

  if (htab->elf.sgot)
    {
      if (htab->elf.sgot->size > 0)
	{
	  /* Set the first entry in the global offset table to the address of
	     the dynamic section.  */
	  bfd_vma val = sdyn ? sec_addr (sdyn) : 0;
	  ELF_PUT_WORD (output_bfd, val, htab->elf.sgot->contents);
	}

      elf_section_data (htab->elf.sgot->output_section)->this_hdr.sh_entsize =
	GOT_ENTRY_SIZE (info);
    }

  return TRUE;
}

/* Return address for Ith PLT stub in section PLT, for relocation REL
   or (bfd_vma) -1 if it should not be included.  */

bfd_vma
riscv_elf_plt_sym_val (bfd_vma i, const asection *plt,
		       const arelent *rel ATTRIBUTE_UNUSED)
{
  return plt->vma + PLT_HEADER_SIZE + i * PLT_ENTRY_SIZE;
}

enum elf_reloc_type_class
riscv_reloc_type_class (const struct bfd_link_info *info ATTRIBUTE_UNUSED,
			const asection *rel_sec ATTRIBUTE_UNUSED,
			const Elf_Internal_Rela *rela)
{
  switch (ELF_R_TYPE (rela->r_info))
    {
    case R_RISCV_RELATIVE:
      return reloc_class_relative;
    case R_RISCV_JUMP_SLOT:
      return reloc_class_plt;
    case R_RISCV_COPY:
      return reloc_class_copy;
    default:
      return reloc_class_normal;
    }
}

/* Return true if bfd machine EXTENSION is an extension of machine BASE.  */

static bfd_boolean
riscv_mach_extends_p (unsigned long base, unsigned long extension)
{
  return extension == base;
}

/* Return printable name for ABI.  */

static const char *
riscv_abi_name (bfd *abfd)
{
  return ABI_64_P (abfd) ? "rv64" : "rv32";
}

/* Merge backend specific data from an object file to the output
   object file when linking.  */

bfd_boolean
_bfd_riscv_elf_merge_private_bfd_data (bfd *ibfd, bfd *obfd)
{
  flagword old_flags;
  flagword new_flags;

  if (!is_riscv_elf (ibfd) || !is_riscv_elf (obfd))
    return TRUE;

  if (strcmp (bfd_get_target (ibfd), bfd_get_target (obfd)) != 0)
    {
      (*_bfd_error_handler)
	(_("%B: ABI is incompatible with that of the selected emulation"),
	 ibfd);
      return FALSE;
    }

  if (!_bfd_elf_merge_object_attributes (ibfd, obfd))
    return FALSE;

  new_flags = elf_elfheader (ibfd)->e_flags;
  old_flags = elf_elfheader (obfd)->e_flags;

  if (! elf_flags_init (obfd))
    {
      elf_flags_init (obfd) = TRUE;
      elf_elfheader (obfd)->e_flags = new_flags;
      elf_elfheader (obfd)->e_ident[EI_CLASS]
	= elf_elfheader (ibfd)->e_ident[EI_CLASS];

      if (bfd_get_arch (obfd) == bfd_get_arch (ibfd)
	  && (bfd_get_arch_info (obfd)->the_default
	      || riscv_mach_extends_p (bfd_get_mach (obfd), 
				       bfd_get_mach (ibfd))))
	{
	  if (! bfd_set_arch_mach (obfd, bfd_get_arch (ibfd),
				   bfd_get_mach (ibfd)))
	    return FALSE;
	}

      return TRUE;
    }

  /* Check flag compatibility.  */

  if (new_flags == old_flags)
    return TRUE;

  /* Don't link RV32 and RV64.  */
  if (elf_elfheader (ibfd)->e_ident[EI_CLASS]
      != elf_elfheader (obfd)->e_ident[EI_CLASS])
    {
      (*_bfd_error_handler)
	(_("%B: ABI mismatch: linking %s module with previous %s modules"),
	  ibfd, riscv_abi_name (ibfd), riscv_abi_name (obfd));
      goto fail;
    }

  /* Warn about any other mismatches.  */
  if (new_flags != old_flags)
    {
      if (!EF_IS_RISCV_EXT_Xcustom (new_flags) &&
	  !EF_IS_RISCV_EXT_Xcustom (old_flags))
	{
	  (*_bfd_error_handler)
	    (_("%B: uses different e_flags (0x%lx) fields than previous modules (0x%lx)"),
	    ibfd, (unsigned long) new_flags,
	    (unsigned long) old_flags);
	  goto fail;
	}
      else if (EF_IS_RISCV_EXT_Xcustom(new_flags))
	EF_SET_RISCV_EXT (elf_elfheader (obfd)->e_flags,
			  EF_GET_RISCV_EXT (old_flags));
    }

  return TRUE;

fail:
  bfd_set_error (bfd_error_bad_value);
  return FALSE;
}

/* Delete some bytes from a section while relaxing.  */

static bfd_boolean
riscv_relax_delete_bytes (bfd *abfd, asection *sec, bfd_vma addr, size_t count)
{
  Elf_Internal_Shdr * symtab_hdr;
  unsigned int        sec_shndx;
  bfd_byte *          contents;
  Elf_Internal_Rela * irel;
  Elf_Internal_Rela * irelend;
  Elf_Internal_Sym *  isym;
  Elf_Internal_Sym *  isymend;
  bfd_vma             toaddr;
  unsigned int        symcount;
  struct elf_link_hash_entry ** sym_hashes;
  struct elf_link_hash_entry ** end_hashes;

  /* TODO: handle alignment */
  Elf_Internal_Rela *alignment_rel = NULL;

  sec_shndx = _bfd_elf_section_from_bfd_section (abfd, sec);

  contents = elf_section_data (sec)->this_hdr.contents;

  /* The deletion must stop at the next alignment boundary, if
     ALIGNMENT_REL is non-NULL.  */
  toaddr = sec->size;
  if (alignment_rel)
    toaddr = alignment_rel->r_offset;

  irel = elf_section_data (sec)->relocs;
  irelend = irel + sec->reloc_count;

  /* Actually delete the bytes.  */
  memmove (contents + addr, contents + addr + count,
	   (size_t) (toaddr - addr - count));

  if (alignment_rel)
    {
      size_t i;
      BFD_ASSERT (count % 4 == 0);
      for (i = 0; i < count; i += 4)
	bfd_put_32 (abfd, RISCV_NOP, contents + toaddr - count + i);
      /* TODO: RVC NOP if count % 4 == 2 */
    }
  else
    sec->size -= count;

  /* Adjust all the relocs.  */
  for (irel = elf_section_data (sec)->relocs; irel < irelend; irel++)
    {
      if (irel->r_offset <= addr)
	{
	  if (irel->r_offset + irel->r_addend > addr)
	    irel->r_addend -= ELF_R_SYM (abfd, irel->r_info) ? 0 : count;
	}
      else
	{
	  if (irel->r_offset + irel->r_addend <= addr)
	    irel->r_addend += ELF_R_SYM (abfd, irel->r_info) ? 0 : count;
	  if (irel->r_offset < toaddr)
	    irel->r_offset -= count;
	}
    }

  /* Adjust the local symbols defined in this section.  */
  symtab_hdr = &elf_tdata (abfd)->symtab_hdr;
  isym = (Elf_Internal_Sym *) symtab_hdr->contents;
  isymend = isym + symtab_hdr->sh_info;

  for (; isym < isymend; isym++)
    {
      /* If the symbol is in the range of memory we just moved, we
	 have to adjust its value.  */
      if (isym->st_shndx == sec_shndx
	  && isym->st_value > addr
	  && isym->st_value <= toaddr)
	isym->st_value -= count;

      /* If the symbol *spans* the bytes we just deleted (i.e. its
	 *end* is in the moved bytes but its *start* isn't), then we
	 must adjust its size.  */
      if (isym->st_shndx == sec_shndx
	  && isym->st_value < addr
	  && isym->st_value + isym->st_size > addr
	  && isym->st_value + isym->st_size <= toaddr)
	isym->st_size -= count;
    }

  /* Now adjust the global symbols defined in this section.  */
  symcount = symtab_hdr->sh_size / sizeof(Elf64_External_Sym);
  if (!ABI_64_P (abfd))
    symcount = symtab_hdr->sh_size / sizeof(Elf32_External_Sym);
  symcount -= symtab_hdr->sh_info;

  sym_hashes = elf_sym_hashes (abfd);
  end_hashes = sym_hashes + symcount;

  for (; sym_hashes < end_hashes; sym_hashes++)
    {
      struct elf_link_hash_entry *sym_hash = *sym_hashes;

      if ((sym_hash->root.type == bfd_link_hash_defined
	   || sym_hash->root.type == bfd_link_hash_defweak)
	  && sym_hash->root.u.def.section == sec)
	{
	  /* As above, adjust the value if needed.  */
	  if (sym_hash->root.u.def.value > addr
	      && sym_hash->root.u.def.value < toaddr)
	    sym_hash->root.u.def.value -= count;

	  /* As above, adjust the size if needed.  */
	  if (sym_hash->root.u.def.value < addr
	      && sym_hash->root.u.def.value + sym_hash->size > addr
	      && sym_hash->root.u.def.value + sym_hash->size < toaddr)
	    sym_hash->size -= count;
	}
    }

  return TRUE;
}

static bfd_boolean
_bfd_riscv_relax_call (bfd *abfd, asection *sec,
		       struct bfd_link_info *link_info, bfd_byte *contents,
		       Elf_Internal_Shdr *symtab_hdr,
		       Elf_Internal_Sym *isymbuf,
		       Elf_Internal_Rela *internal_relocs,
		       Elf_Internal_Rela *irel, bfd_vma symval,
		       bfd_boolean *again)
{
  /* See if this function call can be shortened.  */
  bfd_signed_vma foff = symval - (sec_addr (sec) + irel->r_offset);
  bfd_boolean near_zero = !link_info->shared && symval < RISCV_IMM_REACH/2;
  bfd_vma auipc, jalr;
  int r_type;

  if (!VALID_UJTYPE_IMM (foff) && !near_zero)
    return TRUE;

  /* Shorten the function call.  */
  elf_section_data (sec)->relocs = internal_relocs;
  elf_section_data (sec)->this_hdr.contents = contents;
  symtab_hdr->contents = (unsigned char *) isymbuf;

  BFD_ASSERT (irel->r_offset + 8 <= sec->size);

  auipc = bfd_get_32 (abfd, contents + irel->r_offset);
  jalr = bfd_get_32 (abfd, contents + irel->r_offset + 4);

  if (VALID_UJTYPE_IMM (foff))
    {
      /* Relax to JAL rd, addr.  */
      r_type = R_RISCV_JAL;
      auipc = (jalr & (OP_MASK_RD << OP_SH_RD)) | MATCH_JAL;
    }
  else /* near_zero */
    {
      /* Relax to JALR rd, x0, addr.  */
      r_type = R_RISCV_LO12_I;
      auipc = (jalr & (OP_MASK_RD << OP_SH_RD)) | MATCH_JALR;
    }

  /* Replace the R_RISCV_CALL reloc.  */
  irel->r_info = ELF_R_INFO (abfd, ELF_R_SYM (abfd, irel->r_info), r_type);
  /* Replace the AUIPC.  */
  bfd_put_32 (abfd, auipc, contents + irel->r_offset);

  /* Delete unnecessary JALR.  */
  if (! riscv_relax_delete_bytes (abfd, sec, irel->r_offset + 4, 4))
    return FALSE;

  *again = TRUE;
  return TRUE;
}

static bfd_boolean
_bfd_riscv_relax_lui (bfd *abfd, asection *sec,
		      struct bfd_link_info *link_info, bfd_byte *contents,
		      Elf_Internal_Shdr *symtab_hdr,
		      Elf_Internal_Sym *isymbuf,
		      Elf_Internal_Rela *internal_relocs,
		      Elf_Internal_Rela *irel, bfd_vma symval,
		      bfd_boolean *again)
{
  bfd_vma gp = riscv_global_pointer_value (link_info);

  /* See if this symbol is in range of gp.  */
  if (RISCV_CONST_HIGH_PART (symval - gp) != 0)
    return TRUE;

  /* We can delete the unnecessary AUIPC. The corresponding LO12 reloc
     will be converted to GPREL during relocation.  */
  elf_section_data (sec)->relocs = internal_relocs;
  elf_section_data (sec)->this_hdr.contents = contents;
  symtab_hdr->contents = (unsigned char *) isymbuf;

  BFD_ASSERT (irel->r_offset + 4 <= sec->size);
  irel->r_info = ELF_R_INFO (abfd, ELF_R_SYM (abfd, irel->r_info), R_RISCV_NONE);
  if (! riscv_relax_delete_bytes (abfd, sec, irel->r_offset, 4))
    return FALSE;

  *again = TRUE;
  return TRUE;
}

static bfd_boolean
_bfd_riscv_relax_tls_le (bfd *abfd, asection *sec,
			 struct bfd_link_info *link_info, bfd_byte *contents,
			 Elf_Internal_Shdr *symtab_hdr,
			 Elf_Internal_Sym *isymbuf,
			 Elf_Internal_Rela *internal_relocs,
			 Elf_Internal_Rela *irel, bfd_vma symval,
			 bfd_boolean *again)
{
  /* See if this symbol is in range of tp.  */
  if (RISCV_CONST_HIGH_PART (tpoff (link_info, symval)) != 0)
    return TRUE;

  /* We can delete the unnecessary LUI and tp add.  The LO12 reloc will be
     made directly tp-relative.  */
  elf_section_data (sec)->relocs = internal_relocs;
  elf_section_data (sec)->this_hdr.contents = contents;
  symtab_hdr->contents = (unsigned char *) isymbuf;

  BFD_ASSERT (irel->r_offset + 4 <= sec->size);
  irel->r_info = ELF_R_INFO (abfd, ELF_R_SYM (abfd, irel->r_info), R_RISCV_NONE);
  if (! riscv_relax_delete_bytes (abfd, sec, irel->r_offset, 4))
    return FALSE;

  *again = TRUE;
  return TRUE;
}

/* Relax TLS IE to TLS LE.  */
static bfd_boolean
_bfd_riscv_relax_tls_ie (bfd *abfd, asection *sec,
			 bfd_byte *contents,
			 Elf_Internal_Shdr *symtab_hdr,
			 Elf_Internal_Sym *isymbuf,
			 Elf_Internal_Rela *internal_relocs,
			 Elf_Internal_Rela *irel,
			 bfd_boolean *again)
{
  bfd_vma insn;

  elf_section_data (sec)->relocs = internal_relocs;
  elf_section_data (sec)->this_hdr.contents = contents;
  symtab_hdr->contents = (unsigned char *) isymbuf;

  switch (ELF_R_TYPE (irel->r_info))
    {
    case R_RISCV_TLS_IE_HI20:
      /* Replace with R_RISCV_TPREL_HI20.  */
      irel->r_info = ELF_R_INFO (abfd, ELF_R_SYM (abfd, irel->r_info), R_RISCV_TPREL_HI20);
      /* Overwrite AUIPC with LUI.  */
      BFD_ASSERT (irel->r_offset + 4 <= sec->size);
      insn = bfd_get_32 (abfd, contents + irel->r_offset);
      insn = (insn & ~MASK_LUI) | MATCH_LUI;
      bfd_put_32 (abfd, insn, contents + irel->r_offset);
      break;

    case R_RISCV_TLS_IE_LO12:
      /* Just delete the reloc.  */
      irel->r_info = ELF_R_INFO (abfd, ELF_R_SYM (abfd, irel->r_info), R_RISCV_NONE);
      if (! riscv_relax_delete_bytes (abfd, sec, irel->r_offset, 4))
	return FALSE;
      break;

    case R_RISCV_TLS_IE_ADD:
      /* Replace with R_RISCV_TPREL_ADD.  */
      irel->r_info = ELF_R_INFO (abfd, ELF_R_SYM (abfd, irel->r_info), R_RISCV_TPREL_ADD);
      break;

    case R_RISCV_TLS_IE_LO12_I:
      /* Replace with R_RISCV_TPREL_LO12_I.  */
      irel->r_info = ELF_R_INFO (abfd, ELF_R_SYM (abfd, irel->r_info), R_RISCV_TPREL_LO12_I);
      break;

    case R_RISCV_TLS_IE_LO12_S:
      /* Replace with R_RISCV_TPREL_LO12_S.  */
      irel->r_info = ELF_R_INFO (abfd, ELF_R_SYM (abfd, irel->r_info), R_RISCV_TPREL_LO12_S);
      break;

    default:
      abort();
    }

  *again = TRUE;
  return TRUE;
}

/* Relax AUIPC/JALR into JAL.  */

bfd_boolean
_bfd_riscv_relax_section (bfd *abfd, asection *sec,
			  struct bfd_link_info *link_info, bfd_boolean *again)
{
  Elf_Internal_Shdr *symtab_hdr;
  Elf_Internal_Rela *internal_relocs;
  Elf_Internal_Rela *irel, *irelend;
  bfd_byte *contents = NULL;
  Elf_Internal_Sym *isymbuf = NULL;
  struct riscv_elf_link_hash_table *htab;

  htab = riscv_elf_hash_table (link_info);

  *again = FALSE;

  if (link_info->relocatable
      || (sec->flags & SEC_RELOC) == 0
      || sec->reloc_count == 0)
    return TRUE;

  symtab_hdr = &elf_symtab_hdr (abfd);

  internal_relocs = (_bfd_elf_link_read_relocs
		     (abfd, sec, NULL, (Elf_Internal_Rela *) NULL,
		      link_info->keep_memory));
  if (internal_relocs == NULL)
    goto error_return;

  irelend = internal_relocs + sec->reloc_count;
  for (irel = internal_relocs; irel < irelend; irel++)
    {
      bfd_vma symval;
      int type = ELF_R_TYPE (irel->r_info);
      bfd_boolean call = type == R_RISCV_CALL || type == R_RISCV_CALL_PLT;
      bfd_boolean lui = type == R_RISCV_HI20;
      bfd_boolean tls_le = type == R_RISCV_TPREL_HI20 || type == R_RISCV_TPREL_ADD;
      bfd_boolean tls_ie = type == R_RISCV_TLS_IE_HI20 || type == R_RISCV_TLS_IE_LO12 || type == R_RISCV_TLS_IE_ADD || type == R_RISCV_TLS_IE_LO12_I || type == R_RISCV_TLS_IE_LO12_S;

      if (!(call || lui || tls_le || tls_ie))
	continue;

      /* Get the section contents.  */
      if (contents == NULL)
	{
	  if (elf_section_data (sec)->this_hdr.contents != NULL)
	    contents = elf_section_data (sec)->this_hdr.contents;
	  else
	    {
	      if (!bfd_malloc_and_get_section (abfd, sec, &contents))
		goto error_return;
	    }
	}

      /* Read this BFD's symbols if we haven't done so already.  */
      if (isymbuf == NULL && symtab_hdr->sh_info != 0)
	{
	  isymbuf = (Elf_Internal_Sym *) symtab_hdr->contents;
	  if (isymbuf == NULL)
	    isymbuf = bfd_elf_get_elf_syms (abfd, symtab_hdr,
					    symtab_hdr->sh_info, 0,
					    NULL, NULL, NULL);
	  if (isymbuf == NULL)
	    goto error_return;
	}

      /* Get the value of the symbol referred to by the reloc.  */
      if (ELF_R_SYM (abfd, irel->r_info) < symtab_hdr->sh_info)
	{
	  /* A local symbol.  */
	  Elf_Internal_Sym *isym = isymbuf + ELF_R_SYM (abfd, irel->r_info);

	  if (isym->st_shndx == SHN_UNDEF)
	    symval = sec_addr (sec) + irel->r_offset;
	  else
	    {
	      asection *isec;
	      BFD_ASSERT (isym->st_shndx < elf_numsections (abfd));
	      isec = elf_elfsections (abfd)[isym->st_shndx]->bfd_section;
	      symval = sec_addr (isec) + isym->st_value;
	    }
	}
      else
	{
	  unsigned long indx;
	  struct elf_link_hash_entry *h;

	  indx = ELF_R_SYM (abfd, irel->r_info) - symtab_hdr->sh_info;
	  h = elf_sym_hashes (abfd)[indx];

	  while (h->root.type == bfd_link_hash_indirect
		 || h->root.type == bfd_link_hash_warning)
	    h = (struct elf_link_hash_entry *) h->root.u.i.link;

	  if (h->plt.offset != MINUS_ONE)
	    symval = sec_addr (htab->elf.splt) + h->plt.offset;
	  else if (h->root.u.def.section->output_section == NULL
		   || (h->root.type != bfd_link_hash_defined
		       && h->root.type != bfd_link_hash_defweak))
	    continue;
	  else
	    symval = sec_addr (h->root.u.def.section) + h->root.u.def.value;
	}

      symval += irel->r_addend;

      if (call && !_bfd_riscv_relax_call (abfd, sec, link_info, contents,
					  symtab_hdr, isymbuf, internal_relocs,
					  irel, symval, again))
	goto error_return;
      if (lui && !_bfd_riscv_relax_lui (abfd, sec, link_info, contents,
					symtab_hdr, isymbuf, internal_relocs,
					irel, symval, again))
	goto error_return;
      if (tls_le && !_bfd_riscv_relax_tls_le (abfd, sec, link_info, contents,
					      symtab_hdr, isymbuf, internal_relocs,
					      irel, symval, again))
	goto error_return;
      if (tls_ie && !_bfd_riscv_relax_tls_ie (abfd, sec, contents, symtab_hdr,
					      isymbuf, internal_relocs, irel,
					      again))
	goto error_return;
    }

  if (isymbuf != NULL
      && symtab_hdr->contents != (unsigned char *) isymbuf)
    {
      if (! link_info->keep_memory)
	free (isymbuf);
      else
	{
	  /* Cache the symbols for elf_link_input_bfd.  */
	  symtab_hdr->contents = (unsigned char *) isymbuf;
	}
    }

  if (contents != NULL
      && elf_section_data (sec)->this_hdr.contents != contents)
    {
      if (! link_info->keep_memory)
	free (contents);
      else
	{
	  /* Cache the section contents for elf_link_input_bfd.  */
	  elf_section_data (sec)->this_hdr.contents = contents;
	}
    }

  if (internal_relocs != NULL
      && elf_section_data (sec)->relocs != internal_relocs)
    free (internal_relocs);

  return TRUE;

 error_return:
  if (isymbuf != NULL
      && symtab_hdr->contents != (unsigned char *) isymbuf)
    free (isymbuf);
  if (contents != NULL
      && elf_section_data (sec)->this_hdr.contents != contents)
    free (contents);
  if (internal_relocs != NULL
      && elf_section_data (sec)->relocs != internal_relocs)
    free (internal_relocs);

  return FALSE;
}
