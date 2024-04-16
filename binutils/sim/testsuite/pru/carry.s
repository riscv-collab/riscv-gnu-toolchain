# Check that carry for addition and subtraction works.
# mach: pru

# Copyright (C) 2023-2024 Free Software Foundation, Inc.
# Contributed by Dimitar Dimitrov <dimitar@dinux.eu>
#
# This file is part of the GNU simulators.
#
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

.include "testutils.inc"

	# Helper macro to exercise three consecutive
	# instructions using the carry bit.
	.macro test_seq srcmode, dstmode, init0, init1, init2, init3, alu0, alu1, alu2, expected
	# In case srcmode>dstmode, "garbage" in the r20 MSB
	# bits could falsely be interpreted as carry.
	# So start with initialized destination to make tests consistent.
	ldi	r20, 0
	ldi32	r15, \init0
	ldi32	r16, \init1
	ldi32	r17, \init2
	ldi32	r18, \init3
	ldi32	r0, \expected
	\alu0	r20\dstmode, r15\srcmode, r16\srcmode
	\alu1	r20\dstmode, r20\srcmode, r17\srcmode
	\alu2	r20\dstmode, r20\srcmode, r18\srcmode
	qbeq	1f, r0, r20\dstmode
	jmp	EXIT_FAIL
1:
	.endm

	# Helper macro to verify one ALU instruction
	# using the carry bit.
	.macro test1 alu, dstmode, src0mode, src0init, src1mode, src1init, expected
	ldi32	r15, \src0init
	ldi32	r16, \src1init
	ldi32	r0, \expected
	\alu	r20\dstmode, r15\src0mode, r16\src1mode
	qbeq	1f, r0, r20\dstmode
	jmp	EXIT_FAIL
1:
	.endm

	start

	# ***** ADD 32-bit dst, 32-bit src *****
	# {add, clear carry}
	test1	add, "   ", "   ", 0x00000000, "   ", 0x00000000, 0x00000000
	# {add with carry=0, clear carry}
	test1	adc, "   ", "   ", 0x00000000, "   ", 0x00000000, 0x00000000
	# {add with carry=0, set carry}
	test1	adc, "   ", "   ", 0x00000001, "   ", 0xffffffff, 0x00000000
	# {add with carry=1, set carry}
	test1	adc, "   ", "   ", 0x00000010, "   ", 0xfffffffe, 0x0000000f
	# {add with carry=1, clear carry}
	test1	adc, "   ", "   ", 0x00000010, "   ", 0x0ffffffe, 0x1000000f
	# {add with carry=0, set carry}
	test1	adc, "   ", "   ", 0x00000001, "   ", 0xffffffff, 0x00000000
	# {add, set carry}
	test1	add, "   ", "   ", 0x00000001, "   ", 0xffffffff, 0x00000000
	# {add with carry=1, set carry}
	test1	adc, "   ", "   ", 0x00000010, "   ", 0xfffffffe, 0x0000000f

	# ***** ADD 32-bit dst, 16-bit src *****
	test1	add, "   ", ".w0", 0x00000000, ".w0", 0x00000000, 0x00000000
	test1	adc, "   ", ".w0", 0x00000000, ".w0", 0x00000000, 0x00000000
	test1	adc, "   ", ".w0", 0x00000003, ".w0", 0xffffffff, 0x00010002
	test1	adc, "   ", ".w0", 0x00000010, ".w0", 0x0000fffe, 0x0001000e

	# ***** ADD 32-bit dst, 8-bit src *****
	test1	add, "   ", ".b0", 0x00000000, ".b0", 0x00000000, 0x00000000
	test1	adc, "   ", ".b0", 0x00000000, ".b0", 0x00000000, 0x00000000
	test1	adc, "   ", ".b0", 0x00000003, ".b0", 0xffffffff, 0x00000102
	test1	adc, "   ", ".b0", 0x00000010, ".b0", 0x0000f0fe, 0x0000010e

	# ***** ADD 16-bit dst, 32-bit src *****
	test1	add, ".w0", "   ", 0x00000000, "   ", 0x00000000, 0x00000000
	test1	adc, ".w0", "   ", 0x00000000, "   ", 0x00000000, 0x00000000
	test1	adc, ".w0", "   ", 0x00000001, "   ", 0xfff0ffff, 0x00000000
	test1	adc, ".w0", "   ", 0x00000010, "   ", 0x0000fffe, 0x0000000f
	test1	adc, ".w0", "   ", 0x00000010, "   ", 0x00000ffe, 0x0000100f
	test1	adc, ".w0", "   ", 0x00000001, "   ", 0x0000ffff, 0x00000000
	test1	add, ".w0", "   ", 0x00000001, "   ", 0x0000ffff, 0x00000000
	test1	adc, ".w0", "   ", 0x00000010, "   ", 0x0000fffe, 0x0000000f
	# Test when intermediate sum sets the carry.
	test1	add, ".w0", "   ", 0x00010000, "   ", 0x00000000, 0x00000000
	test1	adc, ".w0", "   ", 0x00000000, "   ", 0x00000000, 0x00000001
	test1	add, ".w0", "   ", 0x00020000, "   ", 0x00000000, 0x00000000
	test1	adc, ".w0", "   ", 0x00000000, "   ", 0x00000000, 0x00000000

	# ***** ADD 16-bit dst, 16-bit src *****
	test1	add, ".w0", ".w0", 0x00000210, ".w0", 0x00000130, 0x00000340
	test1	adc, ".w0", ".w0", 0x00000000, ".w0", 0x00000000, 0x00000000
	test1	adc, ".w0", ".w0", 0x00000001, ".w0", 0xffffffff, 0x00000000
	test1	adc, ".w0", ".w0", 0x00000010, ".w0", 0xfffffffe, 0x0000000f
	test1	adc, ".w0", ".w0", 0x00000010, ".w0", 0x00000ffe, 0x0000100f
	test1	adc, ".w0", ".w0", 0x00000001, ".w0", 0xfff0ffff, 0x00000000
	test1	add, ".w0", ".w0", 0x00000001, ".w0", 0xfff0ffff, 0x00000000
	test1	adc, ".w0", ".w0", 0x00000010, ".w0", 0xfff1fffe, 0x0000000f

	# ***** ADD 16-bit dst, 8-bit src *****
	test1	add, ".w0", ".b0", 0x00000000, ".b0", 0x00000000, 0x00000000
	test1	adc, ".w0", ".b0", 0x00000000, ".b0", 0x00000000, 0x00000000
	test1	adc, ".w0", ".b0", 0x00000003, ".b0", 0xffffffff, 0x00000102
	test1	adc, ".w0", ".b0", 0x00000010, ".b0", 0x0000f0fe, 0x0000010e

	# ***** ADD 8-bit dst, 32-bit src *****
	test1	add, ".b0", "   ", 0x00000000, "   ", 0x00000000, 0x00000000
	test1	adc, ".b0", "   ", 0x00000000, "   ", 0x00000000, 0x00000000
	test1	adc, ".b0", "   ", 0x00000001, "   ", 0x000000ff, 0x00000000
	test1	adc, ".b0", "   ", 0x00000010, "   ", 0x000000fe, 0x0000000f
	test1	adc, ".b0", "   ", 0x00000021, "   ", 0x0000001e, 0x00000040
	test1	adc, ".b0", "   ", 0x00000001, "   ", 0x000000ff, 0x00000000
	test1	add, ".b0", "   ", 0x00000001, "   ", 0x000000ff, 0x00000000
	test1	adc, ".b0", "   ", 0x00000010, "   ", 0x000000fe, 0x0000000f
	# Test when intermediate sum sets the carry.
	test1	add, ".b0", "   ", 0x10000100, "   ", 0x00000000, 0x00000000
	test1	adc, ".b0", "   ", 0x00000000, "   ", 0x00000000, 0x00000001

	# ***** ADD 8-bit dst, 16-bit src *****
	test1	add, ".b0", ".w0", 0x10000000, ".w0", 0x00000000, 0x00000000
	test1	adc, ".b0", ".w0", 0x02000000, ".w0", 0x00000000, 0x00000000
	test1	adc, ".b0", ".w0", 0x00030001, ".w0", 0x000000ff, 0x00000000
	test1	adc, ".b0", ".w0", 0x00004010, ".w0", 0x000000fe, 0x0000000f
	test1	adc, ".b0", ".w0", 0x00000021, ".w0", 0x0000001e, 0x00000040
	test1	adc, ".b0", ".w0", 0x00000001, ".w0", 0x000000ff, 0x00000000
	test1	add, ".b0", ".w0", 0x00000001, ".w0", 0x000000ff, 0x00000000
	test1	adc, ".b0", ".w0", 0x00000010, ".w0", 0x000000fe, 0x0000000f
	# Test when intermediate sum sets the carry.
	test1	add, ".b0", ".w0", 0x10003100, ".w0", 0x00000000, 0x00000000
	test1	adc, ".b0", ".w0", 0x00000000, ".w0", 0x00000000, 0x00000001

	# ***** ADD 8-bit dst, 8-bit src  *****
	test1	add, ".b0", ".b0", 0x00000210, ".b0", 0x00000130, 0x00000040
	test1	adc, ".b0", ".b0", 0x00000000, ".b0", 0x00000000, 0x00000000
	test1	adc, ".b0", ".b0", 0x00000001, ".b0", 0xffffffff, 0x00000000
	test1	adc, ".b0", ".b0", 0x00000010, ".b0", 0xfffffffe, 0x0000000f
	test1	adc, ".b0", ".b0", 0x00000010, ".b0", 0x0000000e, 0x0000001f
	test1	adc, ".b0", ".b0", 0x00000001, ".b0", 0xfff0ffff, 0x00000000
	test1	add, ".b0", ".b0", 0x00000001, ".b0", 0xfff0ffff, 0x00000000
	test1	adc, ".b0", ".b0", 0x00000010, ".b0", 0xfff1fffe, 0x0000000f

	# ***** SUB 32-bit dst, 32-bit src *****
	# {sub, clear borrow}
	test1	sub, "   ", "   ", 0x00000000, "   ", 0x00000000, 0x00000000
	# {sub with borrow=0, clear borrow}
	test1	suc, "   ", "   ", 0x00000010, "   ", 0x00000001, 0x0000000f
	# {sub with borrow=0, set borrow}
	test1	suc, "   ", "   ", 0x00000008, "   ", 0x00000009, 0xffffffff
	# {sub with borrow=1, set borrow}
	test1	suc, "   ", "   ", 0x00000008, "   ", 0x00000009, 0xfffffffe
	# {sub with borrow=1, clear borrow}
	test1	suc, "   ", "   ", 0x00000008, "   ", 0x00000001, 0x00000006
	# {sub with borrow=0, set borrow}
	test1	suc, "   ", "   ", 0x80000000, "   ", 0x90000000, 0xf0000000
	# {sub, set borrow}
	test1	sub, "   ", "   ", 0x00000000, "   ", 0x00000001, 0xffffffff
	# {sub with borrow=1, set borrow}
	test1	suc, "   ", "   ", 0x80000000, "   ", 0x90000000, 0xefffffff

	# ***** SUB 32-bit dst, 16-bit src *****
	test1	sub, "   ", ".w0", 0x00000000, ".w0", 0x00000000, 0x00000000
	test1	suc, "   ", ".w0", 0x00000010, ".w0", 0x00000001, 0x0000000f
	test1	suc, "   ", ".w0", 0x00000008, ".w0", 0x00000009, 0xffffffff
	test1	suc, "   ", ".w0", 0x00000008, ".w0", 0x00000009, 0xfffffffe
	test1	suc, "   ", ".w0", 0x00000008, ".w0", 0x00000001, 0x00000006
	test1	suc, "   ", ".w0", 0x00108000, ".w0", 0x00009000, 0xfffff000
	test1	sub, "   ", ".w0", 0x00000000, ".w0", 0x00000001, 0xffffffff
	test1	suc, "   ", ".w0", 0x00008000, ".w0", 0x00009000, 0xffffefff

	# ***** SUB 32-bit dst, 8-bit src *****
	test1	sub, "   ", ".b0", 0x00000000, ".b0", 0x00000000, 0x00000000
	test1	suc, "   ", ".b0", 0x00000010, ".b0", 0x00000001, 0x0000000f
	test1	suc, "   ", ".b0", 0x00000008, ".b0", 0x00000009, 0xffffffff
	test1	suc, "   ", ".b0", 0x00000008, ".b0", 0x00000009, 0xfffffffe
	test1	suc, "   ", ".b0", 0x00000008, ".b0", 0x00000001, 0x00000006
	test1	suc, "   ", ".b0", 0x00108080, ".b0", 0x00009090, 0xfffffff0
	test1	sub, "   ", ".b0", 0x00000000, ".b0", 0x00000001, 0xffffffff
	test1	suc, "   ", ".b0", 0x00008080, ".b0", 0x00009090, 0xffffffef

	# ***** SUB 16-bit dst, 32-bit src *****
	test1	sub, ".w0", "   ", 0x00000000, "   ", 0x00000000, 0x00000000
	test1	suc, ".w0", "   ", 0x00000010, "   ", 0x00000001, 0x0000000f
	test1	suc, ".w0", "   ", 0x00000008, "   ", 0x00000009, 0x0000ffff
	test1	suc, ".w0", "   ", 0x00000008, "   ", 0x00000009, 0x0000fffe
	test1	suc, ".w0", "   ", 0x00000008, "   ", 0x00000001, 0x00000006
	test1	suc, ".w0", "   ", 0x00108000, "   ", 0x00009000, 0x0000f000
	test1	sub, ".w0", "   ", 0x00000000, "   ", 0x00000001, 0x0000ffff
	test1	suc, ".w0", "   ", 0x00008000, "   ", 0x00009000, 0x0000efff
	# Test when intermediate value sets the borrow.
	test1	sub, ".w0", "   ", 0x00010000, "   ", 0x00000000, 0x00000000
	test1	suc, ".w0", "   ", 0x00000002, "   ", 0x00000000, 0x00000001

	# ***** SUB 16-bit dst, 16-bit src *****
	test1	sub, ".w0", ".w0", 0x00000000, ".w0", 0x00000000, 0x00000000
	test1	suc, ".w0", ".w0", 0x00000010, ".w0", 0x00000001, 0x0000000f
	test1	suc, ".w0", ".w0", 0x00000008, ".w0", 0x00000009, 0x0000ffff
	test1	suc, ".w0", ".w0", 0x00000008, ".w0", 0x00000009, 0x0000fffe
	test1	suc, ".w0", ".w0", 0x00000008, ".w0", 0x00000001, 0x00000006
	test1	suc, ".w0", ".w0", 0x00108000, ".w0", 0x00009000, 0x0000f000
	test1	sub, ".w0", ".w0", 0x00000000, ".w0", 0x00000001, 0x0000ffff
	test1	suc, ".w0", ".w0", 0x00008000, ".w0", 0x00009000, 0x0000efff

	# ***** SUB 16-bit dst, 8-bit src *****
	test1	sub, ".w0", ".b0", 0x00000000, ".b0", 0x00000000, 0x00000000
	test1	suc, ".w0", ".b0", 0x00000010, ".b0", 0x00000001, 0x0000000f
	test1	suc, ".w0", ".b0", 0x00000008, ".b0", 0x00000009, 0x0000ffff
	test1	suc, ".w0", ".b0", 0x00000008, ".b0", 0x00000009, 0x0000fffe
	test1	suc, ".w0", ".b0", 0x00000008, ".b0", 0x00000001, 0x00000006
	test1	suc, ".w0", ".b0", 0x00108080, ".b0", 0x00009090, 0x0000fff0
	test1	sub, ".w0", ".b0", 0x00000000, ".b0", 0x00000001, 0x0000ffff
	test1	suc, ".w0", ".b0", 0x0000a080, ".b0", 0x0000c090, 0x0000ffef

	# ***** SUB 8-bit dst, 32-bit src *****
	test1	sub, ".b0", "   ", 0x00000000, "   ", 0x00000000, 0x00000000
	test1	suc, ".b0", "   ", 0x00000010, "   ", 0x00000001, 0x0000000f
	test1	suc, ".b0", "   ", 0x00000008, "   ", 0x00000009, 0x000000ff
	test1	suc, ".b0", "   ", 0x00000008, "   ", 0x00000009, 0x000000fe
	test1	suc, ".b0", "   ", 0x00000008, "   ", 0x00000001, 0x00000006
	test1	suc, ".b0", "   ", 0x00000080, "   ", 0x00000090, 0x000000f0
	test1	sub, ".b0", "   ", 0x00000000, "   ", 0x00000001, 0x000000ff
	test1	suc, ".b0", "   ", 0x00000080, "   ", 0x00000090, 0x000000ef
	# Test when intermediate value sets the borrow.
	test1	sub, ".b0", "   ", 0x00000100, "   ", 0x00000000, 0x00000000
	test1	suc, ".b0", "   ", 0x00000002, "   ", 0x00000000, 0x00000001
	test1	sub, ".b0", "   ", 0x00000100, "   ", 0x00000000, 0x00000000
	test1	rsc, ".b0", "   ", 0x00000000, "   ", 0x00000002, 0x00000001

	# ***** SUB 8-bit dst, 16-bit src *****
	test1	sub, ".b0", ".w0", 0x00000000, ".w0", 0x00000000, 0x00000000
	test1	suc, ".b0", ".w0", 0x00000010, ".w0", 0x00000001, 0x0000000f
	test1	suc, ".b0", ".w0", 0x00000008, ".w0", 0x00000009, 0x000000ff
	test1	suc, ".b0", ".w0", 0x00000008, ".w0", 0x00000009, 0x000000fe
	test1	suc, ".b0", ".w0", 0x00000008, ".w0", 0x00000001, 0x00000006
	test1	suc, ".b0", ".w0", 0x00000080, ".w0", 0x00000090, 0x000000f0
	test1	sub, ".b0", ".w0", 0x00000000, ".w0", 0x00000001, 0x000000ff
	test1	suc, ".b0", ".w0", 0x00000080, ".w0", 0x00000090, 0x000000ef
	# Test when intermediate value sets the borrow.
	test1	sub, ".b0", ".w0", 0x00000100, ".w0", 0x00000000, 0x00000000
	test1	suc, ".b0", ".w0", 0x00000002, ".w0", 0x00000000, 0x00000001
	test1	sub, ".b0", ".w0", 0x00000100, ".w0", 0x00000000, 0x00000000
	test1	rsc, ".b0", ".w0", 0x00000000, ".w0", 0x00000002, 0x00000001

	# ***** SUB 8-bit dst, 8-bit src *****
	test1	sub, ".b0", ".w0", 0x00000000, ".w0", 0x00000000, 0x00000000
	test1	suc, ".b0", ".w0", 0x00000010, ".w0", 0x00000001, 0x0000000f
	test1	suc, ".b0", ".w0", 0x00000008, ".w0", 0x00000009, 0x000000ff
	test1	suc, ".b0", ".w0", 0x00000008, ".w0", 0x00000009, 0x000000fe
	test1	suc, ".b0", ".w0", 0x00000008, ".w0", 0x00000001, 0x00000006
	test1	suc, ".b0", ".w0", 0x00000080, ".w0", 0x00000090, 0x000000f0
	test1	sub, ".b0", ".w0", 0x00000000, ".w0", 0x00000001, 0x000000ff
	test1	suc, ".b0", ".w0", 0x00000080, ".w0", 0x00000090, 0x000000ef

	# ***** Reverse SUB 32-bit dst, 32-bit src *****
	# {rsb, clear borrow}
	test1	rsb, "   ", "   ", 0x00000000, "   ", 0x00000000, 0x00000000
	# {rsb with borrow=0, clear borrow}
	test1	rsc, "   ", "   ", 0x00000001, "   ", 0x00000010, 0x0000000f
	# {rsb with borrow=0, set borrow}
	test1	rsc, "   ", "   ", 0x00000009, "   ", 0x00000008, 0xffffffff
	# {rsb with borrow=1, set borrow}
	test1	rsc, "   ", "   ", 0x00000009, "   ", 0x00000008, 0xfffffffe
	# {rsb with borrow=1, clear borrow}
	test1	rsc, "   ", "   ", 0x00000001, "   ", 0x00000008, 0x00000006
	# {rsb with borrow=0, set borrow}
	test1	rsc, "   ", "   ", 0x90000000, "   ", 0x80000000, 0xf0000000
	# {rsb, set borrow}
	test1	rsb, "   ", "   ", 0x00000001, "   ", 0x00000000, 0xffffffff
	# {rsb with borrow=1, set borrow}
	test1	rsc, "   ", "   ", 0x90000000, "   ", 0x80000000, 0xefffffff

	# ***** Reverse SUB 32-bit dst, 16-bit src *****
	test1	rsb, "   ", ".w0", 0x00000000, ".w0", 0x00000000, 0x00000000
	test1	rsc, "   ", ".w0", 0x00000001, ".w0", 0x00000010, 0x0000000f
	test1	rsc, "   ", ".w0", 0x00000009, ".w0", 0x00000008, 0xffffffff
	test1	rsc, "   ", ".w0", 0x00000009, ".w0", 0x00000008, 0xfffffffe
	test1	rsc, "   ", ".w0", 0x00000001, ".w0", 0x00000008, 0x00000006
	test1	rsc, "   ", ".w0", 0x00109000, ".w0", 0x00008000, 0xfffff000
	test1	rsb, "   ", ".w0", 0x00000001, ".w0", 0x00000000, 0xffffffff
	test1	rsc, "   ", ".w0", 0x00009000, ".w0", 0x00008000, 0xffffefff

	# ***** Reverse SUB 32-bit dst, 8-bit src *****
	test1	rsb, "   ", ".b0", 0x00000000, ".b0", 0x00000000, 0x00000000
	test1	rsc, "   ", ".b0", 0x00000001, ".b0", 0x00000010, 0x0000000f
	test1	rsc, "   ", ".b0", 0x00000009, ".b0", 0x00000008, 0xffffffff
	test1	rsc, "   ", ".b0", 0x00000009, ".b0", 0x00000008, 0xfffffffe
	test1	rsc, "   ", ".b0", 0x00000001, ".b0", 0x00000008, 0x00000006
	test1	rsc, "   ", ".b0", 0x00108090, ".b0", 0x00009080, 0xfffffff0
	test1	rsb, "   ", ".b0", 0x00000001, ".b0", 0x00000000, 0xffffffff
	test1	rsc, "   ", ".b0", 0x00008090, ".b0", 0x00009080, 0xffffffef

	# ***** Reverse SUB 16-bit dst, 32-bit src *****
	test1	rsb, ".w0", "   ", 0x00000000, "   ", 0x00000000, 0x00000000
	test1	rsc, ".w0", "   ", 0x00000001, "   ", 0x00000010, 0x0000000f
	test1	rsc, ".w0", "   ", 0x00000009, "   ", 0x00000008, 0x0000ffff
	test1	rsc, ".w0", "   ", 0x00000009, "   ", 0x00000008, 0x0000fffe
	test1	rsc, ".w0", "   ", 0x00000001, "   ", 0x00000008, 0x00000006
	test1	rsc, ".w0", "   ", 0x00109000, "   ", 0x00008000, 0x0000f000
	test1	rsb, ".w0", "   ", 0x00000001, "   ", 0x00000000, 0x0000ffff
	test1	rsc, ".w0", "   ", 0x00009000, "   ", 0x00008000, 0x0000efff
	# Test when intermediate value sets the borrow.
	test1	rsb, ".w0", "   ", 0x00000000, "   ", 0x00010000, 0x00000000
	test1	rsc, ".w0", "   ", 0x00000000, "   ", 0x00000002, 0x00000001

	# ***** Reverse SUB 16-bit dst, 16-bit src *****
	test1	rsb, ".w0", ".w0", 0x00000000, ".w0", 0x00000000, 0x00000000
	test1	rsc, ".w0", ".w0", 0x00000001, ".w0", 0x00000010, 0x0000000f
	test1	rsc, ".w0", ".w0", 0x00000009, ".w0", 0x00000008, 0x0000ffff
	test1	rsc, ".w0", ".w0", 0x00000009, ".w0", 0x00000008, 0x0000fffe
	test1	rsc, ".w0", ".w0", 0x00000001, ".w0", 0x00000008, 0x00000006
	test1	rsc, ".w0", ".w0", 0x00109000, ".w0", 0x00008000, 0x0000f000
	test1	rsb, ".w0", ".w0", 0x00000001, ".w0", 0x00000000, 0x0000ffff
	test1	rsc, ".w0", ".w0", 0x00009000, ".w0", 0x00008000, 0x0000efff

	# ***** Reverse SUB 16-bit dst, 8-bit src *****
	test1	rsb, ".w0", ".b0", 0x00000000, ".b0", 0x00000000, 0x00000000
	test1	rsc, ".w0", ".b0", 0x00000001, ".b0", 0x00000010, 0x0000000f
	test1	rsc, ".w0", ".b0", 0x00000009, ".b0", 0x00000008, 0x0000ffff
	test1	rsc, ".w0", ".b0", 0x00000009, ".b0", 0x00000008, 0x0000fffe
	test1	rsc, ".w0", ".b0", 0x00000001, ".b0", 0x00000008, 0x00000006
	test1	rsc, ".w0", ".b0", 0x00108090, ".b0", 0x00009080, 0x0000fff0
	test1	rsb, ".w0", ".b0", 0x00000001, ".b0", 0x00000000, 0x0000ffff
	test1	rsc, ".w0", ".b0", 0x0000a090, ".b0", 0x0000c080, 0x0000ffef

	# ***** Reverse SUB 8-bit dst, 32-bit src *****
	test1	rsb, ".b0", "   ", 0x00000000, "   ", 0x00000000, 0x00000000
	test1	rsc, ".b0", "   ", 0x00000001, "   ", 0x00000010, 0x0000000f
	test1	rsc, ".b0", "   ", 0x00000009, "   ", 0x00000008, 0x000000ff
	test1	rsc, ".b0", "   ", 0x00000009, "   ", 0x00000008, 0x000000fe
	test1	rsc, ".b0", "   ", 0x00000001, "   ", 0x00000008, 0x00000006
	test1	rsc, ".b0", "   ", 0x00000090, "   ", 0x00000080, 0x000000f0
	test1	rsb, ".b0", "   ", 0x00000001, "   ", 0x00000000, 0x000000ff
	test1	rsc, ".b0", "   ", 0x00000090, "   ", 0x00000080, 0x000000ef
	# Test when intermediate value sets the borrow.
	test1	rsb, ".b0", "   ", 0x00000000, "   ", 0x00000100, 0x00000000
	test1	rsc, ".b0", "   ", 0x00000000, "   ", 0x00000002, 0x00000001
	test1	rsb, ".b0", "   ", 0x00000000, "   ", 0x00000100, 0x00000000
	test1	suc, ".b0", "   ", 0x00000002, "   ", 0x00000000, 0x00000001

	# ***** Reverse SUB 8-bit dst, 16-bit src *****
	test1	rsb, ".b0", ".w0", 0x00000000, ".w0", 0x00000000, 0x00000000
	test1	rsc, ".b0", ".w0", 0x00000001, ".w0", 0x00000010, 0x0000000f
	test1	rsc, ".b0", ".w0", 0x00000009, ".w0", 0x00000008, 0x000000ff
	test1	rsc, ".b0", ".w0", 0x00000009, ".w0", 0x00000008, 0x000000fe
	test1	rsc, ".b0", ".w0", 0x00000001, ".w0", 0x00000008, 0x00000006
	test1	rsc, ".b0", ".w0", 0x00000090, ".w0", 0x00000080, 0x000000f0
	test1	rsb, ".b0", ".w0", 0x00000001, ".w0", 0x00000000, 0x000000ff
	test1	rsc, ".b0", ".w0", 0x00000090, ".w0", 0x00000080, 0x000000ef
	# Test when intermediate value sets the borrow.
	test1	rsb, ".b0", ".w0", 0x00000000, ".w0", 0x00000100, 0x00000000
	test1	rsc, ".b0", ".w0", 0x00000000, ".w0", 0x00000002, 0x00000001
	test1	rsb, ".b0", ".w0", 0x00000000, ".w0", 0x00000100, 0x00000000
	test1	suc, ".b0", ".w0", 0x00000002, ".w0", 0x00000000, 0x00000001

	# ***** Reverse SUB 8-bit dst, 8-bit src *****
	test1	rsb, ".b0", ".w0", 0x00000000, ".w0", 0x00000000, 0x00000000
	test1	rsc, ".b0", ".w0", 0x00000001, ".w0", 0x00000010, 0x0000000f
	test1	rsc, ".b0", ".w0", 0x00000009, ".w0", 0x00000008, 0x000000ff
	test1	rsc, ".b0", ".w0", 0x00000009, ".w0", 0x00000008, 0x000000fe
	test1	rsc, ".b0", ".w0", 0x00000001, ".w0", 0x00000008, 0x00000006
	test1	rsc, ".b0", ".w0", 0x00000090, ".w0", 0x00000080, 0x000000f0
	test1	rsb, ".b0", ".w0", 0x00000001, ".w0", 0x00000000, 0x000000ff
	test1	rsc, ".b0", ".w0", 0x00000090, ".w0", 0x00000080, 0x000000ef

	# ***** Mixed 32-bit *****
	test1	sub, "   ", " ", 0x00000000, " ", 0x00000000, 0x00000000
	test1	adc, "   ", " ", 0x00000000, " ", 0x00000000, 0x00000001
	test1	suc, "   ", " ", 0x00000001, " ", 0x00000001, 0xffffffff
	test1	adc, "   ", " ", 0x00000001, " ", 0x00000000, 0x00000001

	test_seq "", "", 0xffffffff, 0x00000001, 0x00000000, 0x00000000 add, suc, add, 0x00000000
	test_seq "", "", 0xffffffff, 0x00000001, 0x00000000, 0x00000000 add, rsc, add, 0x00000000
	test_seq "", "", 0xfffffffe, 0x00000001, 0x00000000, 0x00000000 add, suc, add, 0xfffffffe
	test_seq "", "", 0xffffffff, 0x00000001, 0x00000000, 0x00000000 add, suc, adc, 0x00000001
	test_seq "", "", 0xfffffffe, 0x00000001, 0x00000000, 0x00000000 add, suc, adc, 0xffffffff

	test_seq "", "", 0xffffffff, 0x00000010, 0x00000000, 0x00000000 sub, adc, add, 0xfffffff0
	test_seq "", "", 0x0fffffff, 0x00000010, 0x00000000, 0x00000000 sub, adc, add, 0x0ffffff0
	test_seq "", "", 0x00000000, 0x00000010, 0x00000000, 0x00000000 sub, adc, add, 0xfffffff0
	test_seq "", "", 0x00000000, 0x00000010, 0x00000000, 0x00000000 sub, adc, adc, 0xfffffff0
	test_seq "", "", 0x00000000, 0x00000010, 0x00000000, 0x00000000 sub, adc, suc, 0xffffffef


	# For coverage, also test sequences of ALU instructions
	# (i.e. no other instructions in between).
	test_seq "", "", 0x00000000, 0x00000000, 0x00000000, 0x00000000 add, adc, adc, 0x00000000
	test_seq "", "", 0x80000000, 0xfffffff0, 0xc0000000, 0x0000001a add, adc, adc, 0x4000000c
	test_seq "", ".b0", 0x00000000, 0x00000001, 0x000000ff, 0x00000000 add, adc, adc, 0x00000001
	test_seq "", ".b0", 0x00000000, 0x00000001, 0x000000ff, 0x00000000 add, adc, adc, 0x00000001
	test_seq ".b0", ".b0", 0x00000000, 0x00000001, 0x00000000, 0x0000ffff add, adc, adc, 0x00000000
	test_seq ".b0", ".w0", 0x00000001, 0x00000020, 0x00000400, 0x00008000 add, adc, adc, 0x00000021
	test_seq "", "", 0x00000010, 0x0000000f, 0x00000002, 0x00000020 sub, suc, suc, 0xffffffde
	test_seq "", ".b0", 0x00000202, 0x00000000, 0x00000000, 0x00000000 sub, suc, suc, 0x00000002
	test_seq ".w0", ".b0", 0x00000008, 0x00000000, 0x00000000, 0x00000000, sub, suc, suc, 0x00000008
	test_seq ".w0", ".b0", 0x00000008, 0x000000ff, 0x000000ff, 0x000000ff, sub, suc, suc, 0x00000009
	test_seq ".w0", ".b0", 0x00000008, 0x00000fff, 0x00000fff, 0x00000fff, sub, suc, suc, 0x0000000b
	test_seq "", "", 0x0000000f, 0x00000010, 0x00000002, 0x00000020 rsb, suc, suc, 0xffffffde
	test_seq "", "", 0xffffffff, 0x00000001, 0x00000000, 0x00000000 add, suc, adc, 0x00000001
	test_seq "", "", 0x00000000, 0x00000010, 0x00000000, 0x00000000 sub, adc, adc, 0xfffffff0
	test_seq "", "", 0x00000000, 0x00000010, 0x00000000, 0x00000000 sub, adc, suc, 0xffffffef

	pass
EXIT_FAIL:
	fail
