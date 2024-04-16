.L1:
	.global var
	#TLSDESC large abs
	lu12i.w		$a0,%desc_hi20(var)
	ori		$a0,$a0,%desc_lo12(var)
	lu32i.d		$a0,%desc64_lo20(var)
	lu52i.d		$a0,$a0,%desc64_hi12(var)
	ld.d		$ra,$a0,%desc_ld(var)
	jirl		$ra,$ra,%desc_call(var)

	# Test macro expansion
	la.tls.desc	$a0,var
