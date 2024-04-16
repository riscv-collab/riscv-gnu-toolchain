        ja 32767
        jeq %r1,%r2,65536       /* Overflows */
        jlt %r3,%r4,-32768
        jge %r5,10,-32769       /* Overflows */
