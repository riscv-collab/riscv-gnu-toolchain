/*
   Copyright 2007-2024 Free Software Foundation, Inc.
   
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

/* Despite the sections below will be adjacent the assembler has to produce
   DW_AT_ranges as the linker could place both sections at arbitrary locations.
   */

/* `.fini' section is here to make sure `dw2-ranges.c'
   vs. `dw2-ranges2.c' overlap their DW_AT_ranges with each other.  */

void __attribute__ ((section (".fini")))
func2 (void)
{
}

void
main2 (void)
{
}
