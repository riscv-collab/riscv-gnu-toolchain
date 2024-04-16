	.text
_start:
	addq	foo@GOTTPOFF(%rip), %rax
	movq	foo@GOTTPOFF(%rip), %rax

	addq	foo@GOTTPOFF(%rip), %r16
	movq	foo@GOTTPOFF(%rip), %r20

	.intel_syntax noprefix

	addq	rax, QWORD PTR [rip + foo@GOTTPOFF]
	movq	rax, QWORD PTR [rip + foo@GOTTPOFF]

	addq	r16, QWORD PTR [rip + foo@GOTTPOFF]
	movq	r20, QWORD PTR [rip + foo@GOTTPOFF]
