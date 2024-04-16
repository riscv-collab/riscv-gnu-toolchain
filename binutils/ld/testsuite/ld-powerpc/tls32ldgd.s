#PR 30697
	.section ".tbss","awT",@nobits
	.global _start,x
	.hidden x
	.align 2
x:	.space 4

	.text
_start:
 addi 3,30,x@got@tlsgd
 bl __tls_get_addr(x@tlsgd)@plt

 addi 3,30,x@got@tlsld
 bl __tls_get_addr(x@tlsld)@plt
 addis 3,3,x@dtprel@ha
 addi 3,3,x@dtprel@l
