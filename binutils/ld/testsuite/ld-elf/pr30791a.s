	.section	.text.a,"ax",%progbits
	.globl	a
	.type	a, %function
a:
.LFB0:
	.section	__patchable_function_entries,"awo",%progbits,.LPFE0
	.dc.a	.LPFE0
	.section	.text.a
.LPFE0:
	.byte	0
	.section	.text.b,"ax",%progbits
	.globl	b
	.type	b, %function
b:
.LFB1:
	.section	__patchable_function_entries,"awo",%progbits,.LPFE1
	.dc.a	.LPFE1
	.section	.text.b
.LPFE1:
	.byte	0
