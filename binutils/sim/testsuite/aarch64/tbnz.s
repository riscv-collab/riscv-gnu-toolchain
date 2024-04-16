# mach: aarch64

# Check the test-bit-and-branch instructions: tbnz, and tbz.
# We check the edge condition bit positions: 0, 1<<31, 1<<32, 1<<63.

.include "testutils.inc"

	start
	mov x0, #1
	tbnz x0, #0, .L1
	fail
.L1:
	tbz x0, #0, .Lfailure
	mov x0, #0xFFFFFFFFFFFFFFFE
	tbnz x0, #0, .Lfailure
	tbz x0, #0, .L2
	fail
.L2:

	mov x0, #0x80000000
	tbnz x0, #31, .L3
	fail
.L3:
	tbz x0, #31, .Lfailure
	mov x0, #0xFFFFFFFF7FFFFFFF
	tbnz x0, #31, .Lfailure
	tbz x0, #31, .L4
	fail
.L4:

	mov x0, #0x100000000
	tbnz x0, #32, .L5
	fail
.L5:
	tbz x0, #32, .Lfailure
	mov x0, #0xFFFFFFFEFFFFFFFF
	tbnz x0, #32, .Lfailure
	tbz x0, #32, .L6
	fail
.L6:

	mov x0, #0x8000000000000000
	tbnz x0, #63, .L7
	fail
.L7:
	tbz x0, #63, .Lfailure
	mov x0, #0x7FFFFFFFFFFFFFFF
	tbnz x0, #63, .Lfailure
	tbz x0, #63, .L8
	fail
.L8:

	pass
.Lfailure:
	fail
