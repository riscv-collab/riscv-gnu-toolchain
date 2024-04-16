#name: kvx-farcall-goto-plt
#source: farcall-goto-plt.s
#as:
#ld: -shared
#objdump: -dr
#...

Disassembly of section .plt:

.* <.plt>:
	...

.* <foo@plt>:
.*:	10 00 c4 0f                                     	get \$r16 = \$pc;;

.*:	.. .. 40 .. .. .. .. ..                         	l[wzd]* \$r16 = [0-9]* \(0x[0-9a-b]*\)\[\$r16\];;

.*:	10 00 d8 0f                                     	igoto \$r16;;


Disassembly of section .text:

.* <_start>:
	...
.*:	.. .. .. 10                                     	goto .* <__foo_veneer>;;

.*:	00 00 d0 0f                                     	ret;;


.* <__foo_veneer>:
.*:	.. .. 40 e0 00 00 00 00                         	make \$r16 = [0-9]* \(0x[0-9a-b]*\);;

.*:	10 00 d8 0f                                     	igoto \$r16;;

