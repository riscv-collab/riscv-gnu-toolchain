	.section	.text.c,"ax",%progbits
	.globl	c
	.type	c, %function
c:
.LFB0:
	.section	__patchable_function_entries,"awo",%progbits,.LPFE0
	.dc.a	.LPFE0
	.section	.text.c
.LPFE0:
	.byte	0
	.section	.text.d,"ax",%progbits
	.globl	d
	.type	d, %function
d:
.LFB1:
	.section	__patchable_function_entries,"awo",%progbits,.LPFE1
	.dc.a	.LPFE1
	.section	.text.d
.LPFE1:
	.byte	0
