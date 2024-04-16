	.section        .rodata
	.type   simd_cmp_op, @object
	.size   simd_cmp_op, 8
simd_cmp_op:
	.long   2
	.zero   4

# Testcase for add instruction.
# add reg, reg instruction
	.text
	.globl  foo
	.type   foo, @function
foo:
	.cfi_startproc
	pushq   %rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq    %rsp, %rbp
	.cfi_def_cfa_register %rbp
	pushq   %r12
	.cfi_offset %r12, -24
	mov     %rsp, %r12
# Stack manipulation is permitted if the base register for
# tracking CFA has been changed to FP.
	addq    %rdx, %rsp
	addq    %rsp, %rax
# Some add instructions may access the stack indirectly.  Such
# accesses do not make REG_FP untraceable.
	addl    %eax, -84(%rbp)
# Other kind of add instructions should not error out in the
# x86_64 -> ginsn translator
	addq    $simd_cmp_op+8, %rdx
	addq    $1, symbol
	addl    %edx, -32(%rsp)
	addl    $1, fb_low_counter(,%rbx,4)
	mov     %r12, %rsp
# Popping a callee-saved register.
# RSP must be traceable.
	pop     %r12
	.cfi_restore %r12
	leave
	.cfi_def_cfa_register %rsp
	.cfi_restore %rbp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE0:
	.size   foo, .-foo
