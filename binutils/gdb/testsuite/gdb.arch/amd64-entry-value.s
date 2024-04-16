/* This testcase is part of GDB, the GNU debugger.

   Copyright 2011-2024 Free Software Foundation, Inc.

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

/* This file is compiled from gdb.arch/amd64-entry-value.c
   using -g -dA -S -O2.  */

	.file	"amd64-entry-value.cc"
	.text
.Ltext0:
	.p2align 4,,15
	.type	_ZL1eid, @function
_ZL1eid:
.LFB0:
	.file 1 "gdb.arch/amd64-entry-value.cc"
	# gdb.arch/amd64-entry-value.cc:22
	.loc 1 22 0
	.cfi_startproc
.LVL0:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	# gdb.arch/amd64-entry-value.cc:23
	.loc 1 23 0
	movl	$0, _ZL1v(%rip)
# SUCC: EXIT [100.0%] 
	# gdb.arch/amd64-entry-value.cc:24
	.loc 1 24 0
	ret
	.cfi_endproc
.LFE0:
	.size	_ZL1eid, .-_ZL1eid
	.p2align 4,,15
	.type	_ZL1did, @function
_ZL1did:
.LFB1:
	# gdb.arch/amd64-entry-value.cc:28
	.loc 1 28 0
	.cfi_startproc
.LVL1:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	# gdb.arch/amd64-entry-value.cc:30
	.loc 1 30 0
	addsd	.LC0(%rip), %xmm0
.LVL2:
	# gdb.arch/amd64-entry-value.cc:29
	.loc 1 29 0
	addl	$1, %edi
.LVL3:
	# gdb.arch/amd64-entry-value.cc:31
	.loc 1 31 0
	call	_ZL1eid
.LVL4:
	# gdb.arch/amd64-entry-value.cc:32
	.loc 1 32 0
	movl	_ZL1v(%rip), %eax
	movl	_ZL1v(%rip), %edi
	cvtsi2sd	%eax, %xmm0
	call	_ZL1eid
.LVL5:
	# gdb.arch/amd64-entry-value.cc:33
	.loc 1 33 0
#APP
# 33 "gdb.arch/amd64-entry-value.cc" 1
	breakhere:
# 0 "" 2
	# gdb.arch/amd64-entry-value.cc:34
	.loc 1 34 0
#NO_APP
	movl	_ZL1v(%rip), %eax
	movl	_ZL1v(%rip), %edi
	cvtsi2sd	%eax, %xmm0
	jmp	_ZL1eid
# SUCC: EXIT [100.0%]  (ab,sibcall)
.LVL6:
	.cfi_endproc
.LFE1:
	.size	_ZL1did, .-_ZL1did
	.p2align 4,,15
	.type	_ZL7locexpri, @function
_ZL7locexpri:
.LFB2:
	# gdb.arch/amd64-entry-value.cc:39
	.loc 1 39 0
	.cfi_startproc
.LVL7:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	# gdb.arch/amd64-entry-value.cc:41
	.loc 1 41 0
#APP
# 41 "gdb.arch/amd64-entry-value.cc" 1
	breakhere_locexpr:
# 0 "" 2
# SUCC: EXIT [100.0%] 
	# gdb.arch/amd64-entry-value.cc:42
	.loc 1 42 0
#NO_APP
	ret
	.cfi_endproc
.LFE2:
	.size	_ZL7locexpri, .-_ZL7locexpri
	.p2align 4,,15
	.type	_ZL1cid, @function
_ZL1cid:
.LFB3:
	# gdb.arch/amd64-entry-value.cc:46
	.loc 1 46 0
	.cfi_startproc
.LVL8:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	# gdb.arch/amd64-entry-value.cc:47
	.loc 1 47 0
	mulsd	.LC1(%rip), %xmm0
.LVL9:
	leal	(%rdi,%rdi,4), %edi
.LVL10:
	addl	%edi, %edi
	jmp	_ZL1did
# SUCC: EXIT [100.0%]  (ab,sibcall)
.LVL11:
	.cfi_endproc
.LFE3:
	.size	_ZL1cid, .-_ZL1cid
	.p2align 4,,15
	.type	_ZL1aid, @function
_ZL1aid:
.LFB4:
	# gdb.arch/amd64-entry-value.cc:52
	.loc 1 52 0
	.cfi_startproc
.LVL12:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	# gdb.arch/amd64-entry-value.cc:53
	.loc 1 53 0
	addsd	.LC0(%rip), %xmm0
.LVL13:
	addl	$1, %edi
.LVL14:
	jmp	_ZL1cid
.LVL15:
# SUCC: EXIT [100.0%]  (ab,sibcall)
	.cfi_endproc
.LFE4:
	.size	_ZL1aid, .-_ZL1aid
	.p2align 4,,15
	.type	_ZL1bid, @function
_ZL1bid:
.LFB5:
	# gdb.arch/amd64-entry-value.cc:58
	.loc 1 58 0
	.cfi_startproc
.LVL16:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	# gdb.arch/amd64-entry-value.cc:59
	.loc 1 59 0
	addsd	.LC2(%rip), %xmm0
.LVL17:
	addl	$2, %edi
.LVL18:
	jmp	_ZL1cid
.LVL19:
# SUCC: EXIT [100.0%]  (ab,sibcall)
	.cfi_endproc
.LFE5:
	.size	_ZL1bid, .-_ZL1bid
	.p2align 4,,15
	.type	_ZL5amb_zi, @function
_ZL5amb_zi:
.LFB6:
	# gdb.arch/amd64-entry-value.cc:64
	.loc 1 64 0
	.cfi_startproc
.LVL20:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	# gdb.arch/amd64-entry-value.cc:65
	.loc 1 65 0
	cvtsi2sd	%edi, %xmm0
	addl	$7, %edi
.LVL21:
	addsd	.LC3(%rip), %xmm0
	jmp	_ZL1did
.LVL22:
# SUCC: EXIT [100.0%]  (ab,sibcall)
	.cfi_endproc
.LFE6:
	.size	_ZL5amb_zi, .-_ZL5amb_zi
	.p2align 4,,15
	.type	_ZL5amb_yi, @function
_ZL5amb_yi:
.LFB7:
	# gdb.arch/amd64-entry-value.cc:70
	.loc 1 70 0
	.cfi_startproc
.LVL23:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	# gdb.arch/amd64-entry-value.cc:71
	.loc 1 71 0
	addl	$6, %edi
.LVL24:
	jmp	_ZL5amb_zi
.LVL25:
# SUCC: EXIT [100.0%]  (ab,sibcall)
	.cfi_endproc
.LFE7:
	.size	_ZL5amb_yi, .-_ZL5amb_yi
	.p2align 4,,15
	.type	_ZL5amb_xi, @function
_ZL5amb_xi:
.LFB8:
	# gdb.arch/amd64-entry-value.cc:76
	.loc 1 76 0
	.cfi_startproc
.LVL26:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	# gdb.arch/amd64-entry-value.cc:77
	.loc 1 77 0
	addl	$5, %edi
.LVL27:
	jmp	_ZL5amb_yi
.LVL28:
# SUCC: EXIT [100.0%]  (ab,sibcall)
	.cfi_endproc
.LFE8:
	.size	_ZL5amb_xi, .-_ZL5amb_xi
	.p2align 4,,15
	.type	_ZL3ambi, @function
_ZL3ambi:
.LFB9:
	# gdb.arch/amd64-entry-value.cc:82
	.loc 1 82 0
	.cfi_startproc
.LVL29:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	# gdb.arch/amd64-entry-value.cc:83
	.loc 1 83 0
	testl	%edi, %edi
# SUCC: 4 [19.1%]  (can_fallthru) 3 [80.9%]  (fallthru,can_fallthru)
	js	.L13
# BLOCK 3 freq:8088 seq:1
# PRED: 2 [80.9%]  (fallthru,can_fallthru)
	# gdb.arch/amd64-entry-value.cc:86
	.loc 1 86 0
	addl	$4, %edi
.LVL30:
	jmp	_ZL5amb_xi
.LVL31:
# SUCC: EXIT [100.0%]  (ab,sibcall)
# BLOCK 4 freq:1912 seq:2
# PRED: 2 [19.1%]  (can_fallthru)
.L13:
	# gdb.arch/amd64-entry-value.cc:84
	.loc 1 84 0
	addl	$3, %edi
.LVL32:
	jmp	_ZL5amb_xi
.LVL33:
# SUCC: EXIT [100.0%]  (ab,sibcall)
	.cfi_endproc
.LFE9:
	.size	_ZL3ambi, .-_ZL3ambi
	.p2align 4,,15
	.type	_ZL5amb_bi, @function
_ZL5amb_bi:
.LFB10:
	# gdb.arch/amd64-entry-value.cc:91
	.loc 1 91 0
	.cfi_startproc
.LVL34:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	# gdb.arch/amd64-entry-value.cc:92
	.loc 1 92 0
	addl	$2, %edi
.LVL35:
	jmp	_ZL3ambi
.LVL36:
# SUCC: EXIT [100.0%]  (ab,sibcall)
	.cfi_endproc
.LFE10:
	.size	_ZL5amb_bi, .-_ZL5amb_bi
	.p2align 4,,15
	.type	_ZL5amb_ai, @function
_ZL5amb_ai:
.LFB11:
	# gdb.arch/amd64-entry-value.cc:97
	.loc 1 97 0
	.cfi_startproc
.LVL37:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	# gdb.arch/amd64-entry-value.cc:98
	.loc 1 98 0
	addl	$1, %edi
.LVL38:
	jmp	_ZL5amb_bi
.LVL39:
# SUCC: EXIT [100.0%]  (ab,sibcall)
	.cfi_endproc
.LFE11:
	.size	_ZL5amb_ai, .-_ZL5amb_ai
	.p2align 4,,15
	.type	_ZL4selfi, @function
_ZL4selfi:
.LFB13:
	# gdb.arch/amd64-entry-value.cc:111
	.loc 1 111 0
	.cfi_startproc
.LVL40:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	# gdb.arch/amd64-entry-value.cc:112
	.loc 1 112 0
	cmpl	$200, %edi
	# gdb.arch/amd64-entry-value.cc:111
	.loc 1 111 0
	pushq	%rbx
.LCFI0:
	.cfi_def_cfa_offset 16
	.cfi_offset 3, -16
	# gdb.arch/amd64-entry-value.cc:111
	.loc 1 111 0
	movl	%edi, %ebx
# SUCC: 4 [19.9%]  (can_fallthru) 3 [80.1%]  (fallthru,can_fallthru)
	# gdb.arch/amd64-entry-value.cc:112
	.loc 1 112 0
	je	.L18
# BLOCK 3 freq:8009 seq:1
# PRED: 2 [80.1%]  (fallthru,can_fallthru)
	# gdb.arch/amd64-entry-value.cc:119
	.loc 1 119 0
	movl	_ZL1v(%rip), %eax
	movl	_ZL1v(%rip), %edi
.LVL41:
	cvtsi2sd	%eax, %xmm0
	call	_ZL1eid
.LVL42:
	# gdb.arch/amd64-entry-value.cc:120
	.loc 1 120 0
	cvtsi2sd	%ebx, %xmm0
	leal	2(%rbx), %edi
	# gdb.arch/amd64-entry-value.cc:122
	.loc 1 122 0
	popq	%rbx
.LCFI1:
	.cfi_remember_state
	.cfi_def_cfa_offset 8
.LVL43:
	# gdb.arch/amd64-entry-value.cc:120
	.loc 1 120 0
	addsd	.LC4(%rip), %xmm0
	jmp	_ZL1did
.LVL44:
# SUCC: EXIT [100.0%]  (ab,sibcall)
# BLOCK 4 freq:1991 seq:2
# PRED: 2 [19.9%]  (can_fallthru)
	.p2align 4,,10
	.p2align 3
.L18:
.LCFI2:
	.cfi_restore_state
	# gdb.arch/amd64-entry-value.cc:122
	.loc 1 122 0
	popq	%rbx
.LCFI3:
	.cfi_def_cfa_offset 8
.LVL45:
	# gdb.arch/amd64-entry-value.cc:115
	.loc 1 115 0
	movl	$201, %edi
.LVL46:
	jmp	_ZL5self2i
# SUCC: EXIT [100.0%]  (ab,sibcall)
.LVL47:
	.cfi_endproc
.LFE13:
	.size	_ZL4selfi, .-_ZL4selfi
	.p2align 4,,15
	.type	_ZL5self2i, @function
_ZL5self2i:
.LFB12:
	# gdb.arch/amd64-entry-value.cc:105
	.loc 1 105 0
	.cfi_startproc
.LVL48:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	# gdb.arch/amd64-entry-value.cc:106
	.loc 1 106 0
	jmp	_ZL4selfi
.LVL49:
# SUCC: EXIT [100.0%]  (ab,sibcall)
	.cfi_endproc
.LFE12:
	.size	_ZL5self2i, .-_ZL5self2i
	.p2align 4,,15
	.type	_ZL9stacktestiiiiiiiidddddddddd, @function
_ZL9stacktestiiiiiiiidddddddddd:
.LFB14:
	# gdb.arch/amd64-entry-value.cc:128
	.loc 1 128 0
	.cfi_startproc
.LVL50:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	# gdb.arch/amd64-entry-value.cc:133
	.loc 1 133 0
	movl	_ZL1v(%rip), %eax
	movl	_ZL1v(%rip), %edi
.LVL51:
	cvtsi2sd	%eax, %xmm0
.LVL52:
	call	_ZL1eid
.LVL53:
	# gdb.arch/amd64-entry-value.cc:134
	.loc 1 134 0
#APP
# 134 "gdb.arch/amd64-entry-value.cc" 1
	breakhere_stacktest:
# 0 "" 2
	# gdb.arch/amd64-entry-value.cc:135
	.loc 1 135 0
#NO_APP
	movl	_ZL1v(%rip), %eax
	movl	_ZL1v(%rip), %edi
	cvtsi2sd	%eax, %xmm0
	jmp	_ZL1eid
# SUCC: EXIT [100.0%]  (ab,sibcall)
.LVL54:
	.cfi_endproc
.LFE14:
	.size	_ZL9stacktestiiiiiiiidddddddddd, .-_ZL9stacktestiiiiiiiidddddddddd
	.p2align 4,,15
	.type	_ZL9referenceRiS_iiiiS_S_, @function
_ZL9referenceRiS_iiiiS_S_:
.LFB15:
	# gdb.arch/amd64-entry-value.cc:145
	.loc 1 145 0
	.cfi_startproc
.LVL55:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
.LBB2:
	# gdb.arch/amd64-entry-value.cc:151
	.loc 1 151 0
	movq	8(%rsp), %rax
	# gdb.arch/amd64-entry-value.cc:149
	.loc 1 149 0
	movl	$21, (%rdi)
	# gdb.arch/amd64-entry-value.cc:150
	.loc 1 150 0
	movl	$22, (%rsi)
	# gdb.arch/amd64-entry-value.cc:151
	.loc 1 151 0
	movl	$31, (%rax)
	# gdb.arch/amd64-entry-value.cc:152
	.loc 1 152 0
	movq	16(%rsp), %rax
	movl	$32, (%rax)
	# gdb.arch/amd64-entry-value.cc:153
	.loc 1 153 0
	movl	_ZL1v(%rip), %eax
	movl	_ZL1v(%rip), %edi
.LVL56:
	cvtsi2sd	%eax, %xmm0
	call	_ZL1eid
.LVL57:
	# gdb.arch/amd64-entry-value.cc:154
	.loc 1 154 0
#APP
# 154 "gdb.arch/amd64-entry-value.cc" 1
	breakhere_reference:
# 0 "" 2
	# gdb.arch/amd64-entry-value.cc:155
	.loc 1 155 0
#NO_APP
	movl	_ZL1v(%rip), %eax
	movl	_ZL1v(%rip), %edi
	cvtsi2sd	%eax, %xmm0
	jmp	_ZL1eid
# SUCC: EXIT [100.0%]  (ab,sibcall)
.LVL58:
.LBE2:
	.cfi_endproc
.LFE15:
	.size	_ZL9referenceRiS_iiiiS_S_, .-_ZL9referenceRiS_iiiiS_S_
	.p2align 4,,15
	.type	_ZL5datapv, @function
_ZL5datapv:
.LFB16:
	# gdb.arch/amd64-entry-value.cc:160
	.loc 1 160 0
	.cfi_startproc
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	# gdb.arch/amd64-entry-value.cc:164
	.loc 1 164 0
	movl	$_ZZL5datapvE3two, %eax
# SUCC: EXIT [100.0%] 
	ret
	.cfi_endproc
.LFE16:
	.size	_ZL5datapv, .-_ZL5datapv
	.p2align 4,,15
	.type	_ZL11datap_inputPi, @function
_ZL11datap_inputPi:
.LFB17:
	# gdb.arch/amd64-entry-value.cc:168
	.loc 1 168 0
	.cfi_startproc
.LVL59:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	# gdb.arch/amd64-entry-value.cc:169
	.loc 1 169 0
	addl	$1, (%rdi)
# SUCC: EXIT [100.0%] 
	# gdb.arch/amd64-entry-value.cc:170
	.loc 1 170 0
	ret
	.cfi_endproc
.LFE17:
	.size	_ZL11datap_inputPi, .-_ZL11datap_inputPi
	.p2align 4,,15
	.type	_ZL4datav, @function
_ZL4datav:
.LFB18:
	# gdb.arch/amd64-entry-value.cc:174
	.loc 1 174 0
	.cfi_startproc
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	# gdb.arch/amd64-entry-value.cc:176
	.loc 1 176 0
	movl	$10, %eax
# SUCC: EXIT [100.0%] 
	ret
	.cfi_endproc
.LFE18:
	.size	_ZL4datav, .-_ZL4datav
	.p2align 4,,15
	.type	_ZL5data2v, @function
_ZL5data2v:
.LFB19:
	# gdb.arch/amd64-entry-value.cc:180
	.loc 1 180 0
	.cfi_startproc
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	# gdb.arch/amd64-entry-value.cc:182
	.loc 1 182 0
	movl	$20, %eax
# SUCC: EXIT [100.0%] 
	ret
	.cfi_endproc
.LFE19:
	.size	_ZL5data2v, .-_ZL5data2v
	.p2align 4,,15
	.type	_ZL9differenti, @function
_ZL9differenti:
.LFB20:
	# gdb.arch/amd64-entry-value.cc:186
	.loc 1 186 0
	.cfi_startproc
.LVL60:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	pushq	%rbx
.LCFI4:
	.cfi_def_cfa_offset 16
	.cfi_offset 3, -16
	# gdb.arch/amd64-entry-value.cc:187
	.loc 1 187 0
	leal	1(%rdi), %ebx
.LVL61:
	# gdb.arch/amd64-entry-value.cc:188
	.loc 1 188 0
	cvtsi2sd	%ebx, %xmm0
	movl	%ebx, %edi
	call	_ZL1eid
.LVL62:
	# gdb.arch/amd64-entry-value.cc:189
	.loc 1 189 0
#APP
# 189 "gdb.arch/amd64-entry-value.cc" 1
	breakhere_different:
# 0 "" 2
	# gdb.arch/amd64-entry-value.cc:191
	.loc 1 191 0
#NO_APP
	movl	%ebx, %eax
	popq	%rbx
.LCFI5:
	.cfi_def_cfa_offset 8
.LVL63:
# SUCC: EXIT [100.0%] 
	ret
	.cfi_endproc
.LFE20:
	.size	_ZL9differenti, .-_ZL9differenti
	.p2align 4,,15
	.type	_ZL8validityii, @function
_ZL8validityii:
.LFB21:
	# gdb.arch/amd64-entry-value.cc:195
	.loc 1 195 0
	.cfi_startproc
.LVL64:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	# gdb.arch/amd64-entry-value.cc:197
	.loc 1 197 0
	xorpd	%xmm0, %xmm0
	# gdb.arch/amd64-entry-value.cc:195
	.loc 1 195 0
	pushq	%rbx
.LCFI6:
	.cfi_def_cfa_offset 16
	.cfi_offset 3, -16
	# gdb.arch/amd64-entry-value.cc:197
	.loc 1 197 0
	xorl	%edi, %edi
	# gdb.arch/amd64-entry-value.cc:195
	.loc 1 195 0
	movl	%esi, %ebx
	# gdb.arch/amd64-entry-value.cc:197
	.loc 1 197 0
	call	_ZL1eid
.LVL65:
	# gdb.arch/amd64-entry-value.cc:198
	.loc 1 198 0
#APP
# 198 "gdb.arch/amd64-entry-value.cc" 1
	breakhere_validity:
# 0 "" 2
	# gdb.arch/amd64-entry-value.cc:200
	.loc 1 200 0
#NO_APP
	movl	%ebx, %eax
	popq	%rbx
.LCFI7:
	.cfi_def_cfa_offset 8
.LVL66:
# SUCC: EXIT [100.0%] 
	ret
	.cfi_endproc
.LFE21:
	.size	_ZL8validityii, .-_ZL8validityii
	.p2align 4,,15
	.type	_ZL7invalidi, @function
_ZL7invalidi:
.LFB22:
	# gdb.arch/amd64-entry-value.cc:204
	.loc 1 204 0
	.cfi_startproc
.LVL67:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	# gdb.arch/amd64-entry-value.cc:205
	.loc 1 205 0
	xorpd	%xmm0, %xmm0
	xorl	%edi, %edi
.LVL68:
	call	_ZL1eid
.LVL69:
	# gdb.arch/amd64-entry-value.cc:206
	.loc 1 206 0
#APP
# 206 "gdb.arch/amd64-entry-value.cc" 1
	breakhere_invalid:
# 0 "" 2
# SUCC: EXIT [100.0%] 
	# gdb.arch/amd64-entry-value.cc:207
	.loc 1 207 0
#NO_APP
	ret
	.cfi_endproc
.LFE22:
	.size	_ZL7invalidi, .-_ZL7invalidi
	.section	.text.startup,"ax",@progbits
	.p2align 4,,15
	.globl	main
	.type	main, @function
main:
.LFB23:
	# gdb.arch/amd64-entry-value.cc:211
	.loc 1 211 0
	.cfi_startproc
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	pushq	%rbx
.LCFI8:
	.cfi_def_cfa_offset 16
.LBB3:
	# gdb.arch/amd64-entry-value.cc:212
	.loc 1 212 0
	movl	$30, %edi
.LBE3:
	# gdb.arch/amd64-entry-value.cc:211
	.loc 1 211 0
	subq	$48, %rsp
.LCFI9:
	.cfi_def_cfa_offset 64
	.cfi_offset 3, -16
.LBB5:
	# gdb.arch/amd64-entry-value.cc:212
	.loc 1 212 0
	movsd	.LC6(%rip), %xmm0
	call	_ZL1did
.LVL70:
	# gdb.arch/amd64-entry-value.cc:213
	.loc 1 213 0
	movl	$30, %edi
	call	_ZL7locexpri
.LVL71:
	# gdb.arch/amd64-entry-value.cc:215
	.loc 1 215 0
	movsd	.LC7(%rip), %xmm7
	movabsq	$4623226492472524800, %rax
	movsd	.LC3(%rip), %xmm6
	movabsq	$4622663542519103488, %rdx
	movsd	.LC8(%rip), %xmm5
	movl	$6, %r9d
	movsd	.LC9(%rip), %xmm4
	movl	$5, %r8d
	movsd	.LC10(%rip), %xmm3
	movl	$4, %ecx
	movsd	.LC11(%rip), %xmm2
	movl	$2, %esi
	movsd	.LC4(%rip), %xmm1
	movq	%rax, 24(%rsp)
	movsd	.LC12(%rip), %xmm0
	movq	%rdx, 16(%rsp)
	movl	$1, %edi
	movl	$3, %edx
	movl	$12, 8(%rsp)
	movl	$11, (%rsp)
	call	_ZL9stacktestiiiiiiiidddddddddd
.LVL72:
	# gdb.arch/amd64-entry-value.cc:216
	.loc 1 216 0
	movl	$5, %edi
	call	_ZL9differenti
.LVL73:
	# gdb.arch/amd64-entry-value.cc:217
	.loc 1 217 0
	call	_ZL4datav
.LVL74:
	movl	$5, %edi
	movl	%eax, %esi
	call	_ZL8validityii
.LVL75:
	# gdb.arch/amd64-entry-value.cc:218
	.loc 1 218 0
	call	_ZL5data2v
.LVL76:
	movl	%eax, %edi
	call	_ZL7invalidi
.LVL77:
.LBB4:
	# gdb.arch/amd64-entry-value.cc:221
	.loc 1 221 0
	movl	$1, 36(%rsp)
.LVL78:
	call	_ZL5datapv
.LVL79:
	movq	%rax, %rbx
.LVL80:
	# gdb.arch/amd64-entry-value.cc:222
	.loc 1 222 0
	leaq	44(%rsp), %rax
.LVL81:
	leaq	36(%rsp), %rdi
	movl	$6, %r9d
	movl	$5, %r8d
	movl	$4, %ecx
	movq	%rax, 8(%rsp)
	leaq	40(%rsp), %rax
	movl	$3, %edx
	movq	%rbx, %rsi
	# gdb.arch/amd64-entry-value.cc:221
	.loc 1 221 0
	movl	$11, 40(%rsp)
.LVL82:
	movl	$12, 44(%rsp)
.LVL83:
	# gdb.arch/amd64-entry-value.cc:222
	.loc 1 222 0
	movq	%rax, (%rsp)
	call	_ZL9referenceRiS_iiiiS_S_
.LVL84:
	# gdb.arch/amd64-entry-value.cc:223
	.loc 1 223 0
	movq	%rbx, %rdi
	call	_ZL11datap_inputPi
.LVL85:
.LBE4:
	# gdb.arch/amd64-entry-value.cc:226
	.loc 1 226 0
	movl	_ZL1v(%rip), %eax
	testl	%eax, %eax
# SUCC: 5 [39.0%]  (can_fallthru) 3 [61.0%]  (fallthru,can_fallthru)
	jne	.L32
# BLOCK 3 freq:6100 seq:1
# PRED: 2 [61.0%]  (fallthru,can_fallthru)
	# gdb.arch/amd64-entry-value.cc:229
	.loc 1 229 0
	movsd	.LC16(%rip), %xmm0
	movl	$5, %edi
	call	_ZL1bid
# SUCC: 4 [100.0%]  (fallthru,can_fallthru)
.LVL86:
# BLOCK 4 freq:10000 seq:2
# PRED: 3 [100.0%]  (fallthru,can_fallthru) 5 [100.0%] 
.L31:
	# gdb.arch/amd64-entry-value.cc:230
	.loc 1 230 0
	movl	$100, %edi
	call	_ZL5amb_ai
.LVL87:
	# gdb.arch/amd64-entry-value.cc:231
	.loc 1 231 0
	movl	$200, %edi
	call	_ZL4selfi
.LVL88:
.LBE5:
	# gdb.arch/amd64-entry-value.cc:233
	.loc 1 233 0
	addq	$48, %rsp
.LCFI10:
	.cfi_remember_state
	.cfi_def_cfa_offset 16
	xorl	%eax, %eax
	popq	%rbx
.LCFI11:
	.cfi_def_cfa_offset 8
.LVL89:
# SUCC: EXIT [100.0%] 
	ret
.LVL90:
# BLOCK 5 freq:3900 seq:3
# PRED: 2 [39.0%]  (can_fallthru)
.L32:
.LCFI12:
	.cfi_restore_state
.LBB6:
	# gdb.arch/amd64-entry-value.cc:227
	.loc 1 227 0
	movsd	.LC15(%rip), %xmm0
	movl	$1, %edi
	call	_ZL1aid
.LVL91:
# SUCC: 4 [100.0%] 
	jmp	.L31
.LBE6:
	.cfi_endproc
.LFE23:
	.size	main, .-main
	.local	_ZL1v
	.comm	_ZL1v,4,4
	.data
	.align 4
	.type	_ZZL5datapvE3two, @object
	.size	_ZZL5datapvE3two, 4
_ZZL5datapvE3two:
	.long	2
	.section	.rodata.cst8,"aM",@progbits,8
	.align 8
.LC0:
	.long	0
	.long	1072693248
	.align 8
.LC1:
	.long	0
	.long	1076101120
	.align 8
.LC2:
	.long	0
	.long	1073741824
	.align 8
.LC3:
	.long	0
	.long	1075707904
	.align 8
.LC4:
	.long	0
	.long	1074003968
	.align 8
.LC6:
	.long	0
	.long	1077837824
	.align 8
.LC7:
	.long	0
	.long	1075904512
	.align 8
.LC8:
	.long	0
	.long	1075445760
	.align 8
.LC9:
	.long	0
	.long	1075183616
	.align 8
.LC10:
	.long	0
	.long	1074921472
	.align 8
.LC11:
	.long	0
	.long	1074528256
	.align 8
.LC12:
	.long	0
	.long	1073217536
	.align 8
.LC15:
	.long	0
	.long	1072955392
	.align 8
.LC16:
	.long	0
	.long	1075118080
	.text
.Letext0:
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.long	0xba0	# Length of Compilation Unit Info
	.value	0x2	# DWARF version number
	.long	.Ldebug_abbrev0	# Offset Into Abbrev. Section
	.byte	0x8	# Pointer Size (in bytes)
	.uleb128 0x1	# (DIE (0xb) DW_TAG_compile_unit)
	.long	.LASF1	# DW_AT_producer: "GNU C++ 4.7.0 20110912 (experimental)"
	.byte	0x4	# DW_AT_language
	.long	.LASF2	# DW_AT_name: "gdb.arch/amd64-entry-value.cc"
	.long	.LASF3	# DW_AT_comp_dir: ""
	.long	.Ldebug_ranges0+0	# DW_AT_ranges
	.quad	0	# DW_AT_low_pc
	.quad	0	# DW_AT_entry_pc
	.long	.Ldebug_line0	# DW_AT_stmt_list
	.uleb128 0x2	# (DIE (0x31) DW_TAG_base_type)
	.byte	0x8	# DW_AT_byte_size
	.byte	0x4	# DW_AT_encoding
	.long	.LASF0	# DW_AT_name: "double"
	.uleb128 0x3	# (DIE (0x38) DW_TAG_base_type)
	.byte	0x4	# DW_AT_byte_size
	.byte	0x5	# DW_AT_encoding
	.ascii "int\0"	# DW_AT_name
	.uleb128 0x4	# (DIE (0x3f) DW_TAG_subprogram)
	.ascii "e\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x15	# DW_AT_decl_line
	.quad	.LFB0	# DW_AT_low_pc
	.quad	.LFE0	# DW_AT_high_pc
	.byte	0x2	# DW_AT_frame_base
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.long	0x73	# DW_AT_sibling
	.uleb128 0x5	# (DIE (0x5c) DW_TAG_formal_parameter)
	.ascii "i\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x15	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.uleb128 0x5	# (DIE (0x67) DW_TAG_formal_parameter)
	.ascii "j\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x15	# DW_AT_decl_line
	.long	0x31	# DW_AT_type
	.byte	0x1	# DW_AT_location
	.byte	0x61	# DW_OP_reg17
	.byte	0	# end of children of DIE 0x3f
	.uleb128 0x4	# (DIE (0x73) DW_TAG_subprogram)
	.ascii "d\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x1b	# DW_AT_decl_line
	.quad	.LFB1	# DW_AT_low_pc
	.quad	.LFE1	# DW_AT_high_pc
	.byte	0x2	# DW_AT_frame_base
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.long	0xf6	# DW_AT_sibling
	.uleb128 0x6	# (DIE (0x90) DW_TAG_formal_parameter)
	.ascii "i\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x1b	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.long	.LLST0	# DW_AT_location
	.uleb128 0x6	# (DIE (0x9d) DW_TAG_formal_parameter)
	.ascii "j\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x1b	# DW_AT_decl_line
	.long	0x31	# DW_AT_type
	.long	.LLST1	# DW_AT_location
	.uleb128 0x7	# (DIE (0xaa) DW_TAG_GNU_call_site)
	.quad	.LVL4	# DW_AT_low_pc
	.long	0x3f	# DW_AT_abstract_origin
	.long	0xda	# DW_AT_sibling
	.uleb128 0x8	# (DIE (0xbb) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x5	# DW_AT_GNU_call_site_value
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x55	# DW_OP_reg5
	.byte	0x23	# DW_OP_plus_uconst
	.uleb128 0x1
	.uleb128 0x8	# (DIE (0xc4) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x61	# DW_OP_reg17
	.byte	0x11	# DW_AT_GNU_call_site_value
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x3
	.byte	0xf5	# DW_OP_GNU_regval_type
	.uleb128 0x11
	.uleb128 0x31
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x31
	.byte	0x8
	.long	0	# fp or vector constant word 0
	.long	0x3ff00000	# fp or vector constant word 1
	.byte	0x22	# DW_OP_plus
	.byte	0	# end of children of DIE 0xaa
	.uleb128 0x9	# (DIE (0xda) DW_TAG_GNU_call_site)
	.quad	.LVL5	# DW_AT_low_pc
	.long	0x3f	# DW_AT_abstract_origin
	.uleb128 0xa	# (DIE (0xe7) DW_TAG_GNU_call_site)
	.quad	.LVL6	# DW_AT_low_pc
	.byte	0x1	# DW_AT_GNU_tail_call
	.long	0x3f	# DW_AT_abstract_origin
	.byte	0	# end of children of DIE 0x73
	.uleb128 0xb	# (DIE (0xf6) DW_TAG_subprogram)
	.long	.LASF4	# DW_AT_name: "locexpr"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x26	# DW_AT_decl_line
	.quad	.LFB2	# DW_AT_low_pc
	.quad	.LFE2	# DW_AT_high_pc
	.byte	0x2	# DW_AT_frame_base
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.long	0x121	# DW_AT_sibling
	.uleb128 0x5	# (DIE (0x115) DW_TAG_formal_parameter)
	.ascii "i\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x26	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0	# end of children of DIE 0xf6
	.uleb128 0x4	# (DIE (0x121) DW_TAG_subprogram)
	.ascii "c\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x2d	# DW_AT_decl_line
	.quad	.LFB3	# DW_AT_low_pc
	.quad	.LFE3	# DW_AT_high_pc
	.byte	0x2	# DW_AT_frame_base
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.long	0x188	# DW_AT_sibling
	.uleb128 0x6	# (DIE (0x13e) DW_TAG_formal_parameter)
	.ascii "i\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x2d	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.long	.LLST2	# DW_AT_location
	.uleb128 0x6	# (DIE (0x14b) DW_TAG_formal_parameter)
	.ascii "j\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x2d	# DW_AT_decl_line
	.long	0x31	# DW_AT_type
	.long	.LLST3	# DW_AT_location
	.uleb128 0xc	# (DIE (0x158) DW_TAG_GNU_call_site)
	.quad	.LVL11	# DW_AT_low_pc
	.byte	0x1	# DW_AT_GNU_tail_call
	.long	0x73	# DW_AT_abstract_origin
	.uleb128 0x8	# (DIE (0x166) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x7	# DW_AT_GNU_call_site_value
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x55	# DW_OP_reg5
	.byte	0x35	# DW_OP_lit5
	.byte	0x1e	# DW_OP_mul
	.byte	0x31	# DW_OP_lit1
	.byte	0x24	# DW_OP_shl
	.uleb128 0x8	# (DIE (0x171) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x61	# DW_OP_reg17
	.byte	0x11	# DW_AT_GNU_call_site_value
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x3
	.byte	0xf5	# DW_OP_GNU_regval_type
	.uleb128 0x11
	.uleb128 0x31
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x31
	.byte	0x8
	.long	0	# fp or vector constant word 0
	.long	0x40240000	# fp or vector constant word 1
	.byte	0x1e	# DW_OP_mul
	.byte	0	# end of children of DIE 0x158
	.byte	0	# end of children of DIE 0x121
	.uleb128 0x4	# (DIE (0x188) DW_TAG_subprogram)
	.ascii "a\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x33	# DW_AT_decl_line
	.quad	.LFB4	# DW_AT_low_pc
	.quad	.LFE4	# DW_AT_high_pc
	.byte	0x2	# DW_AT_frame_base
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.long	0x1ed	# DW_AT_sibling
	.uleb128 0x6	# (DIE (0x1a5) DW_TAG_formal_parameter)
	.ascii "i\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x33	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.long	.LLST4	# DW_AT_location
	.uleb128 0x6	# (DIE (0x1b2) DW_TAG_formal_parameter)
	.ascii "j\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x33	# DW_AT_decl_line
	.long	0x31	# DW_AT_type
	.long	.LLST5	# DW_AT_location
	.uleb128 0xc	# (DIE (0x1bf) DW_TAG_GNU_call_site)
	.quad	.LVL15	# DW_AT_low_pc
	.byte	0x1	# DW_AT_GNU_tail_call
	.long	0x121	# DW_AT_abstract_origin
	.uleb128 0x8	# (DIE (0x1cd) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x5	# DW_AT_GNU_call_site_value
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x55	# DW_OP_reg5
	.byte	0x23	# DW_OP_plus_uconst
	.uleb128 0x1
	.uleb128 0x8	# (DIE (0x1d6) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x61	# DW_OP_reg17
	.byte	0x11	# DW_AT_GNU_call_site_value
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x3
	.byte	0xf5	# DW_OP_GNU_regval_type
	.uleb128 0x11
	.uleb128 0x31
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x31
	.byte	0x8
	.long	0	# fp or vector constant word 0
	.long	0x3ff00000	# fp or vector constant word 1
	.byte	0x22	# DW_OP_plus
	.byte	0	# end of children of DIE 0x1bf
	.byte	0	# end of children of DIE 0x188
	.uleb128 0x4	# (DIE (0x1ed) DW_TAG_subprogram)
	.ascii "b\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x39	# DW_AT_decl_line
	.quad	.LFB5	# DW_AT_low_pc
	.quad	.LFE5	# DW_AT_high_pc
	.byte	0x2	# DW_AT_frame_base
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.long	0x252	# DW_AT_sibling
	.uleb128 0x6	# (DIE (0x20a) DW_TAG_formal_parameter)
	.ascii "i\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x39	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.long	.LLST6	# DW_AT_location
	.uleb128 0x6	# (DIE (0x217) DW_TAG_formal_parameter)
	.ascii "j\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x39	# DW_AT_decl_line
	.long	0x31	# DW_AT_type
	.long	.LLST7	# DW_AT_location
	.uleb128 0xc	# (DIE (0x224) DW_TAG_GNU_call_site)
	.quad	.LVL19	# DW_AT_low_pc
	.byte	0x1	# DW_AT_GNU_tail_call
	.long	0x121	# DW_AT_abstract_origin
	.uleb128 0x8	# (DIE (0x232) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x5	# DW_AT_GNU_call_site_value
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x55	# DW_OP_reg5
	.byte	0x23	# DW_OP_plus_uconst
	.uleb128 0x2
	.uleb128 0x8	# (DIE (0x23b) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x61	# DW_OP_reg17
	.byte	0x11	# DW_AT_GNU_call_site_value
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x3
	.byte	0xf5	# DW_OP_GNU_regval_type
	.uleb128 0x11
	.uleb128 0x31
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x31
	.byte	0x8
	.long	0	# fp or vector constant word 0
	.long	0x40000000	# fp or vector constant word 1
	.byte	0x22	# DW_OP_plus
	.byte	0	# end of children of DIE 0x224
	.byte	0	# end of children of DIE 0x1ed
	.uleb128 0xb	# (DIE (0x252) DW_TAG_subprogram)
	.long	.LASF5	# DW_AT_name: "amb_z"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x3f	# DW_AT_decl_line
	.quad	.LFB6	# DW_AT_low_pc
	.quad	.LFE6	# DW_AT_high_pc
	.byte	0x2	# DW_AT_frame_base
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.long	0x2ae	# DW_AT_sibling
	.uleb128 0x6	# (DIE (0x271) DW_TAG_formal_parameter)
	.ascii "i\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x3f	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.long	.LLST8	# DW_AT_location
	.uleb128 0xc	# (DIE (0x27e) DW_TAG_GNU_call_site)
	.quad	.LVL22	# DW_AT_low_pc
	.byte	0x1	# DW_AT_GNU_tail_call
	.long	0x73	# DW_AT_abstract_origin
	.uleb128 0x8	# (DIE (0x28c) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x5	# DW_AT_GNU_call_site_value
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x55	# DW_OP_reg5
	.byte	0x23	# DW_OP_plus_uconst
	.uleb128 0x7
	.uleb128 0x8	# (DIE (0x295) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x61	# DW_OP_reg17
	.byte	0x13	# DW_AT_GNU_call_site_value
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x55	# DW_OP_reg5
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x38
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x31
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x31
	.byte	0x8
	.long	0	# fp or vector constant word 0
	.long	0x401e0000	# fp or vector constant word 1
	.byte	0x22	# DW_OP_plus
	.byte	0	# end of children of DIE 0x27e
	.byte	0	# end of children of DIE 0x252
	.uleb128 0xb	# (DIE (0x2ae) DW_TAG_subprogram)
	.long	.LASF6	# DW_AT_name: "amb_y"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x45	# DW_AT_decl_line
	.quad	.LFB7	# DW_AT_low_pc
	.quad	.LFE7	# DW_AT_high_pc
	.byte	0x2	# DW_AT_frame_base
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.long	0x2f3	# DW_AT_sibling
	.uleb128 0x6	# (DIE (0x2cd) DW_TAG_formal_parameter)
	.ascii "i\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x45	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.long	.LLST9	# DW_AT_location
	.uleb128 0xc	# (DIE (0x2da) DW_TAG_GNU_call_site)
	.quad	.LVL25	# DW_AT_low_pc
	.byte	0x1	# DW_AT_GNU_tail_call
	.long	0x252	# DW_AT_abstract_origin
	.uleb128 0x8	# (DIE (0x2e8) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x5	# DW_AT_GNU_call_site_value
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x55	# DW_OP_reg5
	.byte	0x23	# DW_OP_plus_uconst
	.uleb128 0x6
	.byte	0	# end of children of DIE 0x2da
	.byte	0	# end of children of DIE 0x2ae
	.uleb128 0xb	# (DIE (0x2f3) DW_TAG_subprogram)
	.long	.LASF7	# DW_AT_name: "amb_x"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x4b	# DW_AT_decl_line
	.quad	.LFB8	# DW_AT_low_pc
	.quad	.LFE8	# DW_AT_high_pc
	.byte	0x2	# DW_AT_frame_base
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.long	0x338	# DW_AT_sibling
	.uleb128 0x6	# (DIE (0x312) DW_TAG_formal_parameter)
	.ascii "i\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x4b	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.long	.LLST10	# DW_AT_location
	.uleb128 0xc	# (DIE (0x31f) DW_TAG_GNU_call_site)
	.quad	.LVL28	# DW_AT_low_pc
	.byte	0x1	# DW_AT_GNU_tail_call
	.long	0x2ae	# DW_AT_abstract_origin
	.uleb128 0x8	# (DIE (0x32d) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x5	# DW_AT_GNU_call_site_value
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x55	# DW_OP_reg5
	.byte	0x23	# DW_OP_plus_uconst
	.uleb128 0x5
	.byte	0	# end of children of DIE 0x31f
	.byte	0	# end of children of DIE 0x2f3
	.uleb128 0x4	# (DIE (0x338) DW_TAG_subprogram)
	.ascii "amb\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x51	# DW_AT_decl_line
	.quad	.LFB9	# DW_AT_low_pc
	.quad	.LFE9	# DW_AT_high_pc
	.byte	0x2	# DW_AT_frame_base
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.long	0x399	# DW_AT_sibling
	.uleb128 0x6	# (DIE (0x357) DW_TAG_formal_parameter)
	.ascii "i\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x51	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.long	.LLST11	# DW_AT_location
	.uleb128 0xd	# (DIE (0x364) DW_TAG_GNU_call_site)
	.quad	.LVL31	# DW_AT_low_pc
	.byte	0x1	# DW_AT_GNU_tail_call
	.long	0x2f3	# DW_AT_abstract_origin
	.long	0x380	# DW_AT_sibling
	.uleb128 0x8	# (DIE (0x376) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x5	# DW_AT_GNU_call_site_value
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x55	# DW_OP_reg5
	.byte	0x23	# DW_OP_plus_uconst
	.uleb128 0x4
	.byte	0	# end of children of DIE 0x364
	.uleb128 0xc	# (DIE (0x380) DW_TAG_GNU_call_site)
	.quad	.LVL33	# DW_AT_low_pc
	.byte	0x1	# DW_AT_GNU_tail_call
	.long	0x2f3	# DW_AT_abstract_origin
	.uleb128 0x8	# (DIE (0x38e) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x5	# DW_AT_GNU_call_site_value
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x55	# DW_OP_reg5
	.byte	0x23	# DW_OP_plus_uconst
	.uleb128 0x3
	.byte	0	# end of children of DIE 0x380
	.byte	0	# end of children of DIE 0x338
	.uleb128 0xb	# (DIE (0x399) DW_TAG_subprogram)
	.long	.LASF8	# DW_AT_name: "amb_b"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x5a	# DW_AT_decl_line
	.quad	.LFB10	# DW_AT_low_pc
	.quad	.LFE10	# DW_AT_high_pc
	.byte	0x2	# DW_AT_frame_base
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.long	0x3de	# DW_AT_sibling
	.uleb128 0x6	# (DIE (0x3b8) DW_TAG_formal_parameter)
	.ascii "i\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x5a	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.long	.LLST12	# DW_AT_location
	.uleb128 0xc	# (DIE (0x3c5) DW_TAG_GNU_call_site)
	.quad	.LVL36	# DW_AT_low_pc
	.byte	0x1	# DW_AT_GNU_tail_call
	.long	0x338	# DW_AT_abstract_origin
	.uleb128 0x8	# (DIE (0x3d3) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x5	# DW_AT_GNU_call_site_value
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x55	# DW_OP_reg5
	.byte	0x23	# DW_OP_plus_uconst
	.uleb128 0x2
	.byte	0	# end of children of DIE 0x3c5
	.byte	0	# end of children of DIE 0x399
	.uleb128 0xb	# (DIE (0x3de) DW_TAG_subprogram)
	.long	.LASF9	# DW_AT_name: "amb_a"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x60	# DW_AT_decl_line
	.quad	.LFB11	# DW_AT_low_pc
	.quad	.LFE11	# DW_AT_high_pc
	.byte	0x2	# DW_AT_frame_base
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.long	0x423	# DW_AT_sibling
	.uleb128 0x6	# (DIE (0x3fd) DW_TAG_formal_parameter)
	.ascii "i\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x60	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.long	.LLST13	# DW_AT_location
	.uleb128 0xc	# (DIE (0x40a) DW_TAG_GNU_call_site)
	.quad	.LVL39	# DW_AT_low_pc
	.byte	0x1	# DW_AT_GNU_tail_call
	.long	0x399	# DW_AT_abstract_origin
	.uleb128 0x8	# (DIE (0x418) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x5	# DW_AT_GNU_call_site_value
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x55	# DW_OP_reg5
	.byte	0x23	# DW_OP_plus_uconst
	.uleb128 0x1
	.byte	0	# end of children of DIE 0x40a
	.byte	0	# end of children of DIE 0x3de
	.uleb128 0xe	# (DIE (0x423) DW_TAG_subprogram)
	.long	.LASF10	# DW_AT_name: "self"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x6e	# DW_AT_decl_line
	.quad	.LFB13	# DW_AT_low_pc
	.quad	.LFE13	# DW_AT_high_pc
	.long	.LLST14	# DW_AT_frame_base
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.long	0x4a6	# DW_AT_sibling
	.uleb128 0x6	# (DIE (0x443) DW_TAG_formal_parameter)
	.ascii "i\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x6e	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.long	.LLST15	# DW_AT_location
	.uleb128 0x9	# (DIE (0x450) DW_TAG_GNU_call_site)
	.quad	.LVL42	# DW_AT_low_pc
	.long	0x3f	# DW_AT_abstract_origin
	.uleb128 0xd	# (DIE (0x45d) DW_TAG_GNU_call_site)
	.quad	.LVL44	# DW_AT_low_pc
	.byte	0x1	# DW_AT_GNU_tail_call
	.long	0x73	# DW_AT_abstract_origin
	.long	0x490	# DW_AT_sibling
	.uleb128 0x8	# (DIE (0x46f) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x5	# DW_AT_GNU_call_site_value
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x55	# DW_OP_reg5
	.byte	0x23	# DW_OP_plus_uconst
	.uleb128 0x2
	.uleb128 0x8	# (DIE (0x478) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x61	# DW_OP_reg17
	.byte	0x13	# DW_AT_GNU_call_site_value
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x55	# DW_OP_reg5
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x38
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x31
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x31
	.byte	0x8
	.long	0	# fp or vector constant word 0
	.long	0x40040000	# fp or vector constant word 1
	.byte	0x22	# DW_OP_plus
	.byte	0	# end of children of DIE 0x45d
	.uleb128 0xc	# (DIE (0x490) DW_TAG_GNU_call_site)
	.quad	.LVL47	# DW_AT_low_pc
	.byte	0x1	# DW_AT_GNU_tail_call
	.long	0x4a6	# DW_AT_abstract_origin
	.uleb128 0x8	# (DIE (0x49e) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x2	# DW_AT_GNU_call_site_value
	.byte	0x8	# DW_OP_const1u
	.byte	0xc9
	.byte	0	# end of children of DIE 0x490
	.byte	0	# end of children of DIE 0x423
	.uleb128 0xb	# (DIE (0x4a6) DW_TAG_subprogram)
	.long	.LASF11	# DW_AT_name: "self2"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x68	# DW_AT_decl_line
	.quad	.LFB12	# DW_AT_low_pc
	.quad	.LFE12	# DW_AT_high_pc
	.byte	0x2	# DW_AT_frame_base
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.long	0x4e9	# DW_AT_sibling
	.uleb128 0x6	# (DIE (0x4c5) DW_TAG_formal_parameter)
	.ascii "i\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x68	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.long	.LLST16	# DW_AT_location
	.uleb128 0xc	# (DIE (0x4d2) DW_TAG_GNU_call_site)
	.quad	.LVL49	# DW_AT_low_pc
	.byte	0x1	# DW_AT_GNU_tail_call
	.long	0x423	# DW_AT_abstract_origin
	.uleb128 0x8	# (DIE (0x4e0) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x3	# DW_AT_GNU_call_site_value
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x55	# DW_OP_reg5
	.byte	0	# end of children of DIE 0x4d2
	.byte	0	# end of children of DIE 0x4a6
	.uleb128 0xb	# (DIE (0x4e9) DW_TAG_subprogram)
	.long	.LASF12	# DW_AT_name: "stacktest"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x7d	# DW_AT_decl_line
	.quad	.LFB14	# DW_AT_low_pc
	.quad	.LFE14	# DW_AT_high_pc
	.byte	0x2	# DW_AT_frame_base
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.long	0x620	# DW_AT_sibling
	.uleb128 0x6	# (DIE (0x508) DW_TAG_formal_parameter)
	.ascii "r1\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x7d	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.long	.LLST17	# DW_AT_location
	.uleb128 0x6	# (DIE (0x516) DW_TAG_formal_parameter)
	.ascii "r2\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x7d	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.long	.LLST18	# DW_AT_location
	.uleb128 0x6	# (DIE (0x524) DW_TAG_formal_parameter)
	.ascii "r3\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x7d	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.long	.LLST19	# DW_AT_location
	.uleb128 0x6	# (DIE (0x532) DW_TAG_formal_parameter)
	.ascii "r4\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x7d	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.long	.LLST20	# DW_AT_location
	.uleb128 0x6	# (DIE (0x540) DW_TAG_formal_parameter)
	.ascii "r5\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x7d	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.long	.LLST21	# DW_AT_location
	.uleb128 0x6	# (DIE (0x54e) DW_TAG_formal_parameter)
	.ascii "r6\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x7d	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.long	.LLST22	# DW_AT_location
	.uleb128 0x6	# (DIE (0x55c) DW_TAG_formal_parameter)
	.ascii "s1\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x7d	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.long	.LLST23	# DW_AT_location
	.uleb128 0x6	# (DIE (0x56a) DW_TAG_formal_parameter)
	.ascii "s2\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x7d	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.long	.LLST24	# DW_AT_location
	.uleb128 0x6	# (DIE (0x578) DW_TAG_formal_parameter)
	.ascii "d1\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x7e	# DW_AT_decl_line
	.long	0x31	# DW_AT_type
	.long	.LLST25	# DW_AT_location
	.uleb128 0x6	# (DIE (0x586) DW_TAG_formal_parameter)
	.ascii "d2\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x7e	# DW_AT_decl_line
	.long	0x31	# DW_AT_type
	.long	.LLST26	# DW_AT_location
	.uleb128 0x6	# (DIE (0x594) DW_TAG_formal_parameter)
	.ascii "d3\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x7e	# DW_AT_decl_line
	.long	0x31	# DW_AT_type
	.long	.LLST27	# DW_AT_location
	.uleb128 0x6	# (DIE (0x5a2) DW_TAG_formal_parameter)
	.ascii "d4\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x7e	# DW_AT_decl_line
	.long	0x31	# DW_AT_type
	.long	.LLST28	# DW_AT_location
	.uleb128 0x6	# (DIE (0x5b0) DW_TAG_formal_parameter)
	.ascii "d5\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x7e	# DW_AT_decl_line
	.long	0x31	# DW_AT_type
	.long	.LLST29	# DW_AT_location
	.uleb128 0x6	# (DIE (0x5be) DW_TAG_formal_parameter)
	.ascii "d6\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x7e	# DW_AT_decl_line
	.long	0x31	# DW_AT_type
	.long	.LLST30	# DW_AT_location
	.uleb128 0x6	# (DIE (0x5cc) DW_TAG_formal_parameter)
	.ascii "d7\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x7f	# DW_AT_decl_line
	.long	0x31	# DW_AT_type
	.long	.LLST31	# DW_AT_location
	.uleb128 0x6	# (DIE (0x5da) DW_TAG_formal_parameter)
	.ascii "d8\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x7f	# DW_AT_decl_line
	.long	0x31	# DW_AT_type
	.long	.LLST32	# DW_AT_location
	.uleb128 0x6	# (DIE (0x5e8) DW_TAG_formal_parameter)
	.ascii "d9\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x7f	# DW_AT_decl_line
	.long	0x31	# DW_AT_type
	.long	.LLST33	# DW_AT_location
	.uleb128 0x6	# (DIE (0x5f6) DW_TAG_formal_parameter)
	.ascii "da\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x7f	# DW_AT_decl_line
	.long	0x31	# DW_AT_type
	.long	.LLST34	# DW_AT_location
	.uleb128 0x9	# (DIE (0x604) DW_TAG_GNU_call_site)
	.quad	.LVL53	# DW_AT_low_pc
	.long	0x3f	# DW_AT_abstract_origin
	.uleb128 0xa	# (DIE (0x611) DW_TAG_GNU_call_site)
	.quad	.LVL54	# DW_AT_low_pc
	.byte	0x1	# DW_AT_GNU_tail_call
	.long	0x3f	# DW_AT_abstract_origin
	.byte	0	# end of children of DIE 0x4e9
	.uleb128 0xb	# (DIE (0x620) DW_TAG_subprogram)
	.long	.LASF13	# DW_AT_name: "reference"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x8f	# DW_AT_decl_line
	.quad	.LFB15	# DW_AT_low_pc
	.quad	.LFE15	# DW_AT_high_pc
	.byte	0x2	# DW_AT_frame_base
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.long	0x723	# DW_AT_sibling
	.uleb128 0xf	# (DIE (0x63f) DW_TAG_formal_parameter)
	.long	.LASF14	# DW_AT_name: "regparam"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x8f	# DW_AT_decl_line
	.long	0x723	# DW_AT_type
	.long	.LLST35	# DW_AT_location
	.uleb128 0xf	# (DIE (0x64e) DW_TAG_formal_parameter)
	.long	.LASF15	# DW_AT_name: "nodataparam"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x8f	# DW_AT_decl_line
	.long	0x72e	# DW_AT_type
	.long	.LLST36	# DW_AT_location
	.uleb128 0x6	# (DIE (0x65d) DW_TAG_formal_parameter)
	.ascii "r3\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x8f	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.long	.LLST37	# DW_AT_location
	.uleb128 0x6	# (DIE (0x66b) DW_TAG_formal_parameter)
	.ascii "r4\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x8f	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.long	.LLST38	# DW_AT_location
	.uleb128 0x6	# (DIE (0x679) DW_TAG_formal_parameter)
	.ascii "r5\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x8f	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.long	.LLST39	# DW_AT_location
	.uleb128 0x6	# (DIE (0x687) DW_TAG_formal_parameter)
	.ascii "r6\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x8f	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.long	.LLST40	# DW_AT_location
	.uleb128 0x10	# (DIE (0x695) DW_TAG_formal_parameter)
	.long	.LASF16	# DW_AT_name: "stackparam1"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x90	# DW_AT_decl_line
	.long	0x733	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.uleb128 0x10	# (DIE (0x6a3) DW_TAG_formal_parameter)
	.long	.LASF17	# DW_AT_name: "stackparam2"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x90	# DW_AT_decl_line
	.long	0x738	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 8
	.uleb128 0x11	# (DIE (0x6b1) DW_TAG_lexical_block)
	.quad	.LBB2	# DW_AT_low_pc
	.quad	.LBE2	# DW_AT_high_pc
	.uleb128 0x12	# (DIE (0x6c2) DW_TAG_variable)
	.long	.LASF18	# DW_AT_name: "regcopy"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x92	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.byte	0x7	# DW_AT_location
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x4
	.byte	0x75	# DW_OP_breg5
	.sleb128 0
	.byte	0x94	# DW_OP_deref_size
	.byte	0x4
	.byte	0x9f	# DW_OP_stack_value
	.uleb128 0x12	# (DIE (0x6d5) DW_TAG_variable)
	.long	.LASF19	# DW_AT_name: "nodatacopy"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x92	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.byte	0x7	# DW_AT_location
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x4
	.byte	0x74	# DW_OP_breg4
	.sleb128 0
	.byte	0x94	# DW_OP_deref_size
	.byte	0x4
	.byte	0x9f	# DW_OP_stack_value
	.uleb128 0x13	# (DIE (0x6e8) DW_TAG_variable)
	.long	.LASF20	# DW_AT_name: "stackcopy1"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x93	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.long	.LLST41	# DW_AT_location
	.uleb128 0x13	# (DIE (0x6f7) DW_TAG_variable)
	.long	.LASF21	# DW_AT_name: "stackcopy2"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x93	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.long	.LLST42	# DW_AT_location
	.uleb128 0x9	# (DIE (0x706) DW_TAG_GNU_call_site)
	.quad	.LVL57	# DW_AT_low_pc
	.long	0x3f	# DW_AT_abstract_origin
	.uleb128 0xa	# (DIE (0x713) DW_TAG_GNU_call_site)
	.quad	.LVL58	# DW_AT_low_pc
	.byte	0x1	# DW_AT_GNU_tail_call
	.long	0x3f	# DW_AT_abstract_origin
	.byte	0	# end of children of DIE 0x6b1
	.byte	0	# end of children of DIE 0x620
	.uleb128 0x14	# (DIE (0x723) DW_TAG_const_type)
	.long	0x728	# DW_AT_type
	.uleb128 0x15	# (DIE (0x728) DW_TAG_reference_type)
	.byte	0x8	# DW_AT_byte_size
	.long	0x38	# DW_AT_type
	.uleb128 0x14	# (DIE (0x72e) DW_TAG_const_type)
	.long	0x728	# DW_AT_type
	.uleb128 0x14	# (DIE (0x733) DW_TAG_const_type)
	.long	0x728	# DW_AT_type
	.uleb128 0x14	# (DIE (0x738) DW_TAG_const_type)
	.long	0x728	# DW_AT_type
	.uleb128 0x16	# (DIE (0x73d) DW_TAG_subprogram)
	.long	.LASF23	# DW_AT_name: "datap"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x9f	# DW_AT_decl_line
	.long	0x75c	# DW_AT_type
	.quad	.LFB16	# DW_AT_low_pc
	.quad	.LFE16	# DW_AT_high_pc
	.byte	0x2	# DW_AT_frame_base
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.uleb128 0x17	# (DIE (0x75c) DW_TAG_pointer_type)
	.byte	0x8	# DW_AT_byte_size
	.long	0x38	# DW_AT_type
	.uleb128 0xb	# (DIE (0x762) DW_TAG_subprogram)
	.long	.LASF22	# DW_AT_name: "datap_input"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0xa7	# DW_AT_decl_line
	.quad	.LFB17	# DW_AT_low_pc
	.quad	.LFE17	# DW_AT_high_pc
	.byte	0x2	# DW_AT_frame_base
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.long	0x78f	# DW_AT_sibling
	.uleb128 0x10	# (DIE (0x781) DW_TAG_formal_parameter)
	.long	.LASF23	# DW_AT_name: "datap"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0xa7	# DW_AT_decl_line
	.long	0x75c	# DW_AT_type
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0	# end of children of DIE 0x762
	.uleb128 0x16	# (DIE (0x78f) DW_TAG_subprogram)
	.long	.LASF24	# DW_AT_name: "data"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0xad	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.quad	.LFB18	# DW_AT_low_pc
	.quad	.LFE18	# DW_AT_high_pc
	.byte	0x2	# DW_AT_frame_base
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.uleb128 0x16	# (DIE (0x7ae) DW_TAG_subprogram)
	.long	.LASF25	# DW_AT_name: "data2"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0xb3	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.quad	.LFB19	# DW_AT_low_pc
	.quad	.LFE19	# DW_AT_high_pc
	.byte	0x2	# DW_AT_frame_base
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.uleb128 0x18	# (DIE (0x7cd) DW_TAG_subprogram)
	.long	.LASF26	# DW_AT_name: "different"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0xb9	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.quad	.LFB20	# DW_AT_low_pc
	.quad	.LFE20	# DW_AT_high_pc
	.long	.LLST43	# DW_AT_frame_base
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.long	0x81f	# DW_AT_sibling
	.uleb128 0x6	# (DIE (0x7f1) DW_TAG_formal_parameter)
	.ascii "val\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0xb9	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.long	.LLST44	# DW_AT_location
	.uleb128 0x19	# (DIE (0x800) DW_TAG_GNU_call_site)
	.quad	.LVL62	# DW_AT_low_pc
	.long	0x3f	# DW_AT_abstract_origin
	.uleb128 0x8	# (DIE (0x80d) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x2	# DW_AT_GNU_call_site_value
	.byte	0x73	# DW_OP_breg3
	.sleb128 0
	.uleb128 0x8	# (DIE (0x813) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x61	# DW_OP_reg17
	.byte	0x6	# DW_AT_GNU_call_site_value
	.byte	0x73	# DW_OP_breg3
	.sleb128 0
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x38
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x31
	.byte	0	# end of children of DIE 0x800
	.byte	0	# end of children of DIE 0x7cd
	.uleb128 0x18	# (DIE (0x81f) DW_TAG_subprogram)
	.long	.LASF27	# DW_AT_name: "validity"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0xc2	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.quad	.LFB21	# DW_AT_low_pc
	.quad	.LFE21	# DW_AT_high_pc
	.long	.LLST45	# DW_AT_frame_base
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.long	0x884	# DW_AT_sibling
	.uleb128 0xf	# (DIE (0x843) DW_TAG_formal_parameter)
	.long	.LASF28	# DW_AT_name: "lost"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0xc2	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.long	.LLST46	# DW_AT_location
	.uleb128 0xf	# (DIE (0x852) DW_TAG_formal_parameter)
	.long	.LASF29	# DW_AT_name: "born"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0xc2	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.long	.LLST47	# DW_AT_location
	.uleb128 0x19	# (DIE (0x861) DW_TAG_GNU_call_site)
	.quad	.LVL65	# DW_AT_low_pc
	.long	0x3f	# DW_AT_abstract_origin
	.uleb128 0x8	# (DIE (0x86e) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x1	# DW_AT_GNU_call_site_value
	.byte	0x30	# DW_OP_lit0
	.uleb128 0x8	# (DIE (0x873) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x61	# DW_OP_reg17
	.byte	0xb	# DW_AT_GNU_call_site_value
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x31
	.byte	0x8
	.long	0	# fp or vector constant word 0
	.long	0	# fp or vector constant word 1
	.byte	0	# end of children of DIE 0x861
	.byte	0	# end of children of DIE 0x81f
	.uleb128 0xb	# (DIE (0x884) DW_TAG_subprogram)
	.long	.LASF30	# DW_AT_name: "invalid"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0xcb	# DW_AT_decl_line
	.quad	.LFB22	# DW_AT_low_pc
	.quad	.LFE22	# DW_AT_high_pc
	.byte	0x2	# DW_AT_frame_base
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.long	0x8d5	# DW_AT_sibling
	.uleb128 0x6	# (DIE (0x8a3) DW_TAG_formal_parameter)
	.ascii "inv\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0xcb	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.long	.LLST48	# DW_AT_location
	.uleb128 0x19	# (DIE (0x8b2) DW_TAG_GNU_call_site)
	.quad	.LVL69	# DW_AT_low_pc
	.long	0x3f	# DW_AT_abstract_origin
	.uleb128 0x8	# (DIE (0x8bf) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x1	# DW_AT_GNU_call_site_value
	.byte	0x30	# DW_OP_lit0
	.uleb128 0x8	# (DIE (0x8c4) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x61	# DW_OP_reg17
	.byte	0xb	# DW_AT_GNU_call_site_value
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x31
	.byte	0x8
	.long	0	# fp or vector constant word 0
	.long	0	# fp or vector constant word 1
	.byte	0	# end of children of DIE 0x8b2
	.byte	0	# end of children of DIE 0x884
	.uleb128 0x1a	# (DIE (0x8d5) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.long	.LASF35	# DW_AT_name: "main"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0xd2	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.quad	.LFB23	# DW_AT_low_pc
	.quad	.LFE23	# DW_AT_high_pc
	.long	.LLST49	# DW_AT_frame_base
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.long	0xb8b	# DW_AT_sibling
	.uleb128 0x1b	# (DIE (0x8fa) DW_TAG_lexical_block)
	.quad	.LBB4	# DW_AT_low_pc
	.quad	.LBE4	# DW_AT_high_pc
	.long	0x9ac	# DW_AT_sibling
	.uleb128 0x12	# (DIE (0x90f) DW_TAG_variable)
	.long	.LASF31	# DW_AT_name: "regvar"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0xdd	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 -28
	.uleb128 0x13	# (DIE (0x91d) DW_TAG_variable)
	.long	.LASF32	# DW_AT_name: "nodatavarp"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0xdd	# DW_AT_decl_line
	.long	0x75c	# DW_AT_type
	.long	.LLST50	# DW_AT_location
	.uleb128 0x12	# (DIE (0x92c) DW_TAG_variable)
	.long	.LASF33	# DW_AT_name: "stackvar1"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0xdd	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 -24
	.uleb128 0x12	# (DIE (0x93a) DW_TAG_variable)
	.long	.LASF34	# DW_AT_name: "stackvar2"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0xdd	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 -20
	.uleb128 0x9	# (DIE (0x948) DW_TAG_GNU_call_site)
	.quad	.LVL79	# DW_AT_low_pc
	.long	0x73d	# DW_AT_abstract_origin
	.uleb128 0x7	# (DIE (0x955) DW_TAG_GNU_call_site)
	.quad	.LVL84	# DW_AT_low_pc
	.long	0x620	# DW_AT_abstract_origin
	.long	0x997	# DW_AT_sibling
	.uleb128 0x1c	# (DIE (0x966) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x2	# DW_AT_GNU_call_site_value
	.byte	0x91	# DW_OP_fbreg
	.sleb128 -28
	.byte	0x1	# DW_AT_GNU_call_site_data_value
	.byte	0x31	# DW_OP_lit1
	.uleb128 0x8	# (DIE (0x96e) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x54	# DW_OP_reg4
	.byte	0x2	# DW_AT_GNU_call_site_value
	.byte	0x73	# DW_OP_breg3
	.sleb128 0
	.uleb128 0x8	# (DIE (0x974) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x51	# DW_OP_reg1
	.byte	0x1	# DW_AT_GNU_call_site_value
	.byte	0x33	# DW_OP_lit3
	.uleb128 0x8	# (DIE (0x979) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x52	# DW_OP_reg2
	.byte	0x1	# DW_AT_GNU_call_site_value
	.byte	0x34	# DW_OP_lit4
	.uleb128 0x8	# (DIE (0x97e) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x58	# DW_OP_reg8
	.byte	0x1	# DW_AT_GNU_call_site_value
	.byte	0x35	# DW_OP_lit5
	.uleb128 0x8	# (DIE (0x983) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x59	# DW_OP_reg9
	.byte	0x1	# DW_AT_GNU_call_site_value
	.byte	0x36	# DW_OP_lit6
	.uleb128 0x8	# (DIE (0x988) DW_TAG_GNU_call_site_parameter)
	.byte	0x2	# DW_AT_location
	.byte	0x77	# DW_OP_breg7
	.sleb128 0
	.byte	0x2	# DW_AT_GNU_call_site_value
	.byte	0x91	# DW_OP_fbreg
	.sleb128 -24
	.uleb128 0x8	# (DIE (0x98f) DW_TAG_GNU_call_site_parameter)
	.byte	0x2	# DW_AT_location
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.byte	0x2	# DW_AT_GNU_call_site_value
	.byte	0x91	# DW_OP_fbreg
	.sleb128 -20
	.byte	0	# end of children of DIE 0x955
	.uleb128 0x19	# (DIE (0x997) DW_TAG_GNU_call_site)
	.quad	.LVL85	# DW_AT_low_pc
	.long	0x762	# DW_AT_abstract_origin
	.uleb128 0x8	# (DIE (0x9a4) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x2	# DW_AT_GNU_call_site_value
	.byte	0x73	# DW_OP_breg3
	.sleb128 0
	.byte	0	# end of children of DIE 0x997
	.byte	0	# end of children of DIE 0x8fa
	.uleb128 0x7	# (DIE (0x9ac) DW_TAG_GNU_call_site)
	.quad	.LVL70	# DW_AT_low_pc
	.long	0x73	# DW_AT_abstract_origin
	.long	0x9d2	# DW_AT_sibling
	.uleb128 0x8	# (DIE (0x9bd) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x1	# DW_AT_GNU_call_site_value
	.byte	0x4e	# DW_OP_lit30
	.uleb128 0x8	# (DIE (0x9c2) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x61	# DW_OP_reg17
	.byte	0xb	# DW_AT_GNU_call_site_value
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x31
	.byte	0x8
	.long	0	# fp or vector constant word 0
	.long	0x403e8000	# fp or vector constant word 1
	.byte	0	# end of children of DIE 0x9ac
	.uleb128 0x7	# (DIE (0x9d2) DW_TAG_GNU_call_site)
	.quad	.LVL71	# DW_AT_low_pc
	.long	0xf6	# DW_AT_abstract_origin
	.long	0x9e9	# DW_AT_sibling
	.uleb128 0x8	# (DIE (0x9e3) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x1	# DW_AT_GNU_call_site_value
	.byte	0x4e	# DW_OP_lit30
	.byte	0	# end of children of DIE 0x9d2
	.uleb128 0x7	# (DIE (0x9e9) DW_TAG_GNU_call_site)
	.quad	.LVL72	# DW_AT_low_pc
	.long	0x4e9	# DW_AT_abstract_origin
	.long	0xabd	# DW_AT_sibling
	.uleb128 0x8	# (DIE (0x9fa) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x1	# DW_AT_GNU_call_site_value
	.byte	0x31	# DW_OP_lit1
	.uleb128 0x8	# (DIE (0x9ff) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x54	# DW_OP_reg4
	.byte	0x1	# DW_AT_GNU_call_site_value
	.byte	0x32	# DW_OP_lit2
	.uleb128 0x8	# (DIE (0xa04) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x51	# DW_OP_reg1
	.byte	0x1	# DW_AT_GNU_call_site_value
	.byte	0x33	# DW_OP_lit3
	.uleb128 0x8	# (DIE (0xa09) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x52	# DW_OP_reg2
	.byte	0x1	# DW_AT_GNU_call_site_value
	.byte	0x34	# DW_OP_lit4
	.uleb128 0x8	# (DIE (0xa0e) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x58	# DW_OP_reg8
	.byte	0x1	# DW_AT_GNU_call_site_value
	.byte	0x35	# DW_OP_lit5
	.uleb128 0x8	# (DIE (0xa13) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x59	# DW_OP_reg9
	.byte	0x1	# DW_AT_GNU_call_site_value
	.byte	0x36	# DW_OP_lit6
	.uleb128 0x8	# (DIE (0xa18) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x61	# DW_OP_reg17
	.byte	0xb	# DW_AT_GNU_call_site_value
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x31
	.byte	0x8
	.long	0	# fp or vector constant word 0
	.long	0x3ff80000	# fp or vector constant word 1
	.uleb128 0x8	# (DIE (0xa27) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x62	# DW_OP_reg18
	.byte	0xb	# DW_AT_GNU_call_site_value
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x31
	.byte	0x8
	.long	0	# fp or vector constant word 0
	.long	0x40040000	# fp or vector constant word 1
	.uleb128 0x8	# (DIE (0xa36) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x63	# DW_OP_reg19
	.byte	0xb	# DW_AT_GNU_call_site_value
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x31
	.byte	0x8
	.long	0	# fp or vector constant word 0
	.long	0x400c0000	# fp or vector constant word 1
	.uleb128 0x8	# (DIE (0xa45) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x64	# DW_OP_reg20
	.byte	0xb	# DW_AT_GNU_call_site_value
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x31
	.byte	0x8
	.long	0	# fp or vector constant word 0
	.long	0x40120000	# fp or vector constant word 1
	.uleb128 0x8	# (DIE (0xa54) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x65	# DW_OP_reg21
	.byte	0xb	# DW_AT_GNU_call_site_value
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x31
	.byte	0x8
	.long	0	# fp or vector constant word 0
	.long	0x40160000	# fp or vector constant word 1
	.uleb128 0x8	# (DIE (0xa63) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x66	# DW_OP_reg22
	.byte	0xb	# DW_AT_GNU_call_site_value
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x31
	.byte	0x8
	.long	0	# fp or vector constant word 0
	.long	0x401a0000	# fp or vector constant word 1
	.uleb128 0x8	# (DIE (0xa72) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x67	# DW_OP_reg23
	.byte	0xb	# DW_AT_GNU_call_site_value
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x31
	.byte	0x8
	.long	0	# fp or vector constant word 0
	.long	0x401e0000	# fp or vector constant word 1
	.uleb128 0x8	# (DIE (0xa81) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x68	# DW_OP_reg24
	.byte	0xb	# DW_AT_GNU_call_site_value
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x31
	.byte	0x8
	.long	0	# fp or vector constant word 0
	.long	0x40210000	# fp or vector constant word 1
	.uleb128 0x8	# (DIE (0xa90) DW_TAG_GNU_call_site_parameter)
	.byte	0x2	# DW_AT_location
	.byte	0x77	# DW_OP_breg7
	.sleb128 0
	.byte	0x1	# DW_AT_GNU_call_site_value
	.byte	0x3b	# DW_OP_lit11
	.uleb128 0x8	# (DIE (0xa96) DW_TAG_GNU_call_site_parameter)
	.byte	0x2	# DW_AT_location
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.byte	0x1	# DW_AT_GNU_call_site_value
	.byte	0x3c	# DW_OP_lit12
	.uleb128 0x8	# (DIE (0xa9c) DW_TAG_GNU_call_site_parameter)
	.byte	0x2	# DW_AT_location
	.byte	0x77	# DW_OP_breg7
	.sleb128 16
	.byte	0xb	# DW_AT_GNU_call_site_value
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x31
	.byte	0x8
	.long	0	# fp or vector constant word 0
	.long	0x40270000	# fp or vector constant word 1
	.uleb128 0x8	# (DIE (0xaac) DW_TAG_GNU_call_site_parameter)
	.byte	0x2	# DW_AT_location
	.byte	0x77	# DW_OP_breg7
	.sleb128 24
	.byte	0xb	# DW_AT_GNU_call_site_value
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x31
	.byte	0x8
	.long	0	# fp or vector constant word 0
	.long	0x40290000	# fp or vector constant word 1
	.byte	0	# end of children of DIE 0x9e9
	.uleb128 0x7	# (DIE (0xabd) DW_TAG_GNU_call_site)
	.quad	.LVL73	# DW_AT_low_pc
	.long	0x7cd	# DW_AT_abstract_origin
	.long	0xad4	# DW_AT_sibling
	.uleb128 0x8	# (DIE (0xace) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x1	# DW_AT_GNU_call_site_value
	.byte	0x35	# DW_OP_lit5
	.byte	0	# end of children of DIE 0xabd
	.uleb128 0x9	# (DIE (0xad4) DW_TAG_GNU_call_site)
	.quad	.LVL74	# DW_AT_low_pc
	.long	0x78f	# DW_AT_abstract_origin
	.uleb128 0x7	# (DIE (0xae1) DW_TAG_GNU_call_site)
	.quad	.LVL75	# DW_AT_low_pc
	.long	0x81f	# DW_AT_abstract_origin
	.long	0xaf8	# DW_AT_sibling
	.uleb128 0x8	# (DIE (0xaf2) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x1	# DW_AT_GNU_call_site_value
	.byte	0x35	# DW_OP_lit5
	.byte	0	# end of children of DIE 0xae1
	.uleb128 0x9	# (DIE (0xaf8) DW_TAG_GNU_call_site)
	.quad	.LVL76	# DW_AT_low_pc
	.long	0x7ae	# DW_AT_abstract_origin
	.uleb128 0x9	# (DIE (0xb05) DW_TAG_GNU_call_site)
	.quad	.LVL77	# DW_AT_low_pc
	.long	0x884	# DW_AT_abstract_origin
	.uleb128 0x7	# (DIE (0xb12) DW_TAG_GNU_call_site)
	.quad	.LVL86	# DW_AT_low_pc
	.long	0x1ed	# DW_AT_abstract_origin
	.long	0xb38	# DW_AT_sibling
	.uleb128 0x8	# (DIE (0xb23) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x1	# DW_AT_GNU_call_site_value
	.byte	0x35	# DW_OP_lit5
	.uleb128 0x8	# (DIE (0xb28) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x61	# DW_OP_reg17
	.byte	0xb	# DW_AT_GNU_call_site_value
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x31
	.byte	0x8
	.long	0	# fp or vector constant word 0
	.long	0x40150000	# fp or vector constant word 1
	.byte	0	# end of children of DIE 0xb12
	.uleb128 0x7	# (DIE (0xb38) DW_TAG_GNU_call_site)
	.quad	.LVL87	# DW_AT_low_pc
	.long	0x3de	# DW_AT_abstract_origin
	.long	0xb50	# DW_AT_sibling
	.uleb128 0x8	# (DIE (0xb49) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x2	# DW_AT_GNU_call_site_value
	.byte	0x8	# DW_OP_const1u
	.byte	0x64
	.byte	0	# end of children of DIE 0xb38
	.uleb128 0x7	# (DIE (0xb50) DW_TAG_GNU_call_site)
	.quad	.LVL88	# DW_AT_low_pc
	.long	0x423	# DW_AT_abstract_origin
	.long	0xb68	# DW_AT_sibling
	.uleb128 0x8	# (DIE (0xb61) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x2	# DW_AT_GNU_call_site_value
	.byte	0x8	# DW_OP_const1u
	.byte	0xc8
	.byte	0	# end of children of DIE 0xb50
	.uleb128 0x19	# (DIE (0xb68) DW_TAG_GNU_call_site)
	.quad	.LVL91	# DW_AT_low_pc
	.long	0x188	# DW_AT_abstract_origin
	.uleb128 0x8	# (DIE (0xb75) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x1	# DW_AT_GNU_call_site_value
	.byte	0x31	# DW_OP_lit1
	.uleb128 0x8	# (DIE (0xb7a) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x61	# DW_OP_reg17
	.byte	0xb	# DW_AT_GNU_call_site_value
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x31
	.byte	0x8
	.long	0	# fp or vector constant word 0
	.long	0x3ff40000	# fp or vector constant word 1
	.byte	0	# end of children of DIE 0xb68
	.byte	0	# end of children of DIE 0x8d5
	.uleb128 0x1d	# (DIE (0xb8b) DW_TAG_variable)
	.ascii "v\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value.cc)
	.byte	0x12	# DW_AT_decl_line
	.long	0xb9e	# DW_AT_type
	.byte	0x9	# DW_AT_location
	.byte	0x3	# DW_OP_addr
	.quad	_ZL1v
	.uleb128 0x1e	# (DIE (0xb9e) DW_TAG_volatile_type)
	.long	0x38	# DW_AT_type
	.byte	0	# end of children of DIE 0xb
	.section	.debug_abbrev,"",@progbits
.Ldebug_abbrev0:
	.uleb128 0x1	# (abbrev code)
	.uleb128 0x11	# (TAG: DW_TAG_compile_unit)
	.byte	0x1	# DW_children_yes
	.uleb128 0x25	# (DW_AT_producer)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x13	# (DW_AT_language)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x1b	# (DW_AT_comp_dir)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x55	# (DW_AT_ranges)
	.uleb128 0x6	# (DW_FORM_data4)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x52	# (DW_AT_entry_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x10	# (DW_AT_stmt_list)
	.uleb128 0x6	# (DW_FORM_data4)
	.byte	0
	.byte	0
	.uleb128 0x2	# (abbrev code)
	.uleb128 0x24	# (TAG: DW_TAG_base_type)
	.byte	0	# DW_children_no
	.uleb128 0xb	# (DW_AT_byte_size)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3e	# (DW_AT_encoding)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.byte	0
	.byte	0
	.uleb128 0x3	# (abbrev code)
	.uleb128 0x24	# (TAG: DW_TAG_base_type)
	.byte	0	# DW_children_no
	.uleb128 0xb	# (DW_AT_byte_size)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3e	# (DW_AT_encoding)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
	.byte	0
	.byte	0
	.uleb128 0x4	# (abbrev code)
	.uleb128 0x2e	# (TAG: DW_TAG_subprogram)
	.byte	0x1	# DW_children_yes
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x40	# (DW_AT_frame_base)
	.uleb128 0xa	# (DW_FORM_block1)
	.uleb128 0x2117	# (DW_AT_GNU_all_call_sites)
	.uleb128 0xc	# (DW_FORM_flag)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x5	# (abbrev code)
	.uleb128 0x5	# (TAG: DW_TAG_formal_parameter)
	.byte	0	# DW_children_no
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0xa	# (DW_FORM_block1)
	.byte	0
	.byte	0
	.uleb128 0x6	# (abbrev code)
	.uleb128 0x5	# (TAG: DW_TAG_formal_parameter)
	.byte	0	# DW_children_no
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0x6	# (DW_FORM_data4)
	.byte	0
	.byte	0
	.uleb128 0x7	# (abbrev code)
	.uleb128 0x4109	# (TAG: DW_TAG_GNU_call_site)
	.byte	0x1	# DW_children_yes
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x31	# (DW_AT_abstract_origin)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x8	# (abbrev code)
	.uleb128 0x410a	# (TAG: DW_TAG_GNU_call_site_parameter)
	.byte	0	# DW_children_no
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0xa	# (DW_FORM_block1)
	.uleb128 0x2111	# (DW_AT_GNU_call_site_value)
	.uleb128 0xa	# (DW_FORM_block1)
	.byte	0
	.byte	0
	.uleb128 0x9	# (abbrev code)
	.uleb128 0x4109	# (TAG: DW_TAG_GNU_call_site)
	.byte	0	# DW_children_no
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x31	# (DW_AT_abstract_origin)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0xa	# (abbrev code)
	.uleb128 0x4109	# (TAG: DW_TAG_GNU_call_site)
	.byte	0	# DW_children_no
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x2115	# (DW_AT_GNU_tail_call)
	.uleb128 0xc	# (DW_FORM_flag)
	.uleb128 0x31	# (DW_AT_abstract_origin)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0xb	# (abbrev code)
	.uleb128 0x2e	# (TAG: DW_TAG_subprogram)
	.byte	0x1	# DW_children_yes
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x40	# (DW_AT_frame_base)
	.uleb128 0xa	# (DW_FORM_block1)
	.uleb128 0x2117	# (DW_AT_GNU_all_call_sites)
	.uleb128 0xc	# (DW_FORM_flag)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0xc	# (abbrev code)
	.uleb128 0x4109	# (TAG: DW_TAG_GNU_call_site)
	.byte	0x1	# DW_children_yes
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x2115	# (DW_AT_GNU_tail_call)
	.uleb128 0xc	# (DW_FORM_flag)
	.uleb128 0x31	# (DW_AT_abstract_origin)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0xd	# (abbrev code)
	.uleb128 0x4109	# (TAG: DW_TAG_GNU_call_site)
	.byte	0x1	# DW_children_yes
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x2115	# (DW_AT_GNU_tail_call)
	.uleb128 0xc	# (DW_FORM_flag)
	.uleb128 0x31	# (DW_AT_abstract_origin)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0xe	# (abbrev code)
	.uleb128 0x2e	# (TAG: DW_TAG_subprogram)
	.byte	0x1	# DW_children_yes
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x40	# (DW_AT_frame_base)
	.uleb128 0x6	# (DW_FORM_data4)
	.uleb128 0x2117	# (DW_AT_GNU_all_call_sites)
	.uleb128 0xc	# (DW_FORM_flag)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0xf	# (abbrev code)
	.uleb128 0x5	# (TAG: DW_TAG_formal_parameter)
	.byte	0	# DW_children_no
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0x6	# (DW_FORM_data4)
	.byte	0
	.byte	0
	.uleb128 0x10	# (abbrev code)
	.uleb128 0x5	# (TAG: DW_TAG_formal_parameter)
	.byte	0	# DW_children_no
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0xa	# (DW_FORM_block1)
	.byte	0
	.byte	0
	.uleb128 0x11	# (abbrev code)
	.uleb128 0xb	# (TAG: DW_TAG_lexical_block)
	.byte	0x1	# DW_children_yes
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.byte	0
	.byte	0
	.uleb128 0x12	# (abbrev code)
	.uleb128 0x34	# (TAG: DW_TAG_variable)
	.byte	0	# DW_children_no
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0xa	# (DW_FORM_block1)
	.byte	0
	.byte	0
	.uleb128 0x13	# (abbrev code)
	.uleb128 0x34	# (TAG: DW_TAG_variable)
	.byte	0	# DW_children_no
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0x6	# (DW_FORM_data4)
	.byte	0
	.byte	0
	.uleb128 0x14	# (abbrev code)
	.uleb128 0x26	# (TAG: DW_TAG_const_type)
	.byte	0	# DW_children_no
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x15	# (abbrev code)
	.uleb128 0x10	# (TAG: DW_TAG_reference_type)
	.byte	0	# DW_children_no
	.uleb128 0xb	# (DW_AT_byte_size)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x16	# (abbrev code)
	.uleb128 0x2e	# (TAG: DW_TAG_subprogram)
	.byte	0	# DW_children_no
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x40	# (DW_AT_frame_base)
	.uleb128 0xa	# (DW_FORM_block1)
	.uleb128 0x2117	# (DW_AT_GNU_all_call_sites)
	.uleb128 0xc	# (DW_FORM_flag)
	.byte	0
	.byte	0
	.uleb128 0x17	# (abbrev code)
	.uleb128 0xf	# (TAG: DW_TAG_pointer_type)
	.byte	0	# DW_children_no
	.uleb128 0xb	# (DW_AT_byte_size)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x18	# (abbrev code)
	.uleb128 0x2e	# (TAG: DW_TAG_subprogram)
	.byte	0x1	# DW_children_yes
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x40	# (DW_AT_frame_base)
	.uleb128 0x6	# (DW_FORM_data4)
	.uleb128 0x2117	# (DW_AT_GNU_all_call_sites)
	.uleb128 0xc	# (DW_FORM_flag)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x19	# (abbrev code)
	.uleb128 0x4109	# (TAG: DW_TAG_GNU_call_site)
	.byte	0x1	# DW_children_yes
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x31	# (DW_AT_abstract_origin)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x1a	# (abbrev code)
	.uleb128 0x2e	# (TAG: DW_TAG_subprogram)
	.byte	0x1	# DW_children_yes
	.uleb128 0x3f	# (DW_AT_external)
	.uleb128 0xc	# (DW_FORM_flag)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x40	# (DW_AT_frame_base)
	.uleb128 0x6	# (DW_FORM_data4)
	.uleb128 0x2117	# (DW_AT_GNU_all_call_sites)
	.uleb128 0xc	# (DW_FORM_flag)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x1b	# (abbrev code)
	.uleb128 0xb	# (TAG: DW_TAG_lexical_block)
	.byte	0x1	# DW_children_yes
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x1c	# (abbrev code)
	.uleb128 0x410a	# (TAG: DW_TAG_GNU_call_site_parameter)
	.byte	0	# DW_children_no
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0xa	# (DW_FORM_block1)
	.uleb128 0x2111	# (DW_AT_GNU_call_site_value)
	.uleb128 0xa	# (DW_FORM_block1)
	.uleb128 0x2112	# (DW_AT_GNU_call_site_data_value)
	.uleb128 0xa	# (DW_FORM_block1)
	.byte	0
	.byte	0
	.uleb128 0x1d	# (abbrev code)
	.uleb128 0x34	# (TAG: DW_TAG_variable)
	.byte	0	# DW_children_no
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0xa	# (DW_FORM_block1)
	.byte	0
	.byte	0
	.uleb128 0x1e	# (abbrev code)
	.uleb128 0x35	# (TAG: DW_TAG_volatile_type)
	.byte	0	# DW_children_no
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_loc,"",@progbits
.Ldebug_loc0:
.LLST0:
	.quad	.LVL1	# Location list begin address (*.LLST0)
	.quad	.LVL1	# Location list end address (*.LLST0)
	.value	0x1	# Location expression size
	.byte	0x55	# DW_OP_reg5
	.quad	.LVL1	# Location list begin address (*.LLST0)
	.quad	.LVL3	# Location list end address (*.LLST0)
	.value	0x3	# Location expression size
	.byte	0x75	# DW_OP_breg5
	.sleb128 1
	.byte	0x9f	# DW_OP_stack_value
	.quad	.LVL3	# Location list begin address (*.LLST0)
	.quad	.LFE1	# Location list end address (*.LLST0)
	.value	0x6	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x55	# DW_OP_reg5
	.byte	0x23	# DW_OP_plus_uconst
	.uleb128 0x1
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST0)
	.quad	0	# Location list terminator end (*.LLST0)
.LLST1:
	.quad	.LVL1	# Location list begin address (*.LLST1)
	.quad	.LVL1	# Location list end address (*.LLST1)
	.value	0x1	# Location expression size
	.byte	0x61	# DW_OP_reg17
	.quad	.LVL1	# Location list begin address (*.LLST1)
	.quad	.LVL2	# Location list end address (*.LLST1)
	.value	0x10	# Location expression size
	.byte	0xf5	# DW_OP_GNU_regval_type
	.uleb128 0x11
	.uleb128 0x31
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x31
	.byte	0x8
	.long	0	# fp or vector constant word 0
	.long	0x3ff00000	# fp or vector constant word 1
	.byte	0x22	# DW_OP_plus
	.byte	0x9f	# DW_OP_stack_value
	.quad	.LVL2	# Location list begin address (*.LLST1)
	.quad	.LFE1	# Location list end address (*.LLST1)
	.value	0x12	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x3
	.byte	0xf5	# DW_OP_GNU_regval_type
	.uleb128 0x11
	.uleb128 0x31
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x31
	.byte	0x8
	.long	0	# fp or vector constant word 0
	.long	0x3ff00000	# fp or vector constant word 1
	.byte	0x22	# DW_OP_plus
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST1)
	.quad	0	# Location list terminator end (*.LLST1)
.LLST2:
	.quad	.LVL8	# Location list begin address (*.LLST2)
	.quad	.LVL10	# Location list end address (*.LLST2)
	.value	0x1	# Location expression size
	.byte	0x55	# DW_OP_reg5
	.quad	.LVL10	# Location list begin address (*.LLST2)
	.quad	.LFE3	# Location list end address (*.LLST2)
	.value	0x4	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x55	# DW_OP_reg5
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST2)
	.quad	0	# Location list terminator end (*.LLST2)
.LLST3:
	.quad	.LVL8	# Location list begin address (*.LLST3)
	.quad	.LVL9	# Location list end address (*.LLST3)
	.value	0x1	# Location expression size
	.byte	0x61	# DW_OP_reg17
	.quad	.LVL9	# Location list begin address (*.LLST3)
	.quad	.LFE3	# Location list end address (*.LLST3)
	.value	0x6	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x3
	.byte	0xf5	# DW_OP_GNU_regval_type
	.uleb128 0x11
	.uleb128 0x31
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST3)
	.quad	0	# Location list terminator end (*.LLST3)
.LLST4:
	.quad	.LVL12	# Location list begin address (*.LLST4)
	.quad	.LVL14	# Location list end address (*.LLST4)
	.value	0x1	# Location expression size
	.byte	0x55	# DW_OP_reg5
	.quad	.LVL14	# Location list begin address (*.LLST4)
	.quad	.LVL15-1	# Location list end address (*.LLST4)
	.value	0x3	# Location expression size
	.byte	0x75	# DW_OP_breg5
	.sleb128 -1
	.byte	0x9f	# DW_OP_stack_value
	.quad	.LVL15-1	# Location list begin address (*.LLST4)
	.quad	.LFE4	# Location list end address (*.LLST4)
	.value	0x4	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x55	# DW_OP_reg5
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST4)
	.quad	0	# Location list terminator end (*.LLST4)
.LLST5:
	.quad	.LVL12	# Location list begin address (*.LLST5)
	.quad	.LVL13	# Location list end address (*.LLST5)
	.value	0x1	# Location expression size
	.byte	0x61	# DW_OP_reg17
	.quad	.LVL13	# Location list begin address (*.LLST5)
	.quad	.LFE4	# Location list end address (*.LLST5)
	.value	0x6	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x3
	.byte	0xf5	# DW_OP_GNU_regval_type
	.uleb128 0x11
	.uleb128 0x31
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST5)
	.quad	0	# Location list terminator end (*.LLST5)
.LLST6:
	.quad	.LVL16	# Location list begin address (*.LLST6)
	.quad	.LVL18	# Location list end address (*.LLST6)
	.value	0x1	# Location expression size
	.byte	0x55	# DW_OP_reg5
	.quad	.LVL18	# Location list begin address (*.LLST6)
	.quad	.LVL19-1	# Location list end address (*.LLST6)
	.value	0x3	# Location expression size
	.byte	0x75	# DW_OP_breg5
	.sleb128 -2
	.byte	0x9f	# DW_OP_stack_value
	.quad	.LVL19-1	# Location list begin address (*.LLST6)
	.quad	.LFE5	# Location list end address (*.LLST6)
	.value	0x4	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x55	# DW_OP_reg5
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST6)
	.quad	0	# Location list terminator end (*.LLST6)
.LLST7:
	.quad	.LVL16	# Location list begin address (*.LLST7)
	.quad	.LVL17	# Location list end address (*.LLST7)
	.value	0x1	# Location expression size
	.byte	0x61	# DW_OP_reg17
	.quad	.LVL17	# Location list begin address (*.LLST7)
	.quad	.LFE5	# Location list end address (*.LLST7)
	.value	0x6	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x3
	.byte	0xf5	# DW_OP_GNU_regval_type
	.uleb128 0x11
	.uleb128 0x31
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST7)
	.quad	0	# Location list terminator end (*.LLST7)
.LLST8:
	.quad	.LVL20	# Location list begin address (*.LLST8)
	.quad	.LVL21	# Location list end address (*.LLST8)
	.value	0x1	# Location expression size
	.byte	0x55	# DW_OP_reg5
	.quad	.LVL21	# Location list begin address (*.LLST8)
	.quad	.LVL22-1	# Location list end address (*.LLST8)
	.value	0x3	# Location expression size
	.byte	0x75	# DW_OP_breg5
	.sleb128 -7
	.byte	0x9f	# DW_OP_stack_value
	.quad	.LVL22-1	# Location list begin address (*.LLST8)
	.quad	.LFE6	# Location list end address (*.LLST8)
	.value	0x4	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x55	# DW_OP_reg5
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST8)
	.quad	0	# Location list terminator end (*.LLST8)
.LLST9:
	.quad	.LVL23	# Location list begin address (*.LLST9)
	.quad	.LVL24	# Location list end address (*.LLST9)
	.value	0x1	# Location expression size
	.byte	0x55	# DW_OP_reg5
	.quad	.LVL24	# Location list begin address (*.LLST9)
	.quad	.LVL25-1	# Location list end address (*.LLST9)
	.value	0x3	# Location expression size
	.byte	0x75	# DW_OP_breg5
	.sleb128 -6
	.byte	0x9f	# DW_OP_stack_value
	.quad	.LVL25-1	# Location list begin address (*.LLST9)
	.quad	.LFE7	# Location list end address (*.LLST9)
	.value	0x4	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x55	# DW_OP_reg5
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST9)
	.quad	0	# Location list terminator end (*.LLST9)
.LLST10:
	.quad	.LVL26	# Location list begin address (*.LLST10)
	.quad	.LVL27	# Location list end address (*.LLST10)
	.value	0x1	# Location expression size
	.byte	0x55	# DW_OP_reg5
	.quad	.LVL27	# Location list begin address (*.LLST10)
	.quad	.LVL28-1	# Location list end address (*.LLST10)
	.value	0x3	# Location expression size
	.byte	0x75	# DW_OP_breg5
	.sleb128 -5
	.byte	0x9f	# DW_OP_stack_value
	.quad	.LVL28-1	# Location list begin address (*.LLST10)
	.quad	.LFE8	# Location list end address (*.LLST10)
	.value	0x4	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x55	# DW_OP_reg5
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST10)
	.quad	0	# Location list terminator end (*.LLST10)
.LLST11:
	.quad	.LVL29	# Location list begin address (*.LLST11)
	.quad	.LVL30	# Location list end address (*.LLST11)
	.value	0x1	# Location expression size
	.byte	0x55	# DW_OP_reg5
	.quad	.LVL30	# Location list begin address (*.LLST11)
	.quad	.LVL31-1	# Location list end address (*.LLST11)
	.value	0x3	# Location expression size
	.byte	0x75	# DW_OP_breg5
	.sleb128 -4
	.byte	0x9f	# DW_OP_stack_value
	.quad	.LVL31-1	# Location list begin address (*.LLST11)
	.quad	.LVL31	# Location list end address (*.LLST11)
	.value	0x4	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x55	# DW_OP_reg5
	.byte	0x9f	# DW_OP_stack_value
	.quad	.LVL31	# Location list begin address (*.LLST11)
	.quad	.LVL32	# Location list end address (*.LLST11)
	.value	0x1	# Location expression size
	.byte	0x55	# DW_OP_reg5
	.quad	.LVL32	# Location list begin address (*.LLST11)
	.quad	.LVL33-1	# Location list end address (*.LLST11)
	.value	0x3	# Location expression size
	.byte	0x75	# DW_OP_breg5
	.sleb128 -3
	.byte	0x9f	# DW_OP_stack_value
	.quad	.LVL33-1	# Location list begin address (*.LLST11)
	.quad	.LFE9	# Location list end address (*.LLST11)
	.value	0x4	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x55	# DW_OP_reg5
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST11)
	.quad	0	# Location list terminator end (*.LLST11)
.LLST12:
	.quad	.LVL34	# Location list begin address (*.LLST12)
	.quad	.LVL35	# Location list end address (*.LLST12)
	.value	0x1	# Location expression size
	.byte	0x55	# DW_OP_reg5
	.quad	.LVL35	# Location list begin address (*.LLST12)
	.quad	.LVL36-1	# Location list end address (*.LLST12)
	.value	0x3	# Location expression size
	.byte	0x75	# DW_OP_breg5
	.sleb128 -2
	.byte	0x9f	# DW_OP_stack_value
	.quad	.LVL36-1	# Location list begin address (*.LLST12)
	.quad	.LFE10	# Location list end address (*.LLST12)
	.value	0x4	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x55	# DW_OP_reg5
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST12)
	.quad	0	# Location list terminator end (*.LLST12)
.LLST13:
	.quad	.LVL37	# Location list begin address (*.LLST13)
	.quad	.LVL38	# Location list end address (*.LLST13)
	.value	0x1	# Location expression size
	.byte	0x55	# DW_OP_reg5
	.quad	.LVL38	# Location list begin address (*.LLST13)
	.quad	.LVL39-1	# Location list end address (*.LLST13)
	.value	0x3	# Location expression size
	.byte	0x75	# DW_OP_breg5
	.sleb128 -1
	.byte	0x9f	# DW_OP_stack_value
	.quad	.LVL39-1	# Location list begin address (*.LLST13)
	.quad	.LFE11	# Location list end address (*.LLST13)
	.value	0x4	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x55	# DW_OP_reg5
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST13)
	.quad	0	# Location list terminator end (*.LLST13)
.LLST14:
	.quad	.LFB13	# Location list begin address (*.LLST14)
	.quad	.LCFI0	# Location list end address (*.LLST14)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.quad	.LCFI0	# Location list begin address (*.LLST14)
	.quad	.LCFI1	# Location list end address (*.LLST14)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 16
	.quad	.LCFI1	# Location list begin address (*.LLST14)
	.quad	.LCFI2	# Location list end address (*.LLST14)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.quad	.LCFI2	# Location list begin address (*.LLST14)
	.quad	.LCFI3	# Location list end address (*.LLST14)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 16
	.quad	.LCFI3	# Location list begin address (*.LLST14)
	.quad	.LFE13	# Location list end address (*.LLST14)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.quad	0	# Location list terminator begin (*.LLST14)
	.quad	0	# Location list terminator end (*.LLST14)
.LLST15:
	.quad	.LVL40	# Location list begin address (*.LLST15)
	.quad	.LVL41	# Location list end address (*.LLST15)
	.value	0x1	# Location expression size
	.byte	0x55	# DW_OP_reg5
	.quad	.LVL41	# Location list begin address (*.LLST15)
	.quad	.LVL43	# Location list end address (*.LLST15)
	.value	0x1	# Location expression size
	.byte	0x53	# DW_OP_reg3
	.quad	.LVL43	# Location list begin address (*.LLST15)
	.quad	.LVL44-1	# Location list end address (*.LLST15)
	.value	0x3	# Location expression size
	.byte	0x75	# DW_OP_breg5
	.sleb128 -2
	.byte	0x9f	# DW_OP_stack_value
	.quad	.LVL44-1	# Location list begin address (*.LLST15)
	.quad	.LVL44	# Location list end address (*.LLST15)
	.value	0x4	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x55	# DW_OP_reg5
	.byte	0x9f	# DW_OP_stack_value
	.quad	.LVL44	# Location list begin address (*.LLST15)
	.quad	.LVL45	# Location list end address (*.LLST15)
	.value	0x1	# Location expression size
	.byte	0x53	# DW_OP_reg3
	.quad	.LVL45	# Location list begin address (*.LLST15)
	.quad	.LVL46	# Location list end address (*.LLST15)
	.value	0x1	# Location expression size
	.byte	0x55	# DW_OP_reg5
	.quad	.LVL46	# Location list begin address (*.LLST15)
	.quad	.LFE13	# Location list end address (*.LLST15)
	.value	0x4	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x55	# DW_OP_reg5
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST15)
	.quad	0	# Location list terminator end (*.LLST15)
.LLST16:
	.quad	.LVL48	# Location list begin address (*.LLST16)
	.quad	.LVL49-1	# Location list end address (*.LLST16)
	.value	0x1	# Location expression size
	.byte	0x55	# DW_OP_reg5
	.quad	.LVL49-1	# Location list begin address (*.LLST16)
	.quad	.LFE12	# Location list end address (*.LLST16)
	.value	0x4	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x55	# DW_OP_reg5
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST16)
	.quad	0	# Location list terminator end (*.LLST16)
.LLST17:
	.quad	.LVL50	# Location list begin address (*.LLST17)
	.quad	.LVL51	# Location list end address (*.LLST17)
	.value	0x1	# Location expression size
	.byte	0x55	# DW_OP_reg5
	.quad	.LVL51	# Location list begin address (*.LLST17)
	.quad	.LFE14	# Location list end address (*.LLST17)
	.value	0x4	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x55	# DW_OP_reg5
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST17)
	.quad	0	# Location list terminator end (*.LLST17)
.LLST18:
	.quad	.LVL50	# Location list begin address (*.LLST18)
	.quad	.LVL53-1	# Location list end address (*.LLST18)
	.value	0x1	# Location expression size
	.byte	0x54	# DW_OP_reg4
	.quad	.LVL53-1	# Location list begin address (*.LLST18)
	.quad	.LFE14	# Location list end address (*.LLST18)
	.value	0x4	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x54	# DW_OP_reg4
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST18)
	.quad	0	# Location list terminator end (*.LLST18)
.LLST19:
	.quad	.LVL50	# Location list begin address (*.LLST19)
	.quad	.LVL53-1	# Location list end address (*.LLST19)
	.value	0x1	# Location expression size
	.byte	0x51	# DW_OP_reg1
	.quad	.LVL53-1	# Location list begin address (*.LLST19)
	.quad	.LFE14	# Location list end address (*.LLST19)
	.value	0x4	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x51	# DW_OP_reg1
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST19)
	.quad	0	# Location list terminator end (*.LLST19)
.LLST20:
	.quad	.LVL50	# Location list begin address (*.LLST20)
	.quad	.LVL53-1	# Location list end address (*.LLST20)
	.value	0x1	# Location expression size
	.byte	0x52	# DW_OP_reg2
	.quad	.LVL53-1	# Location list begin address (*.LLST20)
	.quad	.LFE14	# Location list end address (*.LLST20)
	.value	0x4	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x52	# DW_OP_reg2
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST20)
	.quad	0	# Location list terminator end (*.LLST20)
.LLST21:
	.quad	.LVL50	# Location list begin address (*.LLST21)
	.quad	.LVL53-1	# Location list end address (*.LLST21)
	.value	0x1	# Location expression size
	.byte	0x58	# DW_OP_reg8
	.quad	.LVL53-1	# Location list begin address (*.LLST21)
	.quad	.LFE14	# Location list end address (*.LLST21)
	.value	0x4	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x58	# DW_OP_reg8
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST21)
	.quad	0	# Location list terminator end (*.LLST21)
.LLST22:
	.quad	.LVL50	# Location list begin address (*.LLST22)
	.quad	.LVL53-1	# Location list end address (*.LLST22)
	.value	0x1	# Location expression size
	.byte	0x59	# DW_OP_reg9
	.quad	.LVL53-1	# Location list begin address (*.LLST22)
	.quad	.LFE14	# Location list end address (*.LLST22)
	.value	0x4	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x59	# DW_OP_reg9
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST22)
	.quad	0	# Location list terminator end (*.LLST22)
.LLST23:
	.quad	.LVL50	# Location list begin address (*.LLST23)
	.quad	.LVL50	# Location list end address (*.LLST23)
	.value	0x2	# Location expression size
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.quad	.LVL50	# Location list begin address (*.LLST23)
	.quad	.LFE14	# Location list end address (*.LLST23)
	.value	0x2	# Location expression size
	.byte	0x33	# DW_OP_lit3
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST23)
	.quad	0	# Location list terminator end (*.LLST23)
.LLST24:
	.quad	.LVL50	# Location list begin address (*.LLST24)
	.quad	.LVL50	# Location list end address (*.LLST24)
	.value	0x2	# Location expression size
	.byte	0x91	# DW_OP_fbreg
	.sleb128 8
	.quad	.LVL50	# Location list begin address (*.LLST24)
	.quad	.LFE14	# Location list end address (*.LLST24)
	.value	0x2	# Location expression size
	.byte	0x34	# DW_OP_lit4
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST24)
	.quad	0	# Location list terminator end (*.LLST24)
.LLST25:
	.quad	.LVL50	# Location list begin address (*.LLST25)
	.quad	.LVL52	# Location list end address (*.LLST25)
	.value	0x1	# Location expression size
	.byte	0x61	# DW_OP_reg17
	.quad	.LVL52	# Location list begin address (*.LLST25)
	.quad	.LFE14	# Location list end address (*.LLST25)
	.value	0x6	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x3
	.byte	0xf5	# DW_OP_GNU_regval_type
	.uleb128 0x11
	.uleb128 0x31
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST25)
	.quad	0	# Location list terminator end (*.LLST25)
.LLST26:
	.quad	.LVL50	# Location list begin address (*.LLST26)
	.quad	.LVL53-1	# Location list end address (*.LLST26)
	.value	0x1	# Location expression size
	.byte	0x62	# DW_OP_reg18
	.quad	.LVL53-1	# Location list begin address (*.LLST26)
	.quad	.LFE14	# Location list end address (*.LLST26)
	.value	0x6	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x3
	.byte	0xf5	# DW_OP_GNU_regval_type
	.uleb128 0x12
	.uleb128 0x31
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST26)
	.quad	0	# Location list terminator end (*.LLST26)
.LLST27:
	.quad	.LVL50	# Location list begin address (*.LLST27)
	.quad	.LVL53-1	# Location list end address (*.LLST27)
	.value	0x1	# Location expression size
	.byte	0x63	# DW_OP_reg19
	.quad	.LVL53-1	# Location list begin address (*.LLST27)
	.quad	.LFE14	# Location list end address (*.LLST27)
	.value	0x6	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x3
	.byte	0xf5	# DW_OP_GNU_regval_type
	.uleb128 0x13
	.uleb128 0x31
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST27)
	.quad	0	# Location list terminator end (*.LLST27)
.LLST28:
	.quad	.LVL50	# Location list begin address (*.LLST28)
	.quad	.LVL53-1	# Location list end address (*.LLST28)
	.value	0x1	# Location expression size
	.byte	0x64	# DW_OP_reg20
	.quad	.LVL53-1	# Location list begin address (*.LLST28)
	.quad	.LFE14	# Location list end address (*.LLST28)
	.value	0x6	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x3
	.byte	0xf5	# DW_OP_GNU_regval_type
	.uleb128 0x14
	.uleb128 0x31
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST28)
	.quad	0	# Location list terminator end (*.LLST28)
.LLST29:
	.quad	.LVL50	# Location list begin address (*.LLST29)
	.quad	.LVL53-1	# Location list end address (*.LLST29)
	.value	0x1	# Location expression size
	.byte	0x65	# DW_OP_reg21
	.quad	.LVL53-1	# Location list begin address (*.LLST29)
	.quad	.LFE14	# Location list end address (*.LLST29)
	.value	0x6	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x3
	.byte	0xf5	# DW_OP_GNU_regval_type
	.uleb128 0x15
	.uleb128 0x31
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST29)
	.quad	0	# Location list terminator end (*.LLST29)
.LLST30:
	.quad	.LVL50	# Location list begin address (*.LLST30)
	.quad	.LVL53-1	# Location list end address (*.LLST30)
	.value	0x1	# Location expression size
	.byte	0x66	# DW_OP_reg22
	.quad	.LVL53-1	# Location list begin address (*.LLST30)
	.quad	.LFE14	# Location list end address (*.LLST30)
	.value	0x6	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x3
	.byte	0xf5	# DW_OP_GNU_regval_type
	.uleb128 0x16
	.uleb128 0x31
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST30)
	.quad	0	# Location list terminator end (*.LLST30)
.LLST31:
	.quad	.LVL50	# Location list begin address (*.LLST31)
	.quad	.LVL53-1	# Location list end address (*.LLST31)
	.value	0x1	# Location expression size
	.byte	0x67	# DW_OP_reg23
	.quad	.LVL53-1	# Location list begin address (*.LLST31)
	.quad	.LFE14	# Location list end address (*.LLST31)
	.value	0x6	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x3
	.byte	0xf5	# DW_OP_GNU_regval_type
	.uleb128 0x17
	.uleb128 0x31
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST31)
	.quad	0	# Location list terminator end (*.LLST31)
.LLST32:
	.quad	.LVL50	# Location list begin address (*.LLST32)
	.quad	.LVL53-1	# Location list end address (*.LLST32)
	.value	0x1	# Location expression size
	.byte	0x68	# DW_OP_reg24
	.quad	.LVL53-1	# Location list begin address (*.LLST32)
	.quad	.LFE14	# Location list end address (*.LLST32)
	.value	0x6	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x3
	.byte	0xf5	# DW_OP_GNU_regval_type
	.uleb128 0x18
	.uleb128 0x31
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST32)
	.quad	0	# Location list terminator end (*.LLST32)
.LLST33:
	.quad	.LVL50	# Location list begin address (*.LLST33)
	.quad	.LVL50	# Location list end address (*.LLST33)
	.value	0x2	# Location expression size
	.byte	0x91	# DW_OP_fbreg
	.sleb128 16
	.quad	.LVL50	# Location list begin address (*.LLST33)
	.quad	.LFE14	# Location list end address (*.LLST33)
	.value	0xa	# Location expression size
	.byte	0x9e	# DW_OP_implicit_value
	.uleb128 0x8
	.long	0	# fp or vector constant word 0
	.long	0x400c0000	# fp or vector constant word 1
	.quad	0	# Location list terminator begin (*.LLST33)
	.quad	0	# Location list terminator end (*.LLST33)
.LLST34:
	.quad	.LVL50	# Location list begin address (*.LLST34)
	.quad	.LVL50	# Location list end address (*.LLST34)
	.value	0x2	# Location expression size
	.byte	0x91	# DW_OP_fbreg
	.sleb128 24
	.quad	.LVL50	# Location list begin address (*.LLST34)
	.quad	.LFE14	# Location list end address (*.LLST34)
	.value	0xa	# Location expression size
	.byte	0x9e	# DW_OP_implicit_value
	.uleb128 0x8
	.long	0	# fp or vector constant word 0
	.long	0x40120000	# fp or vector constant word 1
	.quad	0	# Location list terminator begin (*.LLST34)
	.quad	0	# Location list terminator end (*.LLST34)
.LLST35:
	.quad	.LVL55	# Location list begin address (*.LLST35)
	.quad	.LVL56	# Location list end address (*.LLST35)
	.value	0x1	# Location expression size
	.byte	0x55	# DW_OP_reg5
	.quad	.LVL56	# Location list begin address (*.LLST35)
	.quad	.LFE15	# Location list end address (*.LLST35)
	.value	0x4	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x55	# DW_OP_reg5
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST35)
	.quad	0	# Location list terminator end (*.LLST35)
.LLST36:
	.quad	.LVL55	# Location list begin address (*.LLST36)
	.quad	.LVL57-1	# Location list end address (*.LLST36)
	.value	0x1	# Location expression size
	.byte	0x54	# DW_OP_reg4
	.quad	.LVL57-1	# Location list begin address (*.LLST36)
	.quad	.LFE15	# Location list end address (*.LLST36)
	.value	0x4	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x54	# DW_OP_reg4
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST36)
	.quad	0	# Location list terminator end (*.LLST36)
.LLST37:
	.quad	.LVL55	# Location list begin address (*.LLST37)
	.quad	.LVL57-1	# Location list end address (*.LLST37)
	.value	0x1	# Location expression size
	.byte	0x51	# DW_OP_reg1
	.quad	.LVL57-1	# Location list begin address (*.LLST37)
	.quad	.LFE15	# Location list end address (*.LLST37)
	.value	0x4	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x51	# DW_OP_reg1
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST37)
	.quad	0	# Location list terminator end (*.LLST37)
.LLST38:
	.quad	.LVL55	# Location list begin address (*.LLST38)
	.quad	.LVL57-1	# Location list end address (*.LLST38)
	.value	0x1	# Location expression size
	.byte	0x52	# DW_OP_reg2
	.quad	.LVL57-1	# Location list begin address (*.LLST38)
	.quad	.LFE15	# Location list end address (*.LLST38)
	.value	0x4	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x52	# DW_OP_reg2
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST38)
	.quad	0	# Location list terminator end (*.LLST38)
.LLST39:
	.quad	.LVL55	# Location list begin address (*.LLST39)
	.quad	.LVL57-1	# Location list end address (*.LLST39)
	.value	0x1	# Location expression size
	.byte	0x58	# DW_OP_reg8
	.quad	.LVL57-1	# Location list begin address (*.LLST39)
	.quad	.LFE15	# Location list end address (*.LLST39)
	.value	0x4	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x58	# DW_OP_reg8
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST39)
	.quad	0	# Location list terminator end (*.LLST39)
.LLST40:
	.quad	.LVL55	# Location list begin address (*.LLST40)
	.quad	.LVL57-1	# Location list end address (*.LLST40)
	.value	0x1	# Location expression size
	.byte	0x59	# DW_OP_reg9
	.quad	.LVL57-1	# Location list begin address (*.LLST40)
	.quad	.LFE15	# Location list end address (*.LLST40)
	.value	0x4	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x59	# DW_OP_reg9
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST40)
	.quad	0	# Location list terminator end (*.LLST40)
.LLST41:
	.quad	.LVL55	# Location list begin address (*.LLST41)
	.quad	.LVL57-1	# Location list end address (*.LLST41)
	.value	0x3	# Location expression size
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.byte	0x6	# DW_OP_deref
	.quad	0	# Location list terminator begin (*.LLST41)
	.quad	0	# Location list terminator end (*.LLST41)
.LLST42:
	.quad	.LVL55	# Location list begin address (*.LLST42)
	.quad	.LVL57-1	# Location list end address (*.LLST42)
	.value	0x3	# Location expression size
	.byte	0x91	# DW_OP_fbreg
	.sleb128 8
	.byte	0x6	# DW_OP_deref
	.quad	0	# Location list terminator begin (*.LLST42)
	.quad	0	# Location list terminator end (*.LLST42)
.LLST43:
	.quad	.LFB20	# Location list begin address (*.LLST43)
	.quad	.LCFI4	# Location list end address (*.LLST43)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.quad	.LCFI4	# Location list begin address (*.LLST43)
	.quad	.LCFI5	# Location list end address (*.LLST43)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 16
	.quad	.LCFI5	# Location list begin address (*.LLST43)
	.quad	.LFE20	# Location list end address (*.LLST43)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.quad	0	# Location list terminator begin (*.LLST43)
	.quad	0	# Location list terminator end (*.LLST43)
.LLST44:
	.quad	.LVL60	# Location list begin address (*.LLST44)
	.quad	.LVL61	# Location list end address (*.LLST44)
	.value	0x1	# Location expression size
	.byte	0x55	# DW_OP_reg5
	.quad	.LVL61	# Location list begin address (*.LLST44)
	.quad	.LVL63	# Location list end address (*.LLST44)
	.value	0x1	# Location expression size
	.byte	0x53	# DW_OP_reg3
	.quad	.LVL63	# Location list begin address (*.LLST44)
	.quad	.LFE20	# Location list end address (*.LLST44)
	.value	0x1	# Location expression size
	.byte	0x50	# DW_OP_reg0
	.quad	0	# Location list terminator begin (*.LLST44)
	.quad	0	# Location list terminator end (*.LLST44)
.LLST45:
	.quad	.LFB21	# Location list begin address (*.LLST45)
	.quad	.LCFI6	# Location list end address (*.LLST45)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.quad	.LCFI6	# Location list begin address (*.LLST45)
	.quad	.LCFI7	# Location list end address (*.LLST45)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 16
	.quad	.LCFI7	# Location list begin address (*.LLST45)
	.quad	.LFE21	# Location list end address (*.LLST45)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.quad	0	# Location list terminator begin (*.LLST45)
	.quad	0	# Location list terminator end (*.LLST45)
.LLST46:
	.quad	.LVL64	# Location list begin address (*.LLST46)
	.quad	.LVL64	# Location list end address (*.LLST46)
	.value	0x1	# Location expression size
	.byte	0x55	# DW_OP_reg5
	.quad	0	# Location list terminator begin (*.LLST46)
	.quad	0	# Location list terminator end (*.LLST46)
.LLST47:
	.quad	.LVL64	# Location list begin address (*.LLST47)
	.quad	.LVL65-1	# Location list end address (*.LLST47)
	.value	0x1	# Location expression size
	.byte	0x54	# DW_OP_reg4
	.quad	.LVL65-1	# Location list begin address (*.LLST47)
	.quad	.LVL66	# Location list end address (*.LLST47)
	.value	0x1	# Location expression size
	.byte	0x53	# DW_OP_reg3
	.quad	.LVL66	# Location list begin address (*.LLST47)
	.quad	.LFE21	# Location list end address (*.LLST47)
	.value	0x1	# Location expression size
	.byte	0x50	# DW_OP_reg0
	.quad	0	# Location list terminator begin (*.LLST47)
	.quad	0	# Location list terminator end (*.LLST47)
.LLST48:
	.quad	.LVL67	# Location list begin address (*.LLST48)
	.quad	.LVL68	# Location list end address (*.LLST48)
	.value	0x1	# Location expression size
	.byte	0x55	# DW_OP_reg5
	.quad	.LVL68	# Location list begin address (*.LLST48)
	.quad	.LFE22	# Location list end address (*.LLST48)
	.value	0x4	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x55	# DW_OP_reg5
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST48)
	.quad	0	# Location list terminator end (*.LLST48)
.LLST49:
	.quad	.LFB23	# Location list begin address (*.LLST49)
	.quad	.LCFI8	# Location list end address (*.LLST49)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.quad	.LCFI8	# Location list begin address (*.LLST49)
	.quad	.LCFI9	# Location list end address (*.LLST49)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 16
	.quad	.LCFI9	# Location list begin address (*.LLST49)
	.quad	.LCFI10	# Location list end address (*.LLST49)
	.value	0x3	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 64
	.quad	.LCFI10	# Location list begin address (*.LLST49)
	.quad	.LCFI11	# Location list end address (*.LLST49)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 16
	.quad	.LCFI11	# Location list begin address (*.LLST49)
	.quad	.LCFI12	# Location list end address (*.LLST49)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.quad	.LCFI12	# Location list begin address (*.LLST49)
	.quad	.LFE23	# Location list end address (*.LLST49)
	.value	0x3	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 64
	.quad	0	# Location list terminator begin (*.LLST49)
	.quad	0	# Location list terminator end (*.LLST49)
.LLST50:
	.quad	.LVL80	# Location list begin address (*.LLST50)
	.quad	.LVL81	# Location list end address (*.LLST50)
	.value	0x1	# Location expression size
	.byte	0x50	# DW_OP_reg0
	.quad	.LVL81	# Location list begin address (*.LLST50)
	.quad	.LVL89	# Location list end address (*.LLST50)
	.value	0x1	# Location expression size
	.byte	0x53	# DW_OP_reg3
	.quad	.LVL90	# Location list begin address (*.LLST50)
	.quad	.LFE23	# Location list end address (*.LLST50)
	.value	0x1	# Location expression size
	.byte	0x53	# DW_OP_reg3
	.quad	0	# Location list terminator begin (*.LLST50)
	.quad	0	# Location list terminator end (*.LLST50)
	.section	.debug_aranges,"",@progbits
	.long	0x3c	# Length of Address Ranges Info
	.value	0x2	# DWARF Version
	.long	.Ldebug_info0	# Offset of Compilation Unit Info
	.byte	0x8	# Size of Address
	.byte	0	# Size of Segment Descriptor
	.value	0	# Pad to 16 byte boundary
	.value	0
	.quad	.Ltext0	# Address
	.quad	.Letext0-.Ltext0	# Length
	.quad	.LFB23	# Address
	.quad	.LFE23-.LFB23	# Length
	.quad	0
	.quad	0
	.section	.debug_ranges,"",@progbits
.Ldebug_ranges0:
	.quad	.Ltext0	# Offset 0
	.quad	.Letext0
	.quad	.LFB23	# Offset 0x10
	.quad	.LFE23
	.quad	0
	.quad	0
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.section	.debug_str,"MS",@progbits,1
.LASF4:
	.string	"locexpr"
.LASF13:
	.string	"reference"
.LASF18:
	.string	"regcopy"
.LASF31:
	.string	"regvar"
.LASF30:
	.string	"invalid"
.LASF29:
	.string	"born"
.LASF17:
	.string	"stackparam2"
.LASF24:
	.string	"data"
.LASF20:
	.string	"stackcopy1"
.LASF2:
	.string	"gdb.arch/amd64-entry-value.cc"
.LASF23:
	.string	"datap"
.LASF28:
	.string	"lost"
.LASF0:
	.string	"double"
.LASF32:
	.string	"nodatavarp"
.LASF35:
	.string	"main"
.LASF10:
	.string	"self"
.LASF9:
	.string	"amb_a"
.LASF8:
	.string	"amb_b"
.LASF26:
	.string	"different"
.LASF1:
	.string	"GNU C++ 4.7.0 20110912 (experimental)"
.LASF33:
	.string	"stackvar1"
.LASF34:
	.string	"stackvar2"
.LASF12:
	.string	"stacktest"
.LASF25:
	.string	"data2"
.LASF19:
	.string	"nodatacopy"
.LASF7:
	.string	"amb_x"
.LASF6:
	.string	"amb_y"
.LASF5:
	.string	"amb_z"
.LASF15:
	.string	"nodataparam"
.LASF3:
	.string	""
.LASF21:
	.string	"stackcopy2"
.LASF16:
	.string	"stackparam1"
.LASF14:
	.string	"regparam"
.LASF11:
	.string	"self2"
.LASF22:
	.string	"datap_input"
.LASF27:
	.string	"validity"
	.ident	"GCC: (GNU) 4.7.0 20110912 (experimental)"
	.section	.note.GNU-stack,"",@progbits
