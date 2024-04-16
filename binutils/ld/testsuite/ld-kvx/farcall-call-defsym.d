#name: kvx-farcall-call-defsym
#source: farcall-call-defsym.s
#as:
#ld: -Ttext 0x1000 --defsym=bar=0x8001000
#objdump: -dr
#...

Disassembly of section .text:

.* <_start>:
    1000:	00 00 00 1a                                     	call 8001000 <bar>;;

    1004:	00 00 d0 0f                                     	ret;;

