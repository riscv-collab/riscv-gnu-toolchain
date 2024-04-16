# output(): Hello world!\n
# mach(): cr16 

	.globl _start
_start:

# write (hello world)
	movw $1,r2
	movd $hello,(r4,r3)
	loadw length,r5
	movw $0x404,r0
	excp 8
# exit (0)
	movw $0,r2
	movw $0x410,r0
	excp 8

length:	.long 14
hello:	.ascii "Hello world!\r\n"
