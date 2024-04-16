	.text

_start:
	rex.w add (%rax,%rax), %rax
	rex.r add (%rax,%rax), %r8
	rex.b add (%r8,%rax), %eax
	rex.x add (%rax,%r8), %eax

	rex mov %al, %ch
	rex mov %ah, %cl

	.insn rex 0x88, %al, %ch
	.insn rex 0x88, %ah, %cl
