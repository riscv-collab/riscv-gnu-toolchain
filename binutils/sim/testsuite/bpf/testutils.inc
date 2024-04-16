
    /* Print "pass\n" and 'exit 0'  */
    .macro      pass
    .data
mpass:
    .string "pass\n"
    .text
_pass:
    lddw        %r1, mpass      /* point to "pass\n" string */
    mov         %r2, 5          /* strlen mpass */
    call        7               /* printk */
    mov         %r0, 0
    exit                        /* exit 0 */
    .endm

/* MACRO fail
   Exit with status 1 */
    .macro fail
    mov %r0, 1
    exit
    .endm

/* MACRO fail_ne32
   Exit with status 1 if \reg32 != \val */
    .macro      fail_ne32 reg val
    jeq32       \reg, \val, 2
    mov         %r0, 1
    exit
    .endm

/* MACRO fail_ne
   Exit with status1 if \reg ne \val */
    .macro      fail_ne reg val
    lddw        %r0, \val
    jeq         \reg, %r0, 2
    mov         %r0, 1
    exit
    .endm
