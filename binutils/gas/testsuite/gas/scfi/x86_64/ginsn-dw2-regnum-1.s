# Testcase for DWARF regnum ginsn API
	.text
	.globl  foo
	.type   foo, @function
foo:
	add    $8, %al
	add    $8, %cl
	add    $8, %dl
	add    $8, %ah
	add    $8, %ch
	add    $8, %dh
	add    $8, %bh

	add    $8, %axl
	add    $8, %spl

	add    $8, %ax
	add    $8, %r8w
	add    $8, %sp

	add    $8, %eax
	add    $8, %r8d
	add    $16384, %esp

	add    $8, %rbp

	lea    -0x2(%rip), %rax
	mov    %rax, 0x2(%eip)
	mov    -0x2(%eip), %rax

	ret
.LFE0:
	.size   foo, .-foo
