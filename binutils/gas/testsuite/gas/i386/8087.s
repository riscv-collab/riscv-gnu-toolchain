# Check 8087-only instructions.

	.text
	.code16
	.arch i8086
	.arch .8087
_8087:
	fdisi
	feni
	fndisi
	fneni
