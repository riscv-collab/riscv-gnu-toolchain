#name: kvx-farcall-back
#source: farcall-back.s
#as:
#ld: -Ttext 0x1000 --section-start .foo=0x80001000
#objdump: -dr

#...

Disassembly of section .text:

.* <_start>:
    .*:	.. .. .. ..                                     	call .* <__bar1_veneer>;;

    .*:	.. .. .. ..                                     	goto .* <__bar1_veneer>;;

    .*:	.. .. .. ..                                     	call .* <__bar2_veneer>;;

    .*:	.. .. .. ..                                     	goto .* <__bar2_veneer>;;

    .*:	.. .. .. ..                                     	call .* <__bar3_veneer>;;

    .*:	.. .. .. ..                                     	goto .* <__bar3_veneer>;;

    .*:	00 00 d0 0f                                     	ret;;

	...

.* <_back>:
    .*:	00 00 d0 0f                                     	ret;;

.* <__bar3_veneer>:
    .*:	00 .. 40 e0 0c 00 20 00                         	make \$r16 = .* \(0x.*\);;

    .*:	10 00 d8 0f                                     	igoto \$r16;;

.* <__bar2_veneer>:
    .*:	00 .. 40 e0 08 00 20 00                         	make \$r16 = .* \(0x.*\);;

    .*:	10 00 d8 0f                                     	igoto \$r16;;

.* <__bar1_veneer>:
    .*:	00 .. 40 e0 04 00 20 00                         	make \$r16 = .* \(0x.*\);;

    .*:	10 00 d8 0f                                     	igoto \$r16;;


Disassembly of section .foo:

.* <bar1>:
.*:	00 00 d0 0f                                     	ret;;

.*:	.. .. .. ..                                     	goto .* <___start_veneer>;;

	...

.* <bar2>:
.*:	00 00 d0 0f                                     	ret;;

.*:	.. .. .. ..                                     	goto .* <___start_veneer>;;

	...

.* <bar3>:
.*:	00 00 d0 0f                                     	ret;;

.*:	.. .. .. ..                                     	goto .* <___back_veneer>;;


.* <___start_veneer>:
.*:	00 .. 40 e0 04 00 00 00                         	make \$r16 = .* \(0x.*\);;

.*:	10 00 d8 0f                                     	igoto \$r16;;

.* <___back_veneer>:
.*:	00 .. 40 e0 08 00 00 00                         	make \$r16 = .* \(0x.*\);;

.*:	10 00 d8 0f                                     	igoto \$r16;;


