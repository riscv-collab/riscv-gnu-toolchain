/* Copyright (C) 2014-2024 Free Software Foundation, Inc.

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

#include <sys/sdt.h>

	.file	"amd64-stap-optional-prefix.S"
	.text
	.globl	main
main:
	mov	%rsp,%rbp
	pushq	$42
	STAP_PROBE1(probe, foo, (%rsp))
	STAP_PROBE1(probe, bar, -8(%rbp))
	STAP_PROBE1(probe, foo_prefix, 8@(%rsp))
	STAP_PROBE1(probe, bar_prefix, 8@-8(%rbp))
	mov	%rbp,%rsp
	STAP_PROBE1(probe, uint8_probe, 1@$8)
	STAP_PROBE1(probe, int8_probe, -1@$-8)
	STAP_PROBE1(probe, uint16_probe, 2@$16)
	STAP_PROBE1(probe, int16_probe, -2@$-16)
	STAP_PROBE1(probe, uint32_probe, 4@$32)
	STAP_PROBE1(probe, int32_probe, -4@$-32)
	STAP_PROBE1(probe, uint64_probe, 8@$64)
	STAP_PROBE1(probe, int64_probe, -8@$-64)
	STAP_PROBE1(probe, fail_probe, -7@$16)
	STAP_PROBE1(probe, fail2_probe, 23-@$16)
	xor	%rax,%rax
	ret
	.section	.note.GNU-stack,"",@progbits
