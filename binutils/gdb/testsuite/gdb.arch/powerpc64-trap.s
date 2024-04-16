/* This testcase is part of GDB, the GNU debugger.

   Copyright 2021-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>. */

/* To test if GDB stops at various trap instructions inserted into
   the code.  */

.global main
.type main,function
main:
  ori 0, 0, 0
  trap
  tw  12, 2, 2
  twi 31, 3, 3
  td  12, 2, 2
  tdi 31, 3, 3
  ori 0, 0, 0
  li  3, 0
  blr

	.section	.note.GNU-stack,"",@progbits
