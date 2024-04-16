# check that the sim doesn't die immediately.
# mach: m32c
# ld: -T$srcdir/$subdir/sample.ld
# xerror:

.include "testutils.inc"

	start
	fail
