        /* The following two instructions have constant targets that
           fix in the jump 16-bit signed displacement operand.  */
1:      jeq %r1, %r2, -32768
        jlt %r1, %r2, 32767
        /* The following instruction refers to a defined symbol that
           is on reach, so it should not be relaxed.  */
        jle %r1, %r2, 1b
        /* The following instructions refer to a defined symbol that
           is not on reach.  They shall be relaxed.  */
        jeq %r1, %r2, tail
        tail = .text + 262160
        jgt %r1, %r2, tail
