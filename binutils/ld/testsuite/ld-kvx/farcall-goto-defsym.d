#name: kvx-farcall-goto-defsym
#source: farcall-goto-defsym.s
#as:
#ld: -Ttext 0x1000 --defsym=bar=0x8001000
#objdump: -dr
#...

Disassembly of section .text:

.* <_start>:
    1000:	00 00 00 12                                     	goto 8001000 <bar>;;

    1004:	00 00 d0 0f                                     	ret;;

