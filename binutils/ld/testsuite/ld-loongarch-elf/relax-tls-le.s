/* This test case mainly tests whether the tls le variable
   address acquisition can be relax normally.

   before relax:                                  after relax:

   lu12i.w $r12,%le_hi20_r(sym)           ====>    (instruction deleted).
   add.d   $r12,$r12,$r2,%le_add_r(sym)   ====>    (instruction deleted).
   st.w    $r13,$r12,%le_lo12_r(sym)      ====>    st.w    $r13,$r2,%le_lo12_r(sym).  */

	.text
	.globl	a
	.section	.tbss,"awT",@nobits
	.align	2
	.type	a, @object
	.size	a, 4
a:
	.space	4
	.text
	.align	2
	.globl	main
	.type	main, @function
main:
	lu12i.w	$r12,%le_hi20_r(a)
	add.d	$r12,$r12,$r2,%le_add_r(a)
	addi.w	$r13,$r0,1			# 0x1
	st.w	$r13,$r12,%le_lo12_r(a)
