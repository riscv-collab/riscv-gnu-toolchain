# Certain APX instructions are not supported currently
# Also, hand-crafting instructions using .insn directive is not supported.
	.text
	.globl   foo
	.type   foo, @function
foo:
	pop2p  %r12, %rax
	pop2   %r12, %rax
	push2  %r12, %rax
	push2p %rax, %r17
	adc %rsp, %r17, %rsp
	# test $0x4,%ecx
	.insn 0xf7/1, $4{:u32}, %ecx
	ret
.LFE0:
	.size   foo, .-foo
