.text
.globl _start
_start:
	la.pcrel $a0, $t0, sym
	jr $ra
.data
sym:
	.dword 0
