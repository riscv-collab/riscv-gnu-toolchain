        jeq %r1,%r2,2147483647  /* Overflows. */
        jlt %r3,%r4,2147483648  /* Overflows. */
        jge %r5,10,-2147483648  /* Overflows. */
        ja -2147483649          /* Overflows. */
