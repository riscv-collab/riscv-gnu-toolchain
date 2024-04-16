# mach: bpf
# output: pass\nexit 0 (0x0)\n
/* jmp32.s
   Tests for eBPF JMP32 instructions in simulator  */

    .include "testutils.inc"

    .text
    .global main
    .type main, @function
main:
    mov32       %r1, 5
    mov32       %r2, 2
    mov32       %r3, 7
    mov32       %r4, -1

    /* ja - jump absolute (unconditional)  */
    ja          2f
1:  fail

2:  /* jeq - jump eq  */
    jeq32       %r1, 4, 1b      /* no */
    jeq32       %r1, %r2, 1b    /* no */
    jeq32       %r1, 5, 2f      /* yes */
    fail
2:  jeq32       %r1, %r1, 2f    /* yes */
    fail

2:  /* jgt - jump (unsigned) greater-than  */
    jgt32       %r1, 6, 1b      /* no */
    jgt32       %r1, -5, 1b     /* no - unsigned */
    jgt32       %r1, %r4, 1b    /* no - unsigned */
    jgt32       %r1, 4, 2f      /* yes */
    fail
2:  jgt32       %r1, %r2, 2f    /* yes */
    fail

2:  /* jge - jump (unsigned) greater-than-or-equal-to  */
    jge32       %r1, 6, 1b      /* no */
    jge32       %r1, 5, 2f      /* yes */
    fail
2:  jge32       %r1, %r3, 1b    /* no */
    jge32       %r1, -5, 1b     /* no - unsigned */
    jge32       %r1, %r2, 2f    /* yes */
    fail

2:  /* jlt - jump (unsigned) less-than */
    jlt32       %r1, 5, 1b      /* no */
    jlt32       %r1, %r2, 1b    /* no */
    jlt32       %r4, %r1, 1b    /* no - unsigned */
    jlt32       %r1, 6, 2f      /* yes */
    fail
2:
    jlt32       %r1, %r3, 2f    /* yes */
    fail

2:  /* jle - jump (unsigned) less-than-or-equal-to */
    jle32       %r1, 4, 1b      /* no */
    jle32       %r1, %r2, 1b    /* no */
    jle32       %r4, %r1, 1b    /* no */
    jle32       %r1, 5, 2f      /* yes */
    fail
2:  jle32       %r1, %r1, 2f    /* yes */
    fail

2:  /* jset - jump "test" (AND) */
    jset32      %r1, 2, 1b      /* no (5 & 2 = 0) */
    jset32      %r1, %r2, 1b    /* no (same) */
    jset32      %r1, 4, 2f      /* yes (5 & 4 != 0) */
    fail

2:  /* jne  - jump not-equal-to */
    jne32       %r1, 5, 1b      /* no */
    jne32       %r1, %r1, 1b    /* no */
    jne32       %r1, 6, 2f      /* yes */
    fail
2:  jne32       %r1, %r4, 2f    /* yes */
    fail

2:  /* jsgt - jump (signed) greater-than  */
    jsgt32      %r1, %r3, 1b    /* no */
    jsgt32      %r1, %r1, 1b    /* no */
    jsgt32      %r1, 5, 1b      /* no */
    jsgt32      %r1, -4, 2f     /* yes */
    fail
2:  jsgt32      %r1, %r4, 2f    /* yes */
    fail

2:  /* jsge - jump (signed) greater-than-or-equal-to */
    jsge32      %r1, %r3, 1b    /* no */
    jsge32      %r1, %r1, 2f    /* yes */
    fail
2:  jsge32      %r1, 7, 1b      /* no */
    jsge32      %r1, -4, 2f     /* yes */
    fail
2:  jsge32      %r1, %r4, 2f    /* yes */
    fail

2:  /* jslt - jump (signed) less-than */
    jslt32      %r1, 5, 1b      /* no */
    jslt32      %r1, %r2, 1b    /* no */
    jslt32      %r4, %r1, 2f    /* yes */
    fail
2:  jslt32      %r1, 6, 2f      /* yes */
    fail
2:  jslt32      %r1, %r3, 2f    /* yes */
    fail

2:  /* jsle - jump (signed) less-than-or-equal-to */
    jsle32       %r1, 4, 1b      /* no */
    jsle32       %r1, %r2, 1b    /* no */
    jsle32       %r4, %r1, 2f    /* yes */
    fail
2:  jsle32       %r1, 5, 2f      /* yes */
    fail
2:  jsle32       %r1, %r3, 2f    /* yes */
    fail

2:
    pass
