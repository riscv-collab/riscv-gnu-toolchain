# Testcase with an indirect jump
# Indirect jumps, when present, make the list of ginsn an invalid
# candidate for CFG creation.  Hence, no SCFI can be generated either.
#
# The testcase is rather long to showcase a simple concept.  The reason of
# such a long testcase is to discuss if it is important to deal with these
# patterns.  It may be possible to deal with this, if we allow some special
# directives for helping the assembler with the indirect jump (jump table).
	.text
	.globl   foo
	.type   foo, @function
foo:
	pushq   %rbp
	movq    %rsp, %rbp
	movl    %edi, -4(%rbp)
	cmpl    $5, -4(%rbp)
	ja      .L2
	movl    -4(%rbp), %eax
	movq    .L4(,%rax,8), %rax
	jmp     *%rax
.L4:
	.quad   .L9
	.quad   .L8
	.quad   .L7
	.quad   .L6
	.quad   .L5
	.quad   .L3
.L9:
	movl    $43, %eax
	jmp     .L1
.L8:
	movl    $42, %eax
	jmp     .L1
.L7:
	movl    $45, %eax
	jmp     .L1
.L6:
	movl    $47, %eax
	jmp     .L1
.L5:
	movl    $37, %eax
	jmp     .L1
.L3:
	movl    $63, %eax
	jmp     .L1
.L2:
.L1:
	popq    %rbp
	ret
	.cfi_endproc
.LFE0:
	.size   foo, .-foo
