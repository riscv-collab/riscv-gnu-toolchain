# Check Illegal AMX-COMPLEX instructions

	.text
_start:
	tcmmimfp16ps	%tmm1, %tmm2, %tmm3
	tcmmrlfp16ps	%tmm1, %tmm2, %tmm3
