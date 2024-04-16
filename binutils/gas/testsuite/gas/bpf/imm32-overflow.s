        add %r1, 2147483647
        or %r2, 4294967296         /* This overflows.  */
        xor %r3, 4294967295
        sub %r4, 4294967296        /* This overflows.  */
