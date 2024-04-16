        /* The following two instructions have constant targets that
           fix in the JA 16-bit signed displacement operand.  These
           are not relaxed.  */
1:      ja -32768
        ja 32767
        /* The following instruction refers to a defined symbol that
           is on reach, so it should not be relaxed.  */
        ja 1b
        /* The following instruction has an undefined symbol as a
           target.  It is not to be relaxed.  */
        ja undefined + 10
        /* The following instructions refer to a defined symbol that
           is not on reach.  They shall be relaxed to a JAL.  */
        ja tail
        tail = .text + 262160
        ja tail
