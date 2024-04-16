/* Copyright (C) 2020-2024 Free Software Foundation, Inc.

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

/* A 16 bit slot holding a 7-bit value of -1.  Note that, for all
   these values, we explicitly set the endian-ness in the DWARF to
   avoid issues.  */
unsigned char i16_m1[2] = { 0x7f, 0 };

/* A 16 bit slot holding a 1-bit value of 1 at offset 2.  */
unsigned char u16_1[2] = { 0x4, 0 };

/* A 32 bit slot holding a 17-bit value of -2.  */
unsigned char u32_m2[4] = { 0xfe, 0xff, 0x01, 0 };

/* A 32 bit slot holding a 31 bit value of 1.  The high bit should be
   ignored when reading.  */
unsigned char u32_1[4] = { 1, 0, 0, 0x80 };

/* A 32 bit slot holding a 31 bit value of 1, offset by 1 bit.  */
unsigned char u32_1_off[4] = { 2, 0, 0, 0 };

/* A 32 bit slot holding a 30 bit value of 1, offset by 1 bit.
   Big-endian.  */
unsigned char be30_1_off[4] = { 0x80, 0, 0, 2 };

/* A 32 bit slot holding a 0 bit value.  We don't use 0 in the array
   here, to catch any situation where gdb tries to use the memory.  */
unsigned char u32_0[4] = { 0xff, 0xff, 0xff, 0xff };

int
main (void)
{
  return 0;
}
