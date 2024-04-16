/* This test case mainly carries out ld compatibility test.
   This test case is the old tls le instruction sequence,
   which will be linked with tls-relax-compatible-check-new.s.
   If the link is normal, it indicates that there is no
   compatibility problem. */

	.text
	.globl	older
	.section	.tbss,"awT",@nobits
	.align	2
	.type	older, @object
	.size	older, 4
older:
	.space	4
	.text
	.align	2
	.globl	old
	.type	old, @function
old:
.LFB0 = .
	addi.d	$r3,$r3,-16
	st.d	$r22,$r3,8
	addi.d	$r22,$r3,16
	lu12i.w	$r12,%le_hi20(older)
	ori	$r12,$r12,%le_lo12(older)
	add.d	$r12,$r12,$r2
	addi.w	$r13,$r0,1			# 0x1
	stptr.w	$r13,$r12,0
	nop
	or	$r4,$r12,$r0
	ld.d	$r22,$r3,8
	addi.d	$r3,$r3,16
	jr	$r1
