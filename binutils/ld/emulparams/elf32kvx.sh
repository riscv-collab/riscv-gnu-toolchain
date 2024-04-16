#   Copyright (C) 2009-2024 Free Software Foundation, Inc.
#   Contributed by Kalray SA.

#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 3 of the License, or
#   (at your option) any later version.

#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.

#   You should have received a copy of the GNU General Public License
#   along with this program; see the file COPYING3. If not,
#   see <http://www.gnu.org/licenses/>.  */

ARCH=kvx
MACHINE=
SCRIPT_NAME=elf

# bundle with 1 nop insn
NOP=0x00f0037f

TEMPLATE_NAME=elf
EXTRA_EM_FILE=kvxelf

OUTPUT_FORMAT="elf32-kvx"
TEXT_START_ADDR=0x10000
MAXPAGESIZE="CONSTANT (MAXPAGESIZE)"
WRITABLE_RODATA=
GENERATE_SHLIB_SCRIPT=yes
