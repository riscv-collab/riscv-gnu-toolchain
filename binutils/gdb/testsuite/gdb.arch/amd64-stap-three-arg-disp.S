	.file	"amd64-stap-three-arg-disp.c"
	.text
	.globl	main
	.type	main, @function
main:
.LFB0:
	.cfi_startproc
# BLOCK 2 seq:0
# PRED: ENTRY (fallthru)
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movl	%edi, -20(%rbp)
	movq	%rsi, -32(%rbp)
	movl	$10, -4(%rbp)
	movl	$0, %eax
#APP
# 8 "amd64-stap-three-arg-disp.c" 1
	990: nop
.pushsection .note.stapsdt,"?","note"
.balign 4
.4byte 992f-991f,994f-993f,3
991: .asciz "stapsdt"
992: .balign 4
993: .8byte 990b
.8byte _.stapsdt.base
.8byte 0
.asciz "test"
.asciz "three_arg"
.asciz "-4@-4(%rbp,%eax,0)"
994: .balign 4
.popsection

# 0 "" 2
# 8 "amd64-stap-three-arg-disp.c" 1
	.ifndef _.stapsdt.base
.pushsection .stapsdt.base,"aG","progbits",.stapsdt.base,comdat
.weak _.stapsdt.base
.hidden _.stapsdt.base
_.stapsdt.base: .space 1
.size _.stapsdt.base,1
.popsection
.endif

# 0 "" 2
#NO_APP
	popq	%rbp
	.cfi_def_cfa 7, 8
# SUCC: EXIT [100.0%] 
	ret
	.cfi_endproc
.LFE0:
	.size	main, .-main
	.ident	"GCC: (GNU) 4.7.2 20120921 (Red Hat 4.7.2-2)"
	.section	.note.GNU-stack,"",@progbits
