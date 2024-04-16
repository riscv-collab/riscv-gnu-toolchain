/* This test case mainly tests whether the original
   tls le assembly instruction can be linked normally
   after tls le relax is added to the current ld.  */

        .text
        .globl  aa
        .section        .tbss,"awT",@nobits
        .align  2
        .type   aa, @object
        .size   aa, 4
aa:
        .space  4
        .text
        .align  2
        .globl  main
        .type   main, @function
main:
        lu12i.w $r12,%le_hi20(aa)
        ori     $r12,$r12,%le_lo12(aa)
        add.d   $r12,$r12,$r2
        addi.w  $r13,$r0,2                      # 0x2
        stptr.w $r13,$r12,0

