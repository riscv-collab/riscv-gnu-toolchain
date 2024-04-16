        ldxh %r2, [%r1 + 65535]
        ldxw %r2, [%r1 + 65536]  /* This overflows.  */
        stxw [%r2 - 32768], %r1
        stxdw [%r2 - 32769], %r1  /* This overflows.  */
