/* KVX-specific relocations table.
   Copyright (C) 2009-2024 Free Software Foundation, Inc.
   Contributed by Kalray SA.

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

#ifdef KVX_KV3_V1_KV3_V2_KV4_V1
static reloc_howto_type elf_kvx_howto_table[] =
{
  HOWTO (R_KVX_NONE,			/* type */
	 0,				/* rightshift */
	 0,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 32,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_bitfield,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_NONE",			/* name */
	 false,				/* partial_inplace */
	 0,				/* src_mask */
	 0,				/* dst_mask */
	 false),			/* pcrel_offset */
  HOWTO (R_KVX_16,			/* type */
	 0,				/* rightshift */
	 2,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 16,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_unsigned,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_16",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffff,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_32,			/* type */
	 0,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 32,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_unsigned,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_32",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffffffff,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_64,			/* type */
	 0,				/* rightshift */
	 8,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 64,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_unsigned,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_64",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffffffffffffffff,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S16_PCREL,			/* type */
	 0,				/* rightshift */
	 2,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 16,				/* bitsize */
	 true,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_signed,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S16_PCREL",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffff,			/* dst_mask */
	 true),			/* pc_offset */
  HOWTO (R_KVX_PCREL17,			/* type */
	 2,				/* rightshift */
	 3,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 17,				/* bitsize */
	 true,				/* pc_relative */
	 6,				/* bitpos (bit field offset) */
	 complain_overflow_signed,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_PCREL17",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x7fffc0,			/* dst_mask */
	 true),			/* pc_offset */
  HOWTO (R_KVX_PCREL27,			/* type */
	 2,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 27,				/* bitsize */
	 true,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_signed,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_PCREL27",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x7ffffff,			/* dst_mask */
	 true),			/* pc_offset */
  HOWTO (R_KVX_32_PCREL,			/* type */
	 0,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 32,				/* bitsize */
	 true,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_signed,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_32_PCREL",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffffffff,			/* dst_mask */
	 true),			/* pc_offset */
  HOWTO (R_KVX_S37_PCREL_LO10,			/* type */
	 0,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 10,				/* bitsize */
	 true,				/* pc_relative */
	 6,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S37_PCREL_LO10",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffc0,			/* dst_mask */
	 true),			/* pc_offset */
  HOWTO (R_KVX_S37_PCREL_UP27,			/* type */
	 10,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 27,				/* bitsize */
	 true,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S37_PCREL_UP27",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x7ffffff,			/* dst_mask */
	 true),			/* pc_offset */
  HOWTO (R_KVX_S43_PCREL_LO10,			/* type */
	 0,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 10,				/* bitsize */
	 true,				/* pc_relative */
	 6,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S43_PCREL_LO10",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffc0,			/* dst_mask */
	 true),			/* pc_offset */
  HOWTO (R_KVX_S43_PCREL_UP27,			/* type */
	 10,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 27,				/* bitsize */
	 true,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S43_PCREL_UP27",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x7ffffff,			/* dst_mask */
	 true),			/* pc_offset */
  HOWTO (R_KVX_S43_PCREL_EX6,			/* type */
	 37,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 6,				/* bitsize */
	 true,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S43_PCREL_EX6",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x3f,			/* dst_mask */
	 true),			/* pc_offset */
  HOWTO (R_KVX_S64_PCREL_LO10,			/* type */
	 0,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 10,				/* bitsize */
	 true,				/* pc_relative */
	 6,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S64_PCREL_LO10",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffc0,			/* dst_mask */
	 true),			/* pc_offset */
  HOWTO (R_KVX_S64_PCREL_UP27,			/* type */
	 10,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 27,				/* bitsize */
	 true,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S64_PCREL_UP27",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x7ffffff,			/* dst_mask */
	 true),			/* pc_offset */
  HOWTO (R_KVX_S64_PCREL_EX27,			/* type */
	 37,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 27,				/* bitsize */
	 true,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S64_PCREL_EX27",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x7ffffff,			/* dst_mask */
	 true),			/* pc_offset */
  HOWTO (R_KVX_64_PCREL,			/* type */
	 0,				/* rightshift */
	 8,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 64,				/* bitsize */
	 true,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_signed,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_64_PCREL",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffffffffffffffff,			/* dst_mask */
	 true),			/* pc_offset */
  HOWTO (R_KVX_S16,			/* type */
	 0,				/* rightshift */
	 2,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 16,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_signed,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S16",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffff,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S32_LO5,			/* type */
	 0,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 5,				/* bitsize */
	 false,				/* pc_relative */
	 6,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S32_LO5",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x7c0,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S32_UP27,			/* type */
	 5,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 27,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S32_UP27",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x7ffffff,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S37_LO10,			/* type */
	 0,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 10,				/* bitsize */
	 false,				/* pc_relative */
	 6,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S37_LO10",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffc0,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S37_UP27,			/* type */
	 10,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 27,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S37_UP27",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x7ffffff,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S37_GOTOFF_LO10,			/* type */
	 0,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 10,				/* bitsize */
	 false,				/* pc_relative */
	 6,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S37_GOTOFF_LO10",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffc0,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S37_GOTOFF_UP27,			/* type */
	 10,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 27,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S37_GOTOFF_UP27",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x7ffffff,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S43_GOTOFF_LO10,			/* type */
	 0,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 10,				/* bitsize */
	 false,				/* pc_relative */
	 6,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S43_GOTOFF_LO10",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffc0,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S43_GOTOFF_UP27,			/* type */
	 10,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 27,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S43_GOTOFF_UP27",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x7ffffff,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S43_GOTOFF_EX6,			/* type */
	 37,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 6,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S43_GOTOFF_EX6",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x3f,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_32_GOTOFF,			/* type */
	 0,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 32,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_32_GOTOFF",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffffffff,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_64_GOTOFF,			/* type */
	 0,				/* rightshift */
	 8,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 64,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_64_GOTOFF",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffffffffffffffff,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_32_GOT,			/* type */
	 0,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 32,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_unsigned,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_32_GOT",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffffffff,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S37_GOT_LO10,			/* type */
	 0,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 10,				/* bitsize */
	 false,				/* pc_relative */
	 6,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S37_GOT_LO10",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffc0,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S37_GOT_UP27,			/* type */
	 10,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 27,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S37_GOT_UP27",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x7ffffff,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S43_GOT_LO10,			/* type */
	 0,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 10,				/* bitsize */
	 false,				/* pc_relative */
	 6,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S43_GOT_LO10",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffc0,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S43_GOT_UP27,			/* type */
	 10,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 27,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S43_GOT_UP27",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x7ffffff,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S43_GOT_EX6,			/* type */
	 37,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 6,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S43_GOT_EX6",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x3f,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_64_GOT,			/* type */
	 0,				/* rightshift */
	 8,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 64,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_64_GOT",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffffffffffffffff,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_GLOB_DAT,			/* type */
	 0,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 32,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_GLOB_DAT",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffffffff,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_COPY,			/* type */
	 0,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 32,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_COPY",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffffffff,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_JMP_SLOT,			/* type */
	 0,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 32,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_JMP_SLOT",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffffffff,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_RELATIVE,			/* type */
	 0,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 32,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_RELATIVE",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffffffff,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S43_LO10,			/* type */
	 0,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 10,				/* bitsize */
	 false,				/* pc_relative */
	 6,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S43_LO10",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffc0,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S43_UP27,			/* type */
	 10,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 27,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S43_UP27",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x7ffffff,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S43_EX6,			/* type */
	 37,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 6,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S43_EX6",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x3f,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S64_LO10,			/* type */
	 0,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 10,				/* bitsize */
	 false,				/* pc_relative */
	 6,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S64_LO10",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffc0,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S64_UP27,			/* type */
	 10,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 27,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S64_UP27",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x7ffffff,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S64_EX27,			/* type */
	 37,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 27,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S64_EX27",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x7ffffff,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S37_GOTADDR_LO10,			/* type */
	 0,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 10,				/* bitsize */
	 true,				/* pc_relative */
	 6,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S37_GOTADDR_LO10",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffc0,			/* dst_mask */
	 true),			/* pc_offset */
  HOWTO (R_KVX_S37_GOTADDR_UP27,			/* type */
	 10,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 27,				/* bitsize */
	 true,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S37_GOTADDR_UP27",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x7ffffff,			/* dst_mask */
	 true),			/* pc_offset */
  HOWTO (R_KVX_S43_GOTADDR_LO10,			/* type */
	 0,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 10,				/* bitsize */
	 true,				/* pc_relative */
	 6,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S43_GOTADDR_LO10",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffc0,			/* dst_mask */
	 true),			/* pc_offset */
  HOWTO (R_KVX_S43_GOTADDR_UP27,			/* type */
	 10,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 27,				/* bitsize */
	 true,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S43_GOTADDR_UP27",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x7ffffff,			/* dst_mask */
	 true),			/* pc_offset */
  HOWTO (R_KVX_S43_GOTADDR_EX6,			/* type */
	 37,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 6,				/* bitsize */
	 true,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S43_GOTADDR_EX6",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x3f,			/* dst_mask */
	 true),			/* pc_offset */
  HOWTO (R_KVX_S64_GOTADDR_LO10,			/* type */
	 0,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 10,				/* bitsize */
	 true,				/* pc_relative */
	 6,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S64_GOTADDR_LO10",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffc0,			/* dst_mask */
	 true),			/* pc_offset */
  HOWTO (R_KVX_S64_GOTADDR_UP27,			/* type */
	 10,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 27,				/* bitsize */
	 true,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S64_GOTADDR_UP27",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x7ffffff,			/* dst_mask */
	 true),			/* pc_offset */
  HOWTO (R_KVX_S64_GOTADDR_EX27,			/* type */
	 37,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 27,				/* bitsize */
	 true,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S64_GOTADDR_EX27",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x7ffffff,			/* dst_mask */
	 true),			/* pc_offset */
  HOWTO (R_KVX_64_DTPMOD,			/* type */
	 0,				/* rightshift */
	 8,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 64,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_64_DTPMOD",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffffffffffffffff,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_64_DTPOFF,			/* type */
	 0,				/* rightshift */
	 8,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 64,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_64_DTPOFF",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffffffffffffffff,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S37_TLS_DTPOFF_LO10,			/* type */
	 0,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 10,				/* bitsize */
	 false,				/* pc_relative */
	 6,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S37_TLS_DTPOFF_LO10",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffc0,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S37_TLS_DTPOFF_UP27,			/* type */
	 10,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 27,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S37_TLS_DTPOFF_UP27",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x7ffffff,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S43_TLS_DTPOFF_LO10,			/* type */
	 0,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 10,				/* bitsize */
	 false,				/* pc_relative */
	 6,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S43_TLS_DTPOFF_LO10",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffc0,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S43_TLS_DTPOFF_UP27,			/* type */
	 10,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 27,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S43_TLS_DTPOFF_UP27",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x7ffffff,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S43_TLS_DTPOFF_EX6,			/* type */
	 37,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 6,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S43_TLS_DTPOFF_EX6",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x3f,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S37_TLS_GD_LO10,			/* type */
	 0,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 10,				/* bitsize */
	 false,				/* pc_relative */
	 6,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S37_TLS_GD_LO10",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffc0,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S37_TLS_GD_UP27,			/* type */
	 10,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 27,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S37_TLS_GD_UP27",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x7ffffff,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S43_TLS_GD_LO10,			/* type */
	 0,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 10,				/* bitsize */
	 false,				/* pc_relative */
	 6,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S43_TLS_GD_LO10",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffc0,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S43_TLS_GD_UP27,			/* type */
	 10,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 27,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S43_TLS_GD_UP27",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x7ffffff,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S43_TLS_GD_EX6,			/* type */
	 37,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 6,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S43_TLS_GD_EX6",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x3f,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S37_TLS_LD_LO10,			/* type */
	 0,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 10,				/* bitsize */
	 false,				/* pc_relative */
	 6,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S37_TLS_LD_LO10",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffc0,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S37_TLS_LD_UP27,			/* type */
	 10,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 27,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S37_TLS_LD_UP27",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x7ffffff,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S43_TLS_LD_LO10,			/* type */
	 0,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 10,				/* bitsize */
	 false,				/* pc_relative */
	 6,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S43_TLS_LD_LO10",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffc0,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S43_TLS_LD_UP27,			/* type */
	 10,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 27,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S43_TLS_LD_UP27",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x7ffffff,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S43_TLS_LD_EX6,			/* type */
	 37,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 6,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S43_TLS_LD_EX6",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x3f,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_64_TPOFF,			/* type */
	 0,				/* rightshift */
	 8,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 64,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_64_TPOFF",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffffffffffffffff,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S37_TLS_IE_LO10,			/* type */
	 0,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 10,				/* bitsize */
	 false,				/* pc_relative */
	 6,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S37_TLS_IE_LO10",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffc0,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S37_TLS_IE_UP27,			/* type */
	 10,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 27,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S37_TLS_IE_UP27",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x7ffffff,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S43_TLS_IE_LO10,			/* type */
	 0,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 10,				/* bitsize */
	 false,				/* pc_relative */
	 6,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S43_TLS_IE_LO10",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffc0,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S43_TLS_IE_UP27,			/* type */
	 10,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 27,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S43_TLS_IE_UP27",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x7ffffff,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S43_TLS_IE_EX6,			/* type */
	 37,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 6,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S43_TLS_IE_EX6",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x3f,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S37_TLS_LE_LO10,			/* type */
	 0,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 10,				/* bitsize */
	 false,				/* pc_relative */
	 6,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S37_TLS_LE_LO10",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffc0,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S37_TLS_LE_UP27,			/* type */
	 10,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 27,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S37_TLS_LE_UP27",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x7ffffff,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S43_TLS_LE_LO10,			/* type */
	 0,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 10,				/* bitsize */
	 false,				/* pc_relative */
	 6,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S43_TLS_LE_LO10",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xffc0,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S43_TLS_LE_UP27,			/* type */
	 10,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 27,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S43_TLS_LE_UP27",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x7ffffff,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_S43_TLS_LE_EX6,			/* type */
	 37,				/* rightshift */
	 4,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 6,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_dont,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_S43_TLS_LE_EX6",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0x3f,			/* dst_mask */
	 false),			/* pc_offset */
  HOWTO (R_KVX_8,			/* type */
	 0,				/* rightshift */
	 1,				/* size (0 = byte, 1 = short, 2 = long, 3 = invalid, 4 = 64bits, 8 = 128bits) */
	 8,				/* bitsize */
	 false,				/* pc_relative */
	 0,				/* bitpos (bit field offset) */
	 complain_overflow_unsigned,	/* complain_on_overflow */
	 bfd_elf_generic_reloc,		/* special_function */
	 "R_KVX_8",			/* name */
	 false,				/* partial_inplace */
	 0x0,				/* src_mask */
	 0xff,			/* dst_mask */
	 false),			/* pc_offset */
};

#endif /* KVX_KV3_V1_KV3_V2_KV4_V1 */
