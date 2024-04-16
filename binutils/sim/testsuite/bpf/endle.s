# mach: bpf
# output: pass\nexit 0 (0x0)\n
/* endle.s
   Tests for BPF endianness-conversion instructions in simulator
   running in LITTLE ENDIAN

   Both 'be' and 'le' ISAs have both endbe and endle instructions.  */

    .include "testutils.inc"

    .text
    .global main
    .type main, @function
main:
    lddw        %r1, 0x12345678deadbeef
    endbe       %r1, 64
    fail_ne     %r1, 0xefbeadde78563412
    endbe       %r1, 64
    fail_ne     %r1, 0x12345678deadbeef

    /* `bitsize` < 64 will truncate  */
    endbe       %r1, 32
    fail_ne     %r1, 0xefbeadde
    endbe       %r1, 32
    fail_ne     %r1, 0xdeadbeef

    endbe       %r1, 16
    fail_ne     %r1, 0xefbe
    endbe       %r1, 16
    fail_ne     %r1, 0xbeef

    /* endle on le should be noop (except truncate)  */
    lddw        %r1, 0x12345678deadbeef
    endle       %r1, 64
    fail_ne     %r1, 0x12345678deadbeef

    endle       %r1, 32
    fail_ne     %r1, 0xdeadbeef

    endle       %r1, 16
    fail_ne     %r1, 0xbeef

    pass
