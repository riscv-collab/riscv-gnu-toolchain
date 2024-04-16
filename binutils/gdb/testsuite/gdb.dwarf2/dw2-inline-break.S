/* This testcase is part of GDB, the GNU debugger.

   Copyright 2012-2024 Free Software Foundation, Inc.

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

/* This source file was generated using the following command line:

   gcc -S -dA -g -O2 ../gdb.opt/inline-break.c -o inline-break.S

*/
	.file	"inline-break.c"
	.section	.debug_abbrev,"",@progbits
.Ldebug_abbrev0:
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.text
.Ltext0:
	.p2align 4,,15
.globl func2
	.type	func2, @function
func2:
.LFB1:
	.file 1 "../gdb.opt/inline-break.c"
	# ../gdb.opt/inline-break.c:39
	.loc 1 39 0
	.cfi_startproc
.LVL0:
	# basic block 2
	# ../gdb.opt/inline-break.c:40
	.loc 1 40 0
	movl	%edi, %eax
	sall	$4, %eax
	addl	%edi, %eax
	# ../gdb.opt/inline-break.c:41
	.loc 1 41 0
	ret
	.cfi_endproc
.LFE1:
	.size	func2, .-func2
	.p2align 4,,15
.globl func4a
	.type	func4a, @function
func4a:
.LFB5:
	# ../gdb.opt/inline-break.c:69
	.loc 1 69 0
	.cfi_startproc
.LVL1:
	# basic block 2
	# ../gdb.opt/inline-break.c:70
	.loc 1 70 0
	movl	%edi, %eax
	sall	$4, %eax
	leal	(%rax,%rdi), %edi
.LVL2:
.LBB46:
.LBB47:
	# ../gdb.opt/inline-break.c:64
	.loc 1 64 0
	xorl	%eax, %eax
	cmpl	$12, %edi
	setg	%al
	addl	$1, %eax
.LBE47:
.LBE46:
	# ../gdb.opt/inline-break.c:71
	.loc 1 71 0
	ret
	.cfi_endproc
.LFE5:
	.size	func4a, .-func4a
	.p2align 4,,15
.globl func5b
	.type	func5b, @function
func5b:
.LFB6:
	# ../gdb.opt/inline-break.c:78
	.loc 1 78 0
	.cfi_startproc
.LVL3:
	# basic block 2
	# ../gdb.opt/inline-break.c:79
	.loc 1 79 0
	xorl	%eax, %eax
	cmpl	$11, %edi
	setg	%al
	addl	$1, %eax
	# ../gdb.opt/inline-break.c:80
	.loc 1 80 0
	ret
	.cfi_endproc
.LFE6:
	.size	func5b, .-func5b
	.p2align 4,,15
.globl func6b
	.type	func6b, @function
func6b:
.LFB8:
	# ../gdb.opt/inline-break.c:93
	.loc 1 93 0
	.cfi_startproc
.LVL4:
	# basic block 2
	# ../gdb.opt/inline-break.c:94
	.loc 1 94 0
	xorl	%eax, %eax
	cmpl	$13, %edi
	setle	%al
	addl	$2, %eax
	# ../gdb.opt/inline-break.c:95
	.loc 1 95 0
	ret
	.cfi_endproc
.LFE8:
	.size	func6b, .-func6b
	.p2align 4,,15
.globl func6a
	.type	func6a, @function
func6a:
.LFB9:
	# ../gdb.opt/inline-break.c:99
	.loc 1 99 0
	.cfi_startproc
.LVL5:
	# basic block 2
	# ../gdb.opt/inline-break.c:100
	.loc 1 100 0
	movl	%edi, %eax
	sall	$4, %eax
	leal	(%rax,%rdi), %edi
.LVL6:
.LBB48:
.LBB49:
	# ../gdb.opt/inline-break.c:94
	.loc 1 94 0
	xorl	%eax, %eax
	cmpl	$13, %edi
	setle	%al
	addl	$2, %eax
.LBE49:
.LBE48:
	# ../gdb.opt/inline-break.c:101
	.loc 1 101 0
	ret
	.cfi_endproc
.LFE9:
	.size	func6a, .-func6a
	.p2align 4,,15
.globl func8b
	.type	func8b, @function
func8b:
.LFB12:
	# ../gdb.opt/inline-break.c:121
	.loc 1 121 0
	.cfi_startproc
.LVL7:
	# basic block 2
	# ../gdb.opt/inline-break.c:122
	.loc 1 122 0
	xorl	%eax, %eax
	cmpl	$6, %edi
	setle	%al
	leal	9(%rax,%rax), %eax
	# ../gdb.opt/inline-break.c:123
	.loc 1 123 0
	ret
	.cfi_endproc
.LFE12:
	.size	func8b, .-func8b
	.p2align 4,,15
.globl main
	.type	main, @function
main:
.LFB14:
	# ../gdb.opt/inline-break.c:135
	.loc 1 135 0
	.cfi_startproc
.LVL8:
	# basic block 2
	# ../gdb.opt/inline-break.c:140
	.loc 1 140 0
	movl	%edi, -4(%rsp)
.LVL9:
	# ../gdb.opt/inline-break.c:142
	.loc 1 142 0
	movl	-4(%rsp), %eax
.LVL10:
.LBB50:
.LBB51:
	# ../gdb.opt/inline-break.c:32
	.loc 1 32 0
	movl	$23, %edi
.LVL11:
.LBE51:
.LBE50:
.LBB53:
.LBB54:
	# ../gdb.opt/inline-break.c:55
	.loc 1 55 0
	movl	$23, %esi
.LVL12:
.LBE54:
.LBE53:
.LBB58:
.LBB59:
	# ../gdb.opt/inline-break.c:85
	.loc 1 85 0
	movl	$23, %ecx
.LBE59:
.LBE58:
.LBB63:
.LBB52:
	# ../gdb.opt/inline-break.c:32
	.loc 1 32 0
	imull	%edi, %eax
.LVL13:
.LBE52:
.LBE63:
	# ../gdb.opt/inline-break.c:142
	.loc 1 142 0
	movl	%eax, -4(%rsp)
	# ../gdb.opt/inline-break.c:144
	.loc 1 144 0
	movl	-4(%rsp), %eax
.LVL14:
.LBB64:
.LBB65:
	# ../gdb.opt/inline-break.c:40
	.loc 1 40 0
	movl	%eax, %edx
	sall	$4, %edx
	leal	(%rdx,%rax), %eax
.LVL15:
.LBE65:
.LBE64:
	# ../gdb.opt/inline-break.c:144
	.loc 1 144 0
	movl	%eax, -4(%rsp)
	# ../gdb.opt/inline-break.c:146
	.loc 1 146 0
	movl	-4(%rsp), %eax
.LVL16:
.LBB66:
.LBB57:
	# ../gdb.opt/inline-break.c:55
	.loc 1 55 0
	imull	%esi, %eax
.LVL17:
.LBB55:
.LBB56:
	# ../gdb.opt/inline-break.c:49
	.loc 1 49 0
	cmpl	$13, %eax
	setg	%al
.LVL18:
	movzbl	%al, %eax
	addl	$1, %eax
.LBE56:
.LBE55:
.LBE57:
.LBE66:
	# ../gdb.opt/inline-break.c:146
	.loc 1 146 0
	movl	%eax, -4(%rsp)
	# ../gdb.opt/inline-break.c:148
	.loc 1 148 0
	movl	-4(%rsp), %eax
.LVL19:
.LBB67:
.LBB68:
	# ../gdb.opt/inline-break.c:70
	.loc 1 70 0
	movl	%eax, %edx
	sall	$4, %edx
	leal	(%rdx,%rax), %eax
.LVL20:
.LBB69:
.LBB70:
	# ../gdb.opt/inline-break.c:64
	.loc 1 64 0
	cmpl	$12, %eax
	setg	%al
	movzbl	%al, %eax
	addl	$1, %eax
.LBE70:
.LBE69:
.LBE68:
.LBE67:
	# ../gdb.opt/inline-break.c:148
	.loc 1 148 0
	movl	%eax, -4(%rsp)
	# ../gdb.opt/inline-break.c:150
	.loc 1 150 0
	movl	-4(%rsp), %eax
.LVL21:
.LBB71:
.LBB62:
	# ../gdb.opt/inline-break.c:85
	.loc 1 85 0
	imull	%ecx, %eax
.LVL22:
.LBB60:
.LBB61:
	# ../gdb.opt/inline-break.c:79
	.loc 1 79 0
	cmpl	$11, %eax
	setg	%al
.LVL23:
	movzbl	%al, %eax
	addl	$1, %eax
.LBE61:
.LBE60:
.LBE62:
.LBE71:
	# ../gdb.opt/inline-break.c:150
	.loc 1 150 0
	movl	%eax, -4(%rsp)
	# ../gdb.opt/inline-break.c:152
	.loc 1 152 0
	movl	-4(%rsp), %eax
.LVL24:
.LBB72:
.LBB73:
	# ../gdb.opt/inline-break.c:100
	.loc 1 100 0
	movl	%eax, %edx
	sall	$4, %edx
	leal	(%rdx,%rax), %eax
.LVL25:
.LBE73:
.LBE72:
.LBB77:
.LBB80:
	# ../gdb.opt/inline-break.c:114
	.loc 1 114 0
	movl	$29, %edx
.LBE80:
.LBE77:
.LBB89:
.LBB76:
.LBB74:
.LBB75:
	# ../gdb.opt/inline-break.c:94
	.loc 1 94 0
	cmpl	$13, %eax
	setle	%al
	movzbl	%al, %eax
	addl	$2, %eax
.LBE75:
.LBE74:
.LBE76:
.LBE89:
	# ../gdb.opt/inline-break.c:152
	.loc 1 152 0
	movl	%eax, -4(%rsp)
	# ../gdb.opt/inline-break.c:154
	.loc 1 154 0
	movl	-4(%rsp), %eax
.LVL26:
.LBB90:
.LBB79:
	# ../gdb.opt/inline-break.c:114
	.loc 1 114 0
	imull	%edx, %eax
.LVL27:
.LBE79:
.LBE90:
	# ../gdb.opt/inline-break.c:154
	.loc 1 154 0
	movl	-4(%rsp), %edx
.LVL28:
.LBB91:
.LBB87:
.LBB82:
.LBB84:
	# ../gdb.opt/inline-break.c:108
	.loc 1 108 0
	cmpl	$22, %eax
	setg	%al
.LVL29:
.LBE84:
.LBE82:
.LBE87:
.LBE91:
.LBB92:
.LBB94:
	cmpl	$22, %edx
	setg	%dl
.LVL30:
.LBE94:
.LBE92:
.LBB96:
.LBB78:
.LBB81:
.LBB83:
	movzbl	%al, %eax
.LBE83:
.LBE81:
.LBE78:
.LBE96:
.LBB97:
.LBB93:
	movzbl	%dl, %edx
.LBE93:
.LBE97:
.LBB98:
.LBB88:
.LBB86:
.LBB85:
	leal	1(%rax,%rax,2), %eax
.LBE85:
.LBE86:
.LBE88:
.LBE98:
.LBB99:
.LBB95:
	leal	1(%rdx,%rdx,2), %edx
.LBE95:
.LBE99:
	# ../gdb.opt/inline-break.c:154
	.loc 1 154 0
	leal	(%rdx,%rax), %eax
.LVL31:
	movl	%eax, -4(%rsp)
.LVL32:
	# ../gdb.opt/inline-break.c:156
	.loc 1 156 0
	movl	-4(%rsp), %edx
.LVL33:
.LBB100:
.LBB101:
	# ../gdb.opt/inline-break.c:128
	.loc 1 128 0
	movl	%edx, %eax
.LVL34:
	sall	$5, %eax
	subl	%edx, %eax
.LBB102:
.LBB103:
	# ../gdb.opt/inline-break.c:122
	.loc 1 122 0
	xorl	%edx, %edx
.LVL35:
	cmpl	$6, %eax
.LBE103:
.LBE102:
.LBE101:
.LBE100:
	# ../gdb.opt/inline-break.c:156
	.loc 1 156 0
	movl	-4(%rsp), %eax
.LVL36:
.LBB107:
.LBB106:
.LBB105:
.LBB104:
	# ../gdb.opt/inline-break.c:122
	.loc 1 122 0
	setle	%dl
.LBE104:
.LBE105:
.LBE106:
.LBE107:
.LBB108:
.LBB109:
	cmpl	$6, %eax
	setle	%al
.LVL37:
	movzbl	%al, %eax
	leal	9(%rax,%rax), %eax
.LBE109:
.LBE108:
	# ../gdb.opt/inline-break.c:156
	.loc 1 156 0
	leal	9(%rax,%rdx,2), %eax
.LVL38:
	movl	%eax, -4(%rsp)
.LVL39:
	# ../gdb.opt/inline-break.c:158
	.loc 1 158 0
	movl	-4(%rsp), %eax
.LVL40:
	# ../gdb.opt/inline-break.c:159
	.loc 1 159 0
	ret
	.cfi_endproc
.LFE14:
	.size	main, .-main
.Letext0:
	.section	.debug_loc,"",@progbits
.Ldebug_loc0:
.LLST0:
	.quad	.LVL1-.Ltext0	# Location list begin address (*.LLST0)
	.quad	.LVL2-.Ltext0	# Location list end address (*.LLST0)
	.value	0x1	# Location expression size
	.byte	0x55	# DW_OP_reg5
	.quad	0x0	# Location list terminator begin (*.LLST0)
	.quad	0x0	# Location list terminator end (*.LLST0)
.LLST1:
	.quad	.LVL1-.Ltext0	# Location list begin address (*.LLST1)
	.quad	.LVL2-.Ltext0	# Location list end address (*.LLST1)
	.value	0x5	# Location expression size
	.byte	0x75	# DW_OP_breg5
	.sleb128 0
	.byte	0x41	# DW_OP_lit17
	.byte	0x1e	# DW_OP_mul
	.byte	0x9f	# DW_OP_stack_value
	.quad	0x0	# Location list terminator begin (*.LLST1)
	.quad	0x0	# Location list terminator end (*.LLST1)
.LLST2:
	.quad	.LVL5-.Ltext0	# Location list begin address (*.LLST2)
	.quad	.LVL6-.Ltext0	# Location list end address (*.LLST2)
	.value	0x1	# Location expression size
	.byte	0x55	# DW_OP_reg5
	.quad	0x0	# Location list terminator begin (*.LLST2)
	.quad	0x0	# Location list terminator end (*.LLST2)
.LLST3:
	.quad	.LVL5-.Ltext0	# Location list begin address (*.LLST3)
	.quad	.LVL6-.Ltext0	# Location list end address (*.LLST3)
	.value	0x5	# Location expression size
	.byte	0x75	# DW_OP_breg5
	.sleb128 0
	.byte	0x41	# DW_OP_lit17
	.byte	0x1e	# DW_OP_mul
	.byte	0x9f	# DW_OP_stack_value
	.quad	0x0	# Location list terminator begin (*.LLST3)
	.quad	0x0	# Location list terminator end (*.LLST3)
.LLST4:
	.quad	.LVL8-.Ltext0	# Location list begin address (*.LLST4)
	.quad	.LVL11-.Ltext0	# Location list end address (*.LLST4)
	.value	0x1	# Location expression size
	.byte	0x55	# DW_OP_reg5
	.quad	.LVL11-.Ltext0	# Location list begin address (*.LLST4)
	.quad	.LFE14-.Ltext0	# Location list end address (*.LLST4)
	.value	0x2	# Location expression size
	.byte	0x91	# DW_OP_fbreg
	.sleb128 -12
	.quad	0x0	# Location list terminator begin (*.LLST4)
	.quad	0x0	# Location list terminator end (*.LLST4)
.LLST5:
	.quad	.LVL8-.Ltext0	# Location list begin address (*.LLST5)
	.quad	.LVL12-.Ltext0	# Location list end address (*.LLST5)
	.value	0x1	# Location expression size
	.byte	0x54	# DW_OP_reg4
	.quad	0x0	# Location list terminator begin (*.LLST5)
	.quad	0x0	# Location list terminator end (*.LLST5)
.LLST6:
	.quad	.LVL9-.Ltext0	# Location list begin address (*.LLST6)
	.quad	.LVL31-.Ltext0	# Location list end address (*.LLST6)
	.value	0x2	# Location expression size
	.byte	0x91	# DW_OP_fbreg
	.sleb128 -12
	.quad	.LVL32-.Ltext0	# Location list begin address (*.LLST6)
	.quad	.LVL34-.Ltext0	# Location list end address (*.LLST6)
	.value	0x1	# Location expression size
	.byte	0x50	# DW_OP_reg0
	.quad	.LVL34-.Ltext0	# Location list begin address (*.LLST6)
	.quad	.LVL38-.Ltext0	# Location list end address (*.LLST6)
	.value	0x2	# Location expression size
	.byte	0x91	# DW_OP_fbreg
	.sleb128 -12
	.quad	.LVL39-.Ltext0	# Location list begin address (*.LLST6)
	.quad	.LVL40-.Ltext0	# Location list end address (*.LLST6)
	.value	0x1	# Location expression size
	.byte	0x50	# DW_OP_reg0
	.quad	.LVL40-.Ltext0	# Location list begin address (*.LLST6)
	.quad	.LFE14-.Ltext0	# Location list end address (*.LLST6)
	.value	0x2	# Location expression size
	.byte	0x91	# DW_OP_fbreg
	.sleb128 -12
	.quad	0x0	# Location list terminator begin (*.LLST6)
	.quad	0x0	# Location list terminator end (*.LLST6)
.LLST7:
	.quad	.LVL10-.Ltext0	# Location list begin address (*.LLST7)
	.quad	.LVL13-.Ltext0	# Location list end address (*.LLST7)
	.value	0x1	# Location expression size
	.byte	0x50	# DW_OP_reg0
	.quad	0x0	# Location list terminator begin (*.LLST7)
	.quad	0x0	# Location list terminator end (*.LLST7)
.LLST8:
	.quad	.LVL16-.Ltext0	# Location list begin address (*.LLST8)
	.quad	.LVL17-.Ltext0	# Location list end address (*.LLST8)
	.value	0x1	# Location expression size
	.byte	0x50	# DW_OP_reg0
	.quad	0x0	# Location list terminator begin (*.LLST8)
	.quad	0x0	# Location list terminator end (*.LLST8)
.LLST9:
	.quad	.LVL16-.Ltext0	# Location list begin address (*.LLST9)
	.quad	.LVL17-.Ltext0	# Location list end address (*.LLST9)
	.value	0x5	# Location expression size
	.byte	0x70	# DW_OP_breg0
	.sleb128 0
	.byte	0x47	# DW_OP_lit23
	.byte	0x1e	# DW_OP_mul
	.byte	0x9f	# DW_OP_stack_value
	.quad	.LVL17-.Ltext0	# Location list begin address (*.LLST9)
	.quad	.LVL18-.Ltext0	# Location list end address (*.LLST9)
	.value	0x1	# Location expression size
	.byte	0x50	# DW_OP_reg0
	.quad	0x0	# Location list terminator begin (*.LLST9)
	.quad	0x0	# Location list terminator end (*.LLST9)
.LLST10:
	.quad	.LVL21-.Ltext0	# Location list begin address (*.LLST10)
	.quad	.LVL22-.Ltext0	# Location list end address (*.LLST10)
	.value	0x1	# Location expression size
	.byte	0x50	# DW_OP_reg0
	.quad	0x0	# Location list terminator begin (*.LLST10)
	.quad	0x0	# Location list terminator end (*.LLST10)
.LLST11:
	.quad	.LVL21-.Ltext0	# Location list begin address (*.LLST11)
	.quad	.LVL22-.Ltext0	# Location list end address (*.LLST11)
	.value	0x5	# Location expression size
	.byte	0x70	# DW_OP_breg0
	.sleb128 0
	.byte	0x47	# DW_OP_lit23
	.byte	0x1e	# DW_OP_mul
	.byte	0x9f	# DW_OP_stack_value
	.quad	.LVL22-.Ltext0	# Location list begin address (*.LLST11)
	.quad	.LVL23-.Ltext0	# Location list end address (*.LLST11)
	.value	0x1	# Location expression size
	.byte	0x50	# DW_OP_reg0
	.quad	0x0	# Location list terminator begin (*.LLST11)
	.quad	0x0	# Location list terminator end (*.LLST11)
.LLST12:
	.quad	.LVL14-.Ltext0	# Location list begin address (*.LLST12)
	.quad	.LVL15-.Ltext0	# Location list end address (*.LLST12)
	.value	0x1	# Location expression size
	.byte	0x50	# DW_OP_reg0
	.quad	0x0	# Location list terminator begin (*.LLST12)
	.quad	0x0	# Location list terminator end (*.LLST12)
.LLST13:
	.quad	.LVL19-.Ltext0	# Location list begin address (*.LLST13)
	.quad	.LVL20-.Ltext0	# Location list end address (*.LLST13)
	.value	0x1	# Location expression size
	.byte	0x50	# DW_OP_reg0
	.quad	0x0	# Location list terminator begin (*.LLST13)
	.quad	0x0	# Location list terminator end (*.LLST13)
.LLST14:
	.quad	.LVL19-.Ltext0	# Location list begin address (*.LLST14)
	.quad	.LVL20-.Ltext0	# Location list end address (*.LLST14)
	.value	0x5	# Location expression size
	.byte	0x70	# DW_OP_breg0
	.sleb128 0
	.byte	0x41	# DW_OP_lit17
	.byte	0x1e	# DW_OP_mul
	.byte	0x9f	# DW_OP_stack_value
	.quad	0x0	# Location list terminator begin (*.LLST14)
	.quad	0x0	# Location list terminator end (*.LLST14)
.LLST15:
	.quad	.LVL24-.Ltext0	# Location list begin address (*.LLST15)
	.quad	.LVL25-.Ltext0	# Location list end address (*.LLST15)
	.value	0x1	# Location expression size
	.byte	0x50	# DW_OP_reg0
	.quad	0x0	# Location list terminator begin (*.LLST15)
	.quad	0x0	# Location list terminator end (*.LLST15)
.LLST16:
	.quad	.LVL24-.Ltext0	# Location list begin address (*.LLST16)
	.quad	.LVL25-.Ltext0	# Location list end address (*.LLST16)
	.value	0x5	# Location expression size
	.byte	0x70	# DW_OP_breg0
	.sleb128 0
	.byte	0x41	# DW_OP_lit17
	.byte	0x1e	# DW_OP_mul
	.byte	0x9f	# DW_OP_stack_value
	.quad	0x0	# Location list terminator begin (*.LLST16)
	.quad	0x0	# Location list terminator end (*.LLST16)
.LLST17:
	.quad	.LVL26-.Ltext0	# Location list begin address (*.LLST17)
	.quad	.LVL27-.Ltext0	# Location list end address (*.LLST17)
	.value	0x1	# Location expression size
	.byte	0x50	# DW_OP_reg0
	.quad	0x0	# Location list terminator begin (*.LLST17)
	.quad	0x0	# Location list terminator end (*.LLST17)
.LLST18:
	.quad	.LVL26-.Ltext0	# Location list begin address (*.LLST18)
	.quad	.LVL27-.Ltext0	# Location list end address (*.LLST18)
	.value	0x5	# Location expression size
	.byte	0x70	# DW_OP_breg0
	.sleb128 0
	.byte	0x4d	# DW_OP_lit29
	.byte	0x1e	# DW_OP_mul
	.byte	0x9f	# DW_OP_stack_value
	.quad	.LVL27-.Ltext0	# Location list begin address (*.LLST18)
	.quad	.LVL29-.Ltext0	# Location list end address (*.LLST18)
	.value	0x1	# Location expression size
	.byte	0x50	# DW_OP_reg0
	.quad	0x0	# Location list terminator begin (*.LLST18)
	.quad	0x0	# Location list terminator end (*.LLST18)
.LLST19:
	.quad	.LVL28-.Ltext0	# Location list begin address (*.LLST19)
	.quad	.LVL30-.Ltext0	# Location list end address (*.LLST19)
	.value	0x1	# Location expression size
	.byte	0x51	# DW_OP_reg1
	.quad	0x0	# Location list terminator begin (*.LLST19)
	.quad	0x0	# Location list terminator end (*.LLST19)
.LLST20:
	.quad	.LVL33-.Ltext0	# Location list begin address (*.LLST20)
	.quad	.LVL35-.Ltext0	# Location list end address (*.LLST20)
	.value	0x1	# Location expression size
	.byte	0x51	# DW_OP_reg1
	.quad	0x0	# Location list terminator begin (*.LLST20)
	.quad	0x0	# Location list terminator end (*.LLST20)
.LLST21:
	.quad	.LVL33-.Ltext0	# Location list begin address (*.LLST21)
	.quad	.LVL35-.Ltext0	# Location list end address (*.LLST21)
	.value	0x5	# Location expression size
	.byte	0x71	# DW_OP_breg1
	.sleb128 0
	.byte	0x4f	# DW_OP_lit31
	.byte	0x1e	# DW_OP_mul
	.byte	0x9f	# DW_OP_stack_value
	.quad	0x0	# Location list terminator begin (*.LLST21)
	.quad	0x0	# Location list terminator end (*.LLST21)
.LLST22:
	.quad	.LVL36-.Ltext0	# Location list begin address (*.LLST22)
	.quad	.LVL37-.Ltext0	# Location list end address (*.LLST22)
	.value	0x1	# Location expression size
	.byte	0x50	# DW_OP_reg0
	.quad	0x0	# Location list terminator begin (*.LLST22)
	.quad	0x0	# Location list terminator end (*.LLST22)
	.section	.debug_info
	.long	0x540	# Length of Compilation Unit Info
	.value	0x3	# DWARF version number
	.long	.Ldebug_abbrev0	# Offset Into Abbrev. Section
	.byte	0x8	# Pointer Size (in bytes)
	.uleb128 0x1	# (DIE (0xb) DW_TAG_compile_unit)
	.long	.LASF17	# DW_AT_producer: "GNU C 4.5.1 20100924 (Red Hat 4.5.1-4)"
	.byte	0x1	# DW_AT_language
	.long	.LASF18	# DW_AT_name: "../gdb.opt/inline-break.c"
	.long	.LASF19	# DW_AT_comp_dir: "/home/gary/work/archer/src/gdb/testsuite/gdb.dwarf2"
	.quad	.Ltext0	# DW_AT_low_pc
	.quad	.Letext0	# DW_AT_high_pc
	.long	.Ldebug_line0	# DW_AT_stmt_list
	.uleb128 0x2	# (DIE (0x2d) DW_TAG_subprogram)
	.long	.LASF0	# DW_AT_name: "func3b"
	.byte	0x1	# DW_AT_decl_file (../gdb.opt/inline-break.c)
	.byte	0x2f	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	0x48	# DW_AT_type
	.byte	0x3	# DW_AT_inline
	.long	0x48	# DW_AT_sibling
	.uleb128 0x3	# (DIE (0x3e) DW_TAG_formal_parameter)
	.ascii "x\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (../gdb.opt/inline-break.c)
	.byte	0x2f	# DW_AT_decl_line
	.long	0x48	# DW_AT_type
	.byte	0x0	# end of children of DIE 0x2d
	.uleb128 0x4	# (DIE (0x48) DW_TAG_base_type)
	.byte	0x4	# DW_AT_byte_size
	.byte	0x5	# DW_AT_encoding
	.ascii "int\0"	# DW_AT_name
	.uleb128 0x2	# (DIE (0x4f) DW_TAG_subprogram)
	.long	.LASF1	# DW_AT_name: "func7b"
	.byte	0x1	# DW_AT_decl_file (../gdb.opt/inline-break.c)
	.byte	0x6a	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	0x48	# DW_AT_type
	.byte	0x3	# DW_AT_inline
	.long	0x6a	# DW_AT_sibling
	.uleb128 0x3	# (DIE (0x60) DW_TAG_formal_parameter)
	.ascii "x\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (../gdb.opt/inline-break.c)
	.byte	0x6a	# DW_AT_decl_line
	.long	0x48	# DW_AT_type
	.byte	0x0	# end of children of DIE 0x4f
	.uleb128 0x2	# (DIE (0x6a) DW_TAG_subprogram)
	.long	.LASF2	# DW_AT_name: "func4b"
	.byte	0x1	# DW_AT_decl_file (../gdb.opt/inline-break.c)
	.byte	0x3e	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	0x48	# DW_AT_type
	.byte	0x3	# DW_AT_inline
	.long	0x85	# DW_AT_sibling
	.uleb128 0x3	# (DIE (0x7b) DW_TAG_formal_parameter)
	.ascii "x\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (../gdb.opt/inline-break.c)
	.byte	0x3e	# DW_AT_decl_line
	.long	0x48	# DW_AT_type
	.byte	0x0	# end of children of DIE 0x6a
	.uleb128 0x5	# (DIE (0x85) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.long	.LASF3	# DW_AT_name: "func5b"
	.byte	0x1	# DW_AT_decl_file (../gdb.opt/inline-break.c)
	.byte	0x4d	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	0x48	# DW_AT_type
	.byte	0x3	# DW_AT_inline
	.long	0xa1	# DW_AT_sibling
	.uleb128 0x3	# (DIE (0x97) DW_TAG_formal_parameter)
	.ascii "x\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (../gdb.opt/inline-break.c)
	.byte	0x4d	# DW_AT_decl_line
	.long	0x48	# DW_AT_type
	.byte	0x0	# end of children of DIE 0x85
	.uleb128 0x5	# (DIE (0xa1) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.long	.LASF4	# DW_AT_name: "func6b"
	.byte	0x1	# DW_AT_decl_file (../gdb.opt/inline-break.c)
	.byte	0x5c	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	0x48	# DW_AT_type
	.byte	0x3	# DW_AT_inline
	.long	0xbd	# DW_AT_sibling
	.uleb128 0x3	# (DIE (0xb3) DW_TAG_formal_parameter)
	.ascii "x\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (../gdb.opt/inline-break.c)
	.byte	0x5c	# DW_AT_decl_line
	.long	0x48	# DW_AT_type
	.byte	0x0	# end of children of DIE 0xa1
	.uleb128 0x5	# (DIE (0xbd) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.long	.LASF5	# DW_AT_name: "func8b"
	.byte	0x1	# DW_AT_decl_file (../gdb.opt/inline-break.c)
	.byte	0x78	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	0x48	# DW_AT_type
	.byte	0x3	# DW_AT_inline
	.long	0xd9	# DW_AT_sibling
	.uleb128 0x3	# (DIE (0xcf) DW_TAG_formal_parameter)
	.ascii "x\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (../gdb.opt/inline-break.c)
	.byte	0x78	# DW_AT_decl_line
	.long	0x48	# DW_AT_type
	.byte	0x0	# end of children of DIE 0xbd
	.uleb128 0x2	# (DIE (0xd9) DW_TAG_subprogram)
	.long	.LASF6	# DW_AT_name: "func1"
	.byte	0x1	# DW_AT_decl_file (../gdb.opt/inline-break.c)
	.byte	0x1e	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	0x48	# DW_AT_type
	.byte	0x3	# DW_AT_inline
	.long	0xf4	# DW_AT_sibling
	.uleb128 0x3	# (DIE (0xea) DW_TAG_formal_parameter)
	.ascii "x\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (../gdb.opt/inline-break.c)
	.byte	0x1e	# DW_AT_decl_line
	.long	0x48	# DW_AT_type
	.byte	0x0	# end of children of DIE 0xd9
	.uleb128 0x5	# (DIE (0xf4) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.long	.LASF7	# DW_AT_name: "func2"
	.byte	0x1	# DW_AT_decl_file (../gdb.opt/inline-break.c)
	.byte	0x26	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	0x48	# DW_AT_type
	.byte	0x3	# DW_AT_inline
	.long	0x110	# DW_AT_sibling
	.uleb128 0x3	# (DIE (0x106) DW_TAG_formal_parameter)
	.ascii "x\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (../gdb.opt/inline-break.c)
	.byte	0x26	# DW_AT_decl_line
	.long	0x48	# DW_AT_type
	.byte	0x0	# end of children of DIE 0xf4
	.uleb128 0x2	# (DIE (0x110) DW_TAG_subprogram)
	.long	.LASF8	# DW_AT_name: "func3a"
	.byte	0x1	# DW_AT_decl_file (../gdb.opt/inline-break.c)
	.byte	0x35	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	0x48	# DW_AT_type
	.byte	0x3	# DW_AT_inline
	.long	0x12b	# DW_AT_sibling
	.uleb128 0x3	# (DIE (0x121) DW_TAG_formal_parameter)
	.ascii "x\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (../gdb.opt/inline-break.c)
	.byte	0x35	# DW_AT_decl_line
	.long	0x48	# DW_AT_type
	.byte	0x0	# end of children of DIE 0x110
	.uleb128 0x5	# (DIE (0x12b) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.long	.LASF9	# DW_AT_name: "func4a"
	.byte	0x1	# DW_AT_decl_file (../gdb.opt/inline-break.c)
	.byte	0x44	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	0x48	# DW_AT_type
	.byte	0x3	# DW_AT_inline
	.long	0x147	# DW_AT_sibling
	.uleb128 0x3	# (DIE (0x13d) DW_TAG_formal_parameter)
	.ascii "x\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (../gdb.opt/inline-break.c)
	.byte	0x44	# DW_AT_decl_line
	.long	0x48	# DW_AT_type
	.byte	0x0	# end of children of DIE 0x12b
	.uleb128 0x2	# (DIE (0x147) DW_TAG_subprogram)
	.long	.LASF10	# DW_AT_name: "func5a"
	.byte	0x1	# DW_AT_decl_file (../gdb.opt/inline-break.c)
	.byte	0x53	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	0x48	# DW_AT_type
	.byte	0x3	# DW_AT_inline
	.long	0x162	# DW_AT_sibling
	.uleb128 0x3	# (DIE (0x158) DW_TAG_formal_parameter)
	.ascii "x\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (../gdb.opt/inline-break.c)
	.byte	0x53	# DW_AT_decl_line
	.long	0x48	# DW_AT_type
	.byte	0x0	# end of children of DIE 0x147
	.uleb128 0x5	# (DIE (0x162) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.long	.LASF11	# DW_AT_name: "func6a"
	.byte	0x1	# DW_AT_decl_file (../gdb.opt/inline-break.c)
	.byte	0x62	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	0x48	# DW_AT_type
	.byte	0x3	# DW_AT_inline
	.long	0x17e	# DW_AT_sibling
	.uleb128 0x3	# (DIE (0x174) DW_TAG_formal_parameter)
	.ascii "x\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (../gdb.opt/inline-break.c)
	.byte	0x62	# DW_AT_decl_line
	.long	0x48	# DW_AT_type
	.byte	0x0	# end of children of DIE 0x162
	.uleb128 0x2	# (DIE (0x17e) DW_TAG_subprogram)
	.long	.LASF12	# DW_AT_name: "func7a"
	.byte	0x1	# DW_AT_decl_file (../gdb.opt/inline-break.c)
	.byte	0x70	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	0x48	# DW_AT_type
	.byte	0x3	# DW_AT_inline
	.long	0x199	# DW_AT_sibling
	.uleb128 0x3	# (DIE (0x18f) DW_TAG_formal_parameter)
	.ascii "x\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (../gdb.opt/inline-break.c)
	.byte	0x70	# DW_AT_decl_line
	.long	0x48	# DW_AT_type
	.byte	0x0	# end of children of DIE 0x17e
	.uleb128 0x2	# (DIE (0x199) DW_TAG_subprogram)
	.long	.LASF13	# DW_AT_name: "func8a"
	.byte	0x1	# DW_AT_decl_file (../gdb.opt/inline-break.c)
	.byte	0x7e	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	0x48	# DW_AT_type
	.byte	0x3	# DW_AT_inline
	.long	0x1b4	# DW_AT_sibling
	.uleb128 0x3	# (DIE (0x1aa) DW_TAG_formal_parameter)
	.ascii "x\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (../gdb.opt/inline-break.c)
	.byte	0x7e	# DW_AT_decl_line
	.long	0x48	# DW_AT_type
	.byte	0x0	# end of children of DIE 0x199
	.uleb128 0x6	# (DIE (0x1b4) DW_TAG_subprogram)
	.long	0xf4	# DW_AT_abstract_origin
	.quad	.LFB1	# DW_AT_low_pc
	.quad	.LFE1	# DW_AT_high_pc
	.byte	0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
	.long	0x1d7	# DW_AT_sibling
	.uleb128 0x7	# (DIE (0x1cf) DW_TAG_formal_parameter)
	.long	0x106	# DW_AT_abstract_origin
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x0	# end of children of DIE 0x1b4
	.uleb128 0x6	# (DIE (0x1d7) DW_TAG_subprogram)
	.long	0x12b	# DW_AT_abstract_origin
	.quad	.LFB5	# DW_AT_low_pc
	.quad	.LFE5	# DW_AT_high_pc
	.byte	0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
	.long	0x21d	# DW_AT_sibling
	.uleb128 0x8	# (DIE (0x1f2) DW_TAG_formal_parameter)
	.long	0x13d	# DW_AT_abstract_origin
	.long	.LLST0	# DW_AT_location
	.uleb128 0x9	# (DIE (0x1fb) DW_TAG_inlined_subroutine)
	.long	0x6a	# DW_AT_abstract_origin
	.quad	.LBB46	# DW_AT_low_pc
	.quad	.LBE46	# DW_AT_high_pc
	.byte	0x1	# DW_AT_call_file (../gdb.opt/inline-break.c)
	.byte	0x46	# DW_AT_call_line
	.uleb128 0x8	# (DIE (0x212) DW_TAG_formal_parameter)
	.long	0x7b	# DW_AT_abstract_origin
	.long	.LLST1	# DW_AT_location
	.byte	0x0	# end of children of DIE 0x1fb
	.byte	0x0	# end of children of DIE 0x1d7
	.uleb128 0x6	# (DIE (0x21d) DW_TAG_subprogram)
	.long	0x85	# DW_AT_abstract_origin
	.quad	.LFB6	# DW_AT_low_pc
	.quad	.LFE6	# DW_AT_high_pc
	.byte	0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
	.long	0x240	# DW_AT_sibling
	.uleb128 0x7	# (DIE (0x238) DW_TAG_formal_parameter)
	.long	0x97	# DW_AT_abstract_origin
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x0	# end of children of DIE 0x21d
	.uleb128 0x6	# (DIE (0x240) DW_TAG_subprogram)
	.long	0xa1	# DW_AT_abstract_origin
	.quad	.LFB8	# DW_AT_low_pc
	.quad	.LFE8	# DW_AT_high_pc
	.byte	0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
	.long	0x263	# DW_AT_sibling
	.uleb128 0x7	# (DIE (0x25b) DW_TAG_formal_parameter)
	.long	0xb3	# DW_AT_abstract_origin
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x0	# end of children of DIE 0x240
	.uleb128 0x6	# (DIE (0x263) DW_TAG_subprogram)
	.long	0x162	# DW_AT_abstract_origin
	.quad	.LFB9	# DW_AT_low_pc
	.quad	.LFE9	# DW_AT_high_pc
	.byte	0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
	.long	0x2a9	# DW_AT_sibling
	.uleb128 0x8	# (DIE (0x27e) DW_TAG_formal_parameter)
	.long	0x174	# DW_AT_abstract_origin
	.long	.LLST2	# DW_AT_location
	.uleb128 0x9	# (DIE (0x287) DW_TAG_inlined_subroutine)
	.long	0xa1	# DW_AT_abstract_origin
	.quad	.LBB48	# DW_AT_low_pc
	.quad	.LBE48	# DW_AT_high_pc
	.byte	0x1	# DW_AT_call_file (../gdb.opt/inline-break.c)
	.byte	0x64	# DW_AT_call_line
	.uleb128 0x8	# (DIE (0x29e) DW_TAG_formal_parameter)
	.long	0xb3	# DW_AT_abstract_origin
	.long	.LLST3	# DW_AT_location
	.byte	0x0	# end of children of DIE 0x287
	.byte	0x0	# end of children of DIE 0x263
	.uleb128 0x6	# (DIE (0x2a9) DW_TAG_subprogram)
	.long	0xbd	# DW_AT_abstract_origin
	.quad	.LFB12	# DW_AT_low_pc
	.quad	.LFE12	# DW_AT_high_pc
	.byte	0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
	.long	0x2cc	# DW_AT_sibling
	.uleb128 0x7	# (DIE (0x2c4) DW_TAG_formal_parameter)
	.long	0xcf	# DW_AT_abstract_origin
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x0	# end of children of DIE 0x2a9
	.uleb128 0xa	# (DIE (0x2cc) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.long	.LASF20	# DW_AT_name: "main"
	.byte	0x1	# DW_AT_decl_file (../gdb.opt/inline-break.c)
	.byte	0x86	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	0x48	# DW_AT_type
	.quad	.LFB14	# DW_AT_low_pc
	.quad	.LFE14	# DW_AT_high_pc
	.byte	0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
	.long	0x52b	# DW_AT_sibling
	.uleb128 0xb	# (DIE (0x2ef) DW_TAG_formal_parameter)
	.long	.LASF14	# DW_AT_name: "argc"
	.byte	0x1	# DW_AT_decl_file (../gdb.opt/inline-break.c)
	.byte	0x86	# DW_AT_decl_line
	.long	0x48	# DW_AT_type
	.long	.LLST4	# DW_AT_location
	.uleb128 0xb	# (DIE (0x2fe) DW_TAG_formal_parameter)
	.long	.LASF15	# DW_AT_name: "argv"
	.byte	0x1	# DW_AT_decl_file (../gdb.opt/inline-break.c)
	.byte	0x86	# DW_AT_decl_line
	.long	0x52b	# DW_AT_type
	.long	.LLST5	# DW_AT_location
	.uleb128 0xc	# (DIE (0x30d) DW_TAG_variable)
	.ascii "x\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (../gdb.opt/inline-break.c)
	.byte	0x8c	# DW_AT_decl_line
	.long	0x53e	# DW_AT_type
	.long	.LLST6	# DW_AT_location
	.uleb128 0xd	# (DIE (0x31a) DW_TAG_inlined_subroutine)
	.long	0xd9	# DW_AT_abstract_origin
	.quad	.LBB50	# DW_AT_entry_pc
	.long	.Ldebug_ranges0+0x0	# DW_AT_ranges
	.byte	0x1	# DW_AT_call_file (../gdb.opt/inline-break.c)
	.byte	0x8e	# DW_AT_call_line
	.long	0x33b	# DW_AT_sibling
	.uleb128 0x8	# (DIE (0x331) DW_TAG_formal_parameter)
	.long	0xea	# DW_AT_abstract_origin
	.long	.LLST7	# DW_AT_location
	.byte	0x0	# end of children of DIE 0x31a
	.uleb128 0xd	# (DIE (0x33b) DW_TAG_inlined_subroutine)
	.long	0x110	# DW_AT_abstract_origin
	.quad	.LBB53	# DW_AT_entry_pc
	.long	.Ldebug_ranges0+0x30	# DW_AT_ranges
	.byte	0x1	# DW_AT_call_file (../gdb.opt/inline-break.c)
	.byte	0x92	# DW_AT_call_line
	.long	0x37d	# DW_AT_sibling
	.uleb128 0x8	# (DIE (0x352) DW_TAG_formal_parameter)
	.long	0x121	# DW_AT_abstract_origin
	.long	.LLST8	# DW_AT_location
	.uleb128 0x9	# (DIE (0x35b) DW_TAG_inlined_subroutine)
	.long	0x2d	# DW_AT_abstract_origin
	.quad	.LBB55	# DW_AT_low_pc
	.quad	.LBE55	# DW_AT_high_pc
	.byte	0x1	# DW_AT_call_file (../gdb.opt/inline-break.c)
	.byte	0x37	# DW_AT_call_line
	.uleb128 0x8	# (DIE (0x372) DW_TAG_formal_parameter)
	.long	0x3e	# DW_AT_abstract_origin
	.long	.LLST9	# DW_AT_location
	.byte	0x0	# end of children of DIE 0x35b
	.byte	0x0	# end of children of DIE 0x33b
	.uleb128 0xd	# (DIE (0x37d) DW_TAG_inlined_subroutine)
	.long	0x147	# DW_AT_abstract_origin
	.quad	.LBB58	# DW_AT_entry_pc
	.long	.Ldebug_ranges0+0x60	# DW_AT_ranges
	.byte	0x1	# DW_AT_call_file (../gdb.opt/inline-break.c)
	.byte	0x96	# DW_AT_call_line
	.long	0x3bf	# DW_AT_sibling
	.uleb128 0x8	# (DIE (0x394) DW_TAG_formal_parameter)
	.long	0x158	# DW_AT_abstract_origin
	.long	.LLST10	# DW_AT_location
	.uleb128 0x9	# (DIE (0x39d) DW_TAG_inlined_subroutine)
	.long	0x85	# DW_AT_abstract_origin
	.quad	.LBB60	# DW_AT_low_pc
	.quad	.LBE60	# DW_AT_high_pc
	.byte	0x1	# DW_AT_call_file (../gdb.opt/inline-break.c)
	.byte	0x55	# DW_AT_call_line
	.uleb128 0x8	# (DIE (0x3b4) DW_TAG_formal_parameter)
	.long	0x97	# DW_AT_abstract_origin
	.long	.LLST11	# DW_AT_location
	.byte	0x0	# end of children of DIE 0x39d
	.byte	0x0	# end of children of DIE 0x37d
	.uleb128 0xe	# (DIE (0x3bf) DW_TAG_inlined_subroutine)
	.long	0xf4	# DW_AT_abstract_origin
	.quad	.LBB64	# DW_AT_low_pc
	.quad	.LBE64	# DW_AT_high_pc
	.byte	0x1	# DW_AT_call_file (../gdb.opt/inline-break.c)
	.byte	0x90	# DW_AT_call_line
	.long	0x3e4	# DW_AT_sibling
	.uleb128 0x8	# (DIE (0x3da) DW_TAG_formal_parameter)
	.long	0x106	# DW_AT_abstract_origin
	.long	.LLST12	# DW_AT_location
	.byte	0x0	# end of children of DIE 0x3bf
	.uleb128 0xe	# (DIE (0x3e4) DW_TAG_inlined_subroutine)
	.long	0x12b	# DW_AT_abstract_origin
	.quad	.LBB67	# DW_AT_low_pc
	.quad	.LBE67	# DW_AT_high_pc
	.byte	0x1	# DW_AT_call_file (../gdb.opt/inline-break.c)
	.byte	0x94	# DW_AT_call_line
	.long	0x42a	# DW_AT_sibling
	.uleb128 0x8	# (DIE (0x3ff) DW_TAG_formal_parameter)
	.long	0x13d	# DW_AT_abstract_origin
	.long	.LLST13	# DW_AT_location
	.uleb128 0x9	# (DIE (0x408) DW_TAG_inlined_subroutine)
	.long	0x6a	# DW_AT_abstract_origin
	.quad	.LBB69	# DW_AT_low_pc
	.quad	.LBE69	# DW_AT_high_pc
	.byte	0x1	# DW_AT_call_file (../gdb.opt/inline-break.c)
	.byte	0x46	# DW_AT_call_line
	.uleb128 0x8	# (DIE (0x41f) DW_TAG_formal_parameter)
	.long	0x7b	# DW_AT_abstract_origin
	.long	.LLST14	# DW_AT_location
	.byte	0x0	# end of children of DIE 0x408
	.byte	0x0	# end of children of DIE 0x3e4
	.uleb128 0xd	# (DIE (0x42a) DW_TAG_inlined_subroutine)
	.long	0x162	# DW_AT_abstract_origin
	.quad	.LBB72	# DW_AT_entry_pc
	.long	.Ldebug_ranges0+0x90	# DW_AT_ranges
	.byte	0x1	# DW_AT_call_file (../gdb.opt/inline-break.c)
	.byte	0x98	# DW_AT_call_line
	.long	0x46c	# DW_AT_sibling
	.uleb128 0x8	# (DIE (0x441) DW_TAG_formal_parameter)
	.long	0x174	# DW_AT_abstract_origin
	.long	.LLST15	# DW_AT_location
	.uleb128 0x9	# (DIE (0x44a) DW_TAG_inlined_subroutine)
	.long	0xa1	# DW_AT_abstract_origin
	.quad	.LBB74	# DW_AT_low_pc
	.quad	.LBE74	# DW_AT_high_pc
	.byte	0x1	# DW_AT_call_file (../gdb.opt/inline-break.c)
	.byte	0x64	# DW_AT_call_line
	.uleb128 0x8	# (DIE (0x461) DW_TAG_formal_parameter)
	.long	0xb3	# DW_AT_abstract_origin
	.long	.LLST16	# DW_AT_location
	.byte	0x0	# end of children of DIE 0x44a
	.byte	0x0	# end of children of DIE 0x42a
	.uleb128 0xd	# (DIE (0x46c) DW_TAG_inlined_subroutine)
	.long	0x17e	# DW_AT_abstract_origin
	.quad	.LBB77	# DW_AT_entry_pc
	.long	.Ldebug_ranges0+0xc0	# DW_AT_ranges
	.byte	0x1	# DW_AT_call_file (../gdb.opt/inline-break.c)
	.byte	0x9a	# DW_AT_call_line
	.long	0x4aa	# DW_AT_sibling
	.uleb128 0x8	# (DIE (0x483) DW_TAG_formal_parameter)
	.long	0x18f	# DW_AT_abstract_origin
	.long	.LLST17	# DW_AT_location
	.uleb128 0xf	# (DIE (0x48c) DW_TAG_inlined_subroutine)
	.long	0x4f	# DW_AT_abstract_origin
	.quad	.LBB82	# DW_AT_entry_pc
	.long	.Ldebug_ranges0+0x120	# DW_AT_ranges
	.byte	0x1	# DW_AT_call_file (../gdb.opt/inline-break.c)
	.byte	0x72	# DW_AT_call_line
	.uleb128 0x8	# (DIE (0x49f) DW_TAG_formal_parameter)
	.long	0x60	# DW_AT_abstract_origin
	.long	.LLST18	# DW_AT_location
	.byte	0x0	# end of children of DIE 0x48c
	.byte	0x0	# end of children of DIE 0x46c
	.uleb128 0xd	# (DIE (0x4aa) DW_TAG_inlined_subroutine)
	.long	0x4f	# DW_AT_abstract_origin
	.quad	.LBB92	# DW_AT_entry_pc
	.long	.Ldebug_ranges0+0x160	# DW_AT_ranges
	.byte	0x1	# DW_AT_call_file (../gdb.opt/inline-break.c)
	.byte	0x9a	# DW_AT_call_line
	.long	0x4cb	# DW_AT_sibling
	.uleb128 0x8	# (DIE (0x4c1) DW_TAG_formal_parameter)
	.long	0x60	# DW_AT_abstract_origin
	.long	.LLST19	# DW_AT_location
	.byte	0x0	# end of children of DIE 0x4aa
	.uleb128 0xd	# (DIE (0x4cb) DW_TAG_inlined_subroutine)
	.long	0x199	# DW_AT_abstract_origin
	.quad	.LBB100	# DW_AT_entry_pc
	.long	.Ldebug_ranges0+0x1a0	# DW_AT_ranges
	.byte	0x1	# DW_AT_call_file (../gdb.opt/inline-break.c)
	.byte	0x9c	# DW_AT_call_line
	.long	0x509	# DW_AT_sibling
	.uleb128 0x8	# (DIE (0x4e2) DW_TAG_formal_parameter)
	.long	0x1aa	# DW_AT_abstract_origin
	.long	.LLST20	# DW_AT_location
	.uleb128 0xf	# (DIE (0x4eb) DW_TAG_inlined_subroutine)
	.long	0xbd	# DW_AT_abstract_origin
	.quad	.LBB102	# DW_AT_entry_pc
	.long	.Ldebug_ranges0+0x1d0	# DW_AT_ranges
	.byte	0x1	# DW_AT_call_file (../gdb.opt/inline-break.c)
	.byte	0x80	# DW_AT_call_line
	.uleb128 0x8	# (DIE (0x4fe) DW_TAG_formal_parameter)
	.long	0xcf	# DW_AT_abstract_origin
	.long	.LLST21	# DW_AT_location
	.byte	0x0	# end of children of DIE 0x4eb
	.byte	0x0	# end of children of DIE 0x4cb
	.uleb128 0x9	# (DIE (0x509) DW_TAG_inlined_subroutine)
	.long	0xbd	# DW_AT_abstract_origin
	.quad	.LBB108	# DW_AT_low_pc
	.quad	.LBE108	# DW_AT_high_pc
	.byte	0x1	# DW_AT_call_file (../gdb.opt/inline-break.c)
	.byte	0x9c	# DW_AT_call_line
	.uleb128 0x8	# (DIE (0x520) DW_TAG_formal_parameter)
	.long	0xcf	# DW_AT_abstract_origin
	.long	.LLST22	# DW_AT_location
	.byte	0x0	# end of children of DIE 0x509
	.byte	0x0	# end of children of DIE 0x2cc
	.uleb128 0x10	# (DIE (0x52b) DW_TAG_pointer_type)
	.byte	0x8	# DW_AT_byte_size
	.long	0x531	# DW_AT_type
	.uleb128 0x10	# (DIE (0x531) DW_TAG_pointer_type)
	.byte	0x8	# DW_AT_byte_size
	.long	0x537	# DW_AT_type
	.uleb128 0x11	# (DIE (0x537) DW_TAG_base_type)
	.byte	0x1	# DW_AT_byte_size
	.byte	0x6	# DW_AT_encoding
	.long	.LASF16	# DW_AT_name: "char"
	.uleb128 0x12	# (DIE (0x53e) DW_TAG_volatile_type)
	.long	0x48	# DW_AT_type
	.byte	0x0	# end of children of DIE 0xb
	.section	.debug_abbrev
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
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x10	# (DW_AT_stmt_list)
	.uleb128 0x6	# (DW_FORM_data4)
	.byte	0x0
	.byte	0x0
	.uleb128 0x2	# (abbrev code)
	.uleb128 0x2e	# (TAG: DW_TAG_subprogram)
	.byte	0x1	# DW_children_yes
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x27	# (DW_AT_prototyped)
	.uleb128 0xc	# (DW_FORM_flag)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x20	# (DW_AT_inline)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0x0
	.byte	0x0
	.uleb128 0x3	# (abbrev code)
	.uleb128 0x5	# (TAG: DW_TAG_formal_parameter)
	.byte	0x0	# DW_children_no
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0x0
	.byte	0x0
	.uleb128 0x4	# (abbrev code)
	.uleb128 0x24	# (TAG: DW_TAG_base_type)
	.byte	0x0	# DW_children_no
	.uleb128 0xb	# (DW_AT_byte_size)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3e	# (DW_AT_encoding)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
	.byte	0x0
	.byte	0x0
	.uleb128 0x5	# (abbrev code)
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
	.uleb128 0x27	# (DW_AT_prototyped)
	.uleb128 0xc	# (DW_FORM_flag)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x20	# (DW_AT_inline)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0x0
	.byte	0x0
	.uleb128 0x6	# (abbrev code)
	.uleb128 0x2e	# (TAG: DW_TAG_subprogram)
	.byte	0x1	# DW_children_yes
	.uleb128 0x31	# (DW_AT_abstract_origin)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x40	# (DW_AT_frame_base)
	.uleb128 0xa	# (DW_FORM_block1)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0x0
	.byte	0x0
	.uleb128 0x7	# (abbrev code)
	.uleb128 0x5	# (TAG: DW_TAG_formal_parameter)
	.byte	0x0	# DW_children_no
	.uleb128 0x31	# (DW_AT_abstract_origin)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0xa	# (DW_FORM_block1)
	.byte	0x0
	.byte	0x0
	.uleb128 0x8	# (abbrev code)
	.uleb128 0x5	# (TAG: DW_TAG_formal_parameter)
	.byte	0x0	# DW_children_no
	.uleb128 0x31	# (DW_AT_abstract_origin)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0x6	# (DW_FORM_data4)
	.byte	0x0
	.byte	0x0
	.uleb128 0x9	# (abbrev code)
	.uleb128 0x1d	# (TAG: DW_TAG_inlined_subroutine)
	.byte	0x1	# DW_children_yes
	.uleb128 0x31	# (DW_AT_abstract_origin)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x58	# (DW_AT_call_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x59	# (DW_AT_call_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.byte	0x0
	.byte	0x0
	.uleb128 0xa	# (abbrev code)
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
	.uleb128 0x27	# (DW_AT_prototyped)
	.uleb128 0xc	# (DW_FORM_flag)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x40	# (DW_AT_frame_base)
	.uleb128 0xa	# (DW_FORM_block1)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0x0
	.byte	0x0
	.uleb128 0xb	# (abbrev code)
	.uleb128 0x5	# (TAG: DW_TAG_formal_parameter)
	.byte	0x0	# DW_children_no
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
	.byte	0x0
	.byte	0x0
	.uleb128 0xc	# (abbrev code)
	.uleb128 0x34	# (TAG: DW_TAG_variable)
	.byte	0x0	# DW_children_no
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
	.byte	0x0
	.byte	0x0
	.uleb128 0xd	# (abbrev code)
	.uleb128 0x1d	# (TAG: DW_TAG_inlined_subroutine)
	.byte	0x1	# DW_children_yes
	.uleb128 0x31	# (DW_AT_abstract_origin)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x52	# (DW_AT_entry_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x55	# (DW_AT_ranges)
	.uleb128 0x6	# (DW_FORM_data4)
	.uleb128 0x58	# (DW_AT_call_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x59	# (DW_AT_call_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0x0
	.byte	0x0
	.uleb128 0xe	# (abbrev code)
	.uleb128 0x1d	# (TAG: DW_TAG_inlined_subroutine)
	.byte	0x1	# DW_children_yes
	.uleb128 0x31	# (DW_AT_abstract_origin)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x58	# (DW_AT_call_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x59	# (DW_AT_call_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0x0
	.byte	0x0
	.uleb128 0xf	# (abbrev code)
	.uleb128 0x1d	# (TAG: DW_TAG_inlined_subroutine)
	.byte	0x1	# DW_children_yes
	.uleb128 0x31	# (DW_AT_abstract_origin)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x52	# (DW_AT_entry_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x55	# (DW_AT_ranges)
	.uleb128 0x6	# (DW_FORM_data4)
	.uleb128 0x58	# (DW_AT_call_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x59	# (DW_AT_call_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.byte	0x0
	.byte	0x0
	.uleb128 0x10	# (abbrev code)
	.uleb128 0xf	# (TAG: DW_TAG_pointer_type)
	.byte	0x0	# DW_children_no
	.uleb128 0xb	# (DW_AT_byte_size)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0x0
	.byte	0x0
	.uleb128 0x11	# (abbrev code)
	.uleb128 0x24	# (TAG: DW_TAG_base_type)
	.byte	0x0	# DW_children_no
	.uleb128 0xb	# (DW_AT_byte_size)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3e	# (DW_AT_encoding)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.byte	0x0
	.byte	0x0
	.uleb128 0x12	# (abbrev code)
	.uleb128 0x35	# (TAG: DW_TAG_volatile_type)
	.byte	0x0	# DW_children_no
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.section	.debug_pubnames,"",@progbits
	.long	0x58	# Length of Public Names Info
	.value	0x2	# DWARF Version
	.long	.Ldebug_info0	# Offset of Compilation Unit Info
	.long	0x544	# Compilation Unit Length
	.long	0x1b4	# DIE offset
	.ascii "func2\0"	# external name
	.long	0x1d7	# DIE offset
	.ascii "func4a\0"	# external name
	.long	0x21d	# DIE offset
	.ascii "func5b\0"	# external name
	.long	0x240	# DIE offset
	.ascii "func6b\0"	# external name
	.long	0x263	# DIE offset
	.ascii "func6a\0"	# external name
	.long	0x2a9	# DIE offset
	.ascii "func8b\0"	# external name
	.long	0x2cc	# DIE offset
	.ascii "main\0"	# external name
	.long	0x0
	.section	.debug_aranges,"",@progbits
	.long	0x2c	# Length of Address Ranges Info
	.value	0x2	# DWARF Version
	.long	.Ldebug_info0	# Offset of Compilation Unit Info
	.byte	0x8	# Size of Address
	.byte	0x0	# Size of Segment Descriptor
	.value	0x0	# Pad to 16 byte boundary
	.value	0x0
	.quad	.Ltext0	# Address
	.quad	.Letext0-.Ltext0	# Length
	.quad	0x0
	.quad	0x0
	.section	.debug_ranges,"",@progbits
.Ldebug_ranges0:
	.quad	.LBB50-.Ltext0	# Offset 0x0
	.quad	.LBE50-.Ltext0
	.quad	.LBB63-.Ltext0
	.quad	.LBE63-.Ltext0
	.quad	0x0
	.quad	0x0
	.quad	.LBB53-.Ltext0	# Offset 0x30
	.quad	.LBE53-.Ltext0
	.quad	.LBB66-.Ltext0
	.quad	.LBE66-.Ltext0
	.quad	0x0
	.quad	0x0
	.quad	.LBB58-.Ltext0	# Offset 0x60
	.quad	.LBE58-.Ltext0
	.quad	.LBB71-.Ltext0
	.quad	.LBE71-.Ltext0
	.quad	0x0
	.quad	0x0
	.quad	.LBB72-.Ltext0	# Offset 0x90
	.quad	.LBE72-.Ltext0
	.quad	.LBB89-.Ltext0
	.quad	.LBE89-.Ltext0
	.quad	0x0
	.quad	0x0
	.quad	.LBB77-.Ltext0	# Offset 0xc0
	.quad	.LBE77-.Ltext0
	.quad	.LBB98-.Ltext0
	.quad	.LBE98-.Ltext0
	.quad	.LBB96-.Ltext0
	.quad	.LBE96-.Ltext0
	.quad	.LBB91-.Ltext0
	.quad	.LBE91-.Ltext0
	.quad	.LBB90-.Ltext0
	.quad	.LBE90-.Ltext0
	.quad	0x0
	.quad	0x0
	.quad	.LBB82-.Ltext0	# Offset 0x120
	.quad	.LBE82-.Ltext0
	.quad	.LBB86-.Ltext0
	.quad	.LBE86-.Ltext0
	.quad	.LBB81-.Ltext0
	.quad	.LBE81-.Ltext0
	.quad	0x0
	.quad	0x0
	.quad	.LBB92-.Ltext0	# Offset 0x160
	.quad	.LBE92-.Ltext0
	.quad	.LBB99-.Ltext0
	.quad	.LBE99-.Ltext0
	.quad	.LBB97-.Ltext0
	.quad	.LBE97-.Ltext0
	.quad	0x0
	.quad	0x0
	.quad	.LBB100-.Ltext0	# Offset 0x1a0
	.quad	.LBE100-.Ltext0
	.quad	.LBB107-.Ltext0
	.quad	.LBE107-.Ltext0
	.quad	0x0
	.quad	0x0
	.quad	.LBB102-.Ltext0	# Offset 0x1d0
	.quad	.LBE102-.Ltext0
	.quad	.LBB105-.Ltext0
	.quad	.LBE105-.Ltext0
	.quad	0x0
	.quad	0x0
	.section	.debug_str,"MS",@progbits,1
.LASF12:
	.string	"func7a"
.LASF1:
	.string	"func7b"
.LASF19:
	.string	"/home/gary/work/archer/src/gdb/testsuite/gdb.dwarf2"
.LASF20:
	.string	"main"
.LASF18:
	.string	"../gdb.opt/inline-break.c"
.LASF6:
	.string	"func1"
.LASF17:
	.string	"GNU C 4.5.1 20100924 (Red Hat 4.5.1-4)"
.LASF14:
	.string	"argc"
.LASF8:
	.string	"func3a"
.LASF0:
	.string	"func3b"
.LASF16:
	.string	"char"
.LASF9:
	.string	"func4a"
.LASF2:
	.string	"func4b"
.LASF11:
	.string	"func6a"
.LASF10:
	.string	"func5a"
.LASF3:
	.string	"func5b"
.LASF5:
	.string	"func8b"
.LASF13:
	.string	"func8a"
.LASF7:
	.string	"func2"
.LASF4:
	.string	"func6b"
.LASF15:
	.string	"argv"
	.ident	"GCC: (GNU) 4.5.1 20100924 (Red Hat 4.5.1-4)"
	.section	.note.GNU-stack,"",@progbits
