/*
   Copyright 2009-2024 Free Software Foundation, Inc.

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

/* This was compiled from a trivial program just to test the
   DW_OP_stack_value and DW_OP_implicit_value operators:

    unsigned int func (unsigned int arg) __attribute__ ((__noinline__));

    unsigned int func (unsigned int arg)
    {
      unsigned int uses_stack_op = 23;
      unsigned int uses_lit_op = 0x7fffffff;
      unsigned int result = arg;

      if (arg % 2)
	result += uses_lit_op + uses_stack_op;
      else
	{
	  ++uses_stack_op;
	  ++uses_lit_op;

	  result -= uses_stack_op + uses_lit_op;
	}

      return result * uses_stack_op;  // line 19, see the .exp file
    }

    int main (int argc, char *argv[])
    {
      return func (1024);
    }

  Then it was compiled with:
	
	gcc -fvar-tracking{,-assignments} -gdwarf-3
	-fno-inline{,-functions,-small-functions,-functions-called-once}
	-O2

*/

	.file	"valop.c"
	.section	.debug_abbrev,"",@progbits
.Ldebug_abbrev0:
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.text
.Ltext0:
	.p2align 4,,15
.globl func
	.type	func, @function
func:
.LFB0:
	.file 1 "valop.c"
	.loc 1 4 0
	.cfi_startproc
.LVL0:
	pushl	%ebp
	.cfi_def_cfa_offset 8
	movl	%esp, %ebp
	.cfi_offset 5, -8
	.cfi_def_cfa_register 5
	.loc 1 4 0
	movl	8(%ebp), %eax
.LVL1:
	.loc 1 9 0
	testb	$1, %al
	jne	.L5
.LVL2:
	.loc 1 16 0
	addl	$2147483624, %eax
.LVL3:
	.loc 1 13 0
	movl	$24, %edx
.LVL4:
	.loc 1 16 0
	imull	%edx, %eax
.LVL5:
	.loc 1 20 0
	popl	%ebp
	.cfi_remember_state
	.cfi_restore 5
	.cfi_def_cfa 4, 4
	ret
.LVL6:
	.p2align 4,,7
	.p2align 3
.L5:
	.cfi_restore_state
	.loc 1 10 0
	subl	$2147483626, %eax
.LVL7:
	.loc 1 5 0
	movl	$23, %edx
.LVL8:
	.loc 1 16 0
	imull	%edx, %eax
.LVL9:
	.loc 1 20 0
	popl	%ebp
	.cfi_def_cfa 4, 4
	.cfi_restore 5
	ret
	.cfi_endproc
.LFE0:
	.size	func, .-func
	.p2align 4,,15
.globl _start
	.type	_start, @function
_start:
.LFB1:
	.loc 1 23 0
	.cfi_startproc
.LVL10:
	pushl	%ebp
	.cfi_def_cfa_offset 8
	movl	%esp, %ebp
	.cfi_offset 5, -8
	.cfi_def_cfa_register 5
	subl	$4, %esp
	.loc 1 24 0
	movl	$1024, (%esp)
	call	func
.LVL11:
	.loc 1 25 0
	leave
	.cfi_restore 5
	.cfi_def_cfa 4, 4
	ret
	.cfi_endproc
.LFE1:
	.size	_start, .-_start
.Letext0:
	.section	.debug_loc,"",@progbits
.Ldebug_loc0:
.LLST0:
	.long	.LVL0-.Ltext0
	.long	.LVL2-.Ltext0
	.value	0x4
	.byte	0x47
	.byte	0x9f
	.byte	0x93
	.uleb128 0x4
	.long	.LVL2-.Ltext0
	.long	.LVL4-.Ltext0
	.value	0x4
	.byte	0x48
	.byte	0x9f
	.byte	0x93
	.uleb128 0x4
	.long	.LVL4-.Ltext0
	.long	.LVL6-.Ltext0
	.value	0x1
	.byte	0x52
	.long	.LVL6-.Ltext0
	.long	.LVL8-.Ltext0
	.value	0x4
	.byte	0x47
	.byte	0x9f
	.byte	0x93
	.uleb128 0x4
	.long	.LVL8-.Ltext0
	.long	.LFE0-.Ltext0
	.value	0x1
	.byte	0x52
	.long	0x0
	.long	0x0
.LLST1:
	.long	.LVL0-.Ltext0
	.long	.LVL2-.Ltext0
	.value	0x6
	.byte	0x9e
	.uleb128 0x4
	.long	0x7fffffff
	.long	.LVL2-.Ltext0
	.long	.LVL6-.Ltext0
	.value	0x6
	.byte	0x9e
	.uleb128 0x4
	.long	0x80000000
	.long	.LVL6-.Ltext0
	.long	.LFE0-.Ltext0
	.value	0x6
	.byte	0x9e
	.uleb128 0x4
	.long	0x7fffffff
	.long	0x0
	.long	0x0
.LLST2:
	.long	.LVL1-.Ltext0
	.long	.LVL3-.Ltext0
	.value	0x1
	.byte	0x50
	.long	.LVL3-.Ltext0
	.long	.LVL5-.Ltext0
	.value	0x1
	.byte	0x50
	.long	.LVL6-.Ltext0
	.long	.LVL7-.Ltext0
	.value	0x1
	.byte	0x50
	.long	.LVL7-.Ltext0
	.long	.LVL9-.Ltext0
	.value	0x1
	.byte	0x50
	.long	0x0
	.long	0x0
.LLST3:
	.long	.LVL10-.Ltext0
	.long	.LVL11-1-.Ltext0
	.value	0x2
	.byte	0x91
	.sleb128 0
	.long	0x0
	.long	0x0
.LLST4:
	.long	.LVL10-.Ltext0
	.long	.LVL11-1-.Ltext0
	.value	0x2
	.byte	0x91
	.sleb128 4
	.long	0x0
	.long	0x0
	.section	.debug_info
	.long	0xd4
	.value	0x3
	.long	.Ldebug_abbrev0
	.byte	0x4
	.uleb128 0x1
	.long	.LASF9
	.byte	0x1
	.long	.LASF10
	.long	.LASF11
	.long	.Ltext0
	.long	.Letext0
	.long	.Ldebug_line0
	.uleb128 0x2
	.byte	0x1
	.long	.LASF3
	.byte	0x1
	.byte	0x3
	.byte	0x1
	.long	0x7c
	.long	.LFB0
	.long	.LFE0
	.byte	0x1
	.byte	0x9c
	.long	0x7c
	.uleb128 0x3
	.string	"arg"
	.byte	0x1
	.byte	0x3
	.long	0x7c
	.byte	0x2
	.byte	0x91
	.sleb128 0
	.uleb128 0x4
	.long	.LASF0
	.byte	0x1
	.byte	0x5
	.long	0x7c
	.long	.LLST0
	.uleb128 0x4
	.long	.LASF1
	.byte	0x1
	.byte	0x6
	.long	0x7c
	.long	.LLST1
	.uleb128 0x4
	.long	.LASF2
	.byte	0x1
	.byte	0x7
	.long	0x7c
	.long	.LLST2
	.byte	0x0
	.uleb128 0x5
	.byte	0x4
	.byte	0x7
	.long	.LASF7
	.uleb128 0x2
	.byte	0x1
	.long	.LASF4
	.byte	0x1
	.byte	0x16
	.byte	0x1
	.long	0xbd
	.long	.LFB1
	.long	.LFE1
	.byte	0x1
	.byte	0x9c
	.long	0xbd
	.uleb128 0x6
	.long	.LASF5
	.byte	0x1
	.byte	0x16
	.long	0xbd
	.long	.LLST3
	.uleb128 0x6
	.long	.LASF6
	.byte	0x1
	.byte	0x16
	.long	0xc4
	.long	.LLST4
	.byte	0x0
	.uleb128 0x7
	.byte	0x4
	.byte	0x5
	.string	"int"
	.uleb128 0x8
	.byte	0x4
	.long	0xca
	.uleb128 0x8
	.byte	0x4
	.long	0xd0
	.uleb128 0x5
	.byte	0x1
	.byte	0x6
	.long	.LASF8
	.byte	0x0
	.section	.debug_abbrev
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
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x10
	.uleb128 0x6
	.byte	0x0
	.byte	0x0
	.uleb128 0x2
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0xc
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0xc
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x40
	.uleb128 0xa
	.uleb128 0x1
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.uleb128 0x3
	.uleb128 0x5
	.byte	0x0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0xa
	.byte	0x0
	.byte	0x0
	.uleb128 0x4
	.uleb128 0x34
	.byte	0x0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x6
	.byte	0x0
	.byte	0x0
	.uleb128 0x5
	.uleb128 0x24
	.byte	0x0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.byte	0x0
	.byte	0x0
	.uleb128 0x6
	.uleb128 0x5
	.byte	0x0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x6
	.byte	0x0
	.byte	0x0
	.uleb128 0x7
	.uleb128 0x24
	.byte	0x0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0x8
	.byte	0x0
	.byte	0x0
	.uleb128 0x8
	.uleb128 0xf
	.byte	0x0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.section	.debug_pubnames,"",@progbits
	.long	0x20
	.value	0x2
	.long	.Ldebug_info0
	.long	0xd8
	.long	0x25
	.string	"func"
	.long	0x83
	.string	"main"
	.long	0x0
	.section	.debug_aranges,"",@progbits
	.long	0x1c
	.value	0x2
	.long	.Ldebug_info0
	.byte	0x4
	.byte	0x0
	.value	0x0
	.value	0x0
	.long	.Ltext0
	.long	.Letext0-.Ltext0
	.long	0x0
	.long	0x0
	.section	.debug_str,"MS",@progbits,1
.LASF7:
	.string	"unsigned int"
.LASF3:
	.string	"func"
.LASF0:
	.string	"uses_stack_op"
.LASF5:
	.string	"argc"
.LASF10:
	.string	"valop.c"
.LASF2:
	.string	"result"
.LASF8:
	.string	"char"
.LASF9:
	.string	"GNU C 4.5.0 20090818 (experimental) [var-tracking-assignments-branch revision 150964]"
.LASF4:
	.string	"main"
.LASF11:
	.string	"/tmp"
.LASF1:
	.string	"uses_lit_op"
.LASF6:
	.string	"argv"
	.ident	"GCC: (GNU) 4.5.0 20090818 (experimental) [var-tracking-assignments-branch revision 150964]"
	.section	.note.GNU-stack,"",@progbits
