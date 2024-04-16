# mach: bpf
# output: pass\nexit 0 (0x0)\n
/* alu32.s
   Tests for ALU(32) BPF instructions in simulator  */

    .include "testutils.inc"

    .text
    .global main
    .type main, @function
main:
    mov32       %r1, 10         /* r1 = 10  */
    mov32       %r2, -5         /* r2 = -5  */

    /* add  */
    add32       %r1, 1          /* r1 += 1  (r1 = 11)  */
    add32       %r2, -1         /* r2 += -1 (r2 = -6)  */
    add32       %r1, %r2        /* r1 += r2 (r1 = 11 + -6 = 5)  */
    fail_ne32   %r1, 5

    /* sub  */
    sub32       %r1, 5          /* r1 -= 5 (r1 = 0)  */
    sub32       %r1, -5         /* r1 -= -5 (r1 = 5)  */
    sub32       %r1, %r2        /* r1 -= r2 (r1 = 5 - -6 = 11)  */
    fail_ne32   %r1, 11

    /* mul  */
    mul32       %r1, 2          /* r1 *= 2  (r1 = 22)  */
    mul32       %r1, -2         /* r1 *= -2 (r1 = -44)  */
    mul32       %r1, %r2        /* r1 *= r2 (r1 = -44 * -6 = 264)  */
    fail_ne32   %r1, 264

    /* div  */
    div32       %r1, 6
    mov32       %r2, 11
    div32       %r1, %r2
    fail_ne32   %r1, 4

    /* div is unsigned  */
    mov32       %r1, -8         /* 0xfffffff8  */
    div32       %r1, 2
    fail_ne32   %r1, 0x7ffffffc /* sign bits are not preserved  */

    /* and (bitwise)  */
    mov32       %r1, 0xb        /* r1  = (0xb = 0b1011)  */
    mov32       %r2, 0x5        /* r2  = (0x5 = 0b0101)  */
    and32       %r1, 0xa        /* r1 &= (0xa = 0b1010) = (0b1010 = 0xa)  */
    fail_ne32   %r1, 0xa
    and32       %r1, %r2        /* r1 &= r2 = 0x0  */
    fail_ne32   %r1, 0x0

    /* or (bitwise)  */
    or32        %r1, 0xb
    or32        %r1, %r2
    fail_ne32   %r1, 0xf

    /* lsh (left shift)  */
    lsh32       %r1, 4          /* r1 <<= 4 (r1 = 0xf0)  */
    mov32       %r2, 24         /* r2 = 24  */
    lsh32       %r1, %r2
    fail_ne32   %r1, -268435456 /* 0xf0000000  */

    /* rsh (right logical shift)  */
    rsh32       %r1, 2
    rsh32       %r1, %r2
    fail_ne32   %r1, 0x3c       /* (0xf000 0000 >> 26)  */

    /* arsh (right arithmetic shift)  */
    arsh32      %r1, 1
    or32        %r1, -2147483648 /* 0x80000000  */
    mov32       %r2, 3
    arsh32      %r1, %r2
    fail_ne     %r1, 0x00000000F0000003
                                /* Note: make sure r1 is NOT sign-extended */
                                /* i.e. upper-32 bits should be untouched.  */

    /* mod  */
    mov32       %r1, 1025
    mod32       %r1, 16
    fail_ne32   %r1, 1

    /* mod is unsigned  */
    mov32       %r1, 1025
    mod32       %r1, -16        /* when unsigned, much larger than 1025  */
    fail_ne32   %r1, 1025

    mov32       %r1, -25        /* when unsigned, a large positive which is */
    mov32       %r2, 5          /* ... not evenly divisible by 5  */
    mod32       %r1, %r2
    fail_ne32   %r1, 1

    /* xor  */
    xor32       %r1, %r2
    fail_ne32   %r1, 4
    xor32       %r1, -268435441 /* 0xF000000F  */
    fail_ne     %r1, 0xF000000B /* Note: check for (bad) sign-extend  */
    xor32       %r1, %r1
    fail_ne     %r1, 0

    /* neg  */
    mov32       %r1, -1
    mov32       %r2, 0x7fffffff
    neg32       %r1
    neg32       %r2
    fail_ne32   %r1, 1
    fail_ne     %r2, 0x80000001 /* Note: check for (bad) sign-extend  */
    neg32       %r2
    fail_ne32   %r2, 0x7fffffff

    pass
