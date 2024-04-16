# Check 287-only instructions.

	.text
	.code16
	.arch i286
	.arch .287
_8087:
	fnsetpm
	frstpm
	fsetpm
