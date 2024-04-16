# Basic jalr tests.
# mach: riscv

.include "testutils.inc"

	start

	# Load desination into register a0.
	la	a0, good_dest

	# Jump to the destination in a0.
	jalr	a0, a0, 0

	# If we write destination into a0 before reading it in order
	# to jump, we might end up here.
bad_dest:
	fail

	# We should end up here.
good_dest:
	pass
	fail
