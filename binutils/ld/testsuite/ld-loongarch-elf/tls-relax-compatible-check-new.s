/* This test case mainly carries out ld compatibility test.
   This test case is the new tls le instruction sequence,
   which will be linked with tls-relax-compatible-check-old.s.
   If the link is normal, it indicates that there is no
   compatibility problem.  */

	.text
	.globl	new
	.section	.tbss,"awT",@nobits
	.align	2
	.type	new, @object
	.size	new, 4
new:
	.space	4
	.text
	.align	2
	.globl	main
	.type	main, @function
main:
.LFB0 = .
	addi.d	$r3,$r3,-16
	st.d	$r1,$r3,8
	stptr.d	$r22,$r3,0
	addi.d	$r22,$r3,16
	bl	%plt(old)
	lu12i.w	$r12,%le_hi20_r(new)
	add.d	$r12,$r12,$r2,%le_add_r(new)
	addi.w	$r13,$r0,2			# 0x2
	st.w	$r13,$r12,%le_lo12_r(new)
	or	$r12,$r0,$r0
	or	$r4,$r12,$r0
	ld.d	$r1,$r3,8
	ldptr.d	$r22,$r3,0
	addi.d	$r3,$r3,16
	jr	$r1
