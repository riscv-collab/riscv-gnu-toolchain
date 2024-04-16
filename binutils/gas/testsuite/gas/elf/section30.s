	.macro func name:req
	.pushsection %S.\name, "+"
	.type \name, %function
	.global \name
	.hidden \name
\name:
	.endm

	.macro data name:req
	.pushsection %S.\name, "+"
	.type \name, %object
\name:
	.endm

	.macro end name:req
	.size \name, . - \name
	.popsection
	.endm


	.text
	func func1
	.nop
	end func1

	func func2
	.nop
	.nop
	end func2

	.data
	data data1
	.byte 1
	end data1

	.section .bss
	data data2
	.skip 2
	end data2

	.section .rodata, "a", %progbits
	data data3
	.byte 3, 3, 3
	end data3

	.section .rodata.str1.1, "aMS", %progbits, 1
	data str1
	.asciz "string1"
	end str1

	.section .rodata.2, "ao", %progbits, func1
	data data4
	.byte 4, 4, 4, 4
	end data4
	.pushsection .bss.data5, "-o", %nobits
	.type data5, %object
data5:	.fill 5
	end data5

	.section .rodata.3, "aG", %progbits, sig1, comdat
	data data6
	.byte 6, 6, 6, 6, 6, 6
	end data6
	.pushsection .bss.data7, "-G", %nobits
	.type data7, %object
data7:	.skip 7
	end data7
