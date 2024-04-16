# Check 64bit APX-Push2Pop2 instructions

	.allow_index_reg
	.text
_start:
	push2 %rbx, %rax
	push2 %r17, %r8
	push2 %r9, %r31
	push2 %r31, %r24
	push2p %rbx, %rax
	push2p %r17, %r8
	push2p %r9, %r31
	push2p %r31, %r24
	pop2 %rax, %rbx
	pop2 %r8, %r17
	pop2 %r31, %r9
	pop2 %r24, %r31
	pop2p %rax, %rbx
	pop2p %r8, %r17
	pop2p %r31, %r9
	pop2p %r24, %r31

	.intel_syntax noprefix
	push2 rax, rbx
	push2 r8, r17
	push2 r31, r9
	push2 r24, r31
	push2p rax, rbx
	push2p r8, r17
	push2p r31, r9
	push2p r24, r31
	pop2 rbx, rax
	pop2 r17, r8
	pop2 r9, r31
	pop2 r31, r24
	pop2p rbx, rax
	pop2p r17, r8
	pop2p r9, r31
	pop2p r31, r24
