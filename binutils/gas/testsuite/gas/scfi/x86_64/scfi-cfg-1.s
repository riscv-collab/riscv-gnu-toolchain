# Testcase with one dominator bb and two exit bbs
# Something like for: return ferror (f) || fclose (f) != 0;
	.text
	.section .rodata.str1.1,"aMS",@progbits,1
.LC0:
	.string "w"
.LC1:
	.string "conftest.out"
	.section .text.startup,"ax",@progbits
	.p2align 4
	.globl  main
	.type   main, @function
main:
.LFB11:
	.cfi_startproc
	pushq   %rbx
	.cfi_def_cfa_offset 16
	.cfi_offset %rbx, -16
	movl   $.LC0, %esi
	movl   $.LC1, %edi
	call   fopen
	movq   %rax, %rdi
	movq   %rax, %rbx
	call   ferror
	movl   %eax, %edx
	movl   $1, %eax
	testl  %edx, %edx
	je     .L7
	.cfi_remember_state
	popq   %rbx
	.cfi_restore %rbx
	.cfi_def_cfa_offset 8
	ret
.L7:
	.cfi_restore_state
	movq    %rbx, %rdi
	call    fclose
	popq    %rbx
	.cfi_restore %rbx
	.cfi_def_cfa_offset 8
	testl   %eax, %eax
	setne   %al
	movzbl  %al, %eax
	ret
	.cfi_endproc
.LFE11:
	.size   main, .-main
