# Check 64bit FRED instructions

	.text
_start:
	erets		 #FRED
	eretu		 #FRED

.intel_syntax noprefix
	erets		 #FRED
	eretu		 #FRED
