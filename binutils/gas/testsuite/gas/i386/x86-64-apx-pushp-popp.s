# Check 64bit APX_F pushp popp instructions

       .text
 _start:
	pushp %rax
	pushp %r31
	popp  %rax
	popp  %r31
