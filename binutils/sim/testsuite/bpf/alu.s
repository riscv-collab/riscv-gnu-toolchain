# mach: bpf
# output: pass\nexit 0 (0x0)\n
/* alu.s
   Tests for ALU64 BPF instructions in simulator.  */

    .include "testutils.inc"

    .text
    .global main
    .type main, @function
main:
    mov         %r1, 0
    mov         %r2, -1

    /* add  */
    add         %r1, 1
    add         %r2, -1
    add         %r1, %r2
    fail_ne     %r1, -1

    /* sub  */
    sub         %r1, %r1
    fail_ne     %r1, 0
    sub         %r1, 10
    sub         %r2, %r1
    fail_ne     %r2, 8

    /* mul  */
    mul         %r2, %r2        /* r2 = 64  */
    mul         %r2, 3          /* r2 = 192 */
    mov         %r1, -3
    mul         %r1, %r2        /* r1 = -576 */
    mul         %r2, 0
    fail_ne     %r1, -576
    fail_ne     %r2, 0
    mul         %r1, %r1
    mul         %r1, %r1
    fail_ne     %r1, 110075314176

    /* div  */
    div         %r2, %r1
    fail_ne     %r2, 0
    div         %r1, 10000
    fail_ne     %r1, 11007531
    div         %r1, %r1
    fail_ne     %r1, 1

    /* div is unsigned  */
    lddw        %r1, -8
    div         %r1, 2
    fail_ne     %r1, 0x7ffffffffffffffc /* sign bits NOT maintained - large pos.  */

    /* and  */
    lddw        %r1, 0xaaaaaaaa55555555
    and         %r1, 0x55aaaaaa         /* we still only have 32-bit imm.  */
    fail_ne     %r1, 0x0000000055000000
    lddw        %r2, 0x5555555a5aaaaaaa
    and         %r2, %r1
    fail_ne     %r2, 0x0000000050000000

    /* or  */
    or          %r2, -559038737         /* 0xdeadbeef  */
    fail_ne     %r2, 0xffffffffdeadbeef /* 0xdeadbeef gets sign extended  */
    lddw        %r1, 0xdead00000000beef
    lddw        %r2, 0x0000123456780000
    or          %r1, %r2
    fail_ne     %r1, 0xdead12345678beef

    /* lsh  */
    mov         %r1, -559038737         /* 0xdeadbeef  */
    lsh         %r1, 11
    fail_ne     %r1, 0xfffffef56df77800 /* because deadbeef gets sign ext.  */
    mov         %r2, 21
    lsh         %r1, %r2
    fail_ne     %r1, 0xdeadbeef00000000

    /* rsh  */
    rsh         %r1, 11
    fail_ne     %r1, 0x001bd5b7dde00000 /* 0xdeadbeef 00000000 >> 0xb  */
    rsh         %r1, %r2
    fail_ne     %r1, 0x00000000deadbeef

    /* arsh  */
    arsh        %r1, 8
    fail_ne     %r1, 0x0000000000deadbe
    lsh         %r1, 40                 /* r1 = 0xdead be00 0000 0000  */
    arsh        %r1, %r2                /* r1 arsh (r2 == 21)  */
    fail_ne     %r1, 0xfffffef56df00000

    /* mod  */
    mov         %r1, 1025
    mod         %r1, 16
    fail_ne     %r1, 1

    /* mod is unsigned  */
    mov         %r1, 1025
    mod         %r1, -16        /* mod unsigned -> will treat as large positive  */
    fail_ne     %r1, 1025

    mov         %r1, -25        /* -25 is 0xff..ffe7  */
    mov         %r2, 5          /* ... which when unsigned is a large positive  */
    mod         %r1, %r2        /* ... which is not evenly divisible by 5  */
    fail_ne     %r1, 1

    /* xor  */
    mov         %r1, 0
    xor         %r1, %r2
    fail_ne     %r1, 5
    xor         %r1, 0x7eadbeef
    fail_ne     %r1, 0x7eadbeea
    xor         %r1, %r1
    fail_ne     %r1, 0

    /* neg  */
    neg         %r2
    fail_ne     %r2, -5
    mov         %r1, -1025
    neg         %r1
    fail_ne     %r1, 1025

    pass
