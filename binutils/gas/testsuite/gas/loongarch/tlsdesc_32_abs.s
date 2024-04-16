.L1:
	lu12i.w		$a0,%desc_hi20(var)
	ori		$a0,$a0,%desc_lo12(var)
	ld.w		$ra,$a0,%desc_ld(var)
	jirl		$ra,$ra,%desc_call(var)

	# Test macro expansion
	la.tls.desc	$a0,var
