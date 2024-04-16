# mach: bpf
# output: pass\nexit 0 (0x0)\n
/* mov.s
   Tests for mov and mov32 instructions  */

    .include "testutils.inc"

    .text
    .global main
    .type main, @function
main:
    /* some basic sanity checks  */
    mov32       %r1, 5
    fail_ne     %r1, 5

    mov32       %r2, %r1
    fail_ne     %r2, 5

    mov         %r2, %r1
    fail_ne     %r2, 5

    mov         %r1, -666
    fail_ne     %r1, -666

    /* should NOT sign extend  */
    mov32       %r1, -1
    fail_ne     %r1, 0x00000000ffffffff

    /* should sign extend  */
    mov         %r2, -1
    fail_ne     %r2, 0xffffffffffffffff

    mov         %r3, -2147483648 /* 0x80000000 */

    /* should NOT sign extend */
    mov32       %r4, %r3
    fail_ne     %r4, 0x0000000080000000

    /* should sign extend  */
    mov         %r5, %r3
    fail_ne     %r5, 0xffffffff80000000

    mov32       %r1, -2147483648
    mov32       %r1, %r1
    fail_ne32   %r1, -2147483648

    /* casting shenanigans  */
    mov         %r1, %r1
    fail_ne     %r1, +2147483648
    mov32       %r2, -1
    mov         %r2, %r2
    fail_ne     %r2, +4294967295

    pass
