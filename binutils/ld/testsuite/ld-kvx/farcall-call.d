#name: kvx-farcall-call
#source: farcall-call.s
#as:
#ld: -Ttext 0x1000 --section-start .foo=0x20001000
#objdump: -dr
#...

Disassembly of section .text:


.* <_start>:
.*:	.. .. .. 18                                     	call .* <__bar_veneer>;;

.*:	00 00 d0 0f                                     	ret;;

.* <__bar_veneer>:
.*:	.. .. 40 e0 .. .. .. ..                         	make \$r16 = .* \(0x.*\);;

.*:	10 00 d8 0f                                     	igoto \$r16;;

Disassembly of section .foo:

.* <bar>:
.*:	00 00 d0 0f                                     	ret;;

