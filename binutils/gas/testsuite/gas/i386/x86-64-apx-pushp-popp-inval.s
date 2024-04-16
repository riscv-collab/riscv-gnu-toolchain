# Check bytecode of APX_F pushp popp instructions with illegal instructions.

	.text
	pushp %eax
	popp  %eax
	pushp (%rax)
	popp  (%rax)
