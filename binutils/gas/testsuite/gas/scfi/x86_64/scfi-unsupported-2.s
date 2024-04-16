	.text
	.globl   foo
	.type    foo, @function
foo:
	addq    %rdx, %rax
# Stack manipulation without switching to RBP
# based tracking is not supported for SCFI.
	addq    %rax, %rsp
	push    %rdi
	leave
	ret
.LFE0:
	.size   foo, .-foo
