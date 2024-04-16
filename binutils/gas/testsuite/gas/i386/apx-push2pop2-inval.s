# Check 32bit APX-PUSH2/POP2 instructions

	.allow_index_reg
	.text
_start:
	push2 %rax, %rbx
	push2p %rax, %rbx
	pop2 %rax, %rbx
	pop2p %rax, %rbx
