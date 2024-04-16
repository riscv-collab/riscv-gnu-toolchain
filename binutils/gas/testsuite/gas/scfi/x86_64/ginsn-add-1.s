## Testcase with a variety of add.
## Some add insns valid in 64-bit mode may not be processed for SCFI.
	.text
	.globl foo
	.type foo, @function
foo:
	push %rsp
	movq %rsp, %rbp

	addq %rax, symbol
	add symbol, %eax
	add (%eax), %esp
	add %esp, (,%eax)
	add (,%eax), %esp

	addq %rax, %rbx
	add %eax, %ebx

	adc $8, %rsp

	addq $1, -16(%rbp)

	{load} addq %rax, %rbx

	ret
.LFE0:
	.size foo, .-foo
