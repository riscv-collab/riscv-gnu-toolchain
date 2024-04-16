# Support for TLS LE symbols with addend
	.text
	.globl	a
	.section	.tdata,"awT",@progbits
	.align	2
	.type	a, @object
	.size	a, 4
a:
	.word	123

	.text
	.global _start
_start:
	lu12i.w $r4,%le_hi20(a + 0x8)
	ori	$r5,$r4,%le_lo12(a + 0x8)
	lu12i.w $r4,%le_hi20_r(a + 0x8)
	addi.d	$r5,$r4,%le_lo12_r(a + 0x8)
	jr $ra
