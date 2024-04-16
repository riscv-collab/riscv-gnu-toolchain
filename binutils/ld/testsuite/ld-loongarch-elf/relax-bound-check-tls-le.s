/* This test case mainly tests whether the address of the
   tls le symbol can be resolved normally when the offset
   of the symbol is greater than 0x800. (When the symbol
   offset is greater than 0x800, relax is not performed).  */

        .text
        .globl  count1
        .section        .tbss,"awT",@nobits
        .align  2
        .type   count1, @object
        .size   count1, 4
count1:
        .space  0x400
        .globl  count2
        .align  2
        .type   count2, @object
        .size   count2, 4
count2:
        .space  0x400
        .globl  count3
        .align  2
        .type   count3, @object
        .size   count3, 4
count3:
	.space  0x400
        .globl  count4
        .align  2
        .type   count4, @object
        .size   count4, 4
count4:
        .space  4
        .text
        .align  2
        .globl  main
        .type   main, @function
main:
	lu12i.w	$r12,%le_hi20_r(count1)
	add.d	$r12,$r12,$r2,%le_add_r(count1)
	addi.w	$r13,$r0,1
	st.w	$r13,$r12,%le_lo12_r(count1)
	lu12i.w $r12,%le_hi20_r(count2)
	add.d   $r12,$r12,$r2,%le_add_r(count2)
	addi.w  $r13,$r0,2
	st.w    $r13,$r12,%le_lo12_r(count2)
	lu12i.w $r12,%le_hi20(count3)
	add.d   $r12,$r12,$r2,%le_add_r(count3)
	addi.w  $r13,$r0,3
	st.w    $r13,$r12,%le_lo12_r(count3)
	lu12i.w $r12,%le_hi20(count4)
	add.d   $r12,$r12,$r2,%le_add_r(count4)
	addi.w  $r13,$r0,4
	st.w    $r13,$r12,%le_lo12_r(count4)

