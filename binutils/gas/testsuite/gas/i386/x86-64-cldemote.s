# Check 64bit CLDEMOTE instructions

	.text
_start:

	cldemote	(%rcx)
	cldemote	0x123(%rax,%r14,8)

	.intel_syntax noprefix
	cldemote	BYTE PTR [rcx]
	cldemote	BYTE PTR [rax+r14*8+0x1234]
