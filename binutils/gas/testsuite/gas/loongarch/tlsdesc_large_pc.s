.L1:
	# R_LARCH_TLS_DESC_PC_HI20 var
	pcalau12i       $a0,%desc_pc_hi20(var)
	# R_LARCH_TLS_DESC_PC_LO12
	addi.d		$a1,$zero,%desc_pc_lo12(var)
	# R_LARCH_TLS_DESC64_PC_LO20
	lu32i.d		$a1,%desc64_pc_lo20(var)
	# R_LARCH_TLS_DESC64_PC_HI12
	lu52i.d		$a1,$a1,%desc64_pc_hi12(var)
	add.d		$a0,$a0,$a1
	# R_LARCH_TLS_DESC_LD
	ld.d		$ra,$a0,%desc_ld(var)
	# R_LARCH_TLS_DESC
	jirl		$ra,$ra,%desc_call(var)

	# Test macro expansion
	la.tls.desc	$a0,$ra,var
