# mach: bpf
# sim: --skb-data-offset=0x20
# output: pass\nexit 0 (0x0)\n
/* ldabs.s
   Tests for non-generic BPF load instructions in simulator.
   These instructions (ld{abs,ind}{b,h,w,dw}) are used to access
   kernel socket data from BPF programs for high performance filters.

   Register r6 is an implicit input holding a pointer to a struct sk_buff.
   Register r0 is an implicit output, holding the fetched data.

   e.g.
   ldabsw means:
   r0 = ntohl (*(u32 *) (((struct sk_buff *)r6)->data + imm32))

   ldindw means
   r0 = ntohl (*(u32 *) (((struct sk_buff *)r6)->data + src_reg + imm32))  */

    .include "testutils.inc"

    .text
    .global main
    .type main, @function
main:
    /* R6 holds a pointer to a struct sk_buff, which we pretend
       exists at 0x1000  */
    mov         %r6, 0x1000

    /* We configure skb-data-offset=0x20
       This specifies offsetof(struct sk_buff, data), where the field 'data'
       is a pointer a data buffer, in this case at 0x2000  */
    stw         [%r6+0x20], 0x2000

    /* Write the value 0x7eadbeef into memory at 0x2004
       i.e. offset 4 within the data buffer pointed to by
       ((struct sk_buff *)r6)->data  */
    stw         [%r6+0x1004], 0x0eadbeef

    /* Now load data[4] into r0 using the ldabsw instruction  */
    ldabsw      0x4

    /* ...and compare to what we expect  */
    fail_ne32   %r0, 0x0eadbeef

    /* Repeat for a half-word (2-bytes)  */
    sth         [%r6+0x1008], 0x1234
    ldabsh      0x8
    fail_ne32   %r0, 0x1234

    /* Repeat for a single byte  */
    stb         [%r6+0x1010], 0x5a
    ldabsb      0x10
    fail_ne32   %r0, 0x5a

    /* Repeat for a double-word (8-byte)
       (note: fail_ne macro uses r0, so copy to another r1 to compare)  */
    lddw        %r2, 0x1234deadbeef5678
    stxdw       [%r6+0x1018], %r2
    ldabsdw     0x18
    mov         %r1, %r0
    fail_ne     %r1, 0x1234deadbeef5678

    /* Now, we do the same for the indirect loads  */
    mov         %r7, 0x100
    stw         [%r6+0x1100], 0x0eedbeef

    ldindw      %r7, 0x0
    fail_ne32   %r0, 0x0eedbeef

    /* half-word */
    sth         [%r6+0x1104], 0x6789
    ldindh      %r7, 0x4
    fail_ne32   %r0, 0x6789

    /* byte  */
    stb         [%r6+0x1108], 0x5f
    ldindb      %r7, 0x8
    fail_ne32   %r0, 0x5f

    /* double-word  */
    lddw        %r2, 0xcafe12345678d00d
    stxdw       [%r6+0x1110], %r2
    ldinddw     %r7, 0x10
    mov         %r1, %r0
    fail_ne     %r1, 0xcafe12345678d00d

    pass
