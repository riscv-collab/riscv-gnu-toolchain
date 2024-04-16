# mach: bpf
# as: --EB
# ld: --EB
# sim: -E big
# output: pass\nexit 0 (0x0)\n
/* endbe.s
   Tests for BPF endianness-conversion instructions in simulator
   running in BIG ENDIAN

   Both 'be' and 'le' ISAs have both endbe and endle instructions.  */

    .include "testutils.inc"

    .text
    .global main
    .type main, @function
main:
    lddw        %r1, 0x12345678deadbeef
    endle       %r1, 64
    fail_ne     %r1, 0xefbeadde78563412
    endle       %r1, 64
    fail_ne     %r1, 0x12345678deadbeef

    /* `bitsize` < 64 will truncate  */
    endle       %r1, 32
    fail_ne     %r1, 0xefbeadde
    endle       %r1, 32
    fail_ne     %r1, 0xdeadbeef

    endle       %r1, 16
    fail_ne     %r1, 0xefbe
    endle       %r1, 16
    fail_ne     %r1, 0xbeef

    /* endbe on be should be noop (except truncate)  */
    lddw        %r1, 0x12345678deadbeef
    endbe       %r1, 64
    fail_ne     %r1, 0x12345678deadbeef

    endbe       %r1, 32
    fail_ne     %r1, 0xdeadbeef

    endbe       %r1, 16
    fail_ne     %r1, 0xbeef

    pass
