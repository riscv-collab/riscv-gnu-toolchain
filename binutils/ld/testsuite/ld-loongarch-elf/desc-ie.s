	.global v1
	.section .tdata,"awT",@progbits
v1:
	.word 1
	.text
	.global	fn1
	.type	fn1,@function
fn1:

	# Use DESC and IE to access the same symbol,
	# DESC will relax to IE.
	la.tls.desc $a0,var
	la.tls.ie   $a0,var

	# extreme cmodel do not do transition.
	la.tls.desc $a0,$a1,var
	la.tls.ie   $a0,$a1,var
