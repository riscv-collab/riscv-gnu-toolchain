#name: kvx-limit-call
#source: limit-call.s
#as:
#ld: -Ttext 0x0000 --section-start .foo=0x0FFFFFFC
#objdump: -dr
#...

Disassembly of section .text:

.* <_start>:
   0:	ff ff ff 1b                                     	call ffffffc <bar>;;

   4:	00 00 d0 0f                                     	ret;;


Disassembly of section .foo:

.* <bar>:
.*:	00 00 d0 0f                                     	ret;;

