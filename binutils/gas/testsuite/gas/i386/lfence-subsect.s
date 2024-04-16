	.text
_start:
	rep

	.subsection 2
aux1:
	nop

	.previous
	call *%eax
	rep

	.pushsection .text, 2
aux2:
	nop

	.popsection
	ret
	.p2align 2, 0xcc
