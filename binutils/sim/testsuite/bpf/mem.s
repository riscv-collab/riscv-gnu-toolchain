# mach: bpf
# output: pass\nexit 0 (0x0)\n
/* mem.s
   Tests for BPF memory (ldx, stx, ..) instructions in simulator  */

    .include "testutils.inc"

    .text
    .global main
    .type main, @function
main:
    lddw        %r1, 0x1234deadbeef5678
    mov         %r2, 0x1000

    /* basic store/load check  */
    stxb        [%r2+0], %r1
    stxh        [%r2+2], %r1
    stxw        [%r2+4], %r1
    stxdw       [%r2+8], %r1

    stb         [%r2+16], 0x5a
    sth         [%r2+18], 0xcafe
    stw         [%r2+20], -1091568946 /* 0xbeefface  */
    stdw        [%r2+24], 0x7eadbeef

    ldxb        %r1, [%r2+16]
    fail_ne     %r1, 0x5a
    ldxh        %r1, [%r2+18]
    fail_ne     %r1, 0xffffffffffffcafe
    ldxw        %r1, [%r2+20]
    fail_ne     %r1, 0xffffffffbeefface
    ldxdw       %r1, [%r2+24]
    fail_ne     %r1, 0x7eadbeef

    ldxb        %r3, [%r2+0]
    fail_ne     %r3, 0x78
    ldxh        %r3, [%r2+2]
    fail_ne     %r3, 0x5678
    ldxw        %r3, [%r2+4]
    fail_ne     %r3, 0xffffffffbeef5678
    ldxdw       %r3, [%r2+8]
    fail_ne     %r3, 0x1234deadbeef5678

    ldxw        %r4, [%r2+10]
    fail_ne     %r4, 0xffffffffdeadbeef

    /* negative offsets  */
    add         %r2, 16
    ldxh        %r5, [%r2+-14]
    fail_ne     %r5, 0x5678
    ldxw        %r5, [%r2+-12]
    fail_ne     %r5, 0xffffffffbeef5678
    ldxdw       %r5, [%r2+-8]
    fail_ne     %r5, 0x1234deadbeef5678

    pass
