	.text
	.globl	a
	.type	a, %function
a:
.LFB0:
	.section	__patchable_function_entries,"awo",%progbits,.LPFE0
	.dc.a	.LPFE0
	.text
.LPFE0:
	.byte	0
	.text
	.globl	b
	.type	b, %function
b:
.LFB1:
	.section	__patchable_function_entries,"awo",%progbits,.LPFE1
	.dc.a	.LPFE1
	.text
.LPFE1:
	.byte	0
