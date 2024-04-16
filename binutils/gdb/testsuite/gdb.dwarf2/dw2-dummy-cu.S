/* Copyright 2015-2024 Free Software Foundation, Inc.

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

	.text
main:
	.4byte	0
main_end:

	.section	.debug_info
	.4byte	.Ldebug_info_end - 1f	/* Length of Compilation Unit Info */
1:
	.2byte	0x2	/* DWARF version number */
	.4byte	.Ldebug_abbrev0	/* Offset Into Abbrev. Section */
	.byte	0x4	/* Pointer Size (in bytes) */

	/* Nothing else, this is a dummy die.  */
.Ldebug_info_end:

	.section	.debug_abbrev
.Ldebug_abbrev0:
	.byte	0x0
#ifdef __arm__
	.section	.note.GNU-stack,"",%progbits
#else
	.section	.note.GNU-stack,"",@progbits
#endif
