/* Copyright 2017-2024 Free Software Foundation, Inc.

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


/* DWARF will describe these contents as being inside a D module.  */

typedef struct tstruct
{
} tstruct;

tstruct my_data;

int _Dmain (void)
{
  asm ("_Dmain_label: .globl _Dmain_label");
  return 0;
}

asm ("_Dmain_end: .globl _Dmain_end");

int
main (void)
{
  return _Dmain ();
}
