	.data
	.section	.tdata,"awT",@progbits
var:
	.word 1
	.text
	.global	fn1
	.type	gn1,@function
fn1:
	# expect IE to relax LE in nomal cmodel.
	la.tls.ie	$a0,var
	# extreme cmodel do not do transition.
	la.tls.ie	$a0,$a1,var
