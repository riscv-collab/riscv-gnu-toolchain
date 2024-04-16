# mach: bpf
# output: pass\nexit 0 (0x0)\n
/* xadd.s
   Tests for BPF atomic exchange-and-add instructions in simulator

   The xadd instructions (XADDW, XADDDW) operate on a memory location
   specified in $dst + offset16, atomically adding the value in $src.

   In the simulator, there isn't anything else happening. The atomic
   instructions are identical to a non-atomic load/add/store.  */

    .include "testutils.inc"

    .text
    .global main
    .type main, @function
main:
    mov         %r1, 0x1000
    mov         %r2, 5

    /* basic xadd w */
    stw         [%r1+0], 10
    xaddw       [%r1+0], %r2
    ldxw        %r3, [%r1+0]
    fail_ne     %r3, 15

    /* basic xadd dw */
    stdw        [%r1+8], 42
    xadddw      [%r1+8], %r2
    ldxdw       %r3, [%r1+8]
    fail_ne     %r3, 47

    /* xadd w negative value */
    mov         %r4, -1
    xaddw       [%r1+0], %r4
    ldxw        %r3, [%r1+0]
    fail_ne     %r3, 14

    /* xadd dw negative val */
    xadddw      [%r1+8], %r4
    ldxdw       %r3, [%r1+8]
    fail_ne     %r3, 46

    pass
