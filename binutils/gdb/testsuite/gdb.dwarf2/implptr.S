	.file	"implptr.c"
	.text
.Ltext0:
	.p2align 4,,15
	.globl	foo
	.type	foo, @function
foo:
.LFB1:
	.file 1 "gdb.dwarf2/implptr.c"
	.loc 1 45 0
	.cfi_startproc
.LVL0:
	.loc 1 45 0
	movl	4(%esp), %eax
.LVL1:
.LBB6:
.LBB7:
	.loc 1 36 0
	addl	$1, u
.LVL2:
	.loc 1 38 0
	addl	$1, u+4
.LVL3:
	.loc 1 40 0
	addl	$1, u+8
.LVL4:
.LBE7:
.LBE6:
.LBB8:
.LBB9:
	.loc 1 36 0
	addl	$1, u+12
.LVL5:
	.loc 1 38 0
	addl	$1, u+16
.LVL6:
	.loc 1 40 0
	addl	$1, u+20
.LBE9:
.LBE8:
	.loc 1 52 0
	leal	(%eax,%eax,4), %eax
	.loc 1 53 0
	ret
	.cfi_endproc
.LFE1:
	.size	foo, .-foo
	.p2align 4,,15
	.globl	bar
	.type	bar, @function
bar:
.LFB2:
	.loc 1 61 0
	.cfi_startproc
.LVL7:
	.loc 1 65 0
	movl	4(%esp), %eax
	addl	$1, %eax
	.loc 1 67 0
	ret
	.cfi_endproc
.LFE2:
	.size	bar, .-bar
	.section	.text.startup,"ax",@progbits
	.p2align 4,,15
	.globl	main
	.type	main, @function
main:
.LFB3:
	.loc 1 70 0
	.cfi_startproc
	pushl	%ebx
	.cfi_def_cfa_offset 8
	.cfi_offset 3, -8
	subl	$4, %esp
	.cfi_def_cfa_offset 12
	.loc 1 71 0
	movl	$5, (%esp)
	call	bar
.LVL8:
	movl	$23, (%esp)
	movl	%eax, %ebx
	call	foo
.LVL9:
	.loc 1 72 0
	addl	$4, %esp
	.cfi_def_cfa_offset 8
	.loc 1 71 0
	addl	%ebx, %eax
	.loc 1 72 0
	popl	%ebx
	.cfi_def_cfa_offset 4
	.cfi_restore 3
	ret
	.cfi_endproc
.LFE3:
	.size	main, .-main
	.comm	u,24,4
	.text
.Letext0:
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.long	0x217
	.value	0x4
	.long	.Ldebug_abbrev0
	.byte	0x4
	.uleb128 0x1
	.long	.LASF4
	.byte	0x1
	.long	.LASF5
	.long	.LASF6
	.long	.Ldebug_ranges0+0
	.long	0
	.long	.Ldebug_line0
	.uleb128 0x2
	.string	"S"
	.byte	0x8
	.byte	0x1
	.byte	0x18
	.long	0x44
	.uleb128 0x3
	.string	"x"
	.byte	0x1
	.byte	0x1a
	.long	0x44
	.byte	0
	.uleb128 0x3
	.string	"y"
	.byte	0x1
	.byte	0x1a
	.long	0x4a
	.byte	0x4
	.byte	0
	.uleb128 0x4
	.byte	0x4
	.long	0x4a
	.uleb128 0x5
	.byte	0x4
	.byte	0x5
	.string	"int"
	.uleb128 0x6
	.long	.LASF0
	.byte	0x1
	.byte	0x37
	.long	0x44
	.uleb128 0x6
	.long	.LASF1
	.byte	0x1
	.byte	0x38
	.long	0x67
	.uleb128 0x4
	.byte	0x4
	.long	0x51
	.uleb128 0x6
	.long	.LASF2
	.byte	0x1
	.byte	0x39
	.long	0x78
	.uleb128 0x4
	.byte	0x4
	.long	0x5c
	.uleb128 0x7
	.string	"add"
	.byte	0x1
	.byte	0x20
	.byte	0x3
	.long	0xa6
	.uleb128 0x8
	.string	"a"
	.byte	0x1
	.byte	0x20
	.long	0xa6
	.uleb128 0x8
	.string	"b"
	.byte	0x1
	.byte	0x20
	.long	0xa6
	.uleb128 0x8
	.string	"c"
	.byte	0x1
	.byte	0x20
	.long	0x4a
	.byte	0
	.uleb128 0x4
	.byte	0x4
	.long	0x25
	.uleb128 0x9
	.string	"foo"
	.byte	0x1
	.byte	0x2c
	.long	0x4a
	.long	.LFB1
	.long	.LFE1
	.uleb128 0x1
	.byte	0x9c
	.long	0x147
	.uleb128 0xa
	.string	"i"
	.byte	0x1
	.byte	0x2c
	.long	0x4a
	.long	.LLST0
	.uleb128 0xb
	.string	"j"
	.byte	0x1
	.byte	0x2e
	.long	0x4a
	.long	.LLST1
	.uleb128 0xb
	.string	"p"
	.byte	0x1
	.byte	0x2f
	.long	0x147
	.long	.LLST2
	.uleb128 0xc
	.long	0x7e
	.long	.LBB6
	.long	.LBE6
	.byte	0x1
	.byte	0x30
	.long	0x11b
	.uleb128 0xd
	.long	0x9c
	.byte	0
	.uleb128 0xe
	.long	0x93
	.uleb128 0x6
	.byte	0xf2
	.long	.Ldebug_info0+223
	.sleb128 8
	.uleb128 0xf
	.long	0x8a
	.long	.LLST3
	.byte	0
	.uleb128 0x10
	.long	0x7e
	.long	.LBB8
	.long	.LBE8
	.byte	0x1
	.byte	0x33
	.uleb128 0xd
	.long	0x9c
	.byte	0x3
	.uleb128 0xe
	.long	0x93
	.uleb128 0x6
	.byte	0xf2
	.long	.Ldebug_info0+223
	.sleb128 8
	.uleb128 0xf
	.long	0x8a
	.long	.LLST4
	.byte	0
	.byte	0
	.uleb128 0x11
	.long	0x25
	.long	0x157
	.uleb128 0x12
	.long	0x157
	.byte	0x1
	.byte	0
	.uleb128 0x13
	.byte	0x4
	.byte	0x7
	.long	.LASF3
	.uleb128 0x9
	.string	"bar"
	.byte	0x1
	.byte	0x3c
	.long	0x4a
	.long	.LFB2
	.long	.LFE2
	.uleb128 0x1
	.byte	0x9c
	.long	0x1b4
	.uleb128 0x14
	.string	"i"
	.byte	0x1
	.byte	0x3c
	.long	0x4a
	.uleb128 0x2
	.byte	0x91
	.sleb128 0
	.uleb128 0x15
	.string	"j"
	.byte	0x1
	.byte	0x3e
	.long	0x51
	.uleb128 0x6
	.byte	0xf2
	.long	.Ldebug_info0+375
	.sleb128 0
	.uleb128 0x15
	.string	"k"
	.byte	0x1
	.byte	0x3f
	.long	0x5c
	.uleb128 0x6
	.byte	0xf2
	.long	.Ldebug_info0+387
	.sleb128 0
	.uleb128 0x15
	.string	"l"
	.byte	0x1
	.byte	0x40
	.long	0x6d
	.uleb128 0x6
	.byte	0xf2
	.long	.Ldebug_info0+403
	.sleb128 0
	.byte	0
	.uleb128 0x16
	.long	.LASF7
	.byte	0x1
	.byte	0x45
	.long	0x4a
	.long	.LFB3
	.long	.LFE3
	.uleb128 0x1
	.byte	0x9c
	.long	0x1f2
	.uleb128 0x17
	.long	.LVL8
	.long	0x15e
	.long	0x1e1
	.uleb128 0x18
	.uleb128 0x2
	.byte	0x74
	.sleb128 0
	.uleb128 0x1
	.byte	0x35
	.byte	0
	.uleb128 0x19
	.long	.LVL9
	.long	0xac
	.uleb128 0x18
	.uleb128 0x2
	.byte	0x74
	.sleb128 0
	.uleb128 0x1
	.byte	0x47
	.byte	0
	.byte	0
	.uleb128 0x11
	.long	0x4a
	.long	0x202
	.uleb128 0x12
	.long	0x157
	.byte	0x5
	.byte	0
	.uleb128 0x1a
	.string	"u"
	.byte	0x1
	.byte	0x1d
	.long	0x1f2
	.uleb128 0x1b
	.string	"u"
	.byte	0x1
	.byte	0x1d
	.long	0x1f2
	.uleb128 0x5
	.byte	0x3
	.long	u
	.byte	0
	.section	.debug_abbrev,"",@progbits
.Ldebug_abbrev0:
	.uleb128 0x1
	.uleb128 0x11
	.byte	0x1
	.uleb128 0x25
	.uleb128 0xe
	.uleb128 0x13
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x1b
	.uleb128 0xe
	.uleb128 0x55
	.uleb128 0x17
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x10
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x2
	.uleb128 0x13
	.byte	0x1
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x3
	.uleb128 0xd
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x38
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0x4
	.uleb128 0xf
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x5
	.uleb128 0x24
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0x8
	.byte	0
	.byte	0
	.uleb128 0x6
	.uleb128 0x16
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x7
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x20
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x8
	.uleb128 0x5
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x9
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x40
	.uleb128 0x18
	.uleb128 0x2117
	.uleb128 0x19
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0xa
	.uleb128 0x5
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0xb
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0xc
	.uleb128 0x1d
	.byte	0x1
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x58
	.uleb128 0xb
	.uleb128 0x59
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0xd
	.uleb128 0x5
	.byte	0
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x1c
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0xe
	.uleb128 0x5
	.byte	0
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x18
	.byte	0
	.byte	0
	.uleb128 0xf
	.uleb128 0x5
	.byte	0
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x10
	.uleb128 0x1d
	.byte	0x1
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x58
	.uleb128 0xb
	.uleb128 0x59
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0x11
	.uleb128 0x1
	.byte	0x1
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x12
	.uleb128 0x21
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2f
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0x13
	.uleb128 0x24
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.byte	0
	.byte	0
	.uleb128 0x14
	.uleb128 0x5
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x18
	.byte	0
	.byte	0
	.uleb128 0x15
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x18
	.byte	0
	.byte	0
	.uleb128 0x16
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x40
	.uleb128 0x18
	.uleb128 0x2117
	.uleb128 0x19
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x17
	.uleb128 0x4109
	.byte	0x1
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x18
	.uleb128 0x410a
	.byte	0
	.uleb128 0x2
	.uleb128 0x18
	.uleb128 0x2111
	.uleb128 0x18
	.byte	0
	.byte	0
	.uleb128 0x19
	.uleb128 0x4109
	.byte	0x1
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x31
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x1a
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3c
	.uleb128 0x19
	.byte	0
	.byte	0
	.uleb128 0x1b
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x2
	.uleb128 0x18
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_loc,"",@progbits
.Ldebug_loc0:
.LLST0:
	.long	.LVL0
	.long	.LVL1
	.value	0x2
	.byte	0x91
	.sleb128 0
	.long	.LVL1
	.long	.LFE1
	.value	0x6
	.byte	0x91
	.sleb128 0
	.byte	0x6
	.byte	0x31
	.byte	0x24
	.byte	0x9f
	.long	0
	.long	0
.LLST1:
	.long	.LVL1
	.long	.LVL4
	.value	0x2
	.byte	0x91
	.sleb128 0
	.long	.LVL4
	.long	.LFE1
	.value	0x6
	.byte	0x91
	.sleb128 0
	.byte	0x6
	.byte	0x33
	.byte	0x1e
	.byte	0x9f
	.long	0
	.long	0
.LLST2:
	.long	.LVL1
	.long	.LVL4
	.value	0x20
	.byte	0xf2
	.long	.Ldebug_info0+197
	.sleb128 0
	.byte	0x93
	.uleb128 0x4
	.byte	0x91
	.sleb128 0
	.byte	0x6
	.byte	0x32
	.byte	0x24
	.byte	0x9f
	.byte	0x93
	.uleb128 0x4
	.byte	0xf2
	.long	.Ldebug_info0+210
	.sleb128 0
	.byte	0x93
	.uleb128 0x4
	.byte	0x91
	.sleb128 0
	.byte	0x6
	.byte	0x31
	.byte	0x24
	.byte	0x9f
	.byte	0x93
	.uleb128 0x4
	.long	.LVL4
	.long	.LFE1
	.value	0x20
	.byte	0xf2
	.long	.Ldebug_info0+210
	.sleb128 0
	.byte	0x93
	.uleb128 0x4
	.byte	0x91
	.sleb128 0
	.byte	0x6
	.byte	0x36
	.byte	0x1e
	.byte	0x9f
	.byte	0x93
	.uleb128 0x4
	.byte	0xf2
	.long	.Ldebug_info0+197
	.sleb128 0
	.byte	0x93
	.uleb128 0x4
	.byte	0x91
	.sleb128 0
	.byte	0x6
	.byte	0x31
	.byte	0x24
	.byte	0x9f
	.byte	0x93
	.uleb128 0x4
	.long	0
	.long	0
.LLST3:
	.long	.LVL1
	.long	.LVL2
	.value	0x6
	.byte	0xf2
	.long	.Ldebug_info0+223
	.sleb128 0
	.long	.LVL2
	.long	.LVL3
	.value	0x2
	.byte	0x30
	.byte	0x9f
	.long	.LVL3
	.long	.LFE1
	.value	0x6
	.byte	0xf2
	.long	.Ldebug_info0+223
	.sleb128 8
	.long	0
	.long	0
.LLST4:
	.long	.LVL4
	.long	.LVL5
	.value	0x6
	.byte	0xf2
	.long	.Ldebug_info0+223
	.sleb128 0
	.long	.LVL5
	.long	.LVL6
	.value	0x2
	.byte	0x30
	.byte	0x9f
	.long	.LVL6
	.long	.LFE1
	.value	0x6
	.byte	0xf2
	.long	.Ldebug_info0+223
	.sleb128 8
	.long	0
	.long	0
	.section	.debug_aranges,"",@progbits
	.long	0x24
	.value	0x2
	.long	.Ldebug_info0
	.byte	0x4
	.byte	0
	.value	0
	.value	0
	.long	.Ltext0
	.long	.Letext0-.Ltext0
	.long	.LFB3
	.long	.LFE3-.LFB3
	.long	0
	.long	0
	.section	.debug_ranges,"",@progbits
.Ldebug_ranges0:
	.long	.Ltext0
	.long	.Letext0
	.long	.LFB3
	.long	.LFE3
	.long	0
	.long	0
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.section	.debug_str,"MS",@progbits,1
.LASF5:
	.string	"gdb.dwarf2/implptr.c"
.LASF6:
	.string	""
.LASF3:
	.string	"unsigned int"
.LASF7:
	.string	"main"
.LASF2:
	.string	"intppp"
.LASF4:
	.string	"GNU C 4.6.1 20110627 (Red Hat 4.6.1-1)"
.LASF0:
	.string	"intp"
.LASF1:
	.string	"intpp"
	.ident	"GCC: (GNU) 4.6.1 20110627 (Red Hat 4.6.1-1)"
	.section	.note.GNU-stack,"",@progbits
