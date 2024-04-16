        .global _start
        .text
_start:
        nop

	.section .note
        .long 1f - 0f           /* name length */
        .long 2f - 1f           /* data length */
	.long 3                 /* note type */
0:	.asciz "QNX"            /* vendore name */
1:	.long 0x4321
	.long 0x1234
	.long 0x0
2:
