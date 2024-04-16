# Testcase where the input may have interleaved sections,
# possibly even text and data.
	.globl  main
	.type   main, @function
main:
.LFB1:
	.cfi_startproc
	pushq   %rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq    %rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq    $16, %rsp
	movl    $17, %esi
	movl    $5, %edi
	call    add
	.section        .rodata
	.align 16
	.type   __test_obj.0, @object
	.size   __test_obj.0, 24
__test_obj.0:
	.string "test_elf_objs_in_rodata"
.LC0:
	.string "the result is = %d\n"
	.text
	movl    %eax, -4(%rbp)
	movl    -4(%rbp), %eax
	movl    %eax, %esi
	movl    $.LC0, %edi
	movl    $0, %eax
	call    printf
	movl    $0, %eax
	leave
	.cfi_def_cfa_register %rsp
	.cfi_restore %rbp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
