# Copyright 2021-2024 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.


# test to verify that the prefixed instructions that
# load/store non-relative values work OK.

.global main
.type main,function
main:
	nop
	lnia 4
	addi 4,4,40
	plxv 4,4(4),0
	plxv 5,12(4),0
	plxv 6,20(4),0
	plxv 7,28(4),0
check_here:
	blr
mydata:
	.long 0xa1b1c1d1	# <<-
	.long 0xa2b2c2d2	# <<- loaded into vs4
	.long 0xa3b3c3d3	# <<- loaded into vs4
	.long 0xa4b4c4d4	# <<- loaded into vs4, vs5
	.long 0xa5b5c5d5	# <<- loaded into vs4, vs5
	.long 0xa6b6c6d6	# <<- loaded into      vs5, vs6
	.long 0xa7b7c7d7	# <<- loaded into      vs5, vs6
	.long 0xa8b8c8d8	# <<- loaded into           vs6, vs7
	.long 0xa9b9c9d9	# <<- loaded into           vs6, vs7
	.long 0xaabacada	# <<- loaded into                vs7
	.long 0xabbbcbdb	# <<- loaded into                vs7
	.long 0xacbcccdc	# <<-

	.section	.note.GNU-stack,"",@progbits
