	.text
_start:
	movq	$foo@GOTPCREL, %rax
	movq	foo@GOTPCREL, %rax
	movq	foo@GOTPCREL(%rip), %rax
	movq	foo@GOTPCREL(%rcx), %rax

	call	*foo@GOTPCREL(%rip)
	call	*foo@GOTPCREL(%rax)
	jmp	*foo@GOTPCREL(%rip)
	jmp	*foo@GOTPCREL(%rcx)

	.intel_syntax noprefix

	mov	rax, offset foo@gotpcrel
	mov	rax, QWORD PTR [foo@GOTPCREL]
	mov	rax, QWORD PTR [rip + foo@GOTPCREL]
	mov	rax, QWORD PTR [rcx + foo@GOTPCREL]

	call	QWORD PTR [rip + foo@GOTPCREL]
	call	QWORD PTR [rax + foo@GOTPCREL]
	jmp	QWORD PTR [rip + foo@GOTPCREL]
	jmp	QWORD PTR [rcx + foo@GOTPCREL]

	.att_syntax prefix
	movq	$foo@GOTPCREL, %r16
	movq	foo@GOTPCREL, %r20
	movq	foo@GOTPCREL(%rip), %r22
	movq	foo@GOTPCREL(%r28), %r22

	call	*foo@GOTPCREL(%r16)
	jmp	*foo@GOTPCREL(%r28)

	.intel_syntax noprefix

	mov	r16, offset foo@gotpcrel
	mov	r20, QWORD PTR [foo@GOTPCREL]
	mov	r22, QWORD PTR [rip + foo@GOTPCREL]
	mov	r22, QWORD PTR [r28 + foo@GOTPCREL]

	call	QWORD PTR [r16 + foo@GOTPCREL]
	jmp	QWORD PTR [r28 + foo@GOTPCREL]
