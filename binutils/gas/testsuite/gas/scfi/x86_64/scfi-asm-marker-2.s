# A programmer may not bother to set the size of the 
# function symbols via an explicit .size directive.
	.globl  foo
	.type   foo, @function
foo:
	.cfi_startproc
	testl   %edi, %edi
	je      .L3
	movl    b(%rip), %eax
	ret
	.cfi_endproc
