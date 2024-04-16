# mach: aarch64

# Check the basic integer compare instructions: adds, adds64, subs, subs64.
# For add, check value pairs 1 and -1 (Z), -1 and -1 (N), 2 and -1 (C),
# and MIN_INT and -1 (V), 
# Also check -2 and 1 (not C).
# For sub, negate the second value.

.include "testutils.inc"

	start
	mov w0, #1
	mov w1, #-1
	adds w2, w0, w1
	bne .Lfailure
	mov w0, #-1
	mov w1, #-1
	adds w2, w0, w1
	bpl .Lfailure
	mov w0, #2
	mov w1, #-1
	adds w2, w0, w1
	bcc .Lfailure
	mov w0, #0x80000000
	mov w1, #-1
	adds w2, w0, w1
	bvc .Lfailure
	mov w0, #-2
	mov w1, #1
	adds w2, w0, w1
	bcs .Lfailure

	mov x0, #1
	mov x1, #-1
	adds x2, x0, x1
	bne .Lfailure
	mov x0, #-1
	mov x1, #-1
	adds x2, x0, x1
	bpl .Lfailure
	mov x0, #2
	mov x1, #-1
	adds x2, x0, x1
	bcc .Lfailure
	mov x0, #0x8000000000000000
	mov x1, #-1
	adds x2, x0, x1
	bvc .Lfailure
	mov x0, #-2
	mov x1, #1
	adds x2, x0, x1
	bcs .Lfailure

	mov w0, #1
	mov w1, #1
	subs w2, w0, w1
	bne .Lfailure
	mov w0, #-1
	mov w1, #1
	subs w2, w0, w1
	bpl .Lfailure
	mov w0, #2
	mov w1, #1
	subs w2, w0, w1
	bcc .Lfailure
	mov w0, #0x80000000
	mov w1, #1
	subs w2, w0, w1
	bvc .Lfailure
	mov w0, #-2
	mov w1, #-1
	subs w2, w0, w1
	bcs .Lfailure

	mov x0, #1
	mov x1, #1
	subs x2, x0, x1
	bne .Lfailure
	mov x0, #-1
	mov x1, #1
	subs x2, x0, x1
	bpl .Lfailure
	mov x0, #2
	mov x1, #1
	subs x2, x0, x1
	bcc .Lfailure
	mov x0, #0x8000000000000000
	mov x1, #1
	subs x2, x0, x1
	bvc .Lfailure
	mov x0, #-2
	mov x1, #-1
	subs x2, x0, x1
	bcs .Lfailure

	pass
.Lfailure:
	fail
