	.text
_start:
	rep

	.section .text2, "ax"
aux1:
	nop

	.text
	call *%eax
	rep

	.section .text2, "ax"
aux2:
	nop

	.text
	ret
	.p2align 2, 0xcc
