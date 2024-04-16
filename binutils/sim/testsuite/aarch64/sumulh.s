# mach: aarch64

# Check the multiply highpart instructions: smulh, umulh.

# Test -2*2, -1<<32*-1<<32, -2*-2, and 2*2.

.include "testutils.inc"

	start

	mov x0, #-2
	mov x1, #2
	smulh x2, x0, x1
	cmp x2, #-1
	bne .Lfailure
	umulh x3, x0, x1
	cmp x3, #1
	bne .Lfailure

	mov w0, #-1
	lsl x0, x0, #32 // 0xffffffff00000000
	mov x1, x0
	smulh x2, x0, x1
	cmp x2, #1
	bne .Lfailure
	umulh x3, x0, x1
	mov w4, #-2
	lsl x4, x4, #32
	add x4, x4, #1  // 0xfffffffe00000001
	cmp x3, x4
	bne .Lfailure

	mov x0, #-2
	mov x1, #-2
	smulh x2, x0, x1
	cmp x2, #0
	bne .Lfailure
	umulh x3, x0, x1
	cmp x3, #-4
	bne .Lfailure

	mov x0, #2
	mov x1, #2
	smulh x2, x0, x1
	cmp x2, #0
	bne .Lfailure
	umulh x3, x0, x1
	cmp x3, #0
	bne .Lfailure

	pass
.Lfailure:
	fail
