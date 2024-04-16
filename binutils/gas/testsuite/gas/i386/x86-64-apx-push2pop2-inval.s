# Check illegal APX-Push2Pop2 instructions

	.allow_index_reg
	.text
_start:
	push2  %ax, %bx
	push2  %eax, %ebx
	push2  %rsp, %r17
	push2  %r17, %rsp
	push2p %eax, %ebx
	push2p %rsp, %r17
	pop2   %ax, %bx
	pop2   %rax, %rsp
	pop2   %rsp, %rax
	pop2   %r12, %r12
	pop2p  %rax, %rsp
	pop2p  %r12, %r12
