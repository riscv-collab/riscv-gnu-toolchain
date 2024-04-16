# mach: bpf
# output: pass\nexit 0 (0x0)\n
/* jmp.s
   Tests for eBPF JMP instructions in simulator  */

    .include "testutils.inc"

    .text
    .global main
    .type main, @function
main:
    mov         %r1, 5
    mov         %r2, 2
    mov         %r3, 7
    mov         %r4, -1

    /* ja - jump absolute (unconditional)  */
    ja          2f
1:  fail

2:  /* jeq - jump eq  */
    jeq         %r1, 4, 1b      /* no  */
    jeq         %r1, %r2, 1b    /* no  */
    jeq         %r1, 5, 2f      /* yes */
    fail
2:  jeq         %r1, %r1, 2f    /* yes  */
    fail

2:  /* jgt - jump (unsigned) greater-than  */
    jgt         %r1, 6, 1b      /* no  */
    jgt         %r1, -5, 1b     /* no - unsigned  */
    jgt         %r1, %r4, 1b    /* no - unsigned  */
    jgt         %r1, 4, 2f      /* yes  */
    fail
2:  jgt         %r1, %r2, 2f    /* yes  */
    fail

2:  /* jge - jump (unsigned) greater-than-or-equal-to  */
    jge         %r1, 6, 1b      /* no  */
    jge         %r1, 5, 2f      /* yes  */
    fail
2:  jge         %r1, %r3, 1b    /* no  */
    jge         %r1, -5, 1b     /* no - unsigned  */
    jge         %r1, %r2, 2f    /* yes  */
    fail

2:  /* jlt - jump (unsigned) less-than  */
    jlt         %r1, 5, 1b      /* no  */
    jlt         %r1, %r2, 1b    /* no  */
    jlt         %r4, %r1, 1b    /* no - unsigned  */
    jlt         %r1, 6, 2f      /* yes  */
    fail
2:
    jlt         %r1, %r3, 2f    /* yes  */
    fail

2:  /* jle - jump (unsigned) less-than-or-equal-to  */
    jle         %r1, 4, 1b      /* no  */
    jle         %r1, %r2, 1b    /* no  */
    jle         %r4, %r1, 1b    /* no  */
    jle         %r1, 5, 2f      /* yes  */
    fail
2:  jle         %r1, %r1, 2f    /* yes  */
    fail

2:  /* jset - jump "test" (AND)  */
    jset        %r1, 2, 1b      /* no (5 & 2 = 0)  */
    jset        %r1, %r2, 1b    /* no (same)  */
    jset        %r1, 4, 2f      /* yes (5 & 4 != 0)  */
    fail

2:  /* jne  - jump not-equal-to  */
    jne         %r1, 5, 1b      /* no  */
    jne         %r1, %r1, 1b    /* no  */
    jne         %r1, 6, 2f      /* yes  */
    fail
2:  jne         %r1, %r4, 2f    /* yes  */
    fail

2:  /* jsgt - jump (signed) greater-than  */
    jsgt        %r1, %r3, 1b    /* no  */
    jsgt        %r1, %r1, 1b    /* no  */
    jsgt        %r1, 5, 1b      /* no  */
    jsgt        %r1, -4, 2f     /* yes  */
    fail
2:  jsgt        %r1, %r4, 2f    /* yes  */
    fail

2:  /* jsge - jump (signed) greater-than-or-equal-to  */
    jsge        %r1, %r3, 1b    /* no  */
    jsge        %r1, %r1, 2f    /* yes  */
    fail
2:  jsge        %r1, 7, 1b      /* no  */
    jsge        %r1, -4, 2f     /* yes */
    fail
2:  jsge        %r1, %r4, 2f    /* yes  */
    fail

2:  /* jslt - jump (signed) less-than  */
    jslt        %r1, 5, 1b      /* no  */
    jslt        %r1, %r2, 1b    /* no  */
    jslt        %r4, %r1, 2f    /* yes  */
    fail
2:  jslt        %r1, 6, 2f      /* yes  */
    fail
2:  jslt        %r1, %r3, 2f    /* yes  */
    fail

2:  /* jsle - jump (signed) less-than-or-equal-to  */
    jsle         %r1, 4, 1b      /* no  */
    jsle         %r1, %r2, 1b    /* no  */
    jsle         %r4, %r1, 2f    /* yes  */
    fail
2:  jsle         %r1, 5, 2f      /* yes  */
    fail
2:  jsle         %r1, %r3, 2f    /* yes  */
    fail

2:
    pass
