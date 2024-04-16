/* Test linker script for OpenRISC.

   Copyright (C) 2017-2024 Free Software Foundation, Inc.

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

MEMORY
{
    /* The exception vectors actually start at 0x100, but if you specify
       that address here, the "--output-target binary" step will start from
       address 0 with the contents meant for address 0x100.  */
    exception_vectors : ORIGIN =  0 , LENGTH =  8K
    rom               : ORIGIN =  8K, LENGTH = 40K
    ram               : ORIGIN = 40K, LENGTH =  2M - 40K
}

SECTIONS
{
    .exception_vectors :
    {
	KEEP(*(.exception_vectors))
    } > exception_vectors

    .text :
    {
	*(.text)
	*(.text.*)
	*(.rodata)
	*(.rodata.*)
    } > rom

    .data :
    {
	*(.data)
	*(.data.*)
    } > ram

    .bss :
    {
	*(.bss)
	*(.bss.*)

	/* WARNING about section size alignment:
	   The start-up assembly code can only clear BSS section sizes
	   which are aligned to 4 bytes.  However, the size of the BSS
	   section may not be aligned, therefore up to 3 bytes more could
	   be zeroed on start-up.  This is normally not an issue, as the
	   start of the next section is usually aligned too, so those extra
	   bytes should be just padding.  I did try the following trick to
	   align the BSS section size, to no avail:

	   . = ALIGN(., 4);
	*/
    } > ram

    _bss_begin = ADDR(.bss);
    _bss_end   = _bss_begin + SIZEOF(.bss);

    .stack ALIGN(16) (NOLOAD):
    {
      *(.stack)
    } > ram
}

ENTRY(_start)  /* Otherwise, --gc-sections would throw everything away. */
