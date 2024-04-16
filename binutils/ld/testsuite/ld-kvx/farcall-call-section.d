#name: kvx-farcall-call-section
#source: farcall-call-section.s
#as:
#ld: -Ttext 0x1000 --section-start .foo=0x20001000
#objdump: -dr
#...

Disassembly of section .text:

.* <_start>:
.*:	.. .. .. ..                                     	call .* <___veneer>;;

.*:	.. .. .. ..                                     	call .* <___veneer>;;

.*:	00 00 d0 0f                                     	ret;;

.* <___veneer>:
.*:	.. 00 40 e0 .. .. .. ..                         	make \$r16 = .* \(0x.*\);;

.*:	10 00 d8 0f                                     	igoto \$r16;;

.* <___veneer>:
.*:	.. 01 40 e0 .. .. .. ..                         	make \$r16 = .* \(0x.*\);;

.*:	10 00 d8 0f                                     	igoto \$r16;;

Disassembly of section .foo:

.* <bar>:
.*:	00 00 d0 0f                                     	ret;;

.* <bar2>:
.*:	00 00 d0 0f                                     	ret;;

